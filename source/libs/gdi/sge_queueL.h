#ifndef __SGE_QUEUEL_H
#define __SGE_QUEUEL_H

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
 * Q types values for QU_qtype 
 */
enum {
   BQ = 0x01,                /* batch Q */
   IQ = 0x02,                /* interactive Q */
   CQ = 0x04,                /* checkpointing Q */
   PQ = 0x08,                /* parallel Q */
   TQ = 0x10                 /* transfer Q */
};

/* Q states moved over from def.h */
#define QALARM                               0x00000001
#define QSUSPEND_ALARM                       0x00000002
#define QDISABLED                            0x00000004
#define QENABLED                             0x00000008
#define QRUNNING                             0x00000080
#define QSUSPENDED                           0x00000100
#define QUNKNOWN                             0x00000400
#define QERROR                               0x00004000
#define QSUSPENDED_ON_SUBORDINATE            0x00008000
#define QCLEAN                               0x00010000
#define QCAL_DISABLED                        0x00020000
#define QCAL_SUSPENDED                       0x00040000
#define QRESCHEDULED                         0x00080000 /* No queue state */

/*
 * SO_Type - _sub_ordinated
 *
 * This list is used as a sublist of QU_Type
 * and is part of the queue configuration.
 *
 * A change of the configuration in this 
 * list has to trigger a new test
 * of suspend_on_subordinate in the 
 * subordinated queues.
 */
enum {
   SO_qname = SO_LOWERBOUND, /* name of the subordinated queue */
   SO_threshold              /* threshold for suspend_on_subordinate 
                              * here you find the number of used slots 
                              * in the queue that hosts the SO-List 
                              * that must be full for triggering a 
                              * suspend_on_subordinate 
                              * or 0 which means 'full' */
};

SLISTDEF(SO_Type, SubordinateQueue)
   SGE_STRING(SO_qname, CULL_DEFAULT) /* no hashing, we will not have too many subordinated queues for one queue */
   SGE_ULONG(SO_threshold, CULL_DEFAULT)
LISTEND 

NAMEDEF(SON)
   NAME("SO_qname")
   NAME("SO_threshold")
NAMEEND

#define SOS sizeof(SON)/sizeof(char*)

