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
#include "cl_message_list.h"
#include "cl_connection_list.h"
#include "cl_util.h"

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

static int cl_com_tcp_write(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long *only_one_write);  /* CR check */
static int cl_com_tcp_read(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long *only_one_read);  /* CR check */
static cl_com_tcp_private_t* cl_com_tcp_get_private(cl_com_connection_t* connection);   /* CR check */


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
#define __CL_FUNCTION__ "cl_dump_tcp_private()"
void cl_dump_tcp_private(cl_com_connection_t* connection) {  /* CR check */
   cl_com_tcp_private_t* private = NULL;
   CL_LOG(CL_LOG_DEBUG,"*** cl_dump_tcp_private() ***");
   if (connection == NULL) {
      CL_LOG(CL_LOG_DEBUG, "connection is NULL");
   } else {
      if ( (private=cl_com_tcp_get_private(connection)) != NULL) {
         CL_LOG_INT(CL_LOG_DEBUG,"server port:",private->server_port);
         CL_LOG_INT(CL_LOG_DEBUG,"connect_port:",private->connect_port);
         CL_LOG_INT(CL_LOG_DEBUG,"socked fd:",private->sockfd);
      }
   }
   CL_LOG(CL_LOG_DEBUG,"****************************");
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
*     given connect_port (set with cl_com_setup_tcp_connection()) on the
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

   int tmp_error = CL_RETVAL_OK;
   if (connection == NULL) { 
      return  CL_RETVAL_PARAMS;
   }
   if (connection->remote == NULL || connection->local == NULL || connection->receiver == NULL || connection->sender == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (cl_com_tcp_get_private(connection) == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   CL_LOG(CL_LOG_INFO,"setting up connection ...");
   if ( connection->connection_sub_state == CL_COM_OPEN_INIT) {
      int sockfd = -1;
      int on = 1;
      char* unique_host = NULL;
      struct timeval now;

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_INIT");
      cl_com_tcp_get_private(connection)->sockfd = -1;
      

      /* create socket */
      if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         CL_LOG(CL_LOG_ERROR,"could not create socket");
         return CL_RETVAL_CREATE_SOCKET;
      }

      /* set local address reuse socket option */
      if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set SO_REUSEADDR");
         cl_com_tcp_get_private(connection)->sockfd = -1;
         return CL_RETVAL_SETSOCKOPT_ERROR;
      }  
   
      /* this is a non blocking socket */
      if ( fcntl(sockfd, F_SETFL, O_NONBLOCK) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set O_NONBLOCK");
         cl_com_tcp_get_private(connection)->sockfd = -1;
         return CL_RETVAL_FCNTL_ERROR;
      }


      /* set address  */
      memset((char *) &(cl_com_tcp_get_private(connection)->client_addr), 0, sizeof(struct sockaddr_in));
      cl_com_tcp_get_private(connection)->client_addr.sin_port = htons(cl_com_tcp_get_private(connection)->connect_port);
      cl_com_tcp_get_private(connection)->client_addr.sin_family = AF_INET;
      if ( (tmp_error=cl_com_cached_gethostbyname(connection->remote->comp_host, &unique_host, &(cl_com_tcp_get_private(connection)->client_addr.sin_addr),NULL , NULL)) != CL_RETVAL_OK) {
         shutdown(sockfd, 2);
         close(sockfd);
         free(unique_host);
         CL_LOG(CL_LOG_ERROR,"could not get hostname");
         cl_com_tcp_get_private(connection)->sockfd = -1;
         return tmp_error; 
      } 
      free(unique_host);

      /* connect */
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      connection->write_buffer_timeout_time = now.tv_sec + timeout;
      cl_com_tcp_get_private(connection)->sockfd = sockfd;
      connection->connection_sub_state = CL_COM_OPEN_CONNECT;
   }
   
   if ( connection->connection_sub_state == CL_COM_OPEN_CONNECT) {
      int sockfd = -1;
      int timeout_flag  = 0;
      fd_set writefds;

      sockfd = cl_com_tcp_get_private(connection)->sockfd;

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_CONNECT");

      while (1) {
         int my_error;
         int i;
         int select_back = 0;
         int sock_err = 0;

         struct timeval now;
         struct timeval stimeout;


         CL_LOG(CL_LOG_INFO,"gettimeofday");
         gettimeofday(&now,NULL);
         if (connection->write_buffer_timeout_time <= now.tv_sec || cl_com_get_ignore_timeouts_flag() == CL_TRUE ) {
            timeout_flag = 1;
            break;
         }
   
         errno = 0;
         i = connect(sockfd, (struct sockaddr *) &(cl_com_tcp_get_private(connection)->client_addr), sizeof(struct sockaddr_in));
         my_error = errno;
         if (i != 0) {
            CL_LOG_INT(CL_LOG_INFO,"connect status errno:", my_error);
            if (my_error == EISCONN) {
               /* socket is allready connected */
               CL_LOG(CL_LOG_INFO,"allready connected");
               break;
            }
   
            if (my_error == ECONNREFUSED || my_error == EADDRNOTAVAIL ) {
               /* can't open connection */
               CL_LOG_INT(CL_LOG_ERROR,"connection refused or not available on port",cl_com_tcp_get_private(connection)->connect_port);
               shutdown(sockfd, 2);
               close(sockfd);
               cl_com_tcp_get_private(connection)->sockfd = -1;
               return CL_RETVAL_CONNECT_ERROR;
            }
            
            if (my_error == EINPROGRESS || my_error == EALREADY) {
               CL_LOG(CL_LOG_INFO,"connecting ...");
               /* wait for select to indicate that write is enabled 
                  this is done with timeout in order to not stress 
                  the cpu */
               FD_ZERO(&writefds);

               FD_SET(sockfd, &writefds);
               if (only_once == 0) {
                  stimeout.tv_sec = 0; 
                  stimeout.tv_usec = 250*1000;   /* 1/4 sec */
               } else {
                  stimeout.tv_sec = 0; 
                  stimeout.tv_usec = 0;   /* don't waste time */
               }
               select_back = select(sockfd + 1, NULL, &writefds, NULL, &stimeout);
               if (select_back > 0) {
                  int socket_error = 0;
                  int socklen = sizeof(socket_error);

#if defined(SOLARIS) && !defined(SOLARIS64) 
                  sock_err = getsockopt(sockfd,SOL_SOCKET, SO_ERROR, (void*)&socket_error, &socklen);
#else
                  sock_err = getsockopt(sockfd,SOL_SOCKET, SO_ERROR, &socket_error, &socklen);
#endif
                  if (socket_error == 0) {
                     CL_LOG(CL_LOG_INFO,"connected");
                     break; /* we are connected */
                  } else {
                     if (socket_error != EINPROGRESS && socket_error != EALREADY && socket_error != EISCONN) {
                        CL_LOG_INT(CL_LOG_ERROR,"socket error errno:", socket_error);
                        shutdown(sockfd, 2);
                        close(sockfd);
                        cl_com_tcp_get_private(connection)->sockfd = -1;
                        return CL_RETVAL_CONNECT_ERROR;
                     }
                     if (only_once != 0) {
                        return CL_RETVAL_UNCOMPLETE_WRITE;
                     }
                  }
               } else {
                  if (only_once != 0) {
                     if (select_back == -1) {
                        CL_LOG(CL_LOG_ERROR,"select error");
                        return CL_RETVAL_SELECT_ERROR;
                     }
                     return CL_RETVAL_UNCOMPLETE_WRITE;
                  }
               } 
            } else {
               /* we have an connect error */
               CL_LOG_INT(CL_LOG_ERROR,"connect error errno:", my_error);
               shutdown(sockfd, 2);
               close(sockfd);
               cl_com_tcp_get_private(connection)->sockfd = -1;
               return CL_RETVAL_CONNECT_ERROR;
            }
         } else {
            /* we are connected */
            CL_LOG(CL_LOG_INFO,"connected");
            break;
         }
      }
   
      if (timeout_flag == 1) {
         /* we had an timeout */
         CL_LOG(CL_LOG_ERROR,"connect timeout error");
         connection->write_buffer_timeout_time = 0;
         shutdown(sockfd, 2);
         close(sockfd);
         cl_com_tcp_get_private(connection)->sockfd = -1;
         return CL_RETVAL_CONNECT_TIMEOUT;
      }
      connection->write_buffer_timeout_time = 0;
      connection->connection_sub_state = CL_COM_OPEN_CONNECTED;
   }

   if ( connection->connection_sub_state == CL_COM_OPEN_CONNECTED) {
      int sockfd = -1;
      int on = 1; 

      CL_LOG(CL_LOG_DEBUG,"state is CL_COM_OPEN_CONNECTED");

      sockfd = cl_com_tcp_get_private(connection)->sockfd;
  
#if defined(SOLARIS) && !defined(SOLARIS64)
      if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &on, sizeof(int)) != 0) {
         CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
      } 
