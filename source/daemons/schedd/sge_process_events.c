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

#include "def.h"
#include "sge.h"
#include "sge_gdi_intern.h"
#include "sge_c_event.h"
#include "sge_ckptL.h"
#include "sge_complexL.h"
#include "sge_hostL.h"
#include "sge_jobL.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_pe.h"
#include "sge_schedd.h"
#include "sge_process_events.h"
#include "sge_prog.h"
#include "sge_queueL.h"
#include "sge_ctL.h"
#include "sge_schedd_conf.h"
#include "sge_usersetL.h"
#include "sge_share_tree_nodeL.h"
#include "sge_userprjL.h"
#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "cull_sort.h"
#include "sge_event.h"
#include "sge_feature.h"
#include "schedd_conf.h"
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
#include "sge_job_jatask.h"
#include "sge_conf.h"

/* defined in sge_schedd.c */
extern int shut_me_down;
extern int start_on_master_host;

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
   *what_complex = NULL,
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
int event_handler_my_scheduler(lList *event_list) 
{
   return event_handler_default_scheduler(event_list);
}
#endif

int event_handler_default_scheduler(lList *event_list) 
{
   int ret;
   sge_Sdescr_t copy;

   DENTER(TOP_LAYER, "event_handler_default_scheduler");

   DPRINTF(("================[SCHEDULING-EPOCH]==================\n"));
   
   if ((ret=sge_process_all_events(event_list))) {
      DEXIT;
      return ret;
   }

   if ((ret=sge_before_dispatch())) {
      DEXIT;
      return ret;
   }

   memset(&copy, 0, sizeof(copy));

   ensure_valid_what_and_where();

   /* the scheduler functions have to work with a reduced copy .. */
   copy.host_list = lSelect("", lists.host_list,
                            where_host, what_host);
   copy.queue_list = lSelect("", lists.queue_list,
                             where_queue, what_queue);

   /* name all queues not suitable for scheduling in tsm-logging */
   copy.all_queue_list = lSelect("", lists.queue_list,
                                 where_all_queue, what_queue);
   copy.job_list = lSelect("", lists.job_list,
                           where_job, what_job);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      copy.dept_list = lSelect("", lists.acl_list, where_dept, what_dept);
      copy.acl_list = lSelect("", lists.acl_list, where_acl, what_acl);
   }
   else {
      copy.acl_list = lCopyList("", lists.acl_list);
      copy.dept_list = NULL;
   }

   /* .. but not in all cases */
   copy.complex_list = lCopyList("", lists.complex_list);
   copy.pe_list = lCopyList("", lists.pe_list);
   copy.share_tree = lCopyList("", lists.share_tree);
   copy.config_list = lCopyList("", lists.config_list);
   copy.user_list = lCopyList("", lists.user_list);
   copy.project_list = lCopyList("", lists.project_list);
   copy.ckpt_list = lCopyList("", lists.ckpt_list);

   /* report number of reduced and raw (in brackets) lists */
   DPRINTF(("Q:%d(%d), AQ:%d(%d) J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
            "P:%d, CKPT:%d US:%d PR:%d S:nd:%d/lf:%d CFG: %s\n",
            lGetNumberOfElem(copy.queue_list),
            lGetNumberOfElem(lists.queue_list),
            lGetNumberOfElem(copy.all_queue_list),
            lGetNumberOfElem(lists.queue_list),
            lGetNumberOfElem(copy.job_list),
            lGetNumberOfElem(lists.job_list),
            lGetNumberOfElem(copy.host_list),
            lGetNumberOfElem(lists.host_list),

            lGetNumberOfElem(copy.complex_list),
            lGetNumberOfElem(copy.acl_list),
            lGetNumberOfElem(copy.dept_list),
            lGetNumberOfElem(copy.pe_list),
            lGetNumberOfElem(copy.ckpt_list),
            lGetNumberOfElem(copy.user_list),
            lGetNumberOfElem(copy.project_list),
            lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
            lGetNumberOfLeafs(NULL, copy.share_tree, STN_children),
            copy.config_list ? "YES" : "NO"
           ));

   if (getenv("SGE_ND"))
         printf("Q:%d(%d), AQ:%d(%d) J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, "
            "P:%d, CKPT:%d US:%d PR:%d S:nd:%d/lf:%d CFG: %s\n",
            lGetNumberOfElem(copy.queue_list),
            lGetNumberOfElem(lists.queue_list),
            lGetNumberOfElem(copy.all_queue_list),
            lGetNumberOfElem(lists.queue_list),
            lGetNumberOfElem(copy.job_list),
            lGetNumberOfElem(lists.job_list),
            lGetNumberOfElem(copy.host_list),
            lGetNumberOfElem(lists.host_list),

            lGetNumberOfElem(copy.complex_list),
            lGetNumberOfElem(copy.acl_list),
            lGetNumberOfElem(copy.dept_list),
            lGetNumberOfElem(copy.pe_list),
            lGetNumberOfElem(copy.ckpt_list),
            lGetNumberOfElem(copy.user_list),
            lGetNumberOfElem(copy.project_list),
            lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
            lGetNumberOfLeafs(NULL, copy.share_tree, STN_children),
            copy.config_list ? MSG_YES  : MSG_NO 
           );
   else
      SCHED_MON((log_string, "-------------START-SCHEDULER-RUN-------------"));

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
   
   if (getenv("SGE_ND"))
      printf("--------------STOP-SCHEDULER-RUN-------------");
   else
      SCHED_MON((log_string, "--------------STOP-SCHEDULER-RUN-------------"));

   monitor_next_run = 0;

   /* .. which gets deleted after using */
   copy.host_list = lFreeList(copy.host_list);
   copy.queue_list = lFreeList(copy.queue_list);
   copy.all_queue_list = lFreeList(copy.all_queue_list);
   copy.job_list = lFreeList(copy.job_list);
   copy.complex_list = lFreeList(copy.complex_list);
   copy.acl_list = lFreeList(copy.acl_list);
   if (feature_is_enabled(FEATURE_SGEEE))
      copy.dept_list = lFreeList(copy.dept_list);
   copy.pe_list = lFreeList(copy.pe_list);
   copy.share_tree = lFreeList(copy.share_tree);
   copy.user_list = lFreeList(copy.user_list);
   copy.project_list = lFreeList(copy.project_list);
   copy.config_list = lFreeList(copy.config_list);
   copy.ckpt_list = lFreeList(copy.ckpt_list);
   
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------*/
void ensure_valid_what_and_where(void) 
{
   static int called = 0;

   DENTER(TOP_LAYER, "ensure_valid_what_and_where");
   
   if (called) {
      DEXIT;
      return;
   }
   called = 1;

   if (!where_queue) 
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
   if (!where_queue) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORQUEUEFAILED ));
