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
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#include "sge_unistd.h"
#include "sge.h"
#include "sge_gdi.h"

#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_load_sensor.h"
#include "sge_log.h"
#include "sge_prog.h"
#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "sge_conf.h"
#include "dispatcher.h"
#include "execd_ck_to_do.h"
#include "execd_get_new_conf.h"
#include "execd_job_exec.h"
#include "execd_kill_execd.h"
#include "execd_signal_queue.h"
#include "execd_ticket.h"
#include "job_report_execd.h"
#include "sge_report_execd.h"
#include "load_avg.h"
#include "parse.h"
#include "sge_feature.h"
#include "reaper_execd.h"
#include "setup_execd.h"
#include "shutdown.h"
#include "sig_handlers.h"
#include "startprog.h"
#include "usage.h"
#include "spool/classic/read_write_job.h"
#include "sge_os.h"
#include "sge_stdlib.h"
#include "sge_spool.h"
#include "sge_answer.h"
#include "basis_types.h"
#include "sge_language.h"
#include "sge_job.h"
#include "sge_mt_init.h"
#include "sge_uidgid.h"
#include "sge_profiling.h"
#include "execd.h"
#include "qm_name.h"
#include "sgeobj/sge_object.h"
#include "uti/sge_bootstrap.h"


#include "msg_common.h"
#include "msg_execd.h"
#include "msg_gdilib.h"

#include "uti/sge_monitor.h"

#ifdef COMPILE_DC
#   include "ptf.h"
#   include "sgedefs.h"
#endif





volatile int jobs_to_start = 1;

/* only used when running as SGE execd */
volatile int waiting4osjid = 1;

/* Store the directory the execd runs in when in normal operation.
 * avoid calling getcwd, cause this catches zombies on sun and is a !?GRML call!
 */
char execd_spool_dir[SGE_PATH_MAX];

static void execd_exit_func(void **ctx, int i);
static void execd_register(sge_gdi_ctx_class_t *ctx);
static void dispatcher_errfunc(const char *err_str);
static void parse_cmdline_execd(char **argv);
static lList *sge_parse_cmdline_execd(char **argv, lList **ppcmdline);
static lList *sge_parse_execd(lList **ppcmdline, lList **ppreflist, u_long32 *help);

/* DISPATCHER TABLE FOR EXECD */
dispatch_entry execd_dispatcher_table[] = {
   { TAG_JOB_EXECUTION, NULL, NULL, 0, execd_job_exec },
   { TAG_SLAVE_ALLOW,   NULL, NULL, 0, execd_job_slave },
   { TAG_CHANGE_TICKET, NULL, NULL, 0, execd_ticket },
   { TAG_ACK_REQUEST,   NULL, NULL, 0, execd_c_ack }, 
   { TAG_SIGJOB,        NULL, NULL, 0, execd_signal_queue },
   { TAG_SIGQUEUE,      NULL, NULL, 0, execd_signal_queue },
   { TAG_KILL_EXECD,    NULL, NULL, 0, execd_kill_execd  },
/*    { TAG_NEW_FEATURES,  NULL, NULL, 0, execd_new_features }, */
   { TAG_GET_NEW_CONF,  NULL, NULL, 0, execd_get_new_conf },
   { -1,                NULL, NULL, 0, execd_ck_to_do}
};


/* time execd maximal waits in the dispatch routine */
#define DISPATCH_TIMEOUT_SGE     2

int main(int argc, char *argv[]);

