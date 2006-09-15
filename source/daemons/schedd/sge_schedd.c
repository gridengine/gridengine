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
#include <errno.h>

#include "sge_lock.h"
#include "sge_bootstrap.h"
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
#include "sge_spool.h"
#include "sge_hostname.h"
#include "sge_os.h"
#include "sge_profiling.h"
#include "sge_serf.h"
#include "sge_mt_init.h"
#include "sge_category.h"

#include "uti/sge_io.h"
#include "uti/sge_stdio.h"
#include "uti/sge_uidgid.h"
#include "uti/sge_unistd.h"

#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_schedd_conf.h"

#include "msg_common.h"
#include "msg_schedd.h"
#include "msg_daemons_common.h"

#ifdef TEST_GDI2
#include "sge_gdi_ctx.h"
#include "sge_event_client2.h"
#endif

/* number of current scheduling alorithm in above array */
int current_scheduler = 0; /* default scheduler */
int new_global_config = 0;

static void schedd_set_serf_log_file(void *context);
static void schedd_serf_record_func(u_long32 job_id, u_long32 ja_taskid, 
   const char *state, u_long32 start_time, u_long32 end_time, char level_char,
   const char *object_name, const char *name, double utilization);
static void schedd_serf_newline(u_long32 time);

static int sge_ck_qmaster(void *context, const char *former_master_host);
static int parse_cmdline_schedd(int argc, char **argv);
static void usage(FILE *fp);
static void schedd_exit_func(void **context, int i);
static int sge_setup_sge_schedd(void *context);
int daemonize_schedd(void *context);


