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

#include "sge_eventL.h"
#include "sge_share_tree_nodeL.h"
#include "sge_jobL.h"
#include "cull.h"
#include "msg_schedd.h"
#include "sge_event.h"

/****** Eventclient/event_text() *************************************************
*  NAME
*     event_text() -- deliver event description
*
*  SYNOPSIS
*     const char* event_text(const lListElem *event) 
*
*  FUNCTION
*     Deliveres a short description of an event object.
*
*  INPUTS
*     const lListElem *event - the event to describe
*
*  RESULT
*     const char* - pointer to the descriptive string.
*
*  NOTES
*     The result points to a static buffer. Subsequent calls to event_text
*     will overwrite previous results.
*
*******************************************************************************/
const char *event_text(
const lListElem *event 
) {
   static char buffer[1024];
   u_long32 type, intkey, number, intkey2;
   int n=0;
   const char *strkey;
   lList *lp;

   number = lGetUlong(event, ET_number);
   type = lGetUlong(event, ET_type);
   intkey = lGetUlong(event, ET_intkey);
   intkey2 = lGetUlong(event, ET_intkey2);
   strkey = lGetString(event, ET_strkey);
   if ((lp=lGetList(event, ET_new_version)))
      n = lGetNumberOfElem(lp);


   switch (type) {

   /* -------------------- */
   case sgeE_ADMINHOST_LIST:
      sprintf(buffer, MSG_EVENT_ADMINHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_ADMINHOST_ADD:
      sprintf(buffer, MSG_EVENT_ADDADMINHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_ADMINHOST_DEL:
      sprintf(buffer, MSG_EVENT_DELADMINHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_ADMINHOST_MOD:
      sprintf(buffer, MSG_EVENT_MODADMINHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CALENDAR_LIST:
      sprintf(buffer, MSG_EVENT_CALENDARLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CALENDAR_ADD:
      sprintf(buffer, MSG_EVENT_ADDCALENDARX_IS, (int)number, strkey);
      break;
   case sgeE_CALENDAR_DEL:
      sprintf(buffer, MSG_EVENT_DELCALENDARX_IS, (int)number, strkey);
      break;
   case sgeE_CALENDAR_MOD:
      sprintf(buffer, MSG_EVENT_MODCALENDARX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CKPT_LIST:
      sprintf(buffer, MSG_EVENT_CKPTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CKPT_ADD:
      sprintf(buffer, MSG_EVENT_ADDCKPT_IS, (int)number, strkey);
      break;
   case sgeE_CKPT_DEL:
      sprintf(buffer, MSG_EVENT_DELCKPT_IS, (int)number, strkey);
      break;
   case sgeE_CKPT_MOD:
      sprintf(buffer, MSG_EVENT_MODCKPT_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_COMPLEX_LIST:
      sprintf(buffer, MSG_EVENT_COMPLEXLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_COMPLEX_ADD:
      sprintf(buffer, MSG_EVENT_ADDCOMPLEXX_IS, (int)number, strkey);
      break;
   case sgeE_COMPLEX_DEL:
      sprintf(buffer, MSG_EVENT_DELCOMPLEXX_IS, (int)number, strkey);
      break;
   case sgeE_COMPLEX_MOD:
      sprintf(buffer, MSG_EVENT_MODCOMPLEXX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_CONFIG_LIST:
      sprintf(buffer, MSG_EVENT_CONFIGLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_CONFIG_ADD:
      sprintf(buffer, MSG_EVENT_ADDCONFIGX_IS, (int)number, strkey);
      break;
   case sgeE_CONFIG_DEL:
      sprintf(buffer, MSG_EVENT_DELCONFIGX_IS, (int)number, strkey);
      break;
   case sgeE_CONFIG_MOD:
      sprintf(buffer, MSG_EVENT_MODCONFIGX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_EXECHOST_LIST:
      sprintf(buffer, MSG_EVENT_EXECHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_EXECHOST_ADD:
      sprintf(buffer, MSG_EVENT_ADDEXECHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_EXECHOST_DEL:
      sprintf(buffer, MSG_EVENT_DELEXECHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_EXECHOST_MOD:
      sprintf(buffer, MSG_EVENT_MODEXECHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_GLOBAL_CONFIG:
      sprintf(buffer, MSG_EVENT_GLOBAL_CONFIG_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_JATASK_DEL:
      sprintf(buffer, MSG_EVENT_DELJATASK_UUU, u32c (number), u32c(intkey), u32c(intkey2));
      break;
   case sgeE_JATASK_MOD:
      sprintf(buffer, MSG_EVENT_MODJATASK_UUU, u32c(number) , u32c(intkey) ,u32c(intkey2));
      break;

   /* -------------------- */
   case sgeE_PETASK_ADD:
      sprintf(buffer, MSG_EVENT_ADDPETASK_UUUS, u32c(number), u32c(intkey), u32c(intkey2), strkey);
      break;
   case sgeE_PETASK_DEL:
      sprintf(buffer, MSG_EVENT_DELPETASK_UUUS, u32c(number), u32c(intkey), u32c(intkey2), strkey);
      break;
#if 0      
   /* JG: we'll have it soon ;-) */
   case sgeE_PETASK_MOD:
      sprintf(buffer, MSG_EVENT_MODPETASK_UUUS, u32c(number), u32c(intkey), u32c(intkey2), strkey);
      break;
#endif

   /* -------------------- */
   case sgeE_JOB_LIST:
      sprintf(buffer, MSG_EVENT_JOBLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_JOB_ADD:
      sprintf(buffer, MSG_EVENT_ADDJOB_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_DEL:
      sprintf(buffer, MSG_EVENT_DELJOB_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_MOD:
      sprintf(buffer, MSG_EVENT_MODJOB_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_MOD_SCHED_PRIORITY:
      sprintf(buffer, MSG_EVENT_MODSCHEDDPRIOOFJOBXTOY_IDI, 
            (int)number, 
            u32c(intkey),
            ((int)lGetUlong(lFirst(lp), JB_priority))-BASE_PRIORITY);
      break;
   case sgeE_JOB_USAGE:
      sprintf(buffer, MSG_EVENT_JOBXUSAGE_II, 
         (int)number, (int)intkey);
      break;
   case sgeE_JOB_FINAL_USAGE:
      sprintf(buffer, MSG_EVENT_JOBXFINALUSAGE_II, 
         (int)number, (int)intkey);
      break;

   /* -------------------- */
   case sgeE_JOB_SCHEDD_INFO_LIST:
      sprintf(buffer, MSG_EVENT_JOB_SCHEDD_INFOLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_JOB_SCHEDD_INFO_ADD:
      sprintf(buffer, MSG_EVENT_ADDJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_SCHEDD_INFO_DEL:
      sprintf(buffer, MSG_EVENT_DELJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;
   case sgeE_JOB_SCHEDD_INFO_MOD:
      sprintf(buffer, MSG_EVENT_MODJOB_SCHEDD_INFO_III, (int)number, (int)intkey, (int)intkey2);
      break;

   /* -------------------- */
   case sgeE_MANAGER_LIST:
      sprintf(buffer, MSG_EVENT_MANAGERLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_MANAGER_ADD:
      sprintf(buffer, MSG_EVENT_ADDMANAGERX_IS, (int)number, strkey);
      break;
   case sgeE_MANAGER_DEL:
      sprintf(buffer, MSG_EVENT_DELMANAGERX_IS, (int)number, strkey);
      break;
   case sgeE_MANAGER_MOD:
      sprintf(buffer, MSG_EVENT_MODMANAGERX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_OPERATOR_LIST:
      sprintf(buffer, MSG_EVENT_OPERATORLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_OPERATOR_ADD:
      sprintf(buffer, MSG_EVENT_ADDOPERATORX_IS, (int)number, strkey);
      break;
   case sgeE_OPERATOR_DEL:
      sprintf(buffer, MSG_EVENT_DELOPERATORX_IS, (int)number, strkey);
      break;
   case sgeE_OPERATOR_MOD:
      sprintf(buffer, MSG_EVENT_MODOPERATORX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_NEW_SHARETREE:
      sprintf(buffer, MSG_EVENT_SHARETREEXNODESYLEAFS_III, (int)number, 
         lGetNumberOfNodes(NULL, lp, STN_children),
         lGetNumberOfLeafs(NULL, lp, STN_children));
      break;

   /* -------------------- */
   case sgeE_PE_LIST:
      sprintf(buffer, MSG_EVENT_PELISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_PE_ADD:
      sprintf(buffer, MSG_EVENT_ADDPEX_IS, (int)number, strkey);
      break;
   case sgeE_PE_DEL:
      sprintf(buffer, MSG_EVENT_DELPEX_IS, (int)number, strkey);
      break;
   case sgeE_PE_MOD:
      sprintf(buffer, MSG_EVENT_MODPEX_IS , (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_PROJECT_LIST:
      sprintf(buffer, MSG_EVENT_PROJECTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_PROJECT_ADD:
      sprintf(buffer, MSG_EVENT_ADDPROJECTX_IS, (int)number, strkey);
      break;
   case sgeE_PROJECT_DEL:
      sprintf(buffer, MSG_EVENT_DELPROJECTX_IS , (int)number, strkey);
      break;
   case sgeE_PROJECT_MOD:
      sprintf(buffer, MSG_EVENT_MODPROJECTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_QMASTER_GOES_DOWN:
      sprintf(buffer, MSG_EVENT_QMASTERGOESDOWN_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_QUEUE_LIST:
      sprintf(buffer, MSG_EVENT_QUEUELISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_QUEUE_ADD:
      sprintf(buffer, MSG_EVENT_ADDQUEUEX_IS, (int)number, strkey);
      break;
   case sgeE_QUEUE_DEL:
      sprintf(buffer, MSG_EVENT_DELQUEUEX_IS, (int)number, strkey);
      break;
   case sgeE_QUEUE_MOD:
      sprintf(buffer, MSG_EVENT_MODQUEUEX_IS, (int)number, strkey);
      break;
   case sgeE_QUEUE_UNSUSPEND_ON_SUB:
      sprintf(buffer, MSG_EVENT_UNSUSPENDQUEUEXONSUBORDINATE_IS, (int)number, strkey);
      break;
   case sgeE_QUEUE_SUSPEND_ON_SUB:
      sprintf(buffer, MSG_EVENT_SUSPENDQUEUEXONSUBORDINATE_IS , (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_SCHED_CONF:
      sprintf(buffer, MSG_EVENT_SCHEDULERCONFIG_I , (int)number);
      break;

   /* -------------------- */
   case sgeE_SCHEDDMONITOR:
      sprintf(buffer, MSG_EVENT_TRIGGERSCHEDULERMONITORING_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_SHUTDOWN:
      sprintf(buffer, MSG_EVENT_SHUTDOWN_I, (int)number);
      break;

   /* -------------------- */
   case sgeE_SUBMITHOST_LIST:
      sprintf(buffer, MSG_EVENT_SUBMITHOSTLISTXELEMENTS_II, (int)number, n);
      break;
   case sgeE_SUBMITHOST_ADD:
      sprintf(buffer, MSG_EVENT_ADDSUBMITHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_SUBMITHOST_DEL:
      sprintf(buffer, MSG_EVENT_DELSUBMITHOSTX_IS, (int)number, strkey);
      break;
   case sgeE_SUBMITHOST_MOD:
      sprintf(buffer, MSG_EVENT_MODSUBMITHOSTX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_USER_LIST:
      sprintf(buffer, MSG_EVENT_USERLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_USER_ADD:
      sprintf(buffer, MSG_EVENT_ADDUSERX_IS, (int)number, strkey);
      break;
   case sgeE_USER_DEL:
      sprintf(buffer, MSG_EVENT_DELUSERX_IS, (int)number, strkey);
      break;
   case sgeE_USER_MOD:
      sprintf(buffer, MSG_EVENT_MODUSERX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   case sgeE_USERSET_LIST:
      sprintf(buffer, MSG_EVENT_USERSETLISTXELEMENTS_II , (int)number, n);
      break;
   case sgeE_USERSET_ADD:
      sprintf(buffer, MSG_EVENT_ADDUSERSETX_IS , (int)number, strkey);
      break;
   case sgeE_USERSET_DEL:
      sprintf(buffer, MSG_EVENT_DELUSERSETX_IS, (int)number, strkey);
      break;
   case sgeE_USERSET_MOD:
      sprintf(buffer, MSG_EVENT_MODUSERSETX_IS, (int)number, strkey);
      break;

   /* -------------------- */
   default:
      sprintf(buffer, MSG_EVENT_NOTKNOWN_I, (int)number);
      break;
   }

   return buffer;
}

