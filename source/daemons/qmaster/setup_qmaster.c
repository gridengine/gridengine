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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>  
#include <errno.h>
#include <time.h>

#include "sge_bootstrap.h"
#include "sge.h"
#include "sge_conf.h"
#include "commlib.h"
#include "sge_subordinate_qmaster.h"
#include "sge_calendar_qmaster.h"
#include "sge_sched.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_pe_qmaster.h"
#include "sge_cqueue_qmaster.h"
#include "sge_manop_qmaster.h"
#include "sge_job_qmaster.h"
#include "configuration_qmaster.h"
#include "qmaster_heartbeat.h"
#include "qm_name.h"
#include "sched_conf_qmaster.h"
#include "sge_sharetree.h"
#include "sge_sharetree_qmaster.h"
#include "sge_userset.h"
#include "sge_feature.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_utility.h"
#include "setup_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "config_file.h"
#include "sge_qmod_qmaster.h"
#include "sge_give_jobs.h"
#include "setup_path.h"
#include "msg_daemons_common.h"
#include "msg_qmaster.h"
#include "reschedule.h"
#include "sge_job.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"
#include "sge_io.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_cqueue.h"
#include "sge_ckpt.h"
#include "sge_userprj.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"
#include "sge_centry.h"
#include "sge_reporting_qmaster.h"
#include "parse.h"
#include "usage.h"
#include "job_log.h"
#include "qmaster_to_execd.h"
#include "shutdown.h"
#include "sge_hostname.h"
#include "sge_any_request.h"
#include "sge_os.h"
#include "lock.h"
#include "sge_persistence_qmaster.h"
#include "sge_spool.h"
#include "setup.h"
#include "sge_event_master.h"
#include "msg_common.h"
#include "spool/sge_spooling.h"


static void   process_cmdline(char**);
static lList* parse_cmdline_qmaster(char**, lList**);
static lList* parse_qmaster(lList**, u_long32*);
static void   qmaster_init(char**);
static void   communication_setup(void);
static bool   is_qmaster_already_running(void);
static void   qmaster_lock_and_shutdown(int);
static int    setup_qmaster(void);
static int    remove_invalid_job_references(int user);
static int    debit_all_jobs_from_qs(void);



/****** qmaster/setup_qmaster/sge_setup_qmaster() ******************************
*  NAME
*     sge_setup_qmaster() -- setup qmaster 
*
*  SYNOPSIS
*     int sge_setup_qmaster(char* anArgv[]) 
*
*  FUNCTION
*     Process commandline arguments. Remove qmaster lock file. Write qmaster
*     host to the 'act_qmaster' file. Initialize qmaster and reporting. Write
*     qmaster PID file.  
*
*     NOTE: Before this function is invoked, qmaster must become admin user.
*
*  INPUTS
*     char* anArgv[] - commandline argument vector 
*
*  RESULT
*     0 - success 
*
*  NOTES
*     MT-NOTE: sge_setup_qmaster() is NOT MT safe! 
*     MT-NOTE:
*     MT-NOTE: This function must be called exclusively, with the qmaster main
*     MT-NOTE: thread being the *only* active thread. In other words, do not
*     MT-NOTE: invoke this function after any additional thread (directly or
*     MT-NOTE: indirectly) has been created.
*
*     Do *not* write the qmaster pid file, before 'qmaster_init()' did return
*     successfully. Otherwise, if we do have a running qmaster and a second
*     qmaster is started (illegally) on the same host, the second qmaster will
*     overwrite the pid of the qmaster started first. The second qmaster will
*     detect it's insubordinate doing and terminate itself, thus leaving behind
*     a useless pid.
*
*******************************************************************************/
int sge_setup_qmaster(char* anArgv[])
{
   char err_str[1024];

   DENTER(TOP_LAYER, "sge_setup_qmaster");

   umask(022); /* this needs a better solution */

   process_cmdline(anArgv);

   INFO((SGE_EVENT, MSG_STARTUP_BEGINWITHSTARTUP));

   qmaster_unlock(QMASTER_LOCK_FILE);

   if (write_qm_name(uti_state_get_qualified_hostname(), path_state_get_act_qmaster_file(), err_str)) {
      ERROR((SGE_EVENT, "%s\n", err_str));
      SGE_EXIT(1);
   }

   qmaster_init(anArgv);

   sge_write_pid(QMASTER_PID_FILE);

   reporting_initialize(NULL);

   DEXIT;
   return 0;
} /* sge_setup_qmaster() */

