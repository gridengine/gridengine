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
#include "basis_types.h"

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


void print_line(char* buffer) {

   printf(buffer);
   
/*
   value=strtok(buffer, " \n");
   while( (value=strtok(NULL, " \n"))) {
      printf("value:%s\n", value);
   } 
*/
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "main()"
extern int main(int argc, char** argv)
{
  struct sigaction sa;
  int i;
  char line_buffer[8192];
  int line_index=0;


  if (argc != 6) {
      printf("syntax: debug_level port host comp comp_id\n");
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
  cl_com_setup_commlib(CL_NO_THREAD ,atoi(argv[1]), NULL );

  printf("setting up handle for connect port %d\n", atoi(argv[2]) );
  handle=cl_com_create_handle(NULL,CL_CT_TCP,CL_CM_CT_STREAM, CL_FALSE, atoi(argv[2]), /* CL_TCP_RESERVED_PORT */ CL_TCP_DEFAULT,"debug_client", 0, 1,0 );
  if (handle == NULL) {
     printf("could not get handle\n");
     exit(1);
  }

  printf("local hostname is \"%s\"\n", handle->local->comp_host);
  printf("local component is \"%s\"\n", handle->local->comp_name);
  printf("local component id is \"%ld\"\n", handle->local->comp_id);

  cl_com_get_connect_port(handle, &i);
  printf("connecting to port \"%d\" on host \"%s\"\n", i, argv[3]);

  while(do_shutdown == 0) {
     int                retval  = 0;
     cl_com_message_t*  message = NULL;
     cl_com_endpoint_t* sender  = NULL;

     cl_commlib_trigger(handle);
     retval = cl_commlib_receive_message(handle, NULL, NULL, 0,      /* handle, comp_host, comp_name , comp_id, */
                                         CL_FALSE, 0,                 /* syncron, response_mid */
                                         &message, &sender );
     if ( retval != CL_RETVAL_OK) {
        if ( retval == CL_RETVAL_CONNECTION_NOT_FOUND ) {
            printf("open connection to \"%s/%s/%d\" ...\n", argv[3], argv[4] , atoi(argv[5]));
            retval = cl_commlib_open_connection(handle, argv[3], argv[4] , atoi(argv[5]));
        }
     } else {
        int i;
        for (i=0; i < message->message_length; i++) {
           line_buffer[line_index] = message->message[i];
           
           switch(line_buffer[line_index]) {
              case 0:
                 /* ignore string end information */
                 break;
              case '\n': {
                 line_index++;
                 line_buffer[line_index] = 0;
                 print_line(line_buffer);
                 line_index = 0;
                 break;
              }
              default:
                 line_index++;
           }
        }
        fflush(stdout);
        cl_com_free_message(&message);
        cl_com_free_endpoint(&sender);
     }
  }

  printf("shutdown commlib ...\n");
  cl_com_cleanup_commlib();

  printf("main done\n");
  return 0;
}

