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

#if defined(ALPHA) || defined(DARWIN)
   extern void flockfile(FILE *);
   extern void funlockfile(FILE *);
#endif

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "sge.h"
#include "setup_path.h"
#include "sgermon.h"
#include "sge_log.h"

#include "sge_io.h"
#include "sge_stdio.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_unistd.h"
#include "sge_dstring.h"

#include "sge_answer.h"
#include "sge_object.h"
#include "sge_utility.h"
#include "config.h"

#include "msg_common.h"
#include "msg_spoollib.h"
#include "msg_spoollib_classic.h"
#include "msg_spoollib_flatfile.h"

#include "sge_spooling_flatfile.h"
#include "sge_spooling_flatfile_scanner.h"

static const char *spooling_method = "flatfile";

const char *
get_spooling_method(void)
{
   return spooling_method;
}


/****** spool/flatfile/spool_flatfile_create_context() ********************
*  NAME
*     spool_flatfile_create_context() -- create a flatfile spooling context
*
*  SYNOPSIS
*     lListElem* 
*     spool_flatfile_create_context(int argc, char *argv[])
*
*  FUNCTION
*     Create a spooling context for the flatfile spooling.
* 
*     Two rules are created: One for spooling in the common directory, the
*     other for spooling in the spool directory.
*
*     The following object type descriptions are create:
*        - for SGE_TYPE_CONFIG, referencing the rule for the common directory
*        - for SGE_TYPE_SCHEDD_CONF, also referencing the rule for the common
*          directory
*        - for SGE_TYPE_ALL (default for all object types), referencing the rule
*          for the spool directory
*
*     The function expects to get as argument two absolute paths:
*        1. The path of the common directory
*        2. The path of the spool directory
*     The format of the input string is "<common_dir>;<spool_dir>"
*
*  INPUTS
*     const char *args - arguments to classic spooling
*
*  RESULT
*     lListElem* - on success, the new spooling context, else NULL
*
*  SEE ALSO
*     spool/--Spooling
*     spool/flatfile/--Flatfile-Spooling
*******************************************************************************/
lListElem *
spool_flatfile_create_context(const char *args)
{
   lListElem *context, *rule, *type;
   char *common_dir, *spool_dir;

   DENTER(TOP_LAYER, "spool_flatfile_create_context");

   /* check parameters - both must be set and be absolute paths */
   if (args == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR));
      return NULL;
   }

   common_dir = sge_strtok(args, ";");
   spool_dir  = sge_strtok(NULL, ";");
   
   if (common_dir == NULL || spool_dir == NULL ||
      *common_dir != '/' || *spool_dir != '/') {
      ERROR((SGE_EVENT, MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR));
      return NULL;
   }   

   /* create spooling context */
   context = spool_create_context("flatfile spooling");
   
   /* create rule and type for all objects spooled in the spool dir */
   rule = spool_context_create_rule(context, 
                                    "default rule (spool dir)", 
                                    spool_dir,
                                    spool_flatfile_default_startup_func,
                                    NULL,
                                    spool_flatfile_default_list_func,
                                    spool_flatfile_default_read_func,
                                    spool_flatfile_default_write_func,
                                    spool_flatfile_default_delete_func);
   type = spool_context_create_type(context, SGE_TYPE_ALL);
   spool_type_add_rule(type, rule, true);

   /* create rule and type for all objects spooled in the common dir */
   rule = spool_context_create_rule(context, 
                                    "default rule (common dir)", 
                                    common_dir,
                                    spool_flatfile_common_startup_func,
                                    NULL,
                                    spool_flatfile_default_list_func,
                                    spool_flatfile_default_read_func,
                                    spool_flatfile_default_write_func,
                                    spool_flatfile_default_delete_func);
   type = spool_context_create_type(context, SGE_TYPE_CONFIG);
   spool_type_add_rule(type, rule, true);
   type = spool_context_create_type(context, SGE_TYPE_SCHEDD_CONF);
   spool_type_add_rule(type, rule, true);

   DEXIT;
   return context;
}

