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

#include "symbols.h"
#include "sge_options.h"
#include "sge_gdi_intern.h"
#include "sge_ja_task.h"
#include "sge_stringL.h"
#include "sge_pe.h"
#include "sge_queue.h"
#include "sge_string.h"
#include "parse_qsubL.h"
#include "sge_job_refL.h"
#include "sge_requestL.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "unparse_job_cull.h"
#include "sge_resource.h"
#include "parse.h"
#include "sgermon.h"
#include "cull_parse_util.h"
#include "sge.h"
#include "valid_queue_user.h"
#include "sge_hostname.h"
#include "sge_var.h"
#include "sge_answer.h" 
#include "sge_language.h"
#include "sge_feature.h"
#include "sge_dstring.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_userset.h"
#include "msg_daemons_common.h"

static char *sge_unparse_checkpoint_attr(int opr, char *string);
/* static char *sge_unparse_hold_list(u_long32 hold); */
static char *sge_unparse_mail_options(u_long32 mail_opt);
static int sge_unparse_checkpoint_option(lListElem *job, lList **pcmdline, lList **alpp);
static int sge_unparse_account_string(lListElem *job, lList **pcmdline, lList **alpp);
static int sge_unparse_path_list(lListElem *job, int nm, char *option, lList **pcmdline, lList **alpp);


