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

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "sgermon.h"
#include "basis_types.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_dstring.h"
#include "sge_parse_num_par.h"
#include "sge_hostname.h"
#include "sge_spool.h"
#include "setup_path.h"
#include "sge_answer.h"
#include "msg_utilib.h"
#include "sge_error_class.h"
#include "sge_bootstrap.h"

typedef struct {
    char* admin_user;      
    char* default_domain;
    bool        ignore_fqdn;
    char* spooling_method;
    char* spooling_lib;
    char* spooling_params;
    char* binary_path;
    char* qmaster_spool_dir;
    char* security_mode;
} bootstrap_state_t;

static pthread_once_t bootstrap_once = PTHREAD_ONCE_INIT;
static pthread_key_t bootstrap_state_key;

static void bootstrap_once_init(void);
static void bootstrap_state_destroy(void* theState);
static void bootstrap_state_init(bootstrap_state_t* theState);
 
typedef bootstrap_state_t sge_bootstrap_state_t;

static bool sge_bootstrap_state_setup(sge_bootstrap_state_class_t *thiz, sge_path_state_class_t *sge_paths, sge_error_class_t *eh);
static void sge_bootstrap_state_dprintf(sge_bootstrap_state_class_t *thiz);
static const char* get_admin_user(sge_bootstrap_state_class_t *thiz);
static const char* get_default_domain(sge_bootstrap_state_class_t *thiz);
static const char* get_spooling_method(sge_bootstrap_state_class_t *thiz);
static const char* get_spooling_lib(sge_bootstrap_state_class_t *thiz);
static const char* get_spooling_params(sge_bootstrap_state_class_t *thiz);
static const char* get_binary_path(sge_bootstrap_state_class_t *thiz);
static const char* get_qmaster_spool_dir(sge_bootstrap_state_class_t *thiz);
static const char* get_security_mode(sge_bootstrap_state_class_t *thiz);
static bool get_ignore_fqdn(sge_bootstrap_state_class_t *thiz);
static void set_admin_user(sge_bootstrap_state_class_t *thiz, const char *admin_user);
static void set_default_domain(sge_bootstrap_state_class_t *thiz, const char *default_domain);
static void set_spooling_method(sge_bootstrap_state_class_t *thiz, const char *spooling_method);
static void set_spooling_lib(sge_bootstrap_state_class_t *thiz, const char *spooling_lib);
static void set_spooling_params(sge_bootstrap_state_class_t *thiz, const char *spooling_params);
static void set_binary_path(sge_bootstrap_state_class_t *thiz, const char *binary_path);
static void set_qmaster_spool_dir(sge_bootstrap_state_class_t *thiz, const char *qmaster_spool_dir);
static void set_security_mode(sge_bootstrap_state_class_t *thiz, const char *security_mode);
static void set_ignore_fqdn(sge_bootstrap_state_class_t *thiz, bool ignore_fqdn);


/*** AA **/ 

/****** uti/sge_bootstrap/bootstrap_mt_init() **********************************
*  NAME
*     bootstrap_mt_init() -- Initialize bootstrap code for multi threading use.
*
*  SYNOPSIS
*     void bootstrap_mt_init(void) 
*
*  FUNCTION
*     Set up bootstrap code. This function must be called at least once before
*     any of the bootstrap oriented functions can be used. This function is
*     idempotent, i.e. it is safe to call it multiple times.
*
*     Thread local storage for the bootstrap state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: bootstrap_mt_init() is MT safe 
*
*******************************************************************************/
void bootstrap_mt_init(void)
{
   pthread_once(&bootstrap_once, bootstrap_once_init);
}

const char *bootstrap_get_admin_user(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_admin_user");
   return bootstrap->admin_user;
}

const char *bootstrap_get_default_domain(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_default_domain");
   return bootstrap->default_domain;
}

bool bootstrap_get_ignore_fqdn(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_ignore_fqdn");
   return bootstrap->ignore_fqdn;
}

const char *bootstrap_get_spooling_method(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_method");
   return bootstrap->spooling_method;
}

const char *bootstrap_get_spooling_lib(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_lib");
   return bootstrap->spooling_lib;
}

