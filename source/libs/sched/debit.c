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
#include "debit.h"
#include "sge_select_queue.h"
#include "sort_hosts.h"
#include "sge_pe_schedd.h"
#include "sgermon.h"

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
lListElem *job,      /* the job that was scheduled */
lList *granted,      /* a JB-List containing one element for each queue */
lList *queue_list,   /* in this queue list the job gets debited */
lListElem *pe,       /* in this pe the job gets debited */
lList *host_list,    /* in this host list the job gets debited */
lList *complex_list, /* needed for interpretation of jobs resource request */
int *sort_hostlist,  /* do we have to resort the hostlist? */
lList *orders_list   /* needed to warn on jobs that were dispatched into
                        queues and get suspended on subordinate in the very
                        same interval */
) {
   u_long32 pe_slots;

   DENTER(TOP_LAYER, "debit_scheduled_job");

   if (!job || !granted || !queue_list || !host_list ) {
      DEXIT;
      return -1;
   }

   debit_job_from_hosts(job, granted, host_list, complex_list, sort_hostlist);
   debit_job_from_queues(job, granted, queue_list, complex_list, &pe_slots, orders_list);

   if (pe)
      sge_debit_job_from_pe(pe, job, pe_slots);

   DEXIT;
   return 0;
}
