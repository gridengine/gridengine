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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fnmatch.h>

#include "sge_unistd.h"
#include "sgermon.h"
#include "def.h"
#include "symbols.h"
#include "sge.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_complex.h"
#include "slots_used.h"
#include "sge_resource.h"
#include "sge_jobL.h"
#include "sge_complexL.h"
#include "sge_sched.h"
#include "cull_sort.h"
#include "usage.h"
#include "sge_feature.h"
#include "parse.h"
#include "sge_prog.h"
#include "sge_parse_num_par.h"
#include "sge_string.h"
#include "show_job.h"
#include "sge_dstring.h"
#include "qstat_printing.h"
#include "sge_range.h"
#include "sig_handlers.h"
#include "msg_clients_common.h"
#include "sge_job_jatask.h"
#include "get_path.h"
#include "sge_job_queue.h"
#include "sge_job_jatask.h"
#include "sge_var.h"
#include "sge_answer.h"

static int sge_print_job(lListElem *job, lListElem *jatep, lListElem *qep, int print_jobid, char *master, dstring *task_str, u_long32 full_listing, int slots, int slot, lList *ehl, lList *cl, lList *pe_list, char *intend);

static int sge_print_subtask(lListElem *job, lListElem *ja_task, lListElem *task, int print_hdr, int indent);

static void qtype(char *type_string, u_long32 type);

static int sge_print_jobs_not_enrolled(lListElem *job, lListElem *qep,
                           int print_jobid, char *master, u_long32 full_listing,
                           int slots, int slot, lList *exechost_list,
                           lList *complex_list, lList *pe_list, char *indent,
                           u_long32 sge_ext, u_long32 group_opt);

static void sge_printf_header(u_long32 full_listing, u_long32 sge_ext);

static char *queue_types[] = {
   "BATCH",        
   "INTERACTIVE",  
   "CHECKPOINTING",
   "PARALLEL",
   "TRANSFER",
   ""
};

static char hashes[] = "##############################################################################################################";

/* size of buffer for reason of alarm state */
#ifndef REASON_BUF_SIZE
#define REASON_BUF_SIZE 1023
#endif

int sge_print_queue(
lListElem *q,
lList *exechost_list,
lList *complex_list,
u_long32 full_listing,
lList *qresource_list 
) {
   char type_string[8];
   char state_string[8];
   char to_print[80];
   char arch_string[80];
   u_long32 state;
   double load_avg;
   static int first_time = 1;
   int sge_ext;
   char *load_avg_str;
   char reason[REASON_BUF_SIZE + 1];

   DENTER(TOP_LAYER, "sge_print_queue");

   *reason = 0;

   /* make it possible to display any load value in qstat output */
   if (!(load_avg_str=getenv("SGE_LOAD_AVG")) || !strlen(load_avg_str))
      load_avg_str = LOAD_ATTR_LOAD_AVG;

   if (!(full_listing & QSTAT_DISPLAY_FULL)) {
      DEXIT;
      return 1;
   }

   if (first_time) {
      first_time = 0;
      printf("%-20.20s %-5.5s %-9.9s %-8.8s %-9.9s %s\n", 
            MSG_QSTAT_PRT_QUEUENAME,
            MSG_QSTAT_PRT_QTYPE, 
            MSG_QSTAT_PRT_USEDTOT,
            load_avg_str,
            LOAD_ATTR_ARCH,
            MSG_QSTAT_PRT_STATES);
   }

   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
             (full_listing & QSTAT_DISPLAY_EXTENDED);

   printf("----------------------------------------------------------------------------%s\n", 
      sge_ext?"------------------------------------------------------------------------------------------------------------":"");
   printf("%-20.20s ", lGetString(q, QU_qname));

   qtype(type_string, lGetUlong(q, QU_qtype));
   printf("%-5.5s ", type_string); 

   /* number of used/free slots */
   sprintf(to_print, "%d/%d ", 
      qslots_used(q),
      (int)lGetUlong(q, QU_job_slots));
   printf("%-9.9s ", to_print);   

   /* load avg */
   if (!sge_get_double_qattr(&load_avg, load_avg_str, q, exechost_list, complex_list)) {
      sprintf(to_print, "%2.2f ", load_avg);
   }
   else
      sprintf(to_print, "-NA- ");
   printf("%-8.8s ", to_print);   

   /* arch */
   if (!sge_get_string_qattr(arch_string, sizeof(arch_string)-1, LOAD_ATTR_ARCH, 
         q, exechost_list, complex_list))
      sprintf(to_print, "%s ", arch_string);
   else
      sprintf(to_print, "-NA- ");
   printf("%-9.9s ", to_print);   

   state = lGetUlong(q, QU_state);
   if (sge_load_alarm(q, lGetList(q, QU_load_thresholds), exechost_list, complex_list, NULL)) {
      state |= QALARM;
      sge_load_alarm_reason(q, lGetList(q, QU_load_thresholds), exechost_list, complex_list, reason, REASON_BUF_SIZE, "load");
   }
   if (sge_load_alarm(q, lGetList(q, QU_suspend_thresholds), exechost_list, complex_list, NULL)) {
     state |= QSUSPEND_ALARM;
     sge_load_alarm_reason(q, lGetList(q, QU_suspend_thresholds), exechost_list, complex_list, reason, REASON_BUF_SIZE, "suspend");
   }

   queue_get_state_string(state_string, state);
   printf("%s", state_string); 
   printf("\n");

   if((full_listing & QSTAT_DISPLAY_ALARMREASON)) {
      if(*reason) {
	printf(reason);
      }
   }

   /* view (selected) resources of queue in case of -F [attr,attr,..] */ 
   if ((full_listing & QSTAT_DISPLAY_QRESOURCES)) {
      lList *rlp;
      lListElem *rep;
      char dom[5];
      const char *s;
      u_long32 dominant;

      rlp = NULL;
      queue_complexes2scheduler(&rlp, q, exechost_list, complex_list, 0);
      for_each (rep , rlp) {

         if (qresource_list) {
            lListElem *r1, *r2 = NULL;
            for_each (r1, qresource_list) {
               if ((r2=lGetSubStr(r1, CE_name, lGetString(rep, CE_name), RE_entries)) ||
                  (r2=lGetSubStr(r1, CE_name, lGetString(rep, CE_shortcut), RE_entries)))
                  break;
            }
            if (!r2)
               continue;
         }

         { 
            u_long32 type = lGetUlong(rep, CE_valtype);
            switch (type) {
            case TYPE_HOST:   
            case TYPE_STR:   
            case TYPE_CSTR:   
               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  dominant = lGetUlong(rep, CE_pj_dominant);
                  s = lGetString(rep, CE_pj_stringval);
               } else {
                  dominant = lGetUlong(rep, CE_dominant);
                  s = lGetString(rep, CE_stringval);
               }
               break;
            default:   
               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  dominant = lGetUlong(rep, CE_pj_dominant);
                  s = resource_descr(lGetDouble(rep, CE_pj_doubleval), lGetUlong(rep, CE_valtype), NULL);
               } else {
                  dominant = lGetUlong(rep, CE_dominant);
                  s = resource_descr(lGetDouble(rep, CE_doubleval), lGetUlong(rep, CE_valtype), NULL);
               }
               break;
            }
         }
         monitor_dominance(dom, dominant); 
         switch(lGetUlong(rep, CE_valtype)) {
         case TYPE_INT:  
         case TYPE_TIM:  
         case TYPE_MEM:  
         case TYPE_BOO:  
         case TYPE_DOUBLE:  
         default:
            printf("\t%s:%s=%s\n", dom, lGetString(rep, CE_name), s);
            break;
         }
      }
      lFreeList(rlp);
   }

   DEXIT;
   return 1;
}


