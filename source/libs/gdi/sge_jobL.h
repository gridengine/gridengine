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
#define JTRANSITING                          0x00000200
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
#define JPSEUDORESCHEDULED                   0x00040000

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
#define QLOGIN_PORT "_QLOGIN_PORT"
#define CONTEXT_PARENT "PARENT"

/* *INDENT-OFF* */

enum {
   JB_job_number = JB_LOWERBOUND,
   JB_job_file,
   JB_exec_file,
   JB_script_file,
   JB_script_size,
   JB_script_ptr,
   JB_submission_time,
   JB_end_time,
   JB_owner,
   JB_uid,
   
   JB_group,
   JB_gid,
   JB_sge_o_home,
   JB_sge_o_log_name,
   JB_sge_o_path,
   JB_sge_o_mail,
   JB_sge_o_shell,
   JB_sge_o_tz,
   JB_sge_o_workdir,
   JB_sge_o_host,
  
   JB_execution_time,
   JB_account,
   JB_checkpoint_attr,
   JB_checkpoint_object,
   JB_checkpoint_object_list,
   JB_checkpoint_interval, 
   JB_cell,              
   JB_cwd,                
   JB_directive_prefix, 
   JB_stderr_path_list,
  
   JB_full_listing,    
   JB_merge_stderr, 
   JB_hard_resource_list,
   JB_soft_resource_list,
   JB_mail_options,     
   JB_mail_list,
   JB_notify,        
   JB_now,
   JB_job_name,     
   JB_stdout_path_list,
  
   JB_priority,         
   JB_passwd,          
   JB_passwd_list,    
   JB_hard_queue_list,
   JB_soft_queue_list,
   JB_reauth_time,   
   JB_restart,      
   JB_signal,    
   JB_shell_list,
   JB_user_list, 
   
   JB_unlog,  
   JB_verify,      
   JB_env_list,
   JB_job_args,
   JB_task_id_range,    
   JB_first_host,
   JB_last_host,
   JB_master_hard_queue_list,
   JB_job_identifier_list,
   JB_message,
  
   JB_job_source,
   JB_ext,
   JB_pe,
   JB_pe_range,
   JB_scheduling_priority,
   JB_jid_predecessor_list,
   JB_jid_sucessor_list,
   JB_pvm_pid,
   JB_verify_suitable_queues,
   JB_sig,
  
   JB_notified,
   JB_nrunning,
   JB_reauth_gmt,
   JB_soft_wallclock_gmt,
   JB_hard_wallclock_gmt,
   JB_suspend_enable,
   JB_soc_xsoc,
   JB_force,
   JB_version,
   JB_project,
  
   JB_department,
   JB_jobclass,
   JB_deadline,
   JB_host,
   JB_override_tickets,
   JB_qs_args,
   JB_path_aliases,
   JB_foreign_job_id,
   JB_poll_lastgmt,
   JB_poll_interval,
  
   JB_pe_task_id_str,
   JB_pe_object,
   JB_tagged,
   JB_tgt,
   JB_cred,
   JB_context,
   JB_ja_structure,
   JB_ja_tasks,
   JB_category
};
   
