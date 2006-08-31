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

#include "sgermon.h"
#include "sge_log.h"

#include "comm/cl_communication.h" /* CL_DEFINE_CLIENT_CONNECTION_LIFETIME */
#include "cull/cull.h"
#include "uti/sge_prog.h"
#include "uti/sge_string.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_sharetree.h"
#include "sgeobj/sge_event.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_object.h"
#include "sgeobj/sge_utility.h"

#include "msg_common.h"
#include "sgeobj/msg_sgeobjlib.h"

/* documentation see libs/evc/sge_event_client.c */
const char *event_text(const lListElem *event, dstring *buffer) 
{
   u_long32 type, intkey, number, intkey2;
   int n=0;
   const char *strkey, *strkey2;
   lList *lp;

   number = lGetUlong(event, ET_number);
   type = lGetUlong(event, ET_type);
   intkey = lGetUlong(event, ET_intkey);
   intkey2 = lGetUlong(event, ET_intkey2);
   strkey = lGetString(event, ET_strkey);
   strkey2 = lGetString(event, ET_strkey2);
   if ((lp=lGetList(event, ET_new_version))) {
      n = lGetNumberOfElem(lp);
   }

   switch (type) {

   /* -------------------- */
   case sgeE_ADMINHOST_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADMINHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_ADMINHOST_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDADMINHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_ADMINHOST_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELADMINHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_ADMINHOST_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODADMINHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CALENDAR_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_CALENDARLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CALENDAR_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDCALENDARX_IS, (int)number, strkey);
      break;
   case sgeE_CALENDAR_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELCALENDARX_IS, (int)number, strkey);
      break;
   case sgeE_CALENDAR_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODCALENDARX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CKPT_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_CKPTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CKPT_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDCKPT_IS, (int)number, strkey);
      break;
   case sgeE_CKPT_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELCKPT_IS, (int)number, strkey);
      break;
   case sgeE_CKPT_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODCKPT_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CONFIG_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_CONFIGLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CONFIG_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDCONFIGX_IS, (int)number, strkey);
      break;
   case sgeE_CONFIG_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELCONFIGX_IS, (int)number, strkey);
      break;
   case sgeE_CONFIG_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODCONFIGX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_EXECHOST_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_EXECHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_EXECHOST_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDEXECHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_EXECHOST_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELEXECHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_EXECHOST_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODEXECHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_GLOBAL_CONFIG:
      sge_dstring_sprintf(buffer, MSG_EVENT_GLOBAL_CONFIG_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_JATASK_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDJATASK_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JATASK_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELJATASK_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JATASK_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODJATASK_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;

   /* -------------------- */
   case sgeE_PETASK_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDPETASK_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_PETASK_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELPETASK_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
#if 0      
   /* JG: we'll have it soon ;-) */
   case sgeE_PETASK_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODPETASK_UUUS, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
#endif

   /* -------------------- */
   case sgeE_JOB_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_JOBLISTXELEMENTS_UI, sge_u32c(number), n);
      break;
   case sgeE_JOB_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDJOB_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JOB_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELJOB_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JOB_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODJOB_US, sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JOB_MOD_SCHED_PRIORITY:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODSCHEDDPRIOOFJOBXTOY_USI, 
            sge_u32c(number), 
            job_get_id_string(intkey, intkey2, strkey),
            ((int)lGetUlong(lFirst(lp), JB_priority))-BASE_PRIORITY);
      break;
   case sgeE_JOB_USAGE:
      sge_dstring_sprintf(buffer, MSG_EVENT_JOBXUSAGE_US, 
         sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;
   case sgeE_JOB_FINAL_USAGE:
      sge_dstring_sprintf(buffer, MSG_EVENT_JOBXFINALUSAGE_US, 
         sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;

   case sgeE_JOB_FINISH:
      sge_dstring_sprintf(buffer, MSG_EVENT_JOBXFINISH_US, 
         sge_u32c(number), job_get_id_string(intkey, intkey2, strkey));
      break;

   /* -------------------- */
   case sgeE_JOB_SCHEDD_INFO_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_JOB_SCHEDD_INFOLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_JOB_SCHEDD_INFO_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_SCHEDD_INFO_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_SCHEDD_INFO_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;

   /* -------------------- */
   case sgeE_MANAGER_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_MANAGERLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_MANAGER_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDMANAGERX_IS, (int)number, strkey);
      break;
   case sgeE_MANAGER_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELMANAGERX_IS, (int)number, strkey);
      break;
   case sgeE_MANAGER_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODMANAGERX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_OPERATOR_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_OPERATORLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_OPERATOR_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDOPERATORX_IS, (int)number, strkey);
      break;
   case sgeE_OPERATOR_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELOPERATORX_IS, (int)number, strkey);
      break;
   case sgeE_OPERATOR_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODOPERATORX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_NEW_SHARETREE:
      sge_dstring_sprintf(buffer, MSG_EVENT_SHARETREEXNODESYLEAFS_III, (int)number, 
         lGetNumberOfNodes(NULL, lp, STN_children),
         lGetNumberOfLeafs(NULL, lp, STN_children));
      break;

   /* -------------------- */
   case sgeE_PE_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_PELISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_PE_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDPEX_IS, (int)number, strkey);
      break;
   case sgeE_PE_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELPEX_IS, (int)number, strkey);
      break;
   case sgeE_PE_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODPEX_IS , (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_PROJECT_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_PROJECTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_PROJECT_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDPROJECTX_IS, (int)number, strkey);
      break;
   case sgeE_PROJECT_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELPROJECTX_IS , (int)number, strkey);
      break;
   case sgeE_PROJECT_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODPROJECTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_QMASTER_GOES_DOWN:
      sge_dstring_sprintf(buffer, MSG_EVENT_QMASTERGOESDOWN_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_CQUEUE_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_CQUEUELISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CQUEUE_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDCQUEUEX_IS, (int)number, strkey);
      break;
   case sgeE_CQUEUE_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELCQUEUEX_IS, (int)number, strkey);
      break;
   case sgeE_CQUEUE_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODCQUEUEX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_QINSTANCE_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDQINSTANCE_ISS, (int)number, strkey, strkey2);
      break;
   case sgeE_QINSTANCE_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELQINSTANCE_ISS, (int)number, strkey, strkey2);
      break;
   case sgeE_QINSTANCE_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODQINSTANCE_ISS, (int)number, strkey, strkey2);
      break;
   case sgeE_QINSTANCE_USOS:
      sge_dstring_sprintf(buffer, MSG_EVENT_UNSUSPENDQUEUEXONSUBORDINATE_IS, (int)number, strkey);
      break;
   case sgeE_QINSTANCE_SOS:
      sge_dstring_sprintf(buffer, MSG_EVENT_SUSPENDQUEUEXONSUBORDINATE_IS , (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_SCHED_CONF:
      sge_dstring_sprintf(buffer, MSG_EVENT_SCHEDULERCONFIG_I , (int)number);
      break;

   /* -------------------- */
   case sgeE_SCHEDDMONITOR:
      sge_dstring_sprintf(buffer, MSG_EVENT_TRIGGERSCHEDULERMONITORING_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_SHUTDOWN:
      sge_dstring_sprintf(buffer, MSG_EVENT_SHUTDOWN_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_SUBMITHOST_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_SUBMITHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_SUBMITHOST_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDSUBMITHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_SUBMITHOST_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELSUBMITHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_SUBMITHOST_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODSUBMITHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_USER_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_USERLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_USER_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDUSERX_IS, (int)number, strkey);
      break;
   case sgeE_USER_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELUSERX_IS, (int)number, strkey);
      break;
   case sgeE_USER_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODUSERX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_USERSET_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_USERSETLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_USERSET_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDUSERSETX_IS , (int)number, strkey);
      break;
   case sgeE_USERSET_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELUSERSETX_IS, (int)number, strkey);
      break;
   case sgeE_USERSET_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODUSERSETX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_HGROUP_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_HGROUPLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_HGROUP_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDHGROUPX_IS , (int)number, strkey);
      break;
   case sgeE_HGROUP_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELHGROUPX_IS, (int)number, strkey);
      break;
   case sgeE_HGROUP_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODHGROUPX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CENTRY_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_CENTRYLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_CENTRY_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDCENTRYX_IS , (int)number, strkey);
      break;
   case sgeE_CENTRY_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELCENTRYX_IS, (int)number, strkey);
      break;
   case sgeE_CENTRY_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODCENTRYX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_LIRS_LIST:
      sge_dstring_sprintf(buffer, MSG_EVENT_LIRSLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_LIRS_ADD:
      sge_dstring_sprintf(buffer, MSG_EVENT_ADDLIRSX_IS, (int)number, strkey);
      break;
   case sgeE_LIRS_DEL:
      sge_dstring_sprintf(buffer, MSG_EVENT_DELLIRSX_IS, (int)number, strkey);
      break;
   case sgeE_LIRS_MOD:
      sge_dstring_sprintf(buffer, MSG_EVENT_MODLIRSX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   default:
      sge_dstring_sprintf(buffer, MSG_EVENT_NOTKNOWN_I, (int)number);
      break;
   }

   return sge_dstring_get_string(buffer);
}

