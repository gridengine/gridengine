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

/* system */
#include <errno.h>

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
#include "sge_hostname.h"
#include "spool/sge_dirent.h"

#include "sge_answer.h"
#include "sge_object.h"
#include "sge_utility.h"
#include "config.h"

#include "commlib.h"

#include "sge_conf.h"
#include "sge_str.h"
#include "sge_host.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_usage.h"
#include "sge_sharetree.h"
#include "sge_userset.h"

#include "sort_hosts.h"
#include "sge_complex_schedd.h"
#include "sge_select_queue.c"

/* includes for old job spooling */
#include "spool/classic/read_write_job.h"

#include "msg_common.h"
#include "spool/msg_spoollib.h"
#include "spool/classic/msg_spoollib_classic.h"
#include "spool/flatfile/msg_spoollib_flatfile.h"

#include "spool/flatfile/sge_flatfile.h"
#include "spool/flatfile/sge_spooling_flatfile.h"

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
*     spool_flatfile_create_context(lList **answer_list, const char *args)
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
*     lList **answer_list - to return error messages
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
spool_flatfile_create_context(lList **answer_list, const char *args)
{
   lListElem *context = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_create_context");

   /* check parameters - both must be set and be absolute paths */
   if (args == NULL) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR);
   } else {
      char *common_dir, *spool_dir;

      common_dir = sge_strtok(args, ";");
      spool_dir  = sge_strtok(NULL, ";");
      
      if (common_dir == NULL || spool_dir == NULL ||
         *common_dir != '/' || *spool_dir != '/') {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR);
      } else {   
         sge_object_type i;
         flatfile_info *field_info;
         lListElem *rule, *type;

         /* create info which fields to spool once */
         field_info = malloc(sizeof(flatfile_info) * SGE_TYPE_ALL);
         for (i = SGE_TYPE_ADMINHOST; i < SGE_TYPE_ALL; i++) {
            switch (i) {
               /* pseudo types without spooling action */
               case SGE_TYPE_GLOBAL_CONFIG:
               case SGE_TYPE_JOB_SCHEDD_INFO:
               case SGE_TYPE_SCHEDD_MONITOR:
               case SGE_TYPE_SHUTDOWN:
               case SGE_TYPE_QMASTER_GOES_DOWN:
                  field_info[i].fields = NULL;
                  field_info[i].instr  = NULL;
                  break;
               /* standard case of spooling */
               case SGE_TYPE_ADMINHOST:
               case SGE_TYPE_CALENDAR:
               case SGE_TYPE_CKPT:
               case SGE_TYPE_MANAGER:
               case SGE_TYPE_OPERATOR:
               case SGE_TYPE_CQUEUE:
               case SGE_TYPE_QINSTANCE:
               case SGE_TYPE_SUBMITHOST:
               case SGE_TYPE_USERSET:
               case SGE_TYPE_HGROUP:
#ifndef __SGE_NO_USERMAPPING__ 
               case SGE_TYPE_CUSER:
#endif
               case SGE_TYPE_SCHEDD_CONF:
#if 0
                  field_info[i].fields = spool_get_fields_to_spool(answer_list, 
                                              object_type_get_descr(i), 
                                              &spool_config_instr);
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  break;
#endif
               case SGE_TYPE_EXECHOST:
#if 0
                  field_info[i].fields = spool_get_fields_to_spool(answer_list, 
                                              object_type_get_descr(i), 
                                              &spool_config_instr);
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  {
                     int j;
                     spooling_field *fields = field_info[i].fields;

                     for (j = 0; fields[j].nm != NoName; j++) {
                        if (fields[j].nm == EH_reschedule_unknown_list) {
                           fields[j].clientdata = &spool_flatfile_instr_ru_type;
                           break;
                        }
                     }
                  }
                  break;
#endif
               case SGE_TYPE_PE:
#if 0
                  field_info[i].fields = spool_get_fields_to_spool(answer_list, 
                                              object_type_get_descr(i), 
                                              &spool_config_instr);
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  /* inst_sge writes a pe with name field called pe_name - we 
                   * have to use this name as field name for PE_name 
                   */
                  field_info[i].fields[0].name = "pe_name"; 
                  break;
#endif
               case SGE_TYPE_CONFIG:
#if 0
               /* special case config spooling */
                  field_info[i].fields = spool_get_fields_to_spool(answer_list, 
                                              object_type_get_descr(i), 
                                              &spool_conf_instr);
                  field_info[i].instr  = &spool_flatfile_instr_conf;
                  break;
#endif
               case SGE_TYPE_PROJECT:
#if 0
                  field_info[i].fields = PROJECT_fields;
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  break;
#endif
               case SGE_TYPE_USER:
#if 0
                  field_info[i].fields = USER_fields;
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  break;
#endif
               case SGE_TYPE_SHARETREE:
#if 0
                  field_info[i].fields = STN_fields;
                  field_info[i].instr  = &spool_flatfile_instr_config;
                  break;
#endif
               case SGE_TYPE_JOB:
               case SGE_TYPE_JATASK:
               case SGE_TYPE_PETASK:
               default:
                  break;
            }
         }

         /* create spooling context */
         context = spool_create_context(answer_list, "flatfile spooling");
         
         /* create rule and type for all objects spooled in the spool dir */
         rule = spool_context_create_rule(answer_list, context, 
                                          "default rule (spool dir)", 
                                          spool_dir,
                                          spool_flatfile_default_startup_func,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          spool_flatfile_default_list_func,
                                          spool_flatfile_default_read_func,
                                          spool_flatfile_default_write_func,
                                          spool_flatfile_default_delete_func,
                                          spool_default_validate_func,
                                          spool_default_validate_list_func);
         lSetRef(rule, SPR_clientdata, field_info);
         type = spool_context_create_type(answer_list, context, SGE_TYPE_ALL);
         spool_type_add_rule(answer_list, type, rule, true);

         /* create rule and type for all objects spooled in the common dir */
         rule = spool_context_create_rule(answer_list, context, 
                                          "default rule (common dir)", 
                                          common_dir,
                                          spool_flatfile_common_startup_func,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          spool_flatfile_default_list_func,
                                          spool_flatfile_default_read_func,
                                          spool_flatfile_default_write_func,
                                          spool_flatfile_default_delete_func,
                                          spool_default_validate_func,
                                          spool_default_validate_list_func);
         lSetRef(rule, SPR_clientdata, field_info);
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

/****** spool/flatfile/spool_flatfile_default_startup_func() **************
*  NAME
*     spool_flatfile_default_startup_func() -- setup the spool directory
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_startup_func(lList **answer_list, 
*                                         const lListElem *rule, bool check) 
*
*  FUNCTION
*     Checks the existence of the spool directory.
*     If the current working directory is not yet the spool direcotry,
*     changes the current working directory.
*     If the subdirectories for the different object types do not yet
*     exist, they are created.
*
*  INPUTS
*     lList **answer_list   - to return error messages
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_flatfile_default_startup_func(lList **answer_list, 
                                    const lListElem *rule, bool check)
{
   bool ret = true;
   const char *url;

   DENTER(TOP_LAYER, "spool_flatfile_default_startup_func");

   /* check spool directory */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_SPOOLDIRDOESNOTEXIST_S, url);
      ret = false;
   } else {
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
      sge_mkdir2(url, MAN_DIR, 0755, true);
      sge_mkdir2(url, OP_DIR, 0755, true);
   }

   DEXIT;
   return ret;
}

