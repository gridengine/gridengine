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
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_queueL.h"
#include "slots_used.h"
#include "dispatcher.h"
#include "sge_string.h"
#include "sge_parse_num_par.h"
#include "job_log.h"
#include "reaper_execd.h"
#include "job_report_execd.h"
#include "execd_job_exec.h"
#include "read_write_job.h"
#include "sge_feature.h"
#include "sge_conf.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "sge_log.h"
#include "sge_max_nis_retries.h"
#include "sge_io.h"
#include "sge_getpwnam.h"
#include "execution_states.h"
#include "job.h"
#include "sge_peopen.h"
#include "sge_copy_append.h"
#include "sge_arch.h"
#include "sge_afsutil.h"
#include "sge_switch_user.h"
#include "setup_path.h"
#include "sge_stat.h" 
#include "jb_now.h"
#include "sge_security.h"
#include "msg_common.h"
#include "msg_execd.h"
#include "msg_gdilib.h"

extern volatile int jobs_to_start;
extern lList *Master_Job_List;

static int execd_job_exec_(struct dispatch_entry *de, sge_pack_buffer *pb, sge_pack_buffer *apb, int *synchron, int slave);
static int handle_job(lListElem *jelem, lListElem *jatep, struct dispatch_entry *de, sge_pack_buffer *pb, int slave);
static int handle_task(lListElem *jelem, lListElem *jatep, struct dispatch_entry *de, sge_pack_buffer *pb, sge_pack_buffer *apb, int *synchron);

/*************************************************************************
EXECD/QSTD function called by dispatcher

get job from qmaster and store it in execd local structures and files
real execution is done by the cyclic execd_ck_to_do()/qstd_ck_to_do()

 jobs_to_start will be incremented

 qmaster counterpart is sge_give_jobs()


 Files generated here:
   job_scripts/<jid>   for the script 
   jobs/<jid>          for the job structure

 *************************************************************************/

int execd_job_exec(de, pb, apb, rcvtimeout, synchron, err_str, answer_error)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
u_long *rcvtimeout; 
int *synchron; 
char *err_str; 
int answer_error;
{
   return execd_job_exec_(de, pb, apb, synchron, 0);
}

int execd_job_slave(de, pb, apb, rcvtimeout, synchron, err_str, answer_error)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
u_long *rcvtimeout; 
int *synchron; 
char *err_str; 
int answer_error;
{
   return execd_job_exec_(de, pb, NULL, NULL, 1);
}

static int execd_job_exec_(de, pb, apb, synchron, slave)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
int *synchron;
int slave;
{
   int ret = 1;
   lListElem *jelem, *ja_task;
   u_long32 feature_set;

   DENTER(TOP_LAYER, "execd_job_exec");

   /* ------- featureset */
   if (unpackint(pb, &feature_set)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKFEATURESET));
      DEXIT;
      return 0;
   }

   feature_activate(feature_set);

   /* ------- job */
   if (cull_unpack_elem(pb, &jelem, NULL)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKJOB));
      DEXIT;
      return 0;
   }

   for_each(ja_task, lGetList(jelem, JB_ja_tasks)) {
      DPRINTF(("Job: %ld Task: %ld\n", (long) lGetUlong(jelem, JB_job_number), 
         (long) lGetUlong(ja_task, JAT_task_number)));
      if (strcmp(de->commproc, prognames[QMASTER])) {
         ret = handle_task(jelem, ja_task, de, pb, apb, synchron);
      } else {
         ret = handle_job(jelem, ja_task, de, pb, slave);
      }
   }
   if (ret)  {
      lFreeElem(jelem);
   } else { /* succsess - set some triggers */ 
      if (slave)
         flush_jr = 1;
      else    
         jobs_to_start = 1;
   }
   DEXIT;
   return 0;
}

