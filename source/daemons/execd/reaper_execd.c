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
#include <sys/types.h>
#include <errno.h>
#include <float.h>

#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <grp.h>

#include "sgermon.h"
#include "sge.h"
#include "symbols.h"
#include "config.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_qinstance.h"
#include "sge_os.h"
#include "sge_log.h"
#include "sge_usage.h"
#include "sge_any_request.h"
#include "sge_time.h"
#include "admin_mail.h"
#include "mail.h"
#include "exec_job.h"
#include "config_file.h"
#include "sge_signal.h"
#include "dispatcher.h"
#include "tmpdir.h"
#include "sge_job_qmaster.h"
#include "execution_states.h"
#include "sge_load_sensor.h"
#include "reaper_execd.h"
#include "job_report_execd.h"
#include "sge_prog.h"
#include "sge_conf.h"
#include "sge_qexec.h"
#include "sge_string.h"
#include "sge_afsutil.h"
#include "sge_parse_num_par.h"
#include "setup_path.h"
#include "get_path.h"
#include "msg_common.h"
#include "msg_daemons_common.h"
#include "msg_execd.h"
#include "sge_security.h" 
#include "sge_feature.h"
#include "sge_spool.h"
#include "spool/classic/read_write_job.h"
#include "sge_job.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"
#include "sge_var.h"
#include "sge_report.h"
#include "sge_ulong.h"

#ifdef COMPILE_DC
#  include "ptf.h"
static void unregister_from_ptf(u_long32 jobid, u_long32 jataskid, const char *pe_task_id, lListElem *jr);
#endif

static int clean_up_job(lListElem *jr, int failed, int signal, int is_array, const lListElem *pe);
static void convert_attribute(lList **cflpp, lListElem *jr, char *name, u_long32 udefau);
static int extract_ulong_attribute(lList **cflpp, char *name, u_long32 *valuep); 

static lListElem *execd_job_failure(lListElem *jep, lListElem *jatep, lListElem *petep, const char *error_string, int general, int failed);
static int read_dusage(lListElem *jr, const char *jobdir, u_long32 jobid, u_long32 jataskid, int failed, int usage_mul_factor);
static void build_derived_final_usage(lListElem *jr, int usage_mul_factor);

static void examine_job_task_from_file(int startup, char *dir, lListElem *jep, lListElem *jatep, lListElem *petep, pid_t *pids, int npids);


/*****************************************************************************
 This code is used only by execd.
 Execd has gotten a SIGCLD or execd starts up.
 Look whether there are finished jobs. Report them to the qmaster.
 Our input is what shepherd wrote to the directory of the job:
 "<execd_spool>/active_jobs/<jobnumber>/".

 "error"-file  errors detected by shepherd
 "trace"-file  information only interesting for sge programmers
 "usage"-file  usage of job (man getrusage)
 "pid"-file    pid of shepherd; can be used to look into the process table

 If everything is done we can remove the job directory.
 ****************************************************************************/
void sge_reap_children_execd()
{
   int pid = 999;
   int exit_status, child_signal, core_dumped, failed;
   u_long32 jobid, jataskid;
   lListElem *jep, *petep = NULL, *jatep = NULL;

   int status;

   DENTER(TOP_LAYER, "sge_reap_children_execd");
   DPRINTF(("========================REAPER======================\n"));

   pid = 999;

   while (pid > 0) {

      exit_status = child_signal = core_dumped = failed = 0;

      pid = waitpid(-1, &status, WNOHANG);

      if (pid == 0) {
         DPRINTF(("pid==0 - no stopped or exited children\n"));
         DEXIT;                 /* no stopped or exited children */
         continue;
      }

      if (pid == -1) {
         DPRINTF(("pid==-1 - no children not previously waited for\n"));
         DEXIT;
         return;
      }

      if (WIFSTOPPED(status)) {
         DPRINTF(("PID %d WIFSTOPPED\n", pid));
         continue;
      }

#ifdef WIFCONTINUED
      if (WIFCONTINUED(status)) {
         DPRINTF(("PID %d WIFCONTINUED\n", pid));
         continue;
      }
#endif

      if (WIFSIGNALED(status)) {
         child_signal = WTERMSIG(status);
#ifdef WCOREDUMP
         core_dumped = WCOREDUMP(status);
#else
         core_dumped = status & 80;
#endif
         failed = ESSTATE_DIED_THRU_SIGNAL;
      } else if (WIFEXITED(status)) {
         exit_status = WEXITSTATUS(status);
         if (exit_status)
            failed = ESSTATE_SHEPHERD_EXIT;
      } else {
         /* not signaled and not exited - so what else happend with this guy? */
         WARNING((SGE_EVENT, MSG_WAITPIDNOSIGNOEXIT_UI, u32c(pid), status));
         continue;
      }
  
      /* search whether it was a job or one of its tasks */
      for_each(jep, Master_Job_List) {
         int Break = 0;
   
         petep = NULL;
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if (lGetUlong(jatep, JAT_pid) == pid) {
               petep = NULL;
               Break = 1;
               break;
            }
            
            for_each(petep, lGetList(jatep, JAT_task_list)) {
               if (lGetUlong(petep, PET_pid) == pid) {
                  break;
               }
            }
            
            if (petep) {
               Break = 1;
               break;
            }
         }
         if (Break) {
            break;
         }
      }

      /* was it a job ? */
      if (jep && jatep) {
         lListElem *jr;

         jobid = lGetUlong(jep, JB_job_number);
         jataskid = lGetUlong(jatep, JAT_task_number);
   
         if (child_signal)
            ERROR((SGE_EVENT, MSG_SHEPHERD_VSHEPHERDOFJOBWXDIEDTHROUGHSIGNALYZ_SUUSI,
                     (petep ? MSG_SLAVE : "" ),
                     u32c(jobid), 
                     u32c(jataskid),
                     core_dumped ? MSG_COREDUMPED: "",
                     child_signal));

         if (exit_status) {
            ERROR((SGE_EVENT, MSG_SHEPHERD_WSHEPHERDOFJOBXYEXITEDWITHSTATUSZ_SUUI, 
                  (petep ? MSG_SLAVE : "" ), 
                  u32c(jobid), 
                  u32c(jataskid), 
                  exit_status));
         }

         /* 
          *  seek job report for this job it must be contained in job report
          *  if not it should be a job kept with SGE_KEEP_ACTIVE (without
          *  keeping job object itself)
          */
         DPRINTF(("Job: "u32", JA-Task: "u32", PE-Task: %s\n", jobid, jataskid, 
            petep != NULL ? lGetString(petep, PET_id) : ""));
         if (!(jr=get_job_report(jobid, jataskid, petep != NULL ? lGetString(petep, PET_id) : NULL))) {
            ERROR((SGE_EVENT, MSG_JOB_MISSINGJOBXYINJOBREPORTFOREXITINGJOBADDINGIT_UU, 
                   u32c(jobid), u32c(jataskid)));
            jr = add_job_report(jobid, jataskid, petep != NULL ? lGetString(petep, PET_id) : NULL, jep);
         }

         /* when restarting execd it happens that cleanup_old_jobs()
            has already cleaned up this job */
         if (lGetUlong(jr, JR_state)==JEXITING) {
            DPRINTF(("State of job "u32" already changed to JEXITING\n", jobid));
            continue;
         }

         lSetUlong(jr, JR_state, JEXITING);

         if(petep != NULL) {
            lSetUlong(petep, PET_status, JEXITING);
         } else {
            lSetUlong(jatep, JAT_status, JEXITING);
         }

         clean_up_job(jr, failed, exit_status, job_is_array(jep),
                      lGetObject(jatep, JAT_pe_object));

         flush_jr = 1; /* trigger direct sending of job reports */ 

      } else  if (sge_ls_stop_if_pid(pid, 1)) {
         if (child_signal) {
            ERROR((SGE_EVENT, MSG_STATUS_LOADSENSORDIEDWITHSIGNALXY_SI,
                  core_dumped ? MSG_COREDUMPED: "",
                  child_signal));
         } else {
            WARNING((SGE_EVENT, MSG_STATUS_LOADSENSOREXITEDWITHEXITSTATUS_I,
                    exit_status));
         }
      } else {
         if (child_signal)
            ERROR((SGE_EVENT, MSG_STATUS_MAILERDIEDTHROUGHSIGNALXY_SI,
                  core_dumped ? MSG_COREDUMPED: "",
                  child_signal));
         else if (exit_status)
            ERROR((SGE_EVENT, MSG_STATUS_MAILEREXITEDWITHEXITSTATUS_I,
                  exit_status));
      }
   }

   DEXIT;
   return;
}

#ifdef COMPILE_DC
/* 

DESC

   unregisters job from ptf and fills usage values
   into appropriate job report element

RETURNS

   0 everyting worked fine
   1 got only zero usage from ptf

*/
static void unregister_from_ptf(
u_long32 job_id,
u_long32 ja_task_id,
const char *pe_task_id,
lListElem *jr 
) {
   lList* usage = NULL;
   int ptf_error;

   DENTER(TOP_LAYER, "unregister_from_ptf");

   ptf_error = ptf_job_complete(job_id, ja_task_id, pe_task_id, &usage);
   if (ptf_error) {
      WARNING((SGE_EVENT, MSG_JOB_REAPINGJOBXPTFCOMPLAINSY_US,
               u32c(job_id), ptf_errstr(ptf_error)));
   } else {
      if (usage) {
         lXchgList(jr, JR_usage, &usage);
         lFreeList(usage);
      }
   }

   DEXIT;
   return;
}
#endif

