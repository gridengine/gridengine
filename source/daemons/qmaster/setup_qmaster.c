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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  
#include <errno.h>

#include "sge.h"
#include "def.h"
#include "sge_conf.h"
#include "commlib.h"
#include "subordinate_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_sched.h"
#include "sge_all_listsL.h"
#include "read_write_host.h"
#include "read_write_cal.h"
#include "read_write_manop.h"
#include "read_write_pe.h"
#include "read_write_host_group.h"
#include "read_write_ume.h"
#include "read_write_job.h"
#include "read_write_userset.h"
#include "read_write_sharetree.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_queue_qmaster.h"
#include "sge_manop_qmaster.h"
#include "slots_used.h"
#include "complex_qmaster.h"
#include "complex_history.h"
#include "path_history.h"
#include "opt_history.h"
#include "sge_job_qmaster.h"
#include "configuration_qmaster.h"
#include "qmaster_heartbeat.h"
#include "qm_name.h"
#include "sched_conf.h"
#include "sched_conf_qmaster.h"
#include "read_write_userprj.h"
#include "sge_sharetree.h"
#include "sge_sharetree_qmaster.h"
#include "sge_userset.h"
#include "read_write_ckpt.h"
#include "read_write_queue.h"
#include "read_write_job.h"
#include "sge_feature.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "utility_daemon.h"
#include "gdi_utility_qmaster.h"
#include "setup_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "resolve_host.h"
#include "config_file.h"
#include "sge_qmod_qmaster.h"
#include "time_event.h"
#include "sge_give_jobs.h"
#include "sge_user_mapping.h"
#include "setup_path.h"
#include "reschedule.h"
#include "msg_daemons_common.h"
#include "msg_qmaster.h"
#include "reschedule.h"
#include "sge_job.h"
#include "sge_spool.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"
#include "sge_io.h"
#include "sge_sharetree_qmaster.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_complex.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_sharetree.h"
#include "sge_hostgroup.h"
#include "sge_usermap.h"

#include "msg_common.h"

#ifndef __SGE_NO_USERMAPPING__
static int sge_read_user_mapping_entries_from_disk(void);
static int sge_read_host_group_entries_from_disk(void);
#endif

static int reresolve_host(lListElem *ep, int nm, char *object_name, char *object_dir);

static int sge_read_host_list_from_disk(void);
static int sge_read_pe_list_from_disk(void);
static int sge_read_ckpt_list_from_disk(void);
static int sge_read_cal_list_from_disk(void);
static int remove_invalid_job_references(int user);

static int debit_all_jobs_from_qs(void);

/*------------------------------------------------------------*/
int sge_setup_qmaster()
{
   lList *direntries;
   lListElem *jep, *ep, *qep, *tmpqep, *direntry = NULL;
   static int first = TRUE;
   int ret, config_tag = 0;
   lListElem *lep = NULL;
   lList *alp=NULL;
   char err_str[1024];
   u_long32 state;
#ifdef PW   
   extern u_long32 pw_num_submit;
#endif   
   extern int new_config;

   DENTER(TOP_LAYER, "sge_setup_qmaster");

   if (first)
      first = FALSE;
   else {
      CRITICAL((SGE_EVENT, MSG_SETUP_SETUPMAYBECALLEDONLYATSTARTUP));
      DEXIT;
      return -1;
   }   

   /* register our error function for use in replace_params() */
   config_errfunc = error;

   /*
   ** get cluster configuration
   */
   read_all_configurations(&Master_Config_List);
   ret = select_configuration(me.qualified_hostname, Master_Config_List, &lep);
   if (ret) {
      if (ret == -3)
         WARNING((SGE_EVENT, MSG_CONFIG_FOUNDNOLOCALCONFIGFORQMASTERHOST_S,
                 me.qualified_hostname));
      else {           
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_IS, ret, me.qualified_hostname));
         return -1;
      }   
   }
   ret = merge_configuration( lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), lep, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS, ret, me.qualified_hostname));
      return -1;
   }
   sge_show_conf();         
   new_config = 1;

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_ERROR_CANTSWITCHTOADMINUSER));
      SGE_EXIT(1);
   }
   /* pass max unheard to commlib */
   set_commlib_param(CL_P_LT_HEARD_FROM_TIMEOUT, conf.max_unheard, NULL, NULL);

   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();

   /*
   ** build and change to master spool dir
   */
   DPRINTF(("chdir(\"/\")----------------------------\n"));
   sge_chdir("/", 1);

   DPRINTF(("Making directories----------------------------\n"));
   sge_mkdir(conf.qmaster_spool_dir, 0755, 1);

   DPRINTF(("chdir("SFQ")----------------------------\n", conf.qmaster_spool_dir));
   sge_chdir(conf.qmaster_spool_dir, 1);

   /* 
   ** we are in the master spool dir now 
   ** log messages into ERR_FILE in master spool dir 
   */
   sge_copy_append(TMP_ERR_FILE_QMASTER, ERR_FILE, SGE_MODE_APPEND);
   sge_switch2start_user();
   unlink(TMP_ERR_FILE_QMASTER);   
   sge_switch2admin_user();
   error_file = ERR_FILE;

   /* 
   ** increment the heartbeat as early as possible 
   ** and write our name to the act_qmaster file
   ** the lock file will be removed as late as possible
   */
   inc_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE);

   /* 
   ** write our host name to the act_qmaster file 
   */
   if (write_qm_name(me.qualified_hostname, path.act_qmaster_file, err_str)) {
      ERROR((SGE_EVENT, "%s\n", err_str));
      SGE_EXIT(1);
   }

   /*
   ** create directory tree needed in master spool dir
   */
   sge_mkdir(JOB_DIR,  0755, 1);
   sge_mkdir(ZOMBIE_DIR, 0755, 1);
   sge_mkdir(QUEUE_DIR,  0755, 1);
   sge_mkdir(EXECHOST_DIR, 0755, 1);
   sge_mkdir(SUBMITHOST_DIR, 0755, 1);
   sge_mkdir(ADMINHOST_DIR, 0755, 1);
   sge_mkdir(COMPLEX_DIR, 0755, 1);
   sge_mkdir(EXEC_DIR, 0755, 1);
   sge_mkdir(PE_DIR, 0755, 1);
   sge_mkdir(CKPTOBJ_DIR, 0755, 1);
   sge_mkdir(USERSET_DIR, 0755, 1);
   sge_mkdir(CAL_DIR, 0755, 1);
