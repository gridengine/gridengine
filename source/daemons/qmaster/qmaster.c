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
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


#ifdef QIDL
#include <pthread.h>
#include "qidl_setup.h"
#include "qidl_c_gdi.h"
#endif

#include "basis_types.h"
#include "sge.h"
#include "def.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "sge_time.h"
#include "sge_timestop.h"
#include "sge_string.h"
#include "sge_log_pid.h"
#include "sge_mkdir.h"
#include "sge_daemonize.h"
#include "sge_me.h"
#include "sge_conf.h"
#include "configuration_qmaster.h"
#include "sge_all_listsL.h"
#include "sge_c_gdi.h"
#include "sge_m_event.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_queue_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "qm_name.h"
#include "qmaster_to_execd.h"
#ifdef PW
#include "qmaster_license.h"
#endif
#include "startprog.h"
#include "qmaster_running.h"
#include "qmaster_heartbeat.h"
#include "lock.h"
#include "job_report_qmaster.h"
#include "shutdown.h"
#include "parse.h"
#include "job_log.h"
#include "opt_silent.h"
#include "opt_history.h"
#include "usage.h"
#include "setup_qmaster.h"
#include "ck_to_do_qmaster.h"
#include "sec.h"
#include "sge_prognames.h"
#include "sched_conf_qmaster.h"
#include "qmaster.h"
#include "sge_feature.h"
#include "sge_user_mapping.h"
#include "sge_groups.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "sge_language.h"
#include "sge_bitop.h"
#include "setup_path.h"
#include "sge_dirent.h"
#include "sge_security.h"
#include "read_write_host.h"
#include "complex_history.h"

#ifdef PW
/* The license key - to be replaced when serialized */
static char key[KEYSIZE]= { (char) 0x11, (char) 0x19, (char) 0xf8,
                            (char) 0x3a, (char) 0x2c, (char) 0x08,
                            (char) 0x19, (char) 0x0b
                          };
#endif
#ifdef QIDL
static pthread_t        corba_thread;
#endif

int in_spool_dir = 0;                 /* to prevent lock file writing */

/*
** the global lists of the master referenced somewhere else
*/
lList *Master_Adminhost_List           = NULL;
lList *Master_Calendar_List            = NULL;
lList *Master_Ckpt_List                = NULL;
lList *Master_Complex_List             = NULL;
lList *Master_Config_List              = NULL;
lList *Master_Exechost_List            = NULL;
lList *Master_Feature_Set_List         = NULL;
lList *Master_Job_List                 = NULL;
lList *Master_Job_Schedd_Info_List     = NULL;
lList *Master_Manager_List             = NULL;
lList *Master_Operator_List            = NULL;
lList *Master_Pe_List                  = NULL;
lList *Master_Project_List             = NULL;
lList *Master_Queue_List               = NULL;
lList *Master_Sched_Config_List        = NULL;
lList *Master_Sharetree_List           = NULL;
lList *Master_Submithost_List          = NULL;
lList *Master_User_List                = NULL;
lList *Master_Userset_List             = NULL;
lList *Master_Zombie_List              = NULL;

#ifndef __SGE_NO_USERMAPPING__
lList *Master_Host_Group_List          = NULL;
lList *Master_Usermapping_Entry_List   = NULL;
#endif

void sge_c_ack(char *host, char *commproc, sge_pack_buffer *pb);
static void qmaster_exit_func(int i);
static void sge_c_report(char *, char *, int, lList *);
static void makedirs(void);
static void increment_heartbeat(time_t);

