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
/*----------------------------------------------------------------
 * sge.c
 *---------------------------------------------------------------*/

#define SGE_INCLUDE_QUEUED_JOBS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#include "sge.h"
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_copy_append.h"
#include "commlib.h"
#include "sge_time.h"
#include "sge_complex.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "scheduler.h"
/* #include "sge_schedd.h" */
#include "sge_orders.h"
#include "sge_job_schedd.h"

#include "schedd_conf.h"

#include "sgeee.h"
#include "sge_support.h"
#include "sge_schedconfL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_userprjL.h"
#include "sge_share_tree_nodeL.h"
/* #include "sge_sharetree.h" */
#include "sge_usageL.h"
#include "sge_hostL.h"
#include "sge_queueL.h"
#include "sge_usersetL.h"
#include "sge_eejobL.h"

#include "sort_hosts.h"
#include "msg_schedd.h"
#include "sge_language.h"
#include "sge_string.h"
#include "slots_used.h"
#include "sge_conf.h"
#include "sge_job_jatask.h"
#include "sge_range.h"

static sge_Sdescr_t *all_lists;
static u_long32 sge_scheduling_run;

static lListElem *get_mod_share_tree(lListElem *node, lEnumeration *what, int seqno);
static lList *sge_sort_pending_job_nodes(lListElem *root, lListElem *node,
                           double total_share_tree_tickets);
static int sge_calc_node_targets(lListElem *root, lListElem *node, sge_Sdescr_t *lists);
static int sge_calc_sharetree_targets(lListElem *root, sge_Sdescr_t *lists,
                           lList *decay_list, u_long curr_time,
                           u_long seqno);

static lListElem *locate_user_or_project(lList *user_list, const char *name);


#if 1 /* EB: */
static void task_ref_initialize_table(u_long32 number_of_tasks);
static void task_ref_destroy_table(void);
static sge_task_ref_t *task_ref_get_entry(u_long32 index);
static sge_task_ref_t *task_ref_table = NULL;
static u_long32 task_ref_entries = 0;
static u_long32 task_ref_job_pos = 0;

static void task_ref_initialize_table(u_long32 number_of_tasks) 
{
   const size_t size = number_of_tasks * sizeof(sge_task_ref_t);

   if (task_ref_entries != number_of_tasks && number_of_tasks > 0) {
      task_ref_destroy_table();
      task_ref_table = (sge_task_ref_t *) malloc(size);
      task_ref_entries = number_of_tasks;
   }
   if (task_ref_table != NULL) {
      memset(task_ref_table, 0, size);
   }
}

static void task_ref_destroy_table(void) 
{
   if (task_ref_table != NULL) {
      free(task_ref_table);
      task_ref_entries = 0;
   }
}

static sge_task_ref_t *task_ref_get_entry(u_long32 index) 
{
   sge_task_ref_t *ret = NULL;

   DENTER(BASIS_LAYER, "task_ref_get_entry");
   if (index >= 0 && index < task_ref_entries) {
      ret = &task_ref_table[index];
   }
   DEXIT;
   return ret;
}

#if 0 /* EB: debug */
void task_ref_print_table(void)
{
   u_long32 i;

   DENTER(BASIS_LAYER, "task_ref_print_table");
   for (i = 0; i < task_ref_entries; i++) {
      sge_task_ref_t *tref = task_ref_get_entry(i);

      if (tref == NULL) {
         break;
      }

      task_ref_print_table_entry(tref);
   }
   DEXIT;
}

void task_ref_print_table_entry(sge_task_ref_t *tref) 
{
   if (tref != NULL) {
      fprintf(stderr, "@@@ "
         "job: "u32" "
         "ja_task: "u32" "
         "t: %f "
         "st: %f "
         "s: %f "
         "\n",
         tref->job_number,
         tref->ja_task_number,
         tref->ja_task_ticket,
         tref->ja_task_sticket,
         tref->ja_task_share
      );       
   }
}
#endif

sge_task_ref_t *task_ref_get_first(u_long32 job_number, 
                                   u_long32 ja_task_number)
{
   sge_task_ref_t *ret = NULL;
   u_long32 i;

   DENTER(BASIS_LAYER, "task_ref_get_first");
   for (i = 0; i < task_ref_entries; i++) {
      sge_task_ref_t *tref = task_ref_get_entry(i);

      if (tref != NULL && tref->job_number == job_number &&  
          tref->ja_task_number == ja_task_number) {
         ret = tref;
         break;
      }
   }
   DEXIT;
   return ret;
}

sge_task_ref_t *task_ref_get_first_job_entry(void)
{
   sge_task_ref_t *ret = NULL;

   DENTER(BASIS_LAYER, "task_ref_get_first_job_entry");
   task_ref_job_pos = 0;
   if (task_ref_job_pos < task_ref_entries) {
      ret = task_ref_get_entry(task_ref_job_pos);
   }
   DEXIT;
   return ret;
}

sge_task_ref_t *task_ref_get_next_job_entry(void)
{
   sge_task_ref_t *current_entry = task_ref_get_entry(task_ref_job_pos);
   sge_task_ref_t *ret = NULL;
   u_long32 pos = task_ref_job_pos;

   DENTER(BASIS_LAYER, "task_ref_get_next_job_entry"); 
   if (current_entry != NULL) {
      u_long32 current_job_number = current_entry->job_number;
   
      while (++pos < task_ref_entries) {
         sge_task_ref_t *next_entry = task_ref_get_entry(pos);
         
         if (next_entry != NULL && 
             current_job_number != next_entry->job_number) {
            task_ref_job_pos = pos;
            ret = next_entry;
            break; 
         } 
      } 
   }
   DEXIT;
   return ret;        
}

void task_ref_copy_to_ja_task(sge_task_ref_t *tref, lListElem *ja_task) 
{
   DENTER(BASIS_LAYER, "task_ref_copy_to_ja_task");
   if (ja_task != NULL && tref != NULL) {
      lSetUlong(ja_task, JAT_task_number, tref->ja_task_number);
      lSetDouble(ja_task, JAT_ticket, tref->ja_task_ticket); 
      lSetDouble(ja_task, JAT_fticket, tref->ja_task_fticket); 
      lSetDouble(ja_task, JAT_sticket, tref->ja_task_sticket); 
      lSetDouble(ja_task, JAT_oticket, tref->ja_task_oticket); 
      lSetDouble(ja_task, JAT_dticket, tref->ja_task_dticket); 
      lSetDouble(ja_task, JAT_share, tref->ja_task_share); 
      lSetUlong(ja_task, JAT_fshare, tref->ja_task_fshare); 
   }
   DEXIT;
}

u_long32 sgeee_get_scheduling_run_id(void) 
{
   return sge_scheduling_run;
}

#endif

#define SGE_MIN_USAGE 1.0

#define __REF_SET_TYPE(ref, cull_attr, ref_attr, value, function) \
{ \
   if ((ref)->ja_task) { \
      (function)((ref)->ja_task, cull_attr, (value)); \
   } else { \
      (ref_attr) = (value); \
   } \
}
 
#define __REF_GET_TYPE(ref, cull_attr, ref_attr, function) \
   ((ref)->ja_task ? (function)((ref)->ja_task, cull_attr) : (ref_attr))
 
#define __REF_SET_ULONG(ref, cull_attr, ref_attr, value) \
   __REF_SET_TYPE(ref, cull_attr, ref_attr, value, lSetUlong)
 
#define __REF_SET_DOUBLE(ref, cull_attr, ref_attr, value) \
   __REF_SET_TYPE(ref, cull_attr, ref_attr, value, lSetDouble)
 
#define __REF_GET_ULONG(ref, cull_attr, ref_attr) \
   __REF_GET_TYPE(ref, cull_attr, ref_attr, lGetUlong)
 
#define __REF_GET_DOUBLE(ref, cull_attr, ref_attr) \
   __REF_GET_TYPE(ref, cull_attr, ref_attr, lGetDouble)
 
/* u_long32 REF_GET_JA_TASK_NUMBER(sge_ref_t *ref) */
 
#define REF_GET_JA_TASK_NUMBER(ref) \
   __REF_GET_ULONG((ref), JAT_task_number, (ref)->tref->ja_task_number)
 
/* void REF_SET_JA_TASK_NUMBER(sge_ref_t *ref, u_long32 ja_task_number) */
#define REF_SET_JA_TASK_NUMBER(ref, ja_task_id) \
   __REF_SET_ULONG((ref), JAT_task_number, (ref)->tref->ja_task_number, \
                   (ja_task_id))
 
/* double REF_GET_?TICKET(sge_ref_t *ref) */
 
#define REF_GET_FTICKET(ref) \
   __REF_GET_DOUBLE((ref), JAT_fticket, (ref)->tref->ja_task_fticket)
 
#define REF_GET_STICKET(ref) \
   __REF_GET_DOUBLE((ref), JAT_sticket, (ref)->tref->ja_task_sticket)
 
#define REF_GET_OTICKET(ref) \
   __REF_GET_DOUBLE((ref), JAT_oticket, (ref)->tref->ja_task_oticket)
 
#define REF_GET_DTICKET(ref) \
   __REF_GET_DOUBLE((ref), JAT_dticket, (ref)->tref->ja_task_dticket)
 
#define REF_GET_TICKET(ref) \
   __REF_GET_DOUBLE((ref), JAT_ticket, (ref)->tref->ja_task_ticket)
 
#define REF_GET_SHARE(ref) \
   __REF_GET_DOUBLE((ref), JAT_share, (ref)->tref->ja_task_share)
 
#define REF_GET_FSHARE(ref) \
   __REF_GET_ULONG((ref), JAT_fshare, (ref)->tref->ja_task_fshare)

/* REF_SET_?TICKET(sge_ref_t *ref, double new_fticket) */
 
#define REF_SET_FTICKET(ref, ticket) \
   __REF_SET_DOUBLE((ref), JAT_fticket, (ref)->tref->ja_task_fticket, (ticket))
 
#define REF_SET_STICKET(ref, ticket) \
   __REF_SET_DOUBLE((ref), JAT_sticket, (ref)->tref->ja_task_sticket, (ticket))
 
#define REF_SET_OTICKET(ref, ticket) \
   __REF_SET_DOUBLE((ref), JAT_oticket, (ref)->tref->ja_task_oticket, (ticket))
 
#define REF_SET_DTICKET(ref, ticket) \
   __REF_SET_DOUBLE((ref), JAT_dticket, (ref)->tref->ja_task_dticket, (ticket))
 
#define REF_SET_TICKET(ref, ticket) \
   __REF_SET_DOUBLE((ref), JAT_ticket, (ref)->tref->ja_task_ticket, (ticket))
 
#define REF_SET_SHARE(ref, share) \
   __REF_SET_DOUBLE((ref), JAT_share, (ref)->tref->ja_task_share, (share))

#define REF_SET_FSHARE(ref, fshare) \
   __REF_SET_ULONG((ref), JAT_fshare, (ref)->tref->ja_task_fshare, (fshare))

/*------------------------------------------------------------------
 * if we dispatch a job sub-task and the job has more
 * sub-tasks, then the job is still first in the job list.
 * We need to remove and reinsert the job back into the sorted job
 * list in case another job is higher priority (i.e. has more tickets)
 *
 * Additionally it is neccessary to update the number of pending tickets
 * for the following pending array task. (The next task will get less
 * tickets than the current one)
 *------------------------------------------------------------------*/
void sgeee_resort_pending_jobs(lList **job_list, lList *orderlist)
{
   lListElem *next_job = lFirst(*job_list);

   DENTER(TOP_LAYER, "sgeee_resort_pending_jobs");
   if (next_job) {
      u_long32 job_id = lGetUlong(next_job, JB_job_number);
      lList *range_list = lGetList(next_job, JB_ja_n_h_ids);
      u_long32 ja_task_id = range_list_get_first_id(range_list, NULL);
      sge_task_ref_t *tref = task_ref_get_first(job_id, ja_task_id);
      lListElem *ja_task_template = lFirst(lGetList(next_job, JB_ja_template));
      lListElem *insert_jep = NULL;
      lListElem *jep = NULL;
      lListElem *order = NULL;
      double ticket;

      /*
       * Update pending tickets in template element
       */
      task_ref_copy_to_ja_task(tref, ja_task_template);


      /* 
       * Update pending tickets in ORT_ptickets-order which was
       * created previously
       */
      for_each(order, orderlist) {
         if (lGetUlong(order, OR_type) == ORT_ptickets &&
             lGetUlong(order, OR_job_number) == job_id) {
            lListElem *order_job = lFirst(lGetList(order, OR_joker));
            lListElem *order_task = lFirst(lGetList(order_job, JB_ja_tasks));

            task_ref_copy_to_ja_task(tref, order_task);
            break;
         }
      }

      /*
       * Re-Insert job at the correct possition
       */
      lDechainElem(*job_list, next_job);
      ticket = lGetDouble(ja_task_template, JAT_ticket);
      for_each(jep, *job_list) {
         lListElem *ja_task_template2 = lFirst(lGetList(jep, JB_ja_template));
         double ticket2 = lGetDouble(ja_task_template2, JAT_ticket);
         u_long32 job_id2 = lGetUlong(jep, JB_job_number);
 
         if (ticket > ticket2 || (ticket == ticket2 && job_id < job_id2)) {
            break;
         }
         insert_jep = jep;
      }
 
      lInsertElem(*job_list, insert_jep, next_job);
   }
   DEXIT;
}

/*--------------------------------------------------------------------
 * job_is_active - returns true if job is active
 *--------------------------------------------------------------------*/

static int
job_is_active( lListElem *job,
               lListElem *ja_task )
{
   u_long job_status = lGetUlong(ja_task, JAT_status);
#ifdef SGE_INCLUDE_QUEUED_JOBS
   return (job_status == JIDLE ||
       job_status & (JRUNNING | JMIGRATING | JQUEUED | JTRANSITING)) &&
       !(lGetUlong(ja_task, JAT_state) 
         & (JSUSPENDED|JSUSPENDED_ON_THRESHOLD));
#else
   return (job_status & (JRUNNING | JMIGRATING | JTRANSITING)) &&
       !(lGetUlong(ja_task, JAT_state) & 
         (JSUSPENDED|JSUSPENDED_ON_THRESHOLD));
#endif
}


/*--------------------------------------------------------------------
 * locate_user_or_project - locate the user or project object by name
 *--------------------------------------------------------------------*/

static lListElem *locate_user_or_project(lList *user_list, const char *name)
{
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the user or project object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(user_list, UP_name, name);
}


/*--------------------------------------------------------------------
 * locate_department - locate the department object by name
 *--------------------------------------------------------------------*/

static lListElem *
locate_department( lList *dept_list,
                   const char *name )
{
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the department object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(dept_list, US_name, name);
}


/*--------------------------------------------------------------------
 * locate_jobclass - locate the job class object by name
 *--------------------------------------------------------------------*/

static lListElem *
locate_jobclass( lList *job_class_list,
                 const char *name )
{
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the job class object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(job_class_list, QU_qname, name);
}

#if 0
/*--------------------------------------------------------------------
 * locate_jobclass - locate the job class object for a pending job.
 * This returns the first matching queue.
 *--------------------------------------------------------------------*/

static lListElem *
locate_jobclass_for_pending_job( lListElem *jep,
                                 lList *queue_list,
                                 lList *exec_host_list,
                                 lList *complex_list )
{
   lListElem *qep;
   lList *ce;

   /*
   ** if the job has no requests¸ any queue fits
   ** so return the first queue
   */
   if (!lGetList(jep, JB_hard_resource_list)) {
      return lFirst(queue_list);
   }

   /*
   ** return first queue which fulfills the request_list of the job
   */
   for_each(qep, queue_list) {
      lList *ccl[3];
      lListElem *ep;
      ccl[0] = lGetList(lGetElemHost(exec_host_list, EH_name, "global"),
EH_consumable_config_list);
      ccl[1] = (ep=lGetElemHost(exec_host_list, EH_name, lGetHost(qep, QU_qhostname)))?
               lGetList(ep, EH_consumable_config_list):NULL;
      ccl[2] = lGetList(qep, QU_consumable_config_list);

      ce = NULL;
      queue_complexes2scheduler(&ce, qep, exec_host_list, complex_list, 0);
      if (sge_select_queue(ce, lGetList(jep, JB_hard_resource_list), 1,
                                       NULL, 0, -1, ccl)) {
         ce = lFreeList(ce);
         return qep;
      }
      ce = lFreeList(ce);
   }

   return NULL;
}

#endif


/*--------------------------------------------------------------------
 * sge_set_job_refs - set object references in the job entry
 *--------------------------------------------------------------------*/

