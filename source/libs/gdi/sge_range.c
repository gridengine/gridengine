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
#include "sge_complex.h"
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

static void add_taskrange_str(u_long32 start, u_long32 end, int step, StringBufferT *dyn_taskrange_str);

static void range_correct_end(lListElem *range);
 
static int range_is_overlapping(const lListElem *range1, 
                                const lListElem *range2);

static void range_correct_end(lListElem *range) 
{
   if (range) {
      u_long32 start, end, step;

      range_get_all_ids(range, &start, &end, &step);  
      if ((end - start) % step) {
         u_long32 factor;

         factor = (end - start) / step;
         end = start + factor * step;
         range_set_all_ids(range, start, end, step);
      } 
   }
}

static int range_is_overlapping(const lListElem *range1, 
                                const lListElem *range2)
{
   int ret = 0;

   if (range1 != NULL && range2 != NULL) {
      u_long32 start1, end1, step1;
      u_long32 start2, end2, step2;

      range_get_all_ids(range1, &start1, &end1, &step1);
      range_get_all_ids(range2, &start2, &end2, &step2);
      if (end1 >= start2) {
         ret = 1;
      } 
   }
   return ret;
}

void range_list_initialize(lList **range_list, lList **answer_list) 
{
   if (range_list != NULL) {
      if (*range_list != NULL) {
         lListElem *range;
         lListElem *next_range;

         next_range = lFirst(*range_list);
         while ((range = next_range)) {
            next_range = lNext(range);

            lRemoveElem(*range_list, range);
         } 
      } else {
         *range_list = lCreateList("range list", RN_Type);
         if (*range_list == NULL) {
            sge_add_answer(answer_list, "unable to create range list",
                           STATUS_ERROR1, NUM_AN_ERROR);
         }
      }
   }
}

u_long32 range_list_get_number_of_ids(const lList *range_list)
{
   u_long32 ret = 0;
   lListElem *range;

   for_each(range, range_list) {
      ret += range_get_number_of_ids(range);
   } 
   return ret;
}

u_long32 range_get_number_of_ids(const lListElem *range) 
{
   u_long32 start, end, step;

   range_get_all_ids(range, &start, &end, &step);
   return 1 + (end - start) / step;
}

void range_print_to_string(const lList *range_list, StringBufferT *string) 
{
   if (range_list != NULL && string != NULL) {
      lListElem *range;
      u_long32 start, end, step;

      for_each(range, range_list) {
         range_get_all_ids(range, &start, &end, &step);
         add_taskrange_str(start, end, step, string);
      }
   }
}

u_long32 range_list_get_first_id(const lList *range_list, lList **answer_list) 
{
   u_long32 start = 0;
   lListElem *range = lFirst(range_list);
   DENTER(BASIS_LAYER, "range_list_get_first_id");

   if (range) {
      u_long32 end, step;

      range_get_all_ids(range, &start, &end, &step);
   } else {
      DTRACE;
      sge_add_answer(answer_list, "range_list containes no elements",
                     STATUS_ERROR1, NUM_AN_ERROR);
   }
   DEXIT;
   return start;
}

u_long32 range_list_get_last_id(const lList *range_list, lList **answer_list) 
{
   u_long32 end = 0;
   lListElem *range = lLast(range_list);

   if (range) {
      u_long32 start, step;

      range_get_all_ids(range, &start, &end, &step);
   } else {
      sge_add_answer(answer_list, "range_list containes no elements",
                     STATUS_ERROR1, NUM_AN_ERROR);
   }
   return end;
}

void range_sort_uniq_compress(lList *range_list, lList **answer_list)
{
   if (range_list) {
      lListElem *range1, *next_range1;
      lListElem *range2, *next_range2;
      lList *tmp_list;

      lSortList2(range_list, "%I+", RN_min);

      /* 
       * Remove overlapping ranges
       */ 
      tmp_list = lCreateList("tmp list", RN_Type); 
      if (tmp_list) {
         next_range1 = lFirst(range_list);
         while ((range1 = next_range1)) {
            next_range2 = lNext(next_range1); 
            range_correct_end(range1);
            while ((range2 = next_range2)) {
               next_range2 = lNext(range2);
               range_correct_end(range2);
               if (range_is_overlapping(range1, range2)) {
                  range2 = lDechainElem(range_list, range2);
                  lAppendElem(tmp_list, range2);
               } else {
                  break;
               } 
            }
            next_range1 = lNext(range1);
         }

         /*
          * Insert all removed entries at the correct position
          */
         for_each(range1, tmp_list) {
            u_long32 start1, end1, step1;

            range_get_all_ids(range1, &start1, &end1, &step1);
            for (; start1 <= end1; start1 += step1) {
               range_list_insert_id(&range_list, answer_list, start1);
            }
         }

         lFreeList(tmp_list);
         range_compress(range_list);
      } else {
         sge_add_answer(answer_list, "unable to create range list",
                        STATUS_ERROR1, NUM_AN_ERROR);            
      }
   }
}

