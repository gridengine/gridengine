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

#include "basis_types.h"
#include "sgermon.h"
#include "sge.h"
#include "sge_all_listsL.h"
#include "job_exit.h"
#include "sge_m_event.h"
#include "sge_host.h"
#include "sge_log.h"
#include "sge_ckpt_qmaster.h"
#include "sge_queue_qmaster.h"
#include "sge_host_qmaster.h"
#include "sge_parse_num_par.h"
#include "execution_states.h"
#include "job.h"
#include "mail.h"
#include "time_event.h"
#include "symbols.h"
#include "jb_now.h"
#include "sge_time.h"
#include "def.h"
#include "read_write_host.h"
#include "reschedule.h"
#include "msg_qmaster.h"
#include "sge_conf.h"
#include "sge_string.h"

extern lList *Master_Queue_List;
extern lList *Master_Job_List;
extern lList *Master_Exechost_List;

u_long32 add_time = 0;

void reschedule_unknown_event(
u_long32 type,
u_long32 when,
u_long32 timeout,
u_long32 not_used2,
char *hostname 
) {
   lListElem *qep;            /* QU_Type */
   lList *answer_list = NULL; /* AN_Type */
   lListElem *hep;            /* EH_Type */
   u_long32 new_timeout = 0;

   DENTER(TOP_LAYER, "reschedule_unknown_event");
 
   /*
   ** delete the timer entry which triggers the execution of this function
   */
   te_delete(type, hostname, timeout, not_used2);

   /*
   ** is the automatic rescheduling disabled
   */
   if (disable_reschedule) {
      DEXIT;
      goto Error;
   }         
 
   /*
   ** locate the host object which went in unknown-state
   */
   if (!(hep = lGetElemHost(Master_Exechost_List, EH_name, hostname))) {
      DEXIT;
      goto Error;
   }
 
   /*
   ** Did someone change the timeout value?
   */
   new_timeout = reschedule_unknown_timeout(hep);
   if (new_timeout == 0) {
      INFO((SGE_EVENT, MSG_RU_CANCELED_S, hostname));
      DEXIT;
      goto Error;
   } else if (new_timeout+add_time > timeout) {
      u_long32 now;
 
      now = sge_get_gmt();
      te_add(TYPE_RESCHEDULE_UNKNOWN_EVENT, now + 
         (new_timeout + add_time - timeout), new_timeout + add_time, 
         0, hostname);
      INFO((SGE_EVENT, MSG_RU_TRIGGER_SU, hostname,
         u32c(new_timeout + add_time - timeout)));
      DEXIT;
      goto Error;
   }
 
   /*
   ** Check if this host is still in unknown state
   */

   for_each(qep, Master_Queue_List) {
      if (!hostcmp(hostname, lGetString(qep, QU_qhostname))) {
         u_long32 state;

         state = lGetUlong(qep, QU_state);
         if (!VALID(QUNKNOWN, state)) {
            DEXIT;
            goto Error;
         }
      }
   }

   /*
   ** Find all jobs currently running on the host which went into
   ** unknown state and append the jobids/taskids into
   ** a sublist of the exechost object
   */

 
   reschedule_jobs(hep, 0, &answer_list);
   lFreeList(answer_list);
   DEXIT;
   return;

Error:
   DEXIT;
   return;
}
 
/*
** ep         Host element pointer (EH_Type) or Queue element pointer (QU_Type)
**
** return      0 - success
**             1 - invalid parameter
*/
int reschedule_jobs(
lListElem *ep,    /* EH_Type or QU_Type */
u_long32 force,   /* boolean */
lList **answer    /* AN_Type */
) {
   lListElem *hep = NULL;        /* EH_Type */
   lListElem *qep = NULL;        /* QU_Type */
   lListElem *jep;               /* JB_Type */
   char *hostname = NULL;
   int ret = 0;
 
   DENTER(TOP_LAYER, "reschedule_jobs");
 
   /*
   ** if ep is of type EH_Type than we will reschedule all jobs
   ** running on that host. if it is of type QU_Type than we will
   ** only reschedule the jobs for that queue
   */
   if (is_obj_of_type(ep, EH_Type)) {
      hep = ep;
      qep = NULL;
      hostname = lGetString(ep, EH_name);
   } else if (is_obj_of_type(ep, QU_Type)) {
      qep = ep;
      hostname = lGetString(qep, QU_qhostname);
      hep = lGetElemHost(Master_Exechost_List, EH_name, hostname);
   } else {
      ret = 1;
   }
 
   if (!ret) {
      /*
      ** Find all jobs currently running on the host/queue
      ** append the jobids/taskids into a sublist of the exechost object
      */
      for_each(jep, Master_Job_List) {
         reschedule_job(jep, NULL, ep, force, answer);
      }                      
   }
 
   DEXIT;
   return ret;
}
 
