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

/* 
 * This module contains the default scheduler.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <limits.h>
#include <math.h>
#include <fnmatch.h>
#include <unistd.h>

#include "sge_profiling.h"
#include "sge_conf.h"
#include "sge_string.h"
#include "sge.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_schedd.h"
#include "sge_orders.h"
#include "scheduler.h"
#include "sgermon.h"
#include "sge_all_listsL.h"
#include "sge_time.h"
#include "sge_parse_num_par.h"
#include "load_correction.h"
#include "schedd_monitor.h"
#include "suspend_thresholds.h"
#include "sge_sched.h"
#include "sge_feature.h"
#include "sgeee.h"
#include "sge_support.h"
#include "interactive_sched.h"
#include "shutdown.h"
#include "schedd_message.h"
#include "sge_process_events.h"
#include "sge_access_tree.h"
#include "sge_category.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "job_log.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_host.h"
#include "sge_schedd_conf.h"
#include "sge_qinstance_state.h"

/* profiling info */
extern int scheduled_fast_jobs;
extern int scheduled_complex_jobs;
extern int do_profiling;

/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, 
                         lList **splitted_job_list[]);

static int select_assign_debit(lList **queue_list, lList **job_list, lListElem *job, lListElem *ja_task, lListElem *pe, lListElem *ckpt, lList *centry_list, lList *host_list, lList *acl_list, lList **user_list, lList **group_list, lList **orders_list, double *total_running_job_tickets, int ndispatched, int *dispatch_type, int host_order_changed, int *sort_hostlist);


/****** schedd/scheduler/scheduler() ******************************************
*  NAME
*     scheduler() -- Default scheduler
*
*  SYNOPSIS
*     int scheduler(sge_Sdescr_t *lists) 
*
*  FUNCTION
*     This function get a copy of the schedd internal data, dispatches jobs, 
*
*  INPUTS
*     sge_Sdescr_t *lists - all lists
*
*  RESULT
*     0 success  
*    -1 error  
*******************************************************************************/
#ifdef SCHEDULER_SAMPLES
int my_scheduler(sge_Sdescr_t *lists)
{
   return scheduler(lists);
}
#endif
int scheduler(sge_Sdescr_t *lists) {
   lList *orderlist=NULL;
   lList **splitted_job_lists[SPLIT_LAST];         /* JB_Type */
   lList *waiting_due_to_pedecessor_list = NULL;   /* JB_Type */
   lList *waiting_due_to_time_list = NULL;         /* JB_Type */
   lList *pending_excluded_list = NULL;            /* JB_Type */
   lList *suspended_list = NULL;                   /* JB_Type */
   lList *finished_list = NULL;                    /* JB_Type */
   lList *pending_list = NULL;                     /* JB_Type */
   lList *pending_excludedlist = NULL;             /* JB_Type */
   lList *running_list = NULL;                     /* JB_Type */
   lList *error_list = NULL;                       /* JB_Type */
   lList *hold_list = NULL;                        /* JB_Type */
   int prof_job_count;

   int ret;
   int i;
#ifdef TEST_DEMO
   FILE *fpdjp;
#endif

   DENTER(TOP_LAYER, "scheduler");
#ifdef TEST_DEMO
   fpdjp = fopen("/tmp/sge_debug_job_place.out", "a");
#endif

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);

   prof_job_count = lGetNumberOfElem(lists->job_list);

   scheduled_fast_jobs    = 0;
   scheduled_complex_jobs = 0;
   schedd_mes_initialize();
   schedd_mes_set_logging(1); 

   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      splitted_job_lists[i] = NULL;
   }
   splitted_job_lists[SPLIT_WAITING_DUE_TO_PREDECESSOR] = 
                                               &waiting_due_to_pedecessor_list;
   splitted_job_lists[SPLIT_WAITING_DUE_TO_TIME] = &waiting_due_to_time_list;
   splitted_job_lists[SPLIT_PENDING_EXCLUDED] = &pending_excluded_list;
   splitted_job_lists[SPLIT_SUSPENDED] = &suspended_list;
   splitted_job_lists[SPLIT_FINISHED] = &finished_list;
   splitted_job_lists[SPLIT_PENDING] = &pending_list;
   splitted_job_lists[SPLIT_PENDING_EXCLUDED_INSTANCES] = &pending_excludedlist;
   splitted_job_lists[SPLIT_RUNNING] = &running_list;
   splitted_job_lists[SPLIT_ERROR] = &error_list;
   splitted_job_lists[SPLIT_HOLD] = &hold_list;

   split_jobs(&(lists->job_list), NULL, lists->all_queue_list, 
              conf.max_aj_instances, splitted_job_lists);
 
