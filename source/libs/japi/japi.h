#ifndef __JAPI_H


#ifdef JAPI_HOOKS
extern int delay_after_submit;
#endif

enum {
   /* -------------- these are relevant to all sections ---------------- */
   DRMAA_ERRNO_SUCCESS = 0, /* Routine returned normally with success. */
   DRMAA_ERRNO_INTERNAL_ERROR, /* Unexpected or internal DRMAA error like memory allocation, system call failure, etc. */
   DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, /* Could not contact DRM system for this request. */
   DRMAA_ERRNO_AUTH_FAILURE, /* The specified request is not processed successfully due to authorization failure. */
   DRMAA_ERRNO_INVALID_ARGUMENT, /* The input value for an argument is invalid. */
   DRMAA_ERRNO_NO_ACTIVE_SESSION, /* Exit routine failed because there is no active session */

   /* -------------- init and exit specific --------------- */
   DRMAA_ERRNO_INVALID_CONTACT_STRING, /* Initialization failed due to invalid contact string. */
   DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR, /* DRMAA could not use the default contact string to connect to DRM system. */
   DRMAA_ERRNO_DRMS_INIT_FAILED, /* Initialization failed due to failure to init DRM system. */
   DRMAA_ERRNO_ALREADY_ACTIVE_SESSION, /* Initialization failed due to existing DRMAA session. */
   DRMAA_ERRNO_DRMS_EXIT_ERROR, /* DRM system disengagement failed. */

   /* ---------------- job attributes specific -------------- */
   DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT, /* The format for the job attribute value is invalid. */
   DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, /* The value for the job attribute is invalid. */
   DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES, /* The value of this attribute is conflicting with a previously set attributes. */

   /* --------------------- job submission specific -------------- */
   DRMAA_ERRNO_TRY_LATER, /* Could not pass job now to DRM system. A retry may succeed however (saturation). */
   DRMAA_ERRNO_DENIED_BY_DRM, /* The DRM system rejected the job. The job will never be accepted due to DRM configuration or job template settings. */

   /* ------------------------------- job control specific ---------------- */
   DRMAA_ERRNO_INVALID_JOB, /* The job specified by the 'jobid' does not exist. */
   DRMAA_ERRNO_RESUME_INCONSISTENT_STATE, /* The job has not been suspended. The RESUME request will not be processed. */
   DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE, /* The job has not been running, and it cannot be suspended. */
   DRMAA_ERRNO_HOLD_INCONSISTENT_STATE, /* The job cannot be moved to a HOLD state. */
   DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE, /* The job is not in a HOLD state. */
   DRMAA_ERRNO_EXIT_TIMEOUT, /* We have encountered a time-out condition for drmaa_synchronize or drmaa_wait. */

   DRMAA_NO_ERRNO
};

#define DRMAA_ERROR_STRING_BUFFER   1024
#define DRMAA_JOBNAME_BUFFER        1024
#define DRMAA_SIGNAL_BUFFER         32
#define DRMAA_TIMEOUT_WAIT_FOREVER  -1
#define DRMAA_TIMEOUT_NO_WAIT       0 

#define DRMAA_JOB_IDS_SESSION_ANY "*"

/* names of job template attributes */
#define DRMAA_REMOTE_COMMAND         "drmaa_remote_command"
#define DRMAA_JS_STATE               "drmaa_js_state"
#define DRMAA_WD                     "drmaa_wd"
#define DRMAA_JOB_CATEGORY           "drmaa_job_category"
#define DRMAA_NATIVE_SPECIFICATION   "drmaa_native_specification"
#define DRMAA_BLOCK_EMAIL            "drmaa_block_email"
#define DRMAA_START_TIME             "drmaa_start_time"
#define DRMAA_JOB_NAME               "drmaa_job_name"
#define DRMAA_INPUT_PATH             "drmaa_input_path"
#define DRMAA_OUTPUT_PATH            "drmaa_output_path"
#define DRMAA_ERROR_PATH             "drmaa_error_path"
#define DRMAA_JOIN_FILES             "drmaa_join_files"
#define DRMAA_TRANSFER_FILES         "drmaa_transfer_files"
#define DRMAA_DEADLINE_TIME          "drmaa_deadline_time"
#define DRMAA_WCT_HLIMIT             "drmaa_wct_hlimit"
#define DRMAA_WCT_SLIMIT             "drmaa_wct_slimit"
#define DRMAA_DURATION_HLIMIT        "drmaa_durartion_hlimit"
#define DRMAA_DURATION_SLIMIT        "drmaa_durartion_slimit"

/* names of job template vector attributes */
#define DRMAA_V_ARGV                 "drmaa_v_argv"
#define DRMAA_V_ENV                  "drmaa_v_env"
#define DRMAA_V_EMAIL                "drmaa_v_email"

typedef struct job_template_s job_template_t;

/*
 * Initialize DRMAA API library and create a new DRMAA Session. 'Contact'
 * is an implementation dependent string which may be used to specify
 * which DRM system to use. This routine must be called before any
 * other DRMAA calls, except for drmaa_version().
 * If 'contact' is NULL, the default DRM system will be used.
 */ 
int drmaa_init(const char *contact);


