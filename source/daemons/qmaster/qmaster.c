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

#include "qmaster.h"

#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include "sgermon.h"
#include "sge_prog.h"
#include "sig_handlers.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_event_master.h"
#include "shutdown.h"
#include "ck_to_do_qmaster.h"
#include "sge_qmaster_process_message.h"
#include "sge_all_listsL.h"
#include "setup.h"
#include "msg_qmaster.h"
#include "setup_qmaster.h"
#include "sge_unistd.h"
#include "setup_path.h"
#include "sge_spool.h"
#include "qmaster_to_execd.h"
#include "sge_host.h"
#include "sge_hostname.h"
#include "qmaster_running.h"
#include "commlib.h"
#include "usage.h"
#include "startprog.h"
#include "msg_common.h"
#include "sge_any_request.h"
#include "sge_os.h"
#include "lock.h"
#include "parse.h"
#include "sge_answer.h"
#include "job_log.h"
#include "sge_security.h"
#include "sge_gdi_request.h"
#include "sge_manop.h"
#include "qmaster_heartbeat.h"
#include "sge_cuser.h"
#include "sge_mt_init.h"
#include "sge_reporting_qmaster.h"
#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"


static void qmaster_init(char **anArgv);
static void communication_setup(char **anArgv);
static void set_message_priorities(const char* thePriorities);
static void daemonize_qmaster(void);
static void qmaster_shutdown(int anExitValue);
static void qmaster_lock_and_shutdown(int anExitValue);
static void process_cmdline(char **anArgv);
static lList *parse_cmdline_qmaster(char **argv, lList **ppcmdline);
static lList *parse_qmaster(lList **ppcmdline, lList **ppreflist, u_long32 *help);


/****** main() ***************************************************************
*  NAME
*     main() -- qmaster entry point
*
*  SYNOPSIS
*     int main(int argc, char **argv)
*
*  FUNCTION
*     qmaster entry point
*
*  INPUTS
*     int argc
*     char **argv
*
*  RESULT
*
******************************************************************************/
int main(int argc, char **argv)
{
   enum { TIMELEVEL = 0 };

   DENTER_MAIN(TOP_LAYER, "qmaster");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   sge_init_language_func((gettext_func_type)gettext, (setlocale_func_type)setlocale, (bindtextdomain_func_type)bindtextdomain, (textdomain_func_type)textdomain);
   sge_init_language(NULL,NULL);   
#endif 

   sge_mt_init();
   commlib_mt_init(); /* shall be removed with new comm system */

   sge_getme(QMASTER);
   uti_state_set_exit_func(qmaster_shutdown);

   in_main_loop = 0; /* global var, used in signal handler */
   sge_setup_sig_handlers(QMASTER);

   /* Use tmp message file; no qmaster spool directory as yet */
   log_state_set_log_file(TMP_ERR_FILE_QMASTER);
   
   process_cmdline(argv);
   qmaster_init(argv);
   in_main_loop = 1;

   /* initialize accounting and reporting file generation */
   reporting_initialize(NULL);

   while (true) {
      time_t now;

      now = (time_t)sge_get_gmt();
      increment_heartbeat(now);

      if (dead_children) {
         int stat;

         while (waitpid(-1, &stat, WNOHANG) > 0) { /*EMPTY */ };
         dead_children = 0;
      }

      if (shut_me_down) {
         /* we have to deliver events before shutting down */
         sge_add_event(NULL, now, sgeE_QMASTER_GOES_DOWN, 0, 0, 
                       NULL, NULL, NULL, NULL);
         /* send event, even if event clients are busy */
         set_event_client_busy(NULL, 0); 
         ck_4_deliver_events(now);

         /* shutdown spooling framework */
         sge_shutdown_persistence(NULL);

         /* shutdown reporting, flush buffers */
         reporting_shutdown(NULL);

         sge_shutdown();
      }

      sge_stopwatch_start(TIMELEVEL);
      sge_ck_to_do_qmaster(0);
      sge_stopwatch_log(TIMELEVEL, "check to do:");

      DPRINTF(("===========================[EPOCH]=====================================\n"));

      sge_stopwatch_start(TIMELEVEL);
      sge_qmaster_process_message(NULL);
   }

   DEXIT;
   return 0;
} /* main() */


