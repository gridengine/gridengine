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

#include <fnmatch.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"

#include "config_file.h"

#include "sge_answer.h"
#include "sge_job.h"
#include "sge_queue.h"
#include "sge_userset.h"
#include "sge_utility.h"
#include "sge_pe.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

lList *Master_Pe_List = NULL;

/****** sgeobj/pe/pe_is_matching() ********************************************
*  NAME
*     pe_is_matching() -- Does Pe name match the wildcard? 
*
*  SYNOPSIS
*     bool pe_is_matching(const lListElem *pe, const char *wildcard) 
*
*  FUNCTION
*     The function returns true (1) if the name of the given
*     "pe" matches the "wildcard". 
*
*  INPUTS
*     const lListElem *pe  - PE_Type element 
*     const char *wildcard - wildcard 
*
*  RESULT
*     bool - true or false 
******************************************************************************/
bool pe_is_matching(const lListElem *pe, const char *wildcard) 
{
   return !fnmatch(wildcard, lGetString(pe, PE_name), 0);
}

/****** sgeobj/pe/pe_list_find_matching() *************************************
*  NAME
*     pe_list_find_matching() -- Find a PE matching  wildcard expr 
*
*  SYNOPSIS
*     lListElem* pe_list_find_matching(lList *pe_list, 
*                                      const char *wildcard) 
*
*  FUNCTION
*     Try to find a PE that matches the given "wildcard" expression.
*
*  INPUTS
*     lList *pe_list       - PE_Type list
*     const char *wildcard - Wildcard expression 
*
*  RESULT
*     lListElem* - PE_Type object or NULL
*******************************************************************************/
lListElem *pe_list_find_matching(lList *pe_list, const char *wildcard) 
{
   lListElem *ret = NULL;

   for_each (ret, pe_list) {
      if (pe_is_matching(ret, wildcard)) {
         break;
      }
   }
   return ret;
}

/****** sgeobj/pe/pe_list_locate() ********************************************
*  NAME
*     pe_list_locate() -- Locate a certain PE 
*
*  SYNOPSIS
*     lListElem* pe_list_locate(lList *pe_list, const char *pe_name) 
*
*  FUNCTION
*     Locate the PE with the name "pe_name". 
*
*  INPUTS
*     lList *pe_list      - PE_Type list
*     const char *pe_name - PE name 
*
*  RESULT
*     lListElem* - PE_Type object or NULL
******************************************************************************/
lListElem *pe_list_locate(lList *pe_list, const char *pe_name) 
{
   return lGetElemStr(pe_list, PE_name, pe_name);
}

/****** sgeobj/pe/pe_is_referenced() ******************************************
*  NAME
*     pe_is_referenced() -- Is a given PE referenced in other objects? 
*
*  SYNOPSIS
*     bool pe_is_referenced(const lListElem *pe, lList **answer_list, 
*                           const lList *master_job_list) 
*
*  FUNCTION
*     This function returns true (1) if the given "pe" is referenced
*     in a job contained in "master_job_list". If this is the case than
*     a corresponding message will be added to the "answer_list". 
*
*  INPUTS
*     const lListElem *pe          - PE_Type object 
*     lList **answer_list          - AN_Type list 
*     const lList *master_job_list - JB_Type list 
*
*  RESULT
*     bool - true or false  
******************************************************************************/
bool pe_is_referenced(const lListElem *pe, lList **answer_list,
                      const lList *master_job_list)
{
   lListElem *job = NULL;
   bool ret = false;

   for_each(job, master_job_list) {
      if (job_is_pe_referenced(job, pe)) {
         const char *pe_name = lGetString(pe, PE_name);
         u_long32 job_id = lGetUlong(job, JB_job_number);

         sprintf(SGE_EVENT, MSG_PEREFINJOB_SU, pe_name, u32c(job_id));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_INFO);
         ret = true;
      }
   } 
   return ret;
}

/****** gdi/pe/pe_validate() ***************************************************
*  NAME
*     pe_validate() -- validate a parallel environment
*
*  SYNOPSIS
*     int pe_validate(int startup, lListElem *pep, lList **alpp)
*
*  FUNCTION
*     Ensures that a new pe is not a duplicate of an already existing one
*     and checks consistency of the parallel environment:
*        - pseudo parameters in start and stop proc
*        - validity of the allocation rule
*        - correctness of the queue list, the user list and the xuser list
*
*
*  INPUTS
*     lListElem *pep - the pe to check
*     lList **alpp   - answer list pointer, if an answer shall be created, else
*                      NULL - errors will in any case be output using the
*                      Grid Engine error logging macros.
*     int startup    - are we in qmaster startup phase?
*
*  RESULT
*     int - STATUS_OK, if everything is ok, else other status values,
*           see libs/gdi/sge_answer.h
*
*  NOTES
*     MT-NOTE: pe_validate() is not MT safe
*******************************************************************************/
int pe_validate(lListElem *pep, lList **alpp, int startup)
{
   const char *s;
   const char *pe_name;
   int ret;

   DENTER(TOP_LAYER, "pe_validate");
   pe_name = lGetString(pep, PE_name);
   if (pe_name && verify_str_key(alpp, pe_name, MSG_OBJ_PE)) {
      ERROR((SGE_EVENT, "Invalid character in pe name of pe "SFQ, pe_name));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* register our error function for use in replace_params() */
   config_errfunc = set_error;

   /* -------- start_proc_args */
   s = lGetString(pep, PE_start_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STARTPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }


   /* -------- stop_proc_args */
   s = lGetString(pep, PE_stop_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STOPPROCARGS_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- allocation_rule */
   s = lGetString(pep, PE_allocation_rule);
   if (!s)  {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_allocation_rule), "validate_pe"));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
      ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- PE_queue_list */
   if ((ret=queue_reference_list_validate(alpp, lGetList(pep, PE_queue_list), MSG_OBJ_QLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK && !startup) {
      DEXIT;
      return ret;
   }

   /* -------- PE_user_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_user_list), MSG_OBJ_USERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   /* -------- PE_xuser_list */
   if ((ret=userset_list_validate_acl_list(alpp, lGetList(pep, PE_xuser_list), MSG_OBJ_XUSERLIST,
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return STATUS_OK;
}

