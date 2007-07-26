#ifndef __SGE_H
#define __SGE_H
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

#ifdef WIN32
#   define DEFAULT_EDITOR     "notepad.exe"
#else
#   define DEFAULT_EDITOR     "vi"
#endif

#define MAX_SEQNUM        9999999

/* template/global/default/queue names */
#define SGE_TEMPLATE_NAME        "template"
#define SGE_UNKNOWN_NAME         "unknown"
#define SGE_DEFAULT_NAME         "default"
#define SGE_GLOBAL_NAME          "global"
#define SGE_QUEUE_NAME           "queue"
#define SGE_HOST_NAME            "host"
#define SGE_RQS_NAME             "resource_quota"

/* sge object names */
#define SGE_OBJ_QUEUE                  "queue"
#define SGE_OBJ_CQUEUE                 "queue"
#define SGE_OBJ_HGROUP                 "hostgroup"
#define SGE_OBJ_EXECHOST               "exechost"
#define SGE_OBJ_PE                     "pe"
#define SGE_OBJ_CKPT                   "ckpt"
#define SGE_OBJ_CALENDAR               "calendar"
#define SGE_OBJ_USER_MAPPING           "usermapping"
#define SGE_OBJ_RQS                    "resource_quota"
#define SGE_OBJ_PROJECT                "project"
#define SGE_OBJ_USER                   "user"
#define SGE_OBJ_USERSET                "userset"
#define SGE_OBJ_COMPLEXATTR            "complex_attribute"
#define SGE_OBJ_JOB                    "job"
#define SGE_OBJ_AR                     "advance_reservation"

/* attribute names of sge objects */
#define SGE_ATTR_LOAD_FORMULA          "load_formula"
#define SGE_ATTR_DYNAMICAL_LIMIT       "dynamical_limit"
#define SGE_ATTR_LOAD_SCALING          "load_scaling"
#define SGE_ATTR_PE_LIST               "pe_list"
#define SGE_ATTR_HOST_LIST             "hostlist"
#define SGE_ATTR_CKPT_LIST             "ckpt_list"
#define SGE_ATTR_COMPLEX_VALUES        "complex_values"
#define SGE_ATTR_LOAD_VALUES           "load_values"
#define SGE_ATTR_PROCESSORS            "processors"
#define SGE_ATTR_USER_LISTS            "user_lists"
#define SGE_ATTR_XUSER_LISTS           "xuser_lists"
#define SGE_ATTR_PROJECTS              "projects"
#define SGE_ATTR_RQSRULES              "resource_quota_rules"
#define SGE_ATTR_XPROJECTS             "xprojects"
#define SGE_ATTR_USAGE_SCALING         "usage_scaling"
#define SGE_ATTR_SEQ_NO                "seq_no"
#define SGE_ATTR_LOAD_THRESHOLD        "load_thresholds"
#define SGE_ATTR_SUSPEND_THRESHOLD     "suspend_thresholds"
#define SGE_ATTR_NSUSPEND              "nuspend"
#define SGE_ATTR_SUSPEND_INTERVAL      "suspend_interval"
#define SGE_ATTR_PRIORITY              "priority"
#define SGE_ATTR_MIN_CPU_INTERVAL      "min_cpu_interval"
#define SGE_ATTR_PROCESSORS            "processors"
#define SGE_ATTR_QTYPE                 "qtype"
#define SGE_ATTR_RERUN                 "rerun"
#define SGE_ATTR_SLOTS                 "slots"
#define SGE_ATTR_TMPDIR                "tmpdir"
#define SGE_ATTR_SHELL                 "shell"
#define SGE_ATTR_SHELL_START_MODE      "shell_start_mode"
#define SGE_ATTR_PROLOG                "prolog"
#define SGE_ATTR_EPILOG                "epilog"
#define SGE_ATTR_STARTER_METHOD        "starter_method"
#define SGE_ATTR_SUSPEND_METHOD        "suspend_method"
#define SGE_ATTR_RESUME_METHOD         "resume_method"
#define SGE_ATTR_TERMINATE_METHOD      "terminate_method"
#define SGE_ATTR_NOTIFY                "notify"
#define SGE_ATTR_OWNER_LIST            "owner_list"
#define SGE_ATTR_CALENDAR              "calendar"
#define SGE_ATTR_INITIAL_STATE         "initial_state"
#define SGE_ATTR_FSHARE                "fshare"
#define SGE_ATTR_OTICKET               "oticket"
#define SGE_ATTR_QNAME                 "qname"
#define SGE_ATTR_QTYPE                 "qtype"
#define SGE_ATTR_SUBORDINATE_LIST      "subordinate_list"
#define SGE_ATTR_MAIL_LIST             "mail_list"
#define SGE_ATTR_QUEUE_LIST            "queue_list"
#define SGE_ATTR_HOSTNAME              "hostname"
#define SGE_ATTR_HOSTLIST              "hostlist"
#define SGE_ATTR_PE_NAME               "pe_name"
#define SGE_ATTR_CKPT_NAME             "ckpt_name"
#define SGE_ATTR_HGRP_NAME             "group_name"
#define SGE_ATTR_RQS_NAME              "name"
#define SGE_ATTR_CALENDAR_NAME         "calendar_name"
#define SGE_ATTR_YEAR                  "year"
#define SGE_ATTR_WEEK                  "week"
#define SGE_ATTR_CKPT_NAME             "ckpt_name"
#define SGE_ATTR_INTERFACE             "interface"
#define SGE_ATTR_CKPT_COMMAND          "ckpt_command"
#define SGE_ATTR_MIGR_COMMAND          "migr_command"
#define SGE_ATTR_RESTART_COMMAND       "restart_command"
#define SGE_ATTR_CLEAN_COMMAND         "clean_command"
#define SGE_ATTR_CKPT_DIR              "ckpt_dir"
#define SGE_ATTR_SIGNAL                "signal"
#define SGE_ATTR_WHEN                  "when"
#define SGE_ATTR_H_FSIZE               "h_fsize"
#define SGE_ATTR_S_FSIZE               "s_fsize"
#define SGE_ATTR_H_RT                  "h_rt"
#define SGE_ATTR_S_RT                  "s_rt"
#define SGE_ATTR_H_CPU                 "h_cpu"
#define SGE_ATTR_S_CPU                 "s_cpu"
#define SGE_ATTR_H_DATA                "h_data"
#define SGE_ATTR_S_DATA                "s_data"
#define SGE_ATTR_H_STACK               "h_stack"
#define SGE_ATTR_S_STACK               "s_stack"
#define SGE_ATTR_H_CORE                "h_core"
#define SGE_ATTR_S_CORE                "s_core"
#define SGE_ATTR_H_RSS                 "h_rss"
#define SGE_ATTR_S_RSS                 "s_rss"
#define SGE_ATTR_H_VMEM                "h_vmem"
#define SGE_ATTR_S_VMEM                "s_vmem"