void
sge_set_job_refs( lListElem *job,
                  lListElem *ja_task,
                  sge_ref_t *ref,
                  sge_task_ref_t *tref,
                  sge_Sdescr_t *lists,
                  int queued)
{
   lListElem *root;
   lList *granted;
   lListElem *pe, *granted_el;
   int is_enrolled_ja_task = (ja_task != NULL && tref == NULL) ? 1 : 0;
   const char *pe_str;

   if (ref->task_jobclass) {
      free(ref->task_jobclass);
   }

   memset(ref, 0, sizeof(sge_ref_t));

   if (tref != NULL) {
      memset(tref, 0, sizeof(sge_task_ref_t));

      tref->job_number = lGetUlong(job, JB_job_number);
      tref->ja_task_number = lGetUlong(ja_task, JAT_task_number);
      ref->tref = tref;
   }

   /*-------------------------------------------------------------
    * save job reference
    *-------------------------------------------------------------*/

   ref->job = job;

   /*-------------------------------------------------------------
    * save task reference 
    *-------------------------------------------------------------*/

   ref->ja_task = is_enrolled_ja_task ? ja_task : NULL;

   /*-------------------------------------------------------------
    * save job state
    *-------------------------------------------------------------*/

   ref->queued = queued;

   /*-------------------------------------------------------------
    * locate user object and save off reference
    *-------------------------------------------------------------*/

   ref->user = locate_user_or_project(lists->user_list,
                                      lGetString(job, JB_owner));
   if (ref->user) {
      lSetUlong(ref->user, UP_job_cnt, 0);
      lSetUlong(ref->user, UP_pending_job_cnt, 0);
   }

   /*-------------------------------------------------------------
    * locate project object and save off reference
    *-------------------------------------------------------------*/

   ref->project = locate_user_or_project(lists->project_list,
                                         lGetString(job, JB_project));
   if (ref->project) {
      lSetUlong(ref->project, UP_job_cnt, 0);
      lSetUlong(ref->project, UP_pending_job_cnt, 0);
   }

   /*-------------------------------------------------------------
    * locate job class object and save off reference
    *-------------------------------------------------------------*/

   if (ref->queued) {
#if 0
      ref->jobclass = locate_jobclass_for_pending_job(job,
                                 lists->all_queue_list,
                                 lists->host_list,
                                 lists->complex_list);
      if (ref->jobclass) {
         lSetUlong(ref->jobclass, QU_job_cnt, 0);
         lSetUlong(ref->jobclass, QU_pending_job_cnt, 0);
      }
#endif
   } else if (is_enrolled_ja_task) {
      ref->jobclass = locate_jobclass(lists->all_queue_list,
                                      lGetString(ja_task, JAT_master_queue));
      if (ref->jobclass) {
         lSetUlong(ref->jobclass, QU_job_cnt, 0);
         lSetUlong(ref->jobclass, QU_pending_job_cnt, 0);
      }
   }

   /*-------------------------------------------------------------
    * for task-controlled PE jobs
    *    for each task
    *       locate job class object and save off reference
    *-------------------------------------------------------------*/

   if (is_enrolled_ja_task && (pe_str=lGetString(ja_task, JAT_granted_pe)) && 
       (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str)) &&
       lGetUlong(pe, PE_control_slaves) &&
       (granted=lGetList(ja_task, JAT_granted_destin_identifier_list))) {

      ref->num_task_jobclasses = lGetNumberOfElem(granted);

      if (ref->num_task_jobclasses > 0) {
         const size_t size = ref->num_task_jobclasses * sizeof(lListElem *);
         int i=0;

         ref->task_jobclass = (lListElem **)malloc(size); 
         memset((void *)ref->task_jobclass, 0, size); 

         for_each(granted_el, granted) {
            if (active_subtasks(job, lGetString(granted_el, JG_qname))) {
               ref->task_jobclass[i] = locate_jobclass(lists->all_queue_list,
                                       lGetString(granted_el, JG_qname));
               if (ref->task_jobclass[i]) {
                  lSetUlong(ref->task_jobclass[i], QU_job_cnt, 0);
                  lSetUlong(ref->task_jobclass[i], QU_pending_job_cnt, 0);
               }
            }
            i++;
         }
      }
   }

   /*-------------------------------------------------------------
    * locate share tree node and save off reference
    *-------------------------------------------------------------*/

   if ((root = lFirst(lists->share_tree))) {
      lListElem *pnode = NULL;
       
      ref->share_tree_type = lGetUlong(root, STN_type);

#ifdef notdef
      if (ref->share_tree_type == STT_PROJECT)
         userprj = ref->project;
      else
         userprj = ref->user;
#endif

      if (ref->user || ref->project) {
         ref->node = search_userprj_node(root,
               ref->user ? lGetString(ref->user, UP_name) : NULL,
               ref->project ? lGetString(ref->project, UP_name) : NULL,
               &pnode);
         /*
          * if the found node is the "default" node, then create a
          * temporary sibling node using the "default" node as a
          * template
          */

         if (ref->user && ref->node && pnode &&
             strcmp(lGetString(ref->node, STN_name), "default") == 0) {
            ref->node = lCopyElem(ref->node);
            lSetString(ref->node, STN_name, lGetString(ref->user, UP_name));
            lSetUlong(ref->node, STN_temp, 1);
            lAppendElem(lGetList(pnode, STN_children), ref->node);
         }
      } else
         ref->node = NULL;
   }

   /*-------------------------------------------------------------
    * locate department object and save off reference in job entry
    *-------------------------------------------------------------*/

   ref->dept = locate_department(lists->dept_list,
                                 lGetString(job, JB_department));
   if (ref->dept) {
      lSetUlong(ref->dept, US_job_cnt, 0);
      lSetUlong(ref->dept, US_pending_job_cnt, 0);
   }
}


/*--------------------------------------------------------------------
 * sge_set_job_cnts - set job counts in the various object entries
 *--------------------------------------------------------------------*/

void
sge_set_job_cnts( sge_ref_t *ref,
                  int queued )
{
   int i;
   u_long up_job_cnt = queued ? UP_pending_job_cnt : UP_job_cnt;
   u_long us_job_cnt = queued ? US_pending_job_cnt : US_job_cnt;
   u_long qu_job_cnt = queued ? QU_pending_job_cnt : QU_job_cnt;
   if (ref->user)
      lSetUlong(ref->user, up_job_cnt, lGetUlong(ref->user, up_job_cnt)+1);
   if (ref->project)
      lSetUlong(ref->project, up_job_cnt, lGetUlong(ref->project,up_job_cnt)+1);
   if (ref->dept)
      lSetUlong(ref->dept, us_job_cnt, lGetUlong(ref->dept, us_job_cnt)+1);
   if (ref->task_jobclass)
      for(i=0; i<ref->num_task_jobclasses; i++) {
         if (ref->task_jobclass[i])
            lSetUlong(ref->task_jobclass[i], qu_job_cnt,
                      lGetUlong(ref->task_jobclass[i], qu_job_cnt)+1);
      }
   else
      if (ref->jobclass)
         lSetUlong(ref->jobclass, qu_job_cnt, lGetUlong(ref->jobclass,
               qu_job_cnt)+1);
   return;
}


/*--------------------------------------------------------------------
 * sge_unset_job_cnts - set job counts in the various object entries
 *--------------------------------------------------------------------*/

void
sge_unset_job_cnts( sge_ref_t *ref,
                    int queued )
{
   int i;
   u_long up_job_cnt = queued ? UP_pending_job_cnt : UP_job_cnt;
   u_long us_job_cnt = queued ? US_pending_job_cnt : US_job_cnt;
   u_long qu_job_cnt = queued ? QU_pending_job_cnt : QU_job_cnt;
   if (ref->user)
      lSetUlong(ref->user, up_job_cnt, lGetUlong(ref->user, up_job_cnt)-1);
   if (ref->project)
      lSetUlong(ref->project, up_job_cnt, lGetUlong(ref->project,up_job_cnt)-1);
   if (ref->dept)
      lSetUlong(ref->dept, us_job_cnt, lGetUlong(ref->dept, us_job_cnt)-1);
   if (ref->task_jobclass)
      for(i=0; i<ref->num_task_jobclasses; i++) {
         if (ref->task_jobclass[i])
            lSetUlong(ref->task_jobclass[i], qu_job_cnt,
                      lGetUlong(ref->task_jobclass[i], qu_job_cnt)-1);
      }
   else
      if (ref->jobclass)
         lSetUlong(ref->jobclass, qu_job_cnt, lGetUlong(ref->jobclass,
               qu_job_cnt)-1);
   return;
}


/*--------------------------------------------------------------------
 * calculate_m_shares - calculate m_share for share tree node
 *      descendants
 *
 * Calling calculate_m_shares(root_node) will calculate shares for
 * every active node in the share tree.
 *
 * Rather than having to recalculate m_shares on every scheduling
 * interval, we calculate it whenever the scheduler comes up or
 * whenever the share tree itself changes and make adjustments
 * every time a job becomes active or inactive (in adjust_m_shares).
 *--------------------------------------------------------------------*/

void
calculate_m_shares( lListElem *parent_node )
{
   u_long sum_of_child_shares = 0;
   lListElem *child_node;
   lList *children;
   double parent_m_share;

   DENTER(TOP_LAYER, "calculate_m_shares");

   children = lGetList(parent_node, STN_children);
   if (!children) {
      DEXIT;
      return;
   }

   /*-------------------------------------------------------------
    * Sum child shares
    *-------------------------------------------------------------*/

   for_each(child_node, children) {

      if (lGetUlong(child_node, STN_job_ref_count) > 0)
         sum_of_child_shares += lGetUlong(child_node, STN_shares);
   }

   /*-------------------------------------------------------------
    * Calculate m_shares for each child and descendants
    *-------------------------------------------------------------*/

   parent_m_share = lGetDouble(parent_node, STN_m_share);

   for_each(child_node, children) {
      if (lGetUlong(child_node, STN_job_ref_count) > 0)
         lSetDouble(child_node, STN_m_share,
                    parent_m_share *
                    (sum_of_child_shares ?
                    ((double)(lGetUlong(child_node, STN_shares)) /
                    (double)sum_of_child_shares) : 0));
      else
         lSetDouble(child_node, STN_m_share, 0);

      calculate_m_shares(child_node);
   }

   DEXIT;
   return;
}


#ifdef notdef

/*--------------------------------------------------------------------
 * adjust_m_shares - adjust m_share for all ancestor nodes with
 * reference count less than or equal to count parameter.
 *--------------------------------------------------------------------*/

void
adjust_m_shares( lListElem *root,
                 lListElem *job,
                 u_long count )
{
   lListElem *node;
   char *name;
   ancestors_t ancestors;
   int depth;

   if (!root)
      return;

   /*-------------------------------------------------------------
    * Look up share tree node
    *-------------------------------------------------------------*/

   name = lGetString(job, lGetUlong(root, STN_type) == STT_PROJECT ?
                          JB_project : JB_owner);

   if (!name)
      return;

   memset(&ancestors, 0, sizeof(ancestors_t));
   node = search_ancestor_list(root, name, &ancestors);
   depth = ancestors.depth;

   /*-------------------------------------------------------------
    * Search ancestor nodes until a previously active node is found
    *-------------------------------------------------------------*/

   while (depth-- && node && node != root &&
          lGetUlong(node, STN_job_ref_count)<=count) {
      node = ancestors.nodes[depth-1];
   }

   if (ancestors.nodes) free(ancestors.nodes);

   /*-------------------------------------------------------------
    * Calculate m_shares for every descendant of the active node
    *-------------------------------------------------------------*/

   if (node) calculate_m_shares(node);

}

/*

   To know if a node is active, we maintain a reference count in each
   node and do the following to keep it up-to-date.

   o call setup_share_tree when scheduler is initialized or
     if the share tree structure or shares are modified
   o call increment_job_ref_count when a new job is added (i.e. submitted)
   o call decrement_job_ref_count when a job is deleted (i.e. ended, aborted)

*/

/*--------------------------------------------------------------------
 * adjust_job_ref_count - adjusts all of the corresponding share
 * tree nodes' job reference counts
 *--------------------------------------------------------------------*/

void
adjust_job_ref_count( lListElem *root,
                      lListElem *job,
                      long adjustment )
{
   lListElem *node=NULL;
   char *name;
   ancestors_t ancestors;
   int depth;

   /*-------------------------------------------------------------
    * Adjust job reference count for share tree node and
    *     ancestor nodes
    *-------------------------------------------------------------*/

   memset(&ancestors, 0, sizeof(ancestors_t));

   name = lGetString(job, lGetUlong(root, STN_type) == STT_PROJECT ?
                          JB_project : JB_owner);

   if (!name)
      return;

   memset(&ancestors, 0, sizeof(ancestors_t));
   node = search_ancestor_list(root, name, &ancestors);
   depth = ancestors.depth;

   while (depth--) {
      node = ancestors.nodes[depth];
      lSetUlong(node, STN_job_ref_count,
         lGetUlong(node, STN_job_ref_count)+adjustment);
   }

   if (ancestors.nodes) free(ancestors.nodes);

   return;
}


/*--------------------------------------------------------------------
 * increment_job_ref_count - increments all of the corresponding share
 * tree nodes' job reference counts and the job's share tree node
 * reference
 *--------------------------------------------------------------------*/

void
increment_job_ref_count( lListElem *root,
                         lListElem *job )
{
   adjust_job_ref_count(root, job, 1);
}


/*--------------------------------------------------------------------
 * decrement_job_ref_count - decrements all of the corresponding share
 * tree nodes' job reference counts and the job's share tree node
 * reference
 *--------------------------------------------------------------------*/

void
decrement_job_ref_count( lListElem *root,
                         lListElem *job )
{
   adjust_job_ref_count(root, job, -1);
}


#endif /* notdef */


/*--------------------------------------------------------------------
 * update_job_ref_count - update job_ref_count for node and descendants
 *--------------------------------------------------------------------*/

u_long
update_job_ref_count( lListElem *node )
{
   int job_count=0;
   lList *children;
   lListElem *child;

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         job_count += update_job_ref_count(child);
      }
      lSetUlong(node, STN_job_ref_count, job_count);
   }

   return lGetUlong(node, STN_job_ref_count);
}

/*--------------------------------------------------------------------
 * update_active_job_ref_count - update active_job_ref_count for node and descendants
 *--------------------------------------------------------------------*/

u_long
update_active_job_ref_count( lListElem *node )
{
   int active_job_count=0;
   lList *children;
   lListElem *child;

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         active_job_count += update_active_job_ref_count(child);
      }
      lSetUlong(node, STN_active_job_ref_count, active_job_count);
   }

   return lGetUlong(node, STN_active_job_ref_count);
}


/*--------------------------------------------------------------------
 * sge_init_share_tree_node_fields - zero out the share tree node
 * fields that will be set and used during sge_calc_tickets
 *--------------------------------------------------------------------*/

int
sge_init_share_tree_node_fields( lListElem *node,
                                 void *ptr )
{
   static int sn_m_share_pos = -1;
   static int sn_adjusted_current_proportion_pos,
              sn_last_actual_proportion_pos, sn_sum_priority_pos,
              sn_job_ref_count_pos, sn_active_job_ref_count_pos,
              sn_usage_list_pos, sn_stt_pos, sn_ostt_pos,
              sn_ltt_pos, sn_oltt_pos, sn_shr_pos, sn_ref_pos,
              sn_proportion_pos, sn_adjusted_proportion_pos, sn_target_proportion_pos,
              sn_current_proportion_pos, sn_adjusted_usage_pos, sn_combined_usage_pos,
              sn_actual_proportion_pos;


   if (sn_m_share_pos == -1) {
      sn_m_share_pos = lGetPosViaElem(node, STN_m_share);
      sn_adjusted_current_proportion_pos =
            lGetPosViaElem(node, STN_adjusted_current_proportion);
      sn_last_actual_proportion_pos =
            lGetPosViaElem(node, STN_last_actual_proportion);
      sn_job_ref_count_pos = lGetPosViaElem(node, STN_job_ref_count);
      sn_active_job_ref_count_pos = lGetPosViaElem(node, STN_active_job_ref_count);
      sn_usage_list_pos = lGetPosViaElem(node, STN_usage_list);
      sn_sum_priority_pos = lGetPosViaElem(node, STN_sum_priority);
      /* sn_temp_pos = lGetPosViaElem(node, STN_temp); */
      sn_stt_pos = lGetPosViaElem(node, STN_stt);
      sn_ostt_pos = lGetPosViaElem(node, STN_ostt);
      sn_ltt_pos = lGetPosViaElem(node, STN_ltt);
      sn_oltt_pos = lGetPosViaElem(node, STN_oltt);
      sn_shr_pos = lGetPosViaElem(node, STN_shr);
      sn_ref_pos = lGetPosViaElem(node, STN_ref);
      sn_proportion_pos = lGetPosViaElem(node, STN_proportion);
      sn_adjusted_proportion_pos = lGetPosViaElem(node, STN_adjusted_proportion);
      sn_target_proportion_pos = lGetPosViaElem(node, STN_target_proportion);
      sn_current_proportion_pos = lGetPosViaElem(node, STN_current_proportion);
      sn_adjusted_usage_pos = lGetPosViaElem(node, STN_adjusted_usage);
      sn_combined_usage_pos = lGetPosViaElem(node, STN_combined_usage);
      sn_actual_proportion_pos = lGetPosViaElem(node, STN_actual_proportion);
   }

   lSetPosDouble(node, sn_m_share_pos, 0);
   lSetPosDouble(node, sn_last_actual_proportion_pos, 0);
   lSetPosDouble(node, sn_adjusted_current_proportion_pos, 0);
   lSetPosDouble(node, sn_proportion_pos, 0);
   lSetPosDouble(node, sn_adjusted_proportion_pos, 0);
   lSetPosDouble(node, sn_target_proportion_pos, 0);
   lSetPosDouble(node, sn_current_proportion_pos, 0);
   lSetPosDouble(node, sn_adjusted_usage_pos, 0);
   lSetPosDouble(node, sn_combined_usage_pos, 0);
   lSetPosDouble(node, sn_actual_proportion_pos, 0);
   lSetPosUlong(node, sn_job_ref_count_pos, 0);
   lSetPosUlong(node, sn_active_job_ref_count_pos, 0);
   lSetPosList(node, sn_usage_list_pos, NULL);
   lSetPosUlong(node, sn_sum_priority_pos, 0);
   /* lSetPosUlong(node, sn_temp_pos, 0); */
   lSetPosDouble(node, sn_stt_pos, 0);
   lSetPosDouble(node, sn_ostt_pos, 0);
   lSetPosDouble(node, sn_ltt_pos, 0);
   lSetPosDouble(node, sn_oltt_pos, 0);
   lSetPosDouble(node, sn_shr_pos, 0);
   lSetPosUlong(node, sn_ref_pos, 0);
   return 0;
}

/*--------------------------------------------------------------------
 * sge_init_share_tree_nodes - zero out the share tree node fields 
 * that will be set and used during sge_calc_tickets
 *--------------------------------------------------------------------*/

int
sge_init_share_tree_nodes( lListElem *root )
{
   return sge_for_each_share_tree_node(root,
         sge_init_share_tree_node_fields, NULL);
}


/*--------------------------------------------------------------------
 * set_share_tree_project_flags - set the share tree project flag for
 *       node and descendants
 *--------------------------------------------------------------------*/

void
set_share_tree_project_flags( lList *project_list,
                              lListElem *node )
{
   lList *children;
   lListElem *child;

   if (!project_list || !node)
      return;

   if (lGetElemStr(project_list, UP_name, lGetString(node, STN_name)))
      lSetUlong(node, STN_project, 1);
   else
      lSetUlong(node, STN_project, 0);

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         set_share_tree_project_flags(project_list, child);
      }
   }
   return;
}


/*--------------------------------------------------------------------
 * get_usage - return usage entry based on name
 *--------------------------------------------------------------------*/

static lListElem *
get_usage( lList *usage_list,
           const char *name )
{
   return lGetElemStr(usage_list, UA_name, name);
}