static int handle_job(
lListElem *jelem,
lListElem *jatep,
struct dispatch_entry *de,
sge_pack_buffer *pb, 
int slave 
) {
   lListElem *jep, *pelem = NULL, *jep_jatep = NULL;
   char dir[SGE_PATH_MAX];
   char err_str[256+SGE_PATH_MAX];
   u_long32 jobid, jataskid;
   int mail_on_error = 0, general = GFSTATE_QUEUE;
   lListElem *qep, *gdil_ep;
   lList *tmp_qlp, *qlp = NULL;
   char *qnm;
   int slots;
   int fd;

   DENTER(TOP_LAYER, "handle_job");

   DPRINTF(("got %s job "u32" from (%s/%s/%d)\n",
           slave ?"slave ":"", lGetUlong(jelem, JB_job_number),
           de->commproc, de->host, de->id));

   lSetString(jelem, JB_job_source, NULL); 

   jobid = lGetUlong(jelem, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);

   /* 
    * Sometimes the rescheduled job from qmaster which is 
    * sent synchronuously arrives earlier than the ack 
    * reporting that qmaster got job exit message from run before 
    * 
    * We can ignore this job because job is resend by qmaster.
    */
   for_each(jep, Master_Job_List) {
      if (lGetUlong(jep, JB_job_number) == jobid && 
          (jep_jatep = search_task(jataskid, jep))) {
         DPRINTF(("Job "u32"."u32" is already running - skip the new one\n", 
            jobid, jataskid));
         DEXIT;
         goto Ignore;   /* don't set queue in error state */
      }
   }

   if (slave) {
      /* make job directory for sub tasks
         in case of master jobs this is done when starting the jobs */
      sprintf(dir, "%s/"u32"."u32 , ACTIVE_DIR, jobid, jataskid);
      if (mkdir(dir, 0755) == -1) {
         sprintf(err_str, MSG_FILE_CREATEDIR_SS, dir, strerror(errno));
         DEXIT;
         goto Error;
      }
   }

   /* initialize state - prevent slaves from getting started */
   lSetUlong(jatep, JAT_status, slave?JSLAVE:JIDLE);

   /* now we have a queue and a job filled */
   if (feature_is_enabled(FEATURE_SGEEE))
      DPRINTF(("===>JOB_EXECUTION: >"u32"."u32"< with "u32" tickets\n", jobid, jataskid,
               (u_long32)lGetDouble(jatep, JAT_ticket)));
   else
      DPRINTF(("===>JOB_EXECUTION: >"u32"."u32"<\n", jobid, jataskid));

   job_log(jobid, jataskid, MSG_COM_RECEIVED);

   if (cull_unpack_list(pb, &qlp)) {
      sprintf(err_str, MSG_COM_UNPACKINGQ);
      DEXIT;
      goto Error;
   }

   /* initialize job */
   /* store queues as sub elems of gdil */
   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      qnm=lGetString(gdil_ep, JG_qname);
      if (!(qep=lGetElemStr(qlp, QU_qname, qnm))) {
         sprintf(err_str, MSG_JOB_MISSINGQINGDIL_SU, qnm, u32c(lGetUlong(jelem, JB_job_number)));
         DEXIT;
         goto Error;
      }

      tmp_qlp = lCreateList("", lGetListDescr(qlp));
      qep = lDechainElem(qlp, qep);
      /* clear any queue state that might be set from qmaster */
      lSetUlong(qep, QU_state, 0);

      /* store number of slots we got in this queue for this job */
      slots = lGetUlong(gdil_ep, JG_slots);
      lSetUlong(qep, QU_job_slots, slots);
      set_qslots_used(qep, 0);
      lAppendElem(tmp_qlp, qep);
      lSetList(gdil_ep, JG_queue, tmp_qlp);
      DPRINTF(("Q: %s %d\n", qnm, slots));
   }
   /* trash envelope */
   qlp = lFreeList(qlp);

   /* ------- optionally pe */
   if (lGetString(jatep, JAT_granted_pe)) {
      cull_unpack_elem(pb, &pelem, NULL); /* pelem will be freed with jelem */
      lSetList(jelem, JB_pe_object, lCreateList("pe list", PE_Type));
      lAppendElem(lGetList(jelem, JB_pe_object), pelem);

      if (lGetUlong(pelem, PE_control_slaves)) {
         if (!lGetUlong(pelem, PE_job_is_first_task)) {
            int slots;
            lListElem *mq = lFirst(lGetList(lFirst(lGetList(jatep,
                        JAT_granted_destin_identifier_list)), JG_queue));
            slots = lGetUlong(mq, QU_job_slots) + 1;
            DPRINTF(("Increasing job slots in master queue \"%s\" "
               "to %d because job is not first task\n",
                  lGetString(mq, QU_qname), slots));
            lSetUlong(mq, QU_job_slots, slots);

         }
         DPRINTF(("Task ID range is "u32"-"u32"\n", 
            lGetUlong(jelem, JB_task_id_range),
            lGetUlong(jelem, JB_task_id_range)+TASK_ID_RANGE_SIZE-1));
      }
   }

   /* interactive jobs and slave jobs do not have a script file */
   if (!slave && lGetString(jelem, JB_script_file)) {
      int nwritten;
    
      /* We are root. Make the scriptfile readable for the jobs submitter,
         so shepherd can open (execute) it after changing to the user. */
      fd = open(lGetString(jelem, JB_exec_file), O_CREAT | O_WRONLY, 0755);

      if (fd < 0) {
         {
            char buffer[1024];

            getcwd(buffer, 1024);
            DPRINTF(("### CWD: %s", buffer));
         }

         sprintf(err_str, MSG_FILE_NOWRITE_SS, lGetString(jelem, JB_exec_file), strerror(errno));
         DEXIT;
         goto Error;
      }

      if ((nwritten = sge_writenbytes(fd, lGetString(jelem, JB_script_ptr), 
         lGetUlong(jelem, JB_script_size))) !=
         lGetUlong(jelem, JB_script_size)) {
         sprintf(err_str, MSG_EXECD_NOWRITESCRIPT_SIUS, lGetString(jelem, JB_exec_file), 
               nwritten, u32c(lGetUlong(jelem, JB_script_size)), strerror(errno));
         close(fd);
         DEXIT;
         goto Error;
      }      
      close(fd);
      lSetString(jelem, JB_script_ptr, NULL);
   }

   /* 
   ** security hook
   **
   ** Execute command to store the client's DCE or Kerberos credentials.
   ** This also creates a forwardable credential for the user.
   */
   if (do_credentials) {
      if (store_sec_cred2(jelem, do_authentication, &general, err_str) != 0)
         goto Error;
   }

