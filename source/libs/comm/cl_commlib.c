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
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include "sge_arch.h"


#include "cl_commlib.h"
#include "cl_handle_list.h"
#include "cl_connection_list.h"
#include "cl_message_list.h"
#include "cl_host_list.h"
#include "cl_endpoint_list.h"
#include "cl_host_alias_list.h"
#include "cl_communication.h"
#include "cl_tcp_framework.h"
#include "cl_util.h"

static int cl_commlib_check_connection_count(cl_com_handle_t* handle);
static int cl_commlib_calculate_statistic(cl_com_handle_t* handle, int lock_list);

static int cl_commlib_handle_connection_read(cl_com_connection_t* connection);
static int cl_commlib_handle_connection_write(cl_com_connection_t* connection);
static int cl_commlib_finish_request_completeness(cl_com_connection_t* connection);

static int cl_commlib_handle_connection_ack_timeouts(cl_com_connection_t* connection);
static int cl_commlib_send_ack_message(cl_com_connection_t* connection, cl_com_message_t* message);
static int cl_commlib_send_ccm_message(cl_com_connection_t* connection);
static int cl_commlib_send_ccrm_message(cl_com_connection_t* connection);
static int cl_commlib_send_sim_message(cl_com_connection_t* connection, unsigned long* mid);
static int cl_commlib_send_sirm_message(cl_com_connection_t* connection,
                                        cl_com_message_t* message,     /* message id of SIM */
                                        unsigned long        starttime,
                                        unsigned long        runtime,
                                        unsigned long        buffered_read_messages,
                                        unsigned long        buffered_write_messages,
                                        unsigned long        connection_count,
                                        unsigned long application_status,
                                        char*                infotext);
static int cl_com_trigger(cl_com_handle_t* handle);

/* threads functions */
static void *cl_com_trigger_thread(void *t_conf);
static void cl_com_trigger_thread_cleanup(void* none);

static void *cl_com_handle_service_thread(void *t_conf);
static void cl_com_handle_service_thread_cleanup(void* none);

static void *cl_com_handle_write_thread(void *t_conf);
static void cl_com_handle_write_thread_cleanup(void* none);

static void *cl_com_handle_read_thread(void *t_conf);
static void cl_com_handle_read_thread_cleanup(void* none);



/* global lists */

/* cl_com_handle_list
 * ==================
 *
 * Each entry in this list is a service handler connection */
static pthread_mutex_t cl_com_handle_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_raw_list_t* cl_com_handle_list = NULL;


/* cl_com_log_list
 * ===============
 *
 * Each entry in this list can be logged via cl_log_list_flush() */
static pthread_mutex_t cl_com_log_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_raw_list_t* cl_com_log_list = NULL;


/* cl_com_host_list
 * ================
 *
 * Each entry in this list is a cached cl_get_hostbyname() 
   or cl_gethostbyaddr() call */
static pthread_mutex_t cl_com_host_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_raw_list_t* cl_com_host_list = NULL;

/* cl_com_endpoint_list
 * ====================
 *
 * Each entry in this list is a known endpoint */
static pthread_mutex_t cl_com_endpoint_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_raw_list_t* cl_com_endpoint_list = NULL;

/* cl_com_thread_list
 * ================
 *
 * Each entry in this list is a cached cl_get_hostbyname() 
   or cl_gethostbyaddr() call */
static pthread_mutex_t cl_com_thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_raw_list_t* cl_com_thread_list = NULL;




/* global flags/variables */

/* cl_com_create_threads
 * =====================
 *
 * If set to 0, no threads are used */
static cl_thread_mode_t       cl_com_create_threads = CL_NO_THREAD;


/* global application function pointer for statistic calculation */
/* see cl_commlib_calculate_statistic() */
static pthread_mutex_t cl_com_application_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_app_status_func_t   cl_com_application_status_func = NULL;

/* global application function pointer for errors */
static pthread_mutex_t cl_com_error_mutex = PTHREAD_MUTEX_INITIALIZER;
static cl_error_func_t   cl_com_error_status_func = NULL;



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_host_list()"
cl_raw_list_t* cl_com_get_host_list(void) {
   cl_raw_list_t* host_list = NULL;
   pthread_mutex_lock(&cl_com_host_list_mutex);
   host_list = cl_com_host_list;
   pthread_mutex_unlock(&cl_com_host_list_mutex);
   return host_list;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_endpoint_list()"
cl_raw_list_t* cl_com_get_endpoint_list(void) {
   cl_raw_list_t* endpoint_list = NULL;
   pthread_mutex_lock(&cl_com_endpoint_list_mutex);
   endpoint_list = cl_com_endpoint_list;
   pthread_mutex_unlock(&cl_com_endpoint_list_mutex);
   return endpoint_list;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_log_list()"
cl_raw_list_t* cl_com_get_log_list(void) {
   cl_raw_list_t* log_list = NULL;
   pthread_mutex_lock(&cl_com_log_list_mutex);
   log_list = cl_com_log_list;
   pthread_mutex_unlock(&cl_com_log_list_mutex);
   return log_list;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_status_func()"
int cl_com_set_status_func(cl_app_status_func_t status_func) {
   pthread_mutex_lock(&cl_com_application_mutex);
   cl_com_application_status_func = *status_func;
   pthread_mutex_unlock(&cl_com_application_mutex);
   return CL_RETVAL_OK;
}



/* The only errors supported by error func calls are:
    CL_RETVAL_UNKNOWN:             when CRM contains unexpected error
    CL_RETVAL_ACCESS_DENIED:       when CRM says that access to service is denied 
    CL_CRM_CS_ENDPOINT_NOT_UNIQUE: when CRM says that there is already an endpoint with this id connected */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_error_func()"
int cl_com_set_error_func(cl_error_func_t error_func) {
   pthread_mutex_lock(&cl_com_error_mutex);
   cl_com_error_status_func = *error_func;
   pthread_mutex_unlock(&cl_com_error_mutex);
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_setup_commlib()"
int cl_com_setup_commlib( cl_thread_mode_t t_mode, int debug_level , cl_log_func_t flush_func ) {
   int ret_val = CL_RETVAL_OK;
   cl_thread_settings_t* thread_p = NULL;
   
   cl_com_create_threads = t_mode;

   /* setup global log list */
   pthread_mutex_lock(&cl_com_log_list_mutex);
   if (cl_com_log_list == NULL) {
      ret_val = cl_log_list_setup(&cl_com_log_list,"application thread",0, /* CL_LOG_FLUSHED */ CL_LOG_IMMEDIATE  , flush_func); 
      if (cl_com_log_list == NULL) {
         pthread_mutex_unlock(&cl_com_log_list_mutex);
         cl_com_cleanup_commlib();
         return ret_val;
      }
   }
   pthread_mutex_unlock(&cl_com_log_list_mutex);
   cl_log_list_set_log_level(cl_com_log_list, debug_level );


   /* setup global cl_com_handle_list */
   pthread_mutex_lock(&cl_com_handle_list_mutex);
   if (cl_com_handle_list == NULL) {
      ret_val = cl_handle_list_setup(&cl_com_handle_list,"handle list");
      if (cl_com_handle_list == NULL) {
         pthread_mutex_unlock(&cl_com_handle_list_mutex);
         cl_com_cleanup_commlib();
         return ret_val;
      }
   }
   pthread_mutex_unlock(&cl_com_handle_list_mutex);

   /* setup global host list */
   pthread_mutex_lock(&cl_com_host_list_mutex);
   if (cl_com_host_list == NULL) {
      ret_val = cl_host_list_setup(&cl_com_host_list, "global_host_cache", CL_SHORT, NULL , NULL, 0 , 0, 0 );
      if (cl_com_host_list == NULL) {
         pthread_mutex_unlock(&cl_com_host_list_mutex);
         cl_com_cleanup_commlib();
         return ret_val;
      }
   }
   pthread_mutex_unlock(&cl_com_host_list_mutex);

   /* setup global endpoint list */
   pthread_mutex_lock(&cl_com_endpoint_list_mutex);
   if (cl_com_endpoint_list == NULL) {
      ret_val = cl_endpoint_list_setup(&cl_com_endpoint_list, "global_endpoint_list" , 0 , 0);
      if (cl_com_endpoint_list == NULL) {
         pthread_mutex_unlock(&cl_com_endpoint_list_mutex);
         cl_com_cleanup_commlib();
         return ret_val;
      }
   }
   pthread_mutex_unlock(&cl_com_endpoint_list_mutex);

   /* setup global thread list */
   pthread_mutex_lock(&cl_com_thread_list_mutex);
   switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         break;
      case CL_ONE_THREAD:
         if (cl_com_thread_list == NULL) {
            ret_val = cl_thread_list_setup(&cl_com_thread_list,"global_thread_list");
            if (cl_com_thread_list == NULL) {
               pthread_mutex_unlock(&cl_com_thread_list_mutex);
               CL_LOG(CL_LOG_ERROR,"could not setup thread list");
               cl_com_cleanup_commlib();
               return ret_val;
            }
            CL_LOG(CL_LOG_INFO,"starting trigger thread ...");
            ret_val = cl_thread_list_create_thread(cl_com_thread_list,
                                                   &thread_p,
                                                   cl_com_log_list,
                                                   "trigger_thread", 1, cl_com_trigger_thread );
            if (ret_val != CL_RETVAL_OK) {
               pthread_mutex_unlock(&cl_com_thread_list_mutex);
               CL_LOG(CL_LOG_ERROR,"could not start trigger_thread");
               cl_com_cleanup_commlib();
               return ret_val;
            }
         }
         break;
   }
   pthread_mutex_unlock(&cl_com_thread_list_mutex);
 
   CL_LOG(CL_LOG_INFO,"ngc library setup done");
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_cleanup_commlib()"
int cl_com_cleanup_commlib(void) {
   int ret_val = CL_RETVAL_OK;
   cl_thread_settings_t* thread_p = NULL;
   cl_handle_list_elem_t* elem = NULL;

   CL_LOG(CL_LOG_INFO,"cleanup commlib ...");

   /* lock handle list mutex */   
   pthread_mutex_lock(&cl_com_handle_list_mutex);

   if (cl_com_handle_list  == NULL) {
      pthread_mutex_unlock(&cl_com_handle_list_mutex);
      /* cleanup allready called or cl_com_setup_commlib() was not called */
      return CL_RETVAL_PARAMS;
   }
 
   /* shutdown all connection handle objects (and threads) */  
   while ( (elem = cl_handle_list_get_first_elem(cl_com_handle_list)) != NULL) {
      cl_commlib_shutdown_handle(elem->handle,0);
   }

   CL_LOG(CL_LOG_INFO,"cleanup thread list ...");
   /* cleanup global thread list */
   pthread_mutex_lock(&cl_com_thread_list_mutex);
   switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         break;
      case CL_ONE_THREAD:
         CL_LOG(CL_LOG_INFO,"shutdown trigger thread ...");
         ret_val = cl_thread_list_delete_thread_by_id(cl_com_thread_list, 1);
         if (ret_val != CL_RETVAL_OK) {
            CL_LOG_STR(CL_LOG_ERROR,"error shutting down trigger thread", cl_get_error_text(ret_val));
         } else {
            CL_LOG(CL_LOG_INFO,"shutdown trigger thread OK");
         }
         break;
   }
   
   while( (thread_p=cl_thread_list_get_first_thread(cl_com_thread_list)) != NULL) {
     CL_LOG(CL_LOG_ERROR,"cleanup of threads did not shutdown all threads ...");
     cl_thread_list_delete_thread(cl_com_thread_list, thread_p);
   }

   cl_thread_list_cleanup(&cl_com_thread_list);
   pthread_mutex_unlock(&cl_com_thread_list_mutex);

   CL_LOG(CL_LOG_INFO,"cleanup thread list done");


   CL_LOG(CL_LOG_INFO,"cleanup handle list ...");

   /* cleanup global cl_com_handle_list */
   cl_handle_list_cleanup(&cl_com_handle_list);
   pthread_mutex_unlock(&cl_com_handle_list_mutex);

   
   CL_LOG(CL_LOG_INFO,"cleanup endpoint list ...");
   pthread_mutex_lock(&cl_com_endpoint_list_mutex);
   cl_endpoint_list_cleanup(&cl_com_endpoint_list);
   pthread_mutex_unlock(&cl_com_endpoint_list_mutex);
    
   CL_LOG(CL_LOG_INFO,"cleanup host list ...");
   pthread_mutex_lock(&cl_com_host_list_mutex);
   cl_host_list_cleanup(&cl_com_host_list);
   pthread_mutex_unlock(&cl_com_host_list_mutex);

   CL_LOG(CL_LOG_INFO,"cleanup log list ...");
   /* cleanup global cl_com_log_list */
   pthread_mutex_lock(&cl_com_log_list_mutex);
   cl_log_list_cleanup(&cl_com_log_list);
   pthread_mutex_unlock(&cl_com_log_list_mutex);
   return CL_RETVAL_OK;
}



/*TODO check this function , check locking */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_set_connection_param()"
int cl_commlib_set_connection_param(cl_com_handle_t* handle, int parameter, int value) {
   if (handle != NULL) {
      switch(parameter) {
         case HEARD_FROM_TIMEOUT:
            handle->last_heard_from_timeout = value;
            /* add a few seconds to actual HEARD_FROM_TIMEOUT, because we want to be sure that
               qmaster will find the execd when try to send a job at end of HEARD_FROM_TIMEOUT intervall */
            cl_endpoint_list_set_entry_life_time(cl_com_get_endpoint_list(), value + handle->open_connection_timeout );
            break;
      }
   }
   return CL_RETVAL_OK;
}
/*TODO check this function  , check locking*/
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_get_connection_param()"
int cl_commlib_get_connection_param(cl_com_handle_t* handle, int parameter, int* value) {
   if (handle != NULL) {
      switch(parameter) {
         case HEARD_FROM_TIMEOUT:
            *value = handle->last_heard_from_timeout;
            break;
      }
   }
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_create_handle()"
cl_com_handle_t* cl_com_create_handle(int framework, int data_flow_type, int service_provider , int handle_port , char* component_name, unsigned long component_id, int select_sec_timeout, int select_usec_timeout) {

   int thread_start_error = 0;
   cl_com_handle_t* new_handle = NULL;
   int return_value = CL_RETVAL_OK;
   int usec_rest = 0;
   int full_usec_seconds = 0;
   int sec_param = 0;
   char help_buffer[80];
   char* local_hostname = NULL;
   cl_handle_list_elem_t* elem = NULL;

   if (cl_com_handle_list  == NULL) {
      CL_LOG(CL_LOG_ERROR,"cl_com_setup_commlib() not called");
      return NULL;
   }

   if (component_name == NULL ) {
      CL_LOG(CL_LOG_ERROR,"component name is NULL");
      return NULL;
   }

   if ( service_provider == 1 && component_id == 0) {
      CL_LOG(CL_LOG_ERROR,"service can't use component id 0");
      return NULL;
   }

   /* first lock handle list */   
   cl_raw_list_lock(cl_com_handle_list);

   elem = cl_handle_list_get_first_elem(cl_com_handle_list);
   while ( elem != NULL) {
      cl_com_endpoint_t* local_endpoint = elem->handle->local;
       
      if (local_endpoint->comp_id == component_id) {
         if (strcmp(local_endpoint->comp_name, component_name) == 0) {
            /* we have this handle allready in list */
            CL_LOG(CL_LOG_ERROR,"component not unique");
            cl_raw_list_unlock(cl_com_handle_list);
            return NULL;
         }
      }
      elem = cl_handle_list_get_next_elem(cl_com_handle_list, elem);
   }

   return_value = cl_com_gethostname(&local_hostname, NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(return_value));
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }
   CL_LOG_STR(CL_LOG_INFO,"local host name is",local_hostname );


   new_handle = (cl_com_handle_t*) malloc(sizeof(cl_com_handle_t));
   if (new_handle == NULL) {
      free(local_hostname);
      CL_LOG(CL_LOG_ERROR,"malloc() error");
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }

   usec_rest = select_usec_timeout % 1000000;           /* usec parameter for select should not be > 1000000 !!!*/
   full_usec_seconds = select_usec_timeout / 1000000;   /* full seconds from timeout_val_usec parameter */
   sec_param = select_sec_timeout + full_usec_seconds;  /* add full seconds from usec parameter to timeout_val_sec parameter */



   new_handle->local = NULL;
   new_handle->messages_ready_for_read = 0;
   new_handle->messages_ready_mutex = NULL;
   new_handle->connection_list_mutex = NULL;
   new_handle->connection_list = NULL;
   new_handle->next_free_client_id = 1;
   new_handle->service_handler = NULL;
   new_handle->framework = framework;
   new_handle->data_flow_type = data_flow_type;
   new_handle->service_provider = service_provider;
   if ( new_handle->service_provider != 0) {
      /* we are service provider */
      new_handle->connect_port = 0;
      if ( handle_port == 0 ) {
         new_handle->service_port = 0;
         CL_LOG(CL_LOG_WARNING,"no port specified, using next free port");
      } else {
         new_handle->service_port = handle_port;
      }
   } else {
      /* we are client, use port for connect */
      new_handle->connect_port = handle_port;
      new_handle->service_port = 0;
   }
   new_handle->connection_timeout = CL_DEFINE_CLIENT_CONNECTION_LIFETIME;
   new_handle->last_heard_from_timeout = CL_DEFINE_CLIENT_CONNECTION_LIFETIME; /* don't use should be removed */
   new_handle->read_timeout = CL_DEFINE_READ_TIMEOUT;
   new_handle->write_timeout = CL_DEFINE_WRITE_TIMEOUT;
   new_handle->open_connection_timeout = CL_DEFINE_GET_CLIENT_CONNECTION_DATA_TIMEOUT;
   new_handle->acknowledge_timeout = CL_DEFINE_ACK_TIMEOUT;
   new_handle->select_sec_timeout = sec_param;
   new_handle->select_usec_timeout = usec_rest;
   new_handle->synchron_receive_timeout = CL_DEFINE_SYNCHRON_RECEIVE_TIMEOUT;  /* default from old commlib */
   new_handle->do_shutdown = 0;  /* no shutdown till now */
   new_handle->max_connection_count_reached = 0;
   new_handle->max_connection_count_found_connection_to_close = 0;
   new_handle->allowed_host_list = NULL;
   
   new_handle->auto_close_mode = CL_CM_AC_DISABLED;
   new_handle->max_open_connections = sysconf(_SC_OPEN_MAX);
   if ( new_handle->max_open_connections < 23 ) {
      CL_LOG_INT(CL_LOG_ERROR, "to less file descriptors:", new_handle->max_open_connections );
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }
   CL_LOG_INT(CL_LOG_INFO, "max file descriptors on this system    :", new_handle->max_open_connections);

   new_handle->max_open_connections = new_handle->max_open_connections - 20;

   CL_LOG_INT(CL_LOG_INFO, "max. used descriptors for communication:", new_handle->max_open_connections);
   
   new_handle->max_con_close_mode = CL_ON_MAX_COUNT_OFF;
   new_handle->app_condition = NULL;
   new_handle->read_condition = NULL;
   new_handle->write_condition = NULL;
   new_handle->service_thread = NULL;
   new_handle->read_thread = NULL;
   new_handle->write_thread = NULL;


   new_handle->statistic = (cl_com_handle_statistic_t*)malloc(sizeof(cl_com_handle_statistic_t));
   if (new_handle->statistic == NULL) {
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }
   memset(new_handle->statistic, 0, sizeof(cl_com_handle_statistic_t));


   gettimeofday(&(new_handle->statistic->last_update),NULL);
   gettimeofday(&(new_handle->start_time),NULL);

   new_handle->messages_ready_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
   if (new_handle->messages_ready_mutex == NULL) {
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }

   new_handle->connection_list_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
   if (new_handle->connection_list_mutex == NULL) {
      free(new_handle->messages_ready_mutex);
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }


   if (pthread_mutex_init(new_handle->messages_ready_mutex, NULL) != 0) {
      int mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->connection_list_mutex);
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   } 

   if (pthread_mutex_init(new_handle->connection_list_mutex, NULL) != 0) {
      int mutex_ret_val;
      mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->connection_list_mutex); 
      }
      mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }


   new_handle->local = cl_com_create_endpoint(local_hostname, component_name, component_id );
   if (new_handle->local == NULL) {
      int mutex_ret_val;
      mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->connection_list_mutex); 
      }
      mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }


   if (cl_connection_list_setup(&(new_handle->connection_list), "connection list", 1) != CL_RETVAL_OK) {
      int mutex_ret_val;
      cl_connection_list_cleanup(&(new_handle->connection_list));
      cl_com_free_endpoint(&(new_handle->local));
      mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->connection_list_mutex); 
      }
      mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   } 

   if (cl_string_list_setup(&(new_handle->allowed_host_list), "allowed host list") != CL_RETVAL_OK) {
      int mutex_ret_val;

      cl_string_list_cleanup(&(new_handle->allowed_host_list));
      cl_connection_list_cleanup(&(new_handle->connection_list));
      cl_com_free_endpoint(&(new_handle->local));
      mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->connection_list_mutex); 
      }
      mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->statistic);
      free(new_handle);
      free(local_hostname);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }
 
 


   /* local host name not needed anymore */
   free(local_hostname);
   local_hostname = NULL;


   if (new_handle->service_provider != 0) {
      /* create service */
      cl_com_connection_t* new_con = NULL;

      CL_LOG(CL_LOG_INFO,"creating service ...");
      switch(new_handle->framework) {
         case CL_CT_TCP: {
            /* autoclose is not used for service connection */
            if (cl_com_setup_tcp_connection(&new_con, new_handle->service_port, new_handle->connect_port,CL_CM_CT_STREAM, CL_CM_AC_UNDEFINED ) != CL_RETVAL_OK) {
               int mutex_ret_val;

               cl_com_close_connection(&new_con);
               cl_string_list_cleanup(&(new_handle->allowed_host_list));
               cl_connection_list_cleanup(&(new_handle->connection_list));
               cl_com_free_endpoint(&(new_handle->local));
               mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
               if (mutex_ret_val != EBUSY) {
                  free(new_handle->connection_list_mutex); 
               }
               mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
               if (mutex_ret_val != EBUSY) {
                  free(new_handle->messages_ready_mutex); 
               }
               free(new_handle->statistic);
               free(new_handle);
               cl_raw_list_unlock(cl_com_handle_list);
               return NULL;
            }
            new_con->handler = new_handle;
            if (cl_com_connection_request_handler_setup(new_con, new_handle->local) != CL_RETVAL_OK) {
               int mutex_ret_val;
               cl_com_connection_request_handler_cleanup(new_con);
               cl_com_close_connection(&new_con);
               cl_string_list_cleanup(&(new_handle->allowed_host_list));
               cl_connection_list_cleanup(&(new_handle->connection_list));
               cl_com_free_endpoint(&(new_handle->local));
               mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
               if (mutex_ret_val != EBUSY) {
                  free(new_handle->connection_list_mutex); 
               }
               mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
               if (mutex_ret_val != EBUSY) {
                  free(new_handle->messages_ready_mutex); 
               }
               free(new_handle->statistic);
               free(new_handle);
               cl_raw_list_unlock(cl_com_handle_list);
               return NULL;
            } 
            
            new_handle->service_handler = new_con;
            if ( new_handle->service_port == 0) {
               cl_com_tcp_private_t* tcp_private = NULL;
               tcp_private = (cl_com_tcp_private_t*) new_con->com_private;
               if ( tcp_private != NULL) {
                  new_handle->service_port = tcp_private->server_port;
               }
            }
            break;
         }
         default: {
            int mutex_ret_val;
            cl_connection_list_cleanup(&(new_handle->connection_list));
            cl_string_list_cleanup(&(new_handle->allowed_host_list));
            cl_com_free_endpoint(&(new_handle->local));
            mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
            if (mutex_ret_val != EBUSY) {
               free(new_handle->connection_list_mutex); 
            }
            mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
            if (mutex_ret_val != EBUSY) {
               free(new_handle->messages_ready_mutex); 
            }
            free(new_handle->statistic);
            free(new_handle);
            cl_raw_list_unlock(cl_com_handle_list);
            return NULL;
         }
      }
   }

   /* create handle thread */
   switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         break;
      case CL_ONE_THREAD:
         CL_LOG(CL_LOG_INFO,"creating read condition ...");
         return_value = cl_thread_create_thread_condition(&(new_handle->read_condition));
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not setup read condition");
            thread_start_error = 1;
            break;
         }

         CL_LOG(CL_LOG_INFO,"creating application read condition ...");
         return_value = cl_thread_create_thread_condition(&(new_handle->app_condition));
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not setup application read condition");
            thread_start_error = 1;
            break;
         }

         CL_LOG(CL_LOG_INFO,"creating write condition ...");
         return_value = cl_thread_create_thread_condition(&(new_handle->write_condition));
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not setup write condition");
            thread_start_error = 1;
            break;
         }


         CL_LOG(CL_LOG_INFO,"starting handle service thread ...");
         snprintf(help_buffer,80,"%s_service",new_handle->local->comp_name);
         return_value = cl_thread_list_create_thread(cl_com_thread_list,
                                                     &(new_handle->service_thread),
                                                     cl_com_log_list,
                                                     help_buffer, 2, cl_com_handle_service_thread );
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not start handle service thread");
            thread_start_error = 1;
            break;
         }

         CL_LOG(CL_LOG_INFO,"starting handle read thread ...");
         snprintf(help_buffer,80,"%s_read",new_handle->local->comp_name);
         return_value = cl_thread_list_create_thread(cl_com_thread_list,
                                                     &(new_handle->read_thread),
                                                     cl_com_log_list,
                                                     help_buffer, 3, cl_com_handle_read_thread );
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not start handle read thread");
            thread_start_error = 1;
            break;
         }

         CL_LOG(CL_LOG_INFO,"starting handle write thread ...");
         snprintf(help_buffer,80,"%s_write",new_handle->local->comp_name);
         return_value = cl_thread_list_create_thread(cl_com_thread_list,
                                                     &(new_handle->write_thread),
                                                     cl_com_log_list,
                                                     help_buffer, 2, cl_com_handle_write_thread );
         if (return_value != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"could not start handle write thread");
            thread_start_error = 1;
            break;
         }
         break;
   }
   if (thread_start_error != 0) {
      int mutex_ret_val;
      if (new_handle->service_handler != NULL) {
         cl_com_connection_request_handler_cleanup(new_handle->service_handler);
         cl_com_close_connection(&(new_handle->service_handler));
      }
      cl_connection_list_cleanup(&(new_handle->connection_list));
      cl_string_list_cleanup(&(new_handle->allowed_host_list));
      cl_com_free_endpoint(&(new_handle->local));
      mutex_ret_val = pthread_mutex_destroy(new_handle->connection_list_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->connection_list_mutex); 
      }
      mutex_ret_val = pthread_mutex_destroy(new_handle->messages_ready_mutex);
      if (mutex_ret_val != EBUSY) {
         free(new_handle->messages_ready_mutex); 
      }
      free(new_handle->statistic);
      free(new_handle);
      cl_raw_list_unlock(cl_com_handle_list);
      return NULL;
   }
   cl_handle_list_append_handle(cl_com_handle_list, new_handle,0);
   cl_raw_list_unlock(cl_com_handle_list);
   CL_LOG(CL_LOG_INFO, "new handle created");
   return new_handle;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_shutdown_handle()"
