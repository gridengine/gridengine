
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
#if defined(ALPHA) || defined(SOLARIS)
#  include <sys/param.h>        /* for MAX() macro */
#endif

#if defined(NECSX4) || defined(NECSX5)
#  include <sys/types.h>
#  include <sys/disp.h>
#endif

#include <sys/types.h>

#include "sgermon.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_switch_user.h"
#include "sge.h"
#include "job.h"
#include "basis_types.h"
#include "sge_language.h"
#include "sge_conf.h"
#include "msg_common.h"
#include "msg_execd.h"
#include "sgedefs.h"
#include "exec_ifm.h"
#include "pdc.h"
#include "sge_feature.h"

#if defined(COMPILE_DC) || defined(MODULE_TEST)

#if defined(IRIX6) || defined(ALPHA) || defined(LINUX) || defined(SOLARIS) || defined(NECSX4) || defined(NECSX5) || !defined(MODULE_TEST)
#   define USE_DC
#endif

#define PTF_DO_BACKGROUND_JOBS

/* #define PTF_DYNAMIC_PRIORITY_ADJUSTMENT */

/* #define PTF_DYNAMIC_USAGE_ADJUSTMENT */
#define PTF_NEW_ALGORITHM

#define SET_TIMESTRUC_FROM_MS(ts, n) { \
   ts.tv_sec = (n) / 1000; \
   ts.tv_nsec = ((n) % 1000)*1000000; \
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#ifdef __sgi
#  include <sys/resource.h>
#  include <sys/systeminfo.h>
#  ifdef IRIX64
#     include <sys/sched.h>
#  endif
#endif

#ifdef CRAY
#  include <sys/category.h>
#endif

#if defined(ALPHA) || defined(SOLARIS) || defined(LINUX)
#  include <sys/resource.h>
#endif

#if defined(SOLARIS)
int setpriority(int which, id_t who, int prio);
#endif

#ifdef MODULE_TEST
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <sys/signal.h>
#   if ( IRIX6 || IRIX64 || ALPHA || SOLARIS )
#      include <sys/fault.h>
#   endif
#   include <sys/syscall.h>
#   include <sys/procfs.h>
#endif

#include "sge_all_listsL.h"
#include "sge_copy_append.h"
#include "commlib.h"

#ifdef USE_DC
#   include "sgedefs.h"
#   include "exec_ifm.h"
#endif

#include "ptf.h"

/*
 *
 * PTF Data Structures
 *
 *    job_ticket_list     list of jobs and their associated tickets which is
 *                        sent from the SGE scheduler to the PTF.
 *
 *    job_ticket_entry    contains the job ID and job tickets for each job.
 *
 *    job_list            local list of jobs and all the associated attributes
 *
 *    job                 entry contained in the job list.
 *
 *
 * Notes:
 *
 *   Make sure JL_usage is always set to at least PTF_MIN_JOB_USAGE
 *
 *   Where do we create the usage list?  Probably in the routine that
 *   we get the usage from the Data Collector.
 *
 *   When does usage get reset to PTF_MIN_JOB_USAGE?  Possibly, upon
 *   receiving a new job tickets list.
 *
 *   When do we update the usage in the qmaster?  Possibly, after
 *   receiving a new job tickets list.
 *
 *   Make sure we don't delete a job from the job list until all the
 *   usage has been collected and reported back to the qmaster.
 *
 */

#define INCPTR(type, ptr, nbyte) ptr = (type *)((char *)ptr + nbyte)

#define INCJOBPTR(ptr, nbyte) INCPTR(struct psJob_s, ptr, nbyte)

#define INCPROCPTR(ptr, nbyte) INCPTR(struct psProc_s, ptr, nbyte)

static void ptf_calc_job_proportion_pass0(lListElem *job,
                                          u_long * sum_of_job_tickets,
                                          double *sum_of_last_usage);

static void ptf_calc_job_proportion_pass1(lListElem *job,
                                          u_long sum_of_job_tickets,
                                          double sum_of_last_usage,
                                          double *sum_proportion);

static void ptf_calc_job_proportion_pass2(lListElem *job,
                                          u_long sum_of_job_tickets,
                                          double sum_proportion,
                                          double *sum_adjusted_proportion,
                                          double *sum_last_interval_usage);

static void ptf_calc_job_proportion_pass3(lListElem *job,
                                          double sum_adjusted_proportion,
                                          double sum_last_interval_usage,
                                          double *min_share,
                                          double *max_share);

static void ptf_set_OS_scheduling_parameters(lList *job_list, double min_share,
                                             double max_share);

static void ptf_get_usage_from_data_collector(void);

static lListElem *ptf_process_job(osjobid_t os_job_id,
                                  char *task_id_str,
                                  lListElem *new_job, u_long32 jataskid);

static lListElem *ptf_get_job_os(lList *job_list, osjobid_t os_job_id,
                                 lListElem **job_elem);

static void ptf_set_job_priority(lListElem *job);

static lList *_ptf_get_job_usage(lListElem *job, u_long ja_task_id,
                                 char *task_id);

static osjobid_t ptf_get_osjobid(lListElem *osjob);


static void ptf_set_osjobid(lListElem *osjob, osjobid_t osjobid);

static void ptf_set_native_job_priority(lListElem *job, lListElem *osjob,
                                        long pri);

static lList *ptf_build_usage_list(char *name, lList *old_usage_list);

static lListElem *ptf_get_job(u_long job_id);

#if defined(__sgi) && !defined(IRIX5)

static void ptf_setpriority_ash(lListElem *job, lListElem *osjob, long pri);

#elif defined(CRAY) || defined(NECSX4) || defined(NECSX5)

static void ptf_setpriority_jobid(lListElem *job, lListElem *osjob, long pri);

#elif defined(ALPHA) || defined(SOLARIS) || defined(LINUX)

static void ptf_setpriority_addgrpid(lListElem *job, lListElem *osjob,
                                     long pri);

#endif

#if 0

static int ptf_is_job_complete(u_long job_id, int *job_complete);

static int ptf_get_pids(u_long jobid, lList **pid_list);

static int ptf_kill(u_long jobid, int sig);

#endif

static double ptf_compensation_factor = PTF_COMPENSATION_FACTOR;

static lList *ptf_jobs = NULL;

static char ptf_error_str[256];

static int is_ptf_running = 0;

#if defined(__sgi)

static char irix_release[257] = "6.2";

static int got_release = 0;

#endif

/****** ptf/ptf_get_osjobid() *************************************************
*  NAME
*     ptf_get_osjobid() -- return the job id 
*
*  SYNOPSIS
*     static osjobid_t ptf_get_osjobid(lListElem *osjob) 
*
*  FUNCTION
*     The function returns the os job id contained in the CULL element
*
*  INPUTS
*     lListElem *osjob - element of type JO_Type 
*
*  RESULT
*     static osjobid_t - os job id (job id / ash / supplementary gid)
******************************************************************************/
static osjobid_t ptf_get_osjobid(lListElem *osjob)
{
   osjobid_t osjobid;

#if !defined(LINUX) && !defined(SOLARIS) && !defined(ALPHA5) && !defined(NECSX4) && !defined(NECSX5)

   osjobid = lGetUlong(osjob, JO_OS_job_ID2);
   osjobid = (osjobid << 32) + lGetUlong(osjob, JO_OS_job_ID);

#else

   osjobid = lGetUlong(osjob, JO_OS_job_ID);

#endif

   return osjobid;
}

/****** ptf/ptf_set_osjobid() *************************************************
*  NAME
*     ptf_set_osjobid() -- set os job id 
*
*  SYNOPSIS
*     static void ptf_set_osjobid(lListElem *osjob, osjobid_t osjobid) 
*
*  FUNCTION
*     Set the attribute of "osjob" containing the os job id
*
*  INPUTS
*     lListElem *osjob  - element of type JO_Type 
*     osjobid_t osjobid - os job id (job id / ash / supplementary gid) 
******************************************************************************/
static void ptf_set_osjobid(lListElem *osjob, osjobid_t osjobid)
{
#if !defined(LINUX) && !defined(SOLARIS) && !defined(ALPHA5) && !defined(NECSX4) && !defined(NECSX5)

   lSetUlong(osjob, JO_OS_job_ID2, ((u_osjobid_t) osjobid) >> 32);
   lSetUlong(osjob, JO_OS_job_ID, osjobid & 0xffffffff);

#else

   lSetUlong(osjob, JO_OS_job_ID, osjobid);

#endif
}

/****** ptf/ptf_build_usage_list() ********************************************
*  NAME
*     ptf_build_usage_list() -- create a new usage list from an existing list 
*
*  SYNOPSIS
*     static lList* ptf_build_usage_list(char *name, lList *old_usage_list) 
*
*  FUNCTION
*     This method creates a new usage list or makes a copy of a old
*     usage list and zeros out the usage values. 
*
*  INPUTS
*     char *name            - name of the new list 
*     lList *old_usage_list - old usage list or NULL 
*
*  RESULT
*     static lList* - copy of "old_usage_list" or a real new one 
******************************************************************************/
static lList *ptf_build_usage_list(char *name, lList *old_usage_list)
{
   lList *usage_list;
   lListElem *usage;

   if (old_usage_list) {
      usage_list = lCopyList(name, old_usage_list);
      for_each(usage, usage_list) {
         lSetDouble(usage, UA_value, 0);
      }
   } else {
      usage_list = lCreateList(name, UA_Type);

      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_IO);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);

      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_IOW);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);

      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_MEM);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);

      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_CPU);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);

#if defined(ALPHA) || defined(LINUX) || defined(SOLARIS)
      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_VMEM);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);

      usage = lCreateElem(UA_Type);
      lSetString(usage, UA_name, USAGE_ATTR_HIMEM);
      lSetDouble(usage, UA_value, 0);
      lAppendElem(usage_list, usage);
