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

/* 
 * Q types values for QU_qtype 
 */
enum {
   BQ = 0x01,                /* batch Q */
   IQ = 0x02                 /* interactive Q */
};

enum {
   GDI_DO_LATER = 0x01
};

#if 0
/* *INDENT-OFF* */

/*
 * Qinstance explanation element
 */

enum {
   QIM_type = QIM_LOWERBOUND,
   QIM_message
};

LISTDEF(QIM_Type)
   SGE_ULONG(QIM_type, CULL_DEFAULT)
   SGE_STRING(QIM_message, CULL_DEFAULT)
LISTEND

NAMEDEF(QIMN)
   NAME("QIM_type")
   NAME("QIM_message")
NAMEEND

#define QIMS sizeof(QIMN)/sizeof(char*)

/* *INDENT-ON* */
#endif 

/* *INDENT-OFF* */

enum {
   QU_qhostname = QU_LOWERBOUND,
   QU_qname,
   QU_full_name,
   QU_tag,
   QU_available_at, /* scheduling code only */

   QU_seq_no,
   QU_nsuspend,
   QU_qtype,
   QU_job_slots,

   QU_tmpdir,
   QU_shell,
   QU_calendar,
   QU_priority,
   QU_processors,
   QU_prolog,
   QU_epilog,
   QU_shell_start_mode,
   QU_starter_method,
   QU_suspend_method,
   QU_resume_method,
   QU_terminate_method,
   QU_initial_state,

   QU_rerun,

   QU_s_rt,
   QU_h_rt,
   QU_s_cpu,
   QU_h_cpu,
   QU_s_fsize,
   QU_h_fsize,
   QU_s_data,
   QU_h_data,
   QU_s_stack,
   QU_h_stack,
   QU_s_core,
   QU_h_core,
   QU_s_rss,
   QU_h_rss,
   QU_s_vmem,
   QU_h_vmem,

   QU_suspend_interval,
   QU_min_cpu_interval,
   QU_notify,
   
   QU_ckpt_list,
   QU_pe_list,

   QU_owner_list,
   QU_acl,
   QU_xacl,

   QU_projects,
   QU_xprojects,

   QU_consumable_config_list,
   QU_load_thresholds,
   QU_suspend_thresholds,

   QU_subordinate_list,

/* EB: TODO: Add internal attributes */

   QU_queue_number,
   QU_state,
   QU_tagged4schedule,
   QU_cache_version,
   QU_pending_signal,
   QU_pending_signal_delivery_time,
   QU_version,
   QU_suspended_on_subordinate,
   QU_last_suspend_threshold_ckeck,
   QU_job_cnt,
   QU_pending_job_cnt,
   QU_soft_violation,
   QU_host_seq_no,
   QU_resource_utilization,
   QU_message_list,
   QU_gdi_do_later
};

SLISTDEF(QU_Type, QInstance)
   SGE_HOST(QU_qhostname, CULL_SPOOL) 
   SGE_STRING(QU_qname, CULL_SPOOL)
   SGE_STRING(QU_full_name, CULL_PRIMARY_KEY)
   SGE_ULONG(QU_tag, CULL_DEFAULT)
   SGE_ULONG(QU_available_at, CULL_DEFAULT)

   SGE_ULONG(QU_seq_no, CULL_CONFIGURE)
   SGE_ULONG(QU_nsuspend, CULL_CONFIGURE)
   SGE_ULONG(QU_qtype, CULL_CONFIGURE)
   SGE_ULONG(QU_job_slots, CULL_CONFIGURE)

   SGE_STRING(QU_tmpdir, CULL_CONFIGURE)
   SGE_STRING(QU_shell, CULL_CONFIGURE)
   SGE_STRING(QU_calendar, CULL_CONFIGURE)
   SGE_STRING(QU_priority, CULL_CONFIGURE)
   SGE_STRING(QU_processors, CULL_CONFIGURE)
   SGE_STRING(QU_prolog, CULL_CONFIGURE)
   SGE_STRING(QU_epilog, CULL_CONFIGURE)
   SGE_STRING(QU_shell_start_mode, CULL_CONFIGURE)
   SGE_STRING(QU_starter_method, CULL_CONFIGURE)
   SGE_STRING(QU_suspend_method, CULL_CONFIGURE)
   SGE_STRING(QU_resume_method, CULL_CONFIGURE)
   SGE_STRING(QU_terminate_method, CULL_CONFIGURE)
   SGE_STRING(QU_initial_state, CULL_CONFIGURE)

   SGE_BOOL(QU_rerun, CULL_CONFIGURE)

   SGE_STRING(QU_s_rt, CULL_CONFIGURE)
   SGE_STRING(QU_h_rt, CULL_CONFIGURE)
   SGE_STRING(QU_s_cpu, CULL_CONFIGURE)
   SGE_STRING(QU_h_cpu, CULL_CONFIGURE)
   SGE_STRING(QU_s_fsize, CULL_CONFIGURE)
   SGE_STRING(QU_h_fsize, CULL_CONFIGURE)
   SGE_STRING(QU_s_data, CULL_CONFIGURE)
   SGE_STRING(QU_h_data, CULL_CONFIGURE)
   SGE_STRING(QU_s_stack, CULL_CONFIGURE)
   SGE_STRING(QU_h_stack, CULL_CONFIGURE)
   SGE_STRING(QU_s_core, CULL_CONFIGURE)
   SGE_STRING(QU_h_core, CULL_CONFIGURE)
   SGE_STRING(QU_s_rss, CULL_CONFIGURE)
   SGE_STRING(QU_h_rss, CULL_CONFIGURE)
   SGE_STRING(QU_s_vmem, CULL_CONFIGURE)
   SGE_STRING(QU_h_vmem, CULL_CONFIGURE)

   SGE_STRING(QU_suspend_interval, CULL_CONFIGURE)
   SGE_STRING(QU_min_cpu_interval, CULL_CONFIGURE)
   SGE_STRING(QU_notify, CULL_CONFIGURE)

   SGE_LIST(QU_ckpt_list, ST_Type, CULL_CONFIGURE)
   SGE_LIST(QU_pe_list, ST_Type, CULL_CONFIGURE)

   SGE_LIST(QU_owner_list, US_Type, CULL_CONFIGURE)
   SGE_LIST(QU_acl, US_Type, CULL_CONFIGURE)
   SGE_LIST(QU_xacl, US_Type, CULL_CONFIGURE)

   SGE_LIST(QU_projects, UP_Type, CULL_CONFIGURE)
   SGE_LIST(QU_xprojects, UP_Type, CULL_CONFIGURE)

   SGE_LIST(QU_consumable_config_list, CE_Type, CULL_CONFIGURE)
   SGE_LIST(QU_load_thresholds, CE_Type, CULL_CONFIGURE)
   SGE_LIST(QU_suspend_thresholds, CE_Type, CULL_CONFIGURE)

   SGE_LIST(QU_subordinate_list, SO_Type, CULL_CONFIGURE)