void range_compress(lList *range_list) 
{
   if (range_list != NULL) {
      lListElem *range1 = NULL;
      lListElem *range2 = NULL;
      lListElem *next_range1 = lFirst(range_list);
      lListElem *next_range2 = lNext(next_range1);   

      while ((range1 = next_range1) &&
             (range2 = next_range2)) {
         u_long32 start1, end1, step1;
         u_long32 start2, end2, step2;
    
         range_get_all_ids(range1, &start1, &end1, &step1);
         range_get_all_ids(range2, &start2, &end2, &step2);
         if (end1 + step1 == start2 && step1 == step2) { 
            end1 = end2;
            step1 = step2;
            range_set_all_ids(range1, start1, end1, step1);
            lRemoveElem(range_list, range2);
            range2 = NULL;
            next_range1 = range1;
         } else if (start1 == end1 && step1 == 1 && end1 == start2 - step2) {
            end1 = end2;
            step1 = step2;
            range_set_all_ids(range1, start1, end1, step1);
            lRemoveElem(range_list, range2);
            range2 = NULL;
            next_range1 = range1;
         } else if (start2 == end2 && step2 == 1 && end1 + step1 == end2) {
            end1 = end2;
            range_set_all_ids(range1, start1, end1, step1);
            lRemoveElem(range_list, range2);
            range2 = NULL;
            next_range1 = range1;
         } else if (start1 == end1 && start2 == end2 && step1 == step2 &&
                    step1 == 1) {
            end1 = start2;
            step1 = end1 - start1;
            range_set_all_ids(range1, start1, end1, step1);
            lRemoveElem(range_list, range2);
            range2 = NULL;
            next_range1 = range1;
         } else {
            next_range1 = lNext(range1);
         }
         next_range2 = lNext(next_range1);
      }             
   }
}

int range_list_is_id_within(const lList *range_list, u_long32 id) 
{
   lListElem *range = NULL;
   int ret = 0;

   for_each(range, range_list) {
      if (range_is_id_within(range, id)) {
         ret = 1;
         break;
      }
   } 
   return ret;
}

int range_is_id_within(const lListElem *range, u_long32 id) 
{
   int ret = 0;

   if (range) {
      u_long32 start, end, step; 

      range_get_all_ids(range, &start, &end, &step); 
      if (id >= start && id <= end && ((id - start) % step) == 0) {
         ret = 1;
      } 
   }
   return ret;
}

void range_list_remove_id(lList **range_list, lList **answer_list, u_long32 id) 
{
   lListElem *range = NULL;
   lListElem *next_range = lFirst(*range_list);

   if (range_list != NULL && *range_list != NULL) {
      while ((range = next_range)) {
         u_long32 start, end, step;
         
         next_range = lNext(range); 
         range_get_all_ids(range, &start, &end, &step);      
         if (id >= start && id <= end && ((id - start) % step) == 0) { 
            if (id == start && id == end) {
               lRemoveElem(*range_list, range);
               break;
            } else if (id  == start) {
               start += step;
               range_set_all_ids(range, start, end, step);
               break;
            } else if (id == end) {
               end -= step;
               range_set_all_ids(range, start, end, step);
               break;
            } else {
               lListElem *new_range = lCreateElem(RN_Type);

               if (new_range != NULL) {
                  range_set_all_ids(range, start, id - step, step);
                  range_set_all_ids(new_range, id + step, end, step);
                  lInsertElem(*range_list, range, new_range);
               } else {
                  sge_add_answer(answer_list, "unable to split range element",
                                 STATUS_ERROR1, NUM_AN_ERROR);     
               }
               break;
            }
         }
      }
      if (lGetNumberOfElem(*range_list) == 0) {
         *range_list = lFreeList(*range_list);
      } 
   }
}

