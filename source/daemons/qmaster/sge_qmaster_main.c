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
#include "sge_qmaster_main.h"


/*
 * This is NOT officially approved by POSIX. In fact, POSIX does not specify a
 * 'null thread id'. Given, that any variable with static storage class will be
 * initialized to '0', using '0' as a thread id would be an insane choice anyway.
 */
enum { INVALID_THREAD = 0 };

typedef struct {
   pthread_mutex_t mutex;      /* used for thread exclusion  */
   pthread_cond_t  cond_var;   /* used for thread waiting    */
   pthread_t       sig_thrd;   /* signal thread              */
   bool            shutdown;   /* true -> shutdown qmaster   */
   short           thrd_count; /* number of active threads (main thread does not count) */
} qmaster_control_t;


static qmaster_control_t Qmaster_Control = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, INVALID_THREAD, false, 0};
static pthread_mutex_t   Global_Lock;

/* lock service provider */
static void setup_lock_service(void);
static void teardown_lock_service(void);
static void lock_callback(sge_locktype_t, sge_lockmode_t, sge_locker_t);
static void unlock_callback(sge_locktype_t, sge_lockmode_t, sge_locker_t);
static sge_locker_t id_callback(void);

/* thread management */
static void      create_and_join_threads(void);
static void      inc_thread_count(void);
static bool      should_terminate(void);
static void      wait_for_thread_termination(void);
static void      set_signal_thread(pthread_t);
static pthread_t get_signal_thread(void);
static void*     signal_thread(void*);
static void*     message_thread(void*);

/* misc functions */
static void daemonize_qmaster(void);
static void become_admin_user(void);
static void start_heartbeat(void);
static void increment_heartbeat(te_event_t);
static void start_periodic_tasks(void);
static void qmaster_shutdown(void);
static void exit_func(int);


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
*     We need to inovke 'prepare_enroll()' *before* the user id is switched via
*     'become_admin_user()'. This is because qmaster must be able to bind a so
*     called reserved port (requires root privileges) if configured to do so.
*
*******************************************************************************/
int main(int argc, char* argv[])
{
   sigset_t sig_set;

   DENTER_MAIN(TOP_LAYER, "qmaster");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   sge_init_language_func((gettext_func_type)gettext, (setlocale_func_type)setlocale, (bindtextdomain_func_type)bindtextdomain, (textdomain_func_type)textdomain);
   sge_init_language(NULL,NULL);   
#endif 

   daemonize_qmaster();

   sge_mt_init();

   sigfillset(&sig_set);
   pthread_sigmask(SIG_SETMASK, &sig_set, NULL);

   sge_qmaster_thread_init();

   prepare_enroll(prognames[QMASTER], 1, NULL);

   become_admin_user();

   sge_chdir_exit(bootstrap_get_qmaster_spool_dir(), 1);

   log_state_set_log_file(ERR_FILE);

   uti_state_set_exit_func(exit_func);

   start_heartbeat();

   sge_setup_qmaster(argv);

   setup_lock_service();

   start_periodic_tasks();

   create_and_join_threads();

   teardown_lock_service();

   qmaster_shutdown();

   DEXIT;
   return 0;
} /* main() */

/****** qmaster/sge_qmaster_main/sge_gdi_kill_master() *************************
*  NAME
*     sge_gdi_kill_master() -- Shutdown qmaster via GDI
*
*  SYNOPSIS
*     void sge_gdi_kill_master(char *host, sge_gdi_request *request, 
*     sge_gdi_request *answer) 
*
*  FUNCTION
*     Shutdown qmaster by means of a GDI request. This operation is only
*     permitted for a user of type 'manager'.
*
*  INPUTS
*     char *host               - host this request does come from 
*     sge_gdi_request *request - GDI request 
*     sge_gdi_request *answer  - GDI answer 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_gdi_kill_master() is not MT safe.
*     MT-NOTE:
*     MT-NOTE: This is acceptable for now, because this function is currently
*     MT-NOTE: only invoked from the message thread.
*
*     TODO-AD: make this function thread safe. 'manop_is_manager()' is NOT MT
*     TODO-AD  safe.
*
*******************************************************************************/
void sge_gdi_kill_master(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{
   uid_t uid;
   gid_t gid;
   char username[128];
   char groupname[128];

   DENTER(GDI_LAYER, "sge_gdi_kill_master");

   if (sge_get_auth_info(request, &uid, username, &gid, groupname) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!manop_is_manager(username)) {
      ERROR((SGE_EVENT, MSG_SHUTDOWN_SHUTTINGDOWNQMASTERREQUIRESMANAGERPRIVILEGES));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (pthread_kill(get_signal_thread(), SIGINT) == 0) {
      INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, username, host, prognames[QMASTER]));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   } else {
      ERROR((SGE_EVENT, MSG_SGETEXT_KILL_FAILED_SSS, username, host, prognames[QMASTER]));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ERROR1, ANSWER_QUALITY_ERROR);
   }

   DEXIT;
   return;
} /* sge_gdi_kill_master() */

