#ifndef __SGE_REPORTL_H
#define __SGE_REPORTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* 
** valid values for REP_type 
*/

/* REP_list is LR_Type */
#define NUM_REP_REPORT_LOAD    1

/* REP_list is ET_Type */
#define NUM_REP_REPORT_EVENTS  2

/* REP_list is CONF_Type */
#define NUM_REP_REPORT_CONF    3

/* REP_list is LIC_Type */
#define NUM_REP_REPORT_PROCESSORS 4

/* REP_list is JR_Type */
#define NUM_REP_REPORT_JOB     5

/* *INDENT-OFF* */ 

/*
 * definition for REP_Type, sge report type
 */
enum {
   REP_type = REP_LOWERBOUND,
   REP_host,
   REP_list,
   REP_version,
   REP_seqno
};

LISTDEF(REP_Type)
   SGE_ULONG(REP_type)        /* type of report, e.g. load report */
   SGE_HOST(REP_host)       /* hostname as it is seen by sender of report */ /* CR - hostname change */
   SGE_LIST(REP_list)         /* list type depends on REP_type */
   SGE_ULONG(REP_version)     /* used to report software version of execd */
   SGE_ULONG(REP_seqno)       /* used to recognize old reports sent by execd */
LISTEND 

NAMEDEF(REPN)
   NAME("REP_type")
   NAME("REP_host")
   NAME("REP_list")
   NAME("REP_version")
   NAME("REP_seqno")
NAMEEND

#define REPS sizeof(REPN)/sizeof(char*)

enum {
   JR_job_number = JR_LOWERBOUND,
   JR_ja_task_number,
   JR_queue_name,
   JR_host_name,
   JR_owner,
   JR_group,
   JR_state,
   JR_failed,
   JR_general_failure,
   JR_err_str,
   JR_usage,                 /* UA_Type */
   JR_job_pid,
   JR_ckpt_arena,
   JR_pe_task_id_str,
   JR_osjobid
};

LISTDEF(JR_Type)
   SGE_ULONG(JR_job_number)   /* Job to report */
   SGE_ULONG(JR_ja_task_number)       /* JobArray task to report */
   SGE_STRING(JR_queue_name)  /* Queue this job (tried to) run in */
   SGE_HOST(JR_host_name)   /* Host this job (tried to) run on */ /* CR - hostname change */
   SGE_STRING(JR_owner)       /* Owner (User) of this job */
   SGE_STRING(JR_group)       /* Owner (User) of this job */
   SGE_ULONG(JR_state)        /* either JRUNNING or JEXITING, JRUNNING sent 
                               * * as ack for jobdelivery and cyclic */
   SGE_ULONG(JR_failed)       /* FAILED_... */
   SGE_ULONG(JR_general_failure)      /* 1 -> general problem */
   SGE_STRING(JR_err_str)     /* describes failure */
   SGE_LIST(JR_usage)         /* used resources UA_Type */
   SGE_ULONG(JR_job_pid)      /* pid of job script */
   SGE_ULONG(JR_ckpt_arena)   /* if there is a checkpoint in the arena */
   SGE_STRING(JR_pe_task_id_str)
   /* string describing task from sight of PE
    * if this is non null this is a PE task */
   SGE_STRING(JR_osjobid)     /* string containing osjobid for ckpt jobs */
LISTEND

NAMEDEF(JRN)
   NAME("JR_job_number")
   NAME("JR_ja_task_number")
   NAME("JR_queue_name")
   NAME("JR_host_name")
   NAME("JR_owner")
   NAME("JR_group")
   NAME("JR_state")
   NAME("JR_failed")
   NAME("JR_general_failure")
   NAME("JR_err_str")
   NAME("JR_usage")
   NAME("JR_job_pid")
   NAME("JR_ckpt_arena")
   NAME("JR_pe_task_id_str")
   NAME("JR_osjobid")
NAMEEND

#define JRS sizeof(JRN)/sizeof(char*)

/*
 * definition for license report, still to be enhanced
 */
enum {
   LIC_processors = LIC_LOWERBOUND,
   LIC_arch
};

LISTDEF(LIC_Type)
   SGE_ULONG(LIC_processors)
   SGE_STRING(LIC_arch)
LISTEND 

NAMEDEF(LICN)
   NAME("LIC_processors")
   NAME("LIC_arch")
NAMEEND

#define LICS sizeof(LICN)/sizeof(char*)

enum {
   LR_name = LR_LOWERBOUND,
   LR_value,
   LR_global,
   LR_static,
   LR_host
};

LISTDEF(LR_Type)
   SGE_STRING(LR_name)
   SGE_STRING(LR_value)
   SGE_ULONG(LR_global)       /* ==1 global load value */
   SGE_ULONG(LR_static)       /* ==1 static load value */
   SGE_HOSTH(LR_host)        /* sender host of load value */  /* CR - hostname change */
LISTEND

NAMEDEF(LRN)
   NAME("LR_name")
   NAME("LR_value")
   NAME("LR_global")
   NAME("LR_static")
   NAME("LR_host")
NAMEEND

#define LRS sizeof(LRN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef __cplusplus
}
#endif

#endif                          /* __SGE_REPORTL_H */