/* array used to select from different scheduling alorithms */
sched_func_struct sched_funcs[] =
{ /*algorithm_config        name                event subscription         data preparation    -- calls --> scheduler_impl */
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
   const char *master_host = NULL;
   int ret;
   char* initial_qmaster_host = NULL;
   char* local_host = NULL;
   time_t next_prof_output = 0;
   bool done = false;
   int schedd_exit_state = 0;
   const char* progname = NULL;
   lListElem *event_client = NULL;
#ifdef TEST_GDI2   
   sge_gdi_ctx_class_t *ctx = NULL;
   sge_evc_class_t *evc = NULL;
   lList *alp = NULL;
#else
   void *ctx = NULL;
   void *evc = NULL;
#endif

   DENTER_MAIN(TOP_LAYER, "schedd");

#ifndef TEST_GDI2
   sge_mt_init();
#else
   sge_prof_setup();
#endif   

   /* set profiling parameters */
   prof_set_level_name(SGE_PROF_EVENTMASTER, NULL, NULL);
   prof_set_level_name(SGE_PROF_SPOOLING, NULL, NULL);
   prof_set_level_name(SGE_PROF_CUSTOM0, "scheduler", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM1, "pending ticket calculation", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM3, "job sorting", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM4, "job dispatching", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM5, "send orders", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM6, "scheduler event loop", NULL);
   prof_set_level_name(SGE_PROF_CUSTOM7, "copy lists", NULL);
   prof_set_level_name(SGE_PROF_SCHEDLIB4, NULL, NULL);

   /* we wish these functions be used for schedule entry recording */
   serf_init(schedd_serf_record_func, schedd_serf_newline);

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
   sge_sig_handler_in_main_loop = 0;
   sge_setup_sig_handlers(SCHEDD);

#ifdef TEST_GDI2
   if (sge_setup2(&ctx, SCHEDD, NULL) != AE_OK) {
      SGE_EXIT((void**)&ctx, 1);
   }
   ctx->set_exit_func(ctx, schedd_exit_func);
#else
   uti_state_set_exit_func(schedd_exit_func);
   if (sge_setup(SCHEDD, NULL) != 0) {
      /* sge_setup has already printed the error message */
      SGE_EXIT(NULL, 1);
   }
#endif   
   
   /* TODO: to remove the internal path_state_get_cell_root() dependency
   **       schedd_set_schedd_log_file(ctx)
   **       schedd_set_serf_log_file(ctx)
   **       have been added
   */
   schedd_set_schedd_log_file(ctx);
   schedd_set_serf_log_file(ctx);

   /* prepare daemonize */
   if (!getenv("SGE_ND")) {
      sge_daemonize_prepare(ctx);
   }

   if ((ret = sge_occupy_first_three()) >= 0) {
      CRITICAL((SGE_EVENT, MSG_FILE_REDIRECTFILEDESCRIPTORFAILED_I , ret));
      SGE_EXIT(NULL, 1);
   }

   lInit(nmv);

   /* parse scheduler command line arguments */
   parse_cmdline_schedd(argc, argv);

   /* setup communication (threads) and daemonize if qmaster is unreachable */
   sge_sig_handler_in_main_loop = 1;
   check_qmaster = (sge_setup_sge_schedd(ctx) != 0) ? true : false;

   /* prepare event client/mirror mechanism */
#ifdef TEST_GDI2   
   /* TODO: check if this works with daemonizing */
   if (false == sge_gdi2_evc_setup(&evc, ctx, EV_ID_SCHEDD, &alp)) {
      answer_list_output(&alp);
      SGE_EXIT((void**)&ctx, 1);
   }
#if 0   
   /* TODO: why does the scheduler not work if this is done here ??? */
   if (!evc->ec_register(evc, false, &alp)) {
      answer_list_output(&alp);
      SGE_EXIT(NULL, 1);
   }
#endif   
   sge_schedd_mirror_register(evc);
   event_client = evc->ec_get_event_client(evc);
#else   
   sge_schedd_mirror_register(NULL);
   event_client = ec_get_event_client();
#endif   

#ifdef TEST_GDI2
   master_host = ctx->get_master(ctx, false);
#else
   master_host = sge_get_master(0);
#endif   
   if ( (ret=cl_com_cached_gethostbyname((char*)master_host, &initial_qmaster_host, NULL,NULL,NULL)) != CL_RETVAL_OK) {
      CRITICAL((SGE_EVENT, cl_get_error_text(ret)));
      SGE_EXIT(NULL, 1);
   }
   if ( (ret=cl_com_gethostname(&local_host, NULL,NULL,NULL)) != CL_RETVAL_OK) {
      FREE(initial_qmaster_host); 
      CRITICAL((SGE_EVENT, cl_get_error_text(ret)));
      SGE_EXIT(NULL, 1);
   }

   if (cl_com_compare_hosts((char*)master_host,local_host) != CL_RETVAL_OK) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_STARTSCHEDONMASTERHOST_S , master_host));
      FREE(initial_qmaster_host); 
      SGE_EXIT(NULL, 1);
   }
   FREE(local_host);

   /* finalize daeamonize */
   if (!getenv("SGE_ND")) {
      sge_daemonize_finalize(ctx);
   }

   starting_up();
   sge_write_pid(SCHEDD_PID_FILE);

   cl_com_set_synchron_receive_timeout(cl_com_get_handle((char*)prognames[SCHEDD] ,0), 
                                       (int) (sconf_get_schedule_interval() * 2) );

   sge_sig_handler_in_main_loop = 1;

   while (done == false) {
      if (shut_me_down) {
         done = true;
      }   

      if (sge_get_com_error_flag(SCHEDD, SGE_COM_ACCESS_DENIED) == true) {
         schedd_exit_state = SGE_COM_ACCESS_DENIED;
         done = true;
      }

      if (sge_get_com_error_flag(SCHEDD, SGE_COM_ENDPOINT_NOT_UNIQUE) == true) {
         schedd_exit_state = SGE_COM_ENDPOINT_NOT_UNIQUE;
         done = true;
      }


      if (sge_sig_handler_sigpipe_received) {
         sge_sig_handler_sigpipe_received = 0;
         INFO((SGE_EVENT, "SIGPIPE received"));
      }

      /* event processing can trigger a re-registration, 
       * -> if qmaster goes down
       * -> on shutdown
       * -> the scheduling algorithm was changed
       * in this case do not start a scheduling run
       */
      if (done == false) {

         if (check_qmaster) {
            if ((ret = sge_ck_qmaster(ctx, initial_qmaster_host)) < 0) {
               FREE(initial_qmaster_host);
               CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTGOFURTHER ));
               SGE_EXIT(NULL, 1);
            } 
            
            if (ret > 0) {
               sleep(5);
               continue;
            }

            if (ret == 0) {
               check_qmaster = false;
            }
         }

         if(sge_mirror_process_events(evc, event_client) == SGE_EM_TIMEOUT) {
            check_qmaster = true;
            continue;
         }