#endif
   }

   return usage_list;
}

/****** ptf/ptf_reinit_queue_priority() ***************************************
*  NAME
*     ptf_reinit_queue_priority() -- set static priority 
*
*  SYNOPSIS
*     void ptf_reinit_queue_priority(u_long32 job_id, u_long32 ja_task_id, 
*                                    char *pe_task_id_str, u_long32 priority) 
*
*  FUNCTION
*     If execd switches from SGEEE to SGE mode this functions is used to
*     reinitialize static priorities of all jobs currenty running.
*
*  INPUTS
*     u_long32 job_id      - job id 
*     u_long32 ja_task_id  - task number 
*     char *pe_task_id_str - pe task id string or NULL 
*     u_long32 priority    - new static priority 
******************************************************************************/
void ptf_reinit_queue_priority(u_long32 job_id, u_long32 ja_task_id,
                               char *pe_task_id_str, u_long32 priority)
{
   lListElem *job_elem;
   DENTER(TOP_LAYER, "ptf_reinit_queue_priority");

   if (!job_id || !ja_task_id) {
      DEXIT;
      return;
   }

   for_each(job_elem, ptf_jobs) {
      lList *os_job_list;
      lListElem *os_job;

      if (lGetUlong(job_elem, JL_job_ID) == job_id) {
         DPRINTF(("\tjob id: " u32 "\n", lGetUlong(job_elem, JL_job_ID)));
         os_job_list = lGetList(job_elem, JL_OS_job_list);
         for_each(os_job, os_job_list) {
            if (lGetUlong(os_job, JO_ja_task_ID) == ja_task_id &&
                ((!pe_task_id_str && !lGetString(os_job, JO_task_id_str))
                 || (pe_task_id_str && lGetString(os_job, JO_task_id_str) &&
                     !strcmp(pe_task_id_str,
                             lGetString(os_job, JO_task_id_str))))) {

               DPRINTF(("\t\tChanging priority for osjobid: " u32 " jatask "
                        u32 " petask %s\n",
                        lGetUlong(os_job, JO_OS_job_ID),
                        lGetUlong(os_job, JO_ja_task_ID),
                        pe_task_id_str ? pe_task_id_str : ""));

               ptf_set_native_job_priority(job_elem, os_job,
                                           PTF_PRIORITY_TO_NATIVE_PRIORITY
                                           (priority));
            }
         }
      }
   }
   DEXIT;
}

/****** ptf/ptf_set_job_priority() ********************************************
*  NAME
*     ptf_set_job_priority() -- Update job priority 
*
*  SYNOPSIS
*     static void ptf_set_job_priority(lListElem *job) 
*
*  FUNCTION
*     The funktion updates the priority of each process which belongs
*     to "job". The attribute JL_pri of "job" specifies the new priority.
*
*  INPUTS
*     lListElem *job - JL_Type 
******************************************************************************/
static void ptf_set_job_priority(lListElem *job)
{
   lListElem *osjob;
   long pri = lGetLong(job, JL_pri);

   DENTER(TOP_LAYER, "ptf_set_job_priority");

   for_each(osjob, lGetList(job, JL_OS_job_list)) {
      ptf_set_native_job_priority(job, osjob, pri);
   }
   DEXIT;
}

/****** ptf/ptf_set_native_job_priority() *************************************
*  NAME
*     ptf_set_native_job_priority() -- Change job priority  
*
*  SYNOPSIS
*     static void ptf_set_native_job_priority(lListElem *job, lListElem *osjob, 
*                                             long pri) 
*
*  FUNCTION
*     The function updates the priority of each process which belongs
*     to "job" and "osjob".
*
*  INPUTS
*     lListElem *job   - job 
*     lListElem *osjob - one of the os jobs of "job" 
*     long pri         - new priority value 
******************************************************************************/
static void ptf_set_native_job_priority(lListElem *job, lListElem *osjob, 
                                        long pri)
{
#if defined(__sgi) && !defined(IRIX5)
   ptf_setpriority_ash(job, osjob, pri);
#elif defined(CRAY) || defined(NECSX4) || defined(NECSX5)
   ptf_setpriority_jobid(job, osjob, pri);
#elif defined(ALPHA) || defined(SOLARIS) || defined(LINUX)
   ptf_setpriority_addgrpid(job, osjob, pri);
#endif
}

#if defined(__sgi) && !defined(IRIX5)

/****** ptf/ptf_setpriority_ash() *********************************************
*  NAME
*     ptf_setpriority_ash() -- Change priority of processes 
*
*  SYNOPSIS
*     static void ptf_setpriority_ash(lListElem *job, lListElem *osjob, 
*        long *pri) 
*
*  FUNCTION
*     This function is only available for the architecture IRIX6.
*     All processes belonging to "job" and "osjob" will get a new priority.
*
*  INPUTS
*     lListElem *job   - job  
*     lListElem *osjob - one of the os jobs of "job" 
*     long pri         - new priority 
******************************************************************************/
static void ptf_setpriority_ash(lListElem *job, lListElem *osjob, long pri)
{
   static int got_bg_flag = 0;
   static int bg_flag;
   lListElem *pid;

#  if defined(IRIX6)
   int nprocs;
   static int first = 1;
#  endif

   DENTER(TOP_LAYER, "ptf_setpriority_ash");

#  if defined(IRIX6)
   if (first) {
      nprocs = sysmp(MP_NPROCS);
      if (nprocs <= 0)
         nprocs = 1;
      first = 0;
   }
#  endif

   if (!got_bg_flag) {
      bg_flag = (getenv("PTF_NO_BACKGROUND_PRI") == NULL);
      got_bg_flag = 1;
   }
#  if defined(__sgi)

   if (!got_release) {
      if (sysinfo(SI_RELEASE, irix_release, sizeof(irix_release)) < 0) {
         ERROR((SGE_EVENT, MSG_SYSTEM_SYSINFO_SI_RELEASE_CALL_FAILED_S,
                strerror(errno)));
      }
      got_release = 1;
   }
#  endif
#  if defined(__sgi) && !defined(IRIX5)
   for_each(pid, lGetList(osjob, JO_pid_list)) {
#     ifdef PTF_NICE_BASED
#        if defined(IRIX6)

      /* 
       * IRIX 6.2 has several scheduler bugs which requires this complicated
       * /workaround.  First, a nice value of 20 does not produce a
       * "background" priority as documented.  Second, if you raise a low 
       * non-degrading priority to a high priority, the process will still
       * not move to the run queue.  We used the deadline schedule to
       * force the process to run and therefore to move it up the run queue 
       */
#        ifndef IRIX_NODEADLINE
      struct sched_deadline deadline;

      /* 
       * If IRIX 6.[23], turn off deadline scheduler 
       */
      if (strcmp(irix_release, "6.2") == 0 || 
          strcmp(irix_release, "6.3") == 0) {
         if (lGetUlong(pid, JP_background) == 2) {

            SET_TIMESTRUC_FROM_MS(deadline.dl_period, 0);
            SET_TIMESTRUC_FROM_MS(deadline.dl_alloc, 0);

            if (schedctl(DEADLINE, lGetUlong(pid, JP_pid), &deadline) < 0
                && errno != ESRCH) {
               ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                      u32c(lGetUlong(job, JL_job_ID)),
                      u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
            }

            lSetUlong(pid, JP_background, 0);
         }
      }
#        endif /* IRIX_NODEADLINE */

      if (bg_flag && lGetUlong(job, JL_interactive) == 0 &&
          lGetDouble(job, JL_adjusted_current_proportion) <
          (PTF_BACKGROUND_JOB_PROPORTION / (double) nprocs)) {

         /* set a low non-degrading priority */
         if (schedctl(NDPRI, lGetUlong(pid, JP_pid),
                      PTF_BACKGROUND_JOB_PRIORITY) < 0 && errno != ESRCH) {
            ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                   u32c(lGetUlong(job, JL_job_ID)),
                   u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
         }

         lSetUlong(pid, JP_background, 1);
      } else {

         /* 
          * if the process was previously "backgrounded", then
          * bring it back to the foreground priority 
          */
         if (lGetUlong(pid, JP_background) == 1) {

            /* 
             * turn non-degrading priority off 
             */
            if (schedctl(NDPRI, lGetUlong(pid, JP_pid), 0) < 0 &&
                errno != ESRCH) {
               ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                      u32c(lGetUlong(job, JL_job_ID)),
                      u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
            }
#      ifndef IRIX_NODEADLINE

            /* 
             * use workaround for IRIX 6.[23] 
             */
            if (strcmp(irix_release, "6.2") == 0 ||
                strcmp(irix_release, "6.3") == 0) {

               /* 
                * force process to run once during the next interval
                * using deadline scheduler 
                */
               SET_TIMESTRUC_FROM_MS(deadline.dl_period,
                                     MAX(PTF_SCHEDULE_TIME / 2, 1) * 1000);
               SET_TIMESTRUC_FROM_MS(deadline.dl_alloc, 1);

               if (schedctl(DEADLINE, lGetUlong(pid, JP_pid), &deadline) < 0
                   && errno != ESRCH) {
                  ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                         u32c(lGetUlong(job, JL_job_ID)),
                         u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
               }
               lSetUlong(pid, JP_background, 2);
            }
#      endif /* IRIX_NODEADLINE */

         }

         /* set nice value */
         if (schedctl(RENICE, lGetUlong(pid, JP_pid), pri) < 0 && 
             errno != ESRCH) {
            ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                   u32c(lGetUlong(job, JL_job_ID)),
                   u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
         }
      }

#       else
      /* 
       * IRIX 6.4 also has some scheduler bugs.  Namely, when you use
       * setpriority and assign a "weightless" priority of 20, setting
       * a new priority doesn't bring the process out of the "weightless"
       * class.  The only way to bring the process out is to force a
       * reset of its scheduling class using sched_setscheduler. 
       */
      if (bg_flag && lGetUlong(job, JL_interactive) == 0 &&
          lGetDouble(job, JL_adjusted_current_proportion) <
          (PTF_BACKGROUND_JOB_PROPORTION / (double) nprocs)) {

         /* set the background "weightless" priority */

         if (setpriority(PRIO_PROCESS, lGetUlong(pid, JP_pid),
                         PTF_BACKGROUND_NICE_VALUE) < 0 && errno != ESRCH) {

            ERROR((SGE_EVENT, MSG_PRIO_JOBXPIDYSETPRIORITYFAILURE_UUS,
                   u32c(lGetUlong(job, JL_job_ID)),
                   u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
         }

         lSetUlong(pid, JP_background, 1);

      } else {

         struct sched_param sp;

         sp.sched_priority = pri;

         if (sched_setscheduler(lGetUlong(pid, JP_pid), SCHED_TS, &sp) < 0 
             && errno != ESRCH) {
            ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDSETSCHEDULERFAILURE_UUS,
                   u32c(lGetUlong(job, JL_job_ID)),
                   u32c(lGetUlong(pid, JP_pid)), strerror(errno)));
         }

         lSetUlong(pid, JP_background, 0);
      }

#       endif /* IRIX6 */
#     endif
#     ifdef PTF_NDPRI_BASED
      if (schedctl(NDPRI, lGetUlong(pid, JP_pid), pri) < 0 && errno != ESRCH) {
         ERROR((SGE_EVENT, MSG_SCHEDD_JOBXPIDYSCHEDCTLFAILUREX_UUS,
                u32c(lGetUlong(job, JL_job_ID)), u32c(lGetUlong(pid, JP_pid)),
                strerror(errno)));
      }
#     endif
   }
#  endif
   DEXIT;
}

