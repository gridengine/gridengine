#ifndef __JAPI_H
#define __JAPI_H

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

#include "drmaa.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* ------------------- init/exit routines ------------------- */
/*
 * Initialize DRMAA API library and create a new DRMAA Session. 'Contact'
 * is an implementation dependent string which may be used to specify
 * which DRM system to use. This routine must be called before any
 * other DRMAA calls, except for japi_version().
 * If 'contact' is NULL, the default DRM system will be used.
 */ 
int japi_init(const char *contact, char *error_diagnosis, size_t error_diag_len);


/*
 * Disengage from DRMAA library and allow the DRMAA library to perform
 * any necessary internal clean up.
 * This routine ends this DRMAA Session, but does not effect any jobs (e.g.,
 * queued and running jobs remain queued and running).
 */
int japi_exit(char *error_diagnosis, size_t error_diag_len);

/* ------------------- job template routines ------------------- */

/* 
 * Allocate a new job template. 
 */
int japi_allocate_job_template(drmaa_job_template_t **jt, char *error_diagnosis, size_t error_diag_len);

/* 
 * Deallocate a job template. This routine has no effect on jobs.
 */
int japi_delete_job_template(drmaa_job_template_t *jt, char *error_diagnosis, size_t error_diag_len);


/* 
 * Adds ('name', 'value') pair to list of attributes in job template 'jt'.
 * Only non-vector attributes may be passed.
 */
int japi_set_attribute(drmaa_job_template_t *jt, const char *name, const char *value, char *error_diagnosis, size_t error_diag_len);


/* 
 * If 'name' is an existing non-vector attribute name in the job 
 * template 'jt', then the value of 'name' is returned; otherwise, 
 * NULL is returned.
 */ 
int japi_get_attribute(drmaa_job_template_t *jt, const char *name, char *value, size_t value_len, char *error_diagnosis, size_t error_diag_len);

/* Adds ('name', 'values') pair to list of vector attributes in job template 'jt'.
 * Only vector attributes may be passed.
 */
int japi_set_vector_attribute(drmaa_job_template_t *jt, const char *name, char *value[], char *error_diagnosis, size_t error_diag_len);


/* 
 * If 'name' is an existing vector attribute name in the job template 'jt',
 * then the values of 'name' are returned; otherwise, NULL is returned.
 */
int japi_get_vector_attribute(drmaa_job_template_t *jt, const char *name, /* vector of attribute values (string vector), */ char *error_diagnosis, size_t error_diag_len);


/* 
 * Returns the set of supported attribute names whose associated   
 * value type is String. This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 */
int japi_get_attribute_names( /* vector of attribute name (string vector), */ char *error_diagnosis, size_t error_diag_len);

/*
 * Returns the set of supported attribute names whose associated 
 * value type is String Vector.  This set will include supported DRMAA reserved 
 * attribute names and native attribute names. */
int japi_get_vector_attribute_names(/* vector of attribute name (string vector), */ char *error_diagnosis, size_t error_diag_len);

/* ------------------- job submission routines ------------------- */

/*
 * Submit a job with attributes defined in the job template 'jt'.
 * The job identifier 'job_id' is a printable, NULL terminated string,
 * identical to that returned by the underlying DRM system.
 */
int japi_run_job(char *job_id, size_t job_id_len, drmaa_job_template_t *jt, char *error_diagnosis, size_t error_diag_len);

/* 
 * Submit a set of parametric jobs, dependent on the implied loop index, each
 * with attributes defined in the job template 'jt'.
 * The job identifiers 'job_ids' are all printable,
 * NULL terminated strings, identical to those returned by the underlying
 * DRM system. Nonnegative loop bounds are mandated to avoid file names
 * that start with minus sign like command line options.
 * The special index placeholder is a DRMAA defined string
 * drmaa_incr_ph == $incr_pl$
 * that is used to construct parametric job templates.
 * For example:
 * drmaa_set_attribute(pjt, "stderr", drmaa_incr_ph + ".err" ); (C++/java string syntax used)
 */
