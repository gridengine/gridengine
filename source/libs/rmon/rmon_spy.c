/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
#define MAINPROGRAM
#define DEBUG

#include "rmon_h.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include "rmon_err.h"
#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_semaph.h"
#include "rmon_siginit.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_daemon.h"
#include "rmon_server.h"

#include "rmon_job_list.h"
#include "rmon_job_protocol.h"

#include "rmon_message_list.h"
#include "rmon_message_protocol.h"

/* spy cases */
#include "rmon_s_c_exit.h"
#include "rmon_s_c_flush.h"
#include "rmon_s_c_sleep.h"
#include "rmon_s_c_mconf.h"
#include "rmon_s_c_wake_up.h"
#include "rmon_s_c_overflow.h"
#include "rmon_s_c_spy_register.h"
#include "rmon_s_c_monitoring_level.h"

#include "rmon_spy.h"

extern u_long max_messages;
extern char rmond[];
extern u_long name_is_valid;

monitoring_box_type *monitoring_box_down;
int semid_down, shmid_down;

/* ------------------------- global variables ------------------------- */

string programname;

int sfd0, pfdtospy[2];
int mlevel = 0;
int state;
u_long message_counter = 0;
volatile int SFD = 0;
u_long all_jobs = 0;
u_long addr, port;
volatile u_long childdeath = 0;
int shmkey_down, semkey_down;

message_list_type *last_element = NULL;
struct servent *sp;
struct sockaddr_in from;
int sfd;

/* --------------------------- PROTOtypes  ---------------------------- */

int main(int, char **);
static void parent(void);
static void child(char *argv[], int argc, int i);
static void timeouthandling(void);
static void terminate(int);
static void sig_int(int);
static void shut_down_on_kill(void);
static void shut_down_on_int(void);
static void shut_down_on_childdeath(void);
static int spy_mayclose(int fd);
static int parser(int argc, char **argv);
static void usage(void);

int rmon_d_writenbytes(register int sfd, register char *ptr, register int n);
/**************** FUNCTIONS *********************************************/

int main(
int argc,
char *argv[] 
) {
   int usercommand;
   struct hostent *he;
   struct in_addr bert;
   string myname;

#undef FUNC
#define FUNC "main"
   DOPEN("spy");
   DENTER;

   rmon_init_alarm();
   rmon_init_terminate(terminate);
   rmon_init_sig_int(sig_int);

   usercommand = parser(argc, argv);

   if (!name_is_valid)
      rmon_read_configuration();

   if (gethostname(myname, STRINGSIZE) < 0)
      rmon_errf(TERMINAL, MSG_RMON_UNABLETOGETMYHOSTNAMEX_S, sys_errlist[errno]);

   if ((he = gethostbyname(myname)) == NULL)
      rmon_errf(TERMINAL, MSG_RMON_UNABLETORESOLVEMYHOSTNAMEXY_SS, myname, sys_errlist[errno]);
   bcopy((char *) he->h_addr, (char *) &addr, he->h_length);

   bert.s_addr = addr;
   /* printf(" ------------------------ \n"); */
   /* printf(" HOSTNAME:  %s\n", myname ); */
   /* printf(" inet_addr: %s\n", inet_ntoa(bert)); */
   /* printf(" ------------------------ \n"); */

   if (!(sp = getservbyname(RMOND_SERVICE, "tcp")))
      rmon_errf(TERMINAL, MSG_RMON_XBADSERVICE_S, RMOND_SERVICE);

   /* Create a pipe for reading from knecht */
   if (pipe(pfdtospy) != 0)
      rmon_errf(TERMINAL, MSG_RMON_CANTOPENPIPESX_S, sys_errlist[errno]);

   if (fcntl(pfdtospy[READ], F_SETFL, O_NDELAY) == -1)
      rmon_errf(TERMINAL, MSG_RMON_FCNTLFAILURE_S, sys_errlist[errno]);

   if (fcntl(pfdtospy[WRITE], F_SETFL, O_NDELAY) == -1)
      rmon_errf(TERMINAL, MSG_RMON_FCNTLFAILURE_S, sys_errlist[errno]);

   /* Get an exclusive shared memory segment for IPC to knecht */
   shmkey_down = SHMKEY_BASE;
   do {
      if ((shmid_down = shmget(shmkey_down, sizeof(monitoring_box_type), PERMS | IPC_CREAT | IPC_EXCL)) != -1)
         DPRINTF(("shmid_down: %d\n", shmid_down));
      if (shmid_down == -1 && errno == EEXIST)
         shmkey_down++;
   } while (shmid_down == -1 && errno == EEXIST);
   if (shmid_down == -1)
      rmon_errf(TERMINAL, MSG_RMON_SHMGETFAILED_S, sys_errlist[errno]);

   monitoring_box_down = (monitoring_box_type *) shmat(shmid_down, NULL, 0);

   if (monitoring_box_down == NULL)
      rmon_errf(TERMINAL, MSG_RMON_SHMATFAILED_S, sys_errlist[errno]);

   monitoring_box_down->valid = 0;
   monitoring_box_down->count = 0;
   monitoring_box_down->quit = 0;
   monitoring_box_down->programname_is_valid = 0;

   /* Get an exclusive semaphore for save access on monitoring box */
   semkey_down = SEMKEY_BASE;
   do {
      if ((semid_down = rmon_sem_create(semkey_down, 1, IPC_EXCL)) != -1)
         DPRINTF(("semkey_down: %d\n", semkey_down));

      if (semid_down == -1 && errno == EEXIST)
         semkey_down++;
   } while (semid_down == -1 && errno == EEXIST);

   if (semid_down == -1) {
      rmon_shut_down_on_error();
      switch (errno) {
      case ENOSPC:
         rmon_errf(TERMINAL, MSG_RMON_NEEDMORESEMAPHORES);
      default:
         rmon_errf(TERMINAL, MSG_RMON_RMONSEMCREATEFAILED_S, sys_errlist[errno]);
      }
   }

   if (putenv("SGE_DEBUG_LEVEL=RMON_REMOTE"))
      rmon_errf(TERMINAL, MSG_RMON_CANTPUTENV_S, sys_errlist[errno]);

   if (!fork()) {
      close(pfdtospy[READ]);
      child(argv, argc, usercommand);
   }
   else {
      close(pfdtospy[WRITE]);
      parent();
   }

   DEXIT;
   return 1;
}

