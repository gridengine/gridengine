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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>  

#include "sgermon.h"
#include "def.h"
#include "symbols.h"
#include "sge.h"
#include "sge_time.h"
#include "sge_exit.h"
#include "sge_log.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "complex.h"
#include "slots_used.h"
#include "sge_resource.h"
#include "sge_jobL.h"
#include "sge_complexL.h"
#include "sge_sched.h"
#include "cull_sort.h"
#include "usage.h"
#include "parse.h"
#include "parse_range.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "utility.h"
#include "sge_parse_num_par.h"
#include "sge_string.h"
#include "show_job.h"
#include "sge_string_append.h"
#include "sge_range.h"
#include "sge_schedd_text.h"
#include "job.h"

#if 0
   for (elem = lFirst(list);
        elem;
        elem = lNext(elem)) {
      for(id = lGetUlong(elem, RN_start); 
          id <= lGetUlong(elem, RN_end); 
          id += lGetUlong(elem, RN_step)) {
      }
   }
#endif

static void add_taskrange_str(u_long32 start, u_long32 end, int step, StringBufferT *dyn_taskrange_str);

static void range_correct_end(lListElem *range_elem);
 
static int range_is_overlapping(lListElem *range_elem1,
                                lListElem *range_elem2);
 
void get_taskrange_str(lList* task_list, StringBufferT *dyn_taskrange_str) {
   lListElem *jatep, *nxt_jatep;
   u_long32 before_last_id = (u_long32)-1;
   u_long32 last_id = (u_long32)-1;
   u_long32 id = (u_long32)-1;
   int diff = -1;      
   int last_diff = -1;      
   u_long32 end = (u_long32)-1;
   u_long32 start = (u_long32)-1;
   u_long32 step = (u_long32)-1;
   int state = 1;
   int counter = 0;
   int new_start=0;

   lPSortList(task_list, "%I+", JAT_task_number);

   nxt_jatep = lFirst(task_list); 
   while((jatep=nxt_jatep)) {
      if (nxt_jatep)
         nxt_jatep = lNext(nxt_jatep);

      if (last_id!=-1)
         before_last_id = last_id;
      if (id!=-1)
         last_id = id;
      if (jatep)
         id = lGetUlong(jatep, JAT_task_number);
      if (diff)
         last_diff = diff;
      if ((last_id != -1) && (id != -1))
         diff = id - last_id;

      if (last_diff != diff && !new_start) {
         if (state == 1) {
            state = 2;
            start = last_id;
            counter = 0;
         } else if (state == 2) {
            end = last_id;
            step = (start!=end)?last_diff:0;
            counter = 0;
            new_start = 1;
            add_taskrange_str(start, end, step, dyn_taskrange_str); 
#if 0
            fprintf (stderr, "=>start: %ld, end: %ld, step: %ld\n", start, end, step);
#endif

            start = id;
         }
      } else
         new_start = 0;
      counter++;
#if 0
      fprintf(stderr, "b-id: %+ld l-id: %+ld id: %+ld ld: %+d, d: %+d st: %+d \t start: %ld end: %ld c: %d\n", 
         before_last_id, last_id, id, last_diff, diff, state, start, end, counter);
#endif
   }

   if (before_last_id==-1 && last_id==-1 && id!=-1) {
      start = end = id;
      step = 0;
      add_taskrange_str(start, end, step, dyn_taskrange_str); 
   } else if (before_last_id==-1 && last_id!=-1 && id!=-1) {
      start = last_id;
      end = id;
      step = end-start;
      add_taskrange_str(start, end, step, dyn_taskrange_str); 
   } else if (before_last_id!=-1 && last_id!=-1 && id!=-1) {
      if (last_diff != diff) {
         if (counter == 1) {
            end = id;
            step = diff;
#if 0
            fprintf (stderr, "1 -> start: %ld, end: %ld, step: %ld\n", (long) start, (long)end, (long)step);
#endif
         } else {
            end = id;
            step = diff;
#if 0
            fprintf (stderr, "2 -> start: %ld, end: %ld, step: %ld\n", (long) start, (long)end, (long)step);
#endif
         }  
      } else {
         end = id;
         step = diff;
#if 0
         fprintf (stderr, "3 -> start: %ld, end: %ld, step: %ld\n", (long) start, (long)end, (long)step);
#endif
      }
      add_taskrange_str(start, end, step, dyn_taskrange_str); 
   }
#if 0
   fprintf (stderr, "=>start: %ld, end: %ld, step: %ld\n", start, end, step);
   fprintf(stderr, "String: %s\n", dyn_taskrange_str->s);
#endif
} 

