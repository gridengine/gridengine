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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>

#if defined(DARWIN)
#include <sys/time.h>
#endif

#include <sys/resource.h>

#if defined(AIX32) || defined(AIX41)
#   include <sys/select.h>
#endif

#include "commlib.h"
#include "commd_error.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_prog.h"
#include "commd.h"
#include "message.h"
#include "rwfd.h"
#include "debug_malloc.h"
#include "sge_time.h"
#include "sge.h"
#include "msg_common.h"
#include "msg_commd.h"
#include "sge_language.h"
#include "sge_feature.h"
#include "setup_commd_path.h"
#include "sge_unistd.h"
#include "sge_os.h"

void init_send(message *mp, int reserved_port, int commdport);
static char *fdsetstr(fd_set *fds, char *);
static void schedule_resends(int port_security, int port, u_long now);
void dump(void);
void trace(const char *);

/** local funcs **/
static void commd_usage(FILE *out, char **argv);
void lack_of_memory(void);
int build_read_fd_set(fd_set *readfds, message *mp, commproc *commp);
int build_write_fd_set(fd_set *writefds, message *mp);
void sighandler(int sig);
void log_state_transition_too_many_fds_open(u_long t);
int seek_badfd(int nfds, fd_set *rfds, fd_set *wfds);
int main(int argc, char **argv);

static int memorylack = 0;
extern message *message_list;
extern commproc *commprocs;
static int sockfd = 0;          /* socket we wait on for incoming connections */
message *termination_message = NULL;
int message_tracefd = -1;
int messagelog_fd = -1;
int hostname_refresh = 1;       /* resolve hostnames on a regular basis */
char *aliasfile = NULL;         /* File used for specifying host aliases */
char *actmasterfile = NULL;     /* File with actual qmaster name */
char *product_mode_file = NULL; /* File with actual product mode */
u_long too_many_fds_open = 0;   /* time at which too many fds were open */

char logfile[256] = "/tmp/commd.errors";

/* These global variables are used for profiling */
u_long32 last_logginglevel = 0;
int enable_commd_profile = 0;
unsigned long new_messages = 0;
unsigned long del_messages = 0;
unsigned long resend_messages = 0;
unsigned long sockets_created = 0;
unsigned long connection_errors = 0;
unsigned long connection_closed = 0;
unsigned long connection_accept = 0;
unsigned long bytes_sent = 0;
unsigned long unique_hosts_messages = 0;
unsigned long data_message_byte_count = 0;
unsigned long data_message_count = 0;
double max_med_message_size = 0.0;
double max_kbit_per_second = 0.0;
double max_mc_per_second = 0.0;
double max_new_m_per_second  = 0.0;
double max_del_m_per_second = 0.0;
double max_resend_m_per_second = 0.0;
double max_new_sockets_per_second = 0.0;
double max_con_errors_per_second = 0.0;
double max_con_closed_per_second = 0.0;
double max_con_accepted_per_second = 0.0;
double max_unique_hosts_messages_per_second = 0.0;


/** profiling funcs **/
static void reset_measurement_data(void); 





/*---------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int i, on = 1;
   struct sockaddr_in serv_addr;
   char *service, *message_logging = NULL;
   int port = 0, daemon = 1;
   struct timeval timeout;
   fd_set readfds, writefds, dontclose;
   int port_security = 0;
   message *mp;
   char tmpstr1[1024], tmpstr2[1024], *cp;
   time_t now;
   time_t last_loop_time = 0;         /* profiling */
   SGE_STRUCT_STAT stat_dummy;
   struct servent *se = NULL;
   int nisretry;
   unsigned long lasthostrefresh = 0;
   struct sigaction sa;
   int cnt, nfd;
   
   char **argp = argv;

   DENTER_MAIN(TOP_LAYER, "commd");
   
   /* This needs a better solution */
   umask(022);
  