DTRACE;

   /* ---------------------------------------- */

   if (!where_all_queue) 
      where_all_queue = lWhere("%T(%I!=%s)", QU_Type,    
            QU_qname, SGE_TEMPLATE_NAME); /* do not select queue "template" */
   if (!where_all_queue) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORALLQUEUESFAILED ));
DTRACE;

   /* ---------------------------------------- */

DTRACE;
   if (!where_host) 
      where_host = lWhere("%T(!(%Ic=%s))", EH_Type, EH_name, SGE_TEMPLATE_NAME);
   if (!where_host) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORHOSTFAILED ));

   /* ---------------------------------------- */

DTRACE;
   if (!where_dept) 
      where_dept = lWhere("%T(%I m= %u)", US_Type, US_type, US_DEPT);
   if (!where_dept) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORDEPTFAILED));

   /* ---------------------------------------- */

DTRACE;
   if (!where_acl) 
      where_acl = lWhere("%T(%I m= %u)", US_Type, US_type, US_ACL);
   if (!where_acl) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORACLFAILED));

DTRACE;

   /* ---------------------------------------- */
   if (!what_host ) 
      what_host = lWhat("%T(ALL)", EH_Type);
   /* ---------------------------------------- */
   if (!what_queue) 
      what_queue = lWhat("%T(ALL)", QU_Type);
   /* ---------------------------------------- */
   if (!what_complex) 
      what_complex = lWhat("%T(ALL)", CX_Type);
   /* ---------------------------------------- */
   if (!what_acl) 
      what_acl = lWhat("%T(ALL)", US_Type);
   /* ---------------------------------------- */
   if (!what_dept) 
      what_dept = lWhat("%T(ALL)", US_Type);
   /* ---------------------------------------- */
   if (!what_job) 
#define NM10 "%I%I%I%I%I%I%I%I%I%I"
#define NM5  "%I%I%I%I%I"
#define NM2  "%I%I"
#define NM1  "%I"

      what_job = lWhat("%T(" NM10 NM10 NM10 NM10 NM2")", JB_Type,
            JB_job_number, 
            JB_script_file,
            JB_submission_time,
            JB_owner,
            JB_uid,      /* x*/
            JB_group,
            JB_gid,        /* x*/
            JB_nrunning,
            JB_execution_time,
            JB_checkpoint_attr,     /* x*/

            JB_checkpoint_interval, /* x*/
            JB_checkpoint_object,   
            JB_hard_resource_list,
            JB_soft_resource_list,
            JB_mail_options, /* may be we want to send mail */ /* x*/
            JB_mail_list,  /* x*/
            JB_job_name,   /* x*/
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
            JB_now,
            JB_project,
  /* SGE */ JB_department,

            JB_jobclass, /*x*/
            JB_deadline,
            JB_host,
            JB_override_tickets,
            JB_ja_structure,
            JB_ja_n_h_ids,
            JB_ja_u_h_ids,
            JB_ja_s_h_ids,
            JB_ja_o_h_ids,   
	         JB_ja_tasks,

            JB_ja_template,
            JB_category);
   if (!what_job) 
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ENSUREVALIDWHERE_LWHEREFORJOBFAILED ));

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
#define FREE_AND_NULL_IT(list) list = lFreeList(list)
  
   /* free internal lists */ 
   FREE_AND_NULL_IT(lists.host_list);
   FREE_AND_NULL_IT(lists.queue_list);
   FREE_AND_NULL_IT(lists.all_queue_list);
   FREE_AND_NULL_IT(lists.job_list);
   FREE_AND_NULL_IT(lists.complex_list);
   FREE_AND_NULL_IT(lists.acl_list);
   FREE_AND_NULL_IT(lists.pe_list);
   FREE_AND_NULL_IT(lists.config_list);
   FREE_AND_NULL_IT(lists.user_list);
   FREE_AND_NULL_IT(lists.dept_list);
   FREE_AND_NULL_IT(lists.project_list);
   FREE_AND_NULL_IT(lists.share_tree);
   FREE_AND_NULL_IT(lists.ckpt_list);
   
   /* free job sorting access tree */ 
   at_finish();

   /* free job category data */ 
   sge_free_job_category();
}

