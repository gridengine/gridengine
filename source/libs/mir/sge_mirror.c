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
#include <sys/types.h>

#include "sge_time.h"
#include "sge_profiling.h"

#include "sge_event.h"
#include "sge_event_client.h"

#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_complex.h"
#include "sge_conf.h"
#include "sge_host.h"
#include "sge_hgroup.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_manop.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_schedd_conf.h"
#include "sge_sharetree.h"
#include "sge_cuser.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_answer.h"

#include "sge_unistd.h"
#include "sgermon.h"
#include "sge_log.h"

#include "msg_evclib.h"
#include "msg_mirlib.h"

#include "sge_host_mirror.h"
#include "sge_queue_mirror.h"
#include "sge_job_mirror.h"
#include "sge_ja_task_mirror.h"
#include "sge_pe_task_mirror.h"
#include "sge_sharetree_mirror.h"
#include "sge_sched_conf_mirror.h"

#include "sge_mirror.h"

/* for profiling output, number of processed events */
int num_events = 0;

/* Static functions for internal use */
static sge_mirror_error _sge_mirror_subscribe(sge_object_type type, 
                                              sge_mirror_callback callback_before, 
                                              sge_mirror_callback callback_after, 
                                              void *clientdata);

static sge_mirror_error _sge_mirror_unsubscribe(sge_object_type type);     

static void sge_mirror_free_list(sge_object_type type);

static sge_mirror_error sge_mirror_process_event_list(lList *event_list);
static sge_mirror_error sge_mirror_process_event(sge_object_type type, 
                                                 sge_event_action action, 
                                                 lListElem *event);

static bool sge_mirror_process_shutdown(sge_object_type type, 
                                             sge_event_action action, 
                                             lListElem *event,
                                             void *clientdata);
static bool sge_mirror_process_qmaster_goes_down(sge_object_type type, 
                                                sge_event_action action, 
                                                lListElem *event, 
                                                void *clientdata);

static bool 
generic_update_master_list(sge_object_type type, sge_event_action action,
                           lListElem *event, void *clientdata);

/* Datastructure for internal storage of subscription information
 * and callbacks.
 */
typedef struct {
   sge_mirror_callback callback_before;   /* callback before mirroring      */
   sge_mirror_callback callback_default;  /* default mirroring function     */
   sge_mirror_callback callback_after;    /* callback after mirroring       */
   void *clientdata;                      /* client data passed to callback */
} mirror_description;

/* One entry per event type */
static mirror_description mirror_base[SGE_TYPE_ALL] = {
   /*cbb   cbd                                     cba   cd   */
   { NULL, host_update_master_list,                NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, NULL,                                   NULL, NULL },
   { NULL, host_update_master_list,                NULL, NULL },
   { NULL, ja_task_update_master_list,             NULL, NULL },
   { NULL, pe_task_update_master_list,             NULL, NULL },
   { NULL, job_update_master_list,                 NULL, NULL },
   { NULL, job_schedd_info_update_master_list,     NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, sharetree_update_master_list,           NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, queue_update_master_list,               NULL, NULL },
   { NULL, schedd_conf_update_master_list,         NULL, NULL },
   { NULL, NULL,                                   NULL, NULL },
   { NULL, sge_mirror_process_shutdown,            NULL, NULL },
   { NULL, sge_mirror_process_qmaster_goes_down,   NULL, NULL },
   { NULL, host_update_master_list,                NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },
   { NULL, generic_update_master_list,             NULL, NULL },

#ifndef __SGE_NO_USERMAPPING__
   { NULL, NULL,                                   NULL, NULL },
#endif
   { NULL, NULL,                                   NULL, NULL },
};

/* Static functions for internal use */
static sge_mirror_error _sge_mirror_subscribe(sge_object_type type, 
                                              sge_mirror_callback callback_before, 
                                              sge_mirror_callback callback_after, 
                                              void *clientdata);

static sge_mirror_error _sge_mirror_unsubscribe(sge_object_type type);     

static void sge_mirror_free_list(sge_object_type type);

static sge_mirror_error sge_mirror_process_event_list(lList *event_list);
static sge_mirror_error sge_mirror_process_event(sge_object_type type, 
                                                 sge_event_action action, 
                                                 lListElem *event);

/****** Eventmirror/sge_mirror_initialize() *************************************
*  NAME
*     sge_mirror_initialize() -- initialize the event mirror interface
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_initialize(ev_registration_id id, 
*                                            const char *name) 
*
*  FUNCTION
*     Initializes internal data structures and registers with qmaster
*     using the event client mechanisms.
*
*     Events covering shutdown requests and qmaster shutdown notification
*     are subscribed.
*     
*
*  INPUTS
*     ev_registration_id id - id used to register with qmaster
*     const char *name      - name used to register with qmaster 
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/sge_mirror_shutdown()
*     Eventclient/-ID-numbers
*******************************************************************************/
sge_mirror_error sge_mirror_initialize(ev_registration_id id, const char *name)
{
   int i;

   DENTER(TOP_LAYER, "sge_mirror_initialize");

   /* initialize mirroring data structures - only changeable fields */
   for(i = 0; i < SGE_TYPE_ALL; i++) {
      mirror_base[i].callback_before  = NULL;
      mirror_base[i].callback_after   = NULL;
      mirror_base[i].clientdata       = NULL;
   }

   /* registration with event client interface */
   ec_prepare_registration(id, name);

   /* subscribe some events with default handling */
   sge_mirror_subscribe(SGE_TYPE_SHUTDOWN, NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_QMASTER_GOES_DOWN, NULL, NULL, NULL);

   /* register with qmaster */
   ec_commit();

   DEXIT;
   return SGE_EM_OK;
}

/****** Eventmirror/sge_mirror_shutdown() ***************************************
*  NAME
*     sge_mirror_shutdown() -- shutdown mirroring
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_shutdown(void) 
*
*  FUNCTION
*     Shuts down the event mirroring mechanism:
*     Unsubscribes all events, deletes contents of the corresponding
*     object lists and deregisteres from qmaster.
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or error code
*
*  SEE ALSO
*     Eventmirror/sge_mirror_initialize()
*******************************************************************************/
sge_mirror_error sge_mirror_shutdown(void)
{
   DENTER(TOP_LAYER, "sge_mirror_shutdown");

   if(ec_is_initialized()) {
      sge_mirror_unsubscribe(SGE_TYPE_ALL);
      ec_deregister();
   }

   DEXIT;
   return SGE_EM_OK;
}   

