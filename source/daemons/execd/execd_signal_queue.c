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
#include <stdlib.h>
#include <string.h>

#include "sgermon.h"
#include "def.h"
#include "sge.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_queueL.h"
#include "sge_parse_num_par.h"
#include "dispatcher.h"
#include "reaper_execd.h"
#include "sge_log.h"
#include "admin_mail.h"
#include "sge_signal.h"
#include "job_log.h"
#include "commlib.h"
#include "execd_signal_queue.h"
#include "sig_handlers.h"
#include "read_write_job.h"
#include "sge_prog.h"
#include "symbols.h"
#include "sge_time.h"
#include "job_report_execd.h"
#include "msg_execd.h"
#include "sge_job_jatask.h"
#include "sge_unistd.h"
#include "sge_conf.h"
#include "sge_job_reportL.h"
#include "sge_usageL.h"

#if defined(CRAY) && !defined(SIGXCPU)
#   define SIGXCPU SIGCPULIM
#endif

extern volatile int shut_me_down;
extern lList *Master_Job_List;

/**************************************************************************
 called from dispatcher

 counterpart in qmaster: c_qmod.c
 **************************************************************************/
int execd_signal_queue(de, pb, apb, rcvtimeout, synchron, err_str, answer_error)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
u_long *rcvtimeout; 
int *synchron; 
char *err_str; 
int answer_error;
{
   lListElem *jep;
   int found = 0;
   u_long32 jobid, signal, jataskid;
   char *qname;

   DENTER(TOP_LAYER, "execd_signal_queue");

   unpackint(pb, &jobid);
   unpackint(pb, &jataskid);
   unpackstr(pb, &qname);       /* mallocs qname !! */
   unpackint(pb, &signal);

   DPRINTF(("===>DELIVER_SIGNAL: %s >%s< Job(s) "u32"."u32" \n",
            sge_sig2str(signal), qname, jobid, jataskid));

   /* In real this is both a signal queue and a signal job request.
      If jobid is set this is a job signal.
         qname is set this is a queue signal.
      Acknowledges are sent allready by the dispatcher. */

   if (jobid) {     /* signal a job / task */
      found = (signal_job(jobid, jataskid, signal)==0);
   } else {            /* signal a queue */
      for_each(jep, Master_Job_List) {
         lListElem *gdil_ep, *master_q, *jatep;
         const char *qnm;

         for_each (jatep, lGetList(jep, JB_ja_tasks)) {

            if (lGetUlong(jatep, JAT_status) == JSLAVE) 
               break;

            /* iterate through all queues of a parallell job -
               this is done to ensure that signal delivery is also
               forwarded to the job in case the master queue keeps still active */
            for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
               master_q = lFirst(lGetList(gdil_ep, JG_queue));
               qnm =  lGetString(master_q, QU_qname);
               if (!strcmp(qname, qnm)) {
                  char tmpstr[SGE_PATH_MAX];

                  /* job signaling triggerd by a queue signal */
                  sprintf(tmpstr, "%s (%s)", sge_sig2str(signal), qnm);
                  job_log(lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number), tmpstr);
                  /* if the queue gets suspended and the job is already suspended
                     we do not deliver a signal */
                  if (signal == SGE_SIGSTOP) {
                     lSetUlong(master_q, QU_state, lGetUlong(master_q, QU_state) | 
                           QSUSPENDED);
                     if (!VALID(JSUSPENDED, lGetUlong(jatep, JAT_state))) {
                        if (lGetUlong(jep, JB_checkpoint_attr)& CHECKPOINT_SUSPEND) {
                           INFO((SGE_EVENT, MSG_JOB_INITMIGRSUSPQ_U, u32c(lGetUlong(jep, JB_job_number))));
                           signal = SGE_MIGRATE;
                        }   
                        sge_execd_deliver_signal(signal, jep, jatep);
                     }   
                  } 
                  else {
                     /* if the signal is a unsuspend and the job is suspended
                        we do not deliver a signal */
                     if (signal == SGE_SIGCONT) {
                        lSetUlong(master_q, QU_state, lGetUlong(master_q, QU_state) & ~QSUSPENDED);
                        if (!VALID(JSUSPENDED, lGetUlong(jatep, JAT_state)))
                           sge_execd_deliver_signal(signal, jep, jatep);
                     }
                     else
                        sge_execd_deliver_signal(signal, jep, jatep);
                  }
                  found = lGetUlong(jep, JB_job_number);

                  job_write_spool_file(jep, 
                     lGetUlong(lFirst(lGetList(jep, JB_ja_tasks)), 
                     JAT_task_number), SPOOL_WITHIN_EXECD);

               }
            }
         }
      }
      /*
      ** when a queue state has changed
      ** we release the block on sending admin mails
      */
      adm_mail_reset(BIT_ADM_QCHANGE);
   }

   /* If this is a queue signal 'found' now holds the number of a job
      running in this queue. We use this job_number for acking to the
      qmaster. */

   if (!found && jobid) {
      lListElem *jr;
      jr = get_job_report(jobid, jataskid, NULL);
      remove_acked_job_exit(jobid, jataskid, jr);
      job_unknown(jobid, jataskid, qname);
   }

   if (qname)
      free(qname);
   DEXIT;
   return 0;
}

