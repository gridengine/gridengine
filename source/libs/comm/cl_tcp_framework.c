#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>


#include <netinet/tcp.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <limits.h>


#include "cl_tcp_framework.h"
#include "cl_communication.h"
#include "cl_commlib.h"
#include "cl_message_list.h"
#include "cl_connection_list.h"
#include "cl_util.h"
#include "msg_commlib.h"

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

/* connection specific struct (not used from outside) */
typedef struct cl_com_tcp_private_type {
   /* TCP/IP specific */
   int           server_port;         /* used port for server setup */
   int           connect_port;        /* port to connect to */
   int           connect_in_port;     /* port from where client is connected (used for reserved port check) */
   int           sockfd;              /* socket file descriptor */
   struct sockaddr_in client_addr;    /* used in connect for storing client addr of connection partner */ 
} cl_com_tcp_private_t;


static cl_com_tcp_private_t* cl_com_tcp_get_private(cl_com_connection_t* connection);
static int cl_com_tcp_free_com_private(cl_com_connection_t* connection);



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_get_fd()"
int cl_com_tcp_get_fd(cl_com_connection_t* connection, int* fd) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL || fd == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
      *fd = private->sockfd;
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_get_service_port()"
int cl_com_tcp_get_service_port(cl_com_connection_t* connection, int* port) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL || port == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
      *port = private->server_port;
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_get_client_socket_in_port()"
int cl_com_tcp_get_client_socket_in_port(cl_com_connection_t* connection, int* port) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL || port == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
      *port = private->connect_in_port;
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_get_connect_port()"
int cl_com_tcp_get_connect_port(cl_com_connection_t* connection, int* port) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL || port == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
      *port = private->connect_port;
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_set_connect_port()"
int cl_com_tcp_set_connect_port(cl_com_connection_t* connection, int port) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
      private->connect_port = port;
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_dump_tcp_private()"
void cl_dump_tcp_private(cl_com_connection_t* connection) {
   cl_com_tcp_private_t* private = NULL;
   if (connection == NULL) {
      CL_LOG(CL_LOG_DEBUG, "connection is NULL");
   } else {
      if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
         CL_LOG_INT(CL_LOG_DEBUG,"server port:",private->server_port);
         CL_LOG_INT(CL_LOG_DEBUG,"connect_port:",private->connect_port);
         CL_LOG_INT(CL_LOG_DEBUG,"socked fd:",private->sockfd);
      }
   }
}



