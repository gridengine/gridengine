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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "msg_utilbin.h"
#include "sge_time.h"
#include "cl_commlib.h"
#include "sge_arch.h"

static void sighandler_ping(int sig);
static int pipe_signal = 0;
static int hup_signal = 0;
static int do_shutdown = 0;

static void sighandler_ping(int sig) {
   if (sig == SIGPIPE) {
      pipe_signal = 1;
      return;
   }
   if (sig == SIGHUP) {
      hup_signal = 1;
      return;
   }
   do_shutdown = 1;
}

void usage(void)
{
  fprintf(stderr, "%s qping [-i <interval>] [-info] [-f] [-noalias] <host> <port> <name> <id>\n",MSG_UTILBIN_USAGE);
  fprintf(stderr, "   -i       : set ping interval time\n");
  fprintf(stderr, "   -info    : show full status information and exit\n");
  fprintf(stderr, "   -f       : show full status information on each ping interval\n");
  fprintf(stderr, "   -noalias : ignore $SGE_ROOT/SGE_CELL/common/host_aliases file\n");
  fprintf(stderr, "   host     : host name of running component\n");
  fprintf(stderr, "   port     : port number of running component\n");
  fprintf(stderr, "   name     : name of running component (e.g.: \"qmaster\" or \"execd\")\n");
  fprintf(stderr, "   id       : id of running component (e.g.: 1 for daemons)\n\n");

  fprintf(stderr, "example:\nqping -info clustermaster 5000 qmaster 1\n");
  exit(1);
}