/*
 * Disengage from DRMAA library and allow the DRMAA library to perform
 * any necessary internal clean up.
 * This routine ends this DRMAA Session, but does not effect any jobs (e.g.,
 * queued and running jobs remain queued and running).
 */
int drmaa_exit(void);

/* ------------------- job template routines ------------------- */

/* 
 * Allocate a new job template. 
 */
int drmaa_allocate_job_template(job_template_t **jt);

/* 
 * Deallocate a job template. This routine has no effect on jobs.
 */
int drmaa_delete_job_template(job_template_t *jt);


/* 
 * Adds ('name', 'value') pair to list of attributes in job template 'jt'.
 * Only non-vector attributes may be passed.
 */
int drmaa_set_attribute(job_template_t *jt, const char *name, const char *value);


/* 
 * If 'name' is an existing non-vector attribute name in the job 
 * template 'jt', then the value of 'name' is returned; otherwise, 
 * NULL is returned.
 */ 
int drmaa_get_attribute(job_template_t *jt, const char *name, char *value, size_t value_size);

/* Adds ('name', 'values') pair to list of vector attributes in job template 'jt'.
 * Only vector attributes may be passed.
 */
int drmaa_set_vector_attribute(job_template_t *jt, const char *name, char *value[]);


/* 
 * If 'name' is an existing vector attribute name in the job template 'jt',
 * then the values of 'name' are returned; otherwise, NULL is returned.
 */
int drmaa_get_vector_attribute(job_template_t *jt, const char *name /* , vector of attribute values */ );


/* 
 * Returns the set of supported attribute names whose associated   
 * value type is String. This set will include supported DRMAA reserved 
 * attribute names and native attribute names. 
 */
int drmaa_get_attribute_names( void /* vector of attribute name (string vector) */);

/*
 * Returns the set of supported attribute names whose associated 
 * value type is String Vector.  This set will include supported DRMAA reserved 
 * attribute names and native attribute names. */
int drmaa_get_vector_attribute_names(void /* vector of attribute name (string vector) */);

/* ------------------- job submission routines ------------------- */

/*
 * Submit a job with attributes defined in the job template 'jt'.
 * The job identifier 'job_id' is a printable, NULL terminated string,
 * identical to that returned by the underlying DRM system.
 */
int drmaa_run_job(char *job_id, int job_id_size, job_template_t *jt, 
      char *error_diagnosis, int error_diag_len);

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
int drmaa_run_bulk_jobs(char *job_ids[], job_template_t *jt, int start, int end, int incr);

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
int drmaa_control(const char *jobid, int action);


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
int drmaa_synchronize(char *job_ids[], signed long timeout, int dispose);


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
int drmaa_wait(const char *job_id, char *job_id_out, int job_id_size, int *stat, signed long timeout, char *rusage[]);

#if 0
drmaa_wifexited(OUT exited, IN stat,  INOUT drmaa_context_error_buf)
    Evaluates into 'exited' a non-zero value if stat was returned for a
    job that terminated normally. A zero value can also indicate that
    altough the job has terminated normally an exit status is not available
    or that it is not known whether the job terminated normally. In both
    cases drmaa_wexitstatus() will not provide exit status information.
    A non-zero 'exited' value indicates more detailed diagnosis can be provided
    by means of drmaa_wifsignaled(), drmaa_wtermsig() and drmaa_wcoredump().

drmaa_wexitstatus(OUT exited, IN stat,  INOUT drmaa_context_error_buf)
     If the OUT parameter 'exited' of drmaa_wifexited() is non-zero,
     this function evaluates into 'exit_code' the exit code that the
     job passed to _exit() (see exit(2)) or exit(3C), or the value that
     the child process returned from main.

drmaa_wifsignaled(OUT signaled, IN stat, INOUT drmaa_context_error_buf )
     Evaluates into 'signaled' a non-zero value if status was returned
     for a job that terminated due to the receipt of a signal. A zero value
     can also indicate that altough the job has terminated due to the receipt
     of a signal the signal is not available or that it is not known whether
     the job terminated due to the receipt of a signal. In both cases
     drmaa_wtermsig() will not provide signal information.

drmaa_wtermsig(OUT signal, IN stat, INOUT drmaa_context_error_buf )
     If the OUT parameter 'signaled' of drmaa_wifsignaled(stat) is
     non-zero, this function evaluates into signal a string representation of the signal
     that caused the termination of the job. For signals declared by POSIX, the symbolic
     names are returned (e.g., SIGABRT, SIGALRM).
     For signals not declared by POSIX, any other string may be returned.

drmaa_wcoredump(OUT core_dumped, IN stat, INOUT drmaa_context_error_buf )
     If the OUT parameter 'signaled' of drmaa_wifsignaled(stat) is
     non-zero, this function evaluates into 'core_dumped' a non-zero value
     if a core image of the terminated job was created.

drmaa_wifaborted( OUT aborted, IN stat, INOUT drmaa_context_error_buf )
      Evaluates into 'aborted' a non-zero value if 'stat'
      was returned for a job that ended before entering the running state.
#endif


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
int drmaa_job_ps( const char *job_id, int *remote_ps);

const char *drmaa_strerror(int drmaa_errno);

/*
contact drmaa_get_contact();
OUT contact Current contact information for DRM system (string)
*/

#endif /* __JAPI_H */