ILISTDEF(JB_Type, Job, SGE_JOB_LIST)
   SGE_KULONG(JB_job_number)
   SGE_XSTRING(JB_job_file)
   SGE_XSTRING(JB_exec_file)
   SGE_STRING(JB_script_file)
   SGE_ULONG(JB_script_size)
   SGE_STRING(JB_script_ptr)
   SGE_RULONG(JB_submission_time)
   SGE_RULONG(JB_end_time)
   SGE_RSTRING(JB_owner)
   SGE_RULONG(JB_uid)
   SGE_RSTRING(JB_group)
   SGE_RULONG(JB_gid)
   SGE_STRING(JB_sge_o_home)
   SGE_STRING(JB_sge_o_log_name)
   SGE_STRING(JB_sge_o_path)
   SGE_STRING(JB_sge_o_mail)
   SGE_STRING(JB_sge_o_shell)
   SGE_STRING(JB_sge_o_tz)
   SGE_STRING(JB_sge_o_workdir)
   SGE_STRING(JB_sge_o_host)
   SGE_ULONG(JB_execution_time)         /* "-a date_time" specifies time    
                                         * availability of job(qalter,qsub) */
   SGE_STRING(JB_account)               /* "-A account_string" specifies   
                                         * account to run job(qalter,qsub)  */
   SGE_ULONG(JB_checkpoint_attr)        /* "-c interval" specifies check-   
                                         * point attributes (qalter,qsub)   */
   SGE_XSTRING(JB_checkpoint_object)    /* "-ckpt name" name of checkpoint 
                                         * object                           */
   SGE_IOBJECT(JB_checkpoint_object, CK_Type) /* - checkpoint obj, qidl only */
   SGE_XOBJECT(JB_checkpoint_object_list, CK_Type)/* - checkpoint obj, only 
                                                   * sent to execd  */
   SGE_ULONG(JB_checkpoint_interval)    /* "-c seconds" specifies check-   
                                         * point frequency(q alter,qsub)    */
   SGE_XSTRING(JB_cell)                 /* "-cell cell_name" specifies     
                                         * cell(ALL COMMANDS)               */
   SGE_STRING(JB_cwd)                   /* "-cwd" use current working       
                                         * directory                        */
   SGE_XSTRING(JB_directive_prefix)     /* "-C directive_prefix" specifies  
                                         * the comment directive prfx(qsub) */
   SGE_TLIST(JB_stderr_path_list, PN_Type) /* PN_Type - "-e path_name" stderr 
                                            * stream path (qsub,qalte r)    */
   SGE_XULONG(JB_full_listing)          /* "-f" specifies full format    
                                         * display(qstat) !!! UNUSED !!!  */
   SGE_BOOL(JB_merge_stderr)            /* "-j y|n" merge stderr into      
                                         * stdout(qalter,qsub)            */ 
   SGE_TLIST(JB_hard_resource_list, RE_Type) /* RE_Type - "-l resource_list" 
                                              * resource limits or restric tions*/ 
   SGE_TLIST(JB_soft_resource_list, RE_Type) /* RE_Type - "-l resource_list" 
                                              * resource limits or restric tions*/ 
   SGE_ULONG(JB_mail_options)           /* "-m mail_options" mail options 
                                         * (qalter,qsub)                   */ 
   SGE_TLIST(JB_mail_list, MR_Type)     /* MR_Type - "-M mail_list" list of
                                         * mail recipiants(qalter ,qsub)   */ 
   SGE_BOOL(JB_notify)                  /* "-notify" notify job of         
                                         * impending SIGKILL/SIG STOP      */ 
   SGE_BOOL(JB_now)                     /* "-now" start job immediately    
                                         * or not at all                   */ 
   SGE_STRING(JB_job_name)              /* "-N job_name" specifies name of 
                                         * job(qalter,qs ub)               */ 
   SGE_TLIST(JB_stdout_path_list, PN_Type) /* PN_Type "-o path_name" specifies
                                            * pathname for stdout(qalter,qsub)*/ 
   SGE_ULONG(JB_priority)               /* "-p priority" Specifies job     
                                         * priority(qalter,q sub)          */ 
   SGE_XULONG(JB_passwd)                /* "-passwd" sets reauth passwd    
                                         * UNUSED */
   SGE_LIST(JB_passwd_list)             /* DES could encrypt first field to
                                         * a NULL - erg o we gotta do it   
                                         * this way  UNUSED                */ 
   SGE_XLIST(JB_hard_queue_list, QR_Type) /* QR_Type - "-q dest_identifier"  
                                           * specifies a queue (qsub)      */
   SGE_ILIST(JB_hard_queue_list, QU_Type)
   
   SGE_XLIST(JB_soft_queue_list, QR_Type) /* QR_Type - "-q dest_identifier" 
                                           * restricts to a qu eue(qselect) */
   SGE_ILIST(JB_soft_queue_list, QU_Type)

   SGE_XULONG(JB_reauth_time)           /* "-reautht #secs" specifies how   */
                                        /* often to reaut h AFS tokens      */
                                        /* UNUSED */
   SGE_BOOL(JB_restart)                 /* "-r y|n" specifies if a job can  */
                                        /* be rerun(qalt er,qsub)           */
   SGE_XULONG(JB_signal)                /* "-s signal" speifies the signal  */
                                        /* to be sent to a job(qsig)        */
                                        /* UNUSED */
   SGE_TLIST(JB_shell_list, PN_Type)    /* PN_Type - "-S shell" specifies   */
                                        /* the interpretting shell          */
                                        /* (qalter,qsub)                    */
   SGE_LIST(JB_user_list)               /* "-u user_list" specifies the     */
                                        /* user name under which a job is   */
                                        /* to run(qalter,qsub)              */
                                        /* UNUSED */
   SGE_XULONG(JB_unlog)                 /* "-ul" unlog AFS tokens at job    */
                                        /* reap time                        */
   SGE_XULONG(JB_verify)                /* "-verify" specifies print out of */
                                        /* options(ALL commands)            */
   SGE_TLIST(JB_env_list, VA_Type)      /* VA_Type - "-V" specififies       */
                                        /* exportation of all environme     */
                                        /* ntal variables(qsub)             */
   SGE_TLIST(JB_job_args, ST_Type)      /* ST_Type - contains job arguments */
                                        /* given to job when it is executed */
   SGE_XULONG(JB_task_id_range)         /* task id range for job at this host */
   SGE_XSTRING(JB_first_host)           /* unused */
   SGE_XSTRING(JB_last_host)            /* unused */
   SGE_LIST(JB_master_hard_queue_list)  /* QR_Type - "-masterq dest_identifier" */
                                        /* specifies a queue (qsub)         */

   SGE_LIST(JB_job_identifier_list)     /* ID_Type                          */
   SGE_XSTRING(JB_message)
   SGE_XSTRING(JB_job_source)           /* name of the component that fed   */
   SGE_XULONG(JB_ext)
   SGE_XSTRING(JB_pe)                   /* see JB_pe_object for qidl obj    */
   SGE_TLIST(JB_pe_range, RN_Type)      /* RN_Type                          */
   SGE_XULONG(JB_scheduling_priority)   /* unused ? */
   SGE_XLIST(JB_jid_predecessor_list,JRE_Type) /* JRE_Type only JRE_job_name */ 
   SGE_ILIST(JB_jid_predecessor_list, JB_Type) /* IDL only                   */ 
   SGE_LIST(JB_jid_sucessor_list)       /* JRE_Type only JRE_job_number      */
   SGE_XULONG(JB_pvm_pid)
   SGE_XULONG(JB_verify_suitable_queues)  
   SGE_XULONG(JB_sig)
   SGE_XULONG(JB_notified)
   SGE_XULONG(JB_nrunning)
   SGE_XULONG(JB_reauth_gmt)
   SGE_XULONG(JB_soft_wallclock_gmt)
   SGE_XULONG(JB_hard_wallclock_gmt)
   SGE_XULONG(JB_suspend_enable)
   SGE_XULONG(JB_soc_xsoc)
   SGE_XULONG(JB_force)
   SGE_XULONG(JB_version)
   
   SGE_XSTRING(JB_project)              /* SGEEE - project name set by qmaster*/
                                        /* at submit time;  spooled         */
   SGE_IOBJECT(JB_project, UP_Type)     /* project, qidl only               */
   SGE_XSTRING(JB_department)           /* SGEEE - department name ???ste???  */
                                        /* set by schedd, saved (once) to   */
                                        /* qmaster                          */
   SGE_IROBJECT(JB_department, US_Type) /* see above, idl only            */
   SGE_XSTRING(JB_jobclass)             /* SGEEE - job class name local to    */
                                        /* schedd, identical to master_queue*/
                                        /* not spooled                      */
   SGE_ULONG(JB_deadline)               /* SGEEE - deadline initiation time   */
                                        /* set by qsub; spooled             */
   SGE_XSTRING(JB_host)                 /* SGEEE - host job is executing on   */
                                        /* local to schedd; not spooled     */
   SGE_ULONG(JB_override_tickets)       /* SGEEE - override tickets assigned  */
                                        /* by admin; spooled                */

   SGE_TLIST(JB_qs_args, ST_Type)       /* ST_Type - contains arguments     */
                                        /* given to a foreign QS            */
   SGE_TLIST(JB_path_aliases, PA_Type)  /* path aliases list               */
   SGE_XSTRING(JB_foreign_job_id)       /* qstd needs this to hold the name */
                                        /* of the foreign queueing system   */
   SGE_XULONG(JB_poll_lastgmt)          /* needed by qstd to trigger job    */
   SGE_XULONG(JB_poll_interval)         /* status polling                   */

   SGE_XSTRING(JB_pe_task_id_str)       /* string identifying pe task -     */
                                        /* e.g. pvm tid                     */
   SGE_OBJECT(JB_pe_object, PE_Type)    /* PE_Type                          */
   SGE_XULONG(JB_tagged)
   SGE_XSTRING(JB_tgt)                  /* Kerberos client TGT              */
   SGE_XSTRING(JB_cred)                 /* DCE / Kerberos credentials       */
   SGE_TLIST(JB_context, VA_Type)       /* custom attributes,(name,val)pairs*/

   SGE_RLIST(JB_ja_structure, RN_Type)  /* Elements describe task id range */ 
                                             
   SGE_RLIST(JB_ja_tasks, JAT_Type)     /* List of JobArray Tasks */
   SGE_REF(JB_category)                 /* category string ref for scheduler */

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
   NAME("JB_job_file")
   NAME("JB_exec_file")
   NAME("JB_script_file")
   NAME("JB_script_size")
   NAME("JB_script_ptr")
   NAME("JB_submission_time")
   NAME("JB_end_time")
   NAME("JB_owner")
   NAME("JB_uid")

   NAME("JB_group")
   NAME("JB_gid")
   NAME("JB_sge_o_home")
   NAME("JB_sge_o_log_name")
   NAME("JB_sge_o_path")
   NAME("JB_sge_o_mail")
   NAME("JB_sge_o_shell")
   NAME("JB_sge_o_tz")
   NAME("JB_sge_o_workdir")
   NAME("JB_sge_o_host")

   NAME("JB_execution_time")
   NAME("JB_account")
   NAME("JB_checkpoint_attr")
   NAME("JB_checkpoint_object")
   NAME("JB_checkpoint_object_list")
   NAME("JB_checkpoint_interval")
   NAME("JB_cell")
   NAME("JB_cwd")
   NAME("JB_directive_prefix")
   NAME("JB_stderr_path_list")
   
   NAME("JB_full_listing")
   NAME("JB_merge_stderr")
   NAME("JB_hard_resource_list")
   NAME("JB_soft_resource_list")
   NAME("JB_mail_options")
   NAME("JB_mail_list")
   NAME("JB_notify")
   NAME("JB_now")
   NAME("JB_job_name")
   NAME("JB_stdout_path_list")
   
   NAME("JB_priority")
   NAME("JB_passwd")
   NAME("JB_passwd_list")
   NAME("JB_hard_queue_list")
   NAME("JB_soft_queue_list")
   NAME("JB_reauth_time")
   NAME("JB_restart")
   NAME("JB_signal")
   NAME("JB_shell_list")
   NAME("JB_user_list")
   
   NAME("JB_unlog")
   NAME("JB_verify")
   NAME("JB_env_list")
   NAME("JB_job_args")
   NAME("JB_task_id_range")
   NAME("JB_first_host")
   NAME("JB_last_host")
   NAME("JB_master_hard_queue_list")
   NAME("JB_job_identifier_list")
   NAME("JB_message")
   
   NAME("JB_job_source")
   NAME("JB_ext")
   NAME("JB_pe")
   NAME("JB_pe_range")
   NAME("JB_scheduling_priority")
   NAME("JB_jid_predecessor_list")
   NAME("JB_jid_sucessor_list")
   NAME("JB_pvm_pid")
   NAME("JB_verify_suitable_queues")
   NAME("JB_sig")
   
   NAME("JB_notified")
   NAME("JB_nrunning")
   NAME("JB_reauth_gmt")
   NAME("JB_soft_wallclock_gmt")
   NAME("JB_hard_wallclock_gmt")
   NAME("JB_suspend_enable")
   NAME("JB_soc_xsoc")
   NAME("JB_force")
   NAME("JB_version")
   NAME("JB_project")
   
   NAME("JB_department")
   NAME("JB_jobclass")
   NAME("JB_deadline")
   NAME("JB_host")
   NAME("JB_override_tickets")
   NAME("JB_qs_args")
   NAME("JB_path_aliases")
   NAME("JB_foreign_job_id")
   NAME("JB_poll_lastgmt")
   NAME("JB_poll_interval")
   
   NAME("JB_pe_task_id_str")
   NAME("JB_pe_object")
   NAME("JB_tagged")
   NAME("JB_tgt")
   NAME("JB_cred")
   NAME("JB_context")
   NAME("JB_ja_structure")
   NAME("JB_ja_tasks")
   NAME("JB_category")
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
   SGE_STRING(PN_host)
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


