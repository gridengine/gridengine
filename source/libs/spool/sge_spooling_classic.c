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

#include "unistd.h"
#include "stdlib.h"
#include "string.h"

#include "sgermon.h"
#include "sge_log.h"

#include "sge_unistd.h"
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

#ifndef __SGE_NO_USERMAPPING__
#include "sge_usermap.h"
#include "sge_hostgroup.h"
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

#include "msg_spoollib_classic.h"

#include "sge_spooling_classic.h"

/* We expect the two directories to exist:
 * common_dir
 * spool_dir
 * cwd should be spool_dir, if it is not, a warning is output and
 * the directory is changed.
 */

lListElem *spool_classic_create_context(const char *common_dir, 
                                        const char *spool_dir)
{
   lListElem *context, *rule, *type;

   DENTER(TOP_LAYER, "spool_classic_create_context");

   /* check parameters - both must be set and be absolute paths */
   if(common_dir == NULL || spool_dir == NULL ||
      *common_dir != '/' || *spool_dir != '/') {
      CRITICAL((SGE_EVENT, MSG_SPOOL_INCORRECTPATHSFORCOMMONANDSPOOLDIR));
      SGE_EXIT(EXIT_FAILURE);
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
   type = spool_context_create_type(context, SGE_EMT_ALL);
   spool_type_add_rule(type, rule, TRUE);

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
   type = spool_context_create_type(context, SGE_EMT_CONFIG);
   spool_type_add_rule(type, rule, TRUE);
   type = spool_context_create_type(context, SGE_EMT_SCHEDD_CONF);
   spool_type_add_rule(type, rule, TRUE);

   DEXIT;
   return context;
}

int spool_classic_default_startup_func(const lListElem *rule)
{
   char *cwd;
   const char *url;

   DENTER(TOP_LAYER, "spool_classic_default_startup_func");

   /* check, if we are in the spool directory */
   cwd = getcwd(NULL, SGE_PATH_MAX);
   url = lGetString(rule, SPR_url);
   if(strcmp(cwd, url) != 0) {
      WARNING((SGE_EVENT, MSG_SPOOL_STARTEDINWRONGDIRECTORY_SS, url, cwd));
      sge_chdir(url, TRUE);
   }
   free(cwd);

   /* create spool sub directories */
   sge_mkdir(JOB_DIR,  0755, TRUE);
   sge_mkdir(ZOMBIE_DIR, 0755, TRUE);
   sge_mkdir(QUEUE_DIR,  0755, TRUE);
   sge_mkdir(EXECHOST_DIR, 0755, TRUE);
   sge_mkdir(SUBMITHOST_DIR, 0755, TRUE);
   sge_mkdir(ADMINHOST_DIR, 0755, TRUE);
   sge_mkdir(COMPLEX_DIR, 0755, TRUE);
   sge_mkdir(EXEC_DIR, 0755, TRUE);
   sge_mkdir(PE_DIR, 0755, TRUE);
   sge_mkdir(CKPTOBJ_DIR, 0755, TRUE);
   sge_mkdir(USERSET_DIR, 0755, TRUE);
   sge_mkdir(CAL_DIR, 0755, TRUE);

#ifndef __SGE_NO_USERMAPPING__
   sge_mkdir(HOSTGROUP_DIR, 0755, TRUE);
   sge_mkdir(UME_DIR, 0755, TRUE);
#endif

   sge_mkdir(USER_DIR, 0755, TRUE);
   sge_mkdir(PROJECT_DIR, 0755, TRUE);

   DEXIT;
   return TRUE;
}

int spool_classic_common_startup_func(const lListElem *rule)
{
   const char *url;
   dstring local_dir = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_common_startup_func");

   /* check common directories */
   url = lGetString(rule, SPR_url);
   if(!sge_is_directory(url)) {
      CRITICAL((SGE_EVENT, MSG_SPOOL_COMMONDIRDOESNOTEXIST_S, url));
      SGE_EXIT(EXIT_FAILURE);
   }

   /* create directory for local configurations */
   sge_dstring_sprintf(&local_dir, "%s/%s", url, LOCAL_CONF_DIR);
   sge_mkdir(sge_dstring_get_string(&local_dir), 0755, TRUE);

   DEXIT;
   return TRUE;
}

int spool_classic_default_list_func(const lListElem *type, const lListElem *rule,
                                    lList **list, const sge_event_type event_type)
{
   static dstring file_name = DSTRING_INIT;
   static dstring dir_name  = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_list_func");

   switch(event_type) {
      case SGE_EMT_ADMINHOST:
         sge_read_adminhost_list_from_disk();
         break;
      case SGE_EMT_EXECHOST:
         sge_read_exechost_list_from_disk();
         break;
      case SGE_EMT_SUBMITHOST:
         sge_read_submithost_list_from_disk();
         break;
      case SGE_EMT_CALENDAR:
         sge_read_cal_list_from_disk();
         break;
      case SGE_EMT_CKPT:
         sge_read_ckpt_list_from_disk();
         break;
      case SGE_EMT_COMPLEX:
         read_all_complexes();
         break;
      case SGE_EMT_CONFIG:
         sge_dstring_sprintf(&file_name, "%s/%s",
                             lGetString(rule, SPR_url), CONF_FILE);
         sge_dstring_sprintf(&dir_name, "%s/%s",
                             lGetString(rule, SPR_url), LOCAL_CONF_DIR);
         read_all_configurations(&Master_Config_List, 
                                 sge_dstring_get_string(&file_name), 
                                 sge_dstring_get_string(&dir_name));
         break;
      case SGE_EMT_JOB:
         job_list_read_from_disk(&Master_Job_List, "Master_Job_List", 0,
                                 SPOOL_DEFAULT, NULL);
         job_list_read_from_disk(&Master_Zombie_List, "Master_Zombie_List", 0,
                                 SPOOL_HANDLE_AS_ZOMBIE, NULL);
         break;
      case SGE_EMT_MANAGER:
         read_manop(SGE_MANAGER_LIST);
         break;
      case SGE_EMT_OPERATOR:
         read_manop(SGE_OPERATOR_LIST);
         break;
      case SGE_EMT_SHARETREE:
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
      case SGE_EMT_PE:
         sge_read_pe_list_from_disk();
         break;
      case SGE_EMT_PROJECT:
         sge_read_project_list_from_disk();
         break;
      case SGE_EMT_QUEUE:
         sge_read_queue_list_from_disk();
         break;
      case SGE_EMT_SCHEDD_CONF:
         if(*list != NULL) {
            *list = lFreeList(*list);
         }
         sge_dstring_sprintf(&file_name, "%s/%s", 
                             lGetString(rule, SPR_url), SCHED_CONF_FILE);
         *list = read_sched_configuration(lGetString(rule, SPR_url), sge_dstring_get_string(&file_name), 1, NULL);
         break;
      case SGE_EMT_USER:
         sge_read_user_list_from_disk();
         break;
      case SGE_EMT_USERSET:
         sge_read_userset_list_from_disk();
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_EMT_USERMAPPING:
         sge_read_user_mapping_entries_from_disk();
         break;
      case SGE_EMT_HOSTGROUP:
         sge_read_host_group_entries_from_disk();
         break;
#endif
      default:
         break;
   }

   return TRUE;
}

lListElem *spool_classic_default_read_func(const lListElem *type, const lListElem *rule,
                                           const char *key, const sge_event_type event_type)
{
   lListElem *ep = NULL;

   static dstring file_name = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_read_func");

   switch(event_type) {
      case SGE_EMT_ADMINHOST:
         ep = cull_read_in_host(ADMINHOST_DIR, key, CULL_READ_SPOOL, AH_name, NULL, NULL);
         break;
      case SGE_EMT_EXECHOST:
         ep = cull_read_in_host(EXECHOST_DIR, key, CULL_READ_SPOOL, EH_name, NULL, NULL);
         break;
      case SGE_EMT_SUBMITHOST:
         ep = cull_read_in_host(SUBMITHOST_DIR, key, CULL_READ_SPOOL, SH_name, NULL, NULL);
         break;
      case SGE_EMT_CALENDAR:
         ep = cull_read_in_cal(CAL_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_EMT_CKPT:
         ep = cull_read_in_ckpt(CKPTOBJ_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_EMT_COMPLEX:
         sge_dstring_sprintf(&file_name, "%s/%s", COMPLEX_DIR, key);
         ep = read_cmplx(sge_dstring_get_string(&file_name), key, NULL);
         break;
      case SGE_EMT_CONFIG:
         sge_dstring_sprintf(&file_name, "%s/%s/%s",
                             lGetString(rule, SPR_url), LOCAL_CONF_DIR, key);
         ep = read_configuration(sge_dstring_get_string(&file_name), 
                                 key, FLG_CONF_SPOOL);
         break;
      case SGE_EMT_JOB:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGJOB));
         break;
      case SGE_EMT_MANAGER:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGMANAGER));
         break;
      case SGE_EMT_OPERATOR:
         WARNING((SGE_EVENT, MSG_SPOOL_NOTSUPPORTEDREADINGOPERATOR));
         break;
      case SGE_EMT_SHARETREE:
         {
            lListElem *ep;
            char err_str[1024];
            ep = read_sharetree(SHARETREE_FILE, NULL, 1, err_str, 1, NULL);
         }
         break;
      case SGE_EMT_PE:
         ep = cull_read_in_pe(PE_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_EMT_PROJECT:
         ep = cull_read_in_userprj(PROJECT_DIR, key, 1, 0, NULL);
         break;
      case SGE_EMT_QUEUE:
         ep = cull_read_in_qconf(QUEUE_DIR, key, 1, 0, NULL, NULL);
         break;
      case SGE_EMT_SCHEDD_CONF:
         sge_dstring_sprintf(&file_name, "%s/%s",
                             lGetString(rule, SPR_url), SCHED_CONF_FILE);
         ep = cull_read_in_schedd_conf(NULL, sge_dstring_get_string(&file_name),
                                       1, NULL);
         break;
      case SGE_EMT_USER:
         ep = cull_read_in_userprj(USER_DIR, key, 1, 0, NULL);
         break;
      case SGE_EMT_USERSET:
         ep = cull_read_in_userset(USERSET_DIR, key, 1, 0, NULL); 
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_EMT_USERMAPPING:
         ep = cull_read_in_ume(UME_DIR, key , 1, 0, NULL); 
         break;
      case SGE_EMT_HOSTGROUP:
         ep = cull_read_in_host_group(HOSTGROUP_DIR, key, 1, 0, NULL); 
         break;
#endif
      default:
         break;
   }

   return NULL;
}