#ifdef TEST_GDI2            
         if(evc->ec_need_new_registration(evc)) {
            check_qmaster = true;
            continue;
         }
#else
         /* we must re-initialize 'event_client' as it might have been 
            freed deep down in sge_mirror_process_events() */
         event_client = ec_get_event_client();

         if(ec_need_new_registration()) {
            check_qmaster = true;
            continue;
         }
#endif         

         /* got new config? */
         if (sconf_is_new_config()) {

            /* set actual syncron receive timeout */
            cl_com_set_synchron_receive_timeout( cl_com_get_handle((char*)progname ,0),
                                                 (int) (sconf_get_schedule_interval() * 2) );

            /* check profiling settings, if necessary, switch profiling on/off */
            if (sconf_get_profiling()) {
               prof_start(SGE_PROF_OTHER, NULL);
               prof_start(SGE_PROF_PACKING, NULL);
               prof_start(SGE_PROF_EVENTCLIENT, NULL);
               prof_start(SGE_PROF_MIRROR, NULL);
               prof_start(SGE_PROF_GDI, NULL);
               prof_start(SGE_PROF_HT_RESIZE, NULL);
               prof_start(SGE_PROF_CUSTOM0, NULL);
               prof_start(SGE_PROF_CUSTOM1, NULL);
               prof_start(SGE_PROF_CUSTOM3, NULL);
               prof_start(SGE_PROF_CUSTOM4, NULL);
               prof_start(SGE_PROF_CUSTOM5, NULL);
               prof_start(SGE_PROF_CUSTOM6, NULL);
               prof_start(SGE_PROF_CUSTOM7, NULL);
               prof_start(SGE_PROF_SCHEDLIB4, NULL);
            } 
            else {
               prof_stop(SGE_PROF_OTHER, NULL);
               prof_stop(SGE_PROF_PACKING, NULL);
               prof_stop(SGE_PROF_EVENTCLIENT, NULL);
               prof_stop(SGE_PROF_MIRROR, NULL);
               prof_stop(SGE_PROF_GDI, NULL);
               prof_stop(SGE_PROF_HT_RESIZE, NULL);
               prof_stop(SGE_PROF_CUSTOM0, NULL);
               prof_stop(SGE_PROF_CUSTOM1, NULL);
               prof_stop(SGE_PROF_CUSTOM3, NULL);
               prof_stop(SGE_PROF_CUSTOM4, NULL);
               prof_stop(SGE_PROF_CUSTOM5, NULL);
               prof_stop(SGE_PROF_CUSTOM6, NULL);
               prof_stop(SGE_PROF_CUSTOM7, NULL);
               prof_stop(SGE_PROF_SCHEDLIB4, NULL);
            }
         }
   
         sched_funcs[current_scheduler].event_func(evc);

         sconf_reset_new_config();
      }
      
      /* output profiling information */
      if (prof_is_active(SGE_PROF_CUSTOM0)) {
         time_t now = (time_t)sge_get_gmt();

         if (now > next_prof_output || done == true) {
            prof_output_info(SGE_PROF_ALL, false, "profiling summary:\n");
            next_prof_output = now + 60;
         }
      }
   }

   sge_mirror_shutdown(evc, &event_client);

   sge_teardown_lock_service();
   
   sge_prof_cleanup();

   FREE(initial_qmaster_host);

#ifdef TEST_GDI2
   sge_shutdown((void**)&ctx, schedd_exit_state);
#else
   sge_shutdown(NULL, schedd_exit_state);
#endif

   DRETURN(EXIT_SUCCESS);
}

/*************************************************************/
static void usage(FILE *fp) 
{
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "usage");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
   SGE_EXIT(NULL, 1);
   DRETURN_VOID;
}