/****** spool/flatfile/spool_flatfile_default_startup_func() **************
*  NAME
*     spool_flatfile_default_startup_func() -- setup the spool directory
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_startup_func(const lListElem *rule) 
*
*  FUNCTION
*     Checks the existence of the spool directory.
*     If the current working directory is not yet the spool direcotry,
*     changes the current working directory.
*     If the subdirectories for the different object types do not yet
*     exist, they are created.
*
*  INPUTS
*     const lListElem *rule - the rule containing data necessary for
*                             the startup (e.g. path to the spool directory)
*
*  RESULT
*     bool - true, if the startup succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_flatfile_default_startup_func(const lListElem *rule)
{
   const char *url;

   DENTER(TOP_LAYER, "spool_flatfile_default_startup_func");

   /* check spool directory */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      CRITICAL((SGE_EVENT, MSG_SPOOL_SPOOLDIRDOESNOTEXIST_S, url));
      SGE_EXIT(EXIT_FAILURE);
   }

   /* create spool sub directories */
   sge_mkdir2(url, JOB_DIR,  0755, true);
   sge_mkdir2(url, ZOMBIE_DIR, 0755, true);
   sge_mkdir2(url, QUEUE_DIR,  0755, true);
   sge_mkdir2(url, EXECHOST_DIR, 0755, true);
   sge_mkdir2(url, SUBMITHOST_DIR, 0755, true);
   sge_mkdir2(url, ADMINHOST_DIR, 0755, true);
   sge_mkdir2(url, COMPLEX_DIR, 0755, true);
   sge_mkdir2(url, EXEC_DIR, 0755, true);
   sge_mkdir2(url, PE_DIR, 0755, true);
   sge_mkdir2(url, CKPTOBJ_DIR, 0755, true);
   sge_mkdir2(url, USERSET_DIR, 0755, true);
   sge_mkdir2(url, CAL_DIR, 0755, true);
   sge_mkdir2(url, HGROUP_DIR, 0755, true);
   sge_mkdir2(url, UME_DIR, 0755, true);
   sge_mkdir2(url, USER_DIR, 0755, true);
   sge_mkdir2(url, PROJECT_DIR, 0755, true);

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_common_startup_func() ***************
*  NAME
*     spool_flatfile_common_startup_func() -- setup the common directory
*
*  SYNOPSIS
*     bool
*     spool_flatfile_common_startup_func(const lListElem *rule) 
*
*  FUNCTION
*     Checks the existence of the common directory.
*     If the subdirectory for the local configurtations does not yet exist,
*     it is created.
*
*  INPUTS
*     const lListElem *rule - rule containing data like the path to the common 
*                             directory
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_flatfile_common_startup_func(const lListElem *rule)
{
   const char *url;

   DENTER(TOP_LAYER, "spool_flatfile_common_startup_func");

   /* check common directory */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      CRITICAL((SGE_EVENT, MSG_SPOOL_COMMONDIRDOESNOTEXIST_S, url));
      SGE_EXIT(EXIT_FAILURE);
   }

   /* create directory for local configurations */
   sge_mkdir2(url, LOCAL_CONF_DIR, 0755, true);

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_default_list_func() *****************
*  NAME
*     spool_flatfile_default_list_func() -- read lists through flatfile spooling
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_list_func(const lListElem *type, 
*                                      const lListElem *rule, 
*                                      lList **list, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*     Depending on the object type given, calls the appropriate functions
*     reading the correspondent list of objects using the old spooling
*     functions.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to be used 
*     lList **list                    - target list
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true, on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_read_list()
*******************************************************************************/
bool
spool_flatfile_default_list_func(const lListElem *type, const lListElem *rule,
                                 lList **list, const sge_object_type event_type)
{
/*    static dstring file_name = DSTRING_INIT; */
/*    static dstring dir_name  = DSTRING_INIT; */

   DENTER(TOP_LAYER, "spool_flatfile_default_list_func");
#if 0
   switch(event_type) {
      case SGE_TYPE_ADMINHOST:
         sge_read_adminhost_list_from_disk();
         break;
      case SGE_TYPE_EXECHOST:
         sge_read_exechost_list_from_disk();
         break;
      case SGE_TYPE_SUBMITHOST:
         sge_read_submithost_list_from_disk();
         break;
      case SGE_TYPE_CALENDAR:
         sge_read_cal_list_from_disk();
         break;
      case SGE_TYPE_CKPT:
         sge_read_ckpt_list_from_disk();
         break;
      case SGE_TYPE_COMPLEX:
         read_all_complexes();
         break;
      case SGE_TYPE_CONFIG:
         sge_dstring_sprintf(&file_name, "%s/%s",
                             lGetString(rule, SPR_url), CONF_FILE);
         sge_dstring_sprintf(&dir_name, "%s/%s",
                             lGetString(rule, SPR_url), LOCAL_CONF_DIR);
         read_all_configurations(&Master_Config_List, 
                                 sge_dstring_get_string(&file_name), 
                                 sge_dstring_get_string(&dir_name));
         break;
      case SGE_TYPE_JOB:
         job_list_read_from_disk(&Master_Job_List, "Master_Job_List", 0,
                                 SPOOL_DEFAULT, NULL);
         job_list_read_from_disk(&Master_Zombie_List, "Master_Zombie_List", 0,
                                 SPOOL_HANDLE_AS_ZOMBIE, NULL);
         break;
      case SGE_TYPE_MANAGER:
         read_manop(SGE_MANAGER_LIST);
         break;
      case SGE_TYPE_OPERATOR:
         read_manop(SGE_OPERATOR_LIST);
         break;
      case SGE_TYPE_SHARETREE:
         {
            lListElem *ep;
            char err_str[1024];
            ep = read_sharetree(SHARETREE_FILE, NULL, 1, err_str, 1, NULL);
            if(*list != NULL) {
               *list = lFreeList(*list);
            }
            *list = lCreateList("share tree", STN_Type);
            lAppendElem(*list, ep);
         }
         break;
      case SGE_TYPE_PE:
         sge_read_pe_list_from_disk();
         break;
      case SGE_TYPE_PROJECT:
         sge_read_project_list_from_disk();
         break;
      case SGE_TYPE_QUEUE:
         sge_read_queue_list_from_disk();
         break;
      case SGE_TYPE_SCHEDD_CONF:
         if(*list != NULL) {
            *list = lFreeList(*list);
         }
         sge_dstring_sprintf(&file_name, "%s/%s", 
                             lGetString(rule, SPR_url), SCHED_CONF_FILE);
         *list = read_sched_configuration(lGetString(rule, SPR_url), sge_dstring_get_string(&file_name), 1, NULL);
         break;
      case SGE_TYPE_USER:
         sge_read_user_list_from_disk();
         break;
      case SGE_TYPE_USERSET:
         sge_read_userset_list_from_disk();
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         sge_read_user_mapping_entries_from_disk();
         break;
#endif
      case SGE_TYPE_HGROUP:
         sge_read_host_group_entries_from_disk();
         break;
      default:
         break;
   }
#endif
   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_default_read_func() *****************
*  NAME
*     spool_flatfile_default_read_func() -- read objects through flatfile spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_flatfile_default_read_func(const lListElem *type, 
*                                      const lListElem *rule, 
*                                      const char *key, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*     Reads an individual object by calling the appropriate flatfile spooling 
*     function.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key specifying the object
*     const sge_object_type event_type - object type
*
*  RESULT
*     lListElem* - the object, if it could be read, else NULL
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_flatfile_default_read_func(const lListElem *type, const lListElem *rule,
                                 const char *key, 
                                 const sge_object_type event_type)
{
   lListElem *ep = NULL;

/*    static dstring file_name = DSTRING_INIT; */

   DENTER(TOP_LAYER, "spool_flatfile_default_read_func");
#if 0
   switch(event_type) {
      case SGE_TYPE_ADMINHOST:
         ep = cull_read_in_host(ADMINHOST_DIR, key, CULL_READ_SPOOL, AH_name, NULL, NULL);
         break;
      case SGE_TYPE_EXECHOST:
         ep = cull_read_in_host(EXECHOST_DIR, key, CULL_READ_SPOOL, EH_name, NULL, NULL);
         break;
      case SGE_TYPE_SUBMITHOST:
         ep = cull_read_in_host(SUBMITHOST_DIR, key, CULL_READ_SPOOL, SH_name, NULL, NULL);
         break;
      case SGE_TYPE_CALENDAR:
         ep = cull_read_in_cal(CAL_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_CKPT:
         ep = cull_read_in_ckpt(CKPTOBJ_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_COMPLEX:
         sge_dstring_sprintf(&file_name, "%s/%s", COMPLEX_DIR, key);
         ep = read_cmplx(sge_dstring_get_string(&file_name), key, NULL);
         break;
      case SGE_TYPE_CONFIG:
         sge_dstring_sprintf(&file_name, "%s/%s/%s",
                             lGetString(rule, SPR_url), LOCAL_CONF_DIR, key);
         ep = read_configuration(sge_dstring_get_string(&file_name), 
                                 key, FLG_CONF_SPOOL);
         break;
      case SGE_TYPE_JOB:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGJOB));
         break;
      case SGE_TYPE_MANAGER:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGMANAGER));
         break;
      case SGE_TYPE_OPERATOR:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGOPERATOR));
         break;
      case SGE_TYPE_SHARETREE:
         {
            lListElem *ep;
            char err_str[1024];
            ep = read_sharetree(SHARETREE_FILE, NULL, 1, err_str, 1, NULL);
         }
         break;
      case SGE_TYPE_PE:
         ep = cull_read_in_pe(PE_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_PROJECT:
         ep = cull_read_in_userprj(PROJECT_DIR, key, 1, 0, NULL);
         break;
      case SGE_TYPE_QUEUE:
         ep = cull_read_in_qconf(QUEUE_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         sge_dstring_sprintf(&file_name, "%s/%s",
                             lGetString(rule, SPR_url), SCHED_CONF_FILE);
         ep = cull_read_in_schedd_conf(NULL, sge_dstring_get_string(&file_name),
                                       1, NULL);
         break;
      case SGE_TYPE_USER:
         ep = cull_read_in_userprj(USER_DIR, key, 1, 0, NULL);
         break;
      case SGE_TYPE_USERSET:
         ep = cull_read_in_userset(USERSET_DIR, key, 1, 0, NULL); 
         break;
      case SGE_TYPE_CUSER:
         ep = cull_read_in_ume(UME_DIR, key , 1, 0, NULL); 
         break;
      case SGE_TYPE_HGROUP:
         ep = cull_read_in_host_group(HGROUP_DIR, key, 1, 0, NULL); 
         break;
      default:
         break;
   }
#endif
   DEXIT;
   return ep;
}

/****** spool/flatfile/spool_flatfile_default_write_func() ****************
*  NAME
*     spool_flatfile_default_write_func() -- write objects through flatfile spooling
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_write_func(const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const lListElem *object, 
*                                       const char *key, 
*                                       const sge_object_type event_type) 
*
*  FUNCTION
*     Writes an object through the appropriate flatfile spooling functions.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to spool
*     const char *key                 - unique key
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_flatfile_default_write_func(const lListElem *type, const lListElem *rule, 
                                  const lListElem *object, const char *key, 
                                  const sge_object_type event_type)
{
   lList *answer_list = NULL;
   const char *url = NULL;
   const char *directory = NULL;
   const char *filename = NULL;
   bool ret = true;

   DENTER(TOP_LAYER, "spool_flatfile_default_write_func");

   /* prepare filenames */
   switch(event_type) {
      case SGE_TYPE_ADMINHOST:
         directory = ADMINHOST_DIR;
         filename  = key;
         break;
      case SGE_TYPE_CALENDAR:
         directory = CAL_DIR;
         filename = key;
         break;
      case SGE_TYPE_CKPT:
         directory = CKPTOBJ_DIR;
         filename = key;
         break;
      case SGE_TYPE_COMPLEX:
      case SGE_TYPE_CONFIG:
      case SGE_TYPE_EXECHOST:
      case SGE_TYPE_JOB:
      case SGE_TYPE_MANAGER:
      case SGE_TYPE_OPERATOR:
      case SGE_TYPE_SHARETREE:
      case SGE_TYPE_PE:
      case SGE_TYPE_PROJECT:
      case SGE_TYPE_QUEUE:
      case SGE_TYPE_SCHEDD_CONF:
      case SGE_TYPE_SUBMITHOST:
      case SGE_TYPE_USER:
      case SGE_TYPE_USERSET:
      case SGE_TYPE_HGROUP:
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
#endif
      default:
         WARNING((SGE_EVENT, "writing of "SFQ" not yet implemented\n", 
                  object_type_get_name(event_type)));
         break;
   }

   url = lGetString(rule, SPR_url);

   /* spool, if possible */
   if (url != NULL && directory != NULL && filename != NULL) {
      dstring filepath_buffer = DSTRING_INIT;
      const char *tmpfilepath, *filepath;

      /* first write to a temporary file */
      tmpfilepath = sge_dstring_sprintf(&filepath_buffer, "%s/%s/.%s", url, directory, filename);
     
      /* spool */
      tmpfilepath = spool_flatfile_write_object(&answer_list, object, NULL, 
      &spool_flatfile_instr_config, SP_DEST_SPOOL, SP_FORM_ASCII, tmpfilepath);

      if(tmpfilepath == NULL) {
         /* spooling failed */
         /* JG: TODO: better call a function calling ERROR and WARNING */
         answer_list_print_err_warn(&answer_list, NULL, NULL);
         ret = false;
      } else {
         /* spooling was ok: rename temporary to target file */
         filepath = sge_dstring_sprintf(&filepath_buffer, "%s/%s/%s", url, directory, filename);

         if (rename(tmpfilepath, filepath) == -1) {
            ret = false;
         }
      }

      FREE(tmpfilepath);
      sge_dstring_free(&filepath_buffer);
   }
   
   DEXIT;
   return ret;
}