#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   /* increase filedescriptor limit to max. */
   {
      struct rlimit limit;
 
      if (!getrlimit(RLIMIT_NOFILE, &limit)) {
         if (limit.rlim_cur < limit.rlim_max) {
            limit.rlim_cur = limit.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limit);
         }
      }
   }          

   /* temporary logfile until we are daemonized */
   log_state_set_log_file(logfile);
   log_state_set_log_trace_func(trace);
   
   /* determine service from our program name */
   
   service = SGE_COMMD_SERVICE;

   if ((cp = getenv("COMMD_PORT")))
      port = atoi(cp);

   /* check if reserved port shall be used; set in product_mode file */
   /* gdi lib call */
   sge_commd_setup(COMMD);
   if (use_reserved_port())
      port_security = 1; 
   DPRINTF(("reserved ports %s\n", port_security ? "on" : "off"));


   /* scan arguments */
   while (*(++argp)) {          /* skip first */
      if (!strcmp("-h", *argp) || !strcmp("-help", *argp)) {
         commd_usage(stdout, argv);
      }
      if (!strcmp("-s", *argp)) {
         argp++;
         if (*argp)
            service = *argp;
         else
            commd_usage(stderr, argv);
         continue;
      }
      if (!strcmp("-p", *argp)) {
         argp++;
         if (*argp)
            port = atoi(*argp);
         else
            commd_usage(stderr, argv);
         continue;
      }
      if (!strcmp("-ml", *argp)) {
         argp++;
         if (*argp)
            message_logging = *argp;
         else
            commd_usage(stderr, argv);
         continue;
      }
      if (!strcmp("-ll", *argp)) {
         argp++;
         if (*argp) {
            log_state_set_log_level(atoi(*argp));
            if (log_state_get_log_level() < 2 || log_state_get_log_level() > 7)
               commd_usage(stderr, argv);
         }      
         else
            commd_usage(stderr, argv);
         continue;
      }
      if (!strcmp("-nd", *argp)) {
         daemon = 0;
         continue;
      }
      if (!strcmp("-a", *argp)) {
         argp++;
         if (*argp) {
            aliasfile = *argp;
            if (SGE_STAT(aliasfile, &stat_dummy)) {
                CRITICAL((SGE_EVENT, MSG_COMMD_STATHOSTALIASFILEFAILED_SS, 
                         aliasfile, strerror(errno)));
                SGE_EXIT(1);
             }
         } else
            commd_usage(stderr, argv);
         continue;
      }
      if (!strcmp("-dhr", *argp)) {
         hostname_refresh = 0;
         continue;
      }

      /* got an invalid option */
      commd_usage(stderr, argv);
   }

   /* setup signalhandling */
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = sighandler;  /* one handler for all signals */
   sigemptyset(&sa.sa_mask);
   sigaction(SIGINT, &sa, NULL);
   sigaction(SIGTERM, &sa, NULL);
   sigaction(SIGHUP, &sa, NULL);
   sigaction(SIGPIPE, &sa, NULL);

   /* initialize host module */
   sge_host_list_initialize();

   if (aliasfile == NULL) {
      /* expect alias file at default location */
      aliasfile = sge_get_alias_path();

   }   
   if (actmasterfile == NULL) {
      /* expect act_master file at default location */
      actmasterfile = get_act_master_path(uti_state_get_default_cell());
   }
   if (product_mode_file == NULL) {
      /* expect product_mode_file file at default location */
      product_mode_file = get_product_mode_file_path(uti_state_get_default_cell());
   }
 

   /* read aliasfile */
   if (aliasfile) {
      if (SGE_STAT(aliasfile, &stat_dummy))
         INFO((SGE_EVENT, MSG_COMMD_STATHOSTALIASFILEFAILED_SS, 
               aliasfile, strerror(errno)));
      else {
         sge_host_list_read_aliasfile(aliasfile);
      }
   }

   if (message_logging)
      if (enable_message_logging(message_logging) == -1) {
         CRITICAL((SGE_EVENT,MSG_COMMD_OPENFILEFORMESSAGELOGFAILED_SS , 
                  message_logging,
                  strerror(errno)));
         SGE_EXIT(1);
      }

   if (port)
      printf(MSG_NET_USINGPORT_I , port);
   else
      printf(MSG_NET_USINGSERVICE_S , service);

   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      CRITICAL((SGE_EVENT, MSG_NET_OPENSTREAMSOCKETFAILED_S , 
         strerror(errno)));
      SGE_EXIT(1);
   }   

   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

   /* bind an address to socket */
   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   if (port)
      serv_addr.sin_port = htons(port);
   else {
      nisretry = MAXNISRETRY;
      while (nisretry-- && !((se = getservbyname(service, "tcp"))));
      if (!se) {
         CRITICAL((SGE_EVENT, MSG_NET_RESOLVESERVICEFAILED_SS , 
                  service,
                  strerror(errno)));
         SGE_EXIT(1);
      }
      port = ntohs(se->s_port);
      serv_addr.sin_port = se->s_port;
   }
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      CRITICAL((SGE_EVENT, MSG_NET_BINDPORTFAILED_IS , 
               (int) ntohs(serv_addr.sin_port),
               strerror(errno)));
      SGE_EXIT(1);
   }

   printf(MSG_NET_BOUNDTOPORT_I , (int) ntohs(serv_addr.sin_port));

   if (daemon) {
      FD_ZERO(&dontclose);
      if (messagelog_fd != -1)
         FD_SET(messagelog_fd, &dontclose);
      FD_SET(sockfd, &dontclose);
      sge_daemonize(&dontclose);
   }

   /* path for log file */
   sprintf(logfile, "/tmp/commd/err.%d", (int) getpid());
   log_state_set_log_file(logfile);
   
   /* log the port number */
   if (port)
      WARNING((SGE_EVENT, MSG_NET_USINGPORT_I , port));
   else 
      WARNING((SGE_EVENT, MSG_NET_USINGSERVICE_S , service));

   /* make socket listening for incoming connects */
   listen(sockfd, 5);

   /*
    * main loop waiting for external events 
    */
   while (1) {
      int maxfd, maxrfd, maxwfd;
                
      now = sge_get_gmt();
      if (__CONDITION(INFOPRINT)) {
         char date[256], tmp_date[256];
         char *tmp_ctime;
         tmp_ctime = ctime(&now);
         strcpy(tmp_date, tmp_ctime);
         sscanf(tmp_date, "%[^\n]", date);
         DPRINTF(("================[EPOCH %s]================\n", date));
      } else {
         DPRINTF(("================[EPOCH]================\n"));
      }

      /* look for messages which couldn't be sent to remote commds -> try it 
         again */

      schedule_resends(port_security, port, now);

      /* look for changes in host names and aliases */
      if (hostname_refresh) {
         if (now - lasthostrefresh > HOSTREFRESHTIME) {
            sge_host_list_refresh();
            lasthostrefresh = now;
         }
      }

      look4timeouts(now);       /* look 4 dead commprocs;
                                   we use a timeout to consider them dead */
      look4timeouts_messages(now);      /* same for messages */

      /* build selectable fds */
      maxrfd = build_read_fd_set(&readfds, message_list, commprocs);
      maxwfd = build_write_fd_set(&writefds, message_list);

      maxfd = MAX(sockfd, MAX(maxrfd, maxwfd));

      if (termination_message) {
         if (!exist_message(termination_message)) {
            /* message seems to be acknowledged -> die */
            WARNING((SGE_EVENT, MSG_COMMD_SHUTDOWNDUECONTROLMESSAGE ));
            SGE_EXIT(0);
         }

      }

      if (memorylack) {
         DPRINTF(("memorylack=TRUE"));
         FD_CLR(sockfd, &readfds);      /* we cant accept a new connection without 
                                           memory */
         timeout.tv_sec = 1;    /* look every second for free memory */
         timeout.tv_usec = 0;
#if 0
         DEBUG((SGE_EVENT, "--- select(%d, (%s), (%s), NULL, %d:%d)",
              FD_SETSIZE, 
              fdsetstr(&readfds, tmpstr1),
              fdsetstr(&writefds, tmpstr2),
              (int) timeout.tv_sec, 
              (int) timeout.tv_usec));
#endif
         cnt = 0;

#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
         while ((i = select(FD_SETSIZE, (int *) &readfds, (int *) &writefds,
                            NULL, &timeout)) == -1 && errno == EINTR)
            cnt++;
#else
         while ((i = select(FD_SETSIZE, &readfds, &writefds, NULL, &timeout))
                == -1 && errno == EINTR)
            cnt++;
#endif
         if (cnt || i) {
            DPRINTF(("select returned with %d - count %d\n", i, cnt));
         }
         nfd = i;
      }
      else {
         timeout.tv_sec = 10;   /* look every 10 seconds for messages to be 
                                   rescheduled */
         timeout.tv_usec = 0;

         /* If we have to many open fds accept() on sockfd will fail.
            To avoid busy looping do not accept new connections after a 
            failed accept for 5 seconds. */
         if (too_many_fds_open) {
            if (now - too_many_fds_open > 5) {
               too_many_fds_open = 0;
               log_state_transition_too_many_fds_open(too_many_fds_open);
               FD_SET(sockfd, &readfds);
            }
         }
         else
            FD_SET(sockfd, &readfds);
#if 0
         DEBUG((SGE_EVENT, "--- select(%d, (%s), (%s), NULL, %d:%d)", 
               FD_SETSIZE,
               fdsetstr(&readfds, tmpstr1), 
               fdsetstr(&writefds, tmpstr2),
               (int) timeout.tv_sec, 
               (int) timeout.tv_usec));
#endif
         cnt = 0;
#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
         while ((i = select(FD_SETSIZE, (int *) &readfds, (int *) &writefds,
                            NULL, &timeout)) == -1 && errno == EINTR)
            cnt++;
#else
         while ((i = select(FD_SETSIZE, &readfds, &writefds, NULL, &timeout))
                == -1 && errno == EINTR)
            cnt++;
#endif
         if (cnt || i) {
            DPRINTF(("select returned with %d - count %d\n", i, cnt));
         }
         nfd = i;
      }
#if 0
      if (i)
         DEBUG((SGE_EVENT, "select returns %d (%s) (%s)", 
               i,
               fdsetstr(&readfds, tmpstr1), 
               fdsetstr(&writefds, tmpstr2)));
#endif
      if (i == -1) {
         if (errno == EBADF) {  /* need to find and close the bad fd */
            int bad_fd;

            /* bad read fd */
            if ((bad_fd = seek_badfd(FD_SETSIZE, &readfds, NULL)) != -1) {
               commproc *commp;
               message *mp;

               if (bad_fd == sockfd) {
                  /* our sockfd is invalid - may never happen */
                  CRITICAL((SGE_EVENT, MSG_NET_SELECTSOCKFDFAILED_ABORT_SI,
                          strerror(EBADF), 
                          sockfd));
                  abort();
               } else if ((mp = search_message(bad_fd, 0))) {
                  /* have a message with bad read fd - log and remove it */
                  ERROR((SGE_EVENT, MSG_NET_SELECTREADFAILEDMESSAGEFOLLOWS_SI ,
                         strerror(EBADF), 
                         bad_fd));

                  if (!uti_state_get_daemonized())
                     print_message(mp, stderr);

                  delete_message(mp, "bad read fd");
               } else if ((commp = search_commproc_waiting_on_fd(bad_fd))) {
                  /* have a commproc with bad read fd - log and remove it */
                  ERROR((SGE_EVENT, MSG_NET_SELECTREADFAILEDCOMMPROCFOLLOWS_SI ,
                         strerror(EBADF), 
                         bad_fd));

                  if (!uti_state_get_daemonized()) {
                     print_commproc(commp, stderr);       
                  }

                  delete_commproc(commp);
               } else if ((commp = search_commproc_using_fd(bad_fd))) {
                  ERROR((SGE_EVENT, MSG_NET_SELECTREADFAILEDNOTCOMPLETE_SI, 
                     strerror(EBADF), bad_fd));

                  if (!uti_state_get_daemonized()) {
                     print_commproc(commp, stderr);
                  }

                  delete_commproc(commp);
               } else {
                  /* dont know from where bad fd came - may also never happen */
                  ERROR((SGE_EVENT, MSG_NET_SELECTREADFAILEDNOCORSPDOBJECT_SI ,
                        strerror(EBADF), bad_fd));
               }
            }

            /* bad write fd */
            if ((bad_fd = seek_badfd(FD_SETSIZE, NULL, &writefds)) != -1) {

               if ((mp = search_message(bad_fd, 0))) {
                  /* have a message with bad write fd - log and remove it */
                  ERROR((SGE_EVENT, MSG_NET_SELECTWRITEFAILEDMESSAGEFOLLOWS_SI ,
                         strerror(EBADF), 
                         bad_fd));
                  
                  if (!uti_state_get_daemonized())       
                     print_message(mp, stderr);

                  delete_message(mp, "bad write fd");
               }
               else {
                  /* dont know from where bad fd came */
                   ERROR((SGE_EVENT, MSG_NET_SELECTWRITEFAILEDNOCORSPDOBJECT_SI ,
                         strerror(EBADF), 
                         bad_fd));
               }
            }
         }
         else {
             ERROR((SGE_EVENT, MSG_NET_SELECTERROR_SSS , 
                   strerror(errno),
                   fdsetstr(&readfds, tmpstr1), 
                   fdsetstr(&writefds, tmpstr2)));
         }
      }
      else {
         /* readfdset and writefdset indicate which fds can be handled now */



         /*    This can be used for showing open file descriptors
               and number of messages for that file descriptor 
               (profiling)                                         
         DEBUG((SGE_EVENT, "Profile4 timestamp=%ld, fds(%s)\n",
            now,
            count_different_file_descriptors()
         )); 
         */


         DPRINTF(("fd 0 - %d, maxfd = %d\n", FD_SETSIZE - 1, maxfd));
         /* be sure to keep track of nfd, otherwise you'll loop indefinitely */
         while(nfd) {
            for (i = 0; nfd && i <= maxfd; i++) {
               if (FD_ISSET(i, &readfds)) {
                  DEBUG((SGE_EVENT, "can read fd=%d", i));

                  /* we can read from fd with number i */
                  if (i == sockfd && nfd == 1) { /* proceed sockfd activeness after other fd's */
                     if((mp = mknewconnect(i))) {
                        /* we got a new connection */
                        /* now try to read from the new conn fd without*/
                        /* doing a select() */
                        FD_SET(mp->fromfd, &readfds);
                        if(mp->fromfd > maxfd)
                           maxfd = mp->fromfd;
                        nfd++;
                     }
                     FD_CLR(i, &readfds);
                     nfd--;
                  }
                  else if(i != sockfd) {
                     if(FD_ISSET(i, &writefds)) {
                        /* this happens when a commproc gets killed when it
                           was waiting to receive s.th.: the fd is ready to
                           receive AND ready to send an EOF. only choice:
                           KILL IT. */
                        /* if a remote commd gets killed, we don't find
                           a corresponding commproc => the message remains
                           active and we try to resend it again and 
                           again and again and...
                           so if we still find an active message for
                           this fd, we have to kill it manually */
                        WARNING((SGE_EVENT, MSG_NET_SELECTIGNCOMMPROCRCVANDEOF_I, i));
#if 0
                        dump();

                        if(delete_commproc_using_fd(i) == 0)
                           /* this is the dead commd case */
                           while((mp = search_message(i, 0)))
                              delete_message(mp, "receive and EOF");
#endif
                        FD_CLR(i, &writefds);
                        nfd--;
                     }
                     else {
                        mp = search_message(i, 1);
                        readfromfd(i, mp, port_security, port);
                        /* if the message is in a state where we can */
                        /* directly go on writing to the fd, then we */
                        /* try to do so. */
                        if((mp = search_message(i, 0))) {
                           if((MESSAGE_STATUS(mp) == S_WRITE_ACKE ||
                               MESSAGE_STATUS(mp) == S_ACK_THEN_PROLOG
                              ) ||
                              ((MESSAGE_STATUS(mp) == S_WRITE_ACK ||
                                MESSAGE_STATUS(mp) == S_WRITE_ACK_SND
                               ) && mp->fromfd == i
                              )
                             ) {
                              FD_SET(i, &writefds);
                              nfd++;
                           }
                        }
                        FD_CLR(i, &readfds);
                        nfd--;
                     }
                  }
               }
               if (FD_ISSET(i, &writefds)) {
                  DEBUG((SGE_EVENT, "can write fd=%d", i));
                  /* we can write to fd with number i */
                  mp = search_message(i, 1);
                  write2fd(mp, port_security, port);
                  FD_CLR(i, &writefds);
                  nfd--;
                  /* if the message is in a state where we can */
                  /* directly go on reading from the fd, then we */
                  /* try to do so. */
                  if((mp = search_message(i, 0)) &&
                     mp->tofd == i &&
                     (MESSAGE_STATUS(mp) == S_ACK)) {
                     FD_SET(mp->tofd, &readfds);
                     nfd++;
                     i--; /* to prevent looping until FD_SETSIZE */
                  }
               }
            }
         }
      }
      memorylack = 0;           /* retry getting memory every second */


      /*
       *  calculate profiling data (if enabled )
       */
      if ( now - last_loop_time > 10 && enable_commd_profile ) {
         double last_interval_time = now - last_loop_time; 
         double kbit_per_second = 0.0;
         double mc_per_second = 0.0;
         double new_m_per_second  = 0.0;
         double del_m_per_second = 0.0;
         double resend_m_per_second = 0.0;
         double new_sockets_per_second = 0.0;
         double con_errors_per_second = 0.0;
         double con_closed_per_second = 0.0;
         double con_accepted_per_second = 0.0;
         double unique_hosts_messages_per_second = 0.0;
         double med_message_size = 0.0;


         /*
          * normalize avg. message data size 
          */
         if ( data_message_count > 0 ) {
            med_message_size = data_message_byte_count / data_message_count ;
            med_message_size = med_message_size / 1024.0;
         } 

         /*
          * normalize times to per second values
          */
         if (last_interval_time > 0) {
            kbit_per_second = bytes_sent / last_interval_time;
            kbit_per_second = (kbit_per_second * 8.0) / 1024.0;
            mc_per_second = count_messages() / last_interval_time;
            new_m_per_second = new_messages / last_interval_time;
            del_m_per_second = del_messages / last_interval_time;
            resend_m_per_second = resend_messages / last_interval_time;
            new_sockets_per_second = sockets_created / last_interval_time;
            con_errors_per_second  = connection_errors / last_interval_time;
            con_closed_per_second   = connection_closed / last_interval_time;
            con_accepted_per_second = connection_accept / last_interval_time;
            unique_hosts_messages_per_second = unique_hosts_messages / last_interval_time;
         }

         /*
          * calculate maximas
          */
         max_kbit_per_second = MAX(max_kbit_per_second,kbit_per_second);
         max_mc_per_second = MAX(max_mc_per_second,mc_per_second);
         max_new_m_per_second  = MAX(max_new_m_per_second,new_m_per_second);
         max_del_m_per_second = MAX(max_del_m_per_second,del_m_per_second);
         max_resend_m_per_second = MAX(max_resend_m_per_second,resend_m_per_second);
         max_new_sockets_per_second = MAX(max_new_sockets_per_second,new_sockets_per_second);
         max_con_errors_per_second = MAX(max_con_errors_per_second,con_errors_per_second);
         max_con_closed_per_second = MAX(max_con_closed_per_second,con_closed_per_second);
         max_con_accepted_per_second = MAX(max_con_accepted_per_second,con_accepted_per_second);
         max_unique_hosts_messages_per_second = MAX(max_unique_hosts_messages_per_second,unique_hosts_messages_per_second);
         max_med_message_size = MAX(max_med_message_size,med_message_size);

         
         last_loop_time = now;
         last_logginglevel = log_state_get_log_level();
         log_state_set_log_level(LOG_INFO);

         /* 
          * common  
          *            kbit_per_second                  sent kilobytes per second for all messages
          *            unique_hosts_messages_per_second number of UNIQUE HOST requests
          *            med_message_size                 average message size of messages sent to commlib
          *                                             clients or other commds
          * messages
          *            mc_per_second                    number of messages not yet fetched by commlib clients
          *                                             some of these messages will be delivered to remote commds
          *            new_m_per_second                 number of new messages (all)
          *            del_m_per_second                 number of deleted messages (all)
          *            resend_m_per_second              number of messages for which init_send() has been called
          *                                             (commd-commd)
          * socketes   
          *            new_sockets_per_second           number of sockets created for commd-commd message transfer   
          *            con_errors_per_second            number of commd-commd message transfer tries where connect()
          *                                             failed
          *            con_closed_per_second            number of closed file descriptors with all messages
          *            con_accepted_per_second          number of connections accepted commlib clients and other 
          *                                             commds
          * gethostbyname
          *            gethostbyname_calls              number of gethostbyname() calls
          *            gethostbyname_sec                wall-clock time spent in gethostbyname() calls
          *                                             commds
          * gethostbyaddr
          *            gethostbyaddr_calls              number of gethostbyaddr() calls
          *            gethostbyaddr_sec                wall-clock time spent in gethostbyaddr() calls
          */
            
         INFO((SGE_EVENT, "Profile : ==========================")); 

         INFO((SGE_EVENT, "Profile1: %ld, common(KBit/s=%.3f, hostres/s=%.3f, avgmsize = %.3f Kbyte)",
                           now , kbit_per_second , unique_hosts_messages_per_second , med_message_size
         )); 
         INFO((SGE_EVENT, "Profile1: %ld, messages(wait/s=%.3f, new/s=%.3f, del/s=%.3f, init_send/s=%.3f)",
                           now , mc_per_second , new_m_per_second , del_m_per_second  , resend_m_per_second
         )); 
         INFO((SGE_EVENT, "Profile1: %ld, sockets(new/s: %.3f, errors/s=%.3f, closed/s=%.3f, accept/s=%.3f)",
                           now , new_sockets_per_second , con_errors_per_second , 
                           con_closed_per_second , con_accepted_per_second
         )); 

         INFO((SGE_EVENT, "Profile2: %ld, MAX common(KBit/s=%.3f, hostres/s=%.3f, avgmsize = %.3f Kbyte)",
                           now , max_kbit_per_second , max_unique_hosts_messages_per_second , max_med_message_size
         )); 
         INFO((SGE_EVENT, "Profile2: %ld, MAX messages(wait/s=%.3f, new/s=%.3f, del/s=%.3f, init_send/s=%.3f)",
                           now , max_mc_per_second , max_new_m_per_second , max_del_m_per_second  , max_resend_m_per_second
         )); 
         INFO((SGE_EVENT, "Profile2: %ld, MAX sockets(new/s: %.3f, errors/s=%.3f, closed/s=%.3f, accept/s=%.3f)",
                           now , max_new_sockets_per_second , max_con_errors_per_second , 
                           max_con_closed_per_second , max_con_accepted_per_second
         )); 
         
         INFO((SGE_EVENT, "Profile3: %ld, gethostbyname(calls=%ld time=%lds) gethostbyaddr(calls=%ld time=%lds)",
                           now, gethostbyname_calls, gethostbyname_sec, gethostbyaddr_calls, gethostbyaddr_sec
         )); 

         log_state_set_log_level(last_logginglevel);
         reset_measurement_data();
      }
   }
}

