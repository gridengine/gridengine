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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


#include "basis_types.h"

#include "comm/cl_commlib.h"

#include "rmon/sgermon.h"

#include "lck/sge_mtutil.h"
#include "lck/sge_lock.h"

#include "uti/sge_prog.h"
#include "uti/sge_log.h"
#include "uti/sge_unistd.h"
#include "uti/sge_time.h"
#include "uti/sge_uidgid.h"
#include "uti/sge_bootstrap.h"
#include "uti/setup_path.h"
#include "uti/sge_os.h"
#include "uti/sge_stdio.h"
#include "uti/sge_spool.h"
#include "uti/sge_profiling.h"
#include "uti/sge_thread_ctrl.h"
#include "uti/msg_utilib.h"  /* remove once 'sge_daemonize_qmaster' did become 'sge_daemonize' */

#include "evm/sge_event_master.h"

#include "sgeobj/sge_all_listsL.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_manop.h"
#include "sgeobj/sge_conf.h"

#include "gdi/qm_name.h"
#include "gdi/sge_security.h"

#include "sched/sge_serf.h"

#include "sge_advance_reservation_qmaster.h"
#include "sge_sched_process_events.h"
#include "sge_follow.h"

#include "gdi/sge_gdi_packet.h"

#include "sge_sched_job_category.h"
#include "sge_sched_order.h"

#include "sge_thread_main.h"
#include "sge_thread_scheduler.h"
#include "sge_qmaster_threads.h"
#include "sge_mt_init.h"
#include "setup_qmaster.h"
#include "sge_qmaster_process_message.h"
#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "sge_qmaster_timed_event.h"
#include "sge_host_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_calendar_qmaster.h"
#include "lock.h"
#include "qmaster_heartbeat.h"
#include "shutdown.h"
#include "sge.h"
#include "sge_qmod_qmaster.h"
#include "reschedule.h"
#include "sge_job_qmaster.h"

#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_daemons_common.h"

#define SCHEDULER_TIMEOUT_S 10
#define SCHEDULER_TIMEOUT_N 0

static char schedule_log_path[SGE_PATH_MAX + 1] = "";
const char *schedule_log_file = "schedule";

static void schedd_set_serf_log_file(sge_gdi_ctx_class_t *ctx)
{
   const char *cell_root = ctx->get_cell_root(ctx);

   DENTER(TOP_LAYER, "set_schedd_serf_log_path");

   if (!*schedule_log_path) {
      snprintf(schedule_log_path, sizeof(schedule_log_path), "%s/%s/%s", cell_root, "common", schedule_log_file);
      DPRINTF(("schedule log path >>%s<<\n", schedule_log_path));
   }

   DRETURN_VOID;
}

/* MT-NOTE: schedd_serf_record_func() is not MT safe */
static void
schedd_serf_record_func(u_long32 job_id, u_long32 ja_taskid, const char *state,
                        u_long32 start_time, u_long32 end_time,
                        char level_char, const char *object_name,
                        const char *name, double utilization)
{
   FILE *fp;

   DENTER(TOP_LAYER, "schedd_serf_record_func");

   if (!(fp = fopen(schedule_log_path, "a"))) {
      DRETURN_VOID;
   }

   /* a new record */
   fprintf(fp, sge_U32CFormat":"sge_U32CFormat":%s:"sge_U32CFormat":"
           sge_U32CFormat":%c:%s:%s:%f\n", sge_u32c(job_id),
           sge_u32c(ja_taskid), state, sge_u32c(start_time),
           sge_u32c(end_time - start_time),
           level_char, object_name, name, utilization);
   FCLOSE(fp);

   DRETURN_VOID;
FCLOSE_ERROR:
   DPRINTF((MSG_FILE_ERRORCLOSEINGXY_SS, schedule_log_path, strerror(errno)));
   DRETURN_VOID;
}

/* MT-NOTE: schedd_serf_newline() is not MT safe */
static void schedd_serf_newline(u_long32 time)
{
   FILE *fp;

   DENTER(TOP_LAYER, "schedd_serf_newline");
   fp = fopen(schedule_log_path, "a");
   if (fp) {
      /* well, some kind of new line indicating a new schedule run */
      fprintf(fp, "::::::::\n");
      FCLOSE(fp);
   }
   DRETURN_VOID;
FCLOSE_ERROR:
   DPRINTF((MSG_FILE_ERRORCLOSEINGXY_SS, schedule_log_path, strerror(errno)));
   DRETURN_VOID;
}