static void child(
char *argv[],
int argc,
int i 
) {
   char **newargv;              /* list of arguments                                            */
   char read_channel[8];        /* pipe-read-channel converted to string        */
   char write_channel[8];       /* pipe-write-channel converted to string       */
   char semaphore[8];

   monitoring_box_type *monitoring_box_up;
   int shmid_up, semid_up;
   piped_message_type piped_message;

   long _nfile;

#undef FUNC
#define FUNC "child"
   DENTER;

   argv = &(argv[i]);           /* Skip first i parameters (noramlly i=2) */
   argc -= i;

   sprintf(semaphore, "%d", semkey_down);       /* convert int to string */
   sprintf(read_channel, "%d", shmkey_down);    /* convert int to string */
   sprintf(write_channel, "%d", pfdtospy[WRITE]);       /* convert int to string */

   newargv = (char **) malloc(sizeof(char *) * (argc + 4));
   for (i = 0; i < argc; i++)
      newargv[i] = argv[i];

   newargv[argc++] = semaphore;
   newargv[argc++] = read_channel;
   newargv[argc++] = write_channel;
   newargv[argc] = NULL;

   DEXIT;

   if (((_nfile = sysconf(_SC_OPEN_MAX))) == -1) {
      rmon_errf(TRIVIAL, MSG_RMON_UNABLETOSYSCONFSCOPENMAX);
      DEXIT;
      return;
   }

   for (i = 3; i < _nfile - 1; i++)     /* 3 why ?!? */
      if (i != pfdtospy[WRITE])
         close(i);

   execv(newargv[0], newargv);  /* execute the program `argv[i]` */

   /* if this message is printed, there must have been an error ! */

   shmid_up = shmget(shmkey_down, sizeof(monitoring_box_type), PERMS | IPC_CREAT);
   if (shmid_up == -1)
      rmon_errf(TRIVIAL, MSG_RMON_SHMGETFAILED_S);

   monitoring_box_up = (monitoring_box_type *) shmat(shmid_up, NULL, 0);
   if (monitoring_box_up == NULL)
      rmon_errf(TRIVIAL, MSG_RMON_SHMATFAILED_S);

   semid_up = rmon_sem_open(semkey_down);
   if (semid_up == -1)
      rmon_errf(TRIVIAL, MSG_RMON_RMONSEMCREATEFAILED);

   piped_message.pid = (u_long) getpid();
   piped_message.childdeath = 1;
   for (;;) {
      if (!rmon_sem_wait(semid_up))
         rmon_errf(TRIVIAL, MSG_RMON_CANTRMONSEMWAIT1);

      if (!rmon_d_writenbytes(pfdtospy[WRITE], (char *) &piped_message, sizeof(piped_message_type))) {

         monitoring_box_up->count++;
         if (!rmon_sem_signal(semid_up))
            rmon_errf(TRIVIAL, MSG_RMON_CANTRMONSEMSIGNAL1);
         break;
      }
      if (!rmon_sem_signal(semid_up))
         rmon_errf(TRIVIAL, MSG_RMON_CANTRMONSEMSIGNAL2);

      sleep(1);
   }

   if (!rmon_sem_close(semid_up))
      rmon_errf(TRIVIAL, MSG_RMON_CANTRMONSEMCLOSE1);
   shmdt((char *) monitoring_box_up);

   rmon_errf(TERMINAL, MSG_RMON_CANTEXECUTEPROGXY_SS, newargv[0], sys_errlist[errno]);
   DEXIT;
   DCLOSE;
   exit(0);
}

