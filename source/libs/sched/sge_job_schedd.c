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

static int user_sort = 0;

#define IDLE 0

#ifdef WIN32NATIVE
#	define strcasecmp( a, b) stricmp( a, b)
#	define strncasecmp( a, b, n) strnicmp( a, b, n)
#endif

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

/* ----------------------------------------

   sge_split_job_finished()

   splits from the incoming job list (1st arg) all 
   jobs that are waiting to be deleted JFINISHED (2nd arg)

   returns:
      0 successful
     -1 errors in functions called by sge_split_job_finished

*/
int sge_split_job_finished(
lList **jobs,        /* JB_Type */
lList **finished,     /* JB_Type */
char *finished_name 
) {
   lCondition *where;
   int ret;
   lListElem *job;
   lCondition *where1, *where2;

   DENTER(TOP_LAYER, "sge_split_job_finished");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   /* split jobs */
   where = lWhere("%T(%I->%T(%I!=%u))", lGetListDescr(*jobs), JB_ja_tasks, JAT_Type, 
      JAT_status, JFINISHED);
   ret = lSplit(jobs, finished, finished_name, where);

   where1 = lWhere("%T(%I!=%u)", JAT_Type, JAT_status, JFINISHED);
   where2 = lWhere("%T(%I==%u)", JAT_Type, JAT_status, JFINISHED); 

   for_each(job, *jobs) {
      lListElem *task, *new_job;
      int CopyJob = 0;

      for_each(task, lGetList(job, JB_ja_tasks)) {
         u_long32 status;

         status = lGetUlong(task, JAT_status);
         if (!(status != JFINISHED)) {
            CopyJob = 1;
            break;
         }
      }
      if (CopyJob) {
         lList *tmp_lp = NULL;

         if (*finished == NULL)
            *finished = lCreateList(finished_name?finished_name:"", (*jobs)->descr);
         new_job = lCopyElem(job);

         lXchgList(job, JB_ja_tasks, &tmp_lp);
         tmp_lp = lSelectDestroy(tmp_lp, where1);
         lXchgList(job, JB_ja_tasks, &tmp_lp);

         lXchgList(new_job, JB_ja_tasks, &tmp_lp);
         tmp_lp = lSelectDestroy(tmp_lp, where2);
         lXchgList(new_job, JB_ja_tasks, &tmp_lp);
         
         ret=lAppendElem(*finished, new_job);
      }
   }

   lFreeWhere(where1);
   lFreeWhere(where2);
   lFreeWhere(where);

   DEXIT;
   return ret;
}


/* ----------------------------------------

   sge_split_job_running()

   splits from the incoming job list (1st arg) all 
   jobs that are running (2nd arg)

   returns:
      0 successful
     -1 errors in functions called by sge_split_job_running

*/
int sge_split_job_running(
lList **jobs,        /* JB_Type */
lList **running,     /* JB_Type */
char *running_name 
) {
   lListElem *job, *nxt_job;

   DENTER(TOP_LAYER, "sge_split_job_running");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   if (!*jobs) {
      /* no jobs - nothing to do -  no error */
      DEXIT;
      return 0;
   }

   nxt_job = lFirst(*jobs);
   while ((job=nxt_job)) {
      lListElem *task, *nxt_task;
      lList *pending_task_lp = lGetList(job, JB_ja_tasks), 
            *running_task_lp = NULL;
      nxt_job = lNext(job);

      /* dechain all non-pending tasks, collect them in running_task_lp */ 
      nxt_task = lFirst(pending_task_lp);
      while ((task=nxt_task)) {
         nxt_task = lNext(task);
         if (lGetUlong(task, JAT_status) != IDLE) {
            lDechainElem(pending_task_lp, task);
            if (!running_task_lp)
               running_task_lp = lCreateList("running", JAT_Type);
            lAppendElem(running_task_lp, task);
         }
      }

      /* ensure a running task job-array container exists and store running tasks there */
      if (running_task_lp) {
         lListElem *running_job;
         if (!(running_job = lGetElemUlong(*running, JB_job_number, lGetUlong(job, JB_job_number)))) {
             running_job = lCopyElem(job); /* AH: in worst case this copies in vain 1.000.000
                                              elements in JB_ja_tasks. Need masked lCopyElem()
                                              to improve this */
             if (!*running)
                *running = lCreateList("running", (*jobs)->descr);
             lAppendElem(*running, running_job);
         }
         lSetList(running_job, JB_ja_tasks, running_task_lp);
      }

      /* remove pending task job-array container if empty */
      if (!lGetNumberOfElem(pending_task_lp)) {
         lRemoveElem(*jobs, job);
      }
   } 

   DEXIT;
   return 0;
}


