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

#include "cull.h"
#include "sge_report_execd.h"
#include "sge_usageL.h"
#include "sge_ack.h"
#include "job_report_execd.h"
#include "sge_any_request.h"
#include "reaper_execd.h"
#include "sge_signal.h"
#include "execd_signal_queue.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "msg_execd.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe.h"
#include "sge_report.h"

lList *jr_list = NULL;
int flush_jr = 0;

void trace_jr()
{
   lListElem *jr;

   DENTER(TOP_LAYER, "trace_jr");

   DPRINTF(("--- JOB REPORT LIST ----------------\n"));
   for_each (jr, jr_list) {
      const char *s;

      if ((s=lGetString(jr, JR_pe_task_id_str)))
         DPRINTF(("Jobtask "u32"."u32" task %s\n", lGetUlong(jr, JR_job_number), lGetUlong(jr, JR_ja_task_number), s));
      else
         DPRINTF(("Jobtask "u32"."u32"\n", lGetUlong(jr, JR_job_number), lGetUlong(jr, JR_ja_task_number)));
   }
   DEXIT;
}

lListElem *add_job_report(
u_long32 jobid,
u_long32 jataskid,
const char *petaskid,
lListElem *jep 
) {  
   lListElem *jr, *jatep = NULL;
 
   DENTER(TOP_LAYER, "add_job_report");

   if (!jr_list) 
      jr_list = lCreateList("job report list", JR_Type);
  
   if (!jr_list || !(jr=lCreateElem(JR_Type))) {
      ERROR((SGE_EVENT, MSG_JOB_TYPEMALLOC));  
      DEXIT;
      return NULL;
   }

   lSetUlong(jr, JR_job_number, jobid);
   lSetUlong(jr, JR_ja_task_number, jataskid);
   if(petaskid != NULL) {
      lSetString(jr, JR_pe_task_id_str, petaskid);
   }

   lAppendElem(jr_list, jr);

   if(jep != NULL) {
      jatep = job_search_task(jep, NULL, jataskid);
      if (jatep != NULL) { 
         lListElem *petep = NULL;
         if(petaskid != NULL) {
            petep = ja_task_search_pe_task(jatep, petaskid);
         }   
         job_report_init_from_job(jr, jep, jatep, petep);
      }
   }
 
   DEXIT;
   return jr;
}

lListElem *get_job_report(
u_long32 jobid, 
u_long32 jataskid, 
const char *petaskid 
) {
   lListElem *jr;
   const char *s;

   DENTER(TOP_LAYER, "get_job_report");

   /* JG: TODO (256): Could be optimized by using lGetElemUlong for jobid */
   for_each (jr, jr_list) {
      s = lGetString(jr, JR_pe_task_id_str);

      if (lGetUlong(jr, JR_job_number) == jobid && 
          lGetUlong(jr, JR_ja_task_number) == jataskid &&
          !sge_strnullcmp(s, petaskid))
         break; 
   }

   DEXIT;
   return jr;
}

void del_job_report(
lListElem *jr 
) {
   lRemoveElem(jr_list, jr);
}

void cleanup_job_report(
u_long32 jobid,
u_long32 jataskid 
) {
   lListElem *nxt, *jr;

   DENTER(TOP_LAYER, "clean_job_report");

   /* get rid of job reports for all slave tasks */
   nxt = lFirst(jr_list);
   while ((jr=nxt)) {
      nxt = lNext(jr);
      if (lGetUlong(jr, JR_job_number) == jobid &&
          lGetUlong(jr, JR_ja_task_number) == jataskid) {
         const char *s = lGetString(jr, JR_pe_task_id_str);

         DPRINTF(("!!!! removing jobreport for "u32"."u32" task %s !!!!\n",
            jobid, jataskid, s?s:"master"));
         lRemoveElem(jr_list, jr);
      }
   }

   DEXIT;
   return;
}

/* ------------------------------------------------------------
   NAME

      add_usage()
   
   DESCR

      Adds ulong attribute 'name' to the usage list of a 
      job report 'jr'. If no 'uval_as_str' or it is not
      convertable into a ulong 'uval_as_ulong' is used 
      as value for UA_value.

   RETURN      

      0 on success
      -1 on error
   ------------------------------------------------------------ */