#else
      if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int))!= 0) {
         CL_LOG(CL_LOG_ERROR,"could not set TCP_NODELAY");
      }
#endif
      cl_com_tcp_get_private(connection)->sockfd = sockfd;
      return CL_RETVAL_OK;
   }

   return CL_RETVAL_UNKNOWN;
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
int cl_com_tcp_free_com_private(cl_com_connection_t* connection) {  /* CR check */
   cl_com_tcp_private_t* private = NULL;


   CL_LOG(CL_LOG_INFO,"free private connection data ...");

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
   free(cl_com_tcp_get_private(connection));
   connection->com_private = NULL;
  
   CL_LOG(CL_LOG_INFO,"private connection data released");
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
int cl_com_tcp_close_connection(cl_com_connection_t** connection) {     /* CR check */
   int retval = CL_RETVAL_OK;
   int ret = CL_RETVAL_OK;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   CL_LOG(CL_LOG_INFO,"closing connection");

   if (cl_com_tcp_get_private(*connection) != NULL) {
      if (cl_com_tcp_get_private((*connection))->sockfd >= 0) {
         /* shutdown socket connection */
         shutdown(cl_com_tcp_get_private((*connection))->sockfd, 2);
         close(cl_com_tcp_get_private((*connection))->sockfd);
         cl_com_tcp_get_private((*connection))->sockfd = -1;
      } 
      /* free com private structure */
      ret = cl_com_tcp_free_com_private(*connection);
      if (ret != CL_RETVAL_OK) {
         retval = ret;
      }
   } else {
      retval = CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   CL_LOG(CL_LOG_INFO,"closing connection finished");
   return retval;
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
static int cl_com_tcp_write(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long *only_one_write) {  /* CR check */
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
   /* TODO: this is a boddle neck if only_one_write is not set,
            because if the message can't be read
            complete, we must try it later !!!!!!!!!!!!!!! */

   while ( data_complete != size ) {
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      if ( now.tv_sec >= timeout_time ) {
         CL_LOG(CL_LOG_ERROR,"send timeout error");
         return CL_RETVAL_SEND_TIMEOUT;
      }
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
         data_written =  write(fd, &message[data_complete], size - data_complete );   
         my_errno = errno;
         if (data_written < 0) {
            if (my_errno == EWOULDBLOCK || my_errno == EAGAIN || my_errno == EINTR) {
               if (only_one_write != NULL) {
                  *only_one_write = data_complete;
                  if (data_complete != size) {
                     CL_LOG_INT(CL_LOG_INFO,"incomplete write, bytes to write:", size);
                     CL_LOG_INT(CL_LOG_INFO,"bytes written:", data_complete);
                     return CL_RETVAL_UNCOMPLETE_WRITE;
                  }
                  CL_LOG_INT(CL_LOG_INFO,"all bytes written:",data_complete );
                  return CL_RETVAL_OK;
               }
            }
            if (my_errno == EPIPE) {
               CL_LOG(CL_LOG_ERROR,"pipe error");
               return CL_RETVAL_PIPE_ERROR;
            }
            CL_LOG(CL_LOG_ERROR,"send error");
            return CL_RETVAL_SEND_ERROR;
         } else {
            data_complete = data_complete + data_written;
            data_written = 0;
            if (only_one_write != NULL) {
               *only_one_write = data_complete;
               if (data_complete != size) {
                  CL_LOG_INT(CL_LOG_INFO,"incomplete write, bytes to write:", size);
                  CL_LOG_INT(CL_LOG_INFO,"bytes written:", data_complete);
                  return CL_RETVAL_UNCOMPLETE_WRITE;
               }
               CL_LOG_INT(CL_LOG_INFO,"all bytes written:",data_complete );
               return CL_RETVAL_OK;
            }
         }
      }
      if (only_one_write != NULL) {
         *only_one_write = 0;
         CL_LOG_INT(CL_LOG_INFO,"bytes written", 0 );
         return CL_RETVAL_UNCOMPLETE_WRITE;
      }
   }
   CL_LOG_INT(CL_LOG_INFO,"all bytes written:",data_complete );
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
static int cl_com_tcp_read(long timeout_time, int fd, cl_byte_t* message, unsigned long size, unsigned long* only_one_read) {  /* CR check */
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
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      if ( now.tv_sec >= timeout_time ) {
         return CL_RETVAL_READ_TIMEOUT;
      }
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
         data_read =  read(fd, &message[data_complete], size - data_complete );
         my_errno = errno;
         if (data_read < 0) {
            if (my_errno == EWOULDBLOCK || my_errno == EAGAIN || my_errno == EINTR) {
               if (only_one_read != NULL) {
                  *only_one_read = data_complete;
                  if (data_complete != size) {
                     CL_LOG_INT(CL_LOG_INFO,"incomplete read, bytes to read:", size);
                     CL_LOG_INT(CL_LOG_INFO,"bytes read:", data_complete);
                     return CL_RETVAL_UNCOMPLETE_READ;
                  }
                  CL_LOG_INT(CL_LOG_INFO,"all bytes read:",data_complete );
                  return CL_RETVAL_OK;
               }
            }
            if (my_errno == EPIPE) {
               CL_LOG_INT(CL_LOG_ERROR,"pipe error errno:", my_errno );
               return CL_RETVAL_PIPE_ERROR;
            }
            CL_LOG_INT(CL_LOG_ERROR,"receive error errno:", my_errno);
            return CL_RETVAL_RECEIVE_ERROR;
         } else {
            if (data_read == 0) {
               /* this should only happen if the connection is down */
               CL_LOG(CL_LOG_WARNING,"client connection disconnected");
               return CL_RETVAL_READ_ERROR;
            } 
            data_complete = data_complete + data_read;
            data_read = 0;
            if (only_one_read != NULL) {
               *only_one_read = data_complete;
               if (data_complete != size) {
                  CL_LOG_INT(CL_LOG_INFO,"incomplete read, bytes to read:", size);
                  CL_LOG_INT(CL_LOG_INFO,"bytes read:", data_complete);
                  return CL_RETVAL_UNCOMPLETE_READ;
               }
               CL_LOG_INT(CL_LOG_INFO,"all bytes read:",data_complete );
               return CL_RETVAL_OK;
            }
         }
      }
      if (only_one_read != NULL) {
         *only_one_read = 0;
         CL_LOG_INT(CL_LOG_INFO,"bytes read:", 0);
         return CL_RETVAL_UNCOMPLETE_READ;
      }
   }
   CL_LOG_INT(CL_LOG_INFO,"all bytes read:",data_complete );

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
         CL_LOG(CL_LOG_INFO,"read once");
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
   /* printf("buffer is: \"%s\"\n", connection->data_read_buffer); */
   /* header should be now complete */
   if ( strcmp((char*)&(connection->data_read_buffer[connection->data_read_buffer_pos - 7]) ,"</gmsh>") != 0) {
      return CL_RETVAL_GMSH_ERROR;
   }
   /* printf("parsing header ...\n"); */
   
   /* parse header */
   retval = cl_xml_parse_GMSH(connection->data_read_buffer, connection->data_read_buffer_pos, connection->read_gmsh_header, &processed_data);
   CL_LOG_INT(CL_LOG_INFO,"processed data:",processed_data);
   CL_LOG_STR(CL_LOG_INFO,"buffer:", (char*)connection->data_read_buffer);
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
int cl_com_tcp_send_message(cl_com_connection_t* connection, int timeout_time, cl_byte_t* data , long size , unsigned long *only_one_write) {  /* CR check */

   int retval;
   if (connection == NULL || data == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no data");
      return CL_RETVAL_PARAMS;
   }
   if (cl_com_tcp_get_private(connection) == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   CL_LOG(CL_LOG_INFO,"sending message ...");

   retval = cl_com_tcp_write(timeout_time, cl_com_tcp_get_private(connection)->sockfd, data, size, only_one_write);
   return retval;
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
int cl_com_tcp_receive_message(cl_com_connection_t* connection, int timeout_time, cl_byte_t* data_buffer, unsigned long data_buffer_size , unsigned long *only_one_read) {  /* CR check */
   int retval;
   
   if (connection == NULL || data_buffer == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no data buffer");
      return CL_RETVAL_PARAMS;
   }
   if (cl_com_tcp_get_private(connection) == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   CL_LOG(CL_LOG_INFO,"receiving message ...");

   retval = cl_com_tcp_read(timeout_time, 
                            cl_com_tcp_get_private(connection)->sockfd,
                            data_buffer,
                            data_buffer_size, 
                            only_one_read );
   return retval;
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
int cl_com_tcp_connection_request_handler_setup(cl_com_connection_t* connection,cl_com_endpoint_t* local_endpoint ) {  /* CR check */
   int sockfd = 0;
   int on = 1;
   struct sockaddr_in serv_addr;

   CL_LOG(CL_LOG_INFO,"setting up request handler ...");
    
   if (connection == NULL ) {
      CL_LOG(CL_LOG_ERROR,"no connection");
      return CL_RETVAL_PARAMS;
   }

   if (cl_com_tcp_get_private(connection) == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   /* create socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      CL_LOG(CL_LOG_ERROR,"could not create socket");
      return CL_RETVAL_CREATE_SOCKET;
   }   

   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) != 0) {
      CL_LOG(CL_LOG_ERROR,"could not set SO_REUSEADDR");
      return CL_RETVAL_SETSOCKOPT_ERROR;
   }

   /* bind an address to socket */
   /* TODO FEATURE: we can also try to use a specified port range */
   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_port = htons(cl_com_tcp_get_private(connection)->server_port);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      shutdown(sockfd, 2);
      close(sockfd);
      CL_LOG_INT(CL_LOG_ERROR, "could not bind server socket port:", cl_com_tcp_get_private(connection)->server_port);
      return CL_RETVAL_BIND_SOCKET;
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
   cl_com_tcp_get_private(connection)->sockfd = sockfd;

   CL_LOG(CL_LOG_INFO,"===============================");
   CL_LOG(CL_LOG_INFO,"server setup done:");
   CL_LOG_STR(CL_LOG_INFO,"host:     ",connection->local->comp_host);
   CL_LOG_STR(CL_LOG_INFO,"component:",connection->local->comp_name);
   CL_LOG_INT(CL_LOG_INFO,"id:       ",connection->local->comp_id);
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

   CL_LOG(CL_LOG_INFO,"cleanup of request handler ...");
   if (connection == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   if (cl_com_tcp_get_private(connection) == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   shutdown(cl_com_tcp_get_private(connection)->sockfd, 2);
   close(cl_com_tcp_get_private(connection)->sockfd);
   CL_LOG(CL_LOG_INFO,"request handler cleanup done");

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
*     int timeout_val_sec                  - timeout value in sec (for select())
*     int timeout_val_usec                 - timeout value in usec (for select())
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
int cl_com_tcp_connection_request_handler(cl_com_connection_t* connection, cl_com_connection_t** new_connection , int timeout_val_sec , int timeout_val_usec) {  /* CR check */
   int select_back = 0;
   cl_com_connection_t* tmp_connection = NULL;
   fd_set readfds;
   struct timeval timeout;
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
   
   CL_LOG(CL_LOG_INFO,"request handler check ...");

   if (connection == NULL || new_connection == NULL) {
      CL_LOG(CL_LOG_ERROR,"no connection or no accept connection");
      return CL_RETVAL_PARAMS;
   }

   if (*new_connection != NULL) {
      CL_LOG(CL_LOG_ERROR,"accept connection is not free");
      return CL_RETVAL_PARAMS;
   }
   
   if (cl_com_tcp_get_private(connection) == NULL) {
      CL_LOG(CL_LOG_ERROR,"framework is not initalized");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if (connection->service_handler_flag != CL_COM_SERVICE_HANDLER) {
      CL_LOG(CL_LOG_ERROR,"connection is no service handler");
      return CL_RETVAL_NOT_SERVICE_HANDLER;
   }
   server_fd = cl_com_tcp_get_private(connection)->sockfd;

   FD_ZERO(&readfds);
   FD_SET(server_fd, &readfds);

   timeout.tv_sec = timeout_val_sec; 
   timeout.tv_usec = timeout_val_usec;
   select_back = select(server_fd + 1, &readfds, NULL, NULL, &timeout);
   if (select_back == -1) {
      return CL_RETVAL_SELECT_ERROR;
   }

   if (FD_ISSET(server_fd, &readfds)) {
      /* got new connect */
      fromlen = sizeof(cli_addr);
      memset((char *) &cli_addr, 0, sizeof(cli_addr));
      new_sfd = accept(server_fd, (struct sockaddr *) &cli_addr, &fromlen);
      if (new_sfd > -1) {
          char* resolved_host_name = NULL;
          CL_LOG(CL_LOG_INFO,"new connection setup ...");
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
          if ( (retval=cl_com_setup_tcp_connection(&tmp_connection, 
                                                   cl_com_tcp_get_private(connection)->server_port,
                                                   cl_com_tcp_get_private(connection)->connect_port,
                                                   connection->data_flow_type )) != CL_RETVAL_OK) {
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
          cl_com_tcp_get_private(tmp_connection)->sockfd = new_sfd;   /* fd from accept() call */
          *new_connection = tmp_connection;
          CL_LOG(CL_LOG_INFO,"new connection accepted");
          return CL_RETVAL_OK;
      }
   }
   return CL_RETVAL_OK;
}



/*
  fill connection struct with client information
  ==============================================

  connection = connection to fill in client information for new connect
  sockfd = new connection socket file descriptor (from accept call)
*/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_tcp_connection_complete_request()"

/* If timeout is 0 then the function will return after one read try, the
   caller has to call this function again */

/* return values CL_RETVAL_OK - connection is connected 
                 CL_RETVAL_UNCOMPLETE_READ  - waiting for client data
                 CL_RETVAL_UNCOMPLETE_WRITE - could not send all data
*/

int cl_com_tcp_connection_complete_request( cl_com_connection_t* connection, unsigned long timeout, unsigned long only_once ) {

   struct timeval now;
   int retval = CL_RETVAL_OK;
   cl_com_tcp_private_t* con_private = NULL;
   cl_com_CM_t* cm_message = NULL;
   cl_com_CRM_t* crm_message = NULL;
/*   cl_com_hostent_t* hostent_p = NULL; */
   char* unique_host = NULL;


   unsigned long data_read = 0;
   long data_to_read;
   unsigned long data_written = 0;


   if (connection == NULL ) {
      CL_LOG(CL_LOG_ERROR,"no connection");
      return CL_RETVAL_PARAMS;
   }

   /* printf("connection local hostname is: \"%s\"\n", connection->local->comp_host); */

   if ( (con_private=cl_com_tcp_get_private(connection)) == NULL) {
      CL_LOG(CL_LOG_ERROR,"no framework init");
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   if (connection->connection_state != CL_COM_CONNECTING) {
      CL_LOG(CL_LOG_ERROR,"connection statis is not connecting");
      return CL_RETVAL_ALLREADY_CONNECTED;
   }
 

   if (connection->client_host_name == NULL && connection->was_accepted != 0) {
      CL_LOG(CL_LOG_ERROR,"client hostname could not be resolved");
      return CL_RETVAL_GETHOSTNAME_ERROR; 
   }

   if (connection->connection_sub_state == CL_COM_READ_INIT) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_READ_INIT");
      if (connection->receiver != NULL ||
          connection->sender   != NULL ||
          connection->remote   != NULL      ) {
         CL_LOG(CL_LOG_ERROR,"connection is not free");
         return CL_RETVAL_PARAMS;
      }
      /* set connecting timeout in private structure */  
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      connection->read_buffer_timeout_time = now.tv_sec + timeout;
      connection->read_gmsh_header->dl = 0;
      connection->data_read_buffer_pos = 0;
      connection->data_read_buffer_processed = 0;
      connection->connection_sub_state = CL_COM_READ_GMSH;
      connection->data_write_flag = CL_COM_DATA_NOT_READY;
   }

   if (connection->connection_sub_state == CL_COM_READ_GMSH) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_READ_GMSH");

      /* read in GMSH header (General Message Size Header)*/
      if (only_once != 0) {
         data_read = 0;
         CL_LOG(CL_LOG_INFO,"read only once");
         retval = cl_com_tcp_read_GMSH(connection, &data_read);
      } else {
         CL_LOG(CL_LOG_INFO,"read with timeout");
         retval = cl_com_tcp_read_GMSH(connection, NULL );
      }
      if (retval != CL_RETVAL_OK) {
         return retval;
      }
      connection->connection_sub_state = CL_COM_READ_CM;
   }

   if (connection->connection_sub_state == CL_COM_READ_CM) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_READ_CM");
      CL_LOG_INT(CL_LOG_INFO,"GMSH dl:",connection->read_gmsh_header->dl );

      /* calculate (rest) data size to read = data length - stream buff position */

      data_to_read = connection->read_gmsh_header->dl - (connection->data_read_buffer_pos - connection->data_read_buffer_processed);
      CL_LOG_INT(CL_LOG_INFO,"data to read=",data_to_read);

      if ( (data_to_read + connection->data_read_buffer_pos) >= connection->data_buffer_size ) {
         CL_LOG(CL_LOG_ERROR,"stream buffer to small");
         return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
      }

      /* is data allready in buffer ? */
      if (data_to_read > 0) {
         if (only_once != 0) {
            data_read = 0;
            retval = cl_com_tcp_read(connection->read_buffer_timeout_time,
                                     con_private->sockfd, 
                                     &(connection->data_read_buffer[(connection->data_read_buffer_pos)]), /* position to continue */
                                     data_to_read, 
                                     &data_read);               /* returns the data bytes read */
            connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_read; /* add data read count to buff position */
         } else {
            retval = cl_com_tcp_read(connection->read_buffer_timeout_time,
                                     con_private->sockfd, 
                                     &(connection->data_read_buffer[(connection->data_read_buffer_pos)]),
                                     data_to_read, 
                                     NULL);
            connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_to_read; /* add data read count to buff position */
         }
         if (retval != CL_RETVAL_OK) {
            return retval;
         }
      }
      retval = cl_xml_parse_CM(&(connection->data_read_buffer[(connection->data_read_buffer_processed)]),connection->read_gmsh_header->dl, &cm_message);
      if (retval != CL_RETVAL_OK) {
         cl_com_free_cm_message(&cm_message);
         return retval;
      }

      cl_com_dump_endpoint(cm_message->src, "src"); 
      cl_com_dump_endpoint(cm_message->dst, "dst"); 
      cl_com_dump_endpoint(cm_message->rdata, "rdata"); 

      connection->data_read_buffer_processed = connection->data_read_buffer_processed + connection->read_gmsh_header->dl;
      

      /* resolve hostnames */
      /* printf("resolving \"%s\"\n", cm_message->src->comp_host); */
      CL_LOG_STR(CL_LOG_INFO,"resolving host", cm_message->src->comp_host);
    
      if ( (retval=cl_com_cached_gethostbyname(cm_message->src->comp_host, &unique_host, NULL, NULL, NULL)) != CL_RETVAL_OK) {
         free(unique_host);
         cl_com_free_cm_message(&cm_message);
         return retval; 
      }
      CL_LOG_STR(CL_LOG_INFO,"resolved as",unique_host  );

/*      printf("resolved as \"%s\"\n",hostent_p->he->h_name ); */

      connection->crm_state = CL_CRM_CS_UNDEFINED;

      if (cl_com_compare_hosts( cm_message->src->comp_host , unique_host ) != CL_RETVAL_OK) {
         CL_LOG(CL_LOG_ERROR,"hostname resolve error");
         connection->crm_state = CL_CRM_CS_DENIED;
      }
      
      connection->receiver = cl_com_create_endpoint(unique_host ,cm_message->src->comp_name,cm_message->src->comp_id);
      free(unique_host);
      unique_host = NULL;
      
      if ( (retval=cl_com_cached_gethostbyname(cm_message->dst->comp_host, &unique_host, NULL,NULL, NULL)) != CL_RETVAL_OK) {
         cl_com_free_endpoint(&(connection->receiver));
         free(unique_host);
         cl_com_free_cm_message(&cm_message);
         return retval; 
      }
      if (cl_com_compare_hosts( cm_message->dst->comp_host , unique_host ) != CL_RETVAL_OK) {
         CL_LOG(CL_LOG_ERROR,"hostname resolve error");
         connection->crm_state = CL_CRM_CS_DENIED;
      }
      connection->sender   = cl_com_create_endpoint( unique_host ,cm_message->dst->comp_name,cm_message->dst->comp_id);
      free(unique_host);
      unique_host = NULL;

      if (cm_message->rdata != NULL) {
         if ( (retval=cl_com_cached_gethostbyname(cm_message->rdata->comp_host, &unique_host,NULL,NULL, NULL)) != CL_RETVAL_OK) {
            cl_com_free_endpoint(&(connection->sender));
            cl_com_free_endpoint(&(connection->receiver));
            free(unique_host);
            cl_com_free_cm_message(&cm_message);
            return retval; 
         }
         connection->remote   = cl_com_create_endpoint(unique_host ,cm_message->rdata->comp_name,cm_message->rdata->comp_id);
         if (cl_com_compare_hosts( cm_message->rdata->comp_host , unique_host ) != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"hostname resolve error");
            connection->crm_state = CL_CRM_CS_DENIED;
         }
         free(unique_host);
         unique_host = NULL;
      } else {
         /* This host is allready resolved */
         connection->remote   = cl_com_create_endpoint(connection->receiver->comp_host,cm_message->src->comp_name,cm_message->src->comp_id);
      }
      connection->data_flow_type = cm_message->ct;
      connection->data_format_type = cm_message->df;

      cl_com_free_cm_message(&cm_message);

      if (connection->data_read_buffer_pos != connection->data_read_buffer_processed ) {
         CL_LOG_INT(CL_LOG_ERROR,"recevied more or less than expected:", 
                                 connection->data_read_buffer_pos - connection->data_read_buffer_processed );
      }

      if (connection->receiver == NULL || connection->sender == NULL || connection->remote == NULL ) {
         cl_com_free_endpoint(&(connection->receiver));
         cl_com_free_endpoint(&(connection->sender));
         cl_com_free_endpoint(&(connection->remote));
         return CL_RETVAL_MALLOC;
      }

      if ( cl_com_compare_hosts( connection->remote->comp_host , connection->client_host_name ) != CL_RETVAL_OK) { 
         CL_LOG(CL_LOG_ERROR,"hostname address resolving error");
         CL_LOG_STR(CL_LOG_ERROR,"hostname from address resolving:", connection->client_host_name );
         CL_LOG_STR(CL_LOG_ERROR,"resolved hostname from client:",connection->remote->comp_host  );
         connection->crm_state = CL_CRM_CS_DENIED;
      }

      if (connection->receiver->comp_id == 0 || connection->remote->comp_id == 0 ) {
         cl_com_handle_t* handle = connection->handler;

         CL_LOG(CL_LOG_INFO,"received connection request for auto client id");
         /* connection list should be locked in higher framework */
         if (handle != NULL) {
            int is_double = 0;
            cl_raw_list_t*   connection_list = NULL; 
            cl_connection_list_elem_t* elem = NULL;
            cl_com_connection_t* tmp_con = NULL;

            connection_list = handle->connection_list;
            /************************************
            * search unique client id           *
            ************************************/
            /* connection list is locked by calling function , so we do not need to lock the connection list */
            CL_LOG(CL_LOG_INFO,"search unique client id");
            connection->receiver->comp_id = 1;
            connection->remote->comp_id = 1;
            do {
               is_double = 0;
               for (elem=cl_connection_list_get_first_elem(connection_list); 
                    elem != NULL ; 
                    elem = cl_connection_list_get_next_elem(connection_list, elem)) {
                  tmp_con = elem->connection;

                  if (tmp_con == connection) {
                     continue;  /* Itself is allowed */
                  }
                  if (cl_com_compare_endpoints(tmp_con->receiver, connection->receiver) != 0) {
                     is_double = 1;
                     connection->receiver->comp_id = connection->receiver->comp_id + 1;
                     break;
                  } 
                  if (cl_com_compare_endpoints(tmp_con->remote, connection->remote) != 0) {
                     is_double = 1;
                     connection->remote->comp_id = connection->remote->comp_id + 1;
                     break;
                  }
               }
            } while (is_double == 1);
         } else {
            CL_LOG(CL_LOG_WARNING,"handle of connection is not set");
            if ( connection->receiver->comp_id == 0) {
               connection->receiver->comp_id = 1;
            }
            if ( connection->remote->comp_id == 0) {
               connection->remote->comp_id = 1;
            }
         }
                 
      }
      connection->read_buffer_timeout_time = 0;
      connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + connection->data_read_buffer_processed;
      connection->connection_sub_state = CL_COM_READ_INIT_CRM;
   }


   if (connection->connection_sub_state == CL_COM_READ_INIT_CRM ) {
      char* connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_OK;
      char* connection_status_text = "ok";
      unsigned long connect_response_message_size = 0;
      unsigned long gmsh_message_size = 0;

      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_READ_INIT_CRM");

      cl_com_dump_endpoint(connection->remote,"client endpoint");
      cl_com_dump_endpoint(connection->sender,"client requested endpoint partner");
      cl_com_dump_endpoint(connection->local,"local endpoint");

      if ( connection->crm_state == CL_CRM_CS_DENIED) {
         connection_status_text = "hostname resolving error";
         connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED;
         CL_LOG(CL_LOG_ERROR, connection_status_text );
      }

      if ( connection->crm_state == CL_CRM_CS_UNDEFINED) {
         connection->crm_state = CL_CRM_CS_CONNECTED;  /* if is ok, this is ok */

         if ( strcmp( connection->local->comp_name, connection->sender->comp_name) != 0 || 
              connection->local->comp_id != connection->sender->comp_id ) {

            connection_status_text = "requested component not found";
            connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED;
            connection->crm_state = CL_CRM_CS_DENIED;
            CL_LOG(CL_LOG_ERROR, connection_status_text );
         } 
      }

      if ( connection->crm_state == CL_CRM_CS_CONNECTED) {
         cl_com_handle_t* handler = NULL;
         cl_raw_list_t*   connection_list = NULL; 
         cl_connection_list_elem_t* elem = NULL;
         cl_com_connection_t* tmp_con = NULL;
         int is_double = 0;

         handler = connection->handler;
         if (handler != NULL) {
            connection_list = handler->connection_list;
            CL_LOG(CL_LOG_INFO,"checking for duplicate endpoints");
            /************************************
            * check duplicate endpoint entries  *
            ************************************/
            /* connection list is locked by calling function , so we do not need to lock the connection list */
            for (elem=cl_connection_list_get_first_elem(connection_list); 
                 elem != NULL ; 
                 elem = cl_connection_list_get_next_elem(connection_list, elem)) {
               tmp_con = elem->connection;
               if (tmp_con == connection) {
                  continue;  /* Itself is allowed */
               }

               if (tmp_con->connection_state == CL_COM_CLOSING) {
                  continue;  /* This connection will be deleted soon, ignore it */
               }                 

               if (cl_com_compare_endpoints(tmp_con->receiver, connection->receiver) != 0) {
                  is_double = 1;
                  break;
               } 
            }
            if (is_double == 0) {
               CL_LOG(CL_LOG_INFO,"new client is unique");
            } else {
               CL_LOG(CL_LOG_WARNING,"new client is allready connected - endpoint not unique error");
               connection->crm_state = CL_CRM_CS_ENDPOINT_NOT_UNIQUE; /* CL_CRM_CS_DENIED; */
               connection_status_text = "allready connected - endpoint not unique error";
/*               connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED; */
               connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_NOT_UNIQUE;
               CL_LOG(CL_LOG_ERROR, connection_status_text );
            }
         } else {
            CL_LOG(CL_LOG_WARNING,"connection list has no handler");
         } 
      }

      if ( connection->crm_state == CL_CRM_CS_CONNECTED ) {
         if ( connection->handler != NULL && connection->was_accepted != 0 ) {
            int check_allowed_host_list = 0;
            if (connection->handler->allowed_host_list == NULL && check_allowed_host_list != 0) {
               connection_status_text = "client is not in allowed host list";
               connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED;
               connection->crm_state = CL_CRM_CS_DENIED;
               CL_LOG(CL_LOG_ERROR, connection_status_text );
            } else {
               cl_string_list_elem_t* elem = NULL;
               int is_ok = 0;
               if ( check_allowed_host_list != 0) {
                  cl_raw_list_lock(connection->handler->allowed_host_list);
                  for (elem = cl_string_list_get_first_elem(connection->handler->allowed_host_list) ;
                       elem != NULL; 
                       elem = cl_string_list_get_next_elem(connection->handler->allowed_host_list,elem)) {
                     char* resolved_host = NULL;
                     retval = cl_com_cached_gethostbyname(elem->string, &resolved_host, NULL, NULL, NULL );
                     if (retval == CL_RETVAL_OK && resolved_host != NULL) {
                        if(cl_com_compare_hosts(resolved_host, connection->client_host_name) == CL_RETVAL_OK) {
                           is_ok = 1;
                           free(resolved_host);
                           break;
                        }
                     }
                     free(resolved_host);
                     resolved_host = NULL;
                  }
                  cl_raw_list_unlock(connection->handler->allowed_host_list);
               } else {
                  CL_LOG(CL_LOG_WARNING,"allowed host list check is not activated");
                  is_ok = 1;
               }

               if (is_ok != 1) {
                  connection_status_text = "client is not in allowed host list";
                  connection_status = CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED;
                  connection->crm_state = CL_CRM_CS_DENIED;
                  CL_LOG(CL_LOG_ERROR, connection_status_text );
               }   
            }
         }
      }

      connect_response_message_size = CL_CONNECT_RESPONSE_MESSAGE_SIZE;
      connect_response_message_size = connect_response_message_size + strlen(CL_CONNECT_RESPONSE_MESSAGE_VERSION);
      connect_response_message_size = connect_response_message_size + strlen(connection_status);
      connect_response_message_size = connect_response_message_size + strlen(connection_status_text);

      connect_response_message_size = connect_response_message_size + strlen(connection->remote->comp_host);
      connect_response_message_size = connect_response_message_size + strlen(connection->remote->comp_name);
      connect_response_message_size = connect_response_message_size + cl_util_get_ulong_number_length(connection->remote->comp_id);

      connect_response_message_size = connect_response_message_size + strlen(connection->receiver->comp_host);
      connect_response_message_size = connect_response_message_size + strlen(connection->receiver->comp_name);
      connect_response_message_size = connect_response_message_size + cl_util_get_ulong_number_length(connection->receiver->comp_id);

      connect_response_message_size = connect_response_message_size + strlen(connection->sender->comp_host);
      connect_response_message_size = connect_response_message_size + strlen(connection->sender->comp_name);
      connect_response_message_size = connect_response_message_size + cl_util_get_ulong_number_length(connection->sender->comp_id);

      gmsh_message_size = CL_GMSH_MESSAGE_SIZE + cl_util_get_ulong_number_length(connect_response_message_size);

      if (connection->data_buffer_size < (gmsh_message_size + connect_response_message_size + 1) ) {
         return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
      }

      sprintf((char*)connection->data_write_buffer, CL_GMSH_MESSAGE ,
               connect_response_message_size);
         
      sprintf((char*)&((connection->data_write_buffer)[gmsh_message_size]),CL_CONNECT_RESPONSE_MESSAGE,
               CL_CONNECT_RESPONSE_MESSAGE_VERSION,
               connection_status,
               connection_status_text,
               connection->receiver->comp_host, 
               connection->receiver->comp_name,
               connection->receiver->comp_id,
               connection->sender->comp_host, 
               connection->sender->comp_name,
               connection->sender->comp_id,
               connection->remote->comp_host, 
               connection->remote->comp_name,
               connection->remote->comp_id);
      connection->data_write_buffer_pos = 0;
      connection->data_write_buffer_processed = 0;
      connection->data_write_buffer_to_send = gmsh_message_size + connect_response_message_size ;
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      connection->write_buffer_timeout_time = now.tv_sec + timeout;
      connection->data_write_flag = CL_COM_DATA_READY;
      connection->connection_sub_state = CL_COM_READ_SEND_CRM;
   }

   if (connection->connection_sub_state == CL_COM_READ_SEND_CRM ) {
         CL_LOG(CL_LOG_INFO,"state is CL_COM_READ_SEND_CRM");
         if (only_once != 0) {
            data_written = 0;
            retval = cl_com_tcp_write(connection->write_buffer_timeout_time,
                                      con_private->sockfd, 
                                      &(connection->data_write_buffer[(connection->data_write_buffer_pos)]),  /* position to continue */
                                      connection->data_write_buffer_to_send, 
                                      &data_written);               /* returns the data bytes read */
            connection->data_write_buffer_pos = connection->data_write_buffer_pos + data_written;  /* add data read count to buff position */
            connection->data_write_buffer_to_send = connection->data_write_buffer_to_send - data_written; 
         } else {
            retval = cl_com_tcp_write(connection->write_buffer_timeout_time,
                                     con_private->sockfd, 
                                     &(connection->data_write_buffer[(connection->data_write_buffer_pos)]),
                                     connection->data_write_buffer_to_send, 
                                     NULL);
            connection->data_write_buffer_pos = connection->data_write_buffer_pos + connection->data_write_buffer_to_send;
         }

         if (retval != CL_RETVAL_OK) {
            return retval;
         }
         if ( cl_raw_list_get_elem_count(connection->send_message_list) == 0) {
            connection->data_write_flag = CL_COM_DATA_NOT_READY;
         } else {
            connection->data_write_flag = CL_COM_DATA_READY;
         }
         connection->write_buffer_timeout_time = 0;

         if (connection->crm_state == CL_CRM_CS_CONNECTED) {
            connection->connection_state = CL_COM_CONNECTED;  /* That was it! */
            connection->connection_sub_state = CL_COM_WORK;
         } else {
            connection->connection_state = CL_COM_CLOSING;  /* That was it! */
            CL_LOG(CL_LOG_WARNING,"access to client denied");
            cl_dump_connection(connection);
         }
         connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + connection->data_write_buffer_pos;
   }
   

   if (connection->connection_sub_state == CL_COM_SEND_INIT) {
      unsigned long connect_message_size = 0;
      unsigned long gmsh_message_size = 0;
      char* format_type = NULL;
      char* flow_type = NULL;

      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_SEND_INIT");
      /* set connecting timeout in private structure */  
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      connection->write_buffer_timeout_time = now.tv_sec + timeout ;
     
      connect_message_size = CL_CONNECT_MESSAGE_SIZE;
      connect_message_size = connect_message_size + strlen(CL_CONNECT_MESSAGE_VERSION);

      if (connection->data_format_type == CL_CM_DF_BIN) {
         format_type = CL_CONNECT_MESSAGE_DATA_FORMAT_BIN;
      }
      if (connection->data_format_type == CL_CM_DF_XML) {
         format_type = CL_CONNECT_MESSAGE_DATA_FORMAT_XML;
      }
      connect_message_size = connect_message_size + strlen(format_type);

      if (connection->data_flow_type == CL_CM_CT_STREAM) {
         flow_type = CL_CONNECT_MESSAGE_DATA_FLOW_STREAM;
      }
      if (connection->data_flow_type == CL_CM_CT_MESSAGE) {
         flow_type = CL_CONNECT_MESSAGE_DATA_FLOW_MESSAGE;
      }
      connect_message_size = connect_message_size + strlen(flow_type);

      connect_message_size = connect_message_size + strlen(connection->sender->comp_host);
      connect_message_size = connect_message_size + strlen(connection->sender->comp_name);
      connect_message_size = connect_message_size + cl_util_get_ulong_number_length(connection->sender->comp_id);

      connect_message_size = connect_message_size + strlen(connection->receiver->comp_host);
      connect_message_size = connect_message_size + strlen(connection->receiver->comp_name);
      connect_message_size = connect_message_size + cl_util_get_ulong_number_length(connection->receiver->comp_id);

      connect_message_size = connect_message_size + strlen(connection->local->comp_host);
      connect_message_size = connect_message_size + strlen(connection->local->comp_name);
      connect_message_size = connect_message_size + cl_util_get_ulong_number_length(connection->local->comp_id);

      gmsh_message_size = CL_GMSH_MESSAGE_SIZE + cl_util_get_ulong_number_length(connect_message_size);

      if (connection->data_buffer_size < (connect_message_size + gmsh_message_size + 1) ) {
         return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
      }
      sprintf((char*)connection->data_write_buffer, CL_GMSH_MESSAGE ,
               connect_message_size);
      sprintf((char*)&((connection->data_write_buffer)[gmsh_message_size]), CL_CONNECT_MESSAGE, 
               CL_CONNECT_MESSAGE_VERSION, 
               format_type,
               flow_type,
               connection->sender->comp_host,
               connection->sender->comp_name,
               connection->sender->comp_id,
               connection->receiver->comp_host,
               connection->receiver->comp_name,
               connection->receiver->comp_id,
               connection->local->comp_host,
               connection->local->comp_name,
               connection->local->comp_id
      );

      /* printf("cm:\n%s\n", (char*)&((connection->data_write_buffer)[gmsh_message_size])); */
      connection->data_write_buffer_pos = 0;
      connection->data_write_buffer_processed = 0;
      connection->data_write_buffer_to_send = connect_message_size + gmsh_message_size;
      connection->data_write_flag = CL_COM_DATA_READY;
      connection->connection_sub_state = CL_COM_SEND_CM;
   }

   if (connection->connection_sub_state == CL_COM_SEND_CM) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_SEND_CM");
      if (only_once != 0) {
         data_written = 0;
         retval = cl_com_tcp_write(connection->write_buffer_timeout_time,
                                   con_private->sockfd, 
                                   &(connection->data_write_buffer[(connection->data_write_buffer_pos)]),  /* position to continue */
                                   connection->data_write_buffer_to_send, 
                                   &data_written);               /* returns the data bytes read */
         connection->data_write_buffer_pos = connection->data_write_buffer_pos + data_written;  /* add data read count to buff position */
         connection->data_write_buffer_to_send = connection->data_write_buffer_to_send - data_written;
      } else {
         retval = cl_com_tcp_write(connection->write_buffer_timeout_time,
                                   con_private->sockfd, 
                                   &(connection->data_write_buffer[(connection->data_write_buffer_pos)]),
                                   connection->data_write_buffer_to_send, 
                                   NULL);
         connection->data_write_buffer_pos = connection->data_write_buffer_pos + connection->data_write_buffer_to_send;
      }
      if (retval != CL_RETVAL_OK) {
         return retval;
      }
      connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + connection->data_write_buffer_pos;
      CL_LOG(CL_LOG_INFO,"gettimeofday");
      gettimeofday(&now,NULL);
      connection->read_buffer_timeout_time = now.tv_sec + timeout;
      connection->data_read_buffer_pos = 0;
      connection->data_read_buffer_processed = 0;
      connection->read_gmsh_header->dl = 0;
      connection->data_write_flag = CL_COM_DATA_NOT_READY;
      connection->connection_sub_state = CL_COM_SEND_READ_GMSH;
      connection->write_buffer_timeout_time = 0;
   }


   if (connection->connection_sub_state == CL_COM_SEND_READ_GMSH) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_SEND_READ_GMSH");

      /* read in GMSH header (General Message Size Header)*/
      if (only_once != 0) {
         retval = cl_com_tcp_read_GMSH(connection, &data_read);
      } else {
         retval = cl_com_tcp_read_GMSH(connection, NULL );
      }
      if (retval != CL_RETVAL_OK) {
         return retval;
      }
      connection->connection_sub_state = CL_COM_SEND_READ_CRM;
   }


   if (connection->connection_sub_state == CL_COM_SEND_READ_CRM) {
      CL_LOG(CL_LOG_INFO,"connection state: CL_COM_SEND_READ_CRM");
      CL_LOG_INT(CL_LOG_INFO,"GMSH dl:",connection->read_gmsh_header->dl );

      /* calculate (rest) data size to read = data length - stream buff position */
      data_to_read = connection->read_gmsh_header->dl - (connection->data_read_buffer_pos - connection->data_read_buffer_processed);
      CL_LOG_INT(CL_LOG_INFO,"data to read=",data_to_read);

      if ( (data_to_read + connection->data_read_buffer_pos) >= connection->data_buffer_size ) {
         CL_LOG(CL_LOG_ERROR,"stream buffer to small");
         return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
      }

      /* is data allready in buffer ? */
      if (data_to_read > 0) {
         if (only_once != 0) {
            data_read = 0;
            retval = cl_com_tcp_read(connection->read_buffer_timeout_time,
                                     con_private->sockfd, 
                                     &(connection->data_read_buffer[(connection->data_read_buffer_pos)]),  /* position to continue */
                                     data_to_read, 
                                     &data_read);               /* returns the data bytes read */
            connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_read;  /* add data read count to buff position */
         } else {
            retval = cl_com_tcp_read(connection->read_buffer_timeout_time,
                                     con_private->sockfd, 
                                     &(connection->data_read_buffer[(connection->data_read_buffer_pos)]),
                                     data_to_read, 
                                     NULL);
            connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_to_read; /* add data read count to buff position */
         }
         if (retval != CL_RETVAL_OK) {
            return retval;
         }
      }
      retval = cl_xml_parse_CRM(&(connection->data_read_buffer[(connection->data_read_buffer_processed)]),connection->read_gmsh_header->dl, &crm_message);
      if (retval != CL_RETVAL_OK) {
         cl_com_free_crm_message(&crm_message);
         return retval;
      }


      if ( cl_raw_list_get_elem_count(connection->send_message_list) == 0) {
         connection->data_write_flag = CL_COM_DATA_NOT_READY;
      } else {
         connection->data_write_flag = CL_COM_DATA_READY;
      }
      connection->read_buffer_timeout_time = 0;

      if (crm_message->cs_condition == CL_CRM_CS_CONNECTED) {
         connection->connection_state = CL_COM_CONNECTED;  /* That was it */
         connection->connection_sub_state = CL_COM_WORK; 
         /* printf("local hostname is  : \"%s\"\n", connection->local->comp_host); */
         /* printf("remote hostname is : \"%s\"\n", connection->remote->comp_host); */
      } else {
         CL_LOG_INT(CL_LOG_ERROR,"Connect Error:",crm_message->cs_condition);
         if (connection->error_func != NULL) {
            CL_LOG(CL_LOG_WARNING,"calling application error function");
            switch(crm_message->cs_condition) {
               case CL_CRM_CS_DENIED:
                  connection->error_func(CL_RETVAL_ACCESS_DENIED);
                  break;
               case CL_CRM_CS_ENDPOINT_NOT_UNIQUE:
                  connection->error_func(CL_RETVAL_ENDPOINT_NOT_UNIQUE);
                  break;
               default:
                  connection->error_func(CL_RETVAL_UNKNOWN);
                  break;
            }
            CL_LOG(CL_LOG_WARNING,"application error function returns");

         } else {
            CL_LOG(CL_LOG_WARNING,"No application error function set");
         }
         CL_LOG_STR(CL_LOG_ERROR,"error:",crm_message->cs_text);
         connection->connection_state = CL_COM_CLOSING;  /* That was it */
      }

      CL_LOG_STR(CL_LOG_INFO,"remote resolved component host name (local host) :", crm_message->rdata->comp_host);
      CL_LOG_STR(CL_LOG_INFO,"local resolved component host name (local host) :", connection->local->comp_host);

      CL_LOG_STR(CL_LOG_INFO,"remote resolved component host name (receiver host) :", crm_message->dst->comp_host);
      CL_LOG_STR(CL_LOG_INFO,"local resolved component host name (receiver host) :", connection->receiver->comp_host);

      CL_LOG_STR(CL_LOG_INFO,"remote resolved component host name (sender host) :", crm_message->src->comp_host);
      CL_LOG_STR(CL_LOG_INFO,"local resolved component host name (sender host) :", connection->sender->comp_host);

      connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + connection->data_read_buffer_pos;

      if ( cl_com_compare_hosts(crm_message->rdata->comp_host , connection->local->comp_host    ) != CL_RETVAL_OK ||
           cl_com_compare_hosts(crm_message->dst->comp_host   , connection->receiver->comp_host ) != CL_RETVAL_OK ||
           cl_com_compare_hosts(crm_message->src->comp_host   , connection->sender->comp_host   ) != CL_RETVAL_OK    ) {
         CL_LOG(CL_LOG_ERROR,"host names are not resolved equal");
         connection->connection_state = CL_COM_CLOSING;  /* That was it */
      }

      if ( connection->local->comp_id == 0 ) {
         connection->local->comp_id = crm_message->rdata->comp_id;
         CL_LOG_INT(CL_LOG_INFO,"requested local component id from server is", connection->local->comp_id);
      }
      if ( connection->sender->comp_id == 0 ) {
         connection->sender->comp_id = crm_message->src->comp_id;
         CL_LOG_INT(CL_LOG_INFO,"requested sender component id from server is", connection->sender->comp_id );
      }

      cl_com_free_crm_message(&crm_message);
      /* cl_dump_connection(connection); */
   }
   
   return CL_RETVAL_OK;
}

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
   int retval = CL_RETVAL_OK;
   int do_read_select = 0;
   int do_write_select = 0;
   int my_errno = 0;


   if (connection_list == NULL ) {
      CL_LOG(CL_LOG_ERROR,"no connection list");
      return CL_RETVAL_PARAMS;
   }

   CL_LOG(CL_LOG_INFO,"checking open connections ...");


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
      service_connection->data_read_flag = CL_COM_DATA_NOT_READY;
   }

   /* lock list */
   if ( cl_raw_list_lock(connection_list) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"could not lock connection list");
      return CL_RETVAL_LOCK_ERROR;
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

      if (connection->framework_type == CL_CT_TCP) {
         switch (connection->connection_state) {
            case CL_COM_CONNECTED:
               if (connection->ccrm_sent == 0) {
                  if (do_read_select != 0) {
                     max_fd = MAX(max_fd,con_private->sockfd);
                     FD_SET(con_private->sockfd,&my_read_fds); 
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
            case CL_COM_CONNECTING:
               if (do_read_select != 0) {
                  max_fd = MAX(max_fd,con_private->sockfd);
                  FD_SET(con_private->sockfd,&my_read_fds); 
                  connection->data_read_flag = CL_COM_DATA_NOT_READY;
               }
               if (connection->data_write_flag == CL_COM_DATA_READY && do_write_select != 0) {
                  /* this is to come out of select when data is ready to write */
                  max_fd = MAX(max_fd, con_private->sockfd);
                  FD_SET(con_private->sockfd,&my_write_fds);
                  connection->fd_ready_for_write = CL_COM_DATA_NOT_READY;
               }
               break;
            case CL_COM_OPENING:
               /* this is to come out of select when connection socket is ready to connect */
               if (connection->connection_sub_state == CL_COM_OPEN_CONNECT) { 
                  if ( con_private->sockfd > 0 && do_read_select != 0) {
                     max_fd = MAX(max_fd, con_private->sockfd);
                     FD_SET(con_private->sockfd,&my_write_fds);
                  }
               } 
               break;
         }
      }
      con_elem = cl_connection_list_get_next_elem(connection_list,con_elem);
   }
   cl_raw_list_unlock(connection_list); 


   CL_LOG_INT(CL_LOG_INFO,"max fd =", max_fd);
   if (max_fd == -1) {
      if ( select_mode == CL_W_SELECT ) {
         /* return immediate for only write select ( only called by write thread) */
         return CL_RETVAL_NO_SELECT_DESCRIPTORS;
      }

      if ( select_mode == CL_R_SELECT ) {
         /* return immediate for only read select ( only called by read thread) */
         return CL_RETVAL_NO_SELECT_DESCRIPTORS;
      }

      
      /* we have no file descriptors, but we do a select with standard timeout
         because we don't want to overload the cpu by endless trigger() calls 
         from application when there is no connection client 
         (no descriptors part 1)
      */
      CL_LOG(CL_LOG_WARNING, "no entries in open connection list ... wait");
#if 0
      /* enable this for shorter timeout */
      timeout.tv_sec = 0; 
      timeout.tv_usec = 100*1000;  /* wait for 1/10 second */
#endif
      max_fd = 0;
   }

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
         CL_LOG(CL_LOG_INFO,"----->>>>>>>>>>> select timeout <<<<<<<<<<<<<<<<<<<---");
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
            con_elem = cl_connection_list_get_next_elem(connection_list,con_elem);
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

