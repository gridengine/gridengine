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

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif    
             
#include "sgermon.h"
#include "sge.h"
#include "sge_conf.h"
#include "sge_ja_task.h"
#include "sge_job.h"
#include "sge_pe_task.h"
#include "sge_pe.h"
#include "sge_qinstance.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_usageL.h"
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
#include "spool/classic/read_write_job.h"
#include "execution_states.h"
#include "msg_execd.h"
#include "sge_string.h"
#include "sge_feature.h"
#include "sge_uidgid.h"
#include "sge_security.h"
#include "sge_unistd.h"
#include "sge_hostname.h"
#include "sge_prog.h"
#include "get_path.h"
#include "sge_report.h"

#ifdef COMPILE_DC
#  include "ptf.h"
#  ifdef DEBUG
#     include "sgedefs.h"
#     include "exec_ifm.h"
#  endif
#endif

#include "sge_str.h"

#if COMPILE_DC
static int reprioritization_enabled = 0;
#endif

extern volatile int dead_children;
extern volatile int waiting4osjid;


static bool 
sge_execd_ja_task_is_tightly_integrated(const lListElem *ja_task);
static bool
sge_kill_petasks(const lListElem *job, const lListElem *ja_task);

static int sge_start_jobs(void);
static int exec_job_or_task(lListElem *jep, lListElem *jatep, lListElem *petep);

static bool should_reprioritize(void);

extern int shut_me_down;
extern volatile int jobs_to_start;

extern lList *jr_list;

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
               if (lGetUlong(tep, PET_status) == JWAITING4OSJID) {
                  switch ( register_at_ptf(jep, jatep, tep)) {
                     case 0:   
                        /* succeeded */
                        lSetUlong(tep, PET_status, JRUNNING);

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
               job_write_spool_file(jep, lGetUlong(jatep, JAT_task_number), 
                                    NULL, SPOOL_WITHIN_EXECD);
         }
      }

      if (waiting4osjid) {
         DPRINTF(("still waiting for osjobids\n"));
      } else {
         DPRINTF(("got all osjobids\n"));
      }   
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

            if (sge_hostcmp(uti_state_get_qualified_hostname(), 
                 lGetHost(gdil_ep, JG_qhostname)) 
                || !(q = lGetObject(gdil_ep, JG_queue)))
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
                     q?lGetString(q, QU_full_name) : "-",
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
                     q?lGetString(q, QU_full_name) : "-",
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

static u_long32 
execd_get_wallclock_limit(lList *gdil_list, int limit_nm, u_long32 now) 
{
   u_long32 ret = U_LONG32_MAX;
   const lListElem *gdil;
   const char *hostname;
   const void *iterator;
  
   hostname = uti_state_get_qualified_hostname();
   gdil = lGetElemHostFirst(gdil_list, JG_qhostname, hostname, &iterator);
   while (gdil != NULL) {
      const lListElem *queue;
      const char *limit;
      u_long32 clock_val;

      queue = lGetObject(gdil, JG_queue);
      if (queue != NULL) {
         limit = lGetString(queue, limit_nm);

         if (strcasecmp(limit, "infinity") == 0) {
            clock_val = U_LONG32_MAX;
         } else {   
            parse_ulong_val(NULL, &clock_val, TYPE_TIM, limit, NULL, 0);
         }   

         ret = MIN(ret, clock_val);
      }

      gdil = lGetElemHostNext(gdil_list, JG_qhostname, hostname, &iterator);
   }

   if (ret != U_LONG32_MAX) {
      ret += now;
   }

   return ret;
}

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
/* TODO: what are the intended intervals? */
#define USAGE_INTERVAL 1
#define SIGNAL_RESEND_INTERVAL 1
#define OLD_JOB_INTERVAL 60

