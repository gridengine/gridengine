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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "msg_commd.h"
#include "sge_language.h"
#include "sge_profiling.h"



void usage(void);
int main(int argc, char **argv);
void run_server_test(int port,int repeat);
void start_server_handling(int sockfd);
void start_server_handling2(int sockfd,int repeat);
int get_open_connections(int *open_connections);
void handle_new_connect(int sockfd, int *open_connections);
int read_data(int fd, char* data);
int write_data(int fd, char* data);
void close_open_connection(int fd, int *open_connections);
int run_client_test(char* host, int port, int repeat);
int run_client_test2(char* host, int port, int repeat);


char mydata[30000];
int tcptime = 30;
void usage()
{  
   printf("%s\n", MSG_USAGE);
   printf("testsuite_rcv -host SENDER_HOST -port PORT [-closefd] [-sync]\n");
   printf("   [-datasize VALUE] [-repeat VALUE]\n");
   printf("-host SENDER_HOST       host who sends the messages\n");
   printf("-port PORT              second client/server port for direct TCP/IP connection\n");
   printf("-closefd                close file descriptors for commd test\n");
   printf("-sync                   use synchron send method for commd test\n");
   printf("-repeat   VALUE         nr. of receive repeats\n");
   printf("-sndname  VALUE         name of sender commproc\n");
   printf("-no-tcp                 don't run tcp/ip test, report message order (FIFO) errors\n");
   printf("-no-commd               don't run commd test\n");
   printf("-tcp-closefd            init new connection in tcp test for each message\n");
   exit(1);
}

