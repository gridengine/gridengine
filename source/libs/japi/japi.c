#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>


#define JOIN_ECT

#include "drmaa.h"
#include "japi.h"

/* CULL */
#include "cull_list.h"

/* self */
#include "japiP.h"

/* RMON */
#include "sgermon.h"

/* UTI */
#include "sge_string.h"
#include "sge_prog.h"
#include "sge_time.h"
#include "sge_log.h"

/* COMMLIB */
#include "commlib.h"


/* EVC */
#include "sge_event_client.h"

/* GDI */
#include "sge_gdi.h"
#include "gdi_tsm.h"
#include "sge_gdiP.h"

/* SGEOBJ */
#include "sge_event.h"
#include "sge_job.h"
#include "sge_queue.h"
#include "sge_path_alias.h"

#include "sge_range.h"
#include "sge_object.h"
#include "sge_feature.h"

/* OBJ */
#include "sge_japiL.h"
#include "sge_varL.h"
#include "sge_identL.h"
#include "sge_stringL.h"
#include "sge_jobL.h"
#include "sge_ja_taskL.h"
#include "sge_answerL.h"
#include "sge_answer.h"


int delay_after_submit;
int ec_return_value;

static void *implementation_thread(void *);
static void japi_standard_error(int drmaa_errno, dstring *ds);
static int japi_drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t *drmaa_jt, 
   int is_bulk, int start, int end, int step, dstring *diag);
static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, lListElem *jt, int is_bulk, 
   int nm, const char *attribute_key, dstring *diag);
static drmaa_attr_values_t *japi_allocate_string_vector(int type); 
static int japi_parse_jobid(const char *jobid_str, u_long32 *jobid, u_long32 *taskid, 
   bool *is_array, dstring *diag);
static int japi_send_job(lListElem *job, u_long32 *jobid, dstring *diag);
static int japi_add_job(u_long32 jobid, u_long32 start, u_long32 end, u_long32 incr, 
      bool is_array, dstring *diag);


static int japi_synchronize_retry(bool sync_all, const char *job_ids[], bool dispose);
static int japi_synchronize_all_retry(bool dispose);
static int japi_synchronize_jobids_retry(const char *jobids[], bool dispose);
static int japi_wait_retry(lList *japi_job_list, int wait4any, u_long32 jobid, u_long32 taskid, 
   bool is_array_task, u_long32 *wjobidp, u_long32 *wtaskidp, bool *wis_task_arrayp);

static pthread_t event_client_thread;

static pthread_once_t japi_once_control = PTHREAD_ONCE_INIT;

/* ------------------------------------- 

   japi_session is used to control drmaa calls can 
   be used only between japi_init() and japi_exit()

   - japi_session is controlled by japi_session_mutex
*/

enum { 
   JAPI_SESSION_ACTIVE,       
   JAPI_SESSION_INACTIVE    
};
int japi_session = JAPI_SESSION_INACTIVE;
/* guards access to japi_session global variable */
static pthread_mutex_t japi_session_mutex;

#define JAPI_LOCK_SESSION()      japi_lock_mutex("SESSION", SGE_FUNC, &japi_session_mutex)
#define JAPI_UNLOCK_SESSION()    japi_unlock_mutex("SESSION", SGE_FUNC, &japi_session_mutex)


/* ------------------------------------- 

   japi_ec_state is used for synchronizing
   with startup of the event client thread 

   could also used to synchronize shutdown (?)

*/

enum { 
   JAPI_EC_DOWN,
   JAPI_EC_UP,
   JAPI_EC_FINISHING,
   JAPI_EC_FAILED
};

int japi_ec_state = JAPI_EC_DOWN;
u_long32 japi_ec_id = 0;

/* guards access to japi_ec_state global variable */
static pthread_mutex_t japi_ec_state_mutex = PTHREAD_MUTEX_INITIALIZER;

/* needed in japi_init() to allow waiting for event 
   client thread being up and running */
static pthread_cond_t japi_ec_state_starting_cv = PTHREAD_COND_INITIALIZER;

/* -------------------------------------

    The Master_japi_job_list contains information 
    about all jobs state of this session 

*/
lList *Master_japi_job_list = NULL;

/* guards access to Master_japi_job_list global variable */
static pthread_mutex_t Master_japi_job_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define JAPI_LOCK_JOB_LIST()     japi_lock_mutex("JOB LIST", SGE_FUNC, &Master_japi_job_list_mutex)
#define JAPI_UNLOCK_JOB_LIST()   japi_unlock_mutex("JOB LIST", SGE_FUNC, &Master_japi_job_list_mutex)

/* this condition is raised each time when a job/task is finshed */
static pthread_cond_t Master_japi_job_list_finished_cv = PTHREAD_COND_INITIALIZER;

/* ------------------------------------- 

   japi_threads_in_session is a counter indicating the 
   number of threads depending upon session state 
   (Master_japi_job_list). In case of a japi_exit() this 
   state must remain valid until the last thread has finished 
   it's operation.

*/


/* kind of a reference counter indicating the number of theads depending 
   on consisten session state information */
int japi_threads_in_session = 0;

/* guards access to threads_in_session global variable */
static pthread_mutex_t japi_threads_in_session_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ------------------------------------- 

   japi_session_key is a string key used at event client 
   registration to select those job events that are related
   to the JAPI session. Write access to japi_session_key happens
   only in mutex guarded japi_init(). So far no mutex is needed
   for japi_session_key.

*/
char *japi_session_key = NULL;

#define JAPI_LOCK_REFCOUNTER()   japi_lock_mutex("REFCOUNTER", SGE_FUNC, &japi_threads_in_session_mutex)
#define JAPI_UNLOCK_REFCOUNTER() japi_unlock_mutex("REFCOUNTER", SGE_FUNC, &japi_threads_in_session_mutex)

/* this condition is raised when a threads_in_session becomes 0 */
static pthread_cond_t japi_threads_in_session_cv = PTHREAD_COND_INITIALIZER;

/* ------------------------------------- */

/* these non vector job template attributes are supported */
const char *japi_supported_nonvector[] = {
   DRMAA_REMOTE_COMMAND,
   DRMAA_JS_STATE,
   DRMAA_WD,
   DRMAA_JOB_NAME,
   DRMAA_INPUT_PATH,
   DRMAA_OUTPUT_PATH,
   DRMAA_ERROR_PATH,
   DRMAA_JOIN_FILES,
#if 0
   DRMAA_DEADLINE_TIME,
   DRMAA_WCT_HLIMIT,
   DRMAA_WCT_SLIMIT,
   DRMAA_DURATION_HLIMIT,
   DRMAA_DURATION_SLIMIT,
   DRMAA_JOB_CATEGORY,
   DRMAA_NATIVE_SPECIFICATION,
   DRMAA_BLOCK_EMAIL,
   DRMAA_START_TIME,
   DRMAA_TRANSFER_FILES,
#endif
   NULL
};

/* these vector job template attributes are supported */
const char *japi_supported_vector[] = {
   DRMAA_V_ARGV,
#if 0
   DRMAA_V_ENV,
   DRMAA_V_EMAIL,
#endif
   NULL
};

static void japi_use_library_signals(void)
{
   /* simply block SIGPIPE */
   sigset_t block;
   sigemptyset(&block);
   sigaddset(&block, SIGPIPE);
   sigprocmask(SIG_BLOCK, &block, NULL);
}


static int is_supported(const char *name, const char *supported_list[])
{
   int i;
   for (i=0; supported_list[i]; i++) {
      if (!strcmp(name, supported_list[i]))
         return 1;
   }
   return 0;
}

static void japi_once_init(void)
{
   /* enable rmon monitoring */
   rmon_mopen(NULL, 0, "japilib");
}

