#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "japi.h"

/* CULL */
#include "cull_list.h"

/* self */
#include "japiP.h"

/* RMON */
#include "sgermon.h"

/* UTI */
#include "sge_string.h"

/* COMMLIB */
#include "commlib.h"

/* EVC */
#include "sge_event_client.h"

/* GDI */
#include "sge_gdi.h"
#include "sge_gdi_intern.h"

#include "sge_event.h"
#include "sge_job.h"

#include "sge_range.h"
#include "sge_object.h"

/* OBJ */
#include "sge_japiL.h"
#include "sge_varL.h"
#include "sge_stringL.h"
#include "sge_jobL.h"
#include "sge_answerL.h"
#include "sge_answer.h"

enum { 
   JAPI_EC_DOWN,
   JAPI_EC_UP,
   JAPI_EC_FINISHING,
   JAPI_EC_FAILED
};

int ec_return_value;

static void *implementation_thread(void *);

static pthread_t event_client_thread;

/* ------------------------------------- */

int japi_session_key = 0;

/* needed to track if drmaa_init() was called and has succeeded */
static pthread_mutex_t japi_session_key_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ------------------------------------- */

int japi_ec_state = JAPI_EC_DOWN;

/* guards access to japi_ec_state global variable */
static pthread_mutex_t japi_ec_state_mutex = PTHREAD_MUTEX_INITIALIZER;

/* needed in drmaa_init() to allow waiting for event 
   client thread being up and running */
static pthread_cond_t japi_ec_state_starting_cv = PTHREAD_COND_INITIALIZER;

/* ------------------------------------- */

lList *Master_japi_job_list = NULL;

/* guards access to Master_japi_job_list global variable */
static pthread_mutex_t Master_japi_job_list_mutex = PTHREAD_MUTEX_INITIALIZER;

/* this condition is raised when a job/task is finshed */
static pthread_cond_t Master_japi_job_list_finished_cv = PTHREAD_COND_INITIALIZER;

/* ------------------------------------- */

/* these non vector job template attributes are supported */
static const char *japi_supported_nonvector[] = {
   DRMAA_REMOTE_COMMAND,
   DRMAA_JS_STATE,
   DRMAA_WD,
   DRMAA_JOB_CATEGORY,
   DRMAA_NATIVE_SPECIFICATION,
   DRMAA_BLOCK_EMAIL,
   DRMAA_START_TIME,
   DRMAA_JOB_NAME,
   DRMAA_INPUT_PATH,
   DRMAA_OUTPUT_PATH,
   DRMAA_ERROR_PATH,
   DRMAA_JOIN_FILES,
   DRMAA_TRANSFER_FILES,
   DRMAA_DEADLINE_TIME,
   DRMAA_WCT_HLIMIT,
   DRMAA_WCT_SLIMIT,
   DRMAA_DURATION_HLIMIT,
   DRMAA_DURATION_SLIMIT,
   NULL
};

/* these vector job template attributes are supported */
static const char *japi_supported_vector[] = {
   DRMAA_V_ARGV,
   DRMAA_V_ENV,
   DRMAA_V_EMAIL,
   NULL
};

static int is_supported(const char *name, const char *supported_list[])
{
   int i;
   for (i=0; supported_list[i]; i++) {
      if (!strcmp(name, supported_list[i]))
         return 1;
   }
   return 0;
}

