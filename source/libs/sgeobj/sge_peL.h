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

/****** sgeobj/pe/--PE_Type ***************************************************
*  NAME
*     PE_Type -- CULL
*
*  ELEMENTS
*     SGE_STRING(PE_name)
*        name of the pe 
*
*     SGE_ULONG(PE_slots)
*        number of total slots
*
*     SGE_LIST(PE_user_list)   
*        US_Type; list of allowed users
*
*     SGE_LIST(PE_xuser_list)  
*        US_Type; list of not allowed users
*
*     SGE_STRING(PE_start_proc_args)
*        cmd line sequence for starting the pe
*
*     SGE_STRING(PE_stop_proc_args)
*        cmd line sequence for stopping the pe
*
*     SGE_STRING(PE_allocation_rule)
*        number of processors per machine
*  
*     SGE_BOOL(PE_control_slaves)
*        whether slave tasks get fed into execd
*
*     SGE_BOOL(PE_job_is_first_task)
*        whether the job script also starts first
*        task like with pvm or job script is just a 
*        starter doing no work like with mpi, dmake 
*        --> has only a meaning when
*        PE_control_slaves is true
*
*     SGE_XULONG(PE_used_slots)
*        internal field; number of used slots
*        this field gets not spooled, updated dynamically
******************************************************************************/
enum {
   PE_name = PE_LOWERBOUND,  
   PE_slots,               
   PE_user_list,          
   PE_xuser_list,         
   PE_start_proc_args,    
   PE_stop_proc_args,     
   PE_allocation_rule,    
   PE_control_slaves,    
   PE_job_is_first_task,
   PE_used_slots            
};


ILISTDEF(PE_Type, ParallelEnvironment, SGE_PE_LIST)
   SGE_STRING(PE_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
   SGE_ULONG(PE_slots, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(PE_user_list, US_Type, CULL_DEFAULT | CULL_SPOOL) 
   SGE_LIST(PE_xuser_list, US_Type, CULL_DEFAULT | CULL_SPOOL) 
   SGE_STRING(PE_start_proc_args, CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING(PE_stop_proc_args, CULL_DEFAULT | CULL_SPOOL)
   SGE_STRING(PE_allocation_rule, CULL_DEFAULT | CULL_SPOOL)
   SGE_BOOL(PE_control_slaves, CULL_DEFAULT | CULL_SPOOL)
   SGE_BOOL(PE_job_is_first_task, CULL_DEFAULT | CULL_SPOOL)
   SGE_ULONG(PE_used_slots, CULL_DEFAULT)
LISTEND 

NAMEDEF(PEN)
   NAME("PE_name")
   NAME("PE_slots")
   NAME("PE_user_list")
   NAME("PE_xuser_list")
   NAME("PE_start_proc_args")
   NAME("PE_stop_proc_args")
   NAME("PE_allocation_rule")
   NAME("PE_control_slaves")
   NAME("PE_job_is_first_task")
   NAME("PE_used_slots")
NAMEEND

/* *INDENT-ON* */ 

#define PES sizeof(PEN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_PEL_H */
