#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cl_errors.h"

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

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_get_error_text()"
const char* cl_get_error_text(int error_id) {       /* CR check */

   switch(error_id) {
      case CL_RETVAL_OK                        : {
         return "CL_RETVAL_OK";
      }
      case CL_RETVAL_MALLOC                    : {
         return "CL_RETVAL_MALLOC";
      }
      case CL_RETVAL_PARAMS                    : {
         return "CL_RETVAL_PARAMS";
      }
      case CL_RETVAL_UNKNOWN                   : {
         return "CL_RETVAL_UNKNOWN";
      }
      case CL_RETVAL_MUTEX_ERROR               : {
         return "CL_RETVAL_MUTEX_ERROR";
      }
      case CL_RETVAL_MUTEX_CLEANUP_ERROR       : {
         return "CL_RETVAL_MUTEX_CLEANUP_ERROR";
      }
      case CL_RETVAL_MUTEX_LOCK_ERROR          : {
         return "CL_RETVAL_MUTEX_LOCK_ERROR";
      }
      case CL_RETVAL_MUTEX_UNLOCK_ERROR        : {
         return "CL_RETVAL_MUTEX_UNLOCK_ERROR";
      }
      case CL_RETVAL_CONDITION_ERROR           : {
         return "CL_RETVAL_CONDITION_ERROR";
      }
      case CL_RETVAL_CONDITION_CLEANUP_ERROR   : {
         return "CL_RETVAL_CONDITION_CLEANUP_ERROR";
      }
      case CL_RETVAL_CONDITION_WAIT_TIMEOUT    : {
         return "CL_RETVAL_CONDITION_WAIT_TIMEOUT";
      }
      case CL_RETVAL_CONDITION_SIGNAL_ERROR    : {
         return "CL_RETVAL_CONDITION_SIGNAL_ERROR";
      }
      case CL_RETVAL_THREAD_CREATE_ERROR       : {
         return "CL_RETVAL_THREAD_CREATE_ERROR";
      }
      case CL_RETVAL_THREAD_START_TIMEOUT      : {
         return "CL_RETVAL_THREAD_START_TIMEOUT";
      }
      case CL_RETVAL_THREAD_NOT_FOUND          : {
         return "CL_RETVAL_THREAD_NOT_FOUND";
      }
      case CL_RETVAL_THREAD_JOIN_ERROR         : {
         return "CL_RETVAL_THREAD_JOIN_ERROR";
      }
      case CL_RETVAL_THREAD_CANCELSTATE_ERROR  : {
         return "CL_RETVAL_THREAD_CANCELSTATE_ERROR";
      }
      case CL_RETVAL_LOG_NO_LOGLIST            : {
         return "CL_RETVAL_LOG_NO_LOGLIST";
      }
      case CL_RETVAL_CONNECTION_NOT_FOUND      : {
         return "CL_RETVAL_CONNECTION_NOT_FOUND";
      }
      case CL_RETVAL_HANDLE_NOT_FOUND          : {
         return "CL_RETVAL_HANDLE_NOT_FOUND";
      }
      case CL_RETVAL_THREADS_ENABLED       : {
         return "CL_RETVAL_THREADS_ENABLED";
      }
      case CL_RETVAL_NO_MESSAGE                : {
         return "CL_RETVAL_NO_MESSAGE";
      }
      case CL_RETVAL_CREATE_SOCKET             : {
         return "CL_RETVAL_CREATE_SOCKET";
      }
      case CL_RETVAL_CONNECT_ERROR             : {
         return "CL_RETVAL_CONNECT_ERROR";
      }
      case CL_RETVAL_CONNECT_TIMEOUT           : {
         return "CL_RETVAL_CONNECT_TIMEOUT";
      }
      case CL_RETVAL_NOT_OPEN                  : {
         return "CL_RETVAL_NOT_OPEN";
      }
      case CL_RETVAL_SEND_ERROR                : {
         return "CL_RETVAL_SEND_ERROR";
      }
      case CL_RETVAL_BIND_SOCKET               : {
         return "CL_RETVAL_BIND_SOCKET";
      }
      case CL_RETVAL_SELECT_ERROR              : {
         return "CL_RETVAL_SELECT_ERROR";
      }
      case CL_RETVAL_RECEIVE_ERROR             : {
         return "CL_RETVAL_RECEIVE_ERROR";
      }
      case CL_RETVAL_PIPE_ERROR                : {
         return "CL_RETVAL_PIPE_ERROR";
      }
      case CL_RETVAL_GETHOSTNAME_ERROR         : {
         return "CL_RETVAL_GETHOSTNAME_ERROR";
      }
      case CL_RETVAL_SEND_TIMEOUT              : {
         return "CL_RETVAL_SEND_TIMEOUT";
      }
      case CL_RETVAL_READ_TIMEOUT              : {
         return "CL_RETVAL_READ_TIMEOUT";
      }
      case CL_RETVAL_UNDEFINED_FRAMEWORK       : {
         return "CL_RETVAL_UNDEFINED_FRAMEWORK";
      }
      case CL_RETVAL_NOT_SERVICE_HANDLER       : {
         return "CL_RETVAL_NOT_SERVICE_HANDLER";
      }
      case CL_RETVAL_NO_FRAMEWORK_INIT         : {
         return "CL_RETVAL_NO_FRAMEWORK_INIT";
      }
      case CL_RETVAL_SETSOCKOPT_ERROR          : {
         return "CL_RETVAL_SETSOCKOPT_ERROR";
      }
      case CL_RETVAL_FCNTL_ERROR               : {
         return "CL_RETVAL_FCNTL_ERROR";
      }
      case CL_RETVAL_LISTEN_ERROR              : {
         return "CL_RETVAL_LISTEN_ERROR";
      }
      case CL_RETVAL_NEED_EMPTY_FRAMEWORK      : {
         return "CL_RETVAL_NEED_EMPTY_FRAMEWORK";
      }
      case CL_RETVAL_LOCK_ERROR                : {
         return "CL_RETVAL_LOCK_ERROR";
      }
      case CL_RETVAL_UNLOCK_ERROR              : {
         return "CL_RETVAL_UNLOCK_ERROR";
      }
      case CL_RETVAL_WRONG_FRAMEWORK           : {
         return "CL_RETVAL_WRONG_FRAMEWORK";
      }
      case CL_RETVAL_READ_ERROR                : {
         return "CL_RETVAL_READ_ERROR";
      }
      case CL_RETVAL_MAX_READ_SIZE             : {
         return "CL_RETVAL_MAX_READ_SIZE";
      }
      case CL_RETVAL_CLIENT_WELCOME_ERROR      : {
         return "CL_RETVAL_CLIENT_WELCOME_ERROR";
      }
      case CL_RETVAL_UNKOWN_HOST_ERROR         : {
         return "CL_RETVAL_UNKOWN_HOST_ERROR";
      }
      case CL_RETVAL_LOCAL_HOSTNAME_ERROR      : {
         return "CL_RETVAL_LOCAL_HOSTNAME_ERROR";
      }
      case CL_RETVAL_UNKNOWN_ENDPOINT          : {
         return "CL_RETVAL_UNKNOWN_ENDPOINT";
      }
      case CL_RETVAL_UNCOMPLETE_WRITE          : {
         return "CL_RETVAL_UNCOMPLETE_WRITE";
      }
      case CL_RETVAL_UNCOMPLETE_READ           : {
         return "CL_RETVAL_UNCOMPLETE_READ";
      }
      case CL_RETVAL_LIST_DATA_NOT_EMPTY           : {
         return "CL_RETVAL_LIST_DATA_NOT_EMPTY";
      }
      case CL_RETVAL_LIST_NOT_EMPTY           : {
         return "CL_RETVAL_LIST_NOT_EMPTY";
      }
      case CL_RETVAL_LIST_DATA_IS_NULL: {
         return "CL_RETVAL_LIST_DATA_IS_NULL";
      }
      case CL_RETVAL_THREAD_SETSPECIFIC_ERROR: {
         return "CL_RETVAL_THREAD_SETSPECIFIC_ERROR";
      } 
      case CL_RETVAL_NOT_THREAD_SPECIFIC_INIT: {
         return "CL_RETVAL_NOT_THREAD_SPECIFIC_INIT";
      }
      case CL_RETVAL_ALLREADY_CONNECTED: {
         return "CL_RETVAL_ALLREADY_CONNECTED";
      }
      case CL_RETVAL_STREAM_BUFFER_OVERFLOW: {
         return "CL_RETVAL_STREAM_BUFFER_OVERFLOW";
      }
      case CL_RETVAL_GMSH_ERROR: {
         return "CL_RETVAL_GMSH_ERROR";
      }
      case CL_RETVAL_MESSAGE_ACK_ERROR: {
         return "CL_RETVAL_MESSAGE_ACK_ERROR";
      }
      case CL_RETVAL_MESSAGE_WAIT_FOR_ACK: {
         return "CL_RETVAL_MESSAGE_WAIT_FOR_ACK";
      }
      case CL_RETVAL_ENDPOINT_NOT_UNIQUE: {
         return "CL_RETVAL_ENDPOINT_NOT_UNIQUE";
      }
      case CL_RETVAL_SYNC_RECEIVE_TIMEOUT: {
         return "CL_RETVAL_SYNC_RECEIVE_TIMEOUT";
      }
      case CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR: {
         return "CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR";
      }
      case CL_RETVAL_RESOLVING_SETUP_ERROR: {
         return "CL_RETVAL_RESOLVING_SETUP_ERROR";
      }
      case CL_RETVAL_IP_NOT_RESOLVED_ERROR: {
         return "CL_RETVAL_IP_NOT_RESOLVED_ERROR";
      }
      case CL_RETVAL_MESSAGE_IN_BUFFER: {
         return "CL_RETVAL_MESSAGE_IN_BUFFER";
      }
      case CL_RETVAL_CONNECTION_GOING_DOWN: {
         return "CL_RETVAL_CONNECTION_GOING_DOWN";
      }
      case CL_RETVAL_CONNECTION_STATE_ERROR: {
         return "CL_RETVAL_CONNECTION_STATE_ERROR";
      } 
      case CL_RETVAL_SELECT_TIMEOUT: {
         return "CL_RETVAL_SELECT_TIMEOUT";
      }
      case CL_RETVAL_SELECT_INTERRUPT: {
         return "CL_RETVAL_SELECT_INTERRUPT";
      }
      case CL_RETVAL_NO_SELECT_DESCRIPTORS: {
         return "CL_RETVAL_NO_SELECT_DESCRIPTORS";
      }
      case CL_RETVAL_ALIAS_EXISTS: {
         return "CL_RETVAL_ALIAS_EXISTS";
      }
   }
   return CL_RETVAL_UNDEFINED_STR;
}