/****** gdi/queue/--QU_Type ***************************************************
*  NAME
*     QU_Type - CULL queue element
*
*  ELEMENTS
*     SGE_STRING(QU_qname) 
*        Unique queue name
*
*     SGE_HOST(QU_qhostname)
*        Hostname
* 
*     SGE_STRING(QU_tmpdir)  
*        Temporary working directory
*     
*     SGE_STRING(QU_shell)
*
*     SGE_LONG(QU_seq_no)   
*        Sequence # for use by qmon 
*
*     SGE_ULONG(QU_queue_number)               
*        Unique internal # of the queue 
*
*     SGE_LIST(QU_load_thresholds)
*        CE_Type; list of load alarm values 
*
*     SGE_LIST(QU_suspend_thresholds) 
*        CE_Type; list of load alarm val. for job susp 
*
*     SGE_ULONG(QU_nsuspend) 
*        Number of jobs to suspend per time interval
*
*     SGE_STRING(QU_suspend_interval)    
*        suspend interval 
*
*     SGE_STRING(QU_priority)     
*        job priority 
*
*     SGE_BOOL(QU_rerun)       
*        restart a job
* 
*     SGE_ULONG(QU_qtype)        
*        BATCH, INTERACTIVE, BI, ... 
* 
*     SGE_STRING(QU_processors)  
*        string describing ranges of processor nodes 
*
*     SGE_ULONG(QU_job_slots)    
*        number of job slots
* 
*     SGE_STRING(QU_calendar)   
*        name of the calendar or NULL
*
*     SGE_STRING(QU_prolog)      
*        overrides prolog in local conf
*
*     SGE_STRING(QU_epilog)      
*        overrides epilog in local conf
*
*     SGE_STRING(QU_shell_start_mode)    
*        overrides shell_start_mode in local conf 
*
*     SGE_STRING(QU_initial_state)       
*        enabled, disabled or default
*
*     SGE_STRING(QU_s_rt)       
*        soft real time 
*
*     SGE_STRING(QU_h_rt)       
*        hard real time
* 
*     SGE_STRING(QU_s_cpu)      
*        soft cpu 
*
*     SGE_STRING(QU_h_cpu)      
*        hard cpu 
*
*     SGE_STRING(QU_s_fsize)    
*        soft file size
*
*     SGE_STRING(QU_h_fsize)    
*        hard file size
*
*     SGE_STRING(QU_s_data)     
*        soft data size 
*
*     SGE_STRING(QU_h_data)     
*        hard data size 
*
*     SGE_STRING(QU_s_stack)    
*        soft stack size 
*
*     SGE_STRING(QU_h_stack)    
*        hard stack_size 
*
*     SGE_STRING(QU_s_core)     
*        soft core fsize 
*
*     SGE_STRING(QU_h_core)     
*        hard core fsize
*
*     SGE_STRING(QU_s_rss)      
*        soft ressident set size
*
*     SGE_STRING(QU_h_rss)      
*        hard ressident set size
*
*     SGE_STRING(QU_s_vmem)     
*        soft virtual memory size 
*
*     SGE_STRING(QU_h_vmem)     
*        hard virtual memory size 
*
*     SGE_ULONG(QU_stamp)       
*        for the scheduler 
*
*     SGE_STRING(QU_min_cpu_interval)    
*        min time between two ckpt cores
*
*     SGE_ULONG(QU_enable_migr)         
*        flag controlled via qrestart
*
*     SGE_ULONG(QU_master)
*
*     SGE_ULONG(QU_state)
*
*     SGE_STRING(QU_notify)              
*        seconds to notify job before SIGKILL/SIGSTOP
*     
*     SGE_LIST(QU_acl)       
*        US_Type - valid user linked list
*
*     SGE_LIST(QU_xacl)        
*        US_Type - excluded user list
*
*     SGE_LIST(QU_owner_list, US_Type)  
*        US_Type - list of "owners"
*
*     SGE_LIST(QU_subordinate_list) 
*        SO_Type - string containing list of subordinate Qs
*
*     SGE_LIST(QU_complex_list)     
*        user defined queue complexes CX_Type
*
*     SGE_LIST(QU_consumable_config_list) 
*        CE_Type - consumable resources of queue
*
*     SGE_LIST(QU_projects)         
*        SGEEE - UP_Type - list of projects which have access to 
*        queue - list contains name only 
*
*     SGE_LIST(QU_xprojects)    
*        SGEEE - UP_Type - list of projects  which have no 
*        access to queue - list contains name only
*
*     SGE_ULONG(QU_fshare)     
*        SGEEE - functional share 
*
*     SGE_ULONG(QU_oticket)    
*        SGEEE - override tickets the following are ephemeral in 
*        nature - don't pack them 
*
*     SGE_LIST(QU_consumable_actual_list) 
*        CE_Type actually debited amout of consumable resources of 
*        queue 
*
*     SGE_ULONG(QU_suitable)
*
*     SGE_ULONG(QU_tagged)
*
*     SGE_ULONG(QU_tagged4schedule)
*  
*     SGE_LIST(QU_cached_complexes)      
*        CE_Type used in scheduler for caching 
*
*     SGE_ULONG(QU_cache_version)       
*        used to decide whether QU_cached_complexes needs a refresh
*
*     SGE_ULONG(QU_pending_signal)
*
*     SGE_ULONG(QU_pending_signal_delivery_time)
*
*     SGE_ULONG(QU_version)             
*        used to control configuration version of queue 
*
*     SGE_STRING(QU_queueing_system)
*
*     SGE_ULONG(QU_suspended_on_subordinate) 
*        number of sos's from other queues for caching only in 
*        the qmaster
*
*     SGE_ULONG(QU_last_suspend_threshold_ckeck) 
*        time when schedd has checked queues suspend threshold - 
*        only used in schedd 
*
*     SGE_ULONG(QU_job_cnt)          
*        SGEEE - job reference count only used in schedd 
*
*     SGE_ULONG(QU_pending_job_cnt)  
*        SGEEE - pending job reference count only used in schedd 
*
*     SGE_ULONG(QU_soft_violation)   
*        number of soft request (-l/-q) violations
*
*     SGE_ULONG(QU_host_seq_no)      
*        sequence number of host only used in schedd 
*
*     SGE_STRING(QU_starter_method)  
*        method how to start a job 
*
*     SGE_STRING(QU_suspend_method)  
*        method how to suspend a job 
*
*     SGE_STRING(QU_resume_method)   
*        method how to resume a stopped job
*
*     SGE_STRING(QU_terminate_method)
*        method how to terminate a job 
******************************************************************************/
enum {
   QU_qname = QU_LOWERBOUND,
   QU_qhostname,
   QU_tmpdir,
   QU_shell,

   QU_seq_no,
   QU_queue_number,
   QU_load_thresholds,
   QU_suspend_thresholds,
   QU_nsuspend,
   QU_suspend_interval,
   QU_priority,
   QU_rerun,
   QU_qtype,
   QU_processors,
   QU_job_slots,

   QU_calendar,

   QU_prolog,
   QU_epilog,
   QU_shell_start_mode,
   QU_initial_state,

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

   QU_stamp,

   QU_min_cpu_interval,