/*************************************************************************
 execds function to deliver a signal to the job. This cant be done direct,
 because there is the shepherd between.
 We do a signal mapping for the shepherd.

 sent to shepherd  |  signal to deliver to job
 ------------------|---------------------------
 SIGTTIN           | given in "signal"-file
 SIGUSR1           | SIGUSR1
 SIGUSR2           | SIGXCPU
 SIGCONT           | SIGCONT
 SIGWINCH          | SIGSTOP
 SIGTSTP           | SIGKILL

 SIGTTIN forces the shepherd to look into the "signal" file. This file
 contains the number of the signal to send.

 The job will get a SIGUSR1 as notification for SIGSTOP and
                    SIGUSR2                     SIGKILL if
 The job has been submitted with -notify and the notification time of the
 queue is > 0.

 returns 
   0 on success
   1 if job is supposed to be not in a healthy state and thus
     should be removed by the calling context
 *************************************************************************/
int sge_execd_deliver_signal(
u_long32 sig,
lListElem *jep,
lListElem *jatep 
) {
   lListElem *tep;
   int queue_already_suspended;
   int getridofjob = 0;

   DENTER(TOP_LAYER, "sge_execd_deliver_signal");

   INFO((SGE_EVENT, MSG_JOB_SIGNALTASK_UUS,   
         u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number)), 
         sge_sig2str(sig)));

   /* for simulated hosts do nothing */
   if(simulate_hosts == 1 && 
      (lGetUlong(jatep, JAT_status) & JSIMULATED)) {

      if(sig == SGE_SIGKILL) {
         lListElem *jr = NULL;
         u_long32 jobid, jataskid;
         u_long32 wallclock;

         jobid = lGetUlong(jep, JB_job_number);
         jataskid = lGetUlong(jatep, JAT_task_number);
         
         DPRINTF(("Simulated job "u32"."u32" is killed\n", jobid, jataskid));

         if ((jr=get_job_report(jobid, jataskid, NULL)) == NULL) {
            ERROR((SGE_EVENT, MSG_JOB_MISSINGJOBXYINJOBREPORTFOREXITINGJOBADDINGIT_UU, 
                   u32c(jobid), u32c(jataskid)));
            jr = add_job_report(jobid, jataskid, NULL);
         }

         lSetUlong(jr, JR_state, JEXITING);
         lSetUlong(jep, JB_end_time, sge_get_gmt());
         add_usage(jr, "submission_time", NULL, lGetUlong(jep, JB_submission_time));
         add_usage(jr, "start_time", NULL, lGetUlong(jatep, JAT_start_time));
         add_usage(jr, "end_time", NULL, lGetUlong(jep, JB_end_time));
         wallclock = lGetUlong(jep, JB_end_time) - lGetUlong(jatep, JAT_start_time);
         add_usage(jr, "ru_wallclock", NULL, wallclock);
         add_usage(jr, USAGE_ATTR_CPU_ACCT, NULL, wallclock * 0.5);
         add_usage(jr, "ru_utime", NULL, wallclock * 0.4 );
         add_usage(jr, "ru_utime", NULL, wallclock * 0.1 );
         add_usage(jr, "exit_status", NULL, 137);
         add_usage(jr, "signal", NULL, sig);

         lSetUlong(jatep, JAT_status, JEXITING | JSIMULATED);
         flush_jr = 1;
      }
      
      return 0;
   }

