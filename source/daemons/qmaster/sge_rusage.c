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

#include "sge_ja_task.h"
#include "sge_usage.h"
#include "sge_rusage.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_job_schedd.h"
#include "sge_schedd.h"
#include "sge_job.h"
#include "sge_spool.h"
#include "sge_report.h"

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

/* ------------------------------------------------------------

   write usage to a dstring buffer

   sge_write_rusage - write rusage info to a dstring buffer
   Returns: false, if it receives invalid data
            true on success

*/
const char *
sge_write_rusage(dstring *buffer, 
                 lListElem *jr, lListElem *jep, lListElem *jatp, 
                 const char *category_str, const char delimiter)
{
   lList *usage_list;
   const char *s, *pe_task_id_str;
#ifdef NEC_ACCOUNTING_ENTRIES
   char arch_dep_usage_buffer[MAX_STRING_SIZE];
   dstring arch_dep_usage_dstring;
   char *arch_dep_usage_string;
#endif
   const char *ret = NULL;
   char *qname = NULL;

   DENTER(TOP_LAYER, "sge_write_rusage");

   /* invalid input data */
   if (buffer == NULL) {
      DEXIT;   
      return ret;
   } 

#ifdef NEC_ACCOUNTING_ENTRIES
   sge_dstring_init(&arch_dep_usage_dstring, arch_dep_usage_buffer, 
                    MAX_STRING_SIZE);
#endif

   /* for tasks we take usage from job report */
   if ((pe_task_id_str=lGetString(jr, JR_pe_task_id_str)))
      usage_list = lGetList(jr, JR_usage);
   else
      usage_list = lGetList(jatp, JAT_usage_list);

#if 1
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
   if (!lGetString(jep, JB_job_name)) 
      lSetString(jep, JB_job_name, "UNKNOWN");
   if (!lGetString(jep, JB_account)) 
      lSetString(jep, JB_account, "UNKNOWN");

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
   
   ret = sge_dstring_sprintf(buffer, ACTFILE_FPRINTF_FORMAT, 
         qname, delimiter,
/*          lGetString(jr, JR_queue_name), delimiter,*/
          lGetHost(jr, JR_host_name), delimiter,
          lGetString(jr, JR_group), delimiter,
          lGetString(jr, JR_owner), delimiter,
          lGetString(jep, JB_job_name), delimiter,
          lGetUlong(jr, JR_job_number), delimiter,
          lGetString(jep, JB_account), delimiter,
          usage_list_get_ulong_usage(usage_list, "priority", 0),  delimiter,
          usage_list_get_ulong_usage(usage_list, "submission_time", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "start_time", 0), delimiter,
          usage_list_get_ulong_usage(usage_list, "end_time", 0), delimiter,
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
          lGetString(jep, JB_project) ? lGetString(jep, JB_project) : "none", delimiter,
          lGetString(jep, JB_department) ? lGetString(jep, JB_department) : "none", delimiter,
          (s = lGetString(jatp, JAT_granted_pe)) ? s : "none", delimiter,
          sge_granted_slots(lGetList(jatp, JAT_granted_destin_identifier_list)), delimiter,
          job_is_array(jep) ? lGetUlong(jatp, JAT_task_number) : 0, delimiter,
          usage_list_get_double_usage(usage_list, USAGE_ATTR_CPU_ACCT, 0), delimiter,
          usage_list_get_double_usage(usage_list, USAGE_ATTR_MEM_ACCT, 0), delimiter,
          usage_list_get_double_usage(usage_list, USAGE_ATTR_IO_ACCT, 0), delimiter,
          category_str?category_str:"none", delimiter,
          usage_list_get_double_usage(usage_list, USAGE_ATTR_IOW_ACCT, 0), delimiter,
          pe_task_id_str?pe_task_id_str:"none", delimiter,
          usage_list_get_double_usage(usage_list, USAGE_ATTR_MAXVMEM_ACCT, 0)
#ifdef NEC_ACCOUNTING_ENTRIES
          , delimiter, arch_dep_usage_string
#endif 
             );
     
   FREE(qname);
   DEXIT;   
   return ret;
}

