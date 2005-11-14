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
#include <sys/types.h>
#include <string.h>

#include "setup.h"
#include "sge_gdiP.h"
#include "sge_any_request.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_profiling.h"
#include "qm_name.h"
#include "pack.h"
#include "sge_feature.h"
#include "sge_security.h"
#include "sge_unistd.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_bootstrap.h"
#include "sge_mtutil.h"
#include "msg_gdilib.h"
#include "sgeobj/sge_answer.h"

static pthread_mutex_t check_alive_mutex = PTHREAD_MUTEX_INITIALIZER;

static int gdi_log_flush_func(cl_raw_list_t* list_p);

/* setup a communication error callback and mutex for it */
static pthread_mutex_t general_communication_error_mutex = PTHREAD_MUTEX_INITIALIZER;
static void general_communication_error(const cl_application_error_list_elem_t* commlib_error);
/* local static struct to store communication errors. The boolean
 * values com_access_denied and com_endpoint_not_unique will never be
 * restored to false again 
 */
typedef struct sge_gdi_com_error_type {
   int  com_error;               /* current commlib error */
   int  com_last_error;          /* last logged commlib error */
   bool com_access_denied;       /* set when commlib reports CL_RETVAL_ACCESS_DENIED */
   bool com_endpoint_not_unique; /* set when commlib reports CL_RETVAL_ENDPOINT_NOT_UNIQUE */
} sge_gdi_com_error_t;
static sge_gdi_com_error_t sge_gdi_communication_error = {CL_RETVAL_OK,
                                                          CL_RETVAL_OK,
                                                          false,
                                                          false};


#ifdef DEBUG_CLIENT_SUPPORT
static void gdi_rmon_print_callback_function(const char *message, unsigned long traceid, unsigned long pid, unsigned long thread_id);
#endif

#ifdef DEBUG_CLIENT_SUPPORT
static void gdi_rmon_print_callback_function(const char *message, unsigned long traceid, unsigned long pid, unsigned long thread_id) {
   cl_com_handle_t* handle = NULL;

   handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   if (handle != NULL) {
      cl_com_application_debug(handle, message);
   }
}
#endif

/****** sge_any_request/sge_dump_message_tag() *************************************
*  NAME
*     sge_dump_message_tag() -- get tag name string
*
*  SYNOPSIS
*     const char* sge_dump_message_tag(int tag) 
*
*  FUNCTION
*     This is a function used for getting a printable string output for the
*     different message tags.
*     (Useful for debugging)
*
*  INPUTS
*     int tag - tag value
*
*  RESULT
*     const char* - name of tag
*
*  NOTES
*     MT-NOTE: sge_dump_message_tag() is MT safe 
*******************************************************************************/
const char* sge_dump_message_tag(unsigned long tag) {
   switch (tag) {
      case TAG_NONE:
         return "TAG_NONE";
      case TAG_OLD_REQUEST:
         return "TAG_OLD_REQUEST";
      case TAG_GDI_REQUEST:
         return "TAG_GDI_REQUEST";
      case TAG_ACK_REQUEST:
         return "TAG_ACK_REQUEST";
      case TAG_REPORT_REQUEST:
         return "TAG_REPORT_REQUEST";
      case TAG_FINISH_REQUEST:
         return "TAG_FINISH_REQUEST";
      case TAG_JOB_EXECUTION:
         return "TAG_JOB_EXECUTION";
      case TAG_SLAVE_ALLOW:
         return "TAG_SLAVE_ALLOW";
      case TAG_CHANGE_TICKET:
         return "TAG_CHANGE_TICKET";
      case TAG_SIGJOB:
         return "TAG_SIGJOB";
      case TAG_SIGQUEUE:
         return "TAG_SIGQUEUE";
      case TAG_KILL_EXECD:
         return "TAG_KILL_EXECD";
      case TAG_NEW_FEATURES:
         return "TAG_NEW_FEATURES";
      case TAG_GET_NEW_CONF:
         return "TAG_GET_NEW_CONF";
      case TAG_JOB_REPORT:
         return "TAG_JOB_REPORT";
      case TAG_QSTD_QSTAT:
         return "TAG_QSTD_QSTAT";
      case TAG_TASK_EXIT:
         return "TAG_TASK_EXIT";
      case TAG_TASK_TID:
         return "TAG_TASK_TID";
      case TAG_EVENT_CLIENT_EXIT:
         return "TAG_EVENT_CLIENT_EXIT";
      case TAG_SEC_ANNOUNCE:
         return "TAG_SEC_ANNOUNCE";
      case TAG_SEC_RESPOND:
         return "TAG_SEC_RESPOND";
      case TAG_SEC_ERROR:
         return "TAG_SEC_ERROR";
      default:
         break;
   }
   return "TAG_NOT_DEFINED";
}

