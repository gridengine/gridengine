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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "sge_hostname.h"
#include "cl_commlib.h"
#include "cl_util.h"
#include "cl_data_types.h"
#include "cl_tcp_framework.h"
#if 0
#include "cl_ssl_framework.h"
#endif
#include "cl_message_list.h"
#include "cl_host_list.h"
#include "cl_host_alias_list.h"
#include "cl_endpoint_list.h"
#include "cl_communication.h"

static int cl_com_gethostbyname(char *hostname, cl_com_hostent_t **hostent, int* system_error );
static int cl_com_gethostbyaddr(struct in_addr *addr, cl_com_hostent_t **hostent, int* system_error_retval );
static cl_bool_t cl_ingore_timeout = CL_FALSE;


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
   CL_LOG_INT(CL_LOG_DEBUG, "comp_id   :", (int)endpoint->comp_id);
   if (text) {
      CL_LOG(CL_LOG_DEBUG, "========================");
   }

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

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_setup_message()"
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
   if ( connection->last_send_message_id == 0) {
      /* the first send message will set last_send_message_id to 1 */
      /* last_send_message_id is initialized with 0 */
      connection->last_send_message_id = 1;
   }
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

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_create_message()"
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

      CL_LOG_INT(CL_LOG_DEBUG,"elements in received_message_list:", (int)cl_raw_list_get_elem_count(connection->received_message_list));
      CL_LOG_INT(CL_LOG_DEBUG,"elements in send_message_list:", (int)cl_raw_list_get_elem_count(connection->send_message_list));
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
         case CL_CT_TCP: {
            cl_dump_tcp_private(connection);
            break;
         }
#if 0
         case CL_CT_SSL: {
            cl_dump_ssl_private(connection);
            break;
         }
#endif
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
#if 0
      case CL_CT_SSL: {
         return cl_com_ssl_read_GMSH(connection,only_one_read);
      }
#endif
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
#if 0
      case CL_CT_SSL: {
         return "CL_CT_SSL";
      }
