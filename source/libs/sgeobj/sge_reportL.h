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
   SGE_ULONG(REP_type, CULL_DEFAULT)        /* type of report, e.g. load report */
   SGE_HOST(REP_host, CULL_DEFAULT)       /* hostname as it is seen by sender of report */ /* CR - hostname change */
   SGE_LIST(REP_list, CULL_ANY_SUBTYPE, CULL_DEFAULT)         /* list type depends on REP_type */
   SGE_ULONG(REP_version, CULL_DEFAULT)     /* used to report software version of execd */
   SGE_ULONG(REP_seqno, CULL_DEFAULT)       /* used to recognize old reports sent by execd */
LISTEND 

NAMEDEF(REPN)
   NAME("REP_type")
   NAME("REP_host")
   NAME("REP_list")
   NAME("REP_version")
   NAME("REP_seqno")
NAMEEND

#define REPS sizeof(REPN)/sizeof(char*)

#define SGE_WEXITED_BIT      0x00000001
#define SGE_WSIGNALED_BIT    0x00000002
#define SGE_WCOREDUMP_BIT    0x00000004
#define SGE_NEVERRAN_BIT     0x00000008
/* POSIX exit status has only 8 bit */
#define SGE_EXIT_STATUS_BITS 0x00000FF0
/* SGE signal numbers are high numbers so we use 16 bit */
#define SGE_SIGNAL_BITS      0x0FFFF000

/* these makros shall be used for read access on JR_wait_status */
#define SGE_GET_WEXITED(status)   ((status)&SGE_WEXITED_BIT)
#define SGE_GET_WSIGNALED(status) ((status)&SGE_WSIGNALED_BIT)
#define SGE_GET_WCOREDUMP(status)   ((status)&SGE_WCOREDUMP_BIT)
#define SGE_GET_NEVERRAN(status)    ((status)&SGE_NEVERRAN_BIT)
#define SGE_GET_WEXITSTATUS(status) (((status)&SGE_EXIT_STATUS_BITS)>>4)
#define SGE_GET_WSIGNAL(status)     (((status)&SGE_SIGNAL_BITS)>>12)

/* these makros shall be used for write access on JR_wait_status */
#define SGE_SET_WEXITED(status, flag) \
   ((status) & ~SGE_WEXITED_BIT)   | ((flag)?SGE_WEXITED_BIT:0)
#define SGE_SET_WSIGNALED(status, flag) \
   ((status) & ~SGE_WSIGNALED_BIT) | ((flag)?SGE_WSIGNALED_BIT:0)
#define SGE_SET_WCOREDUMP(status, flag) \
   ((status) & ~SGE_WCOREDUMP_BIT) | ((flag)?SGE_WCOREDUMP_BIT:0)
#define SGE_SET_NEVERRAN(status, flag) \
   ((status) & ~SGE_NEVERRAN_BIT)  | ((flag)?SGE_NEVERRAN_BIT:0)
#define SGE_SET_WEXITSTATUS(status, exit_status) \
   ((status) & ~SGE_EXIT_STATUS_BITS)  |(((exit_status)<<4) & SGE_EXIT_STATUS_BITS)
#define SGE_SET_WSIGNAL(status, signal) \
   ((status) & ~SGE_SIGNAL_BITS)       |(((signal)<<12) & SGE_SIGNAL_BITS)

/*
 * definition for job report
 */
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
   JR_osjobid,
   JR_wait_status
};

LISTDEF(JR_Type)
   SGE_ULONG(JR_job_number, CULL_DEFAULT)   /* Job to report */
   SGE_ULONG(JR_ja_task_number, CULL_DEFAULT)       /* JobArray task to report */
   SGE_STRING(JR_queue_name, CULL_DEFAULT)  /* Queue this job (tried to) run in */
   SGE_HOST(JR_host_name, CULL_DEFAULT)   /* Host this job (tried to) run on */ /* CR - hostname change */
   SGE_STRING(JR_owner, CULL_DEFAULT)       /* Owner (User) of this job */
   SGE_STRING(JR_group, CULL_DEFAULT)       /* Owner (User) of this job */
   SGE_ULONG(JR_state, CULL_DEFAULT)        /* either JRUNNING or JEXITING, JRUNNING sent 
                               * * as ack for jobdelivery and cyclic */
   SGE_ULONG(JR_failed, CULL_DEFAULT)       /* FAILED_... */
   SGE_ULONG(JR_general_failure, CULL_DEFAULT)      /* 1 -> general problem */
   SGE_STRING(JR_err_str, CULL_DEFAULT)     /* describes failure */
   SGE_LIST(JR_usage, UA_Type, CULL_DEFAULT)         /* used resources UA_Type */
   SGE_ULONG(JR_job_pid, CULL_DEFAULT)      /* pid of job script */
   SGE_ULONG(JR_ckpt_arena, CULL_DEFAULT)   /* if there is a checkpoint in the arena */
   SGE_STRING(JR_pe_task_id_str, CULL_DEFAULT)
   /* string describing task from sight of PE
    * if this is non null this is a PE task */
   SGE_STRING(JR_osjobid, CULL_DEFAULT)     /* string containing osjobid for ckpt jobs */
   SGE_ULONG(JR_wait_status, CULL_DEFAULT)  /* japi_wait() 'status' information  */
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
   NAME("JR_wait_status")
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
   SGE_ULONG(LIC_processors, CULL_DEFAULT)
   SGE_STRING(LIC_arch, CULL_DEFAULT)
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
   SGE_STRING(LR_name, CULL_DEFAULT)
   SGE_STRING(LR_value, CULL_DEFAULT)
   SGE_ULONG(LR_global, CULL_DEFAULT)       /* ==1 global load value */
   SGE_ULONG(LR_static, CULL_DEFAULT)       /* ==1 static load value */
   SGE_HOST(LR_host, CULL_HASH)        /* sender host of load value */  /* CR - hostname change */
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
