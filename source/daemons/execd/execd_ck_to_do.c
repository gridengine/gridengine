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
#include <sys/types.h>
#include <signal.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
             
#include "sgermon.h"
#include "sge.h"
#include "sge_me.h"
#include "sge_conf.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_peL.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_usageL.h"
#include "sge_queueL.h"
#include "sge_job_reportL.h"
#include "dispatcher.h"
#include "sge_parse_num_par.h"
#include "reaper_execd.h"
#include "job_report_execd.h"
#include "sge_job_schedd.h"
#include "sge_signal.h"
#include "load_avg.h"
#include "execd_ck_to_do.h"
#include "execd_signal_queue.h"
#include "symbols.h"
#include "exec_job.h"
#include "read_write_job.h"
#include "sge_switch_user.h"
#include "execution_states.h"
#include "sge_stat.h" 
#include "msg_execd.h"
#include "sge_string.h"
#include "sge_feature.h"
#include "sge_security.h"

#ifdef COMPILE_DC
#  include "ptf.h"
#  ifdef DEBUG
#     include "sgedefs.h"
#     include "exec_ifm.h"
#  endif
#endif

#if COMPILE_DC
static int repriorisation_enabled = 0;
#endif

extern volatile int dead_children;
extern volatile int waiting4osjid;


static int sge_start_jobs(void);
static int exec_job_or_task(lListElem *jep, lListElem *jatep, lListElem *slave_jep, lListElem *slave_jatep);

extern int shut_me_down;
extern volatile int jobs_to_start;

extern lList *jr_list;
extern lList *Master_Job_List;

#ifdef COMPILE_DC
static void notify_ptf(void);
static void notify_ptf()
{
   lListElem *jep, *tep;
   int write_job = -1;

   DENTER(TOP_LAYER, "notify_ptf");

   ptf_show_registered_jobs();

   /* ck joblist if there are still jobs waiting for osjobid */
   if (waiting4osjid) {
      waiting4osjid = 0;

      for_each(jep, Master_Job_List) {
         lListElem* jatep;
         
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            write_job = 0;
            if (lGetUlong(jatep, JAT_status) == JWAITING4OSJID) {
               switch (register_at_ptf(jep, jatep, NULL)) {
                  case 0:   
                     /* succeeded */
                     lSetUlong(jatep, JAT_status, JRUNNING);
                  
                     /* spool state transition */ 
                     write_job = 1;
       
                     break;
                  case 1: 
                     /* still waiting for osjobid - nothing changes */
                     waiting4osjid = 1;
                     break;
                  default:
                     /* should add here some cleanup code to remove job */
                     break;
               }
            }

            for_each(tep, lGetList(jatep, JAT_task_list)) {
               lListElem* task_jatep;

               task_jatep = lFirst(lGetList(tep, JB_ja_tasks));
               if (lGetUlong(task_jatep, JAT_status) == JWAITING4OSJID) {
                  switch ( register_at_ptf(jep, jatep, tep)) {
                     case 0:   
                        /* succeeded */
                        lSetUlong(task_jatep, JAT_status, JRUNNING);

                        /* spool state transition */ 
                        write_job = 1;
                        break;

                     case 1: 
                        /* still waiting for osjobid - nothing changes */
                        waiting4osjid = 1;
                        break;

                     default:
                        /* should add here some cleanup code to remove sub-task */
                        break;
                   }
               }
            }
            if (write_job)
               job_write_spool_file(jep, 
                  lGetUlong(jatep, JAT_task_number), SPOOL_WITHIN_EXECD);
         }
      }

      if (waiting4osjid)
         DPRINTF(("still waiting for osjobids\n"));
      else
         DPRINTF(("got all osjobids\n"));
   }

   DEXIT;
   return;
}

