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
#include "sge_job_schedd.h"
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
                    bool is_reserve, bool is_schedule_based, lList **load_list, lList *hgrp_list, lList *rqs_list,
                    lList *ar_list);

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
         "|| %I m= %u)",
         dp,
         QU_state, QI_SUSPENDED,        /* only not suspended queues      */
         QU_state, QI_SUSPENDED_ON_SUBORDINATE,
         QU_state, QI_ERROR,            /* no queues in error state       */
         QU_state, QI_AMBIGUOUS,
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
   bool is_schedule_based = (max_reserve > 0) ? true: false;

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

   if (max_reserve != 0 || lGetNumberOfElem(lists->ar_list) > 0) {
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
                                 lists->hgrp_list, true);

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
   } else {
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

      if (ret != 0) {
         lFreeList(&user_list);
         lFreeList(&group_list);
         lFreeList(&job_load_adjustments);
         DRETURN(-1);
      }
   }

   if ( ((max_reserve == 0) && (lGetNumberOfElem(lists->queue_list) == 0)) || /* no reservation and no queues avail */
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
            if (sort_hostlist) {
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
            } else { 
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
                  is_schedule_based,
                  &consumable_load_list,
                  lists->hgrp_list,
                  lists->rqs_list,
                  lists->ar_list);
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
               if (job_count_pending_tasks(orig_job, true) < 2)
                  is_pjob_resort = false;
               else
                  is_pjob_resort = true;
               job_move_first_pending_to_running(&orig_job, splitted_job_lists);

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

         /* no more free queues - then exit */
         if (lGetNumberOfElem(lists->queue_list)==0) {
            break;
         }   

         if (is_pjob_resort) {
            sgeee_resort_pending_jobs(splitted_job_lists[SPLIT_PENDING]);
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
                    int *sort_hostlist, bool is_start,  bool is_reserve, bool is_schedule_based, lList **load_list, lList *hgrp_list,
                    lList *rqs_list, lList *ar_list) 
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
   a.rqs_list         = rqs_list;
   a.ar_list          = ar_list;

   /* in reservation scheduling mode a non-zero duration always must be defined */
   if ( !job_get_duration(&a.duration, job) ) {
      schedd_mes_add(a.job_id, SCHEDD_INFO_CKPTNOTFOUND_);
      assignment_release(&a);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   a.duration = duration_add_offset(a.duration, sconf_get_duration_offset());
   a.is_schedule_based = is_schedule_based;

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
         } else {
            result = DISPATCH_NEVER_CAT;
         }   
      }

   } else {
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

   /* when a job is started *now* the RUE_utilized_now must always change */
   /* if jobs with reservations are ahead then we also must change the RUE_utilized */ 
   if (result == DISPATCH_OK) {
      if (a.start == DISPATCH_TIME_NOW) {
         a.start = sconf_get_now();
      }   
      debit_scheduled_job(&a, sort_hostlist, orders, true, 
               SCHEDULING_RECORD_ENTRY_TYPE_STARTING, true);
   } else {
      debit_scheduled_job(&a, sort_hostlist, orders, false,  
            SCHEDULING_RECORD_ENTRY_TYPE_RESERVING, true);
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
      } else {
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
      } else {
         lAddList(*dis_queue_list, &disabled_queues);
      }        
      disabled_queues = NULL;
   }

   /* no longer needed - having built the order 
      and debited the job everywhere */
   assignment_release(&a);
   DRETURN(result);
}
