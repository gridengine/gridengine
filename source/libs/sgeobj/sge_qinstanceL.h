#ifndef __SGE_QINSTANCEL_H
#define __SGE_QINSTANCEL_H

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
   SO_name = SO_LOWERBOUND, 
   SO_threshold    
};

SLISTDEF(SO_Type, SubordinateQueue)
   SGE_STRING(SO_name, CULL_PRIMARY_KEY | CULL_DEFAULT | CULL_SUBLIST) 
   SGE_ULONG(SO_threshold, CULL_DEFAULT)
LISTEND

NAMEDEF(SON)
   NAME("SO_name")
   NAME("SO_threshold")
NAMEEND

#define SOS sizeof(SON)/sizeof(char*)


enum {
   QI_hostname = QI_LOWERBOUND,
   QI_name,
   QI_tag,

   QI_seq_no,
   QI_nsuspend,
   QI_qtype,
   QI_job_slots,
   QI_fshare,
   QI_oticket,

   QI_tmpdir,
   QI_shell,
   QI_calendar,
   QI_priority,
   QI_processors,
   QI_prolog,
   QI_epilog,
   QI_shell_start_mode,
   QI_starter_method,
   QI_suspend_method,
   QI_resume_method,
   QI_terminate_method,
   QI_initial_state,

   QI_rerun,

   QI_s_rt,
   QI_h_rt,
   QI_s_cpu,
   QI_h_cpu,
   QI_s_fsize,
   QI_h_fsize,
   QI_s_data,
   QI_h_data,
   QI_s_stack,
   QI_h_stack,
   QI_s_core,
   QI_h_core,
   QI_s_rss,
   QI_h_rss,
   QI_s_vmem,
   QI_h_vmem,

   QI_suspend_interval,
   QI_min_cpu_interval,
   QI_notify,
   
   QI_ckpt_list,
   QI_pe_list,

   QI_owner_list,
   QI_acl,
   QI_xacl,

   QI_projects,
   QI_xprojects,

   QI_consumable_config_list,
   QI_load_thresholds,
   QI_suspend_thresholds,

   QI_subordinate_list,

/* EB: TODO: Add internal attributes */

   QI_queue_number,
   QI_state,
   QI_tagged,
   QI_tagged4schedule,
   QI_cache_version,
   QI_pending_signal,
   QI_pending_signal_delivery_time,
   QI_version,
   QI_suspended_on_subordinate,
   QI_last_suspend_threshold_ckeck,
   QI_job_cnt,
   QI_pending_job_cnt,
   QI_soft_violation,
   QI_host_seq_no,
};

