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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_usageL.h"
#include "sge_job_refL.h"
#include "sge_report.h"
#include "sge_time_eventL.h"

#include "basis_types.h"
#include "sgermon.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge.h"
#include "sge_conf.h"
#include "sge_any_request.h"
#include "commlib.h"
#include "job_log.h"
#include "pack_job_delivery.h"
#include "sge_subordinate_qmaster.h" 
#include "sge_complex_schedd.h" 
#include "sge_ckpt_qmaster.h"
#include "sge_job_qmaster.h" 
#include "job_exit.h" 
#include "sge_give_jobs.h"
#include "sge_host_qmaster.h"
#include "sge_host.h"
#include "sge_pe_qmaster.h"
#include "sge_event_master.h"
#include "sge_queue_event_master.h"
#include "sge_cqueue_qmaster.h"
#include "sge_prog.h"
#include "sge_select_queue.h"
#include "sort_hosts.h"
#include "sge_afsutil.h"
#include "setup_path.h"
#include "execution_states.h"
#include "sge_parse_num_par.h"
#include "symbols.h"
#include "mail.h"
#include "reschedule.h"
#include "sge_security.h"
#include "sge_pe.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_string.h"
#include "sge_suser.h"
#include "sge_io.h"
#include "sge_hostname.h"
#include "sge_schedd_conf.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_ckpt.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"
#include "sge_todo.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_lock.h"

#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "spool/sge_spooling.h"

static void sge_clear_granted_resources(lListElem *jep, lListElem *ja_task, int incslots);

static void reduce_queue_limit(lListElem *qep, lListElem *jep, int nm, char *rlimit_name);

static void release_successor_jobs(lListElem *jep);

static int send_job(const char *rhost, const char *target, lListElem *jep, lListElem *jatep, lListElem *pe, lListElem *hep, int master);

static int sge_bury_job(lListElem *jep, u_long32 jid, lListElem *ja_task, int spool_job, int no_events);

static int sge_to_zombies(lListElem *jep, lListElem *ja_task, int spool_job);
static void sge_job_finish_event(lListElem *jep, lListElem *jatep, lListElem *jr, sge_commit_flags_t commit_flags, const char *diagnosis);

/************************************************************************
 Master function to give job to the execd.

 We make asynchron sends and implement a retry mechanism.
 Do everything to make sure the execd is prepared for receiving the job.
 ************************************************************************/

int sge_give_job(
lListElem *jep,
lListElem *jatep,
lListElem *master_qep,
lListElem *pe, /* NULL in case of serial jobs */
lListElem *hep 
) {
   const char *target;   /* prognames[EXECD|QSTD] */
   const char *rhost;   /* prognames[EXECD|QSTD] */
   lListElem *gdil_ep, *q;
   int ret = 0, sent_slaves = 0;
   
   DENTER(TOP_LAYER, "sge_give_job");
   
   target = prognames[EXECD];
   rhost = lGetHost(master_qep, QU_qhostname);
   DPRINTF(("execd host: %s\n", rhost));

   /* build complex list for all queues in the gdil list */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) { 
      lList *resources;
      lList *gdil_ep_JG_complex;
      lListElem *ep1, *cep;
       

      if ((q = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), lGetString(gdil_ep, JG_qname)))) {
         resources = NULL;
         queue_complexes2scheduler(&resources, q, Master_Exechost_List, Master_CEntry_List);
         lSetList(gdil_ep, JG_complex, resources);
      } else {
         DTRACE;
      }


      /* job requests overrides complex list */
      gdil_ep_JG_complex = lGetList(gdil_ep, JG_complex); 
      for_each (ep1, lGetList(jep, JB_hard_resource_list)) {
         if ((cep=lGetElemStr(gdil_ep_JG_complex, CE_name, lGetString(ep1, CE_name))))  {  
            lSetString(cep, CE_stringval, lGetString(ep1, CE_stringval));
            DPRINTF(("complex: %s = %s\n", lGetString(ep1, CE_name), lGetString(ep1, CE_stringval)));
         }
         if (!cep) {
            DPRINTF(("complex %s not found\n", lGetString(ep1, CE_name)));
         }
      }
   }

   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) { 
      lListElem *slave_hep;

      if (lGetUlong(gdil_ep, JG_tag_slave_job)) {

         if (!(slave_hep = host_list_locate(Master_Exechost_List, lGetHost(gdil_ep, JG_qhostname)))) {
            ret = -1;   
            break;
         }

         /* start with 1 as first consecutive taskid at each node */
         lSetUlong(jatep, JAT_next_pe_task_id, 1);
   
         if (send_job(lGetHost(gdil_ep, JG_qhostname), target, jep, jatep, pe, slave_hep, 0)) {
            ret = -1;   
            break;
         }


         sent_slaves = 1;
      }
   }

   if (!sent_slaves) {
      /* wait till all slaves are acked */
      lSetUlong(jatep, JAT_next_pe_task_id, 1);
      ret = send_job(rhost, target, jep, jatep, pe, hep, 1);
   }

   /* free complex list in gdil list */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) 
      lSetList(gdil_ep, JG_complex, NULL);

   DEXIT;
   return ret;
}

static int send_job(
const char *rhost,
const char *target,
lListElem *jep,
lListElem *jatep,
lListElem *pe,
lListElem *hep,
int master 
) {
   int len, failed;
   u_long32 now;
   sge_pack_buffer pb;
   u_long32 dummymid = 0;
   lListElem *tmpjep, *qep, *tmpjatep, *next_tmpjatep, *tmpgdil_ep=NULL;
   lListElem *ckpt = NULL, *tmp_ckpt;
   lList *qlp;
   const char *ckpt_name;
   lListElem *gdil_ep;
   char *str;
#ifdef ENABLE_NGC
   unsigned long last_heard_from;
#else
   static u_short number_one = 1;
   int execd_enrolled;  
#endif


   DENTER(TOP_LAYER, "send_job");

   /* map hostname if we are simulating hosts */
   if(simulate_hosts == 1) {
      const lListElem *simhost = lGetSubStr(hep, CE_name, "simhost", EH_consumable_config_list);
      if(simhost != NULL) {
         const char *real_host = lGetString(simhost, CE_stringval);
         if(real_host != NULL && sge_hostcmp(real_host, rhost) != 0) {
            DPRINTF(("deliver job for simulated host %s to host %s\n", rhost, real_host));
            rhost = real_host;
         }   
      }
   }

   /* do ask_commproc() only if we are missing load reports */
   now = sge_get_gmt();
#ifdef ENABLE_NGC
   cl_commlib_get_last_message_time((cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0)),
                                        (char*)rhost, (char*)target,1, &last_heard_from);
   if (last_heard_from + load_report_interval(hep)*2 <= now) {
      ERROR((SGE_EVENT, MSG_COM_NOTENROLLEDONHOST_SSU, target, rhost, u32c(lGetUlong(jep, JB_job_number))));
      sge_mark_unheard(hep, target);
      DEXIT;
      return -1;
   }