/****** execd/sge_execd_application_status() ***********************************
*  NAME
*     sge_execd_application_status() -- commlib status callback function
*
*  SYNOPSIS
*     unsigned long sge_execd_application_status(char** info_message) 
*
*  FUNCTION
*      This is the implementation of the commlib application status callback
*      function. This function is called from the commlib when a connected
*      client wants to get a SIRM (Status Information Response Message).
*      The standard client for this action is the qping command.
*
*      The callback function is set with cl_com_set_status_func() after
*      commlib initalization.
*
*      The function is called by a commlib function which may not run in the
*      context of the execd application. This means no execd specific
*      functions should be called (e.g. locking of global variables).
*
*      status 0:  no errors
*      status 1:  dispatcher has reached warning timeout
*      status 2:  dispatcher has reached error timeout
*      status 3:  dispatcher alive timeout struct not initalized
*
*  INPUTS
*     char** info_message - pointer to an char* inside commlib.
*                           info message must be malloced, commlib will
*                           free this memory. 
*  RESULT
*     unsigned long status - status of application
*
*  NOTES
*     This function is MT save
*******************************************************************************/
unsigned long sge_execd_application_status(char** info_message) 
{
   return sge_monitor_status(info_message, 0);
}

/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int i, dispatch_timeout;
   char err_str[1024];
   int my_pid;
   int ret_val;
   int printed_points = 0;
   int max_enroll_tries;
   static char tmp_err_file_name[SGE_PATH_MAX];
   time_t next_prof_output = 0;
   int execd_exit_state = 0;
   lList **master_job_list = NULL;
   const char* qualified_hostname = NULL;
   const char* binary_path = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;
   lList *alp = NULL;

   DENTER_MAIN(TOP_LAYER, "execd");

   sge_prof_setup();
   set_thread_name(pthread_self(),"Execd Thread");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   /* This needs a better solution */
   umask(022);
      
   /* Initialize path for temporary logging until we chdir to spool */
   my_pid = getpid();
   sprintf(tmp_err_file_name,"%s."sge_U32CFormat"",TMP_ERR_FILE_EXECD,sge_u32c(my_pid));
   log_state_set_log_file(tmp_err_file_name);

   /* exit func for SGE_EXIT() */
   sge_sig_handler_in_main_loop = 0;
   sge_setup_sig_handlers(EXECD);

   if (sge_setup2(&ctx, EXECD, &alp) != AE_OK) {
      answer_list_output(&alp);
      SGE_EXIT((void**)&ctx, 1);
   }
   ctx->set_exit_func(ctx, execd_exit_func);
   qualified_hostname = ctx->get_qualified_hostname(ctx);
   binary_path = ctx->get_binary_path(ctx);

   if ((i=sge_occupy_first_three())>=0) {
      CRITICAL((SGE_EVENT, MSG_FILE_REDIRECTFD_I, i));
      SGE_EXIT((void**)&ctx, 1);
   }     
   lInit(nmv);

   /* unset XAUTHORITY if set */
   if (getenv("XAUTHORITY") != NULL) {
      sge_unsetenv("XAUTHORITY");
   }

   parse_cmdline_execd(argv);   
   
   /* exit if we can't get communication handle (bind port) */
   max_enroll_tries = 30;
   while (cl_com_get_handle(prognames[EXECD],1) == NULL) {
      ctx->prepare_enroll(ctx);
      max_enroll_tries--;
      if ( max_enroll_tries <= 0 || shut_me_down ) {
         /* exit after 30 seconds */
         if (printed_points != 0) {
            printf("\n");
         }
         CRITICAL((SGE_EVENT, MSG_COM_ERROR));
         SGE_EXIT((void**)&ctx, 1);
      }
      if (cl_com_get_handle(prognames[EXECD],1) == NULL) {
        /* sleep when prepare_enroll() failed */
        sleep(1);
        if (max_enroll_tries < 27) {
           printf(".");
           printed_points++;
           fflush(stdout);
        }
      }
   }

   if (printed_points != 0) {
      printf("\n");
   }

   /*
    * now the commlib up and running. Set execd application status function 
    * ( commlib callback function for qping status information response 
    *   messages (SIRM) )
    */
   ret_val = cl_com_set_status_func(sge_execd_application_status);
   if (ret_val != CL_RETVAL_OK) {
      ERROR((SGE_EVENT, cl_get_error_text(ret_val)) );
   }

   /* daemonizes if qmaster is unreachable */   
   sge_setup_sge_execd(ctx, tmp_err_file_name);

   if (!getenv("SGE_ND")) {
      daemonize_execd(ctx);
   }


   /* are we using qidle or not */
   sge_ls_qidle(mconf_get_use_qidle());
   sge_ls_gnu_ls(1);
   
   DPRINTF(("use_qidle: %d\n", mconf_get_use_qidle()));

   /* test load sensor (internal or external) */
   {
      lList *report_list = sge_build_load_report(qualified_hostname, binary_path);
      lFreeList(&report_list);
   }

   execd_register(ctx);

   sge_write_pid(EXECD_PID_FILE);

   /* at this point we are sure we are the only sge_execd */
   /* first we have to report any reaped children that might exist */

   starting_up();
   
   /*
   ** log a warning message if execd hasn't been started by a superuser
   */
   if (!sge_is_start_user_superuser()) {
      WARNING((SGE_EVENT, MSG_SWITCH_USER_NOT_ROOT));
   }   

