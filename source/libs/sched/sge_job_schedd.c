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
#include <stdio.h>
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_pe.h"
#include "sge_job_schedd.h"
#include "sge_range_schedd.h"
#include "valid_queue_user.h"
#include "sge_parse_num_par.h"
#include "schedd_monitor.h"
#include "sge_sched.h"            /*added to support SGE*/
#include "schedd_message.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "cull_lerrnoP.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "sge_string.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_time.h"
#include "sge_userset.h"
#include "sge_centry.h"
#include "sge_schedd_conf.h"
#include "sge_qinstance.h"
#include "sge_gqueue.h"

#include "cull_hash.h"

#define IDLE 0

#ifdef WIN32NATIVE
#	define strcasecmp( a, b) stricmp( a, b)
#	define strncasecmp( a, b, n) strnicmp( a, b, n)
#endif

/****** sched/sge_job_schedd/get_name_of_split_value() ************************
*  NAME
*     get_name_of_split_value() -- Constant to name transformation 
*
*  SYNOPSIS
*     const char* get_name_of_split_value(int value) 
*
*  FUNCTION
*     This function transforms a constant value in its internal
*     name. (Used for debug output) 
*
*  INPUTS
*     int value - SPLIT_-Constant 
*
*  RESULT
*     const char* - string representation of 'value' 
*
*  SEE ALSO
*     sched/sge_job_schedd/SPLIT_-Constants 
*******************************************************************************/
const char *get_name_of_split_value(int value) 
{
   const char *name;
   switch (value) {
   case SPLIT_FINISHED:
      name = "SPLIT_FINISHED"; 
      break;
   case SPLIT_WAITING_DUE_TO_PREDECESSOR:
      name = "SPLIT_WAITING_DUE_TO_PREDECESSOR";
      break;
   case SPLIT_HOLD:
      name = "SPLIT_HOLD";
      break;
   case SPLIT_ERROR:
      name = "SPLIT_ERROR";
      break;
   case SPLIT_WAITING_DUE_TO_TIME:
      name = "SPLIT_WAITING_DUE_TO_TIME";
      break;
   case SPLIT_RUNNING:
      name = "SPLIT_RUNNING";
      break;
   case SPLIT_PENDING:
      name = "SPLIT_PENDING";
      break;
   case SPLIT_PENDING_EXCLUDED:
      name = "SPLIT_PENDING_EXCLUDED";
      break;
   case SPLIT_SUSPENDED:
      name = "SPLIT_SUSPENDED";
      break;
   case SPLIT_PENDING_EXCLUDED_INSTANCES:
      name = "SPLIT_PENDING_EXCLUDED_INSTANCES";
      break;   
   default:
      name = "undefined";
      break;
   }
   return name;
}