#ifndef __SGE_NO_USERMAPPING__
   sge_mkdir(HOSTGROUP_DIR, 0755, 1);
   sge_mkdir(UME_DIR, 0755, 1);
#endif


   /*
   ** read in all objects and check for correctness
   */
   DPRINTF(("Complexes-------------------------------\n"));
   if (read_all_complexes()) {
      DEXIT;
      return -1;
   }

   DPRINTF(("host_list----------------------------\n"));
   if (sge_read_host_list_from_disk()) {
      DEXIT;
      return -1;
   }

   

   if (!host_list_locate(Master_Exechost_List, SGE_TEMPLATE_NAME)) {
      /* add an exec host "template" */
      if (sge_add_host_of_type(SGE_TEMPLATE_NAME, SGE_EXECHOST_LIST))
         ERROR((SGE_EVENT, MSG_CONFIG_ADDINGHOSTTEMPLATETOEXECHOSTLIST));
   }

   /* add host "global" to Master_Exechost_List as an exec host */
   if (!host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME)) {
      /* add an exec host "global" */
      if (sge_add_host_of_type(SGE_GLOBAL_NAME, SGE_EXECHOST_LIST))
         ERROR((SGE_EVENT, MSG_CONFIG_ADDINGHOSTGLOBALTOEXECHOSTLIST));
   }

   /* add qmaster host to Master_Adminhost_List as an administrativ host */
   if (!host_list_locate(Master_Adminhost_List, me.qualified_hostname)) {
      if (sge_add_host_of_type(me.qualified_hostname, SGE_ADMINHOST_LIST)) {
         DEXIT;
         return -1;
      }
   }

#ifdef PW
   /* check for licensed # of submit hosts */
   if ((ret=sge_count_uniq_hosts(Master_Adminhost_List, 
         Master_Submithost_List)) < 0) {
      /* s.th.'s wrong, but we can't blame it on the user so we
       * keep truckin'
       */
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTCOUNT_HOSTS_S, SGE_FUNC));
   } else {
      if (pw_num_submit < ret) {
         /* we've a license violation */
         ERROR((SGE_EVENT, MSG_SGETEXT_TOOFEWSUBMHLIC_II, (int) pw_num_submit, ret));
         return -1;
      }
   }
#endif

   DPRINTF(("manager_list----------------------------\n"));
   read_manop(SGE_MANAGER_LIST);
   if (!sge_locate_manager("root")) {
      lAddElemStr(&Master_Manager_List, MO_name, "root", MO_Type);

      if (write_manop(1, SGE_MANAGER_LIST)) {
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEMANAGERLIST)); 
         return -1;
      }
   }
   for_each(ep, Master_Manager_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));

#ifndef __SGE_NO_USERMAPPING__
   DPRINTF(("host group definitions-----------\n"));
   if (sge_read_host_group_entries_from_disk()) {
     DEXIT;
     return -1;
   }
#endif

   DPRINTF(("operator_list----------------------------\n"));
   read_manop(SGE_OPERATOR_LIST);
   if (!sge_locate_operator("root")) {
      lAddElemStr(&Master_Operator_List, MO_name, "root", MO_Type);

      if (write_manop(1, SGE_OPERATOR_LIST)) {
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEOPERATORLIST)); 
         return -1;
      }
   }
   for_each(ep, Master_Operator_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));


   DPRINTF(("userset_list------------------------------\n"));
   Master_Userset_List = lCreateList("user set list", US_Type);
   direntries = sge_get_dirents(USERSET_DIR);
   if (direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINUSERSETS);

      for_each(direntry, direntries) {
         const char *userset = lGetString(direntry, STR);

         if (userset[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_USERSET_S , lGetString(direntry, STR));
            }
            if (verify_str_key(&alp, userset, "userset")) {
               DEXIT;
               return -1;
            }  

            ep = cull_read_in_userset(USERSET_DIR, userset, 1, 0, NULL); 
            if (!ep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, USERSET_DIR, 
                        userset));
               DEXIT;
               return -1;
            }

            if(sge_verify_userset_entries(lGetList(ep, US_entries), NULL, 1) == STATUS_OK) {
               lAppendElem(Master_Userset_List, ep);
            } else {
               lFreeElem(ep);
            }
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", USERSET_DIR, userset);
            unlink(buffer);
         }
      }
      direntries = lFreeList(direntries);
   }

   if (sge_read_cal_list_from_disk()) {
      DEXIT;
      return -1;
   }


