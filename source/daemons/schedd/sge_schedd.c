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
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "sge.h"
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_c_event.h"
#include "sge_chdir.h"
#include "sge_copy_append.h"
#include "sge_daemonize.h"
#include "sge_exit.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_log_pid.h"
#include "sge_me.h"
#include "sge_mkdir.h"
#include "sge_orders.h"
#include "sge_prognames.h"
#include "sge_schedd.h"
#include "sgermon.h"
#include "commlib.h"
#include "complex.h"
#include "scheduler.h"
#include "sge_feature.h"
#include "shutdown.h"
#include "sge_sched.h"
#include "schedd_monitor.h"
#include "sig_handlers.h"
#include "utility.h"
#include "sge_conf.h"
#include "sge_process_events.h"
#include "sge_switch_user.h"
#include "basis_types.h"
#include "qm_name.h"
#include "msg_schedd.h"
#include "msg_daemons_common.h"
#include "sge_language.h"
#include "sge_string.h"
#include "setup_path.h" 
#include "sge_time.h" 
#include "job_log.h" 


/* number of current scheduling alorithm in above array */
int current_scheduler = 0; /* default scheduler */
int new_global_config = 0;
int start_on_master_host = 0;

static int sge_ck_qmaster(void);
static int parse_cmdline_schedd(int argc, char **argv);
static void usage(FILE *fp);
static void schedd_exit_func(int i);
static int sge_setup_sge_schedd(void);
int daemonize_schedd(void);

extern char *error_file;


/* array used to select from different scheduling alorithms */
sched_func_struct sched_funcs[] =
{
   {"default",      "Default scheduler",   event_handler_default_scheduler, (void *)scheduler },
#ifdef SCHEDULER_SAMPLES
   {"ext_mysched",  "sample #1 scheduler", event_handler_default_scheduler, (void *)my_scheduler },
   {"ext_mysched2", "sample #2 scheduler", event_handler_my_scheduler,      (void *)scheduler },
#endif
   {NULL, NULL}
};


int main(int argc, char *argv[]);

/*-------------------------------------------------------------------------*/
int main(
int argc,
char *argv[] 
) {
   int check_qmaster;
   char *master_host;
   int ret;

   DENTER_MAIN(TOP_LAYER, "schedd");

   /* This needs a better solution */
   umask(022);

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);  
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */



   /* Initialize path for temporary logging until we chdir to spool */
   error_file = TMP_ERR_FILE_SCHEDD;

   /* exit func for SGE_EXIT() */
   in_main_loop = 0;
   install_exit_func(schedd_exit_func);
   sge_setup_sig_handlers(SCHEDD);

   sge_setup(SCHEDD, NULL);
   prepare_enroll(prognames[SCHEDD], 1, NULL);

   if ((ret = occupy_first_three()) >= 0) {
      CRITICAL((SGE_EVENT, MSG_FILE_REDIRECTFILEDESCRIPTORFAILED_I , ret));
      SGE_EXIT(1);
   }

   lInit(nmv);

   feature_initialize_from_file(path.product_mode_file);
   parse_cmdline_schedd(argc, argv);

   /* daemonizes if qmaster is unreachable */
   check_qmaster = sge_setup_sge_schedd();

   master_host = sge_get_master(0);
   if (hostcmp(master_host, me.qualified_hostname) && start_on_master_host) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_STARTSCHEDONMASTERHOST_S , master_host));
      SGE_EXIT(1);
   }

   if (!getenv("SGE_ND")) {
      int fd;
      fd_set fds;
      FD_ZERO(&fds);
      if ((fd=get_commlib_state_sfd())>=0) {
         FD_SET(fd, &fds);
      }
      sge_daemonize(get_commlib_state_closefd()?NULL:&fds);
   }

   sge_log_pid(SCHEDD_PID_FILE);

#if RAND_ERROR
   rand_error = 1;
