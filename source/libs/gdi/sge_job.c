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
#include <unistd.h>

#include <fnmatch.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge_ja_task.h"
#include "sge_job.h"
#include "sge_range.h"
#include "sge_htable.h"
#include "read_write_job.h"
#include "sge_gdi.h"
#include "sge_stdlib.h"
#include "sge_var.h"
#include "sge_path_alias.h"
#include "sge_var.h"
#include "sge_answer.h"
#include "sge_prog.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_queue.h"

#include "msg_gdilib.h"
#include "msg_common.h"

lList *Master_Job_List = NULL;
lList *Master_Zombie_List = NULL;
lList *Master_Job_Schedd_Info_List = NULL;

/****** gdi/job/job_get_ja_task_template_pending() ****************************
*  NAME
*     job_get_ja_task_template_pending() -- create a ja task template 
*
*  SYNOPSIS
*     lListElem* job_get_ja_task_template_pending(const lListElem *job, 
*                                                 u_long32 ja_task_id) 
*
*  FUNCTION
*     The function returns a pointer to a template array task element.
*     This task represents a currently submitted pending task 
*     (no hold state).
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
*     gdi/job/job_get_ja_task_template_pending()
*     gdi/job/job_get_ja_task_template_hold()
*     gdi/job/job_get_ja_task_template()
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

/****** gdi/job/job_get_ja_task_template_hold() *******************************
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
*     gdi/job/job_get_ja_task_template_pending()
*     gdi/job/job_get_ja_task_hold_state()
*     gdi/job/job_get_ja_task_template()
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

/****** gdi/job/job_is_zombie_job() *******************************************
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

/****** gdi/job/job_get_ja_task_template() ************************************
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
*
*  INPUTS
*     const lListElem *job - JB_Type
*     u_long32 ja_task_id  - array task number 
*
*  RESULT
*     lListElem* - template task (JAT_Type)
*
*  SEE ALSO
*     gdi/job/job_get_ja_task_template_pending()
*     gdi/job/job_get_ja_task_template_hold()
*     gdi/job/job_get_ja_task_hold_state()
*     gdi/job/job_get_ja_task_template()
*******************************************************************************/
lListElem *job_get_ja_task_template(const lListElem *job,
                                    u_long32 ja_task_id)
{
   u_long32 hold_state = job_get_ja_task_hold_state(job, ja_task_id);

   return job_get_ja_task_template_hold(job, ja_task_id, hold_state);
}

/****** gdi/job/job_get_ja_task_hold_state() **********************************
*  NAME
*     job_get_ja_task_hold_state() -- Hold state of unenrolled task
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

/****** gdi/job/job_create_hold_id_lists() ************************************
*  NAME
*     job_create_hold_id_lists() -- Lists for hold combinations
*
*  SYNOPSIS
*     void job_create_hold_id_lists(const lListElem *job, 
*                                   lList *id_list[8], 
*                                   u_long32 hold_state[8]) 
*
*  FUNCTION
*     This function creates eight 'id_lists'. Tasks whose id is 
*     contained in an id list has the hold state combination delivered 
*     by 'hold_state'.
*
*     After using 'id_list' the function job_destroy_hold_id_lists() 
*     has to be called to free allocated memory.
*
*  INPUTS
*     const lListElem *job   - JB_Type 
*     lList *id_list[8]      - NULL initialized pointer array 
*     u_long32 hold_state[8] - Array for hold state combinations 
*
*  SEE ALSO
*     gdi/job/job_destroy_hold_id_lists() 
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
   range_list_calculate_intersection_set(&list[0], NULL, 
                  lGetList(job, JB_ja_u_h_ids), lGetList(job, JB_ja_o_h_ids));
   /* us */
   range_list_calculate_intersection_set(&list[1], NULL, 
                  lGetList(job, JB_ja_u_h_ids), lGetList(job, JB_ja_s_h_ids));
   /* os */
   range_list_calculate_intersection_set(&list[2], NULL, 
                  lGetList(job, JB_ja_o_h_ids), lGetList(job, JB_ja_s_h_ids));

   /* uos -> 7 */
   range_list_calculate_intersection_set(&id_list[7], NULL, list[2], list[1]);

   /* osU -> 6 */
   range_list_calculate_difference_set(&id_list[6], NULL, list[2], id_list[7]);
   /* usO -> 5 */
   range_list_calculate_difference_set(&id_list[5], NULL, list[1], id_list[7]);
   /* uoS -> 4 */
   range_list_calculate_difference_set(&id_list[4], NULL, list[0], id_list[7]);

   /* sOU -> 3 */
   range_list_calculate_difference_set(&list[6], NULL, 
                  lGetList(job, JB_ja_s_h_ids), list[1]);
   range_list_calculate_difference_set(&id_list[3], NULL, list[6], id_list[6]);       

   /* oUS -> 2 */
   range_list_calculate_difference_set(&list[5], NULL, 
                  lGetList(job, JB_ja_o_h_ids), list[0]);
   range_list_calculate_difference_set(&id_list[2], NULL, list[5], id_list[6]);
   
   /* uOS -> 1 */ 
   range_list_calculate_difference_set(&list[4], NULL, 
                  lGetList(job, JB_ja_u_h_ids), list[1]);
   range_list_calculate_difference_set(&id_list[1], NULL, list[4], id_list[4]);
   
   /* UOS -> 0 */
   id_list[0] = lCopyList("", lGetList(job, JB_ja_n_h_ids));

   for (i = 0; i < 7; i++) {
      list[i] = lFreeList(list[i]);
   }
   DEXIT;
}

