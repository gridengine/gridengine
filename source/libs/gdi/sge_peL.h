#ifndef __SGE_PEL_H
#define __SGE_PEL_H

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
 * this data structures describes an parallel environment 
 */
enum {

   /* configuration fields */
   PE_name = PE_LOWERBOUND,  /* name of the pe */
   PE_queue_list,            /* which queues have this pe */
   PE_slots,                 /* number of total slots */
   PE_user_list,             /* list of allowed users */
   PE_xuser_list,            /* list of not allowed users */
   PE_start_proc_args,       /* cmd line sequence for starting the pe */
   PE_stop_proc_args,        /* cmd line sequence for stopping the pe */
   PE_allocation_rule,       /* number of processors per machine */
   PE_control_slaves,        /* whether slave tasks get fed into execd */
   PE_job_is_first_task,     /* whether the job script also starts first
                              * task like with pvm or job script is just a 
                              * starter doing no work like with mpi, dmake 
                              * --> has only a meaning when
                              * PE_control_slaves is true */

   /* internal fields */
   PE_used_slots             /* number of used slots 
                              * - this field gets not spooled 
                              * - updated dynamically */
};


ILISTDEF(PE_Type, ParallelEnvironment, SGE_PE_LIST)
   /* configuration fields */
   SGE_KSTRINGHU(PE_name)
   SGE_XLIST(PE_queue_list, QR_Type)  /* QR_Type, cull only */
   SGE_ILIST(PE_queue_list, QU_Type)
   SGE_ULONG(PE_slots)
   SGE_TLIST(PE_user_list, US_Type)   /* US_Type */
   SGE_TLIST(PE_xuser_list, US_Type)  /* US_Type */
   SGE_STRING(PE_start_proc_args)
   SGE_STRING(PE_stop_proc_args)
   SGE_STRING(PE_allocation_rule)
   SGE_BOOL(PE_control_slaves)
   SGE_BOOL(PE_job_is_first_task)

   /* internal fields */
   SGE_XULONG(PE_used_slots)
LISTEND 

NAMEDEF(PEN)
   /* configuration fields */
   NAME("PE_name")
   NAME("PE_queue_list")
   NAME("PE_slots")
   NAME("PE_user_list")
   NAME("PE_xuser_list")
   NAME("PE_start_proc_args")
   NAME("PE_stop_proc_args")
   NAME("PE_allocation_rule")
   NAME("PE_control_slaves")
   NAME("PE_job_is_first_task")

   /* internal fields  */
   NAME("PE_used_slots")
NAMEEND

#define PES sizeof(PEN)/sizeof(char*)

/* 
 * queue references - a sublist of the pe object 
 */
enum {
   QR_name = QR_LOWERBOUND
};

LISTDEF(QR_Type)
   SGE_STRINGHU(QR_name)
LISTEND 

NAMEDEF(QRN)
   NAME("QR_name")
NAMEEND

/* *INDENT-ON* */  

#define QRS sizeof(QRN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_PEL_H */