/*-----------------------------------------------------------------------*/
static void commd_usage(
FILE *out,
char **argv 
) {
   DENTER(TOP_LAYER, "commd_usage");

   fprintf(out, "%s\n", get_short_product_name() );

   fprintf(out, "%s %s [-s service] [-p port] [-ml fname] [-ll loglevel] [-nd] [-a aliasfile][-dhr]\n", MSG_USAGE, argv[0]); 
   fprintf(out, "    -s   %s [commd]\n", MSG_COMMD_s_OPT_USAGE);
   fprintf(out, "    -p   %s [commd]\n", MSG_COMMD_p_OPT_USAGE);
   fprintf(out, "    -ml  %s", MSG_COMMD_ml_OPT_USAGE);
   fprintf(out, "    -ll  %s", MSG_COMMD_ll_OPT_USAGE);
   fprintf(out, "    -nd  %s", MSG_COMMD_nd_OPT_USAGE);
   fprintf(out, "    -a   %s", MSG_COMMD_a_OPT_USAGE);
   fprintf(out, "    -dhr %s", MSG_COMMD_dhr_OPT_USAGE);
   SGE_EXIT(1);
}

/* If this function is called, we have a real problem.
   We have to decide in all parts of the code whether it makes sense to
   do s.th. without the possibility of allocating memory.
   Deliveration of messages should be forced, cause this frees some memory
   for further processing.
   Avoid busy looping on readfds. We have no chance to proceed with this
   requests. Because we are not notified when memory gets available we have
   to do some sort of polling for memory every given interval.
 */