int cl_commlib_shutdown_handle(cl_com_handle_t* handle, int return_for_messages) {
   cl_connection_list_elem_t* elem = NULL;
   cl_thread_settings_t* thread_settings = NULL;
   struct timeval now;
   cl_bool_t connection_list_empty = CL_FALSE;
   cl_bool_t trigger_write = CL_FALSE;
   int ret_val;


   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (cl_com_handle_list  == NULL) {
      CL_LOG(CL_LOG_ERROR,"cl_com_setup_commlib() not called");
      return CL_RETVAL_PARAMS;
   }

   cl_raw_list_lock(cl_com_handle_list);

   CL_LOG(CL_LOG_INFO,"shutting down handle ...");
   if ( handle->do_shutdown != 1) {
      /* wait for connection close response message from heach connection */
      /* get current timeout time */
      gettimeofday(&now,NULL);
      handle->shutdown_timeout = now.tv_sec + handle->acknowledge_timeout;
   }

   handle->do_shutdown = 1; /* stop accepting new connections */

   /* wait for empty connection list */
   while( connection_list_empty == CL_FALSE) {
      cl_bool_t ignore_timeout = cl_com_get_ignore_timeouts_flag();
      connection_list_empty = CL_TRUE;
      trigger_write = CL_FALSE;

      cl_raw_list_lock(handle->connection_list);
      elem = cl_connection_list_get_first_elem(handle->connection_list);
      while(elem) {
         connection_list_empty = CL_FALSE;
         if ( elem->connection->data_flow_type == CL_CM_CT_MESSAGE) {
            if ( elem->connection->connection_state     == CL_COM_CONNECTED && 
                 elem->connection->connection_sub_state == CL_COM_WORK      &&   
                 elem->connection->ccm_received         == 0)        {
               cl_commlib_send_ccm_message(elem->connection);
               trigger_write = CL_TRUE;
               elem->connection->connection_sub_state = CL_COM_SENDING_CCM;
            }
            CL_LOG_STR(CL_LOG_ERROR,"wait for connection removal, current state is", 
                       cl_com_get_connection_state(elem->connection));
            if ( ignore_timeout == CL_TRUE ) {
               CL_LOG(CL_LOG_WARNING,"we are connected, don't ignore timeouts");
               ignore_timeout = CL_FALSE;
            }
         }
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
      }
      cl_raw_list_unlock(handle->connection_list);


      /*
       * If there are still connections, trigger read/write of messages and
       * return for messages (if return_for_messages is set)
       */
      if ( connection_list_empty == CL_FALSE) {
         int return_value = CL_RETVAL_OK;
         /* still waiting for messages */
         switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               pthread_mutex_lock(handle->messages_ready_mutex);
               if (handle->messages_ready_for_read > 0) {
                  pthread_mutex_unlock(handle->messages_ready_mutex);
                  /* return for messages */
                  if (return_for_messages != 0) {
                     pthread_mutex_unlock(handle->messages_ready_mutex);
                     cl_raw_list_unlock(cl_com_handle_list);
                     CL_LOG(CL_LOG_INFO,"delivering MESSAGES");
                     return CL_RETVAL_MESSAGE_IN_BUFFER;
                  } else {
                     /* delete messages */
                     cl_com_message_t* message = NULL;
                     cl_com_endpoint_t* sender = NULL;
                     cl_commlib_receive_message(handle,NULL, NULL, 0, 0, 0, &message, &sender);
                     if (message != NULL) {
                        CL_LOG(CL_LOG_WARNING,"deleting message");
                        cl_com_free_message(&message);
                     }
                     if (sender != NULL) {
                        CL_LOG_STR(CL_LOG_WARNING,"deleted message from:", sender->comp_host);
                        cl_com_free_endpoint(&sender);
                     }

                  }
               } else {
                  pthread_mutex_unlock(handle->messages_ready_mutex);
               }
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               if (trigger_write == CL_TRUE) {
                  cl_thread_trigger_event(handle->write_thread);
               }


               pthread_mutex_lock(handle->messages_ready_mutex);
               if (handle->messages_ready_for_read > 0) {
                  pthread_mutex_unlock(handle->messages_ready_mutex);

                  if ( return_for_messages != 0) {
                     /* return for messages */
                     pthread_mutex_unlock(handle->messages_ready_mutex);
                     cl_raw_list_unlock(cl_com_handle_list);
                     CL_LOG(CL_LOG_ERROR,"delivering MESSAGES");
                     return CL_RETVAL_MESSAGE_IN_BUFFER;
                  } else {
                     /* delete messages */
                     cl_com_message_t* message = NULL;
                     cl_com_endpoint_t* sender = NULL;
                     cl_commlib_receive_message(handle,NULL, NULL, 0, 0, 0, &message, &sender);
                     if (message != NULL) {
                        CL_LOG(CL_LOG_WARNING,"deleting message");
                        cl_com_free_message(&message);
                     }
                     if (sender != NULL) {
                        CL_LOG_STR(CL_LOG_WARNING,"deleted message from:", sender->comp_host);
                        cl_com_free_endpoint(&sender);
                     }
                  }

               } else {
                  pthread_mutex_unlock(handle->messages_ready_mutex);
               }
               
               CL_LOG(CL_LOG_INFO,"APPLICATION WAITING for CCRM");
               return_value = cl_thread_wait_for_thread_condition(handle->read_condition,
                                                   handle->select_sec_timeout,
                                                   handle->select_usec_timeout);
               if (return_value == CL_RETVAL_CONDITION_WAIT_TIMEOUT) {
                  CL_LOG(CL_LOG_WARNING,"APPLICATION GOT CONDITION WAIT TIMEOUT WHILE WAITING FOR CCRM");
               }
               break;
         }
      }

      /*
       * check timeouts
       */
      gettimeofday(&now,NULL);
      if (handle->shutdown_timeout <= now.tv_sec || ignore_timeout == CL_TRUE  ) {
         CL_LOG(CL_LOG_ERROR,"got timeout while waiting for close response message");
         break;
      }
   }

   /* ok now we shutdown the handle */
   CL_LOG(CL_LOG_INFO,"shutdown of handle");

   /* remove handle from list */
   ret_val = cl_handle_list_remove_handle(cl_com_handle_list, handle, 0);
   cl_raw_list_unlock(cl_com_handle_list);


   /* only shutdown handle if handle was removed from list */
   if (ret_val == CL_RETVAL_OK) {
      /* delete handle thread */
      switch(cl_com_create_threads) {
         case CL_NO_THREAD:
            CL_LOG(CL_LOG_INFO,"no threads enabled");
            break;
         case CL_ONE_THREAD:
   
            CL_LOG(CL_LOG_INFO,"shutdown handle write thread ...");
            thread_settings = handle->write_thread;
            handle->write_thread = NULL; /* set thread pointer to NULL because other threads use this pointer */
            ret_val = cl_thread_list_delete_thread(cl_com_thread_list, thread_settings);
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle write thread", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle write thread OK");
            }
   
            CL_LOG(CL_LOG_INFO,"shutdown handle read thread ...");
            thread_settings = handle->read_thread;
            handle->read_thread = NULL; /* set thread pointer to NULL because other threads use this pointer */
            ret_val = cl_thread_list_delete_thread(cl_com_thread_list, thread_settings);
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle read thread", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle read thread OK");
            }
   
            CL_LOG(CL_LOG_INFO,"shutdown handle service thread ...");
            thread_settings = handle->service_thread;
            handle->service_thread = NULL; /* set thread pointer to NULL because other threads use this pointer */
            ret_val = cl_thread_list_delete_thread(cl_com_thread_list,thread_settings );
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle service thread", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle service thread OK");
            }
   
            /* all threads are down, deleteing condition variables */
            CL_LOG(CL_LOG_INFO,"shutdown handle write condition ...");
            ret_val = cl_thread_delete_thread_condition(&(handle->write_condition));
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle write condition", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle write condition OK");
            }
   
            CL_LOG(CL_LOG_INFO,"shutdown handle application read condition ...");
            ret_val = cl_thread_delete_thread_condition(&(handle->app_condition));
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle application read condition", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle application read condition OK");
            }

            CL_LOG(CL_LOG_INFO,"shutdown handle read condition ...");
            ret_val = cl_thread_delete_thread_condition(&(handle->read_condition));
            if (ret_val != CL_RETVAL_OK) {
               CL_LOG_STR(CL_LOG_ERROR,"error shutting down handle read condition", cl_get_error_text(ret_val));
            } else {
               CL_LOG(CL_LOG_INFO,"shutdown handle read condition OK");
            }
            break;
      }
   
      /* cleanup all connection list entries */
      cl_raw_list_lock(handle->connection_list);
   
      /* mark each connection to close */
      elem = cl_connection_list_get_first_elem(handle->connection_list);
      while(elem) {
         elem->connection->connection_state = CL_COM_CLOSING;
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
      }
      cl_raw_list_unlock(handle->connection_list);
   
      cl_connection_list_destroy_connections_to_close(handle->connection_list,1); /* OK */
      
      /* shutdown of service */
      if (handle->service_provider != 0) {
         cl_com_connection_request_handler_cleanup(handle->service_handler);
         cl_com_close_connection(&(handle->service_handler));
      }
   
      cl_connection_list_cleanup(&(handle->connection_list));
      cl_com_free_endpoint(&(handle->local));
   
      cl_string_list_cleanup(&(handle->allowed_host_list));
    
      if (handle->messages_ready_mutex != NULL) {
         ret_val = pthread_mutex_destroy(handle->messages_ready_mutex);
         if (ret_val != EBUSY) {
            free(handle->messages_ready_mutex); 
            handle->messages_ready_mutex = NULL;
         }
      }
      if (handle->connection_list_mutex != NULL) {
         ret_val = pthread_mutex_destroy(handle->connection_list_mutex);
         if (ret_val != EBUSY) {
            free(handle->connection_list_mutex); 
            handle->connection_list_mutex = NULL;
         }
      }
      free(handle->statistic);
      handle->statistic = NULL;
      free(handle);
      return CL_RETVAL_OK;
   } 
   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_setup_connection()"
