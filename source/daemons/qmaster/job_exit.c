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
#include <errno.h>
#include <stdlib.h>

#include "def.h"
#include "sge.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_hostL.h"
#include "sge_queueL.h"
#include "sge_job_report.h"
#include "sge_job_refL.h"
#include "sge_job.h"
#include "sge_queue_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_host.h"
#include "job_log.h"
#include "job_exit.h"
#include "sge_give_jobs.h"
#include "sge_m_event.h"
#include "read_write_queue.h"
#include "subordinate_qmaster.h"
#include "execution_states.h"
#include "sge_feature.h"
#include "sge_rusage.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "symbols.h"
#include "category.h"
#include "setup_path.h"
#include "msg_common.h"
#include "msg_daemons_common.h"
#include "msg_qmaster.h"
#include "sge_string.h"
#include "sge_feature.h"
#include "sge_unistd.h"
#include "sge_spool.h"
#include "sge_hostname.h"

extern lList *Master_Queue_List;
extern lList *Master_Userset_List;

/************************************************************************
 Master routine for job exit

 We need a rusage struct filled.
 In normal cases this is done by the execd, sending this structure
 to notify master about job finish.

 In case of an error noticed by the master which needs the job to be 
 removed we can fill this structure by hand. We need:

 rusage->job_number
 rusage->qname to clean up the queue (if we didn't find it we nevertheless
               clean up the job

 for functions regarding rusage see sge_rusage.c
 ************************************************************************/
