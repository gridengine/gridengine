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
#include "sge_conf.h"
#include "commlib.h"
#include "subordinate_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_sched.h"
#include "sge_all_listsL.h"
#include "read_list.h"
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
#include "gdi_utility.h"
#include "setup_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_host.h"
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

static int remove_invalid_job_references(int user);

static int debit_all_jobs_from_qs(void);

/*------------------------------------------------------------*/
int sge_setup_qmaster()
{
   lListElem *jep, *ep, *tmpqep;
   static int first = TRUE;
   int ret;
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
    * Initialize Master lists and hash tables, if necessary 
    */
   if(Master_Job_List == NULL) {
      Master_Job_List = lCreateList("Master_Job_List", JB_Type);
   }
   cull_hash_new(Master_Job_List, JB_owner, 0);

   /*
   ** get cluster configuration
   */
   read_all_configurations(&Master_Config_List, 
                           path.conf_file, path.local_conf_dir);
   ret = select_configuration(uti_state_get_qualified_hostname(), Master_Config_List, &lep);
   if (ret) {
      if (ret == -3)
         WARNING((SGE_EVENT, MSG_CONFIG_FOUNDNOLOCALCONFIGFORQMASTERHOST_S,
                 uti_state_get_qualified_hostname()));
      else {           
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_IS, ret, uti_state_get_qualified_hostname()));
         return -1;
      }   
   }
   ret = merge_configuration( lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), lep, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS, ret, uti_state_get_qualified_hostname()));
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
   sge_chdir_exit("/", 1);

   DPRINTF(("Making directories----------------------------\n"));
   sge_mkdir(conf.qmaster_spool_dir, 0755, 1);

   DPRINTF(("chdir("SFQ")----------------------------\n", conf.qmaster_spool_dir));
   sge_chdir_exit(conf.qmaster_spool_dir, 1);

   /* 
   ** we are in the master spool dir now 
   ** log messages into ERR_FILE in master spool dir 
   */
   sge_copy_append(TMP_ERR_FILE_QMASTER, ERR_FILE, SGE_MODE_APPEND);
   sge_switch2start_user();
   unlink(TMP_ERR_FILE_QMASTER);   
   sge_switch2admin_user();
   sge_log_set_auser(1);
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
   if (write_qm_name(uti_state_get_qualified_hostname(), path.act_qmaster_file, err_str)) {
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
   if (!host_list_locate(Master_Adminhost_List, uti_state_get_qualified_hostname())) {
      if (sge_add_host_of_type(uti_state_get_qualified_hostname(), SGE_ADMINHOST_LIST)) {
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
   if (!manop_is_manager("root")) {
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
   if (!manop_is_operator("root")) {
      lAddElemStr(&Master_Operator_List, MO_name, "root", MO_Type);

      if (write_manop(1, SGE_OPERATOR_LIST)) {
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEOPERATORLIST)); 
         return -1;
      }
   }
   for_each(ep, Master_Operator_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));


   DPRINTF(("userset_list------------------------------\n"));
   if (sge_read_userset_list_from_disk()) {
      DEXIT;
      return -1;
   }

   DPRINTF(("calendar list ------------------------------\n"));
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
   if (sge_read_queue_list_from_disk()) {
     DEXIT;
     return -1;
   }


   DPRINTF(("pe_list---------------------------------\n"));
   if (sge_read_pe_list_from_disk()) {
      DEXIT;
      return -1;
   }

   DPRINTF(("ckpt_list---------------------------------\n"));
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
    
   {
      char common_dir[SGE_PATH_MAX];
      sprintf(common_dir, "%s"PATH_SEPARATOR"%s", path.cell_root, COMMON_DIR); 
      Master_Sched_Config_List = read_sched_configuration(common_dir, path.sched_conf_file, 1, &alp);
   }
   if (!Master_Sched_Config_List) {
      ERROR((SGE_EVENT, "%s\n", lGetString(lFirst(alp), AN_text)));
      DEXIT;
      return -1;
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {

      sge_mkdir(USER_DIR, 0755, 1);
      sge_mkdir(PROJECT_DIR, 0755, 1);

      /* SGEEE: read user list */
      if (sge_read_user_list_from_disk()) {
         DEXIT;
         return -1;
      }

      remove_invalid_job_references(1);

      /* SGE: read project list */
      if (sge_read_project_list_from_disk()) {
         DEXIT;
         return -1;
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

