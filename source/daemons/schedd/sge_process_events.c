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

/* 
 * This is the main module containing all event handling stuff of the 
 * default scheduler: 
 *    - processing incoming events
 *    - storing the information in data structures 
 *    - then calling scheduler() (via function pointer table) 
 *      which does the real dispatch work                             
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sge_profiling.h"
#include "sge.h"
#include "sge_string.h"
#include "sge_event_client.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_schedd.h"
#include "sge_process_events.h"
#include "sge_prog.h"
#include "sge_ctL.h"
#include "sge_schedd_conf.h"
#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "cull_sort.h"
#include "sge_event.h"
#include "sge_feature.h"
#include "schedd_monitor.h"
#include "unparse_job_cull.h"
#include "sge_dstring.h"
#include "parse_qsubL.h"
#include "sge_access_tree.h"
#include "sge_category.h"
#include "parse.h"
#include "msg_schedd.h"
#include "scheduler.h"
#include "job_log.h"
#include "sge_job.h"
#include "sge_conf.h"
#include "sge_userprj.h"
#include "sge_ckpt.h"
#include "sge_host.h"
#include "sge_userset.h"
#include "sge_centry.h"
#include "sge_sharetree.h"
#include "sge_answer.h"
#include "sge_parse_num_par.h"

/* defined in sge_schedd.c */
extern int shut_me_down;
extern int start_on_master_host;
extern int new_global_config;

bool rebuild_categories = true;
bool rebuild_accesstree = true;

static void sge_rebuild_access_tree(lList *job_list, int trace_running);



lCondition 
      *where_queue = NULL,
      *where_all_queue = NULL,
      *where_job = NULL,
      *where_host = NULL,
      *where_dept = NULL,
      *where_acl = NULL; 


lEnumeration 
   *what_queue = NULL,
   *what_job = NULL,
   *what_host = NULL,
   *what_acl = NULL,
   *what_centry = NULL,
   *what_dept = NULL;

static void ensure_valid_what_and_where(void);


/****** schedd/sge/event_handler_default_scheduler() **************************
*  NAME
*     event_handler_default_scheduler()
*
*  SYNOPSIS
*     int event_handler_default_scheduler(lList *event_list) 
*
*  FUNCTION
*     This event handler is used if "default" scheduler is in effect
*
*  INPUTS
*     lList *event_list - the list of events from qmaster (ET_Type)
*
*  RESULT
*    0 everything was fine
*   -1 inconsistencies with events: register again at qmaster
*    1 configuration changed heavily: register again at qmaster
*    2 got shutdown order from qmaster 
******************************************************************************/
#ifdef SCHEDULER_SAMPLES
int event_handler_my_scheduler() 
{
   return event_handler_default_scheduler();
}
#endif

