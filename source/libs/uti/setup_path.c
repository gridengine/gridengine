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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#if defined(SGE_MT)
#include <pthread.h>
#endif

#include "sgermon.h"
#include "basis_types.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_stdlib.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_dstring.h"

#include "msg_utilib.h"
#include "msg_common.h"

#include "setup_path.h"

struct path_state_t {
    char       *sge_root;      
    char       *cell_root;
    char       *bootstrap_file;
    char       *conf_file;
    char       *sched_conf_file;
    char       *act_qmaster_file;
    char       *acct_file;
    char       *stat_file;
    char       *local_conf_dir;
    char       *shadow_masters_file;
    char       *product_mode_file;
};

#if defined(SGE_MT)
static pthread_key_t   path_state_key;
#else
static struct path_state_t path_state_opaque = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
struct path_state_t *path_state = &path_state_opaque;
#endif

#if defined(SGE_MT)
static void path_state_init(struct path_state_t* state) {
   memset(state, 0, sizeof(struct path_state_t));
}

static void path_state_destroy(void* state) {
   FREE(((struct path_state_t*)state)->sge_root);
   FREE(((struct path_state_t*)state)->cell_root);
   FREE(((struct path_state_t*)state)->bootstrap_file);
   FREE(((struct path_state_t*)state)->conf_file);
   FREE(((struct path_state_t*)state)->sched_conf_file);
   FREE(((struct path_state_t*)state)->act_qmaster_file);
   FREE(((struct path_state_t*)state)->acct_file);
   FREE(((struct path_state_t*)state)->stat_file);
   FREE(((struct path_state_t*)state)->local_conf_dir);
   FREE(((struct path_state_t*)state)->shadow_masters_file);
   FREE(((struct path_state_t*)state)->product_mode_file);
   free(state);
}
 
void path_init_mt(void) {
   pthread_key_create(&path_state_key, &path_state_destroy);
} 
#endif

const char *path_state_get_sge_root(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_sge_root");
   return path_state->sge_root;
}
const char *path_state_get_cell_root(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_cell_root");
   return path_state->cell_root;
}
const char *path_state_get_bootstrap_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_bootstrap_file");
   return path_state->bootstrap_file;
}
const char *path_state_get_conf_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_conf_file");
   return path_state->conf_file;
}
const char *path_state_get_sched_conf_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_sched_conf_file");
   return path_state->sched_conf_file;
}
const char *path_state_get_act_qmaster_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_act_qmaster_file");
   return path_state->act_qmaster_file;
}
const char *path_state_get_acct_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_acct_file");
   return path_state->acct_file;
}
const char *path_state_get_stat_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_stat_file");
   return path_state->stat_file;
}
const char *path_state_get_local_conf_dir(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_local_conf_dir");
   return path_state->local_conf_dir;
}
const char *path_state_get_shadow_masters_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_shadow_masters_file");
   return path_state->shadow_masters_file;
}
const char *path_state_get_product_mode_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_product_mode_file");
   return path_state->product_mode_file;
}




void path_state_set_sge_root(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_sge_root");
   path_state->sge_root = sge_strdup(path_state->sge_root, path);
}
void path_state_set_cell_root(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_cell_root");
   path_state->cell_root = sge_strdup(path_state->cell_root, path);
}
void path_state_set_bootstrap_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_conf_file");
   path_state->bootstrap_file = sge_strdup(path_state->conf_file, path);
}
void path_state_set_conf_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_conf_file");
   path_state->conf_file = sge_strdup(path_state->conf_file, path);
}
void path_state_set_sched_conf_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_sched_conf_file");
   path_state->sched_conf_file = sge_strdup(path_state->sched_conf_file, path);
}
void path_state_set_act_qmaster_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_act_qmaster_file");
   path_state->act_qmaster_file = sge_strdup(path_state->act_qmaster_file, path);
}
void path_state_set_acct_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_acct_file");
   path_state->acct_file = sge_strdup(path_state->acct_file, path);
}
void path_state_set_stat_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_stat_file");
   path_state->stat_file = sge_strdup(path_state->stat_file, path);
}
void path_state_set_local_conf_dir(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_local_conf_dir");
   path_state->local_conf_dir = sge_strdup(path_state->local_conf_dir, path);
}
void path_state_set_shadow_masters_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_shadow_masters_file");
   path_state->shadow_masters_file = sge_strdup(path_state->shadow_masters_file, path);
}
void path_state_set_product_mode_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_product_mode_file");
   path_state->product_mode_file = sge_strdup(path_state->product_mode_file, path);
}

