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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "sge_bootstrap.h"
#include "sge_unistd.h"
#include "sge.h"
#include "setup.h"
#include "sge_all_listsL.h"
#include "sge_event_client.h"
#include "sge_any_request.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_orders.h"
#include "sge_prog.h"
#include "sge_schedd.h"
#include "sgermon.h"
#include "commlib.h"
#include "scheduler.h"
#include "sge_feature.h"
#include "shutdown.h"
#include "sge_sched.h"
#include "schedd_monitor.h"
#include "sig_handlers.h"
#include "sge_conf.h"
#include "gdi_conf.h"
#include "sge_process_events.h"
#include "basis_types.h"
#include "qm_name.h"
#include "msg_schedd.h"
#include "msg_daemons_common.h"
#include "sge_language.h"
#include "sge_string.h"
#include "setup_path.h" 
#include "sge_time.h" 
#include "job_log.h" 
#include "sge_uidgid.h"
#include "sge_io.h"
#include "sge_spool.h"
#include "sge_hostname.h"
#include "sge_os.h"
#include "sge_answer.h"
#include "sge_profiling.h"
#include "sge_mt_init.h"


/* number of current scheduling alorithm in above array */
int current_scheduler = 0; /* default scheduler */
int new_global_config = 0;
int start_on_master_host = 0;
int sgeee_mode = 0;

static int sge_ck_qmaster(const char *former_master_host);
static int parse_cmdline_schedd(int argc, char **argv);
static void usage(FILE *fp);
static void schedd_exit_func(int i);
static int sge_setup_sge_schedd(void);
int daemonize_schedd(void);


/* array used to select from different scheduling alorithms */
sched_func_struct sched_funcs[] =
{
   {"default",      "Default scheduler",   subscribe_default_scheduler, event_handler_default_scheduler, (void *)scheduler },
#ifdef SCHEDULER_SAMPLES
   {"ext_mysched",  "sample #1 scheduler", subscribe_default_scheduler, event_handler_default_scheduler, (void *)my_scheduler },
   {"ext_mysched2", "sample #2 scheduler", subscribe_my_scheduler,      event_handler_my_scheduler,      (void *)scheduler },
#endif
   {NULL, NULL, NULL, NULL}
};


int main(int argc, char *argv[]);

/*-------------------------------------------------------------------------*/
int main(
int argc,
char *argv[] 
) {
   bool check_qmaster;
   const char *master_host;
   int ret;
   char initial_qmaster_host[1024];
   time_t next_prof_output = 0;
   bool done = false;

   DENTER_MAIN(TOP_LAYER, "schedd");

   sge_mt_init();

   /* set profiling parameters */
   prof_set_level_name(SGE_PROF_EVENTMASTER, NULL, NULL);
   prof_set_level_name(SGE_PROF_SPOOLING, NULL, NULL);
   prof_set_level_name(SGE_PROF_CUSTOM0, "scheduler", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM1, "pending ticket calculation", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM2, "active job ticket calculation", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM3, "job sorting", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM4, "job dispatching", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM5, "send orders", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM6, "scheduler event loop", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM7, "copy lists", NULL);


   /* This needs a better solution */
   umask(022);

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);  
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   /* Initialize path for temporary logging until we chdir to spool */
   log_state_set_log_file(TMP_ERR_FILE_SCHEDD);

   /* exit func for SGE_EXIT() */
   in_main_loop = 0;
   uti_state_set_exit_func(schedd_exit_func);
   sge_setup_sig_handlers(SCHEDD);

   sge_setup(SCHEDD, NULL);
   prepare_enroll(prognames[SCHEDD], 1, NULL);

   if ((ret = sge_occupy_first_three()) >= 0) {
      CRITICAL((SGE_EVENT, MSG_FILE_REDIRECTFILEDESCRIPTORFAILED_I , ret));
      SGE_EXIT(1);
   }

   lInit(nmv);

   sgeee_mode = feature_is_enabled(FEATURE_SGEEE);
   parse_cmdline_schedd(argc, argv);

   /* daemonizes if qmaster is unreachable */
   check_qmaster = sge_setup_sge_schedd();

   /* prepare event client/mirror mechanism */
   sge_schedd_mirror_register();

   master_host = sge_get_master(0);
   if (sge_hostcmp(master_host, uti_state_get_qualified_hostname()) && start_on_master_host) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_STARTSCHEDONMASTERHOST_S , master_host));
      SGE_EXIT(1);
   }
   strncpy(initial_qmaster_host, master_host, sizeof(initial_qmaster_host)-1);

   if (!getenv("SGE_ND")) {
      fd_set fds;
#ifdef ENABLE_NGC
      FD_ZERO(&fds);
      if ( cl_com_set_handle_fds(cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0), &fds) == CL_RETVAL_OK) {
         INFO((SGE_EVENT, "there are open file descriptors for communication\n"));
         sge_daemonize(&fds);
      } else {
         sge_daemonize(NULL);
      }