/****** gdi/job/job_destroy_hold_id_lists() ***********************************
*  NAME
*     job_destroy_hold_id_lists() -- destroy hold combination lists
*
*  SYNOPSIS
*     void job_destroy_hold_id_lists(const lListElem *job, 
*                                    lList *id_list[8]) 
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
*     gdi/job/job_create_hold_id_lists 
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

/****** gdi/job/job_is_enrolled() *********************************************
*  NAME
*     job_is_enrolled() -- Is a certain array task enrolled 
*
*  SYNOPSIS
*     int job_is_enrolled(const lListElem *job, u_long32 task_number) 
*
*  FUNCTION
*     This function will return true (1) if the array task with 
*     'task_number' is not enrolled in the JB_ja_tasks sublist 
*     of 'job'.
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

/****** gdi/job/job_is_ja_task_defined() ***************************************
*  NAME
*     job_is_ja_task_defined() -- was this task submitted 
*
*  SYNOPSIS
*     int job_is_ja_task_defined(const lListElem *job, 
*                                u_long32 ja_task_number) 
*
*  FUNCTION
*     This function will return true (1) if the task with 
*     'ja_task_number' is defined within the array 'job'. The task 
*     is defined when 'ja_task_number' is enclosed in the task id 
*     range which was specified during submit time (qsub -t). 
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

/****** gdi/job/job_get_ja_tasks() ********************************************
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
*     gdi/job/job_get_ja_tasks() 
*     gdi/job/job_get_enrolled_ja_tasks() 
*     gdi/job/job_get_not_enrolled_ja_tasks() 
*     gdi/job/job_get_submit_ja_tasks() 
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

/****** gdi/job/job_get_not_enrolled_ja_tasks() *******************************
*  NAME
*     job_get_not_enrolled_ja_tasks() -- num. of unenrolled tasks 
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
*     gdi/job/job_get_ja_tasks() 
*     gdi/job/job_get_enrolled_ja_tasks() 
*     gdi/job/job_get_not_enrolled_ja_tasks() 
*     gdi/job/job_get_submit_ja_tasks() 
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

/****** gdi/job/job_get_enrolled_ja_tasks() ***********************************
*  NAME
*     job_get_enrolled_ja_tasks() -- num. of enrolled array tasks 
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
*     gdi/job/job_get_ja_tasks() 
*     gdi/job/job_get_enrolled_ja_tasks() 
*     gdi/job/job_get_not_enrolled_ja_tasks() 
*     gdi/job/job_get_submit_ja_tasks() 
*******************************************************************************/
u_long32 job_get_enrolled_ja_tasks(const lListElem *job) 
{
   return lGetNumberOfElem(lGetList(job, JB_ja_tasks));
}

/****** gdi/job/job_get_submit_ja_tasks() *************************************
*  NAME
*     job_get_submit_ja_tasks() -- array size during job submittion 
*
*  SYNOPSIS
*     u_long32 job_get_submit_ja_tasks(const lListElem *job) 
*
*  FUNCTION
*     The function returns the ammount of tasks the job had during
*     it's submittion 
*
*  INPUTS
*     const lListElem *job - JB_Type 
*
*  RESULT
*     u_long32 - number of tasks
*
*  SEE ALSO
*     gdi/job/job_get_ja_tasks() 
*     gdi/job/job_get_enrolled_ja_tasks() 
*     gdi/job/job_get_not_enrolled_ja_tasks() 
*     gdi/job/job_get_submit_ja_tasks() 
******************************************************************************/
u_long32 job_get_submit_ja_tasks(const lListElem *job)
{
   u_long32 start, end, step;
 
   job_get_submit_task_ids(job, &start, &end, &step);
   return ((end - start) / step + 1); 
}
 
