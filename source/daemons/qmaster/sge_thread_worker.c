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
#include "sge_qmaster_threads.h"
#include "sgermon.h"
#include "sge_mt_init.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "setup_qmaster.h"
#include "sge_security.h"
#include "sge_manop.h"
#include "sge_mtutil.h"
#include "sge_lock.h"
#include "sge_qmaster_process_message.h"
#include "sge_event_master.h"
#include "sge_persistence_qmaster.h"
#include "sge_reporting_qmaster.h"
#include "sge_qmaster_timed_event.h"
#include "sge_host_qmaster.h"
#include "sge_userprj_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_all_listsL.h"
#include "sge_calendar_qmaster.h"
#include "sge_time.h"
#include "lock.h"
#include "qmaster_heartbeat.h"
#include "shutdown.h"
#include "sge_spool.h"
#include "cl_commlib.h"
#include "sge_uidgid.h"
#include "sge_bootstrap.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_daemons_common.h"
#include "msg_utilib.h"  /* remove once 'sge_daemonize_qmaster' did become 'sge_daemonize' */
#include "sge.h"
#include "sge_qmod_qmaster.h"
#include "reschedule.h"
#include "sge_job_qmaster.h"
#include "sge_profiling.h"
#include "sgeobj/sge_conf.h"
#include "qm_name.h"
#include "setup_path.h"
#include "uti/sge_os.h"
#include "sge_advance_reservation_qmaster.h"
#include "sge_sched_process_events.h"
#include "sge_follow.h"

#include "gdi/sge_gdi_packet.h"
#include "gdi/sge_gdi_packet_queue.h"
#include "gdi/sge_gdi_packet_internal.h"

#include "uti/sge_thread_ctrl.h"

#include "sge_thread_main.h"
#include "sge_thread_worker.h"
#include "sge_qmaster_process_message.h"

static void
sge_worker_cleanup_monitor(monitoring_t *monitor)
{
   DENTER(TOP_LAYER, "sge_worker_cleanup_monitor");
   sge_monitor_free(monitor);
   DRETURN_VOID;
}

void 
sge_worker_initialize(sge_gdi_ctx_class_t *ctx)
{
   const u_long32 max_initial_worker_threads = ctx->get_worker_thread_count(ctx);
   cl_thread_settings_t* dummy_thread_p = NULL;
   int i;   

   DENTER(TOP_LAYER, "sge_worker_initialize");

   sge_init_job_number();
   sge_init_ar_id();
   DPRINTF(("job/ar counter have been initialized\n"));

   reporting_initialize(NULL);
   DPRINTF(("accounting and reporting modlue has been initialized\n"));

   INFO((SGE_EVENT, MSG_QMASTER_THREADCOUNT_US, 
         sge_u32c(max_initial_worker_threads), threadnames[WORKER_THREAD]));
   cl_thread_list_setup(&(Main_Control.worker_thread_pool), "thread pool");
   for (i = 0; i < max_initial_worker_threads; i++) {
      dstring thread_name = DSTRING_INIT;

      sge_dstring_sprintf(&thread_name, "%s%03d", threadnames[WORKER_THREAD], i);
      cl_thread_list_create_thread(Main_Control.worker_thread_pool, &dummy_thread_p,
                                   NULL, sge_dstring_get_string(&thread_name), i, 
                                   sge_worker_main, NULL, NULL);
      sge_dstring_free(&thread_name);
   }
   DRETURN_VOID;
}

void
sge_worker_terminate(sge_gdi_ctx_class_t *ctx)
{
   bool do_final_spooling;

   DENTER(TOP_LAYER, "sge_worker_terminate");

   sge_gdi_packet_queue_wakeup_all_waiting(&Master_Packet_Queue);

   /*
    * trigger pthread_cancel for each thread so that further 
    * shutdown process will be faster
    */
   {
      cl_thread_list_elem_t *thr = NULL;
      cl_thread_list_elem_t *thr_nxt = NULL;

      thr_nxt = cl_thread_list_get_first_elem(Main_Control.worker_thread_pool);
      while ((thr = thr_nxt) != NULL) {
         thr_nxt = cl_thread_list_get_next_elem(thr);

         cl_thread_shutdown(thr->thread_config); 
      }
   }

   /*
    * Shutdown/delete the therads and wait for termination
    */
   {
      cl_thread_settings_t *thread = NULL;

      thread = cl_thread_list_get_first_thread(Main_Control.worker_thread_pool);
      while (thread != NULL) {
         DPRINTF((SFN" gets canceled\n", thread->thread_name));
         cl_thread_list_delete_thread(Main_Control.worker_thread_pool, thread);

         thread = cl_thread_list_get_first_thread(Main_Control.worker_thread_pool);
      }  
      DPRINTF(("all "SFN" threads terminated\n", threadnames[WORKER_THREAD]));
   }

   do_final_spooling = sge_qmaster_do_final_spooling();

   reporting_shutdown(ctx, NULL, do_final_spooling);
   DPRINTF(("accounting and reporting module has been shutdown\n"));

   /*
    * final spooling is only done if the shutdown of the current instance
    * of this master process is not triggered due to the fact that
    * shadowd started another instance which is currently running ...
    *
    * ... in that case we would overwrite data which might have already
    * changed in the second instance of the master
    */
   if (do_final_spooling == true) {
      sge_store_job_number(ctx, NULL, NULL);
      sge_store_ar_id(ctx, NULL, NULL);
      DPRINTF(("job/ar counter were made persistant\n"));
      sge_job_spool(ctx);     /* store qmaster jobs to database */
      sge_userprj_spool(ctx); /* spool the latest usage */
      DPRINTF(("final job and user/project spooling has been triggered\n"));
   }

   sge_shutdown_persistence(NULL);
   DPRINTF(("persistance module has been shutdown\n"));

   DRETURN_VOID;
}