/****** schedd/sge/sge_process_all_events() ***********************************
*  NAME
*     sge_process_all_events() -- Process all events 
*
*  SYNOPSIS
*     int sge_process_all_events(lList *event_list) 
*
*  FUNCTION
*     Process all events tha
*
*  INPUTS
*     lList *event_list - ET_Type 
*
*  RESULT
*    0 ok trigger scheduling
*   -1 inconsistencies with events: register again at qmaster
*    1 configuration changed heavily: register again at qmaster 
*    2 got a shutdown event: do not schedule, finish immediately instead
******************************************************************************/
int sge_process_all_events(lList *event_list) 
{
   static u_long32 user_sort = 0;
   lList *data_list, **lpp;
   lListElem *event, *ep = NULL, *ja_task = NULL;
   u_long32 number, type, intkey, intkey2;
   const char *strkey;
   int ret;
   int sge_mode = feature_is_enabled(FEATURE_SGEEE);
   int rebuild_categories = 0, 
       rebuild_accesstree = 0;

   DENTER(TOP_LAYER, "sge_process_all_events");

   for_each(event, event_list) {
      int n;
      lList *new_version = NULL;
      number = lGetUlong(event, ET_number);
      type = lGetUlong(event, ET_type);
      intkey = lGetUlong(event, ET_intkey);
      intkey2 = lGetUlong(event, ET_intkey2);
      strkey = lGetString(event, ET_strkey);
      if ((new_version = lGetList(event, ET_new_version)))
         n = lGetNumberOfElem(new_version);
      else
         n = 0;

      DPRINTF((event_text(event)));

      if ((ret=handle_administrative_events(type, event))==1)
         continue;
      if (ret == -1) {
         DEXIT;
         goto ReregisterSchedd;   
      }         
      if (ret == 2) {
         DEXIT;
         goto HaltSchedd;   
      }         
     
      /* +================================================================+
       * | NOTE: if you add new eventhandling here,                       |
       * |      make sure to subscribe the event in sge_subscribe_schedd! |
       * +================================================================+
       */
      switch (type) {
         /* ======================================================
          * schedd configuration
          */
      case sgeE_SCHED_CONF:
         {
            lList *src;
            lListElem *ep;
            u_long32 new_user_sort;

            /* remove old sge config */
            lists.config_list = lFreeList(lists.config_list);

            if ((src = lGetList(event, ET_new_version))) {
               /* install new one */
               lists.config_list = lCreateList("config_list",
                 lGetElemDescr(lFirst(lGetList(event, ET_new_version))));
               ep = lDechainElem(src, lFirst(src));
               sc_set(NULL, &scheddconf, ep, NULL, NULL);

               /* check event client settings */
               if(ec_get_edtime() != scheddconf.schedule_interval) {
                  ec_set_edtime(scheddconf.schedule_interval);
               }
               if ((ret=use_alg(lGetString(ep, SC_algorithm)))==2) {
                  /* changings on event handler or schedule interval can take effect 
                     only after a new registration of schedd at qmaster */
                  DEXIT;
                  goto ReregisterSchedd;
               }

               new_user_sort = lGetUlong(ep, SC_user_sort);
               if (user_sort != new_user_sort) {
                  set_user_sort(new_user_sort);
                  /* state has changed */
                  if (new_user_sort) {
                     /* enable user sort */
                     DPRINTF(("SWITCH USER SORT ON\n"));
                  }
                  else {
                     /* disable user sort */
                     DPRINTF(("SWITCH USER SORT OFF\n"));
                  }
                  user_sort = new_user_sort;
                  rebuild_accesstree = 1;
               }
               lAppendElem(lists.config_list, ep);
            }
         }
         break;

         /* ======================================================
          * JOB 
          */
      case sgeE_JOB_DEL:
         {
            u_long32 was_running;

            ep = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXTODELETE_U ,
                      u32c(intkey)));
               break;
            }
            else {
               for_each(ja_task, (lGetList(ep, JB_ja_tasks))) {
                  was_running = running_status(lGetUlong(ja_task, JAT_status));
                  /* decrease # of running jobs for this user */
                  if (was_running && !sge_mode && user_sort)
                     at_dec_job_counter(lGetUlong(ep, JB_priority), lGetString(ep, JB_owner), 1);
               }
               /* delete job category if necessary */
               sge_delete_job_category(ep);
               if (!sge_mode)
                   at_unregister_job_array(ep);
               lDelElemUlong(&(lists.job_list), JB_job_number, intkey);
            }
         }
         break;

      case sgeE_JOB_ADD:
         {
            u_long32 start, end, step;

            if (!n) {
               ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDJOBGOTNONEWJOB_IUU ,
                      (int) number, u32c(intkey), u32c (intkey2)));
               DEXIT;
               goto Error;
            }
   
            if (!lists.job_list)
               lists.job_list = lCreateList("new job list", JB_Type);
            data_list = lGetList(event, ET_new_version);
            ep = lDechainElem(data_list, lFirst(data_list));

            /* put it in sort order into the list */
            lAppendElem(lists.job_list, ep);
         
            /* add job category */
            sge_add_job_category(ep, lists.acl_list);
            if (!sge_mode)
               at_register_job_array(ep);

            job_get_submit_task_ids(ep, &start, &end, &step);
            if (job_is_array(ep)) {
               DPRINTF(("Added job-array "u32"."u32"-"u32":"u32"\n", lGetUlong(ep, JB_job_number),
                  start, end, step));
            } else {
               DPRINTF(("Added job "u32"\n", lGetUlong(ep, JB_job_number)));
            } 
         }
         break;

      case sgeE_JOB_MOD:
         {
            lListElem *job;
            int i;

            /* had problems with replacing job elements
               so we need to overwrite all fields */
            static int str_nm[] =
            {
               JB_owner,
               JB_group,
               JB_checkpoint_object,
               JB_job_name,
               JB_pe,
            /* SGE */
               JB_project,
               JB_department,
               JB_jobclass,
               NoName
            };

            static int host_nm[] =
            {
               JB_host,
               NoName
            };

            static int ulong_nm[] =
            {
               JB_job_number,
               JB_submission_time,
               JB_uid,
               JB_gid,
               JB_execution_time,
               JB_checkpoint_attr,
               JB_checkpoint_interval,
               JB_mail_options, /* may be we want to send mail */
               JB_priority,
               JB_soft_wallclock_gmt,
               JB_hard_wallclock_gmt,
               JB_version,
               JB_now,
            /* SGE */
               JB_override_tickets,
               NoName
            };
            static int list_nm[] =
            {
               JB_jid_predecessor_list,
               JB_pe_range,
               JB_hard_queue_list,
               JB_soft_queue_list,
               JB_hard_resource_list,
               JB_soft_resource_list,
               JB_master_hard_queue_list,
               JB_mail_list,
               JB_ja_n_h_ids,
               JB_ja_u_h_ids,
               JB_ja_o_h_ids,
               JB_ja_s_h_ids,
               JB_ja_template,
               NoName
            };

            /* 
             * Note: We assume that everything in a job may 
             *       change besides the jobs priority.
             *       So here is no need to sort in a job new
             *       when it gets changed (ok actually it is done). 
             *       If you want to change the jobs priority
             *       send a sgeE_JOB_MOD_SCHED_PRIORITY!
             *       
             */
            if (!n) {
               ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_UU ,
                      u32c(number), u32c(intkey)));
               DEXIT;
               goto Error;
            }
            ep = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXTOMODIFY_U , u32c(intkey)));
               DEXIT;
               goto Error;
            }

            /*
            ** before changing anything, remove category reference 
            ** for unchanged job
            */
            if (!sge_mode)
               at_unregister_job_array(ep);
            sge_delete_job_category(ep);

            data_list = lGetList(event, ET_new_version);
            job = lFirst(data_list);

            /* copy all strings */
            for (i = 0; str_nm[i] != NoName; i++)
               if (lGetPosViaElem(job, str_nm[i])) {
                  DPRINTF(("sge_process_all_events: copy all strings\n"));
                  lSetString(ep, str_nm[i], lGetString(job, str_nm[i]));
               }
            /* copy all hosts */
            for (i = 0; host_nm[i] != NoName; i++)
               if (lGetPosViaElem(job, host_nm[i])) {
                  DPRINTF(("sge_process_all_events: copy all hosts\n"));
                  lSetHost(ep, host_nm[i], lGetHost(job, host_nm[i]));
               }


            /* copy all ulongs */
            for (i = 0; ulong_nm[i] != NoName; i++)
               if (lGetPosViaElem(job, ulong_nm[i]))
                  lSetUlong(ep, ulong_nm[i], lGetUlong(job, ulong_nm[i]));

            /* copy all lists */
            for (i = 0; list_nm[i] != NoName; i++)
               if (lGetPosViaElem(job, list_nm[i]))
                  lSetList(ep, list_nm[i], lCopyList("", lGetList(job, list_nm[i])));

            /*
            ** after changing the job, readd category reference 
            ** for changed job
            */
            sge_add_job_category(ep, lists.acl_list);
            if (!sge_mode)
               at_register_job_array(ep);
         }
         break;

      case sgeE_JATASK_MOD:
         {
            lListElem *ja_task, *new_ja_task;
            u_long32 old_status;
            int i; 

            static int str_nm[] =
            {
               JAT_granted_pe,
               JAT_master_queue,
               JAT_osjobid,
               NoName
            };
            static int ulong_nm[] =
            {
               JAT_task_number,
               JAT_status,
               JAT_start_time,
               JAT_hold,
               JAT_state,
               JAT_pvm_ckpt_pid,
               JAT_pending_signal,
               JAT_pending_signal_delivery_time,
               JAT_pid,
               JAT_fshare,
               JAT_suitable,
               JAT_job_restarted, /* BOOL <-> ULONG */
               NoName
            };
            static int list_nm[] =
            {
               JAT_granted_destin_identifier_list,
               JAT_usage_list,
               JAT_scaled_usage_list,
               JAT_previous_usage_list,
               NoName
            };
            static int ref_nm[] =
            {
               NoName
            };
            static int double_nm[] =
            {
               JAT_share,
               JAT_ticket,
               JAT_oticket,
               JAT_dticket,
               JAT_fticket,
               JAT_sticket,
               NoName
            };

            if (!n) {
               ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODJATASKGOTNONEWJATASK_UUU ,
                      u32c(number), u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            }
            ep = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXTOMODIFYTASKY_UU , u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            }
            
            ja_task = job_search_task(ep, NULL, intkey2, 1);
            if (!ja_task) {
               /* usually caused by the precautionally sent delete event */
               WARNING((SGE_EVENT,MSG_JOB_CANTFINDJOBARRAYTASKXYTOMODIFY_UU,
                        u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            } 

            data_list = lGetList(event, ET_new_version);
            new_ja_task = lFirst(data_list);
           
            old_status = lGetUlong(ja_task, JAT_status);
 
            /* copy all strings */
            for (i = 0; str_nm[i] != NoName; i++)
               if (lGetPosViaElem(new_ja_task, str_nm[i])) 
                  lSetString(ja_task, str_nm[i], lGetString(new_ja_task, str_nm[i]));

            /* copy all ulongs */
            for (i = 0; ulong_nm[i] != NoName; i++)
               if (lGetPosViaElem(new_ja_task, ulong_nm[i]))
                  lSetUlong(ja_task, ulong_nm[i], lGetUlong(new_ja_task, ulong_nm[i]));
   
            /* copy all references */
            for (i = 0; ref_nm[i] != NoName; i++)
               if (lGetPosViaElem(new_ja_task, ref_nm[i]))
                  lSetRef(ja_task, ref_nm[i], lGetRef(new_ja_task, ref_nm[i])); 

            /* copy all doubles */
            for (i = 0; double_nm[i] != NoName; i++)
               if (lGetPosViaElem(new_ja_task, double_nm[i]))
                  lSetDouble(ja_task, double_nm[i], lGetDouble(new_ja_task, double_nm[i]));

            /* copy all lists */
            for (i = 0; list_nm[i] != NoName; i++)
               if (lGetPosViaElem(new_ja_task, list_nm[i]))
                  lSetList(ja_task, list_nm[i], lCopyList("", lGetList(new_ja_task, list_nm[i])));

            if (running_status(lGetUlong(ja_task, JAT_status))) {
               if (!running_status(old_status)) {
                  DPRINTF(("JATASK "u32"."u32": IDLE -> RUNNING\n", intkey, intkey2));
                  if (!sge_mode && user_sort)
                     at_inc_job_counter(lGetUlong(ep, JB_priority), lGetString(ep, JB_owner), 1);
               }
            } else {
               if (running_status(old_status)) {
                  DPRINTF(("JATASK "u32"."u32": RUNNING -> IDLE\n", intkey, intkey2));
                  if (!sge_mode && user_sort)
                     at_dec_job_counter(lGetUlong(ep, JB_priority), lGetString(ep, JB_owner), 1);
               }
            }     
         }
         break;

      case sgeE_PETASK_ADD:
         {
            lListElem *job, *ja_task, *pe_task;
            lList *task_list;
   
            /* check if event contains any objects */
            if (n <= 0) {
               ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDPETASKXGOTNONEWPETASK_IUUS,
                      (int) number, u32c(intkey), u32c(intkey2), strkey));
               DEXIT;
               goto Error;
            }
           
            /* search job */
            job = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (job == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXFORADDINGPETASK_U, 
                      u32c(intkey)));
               DEXIT;
               goto Error;
            }

            /* search ja task */
            /* JG: TODO (256): use lGetElemUlong  */
            for_each(ja_task, lGetList(job, JB_ja_tasks)) {
               if (lGetUlong(ja_task, JAT_task_number) == intkey2)
                  break;
            }

            if (ja_task == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKXFORADDINGPETASK_UU, 
                      u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            }

            /* if pe task list not yet existed, create it,
             * else check if the new pe task is already there
             */
            task_list = lGetList(ja_task, JAT_task_list);
            if(task_list == NULL) {
               task_list = lCreateList("pe tasks", PET_Type);
               lSetList(ja_task, JAT_task_list, task_list);
            } else {
               pe_task = lGetElemStr(task_list, PET_id, strkey);
               if (pe_task != NULL) {
                  DPRINTF(("Had to add pe task %s to job/jatask "U32CFormat"."U32CFormat", but it's already here\n",
                           strkey, u32c(intkey), u32c(intkey2)));
                  DEXIT;
                  goto Error;
               }
            }

            /* copy pe task to ja tasks pe task list */
            pe_task = lDechainElem(new_version, lFirst(new_version)); 
            lAppendElem(task_list, pe_task);
         }
         break;

      case sgeE_PETASK_DEL:
         {
            lListElem *job, *ja_task;
            lList *task_list;
   
            /* search job */
            job = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (job == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXFORDELETINGPETASK_U, 
                      u32c(intkey)));
               DEXIT;
               goto Error;
            }

            /* search ja task */
            /* JG: TODO (256): use lGetElemUlong  */
            for_each(ja_task, lGetList(job, JB_ja_tasks)) {
               if (lGetUlong(ja_task, JAT_task_number) == intkey2)
                  break;
            }

            if (ja_task == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKXFORDELETINGPETASK_UU, 
                      u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            }
            
            task_list = lGetList(ja_task, JAT_task_list);
            if(task_list != NULL) {
               lDelElemStr(&task_list, PET_id, strkey);
            }
         }   
         break;
         
      case sgeE_JOB_LIST:
         lXchgList(event, ET_new_version, &(lists.job_list));
         rebuild_categories = 1;
         break;

      case sgeE_JOB_MOD_SCHED_PRIORITY:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODJOBGOTNONEWJOB_IU,
                   (int) number, u32c(intkey)));
            DEXIT;
            goto Error;
         }
         ep = lGetElemUlong(lists.job_list, JB_job_number, intkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXTOMODIFYPRIORITY_U ,
                   u32c(intkey)));
            DEXIT;
            goto Error;
         }

         if (!sge_mode)
            at_unregister_job_array(ep);

         /* use new scheduling priority */
         lSetUlong(ep, JB_priority, lGetUlong(lFirst(
                         lGetList(event, ET_new_version)), JB_priority));

         if (!sge_mode)
            at_register_job_array(ep);

         break;

      case sgeE_JOB_FINAL_USAGE:
      case sgeE_JOB_USAGE:
         {
            u_long32 was_running;
            lListElem *job, *ja_task, *pe_task = NULL;

            job = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (job == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXFORUPDATINGUSAGE_U, 
                      u32c(intkey)));
               DEXIT;
               goto Error;
            }

            /* JG: TODO (256): use lGetElemUlong  */
            for_each(ja_task, lGetList(job, JB_ja_tasks)) {
               if (lGetUlong(ja_task, JAT_task_number) == intkey2)
                  break;
            }

            if (ja_task == NULL) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKXFORUPDATINGUSAGE_UU, 
                      u32c(intkey), u32c(intkey2)));
               DEXIT;
               goto Error;
            }

	         if (strkey != NULL) { /* must be a pe task usage */
               pe_task = lGetSubStr(ja_task, PET_id, strkey, JAT_task_list);
		         if (pe_task == NULL) {
                  ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKXFORUPDATINGUSAGE_UUS, 
                         u32c(intkey), u32c(intkey2), strkey));
                  DEXIT;
                  goto Error;
               }
            }	    


            /* exchange old and new usage list */
            {
               lList *tmp = NULL;

               lXchgList(event, ET_new_version, &tmp);
               if(pe_task != NULL) {
                  lXchgList(pe_task, PET_scaled_usage, &tmp);
               } else {
                  lXchgList(ja_task, JAT_scaled_usage_list, &tmp);
               }
               lXchgList(event, ET_new_version, &tmp);
            }

            was_running = running_status(lGetUlong(ja_task, JAT_status));
            /* decrease # of running jobs for this user */
            if (type == sgeE_JOB_FINAL_USAGE && was_running && pe_task == NULL) {
               if (!sge_mode && user_sort)
                  at_dec_job_counter(lGetUlong(job, JB_priority), lGetString(job, JB_owner), 1);
            }

            if (type == sgeE_JOB_FINAL_USAGE && pe_task == NULL)
               lSetUlong(ja_task, JAT_status, JFINISHED);
         }
         break;

         /* ======================================================
          * QUEUE 
          */
      case sgeE_QUEUE_LIST:

         lXchgList(event, ET_new_version, &(lists.queue_list));
         break;

      case sgeE_QUEUE_DEL:
         ep = lGetElemStr(lists.queue_list, QU_qname, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(&(lists.queue_list), QU_qname, strkey);
         break;

      case sgeE_QUEUE_ADD:
         DPRINTF(("%d. EVENT ADD QUEUE %s\n", (int) number, strkey));

         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDQUEUEXGOTNONEWQUEUE_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.queue_list, QU_qname, strkey);
         if (ep) {
            DPRINTF(("Had to add Queue %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.queue_list)
            lists.queue_list = lCreateList("new queue list", QU_Type);

         data_list = lGetList(event, ET_new_version);
         ep = lDechainElem(data_list, lFirst(data_list));
         lAppendElem(lists.queue_list, ep);
         break;

      case sgeE_QUEUE_MOD:

         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODQUEUEXGOTNONEWQUEUE_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.queue_list, QU_qname, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(lists.queue_list, ep);
         data_list = lGetList(event, ET_new_version);
         ep = lDechainElem(data_list, lFirst(data_list));
         lAppendElem(lists.queue_list, ep);
         break;

      case sgeE_QUEUE_SUSPEND_ON_SUB:
      case sgeE_QUEUE_UNSUSPEND_ON_SUB:
         {
            u_long32 state;

            ep = lGetElemStr(lists.queue_list, QU_qname, strkey);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEXTOYONSUBORDINATE_SS , strkey,
                      (type == sgeE_QUEUE_SUSPEND_ON_SUB) ? MSG_QUEUE_SUSPEND  : MSG_QUEUE_UNSUSPEND ));
            DEXIT;
            goto Error;
            }

            state = lGetUlong(ep, QU_state);
            if (type == sgeE_QUEUE_SUSPEND_ON_SUB)
               state |= QSUSPENDED_ON_SUBORDINATE;      /* set this bit */
            else
               state &= ~QSUSPENDED_ON_SUBORDINATE;     /* reset this bit */
            lSetUlong(ep, QU_state, state);

         }
         break;

         /* ======================================================

            COMPLEX 

          */
      case sgeE_COMPLEX_LIST:
         lXchgList(event, ET_new_version, &(lists.complex_list));
         break;

      case sgeE_COMPLEX_DEL:
         ep = lGetElemStr(lists.complex_list, CX_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_COMPLEX_CANTFINDCOMPLEXXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(&(lists.complex_list), CX_name, strkey);
         break;

      case sgeE_COMPLEX_ADD:

         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDCOMPLEXXGOTNONEWCOMPLEX_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.complex_list, CX_name, strkey);
         if (ep) {
            DPRINTF(("Had to add complex %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.complex_list)
            lists.complex_list = lCreateList("new complex list", CX_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.complex_list,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

      case sgeE_COMPLEX_MOD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODCOMPLEXXGOTNONEWCOMPLEX_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.complex_list, CX_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_COMPLEX_CANTFINDCOMPLEXXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(lists.complex_list, ep);
         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.complex_list,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

         /* ======================================================
          * USERSET 
          */
      case sgeE_USERSET_LIST:
         lXchgList(event, ET_new_version, &(lists.acl_list));
         rebuild_categories = 1;
         break;

      case sgeE_USERSET_DEL:
         ep = lGetElemStr(lists.acl_list, US_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_USERSET_CANTFINDUSERSETXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(&(lists.acl_list), US_name, strkey);
         rebuild_categories = 1;
         break;

      case sgeE_USERSET_ADD:

         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDACLXGOTNONEWACL_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.acl_list, US_name, strkey);
         if (ep) {
            DPRINTF(("Had to add acl %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.acl_list)
            lists.acl_list = lCreateList("new acl list", US_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.acl_list,
                     lDechainElem(data_list, lFirst(data_list)));
         rebuild_categories = 1;
         break;

      case sgeE_USERSET_MOD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODACLXGOTNONEWACL_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.acl_list, US_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_USERSET_CANTFINDUSERSETXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(lists.acl_list, ep);
         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.acl_list,
                     lDechainElem(data_list, lFirst(data_list)));
         rebuild_categories = 1;
         break;

         /* ======================================================
          * USER/PROJECT 
          */
      case sgeE_USER_LIST:
      case sgeE_PROJECT_LIST:
         lpp = (type == sgeE_USER_LIST) ?
            &(lists.user_list) :
            &(lists.project_list);
         lXchgList(event, ET_new_version, lpp);

         break;

      case sgeE_USER_DEL:
      case sgeE_PROJECT_DEL:
         lpp = (type == sgeE_USER_DEL) ?
            &(lists.user_list) :
            &(lists.project_list);
         ep = lGetElemStr(*lpp, UP_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_SCHEDD_CANTFINDUSERORPROJECTXTODELETE_SS ,
                   type == sgeE_USER_DEL ? MSG_USER : MSG_PROJECT,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(lpp, UP_name, strkey);
         break;

      case sgeE_USER_ADD:
      case sgeE_PROJECT_ADD:
         lpp = (type == sgeE_USER_ADD) ?
            &(lists.user_list) :
            &(lists.project_list);
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDUSERORPROJXGOTNONEWONE_ISS ,
                   (int) number,
                   type == sgeE_USER_ADD ? MSG_USER : MSG_PROJECT,
                   strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(*lpp, UP_name, strkey);
         if (ep) {
            DPRINTF(("Had to add %s %s but it's already here\n",
                     type == sgeE_USER_ADD ? MSG_USER : MSG_PROJECT,
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!*lpp)
            *lpp = lCreateList("new list", UP_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(*lpp,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

      case sgeE_USER_MOD:
      case sgeE_PROJECT_MOD:
         lpp = (type == sgeE_USER_MOD) ?
            &(lists.user_list) :
            &(lists.project_list);
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODUSERORPROJXGOTNONEWONE_ISS ,
              (int) number, (type == sgeE_USER_MOD ? MSG_USER : MSG_PROJECT),
                   strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(*lpp, UP_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_SCHEDD_CANTFINDUSERORPROJECTXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(*lpp, ep);
         data_list = lGetList(event, ET_new_version);

         lAppendElem(*lpp,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

         /* ======================================================

          *  EXECHOST 
          */
      case sgeE_EXECHOST_LIST:
         lXchgList(event, ET_new_version, &(lists.host_list));
         break;

      case sgeE_EXECHOST_DEL:
         ep = lGetElemHost(lists.host_list, EH_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_EXECHOST_CANTFINDEXECHOSTXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemHost(&(lists.host_list), EH_name, strkey);
         break;

      case sgeE_EXECHOST_ADD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDEXECHOSTXGOTNONEWEXECHOST_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }

         ep = lGetElemHost(lists.host_list, EH_name, strkey);
         if (ep) {
            DPRINTF(("Had to add exechost %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.host_list)
            lists.host_list = lCreateList("new exechost list", EH_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.host_list,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

      case sgeE_EXECHOST_MOD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODEXECHOSTXGOTNONEWEXECHOST_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }

         ep = lGetElemHost(lists.host_list, EH_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_EXECHOST_CANTFINDEXECHOSTXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }

         lRemoveElem(lists.host_list, ep);
         lAppendElem(lists.host_list,
                     lDechainElem(new_version, lFirst(new_version)));

         break;

         /* ======================================================
          * PE 
          */
      case sgeE_PE_LIST:
         lXchgList(event, ET_new_version, &(lists.pe_list));
         break;

      case sgeE_PE_DEL:
         ep = lGetElemStr(lists.pe_list, PE_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_PARENV_CANTFINDPARENVXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(&(lists.pe_list), PE_name, strkey);
         break;

      case sgeE_PE_ADD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDPEXGOTNONEWPE_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.pe_list, PE_name, strkey);
         if (ep) {
            DPRINTF(("Had to add parallel environment %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.pe_list)
            lists.pe_list = lCreateList("new pe list", PE_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.pe_list,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

      case sgeE_PE_MOD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODPEXGOTNONEWPE_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.pe_list, PE_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_PARENV_CANTFINDPARENVXTOMODIFY_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(lists.pe_list, ep);
         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.pe_list,
                     lDechainElem(data_list, lFirst(data_list)));

         break;

         /* ====================================================== */
      case sgeE_NEW_SHARETREE:
         if (feature_is_enabled(FEATURE_SGEEE)) {
            lList *src;

            /* remove old share tree */
            lists.share_tree = lFreeList(lists.share_tree);

            if ((src = lGetList(event, ET_new_version))) {
               /* install new one */
               lists.share_tree = lCreateList("share tree",
                 lGetElemDescr(lFirst(lGetList(event, ET_new_version))));
               lAppendElem(lists.share_tree, lDechainElem(src, lFirst(src)));
            }
         }
         break;

         /* ======================================================
          * CKPT
          */
      case sgeE_CKPT_LIST:
         lXchgList(event, ET_new_version, &(lists.ckpt_list));
         break;

      case sgeE_CKPT_DEL:
         ep = lGetElemStr(lists.ckpt_list, CK_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_CHKPNT_CANTFINDCHKPNTINTXTODELETE_S ,
                   strkey));
            DEXIT;
            goto Error;
         }
         lDelElemStr(&(lists.ckpt_list), CK_name, strkey);
         break;

      case sgeE_CKPT_ADD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTADDCKPTXGOTNONEWCKPTINT_IS ,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.ckpt_list, CK_name, strkey);
         if (ep) {
            DPRINTF(("Had to add ckpt interface definition %s but it's already here\n",
                     strkey));
            DEXIT;
            goto Error;
         }
         if (!lists.ckpt_list)
            lists.ckpt_list = lCreateList("new ckpt list", CK_Type);

         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.ckpt_list,
                     lDechainElem(data_list, lFirst(data_list)));
         break;

      case sgeE_CKPT_MOD:
         if (!n) {
            ERROR((SGE_EVENT, MSG_EVENT_XEVENTMODCKPTXGOTNONEWCKPTINT_IS,
                   (int) number, strkey));
            DEXIT;
            goto Error;
         }
         ep = lGetElemStr(lists.ckpt_list, CK_name, strkey);
         if (!ep) {
            ERROR((SGE_EVENT, MSG_CHKPNT_CANTFINDCHKPNTINTXTOMODIFY_S , strkey));
            DEXIT;
            goto Error;
         }
         lRemoveElem(lists.ckpt_list, ep);
         data_list = lGetList(event, ET_new_version);
         lAppendElem(lists.ckpt_list,
                     lDechainElem(data_list, lFirst(data_list)));

         break;
         
         /* ======================================================
          * JA-TASK 
          */
      case sgeE_JATASK_DEL:
         {
            u_long32 was_running;

            ep = lGetElemUlong(lists.job_list, JB_job_number, intkey);
            if (!ep) {
               ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBXTODELETEJOBARRAYTASK_U , 
                      u32c(intkey)));
               DEXIT;
               goto Error;
            } else {
               int is_enrolled = job_is_enrolled(ep, intkey2);

               if (is_enrolled) {
                  lList *ja_tasks = lGetList(ep, JB_ja_tasks);

                  ja_task = lGetElemUlong(ja_tasks, JAT_task_number, intkey2);
                  if (!ja_task) {
                     ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBARRAYTASKXTODELETE_U,
                            u32c(intkey2)));
                     DEXIT;
                     goto Error;
                  } else {
                     was_running = running_status(lGetUlong(ja_task, JAT_status));
                     if (was_running && !sge_mode && user_sort)
                        at_dec_job_counter(lGetUlong(ep, JB_priority), 
                                           lGetString(ep, JB_owner), 1);
                     lRemoveElem(lGetList(ep, JB_ja_tasks), ja_task);
                  }
               } else {
                  job_delete_not_enrolled_ja_task(ep, NULL, intkey2);
               }
               if (job_get_ja_tasks(ep) == 0) {
                  sge_delete_job_category(ep);
                  if (!sge_mode)
                     at_unregister_job_array(ep);
                  lDelElemUlong(&(lists.job_list), JB_job_number, intkey);
               }
            }
         }
         break;

      /* +================================================================+
       * | NOTE: if you add new eventhandling here,                       |
       * |      make sure to subscribe the event in sge_subscribe_schedd! |
       * +================================================================+
       */
      default:
         DPRINTF(("Unknown event type %d\n", (int) type));
         DEXIT;
         goto Error;
      }
   }

   lFreeList(event_list);

   if (rebuild_categories) {
      DPRINTF(("### ### ### ### ###     REBUILDING CATEGORIES     ### ### ### ### ###\n"));
      sge_rebuild_job_category(lists.job_list, lists.acl_list);
      /* category references are used in the access tree
         so rebuilding categories makes necessary to rebuild
         the access tree */
      rebuild_accesstree = 1;
   }
   if (rebuild_accesstree && !sge_mode) {
      DPRINTF(("### ### ### ### ###     REBUILDING ACCESS TREE    ### ### ### ### ###\n"));
      sge_rebuild_access_tree(lists.job_list, user_sort);
   }

   DEXIT;
   return 0;

HaltSchedd:
   cleanup_default_scheduler();
   lFreeList(event_list);
   return 2;
ReregisterSchedd:
   cleanup_default_scheduler();
   lFreeList(event_list);
   return 1;
Error:
   cleanup_default_scheduler();
   lFreeList(event_list);
   return -1;
}

static void sge_rebuild_access_tree(
lList *job_list,
int trace_running 
) {
   lListElem *job, *ja_task;

   /* reinit access tree module */
   at_init();

   /* register jobs and # of runnung jobs */
   for_each (job, job_list) {
      at_register_job_array(job);
      if (trace_running)
         for_each (ja_task, lGetList(job, JB_ja_tasks))
            if (running_status(lGetUlong(ja_task, JAT_status))) 
                  at_inc_job_counter(lGetUlong(job, JB_priority), lGetString(job, JB_owner), 1);      
   }
}

