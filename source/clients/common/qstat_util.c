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
/* #include "qstd_status.h" */
#include "parse.h"
#include "parse_range.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "utility.h"
#include "sge_parse_num_par.h"
#include "sge_string.h"
#include "show_job.h"
#include "sge_string_append.h"
#include "qstat_util.h"
#include "sge_schedd_text.h"
#include "job.h"

static void add_taskrange_str(u_long32 start, u_long32 end, int step, char *taskrange_str);

void get_taskrange_str(lList* task_list, char* taskrange_str) {
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
            add_taskrange_str(start, end, step, taskrange_str); 
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
      add_taskrange_str(start, end, step, taskrange_str); 
   } else if (before_last_id==-1 && last_id!=-1 && id!=-1) {
      start = last_id;
      end = id;
      step = end-start;
      add_taskrange_str(start, end, step, taskrange_str); 
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
      add_taskrange_str(start, end, step, taskrange_str); 
   }
#if 0
   fprintf (stderr, "=>start: %ld, end: %ld, step: %ld\n", start, end, step);
   fprintf(stderr, "String: %s\n", taskrange_str);
#endif
} 

static void add_taskrange_str(
u_long32 start,
u_long32 end,
int step,
char *taskrange_str 
) {
   char tail[256]="";

   
   if (strlen(taskrange_str)>0)
      strcat(taskrange_str, ",");

   if (start == end)
      sprintf(tail, u32, start);
   else if (start+step == end)
      sprintf(tail, u32","u32, start, end);
   else {
      sprintf(tail, u32"-"u32":%d", start, end, step); 
   }
   strcat(taskrange_str, tail);
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


/*-------------------------------------------------------------------------*/
/*
** qstat_show_job
** displays information about a given job
** to be extended
**
** returns 0 on success, non-zero on failure
*/
int show_info_for_jobs(
lList *jid_list,
FILE *fp,
lList **alpp,
StringBufferT *sb 
) {
   lListElem *j_elem = 0;
   lList* jlp = NULL;
   lList* ilp = NULL;
   lListElem* aep = NULL;
   lCondition *where = NULL, *newcp = NULL;
   lEnumeration* what = NULL;
   lList* alp = NULL;
   int schedd_info = TRUE;
   int jobs_exist = TRUE;
   int line_separator=0;
   lListElem* mes;

   DENTER(TOP_LAYER, "qstat_show_job");

   /* get job scheduling information */
   what = lWhat("%T(ALL)", SME_Type);
   alp = sge_gdi(SGE_JOB_SCHEDD_INFO, SGE_GDI_GET, &ilp, NULL, what);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         if (fp)
            fprintf(fp, "%s", lGetString(aep, AN_text));
         if (alpp) {
            sge_add_answer(alpp, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
         }
         schedd_info = FALSE;
      }
   }
   alp = lFreeList(alp);

   /* build 'where' for all jobs */
   where = NULL;
   for_each(j_elem, jid_list) {
      u_long32 jid = atol(lGetString(j_elem, STR));

      newcp = lWhere("%T(%I==%u)", JB_Type, JB_job_number, jid);
      if (!where)
         where = newcp;
      else
         where = lOrWhere(where, newcp);
   }
   what = lWhat("%T(ALL)", JB_Type);
   /* get job list */
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_GET, &jlp, where, what);
   lFreeWhere(where);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         if (fp)
            fprintf(fp, "%s", lGetString(aep, AN_text));
         if (alpp) {
            sge_add_answer(alpp, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
         }
         jobs_exist = FALSE;
      }
   }
   lFreeList(alp);
   if(!jobs_exist) {
      DEXIT;
      return 1;
   }

   /* print scheduler job information and global scheduler info */
   for_each (j_elem, jlp) {
      u_long32 jid = lGetUlong(j_elem, JB_job_number);
      lListElem *sme;

      if (line_separator)
         sge_string_printf(sb, "\n");
      else
         line_separator = 1;
/*       cull_show_job(j_elem, 0); */
      if (schedd_info && (sme = lFirst(ilp))) {
         int first_run = 1;

         if (sme) {
            /* global schduling info */
            for_each (mes, lGetList(sme, SME_global_message_list)) {
               if (first_run) {
                  sge_string_printf(sb, "%s", "scheduling info:            ");
                  first_run = 0;
               }
               else
                  sge_string_printf(sb, "%s", "                            ");
               sge_string_printf(sb, "%s\n", lGetString(mes, MES_message));
            }

            /* job scheduling info */
            where = lWhere("%T(%I->%T(%I==%u))", MES_Type, MES_job_number_list, 
               ULNG_Type, ULNG, jid);
            mes = lFindFirst(lGetList(sme, SME_message_list), where);
            if (mes) {
               if (first_run) {
                  sge_string_printf(sb, "%s", "scheduling info:            ");
                  first_run = 0;
               }
               else
                  sge_string_printf(sb, "%s\n", lGetString(mes, MES_message));
            }
            while ((mes = lFindNext(mes, where)))
               sge_string_printf(sb, "                            %s\n", 
                                    lGetString(mes, MES_message));
            lFreeWhere(where);
         }
      }
   }

   lFreeList(ilp);
   lFreeList(jlp);
   DEXIT;
   return 0;
}