int tcp_closefd_flag = 0;
int main(
int argc,
char **argv 
) {

   char host[256] = "";
   char receiver_enroll[256] = "tstrcv";
   int receiver_enroll_id = 1;
   char fromcommproc[256] = "tstsnd";            /* name of sender */
   int closefd = 0;            /* close file descriptor   0/1 */
   int synchron = 0;           /* syncron 0/1 */

   u_long32 message_count = 0;

   char *buffer = NULL;
   u_long32 buflen = 0;

   u_short fromid = 1;
   int tag = 1;
   int no_tcp_flag = 0;
   int no_commd_flag = 0;
   
   int i;
   int repetitions;
   u_long32 nr_bytes = 0;
   double bytes_per_second;
   double run_time;
   int first_message;  

   int port = 0;
   int datasize = 1024;
   int repeat = 5000;
   int wrong_orders = 0;


   DENTER_MAIN(TOP_LAYER, "testsuite_rcv");

#ifdef __SGE_COMPILE_WITH_GETTEXT__
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

  
   while (*(++argv)) {
      if (!strcmp("-h", *argv))
         usage();
      if (!strcmp("-help", *argv))
         usage();

      if (!strcmp("-host", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(host, *argv);
      }
      if (!strcmp("-sndname", *argv)) {
         argv++;
         if (!*argv)
            usage();
         strcpy(fromcommproc, *argv);
      }

      if (!strcmp("-port", *argv)) {
         argv++;
         if (!*argv)
            usage();
         port = atoi(*argv);
      }
      if (!strcmp("-datasize", *argv)) {
         argv++;
         if (!*argv)
            usage();
         datasize = atoi(*argv);
      }
      if (!strcmp("-repeat", *argv)) {
         argv++;
         if (!*argv)
            usage();
         repeat = atoi(*argv);
      }
      if (!strcmp("-tcptime", *argv)) {
         argv++;
         if (!*argv)
            usage();
         tcptime = atoi(*argv);
      }

      if (!strcmp("-closefd", *argv)) {
         closefd = 1;
      }
      if (!strcmp("-no-tcp", *argv)) {
         no_tcp_flag = 1;
      }
      if (!strcmp("-no-commd", *argv)) {
         no_commd_flag = 1;
      }
      if (!strcmp("-tcp-closefd", *argv)) {
         tcp_closefd_flag = 1;
      }
      if (!strcmp("-sync", *argv)) {
         synchron = 1;
      }
   }
   strcpy(mydata,"");
   for (i=1;i<datasize;i++) {
      strcat(mydata,"_");
   }


   if (strcmp(host,"") == 0) {
      printf("no client host name given (use -host argument)\n");
      exit(-1);
   }

   if (port == 0) {
      printf("no client port number given (use -port argument)\n");
      exit(-1);
   }
   printf("starting test with following parameters:\n");
   printf("port=%d\n",port);
   printf("host=%s\n",host);
   printf("closefd=%d\n",closefd);
   printf("sync=%d\n",synchron);
   printf("repeat=%d\n",repeat);
   
   if (!no_commd_flag) { 
      i = set_commlib_param(CL_P_CLOSE_FD, closefd, NULL, NULL);
         DPRINTF(("set_commlib_param(CL_P_CLOSE_FD, %d) returns %d\n", closefd, i));
   
      set_commlib_param(CL_P_NAME, 0, receiver_enroll, NULL);
      set_commlib_param(CL_P_ID,  receiver_enroll_id, NULL, NULL);
   /*   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, priority_tag_list); */
   
   
      repetitions = repeat;
      first_message = 1;
   
      printf("CONNECTED TO COMMD\n");
      prof_start(NULL);
   
      while (repetitions) {
         i = receive_message(fromcommproc, &fromid, host, &tag, &buffer,
                             &buflen, synchron, NULL);
         DPRINTF(("rcv_message returned: %d\n", i));
         if (i == CL_OK) {
            message_count++;
            if (first_message == 1) {
               first_message = 0;
               PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
               nr_bytes = 0;
            }
            DPRINTF(("buflen = %ld\n", buflen));
            if (no_tcp_flag) {
               if ( atoi(buffer) != message_count) {
                  wrong_orders++;
               }
            }
            DPRINTF(("fromcommproc = %s\n", fromcommproc));
            DPRINTF(("fromid = %d\n", fromid));
            DPRINTF(("host = %s\n", host));
            DPRINTF(("tag = %d\n", tag));
            repetitions--;
   /*         if (strcmp(buffer,"LAST_MESSAGE") == 0) {
               repetitions = 0;
            } */
            nr_bytes = nr_bytes + buflen;
   
            free(buffer);
            buffer = NULL;
            fflush(stdout);
         } else {
            sleep(1); 
            printf(MSG_ERROR_S , cl_errstr(i));
            printf("count >"u32"<\n",message_count);
            fflush(stdout); 
         }
      }                            /* while repetitions */
      printf ("wrong message orders: %d\n",wrong_orders ); 
      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
      run_time = prof_get_measurement_wallclock(SGE_PROF_CUSTOM0,true,NULL);
   
      printf("COMMD: "u32" bytes received in %.3f seconds\n", nr_bytes,run_time);
      if ( run_time > 0.0) {
         bytes_per_second = (double)nr_bytes / run_time / 1024.0 * 8.0;
         printf("COMMD: %.3f KBit/s\n",bytes_per_second);
      }
   
   
      i = leave_commd();
      DPRINTF(("leave returned %d\n", i));
      if (i)
         printf(MSG_ERROR_S , cl_errstr(i));
   
      fflush(stdout);
   }
   if (no_tcp_flag) {
      return 0;
   }
   if ( tcp_closefd_flag ) {
      int is_receiver_ready = 0;
      repetitions = repeat;
      prof_start(NULL);
      nr_bytes = 0;
      while (repetitions-- > 0 ) {
         int back = 0; 
         while( (back=run_client_test2(host, port,1)) <= 0 ){
            sleep(1);
         }
         if (!is_receiver_ready ) {
            is_receiver_ready = 1;
            PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
         }
         nr_bytes = nr_bytes + back;
      }
      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
      run_time = prof_get_measurement_wallclock(SGE_PROF_CUSTOM0,true,NULL);
      printf("TCP/IP: %ld bytes send in %.3f seconds\n", (long)nr_bytes,run_time);
      if ( run_time > 0) {
         bytes_per_second = (double)nr_bytes / (double)run_time / 1024.0 * 8.0;
         printf("TCP/IP: %.3f KBit/s\n",bytes_per_second);
         fflush(stdout);
      }
   } else {
   
      while ( run_client_test(host, port,repeat) != 0) {
         printf("waiting for connect ...\n");
         sleep(1);
      } 
   }
   fflush(stdout);
   return 0;
}

void run_server_test(int port, int repeat) {
   int sockfd = 0;
   int on = 1;
   struct sockaddr_in serv_addr;


   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("can't open socket\n");
      exit(-1);
   }   

   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

   /* bind an address to socket */
   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_port = htons(port);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      printf("can't bind socket\n");
      exit(-1);
   }

   /* make socket listening for incoming connects */
   listen(sockfd, 5);
   if (!tcp_closefd_flag) {
      start_server_handling(sockfd);
   } else {
      start_server_handling2(sockfd,repeat);
   }
}

