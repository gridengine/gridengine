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

#include "def.h"
#include "sge.h"
#include "sge_conf.h"
#include "sge_me.h"
#include "sge_confL.h"
#include "sge_usersetL.h"
#include "sge_userprjL.h"
#include "sge_answerL.h"
#include "cull.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "rw_configuration.h"
#include "sge_feature.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_userset_qmaster.h"
#include "sge_prognames.h"
#include "setup_path.h"
#include "sge_confL.h"
#include "sge_hostL.h"
#include "sge_usageL.h"
#include "sge_gdi.h"
#include "sge_time.h"
#include "resolve_host.h"
#include "msg_common.h"
#include "sge_string.h"
#include "sge_dirent.h"

#define SGE_BIN "bin"

const char *const policy_hierarchy_chars = "OFSD";

extern u_long32 logginglevel;
extern u_long32 loggingfacility;
extern lList *Master_Config_List; 

sge_conf_type conf;

int forbid_reschedule = 0;
int enable_forced_qdel = 0;
int do_credentials = 1;
int do_authentication = 1;
int use_qidle = 0;
int disable_reschedule = 0;

int flush_submit_sec = -1;
int flush_finish_sec = -1;
int profile_schedd = 0;
int profile_master = 0;
 
long ptf_max_priority = -999;
long ptf_min_priority = -999;
int execd_priority = 0;
int keep_active = 0;

/* allow the simulation of (non existent) hosts */
int simulate_hosts = 0;


/*
 * SGEEE pending job scheduling algorithm parameters:
 *
 *      classic_sgeee_scheduling - Use the original SGEEE pending job scheduling
 *              algorithm.
 *      share_override_tickets - Override tickets of any object instance
 *              are shared equally among all jobs associated with the object.
 *      share_functional_shares - Functional shares of any object instance
 *              are shared among all the jobs associated with the object.
 *      share_deadline_tickets - Deadline tickets are shared among all the
 *              deadline jobs.
 *              are shared among all the jobs associated with the object.
 *      max_functional_jobs_to_schedule - The maximum number of functional
 *              pending jobs to scheduling using the brute-force method.
 *      max_pending_tasks_per_job - The number of subtasks per pending job to
 *              schedule. This parameter exists in order to reduce overhead.
 *      halflife_decay_list - A list of halflife decay values (UA_Type)
 *
 *
 */

int classic_sgeee_scheduling = 0;
int share_override_tickets = 1;
int share_functional_shares = 1;
int share_deadline_tickets = 1;
int max_functional_jobs_to_schedule = 200;
int max_pending_tasks_per_job = 50;
lList *halflife_decay_list = NULL;


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

int acct_reserved_usage = 0;
int sharetree_reserved_usage = 0;

/* 
 * Use primary group of qsub-host also for the job execution
 */
int use_qsub_gid = 0;

/*
 * Set job environment variables with following prefixes if the
 * corresponding variables are true:
 *
 *   set_sge_environment => SGE_
 *   set_cod_environment => COD_
 *   set_grd_environment => GRD_
 */
int set_sge_environment = 1;
int set_cod_environment = 0;
int set_grd_environment = 0;

/* 
 * Set if admin explicitly deactivated SGEEE PTF component 
 * initual job priorities remain in effect also in SGEEE
 */
int deactivate_ptf = 0;

/*
 * Activate policy hierarchy in case of SGEEE if not NULL
 * and not "NONE"
 */
char policy_hierarchy_string[5] = "";

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
#define MAX_U_TASKS               "0"

static tConfEntry conf_entries[] = {
 { "qmaster_spool_dir", 0, NULL,                1, NULL },
 { "execd_spool_dir",   1, NULL,                1, NULL },
 { "binary_path",       1, NULL,                1, NULL },
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
 { "stat_log_time",     0, STAT_LOG_TIME,       1, NULL },
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
 { "schedd_params",     0, "none",              1, NULL }, 
 { "execd_params",      1, "none",              1, NULL }, 
 { "gid_range",         1, "none",              1, NULL },
 { "admin_user",        0, ADMIN_USER,          1, NULL },
 { "finished_jobs",     0, FINISHED_JOBS,       1, NULL },
 { "qlogin_daemon",     1, "none",              1, NULL },
 { "qlogin_command",    1, "none",              1, NULL },
 { "rsh_daemon",        1, "none",              1, NULL },
 { "rsh_command",       1, "none",              1, NULL },
 { "rlogin_daemon",     1, "none",              1, NULL },
 { "rlogin_command",    1, "none",              1, NULL },
 { "default_domain",    1, "none",              1, NULL },
 { "reschedule_unknown",1, RESCHEDULE_UNKNOWN,  1, NULL },
 { "ignore_fqdn",       0, IGNORE_FQDN,         1, NULL },
 { "max_aj_instances",  0, MAX_AJ_INSTANCES,    1, NULL },
 { "max_aj_tasks",      0, MAX_AJ_TASKS,        1, NULL },
 { "max_u_jobs",        0, MAX_U_TASKS,         1, NULL },
 { NULL,                0, NULL,                0, 0,   }
};

/*-------------------------------------------------------
 * sge_set_defined_defaults
 * Initialize config list with compiled in values
 * set spool directorys from cell 
 *-------------------------------------------------------*/