static void parent()
{
#undef FUNC
#define FUNC "parent"
   int valid = 0;
   int maxfd, fromlen;
   fd_set readfds;
   struct timeval time, *time_ptr;
   piped_message_type piped_message;

   DENTER;

   fromlen = sizeof(from);

   rmon_daemon(spy_mayclose);

   port = SPY_BASE_PORT;
   rmon_startup_spy_server(&port);

   do {
      rmon_sem_wait(semid_down);

      if ((valid = monitoring_box_down->programname_is_valid) == 1)
         strncpy(programname, monitoring_box_down->programname, STRINGSIZE - 1);

      rmon_sem_signal(semid_down);

      if (!valid)
         sleep(1);

   } while (!valid);

   /* try to register at rmond */
   if (!rmon_s_c_spy_register()) {
   }
   else {
   }

   /* *********************************************************** */
   /* **                  MAIN LOOP                            ** */
   /* *********************************************************** */

   for (;;) {

      /* Reset a timeout-period for the select (coming later) */
      time.tv_sec = 0;
      time.tv_usec = 0;

      if (childdeath)
         time_ptr = &time;
      else
         time_ptr = NULL;

      /* Clear the Bitmask for the select (coming later) */
      FD_ZERO(&readfds);

      if (!childdeath && message_counter < max_messages) {
         /* set the bits corresponding to the sockets you want to wait for. */
         FD_SET(sfd0, &readfds);
         FD_SET(pfdtospy[READ], &readfds);

         maxfd = (pfdtospy[READ] > sfd0) ? pfdtospy[READ] : sfd0;
      }
      else {
         FD_SET(sfd0, &readfds);
         maxfd = sfd0;
      }

      /* check all sockets wether there is one ready for reading. */
      /* When a timeout occurs, select returns a 0. */

      if (select(maxfd + 1, TYPE_OF_READFDS & readfds, NULL, NULL, time_ptr) == 0) {
         timeouthandling();
      }

      /* has something arrived */
      if (FD_ISSET(sfd0, &readfds) | FD_ISSET(pfdtospy[READ], &readfds)) {
         /* is the `sfd0` (the tcp-socket) ready to read from ? */
         if (FD_ISSET(sfd0, &readfds)) {
            /* accept the connection on a new socket */
            if ((sfd = accept(sfd0, (struct sockaddr *) &from, &fromlen)) < 0) {
               if (errno == EINTR) {
                  DPRINTF(("EINTR while accepting connection\n"));
                  continue;
               }

               /*      rmon_errf(TERMINAL, "accept error\n"); */
            }

            DPRINTF(("Accepted new connection\n"));
            if (!rmon_get_request(sfd)) {
               rmon_errf(TRIVIAL, MSG_RMON_ERRORREADINGREQUEST);
               shutdown(sfd, 2);
               close(sfd);
               continue;
            }

            switch (request.type) {

            case MONITORING_LEVEL:
               DPRINTF(("MONITORING_LEVEL\n"));
               if (!rmon_s_c_monitoring_level(sfd))
                  DPRINTF(("ERROR in s_c_monitoring_level\n"));

               break;

            case MESSAGE_FLUSH:
               DPRINTF(("MESSAGE_FLUSH\n"));
               if (!rmon_s_c_flush(sfd))
                  DPRINTF(("ERROR in s_c_flush\n"));
               break;

            case SLEEP:
               DPRINTF(("SLEEP\n"));
               if (!rmon_s_c_sleep(sfd, &state))
                  DPRINTF(("ERROR in s_c_sleep\n"));
               break;

            case WAKE_UP:
               DPRINTF(("WAKE_UP\n"));
               if (!rmon_s_c_wake_up(sfd, &state))
                  DPRINTF(("ERROR in s_c_wake_up\n"));
               break;

            case MCONF:
               DPRINTF(("MCONF\n"));
               if (!rmon_s_c_mconf(sfd))
                  DPRINTF(("ERROR in s_c_mconf\n"));
               break;

            case MQUIT:
               DPRINTF(("MQUIT\n"));
               rmon_send_ack(sfd, S_ACCEPTED);
               shut_down_on_kill();

               close(pfdtospy[READ]);
               shutdown(sfd0, 2);

               close(sfd0);

               /* missing: wait until knecht achnowledged my end */
               if (shmdt((char *) monitoring_box_down) < 0)
                  rmon_errf(TERMINAL,  MSG_RMON_ERRORINSHMDT_S,
                            sys_errlist[errno]);

               if (childdeath)
                  if (shmctl(shmid_down, IPC_RMID, NULL) < 0)
                     rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMCTL_S,
                               sys_errlist[errno]);
               if (!childdeath)
                  rmon_sem_close(semid_down);

               DEXIT;
               DCLOSE;
               exit(0);

            default:
               DPRINTF(("Unknown request type %ld\n", request.type));

            }                   /* switch */

            shutdown(sfd, 2);
            close(sfd);

         }
         /* is the `pfdtospy[READ]` (the pipe-socket) ready to read from ? */
         else {

            DPRINTF(("From PIPE:\n"));

            rmon_sem_wait(semid_down);
            if (monitoring_box_down->count > 0) {
               sfd = pfdtospy[READ];

               for (; monitoring_box_down->count > 0; monitoring_box_down->count--) {

                  if (!rmon_get_piped_message(sfd, &piped_message)) {
                     DPRINTF(("Error in get_piped_message\n"));
                  }
                  else {
                     DPRINTF(("MESSAGE_OVERFLOW\n"));
                     if (!rmon_s_c_overflow(&piped_message)) {
                        DPRINTF(("ERROR in s_c_message_overflow\n"));
                     }
                  }
               }
            }
            rmon_sem_signal(semid_down);

         }                      /* else (PIPE) */
      }                         /* if ( something has arrived ) */
   }                            /* for (;;) */
}
/***************************************************************************/

