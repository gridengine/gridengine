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

#include "sge_all_listsL.h"
#include "usage.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "read_defaults.h"
#include "show_job.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_afsutil.h"
#include "setup_path.h"
#include "qm_name.h"
#include "sge_unistd.h"
#include "sge_security.h"
#include "sge_answer.h"
#include "sge_job.h"

#include "msg_clients_common.h"
#include "msg_qsub.h"


static void delete_job(u_long32 job_id, lList *lp);

extern char **environ;

int main(int argc, char **argv);

/************************************************************************/
int main(
int argc,
char **argv 
) {
   lList *opts_cmdline = NULL;
   lList *opts_defaults = NULL;
   lList *opts_scriptfile = NULL;
   lList *opts_all = NULL;
   lListElem *job = NULL;
   lList *lp_jobs;
   lList *alp = NULL;
   lListElem *aep;
   u_long32 status = STATUS_OK;
   u_long32 quality;
   u_long32 job_id = 0;
   int do_exit = 0;
   int scheduled = 0;
   int just_verify;
   int tmp_ret;

   DENTER_MAIN(TOP_LAYER, "qsub");

   sge_gdi_param(SET_MEWHO, QSUB, NULL);
   if (sge_gdi_setup(prognames[QSUB], &alp)!=AE_OK) {
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QSUB);

   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);

   /*
    * read switches from the various defaults files
    */
   opt_list_append_opts_from_default_files(&opts_defaults, &alp, environ);
   tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   /*
    * append the commandline switches to the list
    */
   opt_list_append_opts_from_qsub_cmdline(&opts_cmdline, &alp,
                                          argv + 1, environ);
   tmp_ret = answer_list_print_err_warn(&alp, "qsub: ", MSG_QSUB_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   /*
    * show usage if -help was in commandline
    */
   if (opt_list_has_X(opts_cmdline, "-help")) {
      sge_usage(stdout);
      SGE_EXIT(0);
   }

   /*
    * We will only read commandline options from scripfile if the script
    * itself should not be handled as binary
    */
   if (opt_list_is_X_true (opts_cmdline, "-b") ||
       (!opt_list_has_X (opts_cmdline, "-b") &&
        opt_list_is_X_true (opts_defaults, "-b"))) {
      DPRINTF(("Skipping options from script due to -b option\n"));
   } else {
      opt_list_append_opts_from_script(&opts_scriptfile, &alp, 
                                       opts_cmdline, environ);
      tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
      if (tmp_ret > 0) {
         SGE_EXIT(tmp_ret);
      }
   }

   /*
    * Merge all commandline options and interprete them
    */
   opt_list_merge_command_lines(&opts_all, &opts_defaults, 
                                &opts_scriptfile, &opts_cmdline);

   alp = cull_parse_job_parameter(opts_all, &job);

   tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   DPRINTF(("Everything ok\n"));
#ifndef NO_SGE_COMPILE_DEBUG
   if (rmon_mlgetl(&DEBUG_ON, TOP_LAYER) & INFOPRINT) { 
      lWriteElemTo(job, stdout);
   }
#endif

   if (lGetUlong(job, JB_verify)) {
      cull_show_job(job, 0);
      SGE_EXIT(0);
   }

   if (set_sec_cred(job) != 0) {
      fprintf(stderr, MSG_SEC_SETJOBCRED);
      SGE_EXIT(1);
   }

   just_verify = (lGetUlong(job, JB_verify_suitable_queues)==JUST_VERIFY);

   job_add_parent_id_to_context(job);

   /* add job */
   lp_jobs = lCreateList("submitted jobs", JB_Type);
   lAppendElem(lp_jobs, job);

   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION, &lp_jobs, NULL, NULL);

   /* reinitialize 'job' with pointer to new version from qmaster */
   job = lFirst(lp_jobs);

   for_each(aep, alp) {
      const char *s;

      status = lGetUlong(aep, AN_status);
      quality = lGetUlong(aep, AN_quality);
      s = lGetString(aep, AN_text);
      if (quality == ANSWER_QUALITY_ERROR) {
         if (s[strlen(s)-1] != '\n') {
            fprintf(stderr, "%s\n", s);
         } else {
            fprintf(stderr, "%s", s);
         }
         do_exit = 1;
      } else {
         printf("%s", lGetString(aep, AN_text));
         if (job) {
            job_id =  lGetUlong(job, JB_job_number ); 
         } else { 
            job_id = 0;
         }
         DPRINTF(("job id is: %ld\n", job_id));
      }
   }

   if (just_verify) {
      do_exit = 1;
   }

   /* if error or non-immediate job (w/o -now flag): exit */
   if(do_exit || !JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) {
      lFreeList(lp_jobs);
      lFreeList(alp);
      lFreeList(opts_all);
      if (status == STATUS_OK) {
         SGE_EXIT(0);
      } else if (status == STATUS_NOTOK_DOAGAIN) {
         SGE_EXIT(status);
      } else {
         SGE_EXIT(1);
      }
   }
