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
#include <stdlib.h>
#include <string.h>

#include "rmon/sgermon.h"

#include "uti/sge_log.h"
#include "uti/sge_string.h"
#include "uti/sge_time.h"

#include "sgeobj/sge_job.h"
#include "sgeobj/sge_ja_task.h"
#include "sgeobj/sge_pe_task.h"
#include "sgeobj/sge_report.h"
#include "sgeobj/sge_usage.h"

#include "sched/sge_job_schedd.h"

#include "sge_rusage.h"

#ifdef NEC_ACCOUNTING_ENTRIES
#define ARCH_COLUMN "%c%s"
#else
#define ARCH_COLUMN ""
#endif

#define ACTFILE_FPRINTF_FORMAT \
"%s%c%s%c%s%c%s%c%s%c"u32"%c%s%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c" \
u32"%c"u32"%c"u32"%c%f%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c%f%c" \
u32"%c"u32"%c"u32"%c"u32"%c"u32"%c"u32"%c%s%c%s%c%s%c%d%c"u32"%c%f%c%f%c%f%c%s%c%f%c%s%c%f" \
ARCH_COLUMN \
"\n"

#ifdef NEC_ACCOUNTING_ENTRIES
#define NECSX_ACTFILE_FPRINTF_FORMAT \
"%s,"u32","u32","u32","u32","u32","u32","u32","u32","u32","u32","u32"," \
u32","u32","u32","u32","u32","u32","u32","u32","u32","u32"\n"
#endif 

#define SET_STR_DEFAULT(jr, nm, s) if (!lGetString(jr, nm)) \
                                      lSetString(jr, nm, s);
#define SET_HOST_DEFAULT(jr, nm, s) if (!lGetHost(jr, nm)) \
                                      lSetHost(jr, nm, s);

static double 
reporting_get_double_usage (lList *usage_list, lList *reported_list, 
                            const char *name, const char *rname, double def);
/* ------------------------------------------------------------

   write usage to a dstring buffer

   sge_write_rusage - write rusage info to a dstring buffer
   Returns: false, if it receives invalid data
            true on success

*/

static const char *
none_string(const char *str)
{
   const char *ret = str;

   if (str == NULL || strlen(str) == 0) {
      ret = NONE_STR;
   }

   return ret;
}