/****** spool/flatfile/spool_flatfile_default_delete_func() ***************
*  NAME
*     spool_flatfile_default_delete_func() -- delete object in flatfile spooling
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_delete_func(const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const char *key, 
*                                        const sge_object_type event_type) 
*
*  FUNCTION
*     Deletes an object in the flatfile spooling.
*     In most cases, the correspondent spool file is deleted, in some cases
*     (e.g. jobs), a special remove function is called.
*
*  INPUTS
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key 
*     const sge_object_type event_type - object type
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_flatfile_default_delete_func(const lListElem *type, const lListElem *rule,
                                   const char *key, 
                                   const sge_object_type event_type)
{
/*    static dstring dir_name = DSTRING_INIT; */

   DENTER(TOP_LAYER, "spool_flatfile_default_delete_func");
#if 0
   switch(event_type) {
      case SGE_TYPE_ADMINHOST:
         sge_unlink(ADMINHOST_DIR, key);
         break;
      case SGE_TYPE_CALENDAR:
         sge_unlink(CAL_DIR, key);
         break;
      case SGE_TYPE_CKPT:
         sge_unlink(CKPTOBJ_DIR, key);
         break;
      case SGE_TYPE_COMPLEX:
         sge_unlink(COMPLEX_DIR, key);
         break;
      case SGE_TYPE_CONFIG:
         if(sge_hostcmp(key, "global") == 0) {
            ERROR((SGE_EVENT, MSG_SPOOL_GLOBALCONFIGNOTDELETED));
         } else {
            sge_dstring_sprintf(&dir_name, "%s/%s",
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR);
            sge_unlink(sge_dstring_get_string(&dir_name), key);
         }
         break;
      case SGE_TYPE_EXECHOST:
         sge_unlink(EXECHOST_DIR, key);
         break;
      case SGE_TYPE_JOB:
      case SGE_TYPE_JATASK:   
      case SGE_TYPE_PETASK:   
         {
            u_long32 job_id, ja_task_id;
            char *pe_task_id;
            char *dup = strdup(key);
            bool only_job;
            
            job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id, &only_job);
   
            DPRINTF(("spooling job %d.%d %s\n", job_id, ja_task_id, 
                     pe_task_id != NULL ? pe_task_id : "<null>"));
            job_remove_spool_file(job_id, ja_task_id, pe_task_id, SPOOL_DEFAULT);
            free(dup);
         }
         break;
      case SGE_TYPE_MANAGER:
         write_manop(1, SGE_MANAGER_LIST);
         break;
      case SGE_TYPE_OPERATOR:
         write_manop(1, SGE_OPERATOR_LIST);
         break;
      case SGE_TYPE_SHARETREE:
         sge_unlink(NULL, key);
         break;
      case SGE_TYPE_PE:
         sge_unlink(PE_DIR, key);
         break;
      case SGE_TYPE_PROJECT:
         sge_unlink(PROJECT_DIR, key);
         break;
      case SGE_TYPE_QUEUE:
         sge_unlink(QUEUE_DIR, key);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         ERROR((SGE_EVENT, MSG_SPOOL_SCHEDDCONFIGNOTDELETED));
         break;
      case SGE_TYPE_SUBMITHOST:
         sge_unlink(SUBMITHOST_DIR, key);
         break;
      case SGE_TYPE_USER:
         sge_unlink(USER_DIR, key);
         break;
      case SGE_TYPE_USERSET:
         sge_unlink(USERSET_DIR, key);
         break;
      case SGE_TYPE_CUSER:
         sge_unlink(UME_DIR, key);
         break;
      case SGE_TYPTYPEOSTGROUP:
         sge_unlink(HGROUP_DIR, key);
         break;
      default:
         break;
   }