lList *cull_unparse_job_parameter(
lList **pcmdline,
lListElem *job,
int flags 
) {
   const char *cp;
   u_long32 ul;
   lList *answer = NULL;
   char str[1024 + 1];
   lList *lp;
   int ret;
   lListElem *ep_opt;

   DENTER(TOP_LAYER, "cull_unparse_job_parameter");

   /*
   ** -a
   ** problem with submission time, but that is not a good
   ** default option anyway, is not unparsed
   */

   /*
   ** -A
   */
   if (sge_unparse_account_string(job, pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }

   /*
   ** -c
   */
   if (sge_unparse_checkpoint_option(job, pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }
  
#if 0 /* JG: removed JB_cell from job object */  
   /*
   ** -cell
   ** we make no difference between the default value and some other value
   ** that comes from the -cell option, because we cant differentiate
   ** as JB_cell is always set, there will always be a -cell option in a
   ** defaults file
   */
   if (sge_unparse_string_option(job, JB_cell, "-cell", 
            pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }
#endif   


   /*
    * -ckpt 
    */
   if (sge_unparse_string_option(job, JB_checkpoint_name, "-ckpt", 
            pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }


   /*
   ** -cwd
   */
   if (lGetString(job, JB_cwd)) {
      ep_opt = sge_add_noarg(pcmdline, cwd_OPT, "-cwd", NULL);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      /*
       * -P
       */
      if (sge_unparse_string_option(job, JB_project, "-P",
               pcmdline, &answer) != 0) {
         DEXIT;
         return answer;
      }
   }

   /*
   ** -C
   */
   if (sge_unparse_string_option(job, JB_directive_prefix, "-C", 
            pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }

   /*
   ** -e
   */
   if (sge_unparse_path_list(job, JB_stderr_path_list, "-e", pcmdline, 
                     &answer) != 0) {
      DEXIT;
      return answer;
   }

   /*
   ** -h, here only user hold supported at the moment
   */
   if  ((ul = lGetUlong(lFirst(lGetList(job, JB_ja_tasks)), JAT_hold))) {
      ep_opt = sge_add_noarg(pcmdline, h_OPT, "-h", NULL);
   }

   /*
   ** -hold_jid
   */
   if ((lp = lGetList(job, JB_jid_predecessor_list))) {
      intprt_type fields[] = { JRE_job_number, 0 };
      const char *delis[] = {NULL, ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 0);
      if (ret) {
         DPRINTF(("Error %d formatting jid_predecessor_list as -hold_jid\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGJIDPREDECESSORLISTASHOLDJID);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, hold_jid_OPT, lListT, "-hold_jid", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("hold_jid list", lp));      
   }

   /*
   ** -j
   */
   if ((ul = lGetBool(job, JB_merge_stderr))) {
      ep_opt = sge_add_arg(pcmdline, j_OPT, lIntT, "-j", "y");
      lSetInt(ep_opt, SPA_argval_lIntT, TRUE);
   }
   
   /*
   ** -jid
   */
   if ((lp = lGetList(job, JB_job_identifier_list))) {
      intprt_type fields[] = { JRE_job_number, 0};
      const char *delis[] = {"", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 
         0);
      if (ret) {
         DPRINTF(("Error %d formatting job_identifier_list as -jid\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGJOBIDENTIFIERLISTASJID);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, jid_OPT, lListT, "-jid", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("jid list", lp));      
   }

   /*
   ** -lj is in parsing but can't be unparsed here
   */

   /*
   ** -l
   */
   if (sge_unparse_resource_list(job, JB_hard_resource_list,
            pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }
   if (sge_unparse_resource_list(job, JB_soft_resource_list,
            pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }






   /*
   ** -m
   */
   if ((ul = lGetUlong(job, JB_mail_options))) {
      cp = sge_unparse_mail_options(ul);
      if (!cp) {
         DPRINTF(("Error unparsing mail options\n"));
         sprintf(str, MSG_PARSE_ERRORUNPARSINGMAILOPTIONS);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, m_OPT, lIntT, "-m", cp);
      lSetInt(ep_opt, SPA_argval_lIntT, ul);
   }

   /*
   ** -M obviously a problem!!!
   ** not unparsed at the moment
   ** does it make sense as a default, after all?
   */
   if ((lp = lGetList(job, JB_mail_list))) {
      lList *lp_new = NULL;
      lListElem *ep_new = NULL;
      lListElem *ep = NULL;
      const char *host;
      const char *user;

      /*
      ** or rather take all if there are more than one elements?
      */
      for_each(ep, lp) {
         user = lGetString(ep, MR_user);
         host = lGetHost(ep, MR_host);
         if (sge_strnullcmp(user, uti_state_get_user_name()) || 
             sge_hostcmp(host, uti_state_get_qualified_hostname())) {
            ep_new = lAddElemStr(&lp_new, MR_user, user, MR_Type);
            lSetHost(ep_new, MR_host, host);
         }
      }
      if (lp_new) {
         intprt_type fields[] = { MR_user, MR_host, 0 };
         const char *delis[] = {"@", ",", NULL};

         ret = uni_print_list(NULL, str, sizeof(str) - 1, lp_new, fields, delis, 
            FLG_NO_DELIS_STRINGS);
         if (ret) {
            DPRINTF(("Error %d formatting mail list as -M\n", ret));
            sprintf(str,  MSG_LIST_ERRORFORMATTINGMAILLISTASM );
            answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            return answer;
         }
         ep_opt = sge_add_arg(pcmdline, M_OPT, lListT, "-M", str);
         lSetList(ep_opt, SPA_argval_lListT, lCopyList("mail list", lp_new));      
 
      }
   }

   /*
   ** -N
   ** dont unparse the job name!
   ** each job has a jobname, but not each defaults file wants a -N
   */
#if 0
   if ((cp = lGetString(job, JB_job_name))) {
      ep_opt = sge_add_arg(pcmdline, N_OPT, lStringT, "-N", cp);
      lSetString(ep_opt, SPA_argval_lStringT, cp);
   }
#endif
   /*
   ** -notify
   */
   if  ((ul = lGetBool(job, JB_notify))) {
      ep_opt = sge_add_noarg(pcmdline, notify_OPT, "-notify", NULL);
   }

   /*
   ** -o
   */
   if (sge_unparse_path_list(job, JB_stdout_path_list, "-o", pcmdline, 
                     &answer) != 0) {
      DEXIT;
      return answer;
   }

   /*
   ** -p
   */
   if ((ul = lGetUlong(job, JB_priority)) != BASE_PRIORITY)  {
      int prty;

      prty = ul - BASE_PRIORITY;
      if (prty > 1024) {
         sprintf(str, MSG_PROC_INVALIDPROIRITYMUSTBELESSTHAN1025);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
      if (prty < -1023) {
         sprintf(str, MSG_PROC_INVALIDPRIORITYMUSTBEGREATERTHANMINUS1024);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
      sprintf(str, "%d", prty);
      ep_opt = sge_add_arg(pcmdline, p_OPT, lIntT, "-p", str);
      lSetInt(ep_opt, SPA_argval_lIntT, prty);
   }

   /*
   ** -pe
   */
   if (sge_unparse_pe(job, pcmdline, &answer) != 0) {
      DEXIT;
      return answer;
   }


#if 1
   /*
   ** -q
   ** exec-string is suppressed because uni_print_list can't do this
   ** is not in the manual anyway
   */
   if ((lp = lGetList(job, JB_hard_queue_list))) {
      intprt_type fields[] = { QR_name, 0 };
      const char *delis[] = {"@", ",", NULL};

      ep_opt = sge_add_noarg(pcmdline, hard_OPT, "-hard", NULL);
      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 
         FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting hard_queue_list as -q\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGHARDQUEUELISTASQ);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, q_OPT, lListT, "-q", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("hard queue list", lp));      
      lSetInt(ep_opt, SPA_argval_lIntT, 1); /* means hard */
   }
   if ((lp = lGetList(job, JB_soft_queue_list))) {
      intprt_type fields[] = { QR_name, 0 };
      const char *delis[] = {"@", ",", NULL};

      ep_opt = sge_add_noarg(pcmdline, soft_OPT, "-soft", NULL);
      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 
         FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting soft_queue_list as -q\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGSOFTQUEUELISTASQ);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, q_OPT, lListT, "-q", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("soft queue list", lp));      
      lSetInt(ep_opt, SPA_argval_lIntT, 2); /* means soft */
   }
#endif
   

   /*
   ** -r
   */
   if ((lGetUlong(job, JB_restart) == 1)) {
      ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, "-r", "y");
      lSetInt(ep_opt, SPA_argval_lIntT, 1);
   }
   else if ((lGetUlong(job, JB_restart) == 2)) {
      ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, "-r", "n");
      lSetInt(ep_opt, SPA_argval_lIntT, 2);
   }

   /*
   ** -S
   */
   if ((lp = lGetList(job, JB_shell_list))) {
      intprt_type fields[] = { PN_host, PN_path, 0 };
      const char *delis[] = {":", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting shell_list\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGSHELLLIST);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, S_OPT, lListT, "-S", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("shell list", lp));
   }

   /*
   ** -v, -V
   ** we always generate a -v statement, which means the user should not
   ** declare a job generated with -V as default
   */
   if ((lp = lGetList(job, JB_env_list))) {
      intprt_type fields[] = { VA_variable, VA_value, 0};
      const char *delis[] = {"=", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 
         FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting environment list as -v\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGENVIRONMENTLISTASV);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, v_OPT, lListT, "-v", str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("env list", lp));      
   }

   /*
   ** -verify is not unparsed
   */

   /*
   ** for full cmdline, script is needed
   */
   if ((flags & FLG_FULL_CMDLINE) &&
      (cp = lGetString(job, JB_script_file))) {
      if (!strcmp(cp, "STDIN")) {
         if (lGetList(job, JB_job_args)) {
         ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_SCRIPT, "--");
         lSetString(ep_opt, SPA_argval_lStringT, "--");
         }
      }
      else {
      }
   }

   /*
   ** for full cmdline, job args are also needed
   */
   if ((flags & FLG_FULL_CMDLINE) &&
      (lp = lGetList(job, JB_job_args))) {
      intprt_type fields[] = { STR, 0};
      const char *delis[] = {NULL, " ", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, 
         FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting job arguments\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGJOBARGUMENTS);
         answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return answer;
      }
      ep_opt = sge_add_arg(pcmdline, 0, lListT, STR_PSEUDO_JOBARG, str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("job arguments", lp));      
   }




   DEXIT;
   return answer;
}


static char *sge_unparse_checkpoint_attr(
int opr,
char *str 
) {
   int i = 0;

   if (opr & CHECKPOINT_AT_MINIMUM_INTERVAL)
      str[i++] = CHECKPOINT_AT_MINIMUM_INTERVAL_SYM;
   if (opr & CHECKPOINT_AT_SHUTDOWN)
      str[i++] = CHECKPOINT_AT_SHUTDOWN_SYM;   
   if (opr & CHECKPOINT_SUSPEND)
      str[i++] = CHECKPOINT_SUSPEND_SYM;
   if (opr & NO_CHECKPOINT)
      str[i++] = NO_CHECKPOINT_SYM;

   str[i] = '\0';
   
   return str;
}

#if 0
static char *sge_unparse_hold_list(
u_long32 hold 
) {
   static char hold_str[4 + 1];
   char *pc;

   DENTER(BASIS_LAYER, "sge_unparse_hold_list");
   memset(hold_str, 0, sizeof(hold_str));
   pc = hold_str;
   if (VALID(USER, hold)) {
      *pc++ = USER_SYM;
   }
   if (VALID(SYSTEM, hold)) {
      *pc++ = SYSTEM_SYM;
   }
   if (VALID(OTHER, hold)) {
      *pc++ = OTHER_SYM;
   }
   if (VALID(NO_HOLD, hold)) {
      *pc++ = NO_HOLD_SYM;
   }
   if (!*hold_str) {
      DEXIT;
      return NULL;
   }
   DEXIT;
   return hold_str;
}
#endif

/*-------------------------------------------------------------------------*/
static char *sge_unparse_mail_options(
u_long32 mail_opt 
) {
   static char mail_str[5 + 1];
   char *pc;

   DENTER(TOP_LAYER, "sge_unparse_mail_options");

   memset(mail_str, 0, sizeof(mail_str));
   pc = mail_str;
   if (VALID(MAIL_AT_ABORT, mail_opt)) {
      *pc++ = 'a';
   }
   if (VALID(MAIL_AT_BEGINNING, mail_opt)) {
      *pc++ = 'b';
   }
   if (VALID(MAIL_AT_EXIT, mail_opt)) {
      *pc++ = 'e';
   }
   if (VALID(NO_MAIL, mail_opt)) {
      *pc++ = 'n';
   }
   if (VALID(MAIL_AT_SUSPENSION, mail_opt)) {
      *pc++ = 's';
   }
   
   if  (!*mail_str) {
      DEXIT;
      return NULL;
   }

   DEXIT;
   return (mail_str);
}

/*-------------------------------------------------------------------------*/
static int sge_unparse_account_string(
lListElem *job,
lList **pcmdline,
lList **alpp 
) {
   const char *cp;
   lListElem *ep_opt;

   DENTER(TOP_LAYER, "sge_unparse_account_string");
   
   if ((cp = lGetString(job, JB_account))) {
      if (strcmp(cp, DEFAULT_ACCOUNT)) {
         ep_opt = sge_add_arg(pcmdline, A_OPT, lStringT, "-A", cp);
         lSetString(ep_opt, SPA_argval_lStringT, cp);
      }
   }
   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*/
static int sge_unparse_checkpoint_option(
lListElem *job,
lList **pcmdline,
lList **alpp 
) {
   lListElem *ep_opt = NULL;
   char *cp;
   int i;
   char str[1024 + 1];
   u_long32 ul;
   
   DENTER(TOP_LAYER, "sge_unparse_checkpoint_option");

   if ((i = lGetUlong(job, JB_checkpoint_attr))) {
      if ((cp = sge_unparse_checkpoint_attr(i, str))) {
         ep_opt = sge_add_arg(pcmdline, 0, lIntT, "-c", cp);
         lSetInt(ep_opt, SPA_argval_lIntT, i);
      }
      else {
         sprintf(str, MSG_JOB_INVALIDVALUEFORCHECKPOINTATTRIBINJOB_U, 
            u32c(lGetUlong(job, JB_job_number)));
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return -1;
      }
   }
   
   if ((ul = lGetUlong(job, JB_checkpoint_interval))) {
      sprintf(str, uu32, ul);
      ep_opt = sge_add_arg(pcmdline, c_OPT, lLongT, "-c", str);
      lSetLong(ep_opt, SPA_argval_lLongT, (long) ul);
   }

   DEXIT;
   return 0;
}


/*-------------------------------------------------------------------------*/
int sge_unparse_string_option(
lListElem *job,
int nm,
char *option,
lList **pcmdline,
lList **alpp 
) {
   lListElem *ep_opt = NULL;
   const char *cp;

   DENTER(TOP_LAYER, "sge_unparse_string_option");
   
   if ((cp = lGetString(job, nm))) {
      ep_opt = sge_add_arg(pcmdline, 0, lStringT, option, cp);
      lSetString(ep_opt, SPA_argval_lStringT, cp);
   }
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*/
int sge_unparse_resource_list(
lListElem *job,
int nm,
lList **pcmdline,
lList **alpp 
) {   
   lList *lp;
   int ret = 0;
   char str[BUFSIZ];

   DENTER(TOP_LAYER, "sge_unparse_resource_list");

   if ((lp = lGetList(job, nm))) {
      lList *lp_one;
      lListElem *ep;
      lListElem *ep_opt;
      int hard = (nm == JB_hard_resource_list);
      
      if (hard) 
         ep_opt = sge_add_noarg(pcmdline, hard_OPT, "-hard", NULL);
      else
         ep_opt = sge_add_noarg(pcmdline, soft_OPT, "-soft", NULL);
      for_each (ep, lp) {
         /*
         ** if there is more than one element this means there are -l's with
         ** ranges as well as with no ranges, so the list must be unparsed
         ** into several -l statements
         */
         lp_one = lCreateList("-l 1 elem", RE_Type);
         lAppendElem(lp_one, lCopyElem(ep));
         ret = unparse_resources(NULL, str, sizeof(str) - 1, lp_one);
         if (ret) {
            lFreeList(lp_one);
            DPRINTF(("Error %d formatting hard_resource_list as -l\n", ret));
            sprintf(str, MSG_LIST_ERRORFORMATINGHARDRESOURCELISTASL);
            answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            return ret;
         }
         lFreeList(lp_one);
         if (*str && (str[strlen(str) - 1] == '\n')) {
            str[strlen(str) - 1] = 0;
         }
         ep_opt = sge_add_arg(pcmdline, l_OPT, lListT, "-l", str);

         if (hard)
            lSetList(ep_opt, SPA_argval_lListT, lCopyList("hard res", lp));
         else
            lSetList(ep_opt, SPA_argval_lListT, lCopyList("soft res", lp));

         if (hard) 
            lSetInt(ep_opt, SPA_argval_lIntT, 1); /* means hard */
         else
            lSetInt(ep_opt, SPA_argval_lIntT, 2); /* means soft */
      }
   }
   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*/
int sge_unparse_pe(
lListElem *job,
lList **pcmdline,
lList **alpp 
) {
   const char *cp;
   lList *lp = NULL;
   lListElem *ep_opt;
   dstring string_buffer = DSTRING_INIT;
   char str[BUFSIZ];
   int ret = 0;

   DENTER(TOP_LAYER, "sge_unparse_pe");

   if ((cp = lGetString(job, JB_pe))) {
      sge_dstring_append(&string_buffer, cp);
      sge_dstring_append(&string_buffer, " ");
      if (!(lp = lGetList(job, JB_pe_range))) {
         DPRINTF(("Job has parallel environment with no ranges\n"));
         sprintf(str, MSG_JOB_JOBHASPEWITHNORANGES);
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         sge_dstring_free(&string_buffer);
         DEXIT;
         return -1;
      }
      {
         dstring range_string = DSTRING_INIT;

         range_list_print_to_string(lp, &range_string, 1);
         sge_dstring_append(&string_buffer, 
                            sge_dstring_get_string(&range_string));
         sge_dstring_free(&range_string);
      }
      if (ret) {
         DPRINTF(("Error %d formatting ranges in -pe\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGRANGESINPE);
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         sge_dstring_free(&string_buffer);
         DEXIT;
         return ret;
      }
      ep_opt = sge_add_arg(pcmdline, pe_OPT, lStringT, "-pe", 
                           sge_dstring_get_string(&string_buffer));
      lSetString(ep_opt, SPA_argval_lStringT, cp);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList("pe ranges", lp));
   }
   sge_dstring_free(&string_buffer);
   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*/
static int sge_unparse_path_list(
lListElem *job,
int nm,
char *option,
lList **pcmdline,
lList **alpp 
) {
   lList *lp = NULL;
   int ret = 0;
   char str[BUFSIZ];
   lListElem *ep_opt;

   DENTER(TOP_LAYER, "sge_unparse_path_list");

   if ((lp = lGetList(job, nm))) {
      intprt_type fields[] = { PN_host, PN_path, 0 };
      const char *delis[] = {":", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting path_list\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGPATHLIST);
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return ret;
      }
      ep_opt = sge_add_arg(pcmdline, e_OPT, lListT, option, str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList(option, lp));
   }
   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*/
int sge_unparse_id_list(
lListElem *job,
int nm,
char *option,
lList **pcmdline,
lList **alpp 
) {
   lList *lp = NULL;
   int ret = 0;
   char str[BUFSIZ];
   lListElem *ep_opt;

   DENTER(TOP_LAYER, "sge_unparse_id_list");

   if ((lp = lGetList(job, nm))) {
      intprt_type fields[] = { QR_name, 0 };
      const char *delis[] = {":", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, FLG_NO_DELIS_STRINGS);
      if (ret) {
         DPRINTF(("Error %d formatting id list\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGIDLIST);
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         return ret;
      }
      ep_opt = sge_add_arg(pcmdline, q_OPT, lListT, option, str);
      lSetList(ep_opt, SPA_argval_lListT, lCopyList(option, lp));
   }
   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*/
int sge_unparse_acl(
const char *owner,
const char *group,
const char *option,
lList *acl,
lList **pcmdline,
lList **alpp 
) {
   lList *lp = NULL;
   int ret = 0;
   char str[BUFSIZ];
   lListElem *ap;
   lListElem *ep_opt;

   DENTER(TOP_LAYER, "sge_unparse_acl");

   for_each (ap, acl) 
      if (sge_contained_in_access_list(owner, group, ap, alpp)) 
         lAddElemStr(&lp, STR, lGetString(ap, US_name), ST_Type);

   if (lGetNumberOfElem(lp) > 0) {
      intprt_type fields[] = { STR, 0 };
      const char *delis[] = {":", ",", NULL};

      ret = uni_print_list(NULL, str, sizeof(str) - 1, lp, fields, delis, FLG_NO_DELIS_STRINGS);
      lp = lFreeList(lp);
      if (ret) {
         DPRINTF(("Error %d formatting acl list\n", ret));
         sprintf(str, MSG_LIST_ERRORFORMATINGACLLIST);
         answer_list_add(alpp, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return ret;
      }
      ep_opt = sge_add_arg(pcmdline, q_OPT, lListT, option, str);
   }      

   DEXIT;
   return 0;
}
       
      