/************************************************************************
   This is the job clean up and report function. We design this function
   to be as independent from the execd as possible. Maybe we want to 
   make an extra executable later. We have another function cleaning up jobs
   (execd_job_start_failure). This function is called if the job starting 
   failed. At this time there is no config-file present.

   jobid = id of job to reap
   failed = indicates a failure of job execution, see shepherd_states.h
 ************************************************************************/
static int clean_up_job(lListElem *jr, int failed, int shepherd_exit_status, 
                        int is_array, const lListElem *pe) 
{
   dstring jobdir = DSTRING_INIT;
   dstring fname  = DSTRING_INIT;
   SGE_STRUCT_STAT statbuf;
   char error[10000];
   FILE *fp;
   u_long32 job_id, job_pid, ckpt_arena, general_failure = 0, ja_task_id;
   const char *pe_task_id = NULL;
   lListElem *du;
   int usage_mul_factor;

   DENTER(TOP_LAYER, "clean_up_job");

   if (!jr) {
      CRITICAL((SGE_EVENT, MSG_JOB_CLEANUPJOBCALLEDWITHINVALIDPARAMETERS));
      DEXIT;
      return -1;
   }

   job_id = lGetUlong(jr, JR_job_number);
   ja_task_id = lGetUlong(jr, JR_ja_task_number);
   pe_task_id = lGetString(jr, JR_pe_task_id_str);

   DPRINTF(("cleanup for job %s\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id)));

#ifdef COMPILE_DC
   unregister_from_ptf(job_id, ja_task_id, pe_task_id, jr);
#else
   lDelSubStr(jr, UA_name, USAGE_ATTR_CPU, JR_usage);
   lDelSubStr(jr, UA_name, USAGE_ATTR_MEM, JR_usage);
   lDelSubStr(jr, UA_name, USAGE_ATTR_IO, JR_usage);
   lDelSubStr(jr, UA_name, USAGE_ATTR_IOW, JR_usage);
   lDelSubStr(jr, UA_name, USAGE_ATTR_VMEM, JR_usage);
   lDelSubStr(jr, UA_name, USAGE_ATTR_MAXVMEM, JR_usage);
#endif

#if 0  
   /* moved to remove_acked_job_exit() */
   krb_destroy_forwarded_tgt(job_id);
#endif

   /* set directory for job */
   sge_get_active_job_file_path(&jobdir, job_id, ja_task_id, pe_task_id, NULL);

   if (SGE_STAT(sge_dstring_get_string(&jobdir), &statbuf)) {
      /* This never should happen, cause if we cant create this directory on
         startup we report the job finish immediately */
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDDIRXFORREAPINGJOBYZ_SS, 
             sge_dstring_get_string(&jobdir), 
             job_get_id_string(job_id, ja_task_id, pe_task_id)));
      sge_dstring_free(&jobdir);       
      return -1;        /* nothing can be done without information */
   }

   /* read config written by exec_job */
   sge_get_active_job_file_path(&fname, job_id, ja_task_id, pe_task_id, 
                                "config");
   if (read_config(sge_dstring_get_string(&fname))) {
      /* This should happen very rarely. exec_job() should avoid this 
         condition as far as possible. One possibility for this case is, 
         that the execd dies just after making the jobs active directory. 
         The pain with this case is, that we have not much information
         to report this job to qmaster. */
      ERROR((SGE_EVENT, MSG_JOB_CANTREADCONFIGFILEFORJOBXY_S, 
         job_get_id_string(job_id, ja_task_id, pe_task_id)));
      lSetUlong(jr, JR_failed, ESSTATE_NO_CONFIG);
      lSetString(jr, JR_err_str, (char *) MSG_SHEPHERD_EXECDWENTDOWNDURINGJOBSTART);

      sge_dstring_free(&fname);
      sge_dstring_free(&jobdir);
      DEXIT;
      return 0;
   }

   *error = '\0';

   DTRACE;

      
   /*
    * look for exit status of shepherd This is the last file the shepherd
    * creates. So if we can find this shepherd terminated normal.
    */
   sge_get_active_job_file_path(&fname,
                                job_id, ja_task_id, pe_task_id, "exit_status");
   if (!(fp = fopen(sge_dstring_get_string(&fname), "r"))) {
      /* 
       * we trust the exit status of the shepherd if it exited regularly
       * otherwise we assume it died before starting the job (if died through signal or
       * job_dir was found during execd startup)
       */
      if (failed == ESSTATE_SHEPHERD_EXIT)
         failed = shepherd_exit_status;   
      else 
         failed = SSTATE_BEFORE_PROLOG;
     
      sprintf(error, MSG_STATUS_ABNORMALTERMINATIONOFSHEPHERDFORJOBXY_S,
              job_get_id_string(job_id, ja_task_id, pe_task_id));
      ERROR((SGE_EVENT, SFQ , error));    
 
      /* 
       * failed = ESSTATE_SHEPHERD_EXIT or exit status of shepherd if we are
       * parent in case we can't open the exit_status file
       */
   }
   else {
      int fscanf_count, shepherd_exit_status_file;
         
      fscanf_count = fscanf(fp, "%d", &shepherd_exit_status_file);
      fclose(fp);
      if (fscanf_count != 1) {
         sprintf(error, MSG_STATUS_ABNORMALTERMINATIONFOSHEPHERDFORJOBXYEXITSTATEFILEISEMPTY_S,
                 job_get_id_string(job_id, ja_task_id, pe_task_id));
         ERROR((SGE_EVENT, error));
         /* 
          * If shepherd died through signal assume job was started, else
          * trust exit status
          */
         if (failed == ESSTATE_SHEPHERD_EXIT)
            failed = shepherd_exit_status;
         else
            failed = ESSTATE_NO_EXITSTATUS;
      }   
      else if (failed != ESSTATE_NO_PID)  /* only set during execd startup */
         failed = shepherd_exit_status_file;
         
      /* 
       * failed is content of exit_status file or ESSTATE_NO_PID or real
       * exit status if file can't be read
       */   
   }


   /*
    * failed is:
    *    exit status of shepherd (one of SSTATE_* values)
    *    ESSTATE_DIED_THRU_SIGNAL
    *    ESSTATE_NO_PID 
    */

   if (failed) {
      if (failed == ESSTATE_DIED_THRU_SIGNAL)
         sprintf(error, MSG_SHEPHERD_DIEDTHROUGHSIGNAL);
      else if (failed == ESSTATE_NO_PID)
          sprintf(error, MSG_SHEPHERD_NOPIDFILE);
      else
          sprintf(error, MSG_SHEPHERD_EXITEDWISSTATUS_I, failed);
   }

   /* look for error file this overrules errors found yet */
   sge_get_active_job_file_path(&fname,
                                job_id, ja_task_id, pe_task_id, "error");
   if ((fp = fopen(sge_dstring_get_string(&fname), "r"))) {
      int n;
      char *new_line;
      if ((n = fread(error, 1, sizeof(error), fp))) {
         /* Non empty error file. The shepherd encounterd a problem. */ 
         if (!failed)
            failed = ESSTATE_UNEXP_ERRORFILE;
         error[n] = '\0';
         /* ensure only first line of error file is in 'error' */
         if ((new_line=strchr(error, '\n')))
            *new_line = '\0';
         DPRINTF(("ERRORFILE: %256s\n", error));
      }
      else if (feof(fp)) {
         DPRINTF(("empty error file\n"));
      } else {
         ERROR((SGE_EVENT, MSG_JOB_CANTREADERRORFILEFORJOBXY_S, 
            job_get_id_string(job_id, ja_task_id, pe_task_id)));
      }      
      fclose(fp);
   }
   else {
      ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, sge_dstring_get_string(&fname), strerror(errno)));
      /* There is no error file. */
   }

   DTRACE;

   /*
   ** now that shepherd stops on errors there is no usage file
   ** if the job never ran
   ** but we need a dusage struct because information that has nothing 
   ** to do with the job also goes in there, this should be changed
   ** read_dusage gets the failed parameter to decide what should be there
   */
   
   /* to report correct usage for loosly integrated parallel jobs,
    * we have to compute a multiplication factor for acct_reserved_usage
    */

   {
      const char *s;
      int slots;

      slots = (s=get_conf_val("pe_slots"))?atoi(s):1;
      usage_mul_factor = execd_get_acct_multiplication_factor(pe, slots, 
                                                         pe_task_id != NULL);
   }

   if (read_dusage(jr, sge_dstring_get_string(&jobdir), job_id, ja_task_id, failed, usage_mul_factor)) {
      if (error[0] != '\0') {
         sprintf(error, MSG_JOB_CANTREADUSAGEFILEFORJOBXY_S, 
            job_get_id_string(job_id, ja_task_id, pe_task_id));
      }
      
      ERROR((SGE_EVENT, SFQ, error));
      
      if (!failed) {
         failed = SSTATE_FAILURE_AFTER_JOB;
      }
      
      DTRACE;
   }

   

   /* map system signals into sge signals to make signo's exchangable */
   du=lGetSubStr(jr, UA_name, "signal", JR_usage);
   if (du) {
      int signo = (int)lGetDouble(du, UA_value);

      if (signo) {
         u_long32 sge_signo;

         /* Job died through a signal */
         sprintf(error, MSG_JOB_WXDIEDTHROUGHSIGNALYZ_SSI, 
                 job_get_id_string(job_id, ja_task_id, pe_task_id), 
                 sge_sys_sig2str(signo), signo);

         DPRINTF(("%s\n", error));
         failed = SSTATE_FAILURE_AFTER_JOB;

         if ((sge_signo=sge_map_signal(signo)) != -1)
            lSetDouble(du, UA_value, (double)sge_signo);
      }
   }
      
   if (du && ((u_long32)lGetDouble(du, UA_value) == 0xffffffff)) {
      if (!failed) {
         failed = SSTATE_FAILURE_AFTER_JOB;
         if (!*error)
            sprintf(error, MSG_JOB_CANTREADUSEDRESOURCESFORJOB);
      }
   }

   DTRACE;

   /* Be careful: the checkpointing checking is done at the end. It will
    * often override other failure states.
    * If the job finishes, the shepherd must remove the "checkpointed" file
    */  

   sge_get_active_job_file_path(&fname,
                                job_id, ja_task_id, pe_task_id, "checkpointed");
   ckpt_arena = 1;   /* 1 job will be restarted in case of failure *
                      * 2 job will be restarted from ckpt arena    */
   if (!SGE_STAT(sge_dstring_get_string(&fname), &statbuf)) {
      int dummy;

      failed = SSTATE_MIGRATE;
      if ((fp = fopen(sge_dstring_get_string(&fname), "r"))) {
         DPRINTF(("found checkpointed file\n"));
         if (fscanf(fp, "%d", &dummy)==1) {
            DPRINTF(("need restart from ckpt arena\n"));
            ckpt_arena = 2;
         }
         fclose(fp);
      }   

   sge_get_active_job_file_path(&fname, job_id, ja_task_id, pe_task_id, 
                                "job_pid");
      if (!SGE_STAT(sge_dstring_get_string(&fname), &statbuf)) {
         if ((fp = fopen(sge_dstring_get_string(&fname), "r"))) {
            if (!fscanf(fp, u32 , &job_pid))
               job_pid = 0;
            fclose(fp);
         }
         else {
            job_pid = 0;
            ERROR((SGE_EVENT, MSG_JOB_CANTOPENJOBPIDFILEFORJOBXY_S, 
                   job_get_id_string(job_id, ja_task_id, pe_task_id)));
         }
      }
   }
   else
      job_pid = 0;

   lSetUlong(jr, JR_job_pid, job_pid);
   
   /* Only used for ckpt jobs: 1 checkpointed, 2 checkpoint in the arena */
   lSetUlong(jr, JR_ckpt_arena, ckpt_arena);
   

   /* Currently the shepherd doesn't create this file */
   sge_get_active_job_file_path(&fname, job_id, ja_task_id, pe_task_id, 
                                "noresources");
   if (!SGE_STAT(sge_dstring_get_string(&fname), &statbuf))
      failed = SSTATE_AGAIN;
   
   /* failed */
   lSetUlong(jr, JR_failed, failed);
   DPRINTF(("job report for job "SFN": failed = %ld\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id), 
            failed));
   /* err_str */
   if (*error) {
      lSetString(jr, JR_err_str, error);
      DPRINTF(("job report for job "SFN": err_str = %s\n", 
               job_get_id_string(job_id, ja_task_id, pe_task_id),
               error));
   }

   /* general_failure */
   switch (failed) {
   case SSTATE_BEFORE_PROLOG:
      general_failure = GFSTATE_HOST;
      lSetUlong(jr, JR_general_failure, general_failure);
      job_related_adminmail(jr, is_array);
      break;
   case SSTATE_PROCSET_NOTSET:
   case SSTATE_READ_CONFIG:
   case SSTATE_PROLOG_FAILED:
   case SSTATE_BEFORE_PESTART:
   case SSTATE_PESTART_FAILED:
      general_failure = GFSTATE_QUEUE;
      lSetUlong(jr, JR_general_failure, general_failure);
      job_related_adminmail(jr, is_array);
      break;
   case SSTATE_BEFORE_JOB:
   case SSTATE_NO_SHELL:
      {
         int job_caused_failure = 0;
         lListElem *job = NULL; 
         lListElem *ja_task = NULL;
         lListElem *master_queue = NULL;
         const void *iterator = NULL;

         /* Bugfix: Issuezilla 1031/1034
          * The problem in 1031 is that each task got added as its own job
          * structure, but the reaper was only looking at the first job
          * structure in the list.  Instead, we have to iterate through the
          * list by hand to make sure we find every instance. */

         job = lGetElemUlongFirst(Master_Job_List, JB_job_number, job_id, &iterator);
         while(job != NULL) {
            ja_task = job_search_task(job, NULL, ja_task_id);
            if(ja_task != NULL) {
               break;
            }
            job = lGetElemUlongNext(Master_Job_List, JB_job_number, job_id, 
                                    &iterator);
         }

         if ((job != NULL) && (ja_task != NULL)) {
            master_queue = responsible_queue(job, ja_task, NULL);
         }

         if ((failed == SSTATE_NO_SHELL) && (job != NULL) && 
             ((lGetList(job, JB_shell_list) != NULL) ||
              ((master_queue != NULL) &&
               JOB_TYPE_IS_BINARY(lGetUlong(job, JB_type)) &&
               JOB_TYPE_IS_NO_SHELL(lGetUlong(job, JB_type))))) {
            job_caused_failure = 1;
         } else if ((failed == SSTATE_NO_SHELL) && (master_queue != NULL)) {
            const char *mode = job_get_shell_start_mode(job, master_queue, 
                                                   conf.shell_start_mode);

            if (!strcmp(mode, "unix_behavior") != 0) {
               job_caused_failure = 1;
            }
         } else if ((failed == SSTATE_BEFORE_JOB) && (job != NULL) &&
                  JOB_TYPE_IS_BINARY(lGetUlong(job, JB_type)) &&
                  !sge_is_file(lGetString(job, JB_script_file))) {
            job_caused_failure = 1;
         }

         general_failure = job_caused_failure ? GFSTATE_JOB : GFSTATE_QUEUE;
         lSetUlong(jr, JR_general_failure, general_failure);
         job_related_adminmail(jr, is_array);
      }
      break;
   /*
   ** if an error occurred where the job 
   ** is source of the failure 
   */
   case SSTATE_OPEN_OUTPUT:
   case SSTATE_NO_CWD:
   case SSTATE_AFS_PROBLEM:
   case SSTATE_APPERROR:
      general_failure = GFSTATE_JOB;
      lSetUlong(jr, JR_general_failure, general_failure);
      job_related_adminmail(jr, is_array);
      break;
   /*
   ** if an error occurred after the job has been run
   ** it is not as serious
   */
   case SSTATE_BEFORE_PESTOP:
   case SSTATE_PESTOP_FAILED:
   case SSTATE_BEFORE_EPILOG:
   case SSTATE_EPILOG_FAILED:
   case SSTATE_PROCSET_NOTFREED:
      general_failure = 0;
      lSetUlong(jr, JR_general_failure, general_failure);
      job_related_adminmail(jr, is_array);
      break;
   /*
   ** these are shepherd error conditions met by the execd
   */
   case ESSTATE_NO_CONFIG:
   case ESSTATE_NO_PID:
   case ESSTATE_DIED_THRU_SIGNAL:
   case ESSTATE_SHEPHERD_EXIT:
   case ESSTATE_NO_EXITSTATUS:
   case ESSTATE_UNEXP_ERRORFILE:
   case ESSTATE_UNKNOWN_JOB:
      /*
      ** test for admin mail here
      */
      general_failure = 0;
      lSetUlong(jr, JR_general_failure, general_failure);
      job_related_adminmail(jr, is_array);
      break;
   default: 
      general_failure = 0;
      /*
      ** this is no error, because not all failures apply to general_failure
      */
      break;
   } /* switch */
   
   lSetUlong(jr, JR_general_failure, general_failure);
   DPRINTF(("job report for job "SFN": general_failure = %ld\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id),
            general_failure));

   /* Check whether a message about tasks exit has to be sent.
      We need job data to know whether an ack about exit status was requested */
   /* JG: TODO (257): This message will never be read by anybody. 
    *           We should store the host and port of qrsh -inherit in PET_source
    *           and send data there, or, perhaps better, always use comlib for 
    *           communication of qrsh with other components instead of qrsh socket.
    */
   if (pe_task_id != NULL) {
#if 0
      lListElem *petep = NULL, *uep = NULL;
#endif
      lListElem *jep = NULL, *jatep = NULL;
      const void *iterator;

      jep = lGetElemUlongFirst(Master_Job_List, JB_job_number, job_id, &iterator);
      while(jep != NULL) {
         jatep = job_search_task(jep, NULL, ja_task_id);
         if(jatep != NULL) {
            break;
         }
         jep = lGetElemUlongNext(Master_Job_List, JB_job_number, job_id, &iterator);
      }

      /* CR: TODO: This code is not active because there is no one who calls 
       *           sge_qwaittid() to receive task exit message. Activate this
       *           code when sge_qwaittid() is needed.
       */
#if 0
      if (jatep && (petep = lGetSubStr(jatep, PET_id, pe_task_id, JAT_task_list))) {
         const char *host, *commproc;
         u_short id;
         sge_pack_buffer pb;
         u_long32 exit_status = 1;
         u_long32 dummymid = 0;
         int ret;

         /* extract address from JB_job_source */
         host = sge_strtok(lGetString(petep, PET_source), ":"); 
         commproc = sge_strtok(NULL, ":"); 
         id = atoi(sge_strtok(NULL, ":")); 

         /* extract exit status from job report */
         if ((uep=lGetSubStr(jr, UA_name, "exit_status", JR_usage)))
            exit_status = lGetDouble(uep, UA_value);         
        
         /* fill in pack buffer */
         if(init_packbuffer(&pb, 256, 0) == PACK_SUCCESS) {
            packstr(&pb, pe_task_id);
            packint(&pb, exit_status);

            /* send a task exit message to the submitter of this task */
            ret=gdi_send_message_pb(0, commproc, id, host, TAG_TASK_EXIT, &pb, &dummymid);
            DPRINTF(("%s sending task exit message for pe-task \"%s\" to %s: %s\n",
                  (ret!=CL_RETVAL_OK)?"failed":"success", pe_task_id, 
                  lGetString(petep, PET_source), cl_get_error_text(ret)));
            clear_packbuffer(&pb);
         }
      }
#endif
   }

   sge_dstring_free(&fname);
   sge_dstring_free(&jobdir);

   DEXIT;
   return 0;
}