#endif
   DEXIT;
   return true;
}



const spool_flatfile_instr spool_flatfile_instr_config_sublist = 
{
   &spool_config_subinstr,
   false,
   false,
   false,
   false,
   NULL,
   "=",
   ",",
   NULL,
   NULL
};

const spool_flatfile_instr spool_flatfile_instr_config = 
{
   &spool_config_instr,
   true,
   false,
   true,
   false,
   NULL,
   "\n",
   "\n",
   NULL,
   NULL,
   &spool_flatfile_instr_config_sublist
};

const spool_flatfile_instr spool_flatfile_instr_config_list = 
{
   &spool_config_instr,
   false,
   true,
   true,
   true,
   NULL,
   "|",
   "\n",
   NULL,
   NULL,
   &spool_flatfile_instr_config_sublist
};

static bool 
spool_flatfile_write_object_fields(lList **answer_list, const lListElem *object,
                                   dstring *buffer, 
                                   const spool_flatfile_instr *instr,
                                   const spooling_field *fields);

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instr *instr,
                                 const spooling_field *fields);

static FILE *
spool_flatfile_open_file(lList **answer_list,
                         const spool_flatfile_destination destination,
                         const char *filepath_in,
                         const char **filepath_out);

static bool
spool_flatfile_close_file(lList **answer_list, FILE *file, const char *filepath,
                          const spool_flatfile_destination destination);

static const char *
spool_flatfile_write_data(lList **answer_list, const void *data, int data_len, 
                          const spool_flatfile_destination destination, 
                          const char *filepath);

static lListElem *
_spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                            const spool_flatfile_instr *instr, const spooling_field *fields, int fields_out[], int *token,
                            const char *end_token);

static lList *
_spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                          const spool_flatfile_instr *instr, const spooling_field *fields, int fields_out[], int *token,
                          const char *end_token);

/****** spool/flatfile/spool_flatfile_align_object() ********************
*  NAME
*     spool_flatfile_align_object() -- align object output
*
*  SYNOPSIS
*     bool 
*     spool_flatfile_align_object(lList **answer_list,
*                                 spooling_field *fields) 
*
*  FUNCTION
*     Computes the maximum length of the field names stored in <fields>
*     and sets the width value for all fields stored in <fields> to this 
*     maximum.
*
*  INPUTS
*     lList **answer_list     - answer list used to report errors
*     spooling_field *fields  - field description used for alignment
*
*  RESULT
*     bool - true on success, false if an error occured
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_align_list()
*******************************************************************************/
bool 
spool_flatfile_align_object(lList **answer_list, spooling_field *fields)
{
   int i;
   int width = 0;

   DENTER(TOP_LAYER, "spool_flatfile_align_object");

   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm != NoName; i++) {
      width = MAX(width, sge_strlen(fields[i].name));
   }

   for (i = 0; fields[i].nm != NoName; i++) {
      fields[i].width = width;
   }

   DEXIT;
   return true;
}


/****** spool/flatfile/spool_flatfile_align_list() **********************
*  NAME
*     spool_flatfile_align_list() -- align list data for table output
*
*  SYNOPSIS
*     bool 
*     spool_flatfile_align_list(lList **answer_list, const lList *list, 
*                               spooling_field *fields) 
*
*  FUNCTION
*     Computes the maximum width of field name and field contents for 
*     fields described in <fields> and data in <list>.
*     Stores the computed maxima in <fields>.
*
*  INPUTS
*     lList **answer_list    - answer list for error reporting
*     const lList *list      - list with data to align
*     spooling_field *fields - field description
*
*  RESULT
*     bool -  true on success, else false
*
*  NOTES
*     Sublists are not regarded.
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_align_object()
*******************************************************************************/
bool 
spool_flatfile_align_list(lList **answer_list, const lList *list, 
                          spooling_field *fields)
{
   dstring buffer = DSTRING_INIT;
   const lListElem *object;
   int i;

   DENTER(TOP_LAYER, "spool_flatfile_align_list");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(fields);

   for (i = 0; fields[i].nm != NoName; i++) {
      fields[i].width = sge_strlen(lNm2Str(fields[i].nm));
   }

   for_each (object, list) {
      for (i = 0; fields[i].nm != NoName; i++) {
         const char *value = object_get_field_contents(object, answer_list, 
                             &buffer, fields[i].nm);
         fields[i].width = MAX(fields[i].width, sge_strlen(value));
      }
   }

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_write_list() **********************
*  NAME
*     spool_flatfile_write_list() -- write (spool) a complete list
*
*  SYNOPSIS
*     const char* 
*     spool_flatfile_write_list(lList **answer_list, const lList *list, 
*                               const spooling_field *fields_in, 
*                               const spool_flatfile_instr *instr, 
*                               const spool_flatfile_destination destination, 
*                               const spool_flatfile_format format, 
*                               const char *filepath) 
*
*  FUNCTION
*     Writes all data of a list according to the directives given with the
*     parameters.
*
*     Which fields to write can either be passed to the function by setting
*     the parameter <fields_in>, or the function will generate this information
*     using the spooling instructions passed in <instr>.
*
*     <destination> defines the spooling destination, e.g. to stdout, to 
*     a temporary file, to a named file (name is passed in <filepath>.
*  
*     The data format to use (e.g. ASCII format) is passed in <format>.
*
*     On success, the function returns the name of the output file/stream.
*     It is in the responsibility of the caller to free the memory used
*     by the file/stream name.
*
*  INPUTS
*     lList **answer_list                          - for error reporting
*     const lList *list                            - list to write
*     const spooling_field *fields_in              - optional, field description
*     const spool_flatfile_instr *instr            - spooling instructions
*     const spool_flatfile_destination destination - destination
*     const spool_flatfile_format format           - format
*     const char *filepath                         - if destination == 
*                                                    SP_DEST_SPOOL, path to the 
*                                                    spool file
*
*  RESULT
*     const char* - on success the name of the spool file / stream, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_object()
*******************************************************************************/
const char *
spool_flatfile_write_list(lList **answer_list,
                          const lList *list,
                          const spooling_field *fields_in,
                          const spool_flatfile_instr *instr,
                          const spool_flatfile_destination destination,
                          const spool_flatfile_format format,
                          const char *filepath)
{
   dstring char_buffer = DSTRING_INIT;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_list");

   SGE_CHECK_POINTER_NULL(list);
   SGE_CHECK_POINTER_NULL(instr);

   /* if fields are passed, use them, else retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else { 
      my_fields = spool_get_fields_to_spool(answer_list, lGetListDescr(list), 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* message generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      if (format == SP_FORM_ASCII) {
         if (instr->align_names || instr->align_data) {
            if (!spool_flatfile_align_list(answer_list, list, my_fields)) {
               /* message generated in spool_flatfile_align_object */
               my_fields = spool_free_spooling_fields(my_fields);
               DEXIT;
               return NULL;
            }
         }
      }

      fields = my_fields;
   }

   switch (format) {
      case SP_FORM_ASCII:
         if(!spool_flatfile_write_list_fields(answer_list, list, &char_buffer, 
                                              instr, fields)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
         }

         data     = sge_dstring_get_string(&char_buffer);
         data_len = sge_dstring_strlen(&char_buffer);
         break;
      case SP_FORM_XML:
      case SP_FORM_CULL:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_NOTYETIMPLEMENTED_S, 
                                 "XML and CULL spooling");
         break;
   }      

   if (data == NULL || data_len == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_FLATFILE_NODATATOSPOOL);
      sge_dstring_free(&char_buffer);
      if(my_fields != NULL) {
         my_fields = spool_free_spooling_fields(my_fields);
      }
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath);

   /* cleanup */
   sge_dstring_free(&char_buffer);

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return result;
}

