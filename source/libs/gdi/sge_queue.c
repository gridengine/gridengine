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

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"
#include "sge_job.h"
#include "sge_manop.h"
#include "sge_userset.h"
#include "sge_event.h"
#include "sge_m_event.h"
#include "sge_answer.h"
#include "msg_gdilib.h"

#include "sge_queue.h"

lList *Master_Queue_List = NULL;

void queue_or_job_get_states(int nm, char *str, u_long32 op)
{
   int count = 0;

   DENTER(TOP_LAYER, "queue_or_job_get_states");

   if (nm==QU_qname) {
      if (VALID(QALARM, op))
         str[count++] = ALARM_SYM;
      if (VALID(QSUSPEND_ALARM, op))
         str[count++] = SUSPEND_ALARM_SYM;
      if (VALID(QCAL_SUSPENDED, op))
         str[count++] = SUSPENDED_ON_CALENDAR_SYM;
      if (VALID(QCAL_DISABLED, op))
         str[count++] = DISABLED_ON_CALENDAR_SYM;
      if (VALID(QDISABLED, op))
         str[count++] = DISABLED_SYM;
      if (!VALID(!QDISABLED, op))
         str[count++] = ENABLED_SYM;
      if (VALID(QUNKNOWN, op))
         str[count++] = UNKNOWN_SYM;
      if (VALID(QERROR, op))
         str[count++] = ERROR_SYM;
      if (VALID(QSUSPENDED_ON_SUBORDINATE, op))
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }

   if (nm==JB_job_number) {
      if (VALID(JDELETED, op))
         str[count++] = DISABLED_SYM;
      if (VALID(JERROR, op))
         str[count++] = ERROR_SYM;
      if (VALID(JSUSPENDED_ON_SUBORDINATE, op))
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }

   if (VALID(JSUSPENDED_ON_THRESHOLD, op)) {
      str[count++] = SUSPENDED_ON_THRESHOLD_SYM;
   }

   if (VALID(JHELD, op)) {
      str[count++] = HELD_SYM;
   }

   if (VALID(JMIGRATING, op)) {
      str[count++] = RESTARTING_SYM;
   }

   if (VALID(JQUEUED, op)) {
      str[count++] = QUEUED_SYM;
   }

   if (VALID(JRUNNING, op)) {
      str[count++] = RUNNING_SYM;
   }

   if (VALID(JSUSPENDED, op)) {
      str[count++] = SUSPENDED_SYM;
   }

   if (VALID(JTRANSFERING, op)) {
      str[count++] = TRANSISTING_SYM;
   }

   if (VALID(JWAITING, op)) {
      str[count++] = WAITING_SYM;
   }

   if (VALID(JEXITING, op)) {
      str[count++] = EXITING_SYM;
   }

   str[count++] = '\0';

   DEXIT;
   return;
}

/****** gdi/queue/queue_get_state_string() ************************************
*  NAME
*     queue_get_state_string() -- write queue state flags into a string 
*
*  SYNOPSIS
*     void queue_get_state_string(char *str, u_long32 op) 
*
*  FUNCTION
*     This function writes the state flags given by 'op' into the
*     string 'str'                                     
*
*  INPUTS
*     char *str   - containes the state flags for 'qstat'/'qhost' 
*     u_long32 op - queue state bitmask 
******************************************************************************/
void queue_get_state_string(char *str, u_long32 op)
{
   queue_or_job_get_states(QU_qname, str, op);
}

/****** gdi/queue/queue_list_locate() ******************************************
*  NAME
*     queue_list_locate() -- locate queue given by name 
*
*  SYNOPSIS
*     lListElem* queue_list_locate(lList *queue_list, 
*                                  const char *queue_name) 
*
*  FUNCTION
*     Finds and returnis the queue with name "queue_name" in "queue_list".
*
*  INPUTS
*     lList *queue_list      - QU_Type list 
*     const char *queue_name - name of the queue 
*
*  RESULT
*     lListElem* - pointer to a QU_Type element or NULL
*******************************************************************************/
lListElem *queue_list_locate(lList *queue_list, const char *queue_name) 
{
   return lGetElemStr(queue_list, QU_qname, queue_name);
}

/****** gdi/queue/queue_list_set_tag() *****************************************
*  NAME
*     queue_list_set_tag() -- change the QU_tagged of (all) queues 
*
*  SYNOPSIS
*     void queue_list_set_tag(lList *queue_list, 
*                             queue_tag_t flags, 
*                             u_long32 tag_value) 
*
*  FUNCTION
*     Change the value of the QU_tagged attribute for all queues contained 
*     in "queue_list" to the value "tag_value". "flags" might be specified 
*     to ignore certain queues.
*
*  INPUTS
*     lList *queue_list  - QU_Type list 
*     queue_tag_t flags  - e.g. QUEUE_TAG_IGNORE_TEMPLATE 
*     u_long32 tag_value - new value for the attribute 
*
*  RESULT
*     void - None
*******************************************************************************/
void queue_list_set_tag(lList *queue_list,
                        queue_tag_t flags,
                        u_long32 tag_value)
{
   int ignore_template = flags & QUEUE_TAG_IGNORE_TEMPLATE;
   lListElem *queue = NULL;

   for_each(queue, queue_list) {
      const char *queue_name = lGetString(queue, QU_qname);

      if (ignore_template && !strcmp(queue_name, SGE_TEMPLATE_NAME)) {
         continue;
      }

      lSetUlong(queue, QU_tagged, tag_value);
   }
}