static int gdi_log_flush_func(cl_raw_list_t* list_p) {
   int ret_val;
   cl_log_list_elem_t* elem = NULL;
   DENTER(COMMD_LAYER, "gdi_log_flush_func");

   if (list_p == NULL) {
      DEXIT;
      return CL_RETVAL_LOG_NO_LOGLIST;
   }

   if (  ( ret_val = cl_raw_list_lock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   }

   while ( (elem = cl_log_list_get_first_elem(list_p) ) != NULL) {
      char* param;
      if (elem->log_parameter == NULL) {
         param = "";
      } else {
         param = elem->log_parameter;
      }

      switch(elem->log_type) {
         case CL_LOG_ERROR: 
            if ( log_state_get_log_level() >= LOG_ERR) {
               ERROR((SGE_EVENT,  "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_WARNING:
            if ( log_state_get_log_level() >= LOG_WARNING) {
               WARNING((SGE_EVENT,"%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_INFO:
            if ( log_state_get_log_level() >= LOG_INFO) {
               INFO((SGE_EVENT,   "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_DEBUG:
            if ( log_state_get_log_level() >= LOG_DEBUG) { 
               DEBUG((SGE_EVENT,  "%s %-20s=> %s %s", elem->log_module_name, elem->log_thread_name, elem->log_message, param ));
            } else {
               printf("%s %-20s=> %s %s\n", elem->log_module_name, elem->log_thread_name, elem->log_message, param);
            }
            break;
         case CL_LOG_OFF:
            break;
      }
      cl_log_list_del_log(list_p);
   }
   
   if (  ( ret_val = cl_raw_list_unlock(list_p)) != CL_RETVAL_OK) {
      DEXIT;
      return ret_val;
   } 
   DEXIT;
   return CL_RETVAL_OK;
}

/****** sge_any_request/general_communication_error() **************************
*  NAME
*     general_communication_error() -- callback for communication errors
*
*  SYNOPSIS
*     static void general_communication_error(int cl_error, 
*                                             const char* error_message) 
*
*  FUNCTION
*     This function is used by cl_com_set_error_func() to set the default
*     application error function for communication errors. On important 
*     communication errors the communication lib will call this function
*     with a corresponding error number (within application context).
*
*     This function should never block. Treat it as a kind of signal handler.
*    
*     The error_message parameter is freed by the commlib.
*
*  INPUTS
*     int cl_error              - commlib error number
*     const char* error_message - additional error text message
*
*  NOTES
*     MT-NOTE: general_communication_error() is MT safe 
*     (static struct variable "sge_gdi_communication_error" is used)
*
*
*  SEE ALSO
*     sge_any_request/sge_get_com_error_flag()
*******************************************************************************/
static void general_communication_error(const cl_application_error_list_elem_t* commlib_error) {
   DENTER(TOP_LAYER, "general_communication_error");
   if (commlib_error != NULL) {

      sge_mutex_lock("general_communication_error_mutex",
                     SGE_FUNC, __LINE__, &general_communication_error_mutex);  

      /* save the communication error to react later */
      sge_gdi_communication_error.com_error = commlib_error->cl_error;

      switch (commlib_error->cl_error) {
         case CL_RETVAL_ACCESS_DENIED: {
            sge_gdi_communication_error.com_access_denied = true;
            break;
         }
         case CL_RETVAL_ENDPOINT_NOT_UNIQUE: {
            sge_gdi_communication_error.com_endpoint_not_unique = true;
            break;
         }
         default: {
            break;
         }
      }


      /*
       * now log the error if not already reported the 
       * least CL_DEFINE_MESSAGE_DUP_LOG_TIMEOUT seconds
       */
      if (commlib_error->cl_already_logged == CL_FALSE && 
         sge_gdi_communication_error.com_last_error != sge_gdi_communication_error.com_error) {

         /*  never log the same messages again and again (commlib
          *  will erase cl_already_logged flag every CL_DEFINE_MESSAGE_DUP_LOG_TIMEOUT
          *  seconds (30 seconds), so we have to save the last one!
          */
         sge_gdi_communication_error.com_last_error = sge_gdi_communication_error.com_error;

         switch (commlib_error->cl_err_type) {
            case CL_LOG_ERROR: {
               if (commlib_error->cl_info != NULL) {
                  ERROR((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                         cl_get_error_text(commlib_error->cl_error),
                         commlib_error->cl_info));
               } else {
                  ERROR((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                         cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_WARNING: {
               if (commlib_error->cl_info != NULL) {
                  WARNING((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                           cl_get_error_text(commlib_error->cl_error),
                           commlib_error->cl_info));
               } else {
                  WARNING((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                           cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_INFO: {
               if (commlib_error->cl_info != NULL) {
                  INFO((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                        cl_get_error_text(commlib_error->cl_error),
                        commlib_error->cl_info));
               } else {
                  INFO((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                        cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_DEBUG: {
               if (commlib_error->cl_info != NULL) {
                  DEBUG((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_SS,
                         cl_get_error_text(commlib_error->cl_error),
                         commlib_error->cl_info));
               } else {
                  DEBUG((SGE_EVENT, MSG_GDI_GENERAL_COM_ERROR_S,
                         cl_get_error_text(commlib_error->cl_error)));
               }
               break;
            }
            case CL_LOG_OFF: {
               break;
            }
         }
      }
      sge_mutex_unlock("general_communication_error_mutex", 
                       SGE_FUNC, __LINE__, &general_communication_error_mutex);  
   }
   DEXIT;
}


/****** sge_any_request/sge_get_com_error_flag() *******************************
*  NAME
*     sge_get_com_error_flag() -- return gdi error flag state
*
*  SYNOPSIS
*     bool sge_get_com_error_flag(sge_gdi_stored_com_error_t error_type) 
*
*  FUNCTION
*     This function returns the error flag for the specified error type
*
*  INPUTS
*     sge_gdi_stored_com_error_t error_type - error type value
*
*  RESULT
*     bool - true: error has occured, false: error never occured
*
*  NOTES
*     MT-NOTE: sge_get_com_error_flag() is MT safe 
*
*  SEE ALSO
*     sge_any_request/general_communication_error()
*******************************************************************************/
bool sge_get_com_error_flag(sge_gdi_stored_com_error_t error_type) {
   bool ret_val = false;
   DENTER(TOP_LAYER, "sge_get_com_error_flag");
   sge_mutex_lock("general_communication_error_mutex", 
                  SGE_FUNC, __LINE__, &general_communication_error_mutex);  

   /* 
    * never add a default case for that switch, because of compiler warnings
    * for un-"cased" values 
    */
   switch (error_type) {
      case SGE_COM_ACCESS_DENIED: {
         ret_val = sge_gdi_communication_error.com_access_denied;
         break;
      }
      case SGE_COM_ENDPOINT_NOT_UNIQUE: {
         ret_val = sge_gdi_communication_error.com_endpoint_not_unique;
         break;
      }
   }
   sge_mutex_unlock("general_communication_error_mutex",
                    SGE_FUNC, __LINE__, &general_communication_error_mutex);  

   DEXIT;
   return ret_val;
}

/*-----------------------------------------------------------------------
 * prepare_enroll
 * just store values for later enroll() of commlib
 *
 * NOTES
 *    MT-NOTE: prepare_enroll() is MT safe
 *-----------------------------------------------------------------------*/
int prepare_enroll(const char *name, int* last_commlib_error)
{
   int ret_val;
   u_long32 me_who;
   cl_com_handle_t* handle = NULL;
   cl_host_resolve_method_t resolve_method = CL_SHORT;
   cl_framework_t  communication_framework = CL_CT_TCP;
   const char* default_domain = NULL;
   const char* help = NULL;
   int commlib_error = CL_RETVAL_OK;


   DENTER(TOP_LAYER, "prepare_enroll");

   if (name == NULL) {
      CRITICAL((SGE_EVENT, MSG_GDI_NO_VALID_PROGRAMM_NAME));
      SGE_EXIT(1);
   }

   if (last_commlib_error == NULL) {
      CRITICAL((SGE_EVENT, MSG_GDI_INITCOMMLIBFAILED));
      SGE_EXIT(2);
   }
 
   /* here the we start up some daemons and clients with multithreaded
      communication library */  

   if (cl_com_setup_commlib_complete() == CL_FALSE) {
      if ( uti_state_get_mewho() == QMASTER ||
           uti_state_get_mewho() == QMON    ||
           uti_state_get_mewho() == DRMAA   ||
           uti_state_get_mewho() == SCHEDD      ) {
         INFO((SGE_EVENT,MSG_GDI_MULTI_THREADED_STARTUP));
         ret_val = cl_com_setup_commlib(CL_RW_THREAD,CL_LOG_OFF,gdi_log_flush_func);
      } else {
         INFO((SGE_EVENT,MSG_GDI_SINGLE_THREADED_STARTUP));
         ret_val = cl_com_setup_commlib(CL_NO_THREAD,CL_LOG_OFF,gdi_log_flush_func);
      }
      if (ret_val != CL_RETVAL_OK) {
         ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
         CRITICAL((SGE_EVENT, MSG_GDI_INITCOMMLIBFAILED));
         SGE_EXIT(1);
      }
   }
 
   /* set alias file */
   {
      char *alias_path = sge_get_alias_path();
      ret_val = cl_com_set_alias_file(alias_path);
      if (ret_val != CL_RETVAL_OK && ret_val != *last_commlib_error) {
         ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
      }
      FREE(alias_path);
   }

   /* set hostname resolve (compare) method */
   if (bootstrap_get_ignore_fqdn() == false) {
      resolve_method = CL_LONG;
   }
   if ( (help=bootstrap_get_default_domain()) != NULL) {
      if (SGE_STRCASECMP(help, NONE_STR) != 0) {
         default_domain = help;
      }
   }
   ret_val = cl_com_set_resolve_method(resolve_method, (char*)default_domain);

   if (ret_val != CL_RETVAL_OK && ret_val != *last_commlib_error) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }

   /* reresolve qualified hostname with use of host aliases */
   reresolve_me_qualified_hostname();

   /* set error function */
   ret_val = cl_com_set_error_func(general_communication_error);
   if (ret_val != CL_RETVAL_OK && ret_val != *last_commlib_error) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }

   /* set tag name function */
   ret_val = cl_com_set_tag_name_func(sge_dump_message_tag);
   if (ret_val != CL_RETVAL_OK && ret_val != *last_commlib_error) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }
#ifdef DEBUG_CLIENT_SUPPORT
   /* set debug client callback function to rmon's debug client callback */
   ret_val = cl_com_set_application_debug_client_callback_func(rmon_debug_client_callback);
   if (ret_val != CL_RETVAL_OK && ret_val != *last_commlib_error) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }
#endif

   /* init sge security module */   
   if (sge_security_initialize(name)) {
      CRITICAL((SGE_EVENT, MSG_GDI_INITSECURITYDATAFAILED));
      SGE_EXIT(1);
   }

   /* set communication framework to SSL when CSP security is enabled */
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      DPRINTF(("using communication lib with SSL framework\n"));
      communication_framework = CL_CT_SSL;
   }

   /* OK, now we can create communication handles ... */
   me_who = uti_state_get_mewho();

   handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   if (handle == NULL) {
      int my_component_id = 0; /* 1 for daemons, 0=automatical for clients */
      int execd_port = 0;
      if ( me_who == QMASTER ||
           me_who == EXECD   ||
           me_who == SCHEDD  || 
           me_who == SHADOWD ) {
         my_component_id = 1;   
      }

      switch(me_who) {
         case EXECD:
            /* add qmaster as known endpoint */
            DPRINTF(("re-read actual qmaster file (prepare_enroll)\n"));
            cl_com_append_known_endpoint_from_name((char*)sge_get_master(true), 
                                                   (char*) prognames[QMASTER],
                                                   1 ,
                                                   sge_get_qmaster_port(),
                                                   CL_CM_AC_DISABLED ,
                                                   CL_TRUE);
            execd_port = sge_get_execd_port(); 
            handle = cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE, CL_TRUE,execd_port , CL_TCP_DEFAULT,
                                          (char*)prognames[uti_state_get_mewho()], my_component_id , 1 , 0 );
            cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED );
            if (handle == NULL) {
               if (commlib_error != *last_commlib_error) {
                  ERROR((SGE_EVENT, MSG_GDI_CANT_GET_COM_HANDLE_SSUUS, 
                                    uti_state_get_qualified_hostname(),
                                    (char*) prognames[uti_state_get_mewho()],
                                    sge_u32c(my_component_id), 
                                    sge_u32c(execd_port),
                                    cl_get_error_text(commlib_error)));
               }
            }
            break;
         case QMASTER:
            DPRINTF(("creating QMASTER handle\n"));
            cl_com_append_known_endpoint_from_name((char*)sge_get_master(true), 
                                                   (char*) prognames[QMASTER],
                                                   1 ,
                                                   sge_get_qmaster_port(),
                                                   CL_CM_AC_DISABLED ,
                                                   CL_TRUE);

            handle = cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE,              /* message based tcp communication                */
                                          CL_TRUE, sge_get_qmaster_port(),                          /* create service on qmaster port,                */
                                          CL_TCP_DEFAULT,                                           /* use standard connect mode         */
                                          (char*)prognames[uti_state_get_mewho()], my_component_id, /* this endpoint is called "qmaster" and has id 1 */
                                          1 , 0 );           
                                       /* select timeout is set to 1 second 0 usec       */
            if (handle == NULL) {
               if (commlib_error != *last_commlib_error) {   
                     ERROR((SGE_EVENT, MSG_GDI_CANT_GET_COM_HANDLE_SSUUS, 
                                          uti_state_get_qualified_hostname(),
                                          (char*) prognames[uti_state_get_mewho()],
                                          sge_u32c(my_component_id), 
                                          sge_u32c(sge_get_qmaster_port()),
                                          cl_get_error_text(commlib_error)));
               }
            } else {
               int alive_back = 0;
               char act_resolved_qmaster_name[CL_MAXHOSTLEN]; 
               cl_com_set_synchron_receive_timeout(handle, 5);

               if (sge_get_master(1) != NULL) {
                  /* check a running qmaster on different host */
                  if (getuniquehostname(sge_get_master(0), act_resolved_qmaster_name, 0) == CL_RETVAL_OK &&
                      sge_hostcmp(act_resolved_qmaster_name, uti_state_get_qualified_hostname()) != 0) {
                     DPRINTF(("act_qmaster file contains host "SFQ" which doesn't match local host name "SFQ"\n",
                              sge_get_master(0), uti_state_get_qualified_hostname()  ));

                     cl_com_set_error_func(NULL);

                     alive_back = check_isalive(sge_get_master(0));

                     ret_val = cl_com_set_error_func(general_communication_error);
                     if (ret_val != CL_RETVAL_OK) {
                        ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
                     }

                     if (alive_back == CL_RETVAL_OK && getenv("SGE_TEST_HEARTBEAT_TIMEOUT") == NULL ) {
                        CRITICAL((SGE_EVENT, MSG_GDI_MASTER_ON_HOST_X_RUNINNG_TERMINATE_S, sge_get_master(0) ));
                        SGE_EXIT(1);
                     } else {
                        DPRINTF(("qmaster on host "SFQ" is down\n", sge_get_master(0)));
                     }
                  } else {
                     DPRINTF(("act_qmaster file contains local host name\n"));
                  }
               } else {
                  DPRINTF(("skipping qmaster alive check because act_qmaster is not availabe\n"));
               }

            }
            break;
         case QMON:
            DPRINTF(("creating QMON GDI handle\n"));
            handle = cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE, CL_FALSE, sge_get_qmaster_port(), CL_TCP_DEFAULT,
                                         (char*)prognames[uti_state_get_mewho()], my_component_id , 1 , 0 );
            cl_com_set_auto_close_mode(handle, CL_CM_AC_ENABLED );
            if (handle == NULL) {
               if (commlib_error != *last_commlib_error) {
                     ERROR((SGE_EVENT, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                                          uti_state_get_qualified_hostname(),
                                          (char*) prognames[uti_state_get_mewho()],
                                          sge_u32c(my_component_id), 
                                          sge_u32c(sge_get_qmaster_port()),
                                          cl_get_error_text(commlib_error)));
               }
            }
            break;

         default:
            /* this is for "normal" gdi clients of qmaster */
            DPRINTF(("creating GDI handle\n"));
            handle = cl_com_create_handle(&commlib_error, communication_framework, CL_CM_CT_MESSAGE, CL_FALSE, sge_get_qmaster_port(), CL_TCP_DEFAULT,
                                         (char*)prognames[uti_state_get_mewho()], my_component_id , 1 , 0 );
            if (handle == NULL) {
               if (commlib_error != *last_commlib_error) {
                     ERROR((SGE_EVENT, MSG_GDI_CANT_CONNECT_HANDLE_SSUUS, 
                                          uti_state_get_qualified_hostname(),
                                          (char*) prognames[uti_state_get_mewho()],
                                          sge_u32c(my_component_id), 
                                          sge_u32c(sge_get_qmaster_port()),
                                          cl_get_error_text(commlib_error)));
               }
            }
            break;
      }

   } 

#ifdef DEBUG_CLIENT_SUPPORT
   /* set rmon callback for message printing (after handle creation) */
   rmon_set_print_callback(gdi_rmon_print_callback_function);
#endif

   switch(me_who) {
      case QMASTER: {
         /* this is for testsuite socket bind test (issue 1096 ) */
         if ( getenv("SGE_TEST_SOCKET_BIND") != NULL) {
            struct timeval now;
            gettimeofday(&now,NULL);
        
            /* if this environment variable is set, we wait 15 seconds after 
               communication lib setup */
            DPRINTF(("waiting for 15 seconds, because environment SGE_TEST_SOCKET_BIND is set\n"));
            while ( handle != NULL && now.tv_sec - handle->start_time.tv_sec  <= 15 ) {
               DPRINTF(("timeout: "sge_U32CFormat"\n",sge_u32c(now.tv_sec - handle->start_time.tv_sec)));
               cl_commlib_trigger(handle, 1);
               gettimeofday(&now,NULL);
            }
            DPRINTF(("continue with setup\n"));
         }
         break;
      }
   }

   /* Store the actual commlib error in order to not log the same error twice */
   *last_commlib_error = commlib_error;
   DEXIT;
   return commlib_error;
}



/*---------------------------------------------------------
 *  sge_send_any_request
 *  returns 0 if ok
 *          -4 if peer is not alive or rhost == NULL
 *          return value of gdi_send_message() for other errors
 *
 *  NOTES
 *     MT-NOTE: sge_send_any_request() is MT safe (assumptions)
 *---------------------------------------------------------*/
int sge_send_any_request(int synchron, u_long32 *mid, const char *rhost, 
                         const char *commproc, int id, sge_pack_buffer *pb, 
                         int tag, u_long32  response_id, lList **alpp)
{
   int i;
   cl_xml_ack_type_t ack_type;
   cl_com_handle_t* handle = NULL;
   unsigned long dummy_mid = 0;
   unsigned long* mid_pointer = NULL;

   DENTER(GDI_LAYER, "sge_send_any_request");

   ack_type = CL_MIH_MAT_NAK;

   if (rhost == NULL) {
      answer_list_add(alpp, MSG_GDI_RHOSTISNULLFORSENDREQUEST, STATUS_ESYNTAX,
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return CL_RETVAL_PARAMS;
   }
   
   handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   if (handle == NULL) {
      answer_list_add(alpp, MSG_GDI_NOCOMMHANDLE, 
                      STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
      DEXIT;
      return CL_RETVAL_HANDLE_NOT_FOUND;
   }

   if (strcmp(commproc, (char*)prognames[QMASTER]) == 0 && id == 1) {
      cl_com_append_known_endpoint_from_name((char*)rhost, 
                                             (char*)prognames[QMASTER], 1 , 
                                             sge_get_qmaster_port(), 
                                             CL_CM_AC_DISABLED, CL_TRUE);
   }
   
   if (synchron) {
      ack_type = CL_MIH_MAT_ACK;
   }
  
#if 0
   SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_SENDINGMESSAGE_SIU, commproc,id,
                          (unsigned long) pb->bytes_used));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
#endif

   if (mid) {
      mid_pointer = &dummy_mid;
   }

   i = gdi_send_sec_message(handle,
                            (char*) rhost,(char*) commproc, id, 
                            ack_type, (cl_byte_t*)pb->head_ptr,
                            (unsigned long) pb->bytes_used, 
                            mid_pointer, response_id, tag, 1, synchron);
   if (i != CL_RETVAL_OK) {
      /* try again ( if connection timed out ) */
      i = gdi_send_sec_message(handle,
                               (char*)rhost, (char*)commproc, id, 
                               ack_type, (cl_byte_t*)pb->head_ptr,
                               (unsigned long) pb->bytes_used, mid_pointer, 
                               response_id, tag, 1, synchron);
   }
   
   if (mid) {
      *mid = dummy_mid;
   }

   if (i != CL_RETVAL_OK) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT,
                             MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS ,
                             (synchron ? "" : "a"),
                             commproc, 
                             id, 
                             rhost, 
                             cl_get_error_text(i)));
      answer_list_add(alpp, SGE_EVENT, STATUS_NOCOMMD, ANSWER_QUALITY_ERROR);
   }

   DEXIT;
   return i;
}


/*----------------------------------------------------------
 * sge_get_any_request
 *
 * returns 0               on success
 *         -1              rhost is NULL 
 *         commlib return values (always positive)
 *
 * NOTES
 *    MT-NOTE: sge_get_any_request() is MT safe (assumptions)
 *----------------------------------------------------------*/
int 
sge_get_any_request(char *rhost, char *commproc, u_short *id, sge_pack_buffer *pb, 
                    int *tag, int synchron, u_long32 for_request_mid, u_long32* mid) 
{
   int i;
   ushort usid=0;
   char host[CL_MAXHOSTLEN+1];
   cl_com_message_t* message = NULL;
   cl_com_endpoint_t* sender = NULL;
   cl_com_handle_t* handle = NULL;


   DENTER(GDI_LAYER, "sge_get_any_request");

   PROF_START_MEASUREMENT(SGE_PROF_GDI);

   if (id) {
      usid = (ushort)*id;
   }   

   if (!rhost) {
      ERROR((SGE_EVENT, MSG_GDI_RHOSTISNULLFORGETANYREQUEST ));
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return -1;
   }
   
   strcpy(host, rhost);

   handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);

   /* trigger communication or wait for a new message (select timeout) */
   cl_commlib_trigger(handle, synchron);

   i = gdi_receive_sec_message( handle, rhost, commproc, usid, synchron, for_request_mid, &message, &sender);

   if ( i == CL_RETVAL_CONNECTION_NOT_FOUND ) {
      if ( commproc[0] != '\0' && rhost[0] != '\0' ) {
         /* The connection was closed, reopen it */
         i = cl_commlib_open_connection(handle,rhost,commproc,usid);
         INFO((SGE_EVENT,"reopen connection to %s,%s,"sge_U32CFormat" (2)\n", rhost, commproc, sge_u32c(usid)));
         if (i == CL_RETVAL_OK) {
            INFO((SGE_EVENT,"reconnected successfully\n"));
            i = gdi_receive_sec_message( handle, rhost, commproc, usid, synchron, for_request_mid, &message, &sender);
         }
      } else {
         DEBUG((SGE_EVENT,"can't reopen a connection to unspecified host or commproc (2)\n"));
      }
   }

   if (i != CL_RETVAL_OK) {
      if (i != CL_RETVAL_NO_MESSAGE) {
         /* This if for errors */
         DEBUG((SGE_EVENT, MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS , 
               (commproc[0] ? commproc : "any"), 
               (int) usid, 
               (host[0] ? host : "any"),
                cl_get_error_text(i)));
      }
      cl_com_free_message(&message);
      cl_com_free_endpoint(&sender);
      /* This is if no message is there */
      PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
      DEXIT;
      return i;
   }    

   /* ok, we received a message */
   if (message != NULL ) {
      if (sender != NULL && id) {
         *id = (u_short)sender->comp_id;
      }
      if (tag) {
        *tag = (int)message->message_tag;
      }  
      if (mid) {
        *mid = message->message_id;
      }  


      /* fill it in the packing buffer */
      i = init_packbuffer_from_buffer(pb, (char*)message->message, message->message_length);

      /* TODO: the packbuffer must be hold, not deleted !!! */
      message->message = NULL;

      if(i != PACK_SUCCESS) {
         ERROR((SGE_EVENT, MSG_GDI_ERRORUNPACKINGGDIREQUEST_S, cull_pack_strerror(i)));
         PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
         DEXIT;
         return CL_RETVAL_READ_ERROR;
      } 

      if (sender != NULL ) {
         DEBUG((SGE_EVENT,"received from: %s,"sge_U32CFormat"\n",sender->comp_host, sge_u32c(sender->comp_id) ));
         if (rhost[0] == '\0') {
            strcpy(rhost, sender->comp_host); /* If we receive from anybody return the sender */
         }
         if (commproc[0] == '\0') {
            strcpy(commproc , sender->comp_name); /* If we receive from anybody return the sender */
         }
      }

      cl_com_free_endpoint(&sender);
      cl_com_free_message(&message);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_GDI);
   DEXIT;
   return CL_RETVAL_OK;
}


/**********************************************************************
  send a message giving a packbuffer

  same as gdi_send_message, but this is delivered a sge_pack_buffer.
  this function flushes the z_stream_buffer if compression is turned on
  and passes the result on to send_message
  Always use this function instead of gdi_send_message directly, even
  if compression is turned off.
  
    NOTES
       MT-NOTE: gdi_send_message_pb() is MT safe (assumptions)
**********************************************************************/
int gdi_send_message_pb(int synchron, const char *tocomproc, int toid, 
                        const char *tohost, int tag, sge_pack_buffer *pb, 
                        u_long32 *mid) 
{
   long ret = 0;

   DENTER(GDI_LAYER, "gdi_send_message_pb");

   if ( !pb ) {
       DPRINTF(("no pointer for sge_pack_buffer\n"));
       ret = gdi_send_message(synchron, tocomproc, toid, tohost, tag, NULL, 0, mid);
       DEXIT;
       return ret;
   }

   ret = gdi_send_message(synchron, tocomproc, toid, tohost, tag, pb->head_ptr, pb->bytes_used, mid);

   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*
 * check_isalive
 *    check if master is registered and alive
 *    calls is_commd_alive() and ask_commproc()
 * returns
 *    0                    if qmaster is enrolled at sge_commd
 *    > 0                  commlib error number (always positve)
 *    CL_FIRST_FREE_EC+1   can't get masterhost
 *    CL_FIRST_FREE_EC+2   can't connect to commd
 *
 *  NOTES
 *     MT-NOTE: check_isalive() is MT safe
 *-------------------------------------------------------------------------*/
int check_isalive(const char *masterhost) 
{
   int alive = CL_RETVAL_OK;
   cl_com_handle_t* handle = NULL;
   cl_com_SIRM_t* status = NULL;
   int ret;
   static int error_locked = 0;

 
   DENTER(TOP_LAYER, "check_isalive");

   if (!masterhost) {
      DPRINTF(("can't get masterhost\n"));
      DEXIT;
      return CL_RETVAL_UNKNOWN_ENDPOINT;
   }


   
   handle=cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0);
   if (handle == NULL) {

      sge_mutex_lock("check_alive_mutex", SGE_FUNC, __LINE__, &check_alive_mutex);  

      if ( error_locked == 0 ) {
         WARNING((SGE_EVENT,MSG_GDI_COULD_NOT_GET_COM_HANDLE_S, (char*) uti_state_get_sge_formal_prog_name() ));
      }
      error_locked = 1;

      sge_mutex_unlock("check_alive_mutex", SGE_FUNC, __LINE__, &check_alive_mutex);  
      return CL_RETVAL_UNKNOWN;
   }

   sge_mutex_lock("check_alive_mutex", SGE_FUNC, __LINE__, &check_alive_mutex);  
   error_locked = 0;
   sge_mutex_unlock("check_alive_mutex", SGE_FUNC, __LINE__, &check_alive_mutex);  



   ret = cl_commlib_get_endpoint_status(handle,(char*)masterhost,(char*)prognames[QMASTER] , 1, &status);
   if (ret != CL_RETVAL_OK) {
      DPRINTF(("cl_commlib_get_endpoint_status() returned "SFQ"\n", cl_get_error_text(ret)));
      alive = ret;
   } else {
      DEBUG((SGE_EVENT,MSG_GDI_QMASTER_STILL_RUNNING));   
      alive = CL_RETVAL_OK;
   }

   if (status != NULL) {
      DEBUG((SGE_EVENT,MSG_GDI_ENDPOINT_UPTIME_UU, sge_u32c( status->runtime) , sge_u32c( status->application_status) ));
      cl_com_free_sirm_message(&status);
   }
 
   DEXIT;
   return alive;
}
