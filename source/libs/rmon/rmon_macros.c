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
#define DEBUG RMON_LOCAL

#include <stdarg.h>

/* for accept variables */
#if defined(_AIX)
#include <sys/select.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <sys/param.h>          /* MAXHOSTNAMELEN */
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef RMONLIGHT
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif

#ifdef SGE_MT
#include <pthread.h>
#include "sge_mtutil.h"
#endif

#include "basis_types.h"
#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_monitoring_level.h"
#include "msg_rmon.h"
#ifndef RMONLIGHT
#include "rmon_semaph.h"
#include "rmon_piped_message.h"
#endif

#ifdef SGE_MT
/* MT-NOTE: may not use sge_mutex_lock()/sge_mutex_unlock() for this mutex !! */
/* MT-NOTE: rmon_mutex guards access to global variable msgbuf */
pthread_mutex_t rmon_mutex = PTHREAD_MUTEX_INITIALIZER;
#define RMON_LOCK(func)      pthread_mutex_lock(&rmon_mutex)
#define RMON_UNLOCK(func)    pthread_mutex_unlock(&rmon_mutex)
#else
#define RMON_LOCK(func)      
#define RMON_UNLOCK(func)
#endif

#define PIPE_SIZE               4096L
#define PIPE_MESSAGES   (u_long)( PIPE_SIZE / sizeof(piped_message_type) )

/* ----------- global variables ------------ */

int LAYER = 0;
monitoring_level DEBUG_ON = { {0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L} };
u_long MLEVEL = 0L;
u_long MTYPE = RMON_NONE;  /*RMON_LOCAL;*/
u_long DEBUG_TRACEID = 0;

/* ------------ static variables ----------- */


#ifndef RMONLIGHT
static char *unique_program[] =
{
   "qmaster",
   NULL
};

static char *host_unique_program[] =
{
   "sge_execd",
   "knecht_light",
   NULL
};
#endif /* RMONLIGHT */

static char empty[] = "    ";
static u_long jobid;
static int wfd;

static char msgbuf[10 * STRINGSIZE];

#ifndef RMONLIGHT
static int shut_down = 0, quit = 0;
static int shmid_up, semid_up;
#endif

#ifndef RMONLIGHT
static monitoring_box_type *monitoring_box_up;
static long shmkey_up, semkey_up;
#endif

static FILE *rmon_fp = NULL;

/* ------------ static function prototypes ----------- */

static int mtype(void);
static void mwrite(char *message);

#ifndef RMONLIGHT
static void mkstdname(char *dest, int dest_len, char *programname);
static void sem_error(char *file, int line);
static void shm_error(char *file, int line);
#endif /* RMONLIGHT */


#ifdef SUN4
extern char *sys_errlist[];
char *strerror(
int errno 
) {
   return (errno>=0)?sys_errlist[errno]:MSG_RMON_INVALIDERRNO;
}
#endif



/****** rmon_macros/rmon_condition() *******************************************
*  NAME
*     rmon_condition() -- is monitoring enabled for layer/class?
*
*  SYNOPSIS
*     int rmon_condition(register int layer, register int class) 
*
*  INPUTS
*     register int layer - ??? 
*     register int class - ??? 
*
*  RESULT
*     int - 1 true 
*           0 false
*
*  NOTES
*     MT-NOTES: rmon_condition() is not MT safe due to read access to global
*     MT-NOTES: variable. Write access currently only in one thread
*******************************************************************************/
int rmon_condition(
register int layer,
register int class 
) {

/* instead of a call to rmon_mlgetl() */
#define INLINE_RMON_MLGETL(s, i) ((s)->ml[i])
  


   if ((MTYPE!=RMON_NONE) &&                                          /* in case of monitoring gererally */
      ((MLEVEL=class) &                              /* and in case the message class is selected by ... */

#ifdef RMONLIGHT
         (INLINE_RMON_MLGETL(&DEBUG_ON, layer))) )       /* RMONLIGHT means local monitoring     */
#else
         ((MTYPE==RMON_LOCAL)?
                                         INLINE_RMON_MLGETL(&DEBUG_ON, layer):       /* .. local monitoring     */
               (rmon_d_monlev(&DEBUG_ON),INLINE_RMON_MLGETL(&DEBUG_ON, layer))) ) )  /* .. or remote monitoring */
#endif /* RMONLIGHT */
   {
      return 1;
   }
   else
   {
      return 0;
   }
}




