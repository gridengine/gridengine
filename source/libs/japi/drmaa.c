#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <pthread.h>

#define JOIN_ECT

#include "japi.h"
#include "sge_dstring.h"

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
*  MUTEXES
*      japi_session_mutex -> japi_ec_state_mutex
*
*  NOTES
*      MT-NOTE: drmaa_init() is MT safe
*******************************************************************************/
int drmaa_init(const char *contact, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_init(contact, error_diagnosis?&diag:NULL);
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
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: drmaa_exit() is MT safe
*******************************************************************************/
int drmaa_exit(char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_exit(error_diagnosis?&diag:NULL);
}

/****** japi/drmaa_allocate_job_template() *************************************
*  NAME
*     drmaa_allocate_job_template() -- Allocate a new job template. 
*
*  SYNOPSIS
*
*  FUNCTION
*  RESULT
*
*  NOTES
*      MT-NOTE: drmaa_allocate_job_template() is MT safe
*******************************************************************************/
int drmaa_allocate_job_template(drmaa_job_template_t **jtp, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_allocate_job_template(jtp, error_diagnosis?&diag:NULL);
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
*     drmaa_job_template_t *jt - job template to be deleted
*
*  RESULT
*
*  NOTES
*      MT-NOTE: drmaa_delete_job_template() is MT safe
*******************************************************************************/
int drmaa_delete_job_template(drmaa_job_template_t *jt, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_delete_job_template(jt, error_diagnosis?&diag:NULL);
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
*     drmaa_job_template_t *jt - job template
*     const char *name   - name 
*     const char *value  - value
*
*  RESULT
*
*  NOTES
*      MT-NOTE: drmaa_set_attribute() is MT safe
*******************************************************************************/
int drmaa_set_attribute(drmaa_job_template_t *jt, const char *name, const char *value, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_set_attribute(jt, name, value, error_diagnosis?&diag:NULL);
}

/* 
 * If 'name' is an existing non-vector attribute name in the job 
 * template 'jt', then the value of 'name' is returned; otherwise, 
 * NULL is returned.
 *
 *      MT-NOTE: drmaa_get_attribute() is MT safe
 */ 
int drmaa_get_attribute(drmaa_job_template_t *jt, const char *name, char *value, 
   size_t value_len, char *error_diagnosis, size_t error_diag_len)
{
   dstring val, diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   if (value) 
      sge_dstring_init(&val, value, value_len);
   return japi_get_attribute(jt, name, &val, error_diagnosis?&diag:NULL);
}

/* Adds ('name', 'values') pair to list of vector attributes in job template 'jt'.
 * Only vector attributes may be passed. 
 *
 *      MT-NOTE: drmaa_set_vector_attribute() is MT safe
 */
int drmaa_set_vector_attribute(drmaa_job_template_t *jt, const char *name, 
      char *value[], char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_set_vector_attribute(jt, name, value, error_diagnosis?&diag:NULL);
}

/* 
 * If 'name' is an existing vector attribute name in the job template 'jt',
 * then the values of 'name' are returned; otherwise, NULL is returned.
 *
 *      MT-NOTE: drmaa_get_vector_attribute() is MT safe
 */
int drmaa_get_vector_attribute(drmaa_job_template_t *jt, const char *name, drmaa_string_vector_t **values, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_get_vector_attribute(jt, name, values, error_diagnosis?&diag:NULL);
}


/* 
 * Returns the set of supported attribute names whose associated   
 * value type is String. This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 *
 *      MT-NOTE: drmaa_get_attribute_names() is MT safe
 */
int drmaa_get_attribute_names(drmaa_string_vector_t **values, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_get_attribute_names(values, error_diagnosis?&diag:NULL);
}

/*
 * Returns the set of supported attribute names whose associated 
 * value type is String Vector.  This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 *
 *      MT-NOTE: drmaa_get_vector_attribute_names() is MT safe
 */
int drmaa_get_vector_attribute_names(drmaa_string_vector_t **values, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_get_vector_attribute_names(values, error_diagnosis?&diag:NULL);
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
*      MT-NOTE: drmaa_run_job() is MT safe
*******************************************************************************/
int drmaa_run_job(char *job_id, size_t job_id_len, drmaa_job_template_t *jt, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   dstring jobid;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   if (job_id) 
      sge_dstring_init(&jobid, job_id, job_id_len);
   return japi_run_job(job_id?&jobid:NULL, jt, error_diagnosis?&diag:NULL); 
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
*
*  NOTES
*      MT-NOTE: drmaa_run_bulk_jobs() is MT safe
*******************************************************************************/
int drmaa_run_bulk_jobs(drmaa_string_vector_t **jobids, drmaa_job_template_t *jt, 
      int start, int end, int incr, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_run_bulk_jobs(jobids, jt, start, end, incr, error_diagnosis?&diag:NULL);
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
*
*  NOTES
*      MT-NOTE: drmaa_control() is MT safe
*******************************************************************************/
int drmaa_control(const char *jobid, int action, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_control(jobid, action, error_diagnosis?&diag:NULL);
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
*  MUTEXES
*      japi_session_mutex -> japi_threads_in_session_mutex
*
*  NOTES
*      MT-NOTE: drmaa_synchronize() is MT safe
*******************************************************************************/
int drmaa_synchronize(char *job_ids[], signed long timeout, int dispose, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_synchronize(job_ids, timeout, dispose, error_diagnosis?&diag:NULL);
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
*      MT-NOTE: drmaa_wait() is MT safe
*******************************************************************************/
int drmaa_wait(const char *job_id, char *job_id_out, size_t job_id_out_len, int *stat, signed long timeout, 
   drmaa_string_vector_t **rusage, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   dstring waited_job;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   if (job_id_out) 
      sge_dstring_init(&waited_job, job_id_out, job_id_out_len);
   return japi_wait(job_id, job_id_out?&waited_job:NULL, stat, timeout, rusage, error_diagnosis?&diag:NULL);
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
*
*  NOTES
*      MT-NOTE: drmaa_job_ps() is MT safe
*******************************************************************************/
int drmaa_job_ps(const char *job_id, int *remote_ps, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len);
   return japi_job_ps(job_id, remote_ps, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_strerror() ****************************************************
*  NAME
*     implementation_thread() -- Control flow implementation thread
*
*  SYNOPSIS
*     void drmaa_strerror(int drmaa_errno, char *error_string, int error_len)
*
*  FUNCTION
*     Returns readable text version of errno (constant string)
*
*  INPUTS
*  RESULT
*
*
*  NOTES
*      MT-NOTE: drmaa_strerror() is MT safe
*******************************************************************************/
const char *drmaa_strerror(int drmaa_errno)
{
   return japi_strerror(drmaa_errno);
}

int drmaa_string_vector_get_first(drmaa_string_vector_t* values, char *value, int value_len)
{
   dstring val;
   if (value) 
      sge_dstring_init(&val, value, value_len);
   return japi_string_vector_get_first(values, value?&val:NULL);
}

int drmaa_string_vector_get_next(drmaa_string_vector_t* values, char *value, int value_len)
{
   dstring val;
   if (value) 
      sge_dstring_init(&val, value, value_len);
   return japi_string_vector_get_next(values, value?&val:NULL);
}

void drmaa_delete_string_vector(drmaa_string_vector_t* values)
{
   japi_delete_string_vector(values);
}

