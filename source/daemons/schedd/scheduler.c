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
#include <float.h>
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
#include "sge_qeti.h"
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
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_schedd_text.h"
#include "job_log.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_centry.h"
#include "sge_ckpt.h"
#include "sge_host.h"
#include "sge_schedd_conf.h"
#include "sge_resource_utilization.h"
#include "sge_serf.h"
#include "sge_qeti.h"
#include "sge_qinstance_state.h"

/* profiling info */
extern int scheduled_fast_jobs;
extern int scheduled_complex_jobs;
extern int do_profiling;

/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, lList **splitted_job_list[]);

static int select_assign_debit(lList **queue_list, lList **job_list, lListElem *job, lListElem *ja_task, lList *pe_list, 
   lList *ckpt_list, lList *centry_list, lList *host_list, lList *acl_list, lList **user_list, lList **group_list, 
   lList **orders_list, double *total_running_job_tickets, int *sort_hostlist, bool dont_start, bool dont_reserve);

static int sge_select_parallel_environment(sge_assignment_t *best, lList *pe_list);

static int sge_select_pe_time(sge_assignment_t *best);
static int sge_maximize_slots(sge_assignment_t *best);

static u_long32 determine_default_duration(const lList *centry_list);
static bool job_get_duration(u_long32 *duration, const lListElem *jep);
static void prepare_resource_schedules(const lList *running_jobs, const lList *suspended_jobs, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *centry_list);

static void assignment_init(sge_assignment_t *a, lListElem *job, lListElem *ja_task);
static void assignment_copy(sge_assignment_t *dst, sge_assignment_t *src, bool move_gdil);
static void assignment_release(sge_assignment_t *a);

static void assignment_init(sge_assignment_t *a, lListElem *job, lListElem *ja_task)
{
   memset(a, 0, sizeof(sge_assignment_t));
   a->job         = job;
   a->ja_task     = ja_task;
   a->job_id      = lGetUlong(job,     JB_job_number);
   a->ja_task_id  = lGetUlong(ja_task, JAT_task_number);
}

static void assignment_copy(sge_assignment_t *dst, sge_assignment_t *src, bool move_gdil)
{
   if (move_gdil) 
      dst->gdil = lFreeList(dst->gdil);

   memcpy(dst, src, sizeof(sge_assignment_t));
  
   if (!move_gdil)
      dst->gdil = NULL; 
   else
      src->gdil = NULL; 
}
static void assignment_release(sge_assignment_t *a)
{
   lFreeList(a->gdil);
}


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

   DENTER(TOP_LAYER, "scheduler");

   serf_new_interval(sge_get_gmt());

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
   lList *non_avail_queues = NULL;

   bool queues_available;  
   u_long32 queue_sort_method; 
   u_long32 maxujobs;
   const lList *job_load_adjustments = NULL;
   double total_running_job_tickets=0; 
   int sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   int nreservation = 0;
#if 0
   int dipatch_type = DISPATCH_TYPE_NONE;
