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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "def.h"
#include "sge_conf.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_eventL.h"
#include "sge_answerL.h"
#include "slots_used.h"
#include "sge_sched.h"
#include "sge_signal.h"
#include "subordinate_qmaster.h"
#include "sge_m_event.h"
#include "sge_qmod_qmaster.h"
#include "msg_qmaster.h"
#include "sge_string.h"

extern lList *Master_Queue_List;


/* ------------------------------------------------

   suspend on subordinate
   using granted_destination_identifier_list (gdil)

   use jobs granted_destin_identifier_list
   to suspend queues on subordinate

   NOTE:
      we assume the associated job is already
      debited on all the queues that are referenced in gdil

   ------------------------------------------------ */
int sos_using_gdil(
lList *gdil,
u_long32 jobid  /* just for logging in case of errors */
) {
   char *qname;
   lListElem *ep, *so, *qep, *subqep;
   int ret = 0;

   DENTER(TOP_LAYER, "sos_using_gdil");

   for_each(ep, gdil) {

      qname = lGetString(ep, JG_qname);
      if (!(qep = lGetElemStr(Master_Queue_List, QU_qname, qname))) {
         ERROR((SGE_EVENT, MSG_JOB_SOSUSINGGDILFORJOBXCANTFINDREFERENCEQUEUEY_US, u32c(jobid), qname));
         ret = -1;
         continue; /* should never happen */
      }

      /* suspend subordinated queues in case of a state transition */
      for_each (so, lGetList(qep, QU_subordinate_list)) {

         /* skip if sos before this job came on this queue ? */
         if (tst_sos(qslots_used(qep) - (int)lGetUlong(ep, JG_slots), 
               lGetUlong(qep, QU_job_slots), lGetUlong(qep, QU_suspended_on_subordinate), so)) 
            continue;

         /* skip if not sos since job is on this queue ? */
         if (!tst_sos(qslots_used(qep), lGetUlong(qep, QU_job_slots), 
                  lGetUlong(qep, QU_suspended_on_subordinate), so))
            continue;

         /* suspend it */
         if (!(subqep = lGetElemStr(Master_Queue_List, QU_qname, lGetString(so, SO_qname)))) {
            DPRINTF(("WARNING: sos_using_gdil for job "u32": can't "
                  "find subordinated queue "SFQ, 
                  lGetString(qep, QU_qname), lGetString(so, SO_qname)));
            continue;
         }
         ret |= sos(subqep, 0);
      }
   }

   DEXIT;
   return ret;
}

/* -------------------------------------

   suspend on subordinate 

   suspends the given queue and 
   recursivly its subordinated queues
   
*/
int sos(
lListElem *qep,
int rebuild_cache 
) {
   int ret = 0;
   u_long32 state;

   DENTER(TOP_LAYER, "sos");

   /* increment sos counter */
   lSetUlong(qep, QU_suspended_on_subordinate, lGetUlong(qep, QU_suspended_on_subordinate) + 1);

   /* first sos ? */
   if (lGetUlong(qep, QU_suspended_on_subordinate)==1) { 
      
      DPRINTF(("QUEUE %s: suspend on subordinate\n", lGetString(qep, QU_qname)));
      /* send a signal if it is not already suspended by admin or calendar */
      if ((lGetUlong(qep, QU_state) & (QSUSPENDED|QCAL_SUSPENDED))==0 && !rebuild_cache) {
         ret |= sge_signal_queue(SGE_SIGSTOP, qep, NULL, NULL);
      }
      sge_add_event(sgeE_QUEUE_SUSPEND_ON_SUB, 0, 0, lGetString(qep, QU_qname), NULL); 
      state = lGetUlong(qep, QU_state);
      SETBIT(QSUSPENDED_ON_SUBORDINATE, state); 
      lSetUlong(qep, QU_state, state);
      
   } else {
      DPRINTF(("QUEUE %s: already suspended on subordinate\n", lGetString(qep, QU_qname)));
   }

   DEXIT; 
   return ret; 
}