void lack_of_memory()
{
   DENTER(TOP_LAYER, "lack_of_memory");
   
   if (memorylack) {              /* noticed allready */
      DEXIT;
      return;
   }   
   ERROR((SGE_EVENT, MSG_MEMORY_LACKOFMEMORY ));
   memorylack = 1;
   DEXIT;
}

/****** commd/reset_measurement_data() *****************************************
*  NAME
*     reset_measurement_data() -- reset profiling data
*
*  SYNOPSIS
*     void reset_measurement_data(void) 
*
*  FUNCTION
*     When a measurement interval is over this function is called to reset
*     the global data variables.
*
*  SEE ALSO
*     commd/reset_profiling_data()
*******************************************************************************/
void reset_measurement_data(void) 
{
   DENTER(TOP_LAYER, "reset_measurement_data");
   new_messages = 0;
   del_messages = 0;
   sockets_created = 0;
   resend_messages = 0;
   connection_errors = 0;
   connection_closed = 0;
   connection_accept = 0;
   bytes_sent = 0;
   unique_hosts_messages = 0;
   gethostbyname_calls = 0;
   gethostbyname_sec = 0;
   gethostbyaddr_calls = 0;
   gethostbyaddr_sec = 0;
   data_message_byte_count = 0;
   data_message_count = 0;
   DEXIT;
}

