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

   DENTER_MAIN(TOP_LAYER, "qmaster");

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

   /* now we become admin user */
   sge_become_admin_user();

   sge_chdir_exit(bootstrap_get_qmaster_spool_dir(), 1);

   log_state_set_log_file(ERR_FILE);

   uti_state_set_exit_func(sge_exit_func);

   sge_start_heartbeat();

   sge_setup_lock_service();

   sge_setup_qmaster(argv);

   sge_start_periodic_tasks();

   sge_setup_job_resend();

   sge_create_and_join_threads();

   sge_qmaster_shutdown();

   sge_teardown_lock_service();

   sge_shutdown();

   DEXIT;
   return 0;
} /* main() */