/****** gdi/job/job_enroll() **************************************************
*  NAME
*     job_enroll() -- enrolls a array task into the JB_ja_tasks lists 
*
*  SYNOPSIS
*     void job_enroll(lListElem *job, lList **answer_list, 
*                     u_long32 ja_task_number) 
*
*  FUNCTION
*     The task with 'ja_task_number' will be enrolled into the 
*     JB_ja_tasks list of 'job' when this function is called. 
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

/****** gdi/job/job_has_tasks() ***********************************************
*  NAME
*     job_has_tasks() -- Returns true if there exist unenrolled tasks 
*
*  SYNOPSIS
*     int job_has_tasks(lListElem *job) 
*
*  FUNCTION
*     This function returns true (1) if there exists an unenrolled 
*     pending task in 'job'.
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

/****** gdi/job/job_delete_not_enrolled_ja_task() *****************************
*  NAME
*     job_delete_not_enrolled_ja_task() -- remove unenrolled task 
*
*  SYNOPSIS
*     void job_delete_not_enrolled_ja_task(lListElem *job, 
*                                          lList **answer_list, 
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
*     gdi/job/job_add_as_zombie()
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

/****** gdi/job/job_add_as_zombie() *******************************************
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
*     gdi/job/job_delete_not_enrolled_ja_task()
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

/****** gdi/job/job_has_job_pending_tasks() ***********************************
*  NAME
*     job_has_job_pending_tasks() -- Has the job unenrolled tasks? 
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

/****** gdi/job/job_set_hold_state() ******************************************
*  NAME
*     job_set_hold_state() -- Changes the hold state of a task.
*
*  SYNOPSIS
*     void job_set_hold_state(lListElem *job, lList **answer_list, 
*                             u_long32 ja_task_id, 
*                             u_long32 new_hold_state) 
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

/****** gdi/job/job_get_hold_state() ******************************************
*  NAME
*     job_get_hold_state() -- Returns the hold state of a task
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

/****** gdi/job/job_search_task() *********************************************
*  NAME
*     job_search_task() -- Search an array task (possibly enroll it) 
*
*  SYNOPSIS
*     lListElem* job_search_task(lListElem *job, lList **answer_list, 
*                                u_long32 ja_task_id, 
*                                int enroll_if_not_existing) 
*
*  FUNCTION
*     This function return the array task with the id 'ja_task_id' if 
*     it exists in the JB_ja_tasks-sublist of 'job'. If this task 
*     does not exist in this list, NULL will be returned. If the task 
*     is not found in the sublist and if 'enroll_if_not_existing'  
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

/****** gdi/job/job_get_shell_start_mode() ************************************
*  NAME
*     job_get_shell_start_mode() -- get shell start mode for 'job' 
*
*  SYNOPSIS
*     const char* job_get_shell_start_mode(const lListElem *job, 
*                                        const lListElem *queue,
*                             const char *conf_shell_start_mode) 
*
*  FUNCTION
*     Returns a string identifying the shell start mode for 'job'.
*
*  INPUTS
*     const lListElem *job              - JB_Type element 
*     const lListElem *queue            - QU_Type element
*     const char *conf_shell_start_mode - shell start mode of 
*                                         configuration
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

/****** gdi/job/job_list_add_job() ********************************************
*  NAME
*     job_list_add_job() -- Creates a joblist and adds an job into it 
*
*  SYNOPSIS
*     int job_list_add_job(lList **job_list, const char *name, 
*                          lListElem *job, int check) 
*
*  FUNCTION
*     A 'job_list' will be created by this function if it does not 
*     already exist and 'job' will be inserted into this 'job_list'. 
*     'name' will be the name of the new list.
*
*     If 'check' is true (1) than the function will test whether 
*     there is already an element in 'job_list' which has the same 
*     'JB_job_number' like 'job'. If this is true than -1 will be 
*     returned by this function.
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
       job_list_locate(*job_list, lGetUlong(job, JB_job_number))) {
      ERROR((SGE_EVENT, MSG_JOB_JOBALREADYEXISTS_S, 
             job_get_id_string(lGetUlong(job, JB_job_number), 0, NULL)));
      DEXIT;
      return -1;
   }

   lAppendElem(*job_list, job);

   DEXIT;
   return 0;
}     

/****** gdi/job/job_is_array() ************************************************
*  NAME
*     job_is_array() -- Is "job" an array job or not? 
*
*  SYNOPSIS
*     int job_is_array(const lListElem *job) 
*
*  FUNCTION
*     The function returns true (1) if "job" is an array job with more 
*     than one task. 
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*
*  RESULT
*     int - 1 -> array job
*           0 -> non array job
*
*  SEE ALSO
*     gdi/job/job_is_parallel()
*     gdi/job/job_is_tight_parallel()
******************************************************************************/
int job_is_array(const lListElem *job)
{
   u_long32 start, end, step;

   job_get_submit_task_ids(job, &start, &end, &step);
   return (start != 1 || end != 1 || step != 1);
}  