/*************************************************************/
static void schedd_exit_func(void **context, int i) 
{
   DENTER(TOP_LAYER, "schedd_exit_func");
#ifdef TEST_GDI2
   sge_gdi2_shutdown(context);
#else
   sge_gdi_shutdown();
#endif   
   serf_exit();
   DRETURN_VOID;
}

/*--------------------------------------------------------------*/
static int parse_cmdline_schedd(int argc, char *argv[]) 
{
   DENTER(TOP_LAYER, "parse_cmdline_schedd");

   if (argc > 1) {
      usage(stderr);
   }   

   DRETURN(0);
}

/*----------------------------------------------------------------*
 * sge_ck_qmaster
 *
 * returns 
 *  0 everything ok
 *  1 failed but we should retry (also check_isalive() failed)
 * -1 error 
 *----------------------------------------------------------------*/
static int sge_ck_qmaster(void *context, const char *former_master_host)
{
   lList *alp, *lp = NULL;
   int success;
   lEnumeration *what;
   lCondition *where;
#ifdef TEST_GDI2
   sge_gdi_ctx_class_t *ctx = (sge_gdi_ctx_class_t *)context;
   const char *current_master = ctx->get_master(ctx, true);
   const char *qualified_hostname = ctx->get_qualified_hostname(ctx);
   const char *username = ctx->get_username(ctx);
#else
   const char *current_master = sge_get_master(1);
   const char *qualified_hostname = uti_state_get_qualified_hostname();
   const char *username = uti_state_get_user_name();
#endif   

   DENTER(TOP_LAYER, "sge_ck_qmaster");

   if (former_master_host && sge_hostcmp(current_master, former_master_host)) {
      ERROR((SGE_EVENT, MSG_QMASTERMOVEDEXITING_SS, former_master_host,
      current_master));
      DRETURN(-1);
   }

#ifdef TEST_GDI2
   if (ctx->is_alive(ctx) != CL_RETVAL_OK) {
      DPRINTF(("qmaster is not alive\n"));
      DRETURN(1);
   }
#else
   if (check_isalive(current_master) != CL_RETVAL_OK) {
      DPRINTF(("qmaster is not alive\n"));
      DRETURN(1);
   }
#endif

/*---------------------------------------------------------------*/
   DPRINTF(("Checking if user \"%s\" is manager\n", username));

   what = lWhat("%T(ALL)", MO_Type);
   where = lWhere("%T(%I == %s)",
                  MO_Type,
                  MO_name, username);
                  
#ifdef TEST_GDI2
   alp = ctx->gdi(ctx, SGE_MANAGER_LIST, SGE_GDI_GET, &lp, where, what);
#else
   alp = sge_gdi(SGE_MANAGER_LIST, SGE_GDI_GET, &lp, where, what);
#endif   
   
   lFreeWhere(&where);
   lFreeWhat(&what);

   success = (lGetUlong(lFirst(alp), AN_status) == STATUS_OK);
   if (!success) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      lFreeList(&alp);
      lFreeList(&lp);
      DRETURN(1);                 /* we failed getting get manager list */

   }
   lFreeList(&alp);

   if (success && (lp == NULL || (lGetNumberOfElem(lp) == 0))) {
      ERROR((SGE_EVENT, MSG_SCHEDD_USERXMUSTBEMANAGERFORSCHEDDULING_S,
             username));
      lFreeList(&lp);
      DRETURN(-1);
   }
   lFreeList(&lp);

   /*-------------------------------------------------------------------
    * ensure admin host privileges for host
    */
   DPRINTF(("Checking if host \"%s\" is admin host\n", qualified_hostname));

   what = lWhat("%T(ALL)", AH_Type);
   where = lWhere("%T(%I h= %s)",
                  AH_Type,
                  AH_name, qualified_hostname);
#ifdef TEST_GDI2
   alp = ctx->gdi(ctx, SGE_ADMINHOST_LIST, SGE_GDI_GET, &lp, where, what);
#else
   alp = sge_gdi(SGE_ADMINHOST_LIST, SGE_GDI_GET, &lp, where, what);
