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

#include "sge.h"
#include "sgermon.h"

#include "sge_log.h"

#include "sge_job.h"
#include "sge_ja_task.h"

#include "sge_mirror.h"

#include "msg_sgeobjlib.h"

#include "sge_ja_task_mirror.h"

/****** gdi/ja_task/ja_task_update_master_list_usage() *************************
*  NAME
*     ja_task_update_master_list_usage() -- update an array tasks usage
*
*  SYNOPSIS
*     int ja_task_update_master_list_usage(lListElem *event)
*
*  FUNCTION
*     Updates the scaled usage of an array task (also task data structure
*     of a non array job).
*
*  INPUTS
*     lListElem *event - event object containing the new usage list
*
*  RESULT
*     int - TRUE, if the operation succeeds, else FALSE
*
*  SEE ALSO
*     gdi/job/job_update_master_list_usage()
*     gdi/ja_task/pe_task_update_master_list_usage()
*******************************************************************************/
int ja_task_update_master_list_usage(lListElem *event)
{
   lList *tmp = NULL;
   u_long32 job_id, ja_task_id;
   lListElem *job, *ja_task;

   DENTER(TOP_LAYER, "ja_task_update_master_list_usage");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);

   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
             job_get_id_string(job_id, 0, NULL), "ja_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
             job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   lXchgList(event, ET_new_version, &tmp);
   lXchgList(ja_task, JAT_scaled_usage_list, &tmp);
   lXchgList(event, ET_new_version, &tmp);

   DEXIT;
   return TRUE;
}

/****** gdi/ja_task/ja_task_update_master_list() *****************************
*  NAME
*     ja_task_update_master_list() -- update array tasks of a job
*
*  SYNOPSIS
*     int ja_task_update_master_list(sge_event_type type,
*                                    sge_event_action action,
*                                    lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the list of array tasks of a job
*     based on an event.
*     The function is called from the event mirroring interface.
*
*     An array tasks list of parallel tasks and the
*     scaled usage list are not updated in this function,
*     as this data is maintained by separate events.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*******************************************************************************/
int ja_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata)
{
   u_long32 job_id, ja_task_id;
   lListElem *job, *ja_task;

   lList *list;
   lDescr *list_descr;

   lList *pe_tasks = NULL;
   lList *usage = NULL;

   DENTER(TOP_LAYER, "ja_task_update_master_list");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);

   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS,
             job_get_id_string(job_id, 0, NULL), "ja_task_update_master_list"));
      DEXIT;
      return FALSE;
   }

   ja_task = job_search_task(job, NULL, ja_task_id);

   list = lGetList(job, JB_ja_tasks);
   list_descr = JAT_Type;

   if(action == SGE_EMA_MOD) {
      /* modify event for ja_task.
       * we may not update
       * - JAT_task_list - it is maintained by PETASK events
       * - JAT_scaled_usage - it is maintained by JOB_USAGE events
       */
      if(ja_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
                job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list"));
         DEXIT;
         return FALSE;
      }

      lXchgList(ja_task, JAT_task_list, &pe_tasks);
      lXchgList(ja_task, JAT_scaled_usage_list, &usage);
   }

   /* If an array job is deleted, a delete event is sent for
    * each ja_task. If it is not yet enrolled,
    * sge_mirror_update_master_list will fail.
    * If it is not enrolled, but in the range list
    * for pending tasks, remove it from ranges.
    */
   if(action == SGE_EMA_DEL) {
      if(ja_task == NULL &&
         job_is_ja_task_defined(job, ja_task_id) &&
         (!job_is_enrolled(job, ja_task_id))
        ) {
         job_delete_not_enrolled_ja_task(job, NULL, ja_task_id);
         DEXIT;
         return TRUE;
      }
   }

   if(sge_mirror_update_master_list(&list, list_descr, ja_task, job_get_id_string(job_id, ja_task_id, NULL), action, event) != SGE_EM_OK) {
      lFreeList(pe_tasks);
      lFreeList(usage);
      DEXIT;
      return FALSE;
   }

   /* restore pe_task list after modify event */
   if(action == SGE_EMA_MOD) {
      /* we have to search the replaced ja_task */
      ja_task = job_search_task(job, NULL, ja_task_id);
      if(ja_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS,
                job_get_id_string(job_id, ja_task_id, NULL), "ja_task_update_master_list"));
         lFreeList(pe_tasks);
         lFreeList(usage);
         DEXIT;
         return FALSE;
      }

      lXchgList(ja_task, JAT_task_list, &pe_tasks);
      lXchgList(ja_task, JAT_scaled_usage_list, &usage);
      pe_tasks = lFreeList(pe_tasks);
      usage = lFreeList(usage);
   }

   if(action == SGE_EMA_ADD) {
      /* first jatask add event could have created new ja_task list for job */
      if(lGetList(job, JB_ja_tasks) == NULL && list != NULL) {
         lSetList(job, JB_ja_tasks, list);
      }
      /* we must enroll the task to have it removed in the pending range list */
      job_enroll(job, NULL, ja_task_id);
   }

   DEXIT;
   return TRUE;
}
