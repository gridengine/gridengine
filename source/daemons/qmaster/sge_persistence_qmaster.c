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

#include "basis_types.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"

#include "sge_bootstrap.h"

#include "sge_answer.h"
#include "sge_event.h"
#include "sge_object.h"
#include "sge_job.h"

#include "sge_event_master.h"

#include "spool/sge_spooling.h"
#include "spool/dynamic/sge_spooling_loader.h"

#include "time_event.h"

#include "sge_persistence_qmaster.h"

bool
sge_initialize_persistence(lList **answer_list)
{
   bool ret = true;

   lListElem *spooling_context;

   DENTER(TOP_LAYER, "sge_initialize_persistence");

   /* create spooling context */
   spooling_context = spool_create_dynamic_context(answer_list, 
                           bootstrap_get_spooling_lib(), 
                           bootstrap_get_spooling_params());
   if (spooling_context == NULL) {
      /* error message created in spool_create_dynamic_context */
      ret = false;
   } else {
      /* startup spooling context */
      if (!spool_startup_context(answer_list, spooling_context, true)) {
         /* error message created in spool_startup_context */
         ret = false;
      } else {
         time_t now = time(0);

         /* set this context as default */
         spool_set_default_context(spooling_context);

         /* initialize timer for spooling trigger function */
         te_add(TYPE_SPOOLING_TRIGGER, now, 0, 0, NULL);
      }
   }

   DEXIT;
   return ret;
}

bool
sge_shutdown_persistence(lList **answer_list)
{
   bool ret = true;

   lListElem *context;

   DENTER(TOP_LAYER, "sge_shutdown_persistence");

   /* trigger spooling actions (flush data) */
   deliver_spooling_trigger(TYPE_SPOOLING_TRIGGER, 0, 0, 0, NULL);

   /* shutdown spooling */
   context = spool_get_default_context();
   if (context != NULL) {
      lList *local_answer = NULL;

      if (answer_list != NULL) {
         local_answer = *answer_list;
      }

      spool_shutdown_context(&local_answer, context);
      if (answer_list == NULL) {
         answer_list_output(&local_answer);
      }
   }

   DEXIT;
   return ret;
}

void
deliver_spooling_trigger(u_long32 type, u_long32 when, 
                         u_long32 uval0, u_long32 uval1, const char *key)
{
   time_t next_trigger = 0;
   time_t now;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "deliver_spooling_trigger");

   /* trigger spooling regular actions */
   if (!spool_trigger_context(&answer_list, spool_get_default_context(), 
                              when, &next_trigger)) {
      answer_list_output(&answer_list);
   }

   /* validate next_trigger. If it is invalid, set it to one minute after now */
   now = time(0);
   if (next_trigger <= now) {
      next_trigger = now + 60;
   }

   /* set timerevent for next trigger */
   te_add(type, next_trigger, 0, 0, NULL);

   DEXIT;
   return;
}