#endif   
   lFreeWhere(&where);
   lFreeWhat(&what);

   success = (lGetUlong(lFirst(alp), AN_status) == STATUS_OK);
   if (!success) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      lFreeList(&alp);
      lFreeList(&lp);
      DRETURN(1);                 /* we failed getting admin host list */
   }

   lFreeList(&alp);

   if (success && (lp == NULL || (lGetNumberOfElem(lp) == 0))) {
      ERROR((SGE_EVENT, MSG_SCHEDD_HOSTXMUSTBEADMINHOSTFORSCHEDDULING_S ,
             qualified_hostname));
      DRETURN(-1);
   }
   lFreeList(&lp);

   DRETURN(0);
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
   int (*event_func_before)(void *) = sched_funcs[current_scheduler].event_func;

   DENTER(TOP_LAYER, "use_alg");

   if (alg_name) {
      for (i = 0; sched_funcs[i].name; i++) {
         if (!strcmp(sched_funcs[i].name, alg_name)) {
            current_scheduler = i;
            if (scheduler_before == current_scheduler) {
               DRETURN(0); 
            }
            if (event_func_before == sched_funcs[current_scheduler].event_func) {
               WARNING((SGE_EVENT, "Switching to scheduler \"%s\". No change with event handler\n", alg_name));
               DPRINTF(("scheduler changed but event handler function stays valid\n"));
               DRETURN(1); 
            }
            WARNING((SGE_EVENT, "Switching to event handler scheduler \"%s\"\n", alg_name));
            DRETURN(2);
         }
      }
   }

   ERROR((SGE_EVENT, MSG_SCHEDD_CANTINSTALLALGORITHMXUSINGDEFAULT_S 
          , alg_name ? alg_name : MSG_SCHEDD_UNKNOWN));

   use_alg("default");

   DRETURN((scheduler_before != current_scheduler)?1:0);
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
static int sge_setup_sge_schedd(void *context)
{
   int ret;
   u_long32 saved_logginglevel;
   char err_str[1024];
   lList *schedd_config_list = NULL;


#ifdef TEST_GDI2
   sge_gdi_ctx_class_t *ctx = (sge_gdi_ctx_class_t *)context;
   const char *admin_user = ctx->get_admin_user(ctx);
   const char *qmaster_spool_dir = ctx->get_qmaster_spool_dir(ctx);
#else
   int last_prepare_enroll_error = CL_RETVAL_OK;
   const char *admin_user = bootstrap_get_admin_user();
   const char *qmaster_spool_dir = bootstrap_get_qmaster_spool_dir();
   const char *qualified_hostname = uti_state_get_qualified_hostname();
   const char *progname = uti_state_get_sge_formal_prog_name();
   u_long32 progid = uti_state_get_mewho();
   const char *cell_root = path_state_get_cell_root();
   int is_daemonized = uti_state_get_daemonized();
#endif   

   DENTER(TOP_LAYER, "sge_setup_sge_schedd");


   /*
   ** switch to admin user
   */
   if (sge_set_admin_username(admin_user, err_str)) {
      CRITICAL((SGE_EVENT, err_str));
      SGE_EXIT(NULL, 1);
   }

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSWITCHTOADMINUSER ));
      SGE_EXIT(NULL, 1);
   }

#ifdef TEST_GDI2
   ret = ctx->prepare_enroll(ctx);
   ret = gdi2_get_conf_and_daemonize(ctx, daemonize_schedd, &schedd_config_list, &shut_me_down);
   switch(ret) {
      case 0:
         break;
      case -2:
         INFO((SGE_EVENT, MSG_SCHEDD_SCHEDD_ABORT_BY_USER));
         SGE_EXIT(NULL, 1);
         break;
      default:
         CRITICAL((SGE_EVENT, MSG_SCHEDD_ALREADY_RUNNING));
         SGE_EXIT(NULL, 1);
   }

   sge_show_conf();

   lFreeList(&schedd_config_list);