   QU_enable_migr,

   QU_master,
   QU_state,
   QU_notify,

   QU_acl,
   QU_xacl,
   QU_owner_list,
   QU_subordinate_list,
   QU_complex_list,
   QU_consumable_config_list,
   QU_projects,
   QU_xprojects,

   QU_fshare,
   QU_oticket,

   QU_consumable_actual_list,
   QU_suitable,
   QU_tagged,
   QU_tagged4schedule,
   QU_cached_complexes,
   QU_cache_version,
   QU_pending_signal,
   QU_pending_signal_delivery_time,
   QU_version,
   QU_queueing_system,
   QU_suspended_on_subordinate,
   QU_last_suspend_threshold_ckeck,
   QU_job_cnt,
   QU_pending_job_cnt,
   QU_soft_violation,
   QU_host_seq_no,

   QU_starter_method,
   QU_suspend_method,
   QU_resume_method,
   QU_terminate_method
};

ILISTDEF(QU_Type, Queue, SGE_QUEUE_LIST)
   SGE_STRING(QU_qname, CULL_HASH | CULL_UNIQUE)      
   SGE_HOST(QU_qhostname, CULL_HASH)
   SGE_STRING(QU_tmpdir, CULL_DEFAULT) 
   SGE_STRING(QU_shell, CULL_DEFAULT)

   SGE_ULONG(QU_seq_no, CULL_DEFAULT) 
   SGE_ULONG(QU_queue_number, CULL_HASH | CULL_UNIQUE)
   SGE_LIST(QU_load_thresholds, CE_Type, CULL_DEFAULT) 
   SGE_LIST(QU_suspend_thresholds, CE_Type, CULL_DEFAULT)
   SGE_ULONG(QU_nsuspend, CULL_DEFAULT)
   SGE_STRING(QU_suspend_interval, CULL_DEFAULT)
   SGE_STRING(QU_priority, CULL_DEFAULT)
   SGE_BOOL(QU_rerun, CULL_DEFAULT)  
   SGE_ULONG(QU_qtype, CULL_DEFAULT)  
   SGE_STRING(QU_processors, CULL_DEFAULT)
   SGE_ULONG(QU_job_slots, CULL_DEFAULT)
   SGE_STRING(QU_calendar, CULL_DEFAULT)
   SGE_STRING(QU_prolog, CULL_DEFAULT)  
   SGE_STRING(QU_epilog, CULL_DEFAULT) 
   SGE_STRING(QU_shell_start_mode, CULL_DEFAULT)
                                 
   SGE_STRING(QU_initial_state, CULL_DEFAULT) 
   SGE_STRING(QU_s_rt, CULL_DEFAULT)       
   SGE_STRING(QU_h_rt, CULL_DEFAULT)      
   SGE_STRING(QU_s_cpu, CULL_DEFAULT)    
   SGE_STRING(QU_h_cpu, CULL_DEFAULT)   
   SGE_STRING(QU_s_fsize, CULL_DEFAULT)
   SGE_STRING(QU_h_fsize, CULL_DEFAULT)
   SGE_STRING(QU_s_data, CULL_DEFAULT) 
   SGE_STRING(QU_h_data, CULL_DEFAULT) 
   SGE_STRING(QU_s_stack, CULL_DEFAULT)
   SGE_STRING(QU_h_stack, CULL_DEFAULT)
   SGE_STRING(QU_s_core, CULL_DEFAULT)
   SGE_STRING(QU_h_core, CULL_DEFAULT) 
   SGE_STRING(QU_s_rss, CULL_DEFAULT) 
   SGE_STRING(QU_h_rss, CULL_DEFAULT)  
   SGE_STRING(QU_s_vmem, CULL_DEFAULT) 
   SGE_STRING(QU_h_vmem, CULL_DEFAULT)
   SGE_ULONG(QU_stamp, CULL_DEFAULT) 
   SGE_STRING(QU_min_cpu_interval, CULL_DEFAULT) 
   SGE_ULONG(QU_enable_migr, CULL_DEFAULT)     
   SGE_ULONG(QU_master, CULL_DEFAULT)
   SGE_ULONG(QU_state, CULL_DEFAULT)
   SGE_STRING(QU_notify, CULL_DEFAULT)         
   SGE_LIST(QU_acl, US_Type, CULL_DEFAULT)   
   SGE_LIST(QU_xacl, US_Type, CULL_DEFAULT)
   SGE_LIST(QU_owner_list, US_Type, CULL_DEFAULT)
   SGE_LIST(QU_subordinate_list, SO_Type, CULL_DEFAULT)
                                         
   SGE_LIST(QU_complex_list, CX_Type, CULL_DEFAULT)  
   SGE_LIST(QU_consumable_config_list, CE_Type, CULL_DEFAULT) 
   SGE_LIST(QU_projects, UP_Type, CULL_DEFAULT)       
   SGE_LIST(QU_xprojects, UP_Type, CULL_DEFAULT)    
   SGE_ULONG(QU_fshare, CULL_DEFAULT)    
   SGE_ULONG(QU_oticket, CULL_DEFAULT)  
   SGE_LIST(QU_consumable_actual_list, CE_Type, CULL_DEFAULT) 
   SGE_ULONG(QU_suitable, CULL_DEFAULT)
   SGE_ULONG(QU_tagged, CULL_DEFAULT)
   SGE_ULONG(QU_tagged4schedule, CULL_DEFAULT)
   SGE_LIST(QU_cached_complexes, CE_Type, CULL_DEFAULT)   
   SGE_ULONG(QU_cache_version, CULL_DEFAULT)   
   SGE_ULONG(QU_pending_signal, CULL_DEFAULT)
   SGE_ULONG(QU_pending_signal_delivery_time, CULL_DEFAULT)
   SGE_ULONG(QU_version, CULL_DEFAULT)   
   SGE_STRING(QU_queueing_system, CULL_DEFAULT)
   SGE_ULONG(QU_suspended_on_subordinate, CULL_DEFAULT) 
   SGE_ULONG(QU_last_suspend_threshold_ckeck, CULL_DEFAULT) 
   SGE_ULONG(QU_job_cnt, CULL_DEFAULT)          
   SGE_ULONG(QU_pending_job_cnt, CULL_DEFAULT)
   SGE_ULONG(QU_soft_violation, CULL_DEFAULT)   
   SGE_ULONG(QU_host_seq_no, CULL_DEFAULT)    
                                
   SGE_STRING(QU_starter_method, CULL_DEFAULT)  
   SGE_STRING(QU_suspend_method, CULL_DEFAULT) 
   SGE_STRING(QU_resume_method, CULL_DEFAULT) 
   SGE_STRING(QU_terminate_method, CULL_DEFAULT)
