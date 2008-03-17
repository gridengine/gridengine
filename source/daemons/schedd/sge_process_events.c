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
#include "sge_category.h"
#include "parse.h"
#include "msg_schedd.h"
#include "scheduler.h"
#include "sge_job.h"
#include "sge_conf.h"
#include "sge_userprj.h"
#include "sge_ckpt.h"
#include "sge_host.h"
#include "sge_userset.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_sharetree.h"
#include "sge_answer.h"
#include "sge_parse_num_par.h"
#include "sge_qinstance_state.h"
#include "sgeee.h"


/* defined in sge_schedd.c */
extern int shut_me_down;
extern int start_on_master_host;
extern int new_global_config;

static bool rebuild_categories = true;

const lCondition 
      *where_queue = NULL,
      *where_queue2 = NULL,
      *where_all_queue = NULL,
      *where_cqueue = NULL,
      *where_job = NULL,
      *where_host = NULL,
      *where_dept = NULL,
      *where_acl = NULL,
      *where_jat = NULL;


const lEnumeration 
   *what_queue = NULL,
   *what_queue2 = NULL,
   *what_cqueue = NULL,
   *what_job = NULL,
   *what_host = NULL,
   *what_acl = NULL,
   *what_centry = NULL,
   *what_dept = NULL,
   *what_jat = NULL,
   *what_pet = NULL;

static void ensure_valid_what_and_where(void);

/* callback functions for event processing */
static sge_callback_result
sge_process_schedd_conf_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                     sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_schedd_conf_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                    sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_job_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                             sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_job_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                            sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_ja_task_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                 sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_ja_task_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_global_config_event(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_schedd_monitor_event(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                 sge_event_action action, lListElem *event, void *clientdata);

static sge_callback_result
sge_process_project_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type,
                                     sge_event_action action, lListElem *event, void *clientdata);

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
int event_handler_my_scheduler(sge_evc_class_t *evc) 
{
   return event_handler_default_scheduler(evc);
}
#endif