#ifndef __SGE_NO_USERMAPPING__
   DPRINTF(("administrator user mapping-----------\n"));
   if (sge_read_user_mapping_entries_from_disk()) {
     DEXIT;
     return -1;
   }
#endif


   DPRINTF(("queue_list---------------------------------\n"));
   direntries = sge_get_dirents(QUEUE_DIR);
   if (direntries) {
      const char *queue_str;
      
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINQUEUES);
      for_each(direntry, direntries) {

         queue_str = lGetString(direntry, STR);
         if (queue_str[0] != '.') {
            config_tag = 0;
            if (!sge_silent_get()) {
               printf(MSG_SETUP_QUEUE_S, lGetString(direntry, STR));
            }
            if (verify_str_key(&alp, queue_str, "queue")) {
               DEXIT;
               return -1;
            }   
            qep = cull_read_in_qconf(QUEUE_DIR, lGetString(direntry, STR), 1, 
                  0, &config_tag, NULL);
            if (!qep) {
               ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, QUEUE_DIR, 
                        lGetString(direntry, STR)));
               DEXIT;
               return -1;
            }
            if (config_tag & CONFIG_TAG_OBSOLETE_VALUE) {
               /* an obsolete config value was found in the file.
                  spool it out again to have the newest version on disk. */
               cull_write_qconf(1, 0, QUEUE_DIR, lGetString(direntry, STR), 
                     NULL, qep);
               INFO((SGE_EVENT, MSG_CONFIG_QUEUEXUPDATED_S, 
                     lGetString(direntry, STR)));
            }
            
            if (!strcmp(lGetString(direntry, STR), SGE_TEMPLATE_NAME) && 
                !strcmp(lGetString(qep, QU_qname), SGE_TEMPLATE_NAME)) {
               /* 
                  we do not keep the queue template in the main queue list 
                  to be compatible with other old code in the qmaster
               */
               qep = lFreeElem(qep);
               sge_unlink(QUEUE_DIR, lGetString(direntry, STR));
               WARNING((SGE_EVENT, MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED));
            }
            else if (!strcmp(lGetString(qep, QU_qname), SGE_TEMPLATE_NAME)) {
               /*
                  oops!  found queue 'template', but not in file 'template'
               */
               ERROR((SGE_EVENT, MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT));
               qep = lFreeElem(qep);
            }
            else {
               lListElem *exec_host;

               /* handle slots from now on as a consumble attribute of queue */
               slots2config_list(qep); 

               /* setup actual list of queue */
               debit_queue_consumable(NULL, qep, Master_Complex_List, 0);

               /* init double values of consumable configuration */
               sge_fill_requests(lGetList(qep, QU_consumable_config_list), Master_Complex_List, 1, 0, 1);

               if (complex_list_verify(lGetList(qep, QU_complex_list), NULL, 
                                       "queue", lGetString(qep, QU_qname))
                    !=STATUS_OK) {
                  qep = lFreeElem(qep);            
                  DEXIT;
                  return -1;
               }
               if (ensure_attrib_available(NULL, qep, QU_load_thresholds) ||
                   ensure_attrib_available(NULL, qep, QU_suspend_thresholds) ||
                   ensure_attrib_available(NULL, qep, QU_consumable_config_list)) {
                  qep = lFreeElem(qep); 
                  DEXIT;
                  return -1;
               }

               sge_add_queue(qep);
               state = lGetUlong(qep, QU_state);
               SETBIT(QUNKNOWN, state);
               state &= ~(QCAL_DISABLED|QCAL_SUSPENDED);
               lSetUlong(qep, QU_state, state);

               set_qslots_used(qep, 0);
               
               if (!(exec_host = host_list_locate(Master_Exechost_List, 
                     lGetHost(qep, QU_qhostname)))) {
                  if (lGetUlong(qep, QU_qtype) & TQ) {
                     ERROR((SGE_EVENT, MSG_CONFIG_CANTRECREATEQEUEUEXFROMDISKBECAUSEOFUNKNOWNHOSTY_SS,
                     lGetString(qep, QU_qname), lGetHost(qep, QU_qhostname)));
                     lRemoveElem(Master_Queue_List, qep);
                  }
                  else {
                     if (sge_add_host_of_type(lGetHost(qep, QU_qhostname), 
				SGE_EXECHOST_LIST)) {
                        qep = lFreeElem(qep);
                        lFreeList(direntries);
                        DEXIT;
                        return -1;
                     }
                  }
               } 

               /*
               ** make a start for the history when Sge first starts up
               ** or when history has been deleted
               */
               if (!is_nohist() && lGetString(qep, QU_qname) &&
                   !is_object_in_history(STR_DIR_QUEUES, lGetString(qep, QU_qname))) {
                  int ret;
                  
                  ret = sge_write_queue_history(qep);
                  if (ret) {
                     WARNING((SGE_EVENT, MSG_CONFIG_CANTWRITEHISTORYFORQUEUEX_S,
                        lGetString(qep, QU_qname)));
                  }
               }
            }
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", QUEUE_DIR, queue_str);
            unlink(buffer);
         }
      }
      lFreeList(direntries);
      queue_list_set_state_to_unknown(Master_Queue_List, NULL, 0);
   }
   

   if (sge_read_pe_list_from_disk()) {
      DEXIT;
      return -1;
   }

   if (sge_read_ckpt_list_from_disk()) {
      DEXIT;
      return -1;
   }

   DPRINTF(("job_list-----------------------------------\n"));
   if (job_list_read_from_disk(&Master_Job_List, "Master_Job_List", 1,
                               SPOOL_DEFAULT, NULL)) {
      DEXIT;
      return -1;
   }

   if (conf.zombie_jobs > 0) {
      DPRINTF(("zombie_list--------------------------------------\n"));
      if (job_list_read_from_disk(&Master_Zombie_List, "Master_Zombie_List", 0,
                                  SPOOL_HANDLE_AS_ZOMBIE, NULL)) {
         DEXIT;
         return -1;
      }
   }

   for_each(jep, Master_Job_List) {
      DPRINTF(("JOB "u32" PRIORITY %d\n", lGetUlong(jep, JB_job_number), 
            (int)lGetUlong(jep, JB_priority) - BASE_PRIORITY));

      /* doing this operation we need the complete job list read in */
      job_suc_pre(jep);
   }

   /* 
      if the job is in state running 
      we have to register each slot 
      in a queue and in the parallel 
      environment if the job is a 
      parallel one
   */
   debit_all_jobs_from_qs(); 
   debit_all_jobs_from_pes(Master_Pe_List); 
         
   /* clear suspend on subordinate flag in QU_state */ 
   for_each(tmpqep, Master_Queue_List) {
      state = lGetUlong(tmpqep, QU_state);
      CLEARBIT(QSUSPENDED_ON_SUBORDINATE, state);
      lSetUlong(tmpqep, QU_state, state);
   }

   /* recompute suspend on subordinate caching fields */
   /* here we assume that all jobs are debited on all queues */
   for_each(tmpqep, Master_Queue_List) {
      lList *to_suspend;

      if (check_subordinate_list(NULL, lGetString(tmpqep, QU_qname), lGetHost(tmpqep, QU_qhostname), 
            lGetUlong(tmpqep, QU_job_slots), lGetList(tmpqep, QU_subordinate_list), 
            CHECK4SETUP)!=STATUS_OK) {
         DEXIT; /* inconsistent subordinates */
         return -1;
      }
      to_suspend = NULL;
      copy_suspended(&to_suspend, lGetList(tmpqep, QU_subordinate_list), 
         qslots_used(tmpqep), lGetUlong(tmpqep, QU_job_slots), 0);
      suspend_all(to_suspend, 1); /* just recompute */
      lFreeList(to_suspend);
   }

   /* calendar */
   {
      lListElem *cep;
      for_each (cep, Master_Calendar_List) 
         calendar_update_queue_states(cep, NULL, NULL);
   }

   /* rebuild signal resend events */
   rebuild_signal_events();

   /* scheduler configuration stuff */
   if (!sge_silent_get()) 
      printf(MSG_CONFIG_READINGINSCHEDULERCONFIG);
   Master_Sched_Config_List = read_sched_configuration(path.sched_conf_file, 1, &alp);
   if (!Master_Sched_Config_List) {
      ERROR((SGE_EVENT, "%s\n", lGetString(lFirst(alp), AN_text)));
      DEXIT;
      return -1;
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {

      sge_mkdir(USER_DIR, 0755, 1);
      sge_mkdir(PROJECT_DIR, 0755, 1);

      /* SGEEE: read user list */
      Master_User_List = lCreateList("user list", UP_Type);
      direntries = sge_get_dirents(USER_DIR);
      if (direntries) {
         if (!sge_silent_get()) 
            printf(MSG_CONFIG_READINGINUSERS);
         
         for_each(direntry, direntries) {
            const char *direntry_str;
            
            direntry_str = lGetString(direntry, STR); 
            if (direntry_str[0] != '.') { 
               config_tag = 0;
               if (!sge_silent_get()) 
                  printf(MSG_SETUP_USER_S, lGetString(direntry, STR));

               ep = cull_read_in_userprj(USER_DIR, lGetString(direntry, STR), 1,
                                          1, &config_tag);
               if (!ep) {
                  ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, USER_DIR, 
                           lGetString(direntry, STR)));
                  DEXIT;
                  return -1;
               }

               lAppendElem(Master_User_List, ep);
            } else {
               char buffer[256];

               sprintf(buffer, "%s/%s", USER_DIR, direntry_str); 
               unlink(buffer);
            }
         }
         direntries = lFreeList(direntries);
      }
      remove_invalid_job_references(1);

      /* SGE: read project list */
      Master_Project_List = lCreateList("project list", UP_Type);
      direntries = sge_get_dirents(PROJECT_DIR);
      if (direntries) {
         if (!sge_silent_get()) 
            printf(MSG_CONFIG_READINGINPROJECTS);

         for_each(direntry, direntries) {
            const char *userprj_str;

            userprj_str = lGetString(direntry, STR);
            if (userprj_str[0] != '.') {
               config_tag = 0;
               if (!sge_silent_get()) 
                  printf(MSG_SETUP_PROJECT_S, lGetString(direntry, STR));
               if (verify_str_key(&alp, userprj_str, "project")) {
                  DEXIT;
                  return -1;
               }  
               ep = cull_read_in_userprj(PROJECT_DIR, lGetString(direntry, STR), 1,
                                          0, &config_tag);
               if (!ep) {
                  ERROR((SGE_EVENT, MSG_CONFIG_READINGFILE_SS, PROJECT_DIR, 
                           lGetString(direntry, STR)));
                  DEXIT;
                  return -1;
               }

               lAppendElem(Master_Project_List, ep);
            } else {
               char buffer[256];

               sprintf(buffer, "%s/%s", PROJECT_DIR, userprj_str);
               unlink(buffer);
            }
         }
         lFreeList(direntries);
      }
      remove_invalid_job_references(0);
   }
   
   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* SGEEE: read share tree */
      ep = read_sharetree(SHARETREE_FILE, NULL, 1, err_str, 1, NULL);
      if (ep) {
         lList *alp = NULL;
         lList *found = NULL;
         ret = check_sharetree(&alp, ep, Master_User_List, Master_Project_List, 
               NULL, &found);
         found = lFreeList(found);
         alp = lFreeList(alp); 

         Master_Sharetree_List = lCreateList("sharetree list", STN_Type);
         lAppendElem(Master_Sharetree_List, ep);
      }
      else {
         Master_Sharetree_List = NULL;
         if ((err_str != NULL) && (err_str[0] != '\0')) {
            int pos = strlen(err_str)-1;
            if (err_str[pos] == '\n') {
               err_str[pos] = '\0';
            }
         }
         WARNING((SGE_EVENT, MSG_CONFIG_CANTLOADSHARETREEXSTARTINGUPWITHEMPTYSHARETREE_S, err_str));
      }
   }

   /* RU: */
   /* initiate timer for all hosts because they start in 'unknown' state */ 
   if (Master_Exechost_List) {
      lListElem *host               = NULL;
      lListElem *global_host_elem   = NULL;
      lListElem *template_host_elem = NULL;

      /* get "global" element pointer */
      global_host_elem   = host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME);   

      /* get "template" element pointer */
      template_host_elem = host_list_locate(Master_Exechost_List, SGE_TEMPLATE_NAME);
  
      for_each(host, Master_Exechost_List) {
         if ( (host != global_host_elem ) && (host != template_host_elem ) ) {
            reschedule_add_additional_time(load_report_interval(host));
            reschedule_unknown_trigger(host);
            reschedule_add_additional_time(0); 
         }
      }
   }

   DEXIT;
   return 0;
}

