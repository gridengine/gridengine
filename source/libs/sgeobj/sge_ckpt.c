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

#include <strings.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"

#include "config_file.h"
#include "sge_signal.h"

#include "gdi_utility.h"
#include "sge_answer.h"
#include "sge_job.h"
#include "sge_queue.h"
#include "sge_utility.h"
#include "sge_ckpt.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

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


