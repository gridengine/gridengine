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

#include "sge_range.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"

#include "sge_job.h"

#include "sge_usageL.h"

#include "msg_sgeobjlib.h"

/****** gdi/ja_task/ja_task_search_pe_task()***********************************
*  NAME
*     ja_task_search_pe_task() -- Find a certain PE Task 
*
*  SYNOPSIS
*     lListElem* ja_task_search_pe_task(const lListElem *ja_task, 
*                                       const char **pe_task_id) 
*
*  FUNCTION
*     Find a certain PE Task with "pe_task_id" in "ja_task" 
*
*  INPUTS
*     const lListElem *ja_task - JAT_Type element 
*     const char *pe_task_id   - PE task id string (e.g. "1.speedy") 
*
*  RESULT
*     lListElem* - PET_Type
*******************************************************************************/
lListElem *ja_task_search_pe_task(const lListElem *ja_task, 
                                  const char *pe_task_id)
{
   if(ja_task != NULL) {
      lList *pe_tasks = lGetList(ja_task, JAT_task_list);

      if(pe_tasks != NULL) {
         return lGetElemStr(pe_tasks, PET_id, pe_task_id);
      }
   }

   return NULL;
}

/****** gdi/ja_task/ja_task_list_print_to_string() ****************************
*  NAME
*     ja_task_list_print_to_string() -- print task id ranges into string 
*
*  SYNOPSIS
*     void ja_task_list_print_to_string(const lList *ja_task_list, 
*                                       dstring *range_string) 
*
*  FUNCTION
*     The ids of all tasks contained in 'ja_task_list' will be printed
*     into 'range_string'. 
*
*  INPUTS
*     const lList *ja_task_list - JAT_Type list 
*     dstring *range_string     - dynamic string 
******************************************************************************/
void ja_task_list_print_to_string(const lList *ja_task_list, 
                                  dstring *range_string)
{
   lListElem *ja_task = NULL;    /* JAT_Type */
   lList *range_list = NULL;     /* RN_Type */

   DENTER(TOP_LAYER, "ja_task_list_print_to_string");
   for_each(ja_task, ja_task_list) {
      u_long32 tid = lGetUlong(ja_task, JAT_task_number);

      range_list_insert_id(&range_list, NULL, tid);      
   } 
   range_sort_uniq_compress(range_list, NULL); 
   range_list_print_to_string(range_list, range_string, 0); 
   range_list = lFreeList(range_list);
   DEXIT;
}

/****** gdi/ja_task/ja_task_list_split_group() *****************************
*  NAME
*     ja_task_list_split_group() -- Splits a list into two parts
*
*  SYNOPSIS
*     lList* ja_task_list_split_group(lList **ja_task_list)
*
*  FUNCTION
*     All tasks which have the same state (JAT_status, JAT_state) like
*     the first element of 'ja_task_list' will be removed from i
*     'ja_task_list' and returned by this function.
*
*  INPUTS
*     lList **ja_task_list - JAT_Type list
*
*  RESULT
*     lList* - JAT_Type list (elements with equivalent state)
*
*  SEE ALSO
*     gdi/range/RN_Type
******************************************************************************/
lList* ja_task_list_split_group(lList **ja_task_list)
{
   lList *ret_list = NULL;

   if (ja_task_list && *ja_task_list) {
      lListElem *first_task = lFirst(*ja_task_list);

      if (first_task) {
         u_long32 status = lGetUlong(first_task, JAT_status); 
         u_long32 state = lGetUlong(first_task, JAT_state); 
         u_long32 hold = lGetUlong(first_task, JAT_hold);
         lCondition *where = NULL;

         where = lWhere("%T(%I != %u || %I != %u || %I != %u)", JAT_Type,
                        JAT_status, status, JAT_state, state,
                        JAT_hold, hold);
         lSplit(ja_task_list, &ret_list, NULL, where);
         where = lFreeWhere(where);
      }
   }
   return ret_list;
}                     

bool 
ja_task_add_finished_pe_task(lListElem *ja_task, const char *pe_task_id)
{
   lListElem *pe_task;

   DENTER(TOP_LAYER, "ja_task_add_finished_pe_task");

   pe_task = lGetSubStr(ja_task, FPET_id, pe_task_id, JAT_finished_task_list);
   if (pe_task != NULL) {
      DPRINTF(("already handled exit of pe task "SFQ" in ja_task "U32CFormat
               "\n", pe_task_id, lGetUlong(ja_task, JAT_task_number)));
      DEXIT;
      return false;
   }

   pe_task = lAddSubStr(ja_task, FPET_id, pe_task_id, JAT_finished_task_list, 
                        FPET_Type);

   DEXIT;
   return true;
}