/****** sched/sge_job_schedd/job_move_first_pending_to_running() **************
*  NAME
*     job_move_first_pending_to_running() -- Move a job 
*
*  SYNOPSIS
*     void job_move_first_pending_to_running(lListElem **pending_job, 
*                                            lList **splitted_jobs[]) 
*
*  FUNCTION
*     Move the 'pending_job' from 'splitted_jobs[SPLIT_PENDING]'
*     into 'splitted_jobs[SPLIT_RUNNING]'. If 'pending_job' is an 
*     array job, than the first task (task id) will be moved into 
*     'pending_job[SPLIT_RUNNING]' 
*
*  INPUTS
*     lListElem **pending_job - Pointer to a pending job (JB_Type) 
*     lList **splitted_jobs[] - (JB_Type) array of job lists 
*
*  SEE ALSO
*     sched/sge_job_schedd/SPLIT_-Constants 
*     sched/sge_job_schedd/split_jobs()
*******************************************************************************/
void job_move_first_pending_to_running(lListElem **pending_job,
                                       lList **splitted_jobs[]) 
{
   lList *ja_task_list = NULL;      /* JAT_Type */
   lList *r_ja_task_list = NULL;    /* JAT_Type */
   lListElem *ja_task = NULL;       /* JAT_Type */
   lListElem *running_job = NULL;   /* JB_Type */
   u_long32 job_id;
   u_long32 ja_task_id;

   DENTER(TOP_LAYER, "job_move_first_pending_to_running");

   job_id = lGetUlong(*pending_job, JB_job_number);
   ja_task_list = lGetList(*pending_job, JB_ja_tasks);
   ja_task = lFirst(ja_task_list);
   
   /*
    * Create list for running jobs
    */
   if (*(splitted_jobs[SPLIT_RUNNING]) == NULL) {
      const lDescr *descriptor = lGetElemDescr(*pending_job);
      *(splitted_jobs[SPLIT_RUNNING]) = lCreateList("", descriptor);
   }
   else {
      running_job = lGetElemUlong(*(splitted_jobs[SPLIT_RUNNING]), 
                               JB_job_number, job_id);
   }
   /*
    * Create a running job if it does not exist aleady 
    */
   if (running_job == NULL) {
      lList *n_h_ids, *u_h_ids, *o_h_ids, *s_h_ids, *r_tasks;
      
      n_h_ids = u_h_ids = o_h_ids = s_h_ids = r_tasks = NULL;
      lXchgList(*pending_job, JB_ja_n_h_ids, &n_h_ids);
      lXchgList(*pending_job, JB_ja_u_h_ids, &u_h_ids);
      lXchgList(*pending_job, JB_ja_o_h_ids, &o_h_ids);
      lXchgList(*pending_job, JB_ja_s_h_ids, &s_h_ids);
      lXchgList(*pending_job, JB_ja_tasks, &r_tasks);
      running_job = lCopyElem(*pending_job);
      lXchgList(*pending_job, JB_ja_n_h_ids, &n_h_ids);
      lXchgList(*pending_job, JB_ja_u_h_ids, &u_h_ids);
      lXchgList(*pending_job, JB_ja_o_h_ids, &o_h_ids);
      lXchgList(*pending_job, JB_ja_s_h_ids, &s_h_ids);
      lXchgList(*pending_job, JB_ja_tasks, &r_tasks);
      lAppendElem(*(splitted_jobs[SPLIT_RUNNING]), running_job); 
   } 

   /* 
    * Create an array instance and add it to the running job
    * or move the existing array task into the running job 
    */
   if (ja_task == NULL) {
      lList *n_h_ids = NULL;        /* RN_Type */

      n_h_ids = lGetList(*pending_job, JB_ja_n_h_ids);
      ja_task_id = range_list_get_first_id(n_h_ids, NULL);
      ja_task = job_search_task(*pending_job, NULL, ja_task_id);
      /* JG: TODO: do we need the ja_task instance here or can we
       *           wait until the JATASK_ADD event arrives from qmaster?
       *           The function should work on a copy if the job list.
       *           The event from qmaster has effect on the mirrored lists.
       *           So the code should be ok.
       */
      if(ja_task == NULL) {
         ja_task = job_create_task(*pending_job, NULL, ja_task_id);
         
      }
      ja_task_list = lGetList(*pending_job, JB_ja_tasks);
   }
  
   /*
    * Create an array task list if necessary
    */
   r_ja_task_list = lGetList(running_job, JB_ja_tasks); 
   if (r_ja_task_list == NULL) {
      r_ja_task_list = lCreateList("", lGetElemDescr(ja_task));
      lSetList(running_job, JB_ja_tasks, r_ja_task_list);
   }
  
   lDechainElem(ja_task_list, ja_task);
   lAppendElem(r_ja_task_list, ja_task); 
   
   /*
    * Remove pending job if there are no pending tasks anymore
    */
   if (!job_has_tasks(*pending_job) || 
       lGetList(*pending_job, JB_ja_tasks) == NULL) { 
      lDechainElem(*(splitted_jobs[SPLIT_PENDING]), *pending_job);
      *pending_job = lFreeElem(*pending_job); 
   }

#if 0 /* EB: DEBUG */
   job_lists_print(splitted_jobs);
#endif

   DEXIT;
}

/****** sched/sge_job_schedd/user_list_init_jc() ******************************
*  NAME
*     user_list_init_jc() -- inc. the # of jobs a user has running 
*
*  SYNOPSIS
*     void user_list_init_jc(lList **user_list, 
*                            const lList *running_list) 
*
*  FUNCTION
*     Initialize "user_list" and JC_jobs attribute for each user according
*     to the list of running jobs.
*
*  INPUTS
*     lList **user_list          - JC_Type list 
*     const lList *running_list - JB_Type list 
*
*  RESULT
*     void - None
*******************************************************************************/
void user_list_init_jc(lList **user_list, lList **splitted_job_lists[])
{
   lListElem *job;   /* JB_Type */

   if (splitted_job_lists[SPLIT_RUNNING] != NULL) {
      for_each(job, *(splitted_job_lists[SPLIT_RUNNING])) {
         sge_inc_jc(user_list, lGetString(job, JB_owner), 
                    job_get_ja_tasks(job));
      }
   }
   if (splitted_job_lists[SPLIT_SUSPENDED] != NULL) {
      for_each(job, *(splitted_job_lists[SPLIT_SUSPENDED])) {
         sge_inc_jc(user_list, lGetString(job, JB_owner), 
                    job_get_ja_tasks(job));
      }
   }
}

