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
#include "version.h"

#define ARGUMENT_COUNT 13
char*  cl_values[ARGUMENT_COUNT];
int    cl_short_host_name_option = 0;                                       
int    cl_show[]         = {1,1 ,1,1 ,1,1,1 ,1,1,1,1,1 ,1};
int    cl_alignment[]    = {1,0 ,1,0 ,1,1,1 ,1,1,1,1,0 ,0};
int    cl_column_width[] = {0,0 ,0,25,0,0,22,0,0,0,0,40,10};
char*  cl_description[] = {
                       "time of debug output creation",
                       "endpoint service name where debug client is connected",
                       "message direction",
                       "name of participating communication endpoint",
                       "message data format",
                       "message acknowledge type",
                       "message tag information",
                       "message id",
                       "message response id",
                       "message length",
                       "time when message was sent/received",
                       "commlib xml protocol output",
                       "additional information"
};

char* cl_names[] = {
                       "time",
                       "local",
                       "d.",
                       "remote",
                       "format",
                       "ack type",
                       "msg tag",
                       "msg id",
                       "msg rid",
                       "msg len",
                       "msg time",
                       "xml dump",
                       "info"
};



static void sighandler_ping(int sig);
#if 0
static int pipe_signal = 0;
static int hup_signal = 0;
#endif
static int do_shutdown = 0;

static void sighandler_ping(int sig) {
   if (sig == SIGPIPE) {
#if 0
      pipe_signal = 1;
#endif
      return;
   }
   if (sig == SIGHUP) {
#if 0
      hup_signal = 1;
#endif
      return;
   }
   cl_com_ignore_timeouts(CL_TRUE);
   do_shutdown = 1;
}

static void qping_set_output_option(char* option_string) {
   char* option = NULL;
   char* value1 = NULL;
   char* value2 = NULL;
   int opt = 0;
   printf("%s\n", option_string);
   option = strstr(option_string, ":");
   if (option) {
      option[0]=0;
      if (strcmp(option_string,"h") == 0) {
         opt = 1;
         option++;
         value1=option;
      }
      if (strcmp(option_string,"hn") == 0) {
         opt = 2;
         option++;
         value1=option;
      }
      if (strcmp(option_string,"w") == 0) {
         opt = 3;
         option++;
         value2=strstr(option,":");
         value2[0] = 0;
         value1=option;
         value2++;
      }
      switch(opt) {
         case 1: {
            /* h */
            int column = -1;
            if (value1) {
               column = atoi(value1);
            }
            column--;
            if (column>=0 && column <ARGUMENT_COUNT) {
               cl_show[column] = 0;
            }
            break;
         }
         case 2: {
            /* hn */
            if(value1) {
               if (strcmp(value1,"short") == 0) {
                  cl_short_host_name_option = 1;
               }
               if (strcmp(value1,"long") == 0) {
                  cl_short_host_name_option = 0;
               }
            }
            break;
         }
         case 3: {
            /* w */
            int column = -1;
            int width = -1;
            if(value1) {
               column = atoi(value1);
            }
            if(value2) {
               width = atoi(value2);
            }
            column--;
            if (column>=0 && column <ARGUMENT_COUNT) {
               if (width > 0) {
                  cl_column_width[column] = width;
               }
            }
            break;
         }
      }

   }

   
}

static void qping_convert_time(char* buffer, char* dest) {
   time_t i;
   char* help;
   char* help2;
#ifdef HAS_LOCALTIME_R
   struct tm tm_buffer;
#endif
   struct tm *tm;
   help=strtok(buffer, ".");
   help2=strtok(NULL,".");
   i = atoi(help);
#ifndef HAS_LOCALTIME_R
   tm = localtime(&i);
#else
   tm = (struct tm *)localtime_r(&i, &tm_buffer);
#endif
   sprintf(dest, "%02d:%02d:%02d.%s", tm->tm_hour, tm->tm_min, tm->tm_sec, help2);
}

static void qping_general_communication_error(int cl_err,const char* error_message) {
   if (error_message != NULL) {
      fprintf(stderr,"%s: %s\n", cl_get_error_text(cl_err), error_message);
   } else {
      fprintf(stderr,"error: %s\n", cl_get_error_text(cl_err));
   }
   if (cl_err == CL_RETVAL_ACCESS_DENIED) {
      do_shutdown = 1;
   } 
}
static void qping_printf_fill_up(FILE* fd, char* name, int length, char c, int before) {
   int n = strlen(name);
   int i;

   if (before == 0) {
      fprintf(fd,"%s",name);
   }
   for (i=0;i<(length-n);i++) {
      fprintf(fd,"%c",c);
   }
   if (before != 0) {
      fprintf(fd,"%s",name);
   }

}