static void add_taskrange_str(
u_long32 start,
u_long32 end,
int step,
StringBufferT *dyn_taskrange_str 
) {
   char tail[256]="";

   if (dyn_taskrange_str->size > 0) {
      sge_string_append(dyn_taskrange_str, ",");
   }

   if (start == end)
      sprintf(tail, u32, start);
   else if (start+step == end)
      sprintf(tail, u32","u32, start, end);
   else {
      sprintf(tail, u32"-"u32":%d", start, end, step); 
   }
   sge_string_append(dyn_taskrange_str, tail);
}

lList* split_task_group(
lList **in_list 
) {
   lCondition *where = NULL;
   lList *out_list = NULL;
/*    lListElem *jatep = NULL, *nxt_jatep = NULL; */
   u_long32 status = 0, state = 0;

   if (in_list && *in_list) {
      status = lGetUlong(lFirst(*in_list), JAT_status);
      state = lGetUlong(lFirst(*in_list), JAT_state);
      
      where = lWhere("%T(%I != %u || %I != %u)", JAT_Type,
                        JAT_status, status, JAT_state, state);
      lSplit(in_list, &out_list, NULL, where);

      where = lFreeWhere(where);
   
#if 0
   nxt_jatep = lFirst(*in_list);

   /* Get status of first element and initialize */
   if (lGetNumberOfElem(*in_list) > 0) {
      status = lGetUlong(nxt_jatep, JAT_status);
      state = lGetUlong(nxt_jatep, JAT_state);
      out_list = lCreateList("", JAT_Type);
   }

   /* Move all elements with the same status from in- in out-list */
   while((jatep = nxt_jatep)) {
      nxt_jatep = lNext(nxt_jatep);
      if (lGetUlong(jatep, JAT_status) == status &&
          lGetUlong(jatep, JAT_state) == state) {
         lDechainElem(*in_list, jatep);
         lAppendElem(out_list, jatep);
      } 
   }
#endif

   }

   return out_list;
}

void range_print_to_string(lList *range_list, StringBufferT *string) 
{
   lListElem *range_elem;
   u_long32 start, end, step;

   for_each(range_elem, range_list) {
      range_get_all_ids(range_elem, &start, &end, &step);
      add_taskrange_str(start, end, step, string);
   }
}

void range_sort_uniq_compress(lList *range_list)
{
   lListElem *range_elem1, *next_range_elem1;
   lListElem *range_elem2, *next_range_elem2;
   lList *tmp_list;

   lSortList2(range_list, "%I+", RN_min);

   /* 
    * Remove overlapping ranges
    */ 
   tmp_list = lCreateList("tmp list", RN_Type); 
   next_range_elem1 = lFirst(range_list);
   while ((range_elem1 = next_range_elem1)) {
      next_range_elem2 = lNext(next_range_elem1); 
      range_correct_end(range_elem1);
      while ((range_elem2 = next_range_elem2)) {
         next_range_elem2 = lNext(range_elem2);
         range_correct_end(range_elem2);
         if (range_is_overlapping(range_elem1, range_elem2)) {
            range_elem2 = lDechainElem(range_list, range_elem2);
            lAppendElem(tmp_list, range_elem2);
         } else {
            break;
         } 
      }
      next_range_elem1 = lNext(range_elem1);
   }

   /*
    * Insert all removed entries at the correct position
    */
   for_each(range_elem1, tmp_list) {
      u_long32 start1, end1, step1;

      range_get_all_ids(range_elem1, &start1, &end1, &step1);
      for (; start1 <= end1; start1 += step1) {
         range_insert_id(range_list, start1);
      }
   }

   lFreeList(tmp_list);

   range_compress(range_list);
}

void range_compress(lList *range_list) 
{
   lListElem *range_elem1, *next_range_elem1;
   lListElem *range_elem2, *next_range_elem2;   

   next_range_elem1 = lFirst(range_list);
   next_range_elem2 = lNext(next_range_elem1);
   while ((range_elem1 = next_range_elem1) &&
          (range_elem2 = next_range_elem2)) {
      u_long32 start1, end1, step1;
      u_long32 start2, end2, step2;
 
      range_get_all_ids(range_elem1, &start1, &end1, &step1);
      range_get_all_ids(range_elem2, &start2, &end2, &step2);
      if ((end1 + step1 == start2 && step1 == step2)) { 
         end1 = end2;
         step1 = step2;
         range_set_all_ids(range_elem1, start1, end1, step1);
         lRemoveElem(range_list, range_elem2);
         range_elem2 = NULL;
         next_range_elem1 = range_elem1;
      } else if (start1 == end1 && step1 == 1 && end1 == start2 - step2) {
         end1 = end2;
         step1 = step2;
         range_set_all_ids(range_elem1, start1, end1, step1);
         lRemoveElem(range_list, range_elem2);
         range_elem2 = NULL;
         next_range_elem1 = range_elem1;
      } else if (start2 == end2 && step2 == 1 && end1 + step1 == end2) {
         end1 = end2;
         range_set_all_ids(range_elem1, start1, end1, step1);
         lRemoveElem(range_list, range_elem2);
         range_elem2 = NULL;
         next_range_elem1 = range_elem1;
      } else if (start1 == end1 && start2 == end2 && step1 == step2 &&
                 step1 == 1) {
         end1 = start2;
         step1 = end1 - start1;
         range_set_all_ids(range_elem1, start1, end1, step1);
         lRemoveElem(range_list, range_elem2);
         range_elem2 = NULL;
         next_range_elem1 = range_elem1;
      } else {
         next_range_elem1 = lNext(range_elem1);
      }
      next_range_elem2 = lNext(next_range_elem1);
   }             
}