#else
   /*
    * setup communication as admin user
    */
   prepare_enroll(prognames[SCHEDD], &last_prepare_enroll_error );

   /* get_conf_and_daemonize() will come back for access denied or endpoint not unique
    * erros */
   ret = get_conf_and_daemonize(progid, progname, qualified_hostname, cell_root, is_daemonized,
                                 daemonize_schedd, &schedd_config_list, &shut_me_down);
   switch(ret) {
      case 0:
         break;
      case -2:
         INFO((SGE_EVENT, MSG_SCHEDD_SCHEDD_ABORT_BY_USER));
         SGE_EXIT(NULL, 1);
         break;
      default:
         CRITICAL((SGE_EVENT, MSG_SCHEDD_ALREADY_RUNNING));
         SGE_EXIT(NULL, 1);
   }

   sge_show_conf();

   lFreeList(&schedd_config_list);
   
   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();
#endif

   sge_chdir_exit(qmaster_spool_dir, 1);
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
   if ((ret = sge_ck_qmaster(context, NULL)) < 0) {
      CRITICAL((SGE_EVENT, MSG_SCHEDD_CANTSTARTUP ));
      SGE_EXIT(NULL, 1);
   }
   log_state_set_log_level(saved_logginglevel);

   DRETURN((ret == 0 ? 0 : 1));
}

/*-------------------------------------------------------------------*/
int daemonize_schedd(void *context)
{
   int ret = 0;
   DENTER(TOP_LAYER, "daemonize_schedd");

   ret = sge_daemonize_finalize(context);

   DRETURN(ret);
}


/* do everything that needs to be done in common for all schedulers 
   between processing events and dispatching */
int sge_before_dispatch(void *evc_context)
{
#ifdef TEST_GDI2
   sge_evc_class_t *evc = (sge_evc_class_t *)evc_context;
   sge_gdi_ctx_class_t *ctx = evc->get_gdi_ctx(evc);
   const char *cell_root = ctx->get_cell_root(ctx);
   u_long32 progid = ctx->get_who(ctx);
#else
   const char *cell_root = path_state_get_cell_root();
   u_long32 progid = uti_state_get_mewho();
#endif   
   
   DENTER(TOP_LAYER, "sge_before_dispatch");

   /* hostname resolving scheme in global config could have changed
      get it and use it if we got a notification about a new global config */
   if (new_global_config) {
      lListElem *global = NULL, *local = NULL;

#ifdef TEST_GDI2
      if (gdi2_get_configuration(ctx, SGE_GLOBAL_NAME, &global, &local) == 0) {
         merge_configuration(progid, cell_root, global, local, NULL);
      }   
#else
      if (get_configuration(SGE_GLOBAL_NAME, &global, &local) == 0) {
         merge_configuration(progid, cell_root, global, local, NULL);
      }   
#endif      
      lFreeElem(&global);
      lFreeElem(&local);
      new_global_config = 0;
   }
   
   if (sconf_is_new_config()) {
#ifndef TEST_GDI2   
      lListElem *event_client = ec_get_event_client();
#endif      
      int interval = sconf_get_flush_finish_sec();
      bool flush = (interval > 0) ? true : false;
      interval--;
#ifdef TEST_GDI2      
      if (evc->ec_get_flush(evc, sgeE_JOB_DEL) != interval) {
         evc->ec_set_flush(evc, sgeE_JOB_DEL,flush, interval);
         evc->ec_set_flush(evc, sgeE_JOB_FINAL_USAGE,flush, interval);
         evc->ec_set_flush(evc, sgeE_JATASK_MOD, flush, interval);
         evc->ec_set_flush(evc, sgeE_JATASK_DEL, flush, interval);
      }
#else      
      if (ec_get_flush(event_client, sgeE_JOB_DEL) != interval) {
         ec_set_flush(event_client, sgeE_JOB_DEL,flush, interval);
         ec_set_flush(event_client, sgeE_JOB_FINAL_USAGE,flush, interval);
         ec_set_flush(event_client, sgeE_JATASK_MOD, flush, interval);
         ec_set_flush(event_client, sgeE_JATASK_DEL, flush, interval);
      }
#endif      

      interval= sconf_get_flush_submit_sec();
      flush = (interval > 0) ? true : false;
      interval--;      
#ifdef TEST_GDI2      
      if(evc->ec_get_flush(evc, sgeE_JOB_ADD) != interval) {
         evc->ec_set_flush(evc, sgeE_JOB_ADD, flush, interval);
      }
      evc->ec_commit(evc, NULL);
#else
      if(ec_get_flush(event_client, sgeE_JOB_ADD) != interval) {
         ec_set_flush(event_client, sgeE_JOB_ADD, flush, interval);
      }
      ec_commit(event_client);
#endif      
   }

   /*
    * job categories are reset here, we need 
    *  - an update of the rejected field for every new run
    *  - the resource request dependent urgency contribution is cached 
    *    per job category 
    */
   sge_reset_job_category(); 
   
   DRETURN(0);
}

