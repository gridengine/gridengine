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

#include "cull.h"
#include "scheduler.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_schedd.h"
#include "sge_hash.h"
#include "sge_category.h"
#include "sge_access_tree.h"
#include "sge_job_schedd.h"
#include "schedd_conf.h"

#include "sge_access_treeL.h"
#include "sge_jobL.h"
#include "sge_ctL.h"
#include "msg_schedd.h"

/* the access tree */
static lList *priority_group_list = NULL;

/* hash table used for quick access of jobs arrays from access tree */
static HashTable runnable_job_arrays = NULL;

static lSortOrder *so_pgr = NULL, 
           *so_usr = NULL, 
           *so_jrl = NULL;
static lListElem *current_pgrp = NULL;

static void at_job_counter_impl(u_long32 priority, char *owner, int slots);
static lListElem *at_first_in_jrl(lListElem *jrl_container, int nm_jrl, int nm_sort, int nm_curr);
static void at_trace(void);

/*
**
** Stuff implementing fast access to the next job 
** that is to be dispatched accordingly to the
** job sorting policy 
**
*/

int at_init()
{
   if (!so_pgr) 
      so_pgr = lParseSortOrderVarArg(PGR_Type, "%I-", PGR_priority);
   if (!so_usr)
      so_usr = lParseSortOrderVarArg(USR_Type, "%I+", USR_nrunning_dl);
   if (!so_jrl)
      so_jrl = lParseSortOrderVarArg(JRL_Type, "%I+%I+", JRL_submission_time, JRL_jobid); 

   if (!so_pgr || !so_usr  || !so_jrl) {
      return -1;
   }
   priority_group_list = lFreeList(priority_group_list);

   return 0;
}

/*
** ------------ to be called from within event layer 
*/

void at_finish(void)
{
   if (so_pgr) 
      so_pgr = lFreeSortOrder(so_pgr);
   if (so_usr) 
      so_pgr = lFreeSortOrder(so_pgr);
   if (so_jrl) 
      so_pgr = lFreeSortOrder(so_pgr);

   if (runnable_job_arrays) {
      HashTableDestroy(runnable_job_arrays);
      runnable_job_arrays = NULL;
   }
   
   priority_group_list = lFreeList(priority_group_list);

   return;
}


/*
** register a job array to the access tree
*/
void at_register_job_array(
lListElem *job_array 
) {
   lListElem *pgr, *user = NULL, *last, *jr;
   lList *jrl;
   u_long32 priority;
   int user_sort = set_user_sort(-1);

   DENTER(TOP_LAYER, "at_register_job_array");

   priority = lGetUlong(job_array, JB_priority);

   /* search priority group entry */
   if (!(pgr=lGetElemUlong(priority_group_list, PGR_priority,  priority))) {
      pgr = lAddElemUlong(&priority_group_list, PGR_priority, priority, PGR_Type);
      DPRINTF(("AT + %d\n", priority));
      lSortList(priority_group_list, so_pgr);
   }

   /* make a add new job reference entry */ 
   jr = lCreateElem(JRL_Type);
   lSetUlong(jr, JRL_jobid, lGetUlong(job_array, JB_job_number));
   lSetUlong(jr, JRL_submission_time, lGetUlong(job_array, JB_submission_time));
   lSetRef(jr, JRL_category, lGetRef(job_array, JB_category));

   if (user_sort) {
      char *owner = lGetString(job_array, JB_owner);
      if (!(user=lGetSubStr(pgr, USR_name, owner, PGR_subordinated_list))) {
         user = lAddSubStr(pgr, USR_name, owner, PGR_subordinated_list, USR_Type);
         DPRINTF(("AT + %d %s\n", priority, owner));
         lSortList(priority_group_list, so_pgr);
      }
      if (!(jrl = lGetList(user, USR_job_references))) {
         jrl = lCreateList("", JRL_Type);
         lSetList(user, USR_job_references, jrl);
      }
   } else {
      if (!(jrl = lGetList(pgr, PGR_subordinated_list))) {
         jrl = lCreateList("", JRL_Type);
         lSetList(pgr, PGR_subordinated_list, jrl);
      }
   } 

   /* in most cases the job array must to be added 
      at the last position of the job reference list */
   if (!(last = lLast(jrl)) 
         || lGetUlong(last, JRL_submission_time) < lGetUlong(job_array, JB_submission_time) 
         || (lGetUlong(last, JRL_submission_time) == lGetUlong(job_array, JB_submission_time) &&
            lGetUlong(last, JRL_jobid) < lGetUlong(job_array, JB_job_number))) 
      lAppendElem(jrl, jr);
   else {
      /* "insert sorted"
         we just append the job reference and 
         set a flag which causes that the list is 
         sorted later on completely 
            */
      lAppendElem(jrl, jr);
      if ((user_sort?lGetUlong(user, USR_sort_me):lGetUlong(pgr, PGR_sort_me))==0)
         DPRINTF(("### ### ### ### ### TRIGGER RESORTING OF JOB REFERENCE LIST ### ### ### ### ###\n"));
      if (user_sort)
         lSetUlong(user, USR_sort_me, 1);
      else
         lSetUlong(pgr, PGR_sort_me, 1);
   }

   DEXIT;   
   return;
}

