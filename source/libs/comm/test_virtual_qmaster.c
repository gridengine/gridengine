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

void sighandler_client(int sig);
static int pipe_signal = 0;
static int hup_signal = 0;
static int do_shutdown = 0;

static cl_com_handle_t* handle = NULL; 
cl_raw_list_t* thread_list = NULL;

cl_com_endpoint_t* event_client_array[10];

void *my_message_thread(void *t_conf);
void *my_event_thread(void *t_conf);

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
  cl_thread_settings_t* thread_p = NULL;
  cl_thread_settings_t* dummy_thread_p = NULL;
  struct sigaction sa;
  int i;



  if (argc != 3) {
      printf("please enter  debug level and port\n");
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





  printf("startup commlib ...\n");
  cl_com_setup_commlib(CL_ONE_THREAD ,atoi(argv[1]), NULL );

  printf("setting up service on port %d\n", atoi(argv[2]) );
  handle=cl_com_create_handle(CL_CT_TCP,CL_CM_CT_MESSAGE , 1, atoi(argv[2]) , "virtual_master", 1 , 1,0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     exit(1);
  }


  cl_com_get_service_port(handle,&i), 
  printf("server running on host \"%s\", port %d, component name is \"%s\", id is %ld\n", 
         handle->local->comp_host, 
         i, 
         handle->local->comp_name,  
         handle->local->comp_id);

  printf("create application threads ...\n");
  cl_thread_list_setup(&thread_list,"thread list");
  cl_thread_list_create_thread(thread_list, &dummy_thread_p,cl_com_get_log_list(), "message_thread", 1, my_message_thread);
  cl_thread_list_create_thread(thread_list, &dummy_thread_p,cl_com_get_log_list(), "event_thread__", 3, my_event_thread); 

  while(do_shutdown == 0) {
     printf("\nvirtual qmaster is running ...\n");
     sleep(1);
  }

  printf("shutdown threads ...\n");
  /* delete all threads */
  while ( (thread_p=cl_thread_list_get_first_thread(thread_list)) != NULL ) {
     cl_thread_list_delete_thread(thread_list, thread_p);
  }
  cl_thread_list_cleanup(&thread_list);


  printf("shutdown commlib ...\n");
  cl_com_cleanup_commlib();

  printf("main done\n");
  return 0;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "my_message_thread"
void *my_message_thread(void *t_conf) {
   int do_exit = 0;
   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );

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
      int ret_val = 0;
      cl_com_message_t*  message = NULL;
      cl_com_endpoint_t* sender  = NULL;

      cl_thread_func_testcancel(thread_config);

      printf(" \"%s\" -> running ...\n", thread_config->thread_name );
      ret_val = cl_commlib_receive_message(handle, NULL, NULL, 0,      /* handle, comp_host, comp_name , comp_id, */
                                           1, 0,                       /* syncron, response_mid */
                                           &message, &sender );
      if (ret_val == CL_RETVAL_OK) {
         printf(" \"%s\" -> received message from %s/%s/%ld: \"%s\" (%ld bytes)\n", thread_config->thread_name, 
                                                                        sender->comp_host,sender->comp_name,sender->comp_id,
                                                                        message->message, message->message_length);

         if ( strcmp((char*)message->message,"event") == 0) {
            int i,help;
            printf(" \"%s\" -> new event client\n", thread_config->thread_name);

            cl_com_free_message(&message);
            help = 0;
            for (i=0;i<10;i++) {
               if ( event_client_array[i] == NULL ) {
                  event_client_array[i] = sender;
                  help=1;
                  break;
               }
            }
            if (help != 1) {
               printf(" \"%s\" -> to much connected event clients\n", thread_config->thread_name);
               cl_com_free_endpoint(&sender);
            }
         } else {
            /* no event client, just return message to sender */
            char data[30000];
            sprintf(data,"gdi response");
            printf(" \"%s\" -> send gdi response to %s/%s/%ld\n", thread_config->thread_name, 
                           sender->comp_host, sender->comp_name, sender->comp_id);

            ret_val = cl_commlib_send_message(handle, sender->comp_host, sender->comp_name, sender->comp_id,
                                      CL_MIH_MAT_NAK,
                                      (cl_byte_t*) data , 30000,
                                      NULL, 0, 0 , 1, 0 );
            cl_com_free_message(&message);
            cl_com_free_endpoint(&sender);
         }
         
      } else { 
         if ((ret_val = cl_thread_wait_for_event(thread_config,1,0 )) != CL_RETVAL_OK) {  /* nothing to do sleep 1 sec */
            switch(ret_val) {
               case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
                  CL_LOG(CL_LOG_INFO,"condition wait timeout");
                  break;
               default:
                  CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
                  do_exit = 1;
            }
         }
      }
   }

   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "my_event_thread()"
void *my_event_thread(void *t_conf) {
   int do_exit = 0;
   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );

   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      int ret_val;
      int i,first,nr;
      static int event_nr = 0;
      

      cl_thread_func_testcancel(thread_config);

      printf(" \"%s\" -> running ...\n", thread_config->thread_name );
      
      /* this should be 60 events/second */
      for(nr=0;nr<60;nr++) {
         first = 0;
         for (i=0;i<10;i++) {
            cl_com_endpoint_t* client = event_client_array[i];
            if ( client != NULL) {
               char help[10000];
   
               if (first == 0) {
                  event_nr++;
                  first = 1;
               }
               sprintf(help,"event nr.: %d", event_nr );
               printf(" \"%s\" -> sending event to %s/%s/%ld\n", thread_config->thread_name, 
                                                                 client->comp_host, client->comp_name, client->comp_id  );
               ret_val = cl_commlib_send_message(handle, client->comp_host, client->comp_name, client->comp_id,
                                                 CL_MIH_MAT_NAK, (cl_byte_t*) help , 10000,
                                                 NULL, 0, 0 , 1, 0 );
               if ( ret_val != CL_RETVAL_OK) {
                  cl_com_free_endpoint(&(event_client_array[i]));
               }
            }
         }
      }

      if ((ret_val = cl_thread_wait_for_event(thread_config,1,0 )) != CL_RETVAL_OK) {  /* nothing to do sleep 1 sec */
         switch(ret_val) {
            case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
               CL_LOG(CL_LOG_INFO,"condition wait timeout");
               break;
            default:
               CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
               do_exit = 1;
         }
      }
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");
   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}