void sge_schedd_mirror_register(void *evc_context)
{
   /* register as event mirror */
#ifdef TEST_GDI2
   sge_evc_class_t *evc = (sge_evc_class_t *)evc_context;
   sge_mirror_initialize(evc, EV_ID_SCHEDD, "scheduler", true);
   evc->ec_set_busy_handling(evc, EV_BUSY_UNTIL_RELEASED);
#else
   lListElem *event_client = NULL;
   sge_mirror_initialize(evc_context, EV_ID_SCHEDD, "scheduler", true);
   event_client = ec_get_event_client();
   ec_set_busy_handling(event_client, EV_BUSY_UNTIL_RELEASED);
#endif   

   /* subscribe events */
   sched_funcs[current_scheduler].subscribe_func(evc_context);
}

/* sge_schedd's current schedule entry recording facility (poor mans realization) */

static char schedule_log_path[SGE_PATH_MAX + 1] = "";
const char *schedule_log_file = "schedule";

static void schedd_set_serf_log_file(void *context)
{
#ifdef TEST_GDI2
   sge_gdi_ctx_class_t *ctx = (sge_gdi_ctx_class_t*) context;
   const char *cell_root = ctx->get_cell_root(ctx);
#else
   const char *cell_root = path_state_get_cell_root();
#endif 

   DENTER(TOP_LAYER, "set_schedd_serf_log_path");

   if (!*schedule_log_path) {
      snprintf(schedule_log_path, sizeof(schedule_log_path), "%s/%s/%s", cell_root, "common", schedule_log_file);
      DPRINTF(("schedule log path >>%s<<\n", schedule_log_path));
   }

   DRETURN_VOID;
}   

/* MT-NOTE: schedd_serf_record_func() is not MT safe */
static void 
schedd_serf_record_func(u_long32 job_id, u_long32 ja_taskid, const char *state,
                        u_long32 start_time, u_long32 end_time, 
                        char level_char, const char *object_name, 
                        const char *name, double utilization)
{
   FILE *fp;

   DENTER(TOP_LAYER, "schedd_serf_record_func");

   if (!(fp = fopen(schedule_log_path, "a"))) {
      DRETURN_VOID;
   }

   /* a new record */
   fprintf(fp, sge_U32CFormat":"sge_U32CFormat":%s:"sge_U32CFormat":"
           sge_U32CFormat":%c:%s:%s:%f\n", sge_u32c(job_id), 
           sge_u32c(ja_taskid), state, sge_u32c(start_time), 
           sge_u32c(end_time - start_time), 
           level_char, object_name, name, utilization);
   FCLOSE(fp);

   DRETURN_VOID;
FCLOSE_ERROR:
   DPRINTF((MSG_FILE_ERRORCLOSEINGXY_SS, schedule_log_path, strerror(errno)));
   DRETURN_VOID;
}

/* MT-NOTE: schedd_serf_newline() is not MT safe */
static void schedd_serf_newline(u_long32 time)
{
   FILE *fp;

   DENTER(TOP_LAYER, "schedd_serf_newline");
   fp = fopen(schedule_log_path, "a");
   if (fp) {
      /* well, some kind of new line indicating a new schedule run */
      fprintf(fp, "::::::::\n");
      FCLOSE(fp);
   }
   DRETURN_VOID;
FCLOSE_ERROR:
   DPRINTF((MSG_FILE_ERRORCLOSEINGXY_SS, schedule_log_path, strerror(errno)));
   DRETURN_VOID;
}

