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
#include "sge_pe.h"
#include "sge_ja_task.h"
#include "sgermon.h"

#ifdef COMPILE_DC
#  include "ptf.h"
#endif

#include "dispatcher.h"
#include "job_log.h"
#include "execd_ticket.h"
#include "sge_log.h"
#include "msg_execd.h"
#include "sge_feature.h"
#include "sge_job.h"

extern volatile int jobs_to_start;

/*************************************************************************
 EXECD function called by dispatcher

 get a list of jobid/tickets tuples and pass them to the PTF
 *************************************************************************/

int execd_ticket(de, pb, apb, rcvtimeout, synchron, err_str, answer_error)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
u_long *rcvtimeout; 
int *synchron; 
char *err_str; 
int answer_error;
{
   u_long32 jobid, jataskid;
   double ticket;
   lListElem *job_ticket, *task_ticket;
   lList *ticket_modifier = NULL;

   DENTER(TOP_LAYER, "execd_ticket");

   while (pb_unused(pb)>0) {
      lList *jatasks = NULL;

      if (unpackint(pb, &jobid) || unpackint(pb, &jataskid) || unpackdouble(pb, &ticket)) {
         ERROR((SGE_EVENT, MSG_JOB_TICKETFORMAT));
         DEXIT;
         return 0;
      }
      DPRINTF(("got "u32" new tickets for job "u32"."u32"\n", ticket, jobid, jataskid));
      job_ticket = lAddElemUlong(&ticket_modifier, JB_job_number, jobid, JB_Type);   
      if (job_ticket) {
         task_ticket = lAddElemUlong(&jatasks, JAT_task_number, jataskid, JAT_Type);
         if (task_ticket)
            lSetDouble(task_ticket, JAT_ticket, ticket);
         lSetList(job_ticket, JB_ja_tasks, jatasks);
      }
   }
  
   DPRINTF(("got new tickets for %d jobs\n", lGetNumberOfElem(ticket_modifier)));
#ifdef COMPILE_DC
   if (feature_is_enabled(FEATURE_REPRIORISATION))  {
      int ptf_error;
      /* forward new tickets to ptf */
      if ((ptf_error=ptf_process_job_ticket_list(ticket_modifier))) {
         ERROR((SGE_EVENT, MSG_JOB_TICKETPASS2PTF_IS, 
            lGetNumberOfElem(ticket_modifier), 
            ptf_errstr(ptf_error)));
      }
   }
#endif

   ticket_modifier = lFreeList(ticket_modifier);

   DEXIT;
   return 0;
}