/* ------------------------- */
void remove_acked_job_exit(
u_long32 job_id,
u_long32 ja_task_id,
const char *pe_task_id,
lListElem *jr 
) {
   char *exec_file, *script_file, *tmpdir, *job_owner, *qname; 
   dstring jobdir = DSTRING_INIT;
   char fname[SGE_PATH_MAX];
   char err_str_buffer[1024];
   dstring err_str;
   SGE_STRUCT_STAT statbuf;
   lListElem *jep, *petep = NULL, *jatep = NULL;
   lListElem *master_q;
   const char *pe_task_id_str; 
   const void *iterator;

   DENTER(TOP_LAYER, "remove_acked_job_exit");

   sge_dstring_init(&err_str, err_str_buffer, sizeof(err_str_buffer));

   if (ja_task_id == 0) {
      ERROR((SGE_EVENT, MSG_SHEPHERD_REMOVEACKEDJOBEXITCALLEDWITHX_U, u32c(job_id)));
      DEXIT;
      return;
   }

   pe_task_id_str = jr?lGetString(jr, JR_pe_task_id_str):NULL;

   /* try to find this job in our job list */ 

   jep = lGetElemUlongFirst(Master_Job_List, JB_job_number, job_id, &iterator);
   while(jep != NULL) {
      jatep = job_search_task(jep, NULL, ja_task_id);
      if(jatep != NULL) {
         break;
      }
      jep = lGetElemUlongNext(Master_Job_List, JB_job_number, job_id, &iterator);
   }

   if (jep && jatep) {
      int used_slots;
   
      DPRINTF(("REMOVING WITH jep && jatep\n"));
      if (pe_task_id_str) {
         for_each(petep, lGetList(jatep, JAT_task_list)) {
            if (!strcmp(pe_task_id_str, lGetString(petep, PET_id))) {
               break;  
            }
         }   

         if (!petep) {
            ERROR((SGE_EVENT, MSG_JOB_XYHASNOTASKZ_UUS, 
                   u32c(job_id), u32c(ja_task_id), pe_task_id_str));
            del_job_report(jr);
            DEXIT;
            return;
         }

         if (lGetUlong(jr, JR_state)!=JEXITING) {
            WARNING((SGE_EVENT, MSG_EXECD_GOTACKFORPETASKBUTISNOTINSTATEEXITING_S, pe_task_id_str));
            DEXIT;
            return;
         }

         master_q = responsible_queue(jep, jatep, petep);
      } else { 
         master_q = responsible_queue(jep, jatep, NULL);
      }   
     
      /* use mail list of job instead of tasks one */
      if (jr && lGetUlong(jr, JR_state)!=JSLAVE)
         reaper_sendmail(jep, jr); 


      /*
      ** security hook
      */
#ifdef KERBEROS
      /* destroy credentials cache of job */
      if (!pe_task_id_str)
         krb_destroy_forwarded_tgt(job_id);
#endif
      /*
      ** Execute command to delete the client's DCE or Kerberos credentials.
      */
      if (do_credentials)
         delete_credentials(jep);

      /* remove job/task active dir */
      if (!keep_active && !getenv("SGE_KEEP_ACTIVE")) {
         sge_get_active_job_file_path(&jobdir,
                                      job_id, ja_task_id, pe_task_id,
                                      NULL);
         DPRINTF(("removing active dir: %s\n", sge_dstring_get_string(&jobdir)));
         if (sge_rmdir(sge_dstring_get_string(&jobdir), &err_str)) {
            ERROR((SGE_EVENT, MSG_FILE_CANTREMOVEDIRECTORY_SS,
                   sge_dstring_get_string(&jobdir), err_str_buffer));
         }
      }

      /* increment # of free slots. In case no slot is used any longer 
         we have to remove queues tmpdir for this job */
      used_slots = qinstance_slots_used(master_q) - 1;
      qinstance_set_slots_used(master_q, used_slots);
      if (!used_slots) {
         sge_switch2start_user();
         sge_remove_tmpdir(lGetString(master_q, QU_tmpdir), 
            lGetString(jep, JB_owner), lGetUlong(jep, JB_job_number), 
            ja_task_id, lGetString(master_q, QU_qname));
         sge_switch2admin_user();
      }

      if (!pe_task_id_str) {
         job_remove_spool_file(job_id, ja_task_id, NULL, SPOOL_WITHIN_EXECD);

         if (!JOB_TYPE_IS_BINARY(lGetUlong(jep, JB_type)) &&
             lGetString(jep, JB_exec_file)) {
            int task_number = 0;
            lListElem *tmp_job = NULL;

            /* it is possible to remove the exec_file if
               less than one task of a job is running */
            tmp_job = lGetElemUlongFirst(Master_Job_List, JB_job_number, job_id, &iterator);
            while(tmp_job != NULL) {
               task_number++;
               tmp_job = lGetElemUlongNext(Master_Job_List, JB_job_number, job_id, &iterator);
            }
            
            if (task_number <= 1) {
               DPRINTF(("unlinking script file %s\n", lGetString(jep, JB_exec_file)));
               unlink(lGetString(jep, JB_exec_file));
            }
         }
      } else {
         DPRINTF(("not removing job file: pe_task_id_str = %s\n", 
            pe_task_id_str));
      }



      if (pe_task_id_str) {
         /* unchain pe task element from task list */
         lRemoveElem(lGetList(jatep, JAT_task_list), petep);
      } else {
         lRemoveElem(Master_Job_List, jep);
      }
      del_job_report(jr);   

   } else { /* must be an ack of an ask job request from qmaster */

      DPRINTF(("REMOVING WITHOUT jep && jatep\n"));
      /* clean up active jobs entry */
      if (!pe_task_id_str) {
         ERROR((SGE_EVENT, MSG_SHEPHERD_ACKNOWLEDGEFORUNKNOWNJOBXYZ_UUS, 
                u32c(job_id),  u32c(ja_task_id), 
                (pe_task_id_str ? pe_task_id_str : MSG_MASTER)));

      /*
      ** security hook
      */
#ifdef KERBEROS
         krb_destroy_forwarded_tgt(job_id);
#endif
         sge_get_active_job_file_path(&jobdir,
                                      job_id, ja_task_id, pe_task_id,
                                      NULL);
         if (SGE_STAT(sge_dstring_get_string(&jobdir), &statbuf)) {
            ERROR((SGE_EVENT, MSG_SHEPHERD_CANTFINDACTIVEJOBSDIRXFORREAPINGJOBY_SU, 
                  sge_dstring_get_string(&jobdir), u32c(job_id)));
         } else {
            /*** read config file written by exec_job ***/ 
            sprintf(fname, "%s/config", sge_dstring_get_string(&jobdir));
            if (read_config(fname)) {
               /* This should happen very rarely. exec_job() should avoid this 
                  condition as far as possible. One possibility for this case is, 
                  that the execd dies just after making the jobs active directory. 
                  The pain with this case is, that we have not much information
                  to report this job to qmaster. */

               if (sge_rmdir(sge_dstring_get_string(&jobdir), &err_str)) {
                  ERROR((SGE_EVENT, MSG_FILE_CANTREMOVEDIRECTORY_SS,
                         sge_dstring_get_string(&jobdir), err_str_buffer));
               }
            }

            /* do not remove xterm or qlogin starter ! */
            if ((script_file = get_conf_val("script_file")) 
                  && strcasecmp(script_file, "INTERACTIVE")
                  && strcasecmp(script_file, "QLOGIN")) {
               if ((exec_file = get_conf_val("exec_file"))) {
                  DPRINTF(("removing exec_file %s\n", exec_file));
                  unlink(exec_file);
               }
            }
            
            /* tmpdir */
            if ((!(tmpdir = get_conf_val("queue_tmpdir"))) || 
                 (!(qname = get_conf_val("queue"))) || 
             (!(job_owner = get_conf_val("job_owner")))) {
               ERROR((SGE_EVENT, MSG_SHEPHERD_INCORRECTCONFIGFILEFORJOBXY_UU, 
                     u32c(job_id), u32c(ja_task_id)));
            } else {
               DPRINTF(("removing queue_tmpdir %s\n", tmpdir));
               sge_remove_tmpdir(tmpdir, job_owner, job_id, ja_task_id, qname);
            }
         }

         /* job */
         job_remove_spool_file(job_id, ja_task_id, NULL, SPOOL_WITHIN_EXECD); 

         /* active dir */
         if (!keep_active && !getenv("SGE_KEEP_ACTIVE")) {
            DPRINTF(("removing active dir: %s\n", sge_dstring_get_string(&jobdir)));
            if (sge_rmdir(sge_dstring_get_string(&jobdir), &err_str)) {
               ERROR((SGE_EVENT, MSG_FILE_CANTREMOVEDIRECTORY_SS,
                      sge_dstring_get_string(&jobdir), err_str_buffer));
            }
         }
      }
      del_job_report(jr);
      cleanup_job_report(job_id, ja_task_id);
   }

   sge_dstring_free(&jobdir);

   DEXIT;
   return;
}