lList *sge_set_defined_defaults(lList *lpCfg)
{
   static int first = TRUE;
   int i; 
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_set_defined_defaults");

   if (first) {
      tConfEntry *pConf;

      first = FALSE;

      pConf = getConfEntry("qmaster_spool_dir", conf_entries);
      pConf->value = malloc(strlen(path.cell_root) + strlen(SPOOL_DIR) + 
                              strlen(QMASTER_DIR) + 3);
      sprintf(pConf->value, "%s/%s/%s", path.cell_root, SPOOL_DIR, QMASTER_DIR);

      pConf = getConfEntry("execd_spool_dir", conf_entries);
      pConf->value = malloc(strlen(path.cell_root) + strlen(SPOOL_DIR) + 2);
      sprintf(pConf->value, "%s/%s", path.cell_root, SPOOL_DIR);

      pConf = getConfEntry("binary_path", conf_entries);
      pConf->value = malloc(strlen(path.sge_root) + strlen(SGE_BIN) + 2);
      sprintf(pConf->value, "%s/%s", path.sge_root, SGE_BIN);
   }
   else if (lpCfg)
      lFreeList(lpCfg);
      
   lpCfg = NULL;
   i = 0;       
   while (conf_entries[i].name) {
      if (feature_is_enabled(FEATURE_SGEEE) ||             
          strcmp("enforce_project", conf_entries[i].name) || 
          strcmp("enforce_user", conf_entries[i].name)) {
         ep = lAddElemStr(&lpCfg, CF_name, conf_entries[i].name, CF_Type);
         lSetString(ep, CF_value, conf_entries[i].value);
         lSetUlong(ep, CF_local, conf_entries[i].local);
      }   
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

   char SGE_EVENT[MAX_STRING_SIZE];
#ifndef NO_SGE_COMPILE_DEBUG
   char SGE_FUNC[] = "";   
#endif
      
   if ((ep = lGetElemStr(lp_cfg, CF_name, name))) {
      s = lGetString(ep, CF_value);
      if (s) {
         u_long32 old_me_daemonized;
  
         /* prevent logging function from writing to stderr
          * but log into log file 
          */
         old_me_daemonized = me.daemonized;
         me.daemonized = 1;
         INFO((SGE_EVENT, MSG_GDI_USING_SS, s, name));
         me.daemonized = old_me_daemonized;
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
   u_long32 uval_tmp;

   DENTER(TOP_LAYER, "setConfFromCull");
   
   /* get following logging entries logged if log_info is selected */
   chg_conf_val(lpCfg, "loglevel", NULL, &mconf->loglevel, TYPE_LOG);
   logginglevel = mconf->loglevel;
   
   chg_conf_val(lpCfg, "qmaster_spool_dir", &mconf->qmaster_spool_dir, NULL, 0);
   chg_conf_val(lpCfg, "execd_spool_dir", &mconf->execd_spool_dir, NULL, 0);
   chg_conf_val(lpCfg, "binary_path", &mconf->binary_path, NULL, 0);
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
   chg_conf_val(lpCfg, "stat_log_time", NULL, &mconf->stat_log_time, TYPE_TIM);
   chg_conf_val(lpCfg, "max_unheard", NULL, &mconf->max_unheard, TYPE_TIM);
   chg_conf_val(lpCfg, "loglevel", NULL, &mconf->loglevel, TYPE_LOG);
   chg_conf_val(lpCfg, "administrator_mail", &mconf->administrator_mail, NULL, 0);
   chg_conf_val(lpCfg, "set_token_cmd", &mconf->set_token_cmd, NULL, 0);
   chg_conf_val(lpCfg, "pag_cmd", &mconf->pag_cmd, NULL, 0);
   chg_conf_val(lpCfg, "token_extend_time", NULL, &mconf->token_extend_time, TYPE_TIM);
   chg_conf_val(lpCfg, "shepherd_cmd", &mconf->shepherd_cmd, NULL, 0);
   chg_conf_val(lpCfg, "qmaster_params", &mconf->qmaster_params, NULL, 0);
   chg_conf_val(lpCfg, "schedd_params", &mconf->schedd_params, NULL, 0);
   chg_conf_val(lpCfg, "execd_params",  &mconf->execd_params, NULL, 0);
   chg_conf_val(lpCfg, "admin_user", &mconf->admin_user, NULL, 0);
   chg_conf_val(lpCfg, "finished_jobs", NULL, &mconf->zombie_jobs, TYPE_INT);
   chg_conf_val(lpCfg, "qlogin_daemon", &mconf->qlogin_daemon, NULL, 0);
   chg_conf_val(lpCfg, "qlogin_command", &mconf->qlogin_command, NULL, 0);
   chg_conf_val(lpCfg, "rsh_daemon", &mconf->rsh_daemon, NULL, 0);
   chg_conf_val(lpCfg, "rsh_command", &mconf->rsh_command, NULL, 0);
   chg_conf_val(lpCfg, "rlogin_daemon", &mconf->rlogin_daemon, NULL, 0);
   chg_conf_val(lpCfg, "rlogin_command", &mconf->rlogin_command, NULL, 0);
   chg_conf_val(lpCfg, "default_domain", &default_domain, NULL, 0);
   chg_conf_val(lpCfg, "reschedule_unknown", NULL, &mconf->reschedule_unknown, TYPE_TIM);
   chg_conf_val(lpCfg, "ignore_fqdn", NULL, &uval_tmp, TYPE_TIM);  
         fqdn_cmp = !uval_tmp;  /* logic of ignore_fqdn and fqdn_cmp are contrary */
   chg_conf_val(lpCfg, "max_aj_instances", NULL, &mconf->max_aj_instances, TYPE_INT);
   chg_conf_val(lpCfg, "max_aj_tasks", NULL, &mconf->max_aj_tasks, TYPE_INT);
   chg_conf_val(lpCfg, "max_u_jobs", NULL, &mconf->max_u_jobs, TYPE_INT);

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
      forbid_reschedule = 0;
      enable_forced_qdel = 0;
      do_credentials = 1;
      do_authentication = 1;
      compression_level = 0;
      compression_threshold = 10 * 1024;
      use_qidle = 0;
      disable_reschedule = 0;   
      simulate_hosts = 0;
      scheduler_timeout = 0;
      profile_master = 0;

      for (s=sge_strtok(pconf->qmaster_params, ",; "); s; s=sge_strtok(NULL, ",; "))
         if (!strcasecmp(s, "FORBID_RESCHEDULE")) {
            DPRINTF(("FORBID_RESCHEDULE\n"));
            forbid_reschedule = 1;
         } else if (!strcasecmp(s, "ENABLE_FORCED_QDEL")) {
            DPRINTF(("ENABLE_FORCED_QDEL\n"));
            enable_forced_qdel = 1; 
         } else if (!strcasecmp(s, "NO_SECURITY")) {
            DPRINTF(("NO_SECURITY\n"));
            do_credentials = 0;
         } else if (!strcasecmp(s, "NO_AUTHENTICATION")) {
            DPRINTF(("NO_AUTHENTICATION\n"));
            do_authentication = 0;
         } else if (!strcasecmp(s, "DISABLE_AUTO_RESCHEDULING=true") ||
                    !strcasecmp(s, "DISABLE_AUTO_RESCHEDULING=1")) {
            DPRINTF(("DISABLE_AUTO_RESCHEDULING\n"));
            disable_reschedule = 1;    
         } else if (!strcasecmp(s, "SIMULATE_HOSTS=1")) {
            simulate_hosts = 1;   
         }  else if (!strncasecmp(s, "COMPRESSION_LEVEL", sizeof("COMPRESSION_LEVEL"))) {
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
         } 
         else if (!strncasecmp(s, "COMPRESSION_THRESHOLD", sizeof("COMPRESSION_THRESHOLD"))) {
            char *cp;
            cp = strchr(s, '=');
            if (cp && *(cp+1)) {
               compression_threshold=atoi(cp+1);
               if (compression_threshold < 0)
                  compression_threshold = 0;
            }  
            DPRINTF(("COMPRESSION_THRESHOLD=%d\n", compression_threshold));
         } else if (!strncasecmp(s, "SCHEDULER_TIMEOUT",
                    sizeof("SCHEDULER_TIMEOUT")-1)) {
            scheduler_timeout=atoi(&s[sizeof("SCHEDULER_TIMEOUT=")-1]);
         } else if (!strncasecmp(s, "PROFILE=1", sizeof("PROFILE=1")-1)) {
            profile_master = 1;
         }
       
      /* always initialize to defaults before we check execd_params */
#ifdef COMPILE_DC
      acct_reserved_usage = 0;
      sharetree_reserved_usage = 0;
#else
      acct_reserved_usage = 0;
      sharetree_reserved_usage = (feature_is_enabled(FEATURE_SGEEE)!=0);
#endif
      notify_kill_type = 1;
      notify_susp_type = 1;
      ptf_max_priority = -999;
      ptf_min_priority = -999;
      execd_priority = -999;
      keep_active = 0;
      use_qsub_gid = 0;
      set_sge_environment = 1;
      set_cod_environment = 0;
      set_grd_environment = 0; 
      deactivate_ptf = 0; 
      policy_hierarchy_string[0] = '\0';

      for (s=sge_strtok(pconf->execd_params, ",; "); s; s=sge_strtok(NULL, ",; "))
         if (!strcasecmp(s, "USE_QIDLE")) {
            DPRINTF(("USE_QIDLE\n"));
            use_qidle = 1;
         } else if (!strcasecmp(s, "NO_SECURITY")) { 
            if (me.who == EXECD) {
               DPRINTF(("NO_SECURITY\n"));
               do_credentials = 0;
            }
         } else if (!strcasecmp(s, "KEEP_ACTIVE=true") ||
                    !strcasecmp(s, "KEEP_ACTIVE=1"))
            keep_active = 1;
         else if (!strcasecmp(s, "NO_AUTHENTICATION")) {
            if (me.who == EXECD) {
               DPRINTF(("NO_AUTHENTICATION\n"));
               do_authentication = 0;
            }
         } else if (!strcasecmp(s, "DO_AUTHENTICATION")) {
            if (me.who == EXECD) {
               DPRINTF(("DO_AUTHENTICATION\n"));
               do_authentication = 1;
            }
         } else if (!strncasecmp(s, "ACCT_RESERVED_USAGE",
                                 sizeof("ACCT_RESERVED_USAGE")-1)) {
            if (!strcasecmp(s, "ACCT_RESERVED_USAGE=false") ||
                !strcasecmp(s, "ACCT_RESERVED_USAGE=0"))
               acct_reserved_usage=0;
            else
               acct_reserved_usage=1;
         } else if (!strncasecmp(s, "SHARETREE_RESERVED_USAGE",
                                 sizeof("SHARETREE_RESERVED_USAGE")-1)) {
            if (!strcasecmp(s, "SHARETREE_RESERVED_USAGE=false") ||
                !strcasecmp(s, "SHARETREE_RESERVED_USAGE=0"))
               sharetree_reserved_usage=0;
            else
               sharetree_reserved_usage=1;
         } else if (!strncasecmp(s, "NOTIFY_KILL", sizeof("NOTIFY_KILL")-1)) {
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
         } else if (!strncasecmp(s, "NOTIFY_SUSP", sizeof("NOTIFY_SUSP")-1)) {
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
         } else if (!strncasecmp(s, "USE_QSUB_GID",
                                 sizeof("USE_QSUB_GID")-1)) {
            if (!strcasecmp(s, "USE_QSUB_GID=true") ||
                !strcasecmp(s, "USE_QSUB_GID=1"))
               use_qsub_gid=1;
            else
               use_qsub_gid=0;  
         } else if (!strncasecmp(s, "SET_SGE_ENV", sizeof("SET_SGE_ENV")-1)) {
            if (!strcasecmp(s, "SET_SGE_ENV=true") || !strcasecmp(s, "SET_SGE_ENV=1")) {
               set_sge_environment = 1;                        
            } else {
               set_sge_environment = 0;                        
            }
         } else if (!strncasecmp(s, "SET_COD_ENV", sizeof("SET_COD_ENV")-1)) {
            if (!strcasecmp(s, "SET_COD_ENV=true") || !strcasecmp(s, "SET_COD_ENV=1")) {
               set_cod_environment = 1;                        
            } else {
               set_cod_environment = 0;                        
            }
         } else if (!strncasecmp(s, "SET_GRD_ENV", sizeof("SET_GRD_ENV")-1)) {
            if (!strcasecmp(s, "SET_GRD_ENV=true") || !strcasecmp(s, "SET_GRD_ENV=1")) {
               set_grd_environment = 1;                        
            } else {
               set_grd_environment = 0;                        
            }
         } else if (!strncasecmp(s, "NO_REPRIORITIZATION", sizeof("NO_REPRIORITIZATION")-1)) {
            if (!strcasecmp(s, "NO_REPRIORITIZATION=true") || !strcasecmp(s, "NO_REPRIORITIZATION=1")) {
               deactivate_ptf = 1;                        
            } else {
               deactivate_ptf = 0;                        
            }
         } else if (!strncasecmp(s, "PTF_MAX_PRIORITY", sizeof("PTF_MAX_PRIORITY")-1))
            ptf_max_priority=atoi(&s[sizeof("PTF_MAX_PRIORITY=")-1]);
         else if (!strncasecmp(s, "PTF_MIN_PRIORITY", sizeof("PTF_MIN_PRIORITY")-1))
            ptf_min_priority=atoi(&s[sizeof("PTF_MIN_PRIORITY=")-1]);
         else if (!strncasecmp(s, "EXECD_PRIORITY", sizeof("EXECD_PRIORITY")-1))
            execd_priority=atoi(&s[sizeof("EXECD_PRIORITY=")-1]);

      flush_submit_sec = -1;
      flush_finish_sec = -1;
      profile_schedd = 0;
#ifdef PROFILEMASTER
      profile_master = 0;
#endif
      classic_sgeee_scheduling = 0;
      share_override_tickets = 1;
      share_functional_shares = 1;
      share_deadline_tickets = 1;
      max_functional_jobs_to_schedule = 200;
      max_pending_tasks_per_job = 50;
      halflife_decay_list = NULL;
      
      for (s=sge_strtok(pconf->schedd_params, ",; "); s; s=sge_strtok(NULL, ",; ")) {
         if (!strncasecmp(s, "FLUSH_SUBMIT_SEC", sizeof("FLUSH_SUBMIT_SEC")-1)) {
            flush_submit_sec = atoi(&s[sizeof("FLUSH_SUBMIT_SEC=")-1]);
            if (flush_submit_sec < 0)
               flush_submit_sec=-1;  
         } else if (!strncasecmp(s, "FLUSH_FINISH_SEC", sizeof("FLUSH_FINISH_SEC")-1)) {
            flush_finish_sec = atoi(&s[sizeof("FLUSH_FINISH_SEC=")-1]);
            if (flush_finish_sec < 0)
               flush_finish_sec=-1;  
         } else if (!strcasecmp(s, "CLASSIC_SGEEE_SCHEDULING=true") ||
                    !strcasecmp(s, "CLASSIC_SGEEE_SCHEDULING=1")) {
            classic_sgeee_scheduling = 1;
            share_override_tickets = 1;
            share_functional_shares = 0;
            share_deadline_tickets = 1;
         } else if (!strcasecmp(s, "CLASSIC_SGEEE_SCHEDULING=false") ||
                    !strcasecmp(s, "CLASSIC_SGEEE_SCHEDULING=0")) {
            classic_sgeee_scheduling = 0;
         } else if (!strcasecmp(s, "SHARE_OVERRIDE_TICKETS=true") ||
                    !strcasecmp(s, "SHARE_OVERRIDE_TICKETS=1")) {
            share_override_tickets = 1;
         } else if (!strcasecmp(s, "SHARE_OVERRIDE_TICKETS=false") ||
                    !strcasecmp(s, "SHARE_OVERRIDE_TICKETS=0")) {
            share_override_tickets = 0;
         } else if (!strcasecmp(s, "SHARE_FUNCTIONAL_SHARES=false") ||
                    !strcasecmp(s, "SHARE_FUNCTIONAL_SHARES=0")) {
            share_functional_shares = 0;
         } else if (!strcasecmp(s, "SHARE_DEADLINE_TICKETS=true") ||
                    !strcasecmp(s, "SHARE_DEADLINE_TICKETS=1")) {
            share_deadline_tickets = 1;
         } else if (!strcasecmp(s, "SHARE_DEADLINE_TICKETS=false") ||
                    !strcasecmp(s, "SHARE_DEADLINE_TICKETS=0")) {
            share_deadline_tickets = 0;
         } else if (!strcasecmp(s, "SHARE_FUNCTIONAL_SHARES=false") ||
                    !strcasecmp(s, "SHARE_FUNCTIONAL_SHARES=0")) {
            share_functional_shares = 0;
         } else if (!strncasecmp(s, "MAX_FUNCTIONAL_JOBS_TO_SCHEDULE",
                    sizeof("MAX_FUNCTIONAL_JOBS_TO_SCHEDULE")-1)) {
            max_functional_jobs_to_schedule=atoi(&s[sizeof("MAX_FUNCTIONAL_JOBS_TO_SCHEDULE=")-1]);
         } else if (!strncasecmp(s, "MAX_PENDING_TASKS_PER_JOB",
                    sizeof("MAX_PENDING_TASKS_PER_JOB")-1)) {
            max_pending_tasks_per_job=atoi(&s[sizeof("MAX_PENDING_TASKS_PER_JOB=")-1]);
         } else if (!strcasecmp(s, "HALFLIFE_DECAY_LIST=none")) {
            if (halflife_decay_list)
               lFreeList(halflife_decay_list);
            halflife_decay_list = NULL;
         } else if (!strncasecmp(s, "HALFLIFE_DECAY_LIST=",
                    sizeof("HALFLIFE_DECAY_LIST=")-1)) {
            lListElem *ep;
            const char *s0,*s1,*s2,*s3;
            double value;
            struct saved_vars_s *sv1=NULL, *sv2=NULL;
            if (halflife_decay_list) {
               lFreeList(halflife_decay_list);
               halflife_decay_list = NULL;
            }
            s0 = &s[sizeof("HALFLIFE_DECAY_LIST=")-1];
            for(s1=sge_strtok_r(s0, ":", &sv1); s1;
                s1=sge_strtok_r(NULL, ":", &sv1)) {
               if ((s2=sge_strtok_r(s1, "=", &sv2)) &&
                   (s3=sge_strtok_r(NULL, "=", &sv2)) &&
                   (sscanf(s3, "%lf", &value)==1)) {
                  ep = lAddElemStr(&halflife_decay_list, UA_name, s2, UA_Type);
                  lSetDouble(ep, UA_value, value);
               }
               if (sv2)
                  free(sv2);
            }
            if (sv1)
               free(sv1);
         } else if (!strncasecmp(s, "PROFILE=1", sizeof("PROFILE=1")-1)) {
            profile_schedd = 1;
         } else if (!strncasecmp(s, "POLICY_HIERARCHY=", 
                                 sizeof("POLICY_HIERARCHY=")-1)) {
            const char *value_string;

            value_string = &s[sizeof("POLICY_HIERARCHY=")-1];
            if (value_string) {
               policy_hierarchy_t hierarchy[POLICY_VALUES];

               if (policy_hierarchy_verify_value(value_string)) {
                  WARNING((SGE_EVENT, MSG_GDI_INVALIDPOLICYSTRING)); 
                  strcpy(policy_hierarchy_string, "");
               } else {
                  strcpy(policy_hierarchy_string, value_string);
               }
               policy_hierarchy_fill_array(hierarchy, policy_hierarchy_string);
               policy_hierarchy_print_array(hierarchy);
            } 
         }
      }
   }

   if (mlist) {
      lFreeList(mlist);
   }
 
   if (!global) {
      WARNING((SGE_EVENT, MSG_GDI_NOCONFIGFROMMASTER));
      DEXIT;
      return -2;
   }

   if (set_sge_environment == 0 && 
       set_cod_environment == 0 && 
       set_grd_environment == 0) {
      WARNING((SGE_EVENT, MSG_GDI_NEITHERSGECODGRDSETTINGSGE));
      set_sge_environment = 1;
   }
   
   DEXIT;
   return 0;
}

/*******************************************************************/
void sge_show_conf()
{
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_show_conf");

   DPRINTF(("conf.qmaster_spool_dir      >%s<\n", conf.qmaster_spool_dir));
   DPRINTF(("conf.execd_spool_dir        >%s<\n", conf.execd_spool_dir));
   DPRINTF(("conf.binary_path            >%s<\n", conf.binary_path));
   DPRINTF(("conf.mailer                 >%s<\n", conf.mailer));
   DPRINTF(("conf.prolog                 >%s<\n", conf.prolog));
   DPRINTF(("conf.epilog                 >%s<\n", conf.epilog));
   DPRINTF(("conf.shell_start_mode       >%s<\n", conf.shell_start_mode));
   DPRINTF(("conf.login_shells           >%s<\n", conf.login_shells));
   DPRINTF(("conf.administrator_mail     >%s<\n", conf.administrator_mail?conf.administrator_mail:"none"));
   DPRINTF(("conf.min_gid                >%u<\n", (unsigned) conf.min_gid));
   DPRINTF(("conf.min_uid                >%u<\n", (unsigned) conf.min_uid));
   DPRINTF(("conf.load_report_time       >%u<\n", (unsigned) conf.load_report_time));
   DPRINTF(("conf.stat_log_time          >%u<\n", (unsigned) conf.stat_log_time));
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
   DPRINTF(("conf.schedd_params          >%s<\n", conf.schedd_params?conf.schedd_params:"none"));
   DPRINTF(("conf.execd_params           >%s<\n", conf.execd_params?conf.execd_params:"none"));
   DPRINTF(("conf.gid_range              >%s<\n", conf.gid_range?conf.gid_range:"none")); 
   DPRINTF(("conf.admin_user             >%s<\n", conf.admin_user?conf.admin_user:"none"));
   DPRINTF(("conf.zombie_jobs            >%u<\n", (unsigned) conf.zombie_jobs));
   DPRINTF(("conf.qlogin_daemon          >%s<\n", conf.qlogin_daemon?conf.qlogin_daemon:"none"));
   DPRINTF(("conf.qlogin_command         >%s<\n", conf.qlogin_command?conf.qlogin_command:"none"));
   DPRINTF(("conf.rsh_daemon             >%s<\n", conf.rsh_daemon?conf.rsh_daemon:"none"));
   DPRINTF(("conf.rsh_command            >%s<\n", conf.rsh_command?conf.rsh_command:"none"));
   DPRINTF(("conf.rlogin_daemon          >%s<\n", conf.rlogin_daemon?conf.rlogin_daemon:"none"));
   DPRINTF(("conf.rlogin_command         >%s<\n", conf.rlogin_command?conf.rlogin_command:"none"));
   DPRINTF(("default_domain              >%s<\n", default_domain?default_domain:"none"));
   DPRINTF(("conf.reschedule_unknown     >%u<\n", (unsigned) conf.reschedule_unknown));
   DPRINTF(("ignore_fqdn                 >%d<\n", !fqdn_cmp));
   DPRINTF(("conf.max_aj_instances       >%u<\n", (unsigned) conf.max_aj_instances));
   DPRINTF(("conf.max_aj_tasks           >%u<\n", (unsigned) conf.max_aj_tasks));
   DPRINTF(("conf.max_u_jobs             >%u<\n", (unsigned) conf.max_u_jobs));
   

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
   FREE(conf->qmaster_spool_dir);
   FREE(conf->execd_spool_dir);
   FREE(conf->binary_path);
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
   FREE(conf->schedd_params);
   FREE(conf->execd_params);
   FREE(conf->gid_range);
   FREE(conf->admin_user);
   FREE(conf->qlogin_daemon);
   FREE(conf->qlogin_command);
   FREE(conf->rsh_daemon);
   FREE(conf->rsh_command);
   FREE(conf->rlogin_daemon);
   FREE(conf->rlogin_command);
   FREE(default_domain);
   
   memset(conf, 0, sizeof(sge_conf_type));

   return;
}


/*-------------------------------------------------------------------------*
 * NAME
 *   get_configuration - retrieves configuration from qmaster
 * PARAMETER
 *   config_name       - name of local configuration or "global",
 *                       name is being resolved before action
 *   gepp              - pointer to list element containing global
 *                       configuration, CONF_Type, should point to NULL
 *                       or otherwise will be freed
 *   lepp              - pointer to list element containing local configuration
 *                       by name given by config_name, can be NULL if global
 *                       configuration is requested, CONF_Type, should point
 *                       to NULL or otherwise will be freed
 * RETURN
 *   -1   NULL pointer received
 *   -2   error resolving host
 *   -3   invalid NULL pointer received for local configuration
 *   -4   request to qmaster failed
 *   -5   there is no global configuration
 *   -6   commproc already registered
 *   -7   no permission to get configuration
 * EXTERNAL
 *
 * DESCRIPTION
 *   retrieves a configuration from the qmaster. If the configuration
 *   "global" is requested, then this function requests only this one.
 *   If not, both the global configuration and the requested local
 *   configuration are retrieved.
 *   This function was introduced to make execution hosts independent
 *   of being able to mount the local_conf directory.
 *-------------------------------------------------------------------------*/
int get_configuration(
char *config_name,
lListElem **gepp,
lListElem **lepp 
) {
   lCondition *where;
   lEnumeration *what;
   lList *alp = NULL;
   lList *lp = NULL;
   int is_global_requested = 0;
   int ret;
   lListElem *hep = NULL;
   int success;
   static int already_logged = 0;
   u_long32 status;
   
   DENTER(TOP_LAYER, "get_configuration");

   if (!config_name || !gepp) {
      DEXIT;
      return -1;
   }

   if (*gepp) {
      lFreeElem(*gepp);
      *gepp = NULL;
   }
   if (lepp && *lepp) {
      lFreeElem(*lepp);
      *lepp = NULL;
   }

   if (!strcasecmp(config_name, "global")) {
      is_global_requested = 1;
   }
   else {
      hep = lCreateElem(EH_Type);
      lSetHost(hep, EH_name, config_name);

      ret = sge_resolve_host(hep, EH_name);
      if (ret) {
         DPRINTF(("get_configuration: error %d resolving host %s: %s\n", 
                  ret, config_name, cl_errstr(ret)));
         lFreeElem(hep);
         if (ret==COMMD_NACK_CONFLICT) {
            DEXIT;
            return -6;
         } else {
            ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, config_name));
            DEXIT;
            return -2;
         }
      }
      DPRINTF(("get_configuration: unique for %s: %s\n", config_name, lGetHost(hep, EH_name)));
   }

   if (!is_global_requested && !lepp) {
      ERROR((SGE_EVENT, MSG_NULLPOINTER));
      lFreeElem(hep);
      DEXIT;
      return -3;
   }

   if (is_global_requested) {
      /*
       * they might otherwise send global twice
       */
      where = lWhere("%T(%I c= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME);
      DPRINTF(("requesting global\n"));
   }
   else {
      where = lWhere("%T(%I c= %s || %I h= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME, CONF_hname,
                     lGetHost(hep, EH_name));
      DPRINTF(("requesting global and %s\n", lGetHost(hep, EH_name)));
   }
   what = lWhat("%T(ALL)", CONF_Type);
   alp = sge_gdi(SGE_CONFIG_LIST, SGE_GDI_GET, &lp, where, what);

   lFreeWhat(what);
   lFreeWhere(where);

   success = ((status= lGetUlong(lFirst(alp), AN_status)) == STATUS_OK);
   if (!success) {
      if (!already_logged) {
         ERROR((SGE_EVENT, MSG_CONF_GETCONF_S, lGetString(lFirst(alp), AN_text)));
         already_logged = 1;       
      }
                   
      lFreeList(alp);
      lFreeList(lp);
      lFreeElem(hep);
      DEXIT;
      return (status != STATUS_EDENIED2HOST)?-4:-7;
   }
   lFreeList(alp);

   if (lGetNumberOfElem(lp) > (2 - is_global_requested)) {
      WARNING((SGE_EVENT, MSG_CONF_REQCONF_II, 2 - is_global_requested, lGetNumberOfElem(lp)));
   }

   if (!(*gepp = lGetElemHost(lp, CONF_hname, SGE_GLOBAL_NAME))) {
      ERROR((SGE_EVENT, MSG_CONF_NOGLOBAL));
      lFreeList(lp);
      lFreeElem(hep);
      DEXIT;
      return -5;
   }
   lDechainElem(lp, *gepp);

   if (!is_global_requested) {
      if (!(*lepp = lGetElemHost(lp, CONF_hname, lGetHost(hep, EH_name)))) {
         if (*gepp) {
            WARNING((SGE_EVENT, MSG_CONF_NOLOCAL_S, lGetHost(hep, EH_name)));
         }
         lFreeList(lp);
         lFreeElem(hep);
         already_logged = 0;
         DEXIT;
         return 0;
      }
      lDechainElem(lp, *lepp);
   }
   
   lFreeElem(hep);
   lFreeList(lp);
   already_logged = 0;
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*/
int get_conf_and_daemonize(
tDaemonizeFunc dfunc,
lList **conf_list 
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   int ret;
   time_t now, last;

   DENTER(TOP_LAYER, "get_conf_and_daemonize");
   /*
    * for better performance retrieve 2 configurations
    * in one gdi call
    */
   DPRINTF(("qualified hostname: %s\n",  me.qualified_hostname));
   
   now = last = (time_t) sge_get_gmt();
   while ((ret = get_configuration(me.qualified_hostname, &global, &local))) {
      if (ret==-6 || ret==-7) {
         /* confict: COMMPROC ALREADY REGISTERED */
         DEXIT;
         return -1;
      }
      if (!me.daemonized) {
         ERROR((SGE_EVENT, MSG_CONF_NOCONFBG)); 
         if (!getenv("SGE_ND"))
            dfunc();
         else 
            WARNING((SGE_EVENT, MSG_CONF_NOCONFSLEEP)); 
         sleep(10);
      } 
      else {
         DTRACE;
         sleep(60);
         now = (time_t) sge_get_gmt();
         if (last > now)
            last=now;
         if (now - last > 1800) {
            last = now;
            ERROR((SGE_EVENT, MSG_CONF_NOCONFSTILL));
         }   
      }
   }            
   
   ret = merge_configuration(global, local, &conf, NULL);
   if (ret) {
      DPRINTF((
         "Error %d merging configuration \"%s\"\n", ret, me.qualified_hostname));
   }
   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
   */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);
   *conf_list = lFreeList(*conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);
   
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*
 * NAME
 *   get_merged_conf - requests new configuration set from master
 * RETURN
 *   -1      - could not get configuration from qmaster
 *   -2      - could not merge global and local configuration
 * EXTERNAL
 *
 *-------------------------------------------------------------------------*/
int get_merged_configuration(
lList **conf_list 
) {
   lListElem *global = NULL;
   lListElem *local = NULL;
   int ret;

   DENTER(TOP_LAYER, "get_merged_configuration");

   DPRINTF(("qualified hostname: %s\n",  me.qualified_hostname));
   ret = get_configuration(me.qualified_hostname, &global, &local);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOREADCONF_IS, ret, me.qualified_hostname));
      lFreeElem(global);
      lFreeElem(local);
      return -1;
   }

   ret = merge_configuration(global, local, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONF_NOMERGECONF_IS, ret, me.qualified_hostname));
      lFreeElem(global);
      lFreeElem(local);
      return -2;
   }
   /*
    * we don't keep all information, just the name and the version
    * the entries are freed
    */
   lSetList(global, CONF_entries, NULL);
   lSetList(local, CONF_entries, NULL);

   *conf_list = lFreeList(*conf_list);
   *conf_list = lCreateList("config list", CONF_Type);
   lAppendElem(*conf_list, global);
   lAppendElem(*conf_list, local);
   
   DEXIT;
   return 0;
}

