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


/* shutdown when test client can't connect for more than 15 min */
#define SGE_TEST_VIRTUAL_CLIENT_SHUTDOWN_TIMEOUT 15*60
#define DATA_SIZE 5000

void sighandler_client(int sig);
static int do_shutdown = 0;


void sighandler_client(int sig) {
   if (sig == SIGPIPE) {
      return;
   }
   if (sig == SIGHUP) {
      return;
   }
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
  time_t shutdown_time = 0;
  time_t last_time = 0;
  int no_output = 0;
  int reconnect = 0;
  /* counters */
  int rcv_messages = 0;
  int snd_messages = 0;

  cl_xml_ack_type_t ack_type = CL_MIH_MAT_NAK;
  cl_bool_t synchron = CL_FALSE;
  if (argc < 5) {
      printf("syntax: debug_level vmaster_port vmaster_host reconnect [no_output]\n");
      exit(1);
  }

  if (argc >= 6) {
     if (strcmp(argv[5],"no_output") == 0) {
        no_output = 1;
        printf("virtual gdi client: no_output option set\n");
     }
  }

  reconnect = atoi(argv[4]);

  if (reconnect == 1) {
   /* do all stuff synchron */
      ack_type = CL_MIH_MAT_ACK;
      synchron = CL_TRUE; 
  }

  /* setup signalhandling */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sighandler_client;  /* one handler for all signals */
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);

  gettimeofday(&now, NULL);
  shutdown_time = now.tv_sec + SGE_TEST_VIRTUAL_CLIENT_SHUTDOWN_TIMEOUT;

  printf("virtual gdi client is connecting to the virtual qmaster for each request.\n"); 


  /*
   * Setup commlib
   */
  cl_com_setup_commlib(CL_NO_THREAD , (cl_log_t)atoi(argv[1]), NULL);

  /*
   * Main loop
   */
  while (do_shutdown == 0) {
     cl_com_handle_t* handle = NULL; 

     /*
      * Check test timeout
      */
     gettimeofday(&now, NULL);
     if (now.tv_sec > shutdown_time ) {
        printf("shutting down test - timeout (1)\n");
        do_shutdown = 1;
        break;
     }

     /*
      * Setup create connection handle
      */
     handle = cl_com_create_handle(NULL, CL_CT_TCP, CL_CM_CT_MESSAGE, CL_FALSE, atoi(argv[2]), CL_TCP_DEFAULT, "virtual_gdi_client", 0, 1, 0);
     if (handle == NULL) {
        printf("could not get handle\n");
        exit(1);
     }

     /*
      * Inner loop
      */
     while (do_shutdown == 0) {
        int                retval  = 0;
        cl_com_message_t*  message = NULL;
        cl_com_endpoint_t* sender  = NULL;
        char *snd_data = NULL;


        /*
         * Check max run timeout
         */
        gettimeofday(&now,NULL);
        if (now.tv_sec > shutdown_time ) {
           printf("shutting down test - timeout (2)\n");
           do_shutdown = 1;
           break;
        }

        /*
         * malloc data and write snd_message number into
         */
        snd_data = malloc(DATA_SIZE);
        if (snd_data == NULL) {
           printf("malloc() error! Cannot malloc %d byte for message!\n", DATA_SIZE);
           exit(1);
        }
        sprintf(snd_data, "%d\n", snd_messages);
   
        retval = cl_commlib_send_message(handle, argv[3], "virtual_master", 1,
                                         ack_type,
                                         (cl_byte_t*)snd_data, DATA_SIZE,
                                         NULL, 0, 0, CL_FALSE, synchron);
        /*
         * setting snd_data to NULL (commlib will free memory since copy_data is CL_FALSE
         */
        snd_data = NULL;

        if (retval == CL_RETVAL_OK) {
           snd_messages++;
           retval = cl_commlib_receive_message(handle, NULL, NULL, 0,  /* handle, comp_host, comp_name , comp_id, */
                                               CL_TRUE, 0,                   /* syncron, response_mid */
                                               &message, &sender);
           if (retval == CL_RETVAL_OK) {

                 /*
                  * recalculate shutdown time (test still ok)
                  */
                 gettimeofday(&now,NULL);
                 shutdown_time = now.tv_sec + SGE_TEST_VIRTUAL_CLIENT_SHUTDOWN_TIMEOUT;

                 /*
                  * check for correct message order
                  */
                 if (atoi((char*)message->message) != rcv_messages) {
                    printf("!!!! %d. message was lost, got %s\n", rcv_messages, (char*)message->message);
                    do_shutdown = 1;
                 }

                 /*
                  * print out information, incr received messages and free message
                  */
                 if (now.tv_sec != last_time && !no_output) {
                    printf("virtual gdi client message count[received |%d| / sent |%d|]...\n", rcv_messages, snd_messages);
                    last_time = now.tv_sec;
                 }
                 rcv_messages++;
                 cl_com_free_message(&message);
                 cl_com_free_endpoint(&sender);
           } else {
              /* shutdown when virtual qmaster is not running anymore */
              if (rcv_messages > 0) {
                 printf("cl_commlib_receive_message returned: %s\n", cl_get_error_text(retval));
                 do_shutdown = 1;
              } else {
                /* we are not connected, sleep one second */
                snd_messages = 0;
                cl_commlib_trigger(handle, 1);
              }
           }
        } else {
           /* shutdown when virtual qmaster is not running anymore */
           if (rcv_messages > 0) {
              printf("cl_commlib_send_message returned: %s\n", cl_get_error_text(retval));
              do_shutdown = 1;
           }
        } 
        if (reconnect == 1) {
            break;
        }
     }  /* Inner loop */

     /*
      * Shutdown commlib handle 
      */
     cl_commlib_shutdown_handle(handle, CL_FALSE);

  }  /* Main loop */
  cl_com_cleanup_commlib();
  
  printf("main done\n");
  return 0;
}