int spool_classic_default_write_func(const lListElem *type, const lListElem *rule, 
                                     const lListElem *object, const char *key, 
                                     const sge_event_type event_type)
{
   static dstring file_name = DSTRING_INIT;

   DENTER(TOP_LAYER, "spool_classic_default_write_func");
   switch(event_type) {
      case SGE_EMT_ADMINHOST:
         write_host(1, 2, object, AH_name, NULL);
         break;
      case SGE_EMT_CALENDAR:
         write_cal(1, 2, object);
         break;
      case SGE_EMT_CKPT:
         write_ckpt(1, 2, object);
         break;
      case SGE_EMT_COMPLEX:
         sge_dstring_sprintf(&file_name, "%s/%s", COMPLEX_DIR, lGetString(object, CX_name));
         write_cmplx(1, sge_dstring_get_string(&file_name), lGetList(object, CX_entries), NULL, NULL);
         break;
      case SGE_EMT_CONFIG:
         if(sge_hostcmp(lGetHost(object, CONF_hname), "global") == 0) {
            sge_dstring_sprintf(&file_name, "%s/%s",
                                lGetString(rule, SPR_url), CONF_FILE);
         } else {
            sge_dstring_sprintf(&file_name, "%s/%s/%s", 
                                lGetString(rule, SPR_url), LOCAL_CONF_DIR,
                                lGetHost(object, CONF_hname));
         }
         write_configuration(1, NULL, sge_dstring_get_string(&file_name), object, NULL, FLG_CONF_SPOOL);
         break;
      case SGE_EMT_EXECHOST:
         write_host(1, 2, object, EH_name, NULL);
         break;
      case SGE_EMT_JOB:
         {
            u_long32 job_id, ja_task_id;
            char *pe_task_id;
            char *dup = strdup(key);
            
            job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id);
   
            DPRINTF(("spooling job %d.%d %s\n", job_id, ja_task_id, 
                     pe_task_id != NULL ? pe_task_id : "<null>"));
            /* JG: TODO: why does job spooling not take a const lListElem? */
            job_write_spool_file((lListElem *)object, ja_task_id, pe_task_id, SPOOL_DEFAULT);
            free(dup);
         }
         break;
      case SGE_EMT_MANAGER:
         write_manop(1, SGE_MANAGER_LIST);
         break;
      case SGE_EMT_OPERATOR:
         write_manop(1, SGE_OPERATOR_LIST);
         break;
      case SGE_EMT_SHARETREE:
         write_sharetree(NULL, object, SHARETREE_FILE, NULL, 1, 1, 1);
         break;
      case SGE_EMT_PE:
         write_pe(1, 2, object);
         break;
      case SGE_EMT_PROJECT:
         sge_dstring_sprintf(&file_name, "%s/%s", PROJECT_DIR, lGetString(object, UP_name));
         write_userprj(NULL, object, sge_dstring_get_string(&file_name), NULL, 1, 0);
         break;
      case SGE_EMT_QUEUE:
         sge_dstring_sprintf(&file_name, "%s", lGetString(object, QU_qname));
         cull_write_qconf(1, 0, QUEUE_DIR, sge_dstring_get_string(&file_name), NULL, object);
         break;
      case SGE_EMT_SCHEDD_CONF:
         write_sched_configuration(1, 2, lGetString(rule, SPR_url), object);
         break;
      case SGE_EMT_SUBMITHOST:
         write_host(1, 2, object, SH_name, NULL);
         break;
      case SGE_EMT_USER:
         sge_dstring_sprintf(&file_name, "%s/%s", USER_DIR, lGetString(object, UP_name));
         write_userprj(NULL, object, sge_dstring_get_string(&file_name), NULL, 1, 0);
         break;
      case SGE_EMT_USERSET:
         sge_dstring_sprintf(&file_name, "%s/%s", USERSET_DIR, lGetString(object, US_name));
         write_userset(NULL, object, sge_dstring_get_string(&file_name), NULL, 1);
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_EMT_USERMAPPING:
         write_ume(1, 2, object);
         break;
      case SGE_EMT_HOSTGROUP:
         write_host_group(1, 2, object);
         break;
#endif
      default:
         break;
   }

   DEXIT;
   return TRUE;
}