const char *bootstrap_get_spooling_params(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_params");
   return bootstrap->spooling_params;
}

const char *bootstrap_get_binary_path(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_binary_path");
   return bootstrap->binary_path;
}

const char *bootstrap_get_qmaster_spool_dir(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_qmaster_spool_dir");
   return bootstrap->qmaster_spool_dir;
}

const char *bootstrap_get_security_mode(void)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_security_mode");
   return bootstrap->security_mode;
}

void bootstrap_set_admin_user(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_admin_user");
   bootstrap->admin_user = sge_strdup((char *)bootstrap->admin_user, value);
}

void bootstrap_set_default_domain(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_default_domain");
   bootstrap->default_domain = sge_strdup((char *)bootstrap->default_domain, 
                                          value);
}

void bootstrap_set_ignore_fqdn(bool value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_ignore_fqdn");
   bootstrap->ignore_fqdn = value;
}

void bootstrap_set_spooling_method(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_method");
   bootstrap->spooling_method = sge_strdup((char *)bootstrap->spooling_method, 
                                           value);
}

void bootstrap_set_spooling_lib(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_lib");
   bootstrap->spooling_lib = sge_strdup((char *)bootstrap->spooling_lib, value);
}

void bootstrap_set_spooling_params(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_params");
   bootstrap->spooling_params = sge_strdup((char *)bootstrap->spooling_params, 
                                           value);
}

void bootstrap_set_binary_path(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_binary_path");
   bootstrap->binary_path = sge_strdup((char *)bootstrap->binary_path, 
                                           value);
}

void bootstrap_set_qmaster_spool_dir(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_qmaster_spool_dir");
   bootstrap->qmaster_spool_dir = sge_strdup((char *)bootstrap->qmaster_spool_dir, 
                                           value);
}