/****** commd/reset_profiling_data() *******************************************
*  NAME
*     reset_profiling_data() -- reset profiling data (and maximum values)
*
*  SYNOPSIS
*     void reset_profiling_data(void) 
*
*  FUNCTION
*     This function is called to reset all profiling data. 
*
*  SEE ALSO
*     commd/reset_measurement_data()
*
*******************************************************************************/
void reset_profiling_data(void)
{
   DENTER(TOP_LAYER, "reset_profiling_data");
   max_med_message_size = 0.0;
   max_kbit_per_second = 0.0;
   max_mc_per_second = 0.0;
   max_new_m_per_second  = 0.0;
   max_del_m_per_second = 0.0;
   max_resend_m_per_second = 0.0;
   max_new_sockets_per_second = 0.0;
   max_con_errors_per_second = 0.0;
   max_con_closed_per_second = 0.0;
   max_con_accepted_per_second = 0.0;
   max_unique_hosts_messages_per_second = 0.0;
   reset_measurement_data();
   DEXIT;
}

void enable_commd_profiling(int flag)
{
   DENTER(TOP_LAYER, "enable_commd_profiling");
   enable_commd_profile = flag;
   DEXIT;
}




/* search through messages and look for those who needs reading */
int build_read_fd_set(
fd_set *readfds,
message *mp,
commproc *commp 
) {
   int maxfd = -1;
  
   DENTER(TOP_LAYER, "build_read_fd_set");
 
   FD_ZERO(readfds);

   while (mp) {
      switch (MESSAGE_STATUS(mp)) {
      case S_RECEIVE_PROLOG:
      case S_RECEIVE:
         FD_SET(mp->fromfd, readfds);
         maxfd = MAX(maxfd, mp->fromfd);
         DPRINTF(("read fd=%d S_RECEIVE\n", mp->fromfd));
         break;
      case S_ACK:
         /* This message was received through fromfd and sent through
            tofd. If this is a synchron message fromfd is still open
            and connected with the sender. We wait for an acknowledge
            on tofd. If the sender times out (synchron send) we get an
            NACK_TIMEOUT from fromfd. If the sender breaks we get an
            EOF from fromfd. In both cases we advance this to tofd. */
         if (mp->fromfd != -1) {
            FD_SET(mp->fromfd, readfds);
            maxfd = MAX(maxfd, mp->fromfd);
            DPRINTF(("read fd=%d S_ACK fromfd\n", mp->fromfd));
         }
         FD_SET(mp->tofd, readfds);
         maxfd = MAX(maxfd, mp->tofd);
         DPRINTF(("read fd=%d S_ACK tofd\n", mp->tofd));
         break;

      case S_RDY_4_SND:
         if (mp->fromfd != -1) {
            FD_SET(mp->fromfd, readfds);
            maxfd = MAX(maxfd, mp->fromfd);
            DPRINTF(("read fd=%d S_RDY_4_SND fromfd\n", mp->fromfd));
         }
      }
      mp = mp->next;
   }

   /* if there are commprocs hanging around and waiting for a reply, trace
      for a breakdown of the open connection. If the connection is lost
      make a leave for the commproc (consider him dead). 
      Commpocs waiting for a message can run into a timeout. If so they 
      send us an ACK_TIMEOUT. Leave them enrolled. */

   while (commp) {
      if (commp->w_fd != -1) {
         FD_SET(commp->w_fd, readfds);
         maxfd = MAX(maxfd, commp->w_fd);
         DPRINTF(("read fd=%d commproc %s w_fd\n", commp->w_fd, commp->name));
      }
      if (commp->fd != -1) {
         FD_SET(commp->fd, readfds);
         maxfd = MAX(maxfd, commp->fd);
         DPRINTF(("read fd=%d commproc %s fd\n", commp->fd, commp->name));
      }
      commp = commp->next;
   }

   DEXIT;
   return maxfd;
}

