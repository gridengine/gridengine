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
#include <fnmatch.h>
#include <ctype.h>

#include "sgermon.h"
#include "symbols.h"
#include "sge.h"
#include "sge_gdi.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "sge_host.h"
#include "sig_handlers.h"
#include "sge_sched.h"
#include "cull_sort.h"
#include "usage.h"
#include "sge_dstring.h"
#include "sge_feature.h"
#include "parse.h"
#include "sge_prog.h"
#include "sge_parse_num_par.h"
#include "sge_string.h"
#include "show_job.h"
#include "qstat_printing.h"
#include "sge_range.h"
#include "sge_schedd_text.h"
#include "qm_name.h"
#include "load_correction.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qstat.h"
#include "sge_conf.h" 
#include "sgeee.h" 
#include "sge_support.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_qinstance.h"
#include "sge_qinstance_message.h"
#include "sge_qinstance_state.h"
#include "sge_centry.h"
#include "sge_schedd_conf.h"
#include "sge_cqueue.h"
#include "sge_qref.h"

#include "cull/cull_xml.h"
#include "sge_job.h"
#include "sge_urgency.h"
#include "sge_ulong.h"

#include "sge_mt_init.h"

static void xml_print_jobs_pending(lList *job_list, const lList *pe_list, const lList *user_list, 
                    const lList *exechost_list, const lList *centry_list, lSortOrder *so,
                    u_long32 full_listing, u_long32 group_opt, lList **target_list); 

static lListElem* sge_job_to_XML( lListElem *job, lListElem *jatep, lListElem *qep,
                                 int print_jobid, char *master, dstring *dyn_task_str,
                                 u_long32 full_listing, int slots, int slot, 
                                 const lList *exechost_list, const lList *centry_list, 
                                 const lList *pe_list, u_long32 group_opt, int slots_per_line);

static int xml_jobs_not_enrolled(lListElem *job, lListElem *qep,
                        int print_jobid, char *master, u_long32 full_listing,
                        int slots, int slot, const lList *exechost_list,
                        const lList *centry_list, const lList *pe_list,
                        u_long32 sge_ext, u_long32 group_opt, lList *target_list);

static lListElem *xml_subtask(lListElem *job, lListElem *ja_task,
                             lListElem *pe_task,  int print_hdr); 

static void xml_print_jobs_finished(lList *job_list, const lList *pe_list, const lList *user_list,
                             const lList *exechost_list, const lList *centry_list, 
                             u_long32 full_listing, u_long32 group_opt, lList **target_list);

static void xml_print_jobs_error( lList *job_list, const lList *pe_list, const lList *user_list,
                           const lList *exechost_list, const lList *centry_list,
                           u_long32 full_listing, u_long32 group_opt, lList **target_list); 

static void xml_print_jobs_zombie(lList *zombie_list, const lList *pe_list, const lList *user_list,
                           const lList *exechost_list, const lList *centry_list,
                           u_long32 full_listing, u_long32 group_opt, lList **target_list);



void xml_qstat_show_job_info(lList **list, lList **answer_list){
   lListElem *answer = NULL;
   lListElem *xml_elem = NULL;
   bool error = false;
   DENTER(TOP_LAYER, "xml_qstat_show_job");

   for_each(answer, *answer_list) {
      if (lGetUlong(answer, AN_status) != STATUS_OK) {
         error = true;
         break;
      }
   }

   if (error) {
      xml_elem = xml_getHead("comunication_error", *answer_list, NULL);
      lWriteElemXMLTo(xml_elem, stdout);
      lFreeElem(xml_elem);
   }
   else {
      xml_elem = xml_getHead("message", *list, NULL);
      lWriteElemXMLTo(xml_elem, stdout);
      lFreeElem(xml_elem);
      *list = NULL;
   }

   *answer_list = lFreeList(*answer_list);
 
   DEXIT;
   return;
}

void xml_qstat_show_job(lList **job_list, lList **msg_list, lList **answer_list, lList **id_list){
   lListElem *answer = NULL;
   lListElem *xml_elem = NULL;
   bool error = false;
   DENTER(TOP_LAYER, "xml_qstat_show_job");
   for_each(answer, *answer_list) {
      if (lGetUlong(answer, AN_status) != STATUS_OK) {
         error = true;
         break;
      }
   }
   if (error) {
      xml_elem = xml_getHead("comunication_error", *answer_list, NULL);
      lWriteElemXMLTo(xml_elem, stdout);
      lFreeElem(xml_elem);
   }
   else {
      if (lGetNumberOfElem(*job_list) == 0) {
         xml_elem = xml_getHead("unknown_jobs", *id_list, NULL);
         lWriteElemXMLTo(xml_elem, stdout);
         lFreeElem(xml_elem);
         *id_list = NULL;
      }
      else {
         xml_elem = xml_getHead("detailed_job_info", *job_list, NULL);
         
         lWriteElemXMLTo(xml_elem, stdout);

         xml_elem = lFreeElem(xml_elem);
         *job_list = NULL;

         xml_elem = xml_getHead("message", *msg_list, NULL);

         lWriteElemXMLTo(xml_elem, stdout);
         xml_elem = lFreeElem(xml_elem);
         *msg_list = NULL;
      }
   }

   *answer_list = lFreeList(*answer_list);

   DEXIT;
   return;
}


void xml_qstat_jobs(lList *job_list, lList *zombie_list, const lList *pe_list, 
                    const lList *user_list, const lList *exechost_list, 
                    const lList *centry_list, lSortOrder *so, 
                    u_long32 full_listing, u_long32 group_opt, lList **target_list) {

   DENTER(TOP_LAYER, "xml_qstat_jobs");
   
   /* 
    *
    * step 4: iterate over jobs that are pending;
    *         tag them with TAG_FOUND_IT
    *
    *         print the jobs that run in these queues 
    *
    */
   xml_print_jobs_pending(job_list, pe_list, user_list, exechost_list,
                          centry_list, so, full_listing, group_opt, target_list);

   /* 
    *
    * step 5:  in case of SGE look for finished jobs and view them as
    *          finished  a non SGE-qstat will show them as error jobs
    *
    */
   xml_print_jobs_finished(job_list, pe_list, user_list, exechost_list,
                           centry_list, full_listing, group_opt, target_list);

   /*
    *
    * step 6:  look for jobs not found. This should not happen, cause each
    *          job is running in a queue, or pending. But if there is
    *          s.th. wrong we have
    *          to ensure to print this job just to give hints whats wrong
    *
    */

   xml_print_jobs_error(job_list, pe_list, user_list, exechost_list,
                        centry_list, full_listing, group_opt, target_list);

   /*
    *
    * step 7:  print recently finished jobs ('zombies')
    *
    */
   xml_print_jobs_zombie(zombie_list, pe_list, user_list, exechost_list,
                         centry_list,  full_listing, group_opt, target_list);
/*
   xml_elem = xml_getHead("job_info", target_list, NULL);
         
   lWriteElemXMLTo(xml_elem, stdout);
  
   xml_elem = lFreeElem(xml_elem);
  */ 
   DEXIT;
}



