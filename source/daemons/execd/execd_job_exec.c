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
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "sgermon.h"
#include "sge.h"
#include "sge_pe.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "dispatcher.h"
#include "sge_string.h"
#include "sge_parse_num_par.h"
#include "reaper_execd.h"
#include "job_report_execd.h"
#include "execd_job_exec.h"
#include "spool/classic/read_write_job.h"
#include "sge_feature.h"
#include "sge_conf.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_io.h"
#include "execution_states.h"
#include "sge_afsutil.h"
#include "setup_path.h"
#include "sge_security.h"
#include "sge_job.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_var.h"
#include "sge_qinstance.h"
#include "get_path.h"
#include "sgeobj/sge_object.h"
#include "sge_bootstrap.h"
#include "sge_answer.h"

#include "msg_common.h"
#include "msg_execd.h"
#include "msg_gdilib.h"
#if defined(INTERIX)
   #include "../../libs/wingrid/misc.h"
#endif


extern volatile int jobs_to_start;

static int handle_job(sge_gdi_ctx_class_t *ctx,
                      lListElem *jelem, 
                      lListElem *jatep, 
                      struct dispatch_entry *de, 
                      sge_pack_buffer *pb, 
                      int slave);
static int handle_task(sge_gdi_ctx_class_t *ctx,
                       lListElem *petrep, 
                       struct dispatch_entry *de, 
                       sge_pack_buffer *pb, 
                       sge_pack_buffer *apb, 
                       int *synchron);

/*************************************************************************
EXECD function called by dispatcher

get job from qmaster and store it in execd local structures and files
real execution is done by the cyclic execd_ck_to_do()

 jobs_to_start will be incremented

 qmaster counterpart is sge_give_jobs()


 Files generated here:
   job_scripts/<jid>   for the script 
   jobs/<jid>          for the job structure

 *************************************************************************/
int execd_job_exec(sge_gdi_ctx_class_t *ctx, 
                   struct dispatch_entry *de,
                   sge_pack_buffer *pb, 
                   sge_pack_buffer *apb,
                   u_long *rcvtimeout,
                   int *synchron,
                   char *err_str,
                   int answer_error)
{
   int ret = 1;
   u_long32 feature_set;
   const char *admin_user = ctx->get_admin_user(ctx);
   const char *progname = ctx->get_progname(ctx);

   DENTER(TOP_LAYER, "execd_job_exec");

   /* ------- featureset */
   if (unpackint(pb, &feature_set)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKFEATURESET));
      DRETURN(0);
   }


   /* if request comes from qmaster: start a job
    * else it is a request to start a pe task
    */
   if (strcmp(de->commproc, prognames[QMASTER]) == 0) {
      lListElem *job, *ja_task;
      lList *answer_list = NULL;

      if (!sge_security_verify_unique_identifier(true, admin_user, progname, 0,
                                            de->host, de->commproc, de->id)) {
         DRETURN(0);
      }
       
      if (cull_unpack_elem(pb, &job, NULL)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKJOB));
         DRETURN(0);
      }

      if (!job_verify_execd_job(job, &answer_list)) {
         lFreeElem(&job);
         answer_list_output(&answer_list);
         ERROR((SGE_EVENT, MSG_EXECD_INVALIDJOBREQUEST_SS, de->commproc, de->host));
         DRETURN(0);
      }

      #if defined(INTERIX) 
         if (strcmp(lGetString(job, JB_owner), "root") == 0) {
            char buffer[1000];
            wl_get_superuser_name(buffer, 1000);
            lSetString(job, JB_owner, buffer);            
         }
      #endif
      
      /* we expect one jatask to start per request */
      ja_task = lFirst(lGetList(job, JB_ja_tasks));
      if (ja_task != NULL) {
         DPRINTF(("new job %ld.%ld\n", 
            (long) lGetUlong(job, JB_job_number),
            (long) lGetUlong(ja_task, JAT_task_number)));
         ret = handle_job(ctx, job, ja_task, de, pb, 0);
         if (ret != 0) {
            lFreeElem(&job);
         }
      }
   } else {
      lListElem *petrep;
      lList *answer_list = NULL;

      if (cull_unpack_elem(pb, &petrep, NULL)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKJOB));
         DRETURN(0);
      }

      if (!pe_task_verify_request(petrep, &answer_list)) {
         answer_list_output(&answer_list);
         ERROR((SGE_EVENT, MSG_EXECD_INVALIDTASKREQUEST_SS, de->commproc, de->host));
         DRETURN(0);
      }

      DPRINTF(("new pe task for job: %ld.%ld\n", 
            (long) lGetUlong(petrep, PETR_jobid), 
            (long) lGetUlong(petrep, PETR_jataskid)));

      ret = handle_task(ctx, petrep, de, pb, apb, synchron);

      lFreeElem(&petrep);
   }
   
   if (ret == 0) {
      jobs_to_start = 1;
   }

   DRETURN(0);
}