#else
   if (last_heard_from(target, &number_one, rhost)+ load_report_interval(hep)*2 <= now) {

      execd_enrolled = ask_commproc(rhost, target, 0);
      if (execd_enrolled) {
         ERROR((SGE_EVENT, MSG_COM_NOTENROLLEDONHOST_SSU, 
               target, rhost, u32c(lGetUlong(jep, JB_job_number))));
         sge_mark_unheard(hep, target);

         DEXIT;
         return -1;
      }
   }
#endif

   /* load script into job structure for sending to execd */
   /*
   ** if exec_file is not set, then this is an interactive job
   */
   if (master && lGetString(jep, JB_exec_file)) {

      str = sge_file2string(lGetString(jep, JB_exec_file), &len);
      lSetString(jep, JB_script_ptr, str);
      FREE(str);
      lSetUlong(jep, JB_script_size, len);
   }

   tmpjep = lCopyElem(jep);

   /* insert ckpt object if required **now**. it is only
   ** needed in the execd and we have no need to keep it
   ** in qmaster's permanent job list
   */
   if ((ckpt_name=lGetString(tmpjep, JB_checkpoint_name))) {
      ckpt = ckpt_list_locate(Master_Ckpt_List, ckpt_name);
      if (!ckpt) {
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDCKPT_S, ckpt_name));
         DEXIT;
         return -1;
      }

      /* get the necessary memory */
      if (!(tmp_ckpt=lCopyElem(ckpt))) {
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2CREATECKPT_SU, 
                ckpt_name, u32c(lGetUlong(tmpjep, JB_job_number))));
         DEXIT;
         return -1;
      }
      /* everything's in place. stuff it in */
      lSetObject(tmpjep, JB_checkpoint_object, tmp_ckpt);
   }
   
   qlp = lCreateList("qlist", QU_Type);
   /* add all queues referenced in gdil to qlp 
    * (necessary for availability of ALL resource limits and tempdir in queue) 
    * AND
    * copy all JG_processors from all queues to the JG_processors in tmpgdil
    * (so the execd can decide on which processors (-sets) the job will be run).
    */
   tmpjatep = lFirst( lGetList( tmpjep, JB_ja_tasks ));   
   if( tmpjatep ) {
      tmpgdil_ep = lFirst( lGetList( tmpjatep, JAT_granted_destin_identifier_list ));
   }
   
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      const char *src_qname = lGetString(gdil_ep, JG_qname);
      lListElem *src_qep = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), src_qname);

      /* copy all JG_processors from all queues to tmpgdil (which will be
       * sent to the execd).
       */
      if( tmpgdil_ep ) {
         lSetString(tmpgdil_ep, JG_processors, lGetString(src_qep, QU_processors));
         tmpgdil_ep = lNext( tmpgdil_ep );
      }

      qep = lCopyElem(src_qep);

      /* build minimum of job request and queue resource limit */
      reduce_queue_limit(qep, tmpjep, QU_s_cpu,   "s_cpu");
      reduce_queue_limit(qep, tmpjep, QU_h_cpu,   "h_cpu");
      reduce_queue_limit(qep, tmpjep, QU_s_core,  "s_core");
      reduce_queue_limit(qep, tmpjep, QU_h_core,  "h_core");
      reduce_queue_limit(qep, tmpjep, QU_s_data,  "s_data");
      reduce_queue_limit(qep, tmpjep, QU_h_data,  "h_data");
      reduce_queue_limit(qep, tmpjep, QU_s_stack, "s_stack");
      reduce_queue_limit(qep, tmpjep, QU_h_stack, "h_stack");
      reduce_queue_limit(qep, tmpjep, QU_s_rss,   "s_rss");
      reduce_queue_limit(qep, tmpjep, QU_h_rss,   "h_rss");
      reduce_queue_limit(qep, tmpjep, QU_s_fsize, "s_fsize");
      reduce_queue_limit(qep, tmpjep, QU_h_fsize, "h_fsize");
      reduce_queue_limit(qep, tmpjep, QU_s_vmem,  "s_vmem");
      reduce_queue_limit(qep, tmpjep, QU_h_vmem,  "h_vmem");
      reduce_queue_limit(qep, tmpjep, QU_s_rt,    "s_rt");
      reduce_queue_limit(qep, tmpjep, QU_h_rt,    "h_rt");

      lAppendElem(qlp, qep);
   }

   if (pe) /* we dechain the element to get a temporary free element  */
      pe = lDechainElem(Master_Pe_List, pe);

   /* Remove all ja-task which should not be started on the exec-host */
   next_tmpjatep = lFirst(lGetList(tmpjep, JB_ja_tasks));
   while ((tmpjatep = next_tmpjatep)) {
      next_tmpjatep = lNext(tmpjatep);
      if (lGetUlong(tmpjatep, JAT_task_number) != lGetUlong(jatep, JAT_task_number)) {
         lRemoveElem(lGetList(tmpjep, JB_ja_tasks), tmpjatep);  
      }
   }


   /*
   ** get credential for job
   */
   if (do_credentials) {
      cache_sec_cred(tmpjep, rhost);
   }
  
#ifndef __SGE_NO_USERMAPPING__ 
   /* 
   ** This is for administrator user mapping 
   */
   {
      const char *owner = lGetString(tmpjep, JB_owner);
      const char *mapped_user;

      DPRINTF(("send_job(): Starting job of %s\n", owner));
      cuser_list_map_user(*(cuser_list_get_master_list()), NULL,
                          owner, rhost, &mapped_user);
      if (strcmp(mapped_user, owner)) {
         DPRINTF(("execution mapping: user %s mapped to %s on host %s\n", 
                  owner, mapped_user, rhost));
         lSetString(tmpjep, JB_owner, mapped_user);
      }
   }
#endif
   
   if(init_packbuffer(&pb, 0, 0) != PACK_SUCCESS) {
      lFreeElem(tmpjep);
      lFreeList(qlp);
      DEXIT;
      return -1;
   }

   pack_job_delivery(&pb, tmpjep, qlp, pe);
   
   lFreeElem(tmpjep);
   lFreeList(qlp);

   if (pe) 
      lAppendElem(Master_Pe_List, pe);


   /*
   ** security hook
   */
   tgt2cc(jep, rhost, target);

