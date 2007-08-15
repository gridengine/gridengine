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
 *   The Initial Developer of the Original Code is: Rising Sun Pictures
 * 
 *   Copyright: 2007 by Rising Sun Pictures
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "sge_task_depend.h"
#include "sge_ja_task.h"
#include "sgermon.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "sge_bitfield.h"

static u_long32 task_depend_div_floor(u_long32 a, u_long32 b)
{
   return a / b;
}

static u_long32 nearest_index_in_A(u_long32 i, u_long32 t0, u_long32 sa) 
{
   return t0 + task_depend_div_floor(i - t0, sa) * sa;
}

static void task_depend(u_long32 *lb, u_long32 *ub, u_long32 t0, 
                        u_long32 sa, u_long32 sb, u_long32 step_b_id)
{
   /* simulate the equation i = t0 + n * sb, n in {0, 1, ..., N-1}. */

   u_long32 i = step_b_id;

   *lb = nearest_index_in_A(i, t0, sa);
   *ub = nearest_index_in_A(i + sb - 1, t0, sa);
}

/**************** qmaster/task/sge_task_depend_get_range() *******************
*  NAME
*     sge_task_depend_get_range() -- get predecessor job task depdendencies 
*
*  SYNOPSIS
*     int sge_task_depend_get_range(lListElem **range, lList **alpp, 
*                                   const lListElem *pre_jep, 
*                                   const lListElem *suc_jep, 
*                                   u_long32 task_id) 
*
*  FUNCTION
*     This function determines the range of sub-tasks of job pre_jep that
*     suc_jep.task_id will be dependent on when suc_jep has an explicit
*     array dependency hold on pre_jep (with -hold_jid_ad option).
*
*  INPUTS
*     lListElem **range        - RN_Type pointer
*     lList **alpp             - AN_Type list pointer
*     const lListElem *pre_jep - const JB_Type element
*     const lListElem *suc_jep - const JB_Type element
*     u_long32 task_id         - a valid suc_jep task id  
*
*  RESULT
*     int - 0 on success
*
*  NOTES
*     Let div(a, b) = floor(a / b)
*     Let nearest_index_in_A(i) = t0 + div(i - t0, sa) * sa
*
*     Sub-task B.i will be dependent on all tasks in A between
*     nearest_index_in_A(i) and nearest_index_in_A(i + sb - 1) where 
*     i = t0 + n * sb and n is a positive integer.
*
*     It is safe to swap pre_jep and suc_jep provided that the given task id
*     belongs to pre_jep (therefore reversing the sense of the dependence).
*
*  MT-NOTE
*     sge_task_depend_get_range() is MT safe
*
******************************************************************************/
int sge_task_depend_get_range(lListElem **range, lList **alpp, 
                              const lListElem *pre_jep, 
                              const lListElem *suc_jep, u_long32 task_id) 
{
   u_long32 a0, a1, b0, b1, sa, sb, rmin, rmax;

   DENTER(TOP_LAYER, "sge_task_depend_get_range");

   if (range == NULL || 
         pre_jep == NULL || 
         suc_jep == NULL ||
         task_id == 0) {
      DRETURN(STATUS_EUNKNOWN);
   }

   job_get_submit_task_ids(pre_jep, &a0, &a1, &sa);
   job_get_submit_task_ids(suc_jep, &b0, &b1, &sb);

   /* do some basic checks on the input */
   if (!sge_task_depend_is_same_range(pre_jep, suc_jep) || 
       ((task_id - 1) % sb) != 0) {
      DRETURN(STATUS_EUNKNOWN);
   }

   /* make the actual call to the core dependence function */
   task_depend(&rmin, &rmax, a0, sa, sb, task_id);

   /* do some basic checks on the output */
   if (rmin < a0 || rmax > a1) {
      DRETURN(STATUS_EUNKNOWN);
   }

   /* if an existing range was not given, create one */
   if (*range == NULL) {
      *range = lCreateElem(RN_Type);
      if (*range == NULL) {
         DRETURN(STATUS_EUNKNOWN);
      }
   }

   range_set_all_ids(*range, rmin, rmax, sa);

   DRETURN(0);
}

/*************************************************************************   
 Enrolled tasks matching this template may be held or unheld   
 ************************************************************************/
static bool task_depend_is_resched(lListElem *job, u_long32 task_id)
{
   DENTER(TOP_LAYER, "task_depend_is_resched");

   if (job_is_enrolled(job, task_id)) {
      lListElem *ja_task = job_search_task(job, NULL, task_id);
      if (ja_task) {
         /* JA: FIXME: only alter hold state when idle/waiting? */
         u_long32 state = lGetUlong(ja_task, JAT_state);
         u_long32 status = lGetUlong(ja_task, JAT_status);
         bool ret = (ISSET(status, JIDLE) &&
                     ISSET(state, JQUEUED) &&
                     ISSET(state, JWAITING));
         DRETURN(ret);
      }
   }
   DRETURN(false);
}

