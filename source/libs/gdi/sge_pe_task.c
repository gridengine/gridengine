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

#include "sge_string.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"

#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"

#include "sge_usageL.h"

#include "msg_gdilib.h"

/****** gdi/pe_task/pe_task_sum_past_usage() *******************************
*  NAME
*     pe_task_sum_past_usage() -- sum up pe tasks past usage
*
*  SYNOPSIS
*     lListElem* pe_task_sum_past_usage(lListElem *container, 
*                                       const lListElem *pe_task) 
*
*  FUNCTION
*     The pe task list of a ja task can contain one container element to hold
*     the usage of finished pe tasks no longer stored in the task list.
*
*     This function adds the usage of pe_task to the usage of container.
*
*  INPUTS
*     lListElem *container     - container object to hold past usage
*     const lListElem *pe_task - the pe task from which to copy usage
*
*  RESULT
*     lListElem* - the container object
*
*  NOTES
*     JG: TODO: The code summing up usage should better be in a module
*               libs/gdi/sge_usage.* containing all usage related functions
*               as the same or similar functionality is needed in many places
*               in Grid Engine.
*
*  SEE ALSO
*     gdi/pe_task/pe_task_sum_past_usage_all()
*     gdi/pe_task/pe_task_sum_past_usage_list()
*
*******************************************************************************/
lListElem *pe_task_sum_past_usage(lListElem *container, const lListElem *pe_task)
{
   lList *container_usage;
   const lList *pe_task_usage;
   const lListElem *usage;
   
   DENTER(TOP_LAYER, "pe_task_sum_past_usage");

   /* invalid input - nothing to do */
   if(container == NULL || pe_task == NULL) {
      DEXIT;
      return NULL;
   }

   /* container and pe_task are the same element - nothing to do */
   if(container == pe_task) {
      DEXIT;
      return container;
   }

   container_usage = lGetList(container, PET_scaled_usage);
   pe_task_usage   = lGetList(pe_task,   PET_scaled_usage);

   /* empty usage in pe task - nothing to do */
   if(pe_task_usage == NULL) {
      DEXIT;
      return container;
   }

   /* container has no usage list yet - create it */
   if(container_usage == NULL) {
      container_usage = lCreateList("usage", UA_Type);
      lSetList(container, PET_scaled_usage, container_usage);
   }

   /* sum up usage */
   for_each(usage, pe_task_usage) {
      const char *name = lGetString(usage, UA_name);
      if (!strcmp(name, USAGE_ATTR_CPU) ||
          !strcmp(name, USAGE_ATTR_IO)  ||
          !strcmp(name, USAGE_ATTR_IOW) ||
          !strcmp(name, USAGE_ATTR_VMEM) ||
          !strcmp(name, USAGE_ATTR_MAXVMEM) ||
          !strcmp(name, USAGE_ATTR_MEM)) {
         lListElem *sum = lGetElemStr(container_usage, UA_name, name);
         if(sum == NULL) {
            lAppendElem(container_usage, lCopyElem(usage));
         } else {
            lSetDouble(sum, UA_value, lGetDouble(sum, UA_value) + lGetDouble(usage, UA_value));
         }
      }   
   }

   DEXIT;
   return container;
}

/****** gdi/pe_task/pe_task_sum_past_usage_all() *******************************
*  NAME
*     pe_task_sum_past_usage_all() -- sum up all pe tasks past usage
*
*  SYNOPSIS
*     lListElem* pe_task_sum_past_usage_all(lList *pe_task_list)
*
*  FUNCTION
*     Similar to pe_task_sum_past_usage, but will sum up the usage of all
*     pe tasks in a task list to the container object in this list.
*
*     If the container object does not yet exist, it will be created.
*
*  INPUTS
*     lList *pe_task_list - the pe task list to process
*
*  RESULT
*     lListElem* - the container object
*
*  SEE ALSO
*     gdi/pe_task/pe_task_sum_past_usage()
*     gdi/pe_task/pe_task_sum_past_usage_list()
*******************************************************************************/
lListElem *pe_task_sum_past_usage_all(lList *pe_task_list)
{
   lListElem *container = NULL;
   const lListElem *pe_task;

   DENTER(TOP_LAYER, "pe_task_sum_past_usage_all");

   /* no pe task list - nothing to do */
   if(pe_task_list == NULL) {
      DEXIT;
      return NULL;
   }

   /* loop over all pe tasks and sum up usage */
   for_each(pe_task, pe_task_list) {
      if(container == NULL) {
         container = pe_task_sum_past_usage_list(pe_task_list, pe_task);
      } else {
         pe_task_sum_past_usage(container, pe_task);
      }
   }

   DEXIT;
   return container;
}