static lList *sge_parse_cmdline_qmaster(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qmaster(lList **ppcmdline, lList **ppreflist, u_long32 *help);
static int update_license_data(lListElem *hep, lList *lp_lic); 

extern char **environ;

#define TIMELEVEL 0

int main(int argc, char *argv[]);

/**********************************************************************/
int main(
int argc,
char **argv 
) {
   sge_pack_buffer pb;
   sge_gdi_request *gdi = NULL;
   sge_gdi_request *ar = NULL;
   sge_gdi_request *an = NULL;
   sge_gdi_request *answer = NULL;
   lList *report_list = NULL;
   int i, tag;
   char host[MAXHOSTLEN];
   char commproc[MAXCOMPONENTLEN];
   u_short id;
   int status, old_timeout;
   int priority_tags[10];
#ifdef QIDL
   char **myargv;
   int myargc;
#endif
   int enrolled, rcv_timeout;
   time_t now, next_flush;
#ifdef PW   
   char pw[PWSIZE];
   int pw_print;
   char *pw_file;
   int license_invalid;
   int ckpt_mode, pw_ckpt;
#endif   
   lList *ref_list = NULL, *alp = NULL, *pcmdline = NULL;
   lListElem *aep;
   u_long32 help = 0;
#ifdef PW   
   int mode_guess;
#endif
   DENTER_MAIN(TOP_LAYER, "qmaster");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */
#ifdef PW 
   if ((mode_guess = product_mode_guess(argv[0])) == M_INVALID) {
      fprintf(stderr, MSG_STARTUP_PROGRAMCALLEDWITHINVALIDNAME_S, argv[0] ? argv[0] : MSG_SMALLNULL);
      exit(1);
   } 

#endif   
   /* This needs a better solution */
   umask(022);
   
   /* Initialize path for temporary logging until we chdir to spool */
   error_file = TMP_ERR_FILE_QMASTER;

#ifdef QIDL
   commlib_init();
#endif

   sge_setup(QMASTER, NULL);
   memset(priority_tags, 0, sizeof(priority_tags));
   priority_tags[0] = TAG_ACK_REQUEST;
   priority_tags[1] = TAG_GDI_REQUEST;
   prepare_enroll(prognames[QMASTER], 1, priority_tags);

   /* marker for signal handler for SIGTERM and SIGINT to exit immediately 
    * must be set before installing signal handlers 
    */
   in_main_loop = 0;
   
   install_exit_func(qmaster_exit_func);
   sge_setup_sig_handlers(QMASTER);
   
   lInit(nmv);

#ifdef PW
   /*
   ** show key
   */
   if ((argc == 2) && !strcmp(argv[1],"-sk")) {
      print_area(NULL, "", key, KEYSIZE);
      SGE_EXIT(0);
   }

   /*
   ** show-license
   */
   if ((argc == 2 || argc == 3) && !strcmp(argv[1],"-show-license")) {
      pw_print = 1;
      pw_file = argv[2];
   }
   else {
      pw_print = 0;
      pw_file = NULL;
   }

   /* Exits if license is only printed or invalid */
   check_license_info(sge_get_gmt(), pw, key, pw_print, pw_file);
   if (get_product_mode() != mode_guess) {
      CRITICAL((SGE_EVENT, MSG_STARTUP_PROGRAMDOESNOTMATCHPRODUCTMODE_S, argv[0]));
      SGE_EXIT(1);
   }      
#endif

   alp = sge_parse_cmdline_qmaster(&argv[1], environ, &pcmdline);
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

   alp = sge_parse_qmaster(&pcmdline, &ref_list, &help);
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


   /* Exits if there is a running qmaster or communication problem
    * Will enroll() to host of act_qmaster file, which may be the 
    * current host
    * Side effect is, that we know about a commd on the local host
    * which need not to be started in this case
    */
   enrolled = check_for_running_qmaster();

   /* --- Here we think we are the only qmaster in the system --- */
   
   set_commlib_param(CL_P_COMMDHOST, 0, me.qualified_hostname, NULL);
   set_commlib_state_logging_function(sge_log);     
   
   /* If we are enrolled there is no need to start a commd */
   if (!enrolled) {
      int priority_tags[10];
      int ret;

      memset(priority_tags, 0, sizeof(priority_tags));
      priority_tags[0] = TAG_ACK_REQUEST;
      priority_tags[1] = TAG_GDI_REQUEST;

      set_commlib_param(CL_P_NAME, 0, prognames[QMASTER], NULL);
      set_commlib_param(CL_P_ID, 1, NULL, NULL);
      set_commlib_param(CL_P_PRIO_LIST, 0, NULL, priority_tags); 

      if ((ret = enroll())) {
         DTRACE;
      
         if (ret == CL_CONNECT && start_commd) {
            startprog(argv[0], NULL, SGE_COMMD, NULL);
            sleep(5);
         }
         else if (ret == COMMD_NACK_CONFLICT) {
            ERROR((SGE_EVENT, MSG_SGETEXT_COMMPROC_ALREADY_STARTED_S, prognames[me.who]));
            SGE_EXIT(1);
         }
         else {
            ERROR((SGE_EVENT, MSG_COMMD_CANTENROLLTOCOMMD_S, cl_errstr(ret)));
            SGE_EXIT(1);
         }             


      } else {
         WARNING((SGE_EVENT, MSG_COMMD_FOUNDRUNNINGCOMMDONLOCALHOST));
      }
   }

   INFO((SGE_EVENT, MSG_STARTUP_BEGINWITHSTARTUP));

   if (sge_setup_qmaster()) {
      CRITICAL((SGE_EVENT, MSG_STARTUP_SETUPFAILED));
      SGE_EXIT(1);
   }

   /* Make essential non spool directories: $CELL/common/... */
   makedirs();

#ifdef PW
   /* check here ckpt sge_setup_qmaster reads in the lists first */
   ckpt_mode = set_licensed_feature("ckpt");
   pw_ckpt = check_ckpt_lic(ckpt_mode, 1);
   
   if (pw_ckpt) {
      SGE_EXIT(1);
   }
#endif

   /* Now we are in the spool directory, we may create the lock file in case of exit */
   in_spool_dir = 1;
   
   if (!getenv("SGE_ND")) {
      fd_set fds;
      int fd;
      FD_ZERO(&fds);
      fd = get_commlib_state_sfd();
      if (fd>=0) {
         FD_SET(fd, &fds);
      }
      sge_daemonize(get_commlib_state_closefd()?NULL:&fds);
   }

#ifdef QIDL
   if(!qidl_init()) {
      CRITICAL((SGE_EVENT, MSG_QIDL_CANTINITIALIZEQIDL));
      SGE_EXIT(1);
   }
   lock_master();
   {
      struct arguments args;
      args.argc = myargc;
      args.argv = myargv;
      if(pthread_create(&corba_thread, NULL, start_corba_thread, &args)) {
         CRITICAL((SGE_EVENT, MSG_QIDL_CANTSPAWNTHREADFORCORBASERVER));
         SGE_EXIT(1);
      }
   }
#endif

   sge_setup_sig_handlers(QMASTER);

   sge_log_pid(QMASTER_PID_FILE);

   /* don't wanna get old messages */
   remove_pending_messages(NULL, 0, 0, 0);

   /* commlib call to mark all commprocs as unknown */
   reset_last_heard();

   /* initiate a transition from SGE to SGE3E mode for all execd's */
   master_notify_execds();

  
   INFO((SGE_EVENT, MSG_STARTUP_STARTINGUP_S, feature_get_product_name(FS_VERSION)));

   in_main_loop = 1;

#if RAND_ERROR
   rand_error = 1;
#endif

   while (TRUE) {

      now = (time_t) sge_get_gmt();

      increment_heartbeat(now);

#ifdef PW
      license_invalid = check_license_info(now, pw, key, 0, NULL);
        
      shut_me_down = shut_me_down || license_invalid;
#endif

      if (dead_children) {
         /* do a wait to our children so that os can clean up */
         while (waitpid(-1, &status, WNOHANG) > 0)
                  ;
         dead_children = 0;
      }

      if (shut_me_down) {
         /* slowly! we have to deliver events before shutting down */
         sge_add_event(NULL, sgeE_QMASTER_GOES_DOWN, 0, 0, NULL, NULL);
         sge_flush_events(NULL, FLUSH_EVENTS_SET);
         scheduler_busy = 0;
         ck_4_deliver_events(now);
#ifdef QIDL
         unlock_master();
         shutdownQIDL();
         pthread_join(corba_thread, NULL);
#endif
         sge_shutdown();
      }

      starttime(TIMELEVEL);
      
      sge_ck_to_do_qmaster(0);

      log_time(TIMELEVEL, "check to do:");

      DPRINTF(("===========================[EPOCH]=====================================\n"));

      starttime(TIMELEVEL);
      
      host[0] = '\0';
      commproc[0] = '\0';
      id = 0;
      tag = 0; /* we take everyting */
      
#ifdef QIDL
      unlock_master();
#endif

      now = sge_get_gmt();
      next_flush = sge_next_flush(now);
      
      old_timeout = get_commlib_state_timeout_srcv();

      if (next_flush && ((next_flush - now) >= 0))
         rcv_timeout = MIN(MAX(next_flush - now, 2), 20);
      else
         rcv_timeout = 20;
         
      set_commlib_param(CL_P_TIMEOUT_SRCV, rcv_timeout, NULL, NULL);
  
      DPRINTF(("setting sync receive timeout to %d seconds\n", rcv_timeout));

      i = sge_get_any_request(host, commproc, &id, &pb, &tag, 1);

      set_commlib_param(CL_P_TIMEOUT_SRCV, old_timeout, NULL, NULL);

#ifdef QIDL
      lock_master();
#endif

      if (sigpipe_received) {
         sigpipe_received = 0;
         INFO((SGE_EVENT, "SIGPIPE received"));
      }
      
      if (i != CL_OK) {
         log_time(TIMELEVEL, "sge_get_any_request != 0");
         
         if ( i != COMMD_NACK_TIMEOUT ) {
            DPRINTF(("Problems reading request: %s\n", cl_errstr(i)));
            sleep(2);
         }
         continue;              
      }
      else {
         log_time(TIMELEVEL, "sge_get_any_request == 0");
         starttime(TIMELEVEL);
      }

      switch (tag) {

      /* ======================================== */
#ifdef SECURE
      case TAG_SEC_ANNOUNCE:    /* completly handled in libsec  */
         clear_packbuffer(&pb);
         log_time(TIMELEVEL, "request handling SEC_ANNOUNCE");
         break;
#endif

      /* ======================================== */
      case TAG_GDI_REQUEST:

         if (sge_unpack_gdi_request(&pb, &gdi)) {
            ERROR((SGE_EVENT,MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI,
               host, commproc, (int)id));
            clear_packbuffer(&pb);   
            break;
         }
         clear_packbuffer(&pb);

         for (ar = gdi; ar; ar = ar->next) { 
             
            /* use id, commproc and host for authentication */  
            ar->id = id;
            ar->commproc = sge_strdup(NULL, commproc); 
            ar->host = sge_strdup(NULL, host);

#ifndef __SGE_NO_USERMAPPING__
            /* perform administrator user mapping with sge_gdi_request
               structure, this can change the user name (ar->user) */
            if (Master_Usermapping_Entry_List != NULL) {
              sge_map_gdi_request(Master_Host_Group_List, Master_Usermapping_Entry_List,ar);
            }       
#endif
            if (ar == gdi) {
               answer = an = new_gdi_request();
            }
            else {
               an->next = new_gdi_request();
               an = an->next;
            }

            sge_c_gdi(host, ar, an);
         }

         sge_send_gdi_request(0, host, commproc, id, answer);
         answer = free_gdi_request(answer);
         gdi = free_gdi_request(gdi);
         log_time(TIMELEVEL, "request handling GDI_REQUEST");

         break;
      /* ======================================== */
      case TAG_ACK_REQUEST:

         DPRINTF(("SGE_ACK_REQUEST(%s/%s/%d)\n", host, commproc, id));

         sge_c_ack(host, commproc, &pb);
         clear_packbuffer(&pb);
         log_time(TIMELEVEL, "request handling ACK_REQUEST");
         break;

      /* ======================================== */
      case TAG_EVENT_CLIENT_EXIT:

         DPRINTF(("SGE_EVENT_CLIENT_EXIT(%s/%s/%d)\n", host, commproc, id));

         sge_event_client_exit(host, commproc, &pb);
         clear_packbuffer(&pb);
         log_time(TIMELEVEL, "request handling EVENT_CLIENT_EXIT");
         break;

      /* ======================================== */
      case TAG_REPORT_REQUEST: 

         DPRINTF(("SGE_REPORT(%s/%s/%d)\n", host, commproc, id));

         if (cull_unpack_list(&pb, &report_list)) {
            ERROR((SGE_EVENT,MSG_CULL_FAILEDINCULLUNPACKLISTREPORT));
            break;
         }
         clear_packbuffer(&pb);

         sge_c_report(host, commproc, id, report_list);
         lFreeList(report_list);
         report_list = NULL;
         log_time(TIMELEVEL, "request handling REPORT");
         break;

      /* ======================================== */
      default:
         DPRINTF(("***** UNKNOWN TAG TYPE %d\n", tag));
         clear_packbuffer(&pb);
      }
   }
}

/*************************************************************
 Function installed to be called just before exit() is called.
 clean up
 *************************************************************/
static void qmaster_exit_func(
int i 
) {
   if (in_spool_dir)
      qmaster_lock(QMASTER_LOCK_FILE); /* tell shadows of normal shutdown */

   sge_gdi_shutdown();

#ifdef QIDL
   unlock_master();
   shutdownQIDL();
   pthread_join(corba_thread, NULL);
#endif
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
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (sge_manager(username)) {
      ERROR((SGE_EVENT, MSG_SHUTDOWN_SHUTTINGDOWNQMASTERREQUIRESMANAGERPRIVILEGES));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   /* do it */
   shut_me_down = 1;
   sge_flush_events(NULL, FLUSH_EVENTS_SET);
      
   INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, username, host, prognames[QMASTER]));
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
}