/****** Eventmirror/sge_mirror_subscribe() **************************************
*  NAME
*     sge_mirror_subscribe() -- subscribe certain event types
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_subscribe(sge_object_type type, 
*                                           sge_mirror_callback callback_before, 
*                                           sge_mirror_callback callback_after, 
*                                           void *clientdata) 
*
*  FUNCTION
*     Subscribe a certain event type.
*     Callback functions can be specified, that can be executed before the 
*     mirroring action and/or after the mirroring action.
*
*     The corresponding data structures are initialized,
*     the events associated with the event type are subscribed with the 
*     event client interface.
*
*  INPUTS
*     sge_object_type type                 - event type to subscribe or
*                                           SGE_TYPE_ALL
*     sge_mirror_callback callback_before - callback to be executed before 
*                                           mirroring
*     sge_mirror_callback callback_after  - callback to be executed after 
*                                           mirroring
*     void *clientdata                    - clientdata to be passed to the 
*                                           callback functions
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/-Eventmirror-Typedefs
*     Eventclient/-Subscription
*     Eventclient/-Events
*     Eventmirror/sge_mirror_unsubscribe()
*******************************************************************************/
sge_mirror_error sge_mirror_subscribe(sge_object_type type, 
                                      sge_mirror_callback callback_before, 
                                      sge_mirror_callback callback_after, 
                                      void *clientdata)
{
   sge_mirror_error ret = SGE_EM_OK;

   DENTER(TOP_LAYER, "sge_mirror_subscribe");

   if(type < 0 || type > SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_MIRROR_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
      DEXIT;
      return SGE_EM_BAD_ARG;
   }

   if(type == SGE_TYPE_ALL) {
      int i;

      for(i = 0; i < SGE_TYPE_ALL; i++) {
         if((ret = _sge_mirror_subscribe(i, callback_before, callback_after, clientdata)) != SGE_EM_OK) {
            break;
         }
      }
   } else {
      ret = _sge_mirror_subscribe(type, callback_before, callback_after, clientdata);
   }

   DEXIT;
   return ret;
}   
  