#if 0 /* EB: DEBUG */
   job_lists_print(splitted_job_lists);
#endif                      

   trash_splitted_jobs(splitted_job_lists);

   {
      lList *qlp;
      lCondition *where;
      lEnumeration *what;
      const lDescr *dp = lGetListDescr(lists->all_queue_list);
      lListElem *mes_queues;

      what = lWhat("%T(ALL)", dp); 
      where = lWhere("%T(%I m= %u "
         "|| %I m= %u "
         "|| %I m= %u "
         "|| %I m= %u "
         "|| %I m= %u "
         "|| %I m= %u "
         "|| %I m= %u)",
         dp,
         QU_state, QI_SUSPENDED,        /* only not suspended queues      */
         QU_state, QI_SUSPENDED_ON_SUBORDINATE,
         QU_state, QI_CAL_SUSPENDED,
         QU_state, QI_ERROR,            /* no queues in error state       */
         QU_state, QI_AMBIGUOUS,
         QU_state, QI_ORPHANED,
         QU_state, QI_UNKNOWN);         /* only known queues              */

      if (!what || !where) {
         DPRINTF(("failed creating where or what describing non available queues\n")); 
      }
      qlp = lSelect("", lists->all_queue_list, where, what);

      for_each(mes_queues, qlp)
         schedd_mes_add_global(SCHEDD_INFO_QUEUENOTAVAIL_, 
                                   lGetString(mes_queues, QU_full_name));

      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED, 
                      qlp, QU_full_name);
      lFreeList(qlp);
      lFreeWhere(where);
      lFreeWhat(what);
   }

   ret = dispatch_jobs(lists, &orderlist, splitted_job_lists);

#if 0
   if (ret) {
      /* only inconsistent data may lead to an exit here */ 
#ifdef TEST_DEMO
      fclose(fpdjp);
#endif
      DEXIT;
      return 0;
   }
#endif
   if(prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0);

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "PROF: scheduled in %.3f (u %.3f + s %.3f = %.3f): %d fast, %d complex, %d orders, %d H, %d Q, %d QA, %d J(qw), %d J(r), %d J(s), %d J(h), %d J(e),  %d J(x),%d J(all),  %d C, %d ACL, %d PE, %d U, %d D, %d PRJ, %d ST, %d CKPT, %d RU\n",
         prof_get_measurement_wallclock(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL) + 
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         scheduled_fast_jobs,
         scheduled_complex_jobs,
         lGetNumberOfElem(orderlist), 
         lGetNumberOfElem(lists->host_list), 
         lGetNumberOfElem(lists->queue_list),
         lGetNumberOfElem(lists->all_queue_list),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_RUNNING])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_SUSPENDED])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_HOLD])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_ERROR])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_FINISHED])),
         prof_job_count,
         lGetNumberOfElem(lists->centry_list),
         lGetNumberOfElem(lists->acl_list),
         lGetNumberOfElem(lists->pe_list),
         lGetNumberOfElem(lists->user_list),
         lGetNumberOfElem(lists->dept_list),
         lGetNumberOfElem(lists->project_list),
         lGetNumberOfElem(lists->share_tree),
         lGetNumberOfElem(lists->ckpt_list),
         lGetNumberOfElem(lists->running_per_user)
      ));
      log_state_set_log_level(saved_logginglevel);
   }
  
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM5);
   
   remove_immediate_jobs(*(splitted_job_lists[SPLIT_PENDING]), *(splitted_job_lists[SPLIT_RUNNING]), &orderlist);
   orderlist = sge_add_schedd_info(orderlist);

   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      if (splitted_job_lists[i]) {
         *(splitted_job_lists[i]) = lFreeList(*(splitted_job_lists[i]));
         splitted_job_lists[i] = NULL;
      }
   }

   if (orderlist) {
#ifdef TEST_DEMO
      fprintf(fpdjp, "An orderlist exists\n");
      lDumpList(fpdjp, orderlist, 5);
      fprintf(fpdjp, "\n");
      fclose(fpdjp);
#endif

      sge_send_orders2master(orderlist);
      orderlist = lFreeList(orderlist);
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM5);

   if(prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();
      log_state_set_log_level(LOG_INFO);
   
      INFO((SGE_EVENT, "PROF: send orders took: %.3f s\n", 
         prof_get_measurement_utime(SGE_PROF_CUSTOM5, true, NULL) ));

      log_state_set_log_level(saved_logginglevel);
   }

   schedd_mes_release();
   schedd_mes_set_logging(0); 

   DEXIT;
   return 0;
}