static void qmaster_init(char **anArgv)
{
   DENTER(TOP_LAYER, "qmaster_init");

   umask(022); /* this needs a better solution */

   lInit(nmv); /* set CULL namespace */

   sge_setup(QMASTER, NULL); /* misc setup */

   INFO((SGE_EVENT, MSG_STARTUP_BEGINWITHSTARTUP));

   communication_setup(anArgv);

   if (sge_setup_qmaster()) {
      CRITICAL((SGE_EVENT, MSG_STARTUP_SETUPFAILED));
      SGE_EXIT(1);
   }

   uti_state_set_exit_func(qmaster_lock_and_shutdown); /* CWD is spool directory */

   daemonize_qmaster();

   sge_write_pid(QMASTER_PID_FILE);

   host_list_notify_about_featureset(Master_Exechost_List, feature_get_active_featureset_id());

   starting_up(); /* write startup info message to message file */

   DEXIT;
   return;
} /* qmaster_init() */


static void communication_setup(char **anArgv)
{
   const char *msg_prio = NULL;
   const char *host = NULL;
   int enrolled = 0;

   DENTER(TOP_LAYER, "communication_setup");

   if ((msg_prio = getenv("SGE_PRIORITY_TAGS")) != NULL) {
      set_message_priorities(msg_prio);
   }

   /* to ensure SGE host_aliasing is considered when resolving
    * me.qualified_hostname before commd might be available
    */
   if ((host = sge_host_resolve_name_local(uti_state_get_qualified_hostname()))) {
      uti_state_set_qualified_hostname(host);
   }

   if ((enrolled = check_for_running_qmaster())> 0) {
      remove_pending_messages(NULL, 0, 0, 0);
   }

   set_commlib_param(CL_P_COMMDHOST, 0, uti_state_get_qualified_hostname(), NULL);
   commlib_state_set_logging_function(sge_log);     
   
   if (!enrolled) {
      int ret;

      set_commlib_param(CL_P_NAME, 0, prognames[QMASTER], NULL);
      set_commlib_param(CL_P_ID, 1, NULL, NULL);

      if ((ret = enroll())) {
         if (ret == CL_CONNECT && start_commd) {
            startprog(1, 2, anArgv[0], NULL, SGE_COMMD, NULL);
            sleep(5);
         } else if (ret == COMMD_NACK_CONFLICT) {
            ERROR((SGE_EVENT, MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S, uti_state_get_sge_formal_prog_name()));
            SGE_EXIT(1);
         } else {
            ERROR((SGE_EVENT, MSG_COMMD_CANTENROLLTOCOMMD_S, cl_errstr(ret)));
            SGE_EXIT(1);
         }             
      } else {
         WARNING((SGE_EVENT, MSG_COMMD_FOUNDRUNNINGCOMMDONLOCALHOST));
      }
   } /* !enrolled */

   reset_last_heard(); /* commlib call to mark all commprocs as unknown */

   DEXIT;
   return;
} /* communication_setup() */

