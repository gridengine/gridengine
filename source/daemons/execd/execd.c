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
#include "setup.h"
#include "sge_any_request.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_load_sensor.h"
#include "sge_log.h"
#include "sge_prog.h"
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
#include "job_log.h"
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

#include "msg_common.h"
#include "msg_execd.h"
#include "msg_gdilib.h"

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

static void execd_exit_func(int i);
static void execd_register(void);
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
   { TAG_NEW_FEATURES,  NULL, NULL, 0, execd_new_features },
   { TAG_GET_NEW_CONF,  NULL, NULL, 0, execd_get_new_conf },
   { -1,                NULL, NULL, 0, execd_ck_to_do}
};


/* time execd maximal waits in the dispatch routine */
#define DISPATCH_TIMEOUT_SGE     2

int main(int argc, char *argv[]);

/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int i, ret, suc, dispatch_timeout;
   char err_str[1024];
   int priority_tags[10];
#ifdef PW   
   int mode_guess;
#endif

   DENTER_MAIN(TOP_LAYER, "execd");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   sge_mt_init();

#ifdef PW
   if ((mode_guess = product_mode_guess(argv[0])) == M_INVALID) {
      fprintf(stderr, MSG_EXECD_PROGINVALIDNAME_S,
              argv[0] ? argv[0] : MSG_NULL);
      exit(1);
   }  
#endif

   /* This needs a better solution */
   umask(022);
      
   /* Initialize path for temporary logging until we chdir to spool */
   log_state_set_log_file(TMP_ERR_FILE_EXECD);

#if RAND_ERROR
   rand_error = 1;
#endif

   /* exit func for SGE_EXIT() */
   in_main_loop = 0;
   uti_state_set_exit_func(execd_exit_func);
   sge_setup_sig_handlers(EXECD);

   memset(priority_tags, 0, sizeof(priority_tags));
   priority_tags[0] = TAG_ACK_REQUEST;
   priority_tags[1] = TAG_JOB_EXECUTION;

   sge_setup(EXECD, NULL);   
   prepare_enroll(prognames[EXECD], 1, priority_tags);

   if ((i=sge_occupy_first_three())>=0) {
      CRITICAL((SGE_EVENT, MSG_FILE_REDIRECTFD_I, i));
      SGE_EXIT(1);
   }     

   lInit(nmv);

#ifdef PW
   if (get_product_mode() != mode_guess) {
      CRITICAL((SGE_EVENT, MSG_EXECD_NOPROGNAMEPROD_S, argv[0]));
      SGE_EXIT(1);
   }
#endif

   parse_cmdline_execd(argv);   

   /* check for running execd - ignore $COMMD_HOST */
   if (start_commd) {
      set_commlib_param(CL_P_COMMDHOST, 0, uti_state_get_unqualified_hostname(), NULL);
   }
   
   /* start commd if necessary */ 
   set_commlib_param(CL_P_NAME, 0, prognames[EXECD], NULL);
   set_commlib_param(CL_P_ID, 1, NULL, NULL);
   set_commlib_param(CL_P_PRIO_LIST, 0, NULL, priority_tags); 
   if((ret = enroll())) {
      DTRACE;
      if (ret == CL_CONNECT && start_commd) {
         suc = startprog(1, 2, argv[0], NULL, SGE_COMMD, NULL);

         if (suc) {
            CRITICAL((SGE_EVENT, MSG_COM_CANTSTARTCOMMD));
            SGE_EXIT(1);
         }
         sleep(3);
         ret = enroll();
         switch (ret) {
         case 0:
            /* ok */
            break;
         case CL_CONNECT:
            /* usually clients end here if there is no commd */
            CRITICAL((SGE_EVENT, MSG_SGETEXT_NOCOMMD));
            SGE_EXIT(1);
            break;
         case COMMD_NACK_CONFLICT:
            /* usually daemons end here if they get enrolled twice */
            /* or if they get started after a daemon that made no leave() before exit() */
            CRITICAL((SGE_EVENT, MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S, 
               uti_state_get_sge_formal_prog_name()
               ));
            SGE_EXIT(1);
            break;
         case CL_ALREADYDONE:
            break;
         default:
            CRITICAL((SGE_EVENT, MSG_GDI_ENROLLTOCOMMDFAILED_S , cl_errstr(ret)));
            SGE_EXIT(1);
         }     
      }
      else if (ret == COMMD_NACK_CONFLICT) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S, 
            uti_state_get_sge_formal_prog_name()));
         SGE_EXIT(1);
      }
      else {
         CRITICAL((SGE_EVENT, MSG_COM_CANTENROLL2COMMD_S, cl_errstr(ret)));
         SGE_EXIT(1);
      }            
   }
   
   /* daemonizes if qmaster is unreachable */   
   sge_setup_sge_execd();

   if (!getenv("SGE_ND"))
      daemonize_execd();

   /* don't wanna get old messages */
   remove_pending_messages(NULL, 0, 0, 0);

   /* commlib call to mark all commprocs as unknown */
   reset_last_heard();

   /* are we using qidle or not */
   sge_ls_qidle(use_qidle);
   sge_ls_gnu_ls(1);
   
   DPRINTF(("use_qidle: %d\n", use_qidle));

   /* test load sensor (internal or external) */
   lFreeList(sge_build_load_report());

   execd_register();

   sge_write_pid(EXECD_PID_FILE);

   /* at this point we are sure we are the only sge_execd */
   /* first we have to report any reaped children that might exist */

   starting_up();

