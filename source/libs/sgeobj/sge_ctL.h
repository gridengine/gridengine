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

/* min number of jobs in a category to use
   the skip host, queue and the soft violations */
#define MIN_JOBS_IN_CATEGORY 1


/* 
 * this data structures describes the category list 
 */
enum {
   CT_str = CT_LOWERBOUND,   /* string of category */
   CT_refcount,              /* number of jobs referencing the string */
   CT_count,                 /* number of jobs used in this schuling run, if -1, than CT_refcount is used */ 
   CT_rejected,              /* has this category been rejected as it can not be dispached now */
   CT_cache,                 /* stores all info, which cannot run this job category */ 
   CT_messages_added,        /* if true, the scheduler info messages have been added for this category */
   CT_resource_contribution, /* resource request dependent contribution on urgency 
                                this value is common with all jobs of a category */
   CT_rc_valid               /* indicates whether cached CT_resource_contribution is valid */
};

ILISTDEF(CT_Type, Categories, SGE_CT_LIST)
   SGE_STRING(CT_str, CULL_HASH | CULL_UNIQUE)
   SGE_ULONG(CT_refcount, CULL_DEFAULT)
   SGE_INT(CT_count, CULL_DEFAULT)
   SGE_ULONG(CT_rejected, CULL_DEFAULT)
   SGE_LIST(CT_cache, CCT_Type, CULL_DEFAULT)
   SGE_BOOL(CT_messages_added, CULL_DEFAULT)
   SGE_DOUBLE(CT_resource_contribution, CULL_DEFAULT)
   SGE_BOOL(CT_rc_valid, CULL_DEFAULT)
LISTEND 

NAMEDEF(CTN)
   NAME("CT_str")
   NAME("CT_refcount")
   NAME("CT_count")
   NAME("CT_rejected")
   NAME("CT_cache")
   NAME("CT_messages_added")
   NAME("CT_resource_contribution")
   NAME("CT_rc_valid")
NAMEEND

#define CTS sizeof(CTN)/sizeof(char*)

/**
 * 
 * The caching of dispatch results has to be done per PE. For jobs, which
 * do not request a PE, the pe_name is set to "NONE".
 *
 */
enum {
   CCT_pe_name = CCT_LOWERBOUND,   /* pe name */
   CCT_ignore_queues,         /* stores all queues, which now cannot run this job category */ 
   CCT_ignore_hosts,          /* stores all hosts, which now cannot run this job category */
   CCT_queue_violations,      /* stores in a case of soft requests, for each queue the number of violations */
   CCT_job_messages           /* stores the error messages, which a job got during its dispatching */ 
};

ILISTDEF(CCT_Type, Categories, SGE_CT_LIST)
   SGE_STRING(CCT_pe_name, CULL_HASH | CULL_UNIQUE)
   SGE_LIST(CCT_ignore_queues, CTI_Type, CULL_DEFAULT)
   SGE_LIST(CCT_ignore_hosts, CTI_Type, CULL_DEFAULT)
   SGE_LIST(CCT_queue_violations, CTQV_Type, CULL_DEFAULT)
   SGE_LIST(CCT_job_messages, MES_Type, CULL_DEFAULT)
LISTEND 

NAMEDEF(CCTN)
   NAME("CCT_pe_name")
   NAME("CCT_ignore_queues")
   NAME("CCT_ignore_hosts")
   NAME("CCT_queue_violations")
   NAME("CCT_job_messages")
NAMEEND

#define CCTS sizeof(CCTN)/sizeof(char*)

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

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_CTL_H */
