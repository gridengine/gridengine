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
#include <sys/stat.h>
#include <errno.h>

#include "sge_all_listsL.h"
#include "usage.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "read_defaults.h"
#include "show_job.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_afsutil.h"
#include "setup_path.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_security.h"
#include "sge_answer.h"
#include "sge_job.h"
#include "japi.h"
#include "japiP.h"
#include "lck/sge_mtutil.h"
#include "uti/sge_log.h"

#include "msg_clients_common.h"
#include "msg_qsub.h"
#include "msg_qmaster.h"

extern char **environ;
static int ec_up = 0;
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t exit_cv = PTHREAD_COND_INITIALIZER;
static bool exited = false;

static char *get_bulk_jobid_string (long job_id, int start, int end, int step);
static void qsub_setup_sig_handlers (void);
static void qsub_terminate(void);
static void *sig_thread (void *dummy);
static int report_exit_status (int stat, const char *jobid);
static void error_handler (const char *message);

int main(int argc, char **argv);

/************************************************************************/
int main(
int argc,
char **argv 
) {
   lList *opts_cmdline = NULL;
   lList *opts_defaults = NULL;
   lList *opts_scriptfile = NULL;
   lList *opts_all = NULL;
   lListElem *job = NULL;
   lList *alp = NULL;
   lListElem *ep;
   int exit_status = 0;
   int just_verify;
   int tmp_ret;
   int wait_for_job = 0, is_immediate = 0;
   dstring session_key_out = DSTRING_INIT;
   dstring diag = DSTRING_INIT;
   dstring jobid = DSTRING_INIT;
   u_long32 start, end, step;
   u_long32 num_tasks;
   int count, stat;
   char *jobid_string = NULL;
   drmaa_attr_values_t *jobids = NULL;

   DENTER_MAIN(TOP_LAYER, "qsub");

   /* Set up the program information name */
   sge_setup_sig_handlers(QSUB);

   DPRINTF (("Initializing JAPI\n"));

   if (japi_init(NULL, NULL, NULL, QSUB, false, NULL, &diag)
                                                      != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, MSG_QSUB_COULDNOTINITIALIZEENV_S,
              sge_dstring_get_string (&diag));
      DEXIT;
      exit (1);
   }

   /*
    * read switches from the various defaults files
    */
   opt_list_append_opts_from_default_files(&opts_defaults, &alp, environ);
   tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   /*
    * append the commandline switches to the list
    */
   opt_list_append_opts_from_qsub_cmdline(&opts_cmdline, &alp,
                                          argv + 1, environ);
   tmp_ret = answer_list_print_err_warn(&alp, "qsub: ", MSG_QSUB_WARNING_S);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   /*
    * show usage if -help was in commandline
    */
   if (opt_list_has_X(opts_cmdline, "-help")) {
      sge_usage(stdout);
      SGE_EXIT(0);
   }

   /*
    * We will only read commandline options from scripfile if the script
    * itself should not be handled as binary
    */
   if (opt_list_is_X_true (opts_cmdline, "-b") ||
       (!opt_list_has_X (opts_cmdline, "-b") &&
        opt_list_is_X_true (opts_defaults, "-b"))) {
      DPRINTF(("Skipping options from script due to -b option\n"));
   } else {
      opt_list_append_opts_from_script(&opts_scriptfile, &alp, 
                                       opts_cmdline, environ);
      tmp_ret = answer_list_print_err_warn(&alp, MSG_QSUB_COULDNOTREADSCRIPT_S,
                                           MSG_WARNING);
      if (tmp_ret > 0) {
         SGE_EXIT(tmp_ret);
      }
   }

   /*
    * Merge all commandline options and interprete them
    */
   opt_list_merge_command_lines(&opts_all, &opts_defaults, 
                                &opts_scriptfile, &opts_cmdline);

   /* If "-sync y" is set, wait for the job to end. */   
   /* Remove all -sync switches since cull_parse_job_parameter()
    * doesn't know what to do with them. */
   while ((ep = lGetElemStr(opts_all, SPA_switch, "-sync"))) {
      if (lGetInt (ep, SPA_argval_lIntT) == TRUE) {
         wait_for_job = 1;
      }
      
      lRemoveElem(opts_all, ep);
   }
   
   if (wait_for_job) {
      DPRINTF (("Wait for job end\n"));
   }
   
   alp = cull_parse_job_parameter(opts_all, &job);

   tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   /* Check is we're just verifying the job */
   just_verify = (lGetUlong(job, JB_verify_suitable_queues)==JUST_VERIFY);
   DPRINTF (("Just verifying job\n"));

   /* Check if job is immediate */
   is_immediate = JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type));
   DPRINTF (("Job is%s immediate\n", is_immediate ? "" : " not"));

   DPRINTF(("Everything ok\n"));

   if (lGetUlong(job, JB_verify)) {
      cull_show_job(job, 0);
      DEXIT;
      SGE_EXIT(0);
   }

   if (is_immediate || wait_for_job) {
      pthread_t sigt;
      
      qsub_setup_sig_handlers(); 

      if (pthread_create (&sigt, NULL, sig_thread, (void *)NULL) != 0) {
         fprintf(stderr, MSG_QSUB_COULDNOTINITIALIZEENV_S,
                 " error preparing signal handling thread");
         
         exit_status = 1;
         goto Error;
      }
      
      if (japi_enable_job_wait (NULL, &session_key_out, error_handler, &diag) ==
                                       DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
         const char *msg = sge_dstring_get_string (&diag);
         fprintf(stderr, MSG_QSUB_COULDNOTINITIALIZEENV_S,
                 msg?msg:" error starting event client thread");
         
         exit_status = 1;
         goto Error;
      }
      
      ec_up = 1;
   }
   
   job_get_submit_task_ids(job, &start, &end, &step);
   num_tasks = (end - start) / step + 1;

   if (num_tasks > 1) {
      int error = japi_run_bulk_jobs(&jobids, job, start, end, step, &diag);
      if ((error != DRMAA_ERRNO_SUCCESS)) {
         fprintf(stderr, "%s\n", sge_dstring_get_string(&diag));
         
         /* BUGFIX: Issuezilla #1013
          * To quickly fix this issue, I'm mapping the JAPI/DRMAA error code
          * back into a GDI error code.  This is the easy solution.  The
          * correct solution would be to address issue #859, presumably by
          * having JAPI reuse the GDI error codes instead of the JAPI error
          * codes. */
         if (error == DRMAA_ERRNO_TRY_LATER) {
            exit_status = STATUS_NOTOK_DOAGAIN;
         }
         else {
            exit_status = 1;
         }
         
         goto Error;
      }

      DPRINTF(("job id is: %ld\n", jobids->it.ji.jobid));
      
      jobid_string = get_bulk_jobid_string (jobids->it.ji.jobid, start, end, step);
   }
   else if (num_tasks == 1) {
      int error = japi_run_job(&jobid, job, &diag);
      
      if (error != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "%s\n", sge_dstring_get_string(&diag));
         
         /* BUGFIX: Issuezilla #1013
          * To quickly fix this issue, I'm mapping the JAPI/DRMAA error code
          * back into a GDI error code.  This is the easy solution.  The
          * correct solution would be to address issue #859, presumably by
          * having JAPI reuse the GDI error codes instead of the JAPI error
          * codes. */
         if (error == DRMAA_ERRNO_TRY_LATER) {
            exit_status = STATUS_NOTOK_DOAGAIN;
         }
         else {
            exit_status = 1;
         }
         
         goto Error;
      }

      jobid_string = strdup (sge_dstring_get_string (&jobid));
      DPRINTF(("job id is: %s\n", jobid_string));

      sge_dstring_free (&jobid);
   }
   else {
      fprintf(stderr, MSG_QSUB_COULDNOTRUNJOB_S, "invalid task structure");
      
      exit_status = 1;
      goto Error;
   }
  
   /* only success message is printed to stdout */
   if (!just_verify)
      printf(MSG_QSUB_YOURJOBHASBEENSUBMITTED_SS, jobid_string, lGetString(job, JB_job_name));
   else
      printf(MSG_JOB_VERIFYFOUNDQ);

   if (wait_for_job || is_immediate) {
      int event;

      if (is_immediate) {
         fprintf(stderr, MSG_QSUB_WAITINGFORIMMEDIATEJOBTOBESCHEDULED);

         /* We only need to wait for the first task to be scheduled to be able
          * to say that the job is running. */
         tmp_ret = japi_wait(DRMAA_JOB_IDS_SESSION_ANY, &jobid, &stat,
                             DRMAA_TIMEOUT_WAIT_FOREVER, JAPI_JOB_START, &event,
                             NULL, &diag);

         if ((tmp_ret == DRMAA_ERRNO_SUCCESS) && (event == JAPI_JOB_START)) {
            fprintf(stderr, MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_S,
                  jobid_string);
         }
         /* A job finish event here means that the job was rejected. */
         else if ((tmp_ret == DRMAA_ERRNO_SUCCESS) &&
                  (event == JAPI_JOB_FINISH)) {
            fprintf(stderr, MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER);
            
            exit_status = 1;
            goto Error;
         }
         else {
            /* Since we told japi_wait to wait forever, we know that if it gets
             * a timeout, it's because it's been interrupted to exit, in which
             * case we don't complain. */
            if (tmp_ret != DRMAA_ERRNO_EXIT_TIMEOUT) {
               fprintf(stderr, MSG_QSUB_COULDNOTWAITFORJOB_S,
                       sge_dstring_get_string (&diag));
            }

            exit_status = 1;
            goto Error;
         }
      }
         
      if (wait_for_job) {
         /* Rather than using japi_synchronize on ALL for bulk jobs, we use
          * japi_wait on ANY num_tasks times because with synchronize, we would
          * have to wait for all the tasks to finish before we know if any
          * finished. */
         for (count = 0; count < num_tasks; count++) {
            /* Since there's only one running job in the session, we can just
             * wait for ANY. */
            if ((tmp_ret = japi_wait(DRMAA_JOB_IDS_SESSION_ANY, &jobid, &stat,
                          DRMAA_TIMEOUT_WAIT_FOREVER, JAPI_JOB_FINISH, &event,
                          NULL, &diag)) != DRMAA_ERRNO_SUCCESS) {
               if (tmp_ret != DRMAA_ERRNO_EXIT_TIMEOUT) {
                  fprintf(stderr, MSG_QSUB_COULDNOTWAITFORJOB_S, sge_dstring_get_string (&diag));
               }
               
               exit_status = 1;
               goto Error;
            }
            
            /* report how job finished */
            report_exit_status (stat, sge_dstring_get_string (&jobid));
         }
      }
   }