/*
** return
**    0  successfull 
*/
int reschedule_job(
lListElem *jep,   /* JB_Type */
lListElem *jatep, /* JAT_Type or NULL */
lListElem *ep,    /* EH_Type or QU_Type or NULL */
u_long32 force,   /* boolean */
lList **answer    /* AN_Type */
) {
   lListElem *qep;               /* QU_Type */
   lListElem *hep;               /* EH_Type */
   lListElem *this_jatep;        /* JAT_Type */
   lListElem *next_jatep;        /* JAT_Type */
   char mail_ids[256];
   char mail_type[256];
   u_long32 job_number;
   u_long32 job_now;
   char *hostname;
   int ret = 0;
   DENTER(TOP_LAYER, "reschedule_job");
 
   job_number = lGetUlong(jep, JB_job_number);
            
   /*
   ** if jatep is NULL then reschedule all tasks of this job
   */
   if (jatep) {
      next_jatep = jatep;
   } else {
      next_jatep = lFirst(lGetList(jep, JB_ja_tasks));
   }

   while ((this_jatep = next_jatep)) {
      lListElem *first_granted_queue;  /* JG_Type */
      lListElem *host;                 /* EH_Type */
      lList *granted_qs;
      u_long32 task_number;
      u_long32 found;

      if (jatep) {
         next_jatep = NULL;
      } else {
         next_jatep = lNext(this_jatep);
      }

      task_number = lGetUlong(this_jatep, JAT_task_number);

      if (is_array(jep)) {
         sprintf(mail_ids, U32CFormat"."U32CFormat,
            u32c(job_number), u32c(task_number));
         sprintf(mail_type, MSG_RU_TYPEJOBARRAY);
      } else {
         sprintf(mail_ids, U32CFormat, u32c(job_number));
         sprintf(mail_type, MSG_RU_TYPEJOB);
      }

      granted_qs = lGetList(this_jatep, JAT_granted_destin_identifier_list);
      first_granted_queue = lFirst(granted_qs);

      /*
      ** if ep is of type EH_Type than we will reschedule all tasks
      ** running on that host. if it is of type QU_Type than we will
      ** only reschedule the tasks for that queue. if it is NULL than we will
      ** reschedule all tasks of that job
      */
      if (ep && is_obj_of_type(ep, EH_Type)) {
         hep = ep;
         qep = NULL;
         hostname = lGetString(ep, EH_name);
      } else if (ep && is_obj_of_type(ep, QU_Type)) {
         qep = ep;
         hostname = lGetString(qep, QU_qhostname);
         hep = lGetElemHost(Master_Exechost_List, EH_name, hostname);
      } else {
         qep = NULL;
         hep = NULL;
         hostname = NULL;
      }                        

      /*
      ** Jobs which have no granted queue can not be rescheduled
      */
      if (!first_granted_queue) {
         /* Skip pendig jobs silently */
         continue;
      }              

      /*
      ** We will skip this job if we only reschedule jobs for a
      ** specific queue/host and the current task is not running
      ** on that queue/host
      **
      ** PE-Jobs will only be rescheduled when the master task
      ** is running in that queue/host
      */
      if (first_granted_queue &&
          ((qep && (strcmp(lGetString(first_granted_queue, JG_qname),
              lGetString(qep, QU_qname))))
          || ((hep && hostcmp(lGetString(first_granted_queue, JG_qhostname),
              lGetString(hep, EH_name)))))) {
         /* Skip jobs silently which are not intended to reschedule */
         continue;
      }

      /*
      ** Is this job tagged as restartable?
      ** If the forced flag is set we will also reschedule this job!
      */
      if (!force && lGetUlong(jep, JB_restart) == 2) {
         INFO((SGE_EVENT, MSG_RU_NOT_RESTARTABLE_SS, 
            mail_type, mail_ids));
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         continue;
      }

      /*
      ** qsh, qlogin, qrsh, qrlogin-jobs won't be rescheduled automatically
      ** (immediate jobs (qsub -now y ...) will be rescheduled)
      */
      job_now = lGetUlong(jep, JB_now);
      if (JB_NOW_IS_QSH(job_now) || JB_NOW_IS_QLOGIN(job_now)
          || JB_NOW_IS_QRSH(job_now) || JB_NOW_IS_QRLOGIN(job_now)) {
         INFO((SGE_EVENT, MSG_RU_INTERACTIVEJOB_SSS, mail_ids, mail_type, 
            mail_type));
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, 
            NUM_AN_WARNING);
         continue;
      }

      /*
      ** ckpt-jobs will only be rescheduled when the "when" attribute
      ** contains an appropriate flag or when the forced flag is set
      */
      if (!force && lGetString(jep, JB_checkpoint_object)) {
         lListElem *ckpt_ep; /* CK_Type */
    
         ckpt_ep = sge_locate_ckpt(lGetString(jep, JB_checkpoint_object));
         if (ckpt_ep) {
            u_long32 flags;
    
            flags = lGetUlong(jep, JB_checkpoint_attr);
            if (!flags) {
               flags = sge_parse_checkpoint_attr(
                          lGetString(ckpt_ep, CK_when));
            }
            if (!(flags & CHECKPOINT_AT_AUTO_RES)) {
               INFO((SGE_EVENT, MSG_RU_CKPTNOTVALID_SSS,
                   mail_ids, lGetString(ckpt_ep, CK_name), mail_type));
               sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, 
                  NUM_AN_WARNING);
               continue;
            }
         } else {
            INFO((SGE_EVENT, MSG_RU_CKPTEXIST_SS, mail_ids, 
               lGetString(ckpt_ep, CK_name)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, 
               NUM_AN_WARNING);
            continue;
         }
      }               

      /*
      ** Jobs which were registered for deletion will
      ** not be automaticly rescheduled (exception: forced flag)
      */
      if (!force && (lGetUlong(this_jatep, JAT_state) & JDELETED)) {
         INFO((SGE_EVENT, MSG_RU_INDELETEDSTATE_SS, mail_type, mail_ids));
         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
         continue;
      }

      /*
      ** If the user did not use the -r flag during submit
      ** we have to check the queue default (exception: forced flag)
      */
      if (!force && lGetUlong(jep, JB_restart) == 0) {
         lListElem *queue; /* QU_Type */

         if (qep && !strcmp(lGetString(first_granted_queue,
            JG_qname), lGetString(qep, QU_qname))) {
            queue = qep;
         } else {
            queue = sge_locate_queue(lGetString(first_granted_queue,
                     JG_qname));
         }
         if (!lGetUlong(queue, QU_rerun)) {
            INFO((SGE_EVENT, MSG_RU_NORERUNQUEUE_SSS, mail_type, mail_ids, 
               lGetString(queue, QU_qname)));
            sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, 
               NUM_AN_WARNING);
            continue;
         }
      }

      /*
      ** Is this task already contained in the list?
      ** Append it if necessary
      */
      if (hep && !hostcmp(lGetString(first_granted_queue, JG_qhostname),
          lGetString(hep, EH_name))) {
         host = hep;
      } else {
         host = sge_locate_host(lGetString(first_granted_queue,
            JG_qhostname), SGE_EXECHOST_LIST);
         hostname = lGetString(first_granted_queue, JG_qhostname);
      }
      if (get_from_reschedule_unknown_list(host, job_number, task_number)) {
         found = 1;
      } else {
         found = 0;
      }

      if (!found) {
         add_to_reschedule_unknown_list(host, job_number, task_number, 0);
         ret = 0;                

#if 1
         DPRINTF(("RU: ADDED JOB "u32"."u32
            " ON HOST "SFN" TO RU_TYPE-LIST\n", job_number,
            task_number, hostname));
      } else {
         DPRINTF(("RU: JOB "u32"."u32" ON HOST "SFN
            " already contained in RU_TYPE-LIST\n", job_number,
            task_number, hostname));
#endif
      }

      /*
      ** Trigger the rescheduling of this task
      */
      if (!found) {
         lListElem *pseudo_jr; /* JR_Type */

         lSetUlong(this_jatep, JAT_job_restarted, 1);

         pseudo_jr = lCreateElem(JR_Type);
         lSetUlong(pseudo_jr, JR_job_number, job_number);
         lSetUlong(pseudo_jr, JR_ja_task_number, task_number);
         lSetUlong(pseudo_jr, JR_failed, SSTATE_AGAIN);
         lSetString(pseudo_jr, JR_err_str, (char *) MSG_RU_JR_ERRSTR);
         lSetString(pseudo_jr, JR_queue_name,
            lGetString(first_granted_queue, JG_qname));
         lSetString(pseudo_jr, JR_host_name,
            lGetString(first_granted_queue, JG_qhostname));
         lSetString(pseudo_jr, JR_owner,
            lGetString(jep, JB_owner));
         sge_job_exit(pseudo_jr, jep, this_jatep);
         lFreeElem(pseudo_jr);
      }                         

