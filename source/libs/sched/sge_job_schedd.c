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
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_ckptL.h"
#include "sge_usersetL.h"
#include "sge_complexL.h"
#include "sge_requestL.h"
#include "sge_userprjL.h"     /*added to support SGE*/
#include "sge_job_schedd.h"
#include "sge_range_schedd.h"
#include "valid_queue_user.h"
#include "sge_parse_num_par.h"
#include "schedd_monitor.h"
#include "sge_sched.h"            /*added to support SGE*/
#include "schedd_conf.h"      /*added to support SGE*/
#include "schedd_message.h"
#include "sge_jataskL.h"
#include "cull_lerrnoP.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "sge_string.h"
#include "sge_range.h"
#include "sge_job_jatask.h"
#include "jb_now.h"
#include "sge_time.h"

#include "cull_hash.h"

static int user_sort = 0;

#define IDLE 0

#ifdef WIN32NATIVE
#	define strcasecmp( a, b) stricmp( a, b)
#	define strncasecmp( a, b, n) strnicmp( a, b, n)
#endif

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
   running_job = lGetElemUlong(*(splitted_jobs[SPLIT_RUNNING]),
                               JB_job_number, job_id);
   /*
    * Create list for running jobs
    */
   if (*(splitted_jobs[SPLIT_RUNNING]) == NULL) {
      const lDescr *descriptor = lGetElemDescr(*pending_job);
      *(splitted_jobs[SPLIT_RUNNING]) = lCreateList("", descriptor);
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
    * Create an array task list if necessary
    */
   r_ja_task_list = lGetList(running_job, JB_ja_tasks);
   if (r_ja_task_list == NULL) {
      r_ja_task_list = lCreateList("", JAT_Type);
      lSetList(running_job, JB_ja_tasks, r_ja_task_list);
   }
 
   /*
    * Create an array instance and add it to the running job
    * or move the existing array task into the running job
    */
   if (ja_task == NULL) {
      lList *n_h_ids = NULL;        /* RN_Type */
 
      n_h_ids = lGetList(*pending_job, JB_ja_n_h_ids);
      ja_task_id = range_list_get_first_id(n_h_ids, NULL);
      ja_task = job_search_task(*pending_job, NULL, ja_task_id, 1);
      ja_task_list = lGetList(*pending_job, JB_ja_tasks);
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
 
#if 0 /* EB: debug */
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
void user_list_init_jc(lList **user_list, const lList *running_list)
{
   lListElem *job;   /* JB_Type */

   for_each(job, running_list) {
      sge_inc_jc(user_list, lGetString(job, JB_owner), job_get_ja_tasks(job));
   }
}


/* user_list: JC_Type */
/* max_jobs_per_user: conf.maxujobs */
/* owner_or_group: either JB_owner or JB_group */
void job_lists_split_with_reference_to_max_running(lList **job_lists[],
                                                   lList **user_list,
                                                   int max_jobs_per_user)
{
   DENTER(TOP_LAYER, "job_lists_split_with_reference_to_max_running");
   if (max_jobs_per_user != 0 && 
       job_lists[SPLIT_PENDING] != NULL && *(job_lists[SPLIT_PENDING]) != NULL &&
       job_lists[SPLIT_PENDING_EXCLUDED] != NULL) {
      lListElem *user = NULL;

      /* create a hash table on JB_owner to speedup 
       * searching for jobs of a specific owner
       */
      {
         const lDescr *descr = lGetListDescr(*(job_lists[SPLIT_PENDING]));
         int pos = lGetPosInDescr(descr, JB_owner);
        
         if(descr != NULL && pos >= 0) {
            if(descr[pos].hash == NULL)  {
               cull_hash_new(*(job_lists[SPLIT_PENDING]), JB_owner, &template_hash);
            }
         }
      }

      for_each(user, *user_list) {
         u_long32 jobs_for_user = lGetUlong(user, JC_jobs);
         const char *user_name = lGetString(user, JC_name);

         if (jobs_for_user >= max_jobs_per_user) {
            const void *user_iterator = NULL;
            lListElem *user_job = NULL;         /* JB_Type */
            lListElem *next_user_job = NULL;    /* JB_Type */

            DPRINTF(("USER %s reached limit of %d jobs\n", user_name, 
                     max_jobs_per_user));
            next_user_job = lGetElemStrFirst(*(job_lists[SPLIT_PENDING]), 
                                             JB_owner, user_name, 
                                             &user_iterator);
            while ((user_job = next_user_job)) {
               next_user_job = lGetElemStrNext(*(job_lists[SPLIT_PENDING]), 
                                               JB_owner, user_name, 
                                               &user_iterator);
               if (monitor_get_next_run()) {
                  schedd_add_message(lGetUlong(user_job, JB_job_number),
                                     SCHEDD_INFO_USRGRPLIMIT_);
               }

               lDechainElem(*(job_lists[SPLIT_PENDING]), user_job);
               if (*(job_lists[SPLIT_PENDING_EXCLUDED]) == NULL) {
                  *(job_lists[SPLIT_PENDING_EXCLUDED]) =
                                      lCreateList("", lGetElemDescr(user_job));
               }

               lAppendElem(*(job_lists[SPLIT_PENDING_EXCLUDED]), user_job);
            }
         }
      }
   }
   DEXIT;
}

void split_jobs(lList **job_list, lList **answer_list,
                lList *queue_list, u_long32 max_aj_instances, 
                lList **result_list[])
{
#if 0 /* EB: enable debug messages for this function */
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
         if (target == NULL && result_list[SPLIT_HOLD] &&
             (ja_task_hold & MINUS_H_TGT_ALL)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in hold state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_HOLD]);
         } 
         if (target == NULL && result_list[SPLIT_ERROR] && 
             (ja_task_state & JERROR)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is in error state\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_ERROR]);
         } 
         if (target == NULL && result_list[SPLIT_WAITING_DUE_TO_TIME] &&
             (lGetUlong(job, JB_execution_time) > sge_get_gmt())) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is waiting due to time.\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_WAITING_DUE_TO_TIME]);
         }
         if (target == NULL && result_list[SPLIT_WAITING_DUE_TO_PREDECESSOR] &&
             (lGetList(job, JB_jid_predecessor_list) != NULL)) {
#ifdef JOB_SPLIT_DEBUG
            DPRINTF(("Task "u32" is waiting due to pred.\n", ja_task_id));
#endif
            target = &(target_tasks[SPLIT_WAITING_DUE_TO_PREDECESSOR]);
         }
         if (target == NULL && result_list[SPLIT_PENDING] && 
             (ja_task_status == JIDLE)) {
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
               lList *granted_queue_list = lGetList(ja_task, 
                             JAT_granted_destin_identifier_list); /* QU_Type */
               lListElem *granted_queue = NULL;    /* QU_Type */

               for_each(granted_queue, granted_queue_list) {
                  const char *queue_name = NULL; 
                  lListElem *queue = NULL;
                  u_long32 queue_state;

                  queue_name = lGetString(granted_queue, JG_qname);  
                  queue = lGetElemStr(queue_list, QU_qname, queue_name);
                  queue_state = lGetUlong(queue, QU_state);
                  
                  if ((queue_state & QSUSPENDED) ||
                      (queue_state & QSUSPENDED_ON_SUBORDINATE)) {
#ifdef JOB_SPLIT_DEBUG
                     DPRINTF(("Task "u32" is in suspended state\n",ja_task_id));
#endif
                     target = &(target_tasks[SPLIT_SUSPENDED]);
                     break;
                  }     
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
      job = NULL;

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
               schedd_add_message(job_id, SCHEDD_INFO_JOBINERROR_);
            }
            if (monitor_get_next_run()) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDERRORSTATEREACHED, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_HOLD:
            if (is_first_of_category) {
               schedd_add_message(job_id, SCHEDD_INFO_JOBHOLD_);
            }
            if (monitor_get_next_run()) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEOFXHOLD, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_WAITING_DUE_TO_TIME:
            if (is_first_of_category) {
               schedd_add_message(job_id, SCHEDD_INFO_EXECTIME_);
            }
            if (monitor_get_next_run()) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDEXECUTIONTIMENOTREACHED, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_WAITING_DUE_TO_PREDECESSOR:
            if (is_first_of_category) {
               schedd_add_message(job_id, SCHEDD_INFO_JOBDEPEND_);
            }
            if (monitor_get_next_run()) {
               schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEDEPENDENCIES, 
                               *job_list, JB_job_number);
            }
            break;
         case SPLIT_PENDING_EXCLUDED_INSTANCES:
            if (is_first_of_category) {
               schedd_add_message(job_id, SCHEDD_INFO_MAX_AJ_INSTANCES_);
            }
            break;
         case SPLIT_PENDING_EXCLUDED:
            if (is_first_of_category) {
               schedd_add_message(job_id, SCHEDD_INFO_USRGRPLIMIT_);
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
} 

