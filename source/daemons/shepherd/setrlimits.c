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
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(SUN4)
#   include <sys/time.h>
#endif

#if defined(CRAY)
#   include <sys/param.h>
#   include <sys/unistd.h>
#   include <sys/category.h>
#endif

#if defined(HP10_01) || defined(HPCONVEX)
#   define _KERNEL
#endif

#include <sys/resource.h>

#if defined(HP10_01) || defined(HPCONVEX)
#   undef _KERNEL
#endif

#if defined(IRIX6)
#   define RLIMIT_STRUCT_TAG rlimit64
#else
#   define RLIMIT_STRUCT_TAG rlimit
#endif

#include "basis_types.h"
#include "sge_parse_num_par.h"
#include "config_file.h"
#include "setrlimits.h"
#include "err_trace.h"
#include "sge_switch_user.h"
#include "setjoblimit.h"
#include "sge_nice.h"

#ifndef CRAY
static void pushlimit(int, struct RLIMIT_STRUCT_TAG *, int trace_rlimit);

static int get_resource_info(u_long32 resource, char **name, int *resource_type);
#endif

void setrlimits(
int trace_rlimit 
) {
   sge_rlim_t s_cpu, h_cpu, s_core, h_core, s_data, h_data, 
      s_fsize, h_fsize, s_stack, h_stack, s_vmem, h_vmem;
#if defined(NECSX4) || defined(NECSX5)
   sge_rlim_t s_tmpf, h_tmpf, s_mtdev, h_mtdev, s_nofile, h_nofile,
      s_proc, h_proc, s_rlg0, h_rlg0, s_rlg1, h_rlg1, s_rlg2, h_rlg2,
      s_rlg3, h_rlg3, s_cpurestm, h_cpurestm;
#endif  
#ifndef SINIX
   sge_rlim_t s_rss, h_rss; 
#endif

   int host_slots, priority;
   char *s, error_str[1024];

#ifdef CRAY
   long clock_tick;
#else
#if !defined(HPUX)
/*  */
   struct RLIMIT_STRUCT_TAG rlp;
#endif
#endif

#define PARSE_IT(dstp, attr) \
   s = get_conf_val(attr); \
   sge_parse_limit(dstp, s, error_str, sizeof(error_str));

   PARSE_IT(&s_cpu, "s_cpu");
   PARSE_IT(&h_cpu, "h_cpu");
   PARSE_IT(&s_core, "s_core");
   PARSE_IT(&h_core, "h_core");
   PARSE_IT(&s_data, "s_data");
   PARSE_IT(&h_data, "h_data");
   PARSE_IT(&s_stack, "s_stack");
   PARSE_IT(&h_stack, "h_stack");
#ifndef SINIX 
   PARSE_IT(&s_rss, "s_rss");
   PARSE_IT(&h_rss, "h_rss");
#endif
   PARSE_IT(&s_fsize, "s_fsize");
   PARSE_IT(&h_fsize, "h_fsize");
   PARSE_IT(&s_vmem, "s_vmem");
   PARSE_IT(&h_vmem, "h_vmem");
#if defined(NECSX4) || defined(NECSX5)
   PARSE_IT(&s_tmpf, "s_tmpf");
   PARSE_IT(&h_tmpf, "h_tmpf");
   PARSE_IT(&s_mtdev, "s_mtdev");
   PARSE_IT(&h_mtdev, "h_mtdev");
   PARSE_IT(&s_nofile, "s_nofile");
   PARSE_IT(&h_nofile, "h_nofile");
   PARSE_IT(&s_proc, "s_proc");
   PARSE_IT(&h_proc, "h_proc");
   PARSE_IT(&s_rlg0, "s_rlg0");
   PARSE_IT(&h_rlg0, "h_rlg0");
   PARSE_IT(&s_rlg1, "s_rlg1");
   PARSE_IT(&h_rlg1, "h_rlg1");
   PARSE_IT(&s_rlg2, "s_rlg2");
   PARSE_IT(&h_rlg2, "h_rlg2");
   PARSE_IT(&s_rlg3, "s_rlg3");
   PARSE_IT(&h_rlg3, "h_rlg3");
   PARSE_IT(&s_cpurestm, "s_cpurestm");
   PARSE_IT(&h_cpurestm, "h_cpurestm");
#endif

#define RL_MAX(r1, r2) ((rlimcmp((r1), (r2))>0)?(r1):(r2))
#define RL_MIN(r1, r2) ((rlimcmp((r1), (r2))<0)?(r1):(r2))
    /*
     * we have to define some minimum limits to make sure that
     * that the shepherd can run without trouble
     */
    s_vmem = RL_MAX(s_vmem, LIMIT_VMEM_MIN);
    h_vmem = RL_MAX(h_vmem, LIMIT_VMEM_MIN);
    s_data = RL_MAX(s_data, LIMIT_VMEM_MIN);
    h_data = RL_MAX(h_data, LIMIT_VMEM_MIN);
    s_rss = RL_MAX(s_rss, LIMIT_VMEM_MIN);
    h_rss = RL_MAX(h_rss, LIMIT_VMEM_MIN);
    s_stack = RL_MAX(s_stack, LIMIT_STACK_MIN);
    h_stack = RL_MAX(h_stack, LIMIT_STACK_MIN);
    s_cpu = RL_MAX(s_cpu, LIMIT_CPU_MIN);
    h_cpu = RL_MAX(h_cpu, LIMIT_CPU_MIN);
    s_fsize = RL_MAX(s_fsize, LIMIT_FSIZE_MIN);
    h_fsize = RL_MAX(h_fsize, LIMIT_FSIZE_MIN);   

   /* 
    * s_vmem > h_vmem
    * data segment limit > vmem limit
    * stack segment limit > vmem limit
    * causes problems that are difficult to find
    * we try to prevent these problems silently 
    */
   if (s_vmem > h_vmem) {
      s_vmem = h_vmem;
   }    
   s_data = RL_MIN(s_data, s_vmem);
   s_stack = RL_MIN(s_stack, s_vmem);
   h_data = RL_MIN(h_data, h_vmem);
   h_stack = RL_MIN(h_stack, h_vmem); 

   priority = atoi(get_conf_val("priority"));
   /* had problems doing this with admin user priviledges under HPUX */
   switch2start_user(); 
   SETPRIORITY(priority);
   switch2admin_user();  

   /* how many slots do we have at this host */
   if (!(s=search_nonone_conf_val("host_slots")) || !(host_slots=atoi(s)))
      host_slots = 1;

   /* for multithreaded jobs the per process limit
      must be available for each thread */
   s_cpu = mul_infinity(s_cpu, host_slots);
   h_cpu = mul_infinity(h_cpu, host_slots);
   s_vmem = mul_infinity(s_vmem, host_slots);
   h_vmem = mul_infinity(h_vmem, host_slots);
   s_data = mul_infinity(s_data, host_slots);
   h_data = mul_infinity(h_data, host_slots);
   s_stack = mul_infinity(s_stack, host_slots);
   h_stack = mul_infinity(h_stack, host_slots);

#if defined(CRAY)

   /* Let's play a game: UNICOS doesn't support hard and soft limits */
   /* but it has job and process limits. Let the soft limit setting  */
   /* in the queue configuration represent the per-process limit     */
   /* while the hard limit is the per-job limit. OK, so it's crude.  */

   /* Per-process limits */
   clock_tick = sysconf(_SC_CLK_TCK);
   limit(C_PROC, 0, L_CPU, s_cpu * clock_tick);
   limit(C_PROC, 0, L_CORE, s_core / NBPC);
   limit(C_PROC, 0, L_MEM, s_data / NBPC);

   /* Per-job limits */
   limit(C_JOB, 0, L_CPU, h_cpu * clock_tick);
   limit(C_JOB, 0, L_MEM, h_data / NBPC);
   limit(C_JOB, 0, L_FSBLK, h_fsize / (NBPC * NCPD));

   /* Too bad they didn't have a sysconf call to get bytes/click */
   /* and clicks/disk block. */

#elif !defined(HPUX)
   rlp.rlim_cur = s_cpu;
   rlp.rlim_max = h_cpu;
   pushlimit(RLIMIT_CPU, &rlp, trace_rlimit);

   rlp.rlim_cur = s_fsize;
   rlp.rlim_max = h_fsize;
   pushlimit(RLIMIT_FSIZE, &rlp, trace_rlimit);

   rlp.rlim_cur = s_data;
   rlp.rlim_max = h_data;
   pushlimit(RLIMIT_DATA, &rlp, trace_rlimit);

   rlp.rlim_cur = s_stack;
   rlp.rlim_max = h_stack;
   pushlimit(RLIMIT_STACK, &rlp, trace_rlimit);

   rlp.rlim_cur = s_core;
   rlp.rlim_max = h_core;
   pushlimit(RLIMIT_CORE, &rlp, trace_rlimit);

#  if defined(RLIMIT_VMEM)
   rlp.rlim_cur = s_vmem;
   rlp.rlim_max = h_vmem;
   pushlimit(RLIMIT_VMEM, &rlp, trace_rlimit);
#  elif defined(RLIMIT_AS)
   rlp.rlim_cur = s_vmem;
   rlp.rlim_max = h_vmem;
   pushlimit(RLIMIT_AS, &rlp, trace_rlimit);
#  endif

#  if defined(RLIMIT_RSS)
   rlp.rlim_cur = s_rss;
   rlp.rlim_max = h_rss;
   pushlimit(RLIMIT_RSS, &rlp, trace_rlimit);
#  endif

#if defined(NECSX4) || defined(NECSX5)
   rlp.rlim_cur = s_tmpf;
   rlp.rlim_max = h_tmpf;
   pushlimit(RLIMIT_TMPF, &rlp, trace_rlimit);
   rlp.rlim_cur = s_mtdev;
   rlp.rlim_max = h_mtdev;
   pushlimit(RLIMIT_MTDEV, &rlp, trace_rlimit);
   rlp.rlim_cur = s_nofile;
   rlp.rlim_max = h_nofile;
   pushlimit(RLIMIT_NOFILE, &rlp, trace_rlimit);
   rlp.rlim_cur = s_proc;
   rlp.rlim_max = h_proc;
   pushlimit(RLIMIT_PROC, &rlp, trace_rlimit);
   rlp.rlim_cur = s_rlg0;
   rlp.rlim_max = h_rlg0;
   pushlimit(RLIMIT_RLG0, &rlp, trace_rlimit);
   rlp.rlim_cur = s_rlg1;
   rlp.rlim_max = h_rlg1;
   pushlimit(RLIMIT_RLG1, &rlp, trace_rlimit);
   rlp.rlim_cur = s_rlg2;
   rlp.rlim_max = h_rlg2;
   pushlimit(RLIMIT_RLG2, &rlp, trace_rlimit);
   rlp.rlim_cur = s_rlg3;
   rlp.rlim_max = h_rlg3;
   pushlimit(RLIMIT_RLG3, &rlp, trace_rlimit);
   rlp.rlim_cur = s_cpurestm;
   rlp.rlim_max = h_cpurestm;
   pushlimit(RLIMIT_CPURESTM, &rlp, trace_rlimit);
#endif 

#endif
}