/****** gdi/job/job_is_parallel() *********************************************
*  NAME
*     job_is_parallel() -- Is "job" a parallel job? 
*
*  SYNOPSIS
*     int job_is_parallel(const lListELem *job) 
*
*  FUNCTION
*     This function returns true (1) if "job" is a parallel job
*     (requesting a parallel environment). 
*
*  INPUTS
*     const lListELem *job - JB_Type element 
*
*  RESULT
*     int - 1 -> parallel job
*           0 -> non-parallel job
*
*  SEE ALSO
*     gdi/job/job_is_array() 
*     gdi/job/job_is_tight_parallel()
*******************************************************************************/
int job_is_parallel(const lListElem *job)
{
   return (lGetString(job, JB_pe) != NULL);
} 

/****** gdi/job/job_is_tight_parallel() ***************************************
*  NAME
*     job_is_tight_parallel() -- Is "job" a tightly integrated par. job?
*
*  SYNOPSIS
*     int job_is_tight_parallel(const lListElem *job, 
*                               const lList *pe_list) 
*
*  FUNCTION
*     This function returns true (1) if "job" is really a tightly 
*     integrated parallel job. 
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*     const lList *pe_list - PE_Type list with all existing PEs 
*
*  RESULT
*     int - 1 -> tightly integrated parallel job
*           0 -> other type
*
*  SEE ALSO
*     gdi/job/job_is_array()
*     gdi/job/job_is_parallel()
*     gdi/job/job_might_be_tight_parallel()
*******************************************************************************/
int job_is_tight_parallel(const lListElem *job, const lList *pe_list)
{
   int ret = 0;
   const char *pe_name = NULL;

   DENTER(TOP_LAYER, "job_is_tight_parallel");
   pe_name = lGetString(job, JB_pe);
   if (pe_name != NULL) {
      int found_pe = 0;
      int all_are_tight = 1;
      lListElem *pe;

      for_each(pe, pe_list) {
         if (pe_is_matching(pe, pe_name)) {
            found_pe = 1;
            all_are_tight &= lGetUlong(pe, PE_control_slaves);
         }
      }
   
      if (found_pe && all_are_tight) {
         ret = 1;
      }
   }
   DEXIT;
   return ret;
}

/****** gdi/job/job_might_be_tight_parallel() *********************************
*  NAME
*     job_might_be_tight_parallel() -- Possibly a tightly integrated job? 
*
*  SYNOPSIS
*     int job_might_be_tight_parallel(const lListElem *job, 
*                                     const lList *pe_list) 
*
*  FUNCTION
*     This functions returns true (1) if "job" might be a tightly 
*     integrated job. True will be returned if (at least one) pe 
*     matching the requested (wildcard) pe of a job has 
*     "contol_slaves=true" in its configuration.
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*     const lList *pe_list - PE_Type list with all existing PEs 
*
*  RESULT
*     int - 1 -> possibly a tightly integrated job
*           0 -> non-parallel or not tightly integrated
*
*  SEE ALSO
*     gdi/job/job_is_array()
*     gdi/job/job_is_parallel()
*     gdi/job/job_is_tight_parallel()
*     gdi/job/job_might_be_tight_parallel()
******************************************************************************/
int job_might_be_tight_parallel(const lListElem *job, const lList *pe_list)
{
   int ret = 0;
   const char *pe_name = NULL;

   DENTER(TOP_LAYER, "job_is_tight_parallel");
   pe_name = lGetString(job, JB_pe);
   if (pe_name != NULL) {
      int found_pe = 0;
      int one_is_tight = 0;
      lListElem *pe;

      DTRACE;

      for_each(pe, pe_list) {
         if (pe_is_matching(pe, pe_name)) {
            found_pe = 1;
            one_is_tight |= lGetUlong(pe, PE_control_slaves);
            DTRACE;
         }
      }
   
      if (found_pe && one_is_tight) {
         DTRACE;
         ret = 1;
      }
   }
   DEXIT;
   return ret;
}