#ifdef ENABLE_NGC
   failed = gdi_send_message_pb(0, target, 1, rhost, master?TAG_JOB_EXECUTION:TAG_SLAVE_ALLOW, &pb, &dummymid);
#else
   failed = gdi_send_message_pb(0, target, 0, rhost, master?TAG_JOB_EXECUTION:TAG_SLAVE_ALLOW, &pb, &dummymid);
#endif

   /*
   ** security hook
   */
   tgtcclr(jep, rhost, target);

   clear_packbuffer(&pb);

   /* We will get an acknowledge from execd. But we wait in the main loop so 
      that we can handle some requests until the acknowledge arrives.
      This is to make the qmaster non blocking */
      
   /* remove script file from memory again (dont waste that much memory) */
   if (master) {
      lSetString(jep, JB_script_ptr, NULL);
      lSetUlong(jep, JB_script_size, 0);
   }

#ifdef ENABLE_NGC
   if (failed != CL_RETVAL_OK)
#else
   if (failed) 
#endif
   { /* we failed sending the job to the execd */
      ERROR((SGE_EVENT, MSG_COM_SENDJOBTOHOST_US,   
            u32c(lGetUlong(jep, JB_job_number)), rhost));
      sge_mark_unheard(hep, target);
      DEXIT;
      return -1;
   } else {
      DPRINTF(("successfully sent %sjob "u32" to host \"%s\"\n", 
               master?"":"SLAVE ", 
               lGetUlong(jep, JB_job_number), 
               rhost));
   }

   DEXIT;
   return 0;
}

void sge_job_resend_event_handler(te_event_t anEvent)
{
   lListElem* jep, *jatep, *ep, *hep, *pe, *mqep;
   lList* jatasks;
   const char *qnm, *hnm;
   time_t now;
   u_long32 jobid = te_get_first_numeric_key(anEvent);
   u_long32 jataskid = te_get_second_numeric_key(anEvent);
   
   DENTER(TOP_LAYER, "sge_job_resend_event_handler");

   jep = job_list_locate(Master_Job_List, jobid);
   jatep = job_search_task(jep, NULL, jataskid);
   now = sge_get_gmt();

   if(!jep || !jatep) {
      WARNING((SGE_EVENT, MSG_COM_RESENDUNKNOWNJOB_UU, 
               u32c(jobid), u32c(jataskid)));
      DEXIT;
      return;
   }

   jatasks = lGetList(jep, JB_ja_tasks);

   /* check whether a slave execd allowance has to be retransmitted */
   if (lGetUlong(jatep, JAT_status) == JTRANSFERING) {
      ep = lFirst(lGetList(jatep, JAT_granted_destin_identifier_list));
      if (!ep || 
         !(qnm=lGetString(ep, JG_qname)) || 
         !(hnm=lGetHost(ep, JG_qhostname))) {
         ERROR((SGE_EVENT, MSG_JOB_UNKNOWNGDIL4TJ_UU,
               u32c(jobid), u32c(jataskid)));
         lDelElemUlong(&Master_Job_List, JB_job_number, jobid);
         DEXIT;
         return;
      }

      mqep = cqueue_list_locate_qinstance(
                     *(object_type_get_master_list(SGE_TYPE_CQUEUE)), qnm);
      if (mqep == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_NOQUEUE4TJ_SUU,  
               qnm, u32c(jobid), u32c(jataskid)));
         lDelElemUlong(&jatasks, JAT_task_number, jataskid);
         DEXIT;
         return;
      }
      if (!(hnm=lGetHost(mqep, QU_qhostname)) ||
          !(hep = host_list_locate(Master_Exechost_List, hnm))) {
         ERROR((SGE_EVENT, MSG_JOB_NOHOST4TJ_SUU, 
                hnm, u32c(jobid), u32c(jataskid)));
         lDelElemUlong(&jatasks, JAT_task_number, jataskid);
         DEXIT;
         return;
      }

      if (qinstance_state_is_unknown(mqep)) {
         trigger_job_resend(now, hep, jobid, jataskid);
         return; /* try later again */
      }

      if (lGetString(jatep, JAT_granted_pe)) {
         if (!(pe = pe_list_locate(Master_Pe_List, lGetString(jatep, JAT_granted_pe)))) {
            ERROR((SGE_EVENT, MSG_JOB_NOPE4TJ_SUU, 
                  lGetString(jep, JB_pe), u32c(jobid), u32c(jataskid)));
            lDelElemUlong(&Master_Job_List, JB_job_number, jobid);
            DEXIT;
            return;
         }
      }
      else
         pe = NULL;

      if (lGetUlong(jatep, JAT_start_time)) {
         WARNING((SGE_EVENT, MSG_JOB_DELIVER2Q_UUS, 
                u32c(jobid), u32c(jataskid), 
                lGetString(jatep, JAT_master_queue)));
      }

      sge_give_job(jep, jatep, mqep, pe, hep);
 
      /* reset timer */
      lSetUlong(jatep, JAT_start_time, now);
      trigger_job_resend(now, hep, lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number));

   } 

   DEXIT;
} /* sge_job_resend_event_handler() */


void cancel_job_resend(u_long32 jid, u_long32 ja_task_id)
{
   DENTER(TOP_LAYER, "cancel_job_resend");

   DPRINTF(("CANCEL JOB RESEND "u32"/"u32"\n", jid, ja_task_id)); 
   te_delete_one_time_event(TYPE_JOB_RESEND_EVENT, jid, ja_task_id, "job-resend_event");

   DEXIT;
   return;
}

/* 
 * if hep equals to NULL resend is triggered immediatelly 
 */ 
void trigger_job_resend(u_long32 now, lListElem *hep, u_long32 jid, u_long32 ja_task_id)
{
   u_long32 seconds;
   time_t when = 0;
   te_event_t ev = NULL;

   DENTER(TOP_LAYER, "trigger_job_resend");

   seconds = hep ? MAX(load_report_interval(hep), MAX_JOB_DELIVER_TIME) : 0;
   DPRINTF(("TRIGGER JOB RESEND "u32"/"u32" in %d seconds\n", jid, ja_task_id, seconds)); 

   when = now + seconds;
   ev = te_new_event(when, TYPE_JOB_RESEND_EVENT, ONE_TIME_EVENT, jid, ja_task_id, "job-resend_event");
   te_add_event(ev);
   te_free_event(ev);

   DEXIT;
   return;
}


/***********************************************************************
 sge_zombie_job_cleanup_handler

 Remove zombie jobs, which have expired (currently, we keep a list of
 conf.zombie_jobs entries)
 ***********************************************************************/
