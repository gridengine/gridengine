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
#include <pthread.h>
#include <sys/types.h>

#include "setup_path.h"
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
#include "sge.h"


struct path_state_t {
    char* sge_root;      
    char* cell_root;
    char* bootstrap_file;
    char* conf_file;
    char* sched_conf_file;
    char* act_qmaster_file;
    char* acct_file;
    char* reporting_file;
    char* local_conf_dir;
    char* shadow_masters_file;
};

static pthread_once_t path_once = PTHREAD_ONCE_INIT;
static pthread_key_t path_state_key;

static void path_once_init(void);
static void path_state_destroy(void* theState);
static void path_state_init(struct path_state_t* theState);

 
/****** uti/path/path_mt_init() ************************************************
*  NAME
*     path_mt_init() -- Initialize global SGE path state for multi threading use.
*
*  SYNOPSIS
*     void path_mt_init(void) 
*
*  FUNCTION
*     Set up global SGE path state. This function must be called at least once
*     before any of the path oriented functions can be used. This function is
*     idempotent, i.e. it is safe to call it multiple times.
*
*     Thread local storage for the path state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: path_mt_init() is MT safe 
*
*******************************************************************************/
void path_mt_init(void)
{
   pthread_once(&path_once, path_once_init);
}

/****** uti/path/path_state_get_????() ************************************
*  NAME
*     path_state_get_????() - read access to SGE path state.
*
*  FUNCTION
*     Provide access to thread local storage.
*
******************************************************************************/
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

const char *path_state_get_reporting_file(void)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_get_reporting_file");
   return path_state->reporting_file;
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


/****** uti/path/path_state_set_????() ************************************
*  NAME
*     path_state_get_????() - write access to SGE path state.
*
*  FUNCTION
*     Provide access to thread local storage.
*
******************************************************************************/
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
void path_state_set_reporting_file(const char *path)
{
   GET_SPECIFIC(struct path_state_t, path_state, path_state_init, path_state_key, "path_state_set_reporting_file");
   path_state->reporting_file = sge_strdup(path_state->reporting_file, path);
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

/****** uti/path/sge_setup_paths() *******************************************
*  NAME
*     sge_setup_paths() -- setup global paths 
*
*  SYNOPSIS
*     bool sge_setup_paths(const char *sge_cell, dstring *error_dstring) 
*
*  FUNCTION
*     Set SGE_ROOT and SGE_CELL dependent path components. The spool 
*     directory may later be overridden by global configuration. 
*
*     This function calls 'path_mt_init()' to initialize thread local
*     storage. This function is idempotent, i.e. it is safe to inovke
*     it multiple times.
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
  
   path_mt_init();
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

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, REPORTING_FILE);
   path_state_set_reporting_file(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, LOCAL_CONF_DIR);
   path_state_set_local_conf_dir(sge_dstring_get_string(&bw));

   sge_dstring_sprintf(&bw, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, SHADOW_MASTERS_FILE);
   path_state_set_shadow_masters_file(sge_dstring_get_string(&bw));

   FREE(cell_root);

   DPRINTF(("sge_root            >%s<\n", path_state_get_sge_root()));
   DPRINTF(("cell_root           >%s<\n", path_state_get_cell_root()));
   DPRINTF(("conf_file           >%s<\n", path_state_get_bootstrap_file()));
   DPRINTF(("bootstrap_file      >%s<\n", path_state_get_conf_file()));
   DPRINTF(("act_qmaster_file    >%s<\n", path_state_get_act_qmaster_file()));
   DPRINTF(("acct_file           >%s<\n", path_state_get_acct_file()));
   DPRINTF(("reporting_file      >%s<\n", path_state_get_reporting_file()));
   DPRINTF(("local_conf_dir      >%s<\n", path_state_get_local_conf_dir()));
   DPRINTF(("shadow_masters_file >%s<\n", path_state_get_shadow_masters_file()));
   
   DEXIT;
   return true;
} /* sge_setup_path() */

/****** uit/path/path_once_init() *********************************************
*  NAME
*     path_once_init() -- One-time SGE path state initialization.
*
*  SYNOPSIS
*     static path_once_init(void) 
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
*     MT-NOTE: path_once_init() is MT safe. 
*
*******************************************************************************/
static void path_once_init(void)
{
   pthread_key_create(&path_state_key, &path_state_destroy);
}

/****** uti/path/path_state_destroy() *****************************************
*  NAME
*     path_state_destroy() -- Free thread local storage
*
*  SYNOPSIS
*     static void path_state_destroy(void* theState) 
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
*     MT-NOTE: path_state_destroy() is MT safe.
*
*******************************************************************************/
static void path_state_destroy(void* theState)
{
   FREE(((struct path_state_t*)theState)->sge_root);
   FREE(((struct path_state_t*)theState)->cell_root);
   FREE(((struct path_state_t*)theState)->bootstrap_file);
   FREE(((struct path_state_t*)theState)->conf_file);
   FREE(((struct path_state_t*)theState)->sched_conf_file);
   FREE(((struct path_state_t*)theState)->act_qmaster_file);
   FREE(((struct path_state_t*)theState)->acct_file);
   FREE(((struct path_state_t*)theState)->reporting_file);
   FREE(((struct path_state_t*)theState)->local_conf_dir);
   FREE(((struct path_state_t*)theState)->shadow_masters_file);
   free(theState);
}

/****** uti/path/path_state_init() *********************************************
*  NAME
*     path_state_init() -- Initialize SGE path state.
*
*  SYNOPSIS
*     static void path_state_init(struct path_state_t* theState) 
*
*  FUNCTION
*     Initialize SGE path state.
*
*  INPUTS
*     struct path_state_t* theState - Pointer to SGE path state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: path_state_init() is MT safe. 
*
*******************************************************************************/
static void path_state_init(struct path_state_t* theState)
{
   memset(theState, 0, sizeof(struct path_state_t));
}