int execd_ck_to_do(
struct dispatch_entry *de,
sge_pack_buffer *pb,
sge_pack_buffer *apb,
u_long *rcvtimeout,
int *synchron,
char *err_str,
int answer_error 
) {
   u_long32 now;
   static u_long next_usage = 0;
   static u_long next_signal = 0;
   static u_long next_old_job = 0;
   static u_long next_report = 0;
   lListElem *jep, *jatep;
   int was_communication_error = CL_RETVAL_UNKNOWN;
   int return_value = 0;

   DENTER(TOP_LAYER, "execd_ck_to_do");

   now = sge_get_gmt();

#ifdef KERBEROS
   krb_renew_tgts(Master_Job_List);
#endif

   /*
    * Collecting usage and repriorization is only necessary if there are
    * jobs/tasks on this execution host.
    * do this at maximum once per second
    */
   if (lGetNumberOfElem(Master_Job_List) > 0 &&
       next_usage <= now) {
      next_usage = now + USAGE_INTERVAL;
#ifdef COMPILE_DC
      notify_ptf();

      sge_switch2start_user();
      ptf_update_job_usage();
      sge_switch2admin_user();

      if (should_reprioritize()) {
         sge_switch2start_user();
         DPRINTF(("ADJUST PRIORITIES\n"));
         ptf_adjust_job_priorities();
         sge_switch2admin_user();
         reprioritization_enabled = 1;
      } else {
         /* Here we will make sure that each job which was started
            in SGEEE-Mode (reprioritization) will get its initial
            queue priority if this execd alternates to SGE-Mode */
         if (reprioritization_enabled) {
            lListElem *job, *jatask, *petask;

            sge_switch2start_user();

            for_each(job, Master_Job_List) {
               lListElem *master_queue;

               for_each (jatask, lGetList(job, JB_ja_tasks)) {
                  int priority;
                  master_queue = 
                           responsible_queue(job, jatask, NULL);
                  priority = atoi(lGetString(master_queue, QU_priority));

                  DPRINTF(("Set priority of job "u32"."u32" running in"
                     " queue  %s to %d\n", 
                     lGetUlong(job, JB_job_number), 
                     lGetUlong(jatask, JAT_task_number),
                     lGetString(master_queue, QU_full_name), priority));
                  ptf_reinit_queue_priority(
                     lGetUlong(job, JB_job_number),
                     lGetUlong(jatask, JAT_task_number),
                     NULL,
                     priority);

                  for_each(petask, lGetList(jatask, JAT_task_list)) {
                     master_queue = 
                           responsible_queue(job, jatask, petask);
                     priority = atoi(lGetString(master_queue, QU_priority));
                     DPRINTF(("EB Set priority of task "u32"."u32"-%s running "
                        "in queue %s to %d\n", 
                        lGetUlong(job, JB_job_number), 
                        lGetUlong(jatask, JAT_task_number),
                        lGetString(petask, PET_id),
                        lGetString(master_queue, QU_full_name), priority));
                     ptf_reinit_queue_priority(
                        lGetUlong(job, JB_job_number),
                        lGetUlong(jatask, JAT_task_number),
                        lGetString(petask, PET_id),
                        priority);
                  }
               }
            }
            sge_switch2admin_user();
         } else {
            DPRINTF(("LEAVE PRIORITIES UNTOUCHED\n"));
         }
         reprioritization_enabled = 0;
      }
#endif
      /* update the usage list in our job report list */
      update_job_usage();

#ifdef COMPILE_DC
      /* check for job limits */
      force_job_rlimit();
#endif
   }
      
   /* start jobs if present */
   if (jobs_to_start) {
      sge_start_jobs();
      jobs_to_start = 0;
   }

   now = sge_get_gmt();
   if (next_signal <= now) {
      next_signal = now + SIGNAL_RESEND_INTERVAL;
      /* resend signals to shepherds */
      for_each(jep, Master_Job_List) {
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if (!lGetUlong(jep, JB_hard_wallclock_gmt)) {
               lList *gdil_list = lGetList(jatep, 
                                           JAT_granted_destin_identifier_list);
               lSetUlong(jep, JB_soft_wallclock_gmt, 
                         execd_get_wallclock_limit(gdil_list, QU_s_rt, now));
               lSetUlong(jep, JB_hard_wallclock_gmt, 
                         execd_get_wallclock_limit(gdil_list, QU_h_rt, now));
            }

            if (now >= lGetUlong(jep, JB_hard_wallclock_gmt) ) {
               if (!(lGetUlong(jatep, JAT_pending_signal_delivery_time)) ||
                   (now > lGetUlong(jatep, JAT_pending_signal_delivery_time))) {
                  INFO((SGE_EVENT, MSG_EXECD_EXCEEDHWALLCLOCK_UU,
                       u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number)))); 
                  if (sge_execd_ja_task_is_tightly_integrated(jatep)) {
                     sge_kill_petasks(jep, jatep);
                  }
                  if (lGetUlong(jatep, JAT_pid) != 0) {
                     sge_kill(lGetUlong(jatep, JAT_pid), SGE_SIGKILL, 
                              lGetUlong(jep, JB_job_number),
                              lGetUlong(jatep, JAT_task_number),
                              NULL);     
                  }
                  lSetUlong(jatep, JAT_pending_signal_delivery_time, now+90);
               }    
               continue;
            }

            if (now >= lGetUlong(jep, JB_soft_wallclock_gmt)) {
               if (!(lGetUlong(jatep, JAT_pending_signal_delivery_time)) ||
                   (now > lGetUlong(jatep, JAT_pending_signal_delivery_time))) {
                  INFO((SGE_EVENT, MSG_EXECD_EXCEEDSWALLCLOCK_UU,
                       u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number))));  
                  if (sge_execd_ja_task_is_tightly_integrated(jatep)) {
                     sge_kill_petasks(jep, jatep);
                  }
                  if (lGetUlong(jatep, JAT_pid) != 0) {
                     sge_kill(lGetUlong(jatep, JAT_pid), SGE_SIGUSR1, 
                              lGetUlong(jep, JB_job_number),
                              lGetUlong(jatep, JAT_task_number),
                              NULL);
                  }
                  lSetUlong(jatep, JAT_pending_signal_delivery_time, now+90);         
               }
               continue;
            }   
         }
      }
   }

   if (dead_children) {
      /* reap all jobs, who generated a SIGCLD */
      sge_reap_children_execd();
      dead_children = 0;
   }

   if (next_old_job <= now) {
      next_old_job = now + OLD_JOB_INTERVAL;
      clean_up_old_jobs(0);
   }

   /* check for end of simulated jobs */
   if(simulate_hosts) {
      for_each(jep, Master_Job_List) {
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if((lGetUlong(jatep, JAT_status) & JSIMULATED) && lGetUlong(jatep, JAT_end_time) <= now) {
               lListElem *jr = NULL;
               u_long32 jobid, jataskid;
               u_long32 wallclock;

               jobid = lGetUlong(jep, JB_job_number);
               jataskid = lGetUlong(jatep, JAT_task_number);

               DPRINTF(("Simulated job "u32"."u32" is exiting\n", jobid, jataskid));

               if ((jr=get_job_report(jobid, jataskid, NULL)) == NULL) {
                  ERROR((SGE_EVENT, MSG_JOB_MISSINGJOBXYINJOBREPORTFOREXITINGJOBADDINGIT_UU, 
                         u32c(jobid), u32c(jataskid)));
                  jr = add_job_report(jobid, jataskid, NULL, jep);
               }

               lSetUlong(jr, JR_state, JEXITING);
               add_usage(jr, "submission_time", NULL, lGetUlong(jep, JB_submission_time));
               add_usage(jr, "start_time", NULL, lGetUlong(jatep, JAT_start_time));
               add_usage(jr, "end_time", NULL, lGetUlong(jatep, JAT_end_time));
               wallclock = lGetUlong(jatep, JAT_end_time) - lGetUlong(jatep, JAT_start_time);
               add_usage(jr, "ru_wallclock", NULL, wallclock);
               add_usage(jr, USAGE_ATTR_CPU_ACCT, NULL, wallclock * 0.5);
               add_usage(jr, "ru_utime", NULL, wallclock * 0.4 );
               add_usage(jr, "ru_utime", NULL, wallclock * 0.1 );
               add_usage(jr, "exit_status", NULL, 0);
            
               
               lSetUlong(jatep, JAT_status, JEXITING | JSIMULATED);
               flush_job_report(jr);
            }
         }
      }   
   }

   /* do timeout calculation */
   now = sge_get_gmt();
   if ( flush_jr || next_report <= now) {
      extern int report_seqno;
      if (next_report <= now) {
         next_report = now + conf.load_report_time;
      }

      /* wrap around */
      if (++report_seqno == 10000) {
         report_seqno = 0;
      }

      /* send all reports */
      was_communication_error = sge_send_all_reports(now, 0, execd_report_sources);
      DPRINTF(("----> was_communication_error: %s (%d)\n", cl_get_error_text(was_communication_error),was_communication_error));
   } else {
      was_communication_error = CL_RETVAL_OK;
   }

   /* handle shutdown */
   switch (shut_me_down) {
      case 1:
         return_value = 1; /* tell dispatcher to finish server */
         break;
      case 0:
         return_value = 0;
         break;
      default:
         /* if shut_me_down == 2 we wait one "dispatch epoche"
            and we hope that all the killed jobs are reaped 
            reaped jobs will be reported to qmaster 
            since flush_jr is set in this case
         */
         if (!Master_Job_List || !lGetNumberOfElem(Master_Job_List)) { /* no need to delay shutdown */
            return_value = 1;
         } else {
            DPRINTF(("DELAYED SHUTDOWN\n"));
            shut_me_down--;
            return_value = 0;
         }
   }

   if ( return_value == 0 && was_communication_error != CL_RETVAL_OK ) {
      DPRINTF(("was_communication_error is %s\n", cl_get_error_text(was_communication_error)));
      return_value = -1;  /* leave dispatcher */
   }

   DEXIT;
   return return_value;
}