int event_handler_default_scheduler() 
{
   int ret;
   sge_Sdescr_t copy;
   dstring ds;
   char buffer[128];
   double prof_copy=0, prof_event=0, prof_init=0;
   DENTER(TOP_LAYER, "event_handler_default_scheduler");
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM6);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);


   if (__CONDITION(INFOPRINT)) {
      DPRINTF(("================[SCHEDULING-EPOCH %s]==================\n", 
               sge_at_time(0, &ds)));
   }

   if (rebuild_categories) {
      DPRINTF(("### ### ### ###   REBUILDING CATEGORIES   ### ### ### ###\n"));
      sge_rebuild_job_category(Master_Job_List, Master_Userset_List);
      /* category references are used in the access tree
         so rebuilding categories makes necessary to rebuild
         the access tree */
      rebuild_categories = 0;   
      rebuild_accesstree = 1;
   }

   if (rebuild_accesstree && !sgeee_mode) {
      DPRINTF(("### ### ### ###   REBUILDING ACCESS TREE  ### ### ### ###\n"));
      sge_rebuild_access_tree(Master_Job_List, sconf_get_user_sort());
      rebuild_accesstree = 0;
   }

   if ((ret=sge_before_dispatch())) {
      DEXIT;
      return ret;
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_init = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);

   memset(&copy, 0, sizeof(copy));

   ensure_valid_what_and_where();

   copy.job_list = lSelect("", Master_Job_List,
                           where_job, what_job); 

   /* the scheduler functions have to work with a reduced copy .. */
   copy.host_list = lSelect("", Master_Exechost_List,
                            where_host, what_host);
   copy.queue_list = lSelect("", Master_Queue_List,
                             where_queue, what_queue);

   /* name all queues not suitable for scheduling in tsm-logging */
   copy.all_queue_list = lSelect("", Master_Queue_List,
                                 where_all_queue, what_queue);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      copy.dept_list = lSelect("", Master_Userset_List, where_dept, what_dept);
      copy.acl_list = lSelect("", Master_Userset_List, where_acl, what_acl);
   } else {
      copy.acl_list = lCopyList("", Master_Userset_List);
      copy.dept_list = NULL;
   }

   DTRACE;

   /* .. but not in all cases */
   copy.centry_list = lCopyList("", Master_CEntry_List);
   copy.pe_list = lCopyList("", Master_Pe_List);
   copy.share_tree = lCopyList("", Master_Sharetree_List);
   copy.user_list = lCopyList("", Master_User_List);
   copy.project_list = lCopyList("", Master_Project_List);
   copy.ckpt_list = lCopyList("", Master_Ckpt_List);

   /* report number of reduced and raw (in brackets) lists */
   DPRINTF(("Q:%d(%d), AQ:%d(%d) J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
            "P:%d, CKPT:%d US:%d PR:%d S:nd:%d/lf:%d \n",
            lGetNumberOfElem(copy.queue_list),
            lGetNumberOfElem(Master_Queue_List),
            lGetNumberOfElem(copy.all_queue_list),
            lGetNumberOfElem(Master_Queue_List),
            lGetNumberOfElem(copy.job_list),
            lGetNumberOfElem(Master_Job_List),
            lGetNumberOfElem(copy.host_list),
            lGetNumberOfElem(Master_Exechost_List),

            lGetNumberOfElem(copy.centry_list),
            lGetNumberOfElem(copy.acl_list),
            lGetNumberOfElem(copy.dept_list),
            lGetNumberOfElem(copy.pe_list),
            lGetNumberOfElem(copy.ckpt_list),
            lGetNumberOfElem(copy.user_list),
            lGetNumberOfElem(copy.project_list),
            lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
            lGetNumberOfLeafs(NULL, copy.share_tree, STN_children)
           ));

   if (getenv("SGE_ND")) {
      printf("Q:%d(%d), AQ:%d(%d) J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
         "P:%d, CKPT:%d US:%d PR:%d S:nd:%d/lf:%d \n",
         lGetNumberOfElem(copy.queue_list),
         lGetNumberOfElem(Master_Queue_List),
         lGetNumberOfElem(copy.all_queue_list),
         lGetNumberOfElem(Master_Queue_List),
         lGetNumberOfElem(copy.job_list),
         lGetNumberOfElem(Master_Job_List),
         lGetNumberOfElem(copy.host_list),
         lGetNumberOfElem(Master_Exechost_List),

         lGetNumberOfElem(copy.centry_list),
         lGetNumberOfElem(copy.acl_list),
         lGetNumberOfElem(copy.dept_list),
         lGetNumberOfElem(copy.pe_list),
         lGetNumberOfElem(copy.ckpt_list),
         lGetNumberOfElem(copy.user_list),
         lGetNumberOfElem(copy.project_list),
         lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
         lGetNumberOfLeafs(NULL, copy.share_tree, STN_children)
        );
   } else {
      SCHED_MON((log_string, "-------------START-SCHEDULER-RUN-------------"));
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_copy = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);

