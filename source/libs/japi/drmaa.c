#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <pthread.h>

#include "sge_mtutil.h"

#include "drmaa.h"
#include "japi.h"
#include "japiP.h"

#include "sge_answer.h"

/* CLIENTS/COMMON */
#include "read_defaults.h"
#include "show_job.h" //For debug only.  Please remove!

/* COMMON */
#include "parse_job_cull.h"
#include "parse_qsub.h"
#include "sge_options.h"
#include "symbols.h"

/* GDI */
#include "sge_qtcsh.h"

/* UTI */
#include "sge_dstring.h"
#include "sge_prog.h"
#include "sge_string.h"

/* RMON */
#include "sgermon.h"

/* SGEOBJ */
#include "sge_path_alias.h"
#include "sge_job.h"
#include "parse_qsubL.h"
#include "parse.h"
#include "sge_mailrecL.h"
#include "sge_ulong.h"

/* OBJ */
#include "sge_varL.h"
#include "sge_jobL.h"
#include "sge_ja_taskL.h"
#include "sge_japiL.h"
#include "sge_strL.h"

/****** DRMAA/--DRMAA_Job_API ********************************************************
*  NAME
*     DRMAA_Job_API -- Grid Engine's C/C++ binding for the DRMAA interface 
*
*  FUNCTION
*     This libary gives a C/C++ binding for the DRMAA interface specification. 
*     The DRMAA interface is independed of a DRM system and thus can be implememted 
*     by not only for Grid Engine. It's scope is job submission and control.
*     Refer to www.drmaa.org for more about this interface.
*
*  SEE ALSO
*     DRMAA/-DRMAA_Session_state
*     DRMAA/-DRMAA_Implementation
*     DRMAA/-DRMAA_Interface 
*     DRMAA/-DRMAA_Global_Constants
*******************************************************************************/

/****** DRMAA/-DRMAA_Implementation *******************************************
*  NAME
*     DRMAA_Implementation -- Functions used to implement Grid Engine DRMAA
* 
*  FUNCTION
*     These functions are used to implement DRMAA functions. Most of the 
*     functionality of DRMAA is derived from Grid Engine's Job API.
*   
*  SEE ALSO
*     DRMAA/drmaa_job2sge_job()
*     DRMAA/japi_drmaa_path2sge_job()
*     JAPI/--Job_API
*******************************************************************************/

/* Defined in rshd.c */
extern char **environ;

static const char *session_key_env_var = "SGE_SESSION_KEY"; 
static const char *session_shutdown_mode_env_var = "SGE_KEEP_SESSION";

static int drmaa_is_supported(const char *name, const char *supported_list[]);
static drmaa_attr_names_t *drmaa_fill_string_vector(const char *name[]);

static int drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t *drmaa_jt, 
   int is_bulk, int start, int end, int step, dstring *diag);
/*
static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, lListElem *jt, int is_bulk, 
   int nm, const char *attribute_key, dstring *diag);
*/
static int japi_drmaa_path2path_opt(lList *attrs, lList **args,
   int is_bulk, const char *attribute_key, const char *sw, int opt, dstring *diag);
static int japi_drmaa_path2wd_opt(lList *attrs, lList **args, int is_bulk, 
   dstring *diag);
static int japi_drmaa_path2sge_path(lList *attrs, int is_bulk,
   const char *attribute_key, int do_wd, const char **new_path, dstring *diag);
static void prune_arg_list (lList *args);
static void opt_list_append_default_drmaa_opts (lList **opts);
static void merge_drmaa_options (lList **opts_all, lList **opts_default,
                                 lList **opts_defaults, lList **opts_scriptfile,
                                 lList **opts_job_cat, lList **opts_native,
                                 lList **opts_drmaa);
static int opt_list_append_opts_from_drmaa_attr (lList **args, lList *attrs,
   lList *vattrs, int is_bulk, dstring *diag);
char *drmaa_time2sge_time (const char *drmaa_time, dstring *diag);

/****** DRMAA/-DRMAA_Session_state *******************************************
*  NAME
*     DRMAA_Session_state -- The state of the DRMAA library.
*
*  SYNOPSIS
*     extern char **environ 
*        The environment is used to pass arguments forth and back trough
*        DRMAA interface calls without actually changing the link interface.
*        This makes 'environ' part of the DRMAA session state and access to
*        it is guarded with the mutex 'environ_mutex'.
*
*  SEE ALSO
*     JAPI/-JAPI_Session_state
*******************************************************************************/

static pthread_mutex_t environ_mutex = PTHREAD_MUTEX_INITIALIZER;

#define DRMAA_LOCK_ENVIRON()      sge_mutex_lock("drmaa_environ_mutex", SGE_FUNC, __LINE__, &environ_mutex)
#define DRMAA_UNLOCK_ENVIRON()    sge_mutex_unlock("drmaa_environ_mutex", SGE_FUNC, __LINE__, &environ_mutex)

/****** DRMAA/-DRMAA_Global_Constants *******************************************
*  NAME
*     Global_Constants -- global constants used in DRMAA
*
*  SYNOPSIS
*     static const char *drmaa_supported_nonvector[];
*     static const char *drmaa_supported_vector[];
*     
*  FUNCTION
*     drmaa_supported_nonvector - A string array containing all supported job 
*                    template non-vector attributes.
*     drmaa_supported_vector - A string array containing all supported job 
*                    template vector attributes.
*  SEE ALSO
*******************************************************************************/
/* these non vector job template attributes are supported */
static const char *drmaa_supported_nonvector[] = {
   DRMAA_REMOTE_COMMAND,        /* mandatory */
   DRMAA_JS_STATE,              /* mandatory */
   DRMAA_WD,                    /* mandatory */
   DRMAA_JOB_NAME,              /* mandatory */
   DRMAA_INPUT_PATH,            /* mandatory */
   DRMAA_OUTPUT_PATH,           /* mandatory */
   DRMAA_ERROR_PATH,            /* mandatory */
   DRMAA_JOIN_FILES,            /* mandatory */
   DRMAA_JOB_CATEGORY,          /* mandatory */
   DRMAA_NATIVE_SPECIFICATION,  /* mandatory */
   DRMAA_BLOCK_EMAIL,           /* mandatory */
   DRMAA_START_TIME,            /* mandatory */
   DRMAA_TRANSFER_FILES,        /* optional */
   DRMAA_DEADLINE_TIME,         /* optional */
   DRMAA_WCT_HLIMIT,            /* optional */
   DRMAA_WCT_SLIMIT,            /* optional */
   DRMAA_DURATION_HLIMIT,       /* optional */
   DRMAA_DURATION_SLIMIT,       /* optional */
   NULL
};

/* these vector job template attributes are supported */
static const char *drmaa_supported_vector[] = {
   DRMAA_V_ARGV,  /* mandatory */
   DRMAA_V_ENV,   /* mandatory */
   DRMAA_V_EMAIL, /* mandatory */
   NULL
};

/****** DRMAA/drmaa_init() ****************************************************
*  NAME
*     drmaa_init() -- Initialize DRMAA API library
*
*  SYNOPSIS
*     int drmaa_init(const char *contact, char *error_diagnosis, 
*               size_t error_diag_len)
*
*  FUNCTION
*     Initialize DRMAA API library and create a new DRMAA Session. 'Contact'
*     is an implementation dependent string which may be used to specify
*     which DRM system to use. This routine must be called before any other 
*     DRMAA calls, except for drmaa_version(). If 'contact' is NULL, the default 
*     DRM system will be used. Initializes internal data structures and registers 
*     with qmaster using the event client mechanisms.
*
*  INPUTS
*     const char *contact                    - contact string
*     char *error_diagnosis                  - diagnosis buffer
*     size_t error_diag_len                  - diagnosis buffer length
*     env var SGE_SESSION_KEY - dirty input/output interface to parametrize
*                    without actually changing DRMAA library link interface. 
*                    The string passed before drmaa_init() will be used as session
*                    key for restarting the former Grid Engine JAPI session. After
*                    drmaa_init() this env var contains the session key that is used
*                    with this Grid Engine JAPI session.
*                    
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success otherwise 
*           DRMAA_ERRNO_INVALID_CONTACT_STRING, 
*           DRMAA_ERRNO_ALREADY_ACTIVE_SESSION, or
*           DRMAA_ERRNO_DEFAULT_CONTACT_STRING_ERROR.
* 
*  NOTES
*      MT-NOTE: drmaa_init() is MT safe
*******************************************************************************/
int drmaa_init(const char *contact, char *error_diagnosis, size_t error_diag_len)
{
   int ret;
   dstring diag, session_key_in = DSTRING_INIT, 
           session_key_out = DSTRING_INIT, env_var = DSTRING_INIT;
   dstring *diagp = NULL;
   bool set_session;

   DENTER(TOP_LAYER, "drmaa_init");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }
 
   DRMAA_LOCK_ENVIRON();
   set_session = getenv(session_key_env_var)?true:false;
   if (set_session)
      sge_dstring_copy_string(&session_key_in, getenv(session_key_env_var));
   DRMAA_UNLOCK_ENVIRON();

   /*
    * env var SGE_SESSION_KEY is used 
    * to interface JAPI session key forth ... 
    */
   ret = japi_init(contact, set_session?sge_dstring_get_string(&session_key_in):NULL, 
        &session_key_out, diagp);

   if (set_session)
      sge_dstring_free(&session_key_in);

   if (ret != DRMAA_ERRNO_SUCCESS) {
      /* diag was set in japi_init() */
      return ret;
   }

   /*
    * ... and back to pass resulting session key back to application 
    */
   sge_dstring_sprintf(&env_var, "%s=%s", session_key_env_var, 
         sge_dstring_get_string(&session_key_out));


   DRMAA_LOCK_ENVIRON();
   sge_putenv(sge_dstring_get_string(&env_var));
   DRMAA_UNLOCK_ENVIRON();

   sge_dstring_free(&session_key_out);
   sge_dstring_free(&env_var);

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_exit() ****************************************************
*  NAME
*     drmaa_exit() -- Shutdown DRMAA API library
*
*  SYNOPSIS
*     int drmaa_exit(char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Disengage from DRMAA library and allow the DRMAA library to perform
*     any necessary internal clean up.
*     This routine ends this DRMAA Session, but does not effect any jobs (e.g.,
*     queued and running jobs remain queued and running).
*
*  INPUTS
*     char *error_diagnosis   - diagnosis buffer
*     size_t error_diag_len   - diagnosis buffer length
*     env var SGE_KEEP_SESSION 
*                             - dirty input interface to make sessions restartable 
*                               without actually changing DRMAA library link interface.
*                               If set the session is not cleaned up (default), otherwise 
*                               it is closed.
*     
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success, otherwise DRMAA_ERRNO_DRMS_EXIT_ERROR 
*           or DRMAA_ERRNO_NO_ACTIVE_SESSION.
*
*  NOTES
*      MT-NOTE: drmaa_exit() is MT safe
*******************************************************************************/
int drmaa_exit(char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;
   const char *s;
   int drmaa_errno;
   bool close_session = true;
   extern int sge_clrenv(const char *name);

   DENTER(TOP_LAYER, "drmaa_exit");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   /* session_shutdown_mode_env_var is used to interface japi_exit(close_session) */
   DRMAA_LOCK_ENVIRON();
   if ((s=getenv(session_shutdown_mode_env_var))) {
      close_session = false;
   }
   DRMAA_UNLOCK_ENVIRON();

   drmaa_errno = japi_exit(close_session, diagp);

   /* need to get rid of session key env var */
   DRMAA_LOCK_ENVIRON();
   sge_clrenv(session_key_env_var);
   DRMAA_UNLOCK_ENVIRON();

   DEXIT;
   return drmaa_errno;
}

/****** DRMAA/drmaa_allocate_job_template() *************************************
*  NAME
*     drmaa_allocate_job_template() -- Allocate a new job template. 
*
*  SYNOPSIS
*     int drmaa_allocate_job_template(drmaa_job_template_t **jtp, 
*                     char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Allocate a new job template.
*
*  OUTPUT
*     drmaa_job_template_t **jtp             - The new job template 
*     char *error_diagnosis                  - diagnosis buffer
*     size_t error_diag_len                  - diagnosis buffer length
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE or 
*           DRMAA_ERRNO_INTERNAL_ERROR. 
*
*  NOTES
*      MT-NOTE: drmaa_allocate_job_template() is MT safe
*******************************************************************************/
int drmaa_allocate_job_template(drmaa_job_template_t **jtp, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;

   DENTER(TOP_LAYER, "drmaa_allocate_job_template");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!jtp) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   } 
 
   *jtp = (drmaa_job_template_t *)malloc(sizeof(drmaa_job_template_t));
   (*jtp)->strings = (*jtp)->string_vectors = NULL;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_delete_job_template() ***************************************
