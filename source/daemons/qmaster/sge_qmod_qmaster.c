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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <limits.h>
#include <fnmatch.h>

#include "sge.h"
#include "def.h"
#include "sge_queueL.h"
#include "symbols.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_stringL.h"
#include "sge_identL.h"
#include "sge_answerL.h"
#include "sge_eventL.h"
#include "sge_hostL.h"
#include "sge_peL.h"
#include "sec.h"
#include "sge_signal.h"
#include "sge_prognames.h"
#include "sge_qmod_qmaster.h"
#include "read_write_job.h"
#include "read_write_queue.h"
#include "sge_job.h"
#include "sge_give_jobs.h"
#include "sge_host.h"
#include "sge_parse_num_par.h"
#include "sge_queue_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_isint.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_rangeL.h"
#include "job.h"
#include "time_event.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "reschedule.h"
#include "sge_security.h"

extern lList *Master_Queue_List;
extern lList *Master_Job_List;


/*-------------------------------------------------------------------------*/
static void signal_slave_jobs_in_queue(int how, lListElem *jep);

static void signal_slave_tasks_of_job(int how, lListElem *jep, lListElem *jatep);

static int sge_change_queue_state(char *user, char *host, lListElem *qep, u_long32 action, u_long32 force, lList **answer);

static int sge_change_job_state(char *user, char *host, lListElem *jep, lListElem *jatep, u_long32 action, u_long32 force, lList **answer);