/* this is useful when tracing communication of schedd with qmaster */
#define _DONT_TRACE_SCHEDULING
#ifdef DONT_TRACE_SCHEDULING
   {
      monitoring_level tmp;
      rmon_mlcpy(&tmp, &DEBUG_ON);
      rmon_mlclr(&DEBUG_ON);
#endif
      ((default_scheduler_alg_t) sched_funcs[current_scheduler].alg)(&copy);
#ifdef DONT_TRACE_SCHEDULING
      rmon_mlcpy(&DEBUG_ON, &tmp);
   }
#endif
   
   if (getenv("SGE_ND")) {
      printf("--------------STOP-SCHEDULER-RUN-------------");
   } else {
      SCHED_MON((log_string, "--------------STOP-SCHEDULER-RUN-------------"));
   }

   monitor_next_run = 0;

   /* .. which gets deleted after using */
   copy.host_list = lFreeList(copy.host_list);
   copy.queue_list = lFreeList(copy.queue_list);
   copy.all_queue_list = lFreeList(copy.all_queue_list);
   copy.job_list = lFreeList(copy.job_list);
   copy.centry_list = lFreeList(copy.centry_list);
   copy.acl_list = lFreeList(copy.acl_list);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      copy.dept_list = lFreeList(copy.dept_list);
   }

   copy.pe_list = lFreeList(copy.pe_list);
   copy.share_tree = lFreeList(copy.share_tree);
   copy.user_list = lFreeList(copy.user_list);
   copy.project_list = lFreeList(copy.project_list);
   copy.ckpt_list = lFreeList(copy.ckpt_list);
   
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM6);
   prof_event = prof_get_measurement_wallclock(SGE_PROF_CUSTOM6,true, NULL);
 
   if(prof_is_active()){
      u_long32 saved_logginglevel = log_state_get_log_level();
      log_state_set_log_level(LOG_INFO); 

      INFO((SGE_EVENT, "PROF: schedd run took: %.3f s (init: %.3f s, copying lists: %.3f s)\n",
               prof_event, prof_init, prof_copy ));

      log_state_set_log_level(saved_logginglevel);
   }
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------*/
static void ensure_valid_what_and_where(void) 
{
   static int called = 0;

   DENTER(TOP_LAYER, "ensure_valid_what_and_where");
   
   if (called) {
      DEXIT;
      return;
   }

   called = 1;

   if (where_queue == NULL) {
      where_queue = lWhere("%T(%I!=%s "
         "&& !(%I m= %u) "
         "&& !(%I m= %u) "
         "&& !(%I m= %u) "
         "&& !(%I m= %u) "
         "&& !(%I m= %u))",
         QU_Type,    
         QU_qname, SGE_TEMPLATE_NAME, /* do not select queue "template" */
         QU_state, QSUSPENDED,        /* only not suspended queues      */
         QU_state, QSUSPENDED_ON_SUBORDINATE, 
         QU_state, QCAL_SUSPENDED, 
         QU_state, QERROR,            /* no queues in error state       */
         QU_state, QUNKNOWN);         /* only known queues              */
   }

   if (where_queue == NULL) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORQUEUEFAILED));
   }

   DTRACE;

   /* ---------------------------------------- */

   if (where_all_queue == NULL) {
      where_all_queue = lWhere("%T(%I!=%s)", QU_Type,    
            QU_qname, SGE_TEMPLATE_NAME); /* do not select queue "template" */
   }

   if (where_all_queue == NULL) {
      CRITICAL((SGE_EVENT, 
                MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORALLQUEUESFAILED ));
   }

   DTRACE;

   /* ---------------------------------------- */

   if (where_host == NULL) {
      where_host = lWhere("%T(!(%Ic=%s))", EH_Type, EH_name, SGE_TEMPLATE_NAME);
   }
   
   if (where_host == NULL) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORHOSTFAILED ));
   }

   /* ---------------------------------------- */