/****** qmaster/setup_qmaster/sge_qmaster_thread_init() ************************
*  NAME
*     sge_qmaster_thread_init() -- Initialize a qmaster thread.
*
*  SYNOPSIS
*     int sge_qmaster_thread_init(void) 
*
*  FUNCTION
*     Subsume functions which need to be called immediately after thread
*     startup. This function does make sure that the thread local data
*     structures do contain reasonable values.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     0 - success 
*
*  NOTES
*     MT-NOTE: sge_qmaster_thread_init() is MT safe 
*     MT-NOTE:
*     MT-NOTE: sge_qmaster_thread_init() should be invoked at the beginning
*     MT-NOTE: of a thread function.
*
*******************************************************************************/
int sge_qmaster_thread_init(void)
{
   DENTER(TOP_LAYER, "sge_qmaster_thread_init");

   lInit(nmv);

   sge_setup(QMASTER, NULL);

   reresolve_me_qualified_hostname();

   DEBUG((SGE_EVENT,"%s: qualified hostname \"%s\"\n", SGE_FUNC, uti_state_get_qualified_hostname()));

   DEXIT;
   return 0;
} /* sge_qmaster_thread_init() */

/****** qmaster/setup_qmaster/sge_setup_job_resend() ***************************
*  NAME
*     sge_setup_job_resend() -- Setup job resend events.
*
*  SYNOPSIS
*     void sge_setup_job_resend(void) 
*
*  FUNCTION
*     Register a job resend event for each job or array task which does have a
*     'JTRANSFERING' status.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_setup_job_resend() is not MT safe 
*
*******************************************************************************/
void sge_setup_job_resend(void)
{
   lListElem *job = NULL;

   DENTER(TOP_LAYER, "sge_setup_job_resend");

   job = lFirst(Master_Job_List);

   while (NULL != job)
   {
      lListElem *task;
      u_long32 job_num;

      job_num = lGetUlong(job, JB_job_number);

      task = lFirst(lGetList(job, JB_ja_tasks));
      
      while (NULL != task)
      {
         if (lGetUlong(task, JAT_status) == JTRANSFERING)
         {
            lListElem *granted_queue, *qinstance, *host;
            const char *qname;
            u_long32 task_num, when;
            te_event_t ev;

            task_num = lGetUlong(task, JAT_task_number);

            granted_queue = lFirst(lGetList(task, JAT_granted_destin_identifier_list));

            qname = lGetString(granted_queue, JG_qname);

            qinstance = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), qname);

            host = host_list_locate(Master_Exechost_List, lGetHost(qinstance, QU_qhostname)); 

            when = lGetUlong(task, JAT_start_time);

            when += MAX(load_report_interval(host), MAX_JOB_DELIVER_TIME);

            ev = te_new_event(when, TYPE_JOB_RESEND_EVENT, ONE_TIME_EVENT, job_num, task_num, "job-resend_event");           
            te_add_event(ev);
            te_free_event(ev);

            DPRINTF(("Did add job resend for "u32"/"u32" at %d\n", job_num, task_num, when)); 
         }

         task = lNext(task);
      }

      job = lNext(job);
   }

   DEXIT;
   return;
} /* sge_setup_job_resend() */

/****** qmaster/setup_qmaster/process_cmdline() ********************************
*  NAME
*     process_cmdline() -- Handle command line arguments 
*
*  SYNOPSIS
*     static void process_cmdline(char **anArgv) 
*
*  FUNCTION
*     Handle command line arguments. Parse argument vector and handle options.
*
*  INPUTS
*     char **anArgv - pointer to agrument vector 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: process_cmdline() is NOT MT safe. 
*
*******************************************************************************/
static void process_cmdline(char **anArgv)
{
   lList *alp, *pcmdline;
   lListElem *aep;
   u_long32 help = 0;

   DENTER(TOP_LAYER, "process_cmdline");

   alp = pcmdline = NULL;

   alp = parse_cmdline_qmaster(&anArgv[1], &pcmdline);
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

   alp = parse_qmaster(&pcmdline, &help);
   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }

   if(help) {
      /* user wanted to see help. we can exit */
      lFreeList(pcmdline);
      SGE_EXIT(0);
   }

   DEXIT;
   return;
} /* process_cmdline */

