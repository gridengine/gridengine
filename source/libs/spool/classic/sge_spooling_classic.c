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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_unistd.h"
#include "sge_string.h"
#include "sge_dstring.h"
#include "sge_hostname.h"

#include "sge.h"
#include "sge_gdi.h"
#include "setup_path.h"
#include "sge_host.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_complex.h"
#include "sge_conf.h"
#include "sge_job.h"
#include "sge_manop.h"
#include "sge_sharetree.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_schedd_conf.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_hostgroup.h"

#ifndef __SGE_NO_USERMAPPING__
#include "sge_usermap.h"
#endif

#include "read_list.h"
#include "read_write_host.h"
#include "read_write_cal.h"
#include "read_write_ckpt.h"
#include "read_write_complex.h"
#include "rw_configuration.h"
#include "read_write_job.h"
#include "read_write_manop.h"
#include "read_write_queue.h"
#include "read_write_sharetree.h"
#include "read_write_pe.h"
#include "read_write_userprj.h"
#include "read_write_userset.h"
#include "sched_conf.h"

#ifndef __SGE_NO_USERMAPPING__
#include "read_write_ume.h"
#endif
#include "read_write_host_group.h"

#include "msg_spoollib_classic.h"

#include "sge_spooling_classic.h"

/****** spool/classic/--Classic-Spooling ***************************************
*
*  NAME
*     Classic Spooling -- the old Grid Engine spooling code
*
*  FUNCTION
*     The classic spooling instantiation of the spooling framework
*     provides access to the old Grid Engine spooling code through
*     the spooling framework.
*
*     It uses two spool directories:
*        - the common directory (usually $SGE_ROOT/$SGE_CELL/common) containing
*          the global and local configurations and the scheduler configuration
*        - the spool directory (usually $SGE_ROOT/$SGE_CELL/spool/qmaster or
*          a local spool directory configured in the global configuration) 
*          contains all other data.
*     Other spooling clients than qmaster can use any directories for spooling
*     data through the classic spooling context.
*
*     The spool directory should be the current working directory, if it is not,
*     the startup function of the classic spooling changes the current working
*     directory to the spool directory.
*
*  SEE ALSO
*     spool/--Spooling
*     spool/classic/spool_classic_create_context()
****************************************************************************
*/

static const char *spooling_method = "classic";

const char *get_spooling_method(void)
{
   return spooling_method;
}

/****** spool/classic/spool_classic_create_context() ********************
*  NAME
*     spool_classic_create_context() -- create a classic spooling context
*
*  SYNOPSIS
*     lListElem* 
*     spool_classic_create_context(const char *args)
*
*  FUNCTION
*     Create a spooling context for the classic spooling.
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
*     spool/classic/--Classic-Spooling
*******************************************************************************/
lListElem *
spool_classic_create_context(const char *args)
{
   lListElem *context, *rule, *type;
   char *common_dir, *spool_dir;

   DENTER(TOP_LAYER, "spool_classic_create_context");

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
   context = spool_create_context("classic spooling");
   
   /* create rule and type for all objects spooled in the spool dir */
   rule = spool_context_create_rule(context, 
                                    "default rule (spool dir)", 
                                    spool_dir,
                                    spool_classic_default_startup_func,
                                    NULL,
                                    spool_classic_default_list_func,
                                    spool_classic_default_read_func,
                                    spool_classic_default_write_func,
                                    spool_classic_default_delete_func);
   type = spool_context_create_type(context, SGE_TYPE_ALL);
   spool_type_add_rule(type, rule, true);

   /* create rule and type for all objects spooled in the common dir */
   rule = spool_context_create_rule(context, 
                                    "default rule (common dir)", 
                                    common_dir,
                                    spool_classic_common_startup_func,
                                    NULL,
                                    spool_classic_default_list_func,
                                    spool_classic_default_read_func,
                                    spool_classic_default_write_func,
                                    spool_classic_default_delete_func);
   type = spool_context_create_type(context, SGE_TYPE_CONFIG);
   spool_type_add_rule(type, rule, true);
   type = spool_context_create_type(context, SGE_TYPE_SCHEDD_CONF);
   spool_type_add_rule(type, rule, true);

   DEXIT;
   return context;
}

