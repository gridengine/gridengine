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
#include <time.h>

#include "sge_bootstrap.h"
#include "sge.h"
#include "sge_conf.h"
#include "commlib.h"
#include "sge_subordinate_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_sched.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_cqueue_qmaster.h"
#include "sge_manop_qmaster.h"
#include "sge_job_qmaster.h"
#include "configuration_qmaster.h"
#include "qmaster_heartbeat.h"
#include "qm_name.h"
#include "sched_conf_qmaster.h"
#include "sge_sharetree.h"
#include "sge_sharetree_qmaster.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_utility.h"
#include "setup.h"
#include "setup_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_host.h"
#include "config_file.h"
#include "sge_qmod_qmaster.h"
#include "time_event.h"
#include "sge_give_jobs.h"
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
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_cqueue.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_sharetree.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"
#include "sge_centry.h"

#include "sge_persistence_qmaster.h"

#include "spool/sge_spooling.h"

#include "msg_common.h"

static int 
remove_invalid_job_references(int user);

static int 
debit_all_jobs_from_qs(void);

/*------------------------------------------------------------*/
int sge_setup_qmaster()
{
   lListElem *jep, *ep, *tmpqep;
   static bool first = true;
   int ret;
   lListElem *lep = NULL;
   char err_str[1024];
   extern int new_config;
   lListElem *spooling_context = NULL;
   lList *answer_list = NULL;
   time_t time_start, time_end;

   DENTER(TOP_LAYER, "sge_setup_qmaster");

   if (first)
      first = false;
   else {
      CRITICAL((SGE_EVENT, MSG_SETUP_SETUPMAYBECALLEDONLYATSTARTUP));
      DEXIT;
      return -1;
   }   

   ret = sge_set_admin_username(bootstrap_get_admin_user(), err_str);
   if (ret == -1) {
      CRITICAL((SGE_EVENT, err_str));
      SGE_EXIT(1);
   }

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_ERROR_CANTSWITCHTOADMINUSER));
      SGE_EXIT(1);
   }

   /* register our error function for use in replace_params() */
   config_errfunc = set_error;

   /*
    * Initialize Master lists and hash tables, if necessary 
    */

/** This is part is making the scheduler a
  * lot slower that it was before. This was an enhancement introduced
  * in cvs revision 1.35
  * It might be added again, when hte hashing problem on large job lists
  * with only a view owners is solved.
  */
#if 0
   if (Master_Job_List == NULL) {
      Master_Job_List = lCreateList("Master_Job_List", JB_Type);
   }
   cull_hash_new(Master_Job_List, JB_owner, 0);
