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
#include <stdlib.h>

#include "sge_gdi.h"
#include "sge_unistd.h"
#include "symbols.h"
#include "sge_all_listsL.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "usage.h"
#include "parse_qsub.h"
#include "parse.h"
#include "show_job.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "cull_parse_util.h"
#include "sge_var.h"
#include "sge_job.h"
#include "sge_answer.h"
#include "read_defaults.h"
#include "sge_centry.h"

#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qalter.h"

/* when this character is modified, it has also be modified
   the JOB_NAME_DEL in daemons/qmaster/sge_job_qmaster.c 
   It is used, when a job name is modified via job name to
   differ betwen the new job name and the old one.
   */
static const char *JOB_NAME_DEL = ":";

static lList *qalter_parse_job_parameter(lList *cmdline, lList **pjob, int *all_jobs, int *all_users);

int verify = 0;

extern char **environ;

int main(int argc, char *argv[]);

/************************************************************************/
int main(
int argc,
char **argv 
) {
   int ret = 0;
   lList *alp = NULL;
   lList *request_list = NULL;
   lList *cmdline = NULL;
   lListElem *aep;
   int all_jobs = 0;
   int all_users = 0;
   u_long32 gdi_cmd = SGE_GDI_MOD; 
   int cl_err = 0;
   int tmp_ret;
   int me_who;

   DENTER_MAIN(TOP_LAYER, "qalter");

   /*
   ** get command name: qalter or qresub
   */
   if (!strcmp(sge_basename(argv[0], '/'), "qresub")) {
      DPRINTF(("QRESUB\n"));
      me_who = QRESUB;
   }   
   else if (!strcmp(sge_basename(argv[0], '/'), "qhold")) {
      DPRINTF(("QHOLD\n"));
      me_who = QHOLD;
   }   
   else if (!strcmp(sge_basename(argv[0], '/'), "qrls")) {
      DPRINTF(("QRLS\n"));
      me_who = QRLS;
   } 
   else {
      DPRINTF(("QALTER\n"));
      me_who = QALTER;
   } 

   log_state_set_log_gui(0);

   sge_gdi_param(SET_MEWHO, me_who, NULL);
   if ((cl_err = sge_gdi_setup(prognames[uti_state_get_mewho()], &alp))) {
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(uti_state_get_mewho());

   /*
   ** begin to work
   */
   opt_list_append_opts_from_qalter_cmdline(&cmdline, &alp, argv + 1, environ);
   tmp_ret = answer_list_print_err_warn(&alp, MSG_QALTER, MSG_QALTERWARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   if (opt_list_has_X(cmdline, "-help")) {
      sge_usage(stdout);
      SGE_EXIT(1);
   }
   alp = qalter_parse_job_parameter(cmdline, &request_list, &all_jobs, 
                                    &all_users);
   
   DPRINTF(("all_jobs = %d, all_user = %d\n", all_jobs, all_users));

   if (request_list && verify) {
      /* 
         got a request list containing one element 
         for each job to be modified 
         save jobid all fields contain the same fields
         so we may use show_job() with the first job
         in our list 
         The jobid's in our request list get printed before
         show_job()
      */
      cull_show_job(lFirst(request_list), FLG_QALTER);
      SGE_EXIT(0);
   }

   tmp_ret = answer_list_print_err_warn(&alp, NULL, MSG_WARNING);
   if (tmp_ret > 0) {
      SGE_EXIT(tmp_ret);
   }

   if ((me_who == QALTER) ||
       (me_who == QHOLD) ||
       (me_who == QRLS) 
      ) {
      DPRINTF(("QALTER\n"));
      gdi_cmd = SGE_GDI_MOD;
   } else if (me_who == QRESUB){
      DPRINTF(("QRESUB\n"));
      gdi_cmd = SGE_GDI_COPY;
   } else {
      printf("unknown binary name.\n");
      SGE_EXIT(1);
   }

   if (all_jobs)
      gdi_cmd |= SGE_GDI_ALL_JOBS;
   if (all_users)
      gdi_cmd |= SGE_GDI_ALL_USERS;

   alp = sge_gdi(SGE_JOB_LIST, gdi_cmd, &request_list, NULL, NULL); 
   for_each (aep, alp) {
      printf("%s", lGetString(aep, AN_text));
      if (ret==0) {
         ret = lGetUlong(aep, AN_status);
      }
   }
   
   /* this is to get the correct exec state */ 
   if (ret == STATUS_OK) {
      ret = 0;
   } else {
      if (me_who == QALTER) {
         if (ret != STATUS_NOTOK_DOAGAIN) {
            ret = 1;
         }
      }
   }

   SGE_EXIT(ret);
   DEXIT;
   return 0;
}

/*
** NAME
**   qalter_parse_job_parameter
** PARAMETER
**   cmdline            - NULL or SPA_Type, if NULL, *pjob is initialised with defaults
**   prequestlist       - pointer a list of job modify request for sge_gdi
**
** RETURN
**   answer list, AN_Type or NULL if everything ok, the following stati can occur:
**   STATUS_EUNKNOWN   - bad internal error like NULL pointer received or no memory
**   STATUS_EDISK      - getcwd() failed
**   STATUS_ENOIMP     - unknown switch or -help occurred
** EXTERNAL
**   me
** DESCRIPTION
**   step 1:
**      parse all options into a dummy job 
**   step 2: 
**      iterate over the jobids in the cmd line an build 
**      gdi requests for each using the options from 
**      dummy job and put them into the prequestlist
*/
static lList *qalter_parse_job_parameter(
lList *cmdline,
lList **prequestlist,
int *all_jobs,
int *all_users 
) {
   lListElem *ep  = NULL;
   lListElem *job = NULL;
   lListElem *rep = NULL;
   u_long32 jobid;
   int i;
   lEnumeration *what = NULL;
   lDescr *rdp = NULL;
   lList *answer = NULL;
   int is_mail_requested = 0;
   enum {NOTINIT, JOB, ALL} all_or_jidlist = NOTINIT;
   int users_flag = 0;

   int job_field[100];
   int me_who;
   bool is_hold_option = false;

   DENTER(TOP_LAYER, "qalter_parse_job_parameter"); 

   me_who = uti_state_get_mewho();

   if (!prequestlist) {
      answer_list_add(&answer, MSG_PARSE_NULLPOINTERRECEIVED, STATUS_EUNKNOWN, 
                      ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   /*
      STEP 1:
         parse all options into a complete job structure 
         and build up an array of job structure names
         that are selected for modification by the options 
   */
   /* we need this job to parse our options in */
   job = lCreateElem(JB_Type);
   if (!job) {
      sprintf(SGE_EVENT, MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
      answer_list_add(&answer, SGE_EVENT,
                      STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }
   /* initialize job field set */
   job_field[0] = NoName;
   nm_set(job_field, JB_job_number);
   nm_set(job_field, JB_job_name);

   /* don't do verification of schedulability per default */
   lSetUlong(job, JB_verify_suitable_queues, SKIP_VERIFY);
   nm_set(job_field, JB_verify_suitable_queues);


   /*
   ** -help now handled separately in main()
   ** you dont want the user to be able to make qmon do funny things, do you?
   ** (e.g. by putting -help in a script file)
   */
   if ((ep = lGetElemStr(cmdline, SPA_switch, "-help"))) {
      char str[1024];

      lRemoveElem(cmdline, ep);
      sprintf(str, MSG_ANSWER_HELPNOTALLOWEDINCONTEXT);
      answer_list_add(&answer, str, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   /* ---------------------------------------------------------- */

   if (me_who != QRESUB) {
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-a"))) {
         lSetUlong(job, JB_execution_time, lGetUlong(ep, SPA_argval_lUlongT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_execution_time);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-A"))) {
         lSetString(job, JB_account, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_account);
      }
 


      while ((ep = lGetElemStr(cmdline, SPA_switch, "-cwd"))) {
         char tmp_str[SGE_PATH_MAX + 1];
         char tmp_str2[SGE_PATH_MAX + 1];
         char tmp_str3[SGE_PATH_MAX + 1];
         const char *sge_o_home = job_get_env_string(job, VAR_PREFIX "O_HOME");
 
         if (!getcwd(tmp_str, sizeof(tmp_str))) {
            answer_list_add(&answer, MSG_ANSWER_GETCWDFAILED, 
                            STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         
         if (sge_o_home && !chdir(sge_o_home)) {
            if (!getcwd(tmp_str2, sizeof(tmp_str2))) {
               answer_list_add(&answer, MSG_ANSWER_GETCWDFAILED, 
                               STATUS_EDISK, ANSWER_QUALITY_ERROR);
               DEXIT;
               return answer;
            }

            chdir(tmp_str);
            if (!strncmp(tmp_str2, tmp_str, strlen(tmp_str2))) {
               sprintf(tmp_str3, "%s%s", sge_o_home, (char *) tmp_str + strlen(tmp_str2));
               strcpy(tmp_str, tmp_str3);
            }
         }
         lSetString(job, JB_cwd, tmp_str);
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_cwd);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-c"))) {
         if (lGetUlong(ep, SPA_argtype) == lLongT) {
            lSetUlong(job, JB_checkpoint_interval, 
                        lGetLong(ep, SPA_argval_lLongT));
            nm_set(job_field, JB_checkpoint_interval);
         }
         if (lGetUlong(ep, SPA_argtype) == lIntT) { 
            lSetUlong(job, JB_checkpoint_attr, lGetInt(ep, SPA_argval_lIntT));
            nm_set(job_field, JB_checkpoint_attr);
         }
         lRemoveElem(cmdline, ep);
      }


      while ((ep = lGetElemStr(cmdline, SPA_switch, "-ckpt"))) {
         lSetString(job, JB_checkpoint_name, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_checkpoint_name);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-dl"))) {
         lSetUlong(job, JB_deadline, lGetUlong(ep, SPA_argval_lUlongT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_deadline);
      }

      parse_list_simple(cmdline, "-e", job, JB_stderr_path_list, 0, 0, 
                        FLG_LIST_APPEND);
      if (lGetList(job, JB_stderr_path_list))
         nm_set(job_field, JB_stderr_path_list);
      
      parse_list_simple(cmdline, "-i", job, JB_stdin_path_list, 0, 0, 
                        FLG_LIST_APPEND);
      if (lGetList(job, JB_stdin_path_list))
         nm_set(job_field, JB_stdin_path_list);
   }

   /* STR_PSEUDO_JOBID */
   if (lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_JOBID)) {
      lList *jid_list = NULL;
      if (!parse_multi_jobtaskslist(&cmdline, STR_PSEUDO_JOBID, &answer, &jid_list, true, 0)) {
         DEXIT;
         return answer;
      }                                                 
      lSetList(job, JB_job_identifier_list, jid_list);
   } 

   if (me_who != QRESUB) {
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-u"))) {
         lList *lp = NULL;
         lList *jid_list = NULL;

         lAddElemStr(&jid_list, ID_str, "*", ID_Type); 
         lSetList(job, JB_job_identifier_list, jid_list);
    
         lXchgList(ep, SPA_argval_lListT, &lp);
         lSetList(job, JB_user_list, lp);
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_user_list);
         users_flag = 1;
      }
 
   }                    
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-h"))) {
      lListElem *jid;
      is_hold_option = true;

      for_each (jid, lGetList(job, JB_job_identifier_list)) {
         lSetUlong(jid, ID_force, (u_long32) lGetInt(ep, SPA_argval_lIntT));
      }
      lRemoveElem(cmdline, ep);
      nm_set(job_field, JB_ja_tasks);
      nm_set(job_field, JB_ja_structure);
   }

   if (me_who != QRESUB) {
      /* -hold_jid */
      if (lGetElemStr(cmdline, SPA_switch, "-hold_jid")) {
         lListElem *sep, *ep;
         lList *jref_list = NULL;
         while ((ep = lGetElemStr(cmdline, SPA_switch, "-hold_jid"))) {
            for_each(sep, lGetList(ep, SPA_argval_lListT)) {
               DPRINTF(("-hold_jid %s\n", lGetString(sep, ST_name)));
               lAddElemStr(&jref_list, JRE_job_name, lGetString(sep, ST_name), JRE_Type);
            }
            lRemoveElem(cmdline, ep);
         }
         lSetList(job, JB_jid_request_list , jref_list);
         nm_set(job_field, JB_jid_request_list );
         nm_set(job_field, JB_jid_predecessor_list);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-R"))) {
         lSetBool(job, JB_reserve, lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_reserve);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-j"))) {
         lSetBool(job, JB_merge_stderr, lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_merge_stderr);
      }

      parse_list_hardsoft(cmdline, "-l", job,
                           JB_hard_resource_list, JB_soft_resource_list);
      centry_list_remove_duplicates(lGetList(job, JB_hard_resource_list));
      if (lGetList(job, JB_hard_resource_list))
         nm_set(job_field, JB_hard_resource_list);
      centry_list_remove_duplicates(lGetList(job, JB_soft_resource_list));
      if (lGetList(job, JB_soft_resource_list))
         nm_set(job_field, JB_soft_resource_list);

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-m"))) {
         u_long32 ul;
         u_long32 old_mail_opts;
    
         ul = lGetInt(ep, SPA_argval_lIntT);
         if  ((ul & NO_MAIL)) {
            lSetUlong(job, JB_mail_options, 0);
            is_mail_requested = 0;
         }
         else {
            old_mail_opts = lGetUlong(job, JB_mail_options);
            lSetUlong(job, JB_mail_options, ul | old_mail_opts);
            is_mail_requested = 1;
         }
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_mail_options);
      }

      parse_list_simple(cmdline, "-M", job, JB_mail_list, MR_host, MR_user, FLG_LIST_MERGE);
      if (lGetList(job, JB_mail_list))
         nm_set(job_field, JB_mail_list);

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-N"))) {
         lSetString(job, JB_job_name, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
/*         nm_set(job_field, JB_job_name); */
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-notify"))) {
         lSetBool(job, JB_notify, true);
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_notify);
      }

      parse_list_simple(cmdline, "-o", job, JB_stdout_path_list, 0, 0, 
                        FLG_LIST_APPEND);
      if (lGetList(job, JB_stdout_path_list))
         nm_set(job_field, JB_stdout_path_list);

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-p"))) {
         lSetUlong(job, JB_priority, 
            BASE_PRIORITY + lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_priority);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-js"))) {
         lSetUlong(job, JB_jobshare, lGetUlong(ep, SPA_argval_lUlongT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_jobshare);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-P"))) {
         lSetString(job, JB_project, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_project);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-pe"))) {
         lSetString(job, JB_pe, lGetString(ep, SPA_argval_lStringT));
         /* put sublist from parsing into job */
         lSwapList(job, JB_pe_range, ep, SPA_argval_lListT);
         lRemoveElem(cmdline, ep);

         /* cannot address fields separately in command line but in gdi interface */ 
         nm_set(job_field, JB_pe);
         nm_set(job_field, JB_pe_range);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-ot"))) {
         lSetUlong(job, JB_override_tickets, lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_override_tickets);
      }

      parse_list_hardsoft(cmdline, "-q", job,
                           JB_hard_queue_list, JB_soft_queue_list);
      if (lGetList(job, JB_hard_queue_list))
         nm_set(job_field, JB_hard_queue_list);
      if (lGetList(job, JB_soft_queue_list))
         nm_set(job_field, JB_soft_queue_list);

      parse_list_hardsoft(cmdline, "-masterq", job,
                           JB_master_hard_queue_list, 0);
      if (lGetList(job, JB_master_hard_queue_list))
         nm_set(job_field, JB_master_hard_queue_list);
    
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-r"))) {
         lSetUlong(job, JB_restart, lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_restart);
      }

      while ((ep = lGetElemStr(cmdline, SPA_switch, "-w"))) {
         lSetUlong(job, JB_verify_suitable_queues, lGetInt(ep, SPA_argval_lIntT));
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_verify_suitable_queues);
      }

      /* not needed in job struct - they are still used at this point */
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-soft"))) 
         lRemoveElem(cmdline, ep);
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-hard"))) 
         lRemoveElem(cmdline, ep);

      parse_list_simple(cmdline, "-S", job, JB_shell_list, 0, 0, FLG_LIST_APPEND);
      if (lGetList(job, JB_shell_list))
         nm_set(job_field, JB_shell_list);

    
      /*
      ** to be processed in original order, set -V equal to -v
      */
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-V"))) {
         lSetString(ep, SPA_switch, "-v");
      }
      parse_list_simple(cmdline, "-v", job, JB_env_list, VA_variable, VA_value, FLG_LIST_MERGE);
      if (lGetList(job, JB_env_list))
         nm_set(job_field, JB_env_list);

      /*
      ** -qs_args ... -qs_end
      */
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-qs_args"))) {
         lSwapList(job, JB_qs_args, ep, SPA_argval_lListT);
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_qs_args);
      }


      if ((ep = lGetElemStr(cmdline, SPA_switch, "--"))) {
         lRemoveElem(cmdline, ep);
         nm_set(job_field, JB_job_args);
      }
      {
         lList *lp;
    
         lp = lCopyList("job args", lGetList(job, JB_job_args));
    
         while ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_JOBARG))) {
            lAddElemStr(&lp, ST_name, lGetString(ep, SPA_argval_lStringT), ST_Type);
            lRemoveElem(cmdline, ep);
            nm_set(job_field, JB_job_args);
         }
         lSetList(job, JB_job_args, lp);

      }

      /* context switches are sensitive to order */
      ep = lFirst(cmdline);
      while(ep)
         if(!strcmp(lGetString(ep, SPA_switch), "-ac") ||
            !strcmp(lGetString(ep, SPA_switch), "-dc") ||
            !strcmp(lGetString(ep, SPA_switch), "-sc")) {
            lListElem* temp;
            if(!lGetList(job, JB_context))
               lSetList(job, JB_context, lCopyList("context", lGetList(ep, SPA_argval_lListT)));
            else
               lAddList(lGetList(job, JB_context), lCopyList("context", lGetList(ep, SPA_argval_lListT)));
            temp = lNext(ep);
            lRemoveElem(cmdline, ep);
            ep = temp;
            nm_set(job_field, JB_context);
         }
         else
            ep = lNext(ep);
   }

   /* complain about unused options */
   for_each(ep, cmdline) {
      const char *cp;
      char str[1024];
 
      sprintf(str, MSG_ANSWER_UNKOWNOPTIONX_S,
         lGetString(ep, SPA_switch));
      cp = lGetString(ep, SPA_switch_arg);
      if (cp) {
         strcat(str, " ");
         strcat(str, cp);
      }
      strcat(str, "\n");
      answer_list_add(&answer, str, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, "-verify"))) {
      lRemoveElem(cmdline, ep);
      verify = 1;
      nm_set(job_field, JB_env_list);
   } 
  
   if (job_field[1] == NoName) {
      answer_list_add(&answer, MSG_JOB_NOJOBATTRIBUTESELECTED, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

/* printf("=============== lWriteElemTo(job, stdout); ==================\n"); */
/* lWriteElemTo(job, stdout); */


   /* 
      STEP 2:
         make an gdi request for each jobid in the command line 
         using only these job fields that are in our job_field array 
   */

   if (!(what = lIntVector2What(JB_Type, job_field))) {
      answer_list_add(&answer, MSG_ANSWER_FAILDTOBUILDREDUCEDDESCRIPTOR, 
                        STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   rdp = NULL;
   lReduceDescr(&rdp, JB_Type, what);
   if (!rdp) {
      answer_list_add(&answer, MSG_ANSWER_FAILDTOBUILDREDUCEDDESCRIPTOR, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }
   
   lFreeWhat(what);

   /* if user uses -u or -uall flag and does not enter jids
      we will add a dummy job to send other parameters to qmaster */
   if (users_flag && !lGetList(job, JB_job_identifier_list)){   
      lList *jid_list = NULL;

      lAddElemStr(&jid_list, ID_str, "*", ID_Type);
      lSetList(job, JB_job_identifier_list, jid_list);
   }

   /* get next job id from cmd line */
   for_each (ep, lGetList(job, JB_job_identifier_list)) {
      lList *task_list = NULL;
      lListElem *task;
      lDescr task_descr[] = { 
            {JAT_task_number, lUlongT},
            {JAT_hold, lUlongT},
            {NoName, lEndT}
      };

      jobid = atol(lGetString(ep, ID_str));
/*
      if ((all_or_jidlist == NOTINIT) && !strcmp(lGetString(ep, ID_str), "all")) {
         all_or_jidlist = ALL;
         (*all_jobs) = 1;
         DPRINTF(("got \'all\' from parsing\n", jobid));
      } else */
      if (((all_or_jidlist == NOTINIT) || (all_or_jidlist == JOB)) /* && 
                  jobid != 0*/) { 
         all_or_jidlist = JOB;
         (*all_jobs) = 0; 
         DPRINTF(("got job " u32 " from parsing\n", jobid));
      } else {
         if (!strcmp(lGetString(ep, ID_str), "all") || (all_or_jidlist == ALL)) {
            answer_list_add(&answer, MSG_ANSWER_ALLANDJOBIDSARENOTVALID, 
                            STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         } else if (!strcmp(lGetString(ep, ID_str), "dummy")) {
            /* we will add a dummy-object to send parameters to qmaster */ 
         } else {
            answer_list_add(&answer, MSG_ANSWER_0ISNOTAVALIDJOBID,
                            STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
      }
      if ((jobid != 0) || ((*all_jobs) == 1)){
         rep = lAddElemUlong(prequestlist, JB_job_number, jobid, rdp);
         lSetString(rep, JB_job_name, lGetString(job, JB_job_name));
      }   
      else{
         char *name = NULL;
         const char *job_name = lGetString(job, JB_job_name);
         int size = strlen(lGetString(ep, ID_str));
         if (job_name)
            size += strlen(job_name); 
         size += 3;
          
         name = malloc(size);
         sprintf(name, "%s%s%s%s", JOB_NAME_DEL, lGetString(ep, ID_str), JOB_NAME_DEL, job_name?job_name:"");
         rep = lAddElemStr(prequestlist, JB_job_name, name, rdp);
         FREE(name);
      }   

      if (!rep) {   
         sprintf(SGE_EVENT, MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
         answer_list_add(&answer, SGE_EVENT,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }

      /* build task list from ID_Type from JB_job_identifier */
      
      if (is_hold_option && !lGetList(ep, ID_ja_structure)) {
         task = lAddElemUlong(&task_list, JAT_task_number, 0, task_descr);      
         lSetUlong(task, JAT_hold, lGetUlong(ep, ID_force));
      } else if (lGetList(ep, ID_ja_structure)) {
         lListElem *range;
         for_each(range, lGetList(ep, ID_ja_structure)) {
            u_long32 start = lGetUlong(range, RN_min);
            u_long32 end = lGetUlong(range, RN_max);
            u_long32 step = lGetUlong(range, RN_step);
            for (;start<=end; start += step) {
               task = lAddElemUlong(&task_list, JAT_task_number, 
                                       start, task_descr);
               lSetUlong(task, JAT_hold, lGetUlong(ep, ID_force));
            } 
         }
      }
      if ((lGetPosViaElem(rep, JB_ja_tasks) == -1) && (lGetNumberOfElem(task_list))){
         sprintf(SGE_EVENT, MSG_OPTIONWORKSONLYONJOB);
         answer_list_add(&answer, SGE_EVENT,
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
      lSetList(job, JB_ja_tasks, task_list);
      lSetList(job, JB_ja_structure, 
               lCopyList("", lGetList(ep, ID_ja_structure)));

      /* fill in fields of the job */
      {
         static int str_nm[] = {
            JB_account,
            JB_cwd,
            JB_checkpoint_name,
            JB_project,
            JB_pe,
            NoName
         };
         static int ulong_nm[] = {
            JB_execution_time,
            JB_deadline,
            JB_mail_options,
            JB_priority,
            JB_jobshare,
            JB_override_tickets,
            JB_restart,
            JB_verify_suitable_queues,
            NoName
         };
         static int bool_nm[] = {
            JB_reserve,
            JB_merge_stderr,
            JB_notify,
            NoName
         };
         static int list_nm[] = {
            JB_stderr_path_list,
            JB_jid_request_list,
            JB_hard_resource_list,
            JB_soft_resource_list,
            JB_mail_list,
            JB_stdout_path_list,
            JB_stdin_path_list,
            JB_pe_range,
            JB_hard_queue_list,
            JB_soft_queue_list,
            JB_shell_list,
            JB_env_list,
            JB_job_args,
            JB_qs_args,
            JB_context,
            JB_ja_tasks,
            JB_ja_structure,
            JB_user_list,
            JB_master_hard_queue_list,
            NoName
         };

         /* copy all strings */
         for (i=0; str_nm[i]!=NoName; i++)
            if (lGetPosViaElem(job, str_nm[i]) != -1 && lGetPosViaElem(rep, str_nm[i]) != -1)
               lSetString(rep, str_nm[i], lGetString(job, str_nm[i]));

         /* copy all ulongs */
         for (i=0; ulong_nm[i]!=NoName; i++)
            if (lGetPosViaElem(job, ulong_nm[i]) != -1 && lGetPosViaElem(rep, ulong_nm[i]) != -1)
               lSetUlong(rep, ulong_nm[i], lGetUlong(job, ulong_nm[i]));

         /* copy all bools */
         for (i=0; bool_nm[i]!=NoName; i++)
            if (lGetPosViaElem(job, bool_nm[i]) != -1 && lGetPosViaElem(rep, bool_nm[i]) != -1)
               lSetBool(rep, bool_nm[i], lGetBool(job, bool_nm[i]));

         /* copy all lists */
         for (i=0; list_nm[i]!=NoName; i++)
            if (lGetPosViaElem(job, list_nm[i]) != -1  && lGetPosViaElem(rep, list_nm[i]) != -1)
               lSetList(rep, list_nm[i], lCopyList("", lGetList(job, list_nm[i])));

      }
   }
   if (!*prequestlist) {
      /* got no target */
      answer_list_add(&answer, MSG_JOB_MISSINGJOBID, 
                      STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;      
   }

   DEXIT;
   return answer;
}