/****** spool/flatfile/spool_flatfile_common_startup_func() ***************
*  NAME
*     spool_flatfile_common_startup_func() -- setup the common directory
*
*  SYNOPSIS
*     bool
*     spool_flatfile_common_startup_func(lList **answer_list, 
*                                        const lListElem *rule, bool check) 
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_startup_context()
*******************************************************************************/
bool
spool_flatfile_common_startup_func(lList **answer_list, 
                                   const lListElem *rule, bool check)
{
   bool ret = true;
   const char *url;

   DENTER(TOP_LAYER, "spool_flatfile_common_startup_func");

   /* check common directory */
   url = lGetString(rule, SPR_url);
   if (!sge_is_directory(url)) {
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_SPOOL_COMMONDIRDOESNOTEXIST_S, url);
      ret = false;
   } else {
      /* create directory for local configurations */
      sge_mkdir2(url, LOCAL_CONF_DIR, 0755, true);
   }

   DEXIT;
   return ret;
}

static bool
read_validate_object(lList **answer_list, 
                   const lListElem *type, const lListElem *rule, 
                   const char *key, int key_nm, 
                   sge_object_type object_type, lList **master_list)
{
   bool ret = true;
   lListElem *ep;

   DENTER(TOP_LAYER, "read_validate_object");

   DPRINTF(("reading "SFN" "SFQ"\n", object_type_get_name(object_type), key));

   ep = spool_flatfile_default_read_func(answer_list, type, rule, key, 
                                         object_type);
   if (ep == NULL) {
      ret = false;
   } else {
      spooling_validate_func validate_func;

      /* set key from filename */
      if (key_nm != NoName) {
         object_parse_field_from_string(ep, NULL, key_nm, key);
      }

      /* validate object */
      validate_func = (spooling_validate_func)
                    lGetRef(rule, SPR_validate_func);
      if (validate_func != NULL) {
         if (!validate_func(answer_list, type, rule, ep, object_type)) {
            lFreeElem(ep);
            ep = NULL;
            ret = false;
         }
      }
   }

   /* object read correctly and validate succeeded */
   if (ep != NULL) {
      lAppendElem(*master_list, ep);
   }

   DEXIT;
   return ret;
}