/****** sge_conf/policy_hierarchy_enum2char() *********************************
*  NAME
*     policy_hierarchy_enum2char() -- Return policy char for an enum value 
*
*  SYNOPSIS
*     char policy_hierarchy_enum2char(policy_type_t value) 
*
*  FUNCTION
*     Returns the first letter of a policy name corresponding to the enum 
*     "value".
*
*  INPUTS
*     policy_type_t value - enum value 
*
*  RESULT
*     char - "O", "F", "S", "D"
******************************************************************************/
char policy_hierarchy_enum2char(policy_type_t value) 
{
   return policy_hierarchy_chars[value - 1];
}
 
/****** sge_conf/policy_hierarchy_char2enum() *********************************
*  NAME
*     policy_hierarchy_char2enum() -- Return enum value for a policy char
*
*  SYNOPSIS
*     policy_type_t policy_hierarchy_char2enum(char character) 
*
*  FUNCTION
*     This function returns a enum value for the first letter of a policy
*     name. 
*
*  INPUTS
*     char character - "O", "F", "S" or "D" 
*
*  RESULT
*     policy_type_t - enum value 
******************************************************************************/
policy_type_t policy_hierarchy_char2enum(char character)
{
   const char *pointer;
   policy_type_t ret;
   
   pointer = strchr(policy_hierarchy_chars, character);
   if (pointer != NULL) {
      ret = (pointer - policy_hierarchy_chars) + 1;
   } else {
      ret = INVALID_POLICY;
   }
   return ret;
}

