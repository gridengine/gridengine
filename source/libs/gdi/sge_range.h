#ifndef __SGE_RANGE_H
#define __SGE_RANGE_H
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

#include "sge_string_append.h"
#include "sge_rangeL.h"

#define MAX_IDS_PER_LINE  8
#define MAX_LINE_LEN      70

#if 0

/*
   for_each(range, range_list) { 
      for(id = lGetUlong(range, RN_min); 
          id <= lGetUlong(range, RN_max); 
          id += lGetUlong(range, RN_step)) {

*/

#define for_each_id_in_range_list(id, elem, list) \
   if (list) \
      for (id = ((elem = lFirst(list)) ? lGetUlong(elem, RN_min) : 0); \
           (elem ? (id <= lGetUlong(elem, RN_max)) : 0); \
           (elem ? (id += lGetUlong(elem, RN_step)) : \
              ((elem = lNext(elem)) ? (id = lGetUlong(elem, RN_min)) : 0)))

#endif

void get_taskrange_str(lList *task_list, StringBufferT *taskrange_str);

lList* split_task_group(lList **in_list);

/* *** */

void range_list_calculate_union_set(lList **range_list, lList **answer_list,
                                    const lList *range_list1, 
                                    const lList *range_list2);

void range_calculate_difference_set(lList **range_list, lList **answer_list,
                                    const lList *range_list1,
                                    const lList *range_list2);  

void range_calculate_intersection_set(lList **range_list, lList **answer_list,
                                      const lList *range_list1,
                                      const lList *range_list2); 

void range_get_all_ids(const lListElem *range_elem, u_long32 *min, 
                       u_long32 *max, u_long32 *step);

void range_set_all_ids(lListElem *range_elem, u_long32 min, u_long32 max,
                       u_long32 step);

void range_print_to_string(const lList *range_list, StringBufferT *string);

void range_sort_uniq_compress(lList *range_list, lList **answer_list);  

void range_list_insert_id(lList **range_list, lList **answer_list, u_long32 id);
 
void range_list_remove_id(lList **range_list, lList **answer_list, u_long32 id);

void range_list_move_first_n_ids(lList **range_list, lList **answer_list,
                                 lList **range_list2, u_long32 n);

typedef void (*range_remove_insert_t)(lList**, lList**, u_long32);
 
int range_is_id_within(const lListElem *range, u_long32 id);   

int range_list_is_id_within(const lList *range_list, u_long32 id);   

void range_compress(lList *range_list);    

u_long32 range_list_get_first_id(const lList *range_list, lList **answer_list);

u_long32 range_list_get_last_id(const lList *range_list, lList **answer_list);

void range_list_initialize(lList **range_list, lList **answer_list);

u_long32 range_list_get_number_of_ids(const lList *range_list);
 
u_long32 range_get_number_of_ids(const lListElem *range);

#endif /* __SGE_RANGE_H */
