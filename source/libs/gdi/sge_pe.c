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

#include "sge_answer.h"
#include "sge_job.h"
#include "sge_pe.h"

#include "msg_gdilib.h"

lList *Master_Pe_List = NULL;

/****** gdi/pe/pe_is_matching() ***********************************************
*  NAME
*     pe_is_matching() -- Does Pe name match the wildcard? 
*
*  SYNOPSIS
*     int pe_is_matching(const lListElem *pe, const char *wildcard) 
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
*     int - true (1) or false (0)
******************************************************************************/
int pe_is_matching(const lListElem *pe, const char *wildcard) 
{
   return !fnmatch(wildcard, lGetString(pe, PE_name), 0);
}

/****** gdi/pe/pe_list_find_matching() ****************************************
*  NAME
*     pe_list_find_matching() -- Find a PE matching a wildcard expression 
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

/****** gdi/pe/pe_list_locate() ***********************************************
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
*******************************************************************************/
lListElem *pe_list_locate(lList *pe_list, const char *pe_name) 
{
   return lGetElemStr(pe_list, PE_name, pe_name);
}

/****** gdi/pe/pe_is_referenced() **********************************************
*  NAME
*     pe_is_referenced() -- Is a given PE referenced in other objects? 
*
*  SYNOPSIS
*     int pe_is_referenced(const lListElem *pe, lList **answer_list, 
*                          const lList *master_job_list) 
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
*     int - true (1) or false (0) 
******************************************************************************/
int pe_is_referenced(const lListElem *pe, lList **answer_list,
                     const lList *master_job_list)
{
   lListElem *job = NULL;
   int ret = 0;

   for_each(job, master_job_list) {
      if (job_is_pe_referenced(job, pe)) {
         const char *pe_name = lGetString(pe, PE_name);
         u_long32 job_id = lGetUlong(job, JB_job_number);

         sprintf(SGE_EVENT, MSG_PEREFINJOB_SU, pe_name, u32c(job_id));
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_INFO);
         ret = 1;
      }
   } 
   return ret;
}

/****** gdi/pe/pe_update_master_list() *****************************
*  NAME
*     pe_update_master_list() -- update the master list of parallel environments
*
*  SYNOPSIS
*     int pe_update_master_list(sge_event_type type, 
*                                     sge_event_action action, 
*                                     lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of parallel environments
*     based on an event.
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
int pe_update_master_list(sge_event_type type, sge_event_action action, 
                          lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "pe_update_master_list");

   list = &Master_Pe_List;
   list_descr = PE_Type;
   key_nm = PE_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}
