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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "sge_bootstrap.h"
#include "sge_stdio.h"
#include "sge_unistd.h"
#include "sge.h"
#include "sge_prog.h"
#include "setup.h"
#include "qm_name.h"
#include "sig_handlers.h"
#include "qmaster_heartbeat.h"
#include "lock.h"
#include "startprog.h"
#include "sge_hostname.h"
#include "sge_any_request.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time.h"
#include "commlib.h"
#include "spool/classic/rw_configuration.h"
#include "sge_all_listsL.h"
#include "sge_feature.h"
#include "setup_path.h"
#include "sge_os.h"
#include "shutdown.h"
#include "sge_uidgid.h"
#include "sge_language.h"
#include "sge_string.h"
#include "usage.h"
#include "sge_io.h"
#include "sge_spool.h"
#include "sge_mt_init.h"

#include "msg_common.h"
#include "msg_daemons_common.h"
#include "msg_shadowd.h"

#ifndef FALSE
#   define FALSE 0
#endif

#ifndef TRUE
#   define TRUE  1
#endif

#define CHECK_INTERVAL      60 
#define GET_ACTIVE_INTERVAL 240
#define DELAY_TIME          600 

static int check_interval = CHECK_INTERVAL;
static int get_active_interval = GET_ACTIVE_INTERVAL;
static int delay_time = DELAY_TIME;

char binpath[SGE_PATH_MAX];
char oldqmaster[SGE_PATH_MAX];

char shadow_err_file[SGE_PATH_MAX];
char qmaster_out_file[SGE_PATH_MAX];

int main(int argc, char **argv);
static void shadowd_exit_func(int i);
static int check_if_valid_shadow(const char *shadow_master_file);
static int compare_qmaster_names(char *);
static int host_in_file(const char *, const char *);
static void parse_cmdline_shadowd(int argc, char **argv);
static int shadowd_is_old_master_enrolled(char *oldqmaster);

static int shadowd_is_old_master_enrolled(char *oldqmaster)
{
   cl_com_handle_t* handle = NULL;
   cl_com_SIRM_t* status = NULL;
   int ret;
   int is_up_and_running = 0;

   DENTER(TOP_LAYER, "shadowd_is_old_master_enrolled");

   handle=cl_com_create_handle(CL_CT_TCP,CL_CM_CT_MESSAGE , 0, sge_get_qmaster_port() ,(char*)prognames[SHADOWD] , 0, 1,0 );
   if (handle == NULL) {
      CRITICAL((SGE_EVENT,"could not create communication handle\n"));
      DEXIT;
      return is_up_and_running;
   }

   DPRINTF(("Try to send status information message to previous master host "SFQ" to port %ld\n", oldqmaster, sge_get_qmaster_port() ));
   ret = cl_commlib_get_endpoint_status(handle,oldqmaster ,(char*)prognames[QMASTER] , 1, &status);
   if (ret != CL_RETVAL_OK) {
      DPRINTF(("cl_commlib_get_endpoint_status() returned "SFQ"\n", cl_get_error_text(ret)));
      is_up_and_running = 0;
      DPRINTF(("old qmaster not responding - No master found\n"));   
   } else {
      DPRINTF(("old qmaster is still running\n"));   
      is_up_and_running = 1;
   }

   if (status != NULL) {
      DPRINTF(("endpoint is up since %ld seconds and has status %ld\n", status->runtime, status->application_status));
      cl_com_free_sirm_message(&status);
   }
 
   cl_commlib_shutdown_handle(handle,0);

   DEXIT;
   return is_up_and_running;
}

