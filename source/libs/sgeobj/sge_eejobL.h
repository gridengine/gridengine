#ifndef __SGE_EEJOBL_H
#define __SGE_EEJOBL_H

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
 * This is the list type we use to sort the joblist 
 * in the sge scheduler 
 */
enum {
   FCAT_job_share = FCAT_LOWERBOUND,
   FCAT_user_share,
   FCAT_project_share,
   FCAT_dept_share,
   FCAT_jobclass_share,
   FCAT_jobrelated_ticket_first,
   FCAT_jobrelated_ticket_last
};

LISTDEF(FCAT_Type)
   SGE_ULONG(FCAT_job_share, CULL_DEFAULT)       /* all jobs in this functional category have this amount of jobs shares */
   SGE_ULONG(FCAT_user_share, CULL_DEFAULT)      /* all jobs in this functional category have this amount of user shares */
   SGE_ULONG(FCAT_project_share, CULL_DEFAULT)   /* all jobs in this functional category have this amount of project shares */
   SGE_ULONG(FCAT_dept_share, CULL_DEFAULT)      /* all jobs in this functional category have this amount of department shares */
   SGE_ULONG(FCAT_jobclass_share, CULL_DEFAULT)  /* all jobs in this functional category have this amount of job class shares */
   SGE_REF(FCAT_jobrelated_ticket_first, CULL_ANY_SUBTYPE, CULL_DEFAULT) /* pointer to the first element of job ticket list*/
   SGE_REF(FCAT_jobrelated_ticket_last, CULL_ANY_SUBTYPE, CULL_DEFAULT)  /* pointer to the last element in the hob ticket list*/
LISTEND 

NAMEDEF(FCATN)
   NAME("FCAT_job_share")
   NAME("FCAT_user_share")
   NAME("FCAT_project_share")
   NAME("FCAT_dept_share")
   NAME("FCAT_jobrelated_ticket_first")
   NAME("FCAT_jobrelated_ticket_last")
NAMEEND

#define FCATS sizeof(FCATN)/sizeof(char*)


/*
 * This is the list type we use to sort the joblist 
 * in the sge scheduler 
 */
enum {
   SGEJ_ticket = SGEJ_LOWERBOUND,
   SGEJ_job_number,
   SGEJ_job_reference
};

LISTDEF(SGEJ_Type)
   SGE_DOUBLE(SGEJ_ticket, CULL_DEFAULT)
   SGE_ULONG(SGEJ_job_number, CULL_DEFAULT)
   SGE_REF(SGEJ_job_reference, JRL_Type, CULL_DEFAULT)
LISTEND 

NAMEDEF(SGEJN)
   NAME("SGEJ_ticket")
   NAME("SGEJ_job_number")
   NAME("SGEJ_job_reference")
NAMEEND

/* *INDENT-ON* */

#define SGEJS sizeof(SGEJN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_EEJOBL_H */