/****** sched/sge_job_schedd/job_lists_split_with_reference_to_max_running() **
*  NAME
*     job_lists_split_with_reference_to_max_running() 
*
*  SYNOPSIS
*     void job_lists_split_with_reference_to_max_running(
*              lList **job_lists[], 
*              lList **user_list, 
*              const char* user_name,
*              int max_jobs_per_user) 
*
*  FUNCTION
*     Move those jobs which would exceed the configured 
*     'max_u_jobs' limit (schedd configuration) from 
*     job_lists[SPLIT_PENDING] into job_lists[SPLIT_PENDING_EXCLUDED]. 
*     Only the jobs of the given 'user_name' will be handled. If
*     'user_name' is NULL than all jobs will be handled whose job owner 
*     is mentioned in 'user_list'.
*
*  INPUTS
*     lList **job_lists[]   - Array of JB_Type lists 
*     lList **user_list     - User list of Type JC_Type 
*     const char* user_name - user name
*     int max_jobs_per_user - "max_u_jobs" 
*
*  NOTE
*     JC_jobs of the user elements contained in "user_list" has to be 
*     initialized properly before this function might be called.
*
*  SEE ALSO
*     sched/sge_job_schedd/SPLIT_-Constants
*     sched/sge_job_schedd/trash_splitted_jobs()
*     sched/sge_job_schedd/split_jobs()     
*     sched/sge_job_schedd/user_list_init_jc()
*******************************************************************************/
void job_lists_split_with_reference_to_max_running(lList **job_lists[],
                                                   lList **user_list,
                                                   const char* user_name,
                                                   int max_jobs_per_user)
{
   DENTER(TOP_LAYER, "job_lists_split_with_reference_to_max_running");
   if (max_jobs_per_user != 0 && 
       job_lists[SPLIT_PENDING] != NULL && 
       *(job_lists[SPLIT_PENDING]) != NULL &&
       job_lists[SPLIT_PENDING_EXCLUDED] != NULL) {
      lListElem *user = NULL;
      lListElem *next_user = NULL;

#ifndef CULL_NO_HASH
      /* 
       * create a hash table on JB_owner to speedup 
       * searching for jobs of a specific owner
       */
      cull_hash_new_check(*(job_lists[SPLIT_PENDING]), JB_owner, 0);
#endif      

      if (user_name == NULL) {
         next_user = lFirst(*user_list);
      } else {
         next_user = lGetElemStr(*user_list, JC_name, user_name);
      }
      while ((user = next_user) != NULL) {
         u_long32 jobs_for_user = lGetUlong(user, JC_jobs);
         const char *jc_user_name = lGetString(user, JC_name);

         if (user_name == NULL) {
            next_user = lNext(user);
         } else {
            next_user = NULL;
         }
         if (jobs_for_user >= max_jobs_per_user) {
            const void *user_iterator = NULL;
            lListElem *user_job = NULL;         /* JB_Type */
            lListElem *next_user_job = NULL;    /* JB_Type */

            DPRINTF(("USER %s reached limit of %d jobs\n", jc_user_name, 
                     max_jobs_per_user));
            next_user_job = lGetElemStrFirst(*(job_lists[SPLIT_PENDING]), 
                                             JB_owner, jc_user_name, 
                                             &user_iterator);
            while ((user_job = next_user_job)) {
               next_user_job = lGetElemStrNext(*(job_lists[SPLIT_PENDING]), 
                                               JB_owner, jc_user_name, 
                                               &user_iterator);
               if (monitor_next_run) {
                  schedd_mes_add(lGetUlong(user_job, JB_job_number),
                                     SCHEDD_INFO_USRGRPLIMIT_);
               }

               lDechainElem(*(job_lists[SPLIT_PENDING]), user_job);
               if (*(job_lists[SPLIT_PENDING_EXCLUDED]) == NULL) {
                  lDescr *descr = user_job->descr;
                  int pos = lGetPosInDescr(descr, JB_owner);
        
                  if (pos >= 0) {
                     if (descr[pos].ht != NULL)  {
                        FREE(descr[pos].ht);
                     }
                  }
                  *(job_lists[SPLIT_PENDING_EXCLUDED]) =
                                      lCreateList("", descr);
               }

               lAppendElem(*(job_lists[SPLIT_PENDING_EXCLUDED]), user_job);
            }
         }
      } 
   }
   DEXIT;
}      

