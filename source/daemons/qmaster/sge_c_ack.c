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

#include "sge_c_ack.h"

#include <string.h>
#include "sge.h"
#include "sge_job_qmaster.h"
#include "sge_any_request.h"
#include "sge_ack.h"
#include "sge_answer.h"
#include "sge_ja_task.h"
#include "sge_give_jobs.h"
#include "sge_event_master.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time_eventL.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_job.h"
#include "sge_qinstance.h"
#include "sge_cqueue.h"
#include "sge_lock.h"

#include "spool/sge_spooling.h"


static void sge_c_job_ack(char *, char *, u_long32, u_long32, u_long32);

/****************************************************
 Master code.
 Handle ack requests
 Ack requests can be packed together in one packet.

 These are:
 - an execd sends an ack for a received job
 - an execd sends an ack for a signal delivery
 - the schedd sends an ack for received events

 if the counterpart uses the dispatcher ack_tag is the
 TAG we sent to the counterpart.
 ****************************************************/
void sge_c_ack(
char *host,
char *commproc,
sge_pack_buffer *pb 
) {

   u_long32 ack_tag, ack_ulong, ack_ulong2;

   DENTER(TOP_LAYER, "sge_c_ack");

   /* Do some validity tests */
   while (!unpackint(pb, &ack_tag)) {
      if (unpackint(pb, &ack_ulong)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 1));
         DEXIT;
         return;
      }
      if (unpackint(pb, &ack_ulong2)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 2));
         DEXIT;
         return;
      }

      DPRINTF(("ack_ulong = %ld, ack_ulong2 = %ld\n", ack_ulong, ack_ulong2));
      switch (ack_tag) {
      case TAG_SIGJOB:
      case TAG_SIGQUEUE:
         /* an execd sends a job specific acknowledge ack_ulong == jobid of received job */
         sge_c_job_ack(host, commproc, ack_tag, ack_ulong, ack_ulong2);
         break;

      case ACK_EVENT_DELIVERY:
         sge_handle_event_ack(ack_ulong2, ack_ulong);
         break;

      default:
         WARNING((SGE_EVENT, MSG_COM_UNKNOWN_TAG, u32c(ack_tag)));
         break;
      }
   }
  
   DEXIT;
   return;
}

/***************************************************************/
static void sge_c_job_ack(
char *host, 
char *commproc, 
u_long32 ack_tag, 
u_long32 ack_ulong,
u_long32 ack_ulong2 
) {
   lListElem *qinstance = NULL;
   lListElem *jep = NULL;
   lListElem *jatep = NULL;
   lList *answer_list = NULL;
/*   const char *qinstance_name = NULL; */

   DENTER(TOP_LAYER, "sge_c_job_ack");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   if (strcmp(prognames[EXECD], commproc) &&
      strcmp(prognames[QSTD], commproc)) {
      ERROR((SGE_EVENT, MSG_COM_ACK_S, commproc));
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return;
   }

   switch (ack_tag) {
   case TAG_SIGJOB:
      DPRINTF(("TAG_SIGJOB\n"));
      /* ack_ulong is the jobid */
      if (!(jep = job_list_locate(Master_Job_List, ack_ulong))) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKOWNJOB_U, u32c(ack_ulong) ));
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return;
      }
      jatep = job_search_task(jep, NULL, ack_ulong2);
      if (jatep == NULL) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKNOWNTASKOFJOB_UU, u32c(ack_ulong2), u32c(ack_ulong)));
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return;
      }

      DPRINTF(("JOB "u32": SIGNAL ACK\n", lGetUlong(jep, JB_job_number)));
      lSetUlong(jatep, JAT_pending_signal, 0);
      te_delete_one_time_event(TYPE_SIGNAL_RESEND_EVENT, ack_ulong, ack_ulong2, NULL);
      {
         dstring buffer = DSTRING_INIT;
         spool_write_object(&answer_list, spool_get_default_context(), jep, 
                            job_get_key(lGetUlong(jep, JB_job_number), 
                                        ack_ulong2, NULL, &buffer), 
                            SGE_TYPE_JOB);
         sge_dstring_free(&buffer);
      }
      answer_list_output(&answer_list);
      
      break;

   case TAG_SIGQUEUE:
      {
         lListElem *cqueue = NULL;

         for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
            lList *qinstance_list = lGetList(cqueue, CQ_qinstances);

            qinstance = lGetElemUlong(qinstance_list, 
                                      QU_queue_number, ack_ulong);
            if (qinstance != NULL) {
               break;
            }
         }
         if (qinstance == NULL) {
            ERROR((SGE_EVENT, MSG_COM_ACK_QUEUE_U, u32c(ack_ulong)));
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            DEXIT;
            return;
         }
      }
      
      DPRINTF(("QUEUE %s: SIGNAL ACK\n", lGetString(qinstance, QU_qname)));
               lSetUlong(qinstance, QU_pending_signal, 0);
      te_delete_one_time_event(TYPE_SIGNAL_RESEND_EVENT, 0, 0, lGetString(qinstance, QU_qname));
      spool_write_object(&answer_list, spool_get_default_context(), qinstance, 
                         lGetString(qinstance, QU_qname), SGE_TYPE_QINSTANCE);
      answer_list_output(&answer_list);
      break;

   default:
      ERROR((SGE_EVENT, MSG_COM_ACK_UNKNOWN));
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return;
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   
   DEXIT;
   return;
}
