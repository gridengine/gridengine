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
 *  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *  Copyright: 2001 by Sun Microsystems, Inc.
 *
 *  All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(DARWIN) || defined(FREEBSD) || defined(NETBSD)
#   include <sys/time.h>
#endif

#if defined(CRAY)
#   include <sys/param.h>
#   include <sys/unistd.h>
#   include <sys/category.h>
#endif

#include <sys/resource.h>

#if defined(IRIX)
#   define RLIMIT_STRUCT_TAG rlimit64
#   define RLIMIT_INFINITY RLIM64_INFINITY
#else
#   define RLIMIT_STRUCT_TAG rlimit
#   define RLIMIT_INFINITY RLIM_INFINITY
#endif

/* Format the value, if val == INFINITY, print INFINITY for logs sake */
#define FORMAT_LIMIT(x) (x==RLIMIT_INFINITY)?0:x, (x==RLIMIT_INFINITY)?"\bINFINITY":""

#if defined(DARWIN)
#   include <sys/time.h>
#   include <sys/types.h>
#endif


#include "basis_types.h"
#include "sge_parse_num_par.h"
#include "config_file.h"
#include "setrlimits.h"
#include "err_trace.h"
#include "setjoblimit.h"
#include "sge_uidgid.h"
#include "sge_os.h"

#ifndef CRAY
static void pushlimit(int, struct RLIMIT_STRUCT_TAG *, int trace_rlimit);

static int get_resource_info(u_long32 resource, const char **name, int *resource_type);
#endif

static int rlimcmp(sge_rlim_t r1, sge_rlim_t r2);
static int sge_parse_limit(sge_rlim_t *rlvalp, char *s, char *error_str,
                           int error_len);

/*
 * compare two sge_rlim_t values
 *
 * returns
 * r1 <  r2      < 0
 * r1 == r2      == 0
 * r1 >  r2      > 0
 */
static int rlimcmp(sge_rlim_t r1, sge_rlim_t r2) 
{
   if (r1 == r2)
      return 0;
   if (r1==RLIM_INFINITY)
      return 1;
   if (r2==RLIM_INFINITY)
      return -1;
   return (r1>r2)?1:-1;
}

/* -----------------------------------------

NAME
   sge_parse_limit()

DESCR
   is a wrapper around sge_parse_num_val()
   for migration to code that returns an
   error and does not exit()

PARAM
   uvalp - where to write the parsed value
   s     - string to parse

RETURN
      1 - ok, value in *uvalp is valid
      0 - parsing error

*/
static int sge_parse_limit(sge_rlim_t *rlvalp, char *s, char *error_str,
                    int error_len)
{
   sge_parse_num_val(rlvalp, NULL, s, s, error_str, error_len);

   return 1;
}