*  NAME
*     drmaa_delete_job_template() -- Deallocate a job template
*
*  SYNOPSIS
*     int drmaa_delete_job_template(drmaa_job_template_t *jt, 
*                    char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Deallocate a job template. This routine has no effect on jobs.
*
*  INPUTS
*     drmaa_job_template_t *jt               - job template to be deleted
* 
*  OUTPUTS
*     char *error_diagnosis                  - diagnosis buffer
*     size_t error_diag_len                  - diagnosis buffer length
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE or DRMAA_ERRNO_INTERNAL_ERROR. 
*
*  NOTES
*      MT-NOTE: drmaa_delete_job_template() is MT safe
*******************************************************************************/
int drmaa_delete_job_template(drmaa_job_template_t *jt, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;

   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);

   if (!jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, &diag);
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   } 
 
   jt->strings = lFreeList(jt->strings);
   jt->string_vectors = lFreeList(jt->string_vectors);
   free(jt); 
   jt = NULL;

   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_fill_string_vector() ***************************************
*  NAME
*     drmaa_fill_string_vector() -- Returns values in 'name' as string vector
*
*  SYNOPSIS
*     static drmaa_attr_names_t* drmaa_fill_string_vector(const char *name[]) 
*
*  FUNCTION
*     Returns values in 'name' as string vector
*
*  INPUTS
*     const char *name[] - The name vector.
*
*  RESULT
*     static drmaa_attr_names_t* - The string vector as it is used for DRMAA.
*
*  NOTES
*     MT-NOTES: drmaa_fill_string_vector() is MT safe
*******************************************************************************/
static drmaa_attr_names_t *drmaa_fill_string_vector(const char *name[])
{
   drmaa_attr_names_t *vector;
   int i;

   DENTER(TOP_LAYER, "drmaa_fill_string_vector");

   /* allocate iterator */
   if (!(vector=(drmaa_attr_names_t *)japi_allocate_string_vector(JAPI_ITERATOR_STRINGS))) {
      DEXIT;
      return NULL;
   }

   /* copy all attribute names */
   for (i=0; name[i]; i++) {
      DPRINTF(("adding \"%s\"\n", name[i]));
      if (!lAddElemStr(&(vector->it.si.strings), ST_name, name[i], ST_Type)) {
         japi_delete_string_vector((drmaa_attr_values_t *)vector);
         DEXIT;
         return NULL;
      } 
   }
   
   /* initialize iterator */
   vector->it.si.next_pos = lFirst(vector->it.si.strings);

   DEXIT;
   return vector;
}

static int drmaa_is_supported(const char *name, const char *supported_list[])
{
   int i;
   for (i=0; supported_list[i]; i++) {
      if (!strcmp(name, supported_list[i]))
         return 1;
   }
   return 0;
}