#else
      FD_ZERO(&fds);
      if ((fd=commlib_state_get_sfd())>=0) {
         FD_SET(fd, &fds);
      }
      sge_daemonize(commlib_state_get_closefd()?NULL:&fds);
#endif
   }

   starting_up();
   sge_write_pid(SCHEDD_PID_FILE);

#if RAND_ERROR
   rand_error = 1;
#endif

   in_main_loop = 1;

   /* this is necessary if the master has to send LOTS of data
    * to the schedd. would timout otherwise.
   */
#ifdef ENABLE_NGC
#else
   set_commlib_param(CL_P_TIMEOUT_SRCV, 4*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 4*60, NULL, NULL);
#endif

   while (!done) {
      if (shut_me_down) {
         sge_mirror_shutdown();
         sge_shutdown();
      }   

      if (sigpipe_received) {
         sigpipe_received = 0;
         INFO((SGE_EVENT, "SIGPIPE received"));
      }
      
      if (check_qmaster) {
         if ((ret = sge_ck_qmaster(initial_qmaster_host)) < 0) {
            CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTGOFURTHER ));
            SGE_EXIT(1);
         } else if (ret > 0) {
            sleep(10);
            continue;
         }
      }

      if(sge_mirror_process_events() == SGE_EM_TIMEOUT) {
         check_qmaster = true;
         continue;
      }

      /* event processing can trigger a re-registration, 
       * -> if qmaster goes down
       * -> the scheduling algorithm was changed
       * in this case do not start a scheduling run
       */
      if(ec_need_new_registration()) {
         check_qmaster = true;
         continue;
      }

      sched_funcs[current_scheduler].event_func();

      /* output profiling information */
      if (prof_is_active()) {
         time_t now = sge_get_gmt();

         if (now > next_prof_output || shut_me_down) {
            prof_output_info(SGE_PROF_ALL, false, "profiling summary:\n");
            next_prof_output = now + 60;
         }
      }
   }
   DEXIT;
   return EXIT_SUCCESS;
}

/*************************************************************/
static void usage(FILE *fp) 
{
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "usage");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
   SGE_EXIT(1);
   DEXIT;
}

/*************************************************************/
static void schedd_exit_func(int i) 
{
   DENTER(TOP_LAYER, "schedd_exit_func");
   sge_gdi_shutdown();
   DEXIT;
}

/*--------------------------------------------------------------*/
static int parse_cmdline_schedd(int argc, char *argv[]) 
{
   DENTER(TOP_LAYER, "parse_cmdline_schedd");

   if (argc > 1) {
      usage(stderr);
   }   

   DEXIT;
   return 0;
}

/*----------------------------------------------------------------*
 * sge_ck_qmaster
 *
 * returns 
 *  0 everything ok
 *  1 failed but we should retry (also check_isalive() failed)
 * -1 error 
 *----------------------------------------------------------------*/
static int sge_ck_qmaster(const char *former_master_host)
{
   lList *alp, *lp = NULL;
   int success;
   lEnumeration *what;
   lCondition *where;
   const char *current_master;

   DENTER(TOP_LAYER, "sge_ck_qmaster");

   current_master = sge_get_master(1);
   if (former_master_host && sge_hostcmp(current_master, former_master_host)) {
      ERROR((SGE_EVENT, MSG_QMASTERMOVEDEXITING_SS, former_master_host,
      current_master));
      DEXIT;
      return -1;
   }

#ifdef ENABLE_NGC
   if (check_isalive(current_master) != CL_RETVAL_OK) {
      DPRINTF(("qmaster is not alive\n"));
      DEXIT;
      return 1;
   }
#else
   if (check_isalive(current_master)) {
      DPRINTF(("qmaster is not alive\n"));
      DEXIT;
      return 1;
   }
#endif

/*---------------------------------------------------------------*/
   DPRINTF(("Checking if user \"%s\" is manager\n", uti_state_get_user_name()));

   what = lWhat("%T(ALL)", MO_Type);
   where = lWhere("%T(%I == %s)",
                  MO_Type,
                  MO_name, uti_state_get_user_name());
                  
#ifdef ENABLE_NGC
#else
   old_timeout = commlib_state_get_timeout_ssnd();
   set_commlib_param(CL_P_TIMEOUT_SRCV, 20, NULL, NULL);
#endif
                        
   alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_GET, &lp, where, what);
