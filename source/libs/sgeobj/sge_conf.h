#ifndef __SGE_CONF_H
#define __SGE_CONF_H
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

#include "sge_confL.h"
/* #include "sge_mirror.h" */

/* The scheduler configuration changes this configuration element only. It is
   not spooled and is not shown in qconf -mconf */
#define REPRIORITIZE "reprioritize"

struct confel {                       /* cluster configuration parameters */
    char        *execd_spool_dir;     /* sge_spool directory base path */
    char        *mailer;              /* path to e-mail delivery agent */
    char        *xterm;               /* xterm path for interactive jobs */
    char        *load_sensor;         /* path to a load sensor executable */    
    char        *prolog;              /* start before jobscript may be none */
    char        *epilog;              /* start after jobscript may be none */
    char        *shell_start_mode;    /* script_from_stdin/posix_compliant/unix_behavior */
    char        *login_shells;        /* list of shells to call as login shell */
    u_long32    min_uid;              /* lower bound on UIDs that can qsub */
    u_long32    min_gid;              /* lower bound on GIDs that can qsub */
    u_long32    load_report_time;     /* how often to send in load */
    u_long32    max_unheard;          /* how long before sge_execd considered dead */
    u_long32    loglevel;             /* qmaster event logging level */
    char        *enforce_project;     /* SGEEE attribute: "true" or "false" */
    char        *enforce_user;        /* SGEEE attribute: "true" or "false" */
    char        *administrator_mail;  /* list of mail addresses */
    lList       *user_lists;          /* allowed user lists */
    lList       *xuser_lists;         /* forbidden users lists */
    lList       *projects;            /* allowed project list */
    lList       *xprojects;           /* forbiddent project list */
    char        *set_token_cmd;
    char        *pag_cmd;
    u_long32    token_extend_time;
    char        *shepherd_cmd;
    char        *qmaster_params;
    char        *execd_params;
    char        *reporting_params;
    char        *gid_range;           /* Range of additional group ids */
    u_long32    zombie_jobs;          /* jobs to save after execution */
    char        *qlogin_daemon;       /* eg /usr/sbin/in.telnetd */
    char        *qlogin_command;      /* eg telnet $HOST $PORT */
    char        *rsh_daemon;       /* eg /usr/sbin/in.rshd */
    char        *rsh_command;      /* eg rsh -p $PORT $HOST command */
    char        *rlogin_daemon;       /* eg /usr/sbin/in.rlogind */
    char        *rlogin_command;      /* eg rlogin -p $PORT $HOST */
    u_long32    reschedule_unknown;   /* timout value used for auto. resch. */ 
    u_long32    max_aj_instances;     /* max. number of ja instances of a job */
    u_long32    max_aj_tasks;         /* max. size of an array job */
    u_long32    max_u_jobs;           /* max. number of jobs per user */
    u_long32    max_jobs;             /* max. number of jobs in the system */
    u_long32    reprioritize;         /* reprioritize jobs based on the tickets or not */
    u_long32    auto_user_fshare;     /* SGEEE automatic user fshare */
    u_long32    auto_user_oticket;    /* SGEEE automatic user oticket */
    char        *auto_user_default_project; /* SGEEE automatic user default project */
    u_long32    auto_user_delete_time; /* SGEEE automatic user delete time */
};

typedef struct confel sge_conf_type;

typedef int (*tDaemonizeFunc)(void);

extern lList *Master_Config_List;

extern sge_conf_type conf;

extern bool use_qidle;
extern bool forbid_reschedule;
extern bool forbid_apperror;
extern bool do_credentials;   
extern bool do_authentication;  
extern bool acct_reserved_usage;
extern bool sharetree_reserved_usage;
extern bool keep_active;
extern bool simulate_hosts;
extern long ptf_max_priority;
extern long ptf_min_priority;
extern bool use_qsub_gid;
extern int notify_susp_type;      
extern char* notify_susp;       
extern int notify_kill_type;      
extern char* notify_kill;
extern bool disable_reschedule;  
extern bool set_sge_environment;
extern bool set_grd_environment;
extern bool set_cod_environment;
extern int scheduler_timeout;

/* reporting params */
extern bool do_accounting;
extern bool do_reporting;
extern bool do_joblog;
extern int reporting_flush_time;
extern int sharelog_time;

/* simulation of large clusters: 
 *  - load values will not be trashed
 *  - no jobs will be delivered to unheared hosts
*/
extern int skip_unheared_host; 

lList *sge_set_defined_defaults(lList *lpCfg);
int merge_configuration(lListElem *global, lListElem *local, sge_conf_type *pconf, lList **lpp);
void sge_show_conf(void);

bool sge_is_reprioritize(void);
#endif /* __SGE_CONF_H */