/*--------------------------------------------------------------------
 * create_usage_elem - create a new usage element
 *--------------------------------------------------------------------*/

lListElem *
create_usage_elem( const char *name )
{
   lListElem *usage;
    
   usage = lCreateElem(UA_Type);
   lSetString(usage, UA_name, name);
   lSetDouble(usage, UA_value, 0);

   return usage;
}


/*--------------------------------------------------------------------
 * build_usage_list - create a new usage list from an existing list
 *--------------------------------------------------------------------*/

lList *
build_usage_list( char *name,
                  lList *old_usage_list )
{
   lList *usage_list = NULL;
   lListElem *usage;

   if (old_usage_list) {

      /*-------------------------------------------------------------
       * Copy the old list and zero out the usage values
       *-------------------------------------------------------------*/

      usage_list = lCopyList(name, old_usage_list);
      for_each(usage, usage_list)
         lSetDouble(usage, UA_value, 0);

   } else {

      /*
       * the UA_value fields are implicitly set to 0 at creation
       * time of a new element with lCreateElem or lAddElemStr
       */
        
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_CPU, UA_Type);
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_MEM, UA_Type);
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_IO, UA_Type);
   }

   return usage_list;
}




/*--------------------------------------------------------------------
 * delete_debited_job_usage - deleted debitted job usage for job
 *--------------------------------------------------------------------*/

void
delete_debited_job_usage( sge_ref_t *ref,
                          u_long seqno )
{
   lListElem *job=ref->job,
             *user=ref->user,
             *project=ref->project;
   lList *upu_list;
   lListElem *upu;
   
   DENTER(TOP_LAYER, "delete_debited_job_usage");

   DPRINTF(("DDJU (1) "u32"\n", lGetUlong(job, JB_job_number)));

   if (user) {
      upu_list = lGetList(user, UP_debited_job_usage);
      DPRINTF(("DDJU (2) "u32"\n", lGetUlong(job, JB_job_number)));
      if (upu_list) {
         
         /* Note: In order to cause the qmaster to delete the
            usage for this job, we zero out UPU_old_usage_list
            for this job in the UP_debited_job_usage list */
         DPRINTF(("DDJU (3) "u32"\n", lGetUlong(job, JB_job_number))); 

         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                             lGetUlong(job, JB_job_number)))) {
            DPRINTF(("DDJU (4) "u32"\n", lGetUlong(job, JB_job_number)));
            lSetList(upu, UPU_old_usage_list, NULL);
            lSetUlong(user, UP_usage_seqno, seqno);
         }
      }
   }

   if (project) {
      upu_list = lGetList(project, UP_debited_job_usage);
      if (upu_list) {
         
         /* Note: In order to cause the qmaster to delete the
            usage for this job, we zero out UPU_old_usage_list
            for this job in the UP_debited_job_usage list */

         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                             lGetUlong(job, JB_job_number)))) {
            lSetList(upu, UPU_old_usage_list, NULL);
            lSetUlong(project, UP_usage_seqno, seqno);
         }
      }
   }

   DEXIT;
   return;
}


/*--------------------------------------------------------------------
 * combine_usage - combines a node's associated user/project usage 
 * into a single value and stores it in the node.
 *--------------------------------------------------------------------*/

void
combine_usage( sge_ref_t *ref )
{
   double usage_value = 0;

   /*-------------------------------------------------------------
    * Get usage from associated user/project object
    *-------------------------------------------------------------*/

#if 0
   userprj = ref->share_tree_type == STT_PROJECT ? ref->project : ref->user;
#endif

   if (ref->node) {

      lList *usage_weight_list=NULL, *usage_list=NULL;
      lListElem *usage_weight, *config, *usage_elem;
      double sum_of_usage_weights = 0;
      const char *usage_name;

      /*-------------------------------------------------------------
       * Sum usage weighting factors
       *-------------------------------------------------------------*/

      if ((config = lFirst(all_lists->config_list))) {
         usage_weight_list = lGetList(config, SC_usage_weight_list);
         if (usage_weight_list) {
            for_each(usage_weight, usage_weight_list)
               sum_of_usage_weights += lGetDouble(usage_weight, UA_value);
         }
      }

      /*-------------------------------------------------------------
       * Combine user/project usage based on usage weighting factors
       *-------------------------------------------------------------*/

      if (usage_weight_list) {

         if (ref->user) {
            if (ref->project) {
               lList *upp_list = lGetList(ref->user, UP_project);
               lListElem *upp;
               if (upp_list &&
                   ((upp = lGetElemStr(upp_list, UPP_name,
                                       lGetString(ref->project, UP_name)))))
                  usage_list = lGetList(upp, UPP_usage);
            } else
               usage_list = lGetList(ref->user, UP_usage);
         } else if (ref->project) /* not sure about this, use it when? */
            usage_list = lGetList(ref->project, UP_usage);

         for_each(usage_elem, usage_list) {
            usage_name = lGetString(usage_elem, UA_name);
            usage_weight = lGetElemStr(usage_weight_list, UA_name,
                                       usage_name);
            if (usage_weight && sum_of_usage_weights>0) {
               usage_value += lGetDouble(usage_elem, UA_value) *
                  (lGetDouble(usage_weight, UA_value) /
                   sum_of_usage_weights);
            }
         }
      }

      /*-------------------------------------------------------------
       * Set combined usage in the node
       *-------------------------------------------------------------*/

      lSetDouble(ref->node, STN_combined_usage, usage_value);

   }

   return;
}


/*--------------------------------------------------------------------
 * decay_and_sum_usage - accumulates and decays usage in the correct
 * user and project objects for the specified job
 *--------------------------------------------------------------------*/

