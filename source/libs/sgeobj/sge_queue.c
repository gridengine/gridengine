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

#include "sge_object.h"
#include "sge_complex.h"
#include "sge_job.h"
#include "sge_manop.h"
#include "sge_userset.h"
#include "sge_event.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_queue.h"
#include "sge_todo.h"
#include "sge_utility.h"
#include "parse.h"

#include "commlib.h"
#include "commd.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

static const char *queue_types[] = {
   "BATCH",        
   "INTERACTIVE",  
   "CHECKPOINTING",
   "PARALLEL",
   ""
};

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

/****** sgeobj/queue/queue_get_state_string() *********************************
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

/****** sgeobj/queue/queue_list_locate() **************************************
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
******************************************************************************/
lListElem *queue_list_locate(lList *queue_list, const char *queue_name) 
{
   return lGetElemStr(queue_list, QU_qname, queue_name);
}

/****** sgeobj/queue/queue_list_set_tag() *************************************
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
******************************************************************************/
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

/****** sgeobj/queue/queue_list_clear_tags() **********************************
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
******************************************************************************/
void queue_list_clear_tags(lList *queue_list)
{
   queue_list_set_tag(queue_list, QUEUE_TAG_DEFAULT, 0);
} 

/****** sgeobj/queue/queue_reference_list_validate() **************************
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
*                             (e.g. "parallel environment", "ckpt interface")
*     const char *obj_name  - the name of the referencing object
*
*  RESULT
*     int - STATUS_OK, if everything is OK, else another status code,
*           see libs/gdi/sge_answer.h
******************************************************************************/
int 
queue_reference_list_validate(lList **alpp, lList *qr_list, 
                              const char *attr_name, const char *obj_descr, 
                              const char *obj_name) 
{
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

/****** sgeobj/queue/queue_list_add_queue() ***********************************
*  NAME
*     queue_list_add_queue() -- add a new queue to the queue masterlist
*
*  SYNOPSIS
*     bool queue_list_add_queue(lListElem *qep) 
*
*  FUNCTION
*     Adds the queue to the queue masterlist.
*     The queue is inserted in the sort order of the queue (by queue name).
*
*  INPUTS
*     lListElem *qep - the queue to insert
*
*  RESULT
*     bool - true, if the queue could be inserted, else false
*
*  NOTES
*     Appending the queue and quick sorting the queue list would probably
*     be much faster in systems with many queues.
*
******************************************************************************/
bool queue_list_add_queue(lListElem *queue) 
{
   static lSortOrder *so = NULL;

   DENTER(TOP_LAYER, "queue_list_add_queue");

   if (queue == NULL) {
      ERROR((SGE_EVENT, MSG_QUEUE_NULLPTR));
      DEXIT;
      return false;
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
   return true;
}

/****** sgeobj/queue/queue_check_owner() **************************************
*  NAME
*     queue_check_owner() -- check if a user is queue owner
*
*  SYNOPSIS
*     bool queue_check_owner(const lListElem *queue, const char *user_name) 
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
*     bool - true, if the user is owner, else false
*
******************************************************************************/
bool queue_check_owner(const lListElem *queue, const char *user_name)
{
   bool ret = false;
   lListElem *ep;

   DENTER(TOP_LAYER, "queue_check_owner");
   if (queue == NULL) {
      ret = false;
   } else if (user_name == NULL) {
      ret = false;
   } else if (manop_is_operator(user_name)) {
      ret = true;
   } else {
      for_each(ep, lGetList(queue, QU_owner_list)) {
         DPRINTF(("comparing user >>%s<< vs. owner_list entry >>%s<<\n", 
                  user_name, lGetString(ep, US_name)));
         if (!strcmp(user_name, lGetString(ep, US_name))) {
            ret = true;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/queue/queue_get_type_string() **********************************
*  NAME
*     queue_get_type_string() -- get readable type definition
*
*  SYNOPSIS
*     const char* 
*     queue_get_type_string(const lListElem *queue, lList **answer_list,
*                           dstring *buffer) 
*
*  FUNCTION
*     Returns a readable string representation of the queue type bitfield.
*
*  INPUTS
*     const lListElem *queue - the queue containing the requested information
*     dstring *buffer        - string buffer to hold the result string
*
*  RESULT
*     const char* - resulting string
*
*  SEE ALSO
*     sgeobj/queue/queue_set_type_string()
*******************************************************************************/
const char *
queue_get_type_string(const lListElem *queue, lList **answer_list, 
                      dstring *buffer)
{
   u_long32 type;
   int i;
   bool append = false;
   const char *ret;

   DENTER(TOP_LAYER, "queue_get_type_string");

   
   SGE_CHECK_POINTER_NULL(queue);
   SGE_CHECK_POINTER_NULL(buffer);

   type = lGetUlong(queue, QU_qtype);
   sge_dstring_clear(buffer);

   for (i = 0; queue_types[i] != NULL; i++) {
      if ((type & (1 << i)) != 0) {
         if (append) {
            sge_dstring_append(buffer, " ");
         }
         sge_dstring_append(buffer, queue_types[i]);
         append = true;
      }
   }

   ret = sge_dstring_get_string(buffer);
   DEXIT;
   return ret;
}

/****** sgeobj/queue/queue_set_type_string() **********************************
*  NAME
*     queue_set_type_string() -- set queue type from string representation
*
*  SYNOPSIS
*     bool 
*     queue_set_type_string(lListElem *queue, lList **answer_list, 
*                           const char *value) 
*
*  FUNCTION
*     Takes a string representation for the queue type, e.g. "BATCH PARALLEL"
*     and sets the queue type bitfield (attribute QU_qtype) of the given
*     queue.
*
*  INPUTS
*     lListElem *queue    - the queue to change
*     lList **answer_list - errors will be reported here
*     const char *value   - new value for queue type
*
*  RESULT
*     bool - true on success, 
*            false on error, error message will be in answer_list
*
*  SEE ALSO
*     sgeobj/queue/queue_get_type_string()
******************************************************************************/
bool 
queue_set_type_string(lListElem *queue, lList **answer_list, const char *value)
{
   bool ret = true;
   u_long32 type = 0;
 
   DENTER(TOP_LAYER, "queue_set_type_string");

   SGE_CHECK_POINTER_FALSE(queue);

   if (value != NULL && *value != 0) {
      if (!sge_parse_bitfield_str(value, queue_types, &type, 
                                 "queue type", NULL)) {
         ret = false;
      }
   }

   lSetUlong(queue, QU_qtype, type);

   DEXIT;
   return ret;
}

bool 
queue_validate(lListElem *queue, lList **answer_list)
{
   bool ret = true;

   const char *queue_name;
   const char *host_name;
   const char *str;

   DENTER(TOP_LAYER, "queue_validate");

   /* check queue name */
   queue_name = lGetString(queue, QU_qname);
   if(queue_name == NULL || *queue_name == '\0') {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                              MSG_NULLOREMPTYSTRINGFOR_S, "QU_qname");
      DEXIT;
      return false;
   }

   /* check host name */
   host_name = lGetHost(queue, QU_qhostname);
   if(host_name == NULL || *host_name == '\0') {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                              MSG_NULLOREMPTYSTRINGFOR_S, "QU_qhostname");
      DEXIT;
      return false;
   }

   /* check hostname resolving, accept unknown host */
   if(strcmp(queue_name, SGE_TEMPLATE_NAME) != 0) {
      char unique[MAXHOSTLEN];
      int ret1;

      if ((ret1 = getuniquehostname(host_name, unique, 0)) != CL_OK) {
         if (ret1 != COMMD_NACK_UNKNOWN_HOST) {
            answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS, 
                                    host_name, cl_errstr(ret1));
            ret = false; 
         }
      }
   }

   /* check processors */
   if ((str = lGetString(queue, QU_processors)) != NULL) {
      lList *range_list = NULL;
      range_list_parse_from_string(&range_list, answer_list, str, 
                                   JUST_PARSE, 0, INF_ALLOWED);
      range_list = lFreeList(range_list);
      /* JG: TODO: range_list_parse_from_string should return error */
      if (*answer_list) {   
         DEXIT;
         ret = false;
      }
   }

   NULL_OUT_NONE(queue, QU_calendar);
   NULL_OUT_NONE(queue, QU_prolog);
   NULL_OUT_NONE(queue, QU_epilog);
   NULL_OUT_NONE(queue, QU_shell_start_mode);
   NULL_OUT_NONE(queue, QU_starter_method);
   NULL_OUT_NONE(queue, QU_suspend_method);
   NULL_OUT_NONE(queue, QU_resume_method);
   NULL_OUT_NONE(queue, QU_terminate_method);
   NULL_OUT_NONE(queue, QU_initial_state);

   DEXIT;
   return true;
}

lListElem *queue_create_template(void)
{
   lListElem *queue, *ep;

   queue = lCreateElem(QU_Type);
   lSetString(queue, QU_qname, "template");
   lSetHost(queue, QU_qhostname, "unknown");

   ep = lAddSubStr(queue, CE_name, "np_load_avg", QU_load_thresholds, CE_Type);
   lSetString(ep, CE_stringval, "1.75"); 

   lSetString(queue, QU_suspend_interval, "00:05:00");
   lSetUlong(queue, QU_nsuspend, 1);
   lSetString(queue, QU_min_cpu_interval, "00:05:00");
   lSetString(queue, QU_processors, "UNDEFINED");
   lSetString(queue, QU_priority, "0");
   lSetUlong(queue, QU_qtype, BQ|IQ|PQ);
   lSetUlong(queue, QU_job_slots, 1);
   lSetString(queue, QU_tmpdir, "/tmp");
   lSetString(queue, QU_shell, "/bin/csh");
   lSetString(queue, QU_notify, "00:00:60");
   lSetString(queue, QU_initial_state, "default");

   lSetString(queue, QU_s_rt, "INFINITY");
   lSetString(queue, QU_h_rt, "INFINITY");
   lSetString(queue, QU_s_cpu, "INFINITY");
   lSetString(queue, QU_h_cpu, "INFINITY");
   lSetString(queue, QU_s_fsize, "INFINITY");
   lSetString(queue, QU_h_fsize, "INFINITY");
   lSetString(queue, QU_s_data, "INFINITY");
   lSetString(queue, QU_h_data, "INFINITY");
   lSetString(queue, QU_s_stack, "INFINITY");
   lSetString(queue, QU_h_stack, "INFINITY");
   lSetString(queue, QU_s_core, "INFINITY");
   lSetString(queue, QU_h_core, "INFINITY");
   lSetString(queue, QU_s_rss, "INFINITY");
   lSetString(queue, QU_h_rss, "INFINITY");
   lSetString(queue, QU_s_vmem, "INFINITY");
   lSetString(queue, QU_h_vmem, "INFINITY");

   return queue;
}