SLISTDEF(QI_Type, QInstance)
   SGE_HOST(QI_hostname, CULL_DEFAULT) 
   SGE_STRING(QI_name, CULL_DEFAULT | CULL_PRIMARY_KEY)
   SGE_ULONG(QI_tag, CULL_DEFAULT)

   SGE_ULONG(QI_seq_no, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(QI_nsuspend, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(QI_qtype, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(QI_job_slots, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(QI_fshare, CULL_SPOOL | CULL_CONFIGURE)
   SGE_ULONG(QI_oticket, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(QI_tmpdir, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_shell, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_calendar, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_priority, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_processors, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_prolog, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_epilog, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_shell_start_mode, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_starter_method, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_suspend_method, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_resume_method, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_terminate_method, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_initial_state, CULL_SPOOL | CULL_CONFIGURE)

   SGE_BOOL(QI_rerun, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(QI_s_rt, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_rt, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_cpu, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_cpu, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_fsize, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_fsize, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_data, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_data, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_stack, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_stack, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_core, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_core, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_rss, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_rss, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_s_vmem, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_h_vmem, CULL_SPOOL | CULL_CONFIGURE)

   SGE_STRING(QI_suspend_interval, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_min_cpu_interval, CULL_SPOOL | CULL_CONFIGURE)
   SGE_STRING(QI_notify, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(QI_ckpt_list, ST_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_pe_list, ST_Type, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(QI_owner_list, US_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_acl, US_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_xacl, US_Type, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(QI_projects, UP_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_xprojects, UP_Type, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(QI_consumable_config_list, CE_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_load_thresholds, CE_Type, CULL_SPOOL | CULL_CONFIGURE)
   SGE_LIST(QI_suspend_thresholds, CE_Type, CULL_SPOOL | CULL_CONFIGURE)

   SGE_LIST(QI_subordinate_list, SO_Type, CULL_SPOOL | CULL_CONFIGURE)

/* EB: TODO: Add internal attributes */

   SGE_ULONG(QI_queue_number, CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
   SGE_ULONG(QI_state, CULL_SPOOL)
   SGE_ULONG(QI_tagged, CULL_DEFAULT)
   SGE_ULONG(QI_tagged4schedule, CULL_DEFAULT)
   SGE_ULONG(QI_cache_version, CULL_DEFAULT)
   SGE_ULONG(QI_pending_signal, CULL_SPOOL)
   SGE_ULONG(QI_pending_signal_delivery_time, CULL_SPOOL)
   SGE_ULONG(QI_version, CULL_SPOOL)
   SGE_ULONG(QI_suspended_on_subordinate, CULL_DEFAULT)
   SGE_ULONG(QI_last_suspend_threshold_ckeck, CULL_DEFAULT)
   SGE_ULONG(QI_job_cnt, CULL_DEFAULT)
   SGE_ULONG(QI_pending_job_cnt, CULL_DEFAULT)
   SGE_ULONG(QI_soft_violation, CULL_DEFAULT)
   SGE_ULONG(QI_host_seq_no, CULL_DEFAULT)
LISTEND 

NAMEDEF(QIN)
   NAME("QI_hostname")
   NAME("QI_name")
   NAME("QI_tag")

   NAME("QI_seq_no")
   NAME("QI_nsuspend")
   NAME("QI_qtype")
   NAME("QI_job_slots")
   NAME("QI_fshare")
   NAME("QI_oticket")

   NAME("QI_tmpdir")
   NAME("QI_shell")
   NAME("QI_calendar")
   NAME("QI_priority")
   NAME("QI_processors")
   NAME("QI_prolog")
   NAME("QI_epilog")
   NAME("QI_shell_start_mode")
   NAME("QI_starter_method")
   NAME("QI_suspend_method")
   NAME("QI_resume_method")
   NAME("QI_terminate_method")
   NAME("QI_initial_state")

   NAME("QI_rerun")

   NAME("QI_s_rt")
   NAME("QI_h_rt")
   NAME("QI_s_cpu")
   NAME("QI_h_cpu")
   NAME("QI_s_fsize")
   NAME("QI_h_fsize")
   NAME("QI_s_data")
   NAME("QI_h_data")
   NAME("QI_s_stack")
   NAME("QI_h_stack")
   NAME("QI_s_core")
   NAME("QI_h_core")
   NAME("QI_s_rss")
   NAME("QI_h_rss")
   NAME("QI_s_vmem")
   NAME("QI_h_vmem")

   NAME("QI_suspend_interval")
   NAME("QI_min_cpu_interval")
   NAME("QI_notify")

   NAME("QI_ckpt_list")
   NAME("QI_pe_list")

   NAME("QI_owner_list")
   NAME("QI_acl")
   NAME("QI_xacl")

   NAME("QI_projects")
   NAME("QI_xprojects")

   NAME("QI_consumable_config_list") 
   NAME("QI_load_thresholds") 
   NAME("QI_suspend_thresholds") 

   NAME("QI_subordinate_list") 

/* EB: TODO: Add internal attributes */

   NAME("QI_queue_number")
   NAME("QI_state")
   NAME("QI_tagged")
   NAME("QI_tagged4schedule")
   NAME("QI_cache_version")
   NAME("QI_pending_signal")
   NAME("QI_pending_signal_delivery_time")
   NAME("QI_version")
   NAME("QI_suspended_on_subordinate")
   NAME("QI_last_suspend_threshold_ckeck")
   NAME("QI_job_cnt")
   NAME("QI_pending_job_cnt")
   NAME("QI_soft_violation")
   NAME("QI_host_seq_no")
NAMEEND

#define QIS sizeof(QIN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_QINSTANCEL_H */
