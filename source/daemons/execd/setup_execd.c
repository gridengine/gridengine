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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>  
#include <string.h>
#include <stdlib.h>

#include "sgermon.h"
#include "sge.h"
#include "sge_conf.h"
#include "rw_configuration.h"
#include "sge_chdir.h"
#include "sge_mkdir.h"
#include "sge_log.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_stringL.h"
#include "sge_gdi_intern.h"
#include "sge_copy_append.h"
#include "job_report_execd.h"
#include "execd_ck_to_do.h"
#include "setup_execd.h"
#include "sge_load_sensor.h"
#include "cull_file.h"
#include "utility_daemon.h"
#include "sge_daemonize.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "sge_getloadavg.h"
#include "sge_switch_user.h"
#include "sge_exit.h"
#include "sge_string.h"
#include "sge_stat.h"
#include "reaper_execd.h"
#include "execution_states.h"
#include "msg_common.h"
#include "msg_daemons_common.h"
#include "msg_execd.h"
#include "sge_feature.h"

extern char execd_spool_dir[];
extern lList *execd_config_list;
extern lList *jr_list;
extern lList *Master_Job_List;

char execd_messages_file[SGE_PATH_MAX];

/*-------------------------------------------------------------------*/
void sge_setup_sge_execd()
{
   char err_str[1024];

   DENTER(TOP_LAYER, "sge_setup_sge_execd");

   if (get_conf_and_daemonize(daemonize_execd, &execd_config_list)) {
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
      CRITICAL((SGE_EVENT, MSG_ERROR_CANTSWITCHTOADMINUSER));
      SGE_EXIT(1);
   }

   /* get aliased hostname from commd */
   reresolve_me_qualified_hostname();

   DPRINTF(("chdir(\"/\")----------------------------\n"));
   sge_chdir("/",1);
   DPRINTF(("Making directories----------------------------\n"));
   sge_mkdir(conf.execd_spool_dir, 0755, 1);
   DPRINTF(("chdir(\"%s\")----------------------------\n", conf.execd_spool_dir));
   sge_chdir(conf.execd_spool_dir,1);
   sge_mkdir(me.unqualified_hostname, 0755, 1);
   DPRINTF(("chdir(\"%s\",me.unqualified_hostname)--------------------------\n",
            me.unqualified_hostname));

   sge_chdir(me.unqualified_hostname, 1); 
   /* having passed the  previous statement we may 
      log messages into the ERR_FILE  */
   sge_copy_append(TMP_ERR_FILE_EXECD, ERR_FILE, SGE_APPEND);
   switch2start_user();
   unlink(TMP_ERR_FILE_EXECD);
   switch2admin_user();
   sprintf(execd_messages_file, "%s/%s/%s", conf.execd_spool_dir, 
           me.unqualified_hostname, ERR_FILE);
   error_file = execd_messages_file;

   sprintf(execd_spool_dir, "%s/%s", conf.execd_spool_dir, 
           me.unqualified_hostname);
   
   DPRINTF(("Making directories----------------------------\n"));
   sge_mkdir(EXEC_DIR, 0775, 1);
   sge_mkdir(JOB_DIR, 0775, 1);
   sge_mkdir(ACTIVE_DIR,  0775, 1);

   DEXIT;
   return;
}


/*-------------------------------------------------------------------*/
void sge_setup_old_jobs()
{
   stringT str;
   lListElem *jep, *tep, *direntry, *jatep;
   lList *direntries;

   DENTER(TOP_LAYER, "sge_setup_old_jobs");

   Master_Job_List = lCreateList("Master_Job_List", JB_Type);
   
   DPRINTF(("scanning %s for old jobs---------------\n", JOB_DIR));

   direntries = sge_get_dirents(JOB_DIR);
   for_each(direntry, direntries) {
      sprintf(str, "%s/%s", JOB_DIR, lGetString(direntry, STR));
      jep = lReadElemFromDisk(str, NULL, JB_Type, "job");
      if (jep) {
         u_long32 jobid;

         lAppendElem(Master_Job_List, jep);
         jobid = lGetUlong(jep, JB_job_number);
   
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            u_long32 jataskid = lGetUlong(jatep, JAT_task_number);

            add_job_report(jobid, jataskid, jep);

            /* add also job reports for tasks */
            for_each (tep, lGetList(jatep, JAT_task_list)) {
               add_job_report(jobid, jataskid, tep);
            }

            /* does active dir exist ? */
            if (lGetUlong(jatep, JAT_status) == JRUNNING ||
                lGetUlong(jatep, JAT_status) == JWAITING4OSJID) {
               SGE_STRUCT_STAT sb;
               char active_path[SGE_PATH_MAX];

               sprintf(active_path, ACTIVE_DIR"/"u32"."u32, jobid, jataskid);
               if (SGE_STAT(active_path, &sb)) {
                  /* lost active directory - initiate cleanup for job */
                  execd_job_run_failure(jep, jatep, "lost active dir of running job", GFSTATE_HOST);
                  continue;
               }
            }

#ifdef COMPILE_DC
            {
               int ret;
               /* register still running jobs at ptf */
               if (feature_is_enabled(FEATURE_USE_OSJOB_ID)) {
                  if (lGetUlong(jatep, JAT_status) == JRUNNING) {
                     if ((ret=register_at_ptf(jep, jatep, NULL))) {
                        ERROR((SGE_EVENT, MSG_JOB_XREGISTERINGJOBYATPTFDURINGSTARTUP_SU, 
                              ((ret == 1) ? MSG_DELAYED : MSG_FAILED), 
                              u32c(jobid)));
                     }
                  }
                  for_each(tep, lGetList(jatep, JAT_task_list)) {
                     if (lGetUlong(lFirst(lGetList(tep, JB_ja_tasks)), 
                           JAT_status) == JRUNNING) {
                        if ((ret=register_at_ptf(jep, jatep, tep))) {
                           ERROR((SGE_EVENT, MSG_JOB_XREGISTERINGJOBYTASKZATPTFDURINGSTARTUP_SUS, 
                              ((ret == 1) ? MSG_DELAYED : MSG_FAILED), 
                              u32c(jobid), lGetString(tep, JB_pe_task_id_str)));
                        }
                     }
                  }
               }
            }
#endif
         }
      }
   }
   direntries = lFreeList(direntries);

   DEXIT;
   return;
}


/*-------------------------------------------------------------------------*/
int daemonize_execd()
{
   fd_set keep_open;
   int ret, fd;

   DENTER(TOP_LAYER, "daemonize_execd");

   FD_ZERO(&keep_open); 

   /* ask load sensor to fill in it's fd's */
   set_ls_fds(&keep_open);

   /* do not close fd to /dev/kmem (or s.th. else) of loadavg() */
   if ((fd=get_channel_fd())!=-1) {
      INFO((SGE_EVENT, MSG_ANSWER_KEEPINGCHANNELFDXOPEN_I, fd));
      FD_SET(fd, &keep_open);
   } 

   if(!get_commlib_state_closefd()) {
      int fd = get_commlib_state_sfd();
      if (fd>=0) {
         FD_SET(fd, &keep_open);
      }
   }

   ret = sge_daemonize(&keep_open);

   DEXIT;
   return ret;
}
