#ifndef __SGE_JOB_REPORTL_H
#define __SGE_JOB_REPORTL_H

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
 *  License at http://www.gridengine.sunsource.net/license.html
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

enum {
   FAILED_BEFORE_RUN,
   FAILED_WHILE_RUN
};

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
   SGE_STRING(JR_host_name)   /* Host this job (tried to) run on */
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
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_JOB_REPORTL_H */