int execd_job_slave(sge_gdi_ctx_class_t *ctx,
                    struct dispatch_entry *de,
                    sge_pack_buffer *pb, 
                    sge_pack_buffer *apb,
                    u_long *rcvtimeouts,
                    int *synchron,
                    char *err_str,
                    int answer_error)
{
   int ret = 1;
   lListElem *jelem, *ja_task;
   u_long32 feature_set;

   DENTER(TOP_LAYER, "execd_job_slave");

   /* ------- featureset */
   if (unpackint(pb, &feature_set)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKFEATURESET));
      DRETURN(0);
   }

   /*
   ** the check if the request has admin/root credentials is done by
   ** the dispatcher in authorize_dpe()
   ** so no additional check needed here like in execd_job_exec()
   */

   /* ------- job */
   if (cull_unpack_elem(pb, &jelem, NULL)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKJOB));
      DRETURN(0);
   }

   for_each(ja_task, lGetList(jelem, JB_ja_tasks)) {
      DPRINTF(("Job: %ld Task: %ld\n", (long) lGetUlong(jelem, JB_job_number),
         (long) lGetUlong(ja_task, JAT_task_number)));
      ret = handle_job(ctx, jelem, ja_task, de, pb, 1);
   }

   if (ret)  {
      lFreeElem(&jelem);
   } 

   DRETURN(0);
}