/* get rid of still debited per job usage contained 
   in user or project object if the job is no longer existing */
static int remove_invalid_job_references(
int user 
) {
   lListElem *up, *upu, *next;
   u_long32 jobid;
   char fname[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "remove_invalid_job_references");

   for_each (up, user?Master_User_List:Master_Project_List) {

      int spool_me = 0;
      next = lFirst(lGetList(up, UP_debited_job_usage));
      while ((upu=next)) {
         next = lNext(upu);

         jobid = lGetUlong(upu, UPU_job_number);
         if (!job_list_locate(Master_Job_List, jobid)) {
            lRemoveElem(lGetList(up, UP_debited_job_usage), upu);
            WARNING((SGE_EVENT, "removing reference to no longer existing job "u32" of %s "SFQ"\n",
                           jobid, user?"user":"project", lGetString(up, UP_name)));
            spool_me = 1;
         }
      }

      if (spool_me) {
         sprintf(fname , "%s/%s", user?USER_DIR:PROJECT_DIR, lGetString(up, UP_name));
         write_userprj(NULL, up, fname, NULL, 1, user);
      }
   }

   DEXIT;
   return 0;
}

#ifndef __SGE_NO_USERMAPPING__
static int sge_read_host_group_entries_from_disk()
{ 
  lList*     direntries = NULL; 
  lListElem* direntry = NULL;
  lListElem* ep = NULL;
  const char*      hostGroupEntry = NULL;
  int        ret = 0;  /* 0 means ok */

  DENTER(TOP_LAYER, "sge_read_host_group_entries_from_disk");
 
 
  direntries = sge_get_dirents(HOSTGROUP_DIR);
  if (direntries) {
     if (Master_Host_Group_List == NULL) {
        Master_Host_Group_List = lCreateList("main host group list", GRP_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGHOSTGROUPENTRYS);
     }
     
     for_each(direntry, direntries) {
        hostGroupEntry = lGetString(direntry, STR);

        if (hostGroupEntry[0] != '.') {
           if (!sge_silent_get()) { 
              printf(MSG_SETUP_HOSTGROUPENTRIES_S, hostGroupEntry);
           }

           ep = cull_read_in_host_group(HOSTGROUP_DIR, hostGroupEntry , 1, 0, NULL); 
           lAppendElem(Master_Host_Group_List, ep);
        } else {
           char buffer[256];

           sprintf(buffer, "%s/%s", HOSTGROUP_DIR, hostGroupEntry);
           unlink(buffer);
        }   
     } 
     direntries = lFreeList(direntries);
 
     ep = Master_Host_Group_List->first;  

     while (ep != NULL) {
        hostGroupEntry = lGetString(ep, GRP_group_name);
        if (hostGroupEntry != NULL) {
           DPRINTF(("----------------> checking group '%s'\n",hostGroupEntry));
        }  
        if (sge_verify_host_group_entry(NULL, Master_Host_Group_List,ep,hostGroupEntry) == FALSE) {
           WARNING((SGE_EVENT, MSG_ANSWER_IGNORINGHOSTGROUP_S, hostGroupEntry  ));
           
           lDechainElem(Master_Host_Group_List, ep);
           lFreeElem(ep);
           ep = NULL;
           ep = Master_Host_Group_List->first;
        } else {
           ep = ep->next;
        }
     } 
  }

  /* everything is done very well ! */
  DEXIT; 
  return ret;
}