/****** cl_tcp_framework/cl_com_tcp_open_connection() **************************
*  NAME
*     cl_com_tcp_open_connection() -- open a tcp/ip connection
*
*  SYNOPSIS
*     int cl_com_tcp_open_connection(cl_com_connection_t* connection, const 
*     char* comp_host, const char* comp_name, int comp_id, int timeout) 
*
*  FUNCTION
*     This function will create a new socket file descriptor and set the 
*     SO_REUSEADDR socket option and the O_NONBLOCK file descriptor flag. 
*     After that the socket will try to connect the service handler on the
*     given connect_port (set with cl_com_tcp_setup_connection()) on the
*     host specified with comp_host. After a successful connect the 
*     TCP_NODELAY socket option is set. 
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to a connection struct
*     const char* comp_host           - host where a service is available
*     const char* comp_name           - component name of service
*     int comp_id                     - component id of service
*     int timeout                     - timeout for connect
*
*  RESULT
*     int - CL_COMM_XXXX error value or CL_RETVAL_OK for no errors
*
*  SEE ALSO
*     cl_communication/cl_com_open_connection()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_open_connection()"
int cl_com_tcp_open_connection(cl_com_connection_t* connection, int timeout, unsigned long only_once) {
   cl_com_tcp_private_t* private = NULL;
   
   int tmp_error = CL_RETVAL_OK;
   char tmp_buffer[256];


   if (connection == NULL) { 
      return  CL_RETVAL_PARAMS;
   }
   if (connection->remote == NULL || connection->local == NULL || connection->receiver == NULL || connection->sender == NULL) {
      return CL_RETVAL_PARAMS;
   }

   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if ( private->connect_port <= 0 ) {
      CL_LOG(CL_LOG_ERROR, cl_get_error_text(CL_RETVAL_NO_PORT_ERROR));
      return CL_RETVAL_NO_PORT_ERROR; 
   }

   if ( connection->connection_state != CL_OPENING ) {
      CL_LOG(CL_LOG_ERROR,"state is not CL_OPENING - return connect error");
      return CL_RETVAL_CONNECT_ERROR;   
   }

   if ( connection->connection_sub_state == CL_COM_OPEN_INIT) {
      int on = 1;
      char* unique_host = NULL;
      struct timeval now;
      int res_port = IPPORT_RESERVED -1;

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_INIT");
      private->sockfd = -1;
      
      switch(connection->tcp_connect_mode) {
         case CL_TCP_DEFAULT: {
            /* create socket */
            if ((private->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
               CL_LOG(CL_LOG_ERROR,"could not create socket");
               private->sockfd = -1;
               cl_commlib_push_application_error(CL_RETVAL_CREATE_SOCKET, MSG_CL_TCP_FW_SOCKET_ERROR );
               return CL_RETVAL_CREATE_SOCKET;
            }
            break;
         }

         case CL_TCP_RESERVED_PORT: {
            /* create reserved port socket */
            if ((private->sockfd = rresvport(&res_port)) < 0) {
               CL_LOG(CL_LOG_ERROR,"could not create reserved port socket");
               private->sockfd = -1;
               cl_commlib_push_application_error(CL_RETVAL_CREATE_SOCKET, MSG_CL_TCP_FW_RESERVED_SOCKET_ERROR );
               return CL_RETVAL_CREATE_RESERVED_PORT_SOCKET;
            }
            break;
         }
      }    

      /* set local address reuse socket option */
      if ( setsockopt(private->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set SO_REUSEADDR");
         private->sockfd = -1;
         cl_commlib_push_application_error(CL_RETVAL_SETSOCKOPT_ERROR, MSG_CL_TCP_FW_SETSOCKOPT_ERROR);
         return CL_RETVAL_SETSOCKOPT_ERROR;
      }
   
      /* this is a non blocking socket */
      if ( fcntl(private->sockfd, F_SETFL, O_NONBLOCK) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set O_NONBLOCK");
         private->sockfd = -1;
         cl_commlib_push_application_error(CL_RETVAL_FCNTL_ERROR, MSG_CL_TCP_FW_FCNTL_ERROR);
         return CL_RETVAL_FCNTL_ERROR;
      }


      /* set address  */
      memset((char *) &(private->client_addr), 0, sizeof(struct sockaddr_in));
      private->client_addr.sin_port = htons(private->connect_port);
      private->client_addr.sin_family = AF_INET;
      if ( (tmp_error=cl_com_cached_gethostbyname(connection->remote->comp_host, &unique_host, &(private->client_addr.sin_addr),NULL , NULL)) != CL_RETVAL_OK) {
   
         shutdown(private->sockfd, 2);
         close(private->sockfd);
         free(unique_host);
         CL_LOG(CL_LOG_ERROR,"could not get hostname");
         private->sockfd = -1;
         
         if ( connection != NULL && connection->remote != NULL && connection->remote->comp_host != NULL) {
            snprintf(tmp_buffer,256, MSG_CL_TCP_FW_CANT_RESOLVE_HOST_S, connection->remote->comp_host );
         } else {
            snprintf(tmp_buffer,256, "%s", cl_get_error_text(tmp_error));
         }
         cl_commlib_push_application_error(tmp_error, tmp_buffer);
         return tmp_error; 
      } 
      free(unique_host);

      /* connect */
      gettimeofday(&now,NULL);
      connection->write_buffer_timeout_time = now.tv_sec + timeout;
      connection->connection_sub_state = CL_COM_OPEN_CONNECT;
   }
   
   if ( connection->connection_sub_state == CL_COM_OPEN_CONNECT) {
      int my_error;
      int i;
      cl_bool_t connect_state = CL_FALSE;

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_CONNECT");

      errno = 0;
      i = connect(private->sockfd, (struct sockaddr *) &(private->client_addr), sizeof(struct sockaddr_in));
      my_error = errno;
      if (i == 0) {
         /* we are connected */
         connect_state = CL_TRUE;
      } else {
         switch(my_error) {
            case EISCONN: {
               CL_LOG(CL_LOG_INFO,"allready connected");
               connect_state = CL_TRUE;
               break;
            }
            case ECONNREFUSED: {
               /* can't open connection */
               CL_LOG_INT(CL_LOG_ERROR,"connection refused to port ",private->connect_port);
               shutdown(private->sockfd, 2);
               close(private->sockfd);
               private->sockfd = -1;
               cl_commlib_push_application_error(CL_RETVAL_CONNECT_ERROR, MSG_CL_TCP_FW_CONNECTION_REFUSED );
               return CL_RETVAL_CONNECT_ERROR;
            }
            case EADDRNOTAVAIL: {
               /* can't open connection */
               CL_LOG_INT(CL_LOG_ERROR,"address not available for port ",private->connect_port);
               shutdown(private->sockfd, 2);
               close(private->sockfd);
               private->sockfd = -1;
               cl_commlib_push_application_error(CL_RETVAL_CONNECT_ERROR, MSG_CL_TCP_FW_CANT_ASSIGN_ADDRESS );
               return CL_RETVAL_CONNECT_ERROR;
            }
            case EINPROGRESS:
            case EALREADY: {
               connection->connection_sub_state = CL_COM_OPEN_CONNECT_IN_PROGRESS;
               if (only_once != 0) {
                  return CL_RETVAL_UNCOMPLETE_WRITE;
               }
               break;
            }
            default: {
               /* we have an connect error */
               CL_LOG_INT(CL_LOG_ERROR,"connect error errno:", my_error);
               shutdown(private->sockfd, 2);
               close(private->sockfd);
               private->sockfd = -1;
               snprintf(tmp_buffer, 256, MSG_CL_TCP_FW_CONNECT_ERROR_U, u32c(my_error));
               cl_commlib_push_application_error(CL_RETVAL_CONNECT_ERROR, tmp_buffer);
               return CL_RETVAL_CONNECT_ERROR;
            } 
         }
      }
      if (connect_state == CL_TRUE) {
         connection->write_buffer_timeout_time = 0;
         connection->connection_sub_state = CL_COM_OPEN_CONNECTED;
      }
   }

   if ( connection->connection_sub_state == CL_COM_OPEN_CONNECT_IN_PROGRESS ) {
      int    do_stop      = 0;
      fd_set writefds;
      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_CONNECT_IN_PROGRESS");

      while (do_stop == 0) {
         int select_back = 0;
         struct timeval now;
         struct timeval stimeout;
         int socket_error = 0;
         int socklen = sizeof(socket_error);

         if (only_once == 0) {
            FD_ZERO(&writefds);
            FD_SET(private->sockfd, &writefds);
            stimeout.tv_sec = 0; 
            stimeout.tv_usec = 250*1000;   /* 1/4 sec */

            select_back = select(private->sockfd + 1, NULL, &writefds, NULL, &stimeout);
            
            if (select_back < 0) {
               CL_LOG(CL_LOG_ERROR,"select error");
               cl_commlib_push_application_error(CL_RETVAL_SELECT_ERROR, MSG_CL_TCP_FW_SELECT_ERROR);
               return CL_RETVAL_SELECT_ERROR;
            }
         }

#if defined(SOLARIS) && !defined(SOLARIS64) 
         getsockopt(private->sockfd,SOL_SOCKET, SO_ERROR, (void*)&socket_error, &socklen);
#else
         getsockopt(private->sockfd,SOL_SOCKET, SO_ERROR, &socket_error, &socklen);
#endif
         if (socket_error == 0 || socket_error == EISCONN) {
            CL_LOG(CL_LOG_INFO,"connected");
            connection->write_buffer_timeout_time = 0;
            connection->connection_sub_state = CL_COM_OPEN_CONNECTED;
            break; /* we are connected */
         } else {
            if (socket_error != EINPROGRESS && socket_error != EALREADY) {
               CL_LOG_INT(CL_LOG_ERROR,"socket error errno:", socket_error);
               shutdown(private->sockfd, 2);
               close(private->sockfd);
               private->sockfd = -1;
               snprintf(tmp_buffer, 256, MSG_CL_TCP_FW_SOCKET_ERROR_U, u32c(socket_error));
               cl_commlib_push_application_error(CL_RETVAL_CONNECT_ERROR, tmp_buffer);
               return CL_RETVAL_CONNECT_ERROR;
            }
         } 

         gettimeofday(&now,NULL);
         if (connection->write_buffer_timeout_time <= now.tv_sec || 
             cl_com_get_ignore_timeouts_flag()     == CL_TRUE       ) {

            /* we had an timeout */
            CL_LOG(CL_LOG_ERROR,"connect timeout error");
            connection->write_buffer_timeout_time = 0;
            shutdown(private->sockfd, 2);
            close(private->sockfd);
            private->sockfd = -1;
            cl_commlib_push_application_error(CL_RETVAL_CONNECT_TIMEOUT, MSG_CL_TCP_FW_CONNECT_TIMEOUT );
            return CL_RETVAL_CONNECT_TIMEOUT;
         }

         if (only_once != 0) {
            return CL_RETVAL_UNCOMPLETE_WRITE;
         }
      }  /* while do_stop */
   }

   if ( connection->connection_sub_state == CL_COM_OPEN_CONNECTED) {
      int on = 1; 

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_CONNECTED");

  
#if defined(SOLARIS) && !defined(SOLARIS64)
      if (setsockopt(private->sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &on, sizeof(int)) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
      } 
#else
      if (setsockopt(private->sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int))!= 0) {
         CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
      }
#endif
      return CL_RETVAL_OK;
   }

   return CL_RETVAL_UNKNOWN;
}



