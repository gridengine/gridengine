
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

unsigned long my_application_status(void) {
   return (unsigned long)1;
}

extern int main(int argc, char** argv)
{
  struct sigaction sa;


  cl_com_handle_t* handle = NULL; 
  cl_com_message_t* message = NULL;
  cl_com_endpoint_t* sender = NULL;
#if 0
  cl_com_endpoint_t* clients[10] = { NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, NULL, NULL };
#endif
  int connected_clients = 0;
  int i;
  
  if (argc != 4) {
      printf("please enter  debug level, port and nr. of max connections\n");
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

  printf("setting up service on port %d\n", atoi(argv[2]) );
  handle=cl_com_create_handle(NULL,CL_CT_TCP,CL_CM_CT_MESSAGE , 1,atoi(argv[2]) , CL_TCP_DEFAULT,"server", 1, 2, 0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     exit(-1);
  }

  cl_com_get_service_port(handle,&i), 
  printf("server running on host \"%s\", port %d, component name is \"%s\", id is %ld\n", 
         handle->local->comp_host, 
         i, 
         handle->local->comp_name,  
         handle->local->comp_id);

  cl_com_set_max_connections(handle,atoi(argv[3]));
  cl_com_get_max_connections(handle,&i);
  printf("max open connections is set to %d\n", i);

  printf("enable max connection close\n");
  cl_com_set_max_connection_close_mode(handle, CL_ON_MAX_COUNT_CLOSE_AUTOCLOSE_CLIENTS);

  connected_clients = 0;
  while(do_shutdown != 1) {
     unsigned long mid;
     int ret_val;
     struct timeval now;
     

     CL_LOG(CL_LOG_INFO,"main()");

     gettimeofday(&now,NULL);
     cl_commlib_trigger(handle);
     ret_val = cl_commlib_receive_message(handle,NULL, NULL, 0, 0, 0, &message, &sender);
     if (message != NULL ) {
        ret_val = cl_commlib_send_message(handle, 
                                sender->comp_host, 
                                sender->comp_name, 
                                sender->comp_id, CL_MIH_MAT_NAK,  
                                message->message, 
                                message->message_length, 
                                &mid, message->message_id,0, 
                                0,0);
        if (ret_val != CL_RETVAL_OK) {
/*           printf("cl_commlib_send_message() returned: %s\n",cl_get_error_text(ret_val)); */
           if (ret_val == CL_RETVAL_PROTOCOL_ERROR) { 
           }
        } else {
           message->message = NULL; /* don't delete this message, it's deleted when sent */
        }

        
#if 0

        for(nr=0;nr<connected_clients;nr++) {
           if (cl_com_compare_endpoints( clients[nr], sender ) != 0 ) {
              printf("client %s/%s/%ld allready known\n", sender->comp_host, sender->comp_name, sender->comp_id);
              is_new = 0;
           }
        }
        if (is_new != 0) {
           printf("new connection\n");
           clients[connected_clients] = cl_com_create_endpoint(sender->comp_host, sender->comp_name, sender->comp_id); 
           connected_clients++;
        }
#endif

/*        printf("received message from \"%s\": size of message: %ld\n", sender->comp_host, message->message_length); */
#if 0
        ret_val = cl_commlib_send_message(handle, 
                                sender->comp_host, 
                                sender->comp_name, 
                                sender->comp_id, CL_MIH_MAT_NAK,  
                                message->message, 
                                message->message_length, 
                                &mid, message->message_id,0, 
                                0,0);
        if (ret_val != CL_RETVAL_OK) {
           printf("cl_commlib_send_message() returned: %s\n",cl_get_error_text(ret_val));
        }
#endif

        cl_com_free_message(&message);
        cl_com_free_endpoint(&sender);
        message = NULL;
     } 
#if 0
     if ( last_time != now.tv_sec ) {
        /* send a message to all connected clients */
        int nr;
        cl_com_endpoint_t* client = NULL;
        last_time = now.tv_sec;
        for(nr=0;nr<connected_clients;nr++) {
           client=clients[nr];
           printf("\nsending to %s/%s/%ld\n",client->comp_host, client->comp_name, client->comp_id);
           ret_val = cl_commlib_send_message(handle, 
                                client->comp_host, 
                                client->comp_name, 
                                client->comp_id, CL_MIH_MAT_NAK,  
                                (cl_byte_t*)"test", 
                                5, 
                                &mid, 0,0, 
                                1,0);
           if (ret_val != CL_RETVAL_OK) {
              printf("cl_commlib_send_message() returned: %s\n",cl_get_error_text(ret_val));
           }
        }
     }

#endif
  }


  cl_com_ignore_timeouts(CL_TRUE); 
  cl_com_get_ignore_timeouts_flag();

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
        printf("ignoring message from \"%s\": size of message: %ld\n", sender->comp_host, message->message_length); 
        cl_com_free_message(&message);
        cl_com_free_endpoint(&sender);
        message = NULL;
     } else {
        break;
     }
  }

  printf("commlib cleanup ...\n");
  cl_com_cleanup_commlib();
  fflush(NULL);
  
  printf("main done\n");
  return 0;
}





