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
#include <time.h>

#include "cull.h"
#include "sge_conf.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_queueL.h"
#include "sge_orderL.h"
#include "sge_time.h"
#include "sge_select_queue.h"
#include "schedd_monitor.h"
#include "sge_parse_num_par.h"
#include "sge_orders.h"
#include "suspend_thresholds.h"
#include "sgermon.h"


static int select4suspension(lList *job_list, lListElem *queues, lListElem **jepp, lListElem **ja_taskp);
static int select4unsuspension(lList *job_list, lListElem *queues, lListElem **jepp, lListElem **ja_taskp);

/*
   select and suspend jobs in susp_queues 
*/
void suspend_job_in_queues(
lList *susp_queues,
lList *job_list,
lList **orderlist 
) {
   u_long32 now;
   int i, found;
   lListElem *jep, *qep, *ja_task;

   DENTER(TOP_LAYER, "suspend_job_in_queues");

   now = sge_get_gmt();
   for_each (qep, susp_queues) {
      u_long32 interval;      

      /* are suspend thresholds enabled? */
      parse_ulong_val(NULL, &interval, TYPE_TIM,
                  lGetString(qep, QU_suspend_interval), NULL, 0);

      if (interval == 0
          || !lGetUlong(qep, QU_nsuspend)
          || !lGetList(qep, QU_suspend_thresholds)) {
         continue;
      } 

      /* check time stamp */
      if (lGetUlong(qep, QU_last_suspend_threshold_ckeck) && 
         (lGetUlong(qep, QU_last_suspend_threshold_ckeck) + interval> now)) {
         continue;
      }

      for (i = 0, found = 1; 
           i < (int) lGetUlong(qep, QU_nsuspend) && found; 
           i++) {
         found = 0;
         /* find one running job in suspend queue */
         if (select4suspension(job_list, qep, &jep, &ja_task))
            break;

         /* generate suspend order for found job */
         found = 1;
         *orderlist = sge_create_orders(*orderlist, 
            ORT_suspend_on_threshold, jep, ja_task, NULL);

         DPRINTF(("++++ suspending job "u32"/"u32" on threshold\n", 
            lGetUlong(jep, JB_job_number), lGetUlong(ja_task, JAT_task_number)));

         /* prevent multiple selection of this job */
         lSetUlong(ja_task, JAT_state, lGetUlong(ja_task, JAT_state) 
            & JSUSPENDED_ON_THRESHOLD);
      }

      if (i==0 && !found) {
         DPRINTF(("found no jobs for sot in queue %s\n", 
            lGetString(qep, QU_qname)));
      }
   }
   
   DEXIT;
   return;
}


void unsuspend_job_in_queues(
lList *queue_list,
lList *job_list,
lList **orderlist 
) {
   u_long32 now;
   int i, found;
   lListElem *jep, *qep, *ja_task;

   DENTER(TOP_LAYER, "unsuspend_job_in_queues");

   now = sge_get_gmt();
   for_each (qep, queue_list) {
      u_long32 interval;

      /* are suspend thresholds enabled? */
      parse_ulong_val(NULL, &interval, TYPE_TIM,
                  lGetString(qep, QU_suspend_interval), NULL, 0);

       if (interval == 0
           || !lGetUlong(qep, QU_nsuspend)
           || !lGetList(qep, QU_suspend_thresholds)) {
          continue;
       } 

      /* check time stamp */
      if (lGetUlong(qep, QU_last_suspend_threshold_ckeck) && 
         (lGetUlong(qep, QU_last_suspend_threshold_ckeck) + 
         interval> now)) {
         char tmp[128];
         strcpy(tmp, sge_ctime(lGetUlong(qep, QU_last_suspend_threshold_ckeck)));
         DPRINTF(("queue was last checked at %s (interval = %s, now = %s)\n",
            tmp,
            lGetString(qep, QU_suspend_interval),
            sge_ctime(now)));
         continue;
      }

      for (i = 0, found = 1; 
           i < (int) lGetUlong(qep, QU_nsuspend) && found; 
           i++) {
         found = 0;
         /* find one running job in suspend queue */
         if (select4unsuspension(job_list, qep, &jep, &ja_task))
            break;

         /* generate unsuspend order for found job */
         found = 1;
         *orderlist = sge_create_orders(*orderlist, ORT_unsuspend_on_threshold, 
            jep, ja_task, NULL) ;

         DPRINTF(("---- unsuspending job "u32"/"u32" on threshold\n", 
            lGetUlong(jep, JB_job_number), lGetUlong(ja_task, JAT_task_number)));

         /* prevent multiple selection of this job */
         lSetUlong(ja_task, JAT_state, lGetUlong(ja_task, JAT_state) 
            & JSUSPENDED_ON_THRESHOLD);
      }
      
      if (i==0 && !found) {
         DPRINTF(("found no jobs for usot in queue %s\n", 
            lGetString(qep, QU_qname)));
      }
   }

   DEXIT;
   return;
}
   