void
decay_and_sum_usage( sge_ref_t *ref,
                     lList *decay_list,
                     u_long seqno,
                     u_long curr_time )
{
   lList *job_usage_list=NULL,
         *old_usage_list=NULL,
         *user_usage_list=NULL,
         *project_usage_list=NULL,
         *user_long_term_usage_list=NULL,
         *project_long_term_usage_list=NULL;
   lListElem *job=ref->job,
             *ja_task=ref->ja_task,
             *node=ref->node,
             *user=ref->user,
             *project=ref->project,
             *userprj = NULL,
             *task;

   if (!node && !user && !project)
      return;

#if 0
   if (ref->share_tree_type == STT_PROJECT)
      userprj = ref->project;
   else
      userprj = ref->user;
#endif

   if (ref->user)
      userprj = ref->user;
   else if (ref->project)
      userprj = ref->project;

   /*-------------------------------------------------------------
    * Decay the usage for the associated user and project
    *-------------------------------------------------------------*/
    
   if (user)
      decay_userprj_usage(user, decay_list, seqno, curr_time);

   if (project)
      decay_userprj_usage(project, decay_list, seqno, curr_time);

   /*-------------------------------------------------------------
    * Note: Since CODINE will update job.usage directly, we 
    * maintain the job usage the last time we collected it from
    * the job.  The difference between the new usage and the old
    * usage is what needs to be added to the user or project node.
    * This old usage is maintained in the user or project node
    * depending on the type of share tree.
    *-------------------------------------------------------------*/

   if (ja_task != NULL) {
      job_usage_list = lCopyList(NULL, lGetList(ja_task, JAT_scaled_usage_list));

      /* sum sub-task usage into job_usage_list */
      if (job_usage_list) {
         for_each(task, lGetList(ja_task, JAT_task_list)) {
            lListElem *dst, *src;
            for_each(src, lGetList(ja_task, JAT_scaled_usage_list)) {
               if ((dst=lGetElemStr(job_usage_list, UA_name, lGetString(src, UA_name))))
                  lSetDouble(dst, UA_value, lGetDouble(dst, UA_value) + lGetDouble(src, UA_value));
               else
                  lAppendElem(job_usage_list, lCopyElem(src));
            }
         }
      }
   }

   if (userprj) {
      lListElem *upu;
      lList *upu_list = lGetList(userprj, UP_debited_job_usage);
      if (upu_list) {
         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                                  lGetUlong(job, JB_job_number)))) {
            if ((old_usage_list = lGetList(upu, UPU_old_usage_list))) {
               old_usage_list = lCopyList("", old_usage_list);
            }
         }
      }
   }

   if (!old_usage_list)
      old_usage_list = build_usage_list("old_usage_list", NULL);

   if (user) {

      /* if there is a user & project, usage is kept in the project sub-list */

      if (project) {
         lList *upp_list = lGetList(user, UP_project);
         lListElem *upp;
         const char *project_name = lGetString(project, UP_name);

         if (!upp_list) {
            upp_list = lCreateList("", UPP_Type);
            lSetList(user, UP_project, upp_list);
         }
         if (!((upp = lGetElemStr(upp_list, UPP_name, project_name))))
            upp = lAddElemStr(&upp_list, UPP_name, project_name, UPP_Type);
         user_long_term_usage_list = lGetList(upp, UPP_long_term_usage);
         if (!user_long_term_usage_list) {
            user_long_term_usage_list = 
                  build_usage_list("upp_long_term_usage_list", NULL);
            lSetList(upp, UPP_long_term_usage, user_long_term_usage_list);
         }
         user_usage_list = lGetList(upp, UPP_usage);
         if (!user_usage_list) {
            user_usage_list = build_usage_list("upp_usage_list", NULL);
            lSetList(upp, UPP_usage, user_usage_list);
         }

      } else {
         user_long_term_usage_list = lGetList(user, UP_long_term_usage);
         if (!user_long_term_usage_list) {
            user_long_term_usage_list = 
                  build_usage_list("user_long_term_usage_list", NULL);
            lSetList(user, UP_long_term_usage, user_long_term_usage_list);
         }
         user_usage_list = lGetList(user, UP_usage);
         if (!user_usage_list) {
            user_usage_list = build_usage_list("user_usage_list", NULL);
            lSetList(user, UP_usage, user_usage_list);
         }
      }
   }

   if (project) {
      project_long_term_usage_list = lGetList(project, UP_long_term_usage);
      if (!project_long_term_usage_list) {
         project_long_term_usage_list =
              build_usage_list("project_long_term_usage_list", NULL);
         lSetList(project, UP_long_term_usage, project_long_term_usage_list);
      }
      project_usage_list = lGetList(project, UP_usage);
      if (!project_usage_list) {
         project_usage_list = build_usage_list("project_usage_list", 
                                                NULL);
         lSetList(project, UP_usage, project_usage_list);
      }
   }

   if (job_usage_list) {

      lListElem *job_usage;

      /*-------------------------------------------------------------
       * Add to node usage for each usage type
       *-------------------------------------------------------------*/

      for_each(job_usage, job_usage_list) {

         lListElem *old_usage=NULL, 
                   *user_usage=NULL, *project_usage=NULL,
                   *user_long_term_usage=NULL,
                   *project_long_term_usage=NULL;
         const char *usage_name = lGetString(job_usage, UA_name);

         /* only copy CPU, memory, and I/O usage */
         /* or usage explicitly in decay list */
         if (strcmp(usage_name, USAGE_ATTR_CPU) != 0 &&
             strcmp(usage_name, USAGE_ATTR_MEM) != 0 &&
             strcmp(usage_name, USAGE_ATTR_IO) != 0 &&
             !lGetElemStr(decay_list, UA_name, usage_name))
             continue;

         /*---------------------------------------------------------
          * Locate the corresponding usage element for the job
          * usage type in the node usage, old job usage, user usage,
          * and project usage.  If it does not exist, create a new
          * corresponding usage element.
          *---------------------------------------------------------*/

         if (old_usage_list) {
            old_usage = get_usage(old_usage_list, usage_name);
            if (!old_usage) {
               old_usage = create_usage_elem(usage_name);
               lAppendElem(old_usage_list, old_usage);
            }
         }

         if (user_usage_list) {
            user_usage = get_usage(user_usage_list, usage_name);
            if (!user_usage) {
               user_usage = create_usage_elem(usage_name);
               lAppendElem(user_usage_list, user_usage);
            }
         }

         if (user_long_term_usage_list) {
            user_long_term_usage = get_usage(user_long_term_usage_list,
                                                       usage_name);
            if (!user_long_term_usage) {
               user_long_term_usage = create_usage_elem(usage_name);
               lAppendElem(user_long_term_usage_list, user_long_term_usage);
            }
         }

         if (project_usage_list) {
            project_usage = get_usage(project_usage_list, usage_name);
            if (!project_usage) {
               project_usage = create_usage_elem(usage_name);
               lAppendElem(project_usage_list, project_usage);
            }
         }

         if (project_long_term_usage_list) {
            project_long_term_usage =
                  get_usage(project_long_term_usage_list, usage_name);
            if (!project_long_term_usage) {
               project_long_term_usage = create_usage_elem(usage_name);
               lAppendElem(project_long_term_usage_list,
			   project_long_term_usage);
            }
         }

         if (job_usage && old_usage) {

            double usage_value;

            usage_value = MAX(lGetDouble(job_usage, UA_value) -
                              lGetDouble(old_usage, UA_value), 0);
                                     
            /*---------------------------------------------------
             * Add usage to decayed user usage
             *---------------------------------------------------*/

            if (user_usage)
                lSetDouble(user_usage, UA_value,
                      lGetDouble(user_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to long term user usage
             *---------------------------------------------------*/

            if (user_long_term_usage)
                lSetDouble(user_long_term_usage, UA_value,
                      lGetDouble(user_long_term_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to decayed project usage
             *---------------------------------------------------*/

            if (project_usage)
                lSetDouble(project_usage, UA_value,
                      lGetDouble(project_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to long term project usage
             *---------------------------------------------------*/

            if (project_long_term_usage)
                lSetDouble(project_long_term_usage, UA_value,
                      lGetDouble(project_long_term_usage, UA_value) +
                      usage_value);


         }

      }

   }

   /*-------------------------------------------------------------
    * save off current job usage in debitted job usage list
    *-------------------------------------------------------------*/

   if (old_usage_list)
      lFreeList(old_usage_list);

   if (job_usage_list) {
      if (userprj) {
         lListElem *upu;
         u_long jobnum = lGetUlong(job, JB_job_number);
         lList *upu_list = lGetList(userprj, UP_debited_job_usage);
         if (!upu_list) {
            upu_list = lCreateList("", UPU_Type);
            lSetList(userprj, UP_debited_job_usage, upu_list);
         }
         if ((upu = lGetElemUlong(upu_list, UPU_job_number, jobnum))) {
            lSetList(upu, UPU_old_usage_list,
                     lCopyList(lGetListName(job_usage_list), job_usage_list));
         } else {
            upu = lCreateElem(UPU_Type);
            lSetUlong(upu, UPU_job_number, jobnum);
            lSetList(upu, UPU_old_usage_list,
                     lCopyList(lGetListName(job_usage_list), job_usage_list));
            lAppendElem(upu_list, upu);
         }
      }
      lFreeList(job_usage_list);
   }

}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass0 - performs pass 0 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void
calc_job_share_tree_tickets_pass0( sge_ref_t *ref,
                                   double *sum_m_share,
                                   double *sum_proportion,
                                   u_long seqno )
{
   lListElem *node = ref->node;
   double node_m_share, node_proportion, node_usage=0;

   /*-------------------------------------------------------------
    * Note: The seqno is a global or parameter that is incremented
    * on each sgeee scheduling interval. It is checked against
    * node.pass0_seqno so that the user or project node
    * calculations are only done once per node.
    *-------------------------------------------------------------*/

   if (node && seqno != lGetUlong(node, STN_pass0_seqno)) {

       node_m_share = lGetDouble(node, STN_m_share);
       node_usage = lGetDouble(node, STN_combined_usage);
       if (node_usage < SGE_MIN_USAGE)
           node_usage = SGE_MIN_USAGE * node_m_share;
       node_proportion = node_m_share * node_m_share / node_usage;
       lSetDouble(node, STN_proportion, node_proportion);
       *sum_proportion += node_proportion;
       *sum_m_share += node_m_share;
       lSetUlong(node, STN_pass0_seqno, seqno);

   }

}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass1 - performs pass 1 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void
calc_job_share_tree_tickets_pass1( sge_ref_t *ref,
                                   double sum_m_share,
                                   double sum_proportion,
                                   double *sum_adjusted_proportion,
                                   u_long seqno )
{
   lListElem *job = ref->job;
   lListElem *node = ref->node;
   double target_proportion=0, current_proportion=0,
          adjusted_usage, m_share, adjusted_proportion=0,
          compensation_factor;

   if (!node) {
      return;
   }

   if (classic_sgeee_scheduling) {

      /*-------------------------------------------------------
       * calculate targeted proportion of node
       *-------------------------------------------------------*/

      m_share = lGetDouble(node, STN_m_share);
      if (sum_m_share)
         target_proportion = m_share / sum_m_share;
      lSetDouble(node, STN_target_proportion, target_proportion);

      /*-------------------------------------------------------
       * calculate current proportion of node
       *-------------------------------------------------------*/

      if (sum_proportion)
         current_proportion = lGetDouble(node, STN_proportion) / sum_proportion;
      lSetDouble(node, STN_current_proportion, current_proportion);

      /*-------------------------------------------------------
       * adjust proportion based on compensation factor
       *-------------------------------------------------------*/

      compensation_factor = lGetDouble(lFirst(all_lists->config_list),
                     SC_compensation_factor);
      if (target_proportion > 0 && compensation_factor > 0 &&
         current_proportion > (compensation_factor * target_proportion))

         adjusted_usage = MAX(lGetDouble(node, STN_combined_usage),
                              SGE_MIN_USAGE * m_share) *
                (current_proportion /
                (compensation_factor * target_proportion));
      else
         adjusted_usage = lGetDouble(node, STN_combined_usage);

      lSetDouble(node, STN_adjusted_usage, adjusted_usage);

      if (adjusted_usage < SGE_MIN_USAGE)
         adjusted_usage = SGE_MIN_USAGE * m_share;

      if (seqno != lGetUlong(node, STN_pass1_seqno)) {
         lSetUlong(node, STN_sum_priority, 0);
         if (adjusted_usage > 0)
            adjusted_proportion = m_share * m_share / adjusted_usage;
         lSetDouble(node, STN_adjusted_proportion, adjusted_proportion);
         *sum_adjusted_proportion += adjusted_proportion;
         lSetUlong(node, STN_pass1_seqno, seqno);
      }

   }

   /*-------------------------------------------------------
    * sum POSIX priorities for each job for use in pass 2
    *-------------------------------------------------------*/

   lSetUlong(node, STN_sum_priority,
             lGetUlong(node, STN_sum_priority) +
             lGetUlong(job, JB_priority));
}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass2 - performs pass 2 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void
calc_job_share_tree_tickets_pass2( sge_ref_t *ref,
                                   double sum_adjusted_proportion,
                                   double total_share_tree_tickets,
                                   u_long seqno )
{
   double share_tree_tickets;
   lListElem *job = ref->job;
   lListElem *node = ref->node;

   if (!node) {
      return;
   }

   /*-------------------------------------------------------
    * calculate the number of share tree tickets for this node
    *-------------------------------------------------------*/

   if (classic_sgeee_scheduling && seqno != lGetUlong(node, STN_pass2_seqno)) {

       double adjusted_current_proportion=0;

       if (sum_adjusted_proportion>0)
          adjusted_current_proportion =
                  lGetDouble(node, STN_adjusted_proportion) /
                  sum_adjusted_proportion;
       lSetDouble(node, STN_adjusted_current_proportion,
                  adjusted_current_proportion);
       lSetUlong(node, STN_pass2_seqno, seqno);
   }

   /*-------------------------------------------------------
    * calculate the number of share tree tickets for this job
    *-------------------------------------------------------*/

   share_tree_tickets = lGetDouble(node, STN_adjusted_current_proportion) *
                        total_share_tree_tickets;

   if (lGetUlong(node, STN_sum_priority)) {
      REF_SET_STICKET(ref, 
                (u_long)((double)lGetUlong(job, JB_priority) *
                share_tree_tickets /
                lGetUlong(node, STN_sum_priority)));
   } else {
      REF_SET_STICKET(ref,
                share_tree_tickets / lGetUlong(node, STN_job_ref_count));
   }
}


/*--------------------------------------------------------------------
 * calc_functional_tickets_pass1 - performs pass 1 of calculating
 *      the functional tickets for the specified job
 *--------------------------------------------------------------------*/

void
calc_job_functional_tickets_pass1( sge_ref_t *ref,
                                   double *sum_of_user_functional_shares,
                                   double *sum_of_project_functional_shares,
                                   double *sum_of_department_functional_shares,
                                   double *sum_of_jobclass_functional_shares,
                                   double *sum_of_job_functional_shares,
                                   int shared,
                                   int include_queued_jobs )
{
   double job_cnt;

   /*-------------------------------------------------------------
    * Sum user functional shares
    *-------------------------------------------------------------*/

   if (ref->user) {
      job_cnt = lGetUlong(ref->user, UP_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->user, UP_pending_job_cnt);
      ref->user_fshare = shared ?
            (double)lGetUlong(ref->user, UP_fshare) / job_cnt :
            lGetUlong(ref->user, UP_fshare);
      *sum_of_user_functional_shares += ref->user_fshare;
   }

   /*-------------------------------------------------------------
    * Sum project functional shares
    *-------------------------------------------------------------*/

   if (ref->project) {
      job_cnt = lGetUlong(ref->project, UP_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->project, UP_pending_job_cnt);
      ref->project_fshare = shared ?
            (double)lGetUlong(ref->project, UP_fshare) / job_cnt :
            lGetUlong(ref->project, UP_fshare);
      *sum_of_project_functional_shares += ref->project_fshare;
   }

   /*-------------------------------------------------------------
    * Sum department functional shares
    *-------------------------------------------------------------*/

   if (ref->dept) {
      job_cnt = lGetUlong(ref->dept, US_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->dept, US_pending_job_cnt);
      ref->dept_fshare = shared ?
            (double)lGetUlong(ref->dept, US_fshare) / job_cnt :
            lGetUlong(ref->dept, US_fshare);
      *sum_of_department_functional_shares += ref->dept_fshare;
   }

   /*-------------------------------------------------------------
    * Sum job class functional shares
    *-------------------------------------------------------------*/

   if (ref->task_jobclass) {
      int i;
      for(i=0; i<ref->num_task_jobclasses; i++)
         if (ref->task_jobclass[i]) {
            job_cnt = lGetUlong(ref->task_jobclass[i], QU_job_cnt);
            if (include_queued_jobs)
               job_cnt += lGetUlong(ref->task_jobclass[i], QU_pending_job_cnt);
            if (shared)
               *sum_of_jobclass_functional_shares +=
                     (double)lGetUlong(ref->task_jobclass[i], QU_fshare) / job_cnt;
            else
               *sum_of_jobclass_functional_shares +=
                     lGetUlong(ref->task_jobclass[i], QU_fshare);
         }
   } else if (ref->jobclass) {
      job_cnt = lGetUlong(ref->jobclass, QU_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->jobclass, QU_pending_job_cnt);
      ref->jobclass_fshare = shared ?
            (double)lGetUlong(ref->jobclass, QU_fshare) / job_cnt :
            lGetUlong(ref->jobclass, QU_fshare);
      *sum_of_jobclass_functional_shares += ref->jobclass_fshare;
   }

   /*-------------------------------------------------------------
    * Sum job functional shares
    *-------------------------------------------------------------*/

   REF_SET_FSHARE(ref, lGetUlong(ref->job, JB_priority));

   *sum_of_job_functional_shares += REF_GET_FSHARE(ref);
}

enum {
   k_user=0,
   k_department,
   k_project,
   k_jobclass,
   k_job,
   k_last
};


/*--------------------------------------------------------------------
 * get_functional_weighting_parameters - get the weighting parameters for
 *      the functional policy categories from the configuration
 *--------------------------------------------------------------------*/


static void
get_functional_weighting_parameters( double sum_of_user_functional_shares,
                                     double sum_of_project_functional_shares,
                                     double sum_of_department_functional_shares,
                                     double sum_of_jobclass_functional_shares,
                                     double sum_of_job_functional_shares,
                                     double weight[] )
{
   double k_sum;
   lListElem *config;
   memset(weight, 0, sizeof(double)*k_last);

   if ((config = lFirst(all_lists->config_list))) {

      if (sum_of_user_functional_shares > 0)
         weight[k_user] = lGetDouble(config, SC_weight_user);
      if (sum_of_department_functional_shares > 0)
         weight[k_department] = lGetDouble(config, SC_weight_department);
      if (sum_of_project_functional_shares > 0)
         weight[k_project] = lGetDouble(config, SC_weight_project);
      if (sum_of_jobclass_functional_shares > 0)
         weight[k_jobclass] = lGetDouble(config, SC_weight_jobclass);
      if (sum_of_job_functional_shares > 0)
         weight[k_job] = lGetDouble(config, SC_weight_job);
      k_sum = weight[k_user] + weight[k_department] + weight[k_project] +
              weight[k_jobclass] + weight[k_job];
   } else {
      weight[k_user] = 1;
      weight[k_department] = 1;
      weight[k_project] = 1;
      weight[k_jobclass] = 1;
      weight[k_job] = 1;
      k_sum = weight[k_user] + weight[k_department] + weight[k_project] +
              weight[k_jobclass] + weight[k_job];
   }

   if (k_sum>0) {
      weight[k_user] /= k_sum;
      weight[k_department] /= k_sum;
      weight[k_project] /= k_sum;
      weight[k_jobclass] /= k_sum;
      weight[k_job] /= k_sum;
   }

   return;
}


/*--------------------------------------------------------------------
 * calc_functional_tickets_pass2 - performs pass 2 of calculating
 *      the functional tickets for the specified job
 *--------------------------------------------------------------------*/

double
calc_job_functional_tickets_pass2( sge_ref_t *ref,
                                   double sum_of_user_functional_shares,
                                   double sum_of_project_functional_shares,
                                   double sum_of_department_functional_shares,
                                   double sum_of_jobclass_functional_shares,
                                   double sum_of_job_functional_shares,
                                   double total_functional_tickets,
                                   double weight[],
                                   int shared,
                                   int include_queued_jobs )
{
   double user_functional_tickets=0,
          project_functional_tickets=0,
          department_functional_tickets=0,
          jobclass_functional_tickets=0,
          job_functional_tickets=0,
          total_job_functional_tickets;

   /*-------------------------------------------------------
    * calculate user functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->user && sum_of_user_functional_shares)
      user_functional_tickets = (ref->user_fshare *
                                total_functional_tickets /
                                sum_of_user_functional_shares);

   /*-------------------------------------------------------
    * calculate project functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->project && sum_of_project_functional_shares)
      project_functional_tickets = (ref->project_fshare *
                                 total_functional_tickets /
                                 sum_of_project_functional_shares);

   /*-------------------------------------------------------
    * calculate department functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->dept && sum_of_department_functional_shares)
      department_functional_tickets = (ref->dept_fshare *
                                 total_functional_tickets /
                                 sum_of_department_functional_shares);

   /*-------------------------------------------------------
    * calculate job class functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->task_jobclass && sum_of_jobclass_functional_shares) {
      int i;
      double qshares=0, job_cnt;
      for(i=0; i<ref->num_task_jobclasses; i++)
         if (ref->task_jobclass[i]) {
            job_cnt = lGetUlong(ref->task_jobclass[i], QU_job_cnt);
            if (include_queued_jobs)
               job_cnt += lGetUlong(ref->task_jobclass[i], QU_pending_job_cnt);
            qshares += shared ?
                  (double)lGetUlong(ref->task_jobclass[i], QU_fshare) / job_cnt :
                  lGetUlong(ref->task_jobclass[i], QU_fshare);
         }

      /* NOTE: Should we divide qshares by the number of task job classes
               to get an average across the associated queues? */

      jobclass_functional_tickets = (qshares *
                                 total_functional_tickets /
                                 sum_of_jobclass_functional_shares);

   } else if (ref->jobclass && sum_of_jobclass_functional_shares)
      jobclass_functional_tickets = (ref->jobclass_fshare *
                                 total_functional_tickets /
                                 sum_of_jobclass_functional_shares);

   /*-------------------------------------------------------
    * calculate job functional tickets for this job
    *-------------------------------------------------------*/

   if (sum_of_job_functional_shares)
      job_functional_tickets = ((double)lGetUlong(ref->job, JB_priority) *
                                 (double)total_functional_tickets /
                                  sum_of_job_functional_shares);

   /*-------------------------------------------------------
    * calculate functional tickets for PE tasks
    *-------------------------------------------------------*/

   if (ref->ja_task && ref->task_jobclass) {
      lListElem *granted_pe;
      int i=0;
      double task_jobclass_functional_tickets;
#if 0
      double total_task_functional_tickets;

      total_task_functional_tickets = (weight[k_user] * user_functional_tickets +
                 weight[k_department] * department_functional_tickets +
                 weight[k_project] * project_functional_tickets +
                 weight[k_job] * job_functional_tickets) / ref->num_active_task_jobclasses;
#endif

      for_each(granted_pe, lGetList(ref->ja_task, JAT_granted_destin_identifier_list)) {
         if (ref->task_jobclass[i]) {
            if (sum_of_jobclass_functional_shares) {
               double task_jobclass_fshare, job_cnt;
               job_cnt = lGetUlong(ref->task_jobclass[i], QU_job_cnt);
               if (include_queued_jobs)
                  job_cnt += lGetUlong(ref->task_jobclass[i], QU_pending_job_cnt);
               task_jobclass_fshare = shared ?
                  (double)lGetUlong(ref->task_jobclass[i], QU_fshare) / job_cnt :
                  lGetUlong(ref->task_jobclass[i], QU_fshare);
               task_jobclass_functional_tickets = (task_jobclass_fshare *
                                        total_functional_tickets /
                                        (double)sum_of_jobclass_functional_shares);
            } else
               task_jobclass_functional_tickets = 0;
#if 0
            lSetDouble(granted_pe, JG_fticket,
                        total_task_functional_tickets + weight[k_jobclass] * task_jobclass_functional_tickets);
#endif
            lSetDouble(granted_pe, JG_jcfticket, weight[k_jobclass] * task_jobclass_functional_tickets);
         } else
            lSetDouble(granted_pe, JG_jcfticket, 0);
         i++;
      }
   }

   /*-------------------------------------------------------
    * calculate functional tickets for this job
    *-------------------------------------------------------*/

   total_job_functional_tickets = weight[k_user] * user_functional_tickets +
             weight[k_department] * department_functional_tickets +
             weight[k_project] * project_functional_tickets +
             weight[k_jobclass] * jobclass_functional_tickets +
             weight[k_job] * job_functional_tickets;

   ref->total_jobclass_ftickets = weight[k_jobclass] * jobclass_functional_tickets;
   REF_SET_FTICKET(ref, total_job_functional_tickets);

   return job_functional_tickets;
}


/*--------------------------------------------------------------------
 * calc_deadline_tickets_pass1 - performs pass 1 of calculating
 *      the deadline tickets for the specified job
 *--------------------------------------------------------------------*/

double
calc_job_deadline_tickets_pass1 ( sge_ref_t *ref,
                                  double total_deadline_tickets,
                                  u_long current_time )
{
   lListElem *job = ref->job;
   double job_deadline_tickets;
   u_long job_start_time,
          job_deadline;        /* deadline initiation time */

   /*-------------------------------------------------------
    * If job has not started, set deadline tickets to zero
    *-------------------------------------------------------*/

   if ((job_start_time = lGetUlong(job, JB_execution_time)) == 0)
       job_start_time = lGetUlong(job, JB_submission_time);
   job_deadline = lGetUlong(job, JB_deadline);

   if (job_deadline == 0 ||
       job_start_time == 0 ||
       current_time <= job_start_time)

      job_deadline_tickets = 0;

   /*-------------------------------------------------------
    * If job has started and deadline is in future, set
    * deadline tickets based on time left till deadline.
    *-------------------------------------------------------*/

   else if (current_time < job_deadline)

      job_deadline_tickets = (double)(current_time - job_start_time) * 
                             (double)total_deadline_tickets /
                             (job_deadline - job_start_time);

   /*-------------------------------------------------------
    * If deadline is in past, set deadline tickets to
    * maximum available deadline tickets.
    *-------------------------------------------------------*/

   else
      job_deadline_tickets = total_deadline_tickets;


   /*-------------------------------------------------------
    * Set the number of deadline tickets in the job
    *-------------------------------------------------------*/

   REF_SET_DTICKET(ref, job_deadline_tickets);

   return job_deadline_tickets;
}


/*--------------------------------------------------------------------
 * calc_deadline_tickets_pass2 - performs pass 2 of calculating
 *      the deadline tickets for the specified job.  Only called
 *      if sum_of_deadline_tickets for pass 1 is greater than
 *      total_deadline_tickets.
 *--------------------------------------------------------------------*/

double calc_job_deadline_tickets_pass2 ( sge_ref_t *ref,
                                         double sum_of_deadline_tickets,
                                         double total_deadline_tickets )
{
   double job_deadline_tickets;

   job_deadline_tickets = REF_GET_DTICKET(ref);

   /*-------------------------------------------------------------
    * Scale deadline tickets based on total number of deadline
    * tickets.
    *-------------------------------------------------------------*/

   if (job_deadline_tickets > 0 && share_deadline_tickets) {
      job_deadline_tickets = job_deadline_tickets *
                             (total_deadline_tickets /
                             sum_of_deadline_tickets);
      REF_SET_DTICKET(ref, job_deadline_tickets);
   }

   return job_deadline_tickets;
}


/*--------------------------------------------------------------------
 * calc_override_tickets - calculates the number of override tickets for the
 * specified job
 *--------------------------------------------------------------------*/

double
calc_job_override_tickets( sge_ref_t *ref,
                           int shared,
                           int include_queued_jobs)
{
   double job_override_tickets = 0;
   double otickets, job_cnt;

   DENTER(TOP_LAYER, "calc_job_override_tickets");

   /*-------------------------------------------------------
    * job.override_tickets = user.override_tickets +
    *                        project.override_tickets +
    *                        department.override_tickets +
    *                        jobclass.override_tickets +
    *                        job.override_tickets;
    *-------------------------------------------------------*/

   if (ref->user) {
      job_cnt = lGetUlong(ref->user, UP_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->user, UP_pending_job_cnt);
      if (((otickets = lGetUlong(ref->user, UP_oticket)) &&
          ((job_cnt = shared ? job_cnt : 1))))
         job_override_tickets += (otickets / job_cnt);
   }

   if (ref->project) {
      job_cnt = lGetUlong(ref->project, UP_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->project, UP_pending_job_cnt);
      if (((otickets = lGetUlong(ref->project, UP_oticket)) &&
          ((job_cnt = shared ? job_cnt : 1))))
         job_override_tickets += (otickets / job_cnt);
   }

   if (ref->dept) {
      job_cnt = lGetUlong(ref->dept, US_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->dept, US_pending_job_cnt);
      if (((otickets = lGetUlong(ref->dept, US_oticket)) &&
          ((job_cnt = shared ? job_cnt : 1))))
         job_override_tickets += (otickets / job_cnt);
   }

   job_override_tickets += lGetUlong(ref->job, JB_override_tickets);

   if (ref->ja_task && ref->task_jobclass) {
      lListElem *granted_pe;
      double jobclass_otickets;
      int i=0;
      ref->total_jobclass_otickets=0;
      for_each(granted_pe, lGetList(ref->ja_task, JAT_granted_destin_identifier_list)) {
         jobclass_otickets = 0;
         if (ref->task_jobclass[i]) {
            job_cnt = lGetUlong(ref->task_jobclass[i], QU_job_cnt);
            if (include_queued_jobs)
               job_cnt += lGetUlong(ref->task_jobclass[i], QU_pending_job_cnt);
            if (((otickets = lGetUlong(ref->task_jobclass[i], QU_oticket)) &&
                ((job_cnt = shared ? job_cnt : 1))))
               jobclass_otickets = (otickets / (double)job_cnt);
#if 0
            lSetDouble(granted_pe, JG_oticket,
                  (job_override_tickets/(double)ref->num_active_task_jobclasses) +
                  jobclass_otickets);
#endif
            ref->total_jobclass_otickets += jobclass_otickets;
         }

         lSetDouble(granted_pe, JG_jcoticket, jobclass_otickets);
         i++;
      }
      job_override_tickets += ref->total_jobclass_otickets;
   } else if (ref->jobclass) {
      job_cnt = lGetUlong(ref->jobclass, QU_job_cnt);
      if (include_queued_jobs)
         job_cnt += lGetUlong(ref->jobclass, QU_pending_job_cnt);
      if (((otickets = lGetUlong(ref->jobclass, QU_oticket)) &&
          ((job_cnt = shared ? job_cnt : 1))))
         job_override_tickets += (otickets / job_cnt);
   }

   REF_SET_OTICKET(ref, job_override_tickets);

   DEXIT;
   return job_override_tickets;
}


/*--------------------------------------------------------------------
 * calc_job_tickets - calculates the total number of tickets for
 * the specified job
 *--------------------------------------------------------------------*/

double
calc_job_tickets ( sge_ref_t *ref )
{
   lList *granted;
   lListElem *job = ref->job,
             *ja_task = ref->ja_task;
   double job_tickets;
   lListElem *pe, *granted_el;
   const char *pe_str;

   /*-------------------------------------------------------------
    * Sum up all tickets for job
    *-------------------------------------------------------------*/

   job_tickets = REF_GET_STICKET(ref) + REF_GET_FTICKET(ref) +
                 REF_GET_DTICKET(ref) + REF_GET_OTICKET(ref);

   REF_SET_TICKET(ref, job_tickets);

   /* for PE slave-controlled jobs, set tickets for each granted queue */

   if (ja_task && (pe_str=lGetString(ja_task, JAT_granted_pe))
         && (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str))
         && lGetUlong(pe, PE_control_slaves)
         && (granted=lGetList(ja_task, JAT_granted_destin_identifier_list))) {

      double job_tickets_per_slot, job_dtickets_per_slot, job_otickets_per_slot,
             job_ftickets_per_slot, job_stickets_per_slot, nslots, active_nslots;

      /* Here we get the total number of slots granted on all queues (nslots) and the
         total number of slots granted on all queues with active subtasks or
         on which no subtasks have started yet (active_nslots). */

      nslots = nslots_granted(granted, NULL);
      active_nslots = active_nslots_granted(job, granted, NULL);

      /* If there are some active_nslots, then calculate the number of tickets
         per slot based on the active_nslots */

      if (active_nslots > 0)
         nslots = active_nslots;

      if (nslots > 0) {
         job_dtickets_per_slot = REF_GET_DTICKET(ref)/nslots;
         job_stickets_per_slot = REF_GET_STICKET(ref)/nslots;
      } else {
         job_dtickets_per_slot = 0;
         job_stickets_per_slot = 0;
      }

      for_each(granted_el, granted) {
         double slots;

         /* Only give tickets to queues with active sub-tasks or sub-tasks
            which have not started yet. */
            
         if (active_nslots > 0 && !active_subtasks(job, lGetString(granted_el, JG_qname)))
            slots = 0;
         else
            slots = lGetUlong(granted_el, JG_slots);
         
         if (nslots > 0) {
            job_ftickets_per_slot = (double)(REF_GET_FTICKET(ref) - ref->total_jobclass_ftickets)/nslots;
            job_otickets_per_slot = (double)(REF_GET_OTICKET(ref) - ref->total_jobclass_otickets)/nslots;
            job_tickets_per_slot = job_dtickets_per_slot + job_stickets_per_slot + job_ftickets_per_slot + job_otickets_per_slot;
         } else {
            job_ftickets_per_slot = 0;
            job_otickets_per_slot = 0;
            job_tickets_per_slot = 0;
         }

         lSetDouble(granted_el, JG_fticket, job_ftickets_per_slot*slots + lGetDouble(granted_el, JG_jcfticket));
         lSetDouble(granted_el, JG_oticket, job_otickets_per_slot*slots + lGetDouble(granted_el, JG_jcoticket));
         lSetDouble(granted_el, JG_dticket, job_dtickets_per_slot*slots);
         lSetDouble(granted_el, JG_sticket, job_stickets_per_slot*slots);
         lSetDouble(granted_el, JG_ticket, job_tickets_per_slot*slots +
                   lGetDouble(granted_el, JG_jcoticket) + lGetDouble(granted_el, JG_jcfticket));

      }
   }

   return job_tickets;
}


/*--------------------------------------------------------------------
 * sge_clear_job - clear tickets for job
 *--------------------------------------------------------------------*/

void
sge_clear_job( lListElem *job )
{
   lListElem *ja_task;

   for_each(ja_task, lGetList(job, JB_ja_tasks))
      sge_clear_ja_task(ja_task);
}

/*--------------------------------------------------------------------
 * sge_clear_ja_task - clear tickets for job task
 *--------------------------------------------------------------------*/

void
sge_clear_ja_task( lListElem *ja_task )
{
   lListElem *granted_el;

   lSetDouble(ja_task, JAT_ticket, 0);
   lSetDouble(ja_task, JAT_oticket, 0);
   lSetDouble(ja_task, JAT_dticket, 0);
   lSetDouble(ja_task, JAT_fticket, 0);
   lSetDouble(ja_task, JAT_sticket, 0);
   lSetDouble(ja_task, JAT_share, 0);
   for_each(granted_el, lGetList(ja_task, JAT_granted_destin_identifier_list)) {
      lSetDouble(granted_el, JG_ticket, 0);
      lSetDouble(granted_el, JG_oticket, 0);
      lSetDouble(granted_el, JG_fticket, 0);
      lSetDouble(granted_el, JG_dticket, 0);
      lSetDouble(granted_el, JG_sticket, 0);
      lSetDouble(granted_el, JG_jcoticket, 0);
      lSetDouble(granted_el, JG_jcfticket, 0);
   }
}

void
calc_pending_job_functional_tickets(sge_ref_t *ref,
                                    double *sum_of_user_functional_shares,
                                    double *sum_of_project_functional_shares,
                                    double *sum_of_department_functional_shares,
                                    double *sum_of_jobclass_functional_shares,
                                    double *sum_of_job_functional_shares,
                                    double total_functional_tickets,
                                    double weight[],
                                    int shared,
                                    int included_queued_jobs)
{
   double w[k_last];

   calc_job_functional_tickets_pass1(ref,
                                     sum_of_user_functional_shares,
                                     sum_of_project_functional_shares,
                                     sum_of_department_functional_shares,
                                     sum_of_jobclass_functional_shares,
                                     sum_of_job_functional_shares,
                                     share_functional_shares,
                                     0);

   get_functional_weighting_parameters(1, 1, 1, 1, 1, w);

   calc_job_functional_tickets_pass2(ref,
                                     *sum_of_user_functional_shares,
                                     *sum_of_project_functional_shares,
                                     *sum_of_department_functional_shares,
                                     *sum_of_jobclass_functional_shares,
                                     *sum_of_job_functional_shares,
                                     total_functional_tickets,
                                     w,
                                     share_functional_shares,
                                     0);

   return;
}

/*--------------------------------------------------------------------
 * sge_calc_tickets - calculate proportional shares in terms of tickets
 * for all active jobs.
 *
 * queued_jobs should contain all pending jobs to be scheduled
 * running_jobs should contain all currently executing jobs
 * finished_jobs should contain all completed jobs
 *--------------------------------------------------------------------*/

int
sge_calc_tickets( sge_Sdescr_t *lists,
                  lList *running_jobs,
                  lList *finished_jobs,
                  lList *queued_jobs,
                  int do_usage )
{
   double sum_of_deadline_tickets = 0,
          sum_of_user_functional_shares = 0,
          sum_of_project_functional_shares = 0,
          sum_of_department_functional_shares = 0,
          sum_of_jobclass_functional_shares = 0,
          sum_of_job_functional_shares = 0,
          sum_of_active_tickets = 0,
          sum_of_pending_tickets = 0,
          sum_of_active_override_tickets = 0;

   double sum_of_proportions = 0,
          sum_of_m_shares = 0,
          sum_of_adjusted_proportions = 0;

   u_long curr_time, num_jobs, num_queued_jobs, job_ndx;

   u_long32 num_unenrolled_tasks = 0;

   sge_ref_t *job_ref = NULL;

   lListElem *job, *qep, *root = NULL;

   lListElem *sge_conf = lFirst(lists->config_list);

   double total_share_tree_tickets =
            (double)lGetUlong(sge_conf, SC_weight_tickets_share);
   double total_functional_tickets =
            (double)lGetUlong(sge_conf, SC_weight_tickets_functional);
   double total_deadline_tickets = 
            (double)lGetUlong(sge_conf, SC_weight_tickets_deadline);

   static int halflife = 0;

   lList *decay_list = NULL;

   double weight[k_last];

   u_long32 free_qslots = 0;

   DENTER(TOP_LAYER, "sge_calc_tickets");

   all_lists = lists;

   curr_time = sge_get_gmt();

   sge_scheduling_run++;
   
   if (halflife_decay_list) {
      lListElem *ep, *u;
      double decay_rate, decay_constant;
      for_each(ep, halflife_decay_list) {
         calculate_decay_constant(lGetDouble(ep, UA_value),
                                  &decay_rate, &decay_constant);
         u = lAddElemStr(&decay_list, UA_name, lGetString(ep, UA_name),
                         UA_Type); 
         lSetDouble(u, UA_value, decay_constant);
      }
   } else {
      lListElem *u;
      double decay_rate, decay_constant;
      calculate_decay_constant(-1, &decay_rate, &decay_constant);
      u = lAddElemStr(&decay_list, UA_name, "finished_jobs", UA_Type); 
      lSetDouble(u, UA_value, decay_constant);
   }

   /*-------------------------------------------------------------
    * Decay usage for all users and projects if halflife changes
    *-------------------------------------------------------------*/

   if (do_usage && sge_conf && halflife != lGetUlong(sge_conf, SC_halftime)) {
      lListElem *userprj;
      int oldhalflife = halflife;
      halflife = lGetUlong(sge_conf, SC_halftime);
      /* decay up till now based on old half life (unless it's zero),
         all future decay will be based on new halflife */
      if (oldhalflife == 0)
         calculate_default_decay_constant(halflife);
      else
         calculate_default_decay_constant(oldhalflife);
      for_each(userprj, lists->user_list)
         decay_userprj_usage(userprj, decay_list, sge_scheduling_run, curr_time);
      for_each(userprj, lists->project_list)
         decay_userprj_usage(userprj, decay_list, sge_scheduling_run, curr_time);
   } else
      calculate_default_decay_constant(lGetUlong(sge_conf, SC_halftime));

   /*-------------------------------------------------------------
    * Init job_ref_count in each share tree node to zero
    *-------------------------------------------------------------*/

   if ((lists->share_tree))
      if ((root = lFirst(lists->share_tree))) {
         sge_init_share_tree_nodes(root);
         set_share_tree_project_flags(lists->project_list, root);
      }

   for_each(qep, lists->queue_list)
      free_qslots += MAX(0, lGetUlong(qep, QU_job_slots) - qslots_used(qep));

   /*-----------------------------------------------------------------
    * Create and build job reference array.  The job reference array
    * contains all the references of the jobs to the various objects
    * needed for scheduling. It exists so that the ticket calculation
    * function only has to look up the various objects once.
    *-----------------------------------------------------------------*/
   
   num_jobs = num_queued_jobs = 0;
   if (queued_jobs) {
      for_each(job, queued_jobs) {
         u_long32 max = MIN(max_pending_tasks_per_job, free_qslots+1);
         u_long32 num_unenrolled_tasks_for_job = 0;
         lListElem *ja_task;
         int task_cnt = 0;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if (++task_cnt > max) {
               break;
            }
            if (job_is_active(job, ja_task)) {
               num_queued_jobs++;
               num_jobs++;
            }
         }
         if (!(task_cnt > max)) {
            int space = max - task_cnt;
            int tasks = job_get_not_enrolled_ja_tasks(job);

            num_unenrolled_tasks_for_job = MIN(space, tasks);
            num_unenrolled_tasks += num_unenrolled_tasks_for_job;
            num_queued_jobs += num_unenrolled_tasks_for_job;
            num_jobs += num_unenrolled_tasks_for_job;
         }        
      }
   }
   if (running_jobs) {
      for_each(job, running_jobs) {
         lListElem *ja_task;
         for_each(ja_task, lGetList(job, JB_ja_tasks))
            if (job_is_active(job, ja_task))
               num_jobs++;
      }
   }
   if (num_jobs > 0) {
      job_ref = (sge_ref_t *)malloc(num_jobs * sizeof(sge_ref_t));
      memset(job_ref, 0, num_jobs * sizeof(sge_ref_t));
   }
   if (num_unenrolled_tasks > 0) {
      task_ref_initialize_table(num_unenrolled_tasks);
   }

   /*-----------------------------------------------------------------
    * Note: use of the the job_ref_t array assumes that no jobs
    * will be removed from the job lists and the job list order will
    * be maintained during the execution of sge_calc_tickets.
    *-----------------------------------------------------------------*/

   job_ndx = 0;

   if (running_jobs) {
      for_each(job, running_jobs) {
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if (!job_is_active(job, ja_task)) {
               sge_clear_ja_task(ja_task);
            } else {
               sge_set_job_refs(job, ja_task, &job_ref[job_ndx], NULL, lists, 0);
               if (job_ref[job_ndx].node) {
                  lSetUlong(job_ref[job_ndx].node, STN_job_ref_count,
                            lGetUlong(job_ref[job_ndx].node, STN_job_ref_count)+1);
                  lSetUlong(job_ref[job_ndx].node, STN_active_job_ref_count,
                            lGetUlong(job_ref[job_ndx].node, STN_active_job_ref_count)+1);
               }
               job_ndx++;
            }
         }
      }
   }

   if (queued_jobs) {
      u_long32 ja_task_ndx = 0;

      for_each(job, queued_jobs) {
         lListElem *ja_task = NULL;
         lListElem *range = NULL;
         lList *range_list = NULL;  
         u_long32 id;
         int task_cnt = 0;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            sge_ref_t *jref = &job_ref[job_ndx];

            if (++task_cnt > MIN(max_pending_tasks_per_job, free_qslots+1) ||
                !job_is_active(job, ja_task)) {
               sge_clear_ja_task(ja_task);
            } else {
               sge_set_job_refs(job, ja_task, jref, NULL, lists, 1);
               if (jref->node)
                  lSetUlong(jref->node, STN_job_ref_count,
                            lGetUlong(jref->node, STN_job_ref_count)+1);
               job_ndx++;
            }
         }

         range_list = lGetList(job, JB_ja_n_h_ids); 
         for_each(range, range_list) {
            for(id = lGetUlong(range, RN_min);
                id <= lGetUlong(range, RN_max);
                id += lGetUlong(range, RN_step)) {  
               sge_ref_t *jref = &job_ref[job_ndx];
               sge_task_ref_t *tref = task_ref_get_entry(ja_task_ndx);

               if (++task_cnt > MIN(max_pending_tasks_per_job, 
                                    free_qslots+1)) {
                  break;
               } 
               ja_task = job_get_ja_task_template_pending(job, id);
               sge_set_job_refs(job, ja_task, jref, tref, lists, 1); 
               if (jref->node) {
                  lSetUlong(jref->node, STN_job_ref_count,
                            lGetUlong(jref->node, STN_job_ref_count)+1);
               }
               ja_task_ndx++;
               job_ndx++;
            }
         }
      }
   }

   num_jobs = job_ndx;

   /*-------------------------------------------------------------
    * Calculate the m_shares in the share tree.  The m_share
    * is a hierarchichal share value used in the "classic
    * scheduling" mode. It is calculated by subdividing the shares
    * at each active level in the share tree.
    *-------------------------------------------------------------*/
     
   if (root) {
      update_job_ref_count(root);
      update_active_job_ref_count(root);
      lSetDouble(root, STN_m_share, 1.0);
      calculate_m_shares(root);
   }

   /*-----------------------------------------------------------------
    * Handle finished jobs. We add the finished job usage to the
    * user and project objects and then we delete the debited job
    * usage from the associated user/project object.  The debited
    * job usage is a usage list which is maintained in the user/project
    * object to indicate how much of the job's usage has been added
    * to the user/project usage so far.
    * We also add a usage value called finished_jobs to the job usage
    * so it will be summed and decayed in the user and project usage.
    * If the finished_jobs usage value already exists, then we know
    * we already handled this finished job, but the qmaster hasn't
    * been updated yet.
    *-----------------------------------------------------------------*/

   if (do_usage && finished_jobs) {
      for_each(job, finished_jobs) {
         sge_ref_t jref;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            memset(&jref, 0, sizeof(jref));
            sge_set_job_refs(job, ja_task, &jref, NULL, lists, 0);
            if (lGetElemStr(lGetList(jref.ja_task, JAT_scaled_usage_list),
                            UA_name, "finished_jobs") == NULL) {
               lListElem *u;
               if ((u=lAddSubStr(jref.ja_task, UA_name, "finished_jobs",
                                 JAT_scaled_usage_list, UA_Type)))
                  lSetDouble(u, UA_value, 1.0);
               decay_and_sum_usage(&jref, decay_list, sge_scheduling_run, curr_time);
               DPRINTF(("DDJU (0) "u32"."u32"\n",
                  lGetUlong(job, JB_job_number),
                  lGetUlong(ja_task, JAT_task_number)));
               delete_debited_job_usage(&jref, sge_scheduling_run);
            }
         }
      }
   } else {
      DPRINTF(("\n"));
      DPRINTF(("no DDJU: do_usage: %d finished_jobs %d\n",
         do_usage, (finished_jobs!=NULL)));
      DPRINTF(("\n"));
   }      

   /*-----------------------------------------------------------------
    * PASS 0
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGEEE Pass 0]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {

      sge_set_job_cnts(&job_ref[job_ndx], job_ref[job_ndx].queued);

      /*-------------------------------------------------------------
       * Handle usage
       *-------------------------------------------------------------*/

      if (do_usage)
         decay_and_sum_usage(&job_ref[job_ndx], decay_list, sge_scheduling_run, curr_time);

      combine_usage(&job_ref[job_ndx]);

      if (total_share_tree_tickets > 0 && classic_sgeee_scheduling)
         calc_job_share_tree_tickets_pass0(&job_ref[job_ndx],
                                           &sum_of_m_shares,
                                           &sum_of_proportions,
                                           sge_scheduling_run);

   }

   if (!classic_sgeee_scheduling && root)
      sge_calc_sharetree_targets(root, lists, decay_list,
                                 curr_time, sge_scheduling_run);

   /*-----------------------------------------------------------------
    * PASS 1
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGEEE Pass 1]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
      u_long job_deadline_time;

      if (!classic_sgeee_scheduling && job_ref[job_ndx].queued)
         break;

      job_deadline_time = lGetUlong(job_ref[job_ndx].job, JB_deadline);

      if (total_share_tree_tickets > 0)
         calc_job_share_tree_tickets_pass1(&job_ref[job_ndx],
                                           sum_of_m_shares,
                                           sum_of_proportions,
                                           &sum_of_adjusted_proportions,
                                           sge_scheduling_run);

      if (total_functional_tickets > 0)
         calc_job_functional_tickets_pass1(&job_ref[job_ndx],
                                          &sum_of_user_functional_shares,
                                          &sum_of_project_functional_shares,
                                          &sum_of_department_functional_shares,
                                          &sum_of_jobclass_functional_shares,
                                          &sum_of_job_functional_shares,
                                          share_functional_shares,
                                          classic_sgeee_scheduling ? 1 : 0);

      if (total_deadline_tickets > 0 && job_deadline_time > 0)
         sum_of_deadline_tickets +=
                  calc_job_deadline_tickets_pass1(&job_ref[job_ndx],
                                                  total_deadline_tickets,
                                                  curr_time);

   }

   get_functional_weighting_parameters(sum_of_user_functional_shares,
                                    sum_of_project_functional_shares,
                                    sum_of_department_functional_shares,
                                    sum_of_jobclass_functional_shares,
                                    sum_of_job_functional_shares,
                                    weight);


   /*-----------------------------------------------------------------
    * PASS 2
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGEEE Pass 2]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {

      if (!classic_sgeee_scheduling && job_ref[job_ndx].queued)
         break;

      job = job_ref[job_ndx].job;

      if (total_share_tree_tickets > 0)
         calc_job_share_tree_tickets_pass2(&job_ref[job_ndx],
                                           sum_of_adjusted_proportions,
                                           total_share_tree_tickets,
                                           sge_scheduling_run);

      if (total_functional_tickets > 0)
         calc_job_functional_tickets_pass2(&job_ref[job_ndx],
                                           sum_of_user_functional_shares,
                                           sum_of_project_functional_shares,
                                           sum_of_department_functional_shares,
                                           sum_of_jobclass_functional_shares,
                                           sum_of_job_functional_shares,
                                           total_functional_tickets,
                                           weight,
                                           share_functional_shares,
                                           classic_sgeee_scheduling ? 1 : 0);

      if (total_deadline_tickets > 0 && lGetUlong(job, JB_deadline) > 0 &&
            sum_of_deadline_tickets > total_deadline_tickets)
         calc_job_deadline_tickets_pass2(&job_ref[job_ndx],
                                         sum_of_deadline_tickets,
                                         total_deadline_tickets);

      sum_of_active_override_tickets +=
                  calc_job_override_tickets(&job_ref[job_ndx],
                  share_override_tickets,
                  classic_sgeee_scheduling ? 1 : 0);

      sum_of_active_tickets += calc_job_tickets(&job_ref[job_ndx]);

   }

   /* set scheduler configuration information to go back to GUI */

   if (sge_conf) {
      lSetUlong(sge_conf, SC_weight_tickets_deadline_active,
		MIN(sum_of_deadline_tickets, total_deadline_tickets));
      lSetUlong(sge_conf, SC_weight_tickets_override,
		sum_of_active_override_tickets);
   }

   /*-----------------------------------------------------------------
    * NEW PENDING JOB TICKET CALCULATIONS
    *-----------------------------------------------------------------*/

   if (queued_jobs && !classic_sgeee_scheduling) {
      lList *sorted_job_node_list;
      double sum_of_pending_deadline_tickets;
      policy_hierarchy_t hierarchy[4];
      int policy_ndx;

      policy_hierarchy_fill_array(hierarchy, policy_hierarchy_string);
      policy_hierarchy_print_array(hierarchy);  

      for(policy_ndx = 0; 
          policy_ndx < sizeof(hierarchy) / sizeof(policy_hierarchy_t); 
          policy_ndx++) {

         switch(hierarchy[policy_ndx].policy) {

         case SHARE_TREE_POLICY:

      /*-----------------------------------------------------------------
       * Calculate pending share tree tickets
       *
       *    The share tree tickets for pending jobs are calculated
       *    by adding each job to the share tree as leaf nodes
       *    under the associated user/project node. The jobs are
       *    then sorted based on their respective short term
       *    entitlements to ensure that the overall goals of the
       *    share tree are met.
       *
       *-----------------------------------------------------------------*/

      if (total_share_tree_tickets > 0 && root) {

         for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
            sge_ref_t *jref = &job_ref[job_ndx];
            lListElem *node = jref->node;
            if (jref->queued) {
               if (node) {
                  /* add each job to the share tree as a temporary sibling node */
                  char tmpstr[64];
                  lListElem *child;

                  job = jref->job;
                  sprintf(tmpstr, u32"."u32, lGetUlong(job, JB_job_number),
                         REF_GET_JA_TASK_NUMBER(jref));
                  child = lAddSubStr(node, STN_name, tmpstr, STN_children, STN_Type);
                  lSetUlong(child, STN_jobid, lGetUlong(job, JB_job_number));
                  lSetUlong(child, STN_taskid, REF_GET_JA_TASK_NUMBER(jref));
                  lSetUlong(child, STN_temp, 1);
                  /* save the job reference, so we can refer to it later to set job fields */
                  lSetUlong(child, STN_ref, job_ndx+1);
                  if (hierarchy[policy_ndx].dependent) {
                     /* set the sort value based on tickets of higher level policy */
                     lSetDouble(child, STN_tickets, jref->tickets);
                     lSetDouble(child, STN_sort,
                                jref->tickets + (0.01 * (double)lGetUlong(job, JB_priority)));
                  } else
                     /* set the sort value based on the priority of the job */
                     lSetDouble(child, STN_sort, (double)lGetUlong(job, JB_priority));
               }
            }
         }

         if ((sorted_job_node_list = sge_sort_pending_job_nodes(root, root, total_share_tree_tickets))) {
            lListElem *job_node;
#if 0 /* EB: normalize the number of pending tickets */
            double sum = 0.0;
#endif

            /* 
             * set share tree tickets of each pending job 
             * based on the returned sorted node list 
             */
#if 0 /* EB: normalize the number of pending tickets */
            for_each(job_node, sorted_job_node_list) {
               sum += lGetDouble(job_node, STN_shr); 
            }
#endif
            for_each(job_node, sorted_job_node_list) {
               sge_ref_t *jref = &job_ref[lGetUlong(job_node, STN_ref)-1];
#if 0 /* EB: normalize the number of pending tickets */
               REF_SET_STICKET(jref, 
                lGetDouble(job_node, STN_shr) * total_share_tree_tickets / sum);
#else
               REF_SET_STICKET(jref, 
                     lGetDouble(job_node, STN_shr) * total_share_tree_tickets);
#endif
               if (hierarchy[policy_ndx].dependent)
                  jref->tickets += REF_GET_STICKET(jref);
            }
            lFreeList(sorted_job_node_list);
         }
      }

            break;

         case FUNCTIONAL_POLICY:

      /*-----------------------------------------------------------------
       * Calculate pending functional tickets 
       *        
       *    We use a brute force method where we order the pending
       *    jobs based on which job would get the largest number of 
       *    functional tickets if it were to run next. Because of the
       *    overhead, we limit the number of pending jobs that we will
       *    order based on the configurable schedd parameter called
       *    max_functional_jobs_to_schedule. The default is 100.
       *        
       *-----------------------------------------------------------------*/

      if (total_functional_tickets > 0 && num_queued_jobs > 0) {
         int *sort_list;
         int i,j,sort_ndx=0;
         double pending_user_fshares = sum_of_user_functional_shares;
         double pending_proj_fshares = sum_of_project_functional_shares;
         double pending_dept_fshares = sum_of_department_functional_shares;
         double pending_jobclass_fshares = sum_of_jobclass_functional_shares;
         double pending_job_fshares = sum_of_job_functional_shares;

         sort_list = (int *)malloc(num_queued_jobs * sizeof(int));

         for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
            sge_ref_t *jref = &job_ref[job_ndx];
            if (jref->queued)
               sort_list[sort_ndx++] = job_ndx;
         }

         /* Loop through all the jobs calculating the functional tickets and
            find the job with the most functional tickets.  Move it to the
            top of the list¸ and start the process all over again with the
            remaining jobs. */

         for(i=0; i<MIN(num_queued_jobs,max_functional_jobs_to_schedule); i++) {
            double ftickets, max_ftickets=0;
            u_long jid, save_jid=0, save_tid=0;

            for(j=i; j<num_queued_jobs; j++) {
               sge_ref_t *jref = &job_ref[sort_list[j]];
               double user_fshares = pending_user_fshares + 0.001;
               double proj_fshares = pending_proj_fshares + 0.001;
               double dept_fshares = pending_dept_fshares + 0.001;
               double jobclass_fshares = pending_jobclass_fshares + 0.001;
               double job_fshares = pending_job_fshares + 0.001;

               /* Consider this job "active" by incrementing user and project job counts */
               sge_set_job_cnts(jref, 0);

               calc_pending_job_functional_tickets(jref,
                                                   &user_fshares,
                                                   &proj_fshares,
                                                   &dept_fshares,
                                                   &jobclass_fshares,
                                                   &job_fshares,
                                                   total_functional_tickets,
                                                   weight,
                                                   share_functional_shares,
                                                   0);

               /* Consider job inactive now */
               sge_unset_job_cnts(jref, 0);

               ftickets = REF_GET_FTICKET(jref);

               if (hierarchy[policy_ndx].dependent)
                  ftickets += jref->tickets;

               if (ftickets < max_ftickets)  /* handle common case first */
                  continue;

               if (j == i ||
                   ftickets > max_ftickets ||
                   (jid=lGetUlong(jref->job, JB_job_number)) < save_jid ||
                   (jid == save_jid && 
                    REF_GET_JA_TASK_NUMBER(jref) < save_tid)) {
                  int tmp_ndx;
                  max_ftickets = ftickets;
                  save_jid = lGetUlong(jref->job, JB_job_number);
                  save_tid = REF_GET_JA_TASK_NUMBER(jref);
                  tmp_ndx = sort_list[i];
                  sort_list[i] = sort_list[j];
                  sort_list[j] = tmp_ndx;
               }
            }

            /* This is the job with the most functional tickets, consider it active */

            sge_set_job_cnts(&job_ref[sort_list[i]], 0);

            calc_pending_job_functional_tickets(&job_ref[sort_list[i]],
                                                &pending_user_fshares,
                                                &pending_proj_fshares,
                                                &pending_dept_fshares,
                                                &pending_jobclass_fshares,
                                                &pending_job_fshares,
                                                total_functional_tickets,
                                                weight,
                                                share_functional_shares,
                                                0);
         }

         /* Reset the pending jobs to inactive */

         for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
            sge_ref_t *jref = &job_ref[job_ndx];
            if (jref->queued) {
               sge_unset_job_cnts(jref, 0);
               if (hierarchy[policy_ndx].dependent)
                  jref->tickets += REF_GET_FTICKET(jref);
            }
         }

         free(sort_list);
      }

            break;

         case OVERRIDE_POLICY:

      /*-----------------------------------------------------------------
       * Calculate the pending override tickets
       *-----------------------------------------------------------------*/

      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         if (jref->queued) {
            calc_job_override_tickets(jref, share_override_tickets, 0);
            if (hierarchy[policy_ndx].dependent)
               jref->tickets += REF_GET_OTICKET(jref);
         }
      }

            break;
            
         case DEADLINE_POLICY:

      /*-----------------------------------------------------------------
       * Calculate the pending deadline tickets
       *-----------------------------------------------------------------*/

      sum_of_pending_deadline_tickets = sum_of_deadline_tickets;

      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         if (jref->queued) {

            if (total_deadline_tickets > 0 && lGetUlong(jref->job, JB_deadline))
               sum_of_pending_deadline_tickets +=
                        calc_job_deadline_tickets_pass1(jref,
                                                        total_deadline_tickets,
                                                        curr_time);
         }
      }
      
      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         if (jref->queued) {

            if (total_deadline_tickets > 0 && lGetUlong(jref->job, JB_deadline) > 0 &&
                  sum_of_pending_deadline_tickets > total_deadline_tickets)
               calc_job_deadline_tickets_pass2(jref,
                                               sum_of_pending_deadline_tickets,
                                               total_deadline_tickets);
               if (hierarchy[policy_ndx].dependent)
                  jref->tickets += REF_GET_DTICKET(jref);
         }
      }

            break;

         default:
            break;
         }
      }

      /*-----------------------------------------------------------------
       * Combine the ticket values into the total tickets for the job
       *-----------------------------------------------------------------*/

      for(job_ndx=0; job_ndx<num_jobs; job_ndx++)
         calc_job_tickets(&job_ref[job_ndx]);