static void at_trace()
{
#if 0
   lListElem *u, *p, *j;
   int user_sort = set_user_sort(-1);
   char *s;
   lListElem *current;

   for_each (p, priority_group_list) {
      DPRINTF(("%d%s\n", lGetUlong(p, PGR_priority), 
            lGetUlong(p, PGR_sort_me)?" sortme!":""));
      if (user_sort) {
         for_each (u, lGetList(p, PGR_subordinated_list)) {
            current = lGetRef(u, USR_current);
            DPRINTF(("   %s "u32"("u32")%s\n", lGetString(u, USR_name), 
               lGetUlong(u, USR_nrunning_dl), lGetUlong(u, USR_nrunning_el),
               lGetUlong(u, USR_sort_me)?" sortme!":""));
            for_each (j, lGetList(u, USR_job_references)) {
               s = lGetString(lGetRef(j, JRL_category), CT_str);
               DPRINTF(("   %s   "u32" %s\n", 
                     (current == j)?"-->":"   ", lGetUlong(j, JRL_jobid), s));
            }
         }
      } else {
         current = lGetRef(p, PGR_current);
         for_each (j, lGetList(p, PGR_subordinated_list)) {
            s = lGetString(lGetRef(j, JRL_category), CT_str);
            DPRINTF(("   %s "u32" %s\n", 
                  (current == j)?"-->":"   ", lGetUlong(j, JRL_jobid), s));
         }
      }
   }
#endif
   return;
}

/*
** unregister a job array from access tree
*/
void at_unregister_job_array(
lListElem *job 
) {
   u_long32 jobid, priority;
   lListElem *pgrp;

   DENTER(TOP_LAYER, "at_unregister_job_array");

   priority = lGetUlong(job, JB_priority);
   jobid = lGetUlong(job, JB_job_number);

   if (!(pgrp = lGetElemUlong(priority_group_list, PGR_priority, priority))) {
      ERROR((SGE_EVENT, MSG_SCHEDD_NOPRIORITYINACCESSTREEFOUND_UU, 
            u32c(priority), u32c(jobid)));
      DEXIT;
      return;
   }

   if (set_user_sort(-1)) {
      lListElem *user;
      char *owner = lGetString(job, JB_owner);
      user = lGetSubStr(pgrp, USR_name, owner, PGR_subordinated_list);

      lDelSubUlong(user, JRL_jobid, jobid, USR_job_references);
      if (!lGetNumberOfElem(lGetList(user, USR_job_references)) && !lGetUlong(user, USR_nrunning_el)) {
         DPRINTF(("AT - "u32" %s\n", priority, owner));
         lRemoveElem(lGetList(pgrp, PGR_subordinated_list), user);
      }
   } else {
      lDelSubUlong(pgrp, JRL_jobid, jobid, PGR_subordinated_list);
   }

   DPRINTF(("at_unregister_job_array "u32"\n", jobid));

   if (!lGetNumberOfElem(lGetList(pgrp, PGR_subordinated_list))) {
      lRemoveElem(priority_group_list, pgrp);
      DPRINTF(("AT - "u32"\n", priority));
   }

   DEXIT;
   return;
}