/*
   DPRINTF(("(sig==SGE_MIGRATE) = %d (ckpt on suspend) = %d %d\n", 
      (sig == SGE_MIGRATE), lGetUlong(jep, JB_checkpoint_attr)|CHECKPOINT_SUSPEND, 
         lGetUlong(jep, JB_checkpoint_attr)));
*/
   /* Simply apply signal to all subtasks of the job 
      except in case of SGE_MIGRATE when there is a 
      ckpt env with "migrate on suspend" configured */
   queue_already_suspended = (lGetUlong(jatep, JAT_state)&JSUSPENDED);
   if (!(sig == SGE_MIGRATE 
         && (lGetUlong(jep, JB_checkpoint_attr)|CHECKPOINT_SUSPEND)) 
         && !queue_already_suspended)
      /* signal each pe task */
      for_each (tep, lGetList(jatep, JAT_task_list)) {
         if (sge_kill((int)lGetUlong(lFirst(lGetList(tep, JB_ja_tasks)), JAT_pid), sig, 
            lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number), 
            lGetString(tep, JB_pe_task_id_str))==-2)
            getridofjob = 1;
      }
   if (lGetUlong(jatep, JAT_status)!=JSLAVE)
      if (sge_kill((int)lGetUlong(jatep, JAT_pid), sig, lGetUlong(jep, JB_job_number), 
                        lGetUlong(jatep, JAT_task_number), NULL)==-2)
         getridofjob = 1;

   DEXIT;
   return getridofjob;
}

/***********************************************************************
   forward signal to shepherd

   returns
       0  on success
       -2 if shepherd process is not (or no longer?) here
       -1 in case of other problems
*/

int sge_kill(
int pid,
u_long32 sge_signal,
u_long32 jobid,
u_long32 jataskid,
const char *petask 
) {
   int sig;
   int status=0;
   int direct_signal;   /* deliver per signal or per file */
   char fname[SGE_PATH_MAX];
   char task_job[200];
   FILE *fp;

   DENTER(TOP_LAYER, "sge_kill");

   if (!pid) {
      DPRINTF(("sge_kill won't kill pid 0!\n"));
      DEXIT;
      return -1;
   }

   /* mapping from SGE_SIG... which is equal on all platforms to the platform
      specific SIG... */
   sig = sge_unmap_signal(sge_signal);

   /*  now do the mapping for the shepherd 

       sent to shepherd  |  shepherd signal to deliver to job
       ------------------|---------------------------
       SIGTTIN           | given in "signal"-file 
       SIGTTOU           | SIGTTOU (initiate migration  - shepherd knows the signal)
       SIGUSR1           | SIGUSR1
       SIGUSR2           | SIGXCPU
       SIGCONT           | SIGCONT
       SIGWINCH          | SIGSTOP
       SIGTSTP           | SIGKILL
   */ 
   direct_signal = 1;

   switch (sig) {
   case SIGSTOP:
      sig = SIGWINCH;
      break;
   case SIGKILL:
      sig = SIGTSTP;
      break;
#if defined(SIGXCPU)
   case SIGXCPU:
      sig = SIGUSR2;
      break;
#endif
   case SIGTTOU:
   case SIGUSR1:
   case SIGCONT:
      break;
   default:
      direct_signal = 0;        /* communication has to be done via file */
   }

   DPRINTF(("signalling pid "pid_t_fmt" with %d\n", pid, sig));
   if (petask)
      sprintf(task_job, u32"."u32"/%s", jobid, jataskid, petask);
   else 
      sprintf(task_job, u32"."u32, jobid, jataskid);

   if (!direct_signal) {
      sprintf(fname, "%s/%s/signal", ACTIVE_DIR, task_job);
      if (!(fp = fopen(fname, "w"))) {
         ERROR((SGE_EVENT, MSG_EXECD_WRITESIGNALFILE_S, fname));
         goto CheckShepherdStillRunning;
      } 

      fprintf(fp, "%d\n", sig);
      fclose(fp);
   }

   /*
   ** NECSX4 && NECSX5 !!!
   **
   ** execd.uid==0 && execd.euid==admin_user
   **    => kill does neither send SIGCONT-signals nor return an error
   */
#if defined(NECSX4) || defined(NECSX5)
   sge_switch2start_user();
#endif    
   if ((status = kill(pid, direct_signal?sig:SIGTTIN))) {
#if defined(NECSX4) || defined(NECSX5)
      sge_switch2admin_user();
#endif   
      if (errno == ESRCH)
         goto CheckShepherdStillRunning;
      DEXIT; 
      return -1;
   }
#if defined(NECSX4) || defined(NECSX5)
   sge_switch2admin_user();
#endif 
   
   DEXIT;
   return 0;

CheckShepherdStillRunning:
   {
      char path[SGE_PATH_MAX];
      SGE_STRUCT_STAT statbuf;

      sprintf(path, "%s/%s", ACTIVE_DIR, task_job);
      if (!SGE_STAT(path, &statbuf) && S_ISDIR(statbuf.st_mode)) {
         dead_children = 1; /* may be we've lost a SIGCHLD */
         DEXIT;
         return 0;
      } else {
         WARNING((SGE_EVENT, MSG_JOB_DELIVERSIGNAL_ISSIS,
                sig, task_job, sge_sig2str(sge_signal), pid, strerror(errno)));
         DEXIT;
         return -2;
      }
   }
}