static void
sge_scheduler_cleanup_monitor(monitoring_t *monitor)
{
   DENTER(TOP_LAYER, "sge_scheduler_cleanup_monitor");
   sge_monitor_free(monitor);
   DRETURN_VOID;
}

static void
sge_scheduler_cleanup_event_client(sge_evc_class_t *evc)
{
   DENTER(TOP_LAYER, "sge_scheduler_cleanup_event_client");
   sge_mirror_shutdown(evc);
   DRETURN_VOID;
}

static void sge_scheduler_reset_busy(sge_evc_class_t *evc)
{
   DENTER(TOP_LAYER, "sge_scheduler_reset_busy");
   if ((evc->ec_get_busy_handling(evc) == EV_BUSY_UNTIL_RELEASED)) {
      sge_schedd_block_until_oders_processed(evc->get_gdi_ctx(evc), NULL);
      evc->ec_set_busy(evc, 0);
      evc->ec_commit(evc, NULL, &event_update_func);
   }
   DEXIT;
}

static void sge_scheduler_wait_for_event(void)
{
   u_long32 current_time = 0; 
   struct timespec ts;
   DENTER(TOP_LAYER, "sge_scheduler_wait_for_event");

   current_time = sge_get_gmt();
   while (!Scheduler_Control.triggered && !Scheduler_Control.exit &&
          ((sge_get_gmt() - current_time) < SCHEDULER_TIMEOUT_S)){
      ts.tv_sec = (long) current_time + SCHEDULER_TIMEOUT_S;
      ts.tv_nsec = SCHEDULER_TIMEOUT_N;
      pthread_cond_timedwait(&Scheduler_Control.cond_var,
                             &Scheduler_Control.mutex, &ts);
   };
   DEXIT;
}


typedef struct _master_scheduler_class_t master_scheduler_class_t;
        
struct _master_scheduler_class_t {
   /* 
    * mutex to gard all members of this structure 
    */
   pthread_mutex_t mutex;
      
   /*
    * is running
    */
   bool is_running;
};

master_scheduler_class_t Master_Scheduler = {
   PTHREAD_MUTEX_INITIALIZER,
   false
};

void 
sge_scheduler_initialize(void)
{
   static int thread_id = -1;
           
   DENTER(TOP_LAYER, "sge_scheduler_initialize");
   sge_mutex_lock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
      
   if (Master_Scheduler.is_running == false) {
      cl_thread_settings_t* dummy_thread_p = NULL;
      dstring thread_name = DSTRING_INIT;
   
      DPRINTF((SGE_EVENT, "scheduler thread data structures are initialized\n"))
      cl_thread_list_setup(&(Main_Control.scheduler_thread_pool), "thread pool");
      
      thread_id++;
      sge_dstring_sprintf(&thread_name, "%s%03d", threadnames[SCHEDD_THREAD], thread_id);
      cl_thread_list_create_thread(Main_Control.scheduler_thread_pool, &dummy_thread_p,
                                   NULL, sge_dstring_get_string(&thread_name), thread_id, 
                                   sge_scheduler_main, NULL, NULL);
      DPRINTF((SGE_EVENT, SFN" has been started\n", sge_dstring_get_string(&thread_name)))
      sge_dstring_free(&thread_name);
      Master_Scheduler.is_running = true;
   }
   sge_mutex_unlock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   DRETURN_VOID;
}

void
sge_scheduler_trigger_cancel(void)
{
   cl_thread_list_elem_t* thread = NULL;
   cl_thread_list_elem_t* next = NULL;

   DENTER(TOP_LAYER, "sge_scheduler_trigger_cancel");
   
   sge_mutex_lock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   if (Master_Scheduler.is_running == true) {
      next = cl_thread_list_get_first_elem(Main_Control.scheduler_thread_pool);
      while ((thread = next) != NULL) {
         next = cl_thread_list_get_next_elem(thread);

         DPRINTF(("sending cancel signal to thread "SFN"\n", thread->thread_config->thread_name));
         pthread_cancel(*(thread->thread_config->thread_pointer));
      }  
   }
   sge_mutex_unlock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   DRETURN_VOID;
}

