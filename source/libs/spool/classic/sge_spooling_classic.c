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
#include <errno.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_unistd.h"
#include "sge_string.h"
#include "sge_dstring.h"
#include "sge_hostname.h"
#include "sge_profiling.h"

#include "sge.h"
#include "sge_gdi.h"
#include "sge_answer.h"
#include "setup_path.h"
#include "sge_host.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_conf.h"
#include "sge_job.h"
#include "sge_manop.h"
#include "sge_sharetree.h"
#include "sge_pe.h"
#include "sge_schedd_conf.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"

#include "read_list.h"
#include "read_write_host.h"
#include "read_write_cal.h"
#include "read_write_ckpt.h"
#include "read_write_complex.h"
#include "rw_configuration.h"
#include "read_write_job.h"
#include "read_write_manop.h"
#include "read_write_cqueue.h"
#include "read_write_qinstance.h"
#include "read_write_sharetree.h"
#include "read_write_pe.h"
#include "read_write_userprj.h"
#include "read_write_userset.h"
#include "read_write_ume.h"
#include "read_write_host_group.h"
#include "sched_conf.h"
#include "read_write_centry.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/classic/msg_spoollib_classic.h"

#include "spool/classic/sge_spooling_classic.h"

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
*     spool_classic_create_context(lList **answer_list, const char *args)
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
*     lList **answer_list - to return error messages
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
spool_classic_create_context(lList **answer_list, const char *args)
{
   lListElem *context = NULL;

   DENTER(TOP_LAYER, "spool_classic_create_context");

   /* check parameters - both must be set and be absolute paths */
   if (args == NULL) {
      DPRINTF(("spooling arguments are NULL\n"));
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR);
   } else {
      char *common_dir, *spool_dir;

      common_dir = sge_strtok(args, ";");
      spool_dir  = sge_strtok(NULL, ";");
      
      if (common_dir == NULL || spool_dir == NULL ||
         *common_dir != '/' || *spool_dir != '/') {
         DPRINTF(("common_dir: "SFN"\n", common_dir == NULL ? "<null>" : 
                                                              common_dir));
         DPRINTF(("spool_dir: "SFN"\n", spool_dir == NULL ? "<null>" : 
                                                              spool_dir));
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR);
      } else {   
         lListElem *rule, *type;

         /* create spooling context */
         context = spool_create_context(answer_list, "classic spooling");
         
         /* create rule and type for all objects spooled in the spool dir */
         rule = spool_context_create_rule(answer_list, context, 
                                          "default rule (spool dir)", 
                                          spool_dir,
                                          spool_classic_default_startup_func,
                                          NULL,
                                          spool_classic_default_maintenance_func,
                                          NULL,
                                          NULL,
                                          spool_classic_default_list_func,
                                          spool_classic_default_read_func,
                                          spool_classic_default_write_func,
                                          spool_classic_default_delete_func,
                                          NULL,
                                          NULL);
         type = spool_context_create_type(answer_list, context, SGE_TYPE_ALL);
         spool_type_add_rule(answer_list, type, rule, true);

         /* create rule and type for all objects spooled in the common dir */
         rule = spool_context_create_rule(answer_list, context, 
                                          "default rule (common dir)", 
                                          common_dir,
                                          spool_classic_common_startup_func,
                                          NULL,
                                          spool_classic_common_maintenance_func,
                                          NULL,
                                          NULL,
                                          spool_classic_default_list_func,
                                          spool_classic_default_read_func,
                                          spool_classic_default_write_func,
                                          spool_classic_default_delete_func,
                                          NULL,
                                          NULL);
         type = spool_context_create_type(answer_list, context, 
                                          SGE_TYPE_CONFIG);
         spool_type_add_rule(answer_list, type, rule, true);
         type = spool_context_create_type(answer_list, context, 
                                          SGE_TYPE_SCHEDD_CONF);
         spool_type_add_rule(answer_list, type, rule, true);
      }
   }

   DEXIT;
   return context;
}