/****** qmaster/sge_qmaster_main/daemonize_qmaster() ***************************
*  NAME
*     daemonize_qmaster() -- Turn qmaster into a daemon. 
*
*  SYNOPSIS
*     static void daemonize_qmaster(void) 
*
*  FUNCTION
*     If the environment variable 'SGE_ND' is set, the functions does return
*     immediately.
*
*     First, we call 'fork()'. If the process was started as a shell command in
*     the foreground, when the parent terminates, the shell thinks the command
*     is done. This automatically runs the child process in the background.
*     Also, the child inherits the process group ID from the parent but gets
*     its own process ID. This guarantees that the child is not a process group
*     leader. 
*
*     We call 'setsid()' to create a new session. The process becomes the
*     session leader of the new session, becomes the process group leader of a
*     new process group, and has no controlling terminal.
*
*     By calling 'fork()' a second time, we guarantee the the daemon (second
*     child) is no longer a session leader, so it cannot acquire a controlling
*     terminal. We must ignore 'SIGHUP' because when the session leader
*     terminates (the first child), all processes in the session (our second
*     child) receive the 'SIGHUP' signal.
*
*     We close any open descriptors that are inherited from the process that
*     executed 'sge_qmaster', normally a shell. We redirect 'stdin', 'stdout'
*     and 'stderr' to '/dev/null'. The reason for opening these descriptors
*     is so that any library function called by 'sge_qmaster' that assumes it
*     can read from standard input or write to either standard ouput or
*     standard error will not fail.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: daemonize_qmaster() is not MT safe 
*
*******************************************************************************/
static void daemonize_qmaster(void)
{
   enum { true = 1 };

   pid_t pid = -1;
   int i;

   DENTER(TOP_LAYER, "daemonize_qmaster");

   if(getenv("SGE_ND") != NULL) {
      DPRINTF(("sge_qmaster is not daemonized\n"));
      DEXIT;
      return;
   }

   if((pid = fork()) != 0) {
      if (pid < 0) {
         CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      }
      exit(0); /* parent terminates */
   }

   setsid();

   signal(SIGHUP, SIG_IGN);

   if((pid = fork()) != 0) {
      if (pid < 0) {
         CRITICAL((SGE_EVENT, MSG_PROC_SECONDFORKFAILED_S , strerror(errno)));
      }
      exit(0); /* child 1 terminates */
   }

   for (i = 0; i < sysconf(_SC_OPEN_MAX); i++) { close(i); }

   if (open("/dev/null", O_RDONLY, 0) != 0) { SGE_EXIT(0); }
   if (open("/dev/null", O_RDWR,   0) != 1) { SGE_EXIT(0); }
   if (open("/dev/null", O_RDWR,   0) != 2) { SGE_EXIT(0); }

   uti_state_set_daemonized(true);

   DEXIT;
   return;
} /* daemonize_qmaster() */

