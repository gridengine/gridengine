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

#include "sgermon.h"
#include "sge_log.c"
#include "def.h"   
#include "cull_list.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_answerL.h"
#include "sge_job_jatask.h"
#include "sge_range.h"
#include "msg_gdilib.h"
#include "sge_hash.h"
#include "job.h"
#include "read_write_job.h"

extern HashTable Master_Job_Hash_Table;

lListElem *job_get_ja_task_template_pending(const lListElem *job,
                                            u_long32 ja_task_id)
{
   lListElem *template_task = NULL;   /* JAT_Type */

   DENTER(BASIS_LAYER, "job_get_ja_task_template");
   template_task = lFirst(lGetList(job, JB_ja_template));
   if (!template_task) {
      ERROR((SGE_EVENT, "unable to create template task\n"));
   } 
   if (template_task) {
      lSetUlong(template_task, JAT_state, JQUEUED | JWAITING);
   }
   if (template_task) {
      lSetUlong(template_task, JAT_task_number, ja_task_id);  
   }
   DEXIT;
   return template_task;
}

lListElem *job_get_ja_task_template_hold(const lListElem *job,
                                         u_long32 ja_task_id, 
                                         u_long32 hold_state)
{
   lListElem *template_task = job_get_ja_task_template_pending(job, ja_task_id);
 
   DENTER(BASIS_LAYER, "job_get_ja_task_template");
   if (template_task) {
      u_long32 state;
 
      lSetUlong(template_task, JAT_task_number, ja_task_id);
      lSetUlong(template_task, JAT_hold, hold_state);
      lSetUlong(template_task, JAT_status, JIDLE);
      state = JQUEUED | JWAITING;
      if (lGetUlong(template_task, JAT_hold)) {
         state |= JHELD;
      }
      lSetUlong(template_task, JAT_state, state);
   }
   DEXIT;
   return template_task;                                                        }

lListElem *job_get_ja_task_template(const lListElem *job,
                                    u_long32 ja_task_id)
{
   u_long32 hold_state = job_get_ja_task_hold_state(job, ja_task_id);

   return job_get_ja_task_template_hold(job, ja_task_id, hold_state);
}

