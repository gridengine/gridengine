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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "sge.h"
#include "sge_conf.h"
#include "sge_stdlib.h"
#include "cull.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "sge_feature.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_profiling.h"
#include "sge_string.h"
#include "sge_userset_qmaster.h"
#include "sge_prog.h"
#include "setup_path.h"
#include "sge_usageL.h"
#include "sge_gdi.h"
#include "sge_time.h"
#include "sge_host.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_userprj.h"
#include "sge_userset.h"

#include "config_file.h"

#include "msg_sgeobjlib.h"

#define SGE_BIN "bin"

extern u_long32 loggingfacility;
lList *Master_Config_List = NULL; 

sge_conf_type conf = { NULL };

bool forbid_reschedule = false;
bool forbid_apperror = false;
bool enable_forced_qdel = false;
bool do_credentials = true;
bool do_authentication = true;
bool use_qidle = false;
bool disable_reschedule = false;

long ptf_max_priority = -999;
long ptf_min_priority = -999;
int execd_priority = 0;
int max_dynamic_event_clients = 99;
bool keep_active = false;

/* reporting params */
bool do_accounting         = true;
bool do_reporting          = false;
bool do_joblog             = false;
int reporting_flush_time   = 15;
int sharelog_time          = 0;

/* allow the simulation of (non existent) hosts */
bool simulate_hosts = false;

/*
 * This value overrides the default scheduler timeout (10 minutes)
 * to allow graceful degradation on extremely busy systems with
 * tens of thousands or hundreds of thousands of pending jobs.
 */

int scheduler_timeout = 0;

/* 
 * Reserved usage flags
 *
 * In SGE, hosts which support DR (dynamic repriorization) default to using
 * actual usage so we initialize the reserved usage flags to false. In
 * SGE, hosts which do not support DR default to using reserved usage for
 * sharetree purposes and actual usage for accounting purposes. For Sge,
 * hosts will default to using actual usage for accounting purposes. Sge
 * does not support the sharetree flag so it doesn't matter.
 */

bool acct_reserved_usage = false;
bool sharetree_reserved_usage = false;

/* 
 * Use primary group of qsub-host also for the job execution
 */
bool use_qsub_gid = false;

/*
 * notify_kill_default and notify_susp_default
 *       0  -> use the signal type stored in notify_kill and notify_susp
 *       1  -> user default signale (USR1 for susp and usr2 for kill)
 *       2  -> do not send a signal
 *
 * notify_kill and notify_susp:
 *       !NULL -> Name of the signale (later used in sys_string2signal)
 */
int   notify_susp_type = 1;
char* notify_susp = NULL;
int   notify_kill_type = 1;
char* notify_kill = NULL; 

extern long compression_level;
extern long compression_threshold;

typedef struct {
  char *name;              /* name of parameter */                              
  int local;               /* 0 | 1 -> local -> may be overidden by local conf */
  char *value;             /* value of parameter */            
  int isSet;               /* 0 | 1 -> is already set */        
  char *envp;              /* pointer to environment variable */
} tConfEntry;

static void setConfFromCull(sge_conf_type *mconf, lList *lpCfg);
static tConfEntry *getConfEntry(char *name, tConfEntry conf_entries[]);
static void clean_conf(sge_conf_type *conf);

#define MAILER                    "/bin/mail"
#define PROLOG                    "none"
#define EPILOG                    "none"
#define SHELL_START_MODE          "posix_compliant"
#define LOGIN_SHELLS              "none"
#define MIN_UID                   "0"
#define MIN_GID                   "0"
#define MAX_UNHEARD               "0:2:30"
#define LOAD_LOG_TIME             "0:0:40"
#define STAT_LOG_TIME             "0:15:0"
#define LOGLEVEL                  "log_info"
#define ADMIN_USER                "none"
#define FINISHED_JOBS             "0"
#define RESCHEDULE_UNKNOWN        "0:0:0"
#define IGNORE_FQDN               "true"
#define MAX_AJ_INSTANCES          "2000"
#define MAX_AJ_TASKS              "75000"
#define MAX_U_JOBS                "0"
#define MAX_JOBS                  "0"
#define REPORTING_PARAMS          "accounting=true reporting=false flush_time=00:00:15 joblog=false sharelog=00:00:00"

