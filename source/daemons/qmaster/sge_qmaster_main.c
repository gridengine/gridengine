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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "basis_types.h"
#include "sge_qmaster_main.h"
#include "sgermon.h"
#include "sge_mt_init.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "setup_qmaster.h"
#include "sge_security.h"
#include "sge_manop.h"
#include "sge_mtutil.h"
#include "sge_lock.h"
#include "sge_qmaster_process_message.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "sge_qmaster_timed_event.h"
#include "sge_host_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_all_listsL.h"
#include "sge_calendar_qmaster.h"
#include "lock.h"
#include "qmaster_heartbeat.h"
#include "shutdown.h"
#include "setup.h"
#include "sge_spool.h"
#include "cl_commlib.h"
#include "sge_uidgid.h"
#include "sge_bootstrap.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_daemons_common.h"
#include "msg_utilib.h"  /* remove once 'daemonize_qmaster' did become 'sge_daemonize' */
#include "sge_any_request.h"
#include "sge.h"
#include "sge_qmod_qmaster.h"
#include "reschedule.h"
#include "sge_qmaster_threads.h"
#include "sge_job_qmaster.h"
#include "sge_profiling.h"


/****** qmaster/sge_qmaster_main/sge_qmaster_application_status() ************
*  NAME
*     sge_qmaster_application_status() -- commlib status callback function  
*
*  SYNOPSIS
*      unsigned long sge_qmaster_application_status(char** info_message) 
*
*  FUNCTION
*      This is the implementation of the commlib application status callback
*      function. This function is called from the commlib when a connected
*      client wants to get a SIRM (Status Information Response Message).
*      The standard client for this action is the qping command.
*
*      The callback function is set with cl_com_set_status_func() after
*      commlib initalization.
*
*      The function is called by a commlib thread which is not in the
*      context of the qmaster application. This means no qmaster specific
*      functions should be called (locking of global variables).
*
*      status 0:  no errors
*      status 1:  one or more threads has reached warning timeout
*      status 2:  one or more threads has reached error timeout
*      status 3:  thread alive timeout struct not initalized
*
*  INPUTS
*     char** info_message - pointer to an char* inside commlib.
*                           info message must be malloced, commlib will
*                           free this memory. 
*  RESULT
*     unsigned long status - status of application
*
*  NOTES
*     This function is MT save
* 
*******************************************************************************/
unsigned long sge_qmaster_application_status(char** info_message) {
   char buffer[1024];
   unsigned long status = 0;
   const char* status_message = NULL;
   double last_event_deliver_thread_time          = 0.0;
   double last_timed_event_thread_time = 0.0;
   double last_message_thread_time       = 0.0;
   double last_signal_thread_time        = 0.0;
   sge_thread_alive_times_t* thread_times       = NULL;

   struct timeval now;

   status_message = MSG_QMASTER_APPL_STATE_OK;
   sge_lock_alive_time_mutex();

   gettimeofday(&now,NULL);
   thread_times = sge_get_thread_alive_times();
   if ( thread_times != NULL ) {
      int warning_count = 0;
      int error_count = 0;
      double time1;
      double time2;
      time1 = now.tv_sec + (now.tv_usec / 1000000.0);

      time2 = thread_times->event_deliver_thread.timestamp.tv_sec + (thread_times->event_deliver_thread.timestamp.tv_usec / 1000000.0);
      last_event_deliver_thread_time          = time1 - time2;

      time2 = thread_times->timed_event_thread.timestamp.tv_sec + (thread_times->timed_event_thread.timestamp.tv_usec / 1000000.0);
      last_timed_event_thread_time = time1 - time2;

      time2 = thread_times->message_thread.timestamp.tv_sec + (thread_times->message_thread.timestamp.tv_usec / 1000000.0);
      last_message_thread_time       = time1 - time2;

      time2 = thread_times->signal_thread.timestamp.tv_sec + (thread_times->signal_thread.timestamp.tv_usec / 1000000.0);
      last_signal_thread_time        = time1 - time2;
   

      /* always set running state */
      thread_times->event_deliver_thread.state = 'R';
      thread_times->timed_event_thread.state   = 'R';
      thread_times->message_thread.state       = 'R';
      thread_times->signal_thread.state        = 'R';

      /* check for warning */
      if ( thread_times->event_deliver_thread.warning_timeout > 0 ) {
         if ( last_event_deliver_thread_time > thread_times->event_deliver_thread.warning_timeout ) {
            thread_times->event_deliver_thread.state = 'W';
            warning_count++;
         }
      }
      if ( thread_times->timed_event_thread.warning_timeout > 0 ) {
         if ( last_timed_event_thread_time > thread_times->timed_event_thread.warning_timeout ) {
            thread_times->timed_event_thread.state = 'W';
            warning_count++;
         }
      }
      if ( thread_times->message_thread.warning_timeout > 0 ) {
         if ( last_message_thread_time > thread_times->message_thread.warning_timeout ) {
            thread_times->message_thread.state = 'W';
            warning_count++;
         }
      }
      if ( thread_times->signal_thread.warning_timeout > 0 ) {
         if ( last_signal_thread_time > thread_times->signal_thread.warning_timeout ) {
            thread_times->signal_thread.state = 'W';
            warning_count++;
         }
      }

      /* check for error */
      if ( thread_times->event_deliver_thread.error_timeout > 0 ) {
         if ( last_event_deliver_thread_time > thread_times->event_deliver_thread.error_timeout ) {
            thread_times->event_deliver_thread.state = 'E';
            error_count++;
         }
      }
      if ( thread_times->timed_event_thread.error_timeout > 0 ) {
         if ( last_timed_event_thread_time > thread_times->timed_event_thread.error_timeout ) {
            thread_times->timed_event_thread.state = 'E';
            error_count++;
         }
      }
      if ( thread_times->message_thread.error_timeout > 0 ) {
         if ( last_message_thread_time > thread_times->message_thread.error_timeout ) {
            thread_times->message_thread.state = 'E';
            error_count++;
         }
      }
      if ( thread_times->signal_thread.error_timeout > 0 ) {
         if ( last_signal_thread_time > thread_times->signal_thread.error_timeout ) {
            thread_times->signal_thread.state = 'E';
            error_count++;
         }
      }

      if ( error_count > 0 ) {
         status = 2;
         status_message = MSG_QMASTER_APPL_STATE_TIMEOUT_ERROR;
      } else if ( warning_count > 0 ) {
         status = 1; 
         status_message = MSG_QMASTER_APPL_STATE_TIMEOUT_WARNING;
      }

      snprintf(buffer, 1024, MSG_QMASTER_APPL_STATE_CFCFCFCFS,
                    thread_times->event_deliver_thread.state,
                    last_event_deliver_thread_time,
                    thread_times->timed_event_thread.state,
                    last_timed_event_thread_time,
                    thread_times->message_thread.state,
                    last_message_thread_time,
                    thread_times->signal_thread.state,
                    last_signal_thread_time,
                    status_message);
      if (info_message != NULL && *info_message == NULL) {
         *info_message = strdup(buffer);
      }
   } else {
      status = 3;
   }

   sge_unlock_alive_time_mutex();
   return status;
}