/*----------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int heartbeat, last_heartbeat, latest_heartbeat, ret, delay;
   time_t now, last;
   const char *cp;
   fd_set fds;
   char err_str[1024];
   char shadowd_pidfile[SGE_PATH_MAX];
   dstring ds;
   char buffer[256];

   DENTER_MAIN(TOP_LAYER, "sge_shadowd");
   
   sge_mt_init();

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   /* initialize recovery control variables */
   {
      char *s;
      int val;
      if ((s=getenv("SGE_CHECK_INTERVAL")) &&
          sscanf(s, "%d", &val) == 1)
         check_interval = val;
      if ((s=getenv("SGE_GET_ACTIVE_INTERVAL")) &&
          sscanf(s, "%d", &val) == 1)
         get_active_interval = val;
      if ((s=getenv("SGE_DELAY_TIME")) &&
          sscanf(s, "%d", &val) == 1)
         delay_time = val;
   }
         
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

   log_state_set_log_file(TMP_ERR_FILE_SHADOWD);

   /* minimal setup */
   sge_setup(SHADOWD, NULL);

   /* is there a running shadowd on this host */
   {
      const char *conf_string;
      pid_t shadowd_pid;

      if ((conf_string = bootstrap_get_qmaster_spool_dir())) {
         sprintf(shadowd_pidfile, "%s/"SHADOWD_PID_FILE,
            conf_string, uti_state_get_unqualified_hostname());
         DPRINTF(("pidfilename: %s\n", shadowd_pidfile));
         if ((shadowd_pid = sge_readpid(shadowd_pidfile))) {
            char *shadowd_name;

            DPRINTF(("shadowd_pid: %d\n", shadowd_pid));
            shadowd_name = SGE_SHADOWD;

            if (!sge_checkprog(shadowd_pid, shadowd_name, PSCMD)) {
               CRITICAL((SGE_EVENT, MSG_SHADOWD_FOUNDRUNNINGSHADOWDWITHPIDXNOTSTARTING_I, (int) shadowd_pid));
               SGE_EXIT(1);
            }
         }
      }
   }

   prepare_enroll(prognames[SHADOWD]);

   uti_state_set_exit_func(shadowd_exit_func);
   sge_setup_sig_handlers(SHADOWD);

   lInit(nmv);

   parse_cmdline_shadowd(argc, argv);

   if (!(cp = bootstrap_get_qmaster_spool_dir())) {
      CRITICAL((SGE_EVENT, MSG_SHADOWD_CANTREADQMASTERSPOOLDIRFROMX_S, 
         path_state_get_bootstrap_file()));
      DEXIT;
      SGE_EXIT(1);
   }

   if (chdir(cp)) {
      CRITICAL((SGE_EVENT, MSG_SHADOWD_CANTCHANGETOQMASTERSPOOLDIRX_S, cp));
      DEXIT;
      SGE_EXIT(1);
   }

   if (sge_set_admin_username(bootstrap_get_admin_user(), err_str)) {
      CRITICAL((SGE_EVENT, err_str));
      SGE_EXIT(1);
   }

   if (sge_switch2admin_user()) {
      CRITICAL((SGE_EVENT, MSG_SHADOWD_CANTSWITCHTOADMIN_USER));
      SGE_EXIT(1);
   }

   sprintf(shadow_err_file, "messages_shadowd.%s", uti_state_get_unqualified_hostname());
   sprintf(qmaster_out_file, "messages_qmaster.%s", uti_state_get_unqualified_hostname());
   sge_copy_append(TMP_ERR_FILE_SHADOWD, shadow_err_file, SGE_MODE_APPEND);
   unlink(TMP_ERR_FILE_SHADOWD);
   log_state_set_log_as_admin_user(1);
   log_state_set_log_file(shadow_err_file);

   FD_ZERO(&fds);
   if ( cl_com_set_handle_fds(cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name() ,0), &fds) == CL_RETVAL_OK) {
      sge_daemonize(&fds);
   } else {
      sge_daemonize(NULL);
   }
   sge_write_pid(shadowd_pidfile);

   starting_up();
   
   sge_setup_sig_handlers(SHADOWD);

   last_heartbeat = get_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE);
   last = (time_t) sge_get_gmt();

   delay = 0;
   while (TRUE) {
      sleep(check_interval);

      if (shut_me_down) {
         u_long32 old_ll = log_state_get_log_level();
         log_state_set_log_level(LOG_INFO);
         INFO((SGE_EVENT, MSG_SHADOWD_CONTROLLEDSHUTDOWN_SS, 
               feature_get_product_name(FS_VERSION, &ds),
               feature_get_featureset_name(feature_get_active_featureset_id())));
         log_state_set_log_level(old_ll);
         SGE_EXIT(0);
      }   

      heartbeat = get_qmaster_heartbeat(QMASTER_HEARTBEAT_FILE);
      
      now = (time_t) sge_get_gmt();

      DPRINTF(("heartbeat: %d -- last_heartbeat: %d now-last: %d \n", 
               heartbeat, last_heartbeat, (int) (now - last)));
      
      /* We could read two times the heartbeat */
      if (last_heartbeat != -1 && 
          heartbeat != -1 && 
          (now - last >= (get_active_interval + delay))) {
         delay = 0;
         if ((last_heartbeat - heartbeat) == 0) {
            DPRINTF(("heartbeat not changed since seconds: %d\n", 
               (int) (now - last)));
            delay = delay_time;
            if (!(ret = check_if_valid_shadow(path_state_get_shadow_masters_file()))) {
               if (qmaster_lock(QMASTER_LOCK_FILE)) {
                  ERROR((SGE_EVENT, MSG_SHADOWD_FAILEDTOLOCKQMASTERSOMBODYWASFASTER));
               } else {
                  int out, err;

                  /* still the old qmaster name in act_qmaster file and 
                     still the old heartbeat */
                  latest_heartbeat = get_qmaster_heartbeat(
                     QMASTER_HEARTBEAT_FILE);
                  DPRINTF(("old qmaster name in act_qmaster and "
                     "old heartbeat\n"));
                  if (!compare_qmaster_names(oldqmaster) &&
                      !shadowd_is_old_master_enrolled(oldqmaster) && 
                      (latest_heartbeat - heartbeat == 0)) {
                     char qmaster_name[256];
                     char schedd_name[256];

                     strcpy(qmaster_name, SGE_PREFIX);
                     strcpy(schedd_name, SGE_PREFIX);
                     strcat(qmaster_name, prognames[QMASTER]); 
                     strcat(schedd_name, prognames[SCHEDD]);
                     DPRINTF(("qmaster_name: "SFN"\n", qmaster_name)); 
                     DPRINTF(("schedd_name: "SFN"\n", schedd_name)); 

                     /*
                      * open logfile as admin user for initial qmaster/schedd 
                      * startup messages
                      */
                     out = open(qmaster_out_file, O_CREAT|O_WRONLY|O_APPEND, 
                                0644);
                     err = out;
                     if (out == -1) {
                        /*
                         * First priority is the master restart
                         * => ignore this error
                         */
                        out = 1;
                        err = 2;
                     } 

                     sge_switch2start_user();
                     ret = startprog(out, err, NULL, binpath, qmaster_name, NULL);
                     sge_switch2admin_user();
                     if (ret)
                        ERROR((SGE_EVENT, MSG_SHADOWD_CANTSTARTQMASTER));
                     else {
                        sleep(5);
                        sge_switch2start_user();
                        ret = startprog(out, err, NULL, binpath, schedd_name, NULL);
                        sge_switch2admin_user();
                        if (ret)
                           ERROR((SGE_EVENT, MSG_SHADOWD_CANTSTARTQMASTER));   
                     }
                     close(out);
                  } else {
                     qmaster_unlock(QMASTER_LOCK_FILE);
                  }
               }      
            } else if (ret == -1) {
               /* just log the more important failures */    
               WARNING((SGE_EVENT, MSG_SHADOWD_DELAYINGSHADOWFUNCTIONFOR10MINUTES));
            }
         }
         /* Begin a new interval, set timers and hearbeat to current values */
         last = now;
         last_heartbeat = heartbeat;
      }
   }
}