static tConfEntry conf_entries[] = {
 { "execd_spool_dir",   1, NULL,                1, NULL },
 { "mailer",            1, MAILER,              1, NULL },
 { "xterm",             1, "/usr/bin/X11/xterm",1, NULL },
 { "load_sensor",       1, "none",              1, NULL },
 { "prolog",            1, PROLOG,              1, NULL },
 { "epilog",            1, EPILOG,              1, NULL },
 { "shell_start_mode",  1, SHELL_START_MODE,    1, NULL },
 { "login_shells",      1, LOGIN_SHELLS,        1, NULL },
 { "min_uid",           0, MIN_UID,             1, NULL },
 { "min_gid",           0, MIN_GID,             1, NULL },
 { "user_lists",        0, "none",              1, NULL },
 { "xuser_lists",       0, "none",              1, NULL },
 { "projects",          0, "none",              1, NULL },
 { "xprojects",         0, "none",              1, NULL },
 { "load_report_time",  1, LOAD_LOG_TIME,       1, NULL },
 { "max_unheard",       0, MAX_UNHEARD,         1, NULL },
 { "loglevel",          0, LOGLEVEL,            1, NULL },
 { "enforce_project",   0, "false",             1, NULL },
 { "enforce_user",      0, "false",             1, NULL },
 { "administrator_mail",0, "none",              1, NULL },
 { "set_token_cmd",     1, "none",              1, NULL },
 { "pag_cmd",           1, "none",              1, NULL },
 { "token_extend_time", 1, "24:0:0",            1, NULL },
 { "shepherd_cmd",      1, "none",              1, NULL },
 { "qmaster_params",    0, "none",              1, NULL }, 
 { "execd_params",      1, "none",              1, NULL }, 
 { "reporting_params",  1, REPORTING_PARAMS,    1, NULL },
 { "gid_range",         1, "none",              1, NULL },
 { "finished_jobs",     0, FINISHED_JOBS,       1, NULL },
 { "qlogin_daemon",     1, "none",              1, NULL },
 { "qlogin_command",    1, "none",              1, NULL },
 { "rsh_daemon",        1, "none",              1, NULL },
 { "rsh_command",       1, "none",              1, NULL },
 { "rlogin_daemon",     1, "none",              1, NULL },
 { "rlogin_command",    1, "none",              1, NULL },
 { "reschedule_unknown",1, RESCHEDULE_UNKNOWN,  1, NULL },
 { "max_aj_instances",  0, MAX_AJ_INSTANCES,    1, NULL },
 { "max_aj_tasks",      0, MAX_AJ_TASKS,        1, NULL },
 { "max_u_jobs",        0, MAX_U_JOBS,          1, NULL },
 { "max_jobs",          0, MAX_JOBS,            1, NULL },
 { REPRIORITIZE,        0, "1",                 1, NULL },
 { "auto_user_oticket", 0, "0",                 1, NULL },
 { "auto_user_fshare",  0, "0",                 1, NULL },
 { "auto_user_default_project", 0, "none",      1, NULL },
 { "auto_user_delete_time",     0, "0",         1, NULL },
 { "delegated_file_staging",    0, "false",     1, NULL },
 { NULL,                0, NULL,                0, 0,   }
};

/*-------------------------------------------------------
 * sge_set_defined_defaults
 * Initialize config list with compiled in values
 * set spool directorys from cell 
 *-------------------------------------------------------*/
lList *sge_set_defined_defaults(lList *lpCfg)
{
   static bool first = true;
   int i; 
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_set_defined_defaults");

   if (first) {
      tConfEntry *pConf;

      first = false;

      pConf = getConfEntry("execd_spool_dir", conf_entries);
      pConf->value = malloc(strlen(path_state_get_cell_root()) + strlen(SPOOL_DIR) + 2);
      sprintf(pConf->value, "%s/%s", path_state_get_cell_root(), SPOOL_DIR);
   }
   else if (lpCfg)
      lFreeList(lpCfg);
      
   lpCfg = NULL;
   i = 0;       
   while (conf_entries[i].name) {
      
      ep = lAddElemStr(&lpCfg, CF_name, conf_entries[i].name, CF_Type);
      lSetString(ep, CF_value, conf_entries[i].value);
      lSetUlong(ep, CF_local, conf_entries[i].local);
      
      i++;
   }      

   DEXIT;
   return lpCfg;
}

