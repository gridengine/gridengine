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
#include <ctype.h>
#include "msg_utilbin.h"
#include "sge_time.h"
#include "cl_commlib.h"
#include "cl_util.h"
#include "sge_arch.h"
#include "version.h"
#include "sge_gdi.h"
#include "sge_mt_init.h"
#include "sge_any_request.h"
#include "sge_gdi_request.h"
#include "sge_ack.h"
#include "sge_profiling.h"
#include "sge_uidgid.h"
#include "setup_path.h"
#include "sge_bootstrap.h"
#include "sge_feature.h"
#include "sec_lib.h"

#define ARGUMENT_COUNT 13
static char*  cl_values[ARGUMENT_COUNT];
static int    cl_short_host_name_option = 0;                                       
static int    cl_show[]         = {1,1 ,1,1 ,1,1,1 ,1,1,1,1,0 ,0};
static int    cl_alignment[]    = {1,0 ,1,0 ,1,1,1 ,1,1,1,1,0 ,0};
static int    cl_column_width[] = {0,0 ,0,30,0,0,22,0,0,0,0,20,5};
static char*  cl_description[] = {
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

static char* cl_names[] = {
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
      if (strcmp(option_string,"s") == 0) {
         opt = 4;
         option++;
         value1=option;
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
         case 4: {
            /* s */
            int column = -1;
            if (value1) {
               column = atoi(value1);
            }
            column--;
            if (column>=0 && column <ARGUMENT_COUNT) {
               cl_show[column] = 1;
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

static void qping_parse_environment(void) {
   char* env_opt = NULL;
   char opt_buffer[1024];
   char* opt_str = NULL;

   env_opt = getenv("SGE_QPING_OUTPUT_FORMAT");
   if (env_opt != NULL) {
      snprintf(opt_buffer, 1024, "%s", env_opt);
      opt_str=strtok(opt_buffer, " ");
      if (opt_str) {
         qping_set_output_option(opt_str);
      }
      while( (opt_str=strtok(NULL, " "))) {
         qping_set_output_option(opt_str);
      }
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

static void qping_print_line(char* buffer, int nonewline) {
   int i=0;
   int max_name_length = 0;
   int full_width = 0;
   static int show_header = 1;
   char* message_debug_data = NULL;
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

   if (nonewline != 0) {
      message_debug_data = cl_values[11];
      if ( strstr( cl_values[4] , "bin") != NULL ) {
         cl_values[11] = "binary message data";
      } else {
         cl_values[11] = "xml message data";
      }
   }

   for(i=0;i<ARGUMENT_COUNT;i++) {
      if (cl_column_width[i] < strlen(cl_values[i])) {
         cl_column_width[i] = strlen(cl_values[i]);
      }
      if (cl_column_width[i] < strlen(cl_names[i])) {
         cl_column_width[i] = strlen(cl_names[i]);
      }
      if (cl_show[i]) {
         full_width += cl_column_width[i];
         full_width++;
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

   if (nonewline != 0 && cl_show[11]) {
      if ( strstr( cl_values[4] , "bin") != NULL ) {
         char* binary_buffer = NULL;
         char* bin_start = "--- BINARY block start ";
         char* bin_end   = "--- BINARY block end ";
         int counter = 0;
         unsigned long message_debug_data_length = strlen(message_debug_data);
         printf("%s", bin_start);
         for(i=strlen(bin_start);i<full_width;i++) {
            printf("-");
         }
         printf("\n");
         for (i=0;i<message_debug_data_length;i++) {
            if (counter == 0) {
               printf("   ");
            }
            printf("%c", message_debug_data[i]);
            counter++;
            if (counter % 16 == 0) {
               printf(" ");
            }
            if(counter == 64) {
               int x = i - 63;
               int hi,lo;
               int value = 0;
               int printed = 0;
               printf("   ");
               while(x<=i) {
                  hi = message_debug_data[x++];
                  lo = message_debug_data[x++];
                  value = cl_util_get_hex_value(hi) * 16 + cl_util_get_hex_value(lo);
                  if (isalnum(value)) {
                     printf("%c",value);
                  } else {
                     printf(".");
                  }
                  printed++;

                  if ( printed % 8 == 0) {
                     printf(" ");
                  }
               }
               counter = 0;
               printf("\n");
            }
            
            if((i+1) == message_debug_data_length && counter != 0 ) {
               int x = i - counter + 1;
               int hi,lo;
               int value = 0;
               int printed = 0; 
               int a;
               printf("   ");
               for(a=0;a<(64 - counter);a++) {
                  printf(" ");
                  if (a % 16 == 0) {
                     printf(" ");
                  }

               }
               while(x<=i) {
                  hi = message_debug_data[x++];
                  lo = message_debug_data[x++];
                  value = cl_util_get_hex_value(hi) * 16 + cl_util_get_hex_value(lo);
                  if (isalnum(value)) {
                     printf("%c",value);
                  } else {
                     printf(".");
                  }
                  printed++;

                  if ( printed % 8 == 0) {
                     printf(" ");
                  }
               }
               printf("\n");
            }
         }

         /* try to unpack gdi data */
         if ( strstr( cl_values[6] , "TAG_GDI_REQUEST") != NULL ) {
            unsigned long buffer_length = 0;
            if (  cl_util_get_binary_buffer(message_debug_data, &binary_buffer , &buffer_length) == CL_RETVAL_OK) {
               sge_pack_buffer buf;
               sge_gdi_request *req_head = NULL;  /* head of request linked list */
               sge_gdi_request *req = NULL;


               if ( init_packbuffer_from_buffer(&buf, (char*)binary_buffer, buffer_length , 0) == PACK_SUCCESS) {
                  if ( sge_unpack_gdi_request(&buf, &req_head ) == 0) {
                     int req_no = 0;
                     printf("      unpacked gdi request (binary buffer length %lu):\n", buffer_length );
                     for (req = req_head; req; req = req->next) {
                        req_no++;
                        printf("         request: %d\n", req_no);

                        if (req->op) {
                           printf("op     : "U32CFormat"\n", u32c(req->op));
                        } else {
                           printf("op     : %s\n", "NULL");
                        }
                        if (req->target) {
                           printf("target : "U32CFormat"\n", u32c(req->target));
                        } else {
                           printf("target : %s\n", "NULL");
                        }

                        if (req->host) {
                           printf("host   : %s\n", req->host);
                        } else {
                           printf("host   : %s\n", "NULL");
                        }
                        if (req->commproc) {
                           printf("commproc   : %s\n", req->commproc);
                        } else {
                           printf("commproc   : %s\n", "NULL");
                        }
                        if (req->id) {
                           printf("id   : "U32CFormat"\n", u32c(req->id));
                        } else {
                           printf("id   : %s\n", "NULL");
                        } 
                        if (req->version) {
                           printf("version   : "U32CFormat"\n", u32c(req->version));
                        } else {
                           printf("version   : %s\n", "NULL");
                        }
                        if (req->lp) {
                           lWriteListTo(req->lp,stdout); 
                        } else {
                           printf("lp   : %s\n", "NULL");

                        }
                        if (req->alp) {
                           lWriteListTo(req->alp,stdout); 
                        } else {
                           printf("alp   : %s\n", "NULL");
                        }

                        if (req->cp) {
                           lWriteWhereTo(req->cp,stdout); 
                        } else {
                           printf("cp   : %s\n", "NULL");
                        }
                        if (req->enp) {
                           lWriteWhatTo(req->enp,stdout); 
                        } else {
                           printf("enp   : %s\n", "NULL");
                        }
                        if (req->auth_info) {
                           printf("auth_info   : %s\n", req->auth_info);
                        } else {
                           printf("auth_info   : %s\n", "NULL");
                        }
                        if (req->sequence_id) {
                           printf("sequence_id   : "U32CFormat"\n", u32c(req->sequence_id));
                        } else {
                           printf("sequence_id   : %s\n", "NULL");
                        }
                        if (req->request_id) {
                           printf("request_id   : "U32CFormat"\n", u32c(req->request_id));
                        } else {
                           printf("request_id   : %s\n", "NULL");
                        }
                     }
                  }
                  free_gdi_request(req_head);
                  clear_packbuffer(&buf);
               }
            }

         }
         if ( strstr( cl_values[6] , "TAG_REPORT_REQUEST") != NULL ) {
            unsigned long buffer_length = 0;
            if (  cl_util_get_binary_buffer(message_debug_data, &binary_buffer , &buffer_length) == CL_RETVAL_OK) {
               sge_pack_buffer buf;

               if ( init_packbuffer_from_buffer(&buf, (char*)binary_buffer, buffer_length , 0) == PACK_SUCCESS) {
                  lList *rep = NULL;

                  if (cull_unpack_list(&buf, &rep) == 0) {
                     printf("      unpacked report request (binary buffer length %lu):\n", buffer_length );
                     if (rep) {
                        lWriteListTo(rep,stdout); 
                     } else {
                        printf("rep: NULL\n");
                     }
                  }
                  lFreeList(rep);
                  rep = NULL;
                  clear_packbuffer(&buf);
               }
            }
         }

         if ( strstr( cl_values[6] , "TAG_EVENT_CLIENT_EXIT") != NULL ) {
            unsigned long buffer_length = 0;
            if (  cl_util_get_binary_buffer(message_debug_data, &binary_buffer , &buffer_length) == CL_RETVAL_OK) {
               sge_pack_buffer buf;

               if ( init_packbuffer_from_buffer(&buf, (char*)binary_buffer, buffer_length , 0) == PACK_SUCCESS) {
                  u_long32 client_id = 0;
                  if (unpackint(&buf, &client_id) == PACK_SUCCESS) {
                     printf("      unpacked event client exit (binary buffer length %lu):\n", buffer_length );
                     printf("event client "U32CFormat" exit\n", u32c(client_id));
                  }
                  clear_packbuffer(&buf);
               }
            }
         }

         if ( strstr( cl_values[6] , "TAG_ACK_REQUEST") != NULL ) {
            unsigned long buffer_length = 0;
            if (  cl_util_get_binary_buffer(message_debug_data, &binary_buffer , &buffer_length) == CL_RETVAL_OK) {
               sge_pack_buffer buf;

               if ( init_packbuffer_from_buffer(&buf, (char*)binary_buffer, buffer_length , 0) == PACK_SUCCESS) {
                  u_long32 ack_tag, ack_ulong, ack_ulong2;

                  while(unpackint(&buf,  &ack_tag ) == PACK_SUCCESS) {
                     printf("      unpacked tag ack request (binary buffer length %lu):\n", buffer_length );
                     printf("ack_tag : "U32CFormat"\n", u32c(ack_tag));
                     switch (ack_tag) {
                        case TAG_SIGJOB:
                           printf("   TAG_SIGJOB\n");
                           break;
                        case TAG_SIGQUEUE:
                           printf("   TAG_SIGQUEUE\n");
                           break;
                        case ACK_EVENT_DELIVERY:
                           printf("   ACK_EVENT_DELIVERY\n");
                           break;
                        default:
                           printf("   Unexpected tag\n");
                           break;
                     }
                     if (unpackint(&buf, &ack_ulong) == PACK_SUCCESS) {
                        printf("ack_ulong : "U32CFormat"\n", u32c(ack_ulong));
                     } else {
                        printf("ack_ulong : unpack error\n");
                     }
                     if (unpackint(&buf, &ack_ulong2) == PACK_SUCCESS) {
                        printf("ack_ulong2 : "U32CFormat"\n", u32c(ack_ulong2));
                     } else {
                        printf("ack_ulong2 : unpack error\n");
                     }
                  }
                  clear_packbuffer(&buf);
               }
            }
         }

         if ( strstr( cl_values[6] , "TAG_JOB_EXECUTION") != NULL ) {
            unsigned long buffer_length = 0;
            if (  cl_util_get_binary_buffer(message_debug_data, &binary_buffer , &buffer_length) == CL_RETVAL_OK) {
               sge_pack_buffer buf;

               if ( init_packbuffer_from_buffer(&buf, (char*)binary_buffer, buffer_length , 0) == PACK_SUCCESS) {
                  u_long32 feature_set;
                  lListElem* list_elem = NULL;
                  if (unpackint(&buf, &feature_set) == PACK_SUCCESS) {
                     printf("      unpacked tag job execution (binary buffer length %lu):\n", buffer_length );
                     printf("feature_set: "U32CFormat"\n", feature_set);
                  }
                  if (cull_unpack_elem(&buf, &list_elem, NULL) == PACK_SUCCESS) {
                     lWriteElemTo(list_elem,stdout);
                  } else {
                     printf("could not unpack list elem\n");
                  }
                  if(list_elem) {
                     lFreeElem(list_elem);
                  }
                  clear_packbuffer(&buf);
               }
            }
         }
         
         
         
#if 0
      CR: 

      qping TODO:
 
      a) Add a option do ignore hosts/components

      b) Following Tags are not unpacked
      case TAG_SLAVE_ALLOW
      case TAG_CHANGE_TICKET
      case TAG_SIGJOB
      case TAG_SIGQUEUE
      case TAG_KILL_EXECD
      case TAG_NEW_FEATURES
      case TAG_GET_NEW_CONF
      case TAG_SEC_ANNOUNCE
      ...
      are missing !!!
#endif

         printf("%s", bin_end);
         for(i=strlen(bin_end);i<full_width;i++) {
            printf("-");
         }
         printf("\n");


      } else {
         int spaces = 1;
         int new_line = -1;
         int x;
         char* xml_start = "--- XML block start ";
         char* xml_end   = "--- XML block end ";
         unsigned long message_debug_data_length = strlen(message_debug_data);
         printf("%s", xml_start);
         for(i=strlen(xml_start);i<full_width;i++) {
            printf("-");
         }
         printf("\n");

         for (i=0;i<message_debug_data_length;i++) {
            if (message_debug_data[i] == '<' && message_debug_data[i+1] != '/') {
               if (new_line != -1) {
                  printf("\n");
                  spaces++;
               }
               new_line = 1;
            }
            if (message_debug_data[i] == '<' && message_debug_data[i+1] == '/' && spaces == 1) {
               new_line = 1;
               printf("\n");
            }

            if (new_line == 1) {
               for(x=0;x<spaces;x++) {
                  printf("   ");
               }
               new_line = 0;
            }
            printf("%c", message_debug_data[i]);
            if (message_debug_data[i] == '<' && message_debug_data[i+1] == '/') {
               spaces--;
            }

         }
         
         printf("\n%s", xml_end);
         for(i=strlen(xml_end);i<full_width;i++) {
            printf("-");
         }
         printf("\n");
      }
   }


}


static void usage(void)
{
  int max_name_length = 0;
  int i=0;
  fprintf(stderr, "%s %s\n", GE_SHORTNAME, GDI_VERSION);
  fprintf(stderr, "%s qping [-help] [-noalias] [ [ [-i <interval>] [-info] [-f] ] | [ [-dump] [-nonewline] ] ] <host> <port> <name> <id>\n",MSG_UTILBIN_USAGE);
  fprintf(stderr, "   -i         : set ping interval time\n");
  fprintf(stderr, "   -info      : show full status information and exit\n");
  fprintf(stderr, "   -f         : show full status information on each ping interval\n");
  fprintf(stderr, "   -noalias   : ignore $SGE_ROOT/SGE_CELL/common/host_aliases file\n");
  fprintf(stderr, "   -dump      : dump communication traffic (see \"communication traffic output options\" for additional information)\n");
  fprintf(stderr, "   -nonewline : dump output will not have a linebreak within a message\n");
  fprintf(stderr, "   -help      : show this info\n");
  fprintf(stderr, "   host       : host name of running component\n");
  fprintf(stderr, "   port       : port number of running component\n");
  fprintf(stderr, "   name       : name of running component (e.g.: \"qmaster\" or \"execd\")\n");
  fprintf(stderr, "   id         : id of running component (e.g.: 1 for daemons)\n\n");
  fprintf(stderr, "example:\n");
  fprintf(stderr, "   qping -info clustermaster 5000 qmaster 1\n\n");
  fprintf(stderr, "communication traffic output options:\n");
  fprintf(stderr, "   The environment variable SGE_QPING_OUTPUT_FORMAT can be used to hide columns and\n");
  fprintf(stderr, "   to set default column width. For hostname output the parameter hn is used.\n");
  fprintf(stderr, "   SGE_QPING_OUTPUT_FORMAT=\"h:1 h:4 w:1:20\"\n");
  fprintf(stderr, "   will hide the columns 1 and 4 and set the width of column 1 to 20 characters.\n");
  fprintf(stderr, "       h:X   -> hide column X\n");
  fprintf(stderr, "       s:X   -> show column X\n");
  fprintf(stderr, "       w:X:Y -> set width of column X to Y\n");
  fprintf(stderr, "       hn:X  -> set hostname output parameter X. X values are \"long\" or \"short\"\n\n");
  fprintf(stderr, "   available columns are:\n\n");

  
  qping_parse_environment();

  for (i=0;i<ARGUMENT_COUNT;i++) {
      if (max_name_length < strlen(cl_names[i])) {
         max_name_length = strlen(cl_names[i]);
      }
  }

  for(i=0;i<ARGUMENT_COUNT;i++) {
     char* visible="yes";
     if (cl_show[i] == 0) {
        visible="no";
     }
     if (i==0) {
        fprintf(stderr, "   nr active ");
        qping_printf_fill_up(stderr, "name", max_name_length, ' ', 0);
        fprintf(stderr, " description\n");
        fprintf(stderr, "   == ====== ");
        qping_printf_fill_up(stderr, "====", max_name_length, ' ', 0);
        fprintf(stderr, " ===========\n");
     }
     fprintf(stderr, "   %02d %3s    ", i + 1, visible);
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
   int   option_dump       = 0;
   int   option_nonewline  = 1;
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
         if (strcmp( argv[i] , "-nonewline") == 0) {
             option_nonewline = 0;
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

   sge_prof_setup();
   
   uidgid_mt_init();
   path_mt_init();

   bootstrap_mt_init(); 
   feature_mt_init();

   gdi_mt_init();

#ifdef SECURE
   sec_mt_init();
#endif

   retval = cl_com_setup_commlib(CL_ONE_THREAD ,CL_LOG_OFF, NULL );
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
      dstring line_buffer = DSTRING_INIT;

      qping_parse_environment();

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
               sge_dstring_append_char(&line_buffer, message->message[i]);
               if (message->message[i] == '\n') {
                  qping_print_line((char*)sge_dstring_get_string(&line_buffer), option_nonewline );
                  sge_dstring_free(&line_buffer);
               }
            }
            fflush(stdout);
            cl_com_free_message(&message);
            cl_com_free_endpoint(&sender);
         }
      }
      sge_dstring_free(&line_buffer);
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
   
   sge_prof_cleanup();
   return exit_value;  
}