static void xml_print_jobs_pending(lList *job_list, const lList *pe_list, const lList *user_list, 
                    const lList *exechost_list, const lList *centry_list, lSortOrder *so,
                    u_long32 full_listing, u_long32 group_opt, lList **target_list) {
   
   lListElem *nxt, *jep, *jatep, *nxt_jatep;
   int sge_ext;
   dstring dyn_task_str = DSTRING_INIT;
   lList* ja_task_list = NULL;
   int FoundTasks;

   DENTER(TOP_LAYER, "sge_print_jobs_pending");

   sge_ext = feature_is_enabled(FEATURE_SGEEE) 
              && (full_listing & QSTAT_DISPLAY_EXTENDED);

   if (*target_list == NULL){
      *target_list = lCreateList("job-list", XMLE_Type);
   }
   
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

               if ((full_listing & QSTAT_DISPLAY_PENDING) && 
                   (group_opt & GROUP_NO_TASK_GROUPS) > 0) {
                  lListElem *elem = NULL;
                  
                  sge_dstring_sprintf(&dyn_task_str, u32, 
                                    lGetUlong(jatep, JAT_task_number));
                  elem = sge_job_to_XML(jep, jatep, NULL, 1, NULL,
                                &dyn_task_str, full_listing, 0, 0, exechost_list, centry_list, 
                                pe_list, group_opt, 0);
                  
                  if (elem) {
                     lList *attributes = NULL;
                     lListElem *pending = lCreateElem(XMLA_Type);
                     attributes = lGetList(elem, XMLE_Attribute);
                     
                     if (!attributes){
                        attributes = lCreateList("attributes", XMLA_Type);
                        lSetList(elem, XMLE_Attribute, attributes);
                     }
                     lSetString(pending, XMLA_Name, "state");
                     lSetString(pending, XMLA_Value, "pending");
                     lAppendElem(attributes, pending);

                     lAppendElem(*target_list, elem); 
                  }
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
          (group_opt & GROUP_NO_TASK_GROUPS) == 0 && 
          FoundTasks && 
          ja_task_list) {
         lList *task_group = NULL;

         while ((task_group = ja_task_list_split_group(&ja_task_list))) {
            lListElem *elem = NULL;

            ja_task_list_print_to_string(task_group, &dyn_task_str);
            elem = sge_job_to_XML(jep, lFirst(task_group), NULL, 1, NULL, 
                          &dyn_task_str, full_listing, 0, 0, exechost_list, centry_list, pe_list, group_opt, 0);
            task_group = lFreeList(task_group);
            sge_dstring_free(&dyn_task_str);
            if (elem) {
               lList *attributes = NULL;
               lListElem *pending = lCreateElem(XMLA_Type);
               attributes = lGetList(elem, XMLE_Attribute);
               
               if (!attributes){
                  attributes = lCreateList("attributes", XMLA_Type);
                  lSetList(elem, XMLE_Attribute, attributes);
               }
               lSetString(pending, XMLA_Name, "state");
               lSetString(pending, XMLA_Value, "pending");
               lAppendElem(attributes, pending);
               
               lAppendElem(*target_list, elem); 
            }
         }
         ja_task_list = lFreeList(ja_task_list);
      }
  
      if (jep != nxt && full_listing & QSTAT_DISPLAY_PENDING) {

         xml_jobs_not_enrolled(jep, NULL, 1, NULL, full_listing,
                                     0, 0, exechost_list, centry_list, pe_list, sge_ext, 
                                     group_opt, *target_list);
      }
      
   }
   
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}

static int xml_jobs_not_enrolled(lListElem *job, lListElem *qep,
                        int print_jobid, char *master, u_long32 full_listing,
                        int slots, int slot, const lList *exechost_list,
                        const lList *centry_list, const lList *pe_list,
                        u_long32 sge_ext, u_long32 group_opt, lList *target_list)
{
   lList *range_list[8];         /* RN_Type */
   u_long32 hold_state[8];
   int i;
   dstring ja_task_id_string = DSTRING_INIT;

   DENTER(TOP_LAYER, "xml_jobs_not_enrolled");

   job_create_hold_id_lists(job, range_list, hold_state); 
   for (i = 0; i <= 7; i++) {
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
         if ((group_opt & GROUP_NO_TASK_GROUPS) == 0) {
            range_list_print_to_string(range_list[i], &ja_task_id_string, 0);
            first_id = range_list_get_first_id(range_list[i], &answer_list);
            if (answer_list_has_error(&answer_list) != 1) {
               lListElem *ja_task = (lListElem *)job_get_ja_task_template_hold(job, first_id, 
                                                                  hold_state[i]);
               lList *n_h_ids = NULL;
               lList *u_h_ids = NULL;
               lList *o_h_ids = NULL;
               lList *s_h_ids = NULL;
               lListElem *elem = NULL;
               
               lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
               lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
               lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
               lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
               
               elem = sge_job_to_XML(job, ja_task, qep, print_jobid, master,
                             &ja_task_id_string, full_listing, slots, slot,
                             exechost_list, centry_list, pe_list, group_opt, 0);
              
               if (elem) {
                  lList *attributes = NULL;
                  lListElem *pending = lCreateElem(XMLA_Type);
                  attributes = lGetList(elem, XMLE_Attribute);
                  
                  if (!attributes){
                     attributes = lCreateList("attributes", XMLA_Type);
                     lSetList(elem, XMLE_Attribute, attributes);
                  }
                  lSetString(pending, XMLA_Name, "state");
                  lSetString(pending, XMLA_Value, "pending");
                  lAppendElem(attributes, pending);

                  lAppendElem(target_list, elem); 
               }
               
               lXchgList(job, JB_ja_n_h_ids, &n_h_ids);
               lXchgList(job, JB_ja_u_h_ids, &u_h_ids);
               lXchgList(job, JB_ja_o_h_ids, &o_h_ids);
               lXchgList(job, JB_ja_s_h_ids, &s_h_ids);
            }
            sge_dstring_free(&ja_task_id_string);
         } else {
            lListElem *elem = NULL;
            lListElem *range; /* RN_Type */ 
            
            for_each(range, range_list[i]) {
               u_long32 start, end, step;
               range_get_all_ids(range, &start, &end, &step);
               for (; start <= end; start += step) { 
                  lListElem *ja_task = (lListElem *)job_get_ja_task_template_hold( job, start, 
                                                                     hold_state[i]);
                  sge_dstring_sprintf(&ja_task_id_string, u32, start);
                  elem = sge_job_to_XML(job, ja_task, NULL, 1, NULL,
                                &ja_task_id_string, full_listing, 0, 0, 
                                exechost_list, centry_list, pe_list, group_opt, 0);
                  if (elem) {
                     lList *attributes = NULL;
                     lListElem *pending = lCreateElem(XMLA_Type);
                     attributes = lGetList(elem, XMLE_Attribute);
                     
                     if (!attributes){
                        attributes = lCreateList("attributes", XMLA_Type);
                        lSetList(elem, XMLE_Attribute, attributes);
                     }
                     lSetString(pending, XMLA_Name, "state");
                     lSetString(pending, XMLA_Value, "pending");
                     lAppendElem(attributes, pending);
                     
                     lAppendElem(target_list, elem); 
                  }                  
               }
            }
         }
      }
   }
   job_destroy_hold_id_lists(job, range_list); 
   sge_dstring_free(&ja_task_id_string);
   DEXIT;
   return STATUS_OK;
}     