/****** gdi/job/job_get_submit_task_ids() *************************************
*  NAME
*     job_get_submit_task_ids() -- Submit time task specification 
*
*  SYNOPSIS
*     void job_get_submit_task_ids(const lListElem *job, 
*                                  u_long32 *start, 
*                                  u_long32 *end, 
*                                  u_long32 *step) 
*
*  FUNCTION
*     The function returns the "start", "end" and "step" numbers 
*     which where used to create "job" (qsub -t <range>).
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*     u_long32 *start      - first id 
*     u_long32 *end        - last id 
*     u_long32 *step       - step size (>=1)
******************************************************************************/
void job_get_submit_task_ids(const lListElem *job, u_long32 *start, 
                             u_long32 *end, u_long32 *step)
{
   lListElem *range_elem = NULL; /* RN_Type */
 
   range_elem = lFirst(lGetList(job, JB_ja_structure));
   if (range_elem) {
      u_long32 tmp_step;
 
      *start = lGetUlong(range_elem, RN_min);
      *end = lGetUlong(range_elem, RN_max);
      tmp_step = lGetUlong(range_elem, RN_step);
      *step = tmp_step ? tmp_step : 1;
   } else {
      *start = *end = *step = 1;
   }
}      

/****** gdi/job/job_set_submit_task_ids() *************************************
*  NAME
*     job_set_submit_task_ids() -- store the initial range ids in "job"
*
*  SYNOPSIS
*     int job_set_submit_task_ids(lListElem *job, u_long32 start, 
*                                 u_long32 end, u_long32 step) 
*
*  FUNCTION
*     The function stores the initial range id values ("start", "end" 
*     and "step") in "job". It should only be used in functions 
*     initializing new jobs.
*
*  INPUTS
*     lListElem *job - JB_Type job 
*     u_long32 start - first id 
*     u_long32 end   - last id 
*     u_long32 step  - step size 
*
*  RESULT
*     int - 0 -> OK
*           1 -> no memory
******************************************************************************/
int job_set_submit_task_ids(lListElem *job, u_long32 start, u_long32 end,
                            u_long32 step)
{
   lListElem *range_elem;  /* RN_Type */
   int ret = 0;
 
   range_elem = lFirst(lGetList(job, JB_ja_structure));
   if (range_elem == NULL) {
      lList *range_list;
 
      range_elem = lCreateElem(RN_Type);
      range_list = lCreateList("task id range", RN_Type);
      if (range_elem == NULL || range_list == NULL) {
         range_elem = lFreeElem(range_elem);
         range_list = lFreeList(range_list);

         /* No memory */
         ret = 1;
      } else {
         lAppendElem(range_list, range_elem);
         lSetList(job, JB_ja_structure, range_list);
      }
   }
   if (range_elem != NULL) {
      lSetUlong(range_elem, RN_min, start);
      lSetUlong(range_elem, RN_max, end);
      lSetUlong(range_elem, RN_step, step);
   }

   return ret;
}          

/****** gdi/job/job_get_smallest_unenrolled_task_id() *************************
*  NAME
*     job_get_smallest_unenrolled_task_id() -- get smallest id 
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

/****** gdi/job/job_get_smallest_enrolled_task_id() ***************************
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

/****** gdi/job/job_get_biggest_unenrolled_task_id() **************************
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

/****** gdi/job/job_get_biggest_enrolled_task_id() ****************************
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
 
/****** gdi/job/job_list_register_new_job() ***********************************
*  NAME
*     job_list_register_new_job() -- try to register a new job
*
*  SYNOPSIS
*     int job_list_register_new_job(const lList *job_list, 
*                                   u_long32 max_jobs,
*                                   int force_registration)
*
*  FUNCTION
*     This function checks whether a new job would exceed the maximum
*     of allowed jobs per cluster ("max_jobs"). If the limit would be
*     exceeded than the function will return 1 otherwise 0. In some
*     situations it may be necessary to force the registration
*     of a new job (reading jobs from spool area). This may be done
*     with "force_registration".
*
*
*  INPUTS
*     const lListElem *job   - JB_Type element
*     u_long32 max_jobs      - maximum number of allowed jobs per user
*     int force_registration - force job registration
*
*  RESULT
*     int - 1 => limit would be exceeded
*           0 => otherwise
*
*  SEE ALSO
*     gdi/suser/suser_register_new_job()
******************************************************************************/
int job_list_register_new_job(const lList *job_list, u_long32 max_jobs,
                              int force_registration)
{
   int ret = 1;
 
   DENTER(TOP_LAYER, "job_list_register_new_job");
   if (max_jobs > 0 && !force_registration &&
       max_jobs <= lGetNumberOfElem(job_list)) {
      ret = 1;
   } else {
      ret = 0;
   }
   DEXIT;
   return ret;
}                