/****** setup_path/sge_setup_paths() *******************************************
*  NAME
*     sge_setup_paths() -- setup global pathes 
*
*  SYNOPSIS
*     bool sge_setup_paths(const char *sge_cell, dstring *error_dstring) 
*
*  FUNCTION
*     Set SGE_ROOT and SGE_CELL dependent path components. The spool 
*     directory may later be overridden by global configuration.
*     This routine may be called as often as is is necessary.
*
*  INPUTS
*     const char *sge_cell - the SGE cell to be used
* 
*  OUTPUT
*     dstring *error_dstring - A string buffer to return error messages.
*                              Also used by caller to indicate 
*                              if setup function should exit on errror or not.
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: sge_setup_paths() is MT safe
*******************************************************************************/
bool sge_setup_paths(const char *sge_cell, dstring *error_dstring)
{
   char *cell_root;
   const char *sge_root;
   char *common_dir;
   SGE_STRUCT_STAT sbuf;
   char buffer[2*1024];
   dstring bw;
   
   DENTER(TOP_LAYER, "sge_setup_paths");
  
   sge_dstring_init(&bw, buffer, sizeof(buffer)); 

   if (!(sge_root = sge_get_root_dir(error_dstring == NULL ? 1 : 0, 
                                     buffer, sizeof(buffer)-1, 1))) {
      /* in exit-on-error case program already exited */
      if (error_dstring != NULL) {
         sge_dstring_sprintf(error_dstring, buffer);
      }
      DEXIT;
      return false;
   }

   if (SGE_STAT(sge_root, &sbuf)) {
      if (error_dstring == NULL) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_SGEROOTNOTFOUND_S, sge_root));
         SGE_EXIT(1);
      } else {
         sge_dstring_sprintf(error_dstring, MSG_SGETEXT_SGEROOTNOTFOUND_S, 
                             sge_root);
         DEXIT;
         return false;
      }   
   }
   
   if (!S_ISDIR(sbuf.st_mode)) {
      if (error_dstring == NULL) { 
         CRITICAL((SGE_EVENT, MSG_UTI_SGEROOTNOTADIRECTORY_S , sge_root));
         SGE_EXIT(1);
      } else {   
         sge_dstring_sprintf(error_dstring, MSG_UTI_SGEROOTNOTADIRECTORY_S , 
                             sge_root);
         DEXIT;
         return false;
      }
   } 

   cell_root = sge_malloc(strlen(sge_root) + strlen(sge_cell) + 2);
   if (!cell_root) {
      if (error_dstring == NULL) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_NOMEM));
         SGE_EXIT(1);
      } else {
         sge_dstring_sprintf(error_dstring, MSG_SGETEXT_NOMEM);
         DEXIT;
         return false;
      }
   }

   sprintf(cell_root, "%s"PATH_SEPARATOR"%s", sge_root, sge_cell);   

   if (SGE_STAT(cell_root, &sbuf)) {
      if (uti_state_get_mewho() != QMASTER) {
         if (error_dstring == NULL) {
            CRITICAL((SGE_EVENT, MSG_SGETEXT_NOSGECELL_S, cell_root));
            SGE_EXIT(1);
         } else {
            sge_dstring_sprintf(error_dstring, MSG_SGETEXT_NOSGECELL_S, 
                                cell_root);
            DEXIT;
            return false;
         }
      }   
   }

   common_dir = malloc(strlen(cell_root) + strlen(COMMON_DIR) + 2);
   sprintf(common_dir, "%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR);
   if (SGE_STAT(common_dir, &sbuf)) {
      if (uti_state_get_mewho() != QMASTER) {  
         if (error_dstring == NULL) {
            CRITICAL((SGE_EVENT, MSG_UTI_DIRECTORYNOTEXIST_S , common_dir));
            SGE_EXIT(1);
         } else {
            sge_dstring_sprintf(error_dstring, MSG_UTI_DIRECTORYNOTEXIST_S , 
                                common_dir);
            DEXIT;
            return false;
         }
      }   
   }       

   FREE(common_dir);

   path_state_set_sge_root(sge_root);
   path_state_set_cell_root(cell_root);
  
   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, BOOTSTRAP_FILE);
   path_state_set_bootstrap_file(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, CONF_FILE);
   path_state_set_conf_file(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, SCHED_CONF_FILE);
   path_state_set_sched_conf_file(sge_dstring_get_string(&bw));
   
   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, ACT_QMASTER_FILE);
   path_state_set_act_qmaster_file(sge_dstring_get_string(&bw));
   
   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, ACCT_FILE);
   path_state_set_acct_file(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, STAT_FILE);
   path_state_set_stat_file(sge_dstring_get_string(&bw));
   
   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, LOCAL_CONF_DIR);
   path_state_set_local_conf_dir(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, SHADOW_MASTERS_FILE);
   path_state_set_shadow_masters_file(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, PRODUCT_MODE_FILE);
   path_state_set_product_mode_file(sge_dstring_get_string(&bw));
   FREE(cell_root);

   DPRINTF(("sge_root            >%s<\n", path_state_get_sge_root()));
   DPRINTF(("cell_root           >%s<\n", path_state_get_cell_root()));
   DPRINTF(("conf_file           >%s<\n", path_state_get_bootstrap_file()));
   DPRINTF(("bootstrap_file      >%s<\n", path_state_get_conf_file()));
   DPRINTF(("act_qmaster_file    >%s<\n", path_state_get_act_qmaster_file()));
   DPRINTF(("acct_file           >%s<\n", path_state_get_acct_file()));
   DPRINTF(("stat_file           >%s<\n", path_state_get_stat_file()));
   DPRINTF(("local_conf_dir      >%s<\n", path_state_get_local_conf_dir()));
   DPRINTF(("shadow_masters_file >%s<\n", path_state_get_shadow_masters_file()));
   DPRINTF(("product_mode_file   >%s<\n", path_state_get_product_mode_file()));
   
   DEXIT;
   return true;
}

#ifdef WIN32NATIVE
void sge_delete_paths()
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_sge_root");
	FREE(path_state->sge_root);
	FREE(path_state->cell_root);
	FREE(path_state->bootstrap_file);
	FREE(path_state->conf_file);
	FREE(path_state->sched_conf_file);
	FREE(path_state->act_qmaster_file);
	FREE(path_state->acct_file);
	FREE(path_state->stat_file);
	FREE(path_state->local_conf_dir);
	FREE(path_state->shadow_masters_file);
	FREE(path_state->product_mode_file);
}
#endif /* WIN32NATIVE */