static lListElem* sge_job_to_XML(
lListElem *job,
lListElem *jatep,
lListElem *qep,
int print_jobid,
char *master,
dstring *dyn_task_str,
u_long32 full_listing,
int slots,
int slot,
const lList *exechost_list,
const lList *centry_list,
const lList *pe_list,
u_long32 group_opt,
int slots_per_line  /* number of slots to be printed in slots column 
                       when 0 is passed the number of requested slots printed */
) {
   char state_string[8];
   u_long32 jstate;
   int sge_urg, sge_ext, sge_pri, sgeee_mode;
   lList *ql = NULL;
   lListElem *qrep, *gdil_ep=NULL;
   int running;
   const char *queue_name;
   int tsk_ext;
   u_long tickets,otickets,stickets,ftickets;
   int is_zombie_job;
   dstring ds;
   char buffer[128];
   dstring queue_name_buffer = DSTRING_INIT;
   lListElem *jobElem = NULL;
   lList *attributeList = NULL;
   
   DENTER(TOP_LAYER, "sge_job_to_XML");

   jobElem = lCreateElem(XMLE_Type);
   attributeList = lCreateList("attributes", XMLE_Type);
   lSetList(jobElem, XMLE_List, attributeList);
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   is_zombie_job = job_is_zombie_job(job);

   if (qep != NULL) {
      queue_name = qinstance_get_name(qep, &queue_name_buffer);
   } else {
      queue_name = NULL; 
   }

   sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   sge_ext = sgeee_mode && (full_listing & QSTAT_DISPLAY_EXTENDED);
   tsk_ext = (full_listing & QSTAT_DISPLAY_TASKS);
   sge_urg = sgeee_mode && (full_listing & QSTAT_DISPLAY_URGENCY);
   sge_pri = sgeee_mode && (full_listing & QSTAT_DISPLAY_PRIORITY);

   /* job number / ja task id */
   if (print_jobid){
      xml_append_Attr_I(attributeList, "JB_job_number", (int)lGetUlong(job, JB_job_number));            
   }

   /* per job priority information */
   if (sgeee_mode) {
      if (print_jobid)
         xml_append_Attr_D(attributeList, "JAT_prio", lGetDouble(jatep, JAT_prio));

      if (sge_ext) {
         if (print_jobid)
            xml_append_Attr_D(attributeList, "JAT_ntix", lGetDouble(jatep, JAT_ntix));
      }

      if (sge_urg) {
         if (print_jobid) {
            xml_append_Attr_D(attributeList, "JB_nurg", lGetDouble(job, JB_nurg));
            xml_append_Attr_D8(attributeList, "JB_urg", lGetDouble(job, JB_urg));
            xml_append_Attr_D8(attributeList, "JB_rrcontr", lGetDouble(job, JB_rrcontr));
            xml_append_Attr_D8(attributeList, "JB_wtcontr", lGetDouble(job, JB_wtcontr));
            xml_append_Attr_D8(attributeList, "JB_dlcontr", lGetDouble(job, JB_dlcontr));
         }
      } 

      if (sge_pri) {
         if (print_jobid) {
            xml_append_Attr_D(attributeList, "JB_nppri", lGetDouble(job, JB_nppri));
            xml_append_Attr_I(attributeList, "JB_priority", ((int)lGetUlong(job, JB_priority))-BASE_PRIORITY);
         }
      } 
   } 
   else {
      /* job priority */
      xml_append_Attr_I(attributeList, "JB_priority", ((int)lGetUlong(job, JB_priority))-BASE_PRIORITY);
   }

   if (print_jobid) {
      /* job name */
      xml_append_Attr_S(attributeList, "JB_name", lGetString(job, JB_job_name));

      /* job owner */
      xml_append_Attr_S(attributeList, "JB_owner", lGetString(job, JB_owner));
   }

   if (sge_ext) {
      if (print_jobid) {
         /* job project */
         xml_append_Attr_S(attributeList, "JB_project", lGetString(job, JB_project));
         /* job department */
         xml_append_Attr_S(attributeList, "JB_department", lGetString(job, JB_department));
      }
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

   if (print_jobid) {
      /* write states into string */ 
      job_get_state_string(state_string, jstate);
      xml_append_Attr_S(attributeList, "state", state_string);
   }

   if (!sge_ext) {
      if (print_jobid) {
         /* start/submit time */
         if (!lGetUlong(jatep, JAT_start_time) )
            xml_append_Attr_S(attributeList, "JB_submission_time", sge_ctime(lGetUlong(job, JB_submission_time), &ds));
         else {
#if 0
            /* AH: intermediate change to monitor JAT_stop_initiate_time 
             * must be removed before 6.0 if really needed a better possiblity 
             * for monitoring must be found (TODO)
             */
            if (getenv("JAT_stop_initiate_time") && (lGetUlong(jatep, JAT_state) & JDELETED))
               printf("%s!", sge_ctime(lGetUlong(jatep, JAT_stop_initiate_time), &ds));
            else
#endif
               xml_append_Attr_S(attributeList, "JAT_start_time", sge_ctime(lGetUlong(jatep, JAT_start_time), &ds));
         }
      }
   }

   /* is job logically running */
   running = lGetUlong(jatep, JAT_status)==JRUNNING || 
      lGetUlong(jatep, JAT_status)==JTRANSFERING;

   /* deadline time */
   if (sge_urg) {
      if (print_jobid) { 
         if (lGetUlong(job, JB_deadline) )
            xml_append_Attr_S(attributeList, "JB_deadline", sge_ctime(lGetUlong(job, JB_deadline), &ds));
      }
   }

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


      /* scaled cpu usage */
      if ((up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_CPU))) {
         int secs, minutes, hours, days;
         char xmlBuffer[128];

         secs = lGetDouble(up, UA_value);

         days    = secs/(60*60*24);
         secs   -= days*(60*60*24);

         hours   = secs/(60*60);
         secs   -= hours*(60*60);

         minutes = secs/60;
         secs   -= minutes*60;
      
         sprintf(xmlBuffer, "%d:%2.2d:%2.2d:%2.2d ", days, hours, minutes, secs); 
         xml_append_Attr_S(attributeList, "cpu-usage", xmlBuffer);
      } 
      /* scaled mem usage */
      if ((up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_MEM))) 
         xml_append_Attr_D(attributeList, "mem-usage", lGetDouble(up, UA_value));  
  
      /* scaled io usage */
      if ((up = lGetElemStr(job_usage_list, UA_name, USAGE_ATTR_IO))) 
         xml_append_Attr_D(attributeList, "io-usage", lGetDouble(up, UA_value));  

      lFreeList(job_usage_list);

      /* get tickets for job/slot */
      /* braces needed to suppress compiler warnings */
      if ((pe_name=lGetString(jatep, JAT_granted_pe)) &&
           (pe=pe_list_locate(pe_list, pe_name)) &&
           lGetBool(pe, PE_control_slaves)
         && slots && (gdil_ep=lGetSubStr(jatep, JG_qname, queue_name,
               JAT_granted_destin_identifier_list))) {
         if (slot == 0) {
            tickets = lGetDouble(gdil_ep, JG_ticket);
            otickets = lGetDouble(gdil_ep, JG_oticket);
            ftickets = lGetDouble(gdil_ep, JG_fticket);
            stickets = lGetDouble(gdil_ep, JG_sticket);
         }
         else {
            if (slots) {
               tickets = lGetDouble(gdil_ep, JG_ticket) / slots;
               otickets = lGetDouble(gdil_ep, JG_oticket) / slots;
               ftickets = lGetDouble(gdil_ep, JG_fticket) / slots;
               stickets = lGetDouble(gdil_ep, JG_sticket) / slots;
            } 
            else {
               tickets = otickets = ftickets = stickets = 0;
            }
         }
      }
      else {
         tickets = lGetDouble(jatep, JAT_tix);
         otickets = lGetDouble(jatep, JAT_oticket);
         ftickets = lGetDouble(jatep, JAT_fticket);
         stickets = lGetDouble(jatep, JAT_sticket);
      }

      /* report jobs dynamic scheduling attributes */
      /* only scheduled have these attribute */
      /* Pending jobs can also have tickets */
      if (!is_zombie_job) {
         if (sge_ext || lGetList(jatep, JAT_granted_destin_identifier_list)) {
            xml_append_Attr_I(attributeList, "tickets", (int)tickets);
            xml_append_Attr_I(attributeList, "JB_override_tickets", (int)lGetUlong(job, JB_override_tickets));
            xml_append_Attr_I(attributeList, "JB_jobshare", ((int)lGetUlong(job, JB_jobshare)));
            xml_append_Attr_I(attributeList, "otickets", (int)otickets);
            xml_append_Attr_I(attributeList, "ftickets", (int)ftickets);
            xml_append_Attr_I(attributeList, "stickets", (int)stickets);
            xml_append_Attr_D(attributeList, "JAT_share", lGetDouble(jatep, JAT_share));
         }
      }
   }

   /* if not full listing we need the queue's name in each line */
   if (!(full_listing & QSTAT_DISPLAY_FULL))
      xml_append_Attr_S(attributeList, "queue_name", queue_name);

   if ((group_opt & GROUP_NO_PETASK_GROUPS)) {
      /* MASTER/SLAVE information needed only to show parallel job distribution */
      xml_append_Attr_S(attributeList, "master", master);
   } else {
      /* job slots requested/granted */
      if (!slots_per_line)
         slots_per_line = sge_job_slot_request(job, pe_list);
      xml_append_Attr_I(attributeList, "slots", slots_per_line);
   }

   if (sge_dstring_get_string(dyn_task_str) && job_is_array(job))
      xml_append_Attr_S(attributeList, "tasks", sge_dstring_get_string(dyn_task_str));

   if (tsk_ext) {
      lList *task_list = lGetList(jatep, JAT_task_list);
      lListElem *task, *ep;
      lListElem *xmlElem;
      const char *qname;
      int subtask_ndx=1;
      
      /* print master sub-task belonging to this queue */
      if (!slot && task_list && queue_name &&
          ((ep=lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)))) &&
          ((qname=lGetString(ep, JG_qname))) &&
          !strcmp(qname, queue_name)) {
            
         xmlElem = xml_subtask(job, jatep, NULL, 0);
         lAppendElem(attributeList, xmlElem);
      }
         
      /* print sub-tasks belonging to this queue */
      for_each(task, task_list) {
         if (!slots || (queue_name && 
              ((ep=lFirst(lGetList(task, PET_granted_destin_identifier_list)))) &&
              ((qname=lGetString(ep, JG_qname))) &&
              !strcmp(qname, queue_name) && ((subtask_ndx++%slots)==slot))) {
               
            xmlElem = xml_subtask(job, jatep, task, 0);
            lAppendElem(attributeList, xmlElem);
         }
      }
   } 

   /* print additional job info if requested */
   if (print_jobid && (full_listing & QSTAT_DISPLAY_RESOURCES)) {
      lListElem *xmlElem;
         if (lGetString(job, JB_pe)) {
            dstring range_string = DSTRING_INIT;
            range_list_print_to_string(lGetList(job, JB_pe_range), 
                                       &range_string, 1);
            xmlElem = xml_append_Attr_S(attributeList, "requested_PE", sge_dstring_get_string(&range_string)); 
            xml_addAttribute(xmlElem, "name", lGetString(job, JB_pe));  

            sge_dstring_free(&range_string);
         }
         if (lGetString(jatep, JAT_granted_pe)) {
            lListElem *gdil_ep;
            u_long32 pe_slots = 0;
            for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list))
               pe_slots += lGetUlong(gdil_ep, JG_slots);
            xmlElem = xml_append_Attr_I(attributeList, "granted_PE", (int) pe_slots);       
            xml_addAttribute(xmlElem, "name", lGetString(jatep, JAT_granted_pe));
         }
         if (lGetString(job, JB_checkpoint_name)) { 
            xml_append_Attr_S(attributeList, "JB_checkpoint_name", lGetString(job, JB_checkpoint_name));
         }   

         ql = lGetList(job, JB_hard_resource_list);
         if (ql) {
            for_each(qrep, ql){
               xmlElem = xml_append_Attr_S(attributeList, "hard_request", lGetString(qrep, CE_stringval)); 
               xml_addAttribute(xmlElem, "name", lGetString(qrep, CE_name));
            }
         }
         
         /* display default requests if necessary */
         {
            lList *attributes = NULL;
            lListElem *ce;
            const char *name;
            lListElem *hep;

            queue_complexes2scheduler(&attributes, qep, exechost_list, centry_list);
            for_each (ce, attributes) {
               double dval;

               name = lGetString(ce, CE_name);
               if (!lGetBool(ce, CE_consumable) || !strcmp(name, "slots") || 
                   job_get_request(job, name))
                  continue;

               parse_ulong_val(&dval, NULL, lGetUlong(ce, CE_valtype), lGetString(ce, CE_default), NULL, 0); 
               if (dval == 0.0)
                  continue;

               /* For pending jobs (no queue/no exec host) we may print default request only
                  if the consumable is specified in the global host. For running we print it
                  if the resource is managed at this node/queue */
               if ((qep && lGetSubStr(qep, CE_name, name, QU_consumable_config_list)) ||
                   (qep && (hep=host_list_locate(exechost_list, lGetHost(qep, QU_qhostname))) &&
                    lGetSubStr(hep, CE_name, name, EH_consumable_config_list)) ||
                     ((hep=host_list_locate(exechost_list, SGE_GLOBAL_NAME)) &&
                         lGetSubStr(hep, CE_name, name, EH_consumable_config_list))){
                  xmlElem = xml_append_Attr_S(attributeList, "def_hard_request", lGetString(ce, CE_default)); 
                  xml_addAttribute(xmlElem, "name", name);
               }
            }
            lFreeList(attributes);
         }

         ql = lGetList(job, JB_soft_resource_list);
         if (ql) {
            for_each(qrep, ql){
               xmlElem = xml_append_Attr_S(attributeList, "soft_request", lGetString(qrep, CE_stringval));
               xml_addAttribute(xmlElem, "name", lGetString(qrep, CE_name));
            }
         }

         ql = lGetList(job, JB_hard_queue_list);
         if (ql) {
            for_each(qrep, ql) {
               xml_append_Attr_S(attributeList, "hard_req_queue", lGetString(qrep, QR_name));
            }
         }

         ql = lGetList(job, JB_soft_queue_list);
         if (ql) {
            for_each(qrep, ql) {
               xml_append_Attr_S(attributeList, "soft_req_queue", lGetString(qrep, QR_name));
            }
         }
         ql = lGetList(job, JB_master_hard_queue_list);
         if (ql){
            for_each(qrep, ql) {
               xml_append_Attr_S(attributeList, "master_hard_req_queue", lGetString(qrep, QR_name));
            }
         }
         ql = lGetList(job, JB_jid_request_list );
         if (ql) {
            for_each(qrep, ql) {
               xml_append_Attr_S(attributeList, "predecessor_jobs_req", lGetString(qrep, JRE_job_name));
            }
         }
         ql = lGetList(job, JB_jid_predecessor_list);
         if (ql) {
            for_each(qrep, ql) {
               xml_append_Attr_I(attributeList, "predecessor_jobs", (int) lGetUlong(qrep, JRE_job_number));
            }
         }
   }

   sge_dstring_free(&queue_name_buffer);

   DEXIT;
   return jobElem;
}