/* force job resource limits */
void force_job_rlimit()
{
   lList *usage_list;
   lListElem *q=NULL, *jep, *cpu_ep, *vmem_ep, *gdil_ep, *jatep;
   double cpu_val, vmem_val;
   double s_cpu, h_cpu;
   double s_vmem, h_vmem;
   u_long32 jobid;
   int cpu_exceeded;
   char err_str[128];

   DENTER(TOP_LAYER, "force_job_rlimit");

   for_each (jep, Master_Job_List) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         u_long32 jataskid;
         int nslots=0;

         jobid = lGetUlong(jep, JB_job_number);
         jataskid = lGetUlong(jatep, JAT_task_number);

         cpu_val = vmem_val = s_cpu = h_cpu = s_vmem = h_vmem = 0;

         if (!(usage_list = ptf_get_job_usage(jobid, jataskid, "*")))
            continue;

         if ((cpu_ep = lGetElemStr(usage_list, UA_name, USAGE_ATTR_CPU)))
            cpu_val = lGetDouble(cpu_ep, UA_value);

         if ((vmem_ep = lGetElemStr(usage_list, UA_name, USAGE_ATTR_VMEM)))
            vmem_val = lGetDouble(vmem_ep, UA_value);
            
         for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
            double lim;

            if (hostcmp(me.qualified_hostname, 
                 lGetString(gdil_ep, JG_qhostname)) 
                || !(q = lFirst(lGetList(gdil_ep, JG_queue))))
               continue;

            nslots = lGetUlong(gdil_ep, JG_slots);

            parse_ulong_val(&lim, NULL, TYPE_TIM, lGetString(q, QU_s_cpu), 
                              err_str, sizeof(err_str)-1);
            if (lim == DBL_MAX) 
               s_cpu = DBL_MAX;
            else
               s_cpu += lim * nslots; 

            parse_ulong_val(&lim, NULL, TYPE_TIM, lGetString(q, QU_h_cpu), 
                              err_str, sizeof(err_str)-1);
            if (lim == DBL_MAX) 
               h_cpu = DBL_MAX;
            else
               h_cpu += lim * nslots; 

            parse_ulong_val(&lim, NULL, TYPE_TIM, lGetString(q, QU_s_vmem), 
                              err_str, sizeof(err_str)-1);
            if (lim == DBL_MAX) 
               s_vmem = DBL_MAX;
            else
               s_vmem += lim * nslots; 

            parse_ulong_val(&lim, NULL, TYPE_TIM, lGetString(q, QU_h_vmem), 
                              err_str, sizeof(err_str)-1);
            if (lim == DBL_MAX) 
               h_vmem = DBL_MAX;
            else
               h_vmem += lim * nslots; 

         }

         DPRINTF(("JOB "u32" %s %10.5f %s %10.5f\n", 
            jobid,
            cpu_ep?USAGE_ATTR_CPU:"("USAGE_ATTR_CPU")",  
            cpu_val, 
            vmem_ep?USAGE_ATTR_VMEM:"("USAGE_ATTR_VMEM")",  
            vmem_val));

         if (h_cpu < cpu_val || h_vmem < vmem_val) {
            cpu_exceeded = (h_cpu < cpu_val);
            WARNING((SGE_EVENT, MSG_JOB_EXCEEDHLIM_USSFF, 
                     u32c(jobid), cpu_exceeded ? "h_cpu" : "h_vmem",
                     q?lGetString(q, QU_qname) : "-",
                     cpu_exceeded ? cpu_val : vmem_val,
                     cpu_exceeded ? h_cpu : h_vmem));
            signal_job(jobid, jataskid, SGE_SIGKILL);
            continue;
         }

         if (s_cpu < cpu_val || s_vmem < vmem_val) {
            cpu_exceeded = (s_cpu < cpu_val);
            WARNING((SGE_EVENT, MSG_JOB_EXCEEDSLIM_USSFF,
                     u32c(jobid),
                     cpu_exceeded ? "s_cpu" : "s_vmem",
                     q?lGetString(q, QU_qname) : "-",
                     cpu_exceeded ? cpu_val : vmem_val,
                     cpu_exceeded ? s_cpu : s_vmem));
            signal_job(jobid, jataskid, SGE_SIGXCPU);
            continue;
         }

         if (usage_list)
            lFreeList(usage_list);

      }
   }

   DEXIT;
}
#endif