/****** spool/classic/spool_classic_default_startup_func() **************
*  NAME
*     spool_classic_default_startup_func() -- setup the spool directory
*
*  SYNOPSIS
*     bool
*     spool_classic_default_startup_func(lList **answer_list, 
*                                        const lListElem *rule, bool check) 
*
*  FUNCTION
*     Checks the existence of the spool directory.
*     If the current working directory is not yet the spool direcotry,
*     changes the current working directory.
*     If the subdirectories for the different object types do not yet
*     exist, they are created.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the startup (e.g. path to the spool directory)
*     bool check            - check the spooling database
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
spool_classic_default_startup_func(lList **answer_list, 
                                   const lListElem *rule, bool check)
{
   bool ret = true;

   char cwd_buf[SGE_PATH_MAX];
   char *cwd;

   DENTER(TOP_LAYER, "spool_classic_default_startup_func");

   /* check, if we are in the spool directory */
   cwd = getcwd(cwd_buf, SGE_PATH_MAX);
   if (cwd == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, MSG_ERRORREADINGCWD_S, 
                              strerror(errno));
      ret = false;
   } else {
      const char *url;

      url = lGetString(rule, SPR_url);
      if (strcmp(cwd, url) != 0) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_INFO, 
                                 MSG_SPOOL_CHANGINGTOSPOOLDIRECTORY_S,
                                 url);
         if(sge_chdir(url) != 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORCHANGINGCWD_SS, url, 
                                    strerror(errno));
            ret = false;
         }
      }

      if (ret) {
         /* JG: TODO: if check, we could check for the existence of the
          *           subdirectories.
          */
         /* create spool sub directories */
      }
   }

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_common_startup_func() ***************
*  NAME
*     spool_classic_common_startup_func() -- setup the common directory
*
*  SYNOPSIS
*     bool
*     spool_classic_common_startup_func(lList **answer_list, 
                                        const lListElem *rule, bool check) 
