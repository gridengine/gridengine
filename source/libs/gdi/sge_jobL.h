#ifndef __SGE_JOBL_H
#define __SGE_JOBL_H
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

/* Job states moved in from def.h */
#define JIDLE                                0x00000000
/* #define JENABLED                             0x00000008 */
#define JHELD                                0x00000010
#define JMIGRATING                           0x00000020
#define JQUEUED                              0x00000040
#define JRUNNING                             0x00000080
#define JSUSPENDED                           0x00000100
#define JTRANSFERING                          0x00000200
#define JDELETED                             0x00000400
#define JWAITING                             0x00000800
#define JEXITING                             0x00001000
#define JWRITTEN                             0x00002000
/* used in execd - job waits for getting its ASH/JOBID */
#define JWAITING4OSJID                       0x00004000
/* used in execd - shepherd reports job exit but there are still processes */
#define JERROR                               0x00008000
#define JSUSPENDED_ON_THRESHOLD              0x00010000
/*
   SGE: qmaster delays job removal till schedd 
   does no longer need this finished job 
*/  
#define JFINISHED                            0x00010000
/* used in execd to prevent slave jobs from getting started */
#define JSLAVE                               0x00020000
/* used in execd to prevent simulated jobs from getting started */
#define JSIMULATED                           0x00040000

/* 
   GDI request syntax for JB_hold 

   Example:
 
   qalter -h {u|s|o|n}

  POSIX (overwriting):
  u: SET|USER 
  o: SET|OPERATOR
  s: SET|SYSTEM 
  n: SUB|USER|SYSTEM|OPERATOR
 
  SGE (adding):
  +u: ADD|USER
  +o: ADD|OPERATOR
  +s: ADD|SYSTEM
 
  SGE (removing):
  -u: SUB|USER
  -o: SUB|OPERATOR
  -s: SUB|SYSTEM
   
*/
enum { 
   /* need place for tree bits */
   MINUS_H_CMD_ADD = (0<<3), /* adds targetted flags */
   MINUS_H_CMD_SUB = (1<<3), /* remove targetted flags */
   MINUS_H_CMD_SET = (2<<3)  /* overwrites using targetted flags */
}; 
enum { 
   MINUS_H_TGT_USER     = 1, /* remove needs at least job owner */
   MINUS_H_TGT_OPERATOR = 2, /* remove needs at least operator  */
   MINUS_H_TGT_SYSTEM   = 4, /* remove needs at least manager   */
   MINUS_H_TGT_ALL      = 7 
};

/* values for JB_verify_suitable_queues */
#define OPTION_VERIFY_STR "nwev"
enum { 
   SKIP_VERIFY = 0,     /* -w n no expendable verifications will be done */
   WARINING_VERIFY,     /* -w w qmaster will warn about these jobs - but submit will succeed */ 
   ERROR_VERIFY,        /* -w e qmaster will make expendable verifications to reject 
                            jobs that are not schedulable (default) */ 
   JUST_VERIFY          /* -w v just verify at qmaster but do not submit */
};



/************    scheduling constants   *****************************************/
/* priorities are in the range from -1024 to 1023 */
/* to put them in into u_long we need to add 1024 */
#define BASE_PRIORITY  1024

/* int -> u_long */
#define PRI_ITOU(x) ((x)+BASE_PRIORITY)
/* u_long -> int */
#define PRI_UTOI(x) ((x)-BASE_PRIORITY)

#define PRIORITY_OFFSET 8
#define NEWCOMER_FLAG     0x1000000

/* forced negative sign bit  */
#define MAX_JOBS_EXCEEDED 0x8000000
#define ALREADY_SCANNED   0x4000000
#define PRIORITY_MASK     0xffff00
#define SUBPRIORITY_MASK  0x0000ff
#define JOBS_SCANNED_PER_PASS 10

/* 
   used in qstat:

   JSUSPENDED_ON_SUBORDINATE means that the job is
   suspended because its queue is suspended

*/
#define JSUSPENDED_ON_SUBORDINATE            0x00002000

/* reserved names for JB_context */
#define CONTEXT_IOR "IOR"
#define CONTEXT_PARENT "PARENT"