static void qping_print_line(char* buffer) {
   int i=0;
   int max_name_length = 0;
   static int show_header = 1;
   char  time[512];
   char  msg_time[512];

   for (i=0;i<ARGUMENT_COUNT;i++) {
      if (max_name_length < strlen(cl_names[i])) {
         max_name_length = strlen(cl_names[i]);
      }
   }
   

   i=0;
   cl_values[i++]=strtok(buffer, "\t");
   while( (cl_values[i++]=strtok(NULL, "\t\n")));

   
   qping_convert_time(cl_values[0], time);
   cl_values[0] = msg_time;

   qping_convert_time(cl_values[10], msg_time);
   cl_values[10] = msg_time;

   if (cl_short_host_name_option != 0) {
      char* help = NULL;
      char* help2 = NULL;
      char help_buffer[1024];


      help = strstr(cl_values[1],".");
      if (help) {
         help2 = strstr(cl_values[1],"/");

         help[0]=0;
         if (help2) {
            snprintf(help_buffer,1024,"%s%s", cl_values[1], help2);
            snprintf(cl_values[1],1024, help_buffer);
         }
      }

      help = strstr(cl_values[3],".");
      if (help) {
         help2 = strstr(cl_values[3],"/");

         help[0]=0;
         if (help2) {
            snprintf(help_buffer,1024,"%s%s", cl_values[3], help2);
            snprintf(cl_values[3],1024, help_buffer);
         }
      }

   }

   for(i=0;i<ARGUMENT_COUNT;i++) {
      if (cl_column_width[i] < strlen(cl_values[i])) {
         cl_column_width[i] = strlen(cl_values[i]);
      }
      if (cl_column_width[i] < strlen(cl_names[i])) {
         cl_column_width[i] = strlen(cl_names[i]);
      }
   }

   if (show_header == 1) {
      for (i=0;i<ARGUMENT_COUNT;i++) {
         if (cl_show[i]) {
            qping_printf_fill_up(stdout, cl_names[i],cl_column_width[i],' ',cl_alignment[i]);
            printf("|");
         }
      }
      printf("\n");
      for (i=0;i<ARGUMENT_COUNT;i++) {
         if (cl_show[i]) {
            qping_printf_fill_up(stdout, "",cl_column_width[i],'-',cl_alignment[i]);
            printf("|");
         }
      }
      printf("\n");

      show_header = 0;
   }
   for (i=0;i<ARGUMENT_COUNT;i++) {
      if (cl_show[i]) {
         qping_printf_fill_up(stdout, cl_values[i],cl_column_width[i],' ',cl_alignment[i]);
         printf("|");
      }
   }
   printf("\n");

}