/****** sched/sge_job_schedd/split_jobs() *************************************
*  NAME
*     split_jobs() -- Split list of jobs according to their state
*
*  SYNOPSIS
*     void split_jobs(lList **job_list, lList **answer_list, 
*                     lList *queue_list, u_long32 max_aj_instances, 
*                     lList **result_list[]) 
*
*  FUNCTION
*     Split a list of jobs according to their state. 
*     'job_list' is the input list of jobs. The jobs in this list 
*     have different job states. For the dispatch algorithm only
*     those jobs are of interest which are really pending. Jobs
*     which are pending and in error state or jobs which have a
*     hold applied (start time in future, administrator hold, ...)
*     are not necessary for the dispatch algorithm. 
*     After a call to this function the jobs of 'job_list' may
*     have been moved into one of the 'result_list's. 
*     Each of those lists containes jobs which have a certain state.
*     (e.g. result_list[SPLIT_WAITING_DUE_TO_TIME] will contain
*     all jobs which have to wait according to their start time. 
*     'queue_list' is the whole list of configured queues.
*     'max_aj_instances' are the maximum number of tasks of an 
*     array job which may be instantiated at the same time.
*     'queue_list' and 'max_aj_instances' are used for the
*     split decitions. 
*     In case of any error the 'answer_list' will be used to report
*     errors (It is not used in the moment)
*
*  INPUTS
*     lList **job_list          - JB_Type input list 
*     lList **answer_list       - AN_Type answer list 
*     lList *queue_list         - QU_Type list 
*     u_long32 max_aj_instances - max. num. of task instances 
*     lList **result_list[]     - Array of result list (JB_Type)
*
*  NOTES
*     In former versions of SGE/EE we had 8 split functions.
*     Each of those functions walked twice over the job list.
*     This was time consuming in case of x thousand of jobs. 
*
*     We tried to improve this:
*        - loop over all jobs only once
*        - minimize copy operations where possible
*
*     Unfortunately this function id heavy to understand now. Sorry! 
*
*  SEE ALSO
*     sched/sge_job_schedd/SPLIT_-Constants 
*     sched/sge_job_schedd/trash_splitted_jobs()
*     sched/sge_job_schedd/job_lists_split_with_reference_to_max_running()
*******************************************************************************/
void split_jobs(lList **job_list, lList **answer_list,
                lList *queue_list, u_long32 max_aj_instances, 
                lList **result_list[])
{
#if 0 /* EB: DEBUG: enable debug messages for split_jobs() */
#define JOB_SPLIT_DEBUG
#endif
   lListElem *job = NULL;
   lListElem *next_job = NULL;
   lListElem *previous_job = NULL;
   DENTER(TOP_LAYER, "split_jobs");

   next_job = lFirst(*job_list);
   while ((job = next_job)) {
      lList *ja_task_list = NULL;
      lList *n_h_ids = NULL;
      lList *excluded_n_h_ids = NULL;
      lList *u_h_ids = NULL;
      lList *o_h_ids = NULL;
      lList *s_h_ids = NULL;
      lList *target_tasks[SPLIT_LAST];
      lListElem *target_job[SPLIT_LAST];
      lList *target_ids = NULL;
      u_long32 target_for_ids = SPLIT_LAST;
      lListElem *ja_task = NULL;
      lListElem *next_ja_task = NULL;
      u_long32 task_instances;
      int i, move_job;
#ifdef JOB_SPLIT_DEBUG
      u_long32 job_id = lGetUlong(job, JB_job_number);
#endif

      previous_job = lPrev(job);
      next_job = lNext(job);

      /*
       * Initialize
       */
      for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
         target_job[i] = NULL;
         target_tasks[i] = NULL;
      }
      task_instances = lGetNumberOfElem(lGetList(job, JB_ja_tasks)); 

      /*
       * Remove all ballast for a minimal copy operation of the job
       */
      lXchgList(job, JB_ja_tasks, &ja_task_list);
      lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
      lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
      lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
      lXchgList(job, JB_ja_s_h_ids, &s_h_ids);

      /*
       * Split enrolled tasks
       */
#ifdef JOB_SPLIT_DEBUG
      DPRINTF(("Split enrolled tasks for job "u32":\n", job_id));
#endif
      next_ja_task = lFirst(ja_task_list);
      while ((ja_task = next_ja_task)) {
         u_long32 ja_task_status = lGetUlong(ja_task, JAT_status);
         u_long32 ja_task_state = lGetUlong(ja_task, JAT_state);
         u_long32 ja_task_hold = lGetUlong(ja_task, JAT_hold);
         lList **target = NULL;
#ifdef JOB_SPLIT_DEBUG
         u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
#endif
         next_ja_task = lNext(ja_task);

#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Task "u32": status="u32" state="u32"\n", ja_task_id,
                     ja_task_status, ja_task_state));
#endif

         /*
          * Check the state of the task
          * (ORDER IS IMPORTANT!)
          */
         if (target == NULL && result_list[SPLIT_FINISHED] && 
             (ja_task_status & JFINISHED)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in finished state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_FINISHED]);
         } 

         if (target == NULL && result_list[SPLIT_ERROR] && 
             (ja_task_state & JERROR)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in error state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_ERROR]);
         } 
         if (target == NULL && result_list[SPLIT_WAITING_DUE_TO_TIME] &&
             (lGetUlong(job, JB_execution_time) > sge_get_gmt()) &&
             (ja_task_status == JIDLE)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is waiting due to time.\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_WAITING_DUE_TO_TIME]);
         }
         if (target == NULL && result_list[SPLIT_WAITING_DUE_TO_PREDECESSOR] &&
             (lGetList(job, JB_jid_predecessor_list) != NULL) &&
             (ja_task_status == JIDLE)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is waiting due to pred.\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_WAITING_DUE_TO_PREDECESSOR]);
         }
         if (target == NULL && result_list[SPLIT_PENDING] && 
             (ja_task_status == JIDLE) &&
             !(ja_task_hold & MINUS_H_TGT_ALL)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in pending state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_PENDING]);
         } 
         if (target == NULL && result_list[SPLIT_SUSPENDED]) {
            if ((ja_task_state & JSUSPENDED) ||
                (ja_task_state & JSUSPENDED_ON_THRESHOLD)) {
#ifdef JOB_SPLIT_DEBUG
               DPRINTF(("Task "u32" is in suspended state\n", ja_task_id));
#endif
               target = &(target_tasks[SPLIT_SUSPENDED]);
            } else {
               /*
                * Jobs in suspended queues are not in suspend state.
                * Therefore we have to take this info from the queue state.
                */
               if (gqueue_is_suspended( 
                        lGetList(ja_task, JAT_granted_destin_identifier_list),
                        queue_list)) {
#ifdef JOB_SPLIT_DEBUG
                  DPRINTF(("Task "u32" is in suspended state\n",ja_task_id));
#endif
                  target = &(target_tasks[SPLIT_SUSPENDED]);
               }
            }
         } 
         if (target == NULL && result_list[SPLIT_RUNNING] && 
             ja_task_status != JIDLE) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in running state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_RUNNING]);
         } 
         if (target == NULL && result_list[SPLIT_HOLD] &&
             (ja_task_hold & MINUS_H_TGT_ALL)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in hold state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_HOLD]);
         } 