#ifdef COMPILE_DC
   if (ptf_init()) {
      CRITICAL((SGE_EVENT, MSG_EXECD_NOSTARTPTF));
      SGE_EXIT((void**)&ctx, 1);
   }
   INFO((SGE_EVENT, MSG_EXECD_STARTPDCANDPTF));
#endif

   master_job_list = object_type_get_master_list(SGE_TYPE_JOB);
   *master_job_list = lCreateList("Master_Job_List", JB_Type);
   job_list_read_from_disk(master_job_list, "Master_Job_List",
                           0, SPOOL_WITHIN_EXECD, 
                          job_initialize_job);
   
   /* clean up jobs hanging around (look in active_dir) */
   clean_up_old_jobs(1);
   sge_send_all_reports(ctx, 0, NUM_REP_REPORT_JOB, execd_report_sources);

   dispatch_timeout = DISPATCH_TIMEOUT_SGE;
      
   sge_sig_handler_in_main_loop = 1;

   /***** MAIN LOOP *****/
   while (shut_me_down != 1) {

     if (thread_prof_active_by_id(pthread_self())) {
         prof_start(SGE_PROF_CUSTOM1, NULL);
         prof_set_level_name(SGE_PROF_CUSTOM1, "Execd Thread", NULL); 
         prof_start(SGE_PROF_CUSTOM2, NULL);
         prof_set_level_name(SGE_PROF_CUSTOM2, "Execd Dispatch", NULL); 
         prof_start(SGE_PROF_GDI_REQUEST, NULL);
      } else {
           prof_stop(SGE_PROF_CUSTOM1, NULL);
           prof_stop(SGE_PROF_CUSTOM2, NULL);
           prof_stop(SGE_PROF_GDI_REQUEST, NULL);
        }

      PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);

      i = dispatch(ctx, execd_dispatcher_table, 
                   sizeof(execd_dispatcher_table)/sizeof(dispatch_entry),
                   dispatch_timeout, err_str, dispatcher_errfunc);

      if (sge_sig_handler_sigpipe_received) {
          sge_sig_handler_sigpipe_received = 0;
          INFO((SGE_EVENT, "SIGPIPE received\n"));
      }

      if (sge_get_com_error_flag(EXECD, SGE_COM_ACCESS_DENIED) == true) {
         execd_exit_state = SGE_COM_ACCESS_DENIED;
         break; /* shut down, leave while */
      }

      if (sge_get_com_error_flag(EXECD, SGE_COM_ENDPOINT_NOT_UNIQUE) == true) {
         execd_exit_state = SGE_COM_ENDPOINT_NOT_UNIQUE;
         break; /* shut down, leave while */
      }

      if (i) {
         if (cl_is_commlib_error(i)) {
            if (i != CL_RETVAL_OK) {
               execd_register(ctx); /* reregister at qmaster */
            }
         } else {
            WARNING((SGE_EVENT, MSG_COM_RECEIVEREQUEST_S, err_str ));
         }
      }
   }

   lFreeList(master_job_list);
  
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);

   if (prof_is_active(SGE_PROF_ALL)) {
     time_t now = (time_t)sge_get_gmt();

      if (now > next_prof_output) {
         prof_output_info(SGE_PROF_ALL, false, "profiling summary:\n");
         prof_reset(SGE_PROF_ALL,NULL);
         next_prof_output = now + 60;
      }
   }   

   sge_prof_cleanup();
   sge_shutdown((void**)&ctx, execd_exit_state);
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------
 * Function installed to be called just before exit() is called.
 * clean up
 *-------------------------------------------------------------*/