/****** spool/flatfile/spool_flatfile_default_list_func() *****************
*  NAME
*     spool_flatfile_default_list_func() -- read lists through flatfile spooling
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_list_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, 
*                                      lList **list, 
*                                      const sge_object_type object_type) 
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_read_list()
*******************************************************************************/
bool
spool_flatfile_default_list_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule,
                                 lList **list, 
                                 const sge_object_type object_type)
{
   const lDescr *descr;

   const char *filename  = NULL;
   const char *directory = NULL;
   const char *url = NULL;
   int key_nm = NoName;

   bool ret = true;

   DENTER(TOP_LAYER, "spool_flatfile_default_list_func");

   if (!list){
      answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                     ANSWER_QUALITY_WARNING, 
                     "Cannot read in configuration because target list is missing\n"); 
      ret = false;
   } 
   else {
      url = lGetString(rule, SPR_url);
      descr = object_type_get_descr(object_type);

      if (*list!= NULL && descr != NULL) {
         *list = lCreateList("master list", descr);
      }

      switch(object_type) {
         case SGE_TYPE_ADMINHOST:
            directory = ADMINHOST_DIR;
            break;
         case SGE_TYPE_CALENDAR:
            directory = CAL_DIR;
            break;
         case SGE_TYPE_CKPT:
            directory = CKPTOBJ_DIR;
            break;
         case SGE_TYPE_CONFIG:
            key_nm    = CONF_hname;
            filename  = "global";
            directory = LOCAL_CONF_DIR;
            break;
         case SGE_TYPE_EXECHOST:
            directory = EXECHOST_DIR;
            break;
         case SGE_TYPE_MANAGER:
            directory = MAN_DIR;
            break;
         case SGE_TYPE_OPERATOR:
            directory = OP_DIR;
            break;
         case SGE_TYPE_PE:
            directory = PE_DIR;
            break;
         case SGE_TYPE_QINSTANCE:
            directory = QINSTANCES_DIR;
            /* JG: TODO: we'll have to quicksort the queue list, see
             * function queue_list_add_queue
             */
            break;
         case SGE_TYPE_CQUEUE:
            directory = CQUEUE_DIR;
            /* JG: TODO: we'll have to quicksort the queue list, see
             * function cqueue_list_add_cqueue
             */
            break;
         case SGE_TYPE_SUBMITHOST:
            directory = SUBMITHOST_DIR;
            break;
         case SGE_TYPE_USERSET:
            directory = USERSET_DIR;
            break;
         case SGE_TYPE_HGROUP:
            directory = HGROUP_DIR;
            break;
#ifndef __SGE_NO_USERMAPPING__
         case SGE_TYPE_CUSER:
            directory = UME_DIR;
            break;
#endif
         case SGE_TYPE_PROJECT:
            directory = PROJECT_DIR;
            break;
         case SGE_TYPE_USER:
            directory = USER_DIR;
            break;
         case SGE_TYPE_SHARETREE:
            filename = SHARETREE_FILE;
            break;
         case SGE_TYPE_SCHEDD_CONF:
            filename = SCHED_CONF_FILE;
            break;
         case SGE_TYPE_JOB:
            job_list_read_from_disk(&Master_Job_List, "Master_Job_List", 0,
                                    SPOOL_DEFAULT, NULL);
            job_list_read_from_disk(&Master_Zombie_List, "Master_Zombie_List", 0,
                                    SPOOL_HANDLE_AS_ZOMBIE, NULL);
            break;
         default:
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_WARNING, 
                                    MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                    object_type_get_name(object_type));
            ret = false;
            break;
      }

      /* if all necessary data has been initialized */
      if (url != NULL && list != NULL && descr != NULL) {
         /* if we have a directory (= multiple files) to parse */ 
         if (directory != NULL) { 
            lList *direntries;
            lListElem *direntry;
            char abs_dir_buf[SGE_PATH_MAX];
            dstring abs_dir_dstring;
            const char *abs_dir;

            sge_dstring_init(&abs_dir_dstring, abs_dir_buf, SGE_PATH_MAX);
            abs_dir = sge_dstring_sprintf(&abs_dir_dstring, "%s/%s", url, 
                                          directory);

            direntries = sge_get_dirents(abs_dir);

            for_each (direntry, direntries) {
               const char *key = lGetString(direntry, ST_name);

               if (key[0] != '.') {
                  ret = read_validate_object(answer_list, type, rule, key, key_nm, 
                                           object_type, list);
               }
            }
         } 
         
         /* single file to parse (SHARETREE, global config, schedd config */
         if (filename != NULL) {
            ret = read_validate_object(answer_list, type, rule, filename, key_nm, 
                                     object_type, list);
         }
      }

   #ifdef DEBUG_FLATFILE
      if (list != NULL && *list != NULL) {
         lWriteListTo(*list, stderr);
      }
   #endif

      /* validate complete list */
      if (ret) {
         spooling_validate_list_func validate_list = 
            (spooling_validate_list_func)lGetRef(rule, SPR_validate_list_func);
         
         ret = validate_list(answer_list, type, rule, object_type);
      }
   }
   DEXIT;
   return ret;
}