/****** gdi/job_jatask/--JB_Type **********************************************
*  NAME
*     JB_Type - CULL job element 
*
*  ELEMENTS
*     Job identification and dependencies
*     ===================================
*
*     SGE_ULONG(JB_job_number) ---> JB_id
*        Uniq job number.
*
*     SGE_STRING(JB_job_name) 
*        Job name ("qsub/qalter -N job_name")  
*
*     SGE_XULONG(JB_version)
*
*     SGE_LIST(JB_jid_predecessor_list)
*        Predecessor jobs (JRE_Type only JRE_job_name)
*  
*     SGE_LIST(JB_jid_sucessor_list)  
*        Sucessor jobs (JRE_Type only JRE_job_number)
*
*     Project/Department
*     ==================
*
*     SGE_STRING(JB_project)
*        Project name (qsub -P project_name)
*
*     SGE_STRING(JB_department)
*        Department name. Set by schedd, saved (once) to qmaster.
*
*     Data related to job script
*     ===========================
*
*     SGE_STRING(JB_directive_prefix)     
*        Command prefix for jobscript ("qsub -C prefix") for parsing special comments in the script file.
*
*     SGE_XSTRING(JB_exec_file) ---> what is exec_file, what script_file?
*
*     SGE_STRING(JB_script_file)
*
*     SGE_ULONG(JB_script_size) ---> really needed?
*
*     SGE_STRING(JB_script_ptr) ---> what is it?
*
*     Time information
*     ================
*
*     SGE_RULONG(JB_submission_time)
*
*     SGE_ULONG(JB_execution_time)         
*        When should the job start ("qsub/qalter -a date_time")
*
*     SGE_ULONG(JB_deadline)      
*        SGEEE. Deadline initiation time. (qsub -dl date_time)
*        Not spooled.
*
*     User related information
*     ========================
*
*     SGE_RSTRING(JB_owner) ---> rename to JB_user to be consistent?
*
*     SGE_RULONG(JB_uid)
*
*     SGE_RSTRING(JB_group)
*
*     SGE_RULONG(JB_gid)
*
*     SGE_STRING(JB_account)  
*        Account string ("qsub/qalter -A account string")
*
*     Submission environment
*     ======================
*
*     SGE_STRING(JB_cwd)      
*        Current working directory during qsub ("qsub -cwd")
*
*     SGE_BOOL(JB_notify)                  
*        Notify job of impending kill/stop signal. ("qsub -notify")
*
*     SGE_BOOL(JB_now) 
*        Start job immediately or not at all. ("qsub -now")
*
*     SGE_ULONG(JB_priority) 
*        Priority ("qsub/qalter -p priority")     
*
*     SGE_LIST(JB_shell_list, PN_Type)    
*        Command interpreter to be used (PN_Type).
*        ("qsub/qalter -S shell")
*
*     SGE_ULONG(JB_verify)             
*        Triggers "verify" messages. (qsub -verify)
*
*     SGE_LIST(JB_env_list)      
*        Export these env variables (VA_Type). ("qsub -V").
*
*     SGE_TLIST(JB_context, VA_Type)       
*        Custom attributes (name,val) pairs (VA_Type). 
*        ("qsub/qalter -ac/-dc context_list")
*
*     SGE_LIST(JB_job_args)  
*        Job arguments (ST_Type). 
*
*     Checkpointing/Restart
*     =====================
*     SGE_ULONG(JB_checkpoint_attr)  ----> merge all checkpointing stuff to one object?
*        Checkpoint attributes ("qsub/qalter -c interval_flags")   
*
*     SGE_STRING(JB_checkpoint_object)    
*        Name of ckpt object ("qsub/qalter -ckpt ckpt_name")
*
*     SGE_LIST(JB_checkpoint_object_list, CK_Type)
*        Ckpt object which will be sent from qmaster to execd.
*
*     SGE_ULONG(JB_checkpoint_interval)    
*        Checkpoint frequency ("qsub/qalter -c seconds")
*
*     SGE_BOOL(JB_restart)                 
*        Is job rerunable? ("qsub/qalter -r y/n")
*
*     Job I/O
*     =======
*
*     SGE_LIST(JB_stdout_path_list) 
*        Pathname for stdout (PN_Type). ("qsub/qalter -o path_name")
*
*     SGE_LIST(JB_stderr_path_list)   
*        Std error path streams (PN_Type). ("qsub/qalter "-e path_name")
*
*     SGE_BOOL(JB_merge_stderr)   
*        Merge stdout and stderr? ("qsub/qalter -j y|n")
*
*     Resource requests
*     =================
*
*     SGE_LIST(JB_hard_resource_list, RE_Type) 
*        Hard resource requirements/limits/restrictions (RE_Type).
*        ("qsub -l resource_list")
*
*     SGE_LIST(JB_soft_resource_list) 
*        Soft resource requirements/limits/restrictions (RE_Type).
*        ("qsub -l resource_list")
*
*     SGE_LIST(JB_hard_queue_list) ----> why separated from other requests?
*        Hard queue list (QR_Type). ("qsub -q dest_identifier")
*
*     SGE_LIST(JB_soft_queue_list) ----> why separated from other requests?
*        Soft queue list (QR_Type). ("qsub/qselect -q dest_identifier")
*
*     Mail options
*     ============
*
*     SGE_ULONG(JB_mail_options)           
*        Mail options  ("qsub/qalter -m mail_options")
*
*     SGE_LIST(JB_mail_list)    
*        Mail recipiants (MR_Type). ("qsub/qalter -M mail_list)
*
*     Parallel Job info
*     =================
*
*     SGE_STRING(JB_pe)      
*        Name of a PE
*
*     SGE_LIST(JB_pe_range)
*        PE slot range (RN_Type)    
*
*     SGE_LIST(JB_master_hard_queue_list)  
*        Master queue list (QR_Type). ("qsub -masterq queue_list")
*
*     SGE_STRING(JB_pe_task_id_str)      ----> move to petask object 
*        String identifying pe task (e.g. pvm tid)
*
*     SGE_LIST(JB_pe_object)   
*        PE object (PE_Type).
*
*     Security related data
*     =====================
*
*     SGE_XSTRING(JB_tgt)                  
*        Kerberos client TGT 
*
*     SGE_XSTRING(JB_cred)                 
*        DCE/Kerberos credentials 
*
*     Data related to array jobs
*     ==========================
*
*     SGE_LIST(JB_ja_structure)  
*        Elements describe task id range structure during the
*        submittion time of a (array) job (RN_Type). 
*        ("qsub -t tid_range")
*
*     SGE_LIST(JB_ja_n_h_ids)    
*        Just submitted array task without hold state (RN_Type).
I        ("qsub -t tid_range")
*
*     SGE_LIST(JB_ja_u_h_ids)    
*        Just submitted and user hold applied (RN_Type).
*        ("qsub -h -t tid_range")
*        ("qalter -h u/U jid.tid1-tid2:step")
*
*     SGE_LIST(JB_ja_s_h_ids)    
*        Just submitted and system hold applied (RN_Type).
*        ("qalter -h s/S jid.tid1-tid2:step")
*  
*     SGE_LIST(JB_ja_o_h_ids)    
*        Just submitted and operator hold applied (RN_Type).
*        ("qalter -h o/O jid.tid1-tid2:step")
*
*     SGE_LIST(JB_ja_z_ids)      
*        Zombie task ids (RN_Type).
*
*     SGE_LIST(JB_ja_template)  
*        Template for new tasks. In SGEEE systems the schedd will
*        store initial tickets in this element. (JAT_Type)
*
*     SGE_LIST(JB_ja_tasks)     
*        List of array tasks (in case of array jobs) or one task 
*        (in case of a job) (JAT_Type).
*
*     Data used only in scheduler
*     ===========================
*
*     SGE_XSTRING(JB_jobclass)
*        Job class name. Local to schedd. Identical to master_queue.
*        Not spooled.
*
*     SGE_HOST(JB_host)                    
*        SGEEE - host job is executing on. Local to schedd. 
*        Not spooled.
*
*     SGE_REF(JB_category)
*        Category string reference used in schedd.
*
*     Misc
*     ====
*
*     SGE_LIST(JB_user_list)               
*        List of usernames (qsub/qalter -u username_list). ---> qsub -u does not exist. Not part of a job, but only userd for qalter request as where condition. Could most probably be passed via lCondition.
*
*     SGE_XULONG(JB_next_pe_task_id)         
*        Used locally in execd to store next pe task id for this job on this execd.
*
*     SGE_LIST(JB_job_identifier_list) ---> condition for qalter? Then it should better be passed via condition. 
*        (ID_Type)
*
*     SGE_STRING(JB_job_source)
*        Submitter (host:commproc:id) of a pe task. Only needed in execd.
*
*     SGE_XULONG(JB_verify_suitable_queues)   ---> qalter?
*
*     SGE_XULONG(JB_nrunning)
*
*     SGE_XULONG(JB_soft_wallclock_gmt) ---> the same as complex s_rt?
*
*     SGE_XULONG(JB_hard_wallclock_gmt) ---> the same as complex h_rt?
*
*     SGE_ULONG(JB_override_tickets)       
*        SGEEE - override tickets assigned by admin. 
*        (qalter -ot tickets).
*
*     SGE_LIST(JB_qs_args) ---> qsi? 
*        Arguments for foreign queuing system (ST_Type).
*        Either delete it, or recycle it to be used with starter_method.
*
*     SGE_LIST(JB_path_aliases)  
*        Path aliases list (PA_Type).
*
*
*  FUNCTION
*     JB_Type elements make only sense in conjunction with JAT_Type
*     elements.  One element of each type is necessary to hold all
*     data for the execution of one job. One JB_Type element and
*     x JAT_Type elements are needed to execute an array job with
*     x tasks.
*
*              -----------       1:x        ------------
*              | JB_Type |<---------------->| JAT_Type |
*              -----------                  ------------
*
*     The relation between these two elements is defined in the
*     'JB_ja_tasks' sublist of a 'JB_Type' element. This list will
*     contain all belonging JAT_Type elements.
*
*     The 'JAT_Type' CULL element containes all attributes in which
*     one array task may differ from another array task of the
*     same array job. The 'JB_Type' element defines all attributes
*     wich are equivalent for all tasks of an array job.
*     A job and an array job with one task are equivalent
*     concerning their data structures. Both consist of one 'JB_Type'
*     and one 'JAT_Type' element.
*
*  SEE ALSO
*     gdi/job_jatask/JAT_Type                             
******************************************************************************/
enum {
   JB_job_number = JB_LOWERBOUND,
   JB_job_name,     
   JB_version,
   JB_jid_predecessor_list,
   JB_jid_sucessor_list,