#ifdef ENABLE_NGC
#else
   set_commlib_param(CL_P_TIMEOUT_SRCV, old_timeout, NULL, NULL);
#endif
   
   where = lFreeWhere(where);
   what = lFreeWhat(what);

   success = (lGetUlong(lFirst(alp), AN_status) == STATUS_OK);
   if (!success) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      alp = lFreeList(alp);
      lp = lFreeList(lp);
      DEXIT;
      return 1;                 /* we failed getting get manager list */

   }
   alp = lFreeList(alp);

   if (success && !lp) {
      ERROR((SGE_EVENT, MSG_SCHEDD_USERXMUSTBEMANAGERFORSCHEDDULING_S ,
             uti_state_get_user_name()));
      lp = lFreeList(lp);
      DEXIT;
      return -1;
   }
   lp = lFreeList(lp);

   /*-------------------------------------------------------------------
    * ensure admin host privileges for host
    */
   DPRINTF(("Checking if host \"%s\" is admin host\n", uti_state_get_qualified_hostname()));

   what = lWhat("%T(ALL)", AH_Type);
   where = lWhere("%T(%I h= %s)",
                  AH_Type,
                  AH_name, uti_state_get_qualified_hostname());
   alp = sge_gdi(SGE_ADMINHOST_LIST, SGE_GDI_GET, &lp, where, what);
   where = lFreeWhere(where);
   what = lFreeWhat(what);

   success = (lGetUlong(lFirst(alp), AN_status) == STATUS_OK);
   if (!success) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      alp = lFreeList(alp);
      lp = lFreeList(lp);
      DEXIT;
      return 1;                 /* we failed getting admin host list */
   }

   alp = lFreeList(alp);

   if (success && !lp) {
      ERROR((SGE_EVENT, MSG_SCHEDD_HOSTXMUSTBEADMINHOSTFORSCHEDDULING_S ,
             uti_state_get_qualified_hostname()));
      DEXIT;
      return -1;
   }
   lp = lFreeList(lp);

   DEXIT;
   return 0;
}

/*----------------------------------------------------------------
  returns
     0 if nothing changed
     1 if only sched_func_struct.alg function changed
     2 if sched_func_struct.event_func function changed 
*/
int use_alg(
const char *alg_name 
) {
   int i = 0;
   int scheduler_before = current_scheduler;
   int (*event_func_before)(void) = sched_funcs[current_scheduler].event_func;

   DENTER(TOP_LAYER, "use_alg");

   if (alg_name) {
      for (i = 0; sched_funcs[i].name; i++) {
         if (!strcmp(sched_funcs[i].name, alg_name)) {
            current_scheduler = i;
            if (scheduler_before == current_scheduler) {
               DEXIT;
               return 0; 
            }
            if (event_func_before == sched_funcs[current_scheduler].event_func) {
               WARNING((SGE_EVENT, "Switching to scheduler \"%s\". No change with event handler\n", alg_name));
               DPRINTF(("scheduler changed but event handler function stays valid\n"));
               DEXIT;
               return 1; 
            }
            WARNING((SGE_EVENT, "Switching to event handler scheduler \"%s\"\n", alg_name));
            DEXIT;
            return 2;
         }
      }
   }

   ERROR((SGE_EVENT, MSG_SCHEDD_CANTINSTALLALGORITHMXUSINGDEFAULT_S 
          , alg_name ? alg_name : MSG_SCHEDD_UNKNOWN));

   use_alg("default");

   DEXIT;
   return (scheduler_before != current_scheduler)?1:0;
}

/*-------------------------------------------------------------------
 * sge_setup_sge_schedd
 *    This function will not return until we sucessfully got the
 *    cluster configuration in get_conf_and_daemonize()
 *    If we can't get the cluster configuration, we go into background
 *    The checking of our priveleges will be repeated in the main loop,
 *    if there is a communication problem
 *
 * return: exits if sge_ck_qmaster() returns == -1
 *         1     if sge_ck_qmaster() returns ==  1
 *         0     if setup was completely ok
 *-------------------------------------------------------------------*/