static bool event_client_verify_subscription(const lListElem *event_client, lList **answer_list, int d_time)
{
   bool ret = true;
   const lListElem *ep;

   DENTER(TOP_LAYER, "event_client_verify_subscription");

   for_each (ep, lGetList(event_client, EV_subscribed)) {
      if (ret) {
         u_long32 id = lGetUlong(ep, EVS_id);
         if (id <= sgeE_ALL_EVENTS || id >= sgeE_EVENTSIZE) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                    MSG_EVENT_INVALIDEVENT);
            ret = false;
            break;
         }
      }

#if 0
      /* check flush interval - we get a -1 out of the ulong (!) in case
       * flushing is disabled
       * We can very easily run into this problem by configuring scheduler interval
       * and flush_submit|finish_secs. Disabling the check.
       */
      if (ret) {
         int interval = (int) lGetUlong(ep, EVS_interval);
         if (interval > d_time) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                    MSG_EVENT_FLUSHDELAYCANNOTBEGTDTIME);
            ret = false;
            break;
         }
      }
#endif
   }

   /* TODO: verify the where and what elements */

   DRETURN(ret);
}

/****** sge_event/event_client_verify() ****************************************
*  NAME
*     event_client_verify() -- verify an event client object
*
*  SYNOPSIS
*     bool 
*     event_client_verify(const lListElem *event_client, lList **answer_list) 
*
*  FUNCTION
*     Verifies, if an incoming event client object (new event client registration
*     through a GDI_ADD operation or event client modification through GDI_MOD
*     operation is correct.
*
*     We do the following verifications:
*        - EV_id correct:
*           - add request usually may only request dynamic event client id, 
*             if a special id is requested, we must be on local host and be
*             admin user or root.
*        - EV_name (valid string, limited length)
*        - EV_d_time (valid delivery interval)
*        - EV_flush_delay (valid flush delay)
*        - EV_subscribed (valid subscription list)
*        - EV_busy_handling (valid busy handling)
*        - EV_session (valid string, limited length)
*
*     No verification will be done
*        - EV_host (is always overwritten by qmaster code)
*        - EV_commproc (comes from commlib)
*        - EV_commid (comes from commlib)
*        - EV_uid (is always overwritten  by qmaster code)
*        - EV_last_heard_from (only used by qmaster)
*        - EV_last_send_time (only used by qmaster)
*        - EV_next_send_time (only used by qmaster)
*        - EV_sub_array (only used by qmaster)
*        - EV_changed (?)
*        - EV_next_number (?)
*        - EV_state (?)
*
*  INPUTS
*     const lListElem *event_client - ??? 
*     lList **answer_list           - ??? 
*     bool add                      - is this an add request (or mod)?
*
*  RESULT
*     bool - 
*
*  NOTES
*     MT-NOTE: event_client_verify() is MT safe 
*******************************************************************************/
bool 
event_client_verify(const lListElem *event_client, lList **answer_list, bool add)
{
   bool ret = true;
   const char *str;
   u_long d_time = 0;
  
   DENTER(TOP_LAYER, "event_client_verify");

   if (event_client == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                              MSG_NULLELEMENTPASSEDTO_S, SGE_FUNC);
      ret = false;
   }

   if (ret) {
      if (!object_verify_cull(event_client, EV_Type)) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_OBJECT_STRUCTURE_ERROR);
         ret = false;
      }
   }

   if (ret) {
      /* get the event delivery time - we'll need it in later checks */
      d_time = lGetUlong(event_client, EV_d_time);

      /* 
       * EV_name has to be a valid string.
       * TODO: Is verify_str_key too restrictive? Does drmaa allow to set the name?
       */
      str = lGetString(event_client, EV_name);
      if (str == NULL ||
         verify_str_key(answer_list, str, MAX_VERIFY_STRING, lNm2Str(EV_name)) != STATUS_OK) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_EVENT_INVALIDNAME);
         ret = false;
      }
   }

   /* 
    * Verify the EV_id:
    * Add requests may only contain EV_ID_ANY or a special id.
    * If a special id is requested, it must come from admin/root
    */
   if (ret) {
      u_long32 id = lGetUlong(event_client, EV_id);
      if (add && id >= EV_ID_FIRST_DYNAMIC) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_EVENT_INVALIDNAME);
         ret = false;