/****** execd_ck_to_do/sge_execd_ja_task_is_tightly_integrated() ***************
*  NAME
*     sge_execd_ja_task_is_tightly_integrated() -- is it a tightly integr. parallel job?
*
*  SYNOPSIS
*     static bool 
*     sge_execd_ja_task_is_tightly_integrated(const lListElem *ja_task) 
*
*  FUNCTION
*     Checks if a certain job (ja_task) is running in a tightly integrated
*     parallel environment.
*
*  INPUTS
*     const lListElem *ja_task - ja_task (in execd context)
*
*  RESULT
*     static bool - true, if it is a tightly integrated job, else false
*
*******************************************************************************/
static bool 
sge_execd_ja_task_is_tightly_integrated(const lListElem *ja_task)
{
   bool ret = false;

   if (ja_task != NULL) {
      const lListElem *pe;

      pe = lGetObject(ja_task, JAT_pe_object);
      if (pe != NULL) {
         if (lGetBool(pe, PE_control_slaves)) {
            ret = true;
         }
      }
   }

   return ret;
}

/****** execd_ck_to_do/sge_kill_petasks() **************************************
*  NAME
*     sge_kill_petasks() -- kill all pe tasks of a tightly integr. parallel job
*
*  SYNOPSIS
*     static bool 
*     sge_kill_petasks(const lListElem *job, const lListElem *ja_task) 
*
*  FUNCTION
*     Kills all tasks of a tightly integrated parallel job/array task.
*
*  INPUTS
*     const lListElem *job     - the job
*     const lListElem *ja_task - the array task
*
*  RESULT
*     static bool - true, if any task was found and could be signalled, 
*                   else false
*
*******************************************************************************/
static bool
sge_kill_petasks(const lListElem *job, const lListElem *ja_task)
{
   bool ret = false;

   if (job != NULL && ja_task != NULL) {
      lListElem *pe_task;

      for_each(pe_task, lGetList(ja_task, JAT_task_list)) {
         if (sge_kill(lGetUlong(pe_task, PET_pid), SGE_SIGKILL,
                      lGetUlong(job, JB_job_number),
                      lGetUlong(ja_task, JAT_task_number),
                      lGetString(pe_task, PET_id)) == 0) {
            ret = true;
         }
      }
   }

   return ret;
}