void
sge_scheduler_cleanup_thread_pool(void)
{
   bool do_cleanup = false;
   
   DENTER(TOP_LAYER, "sge_scheduler_cleanup_thread_pool");
   
   sge_mutex_lock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   if (Master_Scheduler.is_running) {
      do_cleanup = true;
      Master_Scheduler.is_running = false;
   } 
   sge_mutex_unlock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   
   if (do_cleanup) {
      cl_thread_settings_t* thread = NULL;

      thread = cl_thread_list_get_first_thread(Main_Control.scheduler_thread_pool);
      while (thread != NULL) {
         DPRINTF((SGE_EVENT, SFQ" gets canceled\n", thread->thread_name));
         cl_thread_list_delete_thread_without_join(Main_Control.scheduler_thread_pool, thread);
         thread = cl_thread_list_get_first_thread(Main_Control.scheduler_thread_pool);
      }  

      DPRINTF(("cleaning up thread pool list\n"));
      cl_thread_list_cleanup(&Main_Control.scheduler_thread_pool);
   }
   
   DRETURN_VOID;
}

/* DON't call this function within a GDI request. Deadlock! */
void
sge_scheduler_terminate(void)
{
   bool do_cleanup = false;
   
   DENTER(TOP_LAYER, "sge_scheduler_terminate");
   
   sge_mutex_lock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   if (Master_Scheduler.is_running) {
      do_cleanup = true;
      Master_Scheduler.is_running = false;
   } 
   sge_mutex_unlock("master scheduler struct", SGE_FUNC, __LINE__, &(Master_Scheduler.mutex));
   
   if (do_cleanup) {
      cl_thread_settings_t* thread = NULL;

      thread = cl_thread_list_get_first_thread(Main_Control.scheduler_thread_pool);
      while (thread != NULL) {
         DPRINTF(("waking up "SFN" which might wait for event\n", thread->thread_name));
         pthread_cond_signal(&Scheduler_Control.cond_var);

         DPRINTF((SGE_EVENT, SFQ" gets canceled\n", thread->thread_name));
         cl_thread_list_delete_thread(Main_Control.scheduler_thread_pool, thread);
         thread = cl_thread_list_get_first_thread(Main_Control.scheduler_thread_pool);
      }
      
      DPRINTF(("cleaning up schedulers thread pool list\n"));
      cl_thread_list_cleanup(&Main_Control.scheduler_thread_pool);
      
      DPRINTF((SGE_EVENT, "all "SFN" threads terminated (or termination is in last stage)\n", threadnames[SCHEDD_THREAD]));
   }
   

   DRETURN_VOID;
}