int event_handler_default_scheduler(sge_evc_class_t *evc) 
{
   sge_Sdescr_t copy;
   dstring ds;
   char buffer[128];
   lList *orders = NULL;
   double prof_copy=0, prof_event=0, prof_init=0, prof_free=0, prof_run=0;
   lList *master_job_list = *object_type_get_master_list(SGE_TYPE_JOB);
   lList *master_userset_list = *object_type_get_master_list(SGE_TYPE_USERSET);
   lList *master_project_list = *object_type_get_master_list(SGE_TYPE_PROJECT);
   lList *master_exechost_list= *object_type_get_master_list(SGE_TYPE_EXECHOST);
   lList *master_rqs_list= *object_type_get_master_list(SGE_TYPE_RQS);
   
   DENTER(GDI_LAYER, "event_handler_default_scheduler");
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM6);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);


   if (__CONDITION(INFOPRINT)) {
      DPRINTF(("================[SCHEDULING-EPOCH %s]==================\n", 
               sge_at_time(0, &ds)));
   }

   if (rebuild_categories) {
      DPRINTF(("### ### ### ###   REBUILDING CATEGORIES   ### ### ### ###\n"));
      sge_rebuild_job_category(master_job_list, 
            master_userset_list, 
            master_project_list,
            master_rqs_list);
      /* category references are used in the access tree
         so rebuilding categories makes necessary to rebuild
         the access tree */
      rebuild_categories = false;   
   }

   sge_before_dispatch(evc);

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_init = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);

   memset(&copy, 0, sizeof(copy));

   ensure_valid_what_and_where();

   /* the scheduler functions have to work with a reduced copy .. */
   copy.host_list = lSelect("", master_exechost_list, where_host, what_host);
   /* 
    * Within the scheduler we do only need QIs
    */

   {
      lList *master_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
      lListElem *cqueue = NULL;

      for_each(cqueue, master_list) {
         lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
         lListElem *queue = NULL;

         if (copy.all_queue_list == NULL) {
            copy.all_queue_list = lCreateList("qi", lGetListDescr(qinstance_list));
         }

         for_each(queue, qinstance_list) {
            lListElem *ep = lCopyElem(queue);
            lAppendElem(copy.all_queue_list, ep);
         }
      }
      copy.queue_list = lSelect("sel_qi_list", copy.all_queue_list, where_queue, what_queue2);
      copy.dis_queue_list = lSelect("dis_qi_list", copy.all_queue_list, where_queue2, what_queue2);
   }


   if (sconf_is_job_category_filtering()) {
      copy.job_list = sge_category_job_copy(copy.queue_list, &orders); 
   }
   else {
      copy.job_list = lCopyList("test", master_job_list);                         
   }   

   copy.dept_list = lSelect("", master_userset_list, where_dept, what_dept);
   copy.acl_list = lSelect("", master_userset_list, where_acl, what_acl);

   /* .. but not in all cases */
   copy.centry_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_CENTRY));
   copy.pe_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_PE));
   copy.share_tree = lCopyList("", *object_type_get_master_list(SGE_TYPE_SHARETREE));
   copy.user_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_USER));
   copy.project_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_PROJECT));
   copy.ckpt_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_CKPT));
   copy.hgrp_list = lCopyList("", *object_type_get_master_list(SGE_TYPE_HGROUP));
   copy.rqs_list = lCopyList("", master_rqs_list);

   /* report number of reduced and raw (in brackets) lists */
   DPRINTF(("Q:%d, AQ:%d J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
            "P:%d, CKPT:%d, US:%d, PR:%d, RQS:%d, S:nd:%d/lf:%d \n",
            lGetNumberOfElem(copy.queue_list),
            lGetNumberOfElem(copy.all_queue_list),
            lGetNumberOfElem(copy.job_list),
            lGetNumberOfElem(master_job_list),
            lGetNumberOfElem(copy.host_list),
            lGetNumberOfElem(master_exechost_list),

            lGetNumberOfElem(copy.centry_list),
            lGetNumberOfElem(copy.acl_list),
            lGetNumberOfElem(copy.dept_list),
            lGetNumberOfElem(copy.pe_list),
            lGetNumberOfElem(copy.ckpt_list),
            lGetNumberOfElem(copy.user_list),
            lGetNumberOfElem(copy.project_list),
            lGetNumberOfElem(copy.rqs_list),
            lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
            lGetNumberOfLeafs(NULL, copy.share_tree, STN_children)
           ));

   if (getenv("SGE_ND")) {
      printf("Q:%d, AQ:%d J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
         "P:%d, CKPT:%d, US:%d, PR:%d, RQS:%d, S:nd:%d/lf:%d \n",
         lGetNumberOfElem(copy.queue_list),
         lGetNumberOfElem(copy.all_queue_list),
         lGetNumberOfElem(copy.job_list),
         lGetNumberOfElem(master_job_list),
         lGetNumberOfElem(copy.host_list),
         lGetNumberOfElem(master_exechost_list),

         lGetNumberOfElem(copy.centry_list),
         lGetNumberOfElem(copy.acl_list),
         lGetNumberOfElem(copy.dept_list),
         lGetNumberOfElem(copy.pe_list),
         lGetNumberOfElem(copy.ckpt_list),
         lGetNumberOfElem(copy.user_list),
         lGetNumberOfElem(copy.project_list),
         lGetNumberOfElem(copy.rqs_list),
         lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
         lGetNumberOfLeafs(NULL, copy.share_tree, STN_children)
        );
   } else {
      schedd_log("-------------START-SCHEDULER-RUN-------------");
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_copy = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);
/* this is useful when tracing communication of schedd with qmaster */
#define _DONT_TRACE_SCHEDULING
#ifdef DONT_TRACE_SCHEDULING
   {
      monitoring_level tmp;
      rmon_mlcpy(&tmp, &RMON_DEBUG_ON);
      rmon_mlclr(&RMON_DEBUG_ON);
#endif
      ((default_scheduler_alg_t) sched_funcs[current_scheduler].alg)(evc, &copy, &orders);
#ifdef DONT_TRACE_SCHEDULING
      rmon_mlcpy(&RMON_DEBUG_ON, &tmp);
   }