/****** cl_communication/cl_com_tcp_setup_connection() *************************
*  NAME
*     cl_com_tcp_setup_connection() -- setup a connection type
*
*  SYNOPSIS
*     int cl_com_tcp_setup_connection(cl_com_connection_t* connection, int 
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
*     is released by a call to cl_com_tcp_close_connection()
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
#define __CL_FUNCTION__ "cl_com_tcp_setup_connection()"
int cl_com_tcp_setup_connection(cl_com_connection_t**          connection,
                                int                            server_port,
                                int                            connect_port,
                                cl_xml_connection_type_t       data_flow_type,
                                cl_xml_connection_autoclose_t  auto_close_mode,
                                cl_framework_t                 framework_type,
                                cl_xml_data_format_t           data_format_type,
                                cl_tcp_connect_t               tcp_connect_mode) {
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

   /* create new connection */
   if ( (ret_val=cl_com_create_connection(connection)) != CL_RETVAL_OK) {
      return ret_val;
   }

   /* check for correct framework specification */
   switch(framework_type) {
      case CL_CT_TCP:
         break;
      case CL_CT_UNDEFINED:
      case CL_CT_SSL: {
         CL_LOG_STR(CL_LOG_ERROR,"unexpected framework:", cl_com_get_framework_type(*connection));
         cl_com_close_connection(connection);
         return CL_RETVAL_WRONG_FRAMEWORK;
      }
   }

   /* create private data structure */
   com_private = (cl_com_tcp_private_t*) malloc(sizeof(cl_com_tcp_private_t));
   if (com_private == NULL) {
      cl_com_close_connection(connection);
      return CL_RETVAL_MALLOC;
   }
   memset(com_private, 0, sizeof(cl_com_tcp_private_t));


   /* set com_private to com_private pointer */
   (*connection)->com_private = com_private;

   /* set modes */
   (*connection)->auto_close_type = auto_close_mode;
   (*connection)->data_flow_type = data_flow_type;
   (*connection)->connection_type = CL_COM_SEND_RECEIVE;
   (*connection)->framework_type = framework_type;
   (*connection)->data_format_type = data_format_type;
   (*connection)->tcp_connect_mode = tcp_connect_mode;

   /* setup tcp private struct */
   com_private->sockfd = -1;
   com_private->server_port = server_port;
   com_private->connect_port = connect_port;
   return CL_RETVAL_OK;
}


/****** cl_tcp_framework/cl_com_tcp_free_com_private() *************************
*  NAME
*     cl_com_tcp_free_com_private() -- free private struct of a tcp connection
*
*  SYNOPSIS
*     int cl_com_tcp_free_com_private(cl_com_connection_t* connection) 
*
*  FUNCTION
*     This function will free the com_private struct pointer of a tcp connection
*     struct
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to tcp/ip connection
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_free_com_private()"
static int cl_com_tcp_free_com_private(cl_com_connection_t* connection) {
   cl_com_tcp_private_t* private = NULL;



   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   private->server_port = -1;
   private->connect_port = -1;
   private->sockfd = -1;

   /* free struct cl_com_tcp_private_t */
   free(private);
   connection->com_private = NULL;
   return CL_RETVAL_OK;
}


/****** cl_tcp_framework/cl_com_tcp_close_connection() *************************
*  NAME
*     cl_com_tcp_close_connection() -- close and shutdown a tcp connection
*
*  SYNOPSIS
*     int cl_com_tcp_close_connection(cl_com_connection_t* connection) 
*
*  FUNCTION
*     This function will shutdown and close the connection (if open) and free
*     the connection->com_private pointer for a tcp connection.
*
*  INPUTS
*     cl_com_connection_t* connection - connection pointer
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_communication/cl_com_close_connection()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_close_connection()"
int cl_com_tcp_close_connection(cl_com_connection_t** connection) {
   cl_com_tcp_private_t* private = NULL;

   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   private = cl_com_tcp_get_private(*connection);

   if (private == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if (private->sockfd >= 0) {
      CL_LOG(CL_LOG_INFO,"closing connection");
      /* shutdown socket connection */
      shutdown(private->sockfd, 2);
      close(private->sockfd);
      private->sockfd = -1;
   }
 
   /* free com private structure */
   return cl_com_tcp_free_com_private(*connection);
}




/****** cl_tcp_framework/cl_com_tcp_write() ************************************
*  NAME
*     cl_com_tcp_write() -- ??? 
*
*  SYNOPSIS
*     static cl_com_tcp_write(long timeout_time, int fd, cl_byte_t* message, 
*     long size) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     long timeout_time  - ??? 
*     int fd             - ??? 
*     cl_byte_t* message - ??? 
*     long size          - ??? 
*
*  RESULT
*     static - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     ??? 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_write()"
int cl_com_tcp_write(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long *only_one_write) {
   struct timeval now;
   long data_written = 0;
   long data_complete = 0;
   int my_errno;
   fd_set writefds;
   int select_back = 0;
   struct timeval timeout;


   if ( message == NULL) {
      CL_LOG(CL_LOG_ERROR,"no message to write");
      return CL_RETVAL_PARAMS;
   }
   
   if ( size == 0 ) {
      CL_LOG(CL_LOG_ERROR,"data size is zero");
      return CL_RETVAL_PARAMS;
   }

   if (fd < 0) {
      CL_LOG(CL_LOG_ERROR,"no file descriptor");
      return CL_RETVAL_PARAMS;
   }
   /*
    * INFO: this can be a boddle neck if only_one_write is not set,
    * because if the message can't be read complete, we must try it later 
    */

   while ( data_complete != size ) {
      if (only_one_write == NULL) {
         FD_ZERO(&writefds);
         FD_SET(fd, &writefds);
         timeout.tv_sec = 1; 
         timeout.tv_usec = 0;  /* 0 ms */
         /* do select */
         select_back = select(fd + 1, NULL, &writefds, NULL , &timeout);
   
         if (select_back == -1) {
            CL_LOG(CL_LOG_INFO,"select error");
            return CL_RETVAL_SELECT_ERROR;
         }
   
         if (FD_ISSET(fd, &writefds)) {
            errno = 0;
            data_written = write(fd, &message[data_complete], size - data_complete );   
            my_errno = errno;
            if (data_written < 0) {
               if (my_errno == EPIPE) {
                  CL_LOG(CL_LOG_ERROR,"pipe error");
                  return CL_RETVAL_PIPE_ERROR;
               }
               CL_LOG(CL_LOG_ERROR,"send error");
               return CL_RETVAL_SEND_ERROR;
            } else {
               data_complete = data_complete + data_written;
            }
         }
         if (data_complete != size) {
            gettimeofday(&now,NULL);
            if ( now.tv_sec >= timeout_time ) {
               CL_LOG(CL_LOG_ERROR,"send timeout error");
               return CL_RETVAL_SEND_TIMEOUT;
            }
         } else {
            break;
         }
      } else {
         errno = 0;
         data_written = write(fd, &message[data_complete], size - data_complete );   
         my_errno = errno;
         if (data_written < 0) {
            if (my_errno != EWOULDBLOCK && my_errno != EAGAIN && my_errno != EINTR) {
               if (my_errno == EPIPE) {
                  CL_LOG(CL_LOG_ERROR,"pipe error");
                  return CL_RETVAL_PIPE_ERROR;
               }
               CL_LOG(CL_LOG_ERROR,"send error");
               return CL_RETVAL_SEND_ERROR;
            }
         } else {
            data_complete = data_complete + data_written;
         }
         *only_one_write = data_complete;
         if (data_complete != size) {
            gettimeofday(&now,NULL);
            if ( now.tv_sec >= timeout_time ) {
               CL_LOG(CL_LOG_ERROR,"send timeout error");
               return CL_RETVAL_SEND_TIMEOUT;
            }
            return CL_RETVAL_UNCOMPLETE_WRITE;
         }
         return CL_RETVAL_OK;
      }
   } /* while */
   return CL_RETVAL_OK;
}