LISTEND 

NAMEDEF(QUN)
   NAME("QU_qname")
   NAME("QU_qhostname")
   NAME("QU_tmpdir")
   NAME("QU_shell")

   NAME("QU_seq_no")
   NAME("QU_queue_number")
   NAME("QU_load_thresholds")
   NAME("QU_suspend_thresholds")
   NAME("QU_nsuspend")
   NAME("QU_suspend_interval")
   NAME("QU_priority")
   NAME("QU_rerun")
   NAME("QU_qtype")
   NAME("QU_processors")
   NAME("QU_job_slots")

   NAME("QU_calendar")

   NAME("QU_prolog")
   NAME("QU_epilog")
   NAME("QU_shell_start_mode")
   NAME("QU_initial_state")

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

   NAME("QU_stamp")
   NAME("QU_min_cpu_interval")

   NAME("QU_enable_migr")

   NAME("QU_master")
   NAME("QU_state")
   NAME("QU_notify")

   NAME("QU_acl")
   NAME("QU_xacl")
   NAME("QU_owner_list")
   NAME("QU_subordinate_list")
   NAME("QU_complex_list")
   NAME("QU_consumable_config_list")
   NAME("QU_projects")
   NAME("QU_xprojects")
   NAME("QU_fshare")
   NAME("QU_oticket")

   NAME("QU_consumable_actual_list")
   NAME("QU_suitable")
   NAME("QU_tagged")
   NAME("QU_tagged4schedule")
   NAME("QU_cached_complexes")
   NAME("QU_cache_version")
   NAME("QU_pending_signal")
   NAME("QU_pending_signal_delivery_time")
   NAME("QU_version")
   NAME("QU_queueing_system")
   NAME("QU_suspended_on_subordinate")
   NAME("QU_last_suspend_threshold_ckeck")
   NAME("QU_job_cnt")
   NAME("QU_pending_job_cnt")
   NAME("QU_soft_violation")
   NAME("QU_host_seq_no")

   NAME("QU_starter_method")
   NAME("QU_suspend_method")
   NAME("QU_resume_method")
   NAME("QU_terminate_method")
NAMEEND

/* *INDENT-ON* */ 

#define QUS sizeof(QUN)/sizeof(char*)

/* *INDENT-OFF* */ 

enum {
   QR_name = QR_LOWERBOUND
};

LISTDEF(QR_Type)
   SGE_STRING(QR_name, CULL_HASH | CULL_UNIQUE)
LISTEND 

NAMEDEF(QRN)
   NAME("QR_name")
NAMEEND

/* *INDENT-ON* */  

#define QRS sizeof(QRN)/sizeof(char*)



/* *INDENT-ON* */  
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_QUEUEL_H */