/*******************************************************
 Build the writing filedescriptor set for select.
 *******************************************************/
int build_write_fd_set(
fd_set *writefds,
message *mp 
) {
   int maxfd = -1;
   
   FD_ZERO(writefds);

   while (mp) {
      switch (MESSAGE_STATUS(mp)) {
      case S_WRITE_ACK:
      case S_WRITE_ACK_SND:
         FD_SET(mp->fromfd, writefds);
         maxfd = MAX(maxfd, mp->fromfd);
         break;
      case S_WRITE_ACKE:
      case S_WRITE:
      case S_ACK_THEN_PROLOG:
      case S_WRITE_PROLOG:
      case S_SENDER_TIMEOUT:
      case S_CONNECTING:
         FD_SET(mp->tofd, writefds);
         maxfd = MAX(maxfd, mp->tofd);
         break;
      }
      mp = mp->next;
   }
   return maxfd;
}

/*---------------------------------------------------------------*/
void sighandler(
int sig 
) {
   message *m = message_list;
   commproc* c = commprocs;

   DENTER(TOP_LAYER, "sighandler");
   
   if (sig == SIGPIPE) {
      DEBUG((SGE_EVENT, "received signal SIGPIPE"));
      DEXIT;
      return;
   }

   if (sig == SIGHUP) {
      dump();
      DEXIT;
      return;
   }

   /* shutdown all sockets */

   if (sockfd) {
      DEBUG((SGE_EVENT, "received signal SIGINT - shutdown socket fd=%d", 
             sockfd));
      shutdown(sockfd, 2);
      close(sockfd);
   }

   while (m) {
      if (m->fromfd != -1)
         fd_close(m->fromfd, "shutdown all fromfd's");
      if (m->tofd != -1)
         fd_close(m->tofd, "shutdown all tofd's");
      m = m->next;
   }

   while (c) {
      shutdown(c->fd, 2);
      close(c->fd);
      DEBUG((SGE_EVENT, "closing fd=%d", c->fd));
      c = c->next;
   }
   
   DEXIT;
   WARNING((SGE_EVENT, MSG_SIGNAL_SIGTERMSHUTDOWN ));
   SGE_EXIT(1);
}