#if 0

      /* 

         At this point we have a job list with the relative importance of each job
         specified by the tickets. We need to sort the pending job list and then
         recalculate the tickets that each job would get if it were dispatched in
         the sorted order.  This provides reasonable starting ticket values for the
         job and reasonable ticket values for all pending jobs which can be displayed
         in qmon and qstat.

      */

      sort_info = (sge_ref_t **)malloc(num_queued_jobs * sizeof(sge_ref_t *));
      sort_ndx = 0;
      for(job_ndx=0; job_ndx<num_jobs; job_ndx++)
         if (job_ref[job_ndx].queued)
            sort_info[sort_ndx++] = &job_ref[job_ndx];
      qsort(sort_info, num_queued_jobs, sizeof(sge_ref_t *), sort_pending_jobs);

      /*-----------------------------------------------------------------
       * Calculate the number of tickets each job would get if it and
       * every higher priority pending job was active.  We do this by
       * going through the sorted job list and calling the appropriate
       * ticket calculation functions for each policy.
       *-----------------------------------------------------------------*/

      for(sort_ndx=0; sort_ndx<num_queued_jobs; sort_ndx++) {
         sge_ref_t *jref = sort_info[sort_ndx];

         /* Consider this job "active" by incrementing user and project job counts */
         sge_set_job_cnts(jref, 0);

         /* share tree tickets are already calculated */

         /* calculate functional tickets */

         if (total_functional_tickets > 0) {

            calc_pending_job_functional_tickets(jref,
                                                &sum_of_user_functional_shares,
                                                &sum_of_project_functional_shares,
                                                &sum_of_department_functional_shares,
                                                &sum_of_jobclass_functional_shares,
                                                &sum_of_job_functional_shares,
                                                total_functional_tickets,
                                                weight,
                                                share_functional_shares,
                                                0);
         }

         /* calculate override tickets */

         calc_job_override_tickets(jref, share_override_tickets, 0);

         /* calculate deadline tickets */

         if (total_deadline_tickets > 0 && lGetUlong(jref->job, JB_deadline)) {

            sum_of_deadline_tickets +=
                     calc_job_deadline_tickets_pass1(jref,
                                                     total_deadline_tickets,
                                                     curr_time);

            if (sum_of_deadline_tickets > total_deadline_tickets)
               calc_job_deadline_tickets_pass2(jref,
                                               sum_of_deadline_tickets,
                                               total_deadline_tickets);
         }

         /* combine policy tickets to get total number of tickets */

         calc_job_tickets(jref);
      }

      free(sort_info);