void *
sge_scheduler_main(void *arg)
{
   time_t next_prof_output = 0;
   int stored_submit = 0; 
   int stored_finish = 0;
   monitoring_t monitor;
   sge_gdi_ctx_class_t *ctx = NULL;
   sge_evc_class_t *evc = NULL;
   lList *alp = NULL;   
   sge_where_what_t where_what;
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)arg;
   bool do_shutdown = false;
   bool do_endlessly = true;

   bool local_ret = true;

   DENTER(TOP_LAYER, "sge_scheduler_main");

   memset(&where_what, 0, sizeof(where_what));
      
   /* 
    * startup
    */
   if (local_ret) {
      /* initialize commlib thread */
      cl_thread_func_startup(thread_config);

      /* initialize monitoring */ 
      sge_monitor_init(&monitor, thread_config->thread_name, NONE_EXT, SCT_WARNING, SCT_ERROR);
      sge_qmaster_thread_init(&ctx, SCHEDD, SCHEDD_THREAD, true);

      /* register at profiling module */   
      set_thread_name(pthread_self(), "Scheduler Thread");
      conf_update_thread_profiling("Scheduler Thread");
      DPRINTF((SFN" started\n", thread_config->thread_name));
   }

   /* set profiling parameters */
   prof_set_level_name(SGE_PROF_EVENTMASTER, NULL, NULL);
   prof_set_level_name(SGE_PROF_SPOOLING, NULL, NULL);
   prof_set_level_name(SGE_PROF_CUSTOM0, "scheduler", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM1, "pending ticket calculation", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM3, "job sorting", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM4, "job dispatching", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM5, "send orders", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM6, "scheduler event loop", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM7, "copy lists", NULL);
   prof_set_level_name(SGE_PROF_SCHEDLIB4, NULL, NULL);

   /* set-up needed for 'schedule' file */
   serf_init(schedd_serf_record_func, schedd_serf_newline);
   schedd_set_serf_log_file(ctx);

   /* 
    * prepare event client/mirror mechanism 
    */
   if (local_ret) {
      local_ret = sge_gdi2_evc_setup(&evc, ctx, EV_ID_SCHEDD, &alp, "scheduler");
      DPRINTF((SFN" prepared event client/mirror mechanism\n", thread_config->thread_name));
   }
      
   /* 
    * register as event mirror 
    */
   if (local_ret) {
      sge_mirror_initialize(evc, EV_ID_FAST_SCHEDD, "scheduler",
                            false, &event_update_func, &sge_mod_event_client, 
                            &sge_add_event_client_local, &sge_remove_event_client,
                            &sge_handle_event_ack);
      evc->ec_register(evc, false, NULL, &event_update_func, &monitor);
      evc->ec_set_busy_handling(evc, EV_BUSY_UNTIL_RELEASED);
      DPRINTF((SFN" registered at event mirror\n", thread_config->thread_name));
   }

   /*
    * subscribe necessary data 
    */
   if (local_ret) {
      ensure_valid_what_and_where(&where_what);
      subscribe_scheduler(evc, &where_what);
      DPRINTF((SFN" subscribed necessary data from event master\n", thread_config->thread_name));
   }

   /* 
    * schedulers main loop  
    */
   if (local_ret) {
      DPRINTF((SFN" entered main loop\n", thread_config->thread_name));
      while (do_endlessly) {
         bool handled_events = false;
         lList *event_list = NULL;
         int execute = 0;

         if (sconf_get_profiling()) {
            prof_start(SGE_PROF_OTHER, NULL);
            prof_start(SGE_PROF_PACKING, NULL);
            prof_start(SGE_PROF_EVENTCLIENT, NULL);
            prof_start(SGE_PROF_MIRROR, NULL);
            prof_start(SGE_PROF_GDI, NULL);
            prof_start(SGE_PROF_HT_RESIZE, NULL);
            prof_start(SGE_PROF_CUSTOM0, NULL);
            prof_start(SGE_PROF_CUSTOM1, NULL);
            prof_start(SGE_PROF_CUSTOM3, NULL);
            prof_start(SGE_PROF_CUSTOM4, NULL);
            prof_start(SGE_PROF_CUSTOM5, NULL);
            prof_start(SGE_PROF_CUSTOM6, NULL);
            prof_start(SGE_PROF_CUSTOM7, NULL);
            prof_start(SGE_PROF_SCHEDLIB4, NULL);
         } else {
            prof_stop(SGE_PROF_OTHER, NULL);
            prof_stop(SGE_PROF_PACKING, NULL);
            prof_stop(SGE_PROF_EVENTCLIENT, NULL);
            prof_stop(SGE_PROF_MIRROR, NULL);
            prof_stop(SGE_PROF_GDI, NULL);
            prof_stop(SGE_PROF_HT_RESIZE, NULL);
            prof_stop(SGE_PROF_CUSTOM0, NULL);
            prof_stop(SGE_PROF_CUSTOM1, NULL);
            prof_stop(SGE_PROF_CUSTOM3, NULL);
            prof_stop(SGE_PROF_CUSTOM4, NULL);
            prof_stop(SGE_PROF_CUSTOM5, NULL);
            prof_stop(SGE_PROF_CUSTOM6, NULL);
            prof_stop(SGE_PROF_CUSTOM7, NULL);
            prof_stop(SGE_PROF_SCHEDLIB4, NULL);
         }

         sge_mutex_lock("scheduler_thread_cond_mutex", SGE_FUNC, __LINE__, 
                        &Scheduler_Control.mutex);

         MONITOR_IDLE_TIME(sge_scheduler_wait_for_event(), (&monitor), mconf_get_monitor_time(), 
                           mconf_is_monitor_message());

         evc->ec_set_busy(evc, 1);

         /* taking out the new events */
         event_list = Scheduler_Control.new_events;
         Scheduler_Control.new_events = NULL;
         Scheduler_Control.triggered = false;

         DPRINTF(("SCHEDULER TAKES FROM QUEUE\n"));

         sge_mutex_unlock("scheduler_thread_cond_mutex", SGE_FUNC, __LINE__, 
                          &Scheduler_Control.mutex);
         
         if (event_list != NULL) {
            DPRINTF((SFN" got new events\n", thread_config->thread_name));

            do_shutdown = (lGetElemUlong(event_list, ET_type, sgeE_SHUTDOWN) != NULL) ? true : false;

            if (!do_shutdown 
                && sge_mirror_process_event_list(evc, event_list) == SGE_EM_OK) {
               handled_events = true;
            }
            lFreeList(&event_list);
         }
         evc->ec_ack(evc);

         if (do_shutdown == true) {
            /* skip loop immediately */
            DPRINTF((SFN" events contain shutdown event\n", thread_config->thread_name));
         }


         /* incorperate the new events */
         if (handled_events == true) {
            scheduler_all_data_t copy;
            lList *orders = NULL;
            lList *master_cqueue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
            lList *master_job_list = *object_type_get_master_list(SGE_TYPE_JOB);
            lList *master_userset_list = *object_type_get_master_list(SGE_TYPE_USERSET);
            lList *master_project_list = *object_type_get_master_list(SGE_TYPE_PROJECT);
            lList *master_exechost_list= *object_type_get_master_list(SGE_TYPE_EXECHOST);
            lList *master_rqs_list= *object_type_get_master_list(SGE_TYPE_RQS);
            lList *master_centry_list = *object_type_get_master_list(SGE_TYPE_CENTRY);
            lList *master_ckpt_list = *object_type_get_master_list(SGE_TYPE_CKPT);
            lList *master_user_list = *object_type_get_master_list(SGE_TYPE_USER);
            lList *master_ar_list = *object_type_get_master_list(SGE_TYPE_AR);
            lList *master_pe_list = *object_type_get_master_list(SGE_TYPE_PE);
            lList *master_hgrp_list = *object_type_get_master_list(SGE_TYPE_HGROUP);
            lList *master_sharetree_list = *object_type_get_master_list(SGE_TYPE_SHARETREE);

            lList *selected_dept_list = NULL;
            lList *selected_acl_list = NULL;

            DPRINTF((SGE_EVENT, SFN" copy data for schedd run\n", thread_config->thread_name));
            memset(&copy, 0, sizeof(copy));

            selected_dept_list = lSelect("", master_userset_list, 
                                         where_what.where_dept, where_what.what_dept);
            selected_acl_list = lSelect("", master_userset_list, 
                                        where_what.where_acl, where_what.what_acl);

            DPRINTF(("RAW CQ:%d, J:%d, H:%d, C:%d, A:%d, D:%d, P:%d, CKPT:%d,"
                     " US:%d, PR:%d, RQS:%d, AR:%d, S:nd:%d/lf:%d\n",
               lGetNumberOfElem(master_cqueue_list),
               lGetNumberOfElem(master_job_list),
               lGetNumberOfElem(master_exechost_list),
               lGetNumberOfElem(master_centry_list),
               lGetNumberOfElem(selected_acl_list),
               lGetNumberOfElem(selected_dept_list),
               lGetNumberOfElem(master_project_list),
               lGetNumberOfElem(master_ckpt_list),
               lGetNumberOfElem(master_user_list),
               lGetNumberOfElem(master_project_list),
               lGetNumberOfElem(master_rqs_list),
               lGetNumberOfElem(master_ar_list),
               lGetNumberOfNodes(NULL, master_sharetree_list, STN_children),
               lGetNumberOfLeafs(NULL, master_sharetree_list, STN_children)
            ));

            if (get_rebuild_categories()) {
               DPRINTF(("### ### ### ###   REBUILDING CATEGORIES   ### ### ### ###\n"));
               sge_rebuild_job_category(master_job_list, master_userset_list, 
                                        master_project_list, master_rqs_list);
               /* category references are used in the access tree
                  so rebuilding categories makes necessary to rebuild
                  the access tree */
               set_rebuild_categories(false);
            }

            sge_before_dispatch(evc);
   
            /* prepare data the the scheduler itself */
            copy.host_list = lSelect("", master_exechost_list, 
                                     where_what.where_host, where_what.what_host);

            /*     
             * Within the scheduler we do only need QIs
             */
            {
               lListElem *cqueue = NULL;

               lEnumeration *what_queue3 = NULL;

               for_each(cqueue, master_cqueue_list) {
                  lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
                  lList *t;

                  if (!qinstance_list)
                     continue;

                  /* all_queue_list contains all queue instances with state and full queue name only */
                  if (!what_queue3)
                     what_queue3 = lWhat("%T(%I%I)", lGetListDescr(qinstance_list), QU_full_name, QU_state);
                  t = lSelect("t", qinstance_list, where_what.where_queue, what_queue3);
                  if (t) {
                     if (copy.all_queue_list == NULL)
                        copy.all_queue_list = lCreateList("all", lGetListDescr(t));
                     lAppendList(copy.all_queue_list, t);
                  }

                  t = lSelect("t", qinstance_list, where_what.where_queue, where_what.what_queue2);
                  if (t) {
                     if (copy.queue_list == NULL)
                        copy.queue_list = lCreateList("enabled", lGetListDescr(t));
                     lAppendList(copy.queue_list, t);
                  }

                  t = lSelect("t", qinstance_list, where_what.where_queue2, where_what.what_queue2);
                  if (t) {
                     if (copy.dis_queue_list == NULL)
                        copy.dis_queue_list = lCreateList("disabled", lGetListDescr(t));
                     lAppendList(copy.dis_queue_list, t);
                  }
               }
               if (what_queue3)
                  lFreeWhat(&what_queue3);
            }

            if (sconf_is_job_category_filtering()) {
               copy.job_list = sge_category_job_copy(copy.queue_list, &orders);
            } else {
               copy.job_list = lCopyList("", master_job_list);
            }

            copy.dept_list = lCopyList("", selected_dept_list);
            lFreeList(&selected_dept_list);
            copy.acl_list = lCopyList("", selected_acl_list);
            lFreeList(&selected_acl_list);
            copy.centry_list = lCopyList("", master_centry_list);
            copy.pe_list = lCopyList("", master_pe_list);
            copy.share_tree = lCopyList("", master_sharetree_list);
            copy.user_list = lCopyList("", master_user_list);
            copy.project_list = lCopyList("", master_project_list);
            copy.ckpt_list = lCopyList("", master_ckpt_list);
            copy.hgrp_list = lCopyList("", master_hgrp_list);
            copy.rqs_list = lCopyList("", master_rqs_list);
            copy.ar_list = lCopyList("", master_ar_list);

            /* report number of reduced and raw (in brackets) lists */
            DPRINTF(("Q:%d, AQ:%d J:%d(%d), H:%d(%d), C:%d, A:%d, D:%d, P:%d, CKPT:%d,"
                     " US:%d, PR:%d, RQS:%d, AR:%d, S:nd:%d/lf:%d \n",
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
               lGetNumberOfElem(copy.ar_list),
               lGetNumberOfNodes(NULL, copy.share_tree, STN_children),
               lGetNumberOfLeafs(NULL, copy.share_tree, STN_children)
            ));

            DPRINTF((SFN" starts scheduler run\n", thread_config->thread_name));
            /* EB: TODO: remove last param */
            scheduler_method(evc, &copy, &orders);
            DPRINTF((SFN" exited scheduler run\n", thread_config->thread_name));

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
            lFreeList(&(copy.ar_list));
         }

         {
            u_long32 interval = sconf_get_schedule_interval();
            int submit_s = 1; /*sconf_get_flush_submit_sec();*/
            int finish_s = 1; /*sconf_get_flush_finish_sec();*/

            if (evc->ec_get_edtime(evc) != interval) {
                 evc->ec_set_edtime(evc, interval);
            }

            if (submit_s != stored_submit) {
               evc->ec_set_flush(evc, sgeE_JOB_ADD, true, (submit_s - 1));
               stored_submit = submit_s;
            }

            if (finish_s != stored_finish) {
               evc->ec_set_flush(evc, sgeE_JOB_DEL, true,         (finish_s-1));
               evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE, true, (finish_s-1));
               evc->ec_set_flush(evc, sgeE_JATASK_DEL, true,      (finish_s-1));
               stored_finish = finish_s;
            }
      
            /* 
             * need to sync with timed event thread 
             */
            sge_scheduler_reset_busy(evc);
         }

         thread_output_profiling("scheduler thread profiling summary:\n", &next_prof_output);

         /* pthread cancelation point */
         DPRINTF((SFN" will pass cancelation point\n", thread_config->thread_name));
         pthread_cleanup_push((void (*)(void *))sge_scheduler_cleanup_thread_pool,
                              (void *)&monitor);
         pthread_cleanup_push((void (*)(void *))sge_scheduler_cleanup_monitor,
                              (void *)&monitor);
         pthread_cleanup_push((void (*)(void *))sge_scheduler_cleanup_event_client,
                              (void *)evc);
         cl_thread_func_testcancel(thread_config);
         pthread_cleanup_pop(execute);
         pthread_cleanup_pop(execute);
         pthread_cleanup_pop(execute);
         DPRINTF((SFN" passed cancelation point\n", thread_config->thread_name));
      }
   }
   DPRINTF((SFN" terminated\n", thread_config->thread_name));

   /*
    * Don't add cleanup code here. It will never be executed. Instead register
    * a cleanup function with pthread_cleanup_push()/pthread_cleanup_pop() before 
    * the call of cl_thread_func_testcancel()
    */

   DRETURN(NULL);
}