Error:
   FREE (jobid_string);
   lFreeList(alp);
   lFreeList(opts_all);
   
   if ((tmp_ret = japi_exit (1, JAPI_EXIT_NO_FLAG, &diag)) != DRMAA_ERRNO_SUCCESS) {
      if (tmp_ret != DRMAA_ERRNO_NO_ACTIVE_SESSION) {
         fprintf(stderr, MSG_QSUB_COULDNOTFINALIZEENV_S, sge_dstring_get_string (&diag));
      }
      else {
         struct timespec ts;
         /* We know that if we get a DRMAA_ERRNO_NO_ACTIVE_SESSION here, it's
          * because the signal handler thread called japi_exit().  We know this
          * because if the call to japi_init() fails, we just exit directly.
          * If the call to japi_init() succeeds, then we have an active session,
          * so coming here because of an error would not result in the
          * DRMAA_ERRNO_NO_ACTIVE_SESSION error. */
         DPRINTF (("Sleeping for 15 seconds to wait for the exit to finish.\n"));
         
         sge_relative_timespec(15, &ts);
         sge_mutex_lock("qsub_exit_mutex", SGE_FUNC, __LINE__, &exit_mutex);
         
         while (!exited) {
#if 1
            if (pthread_cond_timedwait (&exit_cv, &exit_mutex, &ts) == ETIMEDOUT) {
               DPRINTF (("Exit has not finished after 15 seconds.  Exiting.\n"));
               break;
            }
#else
            sleep (15);
#endif
         }
         
         sge_mutex_unlock("qsub_exit_mutex", SGE_FUNC, __LINE__, &exit_mutex);
      }
   }

   /* This is an exit() instead of an SGE_EXIT() because when the qmaster is
    * supended, SGE_EXIT() hangs. */
   exit (exit_status);
   DEXIT;
   return 0;
}