#elif defined(CRAY) || defined(NECSX4) || defined(NECSX5)

/****** ptf/ptf_setpriority_jobid() ******************************************
*  NAME
*     ptf_setpriority_ash() -- Change priority of processes 
*
*  SYNOPSIS
*     static void ptf_setpriority_jobid(lListElem *job, lListElem *osjob, 
*                                       long *pri) 
*
*  FUNCTION
*     This function is only available for the architecture CRAY and NECSX 4/5.
*     All processes belonging to "job" and "osjob" will get a new priority.
*
*  INPUTS
*     lListElem *job   - job  
*     lListElem *osjob - one of the os jobs of "job" 
*     long pri         - new priority 
******************************************************************************/
static void ptf_setpriority_jobid(lListElem *job, lListElem *osjob, long pri)
{
#  ifdef CRAY
   int nice;
#  endif
   DENTER(TOP_LAYER, "ptf_setpriority_jobid");

#  ifdef CRAY
   nice = nicem(C_JOB, ptf_get_osjobid(osjob), 0);
   if (nice >= 0) {
      /* for interactive jobs, don't set "background" priority */
      if (lGetUlong(job, JL_interactive) && pri == PTF_MIN_PRIORITY) {
         pri--;
      }
      if (nicem(C_JOB, ptf_get_osjobid(osjob), pri - nice) < 0) {
         ERROR((SGE_EVENT, MSG_PRIO_JOBXNICEMFAILURE_S,
                u32c(lGetUlong(job, JL_job_ID)), strerror(errno)));

      }
   } else {
      ERROR((SGE_EVENT, MSG_PRIO_JOBXNICEMFAILURE_S,
             u32c(lGetUlong(job, JL_job_ID)), strerror(errno)));
   }
#  endif

#  if defined(NECSX4 || NECSX5)
   if (nicej(ptf_get_osjobid(osjob), pri) == -1) {
      if (errno != ESRCH) {
         ERROR((SGE_EVENT, "job " u32 " setpriority failure: %s\n",
                lGetUlong(job, JL_job_ID), strerror(errno)));
      }
   } else {
      DPRINTF(("NICEJ(" u32 ", " u32 ")\n",
               (u_long32) ptf_get_osjobid(osjob), (u_long32) pri));
   }
#  endif
   DEXIT;
}

#elif defined(ALPHA) || defined(SOLARIS) || defined(LINUX)

/****** ptf/ptf_setpriority_jobid() ******************************************
*  NAME
*     ptf_setpriority_addgrpid() -- Change priority of processes
*
*  SYNOPSIS
*     static void ptf_setpriority_jobid(lListElem *job, lListElem *osjob,
*                                       long *pri)
*
*  FUNCTION
*     This function is only available for the architecture SOLARIS, ALPHA and
*     LINUX. All processes belonging to "job" and "osjob" will get a new i
*     priority.
*
*  INPUTS
*     lListElem *job   - job
*     lListElem *osjob - one of the os jobs of "job"
*     long pri         - new priority
******************************************************************************/
static void ptf_setpriority_addgrpid(lListElem *job, lListElem *osjob, 
                                     long pri)
{
   lListElem *pid;

   DENTER(TOP_LAYER, "ptf_setpriority_addgrpid");

#  ifdef USE_ALPHA_PGRPS

   /*
    * set the priority for the entire process group
    */
   if (setpriority(PRIO_PGRP, ptf_get_osjobid(osjob), pri) < 0 &&
       errno != ESRCH) {
      ERROR((SGE_EVENT, MSG_PRIO_JOBXSETPRIORITYFAILURE_DS,
             u32c(lGetUlong(job, JL_job_ID)), strerror(errno)));
   }
   DEXIT;
#  else

   /*
    * set the priority for each active process
    */
   for_each(pid, lGetList(osjob, JO_pid_list)) {
      if (setpriority(PRIO_PROCESS, lGetUlong(pid, JP_pid), pri) < 0 &&
          errno != ESRCH) {
         ERROR((SGE_EVENT, MSG_PRIO_JOBXPIDYSETPRIORITYFAILURE_UUS,
                u32c(lGetUlong(job, JL_job_ID)), u32c(lGetUlong(pid, JP_pid)),
                strerror(errno)));
      } else {
         DPRINTF(("Changing Priority of process " u32 " to " u32 "\n",
                  u32c(lGetUlong(pid, JP_pid)), u32c((u_long32) pri)));
      }
   }
   DEXIT;
#  endif
}

#endif

/****** ptf/ptf_get_job() *****************************************************
*  NAME
*     ptf_get_job() -- look up the job for the supplied job_id and return it 
*
*  SYNOPSIS
*     static lListElem* ptf_get_job(u_long job_id) 
*
*  FUNCTION
*     look up the job for the supplied job_id and return it 
*
*  INPUTS
*     u_long job_id - SGE job id 
*
*  RESULT
*     static lListElem* - JL_Type
******************************************************************************/
static lListElem *ptf_get_job(u_long job_id)
{
   lListElem *job;
   lCondition *where;

   where = lWhere("%T(%I == %u)", JL_Type, JL_job_ID, job_id);
   job = lFindFirst(ptf_jobs, where);
   lFreeWhere(where);
   return job;
}

/****** ptf/ptf_get_job_os() ***************************************************
*  NAME
*     ptf_get_job_os() -- look up the job for the supplied OS job_id 
*
*  SYNOPSIS
*     static lListElem* ptf_get_job_os(lList *job_list, 
*                                      osjobid_t os_job_id, 
*                                      lListElem **job_elem) 
*
*  FUNCTION
*     This functions tries to find a os job element (JO_Type) with
*     "os_job_id" within the list of os jobs of "job_elem". If "job_elem"
*     is not provided the function will look up the whole "job_list".
*
*  INPUTS
*     lList *job_list      - List of all known jobs (JL_Type) 
*     osjobid_t os_job_id  - os job id (ash, job id, supplementary gid) 
*     lListElem **job_elem - Pointer to a job element pointer (JL_Type)
*                          - pointer to a NULL pointer
*                            => *job_elem will contain the corresponding
*                               job element pointer when the function 
*                               returns successfully
*                          - NULL 
*
*  RESULT
*     static lListElem* - osjob (JO_Type) 
*                         or NULL if it was not found.
******************************************************************************/
static lListElem *ptf_get_job_os(lList *job_list, osjobid_t os_job_id, 
                                 lListElem **job_elem)
{
   lListElem *job, *osjob = NULL;
   lCondition *where;

   DENTER(TOP_LAYER, "ptf_get_job_os");

#if defined(LINUX) || defined(SOLARIS) || defined(ALPHA5) || defined(NECSX4) || defined(NECSX5)
   where = lWhere("%T(%I == %u)", JO_Type, JO_OS_job_ID, (u_long32) os_job_id);
#else
   where = lWhere("%T(%I == %u && %I == %u)", JO_Type,
                  JO_OS_job_ID, (u_long) (os_job_id & 0xffffffff),
                  JO_OS_job_ID2, (u_long) (((u_osjobid_t) os_job_id) >> 32));
#endif

   if (!where) {
      CRITICAL((SGE_EVENT, MSG_WHERE_FAILEDTOBUILDWHERECONDITION));
      return NULL;
   }

   if (job_elem && (*job_elem)) {
      osjob = lFindFirst(lGetList(*job_elem, JL_OS_job_list), where);
   } else {
      for_each(job, job_list) {
         osjob = lFindFirst(lGetList(job, JL_OS_job_list), where);
         if (osjob) {
            break;
         }
      }
      if (job_elem) {
         *job_elem = osjob ? job : NULL;
      }
   }

   lFreeWhere(where);
   DEXIT;
   return osjob;
}


/*--------------------------------------------------------------------
 * ptf_process_job - process a job received from the SGE scheduler.
 * This assumes that new jobs can be received after a job ticket
 * list has been sent.  The new jobs will have an associated number
 * of tickets which will be updated in the job_list maintained by
 * the PTF.
 *--------------------------------------------------------------------*/