/******************************************************
 EXECD function

 called by dispatcher on a cyclic basis 
 cyclic means on every received message and on timeout


   as all dispatcher called function returns
   -  0 on success
   - -1 on error
   -  1 if we want to quit the dispacher loop
   
 do cyclic jobs
 ******************************************************/

int execd_ck_to_do(
struct dispatch_entry *de,
sge_pack_buffer *pb,
sge_pack_buffer *apb,
u_long *rcvtimeout,
int *synchron,
char *err_str,
int answer_error 
) {
   u_long32 now, clock_val;
   static u_long then = 0;
   lListElem *jep, *jatep;

   DENTER(TOP_LAYER, "execd_ck_to_do");

#ifdef KERBEROS
   krb_renew_tgts(Master_Job_List);
#endif


#ifdef COMPILE_DC
   if (feature_is_enabled(FEATURE_USE_OSJOB_ID)) {
      notify_ptf();
   }
   if (feature_is_enabled(FEATURE_REPORT_USAGE)) {
      switch2start_user();
      ptf_update_job_usage();
      switch2admin_user();
   }
   if (feature_is_enabled(FEATURE_REPRIORISATION)) {
      switch2start_user();
      ptf_adjust_job_priorities();
      switch2admin_user();
      repriorisation_enabled = 1;
   } else {
      /* Here we will make sure that each job which was started
         in SGEEE-Mode (repriorisation) will get its initial
         queue priority if this execd alternates to SGE-Mode */
      if (repriorisation_enabled) {
         lListElem *job, *jatask;
         lListElem *slave_job, *slave_jatask;

         for_each(job, Master_Job_List) {
            lListElem *master_queue;

            for_each (jatask, lGetList(job, JB_ja_tasks)) {
               master_queue = 
                        responsible_queue(job, jatask, NULL, NULL);

               DPRINTF(("EB Set priority of job "u32"."u32" running in"
                  " queue  %s to "u32"\n", 
                  lGetUlong(job, JB_job_number), 
                  lGetUlong(jatask, JAT_task_number),
                  lGetString(master_queue, QU_qname),
                  lGetUlong(master_queue, QU_priority)));
               ptf_reinit_queue_priority(
                  lGetUlong(job, JB_job_number),
                  lGetUlong(jatask, JAT_task_number),
                  NULL,
                  lGetUlong(master_queue, QU_priority));

               for_each(slave_job, lGetList(jatask, JAT_task_list)) {
                  slave_jatask = lFirst(lGetList(slave_job, JB_ja_tasks)); 
                  master_queue = 
                        responsible_queue(slave_job, slave_jatask, job, jatask);
                  DPRINTF(("EB Set priority of task "u32"."u32"-%s running "
                     "in queue %s to "u32" \n", 
                     lGetUlong(slave_job, JB_job_number), 
                     lGetUlong(slave_jatask, JAT_task_number),
                     lGetString(slave_job, JB_pe_task_id_str),
                     lGetString(master_queue, QU_qname),
                     lGetUlong(master_queue, QU_priority)));
                  ptf_reinit_queue_priority(
                     lGetUlong(slave_job, JB_job_number),
                     lGetUlong(slave_jatask, JAT_task_number),
                     lGetString(slave_job, JB_pe_task_id_str),
                     lGetUlong(master_queue, QU_priority));
               }
            }
         }
      }          
      repriorisation_enabled = 0;
   }
#endif
      
   /* start jobs if present */
   if (jobs_to_start) {
      sge_start_jobs();
      jobs_to_start = 0;
   }

   now = sge_get_gmt();

   /* resend signals to shepherds */
   for_each(jep, Master_Job_List) {
      lListElem *master_q;

      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         master_q = lFirst(lGetList(lFirst(
                           lGetList(jatep, JAT_granted_destin_identifier_list)), JG_queue));

         if (!lGetUlong(jep, JB_hard_wallclock_gmt)) {       /* initialize everything */
            if (strcasecmp(lGetString(master_q, QU_s_rt), "infinity")) {
               parse_ulong_val(NULL, &clock_val, TYPE_TIM, lGetString(master_q, QU_s_rt), NULL, 0);
               lSetUlong(jep, JB_soft_wallclock_gmt, clock_val + now);
            } else
               lSetUlong(jep, JB_soft_wallclock_gmt, (u_long32)(~0L>>1));

            if (strcasecmp(lGetString(master_q, QU_h_rt), "infinity")) {
               parse_ulong_val(NULL, &clock_val, TYPE_TIM, lGetString(master_q, QU_h_rt), NULL, 0);
               lSetUlong(jep, JB_hard_wallclock_gmt, clock_val + now);
            } else 
               lSetUlong(jep, JB_hard_wallclock_gmt, (u_long32)(~0L>>1));
         }

         if (now >= lGetUlong(jep, JB_hard_wallclock_gmt) ) {
            if (!(lGetUlong(jatep, JAT_pending_signal_delivery_time)) ||
                (now > lGetUlong(jatep, JAT_pending_signal_delivery_time))) {
               INFO((SGE_EVENT, MSG_EXECD_EXCEEDHWALLCLOCK_UU,
                    u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number)))); 
               sge_kill(lGetUlong(jatep, JAT_pid), SGE_SIGKILL, 
                        lGetUlong(jep, JB_job_number),
                        lGetUlong(jatep, JAT_task_number),
                        NULL);     
               lSetUlong(jatep, JAT_pending_signal_delivery_time, now+90);
            }    
            continue;
         }

         if (now >= lGetUlong(jep, JB_soft_wallclock_gmt)) {
            if (!(lGetUlong(jatep, JAT_pending_signal_delivery_time)) ||
                (now > lGetUlong(jatep, JAT_pending_signal_delivery_time))) {
               INFO((SGE_EVENT, MSG_EXECD_EXCEEDSWALLCLOCK_UU,
                    u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number))));  
               sge_kill(lGetUlong(jatep, JAT_pid), SGE_SIGUSR1, 
                        lGetUlong(jep, JB_job_number),
                        lGetUlong(jatep, JAT_task_number),
                        NULL);
               lSetUlong(jatep, JAT_pending_signal_delivery_time, now+90);         
            }
            continue;
         }   
      }
   }

   if (dead_children) {
      /* reap all jobs, who generated a SIGCLD */
      sge_reap_children_execd();
      dead_children = 0;
   }

   clean_up_old_jobs(0);

   /* do timeout calculation */
   now = sge_get_gmt();
   if ( flush_jr || now - then > conf.load_report_time) {
      extern int report_seqno;
      then = now;
      /* wrap around */
      if (++report_seqno == 10000)
         report_seqno = 0;
      sge_send_all_reports(now, 0, execd_report_sources);
      flush_jr = 0;
   }

   /* handle shutdown */
   switch (shut_me_down) {
      case 1:
         DEXIT;
         return 1;   /* tell dispatcher to finish server */
      case 0:
         DEXIT;
         return 0;
      default:
         /* if shut_me_down == 2 we wait one "dispatch epoche"
            and we hope that all the killed jobs are reaped 
            reaped jobs will be reported to qmaster 
            since flush_jr is set in this case
         */
         if (!Master_Job_List || !lGetNumberOfElem(Master_Job_List)) { /* no need to delay shutdown */
            DEXIT;
            return 1;
         }
         DPRINTF(("DELAYED SHUTDOWN\n"));
         shut_me_down--;
         DEXIT;
         return 0;
   }
}


