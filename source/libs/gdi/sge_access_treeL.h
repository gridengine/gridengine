#ifndef __SGE_ACCESS_TREEL_H
#define __SGE_ACCESS_TREEL_H

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
#ifndef __SGE_SORTL_H
#define __SGE_SORTL_H

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* 
 * access tree object priority group 
 */

enum {
   PGR_priority = PGR_LOWERBOUND,
   PGR_subordinated_list,
   PGR_sort_me,
   PGR_current
};

LISTDEF(PGR_Type)
   SGE_ULONG(PGR_priority)    /* priority of the priority group */
   SGE_LIST(PGR_subordinated_list)
      /* FCFS: jobreference list sorted by arrival time */
      /* USERSORT: list of jobreference lists grouped by users */
   SGE_ULONG(PGR_sort_me)
      /* FCFS: subordinated jobreference list needs to be resorted */
      /* USERSORT: subordinated user list needs to be resorted */
   SGE_REF(PGR_current)
      /* FCFS: reference to the current entry in the jobreferences list */
      /* USERSORT: unused */
LISTEND 

NAMEDEF(PGRN)
   NAME("PGR_priority")
   NAME("PGR_subordinated_list")
   NAME("PGR_sort_me")
   NAME("PGR_current")
NAMEEND


#define PGRS sizeof(PGRN)/sizeof(char*)

/* 
 * access tree object user reference list 
 */
enum {
   USR_name = USR_LOWERBOUND,
   USR_nrunning_el,
   USR_nrunning_dl,
   USR_job_references,
   USR_sort_me,
   USR_current
};

LISTDEF(USR_Type)
   SGE_STRING(USR_name)       /* user name */
   SGE_ULONG(USR_nrunning_el) /* # of running jobs in event layer */
   SGE_ULONG(USR_nrunning_dl) /* # of running jobs in dispatch layer */
   SGE_LIST(USR_job_references)       /* sublist of jobsreferences for that 
                                       * user */
   SGE_ULONG(USR_sort_me)     /* job reference list must be resorted */
   SGE_REF(USR_current)       /* reference to the current entry in the *
                               * jobreferences list */
LISTEND 

NAMEDEF(USRN)
   NAME("USR_name")
   NAME("USR_nrunning_el")
   NAME("USR_nrunning_dl")
   NAME("USR_job_references")
   NAME("USR_sort_me")
   NAME("USR_current")
NAMEEND

#define USRS sizeof(USRN)/sizeof(char*)

/* 
 * access tree object jobreference list 
 */
enum {
   JRL_jobid = JRL_LOWERBOUND,
   JRL_submission_time,
   JRL_category
};

LISTDEF(JRL_Type)
   SGE_ULONG(JRL_jobid)       /* jobid used for hasing */
   SGE_ULONG(JRL_submission_time)     /* submission time of this job array */
   SGE_REF(JRL_category)      /* category reference */
LISTEND 

NAMEDEF(JRLN)
   NAME("JRL_jobid")
   NAME("JRL_submission_time")
   NAME("JRL_category")
NAMEEND

/* *INDENT-ON* */

#define JRLS sizeof(JRLN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_SORTL_H */
#endif                          /* __SGE_ACCESS_TREEL_H */