u_long32 job_get_ja_task_hold_state(const lListElem *job,
                                     u_long32 ja_task_id)
{
   u_long32 ret = 0;

   if (range_list_is_id_within(lGetList(job, JB_ja_u_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_USER;
   }
   if (range_list_is_id_within(lGetList(job, JB_ja_o_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_OPERATOR;
   }
   if (range_list_is_id_within(lGetList(job, JB_ja_s_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_SYSTEM;
   }
   return ret;
}

void job_create_hold_id_lists(const lListElem *job, lList *id_list[8], 
                              u_long32 hold_state[8]) 
{
   int i;
   lList *list[7];

   hold_state[0] = 0;
   hold_state[1] = MINUS_H_TGT_USER;
   hold_state[2] = MINUS_H_TGT_OPERATOR;
   hold_state[3] = MINUS_H_TGT_SYSTEM;
   hold_state[4] = MINUS_H_TGT_USER | MINUS_H_TGT_OPERATOR;
   hold_state[5] = MINUS_H_TGT_USER | MINUS_H_TGT_SYSTEM;
   hold_state[6] = MINUS_H_TGT_OPERATOR | MINUS_H_TGT_SYSTEM;
   hold_state[7] = MINUS_H_TGT_USER | MINUS_H_TGT_OPERATOR |
                   MINUS_H_TGT_SYSTEM;

   for (i = 0; i < 7; i++) {
      list[i] = NULL;
   }
   for (i = 0; i < 8; i++) {
      id_list[i] = NULL;
   }

   /* uo */
   range_calculate_intersection_set(&list[0], NULL, 
                  lGetList(job, JB_ja_u_h_ids), lGetList(job, JB_ja_o_h_ids));
   /* us */
   range_calculate_intersection_set(&list[1], NULL, 
                  lGetList(job, JB_ja_u_h_ids), lGetList(job, JB_ja_s_h_ids));
   /* os */
   range_calculate_intersection_set(&list[2], NULL, 
                  lGetList(job, JB_ja_o_h_ids), lGetList(job, JB_ja_s_h_ids));

   /* uos -> 7 */
   range_calculate_intersection_set(&id_list[7], NULL, list[2], list[1]);

   /* osU -> 6 */
   range_calculate_difference_set(&id_list[6], NULL, list[2], id_list[7]);
   /* usO -> 5 */
   range_calculate_difference_set(&id_list[5], NULL, list[1], id_list[7]);
   /* uoS -> 4 */
   range_calculate_difference_set(&id_list[4], NULL, list[0], id_list[7]);

   /* sOU -> 3 */
   range_calculate_difference_set(&list[6], NULL, 
                  lGetList(job, JB_ja_s_h_ids), list[1]);
   range_calculate_difference_set(&id_list[3], NULL, list[6], id_list[6]);       

   /* oUS -> 2 */
   range_calculate_difference_set(&list[5], NULL, 
                  lGetList(job, JB_ja_o_h_ids), list[0]);
   range_calculate_difference_set(&id_list[2], NULL, list[5], id_list[6]);
   
   /* uOS -> 1 */ 
   range_calculate_difference_set(&list[4], NULL, 
                  lGetList(job, JB_ja_u_h_ids), list[1]);
   range_calculate_difference_set(&id_list[1], NULL, list[4], id_list[4]);
   
   /* UOS -> 0 */
   id_list[0] = lCopyList("", lGetList(job, JB_ja_n_h_ids));

   for (i = 0; i < 7; i++) {
      list[i] = lFreeList(list[i]);
   }
}

void job_destroy_hold_id_lists(const lListElem *job, lList *id_list[8]) 
{
   int i;

   for (i = 0; i < 8; i++) {
      id_list[i] = lFreeList(id_list[i]);
   }
}

int job_is_enrolled(const lListElem *job, u_long32 task_number)
{
   int ret = 1;

   DENTER(TOP_LAYER, "job_is_enrolled");
   if (range_list_is_id_within(lGetList(job, JB_ja_n_h_ids), task_number) ||
       range_list_is_id_within(lGetList(job, JB_ja_u_h_ids), task_number) ||
       range_list_is_id_within(lGetList(job, JB_ja_o_h_ids), task_number) ||
       range_list_is_id_within(lGetList(job, JB_ja_s_h_ids), task_number)) {
      ret = 0;
   }
   DEXIT;
   return ret;
}

int job_is_ja_task_defined(const lListElem *job, u_long32 ja_task_number) 
{
   lList *range_list = lGetList(job, JB_ja_structure);
   int ret = range_list_is_id_within(range_list, ja_task_number);

   DENTER(BASIS_LAYER, "job_is_ja_task_defined");
   DPRINTF(("Task "u32" is "SFQ"\n", ja_task_number, 
            ret ? "defined" : "undefined"));
   DEXIT;
   return ret;
}

u_long32 job_get_ja_tasks(const lListElem *job) 
{  
   u_long32 ret = 0;

   ret += job_get_not_enrolled_ja_tasks(job);
   ret += job_get_enrolled_ja_tasks(job);
   return ret;
}

u_long32 job_get_not_enrolled_ja_tasks(const lListElem *job) 
{
   const int attributes = 4;
   const int attribute[] = {JB_ja_n_h_ids, JB_ja_u_h_ids, JB_ja_o_h_ids,
                            JB_ja_s_h_ids};
   u_long32 ret = 0;
   int i;
     
   for (i = 0; i < attributes; i++) {
      ret += range_list_get_number_of_ids(lGetList(job, attribute[i]));
   } 
   return ret;
}

u_long32 job_get_enrolled_ja_tasks(const lListElem *job) 
{
   return lGetNumberOfElem(lGetList(job, JB_ja_tasks));
}
 
void job_enroll(lListElem *job, lList **answer_list, u_long32 ja_task_number)
{
   lList *range_list = NULL;
   lListElem *ja_task = NULL;

   lXchgList(job, JB_ja_n_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_n_h_ids, &range_list);
   lXchgList(job, JB_ja_u_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_u_h_ids, &range_list);
   lXchgList(job, JB_ja_o_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_o_h_ids, &range_list);
   lXchgList(job, JB_ja_s_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_s_h_ids, &range_list);
   range_compress(lGetList(job, JB_ja_n_h_ids));
   range_compress(lGetList(job, JB_ja_u_h_ids));
   range_compress(lGetList(job, JB_ja_o_h_ids));
   range_compress(lGetList(job, JB_ja_s_h_ids));

   /* EB: should we add a new CULL function? */
   ja_task = lGetSubUlong(job, JAT_task_number, ja_task_number, JB_ja_tasks);
   if (ja_task == NULL) {
      lList *ja_task_list = lGetList(job, JB_ja_tasks);

      if (ja_task_list == NULL) {
         ja_task_list = lCreateList("", JAT_Type);
         lSetList(job, JB_ja_tasks, ja_task_list);
      } 
      ja_task = lCopyElem(job_get_ja_task_template_pending(job, ja_task_number));
      lAppendElem(ja_task_list, ja_task); 
   }
}  

int job_has_tasks(lListElem *job) 
{
   int ret = 0;

   if (job != NULL) {
      if (lGetList(job, JB_ja_n_h_ids) != NULL || 
          lGetList(job, JB_ja_u_h_ids) != NULL ||
          lGetList(job, JB_ja_o_h_ids) != NULL ||
          lGetList(job, JB_ja_s_h_ids) != NULL) {
         ret = 1;
      }
   }
   return ret;
}

void job_delete_not_enrolled_ja_task(lListElem *job, lList **answer_list, 
                                     u_long32 ja_task_number) 
{
   const int attributes = 4;
   const int attribute[] = {JB_ja_n_h_ids, JB_ja_u_h_ids, JB_ja_o_h_ids,
                            JB_ja_s_h_ids};
   int i;

   for (i = 0; i < attributes; i++) { 
      lList *range_list = NULL;

      lXchgList(job, attribute[i], &range_list);
      range_list_remove_id(&range_list, answer_list, ja_task_number);
      lXchgList(job, attribute[i], &range_list);
      range_compress(range_list);
   }
} 

void job_add_as_zombie(lListElem *zombie, const lListElem *job, 
                                lList **answer_list, u_long32 ja_task_id) 
{
   lList *z_ids = NULL;    /* RN_Type */
 
   lXchgList(zombie, JB_ja_z_ids, &z_ids);
   range_list_insert_id(&z_ids, NULL, ja_task_id);
   range_compress(z_ids);
   lXchgList(zombie, JB_ja_z_ids, &z_ids);    
}

int job_has_job_pending_tasks(lListElem *job) 
{
   int ret = 0;

   if (lGetList(job, JB_ja_n_h_ids) || lGetList(job, JB_ja_u_h_ids) ||
       lGetList(job, JB_ja_o_h_ids) || lGetList(job, JB_ja_s_h_ids)) {
      ret = 1;
   }
   return ret;
}

void job_set_hold_state(lListElem *job, lList **answer_list, 
                        u_long32 ja_task_id,
                        u_long32 new_hold_state)
{
   DENTER(TOP_LAYER, "job_set_hold_state");
   if (!job_is_enrolled(job, ja_task_id)) {
      const int lists = 4;
      const u_long32 mask[] = {MINUS_H_TGT_ALL, MINUS_H_TGT_USER, 
                               MINUS_H_TGT_OPERATOR, MINUS_H_TGT_SYSTEM};
      const int attribute[] = {JB_ja_n_h_ids, JB_ja_u_h_ids,
                               JB_ja_o_h_ids, JB_ja_s_h_ids}; 
      const range_remove_insert_t if_function[] = {range_list_remove_id,
                                                   range_list_insert_id,
                                                   range_list_insert_id,
                                                   range_list_insert_id}; 
      const range_remove_insert_t else_function[] = {range_list_insert_id,
                                                     range_list_remove_id,
                                                     range_list_remove_id,
                                                     range_list_remove_id};
      int i;

      for (i = 0; i < lists; i++) {
         lList *range_list = NULL;

         if (new_hold_state & mask[i]) {
            lXchgList(job, attribute[i], &range_list);
            if_function[i](&range_list, answer_list, ja_task_id);
            lXchgList(job, attribute[i], &range_list); 
         } else {
            lXchgList(job, attribute[i], &range_list);
            else_function[i](&range_list, answer_list, ja_task_id);
            lXchgList(job, attribute[i], &range_list);
         }
         range_compress(lGetList(job, attribute[i]));
      }
   } else {
      lListElem *ja_task = job_search_task(job, NULL, ja_task_id, 0);

      lSetUlong(ja_task, JAT_hold, new_hold_state); 
      if (new_hold_state) {
         lSetUlong(ja_task, JAT_state, lGetUlong(ja_task, JAT_state) | JHELD);
      } else {
         lSetUlong(ja_task, JAT_state, lGetUlong(ja_task, JAT_state) & ~JHELD);
      }
   }
   DEXIT;
}

u_long32 job_get_hold_state(lListElem *job, u_long32 ja_task_id)
{
   u_long32 ret = 0;

   if (job_is_enrolled(job, ja_task_id)) {
      lListElem *ja_task = job_search_task(job, NULL, ja_task_id, 0);
   
      ret = lGetUlong(ja_task, JAT_hold) & MINUS_H_TGT_ALL;
   } else {
      int attribute[3] = {JB_ja_u_h_ids, JB_ja_o_h_ids, JB_ja_s_h_ids};
      u_long32 hold_flag[3] = {MINUS_H_TGT_USER, MINUS_H_TGT_OPERATOR,
                               MINUS_H_TGT_SYSTEM};
      int i;

      for (i = 0; i < 3; i++) {
         lList *hold_list = lGetList(job, attribute[i]);

         if (range_list_is_id_within(hold_list, ja_task_id)) {
            ret |= hold_flag[i];
         }
      }
   }
   return ret;
}

lListElem *job_search_task(lListElem *job, lList **answer_list,
                           u_long32 ja_task_id, int enroll_if_not_existing) 
{
   lListElem *ja_task = NULL; 

   DENTER(TOP_LAYER, "job_search_task");
   if (job != NULL) {
      ja_task = lGetSubUlong(job, JAT_task_number, ja_task_id, JB_ja_tasks);
      if (ja_task == NULL && enroll_if_not_existing &&
          job_is_ja_task_defined(job, ja_task_id)) {
         job_enroll(job, answer_list, ja_task_id);
         ja_task = lGetSubUlong(job, JAT_task_number, ja_task_id, JB_ja_tasks);
      } 
   }
   DEXIT;
   return ja_task;
}
                            
int job_list_add_job(lList **job_list, const char *name, lListElem *job, 
                     int check) {
   DENTER(TOP_LAYER, "job_list_add_job");

   if (!job_list) {
      ERROR((SGE_EVENT, MSG_JOB_JLPPNULL));
      DEXIT;
      return 1;
   }
   if (!job) {
      ERROR((SGE_EVENT, MSG_JOB_JEPNULL));
      DEXIT;
      return 1;
   }

   if(!*job_list) {
      *job_list = lCreateList(name, JB_Type);
   }

   if (check && *job_list &&
       lGetElemUlong(*job_list, JB_job_number, lGetUlong(job, JB_job_number))) {
      ERROR((SGE_EVENT, MSG_JOB_JOBALREADYEXISTS_U, 
             u32c(lGetUlong(job, JB_job_number))));
      DEXIT;
      return -1;
   }

   lAppendElem(*job_list, job);

   DEXIT;
   return 0;
}     

void job_list_print(lList *job_list)
{
   lListElem *job = NULL;

   for_each(job, job_list) {
      StringBufferT string = {NULL, 0};

      range_print_to_string(lGetList(job, JB_ja_n_h_ids), &string);
      if(string.s) {
         fprintf(stderr, "job: "u32" n_h_list: %s\n", 
                 lGetUlong(job, JB_job_number), string.s);
      }
      sge_string_free(&string);
      range_print_to_string(lGetList(job, JB_ja_u_h_ids), &string);
      if(string.s) {
         fprintf(stderr, "job: "u32" u_h_list: %s\n", 
                 lGetUlong(job, JB_job_number), string.s);
      }
      sge_string_free(&string);
      range_print_to_string(lGetList(job, JB_ja_o_h_ids), &string);
      if(string.s) {
         fprintf(stderr, "job: "u32" o_h_list: %s\n", 
                 lGetUlong(job, JB_job_number), string.s);
      }
      sge_string_free(&string);
      range_print_to_string(lGetList(job, JB_ja_s_h_ids), &string);
      if(string.s) {
         fprintf(stderr, "job: "u32" s_h_list: %s\n", 
                 lGetUlong(job, JB_job_number), string.s);
      }
   }
}          