#ifdef JOB_SPLIT_DEBUG
         if (target == NULL) {
            ERROR((SGE_EVENT, "Task "u32" has no known state: "
                   "status="u32" state="u32"\n",
                   ja_task_id, ja_task_status, ja_task_state));  
         }
#endif

         /* 
          * Move the task into the target list
          */
         if (target != NULL) {
            if (*target == NULL) {
               *target = lCreateList(NULL, lGetElemDescr(ja_task));
            }
            lDechainElem(ja_task_list, ja_task);
            lAppendElem(*target, ja_task);
         }
      }

      if (target_for_ids == SPLIT_LAST &&
          result_list[SPLIT_WAITING_DUE_TO_PREDECESSOR] &&
          lGetList(job, JB_jid_predecessor_list) != NULL) {
#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Unenrolled tasks are waiting for pred. jobs\n"));
#endif
         target_for_ids = SPLIT_WAITING_DUE_TO_PREDECESSOR;
         target_ids = n_h_ids;
         n_h_ids = NULL;
      }
      if (target_for_ids == SPLIT_LAST &&
          result_list[SPLIT_WAITING_DUE_TO_TIME] &&
          lGetUlong(job, JB_execution_time) > sge_get_gmt()) {
#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Unenrolled tasks are waiting due to time\n"));
#endif
         target_for_ids = SPLIT_WAITING_DUE_TO_TIME;
         target_ids = n_h_ids;
         n_h_ids = NULL;
      }
      if (target_for_ids == SPLIT_LAST &&
          result_list[SPLIT_PENDING_EXCLUDED_INSTANCES] &&
          max_aj_instances > 0) {
         excluded_n_h_ids = n_h_ids;
         n_h_ids = NULL;
         if (task_instances < max_aj_instances) {
            u_long32 allowed_instances = max_aj_instances - task_instances;

            range_list_move_first_n_ids(&excluded_n_h_ids, NULL, &n_h_ids,
                                        allowed_instances);
         }
         target_for_ids = SPLIT_PENDING_EXCLUDED_INSTANCES;
         target_ids = excluded_n_h_ids;
      }

      /*
       * Copy/Move and insert job into the target lists
       */
      move_job = 1;
      for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
         if ((target_tasks[i] != NULL) ||
             (i == target_for_ids && target_ids != NULL) ||
             (i == SPLIT_PENDING && n_h_ids != NULL ) ||
             (i == SPLIT_HOLD && (u_h_ids != NULL ||
                                         o_h_ids != NULL || s_h_ids != NULL))) {
            if (*(result_list[i]) == NULL) {
               const lDescr *reduced_decriptor = lGetElemDescr(job);

#ifdef JOB_SPLIT_DEBUG               
               DPRINTF(("Create "SFN"-list\n", get_name_of_split_value(i)));
#endif
               *(result_list[i]) = lCreateList("", reduced_decriptor);
            } 
            if (move_job == 1) {
#ifdef JOB_SPLIT_DEBUG
               DPRINTF(("Reuse job element "u32" for "SFN"-list\n", 
                        lGetUlong(job, JB_job_number), 
                        get_name_of_split_value(i)));
#endif
               move_job = 0;
               lDechainElem(*job_list, job);
               target_job[i] = job;
            } else {
#ifdef JOB_SPLIT_DEBUG
               DPRINTF(("Copy job element "u32" for "SFN"-list\n", 
                        lGetUlong(job, JB_job_number), 
                        get_name_of_split_value(i)));
#endif
               target_job[i] = lCopyElem(job);
            }
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Add job element "u32" into "SFN"-list\n",
                     lGetUlong(target_job[i], JB_job_number),
                     get_name_of_split_value(i))); 