/* ------------------------------------------------

   unsuspend on subordinate
   using granted_destination_identiefier_list (gdil)

   use jobs granted_destin_identifier_list to 
   unsuspend queues that were suspended on subordinate

   NOTE:
      we assume the associated job is still debited
      on all the queues that are referenced in gdil

   ------------------------------------------------ */
int usos_using_gdil(
lList *gdil,
u_long32 jobid  /* just for logging in case of errors */
) {
   char *qname;
   int ret = 0;
   lListElem *ep, *so, *qep, *subqep;

   DENTER(TOP_LAYER, "usos_using_gdil");

   for_each(ep, gdil) {

      qname = lGetString(ep, JG_qname);
      if (!(qep = lGetElemStr(Master_Queue_List, QU_qname, qname))) {
         /* inconsistent data */
         ERROR((SGE_EVENT, MSG_JOB_USOSUSINGGDILFORJOBXCANTFINDREFERENCEQUEUEY_US, u32c(jobid), qname));
         ret = -1;
         continue;
      }

      /* unsuspend subordinated queues if needed */
      for_each (so, lGetList(qep, QU_subordinate_list)) {

         /* skip if not sos since job is on this queue ? */
         if (!tst_sos(qslots_used(qep), lGetUlong(qep, QU_job_slots), 
            lGetUlong(qep, QU_suspended_on_subordinate), so))
            continue;

         /* skip if sos after job gone from this queue ? */
         if (tst_sos(qslots_used(qep) - (int)lGetUlong(ep, JG_slots), 
            lGetUlong(qep, QU_job_slots), lGetUlong(qep, QU_suspended_on_subordinate), so))
            continue;

         subqep = lGetElemStr(Master_Queue_List, QU_qname, lGetString(so, SO_qname));
         if (!subqep) {
            DPRINTF(("queue "SFQ": can't find "
                  "subordinated queue "SFQ".\n", 
                  lGetString(qep, QU_qname), lGetString(so, SO_qname)));
            continue;
         }
         ret |= usos(subqep, 0);
      }
   }

   DEXIT;
   return ret;
}

/* -------------------------------------

   unsuspend on subordinate 

   unsuspends the given queue and 
   recursivly its subordinated queues
   
*/
int usos(
lListElem *qep,
int rebuild_cache 
) {
   int ret = 0;
   u_long32 state;

   DENTER(TOP_LAYER, "usos");

   /* decrement sos counter */
   lSetUlong(qep, QU_suspended_on_subordinate, lGetUlong(qep, QU_suspended_on_subordinate) - 1);

   /* last sos ? */
   if (lGetUlong(qep, QU_suspended_on_subordinate)==0) { /* this also stops endless recursion */

      DPRINTF(("QUEUE %s: unsuspend on subordinate\n", lGetString(qep, QU_qname)));
      /* send a signal if it is not still suspended by admin or calendar */
      if ((lGetUlong(qep, QU_state) & (QSUSPENDED|QCAL_SUSPENDED))==0 && !rebuild_cache) {
         ret |= sge_signal_queue(SGE_SIGCONT, qep, NULL, NULL);
      }
      sge_add_event(sgeE_QUEUE_UNSUSPEND_ON_SUB, 0, 0, lGetString(qep, QU_qname), NULL); 
      state = lGetUlong(qep, QU_state);
      CLEARBIT(QSUSPENDED_ON_SUBORDINATE, state); 
      lSetUlong(qep, QU_state, state);

   } else {
      DPRINTF(("QUEUE %s: still suspended on subordinate\n", lGetString(qep, QU_qname)));
   }

   DEXIT; 
   return ret; 
}