/****** DRMAA/drmaa_set_attribute() *********************************************
*  NAME
*     drmaa_set_attribute() -- Set non vector attribute in job template
*
*  SYNOPSIS
*     int drmaa_set_attribute(drmaa_job_template_t *jt, const char *name, 
*            const char *value, char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Adds ('name', 'value') pair to list of attributes in job template 'jt'.
*     Only non-vector attributes may be passed.
*
*  INPUTS
*     drmaa_job_template_t *jt - job template
*     const char *name         - name 
*     const char *value        - value
*
*  OUTPUTS
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*         DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT, DRMAA_ERRNO_INVALID_ARGUMENT, 
*         DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, or 
*         DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES
*
*  NOTES
*      MT-NOTE: drmaa_set_attribute() is MT safe
*******************************************************************************/
int drmaa_set_attribute(drmaa_job_template_t *jt, const char *name, const char *value, 
      char *error_diagnosis, size_t error_diag_len)
{
   lListElem *ep;

   dstring diag, *diagp = NULL;
   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   if (drmaa_is_supported(name, drmaa_supported_nonvector)) {
      /* verify value */

      /* join files must be either 'y' or 'n' */
      if (!strcmp(name, DRMAA_JOIN_FILES)) {
         if (strlen(value)!=1 || (value[0] != 'y' && value[0] != 'n' )) {
            if (diagp) 
               sge_dstring_sprintf(diagp, "attribute "SFQ" must be either "SFQ" or "SFQ"\n", 
                     DRMAA_JOIN_FILES, "y", "n");
            return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
         }
      }

      /* submission state must be either active or hold */
      if (!strcmp(name, DRMAA_JS_STATE)) {
         if (strcmp(value, DRMAA_SUBMISSION_STATE_ACTIVE)
            && strcmp(value, DRMAA_SUBMISSION_STATE_HOLD)) {
            if (diagp) 
               sge_dstring_sprintf(diagp, "attribute "SFQ" must be either "SFQ" or "SFQ"\n", 
                     DRMAA_JS_STATE, DRMAA_SUBMISSION_STATE_ACTIVE, DRMAA_SUBMISSION_STATE_HOLD);
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

   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_get_attribute() **********************************************
*  NAME
*     drmaa_get_attribute() -- Return job template attribute value.
*
*  SYNOPSIS
*     int drmaa_get_attribute(drmaa_job_template_t *jt, const char *name, char *value, 
*        size_t value_len, char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     If 'name' is an existing non-vector attribute name in the job 
*     template 'jt', then the value of 'name' is returned; otherwise, 
*     DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE is returned.
*
*  INPUTS
*     drmaa_job_template_t *jt - the job template
*     const char *name         - the attribute name
*  
*  OUTPUTS
*     char *value              - value buffer
*     size_t value_len         - value buffer length
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE.
*
*  NOTES
*     MT-NOTE: drmaa_get_attribute() is MT safe
*******************************************************************************/
int drmaa_get_attribute(drmaa_job_template_t *jt, const char *name, char *value, 
   size_t value_len, char *error_diagnosis, size_t error_diag_len)
{
   dstring val, diag, *diagp = NULL;
   lListElem *va;

   DENTER(TOP_LAYER, "drmaa_get_attribute");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!value || !name || !jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   sge_dstring_init(&val, value, value_len+1);

   /* search name in string_vectors */
   if (!(va = lGetElemStr(jt->strings, VA_variable, name))) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
   }
  
   sge_dstring_copy_string(&val, lGetString(va, VA_value));
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_set_vector_attribute() ***************************************
*  NAME
*     drmaa_set_vector_attribute() -- Set vector attribute in job template
*
*  SYNOPSIS
*     int drmaa_set_vector_attribute(drmaa_job_template_t *jt, const char *name, 
*           const char *value[], char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Adds ('name', 'values') pair to list of vector attributes in job template 
*     'jt'. Only vector attributes may be passed. 
*
*  INPUTS
*     drmaa_job_template_t *jt - job template
*     const char *name         - attribute name
*     char *value[]            - array of string values
*
*  OUTPUTS
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*          DRMAA_ERRNO_INVALID_ATTRIBUTE_FORMAT, 
*          DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE,
*          DRMAA_ERRNO_CONFLICTING_ATTRIBUTE_VALUES.
*
*  NOTES
*      MT-NOTE: drmaa_set_vector_attribute() is MT safe
*******************************************************************************/
int drmaa_set_vector_attribute(drmaa_job_template_t *jt, const char *name, 
      const char *value[], char *error_diagnosis, size_t error_diag_len)
{
   lListElem *sep, *ep;
   lList *lp;
   dstring diag, *diagp = NULL;
   int i;

   DENTER(TOP_LAYER, "drmaa_set_vector_attribute");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!jt || !name || !value ) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   if (!drmaa_is_supported(name, drmaa_supported_vector)) {
      DPRINTF(("setting not supported attribute \"%s\"\n", name));
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
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

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRAMA/drmaa_get_vector_attribute() ***************************************
*  NAME
*     drmaa_get_vector_attribute() -- Return attributes values of a vector attribute.
*
*  SYNOPSIS
*     int drmaa_get_vector_attribute(drmaa_job_template_t *jt, const char *name, 
*         drmaa_attr_values_t **values, char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     If 'name' is an existing vector attribute name in the job template 'jt',
*     then the values of 'name' are returned; otherwise, DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE 
*     is returned.
*
*  INPUTS
*     drmaa_job_template_t *jt       - the job template
*     const char *name               - the vector attribute name 
*  
*  OUTPUT
*     drmaa_attr_values_t **values   - destination string vector 
*     char *error_diagnosis          - diagnosis buffer
*     size_t error_diag_len          - diagnosis buffer length
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE.
*
*  NOTES
*     MT-NOTE: drmaa_get_vector_attribute() is MT safe
*******************************************************************************/
int drmaa_get_vector_attribute(drmaa_job_template_t *jt, const char *name, 
         drmaa_attr_values_t **values, char *error_diagnosis, size_t error_diag_len)
{
   lListElem *nsv;
   drmaa_attr_values_t *iter;
   dstring diag, *diagp = NULL;

   DENTER(TOP_LAYER, "drmaa_get_vector_attribute");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!values || !name || !jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   /* search name in string_vectors */
   if (!(nsv = lGetElemStr(jt->string_vectors, NSV_name, name))) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE;
   }
 
   /* allocate iterator */
   if (!(iter=japi_allocate_string_vector(JAPI_ITERATOR_STRINGS))) {
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diagp);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* copy job template attributes into iterator */ 
   iter->it.si.strings = lCopyList(NULL, lGetList(nsv, NSV_strings));
   if (!iter->it.si.strings) {
      japi_delete_string_vector(iter);
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diagp);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* initialize iterator */
   iter->it.si.next_pos = lFirst(iter->it.si.strings);

   *values = iter;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_get_attribute_names() **************************************
*  NAME
*     drmaa_get_attribute_names() -- Return supported job template attributes
*
*  SYNOPSIS
*     int drmaa_get_attribute_names(drmaa_attr_names_t **values, char 
*     *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     Returns the set of supported attribute names whose associated   
*     value type is String. This set will include supported DRMAA reserved 
*     attribute names and native attribute names. 
*
*  INPUTS
*     drmaa_attr_names_t **values    - String vector containing names of supported 
*                                      attributes
*     char *error_diagnosis          - diagnosis buffer
*     size_t error_diag_len          - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success
*
*  NOTES
*     MT-NOTE: drmaa_get_attribute_names() is MT safe
*******************************************************************************/
int drmaa_get_attribute_names(drmaa_attr_names_t **values, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;
   drmaa_attr_names_t *iter;

   DENTER(TOP_LAYER, "drmaa_get_attribute_names");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!(iter=drmaa_fill_string_vector(drmaa_supported_nonvector))) {
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diagp);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   *values = iter;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}


/****** DRMAA/drmaa_get_vector_attribute_names() *******************************
*  NAME
*     drmaa_get_vector_attribute_names() -- Return supported job template vector 
*                                           attributes
*
*  SYNOPSIS
*     int drmaa_get_vector_attribute_names(drmaa_attr_names_t **values, char 
*               *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     Returns the set of supported attribute names whose associated 
*     value type is String Vector.  This set will include supported DRMAA reserved 
*     attribute names and native attribute names. 
*
*  INPUTS
*     drmaa_attr_names_t **values    - String vector containing names of supported 
*                                      vector attributes
*     char *error_diagnosis          - diagnosis buffer
*     size_t error_diag_len          - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success
*
*  NOTES
*     MT-NOTE: drmaa_get_vector_attribute_names() is MT safe
*******************************************************************************/
int drmaa_get_vector_attribute_names(drmaa_attr_names_t **values, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;
   drmaa_attr_names_t *iter;

   DENTER(TOP_LAYER, "drmaa_get_vector_attribute_names");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!(iter=drmaa_fill_string_vector(drmaa_supported_vector))) {
      japi_standard_error(DRMAA_ERRNO_NO_MEMORY, diagp);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   *values = iter;

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_run_job() ****************************************************
*  NAME
*     drmaa_run_job() -- Submit a job
*
*  SYNOPSIS
*     int drmaa_run_job(char *job_id, size_t job_id_len, drmaa_job_template_t *jt, 
*                char *error_diagnosis, size_t error_diag_len)
*
*  FUNCTION
*     Submit a job with attributes defined in the job template 'jt'.
*     The job identifier 'job_id' is a printable, NULL terminated string,
*     identical to that returned by the underlying DRM system.
*
*  INPUTS
*     drmaa_job_template_t *jt - the job template
* 
*  OUTPUTS
*     char *job_id             - buffer for resulting jobid 
*     size_t job_id_len        - size of job_id buffer
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_TRY_LATER, DRMAA_ERRNO_DENIED_BY_DRM, 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, or DRMAA_ERRNO_AUTH_FAILURE.
*
*  NOTES
*      MT-NOTE: drmaa_run_job() is MT safe
*******************************************************************************/
int drmaa_run_job(char *job_id, size_t job_id_len, drmaa_job_template_t *jt, 
    char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;
   dstring jobid;
   int drmaa_errno;
   lListElem *sge_job_template;

   DENTER(TOP_LAYER, "drmaa_run_job");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!job_id || !jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   sge_dstring_init(&jobid, job_id, job_id_len+1);

   /* per thread initialization */
   if (japi_init_mt(diagp)!=DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_init_mt() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* convert DRMAA job template into Grid Engine job template */
   if ((drmaa_errno=drmaa_job2sge_job(&sge_job_template, jt, 
            0, 1, 1, 1, diagp))!=DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_drmaa_job2sge_job() */
      return drmaa_errno;
   }

   drmaa_errno = japi_run_job(&jobid, sge_job_template, diagp); 
   sge_job_template = lFreeElem(sge_job_template);

   DEXIT;
   return drmaa_errno;
}

/****** DRMAA/drmaa_run_bulk_jobs() ****************************************************
*  NAME
*     drmaa_run_bulk_jobs() -- Submit a bulk of jobs
*
*  SYNOPSIS
*     int drmaa_run_bulk_jobs(drmaa_job_ids_t **jobids, drmaa_job_template_t *jt, 
*           int start, int end, int incr, char *error_diagnosis, size_t error_diag_len)
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
*     drmaa_job_template_t *jt - The job template.
*     int start                - Start index
*     int end                  - End index
*     int incr                 - Increment
*
*  OUTPUTS
*     drmaa_job_ids_t **jobids - returns vector of job ids
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise DRMAA_ERRNO_TRY_LATER, 
*            DRMAA_ERRNO_DENIED_BY_DRM, DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, or 
*            DRMAA_ERRNO_AUTH_FAILURE.
*
*  NOTES
*      MT-NOTE: drmaa_run_bulk_jobs() is MT safe
*******************************************************************************/
int drmaa_run_bulk_jobs(drmaa_job_ids_t **jobids, drmaa_job_template_t *jt, 
      int start, int end, int incr, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag, *diagp = NULL;
   int drmaa_errno;
   lListElem *sge_job_template;

   DENTER(TOP_LAYER, "drmaa_run_bulk_jobs");

   if (error_diagnosis) {
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
      diagp = &diag;
   }

   if (!jobids || !jt) {
      japi_standard_error(DRMAA_ERRNO_INVALID_ARGUMENT, diagp);
      DEXIT;
      return DRMAA_ERRNO_INVALID_ARGUMENT;
   }

   /* per thread initialization */
   if (japi_init_mt(diagp)!=DRMAA_ERRNO_SUCCESS) {
      /* diag written by japi_init_mt() */
      DEXIT;
      return DRMAA_ERRNO_INTERNAL_ERROR;
   }

   /* convert DRMAA job template into Grid Engine job template */
   if ((drmaa_errno=drmaa_job2sge_job(&sge_job_template, jt, 1, start, end, incr, diagp))!=DRMAA_ERRNO_SUCCESS) {
      /* diag written by drmaa_job2sge_job() */
      DEXIT;
      return drmaa_errno;
   }

   drmaa_errno = japi_run_bulk_jobs((drmaa_attr_values_t **)jobids, sge_job_template, 
         start, end, incr, diagp);
   sge_job_template = lFreeElem(sge_job_template);

   DEXIT;
   return drmaa_errno;
}

/****** DRMAA/drmaa_control() ****************************************************
*  NAME
*     drmaa_control() -- Start, stop, restart, or kill jobs
*
*  SYNOPSIS
*     int drmaa_control(const char *jobid, int action, 
*                char *error_diagnosis, size_t error_diag_len)
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
*  INPUT
*     const char *jobid        - job id or DRMAA_JOB_IDS_SESSION_ALL
*     int action               - a DRMAA_CONTROL_* value
*
*  OUTPUT
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - DRMAA error codes
*        DRMAA_ERRNO_SUCCESS
*        DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE
*        DRMAA_ERRNO_AUTH_FAILURE
*        DRMAA_ERRNO_RESUME_INCONSISTENT_STATE 
*        DRMAA_ERRNO_SUSPEND_INCONSISTENT_STATE
*        DRMAA_ERRNO_HOLD_INCONSISTENT_STATE 
*        DRMAA_ERRNO_RELEASE_INCONSISTENT_STATE 
*        DRMAA_ERRNO_INVALID_JOB
*
*  NOTES
*      MT-NOTE: drmaa_control() is MT safe
*******************************************************************************/
int drmaa_control(const char *jobid, int action, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_control(jobid, action, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_synchronize() ****************************************************
*  NAME
*     drmaa_synchronize() -- Synchronize with jobs to finish
*
*  SYNOPSIS
*     int drmaa_synchronize(const char *job_ids[], signed long timeout, 
*             int dispose, char *error_diagnosis, size_t error_diag_len)
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
*     const char *job_ids[]    - vector of jobids to synchronize or 
*                                DRMAA_JOB_IDS_SESSION_ALL
*     signed long timeout      - timeout in seconds or 
*                                DRMAA_TIMEOUT_WAIT_FOREVER for infinite waiting
*                                DRMAA_TIMEOUT_NO_WAIT for immediate returning
*     int dispose              - Whether job finish information shall be reaped (1) or not (0).
*
*  OUTPUTS
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, DRMAA_ERRNO_AUTH_FAILURE,
*           DRMAA_ERRNO_EXIT_TIMEOUT, or DRMAA_ERRNO_INVALID_JOB.
*
*  NOTES
*      MT-NOTE: drmaa_synchronize() is MT safe
*******************************************************************************/
int drmaa_synchronize(const char *job_ids[], signed long timeout, int dispose, 
      char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_synchronize(job_ids, timeout, dispose, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_wait() ****************************************************
*  NAME
*     drmaa_wait() -- Wait for job
*
*  SYNOPSIS
*     int drmaa_wait(const char *job_id, char *job_id_out, size_t job_id_out_len, 
*           int *stat, signed long timeout, drmaa_attr_values_t **rusage, 
*           char *error_diagnosis, size_t error_diag_len)
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
*     const char *job_id       - jobid we're waiting for or DRMAA_JOB_IDS_SESSION_ANY
*     signed long timeout      - timeout in seconds or
*                                DRMAA_TIMEOUT_WAIT_FOREVER for infinite waiting
*                                DRMAA_TIMEOUT_NO_WAIT for immediate returning
*
*     
*     
*  OUTPUTS
*     char *job_id             - returns job id of waited job on success
*     size_t job_id_out        - job id buffer size
*     char *error_diagnosis    - diagnosis buffer on error
*     size_t error_diag_len    - diagnosis buffer length on error
*
*  RESULT
*     int - DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_EXIT_TIMEOUT
*              No job end within specified time.
*
*           DRMAA_ERRNO_INVALID_JOB
*              The job id specified was invalid or DRMAA_JOB_IDS_SESSION_ANY has been specified
*              and all jobs of this session have already finished.
*
*           DRMAA_ERRNO_NO_ACTIVE_SESSION
*              No active session. 
* 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE
*           DRMAA_ERRNO_AUTH_FAILURE
*
*  NOTES
*      MT-NOTE: drmaa_wait() is MT safe
*******************************************************************************/
int drmaa_wait(const char *job_id, char *job_id_out, size_t job_id_out_len, 
      int *stat, signed long timeout, drmaa_attr_values_t **rusage, 
      char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   dstring waited_job;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   if (job_id_out) 
      sge_dstring_init(&waited_job, job_id_out, job_id_out_len+1);
   return japi_wait(job_id, job_id_out?&waited_job:NULL, stat, timeout, rusage, error_diagnosis?&diag:NULL);
}


/****** DRMAA/drmaa_job_ps() ****************************************************
*  NAME
*     drmaa_job_ps() -- Get job status
*
*  SYNOPSIS
*     int drmaa_job_ps(const char *job_id, int *remote_ps, 
*                 char *error_diagnosis, size_t error_diag_len)
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
*     const char *job_id       - Job id of job to retrieve status.
*
*  OUTPUTS
*     int *remote_ps           - One of the DRMAA_PS_* constants.
*     char *error_diagnosis    - diagnosis buffer
*     size_t error_diag_len    - diagnosis buffer length
*
*  RESULT
*     int - returns DRMAA_ERRNO_SUCCESS on success, otherwise 
*           DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE, DRMAA_ERRNO_AUTH_FAILURE, 
*           or DRMAA_ERRNO_INVALID_JOB.
*
*  NOTES
*      MT-NOTE: drmaa_job_ps() is MT safe
*******************************************************************************/
int drmaa_job_ps(const char *job_id, int *remote_ps, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_job_ps(job_id, remote_ps, error_diagnosis?&diag:NULL);
}


/****** DRMAA/drmaa_wifexited() ************************************************
*  NAME
*     drmaa_wifexited() -- Has job terminated normally?
*
*  SYNOPSIS
*     int drmaa_wifexited(int *exited, int stat, char *error_diagnosis, size_t 
*     error_diag_len) 
*
*  FUNCTION
*     Evaluates into 'exited' a non-zero value if stat was returned for a job 
*     that terminated normally. A zero value can also indicate that altough the 
*     job has terminated normally an exit status is not available or that it is 
*     not known whether the job terminated normally. In both cases drmaa_wexitstatus() 
*     will not provide exit status information. A non-zero 'exited' value indicates 
*     more detailed diagnosis can be provided by means of drmaa_wifsignaled(), 
*     drmaa_wtermsig() and drmaa_wcoredump().
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
*
*  OUTPUTS
*     int *exited           - Returns 0 or 1.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wifexited() is MT safe
*******************************************************************************/
int drmaa_wifexited(int *exited, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_wifexited(exited, stat, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_wexitstatus() **********************************************
*  NAME
*     drmaa_wexitstatus() -- Return job exit status 
*
*  SYNOPSIS
*     int drmaa_wexitstatus(int *exit_status, int stat, char *error_diagnosis, 
*     size_t error_diag_len) 
*
*  FUNCTION
*     If the OUT parameter 'exited' of drmaa_wifexited() is non-zero, this function 
*     evaluates into 'exit_code' the exit code that the job passed to _exit() 
*     (see exit(2)) or exit(3C), or the value that the child process returned from main.
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
*
*  OUTPUTS
*     int *exit_status      - Exit status.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wexitstatus() is MT safe
*******************************************************************************/
int drmaa_wexitstatus(int *exit_status, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_wexitstatus(exit_status, stat, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_wifsignaled() **********************************************
*  NAME
*     drmaa_wifsignaled() -- Has job terminated due to a signal?
*
*  SYNOPSIS
*     int drmaa_wifsignaled(int *signaled, int stat, char *error_diagnosis, 
*     size_t error_diag_len) 
*
*  FUNCTION
*     Evaluates into 'signaled' a non-zero value if status was returned for a job 
*     that terminated due to the receipt of a signal. A zero value can also indicate 
*     that altough the job has terminated due to the receipt of a signal the signal 
*     is not available or that it is not known whether the job terminated due to the 
*     receipt of a signal. In both cases drmaa_wtermsig() will not provide signal
*     information.
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
* 
*  OUTPUTS
*     int *signaled         - Returns 0 or 1.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wifsignaled() is MT safe
*******************************************************************************/
int drmaa_wifsignaled(int *signaled, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_wifsignaled(signaled, stat, error_diagnosis?&diag:NULL);
}

/****** DRMAA/drmaa_wtermsig() *************************************************
*  NAME
*     drmaa_wtermsig() -- Return signal that caused job termination.
*
*  SYNOPSIS
*     int drmaa_wtermsig(char *signal, size_t signal_len, int stat, char 
*     *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     If the OUT parameter 'signaled' of drmaa_wifsignaled(stat) is non-zero, 
*     this function evaluates into signal a string representation of the signal that 
*     caused the termination of the job. For signals declared by POSIX, the symbolic 
*     names are returned (e.g., SIGABRT, SIGALRM). For signals not declared by POSIX, 
*     any other string may be returned.
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
*
*  OUTPUTS
*     char *signal          - Signal string buffer.
*     size_t signal_len     - Signal string buffer lenght.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wifsignaled() is MT safe
*******************************************************************************/
int drmaa_wtermsig(char *signal, size_t signal_len, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring sig, diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   if (signal) 
      sge_dstring_init(&sig, signal, signal_len+1);
   return japi_wtermsig(signal?&sig:NULL, stat, error_diagnosis?&diag:NULL);
}


/****** DRMAA/drmaa_wcoredump() ************************************************
*  NAME
*     drmaa_wcoredump() -- Was a core image created.
*
*  SYNOPSIS
*     int drmaa_wcoredump(int *core_dumped, int stat, char *error_diagnosis, 
*     size_t error_diag_len) 
*
*  FUNCTION
*     If the OUT parameter 'signaled' of drmaa_wifsignaled(stat) is non-zero, 
*     this function evaluates into 'core_dumped' a non-zero value if a core image 
*     of the terminated job was created.
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
*
*  OUTPUTS
*     int *core_dumped      - Returns 0 or 1.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wcoredump() is MT safe
*******************************************************************************/
int drmaa_wcoredump(int *core_dumped, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_wifcoredump(core_dumped, stat, error_diagnosis?&diag:NULL);
}


/****** drmaa/drmaa_wifaborted() ***********************************************
*  NAME
*     drmaa_wifaborted() -- Did the job ever run?
*
*  SYNOPSIS
*     int drmaa_wifaborted(int *aborted, int stat, char *error_diagnosis, 
*     size_t error_diag_len) 
*
*  FUNCTION
*     Evaluates into 'aborted' a non-zero value if 'stat' was returned for 
*     a job that ended before entering the running state.
*
*  INPUTS
*     int stat              - The stat value returned by drmaa_wait()
* 
*  OUTPUTS
*     int *aborted          - Returns 0 or 1.
*     char *error_diagnosis - diagnosis buffer
*     size_t error_diag_len - diagnosis buffer length
*
*  RESULT
*     int - Returns DRMAA_ERRNO_SUCCESS on success.
*
*  NOTES
*     MT-NOTE: drmaa_wifaborted() is MT safe
*******************************************************************************/
int drmaa_wifaborted(int *aborted, int stat, char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_wifaborted(aborted, stat, error_diagnosis?&diag:NULL);
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
*     int drmaa_errno - DRMAA errno number.
*  
*  RESULT
*     const char * - Returns string representation of errno.
*
*  NOTES
*      MT-NOTE: drmaa_strerror() is MT safe
*******************************************************************************/
const char *drmaa_strerror(int drmaa_errno)
{
   return japi_strerror(drmaa_errno);
}


/****** DRMAA/drmaa_get_next_attr_value() ***************************************
*  NAME
*     drmaa_get_next_attr_value() -- Get next entry from attribute name vector.
*
*  SYNOPSIS
*     int drmaa_get_next_attr_value(drmaa_attr_values_t* values, char *value, int 
*     value_len) 
*
*  FUNCTION
*     Returns the next entry from attribute value vector.
*
*  INPUTS
*     drmaa_attr_values_t* values - The attribute value vector.
*
*  OUTPUTS
*     char *value                - Buffer for the entry.
*     int value_len              - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE if no more entries
*           or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_get_next_attr_value() is MT safe
*******************************************************************************/
int drmaa_get_next_attr_value(drmaa_attr_values_t* values, char *value, int value_len)
{
   dstring val;
   if (value) 
      sge_dstring_init(&val, value, value_len+1);
   return japi_string_vector_get_next(values, value?&val:NULL);
}


/****** DRMAA/drmaa_get_next_attr_name() ***************************************
*  NAME
*     drmaa_get_next_attr_name() -- Get next entry from attribute name vector.
*
*  SYNOPSIS
*     int drmaa_get_next_attr_name(drmaa_attr_names_t* values, char *value, int 
*     value_len) 
*
*  FUNCTION
*     Returns the next entry from attribute name vector.
*
*  INPUTS
*     drmaa_attr_names_t* values - The attribute name vector.
*
*  OUTPUTS
*     char *value                - Buffer for the entry.
*     int value_len              - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE if no more entries
*           or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_get_next_attr_name() is MT safe
*******************************************************************************/
int drmaa_get_next_attr_name(drmaa_attr_names_t* values, char *value, int value_len)
{
   dstring val;
   if (value) 
      sge_dstring_init(&val, value, value_len+1);
   return japi_string_vector_get_next((drmaa_attr_values_t*)values, value?&val:NULL);
}


/****** DRMAA/drmaa_get_next_job_id() ***************************************
*  NAME
*     drmaa_get_next_job_id() -- Get next entry from job id vector.
*
*  SYNOPSIS
*     int drmaa_get_next_job_id(drmaa_job_ids_t* values, char *value, int 
*     value_len) 
*
*  FUNCTION
*     Returns the next entry from job id vector.
*
*  INPUTS
*     drmaa_job_ids_t* values - The job id name vector.
*
*  OUTPUTS
*     char *value                - Buffer for the entry.
*     int value_len              - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INVALID_ATTRIBUTE_VALUE if no more entries
*           or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_get_next_job_id() is MT safe
*******************************************************************************/
int drmaa_get_next_job_id(drmaa_job_ids_t* values, char *value, int value_len)
{
   dstring val;
   if (value) 
      sge_dstring_init(&val, value, value_len+1);
   return japi_string_vector_get_next((drmaa_attr_values_t*)values, value?&val:NULL);
}

/****** DRMAA/drmaa_release_attr_values() **************************************
*  NAME
*     drmaa_release_attr_values() -- Release attribute value vector
*
*  SYNOPSIS
*     void drmaa_release_attr_values(drmaa_attr_values_t* values) 
*
*  FUNCTION
*     Release resources used by attribute value vector.
*
*  INPUTS
*     drmaa_attr_values_t* values - The attribute value vector.
*
*  NOTES
*     MT-NOTE: drmaa_release_attr_values() is MT safe
*******************************************************************************/
void drmaa_release_attr_values(drmaa_attr_values_t* values)
{
   japi_delete_string_vector(values);
}

/****** DRMAA/drmaa_release_attr_names() **************************************
*  NAME
*     drmaa_release_attr_names() -- Release attribute name vector
*
*  SYNOPSIS
*     void drmaa_release_attr_names(drmaa_attr_names_t* values) 
*
*  FUNCTION
*     Release resources used by attribute name vector.
*
*  INPUTS
*     drmaa_attr_names_t* values - The attribute name vector.
*
*  NOTES
*     MT-NOTE: drmaa_release_attr_names() is MT safe
*******************************************************************************/
void drmaa_release_attr_names(drmaa_attr_names_t* values)
{
   japi_delete_string_vector((drmaa_attr_values_t*)values);
}

/****** DRMAA/drmaa_release_job_ids() **************************************
*  NAME
*     drmaa_release_job_ids() -- Release job id vector
*
*  SYNOPSIS
*     void drmaa_release_job_ids(drmaa_job_ids_t* values) 
*
*  FUNCTION
*     Release resources used by job id vector.
*
*  INPUTS
*     drmaa_job_ids_t* values - The job id vector.
*
*  NOTES
*     MT-NOTE: drmaa_release_job_ids() is MT safe
*******************************************************************************/
void drmaa_release_job_ids(drmaa_job_ids_t* values)
{
   japi_delete_string_vector((drmaa_attr_values_t*)values);
}

/****** DRMAA/drmaa_get_DRM_system() *******************************************
*  NAME
*     drmaa_get_DRM_system() -- Return DRM system information
*
*  SYNOPSIS
*     int drmaa_get_DRM_system(char *drm_system, size_t drm_system_len, char 
*     *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     Returns SGE system implementation information. The output contains the 
*     DRM name and release information.
*
*  OUTPUTS
*     char *drm_system      - Buffer for the DRM system name.
*     size_t drm_system_len - Buffer length.
*     char *error_diagnosis - Buffer for error diagnosis information.
*     size_t error_diag_len - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INTERNAL_ERROR on error or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_get_DRM_system() is MT safe
*******************************************************************************/
int drmaa_get_DRM_system(char *drm_system, size_t drm_system_len, 
         char *error_diagnosis, size_t error_diag_len)
{
   dstring drm, diag;
   if (drm_system) 
      sge_dstring_init(&drm, drm_system, drm_system_len+1);
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);
   return japi_get_drm_system(drm_system?&drm:NULL, error_diagnosis?&diag:NULL); 
}

/****** DRMAA/drmaa_get_contact() **********************************************
*  NAME
*     drmaa_get_contact() -- Return current (session) contact information.
*
*  SYNOPSIS
*     int drmaa_get_contact(char *contact, size_t contact_len, char 
*     *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     Return current (session) contact information.
*
*  INPUTS
*     char *contact         - Buffer for contact string.
*     size_t contact_len    - Buffer length.
*     char *error_diagnosis - Buffer for error diagnosis information.
*     size_t error_diag_len - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INTERNAL_ERROR on error or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_get_contact() is MT safe
*******************************************************************************/
int drmaa_get_contact(char *contact, size_t contact_len, 
     char *error_diagnosis, size_t error_diag_len)
{
   dstring con, diag;
   if (contact) 
      sge_dstring_init(&con, contact, contact_len+1);
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);

   /* So far drmaa_init(contact) is not used. Consequently 
      we do not provide contact information here either.
      We just return a NULL character to ensure string
      is initialized */
   sge_dstring_copy_string(&con, "");

   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_version() **************************************************
*  NAME
*     drmaa_version() -- Return DRMAA version information.
*
*  SYNOPSIS
*     int drmaa_version(unsigned int *major, unsigned int *minor, char 
*     *error_diagnosis, size_t error_diag_len) 
*
*  FUNCTION
*     Returns the major and minor version numbers of the DRMAA library.
*
*  INPUTS
*     unsigned int *major   - Major number.
*     unsigned int *minor   - Minor number. 
* 
*  OUTPUTS
*     char *error_diagnosis - Buffer for error diagnosis information.
*     size_t error_diag_len - Buffer length.
*
*  RESULT
*     int - DRMAA_ERRNO_INTERNAL_ERROR on error or DRMAA_ERRNO_SUCCESS
*
*  NOTES
*     MT-NOTE: drmaa_version() is MT safe
*******************************************************************************/
int drmaa_version(unsigned int *major, unsigned int *minor, 
      char *error_diagnosis, size_t error_diag_len)
{
   dstring diag;
   if (error_diagnosis) 
      sge_dstring_init(&diag, error_diagnosis, error_diag_len+1);

   if (major)
      *major = 0;
   if (minor)
      *minor = 8;

   return DRMAA_ERRNO_SUCCESS;
}

#if 0
/****** DRMAA/drmaa_path2sge_job() *****************************************
*  NAME
*     drmaa_path2sge_job() -- Transform a DRMAA job path into SGE 
*                             counterpart and put it into the SGE job
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
/* This method is no longer used and should be removed as soon as this DRMAA
 * implementation is complete. */
static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, lListElem *jt, 
   int is_bulk, int nm, const char *attribute_key, dstring *diag)
{
   const char *new_path = NULL;
   int ret_val;
   
   DENTER(TOP_LAYER, "japi_drmaa_path2sge_job");
   
   lSetList(jt, nm, NULL);
   
   if ((ret_val = japi_drmaa_path2sge_path (drmaa_jt->strings, is_bulk,
                                            attribute_key, 1, &new_path,
                                            diag)) == DRMAA_ERRNO_SUCCESS) {
      DPRINTF(("%s = \"%s\"\n", lNm2Str(nm), new_path));
      lAddSubStr(jt, PN_path, new_path, nm, PN_Type);
   }   

   DEXIT;
   return ret_val;
}
#endif
/****** DRMAA/drmaa_path2wd_opt() **********************************************
*  NAME
*     drmaa_path2wd_opt() -- Transform a DRMAA job path into SGE 
*                            qsub style -wd option
*
*  SYNOPSIS
*     static int japi_drmaa_path2wd_opt (lList *attrs, lList **args, int is_bulk,
*                                        dstring *diag)
*
*  FUNCTION
*     Transform a DRMAA job path into SGE qsub style -wd option. The following 
*     substitutions are performed
*         
*        $drmaa_hd_ph$     --> $HOME
*        $drmaa_incr_ph$   --> $TASK_ID   
*
*     The $drmaa_incr_ph$ substitutions are performed only for bulk jobs
*     otherwise submission fails.
*
*  INPUTS
*     lList* attrs                   - the DRMAA job attribute list (drmaa_jt->strings)
*     lList *args                    - the list to which to append the switch
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*                                      path
*
*  OUTPUTS
*     dstring *diag                  - diagnosis inforation
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_drmaa_path2wd_opt() is MT safe
*******************************************************************************/
static int japi_drmaa_path2wd_opt (lList *attrs, lList **args, int is_bulk,
                                   dstring *diag) {
   const char *new_path = NULL;
   int ret_val;
   
   DENTER (TOP_LAYER, "japi_drmaa_path2wd_opt");
   
   if ((ret_val = japi_drmaa_path2sge_path (attrs, is_bulk,
                                            DRMAA_WD, 0, &new_path,
                                            diag)) == DRMAA_ERRNO_SUCCESS) {
      if (new_path) {

         lListElem *ep = lGetElemStr (attrs, VA_variable, DRMAA_WD);
         const char *value = lGetString (ep, VA_value);

         DPRINTF (("-wd = \"%s\"\n", new_path));

         ep = sge_add_arg (args, wd_OPT, lStringT, "-wd", value);
         lSetString (ep, SPA_argval_lStringT, new_path);
      }
      else {
         ret_val = 0;
      }
   }   
   
   DEXIT;
   return ret_val;
}

/****** DRMAA/drmaa_path2path_opt() ********************************************
*  NAME
*     drmaa_path2sge_job() -- Transform a DRMAA job path into SGE qsub style
*                             path option
*
*  SYNOPSIS
*     static int japi_drmaa_path2path_opt (lList *attrs, lList **args,
*                                          int is_bulk, const char *attribute_key,
*                                          const char *sw, int opt, dstring *diag)
*
*  FUNCTION
*     Transform a DRMAA job path into SGE qsub style path option. The following 
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
*     lList* attrs                   - the DRMAA job attribute list (drmaa_jt->strings)
*     lList *args                    - the list to which to append the switch
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*     const char *attribute_key      - The DRMAA job template keyword for this
*                                      path
*     const char *sw                 - The qsub switch to store this under
*     int *opt                       - The type to store this under
*
*  OUTPUTS
*     dstring *diag                  - diagnosis inforation
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_drmaa_path2path_opt() is MT safe
*******************************************************************************/
static int japi_drmaa_path2path_opt (lList *attrs, lList **args, int is_bulk,
   const char *attribute_key, const char *sw, int opt, dstring *diag) {
   const char *new_path = NULL;
   int ret_val;
   lList *path_list = lCreateList("path list", PN_Type);
   
   DENTER (TOP_LAYER, "japi_drmaa_path2path_opt");

   if (path_list == NULL) {
      return 1;
   }
   else if ((ret_val = japi_drmaa_path2sge_path (attrs, is_bulk,
                                                 attribute_key, 1, &new_path,
                                                 diag)) == DRMAA_ERRNO_SUCCESS) {
      if (new_path) {
         lListElem *ep = lGetElemStr (attrs, VA_variable, attribute_key);
         const char *value = lGetString(ep, VA_value);
         char *cell = NULL;
         char *path = NULL;

         DPRINTF (("%s = \"%s\"\n", sw, new_path));

         if (new_path[0] == ':') {  /* :path */
            path = (char *)new_path + 1;
         } else if ((path = strstr (new_path, ":"))){ /* host:path */
            path[0] = '\0';
            cell = strdup (new_path);
            path[0] = ':';
            path += 1;
         } else { /* path */
            path = (char *)new_path;
         }

         ep = lCreateElem (AT_Type);
         lAppendElem (path_list, ep);
         DPRINTF (("PN_path = \"%s\"\n", path));
         lSetString (ep, PN_path, path);

         if (cell) {
            DPRINTF (("PN_host = \"%s\"\n", cell));
            lSetHost (ep, PN_host, cell);
            FREE (cell);
         }

         ep = sge_add_arg (args, opt, lListT, sw, value);
         lSetList (ep, SPA_argval_lListT, path_list);
         
         FREE (new_path);
      }   
      else {
         ret_val = 0;
      }
   }

   DEXIT;
   return ret_val;
}

/****** DRMAA/drmaa_path2sge_path() ********************************************
*  NAME
*     drmaa_path2sge_path() -- Transform a DRMAA job path into SGE 
*                             counterpart
*
*  SYNOPSIS
*     static int japi_drmaa_path2sge_path (lList *attrs, int is_bulk,
*                                          const char *attribute_key, int do_wd,
*                                          const char **new_path, dstring *diag)
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
*     lList* attrs                   - the DRMAA job attribute list (drmaa_jt->strings)
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*     const char *attribute_key      - The DRMAA job template keyword for this
*                                      path
*     int do_wd                      - whether the WD placeholder should be used
*
*  OUTPUTS
*     char **new_path                - The modified path
*     dstring *diag                  - diagnosis inforation
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: japi_drmaa_path2sge_path() is MT safe
*******************************************************************************/
static int japi_drmaa_path2sge_path (lList *attrs, int is_bulk,
   const char *attribute_key, int do_wd, const char **new_path, dstring *diag) {
   lListElem *ep;

   DENTER (TOP_LAYER, "japi_drmaa_path2sge_path");

   if ((ep=lGetElemStr(attrs, VA_variable, attribute_key ))) {
      dstring ds = DSTRING_INIT;
      const char *p, *value = lGetString(ep, VA_value);
      /* substitute DRMAA placeholder with grid engine counterparts */

      /* home directory and working directory placeholder only recognized at the begin */
      if (!strncmp(value, DRMAA_PLACEHOLDER_HD, strlen(DRMAA_PLACEHOLDER_HD))) {
         sge_dstring_copy_string(&ds, "$HOME");
         value += strlen(DRMAA_PLACEHOLDER_HD);
      } else if (do_wd && (!strncmp(value, DRMAA_PLACEHOLDER_WD, strlen(DRMAA_PLACEHOLDER_WD)))) {
         sge_dstring_copy_string(&ds, "$CWD"); /* not yet supported by Grid Engine */
         value += strlen(DRMAA_PLACEHOLDER_WD);
      }

      /* bulk job index placeholder recognized at any position */
      if ((p=strstr(value, DRMAA_PLACEHOLDER_INCR))) {
         
         if (!is_bulk) {
            /* reject incr placeholder for non-array jobs */
            sge_dstring_free(&ds);
            sge_dstring_sprintf(diag, "increment placeholder "SFQ" only in pathes "
                  "for bulk jobs\n", DRMAA_PLACEHOLDER_INCR);
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
      *new_path = strdup (sge_dstring_get_string (&ds));
      sge_dstring_free(&ds);
   }

   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/drmaa_job2sge_job() **********************************************
*  NAME
*     drmaa_job2sge_job() -- convert a DRMAA job template into the Grid 
*                                 Engine counterpart 
*
*  SYNOPSIS
*     int drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t 
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
static int drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t *drmaa_jt, 
   int is_bulk, int start, int end, int step, dstring *diag)
{
   lListElem *jt, *ep;
   int drmaa_errno;
   lList *alp = NULL;
   lList *opts_drmaa = NULL;
   lList *opts_native = NULL;
   lList *opts_default = NULL;
   lList *opts_job_cat = NULL;
   lList *opts_defaults = NULL;
   lList *opts_scriptfile = NULL;
   lList *opts_all = NULL;
   int read_scriptfile = 0;

   DENTER (TOP_LAYER, "japi_drmaa_job2sge_job");

   /* make JB_Type job description out of DRMAA job template */
   if (!(jt = lCreateElem (JB_Type))) {
      japi_standard_error (DRMAA_ERRNO_NO_MEMORY, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   /* init range of jobids and put all tasks in 'active' state */
   if (job_set_submit_task_ids (jt, start, end, step) ||
       job_initialize_id_lists (jt, NULL)) {
      jt = lFreeElem (jt);   
      japi_standard_error (DRMAA_ERRNO_NO_MEMORY, diag);
      DEXIT;
      return DRMAA_ERRNO_NO_MEMORY;
   }

   {
      u_long32 jb_now = lGetUlong (jt, JB_type);

      /*
       * An error state does not exist with DRMAA jobs.
       * This setting is necessary to ensure e.g. jobs
       * with a wrong input path specification fail when
       * doing drmaa_wait(). A SGE job template attribute
       * could be supported to enable SGE error state.
       */
      JOB_TYPE_SET_NO_ERROR (jb_now);

      /* mark array job */
      if (is_bulk)
         JOB_TYPE_SET_ARRAY (jb_now);
      lSetUlong (jt, JB_type, jb_now);
   }
   
   /*
    * read switches from the various defaults files
    */
   opt_list_append_opts_from_default_files (&opts_defaults, &alp, environ);
   
   if (answer_list_has_error (&alp)) {
      answer_list_to_dstring (alp, diag);
      lFreeList (alp);
      jt = lFreeElem (jt);   
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }

   /*
    * append the native spec switches to the list if they exist
    */
   if ((ep=lGetElemStr (drmaa_jt->strings, VA_variable, DRMAA_NATIVE_SPECIFICATION))) {
      const char *value = lGetString (ep, VA_value);
      int num_args = sge_quick_count_num_args (value);
      char *args[num_args + 1];
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_NATIVE_SPECIFICATION, value));
      
      sge_parse_args (value, args);
      args[num_args] = NULL;
      opt_list_append_opts_from_qsub_cmdline (&opts_native, &alp,
                                              args, environ);
      
      if (answer_list_has_error (&alp)) {
         answer_list_to_dstring (alp, diag);
         lFreeList (alp);
         jt = lFreeElem (jt);   
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }
   }

   if ((drmaa_errno = opt_list_append_opts_from_drmaa_attr
                      (&opts_drmaa, drmaa_jt->strings, drmaa_jt->string_vectors,
                       is_bulk, diag)) != DRMAA_ERRNO_SUCCESS) {
      jt = lFreeElem (jt);
      DEXIT;
      return drmaa_errno;
   }
 
   /*
    * Set up default options
    */
   opt_list_append_default_drmaa_opts (&opts_default);

   /*
    * We will only read commandline options from scripfile if the script
    * itself should not be handled as binary
    */   
   
   if (opt_list_is_X_true (opts_native, "-b") ||
       (!opt_list_has_X (opts_native, "-b") &&
        (opt_list_is_X_true (opts_defaults, "-b") ||
        (!opt_list_has_X (opts_defaults, "-b") &&
         opt_list_is_X_true (opts_default, "-b"))))) {
      DPRINTF(("Skipping options from script due to -b option\n"));
   } else {
      opt_list_append_opts_from_script (&opts_scriptfile, &alp, opts_all, environ);
      
      if (answer_list_has_error (&alp)) {
         answer_list_to_dstring (alp, diag);
         lFreeList (alp);
         jt = lFreeElem (jt);   
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }
      
      /* Note that we parsed the script file for command line options so that
       * we can reneg on it later if we need to. */
      read_scriptfile = 1;
   }

   /*
    * append the job category switches to the list if they exist
    */
   if (opt_list_has_X (opts_drmaa, "-cat") ||
       opt_list_has_X (opts_scriptfile, "-cat") ||
       opt_list_has_X (opts_native, "-cat") ||
       opt_list_has_X (opts_defaults, "-cat") ||
       opt_list_has_X (opts_default, "-cat")) {
      /* No need to free job_cat since it points to a string in an element
       * in jt->strings. */
      char *job_cat = NULL;
      char **args = NULL;
      lListElem *ep = NULL;
      
      DPRINTF (("Processing job category\n"));
      
      /* This long series of ifs is really pointless since opts_drmaa is the
       * only one that can contain -cat.  However, I expect that at some point
       * -cat will be added as a normal switch to qsub et al, at which point
       * this long series of ifs becomes necessary. */
      if ((ep = lGetElemStr (opts_drmaa, SPA_switch, "-cat")) != NULL) {
         job_cat = strdup (lGetString (ep, SPA_argval_lStringT));
         lRemoveElem (opts_drmaa, ep);
      }
      else if ((ep = lGetElemStr (opts_scriptfile, SPA_switch, "-cat")) != NULL) {
         job_cat = strdup (lGetString (ep, SPA_argval_lStringT));
         lRemoveElem (opts_scriptfile, ep);
      }
      else if ((ep = lGetElemStr (opts_native, SPA_switch, "-cat")) != NULL) {
         job_cat = strdup (lGetString (ep, SPA_argval_lStringT));
         lRemoveElem (opts_native, ep);
      }
      else if ((ep = lGetElemStr (opts_defaults, SPA_switch, "-cat")) != NULL) {
         job_cat = strdup (lGetString (ep, SPA_argval_lStringT));
         lRemoveElem (opts_defaults, ep);
      }
      else if ((ep = lGetElemStr (opts_default, SPA_switch, "-cat")) != NULL) {
         job_cat = strdup (lGetString (ep, SPA_argval_lStringT));
         lRemoveElem (opts_default, ep);
      }
      else {
         /* This theoretically can't happen. */
         sge_dstring_copy_string (diag, "No job category could be found even though -cat was detected.");
         lFreeList (alp);
         jt = lFreeElem (jt);   
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }

      
      /* We need to document a standard practice for naming job categories so
       * they don't conflict with command names.  I think something like
       * <cat_name>.jcat would work fine. */
      args = sge_get_qtask_args (job_cat, alp);
      
      if (answer_list_has_error (&alp)) {
         answer_list_to_dstring (alp, diag);
         lFreeList (alp);
         jt = lFreeElem (jt);   
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }

      FREE (job_cat);
      
      if (args != NULL) {
         opt_list_append_opts_from_qsub_cmdline (&opts_job_cat, &alp,
                                                 args, environ);
         
         if (answer_list_has_error (&alp)) {
            answer_list_to_dstring (alp, diag);
            jt = lFreeElem (jt);   
            DEXIT;
            return DRMAA_ERRNO_DENIED_BY_DRM;
         }
         
         /* Now, since the job category can affect whether the script files
          * get parsed for options, we have to step back a bit and make sure
          * that if the job category contained a -b option, that it's effect
          * gets counted appropriately. */
         if (opt_list_has_X (opts_job_cat, "-b")) {
            if (!opt_list_has_X (opts_native, "-b") &&
                (read_scriptfile && !opt_list_is_X_true (opts_job_cat, "-b"))) {
               /* If we parsed the script file due to a -b n in the defaults files
                * or the DRMAA defaults or because of no -b option, and a -b y
                * is given in the job category, clear the script file options. */
               opts_scriptfile = lFreeList (opts_scriptfile);
            }
            else if (!opt_list_has_X (opts_native, "-b") &&
                (!read_scriptfile && opt_list_is_X_true (opts_job_cat, "-b"))) {
               /* If we didn't parse the script file due to a -b y in the defaults
                * files or the DRMAA defaults, and a -b n is given in the job
                * category, parse the script file now. */
               /* No need to worry about recursion or infinite loops here since
                * the script file cannot contain -cat.  (-cat can only come from
                * DRMAA_JOB_CATEGORY.  If at some point in the future -cat gets
                * added as a normal switch to qsub et al, the issue of inifite
                * loops will have to be addressed.  The best solution at that
                * point will probably be to parse the job category before the
                * script file and simply not allow the script file to set the
                * job category. */
               opt_list_append_opts_from_script (&opts_scriptfile, &alp, 
                                                 opts_all, environ);

               if (answer_list_has_error (&alp)) {
                  answer_list_to_dstring (alp, diag);
                  alp = lFreeList (alp);
                  jt = lFreeElem (jt);   
                  DEXIT;
                  return DRMAA_ERRNO_DENIED_BY_DRM;
               }
            }
         }
      }
      else {
         /* Bad job category */
         sge_dstring_copy_string (diag, "Unknown job category");
         alp = lFreeList (alp);
         jt = lFreeElem (jt);
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }
   }
   
   /*
    * Merge all commandline options and interprete them
    */
   DPRINTF (("Before merge...\n"));
   DPRINTF (("%d items in default\n", lGetNumberOfElem (opts_default)));
   DPRINTF (("%d items in defaults\n", lGetNumberOfElem (opts_defaults)));
   DPRINTF (("%d items in scriptfile\n", lGetNumberOfElem (opts_scriptfile)));
   DPRINTF (("%d items in job cat\n", lGetNumberOfElem (opts_job_cat)));
   DPRINTF (("%d items in native\n", lGetNumberOfElem (opts_native)));
   DPRINTF (("%d items in drmaa\n", lGetNumberOfElem (opts_drmaa)));
   DPRINTF (("%d items in all\n", lGetNumberOfElem (opts_all)));
   merge_drmaa_options (&opts_all, &opts_default, &opts_defaults, &opts_scriptfile,
                       &opts_job_cat, &opts_native, &opts_drmaa);
   DPRINTF (("After merge...\n"));
   DPRINTF (("%d items in default\n", lGetNumberOfElem (opts_default)));
   DPRINTF (("%d items in defaults\n", lGetNumberOfElem (opts_defaults)));
   DPRINTF (("%d items in scriptfile\n", lGetNumberOfElem (opts_scriptfile)));
   DPRINTF (("%d items in job cat\n", lGetNumberOfElem (opts_job_cat)));
   DPRINTF (("%d items in native\n", lGetNumberOfElem (opts_native)));
   DPRINTF (("%d items in drmaa\n", lGetNumberOfElem (opts_drmaa)));
   DPRINTF (("%d items in all\n", lGetNumberOfElem (opts_all)));

   alp = cull_parse_job_parameter (opts_all, &jt);
   
   if (answer_list_has_error (&alp)) {
      answer_list_to_dstring (alp, diag);
      jt = lFreeElem (jt);   
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }

   *jtp = jt;
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/opt_list_append_opts_from_drmaa_attr() ***************************
*  NAME
*     opt_list_append_opts_from_drmaa_attr() -- covert the DRMAA attributes
*                                               into qsub style option lListElem
*                                               and append them to the given
*                                               lList.
*
*  SYNOPSIS
*     int opt_list_append_opts_from_drmaa_attr (lList **args, lList *attrs, lList *vattrs,
                                                 int is_bulk, dstring *diag)
*
*  FUNCTION
*     All DRMAA job template attributes are translated into qsub style option
*     lListElem for parsing by cull_parse_job_parameter()
*
*  INPUTS
*     lList **args                   - list to which options will be appended
*     lList *attrs                   - list of DRMAA scalar attributes
*     lList *vattrs                  - list of DRMAA vector attributes
*     int is_bulk                    - 1 for bulk jobs 0 otherwise
*     dstring *diag                  - diagnosis information
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: opt_list_append_opts_from_drmaa_attr() is MT safe
*
*******************************************************************************/
static int opt_list_append_opts_from_drmaa_attr (lList **args, lList *attrs, lList *vattrs,
                                                 int is_bulk, dstring *diag) {
   int drmaa_errno;
   lListElem *ep = NULL;
   lListElem *ep_opt = NULL;
   /* Turn each DRMAA attribute into a list entry. */

   DENTER (TOP_LAYER, "opt_list_append_opts_from_drmaa_attr");
   DPRINTF (("%d DRMAA attributes\n", lGetNumberOfElem (attrs)));
   DPRINTF (("%d DRMAA vector attributes\n", lGetNumberOfElem (vattrs)));
   
   /* job name -- -N <name>*/
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_JOB_NAME))) {
      DPRINTF(("processing %s = \"%s\"\n", DRMAA_JOB_NAME, lGetString (ep, VA_value)));
      
      ep_opt = sge_add_arg (args, N_OPT, lStringT, "-N", lGetString (ep, VA_value));
      lSetString (ep_opt, SPA_argval_lStringT, lGetString (ep, VA_value));
   }
   
   /* job category -- -cat (not exposed in qsub) */
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_JOB_CATEGORY))) {
      const char *value = lGetString (ep, VA_value);
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_JOB_CATEGORY, value));
      
      DPRINTF (("%d args before adding job cat\n", lGetNumberOfElem (*args)));
      ep_opt = sge_add_arg (args, cat_OPT, lStringT, "-cat", value);
      lSetString (ep_opt, SPA_argval_lStringT, value);
      DPRINTF (("%d args after adding job cat\n", lGetNumberOfElem (*args)));
   }
   
   /* join files -- -j "y|n" */
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_JOIN_FILES))) {
      const char *value = lGetString (ep, VA_value);
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_JOIN_FILES, value));
      
      if (value[0] == 'y') {
         ep_opt = sge_add_arg (args, j_OPT, lIntT, "-j", "y");
         lSetInt (ep_opt, SPA_argval_lIntT, TRUE);
      }
      else {
         ep_opt = sge_add_arg (args, j_OPT, lIntT, "-j", "n");
         lSetInt (ep_opt, SPA_argval_lIntT, FALSE);
      }
   }

   /* working directory -- -wd (not exposed in qsub) */
   if ((drmaa_errno = japi_drmaa_path2wd_opt (attrs, args, is_bulk, diag))
        != DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return drmaa_errno;
   }

   /* jobs input/output/error stream -- -i/o/e */
   if ((drmaa_errno = japi_drmaa_path2path_opt (attrs, args, is_bulk, DRMAA_OUTPUT_PATH,
                                                "-o", o_OPT, diag))
        != DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return drmaa_errno;
   }
   if ((drmaa_errno = japi_drmaa_path2path_opt (attrs, args, is_bulk, DRMAA_ERROR_PATH,
                                                "-e", e_OPT, diag))
        != DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return drmaa_errno;
   }
   if ((drmaa_errno = japi_drmaa_path2path_opt (attrs, args, is_bulk, DRMAA_INPUT_PATH,
                                                "-i", i_OPT, diag))
        != DRMAA_ERRNO_SUCCESS) {
      DEXIT;
      return drmaa_errno;
   }

   /* user hold state -- -h */
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_JS_STATE))) {
      const char *value = lGetString (ep, VA_value);
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_JS_STATE, value));
      
      if (!strcmp (value, DRMAA_SUBMISSION_STATE_HOLD)) {
         ep_opt = sge_add_arg (args, h_OPT, lIntT, "-h", "");
         lSetInt (ep_opt, SPA_argval_lIntT, MINUS_H_TGT_USER);
      }
   }

   /* job environment -- -v */
   if ((ep=lGetElemStr (vattrs, NSV_name, DRMAA_V_ENV))) {
      dstring env = DSTRING_INIT;
      char *variable = NULL;
      char *value = NULL;
      lListElem *oep = NULL;
      lList *olp = lGetList (ep, NSV_strings);
      lListElem *nep = NULL;
      lList *nlp = lCreateList ("variable list", VA_Type);
      int first_time = 1;
      
      DPRINTF(("processing %s = ", DRMAA_V_ENV));
      
      for_each (oep, olp) {
         const char *str = lGetString (oep, ST_name);
         
         sge_dstring_append (&env, str);
         
         if (first_time) {
            first_time = 0;
         }
         else {
            sge_dstring_append_char (&env, ',');
         }
         
         nep = lCreateElem (VA_Type);
         lAppendElem (nlp, nep);
         
         variable = sge_strtok (str, "=");
         lSetString (nep, VA_variable, variable);
         
         value = sge_strtok ((char *)NULL, "=");
         
         if (value)
            lSetString (nep, VA_value, value);
         else
            lSetString (nep, VA_value, NULL);
      }

      DPRINTF (("\"%s\"\n", env));
      
      ep_opt = sge_add_arg (args, v_OPT, lListT, "-v", sge_dstring_get_string (&env));
      lSetList (ep_opt, SPA_argval_lListT, nlp);
   }

   /* email address -- -M */
   /* steal this code from the qsub -M option in cull_parse_cmdline
	  * [parse_qsub.c] around line 935.*/
   if ((ep=lGetElemStr (vattrs, NSV_name, DRMAA_V_EMAIL))) {
      dstring email = DSTRING_INIT;
      char *user = NULL;
      char *host = NULL;
      lListElem *oep = NULL;
      lList *olp = lGetList (ep, NSV_strings);
      lListElem *nep = NULL;
      lList *nlp = lCreateList ("mail list", MR_Type);
      lListElem *tmp = NULL;
      int first_time = 1;
      
      DPRINTF (("processing %s = ", DRMAA_V_EMAIL));
      
      for_each (oep, olp) {
         const char *str = lGetString (oep, ST_name);
         
         sge_dstring_append (&email, str);
         
         if (first_time) {
            first_time = 0;
         }
         else {
            sge_dstring_append_char (&email, ',');
         }
         
         user = sge_strtok (str, "@");
         host = sge_strtok (NULL, "@");
         
         if ((tmp=lGetElemStr (nlp, MR_user, user))) {
            if (!sge_strnullcmp (host, lGetHost (tmp, MR_host))) {
               /* got this mail adress twice */
               continue;
            }
         }

         /* got a new adress - add it */
         nep = lCreateElem (MR_Type);
         lSetString (nep, MR_user, user);
         if (host) 
            lSetHost (nep, MR_host, host);
         lAppendElem (nlp, nep);
      }

      DPRINTF (("\"%s\"\n", email));
      
      ep_opt = sge_add_arg (args, M_OPT, lListT, "-M", sge_dstring_get_string (&email));
      lSetList (ep_opt, SPA_argval_lListT, nlp);
   }
#if 0
/* I don't know yet if no email address should imply implicit email suppression.
 * Since SGE automatically fills in an email for the user, I'll assume for the
 * moment that it doesn't.  This needs to be changed when the DRMAA-WG decides
 * how this attribute should work. */
   else {
      /* If no email addresses are specified, suppress email sending. */
      ep_opt = sge_add_arg (args, m_OPT, lIntT, "-m", "n");
      lSetInt (ep_opt, SPA_argval_lIntT, NO_MAIL);
   }
#endif

   /* email supression -- -m */
   /* steal this code from the qsub -m n option in cull_parse_cmdline
	  * [parse_qsub.c] around line 865 and sge_parse_mail_options.*/
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_BLOCK_EMAIL))) {
      const char *value = lGetString (ep, VA_value);
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_BLOCK_EMAIL, value));
      
      if (value[0] == '1') {
         ep_opt = sge_add_arg (args, m_OPT, lIntT, "-m", "n");
         lSetInt (ep_opt, SPA_argval_lIntT, NO_MAIL);
      }
      else if (value[0] == '0') {
         /* This needs to be implemented so that DRMAA can reenable email that
          * may have been blocked by a job cat, native spec, et al.  Since we
          * don't have the granularity to say how exactly to unblock the email,
          * we just assume an innocuous default, i.e. email when the job is
          * done.  This should definitely be documented somewhere. */
         ep_opt = sge_add_arg (args, m_OPT, lIntT, "-m", "e");
         lSetInt (ep_opt, SPA_argval_lIntT, MAIL_AT_EXIT);
      }
      else {
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }
   }

   /* job start time -- -a */
   /* steal this code from the qsub -a option in cull_parse_cmdline
	  * [parse_qsub.c] around line 155.*/
   if ((ep=lGetElemStr (attrs, VA_variable, DRMAA_START_TIME))) {
      u_long32 timeval;
      const char *value = (const char*)drmaa_time2sge_time (lGetString (ep, VA_value), diag);

      if (value == NULL) {
         /* diag is set by drmma_time2sge_time */
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }
      
      DPRINTF (("processing %s = \"%s\"\n", DRMAA_START_TIME, value));
      
      if (!ulong_parse_date_time_from_string (&timeval, NULL, value)) {
         sge_dstring_copy_string (diag, "invalid format for job start time");
         DEXIT;
         return DRMAA_ERRNO_DENIED_BY_DRM;
      }

      ep_opt = sge_add_arg (args, a_OPT, lUlongT, "-a", value);
      lSetUlong (ep_opt, SPA_argval_lUlongT, timeval);
   }

   /* remote command -- last thing on the command line before the job args */
   if (!(ep=lGetElemStr (attrs, VA_variable, DRMAA_REMOTE_COMMAND))) {      
      DPRINTF (("no remote command given\n"));
      
      sge_dstring_copy_string (diag, "job template must have \""DRMAA_REMOTE_COMMAND"\" attribute set");
      
      DEXIT;
      return DRMAA_ERRNO_DENIED_BY_DRM;
   }
   
   if (lGetString (ep, VA_value) != NULL) {
      DPRINTF (("remote command is \"%s\"\n", lGetString (ep, VA_value)));
      ep_opt = sge_add_arg (args, 0, lStringT, STR_PSEUDO_SCRIPT, NULL);
      lSetString (ep_opt, SPA_argval_lStringT, lGetString (ep, VA_value));

      /* job arguments -- last thing on the command line */
      if ((ep=lGetElemStr (vattrs, NSV_name, DRMAA_V_ARGV))) {
         lList *lp = lGetList (ep, NSV_strings);
         lListElem *aep = NULL;

         DPRINTF (("processing %s\n", DRMAA_V_ARGV));

         for_each (aep, lp) {
            DPRINTF (("arg: \"%s\"\n", lGetString (aep, ST_name)));
            ep_opt = sge_add_arg(args, 0, lStringT, STR_PSEUDO_JOBARG, NULL);
            lSetString (ep_opt, SPA_argval_lStringT, lGetString (aep, ST_name));
         }
      }
   }

   DPRINTF (("%d args at end of method\n", lGetNumberOfElem (*args)));
   
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