static void execd_exit_func(
void **ctx_ref,
int i 
) {
   DENTER(TOP_LAYER, "execd_exit_func");

   sge_gdi2_shutdown(ctx_ref);

   /* trigger load sensors shutdown */
   sge_ls_stop(0);

#ifdef COMPILE_DC
   ptf_stop();
#endif

   DEXIT;
}

/*-------------------------------------------------------------
 * dispatcher_errfunc
 *
 * function called by dispatcher on non terminal errors 
 *-------------------------------------------------------------*/
static void dispatcher_errfunc(
const char *err_str 
) {
   DENTER(TOP_LAYER, "dispatcher_errfunc");
   ERROR((SGE_EVENT, "%s", err_str));
   DEXIT;
}

/****** execd/sge_execd_register_at_qmaster() **********************************
*  NAME
*     sge_execd_register_at_qmaster() -- modify execd list at qmaster site
*
*  SYNOPSIS
*     int sge_execd_register_at_qmaster(void) 
*
*  FUNCTION
*     add local execd name to SGE_EXECHOST_LIST in order to register at
*     qmaster
*
*  INPUTS
*     void - no input
*
*  RESULT
*     int - 0 = success / 1 = error
*
*  NOTES
*     MT-NOTE: sge_execd_register_at_qmaster() is not MT safe 
*
*  SEE ALSO
*     execd/execd_register()
*******************************************************************************/
int sge_execd_register_at_qmaster(sge_gdi_ctx_class_t *ctx) {
   int return_value = 0;
   static int sge_last_register_error_flag = 0;
   lList *hlp = NULL, *alp = NULL;
   lListElem *aep = NULL;
   lListElem *hep = NULL;
   const char *master_host = ctx->get_master(ctx, false);
   
   DENTER(TOP_LAYER, "sge_execd_register_at_qmaster");

   hlp = lCreateList("exechost starting", EH_Type);
   hep = lCreateElem(EH_Type);
   lSetUlong(hep, EH_featureset_id, feature_get_active_featureset_id());
   lAppendElem(hlp, hep);

   /* register at qmaster */
   DPRINTF(("*****  Register at qmaster   *****\n"));
   alp = ctx->gdi(ctx, SGE_EXECHOST_LIST, SGE_GDI_ADD, &hlp, NULL, NULL);
   aep = lFirst(alp);
   if (!alp || (lGetUlong(aep, AN_status) != STATUS_OK)) {
      if (sge_last_register_error_flag == 0) {
         WARNING((SGE_EVENT, MSG_COM_CANTREGISTER_S, aep?lGetString(aep, AN_text):MSG_COM_ERROR));
         sge_last_register_error_flag = 1;
      }
      return_value = 1;
   } else {
      sge_last_register_error_flag = 0;
      INFO((SGE_EVENT, MSG_EXECD_REGISTERED_AT_QMASTER_S, master_host));
   }
   lFreeList(&alp);
   lFreeList(&hlp);
   DEXIT;
   return return_value;
}

/*-------------------------------------------------------------
 * execd_register
 *
 * Function for registering the execd at qmaster
 *-------------------------------------------------------------*/