/****** cl_tcp_framework/cl_com_tcp_read() *************************************
*  NAME
*     cl_com_tcp_read() -- ??? 
*
*  SYNOPSIS
*     static cl_com_tcp_read(long timeout_time, int fd, cl_byte_t* message, 
*     long size) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     long timeout_time  - ??? 
*     int fd             - ??? 
*     cl_byte_t* message - ??? 
*     long size          - ??? 
*     int* only_one_read - if not NULL: read only once and save data count
*                          into this variable.
*
*  RESULT
*     static - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     ??? 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_read()"
int cl_com_tcp_read(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long* only_one_read) {
   struct timeval now;
   long data_read = 0;
   long data_complete = 0;
   int my_errno;
   int select_back = 0;
   fd_set readfds;
   struct timeval timeout;

   if (message == NULL) {
      CL_LOG(CL_LOG_ERROR,"no message buffer");
      return CL_RETVAL_PARAMS;
   }

   if (fd < 0) {
      CL_LOG(CL_LOG_ERROR,"no file descriptor");
      return CL_RETVAL_PARAMS;
   }


   if (size == 0) {
      CL_LOG(CL_LOG_ERROR,"no data size");
      return CL_RETVAL_PARAMS;
   }

   if (size > CL_DEFINE_MAX_MESSAGE_LENGTH) {
      CL_LOG_INT(CL_LOG_ERROR,"data to read is > max message length =", CL_DEFINE_MAX_MESSAGE_LENGTH );
      return CL_RETVAL_MAX_READ_SIZE;
   }

   /* TODO: this is a boddle neck if only_one_read is not set.
            because if the message can't be read
            complete, we must try it later !!!!!!!!!!!!!!! */


   while ( data_complete != size ) {
      if (only_one_read == NULL) {
         FD_ZERO(&readfds);
         FD_SET(fd, &readfds);
         timeout.tv_sec = 1; 
         timeout.tv_usec = 0;  /* 0 ms */
   
         /* do select */
         select_back = select(fd + 1, &readfds,NULL , NULL , &timeout);
         if (select_back == -1) {
            CL_LOG(CL_LOG_INFO,"select error");
            return CL_RETVAL_SELECT_ERROR;
         }
         
         if (FD_ISSET(fd, &readfds)) {
            errno = 0;
            data_read = read(fd, &message[data_complete], size - data_complete );
            my_errno = errno;
            if (data_read <= 0) {
               if (data_read == 0) {
                  /* this should only happen if the connection is down */
                  CL_LOG(CL_LOG_WARNING,"client connection disconnected");
                  return CL_RETVAL_READ_ERROR;
               }
               if (my_errno == EPIPE) {
                  CL_LOG_INT(CL_LOG_ERROR,"pipe error errno:", my_errno );
                  return CL_RETVAL_PIPE_ERROR;
               }
               CL_LOG_INT(CL_LOG_ERROR,"receive error errno:", my_errno);
               return CL_RETVAL_RECEIVE_ERROR;
            } else {
               data_complete = data_complete + data_read;
            }
         }
         if (data_complete != size) {
            gettimeofday(&now,NULL);
            if ( now.tv_sec >= timeout_time ) {
               return CL_RETVAL_READ_TIMEOUT;
            }
         } else {
            break;
         }
      } else {
         errno = 0;
         data_read = read(fd, &message[data_complete], size - data_complete );
         my_errno = errno;
         if (data_read <= 0) {
            if (data_read == 0) {
               /* this should only happen if the connection is down */
               CL_LOG(CL_LOG_WARNING,"client connection disconnected");
               return CL_RETVAL_READ_ERROR;
            }
            if (my_errno != EWOULDBLOCK && my_errno != EAGAIN && my_errno != EINTR) {
               if (my_errno == EPIPE) {
                  CL_LOG_INT(CL_LOG_ERROR,"pipe error errno:", my_errno );
                  return CL_RETVAL_PIPE_ERROR;
               }
               CL_LOG_INT(CL_LOG_ERROR,"receive error errno:", my_errno);
               return CL_RETVAL_RECEIVE_ERROR;
            }
         } else {
            data_complete = data_complete + data_read;
         }
         *only_one_read = data_complete;
         if (data_complete != size) {
            gettimeofday(&now,NULL);
            if ( now.tv_sec >= timeout_time ) {
               return CL_RETVAL_READ_TIMEOUT;
            }
            return CL_RETVAL_UNCOMPLETE_READ;
         }
         return CL_RETVAL_OK;
      }
   }
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_read_GMSH()"
int cl_com_tcp_read_GMSH(cl_com_connection_t* connection, unsigned long *only_one_read) {
   int retval = CL_RETVAL_OK;
   unsigned long data_read = 0;
   cl_com_tcp_private_t* private = NULL;
   unsigned long processed_data = 0;

   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if ( (private=cl_com_tcp_get_private(connection)) == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   /* first read size of gmsh header without data */
   if ( connection->data_read_buffer_pos < CL_GMSH_MESSAGE_SIZE ) {
      if (only_one_read != NULL) {
         data_read = 0;
         retval = cl_com_tcp_read(connection->read_buffer_timeout_time, 
                                  private->sockfd, 
                                  &(connection->data_read_buffer[connection->data_read_buffer_pos]),
                                  CL_GMSH_MESSAGE_SIZE - connection->data_read_buffer_pos,
                                  &data_read);
         connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_read;
         *only_one_read = data_read;
      } else {
         retval = cl_com_tcp_read(connection->read_buffer_timeout_time, 
                               private->sockfd, 
                               connection->data_read_buffer,
                               CL_GMSH_MESSAGE_SIZE ,
                               NULL);
         connection->data_read_buffer_pos = connection->data_read_buffer_pos + CL_GMSH_MESSAGE_SIZE;
      }
      if ( retval != CL_RETVAL_OK) {
         CL_LOG_STR(CL_LOG_INFO,"uncomplete read:", cl_get_error_text(retval));
         return retval;
      }
   }

   /* now read complete header */
   while ( connection->data_read_buffer[connection->data_read_buffer_pos - 1] != '>' ) {
      if ( connection->data_read_buffer_pos >= connection->data_buffer_size) {
         CL_LOG(CL_LOG_INFO,"buffer overflow");
         return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
      }
      if (only_one_read != NULL) {
         data_read = 0;
         retval = cl_com_tcp_read(connection->read_buffer_timeout_time, 
                                  private->sockfd, 
                                  &(connection->data_read_buffer[connection->data_read_buffer_pos]),
                                  1,
                                  &data_read);
         connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_read;
         *only_one_read = data_read;
      } else {
         retval = cl_com_tcp_read(connection->read_buffer_timeout_time, 
                                  private->sockfd, 
                                  &(connection->data_read_buffer[connection->data_read_buffer_pos]),
                                  1,
                                  NULL);
         connection->data_read_buffer_pos = connection->data_read_buffer_pos + 1;
      }
      if (retval != CL_RETVAL_OK) {
         CL_LOG(CL_LOG_WARNING,"uncomplete read(2):");
         return retval;
      }
   }

   connection->data_read_buffer[connection->data_read_buffer_pos] = 0;
   /* header should be now complete */
   if ( strcmp((char*)&(connection->data_read_buffer[connection->data_read_buffer_pos - 7]) ,"</gmsh>") != 0) {
      return CL_RETVAL_GMSH_ERROR;
   }
   
   /* parse header */
   retval = cl_xml_parse_GMSH(connection->data_read_buffer, connection->data_read_buffer_pos, connection->read_gmsh_header, &processed_data);
   connection->data_read_buffer_processed = connection->data_read_buffer_processed + processed_data ;
   if ( connection->read_gmsh_header->dl == 0) {
      CL_LOG(CL_LOG_ERROR,"gmsh header has dl=0 entry");
      return CL_RETVAL_GMSH_ERROR;
   }
   if ( connection->read_gmsh_header->dl > CL_DEFINE_MAX_MESSAGE_LENGTH ) {
      CL_LOG(CL_LOG_ERROR,"gmsh header dl entry is larger than CL_DEFINE_MAX_MESSAGE_LENGTH");
      return CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR;
   }
   return retval;
}





/****** cl_tcp_framework/cl_com_tcp_send_message() *****************************
*  NAME
*     cl_com_tcp_send_message() -- send a message over an open tcp connection
*
*  SYNOPSIS
*     int cl_com_tcp_send_message(cl_com_connection_t* connection, int timeout, 
*     cl_byte_t* data, long size) 
*
*  FUNCTION
*     This function will send size bytes from data buffer. When the connection
*     data flow type is set to CL_CM_CT_MESSAGE, the first byte sent is the size
*     of the message. After that the message is send. When the data flow type
*     is set to CL_CM_CT_STREAM the message is directly sent.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to open connection struct
*     int timeout                     - timeout for sending the message
*     cl_byte_t* data                 - data to send
*     long size                       - length of data in bytes
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_receive_message()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_send_message()"
int cl_com_tcp_send_message(cl_com_connection_t* connection, int timeout_time, cl_byte_t* data , unsigned long size , unsigned long *only_one_write) {
   cl_com_tcp_private_t* private = NULL;

   if (connection == NULL || data == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no data");
      return CL_RETVAL_PARAMS;
   }
   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   return cl_com_tcp_write(timeout_time, private->sockfd, data, size, only_one_write);
}

/****** cl_tcp_framework/cl_com_tcp_receive_message() **************************
*  NAME
*     cl_com_tcp_receive_message() -- get a message from an open tcp connection
*
*  SYNOPSIS
*     int cl_com_tcp_receive_message(cl_com_connection_t* connection, int 
*     timeout, cl_byte_t** data) 
*
*  FUNCTION
*     This function will read data from an open tcp connection. If the data 
*     flow type of the connection is set to CL_CM_CT_MESSAGE then the first
*     data byte read from the connection is the length of the message. After
*     that a data buffer is requested and the full message is read and stored
*     into that buffer. The data parameter will get the memory address from
*     the buffer. The user has to free the memory if not used anymore.
*
*     If the data flow type of the connection is set to CL_CM_CT_STREAM only
*     one byte is read from the open connection. The rest is qual to the
*     CL_CM_CT_MESSAGE data flow type.
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to open connection struct
*     int timeout                     - timeout for receive the message 
*     cl_byte_t** data                - address of an pointer to cl_byte_t
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_send_message()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_receive_message()"
int cl_com_tcp_receive_message(cl_com_connection_t* connection,
                               int                  timeout_time,
                               cl_byte_t*           data_buffer,
                               unsigned long        data_buffer_size,
                               unsigned long*       only_one_read) {

   cl_com_tcp_private_t* private = NULL;
   
   if (connection == NULL || data_buffer == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no data buffer");
      return CL_RETVAL_PARAMS;
   }
  
   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   return cl_com_tcp_read(timeout_time, private->sockfd, data_buffer, data_buffer_size, only_one_read);
}


/****** cl_tcp_framework/cl_com_tcp_connection_request_handler_setup() *********
*  NAME
*     cl_com_tcp_connection_request_handler_setup() -- bind tcp/ip socket
*
*  SYNOPSIS
*     int cl_com_tcp_connection_request_handler_setup(cl_com_connection_t* 
*     connection) 
*
*  FUNCTION
*     This function creates a new stream socket and sets SO_REUSEADDR socket
*     option. After that the socket is bind to the server_port. A final listen
*     enables connection requests on that socket. 
*
*  INPUTS
*     cl_com_connection_t* connection - pointer to connection
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success 
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_connection_request_handler()
*     cl_tcp_framework/cl_com_tcp_connection_request_handler_cleanup()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_connection_request_handler_setup()"
int cl_com_tcp_connection_request_handler_setup(cl_com_connection_t* connection ) {
   int sockfd = 0;
   struct sockaddr_in serv_addr;
   cl_com_tcp_private_t* private = NULL;

   CL_LOG(CL_LOG_INFO,"setting up TCP request handler ...");
    
   if (connection == NULL ) {
      CL_LOG(CL_LOG_ERROR,"no connection");
      return CL_RETVAL_PARAMS;
   }

   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if ( private->server_port < 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_NO_PORT_ERROR));
      return CL_RETVAL_NO_PORT_ERROR;
   }

   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      CL_LOG(CL_LOG_ERROR,"could not create socket");
      return CL_RETVAL_CREATE_SOCKET;
   }

   {
      int on = 1;

      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set SO_REUSEADDR");
         return CL_RETVAL_SETSOCKOPT_ERROR;
      }
   }

   /* bind an address to socket */
   /* TODO FEATURE: we can also try to use a specified port range */
   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_port = htons(private->server_port);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      shutdown(sockfd, 2);
      close(sockfd);
      CL_LOG_INT(CL_LOG_ERROR, "could not bind server socket port:", private->server_port);
      return CL_RETVAL_BIND_SOCKET;
   }

   if (private->server_port == 0) {
#if defined(AIX43) || defined(AIX51)
      size_t length;
#else
      int length;
#endif
      length = sizeof(serv_addr);
      /* find out assigned port number and pass it to caller */
      if (getsockname(sockfd,(struct sockaddr *) &serv_addr, &length ) == -1) {
         shutdown(sockfd, 2);
         close(sockfd);
         CL_LOG_INT(CL_LOG_ERROR, "could not bind random server socket port:", private->server_port);
         return CL_RETVAL_BIND_SOCKET;
      }
      private->server_port = ntohs(serv_addr.sin_port);
      CL_LOG_INT(CL_LOG_INFO,"random server port is:", private->server_port);
   }

   /* make socket listening for incoming connects */
   if (listen(sockfd, 5) != 0) {   /* TODO: set listen params */
      shutdown(sockfd, 2);
      close(sockfd);
      CL_LOG(CL_LOG_ERROR,"listen error");
      return CL_RETVAL_LISTEN_ERROR;
   }
   CL_LOG_INT(CL_LOG_INFO,"listening with backlog=", 5);

   /* set server socked file descriptor and mark connection as service handler */
   private->sockfd = sockfd;

   CL_LOG(CL_LOG_INFO,"===============================");
   CL_LOG(CL_LOG_INFO,"TCP server setup done:");
   CL_LOG_STR(CL_LOG_INFO,"host:     ",connection->local->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"component:",connection->local->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"id:       ",(int)connection->local->comp_id);
   CL_LOG(CL_LOG_INFO,"===============================");
   return CL_RETVAL_OK;
}

