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
#include <string.h>

#include "sge.h"
#include "sge_job_qmaster.h"
#include "sge_ja_task.h"
#include "sge_give_jobs.h"
#include "sge_m_event.h"
#include "read_write_queue.h"
#include "read_write_job.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time_eventL.h"
#include "time_event.h"
#include "msg_qmaster.h"
#include "sge_job.h"
#include "sge_queue.h"

void sge_c_ack(char *host, char *commproc, sge_pack_buffer *pb);
static void sge_c_job_ack(char *, char *, u_long32, u_long32, u_long32);
static void sge_c_event_ack(char *, char *, u_long32, u_long32, u_long32);

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
         /* an execd sends a job specific acknowledge 
            ack_ulong == jobid of received job */
         sge_c_job_ack(host, commproc, ack_tag, ack_ulong, ack_ulong2);
         break;

      case ACK_EVENT_DELIVERY:
         sge_c_event_ack(host, commproc, ack_tag, ack_ulong, ack_ulong2);
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
   lListElem *qep, *jep = NULL, *jatep = NULL;

   DENTER(TOP_LAYER, "sge_c_job_ack");

   if (strcmp(prognames[EXECD], commproc) &&
      strcmp(prognames[QSTD], commproc)) {
      ERROR((SGE_EVENT, MSG_COM_ACK_S, commproc));
      DEXIT;
      return;
   }

   switch (ack_tag) {
   case TAG_SIGJOB:
      DPRINTF(("TAG_SIGJOB\n"));
      /* ack_ulong is the jobid */
      if (!(jep = job_list_locate(Master_Job_List, ack_ulong))) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKOWNJOB_U, u32c(ack_ulong) ));
         DEXIT;
         return;
      }
      jatep = job_search_task(jep, NULL, ack_ulong2, 0);
      if (jatep == NULL) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKNOWNTASKOFJOB_UU, u32c(ack_ulong2), u32c(ack_ulong)));
         DEXIT;
         return;

      }

      if (!(qep = queue_list_locate(Master_Queue_List,  
                        lGetString(jatep, JAT_master_queue)))) {
         ERROR((SGE_EVENT, MSG_COM_ACK_US, u32c(ack_ulong), 
                lGetString(jatep, JAT_master_queue) ?
                           lGetString(jatep, JAT_master_queue) :
                           MSG_COM_NOQUEUE));
         DEXIT;
         return;
      }
      break;

   case TAG_SIGQUEUE:
      /* ack_ulong is the queueid */
/*       DPRINTF(("TAG_SIGQUEUE\n")); */
      if (!(qep = lGetElemUlong(Master_Queue_List, QU_queue_number, ack_ulong))) {
         ERROR((SGE_EVENT, MSG_COM_ACK_QUEUE_U, u32c(ack_ulong)));
         DEXIT;
         return;
      }
      break;

   default:
      ERROR((SGE_EVENT, MSG_COM_ACK_UNKNOWN));
      DEXIT;
      return;
   }

   switch (ack_tag) {
   case TAG_SIGQUEUE:
      DPRINTF(("QUEUE %s: SIGNAL ACK\n", lGetString(qep, QU_qname)));
      lSetUlong(qep, QU_pending_signal, 0);
      te_delete(TYPE_SIGNAL_RESEND_EVENT, lGetString(qep, QU_qname), 0, 0);
      cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);
      break;
   case TAG_SIGJOB:
      DPRINTF(("JOB "u32": SIGNAL ACK\n", lGetUlong(jep, JB_job_number)));
      lSetUlong(jatep, JAT_pending_signal, 0);
      te_delete(TYPE_SIGNAL_RESEND_EVENT, NULL, ack_ulong, ack_ulong2);
      job_write_spool_file(jep, ack_ulong2, NULL, SPOOL_DEFAULT); 
      break;
   }

   DEXIT;
   return;
}


/*******************************************************************/
static void sge_c_event_ack(
char *host, 
char *commproc, 
u_long32 ack_tag, 
u_long32 event_number,
u_long32 ev_id
) {
   lListElem *event_client;

   DENTER(TOP_LAYER, "sge_c_event_ack");

   /* search commproc in event client list */
   event_client = lGetElemUlong(EV_Clients, EV_id, ev_id);
   if (event_client == NULL) {
      ERROR((SGE_EVENT, MSG_COM_NO_EVCLIENTWITHID_U, u32c(ev_id)));
      DEXIT;
      return;
   }

   sge_ack_event(event_client, event_number);

   DEXIT;   
   return;
}
