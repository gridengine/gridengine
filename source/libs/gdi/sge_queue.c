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
#include "sge_queue.h"
#include "sge_event.h"

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
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "queue_update_master_list");

   list = &Master_Queue_List;
   list_descr = QU_Type;
   key_nm = QU_qname;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}