static sge_mirror_error _sge_mirror_subscribe(sge_object_type type, 
                                              sge_mirror_callback callback_before, 
                                              sge_mirror_callback callback_after, 
                                              void *clientdata)
{
   /* type already has been checked before */
   switch(type) {
      case SGE_TYPE_ADMINHOST:
         ec_subscribe(sgeE_ADMINHOST_LIST);
         ec_subscribe(sgeE_ADMINHOST_ADD);
         ec_subscribe(sgeE_ADMINHOST_DEL);
         ec_subscribe(sgeE_ADMINHOST_MOD);
         break;
      case SGE_TYPE_CALENDAR:
         ec_subscribe(sgeE_CALENDAR_LIST);
         ec_subscribe(sgeE_CALENDAR_ADD);
         ec_subscribe(sgeE_CALENDAR_DEL);
         ec_subscribe(sgeE_CALENDAR_MOD);
         break;
      case SGE_TYPE_CKPT:
         ec_subscribe(sgeE_CKPT_LIST);
         ec_subscribe(sgeE_CKPT_ADD);
         ec_subscribe(sgeE_CKPT_DEL);
         ec_subscribe(sgeE_CKPT_MOD);
         break;
      case SGE_TYPE_COMPLEX:
         ec_subscribe(sgeE_COMPLEX_LIST);
         ec_subscribe(sgeE_COMPLEX_ADD);
         ec_subscribe(sgeE_COMPLEX_DEL);
         ec_subscribe(sgeE_COMPLEX_MOD);
         break;
      case SGE_TYPE_CONFIG:
         ec_subscribe(sgeE_CONFIG_LIST);
         ec_subscribe(sgeE_CONFIG_ADD);
         ec_subscribe(sgeE_CONFIG_DEL);
         ec_subscribe(sgeE_CONFIG_MOD);
         break;
      case SGE_TYPE_GLOBAL_CONFIG:
         ec_subscribe(sgeE_GLOBAL_CONFIG);
                                         /* regard it as a sort of trigger
                                          * in fact this event is not needed!
                                          * it doesn't even contain the config data!
                                          * it is sent before the CONFIG_MOD event, so
                                          * the receiver cannot even properly react to it!
                                          */
         break;
      case SGE_TYPE_EXECHOST:
         ec_subscribe(sgeE_EXECHOST_LIST);
         ec_subscribe(sgeE_EXECHOST_ADD);
         ec_subscribe(sgeE_EXECHOST_DEL);
         ec_subscribe(sgeE_EXECHOST_MOD);
         break;
      case SGE_TYPE_JATASK:
         ec_subscribe(sgeE_JATASK_ADD);
         ec_subscribe(sgeE_JATASK_DEL);
         ec_subscribe(sgeE_JATASK_MOD);
         break;
      case SGE_TYPE_PETASK:
         ec_subscribe(sgeE_PETASK_ADD);
         ec_subscribe(sgeE_PETASK_DEL);
         break;
      case SGE_TYPE_JOB:
         ec_subscribe(sgeE_JOB_LIST);
         ec_subscribe(sgeE_JOB_ADD);
         ec_subscribe(sgeE_JOB_DEL);
         ec_subscribe(sgeE_JOB_MOD);
         ec_subscribe(sgeE_JOB_MOD_SCHED_PRIORITY);
         ec_subscribe(sgeE_JOB_USAGE);
         ec_subscribe(sgeE_JOB_FINAL_USAGE);
/*          ec_subscribe(sgeE_JOB_FINISH); */
         break;
      case SGE_TYPE_JOB_SCHEDD_INFO:
         ec_subscribe(sgeE_JOB_SCHEDD_INFO_LIST);
         ec_subscribe(sgeE_JOB_SCHEDD_INFO_ADD);
         ec_subscribe(sgeE_JOB_SCHEDD_INFO_DEL);
         ec_subscribe(sgeE_JOB_SCHEDD_INFO_MOD);
         break;
      case SGE_TYPE_MANAGER:
         ec_subscribe(sgeE_MANAGER_LIST);
         ec_subscribe(sgeE_MANAGER_ADD);
         ec_subscribe(sgeE_MANAGER_DEL);
         ec_subscribe(sgeE_MANAGER_MOD);
         break;
      case SGE_TYPE_OPERATOR:
         ec_subscribe(sgeE_OPERATOR_LIST);
         ec_subscribe(sgeE_OPERATOR_ADD);
         ec_subscribe(sgeE_OPERATOR_DEL);
         ec_subscribe(sgeE_OPERATOR_MOD);
         break;
      case SGE_TYPE_SHARETREE:
         ec_subscribe(sgeE_NEW_SHARETREE);
         break;
      case SGE_TYPE_PE:
         ec_subscribe(sgeE_PE_LIST);
         ec_subscribe(sgeE_PE_ADD);
         ec_subscribe(sgeE_PE_DEL);
         ec_subscribe(sgeE_PE_MOD);
         break;
      case SGE_TYPE_PROJECT:
         ec_subscribe(sgeE_PROJECT_LIST);
         ec_subscribe(sgeE_PROJECT_ADD);
         ec_subscribe(sgeE_PROJECT_DEL);
         ec_subscribe(sgeE_PROJECT_MOD);
         break;
      case SGE_TYPE_QUEUE:
         ec_subscribe(sgeE_QUEUE_LIST);
         ec_subscribe(sgeE_QUEUE_ADD);
         ec_subscribe(sgeE_QUEUE_DEL);
         ec_subscribe(sgeE_QUEUE_MOD);
         ec_subscribe(sgeE_QUEUE_SUSPEND_ON_SUB);
         ec_subscribe(sgeE_QUEUE_UNSUSPEND_ON_SUB);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         ec_subscribe(sgeE_SCHED_CONF);
         break;
      case SGE_TYPE_SCHEDD_MONITOR:
         ec_subscribe(sgeE_SCHEDDMONITOR);
         break;
      case SGE_TYPE_SHUTDOWN:
         ec_subscribe_flush(sgeE_SHUTDOWN, 0);
         break;
      case SGE_TYPE_QMASTER_GOES_DOWN:
         ec_subscribe_flush(sgeE_QMASTER_GOES_DOWN, 0);
         break;
      case SGE_TYPE_SUBMITHOST:
         ec_subscribe(sgeE_SUBMITHOST_LIST);
         ec_subscribe(sgeE_SUBMITHOST_ADD);
         ec_subscribe(sgeE_SUBMITHOST_DEL);
         ec_subscribe(sgeE_SUBMITHOST_MOD);
         break;
      case SGE_TYPE_USER:
         ec_subscribe(sgeE_USER_LIST);
         ec_subscribe(sgeE_USER_ADD);
         ec_subscribe(sgeE_USER_DEL);
         ec_subscribe(sgeE_USER_MOD);
         break;
      case SGE_TYPE_USERSET:
         ec_subscribe(sgeE_USERSET_LIST);
         ec_subscribe(sgeE_USERSET_ADD);
         ec_subscribe(sgeE_USERSET_DEL);
         ec_subscribe(sgeE_USERSET_MOD);
         break;
      case SGE_TYPE_HGROUP:
         ec_subscribe(sgeE_HGROUP_LIST);
         ec_subscribe(sgeE_HGROUP_ADD);
         ec_subscribe(sgeE_HGROUP_DEL);
         ec_subscribe(sgeE_HGROUP_MOD);
         break;
#ifndef __SGE_NO_USERMAPPING__
      /* JG: TODO: usermapping and hostgroup not yet implemented */
      case SGE_TYPE_CUSER:
         ec_subscribe(sgeE_CUSER_LIST);
         ec_subscribe(sgeE_CUSER_ADD);
         ec_subscribe(sgeE_CUSER_DEL);
         ec_subscribe(sgeE_CUSER_MOD);
         break;
#endif

      default:
         /* development fault: forgot to handle a valid event group */
         abort();
         return SGE_EM_BAD_ARG;
   }

   /* install callback function */
   mirror_base[type].callback_before = callback_before;
   mirror_base[type].callback_after  = callback_after;
   mirror_base[type].clientdata      = clientdata;

   return SGE_EM_OK;
}   

/****** Eventmirror/sge_mirror_unsubscribe() ************************************
*  NAME
*     sge_mirror_unsubscribe() -- unsubscribe event types 
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_unsubscribe(sge_object_type type) 
*
*  FUNCTION
*     Unsubscribes a certain event type or all if SGE_TYPE_ALL is given as type.
*  
*     Unsubscribes the corresponding events in the underlying event client 
*     interface and frees data stored in the corresponding mirrored list(s).
*  INPUTS
*     sge_object_type type - the event type to unsubscribe or SGE_TYPE_ALL
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/-Eventmirror-Typedefs
*     Eventclient/-Subscription
*     Eventclient/-Events
*     Eventmirror/sge_mirror_subscribe()
*******************************************************************************/
sge_mirror_error sge_mirror_unsubscribe(sge_object_type type)
{
   DENTER(TOP_LAYER, "sge_mirror_unsubscribe");

   if(type < 0 || type > SGE_TYPE_ALL) {
      ERROR((SGE_EVENT, MSG_MIRROR_INVALID_OBJECT_TYPE_SI, SGE_FUNC, type));
      DEXIT;
      return SGE_EM_BAD_ARG;
   }

   if(type == SGE_TYPE_ALL) {
      int i;
      
      for(i = 0; i < SGE_TYPE_ALL; i++) {
         if(i != SGE_TYPE_SHUTDOWN && i != SGE_TYPE_QMASTER_GOES_DOWN) {
            _sge_mirror_unsubscribe(i);
         }
      }
   } else {
      _sge_mirror_unsubscribe(type);
   }
   
   DEXIT;
   return SGE_EM_OK;
}