/*----------------------------------------------------*
 * chg_conf_val()
 * seeks for a config attribute "name", frees old 
 * value (if string) from *cpp and writes new value into *cpp
 * logging is done to file
 *----------------------------------------------------*/
void chg_conf_val(
lList *lp_cfg,
char *name,
char **cpp,
u_long32 *val,
int type 
) {
   lListElem *ep;
   const char *s;

#ifndef NO_SGE_COMPILE_DEBUG
   char SGE_FUNC[] = "";   
#endif
      
   if ((ep = lGetElemStr(lp_cfg, CF_name, name))) {
      s = lGetString(ep, CF_value);
      if (s) {
         int old_verbose = log_state_get_log_verbose();
  
         /* prevent logging function from writing to stderr
          * but log into log file 
          */
         log_state_set_log_verbose(0);
         INFO((SGE_EVENT, MSG_CONF_USING_SS, s, name));
         log_state_set_log_verbose(old_verbose);
      }
      if (cpp)
         *cpp = sge_strdup(*cpp, s);
      else
         parse_ulong_val(NULL, val, type, s, NULL, 0);    
   }
}


/*---------------------------------------------------------------------*/
static void setConfFromCull(
sge_conf_type *mconf,
lList *lpCfg 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "setConfFromCull");

   /* get following logging entries logged if log_info is selected */
   chg_conf_val(lpCfg, "loglevel", NULL, &mconf->loglevel, TYPE_LOG);
   log_state_set_log_level(mconf->loglevel);
   
   chg_conf_val(lpCfg, "execd_spool_dir", &mconf->execd_spool_dir, NULL, 0);
   chg_conf_val(lpCfg, "mailer", &mconf->mailer, NULL, 0);
   chg_conf_val(lpCfg, "xterm", &mconf->xterm, NULL, 0);
   chg_conf_val(lpCfg, "load_sensor", &mconf->load_sensor, NULL, 0);
   chg_conf_val(lpCfg, "prolog", &mconf->prolog, NULL, 0);
   chg_conf_val(lpCfg, "epilog", &mconf->epilog, NULL, 0);
   chg_conf_val(lpCfg, "shell_start_mode", &mconf->shell_start_mode, NULL, 0);
   chg_conf_val(lpCfg, "login_shells", &mconf->login_shells, NULL, 0);
   chg_conf_val(lpCfg, "min_uid", NULL, &mconf->min_uid, TYPE_INT);
   chg_conf_val(lpCfg, "min_gid", NULL, &mconf->min_gid, TYPE_INT);
   chg_conf_val(lpCfg, "gid_range", &mconf->gid_range, NULL, 0);

   if ((ep = lGetElemStr(lpCfg, CF_name, "user_lists"))) {
      lList *lp = NULL;
      if (!lString2ListNone(lGetString(ep, CF_value), &lp, US_Type, US_name, " \t,")) {
         lFreeList(mconf->user_lists);
         mconf->user_lists = lp;
      }   
   }

   if ((ep = lGetElemStr(lpCfg, CF_name, "xuser_lists"))) {
      lList *lp = NULL;
      if (!lString2ListNone(lGetString(ep, CF_value), &lp, US_Type, US_name, " \t,")) {
         lFreeList(mconf->xuser_lists);
         mconf->xuser_lists = lp;
      }   
   }
   
   if ((ep = lGetElemStr(lpCfg, CF_name, "projects"))) {
      lList *lp = NULL;
      if (!lString2ListNone(lGetString(ep, CF_value), &lp, UP_Type, UP_name, " \t,")) {
         lFreeList(mconf->projects);
         mconf->projects = lp;
      }   
   }

   if ((ep = lGetElemStr(lpCfg, CF_name, "xprojects"))) {
      lList *lp = NULL;
      if (!lString2ListNone(lGetString(ep, CF_value), &lp, UP_Type, UP_name, " \t,")) {
         lFreeList(mconf->xprojects);
         mconf->xprojects = lp;
      }   
   }
   
   chg_conf_val(lpCfg, "load_report_time", NULL, &mconf->load_report_time, TYPE_INT);
   chg_conf_val(lpCfg, "enforce_project", &mconf->enforce_project, NULL, 0);
   chg_conf_val(lpCfg, "enforce_user", &mconf->enforce_user, NULL, 0);
   chg_conf_val(lpCfg, "max_unheard", NULL, &mconf->max_unheard, TYPE_TIM);
   chg_conf_val(lpCfg, "loglevel", NULL, &mconf->loglevel, TYPE_LOG);
   chg_conf_val(lpCfg, "administrator_mail", &mconf->administrator_mail, NULL, 0);
   chg_conf_val(lpCfg, "set_token_cmd", &mconf->set_token_cmd, NULL, 0);
   chg_conf_val(lpCfg, "pag_cmd", &mconf->pag_cmd, NULL, 0);
   chg_conf_val(lpCfg, "token_extend_time", NULL, &mconf->token_extend_time, TYPE_TIM);
   chg_conf_val(lpCfg, "shepherd_cmd", &mconf->shepherd_cmd, NULL, 0);
   chg_conf_val(lpCfg, "qmaster_params", &mconf->qmaster_params, NULL, 0);
   chg_conf_val(lpCfg, "execd_params",  &mconf->execd_params, NULL, 0);
   chg_conf_val(lpCfg, "reporting_params",  &mconf->reporting_params, NULL, 0);
   chg_conf_val(lpCfg, "finished_jobs", NULL, &mconf->zombie_jobs, TYPE_INT);
   chg_conf_val(lpCfg, "qlogin_daemon", &mconf->qlogin_daemon, NULL, 0);
   chg_conf_val(lpCfg, "qlogin_command", &mconf->qlogin_command, NULL, 0);
   chg_conf_val(lpCfg, "rsh_daemon", &mconf->rsh_daemon, NULL, 0);
   chg_conf_val(lpCfg, "rsh_command", &mconf->rsh_command, NULL, 0);
   chg_conf_val(lpCfg, "rlogin_daemon", &mconf->rlogin_daemon, NULL, 0);
   chg_conf_val(lpCfg, "rlogin_command", &mconf->rlogin_command, NULL, 0);

   chg_conf_val(lpCfg, "reschedule_unknown", NULL, &mconf->reschedule_unknown, TYPE_TIM);

   chg_conf_val(lpCfg, "max_aj_instances", NULL, &mconf->max_aj_instances, TYPE_INT);
   chg_conf_val(lpCfg, "max_aj_tasks", NULL, &mconf->max_aj_tasks, TYPE_INT);
   chg_conf_val(lpCfg, "max_u_jobs", NULL, &mconf->max_u_jobs, TYPE_INT);
   chg_conf_val(lpCfg, "max_jobs", NULL, &mconf->max_jobs, TYPE_INT);
   chg_conf_val(lpCfg, REPRIORITIZE, NULL, &mconf->reprioritize, TYPE_BOO );
   chg_conf_val(lpCfg, "auto_user_oticket", NULL, &mconf->auto_user_oticket, TYPE_INT);
   chg_conf_val(lpCfg, "auto_user_fshare", NULL, &mconf->auto_user_fshare, TYPE_INT);
   chg_conf_val(lpCfg, "auto_user_default_project", &mconf->auto_user_default_project, NULL, 0);
   chg_conf_val(lpCfg, "auto_user_delete_time", NULL, &mconf->auto_user_delete_time, TYPE_TIM);
   chg_conf_val(lpCfg, "delegated_file_staging", &mconf->delegated_file_staging, NULL, 0);
   DEXIT;
}