int sge_read_rusage(FILE *f, sge_rusage_type *d) 
{
   static char szLine[4092] = "";
   char  *pc;
   int len;

   DENTER(TOP_LAYER, "sge_read_rusage");

   do {
      pc = fgets(szLine, sizeof(szLine), f);
      if (pc == NULL) 
         return 2;
      len = strlen(szLine);
      if (szLine[len] == '\n')
         szLine[len] = '\0';
   } while (len <= 1 || szLine[0] == COMMENT_CHAR); 
   
   /*
    * qname
    */
   pc = strtok(szLine, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->qname = sge_strdup(d->qname, pc);
   
   /*
    * hostname
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->hostname = sge_strdup(d->hostname, pc);

   /*
    * group
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->group = sge_strdup(d->group, pc);
          
           
   /*
    * owner
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->owner = sge_strdup(d->owner, pc);

   /*
    * job_name
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->job_name = sge_strdup(d->job_name, pc);

   /*
    * job_number
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   
   d->job_number = atol(pc);
   
   /*
    * account
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->account = sge_strdup(d->account, pc);

   /*
    * priority
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->priority = atol(pc);

   /*
    * submission_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->submission_time = atol(pc);

   /*
    * start_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->start_time = atol(pc);

   /*
    * end_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->end_time = atol(pc);

   /*
    * failed
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->failed = atol(pc);

   /*
    * exit_status
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->exit_status = atol(pc);

   /*
    * ru_wallclock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_wallclock = atol(pc); 

   /*
    * ru_utime
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_utime = atol(pc);

   /*
    * ru_stime
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_stime = atol(pc);

   /*
    * ru_maxrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_maxrss = atol(pc);

   /*
    * ru_ixrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_ixrss = atol(pc);

   /*
    * ru_ismrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_ismrss = atol(pc);

   /*
    * ru_idrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_idrss = atol(pc);

   /*
    * ru_isrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_isrss = atol(pc);
   
   /*
    * ru_minflt
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_minflt = atol(pc);

   /*
    * ru_majflt
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_majflt = atol(pc);

   /*
    * ru_nswap
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_nswap = atol(pc);

   /*
    * ru_inblock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_inblock = atol(pc);

   /*
    * ru_oublock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_oublock = atol(pc);

   /*
    * ru_msgsnd
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_msgsnd = atol(pc);

   /*
    * ru_msgrcv
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_msgrcv = atol(pc);

   /*
    * ru_nsignals
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_nsignals = atol(pc);

   /*
    * ru_nvcsw
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_nvcsw = atol(pc);

   /*
    * ru_nivcsw
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->ru_nivcsw = atol(pc);

   /*
    * project
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->project = sge_strdup(d->project, pc);

   /*
    * department
    */
   pc = strtok(NULL, ":\n");
   if (!pc) {
      DEXIT;
      return -1;
   }
   d->department = sge_strdup(d->department, pc);

   /* PE name */
   pc = strtok(NULL, ":");
   if (pc)
      d->granted_pe = sge_strdup(d->granted_pe, pc);
   else
      d->granted_pe = sge_strdup(d->granted_pe, "none");   

   /* slots */
   pc = strtok(NULL, ":");
   if (pc)
      d->slots = atol(pc);
   else
      d->slots = 0;

   /* task number */
   pc = strtok(NULL, ":");
   if (pc)
      d->task_number = atol(pc);
   else
      d->task_number = 0;

   d->cpu = ((pc=strtok(NULL, ":")))?atof(pc):0;
   d->mem = ((pc=strtok(NULL, ":")))?atof(pc):0;
   d->io = ((pc=strtok(NULL, ":")))?atof(pc):0;

   /* skip job category */
   while ((pc=strtok(NULL, ":")) &&
          strlen(pc) &&
          pc[strlen(pc)-1] != ' ' &&
          strcmp(pc, "none")) {
      /*
       * The job category field might contain colons (':').
       * Therefore we have to skip all colons until we find a " :".
       * Only if the category is "none" then ":" is the real delimiter.
       */
      ;
   }

   d->iow = ((pc=strtok(NULL, ":")))?atof(pc):0;

   /* skip pe_taskid */
   pc=strtok(NULL, ":");

   d->maxvmem = ((pc=strtok(NULL, ":")))?atof(pc):0;

   /* ... */ 

   DEXIT;
   return 0;
}
