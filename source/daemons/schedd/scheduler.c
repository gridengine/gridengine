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
#include "sge_category.h"
#include "msg_schedd.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_schedd_text.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_centry.h"
#include "sge_ckpt.h"
#include "sge_host.h"
#include "sge_schedd_conf.h"
#include "sge_resource_utilization.h"
#include "sge_serf.h"
#include "sge_qinstance_state.h"

/* profiling info */
extern int scheduled_fast_jobs;
extern int scheduled_complex_jobs;

/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int dispatch_jobs(sge_Sdescr_t *lists, order_t *orders, lList **splitted_job_list[]);

static int select_assign_debit(lList **queue_list, lList **job_list, lListElem *job, lListElem *ja_task, 
                               lList *pe_list, lList *ckpt_list, lList *centry_list, lList *host_list, 
                               lList *acl_list, lList **user_list, lList **group_list, order_t *orders, 
                               double *total_running_job_tickets, int *sort_hostlist, bool dont_start, 
                               bool dont_reserve, lList **load_lis);

static bool job_get_duration(u_long32 *duration, const lListElem *jep);
static void prepare_resource_schedules(const lList *running_jobs, const lList *suspended_jobs, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *centry_list);



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
   order_t orders = ORDER_INIT;
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
   lList *not_started_list = NULL; /* JB_Type */
   int prof_job_count;

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
   splitted_job_lists[SPLIT_NOT_STARTED] = &not_started_list;

   split_jobs(&(lists->job_list), NULL, lists->all_queue_list, 
              conf.max_aj_instances, splitted_job_lists);
 
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

   dispatch_jobs(lists, &orders, splitted_job_lists);

   remove_immediate_jobs(*(splitted_job_lists[SPLIT_PENDING]), *(splitted_job_lists[SPLIT_RUNNING]), &(orders.jobStartOrderList));

   /* send job_start_orders */
   sge_send_job_start_orders(&orders);

   PROF_START_MEASUREMENT(SGE_PROF_SCHEDLIB4);
   {
      int clean_jobs[] = {SPLIT_WAITING_DUE_TO_PREDECESSOR,
                          SPLIT_WAITING_DUE_TO_TIME,
                          SPLIT_PENDING_EXCLUDED,
                          SPLIT_PENDING_EXCLUDED_INSTANCES,
                          SPLIT_ERROR,
                          SPLIT_HOLD};
      int i = 0;
      int max = 6;
      lListElem *job;

      for (i = 0; i < max; i++) {
         /* clear SGEEE fields for queued jobs */
         for_each(job, *(splitted_job_lists[clean_jobs[i]])) {
            sge_clear_job(job, true);         
         }
         
         orders.pendingOrderList = sge_build_sgeee_orders(lists, NULL,
                                                   *(splitted_job_lists[clean_jobs[i]]), NULL, 
                                                    orders.pendingOrderList, false, 0,false);    
      }
                          
   }
   
   orders.pendingOrderList = sge_build_sgeee_orders(lists, NULL,*(splitted_job_lists[SPLIT_PENDING]), NULL, 
                                                    orders.pendingOrderList, false, 0, false); 
   
   orders.pendingOrderList = sge_build_sgeee_orders(lists, NULL,*(splitted_job_lists[SPLIT_NOT_STARTED]), NULL, 
                                                    orders.pendingOrderList, false, 0, false); 
  
   /* generated scheduler messages, thus we have to call it */
   trash_splitted_jobs(splitted_job_lists); 

   orders.jobStartOrderList= sge_add_schedd_info(orders.jobStartOrderList);

   PROF_STOP_MEASUREMENT(SGE_PROF_SCHEDLIB4);
   if (prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();
      log_state_set_log_level(LOG_INFO); 

      INFO((SGE_EVENT, "PROF: create pending job orders: %.3f s\n",
               prof_get_measurement_utime(SGE_PROF_SCHEDLIB4,false, NULL)));

      log_state_set_log_level(saved_logginglevel);
   }   

   if(prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM0); 

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "PROF: scheduled in %.3f (u %.3f + s %.3f = %.3f): %d sequential, %d parallel, %d orders, %d H, %d Q, %d QA, %d J(qw), %d J(r), %d J(s), %d J(h), %d J(e),  %d J(x),%d J(all),  %d C, %d ACL, %d PE, %d U, %d D, %d PRJ, %d ST, %d CKPT, %d RU\n",
         prof_get_measurement_wallclock(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL) + 
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         scheduled_fast_jobs,
         scheduled_complex_jobs,
         sge_GetNumberOfOrders(&orders), 
         lGetNumberOfElem(lists->host_list), 
         lGetNumberOfElem(lists->queue_list),
         lGetNumberOfElem(lists->all_queue_list),
         lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]) + lGetNumberOfElem(*(splitted_job_lists[SPLIT_NOT_STARTED])) ),
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
   
   /* free all job lists */
   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      if (splitted_job_lists[i]) {
         *(splitted_job_lists[i]) = lFreeList(*(splitted_job_lists[i]));
         splitted_job_lists[i] = NULL;
      }
   }
   
   {
      lList *orderlist=sge_join_orders(&orders);
     
      if (orderlist) {
         sge_send_orders2master(orderlist);
         orderlist = lFreeList(orderlist);
      }
   }

   schedd_mes_release();
   schedd_mes_set_logging(0); 

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM5);

   if(prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();
      log_state_set_log_level(LOG_INFO);
   
      INFO((SGE_EVENT, "PROF: send orders and cleanup took: %.3f (u %.3f,s %.3f) s\n",
         prof_get_measurement_wallclock(SGE_PROF_CUSTOM5, true, NULL), 
         prof_get_measurement_utime(SGE_PROF_CUSTOM5, true, NULL),
         prof_get_measurement_stime(SGE_PROF_CUSTOM5, true, NULL) ));

      log_state_set_log_level(saved_logginglevel);
   }
   
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
static int dispatch_jobs(sge_Sdescr_t *lists, order_t *orders,
                         lList **splitted_job_lists[]) 
{
   lList *user_list=NULL, *group_list=NULL;
   lListElem *orig_job, *job, *cat=NULL;
   lList *non_avail_queues = NULL;
   lList *consumable_load_list = NULL;

   bool queues_available;  
   u_long32 queue_sort_method; 
   u_long32 maxujobs;
   const lList *job_load_adjustments = NULL;
   double total_running_job_tickets=0; 
   int nreservation = 0;

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
   
   /* we will assume this time as start time for now assignments */
   sconf_set_now(sge_get_gmt());

   if (max_reserve != 0) {
      /*----------------------------------------------------------------------
       * ENSURE RUNNING JOBS ARE REFLECTED IN PER RESOURCE SCHEDULE
       *---------------------------------------------------------------------*/
      prepare_resource_schedules(*(splitted_job_lists[SPLIT_RUNNING]),
                              *(splitted_job_lists[SPLIT_SUSPENDED]),
                              lists->pe_list, lists->host_list, lists->queue_list, 
                              lists->centry_list);
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
         NULL, false, false,
         QU_suspend_thresholds)) {
      DPRINTF(("couldn't split queue list with regard to suspend thresholds\n"));
      DEXIT;
      return -1;
   }

   unsuspend_job_in_queues(lists->queue_list, 
                           *(splitted_job_lists[SPLIT_SUSPENDED]), 
                           &(orders->configOrderList)); 
   suspend_job_in_queues(non_avail_queues, 
                         *(splitted_job_lists[SPLIT_RUNNING]), 
                         &(orders->configOrderList)); 
   non_avail_queues = lFreeList(non_avail_queues);

   /*---------------------------------------------------------------------
    * FILTER QUEUES
    *---------------------------------------------------------------------*/
   /* split queues into overloaded and non-overloaded queues */
   if (sge_split_queue_load(&(lists->queue_list), NULL, 
         lists->host_list, lists->centry_list, job_load_adjustments,
         NULL, false, true, QU_load_thresholds)) {
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

   /* Once we know if there are available queues we put
      the non available ones back into our main queue list
      this is actually needed for reservation scheduling */
   queues_available = (lFirst(lists->queue_list) != NULL);

   if (lists->queue_list == NULL) {
      lists->queue_list = non_avail_queues;
   } else {
      lAddList(lists->queue_list, non_avail_queues);
   }
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

   {
      int ret;
      PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);

      ret = sgeee_scheduler(lists, 
                    *(splitted_job_lists[SPLIT_RUNNING]),
                    *(splitted_job_lists[SPLIT_FINISHED]),
                    *(splitted_job_lists[SPLIT_PENDING]),
                    &(orders->pendingOrderList),
                    queues_available,
                    nr_pending_jobs > 0); 

      PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
      if (prof_is_active()) {
         u_long32 saved_logginglevel = log_state_get_log_level();

         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, "PROF: job-order calculation took %.3f s\n",
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

   if (!queues_available) {
      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_mes_add_global(SCHEDD_INFO_ALLALARMOVERLOADED_);
      lFreeList(user_list);
      lFreeList(group_list);
      lFreeList(non_avail_queues);
      DEXIT;
      return 0;
   } 
   
   job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                                 &user_list,
                                                 NULL,
                                                 maxujobs);

   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

/*   trash_splitted_jobs(splitted_job_lists); */

   if (nr_pending_jobs == 0) {
      /* no jobs to schedule */
      SCHED_MON((log_string, MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON ));
      lFreeList(user_list);
      lFreeList(group_list);
      lFreeList(non_avail_queues);
      DEXIT;
      return 0;
   }

   /* 
    * Order Jobs in descending order according to tickets and 
    * then job number 
    */
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM3);

   sgeee_sort_jobs(splitted_job_lists[SPLIT_PENDING]);

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM3);

   if (prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "PROF: job sorting took %.3f s\n",
            prof_get_measurement_wallclock(SGE_PROF_CUSTOM3, false, NULL)));
      log_state_set_log_level(saved_logginglevel);
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

   /* generate a consumable laod list structure. It stores which queues
      are using consumables in their load threshold. */
   sge_create_load_list(lists->queue_list, lists->host_list, lists->centry_list, 
                        &consumable_load_list);

   
   /*---------------------------------------------------------------------
    * DISPATCH JOBS TO QUEUES
    *---------------------------------------------------------------------*/

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM4);

   /*
    * loop over the jobs that are left in priority order
    */
   while ( (orig_job = lFirst(*(splitted_job_lists[SPLIT_PENDING])) ) &&
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
         if (sge_is_job_category_rejected(job))
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
               orders,
               &total_running_job_tickets, 
               &sort_hostlist, 
               dont_start,
               dont_reserve,
               &consumable_load_list);
      
      }

      switch (result) {
      case 0: /* now assignment */
         /* here the job got an assignment that will cause it be started immediately */
         DPRINTF(("Found NOW assignment\n"));
         schedd_mes_rollback();
         dispatched_a_job = true;
         job_move_first_pending_to_running(&orig_job, splitted_job_lists, &(orders->pendingOrderList));

         /* 
          * after sge_move_to_running() orig_job can be removed and job 
          * should be used instead 
          */
         orig_job = NULL;

         /* 
          * drop idle jobs that exceed maxujobs limit 
          * should be done after resort_job() 'cause job is referenced 
          */
         job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                             &user_list, 
                                             lGetString(job, JB_owner),
                                             maxujobs);
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
         schedd_mes_commit(*(splitted_job_lists[SPLIT_PENDING]), 0, cat);

         /* before deleting the element mark the category as rejected */
         if ((result == -1 || result == 1) && (cat = lGetRef(job, JB_category))) {
            DPRINTF(("SKIP JOB " u32 " of category '%s' (rc: "u32 ")\n", job_id, 
                        lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
            sge_reject_category(cat);
         }
         if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* immediate job */
            /* delet the job, it will be deleted on master side anyway */
            lDelElemUlong(splitted_job_lists[SPLIT_PENDING], JB_job_number, job_id); 
         }
         else {
         /* prevent that we get the same job next time again */
            lDechainElem(*(splitted_job_lists[SPLIT_PENDING]),orig_job);
            if ((*(splitted_job_lists[SPLIT_NOT_STARTED])) == NULL) {
               *(splitted_job_lists[SPLIT_NOT_STARTED]) = lCreateList("", lGetListDescr(*(splitted_job_lists[SPLIT_PENDING])));
            }
            lAppendElem(*(splitted_job_lists[SPLIT_NOT_STARTED]), orig_job);
         }
         orig_job = NULL;
         break;
      default:
         break;
      }

      job = lFreeElem(job);

      /*------------------------------------------------------------------ 
       * SGEEE mode - if we dispatch a job sub-task and the job has more
       * sub-tasks, then the job is still first in the job list.
       * We need to remove and reinsert the job back into the sorted job
       * list in case another job is higher priority (i.e. has more tickets)
       *------------------------------------------------------------------*/

      if (dispatched_a_job) {
         sgeee_resort_pending_jobs(splitted_job_lists[SPLIT_PENDING],
                                   orders->pendingOrderList);
      }

      /* no more free queues - then exit */
      if (lGetNumberOfElem(lists->queue_list)==0)
         break;

   } /* end of while */

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM4);

   if (prof_is_active()) {
      u_long32 saved_logginglevel = log_state_get_log_level();

      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "PROF: job dispatching took %.3f s\n",
            prof_get_measurement_wallclock(SGE_PROF_CUSTOM4, false, NULL)));
      log_state_set_log_level(saved_logginglevel);
   }

   lFreeList(user_list);
   lFreeList(group_list);
   sge_free_load_list(&consumable_load_list);
   
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
order_t *orders,
double *total_running_job_tickets,
int *sort_hostlist,
bool dont_start,  /* don't try to find a now assignment */
bool dont_reserve, /* don't try to find a reservation assignment */
lList **load_list
) {
   lListElem *granted_el;     
   int result = 1;
   const char *pe_name, *ckpt_name;
   sge_assignment_t a;
   bool reservation_mode = sconf_get_max_reservations()!=0;

   DENTER(TOP_LAYER, "select_assign_debit");

   assignment_init(&a, job, ja_task);
   a.load_adjustments = sconf_get_job_load_adjustments();
   a.queue_list       = *queue_list;
   a.host_list        = host_list;
   a.centry_list      = centry_list;
   a.acl_list         = acl_list;


   /* in reservation scheduling mode a non-zero duration always must be defined */
   if (reservation_mode && (!job_get_duration(&a.duration, job) || a.duration == 0)) {
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
         if (reservation_mode) {
            DPRINTF(("### looking for immediate parallel assignment for job "
               U32CFormat"."U32CFormat" requesting pe \"%s\" duration "U32CFormat"\n", 
                  a.job_id, a.ja_task_id, pe_name, a.duration)); 
         } else {
            DPRINTF(("### looking for immediate parallel assignment for job "
               U32CFormat"."U32CFormat" requesting pe \"%s\"\n", 
                  a.job_id, a.ja_task_id, pe_name)); 
         }
         a.start = DISPATCH_TIME_NOW;
         result = sge_select_parallel_environment(&a, pe_list);
      }

      if (result == 1) {
         if (!dont_reserve) {
            DPRINTF(("### looking for parallel reservation for job "
               U32CFormat"."U32CFormat" requesting pe \"%s\" duration "U32CFormat"\n", 
                  a.job_id, a.ja_task_id, pe_name, a.duration)); 

            a.start = DISPATCH_TIME_QUEUE_END;
            result = sge_select_parallel_environment(&a, pe_list);
            if (result == 0)
               result = 1; /* this job got a reservation */
         } else
            result = -1;
      }

   } else {
      lListElem *category = lGetRef(job, JB_category);
      bool use_category = (category != NULL) && lGetUlong(category, CT_refcount) > MIN_JOBS_IN_CATEGORY;

      a.slots = 1;

      if (!dont_start) {
         lList *ignore_queues = NULL, *ignore_hosts = NULL;

         if (reservation_mode) {
            DPRINTF(("### looking for immediate sequential assignment for job "
               U32CFormat"."U32CFormat" duration "U32CFormat"\n", 
                  a.job_id, a.ja_task_id, a.duration)); 
         } else {
            DPRINTF(("### looking for immediate sequential assignment for job "
               U32CFormat"."U32CFormat"\n", a.job_id, a.ja_task_id)); 
         }
         a.start = DISPATCH_TIME_NOW;

         result = sge_sequential_assignment(&a, &ignore_queues, &ignore_hosts);
         
         DPRINTF(("sge_sequential_assignment(immediate) returned %d\n", result));

         if (result == -1) {
            /* we store these hosts/queues only if an assignment is not possible now */
            if (use_category) {
               lAddSubList(category, CT_ignore_queues, ignore_queues);
               lAddSubList(category, CT_ignore_hosts, ignore_hosts);
               ignore_queues = NULL;
               ignore_hosts = NULL;
            }
         }

         ignore_queues = lFreeList(ignore_queues);
         ignore_hosts = lFreeList(ignore_hosts);
      }

      /* don't try to reserve for jobs that can not be dispatched with the current configuration */
      if (result == 1) {
         if (!dont_reserve) {
            DPRINTF(("### looking for sequential reservation for job "
               U32CFormat"."U32CFormat" duration "U32CFormat"\n", 
                  a.job_id, a.ja_task_id, a.duration)); 
            a.start = DISPATCH_TIME_QUEUE_END;

            result = sge_sequential_assignment(&a, NULL, NULL);
            if (result == 0)
               result = 1; /* this job got a reservation */
         } else 
            result = -1;
      } 
   }

   /*------------------------------------------------------------------
    * BUILD ORDERS LIST THAT TRANSFERS OUR DECISIONS TO QMASTER
    *------------------------------------------------------------------*/
   if (result == -1 || result == -2) {
      /* failed scheduling this job */
      if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* immediate job */
         /* generate order for removing it at qmaster */
         order_remove_immediate(job, ja_task, &(orders->pendingOrderList));
      }   
      DEXIT;
      return result;
   }

   if (result == 0) {
      /* in SGEEE we must account for job tickets on hosts due to parallel jobs */
      {
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

      orders->jobStartOrderList = sge_create_orders(orders->jobStartOrderList, ORT_start_job, 
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
      debit_scheduled_job(&a, sort_hostlist, orders, true, 
               SCHEDULING_RECORD_ENTRY_TYPE_STARTING);
   } else
      debit_scheduled_job(&a, sort_hostlist, orders, true, 
            SCHEDULING_RECORD_ENTRY_TYPE_RESERVING);

   /*------------------------------------------------------------------
    * REMOVE QUEUES THAT ARE NO LONGER USEFUL FOR FURTHER SCHEDULING
    *------------------------------------------------------------------*/
   if (!sconf_get_max_reservations()) {
      lList *disabled_queues = NULL;
      lListElem *queue;
      bool is_consumable_load_alarm = false;

      if (*load_list == NULL) {
         sge_split_suspended(queue_list, NULL);
         sge_split_queue_slots_free(queue_list, NULL);
      }
      else {
         sge_split_suspended(queue_list, &disabled_queues);
         sge_remove_queue_from_load_list(load_list, disabled_queues);
         disabled_queues = lFreeList(disabled_queues);

         sge_split_queue_slots_free(queue_list, &disabled_queues);
         sge_remove_queue_from_load_list(load_list, disabled_queues);
         disabled_queues = lFreeList(disabled_queues);
      }

      /* remove all taggs */
      for_each(queue, *queue_list) {
         lSetUlong(queue, QU_tagged4schedule, 0);
      }

      is_consumable_load_alarm = sge_load_list_alarm(*load_list, host_list,
                                                     centry_list);
      
      /* split queues into overloaded and non-overloaded queues */
      if (sge_split_queue_load(
            queue_list,                                     /* source list                              */
            (*load_list != NULL)? &disabled_queues: NULL,   /* no destination so they get trashed       */
            host_list,                                      /* host list contains load values           */
            centry_list, 
            a.load_adjustments,
            a.gdil,
            is_consumable_load_alarm,
            false, QU_load_thresholds)) {   /* use load thresholds here */
            
         DPRINTF(("couldn't split queue list concerning load\n"));
         assignment_release(&a);
         DEXIT;
         return MATCH_NEVER;
      }
      if (*load_list != NULL) {
         sge_remove_queue_from_load_list(load_list, disabled_queues);
         disabled_queues = lFreeList(disabled_queues);
      }
   }

   /* no longer needed - having built the order 
      and debited the job everywhere */
   assignment_release(&a);
  
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
*     time values were specified the default duration is used.
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
   double d_ret = 0, d_tmp;
   bool got_duration = false;
   char error_str[1024];
   const char *s;

   DENTER(TOP_LAYER, "job_get_duration");

   if ((ep=lGetElemStr(lGetList(jep, JB_hard_resource_list), CE_name, SGE_ATTR_H_RT))) {
      if (parse_ulong_val(&d_tmp, NULL, TYPE_TIM, (s=lGetString(ep, CE_stringval)),
               error_str, sizeof(error_str)-1)==0) {
         ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
         DEXIT;
         return false;
      }
      d_ret = d_tmp;
      got_duration = true;
   }
   if ((ep=lGetElemStr(lGetList(jep, JB_hard_resource_list), CE_name, SGE_ATTR_S_RT))) {
      if (parse_ulong_val(&d_tmp, NULL, TYPE_TIM, (s=lGetString(ep, CE_stringval)),
               error_str, sizeof(error_str)-1)==0) {
         ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
         DEXIT;
         return false;
      }

      if (got_duration) {
         d_ret = MAX(d_ret, d_tmp);
      } else {
         d_ret = d_tmp;
         got_duration = true;
      }
   }

   if (got_duration) {
      if (d_ret > (double)U_LONG32_MAX)
         *duration = U_LONG32_MAX;
      else
         *duration = d_ret;
      DEXIT;
      return true;
   } 

   *duration = sconf_get_default_duration();

   DEXIT;
   return true;
}


static int add_job_list_to_schedule(const lList *job_list, bool suspended, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *centry_list)
{
   lListElem *jep, *ja_task;
   const char *pe_name;
   const char *type;
   u_long32 now = sconf_get_now();
   u_long32 interval = sconf_get_schedule_interval();

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

         if (!job_get_duration(&a.duration, jep) || a.duration == 0) {
            ERROR((SGE_EVENT, "got running job with invalid duration\n"));
            continue; /* may never happen */
         }

         /* Prevent jobs that exceed their prospective duration are not reflected 
            in the resource schedules. Note duration enforcement is domain of 
            sge_execd and default_duration is not enforced at all anyways.
            All we can do here is hope the job will be finished in the next interval. */
         if (a.start + a.duration <= now) {
            a.duration = (now - a.start) + interval;
         }

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
               "start "U32CFormat" duration "U32CFormat"\n",  
                  lGetUlong(jep, JB_job_number), 
                  lGetUlong(ja_task, JAT_task_number), 
                  a.start, a.duration));

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

   /* just for information purposes... */

#ifdef DEBUG
   utilization_print_all(pe_list, host_list, queue_list); 
#endif   

   DEXIT;
   return;
}