#endif

   in_main_loop = 1;

   /* this is necessary if the master has to send LOTS of data
    * to the schedd. would timout otherwise.
   */
   set_commlib_param(CL_P_TIMEOUT_SRCV, 4*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 4*60, NULL, NULL);

   while (1) {
      lList* event_list = NULL;

      if (shut_me_down)
         sge_shutdown();

      if (sigpipe_received) {
         sigpipe_received = 0;
         INFO((SGE_EVENT, "SIGPIPE received"));
      }
         
      if (check_qmaster) {
         if ((ret = sge_ck_qmaster()) < 0) {
            CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTGOFURTHER ));
            SGE_EXIT(1);
         }
         else if (ret > 0) {
            sleep(10);
            continue;
         }
      }

      ret = ec_get(&event_list);

      {
         static u_long32 last_list = 0;
         if (!ret)
            last_list = sge_get_gmt();
         else {
            if (last_list && (sge_get_gmt() > last_list + ec_get_edtime() * 10)) {
               DPRINTF(("QMASTER ALIVE TIMEOUT EXPIRED\n"));
               ec_mark4registration();
            }
            if (ret == 1)
               check_qmaster = 0;
            else
               check_qmaster = 1;
            continue;
         }
      }

      /* pass event list to event handler of current scheduler 
         the event handler also starts a dispatch epoch 

         a change on the scheduler algorithm is also sent as an event,
         and must be recognized by the current scheduler. It must lead 
         to a new registration of schedd with qmaster as the new 
         scheduler needs the initial events */

      ret = sched_funcs[current_scheduler].event_func(event_list);

      if (ret) {
         ec_mark4registration();
         check_qmaster = 1;
         if (ret == -1) {              /* error in event layer */
            WARNING((SGE_EVENT, MSG_SCHEDD_REREGISTER_ERROR));
         } else if (ret == 1) {        /* schedd parameter changed */
            INFO((SGE_EVENT, MSG_SCHEDD_REREGISTER_PARAM));
         } else /* (ret == 2) */ {     /* shutdown order from qmaster */           
            INFO((SGE_EVENT, MSG_SHADOWD_CONTROLLEDSHUTDOWN));
         }
      }
   }
}

/*************************************************************/
static void usage(
FILE *fp 
) {
   DENTER(TOP_LAYER, "usage");

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION));

   fprintf(fp, "%s schedd [options]\n", MSG_SCHEDD_USAGE );
   fprintf(fp, "  [-help]  %s", MSG_SCHEDD_help_OPT_USAGE);
   fprintf(fp, "  [-salg]  %s", MSG_SCHEDD_salg_OPT_USAGE);
   fprintf(fp, "  [-k]     %s", MSG_SCHEDD_k_OPT_USAGE);

   SGE_EXIT(1);
}

/*************************************************************/
static void schedd_exit_func(
int i 
) {
   DENTER(TOP_LAYER, "schedd_exit_func");
   leave_commd();
   DEXIT;
}

/*--------------------------------------------------------------*/
static int parse_cmdline_schedd(
int argc,
char *argv[] 
) {
   int ret = 0, i;

   DENTER(TOP_LAYER, "parse_cmdline_execd");

   if (argc > 1) {

      while (*(++argv)) {

         if (!strcmp("-help", *argv)) {
            usage(stdout);
         }

         if (!strcmp("-salg", *argv)) {
            /* we will not daemonize - printing is possible */
            printf(MSG_SCHEDD_AVAILABLESCHEDDALGORITHMS );
#define HFORMAT "   %6s%16s   %s\n"
#define DFORMAT "   %6d%16s   %s\n"
            printf(HFORMAT, MSG_SCHEDD_ALGNR, MSG_SCHEDD_NAME, MSG_SCHEDD_DESCRIPTION);
            printf("   --------------------------------------------------------------------\n");
            i = 0;
            while (sched_funcs[i].name) {
               printf(DFORMAT, i, sched_funcs[i].name, sched_funcs[i].descr);
               i++;
            }
#undef HFORMAT
#undef DFORMAT
            printf("\n");
            /* Let's go */
            exit(0);
         }

         /* -lj */
         if (!strcmp("-lj", *argv)) {
            enable_job_logging(*++argv);
            continue;
         }

         if (!strcmp("-k", *argv)) {
            if ((i = read_pid(SCHED_PID_FILE)) > 0) {
               kill(i, SIGTERM);
               printf(MSG_SCHEDD_HALTINGEXISTINGSCHEDDPID_I , i);
            }
            else {
               fprintf(stderr, MSG_PROC_CANTREADPIDFILE_S , SCHED_PID_FILE);
               exit(1);
            }
            exit(0);
         }
      }
   }

   DEXIT;
   return ret;
}