/**************************************************************************
 This function is called if we fail to start a job. 
 Returns the appropriate job report.

 We clean up the job.
 We report the job_number and the error to the master.
 If general is set we have a general problems starting jobs.
 The master should not give us jobs in the future and the administrator
 should be informed about the problem (e.g. we cant write to a filesystem).
 **************************************************************************/
lListElem *execd_job_start_failure(
lListElem *jep,
lListElem *jatep,
lListElem *petep,
const char *error_string,
int general 
) {
   return execd_job_failure(jep, jatep, petep, error_string, general, SSTATE_FAILURE_BEFORE_JOB);
}

lListElem *execd_job_run_failure(
lListElem *jep,
lListElem *jatep,
lListElem *petep,
const char *error_string,
int general 
) {
   return execd_job_failure(jep, jatep, petep, error_string, general, SSTATE_FAILURE_AFTER_JOB);
}

static lListElem *execd_job_failure(
lListElem *jep,
lListElem *jatep,
lListElem *petep,
const char *error_string,
int general,
int failed 
) {
   lListElem *jr, *ep;
   u_long32 jobid, jataskid;
   const char *petaskid = NULL;

   DENTER(TOP_LAYER, "execd_job_failure");

   jobid = lGetUlong(jep, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);
   if(petep != NULL) {
      petaskid = lGetString(petep, PET_id);
   }

   ERROR((SGE_EVENT, 
      (failed==SSTATE_FAILURE_BEFORE_JOB)?
         MSG_SHEPHERD_CANTSTARTJOBXY_US:
         MSG_SHEPHERD_PROBLEMSAFTERSTART_DS, u32c(jobid), error_string));

   jr = get_job_report(jobid, jataskid, petaskid);
   if (!jr) {
      DPRINTF(("no job report found to report job start failure!\n"));
      jr = add_job_report(jobid, jataskid, petaskid, jep);
   }

   if(petep != NULL) {
      ep = lFirst(lGetList(petep, PET_granted_destin_identifier_list));
   } else {
      ep = lFirst(lGetList(jatep, JAT_granted_destin_identifier_list));
   }

   if (ep != NULL) {
      lSetString(jr, JR_queue_name, lGetString(ep, JG_qname));
      lSetHost(jr, JR_host_name,  lGetHost(ep, JG_qhostname));
   }
      
   lSetUlong(jr, JR_failed, failed);
   lSetUlong(jr, JR_general_failure, general);
   lSetString(jr, JR_err_str, error_string);
  
   lSetUlong(jr, JR_state, JEXITING);

   job_related_adminmail(jr, job_is_array(jep));

   DEXIT;
   return jr;
}