/****** sge_conf/policy_hierarchy_verify_value() ******************************
*  NAME
*     policy_hierarchy_verify_value() -- verify a policy string 
*
*  SYNOPSIS
*     int policy_hierarchy_verify_value(const char* value) 
*
*  FUNCTION
*     The function tests whether the given policy string (value) is valid. 
*
*  INPUTS
*     const char* value - policy string 
*
*  RESULT
*     int - 0 -> OK
*           1 -> ERROR: one char is at least twice in "value"
*           2 -> ERROR: invalid char in "value"
*           3 -> ERROR: value == NULL
******************************************************************************/
int policy_hierarchy_verify_value(const char* value) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "policy_hierarchy_verify_value");

   if (value != NULL) {
      if (strcmp(value, "") && strcasecmp(value, "NONE")) {
         int is_contained[POLICY_VALUES]; 
         size_t i;

         for (i = 0; i < POLICY_VALUES; i++) {
            is_contained[i] = 0;
         }

         for (i = 0; i < strlen(value); i++) {
            char c = value[i];
            int index = policy_hierarchy_char2enum(c);

            if (is_contained[index]) {
               DPRINTF(("character \'%c\' is contained at least twice\n", c));
               ret = 1;
               break;
            } 

            is_contained[index] = 1;

            if (is_contained[INVALID_POLICY]) {
               DPRINTF(("Invalid character \'%c\'\n", c));
               ret = 2;
               break;
            }
         }
      }
   } else {
      ret = 3;
   }

   DEXIT;
   return ret;
}

