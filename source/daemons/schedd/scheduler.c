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

/* profiling info */
extern int scheduled_fast_jobs;
extern int scheduled_complex_jobs;
extern int do_profiling;

/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, 
                         lList **splitted_job_list[]);

static int select_assign_debit(lList **queue_list, lList **job_list, lListElem *job, lListElem *ja_task, lListElem *pe, lListElem *ckpt, lList *complex_list, lList *host_list, lList *acl_list, lList **user_list, lList **group_list, lList **orders_list, double *total_running_job_tickets, int ndispatched, int *dispatch_type, int host_order_changed, int *sort_hostlist);


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
   lList *pending_excludedlist = NULL;                     /* JB_Type */
   lList *running_list = NULL;                     /* JB_Type */
   lList *error_list = NULL;                       /* JB_Type */
   lList *hold_list = NULL;                        /* JB_Type */

   int ret;
   int i;
#ifdef TEST_DEMO
   FILE *fpdjp;
#endif

   DENTER(TOP_LAYER, "scheduler");
#ifdef TEST_DEMO
   fpdjp = fopen("/tmp/sge_debug_job_place.out", "a");
#endif

   PROFILING_START_MEASUREMENT;

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
 
#if 0 /* EB: debug */
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
         "|| %I m= %u)",
         dp,
         QU_state, QSUSPENDED,        /* only not suspended queues      */
         QU_state, QSUSPENDED_ON_SUBORDINATE,
         QU_state, QCAL_SUSPENDED,
         QU_state, QERROR,            /* no queues in error state       */
         QU_state, QUNKNOWN);         /* only known queues              */

      if (!what || !where) {
         DPRINTF(("failed creating where or what describing non available queues\n")); 
      }
      qlp = lSelect("", lists->all_queue_list, where, what);

      for_each(mes_queues, qlp)
         schedd_mes_add_global(SCHEDD_INFO_QUEUENOTAVAIL_, 
                                   lGetString(mes_queues, QU_qname));

      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED, 
                      qlp, QU_qname);
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
   if(profiling_started) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      PROFILING_STOP_MEASUREMENT;

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "scheduled in %.3f (u %.3f + s %.3f = %.3f): %d fast, %d complex, %d orders, %d H, %d Q, %d QA, %d J(qw), %d J(r), %d J(x), %d C, %d ACL, %d PE, %d CONF, %d U, %d D, %d PRJ, %d ST, %d CKPT, %d RU\n",
         profiling_get_measurement_wallclock(),
         profiling_get_measurement_utime(),
         profiling_get_measurement_stime(),
         profiling_get_measurement_utime() + profiling_get_measurement_stime(),
         scheduled_fast_jobs,
         scheduled_complex_jobs,
         lGetNumberOfElem(orderlist), 
         lGetNumberOfElem(lists->host_list), 
         lGetNumberOfElem(lists->queue_list),
         lGetNumberOfElem(lists->all_queue_list),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_RUNNING])),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_FINISHED])),
         lGetNumberOfElem(lists->complex_list),
         lGetNumberOfElem(lists->acl_list),
         lGetNumberOfElem(lists->pe_list),
         lGetNumberOfElem(lists->config_list),
         lGetNumberOfElem(lists->user_list),
         lGetNumberOfElem(lists->dept_list),
         lGetNumberOfElem(lists->project_list),
         lGetNumberOfElem(lists->share_tree),
         lGetNumberOfElem(lists->ckpt_list),
         lGetNumberOfElem(lists->running_per_user)
      ));
      log_state_set_log_level(saved_logginglevel);
   }

   remove_immediate_jobs(*(splitted_job_lists[SPLIT_PENDING]), &orderlist);
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
   lListElem *rjob, *rja_task;
   lListElem *queue;  
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
   struct tms tms_buffer;
   clock_t start;

   DENTER(TOP_LAYER, "dispatch_jobs");

   /*---------------------------------------------------------------------
    * LOAD ADJUSTMENT
    *
    * Load sensors report load delayed. So we have to increase the load
    * of an exechost for each job started before load adjustment decay time.
    * load_adjustment_decay_time is a configuration value. 
    *---------------------------------------------------------------------*/

   if (scheddconf.load_adjustment_decay_time ) {
      correct_load(*(splitted_job_lists[SPLIT_RUNNING]),
		   lists->queue_list,
		   lists->host_list,
		   scheddconf.load_adjustment_decay_time);

      /* is there a "global" load value in the job_load_adjustments?
         if so this will make it necessary to check load thresholds
         of each queue after each dispatched job */
      {
         lListElem *gep, *lcep;
         if ((gep = host_list_locate(lists->host_list, "global"))) {
            for_each (lcep, scheddconf.job_load_adjustments) {
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

   set_global_load_correction(global_lc);

   /*---------------------------------------------------------------------
    * CAPACITY CORRECTION
    *---------------------------------------------------------------------*/
   correct_capacities(lists->host_list, lists->complex_list);

   /*---------------------------------------------------------------------
    * KEEP SUSPEND THRESHOLD QUEUES
    *---------------------------------------------------------------------*/
   if (sge_split_queue_load(
         &(lists->queue_list),    /* queue list                             */
         &susp_queues,            /* list of queues in suspend alarm state  */
         lists->host_list,
         lists->complex_list,
         scheddconf.job_load_adjustments,
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
         lists->complex_list,  /* complex list is needed to use load values */
         scheddconf.job_load_adjustments,
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

   queue = lFirst(lists->queue_list);
   if(queue == NULL) {
      if (sgeee_mode) {
         sge_scheduler(lists, 
                       *(splitted_job_lists[SPLIT_RUNNING]), 
                       *(splitted_job_lists[SPLIT_FINISHED]), 
                       *(splitted_job_lists[SPLIT_PENDING]),
                       orderlist);
      }

      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_mes_add_global(SCHEDD_INFO_ALLALARMOVERLOADED_);

      DEXIT;
      return 0;
   }

   user_list_init_jc(&user_list, *(splitted_job_lists[SPLIT_RUNNING]));
   job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                                 &user_list,
                                                 NULL,
                                                 scheddconf.maxujobs);

   trash_splitted_jobs(splitted_job_lists);

   /*--------------------------------------------------------------------
    * CALL SGEEE SCHEDULER TO
    * CALCULATE TICKETS FOR EACH JOB  - IN SUPPORT OF SGEEE
    *------------------------------------------------------------------*/

   if (sgeee_mode) {
      start = times(&tms_buffer);

      sge_scheduler(lists, 
                    *(splitted_job_lists[SPLIT_RUNNING]),
                    *(splitted_job_lists[SPLIT_FINISHED]),
                    *(splitted_job_lists[SPLIT_PENDING]),
                    orderlist);     

      if (do_profiling) {
         clock_t now = times(&tms_buffer);
         u_long32 saved_logginglevel = log_state_get_log_level();

         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, "SGEEE pending job ticket calculation took %.3f s\n",
               (now - start) * 1.0 / CLK_TCK ));
         log_state_set_log_level(saved_logginglevel);
      }
   }

   DPRINTF(("STARTING PASS 2 WITH %d PENDING JOBS\n",
            lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]))));

   if (lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING])) == 0) {
      /* no jobs to schedule */
      SCHED_MON((log_string, MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON ));
      lFreeList(user_list);
      lFreeList(group_list);
      DEXIT;
      return 0;
   }

   if (sgeee_mode) {

      start = times(&tms_buffer);

      /* 
       * calculate the number of tickets for all jobs already 
       * running on the host 
       */
      if (calculate_host_tickets(splitted_job_lists[SPLIT_RUNNING], 
                                 &(lists->host_list)))  {
         DPRINTF(("no host for which to calculate host tickets\n"));
         lFreeList(user_list);
         lFreeList(group_list);
         DEXIT;
         return -1;
      }

      if (do_profiling) {
         clock_t now = times(&tms_buffer);
         u_long32 saved_logginglevel = log_state_get_log_level();

         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, "SGEEE active job ticket calculation took %.3f s\n",
               (now - start) * 1.0 / CLK_TCK ));
         log_state_set_log_level(saved_logginglevel);
      }

      /* 
       * temporary job placement workaround - 
       * calc tickets for running and queued jobs 
       */
      if (classic_sgeee_scheduling) {
         int seqno;

         seqno = sge_calc_tickets(lists, 
                                  *(splitted_job_lists[SPLIT_RUNNING]), 
                                  NULL, 
                                  *(splitted_job_lists[SPLIT_PENDING]),
                                  0);

         *orderlist = sge_build_sge_orders(lists, NULL,
                                           *(splitted_job_lists[SPLIT_PENDING]), NULL,
                                           *orderlist, 0, seqno);
      }

      /* 
       * Order Jobs in descending order according to tickets and 
       * then job number 
       */
      start = times(&tms_buffer);

      sgeee_sort_jobs(splitted_job_lists[SPLIT_PENDING]);

      if (do_profiling) {
         clock_t now = times(&tms_buffer);
         u_long32 saved_logginglevel = log_state_get_log_level();

         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, "SGEEE job sorting took %.3f s\n",
               (now - start) * 1.0 / CLK_TCK ));
         log_state_set_log_level(saved_logginglevel);
      }

   }  /* if sgeee_mode */

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
   switch (scheddconf.queue_sort_method) {
   case QSM_SHARE:   

      if (sgeee_mode) {
         sort_host_list(lists->host_list, lists->complex_list);
         sort_host_list_by_share_load(lists->host_list, lists->complex_list);

#ifdef TEST_DEMO

         for_each(host, lists->host_list) {
            fprintf(fpdjp, "after sort by share load - for running jobs only\n");
            fprintf(fpdjp, "For host %s :\n", lGetHost(host, EH_name));
            fprintf(fpdjp, "EH_sge_tickets is %u\n", (u_long32)lGetDouble(host, EH_sge_tickets));
            fprintf(fpdjp, "EH_sge_ticket_pct is %f\n", lGetDouble(host, EH_sge_ticket_pct));
            fprintf(fpdjp, "EH_resource_capability_factor is %f\n", lGetDouble(host, EH_resource_capability_factor));
            fprintf(fpdjp, "EH_resource_capability_factor_pct is %f\n", lGetDouble(host, EH_resource_capability_factor_pct));
            fprintf(fpdjp, "EH_sort_value  is %f\n", lGetDouble(host, EH_sort_value));
            fprintf(fpdjp, "EH_sge_load_pct  is %f\n", lGetDouble(host, EH_sge_load_pct));
            fprintf(fpdjp, "share load value for host %s is %d\n", lGetHost(host, EH_name), lGetUlong(host, EH_sge_load));
       }
#endif
      }

      break;
      
   case QSM_LOAD:
   case QSM_SEQNUM:   
   default:

      DPRINTF(("sorting hosts by load\n"));
      sort_host_list(lists->host_list, lists->complex_list);

#ifdef TEST_DEMO
      fprintf(fpdjp, "after sort by  load");

      for_each(host, lists->host_list) {
         fprintf(fpdjp, "load value for host %s is %f\n",
                 lGetHost(host, EH_name), lGetDouble(host, EH_sort_value));

         if (sgeee_mode)  {
            fprintf(fpdjp, "EH_sge_tickets is %u\n", (u_long32)lGetDouble(host, EH_sge_tickets));
         }
       }

#endif

      break;
   }

   /*---------------------------------------------------------------------
    * DISPATCH JOBS TO QUEUES
    *---------------------------------------------------------------------*/

   /* 
    * Calculate the total number of tickets for running jobs - to be used
    * later for SGE job placement 
    */
   if (sgeee_mode && scheddconf.queue_sort_method == QSM_SHARE)  {
      total_running_job_tickets = 0;
      for_each(rjob, *splitted_job_lists[SPLIT_RUNNING]) {
         for_each(rja_task, lGetList(rjob, JB_ja_tasks)) {
            total_running_job_tickets += lGetDouble(rja_task, JAT_ticket);
         }
      }
   }

   /*
    * job categories are reset here, we need an update of the rejected field
    * for every new run
    */
   sge_reset_job_category(); 

   if (!sgeee_mode) {
      /* 
       * establish the access tree for all job arrays containing runnable tasks 
       */
      at_notice_runnable_job_arrays(*splitted_job_lists[SPLIT_PENDING]);
   }

   start = times(&tms_buffer);

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
                  lists->complex_list, 
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
               lists->complex_list, 
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
                                             scheddconf.maxujobs);
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
       if (scheddconf.queue_sort_method == QSM_SEQNUM)  {
            fprintf(fpdjp, "using sort_by_seq_no method - after sge_debit_job:\n");
            fprintf(fpdjp, " \n");
            fprintf(fpdjp, "QUEUE ORDER:\n");
            for_each(queue, lists->queue_list)  {
               fprintf(fpdjp, "  queue %s\n", lGetString(queue, QU_qname));
            }
       }
       if (scheddconf.queue_sort_method == QSM_LOAD)  {
            fprintf(fpdjp, "using sort_by_load method - after sge_debit_job:\n");
            for_each(host, lists->host_list) {
               fprintf(fpdjp, "load value for host %s is %f\n", lGetHost(host, EH_name), lGetDouble(host, EH_sort_value));
            }
            fprintf(fpdjp, " \n");
            fprintf(fpdjp, "QUEUE ORDER:\n");
            for_each(queue, lists->queue_list)  {
               fprintf(fpdjp, "  queue %s\n", lGetString(queue, QU_qname));
            }
        }