static int spy_mayclose(
int fd 
) {
   return (fd != pfdtospy[READ]) && rmon_mmayclose(fd);
}

/***************************************************************************/

static void timeouthandling()
{
   int i = 0;
#undef FUNC
#define FUNC "timeouthandling"
   DENTER;

   if (childdeath)
      i = rmon_s_c_exit();
   if (i || (!i && state == STATE_SLEEPING) || (!i && errval == ECONNREFUSED))
      shut_down_on_childdeath();

   DEXIT;
}

/***************************************************************************/

static void shut_down_on_childdeath()
{
#undef FUNC
#define FUNC "shut_down_on_childdeath"
   DENTER;
   rmon_sem_rm(semid_down);

   if (shmctl(shmid_down, IPC_RMID, NULL) < 0)
      rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMCTL_S, sys_errlist[errno]);

   close(pfdtospy[READ]);

   shutdown(sfd, 2);
   close(sfd);

   shutdown(sfd0, 2);
   close(sfd0);

   DEXIT;
   DCLOSE;
   exit(0);
}

/***************************************************************************/

static void terminate(
int n 
) {
#undef  FUNC
#define FUNC "terminate"
   DENTER;

   rmon_s_c_exit();
   shut_down_on_kill();

   DEXIT;
}

static void sig_int(
int n 
) {
#undef  FUNC
#define FUNC "sig_int"
   DENTER;

   rmon_s_c_exit();
   shut_down_on_int();

   DEXIT;
}