void sge_zombie_job_cleanup_handler(te_event_t anEvent)
{
   lListElem *dep;
   
   DENTER(TOP_LAYER, "sge_zombie_job_cleanup_handler");

   SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE);

   while (Master_Zombie_List &&
            (lGetNumberOfElem(Master_Zombie_List) > conf.zombie_jobs)) {
      dep = lFirst(Master_Zombie_List);
      lRemoveElem(Master_Zombie_List, dep);
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);

   DEXIT;
}

/****** sge_give_jobs/sge_commit_job() *****************************************
*  NAME
*     sge_commit_job() -- Do job state transitions
*
*  SYNOPSIS
*     void sge_commit_job(lListElem *jep, lListElem *jatep, lListElem *jr, 
*     sge_commit_mode_t mode, sge_commit_flags_t commit_flags) 
*
*  FUNCTION
*     sge_commit_job() implements job state transitons. When a dispatch
*     order arrives from schedd the job is sent asynchonously to execd and 
*     sge_commit_job() is called with mode==COMMIT_ST_SENT. When a job report 
*     arrives from the execd mode is COMMIT_ST_ARRIVED. When the job failed
*     or finished mode is COMMIT_ST_FINISHED_FAILED or 
*     COMMIT_ST_FINISHED_FAILED_EE depending on product mode:
* 
*     A SGE job can be removed immediately when it is finished 
*     (mode==COMMIT_ST_FINISHED_FAILED). A SGEEE job may not be deleted 
*     (mode==COMMIT_ST_FINISHED_FAILED_EE) before the SGEEE scheduler has debited 
*     the jobs resource consumption in the corresponding objects (project/user/..). 
*     Only the job script may be deleted at this stage. When an order arrives at 
*     qmaster telling that debitation was done (mode==COMMIT_ST_DEBITED_EE) the 
*     job can be deleted.
*     
*     sge_commit_job() releases resources when a job is no longer running.
*     Also state changes jobs are spooled and finally spooling files are 
*     deleted. Also jobs start time is set to the actual time when job is sent.
*
*  INPUTS
*     lListElem *jep                  - the job 
*     lListElem *jatep                - the array task
*     lListElem *jr                   - the job report (may be NULL)
*     sge_commit_mode_t mode          - the 'mode' - actually the state transition
*     sge_commit_flags_t commit_flags - additional flags for parametrizing 
*
*  SEE ALSO
*     See sge_commit_mode_t typedef for documentation on mode.
*     See sge_commit_flags_t typedef for documentation on commit_flags.
*******************************************************************************/
void sge_commit_job(
lListElem *jep,
lListElem *jatep,
lListElem *jr,
sge_commit_mode_t mode,
sge_commit_flags_t commit_flags
) {
   lListElem *cqueue, *hep, *petask, *tmp_ja_task;
   lListElem *global_host_ep;
   int slots;
   lUlong jobid, jataskid;
   int no_unlink = 0;
   int spool_job = !(commit_flags & COMMIT_NO_SPOOLING);
   int no_events = (commit_flags & COMMIT_NO_EVENTS);
   int unenrolled_task = (commit_flags & COMMIT_UNENROLLED_TASK);
   int handle_zombies = (conf.zombie_jobs > 0);
   u_long32 now = 0;
   const char *session;
   lList *answer_list = NULL;
   const char *hostname;

   DENTER(TOP_LAYER, "sge_commit_job");

   jobid = lGetUlong(jep, JB_job_number);
   jataskid = jatep?lGetUlong(jatep, JAT_task_number):0;
   session = lGetString(jep, JB_session);

   now = sge_get_gmt();

   /* need hostname for job_log */
   hostname = uti_state_get_qualified_hostname();

   switch (mode) {
   case COMMIT_ST_SENT:
      lSetUlong(jatep, JAT_state, JRUNNING);
      lSetUlong(jatep, JAT_status, JTRANSFERING);

/*       job_log(jobid, jataskid, MSG_LOG_SENT2EXECD); */
      reporting_create_job_log(NULL, now, JL_SENT, MSG_QMASTER, hostname, jr, jep, jatep, NULL, MSG_LOG_SENT2EXECD);

      /* should use jobs gdil instead of tagging queues */
      global_host_ep = host_list_locate(Master_Exechost_List, "global");

      for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {     
         lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
         lListElem *qinstance;

         for_each(qinstance, qinstance_list) {
            if (lGetUlong(qinstance, QU_tag)) {
               const char* qep_QU_qhostname;

               slots = lGetUlong(qinstance, QU_tag);
               lSetUlong(qinstance, QU_tag, 0);

               qep_QU_qhostname = lGetHost(qinstance, QU_qhostname);

               /* debit in all layers */
               if (debit_host_consumable(jep, global_host_ep, Master_CEntry_List, slots) > 0 ) {
                  /* this info is not spooled */
                  sge_add_event(0, sgeE_EXECHOST_MOD, 0, 0, 
                                "global", NULL, NULL, global_host_ep );
                  reporting_create_host_consumable_record(&answer_list, global_host_ep, now);
                  answer_list_output(&answer_list);
                  lListElem_clear_changed_info(global_host_ep);
               }

               hep = host_list_locate(Master_Exechost_List, qep_QU_qhostname);
               if ( debit_host_consumable(jep, hep, Master_CEntry_List, slots) > 0 ) {
                  /* this info is not spooled */
                  sge_add_event( 0, sgeE_EXECHOST_MOD, 0, 0, 
                                qep_QU_qhostname, NULL, NULL, hep);
                  reporting_create_host_consumable_record(&answer_list, hep, now);
                  answer_list_output(&answer_list);
                  lListElem_clear_changed_info(hep);
               }
               qinstance_debit_consumable(qinstance, jep, Master_CEntry_List, slots);
               reporting_create_queue_consumable_record(&answer_list, qinstance, now);
               /* this info is not spooled */
               qinstance_add_event(qinstance, sgeE_QINSTANCE_MOD); 
               lListElem_clear_changed_info(qinstance);
            }
         }
      }

      /* 
       * Would be nice if we could use a more accurate start time that could be reported 
       * by execd. However this would constrain time synchronization between qmaster and 
       * execd host .. sigh!
       */
      lSetUlong(jatep, JAT_start_time, now);
      job_enroll(jep, NULL, jataskid);
      {
         const char *session = lGetString (jep, JB_session);

         sge_event_spool(&answer_list, 0, sgeE_JATASK_MOD,
                         jobid, jataskid, NULL, NULL, session,
                         jep, jatep, NULL, true, true);
         answer_list_output(&answer_list);
      }
      break;

   case COMMIT_ST_ARRIVED:
      lSetUlong(jatep, JAT_status, JRUNNING);
/*       job_log(jobid, jataskid, MSG_LOG_DELIVERED); */
      reporting_create_job_log(NULL, now, JL_DELIVERED, MSG_QMASTER, hostname, jr, jep, jatep, NULL, MSG_LOG_DELIVERED);
      job_enroll(jep, NULL, jataskid);
      {
         dstring buffer = DSTRING_INIT;
         /* JG: TODO: why don't we generate an event? */
         lList *answer_list = NULL;
         spool_write_object(&answer_list, spool_get_default_context(), jep, 
                            job_get_key(jobid, jataskid, NULL, &buffer), 
                            SGE_TYPE_JATASK);
         answer_list_output(&answer_list);
         lListElem_clear_changed_info(jatep);
         sge_dstring_free(&buffer);
      }
      break;

   case COMMIT_ST_RESCHEDULED:
   case COMMIT_ST_FAILED_AND_ERROR:
      WARNING((SGE_EVENT, MSG_JOB_RESCHEDULE_UU,
               u32c(lGetUlong(jep, JB_job_number)), 
               u32c(lGetUlong(jatep, JAT_task_number))));

      reporting_create_job_log(NULL, now, JL_RESTART, MSG_QMASTER, hostname, jr, jep, jatep, NULL, SGE_EVENT);
      /* JG: TODO: no accounting record created? Or somewhere else? */
      /* add a reschedule unknown list entry to all slave
         hosts where a part of that job ran */
      {
         lListElem *granted_queue;
         const char *master_host = NULL;
         lListElem *pe;
         const char *pe_name;      

         pe_name = lGetString(jep, JB_pe);
         if (pe_name) {
            pe = pe_list_locate(Master_Pe_List, pe_name);
            if (pe && lGetBool(pe, PE_control_slaves)) { 
               for_each(granted_queue, lGetList(jatep, JAT_granted_destin_identifier_list)) { 
                  lListElem *host;
                  const char *granted_queue_JG_qhostname;

                  granted_queue_JG_qhostname = lGetHost(granted_queue, JG_qhostname);

                  if (!master_host) {
                     master_host = granted_queue_JG_qhostname;
                  } 
                  if (sge_hostcmp(master_host, granted_queue_JG_qhostname )) {
                     host = host_list_locate(Master_Exechost_List, granted_queue_JG_qhostname ); 
                     
                     add_to_reschedule_unknown_list(host, 
                        lGetUlong(jep, JB_job_number),
                        lGetUlong(jatep, JAT_task_number),
                        RESCHEDULE_HANDLE_JR_WAIT);

                     DPRINTF(("RU: sge_commit: granted_queue %s job "u32"."u32"\n",
                        lGetString(granted_queue, JG_qname), 
                        lGetUlong(jep, JB_job_number), 
                        lGetUlong(jatep, JAT_task_number)));
                  }
               }
            }
         } else {
            lList *granted_list = NULL;
            lListElem *granted_queue = NULL;
            lListElem *host = NULL;

            granted_list = lGetList(jatep, JAT_granted_destin_identifier_list);
            granted_queue = lFirst(granted_list);
            host = host_list_locate(Master_Exechost_List, lGetHost(granted_queue, JG_qhostname));
            add_to_reschedule_unknown_list(host, lGetUlong(jep, JB_job_number),
                                           lGetUlong(jatep, JAT_task_number),
                                           RESCHEDULE_SKIP_JR);
         }
      }

      lSetUlong(jatep, JAT_status, JIDLE);
      lSetUlong(jatep, JAT_state, (mode==COMMIT_ST_RESCHEDULED) ?(JQUEUED|JWAITING): (JQUEUED|JWAITING|JERROR));

      lSetList(jatep, JAT_previous_usage_list, lCopyList("name", lGetList(jatep, JAT_scaled_usage_list)));
      lSetList(jatep, JAT_scaled_usage_list, NULL);
      {
         lListElem *ep;
         const char *s;

         DPRINTF(("osjobid = %s\n", (s=lGetString(jatep, JAT_osjobid))?s:"(null)"));
         for_each(ep, lGetList(jatep, JAT_previous_usage_list)) {
            DPRINTF(("previous usage: \"%s\" = %f\n",
                        lGetString(ep, UA_name),
                                    lGetDouble(ep, UA_value)));
         }
      }

      /* sum up the usage of all pe tasks not yet deleted (e.g. tasks from 
       * exec host in unknown state). Then remove all pe tasks except the 
       * past usage container 
       */
      { 
         lList *pe_task_list = lGetList(jatep, JAT_task_list);

         if(pe_task_list != NULL) {
            lListElem *pe_task, *container, *existing_container;

            existing_container = lGetElemStr(pe_task_list, PET_id, PE_TASK_PAST_USAGE_CONTAINER); 
            container = pe_task_sum_past_usage_all(pe_task_list);
            if(existing_container == NULL) {
               /* the usage container is not spooled */
               sge_add_event( 0, sgeE_PETASK_ADD, jobid, jataskid, 
                             PE_TASK_PAST_USAGE_CONTAINER, NULL,
                             session, container);
               lListElem_clear_changed_info(container);
            } else {
               /* the usage container is not spooled */
               sge_add_list_event( 0, sgeE_JOB_USAGE, jobid, jataskid, 
                                  PE_TASK_PAST_USAGE_CONTAINER, NULL,
                                  lGetString(jep, JB_session), 
                                  lGetList(container, PET_scaled_usage));
               lList_clear_changed_info(lGetList(container, PET_scaled_usage));
            }

            for_each(pe_task, pe_task_list) {
               if(pe_task != container) {
                  lRemoveElem(pe_task_list, pe_task);
               }
            }
         }
      }   

      sge_clear_granted_resources(jep, jatep, 1);
      ja_task_clear_finished_pe_tasks(jatep);
      job_enroll(jep, NULL, jataskid);
      {
         lList *answer_list = NULL;
         const char *session = lGetString (jep, JB_session);
         sge_event_spool(&answer_list, 0, sgeE_JATASK_MOD, 
                         jobid, jataskid, NULL, NULL, session,
                         jep, jatep, NULL, true, true);
         answer_list_output(&answer_list);
      }
      break;

   case COMMIT_ST_FINISHED_FAILED:
/*       job_log(jobid, jataskid, MSG_LOG_EXITED); */
      reporting_create_job_log(NULL, now, JL_FINISHED, MSG_QMASTER, hostname, jr, jep, jatep, NULL, MSG_LOG_EXITED);
      if (handle_zombies) {
         sge_to_zombies(jep, jatep, spool_job);
      }
      if (!unenrolled_task) { 
         sge_clear_granted_resources(jep, jatep, 1);
      }
      sge_job_finish_event(jep, jatep, jr, commit_flags, NULL); 
      sge_bury_job(jep, jobid, jatep, spool_job, no_events);
      break;
   case COMMIT_ST_FINISHED_FAILED_EE:
      jobid = lGetUlong(jep, JB_job_number);
/*       job_log(jobid, jataskid, MSG_LOG_WAIT4SGEDEL); */
      reporting_create_job_log(NULL, now, JL_FINISHED, MSG_QMASTER, hostname, jr, jep, jatep, NULL, MSG_LOG_WAIT4SGEDEL);

      lSetUlong(jatep, JAT_status, JFINISHED);

      sge_job_finish_event(jep, jatep, jr, commit_flags, NULL); 

      if (handle_zombies) {
         sge_to_zombies(jep, jatep, spool_job);
      }
      sge_clear_granted_resources(jep, jatep, 1);
      job_enroll(jep, NULL, jataskid);
      for_each(petask, lGetList(jatep, JAT_task_list)) {
         sge_add_list_event( 0, sgeE_JOB_FINAL_USAGE, jobid,
                            lGetUlong(jatep, JAT_task_number),
                            lGetString(petask, PET_id), 
                            NULL, lGetString(jep, JB_session),
                            lGetList(petask, PET_scaled_usage));
      }
      /* job usage is not spooled */
      sge_add_list_event( 0, sgeE_JOB_FINAL_USAGE, jobid,
                         lGetUlong(jatep, JAT_task_number),
                         NULL, NULL, lGetString(jep, JB_session), 
                         lGetList(jatep, JAT_scaled_usage_list));

      /* finished all ja-tasks => remove job script */
      for_each(tmp_ja_task, lGetList(jep, JB_ja_tasks)) {
         if (lGetUlong(tmp_ja_task, JAT_status) != JFINISHED)
            no_unlink = 1;
      }
      if (job_get_not_enrolled_ja_tasks(jep)) {
         no_unlink = 1;
      }
      if (!no_unlink) {
         release_successor_jobs(jep);
         unlink(lGetString(jep, JB_exec_file));
      }
      break;

   case COMMIT_ST_DEBITED_EE: /* triggered by ORT_remove_job */
   case COMMIT_ST_NO_RESOURCES: /* triggered by ORT_remove_immediate_job */
/*       job_log(jobid, jataskid, (mode==COMMIT_ST_DEBITED_EE) ?  MSG_LOG_DELSGE : MSG_LOG_DELIMMEDIATE); */
      reporting_create_job_log(NULL, now, JL_DELETED, MSG_SCHEDD, hostname, jr, jep, jatep, NULL, (mode==COMMIT_ST_DEBITED_EE) ?  MSG_LOG_DELSGE : MSG_LOG_DELIMMEDIATE);
      jobid = lGetUlong(jep, JB_job_number);

      if (mode == COMMIT_ST_NO_RESOURCES)
         sge_job_finish_event(jep, jatep, jr, commit_flags, NULL); 
      sge_bury_job(jep, jobid, jatep, spool_job, no_events);
      break;
   
   case COMMIT_ST_DELIVERY_FAILED: 
             /*  The same as case COMMIT_ST_RESCHEDULED except 
                 sge_clear_granted_resources() may not increase free slots. */
      WARNING((SGE_EVENT, 
               MSG_JOB_RESCHEDULE_UU, 
               u32c(lGetUlong(jep, JB_job_number)), 
               u32c(lGetUlong(jatep, JAT_task_number))));
      reporting_create_job_log(NULL, now, JL_RESTART, MSG_QMASTER, hostname, jr, jep, jatep, NULL, SGE_EVENT);
      lSetUlong(jatep, JAT_status, JIDLE);
      lSetUlong(jatep, JAT_state, JQUEUED | JWAITING);
      sge_clear_granted_resources(jep, jatep, 0);
      job_enroll(jep, NULL, jataskid);
      {
         lList *answer_list = NULL;
         const char *session = lGetString (jep, JB_session);
         sge_event_spool(&answer_list, 0, sgeE_JATASK_MOD, 
                         jobid, jataskid, NULL, NULL, session,
                         jep, jatep, NULL, true, true);
         answer_list_output(&answer_list);
      }
      break;
   }

   DEXIT;
   return;
}

   /* 
    * Job finish information required by DRMAA
    * u_long32 jobid
    * u_long32 taskid
    * u_long32 status
    *    SGE_WEXITED   flag
    *    SGE_WSIGNALED flag
    *    SGE_WCOREDUMP flag
    *    SGE_WABORTED  flag (= COMMIT_NEVER_RAN)
    *    exit status  (8 bit)
    *    signal       (8 bit)
    * lList *rusage;
    *
    * Additional job finish information provided by JAPI
    * char *failed_reason
    */
