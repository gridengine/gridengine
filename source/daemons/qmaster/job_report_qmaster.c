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

#include "sgermon.h"
#include "sge_log.h"
#include "sge.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_usageL.h"
#include "sge_report_execd.h"
#include "sge_sched.h"
#include "sge_prog.h"
#include "execution_states.h"
#include "sge_feature.h"
#include "job_report_qmaster.h"
#include "job_exit.h"
#include "sge_signal.h"
#include "sge_event_master.h"
#include "sge_job_qmaster.h"
#include "sge_host.h"
#include "sge_give_jobs.h"
#include "sge_pe_qmaster.h"
#include "sge_time.h"
#include "reschedule.h"
#include "msg_daemons_common.h"
#include "msg_qmaster.h"
#include "sge_string.h"
#include "sge_var.h"
#include "sge_job.h"
#include "sge_report.h"

#include "sge_reporting_qmaster.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"
#include "sgeobj/sge_ack.h"

static char *status2str(u_long32 status);

#define is_running(state) (state==JWRITTEN || state==JRUNNING|| state==JWAITING4OSJID)

static char *status2str(
u_long32 status 
) {
   char *s;

   switch (status) {
   case JTRANSFERING:
      s = "JTRANSFERING";
      break;
   case JRUNNING:
      s = "JRUNNING";
      break;
   case JFINISHED:
      s = "JFINISHED";
      break;
   case JIDLE:
      s = "JIDLE";
      break;
   default:
      s = "<unknown>";
      break;
   }

   return s;
}
/* ----------------------------------------

NAME 
   process_job_report

DESCR
   Process 'report' containing a job report list from 
   'commproc' at 'rhost'.

   The 'pb' may get used to collect requests that will be 
   generated in this process. The caller should reply it
   to the sender of this job report list if 'pb' remains
   not empty.

RETURN
   void  because all necessary state changings are done 
         in the apropriate objects

   ---------------------------------------- */