#ifdef KERBEROS
   kerb_job(jelem, de);
#endif



   lSetUlong(jelem, JB_script_size, 0);
   if (cull_write_jobtask_to_disk(jelem, jataskid)) {
      /* SGE_EVENT is written by cull_write_jobtask_to_disk() */
      strcpy(err_str, SGE_EVENT);
      DEXIT;
      goto Error;
   }

   add_job_report(jobid, jataskid, jelem);

   if (!jep_jatep) {
      /* put into job list */
      lAppendElem(Master_Job_List, jelem);
   }

   DEXIT;
   return 0;

Error:
   {
      lListElem *jr;
      jr = execd_job_start_failure(jelem, jatep, err_str, general);
      if (mail_on_error)
         reaper_sendmail(jelem, jr);
   }

Ignore:   
   lFreeList(qlp);
   return -1;  
}

/****** execd/set_queue_info_in_task() ***************************************
*
*  NAME
*    set_queue_info_in_task() -- set queue to use for task
*
*  SYNOPSIS
*     static lList *set_queue_info_in_task(char *qname, lListElem *jatask);
*
*  FUNCTION
*     Extend the task structure of task <jatask> by a JAT_granted_destin_identifier
*     list, which contains the queue <qname> and uses one slot.
*
*  INPUTS
*     qname  - name of queue to set
*     jatask - task structure
*
*  RESULT
*     the new created JAT_granted_destin_identifier list
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/
static lList *set_queue_info_in_task(char *qname, lListElem *jatask)
{
   lListElem *jge;

   jge = lAddSubStr(jatask, JG_qname, qname,
                    JAT_granted_destin_identifier_list, JG_Type);
   lSetString(jge, JG_qhostname, me.qualified_hostname);
   lSetUlong(jge, JG_slots, 1);
   DPRINTF(("selected queue %s for task\n", qname));

   return lGetList(jatask, JAT_granted_destin_identifier_list);
}

/****** execd/get_queue_with_task_about_to_exit() ***************************************
*
*  NAME
*     get_queue_with_task_about_to_exit -- find queue with already exited task
*
*  SYNOPSIS
*     static lList *get_queue_with_task_about_to_exit(lListElem *jatep, 
*                                              lListElem *jatask,
*                                              u_long32 jobid,
*                                              u_long32 jataskid);
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
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     execd/set_queue_info_in_task()
*
****************************************************************************
*/