/***************************************************************************/

static void shut_down_on_int(void) {
#undef  FUNC
#define FUNC "shut_down_on_int"
   DENTER;

   close(pfdtospy[READ]);

   shutdown(sfd, 2);
   close(sfd);

   shutdown(sfd0, 2);
   close(sfd0);

   if (!childdeath) {
      rmon_sem_wait(semid_down);

      rmon_mlclr(&monitoring_box_down->level);
      monitoring_box_down->valid = 1;
      monitoring_box_down->quit = 1;
      rmon_sem_signal(semid_down);
   }

   rmon_sem_close(semid_down);

   if (shmdt((char *) monitoring_box_down) < 0)
      rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMDT_S, sys_errlist[errno]);

   rmon_sem_rm(semid_down);

   if (shmctl(shmid_down, IPC_RMID, NULL) < 0)
      rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMCTL_S, sys_errlist[errno]);

   DEXIT;
   DCLOSE;
   exit(0);
}

/***************************************************************************/

static void shut_down_on_kill()
{
#undef  FUNC
#define FUNC "shut_down_on_kill"
   DENTER;

   close(pfdtospy[READ]);

   shutdown(sfd, 2);
   close(sfd);

   shutdown(sfd0, 2);
   close(sfd0);

   if (!childdeath) {
      rmon_sem_wait(semid_down);

      rmon_mlclr(&monitoring_box_down->level);
      monitoring_box_down->valid = 1;
      monitoring_box_down->quit = 1;
      rmon_sem_signal(semid_down);
   }

   if (shmdt((char *) monitoring_box_down) < 0)
      rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMDT_S, sys_errlist[errno]);

   rmon_sem_close(semid_down);

/*
 */
   rmon_sem_rm(semid_down);

   if (shmctl(shmid_down, IPC_RMID, NULL) < 0)
      rmon_errf(TERMINAL, MSG_RMON_ERRORINSHMCTL_S, sys_errlist[errno]);
/*
 */

   DEXIT;
   DCLOSE;
   exit(0);
}

/***************************************************************************/

void rmon_shut_down_on_error()
{
#undef  FUNC
#define FUNC "rmon_shut_down_on_error"
   DENTER;

   close(pfdtospy[READ]);

   shutdown(sfd, 2);
   close(sfd);

   shutdown(sfd0, 2);
   close(sfd0);

   if (!childdeath) {
      rmon_sem_wait(semid_down);

      rmon_mlclr(&monitoring_box_down->level);
      monitoring_box_down->valid = 1;
      monitoring_box_down->quit = 1;
      rmon_sem_signal(semid_down);
   }

   shmdt((char *) monitoring_box_down);
   shmctl(shmid_down, IPC_RMID, NULL);
   rmon_sem_close(semid_down);

   DEXIT;
   DCLOSE;
}

/***************************************************************************/

static int parser(
int argc,
char **argv 
) {
   int i = 1;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ======================================================== */
   /*                                                                      */
   /* usage:   spy [-r rmond-host] programname                             */
   /*                                                                      */
   /* ======================================================== */

   if (i >= argc)
      usage();

   if (argv[i][0] == '-') {
      if (argv[i][1] == 'r') {
         if (argv[i][2] != '\0') {
            if (!rmon_make_name(&argv[i][2], rmond)) {
               printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
               usage();
            }
         }
         else {
            if (++i >= argc)
               usage();
            if (!rmon_make_name(argv[i], rmond)) {
               printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
               usage();
            }
         }
         name_is_valid = 1;
         i++;
      }
      else {
         printf(MSG_RMON_UNKNOWNOPTION);
         usage();
      }
   }

   if (i >= argc)
      usage();

   /*
      if ( sscanf(argv[i], "%ld", &port) != 1 ) {
      printf("unable to read portnumber\n");
      usage();     
      }
      i++; */

   if (i >= argc)
      usage();

   DEXIT;
   return i;
}

static void usage()
{
   printf(MSG_RMON_SPY_USAGE);
   DEXIT;
   DCLOSE;
   exit(0);
}
