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

#include "sge_answerL.h"
#include "sge_eventL.h"
#include "sge_hostL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_usageL.h"
#include "sge_queueL.h"
#include "sge_ckptL.h"
#include "sge_job_refL.h"
#include "sge_requestL.h"
#include "sge_complexL.h"
#include "sge_time_eventL.h"
#include "sge_job_reportL.h"

#include "basis_types.h"
#include "sgermon.h"
#include "sge_str_from_file.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge.h"
#include "def.h"
#include "sge_conf.h"
#include "sge_arch.h"
#include "time_event.h"
#include "commlib.h"
#include "job_log.h"
#include "pack_job_delivery.h"
#include "subordinate_qmaster.h" 
#include "sge_complex_schedd.h" 
#include "sge_queue_qmaster.h" 
#include "sge_ckpt_qmaster.h"
#include "sge_job.h" 
#include "job_exit.h" 
#include "sge_give_jobs.h"
#include "read_write_job.h"
#include "job.h"
#include "sge_host_qmaster.h"
#include "sge_host.h"
#include "sge_pe_qmaster.h"
#include "sge_m_event.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "slots_used.h"
#include "sge_select_queue.h"
#include "sort_hosts.h"
#include "sge_afsutil.h"
#include "sge_peopen.h"
#include "sge_copy_append.h"
#include "sge_user_mapping.h"
#include "sge_switch_user.h"
#include "setup_path.h"
#include "execution_states.h"
#include "jb_now.h"
#include "sge_parse_num_par.h"
#include "symbols.h"
#include "mail.h"
#include "reschedule.h"
#include "sge_security.h"
#include "sge_peL.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_range.h"
#include "sge_job_jatask.h"

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

extern lList *Master_Queue_List;
extern lList *Master_Job_List;
extern lList *Master_Zombie_List;
extern lList *Master_Pe_List;
extern lList *Master_Complex_List;
extern lList *Master_Exechost_List;

#ifndef __SGE_NO_USERMAPPING__
extern lList *Master_Usermapping_Entry_List;
extern lList *Master_Host_Group_List;
#endif

static void sge_clear_granted_resources(lListElem *jep, lListElem *jatep, int incslots);

static void reduce_queue_limit(lListElem *qep, lListElem *jep, int nm, char *rlimit_name);

static void release_successor_jobs(lListElem *jep);

static int send_job(const char *rhost, const char *target, lListElem *jep, lListElem *jatep, lListElem *pe, lListElem *hep, int master);

static int sge_bury_job(lListElem *jep, u_long32 jid, lListElem *jatep, int spool_job);