/**************************************************************************
 This function is called if we are asked by the master concerning a job
 we dont know anything about. We have to tell this to the qmaster, so that
 he can clean up this job.
 This is done very like the normal job finish and runs into the same
 functions in the qmaster.
 **************************************************************************/
void job_unknown(
u_long32 jobid,
u_long32 jataskid,
char *qname 
) {
   lListElem *jr;

   DENTER(TOP_LAYER, "job_unknown");

   ERROR((SGE_EVENT, MSG_SHEPHERD_JATASKXYISKNOWNREPORTINGITTOQMASTER, 
     u32c(jobid), u32c(jataskid)));

   jr = add_job_report(jobid, jataskid, NULL, NULL);
   if (jr) {
      lSetString(jr, JR_queue_name, qname);
      lSetUlong(jr, JR_failed, ESSTATE_UNKNOWN_JOB); 
      lSetUlong(jr, JR_state, JEXITING);
      lSetString(jr, JR_err_str, (char*) MSG_JR_ERRSTR_EXECDDONTKNOWJOB);
   }

   DEXIT;
   return;
}

/************************************************************************
 Look for old jobs hanging around on disk and report them to the qmaster.
 This is used at execd startup time.
 We have to call it cyclic cause there may be jobs alive while execd went
 down and up. If such a job exits we get no SIGCLD from shepherd.
 If startup is true this is the first call of the execd. We produce more
 output for the administrator the first time.
 ************************************************************************/
int clean_up_old_jobs(
int startup 
) {
   SGE_STRUCT_DIRENT *dent;
   DIR *cwd;
   char dir[SGE_PATH_MAX];
   pid_t pids[10000];   /* a bunch of processes */
   int npids;           /* number of running processes */
   char *jobdir;
   u_long32 jobid, jataskid;
   static int lost_children = 1;
   lListElem *jep, *petep, *jatep = NULL;
   char *cp;

   DENTER(TOP_LAYER, "clean_up_old_jobs");

   if (startup)
      INFO((SGE_EVENT, MSG_SHEPHERD_CKECKINGFOROLDJOBS));

   /* 
      If we get an empty Master_Job_List we know that 
      it is no longer necessary to pass this code 
     
      Getting job information by parsing ps-output 
      is very expensive. The aim is to get informed
      about jobs that were started by the execd 
      running before we were started. Jobs that
      were started by us are our childs and we
      get a cheap SIGCLD informing us about the
      jobs exit. 
       
      So if we arrive here and an empty Master_Job_List
      we know all jobs that were our "lost children" 
      exited and there is no need for ps-commands.

   */
   if (!Master_Job_List || !lGetNumberOfElem(Master_Job_List) || !lost_children) {
      if (lost_children) {
         INFO((SGE_EVENT, MSG_SHEPHERD_NOOLDJOBSATSTARTUP));
         lost_children = 0;
      }
      /* all children exited */
      DEXIT;
      return 0;
   }

   /* Get pids of running jobs. So we can look for running shepherds. */
   cp = SGE_SHEPHERD;
   npids = sge_get_pids(pids, 10000, cp, PSCMD);
   if (npids == -1) {
      ERROR((SGE_EVENT, MSG_SHEPHERD_CANTGETPROCESSESFROMPSCOMMAND));
      DEXIT;
      return -1;
   }

   DPRINTF(("found %d running processes\n", npids));

   /* We read the job information from active dir. There is one subdir for each
      started job. Shepherd writes exit_status and usage to this directory.
      Cause maybe shepherd was killed while execd was down we have to look into
      the process table too. */

   if (!(cwd=opendir(ACTIVE_DIR))) {
      ERROR((SGE_EVENT, MSG_FILE_CANTOPENDIRECTORYX_SS, ACTIVE_DIR, strerror(errno)));
      DEXIT;
      return -1;
   }

   while ((dent=SGE_READDIR(cwd))) {
      char string[256], *token, *endp;
      u_long32 tmp_id;
      const void *iterator;

      jobdir = dent->d_name;    /* jobdir is the jobid.jataskid converted to string */
      strcpy(string, jobdir);

      if (!strcmp(jobdir, ".") || !strcmp(jobdir, "..") )
         continue;

      jobid = jataskid = 0;
      if ((token = strtok(string, " ."))) {
         tmp_id = strtol(token, &endp, 10);
         if (*endp == '\0') {
            jobid = tmp_id;
            if ((token = strtok(NULL, " \n\t"))) {
               tmp_id = strtol(token, &endp, 10);
               if (*endp == '\0') {
                  jataskid = tmp_id;
               }
            }
         }
      }

      if (!jobid || !jataskid) {
         /* someone left his garbage in our directory */
         WARNING((SGE_EVENT, MSG_SHEPHERD_XISNOTAJOBDIRECTORY_S, jobdir)); 
         continue;
      }

      /* seek job to this jobdir */
      jep = lGetElemUlongFirst(Master_Job_List, JB_job_number, jobid, &iterator);
      while(jep != NULL) {
         jatep = job_search_task(jep, NULL, jataskid);
         if(jatep != NULL) {
            break;
         }
         jep = lGetElemUlongNext(Master_Job_List, JB_job_number, jobid, &iterator);
      }
 
      if (!jep || !jatep) {
         /* missing job in job dir but not in active job dir */
         if (startup) {
            ERROR((SGE_EVENT, MSG_SHEPHERD_FOUNDACTIVEJOBDIRXWHILEMISSINGJOBDIRREMOVING_S, jobdir)); 
         }
         /* remove active jobs directory */
         DPRINTF(("+++++++++++++++++++ remove active jobs directory ++++++++++++++++++\n"));
         {
            char path[SGE_PATH_MAX];
            sprintf(path, ACTIVE_DIR"/%s", jobdir);
            sge_rmdir(path, NULL);
         }
         continue;
      }
      if (lGetUlong(jatep, JAT_status) != JSLAVE) {
         sprintf(dir, "%s/%s", ACTIVE_DIR, jobdir);
         examine_job_task_from_file(startup, dir, jep, jatep, NULL, pids, npids);
      }
      for_each(petep, lGetList(jatep, JAT_task_list)) {
         sprintf(dir, "%s/%s/%s", ACTIVE_DIR, jobdir, lGetString(petep, PET_id));
         examine_job_task_from_file(startup, dir, jep, jatep, petep, pids, npids);
      }
   }    /* while (dent=SGE_READDIR(cwd)) */

   closedir(cwd);

   DEXIT;
   return 0;
}