DTRACE;
   if (!where_dept) {
      where_dept = lWhere("%T(%I m= %u)", US_Type, US_type, US_DEPT);
   }
   
   if (!where_dept) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORDEPTFAILED));
   }   

   /* ---------------------------------------- */

DTRACE;
   if (!where_acl) {
      where_acl = lWhere("%T(%I m= %u)", US_Type, US_type, US_ACL);
   }

   if (!where_acl) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORACLFAILED));
   }   

DTRACE;

   /* ---------------------------------------- */
   if (what_host == NULL) {
      what_host = lWhat("%T(ALL)", EH_Type);
   }

   /* ---------------------------------------- */
   if (what_queue == NULL) {
      what_queue = lWhat("%T(ALL)", QU_Type);
   }

   /* ---------------------------------------- */
   if (what_centry == NULL) { 
      what_centry = lWhat("%T(ALL)", CE_Type);
   }

   /* ---------------------------------------- */
   if (what_acl == NULL) {
      what_acl = lWhat("%T(ALL)", US_Type);
   }

   /* ---------------------------------------- */
   if (what_dept == NULL) {
      what_dept = lWhat("%T(ALL)", US_Type);
   }

   /* ---------------------------------------- */
   if (what_job == NULL) {
#define NM10 "%I%I%I%I%I%I%I%I%I%I"
#define NM5  "%I%I%I%I%I"
#define NM2  "%I%I"
#define NM1  "%I"

      what_job = lWhat("%T(" NM2 NM10 NM10 NM10 NM2")", JB_Type,
/*SGE*/     JB_job_number, 
            JB_script_file,
            JB_submission_time,
            JB_owner,
/*            JB_uid,     */ /* x*/
            JB_group,
/*            JB_gid,     */   /* x*/
            JB_nrunning,
            JB_execution_time,
/*            JB_checkpoint_attr,  */   /* x*/

/*            JB_checkpoint_interval, */ /* x*/
            JB_checkpoint_name,   
            JB_hard_resource_list,
            JB_soft_resource_list,
/*            JB_mail_options,   */ /* may be we want to send mail */ /* x*/
/*            JB_mail_list,  */ /* x*/
/*            JB_job_name,   */ /* x*/
            JB_priority,
            JB_hard_queue_list,
            JB_soft_queue_list,

            JB_master_hard_queue_list,
            JB_pe,
            JB_pe_range,
            JB_jid_predecessor_list,
            JB_soft_wallclock_gmt,
            JB_hard_wallclock_gmt,
            JB_version,
            JB_type,
            JB_project,
            JB_department,
/* SGEEE */
/*            JB_jobclass, */   /*x*/
            JB_deadline,
            JB_host,
            JB_override_tickets,
            JB_ja_structure,
            JB_ja_n_h_ids,
            JB_ja_u_h_ids,
            JB_ja_s_h_ids,
            JB_ja_o_h_ids,   
	         JB_ja_tasks,

/*SGE*/     JB_ja_template,
            JB_category);
   }

   if (what_job == NULL) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORJOBFAILED ));
   }

   DEXIT;
   return;
}

/****** schedd/sge/cleanup_default_scheduler() ********************************
*  NAME
*     cleanup_default_scheduler() -- free resources of default event scheduler
*
*  SYNOPSIS
*     void cleanup_default_scheduler(void) 
*
*  FUNCTION
*     Free all resources allocated by the default event scheduler.
*     This function is called in case the event scheduler changes.
******************************************************************************/
void cleanup_default_scheduler(void)
{
   /* free job sorting access tree */ 
   at_finish();

   /* free job category data */ 
   sge_free_job_category();
}

bool sge_process_schedd_conf_event_after(sge_object_type type, sge_event_action action, 
                                         lListElem *event, void *clientdata){
   sconf_print_config();
   return true;
}

