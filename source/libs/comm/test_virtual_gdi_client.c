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
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>



#include "cl_lists.h"
#include "cl_commlib.h"


/* counters */
static int rcv_messages = 0;
static int snd_messages = 0;

void sighandler_client(int sig);
static int pipe_signal = 0;
static int hup_signal = 0;
static int do_shutdown = 0;

static cl_com_handle_t* handle = NULL; 

void sighandler_client(
int sig 
) {
/*   thread_signal_receiver = pthread_self(); */
   if (sig == SIGPIPE) {
      pipe_signal = 1;
      return;
   }

   if (sig == SIGHUP) {
      hup_signal = 1;
      return;
   }
   printf("do_shutdown\n");
   /* shutdown all sockets */
   do_shutdown = 1;
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "main()"
extern int main(int argc, char** argv)
{
  struct sigaction sa;
  
  struct timeval now;
  time_t last_time = 0;

  if (argc != 4) {
      printf("please enter  debug level, port and hostname of virtual qmaster\n");
      exit(1);
  }

  /* setup signalhandling */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sighandler_client;  /* one handler for all signals */
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);


  while(do_shutdown == 0) {
     gettimeofday(&now,NULL);
     if (now.tv_sec != last_time) {
        printf("virtual gdi client message count[received |%d| / sent |%d|]...\n",rcv_messages,snd_messages);
        last_time = now.tv_sec;
     }
     cl_com_setup_commlib(CL_NO_THREAD ,atoi(argv[1]), NULL );
   
     handle=cl_com_create_handle(CL_CT_TCP,CL_CM_CT_MESSAGE , 0, atoi(argv[2]) , "virtual_gdi_client", 0, 1,0 );
     if (handle == NULL) {
        printf("could not get handle\n");
        exit(1);
     }
   
#if 0
     printf("local hostname is \"%s\"\n", handle->local->comp_host);
     printf("local component is \"%s\"\n", handle->local->comp_name);
     printf("local component id is \"%ld\"\n", handle->local->comp_id);
#endif
   
#if 0
     {
        int i;
        cl_com_get_connect_port(handle, &i);
        printf("connecting to port \"%d\" on host \"%s\"\n", i, argv[3]);
     }
#endif
   
     
     
     while( do_shutdown == 0 ) {
        int                retval  = 0;
        cl_com_message_t*  message = NULL;
        cl_com_endpoint_t* sender  = NULL;
        char data[20000];
   
        sprintf(data,"gdi request");
        retval = cl_commlib_send_message(handle, argv[3], "virtual_master", 1,
                                         CL_MIH_MAT_NAK,
                                         (cl_byte_t*) data , 20000,
                                         NULL, 0, 0 , 1, 0 );
        if ( retval == CL_RETVAL_OK ) {
           snd_messages++;
           retval = cl_commlib_receive_message(handle, NULL, NULL, 0,      /* handle, comp_host, comp_name , comp_id, */
                                               1, 0,                          /* syncron, response_mid */
                                               &message, &sender );
           if ( retval == CL_RETVAL_OK) {
#if 0
                 printf("received message from %s/%s/%ld: \"%s\" (%ld bytes)\n", 
                           sender->comp_host,sender->comp_name,sender->comp_id, message->message,message->message_length);
#endif
                 rcv_messages++;
                 cl_com_free_message(&message);
                 cl_com_free_endpoint(&sender);
                 break;
           }
        } 
#if 0        
        printf("status: %s\n",cl_get_error_text(retval));
#endif
     }
     cl_com_cleanup_commlib();
  }
  
  printf("main done\n");
  return 0;
}

