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

#include <errno.h>

#if defined(SGE_MT)
#include <pthread.h>
#endif

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

struct bootstrap_t {
    const char       *admin_user;      
    const char       *default_domain;
    bool              ignore_fqdn;
    const char       *spooling_method;
    const char       *spooling_lib;
    const char       *spooling_params;
    const char       *binary_path;
    const char       *qmaster_spool_dir;
};

#if defined(SGE_MT)
static pthread_key_t   bootstrap_key;
#else
static struct bootstrap_t bootstrap_opaque = {
  NULL, NULL, false, NULL, NULL, NULL };
struct bootstrap_t *bootstrap = &bootstrap_opaque;
#endif

#if defined(SGE_MT)
static void bootstrap_init(struct bootstrap_t* bootstrap) {
   memset(bootstrap, 0, sizeof(struct bootstrap_t));
}

static void bootstrap_destroy(void* bootstrap) {
   FREE(((struct bootstrap_t*)bootstrap)->admin_user);
   FREE(((struct bootstrap_t*)bootstrap)->default_domain);
   FREE(((struct bootstrap_t*)bootstrap)->spooling_method);
   FREE(((struct bootstrap_t*)bootstrap)->spooling_lib);
   FREE(((struct bootstrap_t*)bootstrap)->spooling_params);
   FREE(((struct bootstrap_t*)bootstrap)->binary_path);
   FREE(((struct bootstrap_t*)bootstrap)->qmaster_spool_dir);
   free(bootstrap);
}
 
void bootstrap_init_mt(void) {
   pthread_key_create(&bootstrap_key, &bootstrap_destroy);
} 
#endif

const char *bootstrap_get_admin_user(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_admin_user");
   return bootstrap->admin_user;
}

const char *bootstrap_get_default_domain(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_default_domain");
   return bootstrap->default_domain;
}

bool bootstrap_get_ignore_fqdn(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_ignore_fqdn");
   return bootstrap->ignore_fqdn;
}

const char *bootstrap_get_spooling_method(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_spooling_method");
   return bootstrap->spooling_method;
}

const char *bootstrap_get_spooling_lib(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_spooling_lib");
   return bootstrap->spooling_lib;
}

const char *bootstrap_get_spooling_params(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_spooling_params");
   return bootstrap->spooling_params;
}

const char *bootstrap_get_binary_path(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_binary_path");
   return bootstrap->binary_path;
}

const char *bootstrap_get_qmaster_spool_dir(void)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_get_qmaster_spool_dir");
   return bootstrap->qmaster_spool_dir;
}

void bootstrap_set_admin_user(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_admin_user");
   bootstrap->admin_user = sge_strdup((char *)bootstrap->admin_user, value);
}

void bootstrap_set_default_domain(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_default_domain");
   bootstrap->default_domain = sge_strdup((char *)bootstrap->default_domain, 
                                          value);
}

void bootstrap_set_ignore_fqdn(bool value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_ignore_fqdn");
   bootstrap->ignore_fqdn = value;
}

void bootstrap_set_spooling_method(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_spooling_method");
   bootstrap->spooling_method = sge_strdup((char *)bootstrap->spooling_method, 
                                           value);
}

void bootstrap_set_spooling_lib(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_spooling_lib");
   bootstrap->spooling_lib = sge_strdup((char *)bootstrap->spooling_lib, value);
}

void bootstrap_set_spooling_params(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_spooling_params");
   bootstrap->spooling_params = sge_strdup((char *)bootstrap->spooling_params, 
                                           value);
}

void bootstrap_set_binary_path(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_binary_path");
   bootstrap->binary_path = sge_strdup((char *)bootstrap->binary_path, 
                                           value);
}

void bootstrap_set_qmaster_spool_dir(const char *value)
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "bootstrap_set_qmaster_spool_dir");
   bootstrap->qmaster_spool_dir = sge_strdup((char *)bootstrap->qmaster_spool_dir, 
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
#define NUM_BOOTSTRAP 8
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
                                       "qmaster_spool_dir"
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
                                    value)) {
      if (error_dstring == NULL) {
      CRITICAL((SGE_EVENT, MSG_UTI_CANNOTBOOTSTRAP_S, bootstrap_file));
      } else {
         sge_dstring_sprintf(error_dstring, MSG_UTI_CANNOTBOOTSTRAP_S, 
                             bootstrap_file);
      }
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
   } 
   
   DEXIT;
   return ret;
}

#ifdef WIN32NATIVE
void sge_delete_bootstrap()
{
   GET_SPECIFIC(struct bootstrap_t, bootstrap, bootstrap_init, bootstrap_key, 
                "sge_delete_bootstrap");
	FREE(bootstrap->admin_user);
	FREE(bootstrap->default_domain);
	FREE(bootstrap->ignore_fqdn);
	FREE(bootstrap->spooling_method);
	FREE(bootstrap->spooling_lib);
	FREE(bootstrap->spooling_params);
	FREE(bootstrap->binary_path);
	FREE(bootstrap->qmaster_spool_dir);
}
#endif /* WIN32NATIVE */