int cl_com_setup_connection(cl_com_handle_t* handle, cl_com_connection_t** connection) {
   int ret_val = CL_RETVAL_HANDLE_NOT_FOUND;
   if (handle != NULL) {
      switch(handle->framework) {
         case CL_CT_TCP:   
            ret_val = cl_com_setup_tcp_connection( connection, handle->service_port, handle->connect_port, handle->data_flow_type, handle->auto_close_mode );
            if (ret_val == CL_RETVAL_OK) {
               pthread_mutex_lock(&cl_com_error_mutex);
               (*connection)->error_func = cl_com_error_status_func;
               pthread_mutex_unlock(&cl_com_error_mutex);
            }
         default:
            break;
      }
   }
   return ret_val;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_auto_close_mode()"
int cl_com_set_auto_close_mode(cl_com_handle_t* handle, cl_xml_connection_autoclose_t  mode ) {
   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }
   handle->auto_close_mode = mode;
   switch(mode) {
      case CL_CM_AC_ENABLED: 
          CL_LOG(CL_LOG_INFO,"auto close mode is enabled");
          break;
      case CL_CM_AC_DISABLED:
          CL_LOG(CL_LOG_INFO,"auto close mode is disabled");
          break;
      default:
          CL_LOG(CL_LOG_INFO,"unexpeced auto close mode");
   }
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_auto_close_mode()"
int cl_com_get_auto_close_mode(cl_com_handle_t* handle, cl_xml_connection_autoclose_t* mode ) {
   if (handle == NULL || mode == NULL ) {
      return CL_RETVAL_PARAMS;
   }
   *mode = handle->auto_close_mode;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_max_connection_close_mode()"
int cl_com_set_max_connection_close_mode(cl_com_handle_t* handle, cl_max_count_t mode) {

   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }
   handle->max_connection_count_reached = 0;
   handle->max_con_close_mode = mode;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_max_connection_close_mode()"
int cl_com_get_max_connection_close_mode(cl_com_handle_t* handle, cl_max_count_t* mode) {
   if (handle == NULL|| mode == NULL ) {
      return CL_RETVAL_PARAMS;
   }
   *mode = handle->max_con_close_mode;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_max_connections()"
int cl_com_set_max_connections(cl_com_handle_t* handle, int value) {
   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (value < 1) {
      return CL_RETVAL_PARAMS;
   }
   handle->max_open_connections = value;
   return CL_RETVAL_OK;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_max_connections()"
int cl_com_get_max_connections(cl_com_handle_t* handle, int* value) {
   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }  
   if (value == NULL) {
      return CL_RETVAL_PARAMS;
   } 
   *value = handle->max_open_connections;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_handle()"
cl_com_handle_t* cl_com_get_handle(char* component_name, unsigned long component_id) {
   cl_handle_list_elem_t* elem = NULL;
   cl_com_handle_t* ret_handle = NULL;

   if ( component_name == NULL || cl_com_handle_list  == NULL) {
      CL_LOG(CL_LOG_WARNING,"cl_com_get_handle() - parameter error");
      return NULL;
   }

   /* lock list */
   if ( cl_raw_list_lock(cl_com_handle_list) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_WARNING,"cl_com_get_handle() - lock error");
      return NULL;
   }

   CL_LOG_STR(CL_LOG_INFO,"try to find handle for", component_name);
   if ( component_id != 0) {
      CL_LOG_INT(CL_LOG_INFO,"handle must have id", component_id);
   } else {
      CL_LOG(CL_LOG_INFO,"ignoring component_id");
   }
   elem = cl_handle_list_get_first_elem(cl_com_handle_list);
   while ( elem != NULL) { 
      cl_com_handle_t* handle = elem->handle;

      /* if component id is zero, we just search for the name */
      if (handle->local->comp_id == component_id || component_id == 0) {
         if (strcmp(handle->local->comp_name, component_name) == 0) {
            if ( ret_handle != NULL) {
               CL_LOG(CL_LOG_ERROR,"cl_com_get_handle() - found more than one handle");
            } else {
               ret_handle = handle;
            }
         }
      }
      elem = cl_handle_list_get_next_elem(cl_com_handle_list, elem);
   }
   
   /* unlock list */
   if ( cl_raw_list_unlock(cl_com_handle_list ) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_WARNING,"cl_com_get_handle() - unlock error");
      return NULL;
   }

   if (ret_handle == NULL) {
      CL_LOG(CL_LOG_INFO,"cl_com_get_handle() - handle not found");
   }
   return ret_handle;
}


cl_thread_mode_t cl_commlib_get_thread_state(void) {
   return cl_com_create_threads;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_alias_file()"
int cl_com_set_alias_file(char* alias_file) {
   int ret_val;
   if (alias_file == NULL) {
      return CL_RETVAL_PARAMS;
   }
   
   pthread_mutex_lock(&cl_com_host_list_mutex);
   if (cl_com_host_list != NULL) {
      ret_val = cl_host_list_set_alias_file(cl_com_host_list, alias_file );
   } else {
      ret_val = CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   pthread_mutex_unlock(&cl_com_host_list_mutex);
   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_alias_file_dirty()"
int cl_com_set_alias_file_dirty(void) {
   int ret_val;
   
   pthread_mutex_lock(&cl_com_host_list_mutex);
   if (cl_com_host_list != NULL) {
      ret_val = cl_host_list_set_alias_file_dirty(cl_com_host_list);
   } else {
      ret_val = CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   pthread_mutex_unlock(&cl_com_host_list_mutex);
   return ret_val;

}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_append_host_alias()"
int cl_com_append_host_alias(char* local_resolved_name, char* alias_name) {
   int ret_val = CL_RETVAL_OK;
   cl_host_list_data_t* ldata = NULL;
   if (local_resolved_name == NULL || alias_name == NULL) {
      return CL_RETVAL_PARAMS;
   }

   ldata = cl_host_list_get_data(cl_com_get_host_list());
   if (ldata == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }
   ret_val = cl_host_alias_list_append_host(ldata->host_alias_list, local_resolved_name, alias_name, 1 );
   if (ret_val == CL_RETVAL_OK) {
      CL_LOG(CL_LOG_INFO,"added host alias:");
      CL_LOG_STR(CL_LOG_INFO,"local resolved name:",local_resolved_name );
      CL_LOG_STR(CL_LOG_INFO,"aliased name       :",alias_name);
   }
   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_remove_host_alias()"
int cl_com_remove_host_alias(char* alias_name) {
   int ret_val = CL_RETVAL_OK;
   cl_host_list_data_t* ldata = NULL;
   cl_host_alias_list_elem_t* elem = NULL;
   if ( alias_name == NULL) {
      return CL_RETVAL_PARAMS;
   }

   ldata = cl_host_list_get_data(cl_com_get_host_list());
   if (ldata == NULL) {
      return CL_RETVAL_NO_FRAMEWORK_INIT;
   }

   cl_raw_list_lock(ldata->host_alias_list);
   elem = cl_host_alias_list_get_first_elem(ldata->host_alias_list);
   while(elem) {
      if (strcmp(elem->alias_name,alias_name) == 0) {
         CL_LOG(CL_LOG_INFO,"removing host alias:");
         CL_LOG_STR(CL_LOG_INFO,"local resolved name:",elem->local_resolved_hostname );
         CL_LOG_STR(CL_LOG_INFO,"aliased name       :",elem->alias_name);

         ret_val = cl_host_alias_list_remove_host(ldata->host_alias_list,elem,0);
         cl_raw_list_unlock(ldata->host_alias_list);
         if (ret_val != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"error removing host alias");
         }
         return ret_val;
      }
      elem = cl_host_alias_list_get_next_elem(ldata->host_alias_list,elem);
   }
   cl_raw_list_unlock(ldata->host_alias_list);
   return CL_RETVAL_UNKNOWN;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_append_known_endpoint_from_name()"
int cl_com_append_known_endpoint_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, int comp_port, cl_xml_connection_autoclose_t autoclose, int is_static) {

   int retval = CL_RETVAL_OK;
   int function_return = CL_RETVAL_OK;

   char* resolved_hostname = NULL;
   cl_com_endpoint_t* endpoint = NULL;

   if ( unresolved_comp_host == NULL || comp_name == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   retval = cl_com_cached_gethostbyname(unresolved_comp_host, &resolved_hostname, NULL, NULL, NULL );
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_ERROR,"could not resolve host",unresolved_comp_host);
      return retval;
   }

   endpoint = cl_com_create_endpoint(resolved_hostname,comp_name , comp_id );
   if (endpoint == NULL) {
      free(resolved_hostname); 
      return CL_RETVAL_MALLOC;
   }

   function_return = cl_com_append_known_endpoint(endpoint, comp_port, autoclose , is_static );

   free(resolved_hostname); 
   cl_com_free_endpoint(&endpoint);
   
   return function_return;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_append_known_endpoint()"
int cl_com_append_known_endpoint(cl_com_endpoint_t* endpoint, int service_port, cl_xml_connection_autoclose_t autoclose, int is_static ) {
   if (endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }
   return cl_endpoint_list_define_endpoint(cl_com_get_endpoint_list(), endpoint, service_port, autoclose, is_static);
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_remove_known_endpoint()"
int cl_com_remove_known_endpoint(cl_com_endpoint_t* endpoint) {
   if (endpoint == NULL) {
      return CL_RETVAL_PARAMS;
   }
   return cl_endpoint_list_undefine_endpoint(cl_com_get_endpoint_list(), endpoint);
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_known_endpoint_autoclose_mode()"
int cl_com_get_known_endpoint_autoclose_mode(cl_com_endpoint_t* endpoint, cl_xml_connection_autoclose_t* auto_close_mode ) {
   if (endpoint == NULL || auto_close_mode == NULL) {
      return CL_RETVAL_PARAMS;
   }

   return cl_endpoint_list_get_autoclose_mode(cl_com_get_endpoint_list(), endpoint,  auto_close_mode);

}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_known_endpoint_port()"
int cl_com_get_known_endpoint_port(cl_com_endpoint_t* endpoint, int* service_port ) {
   if (endpoint == NULL || service_port == NULL) {
      return CL_RETVAL_PARAMS;
   }

   return cl_endpoint_list_get_service_port(cl_com_get_endpoint_list(), endpoint, service_port);
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_remove_known_endpoint_from_name()"
int cl_com_remove_known_endpoint_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id) {
   int retval = CL_RETVAL_OK;
   int function_return = CL_RETVAL_OK;

   char* resolved_hostname = NULL;
   cl_com_endpoint_t* endpoint = NULL;

   if ( unresolved_comp_host == NULL || comp_name == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   retval = cl_com_cached_gethostbyname(unresolved_comp_host, &resolved_hostname, NULL, NULL, NULL );
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_ERROR,"could not resolve host",unresolved_comp_host);
      return retval;
   }

   endpoint = cl_com_create_endpoint(resolved_hostname,comp_name , comp_id );
   if (endpoint == NULL) {
      free(resolved_hostname); 
      return CL_RETVAL_MALLOC;
   }

   function_return = cl_com_remove_known_endpoint(endpoint);

   free(resolved_hostname); 
   cl_com_free_endpoint(&endpoint);
   
   return function_return;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_known_endpoint_autoclose_mode_from_name()"
int cl_com_get_known_endpoint_autoclose_mode_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, cl_xml_connection_autoclose_t* auto_close_mode ) {
  int retval = CL_RETVAL_OK;
   int function_return = CL_RETVAL_OK;

   char* resolved_hostname = NULL;
   cl_com_endpoint_t* endpoint = NULL;

   if ( unresolved_comp_host == NULL || comp_name == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   retval = cl_com_cached_gethostbyname(unresolved_comp_host, &resolved_hostname, NULL, NULL, NULL );
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_ERROR,"could not resolve host",unresolved_comp_host);
      return retval;
   }

   endpoint = cl_com_create_endpoint(resolved_hostname,comp_name , comp_id );
   if (endpoint == NULL) {
      free(resolved_hostname); 
      return CL_RETVAL_MALLOC;
   }

   function_return = cl_com_get_known_endpoint_autoclose_mode(endpoint, auto_close_mode);

   free(resolved_hostname); 
   cl_com_free_endpoint(&endpoint);
   
   return function_return;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_known_service_port_from_name()"
int cl_com_get_known_endpoint_port_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, int* service_port ) {
   int retval = CL_RETVAL_OK;
   int function_return = CL_RETVAL_OK;

   char* resolved_hostname = NULL;
   cl_com_endpoint_t* endpoint = NULL;

   if ( unresolved_comp_host == NULL || comp_name == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   retval = cl_com_cached_gethostbyname(unresolved_comp_host, &resolved_hostname, NULL, NULL, NULL );
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_ERROR,"could not resolve host",unresolved_comp_host);
      return retval;
   }

   endpoint = cl_com_create_endpoint(resolved_hostname,comp_name , comp_id );
   if (endpoint == NULL) {
      free(resolved_hostname); 
      return CL_RETVAL_MALLOC;
   }

   function_return = cl_com_get_known_endpoint_port(endpoint,service_port);

   free(resolved_hostname); 
   cl_com_free_endpoint(&endpoint);
   
   return function_return;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_service_fd()"
int cl_com_set_handle_fds(cl_com_handle_t* handle, fd_set* file_descriptor_set) {
   int fd;
   int ret_val = CL_RETVAL_UNKNOWN;
   cl_connection_list_elem_t* elem = NULL;
   cl_com_connection_t* connection = NULL;

   if (handle == NULL || file_descriptor_set == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (handle->service_handler != NULL) {
      switch(handle->framework) {
         case CL_CT_TCP:
            cl_com_tcp_get_fd(handle->service_handler, &fd);
            if (fd >= 0) {
               FD_SET(fd, file_descriptor_set);
               ret_val = CL_RETVAL_OK;
            }
      }
   }

   cl_raw_list_lock(handle->connection_list);
   elem = cl_connection_list_get_first_elem(handle->connection_list);    
   while(elem) {
      connection = elem->connection;
      switch(handle->framework) {
         case CL_CT_TCP:
            cl_com_tcp_get_fd(connection, &fd);
            if (fd >= 0) {
               FD_SET(fd,file_descriptor_set);
               ret_val = CL_RETVAL_OK;
            }
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
   }
   cl_raw_list_unlock(handle->connection_list);
   
   return ret_val;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_service_port()"
int cl_com_get_service_port(cl_com_handle_t* handle, int* port) {
   if (handle == NULL || port == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (handle->service_provider == 0) {
      CL_LOG(CL_LOG_WARNING,"no service running");
      *port = -1;
      return CL_RETVAL_UNKNOWN;
   }

   if (handle->service_handler == NULL) {
      CL_LOG(CL_LOG_ERROR,"no service handler found");
      *port = -1;
      return CL_RETVAL_UNKNOWN;
   }
   switch(handle->framework) {
      case CL_CT_TCP:
         return cl_com_tcp_get_service_port(handle->service_handler, port);
      default:
         break;
   }
   return CL_RETVAL_UNKNOWN;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_set_synchron_receive_timeout()"
int cl_com_set_synchron_receive_timeout(cl_com_handle_t* handle, int timeout) {
   if (handle == NULL || timeout <= 0 ) {
      CL_LOG(CL_LOG_ERROR,"error setting synchron receive timeout");
      return CL_RETVAL_PARAMS;
   }
   CL_LOG_INT(CL_LOG_INFO,"setting synchron receive timeout to", timeout);
   handle->synchron_receive_timeout = timeout;
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_get_connect_port()"
int cl_com_get_connect_port(cl_com_handle_t* handle, int* port) {
   if (handle == NULL || port == NULL) {
      return CL_RETVAL_PARAMS;
   }
   *port = handle->connect_port;
   return CL_RETVAL_UNKNOWN;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_add_allowed_host()"
int cl_com_add_allowed_host(cl_com_handle_t* handle, char* hostname) {
   int retval = CL_RETVAL_OK;
   char* resolved_name = NULL;
   if (handle == NULL) {
      CL_LOG(CL_LOG_ERROR,"no handle specified");
      return CL_RETVAL_PARAMS;
   }
   if (hostname == NULL) {
      CL_LOG(CL_LOG_ERROR,"no host specified");
      return CL_RETVAL_PARAMS;
   }
   retval = cl_com_cached_gethostbyname(hostname, &resolved_name, NULL, NULL, NULL );
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_ERROR,"could not resolve host",hostname);
      return retval;
   }
   free(resolved_name);
   resolved_name = NULL;
   retval = cl_string_list_append_string(handle->allowed_host_list, hostname ,1);
   if (retval != CL_RETVAL_OK) {
      CL_LOG_STR(CL_LOG_WARNING,"could not add host to allowed host list:", hostname);
   } else {
      CL_LOG_STR(CL_LOG_INFO,"added host to allowed host list:", hostname);
   }
   return retval;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_remove_allowed_host()"
int cl_com_remove_allowed_host(cl_com_handle_t* handle, char* hostname) {
   if (handle == NULL) {
      CL_LOG(CL_LOG_ERROR,"no handle specified");
      return CL_RETVAL_PARAMS;
   }
   if (hostname == NULL) {
      CL_LOG(CL_LOG_ERROR,"no host specified");
      return CL_RETVAL_PARAMS;
   }
   return cl_string_list_remove_string(handle->allowed_host_list, hostname ,1);
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_trigger()"
int cl_commlib_trigger(cl_com_handle_t* handle) {

   int ret_val;

   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }
   switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         return cl_com_trigger(handle);
      case CL_ONE_THREAD:
         /* application has nothing to do, wait for next message */
         pthread_mutex_lock(handle->messages_ready_mutex);
         if (handle->messages_ready_for_read == 0) {
            CL_LOG(CL_LOG_INFO,"NO MESSAGES to READ, WAITING ...");
            pthread_mutex_unlock(handle->messages_ready_mutex);
            ret_val = cl_thread_wait_for_thread_condition(handle->read_condition,
                                                         handle->select_sec_timeout,
                                                         handle->select_usec_timeout);
         } else {
            pthread_mutex_unlock(handle->messages_ready_mutex);
         }
         return CL_RETVAL_THREADS_ENABLED;
   }
   return CL_RETVAL_UNKNOWN;
}

/* this function is used for no thread implementation and must be called 
   permanent to get data into the data structures and lists */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_trigger()"
static int cl_com_trigger(cl_com_handle_t* handle) {
   cl_connection_list_elem_t* elem = NULL;
   
   struct timeval now;
   int retval = CL_RETVAL_OK;
   cl_com_connection_t* the_handler = NULL;


   if (handle == NULL) {
      CL_LOG(CL_LOG_ERROR,"no handle specified");
      return CL_RETVAL_PARAMS;
   }
 
   /* when threads enabled: this is done by cl_com_trigger_thread() - OK */
   if (cl_commlib_get_thread_state() == CL_NO_THREAD) {
      cl_com_host_list_refresh(cl_com_get_host_list());
      cl_com_endpoint_list_refresh(cl_com_get_endpoint_list());
   }

   /* remove broken connections */
   /* when threads enabled: this is done by cl_com_handle_service_thread() - OK */
   cl_connection_list_destroy_connections_to_close(handle->connection_list,1); /* OK */

   /* calculate statistics each second */
   gettimeofday(&now,NULL);
   if (handle->statistic->last_update.tv_sec != now.tv_sec) {
      cl_commlib_calculate_statistic(handle, 1);
   }


   /* check number of connections */
   cl_commlib_check_connection_count(handle);


   /* get service handle for virtual select call */
   if (handle->do_shutdown == 0 && handle->max_connection_count_reached == 0) {
      the_handler = handle->service_handler;
   } else {
      the_handler = NULL;
   }
  
   /* do virtual select */
   retval = cl_com_open_connection_request_handler(handle->framework, 
                                          handle->connection_list, 
                                          the_handler,
                                          handle->select_sec_timeout,
                                          handle->select_usec_timeout,
                                          CL_RW_SELECT);


   /* when threads enabled: this is done by cl_com_handle_service_thread() */
   if (handle->service_provider                != 0 && 
       handle->do_shutdown                     == 0 && 
       handle->service_handler->data_read_flag == CL_COM_DATA_READY &&
       handle->max_connection_count_reached    == 0 ) {

      /* new connection requests */
      cl_com_connection_t* new_con = NULL;


      /* check without timeout */
      cl_com_connection_request_handler(handle->service_handler, &new_con, 0, 0 );

      if (new_con != NULL) {
         handle->statistic->new_connections = handle->statistic->new_connections + 1;
         new_con->handler = handle->service_handler->handler;
         CL_LOG(CL_LOG_INFO,"adding new client");
         new_con->read_buffer_timeout_time = now.tv_sec + handle->open_connection_timeout;
         cl_connection_list_append_connection(handle->connection_list, new_con,1);
         new_con = NULL;
      }
   }


   /* read / write messages */
   cl_raw_list_lock(handle->connection_list);

   elem = cl_connection_list_get_first_elem(handle->connection_list);     
   
   while(elem) {
      if (elem->connection->connection_state == CL_COM_DISCONNECTED) {
         /* open connection if there are messages to send */
         if ( cl_raw_list_get_elem_count(elem->connection->send_message_list) > 0) {
            CL_LOG(CL_LOG_INFO,"setting connection state to CL_COM_OPENING");
            elem->connection->connection_state = CL_COM_OPENING;
         }
      }

      if (elem->connection->connection_state == CL_COM_OPENING) {
         int return_value;
         /* trigger connect */
         return_value = cl_com_open_connection(elem->connection, handle->open_connection_timeout,NULL , NULL, NULL, NULL);
         if (return_value != CL_RETVAL_OK && return_value != CL_RETVAL_UNCOMPLETE_WRITE ) {
            CL_LOG_STR(CL_LOG_ERROR,"could not open connection:",cl_get_error_text(return_value));
            elem->connection->connection_state = CL_COM_CLOSING;
         }
         CL_LOG(CL_LOG_INFO,"OPENING...");
      }

      if (elem->connection->connection_state == CL_COM_CONNECTING) {
         int return_value;
         if (elem->connection->data_read_flag == CL_COM_DATA_READY || elem->connection->fd_ready_for_write == CL_COM_DATA_READY ) {
            return_value = cl_com_tcp_connection_complete_request(elem->connection,handle->open_connection_timeout,1);
            if (return_value != CL_RETVAL_OK && 
                return_value != CL_RETVAL_UNCOMPLETE_READ && 
                return_value != CL_RETVAL_UNCOMPLETE_WRITE && 
                return_value != CL_RETVAL_SELECT_ERROR ) {
               CL_LOG_STR(CL_LOG_ERROR,"connection establish error:",cl_get_error_text(return_value));
               elem->connection->connection_state = CL_COM_CLOSING;
            }
            if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
               elem->connection->connection_state = CL_COM_CLOSING;
            } 
            if ( elem->connection->connection_state == CL_COM_CONNECTED ) {
               cl_commlib_finish_request_completeness(elem->connection);
               /* connection is now in connect state, do select before next reading */
               elem->connection->data_read_flag = CL_COM_DATA_NOT_READY;
            }
         } else {
            /* check timeouts */
            if ( elem->connection->read_buffer_timeout_time != 0) {
               if ( now.tv_sec >= elem->connection->read_buffer_timeout_time ) {
                  CL_LOG(CL_LOG_ERROR,"read timeout for connection completion");
                  elem->connection->connection_state = CL_COM_CLOSING;
               }
            }
            if ( elem->connection->write_buffer_timeout_time != 0) {
               if ( now.tv_sec >= elem->connection->write_buffer_timeout_time ) {
                  CL_LOG(CL_LOG_ERROR,"write timeout for connection completion");
                  elem->connection->connection_state = CL_COM_CLOSING;
               }
            }
         }
      }

      if (elem->connection->connection_state == CL_COM_CONNECTED) {
         /* check ack timeouts */
         int return_value = cl_commlib_handle_connection_ack_timeouts(elem->connection); 

         if (elem->connection->data_read_flag == CL_COM_DATA_READY && 
             elem->connection->ccrm_sent      == 0                 && 
             elem->connection->ccrm_received  == 0                    ) {
            return_value = cl_commlib_handle_connection_read(elem->connection);
            if ( return_value != CL_RETVAL_OK && 
                 return_value != CL_RETVAL_UNCOMPLETE_READ && 
                 return_value != CL_RETVAL_SELECT_ERROR ) {
               elem->connection->connection_state = CL_COM_CLOSING;
               CL_LOG_STR(CL_LOG_WARNING,"read from connection: setting close flag! Reason:", cl_get_error_text(return_value));
            }
            if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
               elem->connection->connection_state = CL_COM_CLOSING;
            }
         } else {
            /* check timeouts */
            if ( elem->connection->read_buffer_timeout_time != 0) {
               if ( now.tv_sec >= elem->connection->read_buffer_timeout_time ) {
                  CL_LOG(CL_LOG_ERROR,"connection read timeout");
                  elem->connection->connection_state = CL_COM_CLOSING;
               }
            }
         }
         
         if (elem->connection->fd_ready_for_write == CL_COM_DATA_READY && elem->connection->ccrm_sent == 0 ) {
            return_value = cl_commlib_handle_connection_write(elem->connection);
            if (return_value != CL_RETVAL_OK && 
                return_value != CL_RETVAL_UNCOMPLETE_WRITE && 
                return_value != CL_RETVAL_SELECT_ERROR ) {
               elem->connection->connection_state = CL_COM_CLOSING;
               CL_LOG_STR(CL_LOG_WARNING,"write to connection: setting close flag! Reason:", cl_get_error_text(return_value));
            }
            if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
               elem->connection->connection_state = CL_COM_CLOSING;
            }
         } else {
            /* check timeouts */
            if ( elem->connection->write_buffer_timeout_time != 0) {
               if ( now.tv_sec >= elem->connection->write_buffer_timeout_time ) {
                  CL_LOG(CL_LOG_ERROR,"write timeout for connection completion");
                  elem->connection->connection_state = CL_COM_CLOSING;
               }
            }
         }

         /* send connection close response message (if no message are in buffers) */
         if (elem->connection->ccm_received == 1 ) {
            CL_LOG(CL_LOG_INFO,"received ccm");

            CL_LOG_INT(CL_LOG_INFO,"receive buffer:",cl_raw_list_get_elem_count(elem->connection->received_message_list) );
            CL_LOG_INT(CL_LOG_INFO,"send buffer   :",cl_raw_list_get_elem_count(elem->connection->send_message_list) );

            if( cl_raw_list_get_elem_count(elem->connection->send_message_list) == 0 && 
                cl_raw_list_get_elem_count(elem->connection->received_message_list) == 0) {
               elem->connection->ccm_received = 2;
               elem->connection->connection_sub_state = CL_COM_SENDING_CCRM;
               /* disable #if 1 if you want to test a client, that is frozen! */
               cl_commlib_send_ccrm_message(elem->connection);
               CL_LOG(CL_LOG_INFO,"sending ccrm");
            } else {
               CL_LOG(CL_LOG_WARNING,"can't send ccrm, still messages in buffer");
            }
         }
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
   }
   cl_raw_list_unlock(handle->connection_list);

   return retval;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_handle_connection_read()"


/* WARNING: connection_list must be locked by caller */
static int cl_commlib_handle_connection_read(cl_com_connection_t* connection) {
   cl_com_message_t* message = NULL;
   cl_message_list_elem_t* message_list_elem = NULL;
   unsigned long size = 0;
   int return_value = CL_RETVAL_OK;
   struct timeval now;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (connection->data_flow_type == CL_CM_CT_STREAM) {
      return_value = cl_com_create_message(&message);
      if (return_value != CL_RETVAL_OK) {
         return return_value;   
      }
      gettimeofday(&now,NULL);
      connection->read_buffer_timeout_time = now.tv_sec + connection->handler->read_timeout;

      return_value = cl_com_receive_message(connection, 
                                            connection->read_buffer_timeout_time , 
                                            connection->data_read_buffer, 
                                            connection->data_buffer_size, 
                                            &size );
      connection->read_buffer_timeout_time = 0 ;
      if (return_value != CL_RETVAL_OK && return_value != CL_RETVAL_UNCOMPLETE_READ) {
         cl_com_free_message(&message);
         return return_value;
      }

      
      CL_LOG_STR(CL_LOG_INFO,"received stream message from:", connection->receiver->comp_host);
      message->message_state  = CL_MS_READY;         /* set message state */
      message->message_mat    = CL_MIH_MAT_NAK;      /* no acknoledge for stream messages */
      message->message_length = size;                    /* set message size */
      gettimeofday(&message->message_receive_time,NULL);   /* set receive time */


      /* Touch endpoint, he is still active */
      if (connection->framework_type == CL_CT_TCP ) {
         cl_com_tcp_private_t* tcp_private = NULL;
         tcp_private = (cl_com_tcp_private_t*) connection->com_private;
         if (tcp_private != NULL) {
            cl_endpoint_list_define_endpoint(cl_com_get_endpoint_list(),connection->remote, tcp_private->connect_port, connection->auto_close_type, 0 );
         }
      }
      
      /* set last transfer time of connection */
      memcpy(&connection->last_transfer_time, &message->message_receive_time ,sizeof (struct timeval));

      message->message = (cl_byte_t*) malloc(sizeof(cl_byte_t) * size);
      if (message->message == NULL) {
         cl_com_free_message(&message);
         return CL_RETVAL_MALLOC;   
      }
      memcpy(message->message, connection->data_read_buffer , sizeof(cl_byte_t) * size);
      return_value = cl_message_list_append_message(connection->received_message_list, message, 1);
      if (return_value == CL_RETVAL_OK) {
         if (connection->handler != NULL) { 
            cl_com_handle_t* handle = connection->handler;
            /* increase counter for ready messages */
            pthread_mutex_lock(handle->messages_ready_mutex);
            handle->messages_ready_for_read = handle->messages_ready_for_read + 1;
            pthread_mutex_unlock(handle->messages_ready_mutex);
         }
      }
      connection->statistic->bytes_received = connection->statistic->bytes_received + size;
      connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + size;
      return return_value;
   }

   if (connection->data_flow_type == CL_CM_CT_MESSAGE) {

      cl_raw_list_lock(connection->received_message_list);

      message = NULL;
      /* try to find actual message */
      message_list_elem = cl_message_list_get_least_elem(connection->received_message_list);
      if (message_list_elem != NULL) {
         message = message_list_elem->message;
         if (message->message_state == CL_MS_READY || message->message_state == CL_MS_PROTOCOL) {
            message = NULL; /* allready read, create new message */
         }
      }
     
      if (message == NULL) {
         /* create a new message */
         CL_LOG(CL_LOG_INFO,"creating new empty read message");
         
         return_value = cl_com_create_message(&message);
         if (return_value != CL_RETVAL_OK) {
            cl_raw_list_unlock(connection->received_message_list);
            return return_value;   
         }
         message->message_state = CL_MS_INIT_RCV;
         return_value = cl_message_list_append_message(connection->received_message_list, message, 0);
         if (return_value != CL_RETVAL_OK) {
            cl_raw_list_unlock(connection->received_message_list);
            return return_value;   
         }
      } 
         
      if (message->message_state == CL_MS_INIT_RCV) {
         struct timeval now;

         CL_LOG(CL_LOG_INFO,"CL_MS_INIT_RCV");
         connection->data_read_buffer_pos = 0;
         connection->data_read_buffer_processed = 0;
         connection->read_gmsh_header->dl = 0;
         gettimeofday(&now,NULL);
         connection->read_buffer_timeout_time = now.tv_sec + connection->handler->read_timeout;
         message->message_rcv_pointer = 0;
         message->message_state = CL_MS_RCV_GMSH;
      }

      if (message->message_state == CL_MS_RCV_GMSH) {
         CL_LOG(CL_LOG_INFO,"CL_MS_RCV_GMSH");
         size = 0;
         return_value = cl_com_read_GMSH(connection, &size);
         if (return_value != CL_RETVAL_OK ) {
             /* header is not complete, try later */
             cl_raw_list_unlock(connection->received_message_list);
             CL_LOG_STR(CL_LOG_INFO,"cl_com_read_GMSH returned", cl_get_error_text(return_value));
             return return_value;
         }
         connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + connection->data_read_buffer_pos;
         connection->data_read_buffer_pos = 0;
         connection->data_read_buffer_processed = 0;
         message->message_state = CL_MS_RCV_MIH;
      }

      if (message->message_state == CL_MS_RCV_MIH) {
         cl_com_MIH_t* mih_message = NULL;
         CL_LOG(CL_LOG_INFO,"CL_MS_RCV_MIH");
         size = connection->read_gmsh_header->dl - (connection->data_read_buffer_pos - connection->data_read_buffer_processed);
         if ( (size + connection->data_read_buffer_pos) >= connection->data_buffer_size ) {
            CL_LOG(CL_LOG_ERROR,"stream buffer to small");
            return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
         }
         if (size > 0) {
            unsigned long data_read = 0;
            
            return_value = cl_com_receive_message(connection,
                                                  connection->read_buffer_timeout_time,
                                                  &(connection->data_read_buffer[(connection->data_read_buffer_pos)]),
                                                  size, 
                                                  &data_read);  /* returns the data bytes read */
            connection->data_read_buffer_pos = connection->data_read_buffer_pos + data_read;
            if (return_value != CL_RETVAL_OK) {
               cl_raw_list_unlock(connection->received_message_list);
               CL_LOG_STR(CL_LOG_INFO,"cl_com_receive_message returned", cl_get_error_text(return_value));
               return return_value;
            }
         }
         connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + connection->data_read_buffer_pos;
         return_value = cl_xml_parse_MIH(&(connection->data_read_buffer[(connection->data_read_buffer_processed)]),connection->read_gmsh_header->dl, &mih_message);

         if (return_value != CL_RETVAL_OK ) {
             cl_raw_list_unlock(connection->received_message_list);
             CL_LOG_STR(CL_LOG_INFO,"cl_xml_parse_MIH returned", cl_get_error_text(return_value));
             return return_value;
         }

         /* TODO: check version */
         message->message_id          = mih_message->mid;
         message->message_length      = mih_message->dl;
         message->message_df          = mih_message->df;
         message->message_mat         = mih_message->mat;
         message->message_tag         = mih_message->tag;
         message->message_response_id = mih_message->rid;
         cl_com_free_mih_message(&mih_message);
         message->message = (cl_byte_t*) malloc( sizeof(cl_byte_t) * message->message_length );
         if (message->message == NULL) {
            cl_raw_list_unlock(connection->received_message_list);
            return CL_RETVAL_MALLOC;
         }
         message->message_state = CL_MS_RCV;
      }
      
      if (message->message_state == CL_MS_RCV) {
         CL_LOG(CL_LOG_INFO,"CL_MS_RCV");

         size = 0;
         /* is message allready complete received ? */
         if (message->message_rcv_pointer < message->message_length) {
            return_value = cl_com_receive_message(connection, 
                                                  connection->read_buffer_timeout_time, 
                                                  &(message->message[message->message_rcv_pointer]), 
                                                  message->message_length - message->message_rcv_pointer, 
                                                  &size );
            message->message_rcv_pointer = message->message_rcv_pointer + size;
            if (return_value != CL_RETVAL_OK) {
               cl_raw_list_unlock(connection->received_message_list);
               CL_LOG_STR(CL_LOG_INFO,"cl_com_receive_message returned", cl_get_error_text(return_value));
               return return_value;
            }
         }
         CL_LOG(CL_LOG_INFO,"<-<-<-<-  received message <-<-<-<-");
         CL_LOG_STR(CL_LOG_INFO,"received message from host:", connection->receiver->comp_host);
         CL_LOG_STR(CL_LOG_INFO,"which has name:            ", connection->receiver->comp_name);
         CL_LOG_INT(CL_LOG_INFO,"and id:                    ", connection->receiver->comp_id);

         gettimeofday(&message->message_receive_time,NULL);
         /* set last transfer time of connection */
         memcpy(&connection->last_transfer_time,&message->message_receive_time,sizeof (struct timeval));

 
         connection->read_buffer_timeout_time = 0;
         CL_LOG_INT(CL_LOG_INFO,"message size:         ", message->message_length);
         CL_LOG_STR(CL_LOG_INFO,"received message from:", connection->receiver->comp_host);

         connection->statistic->bytes_received = connection->statistic->bytes_received + message->message_length;
         connection->statistic->real_bytes_received = connection->statistic->real_bytes_received + message->message_length;

         connection->last_received_message_id = message->message_id;

         if (connection->connection_state == CL_COM_CONNECTED) {
            int return_value = CL_RETVAL_UNKNOWN;      
            char* autoclose = "disabled";
            if ( connection->auto_close_type == CL_CM_AC_ENABLED) {
               autoclose = "enabled";
            }
#if 0
            printf("---> touching endpoint %s/%s/%ld autoclose is %s\n\n", connection->remote->comp_host, connection->remote->comp_name, connection->remote->comp_id,  autoclose );
#endif
            /* Touch endpoint, he is still active */
            if (connection->framework_type == CL_CT_TCP ) {
               cl_com_tcp_private_t* tcp_private = NULL;
               tcp_private = (cl_com_tcp_private_t*) connection->com_private;
               if (tcp_private != NULL) {
                  return_value = cl_endpoint_list_define_endpoint(cl_com_get_endpoint_list(),connection->remote, tcp_private->connect_port, connection->auto_close_type, 0 );
               }
            }
         }

         switch(message->message_df) {
            case CL_MIH_DF_BIN:
               CL_LOG(CL_LOG_INFO,"received binary message");
               message->message_state = CL_MS_READY;
               if (connection->handler != NULL) { 
                  cl_com_handle_t* handle = connection->handler;
                  /* increase counter for ready messages */
                  pthread_mutex_lock(handle->messages_ready_mutex);
                  handle->messages_ready_for_read = handle->messages_ready_for_read + 1;
                  pthread_mutex_unlock(handle->messages_ready_mutex);
               }
               break;
            case CL_MIH_DF_XML:
               CL_LOG(CL_LOG_INFO,"received XML message");
               message->message_state = CL_MS_READY;
               if (connection->handler != NULL) { 
                  cl_com_handle_t* handle = connection->handler;
                  /* increase counter for ready messages */
                  pthread_mutex_lock(handle->messages_ready_mutex);
                  handle->messages_ready_for_read = handle->messages_ready_for_read + 1;
                  pthread_mutex_unlock(handle->messages_ready_mutex);
               }
               break;
            default:
               CL_LOG(CL_LOG_INFO,"received protocol message");
               message->message_state = CL_MS_PROTOCOL;
               break;
         }
      }

      if (message->message_state == CL_MS_PROTOCOL) {
         cl_com_AM_t*            ack_message       = NULL;
         cl_com_SIM_t*           sim_message       = NULL;
         cl_com_SIRM_t*          sirm_message      = NULL;
         cl_com_CCM_t*           ccm_message       = NULL;
         cl_com_CCRM_t*          ccrm_message      = NULL;
         cl_com_message_t*       sent_message      = NULL;
         cl_message_list_elem_t* message_list_elem = NULL;
         unsigned long           run_time = 0;
         struct timeval          now;



         switch(message->message_df) {
            case CL_MIH_DF_SIM:
               CL_LOG(CL_LOG_INFO,"received SIM message");
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               cl_raw_list_unlock(connection->received_message_list);
               if (return_value != CL_RETVAL_OK) {
                  return return_value;
               }
               return_value = cl_xml_parse_SIM((unsigned char*)message->message, message->message_length, &sim_message);
               if (return_value != CL_RETVAL_OK) {
                  cl_com_free_message(&message);
                  return return_value;
               }
               if (connection->handler != NULL) { 
                  cl_com_handle_t* handle = connection->handler;
                  
                  cl_commlib_calculate_statistic(handle,0);


                  gettimeofday(&now,NULL);
                  run_time = (unsigned long)now.tv_sec - handle->start_time.tv_sec;


                  cl_commlib_send_sirm_message(                connection,
                                                               message,
                                               (unsigned long) handle->start_time.tv_sec,
                                                               run_time,
                                                               handle->statistic->unread_message_count,
                                                               handle->statistic->unsend_message_count,
                                                               handle->statistic->nr_of_connections, 
                                                               handle->statistic->application_status,
                                                               "ok");
               } else {
                  cl_commlib_send_sirm_message(connection, message, 0, 0 ,0 ,0 , 0, 0, "get status info error");
               }
               cl_com_free_message(&message);
               cl_com_free_sim_message(&sim_message);
               return CL_RETVAL_OK;
            case CL_MIH_DF_SIRM:
               CL_LOG(CL_LOG_INFO,"received SIRM message");
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               cl_raw_list_unlock(connection->received_message_list);
               if (return_value != CL_RETVAL_OK) {
                  return return_value;
               }

               return_value = cl_xml_parse_SIRM((unsigned char*)message->message, message->message_length, &sirm_message);
               if (return_value != CL_RETVAL_OK) {
                  cl_com_free_message(&message);
                  return return_value;
               }
               CL_LOG_INT(CL_LOG_INFO,"received SIRM message for sent SIM message with id:", sirm_message->mid);

               /* set sirm flag for sent SIM message with ack id from sirm */
               cl_raw_list_lock(connection->send_message_list);
               message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
               sent_message = NULL;
               while (message_list_elem != NULL) {
                  sent_message = message_list_elem->message;
                  if (sent_message->message_state == CL_MS_PROTOCOL) {
                     if (sent_message->message_id == sirm_message->mid) {
                        break;
                     }
                  } 
                  message_list_elem = cl_message_list_get_next_elem(connection->send_message_list,message_list_elem );
               }
               if (sent_message == NULL) {
                  CL_LOG_INT(CL_LOG_ERROR,"got SIRM message for unexpected message id",sirm_message->mid);
                  cl_com_free_sirm_message(&sirm_message);
               } else {
                  CL_LOG_INT(CL_LOG_INFO,"got SIRM message for SIM message id",sirm_message->mid);
                  sent_message->message_sirm = sirm_message;
               }
               cl_raw_list_unlock(connection->send_message_list);

               cl_com_free_message(&message);
               return CL_RETVAL_OK;

            case CL_MIH_DF_AM:
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               cl_raw_list_unlock(connection->received_message_list);
               if (return_value != CL_RETVAL_OK) {
                  return return_value;
               }
               return_value = cl_xml_parse_AM((unsigned char*)message->message, message->message_length, &ack_message);
               if (return_value != CL_RETVAL_OK) {
                  cl_com_free_message(&message);
                  return return_value;
               }
               CL_LOG_INT(CL_LOG_INFO,"received ACK message. Id of ACK message:", message->message_id);

               /* set ack flag for sent message with ack id from send_message_list */
               cl_raw_list_lock(connection->send_message_list);
               message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
               sent_message = NULL;
               while (message_list_elem != NULL) {
                  sent_message = message_list_elem->message;
                  if (sent_message->message_state == CL_MS_PROTOCOL) {
                     if (sent_message->message_id == ack_message->mid) {
                        break;
                     }
                  } 
                  message_list_elem = cl_message_list_get_next_elem(connection->send_message_list,message_list_elem );
               }
               if (sent_message == NULL) {
                  CL_LOG_INT(CL_LOG_ERROR,"got ACK message for unexpected message id",ack_message->mid);
               } else {
                  CL_LOG_INT(CL_LOG_INFO,"got ACK message for message id",ack_message->mid);
                  sent_message->message_ack_flag = 1;
               }
               cl_raw_list_unlock(connection->send_message_list);


               cl_com_free_message(&message);
               cl_com_free_am_message(&ack_message);
               return CL_RETVAL_OK;

            case CL_MIH_DF_CCM:
               CL_LOG(CL_LOG_WARNING,"received a CCM");
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               cl_raw_list_unlock(connection->received_message_list);
               if (return_value != CL_RETVAL_OK) {
                  return return_value;
               }
               return_value = cl_xml_parse_CCM((unsigned char*)message->message, message->message_length, &ccm_message);
               if (return_value != CL_RETVAL_OK) {
                  cl_com_free_message(&message);
                  return return_value;
               }
               if (connection->connection_state  == CL_COM_CONNECTED) {
                  if (connection->connection_sub_state == CL_COM_WORK) {
                     CL_LOG(CL_LOG_INFO,"received connection close message");
                     connection->ccm_received = 1;
                  } else {
                     if ( connection->was_accepted == 1) {
                        CL_LOG(CL_LOG_ERROR,"received connection close message from client while sending ccm message - ignoring");
                     } else {
                        CL_LOG(CL_LOG_INFO,"received connection close message");
                        connection->ccm_received = 1;
                     }
                  }
               }
               cl_com_free_message(&message);
               cl_com_free_ccm_message(&ccm_message);
               return CL_RETVAL_OK;

            case CL_MIH_DF_CCRM:
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               cl_raw_list_unlock(connection->received_message_list);
               if (return_value != CL_RETVAL_OK) {
                  return return_value;
               }
               return_value = cl_xml_parse_CCRM((unsigned char*)message->message, message->message_length, &ccrm_message);
               if (return_value != CL_RETVAL_OK) {
                  cl_com_free_message(&message);
                  return return_value;
               }
               if (connection->connection_state == CL_COM_CONNECTED) {
                  CL_LOG(CL_LOG_WARNING,"received connection close response message");
                  connection->ccrm_received = 1;
                  connection->connection_sub_state = CL_COM_DONE;  /* CLOSE message */
                  connection->data_write_flag = CL_COM_DATA_NOT_READY;
               } 

               cl_com_free_message(&message);
               cl_com_free_ccrm_message(&ccrm_message);
               return CL_RETVAL_OK;

            default:
               CL_LOG(CL_LOG_ERROR,"reseived unsupported protocol message");
               return_value = cl_message_list_remove_message(connection->received_message_list, message,0);
               if (return_value == CL_RETVAL_OK) {
                  cl_com_free_message(&message);
               }
               break;
         }
      }

      cl_raw_list_unlock(connection->received_message_list);
      if (connection->ccm_received == 1) {
          CL_LOG(CL_LOG_INFO,"received ccm");
   
          CL_LOG_INT(CL_LOG_WARNING,"receive buffer:",cl_raw_list_get_elem_count(connection->received_message_list) );
          CL_LOG_INT(CL_LOG_WARNING,"send buffer   :",cl_raw_list_get_elem_count(connection->send_message_list) );
          CL_LOG_INT(CL_LOG_WARNING,"ccm_received  :",connection->ccm_received);

          if( cl_raw_list_get_elem_count(connection->send_message_list) == 0 && 
              cl_raw_list_get_elem_count(connection->received_message_list) == 0 ) {
             connection->ccm_received = 2;
             connection->connection_sub_state = CL_COM_SENDING_CCRM;
             /* disable #if 1 if you want to test a client, that is frozen! */
             cl_commlib_send_ccrm_message(connection);
             CL_LOG(CL_LOG_WARNING,"sending ccrm");
          } else {
             CL_LOG(CL_LOG_WARNING,"can't send ccrm, still messages in buffer");
          }
       }
   }
   return return_value;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_handle_connection_ack_timeouts()"

/* This function removes all messages in sent_message_list in protocol state where
   the ack timeout is reached (e.g. ACK messages and SIM messages)*/
static int cl_commlib_handle_connection_ack_timeouts(cl_com_connection_t* connection) {
   cl_com_message_t* message = NULL;
   cl_message_list_elem_t* message_list_elem = NULL;
   cl_message_list_elem_t* next_message_list_elem = NULL;

   int return_value = CL_RETVAL_OK;
   struct timeval now;
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }


   /* stream connections have no ack messages */
   if (connection->data_flow_type == CL_CM_CT_STREAM) {
      return CL_RETVAL_OK;
   }

   if (connection->data_flow_type == CL_CM_CT_MESSAGE) {
      long timeout_time = 0;
       CL_LOG(CL_LOG_INFO,"checking timeouts for ack messages");
      /* lock received messages list */
      cl_raw_list_lock(connection->send_message_list);

      /* get current timeout time */
      gettimeofday(&now,NULL);

      message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
      while(message_list_elem != NULL) {
         message = message_list_elem->message;
         next_message_list_elem = cl_message_list_get_next_elem(connection->send_message_list, message_list_elem );
         if (message->message_state == CL_MS_PROTOCOL) {
            timeout_time = (message->message_send_time).tv_sec + connection->handler->acknowledge_timeout;
            if ( timeout_time <= now.tv_sec ) {
               CL_LOG_INT(CL_LOG_ERROR,"ack timeout for message", message->message_id);
               cl_message_list_remove_message(connection->send_message_list, message,0 );
               cl_com_free_message(&message);
            } else {
               if ( cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                  if ( connection->connection_state == CL_COM_CONNECTED &&
                       connection->connection_sub_state == CL_COM_WORK ) {
                     CL_LOG(CL_LOG_INFO,"ignore ack timeout flag is set, but this connection is connected and waiting for ack - continue waiting");
                  } else {
                     CL_LOG(CL_LOG_INFO,"ignore ack timeout flag is set and connection is not connected - ignore timeout");
                     cl_message_list_remove_message(connection->send_message_list, message,0 );
                     cl_com_free_message(&message);
                  }
               } 
            }
         }
         message_list_elem = next_message_list_elem;
      }
      /* unlock received messages list */
      cl_raw_list_unlock(connection->send_message_list);
   }
   return return_value;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_check_connection_count()"
static int cl_commlib_check_connection_count(cl_com_handle_t* handle) {
   cl_connection_list_elem_t* elem = NULL;

   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }

    /* check max open connections */
   if (handle->max_con_close_mode != CL_ON_MAX_COUNT_OFF) {

      /* first lock the connection list */
      cl_raw_list_lock(handle->connection_list);

      /* check if we exceed the max. connection count */
      if ( cl_raw_list_get_elem_count(handle->connection_list) >= handle->max_open_connections ) {

         /* we have reached max connection count, set flag */
         if ( handle->max_connection_count_reached == 0) {
            handle->max_connection_count_reached = 1;
            CL_LOG(CL_LOG_ERROR,"max open connection count reached");
         }

         /* still haven't found a connection to close */
         if (  handle->max_connection_count_found_connection_to_close == 0 ) {
            cl_com_connection_t* oldest_connection = NULL;

            if ( handle->max_con_close_mode == CL_ON_MAX_COUNT_CLOSE_AUTOCLOSE_CLIENTS ) {
               /* try to find the oldest connected connection of type CL_CM_CT_MESSAGE */
               elem = cl_connection_list_get_first_elem(handle->connection_list);
               while(elem) {
                  if (elem->connection->data_flow_type       == CL_CM_CT_MESSAGE                       &&
                      elem->connection->connection_state     == CL_COM_CONNECTED                       &&    
                      elem->connection->connection_sub_state == CL_COM_WORK                            &&
                      elem->connection->auto_close_type      == CL_CM_AC_ENABLED                       && 
                      elem->connection                       != handle->last_receive_message_connection   ) {
                  
                     /* we haven't selected an elem till now, take the first one */
                     if (oldest_connection == NULL) {
                        oldest_connection = elem->connection; 
                     } else {
                        /* check if the actual elem is older than oldest_connection */
                        if (elem->connection->last_transfer_time.tv_sec < oldest_connection->last_transfer_time.tv_sec) {
                           oldest_connection = elem->connection;
                        } else {
                           if (elem->connection->last_transfer_time.tv_sec == oldest_connection->last_transfer_time.tv_sec) {
                              if (elem->connection->last_transfer_time.tv_usec < oldest_connection->last_transfer_time.tv_usec) {
                                 oldest_connection = elem->connection;
                              }
                           }
                        }
                     }
                  }
                  elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
               }
            }

            /* close connection ( if found ) */
            if ( oldest_connection != NULL ) {
               cl_commlib_send_ccm_message(oldest_connection);
               oldest_connection->connection_sub_state = CL_COM_SENDING_CCM;
               handle->max_connection_count_found_connection_to_close = 1;
               CL_LOG_STR(CL_LOG_WARNING,"closing connection to host:", oldest_connection->remote->comp_host );
               CL_LOG_STR(CL_LOG_WARNING,"component name:            ", oldest_connection->remote->comp_name );
               CL_LOG_INT(CL_LOG_WARNING,"component id:              ", oldest_connection->remote->comp_id );
            } else {
               CL_LOG(CL_LOG_WARNING,"can't close any connection");
               handle->max_connection_count_found_connection_to_close = 0;
            }
         }

         /* we have found a connection to close, check if closing is still in progress */
         if ( handle->max_connection_count_found_connection_to_close != 0 ) {
            int is_still_in_list = 0;
            elem = cl_connection_list_get_first_elem(handle->connection_list);
            while(elem) {
               if (elem->connection->data_flow_type       == CL_CM_CT_MESSAGE   &&
                   elem->connection->connection_state     == CL_COM_CONNECTED   &&
                   elem->connection->connection_sub_state != CL_COM_WORK       ) {
                  CL_LOG_STR(CL_LOG_WARNING,"processing close of connection to host:", elem->connection->remote->comp_host );
                  CL_LOG_STR(CL_LOG_WARNING,"component name:            ", elem->connection->remote->comp_name );
                  CL_LOG_INT(CL_LOG_WARNING,"component id:              ", elem->connection->remote->comp_id );
                  is_still_in_list = 1;
                  break;
               }
               elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
            }
            if ( is_still_in_list == 0) {
               handle->max_connection_count_found_connection_to_close = 0;
            } else {
               CL_LOG(CL_LOG_WARNING,"still waiting for closing of connection");
            }
         }
      } else {
         /* we have enough free connections, check if connection count reached flag is set */
         if (handle->max_connection_count_reached != 0) {
            handle->max_connection_count_reached = 0;
            handle->max_connection_count_found_connection_to_close = 0;
            CL_LOG(CL_LOG_ERROR,"new connections enabled again");
         }
      }
      cl_raw_list_unlock(handle->connection_list);
   }
   return CL_RETVAL_OK;
} 

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_calculate_statistic()"
static int cl_commlib_calculate_statistic(cl_com_handle_t* handle, int lock_list) {
   cl_connection_list_elem_t* elem = NULL;
   struct timeval now;
   double handle_time_last = 0.0;
   double handle_time_now = 0.0;
   double handle_time_range = 0.0;
   double con_per_second = 0.0;
   double kbits_sent = 0.0;
   double kbits_received = 0.0;
   double real_kbits_sent = 0.0;
   double real_kbits_received = 0.0;
   double send_pay_load = 0.0;
   double receive_pay_load = 0.0;
   char help[256];

   cl_com_con_statistic_t* con_stat = NULL;
   if (handle == NULL) {
      CL_LOG(CL_LOG_ERROR,"no handle specified");
      return CL_RETVAL_PARAMS;
   }

   if (lock_list != 0) {
      cl_raw_list_lock(handle->connection_list);
   }
   gettimeofday(&now,NULL);
   /* get application status */
   pthread_mutex_lock(&cl_com_application_mutex);
   handle->statistic->application_status = 0;
   if (cl_com_application_status_func != NULL) {
      handle->statistic->application_status = cl_com_application_status_func();
   } 
   pthread_mutex_unlock(&cl_com_application_mutex);

   handle_time_now = now.tv_sec + (now.tv_usec / 1000000.0);
   handle_time_last = handle->statistic->last_update.tv_sec + (handle->statistic->last_update.tv_usec / 1000000.0 );
   handle_time_range = handle_time_now - handle_time_last;
   gettimeofday(&(handle->statistic->last_update),NULL);

   con_per_second = handle->statistic->new_connections / handle_time_range;
   
   handle->statistic->new_connections = 0;
   handle->statistic->unsend_message_count = 0;
   handle->statistic->unread_message_count = 0;
   handle->statistic->nr_of_connections = cl_raw_list_get_elem_count(handle->connection_list);
   elem = cl_connection_list_get_first_elem(handle->connection_list);
   while(elem) {
      /* elem->connection */
      con_stat = elem->connection->statistic;
      if (con_stat) {
         handle->statistic->bytes_sent = handle->statistic->bytes_sent + con_stat->bytes_sent;
         handle->statistic->real_bytes_sent = handle->statistic->real_bytes_sent + con_stat->real_bytes_sent;
         handle->statistic->bytes_received = handle->statistic->bytes_received + con_stat->bytes_received;
         handle->statistic->real_bytes_received = handle->statistic->real_bytes_received + con_stat->real_bytes_received;
         con_stat->bytes_sent = 0;
         con_stat->bytes_received = 0;
         con_stat->real_bytes_sent = 0;
         con_stat->real_bytes_received = 0;

         handle->statistic->unsend_message_count = handle->statistic->unsend_message_count + cl_raw_list_get_elem_count(elem->connection->send_message_list);
         handle->statistic->unread_message_count = handle->statistic->unread_message_count + cl_raw_list_get_elem_count(elem->connection->received_message_list);
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
   }
   if (handle_time_range > 0.0) {
      kbits_sent          = ( (handle->statistic->bytes_sent          / 1024.0) * 8.0) / handle_time_range;
      kbits_received      = ( (handle->statistic->bytes_received      / 1024.0) * 8.0) / handle_time_range;
      real_kbits_sent     = ( (handle->statistic->real_bytes_sent     / 1024.0) * 8.0) / handle_time_range;
      real_kbits_received = ( (handle->statistic->real_bytes_received / 1024.0) * 8.0) / handle_time_range;
   }

   if (real_kbits_sent > 0.0) {
      send_pay_load = kbits_sent / real_kbits_sent;
   }
   if (real_kbits_received > 0.0) {
      receive_pay_load = kbits_received / real_kbits_received;
   }
   

   snprintf(help,255,"           %.3f", handle_time_range);
   CL_LOG_STR(CL_LOG_INFO,"time_range:",help);

   snprintf(help,255,"  %.3f", con_per_second );
   CL_LOG_STR(CL_LOG_INFO,"new connections/sec:",help);

   snprintf(help,255,"           %.3f", send_pay_load);
   CL_LOG_STR(CL_LOG_INFO,"sent ratio:",help);  
   snprintf(help,255,"          %.3f", kbits_sent);    
   CL_LOG_STR(CL_LOG_INFO,"sent kbit/s:",help);  
   snprintf(help,255,"     %.3f", real_kbits_sent);    
   CL_LOG_STR(CL_LOG_INFO,"real sent kbit/s:",help);


   snprintf(help,255,"        %.3f", receive_pay_load);
   CL_LOG_STR(CL_LOG_INFO,"receive ratio:",help);  
   snprintf(help,255,"      %.3f", kbits_received);   
   CL_LOG_STR(CL_LOG_INFO,"received kbit/s:",help);
   snprintf(help,255," %.3f", real_kbits_received);   
   CL_LOG_STR(CL_LOG_INFO,"real received kbit/s:",help);


   snprintf(help,255,"           %.3f" , (double) handle->statistic->bytes_sent / 1024.0);  
   CL_LOG_STR(CL_LOG_INFO,"sent kbyte:",help);   
   snprintf(help,255,"      %.3f" , (double) handle->statistic->real_bytes_sent / 1024.0);  
   CL_LOG_STR(CL_LOG_INFO,"real sent kbyte:",help);   
 

 
   snprintf(help,255,"       %.3f" , (double)handle->statistic->bytes_received / 1024.0);   
   CL_LOG_STR(CL_LOG_INFO,"received kbyte:",help);
   snprintf(help,255,"  %.3f" , (double)handle->statistic->real_bytes_received / 1024.0);   
   CL_LOG_STR(CL_LOG_INFO,"real received kbyte:",help);



   snprintf(help,255," %ld" , handle->statistic->unsend_message_count);
   CL_LOG_STR(CL_LOG_INFO,"unsend_message_count:",help);

   snprintf(help,255," %ld" , handle->statistic->unread_message_count);
   CL_LOG_STR(CL_LOG_INFO,"unread_message_count:",help);

   snprintf(help,255,"     %ld" , handle->statistic->nr_of_connections);
   CL_LOG_STR(CL_LOG_INFO,"open connections:",help);

   snprintf(help,255,"    %ld" , handle->statistic->application_status);
   CL_LOG_STR(CL_LOG_INFO,"application state:",help);


   handle->statistic->bytes_sent = 0;
   handle->statistic->bytes_received = 0;
   handle->statistic->real_bytes_sent = 0;
   handle->statistic->real_bytes_received = 0;
   
   if (lock_list != 0) {
      cl_raw_list_unlock(handle->connection_list);
   }
   return CL_RETVAL_OK;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_finish_request_completeness()"
static int cl_commlib_finish_request_completeness(cl_com_connection_t* connection) {
   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }


   if ( connection->framework_type == CL_CT_TCP ) {
      cl_com_tcp_private_t* tcp_private = NULL;

      if (connection->was_accepted == 1) {
         tcp_private = (cl_com_tcp_private_t*) connection->com_private;
         if ( tcp_private != NULL ) {
            if ( tcp_private->connect_port > 0 ) {
               CL_LOG_STR(CL_LOG_INFO,"comp_host :", connection->remote->comp_host);
               CL_LOG_STR(CL_LOG_INFO,"comp_name :", connection->remote->comp_name);
               CL_LOG_INT(CL_LOG_INFO,"comp_id   :", connection->remote->comp_id);
               CL_LOG_INT(CL_LOG_INFO,"new connected client can be reached at port", tcp_private->connect_port);
               if ( connection->auto_close_type == CL_CM_AC_ENABLED) {
                  CL_LOG(CL_LOG_INFO,"new connected client supports auto close");
               }

               cl_com_append_known_endpoint(connection->remote, tcp_private->connect_port, connection->auto_close_type, 0);
            } else {
               CL_LOG(CL_LOG_INFO,"client is not reachable via service port");
            }
         }
      }

      return CL_RETVAL_OK;
   }
   return CL_RETVAL_UNKNOWN;
}



#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_handle_connection_write()"
static int cl_commlib_handle_connection_write(cl_com_connection_t* connection) {
   cl_com_message_t* message = NULL;
   cl_message_list_elem_t* message_list_elem = NULL;
   int return_value = CL_RETVAL_OK;
   struct timeval now;

   if (connection == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (connection->data_flow_type == CL_CM_CT_STREAM) {

       cl_raw_list_lock(connection->send_message_list);

       message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
       if (message_list_elem != NULL) {
          message = message_list_elem->message;
       } else {
          connection->data_write_flag = CL_COM_DATA_NOT_READY;
          cl_raw_list_unlock(connection->send_message_list);
          return CL_RETVAL_OK;
       }

       if (message->message_state == CL_MS_INIT_SND) {
          message->message_snd_pointer = 0;    
          message->message_state = CL_MS_SND;
          gettimeofday(&now,NULL);
          connection->write_buffer_timeout_time = now.tv_sec + connection->handler->write_timeout;
       }

       if (message->message_state == CL_MS_SND) {
          unsigned long written = 0;
          return_value = cl_com_send_message(connection, 
                                             connection->write_buffer_timeout_time , 
                                             &(message->message[message->message_snd_pointer]) , 
                                             message->message_length - message->message_snd_pointer, 
                                             &written );
          message->message_snd_pointer = message->message_snd_pointer + written;
          if (return_value != CL_RETVAL_OK) {
             cl_raw_list_unlock(connection->send_message_list);
             return return_value;
          }
          if (message->message_snd_pointer == message->message_length) {
             message->message_state = CL_MS_READY;
          } else {
             cl_raw_list_unlock(connection->send_message_list);
             return CL_RETVAL_SEND_ERROR;
          }
       }

       if (message->message_state == CL_MS_READY) {
          connection->write_buffer_timeout_time = 0;
          connection->statistic->bytes_sent = connection->statistic->bytes_sent + message->message_length;
          connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + message->message_length;
          if ( cl_message_list_remove_message(connection->send_message_list, message,0 ) == CL_RETVAL_OK) {
             cl_com_free_message(&message);
          }

          if (cl_message_list_get_first_elem(connection->send_message_list) == NULL) {
             connection->data_write_flag = CL_COM_DATA_NOT_READY;
          }

          cl_raw_list_unlock(connection->send_message_list);

          /* set last transfer time of connection */
          gettimeofday(&connection->last_transfer_time,NULL);

          /* Touch endpoint, he is still active */
          if (connection->framework_type == CL_CT_TCP ) {
             cl_com_tcp_private_t* tcp_private = NULL;
             tcp_private = (cl_com_tcp_private_t*) connection->com_private;
             if (tcp_private != NULL) {
                cl_endpoint_list_define_endpoint(cl_com_get_endpoint_list(),connection->remote, tcp_private->connect_port, connection->auto_close_type, 0 );
             }
          }
       }
    }
  
    if (connection->data_flow_type == CL_CM_CT_MESSAGE) {

       cl_raw_list_lock(connection->send_message_list);
       message = NULL;
       CL_LOG_INT(CL_LOG_INFO,"number of messages in send list:",cl_raw_list_get_elem_count(connection->send_message_list));
       message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
       while (message_list_elem != NULL) {
          message = message_list_elem->message;
          if (message->message_state != CL_MS_PROTOCOL && message->message_state != CL_MS_READY) {
             break;
          } 
          message = NULL;
          message_list_elem = cl_message_list_get_next_elem(connection->send_message_list,message_list_elem );
       } 

       if (message == NULL) {
          connection->data_write_flag = CL_COM_DATA_NOT_READY;
          cl_raw_list_unlock(connection->send_message_list);
          return CL_RETVAL_OK;
       }

       if (message->message_state == CL_MS_INIT_SND) {
          unsigned long gmsh_message_size = 0;
          unsigned long mih_message_size = 0;

          mih_message_size = CL_MIH_MESSAGE_SIZE;
          mih_message_size = mih_message_size + strlen(CL_MIH_MESSAGE_VERSION);
          mih_message_size = mih_message_size + cl_util_get_ulong_number_length(message->message_id);
          mih_message_size = mih_message_size + cl_util_get_ulong_number_length(message->message_length);
          mih_message_size = mih_message_size + strlen(cl_com_get_mih_df_string(message->message_df));
          mih_message_size = mih_message_size + strlen(cl_com_get_mih_mat_string(message->message_mat));
          mih_message_size = mih_message_size + cl_util_get_ulong_number_length(message->message_tag);
          mih_message_size = mih_message_size + cl_util_get_ulong_number_length(message->message_response_id);


          if (connection->data_buffer_size < (mih_message_size + 1) ) {
             return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
          }

          gmsh_message_size = CL_GMSH_MESSAGE_SIZE + cl_util_get_ulong_number_length(mih_message_size);
          if (connection->data_buffer_size < (gmsh_message_size + 1) ) {
             return CL_RETVAL_STREAM_BUFFER_OVERFLOW;
          }
          sprintf((char*)connection->data_write_buffer, CL_GMSH_MESSAGE , mih_message_size);


          gettimeofday(&now,NULL);
          connection->write_buffer_timeout_time = now.tv_sec + connection->handler->write_timeout;
          connection->data_write_buffer_pos = 0;
          connection->data_write_buffer_processed = 0;
          connection->data_write_buffer_to_send = gmsh_message_size;
          message->message_snd_pointer = 0;    
          message->message_state = CL_MS_SND_GMSH;
       }

       if (message->message_state == CL_MS_SND_GMSH) {
          unsigned long written = 0;

          return_value = cl_com_send_message(connection, 
                                             connection->write_buffer_timeout_time , 
                                             &(connection->data_write_buffer[connection->data_write_buffer_pos]),
                                             connection->data_write_buffer_to_send,
                                             &written );
          connection->data_write_buffer_pos = connection->data_write_buffer_pos + written;
          connection->data_write_buffer_to_send = connection->data_write_buffer_to_send - written;
          if (return_value != CL_RETVAL_OK) {
             cl_raw_list_unlock(connection->send_message_list);
             return return_value;
          }
          if (connection->data_write_buffer_to_send > 0) {
             cl_raw_list_unlock(connection->send_message_list);
             return CL_RETVAL_SEND_ERROR;
          }
          connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + connection->data_write_buffer_pos ;

          sprintf((char*)connection->data_write_buffer, CL_MIH_MESSAGE ,
                   CL_MIH_MESSAGE_VERSION,
                   message->message_id,
                   message->message_length,
                   cl_com_get_mih_df_string(message->message_df),
                   cl_com_get_mih_mat_string(message->message_mat),
                   message->message_tag ,
                   message->message_response_id);

          
          CL_LOG_STR(CL_LOG_DEBUG,"write buffer:",(char*)connection->data_write_buffer);
          CL_LOG_STR(CL_LOG_INFO,"CL_MS_SND_GMSH, MIH: ",(char*)connection->data_write_buffer );     

          connection->data_write_buffer_pos = 0;
          connection->data_write_buffer_processed = 0;
          connection->data_write_buffer_to_send = strlen((char*)connection->data_write_buffer);

          message->message_snd_pointer = 0;    
          message->message_state = CL_MS_SND_MIH;
       }

       if (message->message_state == CL_MS_SND_MIH) {
          unsigned long written = 0;
          return_value = cl_com_send_message(connection, 
                                             connection->write_buffer_timeout_time , 
                                             &(connection->data_write_buffer[connection->data_write_buffer_pos]),
                                             connection->data_write_buffer_to_send,
                                             &written );
          connection->data_write_buffer_pos = connection->data_write_buffer_pos + written;
          connection->data_write_buffer_to_send = connection->data_write_buffer_to_send - written;
          if (return_value != CL_RETVAL_OK) {
             cl_raw_list_unlock(connection->send_message_list);
             return return_value;
          }
          if (connection->data_write_buffer_to_send > 0) {
             cl_raw_list_unlock(connection->send_message_list);
             return CL_RETVAL_SEND_ERROR;
          }
          connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + connection->data_write_buffer_pos ;

          message->message_state = CL_MS_SND;
       }
       
       if (message->message_state == CL_MS_SND) {
          unsigned long written = 0;
          return_value = cl_com_send_message(connection, 
                                             connection->write_buffer_timeout_time , 
                                             &(message->message[message->message_snd_pointer]) , 
                                             message->message_length - message->message_snd_pointer, 
                                             &written );
          message->message_snd_pointer = message->message_snd_pointer + written;
          if (return_value != CL_RETVAL_OK) {
             cl_raw_list_unlock(connection->send_message_list);
             return return_value;
          }
          if (message->message_snd_pointer == message->message_length) {
             message->message_state = CL_MS_READY;
          } else {
             cl_raw_list_unlock(connection->send_message_list);
             return CL_RETVAL_SEND_ERROR;
          }
       }

       if (message->message_state == CL_MS_READY) {
          CL_LOG(CL_LOG_INFO,"->->->->->  sent message ->->->->->");
          CL_LOG_STR(CL_LOG_INFO,"message sent to host:      ", connection->receiver->comp_host);
          CL_LOG_STR(CL_LOG_INFO,"which has name:            ", connection->receiver->comp_name);
          CL_LOG_INT(CL_LOG_INFO,"and id:                    ", connection->receiver->comp_id);

          connection->statistic->bytes_sent = connection->statistic->bytes_sent + message->message_length;
          connection->statistic->real_bytes_sent = connection->statistic->real_bytes_sent + message->message_length  ;

          if (connection->connection_state == CL_COM_CONNECTED) {
             /* Touch endpoint, he is still active */
             if (connection->framework_type == CL_CT_TCP ) {
                cl_com_tcp_private_t* tcp_private = NULL;
                tcp_private = (cl_com_tcp_private_t*) connection->com_private;
                if (tcp_private != NULL) {
                   cl_endpoint_list_define_endpoint(cl_com_get_endpoint_list(),connection->remote, tcp_private->connect_port, connection->auto_close_type, 0 );
                }
             }

             switch (message->message_df) {
                case CL_MIH_DF_CCM:
                   CL_LOG(CL_LOG_WARNING,"sent connection close message");
                   connection->connection_sub_state = CL_COM_WAIT_FOR_CCRM;
                   connection->ccm_sent = 1;
                   break;

                case CL_MIH_DF_CCRM:
                   CL_LOG(CL_LOG_WARNING,"sent connection close response message");
                   connection->connection_sub_state = CL_COM_DONE;   /* CLOSE message */
                   connection->data_write_flag = CL_COM_DATA_NOT_READY;
                   connection->ccrm_sent = 1;
 
                   /* This client has been gone forever, remove his port from endpoint list */
                   cl_endpoint_list_undefine_endpoint(cl_com_get_endpoint_list(), connection->remote);
                   break;
           
                case CL_MIH_DF_BIN:
                   CL_LOG(CL_LOG_INFO,"sent binary message");
                   break;
                case CL_MIH_DF_XML:
                   CL_LOG(CL_LOG_INFO,"sent XML message");
                   break;
                case CL_MIH_DF_AM:
                   CL_LOG(CL_LOG_INFO,"sent acknowledge message");
                   break;
                case CL_MIH_DF_SIM:
                   CL_LOG(CL_LOG_INFO,"sent status information message");
                   break;
                case CL_MIH_DF_SIRM:
                   CL_LOG(CL_LOG_INFO,"sent status information response message");
                   break;
     
                default:
                   CL_LOG(CL_LOG_ERROR,"unexpected data format");
             }
          }
          connection->write_buffer_timeout_time = 0;
          gettimeofday(&message->message_send_time,NULL);
          /* set last transfer time of connection */
          memcpy(&connection->last_transfer_time,&message->message_send_time,sizeof(struct timeval) );
          switch(message->message_df) {
             case CL_MIH_DF_SIM:
                /* don't remove message */
                message->message_state = CL_MS_PROTOCOL; /* wait for SIRM, do not delete message */
                break;

             /* remove messages which don't want an acknoledge */
             default:
             if (message->message_mat == CL_MIH_MAT_NAK) {
                if (cl_message_list_remove_message(connection->send_message_list, message,0 ) == CL_RETVAL_OK) {
                   cl_com_free_message(&message);
                   CL_LOG(CL_LOG_INFO,"last sent message removed from send_message_list");
                }

                message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
             } else {
                message->message_state = CL_MS_PROTOCOL;  /* wait for ACK, do not delete message */
             }
          } /* switch */

          /* try to find out if there are more messages to send */
          message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
          while (message_list_elem != NULL) {
             message = message_list_elem->message;
             if (message->message_state != CL_MS_PROTOCOL && message->message_state != CL_MS_READY) {
                break;
             } 
             message = NULL;
             message_list_elem = cl_message_list_get_next_elem(connection->send_message_list,message_list_elem );
          } 
          if (message == NULL) {
             /* no messages to send, data is not ready */
             connection->data_write_flag = CL_COM_DATA_NOT_READY;
          }
       }
       cl_raw_list_unlock(connection->send_message_list);

       if (connection->ccm_received == 1) {
          CL_LOG(CL_LOG_INFO,"received ccm");
   
          CL_LOG_INT(CL_LOG_WARNING,"receive buffer:",cl_raw_list_get_elem_count(connection->received_message_list) );
          CL_LOG_INT(CL_LOG_WARNING,"send buffer   :",cl_raw_list_get_elem_count(connection->send_message_list) );
          CL_LOG_INT(CL_LOG_WARNING,"ccm_received  :",connection->ccm_received);

          if( cl_raw_list_get_elem_count(connection->send_message_list) == 0 && 
              cl_raw_list_get_elem_count(connection->received_message_list) == 0 ) {
             connection->ccm_received = 2;
             connection->connection_sub_state = CL_COM_SENDING_CCRM;
             /* disable #if 1 if you want to test a client, that is frozen! */
             cl_commlib_send_ccrm_message(connection);
             CL_LOG(CL_LOG_WARNING,"sending ccrm");
          } else {
             CL_LOG(CL_LOG_WARNING,"can't send ccrm, still messages in buffer");
          }
       }
    }
    return return_value;
}


/* receive message from host, component, component id from handle */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_receive_message()"
int cl_commlib_receive_message(cl_com_handle_t* handle,char* un_resolved_hostname, char* component_name, unsigned long component_id, int synchron, unsigned long response_mid, cl_com_message_t** message, cl_com_endpoint_t** sender ) {
   cl_com_connection_t* connection = NULL;
   cl_message_list_elem_t* message_elem = NULL;
   cl_connection_list_elem_t* elem = NULL;
   
   long my_timeout = 0;
   int message_sent = 0;
   struct timeval now;
   int leave_reason = CL_RETVAL_OK;

   if (message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   if (*message != NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (handle == NULL) {
      return CL_RETVAL_PARAMS;
   }

   if (synchron != 0) {
      gettimeofday(&now,NULL);
      my_timeout = now.tv_sec + handle->synchron_receive_timeout;
   }

   if ( un_resolved_hostname != NULL || component_name != NULL || component_id != 0) {
      CL_LOG(CL_LOG_WARNING,"message filtering not supported");
   }
   do {
      leave_reason = CL_RETVAL_OK;
      /* return if there are no connections in list */
      cl_raw_list_lock(handle->connection_list);
      elem = cl_connection_list_get_first_elem(handle->connection_list);     
      if (elem == NULL) {
         leave_reason = CL_RETVAL_CONNECTION_NOT_FOUND;
         if ( synchron != 0 && handle->service_provider == 0 ) {
            /* we are no service provider, we can't wait for a (possible)
               new connection, we can return immediately */
            cl_raw_list_unlock(handle->connection_list);
            return leave_reason;
         }
      }

      /* only search for messages if there are any messages in message state CL_MS_READY */
      pthread_mutex_lock(handle->messages_ready_mutex);
      if (handle->messages_ready_for_read > 0) {
         pthread_mutex_unlock(handle->messages_ready_mutex);

         cl_thread_clear_triggered_conditions(handle->app_condition);

         while(elem) {
            int endpoint_match = 1;
            connection = elem->connection;
             
            /* TODO: filter messages for endpoint, if specified and open connection if necessary  (use cl_commlib_open_connection()) */
            if (endpoint_match == 1) {
               /* ok, the endpoint matches the given parameters */  
   
               /* try to find complete received message in received message list of this connection */
               cl_raw_list_lock(connection->received_message_list);
               message_elem = cl_message_list_get_first_elem(connection->received_message_list);
               while(message_elem) {
      
                  if (message_elem->message->message_state == CL_MS_READY) {
                     int match = 1;  /* always match the message */
                    
         
                     /* try to find response for mid */
                     /* TODO: Just return a matchin response_mid !!!  0 = match all else match response_id */
                     if (response_mid != 0) {
                        if(message_elem->message->message_response_id != response_mid) {
                           /* if the response_mid parameter is set we can't return this message because
                              the response id is not the same. -> We have to wait for the correct message */
                           if ( response_mid > connection->last_send_message_id || connection->last_send_message_id == 0 ) {
                              CL_LOG(CL_LOG_ERROR,"protocol error: can't wait for unsent message!!!");
                           }


                           if ( response_mid > message_elem->message->message_response_id ) {
                              CL_LOG(CL_LOG_ERROR,"protocol error: There is still a lower message id than requested");
                           }

                           match = 0;  
                        } else {
                           CL_LOG_INT(CL_LOG_INFO,"received response for message id", response_mid);
                        }
                     } else {
                        /* never return a message with response id  when response_mid is 0 */
                        if (message_elem->message->message_response_id != 0) {
                           CL_LOG_INT(CL_LOG_ERROR,"message response id is set for this message:", message_elem->message->message_response_id);
                           if ( handle->do_shutdown == 0 ) {
                              CL_LOG(CL_LOG_ERROR,"return message without request, because handle goes down:");
                              match = 0;
                           }
                        }
                     }
   
                     if (match == 1) {
                        /* remove message from received message list*/
                        *message = message_elem->message;
                        cl_message_list_remove_message(connection->received_message_list, *message,0);

                        /* decrease counter for ready messages */
                        pthread_mutex_lock(handle->messages_ready_mutex);
                        handle->messages_ready_for_read = handle->messages_ready_for_read - 1;
                        pthread_mutex_unlock(handle->messages_ready_mutex);

                        

                        if (sender != NULL) {
                           *sender = cl_com_create_endpoint(connection->receiver->comp_host,
                                                            connection->receiver->comp_name,
                                                            connection->receiver->comp_id );
                        }
                        cl_raw_list_unlock(connection->received_message_list);

                       

                        /* send acknowledge for CL_MIH_MAT_ACK type (application removed message from buffer) */
                        if ( (*message)->message_mat == CL_MIH_MAT_ACK) {
                           cl_commlib_send_ack_message(connection, *message );
                           message_sent = 1; 
                        } 

                           
#if 1
                        /* now take list entry and add it at least if there are more connections */
                        /* this must be done in order to NOT prefer the first connection in list */
                        if ( cl_raw_list_get_elem_count(handle->connection_list) > 1) {
/*                           elem = cl_connection_list_get_first_elem(handle->connection_list); */
/*                           if (elem) { */
                              cl_raw_list_dechain_elem(handle->connection_list,elem->raw_elem);
                              cl_raw_list_append_dechained_elem(handle->connection_list,elem->raw_elem);
/*                           } */
                        }
#endif
                        

                        if (connection->ccm_received == 1) {
                           CL_LOG(CL_LOG_INFO,"received ccm");

                           CL_LOG_INT(CL_LOG_WARNING,"receive buffer:",cl_raw_list_get_elem_count(connection->received_message_list) );
                           CL_LOG_INT(CL_LOG_WARNING,"send buffer   :",cl_raw_list_get_elem_count(connection->send_message_list) );
                           CL_LOG_INT(CL_LOG_WARNING,"ccm_received  :",connection->ccm_received);


                           if( cl_raw_list_get_elem_count(connection->send_message_list) == 0 && 
                               cl_raw_list_get_elem_count(connection->received_message_list) == 0 ) {
                              connection->ccm_received = 2;
                              connection->connection_sub_state = CL_COM_SENDING_CCRM;
                              /* disable #if 1 if you want to test a client, that is frozen! */
                              cl_commlib_send_ccrm_message(connection);
                              message_sent = 1;
                              CL_LOG(CL_LOG_WARNING,"sending ccrm");
                           } else {
                              CL_LOG(CL_LOG_WARNING,"can't send ccrm, still messages in buffer");
                           }
                        }
  
                        handle->last_receive_message_connection = connection;                       

                        cl_raw_list_unlock(handle->connection_list);

                        if ( message_sent ) {
                           switch(cl_com_create_threads) {
                           case CL_NO_THREAD:
                              CL_LOG(CL_LOG_INFO,"no threads enabled");
                              /* we just want to trigger write , no wait for read*/
                              cl_commlib_trigger(handle);
                              break;
                           case CL_ONE_THREAD:
                              /* we just want to trigger write , no wait for read*/
                              CL_LOG(CL_LOG_WARNING,"trigger write thread");
                              cl_thread_trigger_event(handle->write_thread);
                              break;
                           }
                        }
                        return CL_RETVAL_OK;
                     }
                  }
                  /* get next message */
                  message_elem = cl_message_list_get_next_elem(connection->received_message_list, message_elem);
               }
               /* unlock message list */
               cl_raw_list_unlock(connection->received_message_list);
            } 
   
            /* get next connection, because we found no message or endpoint does not match */
            elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         }
         cl_raw_list_unlock(handle->connection_list);
         /* connection list */
      } else {
         pthread_mutex_unlock(handle->messages_ready_mutex);
         cl_raw_list_unlock(handle->connection_list);
      }
       
      if (synchron != 0) {
         int return_value;
         switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               cl_thread_trigger_event(handle->read_thread);
               return_value = cl_thread_wait_for_thread_condition(handle->app_condition,
                                                   handle->select_sec_timeout,
                                                   handle->select_usec_timeout);
               if (return_value == CL_RETVAL_CONDITION_WAIT_TIMEOUT) {
                  CL_LOG(CL_LOG_INFO,"APPLICATION GOT CONDITION WAIT TIMEOUT");
               }
               break;
         }
         /* at this point the handle->connection_list must be unlocked */
         if (leave_reason != CL_RETVAL_OK) {
            return leave_reason;   /* CL_RETVAL_CONNECTION_NOT_FOUND */
         }
         gettimeofday(&now,NULL);
         if (now.tv_sec > my_timeout) {
            return CL_RETVAL_SYNC_RECEIVE_TIMEOUT;
         }
      } 
   } while (synchron != 0 && cl_com_get_ignore_timeouts_flag() == CL_FALSE);
   return CL_RETVAL_NO_MESSAGE;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_send_ack_message()"
/* connection_list is locked inside of this call */
static int cl_commlib_send_ack_message(cl_com_connection_t* connection, cl_com_message_t* message) {
   cl_byte_t* ack_message_data = NULL;
   unsigned long ack_message_size = 0;
   int ret_val = CL_RETVAL_OK;
   cl_com_message_t* ack_message = NULL;

   if (connection == NULL || message == NULL) {
      return CL_RETVAL_PARAMS;
   }
   

   ack_message_size = CL_AM_MESSAGE_SIZE;
   ack_message_size = ack_message_size + strlen(CL_AM_MESSAGE_VERSION);
   ack_message_size = ack_message_size + cl_util_get_ulong_number_length(message->message_id);
   
   ack_message_data = (cl_byte_t*)malloc(sizeof(cl_byte_t)* ( ack_message_size + 1) ) ;
   if (ack_message_data == NULL) {
      return CL_RETVAL_MALLOC;
   }
   sprintf((char*)ack_message_data,CL_AM_MESSAGE  ,
                  CL_AM_MESSAGE_VERSION,
                  message->message_id );

 
   ret_val = cl_com_setup_message(&ack_message, connection, ack_message_data , ack_message_size ,CL_MIH_MAT_NAK , 0 ,0);
   if (ret_val != CL_RETVAL_OK) {
      return ret_val;
   }
   ack_message->message_df = CL_MIH_DF_AM;

   CL_LOG_INT(CL_LOG_INFO,"sending ack for message=", message->message_id);
   
   ret_val = cl_message_list_append_message(connection->send_message_list,ack_message,1);

   return ret_val;
}

/* connection_list is locked inside of this call */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_send_ccm_message()"
static int cl_commlib_send_ccm_message(cl_com_connection_t* connection) {
   cl_byte_t* ccm_message_data = NULL;
   unsigned long ccm_message_size = 0;
   int ret_val = CL_RETVAL_OK;
   cl_com_message_t* ccm_message = NULL;

   if (connection == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   ccm_message_size = CL_CCM_MESSAGE_SIZE;
   ccm_message_size = ccm_message_size + strlen(CL_CCM_MESSAGE_VERSION);
   
   ccm_message_data = (cl_byte_t*)malloc(sizeof(cl_byte_t)* ( ccm_message_size + 1) ) ;
   if (ccm_message_data == NULL) {
      return CL_RETVAL_MALLOC;
   }
   sprintf((char*)ccm_message_data,CL_CCM_MESSAGE , CL_CCM_MESSAGE_VERSION );

   ret_val = cl_com_setup_message(&ccm_message, connection, ccm_message_data , ccm_message_size , CL_MIH_MAT_NAK , 0 ,0);
   if (ret_val != CL_RETVAL_OK) {
      return ret_val;
   }
   ccm_message->message_df = CL_MIH_DF_CCM;
   CL_LOG(CL_LOG_INFO,"sending connection close message");
   ret_val = cl_message_list_append_message(connection->send_message_list,ccm_message,1);
   return ret_val;
}

/* connection_list is locked inside of this call */
static int cl_commlib_send_sim_message(cl_com_connection_t* connection, unsigned long* mid) {
   cl_byte_t* sim_message_data = NULL;
   unsigned long sim_message_size = 0;
   int ret_val = CL_RETVAL_OK;
   cl_com_message_t* sim_message = NULL;

   if (connection == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   sim_message_size = CL_SIM_MESSAGE_SIZE;
   sim_message_size = sim_message_size + strlen(CL_SIM_MESSAGE_VERSION);
   
   sim_message_data = (cl_byte_t*)malloc(sizeof(cl_byte_t)* ( sim_message_size + 1) ) ;
   if (sim_message_data == NULL) {
      return CL_RETVAL_MALLOC;
   }
   sprintf((char*)sim_message_data,CL_SIM_MESSAGE , CL_SIM_MESSAGE_VERSION );

   ret_val = cl_com_setup_message(&sim_message, connection, sim_message_data , sim_message_size , CL_MIH_MAT_NAK , 0 ,0);
   if (ret_val != CL_RETVAL_OK) {
      return ret_val;
   }
   sim_message->message_df = CL_MIH_DF_SIM;
   if (mid != NULL) {
      *mid = sim_message->message_id;
   }
   CL_LOG(CL_LOG_INFO,"sending information message (SIM)");
   ret_val = cl_message_list_append_message(connection->send_message_list,sim_message,1);
   return ret_val;
}

/* connection_list is locked inside of this call */
static int cl_commlib_send_sirm_message(cl_com_connection_t* connection, 
                                        cl_com_message_t* message,
                                        unsigned long starttime,
                                        unsigned long runtime,
                                        unsigned long buffered_read_messages,
                                        unsigned long buffered_write_messages,
                                        unsigned long connection_count,
                                        unsigned long application_status,
                                        char* infotext ) {
   cl_byte_t* sirm_message_data = NULL;
   unsigned long sirm_message_size = 0;
   int ret_val = CL_RETVAL_OK;
   cl_com_message_t* sirm_message = NULL;

   if (connection == NULL|| message == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   sirm_message_size = CL_SIRM_MESSAGE_SIZE;
   sirm_message_size = sirm_message_size + strlen(CL_SIRM_MESSAGE_VERSION);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(message->message_id);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(starttime);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(runtime);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(buffered_read_messages);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(buffered_write_messages);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(connection_count);
   sirm_message_size = sirm_message_size + cl_util_get_ulong_number_length(application_status);
   sirm_message_size = sirm_message_size + strlen(infotext);

   sirm_message_data = (cl_byte_t*)malloc(sizeof(cl_byte_t)* ( sirm_message_size + 1) ) ;
   if (sirm_message_data == NULL) {
      return CL_RETVAL_MALLOC;
   }
   sprintf((char*)sirm_message_data, CL_SIRM_MESSAGE , 
           CL_SIRM_MESSAGE_VERSION,
           message->message_id,
           starttime,
           runtime,
           buffered_read_messages,
           buffered_write_messages,
           connection_count,
           application_status,
           infotext );

   ret_val = cl_com_setup_message(&sirm_message, connection, sirm_message_data , sirm_message_size , CL_MIH_MAT_NAK , 0 ,0);
   if (ret_val != CL_RETVAL_OK) {
      return ret_val;
   }
   sirm_message->message_df = CL_MIH_DF_SIRM;
   CL_LOG_INT(CL_LOG_INFO,"sending SIRM for message=", message->message_id);

   ret_val = cl_message_list_append_message(connection->send_message_list,sirm_message,1);
   return ret_val;
}


/* connection_list is locked inside of this call */
#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_send_ccrm_message()"
static int cl_commlib_send_ccrm_message(cl_com_connection_t* connection) {
   cl_byte_t* ccrm_message_data = NULL;
   unsigned long ccrm_message_size = 0;
   int ret_val = CL_RETVAL_OK;
   cl_com_message_t* ccrm_message = NULL;

   if (connection == NULL ) {
      return CL_RETVAL_PARAMS;
   }

   ccrm_message_size = CL_CCRM_MESSAGE_SIZE;
   ccrm_message_size = ccrm_message_size + strlen(CL_CCRM_MESSAGE_VERSION);
   
   ccrm_message_data = (cl_byte_t*)malloc(sizeof(cl_byte_t)* ( ccrm_message_size + 1) ) ;
   if (ccrm_message_data == NULL) {
      return CL_RETVAL_MALLOC;
   }
   sprintf((char*)ccrm_message_data,CL_CCRM_MESSAGE , CL_CCRM_MESSAGE_VERSION );

   ret_val = cl_com_setup_message(&ccrm_message, connection, ccrm_message_data , ccrm_message_size , CL_MIH_MAT_NAK , 0 ,0);
   if (ret_val != CL_RETVAL_OK) {
      return ret_val;
   }
   ccrm_message->message_df = CL_MIH_DF_CCRM;
   CL_LOG(CL_LOG_INFO,"sending connection close response message");
   ret_val = cl_message_list_append_message(connection->send_message_list,ccrm_message,1);
   return ret_val;

}



/* ack_type: 
   CL_MIH_MAT_NAK  = send message and don't expect any acknowledge
   CL_MIH_MAT_ACK  = send message and block till communication partner application has read the message
   CL_MIH_MAT_SYNC = send message and block till communication partner application has processed all message actions (TODO)
*/

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_check_for_ack()"
int cl_commlib_check_for_ack(cl_com_handle_t* handle, char* un_resolved_hostname, char* component_name, unsigned long component_id, unsigned long mid , int do_block) {
   int found_message = 0;
   int message_added = 0;
   cl_connection_list_elem_t* elem = NULL;
   cl_com_connection_t* connection;
   cl_com_endpoint_t receiver;
   cl_message_list_elem_t* message_list_elem = NULL;
   cl_message_list_elem_t* next_message_list_elem = NULL;
   cl_com_message_t*     message = NULL;
   int return_value = CL_RETVAL_OK;
   char* unique_hostname = NULL;

   if ( handle == NULL) {
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }

   /* check endpoint parameters: un_resolved_hostname, componenet_name and componenet_id */
   if ( un_resolved_hostname == NULL || component_name == NULL || component_id == 0 ) {
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }

   /* resolve hostname */
   return_value = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname, NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      return return_value;
   }

   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;
   CL_LOG_STR(CL_LOG_INFO,"searching for connection to:",receiver.comp_host);


   while(1) {
      found_message = 0;
      /* lock handle connection list */
      cl_raw_list_lock(handle->connection_list);
      
      elem = cl_connection_list_get_first_elem(handle->connection_list);     
      connection = NULL;
      while(elem) {
         connection = elem->connection;
         if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
            break;
         }
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         connection = NULL;
      }
    
      if (connection != NULL) {
         /* check for message acknowledge */
         cl_raw_list_lock(connection->send_message_list);
   
         message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
         while(message_list_elem != NULL && found_message == 0) {
            message = message_list_elem->message;
            next_message_list_elem = cl_message_list_get_next_elem(connection->send_message_list, message_list_elem );
            if (message->message_id == mid) {
               /* found message */
               found_message = 1;
               if (message->message_ack_flag == 1) {
                  cl_message_list_remove_message(connection->send_message_list, message,0 );
                  cl_com_free_message(&message);
                  cl_raw_list_unlock(connection->send_message_list);

                  if (connection->ccm_received == 1) {
                     CL_LOG(CL_LOG_INFO,"received ccm");
              
                     CL_LOG_INT(CL_LOG_WARNING,"receive buffer:",cl_raw_list_get_elem_count(connection->received_message_list) );
                     CL_LOG_INT(CL_LOG_WARNING,"send buffer   :",cl_raw_list_get_elem_count(connection->send_message_list) );
                     CL_LOG_INT(CL_LOG_WARNING,"ccm_received  :",connection->ccm_received);
           
                     if( cl_raw_list_get_elem_count(connection->send_message_list) == 0 && 
                         cl_raw_list_get_elem_count(connection->received_message_list) == 0 ) {
                        connection->ccm_received = 2;
                        connection->connection_sub_state = CL_COM_SENDING_CCRM;
                        /* disable #if 1 if you want to test a client, that is frozen! */
                        cl_commlib_send_ccrm_message(connection);
                        message_added = 1;
                        CL_LOG(CL_LOG_WARNING,"sending ccrm");
                     } else {
                        CL_LOG(CL_LOG_WARNING,"can't send ccrm, still messages in buffer");
                     }
                  }

                  cl_raw_list_unlock(handle->connection_list);
                  free(unique_hostname);
                  CL_LOG_INT(CL_LOG_INFO,"got message acknowledge:",mid);
                  if (message_added) {
                     switch(cl_com_create_threads) {
                        case CL_NO_THREAD:
                           CL_LOG(CL_LOG_INFO,"no threads enabled");
                           /* we just want to trigger write , no wait for read*/
                           cl_commlib_trigger(handle);
                           break;
                        case CL_ONE_THREAD:
                           /* we just want to trigger write , no wait for read*/
                           cl_thread_trigger_event(handle->write_thread);
                           break;
                     }
                  }


                  return CL_RETVAL_OK;
               } else {
                  CL_LOG_INT(CL_LOG_INFO,"message is not acknowledged:", mid);
               }
            }
            message_list_elem = next_message_list_elem;
         }
         cl_raw_list_unlock(connection->send_message_list);
      } else {
         CL_LOG(CL_LOG_ERROR,"no connection FOUND");
         cl_raw_list_unlock(handle->connection_list);
         free(unique_hostname);
         return CL_RETVAL_CONNECTION_NOT_FOUND; 
      }
      cl_raw_list_unlock(handle->connection_list);

      if (found_message == 0) {
         CL_LOG_INT(CL_LOG_ERROR,"message not found or removed because of ack timeout", mid);
         free(unique_hostname);
         return CL_RETVAL_MESSAGE_ACK_ERROR; /* message not found or removed because of ack timeout */
      }

      if (do_block != 0) {
         switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               cl_thread_wait_for_thread_condition(handle->read_condition,
                                                   handle->select_sec_timeout,
                                                   handle->select_usec_timeout);
               break;
         } /* switch */
      } else  {
         free(unique_hostname);
         return CL_RETVAL_MESSAGE_WAIT_FOR_ACK;
      }
   } /* while (1) */
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_open_connection()"
int cl_commlib_open_connection(cl_com_handle_t* handle, char* un_resolved_hostname, char* component_name, unsigned long component_id) {

   int ret_val;
   char* unique_hostname = NULL;
   cl_com_endpoint_t receiver;
   cl_connection_list_elem_t* elem = NULL;
   cl_com_connection_t* connection = NULL;
   cl_com_connection_t* new_con = NULL;
   cl_com_endpoint_t* remote_endpoint = NULL;
   cl_com_endpoint_t* local_endpoint  = NULL;
   cl_com_endpoint_t* receiver_endpoint  = NULL;
   cl_com_endpoint_t* sender_endpoint  = NULL;

   int shutdown_received = 0;
   struct timeval now;

   /* check endpoint parameters: un_resolved_hostname , componenet_name and componenet_id */
   if ( un_resolved_hostname == NULL || component_name == NULL || component_id == 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_UNKNOWN_ENDPOINT));
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }


   CL_LOG_STR(CL_LOG_INFO,"open host           :",un_resolved_hostname );
   CL_LOG_STR(CL_LOG_INFO,"open component_name :",component_name );
   CL_LOG_INT(CL_LOG_INFO,"open component_id   :",component_id );


   if ( handle == NULL) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_HANDLE_NOT_FOUND));
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }
 
   /* resolve hostname */
   ret_val = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname,NULL, NULL, NULL);
   if (ret_val != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(ret_val));
      return ret_val;
   }


   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;
   CL_LOG_STR(CL_LOG_INFO,"searching for connection to:",receiver.comp_host);



   /* lock handle mutex, because we don't want to allow more threads to create a new 
      connection for the same receiver endpoint */
   pthread_mutex_lock(handle->connection_list_mutex);


   /* lock handle connection list */
   cl_raw_list_lock(handle->connection_list);

   elem = cl_connection_list_get_first_elem(handle->connection_list);     
   connection = NULL;
   while(elem) {

      connection = elem->connection;
      if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
         if (connection->ccm_received != 0) {  
            /* we have sent connection close response, do not accept any new message */
            /* we wait till connection is down and try to reconnect  */
            CL_LOG(CL_LOG_ERROR,"connection is open, but going down now");
            gettimeofday(&now,NULL);
            connection->shutdown_timeout = now.tv_sec + handle->acknowledge_timeout;
            shutdown_received = 1;
            break;
         }

         if (connection->connection_state == CL_COM_CONNECTED &&
             connection->connection_sub_state != CL_COM_WORK) {
            CL_LOG(CL_LOG_ERROR,"connection is open, but going down now");
            gettimeofday(&now,NULL);
            connection->shutdown_timeout = now.tv_sec + handle->acknowledge_timeout;
            shutdown_received = 2;
            break;
         }
         /* connection is open, just return */
         cl_raw_list_unlock(handle->connection_list);
         CL_LOG(CL_LOG_WARNING,"connection is allready open");
         free(unique_hostname);
         unique_hostname = NULL;
         receiver.comp_host = NULL;

         /* unlock connection list */
         pthread_mutex_unlock(handle->connection_list_mutex);
         return CL_RETVAL_OK;
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
      connection = NULL;
   }
   cl_raw_list_unlock(handle->connection_list);


   if (shutdown_received != 0) {
      /* the connection is going down -> wait for removal of connection*/  
      int still_in_list = 1;
      /* TODO: save unread messages with state READY in new connection object? If so,
         the application don't have to take care about any error messages. But for now
         it should be ok to let the application hande those kind of problems. (gdi) */

      /* we don't have a port to connect to, re open would fail, return now */
      if (handle->connect_port <= 0) {
         int tcp_port = 0;
         if ( cl_com_get_known_endpoint_port(&receiver, &tcp_port) != CL_RETVAL_OK) {
            CL_LOG(CL_LOG_ERROR,"no port to connect");
            /* unlock connection list */
            pthread_mutex_unlock(handle->connection_list_mutex);
            return CL_RETVAL_NO_PORT_ERROR;
         }
      }

      while (still_in_list == 1) {
         int message_sent = 0;

         still_in_list = 0;
         /* lock handle connection list */
         cl_raw_list_lock(handle->connection_list);
         elem = cl_connection_list_get_first_elem(handle->connection_list);
         while(elem) {
            message_sent = 0;
            connection = elem->connection;
            if ( cl_com_compare_endpoints(connection->receiver, &receiver)) {
               struct timeval now;
               still_in_list = 1;
/**/
               if ( shutdown_received == 1 ) {
                  if (connection->ccm_received == 0) {  
                     /* This must be a new connection ( initiated from other endpoint ), so 
                        we return that connection is already open */
                     cl_raw_list_unlock(handle->connection_list);
                     free(unique_hostname);
                     unique_hostname = NULL;
                     receiver.comp_host = NULL;

                     pthread_mutex_unlock(handle->connection_list_mutex);
                     CL_LOG(CL_LOG_INFO,"This is a new connected client, don't reopen, use new connection");
                     return CL_RETVAL_OK;
                  }
               } else {
                  if ( (connection->connection_state == CL_COM_CONNECTED && connection->connection_sub_state == CL_COM_WORK) || 
                        connection->connection_state == CL_COM_OPENING ||
                        connection->connection_state == CL_COM_CONNECTING ||
                        connection->connection_state == CL_COM_DISCONNECTED) {
                     /* This must be a new connection ( initiated from other endpoint ), so 
                        we return that connection is already open */
                     cl_raw_list_unlock(handle->connection_list);
                     free(unique_hostname);
                     unique_hostname = NULL;
                     receiver.comp_host = NULL;

                     pthread_mutex_unlock(handle->connection_list_mutex);
                     CL_LOG(CL_LOG_INFO,"This is a new connected client, don't reopen, use new connection");
                     return CL_RETVAL_OK;
                  }
               }
/**/
               CL_LOG(CL_LOG_WARNING,"connection still alive ...");
               CL_LOG_INT(CL_LOG_WARNING,"receive buffer:",cl_raw_list_get_elem_count(connection->received_message_list) );
               CL_LOG_INT(CL_LOG_WARNING,"send buffer   :",cl_raw_list_get_elem_count(connection->send_message_list) );
               
               if ( connection->ccm_received == 1 && 
                    cl_raw_list_get_elem_count(connection->received_message_list) == 0 &&  
                    cl_raw_list_get_elem_count(connection->send_message_list) == 0 ) {
                  CL_LOG(CL_LOG_WARNING,"received ccm");
                  connection->ccm_received = 2;
                  connection->connection_sub_state = CL_COM_SENDING_CCRM;
                  cl_commlib_send_ccrm_message(connection);
                  message_sent = 1;
                  CL_LOG(CL_LOG_WARNING,"sending ccrm");
               }

               /* There are messages to read for application, return */
               if ( cl_raw_list_get_elem_count(connection->received_message_list) != 0 ) {
                  cl_raw_list_unlock(handle->connection_list);
                  free(unique_hostname);
                  unique_hostname = NULL;
                  receiver.comp_host = NULL;

                  pthread_mutex_unlock(handle->connection_list_mutex);
                  return CL_RETVAL_OK;
               }

               gettimeofday(&now,NULL);
               if (connection->shutdown_timeout <= now.tv_sec || cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                  CL_LOG(CL_LOG_WARNING,"got timeout while waiting for connection close");
                  connection->connection_state = CL_COM_CLOSING;
                  cl_raw_list_unlock(handle->connection_list);
                  free(unique_hostname);
                  unique_hostname = NULL;
                  receiver.comp_host = NULL;

                  /* unlock connection list */
                  pthread_mutex_unlock(handle->connection_list_mutex);
                  return CL_RETVAL_CONNECTION_GOING_DOWN;
               }
            }
            elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         }
         cl_raw_list_unlock(handle->connection_list);
         

         if (still_in_list == 1) {
            switch(cl_com_create_threads) {
               case CL_NO_THREAD:
                  CL_LOG(CL_LOG_INFO,"no threads enabled");
                  cl_commlib_trigger(handle);
                  break;
               case CL_ONE_THREAD:
                  /* unlock connection list */
                  if (message_sent != 0) {
                     cl_thread_trigger_event(handle->write_thread);
                  }
                  cl_thread_wait_for_thread_condition(handle->read_condition,
                                                   handle->select_sec_timeout,
                                                   handle->select_usec_timeout);
                  break;
            }
            /* at this point the handle->connection_list must be unlocked */
         }

      }
      CL_LOG(CL_LOG_WARNING,"connection is down! Try to reopen ...");
   }
   
   CL_LOG(CL_LOG_INFO,"open new connection"); 
     
   ret_val = cl_com_setup_connection(handle, &new_con);
   if (ret_val != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"could not setup connection");
      cl_com_close_connection(&new_con);
      free(unique_hostname);
      unique_hostname = NULL;
      receiver.comp_host = NULL;

      /* unlock connection list */
      pthread_mutex_unlock(handle->connection_list_mutex);
      return ret_val;
   }
  
   /* now open connection to the endpoint */
   local_endpoint    = cl_com_create_endpoint(handle->local->comp_host, handle->local->comp_name, handle->local->comp_id);
   sender_endpoint   = cl_com_create_endpoint(handle->local->comp_host, handle->local->comp_name, handle->local->comp_id);
   remote_endpoint   = cl_com_create_endpoint(receiver.comp_host, component_name , component_id);
   receiver_endpoint = cl_com_create_endpoint(receiver.comp_host, component_name , component_id);
      

   ret_val = cl_com_open_connection(new_con, handle->open_connection_timeout, remote_endpoint,local_endpoint, receiver_endpoint, sender_endpoint);

   cl_com_free_endpoint(&remote_endpoint);
   cl_com_free_endpoint(&local_endpoint);
   cl_com_free_endpoint(&receiver_endpoint);
   cl_com_free_endpoint(&sender_endpoint);

   if (ret_val != CL_RETVAL_OK && ret_val != CL_RETVAL_UNCOMPLETE_WRITE ) {
      CL_LOG(CL_LOG_ERROR,"could not open connection");
      cl_com_close_connection(&new_con);
      free(unique_hostname);
      unique_hostname = NULL;
      receiver.comp_host = NULL;

      /* unlock connection list */
      pthread_mutex_unlock(handle->connection_list_mutex);

      /* do this for compatibility to remote clients, which
         will never immediately will return a connect error */
      switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         cl_commlib_trigger(handle);
         break;
      case CL_ONE_THREAD:
         cl_thread_trigger_event(handle->read_thread);
         cl_commlib_trigger(handle); /* TODO: check if this is ok ???  */
         break;
      }

      return ret_val;
   }
   new_con->handler = handle;


   /* lock connection list */
   cl_raw_list_lock(handle->connection_list);
   
   /* Check if this connection is unique */
   elem = cl_connection_list_get_first_elem(handle->connection_list);
   while(elem != NULL) {
      if ( cl_com_compare_endpoints(elem->connection->receiver, &receiver)) {
         break;
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
   }
   if (elem == NULL) {
      /* endpoint is unique, add it to connection list */
      ret_val = cl_connection_list_append_connection(handle->connection_list, new_con,0);
      cl_raw_list_unlock(handle->connection_list);
   } else {
      if ( elem->connection->connection_state != CL_COM_CLOSING ) {
         CL_LOG(CL_LOG_INFO,"try to open connection to already connected endpoint");

         cl_raw_list_unlock(handle->connection_list);
         cl_com_close_connection(&new_con);
         free(unique_hostname); /* don't access receiver after this */
         unique_hostname = NULL;
         receiver.comp_host = NULL;

         /* unlock connection list */
         pthread_mutex_unlock(handle->connection_list_mutex);
         return CL_RETVAL_OK;
      } else {
         CL_LOG(CL_LOG_ERROR,"client not unique error, can't add opened connection into connection list");
         cl_raw_list_unlock(handle->connection_list);
         cl_com_close_connection(&new_con);
         free(unique_hostname); /* don't access receiver after this */
         unique_hostname = NULL;
         receiver.comp_host = NULL;

         /* unlock connection list */
         pthread_mutex_unlock(handle->connection_list_mutex);
         return CL_RETVAL_ENDPOINT_NOT_UNIQUE;
      }
   }

   free(unique_hostname); /* don't access receiver after this */
   unique_hostname = NULL;
   receiver.comp_host = NULL;

   CL_LOG(CL_LOG_INFO,"new connection created");
   handle->statistic->new_connections =  handle->statistic->new_connections + 1;
   /* unlock connection list */
   pthread_mutex_unlock(handle->connection_list_mutex);

   switch(cl_com_create_threads) {
      case CL_NO_THREAD:
         CL_LOG(CL_LOG_INFO,"no threads enabled");
         cl_commlib_trigger(handle);
         break;
      case CL_ONE_THREAD:
         /* new connection, trigger read_thread */
         cl_thread_trigger_event(handle->write_thread);
         cl_thread_trigger_event(handle->read_thread);
         break;
   }
   
   return ret_val;
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_close_connection()"
int cl_commlib_close_connection(cl_com_handle_t* handle,char* un_resolved_hostname, char* component_name, unsigned long component_id) {
   int closed = 0;
   int return_value = CL_RETVAL_OK;
   cl_bool_t trigger_write = CL_FALSE;
   char* unique_hostname = NULL;
   cl_com_endpoint_t receiver;
   cl_connection_list_elem_t* elem = NULL;
   cl_com_connection_t* connection = NULL;


   if ( handle == NULL) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_HANDLE_NOT_FOUND));
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }
   /* check endpoint parameters: un_resolved_hostname , componenet_name and componenet_id */
   if ( un_resolved_hostname == NULL || component_name == NULL || component_id == 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_UNKNOWN_ENDPOINT));
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }
   /* resolve hostname */
   return_value = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname,NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(return_value));
      return return_value;
   }
   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;
   CL_LOG_STR(CL_LOG_INFO,"searching for connection to:",receiver.comp_host);
   /* lock handle connection list */
   cl_raw_list_lock(handle->connection_list);

   elem = cl_connection_list_get_first_elem(handle->connection_list);     
   connection = NULL;
   while(elem) {
      connection = elem->connection;
      if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
         if (connection->data_flow_type == CL_CM_CT_MESSAGE) {
            if (connection->connection_state == CL_COM_CONNECTED &&
                connection->connection_sub_state == CL_COM_WORK  &&
                connection->ccm_received         == 0) {
               cl_commlib_send_ccm_message(connection);
               trigger_write = CL_TRUE;
               connection->connection_sub_state = CL_COM_SENDING_CCM;
               CL_LOG_STR(CL_LOG_WARNING,"closing connection to host:", connection->remote->comp_host );
               CL_LOG_STR(CL_LOG_WARNING,"component name:            ", connection->remote->comp_name );
               CL_LOG_INT(CL_LOG_WARNING,"component id:              ", connection->remote->comp_id );
               closed = 1;
            }
         } 
         if (connection->data_flow_type == CL_CM_CT_STREAM) {
            CL_LOG(CL_LOG_WARNING,"closing stream connection");
            CL_LOG_STR(CL_LOG_WARNING,"closing connection to host:", connection->remote->comp_host );
            CL_LOG_STR(CL_LOG_WARNING,"component name:            ", connection->remote->comp_name );
            CL_LOG_INT(CL_LOG_WARNING,"component id:              ", connection->remote->comp_id );
            connection->connection_state = CL_COM_CLOSING;
            closed = 1;
         }
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
      connection = NULL;
   }
   cl_raw_list_unlock(handle->connection_list);
   free(unique_hostname);
   unique_hostname = NULL;
   if ( trigger_write == CL_TRUE ) {
      switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               /* we just want to trigger write , no wait for read*/
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               /* we just want to trigger write , no wait for read*/
               cl_thread_trigger_event(handle->write_thread);
               break;
      }
   }
   if (closed == 1) {
      return CL_RETVAL_OK;
   }
   return CL_RETVAL_CONNECTION_NOT_FOUND;
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_get_endpoint_status()"
int cl_commlib_get_endpoint_status(cl_com_handle_t* handle,
                                   char* un_resolved_hostname, char* component_name, unsigned long component_id,
                                   cl_com_SIRM_t** status) {


   cl_com_connection_t* connection = NULL;
   cl_connection_list_elem_t* elem = NULL;
   int message_added = 0;
   unsigned long my_mid = 0;
   int return_value = CL_RETVAL_OK;
   cl_com_endpoint_t receiver;
   char* unique_hostname = NULL;
   int retry_send = 1;
   int found_message = 0;
   cl_message_list_elem_t* message_list_elem = NULL;
   cl_message_list_elem_t* next_message_list_elem = NULL;

   cl_com_message_t*     message = NULL;



   if ( handle == NULL || status == NULL) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_HANDLE_NOT_FOUND));
      return CL_RETVAL_PARAMS;
   }

   /* check endpoint parameters: un_resolved_hostname , componenet_name and componenet_id */
   if ( un_resolved_hostname == NULL || component_name == NULL || component_id == 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_UNKNOWN_ENDPOINT));
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }

   

   if (*status != NULL) {
      CL_LOG(CL_LOG_ERROR,"expected empty status pointer address");
      return CL_RETVAL_PARAMS;
   }
   
   CL_LOG_STR(CL_LOG_WARNING,"to host           :",un_resolved_hostname );
   CL_LOG_STR(CL_LOG_WARNING,"to component_name :",component_name );
   CL_LOG_INT(CL_LOG_WARNING,"to component_id   :",component_id );

   /* resolve hostname */
   return_value = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname,NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(return_value));
      return return_value;
   }

   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;
   CL_LOG_STR(CL_LOG_INFO,"searching for connection to:",receiver.comp_host);


   while(retry_send != 0) {
   
      /* lock handle connection list */
      cl_raw_list_lock(handle->connection_list);
      elem = cl_connection_list_get_first_elem(handle->connection_list);     
      connection = NULL;
      while(elem) {
   
         connection = elem->connection;
   
         /* send message to client (no broadcast) */
         if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
   
            if (connection->ccm_received != 0) {  
               /* we have sent connection close response, do not accept any new message */
               /* we wait till connection is down and try to reconnect  */
               CL_LOG(CL_LOG_ERROR,"connection is going down now, can't send message (ccrm sent)");
               break;
            }
   
            if (connection->connection_state == CL_COM_CONNECTED && connection->connection_sub_state != CL_COM_WORK) {
               CL_LOG(CL_LOG_WARNING,"connection is going down now, can't send message");
               break;
            }
   
            return_value = cl_commlib_send_sim_message(connection, &my_mid); 
   
            if (return_value != CL_RETVAL_OK) {
               cl_raw_list_unlock(handle->connection_list);
               free(unique_hostname);
               return return_value;
            }
            message_added = 1;
            break;
         }
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         connection = NULL;
      }
      cl_raw_list_unlock(handle->connection_list);

      /* if message is not added, the connection was not found -> try to open it */
      if (message_added != 1) {
         retry_send++;  
         return_value = cl_commlib_open_connection(handle, un_resolved_hostname, component_name, component_id );
         if (return_value != CL_RETVAL_OK) {
            free(unique_hostname);
            CL_LOG_STR(CL_LOG_ERROR,"cl_commlib_open_connection() returned: ",cl_get_error_text(return_value));
            return return_value;
         }
         if (retry_send >= 3) {
            CL_LOG(CL_LOG_ERROR,"can't open connection, don't retry to send this message");
            retry_send = 0;
         }
      } else {
         retry_send = 0; /* break */
      }
   }


   if (message_added == 1) {
      switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               /* we just want to trigger write , no wait for read*/
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               /* we just want to trigger write , no wait for read*/
               cl_thread_trigger_event(handle->write_thread);
               break;
      }
   } else {
      free(unique_hostname);
      return CL_RETVAL_SEND_ERROR;
   } 
 
   CL_LOG_INT(CL_LOG_WARNING,"waiting for SIRM with id",my_mid);
   while(1) {
      found_message = 0;
      /* lock handle connection list */
      cl_raw_list_lock(handle->connection_list);
      
      elem = cl_connection_list_get_first_elem(handle->connection_list);     
      connection = NULL;
      while(elem) {
         connection = elem->connection;
         if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
            break;
         }
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         connection = NULL;
      }
      if (connection != NULL) {
         /* check for message acknowledge */
         cl_raw_list_lock(connection->send_message_list);
   
         message_list_elem = cl_message_list_get_first_elem(connection->send_message_list);
         while(message_list_elem != NULL && found_message == 0) {
            message = message_list_elem->message;
            next_message_list_elem = cl_message_list_get_next_elem(connection->send_message_list, message_list_elem );
            if (message->message_id == my_mid) {
               /* found message */
               found_message = 1;
               if (message->message_sirm != NULL) {
                  cl_message_list_remove_message(connection->send_message_list, message,0 );
                  *status = message->message_sirm;
                  message->message_sirm = NULL;
                  cl_com_free_message(&message);
                  cl_raw_list_unlock(connection->send_message_list);
                  cl_raw_list_unlock(handle->connection_list);
                  free(unique_hostname);
                  CL_LOG_INT(CL_LOG_WARNING,"got SIRM for SIM with id:",my_mid);
                  return CL_RETVAL_OK;
               } else {
                  CL_LOG_INT(CL_LOG_WARNING,"no SRIM for SIM with id", my_mid);
               }
            }
            message_list_elem = next_message_list_elem;
         }
         cl_raw_list_unlock(connection->send_message_list);
      } else {
         CL_LOG(CL_LOG_ERROR,"no connection FOUND");
         cl_raw_list_unlock(handle->connection_list);
         free(unique_hostname);
         return CL_RETVAL_CONNECTION_NOT_FOUND; 
      }
      cl_raw_list_unlock(handle->connection_list);

      if (found_message == 0) {
         CL_LOG_INT(CL_LOG_ERROR,"SIM not found or removed because of SIRM ack timeout", my_mid);
         free(unique_hostname);
         return CL_RETVAL_MESSAGE_ACK_ERROR; /* message not found or removed because of ack timeout */
      }

      switch(cl_com_create_threads) {
         case CL_NO_THREAD:
            CL_LOG(CL_LOG_INFO,"no threads enabled");
            cl_commlib_trigger(handle);
            break;
         case CL_ONE_THREAD:
            cl_thread_wait_for_thread_condition(handle->read_condition,
                                                handle->select_sec_timeout,
                                                handle->select_usec_timeout);
            break;
      } 
   } /* while(1) */
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_send_message()"
int cl_commlib_send_message(cl_com_handle_t* handle,
                            char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                            cl_xml_ack_type_t ack_type, 
                            cl_byte_t* data, unsigned long size , 
                            unsigned long* mid, unsigned long response_mid, unsigned long tag ,
                            int copy_data,
                            int wait_for_ack) {


   cl_com_connection_t* connection = NULL;
   cl_connection_list_elem_t* elem = NULL;
   cl_com_message_t* message = NULL;
   int message_added = 0;
   unsigned long my_mid = 0;
   int return_value = CL_RETVAL_OK;
   cl_com_endpoint_t receiver;
   char* unique_hostname = NULL;
   int retry_send = 1;


   /* check acknowledge method */
   if (ack_type == CL_MIH_MAT_UNDEFINED || data == NULL || size == 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_PARAMS));
      return CL_RETVAL_PARAMS;
   }

   if ( handle == NULL) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_HANDLE_NOT_FOUND));
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }

   /* check endpoint parameters: un_resolved_hostname , componenet_name and componenet_id */
   if ( un_resolved_hostname == NULL || component_name == NULL || component_id == 0 ) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(CL_RETVAL_UNKNOWN_ENDPOINT));
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }
   
   CL_LOG_STR(CL_LOG_INFO,"to host           :",un_resolved_hostname );
   CL_LOG_STR(CL_LOG_INFO,"to component_name :",component_name );
   CL_LOG_INT(CL_LOG_INFO,"to component_id   :",component_id );


   /* resolve hostname */
   return_value = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname,NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,cl_get_error_text(return_value));
      return return_value;
   }


   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;
   CL_LOG_STR(CL_LOG_INFO,"searching for connection to:",receiver.comp_host);


   while(retry_send != 0) {
   
      /* lock handle connection list */
      cl_raw_list_lock(handle->connection_list);
      elem = cl_connection_list_get_first_elem(handle->connection_list);     
      connection = NULL;
      while(elem) {
   
         connection = elem->connection;
   
         /* send message to client (no broadcast) */
         if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
            cl_byte_t* help_data = NULL;
   
            if (connection->ccm_received != 0) {  
               /* we have sent connection close response, do not accept any new message */
               /* we wait till connection is down and try to reconnect  */
               CL_LOG(CL_LOG_ERROR,"connection is going down now, can't send message (ccrm sent)");
               break;
            }
   
            if (connection->connection_state == CL_COM_CONNECTED && connection->connection_sub_state != CL_COM_WORK) {
               CL_LOG(CL_LOG_WARNING,"connection is going down now, can't send message");
               break;
            }
   

            if (  response_mid > 0 && response_mid > connection->last_received_message_id ) {
               CL_LOG_INT(CL_LOG_DEBUG,"last_received_message_id:", connection->last_received_message_id );
               CL_LOG_INT(CL_LOG_DEBUG,"last_send_message_id    :", connection->last_send_message_id);
               CL_LOG_INT(CL_LOG_DEBUG,"response_mid to send    :", response_mid);

               CL_LOG(CL_LOG_ERROR,"Protocol error: haven't received such a high message id till now");
               cl_raw_list_unlock(handle->connection_list);
               free(unique_hostname);
               return CL_RETVAL_PROTOCOL_ERROR;
            }    

            CL_LOG_STR(CL_LOG_DEBUG,"sending to:", connection->receiver->comp_host); 
            if (copy_data == 1) {
               help_data = (cl_byte_t*) malloc(sizeof(cl_byte_t)*size);
               if (help_data == NULL) {
                  cl_raw_list_unlock(handle->connection_list);
                  free(unique_hostname);
                  return CL_RETVAL_MALLOC;
               }
               memcpy(help_data, data,sizeof(cl_byte_t)*size );
               return_value = cl_com_setup_message(&message, connection, help_data, size,ack_type,response_mid,tag);
            } else {
               return_value = cl_com_setup_message(&message, connection, data, size, ack_type,response_mid,tag);
            }
   
            if (return_value != CL_RETVAL_OK) {
               cl_raw_list_unlock(handle->connection_list);
               free(unique_hostname);
               return return_value;
            }
   
            my_mid = message->message_id;
            if (mid != NULL) {
              *mid = message->message_id;
            }
            return_value = cl_message_list_append_message(connection->send_message_list,message,1);
            if (return_value != CL_RETVAL_OK) {
               cl_com_free_message(&message);
               cl_raw_list_unlock(handle->connection_list);
               free(unique_hostname);
               return return_value;
            }
            message_added = 1;
            
            break;
         }
         elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         connection = NULL;
      }
      cl_raw_list_unlock(handle->connection_list);

      /* if message is not added, the connection was not found -> try to open it */
      if (message_added != 1) {
         retry_send++;  
         return_value = cl_commlib_open_connection(handle, un_resolved_hostname, component_name, component_id );
         if (return_value != CL_RETVAL_OK) {
            free(unique_hostname);
            CL_LOG_STR(CL_LOG_ERROR,"cl_commlib_open_connection() returned: ",cl_get_error_text(return_value));
            return return_value;
         }
         if (retry_send >= 3) {
            CL_LOG(CL_LOG_ERROR,"can't open connection, don't retry to send this message");
            retry_send = 0;
         }
      } else {
         retry_send = 0; /* break */
      }
   }

   if (message_added == 1) {
      switch(cl_com_create_threads) {
            case CL_NO_THREAD:
               CL_LOG(CL_LOG_INFO,"no threads enabled");
               /* we just want to trigger write , no wait for read*/
               cl_commlib_trigger(handle);
               break;
            case CL_ONE_THREAD:
               /* we just want to trigger write , no wait for read*/
               cl_thread_trigger_event(handle->write_thread);
               break;
      }
   } else {
      free(unique_hostname);
      return CL_RETVAL_SEND_ERROR;
   } 
 
   if (ack_type == CL_MIH_MAT_NAK) {
      free(unique_hostname);
      return CL_RETVAL_OK;
   }

   if (wait_for_ack == 0) {
      free(unique_hostname);
      return CL_RETVAL_OK;
   }

   CL_LOG_INT(CL_LOG_INFO,"message acknowledge expected, waiting for ack", my_mid);
   return_value = cl_commlib_check_for_ack(handle, receiver.comp_host, component_name, component_id, my_mid,1);
   free(unique_hostname);
   return return_value;
}