/* attribute values for certain object attributes */
#define SGE_ATTRVAL_ALL                "all"
#define SGE_ATTRVAL_MIN                "min"
#define SGE_ATTRVAL_MAX                "max"
#define SGE_ATTRVAL_AVG                "avg"

/* tmp filenames */
#define TMP_ERR_FILE_SNBU         "/tmp/sge_messages"
#define TMP_ERR_FILE_EXECD        "/tmp/execd_messages"
#define TMP_ERR_FILE_QMASTER      "/tmp/qmaster_messages"
#define TMP_ERR_FILE_SCHEDD       "/tmp/schedd_messages"
#define TMP_ERR_FILE_SHADOWD      "/tmp/shadowd_messages"
#define TMP_ERR_FILE_QIDLD        "/tmp/qidl_messages"

#define COMMON_DIR               "common"
#define SPOOL_DIR                "spool"
#define QMASTER_DIR              "qmaster"
#define QSI_DIR                  "qsi"

#define QMASTER_PID_FILE          "qmaster.pid"
#define EXECD_PID_FILE            "execd.pid"
#define SCHEDD_PID_FILE           "schedd.pid"
#define SHADOWD_PID_FILE          "shadowd_%s.pid"

#define DEFAULT_ACCOUNT           "sge"
#define DEFAULT_CELL              "default"
#define SHARETREE_FILE            "sharetree"
#define ACTIVE_DIR                "active_jobs"
#define OSJOBID                   "osjobid"
#define ADDGRPID                  "addgrpid"

/* These files exist in the qmaster spool directory. These files may be
 * accessed directly, since they are used after chdir() of qmaster/execd
 * to their spool directory 
 */
#define EXECHOST_DIR              "exec_hosts"  
#define ADMINHOST_DIR             "admin_hosts" 
#define SUBMITHOST_DIR            "submit_hosts"
#define ACCESS_DIR                "access_lists"
#define CQUEUE_DIR                "cqueues"
#define QINSTANCES_DIR            "qinstances"
#define PE_DIR                    "pe"
#define UME_DIR                   "usermapping"
#define HGROUP_DIR                "hostgroups"
#define CENTRY_DIR                "centry"
#define CKPTOBJ_DIR               "ckpt"
#define CAL_DIR                   "calendars"
#define ZOMBIE_DIR                "zombies"
#define RESOURCEQUOTAS_DIR        "resource_quotas"
#define AR_DIR                    "advance_reservations"

#define MAN_FILE                  "managers"
#define OP_FILE                   "operators"
#define SEQ_NUM_FILE              "jobseqnum"
#define ARSEQ_NUM_FILE            "arseqnum"
#define ALIAS_FILE                "host_aliases"
#define ACT_QMASTER_FILE          "act_qmaster"

/* These files exist in the qmaster and execd spool area */
#define EXEC_DIR                  "job_scripts"
#define JOB_DIR                   "jobs"
#define ERR_FILE                  "messages"
#define LIC_INFO_FILE             "lic_information"
#define USER_DIR                  "users"
#define USERSET_DIR               "usersets"
#define PROJECT_DIR               "projects"
#define AFS                       FALSE

#endif /* __SGE_H */