int run_client_test(char* host, int port, int repeat) {
   int sockfd = 0;
   int i,sso;
   struct hostent *he;
   struct sockaddr_in client_addr;
   fd_set writefds;
   struct timeval timeout;
   int resends = 0;


   char buffer[30000]; 
   u_long32 nr_bytes;
   u_long32 start_time;
   double bytes_per_second;
   double run_time;

   
   
   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("can't open socket\n");
      return -1;
   }   

/*   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)); */




/*   fcntl(sockfd, F_SETFL, O_NONBLOCK);  */
   /* doing this later to block until connect is successfully */
   
   
   /* bind an address to socket */
   memset((char *) &client_addr, 0, sizeof(client_addr));
   client_addr.sin_port = htons(port);
   client_addr.sin_family = AF_INET;
   he = gethostbyname(host);
   memcpy((char *) &client_addr.sin_addr, (char *) he->h_addr, he->h_length);

      i = connect(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr));
      if (i == -1) {
         if (errno == ECONNREFUSED || errno == EADDRNOTAVAIL ) {
            printf("connection refused or not available\n");
            shutdown(sockfd, 2);
            close(sockfd);
            return -1;
         }
         
         if (errno == EINPROGRESS) {
            printf("connect in progress ...\n");
         } else {
            printf("error for connect, errno = %s\n", strerror(errno));
            shutdown(sockfd, 2);
            close(sockfd);
            return -1;
         }
      } 
   sso = 1;

   printf("sockfd is: %d\n",sockfd);   

   fcntl(sockfd, F_SETFL, O_NONBLOCK); 



#if defined(SOLARIS) && !defined(SOLARIS64)
   setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int));
#else
   setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int));
#endif




   
   printf("connected\n");

   nr_bytes = 0;
   start_time = sge_get_gmt();

   printf("sockfd is: %d\n",sockfd);   
   resends = repeat;
   prof_start(NULL);
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
   while(resends-- > 0) {   
      int help = strlen(mydata)+1;
      int sent_data = 0;
      int write_ret = 0;
      int maxfd = 0;
      int select_back = 0;
      

      FD_ZERO(&writefds);
      maxfd = MAX(maxfd,sockfd);
      FD_SET(sockfd, &writefds); 
      
      timeout.tv_sec = 2; 
      timeout.tv_usec = 0;
#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
      select_back = select(FD_SETSIZE, NULL, (int *) &writefds, NULL, &timeout);
#else
      select_back = select(FD_SETSIZE, NULL, &writefds, NULL, &timeout);
#endif
      if (select_back > 0) {
         strcpy(buffer,mydata);
         while (sent_data != help) {
             write_ret = write_data(sockfd,buffer);
             if (write_ret > 0) {
                sent_data = sent_data + write_ret; 
             }
         }
         nr_bytes = nr_bytes + help;
      }
      if (select_back == -1) {
         if (errno == EBADF) {
            printf("error for select, errno = EBADF\n");
            printf("error for select, errno = %s\n", strerror(errno));
         } else {
            printf("error for select, errno = %s\n", strerror(errno));
         }
      } 
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
   run_time = prof_get_measurement_wallclock(SGE_PROF_CUSTOM0,true,NULL);

   printf("TCP/IP: "u32" bytes send in %.3f seconds\n", nr_bytes,run_time);
   if ( run_time > 0.0) {
      bytes_per_second = (double)nr_bytes / (double)run_time / 1024.0 * 8.0;
      printf("TCP/IP: %.3f KBit/s\n",bytes_per_second);
      fflush(stdout);
   }

   shutdown(sockfd, 2);
   close(sockfd);
   return 0;
}

int run_client_test2(char* host, int port, int repeat) {
   int sockfd = 0;
   int i,sso;
   struct hostent *he;
   struct sockaddr_in client_addr;
   fd_set writefds;
   struct timeval timeout;
   int resends = 0;
   char buffer[30000]; 
   u_long32 nr_bytes;
   int sent_data = 0;
   
   
   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("can't open socket\n");
      fflush(stdout);
      return -1;
   }   
/*   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)); */