static int sge_print_subtask(
lListElem *job,
lListElem *ja_task,
lListElem *pe_task,  /* NULL, if master task shall be printed */
int print_hdr,
int indent 
) {
   char task_state_string[8];
   u_long32 tstate, tstatus;
   int task_running;
   const char *str;
   lListElem *ep;
   int sge_mode = feature_is_enabled(FEATURE_SGEEE);
   lList *usage_list;
   lList *scaled_usage_list;

   DENTER(TOP_LAYER, "sge_print_subtask");

   /* is sub-task logically running */
   if(pe_task == NULL) {
      tstatus = lGetUlong(ja_task, JAT_status);
      usage_list = lGetList(ja_task, JAT_usage_list);
      scaled_usage_list = lGetList(ja_task, JAT_scaled_usage_list);
   } else {
      tstatus = lGetUlong(pe_task, PET_status);
      usage_list = lGetList(pe_task, PET_usage);
      scaled_usage_list = lGetList(pe_task, PET_scaled_usage);
   }

   task_running = (tstatus==JRUNNING || tstatus==JTRANSFERING);

   if (print_hdr) {
      printf(QSTAT_INDENT "Sub-tasks:           %-12.12s %5.5s %s %-4.4s %-6.6s\n", 
             "task-ID",
             "state",
             sge_mode ? USAGE_ATTR_CPU "        " USAGE_ATTR_MEM "     " USAGE_ATTR_IO "     "
                 : "",
             "stat",
             "failed");
   }

   if(pe_task == NULL) {
      str = "";
   } else {
      str = lGetString(pe_task, PET_id);
   }
   printf("   %s%-12s ", indent?QSTAT_INDENT2:"", str);

   /* move status info into state info */
   tstate = lGetUlong(ja_task, JAT_state);
   if (tstatus==JRUNNING) {
      tstate |= JRUNNING;
      tstate &= ~JTRANSFERING;
   } else if (tstatus==JTRANSFERING) {
      tstate |= JTRANSFERING;
      tstate &= ~JRUNNING;
   } else if (tstatus==JFINISHED) {
      tstate |= JEXITING;
      tstate &= ~(JRUNNING|JTRANSFERING);
   }

   if (lGetList(job, JB_jid_predecessor_list) || lGetUlong(ja_task, JAT_hold)) {
      tstate |= JHELD;
   }

   if (lGetUlong(ja_task, JAT_job_restarted)) {
      tstate &= ~JWAITING;
      tstate |= JMIGRATING;
   }

   /* write states into string */ 
   job_get_state_string(task_state_string, tstate);
   printf("%-5.5s ", task_state_string); 

   if (sge_mode) {
      lListElem *up;

      /* scaled cpu usage */
      if (!(up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_CPU))) 
         printf("%-10.10s ", task_running?"NA":""); 
      else 
         printf("%s ", resource_descr(lGetDouble(up, UA_value), TYPE_TIM, NULL));

      /* scaled mem usage */
      if (!(up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_MEM))) 
         printf("%-7.7s ", task_running?"NA":""); 
      else
         printf("%-5.5f ", lGetDouble(up, UA_value)); 
  
      /* scaled io usage */
      if (!(up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_IO))) 
         printf("%-7.7s ", task_running?"NA":""); 
      else
         printf("%-5.5f ", lGetDouble(up, UA_value)); 
   }

   if (tstatus==JFINISHED) {
      ep=lGetElemStr(usage_list, UA_name, "exit_status");
      if(pe_task == NULL) {
         str = "";
      } else {
         str = var_list_get_string(lGetList(pe_task, PET_environment), VAR_PREFIX "O_MAIL");
      }

      printf("%-4d %s", ep ? (int)lGetDouble(ep, UA_value) : 0, str);
   }

   putchar('\n');

   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*/

#if 0
{
   lCondition *where = lWhere("%T(%I->%T(%I==%s))", lGetListDescr(job_list),
                        JB_granted_destin_identifier_list, 
                        JG_Type, JG_qname, lGetString(qep, QU_qname));
   lEnumeration *what = lWhat("%T(ALL)", lGetListDescr(job_list)); 
lWriteListTo(lSelect("trala", job_list, where, what), stdout);
   where = lFreeWhere(where);
   what = lFreeWhat(what);
}
#endif