static int select4suspension(
lList *job_list,
lListElem *qep,
lListElem **jepp,
lListElem **ja_taskp 
) {
   u_long32 jstate, qstate;
   lListElem *jep, *jshortest = NULL, *shortest = NULL, *ja_task;
   const char *qnm;

   DENTER(TOP_LAYER, "select4suspension");

   qnm = lGetString(qep, QU_qname);
   qstate = lGetUlong(qep, QU_state);
   if ((qstate & (QSUSPENDED|QCAL_SUSPENDED)) 
      || (qstate & QSUSPENDED_ON_SUBORDINATE)) {
      DEXIT;
      return -1;
   }
  
   for_each (jep, job_list) {
      lListElem *gqueue;

      /* job running */ 
      for_each (ja_task, lGetList(jep, JB_ja_tasks)) {
         jstate = lGetUlong(ja_task, JAT_state);
         if (!(jstate & JRUNNING) || 
             (jstate & JSUSPENDED) || (jstate & JSUSPENDED_ON_THRESHOLD)) {
            continue;
         }

         /*
         ** if the current task is
         **    a job / one task of an array-job
         **    a master-task of a pe-job with sub-tasks in this queue
         ** then it is a potential candidate which we could suspend
         */      
         for_each (gqueue, 
               lGetList(ja_task,JAT_granted_destin_identifier_list)) {
            if (!strcmp(qnm, lGetString(gqueue, JG_qname))) {
               break;
            }
         }
         if (!gqueue) {
            continue;
         }      

         /* select job that runs shortest time for suspension */
         if (!shortest || lGetUlong(shortest, JAT_start_time) < 
               lGetUlong(ja_task, JAT_start_time)) {
            shortest = ja_task;
            jshortest = jep;
         }
      }
   }

   if (shortest) {
      *jepp = jshortest; 
      *ja_taskp = shortest; 
   }

   DEXIT;
   return shortest?0:1;
}

static int select4unsuspension(
lList *job_list,
lListElem *qep,
lListElem **jepp,
lListElem **ja_taskp 
) {
   u_long32 jstate;
   lListElem *jep, *jlongest = NULL, *longest = NULL, *ja_task;
   const char *qnm;

   DENTER(TOP_LAYER, "select4unsuspension");

   qnm = lGetString(qep, QU_qname);

   for_each (jep, job_list) {
      for_each (ja_task, lGetList(jep, JB_ja_tasks)) {
         /* job must be suspended */ 
         jstate = lGetUlong(ja_task, JAT_state);
         if (!(jstate & JSUSPENDED_ON_THRESHOLD)) {
            continue;
         }

         /* is this the master queue of this job ? */
         if (strcmp(qnm, lGetString(lFirst(lGetList(ja_task, 
               JAT_granted_destin_identifier_list)), JG_qname))) {
            DTRACE;
            continue;
         }
         
         /* select task that runs longest time for unsuspension */
         if (!longest ||lGetUlong(longest, JAT_start_time) > lGetUlong(ja_task, JAT_start_time)) {
            longest = ja_task;
            jlongest = jep;
         }
      }
   }

   if (longest) {
      *jepp = jlongest; 
      *ja_taskp = longest; 
   }

   DEXIT;
   return longest?0:1;
}