   JB_project,
   JB_department,
   
   JB_directive_prefix, 
   JB_exec_file,
   JB_script_file,
   JB_script_size,
   JB_script_ptr,
   
   JB_submission_time,
   JB_execution_time,
   JB_deadline,

   JB_owner,
   JB_uid,
   JB_group,
   JB_gid,
   JB_account,

   JB_cwd,                
   JB_notify,        
   JB_now,
   JB_priority,         
   JB_shell_list,
   JB_verify,      
   JB_env_list,
   JB_context,
   JB_job_args,
  
   JB_checkpoint_attr,
   JB_checkpoint_object,
   JB_checkpoint_object_list,
   JB_checkpoint_interval, 
   JB_restart,      

   JB_stdout_path_list,
   JB_stderr_path_list,
   JB_merge_stderr, 

   JB_hard_resource_list,
   JB_soft_resource_list,
   JB_hard_queue_list,
   JB_soft_queue_list,

   JB_mail_options,     
   JB_mail_list,

   JB_pe,
   JB_pe_range,
   JB_master_hard_queue_list,
   JB_pe_task_id_str,
   JB_pe_object,
  
   JB_tgt,
   JB_cred,

   JB_ja_structure,
   JB_ja_n_h_ids,
   JB_ja_u_h_ids,
   JB_ja_s_h_ids,
   JB_ja_o_h_ids,
   JB_ja_z_ids,
   JB_ja_template,
   JB_ja_tasks,

