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

#include "commlib.h"
#include "sge_prog.h"
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
#include "sgeobj/sge_range.h"
#include "sge_support.h"
#include "sge_interactive_sched.h"
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
/*#include "sge_qinstance_type.h" */
#include "sig_handlers.h"
#include "sched/sge_resource_quota_schedd.h"

/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int 
dispatch_jobs(sge_evc_class_t *evc, sge_Sdescr_t *lists, order_t *orders, lList **splitted_job_list[]);

static dispatch_t 
select_assign_debit(lList **queue_list, lList **dis_queue_list, lListElem *job, lListElem *ja_task, 
                    lList *pe_list, lList *ckpt_list, lList *centry_list, lList *host_list, 
                    lList *acl_list, lList **user_list, lList **group_list, order_t *orders, 
                    double *total_running_job_tickets, int *sort_hostlist, bool is_start, 
                    bool is_reserve, lList **load_list, lList *hgrp_list, lList *rqs_list);

static bool 
job_get_duration(u_long32 *duration, const lListElem *jep);

static void 
prepare_resource_schedules(const lList *running_jobs, const lList *suspended_jobs, 
                           lList *pe_list, lList *host_list, lList *queue_list, 
                           lList *rqs_list, lList *centry_list, lList *acl_list, lList *hgroup_list);


static void 
add_calendar_to_schedule(lList *queue_list); 

static void 
set_utilization(lList *uti_list, u_long32 from, u_long32 till, double uti);