void rmon_mpush_layer(
int new_layer 
) {
   if ((LAYER + 1) != new_layer) {
      printf(MSG_RMON_DEBUGLAZERSAREINCONSISTENT);
      exit(-1);
   }
   if (new_layer >= rmon_mlnlayer()) {
      printf(MSG_RMON_TRIEDTOSETTOLARGELAYER);
      exit(-1);
   }
   LAYER++;

   return;
}

int rmon_mpop_layer()
{
   if (LAYER == 0) {
      printf(MSG_RMON_TRIEDTOPOPHIGHESTLAYER );
      exit(-1);
   }
   return LAYER--;
}

int rmon_mfork()
{
   int pid;

   if ((pid = fork()) != 0)
      return pid;

#ifndef RMONLIGHT
   if (MTYPE == RMON_REMOTE) {
      semid_up = rmon_sem_open(semkey_up);
      if (semid_up == -1)
         sem_error(__FILE__, __LINE__);
   }
#endif

   return pid;
}

int rmon_mexeclp(char *file,...)
{
   va_list ap;
   va_list countap;

   char **newargv;

#ifndef RMONLIGHT
   char read_channel[8];        /* pipe-read-channel converted to string    */
   char write_channel[8];       /* pipe-write-channel converted to string   */
   char semaphore[8];
#endif   

   int i = 0, n = 0;
   int retval;

#undef FUNC
#define FUNC "rmon_mexeclp"

   DENTER;

   va_start(ap, file);
   /* how many args are given ? */
   for (countap = ap; va_arg(countap, char *); n++);

   /* build extended argv */
   newargv = (char **) malloc(sizeof(char *) * (n + 3 + 1));
   if (!newargv) {
      DPRINTF(("cannot malloc()\n"));
      DEXIT;
      return -1;
   }

   /* copy given args into newargv */
   for (i = 0; (newargv[i] = va_arg(ap, char *)) != NULL; i++);
   va_end(ap);

#ifndef RMONLIGHT
   if (MTYPE == RMON_REMOTE) {
      /* copy extra args into newargv */
      sprintf(semaphore, "%ld", semkey_up);     /* convert int to string */
      sprintf(read_channel, "%ld", shmkey_up);  /* convert int to string */
      sprintf(write_channel, "%d", wfd);        /* convert int to string */

      newargv[i++] = semaphore;
      newargv[i++] = read_channel;
      newargv[i] = NULL;

      n += 3;
   }
#endif

   /* for ( i = 0; i<n; i++ )
      DPRINTF(("%d. Argument: <%s>\n", i, newargv[i] )); */

   retval = execvp(file, newargv);
   free(newargv);

   DEXIT;

   return retval;
}

int rmon_mexecvp(
char *file,
char *argv[] 
) {

   char **newargv;

#ifndef RMONLIGHT
   char read_channel[8];        /* pipe-read-channel converted to string    */
   char write_channel[8];       /* pipe-write-channel converted to string   */
   char semaphore[8];
#endif

   int retval;
   int i, n = 0;

#undef FUNC
#define FUNC "rmon_mexecvp"
   DENTER;

   /* how many args are given ? */
   for (i = 0; argv[i]; i++)
      n++;

   /* build extended argv */
   newargv = (char **) malloc(sizeof(char *) * (n + 3 + 1));
   if (!newargv) {
      DPRINTF(("cannot malloc()\n"));
      DEXIT;
      return -1;
   }

   /* copy given args into newargv */
   for (i = 0; (newargv[i] = argv[i]) != NULL; i++);

#ifndef RMONLIGHT
   if (MTYPE == RMON_REMOTE) {
      /* copy extra args into newargv */
      sprintf(semaphore, "%ld", semkey_up);     /* convert int to string */
      sprintf(read_channel, "%ld", shmkey_up);  /* convert int to string */
      sprintf(write_channel, "%d", wfd);        /* convert int to string */

      newargv[i++] = semaphore;
      newargv[i++] = read_channel;
      newargv[i++] = write_channel;
      newargv[i] = NULL;

      n += 3;
   }
#endif

   /* for ( i = 0; i<n; i++ )
      DPRINTF(("%d. Argument: <%s>\n", i, newargv[i] )); */

   retval = execvp(file, newargv);
   free(newargv);

   DEXIT;
   return retval;
}

/* may fd be closed ? */
int rmon_mmayclose(
int fd 
) {
   return (MTYPE == RMON_REMOTE ? fd != wfd : 1);
}

static int mtype()
{
   char *s;

   s = getenv("SGE_DEBUG_LEVEL");

   if (!s)
      return RMON_NONE;
   else
#ifndef RMONLIGHT
      return( RMON_LOCAL );
#else
      return (strcmp(s, "RMON_REMOTE") == 0) ? RMON_REMOTE : RMON_LOCAL;
#endif
}