static int handle_job(
sge_gdi_ctx_class_t *ctx,
lListElem *jelem,
lListElem *jatep,
struct dispatch_entry *de,
sge_pack_buffer *pb, 
int slave 
) {
   lListElem *jep, *pelem = NULL, *jep_jatep = NULL;
   dstring err_str = DSTRING_INIT;
   u_long32 jobid, jataskid;
   int mail_on_error = 0, general = GFSTATE_QUEUE;
   lListElem *qep, *gdil_ep;
   lList *qlp = NULL;
   const char *qnm;
   int slots;
   int fd;
   const void *iterator = NULL;
   bool report_job_error = true;   /* send job report on error? */
   const char *sge_root = ctx->get_sge_root(ctx);
   const char *unqualified_hostname = ctx->get_unqualified_hostname(ctx);

   DENTER(TOP_LAYER, "handle_job");

   DPRINTF(("got %s job "sge_u32" from (%s/%s/%d)\n",
           slave ?"slave ":"", lGetUlong(jelem, JB_job_number),
           de->commproc, de->host, de->id));

   jobid = lGetUlong(jelem, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);

   /* 
    * Sometimes the rescheduled job from qmaster which is 
    * sent synchronuously arrives earlier than the ack 
    * reporting that qmaster got job exit message from run before 
    * 
    * We can ignore this job because job is resend by qmaster.
    */
   jep = lGetElemUlongFirst(*(object_type_get_master_list(SGE_TYPE_JOB)), JB_job_number, jobid, &iterator);
   while (jep != NULL) {
      jep_jatep = job_search_task(jep, NULL, jataskid);
      if(jep_jatep != NULL) {
         DPRINTF(("Job "sge_u32"."sge_u32" is already running - skip the new one\n", 
                  jobid, jataskid));
         goto Ignore;   /* don't set queue in error state */
      }

      jep = lGetElemUlongNext(*(object_type_get_master_list(SGE_TYPE_JOB)), JB_job_number, jobid, &iterator);
   }

   /* initialize state - prevent slaves from getting started */
   lSetUlong(jatep, JAT_status, slave?JSLAVE:JIDLE);

   /* now we have a queue and a job filled */
   DPRINTF(("===>JOB_EXECUTION: >"sge_u32"."sge_u32"< with "sge_u32" tickets\n", jobid, jataskid,
               (u_long32)lGetDouble(jatep, JAT_tix)));

   if (cull_unpack_list(pb, &qlp)) {
      sge_dstring_sprintf(&err_str, MSG_COM_UNPACKINGQ);
      goto Error;
   }

   /* 
    * Verify the queue list sent with the job start order.
    * If it is incorrect, we reject the job start.
    * We do not send a job report for this job - this would trigger
    * rescheduling in qmaster ...
    *
    * TODO: A better solution to stepwise unpacking and verification of the job order
    * in this function would be to do all unpacking and verification in the
    * calling function (execd_job_exec)!
    *
    * TODO: Why do we send the queue list (and also the pe) as a separate list, and then
    * move it into the granted destination identifier list here in execd?
    * Couldn't qmaster just put the queues in to the JG_queue field instead?
    */
   {
      lList *answer_list = NULL;
      if (!qinstance_list_verify_execd_job(qlp, &answer_list)) {
         sge_dstring_sprintf(&err_str, MSG_EXECD_INVALIDJOBREQUEST_SS, de->commproc, de->host);
         answer_list_output(&answer_list);
         report_job_error = false;
         goto Error;
      }
      lFreeList(&answer_list);
   }

   /* initialize job */
   /* store queues as sub elems of gdil */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      qnm=lGetString(gdil_ep, JG_qname);
      if (!(qep=qinstance_list_locate2(qlp, qnm))) {
         sge_dstring_sprintf(&err_str, MSG_JOB_MISSINGQINGDIL_SU, qnm, 
                             sge_u32c(lGetUlong(jelem, JB_job_number)));
         goto Error;
      }

      qep = lDechainElem(qlp, qep);
      /* clear any queue state that might be set from qmaster */
      lSetUlong(qep, QU_state, 0);

      /* store number of slots we got in this queue for this job */
      slots = lGetUlong(gdil_ep, JG_slots);
      lSetUlong(qep, QU_job_slots, slots);
      qinstance_set_slots_used(qep, 0);
      lSetObject(gdil_ep, JG_queue, qep);
      DPRINTF(("Q: %s %d\n", qnm, slots));
   }
   /* trash envelope */
   lFreeList(&qlp);

   /* ------- optionally pe */
   if (lGetString(jatep, JAT_granted_pe)) {
      cull_unpack_elem(pb, &pelem, NULL); /* pelem will be freed with jelem */
      lSetObject(jatep, JAT_pe_object, pelem);

      if (lGetBool(pelem, PE_control_slaves)) {
         if (!lGetBool(pelem, PE_job_is_first_task)) {
            int slots;
            lListElem *mq = lGetObject(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), 
                                       JG_queue);
            slots = lGetUlong(mq, QU_job_slots) + 1;
            DPRINTF(("Increasing job slots in master queue \"%s\" "
               "to %d because job is not first task\n",
                  lGetString(mq, QU_qname), slots));
            lSetUlong(mq, QU_job_slots, slots);

         }
      }
   }

   if (sge_make_ja_task_active_dir(jelem, jatep, &err_str) == NULL) {
      goto Error;
   }

   if (!JOB_TYPE_IS_BINARY(lGetUlong(jelem, JB_type))) {
      /* interactive jobs and slave jobs do not have a script file */
      if (!slave && lGetString(jelem, JB_script_file)) {
         int nwritten;
         int found_script = 0;
         const void *iterator = NULL;
         lListElem *tmp_job = NULL;
         lListElem *next_tmp_job = NULL;
         u_long32 job_id = lGetUlong(jelem, JB_job_number);
         lList *gdi_list = NULL;
         lList *ja_task_list = NULL;   
         const char *hostname = NULL;
         const char *tmp_hostname = NULL;

         gdi_list = lGetList(jatep, JAT_granted_destin_identifier_list);
         hostname = lGetHost(lFirst(gdi_list), JG_qhostname);
         
         /*
          * Is another array task of the same job already here?
          * In this case it is not necessary to spool the jobscript.
          *
          * But it is not enough, just to look for another array task. we have
          * check wether there is another master task of the same job running
          * on this host. This is important in case of array pe-jobs.
          */
         next_tmp_job = lGetElemUlongFirst(*(object_type_get_master_list(SGE_TYPE_JOB)),
                                           JB_job_number, job_id, &iterator);
         while((tmp_job = next_tmp_job) != NULL) {
            next_tmp_job = lGetElemUlongNext(*(object_type_get_master_list(SGE_TYPE_JOB)),
                                           JB_job_number, job_id, &iterator);
           
            ja_task_list = lGetList(tmp_job,JB_ja_tasks);
            gdi_list = lGetList(lFirst(ja_task_list), JAT_granted_destin_identifier_list);
            tmp_hostname = lGetHost(lFirst(gdi_list), JG_qhostname);
           
            if (sge_hostcmp(hostname, tmp_hostname) == 0) {
               found_script = 1;
               break;
            }
         }

         if (!found_script) {
            /* We are root. Make the scriptfile readable for the jobs submitter,
               so shepherd can open (execute) it after changing to the user. */
            fd = SGE_OPEN3(lGetString(jelem, JB_exec_file), O_CREAT | O_WRONLY, 0755);
            if (fd < 0) {
               sge_dstring_sprintf(&err_str, MSG_ERRORWRITINGFILE_SS, 
                                   lGetString(jelem, JB_exec_file), 
                                   strerror(errno));
               goto Error;
            }

            if ((nwritten = sge_writenbytes(fd, lGetString(jelem, JB_script_ptr), 
               lGetUlong(jelem, JB_script_size))) !=
               lGetUlong(jelem, JB_script_size)) {
               DPRINTF(("errno: %d\n", errno));
               sge_dstring_sprintf(&err_str, MSG_EXECD_NOWRITESCRIPT_SIUS, 
                                   lGetString(jelem, JB_exec_file), nwritten, 
                                   sge_u32c(lGetUlong(jelem, JB_script_size)), 
                                   strerror(errno));
               close(fd);
               goto Error;
            }      
            close(fd);
            lSetString(jelem, JB_script_ptr, NULL);
         }
      }
   }
      
   /* 
   ** security hook
   **
   ** Execute command to store the client's DCE or Kerberos credentials.
   ** This also creates a forwardable credential for the user.
   */
   if (mconf_get_do_credentials()) {
      if (store_sec_cred2(sge_root, unqualified_hostname, jelem, mconf_get_do_authentication(), &general, SGE_EVENT) != 0) {
         sge_dstring_copy_string(&err_str, SGE_EVENT);
         goto Error;
      }   
   }