/* make a printable string from an fd_set showing all set bits 
   start[-end],...
 */
static char *fdsetstr(
fd_set *fds,
char *cp 
) {
   int on = 0, set, i;
   int start = 0, first = 1;
   char tmpstr[256];

   strcpy(cp, "");

   for (i = 0; i < FD_SETSIZE; i++) {
      set = FD_ISSET(i, fds);
      if (on) {
         if (!set) {
            if (!first)
               strcat(cp, ",");
            if (start == i - 1)
               sprintf(tmpstr, "%d", start);
            else
               sprintf(tmpstr, "%d-%d", start, i - 1);
            strcat(cp, tmpstr);
            first = 0;
            on = 0;
         }
      }
      else {
         if (set) {
            on = 1;
            start = i;
         }
      }
   }
   return cp;
}


void dump()
{
   FILE *fp;

   DENTER(TOP_LAYER, "dump");
   
   WARNING((SGE_EVENT, MSG_COMMD_DUMPTOFILE ));

   fp = fopen("/tmp/commd/commd.dump", "w");
   if (fp) {
      fprintf(fp, "----------  HOSTLIST ----------\n");
      sge_host_list_print(fp);
      fprintf(fp, "----------  COMMPROCLIST ------\n");
      print_commprocs(fp);
      fprintf(fp, "----------  MESSAGELIST ------\n");
      print_messages(fp);
      fclose(fp);
   }

#ifdef DMALLOC
   dstatus("/tmp/commd/commd.mem", 0, 0);
#endif

  DEXIT;  
}