   JB_jobclass,
   JB_host,
   JB_category,

   JB_user_list, 
   JB_next_pe_task_id,    
   JB_job_identifier_list,
   JB_job_source,
   JB_verify_suitable_queues,
   JB_nrunning,
   JB_soft_wallclock_gmt,
   JB_hard_wallclock_gmt,
   JB_override_tickets,
   JB_qs_args,
   JB_path_aliases
};

/* 
 * IF YOU CHANGE SOMETHING HERE THEN CHANGE ALSO THE ADOC COMMENT ABOVE 
 */
   
ILISTDEF(JB_Type, Job, SGE_JOB_LIST)
   SGE_KULONGH(JB_job_number)             
   SGE_STRING(JB_job_name)     
   SGE_XULONG(JB_version)
   SGE_XLIST(JB_jid_predecessor_list,JRE_Type) 
   SGE_ILIST(JB_jid_predecessor_list,JRE_Type) 
   SGE_LIST(JB_jid_sucessor_list) 

   SGE_XSTRING(JB_project)             
   SGE_IOBJECT(JB_project, UP_Type)  
   SGE_XSTRING(JB_department)  
   SGE_IROBJECT(JB_department, US_Type) 

   SGE_XSTRING(JB_directive_prefix)     
   SGE_XSTRING(JB_exec_file)
   SGE_STRING(JB_script_file)
   SGE_ULONG(JB_script_size)
   SGE_STRING(JB_script_ptr)

   SGE_RULONG(JB_submission_time)
   SGE_ULONG(JB_execution_time)  
   SGE_ULONG(JB_deadline) 

   SGE_RSTRING(JB_owner)
   SGE_RULONG(JB_uid)
   SGE_RSTRING(JB_group)
   SGE_RULONG(JB_gid)
   SGE_STRING(JB_account)      

   SGE_STRING(JB_cwd)     
   SGE_BOOL(JB_notify)  
   SGE_BOOL(JB_now)     
   SGE_ULONG(JB_priority)       
   SGE_TLIST(JB_shell_list, PN_Type) 
   SGE_XULONG(JB_verify) 
   SGE_TLIST(JB_env_list, VA_Type)  
   SGE_TLIST(JB_context, VA_Type)  
   SGE_TLIST(JB_job_args, ST_Type)  

   SGE_ULONG(JB_checkpoint_attr)     
   SGE_XSTRING(JB_checkpoint_object)   
   SGE_IOBJECT(JB_checkpoint_object, CK_Type) 
   SGE_XOBJECT(JB_checkpoint_object_list, CK_Type)
   SGE_ULONG(JB_checkpoint_interval)   
   SGE_BOOL(JB_restart)  

   SGE_TLIST(JB_stdout_path_list, PN_Type) 
   SGE_TLIST(JB_stderr_path_list, PN_Type) 
   SGE_BOOL(JB_merge_stderr)     

   SGE_TLIST(JB_hard_resource_list, RE_Type) 
   SGE_TLIST(JB_soft_resource_list, RE_Type) 
   SGE_XLIST(JB_hard_queue_list, QR_Type) 
   SGE_ILIST(JB_hard_queue_list, QU_Type)
   SGE_XLIST(JB_soft_queue_list, QR_Type) 
   SGE_ILIST(JB_soft_queue_list, QU_Type)
   
   SGE_ULONG(JB_mail_options) 
   SGE_TLIST(JB_mail_list, MR_Type)  

   SGE_XSTRING(JB_pe)              
   SGE_TLIST(JB_pe_range, RN_Type)     
   SGE_LIST(JB_master_hard_queue_list)  
   SGE_XSTRING(JB_pe_task_id_str) 
   SGE_OBJECT(JB_pe_object, PE_Type)   

   SGE_XSTRING(JB_tgt)      
   SGE_XSTRING(JB_cred)   

   SGE_RLIST(JB_ja_structure, RN_Type)  
   SGE_RLIST(JB_ja_n_h_ids, RN_Type) 
   SGE_RLIST(JB_ja_u_h_ids, RN_Type)   
   SGE_RLIST(JB_ja_s_h_ids, RN_Type)    
   SGE_RLIST(JB_ja_o_h_ids, RN_Type)   
   SGE_RLIST(JB_ja_z_ids, RN_Type)   
   SGE_RLIST(JB_ja_template, JAT_Type)  
   SGE_RLIST(JB_ja_tasks, JAT_Type)  

   SGE_XSTRING(JB_jobclass) 
   SGE_HOST(JB_host)       
   SGE_REF(JB_category)    

   SGE_LIST(JB_user_list)  
   SGE_XULONG(JB_next_pe_task_id) 
   SGE_LIST(JB_job_identifier_list)    
   SGE_XSTRING(JB_job_source)
   SGE_XULONG(JB_verify_suitable_queues)
   SGE_XULONG(JB_nrunning)
   SGE_XULONG(JB_soft_wallclock_gmt)
   SGE_XULONG(JB_hard_wallclock_gmt)
   SGE_ULONG(JB_override_tickets)   
   SGE_TLIST(JB_qs_args, ST_Type)   
   SGE_TLIST(JB_path_aliases, PA_Type)  


   /* 
    * IF YOU ADD SOMETHING HERE THEN CHANGE ALSO THE ADOC COMMENT ABOVE 
    */



   /*IDL
   void submit()
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void submit_array(in sge_ulong min,
                     in sge_ulong max,
                     in sge_ulong step)
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void hold() 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void hold_task(in sge_ulong task_id) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void hold_range(in sge_ulong start,
                   in sge_ulong end,
                   in sge_ulong step) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void release() 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void release_task(in sge_ulong task_id) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void release_range(in sge_ulong start,
                      in sge_ulong end,
                      in sge_ulong step) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void suspend(in boolean force) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void suspend_task(in sge_ulong task_id, in boolean force) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void suspend_range(in sge_ulong start,
                      in sge_ulong end,
                      in sge_ulong step,
                      in boolean force) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void unsuspend(in boolean force)
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void unsuspend_task(in sge_ulong task_id, in boolean force)
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   void unsuspend_range(in sge_ulong start,
                        in sge_ulong end,
                        in sge_ulong step,
                        in boolean force) 
            raises(ObjDestroyed, Authentication, Error) context("sge_auth");
   XIDL*/