void *
sge_worker_main(void *arg)
{
   bool do_endlessly = true;
   cl_thread_settings_t *thread_config = (cl_thread_settings_t*)arg;
   sge_gdi_ctx_class_t *ctx = NULL;
   monitoring_t monitor;
   time_t next_prof_output = 0;

   DENTER(TOP_LAYER, "sge_worker_main");

   DPRINTF((SFN" started", thread_config->thread_name));
   cl_thread_func_startup(thread_config);
   sge_monitor_init(&monitor, thread_config->thread_name, GDI_EXT, MT_WARNING, MT_ERROR);
   sge_qmaster_thread_init(&ctx, QMASTER, WORKER_THREAD, true);

   /* register at profiling module */
   set_thread_name(pthread_self(), "Worker Thread");
   conf_update_thread_profiling("Worker Thread");
 
   while (do_endlessly) {
      sge_gdi_packet_class_t *packet = NULL;
#ifdef DO_LATE_LOCK
      request_handling_t type = ATOMIC_NONE;
#endif

      /*
       * Wait for packets. As long as packets are available cancelation 
       * of this thread is ignored. The shutdown procedure in the main 
       * thread takes care that packet producers will be terminated 
       * before all worker threads so that this won't be a problem.
       */
      sge_gdi_packet_queue_wait_for_new_packet(&Master_Packet_Queue, &packet);
      if (packet != NULL) {
         sge_gdi_task_class_t *task = packet->first_task;
#ifdef DO_LATE_LOCK
#else
         bool is_only_read_request = true;
#endif

         thread_start_stop_profiling();

         DPRINTF((SFN" waits for atomic slot\n", thread_config->thread_name));

#ifdef DO_LATE_LOCK
         MONITOR_WAIT_TIME((type = eval_gdi_and_block(task)), &monitor);
#else
         /*
          * test if a write lock is neccessary
          */
         task = packet->first_task;
         while (task != NULL) {
            u_long32 command = SGE_GDI_GET_OPERATION(task->command); 

            if (command != SGE_GDI_GET) {
               is_only_read_request = false;
               break;
            }
            task = task->next;            
         }

         /*
          * acquire the correct lock
          */
         if (is_only_read_request) {
            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_READ), &monitor); 
         } else {
            MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), &monitor);
         }
#endif

         /*
          * do the GDI request
          */
         task = packet->first_task;
         while (task != NULL) {
            sge_c_gdi(ctx, packet, task, &(task->answer_list), &monitor);

            task = task->next;
         }
     
         sge_gdi_packet_broadcast_that_handled(packet);

         thread_output_profiling("worker thread profiling summary:\n",
                                 &next_prof_output);

#ifdef DO_LATE_LOCK
         eval_atomic_end(type);
#else
         /*
          * do unlock
          */
         if (is_only_read_request) {
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_READ)
         } else {
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE)
         }
#endif
      } else { 
         int execute = 0;

         /* 
          * pthread cancelation point
          */
         pthread_cleanup_push((void (*)(void *))sge_worker_cleanup_monitor,
                              (void *)&monitor);
         cl_thread_func_testcancel(thread_config);
         pthread_cleanup_pop(execute); 
      }
   }

   /*
    * Don't add cleanup code here. It will never be executed. Instead register
    * a cleanup function with pthread_cleanup_push()/pthread_cleanup_pop() before 
    * and after the call of cl_thread_func_testcancel()
    */

   DRETURN(NULL);
}