/****** DRMAA/opt_list_append_default_drmaa_opts() ***************************
*  NAME
*     opt_list_append_default_drmaa_opts() -- append the DRMAA default setting
*                                             to the list in the form of qsub
*                                             style lListElem.
*
*  SYNOPSIS
*     int opt_list_append_default_drmaa_opts (lList **opts)
*
*  FUNCTION
*     All DRMAA default settings are translated into qsub style option
*     lListElem for parsing by cull_parse_job_parameter()
*
*  INPUTS
*     lList **opts                   - list to which options will be appended
*
*  RESULT
*     static int - DRMAA error codes
*
*  NOTES
*     MT-NOTE: opt_list_append_default_drmaa_opts() is MT safe
*
*******************************************************************************/
static void opt_list_append_default_drmaa_opts (lList **opts) {
   lListElem *ep_opt;
   DENTER (TOP_LAYER, "opt_list_append_drmaa_default_opts");
   
   /* average priority of 0 -- -p 0 */
   DPRINTF (("setting default priority to 0\n"));
   ep_opt = sge_add_arg (opts, p_OPT, lIntT, "-p", "0");
   lSetInt (ep_opt, SPA_argval_lIntT, 0);
   
   /* 
    * We always use binary submission mode by default. A native spec, job
    * category, or default file attribute can use -b n to reenable script
    * attributes.
    */
   DPRINTF (("enabling binary mode\n"));
   ep_opt = sge_add_arg (opts, b_OPT, lIntT, "-b", "y");
   lSetInt (ep_opt, SPA_argval_lIntT, 1);
   
   /* DRMAA sends email by default.  Since DRMAA doesn't really specify when
    * email gets sent, we assume an innocuous default: send email when the job
    * is done. */
   DPRINTF (("Enabling email at end of job\n"));
   ep_opt = sge_add_arg (opts, m_OPT, lIntT, "-m", "e");
   lSetInt (ep_opt, SPA_argval_lIntT, MAIL_AT_EXIT);
   
   DEXIT;
}

