#ifndef __SGE_CQUEUEL_H
#define __SGE_CQUEUEL_H

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

enum {
   CQ_name = CQ_LOWERBOUND,
   CQ_hostlist,

   CQ_tmpdir,
   CQ_shell,

   CQ_seq_no,
   CQ_load_thresholds,
   CQ_suspend_thresholds,
   CQ_nsuspend,
   CQ_suspend_interval,

   CQ_priority,
   CQ_rerun,
   CQ_qtype,
   CQ_processors,
   CQ_job_slots,

   CQ_calendar,

   CQ_prolog,
   CQ_epilog,
   CQ_shell_start_mode,
   CQ_initial_state,

   CQ_s_rt,
   CQ_h_rt,
   CQ_s_cpu,
   CQ_h_cpu,
   CQ_s_fsize,
   CQ_h_fsize,
   CQ_s_data,
   CQ_h_data,
   CQ_s_stack,
   CQ_h_stack,
   CQ_s_core,
   CQ_h_core,
   CQ_s_rss,
   CQ_h_rss,
   CQ_s_vmem,
   CQ_h_vmem,

   CQ_min_cpu_interval,

   CQ_notify,

   CQ_acl,
   CQ_xacl,
   CQ_owner_list,
   CQ_subordinate_list,

   CQ_consumable_config_list,
   CQ_projects,
   CQ_xprojects,

   CQ_fshare,
   CQ_oticket,

   CQ_starter_method,
   CQ_suspend_method,
   CQ_resume_method,
   CQ_terminate_method,

   CQ_pe_list,
   CQ_ckpt_list
};

ILISTDEF(CQ_Type, CQueue, SGE_QUEUE_LIST)
   SGE_STRING(CQ_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(CQ_hostlist, HR_Type, CULL_HASH | CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_tmpdir, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_shell, CULL_SPOOL | CULL_CONFIGURE)

   SGE_ULONG(CQ_seq_no, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_LIST(CQ_load_thresholds, CE_Type, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_LIST(CQ_suspend_thresholds, CE_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(CQ_nsuspend, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_suspend_interval, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(CQ_priority, CULL_SPOOL | CULL_CONFIGURE)
   SGE_BOOL(CQ_rerun, CULL_SPOOL | CULL_CONFIGURE)  
   SGE_ULONG(CQ_qtype, CULL_SPOOL | CULL_CONFIGURE)  
   SGE_STRING(CQ_processors, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(CQ_job_slots, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(CQ_calendar, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_prolog, CULL_SPOOL | CULL_CONFIGURE)  
   SGE_STRING(CQ_epilog, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_shell_start_mode, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_initial_state, CULL_SPOOL | CULL_CONFIGURE) 

   SGE_STRING(CQ_s_rt, CULL_SPOOL | CULL_CONFIGURE)       
   SGE_STRING(CQ_h_rt, CULL_SPOOL | CULL_CONFIGURE)      
   SGE_STRING(CQ_s_cpu, CULL_SPOOL | CULL_CONFIGURE)    
   SGE_STRING(CQ_h_cpu, CULL_SPOOL | CULL_CONFIGURE)   
   SGE_STRING(CQ_s_fsize, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_h_fsize, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_s_data, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_h_data, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_s_stack, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_h_stack, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_s_core, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(CQ_h_core, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_s_rss, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_h_rss, CULL_SPOOL | CULL_CONFIGURE)  
   SGE_STRING(CQ_s_vmem, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_h_vmem, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(CQ_min_cpu_interval, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_notify, CULL_SPOOL | CULL_CONFIGURE)         
   SGE_LIST(CQ_acl, US_Type, CULL_SPOOL | CULL_CONFIGURE)   
   SGE_LIST(CQ_xacl, US_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(CQ_owner_list, US_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(CQ_subordinate_list, SO_Type, CULL_SPOOL | CULL_CONFIGURE)
                                         
   SGE_LIST(CQ_consumable_config_list, CE_Type, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_LIST(CQ_projects, UP_Type, CULL_SPOOL | CULL_CONFIGURE)       
   SGE_LIST(CQ_xprojects, UP_Type, CULL_SPOOL | CULL_CONFIGURE)    
   SGE_ULONG(CQ_fshare, CULL_SPOOL | CULL_CONFIGURE)    
   SGE_ULONG(CQ_oticket, CULL_SPOOL | CULL_CONFIGURE)  

   SGE_STRING(CQ_starter_method, CULL_SPOOL | CULL_CONFIGURE)  
   SGE_STRING(CQ_suspend_method, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_resume_method, CULL_SPOOL | CULL_CONFIGURE) 
   SGE_STRING(CQ_terminate_method, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(CQ_pe_list, ST_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(CQ_ckpt_list, ST_Type, CULL_SPOOL | CULL_CONFIGURE)
LISTEND 

NAMEDEF(CQN)
   NAME("CQ_name")
   NAME("CQ_hostlist")
   NAME("CQ_tmpdir")
   NAME("CQ_shell")

   NAME("CQ_seq_no")
   NAME("CQ_load_thresholds")
   NAME("CQ_suspend_thresholds")
   NAME("CQ_nsuspend")
   NAME("CQ_suspend_interval")

   NAME("CQ_priority")
   NAME("CQ_rerun")
   NAME("CQ_qtype")
   NAME("CQ_processors")
   NAME("CQ_job_slots")

   NAME("CQ_calendar")

   NAME("CQ_prolog")
   NAME("CQ_epilog")
   NAME("CQ_shell_start_mode")
   NAME("CQ_initial_state")

   NAME("CQ_s_rt")
   NAME("CQ_h_rt")
   NAME("CQ_s_cpu")
   NAME("CQ_h_cpu")
   NAME("CQ_s_fsize")
   NAME("CQ_h_fsize")
   NAME("CQ_s_data")
   NAME("CQ_h_data")
   NAME("CQ_s_stack")
   NAME("CQ_h_stack")
   NAME("CQ_s_core")
   NAME("CQ_h_core")
   NAME("CQ_s_rss")
   NAME("CQ_h_rss")
   NAME("CQ_s_vmem")
   NAME("CQ_h_vmem")

   NAME("CQ_min_cpu_interval")
   NAME("CQ_notify")

   NAME("CQ_acl")
   NAME("CQ_xacl")
   NAME("CQ_owner_list")
   NAME("CQ_subordinate_list")

   NAME("CQ_consumable_config_list")
   NAME("CQ_projects")
   NAME("CQ_xprojects")
   NAME("CQ_fshare")
   NAME("CQ_oticket")

   NAME("CQ_starter_method")
   NAME("CQ_suspend_method")
   NAME("CQ_resume_method")
   NAME("CQ_terminate_method")

   NAME("CQ_pe_list")
   NAME("CQ_ckpt_list")
NAMEEND

#define CQS sizeof(CQN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_CQUEUEL_H */