static lListElem *ptf_process_job(osjobid_t os_job_id, char *task_id_str,
                                  lListElem *new_job, u_long32 jataskid)
{
   lListElem *job, *osjob;
   lList *job_list = ptf_jobs;
   u_long job_id = lGetUlong(new_job, JB_job_number);
   u_long job_tickets =
      lGetUlong(lFirst(lGetList(new_job, JB_ja_tasks)), JAT_ticket);
   u_long interactive = (lGetString(new_job, JB_script_file) == NULL);

   DENTER(TOP_LAYER, "ptf_process_job");

   /*
    * Add the job to the job list, if it does not already exist
    */

/*
 * cases:
 *
 * job == NULL && osjobid == 0
 *    return
 *    job == NULL && osjobid > 0
 *    add osjob job && osjobid > 0 search by osjobid if osjob
 *    found skip
 *    else
 *    add osjob job && osjobid == 0 skip
 */
   job = ptf_get_job(job_id);
   if (os_job_id == 0) {
      if (job == NULL) {
         DEXIT;
         return NULL;
      }
   } else {
      lList *osjoblist;

      if (!job) {
         job = lCreateElem(JL_Type);
         lAppendElem(job_list, job);
         lSetUlong(job, JL_job_ID, job_id);
      }
      osjoblist = lGetList(job, JL_OS_job_list);
      osjob = ptf_get_job_os(osjoblist, os_job_id, &job);
      if (!osjob) {
         if (!osjoblist) {
            osjoblist = lCreateList("osjoblist", JO_Type);
            lSetList(job, JL_OS_job_list, osjoblist);
         }
         osjob = lCreateElem(JO_Type);
         lSetUlong(osjob, JO_ja_task_ID, jataskid);
         lAppendElem(osjoblist, osjob);
         lSetList(osjob, JO_usage_list,
                  ptf_build_usage_list("usagelist", NULL));
         ptf_set_osjobid(osjob, os_job_id);
      }
      if (task_id_str) {
         lSetString(osjob, JO_task_id_str, task_id_str);
      }
   }

   /*
    * set number of tickets in job entry
    */
   lSetUlong(job, JL_tickets, MAX(job_tickets, 1));

   /*
    * set interactive job flag
    */
   if (interactive) {
      lSetUlong(job, JL_interactive, 1);
   }

   DEXIT;
   return job;
}

/****** ptf/ptf_get_usage_from_data_collector() *******************************
*  NAME
*     ptf_get_usage_from_data_collector() -- get usage from PDC 
*
*  SYNOPSIS
*     static void ptf_get_usage_from_data_collector(void) 
*
*  FUNCTION
*     get the usage for all the jobs in the job ticket list and update 
*     the job list 
*
*     call data collector routine with a list of all the jobs in the job_list
*     for each job in the job_list
*        update job.usage with data collector info
*        update list of process IDs associated with job end     
*     do
******************************************************************************/
static void ptf_get_usage_from_data_collector(void)
{
#ifdef USE_DC

   lListElem *job, *osjob;
   lList *pidlist, *oldpidlist;
   struct psJob_s *jobs, *ojobs, *tmp_jobs;
   struct psProc_s *procs;
   uint64 jobcount;
   int proccount;
   char *tid;
   int i, j;

   DENTER(TOP_LAYER, "ptf_get_usage_from_data_collector");

   ojobs = jobs = psGetAllJobs();
   if (jobs) {
      jobcount = *(uint64 *) jobs;

#ifndef SOLARIS
      INCJOBPTR(jobs, sizeof(uint64));
#else
      INCJOBPTR(jobs, 8);
#endif

      for (i = 0; i < jobcount; i++) {
         lList *usage_list;
         lListElem *usage;
         double cpu_usage_value;

         /* look up job in job list */

         job = NULL;
         osjob = ptf_get_job_os(ptf_jobs, jobs->jd_jid, &job);

         if (osjob) {
            u_long job_state = lGetUlong(osjob, JO_state);

            tmp_jobs = jobs;

            /* fill in job completion state */
            lSetUlong(osjob, JO_state, jobs->jd_refcnt ?
                      (job_state & ~JL_JOB_COMPLETE) : (job_state |
                                                        JL_JOB_COMPLETE));

            /* fill in usage for job */
            usage_list = lGetList(osjob, JO_usage_list);
            if (!usage_list) {
               usage_list = ptf_build_usage_list("usagelist", NULL);
               lSetList(osjob, JO_usage_list, usage_list);
            }

            /* set CPU usage */
            cpu_usage_value = jobs->jd_utime_c + jobs->jd_utime_a +
               jobs->jd_stime_c + jobs->jd_stime_a;
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_CPU))) {
               lSetDouble(usage, UA_value, 
                          MAX(cpu_usage_value, lGetDouble(usage, UA_value)));
            }

            /* set mem usage (in GB seconds) */
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_MEM))) {
               lSetDouble(usage, UA_value, (double) jobs->jd_mem / 1048576.0);
            }

            /* set I/O usage (in GB) */
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_IO))) {
               lSetDouble(usage, UA_value,
                          (double) jobs->jd_chars / 1073741824.0);
            }

            /* set I/O wait time */
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_IOW))) {
               lSetDouble(usage, UA_value,
                          (double) jobs->jd_bwtime_c + jobs->jd_bwtime_a +
                          jobs->jd_rwtime_c + jobs->jd_rwtime_a);
            }

            /* set vmem usage */
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_VMEM))) {
               lSetDouble(usage, UA_value, jobs->jd_vmem);
            }

            /* set himem usage */
            if ((usage = lGetElemStr(usage_list, UA_name, USAGE_ATTR_HIMEM))) {
               lSetDouble(usage, UA_value, jobs->jd_himem);
            }

            /* build new pid list */
            proccount = jobs->jd_proccount;
            INCJOBPTR(jobs, jobs->jd_length);

            if (proccount > 0) {
               oldpidlist = lGetList(osjob, JO_pid_list);
               pidlist = lCreateList("pidlist", JP_Type);

               procs = (struct psProc_s *) jobs;
               for (j = 0; j < proccount; j++) {
                  lListElem *pid;

                  if (procs->pd_state == 1) {
                     if ((pid = lGetElemUlong(oldpidlist, JP_pid,
                                              procs->pd_pid))) {
                        lAppendElem(pidlist, lCopyElem(pid));
                     } else {
                        pid = lCreateElem(JP_Type);

                        lSetUlong(pid, JP_pid, procs->pd_pid);
                        lAppendElem(pidlist, pid);
                     }
                  }
                  INCPROCPTR(procs, procs->pd_length);
               }

               jobs = (struct psJob_s *) procs;
               lSetList(osjob, JO_pid_list, pidlist);
            } else {
               lSetList(osjob, JO_pid_list, NULL);
            }

#if 1
            tid = lGetString(osjob, JO_task_id_str);
            DPRINTF(("JOB " u32 "." u32 ": %s: (cpu = %8.3lf / mem = "
                     UINT64_FMT " / io = " UINT64_FMT " / vmem = "
                     UINT64_FMT " / himem = " UINT64_FMT ")\n",
                     lGetUlong(job, JL_job_ID), 
                     lGetUlong(osjob, JO_ja_task_ID), tid ? tid : "",
                     tmp_jobs->jd_utime_c + tmp_jobs->jd_utime_a +
                     tmp_jobs->jd_stime_c + tmp_jobs->jd_stime_a,
                     tmp_jobs->jd_mem, tmp_jobs->jd_chars,
                     tmp_jobs->jd_vmem, tmp_jobs->jd_himem));
#endif
         } else {
            /* 
             * NOTE: Under what conditions would DC have a job
             * that the PTF doesn't know about? 
             */
            psIgnoreJob(jobs->jd_jid);

            proccount = jobs->jd_proccount;
            INCJOBPTR(jobs, jobs->jd_length);
            procs = (struct psProc_s *) jobs;
            for (j = 0; j < proccount; j++) {
               INCPROCPTR(procs, procs->pd_length);
            }
            jobs = (struct psJob_s *) procs;
         }
      }
      free(ojobs);

      for_each(job, ptf_jobs) {
         double usage_value, old_usage_value;
         double cpu_usage_value = 0;
         int active_jobs = 0;

         for_each(osjob, lGetList(job, JL_OS_job_list)) {
            lListElem *usage;

            if ((usage = lGetElemStr(lGetList(osjob, JO_usage_list), 
                                     UA_name, USAGE_ATTR_CPU))) {
               cpu_usage_value += lGetDouble(usage, UA_value);
            }
            if (!(lGetUlong(osjob, JO_state) & JL_JOB_COMPLETE)) {
               active_jobs = 1;
            }
         }

         /* calculate JL_usage */
         usage_value = cpu_usage_value;
         old_usage_value = lGetDouble(job, JL_old_usage_value);
         lSetDouble(job, JL_old_usage_value, usage_value);
         lSetDouble(job, JL_usage, 
                    MAX(PTF_MIN_JOB_USAGE, lGetDouble(job, JL_usage) *
                    PTF_USAGE_DECAY_FACTOR + (usage_value - old_usage_value)));
         lSetDouble(job, JL_last_usage, usage_value - old_usage_value);

         /* set job state */
         if (!active_jobs) {
            lSetUlong(job, JL_state, lGetUlong(job, JL_state) 
                      & JL_JOB_COMPLETE);
         }
      }
   }
   DEXIT;

#else