#ifndef CRAY
static int get_resource_info(
u_long32 resource,
char **name,
int *resource_type 
) {
   int is_job_resource_column;
   int row;

   /* *INDENT-OFF* */
   /* resource           resource_name              resource_type
                                                    NECSX 4/5
                                                    |         OTHER ARCHS
                                                    |         |          */
   struct resource_table_entry resource_table[] = {
      {RLIMIT_FSIZE,     "RLIMIT_FSIZE",            {RES_PROC, RES_PROC}},
      {RLIMIT_DATA,      "RLIMIT_DATA",             {RES_PROC, RES_PROC}},
      {RLIMIT_STACK,     "RLIMIT_STACK",            {RES_PROC, RES_PROC}},
      {RLIMIT_CORE,      "RLIMIT_CORE",             {RES_PROC, RES_PROC}},
      {RLIMIT_CPU,       "RLIMIT_CPU",              {RES_BOTH, RES_PROC}},
#if defined(RLIMIT_RSS)
      {RLIMIT_RSS,       "RLIMIT_RSS",              {RES_PROC, RES_PROC}},
#endif
#if defined(RLIMIT_VMEM)
      {RLIMIT_VMEM,      "RLIMIT_VMEM",             {RES_PROC, RES_PROC}},
#elif defined(RLIMIT_AS)
      {RLIMIT_AS,        "RLIMIT_VMEM/RLIMIT_AS",   {RES_PROC, RES_PROC}},
#endif
#if defined(NECSX4) || defined(NECSX5)
      {RLIMIT_TMPF,      "RLIMIT_TMPF",             {RES_JOB,  RES_PROC}},
      {RLIMIT_MTDEV,     "RLIMIT_MTDEV",            {RES_JOB,  RES_PROC}},
      {RLIMIT_NOFILE,    "RLIMIT_NOFILE",           {RES_BOTH, RES_PROC}},
      {RLIMIT_PROC,      "RLIMIT_PROC",             {RES_BOTH, RES_PROC}},
      {RLIMIT_RLG0,      "RLIMIT_RLG0",             {RES_JOB,  RES_PROC}},
      {RLIMIT_RLG1,      "RLIMIT_RLG1",             {RES_JOB,  RES_PROC}},
      {RLIMIT_RLG2,      "RLIMIT_RLG2",             {RES_JOB,  RES_PROC}},
      {RLIMIT_RLG3,      "RLIMIT_RLG3",             {RES_JOB,  RES_PROC}},
      {RLIMIT_CPURESTM,  "RLIMIT_CPURESTM",         {RES_JOB,  RES_PROC}},
#endif
      {0,                NULL,                      {0, 0}}
   };
   /* *INDENT-ON* */

#if defined(NECSX4) || defined(NECSX5)
   is_job_resource_column = 0;
#else
   is_job_resource_column = 1;
#endif

   row = 0;
   while (resource_table[row].resource_name) {
      if (resource == resource_table[row].resource) {
         *name = resource_table[row].resource_name;
         *resource_type =
            resource_table[row].resource_type[is_job_resource_column];
         return 0;
      }
      row++;
   }
   *name = "unknown";
   return 1;       
}
#endif