void setrlimits(int trace_rlimit) {
   sge_rlim_t s_cpu, s_cpu_is_consumable_job;
   sge_rlim_t h_cpu, h_cpu_is_consumable_job;

   sge_rlim_t s_data, s_data_is_consumable_job;
   sge_rlim_t h_data, h_data_is_consumable_job;

   sge_rlim_t s_stack, s_stack_is_consumable_job;
   sge_rlim_t h_stack, h_stack_is_consumable_job;

   sge_rlim_t s_vmem, s_vmem_is_consumable_job;
   sge_rlim_t h_vmem, h_vmem_is_consumable_job;

   sge_rlim_t s_fsize;
   sge_rlim_t h_fsize;

   sge_rlim_t s_core;
   sge_rlim_t h_core;

   sge_rlim_t s_descriptors;
   sge_rlim_t h_descriptors;

   sge_rlim_t s_maxproc;
   sge_rlim_t h_maxproc;

   sge_rlim_t s_memorylocked;
   sge_rlim_t h_memorylocked;

   sge_rlim_t s_locks;
   sge_rlim_t h_locks;

#if defined(NECSX4) || defined(NECSX5)
   sge_rlim_t s_tmpf, h_tmpf, s_mtdev, h_mtdev, s_nofile, h_nofile,
      s_proc, h_proc, s_rlg0, h_rlg0, s_rlg1, h_rlg1, s_rlg2, h_rlg2,
      s_rlg3, h_rlg3, s_cpurestm, h_cpurestm;
#endif  
#ifndef SINIX
   sge_rlim_t s_rss; 
   sge_rlim_t h_rss; 
#endif

   int host_slots, priority;
   char *s, error_str[1024];

#ifdef CRAY
   long clock_tick;
#else
   struct RLIMIT_STRUCT_TAG rlp;
#endif

#define PARSE_IT(dstp, attr) \
   s = get_conf_val(attr); \
   sge_parse_limit(dstp, s, error_str, sizeof(error_str));

#define PARSE_IT_UNDEF(dstp, attr) \
   s = get_conf_val(attr); \
   if (!strncasecmp(s, "UNDEFINED", sizeof("UNDEFINED")-1)) { \
      *dstp = RLIMIT_UNDEFINED; \
   } else { \
      sge_parse_limit(dstp, s, error_str, sizeof(error_str)); \
   }
   /*
    * Process complex values with attribute consumble that
    * are subject to scaling by slots.
    */
   PARSE_IT(&h_vmem, "h_vmem");
   PARSE_IT(&h_vmem_is_consumable_job, "h_vmem_is_consumable_job");
   PARSE_IT(&s_vmem, "s_vmem");
   PARSE_IT(&s_vmem_is_consumable_job, "s_vmem_is_consumable_job");

   PARSE_IT(&s_cpu, "s_cpu");
   PARSE_IT(&s_cpu_is_consumable_job, "s_cpu_is_consumable_job");
   PARSE_IT(&h_cpu, "h_cpu");
   PARSE_IT(&h_cpu_is_consumable_job, "h_cpu_is_consumable_job");

   PARSE_IT(&s_data, "s_data");
   PARSE_IT(&s_data_is_consumable_job, "s_data_is_consumable_job");
   PARSE_IT(&h_data, "h_data");
   PARSE_IT(&h_data_is_consumable_job, "h_data_is_consumable_job");

   PARSE_IT(&s_stack, "s_stack");
   PARSE_IT(&s_stack_is_consumable_job, "s_stack_is_consumable_job");
   PARSE_IT(&h_stack, "h_stack");
   PARSE_IT(&h_stack_is_consumable_job, "h_stack_is_consumable_job");
   /*
    * Process regular complex values.
    */
   PARSE_IT(&s_core, "s_core");
   PARSE_IT(&h_core, "h_core");

   PARSE_IT(&s_rss, "s_rss");
   PARSE_IT(&h_rss, "h_rss");

   PARSE_IT(&s_fsize, "s_fsize");
   PARSE_IT(&h_fsize, "h_fsize");

   PARSE_IT_UNDEF(&s_descriptors, "s_descriptors");
   PARSE_IT_UNDEF(&h_descriptors, "h_descriptors");
   PARSE_IT_UNDEF(&s_maxproc,     "s_maxproc");
   PARSE_IT_UNDEF(&h_maxproc,     "h_maxproc");
   PARSE_IT_UNDEF(&s_memorylocked, "s_memorylocked");
   PARSE_IT_UNDEF(&h_memorylocked, "h_memorylocked");
   PARSE_IT_UNDEF(&s_locks,        "s_locks");
   PARSE_IT_UNDEF(&h_locks,        "h_locks");
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
   if (s_descriptors != RLIMIT_UNDEFINED) {
      s_descriptors = RL_MAX(s_descriptors, LIMIT_DESCR_MIN);
      s_descriptors = RL_MIN(s_descriptors, LIMIT_DESCR_MAX);
   }
   if (h_descriptors != RLIMIT_UNDEFINED) {
      h_descriptors = RL_MAX(h_descriptors, LIMIT_DESCR_MIN);
      s_descriptors = RL_MIN(s_descriptors, LIMIT_DESCR_MAX);
   }
   if (s_maxproc != RLIMIT_UNDEFINED) {
      s_maxproc = RL_MAX(s_maxproc, LIMIT_PROC_MIN);
   }
   if (h_maxproc != RLIMIT_UNDEFINED) {
      h_maxproc = RL_MAX(h_maxproc, LIMIT_PROC_MIN);
   }
   if (s_memorylocked != RLIMIT_UNDEFINED) {
      s_memorylocked = RL_MAX(s_memorylocked, LIMIT_MEMLOCK_MIN);
   }
   if (h_memorylocked != RLIMIT_UNDEFINED) {
      h_memorylocked = RL_MAX(h_memorylocked, LIMIT_MEMLOCK_MIN);
   }
   if (s_locks != RLIMIT_UNDEFINED) {
      s_locks = RL_MAX(s_locks, LIMIT_LOCKS_MIN);
   }
   if (h_locks != RLIMIT_UNDEFINED) {
      h_locks = RL_MAX(h_locks, LIMIT_LOCKS_MIN);
   }

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
   /*s_data = RL_MIN(s_data, s_vmem);
   s_stack = RL_MIN(s_stack, s_vmem);
   h_data = RL_MIN(h_data, h_vmem);
   h_stack = RL_MIN(h_stack, h_vmem);*/ 

   priority = atoi(get_conf_val("priority"));
   /* had problems doing this with admin user priviledges under HPUX */
   sge_switch2start_user(); 
   SETPRIORITY(priority);
   sge_switch2admin_user();  

   /* how many slots do we have at this host */
   if (!(s=search_nonone_conf_val("host_slots")) || !(host_slots=atoi(s)))
      host_slots = 1;

   /* for multithreaded jobs the per process limit
      must be available for each thread */
   /*
    * Scale resource by slots for type other than CONSUMABLE_JOB.
    */
#define CHECK_FOR_CONSUMABLE_JOB(A) \
   (A##_is_consumable_job) ? (A=mul_infinity(A,1)):(A=mul_infinity(A, host_slots));

   CHECK_FOR_CONSUMABLE_JOB(h_cpu);
   CHECK_FOR_CONSUMABLE_JOB(s_cpu);

   CHECK_FOR_CONSUMABLE_JOB(h_vmem);
   CHECK_FOR_CONSUMABLE_JOB(s_vmem);

   CHECK_FOR_CONSUMABLE_JOB(h_data);
   CHECK_FOR_CONSUMABLE_JOB(s_data);

   CHECK_FOR_CONSUMABLE_JOB(h_stack);
   CHECK_FOR_CONSUMABLE_JOB(s_stack);

   if (s_descriptors != RLIMIT_UNDEFINED) {
      s_descriptors = mul_infinity(s_descriptors, host_slots);
   }
   if (h_descriptors != RLIMIT_UNDEFINED) {
      h_descriptors = mul_infinity(h_descriptors, host_slots);
   }
   if (s_maxproc != RLIMIT_UNDEFINED) {
      s_maxproc = mul_infinity(s_maxproc, host_slots);
   }
   if (h_maxproc != RLIMIT_UNDEFINED) {
      h_maxproc = mul_infinity(h_maxproc, host_slots);
   }
   if (s_memorylocked != RLIMIT_UNDEFINED) {
      s_memorylocked = mul_infinity(s_memorylocked, host_slots);
   }
   if (h_memorylocked != RLIMIT_UNDEFINED) {
      h_memorylocked = mul_infinity(h_memorylocked, host_slots);
   }
   if (s_locks != RLIMIT_UNDEFINED) {
      s_locks = mul_infinity(s_locks, host_slots);
   }
   if (h_locks != RLIMIT_UNDEFINED) {
      h_locks = mul_infinity(h_locks, host_slots);
   }

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

#else
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

#  if defined(RLIMIT_NOFILE)
   if (s_descriptors != RLIMIT_UNDEFINED && h_descriptors == RLIMIT_UNDEFINED) {
      h_descriptors = s_descriptors;
   }
   if (h_descriptors != RLIMIT_UNDEFINED && s_descriptors == RLIMIT_UNDEFINED) {
      s_descriptors = h_descriptors;
   }
   if (s_descriptors != RLIMIT_UNDEFINED && h_descriptors != RLIMIT_UNDEFINED) {
      rlp.rlim_cur = s_descriptors;
      rlp.rlim_max = h_descriptors;
      pushlimit(RLIMIT_NOFILE, &rlp, trace_rlimit);
   }
#  endif

#  if defined(RLIMIT_NPROC)
   if (s_maxproc != RLIMIT_UNDEFINED && h_maxproc == RLIMIT_UNDEFINED) {
      h_maxproc = s_maxproc;
   }
   if (h_maxproc != RLIMIT_UNDEFINED && s_maxproc == RLIMIT_UNDEFINED) {
      s_maxproc = h_maxproc;
   }
   if (s_maxproc != RLIMIT_UNDEFINED && h_maxproc != RLIMIT_UNDEFINED) {
      rlp.rlim_cur = s_maxproc;
      rlp.rlim_max = h_maxproc;
      pushlimit(RLIMIT_NPROC, &rlp, trace_rlimit);
   }
#  endif

#  if defined(RLIMIT_MEMLOCK)
   if (s_memorylocked != RLIMIT_UNDEFINED && h_memorylocked == RLIMIT_UNDEFINED) {
      h_memorylocked = s_memorylocked;
   }
   if (h_memorylocked != RLIMIT_UNDEFINED && s_memorylocked == RLIMIT_UNDEFINED) {
      s_memorylocked = h_memorylocked;
   }
   if (s_memorylocked != RLIMIT_UNDEFINED && h_memorylocked != RLIMIT_UNDEFINED) {
      rlp.rlim_cur = s_memorylocked;
      rlp.rlim_max = h_memorylocked;
      pushlimit(RLIMIT_MEMLOCK, &rlp, trace_rlimit);
   }
# endif

#  if defined(RLIMIT_LOCKS)
   if (s_locks != RLIMIT_UNDEFINED && h_locks == RLIMIT_UNDEFINED) {
      h_locks = s_locks;
   }
   if (h_locks != RLIMIT_UNDEFINED && s_locks == RLIMIT_UNDEFINED) {
      s_locks = h_locks;
   }
   if (s_locks != RLIMIT_UNDEFINED && h_locks != RLIMIT_UNDEFINED) {
      rlp.rlim_cur = s_locks;
      rlp.rlim_max = h_locks;
      pushlimit(RLIMIT_LOCKS, &rlp, trace_rlimit);
   }
#  endif

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
/* *INDENT-OFF* */
/* resource           resource_name              resource_type
                                                 NECSX 4/5
                                                 |         OTHER ARCHS
                                                 |         |          */
const struct resource_table_entry resource_table[] = {
   {RLIMIT_FSIZE,     "RLIMIT_FSIZE",            {RES_PROC, RES_PROC}},
   {RLIMIT_DATA,      "RLIMIT_DATA",             {RES_PROC, RES_PROC}},
   {RLIMIT_STACK,     "RLIMIT_STACK",            {RES_PROC, RES_PROC}},
   {RLIMIT_CORE,      "RLIMIT_CORE",             {RES_PROC, RES_PROC}},
   {RLIMIT_CPU,       "RLIMIT_CPU",              {RES_BOTH, RES_PROC}},
#if defined(RLIMIT_NPROC)
   {RLIMIT_NPROC,     "RLIMIT_NPROC",            {RES_PROC, RES_PROC}},
#endif
#if defined(RLIMIT_MEMLOCK)
   {RLIMIT_MEMLOCK,   "RLIMIT_MEMLOCK",          {RES_PROC, RES_PROC}},
#endif
#if defined(RLIMIT_LOCKS)
   {RLIMIT_LOCKS,     "RLIMIT_LOCKS",            {RES_PROC, RES_PROC}},
#endif
#if defined(RLIMIT_NOFILE)
   {RLIMIT_NOFILE,    "RLIMIT_NOFILE",           {RES_BOTH, RES_PROC}},
#endif
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
   {RLIMIT_PROC,      "RLIMIT_PROC",             {RES_JOB,  RES_PROC}},
   {RLIMIT_RLG0,      "RLIMIT_RLG0",             {RES_JOB,  RES_PROC}},
   {RLIMIT_RLG1,      "RLIMIT_RLG1",             {RES_JOB,  RES_PROC}},
   {RLIMIT_RLG2,      "RLIMIT_RLG2",             {RES_JOB,  RES_PROC}},
   {RLIMIT_RLG3,      "RLIMIT_RLG3",             {RES_JOB,  RES_PROC}},
   {RLIMIT_CPURESTM,  "RLIMIT_CPURESTM",         {RES_JOB,  RES_PROC}},
#endif
   {0,                NULL,                      {0, 0}}
};
const char *unknown_string = "unknown";
/* *INDENT-ON* */

static int get_resource_info(u_long32 resource, const char **name, 
                             int *resource_type) 
{
   int is_job_resource_column;
   int row;

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
   *name = unknown_string;
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


#if !defined(CRAY)
static void pushlimit(int resource, struct RLIMIT_STRUCT_TAG *rlp, 
                      int trace_rlimit) 
{
   const char *limit_str;
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
#if defined(IRIX6)
      getrlimit64(resource,&dlp);
      if (rlp->rlim_cur>dlp.rlim_max)
         rlp->rlim_cur=dlp.rlim_max;
      if (rlp->rlim_max>dlp.rlim_max)
         rlp->rlim_max=dlp.rlim_max;
#endif

      /* hard limit must be greater or equal to soft limit */
      if (rlp->rlim_max < rlp->rlim_cur)
         rlp->rlim_cur = rlp->rlim_max;

#if defined(NECSX4) || defined(NECSX5) || defined(NETBSD_ALPHA) || defined(NETBSD_X86_64) || defined(NETBSD_SPARC64)
#  define limit_fmt "%ld%s"
#elif defined(IRIX) || defined(HPUX) || defined(DARWIN) || defined(FREEBSD) || defined(NETBSD) || defined(INTERIX)
#  define limit_fmt "%lld%s"
#elif (defined(LINUX) && defined(TARGET_32BIT))
#  define limit_fmt "%llu%s"
#elif defined(ALPHA) || defined(SOLARIS) || defined(LINUX)
#  define limit_fmt "%lu%s"
#else
#  define limit_fmt "%d%s"
#endif

      sge_switch2start_user();
#if defined(IRIX)
      ret = setrlimit64(resource, rlp);
#else
      ret = setrlimit(resource,rlp);
#endif
      sge_switch2admin_user();  
      if (ret) {
         /* exit or not exit ? */
         sprintf(trace_str, "setrlimit(%s, {"limit_fmt", "limit_fmt"}) failed: %s",
            limit_str, FORMAT_LIMIT(rlp->rlim_cur), FORMAT_LIMIT(rlp->rlim_max), strerror(errno));
            shepherd_trace(trace_str);
      } else {
#if defined(IRIX)
         getrlimit64(resource,&dlp);
#else
         getrlimit(resource,&dlp);
#endif
      }

      if (trace_rlimit) {
         sprintf(trace_str, "%s setting: (soft "limit_fmt" hard "limit_fmt") "
            "resulting: (soft "limit_fmt" hard "limit_fmt")",
            limit_str,
            FORMAT_LIMIT(rlp->rlim_cur),
            FORMAT_LIMIT(rlp->rlim_max),
            FORMAT_LIMIT(dlp.rlim_cur),
            FORMAT_LIMIT(dlp.rlim_max));
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

#if defined(NECSX4) || defined(NECSX5)
      /*
       * SUPER-UX only allows uid==euid==0 (superuser) to set
       * job limits thus the switch2start_user() and
       * switch2admin_user() calls
       */
      sge_switch2start_user();
      if (setrlimitj(get_rlimits_os_job_id(), resource, rlp)) {
         /* exit or not exit ? */
         sprintf(trace_str, "setrlimitj(%s, {"limit_fmt", "limit_fmt"}) "
            "failed: %s", limit_str, FORMAT_LIMIT(rlp->rlim_cur), FORMAT_LIMIT(rlp->rlim_max),
            strerror(errno));
      } else {
         getrlimitj(get_rlimits_os_job_id(), resource,&dlp);
      }
      sge_switch2admin_user();
#endif

      if (trace_rlimit) {
         sprintf(trace_str, "Job %s setting: (soft "limit_fmt" hard "limit_fmt
            ") resulting: (soft "limit_fmt" hard "limit_fmt")",
            limit_str,
            FORMAT_LIMIT(rlp->rlim_cur),
            FORMAT_LIMIT(rlp->rlim_max),
            FORMAT_LIMIT(dlp.rlim_cur),
            FORMAT_LIMIT(dlp.rlim_max));
         shepherd_trace(trace_str);
      }
   }
}
#endif 