/****** qmaster/setup_qmaster/parse_cmdline_qmaster() **************************
*  NAME
*     parse_cmdline_qmaster() -- Parse command line arguments
*
*  SYNOPSIS
*     static lList* parse_cmdline_qmaster(char **argv, lList **ppcmdline) 
*
*  FUNCTION
*     Decompose argument vector. Handle options and option arguments. 
*
*  INPUTS
*     char **argv       - pointer to argument vector 
*     lList **ppcmdline - pointer to lList pointer which does contain the 
*                         command line arguments upon return. 
*
*  RESULT
*     lList* - pointer to answer list 
*
*  NOTES
*     MT-NOTE: parse_cmdline_qmaster() is MT safe. 
*
*******************************************************************************/
static lList *parse_cmdline_qmaster(char **argv, lList **ppcmdline )
{
   char **sp;
   char **rp;
   stringT str;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "parse_cmdline_qmaster");

   rp = argv;
   while(*(sp=rp))
   {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      printf("%s\n", *sp);
      sge_usage(stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }

   DEXIT;
   return alp;
} /* parse_cmdline_qmaster() */

/****** qmaster/setup_qmaster/parse_qmaster() **********************************
*  NAME
*     parse_qmaster() -- Process options. 
*
*  SYNOPSIS
*     static lList* parse_qmaster(lList **ppcmdline, u_long32 *help) 
*
*  FUNCTION
*     Process options 
*
*  INPUTS
*     lList **ppcmdline - list of options
*     u_long32 *help    - flag is set upon return if help has been requested
*
*  RESULT
*     lList* - answer list 
*
*  NOTES
*     MT-NOTE: parse_qmaster() is not MT safe. 
*
*******************************************************************************/
static lList *parse_qmaster(lList **ppcmdline, u_long32 *help )
{
   stringT str;
   lList *alp = NULL;
   int usageshowed = 0;
   u_long32 flag;

   DENTER(TOP_LAYER, "parse_qmaster");

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
   }

   if(lGetNumberOfElem(*ppcmdline)) {
      sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
      if(!usageshowed)
         sge_usage(stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
} /* parse_qmaster() */

/****** qmaster/setup_qmaster/qmaster_init() ***********************************
*  NAME
*     qmaster_init() -- Initialize qmaster 
*
*  SYNOPSIS
*     static void qmaster_init(char **anArgv) 
*
*  FUNCTION
*     Initialize qmaster. Do general setup and communication setup. 
*
*  INPUTS
*     char **anArgv - process argument vector 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: qmaster_init() is NOT MT safe. 
*
*******************************************************************************/
static void qmaster_init(char **anArgv)
{
   DENTER(TOP_LAYER, "qmaster_init");

   if (setup_qmaster()) {
      CRITICAL((SGE_EVENT, MSG_STARTUP_SETUPFAILED));
      SGE_EXIT(1);
   }

   uti_state_set_exit_func(qmaster_lock_and_shutdown); /* CWD is spool directory */
  
   communication_setup();

   host_list_notify_about_featureset(Master_Exechost_List, feature_get_active_featureset_id());

   starting_up(); /* write startup info message to message file */

   DEXIT;
   return;
} /* qmaster_init() */

/****** qmaster/setup_qmaster/communication_setup() ****************************
*  NAME
*     communication_setup() -- set up communication
*
*  SYNOPSIS
*     static void communication_setup(void) 
*
*  FUNCTION
*     Initialize qmaster communication. 
*
*     This function will fail, if the configured qmaster port is already in
*     use.
*
*     This could happen if either qmaster has been terminated shortly before
*     and the operating system did not get around to free the port or there
*     is a qmaster already running.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: communication_setup() is NOT MT safe 
*
*******************************************************************************/
static void communication_setup(void)
{
   cl_com_handle_t* com_handle = NULL;

   DENTER(TOP_LAYER, "communication_setup");

   DEBUG((SGE_EVENT,"my resolved hostname name is: \"%s\"\n", uti_state_get_qualified_hostname()));

   com_handle = cl_com_get_handle((char*)prognames[QMASTER], 1);

   if (com_handle == NULL)
   {
      ERROR((SGE_EVENT, "port %d already bound\n", sge_get_qmaster_port()));

      if (is_qmaster_already_running() == true)
      {
         char *host = NULL;
         int res = -1; 

         res = cl_com_gethostname(&host, NULL, NULL,NULL);

         CRITICAL((SGE_EVENT, MSG_QMASTER_FOUNDRUNNINGQMASTERONHOSTXNOTSTARTING_S, ((CL_RETVAL_OK == res ) ? host : "unknown")));

         if (CL_RETVAL_OK == res) { free(host); }
      }

      SGE_EXIT(1);
   }

   if (com_handle) {
      int max_connections = 0;

      /* save old debug log level and set log level to INFO */
      u_long32 old_ll = log_state_get_log_level();

      /* enable max connection close mode */
      cl_com_set_max_connection_close_mode(com_handle, CL_ON_MAX_COUNT_CLOSE_AUTOCLOSE_CLIENTS);
#if 0
      /* Enable this to check max. connection count behaviour of qmaster */
      cl_com_set_max_connections(com_handle, 3); 
#endif
      cl_com_get_max_connections(com_handle, &max_connections);

      /* add local host to allowed host list */
      cl_com_add_allowed_host(com_handle,com_handle->local->comp_host);

      /* check dynamic event client count */
      max_dynamic_event_clients = sge_set_max_dynamic_event_clients(max_dynamic_event_clients); 

      /* log startup info into qmaster messages file */
      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, MSG_QMASTER_MAX_FILE_DESCRIPTORS_LIMIT_U, u32c(max_connections)));
      INFO((SGE_EVENT, MSG_QMASTER_MAX_EVC_LIMIT_U, u32c( max_dynamic_event_clients)));
      log_state_set_log_level(old_ll);
   }

   cl_commlib_set_connection_param(cl_com_get_handle("qmaster",1), HEARD_FROM_TIMEOUT, conf.max_unheard);

   DEXIT;
   return;
} /* communication_setup() */