static int sge_to_zombies(lListElem *jep, lListElem *jatep, int spool_job);

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
   
   /*
   originalUser = sge_strdup(NULL,lGetString(jep, JB_owner));
   DPRINTF(("sge_give_job: Starting job from %s\n", originalUser));
   mappedUser = sge_malloc_map_out_going_username(originalUser, lGetHost(master_qep, QU_qhostname));
   if (mappedUser == NULL) {
      DPRINTF(("user %s not mapped !\n",lGetString(jep, JB_owner) ));
      FREE(originalUser);
      originalUser = NULL;
   } else {
      DPRINTF(("user %s mapped to %s!\n", lGetString(jep, JB_owner) , mappedUser ));
      lSetString(jep,JB_owner,mappedUser );
   }
   */

   target = prognames[EXECD];
   rhost = lGetHost(master_qep, QU_qhostname);
   DPRINTF(("execd host: %s\n", rhost));

   /* build complex list for all queues in the gdil list */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      lList *resources;
      lListElem *ep1, *ep2, *cep;

      if ((q = sge_locate_queue(lGetString(gdil_ep, JG_qname)))) {
         resources = NULL;
         queue_complexes2scheduler(&resources, q, Master_Exechost_List, Master_Complex_List, 0);
         lSetList(gdil_ep, JG_complex, resources);
      }

      /* job requests overrides complex list */
      for_each (ep1, lGetList(jep, JB_hard_resource_list)) {
         for_each (ep2, lGetList(ep1, RE_entries)) {
            if ((cep=lGetElemStr(lGetList(gdil_ep, JG_complex), CE_name, lGetString(ep2, CE_name))))  {  
               lSetString(cep, CE_stringval, lGetString(ep2, CE_stringval));
               DPRINTF(("complex: %s = %s\n", lGetString(ep2, CE_name), lGetString(ep2, CE_stringval)));
               break;
            }
            if (!cep) {
               DPRINTF(("complex %s not found\n", lGetString(ep2, CE_name)));
            }
         }
      }
   }

   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      lListElem *slave_hep;

      if (lGetUlong(gdil_ep, JG_tag_slave_job)) {
         if (!(slave_hep = sge_locate_host(lGetHost(gdil_ep, JG_qhostname), 
            SGE_EXECHOST_LIST))) {
            ret = -1;   
            break;
         }

         /* put JG_task_id_range into JB_task_id_range */
         lSetUlong(jep, JB_task_id_range, lGetUlong(gdil_ep, JG_task_id_range));


         if (send_job(lGetHost(gdil_ep, JG_qhostname), target, jep, jatep, pe, slave_hep, 0)) {
            ret = -1;   
            break;
         }


         sent_slaves = 1;
      }
   }

   if (!sent_slaves) {
      /* wait till all slaves are acked */
      lSetUlong(jep, JB_task_id_range, 0);
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
   int execd_enrolled;  
   u_long32 now;
   sge_pack_buffer pb;
   u_long32 dummymid;
   lListElem *tmpjep, *qep, *tmpjatep, *next_tmpjatep;
   lListElem *ckpt = NULL, *tmp_ckpt;
   lList *ckpt_lp, *qlp;
   const char *ckpt_name;
   static u_short number_one = 1;
   lListElem *gdil_ep;
   char *str;

#ifndef __SGE_NO_USERMAPPING__
   char *mappedUser = NULL;   
/*    char *originalUser = NULL;  */
#endif
  
   DENTER(TOP_LAYER, "send_job");

   /* do ask_commproc() only if we are missing load reports */
   now = sge_get_gmt();
   if (last_heard_from(target, &number_one, rhost)+
      load_report_interval(hep)*2 <= now) {

      execd_enrolled = ask_commproc(rhost, target, 0);
      if (execd_enrolled) {
         ERROR((SGE_EVENT, MSG_COM_NOTENROLLEDONHOST_SSU, 
               target, rhost, u32c(lGetUlong(jep, JB_job_number))));
         sge_mark_unheard(hep, target);

         DEXIT;
         return -1;
      }
   }

   /* load script into job structure for sending to execd */
   /*
   ** if exec_file is not set, then this is an interactive job
   */
   if (master && lGetString(jep, JB_exec_file)) {

      str = str_from_file(lGetString(jep, JB_exec_file), &len);
      lSetString(jep, JB_script_ptr, str);
      FREE(str);
      lSetUlong(jep, JB_script_size, len);
   }

   tmpjep = lCopyElem(jep);

#ifdef QIDL
   /* append additional environment variables (SGE_MASTER_IOR and */
   /* SGE_JOB_AUTH) */
   /* these should not be visible to the user in the master cull list */
   /* so this is the best place to put it */
   /* TODO: encode JOB_AUTH */
   {
      lList* envlp;
      lListElem* env;
      char job_id[25];
      sprintf(job_id, u32, lGetUlong(tmpjep, JB_job_number));
      envlp = lGetList(tmpjep, JB_env_list);
      if(!envlp)
         envlp = lCreateList("Environment", VA_Type);
      else
         envlp = lCopyList("Envirionment", envlp);
      env = lCreateElem(VA_Type);
      lSetString(env, VA_variable, "SGE_MASTER_IOR");
      lSetString(env, VA_value, (char*)get_master_ior());
      lAppendElem(envlp, env);
      env = lCreateElem(VA_Type);
      lSetString(env, VA_variable, "SGE_JOB_AUTH");
      lSetString(env, VA_value, job_id);
      lAppendElem(envlp, env);
      lSetList(tmpjep, JB_env_list, envlp);
   }
#endif

   /* insert ckpt object if required **now**. it is only
   ** needed in the execd and we have no need to keep it
   ** in qmaster's permanent job list
   */
   if ((ckpt_name=lGetString(tmpjep, JB_checkpoint_object))) {
      ckpt = sge_locate_ckpt(ckpt_name);
      if (!ckpt) {
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDCKPT_S, ckpt_name));
         DEXIT;
         return -1;
      }

      /* get the necessary memory */
      if (!(ckpt_lp=lCreateList("jobs ckpt definition", CK_Type)) ||
          !(tmp_ckpt=lCopyElem(ckpt))) {
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2CREATECKPT_SU, 
                ckpt_name, u32c(lGetUlong(tmpjep, JB_job_number))));
         DEXIT;
         return -1;
      }
      /* everything's in place. stuff it in */
      lAppendElem(ckpt_lp, tmp_ckpt);
      lSetList(tmpjep, JB_checkpoint_object_list, ckpt_lp);
   }
   
   qlp = lCreateList("qlist", QU_Type);
   /* add all queues referenced in gdil to qlp 
      (necessary for availability of ALL resource limits and tempdir in queue) */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      qep = lCopyElem(lGetElemStr(Master_Queue_List, QU_qname, lGetString(gdil_ep, JG_qname)));

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
   DPRINTF(("send_job(): Starting job of %s\n",  lGetString(tmpjep, JB_owner)  ));
   
   if (Master_Usermapping_Entry_List != NULL) {
      mappedUser = sge_malloc_map_out_going_username( Master_Host_Group_List,
                                                      Master_Usermapping_Entry_List,
                                                      lGetString(tmpjep, JB_owner) , 
                                                      rhost);
      if (mappedUser != NULL) {
         DPRINTF(("execution mapping: user %s mapped to %s on host %s\n", lGetString(tmpjep, JB_owner) , mappedUser , rhost));
  /*     INFO((SGE_EVENT, MSG_MAPPING_USERXMAPPEDTOYONHOSTZ_SSS, lGetString(tmpjep, JB_owner) , mappedUser , rhost )); */
         lSetString(tmpjep,JB_owner,mappedUser );
         FREE(mappedUser);
         mappedUser = NULL;
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

   failed = gdi_send_message_pb(0, target, 0, rhost, 
             master?TAG_JOB_EXECUTION:TAG_SLAVE_ALLOW, 
             &pb, &dummymid);

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

   if (failed) { /* we failed sending the job to the execd */
      ERROR((SGE_EVENT, MSG_COM_SENDJOBTOHOST_US,   
            u32c(lGetUlong(jep, JB_job_number)), rhost));
      sge_mark_unheard(hep, target);
      DEXIT;
      return -1;
   } else {
      DPRINTF(("successfully sent %sjob "u32" to host \"%s\"\n", 
            master?"":"SLAVE ", lGetUlong(jep, JB_job_number), rhost));
   }

   DEXIT;
   return 0;
}

void resend_job(
u_long32 type,
u_long32 when,
u_long32 jobid,
u_long32 jataskid,
const char *queue 
) {
   lListElem* jep, *jatep, *ep, *hep, *pe, *mqep;
   lList* jatasks;
   const char *qnm, *hnm;
   time_t now;
   
   DENTER(TOP_LAYER, "resend_job");

   jep = sge_locate_job(jobid);
   jatep = search_task(jataskid, jep);
   now = sge_get_gmt();

   if(!jep || !jatep) {
      WARNING((SGE_EVENT, MSG_COM_RESENDUNKNOWNJOB_UU, 
               u32c(jobid), u32c(jataskid)));
      DEXIT;
      return;
   }

   jatasks = lGetList(jep, JB_ja_tasks);

   /* check whether a slave execd allowance has to be retransmitted */
   if (lGetUlong(jatep, JAT_status) == JTRANSITING) {
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

      if (!(mqep = lGetElemStr(Master_Queue_List, QU_qname, qnm))) {
         ERROR((SGE_EVENT, MSG_JOB_NOQUEUE4TJ_SUU,  
               qnm, u32c(jobid), u32c(jataskid)));
         lDelElemUlong(&jatasks, JAT_task_number, jataskid);
         DEXIT;
         return;
      }
      if (!(hnm=lGetHost(mqep, QU_qhostname)) ||
          !(hep = sge_locate_host(hnm, SGE_EXECHOST_LIST))) {
         ERROR((SGE_EVENT, MSG_JOB_NOHOST4TJ_SUU, 
                hnm, u32c(jobid), u32c(jataskid)));
         lDelElemUlong(&jatasks, JAT_task_number, jataskid);
         DEXIT;
         return;
      }

      if ((lGetUlong(mqep, QU_state) & QUNKNOWN)) {
         trigger_job_resend(now, hep, jobid, jataskid);
         return; /* try later again */
      }

      if (lGetString(jatep, JAT_granted_pe)) {
         if (!(pe = sge_locate_pe(lGetString(jatep, JAT_granted_pe)))) {
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
         ERROR((SGE_EVENT, MSG_JOB_DELIVER2Q_UUS, 
                u32c(jobid), u32c(jataskid), 
                lGetString(jatep, JAT_master_queue)));
      }

      sge_give_job(jep, jatep, mqep, pe, hep);
 
      /* reset timer */
      lSetUlong(jatep, JAT_start_time, now);
      trigger_job_resend(now, hep, lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number));

   } 

   DEXIT;
}


void cancel_job_resend(
u_long32 jid,
u_long32 tid 
) {
   DENTER(TOP_LAYER, "cancel_job_resend");
   DPRINTF(("CANCEL JOB RESEND "u32"/"u32"\n", jid, tid)); 
   te_delete(TYPE_JOB_RESEND_EVENT, NULL, jid, tid);
}

/* 
 * if hep equals to NULL resend is triggered immediatelly 
 */ 
void trigger_job_resend(
u_long32 now,
lListElem *hep,
u_long32 jid,
u_long32 tid 
) {
   u_long32 seconds;
   DENTER(TOP_LAYER, "trigger_job_resend");
   seconds = hep? MAX(load_report_interval(hep), MAX_JOB_DELIVER_TIME):0;
   DPRINTF(("TRIGGER JOB RESEND "u32"/"u32" in %d seconds\n", jid, tid, seconds)); 

   te_delete(TYPE_JOB_RESEND_EVENT, NULL, jid, tid);
   te_add(TYPE_JOB_RESEND_EVENT, now + seconds, jid, tid, NULL);
}


/***********************************************************************
 ck_4_zombie_jobs

 Remove zombie jobs, which have expired (currently, we keep a list of
 conf.zombie_jobs entries)
 ***********************************************************************/
void ck_4_zombie_jobs(
u_long now 
) {
   lListElem *dep;
   
   DENTER(TOP_LAYER, "ck_4_zombie_jobs");

   while (Master_Zombie_List &&
            (lGetNumberOfElem(Master_Zombie_List) > conf.zombie_jobs)) {
      dep = lFirst(Master_Zombie_List);
      job_remove_spool_file(lGetUlong(dep, JB_job_number), 0, 
                            SPOOL_HANDLE_AS_ZOMBIE);
      lRemoveElem(Master_Zombie_List, dep);
   }

   DEXIT;
}

/***********************************************************************

   sge_commit_job - commits that a job has been started

   First call (with mode == 0):
     We sent the job to the execd. But we dont want to wait for the 
     acknowledge now. So we have to reserve the resources for the meantime.

   If the execd sends a positive Ack we make the commit permanent 
   (sge_commit_job() with mode==1).

   If we got no response or a failed job from the execd we have to free the
   resources for further usage. (mode==2) 

   Hearing about a jobs exit from execd there are two cases:

   SGE
      If we get a job exit that leads to a deletion of the job we have 
      to free the resources for further usage and . (mode==3)
      This is the same as mode=2 except we delete the job from disk and
      generate a delete event 

   SGEEE
      If we get a job exit that leads to a deletion of the job we have 
      to free the resources for further usage (mode==4)
      Here we remove the job script but not the job file itself. 

      Getting the permission from SGEEE schedd to remove the job (mode==5)
      we may delete the job itself from internal lists (joblist)
      and we may remove it the job file.

   Input:
   job we want to commit
   Master_Queue_List with queues tagged which we want to be filled by the job.
      (queue->tagged = number of job nodes to run in this queue)

   Output:
   Master_Queue_List queue->job_list of the concerned queues are filled with the
   job nodes. entry: int0=jobid; int1=MASTER|SLAVE
   The tags are removed from the queues.
   job->start_time is set to the actual time.
   Changes of queues/jobs are spooled to disk.
 ***********************************************************************/
void sge_commit_job(
lListElem *jep,
lListElem *jatep,
int mode,
int spool_job 
) {
   lListElem *qep, *hep, *task, *tmp_ja_task;
   int slots;
   lUlong jid, tid;
   int no_unlink = 0;
   time_t now = 0;
   lListElem *schedd;

   DENTER(TOP_LAYER, "sge_commit_job");

   schedd = sge_locate_scheduler();

   jid = lGetUlong(jep, JB_job_number);
   tid = jatep?lGetUlong(jatep, JAT_task_number):0;

   switch (mode) {
   case 0:
      lSetUlong(jatep, JAT_state, JRUNNING);
      lSetUlong(jatep, JAT_status, JTRANSITING);
      job_log(jid, tid, MSG_LOG_SENT2EXECD);

      /* should use jobs gdil instead of tagging queues */
      for_each(qep, Master_Queue_List) {
         if (lGetUlong(qep, QU_tagged)) {
            slots = lGetUlong(qep, QU_tagged);
            lSetUlong(qep, QU_tagged, 0);

            /* debit in all layers */
            if (debit_host_consumable(jep, hep=sge_locate_host("global", SGE_EXECHOST_LIST), 
                     Master_Complex_List, slots)>0)
               sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, "global", hep);

            if (debit_host_consumable(jep, hep=sge_locate_host(lGetHost(qep, QU_qhostname), 
                     SGE_EXECHOST_LIST), Master_Complex_List, slots)>0)
               sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, lGetHost(qep, QU_qhostname), hep);

            debit_queue_consumable(jep, qep, Master_Complex_List, slots);
            sge_add_queue_event(sgeE_QUEUE_MOD, qep);
         }

      }

      now = sge_get_gmt();
      lSetUlong(jatep, JAT_start_time, now);
      job_enroll(jep, tid);
      job_write_spool_file(jep, tid, SPOOL_DEFAULT);
      sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
      break;

   case 1:
      lSetUlong(jatep, JAT_status, JRUNNING);
      job_log(jid, tid, "job received by execd");
      job_enroll(jep, tid);
      job_write_spool_file(jep, tid, SPOOL_DEFAULT);
      break;

   case 2:
   case 8:
      WARNING((SGE_EVENT, MSG_JOB_RESCHEDULE_UU,
               u32c(lGetUlong(jep, JB_job_number)), 
               u32c(lGetUlong(jatep, JAT_task_number))));

      /* add a reschedule unknown list entry to all slave
         hosts where a part of that job ran */
      {
         lListElem *granted_queue;
         const char *master_host = NULL;
         lListElem *pe;
         const char *pe_name;      

         pe_name = lGetString(jep, JB_pe);
         if (pe_name) {
            pe = lGetElemStr(Master_Pe_List, PE_name, pe_name);
            if (pe && lGetUlong(pe, PE_control_slaves)) { 
               for_each(granted_queue, 
                     lGetList(jatep, JAT_granted_destin_identifier_list)) {
                  lListElem *host;

                  if (!master_host) {
                     master_host = lGetHost(granted_queue, JG_qhostname);
                  } 
                  
                  if (strcasecmp(master_host, lGetHost(granted_queue, JG_qhostname))) {
                     host = lGetElemHost(Master_Exechost_List, EH_name, lGetHost(granted_queue, JG_qhostname)); 
                     
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
            host = lGetElemHost(Master_Exechost_List, EH_name, lGetHost(granted_queue, JG_qhostname));
            add_to_reschedule_unknown_list(host, lGetUlong(jep, JB_job_number),
                                           lGetUlong(jatep, JAT_task_number),
                                           RESCHEDULE_SKIP_JR);
         }
      }

      lSetUlong(jatep, JAT_status, JIDLE);
      lSetUlong(jatep, JAT_state, (mode==2) ?(JQUEUED|JWAITING): (JQUEUED|JWAITING|JERROR));

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

      /* add sub-task usage to previous job usage */
      for_each(task, lGetList(jatep, JAT_task_list)) {
         lListElem *dst, *src, *task_ja_task;
         lList *usage;

         for_each(task_ja_task, lGetList(task, JB_ja_tasks)) {
            for_each(src, lGetList(task_ja_task, JAT_scaled_usage_list)) {
               if ((dst=lGetSubStr(jatep, UA_name, lGetString(src, UA_name), JAT_previous_usage_list)))
                  lSetDouble(dst, UA_value, lGetDouble(dst, UA_value) + lGetDouble(src, UA_value));
               else {                                                                       
                  if (!(usage = lGetList(jatep, JAT_previous_usage_list))) {           
                     usage = lCreateList("usage", UA_Type);                                 
                     lSetList(jatep, JAT_previous_usage_list, usage);                            
                  }                                                                         
                  lAppendElem(usage, lCopyElem(src));                                      
               }
            }
         }
      }

      sge_clear_granted_resources(jep, jatep, 1);
      job_enroll(jep, tid);
      job_write_spool_file(jep, tid, SPOOL_DEFAULT);
      sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
      if(schedd != NULL) {
         sge_flush_events(schedd, FLUSH_EVENTS_JOB_FINISHED);
      }
      break;

   case 3:
      job_log(jid, tid, MSG_LOG_EXITED);
      if (conf.zombie_jobs > 0)
         sge_to_zombies(jep, jatep, spool_job);
      sge_clear_granted_resources(jep, jatep, 1);
      sge_bury_job(jep, jid, jatep, spool_job);
      if(schedd != NULL) {
         sge_flush_events(schedd, FLUSH_EVENTS_JOB_FINISHED);
      }
      break;


   case 4:
      jid = lGetUlong(jep, JB_job_number);
      job_log(jid, tid, MSG_LOG_WAIT4SGEDEL);

      lSetUlong(jatep, JAT_status, JFINISHED);
      if (conf.zombie_jobs > 0)
         sge_to_zombies(jep, jatep, spool_job);
      sge_clear_granted_resources(jep, jatep, 1);
      job_enroll(jep, tid);
      job_write_spool_file(jep, tid, SPOOL_DEFAULT);
      for_each(task, lGetList(jatep, JAT_task_list)) {
         lListElem* task_ja_task;

         for_each (task_ja_task, lGetList(task, JB_ja_tasks)) {   
            sge_add_list_event(NULL, sgeE_JOB_FINAL_USAGE, lGetUlong(jep, JB_job_number),
               lGetUlong(task_ja_task, JAT_task_number), 
               lGetString(task, JB_pe_task_id_str), 
               lGetList(task_ja_task, JAT_scaled_usage_list));
         }
      }
      sge_add_list_event(NULL, sgeE_JOB_FINAL_USAGE, jid = lGetUlong(jep, JB_job_number), 
         lGetUlong(jatep, JAT_task_number),
         NULL, lGetList(jatep, JAT_scaled_usage_list));

      if(schedd != NULL) {   
         sge_flush_events(schedd, FLUSH_EVENTS_JOB_FINISHED);
      }
         
      /* finished all ja-tasks => remove job script */
      for_each(tmp_ja_task, lGetList(jep, JB_ja_tasks)) {
         if (lGetUlong(tmp_ja_task, JAT_status) != JFINISHED)
            no_unlink = 1;
      }
      if (!no_unlink) {
         release_successor_jobs(jep);
         unlink(lGetString(jep, JB_exec_file));
      }
      break;

   case 5: /* triggered by ORT_remove_job */
   case 6: /* triggered by ORT_remove_immediate_job */
      job_log(jid, tid, (mode==5) ?  MSG_LOG_DELSGE : MSG_LOG_DELIMMEDIATE);
      jid = lGetUlong(jep, JB_job_number);
      sge_bury_job(jep, jid, jatep, spool_job);
      break;
   
   case 7: /*  this is really case 2.5! The same as case 2 except 
               sge_clear_granted_resources is told not to increase free slots. */
      WARNING((SGE_EVENT, 
               MSG_JOB_RESCHEDULE_UU, 
               u32c(lGetUlong(jep, JB_job_number)), 
               u32c(lGetUlong(jatep, JAT_task_number))));
      lSetUlong(jatep, JAT_status, JIDLE);
      lSetUlong(jatep, JAT_state, JQUEUED | JWAITING);
      sge_clear_granted_resources(jep, jatep, 0);
      job_enroll(jep, tid);
      job_write_spool_file(jep, tid, SPOOL_DEFAULT);
      sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);

      if(schedd != NULL) {
         sge_flush_events(schedd, FLUSH_EVENTS_JOB_FINISHED);
      }
      break;
   }

   DEXIT;
   return;
}

static void release_successor_jobs(
lListElem *jep 
) {
   lListElem *jid, *suc_jep;
   char job_ident[256];

   DENTER(TOP_LAYER, "release_successor_jobs");

   for_each(jid, lGetList(jep, JB_jid_sucessor_list)) {
      suc_jep = sge_locate_job(lGetUlong(jid, JRE_job_number));
      if (suc_jep) {
         /* if we don't find it by job id we try it with the name */
         sprintf(job_ident, u32, lGetUlong(jep, JB_job_number));
         if (!lDelSubStr(suc_jep, JRE_job_name, job_ident, JB_jid_predecessor_list) &&
             !lDelSubStr(suc_jep, JRE_job_name, lGetString(jep, JB_job_name), JB_jid_predecessor_list)) {
             DPRINTF(("no reference %s and %s to job "u32" in predecessor list of job "u32"\n", 
               job_ident, lGetString(jep, JB_job_name),
               lGetUlong(suc_jep, JB_job_number), lGetUlong(jep, JB_job_number)));
         } else {
            if (lGetList(suc_jep, JB_jid_predecessor_list)) {
               DPRINTF(("removed job "u32"'s dependance from exiting job "u32"\n",
                  lGetUlong(suc_jep, JB_job_number), lGetUlong(jep, JB_job_number)));
            } else 
               DPRINTF(("job "u32"'s job exit triggers start of job "u32"\n",
                  lGetUlong(jep, JB_job_number), lGetUlong(suc_jep, JB_job_number)));
            sge_add_job_event(sgeE_JOB_MOD, suc_jep, 0);
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
static void sge_clear_granted_resources(
lListElem *jep,
lListElem *jatep,
int incslots 
) {
   lListElem *ep, *qep, *hep;
   int tmp_slot, pe_slots = 0;

   DENTER(TOP_LAYER, "sge_clear_granted_resources");

   /* unsuspend queues on subordinate */
   usos_using_gdil(lGetList(jatep, JAT_granted_destin_identifier_list), lGetUlong(jep, JB_job_number));

   /* free granted resources of the queue */
   for_each(ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {

      if ((qep = lGetElemStr(Master_Queue_List, QU_qname, lGetString(ep, JG_qname)))) {

         tmp_slot = lGetUlong(ep, JG_slots);

         /* increase free slots */
         pe_slots += tmp_slot;
         if (incslots) {

            /* undebit consumable resources */ 
            if (debit_host_consumable(jep, (hep=sge_locate_host("global", SGE_EXECHOST_LIST)), 
                  Master_Complex_List, -tmp_slot))
               sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, "global", hep);

            if (debit_host_consumable(jep, (hep=sge_locate_host(lGetHost(qep, QU_qhostname), 
                  SGE_EXECHOST_LIST)), Master_Complex_List, -tmp_slot))
               sge_add_event(NULL, sgeE_EXECHOST_MOD, 0, 0, lGetHost(qep, QU_qhostname), hep);

            debit_queue_consumable(jep, qep, Master_Complex_List, -tmp_slot);
         }

         sge_add_queue_event(sgeE_QUEUE_MOD, qep);
      }
      else
         ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDQ_S, 
               lGetString(ep, JG_qname)));
   }

   /* free granted resources of the parallel environment */
   if (lGetString(jatep, JAT_granted_pe)) {
      lListElem *pe;

      if (incslots) {
         pe = sge_locate_pe(lGetString(jatep, JAT_granted_pe));
         if (!pe) {
            ERROR((SGE_EVENT, MSG_OBJ_UNABLE2FINDPE_S,
                  lGetString(jatep, JAT_granted_pe)));
         }
         else {
            reverse_job_from_pe(pe, pe_slots, lGetUlong(jep, JB_job_number));
            sge_add_event(NULL, sgeE_PE_MOD, 0, 0, lGetString(jatep, JAT_granted_pe), pe);
         }
      }
      lSetString(jatep, JAT_granted_pe, NULL);
   }

   /* forget about old tasks */
   lSetList(jatep, JAT_task_list, NULL);
         
   /* remove granted_destin_identifier_list */
   lSetList(jatep, JAT_granted_destin_identifier_list, NULL);
   lSetString(jatep, JAT_master_queue, NULL);

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
   lListElem *res, *rep;
   int found = 0;

   DENTER(TOP_LAYER, "reduce_queue_limit");

   for_each (res, lGetList(jep, JB_hard_resource_list)) {
      for_each (rep, lGetList(res, RE_entries)) {
         if (!strcmp(lGetString(rep, CE_name), rlimit_name)) {
            if ((s = lGetString(rep, CE_stringval))) {
               DPRINTF(("job reduces queue limit: %s = %s (was %s)\n", 
                        rlimit_name, s, lGetString(qep, nm)));
               lSetString(qep, nm, s);
               found = 1;
            }  
         }
         if (found)
            break;
      }
      if (found)
         break;
   } 

   /* enforce default request if set, but only if the consumable is
      really used to manage resources of this queue, host or globally */
   if (!found) {
      lListElem *dcep;
      if ((dcep=sge_locate_complex_attr(rlimit_name, Master_Complex_List))
               && lGetUlong(dcep, CE_consumable))
         if ( lGetSubStr(qep, CE_name, rlimit_name, QU_consumable_config_list) ||
              lGetSubStr(sge_locate_host(lGetHost(qep, QU_qhostname), SGE_EXECHOST_LIST),
                     CE_name, rlimit_name, EH_consumable_config_list) ||
              lGetSubStr(sge_locate_host(SGE_GLOBAL_NAME, SGE_EXECHOST_LIST),
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
static int sge_bury_job(
lListElem *jep,
u_long32 jid,
lListElem *jatep,
int spool_job 
) {
   u_long32 tid;
   int RemoveJob;

   DENTER(TOP_LAYER, "sge_bury_job");
   /*
   ** get number of tasks in job
   */
   if ((lGetNumberOfElem(lGetList(jep, JB_ja_tasks)) > 1) && jatep) 
      RemoveJob = 0;
   else
      RemoveJob = 1;

   jep = sge_locate_job(jid);
   tid = lGetUlong(jatep, JAT_task_number);

   te_delete(TYPE_SIGNAL_RESEND_EVENT, NULL, jid, tid);

   if (!RemoveJob) {
      /* Remove one ja task or move it into the Master_Zombie_List*/
      job_log(jid, tid, MSG_LOG_JATASKEXIT);
      sge_add_event(NULL, sgeE_JATASK_DEL, jid, lGetUlong(jatep, JAT_task_number), 
                     NULL, NULL);

      /*
      ** remove the task
      */
      job_remove_spool_file(jid, tid, 0);
      lRemoveElem(lGetList(jep, JB_ja_tasks), jatep);
   } else {
      /* Remove the job with the last task */
      job_log(jid, tid, MSG_LOG_EXITED);
      release_successor_jobs(jep);

      /*
      ** security hook
      */
      delete_credentials(jep);

      /* do not try to remove script file for interactive jobs */
      if (lGetString(jep, JB_script_file)) {
         unlink(lGetString(jep, JB_exec_file));
      }
      sge_add_event(NULL, sgeE_JOB_DEL, jid, lGetUlong(jatep, JAT_task_number), NULL, NULL);
      job_remove_spool_file(jid, 0, 0);

      /*
      ** remove the job
      */
      lRemoveElem(Master_Job_List, jep);
   }

   DEXIT;
   return True;
}

static int sge_to_zombies(
lListElem *jep,
lListElem *jatep,
int spool_job 
) {
   lListElem *zombie = NULL;
   lListElem *zombie_task = NULL;
   u_long32 jid, tid;
   
   DENTER(TOP_LAYER, "sge_to_zombies");
   
   if (!Master_Zombie_List)
         Master_Zombie_List = lCreateList("Master_Zombie_List", JB_Type);

   
   jid = lGetUlong(jep, JB_job_number);
   tid = lGetUlong(jatep, JAT_task_number);
   /*
   ** add task to zombie list
   */
   /* search job in Zombie-list */
   zombie = lGetElemUlong(Master_Zombie_List, JB_job_number, jid);

   /* if we found the job => move task 
      else => copy job and remove all not-zombie-task */
   if (zombie) {
      zombie_task = lCopyElem(jatep);
      lAppendElem(lGetList(zombie, JB_ja_tasks), zombie_task);
   } else {
      lList *tasks;
      /* 
      ** make a copy of the job and remove all tasks 
      ** which are not zombies 
      */ 
      zombie = lCopyElem(jep);
      zombie_task = lCopyElem(jatep);
      tasks = lCreateList("tasks", JAT_Type);
      lAppendElem(tasks, zombie_task);
      lSetList(zombie, JB_ja_tasks, tasks);
      lAppendElem(Master_Zombie_List, zombie);
   }

   job_write_spool_file(zombie, tid, SPOOL_HANDLE_AS_ZOMBIE);

   DEXIT;
   return True;
}