/* notify access tree of jobs state transitions between 
   running/pending per user and per priority group */
void at_inc_job_counter(
u_long32 priority,
char *owner,
int slots 
) {
   at_job_counter_impl(priority, owner, slots);
}

void at_dec_job_counter(
u_long32 priority,
char *owner,
int slots 
) {
   at_job_counter_impl(priority, owner, -slots);
}

static void at_job_counter_impl(
u_long32 priority,
char *owner,
int slots 
) {
   int nrunning, resulting;
   lListElem *pgrp, *user;

   DENTER(TOP_LAYER, "at_job_counter_impl");

   DPRINTF(("MOD job counter %d %s %d\n", priority, owner, slots));

   /* add priority group if necessary */
   if (!(pgrp = lGetElemUlong(priority_group_list, PGR_priority, priority))) {
      if (slots<=0) {
         ERROR((SGE_EVENT, MSG_SCHEDD_INCONSISTENTACCESSTREEDATA));
         return;
      }
      pgrp = lAddElemUlong(&priority_group_list, PGR_priority, priority, PGR_Type);
/*       DPRINTF(("AT + %d\n", priority)); */
   }
   
   /* add user if necessary */
   if (!(user = lGetSubStr(pgrp, USR_name, owner, PGR_subordinated_list))) {
      if (slots<=0) {
         ERROR((SGE_EVENT, MSG_SCHEDD_INCONSISTENTACCESSTREEDATA));
         return;
      }
      user = lAddSubStr(pgrp, USR_name, owner, PGR_subordinated_list, USR_Type);
/*       DPRINTF(("AT + %d %s\n", priority, owner)); */
      nrunning = 0;
   } else
      nrunning = lGetUlong(user, USR_nrunning_el);

   resulting = nrunning+slots;
   if ( resulting < 0) {
      ERROR((SGE_EVENT, MSG_SCHEDD_INCONSISTENTACCESSTREEDATA ));
      return;
   }

   lSetUlong(user, USR_nrunning_el, resulting);

   if (resulting == 0) {
      /* remove user if possible */
      if (!lGetNumberOfElem(lGetList(user, USR_job_references))) {
         lDelSubStr(pgrp, USR_name, owner, PGR_subordinated_list);
         DPRINTF(("AT - %d %s\n", priority, owner));
      }

      /* remove priority group if possible */
      if (!lGetNumberOfElem(lGetList(pgrp, PGR_subordinated_list))) {
         lDelElemUlong(&priority_group_list, PGR_priority, priority);
         DPRINTF(("AT - %d\n", priority));
      }
   }

   DEXIT;
   return;
}


/*
** ------------ to be called from within dispatch layer 
*/

/*
**
** The dispatch layer has decided which jobs are runnable and 
** which are not. Take notice of only these jobs for scheduling.
**
*/
void at_notice_runnable_job_arrays(
lList *job_list 
) {
   lListElem *job, *pgrp;

   DENTER(TOP_LAYER, "at_notice_runnable_job_arrays");

   /* build new hash table for all jobs in the job list ...  */
   if (runnable_job_arrays) {
      HashTableDestroy(runnable_job_arrays);
      runnable_job_arrays = NULL;
   }
   /* ... and register all runnable jobs in the hash table */ 
   runnable_job_arrays = HashTableCreate(14, HashFunc_u_long32, HashCompare_u_long32); /* 2^14 entries */
   for_each (job, job_list)
      HashTableStore(runnable_job_arrays, 
            (void *)(u_long)lGetUlong(job, JB_job_number), (void *)(u_long)job);


   /* reinitialize the iterator in our access tree */
   current_pgrp = NULL;
   for_each (pgrp, priority_group_list) {
      if (set_user_sort(-1)) {
         lListElem *user;
         /* use the number of running jobs from event layer as basis
            for keeping the same information in dispatch layer */
         for_each (user, lGetList(pgrp, PGR_subordinated_list)) {
            lSetUlong(user, USR_nrunning_dl, lGetUlong(user, USR_nrunning_el)); 
            lSetRef(user, USR_current, NULL);
         }
         lSetUlong(pgrp, PGR_sort_me, 1);
      }
      lSetRef(pgrp, PGR_current, NULL);
   }

   DPRINTF(("REINITIALIZED ACCESS LIST ITERATOR:\n"));
   at_trace();
   DEXIT;
   return;
}

