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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include <netinet/tcp.h> 
#include <netdb.h>
#ifdef LINUX
#include <rpc/types.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#if defined(AIX43) || defined(AIX42) || defined(sgi)
#include <sys/param.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include "sge_hostname.h"

#include "cl_commlib.h"
#include "cl_util.h"
#include "cl_data_types.h"
#include "cl_tcp_framework.h"
#include "cl_message_list.h"
#include "cl_host_list.h"
#include "cl_host_alias_list.h"
#include "cl_communication.h"


static int cl_com_gethostbyname(char *host, cl_com_hostent_t **hostent );
static int cl_com_gethostbyaddr(struct in_addr *addr, cl_com_hostent_t **hostent );


/****** cl_communication/cl_com_setup_tcp_connection() *************************
*  NAME
*     cl_com_setup_tcp_connection() -- setup a connection type
*
*  SYNOPSIS
*     int cl_com_setup_tcp_connection(cl_com_connection_t* connection, int 
*     server_port, int connect_port) 
*
*  FUNCTION
*     This function is used to setup the connection type. It will malloc
*     a cl_com_tcp_private_t structure and set the pointer 
*     connection->com_private to this structure.
*
*     When the connection structure is used to provide a service the server_port
*     must be specified. If the connection is used to be a client to a service
*     the connect_port must be specified.
*
*     The memory obtained by the malloc() call for the cl_com_tcp_private_t structure 
*     is released by a call to cl_com_close_connection()
*
*  INPUTS
*     cl_com_connection_t* connection - empty connection structure
*     int server_port                 - port to provide a tcp service 
*     int connect_port                - port to connect to
*     int data_flow_type              - CL_COM_STREAM or CL_COM_MESSAGE
*
*  RESULT
*     int - CL_COMM_XXXX error value or CL_RETVAL_OK for no errors
*
*  SEE ALSO
*
*     cl_communication/cl_com_close_connection()
*
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_setup_tcp_connection()"
int cl_com_setup_tcp_connection(cl_com_connection_t** connection, int server_port, int connect_port, int data_flow_type) {  /* CR check */
   cl_com_tcp_private_t* com_private = NULL;
   int ret_val;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*connection != NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (data_flow_type != CL_CM_CT_STREAM && data_flow_type != CL_CM_CT_MESSAGE) {
      return CL_RETVAL_PARAMS;
   }

   *connection = (cl_com_connection_t*) malloc(sizeof(cl_com_connection_t));
   if (*connection == NULL) {
      return CL_RETVAL_MALLOC;
   }
   com_private = (cl_com_tcp_private_t*) malloc(sizeof(cl_com_tcp_private_t));
   if (com_private == NULL) {
      free(*connection);
      *connection = NULL;
      return CL_RETVAL_MALLOC;
   }
   memset(com_private, 0, sizeof(cl_com_tcp_private_t));

   (*connection)->com_private = com_private;
   (*connection)->ccm_received = 0;
   (*connection)->ccm_sent = 0;
   (*connection)->ccrm_received = 0;
   (*connection)->ccrm_sent = 0;
   (*connection)->data_buffer_size  = CL_DEFINE_DATA_BUFFER_SIZE;

   (*connection)->data_read_buffer  = (cl_byte_t*) malloc (sizeof(cl_byte_t) * ((*connection)->data_buffer_size) );
   (*connection)->data_write_buffer = (cl_byte_t*) malloc (sizeof(cl_byte_t) * ((*connection)->data_buffer_size) );
   (*connection)->read_gmsh_header = (cl_com_GMSH_t*) malloc (sizeof(cl_com_GMSH_t));

   if ( (*connection)->data_read_buffer  == NULL || 
        (*connection)->data_write_buffer == NULL || 
        (*connection)->read_gmsh_header  == NULL  ) {
      free(com_private);
      com_private = NULL;
      free( ((*connection)->data_read_buffer) );
      free( ((*connection)->data_write_buffer) );
      free( ((*connection)->read_gmsh_header) );
      free(*connection);
      *connection = NULL;
      return CL_RETVAL_MALLOC;
   }

   (*connection)->read_buffer_timeout_time = 0;
   (*connection)->write_buffer_timeout_time = 0;
   (*connection)->read_gmsh_header->dl = 0;
   (*connection)->data_write_buffer_pos = 0;
   (*connection)->data_write_buffer_processed = 0;
   (*connection)->data_write_buffer_to_send = 0;
   (*connection)->data_read_buffer_pos = 0;
   (*connection)->data_read_buffer_processed = 0;

   (*connection)->handler = NULL;
   (*connection)->last_send_message_id = 1;
   (*connection)->received_message_list = NULL;
   (*connection)->send_message_list = NULL;
   (*connection)->shutdown_timeout = 0;

   gettimeofday(&((*connection)->last_transfer_time),NULL);

   


   if ( (ret_val=cl_message_list_setup(&((*connection)->received_message_list), "rcv messages")) != CL_RETVAL_OK ) {
      free( ((*connection)->data_read_buffer) );
      free( ((*connection)->data_write_buffer) );
      free( ((*connection)->read_gmsh_header) );
      free(com_private);
      cl_message_list_cleanup(&((*connection)->received_message_list));
      free(*connection);
      *connection = NULL;
      return ret_val;
   }

   if ( (ret_val=cl_message_list_setup(&((*connection)->send_message_list), "snd messages")) != CL_RETVAL_OK ) {
      free( ((*connection)->data_read_buffer) );
      free( ((*connection)->data_write_buffer) );
      free( ((*connection)->read_gmsh_header) );
      free(com_private);
      cl_message_list_cleanup(&((*connection)->received_message_list));
      cl_message_list_cleanup(&((*connection)->send_message_list));
      free(*connection);
      *connection = NULL;
      return ret_val;
   }

   (*connection)->statistic = (cl_com_con_statistic_t*) malloc(sizeof(cl_com_con_statistic_t));
   if ((*connection)->statistic == NULL) {
      free( ((*connection)->data_read_buffer) );
      free( ((*connection)->data_write_buffer) );
      free( ((*connection)->read_gmsh_header) );
      free(com_private);
      cl_message_list_cleanup(&((*connection)->received_message_list));
      cl_message_list_cleanup(&((*connection)->send_message_list));
      free(*connection);
      *connection = NULL;
      return CL_RETVAL_MALLOC;
   }
   memset((*connection)->statistic, 0, sizeof(cl_com_con_statistic_t));
   gettimeofday(&((*connection)->statistic->last_update),NULL);


   /* reset connection struct */
   (*connection)->receiver = NULL;
   (*connection)->sender   = NULL;
   (*connection)->local    = NULL;
   (*connection)->remote   = NULL;

   (*connection)->service_handler_flag = CL_COM_SERVICE_UNDEFINED;
   com_private->sockfd = -1;
   (*connection)->framework_type = CL_CT_TCP;
   (*connection)->connection_type = CL_COM_SEND_RECEIVE;
   (*connection)->data_write_flag = CL_COM_DATA_NOT_READY;
   (*connection)->data_read_flag = CL_COM_DATA_NOT_READY;
   (*connection)->fd_ready_for_write = CL_COM_DATA_NOT_READY;
   (*connection)->connection_state = CL_COM_DISCONNECTED;
   (*connection)->data_flow_type = data_flow_type;
   (*connection)->data_format_type = CL_CM_DF_BIN;
   (*connection)->was_accepted = 0;
   (*connection)->was_opened = 0;
   (*connection)->client_host_name = NULL;

   /* setup tcp private struct */
   com_private->server_port = server_port;
   com_private->connect_port = connect_port;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_gmsh_header()"