/* The following is due to problems with parallel jobs on 5.x IRIXes (and
 * possibly above): On such  systems  the  upper  bounds  for  resource
 * limits are set by the kernel. If  limits  are  set  above  the  kernel
 * boundaries (e.g. unlimited) multiprocessing applications may be aborted
 * ("memory too low to grow stack" was one of the messages we saw).
 *
 * For such systems pushlimit doesn't set limits above the hard limit
 * boundaries retrieved by a preceeding getrlimit call. Note that, as a
 * consequence, the limits must be set appropriately at the start of the
 * daemons calling mkprivileged.
 *
 * For other systems pushlimit just calls setrlimit.
 */


#if !defined(CRAY) && !defined(HPUX)
static void pushlimit(
int resource,
struct RLIMIT_STRUCT_TAG *rlp,
int trace_rlimit 
) {
   char *limit_str;
   char trace_str[1024];
   struct RLIMIT_STRUCT_TAG dlp;
   int resource_type;
   int ret;

   if (get_resource_info(resource, &limit_str, &resource_type)) {
      sprintf(trace_str, "no %d-resource-limits set because "
         "unknown resource", resource);
      shepherd_trace(trace_str);
      return;
   }

   /* Process limit */
   if ((resource_type & RES_PROC)) {
#if defined(IRIX5) || defined(IRIX6)
#if defined(IRIX5)
      getrlimit(resource,&dlp);
#else
      getrlimit64(resource,&dlp);
#endif
      if (rlp->rlim_cur>dlp.rlim_max)
         rlp->rlim_cur=dlp.rlim_max;
      if (rlp->rlim_max>dlp.rlim_max)
         rlp->rlim_max=dlp.rlim_max;
#endif

      /* hard limit must be greater or equal to soft limit */
      if (rlp->rlim_max < rlp->rlim_cur)
         rlp->rlim_cur = rlp->rlim_max;

#if defined(LINUX) || ( defined(SOLARIS) && !defined(SOLARIS64) ) || defined(NECSX4) || defined(NECSX5)
#  define limit_fmt "%ld"
#elif defined(IRIX6) || defined(HP11) || defined(HP10)
#  define limit_fmt "%lld"
#elif defined(ALPHA) || defined(SOLARIS64)
#  define limit_fmt "%lu"
#else
#  define limit_fmt "%d"
#endif

      switch2start_user();
#ifdef IRIX6
      ret = setrlimit64(resource, rlp);
#else
      ret = setrlimit(resource,rlp);
#endif
      switch2admin_user();  
      if (ret) {
         /* exit or not exit ? */
         sprintf(trace_str, "setrlimit(%s, {"limit_fmt", "limit_fmt"}) failed: %s"
,
            limit_str, rlp->rlim_cur, rlp->rlim_max, strerror(errno));
            shepherd_trace(trace_str);
      }
      else {
#ifdef IRIX6
         getrlimit64(resource,&dlp);
#else
         getrlimit(resource,&dlp);
#endif
      }

      if (trace_rlimit) {
         sprintf(trace_str, "%s setting: (soft "limit_fmt" hard "limit_fmt") "
            "resulting: (soft "limit_fmt" hard "limit_fmt")",
            limit_str,
            rlp->rlim_cur,
            rlp->rlim_max,
            dlp.rlim_cur,
            dlp.rlim_max);
         shepherd_trace(trace_str);
      }
   }

   /* Job limit */
   if (get_rlimits_os_job_id() && (resource_type & RES_JOB)) {
#if defined(NECSX4) || defined(NECSX5)
      getrlimitj(get_rlimits_os_job_id(), resource,&dlp);
#endif

      /* hard limit must be greater or equal to soft limit */
      if (rlp->rlim_max < rlp->rlim_cur)
         rlp->rlim_cur = rlp->rlim_max;

      if (
#if defined(NECSX4) || defined(NECSX5)
         setrlimitj(get_rlimits_os_job_id(), resource, rlp)
#else
         0
#endif
      ) {
         /* exit or not exit ? */
         sprintf(trace_str, "setrlimitj(%s, {"limit_fmt", "limit_fmt"}) "
            "failed: %s", limit_str, rlp->rlim_cur, rlp->rlim_max,
            strerror(errno));
      } else {         
#if defined(NECSX4) || defined(NECSX5)
         getrlimitj(get_rlimits_os_job_id(), resource,&dlp);
#endif
      }

      if (trace_rlimit) {
         sprintf(trace_str, "Job %s setting: (soft "limit_fmt" hard "limit_fmt
            ") resulting: (soft "limit_fmt" hard "limit_fmt")",
            limit_str,
            rlp->rlim_cur,
            rlp->rlim_max,
            dlp.rlim_cur,
            dlp.rlim_max);
         shepherd_trace(trace_str);
      }
   }
}
#endif 