/*-------------------------------------------------------------------------*/
/* print jobs per queue                                                    */
/*-------------------------------------------------------------------------*/
void sge_print_jobs_queue(
lListElem *qep,
lList *job_list,
lList *pe_list,
lList *user_list,
lList *ehl,
lList *cl,
int print_jobs_of_queue,
u_long32 full_listing,
char *indent 
) {
   int first = 1;
   lListElem *jlep;
   lListElem *jatep;
   lListElem *gdilep;
   u_long32 qstate;
   u_long32 job_tag;
   u_long32 jid = 0, old_jid;
   u_long32 jataskid = 0, old_jataskid;
   const char *qnm;
   dstring dyn_task_str = DSTRING_INIT;

   DENTER(TOP_LAYER, "sge_print_jobs_queue");

   qnm = lGetString(qep, QU_qname);
   qstate = lGetUlong(qep, QU_state);

   for_each(jlep, job_list) {
      int master, i;
      for_each(jatep, lGetList(jlep, JB_ja_tasks)) {
         if (shut_me_down) {
            SGE_EXIT(1);
         }   
         for_each (gdilep, lGetList(jatep, JAT_granted_destin_identifier_list)) {

            if(!strcmp(lGetString(gdilep, JG_qname) , lGetString(qep, QU_qname))) {
               int slot_adjust = 0;

               if (qstate&(QSUSPENDED|QSUSPENDED_ON_SUBORDINATE|QCAL_SUSPENDED)) {
                  u_long32 jstate;

                  jstate = lGetUlong(jatep, JAT_state);
                  jstate &= ~JRUNNING;                 /* unset bit JRUNNING */
                  jstate |= JSUSPENDED_ON_SUBORDINATE; /* set bit JSUSPENDED_ON_SUBORDINATE */
                  lSetUlong(jatep, JAT_state, jstate);
               }
               job_tag = lGetUlong(jatep, JAT_suitable);
               job_tag |= TAG_FOUND_IT;
               lSetUlong(jatep, JAT_suitable, job_tag);

               master = !strcmp(qnm, lGetString(lFirst(lGetList(jatep, 
                     JAT_granted_destin_identifier_list)), JG_qname));

               if (master) {
                  const char *pe_name;
                  lListElem *pe;
                  if (((pe_name=lGetString(jatep, JAT_granted_pe))) &&
                      ((pe=lGetElemStr(pe_list, PE_name, pe_name))) &&
                      !lGetUlong(pe, PE_job_is_first_task))

                      slot_adjust = 1;
               }

               for (i=0;i<(int)lGetUlong(gdilep, JG_slots)+slot_adjust;i++) {
                  int already_printed = 0;

                  if (!lGetNumberOfElem(user_list) || 
                     (lGetNumberOfElem(user_list) && (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) {
                     if (print_jobs_of_queue && (job_tag & TAG_SHOW_IT)) {
                        int different;

                        old_jid = jid;
                        jid = lGetUlong(jlep, JB_job_number);
                        old_jataskid = jataskid;
                        jataskid = lGetUlong(jatep, JAT_task_number);
                        sge_dstring_free(&dyn_task_str);
                        sge_dstring_sprintf(&dyn_task_str, u32, jataskid);
                        different = (jid != old_jid) || (jataskid != old_jataskid);
                        if (!already_printed && (full_listing & QSTAT_DISPLAY_RUNNING) &&
                              (lGetUlong(jatep, JAT_state) & JRUNNING)) {
                           sge_print_job(jlep, jatep, qep, different,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              lGetUlong(gdilep, JG_slots)+slot_adjust, i, ehl, cl, pe_list, indent);   
                           already_printed = 1;
                        }
                        if (!already_printed && (full_listing & QSTAT_DISPLAY_SUSPENDED) &&
                           ((lGetUlong(jatep, JAT_state)&JSUSPENDED) ||
                           (lGetUlong(jatep, JAT_state)&JSUSPENDED_ON_THRESHOLD) ||
                            (lGetUlong(jatep, JAT_state)&JSUSPENDED_ON_SUBORDINATE))) {
                           sge_print_job(jlep, jatep, qep, different,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              lGetUlong(gdilep, JG_slots)+slot_adjust, i, ehl, cl, pe_list, indent);   
                           already_printed = 1;
                        }

                        if (!already_printed && (full_listing & QSTAT_DISPLAY_USERHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_USER)) {
                           sge_print_job(jlep, jatep, qep, different,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              lGetUlong(gdilep, JG_slots)+slot_adjust, i, ehl, cl, pe_list, indent);
                           already_printed = 1;
                        }

                        if (!already_printed && (full_listing & QSTAT_DISPLAY_OPERATORHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_OPERATOR))  {
                           sge_print_job(jlep, jatep, qep, different,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              lGetUlong(gdilep, JG_slots)+slot_adjust, i, ehl, cl, pe_list, indent);
                           already_printed = 1;
                        }
                            
                        if (!already_printed && (full_listing & QSTAT_DISPLAY_SYSTEMHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_SYSTEM)) {
                           sge_print_job(jlep, jatep, qep, different,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              lGetUlong(gdilep, JG_slots)+slot_adjust, i, ehl, cl, pe_list, indent);
                           already_printed = 1;
                        }
                     }
                  }
               }
               first = 0;
            }
         }
      }
   }
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* pending jobs matching the queues                                        */
/*-------------------------------------------------------------------------*/
void sge_print_jobs_pending(
lList *job_list,
lList *user_list,
lList *ehl,
lList *cl,
lList **prunning_per_user,
lSortOrder *so,
u_long32 full_listing,
u_long32 group_opt 
) {
   lListElem *nxt, *jep, *jatep, *nxt_jatep;
   int sge_ext;
   dstring dyn_task_str = DSTRING_INIT;
   lList* ja_task_list = NULL;
   int FoundTasks;

   DENTER(TOP_LAYER, "sge_print_jobs_pending");

   sge_ext = feature_is_enabled(FEATURE_SGEEE) 
              && (full_listing & QSTAT_DISPLAY_EXTENDED);

   nxt = lFirst(job_list);
   while ((jep=nxt)) {
      nxt = lNext(jep);

      sge_dstring_free(&dyn_task_str);
      nxt_jatep = lFirst(lGetList(jep, JB_ja_tasks));
      FoundTasks = 0;
      while((jatep = nxt_jatep)) { 
         if (shut_me_down) {
            SGE_EXIT(1);
         }   
         nxt_jatep = lNext(jatep);

         if (!(((full_listing & QSTAT_DISPLAY_OPERATORHOLD) && (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_OPERATOR))  
               ||
             ((full_listing & QSTAT_DISPLAY_USERHOLD) && (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_USER)) 
               ||
             ((full_listing & QSTAT_DISPLAY_SYSTEMHOLD) && (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_SYSTEM)) 
               ||
             ((full_listing & QSTAT_DISPLAY_JOBHOLD) && lGetList(jep, JB_jid_predecessor_list))
               ||
             ((full_listing & QSTAT_DISPLAY_STARTTIMEHOLD) && lGetUlong(jep, JB_execution_time))
               ||
             !(full_listing & QSTAT_DISPLAY_HOLD))
            ) {
            break;
         }

         if (!(lGetUlong(jatep, JAT_suitable) & TAG_FOUND_IT) && 
            VALID(JQUEUED, lGetUlong(jatep, JAT_state))) {
            lSetUlong(jatep, JAT_suitable, 
            lGetUlong(jatep, JAT_suitable)|TAG_FOUND_IT);

            if ((!lGetNumberOfElem(user_list) || 
               (lGetNumberOfElem(user_list) && 
               (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) &&
               (lGetUlong(jatep, JAT_suitable)&TAG_SHOW_IT)) {

               sge_printf_header((full_listing & QSTAT_DISPLAY_FULL) |
                                 (full_listing & QSTAT_DISPLAY_PENDING), 
                                 sge_ext);

               if ((full_listing & QSTAT_DISPLAY_PENDING) && 
                   group_opt != GROUP_TASK_GROUPS) {
                  sge_dstring_free(&dyn_task_str);
                  sge_dstring_sprintf(&dyn_task_str, u32, 
                                    lGetUlong(jatep, JAT_task_number));
                  sge_print_job(jep, jatep, NULL, 1, NULL,
                                &dyn_task_str, full_listing, 0, 0, ehl, cl, 
                                NULL, "");
               } else {
                  if (!ja_task_list) {
                     ja_task_list = lCreateList("", JAT_Type);
                  }
                  lAppendElem(ja_task_list, lCopyElem(jatep));
                  FoundTasks = 1;
               }
            }
         }
      }
      if ((full_listing & QSTAT_DISPLAY_PENDING)  && 
            group_opt == GROUP_TASK_GROUPS && 
            FoundTasks && ja_task_list) {
         lList *task_group = NULL;

         while ((task_group = ja_task_list_split_group(&ja_task_list))) {
            jatask_list_print_to_string(task_group, &dyn_task_str);

            sge_print_job(jep, lFirst(task_group), NULL, 1, NULL, 
                          &dyn_task_str, full_listing, 0, 0, ehl, cl, NULL, "");
            task_group = lFreeList(task_group);
            sge_dstring_free(&dyn_task_str);
         }
         ja_task_list = lFreeList(ja_task_list);
      }
      if (jep != nxt && full_listing & QSTAT_DISPLAY_PENDING) {
         sge_print_jobs_not_enrolled(jep, NULL, 1, NULL, full_listing,
                                     0, 0, ehl, cl, NULL, "", sge_ext, 
                                     group_opt);
      }
   }
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}

static void sge_printf_header(u_long32 full_listing, u_long32 sge_ext)
{
   static int first_pending = 1;
   static int first_zombie = 1;

   if ((full_listing & QSTAT_DISPLAY_PENDING) && 
       (full_listing & QSTAT_DISPLAY_FULL)) {
      if (first_pending) {
         first_pending = 0;
         printf("\n############################################################################%s\n",
            sge_ext?hashes:"");
         printf(MSG_QSTAT_PRT_PEDINGJOBS);
         printf("############################################################################%s\n",
            sge_ext?hashes:"");
      }
   } 
   if (full_listing & QSTAT_DISPLAY_ZOMBIES) {
      if (first_zombie) {
         first_zombie = 0;
         printf("\n################################################################################%s\n", sge_ext?hashes:"");
         printf(MSG_QSTAT_PRT_FINISHEDJOBS);
         printf(  "################################################################################%s\n", sge_ext?hashes:""); 
      }
   }
}

static int sge_print_jobs_not_enrolled(lListElem *job, lListElem *qep,
                           int print_jobid, char *master, u_long32 full_listing,
                           int slots, int slot, lList *exechost_list,
                           lList *complex_list, lList *pe_list, char *indent,
                           u_long32 sge_ext, u_long32 group_opt)
{
   lList *range_list[8];         /* RN_Type */
   u_long32 hold_state[8];
   int i;
 
   DENTER(TOP_LAYER, "sge_print_jobs_not_enrolled");
 
   job_create_hold_id_lists(job, range_list, hold_state); 
   for (i = 0; i <= 7; i++) {
      dstring ja_task_id_string = DSTRING_INIT;
      lList *answer_list = NULL;
      u_long32 first_id;
      int show = 0;

      if (((full_listing & QSTAT_DISPLAY_USERHOLD) && (hold_state[i] & MINUS_H_TGT_USER)) ||
          ((full_listing & QSTAT_DISPLAY_OPERATORHOLD) && (hold_state[i] & MINUS_H_TGT_OPERATOR)) ||
          ((full_listing & QSTAT_DISPLAY_SYSTEMHOLD) && (hold_state[i] & MINUS_H_TGT_SYSTEM)) ||
          ((full_listing & QSTAT_DISPLAY_STARTTIMEHOLD) && (lGetUlong(job, JB_execution_time) > 0)) ||
          ((full_listing & QSTAT_DISPLAY_JOBHOLD) && (lGetList(job, JB_jid_predecessor_list) != 0)) ||
          (!(full_listing & QSTAT_DISPLAY_HOLD))
         ) {
         show = 1;
      }

      if (range_list[i] != NULL && show) { 
         if (group_opt == GROUP_TASK_GROUPS) {
            range_list_print_to_string(range_list[i], &ja_task_id_string, 0);
            first_id = range_list_get_first_id(range_list[i], &answer_list);
            if (answer_list_has_error(&answer_list) != 1) {
               lListElem *ja_task = job_get_ja_task_template_hold(job, 
                                                      first_id, hold_state[i]);
               lList *n_h_ids = NULL;
               lList *u_h_ids = NULL;
               lList *o_h_ids = NULL;
               lList *s_h_ids = NULL;

               sge_printf_header(full_listing, sge_ext);
               lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
               lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
               lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
               lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
               sge_print_job(job, ja_task, qep, print_jobid, master,
                             &ja_task_id_string, full_listing, slots, slot,
                             exechost_list, complex_list, pe_list, indent);
               lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
               lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
               lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
               lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
            }
            sge_dstring_free(&ja_task_id_string);
         } else {
            lListElem *range; /* RN_Type */
            
            for_each(range, range_list[i]) {
               u_long32 start, end, step;

               range_get_all_ids(range, &start, &end, &step);
               for (; start <= end; start += step) { 
                  lListElem *ja_task = job_get_ja_task_template_hold(job,
                                                          start, hold_state[i]);

                  sge_dstring_free(&ja_task_id_string);
                  sge_dstring_sprintf(&ja_task_id_string, u32, start);
                  sge_print_job(job, ja_task, NULL, 1, NULL,
                                &ja_task_id_string, full_listing, 0, 0, 
                                exechost_list, complex_list, pe_list, indent);
               }
            }
         }
      }
   }
   job_destroy_hold_id_lists(job, range_list); 
   DEXIT;
   return STATUS_OK;
}                          

/*-------------------------------------------------------------------------*/
/* print the finished jobs in case of SGE                                  */
/*-------------------------------------------------------------------------*/
void sge_print_jobs_finished(
lList *job_list,
lList *user_list,
lList *ehl,
lList *cl,
u_long32 full_listing 
) {
   int sge_ext;
   int first = 1;
   lListElem *jep, *jatep;
   dstring dyn_task_str = DSTRING_INIT;

   DENTER(TOP_LAYER, "sge_print_jobs_finished");

   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
               (full_listing & QSTAT_DISPLAY_EXTENDED);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      for_each (jep, job_list) {
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if (shut_me_down) {
               SGE_EXIT(1);
            }   
            if (lGetUlong(jatep, JAT_status) == JFINISHED) {
               lSetUlong(jatep, JAT_suitable, lGetUlong(jatep, JAT_suitable)|TAG_FOUND_IT);

               if (!getenv("MORE_INFO"))
                  continue;

               if (!lGetNumberOfElem(user_list) || (lGetNumberOfElem(user_list) && 
                     (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) {
                  if (first) {
                     first = 0;
                     printf("\n################################################################################%s\n", sge_ext?hashes:"");
                     printf(MSG_QSTAT_PRT_JOBSWAITINGFORACCOUNTING);
                     printf(  "################################################################################%s\n", sge_ext?hashes:"");
                  }
                  sge_dstring_free(&dyn_task_str);
                  sge_dstring_sprintf(&dyn_task_str, u32, 
                                    lGetUlong(jatep, JAT_task_number));
                  sge_print_job(jep, jatep, NULL, 1, NULL, &dyn_task_str, 
                                full_listing, 0, 0, ehl, cl, NULL, "");   
                  
               }
            }
         }
      }
   }
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}
  
/*-------------------------------------------------------------------------*/
/* print the jobs in error                                                 */
/*-------------------------------------------------------------------------*/
void sge_print_jobs_error(
lList *job_list,
lList *user_list,
lList *ehl,
lList *cl,
u_long32 full_listing 
) {
   int first = 1;
   lListElem *jep, *jatep;
   int sge_ext;
   dstring dyn_task_str = DSTRING_INIT;

   DENTER(TOP_LAYER, "sge_print_jobs_error");

   sge_ext = feature_is_enabled(FEATURE_SGEEE) 
              && (full_listing & QSTAT_DISPLAY_EXTENDED);

   for_each (jep, job_list) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         if (!(lGetUlong(jatep, JAT_suitable) & TAG_FOUND_IT) && lGetUlong(jatep, JAT_status) == JERROR) {
            lSetUlong(jatep, JAT_suitable, lGetUlong(jatep, JAT_suitable)|TAG_FOUND_IT);

            if (!lGetNumberOfElem(user_list) || (lGetNumberOfElem(user_list) && 
                  (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) {
               if (first) {
                  first = 0;
                  printf("\n################################################################################%s\n", sge_ext?hashes:"");
                  printf(MSG_QSTAT_PRT_ERRORJOBS);
                  printf("################################################################################%s\n", sge_ext?hashes:"");
               }
               sge_dstring_free(&dyn_task_str);
               sge_dstring_sprintf(&dyn_task_str, "u32", lGetUlong(jatep, JAT_task_number));
               sge_print_job(jep, jatep, NULL, 1, NULL, &dyn_task_str, full_listing, 0, 0, ehl, cl, NULL, "");
            }
         }
      }
   }
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* print the zombie jobs                                                   */
/*-------------------------------------------------------------------------*/
void sge_print_jobs_zombie(
lList *zombie_list,
lList *user_list,
lList *ehl,
lList *cl,
u_long32 full_listing 
) {
   int sge_ext;
   lListElem *jep;
   dstring dyn_task_str = DSTRING_INIT; 

   DENTER(TOP_LAYER, "sge_print_jobs_zombie");
   
   if (! (full_listing & QSTAT_DISPLAY_ZOMBIES)) {
      DEXIT;
      return;
   }

   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
               (full_listing & QSTAT_DISPLAY_EXTENDED);

   for_each (jep, zombie_list) { 
      lList *z_ids = NULL;

      z_ids = lGetList(jep, JB_ja_z_ids);
      if (z_ids != NULL) {
         lListElem *ja_task = NULL;
         u_long32 first_task_id = range_list_get_first_id(z_ids, NULL);

         sge_printf_header(full_listing & 
                           (QSTAT_DISPLAY_ZOMBIES | QSTAT_DISPLAY_FULL), 
                           sge_ext);
         ja_task = job_get_ja_task_template_pending(jep, first_task_id);
         range_list_print_to_string(z_ids, &dyn_task_str, 0);
         sge_print_job(jep, ja_task, NULL, 1, NULL, &dyn_task_str, 
                       full_listing, 0, 0, ehl, cl, NULL, "");
         sge_dstring_free(&dyn_task_str);
      }
   }
   DEXIT;
}

static char jhul1[] = "---------------------------------------------------------------------------------------------";
static char jhul2[] = "-------------------------------------------------------------------------------------------------------------";
static char jhul3[] = "---------------------------------------------------------------------------";

static int sge_print_job(
lListElem *job,
lListElem *jatep,
lListElem *qep,
int print_jobid,
char *master,
dstring *dyn_task_str,
u_long32 full_listing,
int slots,
int slot,
lList *exechost_list,
lList *complex_list,
lList *pe_list,
char *indent 
) {
   char state_string[8];
   static int first_time = 1;
   u_long32 jstate;
   int sge_ext, sge_mode;
   lList *ql = NULL;
   lListElem *qrep, *gdil_ep=NULL;
   int running;
   const char *queue_name;
   int tsk_ext;
   u_long tickets,otickets,dtickets,stickets,ftickets;
   int is_zombie_job;

   DENTER(TOP_LAYER, "sge_print_job");

   is_zombie_job = job_is_zombie_job(job);

   queue_name = qep ? lGetString(qep, QU_qname) : NULL;

   sge_mode = feature_is_enabled(FEATURE_SGEEE);
   sge_ext = sge_mode && (full_listing & QSTAT_DISPLAY_EXTENDED);
   tsk_ext = (full_listing & QSTAT_DISPLAY_TASKS);

   if (first_time) {
      first_time = 0;
      if (!(full_listing & QSTAT_DISPLAY_FULL)) {
         printf("%s%-7.7s %-5.5s %-10.10s %-12.12s %s%-5.5s %-19.19s%s %s%s%s%s%s%s%s%s%-10.10s %s  %s%s%s%s%s%s", 
               indent,
                  "job-ID",
                  "prior",
                  "name",
                  "user",
               sge_ext?"project          department ":"",
                  "state",
                  "submit/start at",
               sge_ext ? " deadline           "
                       : "",
               sge_ext ? USAGE_ATTR_CPU "        " USAGE_ATTR_MEM "     " USAGE_ATTR_IO "      "
                       : "",
               sge_ext?"tckts ":"",
               sge_ext?"ovrts ":"",
               sge_ext?"otckt ":"",
               sge_ext?"dtckt ":"",
               sge_ext?"ftckt ":"",
               sge_ext?"stckt ":"",
               sge_ext?"share ":"",
                  "queue",
                  "master",
                  "ja-task-ID ", 
               tsk_ext?"task-ID ":"",
               tsk_ext?"state ":"",
               tsk_ext?(sge_mode ? USAGE_ATTR_CPU "        " USAGE_ATTR_MEM "     " USAGE_ATTR_IO "      " : "") : "",
               tsk_ext?"stat ":"",
               tsk_ext?"failed ":"" );

         printf("\n%s%s%s%s\n", indent, jhul1, sge_ext ? jhul2 : "", tsk_ext ? jhul3 : "");
      }
   }
      
   printf("%s", indent);

   /* job number / ja task id */
   if (print_jobid)
      printf("%7d ", (int)lGetUlong(job, JB_job_number)); 
   else 
      printf("       "); 

   /* job priority */
   printf("%5d ", ((int)lGetUlong(job, JB_priority))-BASE_PRIORITY); 


   /* job name */
   printf("%-10.10s ", lGetString(job, JB_job_name)); 

   /* job owner */
   printf("%-12.12s ", lGetString(job, JB_owner)); 

   if (sge_ext) {
      const char *s;

      /* job project */
      printf("%-16.16s ", (s=lGetString(job, JB_project))?s:"NA"); 
      /* job department */
      printf("%-10.10s ", (s=lGetString(job, JB_department))?s:"NA"); 
   }

   /* move status info into state info */
   jstate = lGetUlong(jatep, JAT_state);
   if (lGetUlong(jatep, JAT_status)==JTRANSFERING) {
      jstate |= JTRANSFERING;
      jstate &= ~JRUNNING;
   }

   if (lGetList(job, JB_jid_predecessor_list) || lGetUlong(jatep, JAT_hold)) {
      jstate |= JHELD;
   }

   if (lGetUlong(jatep, JAT_job_restarted)) {
      jstate &= ~JWAITING;
      jstate |= JMIGRATING;
   }

   /* write states into string */ 
   job_get_state_string(state_string, jstate);
   printf("%-5.5s ", state_string); 

   /* start/submit time */
   if (!lGetUlong(jatep, JAT_start_time) )
      printf("%s ", sge_ctime(lGetUlong(job, JB_submission_time)));
   else
      printf("%s ", sge_ctime(lGetUlong(jatep, JAT_start_time)));

   /* is job logically running */
   running = lGetUlong(jatep, JAT_status)==JRUNNING || 
      lGetUlong(jatep, JAT_status)==JTRANSFERING;

   if (sge_ext) {
      lListElem *up, *pe, *task;
      lList *job_usage_list;
      const char *pe_name;
      
      if (!master || !strcmp(master, "MASTER"))
         job_usage_list = lCopyList(NULL, lGetList(jatep, JAT_scaled_usage_list));
      else
         job_usage_list = lCreateList("", UA_Type);

      /* sum pe-task usage based on queue slots */
      if (job_usage_list) {
         int subtask_ndx=1;
         for_each(task, lGetList(jatep, JAT_task_list)) {
            lListElem *dst, *src, *ep;
            const char *qname;

            if (!slots ||
                (queue_name && 
                 ((ep=lFirst(lGetList(task, PET_granted_destin_identifier_list)))) &&
                 ((qname=lGetString(ep, JG_qname))) &&
                 !strcmp(qname, queue_name) && ((subtask_ndx++%slots)==slot))) {
               for_each(src, lGetList(task, PET_scaled_usage)) {
                  if ((dst=lGetElemStr(job_usage_list, UA_name, lGetString(src, UA_name))))
                     lSetDouble(dst, UA_value, lGetDouble(dst, UA_value) + lGetDouble(src, UA_value));
                  else
                     lAppendElem(job_usage_list, lCopyElem(src));
               }
            }
         }
      }

      /* start/submit time */
      if (!lGetUlong(job, JB_deadline) )
         printf("                    ");
      else
         printf("%s ", sge_ctime(lGetUlong(job, JB_deadline)));

      /* scaled cpu usage */
      if (!(up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_CPU))) 
         printf("%-10.10s ", running?"NA":""); 
      else {
         int secs, minutes, hours, days;

         secs = lGetDouble(up, UA_value);

         days    = secs/(60*60*24);
         secs   -= days*(60*60*24);

         hours   = secs/(60*60);
         secs   -= hours*(60*60);

         minutes = secs/60;
         secs   -= minutes*60;
      
         printf("%d:%2.2d:%2.2d:%2.2d ", days, hours, minutes, secs); 
      } 
      /* scaled mem usage */
      if (!(up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_MEM))) 
         printf("%-7.7s ", running?"NA":""); 
      else
         printf("%-5.5f ", lGetDouble(up, UA_value)); 
  
      /* scaled io usage */
      if (!(up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_IO))) 
         printf("%-7.7s ", running?"NA":""); 
      else
         printf("%-5.5f ", lGetDouble(up, UA_value)); 

      lFreeList(job_usage_list);

      /* get tickets for job/slot */
      /* braces needed to suppress compiler warnings */
      if ((pe_name=lGetString(jatep, JAT_granted_pe)) &&
           (pe=lGetElemStr(pe_list, PE_name, pe_name)) &&
           lGetUlong(pe, PE_control_slaves)
         && slots && (gdil_ep=lGetSubStr(jatep, JG_qname, queue_name,
               JAT_granted_destin_identifier_list))) {
         if (slot == 0) {
            tickets = lGetDouble(gdil_ep, JG_ticket);
            otickets = lGetDouble(gdil_ep, JG_oticket);
            ftickets = lGetDouble(gdil_ep, JG_fticket);
            stickets = lGetDouble(gdil_ep, JG_sticket);
            dtickets = lGetDouble(gdil_ep, JG_dticket);
         }
         else {
            if (slots) {
               tickets = lGetDouble(gdil_ep, JG_ticket) / slots;
               otickets = lGetDouble(gdil_ep, JG_oticket) / slots;
               ftickets = lGetDouble(gdil_ep, JG_fticket) / slots;
               dtickets = lGetDouble(gdil_ep, JG_dticket) / slots;
               stickets = lGetDouble(gdil_ep, JG_sticket) / slots;
            } 
            else {
               tickets = otickets = ftickets = stickets = dtickets = 0;
            }
         }
      }
      else {
         tickets = lGetDouble(jatep, JAT_ticket);
         otickets = lGetDouble(jatep, JAT_oticket);
         ftickets = lGetDouble(jatep, JAT_fticket);
         stickets = lGetDouble(jatep, JAT_sticket);
         dtickets = lGetDouble(jatep, JAT_dticket);
      }


      /* report jobs dynamic scheduling attributes */
      /* only scheduled have these attribute */
      /* Pending jobs can also have tickets */
      if (is_zombie_job) {
         printf("   NA ");
         printf("   NA ");
         printf("   NA ");
         printf("   NA ");
         printf("   NA ");
         printf("   NA ");
         printf("   NA ");
      } else {
         if (sge_ext || lGetList(jatep, JAT_granted_destin_identifier_list)) {
            printf("%5d ", (int)tickets),
            printf("%5d ", (int)lGetUlong(job, JB_override_tickets)); 
            printf("%5d ", (int)otickets);
            printf("%5d ", (int)dtickets);
            printf("%5d ", (int)ftickets);
            printf("%5d ", (int)stickets);
            printf("%-5.2f ", lGetDouble(jatep, JAT_share)); 
         } else {
            printf("      "); 
            printf("      "); 
            printf("      "); 
            printf("      "); 
            printf("      "); 
            printf("      "); 
            printf("      "); 
         }
      }
   }
   /* if not full listing we need the queue's name in each line */
   if (!(full_listing & QSTAT_DISPLAY_FULL))
      printf("%-10.10s ", queue_name?queue_name:"");

   if (master)
      printf("%-8.7s", master);
   else
      printf("        ");

   if (sge_dstring_get_string(dyn_task_str) && job_is_array(job))
      printf("%s", sge_dstring_get_string(dyn_task_str)); 
   else
      printf("       ");

   if (tsk_ext) {
      lList *task_list = lGetList(jatep, JAT_task_list);
      lListElem *task, *ep;
      const char *qname;
      int indent=0;
      int subtask_ndx=1;
      int num_spaces = sizeof(jhul1)-1 + (sge_ext?sizeof(jhul2)-1:0) - 
            ((full_listing & QSTAT_DISPLAY_FULL)?11:0);

      /* print master sub-task belonging to this queue */
      if (!slot && task_list && queue_name &&
          ((ep=lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)))) &&
          ((qname=lGetString(ep, JG_qname))) &&
          !strcmp(qname, queue_name)) {
         if (indent++)
            printf("%*s", num_spaces, " ");
         sge_print_subtask(job, jatep, NULL, 0, 0);
         /* subtask_ndx++; */
      }
         
      /* print sub-tasks belonging to this queue */
      for_each(task, task_list) {
         if (!slots || (queue_name && 
              ((ep=lFirst(lGetList(task, PET_granted_destin_identifier_list)))) &&
              ((qname=lGetString(ep, JG_qname))) &&
              !strcmp(qname, queue_name) && ((subtask_ndx++%slots)==slot))) {
            if (indent++)
               printf("%*s", num_spaces, " ");
            sge_print_subtask(job, jatep, task, 0, 0);
         }
      }

      if (!indent)
         putchar('\n');

   } 
   else {
      /* print a new line */
      putchar('\n');
   }

   /* print additional job info if requested */
   if (print_jobid && (full_listing & QSTAT_DISPLAY_RESOURCES)) {
         printf(QSTAT_INDENT "Full jobname:     %s\n", lGetString(job, JB_job_name)); 
         if (lGetString(job, JB_pe)) {
            dstring range_string = DSTRING_INIT;

            range_list_print_to_string(lGetList(job, JB_pe_range), 
                                       &range_string, 1);
            printf(QSTAT_INDENT "Requested PE:     %s %s\n", 
                   lGetString(job, JB_pe), sge_dstring_get_string(&range_string)); 
            sge_dstring_free(&range_string);
         }
         if (lGetString(jatep, JAT_granted_pe)) {
            lListElem *gdil_ep;
            u_long32 pe_slots = 0;
            for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list))
               pe_slots += lGetUlong(gdil_ep, JG_slots);
            printf(QSTAT_INDENT "Granted PE:       %s "u32"\n", 
               lGetString(jatep, JAT_granted_pe), pe_slots); 
         }
         if (lGetString(job, JB_checkpoint_object)) 
            printf(QSTAT_INDENT "Checkpoint Env.:  %s\n", 
               lGetString(job, JB_checkpoint_object)); 

         sge_show_re_type_list_line_by_line(QSTAT_INDENT "Hard Resources:   ",
               QSTAT_INDENT2, lGetList(job, JB_hard_resource_list)); 

         /* display default requests if necessary */
         {
            lList *attributes = NULL;
            lListElem *ce;
            const char *name;
            lListElem *hep;

            queue_complexes2scheduler(&attributes, qep, exechost_list, complex_list, 0);
            for_each (ce, attributes) {
               double dval;

               name = lGetString(ce, CE_name);
               if (!lGetUlong(ce, CE_consumable) || !strcmp(name, "slots") || explicit_job_request(job, name))
                  continue;

               parse_ulong_val(&dval, NULL, lGetUlong(ce, CE_valtype), lGetString(ce, CE_default), NULL, 0); 
               if (dval == 0.0)
                  continue;

               /* For pending jobs (no queue/no exec host) we may print default request only
                  if the consumable is specified in the global host. For running we print it
                  if the resource is managed at this node/queue */
               if ((qep && lGetSubStr(qep, CE_name, name, QU_consumable_config_list)) ||
                   (qep && (hep=lGetElemHost(exechost_list, EH_name, lGetHost(qep, QU_qhostname))) &&
                    lGetSubStr(hep, CE_name, name, EH_consumable_config_list)) ||
                     ((hep=lGetElemHost(exechost_list, EH_name, SGE_GLOBAL_NAME)) &&
                         lGetSubStr(hep, CE_name, name, EH_consumable_config_list)))

               printf("%s%s=%s (default)\n", QSTAT_INDENT, name, lGetString(ce, CE_default));      
            }
            lFreeList(attributes);
         }

         sge_show_re_type_list_line_by_line(QSTAT_INDENT "Soft Resources:   ",
               QSTAT_INDENT2, lGetList(job, JB_soft_resource_list)); 

         ql = lGetList(job, JB_hard_queue_list);
         if (ql) {
            printf(QSTAT_INDENT "Hard requested queues: ");
            for_each(qrep, ql) {
               printf("%s", lGetString(qrep, QR_name));
               printf("%s", lNext(qrep)?", ":"\n");
            }
         }

         ql = lGetList(job, JB_soft_queue_list);
         if (ql) {
            printf(QSTAT_INDENT "Soft requested queues: ");
            for_each(qrep, ql) {
               printf("%s", lGetString(qrep, QR_name));
               printf("%s", lNext(qrep)?", ":"\n");
            }
         }

         ql = lGetList(job, JB_master_hard_queue_list);
         if (ql) {
            printf(QSTAT_INDENT "Master task hard requested queues: ");
            for_each(qrep, ql) {
               printf("%s", lGetString(qrep, QR_name));
               printf("%s", lNext(qrep)?", ":"\n");
            }
         }

         ql = lGetList(job, JB_jid_predecessor_list);
         if (ql) {
            printf(QSTAT_INDENT "Predecessor Jobs: ");
            for_each(qrep, ql) {
               printf(u32, lGetUlong(qrep, JRE_job_number));
               printf("%s", lNext(qrep)?", ":"\n");
            }
         }
   }