void range_list_move_first_n_ids(lList **range_list, lList **answer_list,
                                 lList **range_list2, u_long32 n) 
{
   DENTER(TOP_LAYER, "range_list_move_first_n_ids");
   if (range_list && *range_list && range_list2) {
      lListElem *range = NULL;
      u_long32 id;

      for_each_id_in_range_list(id, range, *range_list) {
         range_list_insert_id(range_list2, answer_list, id);
         if (--n == 0) {
            break;
         }
      }
      for_each_id_in_range_list(id, range, *range_list2) {
         range_list_remove_id(range_list, answer_list, id); 
      }
   }
   DEXIT;
}

void range_list_insert_id(lList **range_list, lList **answer_list, u_long32 id)
{
   lListElem *range, *prev_range, *next_range;
   int inserted = 0;
   DENTER(TOP_LAYER, "range_insert_id");

   lSortList2(*range_list, "%I+", RN_min);

   range = NULL;
   if (*range_list == NULL) {
      *range_list = lCreateList("range list", RN_Type);
      if (*range_list == NULL) {
         sge_add_answer(answer_list, "unable to insert id into range",
                        STATUS_ERROR1, NUM_AN_ERROR);  
      }
   }
   prev_range = lLast(*range_list);
   while((next_range = range, range = prev_range)) {
      u_long32 start, end, step;
      u_long32 prev_start, prev_end, prev_step;
      u_long32 next_start, next_end, next_step;
   
      prev_range = lPrev(range);
      range_get_all_ids(range, &start, &end, &step);

      /* 1 */
      if (id < end) {
         if (prev_range) {
            DTRACE;
            continue;
         } else {
            DTRACE;
            next_range = range;
            range = prev_range;
            prev_range = NULL;
         }
      }
      
      if (next_range) {
         range_get_all_ids(next_range, &next_start, &next_end, &next_step);
      }
      if (prev_range) {
         range_get_all_ids(prev_range, &prev_start, &prev_end, &prev_step);
      }

      /* 2 */
      if (next_range && id > next_start) {
         if (((id - next_start) % next_step == 0)) {
            /* id is already part of the range */
            DTRACE;
            inserted = 1;
         } else {
            lListElem *new_range1, *new_range2;
            u_long32 factor, prev_id, next_id;

            DTRACE;
            factor = ((id - next_start) / next_step);
            prev_id = next_start + factor * next_step;
            next_id = next_start + (factor + 1) * next_step;
            range_set_all_ids(next_range, next_start, prev_id, 
                              next_step);
            new_range1 = lCreateElem(RN_Type);  
            range_set_all_ids(new_range1, id, id, 1);
            lInsertElem(*range_list, next_range, new_range1);
            new_range2 = lCreateElem(RN_Type);  
            range_set_all_ids(new_range2, next_id, next_end, 
                              next_step);
            lInsertElem(*range_list, new_range1, new_range2);
            inserted = 1;
         }
      } else {
         if ((range && (end == id)) ||
             (next_range && (next_start == id))) {
            /* id is already part of the range */
            DTRACE;
            inserted = 1;
         } else if (range && (end + step == id)) {
            /* 3 */
            DTRACE;
            end = id;
            range_set_all_ids(range, start, end, step);
            inserted = 1;
         } else if (next_range && (next_start - next_step == id)) {
            /* 4 */
            DTRACE;
            next_start = id;
            range_set_all_ids(next_range, next_start, next_end, next_step);
            inserted = 1;
         } else {
            lListElem *new_range;
            
            /* 5 */
            DTRACE;
            new_range = lCreateElem(RN_Type);
            range_set_all_ids(new_range, id, id, 1);
            lInsertElem(*range_list, range, new_range);
            inserted = 1;
         }
      }
      if (inserted) {
         break;
      }
   } 
   if (!inserted) {
      lListElem *new_range;
 
      DTRACE;
      new_range = lCreateElem(RN_Type);
      range_set_all_ids(new_range, id, id, 1);
      lAppendElem(*range_list, new_range);
      inserted = 1;  
   }
   DEXIT;
}