/*****************************************************************************/
static int sge_start_jobs()
{
   lListElem *jep, *jatep, *petep;
   int state_changed;

   DENTER(TOP_LAYER, "sge_start_jobs");

   if (!Master_Job_List || !lGetNumberOfElem(Master_Job_List)) {
      DPRINTF(("No jobs to start\n"));
      DEXIT;
      return 0;
   }

   for_each(jep, Master_Job_List) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         state_changed = exec_job_or_task(jep, jatep, NULL);

         /* visit all tasks */
         for_each(petep, lGetList(jatep, JAT_task_list))
            state_changed |= exec_job_or_task(jep, jatep, petep);

         /* now save this job so we are up to date on restart */
         if (state_changed)
            job_write_spool_file(jep, lGetUlong(jatep, JAT_task_number), 
                                 NULL, SPOOL_WITHIN_EXECD);
      }
   }

   DEXIT;
   return 0;
}


static int exec_job_or_task(
lListElem *jep,
lListElem *jatep,
lListElem *petep
) {
   char err_str[256];
   int pid;
   u_long32 now;
   u_long32 job_id, ja_task_id;
   const char *pe_task_id = NULL;

   DENTER(TOP_LAYER, "exec_job_or_task");

   /* retrieve ids - we need them later on */
   job_id = lGetUlong(jep, JB_job_number);
   ja_task_id = lGetUlong(jatep, JAT_task_number);
   if (petep != NULL) {
      pe_task_id = lGetString(petep, PET_id);
   }

   /* we only handle idle jobs or tasks */
   /* JG: TODO: make a function is_task_idle(jep, jatep, petep) */
   {
      u_long32 status;

      if(petep != NULL) {
         status = lGetUlong(petep, PET_status);
      } else {
         status = lGetUlong(jatep, JAT_status);
      }

      if (status != JIDLE) {
         DEXIT;
         return 0;
      }
   }   

   now = sge_get_gmt();

   /* we might simulate another host */
   /* JG: TODO: make a function simulate_start_job_or_task() */
   if(simulate_hosts == 1) {
      const char *host = lGetHost(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), JG_qhostname);
      if(sge_hostcmp(host, uti_state_get_qualified_hostname()) != 0) {
         lList *job_args;
         u_long32 duration = 60;

         DPRINTF(("Simulating job "u32"."u32"\n", 
                  job_id, ja_task_id));
         lSetUlong(jatep, JAT_start_time, now);
         lSetUlong(jatep, JAT_status, JRUNNING | JSIMULATED);

         /* set time when job shall be reported as finished */
         job_args = lGetList(jep, JB_job_args);
         if(lGetNumberOfElem(job_args) == 1) {
            const char *arg = NULL;
            char *endptr = NULL;
            u_long32 duration_in;
  
            arg = lGetString(lFirst(job_args), ST_name);
            if(arg != NULL) {
               DPRINTF(("Trying to use first argument ("SFQ") as duration for simulated job\n", arg));
               
               duration_in = strtol(arg, &endptr, 0);
               if(arg != endptr) {
                  duration = duration_in;
               }
            }   
         }

         lSetUlong(jatep, JAT_end_time, now + duration);
         return 1;
      }
   }

   if(petep != NULL) {
      lSetUlong(petep, PET_start_time, now);
#ifdef COMPILE_DC
      lSetUlong(petep, PET_status, JWAITING4OSJID);
      waiting4osjid = 1;
#else
      lSetUlong(petep, PET_status, JRUNNING);
#endif
   } else {
      lSetUlong(jatep, JAT_start_time, now);
#ifdef COMPILE_DC
      lSetUlong(jatep, JAT_status, JWAITING4OSJID);
      waiting4osjid = 1;
#else
      lSetUlong(jatep, JAT_status, JRUNNING);
#endif
   }

   if (getenv("FAILURE_BEFORE_EXEC")) {
      pid = -1; 
      strcpy(err_str, "FAILURE_BEFORE_EXEC");
   } else {
      pid = sge_exec_job(jep, jatep, petep, err_str);
   }   

   if (pid < 0) {
      switch (pid) {
         case -1:
            execd_job_start_failure(jep, jatep, petep, err_str, GFSTATE_NO_HALT);
            break;
         case -2:
            execd_job_start_failure(jep, jatep, petep, err_str, GFSTATE_QUEUE);
            break;
         case -3:
            execd_job_start_failure(jep, jatep, petep, err_str, GFSTATE_JOB);
            break;
         default:
            execd_job_start_failure(jep, jatep, petep, err_str, GFSTATE_HOST);
            break;
      }
      DEXIT;
      return 0;
   }
   DTIMEPRINTF(("TIME IN EXECD FOR STARTING THE JOB: " u32 "\n",
                sge_get_gmt()-now));
   
   if(petep != NULL) {
      lSetUlong(petep, PET_pid, pid);
   } else {
      lSetUlong(jatep, JAT_pid, pid);
   }

   DPRINTF(("***EXECING "u32"."u32" on %s (tid = %s) (pid = %d)\n",
            job_id, ja_task_id, 
            uti_state_get_unqualified_hostname(), pe_task_id != NULL ? pe_task_id : "null", pid));

   /* when a ja_task or pe_task has been started, flush the job report */
   {
      lListElem *jr = get_job_report(job_id, ja_task_id, pe_task_id);
      if (jr == NULL) {
         jr = add_job_report(job_id, ja_task_id, pe_task_id, jep);
      }
      flush_job_report(jr);
   }

   DEXIT;
   return 1;
}