#endif
   
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_run = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM7);
   
   schedd_set_monitor_next_run(false);

   /* .. which gets deleted after using */
   lFreeList(&(copy.host_list));
   lFreeList(&(copy.queue_list));
   lFreeList(&(copy.dis_queue_list));
   lFreeList(&(copy.all_queue_list));
   lFreeList(&(copy.job_list));
   lFreeList(&(copy.centry_list));
   lFreeList(&(copy.acl_list));

   lFreeList(&(copy.dept_list));

   lFreeList(&(copy.pe_list));
   lFreeList(&(copy.share_tree));
   lFreeList(&(copy.user_list));
   lFreeList(&(copy.project_list));
   lFreeList(&(copy.ckpt_list));
   lFreeList(&(copy.hgrp_list));
   lFreeList(&(copy.rqs_list));

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM7);
   prof_free = prof_get_measurement_wallclock(SGE_PROF_CUSTOM7,true, NULL);

   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM6);
   prof_event = prof_get_measurement_wallclock(SGE_PROF_CUSTOM6,true, NULL);
 
   if(prof_is_active(SGE_PROF_CUSTOM6)){
      PROFILING((SGE_EVENT, "PROF: schedd run took: %.3f s (init: %.3f s, copy: %.3f s, run:%.3f, free: %.3f s, jobs: %d, categories: %d/%d)",
            prof_event, prof_init, prof_copy, prof_run, prof_free, lGetNumberOfElem(master_job_list), sge_category_count(), sge_cs_category_count() ));

   }

   if (getenv("SGE_ND") != NULL) {
      printf("--------------STOP-SCHEDULER-RUN-------------\n");
   } else {
      schedd_log("--------------STOP-SCHEDULER-RUN-------------");
   }
   
   DRETURN(0);
}