/*--------------------------------------------------------
 * schedule_resends
 *
 * look for messages which couldn't be sent to remote commds -> try it again 
 *  dont look for local request messages (RECEIVE|...) 
 *--------------------------------------------------------*/
static void schedule_resends(
int reserved_port,
int commdport,
u_long now 
) {
   message *mp = message_list, *next;
   static u_long lasttime = 0;

   DENTER(TOP_LAYER, "schedule_resends");
   
   if (now - lasttime > 10) {
      lasttime = now;
      while (mp) {
         next = mp->next;       /* may be init_send deletes message */
         if (MESSAGE_STATUS(mp) == S_RDY_4_SND &&
             (!(mp->flags & (COMMD_RECEIVE | COMMD_LEAVE | COMMD_CNTL)))) {
            DEBUG((SGE_EVENT, "rescheduling message mid=%d", (int)mp->mid));
            resend_messages++;     /* profiling */
            init_send(mp, reserved_port, commdport);
         }
         mp = next;
      }
   }
   DEXIT;
}


/*--------------------------------------------------------*/
void log_state_transition_too_many_fds_open(
u_long t 
) {
   static u_long time = 0;      /* identically to "too_many_fds_open" */

   DENTER(TOP_LAYER, "log_state_transition_too_many_fds_open");
   if ((time && !t) || (!time && t)) {
      if (!time)
         WARNING((SGE_EVENT, MSG_FILE_TOMANYFDSSTART ));
      else
         WARNING((SGE_EVENT, MSG_FILE_TOMANYFDSEND ));
   }

   time = t;
   
   DEXIT;
   return;
}

/*--------------------------------------------------------
 * seek_badfd
 *
 * scan fd_sets and return fd that causes an EBADF errno 
 * if no such fd is found -1 is returned 
 * a fd_set ptr may be NULL
 *--------------------------------------------------------*/
int seek_badfd(
int nfds,
fd_set *rfds,
fd_set *wfds 
) {
   int i;
   int ret;
   fd_set check_set;
   struct timeval timeout;

   if (rfds) {
      for (i = 0; i < nfds; i++) {
         /* check read fd's */
         if (FD_ISSET(i, rfds)) {
            FD_ZERO(&check_set);
            FD_SET(i, &check_set);

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
            ret = select(nfds, (int *) &check_set, NULL, NULL, &timeout);
#else
            ret = select(nfds, &check_set, NULL, NULL, &timeout);
#endif
            if (ret == -1 && errno == EBADF) {
               /* found it */
               return i;
            }
         }
      }
   }

   if (wfds) {
      for (i = 0; i < nfds; i++) {
         /* check write fd's */
         if (FD_ISSET(i, wfds)) {
            FD_ZERO(&check_set);
            FD_SET(i, &check_set);

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
            ret = select(nfds, NULL, (int *) &check_set, NULL, &timeout);
#else
            ret = select(nfds, NULL, &check_set, NULL, &timeout);
#endif
            if (ret == -1 && errno == EBADF) {
               /* found it */
               return i;
            }
         }
      }
   }

   return -1;
}