/****** DRMAA/merge_drmaa_options() ********************************************
*  NAME
*     merge_drmaa_options() -- merge the various option lists into a single list
*
*  SYNOPSIS
*     void merge_drmaa_options (lList **opts_all, lList **opts_default,
*                               lList **opts_defaults, lList **opts_scriptfile,
*                               lList **opts_job_cat, lList **opts_native,
*                               lList **opts_drmaa)
*
*  FUNCTION
*     All options from the last six lists are cobined into the first list.  Each
*     list has prune_arg_list() called on it to remove inappropriate optnios.
*
*  INPUTS
*     lList **opts_all               - list to which options will be appended
*     lList **opts_default           - list with default options
*     lList **opts_defaults          - list with options from default files
*     lList **opts_scriptfile        - list with options from the script file
*     lList **opts_job_cat           - list with options from the job category
*     lList **opts_native            - list with options from the native spec
*     lList **opts_drmaa             - list with options fro the DRMAA attributes
*
*  NOTES
*     MT-NOTE: merge_drmaa_options() is MT safe
*
*******************************************************************************/
static void merge_drmaa_options (lList **opts_all, lList **opts_default,
                                 lList **opts_defaults, lList **opts_scriptfile,
                                 lList **opts_job_cat, lList **opts_native,
                                 lList **opts_drmaa) {
   DENTER (TOP_LAYER, "merge_drmaa_options");
   
   /*
    * Order is very important here
    */
   if (*opts_default != NULL) {
      DPRINTF (("Adding default options\n"));
      prune_arg_list (*opts_default);
      
      if (*opts_all == NULL) {
         *opts_all = *opts_default;
      } else {
         lAddList (*opts_all, *opts_default);
      }
      *opts_default = NULL;
   }
   
   if (*opts_defaults != NULL) {
      DPRINTF (("Adding defaults options\n"));
      prune_arg_list (*opts_defaults);
      
      if (*opts_all == NULL) {
         *opts_all = *opts_defaults;
      } else {
         lAddList (*opts_all, *opts_defaults);
      }
      *opts_defaults = NULL;
   }
   
   if (*opts_scriptfile != NULL) {
      DPRINTF (("Adding scriptfile options\n"));
      prune_arg_list (*opts_scriptfile);
      
      if (*opts_all == NULL) {
         DPRINTF (("Setting all options to scriptfile options\n"));
         *opts_all = *opts_scriptfile;
      } else {
         DPRINTF (("Copying scriptfile options\n"));
         lAddList (*opts_all, *opts_scriptfile);
      }
      DPRINTF (("Deleting scriptfile options\n"));
      *opts_scriptfile = NULL;
   }
   
   if (*opts_job_cat != NULL) {
      DPRINTF (("Adding job cat options\n"));
      prune_arg_list (*opts_job_cat);
      
      if (*opts_all == NULL) {
         *opts_all = *opts_job_cat;
      } else {
         lAddList (*opts_all, *opts_job_cat);
      }
      *opts_job_cat = NULL;
   }
   
   if (*opts_native != NULL) {
      DPRINTF (("Adding native options\n"));
      prune_arg_list (*opts_native);
      
      if (*opts_all == NULL) {
         *opts_all = *opts_native;
      } else {
         lAddList (*opts_all, *opts_native);
      }
      *opts_native = NULL;
   }
   
   if (*opts_drmaa != NULL) {
      DPRINTF (("Adding drmaa options\n"));
      /* No need to prune since options are already bounded by DRMAA */
      if (*opts_all == NULL) {
         *opts_all = *opts_drmaa;
      } else {
         lAddList (*opts_all, *opts_drmaa);
      }
      *opts_drmaa = NULL;
   }
   
   DEXIT;
}