/****** cl_tcp_framework/cl_com_tcp_connection_request_handler_cleanup() *******
*  NAME
*     cl_com_tcp_connection_request_handler_cleanup() -- shutdown service
*
*  SYNOPSIS
*     int cl_com_tcp_connection_request_handler_cleanup(cl_com_connection_t* 
*     connection) 
*
*  FUNCTION
*     This function will shutdown a service connection, created with the
*     cl_com_tcp_connection_request_handler_setup() function. Free the connection
*     with cl_tcp_close_connection() has to be done by caller.
*
*  INPUTS
*     cl_com_connection_t* connection - Connection to shutdown
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success 
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_connection_request_handler()
*     cl_tcp_framework/cl_com_tcp_connection_request_handler_setup()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_connection_request_handler_cleanup()"
int cl_com_tcp_connection_request_handler_cleanup(cl_com_connection_t* connection) { /* CR check */

   cl_com_tcp_private_t* private = NULL;

   CL_LOG(CL_LOG_INFO,"cleanup of request handler ...");
   if (connection == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   shutdown(private->sockfd, 2);
   close(private->sockfd);
   private->sockfd = -1;

   return CL_RETVAL_OK;
}





/* caller must free new_connection pointer */

/****** cl_tcp_framework/cl_com_tcp_connection_request_handler() ***************
*  NAME
*     cl_com_tcp_connection_request_handler() -- Check for connection requests
*
*  SYNOPSIS
*     int cl_com_tcp_connection_request_handler(cl_com_connection_t* 
*     connection, cl_com_connection_t** new_connection, int timeout_val_sec, 
*     int timeout_val_usec) 
*
*  FUNCTION
*     This function will do a select call for the service connection file de-
*     scriptor. If the select returns with no error the connection is accepted
*     (via accept()) and a new connection structure ( cl_com_connection_t )
*     is malloced. The new connection will get all default settings from the
*     service connection struct. 
*
*     This function has to fill all struct information for the new connection
*     ( cl_com_connection_t type)
*
*  INPUTS
*     cl_com_connection_t* connection      - pointer to service connection
*     cl_com_connection_t** new_connection - NULL
*     int timeout_val_sec                  - timeout value in sec (for select)
*     int timeout_val_usec                 - timeout value in usec (for select)
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success 
*     an address to a new connection in new_connection parameter
*
*  SEE ALSO
*     cl_tcp_framework/cl_com_tcp_connection_request_handler_setup()
*     cl_tcp_framework/cl_com_tcp_connection_request_handler_cleanup()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_connection_request_handler()"
int cl_com_tcp_connection_request_handler(cl_com_connection_t* connection, cl_com_connection_t** new_connection) {
   cl_com_connection_t* tmp_connection = NULL;
   struct sockaddr_in cli_addr;
   int new_sfd = 0;
   int sso;
#if defined(AIX43) || defined(AIX51)
   size_t fromlen = 0;
#else
   int fromlen = 0;
#endif
   int retval;
   int server_fd = -1;
   cl_com_tcp_private_t* private = NULL;
   
   if (connection == NULL || new_connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no accept connection");
      return CL_RETVAL_PARAMS;
   }

   if (*new_connection != NULL) {
      CL_LOG(CL_LOG_ERROR,"accept connection is not free");
      return CL_RETVAL_PARAMS;
   }
   
   private = cl_com_tcp_get_private(connection);
   if (private == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework is not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if (connection->service_handler_flag != CL_COM_SERVICE_HANDLER) {
      CL_LOG(CL_LOG_ERROR,"connection is no service handler");
      return CL_RETVAL_NOT_SERVICE_HANDLER;
   }
   server_fd = private->sockfd;

   /* got new connect */
   fromlen = sizeof(cli_addr);
   memset((char *) &cli_addr, 0, sizeof(cli_addr));
   new_sfd = accept(server_fd, (struct sockaddr *) &cli_addr, &fromlen);
   if (new_sfd > -1) {
       char* resolved_host_name = NULL;
       cl_com_tcp_private_t* tmp_private = NULL;

       cl_com_cached_gethostbyaddr(&(cli_addr.sin_addr), &resolved_host_name ,NULL, NULL); 
       if (resolved_host_name != NULL) {
          CL_LOG_STR(CL_LOG_INFO,"new connection from host", resolved_host_name  );
       } else {
          CL_LOG(CL_LOG_WARNING,"could not resolve incoming hostname");
       }

       fcntl(new_sfd, F_SETFL, O_NONBLOCK);         /* HP needs O_NONBLOCK, was O_NDELAY */
       sso = 1;
#if defined(SOLARIS) && !defined(SOLARIS64)
       if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int)) == -1) {
          CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
       }