bool 
sge_process_schedd_conf_event_before(sge_object_type type, sge_event_action action, 
                                     lListElem *event, void *clientdata)
{
   const lListElem *old;
   lListElem *new;

   DENTER(TOP_LAYER, "sge_process_schedd_conf_event_before");
   DPRINTF(("callback processing schedd config event\n"));

   old = sconf_get_config(); 
   new = lFirst(lGetList(event, ET_new_version));

   if (new == NULL) {
      ERROR((SGE_EVENT, ">>>>> no scheduler configuration available <<<<<<<<\n"));
      DEXIT;
      return false;
   }

   /* check user_sort: if it changes, rebuild accesstree */
   if (!old || (lGetBool(new, SC_user_sort) != lGetBool(old, SC_user_sort))) {
      rebuild_accesstree = 1;
   }
   
   /* check for valid load formula */ 
   {
      const char *new_load_formula = lGetString(new, SC_load_formula);
      lList *alpp = NULL;

      if (Master_CEntry_List != NULL &&
          !sconf_is_valid_load_formula(new, &alpp, Master_CEntry_List)) {
            ERROR((SGE_EVENT,MSG_INVALID_LOAD_FORMULA, new_load_formula ));
            answer_list_output(&alpp);
            if (old)
               lSetString(new, SC_load_formula, lGetString(old, SC_load_formula) );
            else
               lSetString(new, SC_load_formula, "none");

      }
      else{
         char *copy;  

         int n = strlen(new_load_formula);
         if (n) {
            copy = malloc(n + 1);
            if (copy) {
               strcpy(copy, new_load_formula);
            }

            sge_strip_blanks(copy);
            lSetString(new, SC_load_formula, copy);

            free(copy);
         }
      }
   }

   /* check event client settings */
   {
      const char *time = lGetString(new, SC_schedule_interval); 
      u_long32 schedule_interval;  
      if (extended_parse_ulong_val(NULL, &schedule_interval, TYPE_TIM, time, NULL, 0, 0) ) {
         if (ec_get_edtime() != schedule_interval) {
           ec_set_edtime(schedule_interval);
         }
      }
   }

   if (use_alg(lGetString(new, SC_algorithm))==2) {
      /* changings on event handler or schedule interval can take effect 
       * only after a new registration of schedd at qmaster 
       */
      sge_mirror_shutdown();
      sge_schedd_mirror_register();
      DEXIT;
      return true;
   }

   DEXIT;
   return true;
}

bool 
sge_process_job_event_before(sge_object_type type, sge_event_action action, 
                             lListElem *event, void *clientdata)
{
   u_long32 job_id;
   lListElem *job;

   DENTER(TOP_LAYER, "sge_process_job_event_before");
   DPRINTF(("callback processing job event before default rule\n"));

   if (action == SGE_EMA_DEL || action == SGE_EMA_MOD) {
      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(Master_Job_List, job_id);
      if (job == NULL) {
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL)));
         DEXIT;
         return false;
      }   
   } else {
      DEXIT;
      return true;
   }

   switch (action) {
      case SGE_EMA_DEL:
         {
            lListElem *ja_task;

            /* delete job category if necessary */
            sge_delete_job_category(job);
            if(!sgeee_mode){
               for_each (ja_task, (lGetList(job, JB_ja_tasks))) {
                  u_long32 was_running = running_status(lGetUlong(ja_task, 
                                                     JAT_status));
                  /* decrease # of running jobs for this user */
                  if (was_running && sconf_get_user_sort()) {
                     at_dec_job_counter(lGetUlong(job, JB_priority), 
                                        lGetString(job, JB_owner), 1);
                  }
               }   

                at_unregister_job_array(job);
            }
         }   
         break;

      case SGE_EMA_MOD:
         switch (lGetUlong(event, ET_type)) {
            case sgeE_JOB_MOD:
               /*
                * before changing anything, remove category reference 
                * for unchanged job
                */
               if (!sgeee_mode) {
                  at_unregister_job_array(job);
               }

               sge_delete_job_category(job);
            break;

            case sgeE_JOB_MOD_SCHED_PRIORITY:
               if (!sgeee_mode) {
                  if (sconf_get_user_sort()) {
                     lListElem *ja_task;
                     /* changing priority requires running jobs be accounted in a different
                        priority subtree of the access tree. So the counter in the former priority
                        subtree must be decreased before it can be increased again later on */
                     for_each(ja_task, (lGetList(job, JB_ja_tasks))) {
                        if (running_status(lGetUlong(ja_task, JAT_status)))
                           at_dec_job_counter(lGetUlong(job, JB_priority), lGetString(job, JB_owner), 1);
                     }
                  }
                  at_unregister_job_array(job);
               }
               break;

            default:
            break;
         }
         break;

      default:
         break;
   }

   DEXIT;
   return true;
}