/*-----------------------------------------------------------------
 * shadowd_exit_func
 * function installed to be called just before exit() is called.
 *-----------------------------------------------------------------*/
static void shadowd_exit_func(
int i 
) {
   exit(0);
}

/*-----------------------------------------------------------------
 * compare_qmaster_names
 * see if old qmaster name and current qmaster name are still the same
 *-----------------------------------------------------------------*/
static int compare_qmaster_names(
char *oldqmaster 
) {
 char newqmaster[SGE_PATH_MAX];
 int ret;
 
 DENTER(TOP_LAYER, "compare_qmaster_names");
 
 if (get_qm_name(newqmaster, path_state_get_act_qmaster_file(), NULL)) {
    WARNING((SGE_EVENT, MSG_SHADOWD_CANTREADACTQMASTERFILEX_S, path_state_get_act_qmaster_file())); 
    DEXIT;
    return -1;
 }
 
 ret = sge_hostcmp(newqmaster, oldqmaster);
 
 DPRINTF(("strcmp() of old and new qmaster returns: %d\n", ret));
 
 DEXIT;
 return ret;
} 
 
/*-----------------------------------------------------------------
 * check_if_valid_shadow
 * return 0 if we are a valid shadow
 *        -1 if not
 *        -2 if lock file exits or master was running on same machine
 *-----------------------------------------------------------------*/