/****** sge_persistence_qmaster/sge_event_spool() ******************************
*  NAME
*     sge_event_spool() -- send event and spool
*
*  SYNOPSIS
*     bool 
*     sge_event_spool(lList **answer_list, u_long32 timestamp, ev_event event, 
*                     u_long32 intkey1, u_long32 intkey2, const char *strkey, 
*                     const char *strkey2, const char *session,
*                     lListElem *object, lListElem *sub_object1, 
*                     lListElem *sub_object2, bool send_event, bool spool) 
*
*  FUNCTION
*     Spools (writes or deletes) an object.
*     If spooling was successfull, send the given event.
*     Finally, the changed bits (information, which fields of the object
*     and it's subobjects were changed) is cleared.
*
*  INPUTS
*     lList **answer_list    - to return error messages
*     u_long32 timestamp     - timestamp of object change, if 0 is passed,
*                              use current date/time
*     ev_event event         - the event to send
*     u_long32 intkey1       - an integer key (job_id)
*     u_long32 intkey2       - a second integer key (ja_task_id)
*     const char *strkey     - a string key (all other keys)
*     const char *strkey2    - a string key (all other keys)
*     const char *session    - events session key
*     lListElem *object      - the object to spool and send
*     lListElem *sub_object1 - optionally a sub object (ja_task)
*     lListElem *sub_object2 - optionally a sub sub object (pe_task)
*     bool send_event        - shall we send an event, or only spool?
*     bool spool             - shall we spool or only send an event?
*
*  RESULT
*     bool - true on success, 
*            false on error. answer_list will contain an error description 
*  NOTES
*     From an academic standpoint, the parameter spool shouldn't be needed.
*     Whenever an object changes and a change event is created, the data 
*     basis should also be updated (spooled).
*
*  BUGS
*
*  SEE ALSO
*     
*******************************************************************************/
bool 
sge_event_spool(lList **answer_list, u_long32 timestamp, ev_event event, 
                u_long32 intkey1, u_long32 intkey2, const char *strkey, 
                const char *strkey2, const char *session, 
                lListElem *object, lListElem *sub_object1, 
                lListElem *sub_object2, bool send_event, bool spool)
{
   bool ret = true;
  
   const char *key = NULL;
   sge_object_type object_type;
   lListElem *element = NULL;
   bool delete = false;

   switch (event) {
      case sgeE_ADMINHOST_LIST:
      case sgeE_ADMINHOST_ADD:
      case sgeE_ADMINHOST_DEL:
      case sgeE_ADMINHOST_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_ADMINHOST;
         break;
      case sgeE_CALENDAR_LIST:
      case sgeE_CALENDAR_ADD:
      case sgeE_CALENDAR_DEL:
      case sgeE_CALENDAR_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CALENDAR;
         break;
      case sgeE_CKPT_LIST:
      case sgeE_CKPT_ADD:
      case sgeE_CKPT_DEL:
      case sgeE_CKPT_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CKPT;
         break;
      case sgeE_CENTRY_LIST:
      case sgeE_CENTRY_ADD:
      case sgeE_CENTRY_DEL:
      case sgeE_CENTRY_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CENTRY;
         break;
      case sgeE_CONFIG_LIST:
      case sgeE_CONFIG_ADD:
      case sgeE_CONFIG_DEL:
      case sgeE_CONFIG_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CONFIG;
         break;
      case sgeE_EXECHOST_LIST:
      case sgeE_EXECHOST_ADD:
      case sgeE_EXECHOST_DEL:
      case sgeE_EXECHOST_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_EXECHOST;
         break;
      case sgeE_GLOBAL_CONFIG:
         key = strkey;
         element = object;
         /* nothing to spool for this event */
         object_type = SGE_TYPE_ALL;
         break;
      case sgeE_JATASK_ADD:
      case sgeE_JATASK_DEL:
      case sgeE_JATASK_MOD:
         key = job_get_key(intkey1, intkey2, strkey);
         element = sub_object1;
         object_type = SGE_TYPE_JATASK;
         break;
      case sgeE_PETASK_ADD:
      case sgeE_PETASK_DEL:
         key = job_get_key(intkey1, intkey2, strkey);
         element = sub_object2;
         object_type = SGE_TYPE_PETASK;
         break;
      case sgeE_JOB_LIST:
      case sgeE_JOB_ADD:
      case sgeE_JOB_DEL:
      case sgeE_JOB_MOD:
      case sgeE_JOB_MOD_SCHED_PRIORITY:
      case sgeE_JOB_USAGE:
      case sgeE_JOB_FINAL_USAGE:
      case sgeE_JOB_FINISH:
         key = job_get_key(intkey1, intkey2, strkey);
         element = object;
         object_type = SGE_TYPE_JOB;
         break;
      case sgeE_JOB_SCHEDD_INFO_LIST:
      case sgeE_JOB_SCHEDD_INFO_ADD:
      case sgeE_JOB_SCHEDD_INFO_DEL:
      case sgeE_JOB_SCHEDD_INFO_MOD:
         key = job_get_key(intkey1, intkey2, strkey);
         element = object;
         object_type = SGE_TYPE_JOB_SCHEDD_INFO;
         break;
      case sgeE_MANAGER_LIST:
      case sgeE_MANAGER_ADD:
      case sgeE_MANAGER_DEL:
      case sgeE_MANAGER_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_MANAGER;
         break;
      case sgeE_OPERATOR_LIST:
      case sgeE_OPERATOR_ADD:
      case sgeE_OPERATOR_DEL:
      case sgeE_OPERATOR_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_OPERATOR;
         break;
      case sgeE_NEW_SHARETREE:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_SHARETREE;
         break;
      case sgeE_PE_LIST:
      case sgeE_PE_ADD:
      case sgeE_PE_DEL:
      case sgeE_PE_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_PE;
         break;
      case sgeE_PROJECT_LIST:
      case sgeE_PROJECT_ADD:
      case sgeE_PROJECT_DEL:
      case sgeE_PROJECT_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_PROJECT;
         break;
      case sgeE_QMASTER_GOES_DOWN:
         key = strkey;
         element = object;
         /* nothing to spool for this event */
         object_type = SGE_TYPE_ALL;
         break;
      case sgeE_QUEUE_LIST:
      case sgeE_QUEUE_ADD:
      case sgeE_QUEUE_DEL:
      case sgeE_QUEUE_MOD:
      case sgeE_QUEUE_SUSPEND_ON_SUB:
      case sgeE_QUEUE_UNSUSPEND_ON_SUB:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_QUEUE;
         break;
      case sgeE_CQUEUE_LIST:
      case sgeE_CQUEUE_ADD:
      case sgeE_CQUEUE_DEL:
      case sgeE_CQUEUE_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CQUEUE;
         break;
      case sgeE_QINSTANCE_DEL:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_QINSTANCE;
         break;
      case sgeE_SCHED_CONF:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_SCHEDD_CONF;
         break;
      case sgeE_SCHEDDMONITOR:
         key = strkey;
         element = object;
         /* nothing to spool for this event */
         object_type = SGE_TYPE_ALL;
         break;
      case sgeE_SHUTDOWN:
         key = strkey;
         element = object;
         /* nothing to spool for this event */
         object_type = SGE_TYPE_ALL;
         break;
      case sgeE_SUBMITHOST_LIST:
      case sgeE_SUBMITHOST_ADD:
      case sgeE_SUBMITHOST_DEL:
      case sgeE_SUBMITHOST_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_SUBMITHOST;
         break;
      case sgeE_USER_LIST:
      case sgeE_USER_ADD:
      case sgeE_USER_DEL:
      case sgeE_USER_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_USER;
         break;
      case sgeE_USERSET_LIST:
      case sgeE_USERSET_ADD:
      case sgeE_USERSET_DEL:
      case sgeE_USERSET_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_USERSET;
         break;
#ifndef __SGE_NO_USERMAPPING__
      case sgeE_CUSER_LIST:
      case sgeE_CUSER_ADD:
      case sgeE_CUSER_DEL:
      case sgeE_CUSER_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_CUSER;
         break;
#endif
      case sgeE_HGROUP_LIST:
      case sgeE_HGROUP_ADD:
      case sgeE_HGROUP_DEL:
      case sgeE_HGROUP_MOD:
         key = strkey;
         element = object;
         object_type = SGE_TYPE_HGROUP;
         break;

      default:
         /* nothing to spool */
         object_type = SGE_TYPE_ALL;
         ret = false;
         break;
   }
  
   /* only continue in case of valid event */
   if (ret) {
      switch (event) {
         case sgeE_ADMINHOST_DEL:
         case sgeE_CALENDAR_DEL:
         case sgeE_CKPT_DEL:
         case sgeE_CENTRY_DEL:
         case sgeE_CONFIG_DEL:
         case sgeE_EXECHOST_DEL:
         case sgeE_JATASK_DEL:
         case sgeE_PETASK_DEL:
         case sgeE_JOB_DEL:
         case sgeE_JOB_SCHEDD_INFO_DEL:
         case sgeE_MANAGER_DEL:
         case sgeE_OPERATOR_DEL:
         case sgeE_PE_DEL:
         case sgeE_PROJECT_DEL:
         case sgeE_QUEUE_DEL:
         case sgeE_CQUEUE_DEL:
         case sgeE_QINSTANCE_DEL:
         case sgeE_SUBMITHOST_DEL:
         case sgeE_USER_DEL:
         case sgeE_USERSET_DEL:
#ifndef __SGE_NO_USERMAPPING__
         case sgeE_CUSER_DEL:
#endif
         case sgeE_HGROUP_DEL:
            delete = true;
            break;
         case sgeE_NEW_SHARETREE:
            if (object == NULL) {
               delete = true;
            }
            break;
         default:
            delete = false;
            break;
      }

      /* if spooling was requested and we have an object type to spool */
      if (spool && object_type != SGE_TYPE_ALL) {
         if (delete) {
            ret = spool_delete_object(answer_list, spool_get_default_context(), object_type, key);
         } else {
            ret = spool_write_object(answer_list, spool_get_default_context(), 
                                     object, key, object_type);
         }
      }
   }

   /* send event only, if spooling succeeded */
   if (ret) {
      if (send_event) {
         sge_add_event(NULL, timestamp, event, 
                       intkey1, intkey2, strkey, strkey2,
                       session, element);
      }

      /* clear the changed bits */
      lListElem_clear_changed_info(object);
   }

   return ret;
}