void bootstrap_set_security_mode(const char *value)
{
   GET_SPECIFIC(bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_security_mode");
   bootstrap->security_mode = sge_strdup((char *)bootstrap->security_mode, 
                                           value);
}

/****** sge_bootstrap/sge_bootstrap() ******************************************
*  NAME
*     sge_bootstrap() -- read and process bootstrap file 
*
*  SYNOPSIS
*     bool sge_bootstrap(void) 
*
*  FUNCTION
*     Reads the bootstrap file ($SGE_ROOT/$SGE_CELL/common/bootstrap).
*     
*     Initializes the policy for hostcpy and hostcmp (ignore_fqdn and 
*     default_domain).
*
*  INPUTS
*     dstring *error_dstring - dynamic string buffer to return error messages.
*                              if error_string is NULL, error messages will be
*                              written using the sge_log functions, else they
*                              will be returned in error_dstring.
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: sge_bootstrap() is MT safe
*******************************************************************************/
#define NUM_BOOTSTRAP 9
bool sge_bootstrap(dstring *error_dstring) 
{
   bool ret = true;

   const char *bootstrap_file;
   const char *name[NUM_BOOTSTRAP] = { "admin_user",
                                       "default_domain",
                                       "ignore_fqdn",
                                       "spooling_method",
                                       "spooling_lib", 
                                       "spooling_params",
                                       "binary_path", 
                                       "qmaster_spool_dir",
                                       "security_mode"
                                     };
   char value[NUM_BOOTSTRAP][1025];

   DENTER(TOP_LAYER, "sge_bootstrap");

   /* get filepath of bootstrap file */
   bootstrap_file = path_state_get_bootstrap_file();
   if (bootstrap_file == NULL) {
      if (error_dstring == NULL) {
         CRITICAL((SGE_EVENT, MSG_UTI_CANNOTRESOLVEBOOTSTRAPFILE));
      } else {
         sge_dstring_sprintf(error_dstring, MSG_UTI_CANNOTRESOLVEBOOTSTRAPFILE);
      }
      ret = false;
   /* read bootstrapping information */   
   } else if (sge_get_confval_array(bootstrap_file, NUM_BOOTSTRAP, name, 
                                    value, error_dstring)) {
      /*
      if (error_dstring == NULL) {
         CRITICAL((SGE_EVENT, MSG_UTI_CANNOTBOOTSTRAP_S, bootstrap_file));
      } else {
         sge_dstring_sprintf(error_dstring, MSG_UTI_CANNOTBOOTSTRAP_S, 
                             bootstrap_file);
      }
      */
      ret = false;
   } else {
      /* store bootstrapping information */
      bootstrap_set_admin_user(value[0]);
      bootstrap_set_default_domain(value[1]);
      bootstrap_set_spooling_method(value[3]);
      bootstrap_set_spooling_lib(value[4]);
      bootstrap_set_spooling_params(value[5]);
      bootstrap_set_binary_path(value[6]);
      bootstrap_set_qmaster_spool_dir(value[7]);
      bootstrap_set_security_mode(value[8]);
      {
         u_long32 uval;
         parse_ulong_val(NULL, &uval, TYPE_BOO, value[2], 
                         NULL, 0);
         bootstrap_set_ignore_fqdn(uval ? true : false);
      }

      DPRINTF(("admin_user          >%s<\n", bootstrap_get_admin_user()));
      DPRINTF(("default_domain      >%s<\n", bootstrap_get_default_domain()));
      DPRINTF(("ignore_fqdn         >%s<\n", bootstrap_get_ignore_fqdn() ? 
                                             "true" : "false"));
      DPRINTF(("spooling_method     >%s<\n", bootstrap_get_spooling_method()));
      DPRINTF(("spooling_lib        >%s<\n", bootstrap_get_spooling_lib()));
      DPRINTF(("spooling_params     >%s<\n", bootstrap_get_spooling_params()));
      DPRINTF(("binary_path         >%s<\n", bootstrap_get_binary_path()));
      DPRINTF(("qmaster_spool_dir   >%s<\n", bootstrap_get_qmaster_spool_dir()));
      DPRINTF(("security_mode        >%s<\n", bootstrap_get_security_mode()));
   } 
   
   DEXIT;
   return ret;
}


/****** uti/sge_bootstrap/bootstrap_once_init() ********************************
*  NAME
*     bootstrap_once_init() -- One-time bootstrap code initialization.
*
*  SYNOPSIS
*     static bootstrap_once_init(void) 
*
*  FUNCTION
*     Create access key for thread local storage. Register cleanup function.
*
*     This function must be called exactly once.
*
*  INPUTS
*     void - none
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: bootstrap_once_init() is MT safe. 
*
*******************************************************************************/
static void bootstrap_once_init(void)
{
   pthread_key_create(&bootstrap_state_key, bootstrap_state_destroy);
}

/****** uti/sge_bootstrap/bootstrap_state_destroy() ****************************
*  NAME
*     bootstrap_state_destroy() -- Free thread local storage
*
*  SYNOPSIS
*     static void bootstrap_state_destroy(void* theState) 
*
*  FUNCTION
*     Free thread local storage.
*
*  INPUTS
*     void* theState - Pointer to memory which should be freed.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: bootstrap_state_destroy() is MT safe.
*
*******************************************************************************/
static void bootstrap_state_destroy(void* theState)
{
   FREE(((bootstrap_state_t*)theState)->admin_user);
   FREE(((bootstrap_state_t*)theState)->default_domain);
   FREE(((bootstrap_state_t*)theState)->spooling_method);
   FREE(((bootstrap_state_t*)theState)->spooling_lib);
   FREE(((bootstrap_state_t*)theState)->spooling_params);
   FREE(((bootstrap_state_t*)theState)->binary_path);
   FREE(((bootstrap_state_t*)theState)->qmaster_spool_dir);
   FREE(((bootstrap_state_t*)theState)->security_mode);
   free(theState);
}

/****** uti/sge_bootstrap/bootstrap_state_init() *******************************
*  NAME
*     bootstrap_state_init() -- Initialize bootstrap state.
*
*  SYNOPSIS
*     static void bootstrap_state_init(bootstrap_state_t* theState) 
*
*  FUNCTION
*     Initialize bootstrap state.
*
*  INPUTS
*     bootstrap_state_t* theState - Pointer to bootstrap state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: bootstrap_state_init() is MT safe. 
*
*******************************************************************************/
static void bootstrap_state_init(bootstrap_state_t* theState)
{
   memset(theState, 0, sizeof(bootstrap_state_t));
}


/*-------------------------------------------------------------------------*/

sge_bootstrap_state_class_t *sge_bootstrap_state_class_create(sge_path_state_class_t *sge_paths, sge_error_class_t *eh)
{
   sge_bootstrap_state_class_t *ret = (sge_bootstrap_state_class_t *)sge_malloc(sizeof(sge_bootstrap_state_class_t));

   DENTER(TOP_LAYER, "sge_bootstrap_state_class_create");

   if (!ret) {
      eh->error(eh, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      DEXIT;
      return NULL;
   }
   
   ret->dprintf = sge_bootstrap_state_dprintf;

   ret->get_admin_user = get_admin_user;
   ret->get_default_domain = get_default_domain;
   ret->get_ignore_fqdn = get_ignore_fqdn;
   ret->get_spooling_method = get_spooling_method;
   ret->get_spooling_lib = get_spooling_lib;
   ret->get_spooling_params = get_spooling_params;
   ret->get_binary_path = get_binary_path;
   ret->get_qmaster_spool_dir = get_qmaster_spool_dir;
   ret->get_security_mode = get_security_mode;

   ret->set_admin_user = set_admin_user;
   ret->set_default_domain = set_default_domain;
   ret->set_ignore_fqdn = set_ignore_fqdn;
   ret->set_spooling_method = set_spooling_method;
   ret->set_spooling_lib = set_spooling_lib;
   ret->set_spooling_params = set_spooling_params;
   ret->set_binary_path = set_binary_path;
   ret->set_qmaster_spool_dir = set_qmaster_spool_dir;
   ret->set_security_mode = set_security_mode;

   ret->sge_bootstrap_state_handle = sge_malloc(sizeof(sge_bootstrap_state_t));
   
   if (ret->sge_bootstrap_state_handle == NULL ) {
      eh->error(eh, STATUS_EMALLOC, ANSWER_QUALITY_ERROR, MSG_MEMORY_MALLOCFAILED);
      FREE(ret);
      DEXIT;
      return NULL;
   }
   memset(ret->sge_bootstrap_state_handle, 0, sizeof(sge_bootstrap_state_t));

   if (!sge_bootstrap_state_setup(ret, sge_paths, eh)) {
      sge_bootstrap_state_class_destroy(&ret);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return ret;
}   

void sge_bootstrap_state_class_destroy(sge_bootstrap_state_class_t **pst)
{
   DENTER(TOP_LAYER, "sge_bootstrap_state_class_destroy");
   if (!pst || !*pst) {
      DEXIT;
      return;
   }   
   bootstrap_state_destroy((*pst)->sge_bootstrap_state_handle);
   FREE(*pst);
   *pst = NULL;

   DEXIT;
}

static bool sge_bootstrap_state_setup(sge_bootstrap_state_class_t *thiz, sge_path_state_class_t *sge_paths, sge_error_class_t *eh)
{
   #define NUM_BOOTSTRAP 9

   dstring error_dstring = DSTRING_INIT;
   const char *bootstrap_file = NULL;
   const char *name[NUM_BOOTSTRAP] = { "admin_user",
                                       "default_domain",
                                       "ignore_fqdn",
                                       "spooling_method",
                                       "spooling_lib", 
                                       "spooling_params",
                                       "binary_path", 
                                       "qmaster_spool_dir",
                                       "security_mode"
                                     };
   char value[NUM_BOOTSTRAP][1025];

   DENTER(TOP_LAYER, "sge_bootstrap_state_setup");

   if (!sge_paths) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, "sge_paths is NULL");
      DEXIT;
      return FALSE;
   }

   /* get filepath of bootstrap file */
   bootstrap_file = sge_paths->get_bootstrap_file(sge_paths);
   if (bootstrap_file == NULL) {
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, MSG_UTI_CANNOTRESOLVEBOOTSTRAPFILE);
      DEXIT;
      return FALSE;
   } 
   
   /* read bootstrapping information */   
   if (sge_get_confval_array(bootstrap_file, NUM_BOOTSTRAP, name, 
                                    value, &error_dstring)) {
                                       
      eh->error(eh, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, sge_dstring_get_string(&error_dstring));
      sge_dstring_free(&error_dstring);
      DEXIT;
      return FALSE;
   } 

   /* store bootstrapping information */
   thiz->set_admin_user(thiz, value[0]);
   thiz->set_default_domain(thiz, value[1]);
   thiz->set_spooling_method(thiz, value[3]);
   thiz->set_spooling_lib(thiz, value[4]);
   thiz->set_spooling_params(thiz, value[5]);
   thiz->set_binary_path(thiz, value[6]);
   thiz->set_qmaster_spool_dir(thiz, value[7]);
   thiz->set_security_mode(thiz, value[8]);
   {
      u_long32 uval;
      parse_ulong_val(NULL, &uval, TYPE_BOO, value[2], 
                      NULL, 0);
      thiz->set_ignore_fqdn(thiz, uval ? true : false);
   }

   thiz->dprintf(thiz);

   DEXIT;
   return TRUE;
}