/****** sge_job_schedd/sge_move_to_running() ***********************************
*  NAME
*     sge_move_to_running() -- move first task in job to running list
*
*  SYNOPSIS
*     int sge_move_to_running(lList **jobs, lList **running, lListElem *job) 
*
*******************************************************************************/
int sge_move_to_running(
lList **jobs,        /* JB_Type */
lList **running,     /* JB_Type */
lListElem *job
) {
   lList *pending_task_lp;
   lListElem *running_job, *task;
   int ntasks;

   DENTER(TOP_LAYER, "sge_move_to_running");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   if (!*jobs) {
      /* no jobs - nothing to do -  no error */
      DEXIT;
      return 0;
   }

   /* dechain first task of this job */ 
   pending_task_lp = lGetList(job, JB_ja_tasks);
   ntasks = lGetNumberOfElem(pending_task_lp);
   task = lFirst(pending_task_lp);
   lSetUlong(task, JAT_status, JRUNNING);
   lDechainElem(pending_task_lp, task);

   /* ensure a running task job-array container exists and store the running task */
   if (!(running_job = lGetElemUlong(*running, JB_job_number, lGetUlong(job, JB_job_number)))) {
      lList *tmp = NULL;
       /* do not copy the JB_ja_tasks list, because the entries are not needed 
          for the running task job-array container and can be very long */
      lXchgList(job, JB_ja_tasks, &tmp);
      running_job = lCopyElem(job); 
      lXchgList(job, JB_ja_tasks, &tmp);
      if (!*running)
          *running = lCreateList("running", (*jobs)->descr);
      lAppendElem(*running, running_job);
      lSetList(running_job, JB_ja_tasks, lCreateList("running tasks", JAT_Type));
   }
   lAppendElem(lGetList(running_job, JB_ja_tasks), task);

   DPRINTF(("STILL %d of formerly %d tasks\n", 
         lGetNumberOfElem(pending_task_lp), ntasks));

   /* remove pending task job-array container if empty */
   if (!lGetNumberOfElem(pending_task_lp)) {
      lRemoveElem(*jobs, job);
   }

   DEXIT;
   return 0;
}

/* ----------------------------------------

   sge_split_job_wait_at_time()

   splits from the incoming job list (1st arg) all 
   jobs that are running (2nd arg)

   returns:
      0 successful
     -1 errors in functions called by sge_split_job_wait_at_time

*/
int sge_split_job_wait_at_time(
lList **jobs,        /* JB_Type */
lList **waiting,     /* JB_Type */
char *waiting_name,
u_long32 now 
) {
   lCondition *where;
   int ret;
   lList *lp = NULL;
   int do_free_list = 0;

   DENTER(TOP_LAYER, "sge_split_job_wait_at_time");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   if (!waiting) {
       waiting = &lp;
       do_free_list = 1;
   }

   /* split jobs */
   where = lWhere("%T(%I == %u || %I <= %u)", 
         lGetListDescr(*jobs),
            JB_execution_time, 0,
            JB_execution_time, now);
   ret = lSplit(jobs, waiting, waiting_name, where);

   if (*waiting) {
      lListElem *job;

      for_each(job, *waiting) 
         schedd_add_message(lGetUlong(job, JB_job_number), SCHEDD_INFO_EXECTIME_);

      if (monitor_next_run)
         schedd_log_list(MSG_LOG_JOBSDROPPEDEXECUTIONTIMENOTREACHED  , *waiting, JB_job_number);

      DPRINTF(("execution time: " u32 "\n", lGetUlong(lFirst(*waiting), JB_execution_time)));
      if (do_free_list) {
         lFreeList(*waiting);
         *waiting = NULL;
      }
   }
   lFreeWhere(where);

   DEXIT;
   return ret;
}

