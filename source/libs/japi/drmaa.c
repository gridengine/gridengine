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

/* UTI */
#include "sge_dstring.h"
#include "sge_prog.h"
#include "sge_string.h"

/* RMON */
#include "sgermon.h"

/* SGEOBJ */
#include "sge_path_alias.h"
#include "sge_job.h"

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

static const char *session_key_env_var = "SGE_SESSION_KEY"; 
static const char *session_shutdown_mode_env_var = "SGE_KEEP_SESSION";

static int drmaa_is_supported(const char *name, const char *supported_list[]);
static drmaa_attr_names_t *drmaa_fill_string_vector(const char *name[]);

static int drmaa_job2sge_job(lListElem **jtp, drmaa_job_template_t *drmaa_jt, 
   int is_bulk, int start, int end, int step, dstring *diag);
static int japi_drmaa_path2sge_job(drmaa_job_template_t *drmaa_jt, lListElem *jt, int is_bulk, 
   int nm, const char *attribute_key, dstring *diag);

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
#if 0
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
#endif
   NULL
};

/* these vector job template attributes are supported */
static const char *drmaa_supported_vector[] = {
   DRMAA_V_ARGV,  /* mandatory */
#if 0
   DRMAA_V_ENV,   /* mandatory */
   DRMAA_V_EMAIL, /* mandatory */
#endif
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

/****** DRMAA/drmaa_job2sge_job() ******************************************
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
      /* 
       * Currently we always use binary submission mode.
       * A SGE job template attribute could be supported to 
       * allow also script jobs be submitted. These scripts would
       * be transfered to the execution machine. 
       */
      JOB_TYPE_SET_BINARY(jb_now);

      /*
       * An error state does not exist with DRMAA jobs.
       * This setting is necessary to ensure e.g. jobs
       * with a wrong input path specification fail when
       * doing drmaa_wait(). A SGE job template attribute
       * could be supported to enable SGE error state.
       */
      JOB_TYPE_SET_NO_ERROR(jb_now);

      /* mark array job */
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

#if 0
   /* use code in sge_qtcsh to disassemble DRMAA_NATIVE_SPECIFICATION 
      into argv string list*/

   /* run cull_parse_cmdline() to preprocess argv into SPA_switch list */

   /* run code in cull_parse_job_parameter() to modify existing job 

      -ac                                               supported
      -b                                                not supported
      -t        use DRMAA bulk jobs instead             not supported
      -clear                                            not supported
      -A        use DRMAA_START_TIME instead            not supported
      -dc                                               supported
      -dl                                               supported
      -c                                                supported
      -ckpt                                             supported
      -cwd      use DRMAA_WD instead                    not supported
      -C        not relevant without '-b n'             not supported
      -e        use DRMAA_ERROR_PATH instead            not supported
      -hard                                             supported
      -hold_jid                                         supported
      -j        use DRMAA_JOIN_FILES instead            supported
      -l                                                supported
      -m        DRMAA_BLOCK_EMAIL can override this     supported  
      -M        use DRMAA_V_EMAIL instead               not supported  
      -N        use DRMAA_JOB_NAME instead              not supported  
      -notify                                           supported  
      -now                                              supported  
      -o        used DRMAA_OUTPUT_PATH instead          not supported
      -i        used DRMAA_INPUT_PATH instead           not supported
      -p                                                supported
      -pe                                               supported
      -P                                                supported
      -q                                                supported
      -masterq                                          supported
      -r                                                supported
      -sc                                               supported
      -soft                                             supported
      -S                                                supported
      -V        use DRMAA_V_ENV instead                 not supported
      -v        use DRMAA_V_ENV instead                 not supported
      -w                                                not supported
   
   */
#endif

   *jtp = jt;
   DEXIT;
   return DRMAA_ERRNO_SUCCESS;
}