/* JG: TODO (397): move to libs/gdi/sge_usage.* */   
int add_usage(lListElem *jr, char *name, const char *val_as_str, double val) 
{
   lListElem *usage;
   double old_val = 0;

   DENTER(CULL_LAYER, "add_usage");

   if (!jr || !name) {
      DEXIT;
      return -1;
   }

   /* check if we already have an usage value with this name */
   usage = lGetSubStr(jr, UA_name, name, JR_usage);
   if (!usage) {
      if (!(usage = lAddSubStr(jr, UA_name, name, JR_usage, UA_Type))) {
         DEXIT;
         return -1;
      }
   } else 
      old_val = lGetDouble(usage, UA_value);

   if (val_as_str) {
      char *p;
      double parsed;

      parsed = strtod(val_as_str, &p);
      if (p==val_as_str) {
         ERROR((SGE_EVENT, MSG_PARSE_USAGEATTR_SSU, 
                val_as_str, name, u32c(lGetUlong(jr, JR_job_number)))); 
         /* use default value */
         lSetDouble(usage, UA_value, val); 
         DEXIT;
         return -1;
      }
      val = parsed;
   }
      
   lSetDouble(usage, UA_value, val>old_val?val:old_val); 

   DEXIT;
   return 0;
}


/* ------------------------------------------------------------

NAME 
   
   execd_c_ack()

DESCRIPTION
   
   These requests are triggered by our job report list
   that is sent periodically. They are responses of
   Qmaster in different cases. But they get sent as one
   message, to save communication. 

RETURN

   Typical dispatcher service function return values

   ------------------------------------------------------------ */
int execd_c_ack(
struct dispatch_entry *de,
sge_pack_buffer *pb,
sge_pack_buffer *apb,
u_long *rcvtimeout,
int *synchron,
char *err_str,
int answer_error 
) {
   u_long32 ack_type;
   u_long32 jobid, jataskid;
   lListElem *jr;
   char *pe_task_id_str;

   DENTER(TOP_LAYER, "execd_c_ack");
 
   DPRINTF(("------- GOT ACK'S ---------\n"));
 
   /* we get a bunch of ack's */
   while (pb_unused(pb)>0) {
 
      unpackint(pb, &ack_type);
      switch (ack_type) {
 
         case ACK_JOB_EXIT:
/*
**          This is the answer of qmaster if we report a job as exiting
**          - job gets removed from job  report list and from job list
**          - job gets cleaned from file system                       
**          - retry is triggered by next job report sent to qmaster 
**            containing this job as "exiting"                  
*/
            unpackint(pb, &jobid);
            unpackint(pb, &jataskid);
            unpackstr(pb, &pe_task_id_str);

            DPRINTF(("remove exiting job "u32"/"u32"/%s\n", 
                    jobid, jataskid, pe_task_id_str?pe_task_id_str:""));

            if ((jr = get_job_report(jobid, jataskid, pe_task_id_str))) {
               remove_acked_job_exit(jobid, jataskid, pe_task_id_str, jr);
            } 
            else {
               DPRINTF(("acknowledged job "u32"."u32" not found\n", jobid, jataskid));
            }

            if (pe_task_id_str)
               free(pe_task_id_str);
            break;
 
         case ACK_SIGNAL_JOB:
/*
**          This is the answer of qmaster
**          if we report a job as running
**          while qmaster does not know  
**          this job                     
**          - no "unknown job" is added  
**            to the job report          
**          - retry is triggered by next 
**            job report sent to qmaster 
**            containing this job as     
**            "running"                  
*/
            {
               u_long32 signo;
 
               unpackint(pb, &jobid);
               unpackint(pb, &jataskid);
               unpackint(pb, &signo);
 
               if (signal_job(jobid, jataskid, signo)) {
                  lListElem *jr;
                  jr = get_job_report(jobid, jataskid, NULL);
                  remove_acked_job_exit(jobid, jataskid, NULL, jr);
                  job_unknown(jobid, jataskid, NULL);
               }
            }
            break;
 
         default:
            ERROR((SGE_EVENT, MSG_COM_ACK_UNKNOWN));
            break;
      }
   }
   DEXIT;
   return 0;
}

int
execd_get_acct_multiplication_factor(const lListElem *pe, 
                                     int slots, bool task)
{
   int factor = 1;

   /* task of tightly integrated job: default factor 1 is OK - skip it */
   if (!task) {
      /* only parallel jobs need factors != 0 */
      if (pe != NULL) {
         /* only loosely integrated job will get factor != 0 */
         if (!lGetBool(pe, PE_control_slaves)) {
            /* if job is first task: factor = n, else n + 1 */
            if (lGetBool(pe, PE_job_is_first_task)) {
               factor = slots;
            } else {
               factor = slots + 1;
            }
         }
      }
   }

   DPRINTF(("reserved usage will be multiplied by %d\n", factor));

   return factor;
}