int rmon_d_writenbytes(
register int sfd,
register char *ptr,
register int n 
) {
   int i, nleft;

   nleft = n;

   while (nleft > 0) {
      i = write(sfd, ptr, nleft);

      if (i <= 0)
         return (i);

      nleft -= i;
      ptr += i;
   }
   return 0;
}


#ifndef RMONLIGHT

static void mkstdname(
char *completename,
int max,
char *programname 
) {
   char hostname[MAXHOSTNAMELEN + 1];
   string s;
   int i = 0;
   int is_unique = 0;

   /* is this a unique program ? */
   while (unique_program[i] && !is_unique)
      is_unique = !strcmp(programname, unique_program[i++]);

   strcpy(s, programname);

   if (!is_unique) {
      /* so name needs hostname and pid for being unique */

      /* get hostname */
      if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
         printf(MSG_RMON_MKSTDNAMEUNABLETOGETMYHOSTNAME_S , strerror(errno));
         exit(-1);
      }
      sprintf(&s[strlen(s)], "_%s", hostname);

      /* is this a unique program on a host ? */
      while (host_unique_program[i] && !is_unique)
         is_unique = !strcmp(programname, host_unique_program[i++]);

      if (!is_unique) {
         /* build name from programname, hostname and PID */
         sprintf(&s[strlen(s)], "_%d", (int)getpid());
      }
   }

   /* control length of complete name */
   if (strlen(s) > max) {
      printf(MSG_RMON_MKSTDNAMESTRINGTOOLONG );
      exit(-1);
   }
   strcpy(completename, s);

   /* control output */
   printf("name: %s\n", completename);

   return;
}
#endif /* RMONLIGHT */


void rmon_mopen(
int *argc,
char *argv[],
char *programname 
) {
   int i;

#ifndef RMONLIGHT
   string completename;
#endif

   char *sx, sbuf[128], *s = sbuf;
   u_long layer;

   /* do make the ansi compiler happy */
   rmon_fp = stderr;

   /* am I started by a spy ? */
   MTYPE = mtype();
   LAYER = 0;
   switch (MTYPE) {

   case RMON_REMOTE:
#ifndef RMONLIGHT

/*      printf("RMON_REMOTE\n"); */

      sscanf(argv[*argc - 3], "%ld", &semkey_up);
      sscanf(argv[*argc - 2], "%ld", &shmkey_up);
      sscanf(argv[*argc - 1], "%d", &wfd);

      *argc -= 3;
      argv[*argc] = NULL;

      shmid_up = shmget(shmkey_up, sizeof(monitoring_box_type), PERMS | IPC_CREAT);
      if (shmid_up == -1)
         shm_error(__FILE__, __LINE__);

      monitoring_box_up = (monitoring_box_type *) shmat(shmid_up, NULL, 0);
      if (monitoring_box_up == NULL)
         shm_error(__FILE__, __LINE__);

      semid_up = rmon_sem_open(semkey_up);
      if (semid_up == -1)
         sem_error(__FILE__, __LINE__);

      /* send programname to spy */
      if (!rmon_sem_wait(semid_up))
         sem_error(__FILE__, __LINE__);

      mkstdname(completename, STRINGSIZE - 1, programname);
      strncpy(monitoring_box_up->programname, completename, STRINGSIZE - 1);
      monitoring_box_up->programname_is_valid = 1;

      if (!rmon_sem_signal(semid_up))
         sem_error(__FILE__, __LINE__);

#endif /* RmonLight */
      break;

   case RMON_LOCAL:

/*      printf("RMON_LOCAL\n"); */

      rmon_mlclr(&DEBUG_ON);

      if ((sx = getenv("SGE_DEBUG_LEVEL"))) {
         strcpy(s, sx);
         for (i = 0; i < N_LAYER; i++) {
            if (!(s = strtok(i == 0 ? s : NULL, " "))) {
               printf(MSG_RMON_ILLEGALDBUGLEVELFORMAT);
               exit(-1);
            }
            if (sscanf(s, "%ld", &layer) == EOF) {
               printf(MSG_RMON_ILLEGALDBUGLEVELFORMAT);
               exit(-1);
            }
            rmon_mlputl(&DEBUG_ON, i, layer);
/*             printf("layer: %2.2d mask: %4.4d\n", i, layer); */
         }
      }

        if ((s = getenv("SGE_DEBUG_TARGET"))) {
                if (strcmp(s, "stdout")==0)
                        rmon_fp = stdout;
                else if (strcmp(s, "stderr")==0)
                        rmon_fp = stderr;
                else {
                        if ( (rmon_fp = fopen(s, "w"))==0) {
                                rmon_fp = stderr;
                                fprintf(rmon_fp, MSG_RMON_UNABLETOOPENXFORWRITING_S, s);
                                fprintf(rmon_fp,  MSG_RMON_ERRNOXY_DS, errno, strerror(errno));
                        }
                }
   }
   break;

   case RMON_NONE:
/*      printf("RMON_NONE\n"); */

      rmon_mlclr(&DEBUG_ON);
      break;
   }

   return;
}