# ifdef MODULE_TEST

   lListElem *job, *proc, *usage;
   lList *usage_list;
   lList *pid_list;
   int j;

   DENTER(TOP_LAYER, "ptf_get_usage_from_data_collector");

   j = 0;
   for_each(job, ptf_jobs) {

      lListElem *osjob = lFirst(lGetList(job, JL_OS_job_list));

      usage_list = lGetList(osjob, JO_usage_list);
      if (!usage_list) {
         usage_list = ptf_build_usage_list("usagelist", NULL);
         lSetList(osjob, JO_usage_list, usage_list);
      }

      /* add to usage */

      for_each(usage, usage_list) {
         double value;

         value = lGetDouble(usage, UA_value);
         value += drand48();
         lSetDouble(usage, UA_value, value);
      }

      /* calculate JL_usage */
      {
         prstatus_t pinfo;
         int procfd;

         procfd = lGetUlong(job, JL_procfd);
         if (procfd > 0) {

            double usage_value, old_usage_value;

            if (ioctl(procfd, PIOCSTATUS, &pinfo) < 0) {
               perror("ioctl on /proc");
            }

            usage_value =
               pinfo.pr_utime.tv_sec +
               pinfo.pr_utime.tv_nsec / 1000000000.0L +
               pinfo.pr_stime.tv_sec +
               pinfo.pr_stime.tv_nsec / 1000000000.0L +
               pinfo.pr_cutime.tv_sec +
               pinfo.pr_cutime.tv_nsec / 1000000000.0L +
               pinfo.pr_cstime.tv_sec +
               pinfo.pr_cstime.tv_nsec / 1000000000.0L;

            old_usage_value = lGetDouble(job, JL_old_usage_value);

            lSetDouble(job, JL_old_usage_value, usage_value);

            lSetDouble(job, JL_usage,
                       lGetDouble(job, JL_usage) *
                       PTF_USAGE_DECAY_FACTOR +
                       (usage_value - old_usage_value));

            lSetDouble(job, JL_last_usage, usage_value - old_usage_value);
         }

      }

      /* build a fake pid list with the OS job ID as the pid */

      pid_list = lGetList(osjob, JO_pid_list);
      if (!pid_list) {
         pid_list = lCreateList("pidlist", JP_Type);
         lSetList(osjob, JO_pid_list, pid_list);
         proc = lCreateElem(JP_Type);
         lSetUlong(proc, JP_pid, lGetUlong(osjob, JO_OS_job_ID));
         lAppendElem(pid_list, proc);
      }

      j++;
   }

# endif /* MODULE_TEST */

#endif /* USE_DC */

}

/*--------------------------------------------------------------------
 * ptf_calc_job_proporiton_pass0
 *--------------------------------------------------------------------*/

static void ptf_calc_job_proportion_pass0(lListElem *job,
                                          u_long *sum_of_job_tickets,
                                          double *sum_of_last_usage)
{
   *sum_of_job_tickets += lGetUlong(job, JL_tickets);
   *sum_of_last_usage += lGetDouble(job, JL_last_usage);
}


/*--------------------------------------------------------------------
 * ptf_calc_job_proportion_pass1
 *--------------------------------------------------------------------*/

static void ptf_calc_job_proportion_pass1(lListElem *job,
                                          u_long sum_of_job_tickets,
                                          double sum_of_last_usage,
                                          double *sum_proportion)
{
   double share, job_proportion;
   double u;

#ifdef PTF_DYNAMIC_USAGE_ADJUSTMENT

   {
      double K6 = .9;
      double targetted = lGetDouble(job, JL_adjusted_current_proportion);
      double last_usage = lGetDouble(job, JL_last_usage);
      double actual = sum_of_last_usage ? (last_usage / sum_of_last_usage) : 0;

      if (actual < K6 * targetted) {
         lSetDouble(job, JL_usage, lGetDouble(job, JL_usage) - last_usage +
                    lGetDouble(job, JL_last_usage) * 
                    (actual / (K6 * targetted)));
      }
   }

#endif

   share = ((double) lGetUlong(job, JL_tickets) / sum_of_job_tickets) *
      1000.0 + 1.0;
   lSetDouble(job, JL_share, share);

   /*
    * Calculate each jobs proportion value based on the job's tickets
    * and recent usage:
    * job.proportion = (job.tickets/sum_of_job_tickets)^2 / job.usage
    */
   u = MAX(lGetDouble(job, JL_usage), PTF_MIN_JOB_USAGE);
   job_proportion = share * share / u;
   *sum_proportion += job_proportion;
   lSetDouble(job, JL_proportion, job_proportion);

}

/*--------------------------------------------------------------------
 * ptf_calc_job_proportion_pass2
 *--------------------------------------------------------------------*/

static void ptf_calc_job_proportion_pass2(lListElem *job,
                                          u_long sum_of_job_tickets,
                                          double sum_proportion, double
                                          *sum_adjusted_proportion, double
                                          *sum_last_interval_usage)
{
   double job_targetted_proportion, job_current_proportion =
      0, job_adjusted_usage, job_adjusted_proportion, share;

   job_targetted_proportion = (double) lGetUlong(job, JL_tickets) /
      sum_of_job_tickets;
   if (sum_proportion > 0)
      job_current_proportion = lGetDouble(job, JL_proportion) / sum_proportion;

   if ((ptf_compensation_factor > 0 && job_targetted_proportion > 0) 
       && (job_current_proportion > 
                        (ptf_compensation_factor *job_targetted_proportion))) {
      job_adjusted_usage = lGetDouble(job, JL_usage) * 
          (job_current_proportion /
          (ptf_compensation_factor * job_targetted_proportion));
   } else {
      job_adjusted_usage = lGetDouble(job, JL_usage);
   }
   lSetDouble(job, JL_adjusted_usage, job_adjusted_usage);

   /*
    * Recalculate proportions based on adjusted job usage
    */
   share = lGetDouble(job, JL_share);
   job_adjusted_usage = MAX(job_adjusted_usage, PTF_MIN_JOB_USAGE);
   job_adjusted_proportion = share * share / job_adjusted_usage;
   *sum_adjusted_proportion += job_adjusted_proportion;
   lSetDouble(job, JL_adjusted_proportion, job_adjusted_proportion);
}

/*--------------------------------------------------------------------
 * ptf_calc_job_proportion_pass3
 *--------------------------------------------------------------------*/

static void ptf_calc_job_proportion_pass3(lListElem *job,
                                          double sum_adjusted_proportion,
                                          double sum_last_interval_usage,
                                          double *min_share, double *max_share)
{
   double job_adjusted_current_proportion = 0;

   if (sum_adjusted_proportion > 0)
      job_adjusted_current_proportion =
         lGetDouble(job, JL_adjusted_proportion) / sum_adjusted_proportion;

   lSetDouble(job, JL_last_proportion,
              lGetDouble(job, JL_adjusted_current_proportion));

   lSetDouble(job, JL_adjusted_current_proportion,
              job_adjusted_current_proportion);

   *max_share = MAX(*max_share, job_adjusted_current_proportion);
   *min_share = MIN(*min_share, job_adjusted_current_proportion);

#if 0
    DPRINTF(("XXXXXXX  minshare: %f, max_share: %f XXXXXXX\n", *min_share, 
             *max_share)); 
#endif
}

/*--------------------------------------------------------------------
 * ptf_set_OS_scheduling_parameters
 *--------------------------------------------------------------------*/

static void ptf_set_OS_scheduling_parameters(lList *job_list, double min_share,
                                             double max_share)
{
   lListElem *job;
   static long pri_range, pri_min = 1000, pri_max = -1000;
   long pri_min_tmp, pri_max_tmp;
   long pri;
   char *env;
   int val;

   DENTER(TOP_LAYER, "ptf_set_OS_scheduling_parameters");


   pri_min_tmp = PTF_MIN_PRIORITY;
   if ((env = getenv("PTF_MIN_PRIORITY")))
      if (sscanf(env, "%d", &val) == 1)
         pri_min_tmp = val;
   if (ptf_min_priority != -999)
      pri_min_tmp = ptf_min_priority;

   pri_max_tmp = PTF_MAX_PRIORITY;
   if ((env = getenv("PTF_MAX_PRIORITY")))
      if (sscanf(env, "%d", &val) == 1)
         pri_max_tmp = val;
   if (ptf_max_priority != -999)
      pri_max_tmp = ptf_max_priority;

   if (pri_max != pri_max_tmp || pri_min != pri_min_tmp) {
      pri_max = pri_max_tmp;
      pri_min = pri_min_tmp;
      pri_range = pri_min - pri_max;

      INFO((SGE_EVENT, MSG_PRIO_PTFMINMAX_II, (int) pri_max, (int) pri_min));
   }

   /*
    * Set the priority for each job
    */
   for_each(job, job_list) {

#if 0
      /*
       * Note: Should the priorities be distributed across the entire
       * priority range or simply a limited portion of the priority
       * range?  Currently, the algorithm below distributes the
       * job priorities across the entire priority range.
       */
      pri = pri_max + pri_range *
         (max_share - lGetDouble(job, JL_adjusted_current_proportion)) /
         (max_share - min_share);
#endif

      pri = pri_max + (pri_range * (1.0 -
                        lGetDouble(job, JL_adjusted_current_proportion)));

#ifdef PTF_NEW_ALGORITHM

      /*
       * Note: Should calculate targetted proportion and if it is below
       * a certain % then set a background priority.  This is because
       * nice seems to always give at least some minimal % to all
       * processes.
       */

#ifdef notdef
      pri = pri_max + (pri_range * (lGetDouble(job, JL_curr_pri) / max_share));
#endif

      if (max_share > 0) {
         pri = pri_max + (pri_range * ((lGetDouble(job, JL_curr_pri) 
                                       - min_share) / max_share));
      } else {
         pri = pri_min;
      }

#endif

      lSetLong(job, JL_pri, pri);
      ptf_set_job_priority(job);
   }

   DEXIT;
}


/*--------------------------------------------------------------------
 * ptf_job_started - process new job
 *--------------------------------------------------------------------*/