/****** qmaster/set_message_priorities() *********************************************
*  NAME
*     set_message_priorities() -- set message priorities for commd
*
*  SYNOPSIS
*     static void set_message_priorities(const char* thePriorities)
*
*  FUNCTION
*     Message priorities are used by 'commd' to determine the sequence of message
*     delivery. Messages with higher priorities are delivered prior to messages 
*     with lower priorities.
*
*     'thePriorities' contains a blank separated list of tag id's (values
*     0-n, n = enum value of the TAG_* enum as defined in libs/gdi/sge_any_request.h
*     The position within the string determines the priority of the message tag
*     denoted by the given integer.
*
*  INPUTS
*     const char* thePriorities - blank separated list of priotities
*
*  EXAMPLE
*
*     '3 2 9'
*
*       Position  Value    Message Tag     Priority
*
*           0       3    TAG_ACK_REQUEST      0 (lowest)
*           1       2    TAG_GDI_REQUEST      1
*           2       9    TAG_SIGJOB           2 (highest)
* 
*     The old message priorities used correspond to the string '3 2'.
*
*  RESULT
*
*     void
******************************************************************************/
static void set_message_priorities(const char* thePriorities)
{
   enum { NUM_OF_PRIORITIES = 10 };

   int prio[NUM_OF_PRIORITIES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   char *str, *tok, *pos = NULL;
   int cnt = 0;

   DENTER(TOP_LAYER, "set_message_priorities");
   INFO((SGE_EVENT, MSG_SETTING_PRIORITY_TAGS_S, thePriorities));

   str = strdup(thePriorities);
   tok = strtok_r(str, " ", &pos);
   while (tok != NULL) {
      if (cnt >= (NUM_OF_PRIORITIES - 1) ) {
         WARNING((SGE_EVENT, MSG_TOO_MANY_PRIORITY_TAGS_S, thePriorities));
         break;
      }   
      prio[cnt++] = atoi(tok);
      tok = strtok_r(NULL, " ", &pos);
   }
   free(str);

   prepare_enroll(prognames[QMASTER], 1, prio);

   DEXIT;
   return;
} /* set_message_priorities */


static void daemonize_qmaster(void)
{
   DENTER(TOP_LAYER, "daemonize_qmaster");

   if (!getenv("SGE_ND")) {
      fd_set fds;
      int fd;
      lList *answer_list = NULL;

      /* close database and reopen it in child */
      spool_shutdown_context(&answer_list, spool_get_default_context());
      answer_list_output(&answer_list);

      FD_ZERO(&fds);
      fd = commlib_state_get_sfd();
      if (fd>=0) { FD_SET(fd, &fds); }
      sge_daemonize((commlib_state_get_closefd() ? NULL : &fds));

      spool_startup_context(&answer_list, spool_get_default_context(), true);
      answer_list_output(&answer_list);
   }


   DEXIT;
   return;
} /* daemonize_qmaster() */

/*
 * qmaster exit function
 */
static void qmaster_shutdown(int anExitValue)
{
   DENTER(TOP_LAYER, "qmaster_shutdown");

   sge_gdi_shutdown();

   DEXIT;
   return;
} /* qmaster_shutdown */

/*
 * qmaster exit function. This version MUST NOT be used, if the current working
 * directory is NOT the spool directory. Other components do rely on finding the
 * lock file in the spool directory.
 */
static void qmaster_lock_and_shutdown(int anExitValue)
{
   DENTER(TOP_LAYER, "qmaster_lock_and_shutdown");
   
   if (qmaster_lock(QMASTER_LOCK_FILE) == -1) {
      CRITICAL((SGE_EVENT, MSG_QMASTER_LOCKFILE_ALREADY_EXISTS));
   }
   sge_gdi_shutdown();

   DEXIT;
   return;
} /* qmaster_lock_and_shutdown */


static void process_cmdline(char **anArgv)
{
   lList *alp, *pcmdline, *ref_list;
   lListElem *aep;
   u_long32 help = 0;

   DENTER(TOP_LAYER, "process_cmdline");

   alp = pcmdline = ref_list = NULL;

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

   alp = parse_qmaster(&pcmdline, &ref_list, &help);
   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      lFreeList(ref_list);
      SGE_EXIT(1);
   }

   if(help) {
      /* user wanted to see help. we can exit */
      lFreeList(pcmdline);
      lFreeList(ref_list);
      SGE_EXIT(0);
   }

   DEXIT;
   return;
} /* process_cmdline */