void rmon_mclose()
{
#ifndef RMONLIGHT
   piped_message_type piped_message;

   if (MTYPE == RMON_REMOTE && !shut_down) {
      rmon_mlclr(&piped_message.level);
      rmon_mlputl(&piped_message.level, LAYER, MLEVEL);
      piped_message.pid = (u_long) getpid();
      piped_message.jobid = jobid;
      piped_message.childdeath = 1;

      for (;;) {
         if (!rmon_sem_wait(semid_up))
            sem_error(__FILE__, __LINE__);

         if (monitoring_box_up->count < PIPE_MESSAGES) {
            if (!rmon_d_writenbytes(wfd, (char *) &piped_message, sizeof(piped_message_type))) {
               monitoring_box_up->count++;
               if (!rmon_sem_signal(semid_up))
                  sem_error(__FILE__, __LINE__);
               break;
            }
         }

         if (!rmon_sem_signal(semid_up))
            sem_error(__FILE__, __LINE__);

         sleep(1);
      }

      quit = monitoring_box_up->quit;
      if (!rmon_sem_close(semid_up))
         sem_error(__FILE__, __LINE__);
      shmdt((char *) monitoring_box_up);

      if (quit) {
         if (!rmon_sem_rm(semid_up))
            sem_error(__FILE__, __LINE__);

         if (shmctl(shmid_up, IPC_RMID, NULL) < 0)
            shm_error(__FILE__, __LINE__);
      }
   }
#endif
}

void rmon_menter(
const char *func 
) {
   RMON_LOCK("rmon_menter");

   sprintf(msgbuf, "--> %s() {\n", func);
   mwrite(msgbuf);

   RMON_UNLOCK("rmon_menter");
   return;
}

void rmon_mexit(
const char *func,
const char *file,
int line 
) {
   RMON_LOCK("rmon_mexit");

   sprintf(msgbuf, "<-- %s() %s %d }\n", func, file, line);
   mwrite(msgbuf);

   RMON_UNLOCK("rmon_mexit");
   return;
}

void rmon_mexite(
const char *func,
const char *file,
int line 
) {
   RMON_LOCK("rmon_mexite");
   sprintf(msgbuf, "<-- %s() ERROR %s %d\n", func, file, line);
   mwrite(msgbuf);
   RMON_UNLOCK("rmon_mexite");
}

void rmon_mtrace(
const char *func,
const char *file,
int line 
) {
   RMON_LOCK("rmon_mtrace");
   strcpy(msgbuf, empty);
   sprintf(&msgbuf[4], "%s:%s:%d\n", func, file, line);
   mwrite(msgbuf);
   RMON_UNLOCK("rmon_mtrace");

   return;
}

/* use like rmon_mjobtrace(u_long jobid, char *fmt, ... ) */
void rmon_mjobtrace(u_long job_id, const char *fmt,...)
{
   va_list args;
   RMON_LOCK("rmon_mjobtrace");

   jobid = job_id;

   va_start(args, fmt);

   strcpy(msgbuf, empty);

   vsprintf(&msgbuf[4], fmt, args);
   mwrite(msgbuf);

   va_end(args);
   RMON_UNLOCK("rmon_mjobtrace");
}

/* use like printf(char *fmt, ... ) */
void rmon_mprintf(const char *fmt,...)
{
   va_list args;
   RMON_LOCK("rmon_mprintf");

   va_start(args, fmt);

   strcpy(msgbuf, empty);

   /* could use
      int vsnprintf(
         char *s,
         size_t n,
         const char *format,
         va_list ap
      );
      on some systems (linux, ..? ) to prevent overflow of msgbuf */
#if defined(AIX42) || defined(ALPHA4) || defined(HP10) || defined(IRIX6)
   vsprintf(&msgbuf[4], fmt, args); 
#else
   vsnprintf(&msgbuf[4], (10 * STRINGSIZE) - 10 ,  fmt, args);
#endif
   mwrite(msgbuf);

   va_end(args);
   RMON_UNLOCK("rmon_mprintf");
}