/****** schedd/scheduler/dispatch_jobs() **************************************
*  NAME
*     dispatch_jobs() -- dispatches jobs to queues
*
*  SYNOPSIS
*     static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, lList 
*     **running_jobs, lList **finished_jobs) 
*
*  FUNCTION
*     dispatch_jobs() is responsible for splitting
*     still running jobs into 'running_jobs' 
*
*  INPUTS
*     sge_Sdescr_t *lists   - all lists
*     lList **orderlist     - returns orders to be sent to qmaster
*     lList **running_jobs  - returns all running jobs
*     lList **finished_jobs - returns all finished jobs
*
*  RESULT
*     0   ok
*     -1  got inconsistent data
******************************************************************************/
static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, 
                         lList **splitted_job_lists[]) 
{
   lList *user_list=NULL, *group_list=NULL;
   lListElem *orig_job, *job, *cat;
   lList *susp_queues = NULL;
   lListElem *queue;  
   u_long32 queue_sort_method; 
   u_long32 maxujobs;
   const lList *job_load_adjustments = NULL;
#ifdef TEST_DEMO
   lListElem *host;
   FILE *fpdjp;
#endif
   double total_running_job_tickets=0; 
   int sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   int ndispatched = 0;
   int dipatch_type = DISPATCH_TYPE_NONE;
   lListElem *ja_task;
   int matched_pe_count = 0;
   int global_lc = 0;
   int host_order_changed = 0;  /* is passed to assign_select_debit, queue list needs new sorting */
   int sort_hostlist = 1;       /* hostlist has to be sorted. Info returned by select_assign_debit */
                                /* e.g. if load correction was computed */
   int nr_pending_jobs=0;
   DENTER(TOP_LAYER, "dispatch_jobs");

   queue_sort_method =  sconf_get_queue_sort_method();
   maxujobs = sconf_get_maxujobs(); 
   job_load_adjustments = sconf_get_job_load_adjustments();


#ifdef TEST_DEMO
   fpdjp = fopen("/tmp/sge_debug_job_place1.out", "a");
#endif
   /*---------------------------------------------------------------------
    * LOAD ADJUSTMENT
    *
    * Load sensors report load delayed. So we have to increase the load
    * of an exechost for each job started before load adjustment decay time.
    * load_adjustment_decay_time is a configuration value. 
    *---------------------------------------------------------------------*/
   {
   u_long32 decay_time = sconf_get_load_adjustment_decay_time(); 
   if ( decay_time ) {
      correct_load(*(splitted_job_lists[SPLIT_RUNNING]),
		   lists->queue_list,
		   lists->host_list,
		   decay_time);

      /* is there a "global" load value in the job_load_adjustments?
         if so this will make it necessary to check load thresholds
         of each queue after each dispatched job */
      {
         lListElem *gep, *lcep;
         if ((gep = host_list_locate(lists->host_list, "global"))) {
            for_each (lcep, job_load_adjustments) {
               const char *attr = lGetString(lcep, CE_name);
               if (lGetSubStr(gep, HL_name, attr, EH_load_list)) {
                  DPRINTF(("GLOBAL LOAD CORRECTION \"%s\"\n", attr));
                  global_lc = 1;
                  break;
               }
            }
         }
      }
   }
   }

   set_global_load_correction(global_lc);

   /*---------------------------------------------------------------------
    * CAPACITY CORRECTION
    *---------------------------------------------------------------------*/
   correct_capacities(lists->host_list, lists->centry_list);

   /*---------------------------------------------------------------------
    * KEEP SUSPEND THRESHOLD QUEUES
    *---------------------------------------------------------------------*/
   if (sge_split_queue_load(
         &(lists->queue_list),    /* queue list                             */
         &susp_queues,            /* list of queues in suspend alarm state  */
         lists->host_list,
         lists->centry_list,
         job_load_adjustments,
         NULL,
         QU_suspend_thresholds)) {
      DPRINTF(("couldn't split queue list with regard to suspend thresholds\n"));
      DEXIT;
      return -1;
   }

   unsuspend_job_in_queues(lists->queue_list, 
                           *(splitted_job_lists[SPLIT_SUSPENDED]), 
                           orderlist); 
   suspend_job_in_queues(susp_queues, 
                         *(splitted_job_lists[SPLIT_RUNNING]), 
                         orderlist); 
   lFreeList(susp_queues);

   /*---------------------------------------------------------------------
    * FILTER QUEUES
    *---------------------------------------------------------------------*/
   /* split queues into overloaded and non-overloaded queues */
   /* No migration of jobs supported yet - drop overloaded queues */
   DPRINTF(("before sge_split_queue_load()\n"));
   if (sge_split_queue_load(
         &(lists->queue_list), /* source list                              */
         NULL,                 /* no destination so they get trashed       */
         lists->host_list,     /* host list contains load values           */
         lists->centry_list,  /* complex list is needed to use load values */
         job_load_adjustments,
         NULL,
         QU_load_thresholds)) {
      DPRINTF(("couldn't split queue list concerning load\n"));

      DEXIT;
      return -1;
   }

   /* trash disabled queues - needed them for implementing suspend thresholds */
   if (sge_split_disabled(
         &(lists->queue_list), /* source list                              */
         NULL                  /* no destination so they get trashed       */
         )) {
      DPRINTF(("couldn't split queue list concerning disabled state\n"));
      DEXIT;
      return -1;
   }

   /* trash queues with less than 1 free slot */
   if (sge_split_queue_nslots_free(
         &(lists->queue_list), /* source list                              */
         NULL,                 /* no destination so they get trashed       */
         1                     /* the number of slots we need at least     */
         )) {
      DPRINTF(("couldn't split queue list concerning free slots\n"));
      DEXIT;
      return -1;
   }


   /*---------------------------------------------------------------------
    * FILTER JOBS
    *---------------------------------------------------------------------*/

   DPRINTF(("STARTING PASS 1 WITH %d PENDING JOBS\n", 
            lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]))));

   user_list_init_jc(&user_list, splitted_job_lists);

   queue = lFirst(lists->queue_list);
   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

   DPRINTF(("STARTING PASS 2 WITH %d PENDING JOBS\n",nr_pending_jobs ));

   /*
    * job categories are reset here, we need 
    *  - an update of the rejected field for every new run
    *  - the resource request dependent contribution is cached 
    *    per job category 
    */
   sge_reset_job_category(); 

   /*--------------------------------------------------------------------
    * CALL SGEEE SCHEDULER TO
    * CALCULATE TICKETS FOR EACH JOB  - IN SUPPORT OF SGEEE
    *------------------------------------------------------------------*/

   if (sgeee_mode) {
      int ret;
      PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);

      ret = sgeee_scheduler(lists, 
                    *(splitted_job_lists[SPLIT_RUNNING]),
                    *(splitted_job_lists[SPLIT_FINISHED]),
                    *(splitted_job_lists[SPLIT_PENDING]),
                    orderlist,
                    queue != NULL,
                    nr_pending_jobs > 0); 

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
      if (prof_is_active()) {
         u_long32 saved_logginglevel = log_state_get_log_level();

         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, "PROF: SGEEE job ticket calculation took %.3f s\n",
               prof_get_measurement_wallclock(SGE_PROF_CUSTOM1, true, NULL)));
         log_state_set_log_level(saved_logginglevel);
      }

      if(ret!=0){
         lFreeList(user_list);
         lFreeList(group_list);
         DEXIT;
         return -1;
      
      }
   }

   job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                                 &user_list,
                                                 NULL,
                                                 maxujobs);

   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

   trash_splitted_jobs(splitted_job_lists);

   if(queue == NULL) {
      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_mes_add_global(SCHEDD_INFO_ALLALARMOVERLOADED_);

      DEXIT;
      return 0;
   }

   if (nr_pending_jobs == 0) {
      /* no jobs to schedule */
      SCHED_MON((log_string, MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON ));
      lFreeList(user_list);
      lFreeList(group_list);
      DEXIT;
      return 0;
   }

   /*---------------------------------------------------------------------
    * SORT QUEUES OR HOSTS
    *---------------------------------------------------------------------*/
   /* 
      there are two possibilities for SGE administrators 
      selecting queues:

      sort by seq_no
         the sequence number from configuration is used for sorting
     
      sort by load (using a load formula)
         the least loaded queue gets filled first

         to do this we sort the hosts using the load formula
         because there may be more queues than hosts and
         the queue load is identically to the host load

   */
   switch (queue_sort_method) {
   case QSM_LOAD:
   case QSM_SEQNUM:   
   default:

      DPRINTF(("sorting hosts by load\n"));
      sort_host_list(lists->host_list, lists->centry_list);

#ifdef TEST_DEMO
      fprintf(fpdjp, "after sort by  load");

      for_each(host, lists->host_list) {
         fprintf(fpdjp, "load value for host %s is %f\n",
                 lGetHost(host, EH_name), lGetDouble(host, EH_sort_value));
      }

#endif

      break;
   }

   /*---------------------------------------------------------------------
    * DISPATCH JOBS TO QUEUES
    *---------------------------------------------------------------------*/

   if (!sgeee_mode) {
      /* 
       * establish the access tree for all job arrays containing runnable tasks 
       */
      at_notice_runnable_job_arrays(*splitted_job_lists[SPLIT_PENDING]);
   }

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM4);

   /*
    * loop over the jobs that are left in priority order
    */
   while ((orig_job=(sgeee_mode?lFirst(*(splitted_job_lists[SPLIT_PENDING]))
          : at_get_actual_job_array(*(splitted_job_lists[SPLIT_PENDING])))) &&
         (job = lCopyElem(orig_job))) {
      u_long32 job_id; 
      u_long32 ja_task_id; 
      lListElem *pe;
      const char *pe_name;
      lListElem *ckpt;
      const char *ckpt_name;
      int dispatched_a_job;


      /* sort the hostlist */
      if(sort_hostlist) {
         lPSortList(lists->host_list, "%I+", EH_sort_value);
         sort_hostlist      = 0;
         host_order_changed = 1;
      } else {
         host_order_changed = 0;
      }   

      dispatched_a_job = 0;
      job_id = lGetUlong(job, JB_job_number);
      DPRINTF(("-----------------------------------------\n"));

      /*------------------------------------------------------------------ 
       * check if the category of this job has already been rejected 
       * if yes:  goto SKIP_THIS_JOB 
       * if no:   add the new category to the category list and if this
       *          job is rejected mark the categories rejected field
       *          for the following jobs
       *------------------------------------------------------------------*/
      if (sgeee_mode && sge_is_job_category_rejected(job)) {
         goto SKIP_THIS_JOB;
      }

      ja_task = lFirst(lGetList(job, JB_ja_tasks));
      if (!ja_task) {
         lList *answer_list = NULL;

         ja_task_id = range_list_get_first_id(lGetList(job, JB_ja_n_h_ids),
                                              &answer_list);
         if (answer_list_has_error(&answer_list)) {
            answer_list = lFreeList(answer_list);
            DPRINTF(("Found job "u32" with no job array tasks\n", job_id));
            goto SKIP_THIS_JOB; 
         } else {
            ja_task = job_get_ja_task_template_pending(job, ja_task_id);
         }
      } else {
         ja_task_id = lGetUlong(ja_task, JAT_task_number);
      }

      DPRINTF(("Found pending job "u32"."u32"\n", job_id, ja_task_id));
      DPRINTF(("-----------------------------------------\n"));

      /*------------------------------------------------------------------ 
       * seek for ckpt interface definition requested by the job 
       * in case of ckpt jobs 
       *------------------------------------------------------------------*/
      ckpt = NULL;
      if ((ckpt_name = lGetString(job, JB_checkpoint_name))) {
         ckpt = ckpt_list_locate(lists->ckpt_list, ckpt_name);
         if (!ckpt) {
            schedd_mes_add(lGetUlong(job, JB_job_number), 
               SCHEDD_INFO_CKPTNOTFOUND_);
            goto SKIP_THIS_JOB;
         }
      }

      /*------------------------------------------------------------------ 
       * seek for parallel environment requested by the job 
       * in case of parallel jobs 
       *------------------------------------------------------------------*/
      if ((pe_name = lGetString(job, JB_pe))) {
         for_each(pe, lists->pe_list) {
            if (pe_is_matching(pe, pe_name)) {
               matched_pe_count++;
            } else {
               continue;
            }

            /* any objections from pe ? */
            if (pe_restricted(job, pe, lists->acl_list))
               continue;

            DPRINTF(("handling parallel job "u32"."u32"\n", job_id, 
               ja_task_id)); 
            if (select_assign_debit(
                  &(lists->queue_list), 
                  splitted_job_lists[SPLIT_PENDING], 
                  job, ja_task, pe, ckpt, 
                  lists->centry_list, 
                  lists->host_list, 
                  lists->acl_list,
                  &user_list,
                  &group_list,
                  orderlist,
                  &total_running_job_tickets, 
                  ndispatched,
                  &dipatch_type,
                  host_order_changed,
                  &sort_hostlist))
               continue;

            ndispatched++;       
            dispatched_a_job = 1;
            DPRINTF(("Success to schedule job "u32"."u32" to pe \"%s\"\n",
               job_id, ja_task_id, lGetString(pe, PE_name)));
            break;
         }
         if ( matched_pe_count == 0 ) {
            DPRINTF(("NO PE MATCHED!\n"));
            schedd_mes_add(job_id, SCHEDD_INFO_NOPEMATCH_ ); 
         }
      } else {
         DPRINTF(("handling job "u32"."u32"\n", job_id, ja_task_id)); 
         if (!select_assign_debit(
               &(lists->queue_list), 
               splitted_job_lists[SPLIT_PENDING], 
               job, ja_task, NULL, ckpt, 
               lists->centry_list, 
               lists->host_list, 
               lists->acl_list,
               &user_list,
               &group_list,
               orderlist,
               &total_running_job_tickets, 
               ndispatched,
               &dipatch_type,
               host_order_changed,
               &sort_hostlist)) {
            ndispatched++;                  
            dispatched_a_job = 1;
         }
      }

SKIP_THIS_JOB:
      if (dispatched_a_job) {
         schedd_mes_rollback();

         job_move_first_pending_to_running(&orig_job, splitted_job_lists);

         /* 
          * after sge_move_to_running() orig_job can be removed and job 
          * should be used instead 
          */
         orig_job = NULL;

         /* notify access tree */
         if (!sgeee_mode) 
            at_dispatched_a_task(job, 1);
      
         /* 
          * drop idle jobs that exceed maxujobs limit 
          * should be done after resort_job() 'cause job is referenced 
          */
         job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                             &user_list, 
                                             lGetString(job, JB_owner),
                                             maxujobs);
         trash_splitted_jobs(splitted_job_lists);
      } else {
         schedd_mes_commit(*(splitted_job_lists[SPLIT_PENDING]), 0);

         /* before deleting the element mark the category as rejected */
         cat = lGetRef(job, JB_category);
         if (cat) {
            DPRINTF(("SKIP JOB " u32 " of category '%s' (rc: "u32 ")\n", job_id, 
                        lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
            sge_reject_category(cat);
         }

         /* prevent that we get the same job next time again */
         lDelElemUlong(splitted_job_lists[SPLIT_PENDING], JB_job_number, job_id); 
      }

      lFreeElem(job);

      /*------------------------------------------------------------------ 
       * SGEEE mode - if we dispatch a job sub-task and the job has more
       * sub-tasks, then the job is still first in the job list.
       * We need to remove and reinsert the job back into the sorted job
       * list in case another job is higher priority (i.e. has more tickets)
       *------------------------------------------------------------------*/

      if (sgeee_mode && dispatched_a_job) {
         sgeee_resort_pending_jobs(splitted_job_lists[SPLIT_PENDING],
                                   *orderlist);
      }

      /* no more free queues - then exit */
      if (lGetNumberOfElem(lists->queue_list)==0)
         break;
#ifdef TEST_DEMO
       if (queue_sort_method == QSM_SEQNUM)  {
            fprintf(fpdjp, "using sort_by_seq_no method - after sge_debit_job:\n");
            fprintf(fpdjp, " \n");
            fprintf(fpdjp, "QUEUE ORDER:\n");
            for_each(queue, lists->queue_list)  {
               fprintf(fpdjp, "  queue %s\n", lGetString(queue, QU_full_name));
            }
       }
       if (queue_sort_method == QSM_LOAD)  {
            fprintf(fpdjp, "using sort_by_load method - after sge_debit_job:\n");
            for_each(host, lists->host_list) {
               fprintf(fpdjp, "load value for host %s is %f\n", lGetHost(host, EH_name), lGetDouble(host, EH_sort_value));
            }
            fprintf(fpdjp, " \n");
            fprintf(fpdjp, "QUEUE ORDER:\n");
            for_each(queue, lists->queue_list)  {
               fprintf(fpdjp, "  queue %s\n", lGetString(queue, QU_full_name));
            }
        }
#endif

   } /* end of while */

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM4);

   if (sgeee_mode && do_profiling) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "PROF: SGEEE job dispatching took %.3f s\n",
            prof_get_measurement_wallclock(SGE_PROF_CUSTOM4, false, NULL)));
      log_state_set_log_level(saved_logginglevel);
   }

   lFreeList(user_list);
   lFreeList(group_list);