static int drmaa_init_mt(void)
{
   int gdi_errno;
   lList *alp = NULL;

   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   gdi_errno = sge_gdi_setup("japi", &alp);
   if (gdi_errno!=AE_OK && gdi_errno != AE_ALREADY_SETUP) {
      fprintf(stderr, "error: sge_gdi_setup() failed for application thread\n");
      lFreeList(alp);
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_init() ****************************************************
*  NAME
*     drmaa_init() -- Initialize DRMAA API library
*
*  SYNOPSIS
*
*  FUNCTION
*     Initialize DRMAA API library and create a new DRMAA Session. 'Contact'
*     is an implementation dependent string which may be used to specify
*     which DRM system to use. This routine must be called before any
*     other DRMAA calls, except for drmaa_version().
*     If 'contact' is NULL, the default DRM system will be used.
*     Initializes internal data structures and registers with qmaster
*     using the event client mechanisms.
*
*  INPUTS
*  RESULT
*
*******************************************************************************/
int drmaa_init(const char *contact)
{
   int i;
   int ret;
   int *value;

   DENTER(TOP_LAYER, "drmaa_init");

   pthread_mutex_lock(&japi_session_key_mutex);   
   if (japi_session_key != 0) {
      pthread_mutex_unlock(&japi_session_key_mutex);   
      DEXIT;
      return DRMAA_ERRNO_ALREADY_ACTIVE_SESSION;
   }

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      pthread_mutex_unlock(&japi_session_key_mutex);   
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* read in library session data of former session if any */

   /* spawn implementation thread implementation_thread() */
   DPRINTF(("spawning event client thread\n"));

   if ((i=pthread_create(&event_client_thread, NULL, implementation_thread, (void *) NULL))) {
      fprintf(stderr, "error: couldn't create event client thread: %d %d\n", i, errno);
      pthread_mutex_unlock(&japi_session_key_mutex);   
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* wait until event client id is operable or gave up passed by event client thread */
   DPRINTF(("waiting for JAPI_EC_UP ...\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);   
   if (japi_ec_state == JAPI_EC_DOWN) {
      pthread_cond_wait(&japi_ec_state_starting_cv, &japi_ec_state_mutex);
   }
   if (japi_ec_state == JAPI_EC_UP)
      ret = DRMAA_ERRNO_SUCCESS;
   else /* JAPI_EC_FAILED */
      ret = DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   pthread_mutex_unlock(&japi_ec_state_mutex);   
   DPRINTF(("... got JAPI_EC_UP\n"));

   if (ret == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
      if (!pthread_join(event_client_thread, (void *)&value)) {
         DPRINTF(("drmaa_init(): pthread_join returned %d\n", *value));
      }
   }

   japi_session_key = 1;
   pthread_mutex_unlock(&japi_session_key_mutex);   

   DEXIT;
   return ret;
}



/****** DRMAA/drmaa_exit() ****************************************************
*  NAME
*     drmaa_exit() -- Shutdown DRMAA API library
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
*******************************************************************************/
int drmaa_exit(void)
{
   int i;
   int *value;

   DENTER(TOP_LAYER, "drmaa_exit");

   pthread_mutex_lock(&japi_session_key_mutex);   
   if (japi_session_key == 0) {
      pthread_mutex_unlock(&japi_session_key_mutex);   
      DEXIT;
      return DRMAA_ERRNO_NO_ACTIVE_SESSION;
   }

   /* notify event client about shutdown */
   DPRINTF(("notify event client about shutdown\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);
   japi_ec_state = JAPI_EC_FINISHING;
   pthread_mutex_unlock(&japi_ec_state_mutex);

   i = pthread_join(event_client_thread, (void *)&value); 
   DPRINTF(("drmaa_exit(): value = %d pthread_join returned %d: %s\n", *value, i, strerror(errno)));

   japi_session_key = 0;
   pthread_mutex_unlock(&japi_session_key_mutex);   

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/drmaa_allocate_job_template() *************************************
*  NAME
*     drmaa_allocate_job_template() -- Allocate a new job template. 
*
*  SYNOPSIS
*
*  FUNCTION
*  RESULT
*******************************************************************************/
int drmaa_allocate_job_template(job_template_t **jtp)
{
   DENTER(TOP_LAYER, "drmaa_allocate_job_template");

   if (!jtp) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   } 
 
   *jtp = (job_template_t *)malloc(sizeof(job_template_t));
   (*jtp)->strings = (*jtp)->string_vectors = NULL;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** japi/drmaa_delete_job_template() ***************************************
*  NAME
*     drmaa_delete_job_template() -- Deallocate a job template. This routine has no effect on jobs.
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*     job_template_t *jt - job template to be deleted
*
*  RESULT
*******************************************************************************/
int drmaa_delete_job_template(job_template_t *jt)
{
   DENTER(TOP_LAYER, "drmaa_delete_job_template");

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


/****** japi/drmaa_set_attribute() *********************************************
*  NAME
*     drmaa_set_attribute() -- Set non vector attribute in job template
*
*  SYNOPSIS
*
*  FUNCTION
*     Adds ('name', 'value') pair to list of attributes in job template 'jt'.
*     Only non-vector attributes may be passed.
*
*  INPUTS
*     job_template_t *jt - job template
*     const char *name   - name 
*     const char *value  - value
*
*  RESULT
*******************************************************************************/
int drmaa_set_attribute(job_template_t *jt, const char *name, const char *value)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "drmaa_set_attribute");
      
   if (!jt) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

#if 0
   if (is_supported(name, japi_supported_nonvector)) {
      /* verify value */
      if ()
      return INVALID_ATTRIBUTE_VALUE;
   }
#endif
  
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


/* 
 * If 'name' is an existing non-vector attribute name in the job 
 * template 'jt', then the value of 'name' is returned; otherwise, 
 * NULL is returned.
 */ 
int drmaa_get_attribute(job_template_t *jt, const char *name, char *value, size_t value_size)
{
   return DRMAA_ERRNO_SUCCESS;
}

/* Adds ('name', 'values') pair to list of vector attributes in job template 'jt'.
 * Only vector attributes may be passed. 
 */
int drmaa_set_vector_attribute(job_template_t *jt, const char *name, char *value[])
{
   int i;
   lList *lp;
   lListElem *ep, *sep;

   DENTER(TOP_LAYER, "drmaa_set_vector_attribute");

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
      lSetString(sep, STR, value[i]);
      lAppendElem(lp, sep);
   }
   lSetList(ep, NSV_strings, lp);

   return DRMAA_ERRNO_SUCCESS;
}


/* 
 * If 'name' is an existing vector attribute name in the job template 'jt',
 * then the values of 'name' are returned; otherwise, NULL is returned.
 */
int drmaa_get_vector_attribute(job_template_t *jt, const char *name /* , vector of attribute values */ )
{
/*       sge_stradup()/sge_strafree() */
   return DRMAA_ERRNO_SUCCESS;
}


/* 
 * Returns the set of supported attribute names whose associated   
 * value type is String. This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 */
int drmaa_get_attribute_names( void /* vector of attribute name (string vector) */)
{
   return DRMAA_ERRNO_SUCCESS;
}

/*
 * Returns the set of supported attribute names whose associated 
 * value type is String Vector.  This set will include supported DRMAA reserved 
 * attribute names and native attribute names. */
int drmaa_get_vector_attribute_names(void /* vector of attribute name (string vector) */)
{
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_run_job() ****************************************************
*  NAME
*     drmaa_run_job() -- Submit a job
*
*  SYNOPSIS
*
*  FUNCTION
*     Submit a job with attributes defined in the job template 'jt'.
*     The job identifier 'job_id' is a printable, NULL terminated string,
*     identical to that returned by the underlying DRM system.
*
*  INPUTS
*  RESULT
*
*******************************************************************************/
int drmaa_run_job(char *job_id, int job_id_size, job_template_t *jt)
{
   lListElem *job, *ep, *aep;
   lList *job_lp, *alp;
   u_long32 jobid;

   DENTER(TOP_LAYER, "drmaa_run_job");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* make JB_Type job description out of DRMAA job template */
   job = lCreateElem(JB_Type);

   /* remote command */
   if (!(ep=lGetElemStr(jt->strings, VA_variable, DRMAA_REMOTE_COMMAND))) {
      job = lFreeElem(job);   
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }
   lSetString(job, JB_script_file, lGetString(ep, VA_value));

   /* use always binary submission mode */
   {
      u_long32 jb_now = lGetUlong(job, JB_type);
      JOB_TYPE_SET_BINARY(jb_now);
      lSetUlong(job, JB_type, jb_now);
   }

   /* job arguments */
   if ((ep=lGetElemStr(jt->string_vectors, NSV_name, DRMAA_V_ARGV)))
      lSetList(job, JB_job_args, lCopyList(NULL, lGetList(ep, NSV_strings)));

   /* job name */
   if ((ep=lGetElemStr(jt->strings, VA_variable, DRMAA_JOB_NAME))) {
      lSetString(job, JB_job_name, lGetString(ep, VA_value));
   } else {
      /* use command basename */
      const char *command = lGetString(job, JB_script_file);
      lSetString(job, JB_job_name, sge_basename(command, '/'));
   }

   /* job_initialize_env(); */

   /* average priority of 0 */
   lSetUlong(job, JB_priority, BASE_PRIORITY);

   /* init range of jobids */
   job_set_submit_task_ids(job, 1, 1, 1);

   job_lp = lCreateList(NULL, JB_Type);
   lAppendElem(job_lp, job);

   /* use GDI to submit job for this session */
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_ADD|SGE_GDI_RETURN_NEW_VERSION, &job_lp, NULL, NULL);

   /* reinitialize 'job' with pointer to new version from qmaster */
   if ((job = lFirst(job_lp)))
      jobid = lGetUlong(job, JB_job_number);
   job_lp = lFreeList(job_lp);

   if (!(aep = lFirst(alp)) || !job) {
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   } else {
      const char *s;
      u_long32 quality, job_id;

      quality = lGetUlong(aep, AN_quality);
      s = lGetString(aep, AN_text);
      if (quality == ANSWER_QUALITY_ERROR) {
         if (s[strlen(s)-1] != '\n') {
            fprintf(stderr, "%s\n", s);
         } else {
            fprintf(stderr, "%s", s);
         }
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      } 
/*       fprintf(stderr, "%s", lGetString(aep, AN_text)); */
   }

   /* return jobid as string */
   snprintf(job_id, job_id_size, "%ld", jobid);

   /* maintain library session data */ 
   {
      lListElem *japi_job;

      pthread_mutex_lock(&Master_japi_job_list_mutex);   
 
      japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);
      if (japi_job) {
         /* job may not yet exist */
         pthread_mutex_unlock(&Master_japi_job_list_mutex);   
         DEXIT;
         return DRMAA_ERRNO_INTERNAL_ERROR;
      }

      /* add job to library session data 
         -  all tasks in JJ_not_yet_finished_ids
         -  no task in JJ_finished_jobs */
      japi_job = lAddElemUlong(&Master_japi_job_list, JJ_jobid, jobid, JJ_Type);
      object_set_range_id(japi_job, JJ_not_yet_finished_ids, 1, 1, 1);

      pthread_mutex_unlock(&Master_japi_job_list_mutex);   
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_run_bulk_jobs() ****************************************************
*  NAME
*     drmaa_run_bulk_jobs() -- Submit a bulk of jobs
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
*******************************************************************************/
int drmaa_run_bulk_jobs(char *job_ids[], job_template_t *jt, int start, int end, int incr)
{
   DENTER(TOP_LAYER, "drmaa_run_bulk_jobs");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* make JB_Type job arry description out of DRMAA job template */


   /* use GDI to submit job array for this session */

   /* add job arry to library session data */
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_control() ****************************************************
*  NAME
*     drmaa_control() -- Start, stop, restart, or kill jobs
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
*******************************************************************************/
int drmaa_control(const char *jobid, int action)
{
   DENTER(TOP_LAYER, "drmaa_control");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* use GDI to implement control operations */
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

enum {
   JAPI_WAIT_ALLFINISHED, /* there is nothing more to wait for */
   JAPI_WAIT_UNFINISHED,  /* there are still unfinished tasks  */
   JAPI_WAIT_FINISHED     /* got a finished task */
};
static int japi_wait_retry(int wait4any, int jobid, int taskid, lListElem **japi_jobp, lListElem **japi_taskp)
{
   lListElem *job, *task; 
   
   DENTER(TOP_LAYER, "japi_wait_retry");

   /* seek for job_id in JJ_finished_jobs of all jobs */
   if (wait4any) {
      int not_yet_reaped = 0;

      for_each (job, Master_japi_job_list) {
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
      job = lGetElemUlong(Master_japi_job_list, JJ_jobid, jobid);
      if (!job) {
         DEXIT;
         return JAPI_WAIT_ALLFINISHED;
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
   
   *japi_jobp = job;
   *japi_taskp = task;

   DEXIT;
   return JAPI_WAIT_FINISHED;
}

/****** DRMAA/drmaa_synchronize() ****************************************************
*  NAME
*     drmaa_synchronize() -- Synchronize with jobs to finish
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
*******************************************************************************/
int drmaa_synchronize(char *job_ids[], signed long timeout, int dispose)
{
   DENTER(TOP_LAYER, "drmaa_synchronize");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* wait(?) until specified jobs have finished according to library session data */

   /* remove reaped jobs from library session data */
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_wait() ****************************************************
*  NAME
*     drmaa_wait() -- Wait job
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
*     to drmaa_wait should fail returning an error DRMAA_ERRNO_INVALID_JOB meaning
*     that the job has been already reaped. This error is the same as if the job was
*     unknown. Failing due to an elapsed timeout has an effect that it is possible to
*     issue drmaa_wait multiple times for the same job_id.
*
*  INPUTS
*  RESULT
*
*******************************************************************************/
int drmaa_wait(const char *job_id, char *job_id_out, int job_id_size, int *stat, signed long timeout, char *rusage[])
{
   u_long32 jobid, taskid;
   int wait4any = 0;
   int wait_result;
   lListElem *japi_job, *japi_task;

   DENTER(TOP_LAYER, "drmaa_wait");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* check wait conditions */
   if (!strcmp(job_id, DRMAA_JOB_IDS_SESSION_ANY))
      wait4any = 1;
   else {
      wait4any = 1;
      sscanf(job_id, "%ld", &jobid);
      taskid = 1;
   }

   pthread_mutex_lock(&Master_japi_job_list_mutex);   
   
   DPRINTF(("drmaa_wait(1)\n"));

   while ((wait_result=japi_wait_retry(wait4any, jobid, taskid, &japi_job, &japi_task)) == JAPI_WAIT_UNFINISHED) {
      DPRINTF(("drmaa_wait(2)\n"));
      pthread_cond_wait(&Master_japi_job_list_finished_cv, &Master_japi_job_list_mutex);
   }
   DPRINTF(("drmaa_wait(3)\n"));

   if (wait_result==JAPI_WAIT_FINISHED) {
      /* remove reaped jobs from library session data */
      lDechainElem(lGetList(japi_job, JJ_finished_tasks), japi_task);
      if (range_list_is_empty(lGetList(japi_job, JJ_not_yet_finished_ids))) {
         lRemoveElem(Master_japi_job_list, japi_job);
      }
   }
   pthread_mutex_unlock(&Master_japi_job_list_mutex);   

   if (wait_result!=JAPI_WAIT_FINISHED) {
      DEXIT;
      return DRMAA_ERRNO_INVALID_JOB;
   }

   snprintf(job_id_out, job_id_size, "%ld", lGetUlong(japi_job, JJ_jobid));

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_job_ps() ****************************************************
*  NAME
*     drmaa_job_ps() -- Get job status
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
*******************************************************************************/
int drmaa_job_ps(const char *job_id, int *remote_ps)
{
   DENTER(TOP_LAYER, "drmaa_job_ps");

   /* per thread initialization */
   if (drmaa_init_mt()!=DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE;
   }

   /* use GDI to get jobs status */
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
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
*******************************************************************************/
static void *implementation_thread(void *p)
{
   int i;
   lList *alp = NULL, *event_list = NULL;
   lListElem *event;
   char buffer[1024];
   dstring buffer_wrapper;
   int stop_ec = 0;

   DENTER(TOP_LAYER, "implementation_thread");

   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   /* needed to init comlib per thread globals */
   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if (sge_gdi_setup("japi_ec", &alp)!=AE_OK) {
      fprintf(stderr, "error: sge_gdi_setup() failed for event client thread\n");
      lFreeList(alp);
      goto SetupFailed;
   }

   /* register at qmaster as event client */
   printf("registering as event client ...\n");
   ec_prepare_registration(EV_ID_ANY, "japi");
   ec_subscribe(sgeE_JOB_LIST);
   ec_subscribe(sgeE_JOB_FINAL_USAGE);
   ec_subscribe(sgeE_JOB_DEL);
   ec_set_flush(sgeE_JOB_FINAL_USAGE, 0);
   ec_set_flush(sgeE_JOB_DEL, 0);

   if (!ec_register()) {
      fprintf(stderr, "error: ec_register() failed\n");
      goto SetupFailed;
   }

   /* set japi_ec_state to JAPI_EC_UP to notify initialization thread */
   DPRINTF(("signalling event client thread is up and running\n"));
   pthread_mutex_lock(&japi_ec_state_mutex);
   japi_ec_state = JAPI_EC_UP;
      pthread_cond_signal(&japi_ec_state_starting_cv);
   pthread_mutex_unlock(&japi_ec_state_mutex);


   while (!stop_ec) {
      /* read events and add relevant information into library session data */
      ec_get(&event_list);
      DPRINTF(("sleeping ...\n"));
      sleep(1);
      for_each (event, event_list) {
         u_long32 number, type, intkey, intkey2;
         number = lGetUlong(event, ET_number);
         type = lGetUlong(event, ET_type);
         intkey = lGetUlong(event, ET_intkey);
         intkey2 = lGetUlong(event, ET_intkey2);

         printf("Event: %s\n", event_text(event, &buffer_wrapper));

         /* maintain library session data */ 
         switch (type) {
         case sgeE_JOB_LIST:
            /* - check every session job  
               - no longer existing jobs must be moved to JJ_finished_jobs */
            break;
         case sgeE_JOB_FINAL_USAGE:
         case sgeE_JOB_DEL:
            /* sgeE_JOB_FINAL_USAGE/sgeE_JOB_DEL 
               - move job/task to JJ_finished_jobs */
            {
               lListElem *japi_job, *japi_task;
               
               DPRINTF(("impl_tread(1)\n"));
               pthread_mutex_lock(&Master_japi_job_list_mutex);   
               DPRINTF(("impl_tread(2)\n"));
               japi_job = lGetElemUlong(Master_japi_job_list, JJ_jobid, intkey);
               if (japi_job) {
                  DPRINTF(("impl_tread(3)\n"));
                  if (range_list_is_id_within(lGetList(japi_job, JJ_not_yet_finished_ids), intkey2)) {
                     DPRINTF(("impl_tread(4)\n"));
                     /* remove task from not yet finished job id list */
                     object_delete_range_id(japi_job, NULL, JJ_not_yet_finished_ids, intkey2);

                     /* add an entry to the finished tasks */
                     japi_task = lAddSubUlong(japi_job, JJAT_task_id, intkey2, JJ_finished_tasks, JJAT_Type);

                     /* signal all application threads waiting for a job to finish */
                     pthread_cond_broadcast(&Master_japi_job_list_finished_cv);
                  }
               }
               pthread_mutex_unlock(&Master_japi_job_list_mutex);   
               DPRINTF(("impl_tread(5)\n"));
            }

            break;
         }
      }
      event_list = lFreeList(event_list);

      /* has drmaa_exit() been called meanwhile ? */ 
      pthread_mutex_lock(&japi_ec_state_mutex);   
      if (japi_ec_state == JAPI_EC_FINISHING) {
         stop_ec = 1;
      }
      pthread_mutex_unlock(&japi_ec_state_mutex);   
   }
   
   /*  unregister event client */
   DPRINTF(("unregistering from qmaster ...\n"));
   if (ec_deregister()==FALSE) {
      fprintf(stderr, "failed unregistering event client from qmaster.\n");
      ec_return_value = FALSE;
   } else
      ec_return_value = TRUE;

   DPRINTF(("... unregistered.\n"));
   ec_return_value = TRUE;
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