static void examine_job_task_from_file(
   
int startup,
char *dir,
lListElem *jep,
lListElem *jatep,
lListElem *petep,
pid_t *pids,
int npids 
) {
   lListElem *jr;
   int shepherd_alive;  /* =1 -> this shepherd is in the process table */
   FILE *fp;
   SGE_STRUCT_STAT statbuf;
   char fname[SGE_PATH_MAX];
   pid_t pid;           /* pid of shepherd */
   char err_str[1024];
   u_long32 jobid, jataskid;
   const char *pe_task_id_str = NULL;
   static u_long32 startup_time = 0;

   DENTER(TOP_LAYER, "examine_job_task_from_file");
   
   if (!startup_time)
      startup_time = sge_get_gmt();
   
   jobid = lGetUlong(jep, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);
   if(petep != NULL) {
      pe_task_id_str = lGetString(petep, PET_id);
   }

   if (SGE_STAT(dir, &statbuf)) {
      ERROR((SGE_EVENT, MSG_SHEPHERD_CANTSTATXY_SS, dir, strerror(errno)));
      DEXIT;
      return;
   }

   if (!(statbuf.st_mode && S_IFDIR)) {
      ERROR((SGE_EVENT, MSG_FILE_XISNOTADIRECTORY_S, dir));
      DEXIT;
      return;
   }

   DPRINTF(("Found job directory: %s\n", dir));
   if (startup)
      INFO((SGE_EVENT, MSG_SHEPHERD_FOUNDDIROFJOBX_S, dir));

   /* Look for pid of shepherd */
   sprintf(fname, "%s/pid", dir);
   if (!(fp = fopen(fname, "r"))) {
      /* Is it 
            1. a job started before startup of execd
               In this case the job was started by the old execd 
               and there must be a pid fild -> Logging
         or is it 
            2. a newly started job
               In this case the shepherd had not enough time 
               to write the pid file -> No Logging */
      if (startup && startup_time >= lGetUlong(jatep, JAT_start_time)) {
         ERROR((SGE_EVENT, MSG_SHEPHERD_CANTREADPIDFILEXFORJOBYSTARTTIMEZX_SSUS,
                fname, dir, u32c(lGetUlong(jatep, JAT_start_time)), strerror(errno)));
         /* seek job report for this job - it must be contained in job report
            If this is a newly started execd we can assume the execd was broken
            in the interval between making the jobs active directory and writing
            the shepherds pid (done by the started shepherd). So we just remove
            and report this job. */
         if (!(jr=get_job_report(jobid, jataskid, pe_task_id_str))) {
            CRITICAL((SGE_EVENT, MSG_SHEPHERD_MISSINGJOBXINJOBREPORTFOREXITINGJOB_U, u32c(jobid)));
            jr = add_job_report(jobid, jataskid, pe_task_id_str, NULL);
         }
         lSetUlong(jr, JR_state, JEXITING);
         clean_up_job(jr, ESSTATE_NO_PID, 0, job_is_array(jep),
                      lGetObject(jatep, JAT_pe_object));  /* failed before execution */
      }
      DEXIT;
      return;
   }
   
   if (fscanf(fp, pid_t_fmt, &pid) != 1) {
      /* most probably a newly started job
         shepherd usually just had not enough time for writing the pid file 
         if these warnings do appear frequently one might consider having
         execd (and thus the shepherds) do local spooling instead of via NFS */
      WARNING((SGE_EVENT, MSG_SHEPHERD_CANTREADPIDFROMPIDFILEXFORJOBY_SS,
                  fname, dir));
      fclose(fp);
      DEXIT;
      return;
   }
   fclose(fp);

   /* look whether shepherd is still alive */ 
   shepherd_alive = sge_contains_pid(pid, pids, npids);

   /* report this information */
   sprintf(err_str, MSG_SHEPHERD_SHEPHERDFORJOBXHASPIDYANDISZALIVE_SUS, 
           dir, u32c(pid), (shepherd_alive ? "": MSG_NOT));
   if (startup) {
      INFO((SGE_EVENT, err_str));
   }
   else {
      DPRINTF((err_str));
   }

   if (shepherd_alive) {     /* shepherd alive -> nothing to do */
      if (startup) {
         /*  
            at startup we need to change the 
            state of not exited jobs from JWRITTEN 
            to JRUNNING
         */ 
         if ((jr=get_job_report(jobid, jataskid, pe_task_id_str))) {

            lSetUlong(jr, JR_state, JRUNNING);
            /* here we will call a ptf function to get */
            /* the first usage data after restart      */ 
         } else {
            /* found job in active jobs directory 
               but not in spool directory of execd */
            ERROR((SGE_EVENT, MSG_SHEPHERD_INCONSISTENTDATAFORJOBX_U, u32c(jobid)));
            jr = add_job_report(jobid, jataskid, pe_task_id_str, NULL);
            lSetUlong(jr, JR_state, JEXITING);
         }
      }
      DEXIT;
      return;
   }

   /* seek job report for this job - it must be contained in job report */
   if (!(jr=get_job_report(jobid, jataskid, pe_task_id_str))) {
      CRITICAL((SGE_EVENT, MSG_SHEPHERD_MISSINGJOBXYINJOBREPORT_UU, u32c(jobid), u32c(jataskid)));
      jr = add_job_report(jobid, jataskid, pe_task_id_str, jep);
      DEXIT;
      return;
   }

   /* if the state is already JEXITING work is done  
      for this job and we wait for ACK from qmaster */
   if (lGetUlong(jr, JR_state)==JEXITING) {
      DPRINTF(("State of job "u32"."u32" already changed to JEXITING\n", jobid, jataskid));
      DEXIT;
      return;
   }

   clean_up_job(jr, 0, 0, job_is_array(jep), lGetObject(jatep, JAT_pe_object));
   lSetUlong(jr, JR_state, JEXITING);
   
   flush_jr = 1;  /* trigger direct sending of job reports */

   DEXIT;
   return;
}