static lList *get_queue_with_task_about_to_exit(lListElem *jatep, lListElem *jatask,
                                                u_long32 jobid, u_long32 jataskid)
{
   char cwd[SGE_PATH_MAX + 1];
   lListElem *pe_task;
   
   DENTER(TOP_LAYER, "get_queue_with_task_about_to_exit");
   
   if(getcwd(cwd, SGE_PATH_MAX) == NULL) {
      DEXIT;
      return NULL;
   }   

   for_each(pe_task, lGetList(jatep, JAT_task_list)) {
      lListElem *pe_task_ja_task = NULL;

      pe_task_ja_task = lFirst(lGetList(pe_task, JB_ja_tasks));
      if(pe_task_ja_task != NULL) {
         lListElem *pe_task_queue   = NULL;
         pe_task_queue = lFirst(lGetList(pe_task_ja_task, JAT_granted_destin_identifier_list));
         if(pe_task_queue != NULL) {
            char shepherd_about_to_exit[SGE_PATH_MAX + 1];
            char *pe_task_no = NULL;
            SGE_STRUCT_STAT stat_buffer;
            
            pe_task_no = lGetString(pe_task, JB_pe_task_id_str);
            sprintf(shepherd_about_to_exit, "%s/active_jobs/" u32 "." u32 "/%s/shepherd_about_to_exit", 
                    cwd, jobid, jataskid, pe_task_no);

            DPRINTF(("checking for file %s\n", shepherd_about_to_exit));

            if(SGE_STAT(shepherd_about_to_exit, &stat_buffer) == 0) {
               DPRINTF(("task %s of job %d.%d already exited, using his slot for new task\n", 
                        pe_task_no, jobid, jataskid));
               DEXIT;         
               return set_queue_info_in_task(lGetString(pe_task_queue, JG_qname), jatask); 
            }
         }
      }   
   }

   DEXIT;
   return NULL;
}

/****** execd/get_queue_for_task() ***************************************
*
*  NAME
*     get_queue_for_task() -- find a queue suited for task execution
*
*  SYNOPSIS
*     static lList *get_queue_for_task(lListElem *jatep,
*                                      lListElem *jatask);
*
*  FUNCTION
*     Search for a queue, that 
*        - may be used by job in which new task shall be started
*        - resides on the host of this execd
*        - has free slots for the job in which new task shall be started
*     If a suited queue is found, it is set to be used by the new task.
*
*  INPUTS
*     jatep    - the actual job (substructure job array task)
*     jatask   - the new pe task
*
*  RESULT
*     on success, the JAT_granted_destin_identifier list of the new pe task
*     else NULL
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     execd/set_queue_info_in_task()
*
****************************************************************************
*/

static lList *get_queue_for_task(lListElem *jatep, lListElem *jatask) 
{
   lListElem *this_q, *gdil_ep;

   for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
      this_q = lFirst(lGetList(gdil_ep, JG_queue));

      /* must be at this host and either must have free slots or task about to exit */
      if(this_q  && !hostcmp(lGetString(gdil_ep, JG_qhostname), me.qualified_hostname) 
                 && qslots_used(this_q) < lGetUlong(this_q, QU_job_slots)) {
         return set_queue_info_in_task(lGetString(gdil_ep, JG_qname), jatask);
      } 
   }

   return NULL;
}