#endif

      /* calculate the ticket % of each job */

      sum_of_active_tickets = 0;
      sum_of_pending_tickets = 0;
      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         if (jref->queued)
            sum_of_pending_tickets += REF_GET_TICKET(jref);
         else
            sum_of_active_tickets += REF_GET_TICKET(jref);
      }

      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         double ticket_sum = jref->queued ? sum_of_pending_tickets : sum_of_active_tickets;

         if (REF_GET_TICKET(jref) > 0) {
            REF_SET_SHARE(jref, REF_GET_TICKET(jref) / ticket_sum);
         } else {
            REF_SET_SHARE(jref, 0);
         }
      }

   } else {
      for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
         sge_ref_t *jref = &job_ref[job_ndx];
         lListElem *ja_task;

         job = jref->job;
         ja_task = jref->ja_task;

         if (REF_GET_TICKET(jref)  > 0) {
            REF_SET_SHARE(jref, REF_GET_TICKET(jref) / sum_of_active_tickets);
         } else {
            REF_SET_SHARE(jref, 0);
         }

      }

   }

   /* update share tree node for finished jobs */

   /* NOTE: what purpose does this serve? */

   if (do_usage && finished_jobs) {
      for_each(job, finished_jobs) {
         sge_ref_t jref;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            memset(&jref, 0, sizeof(jref));
            sge_set_job_refs(job, ja_task, &jref, NULL, lists, 0);
            if (jref.node) {
               if (sge_scheduling_run != lGetUlong(jref.node, STN_pass2_seqno)) {
                  sge_zero_node_fields(jref.node, NULL);
                  lSetUlong(jref.node, STN_pass2_seqno, sge_scheduling_run);
               }
            }
         }
      }
   }

   /* 
    * EB: copy tickets 
    * 
    * Tickets for unenrolled pending tasks where stored an internal table.
    * Now it is necessary to find the ja_task of a job which got the 
    * most tickets. These ticket numbers will be stored in the template
    * element within the job. 
    */

#if 0 /* EB: debug */
   {
      lListElem *job;

      for_each(job, queued_jobs) {
         DPRINTF(("job_id: "u32"\n", lGetUlong(job, JB_job_number)));
      }
   }
#endif
   if (queued_jobs != NULL) {
      sge_task_ref_t *tref = task_ref_get_first_job_entry();

      while(tref != NULL) {
         lListElem *job = lGetElemUlong(queued_jobs, JB_job_number, 
                                        tref->job_number); 

         if (job) {
            lListElem *ja_task_template;
#if 0 /* EB: debug */
   DPRINTF(("tref->job_number: "u32"\n", tref->job_number));         
#endif

            ja_task_template = lFirst(lGetList(job, JB_ja_template));

            task_ref_copy_to_ja_task(tref, ja_task_template);
         } 
         tref = task_ref_get_next_job_entry();
      }
   }

   
   if (job_ref) {
      for(job_ndx=0; job_ndx<num_jobs; job_ndx++)
         if (job_ref[job_ndx].task_jobclass)
            free(job_ref[job_ndx].task_jobclass);
      free(job_ref);
   }

   if (decay_list)
      lFreeList(decay_list);

   DEXIT;
   return sge_scheduling_run;
}