DTRACE;
   
   /* we only come here if it is an immediate job */
   DPRINTF(("P O L L I N G    F O R   J O B  ! ! ! ! ! ! ! ! ! ! !\n"));
   DPRINTF(("=====================================================\n"));
   printf(MSG_QSUB_WAITINGFORIMMEDIATEJOBTOBESCHEDULED);
   fflush(stdout);
   sleep(5);
   while (1) {
      lCondition *where;
      lEnumeration *what;
      u_long32 job_status;
      int do_shut = 0;
      lList  *lp_poll = NULL;
      
      what = lWhat("%T(%I %I)", JB_Type, JB_ja_tasks, JB_context);
      where = lWhere("%T(%I==%u)", JB_Type, JB_job_number, job_id);
      alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_GET, &lp_poll, where, what);
      lFreeWhere(where);
      lFreeWhat(what);
      for_each(aep, alp) {
         status = lGetUlong(aep, AN_status);
         quality = lGetUlong(aep, AN_quality);
         if (quality == ANSWER_QUALITY_ERROR) {
            fprintf(stderr, "\n%s", lGetString(aep, AN_text));
            do_exit = 1;
            break;
         }
         else if (quality == ANSWER_QUALITY_WARNING) {
            printf("\n%s", lGetString(aep, AN_text));
         }
         else {
            /*
            ** dont print the string ok
            */
         }
         
      }

      do_shut = shut_me_down;
      if (do_shut) {
         fprintf(stderr, MSG_QSUB_REQUESTFORIMMEDIATEJOBHASBEENCANCELED);
         delete_job(job_id, lp_jobs);
         break;
      }

      lWriteList(lp_poll);
      if (!lp_jobs || !lFirst(lp_poll)) {
         fprintf(stderr, MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER);
         do_exit = 1;
         break;
      }
      job_status = lGetUlong(lFirst(lGetList(lFirst(lp_poll), JB_ja_tasks)), JAT_status);
      if (job_status == JIDLE) {
         printf(".");
         fflush(stdout);
      }
      else if ((job_status == JRUNNING) || (job_status == JTRANSFERING)) {
         printf(MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U, u32c(job_id));
         scheduled = 1;
         break;
      }
   } /*end of while(1) polling */

      
   lFreeList(lp_jobs);
   lFreeList(alp);
   lFreeList(opts_all);
   SGE_EXIT(status==STATUS_OK && scheduled ?0:1); /* 0 means ok - others are errors */
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------
 * Functions copied from qsh.c
 * put into shared library some time
 *-------------------------------------------------------------------*/

static void delete_job(
u_long32 job_id,
lList *jlp 
) {
   lListElem *jep;
   lListElem *idp;
   lList *idlp;
   char job_str[128];

   if (!jlp) {
      return;
   }
   jep = lFirst(jlp);
   if (!jep) {
      return;
   }
   
   sprintf(job_str, u32, job_id);
   idp = lAddElemStr(&idlp, ID_str, job_str, ID_Type);

   sge_gdi(SGE_JOB_LIST, SGE_GDI_DEL, &idlp, NULL, NULL);
   /*
   ** no error handling here, we try to delete the job
   ** if we can
   */
}