static int check_if_valid_shadow(
const char *shadow_master_file 
) {
   struct hostent *hp;
   const char *cp, *cp2;
   char localconffile[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "check_if_valid_shadow");

   if (isLocked(QMASTER_LOCK_FILE)) {
      DPRINTF(("lock file exits\n"));
      DEXIT;
      return -2;
   }   

   /* we can't read act_qmaster file */
   if (get_qm_name(oldqmaster, path_state_get_act_qmaster_file(), NULL)) {
      WARNING((SGE_EVENT, MSG_SHADOWD_CANTREADACTQMASTERFILEX_S, path_state_get_act_qmaster_file()));
      DEXIT;
      return -1;
   }

   /* we can't resolve hostname of old qmaster */
   hp = sge_gethostbyname_retry(oldqmaster);
   if (hp == (struct hostent *) NULL) {
      WARNING((SGE_EVENT, MSG_SHADOWD_CANTRESOLVEHOSTNAMEFROMACTQMASTERFILE_SS, 
              path_state_get_act_qmaster_file(), oldqmaster));
      DEXIT;
      return -1;
   }

   /* we are on the same machine as old qmaster */
   if (!strcmp(hp->h_name, uti_state_get_qualified_hostname())) {
      sge_free_hostent(&hp);
      DPRINTF(("qmaster was running on same machine\n"));
      DEXIT;
      return -2;
   }

   sge_free_hostent(&hp);


   /* we are not in the shadow master file */
   if (host_in_file(uti_state_get_qualified_hostname(), shadow_master_file)) {
      WARNING((SGE_EVENT, MSG_SHADOWD_NOTASHADOWMASTERFILE_S, shadow_master_file));
      DEXIT;
      return -1;
   }

   /* we can't get binary path */
   if (!(cp = bootstrap_get_binary_path())) {
      WARNING((SGE_EVENT, MSG_SHADOWD_CANTREADBINARYPATHFROMX_S, path_state_get_bootstrap_file()));
      DEXIT;
      return -1;
   } else {
      sprintf(binpath, cp); /* copy global configuration path */
      sprintf(localconffile, "%s/%s", path_state_get_local_conf_dir(), uti_state_get_qualified_hostname());
      cp2 = bootstrap_get_binary_path();
      if (cp2) {
         strcpy(binpath, cp2); /* overwrite global configuration path */
         DPRINTF(("found local conf binary path:\n"));
      } else {
         DPRINTF(("global conf binary path:\n"));   
      }
      DPRINTF((""SFQ"\n", binpath));   
   }
   
   DPRINTF(("we are a candidate for shadow master\n"));

   DEXIT;
   return 0;
}

/*----------------------------------------------------------------------
 * host_in_file
 * look if resolved host is in "file"
 * return  
 *         0 if present 
 *         1 if not
 *        -1 error occured
 *----------------------------------------------------------------------*/
static int host_in_file(
const char *host,
const char *file 
) {
   FILE *fp;
   char buf[512], *cp;
   struct hostent *hp = NULL;

   DENTER(TOP_LAYER, "host_in_file");

   fp = fopen(file, "r");
   if (!fp) {
      DEXIT;
      return -1;
   }

   while (fgets(buf, sizeof(buf), fp)) {
      for (cp = strtok(buf, " \t\n,"); cp; cp = strtok(NULL, " \t\n,")) {
         hp = sge_gethostbyname_retry(cp);
         if (hp && hp->h_name) {
            if (!sge_hostcmp(host, hp->h_name)) {
               fclose(fp);
               sge_free_hostent(&hp);
               DEXIT;
               return 0;
            }
         }
         sge_free_hostent(&hp);
      }      
   }

   fclose(fp);
   return 1;
}

/*---------------------------------------------------------------------
 * parse_cmdline_shadowd
 *---------------------------------------------------------------------*/
static void parse_cmdline_shadowd(
int argc,
char **argv 
) {
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "parse_cmdline_shadowd");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   /*
   ** -help
   */
   if ((argc == 2) && !strcmp(argv[1],"-help")) {
#define PRINTITD(o,d) print_option_syntax(stdout,o,d)

      fprintf(stdout, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));

      fprintf(stdout, "%s sge_shadowd [options]\n", MSG_GDI_USAGE_USAGESTRING);

      PRINTITD(MSG_GDI_USAGE_help_OPT , MSG_GDI_UTEXT_help_OPT );
      SGE_EXIT(0);
   }

   DEXIT;
}