/****** gdi/job/job_initialize_id_lists() *************************************
*  NAME
*     job_initialize_id_lists() -- initialize task id range lists 
*
*  SYNOPSIS
*     void job_initialize_id_lists(lListElem *job, lList **answer_list) 
*
*  FUNCTION
*     Initialize the task id range lists within "job". All tasks within
*     the JB_ja_structure element of job will be added to the
*     JB_ja_n_h_ids list. All other id lists stored in the "job" will
*     be deleted. 
*
*  INPUTS
*     lListElem *job      - JB_Type element 
*     lList **answer_list - AN_Type list pointer 
*
*  RESULT
*     void - none 
*
*  SEE ALSO
*     gdi/range/RN_Type
*******************************************************************************/
void job_initialize_id_lists(lListElem *job, lList **answer_list)
{
   lList *n_h_list = NULL;    /* RN_Type */

   DENTER(TOP_LAYER, "job_initialize_id_lists");
   n_h_list = lCopyList("", lGetList(job, JB_ja_structure));
   if (n_h_list == NULL) {
      answer_list_add(answer_list, MSG_MEM_MEMORYALLOCFAILED, 
                      STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
   } else {
      lSetList(job, JB_ja_n_h_ids, n_h_list);
      lSetList(job, JB_ja_u_h_ids, NULL);
      lSetList(job, JB_ja_o_h_ids, NULL);
      lSetList(job, JB_ja_s_h_ids, NULL);
   }
   DEXIT;
}

/****** gdi/job/job_initialize_env() ******************************************
*  NAME
*     job_initialize_env() -- initialize environment (partially) 
*
*  SYNOPSIS
*     void job_initialize_env(lListElem *job, lList **answer_list, 
*                             const lList* path_alias_list) 
*
*  FUNCTION
*     Initialize the environment sublist (JB_env_list) of "job".
*     Path aliasing ("path_alias_list") has to be initialized before 
*     this function might be called. 
*
*     Following enironment variables will be added:
*        <VAR_PREFIX>O_HOME
*        <VAR_PREFIX>O_LOGNAME
*        <VAR_PREFIX>O_PATH
*        <VAR_PREFIX>O_SHELL
*        <VAR_PREFIX>O_TZ
*        <VAR_PREFIX>O_HOST
*        <VAR_PREFIX>O_WORKDIR
*        <VAR_PREFIX>O_MAIL
*
*     This function will be used in SGE/EE client applications.
*     Clients do not know which prefix should be used for job
*     environment variables ("SGE_", "GRD_" or "COD_"). Therefore 
*     we use the define <VAR_PREFIX> which will be replaced shortly 
*     before the job is started.
*
*  INPUTS
*     lListElem *job               - JB_Type element 
*     lList **answer_list          - AN_Type list pointer 
*     const lList* path_alias_list - PA_Type list 
*******************************************************************************/
void job_initialize_env(lListElem *job, lList **answer_list, 
                        const lList* path_alias_list)
{
   lList *env_list = NULL;
   DENTER(TOP_LAYER, "job_initialize_env");  
    
   lXchgList(job, JB_env_list, &env_list);
   {   
      int i = -1;
      const char* env_name[] = {"HOME", "LOGNAME", "PATH", 
                                "SHELL", "TZ", "MAIL", NULL};

      while (env_name[++i] != 0) {
         const char *env_value = sge_getenv(env_name[i]);
         char new_env_name[SGE_PATH_MAX];

         sprintf(new_env_name, "%s%s%s", VAR_PREFIX, "O_", env_name[i]);
         var_list_set_string(&env_list, new_env_name, env_value);
      }
   }
   {
      const char* host = sge_getenv("HOST");

      if (host == NULL) {
         host = me.unqualified_hostname;
      }
      var_list_set_string(&env_list, VAR_PREFIX "O_HOST", host);
   } 
   {
      char cwd_out[SGE_PATH_MAX + 1];
      char tmp_str[SGE_PATH_MAX + 1];

      if (!getcwd(tmp_str, sizeof(tmp_str))) {
         answer_list_add(answer_list, MSG_ANSWER_GETCWDFAILED, 
                         STATUS_EDISK, ANSWER_QUALITY_ERROR);
         goto error;
      }
      path_alias_list_get_path(path_alias_list, NULL, 
                               tmp_str, me.qualified_hostname,
                               cwd_out, SGE_PATH_MAX);
      var_list_set_string(&env_list, VAR_PREFIX "O_WORKDIR", 
                                     cwd_out);
   }

error:
   lXchgList(job, JB_env_list, &env_list);
   DEXIT;
}

/****** gdi/job/job_get_env_string() ******************************************
*  NAME
*     job_get_env_string() -- get value of certain job env variable 
*
*  SYNOPSIS
*     const char* job_get_env_string(const lListElem *job, 
*                                    const char *variable) 
*
*  FUNCTION
*     Return the string value of the job environment "variable". 
*
*     Please note: The "*_O_*" env variables get their final
*                  name shortly before job execution. Find more 
*                  information in the ADOC comment of
*                  job_initialize_env()
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*     const char* variable - environment variable name 
*
*  RESULT
*     const char* - value of "variable"
*
*  SEE ALSO
*     gdi/job/job_initialize_env()
*     gdi/job/job_set_env_string() 
******************************************************************************/
const char *job_get_env_string(const lListElem *job, const char *variable)
{
   const char *ret = NULL;
   DENTER(TOP_LAYER, "job_get_env_value");

   ret = var_list_get_string(lGetList(job, JB_env_list), variable);
   DEXIT;
   return ret;
}

/****** gdi/job/job_set_env_string() ******************************************
*  NAME
*     job_set_env_string() -- set value of certain job env variable 
*
*  SYNOPSIS
*     void job_set_env_string(lListElem *job, 
*                             const char* variable, 
*                             const char *value) 
*
*  FUNCTION
*     Set the string "value" of the job environment "variable". 
*
*     Please note: The "*_O_*" env variables get their final
*                  name shortly before job execution. Find more 
*                  information in the ADOC comment of
*                  job_initialize_env()
*
*  INPUTS
*     lListElem *job       - JB_Type element 
*     const char* variable - environment variable name 
*     const char* value    - new value 
*
*  SEE ALSO
*     gdi/job/job_initialize_env()
*     gdi/job/job_get_env_string()
******************************************************************************/
void job_set_env_string(lListElem *job, const char* variable, const char* value)
{
   lList *env_list = NULL;
   DENTER(TOP_LAYER, "job_set_env_value");  

   lXchgList(job, JB_env_list, &env_list);
   var_list_set_string(&env_list, variable, value);
   lXchgList(job, JB_env_list, &env_list);
   DEXIT; 
}

/****** gdi/job/job_check_correct_id_sublists() *******************************
*  NAME
*     job_check_correct_id_sublists() -- test JB_ja_* sublists 
*
*  SYNOPSIS
*     void job_check_correct_id_sublists(lListElem *job, lList **answer_list) 
*
*  FUNCTION
*     Test following elements of "job" whether they are correct:
*        JB_ja_structure, JB_ja_n_h_ids, JB_ja_u_h_ids, 
*        JB_ja_s_h_ids, JB_ja_o_h_ids, JB_ja_z_ids
*     The function will try to correct errors within this lists. If
*     this is not possible an error will be returned in "answer_list".
*      
*
*  INPUTS
*     lListElem *job      - JB_Type element 
*     lList **answer_list - AN_Type list 
*
*  RESULT
*     void - none
*******************************************************************************/
void job_check_correct_id_sublists(lListElem *job, lList **answer_list)
{
   DENTER(TOP_LAYER, "job_check_correct_id_sublists");
   /*
    * Is 0 contained in one of the range lists
    */
   {
      const int field[] = {
         JB_ja_structure,
         JB_ja_n_h_ids,
         JB_ja_u_h_ids,
         JB_ja_s_h_ids,
         JB_ja_o_h_ids,
         JB_ja_z_ids,
         -1
      };
      int i = -1;

      while (field[++i] != -1) {
         lList *range_list = lGetList(job, field[i]);
         lListElem *range = NULL;

         for_each(range, range_list) {
            if (range_is_id_within(range, 0)) {
               ERROR((SGE_EVENT, MSG_JOB_NULLNOTALLOWEDT));
               answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                               ANSWER_QUALITY_ERROR);
               DEXIT;
               return;
            }
            range_correct_end(range);
         }
      }
   }    
 
   /*
    * JB_ja_structure and one of the JB_ja_?_h_ids has
    * to comprise at least one id.
    */
   {
      const int field[] = {
         JB_ja_n_h_ids,
         JB_ja_u_h_ids,
         JB_ja_s_h_ids,
         JB_ja_o_h_ids,
         -1
      };
      int has_structure = 0;
      int has_x_ids = 0;
      int i = -1;

      while (field[++i] != -1) {
         lList *range_list = lGetList(job, field[i]);

         if (!range_list_is_empty(range_list)) {
            has_x_ids = 1;
         }
      }
      has_structure = !range_list_is_empty(lGetList(job, JB_ja_structure));
      if (!has_structure) {
         ERROR((SGE_EVENT, MSG_JOB_NOIDNOTALLOWED));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_ERROR);
         DEXIT;
         return;
      } else if (!has_x_ids) {
         job_initialize_id_lists(job, answer_list);
      }
   }       
   DEXIT; 
}