static int qmod_queue_clear(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static int qmod_queue_enable(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static int qmod_queue_disable(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static int qmod_queue_weakclean(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);  

static int qmod_queue_suspend(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static int qmod_queue_unsuspend(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static int qmod_queue_clean(lListElem *qep, u_long32 force, lList **answer, char *user, char *host, int isoperator, int isowner);

static void qmod_job_suspend(lListElem *jep, lListElem *jatep, lListElem *queueep, u_long32 force, lList **answer, char *user, char *host);

static void qmod_job_unsuspend(lListElem *jep, lListElem *jatep, lListElem *queueep, u_long32 force, lList **answer, char *user, char *host);

static void qmod_job_reschedule(lListElem *jep, lListElem *jatep, lListElem *queueep, u_long32 force, lList **answer, char *user, char *host);

/*-------------------------------------------------------------------------*/

void sge_gdi_qmod(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer 
) {
   lList *alp = NULL;
   lListElem *dep;
   lListElem *qep2;
   lListElem *jatask = NULL, *rn, *job, *tmp_task;
   int found;
   u_long32 jobid;
   u_long32 start = 0, end = 0, step = 0;
   int alltasks;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   
   DENTER(TOP_LAYER, "sge_gdi_qmod");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (!request->host || !user || !request->commproc ||
       !request->id) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return;
   }

   /*
   ** loop over the ids and change queue or job state and signal them
   ** if necessary
   */
   for_each(dep, request->lp) {
      found = FALSE;
      for_each(qep2, Master_Queue_List) {
         if (!fnmatch(lGetString(dep, ID_str), lGetString(qep2, QU_qname), 0)) {

            /* change queue state: */
            sge_change_queue_state(user, host, qep2, 
                  lGetUlong(dep, ID_action), lGetUlong(dep, ID_force),
                  &alp);   
            found = TRUE;
         }
      }
      if (!found) {
         /* 
         ** We found no queue so look for a job. This only makes sense for
         ** suspend, unsuspend and reschedule
         */
         if (isint(lGetString(dep, ID_str)) && 
               (lGetUlong(dep, ID_action) == QSUSPENDED || 
                lGetUlong(dep, ID_action) == QRESCHEDULED ||
                lGetUlong(dep, ID_action) == QERROR || 
                lGetUlong(dep, ID_action) == QRUNNING)) {
            jobid = strtol(lGetString(dep, ID_str), NULL, 10);

            rn = lFirst(lGetList(dep, ID_ja_structure));
            if (rn) {
               start = lGetUlong(rn, RN_min);
               if (start) {
                  end = lGetUlong(rn, RN_max);
                  step = lGetUlong(rn, RN_step);
                  if (!step)
                     step = 1;
                  alltasks = 0;
               } else {
                  start = 1;
                  end = (u_long32)LONG_MAX;
                  step = 1;
                  alltasks = 1;
               }
               if (start > end)
                  end = start;

            } else {
               alltasks = 1;
            }

            job = sge_locate_job(jobid);
            if (job)
               jatask = lFirst(lGetList(job, JB_ja_tasks));
            while ((tmp_task = jatask)) {
               u_long32 task_number;

               jatask = lNext(tmp_task);
               task_number = lGetUlong(tmp_task, JAT_task_number);
               if ((task_number >= start && task_number <= end &&
                  ((task_number-start)%step) == 0) || alltasks) {
                  DPRINTF(("Modify job: "u32"."u32"\n", jobid,
                     task_number));

                  /* this specifies no queue, so lets prove for a job */
                  /* change state of job: */
                  sge_change_job_state(user, host, job, tmp_task,
                      lGetUlong(dep, ID_action), lGetUlong(dep, ID_force), &alp);   
                  found = TRUE;
               }
            }
         }
         else {
            /* job id invalid or action invalid for jobs */
         }
      }

      if (!found) {
         /*
         ** If the action is QRUNNING or QSUSPENDED, 'invalid queue or job' will be printed,
         ** otherwise 'invalid queue' will be printed, because these actions
         ** are not suitable for jobs.
         */
         if ((lGetUlong(dep, ID_action) == QSUSPENDED) || (lGetUlong(dep, ID_action) == QRUNNING))
            WARNING((SGE_EVENT, MSG_QUEUE_INVALIDQORJOB_S, lGetString(dep, ID_str)));
         else
            WARNING((SGE_EVENT, MSG_QUEUE_INVALIDQ_S, lGetString(dep, ID_str)));  
         sge_add_answer(&alp, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      }
   }

   answer->alp = alp;
   
   DEXIT;
}


static int sge_change_queue_state(
char *user,
char *host,
lListElem *qep,
u_long32 action,
u_long32 force,
lList **answer 
) {
   int isoperator;
   int isowner;
   int result = 0;
   
   DENTER(TOP_LAYER, "sge_change_queue_state");

   isowner = !sge_owner(user, lGetList(qep, QU_owner_list));
   isoperator = !sge_operator(user);

   if (!isowner) {
      WARNING((SGE_EVENT, MSG_QUEUE_NOCHANGEQPERMS_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   switch (action) {
      case QERROR:          /* a qmaster "local" state */
         result = qmod_queue_clear(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QENABLED:          /* a qmaster "local" state */
         result = qmod_queue_enable(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QDISABLED: /* a qmaster "local" state */
         result = qmod_queue_disable(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QSUSPENDED:
         result = qmod_queue_suspend(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QRUNNING:
         result = qmod_queue_unsuspend(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QCLEAN:
         result = qmod_queue_clean(qep, force, answer, user, host, isoperator, isowner);
         break;

      case QRESCHEDULED:      /* No queue state! */
         result = qmod_queue_weakclean(qep, force, answer, user, host, isoperator, isowner);
	 break;
      default:
         INFO((SGE_EVENT, MSG_LOG_UNKNOWNQMODCMD_U, u32c(action)));
         sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         break;
   }
   cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep);

   DEXIT;
   return result;
}


static int sge_change_job_state(
char *user,
char *host,
lListElem *jep,
lListElem *jatep,
u_long32 action,
u_long32 force,
lList **answer 
) {
   lListElem *queueep;
   u_long32 job_id, task_id;

   DENTER(TOP_LAYER, "sge_change_job_state");
   
   if (strcmp(user, lGetString(jep, JB_owner)) && sge_operator(user)) {
      WARNING((SGE_EVENT, MSG_JOB_NOMODJOBPERMS_SU, user, u32c(lGetUlong(jep, JB_job_number))));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   job_id = lGetUlong(jep, JB_job_number);
   task_id = lGetUlong(jatep, JAT_task_number);

   /* look for queue this job runs in */
   if (lGetString(jatep, JAT_master_queue))
      queueep = lGetElemStr(Master_Queue_List, QU_qname, lGetString(jatep, JAT_master_queue));
   else
      queueep = NULL;

   switch (action) {
      case QRESCHEDULED:
         qmod_job_reschedule(jep, jatep, queueep, force, answer, user, host);
         break;

      case JSUSPENDED:
         qmod_job_suspend(jep, jatep, queueep, force, answer, user, host);
         break;

      case JRUNNING:
         qmod_job_unsuspend(jep, jatep, queueep, force, answer, user, host);
         break;
         
      case QERROR:
         if (VALID(JERROR, lGetUlong(jatep, JAT_state))) {
            lSetUlong(jatep, JAT_state, lGetUlong(jatep, JAT_state) & ~JERROR);
            sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
            cull_write_job_to_disk(jep);
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_CLEARERRORTASK_SSUU, user, host, u32c(job_id), u32c(task_id)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_CLEARERRORJOB_SSU, user, host, u32c(job_id)));
            }
         } else {
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_NOERRORSTATETASK_UU, u32c(job_id), u32c(task_id)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_NOERRORSTATEJOB_UU, u32c(job_id)));
            }
         }
         sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         break;
         
      default:
         INFO((SGE_EVENT, MSG_LOG_UNKNOWNQMODCMD_U, u32c(action)));
         sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         break;
   }

   DEXIT;
   return 0;
}

/****
 **** qmod_queue_clear (static)
 ****/
static int qmod_queue_clear(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   u_long32 state = 0, old_state = 0;

   DENTER(TOP_LAYER, "qmod_queue_clear");

   if (!isoperator && !isowner) {
      WARNING((SGE_EVENT, MSG_QUEUE_NOCLEARERRORPERMS_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   if ( !VALID(QERROR, lGetUlong(qep, QU_state))) {
      INFO((SGE_EVENT, MSG_QUEUE_NOERRORSTATEQ_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   old_state = state = lGetUlong(qep, QU_state);
   CLEARBIT(QERROR, state);
   lSetUlong(qep, QU_state, state);

   sge_change_queue_version(qep, 0, 0);
   if (cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep) == -1) {
      ERROR((SGE_EVENT, MSG_QUEUE_NOTMODIFIEDSPOOL_S, lGetString(qep, QU_qname))); 
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_ERROR);

      /* rollback */
      lSetUlong(qep, QU_state, old_state);

      DEXIT;
      return -1;
   } else {
      INFO((SGE_EVENT, MSG_QUEUE_CLEARERRORSTATE_SSS, lGetString(qep, QU_qname), user, host ));
      sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
   } 
   DEXIT;
   return 0;
}

/****
 **** qmod_queue_enable (static)
 ****/
static int qmod_queue_enable(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   u_long32 state = 0, old_state = 0;

   DENTER(TOP_LAYER, "qmod_queue_enable");

   if (!isoperator && !isowner) {
      WARNING((SGE_EVENT, MSG_QUEUE_NOENABLEQPERMS_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   if (!VALID(QDISABLED, lGetUlong(qep, QU_state))) {
      INFO((SGE_EVENT, MSG_QUEUE_ALREADYENABLED_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   old_state = state = lGetUlong(qep, QU_state);
   CLEARBIT(QDISABLED, state);
   lSetUlong(qep, QU_state, state);

   sge_change_queue_version(qep, 0, 0);
   if (cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep) == -1) {
      ERROR((SGE_EVENT, MSG_QUEUE_NOTMODIFIEDSPOOL_S, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_ERROR);

      /* rollback */
      lSetUlong(qep, QU_state, old_state);

      DEXIT;
      return -1;
   } else {        
      INFO((SGE_EVENT, MSG_QUEUE_ENABLEQ_SSS, lGetString(qep, QU_qname), user, host));
      sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
   }

   DEXIT;
   return 0;
}

/****
 **** qmod_queue_disable (static)
 ****/
static int qmod_queue_disable(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   u_long32 state = 0, old_state = 0;

   DENTER(TOP_LAYER, "qmod_queue_disable");

   if (!isoperator && !isowner) {
      WARNING((SGE_EVENT, MSG_QUEUE_NODISABLEQPERMS_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   if (VALID(QDISABLED, lGetUlong(qep, QU_state))) {
      INFO((SGE_EVENT, MSG_QUEUE_ALREADYDISABLED_SS, user, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   old_state = state = lGetUlong(qep, QU_state);
   SETBIT(QDISABLED, state);
   lSetUlong(qep, QU_state, state);

   sge_change_queue_version(qep, 0, 0);
   if (cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep) == -1) {
      ERROR((SGE_EVENT, MSG_QUEUE_NOTMODIFIEDSPOOL_S, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_ERROR);

      /* rollback */
      lSetUlong(qep, QU_state, old_state);

      DEXIT;
      return -1;
   } else {        
      INFO((SGE_EVENT, MSG_QUEUE_DISABLEQ_SSS,
         lGetString(qep, QU_qname), user, host));
      sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
   }

   DEXIT;
   return 0;
}

/****
 **** qmod_queue_weakclean (static)
 ****/
static int qmod_queue_weakclean(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   DENTER(TOP_LAYER, "qmod_queue_weakclean");

   if (!isoperator && !isowner) {
      WARNING((SGE_EVENT, MSG_QUEUE_NORESCHEDULEQPERMS_SS, user, 
         lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   reschedule_jobs(qep, force, answer);

   DEXIT;
   return 0;
}
/****
 **** qmod_queue_suspend (static)
 ****/
static int qmod_queue_suspend(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   u_long32 state = 0, old_state = 0;

   DENTER(TOP_LAYER, "qmod_queue_suspend");

   if (VALID(QSUSPENDED, lGetUlong(qep, QU_state))) {
      if (force) {
         if (sge_signal_queue(SGE_SIGSTOP, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOFORCESUSPENDQ_SS, user, lGetString(qep, QU_qname)));
         }
         else {
            WARNING((SGE_EVENT, MSG_QUEUE_FORCESUSPENDQ_SS, user, lGetString(qep, QU_qname)));
         }
      }
      else {
         WARNING((SGE_EVENT, MSG_QUEUE_ALREADYSUSPENDED_SS, user, lGetString(qep, QU_qname)));
      }
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);

      old_state = state = lGetUlong(qep, QU_state);
      SETBIT(QSUSPENDED, state);
      lSetUlong(qep, QU_state, state);

      sge_change_queue_version(qep, 0, 0);
   }
   else {
      if (force) {
         if (sge_signal_queue(SGE_SIGSTOP, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOFORCESUSPENDQ_SS, user, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            INFO((SGE_EVENT, MSG_QUEUE_FORCESUSPENDQ_SS, user, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         }

         old_state = state = lGetUlong(qep, QU_state);
         SETBIT(QSUSPENDED, state);
         lSetUlong(qep, QU_state, state);

         sge_change_queue_version(qep, 0, 0);
      }
      else {
         if ( !VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state)) &&
              !VALID(QCAL_SUSPENDED, lGetUlong(qep, QU_state)) &&
              sge_signal_queue(SGE_SIGSTOP, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOSUSPENDQ_SS, user, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            INFO((SGE_EVENT, MSG_QUEUE_SUSPENDQ_SSS, lGetString(qep, QU_qname), user, host));
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
            
            old_state = state = lGetUlong(qep, QU_state);
            SETBIT(QSUSPENDED, state);
            lSetUlong(qep, QU_state, state);

            sge_change_queue_version(qep, 0, 0);
         }
      }
   }
   if (cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep) == -1) {
      lListElem *tmp_elem;

      /* rollback */
      lSetUlong(qep, QU_state, old_state);

      /* remove last success message */
      tmp_elem = lDechainElem(*answer, lLast(*answer)); 
      lFreeElem(tmp_elem);

      /* add error message */
      ERROR((SGE_EVENT, MSG_QUEUE_NOTMODIFIEDSPOOL_S, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_ERROR);

      DEXIT;
      return -1;
   } 

   DEXIT;
   return 0;
}

/****
 **** qmod_queue_unsuspend (static)
 ****/
static int qmod_queue_unsuspend(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   u_long32 state = 0, old_state = 0;

   DENTER(TOP_LAYER, "qmod_queue_unsuspend");

   if (!VALID(QSUSPENDED, lGetUlong(qep, QU_state))) {
      if (force) {
         if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOUNSUSP4SOS_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         } else if (VALID(QCAL_SUSPENDED, lGetUlong(qep, QU_state))) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOUNSUSP4SOC_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);      
         } else if (sge_signal_queue(SGE_SIGCONT, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOFORCEENABLEQ_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         } else {
            WARNING((SGE_EVENT, MSG_QUEUE_FORCEENABLEQ_SSS, user, host, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      } 
      else {
         if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOUNSUSP4SOS_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         } else if (VALID(QCAL_SUSPENDED, lGetUlong(qep, QU_state))) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOUNSUSP4SOC_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         } else {
            WARNING((SGE_EVENT, MSG_QUEUE_ALREADYUNSUSP_SS, user, 
               lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      }
      
      old_state = state = lGetUlong(qep, QU_state);
      CLEARBIT(QSUSPENDED, state);   /* JIC */
      lSetUlong(qep, QU_state, state);

      sge_change_queue_version(qep, 0, 0);
   } 
   else {
      if (force) {
         if (sge_signal_queue(SGE_SIGCONT, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOFORCEENABLEQ_SS, user, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            INFO((SGE_EVENT, MSG_QUEUE_FORCEENABLEQ_SSS, user, host, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         }

         old_state = state = lGetUlong(qep, QU_state);
         CLEARBIT(QSUSPENDED, state);
         lSetUlong(qep, QU_state, state);

         sge_change_queue_version(qep, 0, 0);
      }
      else {
         if ( !VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state)) &&
              !VALID(QCAL_SUSPENDED, lGetUlong(qep, QU_state)) &&
            sge_signal_queue(SGE_SIGCONT, qep, NULL, NULL)) {
            WARNING((SGE_EVENT, MSG_QUEUE_NOUNSUSPQ_SS, user, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            INFO((SGE_EVENT, MSG_QUEUE_UNSUSPENDQ_SSS, user, host, lGetString(qep, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);

            old_state = state = lGetUlong(qep, QU_state);
            CLEARBIT(QSUSPENDED, state);
            lSetUlong(qep, QU_state, state);

            sge_change_queue_version(qep, 0, 0);
         }
      }
   }
   if (cull_write_qconf(1, 0, QUEUE_DIR, lGetString(qep, QU_qname), NULL, qep) == -1) {
      lListElem *tmp_elem;

      /* rollback */
      lSetUlong(qep, QU_state, old_state);

      /* remove last success message */
      tmp_elem = lDechainElem(*answer, lLast(*answer));
      lFreeElem(tmp_elem);

      /* add error message */
      ERROR((SGE_EVENT, MSG_QUEUE_NOTMODIFIEDSPOOL_S, lGetString(qep, QU_qname)));
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_ERROR);

      DEXIT;
      return -1;
   }         
   DEXIT;
   return 0;
}

/****
 **** qmod_queue_clean (static)
 ****
 **** cleans the specified queue (every job will be deleted)
 **** The user will do this via qconf -cq <qname>
 ****/
static int qmod_queue_clean(
lListElem *qep,
u_long32 force,
lList **answer,
char *user,
char *host,
int isoperator,
int isowner 
) {
   lListElem *gdil_ep, *nextjep, *nexttep, *jep;

   DENTER(TOP_LAYER, "qmod_queue_clean");

   DPRINTF(("cleaning queue >%s<\n", lGetString(qep, QU_qname)));
   
   if (sge_manager(user)) {
      WARNING((SGE_EVENT, MSG_QUEUE_NOCLEANQPERMS)); 
      sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      DEXIT;
      return -1;
   }

   /* using sge_commit_job(j, 3) q->job_list
      should get modified so we have to be careful */

   nextjep = lFirst(Master_Job_List);
   while ((jep=nextjep)) {
      lListElem* jatep;
      nextjep = lNext(jep);

      nexttep = lFirst(lGetList(jep, JB_ja_tasks));
      while ((jatep=nexttep)) {
         nexttep = lNext(jatep);

         for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
            if (!strcmp(lGetString(qep, QU_qname), lGetString(gdil_ep, JG_qname))) {
               sge_commit_job(jep, jatep, 3, 1);
               break;
            }
         }
      }
   }
   INFO((SGE_EVENT, MSG_QUEUE_CLEANQ_SSS, user, host, lGetString(qep, QU_qname)));
   sge_add_answer(answer, SGE_EVENT, STATUS_OK, NUM_AN_INFO);

   DEXIT;
   return 0;
}

/****
 **** qmod_job_reschedule (static)
 ****/
static void qmod_job_reschedule(
lListElem *jep,
lListElem *jatep,
lListElem *queueep,
u_long32 force,
lList **answer,
char *user,
char *host 
) {
   DENTER(TOP_LAYER, "qmod_job_reschedule");

   reschedule_job(jep, jatep, queueep, force, answer);

   DEXIT;
}
/****
 **** qmod_job_suspend (static)
 ****/
static void qmod_job_suspend(
lListElem *jep,
lListElem *jatep,
lListElem *queueep,
u_long32 force,
lList **answer,
char *user,
char *host 
) {
   int i;
   u_long32 state = 0;
   u_long32 jataskid = 0;
   u_long32 jobid = 0;

   DENTER(TOP_LAYER, "qmod_job_suspend");

   jobid = lGetUlong(jep, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);

   if (VALID(JSUSPENDED, lGetUlong(jatep, JAT_state))) {
      /* this job is already suspended or lives in a suspended queue */
      if (force && queueep) {
         /* here force means to send the suspend signal again 
            this can only be done if we know the queue this job
            runs in */
         if (sge_signal_queue(SGE_SIGSTOP, queueep, jep, jatep)) {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_NOFORCESUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_NOFORCESUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_FORCESUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_FORCESUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      }
      else {
         if (is_array(jep)) {
            WARNING((SGE_EVENT, MSG_JOB_ALREADYSUSPENDED_SUU, user, u32c(jobid), u32c(jataskid)));
         } else {
            WARNING((SGE_EVENT, MSG_JOB_ALREADYSUSPENDED_SU, user, u32c(jobid)));
         }
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      }

      /* 
      ** may be the queue is suspended, than the job might not be 
      */
      state = lGetUlong(jatep, JAT_state);
      CLEARBIT(JRUNNING, state);
      SETBIT(JSUSPENDED, state);
      lSetUlong(jatep, JAT_state, state);

      sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
      cull_write_job_to_disk(jep);
   }
   else {   /* job wasn't suspended yet */
      if (queueep) {
         if ((i = sge_signal_queue(SGE_SIGSTOP, queueep, jep, jatep))) {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_NOSUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_NOSUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      }
      else
         i = 1;

      if (force) {
         /* set jobs state to suspend in all cases */
         if (!i) {
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_FORCESUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_FORCESUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         }

         state = lGetUlong(jatep, JAT_state);
         CLEARBIT(JRUNNING, state);
         SETBIT(JSUSPENDED, state);
         lSetUlong(jatep, JAT_state, state);
         sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
         cull_write_job_to_disk(jep);
      }
      else {
         if (!i) {
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_SUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_SUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);

            state = lGetUlong(jatep, JAT_state);
            CLEARBIT(JRUNNING, state);
            SETBIT(JSUSPENDED, state);
            lSetUlong(jatep, JAT_state, state);
            sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
            cull_write_job_to_disk(jep);
         }
      }
   }
   DEXIT;
}

/****
 **** qmod_job_unsuspend (static)
 ****/
static void qmod_job_unsuspend(
lListElem *jep,
lListElem *jatep,
lListElem *queueep,
u_long32 force,
lList **answer,
char *user,
char *host 
) {
   int i;
   u_long32 state = 0;
   u_long32 jobid, jataskid;

   DENTER(TOP_LAYER, "qmod_job_unsuspend");

   jobid = lGetUlong(jep, JB_job_number);
   jataskid = lGetUlong(jatep, JAT_task_number);

   /* admin suspend may not override suspend from threshold */ 
   if (VALID(JSUSPENDED_ON_THRESHOLD, lGetUlong(jatep, JAT_state))) {
      if (VALID(JSUSPENDED, lGetUlong(jatep, JAT_state))) {
         if (is_array(jep)) {
            INFO((SGE_EVENT, MSG_JOB_RMADMSUSPENDTASK_SSUU, user, host, u32c(jobid), u32c(jataskid)));
         } else {
            INFO((SGE_EVENT, MSG_JOB_RMADMSUSPENDJOB_SSU, user, host, u32c(jobid)));
         }
         sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);

         state = lGetUlong(jatep, JAT_state);
         CLEARBIT(JSUSPENDED, state);
         lSetUlong(jatep, JAT_state, state);
         sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
         cull_write_job_to_disk(jep);
         DEXIT;
         return;
      } 
      else {
         /* guess admin tries to remove threshold suspension by qmon -us <jobid> */
         if (is_array(jep)) {
            WARNING((SGE_EVENT, MSG_JOB_NOADMSUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
         } else {
            WARNING((SGE_EVENT, MSG_JOB_NOADMSUSPENDJOB_SU, user, u32c(jobid)));
         }
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         DEXIT;
         return;
      }
   }

   if (VALID(JRUNNING, lGetUlong(jatep, JAT_state))) {
      /* this job is already running */
      if (force && queueep) {
         /* 
         ** here force means to send the cont signal again 
         ** this can only be done if we know the queue this job
         ** runs in 
         */
         if (sge_signal_queue(SGE_SIGCONT, queueep, jep, jatep)) {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_NOFORCEENABLETASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_NOFORCEENABLEJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
         else {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_FORCEENABLETASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_FORCEENABLEJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      }
      else {
         if (is_array(jep)) {
            WARNING((SGE_EVENT, MSG_JOB_ALREADYUNSUSPENDED_SUU, user, u32c(jobid), u32c(jataskid)));
         } else {
            WARNING((SGE_EVENT, MSG_JOB_ALREADYUNSUSPENDED_SU, user, u32c(jobid))); 
         }
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      }
      /* 
      ** job is already running, so no job information has to be changed 
      */
   }
   else {   /* job wasn't suspended till now */
      if (queueep) {
         if ((i = sge_signal_queue(SGE_SIGCONT, queueep, jep, jatep))) {
            if (is_array(jep)) {
               WARNING((SGE_EVENT, MSG_JOB_NOUNSUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               WARNING((SGE_EVENT, MSG_JOB_NOUNSUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         }
      }
      else
         i = 1;

      if (force) {
         /* set jobs state to suspend in all cases */
         if (!i) {
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_FORCEUNSUSPTASK_SSUU, user, host, u32c(jobid), u32c(jataskid)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_FORCEUNSUSPJOB_SSU, user, host, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
         }

         state = lGetUlong(jatep, JAT_state);
         SETBIT(JRUNNING, state);
         CLEARBIT(JSUSPENDED, state);
         lSetUlong(jatep, JAT_state, state);
         sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
         cull_write_job_to_disk(jep);
      }
      else {
         /* set job state only if communication works */
         if (!i) {
            if (is_array(jep)) {
               INFO((SGE_EVENT, MSG_JOB_UNSUSPENDTASK_SUU, user, u32c(jobid), u32c(jataskid)));
            } else {
               INFO((SGE_EVENT, MSG_JOB_UNSUSPENDJOB_SU, user, u32c(jobid)));
            }
            sge_add_answer(answer, SGE_EVENT, STATUS_OK, 0);
            
            state = lGetUlong(jatep, JAT_state);
            SETBIT(JRUNNING, state);
            CLEARBIT(JSUSPENDED, state);
            lSetUlong(jatep, JAT_state, state);
            sge_add_jatask_event(sgeE_JATASK_MOD, jep, jatep);
            cull_write_job_to_disk(jep);
         }
      }
   }
   DEXIT;
}


void rebuild_signal_events()
{
   lListElem *qep, *jep, *jatep;
   u_long32 next_delivery_time;

   DENTER(TOP_LAYER, "rebuild_signal_events");

   /* J O B */
   for_each(jep, Master_Job_List) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) { 
         if (lGetUlong(jatep, JAT_pending_signal) && 
               (next_delivery_time=lGetUlong(jatep, JAT_pending_signal_delivery_time))) {
            te_add(TYPE_SIGNAL_RESEND_EVENT, next_delivery_time, 
                  lGetUlong(jep, JB_job_number), lGetUlong(jatep, JAT_task_number), NULL);
         }
      }
   }

   /* Q U E U E */
   for_each(qep, Master_Queue_List) { 
      if (lGetUlong(qep, QU_pending_signal) && 
          ((next_delivery_time=lGetUlong(qep, QU_pending_signal_delivery_time)))) {
            te_add(TYPE_SIGNAL_RESEND_EVENT, next_delivery_time, 
                  0, 0, lGetString(qep, QU_qname));
      }
   }

   DEXIT;
}

/* this function is called by our timer mechanism for resending signals */  
void resend_signal_event(
u_long32 type,
u_long32 when,
u_long32 jobid,
u_long32 jataskid,
char *queue 
) {
   lListElem *qep, *jep, *jatep;

   DENTER(TOP_LAYER, "resend_signal_event");

   if (!queue) {
      if (!(jep = sge_locate_job(jobid)) || !(jatep=search_task(jataskid, jep))) {
         ERROR((SGE_EVENT, MSG_EVE_RESENTSIGNALTASK_UU, u32c(jobid), u32c(jataskid)));
         DEXIT;
         return;
      }
      if ((qep = lGetElemStr(Master_Queue_List, QU_qname, lGetString(jatep, JAT_master_queue))))
         sge_signal_queue(lGetUlong(jatep, JAT_pending_signal), qep, jep, jatep);
   } else {
      if (!(qep = sge_locate_queue(queue))) {
         ERROR((SGE_EVENT, MSG_EVE_RESENTSIGNALQ_S, queue));
         DEXIT;
         return;
      }
      sge_signal_queue(lGetUlong(qep, QU_pending_signal), qep, NULL, NULL);
   }

   DEXIT;
   return;
}

/************************************************************************
 This is called by the qmaster to:
 - send a signal to all jobs in a queue (job_number == 0);
 - send a signal to one job
 ************************************************************************/
int sge_signal_queue(
int how, /* signal */
lListElem *qep,
lListElem *jep,
lListElem *jatep 
) {
   int i;
   u_long32 next_delivery_time = 60;
   u_long32 now;
   u_long32 dummy;
   sge_pack_buffer pb;
   int sent = 0;

   DENTER(TOP_LAYER, "sge_signal_queue");

   now = sge_get_gmt();

   /* don't try to signal unheard queues */
   if ((lGetUlong(qep, QU_state) & QUNKNOWN)==0) {

      char *hnm, *pnm;

      if (lGetUlong(qep, QU_qtype) & TQ) {
         lListElem *hep;
         pnm = prognames[QSTD];
         if (!(hep=sge_locate_host(lGetString(qep, QU_qhostname), SGE_EXECHOST_LIST))
            || !(hnm=lGetString(hep, EH_real_name))) {
            ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDHOST_S, lGetString(hep, EH_name)));
            DEXIT;
            return -1;
         }
      } else {
         pnm = prognames[EXECD]; 
         hnm = lGetString(qep, QU_qhostname);
      }

      if((i = init_packbuffer(&pb, 256, 0)) == PACK_SUCCESS) {
         /* identifier for acknowledgement */
         if (jep) {
            packint(&pb, lGetUlong(jep, JB_job_number));    /* one for acknowledgement */
            packint(&pb, lGetUlong(jatep, JAT_task_number)); 
            packint(&pb, lGetUlong(jep, JB_job_number));    /* and one for processing */
            packint(&pb, lGetUlong(jatep, JAT_task_number));
         }
         else {
            packint(&pb, lGetUlong(qep, QU_queue_number));
            packint(&pb, 0); 
            packint(&pb, 0); 
            packint(&pb, 0);
         }
         packstr(&pb, lGetString(qep, QU_qname));
         packint(&pb, how); 


         i = send_message_pb(0, pnm, 0, hnm, jep ? TAG_SIGJOB: TAG_SIGQUEUE, 
                          &pb, &dummy);
         clear_packbuffer(&pb);
      } else {
         i = CL_MALLOC;
      }

      if (i) {
         ERROR((SGE_EVENT, MSG_COM_NOUPDATEQSTATE_IS, how, lGetString(qep, QU_qname)));
         DEXIT;
         return i;
      }
      sent = 1;
   }

   next_delivery_time += now;

   /* If this is a operation on one job we enter the signal request in the
      job structure. If the operation is not acknowledged in time we can do
      further steps */
   if (jep) {
      DPRINTF(("JOB "u32": %s signal %s (retry after "u32" seconds) host: %s\n", 
            lGetUlong(jep, JB_job_number), sent?"sent":"queued", sge_sig2str(how), next_delivery_time, 
            lGetString(qep, QU_qhostname)));
      te_delete(TYPE_SIGNAL_RESEND_EVENT, NULL, lGetUlong(jep, JB_job_number), 0);
      lSetUlong(jatep, JAT_pending_signal, how);
      te_add(TYPE_SIGNAL_RESEND_EVENT, next_delivery_time, lGetUlong(jep, JB_job_number),
            lGetUlong(jatep, JAT_task_number), NULL);
      lSetUlong(jatep, JAT_pending_signal_delivery_time, next_delivery_time); 
   }
   else {
      DPRINTF(("QUEUE %s: %s signal %s (retry after "u32" seconds) host %s\n", 
            lGetString(qep, QU_qname), sent?"sent":"queued", sge_sig2str(how), next_delivery_time,
            lGetString(qep, QU_qhostname)));
      te_delete(TYPE_SIGNAL_RESEND_EVENT, lGetString(qep, QU_qname), 0, 0);
      lSetUlong(qep, QU_pending_signal, how);
      te_add(TYPE_SIGNAL_RESEND_EVENT, next_delivery_time, 0, 0, lGetString(qep, QU_qname));
      lSetUlong(qep, QU_pending_signal_delivery_time, next_delivery_time);
   }

   if (!jep) /* signalling a queue ? - handle slave jobs in this queue */
      signal_slave_jobs_in_queue(how, qep); 
   else /* is this the master queue of this job to signal ? - then decide whether slave tasks also 
           must get signalled */
      if (!strcmp(lGetString(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), 
            JG_qname), lGetString(qep, QU_qname)))
         signal_slave_tasks_of_job(how, jep, jatep); 

   DEXIT;
   return 0;
}

/* in case we have to signal a queue 
   in which slave tasks are running 
   we have to notify the master execd 
   where the master task of this job is running
*/  
static void signal_slave_jobs_in_queue(
int how, /* signal */
lListElem *qep 
) {
   lList *gdil_lp;
   lListElem *mq, *pe, *jep, *gdil_ep, *jatep;
   char *qname, *mqname, *pe_name;

   DENTER(TOP_LAYER, "signal_slave_jobs_in_queue");

   /* test whether there are parallel jobs 
      with a slave slot in this queue 
      if so then signal this job */
   for_each (jep, Master_Job_List) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {

         /* skip sequential and not running jobs */
         if (lGetNumberOfElem( gdil_lp =
               lGetList(jatep, JAT_granted_destin_identifier_list))<=1)
            continue;
       
         /* signalling of not "slave controlled" parallel jobs will not work
            since they are not known to the apropriate execd - we should
            omit signalling in this case to prevent waste of communication bandwith */ 
         if (!(pe_name=lGetString(jatep, JAT_granted_pe)) ||
             !(pe=sge_locate_pe(pe_name)) /* ||
             ** signal also jobs, that are not slave controlled
             ** master task must be signaled in every case (JG)
             !lGetUlong(pe, PE_control_slaves) */)
            continue;

         qname = lGetString(qep, QU_qname);
         for (gdil_ep=lNext(lFirst(gdil_lp)); gdil_ep; gdil_ep=lNext(gdil_ep))
            if (!strcmp(lGetString(gdil_ep, JG_qname), qname)) {

               /* search master queue - needed for signalling of a job */
               if ((mq = sge_locate_queue(mqname = lGetString(
                     lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), JG_qname)))) {
                  DPRINTF(("found slave job "u32" in queue %s master queue is %s\n", 
                     lGetUlong(jep, JB_job_number), qname, mqname));
                  sge_signal_queue(how, mq, jep, jatep);
               } else 
                  ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDMQ_SU, mqname, u32c(lGetUlong(jep, JB_job_number))));
               break;
            }
      }
   }

   DEXIT;
   return;
}

static void signal_slave_tasks_of_job(
int how, /* signal */
lListElem *jep,
lListElem *jatep 
) {
   lList *gdil_lp;
   lListElem *mq, *pe, *gdil_ep;
   char *qname, *pe_name;

   DENTER(TOP_LAYER, "signal_slave_tasks_of_job");

   /* do not signal slave tasks in case of checkpointing jobs with 
      STOP/CONT when suspending means migration */
   if ((how==SGE_SIGCONT || how==SGE_SIGSTOP) &&
      (lGetUlong(jep, JB_checkpoint_attr)|CHECKPOINT_SUSPEND)!=0) {
      DPRINTF(("omit signaling - checkpoint script does action for whole job\n"));
      return;
   }

   /* forward signal to slave exec hosts 
      in case of slave controlled jobs */
   if ( !((lGetNumberOfElem(gdil_lp=lGetList(jatep, JAT_granted_destin_identifier_list)))<=1 || 
         !(pe_name=lGetString(jatep, JAT_granted_pe)) ||
         !(pe=sge_locate_pe(pe_name)) ||
         !lGetUlong(pe, PE_control_slaves)))
      for (gdil_ep=lNext(lFirst(gdil_lp)); gdil_ep; gdil_ep=lNext(gdil_ep))
         if ((mq = sge_locate_queue(qname = lGetString(gdil_ep, JG_qname)))) {
            DPRINTF(("found slave job "u32" in queue %s\n", 
               lGetUlong(jep, JB_job_number), qname));
            sge_signal_queue(how, mq, jep, jatep);
         }

   DEXIT;
   return;
}

char *cal_state2str(u_long32 state);
char *cal_state2str(
u_long32 state 
) {
   if ((state & QCAL_SUSPENDED))
      return "SUSPENDED";
   if ((state & QCAL_DISABLED))
      return "DISABLED";
   return "ENABLED";
}

void signal_on_calendar(
lListElem *qep,
u_long32 old_state,
u_long32 new_state 
) {
   char *qname = lGetString(qep, QU_qname);

   DENTER(TOP_LAYER, "signal_on_calendar");

   if (old_state != new_state) {

      DPRINTF(("%s: %s -> %s\n", qname, cal_state2str(old_state), 
         cal_state2str(new_state)));

      if (old_state == QCAL_DISABLED) {
         INFO((SGE_EVENT, MSG_QUEUE_ENABLEQCAL_S, qname)); 
      }

      if (old_state == QCAL_SUSPENDED) {
         if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            INFO((SGE_EVENT, MSG_QUEUE_SOSNOUNSUSPCAL_S, qname));
         } else if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            INFO((SGE_EVENT, MSG_QUEUE_AMDSNOUNSUSPCAL_S, qname));
         } else {
            INFO((SGE_EVENT, MSG_QUEUE_UNSUSPENDQCAL_S, qname)); 
            sge_signal_queue(SGE_SIGCONT, qep, NULL, NULL);
         }
      }

      if (new_state == QCAL_SUSPENDED) {
         if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            INFO((SGE_EVENT, MSG_QUEUE_NOSUSP4SOS_S, qname));
         } else if (VALID(QSUSPENDED_ON_SUBORDINATE, lGetUlong(qep, QU_state))) {
            INFO((SGE_EVENT, MSG_QUEUE_NOSUSP4ADMS_S, qname));
         } else {
            INFO((SGE_EVENT, MSG_QUEUE_SUSPENDQCAL_S, qname)); 
            sge_signal_queue(SGE_SIGSTOP, qep, NULL, NULL);
         }
      }

      if (new_state == QCAL_DISABLED) {
         INFO((SGE_EVENT, MSG_QUEUE_DISABLEQCAL_S, qname));
      }

      lSetUlong(qep, QU_state, 
         (lGetUlong(qep, QU_state) & ~(QCAL_SUSPENDED|QCAL_DISABLED))|new_state);
      sge_add_queue_event(sgeE_QUEUE_MOD, qep);
   }

   DEXIT;
   return;
}



int queue_initial_state(
lListElem *qep,
char *rhost  /* rhost != NULL -> mod */
) {
   char *is;
   int enable, changed = 0;
   u_long32 state;

   DENTER(TOP_LAYER, "queue_initial_state");

   if ((is = lGetString(qep, QU_initial_state)) && strcmp(is, "default")) {
      enable = !strcmp(is, "enabled");
      state = lGetUlong(qep, QU_state);

      if ((enable && (state&QDISABLED)) ||
          (!enable && !(state&QDISABLED))) {

         if (!rhost) {
            if (enable)
               WARNING((SGE_EVENT, MSG_QUEUE_ADDENABLED_S, lGetString(qep, QU_qname)));
            else
               WARNING((SGE_EVENT, MSG_QUEUE_ADDDISABLED_S, lGetString(qep, QU_qname)));
         } else {
            if (enable)
               WARNING((SGE_EVENT, MSG_QUEUE_EXECDRESTARTENABLEQ_SS, rhost, lGetString(qep, QU_qname)));
            else         
               WARNING((SGE_EVENT, MSG_QUEUE_EXECDRESTARTDISABLEQ_SS, rhost, lGetString(qep, QU_qname)));
         }

         if (enable) {
            CLEARBIT(QDISABLED, state);
         } else {
            SETBIT(QDISABLED, state);
         }
         lSetUlong(qep, QU_state, state);
         changed = 1;
      }
   }

   DEXIT;
   return changed;
}