/*-------------------------------------------------------------------*/
static void ensure_valid_what_and_where(void) 
{
   static int called = 0;

   DENTER(GDI_LAYER, "ensure_valid_what_and_where");
   
   if (called) {
      DEXIT;
      return;
   }

   called = 1;

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
   
   /* ---------------------------------------- */
   if (what_host == NULL) {
      what_host = lWhat("%T(ALL)", EH_Type);
   }
   
DTRACE;
   /* ---------------------------------------- */
   if (what_queue == NULL) {
      
      lDescr *queue_des = NULL;
      int index = 0;
      int n = 0;
      
      const int cqueue_nm[] = {         
         CQ_name,  
         CQ_hostlist,
         CQ_qinstances,
         CQ_consumable_config_list,
         CQ_projects,
         CQ_xprojects,
         CQ_qtype,
         CQ_pe_list,
         CQ_nsuspend,
         CQ_job_slots,
         NoName
      };

      const int queue_nm[] = {
         QU_full_name,
         QU_qhostname,
         QU_tag,
         QU_qname,
         QU_acl,
         QU_xacl,
         QU_projects,
         QU_xprojects,
         QU_resource_utilization,
         QU_job_slots,
         QU_load_thresholds,
         QU_suspend_thresholds,
         QU_host_seq_no,
         QU_seq_no,
         QU_state,
         QU_tagged4schedule,
         QU_nsuspend,
         QU_suspend_interval,
         QU_consumable_config_list,
         QU_available_at,
         QU_soft_violation,
         QU_version,
         QU_subordinate_list,

         QU_qtype,
         QU_calendar,
         QU_s_rt,
         QU_h_rt,
         QU_s_cpu,
         QU_h_cpu,
         QU_s_fsize,
         QU_h_fsize,
         QU_s_data,
         QU_h_data,
         QU_s_stack,
         QU_h_stack,
         QU_s_core,
         QU_h_core,
         QU_s_rss,
         QU_h_rss,
         QU_s_vmem,
         QU_h_vmem,
         QU_min_cpu_interval,
         QU_notify,

         QU_suspended_on_subordinate,
         QU_last_suspend_threshold_ckeck,
         QU_job_cnt,
         QU_pending_job_cnt,
         QU_pe_list,
         QU_ckpt_list,

         QU_state_changes,

         NoName
      };
   
      what_cqueue = lIntVector2What(CQ_Type,cqueue_nm);
      what_queue  = lIntVector2What(QU_Type,queue_nm);

      /* create new lList with partial descriptor */
      if ((n = lCountWhat(what_queue, QU_Type)) <= 0) {
         CRITICAL((SGE_EVENT, "empty descriptor\n"));
      }
      
      if (!(queue_des = (lDescr *) malloc(sizeof(lDescr) * (n + 1)))) {
         CRITICAL((SGE_EVENT, "error memory allocation\n")); 
      }
      if (lPartialDescr(what_queue, QU_Type, queue_des, &index) != 0){
         CRITICAL((SGE_EVENT, "partial queue descriptor failed\n")); 
      }
      else {
         what_queue2 = lWhat("%T(ALL)", queue_des);

         where_queue = lWhere("%T("
            " !(%I m= %u) &&" 
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u))",
            queue_des,    
            QU_state, QI_SUSPENDED,        /* only not suspended queues      */
            QU_state, QI_SUSPENDED_ON_SUBORDINATE, 
            QU_state, QI_CAL_SUSPENDED, 
            QU_state, QI_ERROR,            /* no queues in error state       */
            QU_state, QI_UNKNOWN,
            QU_state, QI_AMBIGUOUS,
            QU_state, QI_ORPHANED
            );         /* only known queues              */
           
         where_queue2 = lWhere("%T("
            "  (%I m= %u) &&" 
            " !(%I m= %u) &&" 
            " !(%I m= %u) &&" 
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u) &&"
            " !(%I m= %u))",
            queue_des,    
            QU_state, QI_CAL_SUSPENDED, 
            QU_state, QI_CAL_DISABLED,
            
            QU_state, QI_SUSPENDED,        /* only not suspended queues      */
            QU_state, QI_SUSPENDED_ON_SUBORDINATE, 
            QU_state, QI_ERROR,            /* no queues in error state       */
            QU_state, QI_UNKNOWN,
            QU_state, QI_DISABLED,
            QU_state, QI_AMBIGUOUS,
            QU_state, QI_ORPHANED
            );         /* only known queues              */

         if (where_queue == NULL) {
            CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORQUEUEFAILED));
         }
       
         cull_hash_free_descr(queue_des);
         free(queue_des);
       
         DTRACE;
         
        /* ---------------------------------------- */

         where_all_queue = lWhere("%T(%I!=%s)", QU_Type,    
                  QU_qname, SGE_TEMPLATE_NAME); /* do not select queue "template" */

         if (where_all_queue == NULL) {
            CRITICAL((SGE_EVENT, 
                      MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORALLQUEUESFAILED ));
         }

         DTRACE;
      }
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
      const int job_nm[] = {         
            JB_job_number,
            JB_category,
            JB_hard_queue_list,
            JB_owner,
            JB_hard_resource_list,
            JB_group,
            JB_ja_n_h_ids,
            JB_soft_resource_list,
            JB_ja_template,
            JB_soft_queue_list,
            JB_type,
            JB_ja_u_h_ids,
            JB_ja_s_h_ids,
            JB_ja_o_h_ids,
            JB_pe,
            JB_project,
            JB_department,
            JB_execution_time,
            JB_override_tickets,
            JB_jid_predecessor_list,
            JB_deadline,
            JB_submission_time,
            JB_checkpoint_name,
            JB_version,
            JB_priority,
            JB_host,
            JB_ja_structure,
            JB_jobshare,
            JB_master_hard_queue_list,
            JB_pe_range,
            JB_nppri,
            JB_urg,
            JB_nurg,
            JB_dlcontr,
            JB_wtcontr,
            JB_rrcontr,
            JB_script_file,
            JB_soft_wallclock_gmt,
            JB_hard_wallclock_gmt,
            JB_reserve,
            JB_ja_tasks,
            NoName
         };
  
      what_job =  lIntVector2What(JB_Type, job_nm);
   }

   if (what_job == NULL) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORJOBFAILED ));
   }
   
/**
 * The filtern does not work so easy. I am not sure, how 
 * the jat structures are created and submitted. But
 * if these are filtered, one gets a mixture of full
 * and reduced elements in the same list.
 */

   /* ---------------------------------------- */

   if (what_jat == NULL) {
  
      const int jat_nm[] = {         
         JAT_task_number,
         JAT_tix,
         JAT_state,
         JAT_fshare,
         JAT_status,
         JAT_granted_pe,
         JAT_scaled_usage_list,
         JAT_task_list,
         JAT_start_time,
         JAT_hold,
         JAT_granted_destin_identifier_list,
         JAT_master_queue,
         JAT_oticket,
         JAT_fticket,
         JAT_sticket,
         JAT_share,
         JAT_prio,
         JAT_ntix,
         NoName
      };
 
      what_jat = lIntVector2What(JAT_Type, jat_nm);
   }

   if (what_pet == NULL) {
  
      const int pet_nm[] = {         
         PET_id, 
         PET_status,     
         PET_granted_destin_identifier_list,
         PET_usage,
         PET_scaled_usage,
         PET_previous_usage,
         NoName
      };
 
      what_pet = lIntVector2What(PET_Type, pet_nm);
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
void cleanup_default_scheduler(sge_evc_class_t *evc)
{
   /* free job category data */ 
   sge_free_job_category();
}

static sge_callback_result
sge_process_schedd_conf_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                    sge_event_action action, lListElem *event, void *clientdata){
   sconf_print_config();

   return SGE_EMA_OK;
}