/****** japi/japi_standard_error() *********************************************
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
static void japi_standard_error(int drmaa_errno, dstring *diag)
{
   if (diag)
      sge_dstring_copy_string(diag, japi_strerror(drmaa_errno));
}

static void japi_lock_mutex(const char *mutex_name, const char *func, pthread_mutex_t *mutex)
{
   DPRINTF(("%s: %s() try to obtain mutex\n", mutex_name, func));
   pthread_mutex_lock(mutex);
   DPRINTF(("%s: %s() got mutex\n", mutex_name, func));
}
static void japi_unlock_mutex(const char *mutex_name, const char *func, pthread_mutex_t *mutex)
{
   DPRINTF(("%s: %s() releasing mutex\n", mutex_name, func));
   pthread_mutex_unlock(mutex);
   DPRINTF(("%s: %s() released mutex\n", mutex_name, func));
}

static void japi_inc_threads(const char *SGE_FUNC)
{
   int LAYER = TOP_LAYER;
   JAPI_LOCK_REFCOUNTER();
   japi_threads_in_session++;
   DPRINTF(("%s(): japi_threads_in_session++ %d\n", SGE_FUNC, japi_threads_in_session));
   JAPI_UNLOCK_REFCOUNTER();
}

static void japi_dec_threads(const char *SGE_FUNC)
{
   int LAYER = TOP_LAYER;
   JAPI_LOCK_REFCOUNTER();
   if (--japi_threads_in_session == 0)
      pthread_cond_signal(&japi_threads_in_session_cv);
   DPRINTF(("%s(): japi_threads_in_session-- %d\n", SGE_FUNC, japi_threads_in_session));
   JAPI_UNLOCK_REFCOUNTER();
}


/****** japi/japi_init_mt() ****************************************************
*  NAME
*     japi_init_mt() -- Per thread library initialization
*
*  SYNOPSIS
*     static int japi_init_mt(dstring *diag) 
*
*  FUNCTION
*     Do all per thread initialization required for libraries JAPI builds 
*     upon.
*
*  OUTPUT
*     dstring *diag - returns diagnosis information
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTES: japi_init_mt() is MT safe
*******************************************************************************/
static int japi_init_mt(dstring *diag)
{
   int gdi_errno;
   lList *alp = NULL;
   
   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   gdi_errno = sge_gdi_setup("japi", &alp);
   if (gdi_errno!=AE_OK && gdi_errno != AE_ALREADY_SETUP) {
      answer_to_dstring(lFirst(alp), diag);
      lFreeList(alp);
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }
   log_state_set_log_gui(0);

   /* current major assumptions are

      - code is not compiled -DCOMMCOMPRESS
      - code is not compiled with -DCRYPTO
      - code is not compiled with -DKERBEROS
      - if code is compiled with -SECURE then
        only non secure communication may be used 
      - neither AFS nor DCE/KERBEROS security may be used
   */
   if (feature_is_enabled(FEATURE_CSP_SECURITY)) {
      sge_dstring_copy_string(diag, "error: secure mode not supported\n");
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* as long as signal handling is not restored japi_init_mt() is
      good place to install library signal handling */  
   japi_use_library_signals();

   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/japi_init() ****************************************************
*  NAME
*     japi_init() -- Initialize DRMAA API library
*
*  SYNOPSIS
*
*  FUNCTION
*     Initialize DRMAA API library and create a new DRMAA Session. 'Contact'
*     is an implementation dependent string which may be used to specify
*     which DRM system to use. This routine must be called before any
*     other DRMAA calls, except for japi_version().
*     If 'contact' is NULL, the default DRM system will be used.
*     Initializes internal data structures and registers with qmaster
*     using the event client mechanisms.
*
*  INPUTS
*  RESULT
* 
*  MUTEXES
*      japi_session_mutex -> japi_ec_state_mutex
*
*  NOTES
*      MT-NOTE: japi_init() is MT safe
*******************************************************************************/
int japi_init(const char *contact, dstring *diag)
{
   int i;
   int ret;
   int *value;

   DENTER(TOP_LAYER, "japi_init");

   pthread_once(&japi_once_control, japi_once_init);

   JAPI_LOCK_SESSION();
   if (japi_session == JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_ALREADY_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_ALREADY_ACTIVE_SESSION;
   }

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_INTERNAL_ERROR, diag);
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* intermediate solution */
   japi_session_key = getenv("SGE_JAPI_SESSION");

   /* read in library session data of former session if any */

   /* spawn implementation thread implementation_thread() */
   DPRINTF(("spawning event client thread\n"));

   {
      pthread_attr_t attr;
      pthread_attr_init(&attr);
#ifndef JOIN_ECT
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#else
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif
      if ((i=pthread_create(&event_client_thread, &attr, implementation_thread, (void *) NULL))) {
         JAPI_UNLOCK_SESSION();
         if (diag) 
            sge_dstring_sprintf(diag, "error: couldn't create event client thread: %d %s\n", i, strerror(errno));
         DEXIT;
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }
      pthread_attr_destroy(&attr);
   }

   /* wait until event client id is operable or gave up passed by event client thread */
   DPRINTF(("waiting for JAPI_EC_UP ...\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);   
   if (japi_ec_state == JAPI_EC_DOWN) {
      pthread_cond_wait(&japi_ec_state_starting_cv, &japi_ec_state_mutex);
   }
   if (japi_ec_state == JAPI_EC_UP)
      ret = DRMAA_ERRNO_SUCCESS;
   else { /* JAPI_EC_FAILED */
      ret = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
      japi_standard_error(DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, diag);
   }
   pthread_mutex_unlock(&japi_ec_state_mutex);   
   DPRINTF(("... got JAPI_EC_UP\n"));

#ifdef JOIN_ECT
   if (ret == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
      if (pthread_join(event_client_thread, (void *)&value)) {
         DPRINTF(("japi_init(): pthread_join returned %d\n", *value));
      }
   }
#endif

   japi_session = JAPI_SESSION_ACTIVE;
   JAPI_UNLOCK_SESSION();

   DEXIT;
   return ret;
}



/****** DRMAA/japi_exit() ****************************************************
*  NAME
*     japi_exit() -- Shutdown DRMAA API library
*
*  SYNOPSIS
*
*  FUNCTION
*     Disengage from DRMAA library and allow the DRMAA library to perform
*     any necessary internal clean up.
*     This routine ends this DRMAA Session, but does not effect any jobs (e.g.,
*     queued and running jobs remain queued and running).
*
*  INPUTS
*  RESULT
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: japi_exit() is MT safe
*******************************************************************************/
int japi_exit(dstring *diag)
{
   int cl_errno;

   DENTER(TOP_LAYER, "japi_exit");

   DPRINTF(("entering japi_exit() at "u32"\n", sge_get_gmt()));

   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   /* 
    * notify event client about shutdown
    *
    * Currently this is done by setting japi_ec_state accordingly.
    * This is very simple, but has the drawback that it can take
    * up to 'ed_time' seconds (see implementation_thread()) before
    * event client thread ec_get() times out and japi_ec_state is
    * read. 
    * 
    * There are (at least) two possible solutions:
    * Either the event client threads commlib receive timeout must 
    * be way shorter or kind of 'qconf -kec id' must be used in 
    * japi_exit() to notfiy event client thread via qmaster 
    */
#if 1
   {  lList *alp, *id_list = NULL;
      char id_string[25];
      snprintf(id_string, sizeof(id_string)-1, u32, japi_ec_id);
      lAddElemStr(&id_list, ID_str, id_string, ID_Type);
      alp = gdi_kill(id_list, uti_state_get_default_cell(), 0, EVENTCLIENT_KILL);
      id_list = lFreeList(id_list);
      alp = lFreeList(alp);
   }
#endif

   DPRINTF(("notify event client about shutdown\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);
   japi_ec_state = JAPI_EC_FINISHING;
   pthread_mutex_unlock(&japi_ec_state_mutex);

   /* signal all application threads waiting for a job to finish */
   pthread_cond_broadcast(&Master_japi_job_list_finished_cv);

#ifdef JOIN_ECT
   {
      int *value;
      int i;
      i = pthread_join(event_client_thread, (void *)&value); 
      DPRINTF(("japi_exit(): value = %d pthread_join returned %d: %s\n", *value, i, strerror(errno)));
   }
#endif

   {
      /* do not destroy session state until last japi call 
         depending on it is finished */
      JAPI_LOCK_REFCOUNTER();
      while (japi_threads_in_session > 0) {
          pthread_cond_wait(&japi_threads_in_session_cv, &japi_threads_in_session_mutex);
      }
      Master_japi_job_list = lFreeList(Master_japi_job_list);
      JAPI_UNLOCK_REFCOUNTER();
   }

   japi_ec_state = JAPI_EC_DOWN;
   japi_session = JAPI_SESSION_INACTIVE;
   JAPI_UNLOCK_SESSION();

   DPRINTF(("japi_exit(): event client thread joined at "u32"\n", sge_get_gmt()));

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
   if ((cl_errno=leave_commd())!=CL_OK) {
      if (cl_errno != CL_NOTENROLLED) {
         sge_dstring_sprintf(diag, "leave_commd() failed: %s", cl_errstr(cl_errno));
         DEXIT;
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_allocate_string_vector() *************************************
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
*******************************************************************************/
static drmaa_attr_values_t *japi_allocate_string_vector(int type) 
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

/****** japi/japi_string_vector_get_first() ************************************
*  NAME
*     japi_string_vector_get_first() -- Return first entry of a string vector
*
*  SYNOPSIS
*     int japi_string_vector_get_first(drmaa_attr_values_t* iter, dstring 
*     *val) 
*
*  FUNCTION
*     The first entry of a string vector is returned. This function can be
*     to rewind the string vector. DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE is 
*     returned for an empty string vector.
*
*  INPUTS
*     drmaa_attr_values_t* iter - the string vector
*     dstring *val                - destination 
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_string_vector_get_first() is MT safe
*******************************************************************************/
int japi_string_vector_get_first(drmaa_attr_values_t* iter, dstring *val)
{
   if (!iter)
      return DRMAA_ERRNO_INVALID_ARGUMENT;

   /* (re)init iterator */
   switch (iter->iterator_type) {
   case JAPI_ITERATOR_BULK_JOBS:
      iter->it.ji.next_pos = iter->it.ji.start;   
      break;
   case JAPI_ITERATOR_STRINGS:
      iter->it.si.next_pos = lFirst(iter->it.si.strings);   
      break;
   default:
      break;
   }
   return japi_string_vector_get_next(iter, val);
}

/****** japi/japi_string_vector_get_next() *************************************
*  NAME
*     japi_string_vector_get_next() -- Return next entry of a string vector
*
*  SYNOPSIS
*     int japi_string_vector_get_next(drmaa_attr_values_t* iter, dstring 
*     *val) 
*
*  FUNCTION
*     The next entry of a string vector is returned. 
*     DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE is returned for an empty string 
*     vector.
*
*  INPUTS
*     drmaa_attr_values_t* iter - The string vector
*     dstring *val                - destination
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_string_vector_get_next() is MT safe
*******************************************************************************/
int japi_string_vector_get_next(drmaa_attr_values_t* iter, dstring *val)
{
   if (!iter)
      return DRMAA_ERRNO_INVALID_ARGUMENT;
  
   switch (iter->iterator_type) {
   case JAPI_ITERATOR_BULK_JOBS:
      if (iter->it.ji.next_pos > iter->it.ji.end) {
         return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
      }
      if (val)
         sge_dstring_sprintf(val, "%ld.%d", iter->it.ji.jobid, iter->it.ji.next_pos);
      iter->it.ji.next_pos += iter->it.ji.incr;
      return DRMAA_ERRNO_SUCCESS;
   case JAPI_ITERATOR_STRINGS:
      if (!iter->it.si.next_pos) {
         return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
      } 
      if (val)
         sge_dstring_copy_string(val, lGetString(iter->it.si.next_pos, ST_name));
      iter->it.si.next_pos = lNext(iter->it.si.next_pos);
      return DRMAA_ERRNO_SUCCESS;
   default:
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }
}

/****** japi/japi_delete_string_vector() ***************************************
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
*  RESULT
*     void - 
*
*  NOTES
*     MT-NOTE: japi_delete_string_vector() is MT safe
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

/****** japi/japi_allocate_job_template() *************************************
*  NAME
*     japi_allocate_job_template() -- Allocate a new job template. 
*
*  SYNOPSIS
*
*  FUNCTION
*  RESULT
*
*  NOTES
*      MT-NOTE: japi_allocate_job_template() is MT safe
*******************************************************************************/
int japi_allocate_job_template(drmaa_job_template_t **jtp, dstring *diag)
{
   DENTER(TOP_LAYER, "japi_allocate_job_template");

   if (!jtp) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   } 
 
   *jtp = (drmaa_job_template_t *)malloc(sizeof(drmaa_job_template_t));
   (*jtp)->strings = (*jtp)->string_vectors = NULL;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_delete_job_template() ***************************************
*  NAME
*     japi_delete_job_template() -- Deallocate a job template. This routine has no effect on jobs.
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*     drmaa_job_template_t *jt - job template to be deleted
*
*  RESULT
*
*  NOTES
*      MT-NOTE: japi_delete_job_template() is MT safe
*******************************************************************************/
int japi_delete_job_template(drmaa_job_template_t *jt, dstring *diag)
{
   DENTER(TOP_LAYER, "japi_delete_job_template");

   if (!jt) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   } 
 
   jt->strings = lFreeList(jt->strings);
   jt->string_vectors = lFreeList(jt->string_vectors);
   free(jt); 
   jt = NULL;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** japi/japi_set_attribute() *********************************************
*  NAME
*     japi_set_attribute() -- Set non vector attribute in job template
*
*  SYNOPSIS
*
*  FUNCTION
*     Adds ('name', 'value') pair to list of attributes in job template 'jt'.
*     Only non-vector attributes may be passed.
*
*  INPUTS
*     drmaa_job_template_t *jt - job template
*     const char *name   - name 
*     const char *value  - value
*
*  RESULT
*
*  NOTES
*      MT-NOTE: japi_set_attribute() is MT safe
*******************************************************************************/
int japi_set_attribute(drmaa_job_template_t *jt, const char *name, const char *value, dstring *diag)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "japi_set_attribute");
      
   if (!jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   if (is_supported(name, japi_supported_nonvector)) {
      /* verify value */

      /* join files must be either 'y' or 'n' */
      if (!strcmp(name, DRMAA_JOIN_FILES)) {
         if (strlen(value)!=1 || (value[0] != 'y' && value[0] != 'n' )) {
            if (diag) 
               sge_dstring_sprintf(diag, "attribute "SFQ" must be either "SFQ" or "SFQ"\n", 
                     DRMAA_JOIN_FILES, "y", "n");
            DEXIT;
            return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
         }
      }

      /* submission state must be either active or hold */
      if (!strcmp(name, DRMAA_JS_STATE)) {
         if (strcmp(value, DRMAA_SUBMISSION_STATE_ACTIVE)
            && strcmp(value, DRMAA_SUBMISSION_STATE_HOLD)) {
            if (diag) 
               sge_dstring_sprintf(diag, "attribute "SFQ" must be either "SFQ" or "SFQ"\n", 
                     DRMAA_JS_STATE, DRMAA_SUBMISSION_STATE_ACTIVE, DRMAA_SUBMISSION_STATE_HOLD);
            DEXIT;
            return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
         }
      }
   }
  
   /* add or replace attribute */ 
   if ((ep = lGetElemStr(jt->strings, VA_variable, name))) {
      lSetString(ep, VA_value, value);
   } else {
      ep = lAddElemStr(&(jt->strings), VA_variable, name, VA_Type);
      lSetString(ep, VA_value, value);
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** japi/japi_get_attribute() **********************************************
*  NAME
*     japi_get_attribute() -- Return job template attribute value.
*
*  SYNOPSIS
*     int japi_get_attribute(drmaa_job_template_t *jt, const char *name, 
*     dstring *val, dstring *diag) 
*
*  FUNCTION
*     If 'name' is an existing non-vector attribute name in the job 
*     template 'jt', then the value of 'name' is returned; otherwise, 
*     DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE is returned.
*
*  INPUTS
*     drmaa_job_template_t *jt - the job template
*     const char *name         - the attribute name
*     dstring *val             - value destination
*     dstring *diag            - diagnosis information
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_get_attribute() is MT safe
*******************************************************************************/
int japi_get_attribute(drmaa_job_template_t *jt, const char *name, 
      dstring *val, dstring *diag)
{
   lListElem *va;

   /* search name in string_vectors */
   if (!(va = lGetElemStr(jt->strings, VA_variable, name))) {
      return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
   }
  
   sge_dstring_copy_string(val, lGetString(va, VA_value));
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_set_vector_attribute() ***************************************
*  NAME
*     japi_set_vector_attribute() -- Set vector attribute in job template
*
*  SYNOPSIS
*     int japi_set_vector_attribute(drmaa_job_template_t *jt, const char *name, 
*     char *value[], dstring *diag) 
*
*  FUNCTION
*     Adds ('name', 'values') pair to list of vector attributes in job template 
*     'jt'. Only vector attributes may be passed. 
*
*  INPUTS
*     drmaa_job_template_t *jt - job template
*     const char *name         - attribute name
*     char *value[]            - array of string values
*     dstring *diag            - diagnosis information
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*      MT-NOTE: japi_set_vector_attribute() is MT safe
*******************************************************************************/
int japi_set_vector_attribute(drmaa_job_template_t *jt, const char *name, 
      const char *value[], dstring *diag)
{
   int i;
   lList *lp;
   lListElem *ep, *sep;

   DENTER(TOP_LAYER, "japi_set_vector_attribute");

   if (!jt) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   if ((ep = lGetElemStr(jt->string_vectors, NSV_name, name)))
      lSetList(ep, NSV_strings, NULL);
   else
      ep = lAddElemStr(&(jt->string_vectors), NSV_name, name, NSV_Type);
 
   lp = lCreateList(NULL, ST_Type);
   for (i=0; value[i]; i++) {
      sep = lCreateElem(ST_Type);
      lSetString(sep, ST_name, value[i]);
      lAppendElem(lp, sep);
   }
   lSetList(ep, NSV_strings, lp);

   return DRMAA_ERRNO_SUCCESS;
}


/****** japi/japi_get_vector_attribute() ***************************************
*  NAME
*     japi_get_vector_attribute() -- ??? 
*
*  SYNOPSIS
*     int japi_get_vector_attribute(drmaa_job_template_t *jt, const char *name, 
*     drmaa_attr_values_t **values, dstring *diag) 
*
*  FUNCTION
*     If 'name' is an existing vector attribute name in the job template 'jt',
*     then the values of 'name' are returned; otherwise, DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE 
*     is returned.
*
*  INPUTS
*     drmaa_job_template_t *jt       - the job template
*     const char *name               - the vector attribute name 
*     drmaa_attr_values_t **values - destination string vector 
*     dstring *diag                  - diagnosis information
*
*  RESULT
*     int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_get_vector_attribute() is MT safe
*
*******************************************************************************/
int japi_get_vector_attribute(drmaa_job_template_t *jt, const char *name, drmaa_attr_values_t **values, dstring *diag)
{
   lListElem *nsv;
   drmaa_attr_values_t *iter;

   /* search name in string_vectors */
   if (!(nsv = lGetElemStr(jt->string_vectors, NSV_name, name))) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, diag);
      return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
   }
 
   /* allocate iterator */
   if (!(iter=japi_allocate_string_vector(JAPI_ITERATOR_STRINGS))) {
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* copy job template attributes into iterator */ 
   iter->it.si.strings = lCopyList(NULL, lGetList(nsv, NSV_strings));
   if (!iter->it.si.strings) {
      japi_delete_string_vector(iter);
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* initialize iterator */
   iter->it.si.next_pos = lFirst(iter->it.si.strings);

   *values = iter;
   return DRMAA_ERRNO_SUCCESS;
}


/* 
 * Returns the set of supported attribute names whose associated   
 * value type is String. This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 *
 *      MT-NOTE: japi_get_attribute_names() is MT safe
 */
int japi_get_attribute_names(drmaa_attr_names_t **values, dstring *diag)
{
   return DRMAA_ERRNO_SUCCESS;
}

/*
 * Returns the set of supported attribute names whose associated 
 * value type is String Vector.  This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 *
 *      MT-NOTE: japi_get_vector_attribute_names() is MT safe
 */
int japi_get_vector_attribute_names(drmaa_attr_names_t **values, dstring *diag)
{
   return DRMAA_ERRNO_SUCCESS;
}


/****** japi/japi_drmaa_path2sge_job() *****************************************
*  NAME
*     japi_drmaa_path2sge_job() -- Transform a DRMAA job path into SGE 
*                                  counterpart
*
*  SYNOPSIS
*     static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, 
*     lListElem *jt, int is_bulk, int nm, const char *attribute_key, dstring 
*     *diag) 
*
*  FUNCTION
*     Transform a DRMAA job path into SGE counterpart. The following 
*     substitutions are performed
*         
*        $drmaa_hd_ph$     --> $HOME
*        $drmaa_wd_ph$     --> $CWD
*        $drmaa_incr_ph$   --> $TASK_ID   
*
*     The $drmaa_incr_ph$ substitutions are performed only for bulk jobs
*     otherwise submission fails.
*
*  INPUTS
*     drmaa_job_template_t *drmaa_jt - the DRMAA job template
*     lListElem *jt                  - the destination Grid Engine job 
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*     int nm                         - CULL field name used for path  
*     const char *attribute_key      - The DRMAA job template keyword for this
*                                      path
*     dstring *diag                  - diagnosis inforation
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_drmaa_path2sge_job() is MT safe
*******************************************************************************/
static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, lListElem *jt, 
   int is_bulk, int nm, const char *attribute_key, dstring *diag)
{
   lListElem *ep;

   lSetList(jt, nm, NULL);
   if ((ep=lGetElemStr(drmaa_jt->strings, VA_variable, attribute_key ))) {
      dstring ds = DSTRING_INIT;
      const char *p, *value = lGetString(ep, VA_value);
      /* substitute DRMAA placeholder with grid engine counterparts */

      /* home directory and working directory placeholder only recognized at the begin */
      if (!strncmp(value, DRMAA_PLACEHOLDER_HD, strlen(DRMAA_PLACEHOLDER_HD))) {
         sge_dstring_copy_string(&ds, "$HOME");
         value += strlen(DRMAA_PLACEHOLDER_HD);
      } else if (!strncmp(value, DRMAA_PLACEHOLDER_WD, strlen(DRMAA_PLACEHOLDER_WD))) {
         sge_dstring_copy_string(&ds, "$CWD"); /* not yet supported by Grid Engine */
         value += strlen(DRMAA_PLACEHOLDER_WD);
      }

      /* bulk job index placeholder recognized at any position */
      if ((p=strstr(value, DRMAA_PLACEHOLDER_INCR))) {
         
         if (!is_bulk) {
            /* reject incr placeholder for non-array jobs */
            sge_dstring_free(&ds);
            jt = lFreeElem(jt);   
            sge_dstring_sprintf(diag, "increment placeholder "SFQ" only in pathes "
                  "for bulk jobs\n", DRMAA_PLACEHOLDER_INCR);
            return DRMAA_ERRNO_DENIED_BY_DRM;
         }

         if (p != value) {
            sge_dstring_sprintf_append(&ds, "%.*s", p-value, value);
            value = p;
         }
         sge_dstring_append(&ds, "$TASK_ID");
         value += strlen(DRMAA_PLACEHOLDER_INCR);
      }
   
      /* rest of the path */
      sge_dstring_append(&ds, value);
      DPRINTF(("%s = \"%s\"\n", lNm2Str(nm), sge_dstring_get_string(&ds)));
      lAddSubStr(jt, PN_path, sge_dstring_get_string(&ds), nm, PN_Type);
      sge_dstring_free(&ds);
   }

   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_send_job() ***************************************************
*  NAME
*     japi_send_job() -- Send job to qmaster using GDI
*
*  SYNOPSIS
*     int japi_send_job(lListElem *job, u_long32 *jobid, dstring *diag) 
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
*     Note that the job passed is also free()'d by japi_send_job().
*     MT-NOTE: japi_send_job() is MT safe
*******************************************************************************/
static int japi_send_job(lListElem *job, u_long32 *jobid, dstring *diag)
{
   lList *job_lp, *alp;
   lListElem *aep;

   DENTER(TOP_LAYER, "japi_send_job");

   job_lp = lCreateList(NULL, JB_Type);
   lAppendElem(job_lp, job);

   /* use GDI to submit job for this session */
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_ADD|SGE_GDI_RETURN_NEW_VERSION, &job_lp, NULL, NULL);

   /* reinitialize 'job' with pointer to new version from qmaster */
   if ((job = lFirst(job_lp)))
      *jobid = lGetUlong(job, JB_job_number);
   job_lp = lFreeList(job_lp);

   if (!(aep = lFirst(alp)) || !job) {
      alp = lFreeList(alp);
      sge_dstring_copy_string(diag, "sge_gdi() failed returning answer list");
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }
   
   {
      u_long32 quality;
      quality = lGetUlong(aep, AN_quality);
      if (quality == ANSWER_QUALITY_ERROR) {
         u_long32 answer_status = lGetUlong(aep, AN_status);
         answer_to_dstring(aep, diag);
         alp = lFreeList(alp);

         switch (answer_status) {
         case STATUS_NOQMASTER:
         case STATUS_NOCOMMD:
            DPRINTF(("answer status indicated qmaster/commd down: "u32"\n", answer_status));
            DEXIT;
            return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
         default:
            DEXIT;
            return DRMAA_ERRNO_DENIED_BY_DRM;
         }
      }
      alp = lFreeList(alp);
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** japi/japi_drmaa_job2sge_job() ******************************************
*  NAME
*     japi_drmaa_job2sge_job() -- convert a DRMAA job template into the Grid 
*                                 Engine counterpart 
*
*  SYNOPSIS
*     static int japi_drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t 
*     *drmaa_jt, int is_bulk, int start, int end, int step, dstring *diag) 
*
*  FUNCTION
*     All DRMAA job template attributes are translated into Grid Engine 
*     job attributes.
*
*  INPUTS
*     drmaa_job_template_t *drmaa_jt - the DRMAA job template
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*     int start                      - start index for bulk jobs
*     int end                        - end index for bulk jobs
*     int step                       - increment for bulk jobs
*     dstring *diag                  - diagnosis information
*
*  OUTPUT
*     lListElem **jtp                - returns Grid Engine JB_Type job 
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_drmaa_job2sge_job() is MT safe
*
*******************************************************************************/
static int japi_drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t *drmaa_jt, 
   int is_bulk, int start, int end, int step, dstring *diag)
{
   lListElem *jt, *ep;
   int drmaa_errno;
   lList *alp = NULL;
   lList *path_alias = NULL;

   DENTER(TOP_LAYER, "japi_drmaa_job2sge_job");

   /* make JB_Type job description out of DRMAA job template */
   if (!(jt = lCreateElem(JB_Type))) {
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* remote command */
   if (!(ep=lGetElemStr(drmaa_jt->strings, VA_variable, DRMAA_REMOTE_COMMAND))) {
      jt = lFreeElem(jt);   
      sge_dstring_copy_string(diag, "job template must have \""DRMAA_REMOTE_COMMAND"\" attribute set");
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }
   lSetString(jt, JB_script_file, lGetString(ep, VA_value));

   /* init range of jobids and put all tasks in 'active' state */
   if (job_set_submit_task_ids(jt, start, end, step) ||
       job_initialize_id_lists(jt, NULL)) {
      jt = lFreeElem(jt);   
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   {
      u_long32 jb_now = lGetUlong(jt, JB_type);
      /* use always binary submission mode */
      JOB_TYPE_SET_BINARY(jb_now);
      if (is_bulk)
         JOB_TYPE_SET_ARRAY(jb_now);
      lSetUlong(jt, JB_type, jb_now);
   }

   /* job arguments */
   if ((ep=lGetElemStr(drmaa_jt->string_vectors, NSV_name, DRMAA_V_ARGV)))
      lSetList(jt, JB_job_args, lCopyList(NULL, lGetList(ep, NSV_strings)));

   /* job name */
   if ((ep=lGetElemStr(drmaa_jt->strings, VA_variable, DRMAA_JOB_NAME))) {
      lSetString(jt, JB_job_name, lGetString(ep, VA_value));
   } else {
      /* use command basename */
      const char *command = lGetString(jt, JB_script_file);
      lSetString(jt, JB_job_name, sge_basename(command, '/'));
   }

   /* join files */
   lSetBool(jt, JB_merge_stderr, FALSE);
   if ((ep=lGetElemStr(drmaa_jt->strings, VA_variable, DRMAA_JOIN_FILES))) {
      const char *value = lGetString(ep, VA_value);
      if (value[0] == 'y')
         lSetBool(jt, JB_merge_stderr, TRUE);
      else
         lSetBool(jt, JB_merge_stderr, FALSE);
   }

   /* working directory */
   lSetString(jt, JB_cwd, NULL);
   if ((ep=lGetElemStr(drmaa_jt->strings, VA_variable, DRMAA_WD))) {
      dstring ds = DSTRING_INIT;
      const char *p, *value = lGetString(ep, VA_value);
      /* substitute DRMAA placeholder with grid engine counterparts */

      /* home directory placeholder only recognized at the begin */
      if (!strncmp(value, DRMAA_PLACEHOLDER_HD, strlen(DRMAA_PLACEHOLDER_HD))) {
         sge_dstring_copy_string(&ds, "$HOME");
         value += strlen(DRMAA_PLACEHOLDER_HD);
      }

      /* bulk job index placeholder recognized at any position */
      if ((p=strstr(value, DRMAA_PLACEHOLDER_INCR))) {
         
         if (!is_bulk) {
            sge_dstring_free(&ds);
            jt = lFreeElem(jt);   
            japi_standard_error(DRMAA_ERRNO_DENIED_BY_DRM, diag);
            DEXIT;
            return DRMAA_ERRNO_DENIED_BY_DRM;
         }

         if (p != value) {
            sge_dstring_sprintf_append(&ds, "%.*s", p-value, value);
            value = p;
         }
         sge_dstring_append(&ds, "$TASK_ID");
         value += strlen(DRMAA_PLACEHOLDER_INCR);
      }
   
      /* rest of the path */
      sge_dstring_append(&ds, value);
      DPRINTF(("JB_cwd = \"%s\"\n", sge_dstring_get_string(&ds)));
      lSetString(jt, JB_cwd, sge_dstring_get_string(&ds));

      sge_dstring_free(&ds);
   }

   /* jobs input/output/error stream */
   if ((drmaa_errno = japi_drmaa_path2sge_job(drmaa_jt, jt, is_bulk, JB_stdout_path_list, 
         DRMAA_OUTPUT_PATH, diag))!=DRMAA_ERRNO_SUCCESS) {
      jt = lFreeElem(jt);   
      DEXIT;
      return drmaa_errno;
   }
   if ((drmaa_errno = japi_drmaa_path2sge_job(drmaa_jt, jt, is_bulk, JB_stderr_path_list, 
         DRMAA_ERROR_PATH, diag))!=DRMAA_ERRNO_SUCCESS) {
      jt = lFreeElem(jt);   
      DEXIT;
      return drmaa_errno;
   }
   if ((drmaa_errno = japi_drmaa_path2sge_job(drmaa_jt, jt, is_bulk, JB_stdin_path_list, 
         DRMAA_INPUT_PATH, diag))!=DRMAA_ERRNO_SUCCESS) {
      jt = lFreeElem(jt);   
      DEXIT;
      return drmaa_errno;
   }

   /* path aliasing */
   if (path_alias_list_initialize(&path_alias, &alp, uti_state_get_user_name(),
                                  uti_state_get_qualified_hostname())) {
      answer_to_dstring(lFirst(alp), diag);
      lFreeList(alp);
      jt = lFreeElem(jt);   
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }
  
   /* initialize standard enviromnent */ 
   job_initialize_env(jt, &alp, path_alias);
   if (alp) {
      answer_to_dstring(lFirst(alp), diag);
      lFreeList(alp);
      jt = lFreeElem(jt);   
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }

   /* average priority of 0 */
   lSetUlong(jt, JB_priority, BASE_PRIORITY);

   /* user hold state */
   if ((ep=lGetElemStr(drmaa_jt->strings, VA_variable, DRMAA_JS_STATE))) {
      const char *value = lGetString(ep, VA_value);
      DPRINTF(("processing %s = \"%s\"\n", DRMAA_JS_STATE, value));
      if (!strcmp(value, DRMAA_SUBMISSION_STATE_HOLD)) {
         lList *tmp_lp = NULL;

         /* move intial task id range into hold state list */
         lXchgList(jt, JB_ja_n_h_ids, &tmp_lp);
         lXchgList(jt, JB_ja_u_h_ids, &tmp_lp);
      }
   }

   *jtp = jt;
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_add_job() ****************************************************
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

/****** DRMAA/japi_run_job() ****************************************************
*  NAME
*     japi_run_job() -- Submit a job
*
*  SYNOPSIS
*
*  FUNCTION
*     Submit a job with attributes defined in the job template 'jt'.
*     The job identifier 'job_id' is a printable, NULL terminated string,
*     identical to that returned by the underlying DRM system.
*
*  INPUTS
* 
*
*  RESULT
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*      Master_japi_job_list_mutex
*      japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: japi_run_job() is MT safe
*******************************************************************************/
int japi_run_job(dstring *job_id, drmaa_job_template_t *jt, dstring *diag)
{
   lListElem *job;
   u_long32 jobid = 0;
   int drmaa_errno;

   DENTER(TOP_LAYER, "japi_run_job");

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

   /* per thread initialization */
   if (japi_init_mt(diag)!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* convert DRMAA job template into Grid Engine job template */
   if ((drmaa_errno=japi_drmaa_job2sge_job(&job, jt, 0, 1, 1, 1, diag))!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* tag job with JAPI session key */
   lSetString(job, JB_session, japi_session_key);

   /* send job to qmaster using GDI */
   drmaa_errno = japi_send_job(job, &jobid, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_send_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* this is just a hook for testing purposes 
      need this to enforce certain error conditions */
   if (delay_after_submit) {
      DPRINTF(("sleeping %d seconds\n", delay_after_submit));
      sleep(delay_after_submit);
      DPRINTF(("slept %d seconds\n", delay_after_submit));
   }

   /* add job arry to library session data */
   drmaa_errno = japi_add_job(jobid, 1, 1, 1, false, diag);
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

/****** DRMAA/japi_run_bulk_jobs() ****************************************************
*  NAME
*     japi_run_bulk_jobs() -- Submit a bulk of jobs
*
*  SYNOPSIS
*
*  FUNCTION
*     Submit a set of parametric jobs, dependent on the implied loop index, each
*     with attributes defined in the job template 'jt'.
*     The job identifiers 'job_ids' are all printable,
*     NULL terminated strings, identical to those returned by the underlying
*     DRM system. Nonnegative loop bounds are mandated to avoid file names
*     that start with minus sign like command line options.
*     The special index placeholder is a DRMAA defined string
*     drmaa_incr_ph == $incr_pl$
*     that is used to construct parametric job templates.
*     For example:
*     drmaa_set_attribute(pjt, "stderr", drmaa_incr_ph + ".err" ); (C++/java string syntax used)
*
*  INPUTS
*  RESULT
*
*  NOTES
*      MT-NOTE: japi_run_bulk_jobs() is MT safe
*******************************************************************************/
int japi_run_bulk_jobs(drmaa_attr_values_t **jobidsp, drmaa_job_template_t *jt, 
      int start, int end, int incr, dstring *diag)
{
   drmaa_attr_values_t *jobids;
   lListElem *job;
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
   
   /* convert DRMAA job template into Grid Engine job template */
   if ((drmaa_errno=japi_drmaa_job2sge_job(&job, jt, 1, start, end, incr, diag))!=DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_drmaa_job2sge_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* tag job with JAPI session key */
   lSetString(job, JB_session, japi_session_key);

   /* send job to qmaster using GDI */
   drmaa_errno = japi_send_job(job, &jobid, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_send_job() */
      DEXIT;
      return drmaa_errno;
   }

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


   /* add job arry to library session data */
   drmaa_errno = japi_add_job(jobid, start, end, incr, true, diag);
   japi_dec_threads(SGE_FUNC);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_add_job() */
      DEXIT;
      return drmaa_errno;
   }

   /* return jobids */
   *jobidsp = jobids;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/japi_control() ****************************************************
*  NAME
*     japi_control() -- Start, stop, restart, or kill jobs
*
*  SYNOPSIS
*
*  FUNCTION
*     Start, stop, restart, or kill the job identified by 'job_id'.
*     If 'job_id' is DRMAA_JOB_IDS_SESSION_ALL, then this routine
*     acts on all jobs *submitted* during this DRMAA session.
*     The legal values for 'action' and their meanings are:
*     DRMAA_CONTROL_SUSPEND: stop the job,
*     DRMAA_CONTROL_RESUME: (re)start the job,
*     DRMAA_CONTROL_HOLD: put the job on-hold,
*     DRMAA_CONTROL_RELEASE: release the hold on the job, and
*     DRMAA_CONTROL_TERMINATE: kill the job.
*     This routine returns once the action has been acknowledged by
*     the DRM system, but does not necessarily wait until the action
*     has been completed.
*
*  INPUTS
*  RESULT
*
*  NOTES
*      MT-NOTE: japi_control() is MT safe
*******************************************************************************/
int japi_control(const char *jobid, int action, dstring *diag)
{
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

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

enum {
   JAPI_WAIT_ALLFINISHED, /* there is nothing more to wait for */
   JAPI_WAIT_UNFINISHED,  /* there are still unfinished tasks  */
   JAPI_WAIT_FINISHED,    /* got a finished task               */
   JAPI_WAIT_INVALID      /* the specified task does not exist */
};


/****** DRMAA/japi_synchronize() ****************************************************
*  NAME
*     japi_synchronize() -- Synchronize with jobs to finish
*
*  SYNOPSIS
*
*  FUNCTION
*     Wait until all jobs specified by 'job_ids' have finished
*     execution. If 'job_ids' is DRMAA_JOB_IDS_SESSION_ALL, then this routine
*     waits for all jobs *submitted* during this DRMAA session. To prevent
*     blocking indefinitely in this call the caller could use timeout specifying
*     after how many seconds to time out in this call.
*     If the call exits before timeout all the jobs have been waited on
*     or there was an interrupt.
*     If the invocation exits on timeout, the return code is DRMAA_ERRNO_EXIT_TIMEOUT.
*     The caller should check system time before and after this call
*     in order to check how much time has passed.
*     Dispose parameter specifies how to treat reaping information:
*     True=1 "fake reap", i.e. dispose of the rusage data
*     False=0 do not reap
*
*  INPUTS
*  RESULT
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*     MT-NOTE: japi_synchronize() is MT safe
*******************************************************************************/
int japi_synchronize(const char *job_ids[], signed long timeout, bool dispose, dstring *diag)
{
   bool sync_all = false;
   int drmaa_errno, i;
   int wait_result;

   DENTER(TOP_LAYER, "japi_synchronize");

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

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
   for (i=0; job_ids[i]; i++)
      if (!strcmp(job_ids[i], DRMAA_JOB_IDS_SESSION_ALL)) 
         sync_all = true;
      else {
         if ((drmaa_errno=japi_parse_jobid(job_ids[i], NULL, NULL, NULL, 
                     diag))!=DRMAA_ERRNO_SUCCESS) {
            japi_dec_threads(SGE_FUNC);
            /* diag written by japi_parse_jobid() */
            DEXIT;
            return drmaa_errno;
         }
      }

   JAPI_LOCK_JOB_LIST();

   while ((wait_result=japi_synchronize_retry(sync_all, job_ids, dispose) == JAPI_WAIT_UNFINISHED)) {

      /* must return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE when event client 
         thread was shutdown during japi_wait() use japi_ec_state ?? */
      /* has japi_exit() been called meanwhile ? */
      pthread_mutex_lock(&japi_ec_state_mutex);
      if (japi_ec_state != JAPI_EC_UP) {
         pthread_mutex_unlock(&japi_ec_state_mutex);
         JAPI_UNLOCK_JOB_LIST();
         japi_dec_threads(SGE_FUNC);
         japi_standard_error(DRMAA_ERRNO_EXIT_TIMEOUT, diag);
         DEXIT;
         return DRMAA_ERRNO_EXIT_TIMEOUT;
      }
      pthread_mutex_unlock(&japi_ec_state_mutex);

      pthread_cond_wait(&Master_japi_job_list_finished_cv, &Master_japi_job_list_mutex);
   }
   JAPI_UNLOCK_JOB_LIST();

   /* remove reaped jobs from library session data */
   japi_dec_threads(SGE_FUNC);

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_synchronize_retry() ******************************************
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

/****** japi/japi_synchronize_all_retry() **************************************
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
      if (not_yet_finished) {
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

/****** japi/japi_synchronize_jobids_retry() ***********************************
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

/****** DRMAA/japi_wait() ****************************************************
*  NAME
*     japi_wait() -- Wait job
*
*  SYNOPSIS
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
*
*  INPUTS
*  RESULT
* 
*  RETURNS 
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
*
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*      Master_japi_job_list_mutex -> japi_ec_state_mutex
*
*  NOTES
*     MT-NOTE: japi_wait() is MT safe
*******************************************************************************/
int japi_wait(const char *job_id, dstring *waited_job, int *stat, signed long timeout, 
   drmaa_attr_values_t **values, dstring *diag)
{
   u_long32 jobid;
   u_long32 taskid = 0;
   int wait4any = 0;
   bool is_array_task;
   int drmaa_errno, wait_result;
   bool waited_is_task_array;
   u_long32 waited_jobid, waited_taskid;

   DENTER(TOP_LAYER, "japi_wait");

   /* ensure japi_init() was called */
   JAPI_LOCK_SESSION();
   if (japi_session != JAPI_SESSION_ACTIVE) {
      JAPI_UNLOCK_SESSION();
      japi_standard_error(DRMAA_ERRNO_NO_ACTIVE_SESSION, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

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

   /* check wait conditions */
   if (!strcmp(job_id, DRMAA_JOB_IDS_SESSION_ANY))
      wait4any = 1;
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

      JAPI_LOCK_JOB_LIST();

      while ((wait_result=japi_wait_retry(Master_japi_job_list, wait4any, jobid, taskid, is_array_task, 
         &waited_jobid, &waited_taskid, &waited_is_task_array)) == JAPI_WAIT_UNFINISHED) {

         /* must return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE when event client 
            thread was shutdown during japi_wait() use japi_ec_state ?? */
         /* has japi_exit() been called meanwhile ? */
         pthread_mutex_lock(&japi_ec_state_mutex);
         if (japi_ec_state != JAPI_EC_UP) {
            pthread_mutex_unlock(&japi_ec_state_mutex);
            JAPI_UNLOCK_JOB_LIST();
            japi_dec_threads(SGE_FUNC);
            japi_standard_error(DRMAA_ERRNO_EXIT_TIMEOUT, diag);
            DEXIT;
            return DRMAA_ERRNO_EXIT_TIMEOUT;
         }
         pthread_mutex_unlock(&japi_ec_state_mutex);

         pthread_cond_wait(&Master_japi_job_list_finished_cv, &Master_japi_job_list_mutex);
      }

      JAPI_UNLOCK_JOB_LIST();
      japi_dec_threads(SGE_FUNC);

   }

   if (wait_result==JAPI_WAIT_INVALID) {
      japi_standard_error(DRMAA_ERRNO_INVALID_JOB, diag);
      DEXIT;
      return DRMAA_ERRNO_INVALID_JOB;
   }

   /* copy jobid of finished job into buffer provided by caller */
   if (wait_result==JAPI_WAIT_FINISHED) {
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

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/japi_wait_retry() *************************************************
*  NAME
*     japi_wait_retry() -- seek for job_id in JJ_finished_jobs of all jobs
*
*  SYNOPSIS
*     static int japi_wait_retry(lList *japi_job_list, int wait4any, int jobid, 
*     int taskid, bool is_array_task, u_long32 *wjobidp, u_long32 *wtaskidp, 
*     bool *wis_task_arrayp) 
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
*     u_long32 *wjobidp         - destination for jobid of waited job
*     u_long32 *wtaskidp        - destination for taskid of waited job
*     u_long32 *wis_task_arrayp - destination for taskid of waited job
*
*  RESULT
*     static int - JAPI_WAIT_ALLFINISHED = there is nothing more to wait for
*                  JAPI_WAIT_UNFINISHED  = no job/task finished, but there are still unfinished tasks
*                  JAPI_WAIT_FINISHED    = got a finished task
*
*  NOTES
*     MT-NOTE: japi_wait_retry() is MT safe
*******************************************************************************/
static int japi_wait_retry(lList *japi_job_list, int wait4any, u_long32 jobid, u_long32 taskid, bool is_array_task,
u_long32 *wjobidp, u_long32 *wtaskidp, bool *wis_task_arrayp)
{
   lListElem *job; 
   lListElem *task = NULL; 
   
   DENTER(TOP_LAYER, "japi_wait_retry");

   /* seek for job_id in JJ_finished_jobs of all jobs */
   if (wait4any) {
      int not_yet_reaped = 0;

      for_each (job, japi_job_list) {
         task = lFirst(lGetList(job, JJ_finished_tasks));
         if (task) 
            break;
         if (lGetList(job, JJ_not_yet_finished_ids))
            not_yet_reaped = 1;
      }

      if (!task || !job) {
         if (not_yet_reaped) {
            DEXIT;
            return JAPI_WAIT_UNFINISHED;
         } else {
            DEXIT;
            return JAPI_WAIT_ALLFINISHED;
         }
      }
   } else {
      job = lGetElemUlong(japi_job_list, JJ_jobid, jobid);
      if (!job) {
         DEXIT;
         return JAPI_WAIT_ALLFINISHED;
      }

      /* for non-array jobs no task id may have been specified */
      if (!JOB_TYPE_IS_ARRAY(lGetUlong(job, JJ_type)) && taskid != 1) {
         DEXIT;
         return JAPI_WAIT_INVALID;
      }

      task = lGetSubUlong(job, JJAT_task_id, taskid, JJ_finished_tasks);
      if (!task) {
         if (range_list_is_id_within(lGetList(job, JJ_not_yet_finished_ids), taskid)) {
            DEXIT;
            return JAPI_WAIT_UNFINISHED;
         } else {
            DEXIT;
            return JAPI_WAIT_ALLFINISHED;
         }
      }
   }
  
   /* return all kind of job finish information */
   *wjobidp = lGetUlong(job, JJ_jobid);
   if (JOB_TYPE_IS_ARRAY(lGetUlong(job, JJ_type))) {
      *wis_task_arrayp = true;
      *wtaskidp = lGetUlong(task, JJAT_task_id);
   } else {
      *wis_task_arrayp = false;
   }   

#if 0
      *statp = lGetUlong(task, JJAT_stat);
      *rusagep = lGetList(task, JJAT_rusage);
      *diagnosis = lGetString(task, JJAT_rusage);
#endif

   /* remove reaped jobs from library session data */
   lRemoveElem(lGetList(job, JJ_finished_tasks), task);
   if (range_list_is_empty(lGetList(job, JJ_not_yet_finished_ids)) 
      && lGetNumberOfElem(lGetList(job, JJ_finished_tasks))==0) {
      lRemoveElem(Master_japi_job_list, job);
   }

   DEXIT;
   return JAPI_WAIT_FINISHED;
}

/*
 *     DRMAA_PS_UNDETERMINED = 00H : process status cannot be determined,
 *
 *     DRMAA_PS_QUEUED_ACTIVE = 10H : job is queued and active,
 *     DRMAA_PS_SYSTEM_ON_HOLD = 11H : job is queued and in system hold,
 *     DRMAA_PS_USER_ON_HOLD = 12H : job is queued and in user hold,
 *     DRMAA_PS_USER_SYSTEM_ON_HOLD = 13H : job is queued and in user and system hold,
 *
 *     DRMAA_PS_RUNNING = 20H : job is running,
 *     DRMAA_PS_SYSTEM_SUSPENDED = 21H : job is system suspended,
 *     DRMAA_PS_USER_SUSPENDED = 22H : job is user suspended,
 *     DRMAA_PS_USER_SYSTEM_SUSPENDED = 23H : job is user and system suspended,
 *
 *     DRMAA_PS_DONE = 30H : job finished normally, and
 *     DRMAA_PS_FAILED = 40H : job finished, but failed.
 */

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

/****** japi/sge_state_to_drmaa_state() ****************************************
*  NAME
*     sge_state_to_drmaa_state() -- Map Grid Engine state into DRMAA state
*
*  SYNOPSIS
*     static int sge_state_to_drmaa_state(lListElem *job, lList *queue_list, 
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
*     MT-NOTE: sge_state_to_drmaa_state() is MT safe
*******************************************************************************/
static int sge_state_to_drmaa_state(lListElem *job, lList *queue_list, bool is_array_task,
   u_long32 jobid, u_long32 taskid, int *remote_ps, dstring *diag)
{
   lListElem *ja_task;
   
   DENTER(TOP_LAYER, "sge_state_to_drmaa_state");

   /*
    * The reason for this job not longer be available at qmaster might 
    * be it is done or failed. The JAPI job list contains such information
    * if the job was not yet waited. For a job that is not found there either
    * we return DRMAA_ERRNO_INVALID_JOB.
    */
   if (!job) {
      lListElem *japi_job, *japi_task = NULL;

      JAPI_LOCK_JOB_LIST();
      
      japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);

      if (japi_job) {
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
      }

      if (!japi_job || !japi_task) {
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
   
   if ((ja_task=job_search_task(job, NULL, taskid))) { 
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
          * can be released using drmaa_control().
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
      if ((ja_task_state & JSUSPENDED_ON_THRESHOLD) || 
         queue_list_suspends_ja_task(queue_list, lGetList(ja_task, JAT_granted_destin_identifier_list)))
         *remote_ps |= DRMAA_PS_SUBSTATE_SYSTEM_SUSP;

      DEXIT;
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




/****** japi/japi_get_job_and_queues() *****************************************
*  NAME
*     japi_get_job_and_queues() -- get job and the queue via GDI for job status
*
*  SYNOPSIS
*     static int japi_get_job_and_queues(u_long32 jobid, lList 
*     **retrieved_queue_list, lList **retrieved_job_list, dstring *diag) 
*
*  FUNCTION
*     We use GDI GET to get jobs status. Additionally also the queue list 
*     must be retrieved because the (queue) system suspend is kept in the 
*     queue where the job runs.
*
*  INPUTS
*     u_long32 jobid               - the jobs id
*     lList **retrieved_queue_list - resulting queue list
*     lList **retrieved_job_list   - resulting job list
*     dstring *diag                - diagnosis info
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTES: japi_get_job_and_queues() is MT safe
*******************************************************************************/
static int japi_get_job_and_queues(u_long32 jobid, lList **retrieved_queue_list, 
      lList **retrieved_job_list, dstring *diag)
{
   lList *mal = NULL;
   lList *alp;
   lListElem *aep;
   int qu_id, jb_id = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "japi_get_job_and_queues");

   /* we need all queues */
   {
      lCondition *queue_selection;
      lEnumeration *queue_fields;

      queue_selection = lWhere("%T(%I != %s)", QU_Type, QU_qname, SGE_TEMPLATE_NAME);
      queue_fields = lWhat("%T(%I%I)", QU_Type, QU_qname, QU_state);
      if (!queue_selection || !queue_fields) {
         japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diag);
         DEXIT;
         return DRMAA_ERRNO_NO_MEMORY;
      }

      qu_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_QUEUE_LIST, SGE_GDI_GET, NULL, 
                  queue_selection, queue_fields, NULL, &state);

      queue_selection = lFreeWhere(queue_selection);
      queue_fields = lFreeWhat(queue_fields);
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
      jb_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_JOB_LIST, SGE_GDI_GET, NULL, 
            job_selection, job_fields, &mal, &state);
      job_selection = lFreeWhere(job_selection);
      job_fields = lFreeWhat(job_fields);
   }

   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_QUEUE_LIST, qu_id, mal, retrieved_queue_list );

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

/****** japi/japi_parse_jobid() ************************************************
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

/****** DRMAA/japi_job_ps() ****************************************************
*  NAME
*     japi_job_ps() -- Get job status
*
*  SYNOPSIS
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
*  RESULT
*
*  NOTES
*     MT-NOTE: japi_job_ps() is MT safe
*******************************************************************************/
int japi_job_ps(const char *job_id_str, int *remote_ps, dstring *diag)
{
   u_long32 jobid, taskid;
   lList *retrieved_job_list = NULL,
         *retrieved_queue_list = NULL;
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

   drmaa_errno = japi_get_job_and_queues(jobid, &retrieved_queue_list, &retrieved_job_list, diag);
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      japi_dec_threads(SGE_FUNC);
      /* diag written by japi_get_job_and_queues() */
      DEXIT;
      return drmaa_errno;
   }

   drmaa_errno = sge_state_to_drmaa_state(lFirst(retrieved_job_list), retrieved_queue_list, is_array_task, jobid, taskid, remote_ps, diag);

   japi_dec_threads(SGE_FUNC);

   retrieved_job_list = lFreeList(retrieved_job_list);
   retrieved_queue_list = lFreeList(retrieved_queue_list);

   DEXIT;
   return drmaa_errno;
}

/****** DRMAA/japi_strerror() ****************************************************
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
*  RESULT
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

      { DRMAA_NO_ERRNO, NULL }
   };

   int i;

   for (i=0; error_text[i].drmaa_errno != DRMAA_NO_ERRNO; i++)
      if (drmaa_errno == error_text[i].drmaa_errno) 
         return error_text[i].str;

   return "unknown drmaa_errno";
}

/****** DRMAA/implementation_thread() ****************************************************
*  NAME
*     implementation_thread() -- Control flow implementation thread
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*  RESULT
*
*  NOTES
*     MT-NOTE: implementation_thread() is MT safe
*******************************************************************************/
static void *implementation_thread(void *p)
{
   lList *alp = NULL, *event_list = NULL;
   lListElem *event;
   char buffer[1024];
   dstring buffer_wrapper;
   int cl_errno, stop_ec = 0;
   int parameter, ed_time = 30, flush_delay_rate = 6;
   const char *s;

   DENTER(TOP_LAYER, "implementation_thread");

   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   /* needed to init comlib per thread globals */
   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if (sge_gdi_setup("japi_ec", &alp)!=AE_OK) {
      DPRINTF(("error: sge_gdi_setup() failed for event client thread\n"));
      lFreeList(alp);
      goto SetupFailed;
   }
   log_state_set_log_gui(1);


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
   ec_prepare_registration(EV_ID_ANY, "japi");
   ec_set_edtime(ed_time); 
   ec_set_busy_handling(EV_THROTTLE_FLUSH); 
   ec_set_flush_delay(flush_delay_rate); 
   ec_set_session(japi_session_key); 
   ec_subscribe(sgeE_JOB_LIST);

   ec_subscribe(sgeE_JOB_FINAL_USAGE);
   ec_set_flush(sgeE_JOB_FINAL_USAGE, 0);

   ec_subscribe(sgeE_JOB_DEL);
   ec_set_flush(sgeE_JOB_DEL, 0);

   ec_subscribe(sgeE_JATASK_DEL);
   ec_set_flush(sgeE_JATASK_DEL, 0);

   ec_subscribe(sgeE_SHUTDOWN);
   ec_set_flush(sgeE_SHUTDOWN, 0);

/*    sgeE_QMASTER_GOES_DOWN  ??? */

   /* commlib timeout depends on event delivery interval */
   set_commlib_param(CL_P_TIMEOUT_SRCV, ed_time*2, NULL, NULL);
/*    set_commlib_param(CL_P_TIMEOUT_SSND, ed_time, NULL, NULL); */

   if (!ec_register(false)) {
      DPRINTF(("error: ec_register() failed\n"));
      goto SetupFailed;
   }
   japi_ec_id = ec_get_id();

   /* set japi_ec_state to JAPI_EC_UP and notify initialization thread */
   DPRINTF(("signalling event client thread is up and running\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);
   japi_ec_state = JAPI_EC_UP;
      pthread_cond_signal(&japi_ec_state_starting_cv);
   pthread_mutex_unlock(&japi_ec_state_mutex);

   while (!stop_ec) {
      int ec_get_ret;

      /* read events and add relevant information into library session data */
      if (!(ec_get_ret = ec_get(&event_list, false))) {
         ec_mark4registration();
         sleep(1);
      } else {
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

                  JAPI_LOCK_JOB_LIST();

                  /* - check every session job  
                     - no longer existing jobs must be moved to JJ_finished_jobs */
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
               }
               break;
            case sgeE_JOB_FINAL_USAGE:
            case sgeE_JOB_DEL:
            case sgeE_JATASK_DEL:
               /* sgeE_JOB_FINAL_USAGE/sgeE_JOB_DEL/sgeE_JATASK_DEL
                  - move job/task to JJ_finished_jobs */
               {
                  lListElem *japi_job, *japi_task;
                  
                  JAPI_LOCK_JOB_LIST();

                  japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, intkey);
                  if (japi_job) {
                     if (range_list_is_id_within(lGetList(japi_job, JJ_not_yet_finished_ids), intkey2)) {
                        /* remove task from not yet finished job id list */
                        object_delete_range_id(japi_job, NULL, JJ_not_yet_finished_ids, intkey2);

                        /* add an entry to the finished tasks */
                        DPRINTF(("adding finished task %ld for job %ld\n", intkey2, intkey));
                        japi_task = lAddSubUlong(japi_job, JJAT_task_id, intkey2, JJ_finished_tasks, JJAT_Type);

                        /* signal all application threads waiting for a job to finish */
                        pthread_cond_broadcast(&Master_japi_job_list_finished_cv);
                     }
                  } else {
                     fprintf(stderr, "ignoring event on unknown job "u32"\n", intkey);
                  }

                  JAPI_UNLOCK_JOB_LIST();
               }

               break;

            default:
               /* no explicit action required on sgeE_SHUTDOWN */
               break;
            }
         }
         event_list = lFreeList(event_list);
      }

      /* has japi_exit() been called meanwhile ? */ 
      pthread_mutex_lock(&japi_ec_state_mutex);   
      if (japi_ec_state == JAPI_EC_FINISHING) {
         stop_ec = 1;
      }

      if (!ec_get_ret && !stop_ec)
         fprintf(stderr, "error: problems with ec_get()\n");
      pthread_mutex_unlock(&japi_ec_state_mutex);   
   }
   
   /*  unregister event client */
   DPRINTF(("unregistering from qmaster ...\n"));
   if (ec_deregister()==FALSE) {
      DPRINTF(("failed unregistering event client from qmaster.\n"));
      ec_return_value = FALSE;
   } else {
      ec_return_value = TRUE;
      DPRINTF(("... unregistered.\n"));
   }

   /* 
    * disconnect from commd
    */
   if ((cl_errno=leave_commd())!=CL_OK) {
      DPRINTF(("leave_commd() failed: %s", cl_errstr(cl_errno)));
      DEXIT;
      return &ec_return_value;
   }

   DEXIT;
   return &ec_return_value;

SetupFailed:
   pthread_mutex_lock(&japi_ec_state_mutex);
   japi_ec_state = JAPI_EC_FAILED;
      pthread_cond_signal(&japi_ec_state_starting_cv);
   pthread_mutex_unlock(&japi_ec_state_mutex);
   ec_return_value = FALSE;
   DEXIT;
   return &ec_return_value;
}