static void sge_job_finish_event(lListElem *jep, lListElem *jatep, lListElem *jr, 
                  sge_commit_flags_t commit_flags, const char *diagnosis)
{
   bool release_jr = false;

   DENTER(TOP_LAYER, "sge_job_finish_event");

   if (!jr) {
      jr = lCreateElem(JR_Type);
      lSetUlong(jr, JR_job_number, lGetUlong(jep, JB_job_number));
      lSetUlong(jr, JR_ja_task_number, lGetUlong(jatep, JAT_task_number));
      lSetUlong(jr, JR_wait_status, SGE_NEVERRAN_BIT);
      release_jr = true;   
   } else {
      /* JR_usage gets cleared in the process of cleaning up a finished job.
       * It's contents get put into JAT_usage_list and eventually added to
       * JAT_scaled_usage_list.  In the JAPI, however, it would be ugly to have
       * to go picking through the Master_Job_List to find the accounting data.
       * So instead we pick through it here and stick it back in JR_usage. */
      lListElem *job = job_list_locate(Master_Job_List, lGetUlong (jr, JR_job_number));
      lListElem *jatp = lGetElemUlong(lGetList(job, JB_ja_tasks), JAT_task_number, lGetUlong(jr, JR_ja_task_number));
      lList *usage = lCopyList ("Scaled Usage List", lGetList (jatp, JAT_scaled_usage_list));
      lXchgList (jr, JR_usage, &usage);
      
      if (commit_flags & COMMIT_NEVER_RAN)
         lSetUlong(jr, JR_wait_status, SGE_NEVERRAN_BIT);
   } 

   if (diagnosis)
      lSetString(jr, JR_err_str, diagnosis);
   else {
      if (!lGetString(jr, JR_err_str)) {
         if (SGE_GET_NEVERRAN(lGetUlong(jr, JR_wait_status)))
            lSetString(jr, JR_err_str, "Job never ran");
         else
            lSetString(jr, JR_err_str, "Unknown job finish condition");
      }
   }

   sge_add_event( 0, sgeE_JOB_FINISH, lGetUlong(jep, JB_job_number), 
                 lGetUlong(jatep, JAT_task_number), NULL, NULL,
                 lGetString(jep, JB_session), jr);

   if (release_jr)
      lFreeElem(jr);

   DEXIT;
   return;
}