#ifdef TEST_DEMO
   fclose(fpdjp);
#endif

   DEXIT;
   return 0;
}



/****** schedd/scheduler/select_assign_debit() ********************************
*  NAME
*     select_assign_debit()
*
*  FUNCTION
*     Selects resources for 'job', add appropriate order to the 'orders_list',
*     debits resources of this job for the next dispatch and sort out no longer
*     available queues from the 'queue_list'.
******************************************************************************/
static int select_assign_debit(
lList **queue_list,
lList **job_list,
lListElem *job,
lListElem *ja_task,
lListElem *pe,
lListElem *ckpt,
lList *centry_list,
lList *host_list,
lList *acl_list,
lList **user_list,
lList **group_list,
lList **orders_list,
double *total_running_job_tickets,
int ndispatched,
int *dispatch_type, 
int host_order_changed,
int *sort_hostlist
) {
   int sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   const lList *job_load_adjustments = sconf_get_job_load_adjustments();   
   lListElem *granted_el;     
   lList *granted = NULL;

   DENTER(TOP_LAYER, "select_assign_debit");

   /*------------------------------------------------------------------
    * SELECT POSSIBLE QUEUE(S) FOR THIS JOB
    *------------------------------------------------------------------*/
   granted = sge_replicate_queues_suitable4job(
      *queue_list,           /* source list of available queues         */
      job,                   /* job that needs the queues               */
      ja_task,               /* task that needs the queues              */
      pe,                    /* selects queues (queue list of pe)       */
      ckpt,                  /* selects queues (queue list of ckpt)     */
      sconf_get_queue_sort_method() ,/* sort order to use                       */
      centry_list,         /* to interpret job requests               */
      host_list,            /* for load/architecture                   */
      acl_list,             /* use these access lists                  */
      job_load_adjustments,
      ndispatched,
      dispatch_type,
      host_order_changed);

   /* the following was added to support SGE - 
      account for job tickets on hosts due to parallel jobs */
   if (granted && sgeee_mode) {
      double job_tickets_per_slot, nslots;
      double job_ftickets_per_slot;
      double job_otickets_per_slot;
      double job_stickets_per_slot;
      nslots = nslots_granted(granted, NULL);

      job_tickets_per_slot =(double)lGetDouble(ja_task, JAT_tix)/nslots;
      job_ftickets_per_slot =(double)lGetDouble(ja_task, JAT_fticket)/nslots;
      job_otickets_per_slot =(double)lGetDouble(ja_task, JAT_oticket)/nslots;
      job_stickets_per_slot =(double)lGetDouble(ja_task, JAT_sticket)/nslots;


      for_each(granted_el, granted) {
         u_long32 granted_slots = lGetUlong(granted_el, JG_slots);
         lSetDouble(granted_el, JG_ticket, job_tickets_per_slot * granted_slots);
         lSetDouble(granted_el, JG_oticket ,job_otickets_per_slot  * granted_slots);
         lSetDouble(granted_el, JG_fticket ,job_ftickets_per_slot  * granted_slots);
         lSetDouble(granted_el, JG_sticket ,job_stickets_per_slot  * granted_slots);

      }
      *total_running_job_tickets += lGetDouble(ja_task, JAT_tix);
   }

   /*------------------------------------------------------------------
    * BUILD ORDERS LIST THAT TRANSFERS OUR DECISIONS TO QMASTER
    *------------------------------------------------------------------*/
   if (!granted) {
      /* failed scheduling this job */
      if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) /* immediate job */
         /* generate order for removing it at qmaster */
         order_remove_immediate(job, ja_task, orders_list);
      DEXIT;
      return -1;
   }

   if (pe) {
      DPRINTF(("got PE %s\n", lGetString(pe, PE_name)));
      lSetString(ja_task, JAT_granted_pe, lGetString(pe, PE_name));
   } else 
      DPRINTF(("got no PE\n"));

   job_log(lGetUlong(job, JB_job_number), lGetUlong(ja_task, JAT_task_number), 
      "dispatched");
   *orders_list = sge_create_orders(*orders_list, ORT_start_job, 
         job, ja_task, granted, false, true);
   
   /*------------------------------------------------------------------
    * DEBIT JOBS RESOURCES IN DIFFERENT OBJECTS
    *
    * We have not enough time to wait till qmaster updates our lists.
    * So we do the work by ourselfs for being able to send more than
    * one order per scheduling epoch.
    *------------------------------------------------------------------*/

   /* increase the number of jobs a user/group has running */
   sge_inc_jc(user_list, lGetString(job, JB_owner), 1);

   debit_scheduled_job(job, granted, *queue_list, pe, host_list, 
         centry_list, sort_hostlist, *orders_list);

   /*------------------------------------------------------------------
    * REMOVE QUEUES THAT ARE NO LONGER USEFUL FOR FURTHER SCHEDULING
    *------------------------------------------------------------------*/
   sge_split_suspended(queue_list, NULL);
   sge_split_queue_nslots_free(queue_list, NULL, 1);

   /* split queues into overloaded and non-overloaded queues */
   /* No migration of jobs supported yet - drop overloaded queues */
   if (sge_split_queue_load(
         queue_list, /* source list                              */
         NULL,                 /* no destination so they get trashed       */
         host_list,     /* host list contains load values           */
         centry_list, 
         job_load_adjustments,
         granted,
         QU_load_thresholds)) {   /* use load thresholds here */
         
      DPRINTF(("couldn't split queue list concerning load\n"));
      DEXIT;
      return -1;
   }

   /* no longer needed - having built the order 
      and debited the job everywhere */
   lFreeList(granted);
  
   DEXIT;
   return 0;
}