#ifdef KERBEROS
   kerb_job(jelem, de);
#endif



   lSetUlong(jelem, JB_script_size, 0);
   if (job_write_spool_file(jelem, jataskid, NULL, SPOOL_WITHIN_EXECD)) {
      /* SGE_EVENT is written by job_write_spool_file() */
      sge_dstring_copy_string(&err_str, SGE_EVENT);
      goto Error;
   }

   { 
      lListElem *report = NULL;
      report = add_job_report(jobid, jataskid, NULL, jelem);
      flush_job_report(report);
   }   

   if (!jep_jatep) {
      /* put into job list */
      lAppendElem(*(object_type_get_master_list(SGE_TYPE_JOB)), jelem);
   }

   DRETURN(0);

Error:
   if (report_job_error) {
      lListElem *jr;
      jr = execd_job_start_failure(jelem, jatep, NULL, sge_dstring_get_string(&err_str), general);
      
      if (mail_on_error) {
         reaper_sendmail(ctx, jelem, jr);
      }
   }

   sge_dstring_free(&err_str);

Ignore:   
   lFreeList(&qlp);
   DRETURN(-1);  
}

/****** execd/job/job_set_queue_info_in_task() ********************************
*  NAME
*     job_set_queue_info_in_task() -- set queue to use for task
*
*  SYNOPSIS
*     static lList *job_set_queue_info_in_task(char *qname, lListElem *pe_task);
*
*  FUNCTION
*     Extend the task structure of task <jatask> by a 
*     JAT_granted_destin_identifier list, which contains 
*     the queue <qname> and uses one slot.
*
*  INPUTS
*     qualified_hostname - name of host
*     qname   - name of queue to set
*     pe_task - task structure
*
*  RESULT
*     the new created JAT_granted_destin_identifier list
******************************************************************************/
static lList *job_set_queue_info_in_task(const char *qualified_hostname, const char *qname, lListElem *petep)
{
   lListElem *jge;

   DENTER(TOP_LAYER, "job_set_queue_info_in_task");

   jge = lAddSubStr(petep, JG_qname, qname, 
                    PET_granted_destin_identifier_list, JG_Type);
   lSetHost(jge, JG_qhostname, qualified_hostname);
   lSetUlong(jge, JG_slots, 1);
   DPRINTF(("selected queue %s for task\n", qname));

   DRETURN(lGetList(petep, PET_granted_destin_identifier_list));
}