*
*  FUNCTION
*     Checks the existence of the common directory.
*     If the subdirectory for the local configurtations does not yet exist,
*     it is created.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *rule - rule containing data like the path to the common 
*                             directory
*     bool check            - check the spooling database
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
spool_classic_common_startup_func(lList **answer_list, 
                                  const lListElem *rule, bool check)
{
   bool ret = true;
   const char *url;

   DENTER(TOP_LAYER, "spool_classic_common_startup_func");

   /* check common directories */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_COMMONDIRDOESNOTEXIST_S, url);
   } /* JG: TODO: if check, we could check for subdirectories as well */

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_default_maintenance_func() ************
*  NAME
*     spool_classic_default_maintenance_func() -- maintain database
*
*  SYNOPSIS
*     bool 
*     spool_classic_default_maintenance_func(lList **answer_list, 
*                                    lListElem *rule
*                                    const spooling_maintenance_command cmd,
*                                    const char *args);
*
*  FUNCTION
*     Maintains the database:
*        - initialization: create subdirectories of spool directory
*        - ...
*
*  INPUTS
*     lList **answer_list   - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the maintenance (e.g. path to the spool 
*                             directory)
*     const spooling_maintenance_command cmd - the command to execute
*     const char *args      - arguments to the maintenance command
*
*  RESULT
*     bool - true, if the maintenance succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/classic/--Spooling-BerkeleyDB
*     spool/spool_maintain_context()
*******************************************************************************/
bool
spool_classic_default_maintenance_func(lList **answer_list, 
                                    const lListElem *rule, 
                                    const spooling_maintenance_command cmd,
                                    const char *args)
{
   bool ret = true;
   const char *spool_dir;

   DENTER(TOP_LAYER, "spool_classic_default_maintenance_func");

   spool_dir = lGetString(rule, SPR_url);

   switch (cmd) {
      case SPM_init:
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         /* create spool sub directories */
         sge_mkdir(JOB_DIR, 0755, true, false);
         sge_mkdir(ZOMBIE_DIR, 0755, true, false);
         sge_mkdir(CQUEUE_DIR, 0755, true, false);
         sge_mkdir(QINSTANCES_DIR, 0755, true, false);
         sge_mkdir(EXECHOST_DIR, 0755, true, false);
         sge_mkdir(SUBMITHOST_DIR, 0755, true, false);
         sge_mkdir(ADMINHOST_DIR, 0755, true, false);
         sge_mkdir(CENTRY_DIR, 0755, true, false);
         sge_mkdir(EXEC_DIR, 0755, true, false);
         sge_mkdir(PE_DIR, 0755, true, false);
         sge_mkdir(CKPTOBJ_DIR, 0755, true, false);
         sge_mkdir(USERSET_DIR, 0755, true, false);
         sge_mkdir(CAL_DIR, 0755, true, false);
         sge_mkdir(HGROUP_DIR, 0755, true, false);
         sge_mkdir(UME_DIR, 0755, true, false);
         sge_mkdir(USER_DIR, 0755, true, false);
         sge_mkdir(PROJECT_DIR, 0755, true, false);
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 "unknown maintenance command %d\n", cmd);
         ret = false;
         break;
         
   }

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_common_maintenance_func() ************
*  NAME
*     spool_classic_common_maintenance_func() -- maintain database
*
*  SYNOPSIS
*     bool 
*     spool_classic_common_maintenance_func(lList **answer_list, 
*                                    lListElem *rule
*                                    const spooling_maintenance_command cmd,
*                                    const char *args);
*
*  FUNCTION
*     Maintains the database:
*        - initialization: create subdirectories of common directory
*        - ...
*
*  INPUTS
*     lList **answer_list   - to return error messages
*     const lListElem *rule - the rule containing data necessary for
*                             the maintenance (e.g. path to the spool 
*                             directory)
*     const spooling_maintenance_command cmd - the command to execute
*     const char *args      - arguments to the maintenance command
*
*  RESULT
*     bool - true, if the maintenance succeeded, else false
*
*  NOTES
*     This function should not be called directly, it is called by the
*     spooling framework.
*
*  SEE ALSO
*     spool/classic/--Spooling-BerkeleyDB
*     spool/spool_maintain_context()
*******************************************************************************/
bool
spool_classic_common_maintenance_func(lList **answer_list, 
                                    const lListElem *rule, 
                                    const spooling_maintenance_command cmd,
                                    const char *args)
{
   bool ret = true;
   const char *common_dir;

   DENTER(TOP_LAYER, "spool_classic_common_maintenance_func");

   common_dir = lGetString(rule, SPR_url);

   switch (cmd) {
      case SPM_init:
         PROF_START_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         {
            char local_dir_buf[SGE_PATH_MAX];
            dstring local_dir;

            sge_dstring_init(&local_dir, local_dir_buf, SGE_PATH_MAX);

            /* create directory for local configurations */
            sge_dstring_sprintf(&local_dir, "%s/%s", common_dir, LOCAL_CONF_DIR);
            sge_mkdir(sge_dstring_get_string(&local_dir), 0755, true, false);
         }
         PROF_STOP_MEASUREMENT(SGE_PROF_SPOOLINGIO);
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 "unknown maintenance command %d\n", cmd);
         ret = false;
         break;
         
   }

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_default_list_func() *****************
*  NAME
*     spool_classic_default_list_func() -- read lists through classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_list_func(lList **answer_list, 
*                                     const lListElem *type, 
*                                     const lListElem *rule, lList **list, 
*                                     const sge_object_type object_type) 
*
*  FUNCTION
*     Depending on the object type given, calls the appropriate functions
*     reading the correspondent list of objects using the old spooling
*     functions.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to be used 
*     lList **list                    - target list
*     const sge_object_type object_type - object type
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
spool_classic_default_list_func(lList **answer_list, 
                                const lListElem *type, 
                                const lListElem *rule, lList **list, 
                                const sge_object_type object_type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_classic_default_list_func");

   switch (object_type) {
      case SGE_TYPE_ADMINHOST:
         if (sge_read_adminhost_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_EXECHOST:
         if (sge_read_exechost_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_SUBMITHOST:
         if (sge_read_submithost_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_CALENDAR:
         if (sge_read_cal_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_CKPT:
         if (sge_read_ckpt_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_CENTRY:
         if (read_all_centries(CENTRY_DIR) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_CONFIG:
         {
            char filename_buf[SGE_PATH_MAX];
            char dirname_buf[SGE_PATH_MAX];
            dstring file_name;
            dstring dir_name;

            sge_dstring_init(&file_name, filename_buf, SGE_PATH_MAX);
            sge_dstring_init(&dir_name, dirname_buf, SGE_PATH_MAX);
#if 0 /* debugging of reverse resolving of global symbols.
       * seems to be broken on AIX and IRIX - or we simply don't know the 
       * necessary compiler/linker switches?
       */
ERROR((SGE_EVENT, "----> object_type_get_master_list() = %p\n", object_type_get_master_list(SGE_TYPE_CONFIG)));
ERROR((SGE_EVENT, "----> &Master_Config_List           = %p\n", &Master_Config_List));
ERROR((SGE_EVENT, "----> list                          = %p\n", list));
#endif
            sge_dstring_sprintf(&file_name, "%s/%s",
                                lGetString(rule, SPR_url), CONF_FILE);
            sge_dstring_sprintf(&dir_name, "%s/%s",
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR);
            if (read_all_configurations(list,
                                        sge_dstring_get_string(&file_name), 
                                        sge_dstring_get_string(&dir_name)) 
                != 0) {
               ret = false;
            }
         }
         break;
      case SGE_TYPE_JOB:
         if (job_list_read_from_disk(list, "Master_Job_List", 0,
                                     SPOOL_DEFAULT, NULL) != 0) {
            ret = false;
         }
         if (job_list_read_from_disk(list, "Master_Zombie_List",
                                     0, SPOOL_HANDLE_AS_ZOMBIE, NULL) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_MANAGER:
         if (read_manop(SGE_MANAGER_LIST) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_OPERATOR:
         if (read_manop(SGE_OPERATOR_LIST) != 0) {
            ret = false;
         }
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
         if (sge_read_pe_list_from_disk(PE_DIR) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_PROJECT:
         if (sge_read_project_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_CQUEUE:
         if (sge_read_cqueue_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_SCHEDD_CONF:
         {
            char filename_buf[SGE_PATH_MAX];
            dstring file_name;

            sge_dstring_init(&file_name, filename_buf, SGE_PATH_MAX);

            if (*list != NULL) {
               *list = lFreeList(*list);
            }
            sge_dstring_sprintf(&file_name, "%s/%s", 
                                lGetString(rule, SPR_url), SCHED_CONF_FILE);
            *list = read_sched_configuration(lGetString(rule, SPR_url), 
                                             sge_dstring_get_string(&file_name),
                                             1, answer_list);
         }
         break;
      case SGE_TYPE_USER:
         if (sge_read_user_list_from_disk() != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_USERSET:
         if (sge_read_userset_list_from_disk(USERSET_DIR) != 0) {
            ret = false;
         }
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         if (sge_read_user_mapping_entries_from_disk() != 0) {
            ret = false;
         }
         break;
#endif
      case SGE_TYPE_HGROUP:
         if (sge_read_host_group_entries_from_disk() != 0) {
            ret = false;
         }
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         ret = false;
         break;
   }

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_default_read_func() *****************
*  NAME
*     spool_classic_default_read_func() -- read objects through classic spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_classic_default_read_func(lList **answer_list, 
*                                     const lListElem *type, 
*                                     const lListElem *rule, const char *key, 
*                                     const sge_object_type object_type) 
*
*  FUNCTION
*     Reads an individual object by calling the appropriate classic spooling 
*     function.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key specifying the object
*     const sge_object_type object_type - object type
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
spool_classic_default_read_func(lList **answer_list, 
                                const lListElem *type, 
                                const lListElem *rule, const char *key, 
                                const sge_object_type object_type)
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "spool_classic_default_read_func");

   switch (object_type) {
      case SGE_TYPE_ADMINHOST:
         ep = cull_read_in_host(ADMINHOST_DIR, key, CULL_READ_SPOOL, AH_name, 
                                NULL, NULL);
         break;
      case SGE_TYPE_EXECHOST:
         ep = cull_read_in_host(EXECHOST_DIR, key, CULL_READ_SPOOL, EH_name, 
                                NULL, NULL);
         break;
      case SGE_TYPE_SUBMITHOST:
         ep = cull_read_in_host(SUBMITHOST_DIR, key, CULL_READ_SPOOL, SH_name, 
                                NULL, NULL);
         break;
      case SGE_TYPE_CALENDAR:
         ep = cull_read_in_cal(CAL_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_CKPT:
         ep = cull_read_in_ckpt(CKPTOBJ_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_CENTRY:
         ep = cull_read_in_centry(CENTRY_DIR, key, 1, 0,/* NULL,*/ NULL);
         break;
      case SGE_TYPE_CONFIG:
         {
            char file_name_buf[SGE_PATH_MAX];
            dstring file_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_sprintf(&file_name, "%s/%s/%s",
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR, key);
            ep = read_configuration(sge_dstring_get_string(&file_name), 
                                    key, FLG_CONF_SPOOL);
         }
         break;
      case SGE_TYPE_JOB:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_NOTSUPPORTEDREADINGJOB);
         break;
      case SGE_TYPE_MANAGER:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_NOTSUPPORTEDREADINGMANAGER);
         break;
      case SGE_TYPE_OPERATOR:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_NOTSUPPORTEDREADINGOPERATOR);
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
      case SGE_TYPE_CQUEUE:
         ep = cull_read_in_cqueue(CQUEUE_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         {
            char file_name_buf[SGE_PATH_MAX];
            dstring file_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_sprintf(&file_name, "%s/%s",
                                lGetString(rule, SPR_url), SCHED_CONF_FILE);
            ep = cull_read_in_schedd_conf(NULL, 
                                          sge_dstring_get_string(&file_name),
                                          1, NULL);
         }
         break;
      case SGE_TYPE_USER:
         ep = cull_read_in_userprj(USER_DIR, key, 1, 1, NULL);
         break;
      case SGE_TYPE_USERSET:
         ep = cull_read_in_userset(USERSET_DIR, key, 1, 0, NULL); 
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         ep = cull_read_in_ume(UME_DIR, key , 1, 0, NULL, NULL); 
         break;
#endif
      case SGE_TYPE_HGROUP:
         ep = cull_read_in_host_group(HGROUP_DIR, key, 1, 0, NULL, NULL); 
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         break;
   }

   DEXIT;
   return ep;
}

/****** spool/classic/spool_classic_default_write_func() ****************
*  NAME
*     spool_classic_default_write_func() -- write objects using classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_write_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, 
*                                      const lListElem *object, const char *key,
*                                      const sge_object_type object_type) 
*
*  FUNCTION
*     Writes an object through the appropriate classic spooling functions.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const lListElem *object         - object to spool
*     const char *key                 - unique key
*     const sge_object_type object_type - object type
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
spool_classic_default_write_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule, 
                                 const lListElem *object, const char *key, 
                                 const sge_object_type object_type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_classic_default_write_func");
   switch (object_type) {
      case SGE_TYPE_ADMINHOST:
         write_host(1, 2, object, AH_name, NULL);
         break;
      case SGE_TYPE_CALENDAR:
         write_cal(1, 2, object);
         break;
      case SGE_TYPE_CKPT:
         write_ckpt(1, 2, object);
         break;
      case SGE_TYPE_CENTRY:
         write_centry(1, 2, object);
         break;
      case SGE_TYPE_CONFIG:
         {
            char file_name_buf[SGE_PATH_MAX];
            char real_name_buf[SGE_PATH_MAX];
            dstring file_name;
            dstring real_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_init(&real_name, real_name_buf, SGE_PATH_MAX);

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
            if (write_configuration(1, answer_list, 
                                    sge_dstring_get_string(&file_name), object, 
                                    NULL, FLG_CONF_SPOOL) == 0) {
               rename(sge_dstring_get_string(&file_name), 
                      sge_dstring_get_string(&real_name));
            } else {
               ret = false;
            }
         }
         break;
      case SGE_TYPE_EXECHOST:
         write_host(1, 2, object, EH_name, NULL);
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
            if (only_job) {
               if (job_write_common_part((lListElem *)object, 0, SPOOL_DEFAULT)
                   != 0) {
                  ret = false;
               }
            } else {
               lListElem *job;

               if (object_type == SGE_TYPE_JOB) {
                  job = (lListElem *)object;
               } else {
                  /* job_write_spool_file takes a job, even if we only want
                   * to spool a ja_task or pe_task
                   */
                  job = job_list_locate(Master_Job_List, job_id);
               }

               if (job_write_spool_file((lListElem *)job, ja_task_id, 
                                        pe_task_id, SPOOL_DEFAULT) != 0) {
                  ret = false;
               }
            }

            free(dup);
         }
         break;
      case SGE_TYPE_MANAGER:
         if (write_manop(1, SGE_MANAGER_LIST) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_OPERATOR:
         if (write_manop(1, SGE_OPERATOR_LIST) != 0) {
            ret = false;
         }
         break;
      case SGE_TYPE_SHARETREE:
         {
            char file_name_buf[SGE_PATH_MAX];
            dstring file_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);

            sge_dstring_sprintf(&file_name, ".%s", SHARETREE_FILE);
            if(write_sharetree(answer_list, object, 
                               (char *)sge_dstring_get_string(&file_name), NULL,
                               1, 1, 1) == 0) {
               rename(sge_dstring_get_string(&file_name), SHARETREE_FILE);
            } else {
               ret = false;
            }
         }
         break;
      case SGE_TYPE_PE:
         write_pe(1, 2, object);
         break;
      case SGE_TYPE_PROJECT:
         {
            char file_name_buf[SGE_PATH_MAX];
            char real_name_buf[SGE_PATH_MAX];
            dstring file_name;
            dstring real_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_init(&real_name, real_name_buf, SGE_PATH_MAX);

            sge_dstring_sprintf(&file_name, "%s/.%s", PROJECT_DIR, key);
            sge_dstring_sprintf(&real_name, "%s/%s", PROJECT_DIR, key);
            if(write_userprj(answer_list, object, 
                             sge_dstring_get_string(&file_name), NULL, 1, 0) 
               == 0) {
               rename(sge_dstring_get_string(&file_name), 
                      sge_dstring_get_string(&real_name));
            } else {
               ret = false;
            }
         }
         break;
      case SGE_TYPE_CQUEUE:
         write_cqueue(1, 2, object);
         break;
      case SGE_TYPE_QINSTANCE:
         write_qinstance(1, 2, object);
         break;
      case SGE_TYPE_SCHEDD_CONF:
         write_sched_configuration(1, 2, lGetString(rule, SPR_url), object);
         break;
      case SGE_TYPE_SUBMITHOST:
         write_host(1, 2, object, SH_name, NULL);
         break;
      case SGE_TYPE_USER:
         {
            char file_name_buf[SGE_PATH_MAX];
            char real_name_buf[SGE_PATH_MAX];
            dstring file_name;
            dstring real_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_init(&real_name, real_name_buf, SGE_PATH_MAX);

            sge_dstring_sprintf(&file_name, "%s/.%s", USER_DIR, key);
            sge_dstring_sprintf(&real_name, "%s/%s", USER_DIR, key);
            if(write_userprj(answer_list, object, 
                             sge_dstring_get_string(&file_name), NULL, 1, 1) 
               == 0) {
               rename(sge_dstring_get_string(&file_name), 
                      sge_dstring_get_string(&real_name));
            } else {
               ret = false;
            }
         }
         break;
      case SGE_TYPE_USERSET:
         {
            char file_name_buf[SGE_PATH_MAX];
            char real_name_buf[SGE_PATH_MAX];
            dstring file_name;
            dstring real_name;

            sge_dstring_init(&file_name, file_name_buf, SGE_PATH_MAX);
            sge_dstring_init(&real_name, real_name_buf, SGE_PATH_MAX);

            sge_dstring_sprintf(&file_name, "%s/.%s", USERSET_DIR, key);
            sge_dstring_sprintf(&real_name, "%s/%s", USERSET_DIR, key);
            if(write_userset(answer_list, object, 
                             sge_dstring_get_string(&file_name), NULL, 1) 
               == 0) {
               rename(sge_dstring_get_string(&file_name), 
                      sge_dstring_get_string(&real_name));
            } else {
               ret = false;
            }
         }
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         write_ume(1, 2, object);
         break;
#endif
      case SGE_TYPE_HGROUP:
         write_host_group(1, 2, object);
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         ret = false;
         break;
   }

   DEXIT;
   return ret;
}

/****** spool/classic/spool_classic_default_delete_func() ***************
*  NAME
*     spool_classic_default_delete_func() -- delete object in classic spooling
*
*  SYNOPSIS
*     bool
*     spool_classic_default_delete_func(lList **answer_list, 
*                                       const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const char *key, 
*                                       const sge_object_type object_type) 
*
*  FUNCTION
*     Deletes an object in the classic spooling.
*     In most cases, the correspondent spool file is deleted, in some cases
*     (e.g. jobs), a special remove function is called.
*
*  INPUTS
*     lList **answer_list - to return error messages
*     const lListElem *type           - object type description
*     const lListElem *rule           - rule to use
*     const char *key                 - unique key 
*     const sge_object_type object_type - object type
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
spool_classic_default_delete_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const char *key, 
                                  const sge_object_type object_type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_classic_default_delete_func");

   switch (object_type) {
      case SGE_TYPE_ADMINHOST:
         ret = sge_unlink(ADMINHOST_DIR, key) == 0;
         break;
      case SGE_TYPE_CALENDAR:
         ret = sge_unlink(CAL_DIR, key) == 0;
         break;
      case SGE_TYPE_CKPT:
         ret = sge_unlink(CKPTOBJ_DIR, key) == 0;
         break;
      case SGE_TYPE_CENTRY:
         ret = sge_unlink(CENTRY_DIR, key) == 0;
         break;
      case SGE_TYPE_CONFIG:
         if (sge_hostcmp(key, "global") == 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_SPOOL_GLOBALCONFIGNOTDELETED);
            ret = false;
         } else {
            char dir_name_buf[SGE_PATH_MAX];
            dstring dir_name;
            sge_dstring_init(&dir_name, dir_name_buf, SGE_PATH_MAX);
            sge_dstring_sprintf(&dir_name, "%s/%s",
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR);
            ret = sge_unlink(sge_dstring_get_string(&dir_name), key) == 0;
         }
         break;
      case SGE_TYPE_EXECHOST:
         ret = sge_unlink(EXECHOST_DIR, key) == 0;
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
            ret = job_remove_spool_file(job_id, ja_task_id, pe_task_id, 
                                        SPOOL_DEFAULT) == 0;
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
         ret = sge_unlink(NULL, SHARETREE_FILE) == 0;
         break;
      case SGE_TYPE_PE:
         ret = sge_unlink(PE_DIR, key) == 0;
         break;
      case SGE_TYPE_PROJECT:
         ret = sge_unlink(PROJECT_DIR, key) == 0;
         break;
      case SGE_TYPE_CQUEUE:
         ret = sge_unlink(CQUEUE_DIR, key) == 0;
         break;
      case SGE_TYPE_QINSTANCE:
         ret = sge_unlink(QINSTANCES_DIR, key) == 0;
         break;
      case SGE_TYPE_SCHEDD_CONF:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_SCHEDDCONFIGNOTDELETED);
         ret = false;
         break;
      case SGE_TYPE_SUBMITHOST:
         ret = sge_unlink(SUBMITHOST_DIR, key) == 0;
         break;
      case SGE_TYPE_USER:
         ret = sge_unlink(USER_DIR, key) == 0;
         break;
      case SGE_TYPE_USERSET:
         ret = sge_unlink(USERSET_DIR, key) == 0;
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         ret = sge_unlink(UME_DIR, key) == 0;
         break;
#endif
      case SGE_TYPE_HGROUP:
         ret = sge_unlink(HGROUP_DIR, key) == 0;
         break;
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         ret = false;
         break;
   }

   DEXIT;
   return ret;
}