/*----------------------------------------------------*
 * getConfEntry()
 * return a pointer to the config element "name"
 *----------------------------------------------------*/
static tConfEntry *getConfEntry(
char *name,
tConfEntry conf[] 
) {
 int i;
   
 DENTER(BASIS_LAYER, "getConfEntry");

 for (i = 0; conf[i].name; i++) {
    if (!strcasecmp(conf[i].name,name)) {   
       DEXIT;
       return &conf[i];
    }
 }   
     
 DEXIT;
 return NULL;
}

/*----------------------------------------------------------
 * merge_configuration
 * Merge global and local configuration and set lpConfig list
 * set conf struct from lpConfig
 *----------------------------------------------------------*/
int merge_configuration(lListElem *global, lListElem *local,
                        sge_conf_type *pconf, lList **lpp) {
   lList *cl;
   lListElem *elem, *ep2;
   lList *mlist = NULL;
   static int first_time = 1;
 
   DENTER(TOP_LAYER, "merge_configuration");

    
   if (!pconf) {
      DEXIT;
      return -1;
   }

   if (!lpp) {
      lpp = &mlist;
   }
   *lpp = sge_set_defined_defaults(*lpp);

   /* Merge global configuration */
   /*
   ** the error global == NULL is not ignored
   ** handled later
   */
   if (global) {
      cl = lGetList(global, CONF_entries); 
      for_each(elem, cl) {
         ep2 = lGetElemCaseStr(*lpp, CF_name, lGetString(elem, CF_name));
         if (ep2) {
            lSetString(ep2, CF_value, lGetString(elem, CF_value));
         }
      }
   }


   /* Merge in local configuration */
   if (local) {
      cl = lGetList(local, CONF_entries); 
      for_each(elem, cl) {
         ep2 = lGetElemCaseStr(*lpp, CF_name, lGetString(elem, CF_name));
         if (ep2 && lGetUlong(ep2, CF_local)) {
            lSetString(ep2, CF_value, lGetString(elem, CF_value));
         }
      }
   }
  
   
  
   if (first_time) {
      first_time = 0;
      memset(pconf, 0, sizeof(sge_conf_type));
   }
   else 
      clean_conf(&conf);
    
   /* Set conf struct to reflect changes */
   setConfFromCull(pconf, *lpp);

   /* put contents of qmaster_params and execd_params  
      into some convenient global variables */
   {
      const char *s;
      forbid_reschedule = false;
      forbid_apperror = false;
      enable_forced_qdel = false;
      do_credentials = true;
      do_authentication = true;
      compression_level = 0;
      compression_threshold = 10 * 1024;
      use_qidle = false;
      disable_reschedule = false;   
      simulate_hosts = false;
      scheduler_timeout = 0;
      max_dynamic_event_clients = 99;

      for (s=sge_strtok(pconf->qmaster_params, ",; "); s; s=sge_strtok(NULL, ",; ")) {
         if (parse_bool_param(s, "FORBID_RESCHEDULE", &forbid_reschedule)) {
            continue;
         } 
         if (parse_bool_param(s, "FORBID_APPERROR", &forbid_apperror)) {
            continue;
         }   
         if (parse_bool_param(s, "ENABLE_FORCED_QDEL", &enable_forced_qdel)) {
            continue;
         } 
         if (!strncasecmp(s, "MAX_DYN_EC", sizeof("MAX_DYN_EC")-1)) {
            max_dynamic_event_clients = atoi(&s[sizeof("MAX_DYN_EC=")-1]);
            continue;
         }
         if (parse_bool_param(s, "NO_SECURITY", &do_credentials)) {
            /* reversed logic */
            do_credentials = !do_credentials;
            continue;
         } 
         if (parse_bool_param(s, "NO_AUTHENTICATION", &do_authentication)) {
            /* reversed logic */
            do_authentication = !do_authentication;
            continue;
         } 
         if (parse_bool_param(s, "DISABLE_AUTO_RESCHEDULING", &disable_reschedule)) {
            continue;
         }
         if (parse_bool_param(s, "SIMULATE_HOSTS", &simulate_hosts)) {
            continue;
         }
         if (!strncasecmp(s, "COMPRESSION_LEVEL", sizeof("COMPRESSION_LEVEL"))) {
            char *cp;
            cp = strchr(s, '=');
            if (cp && *(cp+1)) {
               compression_level=atoi(cp+1);
               if (compression_level < 0)
                  compression_level = 0;
               else if (compression_level > 9)
                  compression_level = 9;  
            }  
            DPRINTF(("COMPRESSION_LEVEL=%d\n", compression_level));
            continue;
         } 
         if (!strncasecmp(s, "COMPRESSION_THRESHOLD", sizeof("COMPRESSION_THRESHOLD"))) {
            char *cp;
            cp = strchr(s, '=');
            if (cp && *(cp+1)) {
               compression_threshold=atoi(cp+1);
               if (compression_threshold < 0)
                  compression_threshold = 0;
            }  
            DPRINTF(("COMPRESSION_THRESHOLD=%d\n", compression_threshold));
            continue;
         } 
         if (!strncasecmp(s, "SCHEDULER_TIMEOUT",
                    sizeof("SCHEDULER_TIMEOUT")-1)) {
            scheduler_timeout=atoi(&s[sizeof("SCHEDULER_TIMEOUT=")-1]);
            continue;
         }
      }
       
      /* always initialize to defaults before we check execd_params */
#ifdef COMPILE_DC
      acct_reserved_usage = false;
      sharetree_reserved_usage = false;
#else
      acct_reserved_usage = false;
      sharetree_reserved_usage = true;
#endif
      notify_kill_type = 1;
      notify_susp_type = 1;
      ptf_max_priority = -999;
      ptf_min_priority = -999;
      execd_priority = -999;
      keep_active = false;
      use_qsub_gid = false;

      for (s=sge_strtok(pconf->execd_params, ",; "); s; s=sge_strtok(NULL, ",; ")) {
         if (parse_bool_param(s, "USE_QIDLE", &use_qidle)) {
            continue;
         }
         if (uti_state_get_mewho() == EXECD) {
            if (parse_bool_param(s, "NO_SECURITY", &do_credentials)) { 
               /* reversed logic */
               do_credentials = !do_credentials;
               continue;
            }
            if (parse_bool_param(s, "NO_AUTHENTICATION", &do_authentication)) {
               /* reversed logic */
               do_authentication = !do_authentication;
               continue;
            }
            if (parse_bool_param(s, "DO_AUTHENTICATION", &do_authentication)) {
               continue;
            }
         }   
         if (parse_bool_param(s, "KEEP_ACTIVE", &keep_active)) {
            continue;
         }
         if (parse_bool_param(s, "ACCT_RESERVED_USAGE", &acct_reserved_usage)) {
            continue;
         } 
         if (parse_bool_param(s, "SHARETREE_RESERVED_USAGE", &sharetree_reserved_usage)) {
            continue;
         } 
         if (!strncasecmp(s, "NOTIFY_KILL", sizeof("NOTIFY_KILL")-1)) {
            if (!strcasecmp(s, "NOTIFY_KILL=default")) {
               notify_kill_type = 1;
            } else if (!strcasecmp(s, "NOTIFY_KILL=none")) {
               notify_kill_type = 2;
            } else if (!strncasecmp(s, "NOTIFY_KILL=", sizeof("NOTIFY_KILL=")-1)){
               notify_kill_type = 0;
               if (notify_kill) {
                  free(notify_kill);
               }
               notify_kill = sge_strdup(NULL, &(s[sizeof("NOTIFY_KILL")]));
            }
            continue;
         } 
         if (!strncasecmp(s, "NOTIFY_SUSP", sizeof("NOTIFY_SUSP")-1)) {
            if (!strcasecmp(s, "NOTIFY_SUSP=default")) {
               notify_susp_type = 1;
            } else if (!strcasecmp(s, "NOTIFY_SUSP=none")) {
               notify_susp_type = 2;
            } else if (!strncasecmp(s, "NOTIFY_SUSP=", sizeof("NOTIFY_SUSP=")-1)){
               notify_susp_type = 0;
               if (notify_susp) {
                  free(notify_susp);
               }
               notify_susp = sge_strdup(NULL, &(s[sizeof("NOTIFY_SUSP")]));
            }
            continue;
         } 
         if (parse_bool_param(s, "USE_QSUB_GID", &use_qsub_gid)) {
            continue;
         }
         if (!strncasecmp(s, "PTF_MAX_PRIORITY", sizeof("PTF_MAX_PRIORITY")-1)) {
            ptf_max_priority=atoi(&s[sizeof("PTF_MAX_PRIORITY=")-1]);
            continue;
         }
         if (!strncasecmp(s, "PTF_MIN_PRIORITY", sizeof("PTF_MIN_PRIORITY")-1)) {
            ptf_min_priority=atoi(&s[sizeof("PTF_MIN_PRIORITY=")-1]);
            continue;
         }
         if (!strncasecmp(s, "EXECD_PRIORITY", sizeof("EXECD_PRIORITY")-1)) {
            execd_priority=atoi(&s[sizeof("EXECD_PRIORITY=")-1]);
            continue;
         }
      }

      /* parse reporting parameters */
      for (s=sge_strtok(pconf->reporting_params, ",; "); s; s=sge_strtok(NULL, ",; ")) {
         if (parse_bool_param(s, "accounting", &do_accounting)) {
            continue;
         }
         if (parse_bool_param(s, "reporting", &do_reporting)) {
            continue;
         }
         if (parse_bool_param(s, "joblog", &do_joblog)) {
            continue;
         }
         if (parse_int_param(s, "flush_time", &reporting_flush_time, TYPE_TIM)) {
            if (reporting_flush_time <= 0) {
               WARNING((SGE_EVENT, MSG_CONF_NOCONFIGFROMMASTER));
               reporting_flush_time = 15;
            }
            continue;
         }
         if (parse_int_param(s, "sharelog", &sharelog_time, TYPE_TIM)) {
            continue;
         }
      }
   }

   if (mlist) {
      lFreeList(mlist);
   }
 
   if (!global) {
      WARNING((SGE_EVENT, MSG_CONF_NOCONFIGFROMMASTER));
      DEXIT;
      return -2;
   }

   DEXIT;
   return 0;
}