DTRACE;

      /*
      ** Mails and messages
      */
      if (!found) {
         u_long32 mail_options;
         char mail_action[256];

         mail_options = lGetUlong(jep, JB_mail_options);
         if (force) {
            sprintf(mail_action, MSG_RU_FORCEDR);
         } else {
            sprintf(mail_action, MSG_RU_PUSHEDR);
         }
         if (VALID(MAIL_AT_ABORT, mail_options)) {
            lList *mail_users;
            char mail_subject[1024];
            char mail_body[1024];

            mail_users = lGetList(jep, JB_mail_list);
            sprintf(mail_subject, MSG_RU_MAILSUB_SS, mail_type, mail_ids);
            sprintf(mail_body, MSG_RU_MAILBODY_SSSS,
               mail_type, mail_ids, hostname, mail_action, mail_type);
            cull_mail(mail_users, mail_subject, mail_body, MSG_RU_MAILTYPE);
         }

DTRACE;

         sprintf(SGE_EVENT, MSG_RU_MSGFILEINFO, mail_action, mail_type,
            mail_ids, hostname);

DTRACE;

         sge_add_answer(answer, SGE_EVENT, STATUS_ESEMANTIC, NUM_AN_WARNING);
      }
   }
   DEXIT;
   return ret;
}                     