static void release_successor_jobs(
lListElem *jep 
) {
   lListElem *jid, *suc_jep;
   u_long32 job_ident;

   DENTER(TOP_LAYER, "release_successor_jobs");

   for_each(jid, lGetList(jep, JB_jid_sucessor_list)) {
      suc_jep = job_list_locate(Master_Job_List, lGetUlong(jid, JRE_job_number));
      if (suc_jep) {
         /* if we don't find it by job id we try it with the name */
         job_ident = lGetUlong(jep, JB_job_number);
         if (!lDelSubUlong(suc_jep, JRE_job_number, job_ident, JB_jid_predecessor_list)) {

             DPRINTF(("no reference "u32" and %s to job "u32" in predecessor list of job "u32"\n", 
               job_ident, lGetString(jep, JB_job_name),
               lGetUlong(suc_jep, JB_job_number), lGetUlong(jep, JB_job_number)));
         } else {
            if (lGetList(suc_jep, JB_jid_predecessor_list)) {
               DPRINTF(("removed job "u32"'s dependance from exiting job "u32"\n",
                  lGetUlong(suc_jep, JB_job_number), lGetUlong(jep, JB_job_number)));
            } else 
               DPRINTF(("job "u32"'s job exit triggers start of job "u32"\n",
                  lGetUlong(jep, JB_job_number), lGetUlong(suc_jep, JB_job_number)));
            sge_add_job_event(sgeE_JOB_MOD, suc_jep, NULL);
         }
      }
   }

   DEXIT;
   return;
}