/****** sge_conf/policy_hierarchy_fill_array() ********************************
*  NAME
*     policy_hierarchy_fill_array() -- fill the policy array 
*
*  SYNOPSIS
*     void policy_hierarchy_fill_array(policy_hierarchy_t array[], 
*                                      const char *value) 
*
*  FUNCTION
*     Fill the policy "array" according to the characters given by "value".
*
*     value == "FODS":
*        policy_hierarchy_t array[4] = {
*            {FUNCTIONAL_POLICY, 1},
*            {OVERRIDE_POLICY, 1},
*            {DEADLINE_POLICY, 1},
*            {SHARE_TREE_POLICY, 1}
*        };
*
*     value == "FS":
*        policy_hierarchy_t array[4] = {
*            {FUNCTIONAL_POLICY, 1},
*            {SHARE_TREE_POLICY, 1},
*            {OVERRIDE_POLICY, 0},
*            {DEADLINE_POLICY, 0}
*        };
*
*     value == "OFS":
*        policy_hierarchy_t hierarchy[4] = {
*            {OVERRIDE_POLICY, 1},
*            {FUNCTIONAL_POLICY, 1},
*            {SHARE_TREE_POLICY, 1},
*            {DEADLINE_POLICY, 0}
*        }; 
*
*     value == "NONE":
*        policy_hierarchy_t hierarchy[4] = {
*            {OVERRIDE_POLICY, 0},
*            {FUNCTIONAL_POLICY, 0},
*            {SHARE_TREE_POLICY, 0},
*            {DEADLINE_POLICY, 0}
*        }; 
*
*  INPUTS
*     policy_hierarchy_t array[] - array with at least POLICY_VALUES values 
*     const char* value          - "NONE" or any combination
*                                  of the first letters of the policy names 
*                                  (e.g. "OFSD")
*
*  RESULT
*     "array" will be modified 
******************************************************************************/
void policy_hierarchy_fill_array(policy_hierarchy_t array[], const char *value)
{
   int is_contained[POLICY_VALUES];
   int index = 0;
   size_t i;

   DENTER(TOP_LAYER, "policy_hierarchy_fill_array");

   for (i = 0; i < POLICY_VALUES; i++) {
      is_contained[i] = 0;
   }     
   if (value != NULL && strcmp(value, "") && strcasecmp(value, "NONE")) {
      for (i = 0; i < strlen(value); i++) {
         char c = value[i];
         int enum_value = policy_hierarchy_char2enum(c); 

         is_contained[enum_value] = 1;
         array[index].policy = enum_value;
         array[index].dependent = 1;
         index++;
      }
   }
   for (i = INVALID_POLICY + 1; i < LAST_POLICY_VALUE; i++) {
      if (!is_contained[i])  {
         array[index].policy = i;
         array[index].dependent = 0;
         index++;
      }
   }

   DEXIT;
}

/****** sge_conf/policy_hierarchy_print_array() *******************************
*  NAME
*     policy_hierarchy_print_array() -- print hierarchy array 
*
*  SYNOPSIS
*     void policy_hierarchy_print_array(policy_hierarchy_t array[]) 
*
*  FUNCTION
*     Print hierarchy array in the debug output 
*
*  INPUTS
*     policy_hierarchy_t array[] - array with at least POLICY_VALUES values 
******************************************************************************/
void policy_hierarchy_print_array(policy_hierarchy_t array[])
{
   int i;

   DENTER(TOP_LAYER, "policy_hierarchy_print_array");
   
   for (i = INVALID_POLICY + 1; i < LAST_POLICY_VALUE; i++) {
      char character = policy_hierarchy_enum2char(array[i-1].policy);
      
      DPRINTF(("policy: %c; dependent: %d\n", character, array[i-1].dependent));
   }   

   DEXIT;
}