lListElem* add_to_reschedule_unknown_list(
lListElem *host,
u_long32 job_number,
u_long32 task_number,
u_long32 state 
) {
   lListElem* ruep = NULL;
   DENTER(TOP_LAYER, "add_to_reschedule_unknown_list");
 
   if (host) {
      ruep = lAddSubUlong(host, RU_job_number, job_number,
         EH_reschedule_unknown_list, RU_Type);
 
      DPRINTF(("RU: ADDED "u32"."u32" to EH_reschedule_unknown_list "
               "of host "SFN"\n", job_number, task_number, 
               lGetString(host, EH_name)));
 
      lSetUlong(ruep, RU_task_number, task_number);
      lSetUlong(ruep, RU_state, state);

      write_host(1, 2, host, EH_name, NULL); 
      sge_add_event(sgeE_EXECHOST_MOD, 0, 0, lGetString(host, EH_name), host);
   }
   DEXIT;
   return ruep;
}
 
lListElem* get_from_reschedule_unknown_list(
lListElem *host,
u_long32 job_number,
u_long32 task_number 
) {
   lList *rulp;
   lListElem *ruep = NULL;
 
   DENTER(TOP_LAYER, "get_from_reschedule_unknown_list");
   rulp = lGetList(host, EH_reschedule_unknown_list);
   if (rulp) {
      for_each(ruep, rulp) {
         if (job_number == lGetUlong(ruep, RU_job_number)
             && task_number == lGetUlong(ruep, RU_task_number)) {
             break;
         }
      }
   }
   DEXIT;
   return ruep;
}               

void delete_from_reschedule_unknown_list(
lListElem *host 
) {
   lList *rulp;
   DENTER(TOP_LAYER, "delete_from_reschedule_unknown_list");
 
   rulp = lGetList(host, EH_reschedule_unknown_list);
   if (rulp) {
      lListElem *this, *next;
 
      next = lFirst(rulp);
      while((this = next)) {
         u_long32 state;
   
         next = lNext(this);
         state = lGetUlong(this, RU_state);
         if (state == RESCHEDULE_SKIP_JR_REMOVE
             || state == RESCHEDULE_HANDLE_JR_REMOVE) {
            DPRINTF(("RU: REMOVED "u32"."u32" FROM RU LIST\n",
               lGetUlong(this, RU_job_number),
               lGetUlong(this, RU_task_number)));
            lRemoveElem(rulp, this);
            write_host(1, 2, host, EH_name, NULL);
            sge_add_event(sgeE_EXECHOST_MOD, 0, 0, lGetString(host, EH_name),
               host); 
         }
      }
   }
   DEXIT;
}
 
void update_reschedule_unknown_list(
lListElem *host 
) {
   lListElem *ruep;
   lList *rulp;
 
   DENTER(TOP_LAYER, "update_reschedule_unknown_list");
   if (host) {
      rulp = lGetList(host, EH_reschedule_unknown_list);
      if (rulp) {
         for_each(ruep, rulp) {
            u_long32 state, new_state;

            state = lGetUlong(ruep, RU_state);
            new_state = state;
            if (state == RESCHEDULE_SKIP_JR_SEND_ACK) {
               new_state = RESCHEDULE_SKIP_JR_REMOVE;
            } else if (state == RESCHEDULE_SKIP_JR) {
               new_state = RESCHEDULE_SKIP_JR_SEND_ACK;
            }
            if (state != new_state) {
               lSetUlong(ruep, RU_state, new_state);
            }
            write_host(1, 2, host, EH_name, NULL);
            sge_add_event(sgeE_EXECHOST_MOD, 0, 0, lGetString(host, EH_name), 
               host);
         }
      }
   }
   DEXIT;
}          