/*****************************************************************************
 Clear granted resources for job.
 job->granted_destin_identifier_list->str0 contains the granted queue
 *****************************************************************************/
/****
 **** if incslots is 1, QU_job_slots_used is decreased by the number of
 **** used slots of this job.
 **** if it is 0, QU_job_slots_used is untouched.
 ****/
static void sge_clear_granted_resources(lListElem *job, lListElem *ja_task,
                                        int incslots) {
   int pe_slots = 0;
   lList *master_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
   u_long32 job_id = lGetUlong(job, JB_job_number);
   u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
   lList *gdi_list = lGetList(ja_task, JAT_granted_destin_identifier_list);
   lListElem *ep;
   lList *answer_list = NULL;
   u_long32 now;
 
   DENTER(TOP_LAYER, "sge_clear_granted_resources");

   if (!job_is_ja_task_defined(job, ja_task_id) ||
       !job_is_enrolled(job, ja_task_id)) { 
      DEXIT;
      return;
   }

   now = sge_get_gmt();

   /* unsuspend queues on subordinate */
   cqueue_list_x_on_subordinate_gdil(master_list, false, gdi_list);


   /* free granted resources of the queue */
   for_each(ep, gdi_list) {
      const char *queue_name = lGetString(ep, JG_qname);
      lListElem *queue = cqueue_list_locate_qinstance(
                              *(object_type_get_master_list(SGE_TYPE_CQUEUE)), 
                              queue_name);

      if (queue) {
         const char *queue_hostname = lGetHost(queue, QU_qhostname);
         int tmp_slot = lGetUlong(ep, JG_slots);

         /* increase free slots */
         pe_slots += tmp_slot;
         if (incslots) {
            lListElem *host;

            /* undebit consumable resources */ 
            host = host_list_locate(Master_Exechost_List, "global");
            if (debit_host_consumable(job, host, Master_CEntry_List, -tmp_slot) > 0) {
               /* this info is not spooled */
               sge_add_event( 0, sgeE_EXECHOST_MOD, 0, 0, 
                             "global", NULL, NULL, host);
               reporting_create_host_consumable_record(&answer_list, host, now);
               answer_list_output(&answer_list);
               lListElem_clear_changed_info(host);
            }
            host = host_list_locate(Master_Exechost_List, queue_hostname);
            if (debit_host_consumable(job, host, Master_CEntry_List, -tmp_slot) > 0) {
               /* this info is not spooled */
               sge_add_event( 0, sgeE_EXECHOST_MOD, 0, 0, 
                             queue_hostname, NULL, NULL, host);
               reporting_create_host_consumable_record(&answer_list, host, now);
               answer_list_output(&answer_list);
               lListElem_clear_changed_info(host);
            }
            qinstance_debit_consumable(queue, job, Master_CEntry_List, -tmp_slot);
            reporting_create_queue_consumable_record(&answer_list, queue, now);
         }

         /* this info is not spooled */
         qinstance_add_event(queue, sgeE_QINSTANCE_MOD);
         lListElem_clear_changed_info(queue);

      } else {
         ERROR((SGE_EVENT, MSG_QUEUE_UNABLE2FINDQ_S, queue_name));
      }
   }

   /* free granted resources of the parallel environment */
   if (lGetString(ja_task, JAT_granted_pe)) {
      if (incslots) {
         lListElem *pe = pe_list_locate(Master_Pe_List, lGetString(ja_task, JAT_granted_pe));

         if (!pe) {
            ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDPE_S,
                  lGetString(ja_task, JAT_granted_pe)));
         } else {
            pe_debit_slots(pe, -pe_slots, job_id);
            /* this info is not spooled */
            sge_add_event(0, sgeE_PE_MOD, 0, 0, 
                          lGetString(ja_task, JAT_granted_pe), NULL, NULL, pe);
            lListElem_clear_changed_info(pe);
         }
      }
      lSetString(ja_task, JAT_granted_pe, NULL);
   }

   /* forget about old tasks */
   lSetList(ja_task, JAT_task_list, NULL);
         
   /* remove granted_destin_identifier_list */
   lSetList(ja_task, JAT_granted_destin_identifier_list, NULL);
   lSetString(ja_task, JAT_master_queue, NULL);

   DEXIT;
   return;
}

/* what we do is:
      if there is a hard request for this rlimit then we replace
      the queue's value by the job request 

   no need to compare both values - this is schedd's job
*/