int ptf_job_started(osjobid_t os_job_id, char *task_id_str, lListElem *new_job,
                    u_long32 jataskid)
{
   lListElem *job;

   DENTER(TOP_LAYER, "ptf_job_started");

   /*
    * Add new job to job list
    */
   job = ptf_process_job(os_job_id, task_id_str, new_job, jataskid);

   /*
    * Tell data collector to start collecting data for this job
    */
#ifdef USE_DC
   if (os_job_id > 0)
      psWatchJob(os_job_id);
#else

# ifdef MODULE_TEST
   if (os_job_id > 0) {
      int fd;
      char fname[32];

      sprintf(fname, "/proc/%05d", os_job_id);

      fd = open(fname, O_RDONLY);
      if (fd < 0)
         fprintf(stderr, "opening of %s failed\n", fname);
      else
         lSetUlong(job, JL_procfd, fd);
   }
# endif

#endif

   DEXIT;
   return 0;
}

/*--------------------------------------------------------------------
 * ptf_job_complete - process completed job
 *--------------------------------------------------------------------*/

int ptf_job_complete(lListElem *completed_job)
{
   lList *job_list = ptf_jobs;
   lListElem *job, *osjob, *ja_task;
   u_long job_id = lGetUlong(completed_job, JB_job_number);
   char *tid;
   int job_complete;
   lListElem *completed_task = NULL;
   u_long32 ja_task_id = 0;

   DENTER(TOP_LAYER, "ptf_job_complete");

   job = ptf_get_job(job_id);

   if (job == NULL) {
      return PTF_ERROR_JOB_NOT_FOUND;
   }

   /*
    * if job is not complete, go get latest job usage info
    */
   if (!(lGetUlong(job, JL_state) & JL_JOB_COMPLETE)) {
      ptf_get_usage_from_data_collector();
   }

   /*
    * Get usage for completed job
    *    (If this for a sub-task then we return the sub-task usage)
    */
   tid = lGetString(completed_job, JB_pe_task_id_str);
   for_each(ja_task, lGetList(completed_job, JB_ja_tasks)) {
      ja_task_id = lGetUlong(ja_task, JAT_task_number);

      completed_task = search_task(ja_task_id, completed_job);
      lSetList(completed_task, JAT_usage_list,
               _ptf_get_job_usage(job, ja_task_id, tid));
   }

   /*
    * Mark osjob as complete and see if all tracked osjobs are done
    * Tell data collector to stop collecting data for this job
    */
   job_complete = 1;
   for_each(osjob, lGetList(job, JL_OS_job_list)) {
      char *osjob_tid = lGetString(osjob, JO_task_id_str);
      u_long jstate = lGetUlong(osjob, JO_state);

      if ((!tid && !osjob_tid)
          || (tid && osjob_tid && !strcmp(tid, osjob_tid))) {
         lSetUlong(osjob, JO_state, jstate | JL_JOB_DELETED);
#ifdef USE_DC
         psIgnoreJob(ptf_get_osjobid(osjob));
#endif
      } else if (!(jstate & JL_JOB_DELETED)) {
         job_complete = 0;
      }
   }

#ifndef USE_DC
# ifdef MODULE_TEST

   {
      int procfd = lGetUlong(job, JL_procfd);

      if (procfd > 0)
         close(procfd);
   }
# endif
#endif

   /*
    * Remove job/task from job/task list
    */
   if (job_complete && completed_task) {
      lList *os_jatasks;
      lListElem *os_jatask, *nxt_os_jatask;

      DPRINTF(("PTF: Removing job " u32 "." u32 "\n", job_id, ja_task_id));
      os_jatasks = lGetList(job, JL_OS_job_list);
      nxt_os_jatask = lFirst(os_jatasks);
      while ((os_jatask = nxt_os_jatask)) {
         nxt_os_jatask = lNext(nxt_os_jatask);
         if (ja_task_id == lGetUlong(os_jatask, JO_ja_task_ID)) {
            lRemoveElem(os_jatasks, os_jatask);
         }
      }
      if (lGetNumberOfElem(os_jatasks) == 0) {
         DPRINTF(("PTF: Removing job\n"));
         lDechainElem(job_list, job);
         lFreeElem(job);
      }
   }

   DEXIT;
   return 0;
}


/*--------------------------------------------------------------------
 * ptf_process_job_ticket_list - Process the job ticket list sent
 * from the SGE scheduler.
 *--------------------------------------------------------------------*/

int ptf_process_job_ticket_list(lList *job_ticket_list)
{
   lListElem *jte, *job;

   DENTER(TOP_LAYER, "ptf_process_job_ticket_list");

    /*
     * Update the job entries in the job list with the number of
     * tickets from the job ticket list.  Reset the usage to the
     * minimum usage value.
     */
   for_each(jte, job_ticket_list) {

      /* 
       * set JB_script_file because we don't know if this is
       * an interactive job 
       */
      lSetString(jte, JB_script_file, strdup("dummy"));

      job = ptf_process_job(0, NULL, jte,
                            lGetUlong(lFirst(lGetList(jte, JB_ja_tasks)),
                            JAT_task_number));
      if (job) {
         /* reset temporary usage and priority */
         lSetDouble(job, JL_usage, MAX(PTF_MIN_JOB_USAGE,
                                       lGetDouble(job, JL_usage) * 0.1));

         lSetDouble(job, JL_curr_pri, 0);
      }
   }

   DEXIT;
   return 0;
}

void ptf_update_job_usage()
{
   DENTER(TOP_LAYER, "ptf_update_job_usage");

   ptf_get_usage_from_data_collector();
   DEXIT;
}


/*--------------------------------------------------------------------
 * ptf_adjust_job_priorities - routine to adjust the priorities of
 * executing jobs. Called whenever the PTF interval timer expires.
 *--------------------------------------------------------------------*/

int ptf_adjust_job_priorities(void)
{
   static u_long32 next = 0;
   u_long32 now;
   lList *job_list, *pid_list;
   lListElem *job, *osjob;
   u_long sum_of_job_tickets = 0;
   int num_procs = 0;
   double sum_proportion = 0;
   double sum_adjusted_proportion = 0;
   double min_share = 100.0;
   double max_share = 0;
   double sum_interval_usage = 0;
   double sum_of_last_usage = 0;

   DENTER(TOP_LAYER, "ptf_adjust_job_priorities");

   if ((now = sge_get_gmt()) < next) {
      DEXIT;
      return 0;
   }

   job_list = ptf_jobs;

   /* 
    * just get jobs usage in SGE mode 
    * this is necessary to force job resource limits 
    */
   if (feature_is_enabled(FEATURE_REPRIORISATION)) {

      /*
       * Do pass 0 of calculating job proportional share of resources
       */
      for_each(job, job_list) {
         ptf_calc_job_proportion_pass0(job, &sum_of_job_tickets,
                                       &sum_of_last_usage);
      }
      if (sum_of_job_tickets == 0) {
         sum_of_job_tickets = 1;
      }

      /*
       * Do pass 1 of calculating job proportional share of resources
       */
      for_each(job, job_list) {
         ptf_calc_job_proportion_pass1(job, sum_of_job_tickets,
                                       sum_of_last_usage, &sum_proportion);
      }

      /*
       * Do pass 2 of calculating job proportional share of resources
       */
      for_each(job, job_list) {
         ptf_calc_job_proportion_pass2(job, sum_of_job_tickets,
                                       sum_proportion, 
                                       &sum_adjusted_proportion,
                                       &sum_interval_usage);
      }

      /*
       * Do pass 3 of calculating job proportional share of resources
       */
      for_each(job, job_list) {
         ptf_calc_job_proportion_pass3(job, sum_proportion,
                                       sum_interval_usage, &min_share,
                                       &max_share);
      }

#ifdef PTF_NEW_ALGORITHM

      max_share = 0;
      min_share = -1;

      for_each(job, job_list) {
         double shr;

#ifdef PTF_DYNAMIC_PRIORITY_ADJUSTMENT

      {
         /*
          * calculate share based on tickets and recent performance
          * recent performanced is measure by keeping a decayed sum of
          *     (targetted - actual)
          */
         double targetted = lGetDouble(job, JL_last_proportion);
         double actual = sum_of_last_usage ?
             (lGetDouble(job, JL_last_usage) / sum_of_last_usage) : 0;

         lSetDouble(job, JL_diff_proportion,
                    (lGetDouble(job, JL_diff_proportion) *
                     PTF_DIFF_DECAY_CONSTANT) + (targetted - actual));

         shr = MAX(((lGetUlong(job, JL_tickets) /
                     (double) sum_of_job_tickets)) +
                   (lGetDouble(job, JL_diff_proportion) * 1.0),
                   0) * 1000.0 + 1.0;
      }
#else

         /*
          * calculate share based on tickets only
          */
         shr = lGetDouble(job, JL_share);

#endif

         num_procs = 0;
         for_each(osjob, lGetList(job, JL_OS_job_list)) {
            if ((pid_list = lGetList(osjob, JO_pid_list))) {
               num_procs += lGetNumberOfElem(pid_list);
            }
         }
         num_procs = MAX(1, num_procs);

         /* 
          * NOTE: share algo only adjusts priority when a process runs 
          */
         lSetDouble(job, JL_curr_pri, lGetDouble(job, JL_curr_pri) 
                    + ((lGetDouble(job, JL_adjusted_usage) * num_procs) 
                       / (shr * shr)));

         max_share = MAX(max_share, lGetDouble(job, JL_curr_pri));
         if (min_share < 0) {
            min_share = lGetDouble(job, JL_curr_pri);
         } else {
            min_share = MIN(min_share, lGetDouble(job, JL_curr_pri));
         } 
      }

#endif

      /*
       * Set the O.S. scheduling parameters for the jobs
       */
      ptf_set_OS_scheduling_parameters(job_list, min_share, max_share);
   }
   next = now + PTF_SCHEDULE_TIME;

   DEXIT;
   return 0;
}