#endif
   lListElem *ja_task;
   int global_lc = 0;
   int sort_hostlist = 1;       /* hostlist has to be sorted. Info returned by select_assign_debit */
                                /* e.g. if load correction was computed */
   int nr_pending_jobs=0;
   int max_reserve = sconf_get_max_reservations();

   DENTER(TOP_LAYER, "dispatch_jobs");

   queue_sort_method =  sconf_get_queue_sort_method();
   maxujobs = sconf_get_maxujobs(); 
   job_load_adjustments = sconf_get_job_load_adjustments();
   sconf_set_host_order_changed(false);


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

   sconf_set_global_load_correction(global_lc);
   
   if (max_reserve != 0) {
      /*----------------------------------------------------------------------
       * ENSURE RUNNING JOBS ARE REFLECTED IN PER RESOURCE SCHEDULE
       *---------------------------------------------------------------------*/
      sconf_set_default_duration(determine_default_duration(lists->centry_list));
      prepare_resource_schedules(*(splitted_job_lists[SPLIT_RUNNING]),
                              *(splitted_job_lists[SPLIT_SUSPENDED]),
                              lists->pe_list, lists->host_list, lists->queue_list, 
                              lists->centry_list);
      utilization_print_all(lists->pe_list, lists->host_list, lists->queue_list); 
   }

   /*---------------------------------------------------------------------
    * CAPACITY CORRECTION
    *---------------------------------------------------------------------*/
   correct_capacities(lists->host_list, lists->centry_list);

   /*---------------------------------------------------------------------
    * KEEP SUSPEND THRESHOLD QUEUES
    *---------------------------------------------------------------------*/
   if (sge_split_queue_load(
         &(lists->queue_list),    /* queue list                             */
         &non_avail_queues,            /* list of queues in suspend alarm state  */
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
   suspend_job_in_queues(non_avail_queues, 
                         *(splitted_job_lists[SPLIT_RUNNING]), 
                         orderlist); 
   non_avail_queues = lFreeList(non_avail_queues);

   /*---------------------------------------------------------------------
    * FILTER QUEUES
    *---------------------------------------------------------------------*/
   /* split queues into overloaded and non-overloaded queues */
   if (sge_split_queue_load(&(lists->queue_list), NULL, 
         lists->host_list, lists->centry_list, job_load_adjustments,
         NULL, QU_load_thresholds)) {
      DPRINTF(("couldn't split queue list concerning load\n"));
      lFreeList(non_avail_queues);
      DEXIT;
      return -1;
   }

   /* trash disabled queues - needed them for implementing suspend thresholds */
   if (sge_split_disabled(&(lists->queue_list), NULL)) {
      DPRINTF(("couldn't split queue list concerning disabled state\n"));
      lFreeList(non_avail_queues);
      DEXIT;
      return -1;
   }

   /* tag queue instances with less than one free slot */
   if (sge_split_queue_slots_free(&(lists->queue_list), &non_avail_queues)) {
      DPRINTF(("couldn't split queue list concerning free slots\n"));
      lFreeList(non_avail_queues);
      DEXIT;
      return -1;
   }

   /* Once we now if there are available queues we put
      the non available ones back into our main queue list
      this is actually needed for reservation scheduling */
   queues_available = (lFirst(lists->queue_list) != NULL);
   if (lists->queue_list == NULL)
      lists->queue_list = non_avail_queues;
   else 
      lAddList(lists->queue_list, non_avail_queues);
   non_avail_queues = NULL;

   /*---------------------------------------------------------------------
    * FILTER JOBS
    *---------------------------------------------------------------------*/

   DPRINTF(("STARTING PASS 1 WITH %d PENDING JOBS\n", 
            lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]))));

   user_list_init_jc(&user_list, splitted_job_lists);

   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

   DPRINTF(("STARTING PASS 2 WITH %d PENDING JOBS\n",nr_pending_jobs ));

   /*
    * job categories are reset here, we need 
    *  - an update of the rejected field for every new run
    *  - the resource request dependent urgency contribution is cached 
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
                    queues_available,
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

   if (!queues_available) {
      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_mes_add_global(SCHEDD_INFO_ALLALARMOVERLOADED_);
      lFreeList(user_list);
      lFreeList(group_list);
      lFreeList(non_avail_queues);
      DEXIT;
      return 0;
   } 
   
   if (nr_pending_jobs == 0) {
      /* no jobs to schedule */
      SCHED_MON((log_string, MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON ));
      lFreeList(user_list);
      lFreeList(group_list);
      lFreeList(non_avail_queues);
      DEXIT;
      return 0;
   }

   /*---------------------------------------------------------------------
    * SORT HOSTS
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

   /* we will assume this time as start time for now assignments */
   sconf_set_now(sge_get_gmt());


   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM4);

   /*
    * loop over the jobs that are left in priority order
    */
   while ((orig_job=(sgeee_mode?lFirst(*(splitted_job_lists[SPLIT_PENDING]))
          : at_get_actual_job_array(*(splitted_job_lists[SPLIT_PENDING])))) &&
         (job = lCopyElem(orig_job))) {
      int result = -1;

      u_long32 job_id; 
      u_long32 ja_task_id; 
      bool dispatched_a_job = false;
      bool dont_reserve, dont_start;

      /* sort the hostlist */
      if(sort_hostlist) {
         lPSortList(lists->host_list, "%I+", EH_sort_value);
         sort_hostlist      = 0;
         sconf_set_host_order_changed(true);
      } else {
         sconf_set_host_order_changed(false);
      }   

      job_id = lGetUlong(job, JB_job_number);

      /*------------------------------------------------------------------ 
       * check if the category of this job has already been rejected 
       * if yes:  goto SKIP_THIS_JOB 
       * if no:   add the new category to the category list and if this
       *          job is rejected mark the categories rejected field
       *          for the following jobs
       *------------------------------------------------------------------*/
  

      if (job_get_next_task(job, &ja_task, &ja_task_id)!=0) {
         DPRINTF(("Found job "u32" with no job array tasks\n", job_id));
      } else { 

         /* 
          * We don't try to get a reservation, if 
          * - reservation is generally disabled 
          * - maximum number of reservations is exceeded 
          * - it's not desired for the job
          * - the job is an immediate one
          */
         if (nreservation < max_reserve &&
             lGetBool(job, JB_reserve)==true &&
             !JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type)))
            dont_reserve = false;
         else
            dont_reserve = true;

         /* Don't need to look for a 'now' assignment if the last job 
            of this category got no 'now' assignement either */
         if (sgeee_mode && sge_is_job_category_rejected(job))
            dont_start = true;
         else
            dont_start = false;

         DPRINTF(("Found pending job "u32"."u32". Try %sto start and %sto reserve\n", 
               job_id, ja_task_id, dont_start?"not ":"", dont_reserve?"not ":""));
         DPRINTF(("-----------------------------------------\n"));

         if (dont_start && dont_reserve)
            result = -1;
         else
            result = select_assign_debit(
               &(lists->queue_list), 
               splitted_job_lists[SPLIT_PENDING], 
               job, ja_task, 
               lists->pe_list, 
               lists->ckpt_list, 
               lists->centry_list, 
               lists->host_list, 
               lists->acl_list,
               &user_list,
               &group_list,
               orderlist,
               &total_running_job_tickets, 
               &sort_hostlist, 
               dont_start,
               dont_reserve);
      
      }
      switch (result) {
      case 0: /* now assignment */
         /* here the job got an assignment that will cause it be started immediately */
         DPRINTF(("Found NOW assignment\n"));
         schedd_mes_rollback();
         dispatched_a_job = true;
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
         break;

      case 1: /* reservation */
         /* here the job got a reservation but can't be started now */
         DPRINTF(("Got a RESERVATION\n"));
         nreservation++;
         /* no break */

      case -1: /* never this category */
      case -2: /* never this particular job */

         /* here no reservation was made for a job that couldn't be started now 
            or the job is not dispatchable at all */
         schedd_mes_commit(*(splitted_job_lists[SPLIT_PENDING]), 0);

         /* before deleting the element mark the category as rejected */
         if ((result == -1 || result == 1) && (cat = lGetRef(job, JB_category))) {
            DPRINTF(("SKIP JOB " u32 " of category '%s' (rc: "u32 ")\n", job_id, 
                        lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
            sge_reject_category(cat);
         }

         /* prevent that we get the same job next time again */
         lDelElemUlong(splitted_job_lists[SPLIT_PENDING], JB_job_number, job_id); 
         break;
      default:
         break;
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
*
*  INPUTS
*
*  RESULT
*     int - 0 ok got an assignment now
*           1 got a reservation assignment
*          -1 will never get an assignment for that category
*          -2 will never get an assignment for that particular job
******************************************************************************/
static int select_assign_debit(
lList **queue_list,
lList **job_list,
lListElem *job,
lListElem *ja_task,
lList *pe_list,
lList *ckpt_list,
lList *centry_list,
lList *host_list,
lList *acl_list,
lList **user_list,
lList **group_list,
lList **orders_list,
double *total_running_job_tickets,
int *sort_hostlist,
bool dont_start,  /* don't try to find a now assignment */
bool dont_reserve /* don't try to find a reservation assignment */
) {
   int sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   lListElem *granted_el;     
   int result = 1;
   const char *pe_name, *ckpt_name;
   sge_assignment_t a;

   DENTER(TOP_LAYER, "select_assign_debit");

   assignment_init(&a, job, ja_task);
   a.load_adjustments = sconf_get_job_load_adjustments();
   a.queue_list       = *queue_list;
   a.host_list        = host_list;
   a.centry_list      = centry_list;
   a.acl_list         = acl_list;

   if (sconf_get_max_reservations()!=0 && !job_get_duration(&a.duration, job)) {
      schedd_mes_add(a.job_id, SCHEDD_INFO_CKPTNOTFOUND_);
      return -1;
   }

   /*------------------------------------------------------------------ 
    * seek for ckpt interface definition requested by the job 
    * in case of ckpt jobs 
    *------------------------------------------------------------------*/
   if ((ckpt_name = lGetString(job, JB_checkpoint_name))) {
      a.ckpt = ckpt_list_locate(ckpt_list, ckpt_name);
      if (!a.ckpt) {
         schedd_mes_add(a.job_id, SCHEDD_INFO_CKPTNOTFOUND_);
         return -1;
      }
   }

   /*------------------------------------------------------------------ 
    * if a global host pointer exists it is needed everywhere 
    * down in the assignment code (tired of searching/passing it).
    *------------------------------------------------------------------*/
   a.gep = host_list_locate(host_list, SGE_GLOBAL_NAME);

   /*------------------------------------------------------------------
    * SELECT POSSIBLE QUEUE(S) FOR THIS JOB
    *------------------------------------------------------------------*/
   if ((pe_name = lGetString(job, JB_pe))) {

      if (!dont_start) {
         DPRINTF(("### looking for immediate parallel assignment\n")); 
         a.start = DISPATCH_TIME_NOW;
         result = sge_select_parallel_environment(&a, pe_list);
      }

      if (result == 1) {
         if (!dont_reserve) {
            DPRINTF(("### looking for parallel reservation\n")); 
            a.start = DISPATCH_TIME_QUEUE_END;
            result = sge_select_parallel_environment(&a, pe_list);
            if (result == 0)
               result = 1; /* this job got a reservation */
         } else
            result = -2; /* this particular job couldn't be assigned but got no reservation either */
      }

   } else {
      lList *ignore_queues = NULL, *ignore_hosts = NULL;
      lListElem *category = lGetRef(job, JB_category);
      bool use_category = (category != NULL) && lGetUlong(category, CT_refcount) > MIN_JOBS_IN_CATEGORY;

      a.slots = 1;
      DPRINTF(("handling sequenial job "u32"."u32"\n", a.job_id, a.ja_task_id)); 

      if (!dont_start) {
         DPRINTF(("### looking for immediate sequential assignment\n")); 
         a.start = DISPATCH_TIME_NOW;

         result = sge_sequential_assignment(&a, &ignore_queues, &ignore_hosts);
          
         DPRINTF(("sge_sequential_assignment(immediate) returned %d\n", result));

         if (result == -1) {
            /* we store these hosts/queues only if an assignment is not possible now */
            if (use_category) {
               lAddSubList(category, CT_ignore_queues, ignore_queues);
               lAddSubList(category, CT_ignore_hosts, ignore_hosts);
            }
         }
      }

      /* don't try to reserve for jobs that can not be dispatched with the current configuration */
      if (result == 1) {
         if (!dont_reserve) {
            DPRINTF(("### looking for sequential reservation\n")); 
            a.start = DISPATCH_TIME_QUEUE_END;

            result = sge_sequential_assignment(&a, NULL, NULL);
            if (result == 0)
               result = 1; /* this job got a reservation */
         } else 
            result = -2; /* this particular job can't be assigned but got no reservation either */
      } 
   }

   /*------------------------------------------------------------------
    * BUILD ORDERS LIST THAT TRANSFERS OUR DECISIONS TO QMASTER
    *------------------------------------------------------------------*/
   if (result == -1 || result == -2) {
      /* failed scheduling this job */
      if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) /* immediate job */
         /* generate order for removing it at qmaster */
         order_remove_immediate(job, ja_task, orders_list);
      DEXIT;
      return result;
   }

   if (result == 0) {
      /* in SGEEE we must account for job tickets on hosts due to parallel jobs */
      if (sgeee_mode) {
         double job_tickets_per_slot;
         double job_ftickets_per_slot;
         double job_otickets_per_slot;
         double job_stickets_per_slot;

         job_tickets_per_slot =(double)lGetDouble(ja_task, JAT_tix)/a.slots;
         job_ftickets_per_slot =(double)lGetDouble(ja_task, JAT_fticket)/a.slots;
         job_otickets_per_slot =(double)lGetDouble(ja_task, JAT_oticket)/a.slots;
         job_stickets_per_slot =(double)lGetDouble(ja_task, JAT_sticket)/a.slots;


         for_each(granted_el, a.gdil) {
            u_long32 granted_slots = lGetUlong(granted_el, JG_slots);
            lSetDouble(granted_el, JG_ticket, job_tickets_per_slot * granted_slots);
            lSetDouble(granted_el, JG_oticket, job_otickets_per_slot  * granted_slots);
            lSetDouble(granted_el, JG_fticket, job_ftickets_per_slot  * granted_slots);
            lSetDouble(granted_el, JG_sticket, job_stickets_per_slot  * granted_slots);
         }
         *total_running_job_tickets += lGetDouble(ja_task, JAT_tix);
      }

      if (a.pe) {
         DPRINTF(("got PE %s\n", lGetString(a.pe, PE_name)));
         lSetString(ja_task, JAT_granted_pe, lGetString(a.pe, PE_name));
      }

      job_log(a.job_id, lGetUlong(ja_task, JAT_task_number), 
         "dispatched");
      *orders_list = sge_create_orders(*orders_list, ORT_start_job, 
            job, ja_task, a.gdil, false, true);

      /* increase the number of jobs a user/group has running */
      sge_inc_jc(user_list, lGetString(job, JB_owner), 1);
   }

   /*------------------------------------------------------------------
    * DEBIT JOBS RESOURCES IN DIFFERENT OBJECTS
    *
    * We have not enough time to wait till qmaster updates our lists.
    * So we do the work by ourselfs for being able to send more than
    * one order per scheduling epoch.
    *------------------------------------------------------------------*/

   /* when a job is started *now* the RUE_used_now must always change */
   /* if jobs with reservations are ahead then we also must change the RUE_utilized */ 
   if (result==0) {
      if (a.start == DISPATCH_TIME_NOW)
         a.start = sconf_get_now();
      debit_scheduled_job(&a, sort_hostlist, *orders_list, true, 
               SCHEDULING_RECORD_ENTRY_TYPE_STARTING);
   } else
      debit_scheduled_job(&a, sort_hostlist, *orders_list, true, 
            SCHEDULING_RECORD_ENTRY_TYPE_RESERVING);

   /*------------------------------------------------------------------
    * REMOVE QUEUES THAT ARE NO LONGER USEFUL FOR FURTHER SCHEDULING
    *------------------------------------------------------------------*/
   if (!sconf_get_max_reservations()) {
      sge_split_suspended(queue_list, NULL);
      sge_split_queue_slots_free(queue_list, NULL);

      /* split queues into overloaded and non-overloaded queues */
      if (sge_split_queue_load(
            queue_list, /* source list                              */
            NULL,                 /* no destination so they get trashed       */
            host_list,     /* host list contains load values           */
            centry_list, 
            a.load_adjustments,
            a.gdil,
            QU_load_thresholds)) {   /* use load thresholds here */
            
         DPRINTF(("couldn't split queue list concerning load\n"));
         assignment_release(&a);
         DEXIT;
         return MATCH_NEVER;
      }
   }

   /* no longer needed - having built the order 
      and debited the job everywhere */
   assignment_release(&a);
  
   DEXIT;
   return result;
}



/****** scheduler/sge_select_parallel_environment() ****************************
*  NAME
*     sge_select_parallel_environment() -- Decide about a PE assignment 
*
*  SYNOPSIS
*     static int sge_select_parallel_environment(sge_assignment_t *best, lList 
*     *pe_list, lList *host_list, lList *queue_list, lList *centry_list, lList 
*     *acl_list) 
*
*  FUNCTION
*     When users use wildcard PE request such as -pe <pe_range> 'mpi8_*' 
*     more than a single parallel environment can match the wildcard expression. 
*     In case of 'now' assignments the PE that gives us the largest assignment 
*     is selected. When scheduling a reservation we search for the earliest 
*     assignment for each PE and then choose that one that finally gets us the 
*     maximum number of slots.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*     lList *pe_list         - ??? 
*     lList *host_list       - ??? 
*     lList *queue_list      - ??? 
*     lList *centry_list     - ??? 
*     lList *acl_list        - ??? 
*
*  RESULT
*     int - 0 ok got an assignment
*           1 no assignment at the specified time (???)
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_select_parallel_environment() is not MT safe 
*******************************************************************************/
static int sge_select_parallel_environment( sge_assignment_t *best, lList *pe_list) 
{
   int matched_pe_count = 0;
   bool now_assignment = (best->start == DISPATCH_TIME_NOW);
   lListElem *pe;
   const char *pe_request;
   int result, best_result = -1;

   DENTER(TOP_LAYER, "sge_select_parallel_environment");

   pe_request = lGetString(best->job, JB_pe);

   DPRINTF(("handling parallel job "u32"."u32"\n", best->job_id, best->ja_task_id)); 

   if (!now_assignment) {
      /* reservation scheduling */
      for_each(pe, pe_list) {

         if (!pe_is_matching(pe, pe_request))
            continue;

         matched_pe_count++;

         if (!best->gdil) {
            best->pe = pe;

            /* determine earliest start time with that PE */
            result = sge_select_pe_time(best);
            if (result != 0) {
               best_result = sge_best_result(best_result, result);
               continue;
            }
         } else {
            sge_assignment_t tmp;
            assignment_copy(&tmp, best, false);
            tmp.pe = pe;

            /* try to find earlier assignment again with minimum slot amount */
            tmp.slots = 0;
            result = sge_select_pe_time(&tmp); 
            if (result != 0) {
               best_result = sge_best_result(best_result, result);
               continue;
            }

            if (tmp.start < best->start) {
               assignment_copy(best, &tmp, true);
            }
         }
      }
   } else {
      /* now assignments */ 
      for_each(pe, pe_list) {

         if (!pe_is_matching(pe, pe_request))
            continue;

         matched_pe_count++;

         if (!best->gdil) {
            best->pe = pe;
            result = sge_maximize_slots(best);
            if (result != 0) {
               best_result = sge_best_result(best_result, result);
               continue;
            }
         } else {
            sge_assignment_t tmp;
            assignment_copy(&tmp, best, false);
            tmp.pe = pe;

            result = sge_maximize_slots(&tmp);
            if (result != 0) {
               best_result = sge_best_result(best_result, result);
               continue;
            }

            if (tmp.slots > best->slots) {
               assignment_copy(best, &tmp, true);
            }
         }
      }
   }

   if (matched_pe_count == 0) {
      schedd_mes_add(best->job_id, SCHEDD_INFO_NOPEMATCH_ ); 
      best_result = -1;
   } else if (!now_assignment && best->gdil) {
      result = sge_maximize_slots(best);
      if (result != 0) { /* ... should never happen */
         best_result = -1;
      }
   }

   if (best->gdil)
      best_result = 0;

   switch (best_result) {
   case 0:
      DPRINTF(("SELECT PE("u32"."u32") returns PE %s %d slots at "u32")\n", 
            best->job_id, best->ja_task_id, lGetString(best->pe, PE_name), 
            best->slots, best->start));
      break;
   case 1:
      DPRINTF(("SELECT PE("u32"."u32") returns <later>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   case -1:
      DPRINTF(("SELECT PE("u32"."u32") returns <category_never>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   case -2:
      DPRINTF(("SELECT PE("u32"."u32") returns <job_never>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   default:
      DPRINTF(("!!!!!!!! SELECT PE("u32"."u32") returns unexpected %d\n", 
            best->job_id, best->ja_task_id, best_result));
      break;
   }

   DEXIT;
   return best_result;
}

/****** scheduler/sge_select_pe_time() *****************************************
*  NAME
*     sge_select_pe_time() -- Search earliest possible assignment 
*
*  SYNOPSIS
*     static int sge_select_pe_time(sge_assignment_t *best, lList *host_list, 
*     lList *queue_list, lList *centry_list, lList *acl_list) 
*
*  FUNCTION
*     The earliest possible assignment is searched for a job assuming a 
*     particular parallel environment be used with a particular slot
*     number. If the slot number passed is 0 we start with the minimum 
*     possible slot number for that job. The search starts with the 
*     latest queue end time if DISPATCH_TIME_QUEUE_END was specified 
*     rather than a real time value.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*     lList *host_list       - ??? 
*     lList *queue_list      - ??? 
*     lList *centry_list     - ??? 
*     lList *acl_list        - ??? 
*
*  RESULT
*     int - 0 ok got an assignment
*           1 no assignment at the specified time (???)
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_select_pe_time() is not MT safe 
*******************************************************************************/
static int sge_select_pe_time(sge_assignment_t *best) 
{
   u_long32 pe_time, first_time;
   sge_assignment_t tmp_assignment;
   int result = -1; 
   sge_qeti_t *qeti; 

   DENTER(TOP_LAYER, "sge_select_pe_time");

   assignment_copy(&tmp_assignment, best, false);
   if (best->slots == 0)
      tmp_assignment.slots = range_list_get_first_id(lGetList(best->job, JB_pe_range), NULL);

   qeti = sge_qeti_allocate(best->job, best->pe, best->ckpt, 
         best->host_list, best->queue_list, best->centry_list, best->acl_list); 
   if (!qeti) {
      ERROR((SGE_EVENT, "could not allocate qeti object needed reservation "
            "scheduling of parallel job "U32CFormat"\n", best->job_id));
      DEXIT;
      return -1;
   }

   if (best->start == DISPATCH_TIME_QUEUE_END) 
      first_time = sge_qeti_first(qeti);
   else {
      /* the first iteration will be done using best->start 
         further ones will use earliert times */
      first_time = best->start;
      sge_qeti_next_before(qeti, best->start);
   }

   for (pe_time = first_time ; pe_time; pe_time = sge_qeti_next(qeti)) {
      DPRINTF(("SELECT PE TIME(%s, "u32") tries at "u32"\n", 
         lGetString(best->pe, PE_name), best->job_id, pe_time));
      tmp_assignment.start = pe_time;
      result = sge_parallel_assignment(&tmp_assignment);
      if (result == 0) {
         assignment_copy(best, &tmp_assignment, true);
         break;
      }
   }

   sge_qeti_release(qeti);

   if (best->gdil)
      result = 0;
  
   switch (result) {
   case 0:
      DPRINTF(("SELECT PE TIME(%s, %d) returns "u32"\n", 
            lGetString(best->pe, PE_name), best->slots, best->start));
      break;
   case -1:
      DPRINTF(("SELECT PE TIME(%s, %d) returns <category_never>\n", 
            lGetString(best->pe, PE_name), best->slots));
      break;
   case -2:
      DPRINTF(("SELECT PE TIME(%s, %d) returns <job_never>\n", 
            lGetString(best->pe, PE_name), best->slots));
      break;
   default:
      DPRINTF(("!!!!!!!! SELECT PE TIME(%s, %d) returns unexpected %d\n", 
            lGetString(best->pe, PE_name), best->slots, result));
      break;
   }

   DEXIT;
   return result;
}

/****** scheduler/sge_maximize_slots() *****************************************
*  NAME
*     sge_maximize_slots() -- Maximize number of slots for an assignment
*
*  SYNOPSIS
*     static int sge_maximize_slots(sge_assignment_t *best, lList *host_list, 
*     lList *queue_list, lList *centry_list, lList *acl_list) 
*
*  FUNCTION
*     The largest possible slot amount is searched for a job assuming a 
*     particular parallel environment be used at a particular start time. 
*     If the slot number passed is 0 we start with the minimum 
*     possible slot number for that job.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*     lList *host_list       - ??? 
*     lList *queue_list      - ??? 
*     lList *centry_list     - ??? 
*     lList *acl_list        - ??? 
*
*  RESULT
*     int - 0 ok got an assignment (maybe without maximizing it) 
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_maximize_slots() is not MT safe 
*******************************************************************************/
static int sge_maximize_slots(sge_assignment_t *best) {

   int slots, min_slots, max_slots; 
   int max_pe_slots;
   int first, last;
   lList *pe_range;
   lListElem *pe;
   sge_assignment_t tmp;
   int result = -1; 
   const char *pe_name = lGetString(best->pe, PE_name);
  
   DENTER(TOP_LAYER, "sge_maximize_slots");

   if (!best || !(pe_range=lGetList(best->job, JB_pe_range))
      || !(pe=best->pe)) {
      DEXIT; 
      return -1;
   }
   
   first = range_list_get_first_id(pe_range, NULL);
   last  = range_list_get_last_id(pe_range, NULL);
   max_pe_slots = lGetUlong(pe, PE_slots);

   if (best->slots)
      min_slots = best->slots;
   else
      min_slots = first;

   if (best->gdil || best->slots == max_pe_slots) { /* already found maximum */
      DEXIT; 
      return 0; 
   }

   DPRINTF(("MAXIMIZE SLOT: FIRST %d LAST %d MAX SLOT %d\n", first, last, max_pe_slots));

   /* this limits max range number RANGE_INFINITY (i.e. -pe pe 1-) to reasonable number */
   max_slots = MIN(last, max_pe_slots);

   DPRINTF(("MAXIMIZE SLOT FOR "u32" using \"%s\" FROM %d TO %d\n", 
      best->job_id, pe_name, min_slots, max_slots));

   assignment_copy(&tmp, best, false);

   for (slots = min_slots; slots <= max_slots; slots++) {

      /* sort out slot numbers that would conflict with allocation rule */
      if (sge_pe_slots_per_host(pe, slots)==0)
         continue; 

      /* only slot numbers from jobs PE range request */
      if (!range_list_is_id_within(pe_range, slots))
         continue;

      /* we try that slot amount */
      tmp.slots = slots;
      result = sge_parallel_assignment(&tmp);
      if (result != 0)
         break;

      assignment_copy(best, &tmp, true);
   }

   if (best->gdil)
      result = 0;

   switch (result) {
   case 0:
      DPRINTF(("MAXIMIZE SLOT(%s, %d) returns "u32"\n", 
            pe_name, best->slots, best->start));
      break;
   case 1:
      DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <later>\n", 
            pe_name, best->slots));
      break;
   case -1:
      DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <category_never>\n", 
            pe_name, best->slots));
      break;
   case -2:
      DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <job_never>\n", 
            pe_name, best->slots));
      break;
   default:
      DPRINTF(("!!!!!!!! MAXIMIZE SLOT(%d, %d) returns unexpected %d\n", 
            result));
      break;
   }

   DEXIT;
   return result;
}


/****** scheduler/job_get_duration() *******************************************
*  NAME
*     job_get_duration() -- Determine a jobs runtime duration
*
*  SYNOPSIS
*     static bool job_get_duration(u_long32 *duration, const lListElem *jep) 
*
*  FUNCTION
*     The minimum of the time values the user specified with -l h_rt=<time> 
*     and -l s_rt=<time> is returned in 'duration'. If neither of these 
*     time values were specified no 'duration' is retured and result is 
*     'false' otherwise 'true'.
*
*  INPUTS
*     u_long32 *duration   - Returns duration on success
*     const lListElem *jep - The job (JB_Type)
*
*  RESULT
*     static bool - true on success
*
*  NOTES
*     MT-NOTE: job_get_duration() is MT safe 
*******************************************************************************/
static bool job_get_duration(u_long32 *duration, const lListElem *jep)
{
   lListElem *ep;
   double h_rt = DBL_MAX, s_rt = DBL_MAX;
   bool got_h_rt = false, got_s_rt = false;
   char error_str[1024];
   const char *s;

   DENTER(TOP_LAYER, "job_get_duration");

   if ((ep=lGetElemStr(lGetList(jep, JB_hard_resource_list), CE_name, SGE_ATTR_H_RT))) {
      if (parse_ulong_val(&h_rt, NULL, TYPE_TIM, (s=lGetString(ep, CE_stringval)),
               error_str, sizeof(error_str)-1)==0) {
         ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
         DEXIT;
         return false;
      }
      got_h_rt = true;
   }
   if ((ep=lGetElemStr(lGetList(jep, JB_hard_resource_list), CE_name, SGE_ATTR_S_RT))) {
      if (parse_ulong_val(&s_rt, NULL, TYPE_TIM, (s=lGetString(ep, CE_stringval)),
               error_str, sizeof(error_str)-1)==0) {
         ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
         DEXIT;
         return false;
      }
      got_s_rt = true;
   }
   if (got_h_rt || got_s_rt) {
      *duration = MIN(h_rt, s_rt);
      DEXIT;
      return true;
   } 

   *duration = sconf_get_default_duration();

   DEXIT;
   return true;
}

/****** scheduler/determine_default_duration() *********************************
*  NAME
*     determine_default_duration() -- Determine jobs default duration
*
*  SYNOPSIS
*     static u_long32 determine_default_duration(const lList *centry_list) 
*
*  FUNCTION
*     The default duration is needed for those jobs that do not have a
*     -l h_rt=<time> and -l s_rt=<time> specification. Unfortunately 
*     the {h,s}_cpu can't be used as this is net time while we need 
*     gross time.
*
*  INPUTS
*     const lList *centry_list - ??? 
*
*  RESULT
*     static u_long32 - The default time.
*
*  NOTES
*     MT-NOTE: determine_default_duration() is MT safe 
*******************************************************************************/
static u_long32 determine_default_duration(const lList *centry_list)
{
   double h_rt, s_rt;
   lListElem *cep;
   char error_str[1024];
   const char *s;

   DENTER(TOP_LAYER, "determine_default_duration");

   if (!(cep = centry_list_locate(centry_list, SGE_ATTR_H_RT))) {
      ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S, SGE_ATTR_H_RT));
      DEXIT;
      return 0;
   }
   if (!(parse_ulong_val(&h_rt, NULL, TYPE_TIM, (s=lGetString(cep, CE_default)),
            error_str, sizeof(error_str)-1))) {
      ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
      DEXIT;
      return 0;
   }
 
   if (!(cep = centry_list_locate(centry_list, SGE_ATTR_S_RT))) {
      ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S, SGE_ATTR_S_RT));
      DEXIT;
      return 0;
   }
   if (!(parse_ulong_val(&s_rt, NULL, TYPE_TIM, (s=lGetString(cep, CE_default)),
            error_str, sizeof(error_str)-1))) {
      ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_S_RT, s, error_str));
      DEXIT;
      return 0;
   }

   if (h_rt >= MAX_ULONG32 || s_rt >= MAX_ULONG32) {
      DEXIT;
      return MAX_ULONG32;
   } 
   
   s_rt = MAX(h_rt, s_rt);
   DPRINTF(("determine_default_duration() = %f\n", s_rt));

   DEXIT;
   return s_rt;
}


