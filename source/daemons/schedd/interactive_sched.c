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

/*------------------------------------------------------------------
 * CHECK ALL REMAINING JOBS WHETHER THEY ARE IMMEDIATE JOBS
 * IMMEDIATE JOBS NEED TO BE SCHEDULED INSTANTLY OR THEY 
 * HAVE TO BE REMOVED. THIS IS DONE BY GENERATING A ORDER OF TYPE 
 * ORT_remove_immediate_job 
 *------------------------------------------------------------------*/
int remove_immediate_jobs(
lList *job_list, /* JB_Type */
lList **opp      /* OR_Type */
) {
   lListElem *next_job, *job, *ja_task; 
   lList* lp;

   DENTER(TOP_LAYER, "remove_immediate_jobs");

   next_job = lFirst(job_list);
   while ((job = next_job)) {
      lListElem *range = NULL;
      lList *range_list = NULL;
      u_long32 ja_task_id;
      next_job = lNext(job);

      /* skip non immediate .. */
      if (!JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type)))
         continue;

      /* .. and non idle jobs */
      if ((lp =lGetList(job, JB_ja_tasks)) && 
            lGetUlong(lFirst(lp), JAT_status)==JIDLE) 
         continue;    
  
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         order_remove_immediate(job, ja_task, opp);
      }
      range_list = lGetList(job, JB_ja_n_h_ids);
      for_each(range, range_list) {
         for(ja_task_id = lGetUlong(range, RN_min);
             ja_task_id <= lGetUlong(range, RN_max);
             ja_task_id += lGetUlong(range, RN_step)) {  
            ja_task = job_get_ja_task_template_pending(job, ja_task_id);
            order_remove_immediate(job, ja_task, opp);
         }
      }
      lRemoveElem(job_list, job);
   }

   DEXIT;
   return 0;
}

int order_remove_immediate(
lListElem *job,     /* JB_Type */
lListElem *ja_task, /* JAT_Type */
lList **opp         /* OR_Type */
) {
   DENTER(TOP_LAYER, "order_remove_immediate");

   DPRINTF(("JOB "u32"."u32" can't get dispatched - removing\n", 
      lGetUlong(job, JB_job_number), lGetUlong(ja_task, JAT_task_number)));
   *opp = sge_create_orders(*opp, ORT_remove_immediate_job, job, ja_task, NULL, false) ;

   DEXIT;
   return !*opp;
}