static void range_correct_end(lListElem *range_elem) 
{
   u_long32 start, end, step;
   u_long32 factor;

   range_get_all_ids(range_elem, &start, &end, &step);  
   if ((end - start) % step) {
      factor = (end - start) / step;
      end = start + factor * step;
      range_set_all_ids(range_elem, start, end, step);
   } 
}

static int range_is_overlapping(lListElem *range_elem1, lListElem *range_elem2)
{
   u_long32 start1, end1, step1;
   u_long32 start2, end2, step2;
   int ret;

   range_get_all_ids(range_elem1, &start1, &end1, &step1);
   range_get_all_ids(range_elem2, &start2, &end2, &step2);
   if (end1 >= start2) {
      ret = 1;
   } else {
      ret = 0;
   }
   return ret;
}

int range_is_within(lList *range_list, u_long32 id) 
{
   lListElem *range_elem;
   int ret = 0;

   for_each(range_elem, range_list) {
      u_long32 start, end, step;
      
      range_get_all_ids(range_elem, &start, &end, &step);      
      if (id >= start && id <= end && ((id - start) % step) == 0) {
         ret = 1;
         break;
      }
   } 
   return ret;
}

void range_remove_id(lList *range_list, u_long32 id) 
{
   lListElem *range_elem, *next_range_elem;

   next_range_elem = lFirst(range_list);
   while ((range_elem = next_range_elem)) {
      u_long32 start, end, step;
      
      next_range_elem = lNext(range_elem); 
      range_get_all_ids(range_elem, &start, &end, &step);      
      if (id >= start && id <= end && ((id - start) % step) == 0) { 
         if (id == start && id == end) {
            lRemoveElem(range_list, range_elem);
            break;
         } else if (id  == start) {
            start += step;
            range_set_all_ids(range_elem, start, end, step);
            break;
         } else if (id == end) {
            end -= step;
            range_set_all_ids(range_elem, start, end, step);
            break;
         } else {
            lListElem *new_range_elem;

            range_set_all_ids(range_elem, start, id - step, step);
            new_range_elem = lCreateElem(RN_Type);
            range_set_all_ids(new_range_elem, id + step, end, step);
            lInsertElem(range_list, range_elem, new_range_elem);
            break;
         }
      }
   } 
}

void range_insert_id(lList *range_list, u_long32 id)
{
   lListElem *range_elem, *prev_range_elem, *next_range_elem;
   int inserted = 0;
   DENTER(TOP_LAYER, "range_insert_id");

   lSortList2(range_list, "%I+", RN_min);

   range_elem = NULL;
   prev_range_elem = lLast(range_list);
   while((next_range_elem = range_elem, range_elem = prev_range_elem)) {
      u_long32 start, end, step;
      u_long32 prev_start, prev_end, prev_step;
      u_long32 next_start, next_end, next_step;
   
      prev_range_elem = lPrev(range_elem);
      range_get_all_ids(range_elem, &start, &end, &step);

      /* 1 */
      if (id < end) {
         if (prev_range_elem) {
            DTRACE;
            continue;
         } else {
            DTRACE;
            next_range_elem = range_elem;
            range_elem = prev_range_elem;
            prev_range_elem = NULL;
         }
      }
      
      if (next_range_elem) {
         range_get_all_ids(next_range_elem, &next_start, &next_end, &next_step);
      }
      if (prev_range_elem) {
         range_get_all_ids(prev_range_elem, &prev_start, &prev_end, &prev_step);
      }

      /* 2 */
      if (next_range_elem && id > next_start) {
         if (((id - next_start) % next_step == 0)) {
            /* id is already part of the range */
            DTRACE;
            inserted = 1;
         } else {
            lListElem *new_range_elem1, *new_range_elem2;
            u_long32 factor, prev_id, next_id;

            DTRACE;
            factor = ((id - next_start) / next_step);
            prev_id = next_start + factor * next_step;
            next_id = next_start + (factor + 1) * next_step;
            range_set_all_ids(next_range_elem, next_start, prev_id, 
                              next_step);
            new_range_elem1 = lCreateElem(RN_Type);  
            range_set_all_ids(new_range_elem1, id, id, 1);
            lInsertElem(range_list, next_range_elem, new_range_elem1);
            new_range_elem2 = lCreateElem(RN_Type);  
            range_set_all_ids(new_range_elem2, next_id, next_end, 
                              next_step);
            lInsertElem(range_list, new_range_elem1, new_range_elem2);
            inserted = 1;
         }
      } else {
         if ((range_elem && (end == id)) ||
             (next_range_elem && (next_start == id))) {
            /* id is already part of the range */
            DTRACE;
            inserted = 1;
         } else if (range_elem && (end + step == id)) {
            /* 3 */
            DTRACE;
            end = id;
            range_set_all_ids(range_elem, start, end, step);
            inserted = 1;
         } else if (next_range_elem && (next_start - next_step == id)) {
            /* 4 */
            DTRACE;
            next_start = id;
            range_set_all_ids(next_range_elem, next_start, next_end, next_step);
            inserted = 1;
         } else {
            lListElem *new_range_elem;
            
            /* 5 */
            DTRACE;
            new_range_elem = lCreateElem(RN_Type);
            range_set_all_ids(new_range_elem, id, id, 1);
            lInsertElem(range_list, range_elem, new_range_elem);
            inserted = 1;
         }
      }
      if (inserted) {
         break;
      }
   } 
   if (!inserted) {
      lListElem *new_range_elem;
 
      DTRACE;
      new_range_elem = lCreateElem(RN_Type);
      range_set_all_ids(new_range_elem, id, id, 1);
      lAppendElem(range_list, new_range_elem);
      inserted = 1;  
   }
   DEXIT;
}