static sge_mirror_error _sge_mirror_unsubscribe(sge_object_type type) 
{
   DENTER(TOP_LAYER, "_sge_mirror_unsubscribe");
   /* type has been checked in calling function */
   /* clear callback information */
   mirror_base[type].callback_before  = NULL;
   mirror_base[type].callback_after   = NULL;
   mirror_base[type].clientdata       = NULL;
  
   switch(type) {
      case SGE_TYPE_ADMINHOST:
         ec_unsubscribe(sgeE_ADMINHOST_LIST);
         ec_unsubscribe(sgeE_ADMINHOST_ADD);
         ec_unsubscribe(sgeE_ADMINHOST_DEL);
         ec_unsubscribe(sgeE_ADMINHOST_MOD);
         break;
      case SGE_TYPE_CALENDAR:
         ec_unsubscribe(sgeE_CALENDAR_LIST);
         ec_unsubscribe(sgeE_CALENDAR_ADD);
         ec_unsubscribe(sgeE_CALENDAR_DEL);
         ec_unsubscribe(sgeE_CALENDAR_MOD);
         break;
      case SGE_TYPE_CKPT:
         ec_unsubscribe(sgeE_CKPT_LIST);
         ec_unsubscribe(sgeE_CKPT_ADD);
         ec_unsubscribe(sgeE_CKPT_DEL);
         ec_unsubscribe(sgeE_CKPT_MOD);
         break;
      case SGE_TYPE_COMPLEX:
         ec_unsubscribe(sgeE_COMPLEX_LIST);
         ec_unsubscribe(sgeE_COMPLEX_ADD);
         ec_unsubscribe(sgeE_COMPLEX_DEL);
         ec_unsubscribe(sgeE_COMPLEX_MOD);
         break;
      case SGE_TYPE_CONFIG:
         ec_unsubscribe(sgeE_CONFIG_LIST);
         ec_unsubscribe(sgeE_CONFIG_ADD);
         ec_unsubscribe(sgeE_CONFIG_DEL);
         ec_unsubscribe(sgeE_CONFIG_MOD);
         break;
      case SGE_TYPE_GLOBAL_CONFIG:
         ec_unsubscribe(sgeE_GLOBAL_CONFIG);
         break;
      case SGE_TYPE_EXECHOST:
         ec_unsubscribe(sgeE_EXECHOST_LIST);
         ec_unsubscribe(sgeE_EXECHOST_ADD);
         ec_unsubscribe(sgeE_EXECHOST_DEL);
         ec_unsubscribe(sgeE_EXECHOST_MOD);
         break;
      case SGE_TYPE_JATASK:
         ec_unsubscribe(sgeE_JATASK_ADD);
         ec_unsubscribe(sgeE_JATASK_DEL);
         ec_unsubscribe(sgeE_JATASK_MOD);
         break;
      case SGE_TYPE_PETASK:
         ec_unsubscribe(sgeE_PETASK_ADD);
         ec_unsubscribe(sgeE_PETASK_DEL);
         break;
      case SGE_TYPE_JOB:
         ec_unsubscribe(sgeE_JOB_LIST);
         ec_unsubscribe(sgeE_JOB_ADD);
         ec_unsubscribe(sgeE_JOB_DEL);
         ec_unsubscribe(sgeE_JOB_MOD);
         ec_unsubscribe(sgeE_JOB_MOD_SCHED_PRIORITY);
         ec_unsubscribe(sgeE_JOB_USAGE);
         ec_unsubscribe(sgeE_JOB_FINAL_USAGE);
/*          ec_unsubscribe(sgeE_JOB_FINISH); */
         break;
      case SGE_TYPE_JOB_SCHEDD_INFO:
         ec_unsubscribe(sgeE_JOB_SCHEDD_INFO_LIST);
         ec_unsubscribe(sgeE_JOB_SCHEDD_INFO_ADD);
         ec_unsubscribe(sgeE_JOB_SCHEDD_INFO_DEL);
         ec_unsubscribe(sgeE_JOB_SCHEDD_INFO_MOD);
         break;
      case SGE_TYPE_MANAGER:
         ec_unsubscribe(sgeE_MANAGER_LIST);
         ec_unsubscribe(sgeE_MANAGER_ADD);
         ec_unsubscribe(sgeE_MANAGER_DEL);
         ec_unsubscribe(sgeE_MANAGER_MOD);
         break;
      case SGE_TYPE_OPERATOR:
         ec_unsubscribe(sgeE_OPERATOR_LIST);
         ec_unsubscribe(sgeE_OPERATOR_ADD);
         ec_unsubscribe(sgeE_OPERATOR_DEL);
         ec_unsubscribe(sgeE_OPERATOR_MOD);
         break;
      case SGE_TYPE_SHARETREE:
         ec_unsubscribe(sgeE_NEW_SHARETREE);
         break;
      case SGE_TYPE_PE:
         ec_unsubscribe(sgeE_PE_LIST);
         ec_unsubscribe(sgeE_PE_ADD);
         ec_unsubscribe(sgeE_PE_DEL);
         ec_unsubscribe(sgeE_PE_MOD);
         break;
      case SGE_TYPE_PROJECT:
         ec_unsubscribe(sgeE_PROJECT_LIST);
         ec_unsubscribe(sgeE_PROJECT_ADD);
         ec_unsubscribe(sgeE_PROJECT_DEL);
         ec_unsubscribe(sgeE_PROJECT_MOD);
         break;
      case SGE_TYPE_QUEUE:
         ec_unsubscribe(sgeE_QUEUE_LIST);
         ec_unsubscribe(sgeE_QUEUE_ADD);
         ec_unsubscribe(sgeE_QUEUE_DEL);
         ec_unsubscribe(sgeE_QUEUE_MOD);
         ec_unsubscribe(sgeE_QUEUE_SUSPEND_ON_SUB);
         ec_unsubscribe(sgeE_QUEUE_UNSUSPEND_ON_SUB);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         ec_unsubscribe(sgeE_SCHED_CONF);
         break;
      case SGE_TYPE_SCHEDD_MONITOR:
         ec_unsubscribe(sgeE_SCHEDDMONITOR);
         break;
      case SGE_TYPE_SHUTDOWN:
         ERROR((SGE_EVENT, MSG_EVENT_HAVETOHANDLEEVENTS));
         break;
      case SGE_TYPE_QMASTER_GOES_DOWN:
         ERROR((SGE_EVENT, MSG_EVENT_HAVETOHANDLEEVENTS));
         break;
      case SGE_TYPE_SUBMITHOST:
         ec_unsubscribe(sgeE_SUBMITHOST_LIST);
         ec_unsubscribe(sgeE_SUBMITHOST_ADD);
         ec_unsubscribe(sgeE_SUBMITHOST_DEL);
         ec_unsubscribe(sgeE_SUBMITHOST_MOD);
         break;
      case SGE_TYPE_USER:
         ec_unsubscribe(sgeE_USER_LIST);
         ec_unsubscribe(sgeE_USER_ADD);
         ec_unsubscribe(sgeE_USER_DEL);
         ec_unsubscribe(sgeE_USER_MOD);
         break;
      case SGE_TYPE_USERSET:
         ec_unsubscribe(sgeE_USERSET_LIST);
         ec_unsubscribe(sgeE_USERSET_ADD);
         ec_unsubscribe(sgeE_USERSET_DEL);
         ec_unsubscribe(sgeE_USERSET_MOD);
         break;
      case SGE_TYPE_HGROUP:
         ec_unsubscribe(sgeE_HGROUP_LIST);
         ec_unsubscribe(sgeE_HGROUP_ADD);
         ec_unsubscribe(sgeE_HGROUP_DEL);
         ec_unsubscribe(sgeE_HGROUP_MOD);
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         ec_unsubscribe(sgeE_CUSER_LIST);
         ec_unsubscribe(sgeE_CUSER_ADD);
         ec_unsubscribe(sgeE_CUSER_DEL);
         ec_unsubscribe(sgeE_CUSER_MOD);
         break;
#endif

      default:
         /* development fault: forgot to handle a valid event group */
         abort();
         DEXIT;
         return SGE_EM_BAD_ARG;
   }

   /* clear data */
   sge_mirror_free_list(type);

   DEXIT;
   return SGE_EM_OK;
}