/**************** qmaster/task/sge_task_depend_update() *******************
*  NAME
*     sge_task_depend_update() -- update job array dependencies for a task 
*
*  SYNOPSIS
*     bool sge_task_depend_update(lListElem *suc_jep, lList **alpp,
                                  u_long32 task_id) 
*
*  FUNCTION
*     This function recalculates array dependency hold information
*     for a particular task id of job suc_jep (-hold_jid_ad option).  
*     If the task is independent (i.e., has no predecessor tasks that
*     are not in a finished state), its id will be removed from the
*     JB_ja_a_h_ids hold range and potentially moved to JB_ja_n_h_ids.   
*
*  INPUTS
*     lListElem *suc_jep - JB_Type element
*     lList **alpp       - AN_Type list pointer
*     u_long32 task_id   - a task id in job suc_jep 
*
*  RESULT
*     bool - true if task hold information was updated, otherwise false
*
*  NOTES
*     If array dependency information cannot be determined, then
*     this function will assume that a task dependence still exists.
*
*     A false result from this function should not be considered failure. 
*     Update status is returned to help the caller decide whether modify 
*     event code should be emitted.
*
*     If the job argument suc_jep is NULL, or the task indicated by task_id is
*     already enrolled into ja_tasks, false is returned.
*
*  MT-NOTE  
*     Is not thread safe. Reads from the global Job-List
*
******************************************************************************/
bool sge_task_depend_update(lListElem *suc_jep, lList **alpp, u_long32 task_id)
{
   const lListElem *pre = NULL;  /* JRE_Type */
   u_long32 hold_state, new_state;
   int Depend = 0;

   DENTER(TOP_LAYER, "sge_task_depend_update");

   /* this should not really be necessary */
   if (suc_jep == NULL) {
      DPRINTF(("got NULL for suc_jep job argument\n"));
      DRETURN(false);
   }

   /* for enrolled tasks, we only update the hold state if they are rescheduled */
   if (job_is_enrolled(suc_jep, task_id) &&
       !task_depend_is_resched(suc_jep, task_id)) {
      DRETURN(false);
   }

   /* process the resolved predecessor list */
   for_each(pre, lGetList(suc_jep, JB_ja_ad_predecessor_list)) {
      u_long32 sa, sa_task_id, amin, amax;
      const lListElem *pred_jep = NULL; /* JB_Type */
      lListElem *dep_range = NULL;      /* RN_Type */

      /* locate the job id in the master list, if not found we can't do much here */
      pred_jep = job_list_locate(*(object_type_get_master_list(SGE_TYPE_JOB)), 
         lGetUlong(pre, JRE_job_number));
      if (!pred_jep) continue;

      /* use the RSP functions to determine dependent predecessor task range */
      if (sge_task_depend_get_range(&dep_range, alpp, pred_jep, suc_jep, task_id)) {
         DPRINTF(("could not calculate dependent iteration ranges for a job\n"));
         /* since we can't calculate it, we must assume dependence */
         lFreeElem(&dep_range);
         Depend = 1;
         break;
      }

      /* fetch predecessor job dependency range ids */
      range_get_all_ids(dep_range, &amin, &amax, &sa);

      /* all tasks between {amin, ..., amax} are dependencies unless they finished */
      for (sa_task_id = amin; sa_task_id <= amax; sa_task_id += sa) {
         lListElem *ja_task;
         /* if the task is not enrolled => dependence */
         if (!job_is_enrolled(pred_jep, sa_task_id)) {
            Depend = 1;
            break;
         }
         /* if the task is not finished => dependence */
         ja_task = job_search_task(pred_jep, alpp, sa_task_id);
         if (ja_task) {
            if (lGetUlong(ja_task, JAT_status) != JFINISHED) {
               Depend = 1;
               break;
            }
         }
      }

      /* cleanup, if a dep range was allocated */
      lFreeElem(&dep_range);

      /* minor speed optimization */
      if (Depend) break;
   }

   /* alter the hold state based on dependence info */
   hold_state = job_get_hold_state(suc_jep, task_id);
   if (Depend) {
      new_state = hold_state | MINUS_H_TGT_JA_AD;
   } else {
      new_state = hold_state & ~MINUS_H_TGT_JA_AD;
   }

   /* update the hold state, possibly moving the task between n_h_ids and a_h_ids */
   if (new_state != hold_state) {
      job_set_hold_state(suc_jep, alpp, task_id, new_state);
      DRETURN(true);
   }

   DRETURN(false);
}