/****** get_bulk_jobid_string() ************************************************
*  NAME
*     get_bulk_jobid_string() -- Turn the job id and parameters into a string
*
*  SYNOPSIS
*     char *get_bulk_jobid_string (long job_id, int start, int end, int step)
*
*  FUNCTION
*     Creates a string from the job id, start task, end task, and task step.
*     The return job id string must be freed by the caller.
*
*  INPUT
*     long job_id   - The job's id number
*     int start     - The number of the first task in the job
*     int end       - The number of the last task in the job
*     int step      - The increment between job task numbers
*
*  RESULT
*     static char * - The job id string
*
*  NOTES
*     MT-NOTES: get_bulk_jobid_string() is MT safe
*******************************************************************************/
static char *get_bulk_jobid_string (long job_id, int start, int end, int step)
{
   char *jobid_str = (char *)malloc (sizeof (char) * 1024);
   char *ret_str = NULL;
   
   sprintf(jobid_str, "%ld.%d-%d:%d", job_id, start, end, step);
   ret_str = strdup (jobid_str);
   FREE (jobid_str);
   
   return ret_str;
}

/****** qsub_setup_sig_handlers() **********************************************
*  NAME
*     qsub_setup_sig_handlers() -- Set up the signal handlers
*
*  SYNOPSIS
*     void qsub_setup_sig_handlers (void)
*
*  FUNCTION
*     Blocks all signals so that the signal handler thread receives them.
*
*  NOTES
*     MT-NOTES: get_bulk_jobid_string() is MT safe
*******************************************************************************/
static void qsub_setup_sig_handlers (void)
{
   sigset_t sig_set;

   sigfillset (&sig_set);
   pthread_sigmask (SIG_BLOCK, &sig_set, NULL);
}