static void sge_bootstrap_state_dprintf(sge_bootstrap_state_class_t *thiz)
{
   sge_bootstrap_state_t *bs = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   
   DENTER(TOP_LAYER, "sge_bootstrap_state_dprintf");

   DPRINTF(("admin_user          >%s<\n", bs->admin_user));
   DPRINTF(("default_domain      >%s<\n", bs->default_domain));
   DPRINTF(("ignore_fqdn         >%s<\n", bs->ignore_fqdn ?  "true" : "false"));
   DPRINTF(("spooling_method     >%s<\n", bs->spooling_method));
   DPRINTF(("spooling_lib        >%s<\n", bs->spooling_lib));
   DPRINTF(("spooling_params     >%s<\n", bs->spooling_params));
   DPRINTF(("binary_path         >%s<\n", bs->binary_path));
   DPRINTF(("qmaster_spool_dir   >%s<\n", bs->qmaster_spool_dir));
   DPRINTF(("security_mode       >%s<\n", bs->security_mode));
   
   DEXIT;
}

static const char* get_admin_user(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->admin_user;
}

static const char* get_default_domain(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->default_domain;
}

static const char* get_spooling_method(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->spooling_method;
}

static const char* get_spooling_lib(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->spooling_lib;
}

static const char* get_spooling_params(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->spooling_params;
}