/* EB: TODO: Add internal attributes */

   SGE_ULONG(QU_queue_number, CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
   SGE_ULONG(QU_state, CULL_SPOOL)
   SGE_ULONG(QU_tagged4schedule, CULL_DEFAULT)
   SGE_ULONG(QU_cache_version, CULL_DEFAULT)
   SGE_ULONG(QU_pending_signal, CULL_SPOOL)
   SGE_ULONG(QU_pending_signal_delivery_time, CULL_SPOOL)
   SGE_ULONG(QU_version, CULL_SPOOL)
   SGE_ULONG(QU_suspended_on_subordinate, CULL_DEFAULT)
   SGE_ULONG(QU_last_suspend_threshold_ckeck, CULL_DEFAULT)
   SGE_ULONG(QU_job_cnt, CULL_DEFAULT)
   SGE_ULONG(QU_pending_job_cnt, CULL_DEFAULT)
   SGE_ULONG(QU_soft_violation, CULL_DEFAULT)
   SGE_ULONG(QU_host_seq_no, CULL_DEFAULT)
   SGE_LIST(QU_resource_utilization, RUE_Type, CULL_DEFAULT)
   SGE_LIST(QU_message_list, QIM_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_ULONG(QU_gdi_do_later, CULL_DEFAULT)
LISTEND 

NAMEDEF(QUN)
   NAME("QU_qhostname")
   NAME("QU_qname")
   NAME("QU_full_name")
   NAME("QU_tag")
   NAME("QU_available_at")

   NAME("QU_seq_no")
   NAME("QU_nsuspend")
   NAME("QU_qtype")
   NAME("QU_job_slots")

   NAME("QU_tmpdir")
   NAME("QU_shell")
   NAME("QU_calendar")
   NAME("QU_priority")
   NAME("QU_processors")
   NAME("QU_prolog")
   NAME("QU_epilog")
   NAME("QU_shell_start_mode")
   NAME("QU_starter_method")
   NAME("QU_suspend_method")
   NAME("QU_resume_method")
   NAME("QU_terminate_method")
   NAME("QU_initial_state")

   NAME("QU_rerun")

   NAME("QU_s_rt")
   NAME("QU_h_rt")
   NAME("QU_s_cpu")
   NAME("QU_h_cpu")
   NAME("QU_s_fsize")
   NAME("QU_h_fsize")
   NAME("QU_s_data")
   NAME("QU_h_data")
   NAME("QU_s_stack")
   NAME("QU_h_stack")
   NAME("QU_s_core")
   NAME("QU_h_core")
   NAME("QU_s_rss")
   NAME("QU_h_rss")
   NAME("QU_s_vmem")
   NAME("QU_h_vmem")

   NAME("QU_suspend_interval")
   NAME("QU_min_cpu_interval")
   NAME("QU_notify")

   NAME("QU_ckpt_list")
   NAME("QU_pe_list")

   NAME("QU_owner_list")
   NAME("QU_acl")
   NAME("QU_xacl")

   NAME("QU_projects")
   NAME("QU_xprojects")

   NAME("QU_consumable_config_list") 
   NAME("QU_load_thresholds") 
   NAME("QU_suspend_thresholds") 

   NAME("QU_subordinate_list") 

/* EB: TODO: Add internal attributes */

   NAME("QU_queue_number")
   NAME("QU_state")
   NAME("QU_tagged4schedule")
   NAME("QU_cache_version")
   NAME("QU_pending_signal")
   NAME("QU_pending_signal_delivery_time")
   NAME("QU_version")
   NAME("QU_suspended_on_subordinate")
   NAME("QU_last_suspend_threshold_ckeck")
   NAME("QU_job_cnt")
   NAME("QU_pending_job_cnt")
   NAME("QU_soft_violation")
   NAME("QU_host_seq_no")
   NAME("QU_resource_utilization")
   NAME("QU_message_list")
   NAME("QU_gdi_do_later")
NAMEEND

#define QUS sizeof(QUN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_QINSTANCEL_H */