/****** qmaster/setup_qmaster/is_qmaster_already_running() *********************
*  NAME
*     is_qmaster_already_running() -- is qmaster already running 
*
*  SYNOPSIS
*     static bool is_qmaster_already_running(void) 
*
*  FUNCTION
*     Check, whether there is running qmaster already.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     true  - running qmaster detected. 
*     false - otherwise
*
*  NOTES
*     MT-NOTE: is_qmaster_already_running() is not MT safe 
*
*  BUGS
*     This function will only work, if the PID found in the qmaster PID file
*     either does belong to a running qmaster or no process at all.
*
*     Of course PID's will be reused. This is, however, not a problem because
*     of the very specifc situation in which this function is called.
*
*******************************************************************************/
static bool is_qmaster_already_running(void)
{
   enum { NULL_SIGNAL = 0 };

   bool res = true;
   char pidfile[SGE_PATH_MAX] = { '\0' };
   pid_t pid = 0;

   DENTER(TOP_LAYER, "is_qmaster_already_running");

   sprintf(pidfile, "%s/%s", bootstrap_get_qmaster_spool_dir(), QMASTER_PID_FILE);

   if ((pid = sge_readpid(pidfile)) == 0)
   {
      DEXIT;
      return false;
   }

   res = (kill(pid, NULL_SIGNAL) == 0) ? true: false;

   DEXIT;
   return res;
} /* is_qmaster_already_running() */

/****** qmaster/setup_qmaster/qmaster_lock_and_shutdown() ***************************
*  NAME
*     qmaster_lock_and_shutdown() -- Acquire qmaster lock file and shutdown 
*
*  SYNOPSIS
*     static void qmaster_lock_and_shutdown(int anExitValue) 
*
*  FUNCTION
*     qmaster exit function. This version MUST NOT be used, if the current
*     working   directory is NOT the spool directory. Other components do rely
*     on finding the lock file in the spool directory.
*
*  INPUTS
*     int anExitValue - exit value 
*
*  RESULT
*     void - none
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: qmaster_lock_and_shutdown() is MT safe 
*
*******************************************************************************/
static void qmaster_lock_and_shutdown(int anExitValue)
{
   DENTER(TOP_LAYER, "qmaster_lock_and_shutdown");
   
   if (qmaster_lock(QMASTER_LOCK_FILE) == -1) {
      CRITICAL((SGE_EVENT, MSG_QMASTER_LOCKFILE_ALREADY_EXISTS));
   }
   sge_gdi_shutdown();

   DEXIT;
   return;
} /* qmaster_lock_and_shutdown() */