/*
 * pe_task : NULL, if master task shall be printed 
 */
static lListElem *xml_subtask(lListElem *job, lListElem *ja_task,
                             lListElem *pe_task,  int print_hdr) {
   lListElem *xmlElem = NULL;
   lList *attributeList = NULL;
   char task_state_string[8];
   u_long32 tstate, tstatus;
   int task_running;
   lListElem *ep;
   int sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   lList *usage_list;
   lList *scaled_usage_list;

   DENTER(TOP_LAYER, "xml_subtask");

   xmlElem = lCreateElem(XMLE_Type);
   attributeList = lCreateList("attributes", XMLE_Type);
   lSetList(xmlElem, XMLE_List, attributeList);
   
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

   if(pe_task) {
      xml_append_Attr_S(attributeList, "task-id", lGetString(pe_task, PET_id));      
   }

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
   xml_append_Attr_S(attributeList, "state", task_state_string);

   if (sgeee_mode) {
      lListElem *up;

      /* scaled cpu usage */
      if ((up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_CPU))) {
         dstring resource_string = DSTRING_INIT;

         double_print_time_to_dstring(lGetDouble(up, UA_value), 
                                      &resource_string);
         xml_append_Attr_S(attributeList, "cpu-usage", sge_dstring_get_string(&resource_string));
         sge_dstring_free(&resource_string);
      }

      /* scaled mem usage */
      if ((up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_MEM))) 
         xml_append_Attr_D(attributeList, "mem-usage", lGetDouble(up, UA_value));  

      /* scaled io usage */
      if ((up = lGetElemStr(scaled_usage_list, UA_name, USAGE_ATTR_IO))) 
         xml_append_Attr_D(attributeList, "io-usage", lGetDouble(up, UA_value));  
   }

   if (tstatus==JFINISHED) {
      ep=lGetElemStr(usage_list, UA_name, "exit_status");
      if (ep)
         xml_append_Attr_I(attributeList, "stat", (int)lGetDouble(ep, UA_value) );
   }

   DEXIT;
   return xmlElem;
}

