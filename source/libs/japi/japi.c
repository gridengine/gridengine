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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <pwd.h>

#include "sge_mtutil.h"
#include "drmaa.h"
#include "japi.h"
#include "msg_japi.h"
#include "sge.h"
#include "sge_answer.h"

/* CULL */
#include "cull_list.h"

/* self */
#include "japiP.h"

/* RMON */
#include "sgermon.h"

/* UTI */
#include "sge_prog.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_signal.h"
#include "sge_uidgid.h"
#include "sge_unistd.h"
#include "sge_string.h"
#include "sge_bootstrap.h"
#include "uti/sge_hostname.h"

/* COMMLIB */
#include "commlib.h"


/* EVC */
#include "sge_event_client.h"

/* EVM */
#include "sge_event_master.h"

/* GDI */
#include "sge_conf.h"
#include "gdi_conf.h"
#include "sge_gdi.h"
#include "gdi_tsm.h"
#include "sge_gdi_request.h"
#include "sge_gdiP.h"
#include "sge_security.h"

/* SGEOBJ */
#include "sge_cqueue.h"
#include "sge_event.h"
#include "sge_feature.h"
#include "sge_gqueue.h"
#include "sge_id.h"
#include "sge_job.h"
#include "sge_object.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_range.h"
#include "msg_common.h"
#include "gdi/sge_any_request.h"

/* OBJ */
#include "sge_jobL.h"
#include "sge_ja_taskL.h"
#include "sge_japiL.h"
#include "sge_idL.h"
#include "sge_strL.h"
#include "sge_answerL.h"
#include "sge_reportL.h"
#include "sge_usageL.h"


/****** JAPI/--Job_API ********************************************************
*  NAME
*     Job_JAPI -- Grid Engine's API for job submission and control.
*
*  FUNCTION
*
*  NOTES
*
*  SEE ALSO
*     JAPI/-JAPI_Session_state
*     JAPI/-JAPI_Implementation
*     JAPI/-JAPI_Interface 
*******************************************************************************/

static pthread_once_t japi_once_control = PTHREAD_ONCE_INIT;


/****** JAPI/-JAPI_Session_state *******************************************
*  NAME
*     JAPI_Session_state -- All global variables together constitute the state of a JAPI session 
*
*  SYNOPSIS
*     static pthread_t japi_event_client_thread;
*     static int japi_ec_return_value;
*     static int japi_session = JAPI_SESSION_INACTIVE;
*     static int japi_ec_state = JAPI_EC_DOWN;
*     static u_long32 japi_ec_id = 0;
*     static lList *Master_japi_job_list = NULL;
*     static int japi_threads_in_session = 0;
*     static char *japi_session_key = NULL;
*     static bool japi_delegated_file_staging_is_enabled = false;
*     
*  FUNCTION
*     japi_event_client_thread - the event client thread. Used by japi_init() and
*                    japi_exit() to control start and shutdown of this implementation
*                    thread.
*     japi_ec_return_value - return value of the event client thread
*     japi_session - reflects state of a JAPI session 
*                    state is set to JAPI_SESSION_ACTIVE when japi_init() succeeded
*                    and set to JAPI_SESSION_INACTIVE by japi_exit() 
*                    Code using japi_session must be made reentrant with 
*                    the mutex japi_session_mutex.
*     japi_ec_state - is used for synchronizing with startup of the event 
*                    client thread in japi_init() and for synchronizing
*                    with event client thread in japi_exit(). Also it is used
*                    to ensure blocking functions that depend upon event client
*                    functionality finish when the event client thread finishes
*                    as a result of a japi_exit() called by another thread.
*                    Code using japi_ec_state must be made reentrant with 
*                    japi_ec_state_mutex. To communicate state transistions 
*                    the condition variable japi_ec_state_starting_cv is used.
*     japi_ec_id - contains event client id written by event client thread
*                    read by thread doing japi_exit() to unregister event client
*                    from qmaster. 
*     Master_japi_job_list - The Master_japi_job_list contains information 
*                    about all jobs' state of this session. It is used to
*                    allow japi_wait() and japi_synchronize() for waiting for 
*                    jobs to finish. New jobs are added into this data structure 
*                    by japi_run_job() and japi_run_bulk_jobs(), job finish 
*                    information is stored by the event client thread. Jobs are 
*                    removed by japi_wait() and japi_synchronize() each time when
*                    a job is reaped. Code depending upon Master_japi_job_list
*                    must be made reentrant using mutex Master_japi_job_list_mutex.
*                    To implement synchronuous wait for job finish information 
*                    being added condition variable Master_japi_job_list_finished_cv 
*                    is used. See japi_threads_in_session on strategy to ensure 
*                    Master_japi_job_list integrity in case of multiple application 
*                    threads.
*     japi_threads_in_session - A counter indicating the number of threads depending 
*                    upon Master_japi_job_list: Each thread entering such a JAPI call 
*                    must increase this counter and decrese it again when leaving. 
*                    Code using japi_threads_in_session must be made reentrant using
*                    the mutex japi_threads_in_session_mutex. When decresing the 
*                    counter to 0 the condition variable japi_threads_in_session_cv
*                    is used to notify japi_exit() that Master_japi_job_list can be
*                    released.
*     japi_session_key - is a string key used during event client registration 
*                    to select only those job events that are related to the JAPI 
*                    session. Code using japi_session_key must be made reentant
*                    with mutex japi_session_mutex. It is assumed the session key 
*                    is not changed during an active session.
*     japi_delegated_file_staging_is_enabled - An int indicating if delegated file
*                    staging is enabled in the cluster configuration.
*                    should always be accessed via
*                    japi_is_delegated_file_staging_enabled() which protects the
*                    variable with a mutex.
*
*                    
*  NOTES
*
*  SEE ALSO
*******************************************************************************/

static pthread_t japi_event_client_thread;

/* ---- japi_ec_return_value ------------------------------ */
/* static int japi_ec_return_value; */
static lList *japi_ec_alp;

/* ---- japi_session --------------------------------- */

enum { 
   JAPI_SESSION_ACTIVE,
   JAPI_SESSION_INITIALIZING,
   JAPI_SESSION_SHUTTING_DOWN,
   JAPI_SESSION_INACTIVE
};
static int japi_session = JAPI_SESSION_INACTIVE;
/* guards access to japi_session global variable */
static pthread_mutex_t japi_session_mutex = PTHREAD_MUTEX_INITIALIZER;

#define JAPI_LOCK_SESSION()      sge_mutex_lock("SESSION", SGE_FUNC, __LINE__, &japi_session_mutex)                                 
#define JAPI_UNLOCK_SESSION()    sge_mutex_unlock("SESSION", SGE_FUNC, __LINE__, &japi_session_mutex)
                                 
/* ---- japi_cr ------------------------------------------- */
static int cr_count = 0;
/* guards access to critical regions of code */
static pthread_mutex_t japi_cr_mutex = PTHREAD_MUTEX_INITIALIZER;
/* needed to let japi_exit know when it's ok to exit */
static pthread_cond_t japi_cr_cv = PTHREAD_COND_INITIALIZER;

#define JAPI_ENTER_CR()    sge_mutex_lock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex); \
                           cr_count++; \
                           sge_mutex_unlock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex)                                 
#define JAPI_EXIT_CR()     sge_mutex_lock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex); \
                           cr_count--; \
                           if (!cr_count) pthread_cond_signal (&japi_cr_cv); \
                           sge_mutex_unlock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex)                                 
#define JAPI_LOCK_CR()     sge_mutex_lock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex); \
                           while (cr_count) pthread_cond_wait (&japi_cr_cv, &japi_cr_mutex)                                 
#define JAPI_UNLOCK_CR()   sge_mutex_unlock("CR", SGE_FUNC, __LINE__, &japi_cr_mutex)
/* ---- japi_ec_state ------------------------------------- */

enum { 
   JAPI_EC_DOWN,
   JAPI_EC_UP,
   JAPI_EC_STARTING,
   JAPI_EC_FINISHING,
   JAPI_EC_FAILED
};

static int japi_ec_state = JAPI_EC_DOWN;

/* guards access to japi_ec_state global variable */
static pthread_mutex_t japi_ec_state_mutex = PTHREAD_MUTEX_INITIALIZER;

#define JAPI_LOCK_EC_STATE()      sge_mutex_lock("japi_ec_state_mutex", SGE_FUNC, __LINE__, &japi_ec_state_mutex)                                 
#define JAPI_UNLOCK_EC_STATE()    sge_mutex_unlock("japi_ec_state_mutex", SGE_FUNC, __LINE__, &japi_ec_state_mutex)                                

/* needed in japi_init() to allow waiting for event 
   client thread being up and running */
static pthread_cond_t japi_ec_state_starting_cv = PTHREAD_COND_INITIALIZER;

/* ---- japi_ec_id ------------------------------------------ */
static u_long32 japi_ec_id = 0;

/* ---- Master_japi_job_list -------------------------------- */
static lList *Master_japi_job_list = NULL;

/* guards access to Master_japi_job_list global variable */
static pthread_mutex_t Master_japi_job_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define JAPI_LOCK_JOB_LIST()     sge_mutex_lock("Master_japi_job_list_mutex", SGE_FUNC, __LINE__, &Master_japi_job_list_mutex)
#define JAPI_UNLOCK_JOB_LIST()   sge_mutex_unlock("Master_japi_job_list_mutex", SGE_FUNC, __LINE__, &Master_japi_job_list_mutex)

/* this condition is raised each time when a job/task is finshed */
static pthread_cond_t Master_japi_job_list_finished_cv = PTHREAD_COND_INITIALIZER;

/* ---- japi_threads_in_session ------------------------------ */

int japi_threads_in_session = 0;

/* guards access to threads_in_session global variable */
static pthread_mutex_t japi_threads_in_session_mutex = PTHREAD_MUTEX_INITIALIZER;

#define JAPI_LOCK_REFCOUNTER()   sge_mutex_lock("japi_threads_in_session_mutex", SGE_FUNC, __LINE__, &japi_threads_in_session_mutex)
#define JAPI_UNLOCK_REFCOUNTER() sge_mutex_unlock("japi_threads_in_session_mutex", SGE_FUNC, __LINE__, &japi_threads_in_session_mutex)

/* this condition is raised when a threads_in_session becomes 0 */
static pthread_cond_t japi_threads_in_session_cv = PTHREAD_COND_INITIALIZER;

/* ---- globals ------------------------------------- */
char *japi_session_key = NULL;
static const char *JAPI_SINGLE_SESSION_KEY = "JAPI_SSK";
static int prog_number = JAPI;
static bool multi_threaded = false;
static error_handler_t error_handler = NULL;
static int japi_delegated_file_staging_is_enabled = -1;
/* This variable is only used by japi_init() and hence does not need to be
 * protected by a mutex. */
static bool virgin_session = true;

#define MAX_JOBS_TO_DELETE 500

/****** JAPI/-JAPI_Implementation *******************************************
*  NAME
*     JAPI_Implementation -- Functions used to implement JAPI
* 
*  SEE ALSO
*     JAPI/japi_open_session()
*     JAPI/japi_close_session()
*     JAPI/japi_implementation_thread()
*     JAPI/japi_parse_jobid()
*     JAPI/japi_send_job()
*     JAPI/japi_add_job()
*     JAPI/japi_synchronize_retry()
*     JAPI/japi_synchronize_all_retry()
*     JAPI/japi_synchronize_jobids_retry()
*     JAPI/japi_wait_retry()
*     JAPI/japi_synchronize_retry()
*******************************************************************************/

static int japi_open_session(const char *key_in, dstring *key_out, dstring *diag);
static int japi_close_session(const dstring *key, dstring *diag);

static void *japi_implementation_thread(void *);
static int japi_parse_jobid(const char *jobid_str, u_long32 *jobid, u_long32 *taskid, 
   bool *is_array, dstring *diag);
static int japi_send_job(lListElem *job, u_long32 *jobid, dstring *diag);
static int japi_add_job(u_long32 jobid, u_long32 start, u_long32 end, u_long32 incr, 
      bool is_array, dstring *diag);
static int japi_synchronize_retry(bool sync_all, const char *job_ids[], bool dispose);
static int japi_synchronize_all_retry(bool dispose);
static int japi_synchronize_jobids_retry(const char *jobids[], bool dispose);
static int japi_wait_retry(lList *japi_job_list, int wait4any, u_long32 jobid,
                           u_long32 taskid, bool is_array_task, int event_mask,
                           u_long32 *wjobidp, u_long32 *wtaskidp,
                           bool *wis_task_arrayp, int *wait_status,
                           int *wevent, lList **rusagep);
static int japi_gdi_control_error2japi_error(lListElem *aep, dstring *diag, int drmaa_control_action);
static int japi_clean_up_jobs (int flag, dstring *diag);
static int japi_read_dynamic_attributes(dstring *diag);
static int do_gdi_delete (lList **id_list, int action, bool delete_all,
                          dstring *diag);
static int japi_stop_event_client (void);


static void japi_use_library_signals(void)
{
   /* simply ignore SIGPIPE */
   signal (SIGPIPE, SIG_IGN);
}


static void japi_once_init(void)
{
   /* enable rmon monitoring */
   rmon_mopen(NULL, 0, "japilib");
   feature_mt_init();
}


static void japi_inc_threads(const char *func)
{
   DENTER(TOP_LAYER, "japi_inc_threads");
   JAPI_LOCK_REFCOUNTER();
   japi_threads_in_session++;
   DPRINTF(("%s(): japi_threads_in_session++ %d\n", func, japi_threads_in_session));
   JAPI_UNLOCK_REFCOUNTER();
   DEXIT;
}

static void japi_dec_threads(const char *func)
{
   DENTER(TOP_LAYER, "japi_dec_threads");
   JAPI_LOCK_REFCOUNTER();
   if (--japi_threads_in_session == 0)
      pthread_cond_signal(&japi_threads_in_session_cv);
   DPRINTF(("%s(): japi_threads_in_session-- %d\n", func, japi_threads_in_session));
   JAPI_UNLOCK_REFCOUNTER();
   DEXIT;
}