/****** gdi/pe_task/pe_task_sum_past_usage_list() *******************************
*  NAME
*     pe_task_sum_past_usage_list() -- sum up pe tasks past usage
*
*  SYNOPSIS
*     lListElem* pe_task_sum_past_usage_list(lList *pe_task_list, 
*                                            const lListElem *pe_task) 
*
*  FUNCTION
*     Similar to pe_task_sum_past_usage.
*     The container is retrieved from pe_task_list, if it does not yet
*     exist it is created and inserted into pe_task_list as first element.
*
*  INPUTS
*     lList *pe_task_list      - list containing the container object
*     const lListElem *pe_task - the pe task from which to copy usage
*
*  RESULT
*     lListElem* - the container object
*
*  SEE ALSO
*     gdi/pe_task/pe_task_sum_past_usage()
*     gdi/pe_task/pe_task_sum_past_usage_all()
*******************************************************************************/
lListElem *pe_task_sum_past_usage_list(lList *pe_task_list, const lListElem *pe_task)
{
   lListElem *container;

   DENTER(TOP_LAYER, "pe_task_sum_past_usage_list");

   /* no pe task list - nothing to do */
   if(pe_task_list == NULL) {
      DEXIT;
      return NULL;
   }

   /* get container - if it does not yet exist, create it as first element in pe task list */
   container = lGetElemStr(pe_task_list, PET_id, PE_TASK_PAST_USAGE_CONTAINER);
   if(container == NULL) {
      container = lCreateElem(PET_Type);
      lSetString(container, PET_id, PE_TASK_PAST_USAGE_CONTAINER);
      lInsertElem(pe_task_list, NULL, container);
   }

   /* sum up usage */
   pe_task_sum_past_usage(container, pe_task);

   DEXIT;
   return container;
}

/****** gdi/pe_task/pe_task_update_master_list_usage() *************************
*  NAME
*     pe_task_update_master_list_usage() -- update a parallel tasks usage
*
*  SYNOPSIS
*     int pe_task_update_master_list_usage(lListElem *event) 
*
*  FUNCTION
*     Updates the scaled usage of a parallel task.
*
*  INPUTS
*     lListElem *event - event object containing the new usage list
*
*  RESULT
*     int - TRUE, if the operation succeeds, else FALSE
*
*  SEE ALSO
*     gdi/job/job_update_master_list_usage()
*     gdi/ja_task/ja_task_update_master_list_usage()
*******************************************************************************/
int pe_task_update_master_list_usage(lListElem *event)
{
   lList *tmp;
   u_long32 job_id, ja_task_id;
   const char *pe_task_id;
   lListElem *job, *ja_task, *pe_task;

   DENTER(TOP_LAYER, "pe_task_update_master_list_usage");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);
   pe_task_id = lGetString(event, ET_strkey);
   
   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS, 
             job_get_id_string(job_id, 0, NULL), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }
   
   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, NULL), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   pe_task = ja_task_search_pe_task(ja_task, pe_task_id);
   if(pe_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, pe_task_id), "pe_task_update_master_list_usage"));
      DEXIT;
      return FALSE;
   }

   lXchgList(event, ET_new_version, &tmp);
   lXchgList(pe_task, PET_scaled_usage, &tmp);
   lXchgList(event, ET_new_version, &tmp);
   
   DEXIT;
   return TRUE;
}

/****** gdi/pe_task/pe_task_update_master_list() *****************************
*  NAME
*     pe_task_update_master_list() -- update parallel tasks of an array task
*
*  SYNOPSIS
*     int pe_task_update_master_list(sge_event_type type, 
*                                    sge_event_action action, 
*                                    lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the list of parallel tasks of an array task
*     based on an event.
*     The function is called from the event mirroring interface.
*
*     The scaled usage list of a parallel task is not updated
*     by this function, as this data is maintained by a 
*     separate event.
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
int pe_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata)
{
   u_long32 job_id, ja_task_id;
   const char *pe_task_id;
   
   lListElem *job, *ja_task, *pe_task;

   lList *list;
   lDescr *list_descr;

   lList *usage = NULL;

   DENTER(TOP_LAYER, "pe_task_update_master_list");

   job_id = lGetUlong(event, ET_intkey);
   ja_task_id = lGetUlong(event, ET_intkey2);
   pe_task_id = lGetString(event, ET_strkey);
   
   job = job_list_locate(Master_Job_List, job_id);
   if(job == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJOBFORUPDATEIN_SS, 
             job_get_id_string(job_id, 0, NULL), "pe_task_update_master_list"));
      DEXIT;
      return FALSE;
   }
   
   ja_task = job_search_task(job, NULL, ja_task_id);
   if(ja_task == NULL) {
      ERROR((SGE_EVENT, MSG_JOB_CANTFINDJATASKFORUPDATEIN_SS, 
             job_get_id_string(job_id, ja_task_id, NULL), "pe_task_update_master_list"));
      DEXIT;
      return FALSE;
   }
   
   pe_task = ja_task_search_pe_task(ja_task, pe_task_id);

   list = lGetList(ja_task, JAT_task_list);
   list_descr = PET_Type;
   
   if(action == SGE_EMA_MOD) {
      /* modify event for pe_task.
       * we may not update
       * - PET_scaled_usage - it is maintained by JOB_USAGE events
       */
      if(pe_task == NULL) {
         ERROR((SGE_EVENT, MSG_JOB_CANTFINDPETASKFORUPDATEIN_SS, 
                job_get_id_string(job_id, ja_task_id, pe_task_id), "pe_task_update_master_list"));
         DEXIT;
         return FALSE;
      }
      lXchgList(pe_task, PET_scaled_usage, &usage);
   }
 
   if(sge_mirror_update_master_list(&list, list_descr, pe_task, job_get_id_string(job_id, ja_task_id, pe_task_id), action, event) != SGE_EM_OK) {
      lFreeList(usage);
      DEXIT;
      return FALSE;
   }

   /* restore pe_task list after modify event */
   if(action == SGE_EMA_MOD) {
      lXchgList(pe_task, PET_scaled_usage, &usage);
   }

   /* first petask add event could have created new pe_task list for job */
   if(lGetList(ja_task, JAT_task_list) == NULL && list != NULL) {
      lSetList(ja_task, JAT_task_list, list);
   }

   DEXIT;
   return TRUE;
}