/****** spool/classic/spool_classic_default_startup_func() **************
*  NAME
*     spool_classic_default_startup_func() -- setup the spool directory
*
*  SYNOPSIS
*     bool
*     spool_classic_default_startup_func(const lListElem *rule) 
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
*     spool/classic/--Classic-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_classic_default_startup_func(const lListElem *rule)
{
   char *cwd;
   const char *url;

   DENTER(TOP_LAYER, "spool_classic_default_startup_func");

   /* check, if we are in the spool directory */
   cwd = getcwd(NULL, SGE_PATH_MAX);
   url = lGetString(rule, SPR_url);
   if (strcmp(cwd, url) != 0) {
      WARNING((SGE_EVENT, MSG_SPOOL_STARTEDINWRONGDIRECTORY_SS, url, cwd));
      sge_chdir_exit(url, TRUE);
   }
   free(cwd);

   /* create spool sub directories */
   sge_mkdir(JOB_DIR,  0755, true);
   sge_mkdir(ZOMBIE_DIR, 0755, true);
   sge_mkdir(QUEUE_DIR,  0755, true);
   sge_mkdir(EXECHOST_DIR, 0755, true);
   sge_mkdir(SUBMITHOST_DIR, 0755, true);
   sge_mkdir(ADMINHOST_DIR, 0755, true);
   sge_mkdir(COMPLEX_DIR, 0755, true);
   sge_mkdir(EXEC_DIR, 0755, true);
   sge_mkdir(PE_DIR, 0755, true);
   sge_mkdir(CKPTOBJ_DIR, 0755, true);
   sge_mkdir(USERSET_DIR, 0755, true);
   sge_mkdir(CAL_DIR, 0755, true);

   sge_mkdir(HOSTGROUP_DIR, 0755, true);
#ifndef __SGE_NO_USERMAPPING__
   sge_mkdir(UME_DIR, 0755, true);
#endif

   sge_mkdir(USER_DIR, 0755, true);
   sge_mkdir(PROJECT_DIR, 0755, true);

   DEXIT;
   return true;
}

/****** spool/classic/spool_classic_common_startup_func() ***************
*  NAME
*     spool_classic_common_startup_func() -- setup the common directory
*
*  SYNOPSIS
*     bool
*     spool_classic_common_startup_func(const lListElem *rule) 
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
*     spool/classic/--Classic-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_classic_common_startup_func(const lListElem *rule)
{
   const char *url;
   dstring local_dir = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_common_startup_func");

   /* check common directories */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      CRITICAL((SGE_EVENT, MSG_SPOOL_COMMONDIRDOESNOTEXIST_S, url));
      SGE_EXIT(EXIT_FAILURE);
   }

   /* create directory for local configurations */
   sge_dstring_sprintf(&local_dir, "%s/%s", url, LOCAL_CONF_DIR);
   sge_mkdir(sge_dstring_get_string(&local_dir), 0755, true);
   sge_dstring_free(&local_dir);

   DEXIT;
   return true;
}