#endif
            lAppendElem(*(result_list[i]), target_job[i]);
         }
      }

      /*
       * Do we have remaining tasks which won't fit into the target lists?
       */
      if ((lGetNumberOfElem(ja_task_list) > 0) ||
          (result_list[SPLIT_PENDING] == NULL && n_h_ids != NULL) ||
          (result_list[SPLIT_HOLD] == NULL && 
                   (u_h_ids != NULL || o_h_ids != NULL || s_h_ids != NULL))) {
         if (move_job == 0) {
            /* 
             * We moved 'job' into a target list therefore it is necessary 
             * to create a new job.
             */ 
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Put the remaining tasks into the initial container\n"));
#endif
            job = lCopyElem(job);
            lInsertElem(*job_list, previous_job, job);
         }
      } else {
         job = NULL;
      } 

      /* 
       * Insert array task information for not enrolled tasks
       */
      if (result_list[target_for_ids] != NULL && target_ids != NULL) {
#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Move not enrolled %s tasks\n",
                  get_name_of_split_value(target_for_ids)));
#endif
         lXchgList(target_job[target_for_ids], JB_ja_n_h_ids, &target_ids);
      }
      if (result_list[SPLIT_PENDING] != NULL && n_h_ids != NULL) {
#ifdef JOB_SPLIT_DEBUG 
         DPRINTF(("Move not enrolled pending tasks\n"));
#endif
         lXchgList(target_job[SPLIT_PENDING], JB_ja_n_h_ids, &n_h_ids);
      }
      if (result_list[SPLIT_HOLD] != NULL && 
          (u_h_ids != NULL || o_h_ids != NULL || s_h_ids != NULL)) {
#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Move not enrolled hold tasks\n"));
#endif
         lXchgList(target_job[SPLIT_HOLD], JB_ja_u_h_ids, &u_h_ids);
         lXchgList(target_job[SPLIT_HOLD], JB_ja_o_h_ids, &o_h_ids);
         lXchgList(target_job[SPLIT_HOLD], JB_ja_s_h_ids, &s_h_ids);
      }
      for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
         if (target_tasks[i] != NULL) {
#ifdef JOB_SPLIT_DEBUG 
            DPRINTF(("Put "SFQ"-tasks into job\n", get_name_of_split_value(i)));
#endif
            lSetList(target_job[i], JB_ja_tasks, target_tasks[i]);
         }
      }
      
      /* 
       * Put remaining tasks into job 
       */
      if (job) {
#ifdef JOB_SPLIT_DEBUG
         DPRINTF(("Put unenrolled tasks back into initial container\n"));
#endif
         lXchgList(job, JB_ja_tasks, &ja_task_list);
         lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
         lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
         lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
         lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
      } else {
         ja_task_list = lFreeList(ja_task_list);
      }
   }

   /*
    * Could we dispense all jobs?
    */
   if (lGetNumberOfElem(*job_list) == 0) {
      *job_list = lFreeList(*job_list);
   }

   DEXIT;
   return;
} 

/****** sched/sge_job_schedd/trash_splitted_jobs() ****************************
*  NAME
*     trash_splitted_jobs() -- Trash all not needed job lists
*
*  SYNOPSIS
*     void trash_splitted_jobs(lList **splitted_job_lists[]) 
*
*  FUNCTION
*     Trash all job lists which are not needed for scheduling decisions.
*     Before jobs and lists are trashed, scheduling messages will
*     be generated. 
*
*     Following lists will be trashed:
*        splitted_job_lists[SPLIT_ERROR]
*        splitted_job_lists[SPLIT_HOLD]
*        splitted_job_lists[SPLIT_WAITING_DUE_TO_TIME]
*        splitted_job_lists[SPLIT_WAITING_DUE_TO_PREDECESSOR]
*        splitted_job_lists[SPLIT_PENDING_EXCLUDED_INSTANCES]
*        splitted_job_lists[SPLIT_PENDING_EXCLUDED]
*
*  INPUTS
*     lList **splitted_job_lists[] - list of job lists 
*
*  SEE ALSO
*     sched/sge_job_schedd/SPLIT_-Constants 
*     sched/sge_job_schedd/split_jobs() 
*     sched/sge_job_schedd/job_lists_split_with_reference_to_max_running()
*******************************************************************************/
void trash_splitted_jobs(lList **splitted_job_lists[]) 
{
   int split_id_a[] = {
      SPLIT_ERROR, 
      SPLIT_HOLD, 
      SPLIT_WAITING_DUE_TO_TIME,
      SPLIT_WAITING_DUE_TO_PREDECESSOR,
      SPLIT_PENDING_EXCLUDED_INSTANCES,
      SPLIT_PENDING_EXCLUDED,
      SPLIT_LAST
   }; 
   int i = -1;

   while (split_id_a[++i] != SPLIT_LAST) { 
      lList **job_list = splitted_job_lists[split_id_a[i]];
      lListElem *job = NULL;
      int is_first_of_category = 1;

      for_each (job, *job_list) {
         u_long32 job_id = lGetUlong(job, JB_job_number);

         switch (split_id_a[i]) {
         case SPLIT_ERROR:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_JOBINERROR_);
            }
            if (monitor_next_run) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDERRORSTATEREACHED, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_HOLD:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_JOBHOLD_);
            }
            if (monitor_next_run) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEOFXHOLD, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_WAITING_DUE_TO_TIME:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_EXECTIME_);
            }
            if (monitor_next_run) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDEXECUTIONTIMENOTREACHED, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_WAITING_DUE_TO_PREDECESSOR:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_JOBDEPEND_);
            }
            if (monitor_next_run) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEDEPENDENCIES, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_PENDING_EXCLUDED_INSTANCES:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_MAX_AJ_INSTANCES_);
            }
            break;
         case SPLIT_PENDING_EXCLUDED:
            if (is_first_of_category) {
               schedd_mes_add(job_id, SCHEDD_INFO_USRGRPLIMIT_);
            }
            break;
         default:
            ;
         }
         if (is_first_of_category) {
            is_first_of_category = 0;
            schedd_mes_commit(*job_list, 1);
         } 
      }
      *job_list = lFreeList(*job_list);
   }
} 