#endif

   if (!sge_initialize_persistence(&answer_list)) {
      answer_list_output(&answer_list);
      DEXIT;
      return -1;
   } else {
      spooling_context = spool_get_default_context();
   }

   /*
   ** get cluster configuration
   */
   spool_read_list(&answer_list, spooling_context, &Master_Config_List, SGE_TYPE_CONFIG);
   answer_list_output(&answer_list);

   ret = select_configuration(uti_state_get_qualified_hostname(), Master_Config_List, &lep);
   if (ret) {
      if (ret == -3)
         WARNING((SGE_EVENT, MSG_CONFIG_FOUNDNOLOCALCONFIGFORQMASTERHOST_S,
                 uti_state_get_qualified_hostname()));
      else {           
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_IS, ret, uti_state_get_qualified_hostname()));
         DEXIT;
         return -1;
      }   
   }
   ret = merge_configuration( lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), lep, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS, ret, uti_state_get_qualified_hostname()));
      DEXIT;
      return -1;
   }
   sge_show_conf();         
   new_config = 1;

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
   sge_mkdir(bootstrap_get_qmaster_spool_dir(), 0755, 1, 0);

   DPRINTF(("chdir("SFQ")----------------------------\n", 
            bootstrap_get_qmaster_spool_dir()));
   sge_chdir_exit(bootstrap_get_qmaster_spool_dir(), 1);

   /* 
   ** we are in the master spool dir now 
   ** log messages into ERR_FILE in master spool dir 
   */
   sge_copy_append(TMP_ERR_FILE_QMASTER, ERR_FILE, SGE_MODE_APPEND);
   sge_switch2start_user();
   unlink(TMP_ERR_FILE_QMASTER);   
   sge_switch2admin_user();
   log_state_set_log_as_admin_user(1);
   log_state_set_log_file(ERR_FILE);

   /* 
   ** increment the heartbeat as early as possible 
   ** and write our name to the act_qmaster file
   ** the lock file will be removed as late as possible
   */
   inc_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE);

   /* 
   ** write our host name to the act_qmaster file 
   */
   if (write_qm_name(uti_state_get_qualified_hostname(), path_state_get_act_qmaster_file(), err_str)) {
      ERROR((SGE_EVENT, "%s\n", err_str));
      SGE_EXIT(1);
   }

   /*
   ** read in all objects and check for correctness
   */
   DPRINTF(("Complex Attributes----------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_CEntry_List, SGE_TYPE_CENTRY);
   answer_list_output(&answer_list);

   DPRINTF(("host_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Exechost_List, SGE_TYPE_EXECHOST);
   spool_read_list(&answer_list, spooling_context, &Master_Adminhost_List, SGE_TYPE_ADMINHOST);
   spool_read_list(&answer_list, spooling_context, &Master_Submithost_List, SGE_TYPE_SUBMITHOST);
   answer_list_output(&answer_list);

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

   DPRINTF(("manager_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Manager_List, SGE_TYPE_MANAGER);
   answer_list_output(&answer_list);
   if (!manop_is_manager("root")) {
      ep = lAddElemStr(&Master_Manager_List, MO_name, "root", MO_Type);

      if (!spool_write_object(&answer_list, spooling_context, ep, "root", SGE_TYPE_MANAGER)) {
         answer_list_output(&answer_list);
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEMANAGERLIST)); 
         DEXIT;
         return -1;
      }
   }
   for_each(ep, Master_Manager_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));

   DPRINTF(("host group definitions-----------\n"));
   spool_read_list(&answer_list, spooling_context, hgroup_list_get_master_list(), 
                   SGE_TYPE_HGROUP);
   answer_list_output(&answer_list);

   DPRINTF(("operator_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Operator_List, SGE_TYPE_OPERATOR);
   answer_list_output(&answer_list);
   if (!manop_is_operator("root")) {
      ep = lAddElemStr(&Master_Operator_List, MO_name, "root", MO_Type);

      if (!spool_write_object(&answer_list, spooling_context, ep, "root", SGE_TYPE_OPERATOR)) {
         answer_list_output(&answer_list);
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEOPERATORLIST)); 
         DEXIT;
         return -1;
      }
   }
   for_each(ep, Master_Operator_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));


   DPRINTF(("userset_list------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Userset_List, SGE_TYPE_USERSET);
   answer_list_output(&answer_list);

   DPRINTF(("calendar list ------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Calendar_List, SGE_TYPE_CALENDAR);
   answer_list_output(&answer_list);

#ifndef __SGE_NO_USERMAPPING__
   DPRINTF(("administrator user mapping-----------\n"));
   spool_read_list(&answer_list, spooling_context, cuser_list_get_master_list(), SGE_TYPE_CUSER);
   answer_list_output(&answer_list);
#endif

   DPRINTF(("cluster_queue_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, object_type_get_master_list(SGE_TYPE_CQUEUE), SGE_TYPE_CQUEUE);
   answer_list_output(&answer_list);
   cqueue_list_set_unknown_state(
            *(object_type_get_master_list(SGE_TYPE_CQUEUE)),
            NULL, false, true);
   
   DPRINTF(("pe_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Pe_List, SGE_TYPE_PE);
   answer_list_output(&answer_list);

   DPRINTF(("ckpt_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Ckpt_List, SGE_TYPE_CKPT);
   answer_list_output(&answer_list);

   DPRINTF(("job_list-----------------------------------\n"));
   /* measure time needed to read job database */
   time_start = time(0);
   spool_read_list(&answer_list, spooling_context, &Master_Job_List, SGE_TYPE_JOB);
   time_end = time(0);
   answer_list_output(&answer_list);

{
   u_long32 saved_logginglevel = log_state_get_log_level();
   log_state_set_log_level(LOG_INFO);
   INFO((SGE_EVENT, "read job database with %d entries in %ld seconds\n", 
         lGetNumberOfElem(Master_Job_List), time_end - time_start));
   log_state_set_log_level(saved_logginglevel);
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
   for_each(tmpqep, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      lList *qinstance_list = lGetList(tmpqep, CQ_qinstances);
      lListElem *qinstance;

      for_each(qinstance, qinstance_list) {
         qinstance_state_set_susp_on_sub(qinstance, false);
      }
   }

   /* 
    * Initialize
    *    - suspend on subordinate state 
    *    - cached QI values.
    */
   for_each(tmpqep, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      cqueue_mod_qinstances(tmpqep, NULL, tmpqep, true);
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
   DPRINTF(("scheduler config -----------------------------------\n"));
   {
      lList *sched_conf=NULL;
      spool_read_list(&answer_list, spooling_context, &sched_conf, SGE_TYPE_SCHEDD_CONF);
      /* JG: TODO: reading the schedd configuration may fail, 
      * as it is not created at install time.
      * The corresponding error message is confusing, so do not output the error.
      * Better: Create config at install time (trough spooldefaults)
      * answer_list_output(&answer_list);
      */
      if (lGetNumberOfElem(sched_conf) == 0) {
         lListElem *ep = sconf_create_default();

         if (sched_conf == NULL) {
            sched_conf = lCreateList("schedd config list", SC_Type);
         }
      
         lAppendElem(sched_conf, ep);
         spool_write_object(&answer_list, spool_get_default_context(), ep, "schedd_conf", SGE_TYPE_SCHEDD_CONF);
         answer_list_output(&answer_list);
      }
      
      if (!sconf_set_config(&sched_conf, &answer_list)){
         answer_list_output(&answer_list);
         lFreeList(answer_list);
         lFreeList(sched_conf);
         DEXIT;
         return -1;
      } 

      /* The REPRIORITIZE parameter of the master configuration is not spooled. It is generated
       *  of out the reprioritze_interval flag in the scheduler. After reading in the scheduler
       *  configuration, we have to update the master configuration.
       */   
       
    {
      lListElem *conf = NULL; 
      lList *ep_list = NULL;
      lListElem *ep = NULL; 
      int reprioritize = (sconf_get_reprioritize_interval() != 0); 
      char value[20];
      conf = lGetElemHost(Master_Config_List, CONF_hname, "global");
      ep_list = lGetList(conf, CONF_entries);

      ep = lGetElemStr(ep_list, CF_name, REPRIORITIZE);
      if (!ep){
         ep = lCreateElem(CF_Type);
         lSetString(ep, CF_name, REPRIORITIZE);
         lAppendElem(ep_list, ep);           
      }
      
      sprintf(value, "%d", reprioritize);
      lSetString(ep, CF_value, value);
      lSetUlong(ep, CF_local, 0);    
   }
      
   }
   if (feature_is_enabled(FEATURE_SGEEE)) {

      /* SGEEE: read user list */
      spool_read_list(&answer_list, spooling_context, &Master_User_List, SGE_TYPE_USER);
      answer_list_output(&answer_list);

      remove_invalid_job_references(1);

      /* SGE: read project list */
      spool_read_list(&answer_list, spooling_context, &Master_Project_List, SGE_TYPE_PROJECT);
      answer_list_output(&answer_list);

      remove_invalid_job_references(0);
   }
   
   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* SGEEE: read share tree */
      spool_read_list(&answer_list, spooling_context, &Master_Sharetree_List, SGE_TYPE_SHARETREE);
      answer_list_output(&answer_list);
      ep = lFirst(Master_Sharetree_List);
      if (ep) {
         lList *alp = NULL;
         lList *found = NULL;
         ret = check_sharetree(&alp, ep, Master_User_List, Master_Project_List, 
               NULL, &found);
         found = lFreeList(found);
         alp = lFreeList(alp); 
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
         lList *answer_list = NULL;
         spool_write_object(&answer_list, spool_get_default_context(), up, 
                            lGetString(up, UP_name), user ? SGE_TYPE_USER : 
                                                            SGE_TYPE_PROJECT);
         answer_list_output(&answer_list);
      }
   }

   DEXIT;
   return 0;
}

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
            
            if (!(qep = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), queue_name))) {
               ERROR((SGE_EVENT, MSG_CONFIG_CANTFINDQUEUEXREFERENCEDINJOBY_SU,  
                  queue_name, u32c(lGetUlong(jep, JB_job_number))));
               lRemoveElem(lGetList(jep, JB_ja_tasks), jatep);   
            }

            /* debit in all layers */
            debit_host_consumable(jep, host_list_locate(Master_Exechost_List,
                                  "global"), Master_CEntry_List, slots);
            debit_host_consumable(jep, hep = host_list_locate(
                     Master_Exechost_List, lGetHost(qep, QU_qhostname)), 
                     Master_CEntry_List, slots);
            qinstance_debit_consumable(jep, qep, Master_CEntry_List, slots);
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