/* ----------- variable list -------------- */
/*  variable[=value][,variable[=value],...] */
enum {
   VA_variable = VA_LOWERBOUND,
   VA_value
};

SLISTDEF(VA_Type, Variable)
   SGE_STRING(VA_variable)
   SGE_STRING(VA_value)
LISTEND

NAMEDEF(VAN)
   NAME("VA_variable")
   NAME("VA_value")
NAMEEND

#define VAS sizeof(VAN)/sizeof(char*)



/* ------------- mail recipiants ---------- */
/*    user[@host][,user[@host],...]         */
enum {
   MR_user = MR_LOWERBOUND,
   MR_host
};

SLISTDEF(MR_Type, MailRecipient)
   SGE_STRING(MR_user)
   SGE_STRING(MR_host)
LISTEND

NAMEDEF(MRN)
   NAME("MR_user")
   NAME("MR_host")
NAMEEND

#define MRS sizeof(MRN)/sizeof(char*)

/* ---------------------------------------- 

   granted destination identifiers 

*/

#define TASK_ID_RANGE_SIZE 1000

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
   SGE_XSTRING(JG_qhostname)/* redundant qualified host name for caching  */
   SGE_RULONG(JG_slots)     /* from orders list                           */
   SGE_ROBJECT(JG_queue, QU_Type) /* QU_Type - complete queue only in execd */
   SGE_XULONG(JG_tag_slave_job) /* whether slave execds job has arrived in 
                                 * case of pe's with sge controlled slaves */
   SGE_LIST(JG_complex)         /* CX_Type - complex list for this queue 
                                 * used to transfer these values to execd */
   SGE_XULONG(JG_task_id_range) /* first task id to be used by execd when  
                                 * searching for free task id's in case
                                 * of pe's with sge controlled slaves 
                                 * the valid range is from JG_task_id to 
                                 * JG_task_id + TASK_ID_RANGE_SIZE -1 */
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