#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_commlib_get_last_message_time()"
int cl_commlib_get_last_message_time(cl_com_handle_t* handle, 
                                    char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                                    unsigned long* time) {

   char* unique_hostname = NULL;
   int return_value;
   cl_com_endpoint_t receiver;

   /* set time to 0 if endpoint not found, otherwise return last communication time */
   /* otherwise return error */
   if (time) {
      *time = 0;
   }

   if (handle == NULL || un_resolved_hostname == NULL || component_name == NULL) {
      return CL_RETVAL_PARAMS;
   }
   
   if (component_id <= 0) {
      CL_LOG(CL_LOG_ERROR,"component id 0 is not allowed");
      return CL_RETVAL_PARAMS;
   }

   /* resolve hostname */
   return_value = cl_com_cached_gethostbyname(un_resolved_hostname, &unique_hostname, NULL, NULL, NULL);
   if (return_value != CL_RETVAL_OK) {
      return return_value;
   }

   /* setup endpoint */
   receiver.comp_host = unique_hostname;
   receiver.comp_name = component_name;
   receiver.comp_id   = component_id;


   return_value = cl_endpoint_list_get_last_touch_time(cl_com_get_endpoint_list(),&receiver,time);
   if (time) {
      CL_LOG_STR(CL_LOG_DEBUG,"host              :", receiver.comp_host);
      CL_LOG_STR(CL_LOG_DEBUG,"component         :", receiver.comp_name);
      CL_LOG_INT(CL_LOG_DEBUG,"last transfer time:", *time);
   }

   free(unique_hostname); /* don't touch receiver object after this */

   return return_value;

#if 0

   /* This was old behaviour, when there was no cl_endpoint_list !!! */
   /* This code can be removed when endpoint list is used in future - CR */

   /* lock handle connection list */
   cl_raw_list_lock(handle->connection_list);

   elem = cl_connection_list_get_first_elem(handle->connection_list);
   while(elem) {
      connection = elem->connection;
      if ( cl_com_compare_endpoints(connection->receiver, &receiver) ) {
         /* found endpoint */
         if (time) {
            *time = (connection->last_transfer_time).tv_sec;
         }
         CL_LOG(CL_LOG_INFO,"found connection");
         cl_raw_list_unlock(handle->connection_list);
         free(unique_hostname); /* don't touch receiver object after this */
         return CL_RETVAL_OK;
      }
      elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
   }

   /* unlock handle connection list */
   cl_raw_list_unlock(handle->connection_list);

   CL_LOG(CL_LOG_WARNING,"can't find connection");
   free(unique_hostname); /* don't touch receiver object after this */
   return CL_RETVAL_CONNECTION_NOT_FOUND;
#endif
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_trigger_thread()"
/* This thread is used for commlib specific calls (e.g. hostlist refresh) */
static void *cl_com_trigger_thread(void *t_conf) {
   int ret_val = CL_RETVAL_OK;
   int do_exit = 0;
   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );
   pthread_cleanup_push((void *) cl_com_trigger_thread_cleanup, (void*) NULL);


   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   ret_val = CL_RETVAL_OK;
   if (ret_val != CL_RETVAL_OK) {
      do_exit = 1;
   }

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      CL_LOG(CL_LOG_INFO,"test cancel ...");
      cl_thread_func_testcancel(thread_config);

      CL_LOG(CL_LOG_INFO,"trigger host list refresh ...");
      cl_com_host_list_refresh(cl_com_get_host_list());
      cl_com_endpoint_list_refresh(cl_com_get_endpoint_list());

      CL_LOG(CL_LOG_INFO,"wait for event ...");
      if ((ret_val = cl_thread_wait_for_event(thread_config,1,0 )) != CL_RETVAL_OK) {  /* nothing to do */
         switch(ret_val) {
            case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
               CL_LOG(CL_LOG_INFO,"condition wait timeout");
               break;
            default:
               CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
               do_exit = 1;
         }
      }
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");
   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(1); /*  cl_com_trigger_thread_cleanup() */
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_trigger_thread_cleanup()"
void cl_com_trigger_thread_cleanup(void* none) {
   CL_LOG(CL_LOG_INFO,"trigger thread cleanup ...");
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_handle_service_thread()"
static void *cl_com_handle_service_thread(void *t_conf) {
   int ret_val = CL_RETVAL_OK;
   int do_exit = 0;
   int wait_for_events = 1;
   int select_sec_timeout = 0;
   int select_usec_timeout = 100*1000;


   cl_handle_list_elem_t* handle_elem = NULL;
   cl_com_handle_t* handle = NULL;

   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );
   pthread_cleanup_push((void *) cl_com_handle_service_thread_cleanup, (void*) NULL);


   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      wait_for_events = 1;

      CL_LOG(CL_LOG_INFO,"test cancel ...");
      cl_thread_func_testcancel(thread_config);
 
      if (handle == NULL) {
         cl_raw_list_lock(cl_com_handle_list);
         handle_elem = cl_handle_list_get_first_elem(cl_com_handle_list);
         while(handle_elem && handle == NULL ) {
            if (handle_elem->handle->service_thread == thread_config) {
               handle = handle_elem->handle;
               select_sec_timeout = handle->select_sec_timeout; 
               select_usec_timeout = handle->select_usec_timeout;
            }
            handle_elem = cl_handle_list_get_next_elem(cl_com_handle_list,handle_elem );
         }
         cl_raw_list_unlock(cl_com_handle_list);
      } else {
         cl_commlib_calculate_statistic(handle,1);
      }
      if (wait_for_events != 0) {
         CL_LOG(CL_LOG_INFO,"wait for event ...");
         if ((ret_val = cl_thread_wait_for_event(thread_config,select_sec_timeout,select_usec_timeout )) != CL_RETVAL_OK) {  /* nothing to do */
            switch(ret_val) {
               case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
                  CL_LOG(CL_LOG_INFO,"condition wait timeout");
                  break;
               default:
                  CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
                  do_exit = 1;
            }
         }
         cl_thread_clear_events(thread_config);
      }
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");

   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(1); /*  cl_com_trigger_thread_cleanup() */
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_handle_service_thread_cleanup()"
void cl_com_handle_service_thread_cleanup(void* none) {
   CL_LOG(CL_LOG_INFO,"handle service thread cleanup ...");
}




#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_handle_read_thread()"
static void *cl_com_handle_read_thread(void *t_conf) {
   int ret_val = CL_RETVAL_OK;
   int return_value;
   int do_exit = 0;
   int wait_for_events = 1;
   int select_sec_timeout = 0;
   int select_usec_timeout = 100*1000;
   int message_received = 0;
   int trigger_write_thread = 0;
   cl_connection_list_elem_t* elem = NULL;
   struct timeval now;
   cl_com_connection_t* the_handler = NULL;


   cl_handle_list_elem_t* handle_elem = NULL;
   cl_com_handle_t* handle = NULL;

   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );
   pthread_cleanup_push((void *) cl_com_handle_read_thread_cleanup, (void*) NULL);


   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      wait_for_events = 1;
      trigger_write_thread = 0;
      message_received = 0;


      CL_LOG(CL_LOG_INFO,"test cancel ...");
      cl_thread_func_testcancel(thread_config);
 
      if (handle == NULL) {
         cl_raw_list_lock(cl_com_handle_list);
         handle_elem = cl_handle_list_get_first_elem(cl_com_handle_list);
         while(handle_elem && handle == NULL ) {
            if (handle_elem->handle->read_thread == thread_config) {
               handle = handle_elem->handle;
               select_sec_timeout = handle->select_sec_timeout;
               select_usec_timeout = handle->select_usec_timeout;
               wait_for_events = 0;
            }
            handle_elem = cl_handle_list_get_next_elem(cl_com_handle_list,handle_elem );
         }
         cl_raw_list_unlock(cl_com_handle_list);
      } else {
         unsigned long messages_ready_before_read = 0;

         
         /* check number of connections */
         cl_commlib_check_connection_count(handle);

         CL_LOG(CL_LOG_INFO,"find connections to close ...");
         cl_connection_list_destroy_connections_to_close(handle->connection_list,1); /* OK */

         if (handle->do_shutdown == 0 && handle->max_connection_count_reached == 0) {
            the_handler = handle->service_handler;
         } else {
            the_handler = NULL;
         }
         ret_val = cl_com_open_connection_request_handler(handle->framework, 
                                                          handle->connection_list, 
                                                          the_handler,
                                                          handle->select_sec_timeout ,
                                                          handle->select_usec_timeout,
                                                          CL_R_SELECT);
      
         switch (ret_val) {
            case CL_RETVAL_SELECT_TIMEOUT:
               CL_LOG(CL_LOG_INFO,"got select timeout");
               wait_for_events = 0;
               break;
            case CL_RETVAL_SELECT_INTERRUPT:
               CL_LOG(CL_LOG_WARNING,"got select interrupt");
               wait_for_events = 0;
               break;
            case CL_RETVAL_NO_SELECT_DESCRIPTORS:
               CL_LOG(CL_LOG_INFO,"no select descriptors");
               wait_for_events = 1;
               break;
            case CL_RETVAL_OK:
               /* do not wait for events, because we do select */
               wait_for_events = 0;
               break;
            default:
               CL_LOG_STR(CL_LOG_ERROR,"got error:",cl_get_error_text(ret_val));
               break;
         }

         /* check for new connections */
         if (handle->service_provider                != 0                 && 
             handle->do_shutdown                     == 0                 && 
             handle->service_handler->data_read_flag == CL_COM_DATA_READY &&
             handle->max_connection_count_reached    == 0) {

            /* we have a new connection request */
            cl_com_connection_t* new_con = NULL;
            struct timeval now;

            cl_com_connection_request_handler(handle->service_handler, &new_con, 0,0 );
            if (new_con != NULL) {
               /* got new connection request */
               new_con->handler = handle->service_handler->handler;
               CL_LOG(CL_LOG_INFO,"adding new client");
               gettimeofday(&now,NULL);
               new_con->read_buffer_timeout_time = now.tv_sec + handle->open_connection_timeout;
               cl_connection_list_append_connection(handle->connection_list, new_con,1);
               handle->statistic->new_connections = handle->statistic->new_connections + 1;
               new_con = NULL;
            }
         }

         pthread_mutex_lock(handle->messages_ready_mutex);
         messages_ready_before_read = handle->messages_ready_for_read;
         pthread_mutex_unlock(handle->messages_ready_mutex);

         cl_raw_list_lock(handle->connection_list);
         /* read messages */
         elem = cl_connection_list_get_first_elem(handle->connection_list);     
         gettimeofday(&now,NULL);
         
         while(elem) {
            if (elem->connection->connection_state == CL_COM_DISCONNECTED) {
               /* open connection if there are messages to send */
               if ( cl_raw_list_get_elem_count(elem->connection->send_message_list) > 0) {
                  CL_LOG(CL_LOG_INFO,"setting connection state to CL_COM_OPENING");
                  elem->connection->connection_state = CL_COM_OPENING;
               }
            }

            if (elem->connection->connection_state == CL_COM_OPENING) {
               /* trigger connect */
               CL_LOG(CL_LOG_INFO,"OPENING...");
               return_value = cl_com_open_connection(elem->connection, handle->open_connection_timeout,NULL , NULL, NULL, NULL);
               if (return_value != CL_RETVAL_OK && return_value != CL_RETVAL_UNCOMPLETE_WRITE ) {
                  CL_LOG_STR(CL_LOG_ERROR,"could not open connection:",cl_get_error_text(return_value));
                  elem->connection->connection_state = CL_COM_CLOSING;
               }
            }

            if (elem->connection->connection_state == CL_COM_CONNECTING) {
               if ( elem->connection->data_read_flag == CL_COM_DATA_READY  ) {
                  return_value = cl_com_tcp_connection_complete_request(elem->connection,handle->open_connection_timeout,1);

                  if (return_value != CL_RETVAL_OK && 
                      return_value != CL_RETVAL_UNCOMPLETE_READ && 
                      return_value != CL_RETVAL_UNCOMPLETE_WRITE && 
                      return_value != CL_RETVAL_SELECT_ERROR ) {
                     CL_LOG_STR(CL_LOG_ERROR,"connection establish error:",cl_get_error_text(return_value));
                     elem->connection->connection_state = CL_COM_CLOSING;
                  }
                  if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                     elem->connection->connection_state = CL_COM_CLOSING;
                  } 
                  if ( elem->connection->connection_state == CL_COM_CONNECTED ) {
                     cl_commlib_finish_request_completeness(elem->connection);
                     /* connection is now in connect state, do select before next reading */
                     elem->connection->data_read_flag = CL_COM_DATA_NOT_READY;
                  }
               } else {
                  /* check timeouts */
                  if ( elem->connection->read_buffer_timeout_time != 0) {
                     if ( now.tv_sec >= elem->connection->read_buffer_timeout_time ) {
                        CL_LOG(CL_LOG_ERROR,"read timeout for connection completion");
                        elem->connection->connection_state = CL_COM_CLOSING;
                     }
                  }
                  if ( elem->connection->write_buffer_timeout_time != 0) {
                     if ( now.tv_sec >= elem->connection->write_buffer_timeout_time ) {
                        CL_LOG(CL_LOG_ERROR,"write timeout for connection completion");
                        elem->connection->connection_state = CL_COM_CLOSING;
                     }
                  }
               }
            }
            
            if (elem->connection->connection_state == CL_COM_CONNECTED) {

               /* check ack timeouts */
               return_value = cl_commlib_handle_connection_ack_timeouts(elem->connection);

               if (elem->connection->data_read_flag == CL_COM_DATA_READY && 
                   elem->connection->ccrm_sent      == 0 && 
                   elem->connection->ccrm_received  == 0) {
                  CL_LOG(CL_LOG_INFO,"reading ...");
                  /* TODO: use read thread pool */
                  return_value = cl_commlib_handle_connection_read(elem->connection);
                  if ( return_value != CL_RETVAL_OK && 
                       return_value != CL_RETVAL_UNCOMPLETE_READ && 
                       return_value != CL_RETVAL_SELECT_ERROR ) {
                     elem->connection->connection_state = CL_COM_CLOSING;
                     CL_LOG_STR(CL_LOG_WARNING,"read from connection: setting close flag! Reason:", cl_get_error_text(return_value));
                  }
                  if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                     elem->connection->connection_state = CL_COM_CLOSING;
                  }
                  message_received = 1;
               } else {
                  /* check timeouts */
                  if ( elem->connection->read_buffer_timeout_time != 0) {
                     if ( now.tv_sec >= elem->connection->read_buffer_timeout_time ) {
                        CL_LOG(CL_LOG_ERROR,"connection read timeout");
                        elem->connection->connection_state = CL_COM_CLOSING;
                     }
                  }
               }
               if (elem->connection->ccm_received == 1 ) {
                  CL_LOG(CL_LOG_INFO,"received ccm");
      
                  CL_LOG_INT(CL_LOG_INFO,"receive buffer:",cl_raw_list_get_elem_count(elem->connection->received_message_list) );
                  CL_LOG_INT(CL_LOG_INFO,"send buffer   :",cl_raw_list_get_elem_count(elem->connection->send_message_list) );
      
                  if( cl_raw_list_get_elem_count(elem->connection->send_message_list) == 0 && 
                      cl_raw_list_get_elem_count(elem->connection->received_message_list) == 0) {
                     elem->connection->ccm_received = 2;
                     elem->connection->connection_sub_state = CL_COM_SENDING_CCRM;
                     cl_commlib_send_ccrm_message(elem->connection);
                     CL_LOG(CL_LOG_WARNING,"sending ccrm");
                  }
               }
            }

            if (elem->connection->data_write_flag == CL_COM_DATA_READY) {
               /* there is data to write, trigger write thread */
               trigger_write_thread = 1;
            }

            elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
         }
         cl_raw_list_unlock(handle->connection_list);

         if (trigger_write_thread != 0) {
            cl_thread_trigger_event(handle->write_thread);
         }
         
         /* trigger threads which are waiting for read_condition when
            we will do a wait by itself (because we have no descriptors 
            for reading) or when we have received a message */
         if ( wait_for_events || message_received != 0 ) {
            cl_thread_trigger_thread_condition(handle->read_condition,1);

            /* if we have received a message which was no protocol message
               trigger application ( cl_commlib_receive_message() ) */
            pthread_mutex_lock(handle->messages_ready_mutex);

            if ( handle->messages_ready_for_read > 0 && handle->messages_ready_for_read != messages_ready_before_read ) {
               pthread_mutex_unlock(handle->messages_ready_mutex);
               cl_thread_trigger_thread_condition(handle->app_condition,1); 
            } else {
               pthread_mutex_unlock(handle->messages_ready_mutex);
            }
         } 
      }

      if (wait_for_events != 0) {
         if ((ret_val = cl_thread_wait_for_event(thread_config,select_sec_timeout,select_usec_timeout )) != CL_RETVAL_OK) {
            switch(ret_val) {
               case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
                  CL_LOG(CL_LOG_INFO,"READ THREAD GOT CONDITION WAIT TIMEOUT");
                  break;
               default:
                  CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
                  do_exit = 1;
            }
         }
         /* cleanup all trigger events */
         cl_thread_clear_events(thread_config);
      }
   }

   CL_LOG(CL_LOG_INFO, "exiting ...");

   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(1); /*  cl_com_trigger_thread_cleanup() */
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}