/*
** NAME
**   update_license_data
** PARAMETER
**   hep                 - pointer to host element, EH_Type
**   lp_lic              - list of license data, LIC_Type
**
** RETURN
**    0                  - ok
**   -1                  - NULL pointer received for hep
**   -2                  - NULL pointer received for lp_lic
** EXTERNAL
**   none
** DESCRIPTION
**   updates the number of processors in the host element
**   spools and writes history if it has changed
*/
static int update_license_data(lListElem *hep, lList *lp_lic)
{
   u_long32 processors, old_processors;

   DENTER(TOP_LAYER, "update_license_data");

   if (!hep) {
      DEXIT;
      return -1;
   }

   /*
   ** if it was clear what to do in this case we could return 0
   */
   if (!lp_lic) {
      DEXIT;
      return -2;
   }

   /*
   ** at the moment only the first element is evaluated
   */
   processors = lGetUlong(lFirst(lp_lic), LIC_processors);
   /* arch = lGetString(lFirst(lp_lic), LIC_arch); */ 

   old_processors = lGetUlong(hep, EH_processors);
   lSetUlong(hep, EH_processors, processors);
   /*
   ** we spool and write history, cf. cod_update_load_values()
   */
   if (processors != old_processors) {
      DPRINTF(("%s has " u32 " processors\n",
         lGetHost(hep, EH_name), processors));
      write_host(1, 2, hep, EH_name, NULL);
      if (!is_nohist()) {
         write_host_history(hep);
      }
   }
   DEXIT;
   return 0;
}       