static void usage(void)
{
  int max_name_length = 0;
  int i=0;
  fprintf(stderr, "%s %s\n", GE_SHORTNAME, GDI_VERSION);
  fprintf(stderr, "%s qping [-help] [ [-i <interval>] [-info] [-f] [-noalias] ] | [-dump] <host> <port> <name> <id>\n",MSG_UTILBIN_USAGE);
  fprintf(stderr, "   -i       : set ping interval time\n");
  fprintf(stderr, "   -info    : show full status information and exit\n");
  fprintf(stderr, "   -f       : show full status information on each ping interval\n");
  fprintf(stderr, "   -noalias : ignore $SGE_ROOT/SGE_CELL/common/host_aliases file\n");
  fprintf(stderr, "   -dump    : dump communication traffic (see \"communication traffic output options\" for additional information)\n");
  fprintf(stderr, "   -help    : show this info\n");
  fprintf(stderr, "   host     : host name of running component\n");
  fprintf(stderr, "   port     : port number of running component\n");
  fprintf(stderr, "   name     : name of running component (e.g.: \"qmaster\" or \"execd\")\n");
  fprintf(stderr, "   id       : id of running component (e.g.: 1 for daemons)\n\n");
  fprintf(stderr, "example:\n");
  fprintf(stderr, "   qping -info clustermaster 5000 qmaster 1\n\n");
  fprintf(stderr, "communication traffic output options:\n");
  fprintf(stderr, "   The environment variable SGE_QPING_OUTPUT_FORMAT can be used to hide columns and\n");
  fprintf(stderr, "   to set default column width. For hostname output the parameter hn is used.\n");
  fprintf(stderr, "   SGE_QPING_OUTPUT_FORMAT=\"h:1 h:4 w:1:20\"\n");
  fprintf(stderr, "   will hide the columns 1 and 4 and set the width of column 1 to 20 characters.\n");
  fprintf(stderr, "       h:X   -> hide column X\n");
  fprintf(stderr, "       w:X:Y -> set width of column X to Y\n");
  fprintf(stderr, "       hn:X  -> set hostname output parameter X. X values are \"long\" or \"short\"\n\n");
  fprintf(stderr, "   available columns are:\n\n");

  for (i=0;i<ARGUMENT_COUNT;i++) {
      if (max_name_length < strlen(cl_names[i])) {
         max_name_length = strlen(cl_names[i]);
      }
  }

  for(i=0;i<ARGUMENT_COUNT;i++) {
     fprintf(stderr, "   %02d ", i + 1);
     qping_printf_fill_up(stderr, cl_names[i], max_name_length, ' ', 0);
     fprintf(stderr, " %s\n",cl_description[i] );
  }
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
   int   option_f          = 0;
   int   option_info       = 0;
   int   option_noalias    = 0;
   int   option_dump      = 0;
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
         if (strcmp( argv[i] , "-dump") == 0) {
             option_dump = 1;
             parameter_count++;
             parameter_start++;
         }
         if (strcmp( argv[i] , "-help") == 0) {
             usage();
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

   retval = cl_com_set_error_func(qping_general_communication_error);
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr,"%s\n",cl_get_error_text(retval));
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

   if (option_dump != 0) {
      handle=cl_com_create_handle(&commlib_error, CL_CT_TCP,CL_CM_CT_STREAM  , CL_FALSE, comp_port, CL_TCP_RESERVED_PORT, "debug_client", 0, 1,0 );
#if 0
      printf("local hostname is \"%s\"\n", handle->local->comp_host);
      printf("local component is \"%s\"\n", handle->local->comp_name);
      printf("local component id is \"%lu\"\n", handle->local->comp_id);
#endif
   } else {
      handle=cl_com_create_handle(&commlib_error, CL_CT_TCP,CL_CM_CT_MESSAGE , CL_FALSE, comp_port, CL_TCP_DEFAULT,"qping", 0, 1,0 );
   }

   if (handle == NULL) {
      fprintf(stderr, "could not create communication handle: %s\n", cl_get_error_text(commlib_error));
      cl_com_cleanup_commlib();
      exit(1);
   }

   if (option_dump == 0) {
      /* enable auto close of application */
      cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED );
   }


   retval = cl_com_cached_gethostbyname(comp_host, &resolved_comp_host,NULL, NULL, NULL);
   if (retval != CL_RETVAL_OK) {
      fprintf(stderr, "could not resolve hostname %s\n", comp_host);
      cl_com_cleanup_commlib();
      exit(1);
   }

   if (option_dump == 0) {
      while (do_shutdown == 0 ) {
         cl_com_SIRM_t* status = NULL;
         retval = cl_commlib_get_endpoint_status(handle, resolved_comp_host , comp_name, comp_id, &status);
         if (retval != CL_RETVAL_OK) {
            printf("endpoint %s/%s/"U32CFormat" at port %d: %s\n", 
                   resolved_comp_host, comp_name, u32c(comp_id), comp_port, 
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
                  starttime = (time_t)status->starttime;
                  
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
   } else {
      char line_buffer[8192];
      int line_index=0;
      char* env_opt = NULL;
      char opt_buffer[8192];
      char* opt_str = NULL;

      env_opt = getenv("SGE_QPING_OUTPUT_FORMAT");
      if (env_opt != NULL) {
         snprintf(opt_buffer, 8192, "%s", env_opt);
         opt_str=strtok(opt_buffer, " ");
         if (opt_str) {
            qping_set_output_option(opt_str);
         }
         while( (opt_str=strtok(NULL, " "))) {
            qping_set_output_option(opt_str);
         }
      }

      /* SGE_QPING_OUTPUT_FORMAT="h:1 h:4 w:1:20" */

      while (do_shutdown == 0 ) {
         int                retval  = 0;
         cl_com_message_t*  message = NULL;
         cl_com_endpoint_t* sender  = NULL;

         cl_commlib_trigger(handle);
         retval = cl_commlib_receive_message(handle, NULL, NULL, 0,      /* handle, comp_host, comp_name , comp_id, */
                                             CL_FALSE, 0,                 /* syncron, response_mid */
                                             &message, &sender );
         
         if ( retval != CL_RETVAL_OK) {
            if ( retval == CL_RETVAL_CONNECTION_NOT_FOUND ) {
                printf("open connection to \"%s/%s/"U32CFormat"\" ... " ,resolved_comp_host , comp_name, u32c(comp_id) );
                retval = cl_commlib_open_connection(handle, resolved_comp_host , comp_name, comp_id);
                printf("%s\n", cl_get_error_text(retval));
                if (retval == CL_RETVAL_CREATE_RESERVED_PORT_SOCKET) {
                   printf("please start qping as root\n");
                   break;       
                }
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
                     qping_print_line(line_buffer);
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
   }
   retval = cl_commlib_shutdown_handle(handle,CL_FALSE);
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