#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_handle_read_thread_cleanup()"
void cl_com_handle_read_thread_cleanup(void* none) {
   CL_LOG(CL_LOG_INFO,"handle read thread cleanup ...");
}





#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "cl_com_handle_write_thread()"
static void *cl_com_handle_write_thread(void *t_conf) {
   int return_value = CL_RETVAL_OK;
   int ret_val = CL_RETVAL_OK;
   int do_exit = 0;
   int wait_for_events = 1;
   int select_sec_timeout = 0;
   int select_usec_timeout = 100*1000;
   cl_connection_list_elem_t* elem = NULL;
   struct timeval now;
   cl_handle_list_elem_t* handle_elem = NULL;
   cl_com_handle_t* handle = NULL;
   int trigger_read_thread = 0;

   /* get pointer to cl_thread_settings_t struct */
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)t_conf; 
   /* push default cleanup function */
   pthread_cleanup_push((void *) cl_thread_default_cleanup_function, (void*) thread_config );
   pthread_cleanup_push((void *) cl_com_handle_write_thread_cleanup, (void*) NULL);


   /* set thread config data */
   if (cl_thread_set_thread_config(thread_config) != CL_RETVAL_OK) {
      CL_LOG(CL_LOG_ERROR,"thread setup error"); 
      do_exit = 1;
   }

   /* setup thread */
   CL_LOG(CL_LOG_INFO, "starting initialization ...");

   /* thread init done, trigger startup conditon variable*/
   cl_thread_func_startup(thread_config);
   CL_LOG(CL_LOG_INFO, "starting main loop ...");

   /* ok, thread main */
   while (do_exit == 0) {
      CL_LOG(CL_LOG_INFO,"test cancel ...");
      trigger_read_thread = 0;
      cl_thread_func_testcancel(thread_config);
 
      if (handle == NULL) {
         cl_raw_list_lock(cl_com_handle_list);
         handle_elem = cl_handle_list_get_first_elem(cl_com_handle_list);
         while(handle_elem && handle == NULL ) {
            if (handle_elem->handle->write_thread == thread_config) {
               handle = handle_elem->handle;
               select_sec_timeout = handle->select_sec_timeout; 
               select_usec_timeout = handle->select_usec_timeout;
               wait_for_events = 0;
            }
            handle_elem = cl_handle_list_get_next_elem(cl_com_handle_list,handle_elem );
         }
         cl_raw_list_unlock(cl_com_handle_list);
         
      } else {

         /* do write select */
         ret_val = cl_com_open_connection_request_handler(handle->framework, 
                                                          handle->connection_list, 
                                                          handle->service_handler,
                                                          handle->select_sec_timeout ,
                                                          handle->select_usec_timeout,
                                                          CL_W_SELECT  );
         switch (ret_val) {
            case CL_RETVAL_SELECT_TIMEOUT:
               CL_LOG(CL_LOG_INFO,"select timeout");
               wait_for_events = 0;
               break;
            case CL_RETVAL_SELECT_INTERRUPT:
               CL_LOG(CL_LOG_WARNING,"select interrupt");
               wait_for_events = 0;
               break;
            case CL_RETVAL_NO_SELECT_DESCRIPTORS:
               CL_LOG(CL_LOG_INFO,"no descriptors to select");
               wait_for_events = 1;
               break;
            case CL_RETVAL_OK: 
               wait_for_events = 1;
               cl_raw_list_lock(handle->connection_list);
               /* read messages */
               elem = cl_connection_list_get_first_elem(handle->connection_list);     
               gettimeofday(&now,NULL);
               while(elem) {
                  if (elem->connection->connection_state == CL_COM_CONNECTING) {
                     if ( elem->connection->fd_ready_for_write == CL_COM_DATA_READY ) {
                        return_value = cl_com_tcp_connection_complete_request(elem->connection,handle->open_connection_timeout,1);
                        if (return_value != CL_RETVAL_OK && 
                            return_value != CL_RETVAL_UNCOMPLETE_READ && 
                            return_value != CL_RETVAL_UNCOMPLETE_WRITE && 
                            return_value != CL_RETVAL_SELECT_ERROR ) {
                           CL_LOG_STR(CL_LOG_ERROR,"connection establish error:",cl_get_error_text(return_value));
                           elem->connection->connection_state = CL_COM_CLOSING;
                        }
                        if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                           elem->connection->connection_state = CL_COM_CLOSING;
                        }
                        if ( elem->connection->connection_state == CL_COM_CONNECTED ) {
                           cl_commlib_finish_request_completeness(elem->connection);
                           /* connection is now in connect state, do select before next reading */
                           elem->connection->fd_ready_for_write = CL_COM_DATA_NOT_READY;
                        }
                     } else {
                        /* check timeouts */
                        if ( elem->connection->read_buffer_timeout_time != 0) {
                           if ( now.tv_sec >= elem->connection->read_buffer_timeout_time ) {
                              CL_LOG(CL_LOG_ERROR,"read timeout for connection completion");
                              elem->connection->connection_state = CL_COM_CLOSING;
                           }
                        }
                        if ( elem->connection->write_buffer_timeout_time != 0) {
                           if ( now.tv_sec >= elem->connection->write_buffer_timeout_time ) {
                              CL_LOG(CL_LOG_ERROR,"write timeout for connection completion");
                              elem->connection->connection_state = CL_COM_CLOSING;
                           }
                        }
                     }
                  }
                  
                  if (elem->connection->connection_state == CL_COM_CONNECTED) {
                     if (elem->connection->fd_ready_for_write == CL_COM_DATA_READY && elem->connection->ccrm_sent == 0 ) {
                        CL_LOG(CL_LOG_INFO,"writing ...");
                        /* TODO implement thread pool for data writing */
                        return_value = cl_commlib_handle_connection_write(elem->connection);
                        if (return_value != CL_RETVAL_OK && 
                            return_value != CL_RETVAL_UNCOMPLETE_WRITE && 
                            return_value != CL_RETVAL_SELECT_ERROR ) {
                           elem->connection->connection_state = CL_COM_CLOSING;
                           CL_LOG_STR(CL_LOG_ERROR,"write to connection: setting close flag! Reason:", cl_get_error_text(return_value));
                        }
                        if (return_value != CL_RETVAL_OK && cl_com_get_ignore_timeouts_flag() == CL_TRUE) {
                           elem->connection->connection_state = CL_COM_CLOSING;
                        }
                     } else {
                        /* check timeouts */
                        if ( elem->connection->write_buffer_timeout_time != 0) {
                           if ( now.tv_sec >= elem->connection->write_buffer_timeout_time ) {
                              CL_LOG(CL_LOG_ERROR,"write timeout for connection completion");
                              elem->connection->connection_state = CL_COM_CLOSING;
                           }
                        }
                     }
                  }
                  if (elem->connection->ccm_received == 1 ) {
                     CL_LOG(CL_LOG_INFO,"received ccm");
                     
                     CL_LOG_INT(CL_LOG_INFO,"receive buffer:",cl_raw_list_get_elem_count(elem->connection->received_message_list) );
                     CL_LOG_INT(CL_LOG_INFO,"send buffer   :",cl_raw_list_get_elem_count(elem->connection->send_message_list) );
            
                     if( cl_raw_list_get_elem_count(elem->connection->send_message_list)     == 0 && 
                         cl_raw_list_get_elem_count(elem->connection->received_message_list) == 0)   {

                           elem->connection->ccm_received = 2;
                           elem->connection->connection_sub_state = CL_COM_SENDING_CCRM;
                           cl_commlib_send_ccrm_message(elem->connection);
                           CL_LOG(CL_LOG_INFO,"sending ccrm");
                     }
                  }
                  if (elem->connection->ccrm_sent != 0) {
                     CL_LOG(CL_LOG_WARNING,"this connection is going down!");
                     trigger_read_thread = 1;
                  }
                  if (elem->connection->data_write_flag == CL_COM_DATA_READY) {
                     /* still data to write, do not wait for events */
                     wait_for_events = 0;
                  }
                  elem = cl_connection_list_get_next_elem(handle->connection_list, elem);
               }
               cl_raw_list_unlock(handle->connection_list);
               if (trigger_read_thread != 0) {
                  CL_LOG(CL_LOG_WARNING,"triggering read thread");
                  cl_thread_trigger_event(handle->read_thread);
               }
               break;
            default:
               wait_for_events = 1;
               CL_LOG_STR(CL_LOG_ERROR,"got error:",cl_get_error_text(ret_val));
               break;
         }  /* switch */
      }
      if (wait_for_events != 0 ) {
         if ((ret_val = cl_thread_wait_for_event(thread_config, select_sec_timeout, select_usec_timeout )) != CL_RETVAL_OK) {
            switch(ret_val) {
               case CL_RETVAL_CONDITION_WAIT_TIMEOUT:
                  CL_LOG(CL_LOG_INFO,"WRITE THREAD GOT CONDITION WAIT TIMEOUT");
                  break;
               default:
                  CL_LOG_STR( CL_LOG_INFO, ">got error<: ", cl_get_error_text(ret_val));
                  do_exit = 1;
            }
         }
         /* set event count to zero, because this thread will
            not wait when something is to do */
         cl_thread_clear_events(thread_config);
      }
   }
   CL_LOG(CL_LOG_INFO, "exiting ...");

   /* at least set exit state */
   cl_thread_func_cleanup(thread_config);  
   pthread_cleanup_pop(1); /*  cl_com_trigger_thread_cleanup() */
   pthread_cleanup_pop(0); /*  cl_thread_default_cleanup_function() */
   return(NULL);
}

void cl_com_handle_write_thread_cleanup(void* none) {
   CL_LOG(CL_LOG_INFO,"handle write thread cleanup ...");
}


#ifdef __CL_FUNCTION__
#undef __CL_FUNCTION__
#endif
#define __CL_FUNCTION__ "getuniquehostname()"
/* MT-NOTE: getuniquehostname() is MT safe */
int getuniquehostname(const char *hostin, char *hostout, int refresh_aliases) {
   char* resolved_host = NULL;
   int ret_val;

   if (refresh_aliases != 0) {
      /* TODO: refresh host alias file? But it's never used */
      CL_LOG(CL_LOG_ERROR,"getuniquehostname() refresh of alias file not implemented");
   }
   ret_val = cl_com_cached_gethostbyname((char*)hostin, &resolved_host, NULL, NULL, NULL );
   if (resolved_host != NULL) {
      if (strlen(resolved_host) > MAXHOSTLEN ) {
         free(resolved_host);
         return CL_RETVAL_UNKNOWN;
      }
      snprintf(hostout, MAXHOSTLEN, resolved_host );
      free(resolved_host);
   }
   return ret_val;
}