/****** qsub_terminate() *******************************************************
*  NAME
*     qsub_terminate() -- Terminates qsub
*
*  SYNOPSIS
*     void qsub_terminate(void)
*
*  FUNCTION
*     Prints out messages that qsub is ending and exits JAPI.
*
*  NOTES
*     MT-NOTES: qsub_terminate() is MT safe
*******************************************************************************/
static void qsub_terminate(void)
{
   dstring diag = DSTRING_INIT;
   
   fprintf(stderr, MSG_QSUB_INTERRUPTED);
   fprintf(stderr, MSG_QSUB_TERMINATING);

   if (japi_exit (1, JAPI_EXIT_KILL_PENDING, &diag) != DRMAA_ERRNO_SUCCESS) {
     fprintf(stderr, MSG_QSUB_COULDNOTFINALIZEENV_S, sge_dstring_get_string (&diag));
   }

   sge_dstring_free (&diag);

   sge_mutex_lock("qsub_exit_mutex", "qsub_terminate", __LINE__, &exit_mutex);
   exited = true;
   pthread_cond_signal (&exit_cv);
   sge_mutex_unlock("qsub_exit_mutex", "qsub_terminate", __LINE__, &exit_mutex);
}

/****** sig_thread() ***********************************************************
*  NAME
*     sig_thread() -- Signal handler thread
*
*  SYNOPSIS
*     void *sig_thread (void *dummy)
*
*  FUNCTION
*     Waits for a SIGINT or SIGTERM and then calls qsub_terminate().
*
*  INPUT
*     void *dummy - Unused
*
*  RESULT
*     static void * - Always NULL
*
*  NOTES
*     MT-NOTES: sig_thread() is MT safe
*******************************************************************************/
static void *sig_thread (void *dummy)
{
   int sig;
   sigset_t signal_set;
   dstring diag = DSTRING_INIT;

   sigemptyset (&signal_set);
   sigaddset (&signal_set, SIGINT);
   sigaddset (&signal_set, SIGTERM);

   /* Set up this thread so that when japi_exit() gets called, the GDI is
    * ready for use. */
   japi_init_mt (&diag);

   /* We don't care about sigwait's return (error) code because our response
    * to an error would be the same thing we're doing anyway: shutting down. */
   sigwait (&signal_set, &sig);
   
   qsub_terminate ();
   
   return (void *)NULL;
}

/****** report_exit_status() ***************************************************
*  NAME
*     report_exit_status() -- Prints a job's exit status
*
*  SYNOPSIS
*     static int report_exit_status (int stat, const char *jobid)
*
*  FUNCTION
*     Prints a job's exit status to stdout.
*
*  INPUT
*     int stat          - The job's exit status
*     const char *jobid - The job id string
*
*  RESULT
*     static int        - The exit code of the job
*
*  NOTES
*     MT-NOTES: report_exit_status() is MT safe
*******************************************************************************/
static int report_exit_status (int stat, const char *jobid)
{
   int aborted, exited, signaled;
   int exit_status = 0;
   
   japi_wifaborted(&aborted, stat, NULL);

   if (aborted) {
      printf(MSG_QSUB_JOBNEVERRAN_S, jobid);
   }
   else {
      japi_wifexited(&exited, stat, NULL);
      if (exited) {
         japi_wexitstatus(&exit_status, stat, NULL);
         printf(MSG_QSUB_JOBEXITED_SI, jobid, exit_status);
      } else {
         japi_wifsignaled(&signaled, stat, NULL);
         
         if (signaled) {
            dstring termsig = DSTRING_INIT;
            japi_wtermsig(&termsig, stat, NULL);
            printf(MSG_QSUB_JOBRECEIVEDSIGNAL_SS, jobid,
                    sge_dstring_get_string (&termsig));
            sge_dstring_free (&termsig);
         }
         else {
            printf(MSG_QSUB_JOBFINISHUNCLEAR_S, jobid);
         }

         exit_status = 1;
      }
   }
   
   return exit_status;
}

/****** error_handler() ********************************************************
*  NAME
*     error_handler() -- Prints JAPI error messages
*
*  SYNOPSIS
*     static void error_handler (const char *message)
*
*  FUNCTION
*     Prints error messages from JAPI event client thread to stderr
*
*  INPUT
*     const char *message - The message to print
*
*  NOTES
*     MT-NOTES: error_handler() is MT safe
*******************************************************************************/
static void error_handler (const char *message)
{
   fprintf (stderr, message);
}