/****** gdi/job/job_get_id_string() *******************************************
*  NAME
*     job_get_id_string() -- get an id string for a job/jatask/petask
*
*  SYNOPSIS
*     const char* job_get_id_string(u_long32 job_id, u_long32 ja_task_id, 
*                                   const char *pe_task_id) 
*
*  FUNCTION
*     Returns an id string for a certain job, ja task or pe task.
*     The function should be used in any code that outputs ids, e.g. in error
*     strings to ensure we have the same output format everywhere.
*     If the ja_task_id is 0, only the job id is output.
*
*  INPUTS
*     u_long32 job_id        - the job id
*     u_long32 ja_task_id    - the ja task id or 0 to output only job_id
*     const char *pe_task_id - optionally the pe task id
*
*  RESULT
*     const char* - pointer to a static buffer. It is valid until the next
*                   call of the function.
*******************************************************************************/
const char *job_get_id_string(u_long32 job_id, u_long32 ja_task_id, 
                              const char *pe_task_id)
{
   static dstring id = DSTRING_INIT;

   DENTER(TOP_LAYER, "job_get_id_string");

   if(ja_task_id == 0) {
      sge_dstring_sprintf(&id, MSG_JOB_JOB_ID_U, job_id);
   } else {
      if(pe_task_id == NULL) {
         sge_dstring_sprintf(&id, MSG_JOB_JOB_JATASK_ID_UU,
                             job_id, ja_task_id);
      } else {
         sge_dstring_sprintf(&id, MSG_JOB_JOB_JATASK_PETASK_ID_UUS,
                            job_id, ja_task_id, pe_task_id);
      }
   }
   
   DEXIT;
   return sge_dstring_get_string(&id);
}

