#ifndef __SGE_CTL_H
#define __SGE_CTL_H

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
 * this data structures describes the category list 
 */
enum {
   CT_str = CT_LOWERBOUND,   /* string of category */
   CT_refcount,              /* number of jobs referencing the string */
   CT_rejected,              /* has this category been rejected */
   CT_ignore_queues,         /* stores all queues, which cannot run this job class */ 
   CT_ignore_hosts,          /* stores all hosts, which cannot run this job class */
   CT_queue_violations,      /* stores in a case of soft requests, for each queue the number of violations */
   CT_job_messages,          /* stores the error messages, which a job got during its dispatching */ 
   CT_resource_contribution, /* SGEEE: resource request dependent contribution on urgency 
                                       this value is common with all jobs of a category */
   CT_rc_valid               /* SGEEE: indicates whether cached CT_resource_contribution is valid */
};

ILISTDEF(CT_Type, Categories, SGE_CT_LIST)
   SGE_STRING(CT_str, CULL_HASH | CULL_UNIQUE)
   SGE_ULONG(CT_refcount, CULL_DEFAULT)
   SGE_ULONG(CT_rejected, CULL_DEFAULT)
   SGE_LIST(CT_ignore_queues, CTI_Type, CULL_DEFAULT)
   SGE_LIST(CT_ignore_hosts, CTI_Type, CULL_DEFAULT)
   SGE_LIST(CT_queue_violations, CTQV_Type, CULL_DEFAULT)
   SGE_LIST(CT_job_messages, MES_Type, CULL_DEFAULT)
   SGE_DOUBLE(CT_resource_contribution, CULL_DEFAULT)
   SGE_BOOL(CT_rc_valid, CULL_DEFAULT)
LISTEND 

NAMEDEF(CTN)
   NAME("CT_str")
   NAME("CT_refcount")
   NAME("CT_rejected")
   NAME("CT_ignore_queues")
   NAME("CT_ignore_hosts")
   NAME("CT_queue_violations")
   NAME("CT_job_messages")
   NAME("CT_resource_contribution")
   NAME("CT_rc_valid")
NAMEEND


/**
 * the following data structures describe the ignore_* lists
 */
enum {
   CTI_name = CTI_LOWERBOUND
};

LISTDEF(CTI_Type)
   SGE_STRING(CTI_name, CULL_HASH | CULL_UNIQUE )
LISTEND

NAMEDEF(CTIN)
   NAME("CTI_name")
NAMEEND

#define CTIS sizeof(CTIN)/sizeof(char*)

/**
 * the following data structures describe the violation cache list for queues 
 */
enum {
   CTQV_name = CTQV_LOWERBOUND,
   CTQV_count
};

LISTDEF(CTQV_Type)
   SGE_STRING(CTQV_name, CULL_HASH | CULL_UNIQUE)
   SGE_ULONG(CTQV_count, CULL_DEFAULT)
LISTEND

NAMEDEF(CTQVN)
   NAME("CTQV_name")
   NAME("CTQV_count")
NAMEEND

#define CTQVS sizeof(CTQVN)/sizeof(char*)


/* *INDENT-ON* */

#define CTS sizeof(CTN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_CTL_H */
