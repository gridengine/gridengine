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
#include <math.h>
#include <fnmatch.h>

#include "sge_conf.h"
#include "sge.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_schedd.h"
#include "sge_orders.h"
#include "scheduler.h"
#include "sgermon.h"
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_time.h"
#include "sge_parse_num_par.h"
#include "load_correction.h"
#include "schedd_monitor.h"
#include "suspend_thresholds.h"
#include "sge_sched.h"
#include "sge_feature.h"
#include "sgeee.h"
#include "interactive_sched.h"
#include "shutdown.h"
#include "schedd_message.h"
#include "sge_process_events.h"
#include "sge_access_tree.h"
#include "sge_category.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "jb_now.h"


/* the global list descriptor for all lists needed by the default scheduler */
sge_Sdescr_t lists =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int dispatch_jobs(sge_Sdescr_t *lists, lList **orderlist, lList **running_jobs, lList **finished_jobs);

static int select_assign_debit(lList **queue_list, lList **job_list, lListElem *job, lListElem *ja_task, lListElem *pe, lListElem *ckpt, lList *complex_list, lList *host_list, lList *acl_list, lList **user_list, lList **group_list, lList **orders_list, u_long *total_running_job_tickets, int ndispatched, int *dispatch_type);


/****** scheduler/scheduler() **************************************************
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
*
*******************************************************************************/
#ifdef SCHEDULER_SAMPLES
int my_scheduler(sge_Sdescr_t *lists)
{
   return scheduler(lists);
}
#endif
int scheduler( sge_Sdescr_t *lists ) {
   lList *orderlist=NULL;
   lList *running_jobs=NULL;
   lList *finished_jobs = NULL;
   int ret;
#ifdef TEST_DEMO
   FILE *fpdjp;
#endif

   DENTER(TOP_LAYER, "scheduler");
#ifdef TEST_DEMO
   fpdjp = fopen("/tmp/sge_debug_job_place.out", "a");
#endif

   schedd_initialize_messages();
   schedd_log_schedd_info(1);

   if (feature_is_enabled(FEATURE_SGEEE)) {  
      /* finished jobs are handled in sge_calc_tickets */

      /* split job list in running jobs and others */
      if (sge_split_job_finished(
            &(lists->job_list), /* source list                        */
            &finished_jobs,     /* put the finished jobs at this location */
            "finished jobs")) {
         DPRINTF(("couldn't split job list concerning finished state\n"));

         DEXIT;
         return -1;
      }
   }

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
         schedd_add_global_message(SCHEDD_INFO_QUEUENOTAVAIL_, lGetString(mes_queues, QU_qname));

      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESTEMPORARLYNOTAVAILABLEDROPPED , qlp, QU_qname);
      lFreeList(qlp);
      lFreeWhere(where);
      lFreeWhat(what);
   }

   ret = dispatch_jobs(lists, &orderlist, &running_jobs, &finished_jobs);
   lFreeList(running_jobs);
   lFreeList(finished_jobs);

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

   remove_immediate_jobs(lists->job_list, &orderlist);
   orderlist = sge_add_schedd_info(orderlist);

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

   schedd_release_messages();
   schedd_log_schedd_info(0);

   DEXIT;
   return 0;
}