/****** DRMAA/prune_arg_list() *************************************************
*  NAME
*     prune_arg_list() -- remove inappropriate options
*
*  SYNOPSIS
*     void prune_arg_list (lList *args)
*
*  FUNCTION
*     All options from the list which are not appropriate for DRMAA are removed
*
*  INPUTS
*     lList *args                    - list to prune
*
*  NOTES
*     MT-NOTE: prune_arg_list() is MT safe
*
*******************************************************************************/
static void prune_arg_list (lList *args) {
   /*  o=override, +=keep, -=remove
o   -a date_time                           request a job start time
+   -ac context_list                       add context variable(s)
+   -A account_string                      use account at host
+   -b y|n                                 handle command as binary   
+   -c ckpt_selector                       define type of checkpointing for job
+   -ckpt ckpt-name                        request checkpoint method
+   -clear                                 skip previous definitions for job
o   -cwd                                   use current working directory
+   -C directive_prefix                    define command prefix for job script
+   -dc simple_context_list                remove context variable(s)
+   -dl date_time                          request a deadline initiation time
o   -e path_list                           specify standard error stream path(s)
o   -h                                     place user hold on job
+   -hard                                  consider following requests "hard"
-   -help                                  print this help
+   -hold_jid job_identifier_list          define jobnet interdependencies
o   -i file_list                           specify standard input stream file(s)
+   -j y|n                                 merge stdout and stderr stream of job
+   -l resource_list                       request the given resources
+   -m mail_options                        define mail notification events
+   -masterq destin_id_list                bind master task to queue(s)
o   -M mail_list                           notify these e-mail addresses
+   -notify                                notify job before killing/suspending it
+   -now y[es]|n[o]                        start job immediately or not at all
o   -N name                                specify job name
o   -o path_list                           specify standard output stream path(s)
+   -p priority                            define job's relative priority
+   -pe pe-name slot_range                 request slot range for parallel jobs
+   -P project_name                        set job's project
+   -q destin_id_list                      bind job to queue(s)
+   -r y|n                                 define job as (not) restartable
+   -sc context_list                       set job context (replaces old context)
+   -soft                                  consider following requests as soft
+   -S path_list                           command interpreter to be used
-   -t task_id_range                       create a job-array with these tasks
o   -v variable_list                       export these environment variables
-   -verify                                do not submit just verify
o   -V                                     export all environment variables
-   -w e|w|n|v                             verify mode (error|warning|none|just verify) for jobs
+   -@ file                                read commandline input from file
   */
   lListElem *element = NULL;
   
   DENTER(TOP_LAYER, "prune_arg_list");
   
   /* skip arguments that aren't supported */
   while ((element = lGetElemStr (args, SPA_switch, "-help"))) {
      lRemoveElem (args, element);
   }
   
   /* This one isn't supported because bulk jobs are handled through the
    * drmaa_run_bulk_jobs() method. */
   while ((element = lGetElemStr (args, SPA_switch, "-t"))) {
      lRemoveElem (args, element);
   }
   
   while ((element = lGetElemStr (args, SPA_switch, "-verify"))) {
      lRemoveElem (args, element);
   }
   
   while ((element = lGetElemStr (args, SPA_switch, "-w"))) {
      lRemoveElem (args, element);
   }
   
   DEXIT;
}