int spool_classic_default_delete_func(const lListElem *type, const lListElem *rule, 
                                      const char *key, const sge_event_type event_type)
{
   DENTER(TOP_LAYER, "spool_classic_default_delete_func");

   switch(event_type) {
      case SGE_EMT_ADMINHOST:
         sge_unlink(ADMINHOST_DIR, key);
         break;
      case SGE_EMT_CALENDAR:
         sge_unlink(CAL_DIR, key);
         break;
      case SGE_EMT_CKPT:
         sge_unlink(CKPTOBJ_DIR, key);
         break;
      case SGE_EMT_COMPLEX:
         sge_unlink(COMPLEX_DIR, key);
         break;
      case SGE_EMT_CONFIG:
         if(sge_hostcmp(key, "global") == 0) {
            ERROR((SGE_EVENT, MSG_SPOOL_GLOBALCONFIGNOTDELETED));
         } else {
            sge_unlink("../../common/local_conf", key);
         }
         break;
      case SGE_EMT_EXECHOST:
         sge_unlink(EXECHOST_DIR, key);
         break;
      case SGE_EMT_JOB:
      case SGE_EMT_JATASK:   
      case SGE_EMT_PETASK:   
         {
            u_long32 job_id, ja_task_id;
            char *pe_task_id;
            char *dup = strdup(key);
            
            job_parse_key(dup, &job_id, &ja_task_id, &pe_task_id);
   
            DPRINTF(("spooling job %d.%d %s\n", job_id, ja_task_id, 
                     pe_task_id != NULL ? pe_task_id : "<null>"));
            job_remove_spool_file(job_id, ja_task_id, pe_task_id, SPOOL_DEFAULT);
            free(dup);
         }
         break;
      case SGE_EMT_MANAGER:
         write_manop(1, SGE_MANAGER_LIST);
         break;
      case SGE_EMT_OPERATOR:
         write_manop(1, SGE_OPERATOR_LIST);
         break;
      case SGE_EMT_SHARETREE:
         sge_unlink(NULL, key);
         break;
      case SGE_EMT_PE:
         sge_unlink(PE_DIR, key);
         break;
      case SGE_EMT_PROJECT:
         sge_unlink(PROJECT_DIR, key);
         break;
      case SGE_EMT_QUEUE:
         sge_unlink(QUEUE_DIR, key);
         break;
      case SGE_EMT_SCHEDD_CONF:
         ERROR((SGE_EVENT, MSG_SPOOL_SCHEDDCONFIGNOTDELETED));
         break;
      case SGE_EMT_SUBMITHOST:
         sge_unlink(SUBMITHOST_DIR, key);
         break;
      case SGE_EMT_USER:
         sge_unlink(USER_DIR, key);
         break;
      case SGE_EMT_USERSET:
         sge_unlink(USERSET_DIR, key);
         break;
#ifndef __SGE_NO_USERMAPPING__
      case SGE_EMT_USERMAPPING:
         sge_unlink(UME_DIR, key);
         break;
      case SGE_EMT_HOSTGROUP:
         sge_unlink(HOSTGROUP_DIR, key);
         break;
#endif
      default:
         break;
   }

   return TRUE;
}



