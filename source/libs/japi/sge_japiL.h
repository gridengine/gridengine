#ifndef __SGE_JAPIL_H
#define __SGE_JAPIL_H

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

/****** japi/--JJ_Type **************************************************
*  NAME
*     JJ_Type - JAPI job
*
*  ELEMENTS
*     SGE_ULONG(JJ_jobid)
*        JAPI jobid
*
*     SGE_LIST(JJ_finished_tasks)
*        list of finished job tasks
*
*     SGE_LIST(JJ_not_yet_finished_ids)
*        id's of not yet finished tasks
*
*  FUNCTION
*     CULL element holding per job information about JAPI session 
******************************************************************************/
enum {
   JJ_jobid = JJ_LOWERBOUND,
   JJ_finished_tasks,
   JJ_not_yet_finished_ids
};

LISTDEF(JJ_Type)
   SGE_ULONG(JJ_jobid, CULL_DEFAULT)
   SGE_LIST(JJ_finished_tasks, JJAT_Type, CULL_DEFAULT)
   SGE_LIST(JJ_not_yet_finished_ids, RN_Type, CULL_DEFAULT)
LISTEND 

NAMEDEF(JJN)
   NAME("JJ_jobid")
   NAME("JJ_finished_tasks")
   NAME("JJ_not_yet_finished_ids")
NAMEEND

/* *INDENT-ON* */

#define JJS sizeof(JJN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif

/****** japi/--JJAT_Type **************************************************
*  NAME
*     JJAT_Type - JAPI array task
*
*  ELEMENTS
*     SGE_ULONG(JJAT_task_id)
*        array task id
*
*     SGE_ULONG(JJAT_stat)
*        stat information provided by japi_wait()
*
*     SGE_LIST(JJAT_rusage)
*        resource usage information provided by japi_wait()
*
*     SGE_STRING(JJAT_failed_text)
*        printable error text describing reason of job error
*
*  FUNCTION
*     CULL element holding per job information about JAPI session 
******************************************************************************/
enum {
   JJAT_task_id = JJAT_LOWERBOUND,
   JJAT_stat,
   JJAT_rusage,
   JJAT_failed_text
};

LISTDEF(JJAT_Type)
   SGE_ULONG(JJAT_task_id, CULL_DEFAULT)
   SGE_ULONG(JJAT_stat, CULL_DEFAULT)
   SGE_LIST(JJAT_rusage, UA_Type, CULL_DEFAULT)
   SGE_STRING(JJAT_failed_text, CULL_DEFAULT)
LISTEND 

NAMEDEF(JJATN)
   NAME("JJAT_task_id")
   NAME("JJAT_stat")
   NAME("JJAT_rusage")
   NAME("JJAT_failed_text")
NAMEEND

/* *INDENT-ON* */

#define JJATS sizeof(JJATN)/sizeof(char*)

/****** japi/--JJAT_Type **************************************************
*  NAME
*     NSV_Type - Named string vector
*
*  ELEMENTS
*     SGE_ULONG(JJAT_task_id)
*        array task id
*
*     SGE_LIST(JJAT_rusage)
*        resource usage information provided by japi_wait()
*
*  FUNCTION
*     CULL element implementing JAPI vector job template attributes 
******************************************************************************/
enum {
   NSV_name = NSV_LOWERBOUND,
   NSV_strings
};

LISTDEF(NSV_Type)
   SGE_STRING(NSV_name, CULL_DEFAULT)
   SGE_LIST(NSV_strings, STR_Type, CULL_DEFAULT)
LISTEND 

NAMEDEF(NSVN)
   NAME("NSV_name")
   NAME("NSV_strings")
NAMEEND

/* *INDENT-ON* */

#define NSVS sizeof(NSVN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif

#endif  /* __SGE_JAPIL_H */