/*****************************************************************************/
static int sge_start_jobs()
{
   lListElem *jep, *jatep, *tep;
   int state_changed;

   DENTER(TOP_LAYER, "sge_start_jobs");

   if (!Master_Job_List || !lGetNumberOfElem(Master_Job_List)) {
      DPRINTF(("No jobs to start\n"));
      DEXIT;
      return 0;
   }

   for_each(jep, Master_Job_List) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         state_changed = exec_job_or_task(jep, jatep, NULL, NULL);

         /* visit all tasks */
         for_each(tep, lGetList(jatep, JAT_task_list))
            state_changed |= exec_job_or_task(tep, lFirst(lGetList(tep, JB_ja_tasks)),
                                                   jep, jatep);

         /* now save this job so we are up to date on restart */
         if (state_changed)
            job_write_spool_file(jep, lGetUlong(jatep, JAT_task_number), 
               SPOOL_WITHIN_EXECD);
      }
   }

   DEXIT;
   return 0;
}


static int exec_job_or_task(
lListElem *jep,
lListElem *jatep,
lListElem *slave_jep,
lListElem *slave_jatep 
) {
   char err_str[256];
   int pid;
   const char *tid;
   u_long32 now;

   DENTER(TOP_LAYER, "exec_job_or_task");

   if (lGetUlong(jatep, JAT_status) != JIDLE) {
      DEXIT;
      return 0;
   }

#ifdef COMPILE_DC
   lSetUlong(jatep, JAT_status, JWAITING4OSJID);
   waiting4osjid = 1;
#else
   lSetUlong(jatep, JAT_status, JRUNNING);
#endif

   now = sge_get_gmt();
   lSetUlong(jatep, JAT_start_time, now);
   if (getenv("FAILURE_BEFORE_EXEC")) {
      pid = -1; 
      strcpy(err_str, "FAILURE_BEFORE_EXEC");
   }
   else
      pid = sge_exec_job(jep, jatep, slave_jep, slave_jatep, err_str);

   if (pid < 0) {
      flush_jr = 1;      
      switch (pid) {
         case -1:
            execd_job_start_failure(jep, jatep, err_str, GFSTATE_NO_HALT);
            break;
         case -2:
            execd_job_start_failure(jep, jatep, err_str, GFSTATE_QUEUE);
            break;
         case -3:
            execd_job_start_failure(jep, jatep, err_str, GFSTATE_JOB);
            break;
         default:
            execd_job_start_failure(jep, jatep, err_str, GFSTATE_HOST);
            break;
      }
      DEXIT;
      return 0;
   }
   DTIMEPRINTF(("TIME IN EXECD FOR STARTING THE JOB: " u32 "\n",
                sge_get_gmt()-now));
   lSetUlong(jatep, JAT_pid, pid);
   tid = lGetString(jep, JB_pe_task_id_str);
   DPRINTF(("***EXECING "u32"."u32" on %s (tid = %s) (pid = %d)\n",
            lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number), 
            me.unqualified_hostname, tid?tid:"0", pid));

   DEXIT;
   return 1;
}