static int add_job_list_to_schedule(const lList *job_list, bool suspended, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *centry_list)
{
   lListElem *jep, *ja_task;
   const char *pe_name;
   const char *type;

   DENTER(TOP_LAYER, "add_job_list_to_schedule");

   if (suspended)
      type = SCHEDULING_RECORD_ENTRY_TYPE_SUSPENDED;
   else
      type = SCHEDULING_RECORD_ENTRY_TYPE_RUNNING;

   for_each (jep, job_list) {
      for_each (ja_task, lGetList(jep, JB_ja_tasks)) {  
         sge_assignment_t a;

         assignment_init(&a, jep, ja_task);

         a.start = lGetUlong(ja_task, JAT_start_time);
         if (!job_get_duration(&a.duration, jep))
            a.duration = sconf_get_default_duration();

         a.gdil = lGetList(ja_task, JAT_granted_destin_identifier_list);
         a.slots = nslots_granted(a.gdil, NULL);
         if ((pe_name = lGetString(ja_task, JAT_granted_pe)) && 
             !(a.pe = pe_list_locate(pe_list, pe_name))) {
            ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDPE_S, pe_name));
            continue;
         }
         /* no need (so far) for passing ckpt information to debit_scheduled_job() */

         a.host_list = host_list;
         a.queue_list = queue_list;
         a.centry_list = centry_list;

         DPRINTF(("Adding job "U32CFormat"."U32CFormat" into schedule "
               "from "U32CFormat" until "U32CFormat"\n",  
                  lGetUlong(jep, JB_job_number), 
                  lGetUlong(ja_task, JAT_task_number), 
                  a.start, a.start + a.duration));

         /* only update resource utilization schedule  
            RUE_utililized_now is already set through events */
         debit_scheduled_job(&a, NULL, NULL, false, type);
      }
   }

   DEXIT;
   return 0;
}