/****** execd/job/job_get_queue_with_task_about_to_exit() *********************
*  NAME
*     job_get_queue_with_task_about_to_exit -- find Q with already exited task
*
*  SYNOPSIS
*     static lList *job_get_queue_with_task_about_to_exit(lListElem *jatep, 
*                                                         lListElem *jatask,
*                                                         u_long32 jobid,
*                                                         u_long32 jataskid);
*
*  FUNCTION
*     tries to find a pe task in the job (array task) <jatep> that has
*     already exited, but which is not yet cleaned up by the execd.
*
*     On exit of a qrsh pe task, the shepherd creates a file 
*     "shepherd_about_to_exit" in the active_jobs directory of the pe task.
*     This function tries to find this file, if it exists, the slot of 
*     the exited task can be reused before being freed by the execd.
*
*     The check is done for all running tasks of the job, if one is found, 
*     the corresponding queue is set to be used by the new task.
*
*  INPUTS
*     jatep    - the actual job (substructure job array task)
*     jatask   - the new pe task
*     jobid    - the jobid of the job
*     jataskid - the task id of the job array task
*
*  RESULT
*     on success, the JAT_granted_destin_identifier list of the new pe task
*     else NULL
*
*  SEE ALSO
*     execd/job/job_set_queue_info_in_task()
******************************************************************************/
static lList *job_get_queue_with_task_about_to_exit(lListElem *jep,
                                                    lListElem *jatep, 
                                                    lListElem *petep,
                                                    const char *qualified_hostname,
                                                    const char *queuename)
{
   lListElem *petask;
   
   DENTER(TOP_LAYER, "job_get_queue_with_task_about_to_exit");
   
   for_each(petask, lGetList(jatep, JAT_task_list)) {
      lListElem *pe_task_queue = lFirst(lGetList(petask, PET_granted_destin_identifier_list));
      if (pe_task_queue != NULL) {
         /* if a certain queue is requested, skip non matching tasks */
         if (queuename != NULL && strcmp(queuename, lGetString(pe_task_queue, JG_qname)) != 0) {
            continue;
         } else {
            dstring shepherd_about_to_exit = DSTRING_INIT;
            SGE_STRUCT_STAT stat_buffer;
            u_long32 jobid;
            u_long32 jataskid;
            const char *petaskid = NULL;
           
            jobid = lGetUlong(jep, JB_job_number);
            jataskid = lGetUlong(jatep, JAT_task_number);
            petaskid = lGetString(petask, PET_id);
            
            sge_get_active_job_file_path(&shepherd_about_to_exit,
                                         jobid, jataskid, petaskid,
                                         "shepherd_about_to_exit");
            DPRINTF(("checking for file %s\n", sge_dstring_get_string(&shepherd_about_to_exit)));

            if (SGE_STAT(sge_dstring_get_string(&shepherd_about_to_exit), &stat_buffer) == 0) {
               lList *jat_gdil = job_set_queue_info_in_task(qualified_hostname, 
                                       lGetString(pe_task_queue, JG_qname), petep);
               DPRINTF(("task %s of job %d.%d already exited, using its slot for new task\n", 
                        petaskid, jobid, jataskid));
               sge_dstring_free(&shepherd_about_to_exit);         
               DRETURN(jat_gdil); 
            }
            sge_dstring_free(&shepherd_about_to_exit);         
         }
      }   
   }

   DRETURN(NULL);
}