/*-------------------------------------------------------
 *
 *  sge_split_job_error()
 *
 *  splits from the incoming job list (1st arg) all 
 *  jobs that are in error state
 *
 *  returns:
 *     0 successful
 *    -1 errors in called functions 
 *-------------------------------------------------------*/
int sge_split_job_error(
lList **jobs,        /* JB_Type */
lList **error,     /* JB_Type */
char *error_name 
) {
   lCondition *where;
   int ret;
   lListElem *job, *ja_task;
   lList *lp = NULL;
   int do_free_list = 0;
   lCondition *where1, *where2;

   DENTER(TOP_LAYER, "sge_split_job_error");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   where = lWhere("%T(%I->%T(%I==%u))", lGetListDescr(*jobs), JB_ja_tasks, JAT_Type, JAT_suitable, 1);
   where1 = lWhere("%T(%I==%u)", JAT_Type, JAT_suitable, 1);
   where2 = lWhere("%T(%I!=%u)", JAT_Type, JAT_suitable, 1);

   for_each(job, *jobs) {
      DPRINTF(("sge_split_job_error(1): visiting %d array-jobs\n", 
            lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         if ((lGetUlong(ja_task, JAT_state) & JERROR)) {
            lSetUlong(ja_task, JAT_suitable, 0);
            continue;
         }
         lSetUlong(ja_task, JAT_suitable, 1);
      }
   }

   if (!error) {
       error = &lp;
       do_free_list = 1;
   }

   /* split jobs */
   ret = lSplit(jobs, error, error_name, where);

   for_each(job, *jobs) {
      lListElem *task, *new_job;
      int CopyJob = 0;

      DPRINTF(("sge_split_job_error(2): visiting %d array-jobs\n", 
            lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
      for_each(task, lGetList(job, JB_ja_tasks)) {
         u_long32 Suitable;

         Suitable = lGetUlong(task, JAT_suitable);
         if (!(Suitable == 1)) {
            CopyJob = 1;
            break;
         }
      }
      if (CopyJob) {
         lList *tmp_lp = NULL; 
         if (lp == NULL)
            lp=lCreateList(error_name?error_name:"", (*jobs)->descr);
         new_job = lCopyElem(job);

         lXchgList(job, JB_ja_tasks, &tmp_lp);
         tmp_lp = lSelectDestroy(tmp_lp, where1);
         lXchgList(job, JB_ja_tasks, &tmp_lp);

         lXchgList(new_job, JB_ja_tasks, &tmp_lp);
         tmp_lp = lSelectDestroy(tmp_lp, where2);
         lXchgList(new_job, JB_ja_tasks, &tmp_lp);

         ret=lAppendElem(lp, new_job);
      }
   }
   if (*error) {
      lListElem* job;

      for_each(job, *error)
         schedd_add_message(lGetUlong(job, JB_job_number), SCHEDD_INFO_JOBINERROR_);
      if (monitor_next_run)
         schedd_log_list(MSG_LOG_JOBSDROPPEDERRORSTATEREACHED , *error, JB_job_number);
      if (do_free_list) {
         lFreeList(*error);
         *error = NULL;
      }
   }

   lFreeWhere(where);
   lFreeWhere(where1);
   lFreeWhere(where2);

   DEXIT;
   return ret;
}

/*------------------------------------------------------
 *  sge_split_job_hold()
 *
 *  splits from the incoming job list (1st arg) all 
 *
 *  jobs that are set on hold 
 *
 *  returns:
 *     0 successful
 *    -1 errors in called functions 
 *------------------------------------------------------*/
int sge_split_job_hold(
lList **jobs,        /* JB_Type */
lList **waiting,     /* JB_Type */
char *waiting_name 
) {
   lCondition *where;
   int ret;
   lListElem *job, *ja_task;
   lList *lp = NULL;
   int i, do_free_list = 0;
   char buffer[256];
   lCondition *where1, *where2;

   static int bitmask[] = { 
      MINUS_H_TGT_USER,
      MINUS_H_TGT_OPERATOR,
      MINUS_H_TGT_SYSTEM
   };

   static char *perm_name[] = {
      "user",
      "operator",
      "system",
   };

   DENTER(CULL_LAYER, "sge_split_job_hold");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   where = lWhere("%T(%I->%T(%I==%u))", lGetListDescr(*jobs), JB_ja_tasks, JAT_Type, JAT_suitable, 1);
   where1 = lWhere("%T(%I==%u)", JAT_Type, JAT_suitable, 1);
   where2 = lWhere("%T(%I!=%u)", JAT_Type, JAT_suitable, 1);

   for (i=0; i<3; i++) {

      for_each(job, *jobs) {
         DPRINTF(("sge_split_job_hold(1): visiting %d array-jobs\n", 
               lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
         for_each (ja_task, lGetList(job, JB_ja_tasks)) {
            if (lGetUlong(ja_task, JAT_hold) & bitmask[i]) {
               DPRINTF(("job "u32"."u32" is blocked by a %s hold\n",
                    lGetUlong(job, JB_job_number), lGetUlong(ja_task, JAT_task_number), perm_name[i]));
               lSetUlong(ja_task, JAT_suitable, 0);
               continue;
            }
            lSetUlong(ja_task, JAT_suitable, 1);
         }
      }

      if (!waiting) {
          waiting = &lp;
          do_free_list = 1;
      }

      /* split jobs */
      ret = lSplit(jobs, waiting, waiting_name, where);

      for_each(job, *jobs) {
         lListElem *task, *new_job;
         int CopyJob = 0;

         DPRINTF(("sge_split_job_hold(2): visiting %d array-jobs\n", 
               lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
         for_each(task, lGetList(job, JB_ja_tasks)) {
            u_long32 Suitable;

            Suitable = lGetUlong(task, JAT_suitable);
            if (!(Suitable == 1)) {
               CopyJob = 1;
               break;
            }
         }
         if (CopyJob) {
            lList *tmp_lp;

            if (lp == NULL)
               lp=lCreateList(waiting_name?waiting_name:"", (*jobs)->descr);
            new_job = lCopyElem(job);
            lXchgList(job, JB_ja_tasks, &tmp_lp);
            tmp_lp = lSelectDestroy(tmp_lp, where1);
            lXchgList(job, JB_ja_tasks, &tmp_lp);

            lXchgList(new_job, JB_ja_tasks, &tmp_lp);
            tmp_lp = lSelectDestroy(tmp_lp, where2);
            lXchgList(new_job, JB_ja_tasks, &tmp_lp);

            ret=lAppendElem(lp, new_job);
         }
      }

      if (*waiting) {
         lListElem* mes_job;
        
         for_each(mes_job, *waiting) 
            schedd_add_message(lGetUlong(mes_job, JB_job_number), SCHEDD_INFO_JOBHOLD_S, perm_name[i]);

         sprintf(buffer, MSG_LOG_JOBSDROPPEDBECAUSEOFXHOLD_S , perm_name[i]);

         if (monitor_next_run)
            schedd_log_list(buffer, *waiting, JB_job_number);

         if (do_free_list) {
            lFreeList(*waiting);
            *waiting = NULL;
         }
      }

   }

   lFreeWhere(where2);
   lFreeWhere(where1);
   lFreeWhere(where);
   DEXIT;
   return ret;
}


/*--------------------------------------------------------------
   sge_split_job_wait_predecessor()

   splits from the incoming job list (1st arg) all 
   jobs that wait for completion of other jobs (2nd arg)

   returns:
      0 successful
     -1 errors in functions called by sge_split_job_wait_predecessor

 *--------------------------------------------------------------*/
int sge_split_job_wait_predecessor(
lList **jobs,        /* JB_Type */
lList **waiting,     /* JB_Type */
char *waiting_name 
) {
   lList *pre_jobs;
   lListElem *job, *nxt_job;
   lList *lp = NULL;
   int do_free_list = 0;

   DENTER(TOP_LAYER, "sge_split_job_wait_predecessor");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   if (!waiting) {
      waiting = &lp;
      do_free_list = 1;
   }

   /* put jobs waiting for a predecessor job in *waiting list */
   nxt_job = lFirst(*jobs);
   while ((job=nxt_job)) {
      nxt_job = lNext(job);

      if ((pre_jobs=lGetList(job, JB_jid_predecessor_list))) {
         lDechainElem(*jobs, job);
         if (!*waiting)
            *waiting = lCreateList("waiting", lGetElemDescr(job));
         lAppendElem(*waiting, job);
      }
   }

   for_each(job, *waiting)
      schedd_add_message(lGetUlong(job, JB_job_number), SCHEDD_INFO_JOBDEPEND_);

   if (monitor_next_run)
      schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEDEPENDENCIES , *waiting, JB_job_number);

   if (do_free_list) {
      lFreeList(*waiting);
      *waiting = NULL;
   }

   DEXIT;
   return 0;
}


/*------------------------------------------------------------------------------
   sge_split_job_ckpt_restricted()

   splits those jobs from the incoming job list (1st arg)
   which cannot be processed due to their ckpt requirements
   (e.g. the requested CKPT-object is currently not configured).
   The jobs which cannot run are returned to list arg 2.

   returns:
      0 successful
     -1 errors in functions called by sge_split_job_pe_restricted
 *------------------------------------------------------------------------------*/
int sge_split_job_ckpt_restricted(
lList **jobs,           /* JB_Type */
lList **restricted,     /* JB_Type */
char *restricted_name,
lList *ckpt_list        /* CK_Type */
) {
   int ret;
   lListElem *job, *ckpt;
   char *ckpt_name;
   lCondition *where;

   DENTER(TOP_LAYER, "sge_split_job_ckpt_restricted");

   if (!jobs) {
      DEXIT;
      return -1;
   }

   for_each(job, *jobs) {
      lListElem *ja_task;

      DPRINTF(("sge_split_job_ckpt_restricted(1): visiting %d array-jobs\n", 
            lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         if ((ckpt_name = lGetString(job, JB_checkpoint_object))) {

            ckpt = lGetElemStr(ckpt_list, CK_name, ckpt_name);
            if (!ckpt) {
               CRITICAL((SGE_EVENT, MSG_EVENT_CKPTOBJXFORJOBYNOTFOUND_SI ,
                        ckpt_name, (int)lGetUlong(job, JB_job_number)));
               lSetUlong(ja_task, JAT_suitable, 0);
               schedd_add_message(lGetUlong(job, JB_job_number), SCHEDD_INFO_CKPTNOTFOUND_S,
                  ckpt_name);
               continue;
            }

         }
         lSetUlong(ja_task, JAT_suitable, 1);
      }
   }

   /* split jobs */
   where = lWhere("%T(%I->%T(%I==%u))", lGetListDescr(*jobs), JB_ja_tasks, JAT_Type, JAT_suitable, 1);
   ret = lSplit(jobs, restricted, restricted_name, where);
   lFreeWhere(where);

   DEXIT;
   return ret;
}




/*------------------------------------------------------------------*/
lList *filter_max_running(
lList *pending_jobs,
lList *jc_list,      /* either user_list or group_list */
int max_jobs,        /* either conf.maxujobs or conf.maxgjobs */
int elem             /* either JB_owner or JB_group */
) {
   lCondition *where, *exceed;
   lListElem *jce;

   if (!max_jobs)    /* no limitations --> nothing to do */
      return pending_jobs;

   /* for all elements of jc_list exceeding the limits */
   exceed = lWhere("%T(%I>=%u)", JC_Type, JC_jobs, max_jobs);
   for_each_where(jce, jc_list, exceed) {

      DPRINTF(("USER %s reached limit of %d jobs\n", 
            lGetString(jce, JC_name), max_jobs));
      /* for all users/groups exceeding the limits drop their pending jobs */
      where = lWhere("%T(%I!=%s)", lGetListDescr(pending_jobs), 
            elem, lGetString(jce, JC_name));

      if (monitor_next_run && pending_jobs) {
         lList *lp_dropped;
         lEnumeration *what;
         lCondition *nowhere;

         what = lWhat("%T(%I)", lGetListDescr(pending_jobs), JB_job_number);
         if (what) {
            lListElem* new_elem;

            nowhere = lWhere("%T(%I==%s)", lGetListDescr(pending_jobs), 
                  elem, lGetString(jce, JC_name));
            lp_dropped = lSelect("dropped", pending_jobs, nowhere, what);

            for_each(new_elem, lp_dropped)
               schedd_add_message(lGetUlong(new_elem, JB_job_number), SCHEDD_INFO_USRGRPLIMIT_); 
            schedd_log_list(MSG_LOG_JOBSDROPPEDBECAUSEUSRGRPLIMIT , lp_dropped, JB_job_number);

            lFreeWhat(what);
            lFreeWhere(nowhere);
            lFreeList(lp_dropped);
         }
      }
      
      /* re-assignment of pending_jobs in case NULL is returned */
      pending_jobs = lSelectDestroy(pending_jobs, where);

      lFreeWhere(where);
   }
   lFreeWhere(exceed);

   return pending_jobs;
}


lList *filter_max_running_1step(
lList *pending_jobs,
lList *running_jobs,
lList **jc_list,     /* either user_list or group_list */
int max_jobs,        /* either conf.maxujobs or conf.maxgjobs */
int elem             /* either JB_owner or JB_group */
) {
   lListElem *job;

   if (!max_jobs)    /* no limitations --> nothing to do */
      return pending_jobs;

   for_each(job, running_jobs) /* inc. the # of jobs a user/group is running */
      sge_inc_jc(jc_list, lGetString(job, elem), lGetNumberOfElem(lGetList(job, JB_ja_tasks)));


   pending_jobs = filter_max_running(pending_jobs, *jc_list, max_jobs, elem);

   return pending_jobs;
}

void sge_dec_jc(
lList **jcpp, /* JC_Type */
char *name,
int slots 
) {
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

void sge_inc_jc(
lList **jcpp, /* JC_Type */
char *name,
int slots 
) {
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
      DPRINTF(("rebuild_jc(1): visiting %d array-jobs\n", 
            lGetNumberOfElem(lGetList(job, JB_ja_tasks))));
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
char *owner,
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
char *name 
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
char *name,
lListElem *jep,
lListElem *dcep 
) {
   char *strval;
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
char *qhostname 
) {
   lListElem *gdil_ep;
   int nslots = 0;

   for_each (gdil_ep, granted) {
      if (qhostname && 
            hostcmp(qhostname, lGetString(gdil_ep, JG_qhostname)))
         continue; 
      nslots += lGetUlong(gdil_ep, JG_slots);
   }
   return nslots;
}

/*
   active_subtasks returns 1 if there are active subtasks for the queue
   and 0 if there are not.
*/

int active_subtasks(
lListElem *job,
char *qname 
) {
   lListElem *task, *ep, *ja_task, *task_task;
   char *task_qname;
   char *master_qname;

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
char *qhostname 
) {
   lList *task_list;
   lListElem *gdil_ep, *ja_task;
   int nslots = 0;

   for_each (gdil_ep, granted) {
      if (qhostname && 
            hostcmp(qhostname, lGetString(gdil_ep, JG_qhostname)))
         continue; 
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         task_list = lGetList(ja_task, JAT_task_list);
         if (task_list == NULL || active_subtasks(job, lGetString(gdil_ep, JG_qname)))
            nslots += lGetUlong(gdil_ep, JG_slots);
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