/*-------------------------------------------------------------------------*/
static void sge_c_report(
char *rhost,
char *commproc,
int id,
lList *report_list 
) {
   lListElem *hep = NULL;
   u_long32 rep_type;
   lListElem *report;
   int ret = 0, this_seqno, last_seqno;
   u_long32 rversion;

   DENTER(TOP_LAYER, "sge_c_report");

   if (!lGetNumberOfElem(report_list)) {
      DPRINTF(("received empty report\n"));
      DEXIT;
      return;
   }

   /* accept reports only from execd's */
   if (strcmp(prognames[EXECD], commproc)) {
      ERROR((SGE_EVENT, MSG_GOTSTATUSREPORTOFUNKNOWNCOMMPROC_S, commproc));
      DEXIT;
      return;
   }
  
   /* need exec host for all types of reports */
   if (!(hep = sge_locate_host(rhost, SGE_EXECHOST_LIST))) {
      ERROR((SGE_EVENT, MSG_GOTSTATUSREPORTOFUNKNOWNEXECHOST_S, rhost));
      DEXIT;
      return;
   }

   /* do not process load reports from old execution daemons */
   rversion = lGetUlong(lFirst(report_list), REP_version);
   if (verify_request_version(NULL, rversion, rhost, commproc, id)) {
      DEXIT;
      return;
   }

   /* prevent old reports being proceeded 
      frequent loggings of outdated reports can be an indication 
      of too high message traffic arriving at qmaster */ 
   this_seqno = lGetUlong(lFirst(report_list), REP_seqno);
   last_seqno = lGetUlong(hep, EH_report_seqno);

   if ((this_seqno < last_seqno && (last_seqno - this_seqno) <= 9000) &&
      !(last_seqno > 9990 && this_seqno < 10)) {
      /* this must be an old report, log and then ignore it */
      DPRINTF(("received old load report (%d < %d) from exec host %s\n", 
         this_seqno, last_seqno+1, rhost));
      DEXIT;
      return;
   }
   lSetUlong(hep, EH_report_seqno, this_seqno);

   /*
   ** process the reports one after the other
   ** usually there will be a load report
   ** and a configuration version report
   */

   for_each(report, report_list) {

      rep_type = lGetUlong(report, REP_type);

      switch (rep_type) {
      case NUM_REP_REPORT_LOAD:
      
         /* Now handle execds load reports */
         sge_update_load_values(rhost, lGetList(report, REP_list));

         break;
      case NUM_REP_REPORT_CONF:

         if (hep && !is_configuration_up_to_date(hep, Master_Config_List, 
                                          lGetList(report, REP_list))) {
            DPRINTF(("configuration on host %s is not up to date\n", rhost));

            if (!strcmp(prognames[EXECD], commproc))
               ret = notify_new_conf_2_execd(hep);

            ret = notify_new_conf_2_execd(hep);

            if (ret) {
               ERROR((SGE_EVENT, MSG_CONF_CANTNOTIFYEXECHOSTXOFNEWCONF_S, rhost));
               break;
            }
         }
         break;
         
      case NUM_REP_REPORT_PROCESSORS:
         /*
         ** save number of processors
         */
         ret = update_license_data(hep, lGetList(report, REP_list));
         if (ret) {
            ERROR((SGE_EVENT, MSG_LICENCE_ERRORXUPDATINGLICENSEDATA_I, ret));
            break;
         }

         break;

      case NUM_REP_REPORT_JOB:

         {
            sge_pack_buffer pb;

            if(init_packbuffer(&pb, 1024, 0) == PACK_SUCCESS) {
               process_job_report(report, hep, rhost, commproc, &pb);

               if (pb_filled(&pb)) {
                  /* send all stuff packed during processing to execd */
                  sge_send_any_request(0, NULL, rhost, commproc, id, &pb, 
                                       TAG_ACK_REQUEST); 
               }
               clear_packbuffer(&pb);
            }
         }
         break;

      default:   
         DPRINTF(("received invalid report type %ld\n", rep_type));
         DEXIT;
         return;
      }
   } /* end for_each */

   
   DEXIT;
   return;
}