/*--------------------------------------------------------------------
 * sge_sort_pending_job_nodes - return a sorted list of pending job
 * nodes with the pending priority set in STN_sort.
 *--------------------------------------------------------------------*/

static lList *
sge_sort_pending_job_nodes(lListElem *root,
                           lListElem *node,
                           double total_share_tree_tickets)
{
   lList *job_node_list = NULL;
   lListElem *child, *job_node;
   double node_stt;
   double job_count;
   int job_nodes = 0;

   /* get the child job nodes in a single list */

   for_each(child, lGetList(node, STN_children)) {
      if (lGetUlong(child, STN_ref)) {
         /* this is a job node, chain it onto our list */
         if (!job_node_list)
            job_node_list = lCreateList("sorted job node list", STN_Type);
         lAppendElem(job_node_list, lCopyElem(child));
         job_nodes++;
      } else if ((lGetUlong(child, STN_job_ref_count)-lGetUlong(child, STN_active_job_ref_count))>0) {
         lList *child_job_node_list;
         /* recursively get all the child job nodes onto our list */
         if ((child_job_node_list = sge_sort_pending_job_nodes(root, child, total_share_tree_tickets))) {
            if (job_node_list == NULL)
               job_node_list = child_job_node_list;
            else
               lAddList(job_node_list, child_job_node_list);
         }
      }
   }

   /* free the temporary job nodes */
   if (job_nodes)
      lSetList(node, STN_children, NULL);

   if (root != node) {
      lListElem *u;

      /* sort the job nodes based on the calculated pending priority */

      if (job_node_list && lGetNumberOfElem(job_node_list)>1)
         lPSortList(job_node_list, "%I- %I+ %I+", STN_sort, STN_jobid, STN_taskid);

      /* calculate a new pending priority -
         The priority of each pending job associated with this node is the
         node's short term entitlement (STN_stt) divided by the number of jobs
         which are scheduled ahead of this node times the number of share tree
         tickets. If we are dependent on another higher-level policy, we also add
         the tickets from those policies. */

      job_count = lGetUlong(node, STN_active_job_ref_count);
      if ((u=lGetElemStr(lGetList(node, STN_usage_list), UA_name,
                         "finished_jobs")))
         job_count += lGetDouble(u, UA_value);
      node_stt = lGetDouble(node, STN_stt);
      for_each(job_node, job_node_list) {
         job_count++;
         lSetDouble(job_node, STN_shr, node_stt / job_count);
         lSetDouble(job_node, STN_sort, lGetDouble(job_node, STN_tickets) +
                    ((node_stt / job_count) * total_share_tree_tickets));
      }

   }

   /* return the new list */

   return job_node_list;
}


/*--------------------------------------------------------------------
 * sge_calc_sharetree_targets - calculate the targeted 
 * proportional share for each node in the sharetree.
 *--------------------------------------------------------------------*/

static int
sge_calc_sharetree_targets( lListElem *root,
                            sge_Sdescr_t *lists,
                            lList *decay_list,
                            u_long curr_time,
                            u_long seqno )
{

   DENTER(TOP_LAYER, "sge_calc_sharetree_targets");

   /* decay and store user and project usage into sharetree nodes */

   sge_calc_node_usage(root,
                       lists->user_list,
                       lists->project_list,
                       lists->config_list,
                       decay_list,
                       curr_time,
                       NULL,
                       seqno);

   /* Calculate targeted proportions for each node */

   sge_calc_node_targets(root, root, lists);

   DEXIT;
   return 0;
}


/*--------------------------------------------------------------------
 * sge_calc_node_targets - calculate the targeted proportions
 * for the sub-tree rooted at this node
 *--------------------------------------------------------------------*/

static int
sge_calc_node_targets( lListElem *root,
                       lListElem *node,
                       sge_Sdescr_t *lists )
{
   lList *children;
   lListElem *child;
   double sum_shares, sum, compensation_factor;
   /* char *node_name = lGetString(node, STN_name); */

   DENTER(TOP_LAYER, "sge_calc_node_targets");

   /*---------------------------------------------------------------
    * if this is the root node, initialize proportions
    *    ltt - long term targeted % among siblings
    *    oltt - overall long term targeted % among entire share tree
    *    stt - short term targeted % among siblings
    *    ostt - overall short term targeted % among entire share tree
    *---------------------------------------------------------------*/

   if (node == root) {
      lSetDouble(node, STN_stt, 1.0);
      lSetDouble(node, STN_ltt, 1.0);
      lSetDouble(node, STN_ostt, 1.0);
      lSetDouble(node, STN_oltt, 1.0);
   }

   /*---------------------------------------------------------------
    * if this node has no children, there's nothing to do
    *---------------------------------------------------------------*/

   if ((!(children = lGetList(node, STN_children))) ||
       lGetNumberOfElem(children) == 0) {
      DEXIT;
      return 0;
   }

/*
   do for each active sub-node
      sum_shares += subnode.shares
      subnode.shr = subnode.shares * subnode.shares / subnode.usage
      sum += subnode.shr 
   end do
*/

   /*---------------------------------------------------------------
    * calculate the shr value of each child node
    * calculate the short term targeted proportions 
    *---------------------------------------------------------------*/

   sum_shares = 0;
   for_each(child, children) {
      if (lGetUlong(child, STN_job_ref_count)>0) {
         sum_shares += lGetUlong(child, STN_shares);
      }
   }

   sum = 0;
   for_each(child, children) {
      if (lGetUlong(child, STN_job_ref_count)>0) {
         double shares, shr, ltt, oltt;
         shares = lGetUlong(child, STN_shares);
         if (sum_shares > 0) {
            ltt = shares / (double)sum_shares;
         } else {
            ltt = 0;
         }
         oltt = lGetDouble(node, STN_oltt) * ltt;
         lSetDouble(child, STN_ltt, ltt);
         lSetDouble(child, STN_oltt, oltt);
         if (oltt > 0) {
            shr = shares * shares / 
                MAX(lGetDouble(child, STN_combined_usage), SGE_MIN_USAGE*oltt);
         } else {
            shr = 0;
         } 
         lSetDouble(child, STN_shr, shr);
         sum += shr;
      }
   }

/*
   do for each active sub-node
      subnode.ltt = subnode.shares / sum_shares;
      subnode.oltt = node.oltt * subnode.ltt;
      subnode.stt = subnode.shr / sum;
      subnode.ostt = node.stt * subnode.stt;
   end do
*/

   /*---------------------------------------------------------------
    * calculate the short term targeted proportions 
    *---------------------------------------------------------------*/

   for_each(child, children) {
      if (lGetUlong(child, STN_job_ref_count)>0) {
         double stt;

         if (sum > 0) {
            stt = lGetDouble(child, STN_shr) / sum;
         } else {
            stt = 0;
         }
         lSetDouble(child, STN_stt, stt);
         lSetDouble(child, STN_ostt, lGetDouble(node, STN_ostt) * stt);
         lSetDouble(child, STN_adjusted_current_proportion, lGetDouble(child, STN_ostt));
      }
   }

/*
   if compensation_factor != 0

      sum = 0
      do for each active sub-node
         if subnode.ostt > (compensation_factor * subnode.oltt)
            subnode.shr = subnode.shares * subnode.shares / 
                  (subnode.usage * (subnode.ostt / (compensation_factor * subnode.oltt)))
            sum += subnode.shr
         endif
      enddo

      do for each active sub-node
         subnode.stt = subnode.shr / sum;
         subnode.ostt = node.stt * subnode.stt;
      end do

   endif
*/

   /*---------------------------------------------------------------
    * adjust targeted proportion based on compensation factor
    *---------------------------------------------------------------*/

   compensation_factor = lGetDouble(lFirst(lists->config_list),
                  SC_compensation_factor);

   if (compensation_factor != 0) {
      u_long compensation = 0;

      /*---------------------------------------------------------------
       * recalculate the shr value of each child node based on compensation factor
       *---------------------------------------------------------------*/

      sum = 0;
      for_each(child, children) {
         if (lGetUlong(child, STN_job_ref_count)>0) {
            double ostt = lGetDouble(child, STN_ostt);
            double oltt = lGetDouble(child, STN_oltt);

            if (ostt > (compensation_factor * oltt)) {
               double shares = lGetUlong(child, STN_shares);
               double shr = shares * shares /
                     (MAX(lGetDouble(child, STN_combined_usage), SGE_MIN_USAGE*oltt) *
                     (ostt / (compensation_factor * oltt)));
               lSetDouble(child, STN_shr, shr);
               compensation = 1;
            }
            sum += lGetDouble(child, STN_shr);
         }
      }

      /*---------------------------------------------------------------
       * reset short term targeted proportions (if necessary)
       *---------------------------------------------------------------*/

      if (compensation) {
         for_each(child, children) {
            if (lGetUlong(child, STN_job_ref_count)>0) {
               double stt = lGetDouble(child, STN_shr) / sum;
               lSetDouble(child, STN_stt, stt);
               lSetDouble(child, STN_ostt, lGetDouble(node, STN_ostt) * stt);
               lSetDouble(child, STN_adjusted_current_proportion, lGetDouble(child, STN_ostt));
            }
         }
      }

   }

/*
   do for each active sub-node
      call sge_calc_node_targets(subnode, lists);
   end do
*/

   /*---------------------------------------------------------------
    * recursively call self for all active child nodes
    *---------------------------------------------------------------*/

   for_each(child, children) {
      if (lGetUlong(child, STN_job_ref_count)>0) {
         sge_calc_node_targets(root, child, lists);
      }
   }

   DEXIT;
   return 0;
}


/*--------------------------------------------------------------------
 * sge_dump_list - dump list to stdout (for calling while in debugger)
 *--------------------------------------------------------------------*/

void
sge_dump_list( lList *list )
{
   FILE *f;

   if (!(f=fdopen(1, "w"))) {
      fprintf(stderr, MSG_FILE_OPENSTDOUTASFILEFAILED );
      return;
   }

   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_SGE_UNABLETODUMPJOBLIST );
   }
}


/*--------------------------------------------------------------------
 * sge_dump_list - dump list to file (for calling while in debugger)
 *--------------------------------------------------------------------*/

void
sge_dump_list_to_file(const char *file, lList *list)
{
   FILE *f;

   if (!(f=fopen(file, "w+"))) {
      fprintf(stderr, MSG_FILE_OPENSTDOUTASFILEFAILED );
      return;
   }

   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_SGE_UNABLETODUMPJOBLIST );
   }

   fclose(f);
}


/*--------------------------------------------------------------------
 * get_mod_share_tree - return reduced modified share tree
 *--------------------------------------------------------------------*/


static lListElem *
get_mod_share_tree( lListElem *node,
                    lEnumeration *what,
                    int seqno )
{
   lListElem *new_node=NULL;
   lList *children;
   lListElem *child;

   if (((children = lGetList(node, STN_children))) &&
       lGetNumberOfElem(children) > 0) {

      lList *child_list=NULL;

      for_each(child, children) {
         lListElem *child_node;
         child_node = get_mod_share_tree(child, what, seqno);
         if (child_node) {
            if (!child_list)
               child_list = lCreateList("", STN_Type);
            lAppendElem(child_list, child_node);
         }
      }

      if (child_list) {
#ifdef lmodifywhat_shortcut
         new_node = lCreateElem(STN_Type);
         lModifyWhat(new_node, node, what);
#else
         new_node = lCopyElem(node);
#endif
         lSetList(new_node, STN_children, child_list);
      }
      
   } else {

      if (lGetUlong(node, STN_pass2_seqno) > (u_long32)seqno &&
          lGetUlong(node, STN_temp) == 0) {
#ifdef lmodifywhat_shortcut
         new_node = lCreateElem(STN_Type);
         lModifyWhat(new_node, node, what);
#else
         new_node = lCopyElem(node);
#endif
      }

   }

   return new_node;
}


/*--------------------------------------------------------------------
 * sge_build_sge_orders - build orders for updating qmaster
 *--------------------------------------------------------------------*/

lList *
sge_build_sge_orders( sge_Sdescr_t *lists,
                      lList *running_jobs,
                      lList *queued_jobs,
                      lList *finished_jobs,
                      lList *order_list,
                      int update_usage_and_configuration,
                      int seqno )
{
   lCondition *where=NULL;
   lEnumeration *what=NULL;
   lList *up_list;
   lList *config_list;
   lListElem *order;
   lListElem *root;
   lListElem *job;
   int norders; 
   static int last_seqno = 0;

   DENTER(TOP_LAYER, "sge_build_sge_orders");

   if (!order_list)
      order_list = lCreateList("orderlist", OR_Type);

   /*-----------------------------------------------------------------
    * build ticket orders for running jobs
    *-----------------------------------------------------------------*/

   if (running_jobs) {
      norders = lGetNumberOfElem(order_list);

      DPRINTF(("   got %d running jobs\n", lGetNumberOfElem(running_jobs)));

      for_each(job, running_jobs) {
         const char *pe_str;
         lList *granted;
         lListElem *pe;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if ((pe_str=lGetString(ja_task, JAT_granted_pe))
                  && (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str))
                  && lGetUlong(pe, PE_control_slaves))

               granted=lGetList(ja_task, JAT_granted_destin_identifier_list);
            else
               granted=NULL;

            order_list = sge_create_orders(order_list, ORT_tickets, job, ja_task, granted);
         }
      }
      DPRINTF(("   added %d ticket orders for running jobs\n", 
         lGetNumberOfElem(order_list) - norders));
   }

   if (queued_jobs) {
      lListElem *qep;
      u_long32 free_qslots = 0;
      norders = lGetNumberOfElem(order_list);
      for_each(qep, lists->queue_list)
         free_qslots += MAX(0, lGetUlong(qep, QU_job_slots) - qslots_used(qep));
      for_each(job, queued_jobs) {
         lListElem *ja_task;
         int tasks=0;
#if 0
         /* NOTE: only send pending tickets for the first sub-task */
         /* I commented this out because when the first sub-task
            gets scheduled, then the other sub-tasks didn't have
            any tickets specified */
         order_list = sge_create_orders(order_list, ORT_ptickets, job,
               lFirst(lGetList(job, JB_ja_tasks)), NULL);
#endif
         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if (++tasks > MIN(max_pending_tasks_per_job, free_qslots+1))
               break;
            order_list = sge_create_orders(order_list, ORT_ptickets, job, ja_task, NULL);
         }
         if (job_get_not_enrolled_ja_tasks(job) > 0) {
            lListElem *task_template = NULL;
 
            task_template = lFirst(lGetList(job, JB_ja_template));
            if (task_template) {
               order_list = sge_create_orders(order_list, ORT_ptickets, job,
                                              task_template, NULL);
            }
         }  
      }
      DPRINTF(("   added %d ticket orders for queued jobs\n", 
         lGetNumberOfElem(order_list) - norders));
   }

   /*-----------------------------------------------------------------
    * build delete job orders for finished jobs
    *-----------------------------------------------------------------*/
   if (finished_jobs) {
      order_list  = create_delete_job_orders(finished_jobs, order_list);
   }

   if (update_usage_and_configuration) {

      /*-----------------------------------------------------------------
       * build update user usage order
       *-----------------------------------------------------------------*/

      what = lWhat("%T(%I %I %I %I %I %I %I)", UP_Type,
                   UP_name, UP_usage, UP_usage_time_stamp,
                   UP_long_term_usage, UP_project, UP_debited_job_usage,
                   UP_version);

      /* NOTE: make sure we get all usage entries which have been decayed
         or have accumulated additional usage */

      where = lWhere("%T(%I > %u)", UP_Type, UP_usage_seqno, last_seqno);

      if (lists->user_list) {
         norders = lGetNumberOfElem(order_list); 
         if ((up_list = lSelect("", lists->user_list, where, what))) {
            if (lGetNumberOfElem(up_list)>0) {
               order = lCreateElem(OR_Type);
               lSetUlong(order, OR_seq_no, get_seq_nr());
               lSetUlong(order, OR_type, ORT_update_user_usage);
               lSetList(order, OR_joker, up_list);
               lAppendElem(order_list, order);
            } else
               lFreeList(up_list);
         }
         DPRINTF(("   added %d orders for updating usage of user\n",
            lGetNumberOfElem(order_list) - norders));      
      }

      /*-----------------------------------------------------------------
       * build update project usage order
       *-----------------------------------------------------------------*/

      if (lists->project_list) {
         norders = lGetNumberOfElem(order_list); 
         if ((up_list = lSelect("", lists->project_list, where, what))) {
            if (lGetNumberOfElem(up_list)>0) {
               order = lCreateElem(OR_Type);
               lSetUlong(order, OR_seq_no, get_seq_nr());
               lSetUlong(order, OR_type, ORT_update_project_usage);
               lSetList(order, OR_joker, up_list);
               lAppendElem(order_list, order);
            } else
               lFreeList(up_list);
         }
         DPRINTF(("   added %d orders for updating usage of project\n",
            lGetNumberOfElem(order_list) - norders));
      }

      lFreeWhat(what);
      lFreeWhere(where);

      /*-----------------------------------------------------------------
       * build update share tree order
       *-----------------------------------------------------------------*/

      if (lists->share_tree && ((root = lFirst(lists->share_tree)))) {
         lListElem *node;
         lEnumeration *so_what;
         norders = lGetNumberOfElem(order_list);

         so_what = lWhat("%T(%I %I %I %I %I %I)", STN_Type,
                         STN_version, STN_name, STN_job_ref_count, STN_m_share,
                         STN_last_actual_proportion,
                         STN_adjusted_current_proportion);

         if ((node = get_mod_share_tree(root, so_what, last_seqno))) {
            up_list = lCreateList("", STN_Type);
            lAppendElem(up_list, node);
            order = lCreateElem(OR_Type);
            lSetUlong(order, OR_seq_no, get_seq_nr());
            lSetUlong(order, OR_type, ORT_share_tree);
            lSetList(order, OR_joker, up_list);
            lAppendElem(order_list, order);
         }
         DPRINTF(("   added %d orders for updating share tree\n",
            lGetNumberOfElem(order_list) - norders)); 

         lFreeWhat(so_what);
      } 

      /*-----------------------------------------------------------------
       * build update scheduler configuration order
       *-----------------------------------------------------------------*/

      what = lWhat("%T(%I %I)", SC_Type,
                   SC_weight_tickets_deadline_active,
                   SC_weight_tickets_override);

      if (lists->config_list) {
         norders = lGetNumberOfElem(order_list); 
         if ((config_list = lSelect("", lists->config_list, NULL, what))) {
            if (lGetNumberOfElem(config_list)>0) {
               order = lCreateElem(OR_Type);
               lSetUlong(order, OR_seq_no, get_seq_nr());
               lSetUlong(order, OR_type, ORT_sched_conf);
               lSetList(order, OR_joker, config_list);
               lAppendElem(order_list, order);
            } else
               lFreeList(config_list);
         }
         DPRINTF(("   added %d orders for scheduler configuration\n",
            lGetNumberOfElem(order_list) - norders));   
      }

      lFreeWhat(what);

      last_seqno = seqno;

   }

   return order_list;
}