/*******************************************************************/
void sge_show_conf()
{
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_show_conf");

   DPRINTF(("conf.execd_spool_dir        >%s<\n", conf.execd_spool_dir));
   DPRINTF(("conf.mailer                 >%s<\n", conf.mailer));
   DPRINTF(("conf.prolog                 >%s<\n", conf.prolog));
   DPRINTF(("conf.epilog                 >%s<\n", conf.epilog));
   DPRINTF(("conf.shell_start_mode       >%s<\n", conf.shell_start_mode));
   DPRINTF(("conf.login_shells           >%s<\n", conf.login_shells));
   DPRINTF(("conf.administrator_mail     >%s<\n", conf.administrator_mail?conf.administrator_mail:"none"));
   DPRINTF(("conf.min_gid                >%u<\n", (unsigned) conf.min_gid));
   DPRINTF(("conf.min_uid                >%u<\n", (unsigned) conf.min_uid));
   DPRINTF(("conf.load_report_time       >%u<\n", (unsigned) conf.load_report_time));
   DPRINTF(("conf.max_unheard            >%u<\n", (unsigned) conf.max_unheard));
   DPRINTF(("conf.loglevel               >%u<\n", (unsigned) conf.loglevel));     
   DPRINTF(("conf.xterm                  >%s<\n", conf.xterm?conf.xterm:"none"));
   DPRINTF(("conf.load_sensor            >%s<\n", conf.load_sensor?conf.load_sensor:"none"));
   DPRINTF(("conf.enforce_project        >%s<\n", conf.enforce_project?conf.enforce_project:"none"));
   DPRINTF(("conf.enforce_user           >%s<\n", conf.enforce_user?conf.enforce_user:"none"));
   DPRINTF(("conf.set_token_cmd          >%s<\n", conf.set_token_cmd?conf.set_token_cmd:"none"));
   DPRINTF(("conf.pag_cmd                >%s<\n", conf.pag_cmd?conf.pag_cmd:"none"));
   DPRINTF(("conf.token_extend_time      >%u<\n", (unsigned) conf.token_extend_time));
   DPRINTF(("conf.shepherd_cmd           >%s<\n", conf.shepherd_cmd?conf.pag_cmd:"none"));
   DPRINTF(("conf.qmaster_params         >%s<\n", conf.qmaster_params?conf.qmaster_params:"none"));
   DPRINTF(("conf.execd_params           >%s<\n", conf.execd_params?conf.execd_params:"none"));
   DPRINTF(("conf.gid_range              >%s<\n", conf.gid_range?conf.gid_range:"none")); 
   DPRINTF(("conf.zombie_jobs            >%u<\n", (unsigned) conf.zombie_jobs));
   DPRINTF(("conf.qlogin_daemon          >%s<\n", conf.qlogin_daemon?conf.qlogin_daemon:"none"));
   DPRINTF(("conf.qlogin_command         >%s<\n", conf.qlogin_command?conf.qlogin_command:"none"));
   DPRINTF(("conf.rsh_daemon             >%s<\n", conf.rsh_daemon?conf.rsh_daemon:"none"));
   DPRINTF(("conf.rsh_command            >%s<\n", conf.rsh_command?conf.rsh_command:"none"));
   DPRINTF(("conf.rlogin_daemon          >%s<\n", conf.rlogin_daemon?conf.rlogin_daemon:"none"));
   DPRINTF(("conf.rlogin_command         >%s<\n", conf.rlogin_command?conf.rlogin_command:"none"));
   DPRINTF(("conf.reschedule_unknown     >%u<\n", (unsigned) conf.reschedule_unknown));
   DPRINTF(("conf.max_aj_instances       >%u<\n", (unsigned) conf.max_aj_instances));
   DPRINTF(("conf.max_aj_tasks           >%u<\n", (unsigned) conf.max_aj_tasks));
   DPRINTF(("conf.max_u_jobs             >%u<\n", (unsigned) conf.max_u_jobs));
   DPRINTF(("conf.max_jobs               >%u<\n", (unsigned) conf.max_jobs));
   DPRINTF(("conf.reprioritize           >%u<\n", conf.reprioritize));
   DPRINTF(("conf.auto_user_oticket      >%u<\n", conf.auto_user_oticket));
   DPRINTF(("conf.auto_user_fshare       >%u<\n", conf.auto_user_fshare));
   DPRINTF(("conf.auto_user_default_project >%s<\n", conf.auto_user_default_project));
   DPRINTF(("conf.auto_user_delete_time  >%u<\n", conf.auto_user_delete_time));
   DPRINTF(("conf.delegated_file_staging >%s<\n", conf.delegated_file_staging));

   for_each (ep, conf.user_lists) {
      DPRINTF(("%s             >%s<\n", 
              lPrev(ep)?"             ":"conf.user_lists", 
              lGetString(ep, US_name)));
   }
   for_each (ep, conf.xuser_lists) {
      DPRINTF(("%s            >%s<\n", 
              lPrev(ep)?"              ":"conf.xuser_lists", 
              lGetString(ep, US_name)));
   }

   for_each (ep, conf.projects) {
      DPRINTF(("%s             >%s<\n", 
              lPrev(ep)?"             ":"conf.projects", 
              lGetString(ep, UP_name)));
   }
   for_each (ep, conf.xprojects) {
      DPRINTF(("%s            >%s<\n", 
              lPrev(ep)?"              ":"conf.xprojects", 
              lGetString(ep, UP_name)));
   }

   DEXIT;
   return;
}