void range_get_all_ids(lListElem *range_elem, u_long32 *min, u_long32 *max,
                       u_long32 *step)
{
   *min = lGetUlong(range_elem, RN_min);
   *max = lGetUlong(range_elem, RN_max);
   *step = lGetUlong(range_elem, RN_step);
}

void range_set_all_ids(lListElem *range_elem, u_long32 min, u_long32 max,
                       u_long32 step)
{
   lSetUlong(range_elem, RN_min, min);
   lSetUlong(range_elem, RN_max, max);
   lSetUlong(range_elem, RN_step, (min != max) ? step : 1);
}     

void range_calculate_union_set(lList *range_list1, lList *range_list2,
                               lList **range_list3)
{
   lListElem *range_elem2;

   *range_list3 = lFreeList(*range_list3);
   if (range_list1 && range_list2) {
      *range_list3 = lCopyList("union_set range list", range_list1); 
      range_sort_uniq_compress(*range_list3);
      for_each(range_elem2, range_list2) {
         u_long32 start2, end2, step2;
    
         range_get_all_ids(range_elem2, &start2, &end2, &step2);
         for (; start2 <= end2; start2 += step2) {
            range_insert_id(*range_list3, start2);
         }
      }       
      range_compress(*range_list3); 
   } else if (range_list1 || range_list2) {
      *range_list3 = lCopyList("union_set range list",
                               range_list1 ? range_list1 : range_list2);
      range_sort_uniq_compress(*range_list3);
   } 
}          

void range_calculate_difference_set(lList *range_list1, lList *range_list2,
                                    lList **range_list3)
{
   lListElem *range_elem2;

   *range_list3 = lFreeList(*range_list3);
   if (range_list1 && range_list2) {
      *range_list3 = lCopyList("union_set range list", range_list1); 
      range_sort_uniq_compress(*range_list3);
      for_each(range_elem2, range_list2) {
         u_long32 start2, end2, step2;
 
         range_get_all_ids(range_elem2, &start2, &end2, &step2);
         for (; start2 <= end2; start2 += step2) {
            range_remove_id(*range_list3, start2);
         }
      }
      range_compress(*range_list3);
   } else if (range_list1) {
      *range_list3 = lCopyList("union_set range list", range_list1);
      range_sort_uniq_compress(*range_list3);
   }
}

void range_calculate_intersection_set(lList *range_list1, lList *range_list2,
                                      lList **range_list3)
{ 
   lListElem *range_elem;

   *range_list3 = lFreeList(*range_list3);
   if (range_list1 && range_list2) {
      range_sort_uniq_compress(range_list1); 
      for_each(range_elem, range_list1) {
         u_long32 start, end, step;
 
         range_get_all_ids(range_elem, &start, &end, &step);
         for (; start <= end; start += step) {
            if (range_is_within(range_list2, start)) {
               lListElem *new_range_elem;

               if (!(*range_list3)) {
                  *range_list3 = lCreateList("intersection_set", RN_Type);
               }
               new_range_elem = lCreateElem(RN_Type);
               range_set_all_ids(new_range_elem, start, start, 1);
               lAppendElem(*range_list3, new_range_elem);  
            }
         }
      }
      range_compress(*range_list3);
   }
}
