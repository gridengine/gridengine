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
#include "msg_utilib.h"

#include "sge_bootstrap.h"

struct bootstrap_state_t {
    const char* admin_user;      
    const char* default_domain;
    bool        ignore_fqdn;
    const char* spooling_method;
    const char* spooling_lib;
    const char* spooling_params;
    const char* binary_path;
    const char* qmaster_spool_dir;
    const char* security_mode;
};

static pthread_once_t bootstrap_once = PTHREAD_ONCE_INIT;
static pthread_key_t bootstrap_state_key;

static void bootstrap_once_init(void);
static void bootstrap_state_destroy(void* theState);
static void bootstrap_state_init(struct bootstrap_state_t* theState);
 

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
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_admin_user");
   return bootstrap->admin_user;
}

const char *bootstrap_get_default_domain(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_default_domain");
   return bootstrap->default_domain;
}

bool bootstrap_get_ignore_fqdn(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_ignore_fqdn");
   return bootstrap->ignore_fqdn;
}

const char *bootstrap_get_spooling_method(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_method");
   return bootstrap->spooling_method;
}

const char *bootstrap_get_spooling_lib(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_lib");
   return bootstrap->spooling_lib;
}

const char *bootstrap_get_spooling_params(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_spooling_params");
   return bootstrap->spooling_params;
}

const char *bootstrap_get_binary_path(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_binary_path");
   return bootstrap->binary_path;
}

const char *bootstrap_get_qmaster_spool_dir(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_qmaster_spool_dir");
   return bootstrap->qmaster_spool_dir;
}

const char *bootstrap_get_security_mode(void)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_get_security_mode");
   return bootstrap->security_mode;
}

void bootstrap_set_admin_user(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_admin_user");
   bootstrap->admin_user = sge_strdup((char *)bootstrap->admin_user, value);
}

void bootstrap_set_default_domain(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_default_domain");
   bootstrap->default_domain = sge_strdup((char *)bootstrap->default_domain, 
                                          value);
}

void bootstrap_set_ignore_fqdn(bool value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_ignore_fqdn");
   bootstrap->ignore_fqdn = value;
}

void bootstrap_set_spooling_method(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_method");
   bootstrap->spooling_method = sge_strdup((char *)bootstrap->spooling_method, 
                                           value);
}

void bootstrap_set_spooling_lib(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_lib");
   bootstrap->spooling_lib = sge_strdup((char *)bootstrap->spooling_lib, value);
}

void bootstrap_set_spooling_params(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_spooling_params");
   bootstrap->spooling_params = sge_strdup((char *)bootstrap->spooling_params, 
                                           value);
}

void bootstrap_set_binary_path(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_binary_path");
   bootstrap->binary_path = sge_strdup((char *)bootstrap->binary_path, 
                                           value);
}

void bootstrap_set_qmaster_spool_dir(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
                "bootstrap_set_qmaster_spool_dir");
   bootstrap->qmaster_spool_dir = sge_strdup((char *)bootstrap->qmaster_spool_dir, 
                                           value);
}

void bootstrap_set_security_mode(const char *value)
{
   GET_SPECIFIC(struct bootstrap_state_t, bootstrap, bootstrap_state_init, bootstrap_state_key, 
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
         bootstrap_set_ignore_fqdn(uval);
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
   FREE(((struct bootstrap_state_t*)theState)->admin_user);
   FREE(((struct bootstrap_state_t*)theState)->default_domain);
   FREE(((struct bootstrap_state_t*)theState)->spooling_method);
   FREE(((struct bootstrap_state_t*)theState)->spooling_lib);
   FREE(((struct bootstrap_state_t*)theState)->spooling_params);
   FREE(((struct bootstrap_state_t*)theState)->binary_path);
   FREE(((struct bootstrap_state_t*)theState)->qmaster_spool_dir);
   FREE(((struct bootstrap_state_t*)theState)->security_mode);
   free(theState);
}

/****** uti/sge_bootstrap/bootstrap_state_init() *******************************
*  NAME
*     bootstrap_state_init() -- Initialize bootstrap state.
*
*  SYNOPSIS
*     static void bootstrap_state_init(struct bootstrap_state_t* theState) 
*
*  FUNCTION
*     Initialize bootstrap state.
*
*  INPUTS
*     struct bootstrap_state_t* theState - Pointer to bootstrap state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: bootstrap_state_init() is MT safe. 
*
*******************************************************************************/
static void bootstrap_state_init(struct bootstrap_state_t* theState)
{
   memset(theState, 0, sizeof(struct bootstrap_state_t));
}