/*----------------------------------------------------------------*
 * sge_ck_qmaster
 *
 * returns 
 *  0 everything ok
 *  1 failed but we should retry (also check_isalive() failed)
 * -1 error 
 *----------------------------------------------------------------*/
static int sge_ck_qmaster()
{
   lList *alp, *lp = NULL;
   int success, old_timeout;
   lEnumeration *what;
   lCondition *where;

   DENTER(TOP_LAYER, "sge_ck_qmaster");

   if (check_isalive(sge_get_master(0))) {
      DPRINTF(("qmaster is not alive\n"));
      DEXIT;
      return 1;
   }

/*---------------------------------------------------------------*/
   DPRINTF(("Checking if user \"%s\" is manager\n", me.user_name));

   what = lWhat("%T(ALL)", MO_Type);
   where = lWhere("%T(%I == %s)",
                  MO_Type,
                  MO_name, me.user_name);
                  
   old_timeout = get_commlib_state_timeout_ssnd();
   set_commlib_param(CL_P_TIMEOUT_SRCV, 20, NULL, NULL);
                        
   alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_GET, &lp, where, what);
   set_commlib_param(CL_P_TIMEOUT_SRCV, old_timeout, NULL, NULL);
   
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
             me.user_name));
      lp = lFreeList(lp);
      DEXIT;
      return -1;
   }
   lp = lFreeList(lp);

   /*-------------------------------------------------------------------
    * ensure admin host privileges for host
    */
   DPRINTF(("Checking if host \"%s\" is admin host\n", me.qualified_hostname));

   what = lWhat("%T(ALL)", AH_Type);
   where = lWhere("%T(%I h= %s)",
                  AH_Type,
                  AH_name, me.qualified_hostname);
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
             me.qualified_hostname));
      DEXIT;
      return -1;
   }
   lp = lFreeList(lp);

/*---------------------------------------------------------------*/
   DPRINTF(("Requesting scheduler configuration from qmaster\n"));

   what = lWhat("%T(ALL)", SC_Type);
   alp = sge_gdi(SGE_SC_LIST, SGE_GDI_GET, &lp, NULL, what);
   what = lFreeWhat(what);

   success = (lGetUlong(lFirst(alp), AN_status) == STATUS_OK);
   if (!success) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      DEXIT;
      return 1;
   }

   alp = lFreeList(alp);

   if (success && !lp) {
      INFO((SGE_EVENT, MSG_SCHEDD_GOTEMPTYCONFIGFROMMASTERUSINGDEFAULT ));
      lp = lFreeList(lp);
      DEXIT;
      return 1;
   }

   alp = NULL;
   if (sc_set(&alp, &scheddconf, lFirst(lp), NULL))
      ERROR((SGE_EVENT, "%s", lGetString(lFirst(alp), AN_text)));

   ec_set_edtime(scheddconf.schedule_interval);
   use_alg(scheddconf.algorithm);

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
char *alg_name 
) {
   int i = 0;
   int scheduler_before = current_scheduler;
   int (*event_func_before)(lList *) = sched_funcs[current_scheduler].event_func;

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
   extern u_long32 logginglevel;
   u_long32 saved_logginglevel = logginglevel;
   char err_str[1024];

   DENTER(TOP_LAYER, "sge_setup_sge_schedd");

   if (get_conf_and_daemonize(daemonize_schedd, &schedd_config_list)) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_ALRADY_RUNNING));
      SGE_EXIT(1);
   }
   sge_show_conf();

   /*
   ** switch to admin user
   */
   if (set_admin_username(conf.admin_user, err_str)) {
      CRITICAL((SGE_EVENT, err_str));
      SGE_EXIT(1);
   }

   if (switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSWITCHTOADMINUSER ));
      SGE_EXIT(1);
   }

   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();

   sge_chdir(conf.qmaster_spool_dir, 1);
   sge_mkdir(SCHED_SPOOL_DIR, 0755, 1);
   sge_chdir(SCHED_SPOOL_DIR, 1);

   /* having passed this statement we may log messages into the ERR_FILE */
   sge_copy_append(TMP_ERR_FILE_SCHEDD, ERR_FILE, SGE_APPEND);
   switch2start_user();
   unlink(TMP_ERR_FILE_SCHEDD);
   switch2admin_user();
   error_file = ERR_FILE;
   sge_log_as_admin_user();

   /* suppress the INFO messages during setup phase */
   logginglevel = LOG_WARNING;
   if ((ret = sge_ck_qmaster()) < 0) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSTARTUP ));
      SGE_EXIT(1);
   }
   logginglevel = saved_logginglevel;

   DEXIT;
   return (ret == 0 ? 0 : 1);
}