/****** spool/classic/spool_classic_default_list_func() *****************
*  NAME
*     spool_classic_default_list_func() -- read lists through classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_list_func(const lListElem *type, 
*                                     const lListElem *rule, 
*                                     lList **list, 
*                                     const sge_object_type event_type) 
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
*     spool/classic/--Classic-Spooling
*     spool/spool_read_list()
*******************************************************************************/
bool 
spool_classic_default_list_func(const lListElem *type, const lListElem *rule,
                                lList **list, const sge_object_type event_type)
{
   static dstring file_name = DSTRING_INIT;
   static dstring dir_name  = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_list_func");

   switch (event_type) {
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
      
            /* remove old sharetree */
            if (*list != NULL) {
               *list = lFreeList(*list);
            }

            /* read sharetree from disk */
            ep = read_sharetree(SHARETREE_FILE, NULL, 1, err_str, 1, NULL);
            if (ep != NULL) {
               *list = lCreateList("share tree", STN_Type);
               lAppendElem(*list, ep);
            }
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
         if (*list != NULL) {
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
      case SGE_TYPE_USERMAPPING:
         sge_read_user_mapping_entries_from_disk();
         break;
#endif
      case SGE_TYPE_HOSTGROUP:
         sge_read_host_group_entries_from_disk();
         break;
      default:
         break;
   }

   DEXIT;
   return true;
}

/****** spool/classic/spool_classic_default_read_func() *****************
*  NAME
*     spool_classic_default_read_func() -- read objects through classic spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_classic_default_read_func(const lListElem *type, 
*                                     const lListElem *rule, 
*                                     const char *key, 
*                                     const sge_object_type event_type) 
*
*  FUNCTION
*     Reads an individual object by calling the appropriate classic spooling 
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
*     spool/classic/--Classic-Spooling
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_classic_default_read_func(const lListElem *type, const lListElem *rule,
                                const char *key, 
                                const sge_object_type event_type)
{
   lListElem *ep = NULL;

   static dstring file_name = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_read_func");

   switch (event_type) {
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
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_USERMAPPING:
         ep = cull_read_in_ume(UME_DIR, key , 1, 0, NULL); 
         break;
#endif
      case SGE_TYPE_HOSTGROUP:
         ep = cull_read_in_host_group(HOSTGROUP_DIR, key, 1, 0, NULL); 
         break;
      default:
         break;
   }

   DEXIT;
   return ep;
}

/****** spool/classic/spool_classic_default_write_func() ****************
*  NAME
*     spool_classic_default_write_func() -- write objects through classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_write_func(const lListElem *type, 
*                                      const lListElem *rule, 
*                                      const lListElem *object, 
*                                      const char *key, 
*                                      const sge_object_type event_type) 
*
*  FUNCTION
*     Writes an object through the appropriate classic spooling functions.
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
*     spool/classic/--Classic-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_classic_default_write_func(const lListElem *type, const lListElem *rule, 
                                 const lListElem *object, const char *key, 
                                 const sge_object_type event_type)
{
   static dstring file_name = DSTRING_INIT;
   static dstring real_name = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_write_func");
   switch (event_type) {
      case SGE_TYPE_ADMINHOST:
         write_host(1, 2, object, AH_name, NULL);
         break;
      case SGE_TYPE_CALENDAR:
         write_cal(1, 2, object);
         break;
      case SGE_TYPE_CKPT:
         write_ckpt(1, 2, object);
         break;
      case SGE_TYPE_COMPLEX:
         sge_dstring_sprintf(&file_name, "%s/.%s", COMPLEX_DIR, key);
         sge_dstring_sprintf(&real_name, "%s/%s", COMPLEX_DIR, key);
         if(write_cmplx(1, sge_dstring_get_string(&file_name), lGetList(object, CX_entries), NULL, NULL) == 0) {
            rename(sge_dstring_get_string(&file_name), 
                   sge_dstring_get_string(&real_name));
         }
         break;
      case SGE_TYPE_CONFIG:
         if (sge_hostcmp(lGetHost(object, CONF_hname), "global") == 0) {
            sge_dstring_sprintf(&file_name, "%s/.%s",
                                lGetString(rule, SPR_url), CONF_FILE);
            sge_dstring_sprintf(&real_name, "%s/%s",
                                lGetString(rule, SPR_url), CONF_FILE);
         } else {
            sge_dstring_sprintf(&file_name, "%s/%s/.%s", 
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR,
                                key);
            sge_dstring_sprintf(&real_name, "%s/%s/%s", 
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR,
                                key);
         }
         if (write_configuration(1, NULL, sge_dstring_get_string(&file_name), object, NULL, FLG_CONF_SPOOL) == 0) {
            rename(sge_dstring_get_string(&file_name), 
                   sge_dstring_get_string(&real_name));
         }
         break;
      case SGE_TYPE_EXECHOST:
         write_host(1, 2, object, EH_name, NULL);
         break;
      case SGE_TYPE_JOB:
         {
            u_long32 job_id, ja_task_id;
            char *pe_task_id;
            char *dup = strdup(key);
            bool only_job;
            
            job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id, &only_job);
   
            DPRINTF(("spooling job %d.%d %s\n", job_id, ja_task_id, 
                     pe_task_id != NULL ? pe_task_id : "<null>"));
            if (only_job) {
               job_write_common_part((lListElem *)object, 0, SPOOL_DEFAULT);
            } else {
               job_write_spool_file((lListElem *)object, ja_task_id, pe_task_id, SPOOL_DEFAULT);
            }

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
         write_sharetree(NULL, object, SHARETREE_FILE, NULL, 1, 1, 1);
         break;
      case SGE_TYPE_PE:
         write_pe(1, 2, object);
         break;
      case SGE_TYPE_PROJECT:
         sge_dstring_sprintf(&file_name, "%s/%s", PROJECT_DIR, key);
         write_userprj(NULL, object, sge_dstring_get_string(&file_name), NULL, 1, 0);
         break;
      case SGE_TYPE_QUEUE:
         sge_dstring_sprintf(&file_name, "%s", key);
         cull_write_qconf(1, 0, QUEUE_DIR, sge_dstring_get_string(&file_name), NULL, object);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         write_sched_configuration(1, 2, lGetString(rule, SPR_url), object);
         break;
      case SGE_TYPE_SUBMITHOST:
         write_host(1, 2, object, SH_name, NULL);
         break;
      case SGE_TYPE_USER:
         sge_dstring_sprintf(&file_name, "%s/%s", USER_DIR, key);
         write_userprj(NULL, object, sge_dstring_get_string(&file_name), NULL, 1, 0);
         break;
      case SGE_TYPE_USERSET:
         sge_dstring_sprintf(&file_name, "%s/.%s", USERSET_DIR, key);
         sge_dstring_sprintf(&real_name, "%s/%s", USERSET_DIR, key);
         if(write_userset(NULL, object, sge_dstring_get_string(&file_name), NULL, 1) == 0) {
            rename(sge_dstring_get_string(&file_name), 
                   sge_dstring_get_string(&real_name));
         }
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_USERMAPPING:
         write_ume(1, 2, object);
         break;
#endif
      case SGE_TYPE_HOSTGROUP:
         write_host_group(1, 2, object);
         break;
      default:
         break;
   }

   DEXIT;
   return true;
}

/****** spool/classic/spool_classic_default_delete_func() ***************
*  NAME
*     spool_classic_default_delete_func() -- delete object in classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_delete_func(const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const char *key, 
*                                       const sge_object_type event_type) 
*
*  FUNCTION
*     Deletes an object in the classic spooling.
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
*     spool/classic/--Classic-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool 
spool_classic_default_delete_func(const lListElem *type, const lListElem *rule, 
                                  const char *key, 
                                  const sge_object_type event_type)
{
   static dstring dir_name = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_delete_func");

   switch (event_type) {
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
         if (sge_hostcmp(key, "global") == 0) {
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
         sge_unlink(NULL, SHARETREE_FILE);
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
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_USERMAPPING:
         sge_unlink(UME_DIR, key);
         break;
#endif
      case SGE_TYPE_HOSTGROUP:
         sge_unlink(HOSTGROUP_DIR, key);
         break;
      default:
         break;
   }

   DEXIT;
   return true;
}



