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
#include "sge_log.h"
#include "sge_afsutil.h"
#include "setup_path.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_security.h"
#include "sge_answer.h"
#include "sge_job.h"
#include "japi.h"
#include "japiP.h"

#include "msg_clients_common.h"
#include "msg_qsub.h"

extern char **environ;
static int ec_up = 0;
static char **jobid_strings = NULL;

static char *get_bulk_jobid_string (long job_id, int start, int end, int step);
static void qsub_setup_sig_handlers (void);
static void qsub_terminate(void);
static void *sig_thread (void *dummy);

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

   DENTER_MAIN(TOP_LAYER, "qsub");

   /* Set up the program information name */
   sge_setup_sig_handlers(QSUB);
   qsub_setup_sig_handlers ();

   DPRINTF (("Initializing JAPI\n"));

   if (japi_init(NULL, NULL, NULL, QSUB, false, &diag) != DRMAA_ERRNO_SUCCESS) {
      printf (MSG_QSUB_COULDNOTINITIALIZEENV_U, sge_dstring_get_string (&diag));
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
   tmp_ret = answer_list_print_err_warn(&alp, "qsub: ", MSG_QSUB_WARNING);
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
      tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
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
   if (opt_list_is_X_true (opts_all, "-sync")) {
      wait_for_job = 1;
      DPRINTF (("Wait for job end\n"));
   }
   
   /* Remove all -sync switches since cull_parse_job_parameter()
    * doesn't know what to do with them. */
   while ((ep = lGetElemStr(opts_all, SPA_switch, "-sync"))) {
      lRemoveElem(opts_all, ep);
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
#ifndef NO_SGE_COMPILE_DEBUG
   if (rmon_mlgetl(&DEBUG_ON, TOP_LAYER) & INFOPRINT) { 
      lWriteElemTo(job, stdout);
   }
#endif

   if (lGetUlong(job, JB_verify)) {
      cull_show_job(job, 0);
      DEXIT;
      SGE_EXIT(0);
   }

   if (is_immediate || wait_for_job) {
      pthread_t sigt;
      
      if (pthread_create (&sigt, NULL, sig_thread, (void *)NULL) != 0) {
         printf (MSG_QSUB_COULDNOTINITIALIZEENV_U, " error preparing signal handling thread");
         DEXIT;
         SGE_EXIT (1);
      }
      
      if (japi_enable_job_wait (NULL, &session_key_out, &diag) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
         const char *msg = sge_dstring_get_string (&diag);
         printf (MSG_QSUB_COULDNOTINITIALIZEENV_U, msg?msg:" error starting event client thread");
         DEXIT;
         SGE_EXIT (1);
      }
      
      ec_up = 1;
   }
   
   job_get_submit_task_ids(job, &start, &end, &step);
   num_tasks = (end - start) / step + 1;
   jobid_strings = (char**)malloc (sizeof (char *) * (num_tasks));

   if (num_tasks > 1) {
      drmaa_attr_values_t *jobids = NULL;

      if ((japi_run_bulk_jobs(&jobids, job, start, end, step, &diag) != DRMAA_ERRNO_SUCCESS)) {
         printf (MSG_QSUB_COULDNOTRUNJOB_U, sge_dstring_get_string (&diag));
         DEXIT;
         SGE_EXIT (1);
      }

      DPRINTF(("job id is: %ld\n", jobids->it.ji.jobid));

      for (count = 0; count < num_tasks; count++) {            
         japi_string_vector_get_next (jobids, &jobid);
         jobid_strings[count] = strdup (sge_dstring_get_string (&jobid));
      }
      
      jobid_string = get_bulk_jobid_string (jobids->it.ji.jobid, start, end, step);
   }
   else if (num_tasks == 1) {
      if (japi_run_job(&jobid, job, &diag) != DRMAA_ERRNO_SUCCESS) {
         printf (MSG_QSUB_COULDNOTRUNJOB_U, sge_dstring_get_string (&diag));
         DEXIT;
         SGE_EXIT (1);
      }

      jobid_strings[0] = strdup (sge_dstring_get_string (&jobid));
      
      DPRINTF(("job id is: %s\n", jobid_strings[0]));

      jobid_string = strdup (jobid_strings[0]);
      sge_dstring_free (&jobid);
   }
   else {
      printf (MSG_QSUB_COULDNOTRUNJOB_U, "invalid task structure");
      DEXIT;
      SGE_EXIT (1);
   }
   
   printf (MSG_QSUB_YOURJOBHASBEENSUBMITTED_U, jobid_string, lGetString (job, JB_job_name));

   if (wait_for_job || is_immediate) {
      int aborted, exited, signaled, event;

      if (is_immediate) {
         printf(MSG_QSUB_WAITINGFORIMMEDIATEJOBTOBESCHEDULED);

         /* We only need to wait for the first task to be scheduled to be able
          * to say that the job is running. */
         if ((tmp_ret = japi_wait(jobid_strings[0], &jobid, &stat,
                        DRMAA_TIMEOUT_WAIT_FOREVER, JAPI_JOB_START, &event,
                        NULL, &diag)) != DRMAA_ERRNO_SUCCESS) {
            /* Since we told japi_wait to wait forever, we know that if it gets
             * a timeout, it's because it's been interrupted to exit, in which
             * case we don't complain. */
            if (tmp_ret != DRMAA_ERRNO_EXIT_TIMEOUT) {
               printf (MSG_QSUB_COULDNOTWAITFORJOB_U,
                       sge_dstring_get_string (&diag));
            }
            
            /* If -now failed, don't bother with -sync */
            wait_for_job = 0;
         }

         if (event == JAPI_JOB_START) {
            printf(MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U,
                  jobid_string);
         }
         /* Don't complain if we were shut down */
         else if (tmp_ret != DRMAA_ERRNO_EXIT_TIMEOUT) {
            printf(MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER);
            
            wait_for_job = 0;
         }
      }
         
      if (wait_for_job) {
         for (count = 0; count < num_tasks; count++) {            
            if ((tmp_ret = japi_wait(jobid_strings[count], &jobid, &stat,
                          DRMAA_TIMEOUT_WAIT_FOREVER, JAPI_JOB_FINISH, &event,
                          NULL, &diag)) != DRMAA_ERRNO_SUCCESS) {
               if (tmp_ret != DRMAA_ERRNO_EXIT_TIMEOUT) {
                  printf (MSG_QSUB_COULDNOTWAITFORJOB_U, sge_dstring_get_string (&diag));
               }
               
               break;
            }
            
            /* report how job finished */
            japi_wifaborted(&aborted, stat, NULL);

            if (aborted) {
               printf(MSG_QSUB_JOBNEVERRAN_U, jobid_strings[count]);
            }
            else {
               japi_wifexited(&exited, stat, NULL);
               if (exited) {
                  japi_wexitstatus(&exit_status, stat, NULL);
                  printf(MSG_QSUB_JOBEXITED_U, jobid_strings[count], exit_status);
               } else {
                  japi_wifsignaled(&signaled, stat, NULL);
                  if (signaled) {
                     dstring termsig = DSTRING_INIT;
                     japi_wtermsig(&termsig, stat, NULL);
                     printf(MSG_QSUB_JOBRECEIVEDSIGNAL_U, jobid_strings[count], sge_dstring_get_string (&termsig));
                     sge_dstring_free (&termsig);
                  }
                  else {
                     printf(MSG_QSUB_JOBFINISHUNCLEAR_U, jobid_strings[count]);
                  }

                  exit_status = 1;
               }
            }
         }
      }
   }

   /* We free these before calling japi_exit() because if we were interrtuped,
    * the call to japi_exit() will block, and then the signal handler would exit
    * the process before we would get to free things. */
   for (count = 0; count < num_tasks; count++) {
      FREE (jobid_strings[count]);
   }

   FREE (jobid_string);
   lFreeList(alp);
   lFreeList(opts_all);
   
   if ((tmp_ret = japi_exit (1, JAPI_EXIT_NO_FLAG, &diag)) != DRMAA_ERRNO_SUCCESS) {
      if (tmp_ret != DRMAA_ERRNO_NO_ACTIVE_SESSION) {
         printf (MSG_QSUB_COULDNOTFINALIZEENV_U, sge_dstring_get_string (&diag));
      }
   }
   
   SGE_EXIT(exit_status);
   DEXIT;
   return 0;
}

static char *get_bulk_jobid_string (long job_id, int start, int end, int step)
{
   char *jobid_str = (char *)malloc (sizeof (char) * 1024);
   char *ret_str = NULL;
   
   sprintf (jobid_str, "%ld.%d-%d:%d", job_id, start, end, step);
   ret_str = strdup (jobid_str);
   FREE (jobid_str);
   
   return ret_str;
}

static void qsub_setup_sig_handlers ()
{
   sigset_t sig_set;

   sigfillset (&sig_set);
   pthread_sigmask (SIG_BLOCK, &sig_set, NULL);
}

static void qsub_terminate(void)
{
   dstring diag = DSTRING_INIT;
   
   fprintf (stderr, MSG_QSUB_INTERRUPTED_U);
   fprintf (stderr, MSG_QSUB_TERMINATING_U);

   if (japi_exit (1, JAPI_EXIT_KILL_PENDING, &diag) != DRMAA_ERRNO_SUCCESS) {
     fprintf (stderr, MSG_QSUB_COULDNOTFINALIZEENV_U, sge_dstring_get_string (&diag));
   }

   sge_dstring_free (&diag);

   SGE_EXIT (1);
}

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