static void reduce_queue_limit(
lListElem *qep,
lListElem *jep,
int nm,
char *rlimit_name 
) {
   const char *s;
   lListElem *res;
   int found = 0;

   DENTER(BASIS_LAYER, "reduce_queue_limit");

   for_each (res, lGetList(jep, JB_hard_resource_list)) {
      if (!strcmp(lGetString(res, CE_name), rlimit_name)) {
         if ((s = lGetString(res, CE_stringval))) {
            DPRINTF(("job reduces queue limit: %s = %s (was %s)\n", 
                     rlimit_name, s, lGetString(qep, nm)));
            lSetString(qep, nm, s);
            found = 1;
            break;
         }  
      }
   } 

   /* enforce default request if set, but only if the consumable is
      really used to manage resources of this queue, host or globally */
   if (!found) {
      lListElem *dcep;
      if ((dcep=centry_list_locate(Master_CEntry_List, rlimit_name))
               && lGetBool(dcep, CE_consumable))
         if ( lGetSubStr(qep, CE_name, rlimit_name, QU_consumable_config_list) ||
              lGetSubStr(host_list_locate(Master_Exechost_List, lGetHost(qep, QU_qhostname)),
                     CE_name, rlimit_name, EH_consumable_config_list) ||
              lGetSubStr(host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME),
                     CE_name, rlimit_name, EH_consumable_config_list))
            /* managed at queue level, managed at host level or managed at
               global level */
            lSetString(qep, nm, lGetString(dcep, CE_default));
   }

   
   DEXIT;
   return;
}


/*-------------------------------------------------------------------------*/
/* unlink/rename the job specific files on disk, send event to scheduler   */
/*-------------------------------------------------------------------------*/
static int sge_bury_job(lListElem *job, u_long32 job_id, lListElem *ja_task, 
                        int spool_job, int no_events) 
{
   u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
   int remove_job = (!ja_task || job_get_ja_tasks(job) == 1);

   DENTER(TOP_LAYER, "sge_bury_job");

   job = job_list_locate(Master_Job_List, job_id);
   te_delete_one_time_event(TYPE_SIGNAL_RESEND_EVENT, job_id, ja_task_id, NULL);


   /*
    * Remove the job with the last task
    * or
    * Remove one ja task
    */
   if (remove_job) {
/*       job_log(job_id, ja_task_id, MSG_LOG_EXITED); */
      release_successor_jobs(job);

      /*
       * security hook
       */
      delete_credentials(job);

      /* 
       * do not try to remove script file for interactive jobs 
       */
      if (lGetString(job, JB_script_file)) {
         unlink(lGetString(job, JB_exec_file));
      }
      {
         lList *answer_list = NULL;
         dstring buffer = DSTRING_INIT;
         spool_delete_object(&answer_list, spool_get_default_context(), 
                             SGE_TYPE_JOB, 
                             job_get_key(job_id, 0, NULL, &buffer));
         answer_list_output(&answer_list);
         sge_dstring_free(&buffer);
      }
      /*
       * remove the job
       */
      suser_unregister_job(job);
      if (!no_events) {
         sge_add_event( 0, sgeE_JOB_DEL, job_id, ja_task_id, 
                       NULL, NULL, lGetString(job, JB_session), NULL);
      }
      lRemoveElem(Master_Job_List, job);
   } else {
      int is_enrolled = job_is_enrolled(job, ja_task_id);

/*       job_log(job_id, ja_task_id, MSG_LOG_JATASKEXIT); */

      if (!no_events) {
         sge_add_event( 0, sgeE_JATASK_DEL, job_id, ja_task_id, 
                       NULL, NULL, lGetString(job, JB_session), NULL);
      }

      /*
       * remove the task
       */
      if (is_enrolled) {
         lList *answer_list = NULL;
         dstring buffer = DSTRING_INIT;
         spool_delete_object(&answer_list, spool_get_default_context(), 
                             SGE_TYPE_JOB, 
                             job_get_key(job_id, ja_task_id, NULL, &buffer));
         answer_list_output(&answer_list);
         sge_dstring_free(&buffer);
         lRemoveElem(lGetList(job, JB_ja_tasks), ja_task);
      } else {
         job_delete_not_enrolled_ja_task(job, NULL, ja_task_id);
         if (spool_job) {
            lList *answer_list = NULL;
            dstring buffer = DSTRING_INIT;
            spool_write_object(&answer_list, spool_get_default_context(), job, 
                               job_get_key(job_id, ja_task_id, NULL, &buffer), 
                               SGE_TYPE_JOB);
            answer_list_output(&answer_list);
            sge_dstring_free(&buffer);
         }
      }
   }

   DEXIT;
   return True;
}

static int sge_to_zombies(lListElem *job, lListElem *ja_task, int spool_job) 
{
   u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
   u_long32 job_id = lGetUlong(job, JB_job_number);
   int is_defined;
   DENTER(TOP_LAYER, "sge_to_zombies");


   is_defined = job_is_ja_task_defined(job, ja_task_id); 
   if (is_defined) {
      lListElem *zombie = job_list_locate(Master_Zombie_List, job_id);

      /*
       * Create zombie job list if it does not exist
       */
      if (Master_Zombie_List == NULL) {
         Master_Zombie_List = lCreateList("Master_Zombie_List", JB_Type);
      }

      /*
       * Create zombie job if it does not exist 
       * (don't copy unnecessary sublists)
       */
      if (zombie == NULL) {
         lList *n_h_ids = NULL;     /* RN_Type */ 
         lList *u_h_ids = NULL;     /* RN_Type */ 
         lList *o_h_ids = NULL;     /* RN_Type */ 
         lList *s_h_ids = NULL;     /* RN_Type */ 
         lList *ja_tasks = NULL;    /* JAT_Type */ 

         lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
         lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
         lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
         lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
         lXchgList(job, JB_ja_tasks, &ja_tasks);
         zombie = lCopyElem(job);
         lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
         lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
         lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
         lXchgList(job, JB_ja_tasks, &ja_tasks);
         lAppendElem(Master_Zombie_List, zombie);
      }

      /*
       * Add the zombie task id
       */
      if (zombie) {
         job_add_as_zombie(zombie, NULL, ja_task_id); 
      } 

      /* 
       * Spooling
       */
      if (spool_job) {
/*          job_write_spool_file(zombie, ja_task_id, NULL, SPOOL_HANDLE_AS_ZOMBIE); */
      }
   } else {
      WARNING((SGE_EVENT, "It is impossible to move task "u32" of job "u32
               " to the list of finished jobs\n", ja_task_id, job_id)); 
   }

   DEXIT;
   return True;
}