/****** qmaster/sge_qmaster_main/become_admin_user() ***************************
*  NAME
*     become_admin_user() -- Become admin user. 
*
*  SYNOPSIS
*     static void become_admin_user(void) 
*
*  FUNCTION
*     Get admin user from bootstrap configuration. Set admin user and change
*     the effective UID/GID to the admin user UID/GID. 
*
*     Note: The effective UID does determine file access permissions.
*
*  INPUTS
*     void - none
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: become_admin_user() is not MT safe 
*
*******************************************************************************/
static void become_admin_user(void)
{
   char str[1024];

   DENTER(TOP_LAYER, "become_admin_user");

   if (sge_set_admin_username(bootstrap_get_admin_user(), str) == -1) {
      CRITICAL((SGE_EVENT, str));
      SGE_EXIT(1);
   }

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_ERROR_CANTSWITCHTOADMINUSER));
      SGE_EXIT(1);
   }

   DEXIT;
   return;
} /* become_admin_user() */

/****** qmaster/sge_qmaster_main/start_heartbeat() *****************************
*  NAME
*     start_heartbeat() -- Start qmaster heartbeat. 
*
*  SYNOPSIS
*     static void start_heartbeat(void) 
*
*  FUNCTION
*     Add heartbeat event and register according event handler. 
*
*     NOTE: Before registering the heartbeat event, we do increment the
*     heartbeat manually once. This is the 'startup heartbeat'. The first time
*     the hearbeat will be incremented through the heartbeat event handler is
*     after HEARTBEAT_INTERVAL seconds. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: start_heartbeat() is MT safe 
*
*******************************************************************************/
static void start_heartbeat(void)
{
   enum { HEARTBEAT_INTERVAL = 30 };

   te_event_t ev = NULL;

   DENTER(TOP_LAYER, "start_heartbeat");

   inc_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE);

   te_register_event_handler(increment_heartbeat, TYPE_HEARTBEAT_EVENT);

   ev = te_new_event(HEARTBEAT_INTERVAL, TYPE_HEARTBEAT_EVENT, RECURRING_EVENT, 0, 0, "heartbeat-event");
   te_add_event(ev);
   te_free_event(ev);

   DEXIT;
   return;
} /* start_heartbeat(void) */

/****** qmaster/sge_qmaster_main/create_and_join_threads() *********************
*  NAME
*     create_and_join_threads() -- create and join qmaster threads
*
*  SYNOPSIS
*     static void create_and_join_threads(void) 
*
*  FUNCTION
*     Create and join qmaster threads. This function does block until all 
*     created threads could have been joined.
*
*     NOTE: 'set_signal_thread()' must be invoked *before* the message thread
*     is created. Otherwise a GID request to kill the qmaster may not work!
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: create_and_join_threads() is not MT safe 
*
*******************************************************************************/
static void create_and_join_threads(void)
{
   enum { NUM_THRDS = 2 };

   pthread_t tids[NUM_THRDS];
   int i;

   DENTER(TOP_LAYER, "create_and_join_threads");

   pthread_create(&(tids[0]), NULL, signal_thread, NULL);
   inc_thread_count();
   
   set_signal_thread(tids[0]);

   pthread_create(&(tids[1]), NULL, message_thread, NULL);
   inc_thread_count();

   for (i = 0; i < NUM_THRDS; i++) {
      pthread_join(tids[i], NULL);
   }

   DEXIT;
   return;
} /* create_and_join_threads() */

/****** qmaster/sge_qmaster_main/set_signal_thread() ***************************
*  NAME
*     set_signal_thread() -- Store signal thread in qmaster control structure
*
*  SYNOPSIS
*     static void set_signal_thread(pthread_t aThread) 
*
*  FUNCTION
*     Store signal thread in qmaster control structure. To invalidate the
*     signal thread, use 'INVALID_THREAD' as an argument
*
*     NOTE: This function should *ONLY* be invoked from the function which
*     does create the signal thread or from the signal thread itself.
*
*  INPUTS
*     pthread_t aThread - signal thread or 'INVALID_THREAD'.
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: set_signal_thread() is MT safe.
*
*******************************************************************************/
static void set_signal_thread(pthread_t aThread)
{
   DENTER(TOP_LAYER, "set_signal_thread");

   sge_mutex_lock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   Qmaster_Control.sig_thrd = aThread;

   sge_mutex_unlock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   DEXIT;
   return;
} /* set_signal_thread() */