static sge_callback_result
sge_process_schedd_conf_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                     sge_event_action action, lListElem *event, void *clientdata)
{
   lListElem *new = NULL;

   DENTER(GDI_LAYER, "sge_process_schedd_conf_event_before");

   DPRINTF(("callback processing schedd config event\n"));

   new = lFirst(lGetList(event, ET_new_version));

   evc->ec_set_busy(evc, 1);

   if (new == NULL) {
      ERROR((SGE_EVENT, "> > > > > no scheduler configuration available < < < < <\n"));
      DEXIT;
      return SGE_EMA_FAILURE;
   }
   /* check for valid load formula */ 
   {
      lListElem *old = sconf_get_config(); 
      const char *new_load_formula = lGetString(new, SC_load_formula);
      lList *alpp = NULL;
      lList *master_centry_list = *sge_master_list(object_base, SGE_TYPE_CENTRY);

      if (master_centry_list != NULL &&
          !validate_load_formula(new_load_formula, &alpp, master_centry_list, SGE_ATTR_LOAD_FORMULA)) {
            
         ERROR((SGE_EVENT,MSG_INVALID_LOAD_FORMULA, new_load_formula ));
         answer_list_output(&alpp);
         if (old) {
            lSetString(new, SC_load_formula, lGetString(old, SC_load_formula) );
         }   
         else {
            lSetString(new, SC_load_formula, "none");
         }   
      }
      else {
         int n = strlen(new_load_formula);

         if (n > 0) {
            char *copy = NULL;  

            copy = malloc(n + 1);
            if (copy != NULL) {
               strcpy(copy, new_load_formula);

               sge_strip_blanks(copy);
               lSetString(new, SC_load_formula, copy);
            }
            
            FREE(copy);
         }
      }
      lFreeElem(&old);
   }

   if (use_alg(lGetString(new, SC_algorithm))==2) {
      /* changings on event handler or schedule interval can take effect 
       * only after a new registration of schedd at qmaster 
       */
      sge_mirror_shutdown(evc);
      sge_schedd_mirror_register(evc);
      DEXIT;
      return SGE_EMA_OK;
   }

   DEXIT;
   return SGE_EMA_OK;
}

static sge_callback_result
sge_process_job_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                             sge_event_action action, lListElem *event, void *clientdata)
{
   u_long32 job_id = 0;
   lListElem *job = NULL;

   DENTER(GDI_LAYER, "sge_process_job_event_before");
   DPRINTF(("callback processing job event before default rule\n"));

   if (action == SGE_EMA_DEL || action == SGE_EMA_MOD) {
      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(*object_type_get_master_list(SGE_TYPE_JOB), job_id);
      if (job == NULL) {
         dstring id_dstring = DSTRING_INIT;
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL, &id_dstring)));
         sge_dstring_free(&id_dstring);
         DEXIT;
         return SGE_EMA_FAILURE;
      }   
   } else {
      DEXIT;
      return SGE_EMA_OK;
   }
   
   switch (action) {
      case SGE_EMA_DEL:
         {
            /* delete job category if necessary */
            sge_delete_job_category(job);
         }   
         break;

      case SGE_EMA_MOD:
         switch (lGetUlong(event, ET_type)) {
            case sgeE_JOB_MOD:
               sge_delete_job_category(job);
            break;

            case sgeE_JOB_MOD_SCHED_PRIORITY:
               break;

            default:
            break;
         }
         break;

      default:
         break;
   }

   DEXIT;
   return SGE_EMA_OK;
}