/************************************************************/
/* fill dusage with:                                        */ 
/* - data retrieved from config - file                      */
/* - data retrieved from "usage" as shepherd has written it */
/*   if we cant read "usage" exit_status = 0xffffffff       */ 
/************************************************************/
static int read_dusage(
lListElem *jr,
const char *jobdir,
u_long32 jobid,
u_long32 jataskid,
int failed,
int usage_mul_factor
) {
   char pid_file[SGE_PATH_MAX];
   FILE *fp;
   u_long32 pid;

   DENTER(TOP_LAYER, "read_dusage");

   pid = 0xffffffff;

   sprintf(pid_file, "%s/pid", jobdir);

   if (failed != ESSTATE_NO_PID) {
      fp = fopen(pid_file, "r");
      if (fp) {
         fscanf(fp, u32 , &pid);
         fclose(fp);
      }
      else
         ERROR((SGE_EVENT, MSG_SHEPHERD_CANTOPENPIDFILEXFORJOBYZ_SUU,
                pid_file, u32c(jobid), u32c(jataskid)));
   }

   if (failed != ESSTATE_NO_CONFIG) {
      dstring buffer = DSTRING_INIT;
      const char *qinstance_name = NULL;
      char *owner;
   
      qinstance_name = sge_dstring_sprintf(&buffer, SFN"@"SFN,
                                           get_conf_val("queue"),
                                           get_conf_val("host"));
      lSetString(jr, JR_queue_name, qinstance_name);
      qinstance_name = NULL;
      sge_dstring_free(&buffer);

      lSetHost(jr, JR_host_name, get_conf_val("host"));
      lSetString(jr, JR_owner, owner = get_conf_val("job_owner"));
      if (owner) {
         struct passwd *pw;
         struct group *pg;

         pw = sge_getpwnam(owner);
         if (pw) {
            if (use_qsub_gid) {
               char *tmp_qsub_gid = search_conf_val("qsub_gid");
               pw->pw_gid = atol(tmp_qsub_gid);
            } 
            pg = getgrgid(pw->pw_gid);
            if (pg && pg->gr_name)
               lSetString(jr, JR_group, pg->gr_name);
         }
      }

      add_usage(jr, "submission_time", get_conf_val("submission_time"), (double)0);
      add_usage(jr, "priority",        get_conf_val("priority"),        (double)0);
   }

   /* read "usage" file */
   /*
   ** there may be more failure states where there is no usage file
   ** but it is best if we try to read it and ignore the error if reading failed
   ** if we have an error already
   */
   if (!failed || (failed > SSTATE_BEFORE_JOB)) {
      char usage_file[SGE_PATH_MAX];
      sprintf(usage_file, "%s/usage", jobdir);
      fp = fopen(usage_file, "r");
      if (fp) {
         char buf[10000];
         lList *cflp = NULL;
         u_long32 wait_status;

         read_config_list(fp, &cflp, NULL, CF_Type, CF_name, CF_value, 0, "=", 0, buf, sizeof(buf));
         fclose(fp);

         if (extract_ulong_attribute(&cflp, "wait_status", &wait_status)==0)
            lSetUlong(jr, JR_wait_status, wait_status);

         convert_attribute(&cflp, jr, "exit_status",   1);
         convert_attribute(&cflp, jr, "signal",        0);
         convert_attribute(&cflp, jr, "start_time",    0);
         convert_attribute(&cflp, jr, "end_time",      0);

         convert_attribute(&cflp, jr, "ru_wallclock",  0);

         convert_attribute(&cflp, jr, "ru_utime",      0);
         convert_attribute(&cflp, jr, "ru_stime",      0);
         convert_attribute(&cflp, jr, "ru_maxrss",     0);
         convert_attribute(&cflp, jr, "ru_ixrss",      0);
         convert_attribute(&cflp, jr, "ru_ismrss",     0);
         convert_attribute(&cflp, jr, "ru_idrss",      0);
         convert_attribute(&cflp, jr, "ru_isrss",      0);
         convert_attribute(&cflp, jr, "ru_minflt",     0);
         convert_attribute(&cflp, jr, "ru_majflt",     0);
         convert_attribute(&cflp, jr, "ru_nswap",      0);
         convert_attribute(&cflp, jr, "ru_inblock",    0);
         convert_attribute(&cflp, jr, "ru_oublock",    0);
         convert_attribute(&cflp, jr, "ru_msgsnd",     0);
         convert_attribute(&cflp, jr, "ru_msgrcv",     0);
         convert_attribute(&cflp, jr, "ru_nsignals",   0);
         convert_attribute(&cflp, jr, "ru_nvcsw",      0);
         convert_attribute(&cflp, jr, "ru_nivcsw",     0);


#ifdef NEC_ACCOUNTING_ENTRIES
         /* Additional accounting information for NEC SX-4 SX-5 */
#if defined(NECSX4) || defined(NECSX5)
#if defined(NECSX4)
         convert_attribute(&cflp, jr, "necsx_necsx4", 0);
#elif defined(NECSX5)
         convert_attribute(&cflp, jr, "necsx_necsx5", 0);
#endif
         convert_attribute(&cflp, jr, "necsx_base_prty", 0);
         convert_attribute(&cflp, jr, "necsx_time_slice", 0);
         convert_attribute(&cflp, jr, "necsx_num_procs", 0);
         convert_attribute(&cflp, jr, "necsx_kcore_min", 0);
         convert_attribute(&cflp, jr, "necsx_mean_size", 0);
         convert_attribute(&cflp, jr, "necsx_maxmem_size", 0);
         convert_attribute(&cflp, jr, "necsx_chars_trnsfd", 0);
         convert_attribute(&cflp, jr, "necsx_blocks_rw", 0);
         convert_attribute(&cflp, jr, "necsx_inst", 0);
         convert_attribute(&cflp, jr, "necsx_vector_inst", 0);
         convert_attribute(&cflp, jr, "necsx_vector_elmt", 0);
         convert_attribute(&cflp, jr, "necsx_vec_exe", 0);
         convert_attribute(&cflp, jr, "necsx_flops", 0);
         convert_attribute(&cflp, jr, "necsx_conc_flops", 0);
         convert_attribute(&cflp, jr, "necsx_fpec", 0);
         convert_attribute(&cflp, jr, "necsx_cmcc", 0);
         convert_attribute(&cflp, jr, "necsx_bccc", 0);
         convert_attribute(&cflp, jr, "necsx_mt_open", 0);
         convert_attribute(&cflp, jr, "necsx_io_blocks", 0);
         convert_attribute(&cflp, jr, "necsx_multi_single", 0);
         convert_attribute(&cflp, jr, "necsx_max_nproc", 0);
#endif
#endif   

         build_derived_final_usage(jr, usage_mul_factor);
         cflp = lFreeList(cflp);
      }
      else {
         ERROR((SGE_EVENT, MSG_SHEPHERD_CANTOPENUSAGEFILEXFORJOBYZX_SUUS,
                usage_file, u32c(jobid), u32c(jataskid), strerror(errno)));
         DEXIT;
         return -1;
      }
   }


#if 1
   {
      lListElem *ep;

      if (lGetList(jr, JR_usage)) {
         DPRINTF(("resulting usage attributes:\n"));
      } else {
         DPRINTF(("empty usage list\n"));
      }   

      for_each (ep, lGetList(jr, JR_usage)) {
         DPRINTF(("    \"%s\" = %f\n",
            lGetString(ep, UA_name),
            lGetDouble(ep, UA_value)));
      }
   }
#endif
   DEXIT;
   return 0;
}


static void build_derived_final_usage(lListElem *jr, int usage_mul_factor) 
{
   lList *usage_list;
   double ru_cpu, pdc_cpu;
   double cpu, r_cpu,
          mem, r_mem,
          io, iow, r_io, r_iow, maxvmem, r_maxvmem;
   double h_vmem = 0, s_vmem = 0;

   DENTER(TOP_LAYER, "build_derived_final_usage");

   parse_ulong_val(&h_vmem, NULL, TYPE_MEM, get_conf_val("h_vmem"), NULL, 0);
   parse_ulong_val(&s_vmem, NULL, TYPE_MEM, get_conf_val("s_vmem"), NULL, 0);
   h_vmem = MIN(s_vmem, h_vmem);

   usage_list = lGetList(jr, JR_usage);
   
   /* cpu    = MAX(sum of "ru_utime" and "ru_stime" , PDC "cpu" usage) */
   ru_cpu = usage_list_get_double_usage(usage_list, "ru_utime", 0) +
            usage_list_get_double_usage(usage_list, "ru_stime", 0);
   pdc_cpu = usage_list_get_double_usage(usage_list, USAGE_ATTR_CPU, 0);
   cpu = MAX(ru_cpu, pdc_cpu);

   /* r_cpu  = h_rt * usage_mul_factor
    * (see execd_get_acct_multiplication_factor) 
    */
   r_cpu = (usage_list_get_double_usage(usage_list, "end_time", 0) -
           usage_list_get_double_usage(usage_list, "start_time", 0)) *
           usage_mul_factor;

   /* mem    = PDC "mem" usage or zero */
   mem = usage_list_get_double_usage(usage_list, USAGE_ATTR_MEM, 0);

   /* r_mem  = r_cpu * h_vmem */
   if (h_vmem != DBL_MAX)
      r_mem = (r_cpu * h_vmem)/(1024*1024*1024);
   else
      r_mem = mem;

   /* io     = PDC "io" usage or zero */
   io = usage_list_get_double_usage(usage_list, USAGE_ATTR_IO, 0);

   /* r_io   = 0 */
   r_io = io;

   /* iow     = PDC "io wait time" or zero */
   iow = usage_list_get_double_usage(usage_list, USAGE_ATTR_IOW, 0);

   /* r_iow  = 0 */
   r_iow = iow;

   /* maxvmem */
   r_maxvmem = maxvmem = usage_list_get_double_usage(usage_list, USAGE_ATTR_MAXVMEM, 0);

   DPRINTF(("CPU/MEM/IO: M(%f/%f/%f) R(%f/%f/%f) acct: %s stree: %s\n",
         cpu, mem, io, r_cpu, r_mem, r_io,
         acct_reserved_usage?"R":"M", sharetree_reserved_usage?"R":"M"));

   if (acct_reserved_usage) {
      add_usage(jr, USAGE_ATTR_CPU_ACCT, NULL, r_cpu);
      add_usage(jr, USAGE_ATTR_MEM_ACCT, NULL, r_mem);
      add_usage(jr, USAGE_ATTR_IO_ACCT,  NULL, r_io);
      add_usage(jr, USAGE_ATTR_IOW_ACCT, NULL, r_iow);
      if (r_maxvmem != DBL_MAX)
         add_usage(jr, USAGE_ATTR_MAXVMEM_ACCT, NULL, r_maxvmem);
   } else {
      add_usage(jr, USAGE_ATTR_CPU_ACCT, NULL, cpu);
      add_usage(jr, USAGE_ATTR_MEM_ACCT, NULL, mem);
      add_usage(jr, USAGE_ATTR_IO_ACCT,  NULL, io);
      add_usage(jr, USAGE_ATTR_IOW_ACCT, NULL, iow);
      if (maxvmem != DBL_MAX)
         add_usage(jr, USAGE_ATTR_MAXVMEM_ACCT, NULL, maxvmem);
   }

   if (sharetree_reserved_usage) {
      add_usage(jr, USAGE_ATTR_CPU, NULL, r_cpu);
      add_usage(jr, USAGE_ATTR_MEM, NULL, r_mem);
      add_usage(jr, USAGE_ATTR_IO,  NULL, r_io);
      add_usage(jr, USAGE_ATTR_IOW, NULL, r_iow);
      if (r_maxvmem!= DBL_MAX)
         add_usage(jr, USAGE_ATTR_MAXVMEM, NULL, r_maxvmem);
   } else {
      add_usage(jr, USAGE_ATTR_CPU, NULL, cpu);
      add_usage(jr, USAGE_ATTR_MEM, NULL, mem);
      add_usage(jr, USAGE_ATTR_IO,  NULL, io);
      add_usage(jr, USAGE_ATTR_IOW, NULL, iow);
      if (maxvmem!= DBL_MAX)
         add_usage(jr, USAGE_ATTR_MAXVMEM, NULL, maxvmem);
   }

   DEXIT;
   return;
}

/*****************************************************************/
static void convert_attribute(
lList **cflpp,
lListElem *jr,
char *name,
u_long32 udefault 
) {
   const char *s;

   s = get_conf_value(NULL, *cflpp, CF_name, CF_value, name);
   add_usage(jr, name, s, (double)udefault);
   lDelElemStr(cflpp, CF_name, name);
}


/*****************************************************************/

static int extract_ulong_attribute(
lList **cflpp,
char *name,
u_long32 *valuep
) {
   const char *s;
   int ret;

   if (!(s = get_conf_value(NULL, *cflpp, CF_name, CF_value, name)))
      return -1;
   ret = sscanf(s, u32, valuep);
   lDelElemStr(cflpp, CF_name, name);
   return (ret == 1)?0:-1;
}