/*   fcntl(sockfd, F_SETFL, O_NONBLOCK);  */
   /* doing this later to block until connect is successfully */
   
   
   /* bind an address to socket */
   memset((char *) &client_addr, 0, sizeof(client_addr));
   client_addr.sin_port = htons(port);
   client_addr.sin_family = AF_INET;
   he = gethostbyname(host);

   memcpy((char *) &client_addr.sin_addr, (char *) he->h_addr, he->h_length);

      i = connect(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr));
      if (i == -1) {
         if (errno == ECONNREFUSED || errno == EADDRNOTAVAIL ) {
            printf("connection refused or not available\n");
            fflush(stdout); 
            shutdown(sockfd, 2);
            close(sockfd);
            return -10;
         }
         
         if (errno == EINPROGRESS) {
            printf("connect in progress ...\n"); 
         } else {
            printf("error for connect, errno = %s\n", strerror(errno));
            fflush(stdout); 
            shutdown(sockfd, 2);
            close(sockfd);
            return -1;
         }
      } 
   sso = 1;

   fcntl(sockfd, F_SETFL, O_NONBLOCK);  



#if defined(SOLARIS) && !defined(SOLARIS64)
   setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int));
#else
   setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int));
#endif


   nr_bytes = 0;
   resends = repeat;
   while(resends-- > 0) {   
      int help = strlen(mydata)+1;
      int write_ret = 0;
      int maxfd = -1;
      int select_back;


      FD_ZERO(&writefds);
      maxfd = MAX(maxfd,sockfd);
      FD_SET(sockfd, &writefds); 
      
      timeout.tv_sec = 2; 
      timeout.tv_usec = 0;

#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
      select_back = select(FD_SETSIZE, NULL, (int *) &writefds, NULL, &timeout);
#else
      select_back = select(FD_SETSIZE, NULL, &writefds, NULL, &timeout);
#endif
      if (select_back > 0) {
         strcpy(buffer,mydata);
         while (sent_data != help) {
             write_ret = write_data(sockfd,buffer);
             if (write_ret > 0) {
                sent_data = sent_data + write_ret; 
             }
         }
         nr_bytes = nr_bytes + help;
      } 
      if (select_back == -1) {
         if (errno == EBADF) {
            printf("error for select, errno = EBADF\n");
            printf("error for select, errno = %s\n", strerror(errno));
         } else {
            printf("error for select, errno = %s\n", strerror(errno));
         }
      } 
   }
   shutdown(sockfd, 2);
   close(sockfd);
   return sent_data;
}


void start_server_handling(int sockfd) {
   int maxfd=0;
   fd_set readfds, writefds;
   struct timeval timeout;

   int select_back;
   int i,nfd;
   int open_connections[1000];
   char data_buffer[30000];
   char send_buffer[30000];
   u_long32 nr_bytes = 0;
   double bytes_per_second;
   double run_time;



   for (i=0;i<1000;i++) {
      open_connections[i] = -1;
   }

   strcpy(send_buffer,"");
   strcpy(data_buffer,"");

   prof_start(NULL);


   while(1) {
   FD_ZERO(&readfds);
   FD_ZERO(&writefds);

   maxfd = MAX(maxfd,sockfd);
   FD_SET(sockfd, &readfds);
 
   /* set client filedescriptors */
   for(i=get_open_connections(open_connections);i>0;i--) {
      FD_SET(open_connections[i-1],&readfds);
      maxfd = MAX(maxfd,open_connections[i-1]);
      if (strlen(send_buffer) > 0) {
         FD_SET(open_connections[i-1],&writefds);
      }
   }


   timeout.tv_sec = 2; 
   timeout.tv_usec = 0;
#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
   select_back = select(FD_SETSIZE, (int *) &readfds, (int *) &writefds, NULL, &timeout);
#else
   select_back = select(FD_SETSIZE, &readfds, &writefds, NULL, &timeout);
#endif
      
/*      printf("waiting for connections (select_back=%d)(open=%d)...\n",
             select_back,
             get_open_connections(open_connections)
      ); */
/*      for (i=0;i<10;i++) {
         printf("%d ",open_connections[i]);
      }
      printf("\n"); */

      if (select_back == -1) {
         /* error handling */
         if (errno == EBADF) {
            printf("error for select, errno = EBADF\n");
            printf("error for select, errno = %s\n", strerror(errno));
         } else {
            printf("error for select, errno = %s\n", strerror(errno));
         }
      } else {
         /* select was ok */
         nfd = select_back;
         for(i=0; nfd && i <= maxfd  ; i++) {
            int write_set = FD_ISSET(i, &writefds);
            int read_set = FD_ISSET(i, &readfds);
   
            if (!write_set && !read_set) {
               continue;
            }
   
            if (i != sockfd && read_set) {
               int bread = 0;
               nfd--;

               bread=read_data(i,data_buffer); 
               if(bread > 0 ) {
                  nr_bytes = nr_bytes + bread;

               }
               if (bread == -1) {
                  /* close connection */
                  close_open_connection(i,open_connections);
                  PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
                  run_time = prof_get_measurement_wallclock(SGE_PROF_CUSTOM0,true,NULL);

                  printf("TCP/IP: "u32" bytes received in %.3f seconds\n", nr_bytes,run_time);
                  if ( run_time > 0.0) {
                     bytes_per_second = (double)nr_bytes / run_time / 1024.0 * 8.0;
                     printf("TCP/IP: %.3f KBit/s\n",bytes_per_second);
                  }

                  exit(0);
               }
               /* strcpy(send_buffer,data_buffer); */  /* to test write */
            }   
            if (write_set) {
               if (write_data(i,send_buffer) == -1) {
                  /* close connection */
                  close_open_connection(i,open_connections);
               }
               nfd--;
            }   
            if (i == sockfd && read_set) {
               printf("new connection\n");
               handle_new_connect(sockfd,open_connections); 
               PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
               nr_bytes = 0;
            }
         }
      }
   }
}