static int sge_setup_sge_schedd()
{
   int ret;
   u_long32 saved_logginglevel;
   char err_str[1024];
   lList *schedd_config_list = NULL;


   DENTER(TOP_LAYER, "sge_setup_sge_schedd");

   if (get_conf_and_daemonize(daemonize_schedd, &schedd_config_list)) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ALRADY_RUNNING));
      SGE_EXIT(1);
   }
   sge_show_conf();

   schedd_config_list = lFreeList(schedd_config_list);
   
   /*
   ** switch to admin user
   */
   if (sge_set_admin_username(bootstrap_get_admin_user(), err_str)) {
      CRITICAL((SGE_EVENT, err_str));
      SGE_EXIT(1);
   }

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSWITCHTOADMINUSER ));
      SGE_EXIT(1);
   }

   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();

   sge_chdir_exit(bootstrap_get_qmaster_spool_dir(), 1);
   sge_mkdir(SCHED_SPOOL_DIR, 0755, 1, 0);
   sge_chdir_exit(SCHED_SPOOL_DIR, 1);

   /* having passed this statement we may log messages into the ERR_FILE */
   sge_copy_append(TMP_ERR_FILE_SCHEDD, ERR_FILE, SGE_MODE_APPEND);
   sge_switch2start_user();
   unlink(TMP_ERR_FILE_SCHEDD);
   sge_switch2admin_user();
   log_state_set_log_as_admin_user(1);
   log_state_set_log_file(ERR_FILE);
   /* suppress the INFO messages during setup phase */
   saved_logginglevel = log_state_get_log_level();
   log_state_set_log_level(LOG_WARNING);
   if ((ret = sge_ck_qmaster(NULL)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSTARTUP ));
      SGE_EXIT(1);
   }
   log_state_set_log_level(saved_logginglevel);

   DEXIT;
   return (ret == 0 ? 0 : 1);
}

/*-------------------------------------------------------------------*/
int daemonize_schedd()
{
   fd_set keep_open;
   int ret = 0;

   DENTER(TOP_LAYER, "daemonize_schedd");

   FD_ZERO(&keep_open);
#ifdef ENABLE_NGC
   if ( cl_com_set_handle_fds(cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name(),0), &keep_open) == CL_RETVAL_OK) {
      INFO((SGE_EVENT, "there are open file descriptors for communication\n"));
      sge_daemonize(&keep_open);
   } else {
      sge_daemonize(NULL);
   }
#else
   if ((fd = commlib_state_get_sfd()) >= 0) {
      FD_SET(fd, &keep_open);
   }
   sge_daemonize(commlib_state_get_closefd()?NULL:&keep_open);
   ret = sge_daemonize(&keep_open);
#endif

   DEXIT;
   return ret;
}


/* do everything that needs to be done in common for all schedulers 
   between processing events and dispatching */
int sge_before_dispatch(void)
{
   DENTER(TOP_LAYER, "sge_before_dispatch");

   /* hostname resolving scheme in global config could have changed
      get it and use it if we got a notification about a new global config */
   if (new_global_config) {
/*   
      lListElem *global = NULL, *local = NULL;

      if (get_configuration(SGE_GLOBAL_NAME, &global, &local) == 0)
         merge_configuration(global, local, &conf, NULL);
      lFreeElem(global);
      lFreeElem(local);
      new_global_config = 0;
*/
      /* flushing information might have changed */
      /* SG: TODO: is this still needed? */
      {
         int interval = sconf_get_flush_finish_sec();
         bool flush = interval> 0;
         if (interval== 0)
            interval= -1;
         if(ec_get_flush(sgeE_JOB_DEL) != interval) {
            ec_set_flush(sgeE_JOB_DEL,flush, interval);
            ec_set_flush(sgeE_JOB_FINAL_USAGE,flush, interval);
            ec_set_flush(sgeE_JATASK_MOD, flush, interval);
            ec_set_flush(sgeE_JATASK_DEL, flush, interval);
         }

         interval= sconf_get_flush_submit_sec();
         flush = interval> 0;
         if (interval== 0)
            interval= -1;
         if(ec_get_flush(sgeE_JOB_ADD) != interval) {
            ec_set_flush(sgeE_JOB_ADD, flush, interval);
         }
      }
      ec_commit();
   }

   DEXIT;
   return 0;
}

void sge_schedd_mirror_register()
{
   /* register as event mirror */
   sge_mirror_initialize(EV_ID_SCHEDD, "scheduler");
   ec_set_busy_handling(EV_BUSY_UNTIL_RELEASED);
   ec_set_clientdata(-1);

   /* subscribe events */
   sched_funcs[current_scheduler].subscribe_func();
}