/* in case of setting up unknown references are allowed */
int check_subordinate_list(
lList **alpp,
char *qname,
char *host,
u_long32 slots,
lList *sol,
int how 
) {
   lListElem *so;

   DENTER(TOP_LAYER, "check_subordinate_list");

   for_each (so, sol) {
      u_long32 so_threshold;
      char *so_qname;
      lListElem *refqep;

      so_qname = lGetString(so, SO_qname);
  
      /* check for recursions to our self */
      if (!strcmp(qname, so_qname)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_SUBITSELF_S, qname));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      /* try to find a referenced queue which does not exist */
      if (!(refqep=lGetElemStr(Master_Queue_List, QU_qname, so_qname))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNSUB_SS, so_qname, qname));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         if (how!=CHECK4SETUP) {
            DEXIT;
            return STATUS_EUNKNOWN; /* exit if not in SETUP case */
         }
         /* can't test if host of not found queue is same host  */
         /* need to be done in case of suspend_at adding a queue */

      } else {
         if (hostcmp(host, lGetString(refqep, QU_qhostname))) {
            ERROR((SGE_EVENT, MSG_SGETEXT_SUBHOSTDIFF_SSS, 
                  qname, so_qname, lGetString(refqep, QU_qhostname)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }

      /* hope this is not seen as pedantic */
      so_threshold = lGetUlong(so, SO_threshold);
      if (so_threshold && so_threshold>slots) {
         ERROR((SGE_EVENT, MSG_SGETEXT_SUBTHRESHOLD_EXCEEDS_SLOTS_SUSU, 
               qname, u32c(so_threshold), so_qname, u32c(slots)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

   }

   DEXIT;
   return STATUS_OK;
}

/* ------------------------------------
   count how often a queue must be 
   suspended from superordinated queues 
*/
int count_suspended_on_subordinate(
lListElem *queueep 
) {
   int n = 0;
   lListElem *so, *qep;

   DENTER(TOP_LAYER, "count_suspended_on_subordinate");

   for_each(qep, Master_Queue_List) {
      for_each(so, lGetList(qep, QU_subordinate_list)) {
         if (!strcmp(lGetString(so, SO_qname), lGetString(queueep, QU_qname))) {
            /* suspend the queue if neccessary */
            if (tst_sos(qslots_used(qep), lGetUlong(qep, QU_job_slots),
                  lGetUlong(qep, QU_suspended_on_subordinate), so))
               sos(queueep, 0);
               n++;
         }
      }
   }

   lSetUlong(queueep, QU_suspended_on_subordinate, n);

   DEXIT;
   return n;
}


/* ---------------------------------------------------

   This function has to copy all subordinated queues 
   of sol_in that actually are suspended by this queue 

   --------------------------------------------------- */
int copy_suspended(
lList **sol_out,
lList *sol_in,
int used,
int total,
int suspended_on_subordinate 
) {
   lListElem *so, *new_so;

   DENTER(TOP_LAYER, "copy_suspended");

   if (!sol_out)
      return -1;

   for_each (so, sol_in) {
      if (tst_sos(used, total, suspended_on_subordinate, so)) {
         if (!*sol_out)
            *sol_out = lCreateList("sos", SO_Type);

         new_so = lCopyElem(so);
         lAppendElem(*sol_out, new_so);
      }
   }

   DEXIT;
   return 0;
}


int suspend_all(
lList *sol,
int recompute_caches 
) {
   char *qnm;
   lListElem *so, *qep;
   int ret = 0;

   DENTER(TOP_LAYER, "suspend_all");

   for_each(so, sol) {
      qnm = lGetString(so, SO_qname);
      qep = lGetElemStr(Master_Queue_List, QU_qname, qnm);
      if (qep)
         ret |=sos(qep, recompute_caches);
   }

   DEXIT;
   return ret;
}

int unsuspend_all(
lList *sol,
int recompute_caches 
) {
   char *qnm;
   lListElem *so, *qep;
   int ret = 0;

   DENTER(TOP_LAYER, "unsuspend_all");

   for_each(so, sol) {
      qnm = lGetString(so, SO_qname);
      qep = lGetElemStr(Master_Queue_List, QU_qname, qnm);
      if (qep)
         ret |=usos(qep, recompute_caches);
   }

   DEXIT;
   return ret;
}