/****** spool/flatfile/spool_flatfile_write_object() ********************
*  NAME
*     spool_flatfile_write_object() -- write (spool) an object 
*
*  SYNOPSIS
*     const char * 
*     spool_flatfile_write_object(lList **answer_list, const lListElem *object,
*                                 const spooling_field *fields_in, 
*                                 const spool_flatfile_instr *instr, 
*                                 const spool_flatfile_destination destination,
*                                 const spool_flatfile_format format, 
*                                 const char *filepath) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list                          - ??? 
*     const lListElem *object                      - ??? 
*     const spooling_field *fields_in              - ??? 
*     const spool_flatfile_instr *instr            - ??? 
*     const spool_flatfile_destination destination - ??? 
*     const spool_flatfile_format format           - ??? 
*     const char *filepath                         - ??? 
*
*  RESULT
*     const char * - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     ??? 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
const char * 
spool_flatfile_write_object(lList **answer_list, const lListElem *object,
                            const spooling_field *fields_in,
                            const spool_flatfile_instr *instr,
                            const spool_flatfile_destination destination,
                            const spool_flatfile_format format, 
                            const char *filepath)
{
   dstring         char_buffer = DSTRING_INIT;
   const char *result = NULL;
   const void *data = NULL;
   size_t data_len  = 0;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_object");

   SGE_CHECK_POINTER_NULL(object);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else { 
      my_fields = spool_get_fields_to_spool(answer_list, 
                                            object_get_type(object), 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* message generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      if(format == SP_FORM_ASCII) {
         if (instr->align_names) {
            if (!spool_flatfile_align_object(answer_list, my_fields)) {
               /* message generated in spool_flatfile_align_object */
               my_fields = spool_free_spooling_fields(my_fields);
               DEXIT;
               return NULL;
            }
         }
      }

      fields = my_fields;
   }   

   switch (format) {
      case SP_FORM_ASCII:
         if(!spool_flatfile_write_object_fields(answer_list, object, 
                                                &char_buffer, instr, fields)) {
            /* in case of errors, messages are in answer_list,
             * clear data - we don't want to write erroneous data */
            sge_dstring_clear(&char_buffer); 
         }

         data     = sge_dstring_get_string(&char_buffer);
         data_len = sge_dstring_strlen(&char_buffer);
         break;
      case SP_FORM_XML:
      case SP_FORM_CULL:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, "not yet implemented");
         break;
   }      

   if (data == NULL || data_len == 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_FLATFILE_NODATATOSPOOL);
      sge_dstring_free(&char_buffer);
      if(my_fields != NULL) {
         my_fields = spool_free_spooling_fields(my_fields);
      }
      DEXIT;
      return NULL;
   }

   result = spool_flatfile_write_data(answer_list, data, data_len, destination, 
                                      filepath);

   /* cleanup */
   sge_dstring_free(&char_buffer);

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return result;
}

/****** spool/flatfile/spool_flatfile_open_file() ***********************
*  NAME
*     spool_flatfile_open_file() -- open spooling file or stream
*
*  SYNOPSIS
*     static FILE * 
*     spool_flatfile_open_file(lList **answer_list, 
*                              const spool_flatfile_destination destination, 
*                              const char *filepath_in, 
*                              const char **filepath_out) 
*
*  FUNCTION
*     Opens a file or stream as described by <destination>.
*
*     Streams are locked to handle concurrent access by multiple threads.
*     
*     If <destination> is SP_DEST_TMP, a temporary file is opened.
*
*     If <destination> is SP_DEST_SPOOL, the file specified by 
*     <filepath_in> is opened.
*
*     The name of the file/stream opened is returned in <filepath_out>.
*     It is in the responsibility of the caller to free the memory allocated
*     by <filepath_out>.
*
*     spool_flatfile_close_file shall be used to close a file opened using
*     spool_flatfile_open_file.
*
*  INPUTS
*     lList **answer_list                          - for error reporting 
*     const spool_flatfile_destination destination - destination
*     const char *filepath_in                      - optional filename
*     const char **filepath_out                    - returned filename
*
*  RESULT
*     static FILE * - on success a file handle, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_close_file()
*******************************************************************************/
static FILE * 
spool_flatfile_open_file(lList **answer_list,
                         const spool_flatfile_destination destination,
                         const char *filepath_in,
                         const char **filepath_out)
{
   FILE *file = NULL;
   *filepath_out = NULL;

   switch (destination) {
      case SP_DEST_STDOUT:
         file = stdout;

         /* check stdout file handle */
         if (!sge_check_stdout_stream(file, STDOUT_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stdout>");
            return NULL;
         }

#ifndef AIX42
         flockfile(file);
#endif
         fflush(file);
         *filepath_out = strdup("<stdout>");
         break;
      case SP_DEST_STDERR:
         file = stderr;

         /* check stderr file handle */
         if (!sge_check_stdout_stream(file, STDERR_FILENO)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_STDFILEHANDLECLOSEDORCORRUPTED_S,
                                    "<stderr>");
            return NULL;
         }

#ifndef AIX42
         flockfile(file);
#endif
         fflush(file);
         *filepath_out = strdup("<stderr>");
         break;
      case SP_DEST_TMP:
         {
            char buffer[L_tmpnam];
            
            /* get filename for temporary file, pass buffer to make it
             * thread safe.
             */
            filepath_in = sge_tmpnam(buffer);
            if (filepath_in == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERRORGETTINGTMPNAM_S, 
                                       strerror(errno));
               return NULL;
            }
            
            /* open file */
            file = fopen(filepath_in, "w");
            if (file == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERROROPENINGFILEFORWRITING_SS, 
                                       filepath_in, strerror(errno));
               return NULL;
            }

            *filepath_out = strdup(filepath_in);
         }   
         break;
      case SP_DEST_SPOOL:
         /* check file name */
         if (filepath_in == NULL || *filepath_in == 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_INVALIDFILENAMENULLOREMPTY);
            return NULL;
         }
   
         /* open file */
         file = fopen(filepath_in, "w");
         if (file == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERROROPENINGFILEFORWRITING_SS, 
                                    filepath_in, strerror(errno));
            return NULL;
         }

         *filepath_out = strdup(filepath_in);
         break;
   }

   return file;
}