/*------------------------------------------------------------ 

NAME

   signal_job()

DESCRIPTION

   Tries to signal the job. 

RETURN

   0 job was found and was signaled
   1 job was not found you better get rid of it to prevent 
     infinite pingpong effects
   ------------------------------------------------------------ */
int signal_job(
u_long32 jobid,
u_long32 jataskid,
u_long32 signal 
) {
   lListElem *jep;
   u_long32 state;
   lListElem *master_q;
   lListElem *jatep = NULL;
   int getridofjob = 0;
   const void *iterator;

   DENTER(TOP_LAYER, "signal_job");

   job_log(jobid, jataskid, sge_sig2str(signal));

   /* search appropriate array task and job */
   jep = lGetElemUlongFirst(Master_Job_List, JB_job_number, jobid, &iterator);
   while(jep != NULL) {
      jatep = job_search_task(jep, NULL, jataskid, 0);

      if(jatep != NULL) {
         break;
      }
      jep = lGetElemUlongNext(Master_Job_List, JB_job_number, jobid, &iterator);
   }

   if (!jatep) {
      DEXIT;
      return 1;
   }

   master_q = lFirst(lGetList(lFirst(lGetList(jatep, 
      JAT_granted_destin_identifier_list)), JG_queue));

   DPRINTF(("sending %s to job "u32"."u32"\n", 
         sge_sig2str(signal), jobid, jataskid));
   if (signal == SGE_SIGCONT) {
      state = lGetUlong(jatep, JAT_state);
      CLEARBIT(JSUSPENDED, state);
      SETBIT(JRUNNING, state);
      lSetUlong(jatep, JAT_state, state);

      /* If the one of the queues is suspended 
         and we unsuspend the job. 
         The Job should stay sleeping */

      if (!(lGetUlong(master_q, QU_state) & QSUSPENDED)) 
         getridofjob = sge_execd_deliver_signal(signal, jep, jatep);
      else 
         DPRINTF(("Queue is suspended -> do nothing\n"));
   }
   else {
      if ((signal == SGE_SIGSTOP) && (lGetUlong(jep, JB_checkpoint_attr) & 
            CHECKPOINT_SUSPEND)) {
         INFO((SGE_EVENT, MSG_JOB_INITMIGRSUSPJ_UU, 
               u32c(lGetUlong(jep, JB_job_number)), u32c(lGetUlong(jatep, JAT_task_number))));
         signal = SGE_MIGRATE;
         getridofjob = sge_execd_deliver_signal(signal, jep, jatep);
      }
      else if (signal == SGE_SIGSTOP) {
         state = lGetUlong(jatep, JAT_state);
         SETBIT(JSUSPENDED, state);
         CLEARBIT(JRUNNING, state);
         lSetUlong(jatep, JAT_state, state);

         /* if this is a stop signal for a job 
            which is in at least ONE queue
            which is already stopped we 
            do not deliver the signal */

            getridofjob = sge_execd_deliver_signal(signal, jep, jatep);
      } 
      else {
         if (signal == SGE_SIGKILL) {
            state = lGetUlong(jatep, JAT_state);
            SETBIT(JDELETED, state);
            lSetUlong(jatep, JAT_state, state); 
            DPRINTF(("SIGKILL of job "u32"\n", jobid)); 
         }
         getridofjob = sge_execd_deliver_signal(signal, jep, jatep);
      }
   }

   /* now save this job/queue so we are up to date on restart */
   if (!getridofjob)
      job_write_spool_file(jep, jataskid, SPOOL_WITHIN_EXECD);
   else
      DPRINTF(("Job  "u32"."u32" is no longer running\n", jobid, jataskid));

   DEXIT;
   return getridofjob;
}