/****** JAPI/japi_init_mt() ****************************************************
*  NAME
*     japi_init_mt() -- Per thread library initialization
*
*  SYNOPSIS
*     int japi_init_mt(dstring *diag) 
*
*  FUNCTION
*     Do all per thread initialization required for libraries JAPI builds 
*     upon.
*
*  OUTPUT
*     dstring *diag - returns diagnosis information - on error
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTES: japi_init_mt() is MT safe
*******************************************************************************/
int japi_init_mt(dstring *diag)
{
   lList *alp = NULL;
   int gdi_errno;
   
   bootstrap_mt_init();
   feature_mt_init();

   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if ( uti_state_get_mewho() == QUSERDEFINED) { 
      sge_gdi_param(SET_MEWHO, prog_number, NULL);
   } 

   /* current major assumptions are
      - code is not compiled with -DCRYPTO
      - code is not compiled with -DKERBEROS
      - neither AFS nor DCE/KERBEROS security may be used */

   /* as long as signal handling is not restored japi_init_mt() is
      good place to install library signal handling */
   japi_use_library_signals();

   gdi_errno = sge_gdi_setup(prognames[prog_number], &alp);
   
   if ((gdi_errno != AE_OK) && (gdi_errno != AE_ALREADY_SETUP)) {
      answer_to_dstring(lFirst(alp), diag);
      lFreeList(alp);
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_init() ****************************************************
*  NAME
*     japi_init() -- Initialize JAPI library
*
*  SYNOPSIS
*     int japi_init(const char *contact, const char *session_key_in, 
*           dstring *session_key_out, dstring *diag)
*
*  FUNCTION
*     Initialize JAPI library and create a new JAPI session. This 
*     routine must be called before any other JAPI calls, except for 
*     japi_version(). Initializes internal data structures.  Also registers 
*     with qmaster using the event client mechanism if the enable_wait parameter
*     is set to true.  If enable_wait is set to false, japi_enable_job_wait()
*     must be called before calling japi_wait() or japi_synchronize().
*     If enable_wait is set to true, a second thread is spawned as an event client,
*     which imposes threading and synchronization overhead.  If japi_wait() and
*     japi_synchronize() are not needed, JAPI can be made much lighter weight
*     by setting enable_wait to false.
*
*  INPUTS
*     const char *contact        - 'Contact' is an implementation dependent
*                                  string which may be used to specify which DRM
*                                  system to use. If 'contact' is NULL, the
*                                  default DRM system will be used.
*     const char *session_key_in - if non NULL japi_init() tries to restart
*                                  a former session using this session key.
*     int my_prog_num            - the index into prognames to use when
*                                  registering with the qmaster.  See
*                                  sge_gdi_setup().
*     bool enable_wait           - Whether to start up in mutli-threaded mode to
*                                  allow japi_wait() and japi_synchronize() to
*                                  function.
*                                  When true, a new session is created (if
*                                  needed), and the event client thread is
*                                  started.  When false, no session string
*                                  is set, and the event client is not started.
*                                  When false, japi_synchronize() and japi_wait()
*                                  will return DRMAA_ERRNO_NO_ACTIVE_SESSION.
*                                  If enable_wait is set to false, job waiting
*                                  can be explicitly enabled later by calling
*                                  the japi_enable_job_wait() function.
*     error_handler_t handler    - A callback to be used for error messages from
*                                  the event client thread.  When enable_wait is
*                                  false, handler should be set to NULL.  The
*                                  callback should not free the error message
*                                  after processing it.
*
*  OUTPUT
*     dstring *session_key_out   - Returns session key of new session - on success.
*     dstring *diag              - Returns diagnosis information - on failure
*
*  RESULT
*     int - DRMAA error codes
* 
*  MUTEXES
*      japi_session_mutex
*
*  NOTES
*      MT-NOTE: japi_init() is MT safe
*******************************************************************************/
int japi_init(const char *contact, const char *session_key_in, 
              dstring *session_key_out, int my_prog_num, bool enable_wait,
              error_handler_t handler, dstring *diag)
{
   int ret;
   cl_com_handle_t* handle = NULL;
  
   DENTER(TOP_LAYER, "japi_init");

   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_INACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_ALREADY_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_ALREADY_ACTIVE_SESSION;
   }
   
   japi_session = JAPI_SESSION_INITIALIZING;
   JAPI_UNLOCK_SESSION();

   pthread_once(&japi_once_control, japi_once_init);

   if (my_prog_num > 0) {
      prog_number = my_prog_num;
   }

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_session = JAPI_SESSION_INACTIVE;
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* Bugfix: Issuezilla 1025
    * The problem is that the commlib handle was being created in japi_mt_init()
    * even when there was no actual need of a communications channel.  The
    * reason the handle is created at all is that if it is not, calling
    * japi_init() followed by japi_exit() followed by japi_init() again would
    * result in functions like japi_run_job() getting a dead handle.  Once the
    * handle is closed, it has to be explicitly reopened.  (Because this is
    * really only an init issue, it's safe to move this code in japi_init().)
    * The answer is to not create the handle the first time japi_init() is
    * called.  Since the handle hasn't been closed yet, it doesn't need to be
    * explicitly created.  If japi_init() gets called more than once, it's fair
    * to assume that later calls will be doing something more than just
    * initializing to prep for outputing usage information.  At least, that's
    * how it looks right now. */
   /* Besides, it looks like creating the handle wasn't the real problem.  The
    * real problem was the call to read_dynamic_attributes() from japi_init().
    * This bug fix is still a good idea, though. */
   /* No need to worry about locking for this global since it is only used in
    * japi_init(), and only one thread may be in japi_init() at a time. */
   if (!virgin_session) {
      int commlib_error = CL_RETVAL_OK;

      /* Make sure the commlib handle exists  If it doesn't, create it. */
      handle = cl_com_get_handle ((char*)uti_state_get_sge_formal_prog_name(), 0);

      if (handle == NULL) {
         handle = cl_com_create_handle(&commlib_error, CL_CT_TCP, CL_CM_CT_MESSAGE, 0,
                                       sge_get_qmaster_port(),
                                       (char*)prognames[uti_state_get_mewho()],
                                       0, 1, 0);      
      }

      if (handle == NULL) {
         JAPI_UNLOCK_SESSION();
         sge_dstring_sprintf (diag, MSG_JAPI_NO_HANDLE_S, cl_get_error_text(commlib_error));
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }
   }
   else {
      virgin_session = false;
   }
      
   if (enable_wait) {
      /* spawn implementation thread japi_implementation_thread() */
      ret = japi_enable_job_wait (session_key_in, session_key_out, handler,
                                  diag);
   }
   else {
      japi_session_key = (char *)JAPI_SINGLE_SESSION_KEY;
      ret = DRMAA_ERRNO_SUCCESS;
   }

   multi_threaded = enable_wait;

   JAPI_LOCK_SESSION();
   if (ret == DRMAA_ERRNO_SUCCESS) {
      japi_session = JAPI_SESSION_ACTIVE;
   }
   else {
      japi_session = JAPI_SESSION_INACTIVE;
   }
   JAPI_UNLOCK_SESSION();
  
   DEXIT;
   return ret;
}

/****** JAPI/japi_enable_job_wait() ********************************************
*  NAME
*     japi_enable_job_wait() -- Do setup required for doing job waits
*
*  SYNOPSIS
*     int japi_enable_job_wait(const char *session_key_in,
*                              string *session_key_out, dstring *diag)
*
*  FUNCTION
*     Does all of the required setup to be able to use the japi_wait() and
*     japi_synchronize() calls.  This includes starting up the event client
*     thread and establishing a session.
*     If japi_init() was called with enable_wait set to false, this method must
*     be called before japi_wait() or japi_synchronize() can be used.
*     This is useful if, for example, when one doesn't know for sure whether
*     japi_wait() will be needed at the time japi_init() is called.  The
*     overhead associated with starting and stopping the event client thread and
*     creating and destroying a session can thereby be avoided.
*
*  INPUT
*     const char *session_key_in - if non NULL japi_enable_job_wait() tries to restart
*                                  a former session using this session key.
*     error_handler_t handler    - A callback to be used for error messages from
*                                  the event client thread.  When NULL, no error
*                                  messages will be generated by the event
*                                  client thread.  The callback should not free
*                                  the error message after processing it.
*
*  OUTPUT
*     dstring *session_key_out   - Returns session key of new session - on success.
*     dstring *diag              - Returns diagnosis information - on failure
*
*  RESULT
*     int - DRMAA error codes
* 
*  MUTEXES
*     japi_session_mutex -> japi_ec_state_mutex
*
*  NOTES
*      MT-NOTE: japi_enable_job_wait() is MT safe
*******************************************************************************/
int japi_enable_job_wait(const char *session_key_in, dstring *session_key_out,
                         error_handler_t handler, dstring *diag)
{
   int i;
   int ret;
   pthread_attr_t attr;
   sigset_t mask, omask;
   
   DENTER(TOP_LAYER, "japi_enable_job_wait");

   JAPI_LOCK_SESSION();
   /* JAPI_SESSION_INITIALIZING if we're called from japi_init() or
    * JAPI_SESSION_ACTIVE if we're called from the client code directly. */
   /* DT: TODO: There's a potential synchronization bug here.  Calling
    * japi_enable_job_wait() at the same time as japi_init() will cause two
    * event client threads to be started. */
   if (!((japi_session == JAPI_SESSION_INITIALIZING) ||
         (japi_session == JAPI_SESSION_ACTIVE))) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   JAPI_LOCK_EC_STATE();
   if (japi_ec_state != JAPI_EC_DOWN) {
      JAPI_UNLOCK_EC_STATE();
      JAPI_UNLOCK_SESSION();
      sge_dstring_copy_string(diag, MSG_JAPI_EVENT_CLIENT_ALREADY_STARTED);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   JAPI_UNLOCK_EC_STATE();
   JAPI_UNLOCK_SESSION();
   
   /* (re)open JAPI session associated with JAPI session key */
   if ((ret = japi_open_session(session_key_in, session_key_out, diag))!=DRMAA_ERRNO_SUCCESS) {
      /* diag was set by japi_open_session() */
      DEXIT;
      return ret;
   }

   JAPI_LOCK_SESSION();
   if (japi_session_key == JAPI_SINGLE_SESSION_KEY) {
      /* japi_init() was called with enable_wait set to false */
      japi_session_key = strdup(sge_dstring_get_string(session_key_out));
   }
   else {
      /* japi_init() was called with enable_wait set to true */
      japi_session_key = sge_strdup(japi_session_key, sge_dstring_get_string(session_key_out));
   }
   JAPI_UNLOCK_SESSION();

   /* Set handler for dealing with error messages from event client thread. */
   error_handler = handler;
   
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   
   /* Block signals for child thread and thread spawing process */
   sigemptyset (&mask);
   sigaddset (&mask, SIGHUP);
   sigaddset (&mask, SIGINT);
   sigaddset (&mask, SIGQUIT);
   sigaddset (&mask, SIGKILL);
   sigaddset (&mask, SIGUSR1);
   sigaddset (&mask, SIGUSR2);
   pthread_sigmask(SIG_SETMASK, &mask, &omask);

   /* I'm locking the EC_STATE here so that there is no race condition with the
    * event client thread. */
   JAPI_LOCK_EC_STATE();
   
   if (japi_ec_state == JAPI_EC_DOWN) {
      DPRINTF(("Waiting for event client to start up\n"));
      
      if ((i=pthread_create(&japi_event_client_thread, &attr, japi_implementation_thread, (void *)&japi_ec_alp))) {
         if (diag) 
            sge_dstring_sprintf(diag, "error: couldn't create event client thread: %d %s\n", i, strerror(errno));
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }

      /* Restore signals */
      pthread_sigmask(SIG_SETMASK, &omask, NULL);

      /* wait until event client thread is operable or gives up */      
      japi_ec_state = JAPI_EC_STARTING;
      
      /* We wait for !JAPI_EC_STARTING here instead of JAPI_EC_UP because the
       * event client thread may not succeed in starting up. */
      while (japi_ec_state == JAPI_EC_STARTING) {
         pthread_cond_wait(&japi_ec_state_starting_cv, &japi_ec_state_mutex);
      }
      
      if (japi_ec_state == JAPI_EC_UP) {
         JAPI_UNLOCK_EC_STATE();
         DPRINTF(("Event client has been started\n"));
         ret = DRMAA_ERRNO_SUCCESS;
      }
      else if (japi_ec_state == JAPI_EC_FAILED) {
         lListElem *aep = NULL;

         JAPI_UNLOCK_EC_STATE();
         ret = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
         aep = lFirst(japi_ec_alp);
         if (pthread_join(japi_event_client_thread, NULL)) {
            DPRINTF(("japi_init(): pthread_join returned\n"));
         }

         sge_dstring_free(session_key_out);
         FREE(japi_session_key);

         /* return error context from event client thread if there is such */
         if (aep) {
            answer_to_dstring(aep, diag);
         }
         else {
            japi_standard_error(DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, diag);
         }
      }
      else {
         JAPI_UNLOCK_EC_STATE();
         /* else japi_ec_state == JAPI_EC_DOWN which means the thread was shut
          * down by japi_exit() before it could register as an event client.  In
          * this case, we just quietly exit as though everything worked, which
          * techincally it did.  We just triggered a shortcut that prevents the
          * event client thread from starting up completely just to be shut
          * down. */
         ret = DRMAA_ERRNO_SUCCESS;
      }
   }
   else {
      JAPI_UNLOCK_EC_STATE();
      DPRINTF(("Event client has already been started\n"));
      /* If the state is not JAPI_EC_DOWN, return
       * DRMAA_ERRNO_ALREADY_ACTIVE_SESSION because we don't have a better
       * error code to return.  We really need to give JAPI it's own error
       * codes instead of leaning on DRMAA. */
      /* This also applies to finishing because the event client must already
       * be running to be stopping.  Ideally we would return a more specific
       * error code, but for the moment, this is the best I can do. */
      ret = DRMAA_ERRNO_ALREADY_ACTIVE_SESSION;
   }
   
   pthread_attr_destroy(&attr);
   
   DEXIT;
   return ret;
}


/****** JAPI/japi_open_session() ***********************************************
*  NAME
*     japi_open_session() -- create or reopen JAPI session
*
*  SYNOPSIS
*     static int japi_open_session(const char *key_in, dstring *key_out, 
*                dstring *diag)
*
*  FUNCTION
*     A JAPI session is created or reopend, depending on the value of key_in.
*     The session key of the opend session is returned.
*
*  INPUTS
*     const char *key_in - If 'key' is non NULL it is used to reopen 
*        the JAPI session. Otherwise a new session is always created.
*
*  OUTPUT
*     dstring *key_out   - Returns session key of the session that was opened
*                          on success.
*     dstring *diag      - Diagnosis information - on failure.
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_open_session() is MT safe 
*******************************************************************************/
static int japi_open_session(const char *key_in, dstring *key_out, dstring *diag)
{
   struct passwd pw_struct, *pwd;
   char buffer[2048];
   
   char tmp_session_path_buffer[SGE_PATH_MAX];
   dstring tmp_session_path;

   DENTER(TOP_LAYER, "japi_open_session");

   sge_dstring_init(&tmp_session_path, tmp_session_path_buffer, sizeof(tmp_session_path_buffer));  

   pwd = sge_getpwnam_r(uti_state_get_user_name(), &pw_struct, buffer, sizeof(buffer));
   if (!pwd) {
      if (diag) 
         sge_dstring_sprintf(diag, "error: getpwnam() failed for user \"%s\"\n", uti_state_get_user_name());
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   } 
   if (!pwd->pw_dir) {
      if (diag) 
         sge_dstring_sprintf(diag, MSG_USER_NOHOMEDIRFORUSERX_S, uti_state_get_user_name());
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   if (!key_in) {
      char tmp_session_key_buffer[SGE_PATH_MAX];
      dstring tmp_session_key;
      int ret;
      unsigned int id = 0;

      /* seed random function */
      id = sge_get_gmt();

      sge_dstring_init(&tmp_session_key, tmp_session_key_buffer, sizeof(tmp_session_key_buffer));  

      /* a unique session key must be found if we got no session key */
      do {
         id = rand_r((unsigned int *)&id);

         /* a session key is built from <unqualified hostname>.<pid>.<number> */
         sge_dstring_sprintf(&tmp_session_key, "%s."pid_t_fmt".%.6d", 
                             uti_state_get_unqualified_hostname(), getpid(),
                             id);
         sge_dstring_sprintf(&tmp_session_path, "%s/%s/%s", 
                             pwd->pw_dir, JAPI_SESSION_SUBDIR,
                             sge_dstring_get_string(&tmp_session_key));
         ret = sge_mkdir(sge_dstring_get_string(&tmp_session_path), S_IRWXU, 0, 0);

         if (ret == 0) {
            /* found not yet existing directory */
            break;
         }
      } while (errno == EEXIST);

      if (ret != 0) {
         if (diag) 
            sge_dstring_sprintf(diag, "error: mkdir(\"%s\") failed: %s\n", 
                  sge_dstring_get_string(&tmp_session_path), strerror(errno));
         DEXIT;
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }

      DPRINTF(("created new session using generated \"%s\" as JAPI session key\n", 
               sge_dstring_get_string(&tmp_session_key)));
      sge_dstring_copy_dstring(key_out, &tmp_session_key);

   } else {
      SGE_STRUCT_STAT stat_buf;

      /* we got a session key - reopen existing session */
      sge_dstring_sprintf(&tmp_session_path, "%s/%s/%s", 
               pwd->pw_dir, JAPI_SESSION_SUBDIR, key_in);

      if (SGE_STAT(sge_dstring_get_string(&tmp_session_path), &stat_buf) && errno==ENOENT) {
         /* error */
         if (diag) 
            sge_dstring_sprintf(diag, "error: cannot reopen session \"%s\"\n", key_in);
         DEXIT;
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }
      
      {
         /* restart former session */
         DPRINTF(("reopend existing session using \"%s\" that was passed as JAPI session key\n", 
               key_in));
 
         /* read in library session data of former session */

         sge_dstring_copy_string(key_out, key_in);
      }
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}



/****** JAPI/japi_close_session() **********************************************
*  NAME
*     japi_close_session() -- Close and release all JAPI session resources
*
*  SYNOPSIS
*     static int japi_close_session(const dstring *key, dstring *diag)
*
*  FUNCTION
*     The current JAPI session is closed. All session resources are released.
*        
*  INPUTS
*     const dstring *key  - Session key of the session to be closed.
* 
*  OUTPUT
*     dstring *diag       - diagnosis information on error
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_close_session() is MT safe 
*******************************************************************************/
static int japi_close_session(const dstring *key, dstring *diag)
{
   struct passwd pw_struct, *pwd;
   char buffer[2048];
   char tmp_session_path_buffer[SGE_PATH_MAX];
   dstring tmp_session_path;
   const char *session_key = sge_dstring_get_string(key);

   DENTER(TOP_LAYER, "japi_close_session");
 
   sge_dstring_init(&tmp_session_path, tmp_session_path_buffer, sizeof(tmp_session_path_buffer));  

   pwd = sge_getpwnam_r(uti_state_get_user_name(), &pw_struct, buffer, sizeof(buffer));
   if (!pwd) {
      if (diag) 
         sge_dstring_sprintf(diag, "error: getpwnam() failed for user \"%s\"\n", uti_state_get_user_name());
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   } 
   if (!pwd->pw_dir) {
      if (diag) 
         sge_dstring_sprintf(diag, MSG_USER_NOHOMEDIRFORUSERX_S, uti_state_get_user_name());
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   sge_dstring_sprintf(&tmp_session_path, "%s/%s/%s", pwd->pw_dir, JAPI_SESSION_SUBDIR, 
            session_key);

   /* remove directory */
   if (sge_rmdir(tmp_session_path_buffer, diag)!=0) {
      /* diag must be set by sge_rmdir() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_exit() ****************************************************
*  NAME
*     japi_exit() -- Optionally close JAPI session and shutdown JAPI library.
*
*  SYNOPSIS
*     int japi_exit(bool close_session, dstring *diag)
*
*  FUNCTION
*     Disengage from JAPI library and allow the JAPI library to perform
*     any necessary internal clean up. Depending on 'close_session' this 
*     routine also ends a JAPI Session. japi_exit() has no impact on jobs 
*     (e.g., queued and running jobs remain queued and running).
*
*  INPUTS
*     bool close_session - If true the JAPI session is always closed 
*        otherwise it remains and can be reopend later on.
*
*  OUTPUTS
*     dstring *diag      - diagnisis information - on error
*
*  RESULT
*     int - DRMAA error codes
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: japi_exit() is MT safe
*******************************************************************************/
int japi_exit(bool close_session, int flag, dstring *diag)
{
   int *value;
   int cl_errno, ret = DRMAA_ERRNO_SUCCESS;

   DENTER(TOP_LAYER, "japi_exit");

   DPRINTF(("entering japi_exit() at "u32"\n", sge_get_gmt()));

   
   JAPI_LOCK_SESSION();   
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   
   japi_session = JAPI_SESSION_SHUTTING_DOWN;
   JAPI_UNLOCK_SESSION();
   JAPI_LOCK_CR();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_session = JAPI_SESSION_INACTIVE;
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* Here's how this stop process works:
    * o Kill any pending jobs
    * o Wait for the event client thread to die
    * o Close the comm lib connection
    * o Free the job list
    * o Close the session
    */

   /* First we clean up the pending job(s). */
   ret = japi_clean_up_jobs (flag, diag);

   JAPI_LOCK_EC_STATE();
   /* 
    * notify event client about shutdown
    *
    * Currently this is done by using the sge_gsi_kill_eventclient() call.  As
    * a backup, we also set japi_ec_state accordingly.
    */
   DPRINTF(("Notify event client about shutdown\n"));
   if ((japi_ec_state == JAPI_EC_UP) || (japi_ec_state == JAPI_EC_STARTING)) {
      /* If the event client thread is running, it will check the state at the
       * beginning of every cycle.  If the state is set to JAPI_EC_FINISHING
       * it will exit. */
      if (japi_ec_state == JAPI_EC_UP) {
         japi_ec_state = JAPI_EC_FINISHING;
         JAPI_UNLOCK_EC_STATE();

/* This call appears to be causing several problems. */
#if 0
         /* This call will cause the event client thread to return immediately
          * from ec_get(), which is the only place it could be blocked. */
         cl_com_ignore_timeouts(CL_TRUE);
/* So we try doing it the old fashioned way... */
#else
         japi_stop_event_client ();
#endif

         DPRINTF (("Waiting for event client to terminate.\n"));
         pthread_join (japi_event_client_thread, (void *)&value);
         japi_ec_state = JAPI_EC_DOWN;
      }
      /* If the event client thread is still starting up, we can shotcut its
       * start-up by setting the state to JAPI_EC_FINISHING without having to
       * first let it come up and then bring it down. */   
      else {
         japi_ec_state = JAPI_EC_FINISHING;
         JAPI_UNLOCK_EC_STATE();
         
         DPRINTF (("Waiting for event client to terminate.\n"));
         pthread_join (japi_event_client_thread, (void *)&value);
         japi_ec_state = JAPI_EC_DOWN;
      }
   }
   /* If it's down, we're fine.  It can't be finishing because only one
    * thread can be in japi_exit() at a time. */

   /* do not destroy session state until last japi call 
      depending on it is finished */
   JAPI_LOCK_REFCOUNTER();
   
   if (japi_threads_in_session > 0) {
      /* signal all application threads waiting for a job to finish */
      pthread_cond_broadcast(&Master_japi_job_list_finished_cv);
      
      while (japi_threads_in_session > 0) {
         pthread_cond_wait(&japi_threads_in_session_cv, &japi_threads_in_session_mutex);
      }
   }
   
   JAPI_UNLOCK_REFCOUNTER();
   
   /* Make certain nothing is still hanging around. */
   pthread_cond_broadcast (&japi_ec_state_starting_cv);
   
   /* 
    * Try to disconnect from commd
    * this will fail when the thread never made any commd communiction 
    * 
    * When DRMAA calls were made by multiple threads other
    * sge_commd commprocs remain registered. To unregister also
    * these commprocs a list of open commprocs per process is
    * required to implement kind of a all_thread_leave_commproc().
    * This function would then be called here instead.
    */
   /* There's two ways for us to get here.  The first is that we successfully
    * unregistered the event client and signaled the event client thread.  In
    * case, it's possible that the connection is in an unstable state.  However,
    * we don't actually care because all we're doing is closing it, and if the
    * thread was holding any locks, they were released when it died.  The second
    * ways is for the the unregister to have failed.  In this case, we had to
    * ask the GDI to ask the event client to shutdown.  If we've gotten here,
    * the event client thread is stopped and no further communications are
    * needed. */

   /* 
    * disconnect from commd
    */
   cl_errno = cl_commlib_shutdown_handle (cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name(), 0), 0);
   
   if (cl_errno != CL_RETVAL_OK) {
      sge_dstring_sprintf(diag, "cl_commlib_shutdown_handle() failed: %s\n", cl_get_error_text(cl_errno));
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

/* As long as we're killing the EC the old fashioned way, this isn't needed. */
#if 0
   /* Restore timeouts in case japi_init() gets called again. */
   cl_com_ignore_timeouts(CL_FALSE);
#endif
   
   /* We have to wait to free the job list until any waiting or syncing threads
    * have exited.  Otherwise, they may think their jobs exited badly. */
   JAPI_LOCK_JOB_LIST();    
   Master_japi_job_list = lFreeList(Master_japi_job_list);
   JAPI_UNLOCK_JOB_LIST();    

   /* close JAPI session or not */
   if (multi_threaded && close_session) {
      int tmp_ret = ret;
      
      dstring session_key = DSTRING_INIT;
      sge_dstring_copy_string(&session_key, japi_session_key);
      ret = japi_close_session(&session_key, diag);
      sge_dstring_free(&session_key);

      if (ret == DRMAA_ERRNO_SUCCESS) {
         DPRINTF(("successfully closed JAPI session\n"));
         /* If this return code is OK, we go back to the previous, which may not
          * be.*/
         ret = tmp_ret;
      }
   } else if (multi_threaded) {
      DPRINTF(("keeping JAPI session open\n"));
   }

   /* Session is not inactive until the session has been closed (or not).  If
    * I set the session to inactive earlier, another japi_init() could try to
    * open the same session before we could close it.  The result would be that
    * the session would be successfully opened by japi_init() and then quietly
    * closed by japi_exit() causing all kinds of headaches.
    * The same goes for the communications socket. */
   JAPI_LOCK_SESSION();
   japi_session = JAPI_SESSION_INACTIVE;
   JAPI_UNLOCK_SESSION();

   JAPI_UNLOCK_CR();
   
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_allocate_string_vector() *************************************
*  NAME
*     japi_allocate_string_vector() -- Allocate a string vector
*
*  SYNOPSIS
*     static drmaa_attr_values_t* japi_allocate_string_vector(int type) 
*
*  FUNCTION
*     Allocate a string vector iterator. Two different variations are 
*     supported: 
* 
*        JAPI_ITERATOR_BULK_JOBS 
*            Provides bulk job id strings in a memory efficient fashion. 
*
*        JAPI_ITERATOR_STRINGS
*            Implements a simple string list.
*
*  INPUTS
*     int type - JAPI_ITERATOR_BULK_JOBS or JAPI_ITERATOR_STRINGS
*
*  RESULT
*     static drmaa_attr_values_t* - the iterator
*
*  NOTES
*     MT-NOTE: japi_allocate_string_vector() is MT safe
*     should be moved to drmaa.c
*******************************************************************************/
drmaa_attr_values_t *japi_allocate_string_vector(int type) 
{
   drmaa_attr_values_t *iter;

   if (!(iter = (drmaa_attr_values_t *)malloc(sizeof(drmaa_attr_values_t)))) {
      return NULL;
   }
   iter->iterator_type = type;
   
   switch (type) {
   case JAPI_ITERATOR_BULK_JOBS:
      iter->it.ji.jobid    = 0;
      iter->it.ji.start    = 0;
      iter->it.ji.end      = 0;
      iter->it.ji.incr     = 0;
      iter->it.ji.next_pos = 0;
      break;
   case JAPI_ITERATOR_STRINGS:
      iter->it.si.strings = NULL;
      iter->it.si.next_pos = NULL;
      break;
   default:
      free(iter);
      iter = NULL;
   }

   return iter;
}

/****** JAPI/japi_string_vector_get_next() *************************************
*  NAME
*     japi_string_vector_get_next() -- Return next entry of a string vector
*
*  SYNOPSIS
*     int japi_string_vector_get_next(drmaa_attr_values_t* iter, dstring 
*     *val) 
*
*  FUNCTION
*     DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE is returned for an empty string 
*     vector. The next entry of a string vector is returned. 
*
*  INPUTS
*     drmaa_attr_values_t* iter - The string vector
*
*  OUTPUTS
*     dstring *val              - Returns next string value - on success.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_string_vector_get_next() is MT safe
*******************************************************************************/
int japi_string_vector_get_next(drmaa_attr_values_t* iter, dstring *val)
{
   
   DENTER(TOP_LAYER, "japi_string_vector_get_next");

   if (!iter) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   switch (iter->iterator_type) {
   case JAPI_ITERATOR_BULK_JOBS:
      if (iter->it.ji.next_pos > iter->it.ji.end) {
         DEXIT;
         return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
      }
      if (val)
         sge_dstring_sprintf(val, "%ld.%d", iter->it.ji.jobid, iter->it.ji.next_pos);
      iter->it.ji.next_pos += iter->it.ji.incr;
      DEXIT;
      return DRMAA_ERRNO_SUCCESS;
   case JAPI_ITERATOR_STRINGS:
      if (!iter->it.si.next_pos) {
         DEXIT;
         return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
      } 
      if (val)
         sge_dstring_copy_string(val, lGetString(iter->it.si.next_pos, ST_name));
      iter->it.si.next_pos = lNext(iter->it.si.next_pos);
      DEXIT;
      return DRMAA_ERRNO_SUCCESS;
   default:
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }
}

/****** JAPI/japi_delete_string_vector() ***************************************
*  NAME
*     japi_delete_string_vector() -- Release all resources of a string vector
*
*  SYNOPSIS
*     void japi_delete_string_vector(drmaa_attr_values_t* iter) 
*
*  FUNCTION
*     Release all resources of a string vector.
*
*  INPUTS
*     drmaa_attr_values_t* iter - to be released
*
*  NOTES
*     MT-NOTE: japi_delete_string_vector() is MT safe
*     should be moved to drmaa.c
*******************************************************************************/
void japi_delete_string_vector(drmaa_attr_values_t* iter )
{
   if (!iter)
      return;

   switch (iter->iterator_type) {
   case JAPI_ITERATOR_BULK_JOBS:
      break;
   case JAPI_ITERATOR_STRINGS:
      iter->it.si.strings = lFreeList(iter->it.si.strings);
      break;
   default:
      break;
   }
   free(iter);

   return;
}

/****** JAPI/japi_send_job() ***************************************************
*  NAME
*     japi_send_job() -- Send job to qmaster using GDI
*
*  SYNOPSIS
*     static int japi_send_job(lListElem *job, u_long32 *jobid, dstring *diag) 
*
*  FUNCTION
*     The job passed is sent to qmaster using GDI. The jobid is returned.
*
*  INPUTS
*     lListElem *job  - the job (JB_Type)
*     u_long32 *jobid - destination for resulting jobid
*     dstring *diag   - diagnosis information
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_send_job() is MT safe
*******************************************************************************/
static int japi_send_job(lListElem *sge_job_template, u_long32 *jobid, dstring *diag)
{
   lList *job_lp, *alp;
   lListElem *aep, *job;
   int result = DRMAA_ERRNO_SUCCESS;

   DENTER(TOP_LAYER, "japi_send_job");

   job_lp = lCreateList(NULL, JB_Type);
   lAppendElem(job_lp, lCopyElem(sge_job_template));

   /* use GDI to submit job for this session */
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_ADD|SGE_GDI_RETURN_NEW_VERSION, &job_lp, NULL, NULL);

   /* reinitialize 'job' with pointer to new version from qmaster */
   if ((job = lFirst(job_lp))) {
      *jobid = lGetUlong(job, JB_job_number);
   }
   
   job_lp = lFreeList(job_lp);

   if (!(aep = lFirst(alp))) {
      alp = lFreeList(alp);
      sge_dstring_copy_string(diag, "sge_gdi() failed returning answer list");
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* 
    *  We simply put all answer messages into the diag buffer.
    *  Each single answer message is at first added without newline 
    *  characters. Then a newline is added to delimit two messages.
    */
   for_each(aep, alp) {
      u_long32 quality;
      quality = lGetUlong(aep, AN_quality);
      if (quality == ANSWER_QUALITY_ERROR) {
         u_long32 answer_status = lGetUlong(aep, AN_status);

         switch (answer_status) {
         case STATUS_NOQMASTER:
         case STATUS_NOCOMMD:
            result = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
            break;
         case STATUS_NOTOK_DOAGAIN:
            result = DRMAA_ERRNO_TRY_LATER;
            break;
         default:
            result = DRMAA_ERRNO_DENIED_BY_DRM;
            break;
         }
      }

      answer_to_dstring(aep, diag);
      if (lNext(aep)) {
         sge_dstring_append(diag, "\n");
      }
   }
   alp = lFreeList(alp);

   DEXIT;
   return result;
}


/****** JAPI/japi_add_job() ****************************************************
*  NAME
*     japi_add_job() -- Add job/bulk job to library session data
*
*  SYNOPSIS
*     static int japi_add_job(u_long32 jobid, u_long32 start, u_long32 end, 
*     u_long32 incr, bool is_array, const char *func) 
*
*  FUNCTION
*     Add the job/bulk job to the library session data.
*
*  INPUTS
*     u_long32 jobid   - the jobid
*     u_long32 start   - start index
*     u_long32 end     - end index
*     u_long32 incr    - increment
*     bool is_array    - true for array/bulk jobs false otherwise
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTES: japi_add_job() is MT safe 
*******************************************************************************/
static int japi_add_job(u_long32 jobid, u_long32 start, u_long32 end, u_long32 incr, 
      bool is_array, dstring *diag)
{
   lListElem *japi_job;

   DENTER(TOP_LAYER, "japi_add_job");

   JAPI_LOCK_JOB_LIST();

   japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);
   if (japi_job) {
      JAPI_UNLOCK_JOB_LIST();

      /* job may not yet exist */
      sge_dstring_copy_string(diag, "job exists already in japi job list");
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* add job to library session data 
      -  all tasks in JJ_not_yet_finished_ids
      -  no task in JJ_finished_jobs */
   japi_job = lAddElemUlong(&Master_japi_job_list, JJ_jobid, jobid, JJ_Type);
   object_set_range_id(japi_job, JJ_not_yet_finished_ids, start, end, incr);

   /* mark it as array job */
   if (is_array) {
      u_long32 job_type;
      job_type = lGetUlong(japi_job, JJ_type);
      JOB_TYPE_SET_ARRAY(job_type);
      lSetUlong(japi_job, JJ_type, job_type);
   }

   JAPI_UNLOCK_JOB_LIST();

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_run_job() ****************************************************
*  NAME
*     japi_run_job() -- Submit a job using a SGE job template.
*
*  SYNOPSIS
*     int japi_run_job(dstring *job_id, lListElem *sge_job_template, 
*        dstring *diag)
*
*  FUNCTION
*     The job described in the SGE job template is submitted. The id 
*     of the job is returned.
*
*  INPUTS
*     lListElem *sge_job_template - SGE job template
*
*  OUTPUTS
*     dstring *job_id             - SGE jobid as string - on success.
*     dstring *diag               - diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*      Master_japi_job_list_mutex
*      japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: japi_run_job() is MT safe
*      Would be better to return job_id as u_long32.
*******************************************************************************/
int japi_run_job(dstring *job_id, lListElem *sge_job_template, dstring *diag)
{
   u_long32 jobid = 0;
   int drmaa_errno;
   const char *s;

   DENTER(TOP_LAYER, "japi_run_job");

   /* per thread initialization */
   if (japi_init_mt(diag) != DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_init_mt() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   /* ensure job list still is consistent when we add the job id of the submitted job later on */
   japi_inc_threads(SGE_FUNC);

   JAPI_UNLOCK_SESSION();

   /* tag job with JAPI session key */
   lSetString(sge_job_template, JB_session, japi_session_key);

   /* send job to qmaster using GDI */
   JAPI_ENTER_CR();
   drmaa_errno = japi_send_job(sge_job_template, &jobid, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      JAPI_EXIT_CR();
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_send_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* add job arry to library session data */
   drmaa_errno = japi_add_job(jobid, 1, 1, 1, false, diag);
   JAPI_EXIT_CR();
   
   /* this is just a dirty hook for testing purposes 
      need this to enforce certain error conditions */
   if ((s=getenv("SGE_DELAY_AFTER_SUBMIT"))) {
      int seconds = atoi(s);
      DPRINTF(("sleeping %d seconds\n", seconds));
      sleep(seconds);
      DPRINTF(("slept %d seconds\n", seconds));
   }

   japi_dec_threads(SGE_FUNC);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_add_job() */
      DEXIT;
      return drmaa_errno;
   }
   
   /* return jobid as string */
   if (job_id)
      sge_dstring_sprintf(job_id, "%ld", jobid);

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_run_bulk_jobs() ****************************************************
*  NAME
*     japi_run_bulk_jobs() -- Submit a bulk of jobs
*
*  SYNOPSIS
*     int japi_run_bulk_jobs(drmaa_attr_values_t **jobidsp, 
*           lListElem *sge_job_template, int start, int end, int incr, dstring *diag)
*
*  FUNCTION
*     Submit the SGE job template as array job.
*
*  INPUTS
*     lListElem *sge_job_template   - SGE job template
*     int start                     - array job start index
*     int end                       - array job end index
*     int incr                      - array job increment
*  
*  OUTPUTS
*     drmaa_attr_values_t **jobidsp - a string array of jobids - on success
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*      MT-NOTE: japi_run_bulk_jobs() is MT safe
*      Would be better to return job_id instead of drmaa_attr_values_t.
*******************************************************************************/
int japi_run_bulk_jobs(drmaa_attr_values_t **jobidsp, lListElem *sge_job_template, 
      int start, int end, int incr, dstring *diag)
{
   drmaa_attr_values_t *jobids;
   u_long32 jobid = 0;
   int drmaa_errno;

   DENTER(TOP_LAYER, "japi_run_bulk_jobs");

   /* check arguments */
   if (start > end || !incr) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   japi_inc_threads(SGE_FUNC);

   JAPI_UNLOCK_SESSION();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }
   
   /* tag job with JAPI session key */
   if (japi_session_key != NULL) {
      lSetString(sge_job_template, JB_session, japi_session_key);
   }

   /* send job to qmaster using GDI */
   JAPI_ENTER_CR();
   drmaa_errno = japi_send_job(sge_job_template, &jobid, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      JAPI_EXIT_CR();
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_send_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* add job arry to library session data */
   drmaa_errno = japi_add_job(jobid, start, end, incr, true, diag);
   japi_dec_threads(SGE_FUNC);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_add_job() */
      DEXIT;
      return drmaa_errno;
   }
   JAPI_EXIT_CR();

   if (!(jobids = japi_allocate_string_vector(JAPI_ITERATOR_BULK_JOBS))) {
      japi_dec_threads(SGE_FUNC);
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* initialize jobid iterator to be returned */
   jobids->it.ji.jobid    = jobid;
   jobids->it.ji.start    = start;
   jobids->it.ji.end      = end;
   jobids->it.ji.incr     = incr;
   jobids->it.ji.next_pos = start;

   /* return jobids */
   *jobidsp = jobids;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_user_hold_add_jobid() *****************************************
*  NAME
*     japi_user_hold_add_jobid() -- Helper function for composing GDI request
*
*  SYNOPSIS
*     static int japi_user_hold_add_jobid(u_long32 gdi_action, lList **request_list, 
*     u_long32 jobid, u_long32 taskid, bool array, dstring *diag)
*
*  FUNCTION
*     Adds a reduced job structure to the request list that causes the job/task
*     be hold/released when it is used with sge_gdi(SGE_JOB_LIST, SGE_GDI_MOD).
*
*  INPUTS
*     u_long32 gdi_action  - the GDI action to be performed
*     lList **request_list - the request list we operate on
*     u_long32 jobid       - the jobid
*     u_long32 taskid      - the taskid
*     bool array           - true in case of an arry job
*  
*  OUTPUTS
*     dstring *diag        - diagnosis information in case of an error
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*      MT-NOTE: japi_user_hold_add_jobid() is MT safe
*******************************************************************************/
static int japi_user_hold_add_jobid(u_long32 gdi_action, lList **request_list, 
      u_long32 jobid, u_long32 taskid, bool array, dstring *diag)
{
   const lDescr job_descr[] = {
         {JB_job_number,             lUlongT},
         {JB_verify_suitable_queues, lUlongT},
         {JB_ja_tasks,               lListT },
         {JB_ja_structure,           lListT },
         {NoName,                    lEndT}
   };
   const lDescr task_descr[] = {
         {JAT_task_number, lUlongT},
         {JAT_hold, lUlongT},
         {NoName, lEndT}
   };
   lListElem *jep, *tep;

   DENTER(TOP_LAYER, "japi_user_hold_add_jobid");

   if (!array)
      taskid = 0;

   /* ensure JB_Type structure exists */
   if (!(jep=lGetElemUlong(*request_list, JB_job_number, jobid)))
      jep = lAddElemUlong(request_list, JB_job_number, jobid, job_descr);

   /* ensure JAT_Type structure exists */
   if (lGetSubUlong(jep, JAT_task_number, taskid, JB_ja_tasks)) {
      /* taskid is referenced twice */
      if (diag) 
         sge_dstring_sprintf(diag, "task "U32CFormat" of jobid "U32CFormat" referenced twice\n", 
               taskid, jobid);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }
   tep = lAddSubUlong(jep, JAT_task_number, taskid, JB_ja_tasks, task_descr);

   /* set action */
   lSetUlong(tep, JAT_hold, gdi_action);
  
   if (taskid) {
      lList *tlp;
      lXchgList(jep, JB_ja_structure, &tlp);
      range_list_insert_id(&tlp, NULL, taskid);
      lXchgList(jep, JB_ja_structure, &tlp);
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_control() ****************************************************
*  NAME
*     japi_control() -- Apply control operation on JAPI jobs.
*
*  SYNOPSIS
*     int japi_control(const char *jobid, int action, dstring *diag)
*
*  FUNCTION
*     Apply control operation to the job specified. If 'jobid' is 
*     DRMAA_JOB_IDS_SESSION_ALL, then this routine acts on all jobs 
*     *submitted* during this DRMAA session. 
*     This routine returns once the action has been acknowledged, but 
*     does not necessarily wait until the action has been completed.
*
*  INPUTS
*     const char *jobid - The job id or DRMAA_JOB_IDS_SESSION_ALL.
*     int action        - The action to be performed. One of
*           DRMAA_CONTROL_SUSPEND: stop the job (qmod -s )
*           DRMAA_CONTROL_RESUME: (re)start the job (qmod -us)
*           DRMAA_CONTROL_HOLD: put the job on-hold (qhold) 
*           DRMAA_CONTROL_RELEASE: release the hold on the job (qrls)
*           DRMAA_CONTROL_TERMINATE: kill the job (qdel)
*  
*  OUTPUTS
*     drmaa_attr_values_t **jobidsp - a string array of jobids - on success
*
*  RESULT
*     int - DRMAA error codes
*
*
*  NOTES
*      MT-NOTE: japi_control() is MT safe
*      Would be good to have japi_control() operate on a vector of jobids.
*      Would be good to interface also operations qmod -r and qmod -c.
*******************************************************************************/
int japi_control(const char *jobid_str, int drmaa_action, dstring *diag)
{
   int drmaa_errno;
   u_long32 jobid, taskid;
   bool array;
   lList *alp = NULL;
   lListElem *aep;

   DENTER(TOP_LAYER, "japi_control");

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   JAPI_UNLOCK_SESSION();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* use GDI to implement control operations */
   switch (drmaa_action) {
   case DRMAA_CONTROL_SUSPEND:
   case DRMAA_CONTROL_RESUME:
      {
         lList *ref_list = NULL;

         if (!strcmp(jobid_str, DRMAA_JOB_IDS_SESSION_ALL)) {
            lListElem *japi_job;
            JAPI_LOCK_JOB_LIST();    
            for_each (japi_job, Master_japi_job_list) {
               jobid = lGetUlong(japi_job, JJ_jobid);   
               if (!JOB_TYPE_IS_ARRAY(lGetUlong(japi_job, JJ_type))) {
                  char buffer[1024];
                  dstring job_task_specifier;

                  sge_dstring_init(&job_task_specifier, buffer, sizeof(buffer));
                  sge_dstring_sprintf(&job_task_specifier, u32, jobid);
                  lAddElemStr(&ref_list, ST_name, jobid_str, ST_Type);
               } else {
                  lListElem *range;
                  for_each (range, lGetList(japi_job, JJ_not_yet_finished_ids)) {
                     dstring job_task_specifier = DSTRING_INIT;
                     u_long32 start, end, step;
                     sge_dstring_sprintf(&job_task_specifier, u32".", jobid);
                     range_get_all_ids(range, &start, &end, &step);
                     range_to_dstring(start, end, step, &job_task_specifier, false);
                     lAddElemStr(&ref_list, ST_name, jobid_str, ST_Type);
                  }
               }
            }
            JAPI_UNLOCK_JOB_LIST();    
         } else {
            /* just ensure jobid can be parsed */
            if (japi_parse_jobid(jobid_str, &jobid, &taskid, &array, diag)) {
               /* diag written by japi_parse_jobid() */
               ref_list = lFreeList(ref_list);
               DEXIT;
               return DRMAA_ERRNO_INVALID_ARGUMENT;
            }
            lAddElemStr(&ref_list, ST_name, jobid_str, ST_Type);
         }

         if (ref_list) {
            bool tmp_ret;
            lList *id_list = NULL;
                                             
            if (drmaa_action == DRMAA_CONTROL_SUSPEND) {
               tmp_ret = id_list_build_from_str_list(&id_list, &alp, ref_list,
                                                     QI_DO_SUSPEND, 0);
            } else {
               tmp_ret = id_list_build_from_str_list(&id_list, &alp, ref_list,
                                                     QI_DO_UNSUSPEND, 0);
            }
            
            JAPI_ENTER_CR();
            alp = sge_gdi(SGE_CQUEUE_LIST, SGE_GDI_TRIGGER, 
                          &id_list, NULL, NULL);
            JAPI_EXIT_CR();
            id_list = lFreeList(id_list);
            ref_list = lFreeList(ref_list);

            for_each (aep, alp) {
               if (lGetUlong(aep, AN_status) != STATUS_OK) {
                  int ret = japi_gdi_control_error2japi_error(aep, diag, drmaa_action);
                  lFreeList(alp);
                  DEXIT;
                  return ret;
               }
            }
            lFreeList(alp);
         }
      }
      break;

   case DRMAA_CONTROL_HOLD:
   case DRMAA_CONTROL_RELEASE:
      {
         lListElem *aep;
         lList *alp, *request_list = NULL;
         u_long32 gdi_action;

         /* set action */
         if (drmaa_action == DRMAA_CONTROL_HOLD)
            gdi_action = MINUS_H_TGT_USER|MINUS_H_CMD_ADD;
         else
            gdi_action = MINUS_H_TGT_USER|MINUS_H_CMD_SUB;

         if (!strcmp(jobid_str, DRMAA_JOB_IDS_SESSION_ALL)) {
            lListElem *japi_job;
            JAPI_LOCK_JOB_LIST();    
            for_each (japi_job, Master_japi_job_list) {
               jobid = lGetUlong(japi_job, JJ_jobid);   
               if (!JOB_TYPE_IS_ARRAY(lGetUlong(japi_job, JJ_type))) {
                  if ((drmaa_errno=japi_user_hold_add_jobid(gdi_action, &request_list, 
                              jobid, 0, false, diag))!=DRMAA_ERRNO_SUCCESS) {
                        /* diag written by japi_user_hold_add_jobid() */
                        JAPI_UNLOCK_JOB_LIST();    
                        request_list = lFreeList(request_list);
                        DEXIT;
                        return drmaa_errno;
                  }
               } else {
                  lListElem *range;
                  for_each (range, lGetList(japi_job, JJ_not_yet_finished_ids)) {
                     u_long32 min, max, step;
                     range_get_all_ids(range, &min, &max, &step);
                     for (taskid=min; taskid<=max; taskid+= step) {
                        if ((drmaa_errno=japi_user_hold_add_jobid(gdi_action, &request_list, jobid, 
                                 taskid, true, diag))!=DRMAA_ERRNO_SUCCESS) {
                           /* diag written by japi_user_hold_add_jobid() */
                           JAPI_UNLOCK_JOB_LIST();    
                           request_list = lFreeList(request_list);
                           DEXIT;
                           return drmaa_errno;
                        }
                     }
                  }
               }
            }
            JAPI_UNLOCK_JOB_LIST();    
         } else {
            if (japi_parse_jobid(jobid_str, &jobid, &taskid, &array, diag)) {
               /* diag written by japi_parse_jobid() */
               request_list = lFreeList(request_list);
               DEXIT;
               return DRMAA_ERRNO_INVALID_ARGUMENT;
            }
            if ((drmaa_errno=japi_user_hold_add_jobid(gdi_action, &request_list, jobid, taskid, 
                     array, diag))!=DRMAA_ERRNO_SUCCESS) {
               /* diag written by japi_user_hold_add_jobid() */
               request_list = lFreeList(request_list);
               DEXIT;
               return drmaa_errno;
            }
         }

         if (request_list) {
            JAPI_ENTER_CR();
            alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_MOD, &request_list, NULL, NULL);
            JAPI_EXIT_CR();
            request_list = lFreeList(request_list);

            for_each (aep, alp) {
               if (lGetUlong(aep, AN_status) != STATUS_OK) {
                  int ret = japi_gdi_control_error2japi_error(aep, diag, drmaa_action);
                  lFreeList(alp);
                  DEXIT;
                  return ret;
               }
            }
            lFreeList(alp);
         }
      }
      break;
 
   case DRMAA_CONTROL_TERMINATE:
      {
         lList *id_list = NULL;
         lListElem *id_entry;

         if (strcmp(jobid_str, DRMAA_JOB_IDS_SESSION_ALL) == 0) {
            bool done = false;
            int count = 0;
            char buffer[1024];
            dstring job_task_specifier;
            lListElem *japi_job = NULL;

            sge_dstring_init(&job_task_specifier, buffer, sizeof(buffer));

            JAPI_LOCK_JOB_LIST();
            japi_job = lFirst (Master_japi_job_list);
            
            while (!done) {
               count = 0;
               
               while (japi_job != NULL) {
                  jobid = lGetUlong(japi_job, JJ_jobid);
                  /* This overwrites the previous contents of the dstring. */
                  sge_dstring_sprintf(&job_task_specifier, u32, jobid);

                  id_entry = lAddElemStr(&id_list, ID_str, sge_dstring_get_string(&job_task_specifier), ID_Type);
                  if (JOB_TYPE_IS_ARRAY(lGetUlong(japi_job, JJ_type))) {
                     lSetList(id_entry, ID_ja_structure, lCopyList(NULL, lGetList(japi_job, JJ_not_yet_finished_ids)));
                  }
                  
                  /* japi_job starts out as the first element in the master job
                   * list.  Every time through this loop, we move to the next
                   * element.  We do this before the check for maximum num of
                   * jobs to delete so that the next time we come to this loop,
                   * japi_job will already point to the right job.  This saves
                   * us some initializer logic before the loop. */
                  japi_job = lNext (japi_job);
                  
                  /* Stop when we reach the deletion limit. */
                  if (++count >= MAX_JOBS_TO_DELETE) {
                     break;
                  }
               } /* while */
               
               /* If we exhausted the list before reaching the job limit, we're
                * done. */
               if (count < MAX_JOBS_TO_DELETE) {
                  DPRINTF (("Deleting %d jobs\n", count));
                  done = true;
               }
               else {
                  DPRINTF (("Deleting %d jobs\n", MAX_JOBS_TO_DELETE));
               }

               if (id_list) {
                  int ret = DRMAA_ERRNO_SUCCESS;                  
                  lList *idlp = NULL;
                  lListElem *idp = NULL;
                  
                  /* Look for jobs from any user. */
                  for_each (idp, id_list) {
                     idlp = lGetList (idp, ID_user_list);

                     if (idlp == NULL) {
                        idlp = lCreateList ("User List", ST_Type);
                        lSetList (idp, ID_user_list, idlp);
                     }

                     lAddElemStr (&idlp, ST_name, "*", ST_Type);
                  }

                  JAPI_ENTER_CR();
                  /* This function frees id_list */
                  ret = do_gdi_delete (&id_list, drmaa_action, true, diag);
                  JAPI_EXIT_CR();

                  if (ret != DRMAA_ERRNO_SUCCESS) {
                     return ret;
                  }
               } /* if */
            } /* while */
            JAPI_UNLOCK_JOB_LIST();
         } /* if */
         else {
            char buffer[1024];
            dstring job_task_specifier;
            sge_dstring_init(&job_task_specifier, buffer, sizeof(buffer));

            if (japi_parse_jobid(jobid_str, &jobid, &taskid, &array, diag)) {
               /* diag written by japi_parse_jobid() */
               id_list = lFreeList(id_list);
               DEXIT;
               return DRMAA_ERRNO_INVALID_ARGUMENT;
            }

            sge_dstring_sprintf(&job_task_specifier, u32, jobid);
            id_entry = lAddElemStr(&id_list, ID_str, sge_dstring_get_string(&job_task_specifier), ID_Type);
            if (array) {
               lList *tlp;
               lXchgList(id_entry, ID_ja_structure, &tlp);
               range_list_insert_id(&tlp, NULL, taskid);
               lXchgList(id_entry, ID_ja_structure, &tlp);
            }
         } /* else */

         if (id_list) {
            int ret = DRMAA_ERRNO_SUCCESS;
            lList *idlp = NULL;
            lListElem *idp = NULL;

            /* Look for jobs from any user. */
            for_each (idp, id_list) {
               idlp = lGetList (idp, ID_user_list);

               if (idlp == NULL) {
                  idlp = lCreateList ("User List", ST_Type);
                  lSetList (idp, ID_user_list, idlp);
               }

               lAddElemStr (&idlp, ST_name, "*", ST_Type);
            }

            JAPI_ENTER_CR();
            /* This function frees id_list */
            ret = do_gdi_delete (&id_list, drmaa_action, false, diag);
            JAPI_EXIT_CR();

            if (ret != DRMAA_ERRNO_SUCCESS) {
               return ret;
            }
         } /* if */
      }
      break;

   default:
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

enum {
   JAPI_WAIT_ALLFINISHED, /* there is nothing more to wait for */
   JAPI_WAIT_UNFINISHED,  /* there are still unfinished tasks  */
   JAPI_WAIT_FINISHED,    /* got a finished task               */
   JAPI_WAIT_INVALID,     /* the specified task does not exist */
   JAPI_WAIT_TIMEOUT      /* we ran into a timout before condition was met */
};

static int japi_gdi_control_error2japi_error(lListElem *aep, dstring *diag, int drmaa_control_action)
{
   int ret, gdi_error;

   DENTER(TOP_LAYER, "japi_gdi_control_error2japi_error");

   answer_to_dstring(aep, diag);
   switch ((gdi_error=lGetUlong(aep, AN_status))) {
   case STATUS_EEXIST:
      ret = DRMAA_ERRNO_INVALID_JOB;
      break;
   case STATUS_EDENIED2HOST:
   case STATUS_ENOMGR:
   case STATUS_ENOOPR:
   case STATUS_ENOTOWNER:
      ret = DRMAA_ERRNO_AUTH_FAILURE;
      break;
   case STATUS_NOQMASTER:
   case STATUS_NOCOMMD:
      ret = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
      break;
   case STATUS_ESEMANTIC:
      switch (drmaa_control_action) {
      case DRMAA_CONTROL_SUSPEND:
         ret = DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE;
         break;
      case DRMAA_CONTROL_RESUME:
         ret = DRMAA_ERRNO_RESUME_INCONSISTENT_STATE;
         break;
      case DRMAA_CONTROL_HOLD:
         ret = DRMAA_ERRNO_HOLD_INCONSISTENT_STATE;
         break;
      case DRMAA_CONTROL_RELEASE:
         ret = DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE;
         break;
      case DRMAA_CONTROL_TERMINATE:
         /* job termination never fails due to the wrong job state */
         ret = DRMAA_ERRNO_INVALID_JOB;
         break;
      default:
         ret = DRMAA_ERRNO_INTERNAL_ERROR;
         break;
      }
      break;
   default:
      ret = DRMAA_ERRNO_INTERNAL_ERROR;
      break;
   }
   DPRINTF(("mapping GDI error code %d to DRMAA error code %d\n", 
      gdi_error, ret)); 
   DEXIT;
   return ret;
}

/****** JAPI/japi_synchronize() ****************************************************
*  NAME
*     japi_synchronize() -- Synchronize with jobs to finish w/ and w/o reaping 
*                           job finish information.
*
*  SYNOPSIS
*     int japi_synchronize(const char *job_ids[], signed long timeout, 
*        bool dispose, dstring *diag)
*
*  FUNCTION
*     Wait until all jobs specified by 'job_ids' have finished
*     execution. When DRMAA_JOB_IDS_SESSION_ALL is used as jobid
*     one can synchronize with all jobs that were submitted during this 
*     JAPI session. A timeout can be specified to prevent blocking 
*     indefinitely. If the call exits before timeout all the jobs have 
*     been waited on or there was an interrupt. If the invocation exits 
*     on timeout, the return code is DRMAA_ERRNO_EXIT_TIMEOUT. The dispose 
*     parameter specifies whether job finish information shall be reaped.
*     This method requires the event client to have been started, either by
*     passing enable_wait as true to japi_init() or by calling
*     japi_enable_job_wait().
*
*  INPUTS
*     const char *job_ids[] - A vector of job id strings.
*     signed long timeout   - timeout in seconds or 
*                             DRMAA_TIMEOUT_WAIT_FOREVER for infinite waiting
*                             DRMAA_TIMEOUT_NO_WAIT for immediate returning
*     bool dispose          - Whether job finish information shall be reaped.
*
*  OUTPUTS
*     dstring *diag         - Diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*     MT-NOTE: japi_synchronize() is MT safe
*     The caller must check system time before and after this call
*     in order to check how much time has passed. This should be improved.
*******************************************************************************/
int japi_synchronize(const char *job_ids[], signed long timeout, bool dispose, dstring *diag)
{
   bool sync_all = false;
   int drmaa_errno, i;
   int wait_result;
   struct timespec ts;

   DENTER(TOP_LAYER, "japi_synchronize");

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   JAPI_LOCK_EC_STATE();
   if (japi_ec_state == JAPI_EC_DOWN) {
      JAPI_UNLOCK_EC_STATE();
      JAPI_UNLOCK_SESSION();
      sge_dstring_copy_string(diag, MSG_JAPI_NO_EVENT_CLIENT);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   JAPI_UNLOCK_EC_STATE();
   
   /* ensure job list still is consistent when we wait jobs later on */
   japi_inc_threads(SGE_FUNC);

   JAPI_UNLOCK_SESSION();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* wait(?) until specified jobs have finished according to library session data */

   /* synchronize with *all* jobs submitted during this session ? */
   for (i=0; job_ids[i]; i++) {
      if (!strcmp(job_ids[i], DRMAA_JOB_IDS_SESSION_ALL)) {
         sync_all = true;
         break;
      }
      else {
         if ((drmaa_errno=japi_parse_jobid(job_ids[i], NULL, NULL, NULL, 
                     diag))!=DRMAA_ERRNO_SUCCESS) {
            japi_dec_threads(SGE_FUNC);
            /* diag written by japi_parse_jobid() */
            DEXIT;
            return drmaa_errno;
         }
      }
   }

   if (timeout != DRMAA_TIMEOUT_WAIT_FOREVER) 
      sge_relative_timespec(timeout, &ts);

   JAPI_LOCK_JOB_LIST();

   while ((wait_result=japi_synchronize_retry(sync_all, job_ids, dispose) == JAPI_WAIT_UNFINISHED)) {

      /* must return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE when event client 
         thread was shutdown during japi_wait() use japi_ec_state ?? */
      /* has japi_exit() been called meanwhile ? */
      JAPI_LOCK_EC_STATE();
      if (japi_ec_state != JAPI_EC_UP) {
         JAPI_UNLOCK_EC_STATE();
         JAPI_UNLOCK_JOB_LIST();
         japi_dec_threads(SGE_FUNC);
         japi_standard_error(DRMAA_ERRNO_EXIT_TIMEOUT, diag);

         DEXIT;
         return DRMAA_ERRNO_EXIT_TIMEOUT;
      }
      JAPI_UNLOCK_EC_STATE();

      if (timeout != DRMAA_TIMEOUT_WAIT_FOREVER) {
         if (pthread_cond_timedwait(&Master_japi_job_list_finished_cv, 
                  &Master_japi_job_list_mutex, &ts)==ETIMEDOUT) {
            DPRINTF(("got a timeout while waiting for job(s) to finish\n"));
            wait_result = JAPI_WAIT_TIMEOUT; 
            break;
         } 
      } else
         pthread_cond_wait(&Master_japi_job_list_finished_cv, &Master_japi_job_list_mutex);
   }

   JAPI_UNLOCK_JOB_LIST();

   if (wait_result == JAPI_WAIT_TIMEOUT)
      drmaa_errno = DRMAA_ERRNO_EXIT_TIMEOUT;
   else 
      drmaa_errno = DRMAA_ERRNO_SUCCESS;

   /* remove reaped jobs from library session data */
   japi_dec_threads(SGE_FUNC);

   DEXIT;
   return drmaa_errno;
}

/****** JAPI/japi_synchronize_retry() ******************************************
*  NAME
*     japi_synchronize_retry() -- synchronize with all jobs
*
*  SYNOPSIS
*     static int japi_synchronize_retry(bool sync_all, const char *job_ids[], 
*     bool dispose) 
*
*  FUNCTION
*
*  INPUTS
*     bool sync_all         - synchronize with all jobs submitted during this
*                             session or with those specified in job_ids
*     const char *job_ids[] - the jobids in case of sync_all == false
*     bool dispose          - should job finish information be disposed
*
*  RESULT
*     static int - JAPI_WAIT_ALLFINISHED = there is nothing more to wait for
*                  JAPI_WAIT_UNFINISHED  = there are still unfinished tasks
*                  JAPI_WAIT_FINISHED    = got a finished task
*
*  NOTES
*     MT-NOTE: due to acess to Master_japi_job_list japi_synchronize_retry() 
*     MT-NOTE: is not MT safe; only one instance may be called at a time
*******************************************************************************/
static int japi_synchronize_retry(bool sync_all, const char *job_ids[], bool dispose)
{
   if (sync_all)
      return japi_synchronize_all_retry(dispose);
   else
      return japi_synchronize_jobids_retry(job_ids, dispose);
}

/****** JAPI/japi_synchronize_all_retry() **************************************
*  NAME
*     japi_synchronize_all_retry() -- Look whether all jobs are finished?
*
*  SYNOPSIS
*     static int japi_synchronize_all_retry(bool dispose) 
*
*  FUNCTION
*     The Master_japi_job_list is searched to investigate whether all
*     jobs submitted during this session are finshed. If dispose is true
*     job finish information is also removed during this operation.
*
*  INPUTS
*     bool dispose         - should job finish information be disposed
*
*  RESULT
*     static int - JAPI_WAIT_ALLFINISHED = there is nothing more to wait for
*                  JAPI_WAIT_UNFINISHED  = there are still unfinished tasks
*
*  NOTES
*     MT-NOTE: due to acess to Master_japi_job_list japi_synchronize_all_retry() 
*     MT-NOTE: is not MT safe; only one instance may be called at a time!
*******************************************************************************/
static int japi_synchronize_all_retry(bool dispose) 
{
   bool all_finished = true;
   lListElem *japi_task, *japi_job, *next;
   lList *not_yet_finished;
   u_long32 jobid;

   DENTER(TOP_LAYER, "japi_synchronize_all_retry");

   /* 
    * Synchronize with all jobs submitted during this session ...
    * What does "synchronize with *all* jobs of a session" include 
    * when new jobs show up in the library session data due to a second 
    * thread doing drmaa_run_job() while drmaa_synchronize() is not yet 
    * finished?
    *
    * With this implementation drmaa_synchronize() wait also for
    * new jobs that show up in library session data. Maybe excluding
    * these jobs would better ...? 
    * Anyways: Excluding these jobs would requires some extra effort.
    */
   next = lFirst(Master_japi_job_list);
   while ((japi_job = next)) {
      next = lNext(japi_job);

#if 0
      if (submitted in former session)
         continue;
         
#endif
      jobid = lGetUlong(japi_job, JJ_jobid);
      not_yet_finished = lGetList(japi_job, JJ_not_yet_finished_ids);
      if (not_yet_finished != NULL) {
         DPRINTF(("job "u32" still has unfinished tasks: "u32"\n", 
            jobid, range_list_get_first_id(not_yet_finished, NULL)));
         all_finished = false;
      }
      if (dispose) {
         /* remove all JJ_finished_tasks entries */
         for_each (japi_task, lGetList(japi_job, JJ_finished_tasks)) {
            DPRINTF(("dispose job finish information for job "u32" task "u32"\n",
                  jobid, lGetUlong(japi_task, JJAT_task_id)));
         }
         lSetList(japi_job, JJ_finished_tasks, NULL);

         /* remove JAPI job if no longer needed */
         if (!not_yet_finished)
            lRemoveElem(Master_japi_job_list, japi_job);
      }

      if (!all_finished) {
         DEXIT;
         return JAPI_WAIT_UNFINISHED;
      }
   }

   DEXIT;
   return JAPI_WAIT_ALLFINISHED;
}

/****** JAPI/japi_synchronize_jobids_retry() ***********************************
*  NAME
*     japi_synchronize_jobids_retry() --  Look whether particular jobs finished
*
*  SYNOPSIS
*     static int japi_synchronize_jobids_retry(const char *job_ids[], 
*     int dispose) 
*
*  FUNCTION
*     The Master_japi_job_list is searched to investigate whether particular
*     jobs specified in job_ids finshed. If dispose is true job finish 
*     information is also removed during this operation.
*
*  INPUTS
*     const char *job_ids[] - the jobids
*     bool dispose          - should job finish information be removed
*
*  RESULT
*     static int - JAPI_WAIT_ALLFINISHED = there is nothing more to wait for
*                  JAPI_WAIT_UNFINISHED  = there are still unfinished tasks
*
*  NOTES
*     japi_synchronize_jobids_retry() does no error checking with the job_ids
*     passed. Assumption is this was ensured before japi_synchronize_jobids_retry()
*     is called. 
*     MT-NOTE: due to acess to Master_japi_job_list japi_synchronize_jobids_retry() 
*     MT-NOTE: is not MT safe; only one instance may be called at a time!
*******************************************************************************/
static int japi_synchronize_jobids_retry(const char *job_ids[], bool dispose)
{
   bool all_finished = true;
   int i;
   lListElem *japi_job;
   lList *not_yet_finished;
   
   DENTER(TOP_LAYER, "japi_synchronize_jobids_retry");

   /*
    * We simply iterate over all jobids and do the wait operation 
    * for each of them. 
    */
   for (i=0; job_ids[i]; i++) {
      u_long32 jobid, taskid;  
      bool is_array;
    
      /* assumption is all job_ids can be parsed w/ error by japi_parse_jobid() 
         this must be ensured before japi_synchronize_jobids_retry() is called */
      japi_parse_jobid(job_ids[i], &jobid, &taskid, &is_array, NULL);

      japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);
      if (!japi_job) {
         DPRINTF(("synchronized with "u32"."u32"\n", jobid, taskid));
         continue;
      }

      not_yet_finished = lGetList(japi_job, JJ_not_yet_finished_ids);
      if (not_yet_finished && range_list_is_id_within(not_yet_finished, taskid)) {
         DPRINTF(("job "u32"."u32" is a still unfinished task\n", jobid, taskid));
         all_finished = false;

         DEXIT;
         return JAPI_WAIT_UNFINISHED;
      } 

      DPRINTF(("synchronized with "u32"."u32"\n", jobid, taskid));
      if (dispose) { 
         /* remove corresponding entry in JJ_finished_tasks */
         lDelSubUlong(japi_job, JJAT_task_id, taskid, JJ_finished_tasks);
         DPRINTF(("dispose job finish information for job "u32" task "u32"\n", jobid, taskid));
         if (!lGetList(japi_job, JJ_finished_tasks) && !not_yet_finished) {
            /* remove JAPI job if no longer needed */
            lRemoveElem(Master_japi_job_list, japi_job); 
         }
      }
   }

   DEXIT;
   return JAPI_WAIT_ALLFINISHED;
}


/****** JAPI/japi_wait() ****************************************************
*  NAME
*     japi_wait() -- Wait for job(s) to finish and reap job finish info
*
*  SYNOPSIS
*     int japi_wait(const char *job_id, dstring *waited_job, int *stat, 
*        signed long timeout, drmaa_attr_values_t **rusage, dstring *diag)
*
*  FUNCTION
*     This routine waits for a job with job_id to fail or finish execution. Passing a special string
*     DRMAA_JOB_IDS_SESSION_ANY instead job_id waits for any job. If such a job was
*     successfully waited its job_id is returned as a second parameter. This routine is
*     modeled on wait3 POSIX routine. To prevent
*     blocking indefinitely in this call the caller could use timeout specifying
*     after how many seconds to time out in this call.
*     If the call exits before timeout the job has been waited on
*     successfully or there was an interrupt.
*     If the invocation exits on timeout, the return code is DRMAA_ERRNO_EXIT_TIMEOUT.
*     The caller should check system time before and after this call
*     in order to check how much time has passed.
*     The routine reaps jobs on a successful call, so any subsequent calls
*     to japi_wait() should fail returning an error DRMAA_ERRNO_INVALID_JOB meaning
*     that the job has been already reaped. This error is the same as if the job was
*     unknown. Failing due to an elapsed timeout has an effect that it is possible to
*     issue japi_wait() multiple times for the same job_id.
*     This method requires the event client to have been started, either by
*     passing enable_wait as true to japi_init() or by calling
*     japi_enable_job_wait().
*
*  INPUTS
*     const char *job_id           - job id string representation of job to wait for
*                                    or DRMAA_JOB_IDS_SESSION_ANY to wait for any job
*     signed long timeout          - timeout in seconds or 
*                                    DRMAA_TIMEOUT_WAIT_FOREVER for infinite waiting
*                                    DRMAA_TIMEOUT_NO_WAIT for immediate returning
*     dstring *waited_job          - returns job id string presentation of waited job
*     int *wait_status             - returns job finish information about exit status/
*                                    signal/whatever
*     int event_mask               - Indicates what events to listen for.  Can be:
*                                      JAPI_JOB_START
*                                      JAPI_JOB_FINISH
*                                    or a combination by oring them together.
*     int *event                   - returns the actual event that occured.  When
*                                    the event_mask includes JAPI_JOB_START, this
*                                    parameter must be checked to be sure that
*                                    a JAPI_JOB_START event was received.  It is
*                                    possible, such as in the case of a rejected
*                                    immediate job, that japi_wait() will return
*                                    DRMAA_ERRNO_SUCCESS for a JAPI_JOB_FINISH
*                                    event even though the event_mask was set to
*                                    JAPI_JOB_START.
*     drmaa_attr_values_t **rusage - returns resource usage information about job run
*                                    when waiting for JAPI_JOB_FINISH.
*     dstring *diag                - diagnosis information in case japi_wait() fails
*     
*  RESULT
*     DRMAA_ERRNO_SUCCESS
*        Job finished.
*
*     DRMAA_ERRNO_EXIT_TIMEOUT
*        No job end within specified time.
*
*     DRMAA_ERRNO_INVALID_JOB
*        The job id specified was invalid or DRMAA_JOB_IDS_SESSION_ANY has been specified
*        and all jobs of this session have already finished.
*
*     DRMAA_ERRNO_NO_ACTIVE_SESSION
*        No active session. 
* 
*     DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE
*     DRMAA_ERRNO_AUTH_FAILURE
*     DRMAA_ERRNO_NO_RUSAGE
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*      Master_japi_job_list_mutex -> japi_ec_state_mutex
*
*  NOTES
*     MT-NOTE: japi_wait() is MT safe
*     Would be good to also return information about job failures in 
*     JJAT_failed_text.
*     Would be good to enhance japi_wait() in a way allowing not only to 
*     wait for job finish events but also other events that have an meaning
*     for the end user, e.g. job scheduled, job started, job rescheduled.
*******************************************************************************/
int japi_wait(const char *job_id, dstring *waited_job, int *stat,
              signed long timeout, int event_mask, int *event,
              drmaa_attr_values_t **rusage, dstring *diag)
{
   u_long32 jobid = 0;
   u_long32 taskid = 0;
   int wait4any = 0;
   bool is_array_task = 0;
   int drmaa_errno, wait_result;
   bool waited_is_task_array;
   u_long32 waited_jobid, waited_taskid;
   bool got_usage_info = false;

   DENTER(TOP_LAYER, "japi_wait");

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   JAPI_LOCK_EC_STATE();
   if (japi_ec_state == JAPI_EC_DOWN) {
      JAPI_UNLOCK_EC_STATE();
      JAPI_UNLOCK_SESSION();
      sge_dstring_copy_string(diag, MSG_JAPI_NO_EVENT_CLIENT);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }
   JAPI_UNLOCK_EC_STATE();
   
   /* ensure job list still is consistent when we wait jobs later on */
   japi_inc_threads(SGE_FUNC);

   JAPI_UNLOCK_SESSION();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_init_mt() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* check wait conditions */
   if (!strcmp(job_id, DRMAA_JOB_IDS_SESSION_ANY)) {
      wait4any = 1;
   }
   else {
      wait4any = 0;
      if ((drmaa_errno=japi_parse_jobid(job_id, &jobid, &taskid, &is_array_task, diag))
                           !=DRMAA_ERRNO_SUCCESS) {
         japi_dec_threads(SGE_FUNC);
         /* diag written by japi_parse_jobid() */
         DEXIT;
         return drmaa_errno;
      }
   }

   {
      struct timespec ts;
      lList *rusagep = NULL;

      if (timeout != DRMAA_TIMEOUT_WAIT_FOREVER) 
         sge_relative_timespec(timeout, &ts);

      JAPI_LOCK_JOB_LIST();

      while ((wait_result = japi_wait_retry(Master_japi_job_list, wait4any, jobid,
                                          taskid, is_array_task, event_mask, &waited_jobid,
                                          &waited_taskid, &waited_is_task_array,
                                          stat, event, &rusagep)) == JAPI_WAIT_UNFINISHED) {

         /* has japi_exit() been called meanwhile ? */
         JAPI_LOCK_EC_STATE();
         if (japi_ec_state != JAPI_EC_UP) {
            JAPI_UNLOCK_EC_STATE();
            JAPI_UNLOCK_JOB_LIST();
            japi_dec_threads(SGE_FUNC);
            japi_standard_error(DRMAA_ERRNO_EXIT_TIMEOUT, diag);
            DEXIT;
            return DRMAA_ERRNO_EXIT_TIMEOUT; /* could also return something else here */
         }
         JAPI_UNLOCK_EC_STATE();

         if (timeout != DRMAA_TIMEOUT_WAIT_FOREVER) {
            if (pthread_cond_timedwait(&Master_japi_job_list_finished_cv, 
                     &Master_japi_job_list_mutex, &ts)==ETIMEDOUT) {
               DPRINTF(("got a timeout while waiting for job(s) to finish\n"));
               wait_result = JAPI_WAIT_TIMEOUT; 
               break;
            } 
         } else {
            pthread_cond_wait(&Master_japi_job_list_finished_cv, &Master_japi_job_list_mutex);
         }
      } /* while */
      
      JAPI_UNLOCK_JOB_LIST();
      
      /* Build a drmaa_attr_values_t from the rusage list */
      if ((event_mask & JAPI_JOB_FINISH) && (rusage != NULL)) {
         lList *slp = NULL;
         lListElem *uep = NULL;
         lListElem *sep = NULL;
         char buffer[256];
         
         if (rusagep != NULL) {
            slp = lCreateList ("Usage List", ST_Type);
            got_usage_info = true;

            *rusage = japi_allocate_string_vector (JAPI_ITERATOR_STRINGS);

            for_each (uep, rusagep) {
               sep = lCreateElem (ST_Type);
               lAppendElem (slp, sep);

               sprintf (buffer, "%s=%.4f", lGetString (uep, UA_name), lGetDouble (uep, UA_value));
               lSetString (sep, ST_name, strdup (buffer));
            }

            (*rusage)->iterator_type = JAPI_ITERATOR_STRINGS;
            (*rusage)->it.si.strings = slp;
            (*rusage)->it.si.next_pos = lFirst (slp);
         }
      }

      japi_dec_threads(SGE_FUNC);

      rusagep = lFreeList(rusagep);
   }

   if (wait_result==JAPI_WAIT_INVALID) {
      japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_JOB;
   }
   if (wait_result==JAPI_WAIT_TIMEOUT) {
      japi_standard_error(DRMAA_ERRNO_EXIT_TIMEOUT, diag);
      DEXIT;
      return DRMAA_ERRNO_EXIT_TIMEOUT;
   }

   /* copy jobid of finished job into buffer provided by caller */
   if (wait_result==JAPI_WAIT_FINISHED && waited_job) {
      if (waited_is_task_array) 
         sge_dstring_sprintf(waited_job, "%ld.%d", waited_jobid, waited_taskid);
      else 
         sge_dstring_sprintf(waited_job, "%ld", waited_jobid);
   }

   if (wait_result!=JAPI_WAIT_FINISHED) {
      japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_JOB;
   }

   if ((event_mask & JAPI_JOB_FINISH) && (rusage != NULL) && !got_usage_info) {
      japi_standard_error (DRMAA_ERRNO_NO_RUSAGE, diag);
      DEXIT;   
      return DRMAA_ERRNO_NO_RUSAGE;
   }
   else {
      DEXIT;   
      return DRMAA_ERRNO_SUCCESS;
   }
}

/****** JAPI/japi_wait_retry() *************************************************
*  NAME
*     japi_wait_retry() -- seek for job_id in JJ_finished_jobs of all jobs
*
*  SYNOPSIS
*     static int japi_wait_retry(lList *japi_job_list, int wait4any, int jobid, 
*     int taskid, bool is_array_task, u_long32 *wjobidp, u_long32 *wtaskidp, 
*     bool *wis_task_arrayp, int *wait_status) 
*
*  FUNCTION
*     Search the passed japi_job_list for finished jobs matching the wait4any/
*     jobid/taskid condition.
*
*  INPUTS
*     lList *japi_job_list      - The JJ_Type japi joblist that is searched.
*     int wait4any              - 0 any finished job/task is fine
*     u_long32 jobid            - specifies which job is searched
*     u_long32 taskid           - specifies which task is searched
*     bool is_array_task        - true if it is an array taskid
*     int event_mask            - the events to wait for
*     u_long32 *wjobidp         - destination for jobid of waited job
*     u_long32 *wtaskidp        - destination for taskid of waited job
*     u_long32 *wis_task_arrayp - destination for taskid of waited job
*     int *wait_status          - destination for status that is finally returned 
*                                 by japi_wait()
*     int *wevent               - destination for actual event received
*     lList **rusagep           - desitnation for rusage info of waited job
*
*  RESULT
*     static int - JAPI_WAIT_ALLFINISHED = there is nothing more to wait for
*                  JAPI_WAIT_UNFINISHED  = no job/task finished, but there are still unfinished tasks
*                  JAPI_WAIT_FINISHED    = got a finished task
*
*  NOTES
*     MT-NOTE: japi_wait_retry() is MT safe
*******************************************************************************/
static int japi_wait_retry(lList *japi_job_list, int wait4any, u_long32 jobid,
                           u_long32 taskid, bool is_array_task, int event_mask,
                           u_long32 *wjobidp, u_long32 *wtaskidp,
                           bool *wis_task_arrayp, int *wait_status, int *wevent,
                           lList **rusagep)
{
   lListElem *job = NULL; 
   lListElem *task = NULL; 
   int actual_event = 0;
   int return_value = JAPI_WAIT_UNFINISHED;
   
   DENTER(TOP_LAYER, "japi_wait_retry");

   /* seek for job_id in JJ_finished_jobs of all jobs */
   if (event_mask & JAPI_JOB_FINISH) {
      if (wait4any) {
         int not_yet_reaped = 0;

         for_each (job, japi_job_list) {
            task = lFirst(lGetList(job, JJ_finished_tasks));
            
            if (task != NULL) {
               break;
            }
            
            /* This comes after the break because if we have a non-NULL task,
             * we don't bother looking at not_yet_reaped. */
            if (lGetList(job, JJ_not_yet_finished_ids) != NULL) {
               not_yet_reaped = 1;
            }
         }

         if ((task == NULL) || (job == NULL)) {
            if (not_yet_reaped) {
               return_value = JAPI_WAIT_UNFINISHED;
            } else {
               return_value = JAPI_WAIT_ALLFINISHED;
            }
         }
         else {
            return_value = JAPI_WAIT_FINISHED;
         }
      } else {
         job = lGetElemUlong(japi_job_list, JJ_jobid, jobid);
         if (!job) {
            return_value = JAPI_WAIT_ALLFINISHED;
         }
         else {
            /* for non-array jobs no task id may have been specified */
            if (!JOB_TYPE_IS_ARRAY(lGetUlong(job, JJ_type)) && taskid != 1) {
               return_value = JAPI_WAIT_INVALID;
            }
            else {
               task = lGetSubUlong(job, JJAT_task_id, taskid, JJ_finished_tasks);
               if (!task) {
                  if (range_list_is_id_within(lGetList(job, JJ_not_yet_finished_ids), taskid)) {
                     return_value = JAPI_WAIT_UNFINISHED;
                  } else {
                     return_value = JAPI_WAIT_ALLFINISHED;
                  }
               }
               else {
                  return_value = JAPI_WAIT_FINISHED;
               }
            }
         }
      }
   }
   
   if (return_value != JAPI_WAIT_UNFINISHED) {
      *wevent = JAPI_JOB_FINISH;
      actual_event = JAPI_JOB_FINISH;
   }
   else if (event_mask & JAPI_JOB_START) {
      if (wait4any) {
         bool still_running = false;
         bool failed = false;
         
         for_each (job, japi_job_list) {
            /* If there's a task in the started list, that counts. */
            if (lFirst (lGetList (job, JJ_started_task_ids)) != NULL) {
               break;
            }
            /* A task in the finished list when the started list is empty counts
             * as a failure. */
            else if (lFirst (lGetList (job, JJ_finished_tasks)) != NULL) {
               failed = true;
               break;
            }
            
            /* A task in the not yet finished list means we wait. */
            if (lGetList(job, JJ_not_yet_finished_ids) != NULL) {
               still_running = true;
            }
         }
         
         if (failed) {
            return_value = JAPI_WAIT_FINISHED;
            *wevent = JAPI_JOB_FINISH;
            actual_event = JAPI_JOB_START;
         }
         else if ((job == NULL) && still_running) {
            return_value = JAPI_WAIT_UNFINISHED;
         }
         else if (job == NULL) {
            return_value = JAPI_WAIT_ALLFINISHED;
            *wevent = JAPI_JOB_START;
            actual_event = JAPI_JOB_START;
         }
         else {
            return_value = JAPI_WAIT_FINISHED;
            *wevent = JAPI_JOB_START;
            actual_event = JAPI_JOB_START;
         }
      }
      else {
         job = lGetElemUlong(japi_job_list, JJ_jobid, jobid);
         
         if (!job) {
            return_value = JAPI_WAIT_ALLFINISHED;
            *wevent = JAPI_JOB_START;
            actual_event = JAPI_JOB_START;
         }
         else {
            /* for non-array jobs no task id may have been specified */
            if (!JOB_TYPE_IS_ARRAY(lGetUlong(job, JJ_type)) && taskid != 1) {
               return_value = JAPI_WAIT_INVALID;
            }
            else {
               if (range_list_is_id_within(lGetList(job, JJ_started_task_ids), taskid)) {
                  return_value = JAPI_WAIT_FINISHED;
                  *wevent = JAPI_JOB_START;
                  actual_event = JAPI_JOB_START;
               }
               else if (!range_list_is_id_within (lGetList (job, JJ_not_yet_finished_ids), taskid)) {
                  task = lGetSubUlong(job, JJAT_task_id, taskid, JJ_finished_tasks);

                  if (task == NULL) {
                     return_value = JAPI_WAIT_ALLFINISHED;
                     *wevent = JAPI_JOB_START;
                     actual_event = JAPI_JOB_START;
                  }
                  else {
                     /* This is a special case.  If the task makes it into the
                      * finished list without making it into the started list,
                      * the job was rejected before being started.  In this case
                      * there's no need to wait any longer, so we return
                      * JAPI_WAIT_FINISHED, but we set the wevent to
                      * JAPI_JOB_FINISH to show that it wasn't the job start
                      * event that caused the wait to end. */
                     return_value = JAPI_WAIT_FINISHED;
                     *wevent = JAPI_JOB_FINISH;
                     actual_event = JAPI_JOB_START;
                  }
               }
               else {
                  return_value = JAPI_WAIT_UNFINISHED;
               }
            }
         }
      }
   }
   
   if (return_value != JAPI_WAIT_FINISHED) {
      DEXIT;
      return return_value;
   }
  
   /* return all kinds of job finish information */
   *wjobidp = lGetUlong(job, JJ_jobid);
   if (JOB_TYPE_IS_ARRAY(lGetUlong(job, JJ_type))) {
      *wis_task_arrayp = true;
      
      /* For the job start event, the task is NULL at this point */
      if (actual_event == JAPI_JOB_START) {
         *wtaskidp = 1;
      }
      else {
         *wtaskidp = lGetUlong(task, JJAT_task_id);
      }
   } else {
      *wis_task_arrayp = false;
   }   

   if (actual_event == JAPI_JOB_FINISH) {
      if (wait_status) {
         *wait_status = lGetUlong(task, JJAT_stat);
      }

      if (rusagep != NULL) {
         lList *usage = lGetList (task, JJAT_rusage);

         if (usage != NULL) {
            lList *usage_copy = lCopyList ("Usage List", usage);

            if (*rusagep == NULL) {
               *rusagep = usage_copy;
            }
            else {
               lAddList (*rusagep, usage_copy);
            }
         }
      }
   }

   if (*wevent == JAPI_JOB_FINISH) {
      /* remove reaped jobs from library session data */
      lRemoveElem(lGetList(job, JJ_finished_tasks), task);
      if (range_list_is_empty(lGetList(job, JJ_not_yet_finished_ids)) 
         && lGetNumberOfElem(lGetList(job, JJ_finished_tasks))==0) {
         lRemoveElem(Master_japi_job_list, job);
      }
   }

   DEXIT;
   return JAPI_WAIT_FINISHED;
}


/* These bit masks below are used to assemble combined DRMAA state 
 * masks
 *
 *    DRMAA_PS_QUEUED_ACTIVE         DRMAA_PS_SUBSTATE_PENDING 
 *
 *    DRMAA_PS_SYSTEM_ON_HOLD        DRMAA_PS_SUBSTATE_PENDING |
 *                                   DRMAA_PS_SUBSTATE_SYSTEM_SUSP
 *
 *    DRMAA_PS_USER_ON_HOLD          DRMAA_PS_SUBSTATE_PENDING |
 *                                   DRMAA_PS_SUBSTATE_USER_SUSP
 *
 *    DRMAA_PS_USER_SYSTEM_ON_HOLD   DRMAA_PS_SUBSTATE_PENDING |
 *                                   DRMAA_PS_SUBSTATE_SYSTEM_SUSP |
 *                                   DRMAA_PS_SUBSTATE_USER_SUSP
 *
 *    DRMAA_PS_RUNNING               DRMAA_PS_SUBSTATE_RUNNING
 *
 *    DRMAA_PS_SYSTEM_SUSPENDED      DRMAA_PS_SUBSTATE_RUNNING |
 *                                   DRMAA_PS_SUBSTATE_SYSTEM_SUSP
 *
 *    DRMAA_PS_USER_SUSPENDED        DRMAA_PS_SUBSTATE_RUNNING |
 *                                   DRMAA_PS_SUBSTATE_USER_SUSP
 *
 *    DRMAA_PS_USER_SYSTEM_SUSPENDED DRMAA_PS_SUBSTATE_RUNNING |
 *                                   DRMAA_PS_SUBSTATE_SYSTEM_SUSP |
 *                                   DRMAA_PS_SUBSTATE_USER_SUSP
 */
enum {
   DRMAA_PS_SUBSTATE_PENDING        = 0x10,
   DRMAA_PS_SUBSTATE_RUNNING        = 0x20,
   DRMAA_PS_SUBSTATE_SYSTEM_SUSP    = 0x01,
   DRMAA_PS_SUBSTATE_USER_SUSP      = 0x02
};

/****** JAPI/japi_sge_state_to_drmaa_state() ****************************************
*  NAME
*     japi_sge_state_to_drmaa_state() -- Map Grid Engine state into DRMAA state
*
*  SYNOPSIS
*     static int japi_sge_state_to_drmaa_state(lListElem *job, lList *queue_list, 
*     bool is_array_task, u_long32 jobid, u_long32 taskid, int *remote_ps, 
*     dstring *diag) 
*
*  FUNCTION
*     All Grid Engine state information is used and combined into a DRMAA 
*     job state.
*
*  INPUTS
*     lListElem *job     - the job (JB_Type)
*     lList *queue_list  - the queue list
*     bool is_array_task - if false jobid is considered the job id of a
*                          seq. job, if true jobid and taskid must fit
*                          to an existing array task.
*     u_long32 jobid     - the jobid of a seq. job or an array job 
*     u_long32 taskid    - the array task id in case of array jobs, 1 otherwise
*     int *remote_ps     - destination of DRMAA job state 
*     dstring *diag      - diagnosis information
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_sge_state_to_drmaa_state() is MT safe
*******************************************************************************/
static int 
japi_sge_state_to_drmaa_state(lListElem *job, lList *cqueue_list, 
                              bool is_array_task, u_long32 jobid, 
                              u_long32 taskid, int *remote_ps, dstring *diag)
{
   bool task_finished = false;
   lListElem *ja_task = NULL;
   
   DENTER(TOP_LAYER, "japi_sge_state_to_drmaa_state");

   if (job == NULL) {
      task_finished = true; 
   }
   else {
      ja_task = job_search_task(job, NULL, taskid);
      
      if ((ja_task != NULL) && (lGetUlong(ja_task, JAT_status) == JFINISHED)) {
         task_finished = true;
      }
   }

   /*
    * The reason for this job no longer being available at qmaster might 
    * be it is done or failed. The JAPI job list contains such information
    * if the job was not yet waited. For a job that is not found there either
    * we return DRMAA_ERRNO_INVALID_JOB.
    */
   if (task_finished) {
      lListElem *japi_job = NULL;
      lListElem *japi_task = NULL;

      JAPI_LOCK_JOB_LIST();
      
      japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);

      if (japi_job != NULL) {
         u_long32 wait_status;

         /* 
          * When the job/task has already been deleted at qmaster side but 
          * the event reporting this is not yet arrived at JAPI library 
          * the task is still contained in the not_yet_finished list. 
          *
          * We can assume that the job will be finished or failed, but
          * we can't know which one. Or we could presume the job was running, 
          * but what if it was pending and then deleted using qdel?
          */
         if (range_list_is_id_within(lGetList(japi_job, JJ_not_yet_finished_ids), taskid)) {
            JAPI_UNLOCK_JOB_LIST();
            *remote_ps = DRMAA_PS_UNDETERMINED;
            DEXIT;
            return DRMAA_ERRNO_SUCCESS;
         }

         japi_task = lGetSubUlong(japi_job, JJAT_task_id, taskid, JJ_finished_tasks);
         wait_status = lGetUlong(japi_task, JJAT_stat);
         DPRINTF(("wait_status("u32"/"u32") = "u32"\n", jobid, taskid, wait_status));
         
         if (SGE_GET_NEVERRAN(wait_status)) {
            *remote_ps = DRMAA_PS_FAILED;
         } else {
            *remote_ps = DRMAA_PS_DONE;
         }
         
         JAPI_UNLOCK_JOB_LIST();
         DEXIT;
         return DRMAA_ERRNO_SUCCESS;
      }
      
      if ((japi_job == NULL) || (japi_task == NULL)) {
         JAPI_UNLOCK_JOB_LIST();
         japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
         DEXIT;
         return DRMAA_ERRNO_INVALID_JOB;
      }

      /* 
       * JJAT_stat must indicate whether the job finished or failed 
       * at this point we simply assume it finished successfully 
       * when it is found in the finished tasks list
       */

      JAPI_UNLOCK_JOB_LIST();
      *remote_ps = DRMAA_PS_DONE;
      DEXIT;
      return DRMAA_ERRNO_SUCCESS;
   }

   if (!is_array_task) {
      /* reject "jobid" without taskid for array jobs */
      if (JOB_TYPE_IS_ARRAY(lGetUlong(job, JB_type))) {
         japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
         DEXIT;
         return DRMAA_ERRNO_INVALID_JOB;
      }
   } else {
      /* reject "jobid.taskid" for non-array jobs and ensure taskid exists in job array */
      if (!JOB_TYPE_IS_ARRAY(lGetUlong(job, JB_type)) || !job_is_ja_task_defined(job, taskid)) {
         japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
         DEXIT;
         return DRMAA_ERRNO_INVALID_JOB;
      }
   }
   
   ja_task = job_search_task(job, NULL, taskid);
   
   if (ja_task != NULL) { 
      /* the state of enrolled tasks can be direclty determined */
      u_long32 ja_task_status = lGetUlong(ja_task, JAT_status);
      u_long32 ja_task_state = lGetUlong(ja_task, JAT_state);
      u_long32 ja_task_hold = lGetUlong(ja_task, JAT_hold);

      /* ERROR */
      if (ja_task_state & JERROR) { 
         *remote_ps = DRMAA_PS_FAILED;
         DEXIT;
         return DRMAA_ERRNO_SUCCESS;
      }

      /* PENDING */
      if (ja_task_status == JIDLE) {
         *remote_ps = DRMAA_PS_SUBSTATE_PENDING;

         /*
          * Only one hold state (-h u) is considered USER HOLD.
          * Others are also user's hold but only this hold state 
          * can be released using japi_control().
          */
         if ((ja_task_hold & MINUS_H_TGT_USER)) 
            *remote_ps |= DRMAA_PS_SUBSTATE_USER_SUSP;

         /* 
          * These hold states are considered SYSTEM HOLD. Some of 
          * them (WAITING_DUE_TO_TIME, WAITING_DUE_TO_PREDECESSOR ) 
          * actually are the user's hold but DRMAA user interface does 
          * not know these hold * conditions.
          */
         if ((ja_task_hold & (MINUS_H_TGT_OPERATOR|MINUS_H_TGT_SYSTEM)) || 
             (lGetUlong(job, JB_execution_time) > sge_get_gmt()) ||
             lGetList(job, JB_jid_predecessor_list))
            *remote_ps |= DRMAA_PS_SUBSTATE_SYSTEM_SUSP;

         DEXIT;
         return DRMAA_ERRNO_SUCCESS;
      }
      
      /* RUNNING */
      *remote_ps = DRMAA_PS_SUBSTATE_RUNNING;

      /* 
       * Only the qmod -s <jobid> suspension is a USER SUSPEND 
       * other suspension can be controlled only by the admins.
       */
      if ((ja_task_state & JSUSPENDED))
         *remote_ps |= DRMAA_PS_SUBSTATE_USER_SUSP;

      /* 
       * A SYSTEM SUSPEND can be 
       *   - suspended due to suspend threshold
       *   - suspended because queue is qmod -s <queue> suspended 
       *   - suspended because queue is suspended on subordinate 
       *   - suspended because queue is suspended by calendar 
       */
      if (ja_task_state & JSUSPENDED_ON_THRESHOLD) {
         *remote_ps |= DRMAA_PS_SUBSTATE_SYSTEM_SUSP;
      } else {
         lListElem *cqueue = NULL;

         for_each(cqueue, cqueue_list) {
            lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
            lList *qdil_list = lGetList(ja_task, JAT_granted_destin_identifier_list);

            if (gqueue_is_suspended(qdil_list, qinstance_list)) {
               *remote_ps |= DRMAA_PS_SUBSTATE_SYSTEM_SUSP;
               break;
            }
         }
      }

      DEXIT; /* ???  */
      return DRMAA_ERRNO_SUCCESS;
   }
  
   /* not yet enrolled tasks are always PENDING */
   *remote_ps = DRMAA_PS_SUBSTATE_PENDING;

   if (range_list_is_id_within(lGetList(job, JB_ja_u_h_ids), taskid))
      *remote_ps |= DRMAA_PS_SUBSTATE_USER_SUSP;
   if (range_list_is_id_within(lGetList(job, JB_ja_s_h_ids), taskid) ||
       range_list_is_id_within(lGetList(job, JB_ja_o_h_ids), taskid) /* ||
       (lGetUlong(job, JB_execution_time) > sge_get_gmt()) || 
                    lGetList(job, JB_jid_predecessor_list) */
       )
      *remote_ps |= DRMAA_PS_SUBSTATE_SYSTEM_SUSP;
 
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}




/****** JAPI/japi_get_job_and_queues() *****************************************
*  NAME
*     japi_get_job_and_queues() -- get job and the queue via GDI for job status
*
*  SYNOPSIS
*     static int japi_get_job_and_queues(u_long32 jobid, lList 
*     **retrieved_cqueue_list, lList **retrieved_job_list, dstring *diag) 
*
*  FUNCTION
*     We use GDI GET to get jobs status. Additionally also the queue list 
*     must be retrieved because the (queue) system suspend state is kept in 
*     the queue where the job runs.
*
*  INPUTS
*     u_long32 jobid               - the jobs id
*     lList **retrieved_cqueue_list - resulting queue list
*     lList **retrieved_job_list   - resulting job list
*     dstring *diag                - diagnosis info
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTES: japi_get_job_and_queues() is MT safe
*******************************************************************************/
static int japi_get_job_and_queues(u_long32 jobid, lList **retrieved_cqueue_list, 
      lList **retrieved_job_list, dstring *diag)
{
   lList *mal = NULL;
   lList *alp = NULL;
   lListElem *aep = NULL;
   int qu_id, jb_id = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "japi_get_job_and_queues");

   /* we need all queues */
   {
      lCondition *cqueue_selection;
      lEnumeration *cqueue_fields;

      cqueue_selection = lWhere("%T(%I != %s)", CQ_Type, CQ_name, SGE_TEMPLATE_NAME);
      cqueue_fields = lWhat("%T(%I%I)", CQ_Type, CQ_name, CQ_qinstances);
      if (!cqueue_selection || !cqueue_fields) {
         japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
         DEXIT;
         return DRMAA_ERRNO_NO_MEMORY;
      }

      JAPI_ENTER_CR();
      qu_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_CQUEUE_LIST, 
                            SGE_GDI_GET, NULL, cqueue_selection, cqueue_fields, 
                            NULL, &state);
      JAPI_EXIT_CR();
      cqueue_selection = lFreeWhere(cqueue_selection);
      cqueue_fields = lFreeWhat(cqueue_fields);
   }

   /* prepare GDI GET JOB selection */
   {
      lCondition *job_selection;
      lEnumeration *job_fields;

      job_selection = lWhere("%T(%I==%u)", JB_Type, JB_job_number, jobid);
      job_fields = lWhat("%T(%I%I%I%I%I%I%I%I%I%I)", JB_Type, 
            JB_job_number, 
            JB_type, 
            JB_ja_structure, 
            JB_ja_n_h_ids, 
            JB_ja_u_h_ids,
            JB_ja_s_h_ids,
            JB_ja_o_h_ids,
            JB_ja_tasks,
            JB_jid_predecessor_list, 
            JB_execution_time);
      if (!job_selection || !job_fields) {
         japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
         DEXIT;
         return DRMAA_ERRNO_NO_MEMORY;
      }
      JAPI_ENTER_CR();
      jb_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_JOB_LIST, SGE_GDI_GET, NULL, 
            job_selection, job_fields, &mal, &state);
      JAPI_EXIT_CR();
      job_selection = lFreeWhere(job_selection);
      job_fields = lFreeWhat(job_fields);
   }

   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CQUEUE_LIST, qu_id, mal, 
                                retrieved_cqueue_list);

   if (!(aep = lFirst(alp))) {
      sge_dstring_copy_string(diag, "sge_gdi() failed returning answer list");
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }
   {
      u_long32 quality;
      quality = lGetUlong(aep, AN_quality);
      if (quality == ANSWER_QUALITY_ERROR) {
         answer_to_dstring(aep, diag);
         alp = lFreeList(alp);
         DEXIT;
         return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
      } 
      alp = lFreeList(alp);
   }

   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_JOB_LIST, jb_id, mal, retrieved_job_list);
   
   if (!(aep = lFirst(alp))) {
      sge_dstring_copy_string(diag, "sge_gdi() failed returning answer list");
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }
   {
      u_long32 quality;
      quality = lGetUlong(aep, AN_quality);
      if (quality == ANSWER_QUALITY_ERROR) {
         answer_to_dstring(aep, diag);
         alp = lFreeList(alp);
         DEXIT;
         return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
      } 
      alp = lFreeList(alp);
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_parse_jobid() ************************************************
*  NAME
*     japi_parse_jobid() -- Parse jobid string
*
*  SYNOPSIS
*     static int japi_parse_jobid(const char *job_id_str, u_long32 *jp, 
*     u_long32 *tp, bool *ap, dstring *diag) 
*
*  FUNCTION
*     The string is parsed. Jobid and task id are returned, also
*     it is returned whether the id appears to be an array taskid.
*
*  INPUTS
*     const char *job_id_str - the jobid string
*     u_long32 *jp           - destination for jobid
*     u_long32 *tp           - destination for taskid
*     bool *ap               - was it an array task
*     dstring *diag          - diagnosis
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_parse_jobid() is MT safe
*******************************************************************************/
static int japi_parse_jobid(const char *job_id_str, u_long32 *jp, u_long32 *tp, 
   bool *ap, dstring *diag)
{
   u_long32 jobid, taskid;
   int is_array_task;
   const char *s;

   DENTER(TOP_LAYER, "japi_parse_jobid");

   /* parse jobid/taskid */
   if ((s=strchr(job_id_str, '.'))) {
      if (sscanf(job_id_str, u32"."u32, &jobid, &taskid) != 2) {
         sge_dstring_sprintf(diag, "job id passed "SFQ" is not a valid bulk job id\n", job_id_str);
         DEXIT;
         return DRMAA_ERRNO_INVALID_ARGUMENT;
      }
/*       DPRINTF(("parsing jobid.taskid: %ld.%ld\n", jobid, taskid)); */
      is_array_task = 1;
   } else {
      if (sscanf(job_id_str, u32, &jobid) != 1) {
         sge_dstring_sprintf(diag, "job id passed "SFQ" is not a valid job id\n", job_id_str);
         DEXIT;
         return DRMAA_ERRNO_INVALID_ARGUMENT;
      }
/*       DPRINTF(("parsing jobid: %ld\n", jobid)); */
      taskid = 1;
      is_array_task = 0;
   }

   if (jp) 
      *jp = jobid;
   if (tp) 
      *tp = taskid;
   if (ap) 
      *ap = is_array_task;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_job_ps() ****************************************************
*  NAME
*     japi_job_ps() -- Get job status
*
*  SYNOPSIS
*     int japi_job_ps(const char *job_id_str, int *remote_ps, dstring *diag)
*
*  FUNCTION
*     Get the program status of the job identified by 'job_id'.
*     The possible values returned in 'remote_ps' and their meanings are:
*     DRMAA_PS_UNDETERMINED = 00H : process status cannot be determined,
*     DRMAA_PS_QUEUED_ACTIVE = 10H : job is queued and active,
*     DRMAA_PS_SYSTEM_ON_HOLD = 11H : job is queued and in system hold,
*     DRMAA_PS_USER_ON_HOLD = 12H : job is queued and in user hold,
*     DRMAA_PS_USER_SYSTEM_ON_HOLD = 13H : job is queued and in user and system hold,
*     DRMAA_PS_RUNNING = 20H : job is running,
*     DRMAA_PS_SYSTEM_SUSPENDED = 21H : job is system suspended,
*     DRMAA_PS_USER_SUSPENDED = 22H : job is user suspended,
*     DRMAA_PS_USER_SYSTEM_SUSPENDED = 23H : job is user and system suspended,
*     DRMAA_PS_DONE = 30H : job finished normally, and
*     DRMAA_PS_FAILED = 40H : job finished, but failed.
*
*  INPUTS
*     const char *job_id_str - A job id
*
*  OUTPUTS
*     int *remote_ps         - Returns the job state - on success
*     dstring *diag          - Returns diagnosis information - on error.
*
*  RESULT
*     int                    - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_job_ps() is MT safe
*     Would be good to enhance drmaa_job_ps() to operate on an array of 
*     jobids.
*     Would be good to have DRMAA_JOB_IDS_SESSION_ALL supported with 
*     drama_job_ps().
*******************************************************************************/
int japi_job_ps(const char *job_id_str, int *remote_ps, dstring *diag)
{
   u_long32 jobid, taskid;
   lList *retrieved_job_list = NULL;
   lList *retrieved_cqueue_list = NULL;
   int drmaa_errno;
   bool is_array_task;

   DENTER(TOP_LAYER, "japi_job_ps");

   /* check arguments */
   if (!job_id_str || !remote_ps) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   /* ensure job list still is consistent when we must access it later on
      to retrieve state information */
   japi_inc_threads(SGE_FUNC);

   JAPI_UNLOCK_SESSION();

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   DPRINTF(("japi_job_ps("SFQ")\n", job_id_str)); 
   if ((drmaa_errno=japi_parse_jobid(job_id_str, &jobid, &taskid, 
         &is_array_task, diag)) !=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_parse_jobid() */
      DEXIT;
      return drmaa_errno;
   }

   drmaa_errno = japi_get_job_and_queues(jobid, &retrieved_cqueue_list, &retrieved_job_list, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_get_job_and_queues() */
      DEXIT;
      return drmaa_errno;
   }

   drmaa_errno = japi_sge_state_to_drmaa_state(lFirst(retrieved_job_list), 
                                               retrieved_cqueue_list, 
                                               is_array_task, jobid, taskid, 
                                               remote_ps, diag);

   japi_dec_threads(SGE_FUNC);

   retrieved_job_list = lFreeList(retrieved_job_list);
   retrieved_cqueue_list = lFreeList(retrieved_cqueue_list);

   DEXIT;
   return drmaa_errno;
}

/****** JAPI/japi_wifaborted() *************************************************
*  NAME
*     japi_wifaborted() -- Did the job ever run?
*
*  SYNOPSIS
*     int japi_wifaborted(int *aborted, int stat, dstring *diag) 
*
*  FUNCTION
*     Evaluates into 'aborted' a non-zero value if 'stat' was returned for 
*     a JAPI job that ended before entering the running state.
*
*  INPUTS
*     int stat      - 'stat' value returned by japi_wait()
*
*  OUTPUTS
*     int *aborted  - Returns 1 if the job was aborted, 0 otherwise - on success.
*     dstring *diag - Returns diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wifaborted() is MT safe
*
*  SEE ALSO
*     JAPI/japi_wait()
*******************************************************************************/
int japi_wifaborted(int *aborted, int stat, dstring *diag)
{
   *aborted = SGE_GET_NEVERRAN(stat)?1:0;
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_wifexited() **************************************************
*  NAME
*     japi_wifexited() -- Has job exited?
*
*  SYNOPSIS
*     int japi_wifexited(int *exited, int stat, dstring *diag) 
*
*  FUNCTION
*     Allows to investigate whether a job has exited regularly.
*     If 'exited' returns 1 the exit status can be retrieved using
*     japi_wexitstatus(). 
*
*  INPUTS
*     int stat      - 'stat' value returned by japi_wait()
* 
*  OUTPUTS
*     int *exited   - Returns 1 if the job exited, 0 otherwise - on success.
*     dstring *diag - Returns diagnosis information - on error.
*
*  RESULT
*     int           - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wifexited() is MT safe
*
*  SEE ALSO
*     JAPI/japi_wexitstatus()
*******************************************************************************/
int japi_wifexited(int *exited, int stat, dstring *diag)
{
   *exited = SGE_GET_WEXITED(stat)?1:0;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_wexitstatus() ************************************************
*  NAME
*     japi_wexitstatus() -- Get jobs exit status.
*
*  SYNOPSIS
*     int japi_wexitstatus(int *exit_status, int stat, dstring *diag) 
*
*  FUNCTION
*     Retrieves the exit status of a job assumed it exited regularly 
*     according japi_wifexited().
*
*  INPUTS
*     int stat      - 'stat' value returned by japi_wait()
*
*  OUTPUTS
*     int *exit_status - Returns the jobs exit status - on success.
*     dstring *diag    - Returns diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wexitstatus() is MT safe
*
*  SEE ALSO
*     JAPI/japi_wifexited()
*******************************************************************************/
int japi_wexitstatus(int *exit_status, int stat, dstring *diag)
{
   *exit_status = SGE_GET_WEXITSTATUS(stat);
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_wifsignaled() **************************************************
*  NAME
*     japi_wifsignaled() -- Did the job die through a signal.
*
*  SYNOPSIS
*     int japi_wifsignaled(int *signaled, int stat, dstring *diag) 
*
*  FUNCTION
*     Allows to investigate whether a job died through a signal.
*     If 'signaled' returns 1 the signal can be retrieved using
*     japi_wtermsig().
*
*  INPUTS
*     int stat      - 'stat' value returned by japi_wait()
* 
*  OUTPUTS
*     int *signaled - Returns 1 if the job died through a signal, 
*        0 otherwise - on success.
*     dstring *diag - Returns diagnosis information - on error.
*
*  RESULT
*     int           - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wifsignaled() is MT safe
*
*  SEE ALSO
*     JAPI/japi_wtermsig()
*******************************************************************************/
int japi_wifsignaled(int *signaled, int stat, dstring *diag)
{
   *signaled = SGE_GET_WSIGNALED(stat)?1:0;
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_wtermsig() ***************************************************
*  NAME
*     japi_wtermsig() -- Retrieve the signal a job died through.
*
*  SYNOPSIS
*     int japi_wtermsig(dstring *sig, int stat, dstring *diag) 
*
*  FUNCTION
*     Retrieves the signal of a job assumed it died through a signal
*     according japi_wifsignaled().
*
*  INPUTS
*     int stat      - 'stat' value returned by japi_wait()
*
*  OUTPUTS
*     dstring *sig  - Returns signal the job died trough in string form 
*                     (e.g. "SIGKILL")
*     dstring *diag - Returns diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wtermsig() is MT safe
*     Would be better to directly SGE signal value, instead of a string.
*
*  SEE ALSO
*     JAPI/japi_wifsignaled()
*******************************************************************************/
int japi_wtermsig(dstring *sig, int stat, dstring *diag)
{
   u_long32 sge_sig = SGE_GET_WSIGNAL(stat);
   sge_dstring_sprintf(sig, "SIG%s", sge_sig2str(sge_sig));
   return DRMAA_ERRNO_SUCCESS;
}


/****** JAPI/japi_wifcoredump() ************************************************
*  NAME
*     japi_wifcoredump() -- Did job core dump?
*
*  SYNOPSIS
*     int japi_wifcoredump(int *core_dumped, int stat, dstring *diag) 
*
*  FUNCTION
*     If drmaa_wifsignaled() indicates a job died through a signal this function 
*     evaluates into 'core_dumped' a non-zero value if a core image of the terminated 
*     job was created.
*
*  INPUTS
*     int stat         - 'stat' value returned by japi_wait()
*
*  OUTPUTS
*     int *core_dumped - Returns 1 if a core image was created, 0 otherwises - 
*        on success.
*     dstring *diag    - Returns diagnosis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_wifcoredump() is MT safe
*******************************************************************************/
int japi_wifcoredump(int *core_dumped, int stat, dstring *diag)
{
   *core_dumped = SGE_GET_WCOREDUMP(stat)?1:0;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_standard_error() *********************************************
*  NAME
*     japi_standard_error() -- Provide standard diagnosis message.
*
*  SYNOPSIS
*     static void japi_standard_error(int drmaa_errno, dstring *diag) 
*
*  FUNCTION
*     
*
*  INPUTS
*     int drmaa_errno - DRMAA error code
*  
*  OUTPUT
*     dstring *diag   - diagnosis message
*
*  NOTES
*     MT-NOTE: japi_standard_error() is MT safe
*******************************************************************************/
void japi_standard_error(int drmaa_errno, dstring *diag)
{
   if (diag) {
      sge_dstring_copy_string(diag, japi_strerror(drmaa_errno));
   }
}


/****** JAPI/japi_strerror() ****************************************************
*  NAME
*     japi_strerror() -- JAPI strerror()
*
*  SYNOPSIS
*     void japi_strerror(int drmaa_errno, char *error_string, int error_len)
*
*  FUNCTION
*     Returns readable text version of errno (constant string)
*
*  INPUTS
*     int drmaa_errno - DRMAA error code
*
*  RESULT
*     A string describing the DRMAA error case for valid DRMAA error code 
*     and NULL otherwise.
*
*  NOTES
*     MT-NOTE: japi_strerror() is MT safe
*******************************************************************************/
const char *japi_strerror(int drmaa_errno)
{
   const struct error_text_s {
      int drmaa_errno;
      char *str;
   } error_text[] = {
      /* -------------- these are relevant to all sections ---------------- */
      { DRMAA_ERRNO_SUCCESS, "Routine returned normally with success." },
      { DRMAA_ERRNO_INTERNAL_ERROR, "Unexpected or internal DRMAA error like memory allocation, system call failure, etc." },
      { DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, "Could not contact DRM system" },
      { DRMAA_ERRNO_AUTH_FAILURE, "The specified request is not processed successfully due to authorization failure." },
      { DRMAA_ERRNO_INVALID_ARGUMENT, "The input value for an argument is invalid." },
      { DRMAA_ERRNO_NO_ACTIVE_SESSION, "No active session" },
      { DRMAA_ERRNO_NO_MEMORY, "failed allocating memory" },

      /* -------------- init and exit specific --------------- */
      { DRMAA_ERRNO_INVALID_CONTACT_STRING, "Initialization failed due to invalid contact string." },
      { DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR, "DRMAA could not use the default contact string to connect to DRM system." },
      { DRMAA_ERRNO_DRMS_INIT_FAILED, "Initialization failed due to failure to init DRM system." },
      { DRMAA_ERRNO_ALREADY_ACTIVE_SESSION, "Initialization failed due to existing DRMAA session." },
      { DRMAA_ERRNO_DRMS_EXIT_ERROR, "DRM system disengagement failed." },

   /* ---------------- job attributes specific -------------- */
      { DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT, "The format for the job attribute value is invalid." },
      { DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, "The value for the job attribute is invalid." },
      { DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES, "The value of this attribute is conflicting with a previously set attributes." },

   /* --------------------- job submission specific -------------- */
      { DRMAA_ERRNO_TRY_LATER, "Could not pass job now to DRM system. A retry may succeed however (saturation)." },
      { DRMAA_ERRNO_DENIED_BY_DRM, "The DRM system rejected the job. The job will never be accepted due to DRM configuration or job template settings." },

   /* ------------------------------- job control specific ---------------- */
      { DRMAA_ERRNO_INVALID_JOB, "The job specified by the 'jobid' does not exist." },
      { DRMAA_ERRNO_RESUME_INCONSISTENT_STATE, "The job has not been suspended. The RESUME request will not be processed." },
      { DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE, "The job has not been running, and it cannot be suspended." },
      { DRMAA_ERRNO_HOLD_INCONSISTENT_STATE, "The job cannot be moved to a HOLD state." },
      { DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE, "The job is not in a HOLD state." },
      { DRMAA_ERRNO_EXIT_TIMEOUT, "time-out condition" },
      { DRMAA_ERRNO_NO_RUSAGE, "no usage information was returned for the completed job" },

      { DRMAA_NO_ERRNO, NULL }
   };

   int i;

   for (i=0; error_text[i].drmaa_errno != DRMAA_NO_ERRNO; i++)
      if (drmaa_errno == error_text[i].drmaa_errno) 
         return error_text[i].str;

   return NULL; 
}

/****** japi/japi_get_contact() ************************************************
*  NAME
*     japi_get_contact() -- Return current contact information 
*
*  SYNOPSIS
*     void japi_get_contact(dstring *contact) 
*
*  FUNCTION
*     Current contact information for DRM system
*
*  INPUTS
*     dstring *contact - Returns a string simiar to 'contact' of japi_init().
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTES: japi_get_contact() is MT safe
*
*  SEE ALSO
*     JAPI/japi_init()
*******************************************************************************/
void japi_get_contact(dstring *contact)
{
}

/****** japi/japi_version() ****************************************************
*  NAME
*     japi_version() -- Return DRMAA version the JAPI library is compliant to.
*
*  SYNOPSIS
*     void japi_version(unsigned int *major, unsigned int *minor) 
*
*  FUNCTION
*     Return DRMAA version the JAPI library is compliant to.
*
*  OUTPUTs
*     unsigned int *major - ??? 
*     unsigned int *minor - ??? 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: japi_version() is MT safe
*******************************************************************************/
void japi_version(unsigned int *major, unsigned int *minor)
{
}


/****** JAPI/japi_get_drm_system() *********************************************
*  NAME
*     japi_get_drm_system() -- ??? 
*
*  SYNOPSIS
*     int japi_get_drm_system(dstring *drm, dstring *diag) 
*
*  FUNCTION
*     Returns SGE system implementation information. The output contain the DRM 
*     name and release information.
*
*  OUTPUTS
*     dstring *drm  - Returns DRM name - on success
*     dstring *diag - Returns diagnssis information - on error.
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_get_drm_system() is MT safe
*******************************************************************************/
int japi_get_drm_system(dstring *drm, dstring *diag)
{
   dstring buffer = DSTRING_INIT;
   pthread_once(&japi_once_control, japi_once_init);

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
/*       japi_standard_error(DRMAA_ERRNO_INTERNAL_ERROR, diag); */
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   sge_dstring_copy_string(drm, feature_get_product_name(FS_SHORT_VERSION, &buffer)); 
   sge_dstring_free(&buffer);
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_implementation_thread() **************************************
*  NAME
*     japi_implementation_thread() -- Control flow implementation thread
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*  RESULT
*
*  NOTES
*     MT-NOTE: japi_implementation_thread() is MT safe
*******************************************************************************/
static void *japi_implementation_thread(void *p)
{
   const int job_nm[] = {       
      JB_job_number, 
      JB_project, 
      JB_ja_tasks,
      JB_ja_structure,
      JB_ja_n_h_ids,
      JB_ja_u_h_ids,
      JB_ja_s_h_ids,
      JB_ja_o_h_ids,   
      JB_ja_template,
      NoName
   };
   
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lListElem *where_el = NULL;
   lListElem *what_el = NULL;
   lList *alp = NULL, *event_list = NULL;
   lListElem *event;
   char buffer[1024];
   dstring buffer_wrapper;
   bool stop_ec = false;
   int parameter, ed_time = 30, flush_delay_rate = 6;
   const char *s;
   bool up_and_running = false;
   bool qmaster_bound = false; /* Whether we ever successfully connected to the
                                  qmaster. */
   bool disconnected = false; /* Whether we are currently connected to the
                                 qmaster. */

   DENTER(TOP_LAYER, "japi_implementation_thread");

   /* Check EC state before we bother starting.  This also prevents the event
    * client thread from having a race condition with japi_enable_job_wait(). */
   JAPI_LOCK_EC_STATE();
   if (japi_ec_state != JAPI_EC_STARTING) {
      JAPI_UNLOCK_EC_STATE();
      lFreeList(alp);
      goto SetupFailed;
   }
   JAPI_UNLOCK_EC_STATE();
   
   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   /* needed to init comlib per thread globals */
   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if ( uti_state_get_mewho() == QUSERDEFINED) { 
      sge_gdi_param(SET_MEWHO, prog_number, NULL);
   } 

   if (sge_gdi_setup(uti_state_get_sge_formal_prog_name(), &alp)!=AE_OK) {
      DPRINTF(("error: sge_gdi_setup() failed for event client thread\n"));
      if (p) {
         lListElem *aep = lFirst(alp);
         if (aep) {
            answer_list_add((lList **)p, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
         }
      }
      lFreeList(alp);
      goto SetupFailed;
   }
   log_state_set_log_gui(0);

   /* JAPI parameters passed through environment */
   if ((s=getenv("SGE_JAPI_EDTIME"))) {
      parameter = atoi(s);
      if (parameter > 0) {
         ed_time = parameter;
      }
   }
   if ((s=getenv("SGE_JAPI_FLUSH_DELAY_RATE"))) {
      parameter = atoi(s);
      if (parameter > 0 && parameter < ed_time)
         flush_delay_rate = parameter;       
   }

   /* register at qmaster as event client */
   DPRINTF(("registering as event client ...\n"));
   ec_prepare_registration(EV_ID_ANY, uti_state_get_sge_formal_prog_name());
   ec_set_edtime(ed_time); 
   ec_set_busy_handling(EV_THROTTLE_FLUSH); 
   ec_set_flush_delay(flush_delay_rate); 
   ec_set_session(japi_session_key);
   ec_subscribe(sgeE_JOB_LIST);
   
   where = lWhere("%T(%I==%s)", JB_Type, JB_session, japi_session_key);
   what = lIntVector2What(JB_Type, job_nm); 

   where_el = lWhereToElem(where);
   what_el = lWhatToElem(what);
   
   ec_mod_subscription_where(sgeE_JOB_LIST, what_el, where_el);

   where = lFreeWhere(where);
   what = lFreeWhat(what);
   if (where_el) {
      where_el = lFreeElem(where_el);
   }
   
   if (what_el) {
      what_el = lFreeElem(what_el);
   }

   ec_subscribe(sgeE_JOB_FINISH);
   ec_set_flush(sgeE_JOB_FINISH, true, 0);

   ec_subscribe(sgeE_JATASK_MOD);
   ec_set_flush(sgeE_JATASK_MOD, true, 0);

   ec_subscribe(sgeE_SHUTDOWN);
   ec_set_flush(sgeE_SHUTDOWN, true, 0);

/*    sgeE_QMASTER_GOES_DOWN  ??? */

   /* Check again before we commit to this. */
   JAPI_LOCK_EC_STATE();
   if (japi_ec_state != JAPI_EC_STARTING) {
      JAPI_UNLOCK_EC_STATE();
      lFreeList(alp);
      goto SetupFailed;
   }
   
   if (!ec_register(false, &alp)) {      
      DPRINTF(("error: ec_register() failed\n"));
      
      if (p) {
         lListElem *aep = lFirst(alp);
         if (aep) {
            answer_list_add((lList **)p, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
         }
      }
      JAPI_UNLOCK_EC_STATE();
      lFreeList(alp);
      goto SetupFailed;
   }
   japi_ec_id = ec_get_id();
   JAPI_UNLOCK_EC_STATE();

   DPRINTF(("my formal prog name is \"%s\"\n",(char*)uti_state_get_sge_formal_prog_name()));
   cl_com_set_synchron_receive_timeout(cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name(),0),ed_time*2);

   while (!stop_ec) {
      int ec_get_ret = 0;

      /* read events and add relevant information into library session data */
      if ((ec_get_ret = ec_get(&event_list, false)) == false) {
         ec_mark4registration();
         
         DPRINTF (("Sleeping 10 seconds before trying to register again.\n"));
         sleep(10);
      } else {
         /* We need to check that we japi_exit() didn't wake us up to die. */
         JAPI_LOCK_EC_STATE();
         if (japi_ec_state == JAPI_EC_FINISHING) {
            JAPI_UNLOCK_EC_STATE();
            DPRINTF (("Received stop request while waiting for events.\n"));
            event_list = lFreeList(event_list);
            break;
         }
         JAPI_UNLOCK_EC_STATE();
         
         /* Bug Fix: Issuezilla #826
          * The first part of this bug fix is to keep the event client thread
          * from dying when the qmaster goes down.  In distinguish between
          * failures that represent the qmaster going down and failures that
          * represent other errors, such as the qmaster never having been up,
          * we note here that we were able to communication with the qmaster
          * at least once before we started having problems. */
         qmaster_bound = true;

         /* If we think we're disconnected, print a message saying we've
          * reconnected, and note that we're not disconnected. */
         if (disconnected) {
            if (error_handler != NULL) {
               error_handler (MSG_JAPI_RECONNECTED);
            }
            
            DPRINTF ((MSG_JAPI_RECONNECTED));
            disconnected = false;
         }
         
         for_each (event, event_list) {
            u_long32 number, type, intkey, intkey2;
            number = lGetUlong(event, ET_number);
            type = lGetUlong(event, ET_type);
            intkey = lGetUlong(event, ET_intkey);
            intkey2 = lGetUlong(event, ET_intkey2);

            DPRINTF(("\tEvent: %s intkey %d intkey2 %d\n", event_text(event, &buffer_wrapper), intkey, intkey2));

            /* maintain library session data */ 
            switch (type) {
            case sgeE_JOB_LIST:
               {
                  lList *sge_job_list = lGetList(event, ET_new_version);
                  lListElem *sge_job, *japi_job, *japi_task;
                  u_long32 jobid, taskid;
                  int finished_tasks = 0;

                  DPRINTF (("Handling job list event\n"));                  
                  JAPI_LOCK_JOB_LIST();

                  /* - check every session job  
                     - no longer existing jobs must be moved to JJ_finished_jobs
                     - TODO: actually we had to return DRMAA_ERRNO_NO_RUSAGE when japi_wait() is 
                             called for such a job. Must enhance JJAT_Type to reflect the case when 
                             no stat and rusage are known */
                  for_each(japi_job, Master_japi_job_list) {
                     jobid = lGetUlong(japi_job, JJ_jobid);
                     if (!(sge_job = lGetElemUlong(sge_job_list, JB_job_number, jobid))) {
                        while ((taskid = range_list_get_first_id(lGetList(japi_job, JJ_not_yet_finished_ids), NULL))) {
                           /* remove task from not yet finished job id list */
                           object_delete_range_id(japi_job, NULL, JJ_not_yet_finished_ids, taskid);

                           /* add entry to the finished tasks */
                           DPRINTF(("adding finished task "u32" for job "u32" existing not any longer\n", taskid, jobid));
                           japi_task = lAddSubUlong(japi_job, JJAT_task_id, taskid, JJ_finished_tasks, JJAT_Type);
                           finished_tasks++;

                        }
                     } else {
                        lListElem *range;
                        u_long32 min, max, step;
                        lList *range_list_copy;
                     
                        /*
                         * We must iterate over all taskid's in the JJ_not_yet_finished_ids list.
                         * Depending on the tasks state as the reported by qmaster entries 
                         * are removed from the JJ_not_yet_finished_ids list in this loop. 
                         * For this reason we operate on a copy to implement the loop.
                         */
                        range_list_copy = lCopyList(NULL, lGetList(japi_job, JJ_not_yet_finished_ids));

                        /* keep all tasks in 'not yet finished list' if tasks are 
                           still running or not yet running */
                        for_each (range, range_list_copy) {
                           range_get_all_ids(range, &min, &max, &step);
                           for (taskid=min; taskid<=max; taskid+= step) {
                              lListElem *ja_task;

                              if ((ja_task=job_search_task(sge_job, NULL, taskid))) {
                                 DPRINTF(("task "u32"."u32" contained in enrolled task list\n", jobid, taskid));
                                 continue;
                              }

                              if (range_list_is_id_within(lGetList(sge_job, JB_ja_n_h_ids), taskid) ||
                                  range_list_is_id_within(lGetList(sge_job, JB_ja_u_h_ids), taskid) ||
                                  range_list_is_id_within(lGetList(sge_job, JB_ja_s_h_ids), taskid) ||
                                  range_list_is_id_within(lGetList(sge_job, JB_ja_o_h_ids), taskid)) {
                                 DPRINTF(("task "u32"."u32" is still pending\n", jobid, taskid));
                                 continue;
                              }

                              if (range_list_is_id_within(lGetList(sge_job, JB_ja_z_ids), taskid)) {
                                 DPRINTF(("task "u32"."u32" contained in zombie list taskid list\n", jobid, taskid));
                              }

                              DPRINTF(("task "u32"."u32" presumably has finished meanwhile\n", jobid, taskid));

                              /* remove task from not yet finished job id list */
                              object_delete_range_id(japi_job, NULL, JJ_not_yet_finished_ids, taskid);
                              /* add entry to the finished tasks */
                              DPRINTF(("adding finished task %ld for job %ld which still exists\n", taskid, jobid));
                              japi_task = lAddSubUlong(japi_job, JJAT_task_id, taskid, JJ_finished_tasks, JJAT_Type);
                              finished_tasks++;
                           }
                        }

                        range_list_copy = lFreeList(range_list_copy);
                     }
                  }

                  /* signal all application threads waiting for a job to finish */
                  if (finished_tasks)
                     pthread_cond_broadcast(&Master_japi_job_list_finished_cv);
                  
                  JAPI_UNLOCK_JOB_LIST();

                  if (!up_and_running) {
                     /* set japi_ec_state to JAPI_EC_UP and notify initialization thread */
                     DPRINTF(("signalling event client thread is up and running\n"));

                     JAPI_LOCK_EC_STATE();
                     japi_ec_state = JAPI_EC_UP;
                     DPRINTF (("EC STATE is now %d\n", japi_ec_state));
                        pthread_cond_signal(&japi_ec_state_starting_cv);
                     JAPI_UNLOCK_EC_STATE();
                     up_and_running = true;
                  }
               }
               break;

            case sgeE_JOB_FINISH:
               /* - move job/task to JJ_finished_jobs */
               {
                  lListElem *japi_job, *japi_task;
                  u_long32 wait_status;
                  const char *err_str;
                  lListElem *jr = lFirst(lGetList(event, ET_new_version));

                  DPRINTF (("Handling job finish event\n"));
                  
                  wait_status = lGetUlong(jr, JR_wait_status);
                  err_str = lGetString(jr, JR_err_str);
                  if (SGE_GET_NEVERRAN(wait_status)) { 
                     DPRINTF(("JOB_FINISH: %d.%d job never ran: %s\n", 
                              intkey, intkey2, err_str));
                  } else {
                     if (SGE_GET_WEXITED(wait_status)) {
                        DPRINTF(("JOB_FINISH: %d.%d exited with exit status %d\n", 
                                 intkey, intkey2, SGE_GET_WEXITSTATUS(wait_status)));
                     }
                     if (SGE_GET_WSIGNALED(wait_status)) {
                        DPRINTF(("JOB_FINISH: %d.%d died through signal %s%s\n", 
                                 intkey, intkey2, sge_sig2str(SGE_GET_WSIGNAL(wait_status)),
                                 SGE_GET_WCOREDUMP(wait_status)?"(core dumped)":""));
                     }
                  }
                 
                  JAPI_LOCK_JOB_LIST();

                  japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, intkey);
                  if (japi_job) {
                     if (range_list_is_id_within(lGetList(japi_job, JJ_not_yet_finished_ids), intkey2)) {
                        lList *usage = NULL;
                        
                        /* remove task from not yet finished job id list */
                        object_delete_range_id(japi_job, NULL, JJ_not_yet_finished_ids, intkey2);

                        /* add an entry to the finished tasks */
                        DPRINTF(("adding finished task %ld for job %ld\n", intkey2, intkey));
                        japi_task = lAddSubUlong(japi_job, JJAT_task_id, intkey2, JJ_finished_tasks, JJAT_Type);
                        lSetUlong(japi_task, JJAT_stat, wait_status);
                        lSetString(japi_task, JJAT_failed_text, err_str);
                        
                        usage = lGetList (jr, JR_usage);
                        
                        if (usage != NULL)  {
                           lSetList(japi_task, JJAT_rusage, lCopyList ("job usage", usage));
                        }
                        
                        /* signal all application threads waiting for a job event */
                        pthread_cond_broadcast(&Master_japi_job_list_finished_cv);
                     }
                  } else {
                     DPRINTF (("ignoring event on unknown job "u32"\n", intkey));
                  }

                  JAPI_UNLOCK_JOB_LIST();
               }

               break;

            case sgeE_JATASK_MOD:
               /* - add task to JJ_started_task_ids */
               {
                  lListElem *japi_job;
                  lList *jat = lGetList(event, ET_new_version);
                  lListElem *ep = lFirst(jat);
                  u_long job_status = lGetUlong(ep, JAT_status);
                  bool task_running = (job_status==JRUNNING ||
                                      job_status==JTRANSFERING);

                  if (task_running) {
                     DPRINTF (("Handling task start event\n"));
                     
                     JAPI_LOCK_JOB_LIST();

                     japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid,
                                              intkey);
                     if (japi_job) {
                        if (!range_list_is_id_within (
                                    lGetList (japi_job, JJ_started_task_ids),
                                    intkey2)) {
                           lList *range = NULL;
                           
                           lXchgList(japi_job, JJ_started_task_ids, &range);
                           
                           if (range == NULL) {
                              range = lCreateList ("started tasks", RN_Type);
                           }
                           
                           /* add an entry to the started tasks */
                           DPRINTF(("adding started task %ld for job %ld\n",
                                    intkey2, intkey));
                           range_list_insert_id (&range, &alp, intkey2);
                           range_list_sort_uniq_compress (range, &japi_ec_alp);
                           lXchgList(japi_job, JJ_started_task_ids, &range);
                           
                           /* signal all application threads waiting for a job event */
                           pthread_cond_broadcast (&Master_japi_job_list_finished_cv);
                        }
                     } else {
                        DPRINTF(("ignoring event on unknown job "u32"\n", intkey));
                     }

                     JAPI_UNLOCK_JOB_LIST();
                  }
               }

               break;

            /* Bug Fix: Issuezilla #826
             * Since we only want to stop when explicitly told to, we have to
             * draw a distinction between SHUTDOWN and QMASTER_GOES_DOWN. On
             * SHUTDOWN we exit the event client thread.  On QMASTER_GOES_DOWN
             * we may eventually want to issue a warning message. */
            case sgeE_SHUTDOWN:
               DPRINTF (("Received shutdown message\n"));
               stop_ec = true;
               qmaster_bound = false;
               break;
            case sgeE_QMASTER_GOES_DOWN:
               /* Print a message that qmaster is down and note that we are
                * disconnected. */
               if (error_handler != NULL) {
                  error_handler (MSG_JAPI_QMASTER_DOWN);
               }

               DPRINTF ((MSG_JAPI_QMASTER_DOWN));
               disconnected = true;
               
               break;
            default:
               break;
            } /* switch */
         } /* for_each */
         event_list = lFreeList(event_list);
      } /* else */

      if (!stop_ec) {
         /* has japi_exit() been called meanwhile ? */ 
         JAPI_LOCK_EC_STATE();
         if (japi_ec_state == JAPI_EC_FINISHING) {
            stop_ec = true;
         }
         JAPI_UNLOCK_EC_STATE();
      }

      /* Bug Fix: Issuezilla #826
       * Here we have to make sure that we only give up if we've never actually
       * connected to the qmaster.  At some point we should probably implement
       * some kind of timeout to keep clients from waiting indefinitely for a
       * qmaster that may never come back. */
      if ((ec_get_ret == 0) && !stop_ec && !qmaster_bound) {
         /* Print a message that there's a communication problem */
         if (error_handler != NULL) {
            error_handler (MSG_JAPI_EC_GET_PROBLEM);
         }

         DPRINTF ((MSG_JAPI_EC_GET_PROBLEM));
         stop_ec = true;
      }
      else if ((ec_get_ret == 0) && !stop_ec && !disconnected) {
         /* Print a message that the qmaster is unavailable and note that we're
            disconnected. */
         if (error_handler != NULL) {
            error_handler (MSG_JAPI_DISCONNECTED);
         }

         DPRINTF ((MSG_JAPI_DISCONNECTED));
         disconnected = true;
      }
   } /* while */

   /* Unregister event client */
   DPRINTF(("unregistering from qmaster ...\n"));
   if (ec_deregister()==FALSE) {
      DPRINTF(("failed unregistering event client from qmaster.\n"));
   } else {
      DPRINTF(("... unregistered.\n"));
   }
      
   JAPI_LOCK_EC_STATE();
   /* We have to check here whether the event client ever got the first job list
    * event.  If not, being here counts as a failure. */
   /* The only non-error states here are JAPI_EC_UP="success" and
    * JAPI_EC_FINISHING="aborted by main thread." */
   if ((japi_ec_state == JAPI_EC_UP) || (japi_ec_state == JAPI_EC_FINISHING)) {
      japi_ec_state = JAPI_EC_DOWN;
   }
   else {
      japi_ec_state = JAPI_EC_FAILED;
   }
   
   japi_ec_id = 0;
   /* We signal here because it's possible that we started up ok but failed on
    * the first ec_get to get the job list event.  In that case, the main thread
    * will still be waiting for the event client to signal start up. */
   pthread_cond_signal(&japi_ec_state_starting_cv);
   JAPI_UNLOCK_EC_STATE();
   
   /* signal all application threads waiting for a job event */
   pthread_cond_broadcast (&Master_japi_job_list_finished_cv);

   DEXIT;
   return p;

SetupFailed:
   JAPI_LOCK_EC_STATE();
   japi_ec_state = JAPI_EC_FAILED;
   pthread_cond_signal(&japi_ec_state_starting_cv);
   JAPI_UNLOCK_EC_STATE();
   DEXIT;
   return p;
}

/****** JAPI/japi_clean_up_jobs() **********************************************
*  NAME
*     japi_clean_up_jobs() -- stops jobs still running in the session
*
*  SYNOPSIS
*     int japi_clean_up_jobs(int flag, dstring *diag)
*
*  FUNCTION
*     Deletes jobs running in the session when flag is set to JAPI_EXIT_KILL_ALL
*     or JAPI_EXIT_KILL_PENDING.
*
*  RESULT
*     int - 0 = OK, 1 = Error
*
*  NOTES
*     MT-NOTES: japi_clean_up_jobs() is MT safe (assumptions)
*******************************************************************************/
static int japi_clean_up_jobs(int flag, dstring *diag)
{
   lListElem *japi_job = NULL, *id_entry = NULL;
   lList *id_list = NULL, *alp = NULL;
   u_long32 jobid;
   int ret = DRMAA_ERRNO_SUCCESS;
   bool done = false;
   int count = 0;
   char buffer[1024];
   dstring job_task_specifier;

   DENTER (TOP_LAYER, "japi_clean_up_jobs");   
   
   sge_dstring_init(&job_task_specifier, buffer, sizeof(buffer));
   
   /* If there are any pending jobs, and a flag is set, kill them. */
   if ((flag == JAPI_EXIT_KILL_PENDING) || (flag == JAPI_EXIT_KILL_ALL)) {
      if (flag == JAPI_EXIT_KILL_PENDING) {
         DPRINTF (("Stopping all pending jobs in this session.\n"));
      }
      else if (flag == JAPI_EXIT_KILL_ALL) {
         DPRINTF (("Stopping all jobs in this session.\n"));
      }
      
      JAPI_LOCK_JOB_LIST();
      japi_job = lFirst (Master_japi_job_list);
      
      while (!done) {
         count = 0;
         
         while (japi_job != NULL) {
            jobid = lGetUlong(japi_job, JJ_jobid);

            DPRINTF (("Stopping job %ld\n", jobid));

            sge_dstring_sprintf(&job_task_specifier, u32, jobid);
            id_entry = lAddElemStr(&id_list, ID_str,
                                   sge_dstring_get_string(&job_task_specifier),
                                   ID_Type);

            if (JOB_TYPE_IS_ARRAY(lGetUlong(japi_job, JJ_type))) {
               /* Kill every task in the not yet finished list.  Some of the tasks
                * may have finished since we killed the event client, but that's
                * ok. If we can't stop a job, we just move on to the next one. */
               if (flag == JAPI_EXIT_KILL_PENDING) {
                  lList *del_list = NULL;

                  range_list_calculate_difference_set (&del_list, &alp,
                                    lGetList(japi_job, JJ_not_yet_finished_ids),
                                    lGetList(japi_job, JJ_started_task_ids));            
                  lSetList(id_entry, ID_ja_structure, del_list);
               }
               /* Kill every task that is in the not yet finished list but not in
                * the started list.  Same as above for tasks we can't kill. */
               else if (flag == JAPI_EXIT_KILL_ALL) {
                  lSetList(id_entry, ID_ja_structure, lCopyList(NULL,
                                  lGetList(japi_job, JJ_not_yet_finished_ids)));
               }
            }

            /* japi_job starts out as the first element in the master job
             * list.  Every time through this loop, we move to the next
             * element.  We do this before the check for maximum num of
             * jobs to delete so that the next time we come to this loop,
             * japi_job will already point to the right job.  This saves
             * us some initializer logic before the loop. */
            japi_job = lNext (japi_job);

            if (++count >= MAX_JOBS_TO_DELETE) {
               break; /* while */
            }
         } /* while */

         if (count < MAX_JOBS_TO_DELETE) {
            DPRINTF (("Deleting %d jobs\n", count));
            done = true;
         }
         else {
            DPRINTF (("Deleting %d jobs\n", MAX_JOBS_TO_DELETE));
         }

         if (id_list) {
            /* This function frees id_list. */
            ret = do_gdi_delete (&id_list, DRMAA_CONTROL_TERMINATE, true, diag);

            if (ret != DRMAA_ERRNO_SUCCESS) {
               break; /* while */
            }
         } /* if */
      } /* while */
      JAPI_UNLOCK_JOB_LIST();
   } /* if */
   
   DEXIT;
   return ret;
}


/****** japi/japi_was_init_called() *******************************************
*  NAME
*     japi_was_init_called() -- Return current contact information 
*
*  SYNOPSIS
*     int japi_was_init_called(dstring* diag) 
*
*  FUNCTION
*     Check if japi_init was already called.
*
*  OUTPUT
*     dstring *diag - returns diagnosis information - on error
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS if japi_init was already called,
*           DRMAA_ERRNO_NO_ACTIVE_SESSION if japi_init was not called,
*           DRMAA_ERRNO_INTERNAL_ERROR if an unexpected error occurs.
*
*  NOTES
*     MT-NOTES: japi_was_init_called() is MT safe
*******************************************************************************/
int japi_was_init_called(dstring* diag)
{
   int ret = DRMAA_ERRNO_SUCCESS;
   
   DENTER(TOP_LAYER, "japi_was_init_called");                     

   /* per thread initialization */
   /* diag written by japi_init_mt() */
   ret = japi_init_mt(diag);
   
   if (ret == DRMAA_ERRNO_SUCCESS) {
      /* ensure japi_init() was called */
      JAPI_LOCK_SESSION();
      
      if (japi_session != JAPI_SESSION_ACTIVE) {
         ret = DRMAA_ERRNO_NO_ACTIVE_SESSION; 
      }
                                           
      JAPI_UNLOCK_SESSION();
   }
   
   if (ret != DRMAA_ERRNO_SUCCESS) {
      japi_standard_error(ret, diag);
   }
   
   DEXIT;
   return ret;
}


/****** japi/japi_is_delegated_file_staging_enabled() *************************
*  NAME
*     japi_is_delegated_file_staging_enabled() -- Is file staging enabled, i.e.
*              is the "delegated_file_staging" configuration entry set to true?
*
*  SYNOPSIS
*     bool japi_is_delegated_file_staging_enabled()
*
*  FUNCTION
*     Returns if delegated file staging is enabled.
*
*  RESULT
*     bool - true if delegated file staging is enabled, else false.
*
*  NOTES
*     MT-NOTES: japi_is_delegated_file_staging_enabled() is MT safe
*******************************************************************************/
bool japi_is_delegated_file_staging_enabled(dstring *diag)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "japi_is_delegated_file_staging_enabled");
   
   JAPI_LOCK_SESSION();
   if (japi_delegated_file_staging_is_enabled == -1) {
      /* This function call does a GDI call, meaning it could take a while,
       * leaving the session mutex locked.  However, this only happens once.
       * The less noticable way to make this call is to call it from
       * japi_init().  The problem there, however, is documented as Issuezilla
       * bug #1025.  This is the next best solution and doesn't appear to cause
       * any noticable problems. */
      japi_read_dynamic_attributes (diag);
   }
   
   ret = (japi_delegated_file_staging_is_enabled == 1);
   JAPI_UNLOCK_SESSION();
   
   DEXIT;
   return ret;
}

/****** japi/japi_read_dynamic_attributes() ***********************************
*  NAME
*     japi_read_dynamic_attributes() -- Read the 'dynamic' attributes from
*                                       the DRM configuration.
*
*  SYNOPSIS
*     static int japi_read_dynamic_attributes(dstring *diag) 
*
*  FUNCTION
*     Reads from the DRM configuration, which 'dynamic' attributes are enabled.
*
*  OUTPUT
*     dstring *diag - returns diagnosis information - on error
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCES on success,
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE,
*           DRMAA_ERRNO_INVALID_ARGUMENT 
*           on error.
*
*  NOTES
*     MT-NOTES: japi_read_dynamic_attributes() is not MT safe.  It assumes that
*               the calling thread holds the session mutex.
*******************************************************************************/
static int japi_read_dynamic_attributes(dstring *diag)
{
   int        ret=0;
   int        drmaa_errno=DRMAA_ERRNO_SUCCESS;
   lList      *pSubList;
   lListElem  *config = NULL;
   lListElem  *ep = NULL;
   const char *pStr = NULL;

   DENTER(TOP_LAYER, "japi_read_dynamic_attributes");   

   if((ret=get_configuration("global", &config, NULL))<0) {
      switch( ret ) {
         case -2:
         case -4:
         case -6:
         case -7:
            drmaa_errno = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
            break;
         case -1:
         case -3:
            drmaa_errno = DRMAA_ERRNO_INVALID_ARGUMENT;
            break;
         case -5:
            /* -5 there is no global configuration
             * This means that "delegated_file_staging" is not set.
             * This is not an error for us, not set means default value.
             */
            drmaa_errno = DRMAA_ERRNO_SUCCESS;
            break;
      }
      
      japi_standard_error(drmaa_errno, diag);
      DEXIT;
      return drmaa_errno;
   }

   pSubList = lGetList(config, CONF_entries);
   if (pSubList != NULL) {
      ep = lGetElemStr(pSubList, CF_name, "delegated_file_staging");
      if (ep != NULL) {
         pStr = lGetString(ep, CF_value);
         
         if (strcasecmp( pStr, "true") ==0) {
            japi_delegated_file_staging_is_enabled = 1;
         }
         else {
            japi_delegated_file_staging_is_enabled = 0;
         }
      }
   }

   config = lFreeElem(config);
   DEXIT;
   return drmaa_errno;
}

/****** japi/do_gdi_delete() ***************************************************
*  NAME
*     do_gdi_delete() -- Delete the job list
*
*  SYNOPSIS
*     static int do_gdi_delete (lList **id_list, int action, bool delete_all,
*                               dstring diag)
*
*  FUNCTION
*     Deletes all the jobs in the job id list, converts and GDI errors into
*     DRMAA errors, and frees the job id list.
*
*  INPUTS
*     lList **id_list   - List of job ids to delete.  Gets freed.
*     int action        - The action that caused this delete
*     bool delete_all   - Whether this call is deleting all jobs in the session
*
*  OUTPUT
*     dstring *diag - returns diagnosis information - on error
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCES on success,
*           DRMAA error code on error.
*
*  NOTES
*     MT-NOTES: do_gdi_delete() is MT safe
*******************************************************************************/
static int do_gdi_delete (lList **id_list, int action, bool delete_all,
                          dstring *diag)
{
   lList *alp = NULL;
   lListElem *aep = NULL;

   DENTER (TOP_LAYER, "do_gdi_delete");

   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_DEL, id_list, NULL, NULL);
   *id_list = lFreeList(*id_list);

   for_each (aep, alp) {
      int status = lGetUlong(aep, AN_status);
      
   /* If we're doing a bulk delete (i.e. deleting all jobs in the session), we
    * have a problem in that the list we have of the jobs in out session could
    * be out of sync with reality.  That means we may try to delete a job that
    * no longer exists.  Since we're just trying to kill all the jobs, it's not
    * an error if the job doesn't exist when we try to delete it.  Therefore,
    * if we see such as error, we ignore it.  Otherwise, a busy system will
    * return a DRMAA_ERRNO_INVALID_JOB error by every control(ALL, TERM). */
      if ((status != STATUS_OK) && !(delete_all && (status == STATUS_EEXIST))) {
         int ret = japi_gdi_control_error2japi_error(aep, diag, action);
         alp = lFreeList(alp);
         DEXIT;
         return ret;
      }
   }

   alp = lFreeList(alp);

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** JAPI/japi_stop_event_client() ******************************************
*  NAME
*     japi_stop_event_client() -- stops the event client
*
*  SYNOPSIS
*     int japi_stop_event_client(void) 
*
*  FUNCTION
*     Uses the Event Master interface to send a SHUTDOWN event to the event
*     client.
*
*  RESULT
*     int - 0 = OK, 1 = Error
*
*  NOTES
*     MT-NOTES: japi_stop_event_client() is MT safe (assumptions)
*******************************************************************************/
static int japi_stop_event_client (void)
{
   lList *alp = NULL;
   lList *id_list = NULL;
   char id_string[25];

   DENTER(TOP_LAYER, "stop_event_client");

   DPRINTF (("Requesting that GDI kill our event client.\n"));
   snprintf(id_string, sizeof(id_string)-1, u32, japi_ec_id);
   lAddElemStr(&id_list, ID_str, id_string, ID_Type);
   alp = gdi_kill(id_list, uti_state_get_default_cell(), 0, EVENTCLIENT_KILL);
   id_list = lFreeList(id_list);
   alp = lFreeList(alp);
   
   DEXIT;
   return 0;
}