#ifdef COMPILE_DC
   if (ptf_init()) {
      CRITICAL((SGE_EVENT, MSG_EXECD_NOSTARTPTF));
      SGE_EXIT(1);
   }
   INFO((SGE_EVENT, MSG_EXECD_STARTPDCANDPTF));
#endif

   Master_Job_List = lCreateList("Master_Job_List", JB_Type);
   job_list_read_from_disk(&Master_Job_List, "Master_Job_List",
                           0, SPOOL_WITHIN_EXECD, 
                          job_initialize_job);
   
   /* clean up jobs hanging around (look in active_dir) */
   clean_up_old_jobs(1);
   sge_send_all_reports(0, NUM_REP_REPORT_JOB, execd_report_sources);

   dispatch_timeout = DISPATCH_TIMEOUT_SGE;
      
   in_main_loop = 1;

   /***** MAIN LOOP *****/
   while (true) {
      /* use auto acknowlege feature of dispatcher for the following
         inbound messages */
      static int tagarray[] = { TAG_SIGJOB, TAG_SIGQUEUE, TAG_NONE };
 
      i = dispatch(execd_dispatcher_table, 
                   sizeof(execd_dispatcher_table)/sizeof(dispatch_entry),
                   tagarray, dispatch_timeout, err_str, dispatcher_errfunc, 1);

      if (sigpipe_received) {
          sigpipe_received = 0;
          INFO((SGE_EVENT, "SIGPIPE received"));
      }

      if (i) {             
         WARNING((SGE_EVENT, MSG_COM_RECEIVEREQUEST_S, (i==CL_FIRST_FREE_EC) ? err_str : cl_errstr(i)));

         if (shut_me_down == 1) {
            sge_shutdown();
            DEXIT;
            return 0;
         }
      }
      else {
         sge_shutdown();
         DEXIT;
         return 0;
      }
      sleep(1);	/* If there is an error dont kill the system */
   }
}


/*-------------------------------------------------------------
 * Function installed to be called just before exit() is called.
 * clean up
 *-------------------------------------------------------------*/
static void execd_exit_func(
int i 
) {
   DENTER(TOP_LAYER, "execd_exit_func");

   sge_gdi_shutdown();  /* tell commd we're going */

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

/*-------------------------------------------------------------
 * execd_register
 *
 * Function for registering the execd at qmaster
 *-------------------------------------------------------------*/
static void execd_register()
{
   lList *hlp = NULL, *alp = NULL; 
   lListElem *hep;
   int had_problems = 0; /* to ensure single logging */

   DENTER(TOP_LAYER, "execd_register");

   hlp = lCreateList("exechost starting", EH_Type);
   hep = lCreateElem(EH_Type);
   lSetUlong(hep, EH_featureset_id, feature_get_active_featureset_id());
   lAppendElem(hlp, hep);

   while (true) {
      lListElem *aep;
      DPRINTF(("*****Checking In With qmaster*****\n"));

      if (had_problems)
         sleep(10);

      alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_ADD, &hlp, NULL, NULL);
      aep = lFirst(alp);
      if (!alp || (lGetUlong(aep, AN_status)!=STATUS_OK)) {
         WARNING((SGE_EVENT, MSG_COM_CANTREGISTER_S, aep?lGetString(aep, AN_text):MSG_COM_ERROR));
         had_problems = 1;
         alp = lFreeList(alp);
         continue;
      }
 
      if(lGetUlong(lFirst(alp), AN_status) == STATUS_DENIED) {
         CRITICAL((SGE_EVENT, MSG_COM_REGISTERDENIED_S, lGetString(lFirst(alp), AN_text)));
         SGE_EXIT(1);
      }
      break;
   }
   
   hlp = lFreeList(hlp);
   alp = lFreeList(alp);

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
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }

   alp = sge_parse_execd(&pcmdline, &ref_list, &help);
   lFreeList(pcmdline);
   lFreeList(ref_list);

   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      SGE_EXIT(1);
   }
   lFreeList(alp);

   if(help) {
      /*
      ** user wanted only help. we can exit!
      */
      SGE_EXIT(0);
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
      sge_usage(stderr);
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
   u_long32 flag;
   char *filename;

   DENTER(TOP_LAYER, "sge_parse_execd");

   /* Loop over all options. Only valid options can be in the
      ppcmdline list.
   */
   while(lGetNumberOfElem(*ppcmdline))
   {
      flag = 0;
      /* -help */
      if(parse_flag(ppcmdline, "-help", &alp, help)) {
         usageshowed = 1;
         sge_usage(stdout);
         break;
      }

      /* -nostart-commd */
      if(parse_flag(ppcmdline, "-nostart-commd", &alp, &flag)) {
         start_commd = false;
         continue;
      }

      /* -lj */
      if(parse_string(ppcmdline, "-lj", &alp, &filename)) {
         enable_job_logging(filename);
         FREE(filename);
         continue;
      }
   }
   if(lGetNumberOfElem(*ppcmdline)) {
      sprintf(str, MSG_PARSE_TOOMANYARGS);
      if(!usageshowed)
         sge_usage(stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}