/****** DRMAA/drmaa_time2sge_time() ********************************************
*  NAME
*     drmaa_time2sge_time() -- convert DRMAA time strings to SGE time strings
*
*  SYNOPSIS
*     void drmaa_time2sge_time (const char *drmaa_time, dstring *diag)
*
*  FUNCTION
*     The DRMAA time string is converted into an SGE time string.  If the
*     resulting time string represents a time in the past, and any of the date
*     elements were not specified in the DRMAA time string, the least order
*     date element will be incremented.
*
*  INPUTS
*     lList *drmaa_time              - the DRMAA time string
*
*  OUTPUTS
*     lList *diag                    - errors
*
*  NOTES
*     MT-NOTE: drmaa_time2sge_time() is MT safe
*
*******************************************************************************/
char *drmaa_time2sge_time (const char *drmaa_time, dstring *diag) {
   /* SGE time format is [[CC]]YY]MMDDhhmm.[ss] */
   /* DRMAA time format is [[[[CC]YY/]MM/]DD] hh:mm[:ss] [{-|+}UU:uu] */
   int year, month, day, hour, minute, second, tz_hours, tz_minutes;
   int century_set = 0, year_set = 0, month_set = 0, day_set = 0;
   int tz_diff_hours, tz_diff_minutes;
   char *p1, *p2, *start;
   char tz_sign, tmp[128], sge_time[16]; /* We will always build a string 15 + 1 long */
   time_t now;
   struct tm gmnow;
   struct tm herenow;
   
   DENTER (TOP_LAYER, "drmaa_test2sge_time");

   /* Get default times */
   time (&now);
   gmtime_r (&now, &gmnow);
   localtime_r (&now, &herenow);

   /* Set parsed times to defaults */
   year = -1;
   month = -1;
   day = -1;
   hour = -1;
   minute = -1;
   second = -1;
   tz_sign = -1;
   tz_hours = -1;
   tz_minutes = -1;
   start = strdup (drmaa_time);
   p1 = start;
   p2 = strchr (p1, '/');
   
   /* Look for year */
   if (p2 != NULL) {
      /* If we found a /, we know we have either a month or year */
      if ((p2 - p1)/sizeof (char) == 4) {
         /* 4 digit year given */
         strncpy (tmp, p1, 4);
         tmp[4] = 0;
         year = atoi (tmp);
         century_set = 1;
         year_set = 1;
         p1 = p2 + 1;
      }
      else if ((p2 - p1)/sizeof (char) == 2) {
         /* 2 digit year or month given.  We'll sort it out later. */
         strncpy (tmp, p1, 2);
         tmp[2] = 0;
         year = atoi (tmp);
         year_set = 1;
         p1 = p2 + 1;
      }
      else {
         /* Whatever comes before the slash can only be 2 or 4 characters */
         sge_dstring_copy_string (diag, "Error parsing DRMAA date string");
         DEXIT;
         return NULL;
      }
   }
   else {
      /* No year or month given.  Set the year now and worry about the month
       * later. */
      /* tm_year is number of years since 1900.  Since we add 2000 later, we
       * have to subtract 100 now. */
      year = gmnow.tm_year - 100;
   }
   
   p2 = strchr (p1, '/');
   
   /* If we found a second slash, that means the year, month, and day were
    * specified. */
   if (p2 != NULL) {
      /* 2 digit month given.  Set the month now and worry about the day later. */
      strncpy (tmp, p1, 2);
      tmp[2] = 0;
      month = atoi (tmp);
      month_set = 1;
      p1 = p2 + 1;
   }
   else {
      if (year_set) {
         /* If there's only on slash, what we thought was a year was really a
          * month. */
         month = year;
         /* tm_year is number of years since 1900.  Since we add 2000 later, we
          * have to subtract 100 now. */
         year = gmnow.tm_year - 100;
         month_set = 1;
         year_set = 0;
      }
      else {
         /* No month given */
         month = gmnow.tm_mon + 1;
      }
   }
   
   p2 = strchr (p1, ' ');
   
   /* If we find a space that's 2 characters from the last slash, we've found
    * the day. */
   if ((p2 != NULL) && ((p2 - p1)/sizeof (char) == 2)) {
      /* 2 digit day given. */
      strncpy (tmp, p1, 2);
      tmp[2] = 0;
      day = atoi (tmp);
      day_set = 1;
      p1 = p2 + 1;
   }
   else {
      /* No day given */
      day = gmnow.tm_mday;
   }

   /* Since hour and minute are required, we just use sscanf to read them.
    * sscanf also provides the added bonus of dealing with the whitespace for
    * us. */
   if (sscanf (p1, "%2d:%2d", &hour, &minute) != 2) {
      /* Hour and minute as hh:mm is required in DRMAA date strings. */
      sge_dstring_copy_string (diag, "Error parsing DRMAA date string");
      DEXIT;
      return NULL;
   }
   
   /* Rather than trying to figure out how much whitespace the sscanf skipped
    * before getting to the hour and minute, we just find the colon between the
    * hour and minute and add 2 to get past the minutes. */
   p2 = strchr (p1, ':');
   p1 = p2 + 1;
   p2 += 3;
   
   /* If the character after the minutes is a :, we've found the seconds. */
   if (*p2 == ':') {
      /* 2 digit seconds given */
      strncpy (tmp, p2 + 1, 2);
      tmp[2] = 0;
      second = atoi (tmp);
      /* Set our pointer to the end of the minutes plus 1 for the colon plus 2
       * for the seconds. */
      p1 = p2 + 3;
   }
   else {
      /* No seconds given */
      second = 0;
      /* Set our pointer to the end of the minutes. */
      p1 = p2;
   }
   
   /* Check that if a day and month were given that they are a valid
    * combination.  Also check that the hour, minute, and second are ok.  We're
    * making the assumption here that gmtime() isn't going to return out of
    * range values.  We deal with the case of setting a day and getting a month
    * from gmtime that don't work together later. */
   if ((day_set && (day > 31)) ||
       (month_set && (day > 30) && 
        ((month == 4) || (month == 6) || (month == 9) || (month == 11))) ||
       (month_set && (day > 29) &&
        ((month == 2) && (year % 4 == 0))) ||
       (month_set && (day > 28) && (month == 2)) ||
       (month_set && (month > 12)) ||
       (hour > 23) || (minute > 59) || (second > 59)) {
      sge_dstring_append (diag, "Error parsing DRMAA date string");
      DEXIT;
      return NULL;
   }
   /* If the day was gotten from gmtime(), check if the resulting date is in the
    * past.  If it is, increment the day and ripple the change through the
    * month and year. */
   else if (!day_set) {
      if ((hour < gmnow.tm_hour) ||
          ((hour == gmnow.tm_hour) &&
           ((minute < gmnow.tm_min) ||
            ((minute == gmnow.tm_min) && (second < gmnow.tm_sec))))) {
         day++;
         
         if ((day > 31) ||
             ((day > 30) &&
              ((month == 4) || (month == 6) || (month == 9) || (month == 11))) ||
             ((day > 29) &&
              ((month == 2) && (year % 4 == 0))) ||
             ((day > 28) && (month == 2))) {
            day = 1;
            month++;
             
            if (month > 12) {
               month = 1;
               year++;
            }
         }
      }
   }
   /* If the month was gotten from gmtime(), check if the resulting date is in
    * the past.  If it is, increment the month and ripple the change though the
    * year. */
   else if (!month_set) {
      /* Make sure that the date is not in the past.  It doesn't matter if the
       * day of the month doesn't exist in the current month.  From the
       * perspective of determining what's earlier, it will work fine. */
      if ((day < gmnow.tm_mday) ||
               ((day == gmnow.tm_mday) &&
               ((hour < gmnow.tm_hour) ||
                 ((hour == gmnow.tm_hour) &&
                  ((minute < gmnow.tm_min) ||
                   ((minute == gmnow.tm_min) && (second < gmnow.tm_sec))))))) {
         month++;

         if (month > 12) {
            month = 1;
            year++;
         }
      }
      
      /* Now make sure that the month and day make sense together. */
      if ((day > 30) && ((month == 4) || (month == 6) || (month == 9) || (month == 11))) {
         day -= 30;
         month++;
      }
      else if ((day > 29) && ((month == 2) && (year % 4 == 0))) {
         day -= 29;
         month++;
      }
      else if ((day > 28) && (month == 2)) {
         day -= 28;
         month++;
      }
   }
   /* If the year was gotten from gmtime(), check if the resulting date is in
    * the past.  If it is, increment the year.  The change will automatically
    * ripple through the century. */
   else if (!year_set) {
      if ((month < gmnow.tm_mon + 1) ||
          ((month == gmnow.tm_mon + 1) &&
           ((day < gmnow.tm_mday) ||
            ((day == gmnow.tm_mday) &&
             ((hour < gmnow.tm_hour) ||
              ((hour == gmnow.tm_hour) &&
               ((minute < gmnow.tm_min) ||
                ((minute == gmnow.tm_min) && (second < gmnow.tm_sec))))))))) {
      /* Here we can just increment the year because even if it grows larger
       * than 99, since the century is added to it, it all works out. */
         year++;
      }
   }

   /* If the year was set as two digits, or if the year was gotten from gmtime(),
    * Add the current century to it. */
   if (!century_set) {
      year += 2000;
   }
   
   /* It's ok to deal with the timezone after doing all the math to validate the
    * date because timezone only affects hours and minutes, and we never
    * increment or get defaults for hours or minutes; they're required to be
    * specified. */
   tz_diff_hours = herenow.tm_hour - gmnow.tm_hour;
   tz_diff_minutes = herenow.tm_min - gmnow.tm_min;

   /* We use sscanf to deal with the timezone information because of the potential
    * whitespace between the minute/second and the tz info and because the tz
    * info is either all present or all not present. */
   if (sscanf (p1, "%1s%2d:%2d", &tz_sign, &tz_hours, &tz_minutes) == 3) {
      /* If we read all three field, check the sign and adjust the hour and
       * minute accordingly. */
      if (tz_sign == '+') {
         hour += tz_diff_hours - tz_hours;
         minute += tz_diff_minutes - tz_minutes;
      }
      else if (tz_sign == '-') {
         hour += tz_diff_hours + tz_hours;
         minute += tz_diff_minutes + tz_minutes;
      }
      else {
         /* The sign must always be present and be a + or - */
         sge_dstring_copy_string (diag, "Error parsing DRMAA date string");
         DEXIT;
         return NULL;
      }
   }
   /* If no timezone info was given, just use whatever hour and minute were in
    * the DRMAA date string. */

   /* Build the SGE date string from the parsed components. sprintf adds the
    * terminating character for us. */
   sprintf (sge_time, "%.4d%.2d%.2d%.2d%.2d.%.2d", year, month, day, hour,
                                                   minute, second);

   FREE (start);

   DEXIT;
   return strdup (sge_time);
}
