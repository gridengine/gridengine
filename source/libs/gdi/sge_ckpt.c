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
#include "sge_log.h"
#include "cull_list.h"

#include "sge_answer.h"
#include "sge_job.h"
#include "sge_ckpt.h"

#include "msg_gdilib.h"

lList *Master_Ckpt_List = NULL;

/****** gdi/ckpt/ckpt_is_referenced() *****************************************
*  NAME
*     ckpt_is_referenced() -- Is a given CKPT referenced in other objects? 
*
*  SYNOPSIS
*     int ckpt_is_referenced(const lListElem *ckpt, lList **answer_list, 
*                            const lList *job_list) 
*
*  FUNCTION
*     This function returns true (1) if the given "ckpt" is referenced
*     in a job contained in "job_list". If this is the case than
*     a corresponding message will be added to the "answer_list". 
*
*  INPUTS
*     const lListElem *ckpt - CK_Type object 
*     lList **answer_list   - AN_Type list 
*     const lList *job_list - JB_Type list 
*
*  RESULT
*     int - true (1) or false (0) 
******************************************************************************/
int ckpt_is_referenced(const lListElem *ckpt, lList **answer_list,
                       const lList *job_list)
{
   lListElem *job = NULL;
   int ret = 0;

   for_each(job, job_list) {
      if (job_is_ckpt_referenced(job, ckpt)) {
         const char *ckpt_name = lGetString(ckpt, CK_name);
         u_long32 job_id = lGetUlong(job, JB_job_number);

         sprintf(SGE_EVENT, MSG_CKPTREFINJOB_SU, ckpt_name, u32c(job_id));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_INFO);
         ret = 1;
      }
   } 
   return ret;
}

/****** gdi/ckpt/ckpt_list_locate() *******************************************
*  NAME
*     ckpt_list_locate -- find a ckpt object in a list 
*
*  SYNOPSIS
*     lListElem *ckpt_list_locate(lList *ckpt_list, const char *ckpt_name)
*
*  FUNCTION
*     This function will return a ckpt object by name if it exists.
*
*
*  INPUTS
*     lList *ckpt_list      - CK_Type object
*     const char *ckpt_name - name of the ckpt object. 
*
*  RESULT
*     NULL - ckpt object with name "ckpt_name" does not exist
*     !NULL - pointer to the cull element (CK_Type) 
******************************************************************************/
lListElem *ckpt_list_locate(lList *ckpt_list, const char *ckpt_name)
{
   return lGetElemStr(ckpt_list, CK_name, ckpt_name);
}

/****** gdi/ckpt/ckpt_update_master_list() *****************************
*  NAME
*     ckpt_update_master_list() -- update the master list of checkpoint environments
*
*  SYNOPSIS
*     int ckpt_update_master_list(sge_event_type type, 
*                                 sge_event_action action, 
*                                 lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of checkpoint environments based on an
*     event.
*     The function is called from the event mirroring interface.
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
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int ckpt_update_master_list(sge_event_type type, sge_event_action action, 
                            lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "ckpt_update_master_list");

   list = &Master_Ckpt_List;
   list_descr = CK_Type;
   key_nm = CK_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}