static sge_callback_result
sge_process_job_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                            sge_event_action action, lListElem *event, void *clientdata)
{
   u_long32 job_id = 0;
   lListElem *job  = NULL;

   DENTER(GDI_LAYER, "sge_process_job_event_after");
   DPRINTF(("callback processing job event after default rule\n"));

   if (action == SGE_EMA_ADD || action == SGE_EMA_MOD) {
      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(*sge_master_list(object_base, SGE_TYPE_JOB), job_id);
      if (job == NULL) {
         dstring id_dstring = DSTRING_INIT;
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL, &id_dstring)));
         sge_dstring_free(&id_dstring);
         DEXIT;
         return SGE_EMA_FAILURE;
      }   
      sge_do_priority_job(job); /* job got added or modified, recompute the priorities */
   }
   
   switch (action) {
      case SGE_EMA_LIST:
         rebuild_categories = true;
         sge_do_priority(*object_type_get_master_list(SGE_TYPE_JOB), NULL); /* recompute the priorities */
         break;

      case SGE_EMA_ADD:
         {
            u_long32 start, end, step;

            /* add job category */
            sge_add_job_category(job,
                                 *object_type_get_master_list(SGE_TYPE_USERSET),
                                 *object_type_get_master_list(SGE_TYPE_PROJECT),
                                 *object_type_get_master_list(SGE_TYPE_RQS));

            job_get_submit_task_ids(job, &start, &end, &step);

            if (job_is_array(job)) {
               DPRINTF(("Added job-array "sge_u32"."sge_u32"-"sge_u32":"sge_u32"\n", 
                        job_id, start, end, step));
            } else {
               DPRINTF(("Added job "sge_u32"\n", job_id));
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

               sge_add_job_category(job,
                                    *object_type_get_master_list(SGE_TYPE_USERSET),
                                    *object_type_get_master_list(SGE_TYPE_PROJECT),
                                    *object_type_get_master_list(SGE_TYPE_RQS));
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
                               sge_u32c(ja_task_id), sge_u32c(job_id)));
                        DEXIT;
                        return SGE_EMA_FAILURE;
                     }

                     lSetUlong(ja_task, JAT_status, JFINISHED);
                  }   
               }
               break;
            
            case sgeE_JOB_MOD_SCHED_PRIORITY:
               break;

            default:
               break;
         }
         break;

      default:
         break;
   }

   DEXIT;
   return SGE_EMA_OK;
}


static sge_callback_result
sge_process_ja_task_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                 sge_event_action action, lListElem *event, void *clientdata)
{
   DENTER(GDI_LAYER, "sge_process_ja_task_event_before");
   
   DPRINTF(("callback processing ja_task event before default rule\n"));

   DEXIT;
   return SGE_EMA_OK;
}  

static sge_callback_result
sge_process_global_config_event(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                sge_event_action action, lListElem *event, void *clientdata)
{
   DENTER(TOP_LAYER, "sge_process_global_config_event");
   DPRINTF(("notification about new global configuration\n"));
   new_global_config = 1;
   DEXIT;
   return SGE_EMA_OK;
}   

/* If the last ja task of a job is deleted, 
 * remove the job category.
 * Do we really need it?
 * Isn't a job delete event sent after the last array task exited?
 */
static sge_callback_result
sge_process_ja_task_event_after(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                sge_event_action action, lListElem *event, void *clientdata)
{
   DENTER(GDI_LAYER, "sge_process_ja_task_event_after");

   if (action == SGE_EMA_DEL) {
      lListElem *job;
      u_long32 job_id;
      DPRINTF(("callback processing ja_task event after default rule SGE_EMA_DEL\n"));

      job_id = lGetUlong(event, ET_intkey);
      job = job_list_locate(*sge_master_list(object_base, SGE_TYPE_JOB), job_id);
      if (job == NULL) {
         dstring id_dstring = DSTRING_INIT;
         ERROR((SGE_EVENT, MSG_CANTFINDJOBINMASTERLIST_S, 
                job_get_id_string(job_id, 0, NULL, &id_dstring)));
         sge_dstring_free(&id_dstring);
         DEXIT;
         return SGE_EMA_FAILURE;
      }   
   }
   else
      DPRINTF(("callback processing ja_task event after default rule\n"));

   DEXIT;
   return SGE_EMA_OK;
}