/* send mail to users if requested */
void reaper_sendmail(
lListElem *jep,
lListElem *jr 
) {
   lList *mail_users; 
   u_long32 mail_options; 
   char sge_mail_subj[1024];
   char sge_mail_body[10*2048];
   char sge_mail_start[128];
   char sge_mail_end[128];
   u_long32 jobid, taskid, failed, ru_utime, ru_stime, ru_wallclock;
   double ru_cpu = 0.0, ru_maxvmem = 0.0;
   int exit_status = -1, signo = -1;
   const char *q, *h, *u;
   lListElem *ep;
   const char *pe_task_id_str;
   dstring ds;
   char buffer[128];
   dstring cpu_string = DSTRING_INIT;
   dstring maxvmem_string = DSTRING_INIT;

   DENTER(TOP_LAYER, "reaper_sendmail");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   mail_users = lGetList(jep, JB_mail_list);
   mail_options = lGetUlong(jep, JB_mail_options); 
   pe_task_id_str = lGetString(jr, JR_pe_task_id_str);

   if (!(q=lGetString(jr, JR_queue_name)))
      q = MSG_MAIL_UNKNOWN_NAME;

   if (!(h=lGetHost(jr, JR_host_name)))
      h = uti_state_get_qualified_hostname();

   if (!(u=lGetString(jep, JB_owner)))
      u = MSG_MAIL_UNKNOWN_NAME;

   /* JG: TODO (397): Extend usage module: usage_list_get_ctime_usage() 
    *                 and use the other usage_list_get functions.
    */

   if ((ep=lGetSubStr(jr, UA_name, "start_time", JR_usage)))
      strcpy(sge_mail_start, sge_ctime((u_long32)lGetDouble(ep, UA_value), &ds));
   else   
      strcpy(sge_mail_start, MSG_MAIL_UNKNOWN_NAME);

   if ((ep=lGetSubStr(jr, UA_name, "end_time", JR_usage)))
      strcpy(sge_mail_end, sge_ctime((u_long32)lGetDouble(ep, UA_value), &ds));
   else   
      strcpy(sge_mail_end, MSG_MAIL_UNKNOWN_NAME);

   if ((ep=lGetSubStr(jr, UA_name, "ru_utime", JR_usage)))
      ru_utime = (u_long32)lGetDouble(ep, UA_value); 
   else
      ru_utime = 0;

   if ((ep=lGetSubStr(jr, UA_name, "ru_stime", JR_usage)))
      ru_stime = (u_long32)lGetDouble(ep, UA_value); 
   else
      ru_stime = 0;

   if ((ep=lGetSubStr(jr, UA_name, "ru_wallclock", JR_usage)))
      ru_wallclock = (u_long32)lGetDouble(ep, UA_value); 
   else
      ru_wallclock = 0;

   if ((ep=lGetSubStr(jr, UA_name, USAGE_ATTR_CPU_ACCT, JR_usage)))
      ru_cpu = lGetDouble(ep, UA_value);
   if ((ep=lGetSubStr(jr, UA_name, USAGE_ATTR_MAXVMEM_ACCT, JR_usage)))
      ru_maxvmem = lGetDouble(ep, UA_value);

   jobid = lGetUlong(jr, JR_job_number);
   taskid = lGetUlong(jr, JR_ja_task_number);

   failed = lGetUlong(jr, JR_failed);
   
   if ((ep=lGetSubStr(jr, UA_name, "exit_status", JR_usage)))
      exit_status = (int)lGetDouble(ep, UA_value);
   
   double_print_time_to_dstring(ru_cpu, &cpu_string);
   double_print_memory_to_dstring(ru_maxvmem, &maxvmem_string);

	/* send job exit mail only for master task */ 
   if ((VALID(MAIL_AT_EXIT, mail_options)) && !failed && !pe_task_id_str) {
      dstring utime_string = DSTRING_INIT;
      dstring stime_string = DSTRING_INIT;
      dstring wtime_string = DSTRING_INIT;

      DPRINTF(("mail VALID at EXIT\n"));
      double_print_time_to_dstring(ru_utime, &utime_string);
      double_print_time_to_dstring(ru_stime, &stime_string);
      double_print_time_to_dstring(ru_wallclock, &wtime_string);
      if (job_is_array(jep)) {
         sprintf(sge_mail_subj, MSG_MAIL_SUBJECT_JA_TASK_COMP_UUS, 
                 u32c(jobid), u32c(taskid), lGetString(jep, JB_job_name));
         sprintf(sge_mail_body,
                 MSG_MAIL_BODY_COMP_SSSSSSSSSSSI, 
                 sge_mail_subj,
                 u,
                 q, 
                 h,
                 sge_mail_start, 
                 sge_mail_end,
                 sge_dstring_get_string(&utime_string),
                 sge_dstring_get_string(&stime_string),
                 sge_dstring_get_string(&wtime_string),
                 (ru_cpu     == 0.0) ? "NA":sge_dstring_get_string(&cpu_string),
                 (ru_maxvmem == 0.0) ? "NA":sge_dstring_get_string(&maxvmem_string),
                 exit_status);
      } else {
         sprintf(sge_mail_subj, MSG_MAIL_SUBJECT_JOB_COMP_US,
                 u32c(jobid), lGetString(jep, JB_job_name));
         sprintf(sge_mail_body, 
                 MSG_MAIL_BODY_COMP_SSSSSSSSSSSI,
                 sge_mail_subj,
                 u,
                 q, 
                 h,
                 sge_mail_start, 
                 sge_mail_end,
                 sge_dstring_get_string(&utime_string),
                 sge_dstring_get_string(&stime_string),
                 sge_dstring_get_string(&wtime_string),
                 (ru_cpu     == 0.0) ? "NA":sge_dstring_get_string(&cpu_string),
                 (ru_maxvmem == 0.0) ? "NA":sge_dstring_get_string(&maxvmem_string),
                 exit_status);
      }

      cull_mail(mail_users, sge_mail_subj, sge_mail_body, MSG_MAIL_TYPE_COMP);
      sge_dstring_free(&utime_string);
      sge_dstring_free(&stime_string);
      sge_dstring_free(&wtime_string);
   }

   if (((VALID(MAIL_AT_ABORT, mail_options)) 
       || (VALID(MAIL_AT_EXIT, mail_options))) && 
       (failed || lGetUlong(jr, JR_general_failure)==GFSTATE_JOB)) {
      char exitstr[256];
      const char *err_str;
      const char *action, *comment = "";

      if (failed==SSTATE_MIGRATE) {
         action = MSG_MAIL_ACTION_MIGR;
      } else if (failed==SSTATE_AGAIN) {
         action = MSG_MAIL_ACTION_RESCH;
      } else if (failed==SSTATE_APPERROR) {
         action = MSG_MAIL_ACTION_APPERROR;
      } else if (lGetUlong(jr, JR_general_failure)==GFSTATE_JOB) {
         action = MSG_MAIL_ACTION_ERR;
         comment = MSG_MAIL_ACTION_ERR_COMMENT;
      } else {
         action = MSG_MAIL_ACTION_ABORT;
      }

      if ((ep=lGetSubStr(jr, UA_name, "signal", JR_usage)))
         signo = (u_long32)lGetDouble(ep, UA_value);

      if (!(err_str=lGetString(jr, JR_err_str)))
         err_str = MSG_UNKNOWNREASON;

      DPRINTF(("MAIL VALID at ABORT\n"));
      sprintf(exitstr, "%d", exit_status);
      if (pe_task_id_str == NULL) {
         if (job_is_array(jep)) {
            sprintf(sge_mail_subj,
                    MSG_MAIL_SUBJECT_JA_TASK_STATE_UUSS,
                    u32c(jobid),
                    u32c(taskid),
                    lGetString(jep, JB_job_name),
                    action);
            sprintf(sge_mail_body,
                    MSG_MAIL_BODY_STATE_SSSSSSSSSSSSS,
                    sge_mail_subj, 
                    exitstr, 
                    sge_sig2str(signo),
                    u, q, h, sge_mail_start, sge_mail_end,
                    (ru_cpu     == 0.0) ? "NA":sge_dstring_get_string(&cpu_string),
                    (ru_maxvmem == 0.0) ? "NA":sge_dstring_get_string(&maxvmem_string),
                    get_sstate_description(failed), 
                    err_str, 
                    comment);
         } else {
            sprintf(sge_mail_subj,
                    MSG_MAIL_SUBJECT_JOB_STATE_USS,
                    u32c(jobid),
                    lGetString(jep, JB_job_name),
                    action);
            sprintf(sge_mail_body, 
                    MSG_MAIL_BODY_STATE_SSSSSSSSSSSSS,
                    sge_mail_subj,
                    exitstr, 
                    sge_sig2str(signo),
                    u, q, h, sge_mail_start, sge_mail_end,
                    (ru_cpu     == 0.0) ? "NA":sge_dstring_get_string(&cpu_string),
                    (ru_maxvmem == 0.0) ? "NA":sge_dstring_get_string(&maxvmem_string),
                    get_sstate_description(failed), 
                    err_str, 
                    comment);
         }
         cull_mail(mail_users, sge_mail_subj, 
                   sge_mail_body, MSG_MAIL_TYPE_STATE);
      }
   }

   sge_dstring_free(&cpu_string);
   sge_dstring_free(&maxvmem_string);
   DEXIT;
   return ;
}