/****** Eventmirror/sge_mirror_process_events() *********************************
*  NAME
*     sge_mirror_process_events() -- retrieve and process events 
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_process_events(void) 
*
*  FUNCTION
*     Retrieves new events from qmaster.
*     If new events have arrived from qmaster, they are processed, 
*     that means, for each event 
*     - if installed, a "before mirroring" callback is called
*     - the event is mirrored into the corresponding master list
*     - if installed, a "after mirroring" callback is called
*
*     If retrieving new events from qmaster fails over a time period 
*     of 10 times the configured event delivery interval (see event
*     client interface, function ec_get_edtime), a timeout warning
*     is generated and a new registration of the event client is
*     prepared.
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventclient/Client/ec_get_edtime()
*     Eventclient/Client/ec_set_edtime()
*******************************************************************************/
sge_mirror_error sge_mirror_process_events(void)
{
   static u_long32 last_heared = 0;
   u_long32 now;
   lList *event_list = NULL;
   sge_mirror_error ret = SGE_EM_OK;

   DENTER(TOP_LAYER, "sge_mirror_process_events");

   PROF_START_MEASUREMENT(SGE_PROF_MIRROR);

   now = sge_get_gmt();
   num_events = 0;

   if(ec_get(&event_list, true)) {
      last_heared = now;
      if(event_list != NULL) {
         ret = sge_mirror_process_event_list(event_list);
         lFreeList(event_list);
      }
   } else {
      if(last_heared != 0 && (now > last_heared + ec_get_edtime() * 10)) {
         WARNING((SGE_EVENT, MSG_MIRROR_QMASTERALIVETIMEOUTEXPIRED));
         ec_mark4registration();
         ret = SGE_EM_TIMEOUT;
      }
   }

   if(prof_is_active()) {
      prof_stop_measurement(SGE_PROF_MIRROR, NULL);

      INFO((SGE_EVENT, "sge_mirror: processed %d events in %.3f s\n", 
            num_events, prof_get_measurement_wallclock(SGE_PROF_MIRROR, false, NULL)
          ));
   }

   
   DEXIT;
   return SGE_EM_OK;
}

/****** Eventmirror/sge_mirror_strerror() ***************************************
*  NAME
*     sge_mirror_strerror() -- map errorcode to error message 
*
*  SYNOPSIS
*     const char* sge_mirror_strerror(sge_mirror_error num) 
*
*  FUNCTION
*     Returns a string describing a given error number.
*     This function can be used to output error messages
*     if a function of the event mirror interface fails.
*
*  INPUTS
*     sge_mirror_error num - error number
*
*  RESULT
*     const char* - corresponding error message
*
*  SEE ALSO
*     Eventmirror/-Eventmirror-Typedefs
*******************************************************************************/
const char *sge_mirror_strerror(sge_mirror_error num) 
{
   switch(num) {
      case SGE_EM_OK:
         return MSG_MIRROR_OK;
      case SGE_EM_NOT_INITIALIZED:
         return MSG_MIRROR_NOTINITIALIZED;
      case SGE_EM_BAD_ARG:
         return MSG_MIRROR_BADARG;
      case SGE_EM_TIMEOUT:
         return MSG_MIRROR_TIMEOUT;
      case SGE_EM_DUPLICATE_KEY:
         return MSG_MIRROR_DUPLICATEKEY;
      case SGE_EM_KEY_NOT_FOUND:
         return MSG_MIRROR_KEYNOTFOUND;
      case SGE_EM_CALLBACK_FAILED:
         return MSG_MIRROR_CALLBACKFAILED;
      case SGE_EM_PROCESS_ERRORS:
         return MSG_MIRROR_PROCESSERRORS;
      default:
         return "";
   }
}

static void sge_mirror_free_list(sge_object_type type)
{
   object_type_free_master_list(type);
}