/*-----------------------------------------------------------------------
 * makedirs
 * Make directory $SGE_CELL/common and $SGE_CELL/common/local_conf
 * sge_mkdir() implictely works recursive
 *------------------------------------------------------------------------*/
static void makedirs()
{
   DENTER(TOP_LAYER, "makedirs");
   
   /* Implicitely creats cell_root and common dir */
   sge_mkdir(path.local_conf_dir, 0755, 1);
   
   DEXIT;
}

/*-------------------------------------------------------------------
 * increment_heartbeat
 *    - increment heartbeat each 30 seconds
 *    - delete lock file
 *-------------------------------------------------------------------*/
static void increment_heartbeat(
time_t now 
) {
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
}

/****
 **** sge_parse_cmdline_qmaster (static)
 ****/
static lList *sge_parse_cmdline_qmaster(
char **argv,
char **envp,
lList **ppcmdline 
) {
char **sp;
char **rp;
stringT str;
lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_parse_cmdline_qmaster");

   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -nohist */
      if ((rp = parse_noopt(sp, "-nohist", NULL, ppcmdline, &alp)) != sp)
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

      /* -OA*  for qidl */
      if ((rp = parse_until_next_opt(sp, "-OA*", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -ORB*  for qidl */
      if ((rp = parse_until_next_opt(sp, "-ORB*", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      printf("%s\n", *sp);
      sge_usage(stderr);
      sge_add_answer(&alp, str, STATUS_ESEMANTIC, 0);
      DEXIT;
      return alp;
   }

   DEXIT;
   return alp;
}

/****
 **** sge_parse_qmaster (static)
 ****/
static lList *sge_parse_qmaster(
lList **ppcmdline,
lList **ppreflist,
u_long32 *help 
) {
stringT str;
lList *alp = NULL;
int usageshowed = 0;
u_long32 flag;
char *filename;

   DENTER(TOP_LAYER, "sge_parse_qmaster");

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
      /* -nohist */
      if(parse_flag(ppcmdline, "-nohist", &alp, &flag)) {
         set_nohist(1);
         continue;
      }

      /* -nostart-commd */
      if(parse_flag(ppcmdline, "-nostart-commd", &alp, &flag)) {
         start_commd = FALSE;
         continue;
      }

      /* -s */
      if(parse_flag(ppcmdline, "-s", &alp, &flag)) {
         set_silent(1);
         continue;
      }
      
      /* -lj */
      if(parse_string(ppcmdline, "-lj", &alp, &filename)) {
         enable_job_logging(filename);
         FREE(filename);
         continue;
      }

      /* -OA* */
      if(parse_flag(ppcmdline, "-OA*", &alp, &flag)) {
         continue;
      }

      /* -ORB* */
      if(parse_flag(ppcmdline, "-ORB*", &alp, &flag)) {
         continue;
      }
   }

   if(lGetNumberOfElem(*ppcmdline)) {
      sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
      if(!usageshowed)
         sge_usage(stderr);
      sge_add_answer(&alp, str, STATUS_ESEMANTIC, 0);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}
 