void job_lists_print(lList **job_list[]) 
{
   lListElem *job;
   int i;

   DENTER(TOP_LAYER, "job_lists_print");

   for (i = SPLIT_FIRST; i < SPLIT_LAST; i++) {
      u_long32 ids = 0;

      if (job_list[i] && *(job_list[i])) {
         for_each(job, *(job_list[i])) {
            ids += job_get_enrolled_ja_tasks(job);
            ids += job_get_not_enrolled_ja_tasks(job);
         }
         DPRINTF(("job_list[%s] CONTAINES "u32" JOB(S) ("u32" TASK(S))\n",
            get_name_of_split_value(i),
            lGetNumberOfElem(*(job_list[i])), ids));
      }
   } 

   DEXIT;
   return;
} 

lSortOrder *sge_job_sort_order(
const lDescr *dp 
) {
   lSortOrder *so;

   DENTER(TOP_LAYER, "sge_job_sort_order");

   so = lParseSortOrderVarArg(dp, "%I-%I+%I+",
      JB_priority,       /* higher priority is better   */
      JB_submission_time,/* first come first serve      */
      JB_job_number      /* prevent job 13 being scheduled before 12 in case sumission times are equal */
   );

   DEXIT;
   return so;
}


/* jcpp: JC_Type */
void sge_dec_jc(lList **jcpp, const char *name, int slots) 
{
   int n = 0;
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_dec_jc");

   ep = lGetElemStr(*jcpp, JC_name, name);
   if (ep) {
      n = lGetUlong(ep, JC_jobs) - slots;
      if (n <= 0)
         lDelElemStr(jcpp, JC_name, name);
      else
         lSetUlong(ep, JC_jobs, n);
   }

   DEXIT;
   return;
}

/* jcpp: JC_Type */
void sge_inc_jc(lList **jcpp, const char *name, int slots) 
{
   int n = 0;
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_inc_jc");

   ep = lGetElemStr(*jcpp, JC_name, name);
   if (ep) 
      n = lGetUlong(ep, JC_jobs);
   else 
      ep = lAddElemStr(jcpp, JC_name, name, JC_Type);

   n += slots;

   lSetUlong(ep, JC_jobs, n);

   DEXIT;
   return;
}

/* 
   This func has to be called each time the number of 
   running jobs has changed 
   It also has to be called in our event layer when 
   new jobs have arrived or the priority of a job 
   has changed.

   jc       - job counter list - JC_Type
   job_list - job list to be sorted - JB_Type
   owner    - name of owner whose number of running jobs has changed

*/
int resort_jobs(lList *jc, lList *job_list, const char *owner, lSortOrder *so) 
{
   DENTER(TOP_LAYER, "resort_jobs");


   if (sconf_get_user_sort()) {
      int njobs;
      lListElem *job, *jc_owner;

      /* get number of running jobs of this user */
      if (owner) {
         lListElem *next_job;
         const void *iterator = NULL;

         jc_owner = lGetElemStr(jc, JC_name, owner);
         njobs = jc_owner ? lGetUlong(jc_owner, JC_jobs) : 0;

#ifndef CULL_NO_HASH
      /* create a hash table on JB_owner to speedup 
       * searching for jobs of a specific owner
       */
      cull_hash_new_check(job_list, JB_owner, 0);
#endif      
         next_job = lGetElemStrFirst(job_list, JB_owner, owner, &iterator);
         while((job = next_job) != NULL) {
            next_job = lGetElemStrNext(job_list, JB_owner, owner, &iterator);
            lSetUlong(job, JB_nrunning, njobs);
         }
      } else { /* update JB_nrunning for all jobs */
         for_each(job, job_list) {
            jc_owner = lGetElemStr(jc, JC_name, lGetString(job, JB_owner));
            njobs = jc_owner ? lGetUlong(jc_owner, JC_jobs) : 0;
            lSetUlong(job, JB_nrunning, njobs);
         }
      }
   }

   lSortList(job_list, so);
#if 0
   trace_job_sort(job_list);
#endif

   DEXIT;
   return 0;

}

