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

extern HashTable Master_Job_Hash_Table;

static int job_initialize_task(lListElem *job, u_long32 task_id,
                               u_long32 hold_state);

int job_initialize_ja_tasks(lListElem *job)
{
   lListElem *range_elem = NULL; /* RN_Type */
   lList *range_list[8];         /* RN_Type */
   u_long32 hold_state[8];
   u_long32 task_number, job_number;
   int inserted = 0;
   int i;
 
   DENTER(TOP_LAYER, "job_initialize_ja_tasks");
   job_number = lGetUlong(job, JB_job_number);
   lSetList(job, JB_ja_tasks, lCreateList("ja tasks", JAT_Type));
   hold_state[0] = 0;
   hold_state[1] = MINUS_H_TGT_USER;
   hold_state[2] = MINUS_H_TGT_OPERATOR;
   hold_state[3] = MINUS_H_TGT_SYSTEM;
   hold_state[4] = MINUS_H_TGT_USER | MINUS_H_TGT_OPERATOR;
   hold_state[5] = MINUS_H_TGT_OPERATOR | MINUS_H_TGT_SYSTEM;
   hold_state[6] = MINUS_H_TGT_USER | MINUS_H_TGT_SYSTEM;
   hold_state[7] = MINUS_H_TGT_USER | MINUS_H_TGT_OPERATOR |
                   MINUS_H_TGT_SYSTEM;              

   range_list[0] = lGetList(job, JB_ja_n_h_ids);
   range_list[1] = lGetList(job, JB_ja_u_h_ids);
   range_list[2] = lGetList(job, JB_ja_o_h_ids);
   range_list[3] = lGetList(job, JB_ja_s_h_ids);
   range_list[4] = range_list[5] = range_list[6] = range_list[7] = NULL;
   range_calculate_intersection_set(range_list[1], range_list[2],
                                    &range_list[4]);
   range_calculate_intersection_set(range_list[2], range_list[3],
                                    &range_list[5]);
   range_calculate_intersection_set(range_list[1], range_list[3],
                                    &range_list[6]);
   range_calculate_intersection_set(range_list[4], range_list[5],
                                    &range_list[7]);
 
   for (i = 0; i <= 7; i++) {
      for_each_id_in_range_list(task_number, range_elem, range_list[i]) {
         int ret;
 
         ret = job_initialize_task(job, task_number, hold_state[i]);
         if (ret != STATUS_OK) {
            DEXIT;
            return ret;
         }
         inserted = 1;
      }
   }
#if 0 /* EB: */
   if (!inserted) {
      ERROR((SGE_EVENT, "job "u32" was rejected because it containes"
             " no range information", u32c(job_number)));
      DEXIT;
      return STATUS_EUNKNOWN;
   }
#endif
   DEXIT;
   return STATUS_OK;
}              

static int job_initialize_task(lListElem *job, u_long32 task_id,
                               u_long32 hold_state)
{
   lList *task_list = NULL;      /* JAT_Type */
   lListElem *new_task = NULL;   /* JAT_Type */
   u_long32 state, job_number;
 
   DENTER(BASIS_LAYER, "job_initialize_task");
   job_number = lGetUlong(job, JB_job_number);
   task_list = lGetList(job, JB_ja_tasks);
   new_task = lCreateElem(JAT_Type);
   if (!new_task) {
      ERROR((SGE_EVENT, "job "u32" was rejected because it was not "
             "possible to create task list\n", job_number));
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   lSetUlong(new_task, JAT_task_number, task_id);
 
   lSetUlong(new_task, JAT_hold, hold_state);
   lSetUlong(new_task, JAT_status, JIDLE);
   state = JQUEUED | JWAITING;
   if (lGetUlong(new_task, JAT_hold)) {
      state |= JHELD;
   }
   lSetUlong(new_task, JAT_state, state);
   lAppendElem(task_list, new_task);
   DEXIT;
   return STATUS_OK;
}                         

int job_is_enrolled(lListElem *job, u_long32 task_number)
{
   int ret = 1;
 
   if (range_is_within(lGetList(job, JB_ja_n_h_ids), task_number) ||
       range_is_within(lGetList(job, JB_ja_u_h_ids), task_number) ||
       range_is_within(lGetList(job, JB_ja_o_h_ids), task_number) ||
       range_is_within(lGetList(job, JB_ja_s_h_ids), task_number)) {
      ret = 0;
   }
   return ret;
}
 
void job_enroll(lListElem *job, u_long32 task_number)
{
   range_remove_id(lGetList(job, JB_ja_n_h_ids), task_number);
   range_remove_id(lGetList(job, JB_ja_u_h_ids), task_number);
   range_remove_id(lGetList(job, JB_ja_o_h_ids), task_number);
   range_remove_id(lGetList(job, JB_ja_s_h_ids), task_number);
   range_compress(lGetList(job, JB_ja_n_h_ids));
   range_compress(lGetList(job, JB_ja_u_h_ids));
   range_compress(lGetList(job, JB_ja_o_h_ids));
   range_compress(lGetList(job, JB_ja_s_h_ids));
}       

void job_set_hold_state(lListElem *job, lListElem *ja_task,
                        u_long32 new_hold_state)
{
   u_long32 ja_task_id;

   ja_task_id = lGetUlong(ja_task, JAT_task_number);
   if (!job_is_enrolled(job, ja_task_id)) {
      if (new_hold_state & MINUS_H_TGT_ALL) {
         range_remove_id(lGetList(job, JB_ja_n_h_ids), ja_task_id);
      } else {
         range_insert_id(lGetList(job, JB_ja_n_h_ids), ja_task_id);
      }
      range_compress(lGetList(job, JB_ja_n_h_ids));
      if (new_hold_state & MINUS_H_TGT_USER) {
         range_insert_id(lGetList(job, JB_ja_u_h_ids), ja_task_id);
      } else {
         range_remove_id(lGetList(job, JB_ja_u_h_ids), ja_task_id);
      }
      range_compress(lGetList(job, JB_ja_u_h_ids));
      if (new_hold_state & MINUS_H_TGT_OPERATOR) {
         range_insert_id(lGetList(job, JB_ja_o_h_ids), ja_task_id);
      } else {
         range_remove_id(lGetList(job, JB_ja_o_h_ids), ja_task_id);
      }
      range_compress(lGetList(job, JB_ja_o_h_ids));
      if (new_hold_state & MINUS_H_TGT_SYSTEM) {
         range_insert_id(lGetList(job, JB_ja_s_h_ids), ja_task_id);
      } else {
         range_remove_id(lGetList(job, JB_ja_s_h_ids), ja_task_id);
      }
      range_compress(lGetList(job, JB_ja_s_h_ids));
   }
   lSetUlong(ja_task, JAT_hold, new_hold_state); 
}

int job_list_add_job(lList **job_list, const char *name, lListElem *job, 
                     int check, int hash, HashTable* Job_Hash_Table) {
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

   if (hash && Job_Hash_Table) {
      u_long32 temp = lGetUlong(job, JB_job_number);
      if (!*Job_Hash_Table)
         *Job_Hash_Table = HashTableCreate(14, DupFunc_u_long32, HashFunc_u_long32, 
                                 HashCompare_u_long32); /* 2^14 entries */

      HashTableStore(*Job_Hash_Table,
                     &temp,
                     (void *) job);
   }
   DEXIT;
   return 0;
}               
