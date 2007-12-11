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
#include <string.h>
#include <pthread.h>

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif  

/* common/ */
#include "basis_types.h" 
#include "sge_mt_init.h" 
#include "sge.h"

/* daemons/qmaster/ */
#include "setup_qmaster.h"
#include "sge_sched_process_events.h"
#include "sge_qmaster_threads.h"
#include "sge_follow.h"

/* lib/ */
#include "rmon/sgermon.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_conf.h"
#include "sgeobj/sge_reportL.h"
#include "sgeobj/sge_schedd_conf.h"
#include "sgeobj/sge_job.h"
#include "sgeobj//sge_jobL.h"
#include "sgeobj/sge_cqueueL.h"
#include "sgeobj/sge_qinstanceL.h"
#include "sgeobj/sge_ja_taskL.h"
#include "sgeobj/sge_usersetL.h"
#include "sgeobj/sge_qinstance_state.h"
#include "sgeobj/sge_userprj.h"
#include "sgeobj/sge_sharetree.h"
#include "sgeobj/sge_host.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_ctL.h"
#include "sgeobj/sge_ckpt.h"
#include "sgeobj/sge_pe.h"
#include "sgeobj/sge_range.h"
#include "lck/sge_mtutil.h"
#include "mir/sge_mirror.h"
#include "evc/sge_event_client.h"
#include "evm/sge_event_master.h"

#include "uti/sge_unistd.h"
#include "uti/sge_prog.h"
#include "uti/sge_log.h"
#include "uti/sge_profiling.h"
#include "uti/sge_time.h"

#include "sge_sched_prepare_data.h"
#include "sge_sched_job_category.h"

#include "sge_orders.h"
#include "sge_job_schedd.h"
#include "sge_serf.h"
#include "schedd_message.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "schedd_monitor.h"
#include "sge_interactive_sched.h"
#include "sge_orderL.h"
#include "sgeee.h"
#include "load_correction.h"
#include "sge_resource_utilization.h"
#include "suspend_thresholds.h"
#include "sge_support.h"
#include "sort_hosts.h"
#include "debit.h"
#include "sge_follow.h"
#include "sge_qmaster_threads.h"

/****** qmaster/sge_thread_scheduler/event_update_func() **************************
*  NAME
*     event_update_func() -- 
*
*  SYNOPSIS
*     void event_update_func(lList **alpp, lList *event_list)
*
*  FUNCTION
*
*  INPUTS
*     lList **alpp - answer list
*     lList *event_list - a report list, the event are stored in REP_list
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: is MT safe. 
*
*******************************************************************************/
void event_update_func(u_long32 ec_id, lList **alpp, lList *event_list) 
{
   DENTER(TOP_LAYER, "event_update_func");

   sge_mutex_lock("event_control_mutex", SGE_FUNC, __LINE__, &Scheduler_Control.mutex);  
   
   if (Scheduler_Control.new_events != NULL) {
      lList *events = NULL;
      lXchgList(lFirst(event_list), REP_list, &(events));
      lAddList(Scheduler_Control.new_events, &events);
      events = NULL;
   } else {
      lXchgList(lFirst(event_list), REP_list, &(Scheduler_Control.new_events));
   }   
   
   Scheduler_Control.triggered = true;

   DPRINTF(("EVENT UPDATE FUNCTION event_update_func() HAS BEEN TRIGGERED\n"));

   sge_mutex_unlock("event_control_mutex", SGE_FUNC, __LINE__, &Scheduler_Control.mutex);

   pthread_cond_signal(&Scheduler_Control.cond_var);

   DRETURN_VOID;
}

/*********************************************/
/*  event client registration stuff          */
/*********************************************/

