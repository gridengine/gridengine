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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <unistd.h>

#include "sge_log.h"
#include "sge.h"
#include "sge_arch.h"
#include "sge_exit.h"
#include "sge_get_confval.h"
#include "sge_me.h"
#include "sge_pids.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "commlib.h"
#include "sge_conf.h"
#include "def.h"
#include "qm_name.h"
#include "qmaster_running.h"
#include "setup_path.h"
#include "msg_daemons_common.h"
#include "sge_string.h"
#include "sge_feature.h"

#define ENROLL_ERROR_DO_RETRY -50
static int qmaster_running(char *, int *);

/*-----------------------------------------------------------------------
 * qmaster_running
 * Check for running qmaster on given host
 * return  0 if no qmaster is registered at host found in act_qmaster file
 *           or act_qmaster file is not readable
 *         1 qmaster is registered at host found in act_qmaster file
 *        -1 < ENROLL_ERROR_DO_RETRY retry has no use
 *        ENROLL_ERROR_DO_RETRY we might retry
 * enrolled: 1 if we stay enrolled to a local commd
 *           0 if we are not enrolled to a commd
 *-----------------------------------------------------------------------*/
static int qmaster_running(
char *err_str,
int *enrolled 
) {
   char master[MAXHOSTLEN];
   pid_t pid;
   char pidfile[SGE_PATH_MAX], *cp;
   int ret, alive;

   DENTER(TOP_LAYER, "qmaster_running");

   *enrolled = FALSE;

   if (get_qm_name(master, path.act_qmaster_file, err_str)) {
      DEXIT;
      return 0;
   }

   /* get qmaster spool dir, try to read pidfile and check if qmaster is running */
   if (!hostcmp(master, me.qualified_hostname)) {
      DTRACE;
      if ((cp = get_confval("qmaster_spool_dir", path.conf_file))) {
         sprintf(pidfile, "%s/%s", cp, QMASTER_PID_FILE);
	      DPRINTF(("pidfilename: %s\n", pidfile));
         if ((pid = readpid(pidfile))) {
            DPRINTF(("pid: %d\n", pid));
            if (!checkprog(pid, SGE_QMASTER, PSCMD)) {
               CRITICAL((SGE_EVENT, MSG_QMASTER_FOUNDRUNNINGQMASTERWITHPIDXNOTSTARTING_I, (int) pid));
               SGE_EXIT(1);
            }
         }   
      }
   }   

   set_commlib_param(CL_P_COMMDHOST, 0, master, NULL); 


   set_commlib_param(CL_P_NAME, 0, prognames[QMASTER], NULL);
   set_commlib_param(CL_P_ID, 1, NULL, NULL);

   if (feature_is_enabled(FEATURE_RESERVED_PORT_SECURITY)) {
      set_commlib_param(CL_P_RESERVED_PORT, 1, NULL, NULL);
   }   
      

   ret = enroll();

   DPRINTF(("return of enroll(): %d - qmasterhost %s\n", ret, master));

   switch (ret) {
   case 0:
      /* We are enrolled to a commd on another host */
      /* Ask if qmaster is enrolled on that host    */
      if (hostcmp(me.qualified_hostname, master)) {
         alive = ask_commproc(master, prognames[QMASTER], 0);
         DPRINTF(("alive: %s %s %d\n", master, prognames[QMASTER], alive));
         if (alive == 0)
            ret = 1;
         else
            ret = 0;
         leave_commd();
      }
      else {
         /* do not leave if we contacted commd on local host */
         *enrolled = 1;
         ret = 0;
      }

      strcpy(err_str, master);
      break;

   case CL_CONNECT:
      /* No commd on that host - let's hope there is also no qmaster */
      strcpy(err_str, master);
      ret = 0;
      break;
   case NACK_CONFLICT:
      /* qmaster already registered on commd host, assume he is running */
      strcpy(err_str, master);
      ret = 1;
      break;
   case CL_ALREADYDONE:
      /* We are already enrolled */
      strcpy(err_str, MSG_QMASTER_ALREADYENROLLEDSHOULDNOTHAPPEN);
      ret = -1;
      break;
   case CL_RESOLVE:
      /* commlib couldn't resolve name if commd host */
      strcpy(err_str, MSG_QMASTER_CANTRESOLVEHOSTNAMEXFROMACTQMASTERFILE_S);
      ret = -2;
      break;
   case CL_SERVICE:
      /* getservbyname() failed */
      strcpy(err_str, MSG_QMASTER_CANTRESOLVESERVEICESGECOMMDTCP);
      ret = -3;
      break;
   case NACK_PERM:         
      /* we didn't use reserved port, but commd expects it */
      sprintf(err_str, MSG_QMASTER_COMMDONHOSTXEXPECTSRESERVEDPORT_S,
              master);
      ret = -4;
      break;
   case NACK_UNKNOWN_HOST:
      /* commd couldn't resolve our name */
      sprintf(err_str, MSG_QMASTER_COMMDONHOSTXCANTRESOLVEOURHOSTNAME_S, master);
      ret = -5;
      break;
   default:
      /* Something else went wrong, usually a reason to try again */
      sprintf(err_str, MSG_QMASTER_COMMUNICATIONPROBLEONHOSTX_SS, master, cl_errstr(ret)
);
      strcpy(err_str, cl_errstr(ret));
      ret = ENROLL_ERROR_DO_RETRY;
      break;
   }
   DEXIT;
   return ret; 
}

/*-----------------------------------------------------------------------
 * check_for_running_qmaster
 * return: exit if there is a running qmaster
 *         exit if there is a unrecoverable commd error
 *         1 if we are enrolled (since we found a commd on local host)
 *         0 we didn't check yet for a commd on local host
 *-----------------------------------------------------------------------*/
int check_for_running_qmaster()
{
   char err_str[512];
   int ret, retry, enrolled;

   DENTER(TOP_LAYER, "check_for_running_qmaster");

   retry = 3;
   while (retry) {
      err_str[0] = '\0';
      if ((ret = qmaster_running(err_str, &enrolled))) {
         if (ret == 1) {
            CRITICAL((SGE_EVENT, MSG_QMASTER_FOUNDRUNNINGQMASTERONHOSTXNOTSTARTING_S,
                      err_str));
            SGE_EXIT(1);
         }
         else if (ret > ENROLL_ERROR_DO_RETRY) {
            CRITICAL((SGE_EVENT, MSG_QMASTER_CANTCHECKFORRUNNINGQMASTERX_S, err_str));
            SGE_EXIT(1);
         }
         else
            sleep(1);
      }
      else
         break;                 /* enroll was ok */
      retry--;
   }

   if (retry == 0) {
      CRITICAL((SGE_EVENT, MSG_COMMD_CANTCONTACTCOMMDX_S, err_str));
      SGE_EXIT(1);
   }

   DEXIT;
   return enrolled;          
}