/****** qmaster/sge_qmaster_main/get_signal_thread() ***************************
*  NAME
*     get_signal_thread() -- Get signal thread from the qmaster control
*                            structure.
*
*  SYNOPSIS
*     static pthread_t get_signal_thread(void) 
*
*  FUNCTION
*     Get signal thread from the qmaster control structure.
*
*     NOTE: There is *NO* guarantee that the signal thread returned by this 
*     function still is alive and kicking! 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     pthread_t - signal thread or 'INVALID_THREAD'. 
*
*  NOTES
*     MT-NOTE: get_signal_thread() is MT safe 
*
*******************************************************************************/
static pthread_t get_signal_thread(void)
{
   pthread_t thrd = INVALID_THREAD;

   DENTER(TOP_LAYER, "get_signal_thread");

   sge_mutex_lock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   thrd = Qmaster_Control.sig_thrd;

   sge_mutex_unlock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   DEXIT;
   return thrd;
} /* get_signal_thread() */

/****** qmaster/sge_qmaster_main/increment_heartbeat() *************************
*  NAME
*     increment_heartbeat() -- Event handler for heartbeat events
*
*  SYNOPSIS
*     static void increment_heartbeat(te_event_t anEvent) 
*
*  FUNCTION
*     Update qmaster heartbeat file.
*
*  INPUTS
*     te_event_t anEvent - heartbeat event 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: increment_hearbeat() is NOT MT safe. This function is only
*     MT-NOTE: invoked from within the event delivery thread.
*
*     We do assume that the system clock does NOT run backwards. However, we
*     do cope with a system clock which has been put back.
*
*******************************************************************************/
static void increment_heartbeat(te_event_t anEvent)
{
   DENTER(TOP_LAYER, "increment_heartbeat");

   if (inc_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE)) {
      ERROR((SGE_EVENT, MSG_HEARTBEAT_FAILEDTOINCREMENTHEARBEATFILEXINSPOOLDIR_S, QMASTER_HEARTBEAT_FILE));
   }

   DEXIT;
   return;
} /* increment_heartbeat() */

/****** qmaster/sge_qmaster_main/start_periodic_tasks() ************************
*  NAME
*     start_periodic_tasks() -- Start periodic qmaster tasks. 
*
*  SYNOPSIS
*     static void start_periodic_tasks(void) 
*
*  FUNCTION
*     Start periodic qmaster tasks. Periodic tasks are implemented as recurring
*     events. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: start_periodic_tasks() is not MT safe 
*
*******************************************************************************/
static void start_periodic_tasks(void)
{
   te_event_t ev = NULL;

   DENTER(TOP_LAYER, "start_periodic_tasks");

   te_register_event_handler(sge_job_resend_event_handler, TYPE_JOB_RESEND_EVENT);

   te_register_event_handler(sge_load_value_cleanup_handler, TYPE_LOAD_VALUE_CLEANUP_EVENT);
   ev = te_new_event(15, TYPE_LOAD_VALUE_CLEANUP_EVENT, RECURRING_EVENT, 0, 0, "load-value-cleanup");
   te_add_event(ev);
   te_free_event(ev);

   te_register_event_handler(sge_zombie_job_cleanup_handler, TYPE_ZOMBIE_JOB_CLEANUP_EVENT);
   ev = te_new_event(30, TYPE_ZOMBIE_JOB_CLEANUP_EVENT, RECURRING_EVENT, 0, 0, "zombie-job-cleanup");
   te_add_event(ev);
   te_free_event(ev);

   te_register_event_handler(sge_automatic_user_cleanup_handler, TYPE_AUTOMATIC_USER_CLEANUP_EVENT);
   ev = te_new_event(60, TYPE_AUTOMATIC_USER_CLEANUP_EVENT, RECURRING_EVENT, 0, 0, "automatic-user-cleanup");
   te_add_event(ev);
   te_free_event(ev);

   te_register_event_handler(sge_security_event_handler, TYPE_SECURITY_EVENT);
   ev = te_new_event(10, TYPE_SECURITY_EVENT, RECURRING_EVENT, 0, 0, "security-event");
   te_add_event(ev);
   te_free_event(ev);

   DEXIT;
   return;
} /* start_periodic_tasks() */