/****** execd/job/job_get_queue_for_task() ************************************
*  NAME
*     job_get_queue_for_task() -- find a queue suited for task execution
*
*  SYNOPSIS
*     static lList *job_get_queue_for_task(lListElem  *jatep,
*                                          lListElem  *jatask,
*                                          const char *queuename);
*
*  FUNCTION
*     Search for a queue, that 
*        - may be used by job in which new task shall be started
*        - resides on the host of this execd
*        - has free slots for the job in which new task shall be started
*     If a suited queue is found, it is set to be used by the new task.
*
*  INPUTS
*     jatep     - the actual job (substructure job array task)
*     petep     - the new pe task
*     qualified_hostname - qualfied hostname
*     queuename - optional: request a certain queue
*
*  RESULT
*     on success, the JAT_granted_destin_identifier list of the new pe task
*     else NULL
*
*  SEE ALSO
*     execd/job/job_set_queue_info_in_task()
******************************************************************************/
static lList *
job_get_queue_for_task(lListElem *jatep, lListElem *petep, 
                       const char *qualified_hostname, const char *queuename) 
{
   lListElem *this_q, *gdil_ep;

   DENTER(TOP_LAYER, "job_get_queue_for_task");

   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      /* if a certain queuename is requested, check only this queue */
      if (queuename != NULL && 
          strcmp(queuename, lGetString(gdil_ep, JG_qname)) != 0) {
         DTRACE;
         continue;
      } 

      this_q = lGetObject(gdil_ep, JG_queue);

      DTRACE;

      /* Queue must exist and be on this host */
      if (this_q != NULL && 
                     sge_hostcmp(lGetHost(gdil_ep, JG_qhostname), 
                                 qualified_hostname) == 0) {

         DTRACE;

         /* Queue must have free slots */
         if(qinstance_slots_used(this_q) < lGetUlong(this_q, QU_job_slots)) {
            lList *jat_gdil = job_set_queue_info_in_task(qualified_hostname, lGetString(gdil_ep, JG_qname),
                                                          petep);
            DRETURN(jat_gdil); 
         } 
      }
   }
   DRETURN(NULL);
}

static int handle_task(
sge_gdi_ctx_class_t *ctx,
lListElem *petrep,
struct dispatch_entry *de,
sge_pack_buffer *pb, 
sge_pack_buffer *apb, 
int *synchron
) {
   u_long32 jobid, jataskid;
   lListElem *jep   = NULL;
   lListElem *pe    = NULL;
   lListElem *jatep = NULL;
   lListElem *petep = NULL;
   const char *requested_queue;
   char source[1024], new_task_id[1024];
   lList *gdil = NULL;
   int tid = 0;
   const void *iterator;
   dstring err_str = DSTRING_INIT;
   const char *progname = ctx->get_progname(ctx);
   const char *qualified_hostname = ctx->get_qualified_hostname(ctx);
   const char *unqualified_hostname = ctx->get_unqualified_hostname(ctx);

   DENTER(TOP_LAYER, "handle_task");

   petep = lCreateElem(PET_Type);

   /* may be we have to send a task exit message to this guy */
   sprintf(source, "%s:%s:%d", de->host, de->commproc, de->id);
   lSetString(petep, PET_source, source);

#ifdef KERBEROS
   if (krb_verify_user(de->host, de->commproc, de->id,
                       lGetString(petrep, PETR_owner)) < 0) {
      ERROR((SGE_EVENT, MSG_SEC_KRB_CRED_SSSI, lGetString(petrep, PETR_owner), de->host, de->commproc, de->id));
      goto Error;
   }
#endif /* KERBEROS */

   jobid    = lGetUlong(petrep, PETR_jobid);
   jataskid = lGetUlong(petrep, PETR_jataskid);

   jep = lGetElemUlongFirst(*(object_type_get_master_list(SGE_TYPE_JOB)),
                            JB_job_number, jobid, &iterator);
   while (jep != NULL) {
      jatep = job_search_task(jep, NULL, jataskid);
      if (jatep != NULL) {
         break;
      }

      jep = lGetElemUlongNext(*(object_type_get_master_list(SGE_TYPE_JOB)),
                              JB_job_number, jobid, &iterator);
   }
   
   if (jep == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_TASKWITHOUTJOB_U, sge_u32c(jobid))); 
      goto Error;
   }

   if (jatep == NULL) { 
      ERROR((SGE_EVENT, MSG_JOB_TASKNOTASKINJOB_UU, sge_u32c(jobid), sge_u32c(jataskid)));
      goto Error;
   }

   if (!sge_security_verify_unique_identifier(false, lGetString(jep, JB_owner), progname, 0,
                                              de->host, de->commproc, de->id)) {
      goto Error;
   }

   /* do not accept the task if job is not parallel or 'control_slaves' is not active */
   if (!(pe=lGetObject(jatep, JAT_pe_object)) || !lGetBool(pe, PE_control_slaves)) {
      ERROR((SGE_EVENT, MSG_JOB_TASKNOSUITABLEJOB_U, sge_u32c(jobid)));
      goto Error;
   }

   /* do not accept the task if job is in deletion */
   if ((lGetUlong(jatep, JAT_state) & JDELETED)) {
      DPRINTF(("received task exec request while job is in deletion\n"));
      goto Error;
   }

   /* generate unique task id by combining consecutive number 1-max(u_long32) */
   tid = MAX(1, lGetUlong(jatep, JAT_next_pe_task_id));
   sprintf(new_task_id, "%d.%s", tid, unqualified_hostname);
   DPRINTF(("using pe_task_id_str %s for job "sge_u32"."sge_u32"\n", new_task_id, jobid, jataskid));
   lSetString(petep, PET_id, new_task_id);

   /* set taskid for next task to be started */
   lSetUlong(jatep, JAT_next_pe_task_id, tid + 1);

   lSetString(petep, PET_name, "petask");
   lSetUlong(petep, PET_submission_time, lGetUlong(petrep, PETR_submission_time));
   lSetString(petep, PET_cwd, lGetString(petrep, PETR_cwd));
   lSetList(petep, PET_environment, 
            lCopyList("petask environment", lGetList(petrep, PETR_environment)));
   lSetList(petep, PET_path_aliases, 
            lCopyList("petask path_aliases", lGetList(petrep, PETR_path_aliases)));

   requested_queue = lGetString(petrep, PETR_queuename);

   DPRINTF(("got task ("sge_u32"/%s) from (%s/%s/%d) %s queue selection\n", 
            lGetUlong(jep, JB_job_number), new_task_id,
            de->commproc, de->host, de->id, 
            requested_queue != NULL ? "with" : "without"));

   gdil = job_get_queue_for_task(jatep, petep, qualified_hostname, requested_queue);
         
   if (gdil == NULL) { /* ran through list without finding matching queue */ 
      gdil = job_get_queue_with_task_about_to_exit(jep, jatep, petep, qualified_hostname, requested_queue);
   }
         
   if (gdil == NULL) {  /* also no already exited task found -> no way to start new task */
      ERROR((SGE_EVENT, MSG_JOB_NOFREEQ_USSS, sge_u32c(jobid), 
             lGetString(petrep, PETR_owner), de->host, qualified_hostname));
      goto Error;
   }

   /* put task into task_list of slave/master job */ 
   if (lGetList(jatep, JAT_task_list) == NULL) {
DTRACE;
      lSetList(jatep, JAT_task_list, lCreateList("task_list", PET_Type));
   }
