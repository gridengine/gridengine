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
#include <unistd.h>

#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "read_defaults.h"
#include "sge_exit.h"
#include "show_job.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_resource.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"

#include "sge_peopen.h"
#include "utility.h"
#include "sge_copy_append.h"
#include "sge_arch.h"
#include "sge_afsutil.h"
#include "job.h"
#include "setup_path.h"
#include "qm_name.h"

#include "jb_now.h"
#include "sge_security.h"
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
   lListElem *option = NULL;
   lList *lp_jobs;
   lList *alp;
   lListElem *aep;
   u_long32 status = STATUS_OK;
   u_long32 quality;
   u_long32 job_id = 0;
   int do_exit = 0;
   int scheduled = 0;
   int just_verify;
   lListElem *ep;
   const char *dprefix = NULL;
   int cl_err = 0;

   DENTER_MAIN(TOP_LAYER, "qsub");

   sge_gdi_param(SET_MEWHO, QSUB, NULL);
/*    sge_gdi_param(SET_ISALIVE, 1, NULL); */
   if ((cl_err = sge_gdi_setup(prognames[QSUB]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QSUB);

   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);

   /*
   ** begin to work
   */
   /*
   ** read switches from the various defaults files
   */
   alp = get_all_defaults_files(&opts_defaults, environ);
   for_each(aep, alp) {
      const char *s;

      status = lGetUlong(aep, AN_status);
      quality = lGetUlong(aep, AN_quality);
      if (quality == NUM_AN_ERROR) {
         s = lGetString(aep, AN_text);
         if (s[strlen(s)-1] != '\n')
            fprintf(stderr, "%s\n", s);
         else 
            fprintf(stderr, "%s\n", s);
         do_exit = 1;
      }
      else {
         printf(MSG_WARNING_S, lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   /*
   ** append the commandline switches to the list
   */
   alp = cull_parse_cmdline(argv + 1, environ, &opts_cmdline, FLG_USE_PSEUDOS);

   for_each(aep, alp) {
      status = lGetUlong(aep, AN_status);
      quality = lGetUlong(aep, AN_quality);
      if (quality == NUM_AN_ERROR) {
         fprintf(stderr, "qsub: %s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf(MSG_QSUB_WARNING_S, lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   if ((ep = lGetElemStr(opts_cmdline, SPA_switch, "-help"))) {
      sge_usage(stdout);
      SGE_EXIT(0);
   }

   /*
   ** there must always be a script to parse
   ** even if the command line contains no script file argument
   ** then we read it from stdin
   */
   option = lGetElemStr(opts_cmdline, SPA_switch, STR_PSEUDO_SCRIPT);
   ep = lGetElemStr(opts_cmdline, SPA_switch, "-C");
   if (ep) {
      dprefix = lGetString(ep, SPA_argval_lStringT);
   } else {
      dprefix = default_prefix;
   }
   alp = parse_script_file(option ? 
                              lGetString(option, SPA_argval_lStringT) : NULL, 
      dprefix, &opts_scriptfile, environ, FLG_DONT_ADD_SCRIPT);
   for_each(aep, alp) {
      status = lGetUlong(aep, AN_status);
      quality = lGetUlong(aep, AN_quality);
      if (quality == NUM_AN_ERROR) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf(MSG_WARNING_S, lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
   }

   /*
   ** order is very important here
   */
   if (opts_defaults) {
      if (!opts_all) {
         opts_all = opts_defaults;
      }
      else {
         lAddList(opts_all, opts_defaults);
      }
      opts_defaults = NULL;
   }
   if (opts_scriptfile) {
      if (!opts_all) {
         opts_all = opts_scriptfile;
      }
      else {
         lAddList(opts_all, opts_scriptfile);
      }
      opts_scriptfile = NULL;
   }
   if (opts_cmdline) {
      if (!opts_all) {
         opts_all = opts_cmdline;
      }
      else {
         lAddList(opts_all, opts_cmdline);
      }
      opts_cmdline = NULL;
   }

   alp = cull_parse_job_parameter(opts_all, &job);

   for_each(aep, alp) {
      status = lGetUlong(aep, AN_status);
      quality = lGetUlong(aep, AN_quality);
      if (quality == NUM_AN_ERROR) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         do_exit = 1;
      }
      else {
         printf(MSG_WARNING_S, lGetString(aep, AN_text));
      }
   }
   lFreeList(alp);
   if (do_exit) {
      SGE_EXIT(status);
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

   add_parent_uplink(job);

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
      if (quality == NUM_AN_ERROR) {
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
   if(do_exit || !JB_NOW_IS_IMMEDIATE(lGetUlong(job, JB_now))) {
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
         if (quality == NUM_AN_ERROR) {
            fprintf(stderr, "\n%s", lGetString(aep, AN_text));
            do_exit = 1;
            break;
         }
         else if (quality == NUM_AN_WARNING) {
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
      else if ((job_status == JRUNNING) || (job_status == JTRANSITING)) {
         printf(MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U, u32c(job_id));
         scheduled = 1;
         break;
      }
   } /*end of while(1) polling */

      
   lFreeList(lp_jobs);
   lFreeList(alp);
   lFreeList(opts_all);
   SGE_EXIT(status==STATUS_OK && scheduled ?0:1); /* 0 means ok - others are errors */
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