static const char* get_binary_path(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->binary_path;
}

static const char* get_qmaster_spool_dir(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->qmaster_spool_dir;
}

static const char* get_security_mode(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->security_mode;
}

static bool get_ignore_fqdn(sge_bootstrap_state_class_t *thiz) 
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   return es->ignore_fqdn;
}

static void set_admin_user(sge_bootstrap_state_class_t *thiz, const char *admin_user)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->admin_user = sge_strdup(es->admin_user, admin_user);
}

static void set_default_domain(sge_bootstrap_state_class_t *thiz, const char *default_domain)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->default_domain = sge_strdup(es->default_domain, default_domain);
}

static void set_spooling_method(sge_bootstrap_state_class_t *thiz, const char *spooling_method)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->spooling_method = sge_strdup(es->spooling_method, spooling_method);
}

static void set_spooling_lib(sge_bootstrap_state_class_t *thiz, const char *spooling_lib)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->spooling_lib = sge_strdup(es->spooling_lib, spooling_lib);
}

static void set_spooling_params(sge_bootstrap_state_class_t *thiz, const char *spooling_params)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->spooling_params = sge_strdup(es->spooling_params, spooling_params);
}

static void set_binary_path(sge_bootstrap_state_class_t *thiz, const char *binary_path)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->binary_path = sge_strdup(es->binary_path, binary_path);
}

static void set_qmaster_spool_dir(sge_bootstrap_state_class_t *thiz, const char *qmaster_spool_dir)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->qmaster_spool_dir = sge_strdup(es->qmaster_spool_dir, qmaster_spool_dir);
}

static void set_security_mode(sge_bootstrap_state_class_t *thiz, const char *security_mode)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->security_mode = sge_strdup(es->security_mode, security_mode);
}

static void set_ignore_fqdn(sge_bootstrap_state_class_t *thiz, bool ignore_fqdn)
{
   sge_bootstrap_state_t *es = (sge_bootstrap_state_t *) thiz->sge_bootstrap_state_handle;
   es->ignore_fqdn = ignore_fqdn;
}