#if 0
      /* 
       * useless check - EV_uid is set by client 
       * is checked by CSP for special clients
       */
      } else if (id != EV_ID_ANY) {
         u_long32 uid = lGetUlong(event_client, EV_uid);
         u_long32 my_uid = uti_state_get_uid();
         if (uid != my_uid && uid != 0) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                    MSG_EVENT_ONLYADMINMAYSTARTSPECIALEVC);
            ret = false;
         }
#endif
      }
   }

   /* Event delivery time may not be gt commlib timeout */
   if (ret) {
      if (d_time < 1 || d_time > CL_DEFINE_CLIENT_CONNECTION_LIFETIME-5) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_EVENT_INVALIDDTIME_II, d_time,
                                 CL_DEFINE_CLIENT_CONNECTION_LIFETIME-5);
         ret = false;
      }
   }

#if 0
   /* 
    * flush delay cannot be gt event delivery time 
    * We can very easily run into this problem by configuring scheduler interval
    * and flush_submit|finish_secs. Disabling the check.
    */
   if (ret) {
      if (lGetUlong(event_client, EV_flush_delay) > d_time) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_EVENT_FLUSHDELAYCANNOTBEGTDTIME);
         ret = false;
      }
   }
#endif

   /* subscription */
   if (ret) {
      ret = event_client_verify_subscription(event_client, answer_list, (int)d_time);
   }

   /* busy handling comes from an enum with defined set of values */
   if (ret) {
      u_long32 busy = lGetUlong(event_client, EV_busy_handling);
      if (busy != EV_BUSY_NO_HANDLING && busy != EV_BUSY_UNTIL_ACK &&
          busy != EV_BUSY_UNTIL_RELEASED && busy != EV_THROTTLE_FLUSH) {
         answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                 MSG_EVENT_INVALIDBUSYHANDLING);
         ret = false;
      }
   }

   /* verify session key. TODO: verify_str_key too restrictive? */
   if (ret) {
      const char *session = lGetString(event_client, EV_session);
      if (session != NULL) {
         if (verify_str_key(answer_list, session, MAX_VERIFY_STRING, "session key") 
                            != STATUS_OK) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR, 
                                    MSG_EVENT_INVALIDSESSIONKEY);
            ret = false;
         }
      }
   }

   DRETURN(ret);
}