LISTEND

NAMEDEF(JBN)
   NAME("JB_job_number")
   NAME("JB_job_name")
   NAME("JB_version")
   NAME("JB_jid_predecessor_list")
   NAME("JB_jid_sucessor_list")

   NAME("JB_project")
   NAME("JB_department")
   
   NAME("JB_directive_prefix")
   NAME("JB_exec_file")
   NAME("JB_script_file")
   NAME("JB_script_size")
   NAME("JB_script_ptr")
   
   NAME("JB_submission_time")
   NAME("JB_execution_time")
   NAME("JB_deadline")
   
   NAME("JB_owner")
   NAME("JB_uid")
   NAME("JB_group")
   NAME("JB_gid")
   NAME("JB_account")

   NAME("JB_cwd")
   NAME("JB_notify")
   NAME("JB_now")
   NAME("JB_priority")
   NAME("JB_shell_list")
   NAME("JB_verify")
   NAME("JB_env_list")
   NAME("JB_context")
   NAME("JB_job_args")

   NAME("JB_checkpoint_attr")
   NAME("JB_checkpoint_object")
   NAME("JB_checkpoint_object_list")
   NAME("JB_checkpoint_interval")
   NAME("JB_restart")

   NAME("JB_stdout_path_list")
   NAME("JB_stderr_path_list")
   NAME("JB_merge_stderr")

   NAME("JB_hard_resource_list")
   NAME("JB_soft_resource_list")
   NAME("JB_hard_queue_list")
   NAME("JB_soft_queue_list")
   
   NAME("JB_mail_options")
   NAME("JB_mail_list")

   NAME("JB_pe")
   NAME("JB_pe_range")
   NAME("JB_master_hard_queue_list")
   NAME("JB_pe_task_id_str")
   NAME("JB_pe_object")
   
   NAME("JB_tgt")
   NAME("JB_cred")

   NAME("JB_ja_structure")
   NAME("JB_ja_n_h_ids")
   NAME("JB_ja_u_h_ids")
   NAME("JB_ja_s_h_ids")
   NAME("JB_ja_o_h_ids")
   NAME("JB_ja_z_ids")
   NAME("JB_ja_template")
   NAME("JB_ja_tasks")

   NAME("JB_jobclass")
   NAME("JB_host")
   NAME("JB_category")
   
   NAME("JB_user_list")
   NAME("JB_next_pe_task_id")
   NAME("JB_job_identifier_list")
   NAME("JB_job_source")
   NAME("JB_verify_suitable_queues")
   NAME("JB_nrunning")
   NAME("JB_soft_wallclock_gmt")
   NAME("JB_hard_wallclock_gmt")
   NAME("JB_override_tickets")
   NAME("JB_qs_args")
   NAME("JB_path_aliases")
