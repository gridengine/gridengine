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

#include "sge_conf.h"
#include "sge.h"
#include "sge_pe.h"
#include "sge_ja_task.h"
#include "sge_any_request.h"
#include "sge_queue.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_orderL.h"
#include "sge_usageL.h"
#include "sge_schedd_conf.h"
#include "sgermon.h"
#include "commlib.h"
#include "sge_host.h"
#include "sge_signal.h"
#include "sge_job_qmaster.h"
#include "slots_used.h"
#include "sge_queue_qmaster.h"
#include "sge_follow.h"
#include "sge_sched.h"
#include "scheduler.h"
#include "sgeee.h"
#include "sge_support.h"
#include "sge_userprj_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "subordinate_qmaster.h"
#include "sge_sharetree_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_event_master.h"
#include "sge_queue_event_master.h"
#include "sge_feature.h"
#include "sge_prog.h"
#include "config.h"
#include "mail.h"
#include "sge_stringL.h"
#include "sge_messageL.h"
#include "sge_string.h"
#include "sge_security.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_sharetree.h"
#include "sge_todo.h"

#include "sge_spooling.h"

#include "msg_common.h"
#include "msg_evmlib.h"
#include "msg_qmaster.h"

/* static double get_usage_value(lList *usage, char *name); */

/**********************************************************************
 Gets an order and executes it.

 Return 0 if everything is fine or 

 -1 if the scheduler has sent an inconsistent order list but we think
    the next event delivery will correct this

 -2 if the scheduler has sent an inconsistent order list and we don't think
    the next event delivery will correct this

 -3 if delivery to an execd failed

 **********************************************************************/ 