int set_user_sort(
int i /* 0/1/-1 = on/off/ask */
) {
   if (i>=0 && ((i && !user_sort) || (!i && user_sort))) {
      user_sort = i;   
   }
   return user_sort;
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

int rebuild_jc(
lList **jcpp,
lList *job_list 
) {
   lListElem *job, *jc_elem, *ja_task;
   DENTER(TOP_LAYER, "rebuild_jc");

   /* free existing job counter list */
   *jcpp = lFreeList(*jcpp);

   if (!user_sort) {
      DEXIT;
      return 0;
   }

   /* prepare job counter list */
   for_each (job, job_list) {
      u_long32 number_of_tasks;


      number_of_tasks = job_get_ja_tasks(job);
      DPRINTF(("rebuild_jc(1): visiting %d array-jobs\n", number_of_tasks));
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         if (running_status(lGetUlong(ja_task, JAT_status))) {
            sge_inc_jc(jcpp, lGetString(job, JB_owner), 1);      
#if 0
            DPRINTF(("JOB "u32" is RUNNING\n", lGetUlong(job, JB_job_number)));
#endif
         }
      }
   }

   /* use job counter list to fill in # of running jobs */
   for_each (job, job_list) {
      jc_elem = lGetElemStr(*jcpp, JC_name, lGetString(job, JB_owner));
      lSetUlong(job, JB_nrunning, jc_elem ? lGetUlong(jc_elem, JC_jobs): 0);
   }

#if 0
   {
      lListElem *jc;
      for_each (jc, *jcpp) {
         DPRINTF(("USER: %s JOBS RUNNING: "u32"\n", 
           lGetString(jc, JC_name), lGetUlong(jc, JC_jobs))); 
      }
   }
#endif

   DEXIT;
   return 0;
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
int resort_jobs(
lList *jc,
lList *job_list,
const char *owner,
lSortOrder *so 
) {
   lListElem *job, *jc_owner;
   int njobs;

   DENTER(TOP_LAYER, "resort_jobs");


   if (user_sort) {
      /* get number of running jobs of this user */
      if (owner) {
         jc_owner = lGetElemStr(jc, JC_name, owner);
         njobs = jc_owner ? lGetUlong(jc_owner, JC_jobs) : 0;

         for_each(job, job_list) {
            if (!strcmp(owner, lGetString(job, JB_owner)))
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
lListElem *explicit_job_request(
lListElem *jep,
const char *name 
) {
   lListElem *ep = NULL, *res;

   for_each (res, lGetList(jep, JB_hard_resource_list)) 
      if ((ep=lGetSubStr(res, CE_name, name, RE_entries))) 
         return ep;

   for_each (res, lGetList(jep, JB_soft_resource_list)) 
      if ((ep=lGetSubStr(res, CE_name, name, RE_entries))) 
         return ep;

   return NULL;
}


/*---------------------------------------------------------*/
int get_job_contribution(
double *dvalp,
const char *name,
lListElem *jep,
lListElem *dcep 
) {
   const char *strval;
   char error_str[256];
   lListElem *ep;

   DENTER(TOP_LAYER, "get_job_contribution");

   /* explicit job request */
   ep = explicit_job_request(jep, name);

   /* implicit job request */
   if (!ep || !(strval=lGetString(ep, CE_stringval))) {
      strval = lGetString(dcep, CE_default);
   }
   if (!(parse_ulong_val(dvalp, NULL, TYPE_INT, strval,
            error_str, sizeof(error_str)-1))) {
      DEXIT;
      ERROR((SGE_EVENT, MSG_ATTRIB_PARSINGATTRIBUTEXFAILED_SS , name, error_str));
      return -1;
   }

   if (dvalp && *dvalp == 0.0)
      return 1;

   DEXIT;
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
   lListElem *task, *ep, *ja_task, *task_task;
   const char *task_qname;
   const char *master_qname;

   for_each(ja_task, lGetList(job, JB_ja_tasks)) {
      master_qname = lGetString(ja_task, JAT_master_queue);

      /* always consider the master queue to have active sub-tasks */
      if (master_qname && !strcmp(qname, master_qname))
         return 1;

      for_each(task, lGetList(ja_task, JAT_task_list)) {
         for_each(task_task, lGetList(task, JB_ja_tasks)) {
            if (qname &&
                lGetUlong(task_task, JAT_status) != JFINISHED &&
                ((ep=lFirst(lGetList(task_task, JAT_granted_destin_identifier_list)))) &&
                ((task_qname=lGetString(ep, JG_qname))) &&
                !strcmp(qname, task_qname))

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
   lListElem *gdil_ep, *ja_task;
   int nslots = 0;
   const void *iterator = NULL;


   if (qhostname == NULL) {
      for_each (gdil_ep, granted) {   /* for all hosts */
         for_each (ja_task, lGetList(job, JB_ja_tasks)) {
            task_list = lGetList(ja_task, JAT_task_list);
            if (task_list == NULL || active_subtasks(job, lGetString(gdil_ep, JG_qname)))
               nslots += lGetUlong(gdil_ep, JG_slots);
         }
      }
   } else {
      /* only for qhostname */
      gdil_ep = lGetElemHostFirst(granted, JG_qhostname, qhostname, &iterator);
      while (gdil_ep != NULL) {
         for_each (ja_task, lGetList(job, JB_ja_tasks)) {
            task_list = lGetList(ja_task, JAT_task_list);
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