/****** qmaster/sge_qmaster_main/main() ****************************************
*  NAME
*     main() -- qmaster entry point 
*
*  SYNOPSIS
*     int main(int argc, char* argv[]) 
*
*  FUNCTION
*     Qmaster entry point.
*
*     NOTE: The main thread must block all signals before any additional thread
*     is created. Failure to do so will ruin signal handling!
*
*  INPUTS
*     int argc     - number of commandline arguments 
*     char* argv[] - commandline arguments 
*
*  RESULT
*     0 - success 
*
*  NOTES
*     We check whether 'SGE_ROOT' is set before we daemonize. Once qmaster is
*     a daemon, we are no longer connected to a terminal and hence can not
*     output an error message to stdout or stderr.
*
*     We need to inovke 'prepare_enroll()' *before* the user id is switched via
*     'become_admin_user()'. This is because qmaster must be able to bind a so
*     called reserved port (requires root privileges) if configured to do so.
*
*******************************************************************************/
int main(int argc, char* argv[])
{
   sigset_t sig_set;
   int max_enroll_tries;
   int ret_val;

   DENTER_MAIN(TOP_LAYER, "qmaster");

   sge_prof_setup();

   sge_get_root_dir(true, NULL, 0, true);
   
#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   sge_init_language_func((gettext_func_type)gettext, (setlocale_func_type)setlocale, (bindtextdomain_func_type)bindtextdomain, (textdomain_func_type)textdomain);
   sge_init_language(NULL,NULL);   
#endif 

   /* qmaster doesn't support any commandline anymore,
      but we should show version string and -help option */
   if (argc != 1) {
      sge_mt_init();
      sigfillset(&sig_set);
      pthread_sigmask(SIG_SETMASK, &sig_set, NULL);
      sge_qmaster_thread_init(true);
      sge_process_qmaster_cmdline(argv);
      SGE_EXIT(1);
   }

   sge_daemonize_qmaster();

   sge_mt_init();

   sigfillset(&sig_set);
   pthread_sigmask(SIG_SETMASK, &sig_set, NULL);


   /* init qmaster threads without becomming admin user */
   sge_qmaster_thread_init(false);

   /* this must be done as root user to be able to bind ports < 1024 */
   max_enroll_tries = 30;
   while ( cl_com_get_handle((char*)prognames[QMASTER],1) == NULL) {
      prepare_enroll(prognames[QMASTER]); 
      max_enroll_tries--;
      if ( max_enroll_tries <= 0 ) {
         /* exit after 30 seconds */
         CRITICAL((SGE_EVENT, MSG_QMASTER_COMMUNICATION_ERRORS ));
         SGE_EXIT(1);
      }
      if (  cl_com_get_handle((char*)prognames[QMASTER],1) == NULL) {
        /* sleep when prepare_enroll() failed */
        sleep(1);
      }
   }

   /*
    * now the commlib up and running. Set qmaster application status function 
    * ( commlib callback function for qping status information response 
    *   messages (SIRM) )
    */
   ret_val = cl_com_set_status_func(sge_qmaster_application_status);
   if (ret_val != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }

   /* now we become admin user */
   sge_become_admin_user();

   sge_chdir_exit(bootstrap_get_qmaster_spool_dir(), 1);

   log_state_set_log_file(ERR_FILE);

   uti_state_set_exit_func(sge_exit_func);

   sge_start_heartbeat();

   sge_setup_lock_service();

   sge_setup_qmaster(argv);

   sge_start_periodic_tasks();

   sge_init_job_number();

   sge_setup_job_resend();

   sge_create_and_join_threads();

   sge_store_job_number(NULL);

   sge_qmaster_shutdown();

   sge_teardown_lock_service();

   sge_shutdown();

   sge_prof_cleanup();
   DEXIT;
   return 0;
} /* main() */