/* 
** 
** A job array has been dispatched completely in this scheduling 
** run. Thus next call to at_get_next_job_array() may not return
** this job array.
**
*/
void at_finished_array_dispatching(
lListElem *job 
) {
   u_long32 jobid = lGetUlong(job, JB_job_number);
   /* remove this job from our hash table */
   HashTableDelete(runnable_job_arrays, (void *)(u_long)jobid);
   DPRINTF(("remove "u32" from hashtable\n", jobid));
}

/* 
** 
** The scheduler has dispatched a task for that job array.
** Debit it internally if necessary. To implement this access 
** tree internal iterators must be incremented.
** 
*/
void at_dispatched_a_task(
lListElem *job,
int slots 
) {
   lListElem *pgrp, *user;

   DENTER(TOP_LAYER, "at_dispatched_a_task");

   if (set_user_sort(-1)) {
      DPRINTF(("USERSORT: got dispatch notification for %d jobs of user %s\n", 
            slots, lGetString(job, JB_owner)));
      /* debit this job */
      pgrp = lGetElemUlong(priority_group_list, PGR_priority, lGetUlong(job, JB_priority));
      user = lGetSubStr(pgrp, USR_name, lGetString(job, JB_owner), PGR_subordinated_list);
      lSetUlong(user, USR_nrunning_dl, lGetUlong(user, USR_nrunning_dl) + slots);

      /* user list of this priority group needs sorting */
      lSetUlong(pgrp, PGR_sort_me, 1);
   }

   DEXIT;
   return;
}