void sge_job_exit(
lListElem *jr,
lListElem *jep,
lListElem *jatep 
) {
   lListElem *qep, *queueep;
   const char *err_str;
   const char *qname; 
   const char *host;
   u_long32 jobid, state, jataskid;
   lListElem *hep;
   int enhanced_product_mode;

   u_long32 failed, general_failure;

   DENTER(TOP_LAYER, "sge_job_exit");
   
   enhanced_product_mode = feature_is_enabled(FEATURE_SGEEE); 
                           
   qname = lGetString(jr, JR_queue_name);
   if (!qname)
      qname = (char *)MSG_OBJ_UNKNOWNQ;
   err_str = lGetString(jr, JR_err_str);
   if (!err_str)
      err_str = MSG_OBJ_UNKNOWNREASON;

   jobid = lGetUlong(jr, JR_job_number);
   jataskid = lGetUlong(jr, JR_ja_task_number);
   failed = lGetUlong(jr, JR_failed);
   general_failure = lGetUlong(jr, JR_general_failure);

   cancel_job_resend(jobid, jataskid);

   /* This only has a meaning for Hibernator jobs. The job pid must
    * be saved accross restarts, since jobs get there old pid
    */
   lSetUlong(jatep, JAT_pvm_ckpt_pid, lGetUlong(jr, JR_job_pid));

   DPRINTF(("reaping job "u32"."u32" in queue >%s< job_pid %d\n", 
      jobid, jataskid, qname, (int) lGetUlong(jatep, JAT_pvm_ckpt_pid)));

   if (!(queueep = lGetElemStr(Master_Queue_List, QU_qname, qname))) {
      ERROR((SGE_EVENT, MSG_JOB_WRITEJFINISH_S, qname));
   }

   if (failed) {        /* a problem occured */
      WARNING((SGE_EVENT, MSG_JOB_FAILEDONHOST_UUSSSS, u32c(jobid), 
               u32c(jataskid), 
               queueep ? lGetHost(queueep, QU_qhostname) : MSG_OBJ_UNKNOWNHOST,
               general_failure ? MSG_GENERAL : "",
               get_sstate_description(failed), err_str));
   }
   else
      INFO((SGE_EVENT, MSG_JOB_JFINISH_UUS,  u32c(jobid), u32c(jataskid), 
            queueep ? lGetHost(queueep, QU_qhostname) : MSG_OBJ_UNKNOWNHOST));


   /*-------------------------------------------------*/

   /* test if this job is in state JRUNNING or JTRANSFERING */
   if (lGetUlong(jatep, JAT_status) != JRUNNING && 
       lGetUlong(jatep, JAT_status) != JTRANSFERING) {
      ERROR((SGE_EVENT, MSG_JOB_JEXITNOTRUN_UU, u32c(lGetUlong(jep, JB_job_number)), u32c(jataskid)));
      return;
   }

   /*
    * case 1: job being trashed because 
    *    --> failed starting interactive job
    *    --> job was deleted
    */
   if (((lGetUlong(jatep, JAT_state) & JDELETED) == JDELETED) ||
         (failed && !lGetString(jep, JB_exec_file))) {
      job_log(jobid, jataskid, MSG_LOG_JREMOVED);
      sge_log_dusage(jr, jep, jatep);

      sge_commit_job(jep, jatep, (enhanced_product_mode ? 4 : 3), COMMIT_DEFAULT);
   } 
     /*
      * case 2: set job in error state
      *    --> owner requested wrong 
      *        -e/-o/-S/-cwd
      *    --> user did not exist at the execution machine
      */
   else if ((failed && general_failure==GFSTATE_JOB)) {
      job_log(jobid, jataskid, MSG_LOG_JERRORSET);
      DPRINTF(("set job "u32"."u32" in ERROR state\n", lGetUlong(jep, JB_job_number),
         jataskid));
      sge_log_dusage(jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, 8, COMMIT_DEFAULT);
   }
      /*
       * case 3: job being rescheduled because it wasnt even started
       *                            or because it was a general error
       */
   else if (((failed && (failed <= SSTATE_BEFORE_JOB)) || 
        general_failure)) {
      DTRACE;
      job_log(jobid, jataskid, MSG_LOG_JNOSTARTRESCHEDULE);
      sge_commit_job(jep, jatep, 2, COMMIT_DEFAULT);
      sge_log_dusage(jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
   }
      /*
       * case 4: job being rescheduled because rerun specified or ckpt job
       */
   else if (((failed == ESSTATE_NO_EXITSTATUS) || 
              failed == ESSTATE_DIED_THRU_SIGNAL) &&
            ((lGetUlong(jep, JB_restart) == 1 || 
             (lGetUlong(jep, JB_checkpoint_attr) & ~NO_CHECKPOINT)) ||
             (!lGetUlong(jep, JB_restart) && lGetUlong(queueep, QU_rerun)))) {
      DTRACE;
      job_log(jobid, jataskid, MSG_LOG_JRERUNRESCHEDULE);
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      sge_log_dusage(jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, 2, COMMIT_DEFAULT);
   }
   else if (failed == SSTATE_MIGRATE) {
      DTRACE;
      job_log(jobid, jataskid, MSG_LOG_JCKPTRESCHEDULE);
      /* job_restarted == 2 means a checkpoint in the ckpt arena */
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      sge_log_dusage(jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, 2, COMMIT_DEFAULT);
   }
   else if (failed == SSTATE_AGAIN) {
      job_log(jobid, jataskid, MSG_LOG_JNORESRESCHEDULE);
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      sge_log_dusage(jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, 2, COMMIT_DEFAULT);
   }
   else {
      job_log(jobid, jataskid, MSG_LOG_EXITED);
      sge_log_dusage(jr, jep, jatep);
      sge_commit_job(jep, jatep, (enhanced_product_mode ? 4 : 3), COMMIT_DEFAULT);
   }

   if (queueep) {
      /*
      ** to be sure this queue is halted even if the host 
      ** is not found in the next statement
      */
      if (general_failure && general_failure!=GFSTATE_JOB) {  
         /* general error -> this queue cant run any job */
         state = lGetUlong(queueep, QU_state);
         SETBIT(QERROR, state);
         lSetUlong(queueep, QU_state, state);
         cull_write_qconf(1, 0, QUEUE_DIR, lGetString(queueep, QU_qname), NULL, queueep);
         ERROR((SGE_EVENT, MSG_LOG_QERRORBYJOB_SU, lGetString(queueep, QU_qname), u32c(jobid)));    
      }
      /*
      ** in this case we have to halt all queues on this host
      */
      if (general_failure == GFSTATE_HOST) {
         hep = sge_locate_host(lGetHost(queueep, QU_qhostname), SGE_EXECHOST_LIST);
         if (hep) {
            host = lGetHost(hep, EH_name);
            for_each(qep, Master_Queue_List) {
               if (!sge_hostcmp(lGetHost(qep, QU_qhostname), host)) {
                  state = lGetUlong(qep, QU_state);
                  CLEARBIT(QRUNNING,state);
                  SETBIT(QERROR, state);
                  lSetUlong(qep, QU_state, state);
/*                   DPRINTF(("queue %s marked QERROR\n", lGetString(qep, QU_qname))); */
                  ERROR((SGE_EVENT, MSG_LOG_QERRORBYJOBHOST_SUS, lGetString(qep, QU_qname), u32c(jobid), host));    
                  
                  cull_write_qconf(1, 0, QUEUE_DIR, 
                                    lGetString(qep, QU_qname), NULL, qep);
                  sge_add_queue_event(sgeE_QUEUE_MOD, qep);
               }
            }
         }
      }
      sge_add_queue_event(sgeE_QUEUE_MOD, queueep);
   }

   DEXIT;
   return;
}

/************************************************************

 ************************************************************/
void sge_log_dusage(
lListElem *jr,
lListElem *jep,
lListElem *jatep 
) {

   FILE *fp;
   int write_result;
   const char *category_str;
   SGE_STRUCT_STAT statbuf;
   int write_comment;

   DENTER(TOP_LAYER, "sge_log_dusage");

   write_comment = 0;
   if (SGE_STAT(path.acct_file, &statbuf)) {
      write_comment = 1;
   }     

   fp = fopen(path.acct_file, "a");
   if (!fp) {
      ERROR((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, path.acct_file, strerror(errno)));
      DEXIT;
      return;
   }

   if (write_comment && (sge_spoolmsg_write(fp, COMMENT_CHAR, 
         feature_get_product_name(FS_VERSION)) < 0)) {
      ERROR((SGE_EVENT, MSG_FILE_WRITE_S, path.acct_file)); 
      fclose(fp);
      DEXIT;
      return;
   }   

   category_str = sge_build_job_category(jep, Master_Userset_List);
   write_result = sge_write_rusage(fp, jr, jep, jatep, category_str);
   if (write_result == EOF) {
      ERROR((SGE_EVENT, MSG_FILE_WRITE_S, path.acct_file));
      fclose(fp);
      DEXIT;
      return;
   }
   if (category_str)
      free((char *)category_str); 
   else if (write_result == -2) {
      /* The file should be open... */
      ERROR((SGE_EVENT, MSG_FILE_WRITEACCT));
      fclose(fp);
      DEXIT;
      return;
   }

   fclose(fp);

   DEXIT;
   return;
}