/****** scheduler/prepare_resource_schedules() *********************************
*  NAME
*     prepare_resource_schedules() -- Debit non-pending jobs in resource schedule
*
*  SYNOPSIS
*     static void prepare_resource_schedules(const lList *running_jobs, const 
*     lList *suspended_jobs, lList *pe_list, lList *host_list, lList 
*     *queue_list, const lList *centry_list) 
*
*  FUNCTION
*     In order to reflect current and future resource utilization of running 
*     and suspended jobs in the schedule we iterate through all jobs and debit
*     resources requested by those jobs.
*
*  INPUTS
*     const lList *running_jobs   - The running ones (JB_Type)
*     const lList *suspended_jobs - The susepnded ones (JB_Type)
*     lList *pe_list              - ??? 
*     lList *host_list            - ??? 
*     lList *queue_list           - ??? 
*     const lList *centry_list    - ??? 
*
*  NOTES
*     MT-NOTE: prepare_resource_schedules() is not MT safe 
*******************************************************************************/
static void prepare_resource_schedules(const lList *running_jobs, const lList *suspended_jobs, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *centry_list)
{
   DENTER(TOP_LAYER, "prepare_resource_schedules");

   add_job_list_to_schedule(running_jobs, false, 
         pe_list, host_list, queue_list, centry_list);
   add_job_list_to_schedule(suspended_jobs, true, 
         pe_list, host_list, queue_list, centry_list);

   utilization_print_all(pe_list, host_list, queue_list);

   DEXIT;
   return;
}