#else
       if (setsockopt(new_sfd, IPPROTO_TCP, TCP_NODELAY, &sso, sizeof(int))== -1) { 
          CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
       }
#endif
       /* here we can investigate more information about the client */
       /* ntohs(cli_addr.sin_port) ... */

       tmp_connection = NULL;
       /* setup a tcp connection where autoclose is still undefined */
       if ( (retval=cl_com_tcp_setup_connection(&tmp_connection, 
                                                private->server_port,
                                                private->connect_port,
                                                connection->data_flow_type, 
                                                CL_CM_AC_UNDEFINED,
                                                connection->framework_type,
                                                connection->data_format_type,
                                                connection->tcp_connect_mode)) != CL_RETVAL_OK) {
          cl_com_tcp_close_connection(&tmp_connection); 
          if (resolved_host_name != NULL) {
             free(resolved_host_name);
          }
          shutdown(new_sfd, 2);
          close(new_sfd);
          return retval;
       }

       tmp_connection->client_host_name = resolved_host_name; /* set resolved hostname of client */

       /* setup cl_com_tcp_private_t */
       tmp_private = cl_com_tcp_get_private(tmp_connection);
       if (tmp_private != NULL) {
          tmp_private->sockfd = new_sfd;   /* fd from accept() call */
          tmp_private->connect_in_port = ntohs(cli_addr.sin_port);
       }

       *new_connection = tmp_connection;
       return CL_RETVAL_OK;
   }
   return CL_RETVAL_OK;
}



