
#ifndef __SGE_PETASKL_H
#define __SGE_PETASKL_H
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

/****** gdi/job_jatask/PET_Type ************************************************
*  NAME
*     PET_Type - CULL job element 
*
*  ELEMENTS
*     Job identification and dependencies
*     ===================================
*
*
*     JG: TODO: what about all the sge_o_* variables? If they were moved to the
*               environment, we would have no problems.
*     JG: TODO: do we need something similar to JB_path_aliases here?
*
*  FUNCTION
*     PET_Type elements make only sense in conjunction with JAT_Type
*     elements.  One element of each type is necessary to hold all
*     data for the execution of one job. One PET_Type element and
*     x JAT_Type elements are needed to execute an array job with
*     x tasks.
*
*              -----------       1:x        ------------
*              | JB_Type |<---------------->| JAT_Type |
*              -----------                  ------------
*
*     The relation between these two elements is defined in the
*     'JB_ja_tasks' sublist of a 'JB_Type' element. This list will
*     contain all belonging JAT_Type elements.
*
*     The 'JAT_Type' CULL element containes all attributes in which
*     one array task may differ from another array task of the
*     same array job. The 'JB_Type' element defines all attributes
*     wich are equivalent for all tasks of an array job.
*     A job and an array job with one task are equivalent
*     concerning their data structures. Both consist of one 'JB_Type'
*     and one 'JAT_Type' element.
*
*  SEE ALSO
*     gdi/job_jatask/JAT_Type                             
******************************************************************************/
enum {
   PET_id = PET_LOWERBOUND,
   PET_name,

   PET_status,

   PET_granted_destin_identifier_list, /* JG: TODO: maybe only the qname is enough */

   PET_pid,
   PET_osjobid,
   PET_usage,
   PET_scaled_usage,
   PET_previous_usage,

   PET_submission_time,
   PET_start_time,
   PET_end_time,

   PET_cwd,
   PET_environment,
   PET_args, /* JG: TODO: really used? */

   PET_source
};

ILISTDEF(PET_Type, Job, SGE_JOB_LIST)
   SGE_KSTRINGHU(PET_id)
   SGE_STRING(PET_name)
   
   SGE_ULONG(PET_status)

   SGE_LIST(PET_granted_destin_identifier_list)

   SGE_ULONG(PET_pid)
   SGE_STRING(PET_osjobid)
   SGE_LIST(PET_usage)
   SGE_LIST(PET_scaled_usage)
   SGE_LIST(PET_previous_usage)

   SGE_ULONG(PET_submission_time)
   SGE_ULONG(PET_start_time)
   SGE_ULONG(PET_end_time)

   SGE_STRING(PET_cwd)
   SGE_LIST(PET_environment)
   SGE_LIST(PET_args)

   SGE_STRING(PET_source)
LISTEND

NAMEDEF(PETN)
   NAME("PET_id")
   NAME("PET_name")
   
   NAME("PET_status")
   
   NAME("PET_granted_destin_identifier_list")

   NAME("PET_pid")
   NAME("PET_osjobid")
   NAME("PET_usage")
   NAME("PET_scaled_usage")
   NAME("PET_previous_usage")
   
   NAME("PET_submission_time")
   NAME("PET_start_time")
   NAME("PET_end_time")
   
   NAME("PET_cwd")
   NAME("PET_environment")
   NAME("PET_args")

   NAME("PET_source")
NAMEEND


#define PETS sizeof(PETN)/sizeof(char*)

enum {
   PETR_jobid = PETR_LOWERBOUND,
   PETR_jataskid,
   PETR_queuename,
   PETR_owner,
   PETR_cwd,
   PETR_environment,
   PETR_submission_time
};

ILISTDEF(PETR_Type, Job, SGE_JOB_LIST)
   SGE_ULONG(PETR_jobid)
   SGE_ULONG(PETR_jataskid)
   SGE_STRING(PETR_queuename)
   SGE_STRING(PETR_owner)
   SGE_STRING(PETR_cwd)
   SGE_LIST(PETR_environment)
   SGE_ULONG(PETR_submission_time)
LISTEND


NAMEDEF(PETRN)
   NAME("PETR_jobid")
   NAME("PETR_jataskid")
   NAME("PETR_queuename")
   NAME("PETR_owner")
   NAME("PETR_cwd")
   NAME("PETR_environment")
   NAME("PETR_submission_time")
NAMEEND

#define PETRS sizeof(PETRN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_PETASKL_H */