/****** spool/flatfile/spool_flatfile_default_read_func() *****************
*  NAME
*     spool_flatfile_default_read_func() -- read objects using flatfile spooling
*
*  SYNOPSIS
*     lListElem* 
*     spool_flatfile_default_read_func(lList **answer_list, 
*                                      const lListElem *type, 
*                                      const lListElem *rule, 
*                                      const char *key, 
*                                      const sge_object_type object_type) 
*
*  FUNCTION
*     Reads an individual object by calling the appropriate flatfile spooling 
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_read_object()
*******************************************************************************/
lListElem *
spool_flatfile_default_read_func(lList **answer_list, 
                                 const lListElem *type, 
                                 const lListElem *rule,
                                 const char *key, 
                                 const sge_object_type object_type)
{
   const char *url = NULL;
   const char *directory = NULL;
   const char *filename = NULL;
   const lDescr *descr = NULL;
   flatfile_info *rule_clientdata;
   flatfile_info *field_info;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "spool_flatfile_default_read_func");

   rule_clientdata = lGetRef(rule, SPR_clientdata);
   field_info = &(rule_clientdata[object_type]);
   url = lGetString(rule, SPR_url);
   descr = object_type_get_descr(object_type);

   /* prepare filenames */
   switch(object_type) {
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
      case SGE_TYPE_CONFIG:
         if (sge_hostcmp(key, "global") == 0) {
            directory = ".";
            filename  = CONF_FILE;
         } else {
            directory = LOCAL_CONF_DIR;
            filename  = key;
         }
         break;
      case SGE_TYPE_EXECHOST:
         directory = EXECHOST_DIR;
         filename  = key;
         break;
      case SGE_TYPE_MANAGER:
         directory = MAN_DIR;
         filename = key;
         break;
      case SGE_TYPE_OPERATOR:
         directory = OP_DIR;
         filename = key;
         break;
      case SGE_TYPE_PE:
         directory = PE_DIR;
         filename = key;
         break;
      case SGE_TYPE_CQUEUE:
         directory = CQUEUE_DIR;
         filename  = key;
         break;
      case SGE_TYPE_QINSTANCE:
         directory = QINSTANCES_DIR;
         filename  = key;
         break;
      case SGE_TYPE_SUBMITHOST:
         directory = SUBMITHOST_DIR;
         filename  = key;
         break;
      case SGE_TYPE_USERSET:
         directory = USERSET_DIR;
         filename  = key;
         break;
      case SGE_TYPE_HGROUP:
         directory = HGROUP_DIR;
         filename  = key;
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         directory = UME_DIR;
         filename  = key;
         break;
#endif
      case SGE_TYPE_PROJECT:
         directory = PROJECT_DIR;
         filename  = key;
         break;
      case SGE_TYPE_USER:
         directory = USER_DIR;
         filename  = key;
         break;
      case SGE_TYPE_SHARETREE:
         directory = ".";
         filename  = SHARETREE_FILE;
         break;
      case SGE_TYPE_SCHEDD_CONF:
         directory = ".";
         filename  = SCHED_CONF_FILE;
         break;
      case SGE_TYPE_JOB:
      default:
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_WARNING, 
                                 MSG_SPOOL_SPOOLINGOFXNOTSUPPORTED_S, 
                                 object_type_get_name(object_type));
         break;
   }

   /* spool, if possible */
   if (url != NULL && directory != NULL && filename != NULL && descr != NULL) {
      char filepath_buf[SGE_PATH_MAX];
      dstring filepath_dstring;
      const char *filepath;

      sge_dstring_init(&filepath_dstring, filepath_buf, SGE_PATH_MAX);

      filepath = sge_dstring_sprintf(&filepath_dstring, "%s/%s/%s", 
                                     url, directory, filename);
     
      /* spool */
      ep = spool_flatfile_read_object(answer_list, descr, 
                                      field_info->fields, NULL, 
                                      field_info->instr, SP_FORM_ASCII, 
                                      NULL, filepath);
   }

   DEXIT;
   return ep;
}