/*--------------------------------------------------------------------
 * ptf_get_job_usage - routine to return a single usage list for the
 * entire job.
 *--------------------------------------------------------------------*/
lList *ptf_get_job_usage(u_long job_id, u_long ja_task_id, char *task_id)
{
   return _ptf_get_job_usage(ptf_get_job(job_id), ja_task_id, task_id);
}

static lList *_ptf_get_job_usage(lListElem *job, u_long ja_task_id,
                                 char *task_id)
{
   lListElem *osjob, *usrc, *udst;
   lList *job_usage = NULL;
   char *task_id_str;

   if (job == NULL)
      return NULL;

   for_each(osjob, lGetList(job, JL_OS_job_list)) {
      task_id_str = lGetString(osjob, JO_task_id_str);

      if ((((!task_id || !task_id[0]) && !task_id_str) 
           || (task_id && !strcmp(task_id, "*")) 
           || (task_id && task_id_str && !strcmp(task_id, task_id_str)))
          && (lGetUlong(osjob, JO_ja_task_ID) == ja_task_id)) {

         if (job_usage) {
            for_each(usrc, lGetList(osjob, JO_usage_list)) {
               if ((udst = lGetElemStr(job_usage, UA_name,
                                       lGetString(usrc, UA_name)))) {
                  lSetDouble(udst, UA_value,
                             lGetDouble(udst,
                                        UA_value) + lGetDouble(usrc,
                                                               UA_value));
               } else {
                  lAppendElem(job_usage, lCopyElem(usrc));
               }
            }
         } else {
            job_usage = lCopyList(NULL, lGetList(osjob, JO_usage_list));
         }
      }
   }
   return job_usage;
}

/*--------------------------------------------------------------------
 * ptf_get_usage - routine to return the current job usage for
 * all executing jobs.  Returns a job list with the job_ID and usage
 * filled in. A separate job entry is returned for each tracked
 * sub-task.
 *--------------------------------------------------------------------*/

int ptf_get_usage(lList **job_usage_list)
{
   lList *job_list, *temp_usage_list, *new_ja_task_list;
   lListElem *job, *new_job, *osjob, *new_ja_task;
   lEnumeration *what;

   what = lWhat("%T(%I %I %I)", JB_Type, JB_job_number, JB_ja_tasks,
                JB_pe_task_id_str);

   temp_usage_list = lCreateList("PtfJobUsageList", JB_Type);
   job_list = ptf_jobs;
   for_each(job, job_list) {
      lListElem *tmp_job = NULL;
      u_long32 job_id = lGetUlong(job, JL_job_ID);

      for_each(osjob, lGetList(job, JL_OS_job_list)) {
         u_long32 ja_task_id = lGetUlong(osjob, JO_ja_task_ID);

         if (!(lGetUlong(osjob, JO_state) & JL_JOB_DELETED)) {
            int insert = 1;

            for_each(tmp_job, temp_usage_list) {
               if (lGetUlong(tmp_job, JB_job_number) == job_id &&
                   lGetString(tmp_job, JB_pe_task_id_str) &&
                   lGetString(osjob, JO_task_id_str) &&
                   !strcmp(lGetString(tmp_job, JB_pe_task_id_str),
                           lGetString(osjob, JO_task_id_str))) {

                  new_ja_task = lCreateElem(JAT_Type);
                  lSetUlong(new_ja_task, JAT_task_number, ja_task_id);
                  lSetList(new_ja_task, JAT_usage_list,
                           lCopyList(NULL, lGetList(osjob, JO_usage_list)));
                  lAppendElem(lGetList(tmp_job, JB_ja_tasks), new_ja_task);
                  insert = 0;
               }
            }
            if (insert) {
               /* Job not found => Create Job and JobArrayTask */
               new_ja_task_list = lCreateList("", JAT_Type);
               new_job = lCreateElem(JB_Type);
               new_ja_task = lCreateElem(JAT_Type);

               lSetUlong(new_ja_task, JAT_task_number, ja_task_id);
               lSetList(new_ja_task, JAT_usage_list,
                        lCopyList(NULL, lGetList(osjob, JO_usage_list)));
               lAppendElem(new_ja_task_list, new_ja_task);
               lSetUlong(new_job, JB_job_number, lGetUlong(job, JL_job_ID));
               lSetString(new_job, JB_pe_task_id_str,
                          lGetString(osjob, JO_task_id_str));
               lSetList(new_job, JB_ja_tasks, new_ja_task_list);
               lAppendElem(temp_usage_list, new_job);
            }
         }
      }
   }

   *job_usage_list = lSelect("PtfJobUsageList", temp_usage_list, NULL, what);

   lFreeList(temp_usage_list);
   lFreeWhat(what);

   return 0;
}


/*--------------------------------------------------------------------
 * ptf_init - initialize the Priority Translation Facility
 *--------------------------------------------------------------------*/

int ptf_init(void)
{
   DENTER(TOP_LAYER, "ptf_init");
   lInit(nmv);

   ptf_jobs = lCreateList("ptf_job_list", JL_Type);
   if (ptf_jobs == NULL) {
      DEXIT;
      return -1; 
   }

   switch2start_user();
   if (psStartCollector()) {
      switch2admin_user();
      DEXIT;
      return -1;
   }
#if defined(__sgi) && !defined(IRIX5)
   schedctl(RENICE, 0, 0);
#elif defined(CRAY)

   if (getuid() == 0) {
      int nice;

      if ((nice = nicem(C_PGRP, 0, 0)) > 0) {
         if (nicem(C_PGRP, 0, 0 - nice) < 0) {
            ERROR((SGE_EVENT, MSG_PRIO_NICEMFAILED_S, strerror(errno)));
         }
      }
   }
#elif defined(ALPHA) || defined(SOLARIS) || defined(LINUX)
   if (getuid() == 0) {
      if (setpriority(PRIO_PROCESS, getpid(), -20) < 0) {
         ERROR((SGE_EVENT, MSG_PRIO_SETPRIOFAILED_S, strerror(errno)));
      }
   }
#endif
   switch2admin_user();
   DEXIT;
   return 0;
}

void ptf_start(void)
{
   DENTER(TOP_LAYER, "ptf_start");
   if (!is_ptf_running) {
      ptf_init();
      is_ptf_running = 1;
   }
   DEXIT;
}

void ptf_stop(void)
{
   DENTER(TOP_LAYER, "ptf_stop");
   if (is_ptf_running) {
      ptf_unregister_registered_jobs();
      psStopCollector();
      is_ptf_running = 0;
   }
   DEXIT;
}

void ptf_show_registered_jobs(void)
{
   lList *job_list;
   lListElem *job_elem;

   DENTER(TOP_LAYER, "ptf_show_registered_jobs");

   job_list = ptf_jobs;
   for_each(job_elem, job_list) {
      lList *os_job_list;
      lListElem *os_job;

      DPRINTF(("\tjob id: " u32 "\n", lGetUlong(job_elem, JL_job_ID)));
      os_job_list = lGetList(job_elem, JL_OS_job_list);
      for_each(os_job, os_job_list) {
         lList *process_list;
         lListElem *process;
         char *pe_task_id_str;

         pe_task_id_str = lGetString(os_job, JO_task_id_str);
         pe_task_id_str = pe_task_id_str ? pe_task_id_str : "<<NONE>>";

         DPRINTF(("\t\tosjobid: " u32 " petaskid: %s\n",
                  lGetUlong(os_job, JO_OS_job_ID), pe_task_id_str));
         process_list = lGetList(os_job, JO_pid_list);
         for_each(process, process_list) {
            u_long32 pid;

            pid = lGetUlong(process, JP_pid);
            DPRINTF(("\t\t\tpid: " u32 "\n", pid));
         }
      }
   }
   DEXIT;
}

void ptf_unregister_registered_jobs(void)
{
   lListElem *job;
   lListElem *next_job;

   DENTER(TOP_LAYER, "ptf_unregister_registered_jobs");

   next_job = lFirst(ptf_jobs);
   while ((job = next_job)) {
      lList *os_job_list;
      lListElem *os_job;
      lListElem *next_os_job;

      next_job = lNext(job);

      os_job_list = lGetList(job, JL_OS_job_list);
      next_os_job = lFirst(os_job_list);
      while ((os_job = next_os_job)) {
         next_os_job = lNext(os_job);

         psIgnoreJob(ptf_get_osjobid(os_job));
         DPRINTF(("PTF: Notify PDC to remove data for osjobid " u32 "\n",
                  lGetUlong(os_job, JO_OS_job_ID)));
         lDechainElem(os_job_list, os_job);
         lFreeElem(os_job);
      }
      DPRINTF(("PTF: Removing job " u32 "\n", lGetUlong(job, JL_job_ID)));
      lDechainElem(ptf_jobs, job);
      lFreeElem(job);
   }
   ptf_jobs = lFreeList(ptf_jobs);
   DPRINTF(("PTF: All jobs unregistered from PTF\n"));
}

int ptf_is_running(void)
{
   return is_ptf_running;
}

#if 0

/*--------------------------------------------------------------------
 * ptf_is_job_complete - check for job completion
 *   returns job completion status in job_complete parameter
 *--------------------------------------------------------------------*/

static int ptf_is_job_complete(u_long job_id, int *job_complete)
{
   int cc = 0;
   lList *job_list = ptf_jobs;
   lListElem *job = ptf_get_job(job_id);

   if (job) {
      if (job_complete) {
         *job_complete = lGetUlong(job, JL_state) & JL_JOB_COMPLETE;
      } else {
         cc = PTF_ERROR_INVALID_ARGUMENT;
      }
   } else {
      cc = PTF_ERROR_JOB_NOT_FOUND;
   }
   return cc;
}

/*--------------------------------------------------------------------
 * ptf_get_pids - return list of process IDs for job
 *--------------------------------------------------------------------*/