/****** gdi/job/job_is_pe_referenced() ****************************************
*  NAME
*     job_is_pe_referenced() -- Does job reference the given PE? 
*
*  SYNOPSIS
*     int job_is_pe_referenced(const lListElem *job, 
*                              const lListElem *pe) 
*
*  FUNCTION
*     The function returns true (1) if "job" references the "pe". 
*     This is also the case if job requests a wildcard PE and 
*     the wildcard name matches the given pe name. 
*
*  INPUTS
*     const lListElem *job - JB_Type element 
*     const lListElem *pe  - PE_Type object 
*
*  RESULT
*     int - true (1) or false (0) 
*******************************************************************************/
int job_is_pe_referenced(const lListElem *job, const lListElem *pe)
{
   const char *ref_pe_name = lGetString(job, JB_pe);
   int ret = 0;

   if(ref_pe_name != NULL) {
      if (pe_is_matching(pe, ref_pe_name)) {
         ret = 1;
      }
   }
   return ret;
}

/****** gdi/job/job_is_ckpt_referenced() **************************************
*  NAME
*     job_is_ckpt_referenced() -- Does job reference the given CKPT? 
*
*  SYNOPSIS
*     int job_is_ckpt_referenced(const lListElem *job, 
*                                const lListELem *ckpt) 
*
*  FUNCTION
*     The function returns true (1) if "job" references the 
*     checkpointing object "ckpt". 
*
*  INPUTS
*     const lListElem *job  - JB_Type element 
*     const lListElem *ckpt - CK_Type object 
*
*  RESULT
*     int - true (1) or false (0) 
*******************************************************************************/
int job_is_ckpt_referenced(const lListElem *job, const lListElem *ckpt)
{
   const char *ckpt_name = lGetString(ckpt, CK_name);
   const char *ref_ckpt_name = lGetString(job, JB_checkpoint_object);
   int ret = 0;

   if(ckpt_name != NULL && ref_ckpt_name != NULL) {
      if (!strcmp(ref_ckpt_name, ckpt_name)) {
         ret = 1;
      }
   }
   return ret;
}

/****** gdi/job/job_get_state_string() ****************************************
*  NAME
*     job_get_state_string() -- write job state flags into a string 
*
*  SYNOPSIS
*     void job_get_state_string(char *str, u_long32 op) 
*
*  FUNCTION
*     This function writes the state flags given by 'op' into the 
*     string 'str'
*
*  INPUTS
*     char *str   - containes the state flags for 'qstat'/'qhost' 
*     u_long32 op - job state bitmask 
******************************************************************************/
void job_get_state_string(char *str, u_long32 op)
{
   queue_or_job_get_states(JB_job_number, str, op);
}

/****** gdi/job/job_list_locate() *********************************************
*  NAME
*     job_list_locate() -- find job in a list 
*
*  SYNOPSIS
*     lListElem* job_list_locate(lList *job_list, u_long32 job_id) 
*
*  FUNCTION
*     Returns the job element within "job_list" having "job_id" as
*     primary key. 
*
*  INPUTS
*     lList *job_list - JB_Type list 
*     u_long32 job_id - job id 
*
*  RESULT
*     lListElem* - JB_Type element
******************************************************************************/
lListElem *job_list_locate(lList *job_list, u_long32 job_id) 
{
   lListElem *job = NULL;
   DENTER(BASIS_LAYER, "job_list_locate");

   job = lGetElemUlong(job_list, JB_job_number, job_id);

   DEXIT;
   return job;
}

