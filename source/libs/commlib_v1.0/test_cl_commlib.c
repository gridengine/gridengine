
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
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "cl_commlib.h"
#define CL_DO_SLOW 0

void sighandler_server(int sig);
static int pipe_signal = 0;
static int hup_signal = 0;
static int do_shutdown = 0;

void sighandler_server(
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

   /* shutdown all sockets */
   do_shutdown = 1;
}


extern int main(int argc, char** argv)
{
  struct sigaction sa;


  cl_com_handle_t* handle = NULL; 
  cl_com_message_t* message = NULL;
  cl_com_endpoint_t* sender = NULL;
  int i;
  
  if (argc != 2) {
      printf("please enter debug level\n");
      exit(1);
  }

  /* setup signalhandling */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sighandler_server;  /* one handler for all signals */
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);


  printf("commlib setup ...\n");
  cl_com_setup_commlib(CL_ONE_THREAD, atoi(argv[1]),   NULL );

  handle=cl_com_create_handle(CL_CT_TCP,CL_CM_CT_MESSAGE , 1, 5000, 5001, "server", 1, 1, 0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     exit(-1);
  }

  cl_commlib_get_service_port(handle,&i), 


  printf("server running on host \"%s\", port %d, component name is \"%s\", id is %ld\n", 
         handle->local->comp_host, 
         i, 
         handle->local->comp_name,  
         handle->local->comp_id);

  cl_com_add_allowed_host(handle, handle->local->comp_host);
#if 0
  cl_com_add_allowed_host(handle, "waschbaer"); 
#endif
#if 1
  cl_com_add_allowed_host(handle,"sr-ergb01-01");
  cl_com_add_allowed_host(handle,"balin");
  cl_com_add_allowed_host(handle,"eomer"); 
  cl_com_add_allowed_host(handle,"durin");
  cl_com_add_allowed_host(handle,"orgulas");
  cl_com_add_allowed_host(handle,"ori"); 
  cl_com_add_allowed_host(handle,"es-ergb01-01"); 

#endif
  

  cl_com_set_max_connections(handle,4);
  cl_com_get_max_connections(handle,&i);
  printf("max open connections is set to %d\n", i);

  cl_com_enable_max_connection_close(handle);


  while(do_shutdown != 1) {
     unsigned long mid;
     int ret_val;

     CL_LOG(CL_LOG_INFO,"main()");
     cl_commlib_trigger(handle); 

#if 0
     /* TODO: check behaviour for unknown host and for a host which is down */
     cl_commlib_send_message(handle, "down_host", "nocomp", 1, CL_MIH_MAT_ACK, (cl_byte_t*)"blub", 5, NULL, 1, 1 ); /* check wait for ack / ack_types  TODO*/
#endif
 
     ret_val = cl_commlib_receive_message(handle,NULL, NULL, 0, 0, 0, &message, &sender);
     CL_LOG_STR(CL_LOG_INFO,"cl_commlib_receive_message() returned",cl_get_error_text(ret_val));

     if (message != NULL) {
          CL_LOG_STR(CL_LOG_INFO,"received message from",sender->comp_host);

/*        printf("received message from \"%s\"\n", sender->comp_host); */



        if (strstr((char*)message->message,"exit") != NULL) {
           printf("received \"exit\" message from host %s, component %s, id %ld\n",
                  sender->comp_host,sender->comp_name,sender->comp_id );
           cl_commlib_close_connection(handle, sender->comp_host,sender->comp_name,sender->comp_id );
           
        } else {
           int ret_val = CL_RETVAL_OK;
           ret_val = cl_commlib_send_message(handle, 
                                sender->comp_host, 
                                sender->comp_name, 
                                sender->comp_id, CL_MIH_MAT_NAK,  
                                message->message, 
                                message->message_length, 
                                &mid, message->message_id,0, 
                                0,0);
           if (ret_val != CL_RETVAL_OK) {
              CL_LOG_STR(CL_LOG_ERROR,"cl_commlib_send_message() returned:",cl_get_error_text(ret_val));
           }
        }

        message->message = NULL;
        cl_com_free_message(&message);
        cl_com_free_endpoint(&sender);
        message = NULL;
     } 
#if CL_DO_SLOW
     sleep(1);
#endif
  }

  printf("shutting down server ...\n");
  handle = cl_com_get_handle( "server", 1 );
  if (handle == NULL) {
     printf("could not find handle\n");
     exit(1);
  } else {
     printf("found handle\n");
  }

  while ( cl_commlib_shutdown_handle(handle, 1) == CL_RETVAL_MESSAGE_IN_BUFFER) {
     message = NULL;
     cl_commlib_receive_message(handle,NULL, NULL, 0, 0, 0, &message, &sender);

     if (message != NULL) {
        printf("ignoring message from \"%s\"\n", sender->comp_host); 
        cl_com_free_message(&message);
        cl_com_free_endpoint(&sender);
        message = NULL;
     }
  }

  printf("commlib cleanup ...\n");
  cl_com_cleanup_commlib();
  fflush(NULL);
  

  printf("main done\n");
  return 0;
}