static int sge_read_user_mapping_entries_from_disk()
{ 
  lList*     direntries = NULL; 
  lListElem* direntry = NULL;
  lListElem* ep = NULL;
  const char*      ume = NULL;
  int        ret = 0;  /* 0 means ok */

  DENTER(TOP_LAYER, "sge_read_user_mapping_entries_from_disk");
 
 
  direntries = sge_get_dirents(UME_DIR);
  if (direntries) {
     if (Master_Usermapping_Entry_List == NULL) {
        Master_Usermapping_Entry_List = 
           lCreateList("Master_Usermapping_Entry_List", UME_Type);
     }  
     if (!sge_silent_get()) { 
        printf(MSG_CONFIG_READINGUSERMAPPINGENTRY);
     }
     
     for_each(direntry, direntries) {
         ume = lGetString(direntry, STR);

         if (ume[0] != '.') {
            if (!sge_silent_get()) { 
               printf(MSG_SETUP_MAPPINGETRIES_S, ume);
            }

            ep = cull_read_in_ume(UME_DIR, ume , 1, 0, NULL); 
         
            if (sge_verifyMappingEntry(NULL, Master_Host_Group_List,ep, ume, 
               Master_Usermapping_Entry_List) == TRUE) {
               lAppendElem(Master_Usermapping_Entry_List, ep);
            } else {
               WARNING((SGE_EVENT, MSG_ANSWER_IGNORINGMAPPINGFOR_S,  ume ));  
               ep = lFreeElem(ep);
               ep = NULL; 
            } 
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", UME_DIR, ume);
            unlink(buffer);
         }
     } 
     direntries = lFreeList(direntries);
  }
  
  /* everything is done very well ! */
  DEXIT; 
  return ret;
}
#endif