#endif

      if (dispatched_a_job && !(lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]))==0))  {
         if (sgeee_mode && scheddconf.queue_sort_method == QSM_SHARE)  {
            sort_host_list_by_share_load(lists->host_list, lists->complex_list);

#ifdef TEST_DEMO
            for_each(host, lists->host_list) {
               fprintf(fpdjp, "after sort by share load - for running and already placed new jobs \n");
               fprintf(fpdjp, "For host %s :\n", lGetHost(host, EH_name));
               fprintf(fpdjp, "EH_sge_tickets is %f\n", lGetDouble(host, EH_sge_tickets));
               fprintf(fpdjp, "EH_sge_ticket_pct is %f\n", lGetDouble(host, EH_sge_ticket_pct));
               fprintf(fpdjp, "EH_resource_capability_factor is %f\n", 
                              lGetDouble(host, EH_resource_capability_factor));
               fprintf(fpdjp, "EH_resource_capability_factor_pct is %f\n", 
                              lGetDouble(host, EH_resource_capability_factor_pct));
               fprintf(fpdjp, "EH_sort_value  is %f\n", lGetDouble(host, EH_sort_value));
               fprintf(fpdjp, "EH_sge_load_pct  is %f\n", lGetDouble(host, EH_sge_load_pct));
               fprintf(fpdjp, "share load value for host %s is %d\n", 
                              lGetHost(host, EH_name), lGetUlong(host, EH_sge_load));
            }
#endif

         } /* sgeee_mode && scheddconf.queue_sort_method == QSM_SHARE */
      }
   } /* end of while */

   if (sgeee_mode && do_profiling) {
      clock_t now = times(&tms_buffer);
      u_long32 saved_logginglevel = log_state_get_log_level();

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "SGEEE job dispatching took %.3f s\n",
            (now - start) * 1.0 / CLK_TCK ));
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
lList *complex_list,
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
   lListElem *hep;  
   double old_host_tickets;    
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
      scheddconf.queue_sort_method ,/* sort order to use                       */
      complex_list,         /* to interpret job requests               */
      host_list,            /* for load/architecture                   */
      acl_list,             /* use these access lists                  */
      scheddconf.job_load_adjustments,
      ndispatched,
      dispatch_type,
      host_order_changed);

   /* the following was added to support SGE - 
      account for job tickets on hosts due to parallel jobs */
   if (granted && sgeee_mode) {
      double job_tickets_per_slot, nslots;
      nslots = nslots_granted(granted, NULL);

      job_tickets_per_slot =(double)lGetDouble(ja_task, JAT_ticket)/nslots;

      for_each(granted_el, granted) {
         hep = host_list_locate(host_list, lGetHost(granted_el, JG_qhostname));
         old_host_tickets = lGetDouble(hep, EH_sge_tickets);
         lSetDouble(hep, EH_sge_tickets, (old_host_tickets + 
               job_tickets_per_slot*lGetUlong(granted_el, JG_slots)));
         lSetDouble(granted_el, JG_ticket, job_tickets_per_slot*lGetUlong(granted_el, JG_slots));
      }
      *total_running_job_tickets += lGetDouble(ja_task, JAT_ticket);
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
         job, ja_task, granted);
   
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
         complex_list, sort_hostlist, *orders_list);

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
         complex_list,  /* complex list is neede to use load values */
         scheddconf.job_load_adjustments,
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

