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
         return "no error happened";
      }
      case CL_RETVAL_MALLOC                    : {
         return "can't allocate memory";
      }
      case CL_RETVAL_PARAMS                    : {
         return "got unexpected parameters";
      }
      case CL_RETVAL_UNKNOWN                   : {
         return "can't report a reason";
      }
      case CL_RETVAL_MUTEX_ERROR               : {
         return "got general mutex error";
      }
      case CL_RETVAL_MUTEX_CLEANUP_ERROR       : {
         return "can't cleanup mutex";
      }
      case CL_RETVAL_MUTEX_LOCK_ERROR          : {
         return "can't lock mutex";
      }
      case CL_RETVAL_MUTEX_UNLOCK_ERROR        : {
         return "can't unlock mutex";
      }
      case CL_RETVAL_CONDITION_ERROR           : {
         return "got general thread condition error";
      }
      case CL_RETVAL_CONDITION_CLEANUP_ERROR   : {
         return "can't cleanup thread condition";
      }
      case CL_RETVAL_CONDITION_WAIT_TIMEOUT    : {
         return "timeout while waiting for thread condition";
      }
      case CL_RETVAL_CONDITION_SIGNAL_ERROR    : {
         return "received a signal while waiting for thread condition";
      }
      case CL_RETVAL_THREAD_CREATE_ERROR       : {
         return "can't create thread";
      }
      case CL_RETVAL_THREAD_START_TIMEOUT      : {
         return "timeout while waiting for thread start";
      }
      case CL_RETVAL_THREAD_NOT_FOUND          : {
         return "can't find thread";
      }
      case CL_RETVAL_THREAD_JOIN_ERROR         : {
         return "got thread join error";
      }
      case CL_RETVAL_THREAD_CANCELSTATE_ERROR  : {
         return "got unexpected thread cancel state";
      }
      case CL_RETVAL_LOG_NO_LOGLIST            : {
         return "no log list found";
      }
      case CL_RETVAL_CONNECTION_NOT_FOUND      : {
         return "can't find connection";
      }
      case CL_RETVAL_HANDLE_NOT_FOUND          : {
         return "can't find handle";
      }
      case CL_RETVAL_THREADS_ENABLED       : {
         return "threads are enabled";
      }
      case CL_RETVAL_NO_MESSAGE                : {
         return "got no message";
      }
      case CL_RETVAL_CREATE_SOCKET             : {
         return "can't create socket";
      }
      case CL_RETVAL_CONNECT_ERROR             : {
         return "can't connect to service";
      }
      case CL_RETVAL_CONNECT_TIMEOUT           : {
         return "got connect timeout";
      }
      case CL_RETVAL_NOT_OPEN                  : {
         return "not open error";
      }
      case CL_RETVAL_SEND_ERROR                : {
         return "got send error";
      }
      case CL_RETVAL_BIND_SOCKET               : {
         return "can't bind socket";
      }
      case CL_RETVAL_SELECT_ERROR              : {
         return "got select error";
      }
      case CL_RETVAL_RECEIVE_ERROR             : {
         return "got receive error";
      }
      case CL_RETVAL_PIPE_ERROR                : {
         return "got pipe error";
      }
      case CL_RETVAL_GETHOSTNAME_ERROR         : {
         return "can't resolve host name";
      }
      case CL_RETVAL_SEND_TIMEOUT              : {
         return "got send timeout";
      }
      case CL_RETVAL_READ_TIMEOUT              : {
         return "got read timeout";
      }
      case CL_RETVAL_UNDEFINED_FRAMEWORK       : {
         return "framework is not defined";
      }
      case CL_RETVAL_NOT_SERVICE_HANDLER       : {
         return "handle is not defined as service handler";
      }
      case CL_RETVAL_NO_FRAMEWORK_INIT         : {
         return "framework is not initalized";
      }
      case CL_RETVAL_SETSOCKOPT_ERROR          : {
         return "can't set socket options";
      }
      case CL_RETVAL_FCNTL_ERROR               : {
         return "got fcntl error";
      }
      case CL_RETVAL_LISTEN_ERROR              : {
         return "got listen error";
      }
      case CL_RETVAL_NEED_EMPTY_FRAMEWORK      : {
         return "framework is not uninitalized";
      }
      case CL_RETVAL_LOCK_ERROR                : {
         return "can't lock error";
      }
      case CL_RETVAL_UNLOCK_ERROR              : {
         return "can't unlock error";
      }
      case CL_RETVAL_WRONG_FRAMEWORK           : {
         return "used wrong framework";
      }
      case CL_RETVAL_READ_ERROR                : {
         return "got read error";
      }
      case CL_RETVAL_MAX_READ_SIZE             : {
         return "max read size reached";
      }
      case CL_RETVAL_CLIENT_WELCOME_ERROR      : {
         return "got client welcome error";
      }
      case CL_RETVAL_UNKOWN_HOST_ERROR         : {
         return "unkown host error";
      }
      case CL_RETVAL_LOCAL_HOSTNAME_ERROR      : {
         return "local host name error";
      }
      case CL_RETVAL_UNKNOWN_ENDPOINT          : {
         return "unknown endpoint error";
      }
      case CL_RETVAL_UNCOMPLETE_WRITE          : {
         return "couldn't write all data";
      }
      case CL_RETVAL_UNCOMPLETE_READ           : {
         return "couldn't read all data";
      }
      case CL_RETVAL_LIST_DATA_NOT_EMPTY           : {
         return "list data is not empty";
      }
      case CL_RETVAL_LIST_NOT_EMPTY           : {
         return "list is not empty";
      }
      case CL_RETVAL_LIST_DATA_IS_NULL: {
         return "list data is not initalized";
      }
      case CL_RETVAL_THREAD_SETSPECIFIC_ERROR: {
         return "got error setting thread specific data";
      } 
      case CL_RETVAL_NOT_THREAD_SPECIFIC_INIT: {
         return "could not initialize thread specific data";
      }
      case CL_RETVAL_ALLREADY_CONNECTED: {
         return "already connected error";
      }
      case CL_RETVAL_STREAM_BUFFER_OVERFLOW: {
         return "got stream buffer overflow";
      }
      case CL_RETVAL_GMSH_ERROR: {
         return "can't read general message size header (GMSH)";
      }
      case CL_RETVAL_MESSAGE_ACK_ERROR: {
         return "got message acknowledge error";
      }
      case CL_RETVAL_MESSAGE_WAIT_FOR_ACK: {
         return "got message acknowledge timeout error";
      }
      case CL_RETVAL_ENDPOINT_NOT_UNIQUE: {
         return "endpoint is not unique error";
      }
      case CL_RETVAL_SYNC_RECEIVE_TIMEOUT: {
         return "got syncron message receive timeout error";
      }
      case CL_RETVAL_MAX_MESSAGE_LENGTH_ERROR: {
         return "reached max message length";
      }
      case CL_RETVAL_RESOLVING_SETUP_ERROR: {
         return "resolve setup error";
      }
      case CL_RETVAL_IP_NOT_RESOLVED_ERROR: {
         return "can't resolve ip address";
      }
      case CL_RETVAL_MESSAGE_IN_BUFFER: {
         return "still messages in buffer";
      }
      case CL_RETVAL_CONNECTION_GOING_DOWN: {
         return "connection is going down";
      }
      case CL_RETVAL_CONNECTION_STATE_ERROR: {
         return "general connection state error";
      } 
      case CL_RETVAL_SELECT_TIMEOUT: {
         return "got select timeout";
      }
      case CL_RETVAL_SELECT_INTERRUPT: {
         return "select was interrupted";
      }
      case CL_RETVAL_NO_SELECT_DESCRIPTORS: {
         return "no file descriptors for select available";
      }
      case CL_RETVAL_ALIAS_EXISTS: {
         return "alias is already existing";
      }
      case CL_RETVAL_NO_ALIAS_FILE: {
         return "no alias file specified";
      }
      case CL_RETVAL_ALIAS_FILE_NOT_FOUND: {
         return "could not get alias file";
      }
      case CL_RETVAL_OPEN_ALIAS_FILE_FAILED: {
         return "could not open alias file";
      }
      case CL_RETVAL_ALIAS_VERSION_ERROR: {
         return "wrong alias file version";
      }
      case CL_RETVAL_SECURITY_ANNOUNCE_FAILED: {
         return "security announce failed";
      }
      case CL_RETVAL_SECURITY_SEND_FAILED: {
         return "security send failed";
      } 
      case CL_RETVAL_SECURITY_RECEIVE_FAILED: {
         return "security receive failed";
      }
      case CL_RETVAL_ACCESS_DENIED: {
         return "got access denied";
      }
      case CL_RETVAL_MAX_CON_COUNT_REACHED: {
         return "max. connection count reached";
      }
   }
   return CL_RETVAL_UNDEFINED_STR;
}