/*--------------------------------------------------------------------
 * sge_scheduler
 *--------------------------------------------------------------------*/

int
sge_scheduler( sge_Sdescr_t *lists,
               lList *running_jobs,
               lList *finished_jobs,
               lList *pending_jobs,
               lList **orderlist )
{
   static u_long32 next = 0;
   u_long32 now;
   u_long seqno;
   lListElem *job;

   DENTER(TOP_LAYER, "sge_scheduler");

#if 0
   sge_setup_lists(lists, NULL, running_jobs); /* resetup each time */
#endif

   /* clear SGEEE fields for queued jobs */

   if (pending_jobs)
      for_each(job, pending_jobs)
         sge_clear_job(job);

   /*

      On a "normal" scheduling interval:

	 calculate tickets for new and running jobs

	 don't decay and sum usage

	 don't update qmaster

      On a SGEEE scheduling interval:

	 calculate tickets for new and running jobs

	 decay and sum usage

	 handle finished jobs

	 update qmaster

   */

   if ((now = sge_get_gmt())<next) {

      if (classic_sgeee_scheduling) {
         seqno = sge_calc_tickets(lists, running_jobs, NULL, NULL, 0);
      } else {

         /* calculate tickets for pending jobs */
         seqno = sge_calc_tickets(lists, running_jobs, finished_jobs, 
                                  pending_jobs, 1);

         /* Only update qmaster with tickets of pending jobs - everything
            else will be updated on the SGEEE scheduling interval. */
         *orderlist = sge_build_sge_orders(lists, NULL, pending_jobs, NULL,
                                           *orderlist, 0, seqno);
      }

   } else {

      DPRINTF(("=-=-=-=-=-=-=-=-=-=-=  SGEEE ORDER   =-=-=-=-=-=-=-=-=-=-=\n"));
      if (classic_sgeee_scheduling) {
         seqno = sge_calc_tickets(lists, running_jobs, finished_jobs, NULL, 1);
         *orderlist = sge_build_sge_orders(lists, running_jobs, NULL, 
                                           finished_jobs,
                                           *orderlist, 1, seqno);
      } else {

         /* calculate tickets for pending jobs */
         seqno = sge_calc_tickets(lists, running_jobs, finished_jobs, 
                                  pending_jobs, 1);
         
         /* calculate tickets for running jobs */
         seqno = sge_calc_tickets(lists, running_jobs, NULL, NULL, 0);

         /* update qmaster with job tickets, usage, and finished jobs */
         *orderlist = sge_build_sge_orders(lists, running_jobs, pending_jobs,
                                           finished_jobs, *orderlist, 1, seqno);
      }

      next = now + scheddconf.sgeee_schedule_interval;
   }

   DEXIT;
   return 0;
}


/* ----------------------------------------

   calculate_host_tickets()

   calculates the total number of tickets on a host from the
   JAT_ticket field for jobs associated with the host


   returns:
      0 successful
     -1 errors in functions called by calculate_host_tickets

*/
int
calculate_host_tickets( lList **running,   /* JB_Type */
                        lList **hosts )    /* EH_Type */
{
   const char *host_name;
   double host_sge_tickets;
   lListElem *hep, *running_job_elem, *rjq;
   lList *help_list;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "calculate_host_tickets");

   if (!hosts) {
      DEXIT;
      return -1;
   }

  if (!running) {
      for_each (hep, *hosts) {
         lSetDouble(hep, EH_sge_tickets, 0);
      }
      DEXIT;
      return 0;
   }

   for_each (hep, *hosts) { 
      host_name = lGetHost(hep, EH_name);
      lSetDouble(hep, EH_sge_tickets, 0);
      host_sge_tickets = 0;
      for_each (running_job_elem, *running) { 
         lListElem* ja_task;

         for_each(ja_task, lGetList(running_job_elem, JB_ja_tasks)) {  
            help_list = lGetList(ja_task, JAT_granted_destin_identifier_list);
            rjq = lGetElemHostFirst(help_list, JG_qhostname, host_name, &iterator );
            while (rjq != NULL) {
               host_sge_tickets += lGetDouble(ja_task, JAT_ticket);
               rjq = lGetElemHostNext(help_list, JG_qhostname, host_name, &iterator);
            }
         }
      }
      lSetDouble(hep, EH_sge_tickets, host_sge_tickets);
   }

   DEXIT;
   return 0;
}



/*************************************************************************

   sort_host_list_by_share_load

   purpose:
      sort host list according to a share load evaluation formula.

   return values:
      0 on success; -1 otherwise

   input parameters:
      hl             :  the host list to be sorted
      cplx_list      :  the complex list
      formula        :  the share load evaluation formula (containing no blanks)

   output parameters:
      hl             :  the sorted host list

*************************************************************************/
int
sort_host_list_by_share_load( lList *hl,
                              lList *cplx_list )
{
   lListElem *hlp                = NULL;
   lListElem *global_host_elem   = NULL; 
   lListElem *template_host_elem = NULL;

   double total_SGE_tickets = 0;
   double total_resource_capability_factor = 0.0;
   double resource_allocation_factor = 0.0;
   double s_load = 0.0;
   u_long total_load = 0;
#ifdef notdef
   lListElem *host_complex;
   lList *host_complex_attributes = NULL;
#endif

   DENTER(TOP_LAYER, "sort_host_list_by_share_load");

#ifdef notdef
   /*
      don't panic if there is no host_complex
       a given attributename does not exist -
      error handling is done in scale_load_value()
   */
   if ((host_complex = lGetElemStr(cplx_list, CX_name, SGE_HOST_NAME)))
      host_complex_attributes = lGetList(host_complex, CX_entries);

#endif

   /* collect  share load parameter totals  for each host*/
   global_host_elem   = lGetElemHost(hl, EH_name, SGE_GLOBAL_NAME);    /* get "global" element pointer */
   template_host_elem = lGetElemHost(hl, EH_name, SGE_TEMPLATE_NAME);  /* get "template" element pointer */
   for_each (hlp, hl) {
      lDouble hlp_EH_sort_value;
      /* EH_sort_value (i.e., load) should not be less than 1 */
      
      if (hlp == template_host_elem)
         continue;                      /* don't treat template at all */

      hlp_EH_sort_value = lGetDouble(hlp, EH_sort_value); 
 
      if ( ( hlp != global_host_elem ) && ( hlp_EH_sort_value < 1 ) ) {  /* don't treat global */
         lSetDouble(hlp, EH_sort_value, 1);
         hlp_EH_sort_value = (lDouble) 1;
      } 

      total_SGE_tickets += lGetDouble(hlp, EH_sge_tickets);

      /* don't treat global and ERROR_LOAD_VAL */
      if ( (hlp != global_host_elem) && (hlp_EH_sort_value != ERROR_LOAD_VAL) ) { 
         total_resource_capability_factor +=  lGetDouble(hlp, EH_resource_capability_factor);
      } 

      if ( hlp_EH_sort_value != ERROR_LOAD_VAL ) {
         total_load += hlp_EH_sort_value;
      } 
   }

   if ( total_resource_capability_factor == 0.0)  {
      global_host_elem   = lGetElemHost(hl, EH_name, SGE_GLOBAL_NAME);    /* get "global" element pointer */
      template_host_elem = lGetElemHost(hl, EH_name, SGE_TEMPLATE_NAME);  /* get "template" element pointer */
      for_each(hlp, hl)  { 
         if ( (hlp != global_host_elem) && (hlp != template_host_elem) &&  /* don't treat global and template */
               !(lGetDouble(hlp, EH_sort_value) == ERROR_LOAD_VAL)) { 
            lSetDouble(hlp, EH_resource_capability_factor, 1.0);
            total_resource_capability_factor += 1.0;
         } /* for_each(hlp, hl) */
      }  /* don't treat global */
   }  /* total_resource_capability_factor == 0.0 */ 
 

   /* Calculate share load parameter percentages for each host*/
   global_host_elem   = lGetElemHost(hl, EH_name, SGE_GLOBAL_NAME);    /* get "global" element pointer */
   template_host_elem = lGetElemHost(hl, EH_name, SGE_TEMPLATE_NAME);  /* get "template" element pointer */
   for_each (hlp, hl) {  

      if (hlp == template_host_elem)
         continue;

      if ( (hlp != global_host_elem) ) { /* don't treat global */
         lDouble hlp_EH_sort_value;

         hlp_EH_sort_value = lGetDouble(hlp, EH_sort_value);

         if (!(total_SGE_tickets == 0)){
             lSetDouble(hlp, EH_sge_ticket_pct, (((double) lGetDouble(hlp, EH_sge_tickets))/((double) total_SGE_tickets))*100.0);
         }
         else {
            lSetDouble(hlp, EH_sge_ticket_pct, 0.0);
         }
         lSetDouble(hlp, EH_resource_capability_factor_pct, (lGetDouble(hlp,EH_resource_capability_factor)/total_resource_capability_factor)*100);

         if ( hlp_EH_sort_value != ERROR_LOAD_VAL ){
            if (!total_load == 0) {
               lSetDouble(hlp, EH_sge_load_pct, (hlp_EH_sort_value/((double) total_load))*100.0);
            }
            else {
            lSetDouble(hlp, EH_sge_load_pct, 1.0);
            }
         } else {
            lSetDouble(hlp, EH_sge_load_pct, (double) ERROR_LOAD_VAL);
         }
      } /* don't treat global */
   } /* for_each(hlp, hl) */

   /* Calculate share load quantities for each job */
   global_host_elem   = lGetElemHost(hl, EH_name, SGE_GLOBAL_NAME);    /* get "global" element pointer */
   template_host_elem = lGetElemHost(hl, EH_name, SGE_TEMPLATE_NAME);  /* get "template" element pointer */

   for_each (hlp, hl) {  
      if (hlp == template_host_elem)
         continue;

      if ( hlp != global_host_elem ) { /* don't treat global */
         if (!(lGetDouble(hlp, EH_sort_value)==ERROR_LOAD_VAL)){

            resource_allocation_factor = (( lGetDouble(hlp, EH_resource_capability_factor_pct)) - ( lGetDouble(hlp,EH_sge_ticket_pct)));

#ifdef notdef
/*  REMOVE AFTER TESTING */
            lSetUlong(hlp, EH_sge_load, (u_long) (((( lGetDouble(hlp, EH_resource_capability_factor_pct)) - ( lGetDouble(hlp,EH_sge_ticket_pct)))/( lGetDouble(hlp,EH_sge_load_pct)))*100.0 + 1000.0));
#endif /* notdef */

            if (resource_allocation_factor < 0) {
               s_load = (((resource_allocation_factor)/(101.0 -  lGetDouble(hlp,EH_sge_load_pct)))*100.0);
               if (s_load < -100000.0) {
                  lSetUlong(hlp, EH_sge_load, 1);
               } else {
                  lSetUlong(hlp, EH_sge_load, (u_long) (((resource_allocation_factor)/(101.0 - lGetDouble(hlp,EH_sge_load_pct)))*100.0 + 100000.0));
               }
            } else {
               lSetUlong(hlp, EH_sge_load, (u_long) (((resource_allocation_factor)/( lGetDouble(hlp,EH_sge_load_pct)))*100.0 + 100000.0));
            } /* if resource_allocation_factor */
         } else {
            lSetUlong(hlp, EH_sge_load, 0);
         } /* ERROR_LOAD ? */
#ifdef notdef
         lSetDouble(hlp, EH_sort_value,
         load = scaled_mixed_load(lGetList(hlp, EH_load_list),
         lGetList(hlp, EH_scaling_list),
         host_complex_attributes,
         (double)lGetUlong(hlp, EH_load_correction_factor)/100));
#endif /* notdef */
      } else {
         lSetUlong(hlp, EH_sge_load, 0);
      } /* don't treat global */
   }

  /* sort host list in descending order according to sge_load */

   if (lPSortList(hl,"%I- %I+", EH_sge_load, EH_sort_value)) {
      DEXIT;
      return -1;
   } else {
      DEXIT;
      return 0;
   }
}

void
print_job_ref_array( sge_ref_t* ref_list,
                     int max_elem )
{
   int i;

   for(i=0; i<max_elem; i++) {
      fprintf(stderr, "###"
         " Job: "u32
         " Task: "u32 
         " t: %f"
#if 0
         " JobClasses: %d"
         " TreeType: "u32
         " JobClasse-FTicket: %f"
         " JobClasse-OTicket: %f"
#endif
         "\n",
         lGetUlong(ref_list[i].job, JB_job_number),
         REF_GET_JA_TASK_NUMBER(&ref_list[i]),
         REF_GET_TICKET(&ref_list[i])
#if 0
         ref_list[i].num_task_jobclasses,
         ref_list[i].share_tree_type,
         ref_list[i].total_jobclass_ftickets,
         ref_list[i].total_jobclass_otickets
#endif
         );

   }
}

#ifdef MODULE_TEST

int
main(int argc, char **argv)
{
   int job_number;
   sge_Sdescr_t *lists;
   lList *child_list = NULL;
   lList *group_list = NULL;
   lListElem *job, *config, *node, *usage;

   lListElem *ep;

   lists = (sge_Sdescr_t *)malloc(sizeof(sge_Sdescr_t));
   memset(lists, 0, sizeof(sge_Sdescr_t));


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   lInit(nmv);

   /* build host list */
   lAddElemHost(&(lists->host_list), EH_name, "racerx", EH_Type); 
   lAddElemHost(&(lists->host_list), EH_name, "yosemite_sam", EH_Type); 
   lAddElemHost(&(lists->host_list), EH_name, "fritz", EH_Type); 

   /* build queue list */
   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "racerx.q", QU_Type); 
   lSetHost(ep, QU_qhostname, "racerx");

   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "yosemite_sam.q", QU_Type); 
   lSetHost(ep, QU_qhostname, "yosemite_sam");

   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "fritz.q", QU_Type); 
   lSetHost(ep, QU_qhostname, "fritz");
   
   /* build configuration */

   config = lCreateElem(SC_Type);
   lSetUlong(config, SC_halftime, 60*60);
   lSetList(config, SC_usage_weight_list,
       build_usage_list("usageweightlist", NULL));
   for_each(usage, lGetList(config, SC_usage_weight_list))
      lSetDouble(usage, UA_value, 1.0/3.0);
   lSetDouble(config, SC_compensation_factor, 2);
   lSetDouble(config, SC_weight_user, 0.25);
   lSetDouble(config, SC_weight_project, 0.25);
   lSetDouble(config, SC_weight_jobclass, 0.25);
   lSetDouble(config, SC_weight_department, 0.25);
   lSetUlong(config, SC_weight_tickets_functional, 10000);
   lSetUlong(config, SC_weight_tickets_share, 10000);
   lSetUlong(config, SC_weight_tickets_deadline, 10000);
   lists->config_list = lCreateList("config_list", SC_Type);
   lAppendElem(lists->config_list, config);

   /* build user list */

   ep = lAddElemStr(&(lists->user_list), UP_name, "davidson", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 200);

   ep = lAddElemStr(&(lists->user_list), UP_name, "garrenp", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);

   ep = lAddElemStr(&(lists->user_list), UP_name, "stair", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);

   /* build project list */
    
   ep = lAddElemStr(&(lists->project_list), UP_name, "sgeee", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 200);

   ep = lAddElemStr(&(lists->project_list), UP_name, "ms", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);


   /* build department list */

   ep = lAddElemStr(&(lists->dept_list), US_name, "software", US_Type); 
   lSetUlong(ep, US_oticket, 0);
   lSetUlong(ep, US_fshare, 200);

   ep = lAddElemStr(&(lists->dept_list), US_name, "hardware", US_Type); 
   lSetUlong(ep, US_oticket, 0);
   lSetUlong(ep, US_fshare, 100);

   /* build share tree list */

   child_list = lCreateList("childlist", STN_Type);

   group_list = lCreateList("grouplist", STN_Type);

   node = lAddElemStr(&child_list, STN_name, "davidson", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&child_list, STN_name, "garrenp", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&group_list, STN_name, "groupA", STN_Type);
   lSetUlong(node, STN_shares, 1000);
   lSetList(node, STN_children, child_list);


   child_list = lCreateList("childlist", STN_Type);

   node = lAddElemStr(&child_list, STN_name, "stair", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&group_list, STN_name, "groupB", STN_Type);
   lSetUlong(node, STN_shares, 100);
   lSetList(node, STN_children, child_list);

   node = lAddElemStr(&(lists->share_tree), STN_name, "root", STN_Type);
   lSetUlong(node, STN_type, STT_USER);
   lSetList(node, STN_children, group_list);

   /* build job list */

   job_number = 1;


   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "davidson");
   lSetString(job, JB_project, "sgeee");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetHost(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 1000);
   lSetString(job, JB_owner, "davidson");
   lSetString(job, JB_project, "ms");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetHost(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_job_number, job_number++);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "stair");
   lSetString(job, JB_project, "sgeee");
   lSetString(job, JB_department, "hardware");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetHost(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "garrenp");
   lSetString(job, JB_project, "sgeee");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, sge_get_gmt() + 60*2);
   lSetHost(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());


   /* call the SGEEE scheduler */

   sge_setup_lists(lists, lists->job_list, NULL);

   sge_calc_tickets(lists, lists->job_list, NULL, NULL);

   sge_calc_tickets(lists, lists->job_list, NULL, NULL);

   lWriteListTo(lists->job_list, stdout);

   sge_calc_share_tree_proportions( lists->share_tree,
                                    lists->user_list,
                                    lists->project_list,
                                    lists->config_list);

   lWriteListTo(lists->share_tree, stdout);

   return 0;
}

#endif


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