void print_job_list(
lList *job_list /* JB_Type */
) {
   lListElem *job, *task;
   int jobs_exist = 0;
   int jobs = 0;

   DENTER(TOP_LAYER, "print_job_list");
    
   if (job_list && (jobs = lGetNumberOfElem(job_list)) > 0) {
      DPRINTF(("Jobs in list: %ld\n", jobs));
      for_each(job, job_list) {
         DPRINTF(("Job: %ld\n", lGetUlong(job, JB_job_number)));
         for_each(task, lGetList(job, JB_ja_tasks)) {
            DPRINTF(("Task: %ld Status: %ld State: %ld\n",
               lGetUlong(task, JAT_task_number), lGetUlong(task, JAT_status), lGetUlong(task, JAT_state)));
            jobs_exist = 1;
         }
      }
   } else {
      DPRINTF(("NO JOBS IN LIST\n"));
   }
   DEXIT;
}

void trace_job_sort(
lList *job_list 
) {
   lListElem *job;

   DENTER(TOP_LAYER, "trace_job_sort");

   for_each (job, job_list) {
      DPRINTF(("JOB "u32" %d %s %d "u32"\n",
         lGetUlong(job, JB_job_number),
         (int)lGetUlong(job, JB_priority) - BASE_PRIORITY,
         lGetString(job, JB_owner),
         lGetUlong(job, JB_nrunning),
         lGetUlong(job, JB_submission_time)));
   }

   DEXIT;
   return;
}


/*---------------------------------------------------------*
 *
 *  job has transited from non RUNNING to RUNNING
 *  we debit this job to users 'running job account' in 
 *  user list 'ulpp' and use the given sort order 'so'
 *  to resort complete job list 
 *---------------------------------------------------------*/
int up_resort(
lList **ulpp,    /* JC_Type */
lListElem *job,  /* JB_Type */
lList *job_list, /* JB_Type */
lSortOrder *so 
) {
   sge_inc_jc(ulpp, lGetString(job, JB_owner), 1);
   resort_jobs(*ulpp, job_list, lGetString(job, JB_owner), so);
   return 0;
}





/*---------------------------------------------------------*/
int nslots_granted(
lList *granted,
const char *qhostname 
) {
   lListElem *gdil_ep;
   int nslots = 0;
   const void *iterator = NULL;


   if (qhostname == NULL) {
      for_each (gdil_ep, granted) {   
         nslots += lGetUlong(gdil_ep, JG_slots);
      }
   } else {
      gdil_ep = lGetElemHostFirst(granted, JG_qhostname, qhostname, &iterator); 
      while (gdil_ep != NULL) {
         nslots += lGetUlong(gdil_ep, JG_slots);
         gdil_ep = lGetElemHostNext(granted, JG_qhostname , qhostname, &iterator); 
      }
   }

   return nslots;
}

/*
   active_subtasks returns 1 if there are active subtasks for the queue
   and 0 if there are not.
*/

int active_subtasks(
lListElem *job,
const char *qname 
) {
   lListElem *petask, *ep, *jatask;
   const char *task_qname;
   const char *master_qname;

   for_each(jatask, lGetList(job, JB_ja_tasks)) {
      master_qname = lGetString(jatask, JAT_master_queue);

      /* always consider the master queue to have active sub-tasks */
      if (master_qname && !strcmp(qname, master_qname))
         return 1;

      for_each(petask, lGetList(jatask, JAT_task_list)) {
         if (qname &&
             lGetUlong(petask, PET_status) != JFINISHED &&
             ((ep=lFirst(lGetList(petask, PET_granted_destin_identifier_list)))) &&
             ((task_qname=lGetString(ep, JG_qname))) &&
             !strcmp(qname, task_qname)) {
            return 1;
         }   
      }
   }
   return 0;
}


int active_nslots_granted(
lListElem *job,
lList *granted,
const char *qhostname 
) {
   lList *task_list;
   lListElem *gdil_ep, *jatask;
   int nslots = 0;
   const void *iterator = NULL;


   if (qhostname == NULL) {
      for_each (gdil_ep, granted) {   /* for all hosts */
         for_each (jatask, lGetList(job, JB_ja_tasks)) {
            task_list = lGetList(jatask, JAT_task_list);
            if (task_list == NULL || active_subtasks(job, lGetString(gdil_ep, JG_qname)))
               nslots += lGetUlong(gdil_ep, JG_slots);
         }
      }
   } else {
      /* only for qhostname */
      gdil_ep = lGetElemHostFirst(granted, JG_qhostname, qhostname, &iterator);
      while (gdil_ep != NULL) {
         for_each (jatask, lGetList(job, JB_ja_tasks)) {
            task_list = lGetList(jatask, JAT_task_list);
            if (task_list == NULL || active_subtasks(job, lGetString(gdil_ep, JG_qname)))
               nslots += lGetUlong(gdil_ep, JG_slots);
         }
         gdil_ep = lGetElemHostNext(granted, JG_qhostname , qhostname, &iterator); 
      }
   }

   return nslots;
}


/*---------------------------------------------------
 * sge_granted_slots
 * return number of granted slots for a (parallel(
 *---------------------------------------------------*/
int sge_granted_slots(
lList *gdil 
) {
   lListElem *ep;
   int slots = 0;

   for_each(ep, gdil) 
      slots += lGetUlong(ep, JG_slots);

   return slots;
}


