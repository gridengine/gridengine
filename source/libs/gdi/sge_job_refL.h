#ifndef __SGE_JOB_REFL_H
#define __SGE_JOB_REFL_H

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

enum {
   JRE_job_number = JRE_LOWERBOUND,  /* used in JB_jid_successor_list */
   JRE_job_name                      /* used in JB_jid_predecessor_list */
};

LISTDEF(JRE_Type)
   SGE_ULONG(JRE_job_number)
   SGE_STRING(JRE_job_name)
LISTEND 

NAMEDEF(JREN)
   NAME("JRE_job_number")
   NAME("JRE_job_name")
NAMEEND

/* *INDENT-ON* */

#define JRES sizeof(JREN)/sizeof(char*)

#ifdef ENABLE_438_FIX
/* 
 * We need to store information about finished pe tasks to avoid 
 * duplicate accounting records (see IZ 438).
 * The whole job -> ja_task -> pe_task structure has to be maintained
 * with a minimal set of data.
 */

/* *INDENT-OFF* */

/* Finished Job Reference */

enum {
   FJR_job_number = FJR_LOWERBOUND, 
   FJR_ja_tasks
};

LISTDEF(FJR_Type)
   SGE_ULONGHU(FJR_job_number)
   SGE_LIST(FJR_ja_tasks)
LISTEND

NAMEDEF(FJRN)
   NAME("FJR_job_number")
   NAME("FJR_ja_tasks")
NAMEEND

/* *INDENT-ON* */

#define FJRS sizeof(FJRN)/sizeof(char *)

/* *INDENT-OFF* */

enum {
   FTR_task_number = FTR_LOWERBOUND,
   FTR_pe_tasks
};

LISTDEF(FTR_Type)
   SGE_ULONGHU(FTR_task_number)
   SGE_LIST(FTR_pe_tasks)
LISTEND

NAMEDEF(FTRN)
   NAME("FTR_task_number")
   NAME("FTR_pe_tasks")
NAMEEND

/* *INDENT-ON* */

#define FTRS sizeof(FTRN)/sizeof(char *)

/* *INDENT-OFF* */

enum {
   FPR_id = FPR_LOWERBOUND
};

LISTDEF(FPR_Type)
   SGE_STRINGHU(FPR_id)
LISTEND

NAMEDEF(FPRN)
   NAME("FPR_id")
NAMEEND

/* *INDENT-ON* */

#define FPRS sizeof(FPRN)/sizeof(char *)

#endif /* ENABLE_438_FIX */

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_JOB_REFL_H */
