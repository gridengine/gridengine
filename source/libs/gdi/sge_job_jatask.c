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
#include "sge_queueL.h"
#include "sge_jataskL.h"
#include "sge_answerL.h"
#include "sge_job_jatask.h"
#include "sge_range.h"
#include "msg_gdilib.h"
#include "sge_hash.h"
#include "job.h"
#include "read_write_job.h"

/****** gdi/job_jatask/job_get_ja_task_template_pending() **********************
*  NAME
*     job_get_ja_task_template_pending() -- create a ja task template 
*
*  SYNOPSIS
*     lListElem* job_get_ja_task_template_pending(const lListElem *job, 
*                                                 u_long32 ja_task_id) 
*
*  FUNCTION
*     The function returns a pointer to a template array task element.
*     This task represents a currently submitted pending task (no hold state).
*
*     The task number (JAT_task_number) of this template task will be
*     initialized with the value given by 'ja_task_id'.
*
*  INPUTS
*     const lListElem *job - JB_Type
*     u_long32 ja_task_id  - array task number 
*
*  RESULT
*     lListElem* - template task (JAT_Type)
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_task_template_pending()
*     gdi/job_jatask/job_get_ja_task_template_hold()
*     gdi/job_jatask/job_get_ja_task_template()
*******************************************************************************/
lListElem *job_get_ja_task_template_pending(const lListElem *job,
                                            u_long32 ja_task_id)
{
   lListElem *template_task = NULL;    /* JAT_Type */

   DENTER(BASIS_LAYER, "job_get_ja_task_template");
   template_task = lFirst(lGetList(job, JB_ja_template));
   if (!template_task) {
      ERROR((SGE_EVENT, "unable to retrieve template task\n"));
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

/****** gdi/job_jatask/job_get_ja_task_template_hold() *************************
*  NAME
*     job_get_ja_task_template_hold() -- create a ja task template 
*
*  SYNOPSIS
*     lListElem* job_get_ja_task_template_pending(const lListElem *job, 
*                                                 u_long32 ja_task_id) 
*
*  FUNCTION
*     The function returns a pointer to a template array task element.
*     This task represents a currently submitted pending task.
*
*     The task number (JAT_task_number) of this template task will be
*     initialized with the value given by 'ja_task_id'.
*
*     The hold state of the task (JAT_hold) will be initialized with
*     the 'hold_state' value. The function job_get_ja_task_hold_state()
*     may be used to get the current hold state of an unenrolled
*     pending task.
*
*  INPUTS
*     const lListElem *job - JB_Type
*     u_long32 ja_task_id  - array task number 
*
*  RESULT
*     lListElem* - template task (JAT_Type)
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_task_template_pending()
*     gdi/job_jatask/job_get_ja_task_hold_state()
*     gdi/job_jatask/job_get_ja_task_template()
*******************************************************************************/
lListElem *job_get_ja_task_template_hold(const lListElem *job,
                                         u_long32 ja_task_id, 
                                         u_long32 hold_state)
{
   lListElem *template_task = NULL;    /* JAT_Type */
 
   DENTER(BASIS_LAYER, "job_get_ja_task_template");
   template_task = job_get_ja_task_template_pending(job, ja_task_id);
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

/****** gdi/job_jatask/job_is_zombie_job() *************************************
*  NAME
*     job_is_zombie_job() -- Is 'job' a zombie job 
*
*  SYNOPSIS
*     int job_is_zombie_job(const lListElem *job) 
*
*  FUNCTION
*     True will be returned if 'job' is a zombie job. 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*
*  RESULT
*     int - 0 or 1
*******************************************************************************/
int job_is_zombie_job(const lListElem *job)
{
   return (lGetList(job, JB_ja_z_ids) != NULL);
}

/****** gdi/job_jatask/job_get_ja_task_template() ******************************
*  NAME
*     job_get_ja_task_template() -- create a ja task template 
*
*  SYNOPSIS
*     lListElem* job_get_ja_task_template_pending(const lListElem *job, 
*                                                 u_long32 ja_task_id) 
*
*  FUNCTION
*     The function returns a pointer to a template array task element.
*     This task represents a currently submitted pending task.
*     (see job_get_ja_task_hold_state() and job_get_ja_task_template_hold())
*
*  INPUTS
*     const lListElem *job - JB_Type
*     u_long32 ja_task_id  - array task number 
*
*  RESULT
*     lListElem* - template task (JAT_Type)
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_task_template_pending()
*     gdi/job_jatask/job_get_ja_task_template_hold()
*     gdi/job_jatask/job_get_ja_task_hold_state()
*     gdi/job_jatask/job_get_ja_task_template()
*******************************************************************************/
lListElem *job_get_ja_task_template(const lListElem *job,
                                    u_long32 ja_task_id)
{
   u_long32 hold_state = job_get_ja_task_hold_state(job, ja_task_id);

   return job_get_ja_task_template_hold(job, ja_task_id, hold_state);
}

/****** gdi/job_jatask/job_get_ja_task_hold_state() ****************************
*  NAME
*     job_get_ja_task_hold_state() -- return hold state of unenrolled task
*
*  SYNOPSIS
*     u_long32 job_get_ja_task_hold_state(const lListElem *job, 
*                                         u_long32 ja_task_id) 
*
*  FUNCTION
*     Returns the hold state of a task which is not enrolled in 
*     the JB_ja_tasks list of 'job' 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*     u_long32 ja_task_id  - valid ja task id  
*
*  RESULT
*     u_long32 - hold state 
*******************************************************************************/
u_long32 job_get_ja_task_hold_state(const lListElem *job,
                                     u_long32 ja_task_id)
{
   u_long32 ret = 0;

   DENTER(TOP_LAYER, "job_get_ja_task_hold_state");
   if (range_list_is_id_within(lGetList(job, JB_ja_u_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_USER;
   }
   if (range_list_is_id_within(lGetList(job, JB_ja_o_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_OPERATOR;
   }
   if (range_list_is_id_within(lGetList(job, JB_ja_s_h_ids), ja_task_id)) {
      ret |= MINUS_H_TGT_SYSTEM;
   }
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_create_hold_id_lists() ******************************
*  NAME
*     job_create_hold_id_lists() -- create lists for eight hold combinations
*
*  SYNOPSIS
*     void job_create_hold_id_lists(const lListElem *job, lList *id_list[8], 
*                                   u_long32 hold_state[8]) 
*
*  FUNCTION
*     This function creates eight 'id_lists'. Tasks whose id is contained in
*     an id list has the hold state combination delivered by 'hold_state'.
*
*     After using 'id_list' the function job_destroy_hold_id_lists() has
*     to be called to free allocated memory.
*
*  INPUTS
*     const lListElem *job   - JB_Type 
*     lList *id_list[8]      - NULL initialized pointer array 
*     u_long32 hold_state[8] - Array for hold state combinations 
*
*  SEE ALSO
*     gdi/job_jatask/job_destroy_hold_id_lists() 
*******************************************************************************/
void job_create_hold_id_lists(const lListElem *job, lList *id_list[8], 
                              u_long32 hold_state[8]) 
{
   int i;
   lList *list[7];

   DENTER(TOP_LAYER, "job_create_hold_id_lists");

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
   DEXIT;
}

/****** gdi/job_jatask/job_destroy_hold_id_lists() *****************************
*  NAME
*     job_destroy_hold_id_lists() -- destroy lists for eight hold combinations 
*
*  SYNOPSIS
*     void job_destroy_hold_id_lists(const lListElem *job, lList *id_list[8]) 
*
*  FUNCTION
*     This function frees all memory allocated by a previous call of 
*     job_create_hold_id_lists(). 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*     lList *id_list[8]    - array of RN_Type lists
*
*  SEE ALSO
*     gdi/job_jatask/job_create_hold_id_lists 
*******************************************************************************/
void job_destroy_hold_id_lists(const lListElem *job, lList *id_list[8]) 
{
   int i;

   DENTER(TOP_LAYER, "job_destroy_hold_id_lists");
   for (i = 0; i < 8; i++) {
      id_list[i] = lFreeList(id_list[i]);
   }
   DEXIT;
}

/****** gdi/job_jatask/job_is_enrolled() ***************************************
*  NAME
*     job_is_enrolled() -- Is a certain array task enrolled in JB_ja_tasks 
*
*  SYNOPSIS
*     int job_is_enrolled(const lListElem *job, u_long32 task_number) 
*
*  FUNCTION
*     This function will return true (1) if the array task with 
*     'task_number' is not enrolled in the JB_ja_tasks sublist of 'job'.
*
*  INPUTS
*     const lListElem *job - JB_Type 
*     u_long32 task_number - task_number 
*
*  RESULT
*     int - 0 or 1
*******************************************************************************/
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

/****** gdi/job_jatask/job_is_ja_task_defined() ********************************
*  NAME
*     job_is_ja_task_defined() -- was this task submitted 
*
*  SYNOPSIS
*     int job_is_ja_task_defined(const lListElem *job, u_long32 ja_task_number) 
*
*  FUNCTION
*     This function will return true (1) if the task with 'ja_task_number' is
*     defined within the array 'job'. The task is defined when 'ja_task_number'
*     is enclosed in the task id range which was specified during submit
*     time (qsub -t). 
*
*  INPUTS
*     const lListElem *job    - JB_Type 
*     u_long32 ja_task_number - task number 
*
*  RESULT
*     int - 0 or 1
*******************************************************************************/
int job_is_ja_task_defined(const lListElem *job, u_long32 ja_task_number) 
{
   lList *range_list = lGetList(job, JB_ja_structure);

   return range_list_is_id_within(range_list, ja_task_number);
}

/****** gdi/job_jatask/job_get_ja_tasks() **************************************
*  NAME
*     job_get_ja_tasks() -- returns number of array tasks 
*
*  SYNOPSIS
*     u_long32 job_get_ja_tasks(const lListElem *job) 
*
*  FUNCTION
*     This function returns the overall amount of tasks in an array job. 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*
*  RESULT
*     u_long32 - number of tasks 
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_tasks() 
*     gdi/job_jatask/job_get_enrolled_ja_tasks_ja_tasks() 
*     gdi/job_jatask/job_get_not_enrolled_ja_tasks() 
*******************************************************************************/
u_long32 job_get_ja_tasks(const lListElem *job) 
{  
   u_long32 ret = 0;

   DENTER(TOP_LAYER, "job_get_ja_tasks");
   ret += job_get_not_enrolled_ja_tasks(job);
   ret += job_get_enrolled_ja_tasks(job);
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_get_not_enrolled_ja_tasks() *************************
*  NAME
*     job_get_not_enrolled_ja_tasks() -- returns num. of unenrolled array tasks 
*
*  SYNOPSIS
*     u_long32 job_get_not_enrolled_ja_tasks(const lListElem *job) 
*
*  FUNCTION
*     This function returns the amount of tasks not enrolled in the
*     JB_ja_tasks sublist. 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*
*  RESULT
*     u_long32 - number of tasks 
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_tasks() 
*     gdi/job_jatask/job_get_enrolled_ja_tasks_ja_tasks() 
*     gdi/job_jatask/job_get_not_enrolled_ja_tasks() 
*******************************************************************************/
u_long32 job_get_not_enrolled_ja_tasks(const lListElem *job) 
{
   const int attributes = 4;
   const int attribute[] = {JB_ja_n_h_ids, JB_ja_u_h_ids, JB_ja_o_h_ids,
                            JB_ja_s_h_ids};
   u_long32 ret = 0;
   int i;

   DENTER(TOP_LAYER, "job_get_not_enrolled_ja_tasks");     
   for (i = 0; i < attributes; i++) {
      ret += range_list_get_number_of_ids(lGetList(job, attribute[i]));
   } 
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_get_enrolled_ja_tasks() *****************************
*  NAME
*     job_get_enrolled_ja_tasks() -- returns num. of enrolled array tasks 
*
*  SYNOPSIS
*     u_long32 job_get_not_enrolled_ja_tasks(const lListElem *job) 
*
*  FUNCTION
*     This function returns the amount of tasks enrolled in the
*     JB_ja_tasks sublist. 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*
*  RESULT
*     u_long32 - number of tasks 
*
*  SEE ALSO
*     gdi/job_jatask/job_get_ja_tasks() 
*     gdi/job_jatask/job_get_enrolled_ja_tasks_ja_tasks() 
*     gdi/job_jatask/job_get_not_enrolled_ja_tasks() 
*******************************************************************************/
u_long32 job_get_enrolled_ja_tasks(const lListElem *job) 
{
   return lGetNumberOfElem(lGetList(job, JB_ja_tasks));
}
 
/****** gdi/job_jatask/job_enroll() ********************************************
*  NAME
*     job_enroll() -- enrolls a array task into the JB_ja_tasks lists 
*
*  SYNOPSIS
*     void job_enroll(lListElem *job, lList **answer_list, 
*                     u_long32 ja_task_number) 
*
*  FUNCTION
*     The task with 'ja_task_number' will be enrolled into the JB_ja_tasks
*     list of 'job' when this function is called. 
*
*  INPUTS
*     lListElem *job          - JB_Type 
*     lList **answer_list     - AN_Type 
*     u_long32 ja_task_number - task number 
*******************************************************************************/
void job_enroll(lListElem *job, lList **answer_list, u_long32 ja_task_number)
{
   lList *range_list = NULL;
   lListElem *ja_task = NULL;

   DENTER(TOP_LAYER, "job_enroll");
   lXchgList(job, JB_ja_n_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_n_h_ids, &range_list);
   range_list_compress(lGetList(job, JB_ja_n_h_ids));

   lXchgList(job, JB_ja_u_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_u_h_ids, &range_list);
   range_list_compress(lGetList(job, JB_ja_u_h_ids));

   lXchgList(job, JB_ja_o_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_o_h_ids, &range_list);
   range_list_compress(lGetList(job, JB_ja_o_h_ids));

   lXchgList(job, JB_ja_s_h_ids, &range_list);
   range_list_remove_id(&range_list, answer_list, ja_task_number);
   lXchgList(job, JB_ja_s_h_ids, &range_list);
   range_list_compress(lGetList(job, JB_ja_s_h_ids));

   /* EB: should we add a new CULL function? */
   ja_task = lGetSubUlong(job, JAT_task_number, ja_task_number, JB_ja_tasks);
   if (ja_task == NULL) {
      lListElem *template_task = NULL;
      lList *ja_task_list = lGetList(job, JB_ja_tasks);

      if (ja_task_list == NULL) {
         ja_task_list = lCreateList("", JAT_Type);
         lSetList(job, JB_ja_tasks, ja_task_list);
      }
      template_task = job_get_ja_task_template_pending(job, ja_task_number); 
      ja_task = lCopyElem(template_task);
      lAppendElem(ja_task_list, ja_task); 
   }
   DEXIT;
}  

/****** gdi/job_jatask/job_has_tasks() *****************************************
*  NAME
*     job_has_tasks() -- Returns true if there exist unenrolled tasks 
*
*  SYNOPSIS
*     int job_has_tasks(lListElem *job) 
*
*  FUNCTION
*     This function returns true (1) if there exists an unenrolled pending 
*     task in 'job'.
*
*  INPUTS
*     lListElem *job - JB_Type 
*
*  RESULT
*     int - 0 or 1
*******************************************************************************/
int job_has_tasks(lListElem *job) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "job_has_tasks");
   if (job != NULL) {
      if (lGetList(job, JB_ja_n_h_ids) != NULL || 
          lGetList(job, JB_ja_u_h_ids) != NULL ||
          lGetList(job, JB_ja_o_h_ids) != NULL ||
          lGetList(job, JB_ja_s_h_ids) != NULL) {
         ret = 1;
      }
   }
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_delete_not_enrolled_ja_task() ***********************
*  NAME
*     job_delete_not_enrolled_ja_task() -- remove unenrolled pending task 
*
*  SYNOPSIS
*     void job_delete_not_enrolled_ja_task(lListElem *job, lList **answer_list, 
*                                          u_long32 ja_task_number) 
*
*  FUNCTION
*     Removes an unenrolled pending task from the id lists.
*
*  INPUTS
*     lListElem *job          - JB_Type 
*     lList **answer_list     - AN_Type 
*     u_long32 ja_task_number - Task to be removed 
*
*  SEE ALSO
*     gdi/job_jatask/job_add_as_zombie()
*******************************************************************************/
void job_delete_not_enrolled_ja_task(lListElem *job, lList **answer_list, 
                                     u_long32 ja_task_number) 
{
   const int attributes = 4;
   const int attribute[] = {JB_ja_n_h_ids, JB_ja_u_h_ids, JB_ja_o_h_ids,
                            JB_ja_s_h_ids};
   int i;

   DENTER(TOP_LAYER, "job_delete_not_enrolled_ja_task");
   for (i = 0; i < attributes; i++) { 
      lList *range_list = NULL;

      lXchgList(job, attribute[i], &range_list);
      range_list_remove_id(&range_list, answer_list, ja_task_number);
      range_list_compress(range_list);
      lXchgList(job, attribute[i], &range_list);
   }
   DEXIT;
} 

/****** gdi/job_jatask/job_add_as_zombie() *************************************
*  NAME
*     job_add_as_zombie() -- add task into zombie id list 
*
*  SYNOPSIS
*     void job_add_as_zombie(lListElem *zombie, lList **answer_list, 
*                            u_long32 ja_task_id) 
*
*  FUNCTION
*     Adds a task into the zombie id list (JB_ja_z_ids)
*
*  INPUTS
*     lListElem *zombie    - JB_Type 
*     lList **answer_list  - AN_Type 
*     u_long32 ja_task_id  - Task id to be inserted
*
*  SEE ALSO
*     gdi/job_jatask/job_delete_not_enrolled_ja_task()
*******************************************************************************/
void job_add_as_zombie(lListElem *zombie, lList **answer_list, 
                       u_long32 ja_task_id) 
{
   lList *z_ids = NULL;    /* RN_Type */

   DENTER(TOP_LAYER, "job_add_as_zombie");
   lXchgList(zombie, JB_ja_z_ids, &z_ids);
   range_list_insert_id(&z_ids, NULL, ja_task_id);
   range_list_compress(z_ids);
   lXchgList(zombie, JB_ja_z_ids, &z_ids);    
   DEXIT;
}

/****** sge/job_jatask/job_has_job_pending_tasks() *****************************
*  NAME
*     job_has_job_pending_tasks() -- Has the job unenrolled pending tasks? 
*
*  SYNOPSIS
*     int job_has_job_pending_tasks(lListElem *job) 
*
*  FUNCTION
*     True (1) will be returned if the job has unenrolled pending tasks.
*
*  INPUTS
*     lListElem *job - JB_Type 
*
*  RESULT
*     int - True (1) or false (0)
*******************************************************************************/
int job_has_job_pending_tasks(lListElem *job) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "job_has_job_pending_tasks");
   if (lGetList(job, JB_ja_n_h_ids) || lGetList(job, JB_ja_u_h_ids) ||
       lGetList(job, JB_ja_o_h_ids) || lGetList(job, JB_ja_s_h_ids)) {
      ret = 1;
   }
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_set_hold_state() ************************************
*  NAME
*     job_set_hold_state() -- Changes the hold state of a job/array-task.
*
*  SYNOPSIS
*     void job_set_hold_state(lListElem *job, lList **answer_list, 
*                             u_long32 ja_task_id, u_long32 new_hold_state) 
*
*  FUNCTION
*     This function changes the hold state of a job/array-task.
*
*  INPUTS
*     lListElem *job          - JB_Type 
*     lList **answer_list     - AN_Type 
*     u_long32 ja_task_id     - Array task id 
*     u_long32 new_hold_state - hold state (see MINUS_H_TGT_*)
*******************************************************************************/
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
         range_list_compress(lGetList(job, attribute[i]));
      }
   } else {
      lListElem *ja_task = job_search_task(job, NULL, ja_task_id, 0);

      if (ja_task != NULL) { 
         lSetUlong(ja_task, JAT_hold, new_hold_state); 
         if (new_hold_state) {
            lSetUlong(ja_task, JAT_state, 
                      lGetUlong(ja_task, JAT_state) | JHELD);
         } else {
            lSetUlong(ja_task, JAT_state, 
                      lGetUlong(ja_task, JAT_state) & ~JHELD);
         }
      }
   }
   DEXIT;
}

/****** gdi/job_jatask/job_get_hold_state() ************************************
*  NAME
*     job_get_hold_state() -- Returns the hold state of a job/array-task
*
*  SYNOPSIS
*     u_long32 job_get_hold_state(lListElem *job, u_long32 ja_task_id) 
*
*  FUNCTION
*     Returns the hold state of a job/array-task 
*
*  INPUTS
*     lListElem *job      - JB_Type 
*     u_long32 ja_task_id - array task id 
*
*  RESULT
*     u_long32 - hold state (see MINUS_H_TGT_*)
*******************************************************************************/
u_long32 job_get_hold_state(lListElem *job, u_long32 ja_task_id)
{
   u_long32 ret = 0;

   DENTER(TOP_LAYER, "job_get_hold_state");
   if (job_is_enrolled(job, ja_task_id)) {
      lListElem *ja_task = job_search_task(job, NULL, ja_task_id, 0);
  
      if (ja_task != NULL) {  
         ret = lGetUlong(ja_task, JAT_hold) & MINUS_H_TGT_ALL;
      } else {
         ret = 0;
      }
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
   DEXIT;
   return ret;
}

/****** gdi/job_jatask/job_search_task() ***************************************
*  NAME
*     job_search_task() -- Search an array task (possibly enroll it) 
*
*  SYNOPSIS
*     lListElem* job_search_task(lListElem *job, lList **answer_list, 
*                                u_long32 ja_task_id, 
*                                int enroll_if_not_existing) 
*
*  FUNCTION
*     This function return the array task with the id 'ja_task_id' if it exists
*     in the JB_ja_tasks-sublist of 'job'. If this task does not exist in this
*     list, NULL will be returned. 
*     If the task is not found in the sublist and if 'enroll_if_not_existing'  
*     is true (1) than a new element will be created in the sublist
*     of 'job' and a pointer to the new element will be returned.
*     Errors may be found in the 'answer_list'
*
*  INPUTS
*     lListElem *job             - JB_Type 
*     lList **answer_list        - AN_Type 
*     u_long32 ja_task_id        - array task id 
*     int enroll_if_not_existing - True (1) or false (0) 
*
*  RESULT
*     lListElem* - JAT_Type element
*******************************************************************************/
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

/****** gdi/job_jatask/job_get_shell_start_mode() *****************************
*  NAME
*     job_get_shell_start_mode() -- get shell start mode for 'job' 
*
*  SYNOPSIS
*     const char* job_get_shell_start_mode(const lListElem *job, 
*                                          const lListElem *queue,
*                                          const char *conf_shell_start_mode) 
*
*  FUNCTION
*     Returns a string identifying the shell start mode for 'job'.
*
*  INPUTS
*     const lListElem *job              - JB_Type element 
*     const lListElem *queue            - QU_Type element
*     const char *conf_shell_start_mode - shell start mode of configuration
*
*  RESULT
*     const char* - shell start mode
******************************************************************************/
const char *job_get_shell_start_mode(const lListElem *job,
                                     const lListElem *queue,
                                     const char *conf_shell_start_mode) 
{
   const char *ret;

   if (lGetString(job, JB_job_source)) {
      ret = "raw_exec";
   } else {
      const char *queue_start_mode = lGetString(queue, QU_shell_start_mode);
   
      if (queue_start_mode && strcasecmp(queue_start_mode, "none")) {
         ret = queue_start_mode;
      } else {
         ret = conf_shell_start_mode;
      }
   }
   return ret;
}

/****** gdi/job_jatask/job_list_add_job() *************************************
*  NAME
*     job_list_add_job() -- Creates a joblist and adds an job into it 
*
*  SYNOPSIS
*     int job_list_add_job(lList **job_list, const char *name, lListElem *job, 
*                          int check) 
*
*  FUNCTION
*     A 'job_list' will be created by this function if it does not already 
*     exist and 'job' will be inserted into this 'job_list'. 'name' will
*     be the name of the new list.
*
*     If 'check' is true (1) than the function will test whether there is 
*     already an element in 'job_list' which has the same 'JB_job_number' like
*     'job'. If this is true than -1 will be returned by this function.
*      
*
*  INPUTS
*     lList **job_list - JB_Type
*     const char *name - name of the list
*     lListElem *job   - JB_Type element 
*     int check        - Does the element already exist? 
*
*  RESULT
*     int - error code
*           1 => invalid parameter
*          -1 => check failed: element already exists
*           0 => OK
*******************************************************************************/
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

/****** gdi/job_jatask/job_get_smallest_unenrolled_task_id() ******************
*  NAME
*     job_get_smallest_unenrolled_task_id() -- find smallest unenrolled id
*
*  SYNOPSIS
*     u_long32 job_get_smallest_unenrolled_task_id(const lListElem *job)
*
*  FUNCTION
*     Returns the smallest task id currently existing in a job
*     which is not enrolled in the JB_ja_tasks sublist of 'job'.
*     If all tasks are enrolled 0 will be returned.
*
*  INPUTS
*     const lListElem *job - JB_Type element
*
*  RESULT
*     u_long32 - task id or 0
******************************************************************************/
u_long32 job_get_smallest_unenrolled_task_id(const lListElem *job)
{
   u_long32 n_h_id, u_h_id, o_h_id, s_h_id;
   u_long32 ret = 0;

   n_h_id = range_list_get_first_id(lGetList(job, JB_ja_n_h_ids), NULL);
   u_h_id = range_list_get_first_id(lGetList(job, JB_ja_u_h_ids), NULL);
   o_h_id = range_list_get_first_id(lGetList(job, JB_ja_o_h_ids), NULL);
   s_h_id = range_list_get_first_id(lGetList(job, JB_ja_s_h_ids), NULL);
   ret = n_h_id;
   if (ret > 0 && u_h_id > 0) {
      ret = MIN(ret, u_h_id);
   } else if (u_h_id > 0) {
      ret = u_h_id;
   }
   if (ret > 0 && o_h_id > 0) {
      ret = MIN(ret, o_h_id);
   } else if (o_h_id > 0) {
      ret = o_h_id;
   }
   if (ret == 0 && s_h_id > 0)  {
      ret = MIN(ret, s_h_id);
   } else if (s_h_id > 0 ){
      ret = s_h_id;
   }
   return ret;
}                

/****** gdi/job_jatask/job_get_smallest_enrolled_task_id() ********************
*  NAME
*     job_get_smallest_enrolled_task_id() -- find smallest enrolled tid
*
*  SYNOPSIS
*     u_long32 job_get_smallest_enrolled_task_id(const lListElem *job)
*
*  FUNCTION
*     Returns the smallest task id currently existing in a job
*     which is enrolled in the JB_ja_tasks sublist of 'job'.
*     If no task is enrolled 0 will be returned.
*
*  INPUTS
*     const lListElem *job - JB_Type element
*
*  RESULT
*     u_long32 - task id or 0
******************************************************************************/
u_long32 job_get_smallest_enrolled_task_id(const lListElem *job)
{
   lListElem *ja_task;        /* JAT_Type */
   lListElem *nxt_ja_task;    /* JAT_Type */
   u_long32 ret = 0;

   /*
    * initialize ret
    */
   ja_task = lFirst(lGetList(job, JB_ja_tasks));
   nxt_ja_task = lNext(ja_task);
   if (ja_task != NULL) {
      ret = lGetUlong(ja_task, JAT_task_number);
   }

   /*
    * try to find a smaller task id
    */
   while ((ja_task = nxt_ja_task)) {
      nxt_ja_task = lNext(ja_task);

      ret = MIN(ret, lGetUlong(ja_task, JAT_task_number));
   }
   return ret;
}       

/****** gdi/job_jatask/job_get_biggest_unenrolled_task_id() *******************
*  NAME
*     job_get_biggest_unenrolled_task_id() -- find biggest unenrolled id
*
*  SYNOPSIS
*     u_long32 job_get_biggest_unenrolled_task_id(const lListElem *job)
*
*  FUNCTION
*     Returns the biggest task id currently existing in a job
*     which is not enrolled in the JB_ja_tasks sublist of 'job'.
*     If no task is enrolled 0 will be returned.
*
*  INPUTS
*     const lListElem *job - JB_Type element
*
*  RESULT
*     u_long32 - task id or 0
******************************************************************************/
u_long32 job_get_biggest_unenrolled_task_id(const lListElem *job)
{
   u_long32 n_h_id, u_h_id, o_h_id, s_h_id;
   u_long32 ret = 0;

   n_h_id = range_list_get_last_id(lGetList(job, JB_ja_n_h_ids), NULL);
   u_h_id = range_list_get_last_id(lGetList(job, JB_ja_u_h_ids), NULL);
   o_h_id = range_list_get_last_id(lGetList(job, JB_ja_o_h_ids), NULL);
   s_h_id = range_list_get_last_id(lGetList(job, JB_ja_s_h_ids), NULL);
   ret = n_h_id;
   if (ret > 0 && u_h_id > 0) {
      ret = MAX(ret, u_h_id);
   } else if (u_h_id > 0) {
      ret = u_h_id;
   }
   if (ret > 0 && o_h_id > 0) {
      ret = MAX(ret, o_h_id);
   } else if (o_h_id > 0) {
      ret = o_h_id;
   }
   if (ret == 0 && s_h_id > 0)  {
      ret = MAX(ret, s_h_id);
   } else if (s_h_id > 0 ){
      ret = s_h_id;
   }
   return ret;
}  

/****** gdi/job_jatask/job_get_biggest_enrolled_task_id() ********************
*  NAME
*     job_get_biggest_enrolled_task_id() -- find biggest enrolled tid
*
*  SYNOPSIS
*     u_long32 job_get_biggest_enrolled_task_id(const lListElem *job)
*
*  FUNCTION
*     Returns the biggest task id currently existing in a job
*     which is enrolled in the JB_ja_tasks sublist of 'job'.
*     If no task is enrolled 0 will be returned.
*
*  INPUTS
*     const lListElem *job - JB_Type element
*
*  RESULT
*     u_long32 - task id or 0
******************************************************************************/
u_long32 job_get_biggest_enrolled_task_id(const lListElem *job)
{
   lListElem *ja_task;        /* JAT_Type */
   lListElem *nxt_ja_task;    /* JAT_Type */
   u_long32 ret = 0;

   /*
    * initialize ret
    */
   ja_task = lLast(lGetList(job, JB_ja_tasks));
   nxt_ja_task = lPrev(ja_task);
   if (ja_task != NULL) {
      ret = lGetUlong(ja_task, JAT_task_number);
   }

   /*
    * try to find a smaller task id
    */
   while ((ja_task = nxt_ja_task)) {
      nxt_ja_task = lPrev(ja_task);

      ret = MAX(ret, lGetUlong(ja_task, JAT_task_number));
   }
   return ret;
}  