/*
  fill connection struct with client information
  ==============================================

  connection = connection to fill in client information for new connect
  sockfd = new connection socket file descriptor (from accept call)
*/

/****** cl_tcp_framework/cl_com_tcp_get_private() ******************************
*  NAME
*     cl_com_tcp_get_private() -- get cl_com_tcp_private_t struct of a connection
*
*  SYNOPSIS
*     static cl_com_tcp_private_t* cl_com_tcp_get_private(cl_com_connection_t* 
*     connection) 
*
*  FUNCTION
*     This function returns the com_private pointer of the connection and does
*     a type cast.
*
*  INPUTS
*     cl_com_connection_t* connection - tcp connection to get private struct
*
*  RESULT
*     static cl_com_tcp_private_t* - pointer to private tcp data of tcp connection
*
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_get_private()"
static cl_com_tcp_private_t* cl_com_tcp_get_private(cl_com_connection_t* connection) {  /* CR check */
   if (connection != NULL) {
      return (cl_com_tcp_private_t*) connection->com_private;
   }
   return NULL;
}





/****** cl_tcp_framework/cl_com_tcp_open_connection_request_handler() **********
*  NAME
*     cl_com_tcp_open_connection_request_handler() -- ??? 
*
*  SYNOPSIS
*     int cl_com_tcp_open_connection_request_handler(cl_raw_list_t* 
*     connection_list, int timeout_val) 
*
*  FUNCTION
*     First action of this function is do set the data_read_flag of each
*     connection in the list to CL_COM_DATA_NOT_READY.
*
*     After that this function will do a select on all file descriptors in 
*     the list for reading. If the select returns that there is data for a 
*     connection the data_read_flag of the connection 
*     ( struct cl_com_connection_t ) is set.
*
*  INPUTS
*     cl_raw_list_t* connection_list - connection list
*     int timeout_val                - timeout for select
*
*  RESULT
*     int - CL_RETVAL_XXXX error or CL_RETVAL_OK on success 
*
*  SEE ALSO
*     cl_communication/cl_com_open_connection_request_handler()
*******************************************************************************/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_open_connection_request_handler()"
int cl_com_tcp_open_connection_request_handler(cl_raw_list_t* connection_list, cl_com_connection_t* service_connection, int timeout_val_sec, int timeout_val_usec, cl_select_method_t select_mode ) {
   struct timeval timeout;
   int select_back;
   cl_connection_list_elem_t* con_elem = NULL;
   cl_com_connection_t*  connection = NULL;
   cl_com_tcp_private_t* con_private = NULL;
   fd_set my_read_fds;
   fd_set my_write_fds;

   int max_fd = -1;
   int server_fd = -1;
   int retval = CL_RETVAL_UNKNOWN;
   int do_read_select = 0;
   int do_write_select = 0;
   int my_errno = 0;
   int nr_of_descriptors = 0;
   cl_connection_list_data_t* ldata = NULL;


   if (connection_list == NULL ) {
      CL_LOG(CL_LOG_ERROR,"no connection list");
      return CL_RETVAL_PARAMS;
   }

   if (select_mode == CL_RW_SELECT || select_mode == CL_R_SELECT) {
      do_read_select = 1;
   }
   if (select_mode == CL_RW_SELECT || select_mode == CL_W_SELECT) {
      do_write_select = 1;
   }

   timeout.tv_sec = timeout_val_sec; 
   timeout.tv_usec = timeout_val_usec;
   FD_ZERO(&my_read_fds);
   FD_ZERO(&my_write_fds);

   if (service_connection != NULL && do_read_select != 0) {
      /* this is to come out of select when for new connections */
      if(cl_com_tcp_get_private(service_connection) == NULL ) {
         CL_LOG(CL_LOG_ERROR,"service framework is not initalized");
         return CL_RETVAL_NO_FRAMEWORK_INIT;
      }
      if( service_connection->service_handler_flag != CL_COM_SERVICE_HANDLER) {
         CL_LOG(CL_LOG_ERROR,"service connection is no service handler");
         return CL_RETVAL_NOT_SERVICE_HANDLER;
      }
      server_fd = cl_com_tcp_get_private(service_connection)->sockfd;
      max_fd = MAX(max_fd,server_fd);
      FD_SET(server_fd,&my_read_fds); 
      nr_of_descriptors++;
      service_connection->data_read_flag = CL_COM_DATA_NOT_READY;
   }

   /* lock list */
   if ( cl_raw_list_lock(connection_list) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"could not lock connection list");
      return CL_RETVAL_LOCK_ERROR;
   }

   if ( connection_list->list_data == NULL) {
      cl_raw_list_unlock(connection_list);
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   } else {
      ldata = (cl_connection_list_data_t*) connection_list->list_data;
   }

   /* reset connection data_read flags */
   con_elem = cl_connection_list_get_first_elem(connection_list);

   while(con_elem) {
      connection = con_elem->connection;

      if ( (con_private=cl_com_tcp_get_private(connection)) == NULL) {
         cl_raw_list_unlock(connection_list);
         CL_LOG(CL_LOG_ERROR,"no private data pointer");
         return CL_RETVAL_NO_FRAMEWORK_INIT;
      }

      switch(connection->framework_type) {
         case CL_CT_TCP: {
            switch (connection->connection_state) {
               case CL_CONNECTED:
                  if (connection->ccrm_sent == 0) {
                     if (do_read_select != 0) {
                        max_fd = MAX(max_fd,con_private->sockfd);
                        FD_SET(con_private->sockfd,&my_read_fds); 
                        nr_of_descriptors++;
                        connection->data_read_flag = CL_COM_DATA_NOT_READY;
                     }
                     if (connection->data_write_flag == CL_COM_DATA_READY && do_write_select != 0) {
                        /* this is to come out of select when data is ready to write */
                        max_fd = MAX(max_fd, con_private->sockfd);
                        FD_SET(con_private->sockfd,&my_write_fds);
                        connection->fd_ready_for_write = CL_COM_DATA_NOT_READY;
                     } 
                  }
                  break;
               case CL_CONNECTING:
                  if (do_read_select != 0) {
                     max_fd = MAX(max_fd,con_private->sockfd);
                     FD_SET(con_private->sockfd,&my_read_fds); 
                     nr_of_descriptors++;
                     connection->data_read_flag = CL_COM_DATA_NOT_READY;
                  }
                  if (connection->data_write_flag == CL_COM_DATA_READY && do_write_select != 0) {
                     /* this is to come out of select when data is ready to write */
                     max_fd = MAX(max_fd, con_private->sockfd);
                     FD_SET(con_private->sockfd,&my_write_fds);
                     connection->fd_ready_for_write = CL_COM_DATA_NOT_READY;
                  }
                  break;
               case CL_OPENING:
                  /* this is to come out of select when connection socket is ready to connect */
                  switch(connection->connection_sub_state) {
                     case CL_COM_OPEN_CONNECTED:
                     case CL_COM_OPEN_CONNECT_IN_PROGRESS:
                     case CL_COM_OPEN_CONNECT: {
                        if ( con_private->sockfd > 0 && do_read_select != 0) {
                           max_fd = MAX(max_fd, con_private->sockfd);
                           FD_SET(con_private->sockfd,&my_write_fds);
                        } 
                        break;
                     }
                     default:
                        break;
                  }
                  break;
               case CL_DISCONNECTED:
               case CL_CLOSING:
                  break;
            }
            break;
         }
         case CL_CT_UNDEFINED:
         case CL_CT_SSL: {
            CL_LOG_STR(CL_LOG_WARNING,"ignoring unexpected connection type:",
                       cl_com_get_framework_type(connection));
         }
      }
      con_elem = cl_connection_list_get_next_elem(con_elem);
      if (max_fd + 1 >= FD_SETSIZE) {
         CL_LOG(CL_LOG_ERROR,"filedescriptors exeeds FD_SETSIZE of this system");
         max_fd = FD_SETSIZE - 1;
      } 
   }

   /* we don't have any file descriptor for select(), find out why: */
   if (max_fd == -1) {
      CL_LOG_INT(CL_LOG_INFO,"max fd =", max_fd);
/* TODO: remove CL_W_SELECT and CL_R_SELECT handling and use one handling for 
         CL_W_SELECT, CL_R_SELECT and CL_RW_SELECT ? */
      if ( select_mode == CL_W_SELECT ) {
         /* return immediate for only write select ( only called by write thread) */
         cl_raw_list_unlock(connection_list); 
         CL_LOG(CL_LOG_INFO,"returning, because of no select descriptors (CL_W_SELECT)");
         return CL_RETVAL_NO_SELECT_DESCRIPTORS;
      }
#if 0
      if ( select_mode == CL_R_SELECT ) {
         /* return immediate for only read select ( only called by read thread) */
         cl_raw_list_unlock(connection_list); 
         CL_LOG(CL_LOG_INFO,"returning, because of no select  (CL_R_SELECT)");
         return CL_RETVAL_NO_SELECT_DESCRIPTORS; 
      }
#endif

      /* (only when not multithreaded): 
       *    don't return immediately when the last call to this function was also
       *    with no possible descriptors! ( which may be caused by a not connectable service )
       *    This must be done to prevent the application to poll endless ( with 100% CPU usage)
       *
       *    we have no file descriptors, but we do a select with standard timeout
       *    because we don't want to overload the cpu by endless trigger() calls 
       *    from application when there is no connection client 
       *    (no descriptors part 1)
       *
       *    we have a handler of the connection list, try to find out if 
       *    this is the first call without guilty file descriptors 
       */
      
      if ( ldata->select_not_called_count < 3 ) { 
         CL_LOG_INT(CL_LOG_WARNING, "no usable file descriptor for select() call nr.:", ldata->select_not_called_count);
         ldata->select_not_called_count += 1;
         cl_raw_list_unlock(connection_list); 
         return CL_RETVAL_NO_SELECT_DESCRIPTORS; 
      } else {
         CL_LOG(CL_LOG_WARNING, "no usable file descriptors (repeated!) - select() will be used for wait");
         ldata->select_not_called_count = 0;
#if 0
         /* enable this for shorter timeout */
         timeout.tv_sec = 0; 
         timeout.tv_usec = 100*1000;  /* wait for 1/10 second */
#endif
         max_fd = 0;
      }
   }

   
   /* TODO: Fix this problem (multithread mode):
         -  find a way to wake up select when a new connection was added by another thread
            (perhaps with dummy read file descriptor)
   */
    
   if ( nr_of_descriptors != ldata->last_nr_of_descriptors ) {
      if ( nr_of_descriptors == 1 && service_connection != NULL && do_read_select != 0 ) {
         /* This is to return as far as possible if this connection has a service and
             a client was disconnected */

         /* a connection is done and no more connections (beside service connection itself) is alive,
            return to application as far as possible, don't wait for a new connect */
         ldata->last_nr_of_descriptors = nr_of_descriptors;
         cl_raw_list_unlock(connection_list); 
         CL_LOG(CL_LOG_INFO,"last connection closed");
         return CL_RETVAL_NO_SELECT_DESCRIPTORS;
      }
   }

   ldata->last_nr_of_descriptors = nr_of_descriptors;

   cl_raw_list_unlock(connection_list); 


   if (max_fd + 1 >= FD_SETSIZE) {
      CL_LOG(CL_LOG_ERROR,"filedescriptors exeeds FD_SETSIZE of this system");
      max_fd = FD_SETSIZE - 1;
   } 

   errno = 0;
   select_back = select(max_fd + 1, &my_read_fds, &my_write_fds, NULL, &timeout);
   my_errno = errno;

   if (max_fd == 0) {
      /* there were no file descriptors! Return error after select timeout! */
      /* (no descriptors part 2) */
      return CL_RETVAL_NO_SELECT_DESCRIPTORS;
   }
   switch(select_back) {
      case -1:
         if (my_errno == EINTR) {
            CL_LOG(CL_LOG_WARNING,"select interrupted");
            retval = CL_RETVAL_SELECT_INTERRUPT;
         } else {
            CL_LOG(CL_LOG_ERROR,"select error");
            retval = CL_RETVAL_SELECT_ERROR;
         }
         break;
      case 0:
         CL_LOG_INT(CL_LOG_INFO,"----->>>>>>>>>>> select timeout <<<<<<<<<<<<<<<<<<<--- maxfd=",max_fd);
         retval = CL_RETVAL_SELECT_TIMEOUT;
         break;
      default:
         cl_raw_list_lock(connection_list); 
         /* now set the read flags for connections, where data is available */
         con_elem = cl_connection_list_get_first_elem(connection_list);
         while(con_elem) {
            connection  = con_elem->connection;
            con_private = cl_com_tcp_get_private(connection);

            if (do_read_select != 0) {
               if (con_private->sockfd >= 0 && con_private->sockfd <= max_fd) {
                  if (FD_ISSET(con_private->sockfd, &my_read_fds)) {
                     connection->data_read_flag = CL_COM_DATA_READY;
                  }
               }
            }
            if (do_write_select != 0) {
               if (con_private->sockfd >= 0 && con_private->sockfd <= max_fd) { 
                  if (FD_ISSET(con_private->sockfd, &my_write_fds)) {
                     connection->fd_ready_for_write = CL_COM_DATA_READY;
                  }
               }
            }
            con_elem = cl_connection_list_get_next_elem(con_elem);
         }
         cl_raw_list_unlock(connection_list);
         if (server_fd != -1) {
            if (FD_ISSET(server_fd, &my_read_fds)) {
               CL_LOG(CL_LOG_INFO,"NEW CONNECTION");
               service_connection->data_read_flag = CL_COM_DATA_READY;
            }
         }
         return CL_RETVAL_OK; /* OK - done */
   }
   return retval;
}