void range_get_all_ids(const lListElem *range, u_long32 *min, u_long32 *max,
                       u_long32 *step)
{
   if (range) {
      *min = lGetUlong(range, RN_min);
      *max = lGetUlong(range, RN_max);
      *step = lGetUlong(range, RN_step);
   }
}

void range_set_all_ids(lListElem *range, u_long32 min, u_long32 max,
                       u_long32 step)
{
   if (range != NULL) {
      lSetUlong(range, RN_min, min);
      lSetUlong(range, RN_max, max);
      lSetUlong(range, RN_step, (min != max) ? step : 1);
   }
}     

void range_list_calculate_union_set(lList **range_list, lList **answer_list,
                                    const lList *range_list1,
                                    const lList *range_list2)
{
   if (range_list != NULL && (range_list1 != NULL || range_list2 != NULL)) {
      *range_list = lFreeList(*range_list);
      *range_list = lCopyList("union_set range list",
                              range_list1 ? range_list1 : range_list2);
      if (*range_list) {
         goto error;
      }

      range_sort_uniq_compress(*range_list, answer_list);
      if (answer_list_is_error_in_list(answer_list)) {
         goto error;
      }

      if (range_list1 != NULL && range_list2 != NULL) {
         lListElem *range2 = NULL;

         for_each(range2, range_list2) {
            u_long32 start2, end2, step2;

            range_get_all_ids(range2, &start2, &end2, &step2);
            for (; start2 <= end2; start2 += step2) {
               range_list_insert_id(range_list, answer_list, start2);
            }
         }
         range_compress(*range_list); 
      }
   }
   return;

error:
   *range_list = lFreeList(*range_list);
   sge_add_answer(answer_list, "unable to calculate union set", 
                  STATUS_ERROR1, NUM_AN_ERROR);
}

void range_calculate_difference_set(lList **range_list, lList **answer_list,
                                    const lList *range_list1, 
                                    const lList *range_list2)
{
   DENTER(TOP_LAYER, "range_calculate_difference_set");
   if (range_list != NULL && range_list1 != NULL) {
      *range_list = lFreeList(*range_list);
      *range_list = lCopyList("difference_set range list", range_list1);
      if (*range_list == NULL) {
         goto error;
      }

      range_sort_uniq_compress(*range_list, answer_list);
      if (answer_list_is_error_in_list(answer_list)) {
         goto error;
      }            

      if (range_list2 != NULL) {
         lListElem *range2 = NULL;
 
         for_each(range2, range_list2) {
            u_long32 start2, end2, step2;
 
            range_get_all_ids(range2, &start2, &end2, &step2);
            for (; start2 <= end2; start2 += step2) {
               range_list_remove_id(range_list, answer_list, start2);
               if (answer_list_is_error_in_list(answer_list)) {
                  goto error;
               }
            }
         }
         range_compress(*range_list);
      }            
   }
   DEXIT;
   return;
 
error:
   *range_list = lFreeList(*range_list);
   sge_add_answer(answer_list, "unable to calculate union set",
                  STATUS_ERROR1, NUM_AN_ERROR); 
   DEXIT;
}

void range_calculate_intersection_set(lList **range_list, lList **answer_list,
                                      const lList *range_list1, 
                                      const lList *range_list2)
{ 
   *range_list = lFreeList(*range_list);
   if (range_list1 && range_list2) {
      lListElem *range;

      for_each(range, range_list1) {
         u_long32 start, end, step;
 
         range_get_all_ids(range, &start, &end, &step);
         for (; start <= end; start += step) {
            if (range_list_is_id_within(range_list2, start)) {
               lListElem *new_range;

               if (*range_list == NULL) {
                  *range_list = lCreateList("intersection_set", RN_Type);
                  if (*range_list == NULL) {
                     goto error;
                  }
               }
               new_range = lCreateElem(RN_Type);
               if (new_range == NULL) {
                  goto error;
               }
               range_set_all_ids(new_range, start, start, 1);
               lAppendElem(*range_list, new_range);  
            }
         }
      }
      range_compress(*range_list);
   }
   return;

error:
   *range_list = lFreeList(*range_list);
   sge_add_answer(answer_list, "unable to calculate intersection set",
                  STATUS_ERROR1, NUM_AN_ERROR);
}


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