/* 
**
** Use access tree to find actual job array to be respected for
** dispatching. The return value of this function can change
** if between two calls a task was dispatched or a job array has
** been dispatched completely. Skip job arrays whose categories 
** are actually not schedulable.
**
*/
lListElem *at_get_actual_job_array()
{
   lListElem *job_array = NULL;
   lList *usl;

   /* this is our cursor */
   static int current_min_jobs_per_user = -1;

   DENTER(TOP_LAYER, "at_get_next_job_array");


   /* initialize priority group curser */
   if (!current_pgrp) {
      if (!priority_group_list) {
         DEXIT;
         return NULL;
      }
      current_pgrp = lFirst(priority_group_list);
      current_min_jobs_per_user = -1;
   }

   do { /* iterate through all priority groups */
      if (!set_user_sort(-1)) {
         /* FCFS - simply return the current job array 
                   if it is dispatchable and still in
                   our directory of runnable jobs 
                   else take the next job array */
         job_array = at_first_in_jrl(current_pgrp, PGR_subordinated_list, 
            PGR_sort_me, PGR_current);
         if (job_array)
            DPRINTF(("FCFS: "u32"\n", lGetUlong(job_array, JB_job_number)));
         else
            DPRINTF(("FCFS: no more jobs in priority group "u32"\n", 
                  lGetUlong(current_pgrp, PGR_priority)));
      } else {
         lListElem *user;
         usl = lGetList(current_pgrp, PGR_subordinated_list);

         /* sort user list if neccessary */
         if (lGetUlong(current_pgrp, PGR_sort_me)) {
            DPRINTF(("AT: sorting users in priority group "u32" (curr_jpu = %d)\n", 
               lGetUlong(current_pgrp, PGR_priority), current_min_jobs_per_user));
            lSortList(usl, so_usr);
            lSetUlong(current_pgrp, PGR_sort_me, 0);
            DPRINTF(("AT searching job\n"));
            at_trace();
         }

         /* Do we have a 'curser' which counts the number of jobs per user
            in this priority group? */

         if (current_min_jobs_per_user == -1) {
            current_min_jobs_per_user = lGetUlong(lFirst(usl), USR_nrunning_dl);
            DPRINTF(("curr_jpu = %d\n", current_min_jobs_per_user));
         }

         /* USERSORT - take into respect all users with the same number of running jobs 
                       and return the earliest one */

         /* find pointer of first user having 
            'current_min_jobs_per_user' jobs running */
         user=lFirst(usl); 
         while (lGetUlong(user, USR_nrunning_dl) < current_min_jobs_per_user)
            user = lNext(user);

         do { 
            lListElem *t_job_array;

            while (user && lGetUlong(user, USR_nrunning_dl) == current_min_jobs_per_user) {
               t_job_array = at_first_in_jrl(user, USR_job_references, USR_sort_me, USR_current);

               if (t_job_array)
                  DPRINTF(("USERSORT: %s "u32"\n", lGetString(user, USR_name),
                        lGetUlong(t_job_array, JB_job_number)));
               else
                  DPRINTF(("USERSORT: no more jobs of user %s\n", lGetString(user, USR_name)));

               if (!job_array || (t_job_array &&
                     (lGetUlong(t_job_array, JB_submission_time) < lGetUlong(job_array, JB_submission_time)
                     ||(lGetUlong(t_job_array, JB_submission_time) == lGetUlong(job_array, JB_submission_time) &&
                     lGetUlong(t_job_array, JB_job_number) < lGetUlong(job_array, JB_job_number))))) {
                  job_array = t_job_array;
               }
               user = lNext(user);
            }
 
            if (!job_array ) {
               if (user) {
                  DPRINTF(("USERSORT: found no job of %d jobs users - continue with %d jobs users\n", 
                           current_min_jobs_per_user, current_min_jobs_per_user+1));
                  current_min_jobs_per_user = current_min_jobs_per_user+1;

                  if (scheddconf.maxujobs && current_min_jobs_per_user >= scheddconf.maxujobs) {
                     DPRINTF(("USERSORT: reached maxujobs limit of %d\n", scheddconf.maxujobs));
                     current_min_jobs_per_user = -1;
                  }
               } else {
                  DPRINTF(("USERSORT: no more jobs in priority group "u32"\n",
                                    lGetUlong(current_pgrp, PGR_priority)));
                  current_min_jobs_per_user = -1;
               }
            }
         } while (!job_array && current_min_jobs_per_user != -1);
      }
   } while (!job_array && (current_pgrp = lNext(current_pgrp)));
 
   DEXIT;
   return job_array;
}

static lListElem *at_first_in_jrl(
lListElem *jrl_container,
int nm_jrl,
int nm_sort,
int nm_curr 
) {
   lList *jrl;
   lListElem *current_jr, *job_array = NULL;

   DENTER(TOP_LAYER, "at_first_in_jrl");

   jrl = lGetList(jrl_container, nm_jrl);

   /* sort on demand if neccessary */
   if (lGetUlong(jrl_container, nm_sort)) {
      lSortList(jrl, so_jrl);
      lSetUlong(jrl_container, nm_sort, 0);
      DPRINTF(("### ### ### ### #### SORTING JOB REFERENCES ### ### ### ### ####\n"));
      at_trace();
   }

   if (!(current_jr = lGetRef(jrl_container, nm_curr))) {
      current_jr = lFirst(jrl);
   }

   do { 
      int dispatchable = !sge_is_job_category_rejected_(lGetRef(current_jr, JRL_category));
      if (!dispatchable) {
         DPRINTF(("AT: category "SFQ" of job "u32" can't be dispatched\n", 
            lGetString(lGetRef(current_jr, JRL_category), CT_str), 
                  lGetUlong(current_jr, JRL_jobid)));
      } else {
         HashTableLookup(runnable_job_arrays,  (const void *)(u_long)lGetUlong(current_jr, JRL_jobid), 
                  (const void**)&job_array);
         DPRINTF(("AT: job "u32" is %scontained in hash list: %p\n", lGetUlong(current_jr, JRL_jobid), job_array?"":"NOT ", job_array));
      }
   } while (!job_array && (current_jr=lNext(current_jr)));
      
   lSetRef(jrl_container, nm_curr, current_jr);

   DEXIT;
   return job_array;
}