int main(int argc, char *argv[]) {
   char* comp_host          = NULL;
   char* resolved_comp_host = NULL;
   char* comp_name          = NULL;
   cl_com_handle_t* handle  = NULL;

   int   parameter_start   = 1;
   int   comp_id           = -1;
   int   comp_port         = -1;
   int   interval          = 1;
   int   i                 = 0;
   int   exit_value        = 0;
   int   retval            = CL_RETVAL_OK;
   struct sigaction sa;
   int   option_i          = 0;
   int   option_f          = 0;
   int   option_info       = 0;
   int   option_noalias    = 0;
   int   parameter_count   = 4;
   int   commlib_error = CL_RETVAL_OK;

   
   /* setup signalhandling */
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = sighandler_ping;  /* one handler for all signals */
   sigemptyset(&sa.sa_mask);
   sigaction(SIGINT, &sa, NULL);
   sigaction(SIGTERM, &sa, NULL);
   sigaction(SIGHUP, &sa, NULL);
   sigaction(SIGPIPE, &sa, NULL);


   for (i=1;i<argc;i++) {
      if (argv[i][0] == '-') {
         if (strcmp( argv[i] , "-i") == 0) {
             option_i = 1;
             parameter_count = parameter_count + 2;
             parameter_start = parameter_start + 2;
             i++;
             if ( argv[i] != NULL) {
                interval = atoi(argv[i]);
                if (interval < 1) {
                   fprintf(stderr, "interval parameter must be larger than 0\n");
                   exit(1);
                }
             } else {
                fprintf(stderr, "no interval parameter value\n");
                exit(1);
             }
         }
         if (strcmp( argv[i] , "-info") == 0) {
             option_info = 1;
             parameter_count++;
             parameter_start++;
         }
         if (strcmp( argv[i] , "-f") == 0) {
             option_f = 1;
             parameter_count++;
             parameter_start++;
         }
         if (strcmp( argv[i] , "-noalias") == 0) {
             option_noalias = 1;
             parameter_count++;
             parameter_start++;
         }

      } else {
         break;
      }
   }

   if (argc != parameter_count + 1 ) {
      usage();
   }

   comp_host = argv[parameter_start];
   if (argv[parameter_start + 1] != NULL) {
      comp_port = atoi(argv[parameter_start + 1]);
   }
   comp_name = argv[parameter_start + 2];
   if (argv[parameter_start + 3] != NULL) {
      comp_id   = atoi(argv[parameter_start + 3]);
   }

   if ( comp_host == NULL  ) {
      fprintf(stderr,"please enter a host name\n");
      exit(1);
   }

   if ( comp_name == NULL  ) {
      fprintf(stderr,"please enter a component name\n");
      exit(1);
   }
   if ( comp_port < 0  ) {
      fprintf(stderr,"please enter a correct port number\n");
      exit(1);
   }
   if ( comp_id <= 0 ) {
      fprintf(stderr,"please enter a component id larger than 0\n");
      exit(1);
   }

   

   retval = cl_com_setup_commlib(CL_NO_THREAD ,CL_LOG_OFF, NULL );
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr,"%s\n",cl_get_error_text(retval));
      exit(1);
   }

   /* set alias file */
   if ( !option_noalias ) {
      char *alias_path = sge_get_alias_path();
      if (alias_path != NULL) {
         retval = cl_com_set_alias_file(alias_path);
         if (retval != CL_RETVAL_OK) {
            fprintf(stderr,"%s\n",cl_get_error_text(retval));
         }
         FREE(alias_path);
      }
   }

   handle=cl_com_create_handle(&commlib_error, CL_CT_TCP,CL_CM_CT_MESSAGE , 0, comp_port, "qping", 0, 1,0 );
   if (handle == NULL) {
      fprintf(stderr, "could not create communication handle: %s\n", cl_get_error_text(commlib_error));
      cl_com_cleanup_commlib();
      exit(1);
   }

   /* enable auto close of ping application */
   cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED );


   retval = cl_com_cached_gethostbyname(comp_host, &resolved_comp_host,NULL, NULL, NULL);
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr, "could not resolve hostname %s\n", comp_host);
      cl_com_cleanup_commlib();
      exit(1);
   }


   while (do_shutdown == 0 ) {
      cl_com_SIRM_t* status = NULL;
      retval = cl_commlib_get_endpoint_status(handle, resolved_comp_host , comp_name, comp_id, &status);
      if (retval != CL_RETVAL_OK) {
         printf("endpoint %s/%s/%d at port %d: %s\n", 
                resolved_comp_host, comp_name, comp_id, comp_port, 
                cl_get_error_text(retval) );  
         exit_value = 1;
      } else {
         if (status != NULL) {
            char buffer[512];
            dstring ds;
            sge_dstring_init(&ds, buffer, sizeof(buffer));

            printf("%s", sge_ctime(0, &ds));

            if (option_info == 0 && option_f == 0) {
               printf(" endpoint %s/%s/%d at port %d is up since %ld seconds\n", 
                      resolved_comp_host, comp_name, comp_id, comp_port,
                      status->runtime);  
            } else {
               time_t starttime;
               starttime = status->starttime;
               
               printf(":\nSIRM version:             %s\n",           status->version );
               printf("SIRM message id:          "U32CFormat"\n", u32c(status->mid) );
               printf("start time:               %s ("U32CFormat")\n", sge_ctime(starttime, &ds),u32c(status->starttime));
               printf("run time [s]:             "U32CFormat"\n", u32c(status->runtime) );
               printf("messages in read buffer:  "U32CFormat"\n", u32c(status->application_messages_brm) );
               printf("messages in write buffer: "U32CFormat"\n", u32c(status->application_messages_bwm) );
               printf("nr. of connected clients: "U32CFormat"\n", u32c(status->application_connections_noc) );
               printf("status:                   "U32CFormat"\n", u32c(status->application_status) );
               printf("info:                     %s\n",           status->info );
               printf("\n");
            }
         } else {
            printf("unexpected error\n");
         }
      }

      cl_com_free_sirm_message(&status);

      if (option_info != 0) {
         break;
      }
      sleep(interval);
   }
   retval = cl_commlib_shutdown_handle(handle,0);
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr,"%s\n",cl_get_error_text(retval));
      free(resolved_comp_host);
      resolved_comp_host = NULL;
      cl_com_cleanup_commlib();
      exit(1);
   }

   retval = cl_com_cleanup_commlib();
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr,"%s\n",cl_get_error_text(retval));
      free(resolved_comp_host);
      resolved_comp_host = NULL;
      exit(1);
   }
   free(resolved_comp_host);
   resolved_comp_host = NULL;
   
   return exit_value;  
}