int show_info_for_job(
FILE *fp,
lList **alpp,
StringBufferT *sb 
) {
   lList *ilp = NULL, *mlp = NULL;
   lListElem* aep = NULL;
   lEnumeration* what = NULL;
   lList* alp = NULL;
   int schedd_info = TRUE;
   lListElem* mes;
   int initialized = 0;
   u_long32 last_jid = 0;
   u_long32 last_mid = 0;
   char text[256], ltext[256];
   int ids_per_line = 0;
   int first_run = 1;
   int first_row = 1;
   lListElem *sme;
   lListElem *jid_ulng = NULL; 

   DENTER(TOP_LAYER, "qstat_show_job");

   /* get job scheduling information */
   what = lWhat("%T(ALL)", SME_Type);
   alp = sge_gdi(SGE_JOB_SCHEDD_INFO, SGE_GDI_GET, &ilp, NULL, what);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         if (fp)
            fprintf(fp, "%s", lGetString(aep, AN_text));
         if (alpp) {
            sge_add_answer(alpp, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
         }
         schedd_info = FALSE;
      }
   }
   lFreeList(alp);
   if (!schedd_info) {
      DEXIT;
      return 1;
   }

   sme = lFirst(ilp);
   if (sme) {
      /* print global schduling info */
      first_run = 1;
      for_each (mes, lGetList(sme, SME_global_message_list)) {
         if (first_run) {
            sge_string_printf(sb, "%s", "scheduling info:            ");
            first_run = 0;
         }
         else
            sge_string_printf(sb, "%s", "                            ");
         sge_string_printf(sb, "%s\n", lGetString(mes, MES_message));
      }
      if (!first_run)
         sge_string_printf(sb, "\n");

      first_run = 1;

      mlp = lGetList(sme, SME_message_list);
      lPSortList (mlp, "I+", MES_message_number);

      text[0]=0;
      for_each(mes, mlp) {
         lPSortList (lGetList(mes, MES_job_number_list), "I+", ULNG);

         for_each(jid_ulng, lGetList(mes, MES_job_number_list)) {
            u_long32 mid;
            u_long32 jid = 0;
            int skip = 0;
            int header = 0;

            mid = lGetUlong(mes, MES_message_number);
            jid = lGetUlong(jid_ulng, ULNG);

            if (initialized) {
               if (last_mid == mid && last_jid == jid)
                  skip = 1;
               else if (last_mid != mid)
                  header = 1;
            }
            else {
               initialized = 1;
               header = 1;
            }

            if (strlen(text) >= MAX_LINE_LEN || ids_per_line >= MAX_IDS_PER_LINE || header) {
               sge_string_printf(sb, "%s", text);
               text[0] = 0;
               ids_per_line = 0;
               first_row = 0;
            }

            if (header) {
               if (!first_run)
                  printf("\n\n");
               else
                  first_run = 0;
               sge_string_printf(sb, "%s\n", sge_schedd_text(mid+SCHEDD_INFO_OFFSET));
               first_row = 1;
            }

            if (!skip) {
               if (ids_per_line == 0)
                  if (first_row)
                     strcat(text, "\t");
                  else
                     strcat(text, ",\n\t");
               else
                  strcat(text, ",\t");
               sprintf(ltext, u32, jid);
               strcat(text, ltext);
               ids_per_line++;
            }

            last_jid = jid;
            last_mid = mid;
         }
      }
      if (text[0] != 0)
         sge_string_printf(sb, "%s\n", text);
   }

   lFreeList(ilp);
   DEXIT;
   return 0;
}