static int handle_task(jelem, jatask, de, pb, apb, synchron)
lListElem *jelem;
lListElem *jatask;
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
int *synchron; 
{
   u_long32 jobid, jataskid;
   lListElem *jep, *tep, *ep, *pe, *jatep;
   char job_source[1024], new_task_id[12];
   char *task_str;
   lList *gdil = NULL;
   int tid = 0;
   char err_str[256+SGE_PATH_MAX];

   DENTER(TOP_LAYER, "handle_task");

   /* may be we have to send a task exit message to this guy */
   sprintf(job_source, "%s:%s:%d", de->host, de->commproc, de->id);
   lSetString(jelem, JB_job_source, job_source);

#ifdef KERBEROS

   if (krb_verify_user(de->host, de->commproc, de->id,
                       lGetString(jelem, JB_owner)) < 0) {
      ERROR((SGE_EVENT, MSG_SEC_KRB_CRED_SSSI, lGetString(jelem, JB_owner), de->host, de->commproc, de->id));
      goto Error;
   }

#endif /* KERBEROS */

   jobid = lGetUlong(jelem, JB_job_number);
   if (!(jep=lGetElemUlong(Master_Job_List, JB_job_number, jobid))) {
      ERROR((SGE_EVENT, MSG_JOB_TASKWITHOUTJOB_U, u32c(jobid))); 
      goto Error;
   }
   jataskid = lGetUlong(jatask, JAT_task_number);
   if (!(jatep=search_task(jataskid, jep))) { 
      ERROR((SGE_EVENT, MSG_JOB_TASKNOTASKINJOB_UU, u32c(jobid), u32c(jataskid)));
      goto Error;
   }

   /* do not accept the task if job is not parallel or 'control_slaves' is not active */
   if (!(pe=lFirst(lGetList(jep, JB_pe_object))) || !lGetUlong(pe, PE_control_slaves)) {
      ERROR((SGE_EVENT, MSG_JOB_TASKNOSUITABLEJOB_U, u32c(jobid)));
      goto Error;
   }

   /* do not accept the task if job is in deletion */
   if ((lGetUlong(jatep, JAT_state) & JDELETED)) {
      DPRINTF(("received task exec request while job is in deletion\n"));
      goto Error;
   }

   /* test whether we get a taskid from outside */
   if (!(ep=lGetSubStr(jelem, VA_variable, "TASK_ID", JB_env_list)) ||
       !(task_str=lGetString(ep, VA_value))) {
      int found;
   
      /* need to start with different values for different hosts */
      tid = MAX(1, lGetUlong(jep, JB_task_id_range));
      do {
         found = 0;
         for_each (tep, lGetList(jatep, JAT_task_list)) {
            if (atoi(lGetString(tep, JB_pe_task_id_str))==tid) {
               found = 1;
               tid++;
               break;
            }
         }
      } while (found);

      sprintf(new_task_id, "%d", tid);
      task_str = new_task_id;
      DPRINTF(("found free task id %s for job "u32"\n", 
         task_str, jobid));

      if (!ep)
         ep = lAddSubStr(jelem, VA_variable, "TASK_ID", 
               JB_env_list, VA_Type);
      lSetString(ep, VA_value, task_str);
   } else { /* user proposes a task id - ensure we do 
               not already have a task with this id */
      for_each (tep, lGetList(jatep, JAT_task_list)) {
         if (!strcmp(lGetString(tep, JB_pe_task_id_str), task_str)) {
            ERROR((SGE_EVENT, MSG_JOB_TASKALREADYEXISTS_US, u32c(jobid), task_str));
            goto Error;
         }
      }
   }

   lSetString(jelem, JB_pe_task_id_str, task_str);
   if (!lGetString(jelem, JB_job_name))
      lSetString(jelem, JB_job_name, 
            sge_basename(lGetString(jelem, JB_script_file), '/'));

   /* check whether interface to tasker is fulfilled */
   if (!strcmp(de->commproc, prognames[PVM_TASKER]) && 
      (!lGetSubStr(jelem, VA_variable, "PVM_TASKER_PID", JB_env_list) ||
       !lGetSubStr(jelem, VA_variable, "SIG_INFO_FILE", JB_env_list))) {
      ERROR((SGE_EVENT, MSG_JOB_NOTASKPASSINGIF_SU, 
                        lGetString(jelem, JB_job_source), 
                        u32c(lGetUlong(jelem, JB_job_number))));
      goto Error;
   }

   /* has tasker selected a queue or is it our job to decide this */
   gdil = lGetList(jatask, JAT_granted_destin_identifier_list);

   DPRINTF(("got task ("u32"/%s) from (%s/%s/%d) %s queue selection\n", 
      lGetUlong(jelem, JB_job_number), task_str,
      de->commproc, de->host, de->id, gdil?"with":"without"));

   DPRINTF(("===>TASK_EXECUTION: >" u32 "<\n", lGetUlong(jelem, JB_job_number)));

   { 
      lListElem *this_q; 

      if (!gdil) {    /* got task without queue selection - do this for the task */
         gdil = get_queue_for_task(jatep, jatask);
         
         if (!gdil) { /* ran through list without finding matching queue */ 
            gdil = get_queue_with_task_about_to_exit(jatep, jatask, jobid, jataskid);
         }
         
         if(!gdil) {  /* also no already exited task found -> no way to start new task */
            ERROR((SGE_EVENT, MSG_JOB_NOFREEQ_USSS, u32c(jobid), 
                   lGetString(jelem, JB_owner), de->host, me.qualified_hostname));
         }
      } else { /* look whether requested queue fits for task */
         char *qnm = lGetString(lFirst(gdil), JG_qname); 
         lListElem *job_gdil;

         job_gdil = lGetElemStr(lGetList(jatep, JAT_granted_destin_identifier_list), JG_qname, qnm);
         this_q = lFirst(lGetList(job_gdil , JG_queue));
         if (!this_q) {
            ERROR((SGE_EVENT, MSG_JOB_NOSUCHQ_SUSS, qnm, u32c(jobid), lGetString(jelem, JB_owner), de->host));
            gdil = NULL;
         } else if (hostcmp(lGetString(job_gdil, JG_qhostname), me.qualified_hostname)) {
            ERROR((SGE_EVENT, MSG_JOB_NOREQQONHOST_SSS, qnm, me.qualified_hostname, lGetString(job_gdil, JG_qhostname)));
            gdil = NULL;
         } else if (lGetUlong(this_q, QU_job_slots)<=qslots_used(this_q)) {
            ERROR((SGE_EVENT, MSG_JOB_REQQFULL_SII, qnm, (int)lGetUlong(this_q, QU_job_slots), qslots_used(this_q)));
            gdil = NULL;
         }

         if (gdil) 
            lSetString(lFirst(gdil), JG_qhostname, me.qualified_hostname);
      }

      if (!gdil) {
         goto Error;
      }
   }

   lSetUlong(jelem, JB_script_size, 0);
   if (cull_write_jobtask_to_disk(jelem, jataskid)) {
      strcpy(err_str, SGE_EVENT);
      execd_job_start_failure(jelem, jatask, err_str, 1);
      goto Error;
   }

   /* 
    *
    * At this time we are sure that we have the task on disk.
    * Now add a new "running" element for this job to the job 
    * report which is used as ACK for this job send request.
    *
    */
   add_job_report(jobid, jataskid, jelem);
DTRACE;
   /* put task into task_list of slave/master job */ 
   if(!lGetList(jatep, JAT_task_list)) {
DTRACE;
   /* put task into task_list of slave/master job */ 
      lSetList(jatep, JAT_task_list, lCreateList("task_list", JB_Type));
   }
DTRACE;
   /* put task into task_list of slave/master job */ 

   lSetList(jatask, JAT_granted_destin_identifier_list, lCopyList("gdil list", gdil));
   lAppendElem(lGetList(jatep, JAT_task_list), jelem);

DTRACE;

   /* for debugging: never start job but report a failure */
   if (getenv("FAILURE_BEFORE_START"))
      execd_job_start_failure(jelem, jatask, "FAILURE_BEFORE_START", 0);

DTRACE;
   /* put task into task_list of slave/master job */ 
   /* send ack to sender of task */
   if (tid) {
      DPRINTF(("sending tid %d\n", tid)); 
      packint(apb, tid);
      *synchron = 0;
   }

   DEXIT;
   return 0;

Error:
   /* send nack to sender of task */
   /* if (tid) { */
      DPRINTF(("sending nack\n")); 
      packint(apb, 0);    
      *synchron = 0;
   /* } */
   DEXIT;
   return -1;
}