int sge_follow_order(
lListElem *ep,
lList **alpp,
char *ruser,
char *rhost,
lList **topp  /* ticket orders ptr ptr */
) {
   int allowed;
   u_long32 job_number, task_number;
   const char *or_pe, *q_name=NULL;
   u_long32 or_type;
   int seq_no;
   lList *acl, *xacl;
   lListElem *jep, *qep, *master_qep, *oep, *hep, *master_host = NULL, *jatp = NULL;
   u_long32 force, state;
   u_long32 pe_slots = 0, q_slots, q_version, task_id_range = 0;
   lListElem *pe = NULL;
   char opt_sge[256] = ""; 
   lListElem *scheduler;

   DENTER(TOP_LAYER, "sge_follow_order");

   strcpy(SGE_EVENT, MSG_UNKNOWN_ERROR_NL);

   or_type=lGetUlong(ep, OR_type);
   seq_no = (int)lGetUlong(ep, OR_seq_no);
   force=lGetUlong(ep, OR_force);
   or_pe=lGetString(ep, OR_pe);

   scheduler = eventclient_list_locate(EV_ID_SCHEDD);
   if(scheduler == NULL) {
      ERROR((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      DEXIT;
      return -2;
   }

   /* last order has been sent again - ignore it */
   if (seq_no == lGetUlong(scheduler, EV_clientdata)) {
      WARNING((SGE_EVENT, MSG_IGNORE_ORDER_RETRY_I, seq_no));
      DEXIT;
      return STATUS_OK;
   }
   lSetUlong(scheduler, EV_clientdata, seq_no);

   DPRINTF(("-----------------------------------------------------------------------\n"));

   switch(or_type) {

   /* ----------------------------------------------------------------------- 
    * START JOB
    * ----------------------------------------------------------------------- */
   case ORT_start_job: {
      lList *gdil = NULL;

      DPRINTF(("ORDER ORT_start_job\n"));

      job_number=lGetUlong(ep, OR_job_number);
      if(!job_number) {
         ERROR((SGE_EVENT, MSG_JOB_NOJOBID));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -2;
      }
      task_number=lGetUlong(ep, OR_ja_task_number);
      if (!task_number) {
         ERROR((SGE_EVENT, MSG_JOB_NOTASKID));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -2;
      }
      jep = job_list_locate(Master_Job_List, job_number);
      if(!jep) {
         WARNING((SGE_EVENT, MSG_JOB_FINDJOB_U, u32c(job_number)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         /* try to repair schedd data */
         sge_add_event(NULL, 0, sgeE_JOB_DEL, job_number, 0, NULL, NULL);
         DEXIT;
         return -1;
      }

      DPRINTF(("ORDER to start Job %ld Task %ld\n", (long) job_number, 
               (long) task_number));      

      if (!force && 
          lGetUlong(jep, JB_version) != lGetUlong(ep, OR_job_version)) {
         ERROR((SGE_EVENT, MSG_ORD_OLDVERSION_UUU, 
               u32c(lGetUlong(ep, OR_job_version)), u32c(job_number), 
               u32c(task_number)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }

      /* search and enroll task */
      jatp = job_search_task(jep, NULL, task_number);
      if(jatp == NULL) {
         jatp = job_create_task(jep, NULL, task_number);
         sge_add_event(NULL, 0, sgeE_JATASK_ADD, job_number, task_number, NULL, jatp);
      }
      if (!jatp) {
         WARNING((SGE_EVENT, MSG_JOB_FINDJOBTASK_UU, u32c(task_number), 
                  u32c(job_number)));
         /* try to repair schedd data */
         sge_add_event(NULL, 0, sgeE_JATASK_DEL, job_number, task_number, 
                       NULL, NULL);
         DEXIT;
         return -1;
      }

      if (lGetUlong(jatp, JAT_status) != JIDLE) {
         ERROR((SGE_EVENT, MSG_ORD_TWICE_UU, u32c(job_number), 
                u32c(task_number)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }


      /* untag all queues */
      queue_list_clear_tags(Master_Queue_List);

      if (lGetString(jep, JB_pe)) {
         pe = pe_list_locate(Master_Pe_List, or_pe);
         if (!pe) {
            ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDPE_S, or_pe));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -2;
         }
         task_id_range = 0;
         lSetString(jatp, JAT_granted_pe, or_pe);  /* free me on error! */
      }

      master_qep = NULL;
       
      if (feature_is_enabled(FEATURE_SGEEE)) {
         /* fill number of tickets into job */
         lSetDouble(jatp, JAT_ticket, lGetDouble(ep, OR_ticket));
         sprintf(opt_sge, MSG_ORD_INITIALTICKETS_U, 
                 u32c((u_long32)lGetDouble(ep, OR_ticket)));

         if ((oep = lFirst(lGetList(ep, OR_queuelist)))) {
            lSetDouble(jatp, JAT_oticket, lGetDouble(oep, OQ_oticket));
            lSetDouble(jatp, JAT_fticket, lGetDouble(oep, OQ_fticket));
            lSetDouble(jatp, JAT_dticket, lGetDouble(oep, OQ_dticket));
            lSetDouble(jatp, JAT_sticket, lGetDouble(oep, OQ_sticket));
         }
      }

      for_each(oep, lGetList(ep, OR_queuelist)) {
         lListElem *gdil_ep;

         q_name    = lGetString(oep, OQ_dest_queue);
         q_version = lGetUlong(oep, OQ_dest_version);
         q_slots   = lGetUlong(oep, OQ_slots);

         /* ---------------------- 
          *  find and check queue 
          */
         if(!q_name) {
            ERROR((SGE_EVENT, MSG_OBJ_NOQNAME));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -2;
         }

         DPRINTF(("%sORDER #%d: start %d slots of job \"%d\" on"
                  " queue \"%s\" v%d%s\n", 
            force ? "FORCE ": "", seq_no, q_slots, job_number, q_name, 
               (int)q_version, 
               (feature_is_enabled(FEATURE_SGEEE) ? opt_sge : "") ));

         qep = queue_list_locate(Master_Queue_List, q_name);
         if (!qep) {
            ERROR((SGE_EVENT, MSG_QUEUE_UNABLE2FINDQ_S, q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -2;
         }

         /* the first queue is the master queue */
         if (!master_qep)
            master_qep = qep;

         /* check queue version */
         if ( !force && (q_version!=lGetUlong(qep, QU_version))) {
            ERROR((SGE_EVENT, MSG_ORD_QVERSION_UUS,
                  u32c(q_version), u32c(lGetUlong(qep, QU_version)), q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);

            /* try to repair schedd data */
            sge_add_queue_event(sgeE_QUEUE_MOD, qep);

            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }
         
         DPRINTF(("Queue version: %d\n", q_version));

         /* ensure that the jobs owner has access to this queue */
         /* job structure ?                                     */
         acl = lCopyList("acl", lGetList(qep, QU_acl));
         xacl = lCopyList("xacl", lGetList(qep, QU_xacl));
         allowed = sge_has_access_(lGetString(jep, JB_owner), lGetString(jep, JB_group), 
                                    acl, xacl, Master_Userset_List);
         lFreeList(acl); 
         lFreeList(xacl); 
         if (!allowed) {
            ERROR((SGE_EVENT, MSG_JOB_JOBACCESSQ_US, u32c(job_number), q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }

         /* ensure that this queue has enough free slots */
         if (lGetUlong(qep, QU_job_slots) - qslots_used(qep) < q_slots) {
            ERROR((SGE_EVENT, MSG_JOB_FREESLOTS_US, u32c(q_slots), q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }  
            
         /* handle QERROR */
         if (VALID(QERROR, lGetUlong(qep, QU_state))) {
            ERROR((SGE_EVENT, MSG_JOB_QMARKEDERROR_S, q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }  
         if (VALID(QCAL_SUSPENDED, lGetUlong(qep, QU_state))) {
            ERROR((SGE_EVENT, MSG_JOB_QSUSPCAL_S, q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }  
         if (VALID(QCAL_DISABLED, lGetUlong(qep, QU_state))) {
            ERROR((SGE_EVENT, MSG_JOB_QDISABLECAL_S, q_name));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -1;
         }  

         /* set tagged field in queue; this is needed for building */
         /* up job_list in the queue after successful job sending */
         lSetUlong(qep, QU_tagged, q_slots); 

         /* ---------------------- 
          *  find and check host 
          */
         if (!(hep=host_list_locate(Master_Exechost_List, 
                     lGetHost(qep, QU_qhostname)))) {
            ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDHOST_S, lGetHost(qep, QU_qhostname)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            lFreeList(gdil);
            lSetString(jatp, JAT_granted_pe, NULL);
            DEXIT;
            return -2;
         }
         if (hep) {
            lListElem *ruep;
            lList *rulp;
 
            rulp = lGetList(hep, EH_reschedule_unknown_list);
            if (rulp) {
               for_each(ruep, rulp) {
                  if (lGetUlong(jep, JB_job_number) == lGetUlong(ruep, RU_job_number)
                      && lGetUlong(jatp, JAT_task_number) == lGetUlong(ruep, RU_task_number)) {
                     ERROR((SGE_EVENT, MSG_JOB_UNABLE2STARTJOB_US, u32c(lGetUlong(ruep, RU_job_number)),
                        lGetHost(qep, QU_qhostname)));
                     answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
                     lFreeList(gdil);
                     lSetString(jatp, JAT_granted_pe, NULL);
                     DEXIT;
                     return -1;
                  }
               }
            }  
         }

         /* the master host is the host of the master queue */
         if (master_qep && !master_host) 
            master_host = hep;

         /* ------------------------------------------------ 
          *  build up granted_destin_identifier_list (gdil)
          */
         gdil_ep = lAddElemStr(&gdil, JG_qname, q_name, JG_Type); /* free me on error! */
         lSetHost(gdil_ep, JG_qhostname, lGetHost(qep, QU_qhostname));
         lSetUlong(gdil_ep, JG_slots, q_slots);

         /* ------------------------------------------------
          *  tag each gdil entry of slave exec host
          *  in case of sge controlled slaves 
          *  this triggers our retry for delivery of slave jobs
          *  and gets untagged when ack has arrived 
          */
         if (pe && lGetBool(pe, PE_control_slaves)) {

            if (feature_is_enabled(FEATURE_SGEEE)) {
               lSetDouble(gdil_ep, JG_ticket, lGetDouble(oep, OQ_ticket));
               lSetDouble(gdil_ep, JG_oticket, lGetDouble(oep, OQ_oticket));
               lSetDouble(gdil_ep, JG_fticket, lGetDouble(oep, OQ_fticket));
               lSetDouble(gdil_ep, JG_dticket, lGetDouble(oep, OQ_dticket));
               lSetDouble(gdil_ep, JG_sticket, lGetDouble(oep, OQ_sticket));
            }
     

            if (sge_hostcmp(lGetHost(master_host, EH_name), lGetHost(hep, EH_name))) {
               lListElem *first_at_host;

               /* ensure each host gets tagged only one time 
                  we tag the first entry for a host in the existing gdil */
               first_at_host = lGetElemHost(gdil, JG_qhostname, lGetHost(hep, EH_name));
               if (!first_at_host) {
                  ERROR((SGE_EVENT, MSG_JOB_HOSTNAMERESOLVE_US, 
                           u32c(lGetUlong(jep, JB_job_number)), 
                           lGetHost(hep, EH_name)  ));
               } else {
                  lSetUlong(first_at_host, JG_tag_slave_job, 1);   
               }
            } else 
               DPRINTF(("master host %s\n", lGetHost(master_host, EH_name)));
         }
         /* in case of a pe job update free_slots on the pe */
         if (pe) 
            pe_slots += q_slots;
      }

      /* fill in master_queue */
      lSetString(jatp, JAT_master_queue, lGetString(master_qep, QU_qname));
      lSetList(jatp, JAT_granted_destin_identifier_list, gdil);

      if (sge_give_job(jep, jatp, master_qep, pe, master_host)) {

         /* setting of queues in state unheard is done by sge_give_job() */
         sge_commit_job(jep, jatp, 7, COMMIT_DEFAULT);
         /* CB - This was sge_commit_job(jep, 2). It raised problems if a job
            could not be delivered. The jobslotsfree had been increased even if
            they where not decreased bevore. */

         ERROR((SGE_EVENT, MSG_JOB_JOBDELIVER_UU, 
                u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatp, JAT_task_number))));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -3;
      }
      sge_commit_job(jep, jatp, 0, COMMIT_DEFAULT);   /* mode==0 -> really accept when execd acks */
      trigger_job_resend(sge_get_gmt(), master_host, job_number, task_number);

      if (pe) {
         debit_job_from_pe(pe, pe_slots, job_number);
         sge_add_event(NULL, 0, sgeE_PE_MOD, 0, 0, lGetString(jatp, JAT_granted_pe), pe);
      }

      DPRINTF(("successfully handed off job \"" u32 "\" to queue \"%s\"\n",
               lGetUlong(jep, JB_job_number), lGetString(jatp, JAT_master_queue)));

      /* now after successfully (we hope) sent the job to execd 
         suspend all subordinated queues that need suspension */
      sos_using_gdil(lGetList(jatp, JAT_granted_destin_identifier_list), job_number);
   }    
      break;

   /* ----------------------------------------------------------------------- 
    * CHANGE TICKETS OF PENDING JOBS
    *
    * Modify the tickets of pending jobs for the sole purpose of being
    * able to display and sort the pending jobs list based on the
    * expected execution order.
    *
    * modifications performed on the job are not spooled 
    * ----------------------------------------------------------------------- */
   case ORT_ptickets:

      DPRINTF(("ORDER ORT_ptickets\n"));
      if (feature_is_enabled(FEATURE_SGEEE)) {

         lListElem *joker;
         int pos;

         job_number=lGetUlong(ep, OR_job_number);
         if(!job_number) {
            ERROR((SGE_EVENT, MSG_JOB_NOJOBID));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -2;
         }

         DPRINTF(("%sORDER #%d: job("u32")->ticket = "u32"\n", 
            force?"FORCE ":"", seq_no, job_number, 
            (u_long32)lGetDouble(ep, OR_ticket)));

         jep = job_list_locate(Master_Job_List, job_number);
         if(!jep) {
            WARNING((SGE_EVENT, MSG_JOB_UNABLE2FINDJOBORD_U, u32c(job_number)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return 0; /* it's ok - job has exited - forget about him */
         }
         task_number=lGetUlong(ep, OR_ja_task_number);
         if (!task_number) {
            ERROR((SGE_EVENT, MSG_JOB_NOTASKID));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -2;
         }
         jatp = job_search_task(jep, NULL, task_number);
         if (!jatp) {
            jatp = job_get_ja_task_template_pending(jep, task_number);
         }
         if (!jatp) {
            ERROR((SGE_EVENT, MSG_JOB_FINDJOBTASK_UU,  
                  u32c(task_number), u32c(job_number)));
            sge_add_event(NULL, 0, sgeE_JATASK_DEL, job_number, task_number, 
                          NULL, NULL);
            DEXIT;
            return -2;
         }
         if (!force && lGetUlong(jep, JB_version) != 
                                                lGetUlong(ep, OR_job_version)) {
            WARNING((SGE_EVENT, MSG_ORD_OLDVERSION_UUU, 
                     u32c(lGetUlong(ep, OR_job_version)), 
                     u32c(lGetUlong(jep, JB_job_number)),
                     u32c(task_number)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }

         if (lGetUlong(jatp, JAT_status) == JRUNNING ||
             lGetUlong(jatp, JAT_status) == JTRANSFERING ||
             lGetUlong(jatp, JAT_status) == JFINISHED) {

            WARNING((SGE_EVENT, MSG_JOB_CHANGEPTICKETS_UU, 
                     u32c(lGetUlong(jep, JB_job_number)), 
                     u32c(lGetUlong(jatp, JAT_task_number))));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }

         /* modify jobs ticket amount */
         lSetDouble(jatp, JAT_ticket, lGetDouble(ep, OR_ticket));
         DPRINTF(("TICKETS: "u32"."u32" "u32" tickets\n",
            lGetUlong(jep, JB_job_number),
            lGetUlong(jatp, JAT_task_number),
            (u_long32)lGetDouble(jatp, JAT_ticket)));

         /* check several fields to be updated */
         if ((joker=lFirst(lGetList(ep, OR_joker)))) {
            lListElem *joker_task;

            joker_task = lFirst(lGetList(joker, JB_ja_tasks));
            if ((pos=lGetPosViaElem(joker_task, JAT_oticket))>=0) 
               lSetDouble(jatp, JAT_oticket, lGetPosDouble(joker_task, pos));
            if ((pos=lGetPosViaElem(joker_task, JAT_dticket))>=0) 
               lSetDouble(jatp, JAT_dticket, lGetPosDouble(joker_task, pos));
            if ((pos=lGetPosViaElem(joker_task, JAT_fticket))>=0) 
               lSetDouble(jatp, JAT_fticket, lGetPosDouble(joker_task, pos));
            if ((pos=lGetPosViaElem(joker_task, JAT_sticket))>=0) 
               lSetDouble(jatp, JAT_sticket, lGetPosDouble(joker_task, pos));
            if ((pos=lGetPosViaElem(joker_task, JAT_fshare))>=0) 
               lSetUlong(jatp, JAT_fshare, lGetPosUlong(joker_task, pos));
            if ((pos=lGetPosViaElem(joker_task, JAT_share))>=0)           
               lSetDouble(jatp, JAT_share, lGetPosDouble(joker_task, pos));

         }
      } /* just ignore them being not in SGEEE mode */
      break;


   /* ----------------------------------------------------------------------- 
    * CHANGE TICKETS OF RUNNING/TRANSFERING JOBS
    *
    * Our aim is to collect all ticket orders of an gdi request
    * and to send ONE packet to each exec host. Here we just
    * check the orders for consistency. They get forwarded to 
    * execd having checked all orders.  
    * 
    * modifications performed on the job are not spooled 
    * ----------------------------------------------------------------------- */
   case ORT_tickets:

      DPRINTF(("ORDER ORT_tickets\n"));
      if (feature_is_enabled(FEATURE_SGEEE)) {

         lList *oeql;
         lListElem *joker;
         int pos, skip_ticket_order = 0;

         job_number=lGetUlong(ep, OR_job_number);
         if(!job_number) {
            ERROR((SGE_EVENT, MSG_JOB_NOJOBID));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -2;
         }

         DPRINTF(("%sORDER #%d: job("u32")->ticket = "u32"\n", 
            force?"FORCE ":"", seq_no, job_number, (u_long32)lGetDouble(ep, OR_ticket)));

         jep = job_list_locate(Master_Job_List, job_number);
         if(!jep) {
            WARNING((SGE_EVENT, MSG_JOB_UNABLE2FINDJOBORD_U, u32c(job_number)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return 0; /* it's ok - job has exited - forget about him */
         }
         task_number=lGetUlong(ep, OR_ja_task_number);
         if (!task_number) {
            ERROR((SGE_EVENT, MSG_JOB_NOTASKID));
            answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -2;
         }
         jatp = job_search_task(jep, NULL, task_number);
         if (!jatp) {
            ERROR((SGE_EVENT, MSG_JOB_FINDJOBTASK_UU,  
                  u32c(task_number), u32c(job_number)));
            sge_add_event(NULL, 0, sgeE_JATASK_DEL, job_number, task_number, NULL, NULL);
            DEXIT;
            return -2;
         }
         if (!force && lGetUlong(jep, JB_version) != lGetUlong(ep, OR_job_version)) {
            WARNING((SGE_EVENT, MSG_ORD_OLDVERSION_UUU, 
                     u32c(lGetUlong(ep, OR_job_version)), 
                     u32c(lGetUlong(jep, JB_job_number)),
                     u32c(task_number)));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }

         if (lGetUlong(jatp, JAT_status) != JRUNNING && 
             lGetUlong(jatp, JAT_status) != JTRANSFERING) {

            if (lGetUlong(jatp, JAT_status) != JFINISHED) {
               WARNING((SGE_EVENT, MSG_JOB_CHANGETICKETS_UU, 
                        u32c(lGetUlong(jep, JB_job_number)), 
                        u32c(lGetUlong(jatp, JAT_task_number))));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               DEXIT;
               return -1;
            }
            skip_ticket_order = 1;
         }

         if (!skip_ticket_order) {
            /* modify jobs ticket amount and spool job */
            lSetDouble(jatp, JAT_ticket, lGetDouble(ep, OR_ticket));
            DPRINTF(("TICKETS: "u32"."u32" "u32" tickets\n",
               lGetUlong(jep, JB_job_number),
               lGetUlong(jatp, JAT_task_number),
               (u_long32)lGetDouble(jatp, JAT_ticket)));

            /* check several fields to be updated */
            if ((joker=lFirst(lGetList(ep, OR_joker)))) {
               lListElem *joker_task;

               joker_task = lFirst(lGetList(joker, JB_ja_tasks));
               if ((pos=lGetPosViaElem(joker_task, JAT_oticket))>=0) 
                  lSetDouble(jatp, JAT_oticket, lGetPosDouble(joker_task, pos));
               if ((pos=lGetPosViaElem(joker_task, JAT_dticket))>=0) 
                  lSetDouble(jatp, JAT_dticket, lGetPosDouble(joker_task, pos));
               if ((pos=lGetPosViaElem(joker_task, JAT_fticket))>=0) 
                  lSetDouble(jatp, JAT_fticket, lGetPosDouble(joker_task, pos));
               if ((pos=lGetPosViaElem(joker_task, JAT_sticket))>=0) 
                  lSetDouble(jatp, JAT_sticket, lGetPosDouble(joker_task, pos));
               if ((pos=lGetPosViaElem(joker_task, JAT_fshare))>=0) 
                  lSetUlong(jatp, JAT_fshare, lGetPosUlong(joker_task, pos));
               if ((pos=lGetPosViaElem(joker_task, JAT_share))>=0)           
                  lSetDouble(jatp, JAT_share, lGetPosDouble(joker_task, pos));
            }

            /* add a copy of this order to the ticket orders list */
            if (!*topp) 
               *topp = lCreateList("ticket orders", OR_Type);

            /* If a ticket order has a queuelist, then this is a parallel job
               with controlled sub-tasks. We generate a ticket order for
               each host in the queuelist containing the total tickets for
               all job slots being used on the host */

            if ((oeql=lCopyList(NULL, lGetList(ep, OR_queuelist)))) {
               const char *oep_qname=NULL, *oep_hname=NULL;
               lListElem *oep_qep=NULL;

               /* set granted slot tickets */
               for_each(oep, oeql) {
                  lListElem *gdil_ep;
                  if ((gdil_ep=lGetSubStr(jatp, JG_qname, lGetString(oep, OQ_dest_queue),
                       JAT_granted_destin_identifier_list))) {
                     lSetDouble(gdil_ep, JG_ticket, lGetDouble(oep, OQ_ticket));
                     lSetDouble(gdil_ep, JG_oticket, lGetDouble(oep, OQ_oticket));
                     lSetDouble(gdil_ep, JG_fticket, lGetDouble(oep, OQ_fticket));
                     lSetDouble(gdil_ep, JG_dticket, lGetDouble(oep, OQ_dticket));
                     lSetDouble(gdil_ep, JG_sticket, lGetDouble(oep, OQ_sticket));
                  }
               }

               while((oep=lFirst(oeql))) {          
                  if (((oep_qname=lGetString(oep, OQ_dest_queue))) &&
                      ((oep_qep = queue_list_locate(Master_Queue_List,
                                                    oep_qname))) &&
                      ((oep_hname=lGetHost(oep_qep, QU_qhostname)))) {

                     const char *curr_oep_qname=NULL, *curr_oep_hname=NULL;
                     lListElem *curr_oep, *next_oep, *curr_oep_qep=NULL;
                     double job_tickets_on_host = lGetDouble(oep, OQ_ticket);
                     lListElem *newep;

                     for(curr_oep=lNext(oep); curr_oep; curr_oep=next_oep) {
                        next_oep = lNext(curr_oep);
                        if (((curr_oep_qname=lGetString(curr_oep, OQ_dest_queue))) &&
                            ((curr_oep_qep = queue_list_locate(Master_Queue_List, curr_oep_qname))) &&
                            ((curr_oep_hname=lGetHost(curr_oep_qep, QU_qhostname))) &&
                            !sge_hostcmp(oep_hname, curr_oep_hname)) {     /* CR SPEEDUP CANDIDATE */
                           job_tickets_on_host += lGetDouble(curr_oep, OQ_ticket);
                           lRemoveElem(oeql, curr_oep);
                        }
                     }
                     newep = lCopyElem(ep);
                     lSetDouble(newep, OR_ticket, job_tickets_on_host);
                     lAppendElem(*topp, newep);

                  } else
                     ERROR((SGE_EVENT, MSG_ORD_UNABLE2FINDHOST_S,
                            oep_qname ? oep_qname : MSG_OBJ_UNKNOWN));

                  lRemoveElem(oeql, oep);
               }

               lFreeList(oeql);

            } else
               lAppendElem(*topp, lCopyElem(ep));
         }
      } /* just ignore them being not in sge mode */
      break;

   /* ----------------------------------------------------------------------- 
    * REMOVE JOBS THAT ARE WAITING FOR SCHEDD'S PERMISSION TO GET DELETED
    *
    * Using this order schedd can only remove jobs in the 
    * "dead but not buried" state 
    * 
    * ----------------------------------------------------------------------- */
   case ORT_remove_job:
   /* ----------------------------------------------------------------------- 
    * REMOVE IMMEDIATE JOBS THAT COULD NOT GET SCHEDULED IN THIS PASS
    *
    * Using this order schedd can only remove idle immediate jobs 
    * (former ORT_remove_interactive_job)
    * ----------------------------------------------------------------------- */
   case ORT_remove_immediate_job:

      job_number=lGetUlong(ep, OR_job_number);
      if(!job_number) {
         ERROR((SGE_EVENT, MSG_JOB_NOJOBID));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -2;
      }
      task_number=lGetUlong(ep, OR_ja_task_number);
      if (!task_number) {
         ERROR((SGE_EVENT, MSG_JOB_NOTASKID));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -2;
      }
      DPRINTF(("%sORDER #%d: remove %sjob "u32"."u32"\n", 
         force?"FORCE ":"", seq_no,
         or_type==ORT_remove_immediate_job?"immediate ":"" ,
         job_number, task_number));
      jep = job_list_locate(Master_Job_List, job_number);
      if(!jep) {
         ERROR((SGE_EVENT, MSG_JOB_FINDJOB_U, u32c(job_number)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         /* try to repair schedd data */
         sge_add_event(NULL, 0, sgeE_JOB_DEL, job_number, task_number, NULL, NULL);
         DEXIT;
         return -1;
      }
      jatp = job_search_task(jep, NULL, task_number);
      if(jatp == NULL) {
         jatp = job_create_task(jep, NULL, task_number);
         sge_add_event(NULL, 0, sgeE_JATASK_ADD, job_number, task_number, NULL, jatp);
      }
      if (!jatp) {
         ERROR((SGE_EVENT, MSG_JOB_FINDJOBTASK_UU, u32c(task_number), u32c(job_number)));
         sge_add_event(NULL, 0, sgeE_JATASK_DEL, job_number, task_number, NULL, NULL);
         /* try to repair schedd data */
         DEXIT;
         return -1;
      }


      if (or_type==ORT_remove_job) {

         if (lGetUlong(jatp, JAT_status) != JFINISHED) {
            ERROR((SGE_EVENT, MSG_JOB_REMOVENOTFINISHED_U, u32c(lGetUlong(jep, JB_job_number))));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
    
         /* remove it */
         sge_commit_job(jep, jatp, 5, COMMIT_DEFAULT);
      } else {
         if (!JOB_TYPE_IS_IMMEDIATE(lGetUlong(jep, JB_type))) {
            if(lGetString(jep, JB_script_file))
               ERROR((SGE_EVENT, MSG_JOB_REMOVENONINTERACT_U, u32c(lGetUlong(jep, JB_job_number))));
            else
               ERROR((SGE_EVENT, MSG_JOB_REMOVENONIMMEDIATE_U,  u32c(lGetUlong(jep, JB_job_number))));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
         if (lGetUlong(jatp, JAT_status) != JIDLE) {
            ERROR((SGE_EVENT, MSG_JOB_REMOVENOTIDLEIA_U, u32c(lGetUlong(jep, JB_job_number))));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return -1;
         }
         INFO((SGE_EVENT, MSG_JOB_NOFREERESOURCEIA_U, u32c(lGetUlong(jep, JB_job_number))));

         /* remove it */
         sge_commit_job(jep, jatp, 6, COMMIT_DEFAULT);
      }
      break;

   /* ----------------------------------------------------------------------- 
    * REPLACE A USER/PROJECT'S 
    * 
    * - UP_usage   
    * - UP_usage_time_stamp   
    * - UP_long_term_usage  
    * - UP_debited_job_usage  
    *
    * Using this order schedd can debit usage on users/projects
    * both orders are handled identically except target list
    * ----------------------------------------------------------------------- */
   case ORT_update_project_usage:
   case ORT_update_user_usage:
      DPRINTF(("ORDER: ORT_update_project_usage/ORT_update_user_usage\n"));
      if (feature_is_enabled(FEATURE_SGEEE)) {
         lListElem *up_order, *up, *ju, *up_ju, *next;
         int pos;
         const char *up_name;
         lList *tlp;

         DPRINTF(("%sORDER #%d: update %d users/prjs\n", 
            force?"FORCE ":"", 
            seq_no, lGetNumberOfElem(lGetList(ep, OR_joker))));

         for_each (up_order, lGetList(ep, OR_joker)) {
            if ((pos=lGetPosViaElem(up_order, UP_name))<0 || 
                  !(up_name = lGetString(up_order, UP_name)))
               continue;

            DPRINTF(("%s %s usage updating with %d jobs\n",
               or_type==ORT_update_project_usage?"project":"user",
               up_name, lGetNumberOfElem(lGetList(up_order, 
               UP_debited_job_usage))));

            if (!(up=userprj_list_locate( 
                  or_type==ORT_update_project_usage ? Master_Project_List : Master_User_List, up_name )))
               /* order contains reference to unknown user/prj object */
               continue;

#if 0
            {
               lList *old_usage = lGetList(up, UP_usage);
               lList *new_usage = lGetList(up_order, UP_usage);
               INFO((SGE_EVENT, "ORDER #%d: %s %s %d update users/prjs (%8.3f/%8.3f/%8.3f) -> (%8.3f/%8.3f/%8.3f)\n", 
                  (int)seq_no, 
                  or_type==ORT_update_project_usage?"project":"user",
                  up_name,
                  lGetNumberOfElem(lGetList(up_order, UP_debited_job_usage)),

                  get_usage_value(old_usage, USAGE_ATTR_CPU),
                  get_usage_value(old_usage, USAGE_ATTR_MEM),
                  get_usage_value(old_usage, USAGE_ATTR_IO),

                  get_usage_value(new_usage, USAGE_ATTR_CPU),
                  get_usage_value(new_usage, USAGE_ATTR_MEM),
                  get_usage_value(new_usage, USAGE_ATTR_IO)));
            }
#endif

            if ((pos=lGetPosViaElem(up_order, UP_version))>=0 &&
                (lGetPosUlong(up_order, pos) != lGetUlong(up, UP_version))) {
               /* order contains update for outdated user/project usage */
               ERROR((SGE_EVENT, MSG_ORD_USRPRJVERSION_UUS, u32c(lGetPosUlong(up_order, pos)),
                      u32c(lGetUlong(up, UP_version)), up_name));
               /* Note: Should we apply the debited job usage in this case? */
               continue;
            }

            lSetUlong(up, UP_version, lGetUlong(up, UP_version)+1);

            if ((pos=lGetPosViaElem(up_order, UP_project))>=0) {
               lSwapList(up_order, UP_project, up, UP_project);
            }

            if ((pos=lGetPosViaElem(up_order, UP_usage_time_stamp))>=0)
               lSetUlong(up, UP_usage_time_stamp, lGetPosUlong(up_order, pos));

            if ((pos=lGetPosViaElem(up_order, UP_usage))>=0) {
               lSwapList(up_order, UP_usage, up, UP_usage);
            }

            if ((pos=lGetPosViaElem(up_order, UP_long_term_usage))>=0) {
               lSwapList(up_order, UP_long_term_usage, up, UP_long_term_usage);
            }

            /* update old usage in up for each job appearing in
               UP_debited_job_usage of 'up_order' */
            next = lFirst(lGetList(up_order, UP_debited_job_usage));
            while ((ju = next)) {
               next = lNext(ju);

               job_number = lGetUlong(ju, UPU_job_number);
              
               /* seek for existing debited usage of this job */
               if ((up_ju=lGetSubUlong(up, UPU_job_number, job_number,
                                       UP_debited_job_usage))) { 
                  
                  /* if passed old usage list is NULL, delete existing usage */
                  if (lGetList(ju, UPU_old_usage_list) == NULL) {

                     lRemoveElem(lGetList(up_order, UP_debited_job_usage), ju);
                     lRemoveElem(lGetList(up, UP_debited_job_usage), up_ju);

                  } else {

                     /* still exists - replace old usage with new one */
                     DPRINTF(("updating debited usage for job "u32"\n", job_number));
                     lSwapList(ju, UPU_old_usage_list, up_ju, UPU_old_usage_list);
                  }

               } else {
                  /* unchain ju element and chain it into our user/prj object */
                  DPRINTF(("adding debited usage for job "u32"\n", job_number));
                  lDechainElem(lGetList(up_order, UP_debited_job_usage), ju);

                  if (lGetList(ju, UPU_old_usage_list) != NULL) {
                     /* unchain ju element and chain it into our user/prj object */
                     if (!(tlp=lGetList(up, UP_debited_job_usage))) {
                        tlp = lCreateList(up_name, UPU_Type);
                        lSetList(up, UP_debited_job_usage, tlp);
                     }
                     lInsertElem(tlp, NULL, ju);
                  } else {
                     /* do not chain in empty empty usage records */
                     lFreeElem(ju);
                  }
               }
            }

            /* spool */
            {
               lList *answer_list = NULL;
               spool_write_object(&answer_list, 
                                  spool_get_default_context(), up, up_name, 
                                  or_type == ORT_update_user_usage ? 
                                  SGE_TYPE_USER : SGE_TYPE_PROJECT);
               answer_list_output(&answer_list);
            }
            sge_add_event(NULL, 0,
               or_type==ORT_update_user_usage?sgeE_USER_MOD:sgeE_PROJECT_MOD,
               0, 0, up_name, up);
         }
      } /* just ignore them being not in sge mode */
      break;

   /* ----------------------------------------------------------------------- 
    * FILL IN SEVERAL SCHEDULING VALUES INTO QMASTERS SHARE TREE 
    * TO BE DISPLAYED BY QMON AND OTHER CLIENTS
    * ----------------------------------------------------------------------- */
   case ORT_share_tree:
      DPRINTF(("ORDER: ORT_share_tree\n"));
      if (feature_is_enabled(FEATURE_SGEEE) && 
           !sge_init_node_fields(lFirst(Master_Sharetree_List)) &&
	        update_sharetree(alpp, Master_Sharetree_List, lGetList(ep, OR_joker))) {
         /* alpp gets filled by update_sharetree */
         DPRINTF(("%sORDER #%d: ORT_share_tree\n", force?"FORCE ":"", seq_no));
         DEXIT;
         return -1;
      } /* just ignore it being not in sge mode */
       
      break;

   /* ----------------------------------------------------------------------- 
    * UPDATE FIELDS IN SCHEDULING CONFIGURATION 
    * ----------------------------------------------------------------------- */
   case ORT_sched_conf:
      DPRINTF(("ORDER: ORT_sched_conf\n"));
      if (feature_is_enabled(FEATURE_SGEEE)) {
         lListElem *joker, *sc;      
         int pos;

         DPRINTF(("%sORDER #%d: ORT_sched_conf\n", force?"FORCE ":"", seq_no));
         joker = lFirst(lGetList(ep, OR_joker));
         sc = lFirst(Master_Sched_Config_List);

         if (sc && joker) {
            if ((pos=lGetPosViaElem(joker, SC_weight_tickets_deadline_active))>=0) 
               lSetUlong(sc, SC_weight_tickets_deadline_active, 
                  lGetPosUlong(joker, pos));
            if ((pos=lGetPosViaElem(joker, SC_weight_tickets_override))>=0) 
               lSetUlong(sc, SC_weight_tickets_override, 
                  lGetPosUlong(joker, pos));
         }
         /* no need to spool sched conf */
      }
       
      break;

   case ORT_suspend_on_threshold:
      {
         lListElem *queueep;
         u_long32 jobid;

         DPRINTF(("ORDER: ORT_suspend_on_threshold\n"));

         jobid = lGetUlong(ep, OR_job_number);
         task_number = lGetUlong(ep, OR_ja_task_number);

         if (!(jep = job_list_locate(Master_Job_List, jobid))
            || !(jatp = job_search_task(jep, NULL, task_number))
            || !lGetList(jatp, JAT_granted_destin_identifier_list)) {
            /* don't panic - it is probably an exiting job */
            WARNING((SGE_EVENT, MSG_JOB_SUSPOTNOTRUN_UU, u32c(jobid), u32c(task_number)));
         } else {
            const char *qnm = lGetString(lFirst(lGetList(jatp, JAT_granted_destin_identifier_list)), JG_qname);
            queueep = queue_list_locate(Master_Queue_List, qnm);
            if (!queueep) {
               ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDMQ_SU,
                     qnm, u32c(jobid)));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               DEXIT;
               return -1;
            }
            
            INFO((SGE_EVENT, MSG_JOB_SUSPTQ_UUS, u32c(jobid), u32c(task_number), qnm));

            if (!ISSET(lGetUlong(jatp, JAT_state), JSUSPENDED)) {
               sge_signal_queue(SGE_SIGSTOP, queueep, jep, jatp);
               state = lGetUlong(jatp, JAT_state);
               CLEARBIT(JRUNNING, state);
               lSetUlong(jatp, JAT_state, state);
            } 
            state = lGetUlong(jatp, JAT_state);
            SETBIT(JSUSPENDED_ON_THRESHOLD, state);
            lSetUlong(jatp, JAT_state, state);

            sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatp);
            {
               lList *answer_list = NULL;
               spool_write_object(&answer_list, 
                                  spool_get_default_context(), jep, 
                                  job_get_key(jobid, task_number, NULL),
                                  SGE_TYPE_JOB);
               answer_list_output(&answer_list);
            }

            /* update queues time stamp in schedd */
            lSetUlong(queueep, QU_last_suspend_threshold_ckeck, sge_get_gmt());
            sge_add_queue_event(sgeE_QUEUE_MOD, queueep);
         }
      }
      break;

   case ORT_unsuspend_on_threshold:
      {
         lListElem *queueep;
         u_long32 jobid;

         DPRINTF(("ORDER: ORT_unsuspend_on_threshold\n"));

         jobid = lGetUlong(ep, OR_job_number);
         task_number = lGetUlong(ep, OR_ja_task_number);

         if (!(jep = job_list_locate(Master_Job_List, jobid))
            || !(jatp = job_search_task(jep, NULL,task_number))
            || !lGetList(jatp, JAT_granted_destin_identifier_list)) {
            /* don't panic - it is probably an exiting job */  
            WARNING((SGE_EVENT, MSG_JOB_UNSUSPOTNOTRUN_UU, u32c(jobid), u32c(task_number)));
         } 
         else {
            const char *qnm = lGetString(lFirst(lGetList(jatp, JAT_granted_destin_identifier_list)), JG_qname);
            queueep = queue_list_locate(Master_Queue_List, qnm);
            if (!queueep) {
               ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDMQ_SU, qnm, u32c(jobid)));
               answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
               DEXIT;
               return -1;
            }

            INFO((SGE_EVENT, MSG_JOB_UNSUSPOT_UUS, u32c(jobid), u32c(task_number), qnm));
      
            if (!ISSET(lGetUlong(jatp, JAT_state), JSUSPENDED)) {
               sge_signal_queue(SGE_SIGCONT, queueep, jep, jatp);
               state = lGetUlong(jatp, JAT_state);
               SETBIT(JRUNNING, state);
               lSetUlong(jatp, JAT_state, state);
            }
            state = lGetUlong(jatp, JAT_state);
            CLEARBIT(JSUSPENDED_ON_THRESHOLD, state);
            lSetUlong(jatp, JAT_state, state);
            sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatp);
            {
               lList *answer_list = NULL;
               spool_write_object(&answer_list, 
                                  spool_get_default_context(), jep, 
                                  job_get_key(jobid, task_number, NULL),
                                  SGE_TYPE_JOB);
               answer_list_output(&answer_list);
            }
            /* update queues time stamp in schedd */
            lSetUlong(queueep, QU_last_suspend_threshold_ckeck, sge_get_gmt());
            sge_add_queue_event(sgeE_QUEUE_MOD, queueep);
         }
      }
      break;

   case ORT_job_schedd_info:
      {
         lListElem *sme ; /* SME_Type */

         DPRINTF(("ORDER: ORT_job_schedd_info\n"));
         sme = lFirst(lGetList(ep, OR_joker));
         if (sme && Master_Job_Schedd_Info_List) {
            Master_Job_Schedd_Info_List = lFreeList(Master_Job_Schedd_Info_List);
         }
         if (sme && !Master_Job_Schedd_Info_List) {
            Master_Job_Schedd_Info_List = lCreateList("schedd info", SME_Type);
            lAppendElem(Master_Job_Schedd_Info_List, lCopyElem(sme));
         }

         sge_add_event(NULL, 0, sgeE_JOB_SCHEDD_INFO_MOD, 0, 0, NULL, sme);
      }
      break;

   default:
      break;
   }

  /* order sucessfully executed */
  answer_list_add(alpp, "OK\n", STATUS_OK, ANSWER_QUALITY_INFO);

  DEXIT;
  return STATUS_OK;
}

int distribute_ticket_orders(
lList *ticket_orders 
) {
   u_long32 jobid, jataskid; 
   lList *to_send;
   const char *host_name;
   const char *master_host_name;
   lListElem *jep, *other_jep, *ep, *ep2, *next, *other, *jatask = NULL, *other_jatask;
   sge_pack_buffer pb;
   int cl_err = 0;
   u_long32 dummymid;
   int n;
   u_long32 now;
   static u_short number_one = 1;

   DENTER(TOP_LAYER, "distribute_ticket_orders");

   now = sge_get_gmt();

   while ((ep=lFirst(ticket_orders))) {     /* CR SPEEDUP CANDIDATE */
      lListElem *hep;
   
      jobid = lGetUlong(ep, OR_job_number);
      jataskid = lGetUlong(ep, OR_ja_task_number);

      DPRINTF(("Job: %ld, Task: %ld", jobid, jataskid));

      /* seek job element */
      if (!(jep = job_list_locate(Master_Job_List, jobid)) || 
          !(jatask = job_search_task(jep, NULL, jataskid))) { 
         ERROR((SGE_EVENT, MSG_JOB_MISSINGJOBTASK_UU, u32c(jobid), u32c(jataskid)));
         lRemoveElem(ticket_orders, ep);
      }
 
      /* seek master queue */
      master_host_name = lGetHost(lFirst( lGetList(jatask, JAT_granted_destin_identifier_list)), JG_qhostname);

      /* put this one in 'to_send' */ 
      to_send = lCreateList("to send", lGetElemDescr(ep));
      lDechainElem(ticket_orders, ep);
      lAppendElem(to_send, ep);

      /* 
         now seek all other ticket orders 
         for jobs residing on this host 
         and add them to 'to_send'
      */ 
      next = lFirst(ticket_orders);
      while ((other=next)) {      /* CR SPEEDUP CANDIDATE */
         next = lNext(other);

         other_jep = job_list_locate(Master_Job_List, lGetUlong(other, OR_job_number)); 
         other_jatask = job_search_task(other_jep, NULL, lGetUlong(other, OR_ja_task_number));
         if (!other_jep || !other_jatask) {
            ERROR((SGE_EVENT, MSG_JOB_MISSINGJOBTASK_UU, u32c(jobid), u32c(jataskid)));
            lRemoveElem(ticket_orders, other);
         }

         host_name = lGetHost(lFirst(lGetList(other_jatask, 
            JAT_granted_destin_identifier_list)), JG_qhostname);
         if (!sge_hostcmp(host_name, master_host_name)) {
            /* add it */
            lDechainElem(ticket_orders, other);
            lAppendElem(to_send, other);
         } 
      }
 
      hep = host_list_locate(Master_Exechost_List, master_host_name);
      n = lGetNumberOfElem(to_send);

      if (  hep && 
            last_heard_from(prognames[EXECD], &number_one, 
            master_host_name) 
            +10*conf.load_report_time > now) {

         /* should have now all ticket orders for 'host' in 'to_send' */ 
         if (init_packbuffer(&pb, sizeof(u_long32)*3*n, 0)==PACK_SUCCESS) {
            for_each (ep2, to_send) {
               packint(&pb, lGetUlong(ep2, OR_job_number));
               packint(&pb, lGetUlong(ep2, OR_ja_task_number));
               packdouble(&pb, lGetDouble(ep2, OR_ticket));
            }
            cl_err = gdi_send_message_pb(0, prognames[EXECD], 0,
                  master_host_name, TAG_CHANGE_TICKET, &pb, &dummymid);
            clear_packbuffer(&pb);
            DPRINTF(("%s %d ticket changings to execd@%s\n", 
               cl_err?"failed sending":"sent", n, master_host_name));
         }
      } else {
         DPRINTF(("     skipped sending of %d ticket changings to "
               "execd@%s because %s\n", n, master_host_name, 
               !hep?"no such host registered":"suppose host is down"));
      }

      to_send = lFreeList(to_send);
   }

   DEXIT;
   return cl_err;
}

#if 0
static double get_usage_value(lList *usage, char *name)
{
   lListElem *ue;
   double value = 0;
   if ((ue = lGetElemStr(usage, UA_name, name)))
      value = lGetDouble(ue, UA_value);
   return value;
}
#endif