NAMEEND


#define JBS sizeof(JBN)/sizeof(char*)

/* -------- path name list ----------------- */
/* [host:]path[,[host:]path...]              */
   
enum {
   PN_path = PN_LOWERBOUND,
   PN_host
};

SLISTDEF(PN_Type, PathName)
   SGE_STRING(PN_path)
   SGE_HOST(PN_host)                    /* CR - hostname change */
LISTEND

NAMEDEF(PNN)
   NAME("PN_path")
   NAME("PN_host")
NAMEEND

#define PNS sizeof(PNN)/sizeof(char*)

/* ---------- account list ----------------- */
/*   account[@cell][,account[@cell]...]      */

enum {
   AT_account = AT_LOWERBOUND,
   AT_cell
};

LISTDEF(AT_Type)
   SGE_STRING(AT_account)
   SGE_STRING(AT_cell )
LISTEND

NAMEDEF(ATN)
   NAME("AT_account")
   NAME("AT_cell")
NAMEEND

#define ATS sizeof(ATN)/sizeof(char*)

/* ------------- mail recipiants ---------- */
/*    user[@host][,user[@host],...]         */
enum {
   MR_user = MR_LOWERBOUND,
   MR_host
};

SLISTDEF(MR_Type, MailRecipient)
   SGE_STRING(MR_user)
   SGE_HOST(MR_host)       /* CR - hostname change */
