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

#include "sge_ja_task.h"
#include "sge_orderL.h"
#include "sge_orders.h"
#include "interactive_sched.h"
#include "sgermon.h"
#include "sge_range.h"
#include "sge_job.h"

static void remove_immediate_job(lList *job_list, lListElem *job, lList **opp,
                                 int remove_order);
static void order_remove_order_and_immediate(lListElem *job, lListElem *ja_task,
                                       lList **opp);

/*------------------------------------------------------------------
 * CHECK ALL REMAINING JOBS WHETHER THEY ARE IMMEDIATE JOBS
 * IMMEDIATE JOBS NEED TO BE SCHEDULED INSTANTLY OR THEY 
 * HAVE TO BE REMOVED. THIS IS DONE BY GENERATING A ORDER OF TYPE 
 * ORT_remove_immediate_job 
 *------------------------------------------------------------------*/
int remove_immediate_jobs(
lList *pending_job_list, /* JB_Type */
lList *running_job_list, /* JB_Type */
lList **opp              /* OR_Type */
) {
   lListElem *next_job, *job, *ep; 
   lList* lp;

   DENTER (TOP_LAYER, "remove_immediate_jobs");

   next_job = lFirst (pending_job_list);
   
   while ((job = next_job)) {
      lCondition *where = NULL;
      next_job = lNext(job);
      
      /* skip non immediate .. */
      if (!JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type)))
         continue;

      /* .. and non idle jobs */
      if ((lp = lGetList(job, JB_ja_tasks)) && 
            (ep = lFirst(lp)) && 
            lGetUlong(ep, JAT_status)==JIDLE)
         continue;    

      /* Prepare search condition for running list */
      where = lWhere("%T(%I==%u)", JB_Type, JB_job_number, lGetUlong (job, JB_job_number));
      
      /* Remove the job from the pending list */
      remove_immediate_job (pending_job_list, job, opp, 0);
      
      /* If the job also exists in the running list, we need to remove it there
       * as well since array jobs are all or nothing. */
      if ((job = lFindFirst (running_job_list, where)) != NULL) {
         remove_immediate_job (running_job_list, job, opp, 1);
      }
      
      where = lFreeWhere(where);      
   }

   DEXIT;
   return 0;
}

static void remove_immediate_job(
lList *job_list,         /* JB_Type */
lListElem *job,          /* JB_Type */
lList **opp,             /* OR_Type */
int remove_orders
) {
   lListElem *ja_task; 
   lListElem *range = NULL;
   lList *range_list = NULL;
   u_long32 ja_task_id;

   DENTER (TOP_LAYER, "remove_immediate_job");

   for_each (ja_task, lGetList(job, JB_ja_tasks)) {
      if (remove_orders) {
         order_remove_order_and_immediate(job, ja_task, opp);
      }
      else {
         order_remove_immediate(job, ja_task, opp);
      }
   }
   range_list = lGetList(job, JB_ja_n_h_ids);
   for_each(range, range_list) {
      for(ja_task_id = lGetUlong(range, RN_min);
          ja_task_id <= lGetUlong(range, RN_max);
          ja_task_id += lGetUlong(range, RN_step)) {  
         ja_task = job_get_ja_task_template_pending(job, ja_task_id);
         if (remove_orders) {
            order_remove_order_and_immediate(job, ja_task, opp);
         }
         else {
            order_remove_immediate(job, ja_task, opp);
         }
      }
   }
   lRemoveElem(job_list, job);
   
   DEXIT;
}

static void order_remove_order_and_immediate(
lListElem *job,     /* JB_Type */
lListElem *ja_task, /* JAT_Type */
lList **opp         /* OR_Type */
) {
   /* The possibility exists that this task is part of an array task, that it
    * already has earned an order to be scheduled, and that one or more other
    * tasks in this same job were not scheduled, resulting in this delete
    * order.  In this case, we have to remove the schedule order before we add
    * the delete order.  Otherwise, the qmaster will think we're trying to do
    * an ORT_remove_immediate_job on a non-idle task. */   
   lCondition *where = lWhere ("%T(%I==%u && %I==%u && %I==%u)", OR_Type,
                      OR_type, ORT_start_job,
                      OR_job_number, lGetUlong (job, JB_job_number),
                      OR_ja_task_number, lGetUlong (ja_task, JAT_task_number));
   lListElem *ep = lFindFirst (*opp, where);
   
   DENTER(TOP_LAYER, "order_remove_order_and_immediate");
   
   if (ep != NULL) {
      DPRINTF (("Removing job start order for job task %u.%u\n",
                lGetUlong (job, JB_job_number),
                lGetUlong (ja_task, JAT_task_number)));
      lRemoveElem (*opp, ep);
   }
   
   order_remove_immediate (job, ja_task, opp);
   lFreeWhere (where);
   
   DEXIT;
}

int order_remove_immediate(
lListElem *job,     /* JB_Type */
lListElem *ja_task, /* JAT_Type */
lList **opp         /* OR_Type */
) {
   DENTER(TOP_LAYER, "order_remove_immediate");

   DPRINTF(("JOB "u32"."u32" can't get dispatched - removing\n", 
      lGetUlong(job, JB_job_number), lGetUlong(ja_task, JAT_task_number)));
   
   *opp = sge_create_orders(*opp, ORT_remove_immediate_job, job, ja_task, NULL, false, true) ;

   DEXIT;
   return !*opp;
}