/****** spool/flatfile/spool_flatfile_close_file() **********************
*  NAME
*     spool_flatfile_close_file() -- close spool file / stream
*
*  SYNOPSIS
*     static bool 
*     spool_flatfile_close_file(lList **answer_list, FILE *file, 
*                               const char *filepath, 
*                               const spool_flatfile_destination destination) 
*
*  FUNCTION
*     Closes the given file or stream.
*     Streams (stdout, strerr) are not really closed, but just unlocked.
*
*  INPUTS
*     lList **answer_list                          - to return errors
*     FILE *file                                   - file handle to close
*     const char *filepath                         - filename
*     const spool_flatfile_destination destination - destination
*
*  RESULT
*     static bool - true on success, else false
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_open_file()
*******************************************************************************/
static bool 
spool_flatfile_close_file(lList **answer_list, FILE *file, const char *filepath,
                          const spool_flatfile_destination destination)
{
   if (destination == SP_DEST_STDOUT || destination == SP_DEST_STDERR) {
      fflush(file);
#ifndef AIX42
      funlockfile(file);
#endif
      return true;
   }

   if (fclose(file) != 0) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORCLOSINGFILE_SS, 
                              filepath != NULL ? filepath : "<null>", 
                              strerror(errno));
      return false;
   }

   return true;
}

static const char *
spool_flatfile_write_data(lList **answer_list, const void *data, int data_len, 
                          const spool_flatfile_destination destination, 
                          const char *filepath)
{
   FILE *file = NULL;
   const char *result = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_write_data");

   SGE_CHECK_POINTER_NULL(data);

   /* open/get filehandle */
   file = spool_flatfile_open_file(answer_list, destination, filepath, &result);
   if (file == NULL) {
      /* message generated in spool_flatfile_open_file */
      DEXIT;
      return NULL;
   }

   /* write data */
   if (fwrite(data, sizeof(char), data_len, file) != data_len) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_ERROR_WRITINGFILE_SS,
                              result, strerror(errno));
      spool_flatfile_close_file(answer_list, file, result, destination);
      FREE(result);
      DEXIT;
      return NULL;
   }

   /* close file */
   if (!spool_flatfile_close_file(answer_list, file, result, destination)) {
      /* message generated in spool_flatfile_close_file */
      FREE(result);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return result;
}

static bool 
spool_flatfile_write_object_fields(lList **answer_list, const lListElem *object,
                                   dstring *buffer, 
                                   const spool_flatfile_instr *instr,
                                   const spooling_field *fields)
{
   int i, first_field;
   dstring field_buffer = DSTRING_INIT;
   const lDescr *descr;

   DENTER(TOP_LAYER, "spool_flatfile_write_object_fields");

   SGE_CHECK_POINTER_FALSE(object);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
 
   descr = lGetElemDescr(object);
 
   /* clear input buffer */
   sge_dstring_clear(buffer);

   /* loop over all fields */
   i = 0;
   first_field = true;

   for (i = 0; fields[i].nm != NoName; i++) {
      const char *value = NULL;
      int pos;

      pos = lGetPosInDescr(descr, fields[i].nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_NMNOTINELEMENT_S,
                                 lNm2Str(fields[i].nm));
         continue;
      }

      /* if not first field, output field_delimiter */
      if (!first_field) {
         sge_dstring_append(buffer, instr->field_delimiter);
      } else {   
         first_field = false;
      }

      /* if show_field_names, output field name */
      if (instr->show_field_names) {
         const char *name;
        
         /* if name is not contained in field list, create it and warn */
         if(fields[i].name == NULL) {
            name = lNm2Str(fields[i].nm);
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_FIELDDESCRIPTIONDOESNOTCONTAINNAME_S,
                                    name);
         } else {
            name = fields[i].name;
         }

         /* respect alignment */
         if (instr->align_names) {
            sge_dstring_sprintf_append(buffer, "%-*s", fields[0].width, name);
         } else {
            sge_dstring_append(buffer, name);
         }

         /* output name-value delimiter */
         if (instr->name_value_delimiter != NULL) {
            sge_dstring_append(buffer, instr->name_value_delimiter);
         } else {
            sge_dstring_append(buffer, " ");
         }
      }

      /* output value */
      if (mt_get_type(descr[pos].mt) == lListT) {
         if(instr->sub_instr == NULL || fields[i].sub_fields == NULL) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_DONTKNOWHOWTOSPOOLSUBLIST_SS,
                                    lNm2Str(fields[i].nm), SGE_FUNC);
            sge_dstring_append(buffer, NONE_STR);
         } else {
            lList *sub_list = lGetList(object, fields[i].nm);      

            if (sub_list == NULL || lGetNumberOfElem(sub_list) == 0) {
               sge_dstring_append(buffer, NONE_STR);
            } else {
               if (!spool_flatfile_write_list_fields(answer_list, sub_list, 
                                                     &field_buffer, 
                                                     instr->sub_instr,
                                                     fields[i].sub_fields)) {
                  /* error handling has been done in spool_flatfile_write_list_fields */
               } else {
                  sge_dstring_append_dstring(buffer, &field_buffer);
               }
            }
         }
      } else {
         value = object_get_field_contents(object, answer_list, &field_buffer,
                                          fields[i].nm);
         if (instr->align_data) {
            sge_dstring_sprintf_append(buffer, "%-*s", fields[i].width, value);
         } else {
            sge_dstring_append(buffer, value);
         }
      }
#if 0
      if (value == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, "blub");
         sge_dstring_free(&field_buffer);
         DEXIT;
         return false;
      }
#endif      
   }

   sge_dstring_free(&field_buffer);

   DEXIT;
   return true;
}

static bool
spool_flatfile_write_list_fields(lList **answer_list, const lList *list, 
                                 dstring *buffer, 
                                 const spool_flatfile_instr *instr,
                                 const spooling_field *fields)
{
   lListElem *ep;
   int first = true;
   dstring record_buffer = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_flatfile_write_list_fields");

   SGE_CHECK_POINTER_FALSE(list);
   SGE_CHECK_POINTER_FALSE(buffer);
   SGE_CHECK_POINTER_FALSE(instr);
   SGE_CHECK_POINTER_FALSE(fields);
  
   /* clear input buffer */
   sge_dstring_clear(buffer);
 
   for_each (ep, list) {
      /* from second record on write record delimiter */
      if (!first) {
         if (instr->record_delimiter != NULL) {
            sge_dstring_append(buffer, instr->record_delimiter);
         }
      } else {
         first = false;
      }

      /* if record_start, output record_start */
      if (instr->record_start != NULL) {
         sge_dstring_append(buffer, instr->record_start);
      }

      if (!spool_flatfile_write_object_fields(answer_list, ep, &record_buffer, 
                                              instr, fields)) {
         /* error message generated in spool_flatfile_write_object_fields */
      } else {
         sge_dstring_append_dstring(buffer, &record_buffer);
      }

      /* if record_end, output record end, else record_delimiter */
      if (instr->record_end != NULL) {
         sge_dstring_append(buffer, instr->record_end);
      }
   }

   sge_dstring_free(&record_buffer);

   DEXIT;
   return true;
}

