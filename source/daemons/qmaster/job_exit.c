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

#include "sge.h"
#include "sge_ja_task.h"
#include "sge_job_refL.h"
#include "sge_job_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_host.h"
#include "job_log.h"
#include "job_exit.h"
#include "sge_give_jobs.h"
#include "sge_event_master.h"
#include "sge_queue_event_master.h"
#include "sge_cqueue_qmaster.h"
#include "sge_subordinate_qmaster.h"
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
#include "sge_unistd.h"
#include "sge_time.h"
#include "sge_spool.h"
#include "sge_hostname.h"
#include "sgeobj/sge_qinstance.h"
#include "sgeobj/sge_qinstance_state.h"
#include "sgeobj/sge_qinstance_message.h"
#include "sge_job.h"
#include "sge_report.h"
#include "sge_report_execd.h"
#include "sge_userset.h"
#include "sge_todo.h"
#include "sge_cqueue.h"

#include "sge_reporting_qmaster.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

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
   lListElem *queueep;
   const char *err_str;
   const char *qname; 
   const char *hostname = MSG_OBJ_UNKNOWNHOST;
   u_long32 jobid, jataskid;
   lListElem *hep;
   int enhanced_product_mode;
   u_long32 timestamp;

   u_long32 failed, general_failure;

   DENTER(TOP_LAYER, "sge_job_exit");
   
   enhanced_product_mode = feature_is_enabled(FEATURE_SGEEE); 

   /* JG: TODO: we'd prefer some more precise timestamp, e.g. from jr */
   timestamp = sge_get_gmt();
                     
   qname = lGetString(jr, JR_queue_name);
   if (!qname)
      qname = (char *)MSG_OBJ_UNKNOWNQ;
   err_str = lGetString(jr, JR_err_str);
   if (!err_str)
      err_str = MSG_UNKNOWNREASON;

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

   if (!(queueep = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), qname))) {
      ERROR((SGE_EVENT, MSG_JOB_WRITEJFINISH_S, qname));
   }

   /* retrieve hostname for later use */
   if (queueep != NULL) {
      hostname = lGetHost(queueep, QU_qhostname);
   }

   if (failed) {        /* a problem occured */
      WARNING((SGE_EVENT, MSG_JOB_FAILEDONHOST_UUSSSS, u32c(jobid), 
               u32c(jataskid), 
               hostname,
               general_failure ? MSG_GENERAL : "",
               get_sstate_description(failed), err_str));
   }
   else
      INFO((SGE_EVENT, MSG_JOB_JFINISH_UUS,  u32c(jobid), u32c(jataskid), 
            hostname));


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
    *    --> a failed batch job that explicitely shall not enter error state
    */
   if (((lGetUlong(jatep, JAT_state) & JDELETED) == JDELETED) ||
         (failed && !lGetString(jep, JB_exec_file)) ||
         (failed && general_failure==GFSTATE_JOB && JOB_TYPE_IS_NO_ERROR(lGetUlong(jep, JB_type)))) {
/*       job_log(jobid, jataskid, MSG_LOG_JREMOVED); */
      reporting_create_acct_record(NULL, jr, jep, jatep);
      /* JG: TODO: we need more information in the log message */
      reporting_create_job_log(NULL, timestamp, JL_DELETED, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JREMOVED);

      sge_commit_job(jep, jatep, jr, (enhanced_product_mode ? COMMIT_ST_FINISHED_FAILED_EE : COMMIT_ST_FINISHED_FAILED), COMMIT_DEFAULT |
      COMMIT_NEVER_RAN);
   } 
     /*
      * case 2: set job in error state
      *    --> owner requested wrong 
      *        -e/-o/-S/-cwd
      *    --> user did not exist at the execution machine
      */
   else if ((failed && general_failure==GFSTATE_JOB)) {
/*       job_log(jobid, jataskid, MSG_LOG_JERRORSET); */
      DPRINTF(("set job "u32"."u32" in ERROR state\n", lGetUlong(jep, JB_job_number),
         jataskid));
      reporting_create_acct_record(NULL, jr, jep, jatep);
      /* JG: TODO: we need more information in the log message */
      reporting_create_job_log(NULL, timestamp, JL_ERROR, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JERRORSET);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, jr, COMMIT_ST_FAILED_AND_ERROR, COMMIT_DEFAULT);
   }
      /*
       * case 3: job being rescheduled because it wasnt even started
       *                            or because it was a general error
       */
   else if (((failed && (failed <= SSTATE_BEFORE_JOB)) || 
        general_failure)) {
      DTRACE;
/*       job_log(jobid, jataskid, MSG_LOG_JNOSTARTRESCHEDULE); */
      /* JG: TODO: we need more information in the log message */
      reporting_create_job_log(NULL, timestamp, JL_RESTART, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JNOSTARTRESCHEDULE);
      sge_commit_job(jep, jatep, jr, COMMIT_ST_RESCHEDULED, COMMIT_DEFAULT);
      reporting_create_acct_record(NULL, jr, jep, jatep);
      lSetUlong(jatep, JAT_start_time, 0);
   }
      /*
       * case 4: job being rescheduled because rerun specified or ckpt job
       */
   else if (((failed == ESSTATE_NO_EXITSTATUS) || 
              failed == ESSTATE_DIED_THRU_SIGNAL) &&
            ((lGetUlong(jep, JB_restart) == 1 || 
             (lGetUlong(jep, JB_checkpoint_attr) & ~NO_CHECKPOINT)) ||
             (!lGetUlong(jep, JB_restart) && lGetBool(queueep, QU_rerun)))) {
      DTRACE;
/*       job_log(jobid, jataskid, MSG_LOG_JRERUNRESCHEDULE); */
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      reporting_create_acct_record(NULL, jr, jep, jatep);
      /* JG: TODO: we need more information in the log message */
      reporting_create_job_log(NULL, timestamp, JL_RESTART, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JRERUNRESCHEDULE);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, jr, COMMIT_ST_RESCHEDULED, COMMIT_DEFAULT);
   }
      /*
       * case 5: job being rescheduled because it was interrupted and a checkpoint exists
       */
   else if (failed == SSTATE_MIGRATE) {
      DTRACE;
/*       job_log(jobid, jataskid, MSG_LOG_JCKPTRESCHEDULE); */
      /* job_restarted == 2 means a checkpoint in the ckpt arena */
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      reporting_create_acct_record(NULL, jr, jep, jatep);
      reporting_create_job_log(NULL, timestamp, JL_MIGRATE, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JCKPTRESCHEDULE);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, jr, COMMIT_ST_RESCHEDULED, COMMIT_DEFAULT);
   }
      /*
       * case 6: job being rescheduled because of exit 99 
       *                            or because of a rerun e.g. triggered by qmod -r <jobid>
       */
   else if (failed == SSTATE_AGAIN) {
/*       job_log(jobid, jataskid, MSG_LOG_JNORESRESCHEDULE); */
      lSetUlong(jatep, JAT_job_restarted, 
                  MAX(lGetUlong(jatep, JAT_job_restarted), 
                      lGetUlong(jr, JR_ckpt_arena)));
      lSetString(jatep, JAT_osjobid, lGetString(jr, JR_osjobid));
      reporting_create_acct_record(NULL, jr, jep, jatep);
      reporting_create_job_log(NULL, timestamp, JL_RESTART, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_JNORESRESCHEDULE);
      lSetUlong(jatep, JAT_start_time, 0);
      sge_commit_job(jep, jatep, jr, COMMIT_ST_RESCHEDULED, COMMIT_DEFAULT);
   }
      /*
       * case 7: job finished 
       */
   else {
/*       job_log(jobid, jataskid, MSG_LOG_EXITED); */
      reporting_create_acct_record(NULL, jr, jep, jatep);
      reporting_create_job_log(NULL, timestamp, JL_FINISHED, MSG_EXECD, hostname, jr, jep, jatep, NULL, MSG_LOG_EXITED);
      sge_commit_job(jep, jatep, jr, (enhanced_product_mode ? COMMIT_ST_FINISHED_FAILED_EE : COMMIT_ST_FINISHED_FAILED), COMMIT_DEFAULT);
   }

   if (queueep) {
      bool spool_queueep = false;
      lList *answer_list = NULL;
      /*
      ** to be sure this queue is halted even if the host 
      ** is not found in the next statement
      */
      if (general_failure && general_failure!=GFSTATE_JOB) {  
         dstring error = DSTRING_INIT; 

         sge_dstring_sprintf(&error, MSG_LOG_QERRORBYJOB_SU, lGetString(queueep, QU_qname), u32c(jobid));
         
         /* general error -> this queue cant run any job */
         qinstance_state_set_error(queueep, true);
         reporting_create_queue_record(NULL, queueep, timestamp);
         qinstance_message_add(queueep, QI_ERROR, sge_dstring_get_string(&error));
         spool_queueep = true;
         /*ERROR((SGE_EVENT, MSG_LOG_QERRORBYJOB_SU, 
                lGetString(queueep, QU_qname), u32c(jobid)));  */
         ERROR((SGE_EVENT, sge_dstring_get_string(&error)));      
         sge_dstring_free(&error);
      }
      /*
      ** in this case we have to halt all queues on this host
      */
      if (general_failure == GFSTATE_HOST) {
         spool_queueep = true;
         hep = host_list_locate(Master_Exechost_List, 
                                lGetHost(queueep, QU_qhostname));
         if (hep != NULL) {
            lListElem *cqueue = NULL;
            const char *host = lGetHost(hep, EH_name);
            dstring error = DSTRING_INIT;
         
            for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
               lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
               lListElem *qinstance = NULL;
               lListElem *next_qinstance = NULL;
               const void *iterator = NULL;

               next_qinstance = lGetElemHostFirst(qinstance_list, QU_qhostname, 
                                                  host, &iterator);
               while((qinstance = next_qinstance) != NULL) {
                  
                  next_qinstance = lGetElemHostNext(qinstance_list,
                                                    QU_qhostname,
                                                    host, 
                                                    &iterator);
                  qinstance_state_set_error(qinstance, true);
                  reporting_create_queue_record(NULL, qinstance, timestamp);

                  sge_dstring_sprintf(&error, MSG_LOG_QERRORBYJOBHOST_SUS, lGetString(qinstance, QU_qname), u32c(jobid), host);
                  qinstance_message_add(queueep, QI_ERROR, sge_dstring_get_string(&error)); 
                  ERROR((SGE_EVENT, sge_dstring_get_string(&error)));
                  if (qinstance != queueep) {
                     sge_event_spool(&answer_list, 0, sgeE_QINSTANCE_MOD, 
                                     0, 0, lGetString(qinstance, QU_qname), 
                                     lGetHost(qinstance, QU_qhostname), NULL,
                                     qinstance, NULL, NULL, true, true);
                  }
               }
            }
            sge_dstring_free(&error);
         }
      }

      sge_event_spool(&answer_list, 0, sgeE_QINSTANCE_MOD, 
                      0, 0, lGetString(queueep, QU_qname), 
                      lGetHost(queueep, QU_qhostname), NULL,
                      queueep, NULL, NULL, true, spool_queueep);

      cqueue_list_del_all_orphaned(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), &answer_list);

      answer_list_output(&answer_list);
   }

   DEXIT;
   return;
}