bool sge_process_job_event_after(sge_object_type type, sge_event_action action, 
                                lListElem *event, void *clientdata)
{
   u_long32 job_id = 0;
   lListElem *job  = NULL;

   DENTER(TOP_LAYER, "sge_process_job_event_after");
   DPRINTF(("callback processing job event after default rule\n"));

   if (action == SGE_EMA_ADD || action == SGE_EMA_MOD) {
      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(Master_Job_List, job_id);
      if (job == NULL) {
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL)));
         DEXIT;
         return false;
      }   
   }

   switch (action) {
      case SGE_EMA_LIST:
         rebuild_categories = 1;
         break;

      case SGE_EMA_ADD:
         {
            u_long32 start, end, step;

            /* add job category */
            sge_add_job_category(job, Master_Userset_List);

            if (!sgeee_mode) {
               at_register_job_array(job);
            }

            job_get_submit_task_ids(job, &start, &end, &step);

            if (job_is_array(job)) {
               DPRINTF(("Added job-array "u32"."u32"-"u32":"u32"\n", 
                        job_id, start, end, step));
            } else {
               DPRINTF(("Added job "u32"\n", job_id));
            } 
         }
         break;

      case SGE_EMA_MOD:
         switch (lGetUlong(event, ET_type)) {
            case sgeE_JOB_MOD:
               /*
               ** after changing the job, readd category reference 
               ** for changed job
               */

               sge_add_job_category(job, Master_Userset_List);
               if (!sgeee_mode) {
                  at_register_job_array(job);
               }
               break;

            case sgeE_JOB_FINAL_USAGE:
               {
                  const char *pe_task_id;

                  pe_task_id = lGetString(event, ET_strkey);
                  
                  /* ignore FINAL_USAGE for a pe task here */
                  if (pe_task_id == NULL) {
                     u_long32 ja_task_id;
                     lListElem *ja_task;

                     ja_task_id = lGetUlong(event, ET_intkey2);
                     ja_task = job_search_task(job, NULL, ja_task_id);

                     if (ja_task == NULL) {
                        ERROR((SGE_EVENT, MSG_CANTFINDTASKINJOB_UU, 
                               u32c(ja_task_id), u32c(job_id)));
                        DEXIT;
                        return false;
                     }

                     /* decrease # of running jobs for this user */
                     if (running_status(lGetUlong(ja_task, JAT_status))) {
                        if (!sgeee_mode && sconf_get_user_sort()) {
                           at_dec_job_counter(lGetUlong(job, JB_priority), 
                                              lGetString(job, JB_owner), 1);
                        }   
                     }
                     lSetUlong(ja_task, JAT_status, JFINISHED);
                  }   
               }
               break;
            
            case sgeE_JOB_MOD_SCHED_PRIORITY:
               if (!sgeee_mode) {
                  at_register_job_array(job);
                  if (sconf_get_user_sort()) {
                     lListElem *ja_task;
                     /* increase running counter again in new priority subtree */
                     for_each(ja_task, (lGetList(job, JB_ja_tasks))) {
                        if (running_status(lGetUlong(ja_task, JAT_status)))
                           at_inc_job_counter(lGetUlong(job, JB_priority), lGetString(job, JB_owner), 1);
                     }
                  }
               }
               break;

            default:
               break;
         }
         break;

      default:
         break;
   }

   DEXIT;
   return true;
}