/****** spool/flatfile/spool_flatfile_read_object() *********************
*  NAME
*     spool_flatfile_read_object() -- read an object from file / stream
*
*  SYNOPSIS
*     lListElem * 
*     spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
*                                const spooling_field *fields_in, 
*                                int fields_out[], 
*                                const spool_flatfile_instr *instr, 
*                                const spool_flatfile_format format, 
*                                FILE *file, const char *filepath) 
*
*  FUNCTION
*     Read an object of type <descr> from the stream <file> or a file described
*     by <filepath>.
*
*     <fields_in> names the fields that can be contained in the input.
*
*     The fields actually read are stored in <fields_out>.
*
*     <format> and <instr> describe the data format to expect.
*
*  INPUTS
*     lList **answer_list                - to report any errors
*     const lDescr *descr                - object type to read
*     const spooling_field *fields_in    - fields that can be contained in input
*     int fields_out[]                   - field actually read
*     const spool_flatfile_instr *instr  - spooling instruction
*     const spool_flatfile_format format - spooling format
*     FILE *file                         - filehandle to read from
*     const char *filepath               - if <file> == NULL, <filepath> is 
*                                          opened
*
*  RESULT
*     lListElem * - on success the read object, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_object()
*     spool/flatfile/spool_flatfile_read_list()
*******************************************************************************/
lListElem * 
spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                           const spooling_field *fields_in,
                           int fields_out[],
                           const spool_flatfile_instr *instr,
                           const spool_flatfile_format format,
                           FILE *file,
                           const char *filepath)
{
   bool file_opened = false;
   int token;
   lListElem *object = NULL;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_read_object");

   SGE_CHECK_POINTER_NULL(descr);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no file handle is passed, try to open file for reading */
   if (file == NULL) {
      SGE_CHECK_POINTER_NULL(filepath);

      file = fopen(filepath, "r");
      if (file == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERROROPENINGFILEFORREADING_SS,
                                 filepath, strerror(errno));
         DEXIT;
         return NULL;
      }

      file_opened = true;
   }

   /* initialize scanner */
   token = spool_scanner_initialize(file);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else {
      my_fields = spool_get_fields_to_spool(answer_list, descr, 
                                         instr->spool_instr);
      if (my_fields == NULL) {
         /* messages generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      fields = my_fields;
   }

   object = _spool_flatfile_read_object(answer_list, descr, instr, 
                                        fields, fields_out, &token, NULL);

   /* if we opened the file, we also have to close it */
   if (file_opened) {
      fclose(file);
   }

/* JG: TODO: spool_scanner_shutdown(); */

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return object;
}

static lListElem *
_spool_flatfile_read_object(lList **answer_list, const lDescr *descr, 
                            const spool_flatfile_instr *instr, 
                            const spooling_field *fields, int fields_out[],
                            int *token, const char *end_token)
{
   int field_index = -1;
   lListElem *object;
   dstring buffer = DSTRING_INIT;
   bool stop = false;

   DENTER(TOP_LAYER, "_spool_flatfile_read_object");

   object = lCreateElem(descr);
   if (object == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_ERRORCREATINGOBJECT);
      DEXIT;
      return NULL;
   }

   while (*token != 0 && !stop) {
      int nm = NoName;
      int pos, type;
      bool field_end  = false;

      /* check for list end condition */
      if ((*token == SPFT_DELIMITER || *token == SPFT_NEWLINE) &&
         sge_strnullcmp(spool_text, end_token) == 0) {
         stop = true;
         continue;
      }

      /* skip newlines */
      while (*token == SPFT_NEWLINE) {
         *token = spool_lex();
      }

      /* check for eof */
      if (*token == 0) {
         continue;
      }

      /* check for record_start */
      if (instr->record_start != NULL) {
         if (*token != SPFT_DELIMITER || 
            sge_strnullcmp(spool_text, instr->record_start) != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD,
                                    instr->record_start,
                                    token == 0 ? "EOF" : spool_text,
                                    spool_line);
            stop = true;
            continue;
         }

         *token = spool_lex();
      }

      /* read field name from file or from field list */
      if (instr->show_field_names) {
         /* read field name from file */
         if (*token != SPFT_WORD) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD,
                                    "<field name>",
                                    token == 0 ? "EOF" : spool_text,
                                    spool_line);
            stop = true;
            continue;
         }
   
         /* search field name in field array */
         for(field_index = 0; fields[field_index].nm != NoName; field_index++) {
            if(sge_strnullcmp(spool_text, fields[field_index].name) == 0) {
               nm = fields[field_index].nm;
               break;
            }
         }

         /* not found -> error */
         if (nm == NoName) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_UNKNOWNATTRIBUTENAME_S, spool_text);
            stop = true;
            continue;
         }
         
         *token = spool_lex();
     
         /* do we have a special delimiter between attrib name and value? */
         if (instr->name_value_delimiter != NULL) {
            if (*token != SPFT_DELIMITER || 
                sge_strnullcmp(spool_text, instr->name_value_delimiter) != 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD,
                                       instr->name_value_delimiter,
                                       token == 0 ? "EOF" : spool_text,
                                       spool_line);
               stop = true;
               continue;
            }

            *token = spool_lex();
         }
      } else {
         /* get next field from field array */   
         nm = fields[++field_index].nm;

         /* last field reached */
         if (nm == NoName) {
            stop = true;
            continue;
         }
      }

      /* check if nm is an attribute of current object type */
      pos = lGetPosInDescr(descr, nm);
      if (pos < 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_ATTRIBUTENOTINOBJECT_S, lNm2Str(nm));
         stop = true;
         continue;
      }

      /* if list of read fields is requested in fields_out, store this info */
      if (fields_out != NULL) {
         add_nm_to_set(fields_out, nm);
      }

      type = mt_get_type(descr[pos].mt);
      
      /* now read the data */
      if (type == lListT) {
         lList *list;
         const lDescr *sub_descr;

         /* check for empty sublist */
         if (*token == SPFT_WORD && 
             sge_strnullcasecmp(spool_text, NONE_STR) == 0) {
            *token = spool_lex();
            /* check for field end - we have to skip it later */
            if (sge_strnullcmp(spool_text, instr->field_delimiter) == 0) {
               field_end = true;
            }
         } else {
            /* parse sublist - do we have necessary info */
            if (instr->sub_instr == NULL || 
               fields[field_index].sub_fields == NULL) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_DONTKNOWHOWTOHANDLELIST_S, 
                                       lNm2Str(nm));
               stop = true;
               continue;
            }

            /* get / check type of sublist */
            sub_descr = object_get_subtype(nm);
            if (sub_descr == NULL)  {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_UNKNOWNOBJECTTYPEFOR_SS, 
                                       lNm2Str(nm), SGE_FUNC);
               stop = true;
               continue;
            }
           
            /* read sublist */
            list = _spool_flatfile_read_list(answer_list, sub_descr, 
                                             instr->sub_instr, 
                                             fields[field_index].sub_fields, 
                                             fields_out, token, 
                                             instr->field_delimiter);
            lSetPosList(object, pos, list);
         }
      } else {
         bool record_end = false;
         
         /* read field data and append until field/record end */
         sge_dstring_clear(&buffer);
         spool_return_whitespace = true;
         while (*token != 0 && !field_end && !record_end) {
            if (*token == SPFT_DELIMITER || *token == SPFT_NEWLINE || 
                *token == SPFT_WHITESPACE) {
               /* check for external end condition */
               if (sge_strnullcmp(spool_text, end_token) == 0) {
                  record_end = true;
                  continue;
               }
               /* check for field end */
               if (sge_strnullcmp(spool_text, instr->field_delimiter) == 0) {
                  field_end = true;
                  continue;
               }
               /* check for record end */
               if (sge_strnullcmp(spool_text, instr->record_delimiter) == 0) {
                  record_end = true;
                  continue;
               }
            }
            
            /* store data */
            sge_dstring_append(&buffer, spool_text);
            *token = spool_lex();
         }
         spool_return_whitespace = false;
         object_set_field_contents(object, answer_list, nm, 
                                   sge_dstring_get_string(&buffer));
      }

      /* skip field end token */
      if (field_end) {
         *token = spool_lex();
      }

      /* check for record_end */
      if (instr->record_end != NULL) {
         if (*token != SPFT_DELIMITER || 
            sge_strnullcmp(spool_text, instr->record_end) != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_ERROR,
                                    MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD,
                                    instr->record_end,
                                    token == 0 ? "EOF" : spool_text,
                                    spool_line);
            stop = true;
            continue;
         }
         *token = spool_lex();
      }

   }

   /* cleanup */
   sge_dstring_free(&buffer);
   DEXIT;
   return object;
}