/****** sge_process_events/sge_process_project_event_before() ******************
*  NAME
*     sge_process_project_event_before() -- ???
*
*  SYNOPSIS
*     bool sge_process_project_event_before(sge_object_type type,
*     sge_event_action action, lListElem *event, void *clientdata)
*
*  FUNCTION
*     Determine whether categories need to be rebuilt. Rebuilding
*     categories is necessary, if a project (a) gets used first
*     time as ACL or (b) is no longer used as ACL.
*
*  NOTES
*     MT-NOTE: sge_process_project_event_before() is not MT safe
*******************************************************************************/
static sge_callback_result
sge_process_project_event_before(sge_evc_class_t *evc, object_description *object_base, sge_object_type type,
                                     sge_event_action action, lListElem *event, void *clientdata)
{
   const lListElem *new, *old;
   const char *p;

   DENTER(TOP_LAYER, "sge_process_project_event_before");

   if (action != SGE_EMA_ADD &&
       action != SGE_EMA_MOD &&
       action != SGE_EMA_DEL) {
      DEXIT;
      return SGE_EMA_OK;
   }

   p = lGetString(event, ET_strkey);
   new = lFirst(lGetList(event, ET_new_version));
   old = userprj_list_locate(*sge_master_list(object_base, SGE_TYPE_PROJECT), p);

   switch (action) {
   case SGE_EMA_ADD:
      if (lGetBool(new, UP_consider_with_categories) == true) {
         rebuild_categories = true;
         DPRINTF(("callback before project event: rebuild categories due to SGE_EMA_ADD(%s)\n", p));
      }
      break;
   case SGE_EMA_MOD:
      if (lGetBool(new, UP_consider_with_categories) != lGetBool(old, UP_consider_with_categories)) {
         rebuild_categories = true;
         DPRINTF(("callback before project event: rebuild categories due to SGE_EMA_MOD(%s)\n", p));
      }
      break;
   case SGE_EMA_DEL:
      if (lGetBool(old, UP_consider_with_categories) == true) {
         rebuild_categories = true;
         DPRINTF(("callback before project event: rebuild categories due to SGE_EMA_DEL(%s)\n", p));
      }
      break;
   default:
      break;
   }

   DEXIT;
   return SGE_EMA_OK;
}


/****** sge_process_events/sge_process_userset_event_before() ******************
*  NAME
*     sge_process_userset_event_before() -- ???
*
*  SYNOPSIS
*     bool sge_process_userset_event_before(sge_object_type type,
*     sge_event_action action, lListElem *event, void *clientdata)
*
*  FUNCTION
*     Determine whether categories need to be rebuilt. Rebuilding
*     categories is necessary, if a userset (a) gets used first
*     time as ACL or (b) is no longer used as ACL. Also categories
*     must be rebuild if entries change with a userset is used as ACL.
*
*  NOTES
*     MT-NOTE: sge_process_userset_event_before() is not MT safe
*******************************************************************************/
sge_callback_result sge_process_userset_event_before(sge_evc_class_t *evc, object_description *object_base,
      sge_object_type type, sge_event_action action, lListElem *event, void *clientdata)
{
   const lListElem *new, *old;
   const char *u;

   DENTER(TOP_LAYER, "sge_process_userset_event_before");

   if (action != SGE_EMA_ADD &&
       action != SGE_EMA_MOD &&
       action != SGE_EMA_DEL) {
      DEXIT;
      return SGE_EMA_OK;
   }

   u = lGetString(event, ET_strkey);
   new = lFirst(lGetList(event, ET_new_version));
   old = userset_list_locate(*sge_master_list(object_base, SGE_TYPE_USERSET), u);

   switch (action) {
   case SGE_EMA_ADD:
      if (lGetBool(new, US_consider_with_categories) == true) {
         rebuild_categories = true;
         DPRINTF(("callback before userset event: rebuild categories due to SGE_EMA_ADD(%s)\n", u));
      }
      break;
   case SGE_EMA_MOD:
      /* need to redo categories if certain changes occur:
         --> it gets used or was used as ACL with queue_conf(5)/host_conf(5)/sge_pe(5)
         --> it is in use as ACL with queue_conf(5)/host_conf(5)/sge_pe(5)
             and a change with users/groups occured */

      if ((lGetBool(new, US_consider_with_categories) != lGetBool(old, US_consider_with_categories))
          || ( lGetBool(old, US_consider_with_categories) == true &&
            object_list_has_differences(lGetList(old, US_entries), NULL, lGetList(new, US_entries), false))) {
         rebuild_categories = true;
         DPRINTF(("callback before userset event: rebuild categories due to SGE_EMA_MOD(%s)\n", u));
      }

      break;
   case SGE_EMA_DEL:
      if (lGetBool(old, US_consider_with_categories) == true) {
         rebuild_categories = true;
         DPRINTF(("callback before userset event: rebuild categories due to SGE_EMA_DEL(%s)\n", u));
      }
      break;
   default:
      break;
   }

   DEXIT;
   return SGE_EMA_OK;
}