int subscribe_scheduler(sge_evc_class_t *evc, sge_where_what_t *where_what) {
   int temp;

   DENTER(TOP_LAYER, "subscribe_scheduler");

   /* subscribe event types for the mirroring interface */
   sge_mirror_subscribe(evc, SGE_TYPE_AR,             NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_CKPT,           NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_CENTRY,         NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_CQUEUE,         NULL, NULL, NULL, where_what->where_cqueue, where_what->what_cqueue);
   sge_mirror_subscribe(evc, SGE_TYPE_EXECHOST,       NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_HGROUP,         NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_GLOBAL_CONFIG,  NULL, sge_process_global_config_event, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_JOB,            sge_process_job_event_before, sge_process_job_event_after, NULL, where_what->where_job, where_what->what_job);
   sge_mirror_subscribe(evc, SGE_TYPE_JATASK,         NULL, sge_process_ja_task_event_after, NULL, where_what->where_jat, where_what->what_jat);
   sge_mirror_subscribe(evc, SGE_TYPE_PE,             NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_PETASK,         NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_PROJECT,        sge_process_project_event_before, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_QINSTANCE,      NULL, NULL, NULL, where_what->where_all_queue, where_what->what_queue);
   sge_mirror_subscribe(evc, SGE_TYPE_RQS,            NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_SCHEDD_CONF,    sge_process_schedd_conf_event_before, sge_process_schedd_conf_event_after, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_SCHEDD_MONITOR, NULL, sge_process_schedd_monitor_event, NULL, NULL, NULL); 
   sge_mirror_subscribe(evc, SGE_TYPE_SHARETREE,      NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_USER,           NULL, NULL, NULL, NULL, NULL);
   sge_mirror_subscribe(evc, SGE_TYPE_USERSET,        sge_process_userset_event_before, NULL, NULL, NULL, NULL);

   /* set flush parameters for job */
   temp = sconf_get_flush_submit_sec();
   if (temp <= 0) {
      evc->ec_set_flush(evc, sgeE_JOB_ADD, false, -1);
      /* SG: we might want to have sgeE_JOB_MOD in here to be notified, when
      a job is removed from its hold state */
   } else {
      temp--;
      evc->ec_set_flush(evc, sgeE_JOB_ADD, true, temp);
      /* SG: we might want to have sgeE_JOB_MOD in here to be notified, when
      a job is removed from its hold state */
   }  

   temp = sconf_get_flush_finish_sec();
   if (temp <= 0){
      evc->ec_set_flush(evc, sgeE_JOB_DEL, false, -1);
      evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE, false, -1);
      evc->ec_set_flush(evc, sgeE_JATASK_DEL, false, -1);
   } else {
      temp--;
      evc->ec_set_flush(evc, sgeE_JOB_DEL, true, temp);
      evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE, true, temp);
      evc->ec_set_flush(evc, sgeE_JATASK_DEL, true, temp);
   }

   /* for some reason we flush sharetree changes */
   evc->ec_set_flush(evc, sgeE_NEW_SHARETREE, true, 0);

   /* configuration changes and trigger should have immediate effevc->ect */
   evc->ec_set_flush(evc, sgeE_SCHED_CONF, true, 0);
   evc->ec_set_flush(evc, sgeE_SCHEDDMONITOR, true, 0);
   evc->ec_set_flush(evc, sgeE_GLOBAL_CONFIG,true, 0);

   DRETURN(true);
}

/* do everything that needs to be done in common for all schedulers 
   between processing events and dispatching */
int sge_before_dispatch(sge_evc_class_t *evc)
{     
   sge_gdi_ctx_class_t *ctx = evc->get_gdi_ctx(evc);
   const char *cell_root = ctx->get_cell_root(ctx);
   u_long32 progid = ctx->get_who(ctx);
   
   DENTER(TOP_LAYER, "sge_before_dispatch");

   /* hostname resolving scheme in global config could have changed
      get it and use it if we got a notification about a new global config */
   if (st_get_flag_new_global_conf()) {
      lListElem *global = NULL, *local = NULL;
   
      if (gdi2_get_configuration(ctx, SGE_GLOBAL_NAME, &global, &local) == 0) {
         merge_configuration(NULL, progid, cell_root, global, local, NULL);
      }  
      lFreeElem(&global);
      lFreeElem(&local); 
      st_set_flag_new_global_conf(false);
   }  
      
   if (sconf_is_new_config()) {
      int interval = sconf_get_flush_finish_sec();
      bool flush = (interval > 0) ? true : false;
      interval--;
      if (evc->ec_get_flush(evc, sgeE_JOB_DEL) != interval) {
         evc->ec_set_flush(evc, sgeE_JOB_DEL, flush, interval);
         evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE, flush, interval);
         evc->ec_set_flush(evc, sgeE_JATASK_MOD, flush, interval);
         evc->ec_set_flush(evc, sgeE_JATASK_DEL, flush, interval);
      }     
         
      interval= sconf_get_flush_submit_sec();
      flush = (interval > 0) ? true : false;
      interval--;
      if(evc->ec_get_flush(evc, sgeE_JOB_ADD) != interval) {
         evc->ec_set_flush(evc, sgeE_JOB_ADD, flush, interval);
      }
      evc->ec_commit(evc, NULL);
   }

   /*
    * job categories are reset here, we need 
    *  - an update of the rejected field for every new run
    *  - the resource request dependent urgency contribution is cached 
    *    per job category 
    */
   sge_reset_job_category();

   DRETURN(0);
}