/****** gdi/queue/queue_list_clear_tags() *************************************
*  NAME
*     queue_list_clear_tags() -- clear the QU_tagged field
*
*  SYNOPSIS
*     void queue_list_clear_tags(lList *queue_list)
*
*  FUNCTION
*     Clear the QU_tagged field of all queues contained in "queue_list".
*
*  INPUTS
*     lList *queue_list - QU_Type list
*
*  RESULT
*     void - None
*******************************************************************************/
void queue_list_clear_tags(lList *queue_list)
{
   queue_list_set_tag(queue_list, QUEUE_TAG_DEFAULT, 0);
} 

/****** gdi/queue/queue_update_master_list() *****************************
*  NAME
*     queue_update_master_list() -- update the master list of queues
*
*  SYNOPSIS
*     int queue_update_master_list(sge_event_type type, 
*                                  sge_event_action action, 
*                                  lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of queues
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int queue_update_master_list(sge_event_type type, sge_event_action action, 
                             lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   lListElem *queue;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "queue_update_master_list");

   list = &Master_Queue_List;
   list_descr = QU_Type;
   key_nm = QU_qname;

   key = lGetString(event, ET_strkey);

   queue = queue_list_locate(*list, key);

   if(action == SGE_EMA_MOD) {
      u_long32 type = lGetUlong(event, ET_type);

      if(type == sgeE_QUEUE_SUSPEND_ON_SUB || type == sgeE_QUEUE_UNSUSPEND_ON_SUB) {
         u_long32 state;

         if(queue == NULL) {
            ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEFORUPDATEIN_SS, key, "queue_update_master_list"));
            DEXIT;
            return FALSE;
         }

         state = lGetUlong(queue, QU_state);
         
         if (type == sgeE_QUEUE_SUSPEND_ON_SUB) {
            state |= QSUSPENDED_ON_SUBORDINATE;      /* set this bit */
         } else {
            state &= ~QSUSPENDED_ON_SUBORDINATE;     /* reset this bit */
         }
         lSetUlong(queue, QU_state, state);

         DEXIT;
         return TRUE;
      }
   }
   
   if(sge_mirror_update_master_list(list, list_descr, queue, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** gdi/queue/queue_reference_list_validate() ******************************
*  NAME
*     queue_reference_list_validate() -- verify a queue reference list
*
*  SYNOPSIS
*     int queue_reference_list_validate(lList **alpp, lList *qr_list, 
*                                       const char *attr_name, 
*                                       const char *obj_descr, 
*                                       const char *obj_name) 
*
*  FUNCTION
*     verify that all queue names in a QR_Type list refer to existing queues
*
*  INPUTS
*     lList **alpp          - pointer to an answer list
*     lList *qr_list        - the queue reference list
*     const char *attr_name - the attribute name in the referencing object
*     const char *obj_descr - the descriptor of the referencing object
*     const char *obj_name  - the name of the referencing object
*
*  RESULT
*     int - STATUS_OK, if everything is OK, else another status code,
*           see libs/gdi/sge_answer.h
*******************************************************************************/
int queue_reference_list_validate(
lList **alpp,
lList *qr_list,
const char *attr_name, /* e.g. "queue_list" */
const char *obj_descr, /* e.g. "parallel environment", "ckpt interface" */
const char *obj_name   /* e.g. "pvm", "hibernator"  */
) {
   lListElem *qrep;
   int all_name_exists = 0;
   int queue_exist = 0;

   DENTER(TOP_LAYER, "queue_reference_list_validate");

   for_each (qrep, qr_list) {
      if (!strcasecmp(lGetString(qrep, QR_name), SGE_ATTRVAL_ALL)) {
         lSetString(qrep, QR_name, SGE_ATTRVAL_ALL);
         all_name_exists = 1;
      } else if (!queue_list_locate(Master_Queue_List, lGetString(qrep, QR_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNQUEUE_SSSS, 
            lGetString(qrep, QR_name), attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      } else {
         queue_exist = 1;
      }
      if (all_name_exists && queue_exist) {
         ERROR((SGE_EVENT, MSG_SGETEXT_QUEUEALLANDQUEUEARENOT_SSSS,
            SGE_ATTRVAL_ALL, attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

/****** gdi/queue/queue_list_set_unknown_state_to() ************************
*  NAME
*     queue_list_set_unknown_state_to() -- set/clear u state of queues  
*
*  SYNOPSIS
*     void queue_list_set_unknown_state_to(lList *queue_list, 
*                                          const char *hostname, 
*                                          int send_events,
*                                          int new_state) 
*
*  FUNCTION
*     Clears or sets the unknown state of all queues in "queue_list" which 
*     reside on host "hostname". If "hostname" is NULL than all queues
*     mentioned in "queue_list" will get the new unknown state.
*     
*     Modify events for all modified queues will be generated if 
*     "send_events" is 1 (true).
*
*  INPUTS
*     lList *queue_list    - QU_Type list 
*     const char *hostname - valid hostname or NULL 
*     int send_events      - 0 or 1 
*     int new_state        - new unknown state (0 or 1)
*
*  RESULT
*     void - None
*******************************************************************************/
void queue_list_set_unknown_state_to(lList *queue_list, 
                                     const char *hostname,
                                     int send_events,
                                     int new_state)
{
   const void *iterator = NULL;
   lListElem *queue = NULL;
   lListElem *next_queue = NULL;

   if (hostname != NULL) {
      next_queue = lGetElemHostFirst(queue_list, QU_qhostname, 
                                     hostname, &iterator);
   } else {
      next_queue = lFirst(queue_list);
   }
   while ((queue = next_queue)) {
      u_long32 state;

      if (hostname != NULL) {
         next_queue = lGetElemHostNext(queue_list, QU_qhostname, 
                                       hostname, &iterator);
      } else {
         next_queue = lNext(queue);
      }
      state = lGetUlong(queue, QU_state);
      if ((ISSET(state, QUNKNOWN) > 0) != (new_state > 0)) {
         if (new_state) {
            SETBIT(QUNKNOWN, state);
         } else {
            CLEARBIT(QUNKNOWN, state);
         }
         lSetUlong(queue, QU_state, state);

         if (send_events) {
            sge_add_queue_event(sgeE_QUEUE_MOD, queue);
         }
      }
   }
} 

/* JG: TODO: is it really needed? */
void sge_add_queue_event(
u_long32 type,
lListElem *qep 
) {
   DENTER(TOP_LAYER, "sge_add_queue_event");
   sge_add_event(NULL, type, 0, 0, lGetString(qep, QU_qname), qep);
   DEXIT;
   return;
}

/* -----------------------------------
   sge_add_queue - adds a queue to the 
     Master_Queue_List. 

   returns:
   -1 if queue already exists or other error;
   0 if successful;
*/
/* JG: TODO: naming, ADOC */
/****** gdi/queue/queue_list_add_queue() ***************************************
*  NAME
*     queue_list_add_queue() -- add a new queue to the queue masterlist
*
*  SYNOPSIS
*     int queue_list_add_queue(lListElem *qep) 
*
*  FUNCTION
*     Adds the queue to the queue masterlist.
*     The queue is inserted in the sort order of the queue (by queue name).
*
*  INPUTS
*     lListElem *qep - the queue to insert
*
*  RESULT
*     int - TRUE, if the queue could be inserted, else FALSE
*
*  NOTES
*     Appending the queue and quick sorting the queue list would probably
*     be much faster in systems with many queues.
*
*******************************************************************************/
int queue_list_add_queue(lListElem *queue) 
{
   static lSortOrder *so = NULL;

   DENTER(TOP_LAYER, "queue_list_add_queue");

   if (queue == NULL) {
      ERROR((SGE_EVENT, MSG_QUEUE_NULLPTR));
      DEXIT;
      return FALSE;
   }

   /* create SortOrder: */
   if(so == NULL) {
      so = lParseSortOrderVarArg(QU_Type, "%I+", QU_qname);
   };
  
   /* insert Element: */
   if(Master_Queue_List == NULL) {
      Master_Queue_List = lCreateList("Master_Queue_List", QU_Type);
   }

   lInsertSorted(so, queue, Master_Queue_List);

   DEXIT;
   return TRUE;
}

/****** gdi/queue/queue_check_owner() ******************************************
*  NAME
*     queue_check_owner() -- check if a user is queue owner
*
*  SYNOPSIS
*     int queue_check_owner(const lListElem *queue, const char *user_name) 
*
*  FUNCTION
*     Checks if the given user is an owner of the given queue.
*     Managers and operators are implicitly owner of all queues.
*
*  INPUTS
*     const lListElem *queue - the queue to check
*     const char *user_name  - the user name to check
*
*  RESULT
*     int - TRUE, if the user is owner, else FALSE
*
*******************************************************************************/
int queue_check_owner(const lListElem *queue, const char *user_name)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "queue_check_owner");

   if (queue == NULL) {
      DEXIT;
      return FALSE;
   }

   if (user_name == NULL) {
      DEXIT;
      return FALSE;
   }

   if (manop_is_operator(user_name)) {
      DEXIT;
      return TRUE;
   }

   for_each(ep, lGetList(queue, QU_owner_list)) {
      DPRINTF(("comparing user >>%s<< vs. owner_list entry >>%s<<\n", 
               user_name, lGetString(ep, US_name)));
      if (!strcmp(user_name, lGetString(ep, US_name))) {
         DEXIT;
         return TRUE;
      }
   }

   DEXIT;
   return FALSE;
}