static lListElem 
*newResourceElem(u_long32 time, double amount); 

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
*     lList** orders - a list of orders, which needs to be send
*
*  RESULT
*     0 success  
*    -1 error  
*******************************************************************************/
#ifdef SCHEDULER_SAMPLES
int my_scheduler(sge_Sdescr_t *lists, lList **order)
{
   return scheduler(lists);
}
#endif
int scheduler(sge_evc_class_t *evc, sge_Sdescr_t *lists, lList **order) {
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
   lList *not_started_list = NULL;                 /* JB_Type */
   int prof_job_count, global_mes_count, job_mes_count;

   int i;

   DENTER(TOP_LAYER, "scheduler");

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM0);

   serf_new_interval(sge_get_gmt());
   orders.pendingOrderList = *order;
   *order = NULL;

   prof_job_count = lGetNumberOfElem(lists->job_list);

   sconf_reset_jobs();
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
              mconf_get_max_aj_instances(), splitted_job_lists);

   { /* add global queue messages */
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
         "|| %I m= %u)",
         dp,
         QU_state, QI_SUSPENDED,        /* only not suspended queues      */
         QU_state, QI_SUSPENDED_ON_SUBORDINATE,
         QU_state, QI_ERROR,            /* no queues in error state       */
         QU_state, QI_AMBIGUOUS,
         QU_state, QI_ORPHANED,
         QU_state, QI_UNKNOWN);         /* only known queues              */

      if (!what || !where) {
         DPRINTF(("failed creating where or what describing non available queues\n")); 
      }
      qlp = lSelect("", lists->all_queue_list, where, what);

      for_each(mes_queues, qlp) {
         schedd_mes_add_global(SCHEDD_INFO_QUEUENOTAVAIL_, 
                                   lGetString(mes_queues, QU_full_name));
      }                             

      for_each(mes_queues, lists->dis_queue_list) {
         schedd_mes_add_global(SCHEDD_INFO_QUEUENOTAVAIL_, 
                                   lGetString(mes_queues, QU_full_name));
      }
      
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED, 
                      qlp, QU_full_name);
      lFreeList(&qlp);
      lFreeWhere(&where);
      lFreeWhat(&what);
   }

   /**
    * the actual scheduling 
    */
   dispatch_jobs(evc, lists, &orders, splitted_job_lists);

   /**
    * post processing 
    */
   remove_immediate_jobs(*(splitted_job_lists[SPLIT_PENDING]), 
                         *(splitted_job_lists[SPLIT_RUNNING]), &orders);

   /* send job_start_orders */
   if (!shut_me_down) {
      sge_send_job_start_orders(evc, &orders);
   }

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
            orders.pendingOrderList = sge_create_orders(orders.pendingOrderList, 
                                                        ORT_clear_pri_info, 
                                                        job, NULL, NULL, false);

         }
      }
                          
   }
   sge_build_sgeee_orders(lists, NULL,*(splitted_job_lists[SPLIT_PENDING]), NULL, 
                          &orders, false, 0, false); 
   
   sge_build_sgeee_orders(lists, NULL,*(splitted_job_lists[SPLIT_NOT_STARTED]), NULL, 
                          &orders, false, 0, false); 
 
   /* generated scheduler messages, thus we have to call it */
   trash_splitted_jobs(splitted_job_lists); 

   orders.jobStartOrderList= sge_add_schedd_info(orders.jobStartOrderList, &global_mes_count, &job_mes_count);

   if (prof_is_active(SGE_PROF_SCHEDLIB4)) {
      prof_stop_measurement(SGE_PROF_SCHEDLIB4, NULL);
      PROFILING((SGE_EVENT, "PROF: create pending job orders: %.3f s",
               prof_get_measurement_utime(SGE_PROF_SCHEDLIB4,false, NULL)));
   }   

   if(prof_is_active(SGE_PROF_CUSTOM0)) {
      prof_stop_measurement(SGE_PROF_CUSTOM0, NULL); 

      PROFILING((SGE_EVENT, "PROF: scheduled in %.3f (u %.3f + s %.3f = %.3f): %d sequential, %d parallel, %d orders, %d H, %d Q, %d QA, %d J(qw), %d J(r), %d J(s), %d J(h), %d J(e), %d J(x), %d J(all), %d C, %d ACL, %d PE, %d U, %d D, %d PRJ, %d ST, %d CKPT, %d RU, %d gMes, %d jMes, %d/%d pre-send, %d/%d/%d pe-alg\n",
         prof_get_measurement_wallclock(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         prof_get_measurement_utime(SGE_PROF_CUSTOM0, true, NULL) + 
         prof_get_measurement_stime(SGE_PROF_CUSTOM0, true, NULL),
         sconf_get_fast_jobs(),
         sconf_get_comprehensive_jobs(),
         (int)(orders.numberSendOrders + sge_GetNumberOfOrders(&orders)), 
         lGetNumberOfElem(lists->host_list), 
         lGetNumberOfElem(lists->queue_list),
         lGetNumberOfElem(lists->all_queue_list),
         (lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING])) + lGetNumberOfElem(*(splitted_job_lists[SPLIT_NOT_STARTED]))),
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
         lGetNumberOfElem(lists->running_per_user),
         global_mes_count,
         job_mes_count,
         (int)orders.numberSendOrders,
         (int)orders.numberSendPackages,
         sconf_get_pe_alg_value(SCHEDD_PE_LOW_FIRST),
         sconf_get_pe_alg_value(SCHEDD_PE_BINARY),
         sconf_get_pe_alg_value(SCHEDD_PE_HIGH_FIRST)
      ));
   }
   
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM5); 
   
   /* free all job lists */
   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      if (splitted_job_lists[i]) {
         lFreeList(splitted_job_lists[i]);
         splitted_job_lists[i] = NULL;
      }
   }

   if (!shut_me_down) {
      lList *orderlist=sge_join_orders(&orders);

      if (orderlist != NULL) {
         /*
          * Scheduler needs a relatively high synchronuous receive timeout as minimum. This
          * is required for cases when orders processing takes long time due to very slow 
          * qmaster spooling (e.g. classic over NFS).
          *
          * We'll reset it after sending the orders, to have a poss. lower timeout
          * (2 * event client interval) for receiving new events.
          */
         cl_com_set_synchron_receive_timeout(cl_com_get_handle(prognames[SCHEDD], 0), 120);
         sge_send_orders2master(evc, &orderlist);
         cl_com_set_synchron_receive_timeout(cl_com_get_handle(prognames[SCHEDD], 0), 
                                             (int) (sconf_get_schedule_interval() * 2));

         lFreeList(&orderlist);
      }
   }


   schedd_mes_release();
   schedd_mes_set_logging(0); 

   if(prof_is_active(SGE_PROF_CUSTOM5)) {
      prof_stop_measurement(SGE_PROF_CUSTOM5, NULL); 
   
      PROFILING((SGE_EVENT, "PROF: send orders and cleanup took: %.3f (u %.3f,s %.3f) s",
         prof_get_measurement_wallclock(SGE_PROF_CUSTOM5, true, NULL), 
         prof_get_measurement_utime(SGE_PROF_CUSTOM5, true, NULL),
         prof_get_measurement_stime(SGE_PROF_CUSTOM5, true, NULL) ));
   }
   
   DRETURN(0);
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
static int dispatch_jobs(sge_evc_class_t *evc, sge_Sdescr_t *lists, order_t *orders,
                         lList **splitted_job_lists[]) 
{
   lList *user_list=NULL, *group_list=NULL;
   lListElem *orig_job, *cat=NULL;
   lList *none_avail_queues = NULL;
   lList *consumable_load_list = NULL;

   u_long32 queue_sort_method; 
   u_long32 maxujobs;
   lList *job_load_adjustments = NULL;
   double total_running_job_tickets=0; 
   int nreservation = 0;
   int fast_runs = 0;         /* sequential jobs */
   int fast_soft_runs = 0;    /* sequential jobs with soft requests */
   int comprehensive_runs = 0;      /* all kind of pe jobs */

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

   sconf_set_global_load_correction((global_lc != 0) ? true : false);
   
   /* we will assume this time as start time for now assignments */
   sconf_set_now(sge_get_gmt());

   if (max_reserve != 0) {
      lListElem *dis_queue_elem = lFirst(lists->dis_queue_list);
      /*----------------------------------------------------------------------
       * ENSURE RUNNING JOBS ARE REFLECTED IN PER RESOURCE SCHEDULE
       *---------------------------------------------------------------------*/

      if (dis_queue_elem != NULL) {
         lAppendList(lists->queue_list, lists->dis_queue_list);
      }
      
      
      prepare_resource_schedules(*(splitted_job_lists[SPLIT_RUNNING]),
                                 *(splitted_job_lists[SPLIT_SUSPENDED]),
                                 lists->pe_list, lists->host_list, lists->queue_list, 
                                 lists->rqs_list, lists->centry_list, lists->acl_list,
                                 lists->hgrp_list);

      if (dis_queue_elem != NULL) {
         lDechainList(lists->queue_list, &(lists->dis_queue_list), dis_queue_elem);
      }
      
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
         &none_avail_queues,            /* list of queues in suspend alarm state  */
         lists->host_list,
         lists->centry_list,
         job_load_adjustments,
         NULL, false, false,
         QU_suspend_thresholds)) {
      lFreeList(&none_avail_queues);
      lFreeList(&job_load_adjustments);
      DPRINTF(("couldn't split queue list with regard to suspend thresholds\n"));
      DRETURN(-1);
   }

   unsuspend_job_in_queues(lists->queue_list, 
                           *(splitted_job_lists[SPLIT_SUSPENDED]), 
                           orders); 
   suspend_job_in_queues(none_avail_queues, 
                         *(splitted_job_lists[SPLIT_RUNNING]), 
                         orders); 
   lFreeList(&none_avail_queues);

   /*---------------------------------------------------------------------
    * FILTER QUEUES
    *---------------------------------------------------------------------*/
   /* split queues into overloaded and non-overloaded queues */
   if (sge_split_queue_load(&(lists->queue_list), NULL, 
         lists->host_list, lists->centry_list, job_load_adjustments,
         NULL, false, true, QU_load_thresholds)) {
      lFreeList(&job_load_adjustments);
      DPRINTF(("couldn't split queue list concerning load\n"));
      DRETURN(-1);
   }

   /* remove cal_disabled queues - needed them for implementing suspend thresholds */
   if (sge_split_cal_disabled(&(lists->queue_list), &lists->dis_queue_list)) {
      DPRINTF(("couldn't split queue list concerning cal_disabled state\n"));
      lFreeList(&job_load_adjustments);
      DRETURN(-1);
   }
   
   /* trash disabled queues - needed them for implementing suspend thresholds */
   if (sge_split_disabled(&(lists->queue_list), &none_avail_queues)) {
      DPRINTF(("couldn't split queue list concerning disabled state\n"));
      lFreeList(&none_avail_queues);
      lFreeList(&job_load_adjustments);
      DRETURN(-1);
   }

   lFreeList(&none_avail_queues);
   if (sge_split_queue_slots_free(&(lists->queue_list), &none_avail_queues)) {
      DPRINTF(("couldn't split queue list concerning free slots\n"));
      lFreeList(&none_avail_queues);
      lFreeList(&job_load_adjustments);
      DRETURN(-1);
   }
   if (lists->dis_queue_list != NULL) {
      lAddList(lists->dis_queue_list, &none_avail_queues);
   }
   else {
      lists->dis_queue_list = none_avail_queues;
      none_avail_queues = NULL;
   }


   /*---------------------------------------------------------------------
    * FILTER JOBS
    *---------------------------------------------------------------------*/

   DPRINTF(("STARTING PASS 1 WITH %d PENDING JOBS\n", 
            lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]))));

   user_list_init_jc(&user_list, splitted_job_lists);

   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

   DPRINTF(("STARTING PASS 2 WITH %d PENDING JOBS\n",nr_pending_jobs ));

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
                    orders); 

      /* async gdi */
      sge_send_job_start_orders(evc, orders); 

      if (prof_is_active(SGE_PROF_CUSTOM1)) {
         prof_stop_measurement(SGE_PROF_CUSTOM1, NULL);

         PROFILING((SGE_EVENT, "PROF: job-order calculation took %.3f s",
               prof_get_measurement_wallclock(SGE_PROF_CUSTOM1, true, NULL)));
      }

      if( ret != 0){
         lFreeList(&user_list);
         lFreeList(&group_list);
         lFreeList(&job_load_adjustments);
         DRETURN(-1);
      }
   }

   if ( ((max_reserve == 0) && (lGetNumberOfElem(lists->queue_list) == 0)) || /* no reservatoin and no queues avail */
        ((lGetNumberOfElem(lists->queue_list) == 0) && (lGetNumberOfElem(lists->dis_queue_list) == 0))) { /* reservation and no queues avail */
      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_mes_add_global(SCHEDD_INFO_ALLALARMOVERLOADED_);
      lFreeList(&user_list);
      lFreeList(&group_list);
      lFreeList(&job_load_adjustments);
      DRETURN(0);
   } 
   
   job_lists_split_with_reference_to_max_running(splitted_job_lists,
                                                 &user_list,
                                                 NULL,
                                                 maxujobs);

   nr_pending_jobs = lGetNumberOfElem(*(splitted_job_lists[SPLIT_PENDING]));

   if (nr_pending_jobs == 0) {
      /* no jobs to schedule */
      schedd_log(MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON);
      lFreeList(&user_list);
      lFreeList(&group_list);
      lFreeList(&job_load_adjustments);
      DRETURN(0);
   }

   /* 
    * Order Jobs in descending order according to tickets and 
    * then job number 
    */
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM3);

   sgeee_sort_jobs(splitted_job_lists[SPLIT_PENDING]);

   if (prof_is_active(SGE_PROF_CUSTOM3)) {
      prof_stop_measurement(SGE_PROF_CUSTOM3, NULL);

      PROFILING((SGE_EVENT, "PROF: job sorting took %.3f s",
            prof_get_measurement_wallclock(SGE_PROF_CUSTOM3, false, NULL)));
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
   {
      bool is_immediate_array_job = false;
      struct timeval now, later;
      double time;
      gettimeofday(&now, NULL);

      while ( (orig_job = lFirst(*(splitted_job_lists[SPLIT_PENDING]))) != NULL) {
         dispatch_t result = DISPATCH_NEVER_CAT;
         u_long32 job_id; 
         bool is_pjob_resort = false;
         bool is_reserve;
         bool is_start;

         job_id = lGetUlong(orig_job, JB_job_number);

         /* 
          * We don't try to get a reservation, if 
          * - reservation is generally disabled 
          * - maximum number of reservations is exceeded 
          * - it's not desired for the job
          * - the job is an immediate one
          */
         if (nreservation < max_reserve &&
             lGetBool(orig_job, JB_reserve) &&
             !JOB_TYPE_IS_IMMEDIATE(lGetUlong(orig_job, JB_type))) {
            is_reserve = true;
         }   
         else {
            is_reserve = false; 
         }   

         /* Don't need to look for a 'now' assignment if the last job 
            of this category got no 'now' assignement either */
         is_start = (sge_is_job_category_rejected(orig_job))?false:true;
            
         if (is_start || is_reserve) {
            lListElem *job = NULL;
            u_long32 ja_task_id; 
            lListElem *ja_task;


            /* sort the hostlist */
            if(sort_hostlist) {
               lPSortList(lists->host_list, "%I+", EH_sort_value);
               sort_hostlist      = 0;
               sconf_set_host_order_changed(true);
            } 
            else {
               sconf_set_host_order_changed(false);
            }  

            is_immediate_array_job = (is_immediate_array_job || 
                                    (JOB_TYPE_IS_ARRAY(lGetUlong(orig_job, JB_type)) && 
                                     JOB_TYPE_IS_IMMEDIATE(lGetUlong(orig_job, JB_type)))) ? true : false;


            if ((job = lCopyElem(orig_job)) == NULL) {
               break;
            }

            if (job_get_next_task(job, &ja_task, &ja_task_id) != 0) {
               DPRINTF(("Found job "sge_u32" with no job array tasks\n", job_id));
            } 
            else { 
               DPRINTF(("Found pending job "sge_u32"."sge_u32". Try %sto start and %sto reserve\n", 
                     job_id, ja_task_id, is_start?"":"not ", is_reserve?"":"not "));
               DPRINTF(("-----------------------------------------\n"));

               result = select_assign_debit(
                  &(lists->queue_list), 
                  &(lists->dis_queue_list),
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
                  is_start,
                  is_reserve,
                  &consumable_load_list,
                  lists->hgrp_list,
                  lists->rqs_list);
            } 
            lFreeElem(&job);
         }     
  
         /* collect profiling data */
         switch (sconf_get_last_dispatch_type()) {
            case DISPATCH_TYPE_FAST : fast_runs++;
               break;
            case DISPATCH_TYPE_FAST_SOFT_REQ : fast_soft_runs++;
               break;
            case DISPATCH_TYPE_COMPREHENSIVE : comprehensive_runs++;
               break;
         }

         switch (result) {
         case DISPATCH_OK: /* now assignment */
            {
               char *owner = strdup(lGetString(orig_job, JB_owner));
               /* here the job got an assignment that will cause it be started immediately */

               DPRINTF(("Found NOW assignment\n"));

               schedd_mes_rollback();
               is_pjob_resort = job_move_first_pending_to_running(&orig_job, splitted_job_lists);

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
                                                   owner,
                                                   maxujobs);
               FREE(owner);

               /* do not send job start orders inbetween, if we have an immediate array
                  job. */
               if (!is_immediate_array_job && (lGetNumberOfElem(orders->jobStartOrderList) > 10)) {
                  gettimeofday(&later, NULL);
                  time = later.tv_usec - now.tv_usec;
                  time = (time / 1000000) + (later.tv_sec - now.tv_sec);

                  if (time > 0.5) {
                     /*async gdi*/
                     if (sge_send_job_start_orders(evc, orders)) {
                        gettimeofday(&now, NULL);
                     }
                  }
               }
            }
            break;

         case DISPATCH_NOT_AT_TIME: /* reservation */
            /* here the job got a reservation but can't be started now */
            DPRINTF(("Got a RESERVATION\n"));
            nreservation++;

            /* mark the category as rejected */
            if ((cat = lGetRef(orig_job, JB_category))) {
               DPRINTF(("SKIP JOB " sge_u32 " of category '%s' (rc: "sge_u32 ")\n", job_id, 
                           lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
               sge_reject_category(cat);
            }
            /* here no reservation was made for a job that couldn't be started now 
               or the job is not dispatchable at all */
            schedd_mes_commit(*(splitted_job_lists[SPLIT_PENDING]), 0, cat);       

            /* Remove pending job if there are no pending tasks anymore (including the current) */
            if (job_count_pending_tasks(orig_job, true) < 2 || (nreservation >= max_reserve )) {

               lDechainElem(*(splitted_job_lists[SPLIT_PENDING]), orig_job);
               if ((*(splitted_job_lists[SPLIT_NOT_STARTED])) == NULL) {
                  *(splitted_job_lists[SPLIT_NOT_STARTED]) = lCreateList("", lGetListDescr(*(splitted_job_lists[SPLIT_PENDING])));
               }
               lAppendElem(*(splitted_job_lists[SPLIT_NOT_STARTED]), orig_job);
               is_pjob_resort = false;
            }
            else {
               u_long32 ja_task_number = range_list_get_first_id(lGetList(orig_job, JB_ja_n_h_ids), NULL);
               object_delete_range_id(orig_job, NULL, JB_ja_n_h_ids, ja_task_number);
               is_pjob_resort = true;
            }
            orig_job = NULL;
            break;

         case DISPATCH_NEVER_CAT: /* never this category */
            /* before deleting the element mark the category as rejected */
            if ((cat = lGetRef(orig_job, JB_category))) {
               DPRINTF(("SKIP JOB " sge_u32 " of category '%s' (rc: "sge_u32 ")\n", job_id, 
                        lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
               sge_reject_category(cat);
            }
         
         case DISPATCH_NEVER_JOB: /* never this particular job */

            /* here no reservation was made for a job that couldn't be started now 
               or the job is not dispatchable at all */
            schedd_mes_commit(*(splitted_job_lists[SPLIT_PENDING]), 0, cat);

            if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(orig_job, JB_type))) { /* immediate job */
               lCondition *where = NULL;
               lListElem *rjob;
               /* remove job from pending list and generate remove immediate orders 
                  for all tasks including alreaedy assigned ones */
               where = lWhere("%T(%I==%u)", JB_Type, JB_job_number, job_id);
               remove_immediate_job(*(splitted_job_lists[SPLIT_PENDING]), orig_job, orders, 0);
               if ((rjob = lFindFirst(*(splitted_job_lists[SPLIT_RUNNING]), where)) != NULL)
                  remove_immediate_job(*(splitted_job_lists[SPLIT_RUNNING]), rjob, orders, 1);
               lFreeWhere(&where);
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

         case DISPATCH_MISSING_ATTR :  /* should not happen */
         default:
            break;
         }

         /*------------------------------------------------------------------ 
          * SGEEE mode - if we dispatch a job sub-task and the job has more
          * sub-tasks, then the job is still first in the job list.
          * We need to remove and reinsert the job back into the sorted job
          * list in case another job is higher priority (i.e. has more tickets)
          *------------------------------------------------------------------*/

         if (is_pjob_resort) {
            sgeee_resort_pending_jobs(splitted_job_lists[SPLIT_PENDING]);
         }

         /* no more free queues - then exit */
         if (lGetNumberOfElem(lists->queue_list)==0) {
            break;
         }   

      } /* end of while */
   }

   if (prof_is_active(SGE_PROF_CUSTOM4)) {
      prof_stop_measurement(SGE_PROF_CUSTOM4, NULL);
      PROFILING((SGE_EVENT, "PROF: job dispatching took %.3f s (%d fast, %d comp, %d pe, %d res)",
                 prof_get_measurement_wallclock(SGE_PROF_CUSTOM4, false, NULL),
                 fast_runs,
                 fast_soft_runs,
                 comprehensive_runs,
                 nreservation));
   }

   lFreeList(&user_list);
   lFreeList(&group_list);
   sge_free_load_list(&consumable_load_list);
   lFreeList(&job_load_adjustments);
   
   DRETURN(0);
}




/****** schedd/scheduler/select_assign_debit() ********************************
*  NAME
*     select_assign_debit()
*
*  FUNCTION
*     Selects resources for 'job', add appropriate order to the 'orders_list',
*     debits resources of this job for the next dispatch and sort out no longer
*     available queues from the 'queue_list'. If no assignment can be made and 
*     reservation scheduling is enabled a reservation assignment is made if 
*     possible. This is done to prevent lower prior jobs eating up resources 
*     and thus preventing this job from being started at the earliest point in 
*     time.
*
*  INPUTS
*     bool is_start  -   try to find a now assignment 
*     bool is_reserve -  try to find a reservation assignment 
*
*  RESULT
*     int - 0 ok got an assignment now
*           1 got a reservation assignment
*          -1 will never get an assignment for that category
*          -2 will never get an assignment for that particular job
******************************************************************************/
static dispatch_t
select_assign_debit(lList **queue_list, lList **dis_queue_list, lListElem *job, lListElem *ja_task,
                    lList *pe_list, lList *ckpt_list, lList *centry_list, lList *host_list, lList *acl_list,
                    lList **user_list, lList **group_list, order_t *orders, double *total_running_job_tickets,
                    int *sort_hostlist, bool is_start,  bool is_reserve, lList **load_list, lList *hgrp_list,
                    lList *rqs_list) 
{
   lListElem *granted_el;     
   dispatch_t result = DISPATCH_NOT_AT_TIME;
   const char *pe_name, *ckpt_name;
   sge_assignment_t a = SGE_ASSIGNMENT_INIT;
   bool is_computed_reservation = false;

   DENTER(TOP_LAYER, "select_assign_debit");

   assignment_init(&a, job, ja_task, true);
   a.queue_list       = *queue_list;
   a.host_list        = host_list;
   a.centry_list      = centry_list;
   a.acl_list         = acl_list;
   a.hgrp_list        = hgrp_list;
   a.rqs_list        = rqs_list;

   /* in reservation scheduling mode a non-zero duration always must be defined */
   if ( !job_get_duration(&a.duration, job) ) {
      schedd_mes_add(a.job_id, SCHEDD_INFO_CKPTNOTFOUND_);
      assignment_release(&a);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   a.duration += sconf_get_duration_offset();

   /*------------------------------------------------------------------ 
    * seek for ckpt interface definition requested by the job 
    * in case of ckpt jobs 
    *------------------------------------------------------------------*/
   if ((ckpt_name = lGetString(job, JB_checkpoint_name))) {
      a.ckpt = ckpt_list_locate(ckpt_list, ckpt_name);
      if (!a.ckpt) {
         schedd_mes_add(a.job_id, SCHEDD_INFO_CKPTNOTFOUND_);
         assignment_release(&a);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   /*------------------------------------------------------------------ 
    * if a global host pointer exists it is needed everywhere 
    * down in the assignment code (tired of searching/passing it).
    *------------------------------------------------------------------*/
   a.gep = host_list_locate(host_list, SGE_GLOBAL_NAME);

   if ((pe_name = lGetString(job, JB_pe)) != NULL) { 
   /*------------------------------------------------------------------
    * SELECT POSSIBLE QUEUE(S) FOR THIS PE JOB
    *------------------------------------------------------------------*/

      if (is_start) {

         DPRINTF(("### looking for immediate parallel assignment for job "
                  sge_U32CFormat"."sge_U32CFormat" requesting pe \"%s\" duration "sge_U32CFormat"\n", 
                  a.job_id, a.ja_task_id, pe_name, a.duration)); 
         
         a.start = DISPATCH_TIME_NOW;
         a.is_reservation = false;
         if (is_reserve) {
            if (*queue_list == NULL) { 
               *queue_list = lCreateList("temp queue", lGetListDescr(*dis_queue_list));
               a.queue_list       = *queue_list;
            }
            is_computed_reservation = true;
            lAppendList(*queue_list, *dis_queue_list);
         }
         
         result = sge_select_parallel_environment(&a, pe_list);
      }

      if (result == DISPATCH_NOT_AT_TIME) {
         if (is_reserve) {
            DPRINTF(("### looking for parallel reservation for job "
               sge_U32CFormat"."sge_U32CFormat" requesting pe \"%s\" duration "sge_U32CFormat"\n", 
                  a.job_id, a.ja_task_id, pe_name, a.duration)); 
            is_computed_reservation = true;
            a.start = DISPATCH_TIME_QUEUE_END;
            a.is_reservation = true;

            result = sge_select_parallel_environment(&a, pe_list);

            if (result == DISPATCH_OK) {
               result = DISPATCH_NOT_AT_TIME; /* this job got a reservation */
            }   
         } 
         else {
            result = DISPATCH_NEVER_CAT;
         }   
      }

   } 
   else {
      /*------------------------------------------------------------------
       * SELECT POSSIBLE QUEUE(S) FOR THIS SEQUENTIAL JOB
       *------------------------------------------------------------------*/

      a.slots = 1;

      if (is_start) {

         DPRINTF(("### looking for immediate sequential assignment for job "
                  sge_U32CFormat"."sge_U32CFormat" duration "sge_U32CFormat"\n", a.job_id, 
                  a.ja_task_id, a.duration)); 
         
         a.start = DISPATCH_TIME_NOW;
         a.is_reservation = false;
         if (is_reserve) {
            is_computed_reservation = true;
            if (*queue_list == NULL) { 
               *queue_list  = lCreateList("temp queue", lGetListDescr(*dis_queue_list));
               a.queue_list = *queue_list;
            }
            lAppendList(*queue_list, *dis_queue_list);
         }
         result = sge_sequential_assignment(&a);
         
         DPRINTF(("sge_sequential_assignment(immediate) returned %d\n", result));
      }

      /* try to reserve for jobs that can be dispatched with the current configuration */
      if (result == DISPATCH_NOT_AT_TIME) {
         if (is_reserve) {
            DPRINTF(("### looking for sequential reservation for job "
               sge_U32CFormat"."sge_U32CFormat" duration "sge_U32CFormat"\n", 
                  a.job_id, a.ja_task_id, a.duration)); 
            a.start = DISPATCH_TIME_QUEUE_END;
            a.is_reservation = true;
            
            result = sge_sequential_assignment(&a);
            if (result == DISPATCH_OK) {
               result = DISPATCH_NOT_AT_TIME; /* this job got a reservation */
            }   
         } 
         else {
            result = DISPATCH_NEVER_CAT;
         }   
      } 
   }

   /*------------------------------------------------------------------
    * BUILD ORDERS LIST THAT TRANSFERS OUR DECISIONS TO QMASTER
    *------------------------------------------------------------------*/
   if (result == DISPATCH_NEVER_CAT || result == DISPATCH_NEVER_JOB) {
      /* failed scheduling this job */
      if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* immediate job */
         /* generate order for removing it at qmaster */
         order_remove_immediate(job, ja_task, orders);
      }
      assignment_release(&a);
      DRETURN(result);
   }

   if (result == DISPATCH_OK) {
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
            job, ja_task, a.gdil, true);

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
   if (result == DISPATCH_OK) {
      if (a.start == DISPATCH_TIME_NOW) {
         a.start = sconf_get_now();
      }   
      debit_scheduled_job(&a, sort_hostlist, orders, true, 
               SCHEDULING_RECORD_ENTRY_TYPE_STARTING);
   } 
   else {
      debit_scheduled_job(&a, sort_hostlist, orders, false,  
            SCHEDULING_RECORD_ENTRY_TYPE_RESERVING);
   }   

   /*------------------------------------------------------------------
    * REMOVE QUEUES THAT ARE NO LONGER USEFUL FOR FURTHER SCHEDULING
    *------------------------------------------------------------------*/
   if (result == DISPATCH_OK || is_computed_reservation) {
      lListElem *queue;
      lList *disabled_queues = NULL;
      bool is_consumable_load_alarm = false;

      if (*load_list == NULL) {
         sge_split_suspended(queue_list, dis_queue_list);
         sge_split_queue_slots_free(queue_list, dis_queue_list);
      }
      else {
         sge_split_suspended(queue_list, &disabled_queues);
         sge_split_queue_slots_free(queue_list, &disabled_queues);
         
         sge_remove_queue_from_load_list(load_list, disabled_queues);
         if (*dis_queue_list == NULL) {
            *dis_queue_list = disabled_queues;
         }
         else {
            lAddList(*dis_queue_list, &disabled_queues);
         }   
         disabled_queues = NULL;
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
            &disabled_queues,
            host_list,                                      /* host list contains load values           */
            centry_list, 
            a.load_adjustments,
            a.gdil,
            is_consumable_load_alarm,
            false, QU_load_thresholds)) {   /* use load thresholds here */
            
         DPRINTF(("couldn't split queue list concerning load\n"));
         assignment_release(&a);
         DRETURN(DISPATCH_NEVER);
      }
      if (*load_list != NULL) {
         sge_remove_queue_from_load_list(load_list, disabled_queues);
      }

      if (*dis_queue_list == NULL) {
         *dis_queue_list = disabled_queues;
      }
      else {
         lAddList(*dis_queue_list, &disabled_queues);
      }        
      disabled_queues = NULL;
   }

   /* no longer needed - having built the order 
      and debited the job everywhere */
   assignment_release(&a);
   DRETURN(result);
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
         DRETURN(false);
      }
      d_ret = d_tmp;
      got_duration = true;
   }
   
   if ((ep=lGetElemStr(lGetList(jep, JB_hard_resource_list), CE_name, SGE_ATTR_S_RT))) {
      if (parse_ulong_val(&d_tmp, NULL, TYPE_TIM, (s=lGetString(ep, CE_stringval)),
               error_str, sizeof(error_str)-1)==0) {
         ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, SGE_ATTR_H_RT, s, error_str));
         DRETURN(false);
      }

      if (got_duration) {
         d_ret = MAX(d_ret, d_tmp);
      } else {
         d_ret = d_tmp;
         got_duration = true;
      }
   }

   if (got_duration) {
      if (d_ret > (double)U_LONG32_MAX) {
         *duration = U_LONG32_MAX;
      }   
      else {
         *duration = d_ret;
      }   
   } 
   else {
      *duration = sconf_get_default_duration();
   }

   DRETURN(true);
}