/****** qmaster/sge_qmaster_main/setup_lock_service() **************************
*  NAME
*     setup_lock_service() -- setup lock service 
*
*  SYNOPSIS
*     static void setup_lock_service(void) 
*
*  FUNCTION
*     Determine number of locks needed. Create and initialize the respective
*     mutexes. Register the callbacks required by the locking API 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: setup_lock_service() is NOT MT safe. 
*
*     Currently we do not use so called recursive mutexes. This may change
*     *without* warning, if necessary!
*
*  SEE ALSO
*     libs/lck/sge_lock.c
*
*******************************************************************************/
static void setup_lock_service(void)
{
   DENTER(TOP_LAYER, "setup_lock_service");

   pthread_mutex_init(&Global_Lock, NULL);

   sge_set_lock_callback(lock_callback);
   sge_set_unlock_callback(unlock_callback);
   sge_set_id_callback(id_callback);

   DEXIT;
   return;
} /* setup_lock_service() */

/****** qmaster/sge_qmaster_main/teardown_lock_service() ***********************
*  NAME
*     teardown_lock_service() -- teardown lock service 
*
*  SYNOPSIS
*     static void teardown_lock_service(void) 
*
*  FUNCTION
*     Destroy and free mutexes created with 'setup_lock_service()' 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: teardown_lock_service() is NOT MT safe. 
*
*******************************************************************************/
static void teardown_lock_service(void)
{
   DENTER(TOP_LAYER, "teardown_lock_service");

   pthread_mutex_destroy(&Global_Lock);
   
   DEXIT;
   return;
} /* teardown_lock_service() */

/****** qmaster/sge_qmaster_main/lock_callback() *******************************
*  NAME
*     lock_callback() -- lock callback 
*
*  SYNOPSIS
*     static void lock_callback(sge_locktype_t aType, sge_lockmode_t aMode, 
*     sge_locker_t anID) 
*
*  FUNCTION
*     Acquire global lock determined by 'aType' in mode 'aMode'. 
*
*  INPUTS
*     sge_locktype_t aType - lock type 
*     sge_lockmode_t aMode - lock mode 
*     sge_locker_t anID    - locker id
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: lock_callback() is MT safe. 
*
*     Currently a global lock is just a 'pthread_mutex_t'. This does imply
*     that the lock mode has no effect.
*
*******************************************************************************/
static void lock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "lock_callback");

   sge_mutex_lock(sge_type_name(aType), SGE_FUNC, __LINE__, &Global_Lock);

   DEXIT;
   return;
} /* lock_callback() */

/****** qmaster/sge_qmaster_main/unlock_callback() *****************************
*  NAME
*     unlock_callback() -- unlock callback 
*
*  SYNOPSIS
*     static void unlock_callback(sge_locktype_t aType, sge_lockmode_t aMode, 
*     sge_locker_t anID) 
*
*  FUNCTION
*     Release global lock 'aType' which has been acquired in mode 'aMode'
*     previously. 
*
*  INPUTS
*     sge_locktype_t aType - lock type 
*     sge_lockmode_t aMode - lock mode 
*     sge_locker_t anID    - locker id 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: unlock_callback() is MT safe. 
*
*     Currently a global lock is just a 'pthread_mutex_t'. This does imply
*     that the lock mode has no effect.
*
*******************************************************************************/
static void unlock_callback(sge_locktype_t aType, sge_lockmode_t aMode, sge_locker_t anID)
{
   DENTER(TOP_LAYER, "unlock_callback");

   sge_mutex_unlock(sge_type_name(aType), SGE_FUNC, __LINE__, &Global_Lock);

   DEXIT;
   return;
} /* unlock_callback() */

/****** qmaster/sge_qmaster_main/id_callback() *********************************
*  NAME
*     id_callback() -- locker ID callback 
*
*  SYNOPSIS
*     static sge_locker_t id_callback(void) 
*
*  FUNCTION
*     Return ID of current locker. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     sge_locker_t - locker id
*
*  NOTES
*     MT-NOTE: id_callback() is MT safe. 
*
*******************************************************************************/
static sge_locker_t id_callback(void)
{
   sge_locker_t id;

   DENTER(TOP_LAYER, "id_callback");
   
   id = (sge_locker_t)pthread_self();

   DEXIT;
   return id;
} /* id_callback */