static lList *parse_cmdline_qmaster(char **argv, lList **ppcmdline )
{
   char **sp;
   char **rp;
   stringT str;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "parse_cmdline_qmaster");

   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -nostart-commd */
      if ((rp = parse_noopt(sp, "-nostart-commd", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -s */
      if ((rp = parse_noopt(sp, "-s", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -lj */
      if ((rp = parse_until_next_opt(sp, "-lj", NULL, ppcmdline, &alp)) != sp)
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
}


static lList *parse_qmaster(lList **ppcmdline, lList **ppreflist, u_long32 *help )
{
   stringT str;
   lList *alp = NULL;
   int usageshowed = 0;
   u_long32 flag;
   char *filename;

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

      /* -nostart-commd */
      if(parse_flag(ppcmdline, "-nostart-commd", &alp, &flag)) {
         start_commd = false;
         continue;
      }

      /* -s */
      if(parse_flag(ppcmdline, "-s", &alp, &flag)) {
         sge_silent_set(1);
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
      sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
      if(!usageshowed)
         sge_usage(stderr);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}


void sge_gdi_kill_master(char *host, sge_gdi_request *request, sge_gdi_request *answer)
{
   uid_t uid;
   gid_t gid;
   char username[128];
   char groupname[128];

   DENTER(GDI_LAYER, "sge_gdi_kill_master");
   if (sge_get_auth_info(request, &uid, username, &gid, groupname) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   if (!manop_is_manager(username)) {
      ERROR((SGE_EVENT, MSG_SHUTDOWN_SHUTTINGDOWNQMASTERREQUIRESMANAGERPRIVILEGES));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   /* do it */
   shut_me_down = 1;
      
   INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, username, host, prognames[QMASTER]));
   answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
}


/*-------------------------------------------------------------------
 * increment_heartbeat
 *    - increment heartbeat each 30 seconds
 *    - delete lock file
 *-------------------------------------------------------------------*/
void increment_heartbeat(time_t now) 
{
   static time_t last = 0;

   DENTER(TOP_LAYER, "increment_heartbeat");

   if (last > now)
      last = now;

   /* increment heartbeat file and remove lock file */
   if (now - last >= 30) {
      DPRINTF(("increment heartbeat file\n"));
      last = now;
      qmaster_unlock(QMASTER_LOCK_FILE);
      if (inc_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE))
         ERROR((SGE_EVENT, MSG_HEARTBEAT_FAILEDTOINCREMENTHEARBEATFILEXINSPOOLDIR_S,
               QMASTER_HEARTBEAT_FILE));
   }
   DEXIT;
   return;
} /* increment_heartbeat */
 

#ifndef __SGE_NO_USERMAPPING__
/****** src/sge_map_gdi_request() *********************************************
*  NAME
*     sge_map_gdi_request() -- user mapping on gdi request
*
*  SYNOPSIS
*     bool sge_map_gdi_request(sge_gdi_request *pApiRequest);
*
*
*  FUNCTION
*     This function is called from the qmaster when he receives an
*     gdi request of an gdi client. If a guilty user mapping is
*     defined in the user mapping entry list then the user name
*     is mapped.
*
*  INPUTS
*     sge_gdi_request* pApiRequest - pointer to gdi request struct
*
*  RESULT
*     true  - user was mapped and pApiRequest is changed
*     false - pApiRequest is unchanged, no guilty user mapping entry
******************************************************************************/
bool sge_map_gdi_request(sge_gdi_request *pApiRequest)
{
   const char *mapped_user = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(TOP_LAYER,"sge_map_gdi_request" );


   if ((pApiRequest == NULL)) {
      DEXIT;
      return false;
   }

   if (sge_get_auth_info(pApiRequest, &uid, user, &gid, group) == -1) {
      DEXIT;
      return false;
   }

   if ((user == NULL) || (pApiRequest->host == NULL)) {
      DEXIT;
      return false;
   }

   cuser_list_map_user(*(cuser_list_get_master_list()), NULL,
                       user, pApiRequest->host, &mapped_user);

   DPRINTF(("master mapping: user %s from host %s mapped to %s\n",
             user, pApiRequest->host, mapped_user));

   if (!strcmp(user, mapped_user)) {
      DEXIT;
      return false;
   }

   if (sge_set_auth_info(pApiRequest, uid, 
                         (char*)mapped_user, gid, group) == -1) {
      DEXIT;
      return false;
   }

   DEXIT;
   return true;
} 
#endif
