#ifndef __SGE_PTFL_H
#define __SGE_PTFL_H

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

/* *INDENT-OFF* */ 

/*
 * This is the list type we use to hold the jobs which
 * are running under the local execution daemon.
 */

enum {
   JL_job_ID = JL_LOWERBOUND,
   JL_OS_job_list,
   JL_state,
   JL_tickets,
   JL_share,
   JL_ticket_share,
   JL_timeslice,
   JL_usage,
   JL_old_usage_value,
   JL_adjusted_usage,
   JL_last_usage,
   JL_old_usage,
   JL_proportion,
   JL_adjusted_proportion,
   JL_adjusted_current_proportion,
   JL_actual_proportion,
   JL_diff_proportion,
   JL_last_proportion,
   JL_curr_pri,
   JL_pri,
   JL_procfd,
   JL_interactive
};

LISTDEF(JL_Type)
   SGE_ULONG(JL_job_ID)       /* job identifier */
   SGE_LIST(JL_OS_job_list)   /* O.S. job list */
   SGE_ULONG(JL_state)        /* job state (JL_JOB_*) */
   SGE_ULONG(JL_tickets)      /* job tickets */
   SGE_DOUBLE(JL_share)       /* ptf interval share */
   SGE_DOUBLE(JL_ticket_share) /* ptf job ticket share */
   SGE_DOUBLE(JL_timeslice)   /* ptf calculated timeslice (SX) */
   SGE_DOUBLE(JL_usage)       /* ptf interval combined usage */
   SGE_DOUBLE(JL_old_usage_value)     /* ptf interval combined usage */
   SGE_DOUBLE(JL_adjusted_usage)      /* ptf interval adjusted usage */
   SGE_DOUBLE(JL_last_usage)  /* ptf last interval combined usage */
   SGE_DOUBLE(JL_old_usage)   /* prev interval combined usage */
   SGE_DOUBLE(JL_proportion)
   SGE_DOUBLE(JL_adjusted_proportion)
   SGE_DOUBLE(JL_adjusted_current_proportion)
   SGE_DOUBLE(JL_actual_proportion)
   SGE_DOUBLE(JL_diff_proportion)
   SGE_DOUBLE(JL_last_proportion)
   SGE_DOUBLE(JL_curr_pri)
   SGE_LONG(JL_pri)
   SGE_ULONG(JL_procfd)       /* /proc file descriptor */
   SGE_ULONG(JL_interactive)  /* interactive flag */
LISTEND 

NAMEDEF(JLN)
   NAME("JL_job_ID")
   NAME("JL_OS_job_list")
   NAME("JL_state")
   NAME("JL_tickets")
   NAME("JL_share")
   NAME("JL_ticket_share")
   NAME("JL_timeslice")
   NAME("JL_usage")
   NAME("JL_old_usage_value")
   NAME("JL_adjusted_usage")
   NAME("JL_last_usage")
   NAME("JL_old_usage")
   NAME("JL_proportion")
   NAME("JL_adjusted_proportion")
   NAME("JL_adjusted_current_proportion")
   NAME("JL_actual_proportion")
   NAME("JL_diff_proportion")
   NAME("JL_last_proportion")
   NAME("JL_curr_pri")
   NAME("JL_pri")
   NAME("JL_procfd")
   NAME("JL_interactive")
NAMEEND

#define JLS sizeof(JLN)/sizeof(char*)

/*
 * This is the list type we use to hold the list of
 * O.S. jobs being tracked for each PTF job. There
 * will normally only be one O.S. job per PTF job,
 * except in the case of parallel jobs or jobs with
 * multiple tasks.
 */

enum {
   JO_OS_job_ID,
   JO_OS_job_ID2,
   JO_ja_task_ID,
   JO_task_id_str,
   JO_state,
   JO_usage_list,
   JO_pid_list
};

LISTDEF(JO_Type)
   SGE_ULONG(JO_OS_job_ID)    /* O.S. job id (lower 32 bits) */
   SGE_ULONG(JO_OS_job_ID2)   /* O.S. job id (upper 32 bits) */
   SGE_ULONG(JO_ja_task_ID)   /* job array task id */
   SGE_STRING(JO_task_id_str) /* task ID string */
   SGE_ULONG(JO_state)        /* job state (JL_JOB_*) */
   SGE_LIST(JO_usage_list)    /* ptf interval usage */
   SGE_LIST(JO_pid_list)      /* process ID list */
LISTEND 

NAMEDEF(JON)
   NAME("JO_OS_job_ID")
   NAME("JO_OS_job_ID2")
   NAME("JO_ja_task_ID")
   NAME("JO_task_id_str")
   NAME("JO_state")
   NAME("JO_usage_list")
   NAME("JO_pid_list")
NAMEEND

#define JOS sizeof(JON)/sizeof(char*)

/*
 * This is the list type we use to hold the process ID
 * list for jobs in the PTF O.S. job list
 */
enum {
   JP_pid = JP_LOWERBOUND,
   JP_background
};

LISTDEF(JP_Type)
   SGE_ULONGHU(JP_pid)          /* process ID */
   SGE_ULONG(JP_background)   /* background flag */
LISTEND 

NAMEDEF(JPN)
   NAME("JP_pid")
   NAME("JP_background")
NAMEEND

/* *INDENT-ON* */ 

#define JPS sizeof(JPN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_PTFL_H */
