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
#include "sge_select_queue.h" 
#include "debit.h"
#include "sort_hosts.h"
#include "sge_pe.h"
#include "sge_job.h"
#include "sge_centry.h"
#include "sgermon.h"
#include "sge_resource_utilization.h"

#include "sge_qinstanceL.h"
#include "sge_qinstance.h"
#include "subordinate_schedd.h"
#include "sge_subordinate.h"
#include "sge_orderL.h"
#include "sge_log.h"
#include "msg_schedd.h"

static int 
debit_job_from_queues(lListElem *job, lList *selected_queue_list, lList *global_queue_list, 
                      lList *complex_list, order_t *orders);

/* -------------------------------------------------------------

   debit_scheduled_job()

   The following objects will get changed to represent the debitations:

      host_list
         - the load gets increased according the granted list 
         - changes sort order of host list 
           (this sort order is used to get positions for the queues)

      queue_list 
         - the number of free slots gets reduced
         - subordinated queues that will get suspended by 
           the qmaster get marked as suspended

      pe
         - the number of free slots gets reduced
      
      sort_hostlist
         - if the sort order of the host_list is changed, 
           sort_hostlist is set to 1

      orders_list
         - needed to warn on jobs that were dispatched into 
           queues and get suspended on subordinate in the very 
           same interval

   The other objects get not changed and are needed to present
   and interprete the debitations on the upper objects:

      job
      granted
         - list that contains one element for each queue
           describing what has to be debited
      complex_list
         - needed to interprete the jobs -l requests and 
           the load correction

   1st NOTE: 
      The debitations will be lost if you pass local copies
      of the global lists to this function and your scheduler
      will try to put all jobs on one queue (or other funny 
      decisions).

      But this can be a feature if you use local copies to
      test what happens if you schedule a job to a specific
      queue (not tested). 

   2nd NOTE: 
      This function is __not__ responsible for any consistency 
      checking of your slot allocation! E.g. you will get no 
      error if you try to debit a job from a queue where the 
      jobs user has no access.

*/
int debit_scheduled_job(
const sge_assignment_t *a, /* all information describing the assignemnt */
int *sort_hostlist,  /* do we have to resort the hostlist? */
order_t *orders,  /* needed to warn on jobs that were dispatched into
                        queues and get suspended on subordinate in the very
                        same interval */
bool now,             /* if true this is or will be a running job
                         false for all jobs that must only be put into the schedule */
const char *type      /* a string as forseen with serf_record_entry() 
                         'type' parameter (may be NULL) */
) {
   DENTER(TOP_LAYER, "debit_scheduled_job");

   if (!a) {
      DEXIT;
      return -1;
   }

   if (now) {
      if (a->pe)
         pe_debit_slots(a->pe, a->slots, a->job_id);
      debit_job_from_hosts(a->job, a->gdil, a->host_list, a->centry_list, sort_hostlist);
      debit_job_from_queues(a->job, a->gdil, a->queue_list, a->centry_list, orders);
   }

   add_job_utilization(a, type);

   DEXIT;
   return 0;
}

/*
 * Here
 *
 *   - we reduce the amount of free slots in the queue.
 *   - we activte suspend_on_subordinate to prevent
 *     scheduling on queues that will get suspended
 *   - we debit consumable resouces of queue
 *
 * to represent the job again we use the tagged selected queue list
 * (same game as calling sge_create_orders())
 * (would be better to use the granted_destin_identifier_list of the job)
 *
 * order_t *orders    needed to warn on jobs that get dispatched and suspended
 *                    on subordinate in the very same interval 
 */
static int 
debit_job_from_queues(lListElem *job, lList *granted, lList *global_queue_list, lList *centry_list,
                      order_t *orders) 
{
   int qslots, total;
   unsigned int tagged;
   const char *qname;
   lListElem *gel, *qep, *so;
   int ret = 0;

   DENTER(TOP_LAYER, "debit_job_from_queue");

   /* use each entry in sel_q_list as reference into the global_queue_list */
   for_each(gel, granted ) {

      tagged = lGetUlong(gel, JG_slots);
      if (tagged) {
         /* find queue */
         qname = lGetString(gel, JG_qname);
         qep = lGetElemStr(global_queue_list, QU_full_name, qname);

         /* increase used slots */
         qslots = qinstance_slots_used(qep);

         /* precompute suspensions for subordinated queues */
         total = lGetUlong(qep, QU_job_slots);
         for_each (so, lGetList(qep, QU_subordinate_list)) {
            if (!tst_sos(qslots,        total, so)  &&  /* not suspended till now */
                 tst_sos(qslots+tagged, total, so)) {   /* but now                */
               ret |= sos_schedd(lGetString(so, SO_name), global_queue_list);

               /* warn on jobs that were dispatched into that queue in
                  the same scheduling interval based on the orders list */
               {
                  lListElem *order;
                  for_each (order, orders->jobStartOrderList) {
                     if (lGetUlong(order, OR_type) != ORT_start_job)
                        continue;
                     if (lGetSubStr(order, OQ_dest_queue, lGetString(so, SO_name), OR_queuelist)) {
                        WARNING((SGE_EVENT, MSG_SUBORDPOLICYCONFLICT_UUSS, u32c(lGetUlong(job, JB_job_number)),
                        u32c(lGetUlong(order, OR_job_number)), qname, lGetString(so, SO_name)));
                     }
                  }
                  
                  for_each (order, orders->sent_job_StartOrderList) {
                     if (lGetUlong(order, OR_type) != ORT_start_job)
                        continue;
                     if (lGetSubStr(order, OQ_dest_queue, lGetString(so, SO_name), OR_queuelist)) {
                        WARNING((SGE_EVENT, MSG_SUBORDPOLICYCONFLICT_UUSS, u32c(lGetUlong(job, JB_job_number)),
                        u32c(lGetUlong(order, OR_job_number)), qname, lGetString(so, SO_name)));
                     }
                  }

               }
            }
         }

         DPRINTF(("REDUCING SLOTS OF QUEUE %s BY %d\n", qname, tagged));

         qinstance_debit_consumable(qep, job, centry_list, tagged);
      }
   }

   DEXIT;
   return ret;
}