/****** spool/flatfile/spool_flatfile_read_list() ***********************
*  NAME
*     spool_flatfile_read_list() -- read a list from file / stream
*
*  SYNOPSIS
*     lList * 
*     spool_flatfile_read_list(lList **answer_list, const lDescr *descr,
*                              const spooling_field *fields_in, 
*                              int fields_out[], 
*                              const spool_flatfile_instr *instr, 
*                              const spool_flatfile_format format, 
*                              FILE *file, const char *filepath) 
*
*  FUNCTION
*     Read a list of type <descr> from the stream <file> or a file described
*     by <filepath>.
*
*     <fields_in> names the fields that can be contained in the input.
*
*     The fields actually read are stored in <fields_out>.
*
*     <format> and <instr> describe the data format to expect.
*
*  INPUTS
*     lList **answer_list                - to report any errors
*     const lDescr *descr                - list type
*     const spooling_field *fields_in    - fields that can be contained in input
*     int fields_out[]                   - fields actually read
*     const spool_flatfile_instr *instr  - spooling instructions
*     const spool_flatfile_format format - data format
*     FILE *file                         - file to read or NULL if <filepath> 
*                                          shall be opened
*     const char *filepath               - file to open if <file> == NULL
*
*  RESULT
*     lList * - the list read on success, else NULL
*
*  SEE ALSO
*     spool/flatfile/spool_flatfile_write_list()
*     spool/flatfile/spool_flatfile_read_object()
*******************************************************************************/
lList * 
spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                         const spooling_field *fields_in, int fields_out[],
                         const spool_flatfile_instr *instr,
                         const spool_flatfile_format format,
                         FILE *file,
                         const char *filepath)
{
   bool file_opened = false;
   int token;
   lList *list = NULL;
   const spooling_field *fields = NULL;
   spooling_field *my_fields = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_read_list");

   SGE_CHECK_POINTER_NULL(descr);
   SGE_CHECK_POINTER_NULL(instr);

   /* if no file handle is passed, try to open file for reading */
   if (file == NULL) {
      SGE_CHECK_POINTER_NULL(filepath);

      file = fopen(filepath, "r");
      if (file == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_ERROROPENINGFILEFORREADING_SS,
                                 filepath, strerror(errno));
         DEXIT;
         return NULL;
      }

      file_opened = true;
   }

   /* initialize scanner */
   token = spool_scanner_initialize(file);

   /* if no fields are passed, retrieve them from instructions */
   if (fields_in != NULL) {
      fields = fields_in;
   } else {
      my_fields = spool_get_fields_to_spool(answer_list, descr, 
                                            instr->spool_instr);
      if (my_fields == NULL) {
         /* messages generated in spool_get_fields_to_spool */
         DEXIT;
         return NULL;
      }

      fields = my_fields;
   }

   list = _spool_flatfile_read_list(answer_list, descr, instr, 
                                    fields, fields_out, &token, NULL);

   /* if we opened the file, we also have to close it */
   if (file_opened) {
      fclose(file);
   }

   

/* JG: TODO: spool_scanner_shutdown(); */

   /* if we created our own fields */
   if (my_fields != NULL) {
      my_fields = spool_free_spooling_fields(my_fields);
   }

   DEXIT;
   return list;
}

static lList *
_spool_flatfile_read_list(lList **answer_list, const lDescr *descr, 
                          const spool_flatfile_instr *instr, 
                          const spooling_field *fields, int fields_out[], 
                          int *token, const char *end_token)
{
   bool stop = false;
   bool first_record = true;
   lList *list;
   lListElem *object;

   DENTER(TOP_LAYER, "_spool_flatfile_read_list");

   list = lCreateList("list", descr);
   if (list == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                              ANSWER_QUALITY_ERROR,
                              MSG_ERRORCREATINGLIST);
      DEXIT;
      return NULL;
   }

   /* parse all objects in list */
   while (*token != 0 && !stop) {
      /* check for list end condition */
      if (end_token != NULL &&
         (*token == SPFT_DELIMITER || *token == SPFT_NEWLINE) &&
         sge_strnullcmp(spool_text, end_token) == 0) {
         stop = true;
         continue;
      }
  
      /* for subsequent records check record_delimiter */
      if (!first_record) {
         if (instr->record_delimiter != NULL) {
            if ((*token != SPFT_DELIMITER && *token != SPFT_NEWLINE) || 
                sge_strnullcmp(spool_text, instr->record_delimiter) != 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_ERROR,
                                       MSG_PARSINGOBJECTEXPECTEDBUTGOT_SSD,
                                       instr->record_delimiter,
                                       *token == 0 ? "EOF" : spool_text,
                                       /* spool_line */ *token);
               stop = true;
               continue;
            }
            *token = spool_lex();
         }
      }

      /* read an object */
      object = _spool_flatfile_read_object(answer_list, descr, instr, fields, 
                                           fields_out, token, 
                                           end_token == NULL ? 
                                           instr->record_delimiter : end_token);
      /* store object */
      if (object != NULL) {
         lAppendElem(list, object);
      } else {
         /* if no object was read due to an error, a message has been
          * created in _spool_flatfile_read_object
          */
         stop = true;
         continue;
      }
      
      first_record = false;
   }

   *token = spool_lex();

   /* if no objects could be read, we need no list */
   if (lGetNumberOfElem(list) == 0) {
      list = lFreeList(list);
   }

   return list;
}