#if 0

   /* this code displays the sub-task list as a sub-list under resources */

   if (print_jobid && (full_listing & QSTAT_DISPLAY_RESOURCES)) {
      lList *task_list = lGetList(job, JB_task_list);
      lListElem *task, *ep;
      char *qname;
      int no_header=0;

      /* print master sub-task belonging to this queue */
      if (task_list && queue_name &&
          ((ep=lFirst(lGetList(job, JB_granted_destin_identifier_list)))) &&
          ((qname=lGetString(ep, JG_qname))) &&
          !strcmp(qname, queue_name)) {
         sge_print_subtask(job, job, !no_header++, 1);
      }
         
      /* print sub-tasks belonging to this queue */
      for_each(task, task_list)
         if (queue_name && 
             ((ep=lFirst(lGetList(task, JB_granted_destin_identifier_list)))) &&
             ((qname=lGetString(ep, JG_qname))) &&
             !strcmp(qname, queue_name)) {
            sge_print_subtask(job, task, !no_header++, 1);
         }


   }

#endif

#undef QSTAT_INDENT
#undef QSTAT_INDENT2

   DEXIT;
   return 1;
}

static void qtype(
char *type_string,
u_long32 type 
) {
   int i; 
   char *s = type_string;

   DENTER(TOP_LAYER, "qtype");
   

   /* collect first characters of the queue_types array */
   /* if the corresponding bit in type is set           */

   for (i=0; type && i<=4; i++) {

      DPRINTF(("i = %d qtype: %d (u_long)\n", i, (int)type));
      if (type & 1) {  
         *s++ = queue_types[i][0];
         *s = '\0';
         DPRINTF(("qtype: %s (string)\n", type_string));
      }
      type >>= 1;
   } 

   *s = '\0';


   DEXIT;
   return;
}