#endif
   }
   return "unexpected framework type";
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
   }
   CL_LOG(CL_LOG_WARNING,"undefined connection type");
   return "unknown";
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
      case CL_DISCONNECTED: {
         return "CL_DISCONNECTED";
      }
      case CL_CLOSING: {
         return "CL_CLOSING";
      }
      case CL_OPENING: {
         return "CL_OPENING";
      }
      case CL_CONNECTED: {
         return "CL_CONNECTED";
      }
      case CL_CONNECTING: {
         return "CL_CONNECTING";
      }
   }
   CL_LOG(CL_LOG_ERROR,"undefined marked to close flag type");
   return "unknown";
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_connection_sub_state()"
const char* cl_com_get_connection_sub_state(cl_com_connection_t* connection) {
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return "NULL";
   }

   switch(connection->connection_state ) {
      case CL_DISCONNECTED: {
         return "UNEXPECTED CONNECTION SUB STATE";
      }
      case CL_CLOSING: {
         return "UNEXPECTED CONNECTION SUB STATE";
      }
      case CL_OPENING: {
         switch( connection->connection_sub_state ) {
            case CL_COM_OPEN_INIT:
               return "CL_COM_OPEN_INIT";
            case CL_COM_OPEN_CONNECT:
               return "CL_COM_OPEN_CONNECT";
            case CL_COM_OPEN_CONNECTED:
               return "CL_COM_OPEN_CONNECTED";
            default:
               return "UNEXPECTED CONNECTION SUB STATE";
         }
      }
      case CL_CONNECTED: {
         switch( connection->connection_sub_state ) {
            case CL_COM_WORK:
               return "CL_COM_WORK";
            case CL_COM_RECEIVED_CCM:
               return "CL_COM_RECEIVED_CCM";
            case CL_COM_SENDING_CCM:
               return "CL_COM_SENDING_CCM";
            case CL_COM_WAIT_FOR_CCRM:
               return "CL_COM_WAIT_FOR_CCRM";
            case CL_COM_SENDING_CCRM:
               return "CL_COM_SENDING_CCRM";
            case CL_COM_CCRM_SENT:
               return "CL_COM_CCRM_SENT";
            case CL_COM_DONE:
               return "CL_COM_DONE";
            case CL_COM_DONE_FLUSHED:
               return "CL_COM_DONE_FLUSHED";
            default:
               return "UNEXPECTED CONNECTION SUB STATE";
         }

      }
      case CL_CONNECTING: {
         switch( connection->connection_sub_state ) {
            case CL_COM_READ_INIT:
               return "CL_COM_READ_INIT";
            case CL_COM_READ_GMSH:
               return "CL_COM_READ_GMSH";
            case CL_COM_READ_CM:
               return "CL_COM_READ_CM";
            case CL_COM_READ_INIT_CRM:
               return "CL_COM_READ_INIT_CRM";
            case CL_COM_READ_SEND_CRM:
               return "CL_COM_READ_SEND_CRM";
            case CL_COM_SEND_INIT:
               return "CL_COM_SEND_INIT";
            case CL_COM_SEND_CM:
               return "CL_COM_SEND_CM";
            case CL_COM_SEND_READ_GMSH:
               return "CL_COM_SEND_READ_GMSH";
            case CL_COM_SEND_READ_CRM:
               return "CL_COM_SEND_READ_CRM";
            default:
               return "UNEXPECTED CONNECTION SUB STATE";
         }
      }
   }
   CL_LOG(CL_LOG_ERROR,"undefined marked to close flag type");
   return "UNEXPECTED CONNECTION SUB STATE";
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

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_ignore_timeouts()"
void cl_com_ignore_timeouts(cl_bool_t flag) {
   cl_ingore_timeout = flag;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_ignore_timeouts_flag()"
cl_bool_t cl_com_get_ignore_timeouts_flag(void) {
    if ( cl_ingore_timeout == CL_TRUE ) {
       CL_LOG(CL_LOG_WARNING,"ignoring all communication timeouts");
    }
    return cl_ingore_timeout;
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
int cl_com_open_connection(cl_com_connection_t* connection, int timeout, cl_com_endpoint_t* remote_endpoint, cl_com_endpoint_t* local_endpoint, cl_com_endpoint_t* receiver_endpoint ,cl_com_endpoint_t* sender_endpoint) {
   int retval = CL_RETVAL_UNKNOWN;

   /* check parameters and duplicate */
   if (connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
      return CL_RETVAL_PARAMS;
   }
   
   if (connection->connection_state != CL_DISCONNECTED &&
       connection->connection_state != CL_OPENING) {
      CL_LOG(CL_LOG_ERROR,"unexpected connection state");
      return CL_RETVAL_CONNECTION_STATE_ERROR;
   }

   /* starting this function the first time */
   if (connection->connection_state == CL_DISCONNECTED) {
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

      connection->connection_state = CL_OPENING;   
      connection->connection_sub_state = CL_COM_OPEN_INIT;
      connection->was_opened = CL_TRUE;
   }

   /* try to connect (open connection) */
   if (connection->connection_state == CL_OPENING) {
      int connect_port = 0;
      int tcp_port = 0;
      int connection_count_reached = 0;
      cl_xml_connection_autoclose_t autoclose = CL_CM_AC_UNDEFINED;

      switch(connection->framework_type) {
         case CL_CT_TCP: {
            /* get connect port of connection */
            if ((retval = cl_com_tcp_get_connect_port(connection,&connect_port)) != CL_RETVAL_OK) {
               return retval;
            }

            connection->connection_type = CL_COM_SEND_RECEIVE;

            if ( connect_port <= 0 ) {
               if ( cl_com_get_known_endpoint_port(connection->remote, &tcp_port) == CL_RETVAL_OK) {
                  if ((retval=cl_com_tcp_set_connect_port(connection, tcp_port)) != CL_RETVAL_OK) {
                     return retval;
                  }
               } else {
                  CL_LOG(CL_LOG_ERROR,"endpoint port not found");
               }
 
               CL_LOG(CL_LOG_WARNING,"try to find out autoclose mode of endpoint");
               if ( cl_com_get_known_endpoint_autoclose_mode(connection->remote, &autoclose) == CL_RETVAL_OK) {
                  if ( autoclose == CL_CM_AC_ENABLED ) {
                     connection->auto_close_type = autoclose; 
                  }
                  switch ( connection->auto_close_type ) {
                     case CL_CM_AC_ENABLED:
                        CL_LOG(CL_LOG_INFO,"autoclose is enabled");
                        break;
                     case CL_CM_AC_DISABLED:
                        CL_LOG(CL_LOG_INFO,"autoclose is disabled");
                        break;
                     default:
                        CL_LOG(CL_LOG_INFO,"unexpected autoclose value");
                  }
               } else {
                  CL_LOG(CL_LOG_ERROR,"endpoint not found");
               }
            }

            if ( connection->handler != NULL) {
               /* check if max. connection count is reached */
               if ( connection->handler->max_connection_count_reached != 0 ) {
                  connection_count_reached = 1;
               }
            }
 
            /* open tcp connection when max connecion count is not reached, otherwise
               do not open a tcp connection -> return CL_RETVAL_UNCOMPLETE_WRITE */
            if ( connection_count_reached == 0 ) {
               retval = cl_com_tcp_open_connection(connection,timeout,1);
            } else {
               CL_LOG(CL_LOG_WARNING,cl_get_error_text(CL_RETVAL_MAX_CON_COUNT_REACHED));
               retval = CL_RETVAL_UNCOMPLETE_WRITE; /* wait till connection count is ok */
            }

            if (retval != CL_RETVAL_OK && retval != CL_RETVAL_UNCOMPLETE_WRITE) {
               CL_LOG(CL_LOG_ERROR,"connect error");
               connection->connection_type = CL_COM_UNDEFINED;
            } 
            if (retval == CL_RETVAL_OK) {  /* OK */
               CL_LOG(CL_LOG_INFO,"tcp conected");
               connection->connection_state = CL_CONNECTING;
               connection->connection_sub_state = CL_COM_SEND_INIT;
               connection->data_write_flag = CL_COM_DATA_READY;
            }
            return retval;
         }
      }
      CL_LOG(CL_LOG_ERROR,"undefined framework type");
      retval = CL_RETVAL_UNDEFINED_FRAMEWORK;
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
int cl_com_close_connection(cl_com_connection_t** connection) {
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
         elem = cl_message_list_get_next_elem(elem);
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
         elem = cl_message_list_get_next_elem(elem);
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

      (*connection)->data_flow_type = CL_CM_CT_UNDEFINED;

      free( (*connection)->client_host_name);
      (*connection)->client_host_name = NULL;


      free( (*connection)->crm_state_error);
      (*connection)->crm_state_error = NULL;

      free( (*connection)->statistic );
      (*connection)->statistic = NULL;

      switch((*connection)->framework_type) {
         case CL_CT_TCP: {
            retval = cl_com_tcp_close_connection(connection);
            free(*connection);
            *connection = NULL;
            return retval;
         }
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
#define __CL_FUNCTION__ "cl_com_free_handle_statistic()"
int cl_com_free_handle_statistic(cl_com_handle_statistic_t** statistic) {

   if ( statistic == NULL) {
      return CL_RETVAL_PARAMS; /* no pointer pointer */
   }

   if ( *statistic == NULL ) {
      return CL_RETVAL_PARAMS; /* no memory to free */
   }

   if ( (*statistic)->application_info != NULL ) {
      free( (*statistic)->application_info);
      (*statistic)->application_info = NULL;
   }
   free(*statistic);
   *statistic = NULL;
   return CL_RETVAL_OK;

}


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
   sge_free_hostent( &((*hostent_p)->he) );

#if 0
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
#endif

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
#define __CL_FUNCTION__ "cl_com_get_h_error_string()"
char* cl_com_get_h_error_string(int h_error) {

   if (h_error == HOST_NOT_FOUND) {
      return strdup("h_errno = HOST_NOT_FOUND");
   }
   if (h_error == TRY_AGAIN) {
      return strdup("h_errno = TRY_AGAIN");
   }
   if (h_error == NO_RECOVERY) {
      return strdup("h_errno = NO_RECOVERY");
   }

   if ( NO_DATA == NO_ADDRESS && h_error == NO_DATA) {
      return strdup("h_errno = NO_DATA or NO_ADDRESS");
   } else {
      if (h_error == NO_DATA) {
         return strdup("h_errno = NO_DATA");
      }
      if (h_error == NO_ADDRESS) {
         return strdup("h_errno = NO_ADDRESS");
      }
   }
   return NULL;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_copy_in_addr()"
struct in_addr* cl_com_copy_in_addr(struct in_addr *addr) {
   struct in_addr* copy = NULL;    
   
   if ( addr == NULL) {
      return NULL;
   }

   copy = (struct in_addr*) malloc (sizeof(struct in_addr));
   if (copy != NULL) {
      memcpy((char*) copy , addr, sizeof(struct in_addr));
   }
   return copy;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_copy_hostent()"
cl_com_hostent_t* cl_com_copy_hostent(cl_com_hostent_t* hostent) {
   cl_com_hostent_t* copy = NULL;

   if ( hostent == NULL) {
      return NULL;
   }

   copy = (cl_com_hostent_t*)malloc(sizeof(cl_com_hostent_t));
   if ( copy != NULL) {
      copy->he = NULL;

      if ( hostent->he != NULL) {
         copy->he = sge_copy_hostent(hostent->he);
         if (copy->he == NULL ) {
            CL_LOG(CL_LOG_ERROR,"could not copy hostent structure");
            free( copy );
            return NULL;
         }
      } 
   }
   return copy;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostname()"
int cl_com_gethostname(char **unique_hostname,struct in_addr *copy_addr,struct hostent **he_copy , int* system_error_value) {  /* CR check */

   char localhostname[CL_MAXHOSTNAMELEN_LENGTH + 1];
   int retval = CL_RETVAL_OK;
   
   
   if (gethostname(localhostname,CL_MAXHOSTNAMELEN_LENGTH) != 0) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_LOCAL_HOSTNAME_ERROR));
      if (system_error_value != NULL) {
         *system_error_value = errno;
      }
      return CL_RETVAL_LOCAL_HOSTNAME_ERROR;
   }
   CL_LOG_STR( CL_LOG_DEBUG, "local gethostname() returned: ", localhostname);
   retval = cl_com_cached_gethostbyname(localhostname, unique_hostname, copy_addr, he_copy , system_error_value);
   return retval;
}






#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_gethostbyname()"
static int cl_com_gethostbyname(char *hostname, cl_com_hostent_t **hostent, int* system_error ) {

   struct hostent* he = NULL;
   cl_com_hostent_t *hostent_p = NULL;   

   /* check parameters */
   if (hostent == NULL || hostname == NULL) {
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
#if 0
   hostent_p->he_data_buffer = NULL;
#endif
   
   /* use sge_gethostbyname() */
   he = sge_gethostbyname(hostname, system_error);
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
#define __CL_FUNCTION__ "cl_com_gethostbyaddr()"
static int cl_com_gethostbyaddr(struct in_addr *addr, cl_com_hostent_t **hostent, int* system_error_retval) {

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
#if 0
   hostent_p->he_data_buffer = NULL;
#endif

   /* use sge_gethostbyaddr() */  
   he = sge_gethostbyaddr(addr, system_error_retval);
   
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
#define __CL_FUNCTION__ "cl_com_set_resolve_method()"
int cl_com_set_resolve_method(cl_host_resolve_method_t method, char* local_domain_name) {
   cl_raw_list_t* host_list = NULL;
   cl_host_list_data_t* host_list_data = NULL;

   if (local_domain_name == NULL && method == CL_LONG) {
      CL_LOG(CL_LOG_WARNING,"can't compare short host names without default domain when method is CL_LONG");
   }
   host_list = cl_com_get_host_list();
   if (host_list == NULL) {
      CL_LOG(CL_LOG_WARNING,"communication library setup error");
      return CL_RETVAL_PARAMS;
   }
   cl_raw_list_lock(host_list);
   host_list_data = cl_host_list_get_data(host_list);
   if (host_list_data == NULL) {
      CL_LOG(CL_LOG_ERROR,"communication library setup error for hostlist");
      cl_raw_list_unlock(host_list);
      return CL_RETVAL_RESOLVING_SETUP_ERROR;
   }

   if (local_domain_name != NULL) {
      char* new_domain = strdup(local_domain_name);
      if (new_domain == NULL) {
         cl_raw_list_unlock(host_list);
         return CL_RETVAL_MALLOC;
      }
      /* free old local domain */
      if (host_list_data->local_domain_name != NULL) {
         free(host_list_data->local_domain_name);
      }
      host_list_data->local_domain_name = new_domain;
   } else {
      /* free old local domain */
      if (host_list_data->local_domain_name != NULL) {
         free(host_list_data->local_domain_name);
      }
      host_list_data->local_domain_name = NULL;
   }

   if (host_list_data->local_domain_name != NULL) {
      CL_LOG_STR(CL_LOG_INFO,"using local domain name:", host_list_data->local_domain_name);
   } else {
      CL_LOG(CL_LOG_INFO,"no local domain specified");
   }

   host_list_data->resolve_method = method;
   switch(host_list_data->resolve_method) {
      case CL_SHORT:
         CL_LOG(CL_LOG_WARNING,"using short hostname for host compare operations");
         break;
      case CL_LONG:
         CL_LOG(CL_LOG_WARNING,"using long hostname for host compare operations");
         break;
      default:
         CL_LOG(CL_LOG_WARNING,"undefined resolving method");
         break;
   }
   cl_raw_list_unlock(host_list);
   return CL_RETVAL_OK;
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

    CL_LOG_STR(CL_LOG_DEBUG,"original compare host 1:", host1);
    CL_LOG_STR(CL_LOG_DEBUG,"original compare host 2:", host2);

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

    CL_LOG(CL_LOG_INFO,"cl_com_compare_hosts() is slow!");
    CL_LOG_STR(CL_LOG_DEBUG,"compareing host 1:", hostbuf1);
    CL_LOG_STR(CL_LOG_DEBUG,"compareing host 2:", hostbuf2);
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
#define __CL_FUNCTION__ "cl_com_push_application_error()"
int cl_com_push_application_error(int cl_error, const char* cl_info) {
   return cl_commlib_push_application_error(cl_error,cl_info);      
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_cached_gethostbyname()"
int cl_com_cached_gethostbyname( char *unresolved_host, char **unique_hostname, struct in_addr *copy_addr ,struct hostent **he_copy, int* system_error_value ) {
   cl_host_list_elem_t*    elem = NULL;
   cl_host_list_elem_t*    act_elem = NULL;
   cl_com_host_spec_t*       elem_host = NULL;
   cl_host_list_data_t* ldata = NULL;
   cl_raw_list_t* hostlist = NULL;
   cl_com_hostent_t* myhostent = NULL;
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
      CL_LOG(CL_LOG_ERROR,"no global hostlist, resolving without cache");
      retval = cl_com_gethostbyname(unresolved_host, &myhostent, system_error_value);
      if (retval != CL_RETVAL_OK) {
         cl_com_free_hostent(&myhostent);
         return retval;
      }
      *unique_hostname = strdup(myhostent->he->h_name);
      if (*unique_hostname == NULL) {
         cl_com_free_hostent(&myhostent);
         return CL_RETVAL_MALLOC;
      }
      if (copy_addr != NULL) {
         memcpy((char*) copy_addr, myhostent->he->h_addr, sizeof(struct in_addr));
      }
      if (he_copy != NULL) {
         *he_copy = sge_copy_hostent(myhostent->he);
      }
      cl_com_free_hostent(&myhostent);
      return CL_RETVAL_OK;
   }

   if (hostlist->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_host_list_data_t*) hostlist->list_data;

   if (cl_commlib_get_thread_state() == CL_NO_THREAD || ldata->alias_file_changed != 0) {
      cl_com_host_list_refresh(hostlist);
   } 


#if 0
   /* CR:
    *
    * enable this code for 1:1 mapping or for virtual host mapping 
    * (e.g. my_virtual_hostname real_host_name in alias file ) 
    *
    * DO NOT FORGET TO ALSO ENABLE CODE IN cl_host_alias_list_append_host() 
    */

   if ( cl_host_alias_list_get_local_resolved_name(ldata->host_alias_list,unresolved_host, &alias_name) == CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_INFO,"unresolved host name is aliased to", alias_name);
   }
#endif


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
      elem = cl_host_list_get_next_elem(elem);
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

      retval = cl_com_gethostbyname(hostspec->unresolved_name, &hostent, system_error_value);
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
#define __CL_FUNCTION__ "cl_com_read_alias_file()"
int cl_com_read_alias_file(cl_raw_list_t* hostlist) {
   cl_host_list_data_t* ldata = NULL;
   SGE_STRUCT_STAT sb;
   FILE *fp;
   char alias_file_buffer[LINE_MAX*4];
   int max_line = LINE_MAX*4;
   char* alias_delemiters="\n\t ,;";
   char printbuf[ (2*MAXHOSTLEN) + 100 ];

   if (hostlist == NULL) {
      return CL_RETVAL_PARAMS;    
   }

   
   if (hostlist->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }

   cl_raw_list_lock(hostlist);
   ldata = (cl_host_list_data_t*) hostlist->list_data;
   
   ldata->alias_file_changed = 0;

   if (ldata->host_alias_file == NULL) {
      cl_raw_list_unlock(hostlist);
      CL_LOG(CL_LOG_ERROR,"host alias file is not specified");
      return CL_RETVAL_NO_ALIAS_FILE;
   }
   if (SGE_STAT(ldata->host_alias_file, &sb)) {
      cl_raw_list_unlock(hostlist);
      CL_LOG(CL_LOG_ERROR,"host alias file is not existing");
      return CL_RETVAL_ALIAS_FILE_NOT_FOUND;
   }
   fp = fopen(ldata->host_alias_file, "r");
   if (!fp) {
      cl_raw_list_unlock(hostlist);
      CL_LOG(CL_LOG_ERROR,"can't open host alias file");
      return CL_RETVAL_OPEN_ALIAS_FILE_FAILED;
   }
   
   CL_LOG_INT(CL_LOG_INFO,"max. supported line length:", max_line );
   while (fgets(alias_file_buffer, sizeof(alias_file_buffer), fp)) {
      char* help = NULL;
      char *lasts = NULL;
      char* main_name = NULL;



      help = strrchr(alias_file_buffer,'\r');
      if (help != NULL) {
         help[0] = '\0';
      }
      help = strrchr(alias_file_buffer,'\n');
      if (help != NULL) {
         help[0] = '\0';
      }

      if (alias_file_buffer[0] == '#') {
         CL_LOG_STR(CL_LOG_INFO,"ignoring comment:",alias_file_buffer );
         continue;
      }

      CL_LOG_STR(CL_LOG_INFO,"line:",alias_file_buffer);
      help = strtok_r(alias_file_buffer,alias_delemiters,&lasts);
      if (help != NULL) {
         cl_com_hostent_t* he = NULL;
         if ( cl_com_gethostbyname(help, &he, NULL) == CL_RETVAL_OK) {
            main_name = strdup(help); /* he->he->h_name */
            cl_com_free_hostent(&he);
            if (main_name == NULL) {
               CL_LOG(CL_LOG_ERROR,"malloc() error");
               fclose(fp);
               cl_raw_list_unlock(hostlist);
               return CL_RETVAL_MALLOC;
            }
         } else {
            CL_LOG_STR(CL_LOG_ERROR,"mainname in alias file is not resolveable:", help);
            continue;
         }
         while ( cl_com_remove_host_alias(main_name) == CL_RETVAL_OK);

         while( (help = strtok_r(NULL,alias_delemiters,&lasts)) != NULL ) {
            int retval = cl_com_append_host_alias(help, main_name);
            if (retval == CL_RETVAL_OK) {
               snprintf(printbuf,sizeof(printbuf), "\"%s\" aliased to \"%s\"", help,main_name);
               CL_LOG(CL_LOG_INFO,printbuf);
            }
         }
         free(main_name);
         main_name = NULL;
      }
   }
   fclose(fp);

   cl_raw_list_unlock(hostlist);

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
   int ret_val = CL_RETVAL_OK;

   cl_com_host_spec_t*       elem_host = NULL;

   if (list_p == NULL) {
      return CL_RETVAL_PARAMS;    
   }
   
   gettimeofday(&now,NULL);

   cl_raw_list_lock(list_p);
   if (list_p->list_data == NULL) {
      cl_raw_list_unlock(list_p);
      CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_host_list_data_t*) list_p->list_data;


   if (ldata->alias_file_changed != 0) {
      CL_LOG(CL_LOG_INFO,"host alias file dirty flag is set");
      cl_raw_list_unlock(list_p);
      cl_com_read_alias_file(list_p);
      cl_raw_list_lock(list_p);
      if (list_p->list_data == NULL) {
         cl_raw_list_unlock(list_p);
         CL_LOG( CL_LOG_ERROR, "hostlist not initalized");
         return CL_RETVAL_PARAMS;    
      }
      ldata = (cl_host_list_data_t*) list_p->list_data;
   }


   if ( now.tv_sec == ldata->last_refresh_time) {
      cl_raw_list_unlock(list_p);
      return CL_RETVAL_OK;
   }
   ldata->last_refresh_time = now.tv_sec;

   CL_LOG(CL_LOG_INFO,"checking host entries");
   CL_LOG_INT(CL_LOG_INFO,"number of cached host entries:", (int)cl_raw_list_get_elem_count(list_p));



   elem = cl_host_list_get_first_elem(list_p);
   while(elem != NULL) {
      act_elem = elem;
      elem = cl_host_list_get_next_elem(elem);
      elem_host = act_elem->host_spec;



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
         act_elem = NULL;
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
   }
   cl_raw_list_unlock(list_p);

   if ( resolve_host != 0 ) {
      cl_raw_list_t* host_list_copy = NULL; 
      /* we have to resolve at least one host in this list. Make a copy of this list
         and resolve it, because we don't want to lock the list when hosts are resolved */ 
      CL_LOG(CL_LOG_WARNING,"do a list copy");
      ret_val = cl_host_list_copy(&host_list_copy, list_p );
      if (ret_val == CL_RETVAL_OK ) {
         elem = cl_host_list_get_first_elem(host_list_copy);
         while(elem != NULL) {
            resolve_host = 0;
            act_elem = elem;
            elem = cl_host_list_get_next_elem(elem);
            elem_host = act_elem->host_spec;


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
               int resolve_error = CL_RETVAL_OK;
               cl_com_hostent_t* hostent = NULL;

               if (elem_host->unresolved_name != NULL) {
                  CL_LOG_STR(CL_LOG_INFO,"resolving host:", elem_host->unresolved_name);
                  resolve_error = cl_com_gethostbyname(elem_host->unresolved_name, &hostent, NULL);
                  /* free old entries */
                  cl_com_free_hostent(&(elem_host->hostent));
                  free(elem_host->resolved_name);
                  elem_host->resolved_name = NULL;
                  elem_host->hostent = hostent;
                  elem_host->resolve_error = resolve_error;
                  elem_host->last_resolve_time = now.tv_sec;
                  if (elem_host->hostent != NULL) {
                     elem_host->resolved_name = strdup(elem_host->hostent->he->h_name);
                     if (elem_host->resolved_name == NULL) {
                        cl_raw_list_remove_elem(host_list_copy, act_elem->raw_elem);
                        cl_com_free_hostspec(&elem_host);
                        free(act_elem);
                        CL_LOG(CL_LOG_ERROR,"malloc() error");
                        continue;
                     }
                     CL_LOG_STR(CL_LOG_WARNING,"host resolved as:", elem_host->resolved_name);
                  }
               } else {
                  CL_LOG(CL_LOG_INFO,"resolving addr");
                  resolve_error = cl_com_gethostbyaddr(elem_host->in_addr, &hostent,NULL);
                  /* free old entries */
                  cl_com_free_hostent(&(elem_host->hostent));
                  free(elem_host->resolved_name);
                  elem_host->resolved_name = NULL;
                  elem_host->hostent = hostent;
                  elem_host->resolve_error = resolve_error;
                  elem_host->last_resolve_time = now.tv_sec;
                  if (elem_host->hostent != NULL) {
                     elem_host->resolved_name = strdup(elem_host->hostent->he->h_name);
                     if (elem_host->resolved_name == NULL) {
                        cl_raw_list_remove_elem(host_list_copy, act_elem->raw_elem);
                        cl_com_free_hostspec(&elem_host);
                        free(act_elem);
                        CL_LOG(CL_LOG_ERROR,"malloc() error");
                        continue;
                     }
                     CL_LOG_STR(CL_LOG_WARNING,"host resolved as:", elem_host->resolved_name);
                  }
               }
            }
         }

         /* now we have a up-to-date copy of the original host list */
       
         cl_raw_list_lock(list_p);
        
         /* first remove all entries from original list */
         while( (elem = cl_host_list_get_first_elem(list_p)) ) {
            elem_host = elem->host_spec;
            cl_raw_list_remove_elem(list_p, elem->raw_elem);
            cl_com_free_hostspec(&elem_host);
            free(elem);
            elem = NULL;
         }

         /* now dechain elements from copied list into original list */
         while( (elem = cl_host_list_get_first_elem(host_list_copy)) ) {
            cl_raw_list_dechain_elem(host_list_copy, elem->raw_elem);
            cl_raw_list_append_dechained_elem(list_p, elem->raw_elem);
         } 
         
         cl_raw_list_unlock(list_p);

         CL_LOG(CL_LOG_WARNING,"free list copy");
         ret_val = cl_host_list_cleanup(&host_list_copy); 
      }
   }

   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_endpoint_list_refresh()"
int cl_com_endpoint_list_refresh(cl_raw_list_t* list_p) {
   struct timeval              now;
   cl_endpoint_list_elem_t*    act_elem = NULL;
   cl_endpoint_list_elem_t*    elem = NULL;
   cl_endpoint_list_data_t*    ldata = NULL;

   if (list_p == NULL) {
      return CL_RETVAL_PARAMS;    
   }

   if (list_p->list_data == NULL) {
      CL_LOG( CL_LOG_ERROR, "endpoint list not initalized");
      return CL_RETVAL_PARAMS;    
   }
   ldata = (cl_endpoint_list_data_t*) list_p->list_data;

   gettimeofday(&now,NULL);
   if ( now.tv_sec < ldata->refresh_interval + ldata->last_refresh_time) {
      return CL_RETVAL_OK;
   }

   ldata->last_refresh_time = now.tv_sec;

   CL_LOG_INT(CL_LOG_INFO, "number of endpoint entries:",(int)cl_raw_list_get_elem_count(list_p));
   
   cl_raw_list_lock(list_p);

   elem = cl_endpoint_list_get_first_elem(list_p);
   while(elem != NULL) {
      act_elem = elem;
      elem = cl_endpoint_list_get_next_elem(elem);
  
      /* static elements aren't removed */
      if (act_elem->is_static == 0) {
         if (act_elem->last_used + ldata->entry_life_time < now.tv_sec ) {
            CL_LOG_STR(CL_LOG_INFO,"removing non static element (life timeout) with comp host:", act_elem->endpoint->comp_host);
            cl_raw_list_remove_elem(list_p, act_elem->raw_elem);
            cl_com_free_endpoint(&(act_elem->endpoint));
            free(act_elem);
            act_elem = NULL;
            continue;
         }
      } else {
         CL_LOG_STR(CL_LOG_INFO,"ignoring static element with comp host:", act_elem->endpoint->comp_host);
      }
   }

   cl_raw_list_unlock(list_p);
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_cached_gethostbyaddr()"
int cl_com_cached_gethostbyaddr( struct in_addr *addr, char **unique_hostname,struct hostent **he_copy, int* system_error_val ) {
   cl_host_list_elem_t*    elem = NULL;
   cl_host_list_elem_t*    act_elem = NULL;
   cl_com_host_spec_t*       elem_host = NULL;
   cl_host_list_data_t* ldata = NULL;
   cl_raw_list_t* hostlist = NULL;
   int ret_val = CL_RETVAL_OK;
   char* alias_name = NULL;
   int resolve_name_ok = 0;
   int function_return = CL_RETVAL_GETHOSTADDR_ERROR;
   

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
      retval = cl_com_gethostbyaddr(addr, &hostent, system_error_val);
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


   if (cl_commlib_get_thread_state() == CL_NO_THREAD || ldata->alias_file_changed != 0) {
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
      elem = cl_host_list_get_next_elem(elem);
   }

   if (elem_host != NULL) {
      if (elem_host->resolved_name == NULL) {
         CL_LOG(CL_LOG_INFO,"found addr in cache - not resolveable");
         cl_raw_list_unlock(hostlist);
         return CL_RETVAL_GETHOSTADDR_ERROR;
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
      retval = cl_com_gethostbyaddr(addr, &hostent, system_error_val);
      hostspec->hostent = hostent;
      hostspec->resolve_error = retval;
      gettimeofday(&now,NULL);
      hostspec->last_resolve_time = now.tv_sec;
      hostspec->creation_time = now.tv_sec;
      hostspec->resolved_name = NULL;

      if (hostspec->hostent != NULL) {
         /* CHECK for correct resolving */
         retval = cl_com_cached_gethostbyname( hostent->he->h_name, &hostname , NULL, he_copy, NULL);
         if (retval != CL_RETVAL_OK) {
             CL_LOG_STR(CL_LOG_WARNING,"can't resolve host name", hostent->he->h_name);
             hostspec->resolve_error = CL_RETVAL_GETHOSTADDR_ERROR;
             /* add dummy host entry */
             cl_raw_list_lock(hostlist);
             function_return = cl_host_list_append_host(hostlist, hostspec, 0);
             if (function_return != CL_RETVAL_OK) {
                cl_raw_list_unlock(hostlist);
                cl_com_free_hostspec(&hostspec);
                return function_return;
             }
             cl_raw_list_unlock(hostlist);
             return CL_RETVAL_GETHOSTADDR_ERROR;
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
            if ( strcasecmp(hostname , hostent->he->h_name ) != 0 
              && strcasecmp(hostent->he->h_name,"localhost")!=0 ) {
               resolve_name_ok = 0;
            }
         }

         /* if names are not equal -> report error */ 
         if (resolve_name_ok != 1) {     /* hostname compare OK */

            CL_LOG(CL_LOG_ERROR,"host resolving by address returns not the same host name as resolving by name");
            hostspec->resolve_error = CL_RETVAL_GETHOSTADDR_ERROR;
            /* add dummy host entry */
            cl_raw_list_lock(hostlist);
             function_return = cl_host_list_append_host(hostlist, hostspec, 0);
             if (function_return != CL_RETVAL_OK) {
                cl_raw_list_unlock(hostlist);
                cl_com_free_hostspec(&hostspec);
                return function_return;
             }
             cl_raw_list_unlock(hostlist);
             return CL_RETVAL_GETHOSTADDR_ERROR;
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
         return CL_RETVAL_GETHOSTADDR_ERROR;
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
         case CL_CT_TCP: {
            return cl_com_tcp_send_message( connection, timeout_time, data, size, only_one_write );
         }
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
         case CL_CT_TCP: {
            return cl_com_tcp_receive_message(connection, timeout_time, data_buffer, data_buffer_size, only_one_read);
         }
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
int cl_com_connection_request_handler_setup(cl_com_connection_t* connection,cl_com_endpoint_t* local_endpoint) {
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
         case CL_CT_TCP: {
            retval = cl_com_tcp_connection_request_handler_setup(connection);
            if (retval != CL_RETVAL_OK) {
               /* free endpoint */
               cl_com_free_endpoint(&(connection->local));
               /* reset service handler flag */
               connection->service_handler_flag = CL_COM_SERVICE_UNDEFINED;
            }
            return retval;
         }
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
         case CL_CT_TCP: {
            retval = cl_com_tcp_connection_request_handler(connection,new_connection, sec_param, usec_rest );
            if (*new_connection != NULL && retval == CL_RETVAL_OK) {
               /* setup new cl_com_connection_t */
               (*new_connection)->service_handler_flag = CL_COM_CONNECTION;
               (*new_connection)->connection_state = CL_CONNECTING;
               (*new_connection)->connection_sub_state = CL_COM_READ_INIT;
               (*new_connection)->was_accepted = CL_TRUE;
               (*new_connection)->local = cl_com_create_endpoint(connection->local->comp_host,
                                                                 connection->local->comp_name, 
                                                                 connection->local->comp_id );
               if ( (*new_connection)->local == NULL ) {
                  cl_com_tcp_close_connection(new_connection);
                  retval = CL_RETVAL_MALLOC;
               }
            }
            return retval;
         }
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
         case CL_CT_TCP: {
            return cl_com_tcp_connection_request_handler_cleanup(connection);
         }
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


int cl_com_open_connection_request_handler(cl_framework_t framework_type, cl_raw_list_t* connection_list, cl_com_connection_t* service_connection, int timeout_val_sec, int timeout_val_usec , cl_select_method_t select_mode) {  /* CR check */

   int usec_rest = 0;
   int full_usec_seconds = 0;
   int sec_param = 0;

   usec_rest = timeout_val_usec % 1000000;           /* usec parameter for select should not be > 1000000 !!!*/
   full_usec_seconds = timeout_val_usec / 1000000;   /* full seconds from timeout_val_usec parameter */
   sec_param = timeout_val_sec + full_usec_seconds;  /* add full seconds from usec parameter to timeout_val_sec parameter */

   if (connection_list != NULL) {
      switch(framework_type) {
         case CL_CT_TCP: {
            return cl_com_tcp_open_connection_request_handler(connection_list,service_connection,
                                                              sec_param , usec_rest,select_mode );
         }
      }
   }else {
      CL_LOG(CL_LOG_ERROR,"connection pointer is NULL");
   }

   return CL_RETVAL_UNDEFINED_FRAMEWORK;
}