#ifdef COMPILE_DC
int register_at_ptf(
lListElem *job,
lListElem *ja_task,
lListElem *pe_task 
) {
   u_long32 job_id;   
   u_long32 ja_task_id;   
   const char *pe_task_id = NULL;

   int success, newerrno;
   FILE *fp;
   SGE_STRUCT_STAT sb;

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX) 
   gid_t addgrpid;
   dstring addgrpid_path = DSTRING_INIT;
#else   
   dstring osjobid_path = DSTRING_INIT;
   osjobid_t osjobid;   
#endif   
   DENTER(TOP_LAYER, "register_at_ptf");

   job_id = lGetUlong(job, JB_job_number);
   ja_task_id = lGetUlong(ja_task, JAT_task_number);
   if (pe_task != NULL) { 
      pe_task_id = lGetString(pe_task, PET_id);
   }

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
   /**
    ** read additional group id and use it as osjobid 
    **/
   
   /* open addgrpid file */
   sge_get_active_job_file_path(&addgrpid_path,
                                job_id, ja_task_id, pe_task_id, ADDGRPID);
   DPRINTF(("Registering job %s with PTF\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id)));

   if (SGE_STAT(sge_dstring_get_string(&addgrpid_path), &sb) && errno == ENOENT) {
      DPRINTF(("still waiting for addgrpid of job %s\n", 
         job_get_id_string(job_id, ja_task_id, pe_task_id)));
      sge_dstring_free(&addgrpid_path);       
      DEXIT;
      return(1);
   }  

   if (!(fp = fopen(sge_dstring_get_string(&addgrpid_path), "r"))) {
      ERROR((SGE_EVENT, MSG_EXECD_NOADDGIDOPEN_SSS, sge_dstring_get_string(&addgrpid_path), 
             job_get_id_string(job_id, ja_task_id, pe_task_id), strerror(errno)));
      sge_dstring_free(&addgrpid_path);       
      DEXIT;
      return(-1);
   }
  
   sge_dstring_free(&addgrpid_path);       

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
      if ((ptf_error = ptf_job_started(addgrpid, pe_task_id, job, ja_task_id))) {
         ERROR((SGE_EVENT, MSG_JOB_NOREGISTERPTF_SS, 
                job_get_id_string(job_id, ja_task_id, pe_task_id), 
                ptf_errstr(ptf_error)));
         DEXIT;
         return (1);
      }
   }

   /* store addgrpid in job report to be sent to qmaster later on */
   {
      char addgrpid_str[64];
      lListElem *jr;

      sprintf(addgrpid_str, pid_t_fmt, addgrpid);
      if ((jr=get_job_report(job_id, ja_task_id, pe_task_id)))
         lSetString(jr, JR_osjobid, addgrpid_str); 
      DPRINTF(("job %s: addgrpid = %s\n", 
               job_get_id_string(job_id, ja_task_id, pe_task_id), addgrpid_str));
   }
#else
   /* read osjobid if possible */
   sge_get_active_job_file_path(&osjobid_path,
                                job_id, ja_task_id, pe_task_id, OSJOBID);
   
   DPRINTF(("Registering job %s with PTF\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id)));

   if (SGE_STAT(sge_dstring_get_string(&osjobid_path), &sb) && errno == ENOENT) {
      DPRINTF(("still waiting for osjobid of job %s\n", 
            job_get_id_string(job_id, ja_task_id, pe_task_id)));
      sge_dstring_free(&osjobid_path);      
      DEXIT;
      return 1;
   } 

   if (!(fp=fopen(sge_dstring_get_string(&osjobid_path), "r"))) {
      ERROR((SGE_EVENT, MSG_EXECD_NOOSJOBIDOPEN_SSS, sge_dstring_get_string(&osjobid_path), 
             job_get_id_string(job_id, ja_task_id, pe_task_id), 
             strerror(errno)));
      sge_dstring_free(&osjobid_path);      
      DEXIT;
      return -1;
   }

   sge_dstring_free(&osjobid_path);      

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
      if ((ptf_error = ptf_job_started(osjobid, pe_task_id, job, ja_task_id))) {
         ERROR((SGE_EVENT, MSG_JOB_NOREGISTERPTF_SS,  
                job_get_id_string(job_id, ja_task_id, pe_task_id), 
                ptf_errstr(ptf_error)));
         DEXIT;
         return -1;
      }
   }

   /* store osjobid in job report to be sent to qmaster later on */
   {
      char osjobid_str[64];
      lListElem *jr;

      sprintf(osjobid_str, OSJOBID_FMT, osjobid);
      if ((jr=get_job_report(job_id, ja_task_id, pe_task_id)))
         lSetString(jr, JR_osjobid, osjobid_str); 
      DPRINTF(("job %s: osjobid = %s\n", 
               job_get_id_string(job_id, ja_task_id, pe_task_id),
               osjobid_str));
   }
#endif

   DEXIT;
   return 0;
}
#endif

static bool should_reprioritize(void)
{
   lListElem *confl = NULL; 
   lList *ep_list = NULL;
   lListElem *ep = NULL; 
   bool ret = true;

   DENTER(TOP_LAYER, "should_reprioritize");

   confl = lCopyElem(lGetElemHost(Execd_Config_List, CONF_hname, "global"));

   if (confl) {
      ep_list = lGetList(confl, CONF_entries);
   }
    
   if (ep_list) {
      ep = lGetElemStr(ep_list, CF_name, REPRIORITIZE);
   }

   if (ep)
   {
      const char* value;
      value = lGetString(ep, CF_value);
      ret = strncasecmp(value, "0", sizeof("0"));
   }
   else
   {
      ret = conf.reprioritize;
   }

   if (NULL != confl) {
      confl = lFreeElem(confl);
   }

   DEXIT;
   return ret;
} /* should_reprioritize */