static void mwrite(
char *message 
) {
#ifndef RMONLIGHT
   piped_message_type piped_message;
#endif
   switch (MTYPE) {
   case RMON_REMOTE:
#ifndef RMONLIGHT
      strncpy(piped_message.data, message, STRINGSIZE * 3 - 1);
      piped_message.data[STRINGSIZE * 3 - 1] = '\0';
      rmon_mlclr(&piped_message.level);
      rmon_mlputl(&piped_message.level, LAYER, MLEVEL);
      piped_message.pid = (u_long) getpid();
      piped_message.jobid = jobid;
      piped_message.childdeath = 0;

      for (;;) {
         if (!rmon_sem_wait(semid_up))
            sem_error(__FILE__, __LINE__);
         if (monitoring_box_up->count < PIPE_MESSAGES) {
            if (!rmon_d_writenbytes(wfd, (char *) &piped_message, sizeof(piped_message_type))) {
               monitoring_box_up->count++;
               if (!rmon_sem_signal(semid_up))
                  sem_error(__FILE__, __LINE__);
               break;
            }
         }

         if (!rmon_sem_signal(semid_up))
            sem_error(__FILE__, __LINE__);
         sleep(1);
      }

#endif
      break;

   case RMON_LOCAL:
/*       for (i = 0; i < LAYER; i++) */
/*          printf("    "); */
/*       printf("layer: %d ", LAYER); */

#if defined(SGE_MT)
      fprintf(rmon_fp, "%6ld %6d %ld ", DEBUG_TRACEID++, (int)getpid(), (long int)pthread_self());
#else
      fprintf(rmon_fp, "%6ld %6d", DEBUG_TRACEID++, (int)getpid());
#endif
      fprintf(rmon_fp, "%s", message);
      fflush(rmon_fp);
      break;
   }

   return;
}



#ifndef RMONLIGHT

void rmon_d_monlev(
monitoring_level *ml 
) {
   static monitoring_level vorher;
   static int vorher_initialized = 0;
   int valid;
   printf("--> rmon_d_monlev()\n");

   /* --------------------------------------------------------- */
   /* wait for a valid  monitoring_level from the shared memory */
   /* (even if it is the old monitoring_level)                  */
   /* --------------------------------------------------------- */

   if (!vorher_initialized) {
      vorher_initialized = 1;
      rmon_mlclr(&vorher);
      rmon_mlset(&vorher, NO_LEVEL);
   }
   rmon_mlclr(ml);
   if (!quit) {
      do {
         if (!rmon_sem_wait(semid_up))
            sem_error(__FILE__, __LINE__);
         rmon_mlcpy(ml, &(monitoring_box_up->level));
         valid = monitoring_box_up->valid;
         quit = monitoring_box_up->quit;
         if (!rmon_sem_signal(semid_up)) {
            sem_error(__FILE__, __LINE__);
         }
         if (!valid) {
            sleep(1);
         }
      } while (!valid);

      /* Ausgabe nur wenn sich etwas aendert */

      if (rmon_mlcmp(ml, &vorher) != 0) {
         printf("monitoring_level: %ld\n", rmon_mlgetl(ml, 0));
         rmon_mlcpy(&vorher, ml);
      }
   }
   if (quit && !shut_down) {
      shut_down = 1;
      shmdt((char *) monitoring_box_up);
      if (!rmon_sem_rm(semid_up))
         sem_error(__FILE__, __LINE__);

      if (shmctl(shmid_up, IPC_RMID, NULL) < 0)
         shm_error(__FILE__, __LINE__);

      close(wfd);
   }

   printf("<-- rmon_d_monlev()\n");
   return;
}

static void shm_error(
char *file,
int line 
) {
   FILE *fp;

   fp = fopen("semshm-error", "w+t");
   fprintf(fp, MSG_RMON_XERRORINASHAREDMEMOPERATION_I, (int)getpid());
   fprintf(fp, MSG_RMON_FILEXLINEY_SI, file, line);
   fprintf(fp, MSG_RMON_ERRNOXY_DS, errno, strerror(errno));
   fclose(fp);
   exit(-1);
}

static void sem_error(
char *file,
int line 
) {
   FILE *fp;

   fp = fopen("semshm-error", "w+t");
   fprintf(fp, MSG_RMON_XERRORINASEMAPHOREOPERATION_I, (int)getpid());
   fprintf(fp, MSG_RMON_FILEXLINEY_SI, file, line);
   fprintf(fp, MSG_RMON_ERRNOXY_DS,  errno, strerror(errno));
   fclose(fp);
   exit(-1);
}
#endif /* RMONLIGHT */