int japi_run_bulk_jobs( /* vector of job ids (string vector), */ drmaa_job_template_t *jt, int start, int end, int incr, char *error_diagnosis, size_t error_diag_len);

/* ------------------- job control routines ------------------- */

/*
 * Start, stop, restart, or kill the job identified by 'job_id'.
 * If 'job_id' is DRMAA_JOB_IDS_SESSION_ALL, then this routine
 * acts on all jobs *submitted* during this DRMAA session.
 * The legal values for 'action' and their meanings are:
 * DRMAA_CONTROL_SUSPEND: stop the job,
 * DRMAA_CONTROL_RESUME: (re)start the job,
 * DRMAA_CONTROL_HOLD: put the job on-hold,
 * DRMAA_CONTROL_RELEASE: release the hold on the job, and
 * DRMAA_CONTROL_TERMINATE: kill the job.
 * This routine returns once the action has been acknowledged by
 * the DRM system, but does not necessarily wait until the action
 * has been completed.
 */
int japi_control(const char *jobid, int action, char *error_diagnosis, size_t error_diag_len);


/* 
 * Wait until all jobs specified by 'job_ids' have finished
 * execution. If 'job_ids' is DRMAA_JOB_IDS_SESSION_ALL, then this routine
 * waits for all jobs *submitted* during this DRMAA session. To prevent
 * blocking indefinitely in this call the caller could use timeout specifying
 * after how many seconds to time out in this call.
 * If the call exits before timeout all the jobs have been waited on
 * or there was an interrupt.
 * If the invocation exits on timeout, the return code is DRMAA_ERRNO_EXIT_TIMEOUT.
 * The caller should check system time before and after this call
 * in order to check how much time has passed.
 * Dispose parameter specifies how to treat reaping information:
 * True=1 "fake reap", i.e. dispose of the rusage data
 * False=0 do not reap
 */ 
int japi_synchronize(char *job_ids[], signed long timeout, int dispose, char *error_diagnosis, size_t error_diag_len);


/* 
 * This routine waits for a job with job_id to fail or finish execution. Passing a special string
 * DRMAA_JOB_IDS_SESSION_ANY instead job_id waits for any job. If such a job was
 * successfully waited its job_id is returned as a second parameter. This routine is
 * modeled on wait3 POSIX routine. To prevent
 * blocking indefinitely in this call the caller could use timeout specifying
 * after how many seconds to time out in this call.
 * If the call exits before timeout the job has been waited on
 * successfully or there was an interrupt.
 * If the invocation exits on timeout, the return code is DRMAA_ERRNO_EXIT_TIMEOUT.
 * The caller should check system time before and after this call
 * in order to check how much time has passed.
 * The routine reaps jobs on a successful call, so any subsequent calls
 * to drmaa_wait should fail returning an error DRMAA_ERRNO_INVALID_JOB meaning
 * that the job has been already reaped. This error is the same as if the job was
 * unknown. Failing due to an elapsed timeout has an effect that it is possible to
 * issue drmaa_wait multiple times for the same job_id.
 */
int japi_wait(const char *job_id, char *job_id_out, size_t job_id_out_len, int *stat, 
   signed long timeout, /* vector of rusage strings (string vector), */
   char *error_diagnosis, size_t error_diagnois_len);

/* 
 * Evaluates into 'exited' a non-zero value if stat was returned for a
 * job that terminated normally. A zero value can also indicate that
 * altough the job has terminated normally an exit status is not available
 * or that it is not known whether the job terminated normally. In both
 * cases japi_wexitstatus() will not provide exit status information.
 * A non-zero 'exited' value indicates more detailed diagnosis can be provided
 * by means of japi_wifsignaled(), japi_wtermsig() and japi_wcoredump(). 
 */
int japi_wifexited(int *exited, int stat, char *error_diagnosis, size_t error_diag_len);