/**************** qmaster/task/sge_task_depend_flush() *******************
*  NAME
*     sge_task_depend_flush() -- flush job array task dependencies 
*
*  SYNOPSIS
*     bool sge_task_depend_flush(lListElem *suc_jep, lList **alpp) 
*
*  FUNCTION
*     This function clears the JB_ja_a_h_ids dependence cache when it
*     contains one or more task ranges and the job array dependency
*     predecessor list is empty (-hold_jid_ad option). It might also
*     release the JHELD flag of the JAT_state field for enrolled tasks
*     with MINUS_H_TGT_JA_AD in the JAT_hold field.
*
*  INPUTS
*     lListElem *suc_jep - JB_Type element
*     lList **alpp       - AN_Type list pointer
*
*  RESULT
*     bool - true if task hold information was updated, otherwise false
*
*  NOTES
*     It is useful to call this function when jobs are removed from the 
*     JB_ja_ad_predecessor_list as a way of ensuring that dependence state
*     information in both structures is consistent. It is more efficient
*     to call this function than to update dependency information accross
*     the entire range of sub-tasks of job suc_jep.
*
*     If the job was never an array successor, this function has no effect.
*
*  MT-NOTE
*     sge_task_depend_flush() is MT safe
*
******************************************************************************/
bool sge_task_depend_flush(lListElem *suc_jep, lList **alpp)
{
   bool ret = false;

   DENTER(TOP_LAYER, "sge_task_depend_flush");

   /* this should not really be necessary */
   if (suc_jep == NULL) {
      DPRINTF(("got NULL for suc_jep job argument\n"));
      DRETURN(false);
   }

   /* ensure empty hold states are consistent */ 
   if (lGetNumberOfElem(lGetList(suc_jep, JB_ja_ad_request_list)) > 0 &&
       lGetNumberOfElem(lGetList(suc_jep, JB_ja_ad_predecessor_list)) == 0) {
      lListElem *ja_task;  /* JAT_Type */
      if (lGetList(suc_jep, JB_ja_a_h_ids)) {
         const lListElem *range;
         lList *a_h_ids = lCopyList("", lGetList(suc_jep, JB_ja_a_h_ids));
         for_each(range, a_h_ids) {
            u_long32 rmin, rmax, rstep, hold_state;
            range_get_all_ids(range, &rmin, &rmax, &rstep);
            for ( ; rmin <= rmax; rmin += rstep) {
               hold_state = job_get_hold_state(suc_jep, rmin);
               hold_state &= ~MINUS_H_TGT_JA_AD;
               job_set_hold_state(suc_jep, alpp, rmin, hold_state);
            }
         }
         lFreeList(&a_h_ids);
         /* just make sure it is null */
         if (lGetList(suc_jep, JB_ja_a_h_ids)) {
            a_h_ids = NULL;
            lXchgList(suc_jep, JB_ja_a_h_ids, &a_h_ids);
            lFreeList(&a_h_ids);
         }
         ret = true;
      }
      /* unhold any arary held tasks that are enrolled */
      for_each(ja_task, lGetList(suc_jep, JB_ja_tasks)) {
         u_long32 task_id = lGetUlong(ja_task, JAT_task_number);
         u_long32 hold_state = job_get_hold_state(suc_jep, task_id);
         if ((hold_state & MINUS_H_TGT_JA_AD) != 0) {
            hold_state &= ~MINUS_H_TGT_JA_AD;
            job_set_hold_state(suc_jep, alpp, task_id, hold_state);
            ret = true;
         }
      }
   }

   DRETURN(ret);
}

/********** qmaster/task/sge_task_depend_is_same_range() ****************
*  NAME
*     sge_task_depend_is_same_range() -- determine array job equivalence 
*
*  SYNOPSIS
*     bool sge_task_depend_is_same_range(const lListElem *suc_jep, 
                                         const lListElem *pre_jep) 
*
*  FUNCTION
*     This function determines if the job arguments are suitable predecessor
*     and successor jobs for an array dependency pair (-hold_jid_ad option).
*
*  INPUTS
*     lListElem *pre_jep - const JB_Type element
*     lListElem *suc_jep - const JB_Type element
*
*  RESULT
*     bool - true if jobs are compatible array jobs, otherwise false
*
*  MT-NOTE
*    sge_task_depend_is_same_range() is MT safe
*
******************************************************************************/
bool sge_task_depend_is_same_range(const lListElem *pre_jep, 
                                   const lListElem *suc_jep)
{
   u_long32 a0, a1, b0, b1, sa, sb;

   DENTER(TOP_LAYER, "sge_task_depend_is_same_range");

   /* equivalent jobs cannot be NULL */
   if (pre_jep == NULL || suc_jep == NULL) {
      DPRINTF(("got NULL pre_jep or suc_jep job argument\n"));
      DRETURN(false);
   }
   
   /* equivalent jobs must be array jobs (-t option) */
   if (!job_is_array(pre_jep) || !job_is_array(suc_jep)) {
      DRETURN(false);
   }
   
   /* fetch job submit ranges */
   job_get_submit_task_ids(pre_jep, &a0, &a1, &sa);
   job_get_submit_task_ids(suc_jep, &b0, &b1, &sb);

   /* equivalent jobs must have the same range of sub-tasks */
   if (a0 != b0 || (a1 + sa - 1) != (b1 + sb - 1)) {
      DRETURN(false);
   }

   DRETURN(true);
}