static sge_callback_result
sge_process_schedd_monitor_event(sge_evc_class_t *evc, object_description *object_base, sge_object_type type, 
                                     sge_event_action action, lListElem *event, void *clientdata)
{
   DENTER(GDI_LAYER, "sge_process_schedd_monitor_event");
   DPRINTF(("monitoring next scheduler run\n"));
   schedd_set_monitor_next_run(true);
   DEXIT;
   return SGE_EMA_OK;
}

void set_job_flushing(sge_evc_class_t *evc)
{
   int interval;
   bool flush;

   interval= sconf_get_flush_submit_sec();
   flush = (interval > 0) ? true : false;
   interval--;
   evc->ec_set_flush(evc, sgeE_JOB_ADD, flush, interval);

   interval = sconf_get_flush_finish_sec();
   flush = (interval > 0) ? true : false;
   interval--;
   evc->ec_set_flush(evc, sgeE_JOB_DEL, flush, interval);
   evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE, flush, interval);
   evc->ec_set_flush(evc, sgeE_JATASK_DEL, flush, interval);
}

int subscribe_default_scheduler(sge_evc_class_t *evc)
{
   ensure_valid_what_and_where();
   
   /* subscribe event types for the mirroring interface */
   sge_mirror_subscribe(evc, SGE_TYPE_CKPT,           NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_CENTRY,         NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_EXECHOST,       NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_SHARETREE,      NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_PROJECT,        sge_process_project_event_before, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_PE,             NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_CQUEUE,         NULL, NULL, NULL, where_cqueue, what_cqueue);
   sge_mirror_subscribe(evc, SGE_TYPE_QINSTANCE,      NULL, NULL, NULL, where_all_queue, what_queue);
   sge_mirror_subscribe(evc, SGE_TYPE_USER,           NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_HGROUP,         NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_RQS,            NULL, NULL, NULL, NULL, NULL);

   /* SG: this is not suported in the event master right now, for a total update 
      we have to fix it for goood some time. Issue: 1416*/
/*   sge_mirror_subscribe(evc, SGE_TYPE_PETASK,         NULL, NULL, NULL, NULL, what_pet); */
   sge_mirror_subscribe(evc, SGE_TYPE_PETASK,         NULL, NULL, NULL, NULL, NULL);
  
   /* event types with callbacks */

   sge_mirror_subscribe(evc, SGE_TYPE_SCHEDD_CONF, sge_process_schedd_conf_event_before, 
                        sge_process_schedd_conf_event_after, NULL, NULL, NULL);
                                                
   sge_mirror_subscribe(evc, SGE_TYPE_SCHEDD_MONITOR, NULL, 
                        sge_process_schedd_monitor_event, NULL, NULL, NULL);
   
   sge_mirror_subscribe(evc, SGE_TYPE_GLOBAL_CONFIG,  NULL, 
                        sge_process_global_config_event, NULL, NULL, NULL);
   
   sge_mirror_subscribe(evc, SGE_TYPE_JOB, sge_process_job_event_before, 
                        sge_process_job_event_after, NULL, where_job, what_job);
                        
   sge_mirror_subscribe(evc, SGE_TYPE_JATASK, sge_process_ja_task_event_before, 
                        sge_process_ja_task_event_after, NULL, where_jat, what_jat);
                                               
   sge_mirror_subscribe(evc, SGE_TYPE_USERSET, sge_process_userset_event_before, 
                        NULL, NULL, NULL, NULL); 

   /* set flush parameters for job */
   set_job_flushing(evc);

   /* for some reason we flush sharetree changes */
   evc->ec_set_flush(evc, sgeE_NEW_SHARETREE, true, 0);

   /* configuration changes and trigger should have immediate effevc->ect */
   evc->ec_set_flush(evc, sgeE_SCHED_CONF, true, 0);
   evc->ec_set_flush(evc, sgeE_SCHEDDMONITOR, true, 0);
   evc->ec_set_flush(evc, sgeE_GLOBAL_CONFIG,true, 0);

   return true;
}