bool 
sge_process_ja_task_event_before(sge_object_type type, 
                                 sge_event_action action, 
                                 lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_ja_task_event_before");
   
   DPRINTF(("callback processing ja_task event before default rule\n"));

   if (action == SGE_EMA_MOD || action == SGE_EMA_DEL) {
      u_long32 job_id, ja_task_id;
      lListElem *job, *ja_task;

      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(Master_Job_List, job_id);
      if (job == NULL) {
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL)));
         DEXIT;
         return false;
      }   

      ja_task_id = lGetUlong(event, ET_intkey2);
      ja_task = job_search_task(job, NULL, ja_task_id);

      if (action == SGE_EMA_MOD) {
         lListElem *new_ja_task;
         u_long32 old_status, new_status;

         if (ja_task == NULL) {
            ERROR((SGE_EVENT, MSG_CANTFINDTASKINJOB_UU, u32c(ja_task_id), 
                   u32c(job_id)));
            DEXIT;
            return false;
         }

         new_ja_task = lFirst(lGetList(event, ET_new_version));
         if (new_ja_task == NULL) {
            ERROR((SGE_EVENT, MSG_NODATAINEVENT));
            DEXIT;
            return false;
         }
          
         old_status = lGetUlong(ja_task, JAT_status); 
         new_status = lGetUlong(new_ja_task, JAT_status); 
         
         if (running_status(new_status)) {
            if (!running_status(old_status)) {
               DPRINTF(("JATASK "u32"."u32": IDLE -> RUNNING\n", 
                        job_id, ja_task_id));
               if (!sgeee_mode && sconf_get_user_sort()) {
                  at_inc_job_counter(lGetUlong(job, JB_priority), 
                                     lGetString(job, JB_owner), 1);
               }
            }   
         } else {
            if (running_status(old_status)) {
               DPRINTF(("JATASK "u32"."u32": RUNNING -> IDLE\n", 
                        job_id, ja_task_id));
               if (!sgeee_mode && sconf_get_user_sort()) {
                  at_dec_job_counter(lGetUlong(job, JB_priority), 
                                     lGetString(job, JB_owner), 1);
               }
            } 
         }   
      } else {
         if (ja_task != NULL) {
            if (running_status(lGetUlong(ja_task, JAT_status)) && 
                !sgeee_mode && 
                sconf_get_user_sort()) {
               at_dec_job_counter(lGetUlong(job, JB_priority), 
                                  lGetString(job, JB_owner), 1);
            }
         }
      }
   }

   DEXIT;
   return true;
}            
/* If the last ja task of a job is deleted, 
 * remove the job category.
 * Do we really need it?
 * Isn't a job delete event sent after the last array task exited?
 */
bool sge_process_ja_task_event_after(sge_object_type type, 
                                    sge_event_action action, 
                                    lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_ja_task_event_after");

   if (action == SGE_EMA_DEL) {
      lListElem *job;
      u_long32 job_id;
      DPRINTF(("callback processing ja_task event after default rule SGE_EMA_DEL\n"));

      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(Master_Job_List, job_id);
      if (job == NULL) {
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL)));
         DEXIT;
         return false;
      }   

      if (job_get_ja_tasks(job) == 0) {
         sge_delete_job_category(job);
      }   
   }
   else
      DPRINTF(("callback processing ja_task event after default rule\n"));

   DEXIT;
   return true;
}

bool sge_process_userset_event_after(sge_object_type type, 
                                     sge_event_action action, 
                                     lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_userset_event");
   DPRINTF(("callback processing userset event after default rule\n"));
   rebuild_categories = 1;
   DEXIT;
   return true;
}

bool sge_process_schedd_monitor_event(sge_object_type type, 
                                     sge_event_action action, 
                                     lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_schedd_monitor_event");
   DPRINTF(("monitoring next scheduler run\n"));
   monitor_next_run = 1;
   DEXIT;
   return true;
}   