/* 
 * If the OUT parameter 'exited' of japi_wifexited() is non-zero,
 * this function evaluates into 'exit_code' the exit code that the
 * job passed to _exit() (see exit(2)) or exit(3C), or the value that
 * the child process returned from main. 
 */
int japi_wexitstatus(int *exit_status, int stat, char *error_diagnosis, size_t error_diag_len);

/* 
 * Evaluates into 'signaled' a non-zero value if status was returned
 * for a job that terminated due to the receipt of a signal. A zero value
 * can also indicate that altough the job has terminated due to the receipt
 * of a signal the signal is not available or that it is not known whether
 * the job terminated due to the receipt of a signal. In both cases
 * japi_wtermsig() will not provide signal information. 
 */
int japi_wifsignaled(int *signaled, int stat, char *error_diagnosis, size_t error_diag_len);

/* 
 * If the OUT parameter 'signaled' of japi_wifsignaled(stat) is
 * non-zero, this function evaluates into signal a string representation of the signal
 * that caused the termination of the job. For signals declared by POSIX, the symbolic
 * names are returned (e.g., SIGABRT, SIGALRM).
 * For signals not declared by POSIX, any other string may be returned. 
 */
int japi_wtermsig(char *signal, size_t signal_len, int stat, char *error_diagnosis, size_t error_diag_len);

/* 
 * If the OUT parameter 'signaled' of japi_wifsignaled(stat) is
 * non-zero, this function evaluates into 'core_dumped' a non-zero value
 * if a core image of the terminated job was created. 
 */
int japi_wcoredump(int *core_dumped, int stat, char *error_diagnosis, size_t error_diag_len);

/* 
 * Evaluates into 'aborted' a non-zero value if 'stat'
 * was returned for a job that ended before entering the running state. 
 */
int japi_wifaborted(int *aborted, int stat, char *error_diagnosis, size_t error_diag_len);



/* 
 * Get the program status of the job identified by 'job_id'.
 * The possible values returned in 'remote_ps' and their meanings are:
 * DRMAA_PS_UNDETERMINED = 00H : process status cannot be determined,
 * DRMAA_PS_QUEUED_ACTIVE = 10H : job is queued and active,
 * DRMAA_PS_SYSTEM_ON_HOLD = 11H : job is queued and in system hold,
 * DRMAA_PS_USER_ON_HOLD = 12H : job is queued and in user hold,
 * DRMAA_PS_USER_SYSTEM_ON_HOLD = 13H : job is queued and in user and system hold,
 * DRMAA_PS_RUNNING = 20H : job is running,
 * DRMAA_PS_SYSTEM_SUSPENDED = 21H : job is system suspended,
 * DRMAA_PS_USER_SUSPENDED = 22H : job is user suspended,
 * DRMAA_PS_USER_SYSTEM_SUSPENDED = 23H : job is user and system suspended,
 * DRMAA_PS_DONE = 30H : job finished normally, and
 * DRMAA_PS_FAILED = 40H : job finished, but failed.
 */
int japi_job_ps( const char *job_id, int *remote_ps, char *error_diagnosis, size_t error_diag_len);

/* ------------------- auxiliary routines ------------------- */
 
/*
 * Get the error message text associated with the errno number. 
 */
const char *japi_strerror(int drmaa_errno);

/* 
 * Current contact information for DRM system (string)
 */ 
void japi_get_contact(char *contact, size_t contact_len);

/* 
 * OUT major - major version number (non-negative integer)
 * OUT minor - minor version number (non-negative integer)
 * Returns the major and minor version numbers of the DRMAA library;
 * for DRMAA 1.0, 'major' is 1 and 'minor' is 0. 
 */
void japi_version(unsigned int *major, unsigned int *minor);


/* 
 * returns DRM system implementation information
 * Output (string) is implementation dependent and could contain the DRM system and the
 * implementation vendor as its parts.
 */
void japi_get_DRM_system(char *drm_system, size_t drm_system_len);

#ifdef  __cplusplus
}
#endif

#endif /* __JAPI_H */