void start_server_handling2(int sockfd,int repeat) {
   int maxfd=0;
   fd_set readfds, writefds;
   struct timeval timeout;

   int select_back;
   int i,nfd;
   int open_connections[1000];
   char data_buffer[30000];
   char send_buffer[30000];
   u_long32 nr_bytes = 0;
   double bytes_per_second;
   double run_time;
   int messages = 0;
   int mes_start = 0;


   for (i=0;i<1000;i++) {
      open_connections[i] = -1;
   }

   strcpy(send_buffer,"");
   strcpy(data_buffer,"");

   prof_start(NULL);


   while(1) {
   FD_ZERO(&readfds);
   FD_ZERO(&writefds);

   maxfd = MAX(maxfd,sockfd);
   FD_SET(sockfd, &readfds);
 
   /* set client filedescriptors */
#if 0
   for(i=get_open_connections(open_connections);i>0;i--) {
      FD_SET(open_connections[i-1],&readfds);
      maxfd = MAX(maxfd,open_connections[i-1]);
      if (send_buffer[0] != 0) {
         FD_SET(open_connections[i-1],&writefds);
      }
   }
#endif
#if 1 
   for(i=0; i<1000 && open_connections[i] != -1 ;i++) {
      FD_SET(open_connections[i],&readfds);
      maxfd = MAX(maxfd,open_connections[i]);
      if (send_buffer[0] != 0) {
         FD_SET(open_connections[i],&writefds);
      }
   }
#endif


   timeout.tv_sec = 2; 
   timeout.tv_usec = 0;
#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX)
   select_back = select(FD_SETSIZE, (int *) &readfds, (int *) &writefds, NULL, &timeout);
#else
   select_back = select(FD_SETSIZE, &readfds, &writefds, NULL, &timeout);
#endif
      
/*      printf("waiting for connections (select_back=%d)(open=%d)...\n",
             select_back,
             get_open_connections(open_connections)
      ); */
/*      for (i=0;i<10;i++) {
         printf("%d ",open_connections[i]);
      }
      printf("\n"); */

      if (select_back == -1) {
         /* error handling */
         if (errno == EBADF) {
            printf("error for select, errno = EBADF\n");
            printf("error for select, errno = %s\n", strerror(errno));
         } else {
            printf("error for select, errno = %s\n", strerror(errno));
         }
      } else {
         /* select was ok */
         nfd = select_back;
         for(i=0; nfd && i <= maxfd  ; i++) {
            int write_set = FD_ISSET(i, &writefds);
            int read_set = FD_ISSET(i, &readfds);
   
            if (!write_set && !read_set) {
               continue;
            }
   
            if (i != sockfd && read_set) {
               int bread = 0;
               nfd--;

               bread=read_data(i,data_buffer); 
               if(bread > 0 ) {
                  nr_bytes = nr_bytes + bread;

               }
               if (bread == -1) {
                  /* close connection */
                  close_open_connection(i,open_connections);
                  messages++;

                  if (messages == repeat) {
                     PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);
                     run_time = prof_get_measurement_wallclock(SGE_PROF_CUSTOM0,true,NULL);

                     printf("TCP/IP: %ld bytes received in %.3f seconds\n", (long)nr_bytes,run_time);
                     if ( run_time > 0.0) {
                        bytes_per_second = (double)nr_bytes / run_time / 1024.0 * 8.0;
                        printf("TCP/IP: %.3f KBit/s\n",bytes_per_second);
                     }

                     exit(0);
                  }
               }
               /* strcpy(send_buffer,data_buffer); */  /* to test write */
            }   
            if (write_set) {
               if (write_data(i,send_buffer) == -1) {
                  /* close connection */
                  close_open_connection(i,open_connections);
               }
               nfd--;
            }   
            if (i == sockfd && read_set) {
               /* printf("new connection\n"); */
               handle_new_connect(sockfd,open_connections); 
               if (mes_start == 0) {
                  PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);
                  nr_bytes = 0;
                  mes_start = 1;
               }
            }
         }
      }
   }
}