/*-------------------------------------------------------------------------*/
/* print the finished jobs in case of SGE                                  */
/*-------------------------------------------------------------------------*/
static void xml_print_jobs_finished(lList *job_list, const lList *pe_list, const lList *user_list,
                             const lList *exechost_list, const lList *centry_list, 
                             u_long32 full_listing, u_long32 group_opt, lList **target_list) {
   int sge_ext;
   lListElem *jep, *jatep;
   dstring dyn_task_str = DSTRING_INIT;
   lListElem *elem;

   DENTER(TOP_LAYER, "xml_print_jobs_finished");

   if (*target_list == NULL){
      *target_list = lCreateList("Job-List", XMLE_Type);
   }

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
                  
                  sge_dstring_sprintf(&dyn_task_str, u32, 
                                    lGetUlong(jatep, JAT_task_number));

                  elem = sge_job_to_XML(jep, jatep, NULL, 1, NULL, &dyn_task_str, 
                                full_listing, 0, 0, exechost_list, centry_list, pe_list, group_opt, 0);  

                  if (elem) {
                     lList *attributes = NULL;
                     lListElem *pending = lCreateElem(XMLA_Type);
                     attributes = lGetList(elem, XMLE_Attribute);
                     
                     if (!attributes){
                        attributes = lCreateList("attributes", XMLA_Type);
                        lSetList(elem, XMLE_Attribute, attributes);
                     }
                     lSetString(pending, XMLA_Name, "state");
                     lSetString(pending, XMLA_Value, "finished");
                     lAppendElem(attributes, pending);

                     lAppendElem(*target_list, elem); 
                  }
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
static void xml_print_jobs_error( lList *job_list, const lList *pe_list, const lList *user_list,
                           const lList *exechost_list, const lList *centry_list,
                           u_long32 full_listing, u_long32 group_opt, lList **target_list) {
   lListElem *jep, *jatep;
   int sge_ext;
   dstring dyn_task_str = DSTRING_INIT;
   lListElem *elem;

   DENTER(TOP_LAYER, "xml_print_jobs_error");

   if (*target_list == NULL){
      *target_list = lCreateList("Job-List", XMLE_Type);
   }
   
   sge_ext = feature_is_enabled(FEATURE_SGEEE) 
              && (full_listing & QSTAT_DISPLAY_EXTENDED);

   for_each (jep, job_list) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         if (!(lGetUlong(jatep, JAT_suitable) & TAG_FOUND_IT) && lGetUlong(jatep, JAT_status) == JERROR) {
            lSetUlong(jatep, JAT_suitable, lGetUlong(jatep, JAT_suitable)|TAG_FOUND_IT);

            if (!lGetNumberOfElem(user_list) || (lGetNumberOfElem(user_list) && 
                  (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) {

               sge_dstring_sprintf(&dyn_task_str, "u32", lGetUlong(jatep, JAT_task_number));
               elem = sge_job_to_XML(jep, jatep, NULL, 1, NULL, &dyn_task_str, 
                                full_listing, 0, 0, exechost_list, centry_list, pe_list, group_opt, 0);  

               if (elem) {
                  lList *attributes = NULL;
                  lListElem *pending = lCreateElem(XMLA_Type);
                  attributes = lGetList(elem, XMLE_Attribute);
                  
                  if (!attributes){
                     attributes = lCreateList("attributes", XMLA_Type);
                     lSetList(elem, XMLE_Attribute, attributes);
                  }
                  lSetString(pending, XMLA_Name, "state");
                  lSetString(pending, XMLA_Value, "error");
                  lAppendElem(attributes, pending);

                  lAppendElem(*target_list, elem); 
               }
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
static void xml_print_jobs_zombie(lList *zombie_list, const lList *pe_list, const lList *user_list,
                           const lList *exechost_list, const lList *centry_list,
                           u_long32 full_listing, u_long32 group_opt, lList **target_list) {
   int sge_ext;
   lListElem *jep;
   dstring dyn_task_str = DSTRING_INIT; 
   lListElem *elem;
   DENTER(TOP_LAYER, "xml_print_jobs_zombie");

   if (! (full_listing & QSTAT_DISPLAY_ZOMBIES)) {
      DEXIT;
      return;
   }

   if (*target_list == NULL){
      *target_list = lCreateList("Job-List", XMLE_Type);
   }
   
   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
               (full_listing & QSTAT_DISPLAY_EXTENDED);

   for_each (jep, zombie_list) { 
      lList *z_ids = NULL;

      z_ids = lGetList(jep, JB_ja_z_ids);
      if (z_ids != NULL) {
         lListElem *ja_task = NULL;
         u_long32 first_task_id = range_list_get_first_id(z_ids, NULL);

         ja_task = (lListElem *) job_get_ja_task_template_pending(jep, first_task_id);
         range_list_print_to_string(z_ids, &dyn_task_str, 0);
         
         elem = sge_job_to_XML(jep, ja_task, NULL, 1, NULL, &dyn_task_str, 
                             full_listing, 0, 0, exechost_list, centry_list, pe_list, group_opt, 0);  

         if (elem) {
            lList *attributes = NULL;
            lListElem *pending = lCreateElem(XMLA_Type);
            attributes = lGetList(elem, XMLE_Attribute);
            
            if (!attributes){
               attributes = lCreateList("attributes", XMLA_Type);
               lSetList(elem, XMLE_Attribute, attributes);
            }
            lSetString(pending, XMLA_Name, "state");
            lSetString(pending, XMLA_Value, "finished");
            lAppendElem(attributes, pending);

            lAppendElem(*target_list, elem); 
         }
         
         sge_dstring_free(&dyn_task_str);
      }
   }
   DEXIT;
}


/*-------------------------------------------------------------------------*/
/* print jobs per queue                                                    */
/*-------------------------------------------------------------------------*/
void xml_print_jobs_queue(
lListElem *qep,
lList *job_list,
const lList *pe_list,
const lList *user_list,
const lList *ehl,
const lList *centry_list,
int print_jobs_of_queue,
u_long32 full_listing,
u_long32 group_opt,
lList **target_list
) {
   lListElem *jlep;
   lListElem *jatep;
   lListElem *gdilep;
   u_long32 job_tag;
   int sge_ext;
   u_long32 jid = 0, old_jid;
   u_long32 jataskid = 0, old_jataskid;
   dstring queue_name_buffer = DSTRING_INIT;
   const char *qnm;
   dstring dyn_task_str = DSTRING_INIT;

   DENTER(TOP_LAYER, "xml_print_jobs_queue");

   if (*target_list == NULL){
      *target_list = lCreateList("job-list", XMLE_Type);
   }

   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
               (full_listing & QSTAT_DISPLAY_EXTENDED);
   
   qnm = qinstance_get_name(qep, &queue_name_buffer);

   for_each(jlep, job_list) {
      int master, i;

      for_each(jatep, lGetList(jlep, JB_ja_tasks)) {
         if (shut_me_down) {
            SGE_EXIT(1);
         }
            
         for_each (gdilep, lGetList(jatep, JAT_granted_destin_identifier_list)) {

            if(!strcmp(lGetString(gdilep, JG_qname), qnm)) {
               int slot_adjust = 0;
               int lines_to_print;
               int slots_per_line, slots_in_queue = lGetUlong(gdilep, JG_slots); 

               if (qinstance_state_is_manual_suspended(qep) ||
                   qinstance_state_is_susp_on_sub(qep) ||
                   qinstance_state_is_cal_suspended(qep)) {
                  u_long32 jstate;

                  jstate = lGetUlong(jatep, JAT_state);
                  jstate &= ~JRUNNING;                 /* unset bit JRUNNING */
                  jstate |= JSUSPENDED_ON_SUBORDINATE; /* set bit JSUSPENDED_ON_SUBORDINATE */
                  lSetUlong(jatep, JAT_state, jstate);
               }
               job_tag = lGetUlong(jatep, JAT_suitable);
               job_tag |= TAG_FOUND_IT;
               lSetUlong(jatep, JAT_suitable, job_tag);

               master = !strcmp(qnm, 
                     lGetString(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), JG_qname));

               if (master) {
                  const char *pe_name;
                  lListElem *pe;
                  if (((pe_name=lGetString(jatep, JAT_granted_pe))) &&
                      ((pe=pe_list_locate(pe_list, pe_name))) &&
                      !lGetBool(pe, PE_job_is_first_task))

                      slot_adjust = 1;
               }

               /* job distribution view ? */
               if (!(group_opt & GROUP_NO_PETASK_GROUPS)) {
                  /* no - condensed ouput format */
                  if (!master && !(full_listing & QSTAT_DISPLAY_FULL)) {
                     /* skip all slave outputs except in full display mode */
                     continue;
                  }

                  /* print only on line per job for this queue */
                  lines_to_print = 1;

                  /* always only show the number of job slots represented by the line */
                  if ((full_listing & QSTAT_DISPLAY_FULL))
                     slots_per_line = slots_in_queue;
                  else
                     slots_per_line = sge_granted_slots(lGetList(jatep, JAT_granted_destin_identifier_list));
               } else {
                  /* yes */
                  lines_to_print = (int)slots_in_queue+slot_adjust;
                  slots_per_line = 1;
               }

               for (i=0; i<lines_to_print ;i++) {
                  int already_printed = 0;
                  lListElem *elem = NULL;

                  if (!lGetNumberOfElem(user_list) || 
                     (lGetNumberOfElem(user_list) && (lGetUlong(jatep, JAT_suitable)&TAG_SELECT_IT))) {
                     if (print_jobs_of_queue && (job_tag & TAG_SHOW_IT)) {
                        int different, print_jobid;

                        old_jid = jid;
                        jid = lGetUlong(jlep, JB_job_number);
                        old_jataskid = jataskid;
                        jataskid = lGetUlong(jatep, JAT_task_number);
                        sge_dstring_sprintf(&dyn_task_str, u32, jataskid);
                        different = (jid != old_jid) || (jataskid != old_jataskid);

                        if (different) 
                           print_jobid = 1;
                        else {
                           if (!(full_listing & QSTAT_DISPLAY_RUNNING))
                              print_jobid = master && (i==0);
                           else 
                              print_jobid = 0;
                        }

                        if (!already_printed && (full_listing & QSTAT_DISPLAY_RUNNING) &&
                              (lGetUlong(jatep, JAT_state) & JRUNNING)) {
                           elem = sge_job_to_XML(jlep, jatep, qep, print_jobid,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              slots_in_queue+slot_adjust, i, ehl, centry_list, pe_list, 
                              group_opt, slots_per_line); 
                           already_printed = 1;
                           
                        }
                        if (!already_printed && (full_listing & QSTAT_DISPLAY_SUSPENDED) &&
                           ((lGetUlong(jatep, JAT_state)&JSUSPENDED) ||
                           (lGetUlong(jatep, JAT_state)&JSUSPENDED_ON_THRESHOLD) ||
                            (lGetUlong(jatep, JAT_state)&JSUSPENDED_ON_SUBORDINATE))) {
                           
                           elem = sge_job_to_XML(jlep, jatep, qep, print_jobid,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              slots_in_queue+slot_adjust, i, ehl, centry_list, pe_list, 
                              group_opt, slots_per_line); 
                           already_printed = 1;  
                        }

                        if (!already_printed && (full_listing & QSTAT_DISPLAY_USERHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_USER)) {
                           
                           elem = sge_job_to_XML(jlep, jatep, qep, print_jobid,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              slots_in_queue+slot_adjust, i, ehl, centry_list, pe_list,
                              group_opt, slots_per_line);
                           already_printed = 1;
                        }

                        if (!already_printed && (full_listing & QSTAT_DISPLAY_OPERATORHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_OPERATOR))  {
                           
                           elem = sge_job_to_XML(jlep, jatep, qep, print_jobid,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              slots_in_queue+slot_adjust, i, ehl, centry_list, pe_list,
                              group_opt, slots_per_line);
                           already_printed = 1;
                        }
                            
                        if (!already_printed && (full_listing & QSTAT_DISPLAY_SYSTEMHOLD) &&
                            (lGetUlong(jatep, JAT_hold)&MINUS_H_TGT_SYSTEM)) {
                           
                           elem = sge_job_to_XML(jlep, jatep, qep, print_jobid,
                              (master && different && (i==0))?"MASTER":"SLAVE", &dyn_task_str, full_listing,
                              slots_in_queue+slot_adjust, i, ehl, centry_list, pe_list, 
                              group_opt, slots_per_line);
                           already_printed = 1;
                        }
                        
                        if (elem) {
                           lList *attributes = NULL;
                           lListElem *pending = lCreateElem(XMLA_Type);
                           attributes = lGetList(elem, XMLE_Attribute);
                           
                           if (!attributes){
                              attributes = lCreateList("attributes", XMLA_Type);
                              lSetList(elem, XMLE_Attribute, attributes);
                           }
                           lSetString(pending, XMLA_Name, "state");
                           lSetString(pending, XMLA_Value, "running");
                           lAppendElem(attributes, pending);

                           lAppendElem(*target_list, elem); 
                        }
                     }
                  }
               }
            }
         }
      }
   }
   sge_dstring_free(&queue_name_buffer);
   sge_dstring_free(&dyn_task_str);
   DEXIT;
}


lListElem *xml_print_queue(lListElem *q, const lList *exechost_list, const lList *centry_list,
                    u_long32 full_listing, const lList *qresource_list, u_long32 explain_bits) {
   char arch_string[80];
   double load_avg;
   int sge_ext;
   char *load_avg_str;
   char load_alarm_reason[MAX_STRING_SIZE];
   char suspend_alarm_reason[MAX_STRING_SIZE];
   dstring queue_name_buffer = DSTRING_INIT;
   const char *queue_name = NULL;
   bool is_load_value;
   bool has_value_from_object; 
   lListElem *jobElem = NULL;
   lList *attributeList = NULL;
   
   DENTER(TOP_LAYER, "xml_print_queue");

   *load_alarm_reason = 0;
   *suspend_alarm_reason = 0;
   queue_name = qinstance_get_name(q, &queue_name_buffer);

   /* make it possible to display any load value in qstat output */
   if (!(load_avg_str=getenv("SGE_LOAD_AVG")) || !strlen(load_avg_str))
      load_avg_str = LOAD_ATTR_LOAD_AVG;

   if (!(full_listing & QSTAT_DISPLAY_FULL)) {
      DEXIT;
      return jobElem;
   }

   {
      lListElem *temp = NULL;
      temp = lCreateElem(XMLE_Type);
      lSetBool(temp, XMLE_Print, false);
      jobElem = lCreateElem(XMLE_Type);
      attributeList = lCreateList("attributes", XMLE_Type);
      lSetList(temp, XMLE_List, attributeList);
      lSetObject(jobElem, XMLE_Element, temp);
   }
   
   /* compute the load and check for alarm states */

   is_load_value = sge_get_double_qattr(&load_avg, load_avg_str, q, exechost_list, centry_list, &has_value_from_object);
   if (sge_load_alarm(NULL, q, lGetList(q, QU_load_thresholds), exechost_list, centry_list, NULL)) {
      qinstance_state_set_alarm(q, true);
      sge_load_alarm_reason(q, lGetList(q, QU_load_thresholds), exechost_list, 
                            centry_list, load_alarm_reason, 
                            MAX_STRING_SIZE - 1, "load");
   }
   if (sge_load_alarm(NULL, q, lGetList(q, QU_suspend_thresholds), exechost_list, centry_list, NULL)) {
      qinstance_state_set_suspend_alarm(q, true);
      sge_load_alarm_reason(q, lGetList(q, QU_suspend_thresholds), 
                            exechost_list, centry_list, suspend_alarm_reason, 
                            MAX_STRING_SIZE - 1, "suspend");
   }

   sge_ext = feature_is_enabled(FEATURE_SGEEE) && 
             (full_listing & QSTAT_DISPLAY_EXTENDED);

   
   xml_append_Attr_S(attributeList, "name", queue_name);        

   {
      dstring type_string = DSTRING_INIT;

      qinstance_print_qtype_to_dstring(q, &type_string, true);
      xml_append_Attr_S(attributeList, "qtype", sge_dstring_get_string(&type_string)); 
      sge_dstring_free(&type_string);
   }

   /* number of used/free slots */
   xml_append_Attr_I(attributeList, "slots-used", qinstance_slots_used(q) ); 
   xml_append_Attr_I(attributeList, "slots-total", (int)lGetUlong(q, QU_job_slots));

   /* load avg */
   if (!is_load_value) {
      if (has_value_from_object) {
         xml_append_Attr_D(attributeList, "load_avg", load_avg);
      } 
   }
   
   /* arch */
   if (!sge_get_string_qattr(arch_string, sizeof(arch_string)-1, LOAD_ATTR_ARCH, 
         q, exechost_list, centry_list))
      xml_append_Attr_S(attributeList, "arch", arch_string);

   {
      dstring state_string = DSTRING_INIT;
      qinstance_state_append_to_dstring(q, &state_string);
      xml_append_Attr_S(attributeList, "state", sge_dstring_get_string(&state_string));
      sge_dstring_free(&state_string);
   }

   if((full_listing & QSTAT_DISPLAY_ALARMREASON)) {
      if(*load_alarm_reason) {
         xml_append_Attr_S(attributeList, "load-alarm-reason", load_alarm_reason);
      }
      if(*suspend_alarm_reason) {
         xml_append_Attr_S(attributeList, "suspend-alarm-reason", suspend_alarm_reason);
      }
   }

   if ((explain_bits & QI_ALARM) > 0) {
      if(*load_alarm_reason) {
         xml_append_Attr_S(attributeList, "load-alarm-reason", load_alarm_reason);
      }
   }
   if ((explain_bits & QI_SUSPEND_ALARM) > 0) {
      if(*suspend_alarm_reason) {
         xml_append_Attr_S(attributeList, "suspend-alarm-reason", suspend_alarm_reason);
      }
   }
   
   if (explain_bits != QI_DEFAULT) {
      lList *qim_list = lGetList(q, QU_message_list);
      lListElem *qim = NULL;

      for_each(qim, qim_list) {
         u_long32 type = lGetUlong(qim, QIM_type);

         if ((explain_bits & QI_AMBIGUOUS) == type || 
             (explain_bits & QI_ERROR) == type) {
            const char *message = lGetString(qim, QIM_message);
            xml_append_Attr_S(attributeList, "message", message);
         }
      }
   }

   /* view (selected) resources of queue in case of -F [attr,attr,..] */ 
   if ((full_listing & QSTAT_DISPLAY_QRESOURCES)) {
      dstring resource_string = DSTRING_INIT;
      lList *rlp;
      lListElem *rep;
      lListElem *xmlElem;
      char dom[5];
      const char *s;
      u_long32 dominant;

      rlp = NULL;

      queue_complexes2scheduler(&rlp, q, exechost_list, centry_list);
      for_each (rep , rlp) {

         /* we had a -F request */
         if (qresource_list) {
            lListElem *qres;
            qres = lGetElemStr(qresource_list, CE_name, 
                               lGetString(rep, CE_name));
            /* if this complex variable wasn't requested with -F, skip it */
            if (qres == NULL)
               continue ;
         }
         sge_dstring_clear(&resource_string);

         { 
            u_long32 type = lGetUlong(rep, CE_valtype);
            switch (type) {
            case TYPE_HOST:   
            case TYPE_STR:   
            case TYPE_CSTR:  
            case TYPE_RESTR:
               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  dominant = lGetUlong(rep, CE_pj_dominant);
                  s = lGetString(rep, CE_pj_stringval);
               } else {
                  dominant = lGetUlong(rep, CE_dominant);
                  s = lGetString(rep, CE_stringval);
               }
               break;
            case TYPE_TIM: 

               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  double val = lGetDouble(rep, CE_pj_doubleval);

                  dominant = lGetUlong(rep, CE_pj_dominant);
                  double_print_time_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               } else {
                  double val = lGetDouble(rep, CE_doubleval);

                  dominant = lGetUlong(rep, CE_dominant);
                  double_print_time_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               }
               break;
            case TYPE_MEM:

               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  double val = lGetDouble(rep, CE_pj_doubleval);

                  dominant = lGetUlong(rep, CE_pj_dominant);
                  double_print_memory_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               } else {
                  double val = lGetDouble(rep, CE_doubleval);

                  dominant = lGetUlong(rep, CE_dominant);
                  double_print_memory_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               }
               break;
            default:   

               if (!(lGetUlong(rep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
                  double val = lGetDouble(rep, CE_pj_doubleval);

                  dominant = lGetUlong(rep, CE_pj_dominant);
                  double_print_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               } else {
                  double val = lGetDouble(rep, CE_doubleval);

                  dominant = lGetUlong(rep, CE_dominant);
                  double_print_to_dstring(val, &resource_string);
                  s = sge_dstring_get_string(&resource_string);
               }
               break;
            }
         }
         monitor_dominance(dom, dominant); 
         
         xmlElem = xml_append_Attr_S(attributeList, "resource", s);
         xml_addAttribute(xmlElem, "name", lGetString(rep, CE_name));  
         xml_addAttribute(xmlElem, "type", dom);
         
      }

      lFreeList(rlp);
      sge_dstring_free(&resource_string);

   }

   DEXIT;
   return jobElem;
}