u_long32 skip_restarted_job(
lListElem *host,
lListElem *job_report,
u_long32 job_number,
u_long32 task_number 
) {
   lListElem *ruep;
   lList *rulp;
   u_long32 ret = 0;
   DENTER(TOP_LAYER, "skip_restarted_job");
 
   rulp = lGetList(host, EH_reschedule_unknown_list);
   if (rulp) {
      for_each(ruep, rulp) {
         if (lGetUlong(ruep, RU_job_number) == job_number
             && lGetUlong(ruep, RU_task_number) == task_number) {
            u_long32 state, new_state;

            state = lGetUlong(ruep, RU_state);
            new_state = state;
            if (state == RESCHEDULE_SKIP_JR_REMOVE) {
               new_state = RESCHEDULE_SKIP_JR; 
               ret = 2; 
            } else if (state == RESCHEDULE_SKIP_JR_SEND_ACK
                       || state == RESCHEDULE_SKIP_JR) {
               if (lGetUlong(job_report, JR_state) == JEXITING) {
                  ret = 3;
               } else {
                  ret = 1;
                  DPRINTF(("RU: GOT JR\n"));
               }
            }
            if (new_state != state) {
               lSetUlong(ruep, RU_state, new_state);
            }
         }
      }
   }
 
   DEXIT;
   return ret;
}           

void update_reschedule_unknown_list_for_job(
lListElem *host,
u_long32 job_number,
u_long32 task_number 
) {
   lListElem *ruep;
   lList *rulp;
 
   DENTER(TOP_LAYER, "update_reschedule_unknown_list_for_job");
   if (host) {
      rulp = lGetList(host, EH_reschedule_unknown_list);
      if (rulp) {
         for_each(ruep, rulp) {
            if (lGetUlong(ruep, RU_state) == RESCHEDULE_HANDLE_JR_WAIT
                && lGetUlong(ruep, RU_job_number) == job_number
                && lGetUlong(ruep, RU_task_number) == task_number) {
               lSetUlong(ruep, RU_state, RESCHEDULE_HANDLE_JR_REMOVE);
               DPRINTF(("RU: DECREMENTED PE-TAG OF "u32"."u32"\n",
                  job_number, task_number));  
            }
         }
      }
   }
   DEXIT;
}       

u_long32 reschedule_unknown_timeout(
lListElem *hep 
) {
   extern int new_config;
   static int not_init = 1;
   u_long32 timeout;
   lListElem *cfep;
   char *host;
 
   DENTER(TOP_LAYER, "reschedule_unknown_timeout");
 
   host = lGetString(hep, EH_name);
   timeout = lGetUlong(hep, EH_reschedule_unknown);
   /* cache reschedule_unknown parameter in execd host to
      prevent host name resolving */
   if (new_config || not_init) {
      if (!(cfep = get_local_conf_val(host, "reschedule_unknown"))
          || (cfep && !parse_ulong_val(NULL, &timeout, TYPE_TIM,
              lGetString(cfep, CF_value), NULL, 0))) {
         if (cfep) {
            ERROR((SGE_EVENT, MSG_OBJ_RESCHEDULEUNKN_SS,
               host, lGetString(cfep, CF_value)));
         }
         timeout = 0;
      }
      DPRINTF(("reschedule_unknown timeout for host %s is "u32"\n",
               host, timeout));
      lSetUlong(hep, EH_reschedule_unknown, timeout);
      not_init = 0;
   }
   DEXIT;
   return timeout;
}

void reschedule_unknown_trigger(
lListElem *hep  /* EH_Type */
) {
   u_long32 now;
   u_long32 timeout;

   DENTER(TOP_LAYER, "reschedule_unknown_trigger"); 
   now = sge_get_gmt();
   timeout = reschedule_unknown_timeout(hep);
 
   if (timeout) {
      DPRINTF(("RU: Autorescheduling "
         "enabled for host "SFN". ("u32
         " sec)\n", lGetString(hep, EH_name), timeout + add_time));
      te_add(TYPE_RESCHEDULE_UNKNOWN_EVENT, now+timeout+add_time, timeout, 0,
         lGetString(hep, EH_name));
   }
   DEXIT;
}       

void reschedule_add_additional_time(u_long32 time) {
   DENTER(TOP_LAYER, "reschedule_add_additional_time");
   add_time = time;
   DEXIT;
}  