/****** qmaster/sge_qmaster_main/exit_func() **********************************
*  NAME
*     exit_func() -- qmaster exit function
*
*  SYNOPSIS
*     static void exit_func(int anExitValue) 
*
*  FUNCTION
*     qmaster exit function. This function should be used BEFORE qmaster
*     did change its working directory to be the spool directory. This
*     exit function does NOT lock the qmaster lock file.
*
*  INPUTS
*     int anExitValue - exit value 
*
*  RESULT
*     void - none 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: exit_func() is MT safe.
*
*******************************************************************************/
static void exit_func(int anExitValue)
{
   DENTER(TOP_LAYER, "exit_func");

   sge_gdi_shutdown();

   DEXIT;
   return;
} /* exit_func */

/****** qmaster/sge_qmaster_main/inc_thread_count() ****************************
*  NAME
*     inc_thread_count() -- increment thread count 
*
*  SYNOPSIS
*     static void inc_thread_count(void) 
*
*  FUNCTION
*     Increment number of active threads. The so called 'main thread' is NOT
*     counted. The unmber of active threads is used to coordinate the shutdown
*     procedure.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: inc_thread_count() is MT safe. 
*
*     This function is part of a triumvirate which does consist of
*     'inc_thread_count()', 'wait_for_thread_termination()'.
*     and 'should_terminate()'. These three functions do coordinate the thread
*     termination procedure.
*
*******************************************************************************/
static void inc_thread_count(void)
{
   DENTER(TOP_LAYER, "inc_thread_count");

   sge_mutex_lock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);
   Qmaster_Control.thrd_count++;
   sge_mutex_unlock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   DEXIT;
   return;
} /* inc_thread_count() */

/****** qmaster/sge_qmaster_main/should_terminate() ****************************
*  NAME
*     should_terminate() -- should thread terminate? 
*
*  SYNOPSIS
*     static bool should_terminate(void) 
*
*  FUNCTION
*     Determine if thread should terminate. Check the 'shutdown' flag of the
*     qmaster control structure. If the flag is 'true', decrement thread
*     count and signal the qmaster control structure condition variable.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     false - continue
*     true  - terminate
*
*  NOTES
*     MT-NOTE: should_terminate() is MT safe. 
*
*     This function is part of a triumvirate which does consist of
*     'inc_thread_count()', 'wait_for_thread_termination()'.
*     and 'should_terminate()'. These three functions do coordinate the thread
*     termination procedure.
*
*******************************************************************************/
static bool should_terminate(void)
{
   bool res = false;

   DENTER(TOP_LAYER, "should_terminate");

   sge_mutex_lock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   if (Qmaster_Control.shutdown == true) {
      Qmaster_Control.thrd_count--;
      pthread_cond_signal(&Qmaster_Control.cond_var);
      res = true;
   }

   sge_mutex_unlock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   DEXIT;
   return res;
} /* should_terminate() */

/****** qmaster/sge_qmaster_main/signal_thread() *******************************
*  NAME
*     signal_thread() -- signal thread function
*
*  SYNOPSIS
*     static void* signal_thread(void* anArg) 
*
*  FUNCTION
*     Signal handling thread function. Establish recognized signal set. Enter
*     signal wait loop. Wait for signal. Handle signal.
*
*     If signal is 'SIGINT' or 'SIGTERM', kick-off shutdown and invalidate
*     signal thread.
*
*     NOTE: The signal thread will terminate on return of this function.
*
*  INPUTS
*     void* anArg - not used 
*
*  RESULT
*     void* - none 
*
*  NOTES
*     MT-NOTE: signal_thread() is a thread function. Do NOT use this function
*     MT-NOTE: in any other way!
*
*******************************************************************************/
static void* signal_thread(void* anArg)
{
   enum { true = 1 };

   sigset_t sig_set;
   int sig_num;

   DENTER(TOP_LAYER, "signal_thread");

   sge_qmaster_thread_init();

   sigemptyset(&sig_set);
   sigaddset(&sig_set, SIGINT);
   sigaddset(&sig_set, SIGTERM);

   while (true)
   {
      sigwait(&sig_set, &sig_num);

      DPRINTF(("%s: got signal %d\n", SGE_FUNC, sig_num));

      switch (sig_num) {
         case SIGINT:
         case SIGTERM:
            wait_for_thread_termination();
            set_signal_thread(INVALID_THREAD);
            DEXIT;
            return NULL;
         default:
            ERROR((SGE_EVENT, MSG_QMASTER_UNEXPECTED_SIGNAL_I, sig_num));
      }
   }

   DEXIT;
   return NULL;
} /* signal_thread() */

