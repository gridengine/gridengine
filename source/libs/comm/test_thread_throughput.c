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

/* some global things */
int test_server_port = 0;
char* test_server_host = NULL;

void sighandler_client(int sig);
static int do_shutdown = 0;

cl_raw_list_t* thread_list = NULL;

void *my_receive_thread(void *t_conf);
void *my_sender_thread(void *t_conf);

void sighandler_client(
int sig 
) {
/*   thread_signal_receiver = pthread_self(); */
   if (sig == SIGPIPE) {
      return;
   }

   if (sig == SIGHUP) {
      return;
   }
   cl_com_ignore_timeouts(CL_TRUE);
   /* shutdown all sockets */
   do_shutdown = 1;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "main()"
extern int main(void)
{
  cl_thread_settings_t* thread_p = NULL;
  cl_thread_settings_t* dummy_thread_p = NULL;
  struct sigaction sa;
  cl_com_handle_t* handle = NULL; 


  
  /* setup signalhandling */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sighandler_client;  /* one handler for all signals */
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);

  cl_com_setup_commlib(CL_RW_THREAD ,CL_LOG_OFF, NULL );
  handle=cl_com_create_handle(NULL,CL_CT_TCP,CL_CM_CT_MESSAGE , CL_TRUE, 0 , CL_TCP_DEFAULT,"server", 1, 1,0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     exit(1);
  }

  /* setup global data before starting any other thread */
  cl_com_get_service_port(handle, &test_server_port);
  printf("application running on port %d\n", test_server_port);
  
  cl_com_gethostname(&test_server_host, NULL, NULL, NULL);
  printf("application running on host %s\n", test_server_host);

  /* setup thread list */
  cl_thread_list_setup(&thread_list,"thread list");

  cl_thread_list_create_thread(thread_list, &dummy_thread_p,cl_com_get_log_list(), "receiver", 1, my_receive_thread);
  cl_thread_list_create_thread(thread_list, &dummy_thread_p,cl_com_get_log_list(), "sender", 1, my_sender_thread);

  

  /* main thread = application thread */
  while(do_shutdown == 0) {
     cl_com_message_t* message = NULL;
     cl_com_endpoint_t* sender = NULL;

     /* synchron message receive */
     cl_commlib_receive_message(handle,NULL, NULL, 0, CL_TRUE, 0, &message, &sender);
     if (message != NULL) {
        printf("server: received message: \"%s\"\n", message->message);
     }
     cl_com_free_endpoint(&sender);
     cl_com_free_message(&message);
  }

  /* delete all threads */
  while ( (thread_p=cl_thread_list_get_first_thread(thread_list)) != NULL ) {
     cl_thread_list_delete_thread(thread_list, thread_p);
  }

  cl_thread_list_cleanup(&thread_list);

  cl_commlib_shutdown_handle(handle, CL_FALSE);
  cl_com_cleanup_commlib();

  printf("main done\n");
  return 0;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "my_receive_thread"
void *my_receive_thread(void *t_conf) {
   int do_exit = 0;
   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 

   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      cl_thread_func_testcancel(thread_config);
      printf("receive thread running ...\n");
      cl_thread_wait_for_event(thread_config,1,0 );
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");
   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   return(NULL);
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "my_sender_thread()"
void *my_sender_thread(void *t_conf) {
   int do_exit = 0;
   cl_com_handle_t* handle = NULL; 

   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 

   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);



  handle=cl_com_create_handle(NULL,CL_CT_TCP,CL_CM_CT_MESSAGE , CL_FALSE, test_server_port, CL_TCP_DEFAULT,"sender", 0, 1,0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     do_exit = 1;
  }

   CL_LOG(CL_LOG_INFO, "starting main loop ...");
   /* ok, thread main */
   while (do_exit == 0) {
      char* message = "test_message";
      cl_thread_func_testcancel(thread_config);

      cl_commlib_send_message(handle, test_server_host, "server", 1, 
                              CL_MIH_MAT_NAK, (cl_byte_t*) message, strlen(message) + 1,
                              NULL, 0, 0,
                              CL_TRUE, CL_FALSE  );
      printf("sender: message sent\n");

      cl_thread_wait_for_event(thread_config,0,1000000 );
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");
   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   return(NULL);
}


