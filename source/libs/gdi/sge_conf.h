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

struct confel {                       /* cluster configuration parameters */
    char        *qmaster_spool_dir;   /* qmaster spool directory path */
    char        *execd_spool_dir;     /* sge_spool directory base path */
    char        *binary_path;         /* path to the Sge binaries */
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
    u_long32    stat_log_time;        /* how often to log stats */
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
    char        *schedd_params;
    char        *execd_params;
    char        *gid_range;           /* Range of additional group ids */
    char        *admin_user;
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
};

typedef struct confel sge_conf_type;

typedef int (*tDaemonizeFunc)(void);

typedef enum {
   FIRST_POLICY_VALUE,
   INVALID_POLICY = FIRST_POLICY_VALUE,

   OVERRIDE_POLICY,
   FUNCTIONAL_POLICY,
   SHARE_TREE_POLICY,
   DEADLINE_POLICY,

   LAST_POLICY_VALUE,
   POLICY_VALUES = (LAST_POLICY_VALUE - FIRST_POLICY_VALUE)
} policy_type_t;

 
typedef struct {
   policy_type_t policy;
   int dependent;
} policy_hierarchy_t;  

extern const char *const policy_hierarchy_chars; 
extern char policy_hierarchy_string[5];

char policy_hierarchy_enum2char(policy_type_t value);

policy_type_t policy_hierarchy_char2enum(char character);

int policy_hierarchy_verify_value(const char* value);

void policy_hierarchy_fill_array(policy_hierarchy_t array[], const char* value);

void policy_hierarchy_print_array(policy_hierarchy_t array[]);

extern sge_conf_type conf;

extern int use_qidle;
extern int forbid_reschedule;
extern int do_credentials;   
extern int do_authentication;  
extern int acct_reserved_usage;
extern int sharetree_reserved_usage;
extern int flush_submit_sec; 
extern int flush_finish_sec;
extern int profile_schedd;
#ifdef PROFILE_MASTER
extern int profile_master;
#endif 
extern int keep_active;
extern int simulate_hosts;
extern long ptf_max_priority;
extern long ptf_min_priority;
extern int use_qsub_gid;
extern int notify_susp_type;      
extern char* notify_susp;       
extern int notify_kill_type;      
extern char* notify_kill;
extern int disable_reschedule;  
extern int set_sge_environment;
extern int set_grd_environment;
extern int set_cod_environment;
extern int classic_sgeee_scheduling;
extern int share_override_tickets;                                         
extern int share_functional_shares;                                        
extern int share_deadline_tickets;                                         
extern int max_functional_jobs_to_schedule;                                
extern int max_pending_tasks_per_job;                                      
extern lList* halflife_decay_list;                                         
extern int scheduler_timeout;                                      


/* simulation of large clusters: 
 *  - load values will not be trashed
 *  - no jobs will be delivered to unheared hosts
*/
extern int skip_unheared_host; 

lList *sge_set_defined_defaults(lList *lpCfg);
int merge_configuration(lListElem *global, lListElem *local, sge_conf_type *pconf, lList **lpp);
void sge_show_conf(void);

int get_conf_and_daemonize(tDaemonizeFunc dfunc, lList **conf_list);
int get_configuration(char *config_name, lListElem **gepp, lListElem **lepp);
int get_merged_configuration(lList **conf_list);
void chg_conf_val(lList *lp_cfg, char *name, char **field, u_long32 *val, int type);

#endif /* __SGE_CONF_H */