void process_job_report(sge_gdi_ctx_class_t *ctx, lListElem *report,
                       lListElem *hep, char *rhost, char *commproc,
                       sge_pack_buffer *pb, monitoring_t *monitor)
{
   lList* jrl = lGetList(report, REP_list); /* JR_Type */
   lListElem *jep, *jr, *ep, *jatep = NULL; 
   object_description *object_base = object_type_get_object_description();

   DENTER(TOP_LAYER, "process_job_report");

   DPRINTF(("received job report with %d elements:\n", lGetNumberOfElem(jrl)));

   /* 
   ** first process job reports of sub tasks to ensure this we put all these 
   ** job reports to the top of the 'jrl' list this is necessary to ensure 
   ** slave tasks get accounted on a shm machine 
   */
   {
      static lSortOrder *jr_sort_order = NULL;
      if (!jr_sort_order) {
         DPRINTF(("parsing job report sort order\n"));
         jr_sort_order = lParseSortOrderVarArg(JR_Type, "%I-", 
            JR_pe_task_id_str);
      }
      lSortList(jrl, jr_sort_order);
   }

   /*
   ** now check all job reports found in step 1 are 
   ** removed from job report list
   */
   for_each(jr, jrl) {
      const char *queue_name;
      const char *pe_task_id_str = lGetString(jr, JR_pe_task_id_str);
      u_long32 status = 0;
      lListElem *petask = NULL;
      int fret;
      u_long32 jobid, rstate = 0, jataskid = 0;

      jobid = lGetUlong(jr, JR_job_number);
      jataskid = lGetUlong(jr, JR_ja_task_number);
      rstate = lGetUlong(jr, JR_state);

      /* handle protocol to execd for all jobs which are
         already finished and maybe rescheduled */
      /* RU: */
      fret = skip_restarted_job(hep, jr, jobid, jataskid);
      if (fret > 0) {
         if (fret == 2) {
            pack_ack(pb, ACK_SIGNAL_JOB, jobid, jataskid, NULL);
         } else if (fret == 3) {
            pack_ack(pb, ACK_JOB_EXIT, jobid, jataskid, pe_task_id_str);
         }
         continue;
      }

      jep = job_list_locate(*object_base[SGE_TYPE_JOB].list, jobid);
      if (jep != NULL) {
         jatep = lGetElemUlong(lGetList(jep, JB_ja_tasks), JAT_task_number, jataskid);
         if (jatep != NULL) {
            status = lGetUlong(jatep, JAT_status);
         }
      }

#if 0
      /* This is only for debugging */

      if (status & JIDLE) {
         DPRINTF(("job status is JIDLE\n"));
      }
      if (status & JIDLE) {
            DPRINTF(("job status is JIDLE\n"));
      }
      if (status & JHELD) {
            DPRINTF(("job status is JHELD\n"));
      }
      if (status & JMIGRATING) {
            DPRINTF(("job status is JMIGRATING\n"));
      }
      if (status & JQUEUED) {
            DPRINTF(("job status is JQUEUED\n"));
      }
      if (status & JRUNNING) {
            DPRINTF(("job status is JRUNNING\n"));
      }
      if (status & JSUSPENDED) {
            DPRINTF(("job status is JSUSPENDED\n"));
      }
      if (status & JTRANSFERING) {
            DPRINTF(("job status is JTRANSFERING\n"));
      }
      if (status & JDELETED) {
            DPRINTF(("job status is JDELETED\n"));
      }
      if (status & JWAITING) {
            DPRINTF(("job status is JWAITING\n"));
      }
      if (status & JEXITING) {
            DPRINTF(("job status is JEXITING\n"));
      }
      if (status & JWRITTEN) {
            DPRINTF(("job status is JWRITTEN\n"));
      }
      if (status & JWAITING4OSJID) {
            DPRINTF(("job status is JWAITING4OSJID\n"));
      }
      if (status & JERROR) {
            DPRINTF(("job status is JERROR\n"));
      }
      if (status & JFINISHED) {
            DPRINTF(("job status is JFINISHED\n"));
      }
      if (status & JSUSPENDED_ON_THRESHOLD) {
            DPRINTF(("job status is JSUSPENDED_ON_THRESHOLD\n"));
      }
      
      if (status & JSLAVE) {
            DPRINTF(("job status is JSLAVE\n"));
      }
      if (status & JSIMULATED) {
            DPRINTF(("job status is JSIMULATED\n"));
      }
#endif 
      if ((queue_name = lGetString(jr, JR_queue_name)) == NULL) {
         queue_name = MSG_OBJ_UNKNOWNQ;
      }
      
      if (pe_task_id_str != NULL && jep != NULL && jatep != NULL) {
         petask = lGetSubStr(jatep, PET_id, pe_task_id_str, JAT_task_list); 
      }
      
      switch(rstate) {
      case JWRITTEN:
      case JRUNNING:   
      case JWAITING4OSJID:
         if (jep && jatep) {
            lList *answer_list = NULL;

            switch (status) {
            case JTRANSFERING:
            case JRUNNING:   
               /* 
                * If a ja_task was deleted while the execd was down, we'll
                * get a "job running" report when the execd starts up again.
                * The ja_task will be deleted by a timer triggered event
                * (TYPE_SIGNAL_RESEND_EVENT), but this can take up to one
                * minute - better send a kill signal immediately.
                */
               if (ISSET(lGetUlong(jatep, JAT_state), JDELETED)) {
                  DPRINTF(("Received report from "sge_u32"."sge_u32
                           " which is already in \"deleted\" state. "
                           "==> send kill signal\n", jobid, jataskid));

                  pack_ack(pb, ACK_SIGNAL_JOB, jobid, jataskid, NULL);
               }

               if (pe_task_id_str == NULL) {
                    
                  /* store unscaled usage directly in job */
                  lXchgList(jr, JR_usage, lGetListRef(jatep, JAT_usage_list));

                  /* update jobs scaled usage list */
                  lSetList(jatep, JAT_scaled_usage_list, 
                      lCopyList("scaled", lGetList(jatep, JAT_usage_list)));
                  scale_usage(lGetList(hep, EH_usage_scaling_list), 
                              lGetList(jatep, JAT_previous_usage_list),
                              lGetList(jatep, JAT_scaled_usage_list));
                 
                  if (status == JTRANSFERING) { /* got async ack for this job */
                     DPRINTF(("--- transfering job "sge_u32" is running\n", jobid));
                     sge_commit_job(ctx, jep, jatep, jr, COMMIT_ST_ARRIVED, COMMIT_DEFAULT, monitor); /* implicitly sending usage to schedd */
                     cancel_job_resend(jobid, jataskid);
                  } else {
                     /* need to generate a job event for new usage 
                      * the timestamp should better come from report object
                      */
                     /* jatask usage is not spooled (?) */
                     sge_add_list_event( 0, sgeE_JOB_USAGE, 
                                        jobid, jataskid, NULL, NULL,
                                        lGetString(jep, JB_session),
                                        lGetList(jatep, JAT_scaled_usage_list));
                     lList_clear_changed_info(lGetList(jatep, JAT_scaled_usage_list));
                  }
               } else {
                  /* register running task qmaster will log accounting for all registered tasks */
                  lListElem *pe;
                  bool new_task = false;

                  /* do we expect a pe task report from this host? */
                  if (lGetString(jatep, JAT_granted_pe)
                        && (pe=pe_list_locate(*object_base[SGE_TYPE_PE].list, lGetString(jatep, JAT_granted_pe)))
                        && lGetBool(pe, PE_control_slaves)
                        && lGetElemHost(lGetList(jatep, JAT_granted_destin_identifier_list), JG_qhostname, rhost)) {
                    
                    /* is the task already known (object was created earlier)? */
                    if (petask == NULL) {
                        /* here qmaster hears the first time about this task
                           and thus adds it to the task list of the appropriate job */
                        new_task = true;
                        DPRINTF(("--- task (#%d) "sge_u32"/%s -> running\n", 
                           lGetNumberOfElem(lGetList(jatep, JAT_task_list)), jobid, pe_task_id_str));
                        petask = lAddSubStr(jatep, PET_id, pe_task_id_str, JAT_task_list, PET_Type);
                        lSetUlong(petask, PET_status, JRUNNING);
                        /* JG: TODO: this should be delivered from execd! */
                        lSetUlong(petask, PET_start_time, sge_get_gmt());
                        lSetList(petask, PET_granted_destin_identifier_list, NULL);
                        if ((ep=lAddSubHost(petask, JG_qhostname, rhost, PET_granted_destin_identifier_list, JG_Type))) {
                           lSetString(ep, JG_qname, queue_name);
                        }
                    }

                    /* store unscaled usage directly in sub-task */
                    lXchgList(jr, JR_usage, lGetListRef(petask, PET_usage));

                    /* update task's scaled usage list */
                    lSetList(petask, PET_scaled_usage,
                             lCopyList("scaled", lGetList(petask, PET_usage)));

                    scale_usage(lGetList(hep, EH_usage_scaling_list), 
                                lGetList(petask, PET_previous_usage),
                                lGetList(petask, PET_scaled_usage));

                    /* notify scheduler of task usage event */
                    if (new_task) {
                       sge_event_spool(ctx,
                                       &answer_list, 0, sgeE_PETASK_ADD, 
                                       jobid, jataskid, pe_task_id_str, NULL,
                                       lGetString(jep, JB_session),
                                       jep, jatep, petask, true, true);
                    } else {
                       sge_add_list_event( 0, sgeE_JOB_USAGE, 
                                          jobid, jataskid, pe_task_id_str, 
                                          NULL, lGetString(jep, JB_session),
                                          lGetList(petask, PET_scaled_usage));
                    }
                    answer_list_output(&answer_list);
                  } else if (lGetUlong(jatep, JAT_status) != JFINISHED) {
                     lListElem *jg;
                     const char *shouldbe_queue_name;
                     const char *shouldbe_host_name;

                     if (!(jg = lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)))) {
                        shouldbe_queue_name = MSG_OBJ_NOTRUNNING;
                        shouldbe_host_name = MSG_OBJ_NOTRUNNING;
                     } else {
                        if ((shouldbe_queue_name = lGetString(jg, JG_qname)) == NULL) {
                           shouldbe_queue_name = MSG_OBJ_UNKNOWN;
                        }
                        if ((shouldbe_host_name = lGetString(jg, JG_qhostname)) == NULL) {
                           shouldbe_host_name = MSG_OBJ_UNKNOWN;
                        }
                     }
                     /* should never happen */
                     ERROR((SGE_EVENT, MSG_JOB_REPORTEXITQ_SUUSSSSS, 
                            rhost, sge_u32c(jobid), sge_u32c(jataskid), 
                            pe_task_id_str?pe_task_id_str:MSG_MASTER, 
                            queue_name, shouldbe_queue_name, 
                            shouldbe_host_name, 
                            status2str(lGetUlong(jatep, JAT_status))));
                  }
               }

               /* once a day write an intermediate usage record to the 
                * reporting file to have correct daily usage reporting with
                * long running jobs */
               if (reporting_is_intermediate_acct_required(jep, jatep, petask)) {
                  /* write intermediate usage */
                  reporting_create_acct_record(ctx, NULL, jr, jep, jatep, true);

                  /* this action has changed the ja_task/pe_task - spool */
                  if (pe_task_id_str != NULL) {
                     /* JG: TODO we would need a PETASK_MOD event here!
                      * for spooling only, the ADD event is OK
                      */
                     sge_event_spool(ctx,
                                     &answer_list, 0, sgeE_PETASK_ADD, 
                                     jobid, jataskid, pe_task_id_str, NULL,
                                     lGetString(jep, JB_session),
                                     jep, jatep, petask, false, true);
                  } else {
                     sge_event_spool(ctx,
                                     &answer_list, 0, sgeE_JATASK_MOD, 
                                     jobid, jataskid, NULL, NULL,
                                     lGetString(jep, JB_session),
                                     jep, jatep, NULL, false, true);
                  }
                  answer_list_output(&answer_list);
               }
               break;
            default:
               ERROR((SGE_EVENT, MSG_JOB_REPORTRUNQ_SUUSSU, 
                     rhost, sge_u32c(jobid), sge_u32c(jataskid), 
                     pe_task_id_str?pe_task_id_str:"master", 
                     queue_name, sge_u32c(status)));
               break;
            } 
         } else {
            /* execd reports a running job that is unknown */
            /* signal this job to kill it at execd 
               this can be caused by a qdel -f while 
               execd was unreachable or by deletion of 
               the job in qmasters spool dir + qmaster 
               restart  
               retry is triggered if execd reports
               this job again as running
            */
            ERROR((SGE_EVENT, MSG_JOB_REPORTRUNFALSE_SUUSS, rhost, 
                   sge_u32c(jobid), sge_u32c(jataskid), 
                   pe_task_id_str?pe_task_id_str:MSG_MASTER, queue_name));
            pack_ack(pb, ACK_SIGNAL_JOB, jobid, jataskid, NULL);
         }
         break;
         
      case JSLAVE:
         /* we might get load reports of pe slaves, which have finished
            during a load report interval. We do not have any flushing
            for slave load reports or any finish reports for them. If
            the scheduler is fast, it might have send a remove order for
            the job. We then get a load report for a job / task, which
            does not exist anymore. 
            I do nto know, if we have to send a job exit request, but at
            least we have to ignore the load report.
          */  
         if (!jep || !jatep) {
            DPRINTF(("send cleanup request for slave job "sge_u32"."sge_u32"\n", 
               jobid, jataskid));
            pack_ack(pb, ACK_JOB_EXIT, jobid, jataskid, pe_task_id_str);
         } else {
            /* must be ack for slave job */
            lListElem *first_at_host;

            first_at_host = lGetElemHost(lGetList(jatep, JAT_granted_destin_identifier_list), JG_qhostname, rhost);
            if (first_at_host) {
               if (lGetUlong(first_at_host, JG_tag_slave_job) != 0) {

                  DPRINTF(("slave job "sge_u32" arrived at %s\n", jobid, rhost));
                  lSetUlong(first_at_host, JG_tag_slave_job, 0);

                  /* should trigger a fast delivery of the job to master execd 
                     script but only when all other slaves have also arrived */ 
                  if (is_pe_master_task_send(jatep)) {
                     /* triggers direct job delivery to master execd */
                     lSetString(jatep, JAT_master_queue, lGetString( lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), JG_qname));

                     DPRINTF(("trigger retry of job delivery to master execd\n"));
                     lSetUlong(jatep, JAT_start_time, 0);
                     cancel_job_resend(jobid, jataskid);
                     trigger_job_resend(sge_get_gmt(), NULL, jobid, jataskid, 0);
                  }
               }
            } else {
               /* clear state with regards to slave controlled container */
               lListElem *host;

               host = host_list_locate(*object_base[SGE_TYPE_EXECHOST].list, rhost);
               update_reschedule_unknown_list_for_job(host, jobid, jataskid);

               DPRINTF(("RU: CLEANUP FOR SLAVE JOB "sge_u32"."sge_u32" on host "SFN"\n", 
                  jobid, jataskid, rhost));

               /* clean up */
               pack_ack(pb, ACK_JOB_EXIT, jobid, jataskid, pe_task_id_str);
            }
         }
         break;
      case JEXITING:   
      {
         int skip_job_exit = 0;

         if (!jep || !jatep || (jep && status==JFINISHED)) {
            /* must be retry of execds job exit */ 
            /* or job was deleted using "qdel -f" */
            /* while execd was down or .. */
            if (!jatep) {
               DPRINTF(("exiting job "sge_u32" does not exist\n", jobid));
            } else {
               DPRINTF(("exiting job "sge_u32"."sge_u32" does not exist\n", jobid, jataskid));
            }   
         } else {
            /* job exited */  
            if (!pe_task_id_str) {

               /* store unscaled usage directly in job */
               lXchgList(jr, JR_usage, lGetListRef(jatep, JAT_usage_list));

               /* update jobs scaled usage list */
               lSetList(jatep, JAT_scaled_usage_list, 
                  lCopyList("scaled", lGetList(jatep, JAT_usage_list)));
               scale_usage(lGetList(hep, EH_usage_scaling_list),
                           lGetList(jatep, JAT_previous_usage_list),
                           lGetList(jatep, JAT_scaled_usage_list));
               /* skip sge_job_exit() and pack_job_exit() in case there 
                  are still running tasks, since execd resends job exit */
               for_each (petask, lGetList(jatep, JAT_task_list)) {
#if 0
                  /* This is only for debugging */

                  u_long32 tmp_PET_status = lGetUlong(petask, PET_status);
                  DPRINTF(("tmp_PET_status = "sge_U32CFormat"\n", sge_u32c(tmp_PET_status)));
                  {
                     if (tmp_PET_status & JIDLE) {
                        DPRINTF(("job tmp_PET_status is JIDLE\n"));
                     }
                     if (tmp_PET_status & JIDLE) {
                           DPRINTF(("job tmp_PET_status is JIDLE\n"));
                     }
                     if (tmp_PET_status & JHELD) {
                           DPRINTF(("job tmp_PET_status is JHELD\n"));
                     }
                     if (tmp_PET_status & JMIGRATING) {
                           DPRINTF(("job tmp_PET_status is JMIGRATING\n"));
                     }
                     if (tmp_PET_status & JQUEUED) {
                           DPRINTF(("job tmp_PET_status is JQUEUED\n"));
                     }
                     if (tmp_PET_status & JRUNNING) {
                           DPRINTF(("job tmp_PET_status is JRUNNING\n"));
                     }
                     if (tmp_PET_status & JSUSPENDED) {
                           DPRINTF(("job tmp_PET_status is JSUSPENDED\n"));
                     }
                     if (tmp_PET_status & JTRANSFERING) {
                           DPRINTF(("job tmp_PET_status is JTRANSFERING\n"));
                     }
                     if (tmp_PET_status & JDELETED) {
                           DPRINTF(("job tmp_PET_status is JDELETED\n"));
                     }
                     if (tmp_PET_status & JWAITING) {
                           DPRINTF(("job tmp_PET_status is JWAITING\n"));
                     }
                     if (tmp_PET_status & JEXITING) {
                           DPRINTF(("job tmp_PET_status is JEXITING\n"));
                     }
                     if (tmp_PET_status & JWRITTEN) {
                           DPRINTF(("job tmp_PET_status is JWRITTEN\n"));
                     }
                     if (tmp_PET_status & JWAITING4OSJID) {
                           DPRINTF(("job tmp_PET_status is JWAITING4OSJID\n"));
                     }
                     if (tmp_PET_status & JERROR) {
                           DPRINTF(("job tmp_PET_status is JERROR\n"));
                     }
                     if (tmp_PET_status & JFINISHED) {
                           DPRINTF(("job tmp_PET_status is JFINISHED\n"));
                     }
                     if (tmp_PET_status & JSUSPENDED_ON_THRESHOLD) {
                           DPRINTF(("job tmp_PET_status is JSUSPENDED_ON_THRESHOLD\n"));
                     }
                     
                     if (tmp_PET_status & JSLAVE) {
                           DPRINTF(("job tmp_PET_status is JSLAVE\n"));
                     }
                     if (tmp_PET_status & JSIMULATED) {
                           DPRINTF(("job tmp_PET_status is JSIMULATED\n"));
                     }
                  }
#endif
                  if (lGetUlong(petask, PET_status)==JRUNNING) {
                     DPRINTF(("job exit for job "sge_u32": still waiting for task %s\n", 
                        jobid, lGetString(petask, PET_id)));
                     skip_job_exit = 1;
                     break;
                  }
               }

               switch (status) {
               case JRUNNING:
               case JTRANSFERING:
                  if (!skip_job_exit) {
                     DPRINTF(("--- running job "sge_u32"."sge_u32" is exiting\n", 
                        jobid, jataskid, (status==JTRANSFERING)?"transfering":"running"));

                     sge_job_exit(ctx, jr, jep, jatep, monitor);
                  } else {
                     u_long32 failed = lGetUlong(jr, JR_failed);

                     if (failed == SSTATE_FAILURE_AFTER_JOB && 
                           !lGetString(jep, JB_checkpoint_name)) {

                        if (!ISSET(lGetUlong(jatep, JAT_state), JDELETED)) {
                           dstring id_dstring = DSTRING_INIT;
                           job_mark_job_as_deleted(ctx, jep, jatep);
                           ERROR((SGE_EVENT, MSG_JOB_MASTERTASKFAILED_S, 
                                  job_get_id_string(jobid, jataskid, NULL, &id_dstring)));
                           sge_dstring_free(&id_dstring);
                        }
                     }
                  }
                  break;
               case JFINISHED:
                  /* must be retry */
                  skip_job_exit = 1;
                  break;
               default:
                  ERROR((SGE_EVENT, MSG_JOB_REPORTEXITJ_UUU,
                        sge_u32c(jobid), sge_u32c(jataskid), sge_u32c(status)));
                  break;
               }
            } else {
               lListElem *pe;
               if (lGetString(jatep, JAT_granted_pe)
                  && (pe=pe_list_locate(*object_base[SGE_TYPE_PE].list, lGetString(jatep, JAT_granted_pe)))
                  && lGetBool(pe, PE_control_slaves)
                  && lGetElemHost(lGetList(jatep, JAT_granted_destin_identifier_list), JG_qhostname, rhost)) {
                  /* 
                   * here we get usage of tasks that ran on slave/master execd's
                   * we store the pe task id of finished pe tasks in the ja task
                   * to prevent multiple handling of pe task finish in case 
                   * execd resends job report.
                   */

                  if (ja_task_add_finished_pe_task(jatep, pe_task_id_str)) {
                     bool known_pe_task = true; /* did this pe task show up
                                                   earlier (USAGE report) */

                     if (petask == NULL) {
                        known_pe_task = false;
                        petask = lAddSubStr(jatep, PET_id, pe_task_id_str, 
                                            JAT_task_list, PET_Type);
                        lSetUlong(petask, PET_status, JRUNNING);
                     }

                     /* store unscaled usage directly in sub-task */
                     /* lXchgList(jr, JR_usage, lGetListRef(task, JB_usage_list)); */
                     /* copy list because we need to keep usage in jr for sge_log_dusage() */
                     lSetList(petask, PET_usage, lCopyList(NULL, lGetList(jr, JR_usage)));

                     /* update task's scaled usage list */
                     lSetList(petask, PET_scaled_usage,
                              lCopyList("scaled", lGetList(petask, PET_usage)));
                     scale_usage(lGetList(hep, EH_usage_scaling_list), 
                                 lGetList(petask, PET_previous_usage),
                                 lGetList(petask, PET_scaled_usage));


                     if (lGetUlong(petask, PET_status)==JRUNNING ||
                         lGetUlong(petask, PET_status)==JTRANSFERING) {
                        u_long32 failed = lGetUlong(jr, JR_failed);

                        DPRINTF(("--- petask "sge_u32"."sge_u32"/%s -> final usage\n", 
                           jobid, jataskid, pe_task_id_str));
                        lSetUlong(petask, PET_status, JFINISHED);

                        reporting_create_acct_record(ctx, NULL, jr, jep, jatep, 
                                                     false);

                        /* add tasks (scaled) usage to past usage container */
                        {
                           lListElem *container = lGetSubStr(jatep, PET_id, PE_TASK_PAST_USAGE_CONTAINER, JAT_task_list);
                           if (container == NULL) {
                              lList *answer_list = NULL;
                              container = pe_task_sum_past_usage_list(lGetList(jatep, JAT_task_list), petask);
                              /* usage container will be spooled */
                              sge_event_spool(ctx,
                                            &answer_list, 0, sgeE_PETASK_ADD, 
                                            jobid, jataskid, PE_TASK_PAST_USAGE_CONTAINER, NULL, lGetString(jep, JB_session),  
                                            jep, jatep, container, true, true);
                              answer_list_output(&answer_list);
                           } else {
                              lList *answer_list = NULL;

                              pe_task_sum_past_usage(container, petask);
                              /* usage container will not be spooled (?) */
                              sge_add_list_event(0, sgeE_JOB_USAGE, 
                                                 jobid, jataskid, 
                                                 PE_TASK_PAST_USAGE_CONTAINER, 
                                                 NULL,
                                                 lGetString(jep, JB_session),
                                                 lGetList(container, PET_scaled_usage));
                              /* usage container will be spooled */
                              /* JG: TODO: it is not really a sgeE_PETASK_ADD,
                               * but a sgeE_PETASK_MOD. We don't have this event
                               * yet. For spooling only, the add event will do
                               */
                              sge_event_spool(ctx,
                                            &answer_list, 0, sgeE_PETASK_ADD, 
                                            jobid, jataskid, PE_TASK_PAST_USAGE_CONTAINER, NULL, lGetString(jep, JB_session),  
                                            jep, jatep, container, false, true);
                              answer_list_output(&answer_list);
                           }
                        }

                        /* remove pe task from job/jatask */
                        if (known_pe_task) {
                           lList *answer_list = NULL;
                           sge_event_spool(ctx,
                                           &answer_list, 0, sgeE_PETASK_DEL, 
                                          jobid, jataskid, pe_task_id_str, 
                                          NULL, NULL, NULL, NULL, NULL, 
                                          true, true);
                           answer_list_output(&answer_list);
                        }
                        lRemoveElem(lGetList(jatep, JAT_task_list), &petask);
                        
                        /* get rid of this job in case a task died from XCPU/XFSZ or 
                           exited with a core dump */
                        if (failed==SSTATE_FAILURE_AFTER_JOB
                              && (ep=lGetElemStr(lGetList(jr, JR_usage), UA_name, "signal"))) {
                           u_long32 sge_signo = (u_long32)lGetDouble(ep, UA_value);

                           switch (sge_signo) {
                           case SGE_SIGXFSZ:
                              INFO((SGE_EVENT, MSG_JOB_FILESIZEEXCEED_SSUU, 
                                   pe_task_id_str, rhost, sge_u32c(jobid), sge_u32c(jataskid)));
                              break;
                           case SGE_SIGXCPU:
                              INFO((SGE_EVENT, MSG_JOB_CPULIMEXCEED_SSUU, 
                                    pe_task_id_str, rhost, sge_u32c(jobid), sge_u32c(jataskid)));
                              break;
                           default: 
                              INFO((SGE_EVENT, MSG_JOB_DIEDTHROUGHSIG_SSUUS, 
                                   pe_task_id_str, rhost, sge_u32c(jobid), sge_u32c(jataskid), sge_sig2str(sge_signo)));
                              break;
                           }   
                        } else if (failed==0) {
                           INFO((SGE_EVENT, MSG_JOB_TASKFINISHED_SSUU, 
                              pe_task_id_str, rhost, sge_u32c(jobid), sge_u32c(jataskid)));
                        } else {
                           INFO((SGE_EVENT, MSG_JOB_TASKFAILED_SSUUU,
                              pe_task_id_str, rhost, sge_u32c(jobid), sge_u32c(jataskid), sge_u32c(failed)));
                        }

                        if (failed == SSTATE_FAILURE_AFTER_JOB && 
                              !lGetString(jep, JB_checkpoint_name)) {
                           if (!ISSET(lGetUlong(jatep, JAT_state), JDELETED)) {
                              dstring id_dstring = DSTRING_INIT;
                              job_mark_job_as_deleted(ctx, jep, jatep);
                              ERROR((SGE_EVENT, MSG_JOB_JOBTASKFAILED_S, 
                                     job_get_id_string(jobid, jataskid, pe_task_id_str, &id_dstring)));
                              sge_dstring_free(&id_dstring);
                           }
                        }
                     }
#if 0 
               /*
                * CR: TODO: (Issue Bug #1197) 
                * This partly solves some problems when slave tasks of a tight integrated
                * job, running on more than one execd, are killed and qmaster has to kill and
                * delete all tasks.
                * 
                * There seems to be a problem when the slave task on one host is killed and
                * the other host has not reported the job start yet.
                *
                */

               /* skip sge_job_exit() and pack_job_exit() in case there 
                  are still running tasks, since execd resends job exit */

               for_each (petask, lGetList(jatep, JAT_task_list)) {
                  u_long32 tmp_PET_status = lGetUlong(petask, PET_status);
                  DPRINTF(("tmp_PET_status = "sge_U32CFormat"\n", sge_u32c(tmp_PET_status)));

                   {
                     if (tmp_PET_status & JIDLE) {
                        DPRINTF(("job tmp_PET_status is JIDLE\n"));
                     }
                     if (tmp_PET_status & JIDLE) {
                           DPRINTF(("job tmp_PET_status is JIDLE\n"));
                     }
                     if (tmp_PET_status & JHELD) {
                           DPRINTF(("job tmp_PET_status is JHELD\n"));
                     }
                     if (tmp_PET_status & JMIGRATING) {
                           DPRINTF(("job tmp_PET_status is JMIGRATING\n"));
                     }
                     if (tmp_PET_status & JQUEUED) {
                           DPRINTF(("job tmp_PET_status is JQUEUED\n"));
                     }
                     if (tmp_PET_status & JRUNNING) {
                           DPRINTF(("job tmp_PET_status is JRUNNING\n"));
                     }
                     if (tmp_PET_status & JSUSPENDED) {
                           DPRINTF(("job tmp_PET_status is JSUSPENDED\n"));
                     }
                     if (tmp_PET_status & JTRANSFERING) {
                           DPRINTF(("job tmp_PET_status is JTRANSFERING\n"));
                     }
                     if (tmp_PET_status & JDELETED) {
                           DPRINTF(("job tmp_PET_status is JDELETED\n"));
                     }
                     if (tmp_PET_status & JWAITING) {
                           DPRINTF(("job tmp_PET_status is JWAITING\n"));
                     }
                     if (tmp_PET_status & JEXITING) {
                           DPRINTF(("job tmp_PET_status is JEXITING\n"));
                     }
                     if (tmp_PET_status & JWRITTEN) {
                           DPRINTF(("job tmp_PET_status is JWRITTEN\n"));
                     }
                     if (tmp_PET_status & JWAITING4OSJID) {
                           DPRINTF(("job tmp_PET_status is JWAITING4OSJID\n"));
                     }
                     if (tmp_PET_status & JERROR) {
                           DPRINTF(("job tmp_PET_status is JERROR\n"));
                     }
                     if (tmp_PET_status & JFINISHED) {
                           DPRINTF(("job tmp_PET_status is JFINISHED\n"));
                     }
                     if (tmp_PET_status & JSUSPENDED_ON_THRESHOLD) {
                           DPRINTF(("job tmp_PET_status is JSUSPENDED_ON_THRESHOLD\n"));
                     }
                     
                     if (tmp_PET_status & JSLAVE) {
                           DPRINTF(("job tmp_PET_status is JSLAVE\n"));
                     }
                     if (tmp_PET_status & JSIMULATED) {
                           DPRINTF(("job tmp_PET_status is JSIMULATED\n"));
                     }
                   }
                   if (lGetUlong(petask, PET_status)==JRUNNING) {
                     DPRINTF(("job exit for job "sge_u32": still waiting for task %s\n", 
                        jobid, lGetString(petask, PET_id)));
                     skip_job_exit = 1;
                     break;
                   } else {
                     DPRINTF(("SKIP check for other tasks, PET_status is not JRUNNING\n"));
                   }
               }
#endif
                  }
               } else if (status != JFINISHED) {
                  lListElem *jg;
                  const char *shouldbe_queue_name;
                  const char *shouldbe_host_name;

                  if (!(jg = lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)))) {
                     shouldbe_queue_name = MSG_OBJ_NOTRUNNING;
                     shouldbe_host_name = MSG_OBJ_NOTRUNNING;
                  } else {
                     if ((shouldbe_queue_name = lGetString(jg, JG_qname)) == NULL) {
                        shouldbe_queue_name = MSG_OBJ_UNKNOWN;
                     }
                     if ((shouldbe_host_name = lGetString(jg, JG_qhostname)) == NULL) {
                        shouldbe_host_name = MSG_OBJ_UNKNOWN;
                     }
                  }
                  /* should never happen */
                  ERROR((SGE_EVENT, MSG_JOB_REPORTEXITQ_SUUSSSSS, 
                        rhost, sge_u32c(jobid), sge_u32c(jataskid), 
                        pe_task_id_str?pe_task_id_str:MSG_MASTER, queue_name, 
                        shouldbe_queue_name, shouldbe_host_name, 
                        status2str(lGetUlong(jatep, JAT_status))));
               }
            }
         }
         /* pack ack to enable execd cleaning up */
         if (!skip_job_exit) {
            pack_ack(pb, ACK_JOB_EXIT, jobid, jataskid, pe_task_id_str);
         }
      }
         break;
      default:
         ERROR((SGE_EVENT, MSG_EXECD_UNKNOWNJ_SUUSUS, 
                rhost, 
                sge_u32c(jobid), 
                sge_u32c(jataskid), 
                pe_task_id_str?pe_task_id_str:MSG_MASTER, 
                sge_u32c(rstate), 
                queue_name));


         pack_ack(pb, ACK_JOB_EXIT, jobid, jataskid, pe_task_id_str);
         break;
      }
   }


   DRETURN_VOID;
}