static sge_mirror_error sge_mirror_process_event_list(lList *event_list)
{ 
   lListElem *event;
   sge_mirror_error function_ret;

   DENTER(TOP_LAYER, "sge_mirror_process_event_list");

   function_ret = SGE_EM_OK;
   num_events = 0;

   for_each(event, event_list) {
      sge_mirror_error ret = SGE_EM_OK;
      num_events++;

      switch(lGetUlong(event, ET_type)) {
         case sgeE_ADMINHOST_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_ADMINHOST, SGE_EMA_LIST, event);
            break;
         case sgeE_ADMINHOST_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_ADMINHOST, SGE_EMA_ADD, event);
            break;
         case sgeE_ADMINHOST_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_ADMINHOST, SGE_EMA_DEL, event);
            break;
         case sgeE_ADMINHOST_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_ADMINHOST, SGE_EMA_MOD, event);
            break;

         case sgeE_CALENDAR_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_CALENDAR, SGE_EMA_LIST, event);
            break;
         case sgeE_CALENDAR_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_CALENDAR, SGE_EMA_ADD, event);
            break;
         case sgeE_CALENDAR_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_CALENDAR, SGE_EMA_DEL, event);
            break;
         case sgeE_CALENDAR_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_CALENDAR, SGE_EMA_MOD, event);
            break;

         case sgeE_CKPT_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_CKPT, SGE_EMA_LIST, event);
            break;
         case sgeE_CKPT_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_CKPT, SGE_EMA_ADD, event);
            break;
         case sgeE_CKPT_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_CKPT, SGE_EMA_DEL, event);
            break;
         case sgeE_CKPT_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_CKPT, SGE_EMA_MOD, event);
            break;

         case sgeE_COMPLEX_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_COMPLEX, SGE_EMA_LIST, event);
            break;
         case sgeE_COMPLEX_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_COMPLEX, SGE_EMA_ADD, event);
            break;
         case sgeE_COMPLEX_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_COMPLEX, SGE_EMA_DEL, event);
            break;
         case sgeE_COMPLEX_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_COMPLEX, SGE_EMA_MOD, event);
            break;

         case sgeE_CONFIG_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_CONFIG, SGE_EMA_LIST, event);
            break;
         case sgeE_CONFIG_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_CONFIG, SGE_EMA_ADD, event);
            break;
         case sgeE_CONFIG_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_CONFIG, SGE_EMA_DEL, event);
            break;
         case sgeE_CONFIG_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_CONFIG, SGE_EMA_MOD, event);
            break;

         case sgeE_EXECHOST_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_EXECHOST, SGE_EMA_LIST, event);
            break;
         case sgeE_EXECHOST_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_EXECHOST, SGE_EMA_ADD, event);
            break;
         case sgeE_EXECHOST_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_EXECHOST, SGE_EMA_DEL, event);
            break;
         case sgeE_EXECHOST_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_EXECHOST, SGE_EMA_MOD, event);
            break;

         case sgeE_GLOBAL_CONFIG:
            ret = sge_mirror_process_event(SGE_TYPE_GLOBAL_CONFIG, SGE_EMA_MOD, event);
            break;

         case sgeE_JATASK_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_JATASK, SGE_EMA_ADD, event);
            break;
         case sgeE_JATASK_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_JATASK, SGE_EMA_DEL, event);
            break;
         case sgeE_JATASK_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_JATASK, SGE_EMA_MOD, event);
            break;

         case sgeE_PETASK_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_PETASK, SGE_EMA_ADD, event);
            break;
         case sgeE_PETASK_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_PETASK, SGE_EMA_DEL, event);
            break;

         case sgeE_JOB_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_LIST, event);
            break;
         case sgeE_JOB_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_ADD, event);
            break;
         case sgeE_JOB_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_DEL, event);
            break;
         case sgeE_JOB_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_MOD, event);
            break;
         case sgeE_JOB_MOD_SCHED_PRIORITY:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_MOD, event);
            break;
         case sgeE_JOB_USAGE:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_MOD, event);
            break;
         case sgeE_JOB_FINAL_USAGE:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_MOD, event);
            break;
#if 0
         case sgeE_JOB_FINISH:
            ret = sge_mirror_process_event(SGE_TYPE_JOB, SGE_EMA_MOD, event);
            break;
#endif

         case sgeE_JOB_SCHEDD_INFO_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_JOB_SCHEDD_INFO, SGE_EMA_LIST, event);
            break;
         case sgeE_JOB_SCHEDD_INFO_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_JOB_SCHEDD_INFO, SGE_EMA_ADD, event);
            break;
         case sgeE_JOB_SCHEDD_INFO_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_JOB_SCHEDD_INFO, SGE_EMA_DEL, event);
            break;
         case sgeE_JOB_SCHEDD_INFO_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_JOB_SCHEDD_INFO, SGE_EMA_MOD, event);
            break;

         case sgeE_MANAGER_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_MANAGER, SGE_EMA_LIST, event);
            break;
         case sgeE_MANAGER_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_MANAGER, SGE_EMA_ADD, event);
            break;
         case sgeE_MANAGER_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_MANAGER, SGE_EMA_DEL, event);
            break;
         case sgeE_MANAGER_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_MANAGER, SGE_EMA_MOD, event);
            break;

         case sgeE_OPERATOR_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_OPERATOR, SGE_EMA_LIST, event);
            break;
         case sgeE_OPERATOR_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_OPERATOR, SGE_EMA_ADD, event);
            break;
         case sgeE_OPERATOR_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_OPERATOR, SGE_EMA_DEL, event);
            break;
         case sgeE_OPERATOR_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_OPERATOR, SGE_EMA_MOD, event);
            break;

         case sgeE_NEW_SHARETREE:
            ret = sge_mirror_process_event(SGE_TYPE_SHARETREE, SGE_EMA_LIST, event);
            break;

         case sgeE_PE_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_PE, SGE_EMA_LIST, event);
            break;
         case sgeE_PE_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_PE, SGE_EMA_ADD, event);
            break;
         case sgeE_PE_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_PE, SGE_EMA_DEL, event);
            break;
         case sgeE_PE_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_PE, SGE_EMA_MOD, event);
            break;

         case sgeE_PROJECT_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_PROJECT, SGE_EMA_LIST, event);
            break;
         case sgeE_PROJECT_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_PROJECT, SGE_EMA_ADD, event);
            break;
         case sgeE_PROJECT_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_PROJECT, SGE_EMA_DEL, event);
            break;
         case sgeE_PROJECT_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_PROJECT, SGE_EMA_MOD, event);
            break;

         case sgeE_QMASTER_GOES_DOWN:
            ret = sge_mirror_process_event(SGE_TYPE_QMASTER_GOES_DOWN, SGE_EMA_TRIGGER, event);
            break;
                                    
         case sgeE_QUEUE_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_LIST, event);
            break;
         case sgeE_QUEUE_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_ADD, event);
            break;
         case sgeE_QUEUE_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_DEL, event);
            break;
         case sgeE_QUEUE_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_MOD, event);
            break;
         case sgeE_QUEUE_SUSPEND_ON_SUB:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_MOD, event);
            break;
         case sgeE_QUEUE_UNSUSPEND_ON_SUB:
            ret = sge_mirror_process_event(SGE_TYPE_QUEUE, SGE_EMA_MOD, event);
            break;

         case sgeE_SCHED_CONF:
            ret = sge_mirror_process_event(SGE_TYPE_SCHEDD_CONF, SGE_EMA_MOD, event);
            break;

         case sgeE_SCHEDDMONITOR:
            ret = sge_mirror_process_event(SGE_TYPE_SCHEDD_MONITOR, SGE_EMA_TRIGGER, event);
            break;

         case sgeE_SHUTDOWN:
            ret = sge_mirror_process_event(SGE_TYPE_SHUTDOWN, SGE_EMA_TRIGGER, event);
            break;

         case sgeE_SUBMITHOST_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_SUBMITHOST, SGE_EMA_LIST, event);
            break;
         case sgeE_SUBMITHOST_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_SUBMITHOST, SGE_EMA_ADD, event);
            break;
         case sgeE_SUBMITHOST_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_SUBMITHOST, SGE_EMA_DEL, event);
            break;
         case sgeE_SUBMITHOST_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_SUBMITHOST, SGE_EMA_MOD, event);
            break;

         case sgeE_USER_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_USER, SGE_EMA_LIST, event);
            break;
         case sgeE_USER_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_USER, SGE_EMA_ADD, event);
            break;
         case sgeE_USER_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_USER, SGE_EMA_DEL, event);
            break;
         case sgeE_USER_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_USER, SGE_EMA_MOD, event);
            break;

         case sgeE_USERSET_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_USERSET, SGE_EMA_LIST, event);
            break;
         case sgeE_USERSET_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_USERSET, SGE_EMA_ADD, event);
            break;
         case sgeE_USERSET_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_USERSET, SGE_EMA_DEL, event);
            break;
         case sgeE_USERSET_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_USERSET, SGE_EMA_MOD, event);
            break;
   