/****** execd/execd_register() *************************************************
*  NAME
*     execd_register() -- register at qmaster on startup
*
*  SYNOPSIS
*     static void execd_register() 
*
*  FUNCTION
*     This function will block until execd is registered at qmaster
*
*  NOTES
*     MT-NOTE: execd_register() is not MT safe 
*
*  SEE ALSO
*     execd/sge_execd_register_at_qmaster()
*******************************************************************************/
static void execd_register(sge_gdi_ctx_class_t *ctx)
{
   int had_problems = 0;

   DENTER(TOP_LAYER, "execd_register");

   while (!shut_me_down) {
      DPRINTF(("*****Checking In With qmaster*****\n"));

      if (had_problems != 0) {
         int ret_val;
         cl_com_handle_t* handle = NULL;
         
         handle = cl_com_get_handle(prognames[EXECD],1);
         if ( handle == NULL) {
            DPRINTF(("preparing reenroll"));
            ctx->prepare_enroll(ctx);
            handle = cl_com_get_handle(prognames[EXECD],1);
         }

         ret_val = cl_commlib_trigger(handle, 1); /* this will block on errors for 1 second */
         switch(ret_val) {
            case CL_RETVAL_SELECT_TIMEOUT:
            case CL_RETVAL_OK:
               break;
            default:
               DPRINTF(("got communication problems - sleeping 30 s"));
               sleep(30); /* For other errors */
               break;
         }
         if (sge_get_com_error_flag(EXECD, SGE_COM_ACCESS_DENIED) == true ||
             sge_get_com_error_flag(EXECD, SGE_COM_ENDPOINT_NOT_UNIQUE) == true) {
            break;
         }

      }

      if (sge_execd_register_at_qmaster(ctx) != 0) {
         if ( had_problems == 0) {
            had_problems = 1;
         }
         continue;
      }
      break;
   }
   
   DEXIT;
   return;
}


/*---------------------------------------------------------------------
 * parse_cmdline_execd
 *---------------------------------------------------------------------*/
static void parse_cmdline_execd(
char **argv
) {
   lList *ref_list = NULL, *alp = NULL, *pcmdline = NULL;
   lListElem *aep;
   u_long32 help = 0;

   DENTER(TOP_LAYER, "parse_cmdline_execd");
            
   alp = sge_parse_cmdline_execd(argv+1, &pcmdline);
   if(alp) {
      /* 
      ** high level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(&alp);
      lFreeList(&pcmdline);
      /* TODO: replace with alpp and DRETURN */
      SGE_EXIT(NULL, 1);
   }

   alp = sge_parse_execd(&pcmdline, &ref_list, &help);
   lFreeList(&pcmdline);
   lFreeList(&ref_list);

   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(&alp);
      /* TODO: replace with alpp and DRETURN */
      SGE_EXIT(NULL, 1);
   }
   lFreeList(&alp);

   if(help) {
      /*
      ** user wanted only help. we can exit!
      */
      /* TODO: replace with alpp and DRETURN */
      SGE_EXIT(NULL, 0);
   }
   DEXIT;
}


/*-------------------------------------------------------------
 * sge_parse_cmdline_execd
 *
 *-------------------------------------------------------------*/ 
static lList *sge_parse_cmdline_execd(
char **argv,
lList **ppcmdline 
) {
char **sp;
char **rp;
stringT str;
lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_parse_cmdline_execd");

   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -nostart-commd */
      if ((rp = parse_noopt(sp, "-nostart-commd", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -lj */
      if ((rp = parse_until_next_opt(sp, "-lj", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDARG_S, *sp);
      sge_usage(EXECD, stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}


/*-------------------------------------------------------------
 * sge_parse_execd
 *
 *-------------------------------------------------------------*/
static lList *sge_parse_execd(lList **ppcmdline, lList **ppreflist, 
                              u_long32 *help) 
{
   stringT str;
   lList *alp = NULL;
   int usageshowed = 0;

   DENTER(TOP_LAYER, "sge_parse_execd");

   /* Loop over all options. Only valid options can be in the
      ppcmdline list.
   */
   while(lGetNumberOfElem(*ppcmdline)) {
      /* -help */
      if(parse_flag(ppcmdline, "-help", &alp, help)) {
         usageshowed = 1;
         sge_usage(EXECD, stdout);
         break;
      }
   }
   
   if(lGetNumberOfElem(*ppcmdline)) {
      sprintf(str, MSG_PARSE_TOOMANYARGS);
      if(!usageshowed)
         sge_usage(EXECD, stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }

   DEXIT;
   return alp;
}