/*-------------------------------------------------------------------*/
int daemonize_schedd()
{
   fd_set keep_open;
   int ret, fd;

   DENTER(TOP_LAYER, "daemonize_schedd");

   FD_ZERO(&keep_open);
   if ((fd = get_commlib_state_sfd()) >= 0) {
      FD_SET(fd, &keep_open);
   }
   sge_daemonize(get_commlib_state_closefd()?NULL:&keep_open);
   ret = sge_daemonize(&keep_open);

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
      lListElem *global = NULL, *local = NULL;

      if (get_configuration(SGE_GLOBAL_NAME, &global, &local) == 0)
         merge_configuration(global, local, &conf, NULL);
      lFreeElem(global);
      lFreeElem(local);
      new_global_config = 0;
   }

   DEXIT;
   return 0;
}

/* handling administrative events is independent of the scheduler algorithm */
/*
*                                                             max. column:     |
*/
/****** sge_schedd/handle_administrative_events() ******
*  NAME
*     handle_administrative_events() -- ??? 
*
*  SYNOPSIS
*     int handle_administrative_events(u_long32 type, lListElem *event) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     u_long32 type    - ??? 
*     lListElem *event - ??? 
*
*  RESULT
*    0 this event was not an administrative event
*   -1 reregister at qmaster
*    1 handled administrative event
*    2 got a shutdown event: do not schedule, finish immediately instead
*
********************************
*/
int handle_administrative_events(u_long32 type, lListElem *event)
{
   int ret = 1;

   DENTER(TOP_LAYER, "handle_administrative_events");

   switch (type) {

      /* ======================================================
       * administrative events  
       */

   case sgeE_SCHEDDDOWN:
      INFO((SGE_EVENT, MSG_EVENT_GOTSHUTDOWNFROMQMASTER ));
      shut_me_down = 1;
      ret = 2;
      break;

   case sgeE_QMASTER_GOES_DOWN:
      INFO((SGE_EVENT, MSG_EVENT_GOTMASTERGOESDOWNMESSAGEFROMQMASTER ));
      if (start_on_master_host)
         shut_me_down = 1;
      else {
         sleep(8);
         ec_mark4registration();
         ret = -1;
      }
      break;

   case sgeE_SCHEDDMONITOR:
      monitor_next_run = 1;
      DPRINTF(("monitoring next scheduler run"));
      break;

   case sgeE_GLOBAL_CONFIG:
      DPRINTF(("notification about new global configuration\n"));
      new_global_config = 1;
      break;

   default:
      /* non-administrative events - 
         these must be handled by caller of this func */
      ret = 0;
      break;
   }

   DEXIT;
   return ret;
}