static void clean_conf(
sge_conf_type *conf 
) {
   FREE(conf->execd_spool_dir);
   FREE(conf->mailer);
   FREE(conf->xterm);
   FREE(conf->load_sensor);
   FREE(conf->prolog);
   FREE(conf->epilog);
   FREE(conf->shell_start_mode);
   FREE(conf->login_shells);
   FREE(conf->enforce_project);
   FREE(conf->enforce_user);
   FREE(conf->administrator_mail);
   lFreeList(conf->user_lists);
   lFreeList(conf->xuser_lists);
   lFreeList(conf->projects);
   lFreeList(conf->xprojects);
   FREE(conf->set_token_cmd);
   FREE(conf->pag_cmd);
   FREE(conf->shepherd_cmd);
   FREE(conf->qmaster_params);
   FREE(conf->execd_params);
   FREE(conf->gid_range);
   FREE(conf->qlogin_daemon);
   FREE(conf->qlogin_command);
   FREE(conf->rsh_daemon);
   FREE(conf->rsh_command);
   FREE(conf->rlogin_daemon);
   FREE(conf->rlogin_command);
   FREE(conf->delegated_file_staging);
   
   memset(conf, 0, sizeof(sge_conf_type));

   return;
}

bool sge_is_reprioritize(void){
   lListElem *confl = NULL; 
   lList *ep_list = NULL;
   lListElem *ep = NULL; 
   bool ret = true;
   DENTER(TOP_LAYER, "sge_is_reprioritize");
   confl = lCopyElem(lGetElemHost(Master_Config_List, CONF_hname, "global"));
   if (confl)
    ep_list = lGetList(confl, CONF_entries);
    
   if (ep_list)
      ep = lGetElemStr(ep_list, CF_name, REPRIORITIZE);

   if (ep){
      const char* value;
      value = lGetString(ep, CF_value);
      ret = strncasecmp(value, "0", sizeof("0"));
   }
   else{
      ret = conf.reprioritize;
   }

   DEXIT;
   return ret;
}