int get_open_connections(int *open_connections) {
   int i;

   for (i=0 ; i<1000 && open_connections[i] != -1 ; i++); 
   return i;
}

void close_open_connection(int fd, int *open_connections) {
   int max_connection = get_open_connections(open_connections);
   int i;
   int b;
   
   max_connection++;

   for (i=0;i<max_connection;i++) {
      if (open_connections[i] == fd) {
         shutdown(fd, 2);
         close(fd);
         for (b=i;b<=(max_connection-2);b++) {
            open_connections[b] = open_connections[b+1];
         }
         open_connections[(max_connection-1)] = -1;
         return;
      }
   }
}

void handle_new_connect(int sockfd, int *open_connections) {
   struct sockaddr_in cli_addr;
   int new_sfd = 0;
   int sso,i;
#ifdef AIX43
   size_t fromlen = 0;
#else
   int fromlen = 0;
#endif

   fromlen = sizeof(cli_addr);
   memset((char *) &cli_addr, 0, sizeof(cli_addr));
   new_sfd = accept(sockfd, (struct sockaddr *) &cli_addr, &fromlen);
   
   if (new_sfd == -1) {
      if (errno == EMFILE) { 
/*         printf("to many open files\n"); */
      }
/*      printf("error for accept, errno = %s\n", strerror(errno)); */
      return;
   }
   fcntl(new_sfd, F_SETFL, O_NONBLOCK);         /* HP needs O_NONBLOCK, was O_NDELAY */
   sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)
   if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int)) == -1)
#else
   if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int))== -1)
#endif
      printf("cannot setsockopt() to TCP_NODELAY.\n");

   /* here we can investigate more information about the client */
   /* ntohs(cli_addr.sin_port) ... */
   
   i = get_open_connections(open_connections);
   if (i>=1000) {
      printf("too much connections, try later\n");
      shutdown(new_sfd, 2);
      close(new_sfd);
      return;
   } 
   open_connections[i] = new_sfd;
}

int read_data(int fd, char* data) {
   int size;

   strcpy(data,"");
   size = read(fd,data,29999);
   if (size == 0) {
      /* printf("size is 0\n"); */
      return -1;
   }
   if (size == -1) {
      printf("error for read, errno = %s\n", strerror(errno));
      if(errno == EWOULDBLOCK || errno == EAGAIN) {
         printf("sender client not ready\n");
         return 0;
      }
      printf("read error\n");
      return 0;
   }
/*   printf("data size= %d\n",size);  */
   data[size] = 0;
/*   printf("received: %s",data);  */
   return size;
}

int write_data(int fd, char* data) {
   int length;
   int size;
   char buff[30000];

   length = strlen(data) + 1;
/*   printf("sending: %s\n",data); */

   size = write(fd, data, length );
   if (size == 0) {
/*       printf("could not write any data\n"); */
       return -1;
   }
   if (size == -1) {
       if(errno == EWOULDBLOCK || errno == EAGAIN) {
/*          printf("not ready - try again\n"); */
          return 0;
       }
/*       printf("write error\n"); */
       return -1;
   }

   if (size != length) {
/*       printf("could not write all data !!!\n");  */
       strcpy(buff,&data[size]);
       strcpy(data,buff);
   } else {  
       strcpy(data,"");
   }
/*   printf("%d data bytes written\n",size);   */
   return size;
}


