/****** scheduler/dispatch_jobs() **********************************************
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
*
*******************************************************************************/
static int dispatch_jobs(
sge_Sdescr_t *lists,
lList **orderlist,
lList **running_jobs,
lList **finished_jobs 
) {
   lList *user_list=NULL, *group_list=NULL;
   lListElem *orig_job, *job, *queue, *cat;
   lList *susp_queues = NULL;
   lListElem *rjob, *rja_task;  
   u_long total_running_job_tickets=0; 
   int sge_mode = feature_is_enabled(FEATURE_SGEEE);
   int ndispatched = 0;
   int dipatch_type = DISPATCH_TYPE_NONE;
   lListElem *ja_task;
   int matched_pe_count = 0;
   int global_lc = 0;

   DENTER(TOP_LAYER, "dispatch_jobs");

   schedd_initialize_messages_joblist(NULL);

   /*---------------------------------------------------------------------
    * LOAD ADJUSTMENT
    *
    * Load sensors report load delayed. So we have to increase the load
    * of an exechost for each job started before load adjustment decay time.
    * load_adjustment_decay_time is a configuration value. 
    *---------------------------------------------------------------------*/
   
   if (scheddconf.load_adjustment_decay_time ) {
      correct_load(lists->job_list,
		   lists->queue_list,
		   &lists->host_list,
		   scheddconf.load_adjustment_decay_time);

      /* is there a "global" load value in the job_load_adjustments?
         if so this will make it necessary to check load thresholds
         of each queue after each dispatched job */
      {
         lListElem *gep, *lcep;
         if ((gep = lGetElemHost(lists->host_list, EH_name, "global"))) {
            for_each (lcep, scheddconf.job_load_adjustments) {
               char *attr = lGetString(lcep, CE_name);
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


   unsuspend_job_in_queues(lists->queue_list, lists->job_list, orderlist); 
   suspend_job_in_queues(susp_queues, lists->job_list, orderlist); 
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

   DPRINTF(("STARTING PASS 1 WITH %d JOBS\n", lGetNumberOfElem(lists->job_list)));

   /* split job list in running jobs and others */
   if (sge_split_job_running(
         &(lists->job_list),   /* source list                              */
         running_jobs,         /* put the running jobs at this location    */
         "running jobs")) {
      DPRINTF(("couldn't split job list in running and non running ones\n"));
      DEXIT;
      return -1;
   }

   queue=lFirst(lists->queue_list);
   if(!queue) {
      /* no queues to schedule on */

      if (sge_mode) {
         sge_scheduler(lists, *running_jobs, *finished_jobs, orderlist);
      }

      DPRINTF(("queues dropped because of overload or full: ALL\n"));
      schedd_add_global_message(SCHEDD_INFO_ALLALARMOVERLOADED_);

      DEXIT;
      return 0;
   }


   /* 
   ** filter pending jobs with maxujobs violation 
   */
   lists->job_list = filter_max_running_1step(
      lists->job_list,        /* pending jobs to be filtered                */ 
      *running_jobs,          /* running jobs to get number of running jobs */
      &user_list,             /* store here a list with the number of running jobs per user */ 
      scheddconf.maxujobs,    /* max. jobs per user or 0 if no limitations  */
      JB_owner);              /* take job owner for compare                 */ 

   /*  THE FOLLOWING STATEMENT IS NOT TRUE IN THE SGE CODESTREAM:
   ** running_jobs is not referenced after this point
   ** contains all jobs that are not idle, i.e. also snoozed jobs
   ** could be used as input for suspend threshold job filtering
   */

   /* 
   ** remove jobs that wait because of [-a time] option 
   */
   if (sge_split_job_wait_at_time(
      &(lists->job_list),  /* pending jobs to reduce            */
      NULL,                /* no location to store waiting jobs */
      NULL,                /* no name for list of waiting jobs  */ 
      sge_get_gmt())) {    /* actual time                       */
      DPRINTF(("couldn't split job list concerning at time\n"));
      DEXIT;
      return -1;
   } 

   /* 
   ** remove jobs in error state 
   */
   if (sge_split_job_error(&(lists->job_list), NULL, NULL)) {
      DPRINTF(("couldn't split job list concerning error\n"));
      DEXIT;
      return -1;
   }

   /* 
   ** remove jobs in hold state 
   */
   if (sge_split_job_hold(&(lists->job_list), NULL, NULL)) {
      DPRINTF(("couldn't split job list concerning hold\n"));
      DEXIT;
      return -1;
   }

   /* 
   ** remove jobs that wait for other jobs to exit 
   */
   if (sge_split_job_wait_predecessor(&(lists->job_list), NULL, NULL)) {
      DPRINTF(("couldn't split job list concerning wait for predecessor\n"));
      DEXIT;
      return -1;
   } 

   /* 
   ** remove jobs that can't get scheduled because of ckpt restrictions 
   */
   if (sge_split_job_ckpt_restricted(&(lists->job_list), NULL, NULL, 
         lists->ckpt_list)) {
      DPRINTF(("couldn't split job list concerning ckpt restrictions\n"));
      DEXIT;
      return -1;
   }

   /*--------------------------------------------------------------------
    * CALL SGE SCHEDULER TO
    * CALCULATE TICKETS FOR EACH JOB  - IN SUPPORT OF SGE
    *------------------------------------------------------------------*/

   if (sge_mode) {
      sge_scheduler(lists, *running_jobs, *finished_jobs, orderlist);
   }


   DPRINTF(("STARTING PASS 2 WITH %d JOBS\n", lGetNumberOfElem(lists->job_list)));
   if (lGetNumberOfElem(lists->job_list)==0) {
      /* no jobs to schedule */
      SCHED_MON((log_string, MSG_SCHEDD_MON_NOPENDJOBSTOPERFORMSCHEDULINGON ));
      lFreeList(user_list);
      lFreeList(group_list);
      DEXIT;
      return 0;
   }

   if (sge_mode) {
      /* calculate the number of tickets for all jobs already running on the host */
      if (calculate_host_tickets(running_jobs, &(lists->host_list)))  {
         DPRINTF(("no host for which to calculate host tickets\n"));
         lFreeList(user_list);
         lFreeList(group_list);
         DEXIT;
         return -1;
      }

      /* temporary job placement workaround - calc tickets for running and queued jobs */
      sge_calc_tickets(lists, lists->job_list, *running_jobs, NULL, 0);

      /* Order Jobs in descending order according to tickets and then job number */

      sge_sort_jobs(&(lists->job_list));

   }  /* if sge_mode */


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

      if (sge_mode) {
         sort_host_list(lists->host_list, lists->complex_list);
         sort_host_list_by_share_load(lists->host_list, lists->complex_list);

#ifdef TEST_DEMO

         for_each(host, lists->host_list) {
            fprintf(fpdjp, "after sort by share load - for running jobs only\n");
            fprintf(fpdjp, "For host %s :\n", lGetString(host, EH_name));
            fprintf(fpdjp, "EH_sge_tickets is %u\n", lGetUlong(host, EH_sge_tickets));
            fprintf(fpdjp, "EH_sge_ticket_pct is %f\n", lGetDouble(host, EH_sge_ticket_pct));
            fprintf(fpdjp, "EH_resource_capability_factor is %f\n", lGetDouble(host, EH_resource_capability_factor));
            fprintf(fpdjp, "EH_resource_capability_factor_pct is %f\n", lGetDouble(host, EH_resource_capability_factor_pct));
            fprintf(fpdjp, "EH_sort_value  is %f\n", lGetDouble(host, EH_sort_value));
            fprintf(fpdjp, "EH_sge_load_pct  is %f\n", lGetDouble(host, EH_sge_load_pct));
            fprintf(fpdjp, "share load value for host %s is %d\n", lGetString(host, EH_name), lGetUlong(host, EH_sge_load));
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
                 lGetString(host, EH_name), lGetDouble(host, EH_sort_value));

         if (sge_mode)  {
            fprintf(fpdjp, "EH_sge_tickets is %u\n", lGetUlong(host, EH_sge_tickets));
         }
       }

#endif

      break;
   }

   /*---------------------------------------------------------------------
    * DISPATCH JOBS TO QUEUES
    *---------------------------------------------------------------------*/
   /* Calculate the total number of tickets for running jobs - to be used
      later for SGE job placement */
   if (sge_mode && scheddconf.queue_sort_method == QSM_SHARE)  {
      total_running_job_tickets = 0;
      for_each(rjob, *running_jobs) {
         for_each(rja_task, lGetList(rjob, JB_ja_tasks)) {
            total_running_job_tickets += lGetUlong(rja_task, JAT_ticket);
         }
      }
   }

   /*
   ** job categories are reset here, we need an update of the rejected field
   ** for every new run
   */
   sge_reset_job_category(); 

   if (!sge_mode) {
      /* 
      ** establish hash table for all job arrays containing runnable tasks 
      ** whether a job is runnable 
      */
      at_notice_runnable_job_arrays(lists->job_list);
   }


   /*
   ** loop over the jobs that are left in priority order
   */
   while ( (job = lCopyElem(
   orig_job=(sge_mode? lFirst(lists->job_list) :at_get_actual_job_array())
   ))) {
      u_long32 job_id; 
      u_long32 ja_task_id; 
      lListElem *pe;
      char *pe_name;
      lListElem *ckpt;
      char *ckpt_name;
      int dispatched_a_job;

      schedd_initialize_messages_joblist(lists->job_list);

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
      if (sge_mode && sge_is_job_category_rejected(job)) {
         goto SKIP_THIS_JOB;
      }

      ja_task = lFirst(lGetList(job, JB_ja_tasks));
      if (!ja_task) {
         DPRINTF(("Found job "u32" with no job array tasks\n", job_id));
         goto SKIP_THIS_JOB;
      } else 
         ja_task_id = lGetUlong(ja_task, JAT_task_number);

      DPRINTF(("-----------------------------------------\n"));

      /*------------------------------------------------------------------ 
       * seek for ckpt interface definition requested by the job 
       * in case of ckpt jobs 
       *------------------------------------------------------------------*/
      ckpt = NULL;
      if ((ckpt_name = lGetString(job, JB_checkpoint_object))) {
         ckpt = lGetElemStr(lists->ckpt_list, CK_name, ckpt_name);
         if (!ckpt) {
            goto SKIP_THIS_JOB;
         }
      }

      /*------------------------------------------------------------------ 
       * seek for parallel environment requested by the job 
       * in case of parallel jobs 
       *------------------------------------------------------------------*/
      if ((pe_name = lGetString(job, JB_pe))) {
         for_each(pe, lists->pe_list) {
            if (fnmatch(pe_name, lGetString(pe, PE_name), 0)) {
               continue;
            } else {
               matched_pe_count++;
            }

            /* any objections from pe ? */
            if (pe_restricted(job, pe, lists->acl_list))
               continue;

            DPRINTF(("handling parallel job "u32"."u32"\n", job_id, 
               ja_task_id)); 
            if (select_assign_debit(
                  &(lists->queue_list), 
                  &(lists->job_list), 
                  job, ja_task, pe, ckpt, 
                  lists->complex_list, 
                  lists->host_list, 
                  lists->acl_list,
                  &user_list,
                  &group_list,
                  orderlist,
                  &total_running_job_tickets, 
                  ndispatched,
                  &dipatch_type))
               continue;

            ndispatched++;       
            dispatched_a_job = 1;
            DPRINTF(("Success to schedule job "u32"."u32" to pe \"%s\"\n",
               job_id, ja_task_id, lGetString(pe, PE_name)));
            break;
         }
         if ( matched_pe_count == 0 ) {
            DPRINTF(("NO PE MATCHED!\n"));
            schedd_add_message(job_id, SCHEDD_INFO_NOPEMATCH_ ); 
         }
      } else {
         DPRINTF(("handling job "u32"."u32"\n", job_id, ja_task_id)); 
         if (!select_assign_debit(
               &(lists->queue_list), 
               &(lists->job_list), 
               job, ja_task, NULL, ckpt, 
               lists->complex_list, 
               lists->host_list, 
               lists->acl_list,
               &user_list,
               &group_list,
               orderlist,
               &total_running_job_tickets, 
               ndispatched,
               &dipatch_type)) {
            ndispatched++;                  
            dispatched_a_job = 1;
         }
      }

SKIP_THIS_JOB:
      if (dispatched_a_job) {
         int ntasks;

         /* mark task as running in original job */
         lSetUlong(lFirst(lGetList(orig_job, JB_ja_tasks)), JAT_status, JRUNNING);

         /* move running task into running jobs list */
         ntasks = lGetNumberOfElem(lGetList(orig_job, JB_ja_tasks));

         sge_split_job_running(&(lists->job_list), running_jobs, "running jobs");

         /* after sge_split_job_running() orig_job can be removed and job should be used instead */
         orig_job = NULL;

         DPRINTF(("STILL %d of formerly %d tasks\n", ntasks-1, ntasks)); 

         /* notify access tree */
         if (!sge_mode) 
            at_dispatched_a_task(job, 1);
      } else {
         /* before deleting the element mark the category as rejected */
         cat = lGetRef(job, JB_category);
         if (cat) {
            DPRINTF(("SKIP JOB " u32 " of category '%s' (rc: "u32 ")\n", job_id, 
                        lGetString(cat, CT_str), lGetUlong(cat, CT_refcount))); 
            sge_reject_category(cat);
         }
      }

      /* prevent that we get the same job next time again */
      if (!dispatched_a_job || !lGetElemUlong(lists->job_list, JB_job_number, job_id)) {
         if (!sge_mode) 
            at_finished_array_dispatching(job);
         lDelElemUlong(&lists->job_list, JB_job_number, job_id); 
      }

      /* drop idle jobs that exceed maxujobs limit */
      /* should be done after resort_job() 'cause job is referenced */
      if (sge_mode /* || !set_user_sort(-1) */) {
         DPRINTF(("FILTER_MAX_RUNNING\n"));
         lists->job_list = filter_max_running(lists->job_list, user_list, 
               scheddconf.maxujobs, JB_owner);
      }

      lFreeElem(job);

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
               fprintf(fpdjp, "load value for host %s is %f\n", lGetString(host, EH_name), lGetDouble(host, EH_sort_value));
            }
            fprintf(fpdjp, " \n");
            fprintf(fpdjp, "QUEUE ORDER:\n");
            for_each(queue, lists->queue_list)  {
               fprintf(fpdjp, "  queue %s\n", lGetString(queue, QU_qname));
            }
        }
#endif

      if (dispatched_a_job && !(lGetNumberOfElem(lists->job_list)==0))  {
         if (sge_mode && scheddconf.queue_sort_method == QSM_SHARE)  {
            sort_host_list_by_share_load(lists->host_list, lists->complex_list);

#ifdef TEST_DEMO
            for_each(host, lists->host_list) {
               fprintf(fpdjp, "after sort by share load - for running and already placed new jobs \n");
               fprintf(fpdjp, "For host %s :\n", lGetString(host, EH_name));
               fprintf(fpdjp, "EH_sge_tickets is %u\n", lGetUlong(host, EH_sge_tickets));
               fprintf(fpdjp, "EH_sge_ticket_pct is %f\n", lGetDouble(host, EH_sge_ticket_pct));
               fprintf(fpdjp, "EH_resource_capability_factor is %f\n", 
                              lGetDouble(host, EH_resource_capability_factor));
               fprintf(fpdjp, "EH_resource_capability_factor_pct is %f\n", 
                              lGetDouble(host, EH_resource_capability_factor_pct));
               fprintf(fpdjp, "EH_sort_value  is %f\n", lGetDouble(host, EH_sort_value));
               fprintf(fpdjp, "EH_sge_load_pct  is %f\n", lGetDouble(host, EH_sge_load_pct));
               fprintf(fpdjp, "share load value for host %s is %d\n", 
                              lGetString(host, EH_name), lGetUlong(host, EH_sge_load));
            }
#endif

         } /* sge_mode && scheddconf.queue_sort_method == QSM_SHARE */
      } /* !lGetNumberOfElem(lists->job_list)==0 */

   } /* end of while */

   lFreeList(user_list);
   lFreeList(group_list);

#ifdef TEST_DEMO
   fclose(fpdjp);
#endif

   DEXIT;
   return 0;
}



/****** scheduler/select_assign_debit() ****************************************
*  NAME
*     select_assign_debit()
*
*  FUNCTION
*     Selects resources for 'job', add appropriate order to the 'orders_list',
*     debits resources of this job for the next dispatch and sort out no longer
*     available queues from the 'queue_list'.
*
*******************************************************************************/
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
u_long *total_running_job_tickets,
int ndispatched,
int *dispatch_type 
) {
   int sge_mode = feature_is_enabled(FEATURE_SGEEE);
   lListElem *hep;  
   u_long old_host_tickets;    
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
      dispatch_type);
            

   /* the following was added to support SGE - 
      account for job tickets on hosts due to parallel jobs */
   if (granted && sge_mode) {
      double job_tickets_per_slot, nslots;
      nslots = nslots_granted(granted, NULL);

      job_tickets_per_slot =(double)lGetUlong(ja_task, JAT_ticket)/nslots;

      for_each(granted_el, granted) {
         hep = lGetElemHost(host_list, EH_name, lGetString(granted_el, JG_qhostname));
         old_host_tickets = lGetUlong(hep, EH_sge_tickets);
         lSetUlong(hep, EH_sge_tickets, (old_host_tickets + 
               job_tickets_per_slot*lGetUlong(granted_el, JG_slots)));
         lSetUlong(granted_el, JG_ticket, job_tickets_per_slot*lGetUlong(granted_el, JG_slots));
      }
      *total_running_job_tickets += lGetUlong(ja_task, JAT_ticket);
   }

   /*------------------------------------------------------------------
    * BUILD ORDERS LIST THAT TRANSFERS OUR DECISIONS TO QMASTER
    *------------------------------------------------------------------*/
   if (!granted) {
      /* failed scheduling this job */
      if (JB_NOW_IS_IMMEDIATE(lGetUlong(job, JB_now))) /* immediate job */
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
         complex_list);

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