bool sge_process_global_config_event(sge_object_type type, 
                                    sge_event_action action, 
                                    lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_global_config_event");
   DPRINTF(("notification about new global configuration\n"));
   new_global_config = 1;
   DEXIT;
   return true;
}   

static void sge_rebuild_access_tree(lList *job_list, int trace_running) 
{
   lListElem *job, *ja_task;

   /* reinit access tree module */
   at_init();

   /* register jobs and # of runnung jobs */
   for_each (job, job_list) {
      at_register_job_array(job);
      if (trace_running) {
         for_each (ja_task, lGetList(job, JB_ja_tasks)) {
            if (running_status(lGetUlong(ja_task, JAT_status))) {
               at_inc_job_counter(lGetUlong(job, JB_priority), 
                                  lGetString(job, JB_owner), 1);      

            }
         }
      }
   }
}

int subscribe_default_scheduler(void)
{
   /* subscribe event types for the mirroring interface */
   sge_mirror_subscribe(SGE_TYPE_CKPT,           NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_CENTRY,         NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_EXECHOST,       NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_SHARETREE,      NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_PROJECT,        NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_PE,             NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_QUEUE,          NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_CQUEUE,         NULL, NULL, NULL);
   sge_mirror_subscribe(SGE_TYPE_USER,           NULL, NULL, NULL);
  
   /* event types with callbacks */

   sge_mirror_subscribe(SGE_TYPE_SCHEDD_CONF,    
                        sge_process_schedd_conf_event_before, NULL, NULL);

   sge_mirror_subscribe(SGE_TYPE_SCHEDD_CONF,    NULL, 
                        sge_process_schedd_conf_event_after,      NULL);
                                                
   sge_mirror_subscribe(SGE_TYPE_SCHEDD_MONITOR, NULL, 
                        sge_process_schedd_monitor_event,   NULL);
                                                
   sge_mirror_subscribe(SGE_TYPE_GLOBAL_CONFIG,  NULL, 
                        sge_process_global_config_event,    NULL);
                                                
   sge_mirror_subscribe(SGE_TYPE_JOB,            sge_process_job_event_before, 
                        sge_process_job_event_after,        NULL);

   sge_mirror_subscribe(SGE_TYPE_JATASK,         sge_process_ja_task_event_before, 
                        sge_process_ja_task_event_after,    NULL);
                                                
   sge_mirror_subscribe(SGE_TYPE_USERSET,        NULL, 
                        sge_process_userset_event_after,    NULL);

   /* set flush parameters for job */
   {
      int temp = sconf_get_flush_submit_sec();
      if (temp == 0)
         ec_set_flush(sgeE_JOB_ADD, -1);        
      else
         ec_set_flush(sgeE_JOB_ADD, temp);
         
      temp = sconf_get_flush_finish_sec();
      if (temp == 0){
         ec_set_flush(sgeE_JOB_DEL,         -1);
         ec_set_flush(sgeE_JOB_FINAL_USAGE, -1);
         ec_set_flush(sgeE_JATASK_MOD, -1);
         ec_set_flush(sgeE_JATASK_DEL, -1);
      }
      else {
         ec_set_flush(sgeE_JOB_DEL,         temp);
         ec_set_flush(sgeE_JOB_FINAL_USAGE, temp);
         ec_set_flush(sgeE_JATASK_MOD, temp);
         ec_set_flush(sgeE_JATASK_DEL, temp);
      }
   }
   /* for some reason we flush sharetree changes */
   ec_set_flush(sgeE_NEW_SHARETREE,   0);

   /* configuration changes and trigger should have immediate effect */
   ec_set_flush(sgeE_GLOBAL_CONFIG,   0);
   ec_set_flush(sgeE_SCHED_CONF,      0);
   ec_set_flush(sgeE_SCHEDDMONITOR,   0);

   return true;
}