DTRACE;
   /* put task into task_list of slave/master job */ 
   lAppendElem(lGetList(jatep, JAT_task_list), petep);

DTRACE;

   if (job_write_spool_file(jep, jataskid, NULL, SPOOL_WITHIN_EXECD)) { 
      sge_dstring_copy_string(&err_str, SGE_EVENT);
      execd_job_start_failure(jep, jatep, petep, sge_dstring_get_string(&err_str), 1);
      goto Error;
   }
   
DTRACE;   
   /* 
    *
    * At this time we are sure that we have the task on disk.
    * Now add a new "running" element for this job to the job 
    * report which is used as ACK for this job send request.
    * Add the submission time for the task here.
    */
   {
      lListElem *jr = add_job_report(jobid, jataskid, new_task_id, jep);
      add_usage(jr, "submission_time", NULL, lGetUlong(petep, PET_submission_time));
   }
DTRACE;

   /* for debugging: never start job but report a failure */
   if (getenv("FAILURE_BEFORE_START")) {
      execd_job_start_failure(jep, jatep, petep, "FAILURE_BEFORE_START", 0);
   }   

   if (sge_make_pe_task_active_dir(jep, jatep, petep, NULL) == NULL) {
     goto Error;
   }

DTRACE;
   /* put task into task_list of slave/master job */ 
   /* send ack to sender of task */
   if (tid) {
      DPRINTF(("sending tid %s\n", new_task_id)); 
      packstr(apb, new_task_id);
      *synchron = 0;
   }

   DRETURN(0);

Error:
   /* send nack to sender of task */
   DPRINTF(("sending nack\n")); 
   packstr(apb, "none");    
   *synchron = 0;

   sge_dstring_free(&err_str);
   DRETURN(-1);
}