static int ptf_get_pids(u_long job_id, lList **pid_list)
{
   lListElem *osjob;
   lListElem *job = ptf_get_job(job_id);

   DENTER(TOP_LAYER, "ptf_get_pids");

   if (job) {
      *pid_list = NULL;
      for_each(osjob, lGetList(job, JL_OS_job_list)) {
         if (*pid_list) {
            lAddList(*pid_list, lCopyList(NULL, lGetList(osjob, JO_pid_list)));
         } else {
            *pid_list = lCopyList("PidList", lGetList(job, JO_pid_list));
         }
      }
   }
   DEXIT;
   return job ? 0 : -1;
}

/*--------------------------------------------------------------------
 * ptf_kill - kill all the processes belonging to job
 *--------------------------------------------------------------------*/

static int ptf_kill(u_long job_id, int sig)
{
   int cc = -1;
   lListElem *job = ptf_get_job(job_id);
   lListElem *osjob;

   DENTER(TOP_LAYER, "ptf_kill");

   if (job) {
      cc = 0;
      switch2start_user();
      for_each(osjob, lGetList(job, JL_OS_job_list)) {
#if defned(__sgi) || defined(ALPHA)
         lListElem *proc;

         for_each(proc, lGetList(osjob, JO_pid_list)) {
            cc += kill(lGetUlong(proc, JP_pid), sig);
         }
#elif CRAY
         cc += killm(C_JOB, ptf_get_osjobid(osjob), sig);
#endif
      }
      switch2admin_user();
   }
   DEXIT;
   return cc;
}
#endif

/*--------------------------------------------------------------------
 * ptf_errstr - return PTF error string
 *--------------------------------------------------------------------*/

const char *ptf_errstr(int ptf_error_code)
{
   const char *errmsg = MSG_ERROR_UNKNOWNERRORCODE;

   DENTER(TOP_LAYER, "ptf_errstr");

   switch (ptf_error_code) {
   case PTF_ERROR_NONE:
      errmsg = MSG_ERROR_NOERROROCCURED;
      break;

   case PTF_ERROR_DC_FAILURE:
      errmsg = ptf_error_str;
      break;

   case PTF_ERROR_INVALID_ARGUMENT:
      errmsg = MSG_ERROR_INVALIDARGUMENT;
      break;

   case PTF_ERROR_JOB_NOT_FOUND:
      errmsg = MSG_ERROR_JOBDOESNOTEXIST;
      break;

   default:
      break;
   }

   DEXIT;
   return errmsg;
}


void dump_list(lList *list)
{
   FILE *f;

   if (!(f = fdopen(1, "w"))) {
      fprintf(stderr, MSG_ERROR_COULDNOTOPENSTDOUTASFILE);
   }
   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBLIST);
   }
   fclose(f);
}


void dump_list_to_file(lList *list, char *file)
{
   FILE *f;

   if (!(f = fopen(file, "w+"))) {
      fprintf(stderr, MSG_ERROR_COULDNOTOPENSTDOUTASFILE);
      exit(1);
   }
   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBLIST);
   }
   fclose(f);
}


#ifdef MODULE_TEST

#define TESTJOB "./cpu_bound"

int main(int argc, char **argv)
{
   int i;
   FILE *f;
   lList *job_ticket_list, *job_usage_list;
   lListElem *jte, *job;
   u_long job_id = 0;
   osjobid_t os_job_id = 100;

#ifdef __SGE_COMPILE_WITH_GETTEXT__
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type) gettext,
                         (setlocale_func_type) setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type) textdomain);
   sge_init_language(NULL, NULL);
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


#if defined(ALPHA) && defined(notdef)
   if (setsid() < 0) {
      perror("setsid");
   }
#endif

   ptf_init();

#ifdef USE_DC

   psStartCollector();

#endif

   /* build job ticket list */

   job_ticket_list = lCreateList("jobticketlist", JB_Type);

   if (argc < 2) {
      jte = lCreateElem(JB_Type);
      lSetUlong(jte, JB_job_number, ++job_id);
      lSetUlong(jte, JB_ticket, 100);
      lSetString(jte, JB_script_file, TESTJOB);
      lAppendElem(job_ticket_list, jte);

      jte = lCreateElem(JB_Type);
      lSetUlong(jte, JB_job_number, ++job_id);
      lSetUlong(jte, JB_ticket, 500);
      lSetString(jte, JB_script_file, TESTJOB);
      lAppendElem(job_ticket_list, jte);

      jte = lCreateElem(JB_Type);
      lSetUlong(jte, JB_job_number, ++job_id);
      lSetUlong(jte, JB_ticket, 300);
      lSetString(jte, JB_script_file, TESTJOB);
      lAppendElem(job_ticket_list, jte);

      jte = lCreateElem(JB_Type);
      lSetUlong(jte, JB_job_number, ++job_id);
      lSetUlong(jte, JB_ticket, 100);
      lSetString(jte, JB_script_file, TESTJOB);
      lAppendElem(job_ticket_list, jte);
   } else {
      int argn;

      for (argn = 1; argn < argc; argn++) {
         int tickets = atoi(argv[argn]);

         if (tickets <= 0)
            tickets = 1;
         jte = lCreateElem(JB_Type);
         lSetUlong(jte, JB_job_number, ++job_id);
         lSetUlong(jte, JB_ticket, tickets);
         lSetString(jte, JB_script_file, TESTJOB);
         lAppendElem(job_ticket_list, jte);
      }
   }

   /* Start jobs and tell PTF jobs have started */

   for_each(jte, job_ticket_list) {
      pid_t pid;

#if defined(IRIX6) || defined(IRIX64)

      if (newarraysess() < 0) {
         perror("newarraysess");
         exit(1);
      }

      os_job_id = getash();
      printf(MSG_JOB_THEASHFORJOBXISY_DX,
             u32c(lGetUlong(jte, JB_job_number)), u64c(os_job_id));

#endif

      pid = fork();
      if (pid == 0) {
         char *jobname = lGetString(jte, JB_script_file);

         /* schedctl(NDPRI, 0, 0); */
#if defined(ALPHA)
         if (setsid() < 0) {
            perror("setsid");
            exit(1);
         }
#endif
         execl(jobname, jobname, NULL);
         perror("exec");
         exit(1);
      } else {
#if defined(ALPHA)
         os_job_id = pid;
#endif
         ptf_job_started(os_job_id, jte, 0);
      }
   }

#if defined(IRIX6) || defined(IRIX64)

   if (newarraysess() < 0) {
      perror("newarraysess");
      exit(2);
   }

   printf(MSG_JOB_MYASHIS_X, u64c(getash()));

#endif

   ptf_process_job_ticket_list(job_ticket_list);

   /* dump job_list */

   if (!(f = fdopen(1, "w"))) {
      fprintf(stderr, MSG_ERROR_COULDNOTOPENSTDOUTASFILE);
      exit(1);
   }

   if (lDumpList(f, ptf_jobs, 0) == EOF) {
      fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBLIST);
      exit(2);
   }

   /* get job usage */

   ptf_get_usage(&job_usage_list);

   for (i = 0; i < 100; i++) {
      u_long sum_of_tickets = 0;
      double sum_of_usage = 0;
      double sum_of_last_usage = 0;

      /* adjust priorities */

      ptf_adjust_job_priorities();

      for_each(job, ptf_jobs) {
         sum_of_tickets += lGetUlong(job, JL_tickets);
         sum_of_usage += lGetDouble(job, JL_usage);
         sum_of_last_usage += lGetDouble(job, JL_last_usage);
      }

#       if defined(CRAY) || defined(ALPHA)
#         define XFMT "%20d"
#       else
#         define XFMT "%20lld"
#       endif

      puts("                                           adj    total     curr"
           "                      last     next     prev");
      puts("job_id              os_job_id tickets    usage    usage    usage"
           "  target%  actual%  actual%  target%    diff% curr_pri   pri");
      for_each(job, ptf_jobs) {
         printf("%6d   " XFMT " %7d %8.3lf %8.3lf %8.3lf %8.3lf"
                " %8.3lf %8.3lf %8.3lf %8.3lf %8.3lf %5d\n",
                lGetUlong(job, JL_job_ID), ptf_get_osjobid(job),
                lGetUlong(job, JL_tickets), lGetDouble(job, JL_usage),
                lGetDouble(job, JL_adjusted_usage), 
                lGetDouble(job, JL_last_usage),
                sum_of_tickets ? lGetUlong(job, JL_tickets) /
                (double) sum_of_tickets : 0,
                sum_of_usage ? lGetDouble(job, JL_usage) / sum_of_usage : 0,
                sum_of_last_usage ? lGetDouble(job, JL_last_usage) /
                sum_of_last_usage : 0, 
                lGetDouble(job, JL_adjusted_current_proportion),
                lGetDouble(job, JL_diff_proportion), 
                lGetDouble(job, JL_curr_pri), lGetLong(job, JL_pri));
      }

#if 0
      /* 
       * dump job_list 
       */
      if (!(f = fdopen(1, "w"))) {
         fprintf(stderr, MSG_ERROR_COULDNOTOPENSTDOUTASFILE);
         exit(1);
      }

      if (lDumpList(f, ptf_jobs, 0) == EOF) {
         fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBLIST);
         exit(2);
      }

      /* dump job usage list */

      if (lDumpList(f, job_usage_list, 0) == EOF) {
         fprintf(stderr, MSG_ERROR_UNABLETODUMPJOBUSAGELIST);
         exit(2);
      }
#endif

      sleep(10);
   }

   for_each(job, ptf_jobs) {
      ptf_kill(ptf_get_jobid(job), SIGTERM);
   }

#ifdef USE_DC
   psStopCollector();
#endif

   return 0;
}

#endif /* MODULE_TEST */

#endif