static int setup_qmaster(void)
{
   lListElem *jep, *ep, *tmpqep;
   static bool first = true;
   int ret;
   lListElem *lep = NULL;
   extern int new_config;
   lListElem *spooling_context = NULL;
   lList *answer_list = NULL;
   time_t time_start, time_end;

   DENTER(TOP_LAYER, "sge_setup_qmaster");

   if (first)
      first = false;
   else {
      CRITICAL((SGE_EVENT, MSG_SETUP_SETUPMAYBECALLEDONLYATSTARTUP));
      DEXIT;
      return -1;
   }   

   /* register our error function for use in replace_params() */
   config_errfunc = set_error;

   /*
    * Initialize Master lists and hash tables, if necessary 
    */

/** This is part is making the scheduler a
  * lot slower that it was before. This was an enhancement introduced
  * in cvs revision 1.35
  * It might be added again, when hte hashing problem on large job lists
  * with only a view owners is solved.
  */
#if 0
   if (Master_Job_List == NULL) {
      Master_Job_List = lCreateList("Master_Job_List", JB_Type);
   }
   cull_hash_new(Master_Job_List, JB_owner, 0);
#endif

   if (!sge_initialize_persistence(&answer_list)) {
      answer_list_output(&answer_list);
      DEXIT;
      return -1;
   } else {
      spooling_context = spool_get_default_context();
   }

   /*
   ** get cluster configuration
   */
   spool_read_list(&answer_list, spooling_context, &Master_Config_List, SGE_TYPE_CONFIG);
   answer_list_output(&answer_list);

   ret = select_configuration(uti_state_get_qualified_hostname(), Master_Config_List, &lep);
   if (ret) {
      if (ret == -3)
         WARNING((SGE_EVENT, MSG_CONFIG_FOUNDNOLOCALCONFIGFORQMASTERHOST_S,
                 uti_state_get_qualified_hostname()));
      else {           
         ERROR((SGE_EVENT, MSG_CONFIG_ERRORXSELECTINGCONFIGY_IS, ret, uti_state_get_qualified_hostname()));
         DEXIT;
         return -1;
      }   
   }
   ret = merge_configuration( lGetElemHost(Master_Config_List, CONF_hname, SGE_GLOBAL_NAME), lep, &conf, NULL);
   if (ret) {
      ERROR((SGE_EVENT, MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS, ret, uti_state_get_qualified_hostname()));
      DEXIT;
      return -1;
   }
   sge_show_conf();         
   new_config = 1;

   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();
   DEBUG((SGE_EVENT,"uti_state_get_qualified_hostname() returned \"%s\"\n",uti_state_get_qualified_hostname() ));
   /*
   ** read in all objects and check for correctness
   */
   DPRINTF(("Complex Attributes----------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_CEntry_List, SGE_TYPE_CENTRY);
   answer_list_output(&answer_list);

   DPRINTF(("host_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Exechost_List, SGE_TYPE_EXECHOST);
   spool_read_list(&answer_list, spooling_context, &Master_Adminhost_List, SGE_TYPE_ADMINHOST);
   spool_read_list(&answer_list, spooling_context, &Master_Submithost_List, SGE_TYPE_SUBMITHOST);
   answer_list_output(&answer_list);

   if (!host_list_locate(Master_Exechost_List, SGE_TEMPLATE_NAME)) {
      /* add an exec host "template" */
      if (sge_add_host_of_type(SGE_TEMPLATE_NAME, SGE_EXECHOST_LIST))
         ERROR((SGE_EVENT, MSG_CONFIG_ADDINGHOSTTEMPLATETOEXECHOSTLIST));
   }

   /* add host "global" to Master_Exechost_List as an exec host */
   if (!host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME)) {
      /* add an exec host "global" */
      if (sge_add_host_of_type(SGE_GLOBAL_NAME, SGE_EXECHOST_LIST))
         ERROR((SGE_EVENT, MSG_CONFIG_ADDINGHOSTGLOBALTOEXECHOSTLIST));
   }

   /* add qmaster host to Master_Adminhost_List as an administrativ host */
   if (!host_list_locate(Master_Adminhost_List, uti_state_get_qualified_hostname())) {
      if (sge_add_host_of_type(uti_state_get_qualified_hostname(), SGE_ADMINHOST_LIST)) {
         DEXIT;
         return -1;
      }
   }

   DPRINTF(("manager_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Manager_List, SGE_TYPE_MANAGER);
   answer_list_output(&answer_list);
   if (!manop_is_manager("root")) {
      ep = lAddElemStr(&Master_Manager_List, MO_name, "root", MO_Type);

      if (!spool_write_object(&answer_list, spooling_context, ep, "root", SGE_TYPE_MANAGER)) {
         answer_list_output(&answer_list);
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEMANAGERLIST)); 
         DEXIT;
         return -1;
      }
   }
   for_each(ep, Master_Manager_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));

   DPRINTF(("host group definitions-----------\n"));
   spool_read_list(&answer_list, spooling_context, hgroup_list_get_master_list(), 
                   SGE_TYPE_HGROUP);
   answer_list_output(&answer_list);

   DPRINTF(("operator_list----------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Operator_List, SGE_TYPE_OPERATOR);
   answer_list_output(&answer_list);
   if (!manop_is_operator("root")) {
      ep = lAddElemStr(&Master_Operator_List, MO_name, "root", MO_Type);

      if (!spool_write_object(&answer_list, spooling_context, ep, "root", SGE_TYPE_OPERATOR)) {
         answer_list_output(&answer_list);
         CRITICAL((SGE_EVENT, MSG_CONFIG_CANTWRITEOPERATORLIST)); 
         DEXIT;
         return -1;
      }
   }
   for_each(ep, Master_Operator_List) 
      DPRINTF(("%s\n", lGetString(ep, MO_name)));


   DPRINTF(("userset_list------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Userset_List, SGE_TYPE_USERSET);
   answer_list_output(&answer_list);

   DPRINTF(("calendar list ------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Calendar_List, SGE_TYPE_CALENDAR);
   answer_list_output(&answer_list);

#ifndef __SGE_NO_USERMAPPING__
   DPRINTF(("administrator user mapping-----------\n"));
   spool_read_list(&answer_list, spooling_context, cuser_list_get_master_list(), SGE_TYPE_CUSER);
   answer_list_output(&answer_list);
#endif

   DPRINTF(("cluster_queue_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, object_type_get_master_list(SGE_TYPE_CQUEUE), SGE_TYPE_CQUEUE);
   answer_list_output(&answer_list);
   cqueue_list_set_unknown_state(
            *(object_type_get_master_list(SGE_TYPE_CQUEUE)),
            NULL, false, true);
   
   DPRINTF(("pe_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Pe_List, SGE_TYPE_PE);
   answer_list_output(&answer_list);

   DPRINTF(("ckpt_list---------------------------------\n"));
   spool_read_list(&answer_list, spooling_context, &Master_Ckpt_List, SGE_TYPE_CKPT);
   answer_list_output(&answer_list);

   DPRINTF(("job_list-----------------------------------\n"));
   /* measure time needed to read job database */
   time_start = time(0);
   spool_read_list(&answer_list, spooling_context, &Master_Job_List, SGE_TYPE_JOB);
   time_end = time(0);
   answer_list_output(&answer_list);

   {
      u_long32 saved_logginglevel = log_state_get_log_level();
      log_state_set_log_level(LOG_INFO);
      INFO((SGE_EVENT, "read job database with %d entries in %ld seconds\n", 
            lGetNumberOfElem(Master_Job_List), time_end - time_start));
      log_state_set_log_level(saved_logginglevel);
   }

   for_each(jep, Master_Job_List) {
      DPRINTF(("JOB "u32" PRIORITY %d\n", lGetUlong(jep, JB_job_number), 
            (int)lGetUlong(jep, JB_priority) - BASE_PRIORITY));

      /* doing this operation we need the complete job list read in */
      job_suc_pre(jep);

      centry_list_fill_request(lGetList(jep, JB_hard_resource_list), 
                  Master_CEntry_List, false, true, false);
   }

   /* 
      if the job is in state running 
      we have to register each slot 
      in a queue and in the parallel 
      environment if the job is a 
      parallel one
   */
   debit_all_jobs_from_qs(); 
   debit_all_jobs_from_pes(Master_Pe_List); 

   /* clear suspend on subordinate flag in QU_state */ 
   for_each(tmpqep, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      lList *qinstance_list = lGetList(tmpqep, CQ_qinstances);
      lListElem *qinstance;

      for_each(qinstance, qinstance_list) {
         qinstance_state_set_susp_on_sub(qinstance, false);
      }
   }

   /* 
    * Initialize
    *    - suspend on subordinate state 
    *    - cached QI values.
    */
   for_each(tmpqep, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      cqueue_mod_qinstances(tmpqep, NULL, tmpqep, true);
   }
        
   /* calendar */
   {
      lListElem *cep;
      for_each (cep, Master_Calendar_List) 
         calendar_update_queue_states(cep, NULL, NULL);
   }

   /* rebuild signal resend events */
   rebuild_signal_events();

   /* scheduler configuration stuff */
   DPRINTF(("scheduler config -----------------------------------\n"));
   {
      lList *sched_conf=NULL;
      spool_read_list(&answer_list, spooling_context, &sched_conf, SGE_TYPE_SCHEDD_CONF);
      /* JG: TODO: reading the schedd configuration may fail, 
      * as it is not created at install time.
      * The corresponding error message is confusing, so do not output the error.
      * Better: Create config at install time (trough spooldefaults)
      * answer_list_output(&answer_list);
      */
      if (lGetNumberOfElem(sched_conf) == 0) {
         lListElem *ep = sconf_create_default();

         if (sched_conf == NULL) {
            sched_conf = lCreateList("schedd config list", SC_Type);
         }
      
         lAppendElem(sched_conf, ep);
         spool_write_object(&answer_list, spool_get_default_context(), ep, "schedd_conf", SGE_TYPE_SCHEDD_CONF);
         answer_list_output(&answer_list);
      }
      
      if (!sconf_set_config(&sched_conf, &answer_list)){
         answer_list_output(&answer_list);
         lFreeList(answer_list);
         lFreeList(sched_conf);
         DEXIT;
         return -1;
      } 

      /* The REPRIORITIZE parameter of the master configuration is not spooled. It is generated
       *  of out the reprioritze_interval flag in the scheduler. After reading in the scheduler
       *  configuration, we have to update the master configuration.
       */   
       
      {
         lListElem *conf = NULL; 
         lList *ep_list = NULL;
         lListElem *ep = NULL; 
         int reprioritize = (sconf_get_reprioritize_interval() != 0); 
         char value[20];
         conf = lGetElemHost(Master_Config_List, CONF_hname, "global");
         ep_list = lGetList(conf, CONF_entries);

         ep = lGetElemStr(ep_list, CF_name, REPRIORITIZE);
         if (!ep){
            ep = lCreateElem(CF_Type);
            lSetString(ep, CF_name, REPRIORITIZE);
            lAppendElem(ep_list, ep);           
         }
         
         sprintf(value, "%d", reprioritize);
         lSetString(ep, CF_value, value);
         lSetUlong(ep, CF_local, 0);    
      }
      
   }

   /* SGEEE: read user list */
   spool_read_list(&answer_list, spooling_context, &Master_User_List, SGE_TYPE_USER);
   answer_list_output(&answer_list);

   remove_invalid_job_references(1);

   /* SGE: read project list */
   spool_read_list(&answer_list, spooling_context, &Master_Project_List, SGE_TYPE_PROJECT);
   answer_list_output(&answer_list);

   remove_invalid_job_references(0);
   
   /* SGEEE: read share tree */
   spool_read_list(&answer_list, spooling_context, &Master_Sharetree_List, SGE_TYPE_SHARETREE);
   answer_list_output(&answer_list);
   ep = lFirst(Master_Sharetree_List);
   if (ep) {
      lList *alp = NULL;
      lList *found = NULL;
      ret = check_sharetree(&alp, ep, Master_User_List, Master_Project_List, 
            NULL, &found);
      found = lFreeList(found);
      alp = lFreeList(alp); 
   }
   
   /* RU: */
   /* initiate timer for all hosts because they start in 'unknown' state */ 
   if (Master_Exechost_List) {
      lListElem *host               = NULL;
      lListElem *global_host_elem   = NULL;
      lListElem *template_host_elem = NULL;

      /* get "global" element pointer */
      global_host_elem   = host_list_locate(Master_Exechost_List, SGE_GLOBAL_NAME);   

      /* get "template" element pointer */
      template_host_elem = host_list_locate(Master_Exechost_List, SGE_TEMPLATE_NAME);
  
      for_each(host, Master_Exechost_List) {
         if ( (host != global_host_elem ) && (host != template_host_elem ) ) {
            reschedule_add_additional_time(load_report_interval(host));
            reschedule_unknown_trigger(host);
            reschedule_add_additional_time(0); 
         }
      }
   }

   DEXIT;
   return 0;
}

/* get rid of still debited per job usage contained 
   in user or project object if the job is no longer existing */
static int remove_invalid_job_references(
int user 
) {
   lListElem *up, *upu, *next;
   u_long32 jobid;

   DENTER(TOP_LAYER, "remove_invalid_job_references");

   for_each (up, user?Master_User_List:Master_Project_List) {

      int spool_me = 0;
      next = lFirst(lGetList(up, UP_debited_job_usage));
      while ((upu=next)) {
         next = lNext(upu);

         jobid = lGetUlong(upu, UPU_job_number);
         if (!job_list_locate(Master_Job_List, jobid)) {
            lRemoveElem(lGetList(up, UP_debited_job_usage), upu);
            WARNING((SGE_EVENT, "removing reference to no longer existing job "u32" of %s "SFQ"\n",
                           jobid, user?"user":"project", lGetString(up, UP_name)));
            spool_me = 1;
         }
      }

      if (spool_me) {
         lList *answer_list = NULL;
         spool_write_object(&answer_list, spool_get_default_context(), up, 
                            lGetString(up, UP_name), user ? SGE_TYPE_USER : 
                                                            SGE_TYPE_PROJECT);
         answer_list_output(&answer_list);
      }
   }

   DEXIT;
   return 0;
}

static int debit_all_jobs_from_qs()
{
   lListElem *gdi;
   u_long32 slots, jid, tid;
   const char *queue_name;
   lListElem *hep = NULL;
   lListElem *next_jep, *jep, *qep, *next_jatep, *jatep;
   int ret = 0;

   DENTER(TOP_LAYER, "debit_all_jobs_from_qs");

   next_jep = lFirst(Master_Job_List);
   while ((jep=next_jep)) {
   
      /* may be we have to delete this job */   
      next_jep = lNext(jep);
      jid = lGetUlong(jep, JB_job_number);
      
      next_jatep = lFirst(lGetList(jep, JB_ja_tasks));
      while ((jatep = next_jatep)) {
         next_jatep = lNext(jatep);
         tid = lGetUlong(jatep, JAT_task_number);

         /* don't look at states - we only trust in 
            "granted destin. ident. list" */

         for_each (gdi, lGetList(jatep, JAT_granted_destin_identifier_list)) {

            queue_name = lGetString(gdi, JG_qname);
            slots = lGetUlong(gdi, JG_slots);
            
            if (!(qep = cqueue_list_locate_qinstance(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), queue_name))) {
               ERROR((SGE_EVENT, MSG_CONFIG_CANTFINDQUEUEXREFERENCEDINJOBY_SU,  
                      queue_name, u32c(lGetUlong(jep, JB_job_number))));
               lRemoveElem(lGetList(jep, JB_ja_tasks), jatep);   
            } else {
               /* debit in all layers */
               debit_host_consumable(jep, host_list_locate(Master_Exechost_List,
                                     "global"), Master_CEntry_List, slots);
               debit_host_consumable(jep, hep = host_list_locate(
                        Master_Exechost_List, lGetHost(qep, QU_qhostname)), 
                        Master_CEntry_List, slots);
               qinstance_debit_consumable(qep, jep, Master_CEntry_List, slots);
            }
         }
      }
   }

   DEXIT;
   return ret;
}