const char *
sge_write_rusage(dstring *buffer, 
                 lListElem *jr, lListElem *job, lListElem *ja_task, 
                 const char *category_str, const char delimiter, 
                 bool intermediate)
{
   lList *usage_list, *reported_list;
   const char *pe_task_id;
#ifdef NEC_ACCOUNTING_ENTRIES
   char arch_dep_usage_buffer[MAX_STRING_SIZE];
   dstring arch_dep_usage_dstring;
   char *arch_dep_usage_string;
#endif
   const char *ret = NULL;
   char *qname = NULL;
   lListElem *pe_task = NULL;
   u_long32 submission_time = 0;
   u_long32 start_time      = 0;
   u_long32 end_time        = 0;
   u_long32 now             = sge_get_gmt();

   DENTER(TOP_LAYER, "sge_write_rusage");

   /* invalid input data */
   if (buffer == NULL) {
      DEXIT;   
      return ret;
   } 

#ifdef NEC_ACCOUNTING_ENTRIES
   sge_dstring_init(&arch_dep_usage_dstring, arch_dep_usage_buffer, 
                    sizeof(arch_dep_usage_buffer));
#endif

   /* for tasks we take usage from job report */
   if ((pe_task_id=lGetString(jr, JR_pe_task_id_str))) {
      pe_task = lGetElemStr(lGetList(ja_task, JAT_task_list), 
                                     PET_id, pe_task_id);
      if (pe_task == NULL) {
         ERROR((SGE_EVENT, "got usage report for unknown pe_task "SFN"\n",
                           pe_task_id));
         return ret;
      }
      usage_list = lGetList(pe_task, PET_usage);
   } else {
      usage_list = lGetList(ja_task, JAT_usage_list);
   }

   /* for intermediate usage reporting, we need a list containing the already
    * reported usage. If it doesn't exist yet, create it
    */
   if (intermediate) {
      if (pe_task != NULL) {
         reported_list = lGetList(pe_task, PET_reported_usage);
         if (reported_list == NULL) {
            reported_list = lCreateList("reported usage", UA_Type);
            lSetList(pe_task, PET_reported_usage, reported_list);
         }
      } else {
         reported_list = lGetList(ja_task, JAT_reported_usage_list);
         if (reported_list == NULL) {
            reported_list = lCreateList("reported usage", UA_Type);
            lSetList(ja_task, JAT_reported_usage_list, reported_list);
         }
      }

      /* now set actual time as time of last intermediate usage report */
      usage_list_set_ulong_usage(reported_list, LAST_INTERMEDIATE, 
                                 now);
   } else {
      reported_list = NULL;
   }


#if 0
   {
      lListElem *ep;

      if (usage_list) {
         DPRINTF(("received usage attributes:\n"));
      } else {
         DPRINTF(("empty usage list\n"));
      }   

      for_each (ep, usage_list) {
         DPRINTF(("    \"%s\" = %f\n",
            lGetString(ep, UA_name),
            lGetDouble(ep, UA_value)));
      }
   }
#endif

   SET_STR_DEFAULT(jr, JR_queue_name, "UNKNOWN");
   SET_HOST_DEFAULT(jr, JR_host_name,  "UNKNOWN");
   SET_STR_DEFAULT(jr, JR_group,      "UNKNOWN");
   SET_STR_DEFAULT(jr, JR_owner,      "UNKNOWN");
   
   /* job name and account get taken 
      from local job structure */
   if (!lGetString(job, JB_job_name)) 
      lSetString(job, JB_job_name, "UNKNOWN");
   if (!lGetString(job, JB_account)) 
      lSetString(job, JB_account, "UNKNOWN");

#ifdef NEC_ACCOUNTING_ENTRIES
   /* values which will be written for a special architecture */
   {
#if defined(NECSX4) || defined(NECSX5)
      char *arch_string = "";

      ep=lGetElemStr(usage_list, UA_name, "necsx_necsx4");
      if (ep)
         arch_string = "necsx4";
      ep=lGetElemStr(usage_list, UA_name, "necsx_necsx5");
      if (ep)
         arch_string = "necsx5";

      arch_dep_usage_string = sge_dstring_sprintf(&arch_dep_usage_dstring, 
         NECSX_ACTFILE_FPRINTF_FORMAT,
         arch_string,    
         usage_list_get_ulong_usage(usage_list, "necsx_base_prty", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_time_slice", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_num_procs", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_kcore_min", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_mean_size", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_maxmem_size", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_chars_trnsfd", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_blocks_rw", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_inst", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_vector_inst", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_vector_elmt", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_vec_exe", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_flops", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_conc_flops", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_fpec", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_cmcc", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_bccc", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_mt_open", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_io_blocks", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_multi_single", 0),
         usage_list_get_ulong_usage(usage_list, "necsx_max_nproc", 0)
      );
#endif
      DPRINTF(("arch_string: %s\n", arch_dep_usage_string));
   }
#endif 
   {
      char *pos = NULL;
      const char *qi_name = NULL;
      qi_name = lGetString(jr, JR_queue_name);
      qname = malloc(strlen(qi_name)+1);
      strcpy(qname, qi_name);
      if ( (pos = strchr(qname, '@'))){
         pos[0] = '\0';
      }
   }

   if (intermediate) {
      if (job != NULL) {
         submission_time = lGetUlong(job, JB_submission_time);
      }
      if (ja_task != NULL) {
         start_time = lGetUlong(ja_task, JAT_start_time);
      }
      end_time = now;
   } else {
      submission_time = usage_list_get_ulong_usage(usage_list, "submission_time", 0);
      start_time = usage_list_get_ulong_usage(usage_list, "start_time", 0);
      end_time = usage_list_get_ulong_usage(usage_list, "end_time", 0);
   }
   
   ret = sge_dstring_sprintf(buffer, ACTFILE_FPRINTF_FORMAT, 
         qname, delimiter,
          lGetHost(jr, JR_host_name), delimiter,
          lGetString(jr, JR_group), delimiter,
          lGetString(jr, JR_owner), delimiter,
          lGetString(job, JB_job_name), delimiter,
          lGetUlong(jr, JR_job_number), delimiter,
          lGetString(job, JB_account), delimiter,
          usage_list_get_ulong_usage(usage_list, "priority", 0),  delimiter,
          submission_time, delimiter,
          start_time, delimiter,
          end_time, delimiter,
          lGetUlong(jr, JR_failed), delimiter,
          usage_list_get_ulong_usage(usage_list, "exit_status", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_wallclock", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_utime", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_stime", 0), delimiter,
          usage_list_get_double_usage(usage_list, "ru_maxrss", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_ixrss", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_ismrss", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_idrss", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_isrss", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_minflt", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_majflt", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_nswap", 0), delimiter,
          usage_list_get_double_usage(usage_list, "ru_inblock", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_oublock", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_msgsnd", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_msgrcv", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_nsignals", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_nvcsw", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "ru_nivcsw", 0), delimiter,
          none_string(lGetString(job, JB_project)), delimiter,
          none_string(lGetString(job, JB_department)), delimiter,
          none_string(lGetString(ja_task, JAT_granted_pe)), delimiter,
          sge_granted_slots(lGetList(ja_task, JAT_granted_destin_identifier_list)), delimiter,
          job_is_array(job) ? lGetUlong(ja_task, JAT_task_number) : 0, delimiter,
          reporting_get_double_usage(usage_list, reported_list, 
            intermediate ? USAGE_ATTR_CPU : USAGE_ATTR_CPU_ACCT, 
            USAGE_ATTR_CPU, 0), delimiter,
          reporting_get_double_usage(usage_list, reported_list, 
             intermediate ? USAGE_ATTR_MEM : USAGE_ATTR_MEM_ACCT,
             USAGE_ATTR_MEM, 0), delimiter,
          reporting_get_double_usage(usage_list, reported_list, 
             intermediate ? USAGE_ATTR_IO : USAGE_ATTR_IO_ACCT, 
             USAGE_ATTR_IO, 0), delimiter,
          none_string(category_str), delimiter,
          reporting_get_double_usage(usage_list, reported_list, 
             intermediate ? USAGE_ATTR_IOW : USAGE_ATTR_IOW_ACCT, 
             USAGE_ATTR_IOW, 0), delimiter,
          none_string(pe_task_id), delimiter,
          usage_list_get_double_usage(usage_list,  
             intermediate ? USAGE_ATTR_MAXVMEM : USAGE_ATTR_MAXVMEM_ACCT, 0) 
#ifdef NEC_ACCOUNTING_ENTRIES
          , delimiter, arch_dep_usage_string
#endif 
             );
     
   FREE(qname);
   DEXIT;   
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_get_double_usage() is MT-safe
*/
static double 
reporting_get_double_usage (lList *usage_list, lList *reported_list, 
                            const char *name, const char *rname, double def) 
{
   double usage;

   /* total usage */
   usage = usage_list_get_double_usage(usage_list, name, def);

   if (reported_list != NULL) {
      double reported;

      /* usage already reported */
      reported = usage_list_get_double_usage(reported_list, rname, def);

      /* after this action, we'll have reported the total usage */
      usage_list_set_double_usage(reported_list, rname, usage);

      /* in this intermediate accounting record, we'll report the usage 
       * consumed since the last intermediate accounting record.
       */
      usage -= reported;
   }

   return usage;
}