static int sge_read_host_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *host;

   DENTER(TOP_LAYER, "sge_read_host_list_from_disk");

   /* 
   ** read exechosts into Master_Exechost_List 
   */
   if (!Master_Exechost_List)
      Master_Exechost_List = lCreateList("Master_Exechost_List", EH_Type);

   direntries = sge_get_dirents(EXECHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINEXECUTIONHOSTS);
      
      for_each(direntry, direntries) {

         host = lGetString(direntry, STR);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(EXECHOST_DIR, host, CULL_READ_SPOOL, EH_name, 
                                   NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            }

           /* resolve hostname anew */
            if (reresolve_host(ep, EH_name, "exec host", EXECHOST_DIR)) {
               DEXIT;
               return -1; /* general problems */
            }

            /* necessary to setup actual list of exechost */
            debit_host_consumable(NULL, ep, Master_Complex_List, 0);

            /* necessary to init double values of consumalbe configuration */
            sge_fill_requests(lGetList(ep, EH_consumable_config_list), 
                  Master_Complex_List, 1, 0, 1);

            if (complex_list_verify(lGetList(ep, EH_complex_list), NULL, 
                                    "host", lGetHost(ep, EH_name))!=STATUS_OK) {
               DEXIT;
               return -1;
            }
            if (ensure_attrib_available(NULL, ep, EH_consumable_config_list)) {
               ep = lFreeElem(ep);
               DEXIT;
               return -1;
            }

            lAppendElem(Master_Exechost_List, ep);
            /*
            ** make a start for the history when Sge first starts up
            ** or when history has been deleted
            */
            if (!is_nohist() && lGetHost(ep, EH_name) &&
                !is_object_in_history(STR_DIR_EXECHOSTS, 
                   lGetHost(ep, EH_name))) {
               int ret;
            
               ret = write_host_history(ep);
               if (ret) {
                  WARNING((SGE_EVENT, MSG_CONFIG_CANTWRITEHISTORYFORHOSTX_S,
                           lGetHost(ep, EH_name)));
               }
            }
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", EXECHOST_DIR, host);
            unlink(buffer);
         }
      }
      direntries = lFreeList(direntries);
   }

   /* 
   ** read adminhosts into Master_Adminhost_List 
   */
   if (!Master_Adminhost_List)
      Master_Adminhost_List = lCreateList("Master_Adminhost_List", AH_Type);

   direntries = sge_get_dirents(ADMINHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINADMINHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, STR);

         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(ADMINHOST_DIR, host, CULL_READ_SPOOL, AH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, AH_name, "admin host", ADMINHOST_DIR)) {
               direntries = lFreeList(direntries);
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(Master_Adminhost_List, ep);
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", ADMINHOST_DIR, host);
            unlink(buffer);  
         }
      }
      direntries = lFreeList(direntries);
   }

   /* 
   ** read submithosts into Master_Submithost_List 
   */
   if (!Master_Submithost_List)
      Master_Submithost_List = lCreateList("Master_Submithost_List", SH_Type);

   direntries = sge_get_dirents(SUBMITHOST_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINSUBMITHOSTS);
      for_each(direntry, direntries) {
         host = lGetString(direntry, STR);
         if (host[0] != '.') {
            DPRINTF(("Host: %s\n", host));
            ep = cull_read_in_host(SUBMITHOST_DIR, host, CULL_READ_SPOOL, 
               SH_name, NULL, NULL);
            if (!ep) {
               direntries = lFreeList(direntries);
               DEXIT; 
               return -1;
            } 

            /* resolve hostname anew */
            if (reresolve_host(ep, SH_name, "submit host", SUBMITHOST_DIR)) {
               DEXIT;
               return -1; /* general problems */
            }

            lAppendElem(Master_Submithost_List, ep);
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", SUBMITHOST_DIR, host);
            unlink(buffer);             
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return 0;
}

static int sge_read_pe_list_from_disk()
{
   lList *direntries;
   lList *alp = NULL;
   lListElem *ep, *direntry;
   int ret = 0;
   const char *pe;

   DENTER(TOP_LAYER, "sge_read_pe_list_from_disk");
   
   if (!Master_Pe_List)
      Master_Pe_List = lCreateList("Master_Pe_List", PE_Type);

   direntries = sge_get_dirents(PE_DIR);
   if(direntries) {
      if (!sge_silent_get()) {
         printf(MSG_CONFIG_READINGINGPARALLELENV);
      }
      for_each(direntry, direntries) {
         pe = lGetString(direntry, STR);
         if (pe[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_PE_S, pe);
            }
            if (verify_str_key(&alp, pe, "pe")) {
               DEXIT;
               return -1;
            }       
            ep = cull_read_in_pe(PE_DIR, pe, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (validate_pe(1, ep, NULL)!=STATUS_OK) {
               ret = -1;
               break;
            }
            lAppendElem(Master_Pe_List, ep);
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", PE_DIR, pe);
            unlink(buffer);
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}



static int sge_read_cal_list_from_disk()
{
   lList *direntries;
   lListElem *aep, *ep, *direntry;
   int ret = 0;
   const char *cal;
   const char *s;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_read_cal_list_from_disk");
   
   if (!Master_Calendar_List)
      Master_Calendar_List = lCreateList("Master_Calendar_List", CAL_Type);

   direntries = sge_get_dirents(CAL_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCALENDARS);
      for_each(direntry, direntries) {
         cal = lGetString(direntry, STR);

         if (cal[0] != '.') {
            if (!sge_silent_get()) {
               printf(MSG_SETUP_CALENDAR_S, cal);
            }
            if (verify_str_key(&alp, cal, "cal")) {
               DEXIT;
               return -1;
            }      
            ep = cull_read_in_cal(CAL_DIR, cal, 1, 0, NULL, NULL);
            if (!ep) {
               ret = -1;
               break;
            }

            if (parse_year(&alp, ep) || parse_week(&alp, ep)) {
               if (!(aep = lFirst(alp)) || !(s = lGetString(aep, AN_text)))
                  s = MSG_UNKNOWNREASON;
               ERROR((SGE_EVENT,MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS, 
                     cal, s));
               lFreeList(alp);
               ret = -1;
               break;
            }

            lAppendElem(Master_Calendar_List, ep);
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", CAL_DIR, cal);
            unlink(buffer);  
         }
      }
      direntries = lFreeList(direntries);
   }

   DEXIT;
   return ret;
}

static int sge_read_ckpt_list_from_disk()
{
   lList *direntries;
   lListElem *ep, *direntry;
   const char *ckpt;

   DENTER(TOP_LAYER, "sge_read_ckpt_list_from_disk");
   
   if (!Master_Ckpt_List)
      Master_Ckpt_List = lCreateList("Master_Ckpt_List", CK_Type);

   direntries = sge_get_dirents(CKPTOBJ_DIR);
   if(direntries) {
      if (!sge_silent_get()) 
         printf(MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS);
      for_each(direntry, direntries) {
         ckpt = lGetString(direntry, STR);

         if (ckpt[0] != '.') {
            if (!sge_silent_get()) 
               printf(MSG_SETUP_CKPT_S, ckpt);
            ep = cull_read_in_ckpt(CKPTOBJ_DIR, ckpt, 1, 0, NULL, NULL);
            if (!ep) {
               DEXIT;
               return -1;
            }

            if (validate_ckpt(ep, NULL)!=STATUS_OK) {
               DEXIT;
               return -1;
            }
            
            lAppendElem(Master_Ckpt_List, ep);
         } else {
            char buffer[256];

            sprintf(buffer, "%s/%s", CKPTOBJ_DIR, ckpt);
            unlink(buffer);
         }
      }
      direntries = lFreeList(direntries);
   }
   DEXIT;
   return 0;
}


#if 0
static int sge_read_object_dir(
char *obj_dir,
char *obj_name,
lList **obj_list,
lDescr *obj_dp,
char *obj_title 
) {
   int ret = 0;
   lList *direntries = NULL;
   lListElem *ep, *direntry;

   DENTER(TOP_LAYER, "sge_read_object_dir");

   if (!obj_list) {
      DEXIT;
      return -1;
   }

   if (!*obj_list)
      *obj_list = lCreateList(obj_name, dp);

   direntries = sge_get_dirents(obj_dir);
   
   if (obj_title)
      printf("%s", obj_title);
      
   for_each(direntry, direntries) {
      if (obj_title)
         printf("\t%s\n", lGetString(direntry, STR));
   
      ep = obj_read_func();

      if (!ep) {
         ret = -1;
         break;
      }
      if (obj_validate_func && obj_validate_func(ep, NULL) != STATUS_OK) {
         ret = -1;
         break;
      }
      lAppendElem(obj_list, ep);
   }

   direntries = lFreeList(direntries);

   DEXIT;
   return ret;
}

#endif
   

static int debit_all_jobs_from_qs()
{
   lListElem *gdi;
   u_long32 slots, jid, tid;
   const char *queue_name;
   lListElem *hep, *master_hep, *next_jep, *jep, *qep, *next_jatep, *jatep;
   int ret = 0;

   DENTER(TOP_LAYER, "debit_all_jobs_from_qs");

   next_jep = lFirst(Master_Job_List);
   while ((jep=next_jep)) {
   
      /* may be we have to delete this job */   
      next_jep = lNext(jep);
      jid = lGetUlong(jep, JB_job_number);
      
      next_jatep = lFirst(lGetList(jep, JB_ja_tasks));
      while ((jatep = next_jatep)) {
         next_jatep = lNext(jatep);
         tid = lGetUlong(jatep, JAT_task_number);

         /* don't look at states - we only trust in 
            "granted destin. ident. list" */

         master_hep = NULL;
         for_each (gdi, lGetList(jatep, JAT_granted_destin_identifier_list)) {

            queue_name = lGetString(gdi, JG_qname);
            slots = lGetUlong(gdi, JG_slots);
            
            if (!(qep = queue_list_locate(Master_Queue_List, queue_name))) {
               ERROR((SGE_EVENT, MSG_CONFIG_CANTFINDQUEUEXREFERENCEDINJOBY_SU,  
                  queue_name, u32c(lGetUlong(jep, JB_job_number))));
               lRemoveElem(lGetList(jep, JB_ja_tasks), jatep);   
            }

            /* debit in all layers */
            debit_host_consumable(jep, host_list_locate(Master_Exechost_List,
                     "global"), Master_Complex_List, slots);
            debit_host_consumable(jep, hep = host_list_locate(
                     Master_Exechost_List, lGetHost(qep, QU_qhostname)), 
                     Master_Complex_List, slots);
            debit_queue_consumable(jep, qep, Master_Complex_List, slots);
            if (!master_hep)
               master_hep = hep;
         }

         /* check for resend jobs */
         if (lGetUlong(jatep, JAT_status) == JTRANSFERING) {
            trigger_job_resend(lGetUlong(jatep, JAT_start_time), master_hep, 
                  jid, tid);
         }
      }
   }

   DEXIT;
   return ret;
}

static int reresolve_host(
lListElem *ep,
int nm,
char *object_name,
char *object_dir 
) {
   char *old_name;
   const char *new_name;
   int ret;
   int pos;
   int dataType;

   DENTER(TOP_LAYER, "reresolve_host");


   pos = lGetPosViaElem(ep, nm);
   dataType = lGetPosType(lGetElemDescr(ep),pos);
   if (dataType == lHostT) {
      old_name = strdup(lGetHost(ep, nm));
   } else {
      old_name = strdup(lGetString(ep, nm));
   }
   ret = sge_resolve_host(ep, nm);
   if (ret != CL_OK ) {
      if (ret != COMMD_NACK_UNKNOWN_HOST && ret != COMMD_NACK_TIMEOUT) {
         /* finish qmaster setup only if hostname resolving
            does not work at all generally or a timeout
            indicates that commd itself blocks in resolving
            a host, e.g. when DNS times out */
         ERROR((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS,
                  object_name, old_name, cl_errstr(ret)));
         free(old_name);
         DEXIT;
         return -1;
      }
      WARNING((SGE_EVENT, MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS,
               object_name, old_name));
   }

   /* rename config file if resolving changed name */
   if (dataType == lHostT) {
      new_name = lGetHost(ep, nm);
   } else {
      new_name = lGetString(ep, nm);
   }
   if (strcmp(old_name, new_name)) {
      if (!write_host(1, 2, ep, nm, NULL)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object_name, new_name));
         free(old_name);
         DEXIT;
         return -1;
      }
      sge_unlink(object_dir, old_name);
   }
   free(old_name);

   DEXIT;
   return 0;
}