#ifndef __SGE_NO_USERMAPPING__
         case sgeE_CUSER_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_CUSER, SGE_EMA_LIST, event);
            break;
         case sgeE_CUSER_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_CUSER, SGE_EMA_ADD, event);
            break;
         case sgeE_CUSER_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_CUSER, SGE_EMA_DEL, event);
            break;
         case sgeE_CUSER_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_CUSER, SGE_EMA_MOD, event);
            break;
#endif

         case sgeE_HGROUP_LIST:
            ret = sge_mirror_process_event(SGE_TYPE_HGROUP, SGE_EMA_LIST, event);
            break;
         case sgeE_HGROUP_ADD:
            ret = sge_mirror_process_event(SGE_TYPE_HGROUP, SGE_EMA_ADD, event);
            break;
         case sgeE_HGROUP_DEL:
            ret = sge_mirror_process_event(SGE_TYPE_HGROUP, SGE_EMA_DEL, event);
            break;
         case sgeE_HGROUP_MOD:
            ret = sge_mirror_process_event(SGE_TYPE_HGROUP, SGE_EMA_MOD, event);
            break;

         default:
            break;
      }

      /* if processing of an event failed, return an error to the caller of
       * this function, but continue processing.
       */
      if(ret != SGE_EM_OK) {
         function_ret = SGE_EM_PROCESS_ERRORS;
      }
   }

   DEXIT;
   return function_ret;
}

static sge_mirror_error sge_mirror_process_event(sge_object_type type, sge_event_action action, 
                                                 lListElem *event) 
{
   int ret;
   char buffer[1024];
   dstring buffer_wrapper;

   DENTER(TOP_LAYER, "sge_mirror_process_event");

   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   DPRINTF(("%s\n", event_text(event, &buffer_wrapper)));

   if(mirror_base[type].callback_before != NULL) {
      ret = mirror_base[type].callback_before(type, action, event, mirror_base[type].clientdata);
      if(!ret) {
         ERROR((SGE_EVENT, MSG_MIRROR_CALLBACKFAILED_S, event_text(event, &buffer_wrapper)));
         DEXIT;
         return SGE_EM_CALLBACK_FAILED;
      }
   }

   if(mirror_base[type].callback_default != NULL) {
      ret = mirror_base[type].callback_default(type, action, event, NULL);
      if(!ret) {
         ERROR((SGE_EVENT, MSG_MIRROR_CALLBACKFAILED_S, event_text(event, &buffer_wrapper)));
         DEXIT;
         return SGE_EM_CALLBACK_FAILED;
      }
   }

   if(mirror_base[type].callback_after != NULL) {
      ret = mirror_base[type].callback_after(type, action, event, mirror_base[type].clientdata);
      if(!ret) {
         ERROR((SGE_EVENT, MSG_MIRROR_CALLBACKFAILED_S, event_text(event, &buffer_wrapper)));
         DEXIT;
         return SGE_EM_CALLBACK_FAILED;
      }
   }

   DEXIT;
   return SGE_EM_OK;
}

static bool sge_mirror_process_shutdown(sge_object_type type, 
                                             sge_event_action action, 
                                             lListElem *event, 
                                             void *clientdata)
{
   DENTER(TOP_LAYER, "sge_mirror_process_shutdown");

   DPRINTF(("shutting down sge mirror\n"));
   sge_mirror_shutdown();
   SGE_EXIT(0);
   DEXIT;
   return true;
}

static bool sge_mirror_process_qmaster_goes_down(sge_object_type type, 
                                                 sge_event_action action, 
                                                 lListElem *event, 
                                                 void *clientdata)
{
   DENTER(TOP_LAYER, "sge_mirror_process_qmaster_goes_down");

   DPRINTF(("qmaster goes down\n"));
   sleep(8);
   ec_mark4registration();

   DEXIT;
   return true;
}

static bool 
generic_update_master_list(sge_object_type type, sge_event_action action,
                           lListElem *event, void *clientdata)
{
   lList **list;
   const lDescr *list_descr;
   int     key_nm;

   const char *key;

   DENTER(TOP_LAYER, "generic_update_master_list");

   list       = object_type_get_master_list(type);
   list_descr = object_type_get_descr(type);
   key_nm     = object_type_get_key_nm(type);

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return false;
   }

   DEXIT;
   return true;
}