#ifdef COMPILE_DC
int register_at_ptf(
lListElem *jep,
lListElem *jatep,
lListElem *tep 
) {
   const char *task_id_str=NULL;
   u_long32 jobid;   
   u_long32 jataskid;   
   int success, newerrno;
   FILE *fp;
   SGE_STRUCT_STAT sb;

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX) 
   gid_t addgrpid;
   char addgrpid_path[SGE_PATH_MAX];
#else   
   char osjobid_path[SGE_PATH_MAX];
   osjobid_t osjobid;   
#endif   
   DENTER(TOP_LAYER, "register_at_ptf");

   jobid = lGetUlong(jep, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
   /**
    ** read additional group id and use it as osjobid 
    **/
   
   /* open addgrpid file */
   if (tep) { 
      task_id_str = lGetString(tep, JB_pe_task_id_str);
      sprintf(addgrpid_path, ACTIVE_DIR"/"u32"."u32"/%s/"ADDGRPID, jobid, jataskid, 
               task_id_str);
   }
   else {
      sprintf(addgrpid_path, ACTIVE_DIR"/"u32"."u32"/"ADDGRPID, jobid, jataskid);
   }
   DPRINTF(("Registering job "u32"."u32" task \"%s\" with PTF\n", jobid, jataskid, 
            task_id_str?task_id_str:""));

   if (SGE_STAT(addgrpid_path, &sb) && errno == ENOENT) {
      DPRINTF(("still waiting for addgrpid of job "u32"."u32"\n", 
         jobid, jataskid));
      DEXIT;
      return(1);
   }  

   if (!(fp = fopen(addgrpid_path, "r"))) {
      ERROR((SGE_EVENT, MSG_EXECD_NOADDGIDOPEN_SUUS, addgrpid_path, u32c(jobid), u32c(jataskid), strerror(errno)));
      DEXIT;
      return(-1);
   }
   
   /* read addgrpid */
   success = (fscanf(fp, gid_t_fmt, &addgrpid)==1);
   newerrno = errno;
   fclose(fp);
   if (!success) {
      /* can happen that shepherd has opend the file but not written */
      DEXIT;
      return (1);
   }
   {
      int ptf_error;

      DPRINTF(("Register job with AddGrpId at "pid_t_fmt" PTF\n", addgrpid));
      if ((ptf_error = ptf_job_started(addgrpid, task_id_str, jep, jataskid))) {
         ERROR((SGE_EVENT, MSG_JOB_NOREGISTERPTF_UUS, u32c(jobid), u32c(jataskid), ptf_errstr(ptf_error)));
         DEXIT;
         return (1);
      }
   }

   /* store addgrpid in job report to be sent to qmaster later on */
   {
      char addgrpid_str[64];
      lListElem *jr;

      sprintf(addgrpid_str, pid_t_fmt, addgrpid);
      if ((jr=get_job_report(jobid, jataskid, task_id_str)))
         lSetString(jr, JR_osjobid, addgrpid_str); 
      DPRINTF(("job "u32"."u32" addgrpid = %s\n", jobid, jataskid, addgrpid_str));
   }
#else
   /* read osjobid if possible */
   if (tep) {
      task_id_str = lGetString(tep, JB_pe_task_id_str);
      sprintf(osjobid_path, ACTIVE_DIR"/"u32"."u32"/%s/"OSJOBID, jobid, jataskid, task_id_str);
   } else {
      sprintf(osjobid_path, ACTIVE_DIR"/"u32"."u32"/"OSJOBID, jobid, jataskid);
   }
   DPRINTF(("Registering job "u32"."u32" task \"%s\" with PTF\n", jobid, jataskid, task_id_str?task_id_str:""));

   if (SGE_STAT(osjobid_path, &sb) && errno == ENOENT) {
      DPRINTF(("still waiting for osjobid of job "u32"\n", jobid));
      DEXIT;
      return 1;
   } 

   if (!(fp=fopen(osjobid_path, "r"))) {
      ERROR((SGE_EVENT, MSG_EXECD_NOOSJOBIDOPEN_SUUS, osjobid_path, u32c(jobid), u32c(jataskid), strerror(errno)));
      DEXIT;
      return -1;
   }

   success = (fscanf(fp, OSJOBID_FMT, &osjobid)==1);
   newerrno = errno;
   fclose(fp);
   if (!success) {
      /* can happen that shepherd has opend the file but not written */
      DEXIT;
      return 1;
   }

   {
      int ptf_error;
      if ((ptf_error = ptf_job_started(osjobid, task_id_str, jep, jataskid))) {
         ERROR((SGE_EVENT, MSG_JOB_NOREGISTERPTF_UUS,  u32c(jobid), u32c(jataskid), ptf_errstr(ptf_error)));
         DEXIT;
         return -1;
      }
   }

   /* store osjobid in job report to be sent to qmaster later on */
   {
      char osjobid_str[64];
      lListElem *jr;

      sprintf(osjobid_str, OSJOBID_FMT, osjobid);
      if ((jr=get_job_report(jobid, jataskid, task_id_str)))
         lSetString(jr, JR_osjobid, osjobid_str); 
      DPRINTF(("job "u32"."u32" osjobid = %s\n", jobid, jataskid, osjobid_str));
   }
#endif

   DEXIT;
   return 0;
}
#endif