/****** spool/flatfile/spool_flatfile_default_write_func() ****************
*  NAME
*     spool_flatfile_default_write_func() -- write object with flatfile spooling
*
*  SYNOPSIS
*     bool
*     spool_flatfile_default_write_func(lList **answer_list, 
*                                       const lListElem *type, 
*                                       const lListElem *rule, 
*                                       const lListElem *object, 
*                                       const char *key, 
*                                       const sge_object_type object_type) 
*
*  FUNCTION
*     Writes an object through the appropriate flatfile spooling functions.
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_flatfile_default_write_func(lList **answer_list, 
                                  const lListElem *type, 
                                  const lListElem *rule, 
                                  const lListElem *object, 
                                  const char *key, 
                                  const sge_object_type object_type)
{
   const char *url = NULL;
   const char *directory = NULL;
   const char *filename = NULL;
   char tmpfilebuf[SGE_PATH_MAX];
   const char *tmpfilepath = NULL, *filepath = NULL;
   flatfile_info *rule_clientdata;
   flatfile_info *field_info;
   bool ret = true;

   DENTER(TOP_LAYER, "spool_flatfile_default_write_func");

   rule_clientdata = lGetRef(rule, SPR_clientdata);
   field_info = &(rule_clientdata[object_type]);
   url = lGetString(rule, SPR_url);

   /* prepare filenames */
   switch(object_type) {
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
      case SGE_TYPE_CONFIG:
         if (sge_hostcmp(key, "global") == 0) {
            directory = ".";
            filename  = CONF_FILE;
         } else {
            directory = LOCAL_CONF_DIR;
            filename  = key;
         }
         break;
      case SGE_TYPE_EXECHOST:
         directory = EXECHOST_DIR;
         filename  = key;
         break;
      case SGE_TYPE_MANAGER:
         directory = MAN_DIR;
         filename = key;
         break;
      case SGE_TYPE_OPERATOR:
         directory = OP_DIR;
         filename = key;
         break;
      case SGE_TYPE_PE:
         directory = PE_DIR;
         filename = key;
         break;
      case SGE_TYPE_CQUEUE:
         directory = CQUEUE_DIR;
         filename  = key;
         break;
      case SGE_TYPE_QINSTANCE:
         directory = QINSTANCES_DIR;
         filename  = key;
         break;
      case SGE_TYPE_SUBMITHOST:
         directory = SUBMITHOST_DIR;
         filename  = key;
         break;
      case SGE_TYPE_USERSET:
         directory = USERSET_DIR;
         filename  = key;
         break;
      case SGE_TYPE_HGROUP:
         directory = HGROUP_DIR;
         filename  = key;
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_TYPE_CUSER:
         directory = UME_DIR;
         filename  = key;
         break;
#endif
      case SGE_TYPE_PROJECT:
         directory = PROJECT_DIR;
         filename  = key;
         break;
      case SGE_TYPE_USER:
         directory = USER_DIR;
         filename  = key;
         break;
      case SGE_TYPE_SHARETREE:
         directory = ".";
         filename  = SHARETREE_FILE;
         break;
      case SGE_TYPE_SCHEDD_CONF:
         directory = ".";
         filename  = SCHED_CONF_FILE;
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
               job_write_common_part((lListElem *)object, 0, SPOOL_DEFAULT);
            } else {
               job_write_spool_file((lListElem *)object, ja_task_id, pe_task_id,
                                    SPOOL_DEFAULT);
            }

            free(dup);
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

   /* spool, if possible using default spooling behaviour.
    * job are spooled in corresponding case branch 
    */
   if (url != NULL && directory != NULL && filename != NULL) {
      dstring filepath_buffer;

      sge_dstring_init(&filepath_buffer, tmpfilebuf, SGE_PATH_MAX);

      /* first write to a temporary file; for jobs it is already initialized */
      tmpfilepath = sge_dstring_sprintf(&filepath_buffer, "%s/%s/.%s", 
                                        url, directory, filename);

      /* spool */
      tmpfilepath = spool_flatfile_write_object(answer_list, object, 
                                                field_info->fields, 
                                                field_info->instr, 
                                                SP_DEST_SPOOL, SP_FORM_ASCII,
                                                tmpfilepath);

      if(tmpfilepath == NULL) {
         ret = false;
      } else {
         /* spooling was ok: rename temporary to target file */
         filepath = sge_dstring_sprintf(&filepath_buffer, "%s/%s/%s", 
                                        url, directory, filename);

         if (rename(tmpfilepath, filepath) == -1) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORRENAMING_SSS, 
                                    tmpfilepath, filepath, strerror(errno));
            ret = false;
         }
      }

      FREE(tmpfilepath);
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
*     spool_flatfile_default_delete_func(lList **answer_list, 
*                                        const lListElem *type, 
*                                        const lListElem *rule, 
*                                        const char *key, 
*                                        const sge_object_type object_type) 
*
*  FUNCTION
*     Deletes an object in the flatfile spooling.
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
*     spool/flatfile/--Flatfile-Spooling
*     spool/spool_delete_object()
*******************************************************************************/
bool
spool_flatfile_default_delete_func(lList **answer_list, 
                                   const lListElem *type, 
                                   const lListElem *rule,
                                   const char *key, 
                                   const sge_object_type object_type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "spool_flatfile_default_delete_func");

   switch(object_type) {
      case SGE_TYPE_ADMINHOST:
         ret = sge_unlink(ADMINHOST_DIR, key) == 0;
         break;
      case SGE_TYPE_CALENDAR:
         ret = sge_unlink(CAL_DIR, key) == 0;
         break;
      case SGE_TYPE_CKPT:
         ret = sge_unlink(CKPTOBJ_DIR, key) == 0;
         break;
      case SGE_TYPE_CONFIG:
         if(sge_hostcmp(key, "global") == 0) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_SPOOL_GLOBALCONFIGNOTDELETED);
            ret = false;
         } else {
            char dir_name_buf[SGE_PATH_MAX];
            dstring dir_name_dstring;
            const char *dir_name;

            sge_dstring_init(&dir_name_dstring, dir_name_buf, SGE_PATH_MAX);
            dir_name = sge_dstring_sprintf(&dir_name_dstring, "%s/%s",
                                           lGetString(rule, SPR_url), 
                                           LOCAL_CONF_DIR);
            ret = sge_unlink(dir_name, key) == 0;
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
            if (job_remove_spool_file(job_id, ja_task_id, pe_task_id, 
                                      SPOOL_DEFAULT) != 0) {
               ret = false;
            }
            free(dup);
         }
         break;
      case SGE_TYPE_MANAGER:
         ret = sge_unlink(MAN_DIR, key) == 0;
         break;
      case SGE_TYPE_OPERATOR:
         ret = sge_unlink(OP_DIR, key) == 0;
         break;
      case SGE_TYPE_SHARETREE:
         ret = sge_unlink(NULL, key) == 0;
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
#if 0
         ret = sge_unlink(QINSTANCE_DIR, key) == 0;
#else
         ret = false;
#endif
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
         break;
   }

   DEXIT;
   return ret;
}