/****** Eventmirror/sge_mirror_update_master_list_str_key() *********************
*  NAME
*     sge_mirror_update_master_list_str_key() -- update a master list
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_update_master_list_str_key(lList **list, 
*                                                            const lDescr *list_descr, 
*                                                            int key_nm, 
*                                                            const char *key, 
*                                                            sge_event_action action, 
*                                                            lListElem *event) 
*
*  FUNCTION
*     Updates a certain element in a certain mirrored list.
*     Which element to update is specified by a given string key.
*
*  INPUTS
*     lList **list            - the master list to update
*     lDescr *list_descr      - descriptor of the master list 
*     int key_nm              - field identifier of the key
*     const char *key         - value of the key
*     sge_event_action action - action to perform on list
*     lListElem *event        - raw event element
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/sge_mirror_update_master_list()
*******************************************************************************/
sge_mirror_error sge_mirror_update_master_list_str_key(lList **list, const lDescr *list_descr, 
                                                       int key_nm, const char *key, 
                                                       sge_event_action action, lListElem *event)
{
   lListElem *ep;
   sge_mirror_error ret;

   DENTER(TOP_LAYER, "sge_mirror_update_master_list_str_key");

   ep = lGetElemStr(*list, key_nm, key);
   ret = sge_mirror_update_master_list(list, list_descr, ep, key, action, event);

   DEXIT;
   return ret;
}

/****** Eventmirror/sge_mirror_update_master_list_host_key() *********************
*  NAME
*     sge_mirror_update_master_list_host_key() -- update a master list
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_update_master_list_host_key(lList **list, 
*                                                            const lDescr *list_descr, 
*                                                            int key_nm, 
*                                                            const char *key, 
*                                                            sge_event_action action, 
*                                                            lListElem *event) 
*
*  FUNCTION
*     Updates a certain element in a certain mirrored list.
*     Which element to update is specified by a given hostname.
*
*  INPUTS
*     lList **list            - the master list to update
*     lDescr *list_descr      - descriptor of the master list 
*     int key_nm              - field identifier of the key
*     const char *key         - value of the key (a hostname)
*     sge_event_action action - action to perform on list
*     lListElem *event        - raw event element
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK or an error code
*
*  SEE ALSO
*     Eventmirror/sge_mirror_update_master_list()
*******************************************************************************/
sge_mirror_error sge_mirror_update_master_list_host_key(lList **list, const lDescr *list_descr, 
                                                        int key_nm, const char *key, 
                                                        sge_event_action action, lListElem *event)
{
   lListElem *ep;
   sge_mirror_error ret;

   DENTER(TOP_LAYER, "sge_mirror_update_master_list_host_key");

   ep = lGetElemHost(*list, key_nm, key);
   ret = sge_mirror_update_master_list(list, list_descr, ep, key, action, event);

   DEXIT;
   return ret;
}

/****** Eventmirror/sge_mirror_update_master_list() *****************************
*  NAME
*     sge_mirror_update_master_list() -- update a master list
*
*  SYNOPSIS
*     sge_mirror_error sge_mirror_update_master_list(lList **list, 
*                                                    const lDescr *list_descr, 
*                                                    lListElem *ep, 
*                                                    const char *key, 
*                                                    sge_event_action action, 
*                                                    lListElem *event) 
*
*  FUNCTION
*     Updates a given master list according to the given event information.
*     The following actions are performed (depending on parameter action):
*     - SGE_EMA_LIST: an existing mirrored list is completely replaced
*     - SGE_EMA_ADD:  a new element is added to the mirrored list
*     - SGE_EMA_MOD:  a given element is modified
*     - SGE_EMA_DEL:  a given element is deleted
*
*  INPUTS
*     lList **list            - mirrored list to manipulate
*     lDescr *list_descr      - descriptor of mirrored list
*     lListElem *ep           - element to manipulate or NULL
*     const char *key         - key of an element to manipulate
*     sge_event_action action - action to perform
*     lListElem *event        - raw event
*
*  RESULT
*     sge_mirror_error - SGE_EM_OK, or an error code
*
*  SEE ALSO
*     Eventmirror/sge_mirror_update_master_list_str_key()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*******************************************************************************/
sge_mirror_error sge_mirror_update_master_list(lList **list, const lDescr *list_descr,
                                               lListElem *ep, const char *key, 
                                               sge_event_action action, lListElem *event)
{
   lList *data_list;

   DENTER(TOP_LAYER, "sge_mirror_update_master_list");
   switch(action) {
      case SGE_EMA_LIST:
         /* switch to new list */
         lXchgList(event, ET_new_version, list);
         break;

      case SGE_EMA_ADD:
         /* check for duplicate */
         if(ep != NULL) {
            ERROR((SGE_EVENT, "duplicate list element "SFQ"\n", key));
            DEXIT;
            return SGE_EM_DUPLICATE_KEY;
         }
   
         /* if neccessary, create list */
         if(*list == NULL) {
            *list = lCreateList("new master list", list_descr);
         }

         /* insert element */
         data_list = lGetList(event, ET_new_version);
         lAppendElem(*list, lDechainElem(data_list, lFirst(data_list)));
         break;

      case SGE_EMA_DEL:
         /* check for existence */
         if(ep == NULL) {
            ERROR((SGE_EVENT, "element "SFQ" does not exist\n", key));
            DEXIT;
            return SGE_EM_KEY_NOT_FOUND;
         }
         
         /* remove element */
         lRemoveElem(*list, ep);
         break;

      case SGE_EMA_MOD:
         /* check for existence */
         if(ep == NULL) {
            ERROR((SGE_EVENT, "element "SFQ" does not exist\n", key));
            DEXIT;
            return SGE_EM_KEY_NOT_FOUND;
         }

         lRemoveElem(*list, ep);
         data_list = lGetList(event, ET_new_version);
         lAppendElem(*list, lDechainElem(data_list, lFirst(data_list)));
         break;
      default:
         return SGE_EM_BAD_ARG;
   }      
   

   return SGE_EM_OK;
}