int cl_com_free_gmsh_header(cl_com_GMSH_t** header) {   /* CR check */
   if (header == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*header == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free(*header);
   *header = NULL;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_cm_message()"
int cl_com_free_cm_message(cl_com_CM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   cl_com_free_endpoint( &( (*message)->src )  );
   cl_com_free_endpoint( &( (*message)->dst )  );
   cl_com_free_endpoint( &( (*message)->rdata )  );

   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_mih_message(cl_com_MIH_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_am_message(cl_com_AM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_sim_message(cl_com_SIM_t** message) {
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_sirm_message(cl_com_SIRM_t** message) {
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free( (*message)->info    );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}



int cl_com_free_ccm_message(cl_com_CCM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_free_ccrm_message(cl_com_CCRM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_crm_message()"
int cl_com_free_crm_message(cl_com_CRM_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   free( (*message)->version );
   free( (*message)->cs_text );
   free( (*message)->formats );
   cl_com_free_endpoint( &( (*message)->src )  );
   cl_com_free_endpoint( &( (*message)->dst )  );
   cl_com_free_endpoint( &( (*message)->rdata )  );
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_message()"
int cl_com_free_message(cl_com_message_t** message) {   /* CR check */
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if ((*message)->message_sirm != NULL) {
      CL_LOG(CL_LOG_WARNING,"freeing sirm in message struct");
      cl_com_free_sirm_message(&((*message)->message_sirm));
   }
   free((*message)->message);
   free(*message);
   *message = NULL;
   return CL_RETVAL_OK;
}

int cl_com_setup_message(cl_com_message_t** message, cl_com_connection_t* connection, cl_byte_t* data, unsigned long size, cl_xml_ack_type_t ack_type, unsigned long response_id, unsigned long tag) {

   int return_value = CL_RETVAL_OK;

   if (message == NULL || connection == NULL || data == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }
   
   return_value = cl_com_create_message(message);
   if (return_value != CL_RETVAL_OK) {
      return return_value;
   }
   (*message)->message_state = CL_MS_INIT_SND;
   (*message)->message_df = CL_MIH_DF_BIN;
   (*message)->message_mat = ack_type;
   (*message)->message = data;
   (*message)->message_id = connection->last_send_message_id;
   (*message)->message_tag = tag;
   (*message)->message_response_id = response_id;
   if (connection->last_send_message_id >= CL_DEFINE_MAX_MESSAGE_ID) {
      connection->last_send_message_id = 1;
   } else {
      connection->last_send_message_id = connection->last_send_message_id + 1;
   }
   (*message)->message_length = size;
   connection->data_write_flag = CL_COM_DATA_READY;
   return return_value;
}

int cl_com_create_message(cl_com_message_t** message) {
   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }
   
   *message = (cl_com_message_t*)malloc(sizeof(cl_com_message_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC; 
   }

   (*message)->message_state = CL_MS_UNDEFINED;
   (*message)->message_df = CL_MIH_DF_UNDEFINED;
   (*message)->message_mat = CL_MIH_MAT_UNDEFINED;
   (*message)->message_ack_flag = 0;
   (*message)->message_sirm = NULL;
   (*message)->message_id = 0;
   (*message)->message_response_id = 0;
   (*message)->message_length = 0;
   (*message)->message_snd_pointer = 0;
   (*message)->message_rcv_pointer = 0;
   memset(&((*message)->message_send_time), 0,sizeof (struct timeval));
   memset(&((*message)->message_receive_time), 0,sizeof (struct timeval));
   (*message)->message = NULL;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_GMSH()"
int cl_xml_parse_GMSH(unsigned char* buffer, unsigned long buffer_length, cl_com_GMSH_t* header , unsigned long *used_buffer_length) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long dl_begin = 0;
   unsigned long dl_end = 0;

   if (header == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   header->dl = 0;
   *used_buffer_length = 0;

   while(buf_pointer <= buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;
               if (strcmp(help_buf,"/gmsh") == 0) {
                  if (*used_buffer_length == 0) {
                     *used_buffer_length = buf_pointer+1;
                  }
                  buf_pointer++;
                  break;
               }
               if (strcmp(help_buf,"dl") == 0) {
                  dl_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dl") == 0) {
                  dl_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }

   if (dl_begin > 0 && dl_end > 0 && dl_end >= dl_begin) {
      help_buf_pointer = 0;
      for (i=dl_begin;i<=dl_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      header->dl = cl_util_get_ulong_value(help_buf);
   }
   return CL_RETVAL_OK;
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CM()"
int cl_xml_parse_CM(unsigned char* buffer, unsigned long buffer_length, cl_com_CM_t** message ) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long df_begin = 0;
   unsigned long df_end = 0;
   unsigned long ct_begin = 0;
   unsigned long ct_end = 0;
   unsigned long src_begin = 0;
   unsigned long src_end = 0;
   unsigned long dst_begin = 0;
   unsigned long dst_end = 0;
   unsigned long rdata_begin = 0;
   unsigned long rdata_end = 0;


  
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CM_t*)malloc(sizeof(cl_com_CM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CM_t));

   (*message)->df = CL_CM_DF_UNDEFINED;
   (*message)->ct = CL_CM_CT_UNDEFINED;

   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;
               if (strcmp(help_buf,"/cm") == 0) {
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"df") == 0) {
                  df_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"ct") == 0) {
                  ct_begin = buf_pointer + 1;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/df") == 0) {
                  df_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/ct") == 0) {
                  ct_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/src") == 0) {
                  src_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dst") == 0) {
                  dst_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rdata") == 0) {
                  rdata_end = buf_pointer - 8;
                  buf_pointer++;
                  continue;
               }
                  
               if (strncmp(help_buf,"src", 3) == 0 && src_begin == 0) {
                  src_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"dst", 3) == 0 && dst_begin == 0) {
                  dst_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"rdata", 5) == 0 && rdata_begin == 0) {
                  rdata_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }

   if (df_begin > 0 && df_end > 0 && df_end >= df_begin) {
      help_buf_pointer = 0;
      for (i=df_begin;i<=df_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp("bin",help_buf) == 0) {
         (*message)->df = CL_CM_DF_BIN;
      }
      if (strcmp("xml",help_buf) == 0) {
         (*message)->df = CL_CM_DF_XML;
      }
   }

   if (ct_begin > 0 && ct_end > 0 && ct_end >= ct_begin) {
      help_buf_pointer = 0;
      for (i=ct_begin;i<=ct_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp("stream",help_buf) == 0) {
         (*message)->ct = CL_CM_CT_STREAM;
      }
      if (strcmp("message",help_buf) == 0) {
         (*message)->ct = CL_CM_CT_MESSAGE;
      }
   }



   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   if ( (*message)->df == CL_CM_DF_UNDEFINED) {
      CL_LOG(CL_LOG_ERROR,"data formate undefined");
      return CL_RETVAL_UNKNOWN;
   }

   if ( (*message)->ct == CL_CM_CT_UNDEFINED) {
      CL_LOG(CL_LOG_ERROR,"connection type undefined");
      return CL_RETVAL_UNKNOWN;
   }


   /* get src */
   if (src_begin > 0 && src_end > 0 && src_end >= src_begin) {
      (*message)->src = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->src == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = src_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_host = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_id = cl_util_get_ulong_value(help_buf);
   } 
   /* get dst */
   if (dst_begin > 0 && dst_end > 0 && dst_end >= dst_begin) {
      (*message)->dst = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->dst == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = dst_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_id = cl_util_get_ulong_value(help_buf);

      
   }
   /* get rdata */
   if (rdata_begin > 0 && rdata_end > 0 && rdata_end >= rdata_begin) {
      (*message)->rdata = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->rdata == NULL) {
         cl_com_free_cm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = rdata_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_id = cl_util_get_ulong_value(help_buf);
   }
   
   if ( (*message)->src == NULL || (*message)->dst == NULL) {
      CL_LOG(CL_LOG_ERROR,"data formate undefined");
      return CL_RETVAL_UNKNOWN;
   }

   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CRM()"
int cl_xml_parse_CRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CRM_t** message ) {

   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long crm_end = 0;
   unsigned long cs_begin = 0;
   unsigned long cs_end = 0;

   unsigned long src_begin = 0;
   unsigned long src_end = 0;
   unsigned long dst_begin = 0;
   unsigned long dst_end = 0;
   unsigned long rdata_begin = 0;
   unsigned long rdata_end = 0;


   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CRM_t*)malloc(sizeof(cl_com_CRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CRM_t));
   
   (*message)->cs_condition = CL_CRM_CS_UNDEFINED;
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/crm") == 0) {
                  crm_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/cs") == 0) {
                  cs_end = buf_pointer - 5;
                  buf_pointer++;
                  continue;
               }
               
               if (strcmp(help_buf,"/src") == 0) {
                  src_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dst") == 0) {
                  dst_end = buf_pointer - 6;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rdata") == 0) {
                  rdata_end = buf_pointer - 8;
                  buf_pointer++;
                  continue;
               }

               if (strncmp(help_buf,"cs", 2) == 0 && cs_begin == 0) {
                  cs_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }

               if (strncmp(help_buf,"src", 3) == 0 && src_begin == 0) {
                  src_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"dst", 3) == 0 && dst_begin == 0) {
                  dst_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
               if (strncmp(help_buf,"rdata", 5) == 0 && rdata_begin == 0) {
                  rdata_begin = tag_begin;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   /* get cs_condition */
   if (cs_begin > 0 && cs_end > 0 && cs_end >= cs_begin) {
      i = cs_begin;
      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=cs_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;

      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_OK,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_CONNECTED;
      }
      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_DENIED;
      } 
      if (strcmp(CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_UNSUP_DATA_FORMAT,help_buf) == 0) {
         (*message)->cs_condition = CL_CRM_CS_UNSUPPORTED;
      }
      i++;
      help_buf_pointer = 0;
      while(i<=cs_end && help_buf_pointer < 254) {
         help_buf[help_buf_pointer++] = buffer[i];
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->cs_text = strdup(help_buf);
   }
   /* get src */
   if (src_begin > 0 && src_end > 0 && src_end >= src_begin) {
      (*message)->src = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->src == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = src_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_host = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=src_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->src->comp_id = cl_util_get_ulong_value(help_buf);
   } 
   /* get dst */
   if (dst_begin > 0 && dst_end > 0 && dst_end >= dst_begin) {
      (*message)->dst = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->dst == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = dst_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=dst_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dst->comp_id = cl_util_get_ulong_value(help_buf);

      
   }
   /* get rdata */
   if (rdata_begin > 0 && rdata_end > 0 && rdata_end >= rdata_begin) {
      (*message)->rdata = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
      if ((*message)->rdata == NULL) {
         cl_com_free_crm_message(message);
         return CL_RETVAL_MALLOC;
      }
      i = rdata_begin;

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_host = strdup(help_buf);


      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_name = strdup(help_buf);

      tag_begin = 0;
      help_buf_pointer = 0;
      while(i<=rdata_end && help_buf_pointer < 254) {
        
         if (buffer[i] == '\"') {
            if (tag_begin == 1) {
               i++;
               break;             
            }
            tag_begin = 1;
            i++;
            continue;
         }
         if (tag_begin == 1) {
            help_buf[help_buf_pointer++] = buffer[i];
         }
         i++;
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rdata->comp_id = cl_util_get_ulong_value(help_buf);
   }

   (*message)->formats = strdup("not supported");

   CL_LOG_STR(CL_LOG_INFO,"version:     ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"cs_condition:", (*message)->cs_condition);
   CL_LOG_STR(CL_LOG_INFO,"cs_text:     ", (*message)->cs_text);
   CL_LOG_STR(CL_LOG_INFO,"formats:     ", (*message)->formats);
   CL_LOG_STR(CL_LOG_INFO,"src->host:   ", (*message)->src->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"src->comp:   ", (*message)->src->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"src->id:     ", (*message)->src->comp_id);
   CL_LOG_STR(CL_LOG_INFO,"dst->host:   ", (*message)->dst->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"dst->comp:   ", (*message)->dst->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"dst->id:     ", (*message)->dst->comp_id);
   CL_LOG_STR(CL_LOG_INFO,"rdata->host: ", (*message)->rdata->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"rdata->comp: ", (*message)->rdata->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"rdata->id:   ", (*message)->rdata->comp_id);


   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_MIH()"
int cl_xml_parse_MIH(unsigned char* buffer, unsigned long buffer_length, cl_com_MIH_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long mih_end = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;
   unsigned long dl_begin = 0;
   unsigned long dl_end = 0;
   unsigned long df_begin = 0;
   unsigned long df_end = 0;
   unsigned long mat_begin = 0;
   unsigned long mat_end = 0;  
   unsigned long mtag_begin = 0;
   unsigned long mtag_end = 0;  
   unsigned long rid_begin = 0;
   unsigned long rid_end = 0;



   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_MIH_t*)malloc(sizeof(cl_com_MIH_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_MIH_t));
   
   (*message)->df  = CL_MIH_DF_UNDEFINED;
   (*message)->mat = CL_MIH_MAT_UNDEFINED;
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/mih") == 0) {
                  mih_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"dl") == 0 ) {
                  dl_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/dl") == 0) {
                  dl_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"df") == 0 ) {
                  df_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/df") == 0) {
                  df_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               } 
               if (strcmp(help_buf,"mat") == 0 ) {
                  mat_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mat") == 0) {
                  mat_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"tag") == 0 ) {
                  mtag_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/tag") == 0) {
                  mtag_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"rid") == 0 ) {
                  rid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/rid") == 0) {
                  rid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   /* get tag */
   if (mtag_begin > 0 && mtag_end > 0 && mtag_end >= mtag_begin) {
      help_buf_pointer = 0;
      for (i=mtag_begin;i<=mtag_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->tag = cl_util_get_ulong_value(help_buf);
   }

   /* get rid */
   if (rid_begin > 0 && rid_end > 0 && rid_end >= rid_begin) {
      help_buf_pointer = 0;
      for (i=rid_begin;i<=rid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->rid = cl_util_get_ulong_value(help_buf);
   }

   /* get dl */
   if (dl_begin > 0 && dl_end > 0 && dl_end >= dl_begin) {
      help_buf_pointer = 0;
      for (i=dl_begin;i<=dl_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->dl = cl_util_get_ulong_value(help_buf);
   }

   /* get df */
   if (df_begin > 0 && df_end > 0 && df_end >= df_begin) {
      help_buf_pointer = 0;
      for (i=df_begin;i<=df_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_BIN,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_BIN;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_XML,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_XML;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_AM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_AM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_SIM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_SIM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_SIRM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_SIRM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_CCM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_CCM;
      }
      if (strcmp(CL_MIH_MESSAGE_DATA_FORMAT_CCRM,help_buf) == 0) {
         (*message)->df = CL_MIH_DF_CCRM;
      }

   }
   /* get mat */
   if (mat_begin > 0 && mat_end > 0 && mat_end >= mat_begin) {
      help_buf_pointer = 0;
      for (i=mat_begin;i<=mat_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_NAK,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_NAK;
      }
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_ACK,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_ACK;
      }
      if (strcmp(CL_MIH_MESSAGE_ACK_TYPE_SYNC,help_buf) == 0) {
         (*message)->mat = CL_MIH_MAT_SYNC;
      }
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"mid:     ", (*message)->mid);
   CL_LOG_INT(CL_LOG_INFO,"dl:      ", (*message)->dl);
   CL_LOG_STR(CL_LOG_INFO,"df:      ", cl_com_get_mih_df_string((*message)->df));
   CL_LOG_STR(CL_LOG_INFO,"mat:     ", cl_com_get_mih_mat_string((*message)->mat));
   CL_LOG_INT(CL_LOG_INFO,"tag:     ", (*message)->tag);
   CL_LOG_INT(CL_LOG_INFO,"rid:     ", (*message)->rid);


   if ( (*message)->dl > CL_DEFINE_MAX_MESSAGE_LENGTH ) {
      return CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR; 
   }

   return CL_RETVAL_OK;
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_SIRM()"
int cl_xml_parse_SIRM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIRM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long sirm_end = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;

   unsigned long starttime_begin = 0;
   unsigned long starttime_end = 0;

   unsigned long runtime_begin = 0;
   unsigned long runtime_end = 0;

   unsigned long application_messages_brm_begin = 0;
   unsigned long application_messages_brm_end = 0;

   unsigned long application_messages_bwm_begin = 0;
   unsigned long application_messages_bwm_end = 0;

   unsigned long application_connections_noc_begin = 0;
   unsigned long application_connections_noc_end = 0;

   unsigned long application_status_begin = 0;
   unsigned long application_status_end = 0;


   unsigned long info_begin = 0;
   unsigned long info_end = 0;



   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_SIRM_t*)malloc(sizeof(cl_com_SIRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_SIRM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/sirm") == 0) {
                  sirm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"starttime") == 0 ) {
                  starttime_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/starttime") == 0) {
                  starttime_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }

               if (strcmp(help_buf,"runtime") == 0 ) {
                  runtime_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/runtime") == 0) {
                  runtime_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"brm") == 0 ) {
                  application_messages_brm_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/brm") == 0) {
                  application_messages_brm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"bwm") == 0 ) {
                  application_messages_bwm_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/bwm") == 0) {
                  application_messages_bwm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"noc") == 0 ) {
                  application_connections_noc_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/noc") == 0) {
                  application_connections_noc_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"status") == 0 ) {
                  application_status_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/status") == 0) {
                  application_status_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"info") == 0 ) {
                  info_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/info") == 0) {
                  info_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }

   /* get info */
   if (info_begin > 0 && info_end > 0 && info_end >= info_begin) {
      help_buf_pointer = 0;
      for (i=info_begin;i<=info_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->info = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   /* get starttime */
   if (starttime_begin > 0 && starttime_end > 0 && starttime_end >= starttime_begin) {
      help_buf_pointer = 0;
      for (i=starttime_begin;i<=starttime_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->starttime = cl_util_get_ulong_value(help_buf);
   }

   /* get runtime */
   if (runtime_begin > 0 && runtime_end > 0 && runtime_end >= runtime_begin) {
      help_buf_pointer = 0;
      for (i=runtime_begin;i<=runtime_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->runtime = cl_util_get_ulong_value(help_buf);
   }

   /* get application_messages_brm */
   if (application_messages_brm_begin > 0 && application_messages_brm_end > 0 && application_messages_brm_end >= application_messages_brm_begin) {
      help_buf_pointer = 0;
      for (i=application_messages_brm_begin;i<=application_messages_brm_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_messages_brm = cl_util_get_ulong_value(help_buf);
   }

   /* get application_messages_bwm */
   if (application_messages_bwm_begin > 0 && application_messages_bwm_end > 0 && application_messages_bwm_end >= application_messages_bwm_begin) {
      help_buf_pointer = 0;
      for (i=application_messages_bwm_begin;i<=application_messages_bwm_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_messages_bwm = cl_util_get_ulong_value(help_buf);
   }

   /* get application_connections_noc */
   if (application_connections_noc_begin > 0 && application_connections_noc_end > 0 && application_connections_noc_end >= application_connections_noc_begin) {
      help_buf_pointer = 0;
      for (i=application_connections_noc_begin;i<=application_connections_noc_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_connections_noc = cl_util_get_ulong_value(help_buf);
   }

   /* get application_connections_noc */
   if (application_status_begin > 0 && application_status_end > 0 && application_status_end >= application_status_begin) {
      help_buf_pointer = 0;
      for (i=application_status_begin;i<=application_status_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->application_status = cl_util_get_ulong_value(help_buf);
   }


   CL_LOG_STR(CL_LOG_WARNING,"version:   ", (*message)->version);
   CL_LOG_INT(CL_LOG_WARNING,"mid:       ", (*message)->mid);
   CL_LOG_INT(CL_LOG_WARNING,"starttime: ", (*message)->starttime);
   CL_LOG_INT(CL_LOG_WARNING,"runtime:   ", (*message)->runtime);
   CL_LOG_INT(CL_LOG_WARNING,"brm:       ", (*message)->application_messages_brm);
   CL_LOG_INT(CL_LOG_WARNING,"bwm:       ", (*message)->application_messages_bwm);
   CL_LOG_INT(CL_LOG_WARNING,"noc:       ", (*message)->application_connections_noc);
   CL_LOG_INT(CL_LOG_WARNING,"status:    ", (*message)->application_status);
   CL_LOG_STR(CL_LOG_WARNING,"info:      ", (*message)->info);

   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_AM()"
int cl_xml_parse_AM(unsigned char* buffer, unsigned long buffer_length, cl_com_AM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long am_end = 0;
 
   unsigned long mid_begin = 0;
   unsigned long mid_end = 0;


   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_AM_t*)malloc(sizeof(cl_com_AM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_AM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;



               if (strcmp(help_buf,"/am") == 0) {
                  am_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"mid") == 0 ) {
                  mid_begin = tag_end + 2;
                  buf_pointer++;
                  continue;
               }
               if (strcmp(help_buf,"/mid") == 0) {
                  mid_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   /* get mid */
   if (mid_begin > 0 && mid_end > 0 && mid_end >= mid_begin) {
      help_buf_pointer = 0;
      for (i=mid_begin;i<=mid_end && help_buf_pointer < 254 ;i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->mid = cl_util_get_ulong_value(help_buf);
   }

   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   CL_LOG_INT(CL_LOG_INFO,"mid:     ", (*message)->mid);

   return CL_RETVAL_OK;

}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CCM()"
int cl_xml_parse_CCM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long ccm_end = 0;
 
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CCM_t*)malloc(sizeof(cl_com_CCM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CCM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;


               if (strcmp(help_buf,"/ccm") == 0) {
                  ccm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_CCRM()"
int cl_xml_parse_CCRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCRM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long ccrm_end = 0;
 

   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_CCRM_t*)malloc(sizeof(cl_com_CCRM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_CCRM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;

               if (strcmp(help_buf,"/ccrm") == 0) {
                  ccrm_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);

   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_xml_parse_SIM()"
int cl_xml_parse_SIM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIM_t** message ) {
   unsigned long i;
   char help_buf[256];
   unsigned long help_buf_pointer = 0;
   unsigned long buf_pointer = 0;
   int in_tag = 0;
   unsigned long tag_begin = 0;
   unsigned long tag_end = 0;
   unsigned long version_begin = 0;
   unsigned long sim_end = 0;
 
   if (message == NULL || buffer == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   *message = (cl_com_SIM_t*)malloc(sizeof(cl_com_SIM_t));
   if (*message == NULL) {
      return CL_RETVAL_MALLOC;
   }
   memset((char *) (*message), 0, sizeof(cl_com_SIM_t));
   
   while(buf_pointer < buffer_length) {
      switch( buffer[buf_pointer] ) {
         case '=':
            if (in_tag == 1) {
               if (strstr( (char*)&(buffer[tag_begin]),  "version") != NULL) {
                  version_begin = buf_pointer + 2;
               }
            }
         break;   
         case '<':
            in_tag = 1;
            tag_begin = buf_pointer + 1;
         break; 
         case '>':
            in_tag = 0;
            tag_end = buf_pointer - 1;
            if (tag_begin < tag_end && tag_begin > 0 && tag_end > 0) {
               
               help_buf_pointer = 0;
               for (i=tag_begin;i<=tag_end && help_buf_pointer < 254 ;i++) {
                  help_buf[help_buf_pointer++] = buffer[i];
               }
               help_buf[help_buf_pointer] = 0;


               if (strcmp(help_buf,"/sim") == 0) {
                  sim_end = tag_begin - 2;
                  buf_pointer++;
                  continue;
               }
            }
         break;
      }
      buf_pointer++;
   }


   /* get version */
   if (version_begin > 0) {
      help_buf_pointer = 0;
      for (i=version_begin ; i<= buffer_length && buffer[i] != '\"' && help_buf_pointer < 254 ; i++) {
         help_buf[help_buf_pointer++] = buffer[i];
      }
      help_buf[help_buf_pointer] = 0;
      (*message)->version = strdup(help_buf);
   }


   CL_LOG_STR(CL_LOG_INFO,"version: ", (*message)->version);
   return CL_RETVAL_OK;
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_dump_connection()"
void  cl_dump_connection(cl_com_connection_t* connection) {   /* CR check */
   CL_LOG(CL_LOG_DEBUG,"*** cl_dump_connection() ***");
   if (connection == NULL) {
      CL_LOG(CL_LOG_DEBUG, "connection is NULL");
   } else {
      if (connection->service_handler_flag != CL_COM_SERVICE_HANDLER ) {
         cl_com_dump_endpoint(connection->receiver, "receiver");
         cl_com_dump_endpoint(connection->sender, "sender");
         cl_com_dump_endpoint(connection->local, "local");
         cl_com_dump_endpoint(connection->remote, "remote");
      } else {
         CL_LOG(CL_LOG_DEBUG,"this is local service handler:");
         cl_com_dump_endpoint(connection->local, "local");
      }

      CL_LOG_INT(CL_LOG_DEBUG,"elements in received_message_list:", cl_raw_list_get_elem_count(connection->received_message_list));
      CL_LOG_INT(CL_LOG_DEBUG,"elements in send_message_list:", cl_raw_list_get_elem_count(connection->send_message_list));
      if (connection->handler == NULL) {
         CL_LOG(CL_LOG_DEBUG,"no handler pointer is set");
      } else {
         CL_LOG(CL_LOG_DEBUG,"handler pointer is set");
      }
      
      CL_LOG_STR(CL_LOG_DEBUG,"framework_type:",cl_com_get_framework_type(connection)  );
      CL_LOG_STR(CL_LOG_DEBUG,"connection_type:", cl_com_get_connection_type(connection) );
      CL_LOG_STR(CL_LOG_DEBUG,"service_handler_flag:", cl_com_get_service_handler_flag(connection) );
      CL_LOG_STR(CL_LOG_DEBUG,"data_write_flag:", cl_com_get_data_write_flag(connection) );
      CL_LOG_STR(CL_LOG_DEBUG,"data_read_flag:", cl_com_get_data_read_flag(connection) );
      CL_LOG_STR(CL_LOG_DEBUG,"connection_state:", cl_com_get_connection_state(connection) );
      CL_LOG_STR(CL_LOG_DEBUG,"data_flow_type:", cl_com_get_data_flow_type(connection) );
   
      if (connection->com_private == NULL) {
         CL_LOG(CL_LOG_DEBUG,"com_private is not set");
      } else {
         cl_dump_private(connection);
      }
   }
   CL_LOG(CL_LOG_DEBUG,"****************************");
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_dump_private()"
void cl_dump_private(cl_com_connection_t* connection) {  /* CR check */
   if (connection != NULL) {
      switch(connection->framework_type) {
         case CL_CT_TCP:
            cl_dump_tcp_private(connection);
            break;
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_read_GMSH()"
int cl_com_read_GMSH(cl_com_connection_t* connection, unsigned long *only_one_read) {

   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return CL_RETVAL_PARAMS;
   }

   switch(connection->framework_type ) {
      case CL_CT_TCP: {
         return cl_com_tcp_read_GMSH(connection,only_one_read);
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined framework type");
      }
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_framework_type()"
const char* cl_com_get_framework_type(cl_com_connection_t* connection) {  /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->framework_type ) {
      case CL_CT_TCP: {
         return "CL_CT_TCP";
      }
      case CL_CT_GLOBUS: {
         return "CL_CT_GLOBUS";
      }
      case CL_CT_JXTA: {
         return "CL_CT_JXTA";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined framework type");
         return "unknown";
      }
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_connection_type()"
const char* cl_com_get_connection_type(cl_com_connection_t* connection) { /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->connection_type ) {
      case CL_COM_RECEIVE: {
         return "CL_COM_RECEIVE";
      }
      case CL_COM_SEND: {
         return "CL_COM_SEND";
      }
      case CL_COM_SEND_RECEIVE: {
         return "CL_COM_SEND_RECEIVE";
      }
      case CL_COM_UNDEFINED: {
         return "CL_COM_UNDEFINED";
      }
      default: {
         CL_LOG(CL_LOG_WARNING,"undefined connection type");
         return "unknown";
      }
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_service_handler_flag()"
const char* cl_com_get_service_handler_flag(cl_com_connection_t* connection) { /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->service_handler_flag ) {
      case CL_COM_SERVICE_HANDLER: {
         return "CL_COM_SERVICE_HANDLER";
      }
      case CL_COM_CONNECTION: {
         return "CL_COM_CONNECTION";
      }
      case CL_COM_SERVICE_UNDEFINED: {
         return "CL_COM_SERVICE_UNDEFINED";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined service handler flag type");
         return "unknown";
      }
   }
}
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_data_write_flag()"
const char* cl_com_get_data_write_flag(cl_com_connection_t* connection) {  /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->data_write_flag ) {
      case CL_COM_DATA_READY: {
         return "CL_COM_DATA_READY";
      }
      case CL_COM_DATA_NOT_READY: {
         return "CL_COM_DATA_NOT_READY";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined data write flag type");
         return "unknown";
      }
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_data_read_flag()"
const char* cl_com_get_data_read_flag(cl_com_connection_t* connection) {  /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->data_read_flag ) {
      case CL_COM_DATA_READY: {
         return "CL_COM_DATA_READY";
      }
      case CL_COM_DATA_NOT_READY: {
         return "CL_COM_DATA_NOT_READY";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined data read flag type");
         return "unknown";
      }
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_connection_state()"
const char* cl_com_get_connection_state(cl_com_connection_t* connection) {   /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }

   switch(connection->connection_state ) {
      case CL_COM_DISCONNECTED: {
         return "CL_COM_DISCONNECTED";
      }
      case CL_COM_CLOSING: {
         return "CL_COM_CLOSING";
      }
      case CL_COM_OPENING: {
         return "CL_COM_OPENING";
      }
      case CL_COM_CONNECTED: {
         return "CL_COM_CONNECTED";
      }
      case CL_COM_CONNECTING: {
         return "CL_COM_CONNECTING";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined marked to close flag type");
         return "unknown";
      }
   }
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_data_flow_type()"
const char* cl_com_get_data_flow_type(cl_com_connection_t* connection) {  /* CR check */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }
   switch(connection->data_flow_type ) {
      case CL_CM_CT_STREAM: {
         return "CL_COM_STREAM";
      }
      case CL_CM_CT_MESSAGE: {
         return "CL_COM_MESSAGE";
      }
      default: {
         CL_LOG(CL_LOG_ERROR,"undefined data flow flag type");
         return "unknown";
      }
   }
}

const char* cl_com_get_mih_df_string(cl_xml_mih_data_format_t df) {
   switch (df) {
      case CL_MIH_DF_BIN:
         return CL_MIH_MESSAGE_DATA_FORMAT_BIN;
      case CL_MIH_DF_XML:
         return CL_MIH_MESSAGE_DATA_FORMAT_XML;
      case CL_MIH_DF_AM:
         return CL_MIH_MESSAGE_DATA_FORMAT_AM;
      case CL_MIH_DF_SIM:
         return CL_MIH_MESSAGE_DATA_FORMAT_SIM;
      case CL_MIH_DF_SIRM:
         return CL_MIH_MESSAGE_DATA_FORMAT_SIRM;
      case CL_MIH_DF_CCM:
         return CL_MIH_MESSAGE_DATA_FORMAT_CCM;
      case CL_MIH_DF_CCRM:
         return CL_MIH_MESSAGE_DATA_FORMAT_CCRM;
      case CL_MIH_DF_UNDEFINED:
         return "undefined";
   }
   return "undefined";
}


const char* cl_com_get_mih_mat_string(cl_xml_ack_type_t mat) {
   switch (mat) {
      case CL_MIH_MAT_NAK:
         return CL_MIH_MESSAGE_ACK_TYPE_NAK;
      case CL_MIH_MAT_ACK:
         return CL_MIH_MESSAGE_ACK_TYPE_ACK;
      case CL_MIH_MAT_SYNC:
         return CL_MIH_MESSAGE_ACK_TYPE_SYNC;
      case CL_MIH_MAT_UNDEFINED:
         return "undefined";
   }
   return "undefined";
}


/****** cl_communication/cl_com_open_connection() ******************************
*  NAME
*     cl_com_open_connection() -- open a connection to a service handler
*
*  SYNOPSIS
*     int cl_com_open_connection(cl_com_connection_t* connection, const char* 
*     comp_host, const char* comp_name, int comp_id, int timeout) 
*
*  FUNCTION
*     This function is called to setup a connection to a service handler. The
*     connection object (cl_com_connection_t) must be initalized with a call
*     to cl_com_setup_xxx_connection (e.g. cl_com_setup_tcp_connection() for
*     a CL_CT_TCP connection which is using tcp/ip for transport)
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to a connection object
*     const char* comp_host           - host of service
*     const char* comp_name           - component name of service
*     int comp_id                     - component id of service
*     int timeout                     - timeout for connection establishment
*
*  RESULT
*     int - CL_COMM_XXXX error value or CL_RETVAL_OK for no errors
*
*  SEE ALSO
*     
*     cl_communication/cl_com_close_connection()
*     cl_communication/cl_com_setup_tcp_connection()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_open_connection()"
int cl_com_open_connection(cl_com_connection_t* connection, int timeout, cl_com_endpoint_t* remote_endpoint, cl_com_endpoint_t* local_endpoint, cl_com_endpoint_t* receiver_endpoint ,cl_com_endpoint_t* sender_endpoint) {     /* CR check */
   int retval = CL_RETVAL_UNKNOWN;

   /* check parameters and duplicate */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return CL_RETVAL_PARAMS;
   }
   
   if (connection->connection_state != CL_COM_DISCONNECTED &&
       connection->connection_state != CL_COM_OPENING) {
      CL_LOG(CL_LOG_ERROR,"unexpected connection state");
      return CL_RETVAL_CONNECTION_STATE_ERROR;
   }

   /* starting this function the first time */
   if (connection->connection_state == CL_COM_DISCONNECTED) {
      if (remote_endpoint   == NULL ||
          local_endpoint    == NULL ||
          receiver_endpoint == NULL ||
          sender_endpoint   == NULL) {
         CL_LOG(CL_LOG_ERROR,"endpoint pointer parameter not initialized");
         return CL_RETVAL_PARAMS;
      }


      /* check endpoint structure pointer to be free */
      if (connection->receiver != NULL || 
          connection->sender   != NULL || 
          connection->local    != NULL || 
          connection->remote   != NULL  ) {
         CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_PARAMS));
         return CL_RETVAL_PARAMS;
      }

      /* copy the endpoint structures */
      connection->remote = cl_com_create_endpoint(remote_endpoint->comp_host, remote_endpoint->comp_name, remote_endpoint->comp_id);
      connection->local = cl_com_create_endpoint(local_endpoint->comp_host, local_endpoint->comp_name, local_endpoint->comp_id);
      connection->receiver = cl_com_create_endpoint(receiver_endpoint->comp_host, receiver_endpoint->comp_name, receiver_endpoint->comp_id);
      connection->sender = cl_com_create_endpoint(sender_endpoint->comp_host, sender_endpoint->comp_name, sender_endpoint->comp_id);

      /* check new endpoints */
      if (connection->remote == NULL || connection->local == NULL || connection->receiver == NULL || connection->sender == NULL) {
         cl_com_free_endpoint(&(connection->remote));
         cl_com_free_endpoint(&(connection->local));
         cl_com_free_endpoint(&(connection->receiver));
         cl_com_free_endpoint(&(connection->sender));
         CL_LOG(CL_LOG_ERROR,"malloc() error");
         return CL_RETVAL_MALLOC;
      }

      /* check comp ids */
      if ( connection->receiver->comp_id == 0) {
         cl_com_free_endpoint(&(connection->remote));
         cl_com_free_endpoint(&(connection->local));
         cl_com_free_endpoint(&(connection->receiver));
         cl_com_free_endpoint(&(connection->sender));
         CL_LOG(CL_LOG_ERROR,"receiver endpoint id can not be 0");
         return CL_RETVAL_PARAMS;
      }
      if ( connection->remote->comp_id == 0 ) {
         cl_com_free_endpoint(&(connection->remote));
         cl_com_free_endpoint(&(connection->local));
         cl_com_free_endpoint(&(connection->receiver));
         cl_com_free_endpoint(&(connection->sender));
         CL_LOG(CL_LOG_ERROR,"remote endpoint id can not be 0");
         return CL_RETVAL_PARAMS;
      }

      /* init connection */
      connection->data_write_flag = CL_COM_DATA_NOT_READY;
      connection->data_read_flag  = CL_COM_DATA_NOT_READY;
      connection->service_handler_flag = CL_COM_CONNECTION;

      connection->connection_state = CL_COM_OPENING;   
      connection->connection_sub_state = CL_COM_OPEN_INIT;
      connection->was_opened = 1;
   }

   /* try to connect (open connection) */
   if (connection->connection_state == CL_COM_OPENING) {
      switch(connection->framework_type) {
         case CL_CT_TCP:
            connection->connection_type = CL_COM_SEND_RECEIVE;
           
            retval = cl_com_tcp_open_connection(connection,timeout,1);
            if (retval != CL_RETVAL_OK && retval != CL_RETVAL_UNCOMPLETE_WRITE) {
               CL_LOG(CL_LOG_ERROR,"connect error");
               connection->connection_type = CL_COM_UNDEFINED;
/*               cl_com_free_endpoint(&(connection->remote));
               cl_com_free_endpoint(&(connection->local));
               cl_com_free_endpoint(&(connection->receiver));
               cl_com_free_endpoint(&(connection->sender));  */
            } 
            if (retval == CL_RETVAL_OK) {  /* OK */
               CL_LOG(CL_LOG_INFO,"tcp conected");
               connection->connection_state = CL_COM_CONNECTING;
               connection->connection_sub_state = CL_COM_SEND_INIT;
               connection->data_write_flag = CL_COM_DATA_READY;
            }
            return retval;
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            retval = CL_RETVAL_UNDEFINED_FRAMEWORK;
            break;
      }
   }
   return retval;
}



/****** cl_communication/cl_com_close_connection() *****************************
*  NAME
*     cl_com_close_connection() -- cleanup a connection
*
*  SYNOPSIS
*     int cl_com_close_connection(cl_com_connection_t* connection) 
*
*  FUNCTION
*     This wrapper function will call the correct cl_com_xxx_close_connection()
*     function for the selected framework. The called function must free
*     the memory for the connection->com_private pointer.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to a cl_com_connection_t 
*                                       structure
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_setup_tcp_connection()
*
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_close_connection()"
/* connection_list must be locked */
int cl_com_close_connection(cl_com_connection_t** connection) {   /* CR check */
   int retval = CL_RETVAL_OK;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*connection != NULL) {
      cl_message_list_elem_t* elem = NULL;
      cl_message_list_elem_t* elem2 = NULL;
      cl_com_message_t* message = NULL;

      CL_LOG(CL_LOG_INFO,"closing connection");
      cl_dump_connection(*connection);

      /* rcv messages list */
      cl_raw_list_lock( (*connection)->received_message_list );     
      elem = cl_message_list_get_first_elem((*connection)->received_message_list);
      while(elem != NULL) {
         elem2 = elem;
         elem = cl_message_list_get_next_elem((*connection)->received_message_list,elem);
         message = elem2->message;
         if (message->message_state == CL_MS_READY) {
            CL_LOG(CL_LOG_ERROR,"unread message for this connection in received message list");
         } else {
            CL_LOG(CL_LOG_WARNING,"uncompled received message in received messages list");
            CL_LOG_INT(CL_LOG_WARNING,"message state:", message->message_state);
         }
         /* delete elem */
         CL_LOG(CL_LOG_ERROR,"deleting message");

         cl_raw_list_remove_elem( (*connection)->received_message_list , elem2->raw_elem );
         free(elem2);
         elem2 = NULL;
         cl_com_free_message(&message);
      }
 
      cl_raw_list_unlock( (*connection)->received_message_list );      
      cl_message_list_cleanup(&((*connection)->received_message_list));


      /* snd messages list */
      cl_raw_list_lock( (*connection)->send_message_list );     
      elem = cl_message_list_get_first_elem((*connection)->send_message_list);
      while(elem != NULL) {
         elem2 = elem;
         elem = cl_message_list_get_next_elem((*connection)->send_message_list,elem);
         message = elem2->message;
         CL_LOG(CL_LOG_ERROR,"unsent message for this connection in send message list");
         CL_LOG_INT(CL_LOG_WARNING,"message state:", message->message_state);

         /* delete elem */
         CL_LOG(CL_LOG_ERROR,"deleting message");
         cl_raw_list_remove_elem( (*connection)->send_message_list , elem2->raw_elem );
         free(elem2);
         elem2 = NULL;
         cl_com_free_message(&message);
      }
      cl_raw_list_unlock( (*connection)->send_message_list );      


      cl_message_list_cleanup(&((*connection)->send_message_list));

      cl_com_free_endpoint(&((*connection)->remote));
      cl_com_free_endpoint(&((*connection)->local));
      cl_com_free_endpoint(&((*connection)->sender));
      cl_com_free_endpoint(&((*connection)->receiver));

      free( (*connection)->data_read_buffer);
      free( (*connection)->data_write_buffer);
      free( (*connection)->read_gmsh_header);
      (*connection)->data_read_buffer = NULL;
      (*connection)->data_write_buffer = NULL;

      (*connection)->data_flow_type = -1;

      free( (*connection)->client_host_name);
      (*connection)->client_host_name = NULL;

      free( (*connection)->statistic );
      (*connection)->statistic = NULL;

      switch((*connection)->framework_type) {
         case CL_CT_TCP:
            retval = cl_com_tcp_close_connection(connection);
            free(*connection);
            *connection = NULL;
            return retval;
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}



/* free  cl_com_hostent_t structure 
  
   params: 
   cl_com_hostent_t** hostent -> address of an pointer to cl_com_hostent_t

   return:
      - *hostent is set to NULL
      - int - CL_RETVAL_XXXX error number
*/

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_hostent()"
int cl_com_free_hostent(cl_com_hostent_t **hostent_p) {  /* CR check */

   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS; 
   }
   if (*hostent_p == NULL) {
      return CL_RETVAL_PARAMS; /* no memory to free ? */
   }

   /* free hostent structure */
   if ( (*hostent_p)->he_data_buffer == NULL ) {
      /* data buffer is null, we have to free the complete hostent structure */
      sge_free_hostent( &((*hostent_p)->he) );
   } else {
      /* data buffer is used for the hostent struct, we just delete the data
         buffer and the struct */
      if ((*hostent_p)->he) {
         free((*hostent_p)->he);
      }
      if ((*hostent_p)->he_data_buffer) {
         free((*hostent_p)->he_data_buffer);
      }
   }

   /* finally free the struct */
   free(*hostent_p);
   *hostent_p = NULL;
   return CL_RETVAL_OK;
}



int cl_com_free_hostspec(cl_com_host_spec_t **hostspec) {

   if (hostspec == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS; 
   }
   if (*hostspec == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS; /* no memory to free ? */
   }

   cl_com_free_hostent( &((*hostspec)->hostent) );
   if ( (*hostspec)->hostent != NULL) {
      CL_LOG(CL_LOG_ERROR,"could not free hostent structure");
   }

   free( (*hostspec)->unresolved_name );
   free( (*hostspec)->resolved_name );
   free( (*hostspec)->in_addr);
   free(*hostspec);
   *hostspec = NULL;
   return CL_RETVAL_OK;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostname()"
int cl_com_gethostname(char **unique_hostname,struct in_addr *copy_addr,struct hostent **he_copy ) {  /* CR check */

   char localhostname[CL_MAXHOSTNAMELEN_LENGTH + 1];
   int retval = CL_RETVAL_OK;
   
   if (gethostname(localhostname,CL_MAXHOSTNAMELEN_LENGTH) != 0) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_LOCAL_HOSTNAME_ERROR));
      return CL_RETVAL_LOCAL_HOSTNAME_ERROR;
   }
   CL_LOG_STR( CL_LOG_DEBUG, "local gethostname() returned: ", localhostname);

   retval = cl_com_cached_gethostbyname(localhostname, unique_hostname, copy_addr, he_copy );
   return retval;
}






#if 0
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostbyname()"
static int cl_com_gethostbyname(char *host, cl_com_hostent_t **hostent ) {

   int he_error = 0;
   struct hostent *he = NULL; 
   int tmp_buffer_length = CL_MAXHOSTNAMELEN_LENGTH * 2; 
   cl_com_hostent_t *hostent_p = NULL;   

   /* check parameters */
   if (hostent == NULL || host == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we don't accept NULL pointers */
   }
   if (*hostent != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }

   hostent_p = (cl_com_hostent_t*)malloc(sizeof(cl_com_hostent_t));
   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;          /* could not get memory */ 
   }
   /* get memory for struct member variables */
   hostent_p->he = (struct hostent*) malloc(sizeof(struct hostent));
   if (hostent_p->he == NULL) {
      cl_com_free_hostent(&hostent_p);   /* could not get memory */ 
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;
   }

   /* clear memory of the hostent struct */
   memset(hostent_p->he,0,sizeof(struct hostent)); 

   while(1) {
      hostent_p->he_data_buffer = (char*) malloc ( tmp_buffer_length * sizeof(char));
      if (hostent_p->he_data_buffer == NULL) {
         cl_com_free_hostent(&hostent_p);  /* could not get memory */
         CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
         return CL_RETVAL_MALLOC;
      }

      errno = 0;
#ifndef LINUX 
      he = gethostbyname_r(host, hostent_p->he, hostent_p->he_data_buffer, (tmp_buffer_length * sizeof(char)), &he_error); 
#else
      gethostbyname_r(host, hostent_p->he, hostent_p->he_data_buffer, tmp_buffer_length * sizeof(char), &he, &he_error); 
#endif
      if (errno == ERANGE) {
         /* The data buffer is to small for all host information data. 
            We have to malloc more memory for this host */
         free(hostent_p->he_data_buffer);
         hostent_p->he_data_buffer = NULL;
         tmp_buffer_length = tmp_buffer_length * 2;  /* try it with double memory size */
         CL_LOG_INT( CL_LOG_INFO, "too small data buffer, retry with ", tmp_buffer_length );
         if (tmp_buffer_length > 256*256 ) {
            CL_LOG_INT( CL_LOG_ERROR, "stopping with data buffer size ", tmp_buffer_length );
            break;
         } else {
            continue;
         }
      }
      break;
   }

   if (he == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_UNKOWN_HOST_ERROR));
      cl_com_free_hostent(&hostent_p);       /* could not find host */
      return CL_RETVAL_UNKOWN_HOST_ERROR;
   }

   if (hostent_p->he->h_addr == NULL) {
      cl_com_free_hostent(&hostent_p);
      return CL_RETVAL_IP_NOT_RESOLVED_ERROR;
   }

   *hostent = hostent_p;
   cl_com_print_host_info(hostent_p);
   return CL_RETVAL_OK;
}
#endif
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostbyname()"
static int cl_com_gethostbyname(char *host, cl_com_hostent_t **hostent ) {

   struct hostent* he = NULL;
   cl_com_hostent_t *hostent_p = NULL;   

   /* check parameters */
   if (hostent == NULL || host == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we don't accept NULL pointers */
   }
   if (*hostent != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }

   /* get memory for cl_com_hostent_t struct */
   hostent_p = (cl_com_hostent_t*)malloc(sizeof(cl_com_hostent_t));
   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;          /* could not get memory */ 
   }
   hostent_p->he = NULL;
   hostent_p->he_data_buffer = NULL;
   
   /* use sge_gethostbyname() */
   he = sge_gethostbyname(host);
   if (he == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_UNKOWN_HOST_ERROR));
      cl_com_free_hostent(&hostent_p);       /* could not find host */
      return CL_RETVAL_UNKOWN_HOST_ERROR;
   } else {
      hostent_p->he = he;
   }

   if (hostent_p->he->h_addr == NULL) {
      cl_com_free_hostent(&hostent_p);
      return CL_RETVAL_IP_NOT_RESOLVED_ERROR;
   }

   *hostent = hostent_p;
   cl_com_print_host_info(hostent_p);
   return CL_RETVAL_OK;
}


#if 0
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostbyaddr()"
static int cl_com_gethostbyaddr(struct in_addr *addr, cl_com_hostent_t **hostent ) {


   int he_error = 0;
   struct hostent *he = NULL; 
   int tmp_buffer_length = CL_MAXHOSTNAMELEN_LENGTH * 2; 

   cl_com_hostent_t *hostent_p = NULL;   


   /* check parameters */
   if (hostent == NULL || addr == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we don't accept NULL pointers */
   }
   if (*hostent != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }

   hostent_p = (cl_com_hostent_t*)malloc(sizeof(cl_com_hostent_t));
   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;          /* could not get memory */ 
   }
 
   /* get memory for struct member variables */
   hostent_p->he = (struct hostent*) malloc(sizeof(struct hostent));
   if (hostent_p->he == NULL) {
      cl_com_free_hostent(&hostent_p);   /* could not get memory */ 
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;
   }

   /* clear memory of the hostent struct */
   memset(hostent_p->he,0,sizeof(struct hostent)); 

   while(1) {
      hostent_p->he_data_buffer = (char*) malloc ( tmp_buffer_length * sizeof(char));
      if (hostent_p->he_data_buffer == NULL) {
         cl_com_free_hostent(&hostent_p);  /* could not get memory */
         CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
         return CL_RETVAL_MALLOC;
      }

      errno = 0;
#ifndef LINUX 
      
      he = gethostbyaddr_r((const char *)addr, sizeof(struct in_addr), AF_INET, hostent_p->he, hostent_p->he_data_buffer, tmp_buffer_length * sizeof(char), &he_error); 
#else
      gethostbyaddr_r((const char *)addr, sizeof(struct in_addr), AF_INET, hostent_p->he, hostent_p->he_data_buffer, tmp_buffer_length * sizeof(char), &he, &he_error);
#endif
      if (errno == ERANGE) {
         /* The data buffer is to small for all host information data. 
            We have to malloc more memory for this host */
         free(hostent_p->he_data_buffer);
         hostent_p->he_data_buffer = NULL;
         tmp_buffer_length = tmp_buffer_length * 2;  /* try it with double memory size */
         CL_LOG_INT( CL_LOG_INFO, "too small data buffer, retry with ", tmp_buffer_length );
         if (tmp_buffer_length > 256*256 ) {
            CL_LOG_INT( CL_LOG_ERROR, "stopping with data buffer size ", tmp_buffer_length );
            break;
         } else {
            continue;
         }
      }
      break;
   }

   if (he == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_UNKOWN_HOST_ERROR));
      cl_com_free_hostent(&hostent_p);       /* could not find host */
      return CL_RETVAL_UNKOWN_HOST_ERROR;
   }
   
   if (hostent_p->he->h_addr == NULL) {
      cl_com_free_hostent(&hostent_p);
      return CL_RETVAL_IP_NOT_RESOLVED_ERROR;
   }

   *hostent = hostent_p;
   cl_com_print_host_info(hostent_p);
   return CL_RETVAL_OK;
}
#endif

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostbyaddr()"
static int cl_com_gethostbyaddr(struct in_addr *addr, cl_com_hostent_t **hostent ) {

   struct hostent *he = NULL; 
   cl_com_hostent_t *hostent_p = NULL;   


   /* check parameters */
   if (hostent == NULL || addr == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we don't accept NULL pointers */
   }
   if (*hostent != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }

   /* get memory for cl_com_hostent_t struct */
   hostent_p = (cl_com_hostent_t*)malloc(sizeof(cl_com_hostent_t));
   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_MALLOC));
      return CL_RETVAL_MALLOC;          /* could not get memory */ 
   }
   hostent_p->he = NULL;
   hostent_p->he_data_buffer = NULL;

   /* use sge_gethostbyaddr() */  
   he = sge_gethostbyaddr(addr);
   
   if (he == NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_UNKOWN_HOST_ERROR));
      cl_com_free_hostent(&hostent_p);       /* could not find host */
      return CL_RETVAL_UNKOWN_HOST_ERROR;
   } else {
      hostent_p->he = he;
   }
   
   if (hostent_p->he->h_addr == NULL) {
      cl_com_free_hostent(&hostent_p);
      return CL_RETVAL_IP_NOT_RESOLVED_ERROR;
   }

   *hostent = hostent_p;
   cl_com_print_host_info(hostent_p);
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_dup_host()"
int cl_com_dup_host(char** host_dest, char* source, cl_host_resolve_method_t method, char* domain) {

   int retval = CL_RETVAL_OK;
   int hostlen = 0;
   char* the_dot = NULL;

   if (host_dest == NULL || source == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*host_dest != NULL) {
      return CL_RETVAL_PARAMS;
   }

   CL_LOG_STR(CL_LOG_INFO,"dup of host:", source);
   the_dot = strchr(source, '.');

   switch(method) {
       case CL_SHORT:
          CL_LOG(CL_LOG_INFO,"short hostname compare");
          hostlen = strlen(source);
          if (the_dot != NULL) {
             hostlen = hostlen - strlen(the_dot);
          }
          CL_LOG_INT(CL_LOG_INFO,"hostlen is:",hostlen);
          *host_dest = (char*) malloc((sizeof(char) * hostlen) + 1);
          if (*host_dest == NULL) {
             return CL_RETVAL_MALLOC;
          }
          strncpy(*host_dest, source, hostlen);
          (*host_dest)[hostlen]=0;
          CL_LOG_STR(CL_LOG_INFO,"short hostname is:", *host_dest);
          retval = CL_RETVAL_OK;
          break;
       case CL_LONG:
          CL_LOG(CL_LOG_INFO,"long hostname compare");
          hostlen = strlen(source);
          if (domain == NULL) {
             if (the_dot == NULL) {
                CL_LOG(CL_LOG_ERROR,"can't dup host with domain name without default domain");
                /* error copy host without domain name , host is short */
                *host_dest = (char*) malloc((sizeof(char) * hostlen) + 1);
                if (*host_dest == NULL) {
                   return CL_RETVAL_MALLOC;
                }
                strncpy(*host_dest, source, hostlen);
                (*host_dest)[hostlen]=0;
             } else {
                /* we have no domain, but the host is resolved long -> ok */
                *host_dest = (char*) malloc((sizeof(char) * hostlen) + 1);
                if (*host_dest == NULL) {
                   return CL_RETVAL_MALLOC;
                }
                strncpy(*host_dest, source, hostlen);
                (*host_dest)[hostlen]=0;
             }
          } else {
             if (the_dot == NULL) {
                int length = hostlen + strlen(domain) + 1;
                /* we have a short hostname, add the default domain */
                *host_dest = (char*) malloc((sizeof(char) * length) + 1);
                if (*host_dest == NULL) { 
                   return CL_RETVAL_MALLOC;
                }
                strncpy(*host_dest, source, hostlen);
                (*host_dest)[hostlen]=0;
                strncat(*host_dest, ".", 1);
                strncat(*host_dest, domain, strlen(domain));
             } else {
                /* we have a long hostname, return original name */
                *host_dest = (char*) malloc((sizeof(char) * hostlen) + 1);
                if (*host_dest == NULL) {
                   return CL_RETVAL_MALLOC;
                }
                strncpy(*host_dest, source, hostlen);
                (*host_dest)[hostlen]=0;
             }
          }
          CL_LOG_STR(CL_LOG_INFO,"long hostname is:", *host_dest);
          retval = CL_RETVAL_OK;
          break;
       default:
          CL_LOG(CL_LOG_ERROR,"unexpected hostname resolve method");
          retval = CL_RETVAL_UNKNOWN;
          break;
   }
   return retval;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_compare_hosts()"
int cl_com_compare_hosts( char* host1, char* host2) {

    int retval = CL_RETVAL_UNKNOWN;
    char* hostbuf1 = NULL;
    char* hostbuf2 = NULL;
    cl_raw_list_t* host_list = NULL;
    cl_host_list_data_t* host_list_data = NULL;


    if (host1 == NULL || host2 == NULL) {
       return CL_RETVAL_PARAMS;
    }

    host_list = cl_com_get_host_list();
    if (host_list == NULL) {
       CL_LOG(CL_LOG_WARNING,"communication library setup error, just do strcasecmp()");
       if ( strcasecmp(host1,host2) == 0 ) {
          return CL_RETVAL_OK;
       } else {
          return CL_RETVAL_UNKNOWN;
       }
    }
    cl_raw_list_lock(host_list);
    host_list_data = cl_host_list_get_data(host_list);
    if (host_list_data == NULL) {
       CL_LOG(CL_LOG_ERROR,"communication library setup error for hostlist");
       cl_raw_list_unlock(host_list);
       return CL_RETVAL_RESOLVING_SETUP_ERROR;
    }

    CL_LOG_STR(CL_LOG_INFO,"original compare host 1:", host1);
    CL_LOG_STR(CL_LOG_INFO,"original compare host 2:", host2);

    if ( ( retval = cl_com_dup_host(&hostbuf1, host1, host_list_data->resolve_method, host_list_data->local_domain_name)) != CL_RETVAL_OK) {
       cl_raw_list_unlock(host_list);
       return retval;
    }
    if ( ( retval = cl_com_dup_host(&hostbuf2, host2, host_list_data->resolve_method, host_list_data->local_domain_name)) != CL_RETVAL_OK) {
       free(hostbuf1);
       cl_raw_list_unlock(host_list);
       return retval;
    }
    cl_raw_list_unlock(host_list);

    CL_LOG(CL_LOG_WARNING,"cl_com_compare_hosts() is slow!");
    CL_LOG_STR(CL_LOG_INFO,"compareing host 1:", hostbuf1);
    CL_LOG_STR(CL_LOG_INFO,"compareing host 2:", hostbuf2);
    if ( strcasecmp(hostbuf1,hostbuf2) == 0 ) {   /* hostname compare OK */
       CL_LOG(CL_LOG_INFO,"hosts are equal");
       retval = CL_RETVAL_OK;
    } else {
       CL_LOG_STR(CL_LOG_INFO,"hosts are NOT equal", cl_get_error_text(retval));
       retval = CL_RETVAL_UNKNOWN;
    }
    
    free(hostbuf1);
    free(hostbuf2);
    return retval;  
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_cached_gethostbyname()"
int cl_com_cached_gethostbyname( char *unresolved_host, char **unique_hostname, struct in_addr *copy_addr ,struct hostent **he_copy ) {
   cl_host_list_elem_t*    elem = NULL;
   cl_host_list_elem_t*    act_elem = NULL;
   cl_com_host_spec_t*       elem_host = NULL;
   cl_host_list_data_t* ldata = NULL;
   cl_raw_list_t* hostlist = NULL;
   cl_com_hostent_t* hostent = NULL;
   int function_return = CL_RETVAL_GETHOSTNAME_ERROR;
   int ret_val = CL_RETVAL_OK;
   char* alias_name = NULL;

   
   hostlist = cl_com_get_host_list();
   if (unresolved_host == NULL || unique_hostname == NULL) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;
   }

   if (he_copy != NULL) {
      if (*he_copy != NULL) {
         return CL_RETVAL_PARAMS;
      }
   }
   if (*unique_hostname != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }


   if (hostlist == NULL) {
      int retval;
      CL_LOG(CL_LOG_WARNING,"no global hostlist, resolving without cache");
      retval = cl_com_gethostbyname(unresolved_host, &hostent);
      if (retval != CL_RETVAL_OK) {
         cl_com_free_hostent(&hostent);
         return retval;
      }
      *unique_hostname = strdup(hostent->he->h_name);
      if (*unique_hostname == NULL) {
         cl_com_free_hostent(&hostent);
         return CL_RETVAL_MALLOC;
      }
      if (copy_addr != NULL) {
         memcpy((char*) copy_addr, hostent->he->h_addr, sizeof(struct in_addr));
      }
      if (he_copy != NULL) {
         *he_copy = sge_copy_hostent(hostent->he);
      }
      cl_com_free_hostent(&hostent);
      return CL_RETVAL_OK;
   }

   if (hostlist->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_host_list_data_t*) hostlist->list_data;

   if (cl_commlib_get_thread_state() == CL_NO_THREAD) {
      cl_com_host_list_refresh(hostlist);
   }


   if ( cl_host_alias_list_get_local_resolved_name(ldata->host_alias_list,unresolved_host, &alias_name) == CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_INFO,"unresolved host name is aliased to", alias_name);
   }


   /* Try to find unresolved hostname in hostlist */
   cl_raw_list_lock(hostlist);

   elem = cl_host_list_get_first_elem(hostlist);
   while(elem != NULL) {
      act_elem = elem;
      elem_host = act_elem->host_spec;
      if ( elem_host->unresolved_name != NULL) {
         /* do we have an unresolved name for this host ? */
         if ( strcasecmp(elem_host->unresolved_name,unresolved_host) == 0 ) {  /* hostname compare OK */
            CL_LOG(CL_LOG_INFO,"found match for unresolved name");
            break;   /* found host in cache */
         }
         if (alias_name != NULL) {
            if ( strcasecmp(elem_host->unresolved_name,alias_name) == 0 ) {  /* hostname compare OK */
               CL_LOG(CL_LOG_INFO,"found match for alias");
               break;   /* found host in cache */
            }
         }
      }
      elem_host = NULL;
      elem = cl_host_list_get_next_elem(hostlist,elem);
   }

   if (elem_host != NULL) {
      if (alias_name != NULL) {
         free(alias_name);
         alias_name = NULL;
      }
      CL_LOG_STR(CL_LOG_INFO,"found host in cache, unresolved name:", unresolved_host );
      if (elem_host->resolved_name == NULL) {
         cl_raw_list_unlock(hostlist);
         return CL_RETVAL_GETHOSTNAME_ERROR;
      }

      if (copy_addr != NULL && elem_host->hostent != NULL) {
         memcpy((char*) copy_addr, elem_host->hostent->he->h_addr, sizeof(struct in_addr));
      }
      *unique_hostname = strdup(elem_host->resolved_name);
      if (he_copy != NULL && elem_host->hostent != NULL ) {
         *he_copy = sge_copy_hostent(elem_host->hostent->he);
      }
      cl_raw_list_unlock(hostlist);

      if (*unique_hostname == NULL) {
         return CL_RETVAL_MALLOC;
      }
   } else {
      /* resolve the host and add the host to cache */
      int retval = CL_RETVAL_OK;
      struct timeval now;
      cl_com_hostent_t*   hostent = NULL;
      cl_com_host_spec_t* hostspec = NULL;

      if (alias_name == NULL) {
         CL_LOG_STR(CL_LOG_INFO,"NOT found in cache, unresolved name:", unresolved_host);
      } else {
         CL_LOG_STR(CL_LOG_INFO,"NOT found in cach, aliased name:", alias_name);
      }
      cl_raw_list_unlock(hostlist);


      hostspec = ( cl_com_host_spec_t*) malloc( sizeof(cl_com_host_spec_t) );
      if (hostspec == NULL) {
         return CL_RETVAL_MALLOC;
      }

      hostspec->in_addr = NULL;
      if (alias_name == NULL) {
         hostspec->unresolved_name = strdup(unresolved_host);
      } else {
         hostspec->unresolved_name = alias_name;  /* to not free alias_name !!! */
         alias_name = NULL;
      }
      if ( hostspec->unresolved_name == NULL) {
         cl_com_free_hostspec(&hostspec);
         return CL_RETVAL_MALLOC;
      }

      retval = cl_com_gethostbyname(hostspec->unresolved_name, &hostent);
      hostspec->hostent = hostent;
      hostspec->resolve_error = retval;
      gettimeofday(&now,NULL);
      hostspec->last_resolve_time = now.tv_sec;
      hostspec->creation_time = now.tv_sec;
      if (hostspec->hostent != NULL) {
         hostspec->resolved_name = NULL;
         hostspec->resolved_name = strdup(hostspec->hostent->he->h_name); 
         if (hostspec->resolved_name == NULL) {
            cl_com_free_hostspec(&hostspec);
            return CL_RETVAL_MALLOC;
         }
         hostspec->in_addr = (struct in_addr*) malloc (sizeof(struct in_addr));
         if (hostspec->in_addr == NULL) {
            cl_com_free_hostspec(&hostspec);
            return CL_RETVAL_MALLOC;
         }
         memcpy(hostspec->in_addr,hostspec->hostent->he->h_addr,sizeof(struct in_addr) );
      } else {
         hostspec->resolved_name = NULL;
      }

      cl_raw_list_lock(hostlist);
      function_return = cl_host_list_append_host(hostlist, hostspec, 0);
      if (function_return != CL_RETVAL_OK) {
         cl_raw_list_unlock(hostlist);
         cl_com_free_hostspec(&hostspec);
         return function_return;
      }

      if (hostspec->resolved_name == NULL) {
         cl_raw_list_unlock(hostlist);
         return CL_RETVAL_GETHOSTNAME_ERROR;
      }
      if (copy_addr != NULL) {
         memcpy((char*) copy_addr, hostspec->hostent->he->h_addr, sizeof(struct in_addr) );
      }
      *unique_hostname = strdup(hostspec->resolved_name);
      if (he_copy != NULL && hostspec->hostent->he != NULL) {
         *he_copy = sge_copy_hostent(hostspec->hostent->he);
      }

      cl_raw_list_unlock(hostlist);
      if (*unique_hostname == NULL) {
         return CL_RETVAL_MALLOC;
      }
   }
   CL_LOG_STR(CL_LOG_INFO,"resolved name:", *unique_hostname );

   ret_val = cl_host_alias_list_get_alias_name(ldata->host_alias_list, *unique_hostname, &alias_name );
   if (ret_val == CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_INFO,"resolved name aliased to", alias_name);
      free(*unique_hostname);
      *unique_hostname = alias_name;
   }

   return CL_RETVAL_OK;
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_host_list_refresh()"
int cl_com_host_list_refresh(cl_raw_list_t* list_p) {
   struct timeval now;
   cl_host_list_elem_t*    elem = NULL;
   cl_host_list_elem_t*    act_elem = NULL;
   cl_host_list_data_t* ldata = NULL;
   int resolve_host = 0;

   cl_com_host_spec_t*       elem_host = NULL;

   if (list_p->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_host_list_data_t*) list_p->list_data;

   gettimeofday(&now,NULL);
   if ( now.tv_sec == ldata->last_refresh_time) {
      return CL_RETVAL_OK;
   }
   ldata->last_refresh_time = now.tv_sec;

   CL_LOG(CL_LOG_INFO,"checking host entries");
   CL_LOG_INT(CL_LOG_INFO,"number of cached host entries:", cl_raw_list_get_elem_count(list_p));
   
   cl_raw_list_lock(list_p);

   elem = cl_host_list_get_first_elem(list_p);
   while(elem != NULL) {
      act_elem = elem;
      elem = cl_host_list_get_next_elem(list_p,elem);
      elem_host = act_elem->host_spec;
      resolve_host = 0;


      if (elem_host->creation_time + ldata->entry_life_time < now.tv_sec ) {
         /* max entry life time reached, remove entry */
         if (elem_host->unresolved_name != NULL) {
            CL_LOG_STR(CL_LOG_WARNING,"entry life timeout for elem:", elem_host->unresolved_name);
         } else {
            CL_LOG(CL_LOG_WARNING,"entry life timeout for addr");
         }
         cl_raw_list_remove_elem(list_p, act_elem->raw_elem);
         cl_com_free_hostspec(&elem_host);
         free(act_elem);
         continue; /* removed entry, continue with next */
      }

      if (elem_host->last_resolve_time + ldata->entry_update_time < now.tv_sec) {
         /* max update timeout is reached, resolving entry */
         if (elem_host->unresolved_name != NULL) {
            CL_LOG_STR(CL_LOG_WARNING,"update timeout for elem:", elem_host->unresolved_name);
         } else {
            CL_LOG(CL_LOG_WARNING,"update timeout for addr");
         }
         resolve_host = 1;
      }
      
      if (elem_host->resolve_error != CL_RETVAL_OK) {
         /* this is only for hosts with error state */
         if (elem_host->last_resolve_time + ldata->entry_reresolve_time < now.tv_sec) {
            if (elem_host->unresolved_name != NULL) {
               CL_LOG_STR(CL_LOG_WARNING,"reresolve timeout for elem:",elem_host->unresolved_name);
            } else {
               CL_LOG(CL_LOG_WARNING,"reresolve timeout for addr");
            }
            resolve_host = 1;
         }
      }

      if ( resolve_host != 0 ) {
         int retval = CL_RETVAL_OK;
         cl_com_hostent_t* hostent = NULL;
         if (elem_host->unresolved_name != NULL) {
            CL_LOG_STR(CL_LOG_INFO,"resolving host:", elem_host->unresolved_name);
            retval = cl_com_gethostbyname(elem_host->unresolved_name, &hostent);
            /* free old entries */
            cl_com_free_hostent(&(elem_host->hostent));
            free(elem_host->resolved_name);
            elem_host->resolved_name = NULL;
            elem_host->hostent = hostent;
            elem_host->resolve_error = retval;
            elem_host->last_resolve_time = now.tv_sec;
            if (elem_host->hostent != NULL) {
               elem_host->resolved_name = strdup(elem_host->hostent->he->h_name);
               if (elem_host->resolved_name == NULL) {
                  cl_raw_list_remove_elem(list_p, act_elem->raw_elem);
                  cl_com_free_hostspec(&elem_host);
                  free(act_elem);
                  CL_LOG(CL_LOG_ERROR,"malloc() error");
                  cl_raw_list_unlock(list_p);
                  return CL_RETVAL_MALLOC;
               }
               CL_LOG_STR(CL_LOG_WARNING,"host resolved as:", elem_host->resolved_name);
            }
         } else {
            CL_LOG(CL_LOG_INFO,"resolving addr");
            retval = cl_com_gethostbyaddr(elem_host->in_addr, &hostent);
            /* free old entries */
            cl_com_free_hostent(&(elem_host->hostent));
            free(elem_host->resolved_name);
            elem_host->resolved_name = NULL;
            elem_host->hostent = hostent;
            elem_host->resolve_error = retval;
            elem_host->last_resolve_time = now.tv_sec;
            if (elem_host->hostent != NULL) {
               elem_host->resolved_name = strdup(elem_host->hostent->he->h_name);
               if (elem_host->resolved_name == NULL) {
                  cl_raw_list_remove_elem(list_p, act_elem->raw_elem);
                  cl_com_free_hostspec(&elem_host);
                  free(act_elem);
                  CL_LOG(CL_LOG_ERROR,"malloc() error");
                  cl_raw_list_unlock(list_p);
                  return CL_RETVAL_MALLOC;
               }
               CL_LOG_STR(CL_LOG_WARNING,"host resolved as:", elem_host->resolved_name);
            }
         }
      }
   }

   cl_raw_list_unlock(list_p);
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_cached_gethostbyaddr()"
int cl_com_cached_gethostbyaddr( struct in_addr *addr, char **unique_hostname,struct hostent **he_copy ) {
   cl_host_list_elem_t*    elem = NULL;
   cl_host_list_elem_t*    act_elem = NULL;
   cl_com_host_spec_t*       elem_host = NULL;
   cl_host_list_data_t* ldata = NULL;
   cl_raw_list_t* hostlist = NULL;
   int ret_val = CL_RETVAL_OK;
   char* alias_name = NULL;
   int resolve_name_ok = 0;
   int function_return = CL_RETVAL_GETHOSTNAME_ERROR;
   

   hostlist = cl_com_get_host_list();

   if (addr == NULL || unique_hostname == NULL) {
      CL_LOG(CL_LOG_ERROR,"parameters not correct");
      return CL_RETVAL_PARAMS;
   }

   if (he_copy != NULL) {
      if (*he_copy != NULL) {
         return CL_RETVAL_PARAMS;
      }
   }

   if (*unique_hostname != NULL) {
      CL_LOG( CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;    /* we expect an pointer address, set to NULL */
   }

   if (hostlist == NULL) {
      int retval;
      cl_com_hostent_t* hostent = NULL;

      CL_LOG(CL_LOG_WARNING,"no global hostlist, resolving without cache");
      retval = cl_com_gethostbyaddr(addr, &hostent);
      if (retval != CL_RETVAL_OK) {
         cl_com_free_hostent(&hostent);
         return retval;
      }
      *unique_hostname = strdup(hostent->he->h_name);
      if (he_copy != NULL) {
         *he_copy = sge_copy_hostent(hostent->he);
      }
      if (*unique_hostname == NULL) {
         cl_com_free_hostent(&hostent);
         return CL_RETVAL_MALLOC;
      }
      cl_com_free_hostent(&hostent);
      return CL_RETVAL_OK;
   }

   if (hostlist->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_host_list_data_t*) hostlist->list_data;


   if (cl_commlib_get_thread_state() == CL_NO_THREAD) {
      cl_com_host_list_refresh(hostlist);
   }

   cl_raw_list_lock(hostlist);
   elem = cl_host_list_get_first_elem(hostlist);
   while(elem != NULL) {
      act_elem = elem;
      elem_host = act_elem->host_spec;

      /* do we have an unresolved name for this host ? */
      if ( elem_host->in_addr != NULL) {
         if (memcmp(elem_host->in_addr , addr ,sizeof(struct in_addr)) == 0) {
            break; /* found addr in cache */
         }
      }
      elem_host = NULL;
      elem = cl_host_list_get_next_elem(hostlist,elem);
   }

   if (elem_host != NULL) {
      if (elem_host->resolved_name == NULL) {
         CL_LOG(CL_LOG_INFO,"found addr in cache - not resolveable");
         cl_raw_list_unlock(hostlist);
         return CL_RETVAL_GETHOSTNAME_ERROR;
      }
      CL_LOG_STR(CL_LOG_INFO,"found addr in cache, resolved name:",elem_host->resolved_name);

      *unique_hostname = strdup(elem_host->resolved_name);
      if (he_copy != NULL && elem_host->hostent != NULL) {
         *he_copy = sge_copy_hostent(elem_host->hostent->he);
      }
      cl_raw_list_unlock(hostlist);

      if (*unique_hostname == NULL) {
         return CL_RETVAL_MALLOC;
      }
   } else {
      /* resolve the host and add the host to cache */
      struct timeval now;

      int retval = CL_RETVAL_OK;
      cl_com_hostent_t* hostent = NULL;
      cl_com_host_spec_t* hostspec = NULL;
      char* hostname = NULL;
      CL_LOG(CL_LOG_INFO,"addr NOT found in cache");
      cl_raw_list_unlock(hostlist);

      hostspec = ( cl_com_host_spec_t*) malloc( sizeof(cl_com_host_spec_t) );
      if (hostspec == NULL) {
         return CL_RETVAL_MALLOC;
      }

      hostspec->unresolved_name = NULL;
      hostspec->in_addr = (struct in_addr*) malloc (sizeof(struct in_addr));
      if (hostspec->in_addr == NULL) {
         cl_com_free_hostspec(&hostspec);
         return CL_RETVAL_MALLOC;
      }
      memcpy(hostspec->in_addr,addr,sizeof(struct in_addr) );

      /* resolve host with cl_com_gethostbyaddr() */
      retval = cl_com_gethostbyaddr(addr, &hostent);
      hostspec->hostent = hostent;
      hostspec->resolve_error = retval;
      gettimeofday(&now,NULL);
      hostspec->last_resolve_time = now.tv_sec;
      hostspec->creation_time = now.tv_sec;
      hostspec->resolved_name = NULL;

      if (hostspec->hostent != NULL) {
         /* CHECK for correct resolving */
         retval = cl_com_cached_gethostbyname( hostent->he->h_name, &hostname , NULL, he_copy);
         if (retval != CL_RETVAL_OK) {
             CL_LOG_STR(CL_LOG_WARNING,"can't resolve host name", hostent->he->h_name);
             hostspec->resolve_error = CL_RETVAL_GETHOSTNAME_ERROR;
             /* add dummy host entry */
             cl_raw_list_lock(hostlist);
             function_return = cl_host_list_append_host(hostlist, hostspec, 0);
             if (function_return != CL_RETVAL_OK) {
                cl_raw_list_unlock(hostlist);
                cl_com_free_hostspec(&hostspec);
                return function_return;
             }
             cl_raw_list_unlock(hostlist);
             return CL_RETVAL_GETHOSTNAME_ERROR;
         }
         resolve_name_ok = 1;
         ret_val = cl_host_alias_list_get_alias_name(ldata->host_alias_list,hostent->he->h_name , &alias_name );
         if (ret_val == CL_RETVAL_OK) {
            CL_LOG_STR(CL_LOG_INFO,"resolved addr name aliased to", alias_name);
            if ( strcasecmp(hostname , alias_name ) != 0 ) {
               resolve_name_ok = 0;
            }
            free(alias_name);
            alias_name = NULL;
         } else {
            if ( strcasecmp(hostname , hostent->he->h_name ) != 0 ) {
               resolve_name_ok = 0;
            }
         }

         /* if names are not equal -> report error */ 
         if (resolve_name_ok != 1) {     /* hostname compare OK */

            CL_LOG(CL_LOG_ERROR,"host resolving by address returns not the same host name as resolving by name");
            hostspec->resolve_error = CL_RETVAL_GETHOSTNAME_ERROR;
            /* add dummy host entry */
            cl_raw_list_lock(hostlist);
             function_return = cl_host_list_append_host(hostlist, hostspec, 0);
             if (function_return != CL_RETVAL_OK) {
                cl_raw_list_unlock(hostlist);
                cl_com_free_hostspec(&hostspec);
                return function_return;
             }
             cl_raw_list_unlock(hostlist);
             return CL_RETVAL_GETHOSTNAME_ERROR;
         } 
         /* if names are equal -> perfect ! , return resolved hostname */
         *unique_hostname = hostname;
      } else {
         /* add dummy host entry */
         cl_raw_list_lock(hostlist);
         function_return = cl_host_list_append_host(hostlist, hostspec, 0);
         if (function_return != CL_RETVAL_OK) {
            cl_raw_list_unlock(hostlist);
            cl_com_free_hostspec(&hostspec);
            return function_return;
         }
         cl_raw_list_unlock(hostlist);
         return CL_RETVAL_GETHOSTNAME_ERROR;
      }
      cl_com_free_hostspec(&hostspec);
   }

   CL_LOG_STR(CL_LOG_INFO,"resolved name:", *unique_hostname );
   ret_val = cl_host_alias_list_get_alias_name(ldata->host_alias_list, *unique_hostname, &alias_name );
   if (ret_val == CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_INFO,"resolved name aliased to", alias_name);
      free(*unique_hostname);
      *unique_hostname = alias_name;
   }
   return CL_RETVAL_OK;
}





#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_create_endpoint()"
cl_com_endpoint_t* cl_com_create_endpoint(const char* host, const char* name, unsigned long id) {  /* CR check */
   cl_com_endpoint_t* endpoint = NULL;

   if (host == NULL || name == NULL ) {
      CL_LOG(CL_LOG_ERROR,"parameter errors");
      return NULL;
   }

   endpoint = (cl_com_endpoint_t*)malloc(sizeof(cl_com_endpoint_t));
   if (endpoint == NULL) {
      CL_LOG(CL_LOG_ERROR,"malloc error");
      return NULL;
   }
   endpoint->comp_host = strdup(host);
   endpoint->comp_name = strdup(name);
   endpoint->comp_id   = id;

   if (endpoint->comp_host == NULL || endpoint->comp_name == NULL) {
      cl_com_free_endpoint(&endpoint);
      CL_LOG(CL_LOG_ERROR,"malloc error");
      return NULL;
   }
   return endpoint;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_compare_endpoints()"
int cl_com_compare_endpoints(cl_com_endpoint_t* endpoint1, cl_com_endpoint_t* endpoint2) {  /* CR check */
   if (endpoint1 != NULL && endpoint2 != NULL) {
       if (endpoint1->comp_id == endpoint2->comp_id ) {
          if (endpoint1->comp_host && endpoint1->comp_name && 
              endpoint2->comp_host && endpoint2->comp_name) {
             if (cl_com_compare_hosts(endpoint1->comp_host,endpoint2->comp_host) == CL_RETVAL_OK) {
                if (strcmp(endpoint1->comp_name,endpoint2->comp_name ) == 0) {
                   return 1;
                }
             }
          }
       }
   }
   return 0;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_dump_endpoint()"
void cl_com_dump_endpoint(cl_com_endpoint_t* endpoint, const char* text) {  /* CR check */
   if (endpoint == NULL) {
      CL_LOG(CL_LOG_DEBUG,"endpoint is NULL");
      return;
   }
   if (endpoint->comp_host == NULL || endpoint->comp_name == NULL ) {
      CL_LOG(CL_LOG_DEBUG,"endpoint data is NULL");
      return;
   }
   if (text) {
      CL_LOG(CL_LOG_DEBUG, "========================");
      CL_LOG(CL_LOG_DEBUG, text);
      CL_LOG(CL_LOG_DEBUG, "------------------------");
   }
   CL_LOG_STR(CL_LOG_DEBUG, "comp_host :", endpoint->comp_host);
   CL_LOG_STR(CL_LOG_DEBUG, "comp_name :", endpoint->comp_name);
   CL_LOG_INT(CL_LOG_DEBUG, "comp_id   :", endpoint->comp_id);
   if (text) {
      CL_LOG(CL_LOG_DEBUG, "========================");
   }

}



/* free cl_com_endpoint_t  structure 
  
   params: 
   cl_com_endpoint_t** endpoint -> address of an pointer to cl_com_endpoint_t

   return:
      - *endpoint is set to NULL
      - int - CL_RETVAL_XXXX error number
*/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_free_endpoint()"
int cl_com_free_endpoint(cl_com_endpoint_t** endpoint) { /* CR check */
   if (endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }

   free((*endpoint)->comp_host);
   free((*endpoint)->comp_name);
   free(*endpoint);
   *endpoint = NULL;
   return CL_RETVAL_OK;
}



/* cl_com_print_host_info - log a hostent struct 
  
   params: 

   cl_com_hostent_t* hostent_p -> pointer to filled cl_com_hostent_t

   return:
      - int - CL_RETVAL_XXXX error number
*/

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_print_host_info()"
int cl_com_print_host_info(cl_com_hostent_t *hostent_p ) {

   char** tp = NULL;
   struct in_addr in;

   if (hostent_p == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;
   }
   if (hostent_p->he == NULL ) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;
   }
   if (hostent_p->he->h_addr == NULL || 
       hostent_p->he->h_name == NULL ||
       hostent_p->he->h_aliases == NULL || 
       hostent_p->he->h_addr_list == NULL) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;
   }

   memcpy(&in.s_addr,hostent_p->he->h_addr,sizeof (in.s_addr));
   CL_LOG_STR( CL_LOG_INFO, "official name of host : ", hostent_p->he->h_name );
   for (tp = hostent_p->he->h_aliases; *tp; tp++) {
      CL_LOG_STR( CL_LOG_INFO, "alias                 : ", *tp );
   }
   return CL_RETVAL_OK;
}



/****** cl_communication/cl_com_send_message() *********************************
*  NAME
*     cl_com_send_message() -- send a message to an open connection
*
*  SYNOPSIS
*     int cl_com_send_message(cl_com_connection_t* connection, int timeout, 
*     cl_byte_t* data, long size) 
*
*  FUNCTION
*     This wrapper function will call the correct cl_com_xxx_send_message()
*     function for the selected framework.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to open connection object
*     int timeout                     - timeout for sending the message
*     cl_byte_t* data                 - data buffer to send
*     long size                       - size of data buffer
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_receive_message()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_send_message()"
int cl_com_send_message(cl_com_connection_t* connection, int timeout_time, cl_byte_t* data, unsigned long size, unsigned long *only_one_write ) {  /* CR check */
   if (connection != NULL) {
      switch(connection->framework_type) {
         case CL_CT_TCP:
            return cl_com_tcp_send_message( connection, timeout_time, data, size, only_one_write );
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;

}

/****** cl_communication/cl_com_receive_message() ******************************
*  NAME
*     cl_com_receive_message() -- receive a message from an open connection
*
*  SYNOPSIS
*     int cl_com_receive_message(cl_com_connection_t* connection, int timeout, 
*     cl_byte_t** data) 
*
*  FUNCTION
*     This wrapper function will cal the correct cl_com_xxx_receive_message()
*     function for the selected framework. The caller has to free the data
*     buffer returned in data.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to open connection object
*     int timeout                     - timeout for receiving the message 
*     cl_byte_t** data                - address of an pointer
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_send_message()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_receive_message()"
int cl_com_receive_message(cl_com_connection_t* connection, int timeout_time, cl_byte_t* data_buffer, unsigned long data_buffer_size, unsigned long *only_one_read) {   /* CR check */
     
   if (connection != NULL) {
      switch(connection->framework_type) {
         case CL_CT_TCP:
            return cl_com_tcp_receive_message(connection, timeout_time, data_buffer, data_buffer_size, only_one_read);
         default:
            CL_LOG(CL_LOG_ERROR, "undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR, "connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}




/****** cl_communication/cl_com_connection_request_handler_setup() *************
*  NAME
*     cl_com_connection_request_handler_setup() -- Setup service
*
*  SYNOPSIS
*     int cl_com_connection_request_handler_setup(cl_com_connection_t* 
*     connection) 
*
*  FUNCTION
*     This function is used to setup a connection service handler. All service
*     specific setup is done here. When the setup was done the connection can
*     be used to call cl_com_connection_request_handler(). To shutdown the
*     service a call to cl_com_connection_request_handler_cleanup() must be done.
*
*     This function is only a wrapper for the correct 
*     cl_com_xxx_connection_request_handler_setup() function of the selected
*     framework.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to a inizialized connection
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_connection_request_handler_cleanup()
*     cl_communication/cl_com_connection_request_handler()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_connection_request_handler_setup()"
int cl_com_connection_request_handler_setup(cl_com_connection_t* connection,cl_com_endpoint_t* local_endpoint) {  /* CR check */
   int retval = CL_RETVAL_OK;
   if (connection != NULL) {
      if (connection->receiver != NULL ||
          connection->sender   != NULL ||
          connection->local    != NULL ||
          connection->remote   != NULL      ) {
         CL_LOG(CL_LOG_ERROR,"no free connection");
         return CL_RETVAL_PARAMS;
      }
      /* create local endpoint */
      connection->local = cl_com_create_endpoint(local_endpoint->comp_host, local_endpoint->comp_name, local_endpoint->comp_id);
      if (connection->local == NULL) {
         return CL_RETVAL_MALLOC;
      }
      /* set service handler flag */
      connection->service_handler_flag = CL_COM_SERVICE_HANDLER;

      switch(connection->framework_type) {
         case CL_CT_TCP:
            retval = cl_com_tcp_connection_request_handler_setup(connection,local_endpoint);
            if (retval != CL_RETVAL_OK) {
               /* free endpoint */
               cl_com_free_endpoint(&(connection->local));
               /* reset service handler flag */
               connection->service_handler_flag = CL_COM_SERVICE_UNDEFINED;
            }
            return retval;
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}


/****** cl_communication/cl_com_connection_request_handler() *******************
*  NAME
*     cl_com_connection_request_handler() -- Get new incomming connections
*
*  SYNOPSIS
*     int cl_com_connection_request_handler(cl_com_connection_t* connection, 
*     cl_com_connection_t** new_connection, int timeout_val_sec, int 
*     timeout_val_usec) 
*
*  FUNCTION
*     This wrapper function will call the correct 
*     cl_com_xxx_connection_request_handler() function for the selected 
*     framework.
*
*     It will create a new connection pointer and sets new_connection to the
*     new connection when connection requests are queueing. new_connection
*     must point to NULL when calling this function.
*
*     The new connection must be handled (and erased) by the caller of this
*     function.
*
*  INPUTS
*     cl_com_connection_t* connection      - pointer to service connection 
*                                            struct. (Created with a call to
*                                            cl_com_connection_request_handler_setup())
*     cl_com_connection_t** new_connection - pointer to an address of a cl_com_connection_t
*                                            struct. (will be set to a new
*                                            connection)
*     int timeout_val_sec                  - timeout in sec
*     int timeout_val_usec                 - timeout in usec
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_connection_request_handler_cleanup()
*     cl_communication/cl_com_connection_request_handler_setup()
*     cl_communication/cl_com_connection_request_handler()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_connection_request_handler()"
int cl_com_connection_request_handler(cl_com_connection_t* connection,cl_com_connection_t** new_connection, int timeout_val_sec, int timeout_val_usec ) { /* CR check */


   int usec_rest = 0;
   int full_usec_seconds = 0;
   int sec_param = 0;
   int retval = CL_RETVAL_OK;



   usec_rest = timeout_val_usec % 1000000;           /* usec parameter for select should not be > 1000000 !!!*/
   full_usec_seconds = timeout_val_usec / 1000000;   /* full seconds from timeout_val_usec parameter */
   sec_param = timeout_val_sec + full_usec_seconds;  /* add full seconds from usec parameter to timeout_val_sec parameter */

   if (connection != NULL) {

      if (connection->service_handler_flag != CL_COM_SERVICE_HANDLER) {
         CL_LOG(CL_LOG_ERROR,"connection service handler flag not set");
         return CL_RETVAL_NOT_SERVICE_HANDLER;
      }

      switch(connection->framework_type) {
         case CL_CT_TCP:
            retval = cl_com_tcp_connection_request_handler(connection,new_connection, sec_param, usec_rest );
            if (*new_connection != NULL && retval == CL_RETVAL_OK) {
               /* setup new cl_com_connection_t */
               (*new_connection)->service_handler_flag = CL_COM_CONNECTION;
               (*new_connection)->connection_state = CL_COM_CONNECTING;
               (*new_connection)->connection_sub_state = CL_COM_READ_INIT;
               (*new_connection)->was_accepted = 1;
               (*new_connection)->local = cl_com_create_endpoint(connection->local->comp_host,
                                                                 connection->local->comp_name, 
                                                                 connection->local->comp_id );
               if ( (*new_connection)->local == NULL ) {
                  cl_com_tcp_close_connection(new_connection);
                  retval = CL_RETVAL_MALLOC;
               }
            }
            return retval;
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   }else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;

}

/****** cl_communication/cl_com_connection_request_handler_cleanup() ***********
*  NAME
*     cl_com_connection_request_handler_cleanup() -- cleanup service
*
*  SYNOPSIS
*     int cl_com_connection_request_handler_cleanup(cl_com_connection_t* 
*     connection) 
*
*  FUNCTION
*     This wrapper function calls the correct 
*     cl_com_xxx_connection_request_handler_cleanup() function to shutdown a
*     server connection.
*
*  INPUTS
*     cl_com_connection_t* connection - open service connection struct
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_connection_request_handler()
*     cl_communication/cl_com_connection_request_handler_setup()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_connection_request_handler_cleanup()"
int cl_com_connection_request_handler_cleanup(cl_com_connection_t* connection) { /* CR check */
   if (connection != NULL) {

      if (connection->service_handler_flag != CL_COM_SERVICE_HANDLER) {
         return CL_RETVAL_NOT_SERVICE_HANDLER;
      }
      switch(connection->framework_type) {
         case CL_CT_TCP:
            return cl_com_tcp_connection_request_handler_cleanup(connection);
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   } else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }
   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}



/****** cl_communication/cl_com_open_connection_request_handler() **************
*  NAME
*     cl_com_open_connection_request_handler() -- Check for incomming data
*
*  SYNOPSIS
*     int cl_com_open_connection_request_handler(int framework_type, 
*     cl_raw_list_t* connection_list, int timeout) 
*
*  FUNCTION
*     This function is a wrapper for the correct 
*     cl_com_xxx_open_connection_request_handler() function of the selected
*     framework.
*
*     This function will set the connection data_read_flag if there is any
*     data to read from this connection.
*
*  INPUTS
*     int framework_type             - framework type of connection list
*     cl_raw_list_t* connection_list - list of connections to check
*     int timeout                    - timeout
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_open_connection_request_handler()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_open_connection_request_handler()"


/* WARNING connection list must be locked */

int cl_com_open_connection_request_handler(int framework_type, cl_raw_list_t* connection_list, cl_com_connection_t* service_connection, int timeout_val_sec, int timeout_val_usec , cl_select_method_t select_mode) {  /* CR check */

   int usec_rest = 0;
   int full_usec_seconds = 0;
   int sec_param = 0;

   usec_rest = timeout_val_usec % 1000000;           /* usec parameter for select should not be > 1000000 !!!*/
   full_usec_seconds = timeout_val_usec / 1000000;   /* full seconds from timeout_val_usec parameter */
   sec_param = timeout_val_sec + full_usec_seconds;  /* add full seconds from usec parameter to timeout_val_sec parameter */

   if (connection_list != NULL) {
      switch(framework_type) {
         case CL_CT_TCP:
            return cl_com_tcp_open_connection_request_handler(connection_list,service_connection,sec_param , usec_rest,select_mode );
         default:
            CL_LOG(CL_LOG_ERROR,"undefined framework type");
            break;
      }
   }else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }

   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}