LISTEND

NAMEDEF(MRN)
   NAME("MR_user")
   NAME("MR_host")
NAMEEND

#define MRS sizeof(MRN)/sizeof(char*)

/* ---------------------------------------- 

   granted destination identifiers 

*/

enum {
   JG_qname = JG_LOWERBOUND,
   JG_qversion,
   JG_qhostname,
   JG_slots,
   JG_queue,
   JG_tag_slave_job,
   JG_complex,
   JG_task_id_range,
   JG_ticket,
   JG_oticket,
   JG_fticket,
   JG_dticket,
   JG_sticket,
   JG_jcoticket,
   JG_jcfticket
};

SLISTDEF( JG_Type, GrantedQueue )
   SGE_RSTRING(JG_qname)    /* the queue's name                           */
   SGE_XULONG(JG_qversion)  /* it's version                               */
   SGE_HOST(JG_qhostname)/* redundant qualified host name for caching  */  /* CR - hostname change */
   SGE_RULONG(JG_slots)     /* from orders list                           */
   SGE_ROBJECT(JG_queue, QU_Type) /* QU_Type - complete queue only in execd */
   SGE_XULONG(JG_tag_slave_job) /* whether slave execds job has arrived in 
                                 * case of pe's with sge controlled slaves */
   SGE_LIST(JG_complex)         /* CX_Type - complex list for this queue 
                                 * used to transfer these values to execd */
   SGE_XULONG(JG_task_id_range) /* unused - please recycle */
   SGE_RDOUBLE(JG_ticket)    /* SGEEE tickets assigned to slots              */
   SGE_RDOUBLE(JG_oticket)   /* SGEEE override tickets assigned to slots     */
   SGE_RDOUBLE(JG_fticket)   /* SGEEE functional tickets assigned to slots   */
   SGE_RDOUBLE(JG_dticket)   /* SGEEE deadline tickets assigned to slots     */
   SGE_RDOUBLE(JG_sticket)   /* SGEEE sharetree tickets assigned to slots    */
   SGE_XDOUBLE(JG_jcoticket) /* SGEEE job class override tickets             */
   SGE_XDOUBLE(JG_jcfticket) /* SGEEE job class functional tickets           */
LISTEND

NAMEDEF( JGN )
   NAME( "JG_qname" )
   NAME( "JG_qversion" )
   NAME( "JG_qhostname" )
   NAME( "JG_slots" )
   NAME( "JG_queue" )
   NAME( "JG_tag_slave_job" )
   NAME( "JG_complex" )
   NAME( "JG_task_id_range" )
   NAME( "JG_ticket" )
   NAME( "JG_oticket" )
   NAME( "JG_fticket" )
   NAME( "JG_dticket" )
   NAME( "JG_sticket" )
   NAME( "JG_jcoticket" )
   NAME( "JG_jcfticket" )
NAMEEND

/* *INDENT-ON* */

#define JGS sizeof(JGN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_JOBL_H */