static int 
add_job_list_to_schedule(const lList *job_list, bool suspended, lList *pe_list, 
                         lList *host_list, lList *queue_list, lList *rqs_list,
                         lList *centry_list, lList *acl_list, lList *hgroup_list)
{
   lListElem *jep, *ja_task;
   const char *pe_name;
   const char *type;
   u_long32 now = sconf_get_now();
   u_long32 interval = sconf_get_schedule_interval();

   DENTER(TOP_LAYER, "add_job_list_to_schedule");

   if (suspended) {
      type = SCHEDULING_RECORD_ENTRY_TYPE_SUSPENDED;
   }   
   else {
      type = SCHEDULING_RECORD_ENTRY_TYPE_RUNNING;
   }   

   for_each (jep, job_list) {
      for_each (ja_task, lGetList(jep, JB_ja_tasks)) {  
         sge_assignment_t a = SGE_ASSIGNMENT_INIT;

         assignment_init(&a, jep, ja_task, false);

         a.start = lGetUlong(ja_task, JAT_start_time);

         if (!job_get_duration(&a.duration, jep) || a.duration == 0) {
            ERROR((SGE_EVENT, "got running job with invalid duration\n"));
            continue; /* may never happen */
         }
         a.duration += sconf_get_duration_offset();

         /* Prevent jobs that exceed their prospective duration are not reflected 
            in the resource schedules. Note duration enforcement is domain of 
            sge_execd and default_duration is not enforced at all anyways.
            All we can do here is hope the job will be finished in the next interval. */
         if (a.start + a.duration <= now) {
            /* That logging is disabled as it can cause schedd messages file
               be filled up with loggings. There are cases when it can't be 
               considered a misconfiguration if jobs do not complete within the
               time foreseen. If jobs are submitted without -l h_rt limit and 
               aren't cancelled due to default_duration only be in effect */
#if 0
            if (sconf_get_max_reservations() > 0) {
               WARNING((SGE_EVENT, MSG_SCHEDD_SHOULDHAVEFINISHED_UUU, 
                     sge_u32c(a.job_id), sge_u32c(a.ja_task_id), 
                     sge_u32c(now - a.duration - a.start + 1)));
            }
#endif
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
         a.rqs_list = rqs_list;
         a.acl_list = acl_list;
         a.hgrp_list = hgroup_list;

         DPRINTF(("Adding job "sge_U32CFormat"."sge_U32CFormat" into schedule " "start "
                  sge_U32CFormat" duration "sge_U32CFormat"\n", lGetUlong(jep, JB_job_number), 
                  lGetUlong(ja_task, JAT_task_number), a.start, a.duration));

         /* only update resource utilization schedule  
            RUE_utililized_now is already set through events */
         debit_scheduled_job(&a, NULL, NULL, false, type);
      }
   }

   DRETURN(0);
}

/****** scheduler/add_calendar_to_schedule() ***********************************
*  NAME
*     add_calendar_to_schedule() -- addes the queue calendar to the resource
*                                   schedule
*
*  SYNOPSIS
*     static void add_calendar_to_schedule(lList *queue_list) 
*
*  FUNCTION
*     Adds the queue calendars to the resource schedule. It is using
*     the slot entry for simulating and enabled / disabled calendar.
*
*  INPUTS
*     lList *queue_list - all queues, which can posibly run jobs
*
*  NOTES
*     MT-NOTE: add_calendar_to_schedule() is MT safe 
*
*  SEE ALSO
*     scheduler/set_utilization
*     scheduler/newResourceElem
*     scheduler/prepare_resource_schedules
*******************************************************************************/
static void 
add_calendar_to_schedule(lList *queue_list) 
{
   lListElem *queue;

   DENTER(TOP_LAYER, "add_calendar_to_schedule");

   for_each(queue, queue_list) {
      lList *queue_states = lGetList(queue, QU_state_changes);
      u_long32 from       = sconf_get_now();

      if (queue_states != NULL) {
      
         lList *consumable_list = lGetList(queue, QU_consumable_config_list);
         lListElem *slot_elem   = lGetElemStr(consumable_list, CE_name, "slots"); 
         double slot_count      = lGetDouble(slot_elem, CE_doubleval); 

         lList *queue_uti_list = lGetList(queue, QU_resource_utilization);
         lListElem *slot_uti   = lGetElemStr(queue_uti_list, RUE_name, "slots");
         lList *slot_uti_list  = lGetList(slot_uti, RUE_utilized);
         
         lListElem *queue_state = NULL;     

         DPRINTF(("queue: %s time %d\n", lGetString(queue, QU_full_name), from));

         if (slot_uti_list == NULL) {
            slot_uti_list = lCreateList("slot_uti", RDE_Type);
            lSetList(slot_uti, RUE_utilized, slot_uti_list);
         }

         for_each(queue_state, queue_states) {
            bool is_full = (lGetUlong(queue_state, CQU_state) != QI_DO_NOTHING)?true:false;
            u_long32 till = lGetUlong(queue_state, CQU_till);
          
            /* check for now, and set it if it is now */
            if (is_full && (from == sconf_get_now())) {
               lSetDouble(slot_uti, RUE_utilized_now, slot_count);
            }
          
            set_utilization(slot_uti_list, from, till, is_full?slot_count:0);
            
            from = till;     
         } /* end for_each */

      }/* end if*/
   }
   
   DRETURN_VOID;
}

/****** scheduler/set_utilization() ********************************************
*  NAME
*     set_utilization() -- adds one specific calendar entry to the resource schedule
*
*  SYNOPSIS
*     static void set_utilization(lList *uti_list, u_long32 from, u_long32 
*     till, double uti) 
*
*  FUNCTION
*     This set utilization function is unique for calendars. It removes all other
*     uti settings in the given time interval and replaces it with the given one.
*
*  INPUTS
*     lList *uti_list - the uti list for a specifiy resource and queue
*     u_long32 from   - starting time for this uti
*     u_long32 till   - endtime for this uti.
*     double uti      - utilization (needs to bigger than 1 (schould be max)
*
*  NOTES
*     MT-NOTE: set_utilization() is MT safe 
*
*  SEE ALSO
*     scheduler/add_calendar_to_schedule
*     scheduler/newResourceElem
*     scheduler/prepare_resource_schedules
*******************************************************************************/
static void 
set_utilization(lList *uti_list, u_long32 from, u_long32 till, double uti)
{
   DENTER(TOP_LAYER, "set_utilization");

   if (uti > 0) {
      bool is_from_added = false;
      bool is_till_added = false;
      double past_uti = 0;
      lListElem *uti_elem_next = NULL;

      if (till == 0) {
         till = DISPATCH_TIME_QUEUE_END;
      }

      DPRINTF(("queue cal. schedule entry time %d till %d util: %f\n", from, till, uti));

      uti_elem_next = lFirst(uti_list);
     
      /* search for the starting point */
      while (uti_elem_next != NULL) {
         if (lGetUlong(uti_elem_next, RDE_time) > from) { /*insert before this elem */
            lInsertElem(uti_list, lPrev(uti_elem_next), newResourceElem(from, uti));
            past_uti = lGetDouble(uti_elem_next, RDE_amount);
            is_from_added = true; 
            break;
         }
         else if (lGetUlong(uti_elem_next, RDE_time) == from) { /* modify found elem */
            /* override utilization is maximun */
            past_uti = lGetDouble(uti_elem_next, RDE_amount);
            lSetDouble(uti_elem_next, RDE_amount, uti);
            is_from_added = true;
            break;
         }
         else { /* did not find it, continue */
            uti_elem_next = lNext(uti_elem_next);
         }
      }

      if (is_from_added) { /* searc for the endpoint */
          while (uti_elem_next != NULL) {
            if (lGetUlong(uti_elem_next, RDE_time) > till) { /*insert before this elem */
               lInsertElem(uti_list, lPrev(uti_elem_next), newResourceElem(till, past_uti));
               is_till_added = true; 
               break;
            }
            else if (lGetUlong(uti_elem_next, RDE_time) == till) { /* do not override utilization is maximun */
               is_till_added = true;
               break;
            }
            else { /* did not find it, remove the current elem and continue*/
               lListElem *next = lNext(uti_elem_next);
               past_uti = lGetDouble(uti_elem_next, RDE_amount);
               lRemoveElem(uti_list, &uti_elem_next);
               uti_elem_next = next;
            }
         }
      }
      else {
         lAppendElem(uti_list, newResourceElem(from, uti));
      }

      if (!is_till_added) {
         lAppendElem(uti_list, newResourceElem(till, 0));
      }   
   }

   DRETURN_VOID;
}


/****** scheduler/newResourceElem() ********************************************
*  NAME
*     newResourceElem() -- creates new resource schedule entry
*
*  SYNOPSIS
*     static lListElem* newResourceElem(u_long32 time, double amount) 
*
*  FUNCTION
*     creates new resource schedule entry and returns it
*
*  INPUTS
*     u_long32 time - specific time
*     double amount - the utilized amount
*
*  RESULT
*     static lListElem* - new resource schedule entry
*
*  NOTES
*     MT-NOTE: newResourceElem() is MT safe 
*
*  SEE ALSO
*     scheduler/add_calendar_to_schedule
*     scheduler/set_utilization
*     scheduler/prepare_resource_schedules
*******************************************************************************/
static lListElem *newResourceElem(u_long32 time, double amount) 
{
   lListElem *elem = NULL;

   elem = lCreateElem(RDE_Type);
   if (elem != NULL) {
      lSetUlong(elem, RDE_time, time);
      lSetDouble(elem, RDE_amount, amount);    
   }

   return elem;
}

/****** scheduler/prepare_resource_schedules() *********************************
*  NAME
*     prepare_resource_schedules() -- Debit non-pending jobs in resource schedule
*
*  SYNOPSIS
*     static void prepare_resource_schedules(const lList *running_jobs, const 
*     lList *suspended_jobs, lList *pe_list, lList *host_list, lList 
*     *queue_list, lList *centry_list, lList *rqs_list) 
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
*     lList *centry_list          - ??? 
*     lList *rqs_list            - configured resource quota sets
*
*  NOTES
*     MT-NOTE: prepare_resource_schedules() is not MT safe 
*******************************************************************************/
static void prepare_resource_schedules(const lList *running_jobs, const lList *suspended_jobs, 
   lList *pe_list, lList *host_list, lList *queue_list, lList *rqs_list, lList *centry_list, lList *acl_list, lList *hgroup_list)
{
   DENTER(TOP_LAYER, "prepare_resource_schedules");

   add_job_list_to_schedule(running_jobs, false, pe_list, host_list, queue_list, rqs_list, centry_list, acl_list, hgroup_list);
   add_job_list_to_schedule(suspended_jobs, true, pe_list, host_list, queue_list, rqs_list, centry_list, acl_list, hgroup_list);
   add_calendar_to_schedule(queue_list); 

#ifdef SGE_LOCK_DEBUG /* just for information purposes... */
   utilization_print_all(pe_list, host_list, queue_list); 
#endif   

   DRETURN_VOID;
}