/****** qmaster/sge_qmaster_main/message_thread() ******************************
*  NAME
*     message_thread() -- message thread function 
*
*  SYNOPSIS
*     static void* message_thread(void* anArg) 
*
*  FUNCTION
*     Process messages. 
*
*  INPUTS
*     void* anArg - not used 
*
*  RESULT
*     void* - none 
*
*  NOTES
*     MT-NOTE: message_thread() is a thread function. Do NOT use this function
*     MT-NOTE: in any other way!
*
*******************************************************************************/
static void* message_thread(void* anArg)
{
   DENTER(TOP_LAYER, "message_thread");

   sge_qmaster_thread_init();

   while (should_terminate() == false)
   {
      sge_qmaster_process_message(anArg);
   }

   DEXIT;
   return NULL;
} /* message_thread() */

/****** qmaster/sge_qmaster_main/wait_for_thread_termination() *****************
*  NAME
*     wait_for_thread_termination() -- wait for threads 
*
*  SYNOPSIS
*     static void wait_for_thread_termination(void) 
*
*  FUNCTION
*     Wait for thread termination. Set 'shutdown'flag to true and wait until all
*     other threads (except the main thread) did terminate themselves. At this
*     point in time only two threads do remain - the main thread and the thread
*     which did invoke 'wait_for_thread_termination()'.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void -  none
*
*  NOTES
*     MT-NOTE: Although wait_for_thread_termination() is MT safe, this function
*     MT-NOTE: will NOT work properly if multiple threas do invoke it.
*
*     This function is part of a triumvirate which does consist of
*     'inc_thread_count()', 'wait_for_thread_termination()'.
*     and 'should_terminate()'. These three functions do coordinate the thread
*     termination procedure.
*
*  BUGS
*     Currently the event master (evm) does need a working gdi thread to 
*     successfully deliver events. For this reason we need to add the
*     'sgeE_QMASTER_GOES_DOWN' event *before* the message thread does
*     terminate itself.
*
*******************************************************************************/
static void wait_for_thread_termination(void)
{
   DENTER(TOP_LAYER, "wait_for_thread_termination");

   sge_mutex_lock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   Qmaster_Control.shutdown = true;

   sge_add_event(0, sgeE_QMASTER_GOES_DOWN, 0, 0, NULL, NULL, NULL, NULL);

   while (Qmaster_Control.thrd_count > 1) {
      pthread_cond_wait(&Qmaster_Control.cond_var, &Qmaster_Control.mutex);
   }

   sge_mutex_unlock("qmaster_mutex", SGE_FUNC, __LINE__, &Qmaster_Control.mutex);

   DPRINTF(("%s: all threads but one did terminate\n", SGE_FUNC));

   DEXIT;
   return;
} /* wait_for_thread_termination() */

/****** qmaster/sge_qmaster_main/qmaster_shutdown() ****************************
*  NAME
*     qmaster_shutdown() -- shutdown qmaster 
*
*  SYNOPSIS
*     static void qmaster_shutdown(void) 
*
*  FUNCTION
*     Shutdown qmaster. Shutdown persistence and reporting service. Shutdown
*     timed event delivery. Shutdown event delivery. Clean-up communication
*     library.
*
*     This function must be the VERY last function which qmaster does invoke.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: qmaster_shutdown() is NOT MT safe. 
*
*     Do NOT change the shutdown operation sequence!
*
*******************************************************************************/
static void qmaster_shutdown(void)
{
   DENTER(TOP_LAYER, "qmaster_shutdown");

   sge_shutdown_persistence(NULL);

   reporting_shutdown(NULL);

   te_shutdown();

   sge_event_shutdown();

   sge_shutdown();

   DEXIT;
   return;
} /* qmaster_shutdown() */

