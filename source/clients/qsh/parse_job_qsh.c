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
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

#include "symbols.h"
#include "sge_string.h"
#include "sge_time.h"
#include "parse_qsubL.h"
#include "sge_stringL.h"
#include "sge_resource.h"
#include "dispatcher.h"
#include "parse_job_cull.h"
#include "parse_qsub.h"
#include "parse_job_qsh.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_parse_util.h"
#include "sge_host.h"
#include "sge_path_alias.h"
#include "msg_common.h"
#include "sge_job_refL.h"
#include "sge_job.h"
#include "sge_stdlib.h"
#include "sge_prog.h"
#include "sge_var.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_host.h"
#include "sge_mailrec.h"

/*
** NAME
**   cull_parse_qsh_parameter
** PARAMETER
**   cmdline            - NULL or SPA_Type, if NULL, *pjob is initialised with defaults
**   pjob               - pointer to job element, is filled according to cmdline
**
** RETURN
**   answer list, AN_Type or NULL if everything ok, the following stati can occur:
**   STATUS_EUNKNOWN   - bad internal error like NULL pointer received or no memory
**   STATUS_EDISK      - getcwd() failed
**   STATUS_ENOIMP     - unknown switch or -help occurred
** EXTERNAL
**   me
** DESCRIPTION
*/
lList *cull_parse_qsh_parameter(lList *cmdline, lListElem **pjob) 
{
   const char *cp;
   lListElem *ep;
   lList *answer = NULL;
   lList *path_alias = NULL;
   u_long32 job_now;

   DENTER(TOP_LAYER, "cull_parse_qsh_parameter"); 

   if (!pjob) {
      answer_list_add(&answer, MSG_PARSE_NULLPOINTERRECEIVED, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   if (!lGetUlong(*pjob, JB_submission_time)) {
      lSetUlong(*pjob, JB_submission_time, sge_get_gmt());
   }
   if (!lGetString(*pjob, JB_owner)) {
      lSetString(*pjob, JB_owner, uti_state_get_user_name());
   }
   lSetUlong(*pjob, JB_uid, uti_state_get_uid());

   /*
   ** path aliasing
   */
   if (path_alias_list_initialize(&path_alias, &answer, uti_state_get_user_name(), 
                                  uti_state_get_qualified_hostname()) == -1) {
      DEXIT;
      return answer;
   }

   job_initialize_env(*pjob, &answer, path_alias);
   if (answer) {
      DEXIT;
      return answer;
   }

   lSetUlong(*pjob, JB_priority, BASE_PRIORITY);
   lSetUlong(*pjob, JB_verify_suitable_queues, SKIP_VERIFY);

   /*
   ** it does not make sense to rerun interactive jobs
   ** the default is to decide via the queue configuration
   ** we turn this off here explicitly (2 is no)
   */
   job_now = lGetUlong(*pjob, JB_type);
   if (JOB_TYPE_IS_QSH(job_now) || JOB_TYPE_IS_QLOGIN(job_now)
       || JOB_TYPE_IS_QRSH(job_now) || JOB_TYPE_IS_QRLOGIN(job_now)) {
      lSetUlong(*pjob, JB_restart, 2);
   }

   while (JOB_TYPE_IS_QRSH(job_now)
          && (ep = lGetElemStr(cmdline, SPA_switch, "-notify"))) {
      lSetBool(*pjob, JB_notify, true);
      lRemoveElem(cmdline, ep);
   }  

   /* 
    * turn on immediate scheduling as default  
    * can be overwriten by option -now n 
    */
   {
      u_long32 type = lGetUlong(*pjob, JB_type);

      JOB_TYPE_SET_IMMEDIATE(type);
      lSetUlong(*pjob, JB_type, type);
   }

   /*
   ** -clear option is special, is sensitive to order
   ** kills all options that come before
   ** there might be more than one -clear
   */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-clear"))) {
      lListElem *ep_run;
      const char *cp_switch;

      for (ep_run = lFirst(cmdline); ep_run;) {
         /*
         ** remove -clear itsself
         */
         if (ep_run == ep) {
            lRemoveElem(cmdline, ep_run);
            break;
         }
         /*
         ** lNext can never be NULL here, because the -clear
         ** element is the last one to delete
         ** in general, these two lines wont work!
         */
         ep_run = lNext(ep_run);
         
         /*
         ** remove switch only if it is not a pseudo-arg
         */
         cp_switch = lGetString(lPrev(ep_run), SPA_switch);
         if (cp_switch && (*cp_switch == '-')) {
            lRemoveElem(cmdline, lPrev(ep_run));
         }
      }

   }

   /*
   ** general remark: There is a while loop looping through the option
   **                 list. So inside the while loop there must be made
   **                 a decision if a second occurence of an option has
   **                 to be handled as error, should be warned, overwritten
   **                 or simply ignored.
   */ 

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-A"))) {
      /* the old account string is overwritten */
      lSetString(*pjob, JB_account, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-cwd"))) {
      char tmp_str[SGE_PATH_MAX + 1];
      char tmp_str2[SGE_PATH_MAX + 1];
      char tmp_str3[SGE_PATH_MAX + 1];
      const char *env_value = job_get_env_string(*pjob, VAR_PREFIX "O_HOME");

      if (!getcwd(tmp_str, sizeof(tmp_str))) {
         answer_list_add(&answer, MSG_ANSWER_GETCWDFAILED, 
                         STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
      if (!chdir(env_value)) {
         if (!getcwd(tmp_str2, sizeof(tmp_str2))) {
            answer_list_add(&answer, MSG_ANSWER_GETCWDFAILED, 
                            STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         chdir(tmp_str);
         if (!strncmp(tmp_str2, tmp_str, strlen(tmp_str2))) {
            sprintf(tmp_str3, "%s%s", env_value, 
               (char *) tmp_str + strlen(tmp_str2));
            strcpy(tmp_str, tmp_str3);
         }
      }
      lSetString(*pjob, JB_cwd, tmp_str);
      lRemoveElem(cmdline, ep);

      lSetList(*pjob, JB_path_aliases, lCopyList("PathAliases", path_alias));
   }
   path_alias = lFreeList(path_alias);


   /*
   ** the handling of the path lists is not so trivial
   ** if we have several path lists we have to check
   ** 1. is there only one path entry without host
   ** 2. do the entries collide, i.e. are there two
   **    entries for the same host with different paths
   ** These restrictions seem reasonable to me but are
   ** not addressed right now
   */

   /*
   ** to use lAddList correctly we have to get the list out
   ** of the option struct otherwise we free the list once again
   ** in the lSetList(ep, SPA_argval_lListT, NULL); call
   ** this can lead to a core dump
   ** so a better method is to xchange the list in the option struct 
   ** with a null pointer, this is not nice but safe
   ** a little redesign of cull would be nice
   ** see parse_list_simple
   */
   job_set_submit_task_ids(*pjob, 1, 1, 1);
   job_initialize_id_lists(*pjob, &answer);
   if (answer != NULL) {
      DEXIT;
      return answer;
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-h"))) {
      int in_hold_state = 0;

      if (lGetInt(ep, SPA_argval_lIntT) & MINUS_H_TGT_USER) {
         lSetList(*pjob, JB_ja_u_h_ids, lCopyList("user hold ids",
            lGetList(*pjob, JB_ja_n_h_ids)));
         in_hold_state = 1;
      }
      if (lGetInt(ep, SPA_argval_lIntT) & MINUS_H_TGT_OPERATOR) {
         lSetList(*pjob, JB_ja_o_h_ids, lCopyList("operator hold ids",
            lGetList(*pjob, JB_ja_n_h_ids)));
         in_hold_state = 1;
      }
      if (lGetInt(ep, SPA_argval_lIntT) & MINUS_H_TGT_SYSTEM) {
         lSetList(*pjob, JB_ja_s_h_ids, lCopyList("system hold ids",
            lGetList(*pjob, JB_ja_n_h_ids)));
         in_hold_state = 1;
      }
      if (in_hold_state) {
         lSetList(*pjob, JB_ja_n_h_ids, lCreateList("no hold list", RN_Type));
      }
      lRemoveElem(cmdline, ep);
   }

   /* -hold_jid */
   if (lGetElemStr(cmdline, SPA_switch, "-hold_jid")) {
      lListElem *ep, *sep;
      lList *jref_list = NULL;
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-hold_jid"))) {
         for_each(sep, lGetList(ep, SPA_argval_lListT)) {
            DPRINTF(("-hold_jid %s\n", lGetString(sep, ST_name)));
            lAddElemStr(&jref_list, JRE_job_name, lGetString(sep, ST_name), JRE_Type);
         }
         lRemoveElem(cmdline, ep);
      }
      lSetList(*pjob, JB_jid_predecessor_list, jref_list);
   }


   /* not needed in job struct */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-hard"))) {
      lRemoveElem(cmdline, ep);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, "-help"))) {
      char str[1024];

      lRemoveElem(cmdline, ep);
      sprintf(str, MSG_ANSWER_HELPNOTALLOWEDINCONTEXT);
      answer_list_add(&answer, str, STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   parse_list_hardsoft(cmdline, "-l", *pjob, 
                        JB_hard_resource_list, JB_soft_resource_list);
   sge_compress_resources(lGetList(*pjob, JB_hard_resource_list));
   sge_compress_resources(lGetList(*pjob, JB_soft_resource_list));

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-m"))) {
      u_long32 ul;
      u_long32 old_mail_opts;

      ul = lGetInt(ep, SPA_argval_lIntT);
      if  ((ul & NO_MAIL)) {
         lSetUlong(*pjob, JB_mail_options, 0);
      }
      else {
         old_mail_opts = lGetUlong(*pjob, JB_mail_options);
         lSetUlong(*pjob, JB_mail_options, ul | old_mail_opts);
      }
      lRemoveElem(cmdline, ep);
   }

   parse_list_simple(cmdline, "-M", *pjob, JB_mail_list, MR_host, MR_user, FLG_LIST_MERGE);

   if (!lGetList(*pjob, JB_mail_list)) {   
      ep = lAddSubStr(*pjob, MR_user, uti_state_get_user_name(), JB_mail_list, MR_Type);
      lSetHost(ep, MR_host, uti_state_get_qualified_hostname());
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-N"))) {
      lSetString(*pjob, JB_job_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-now"))) {
      u_long32 jb_now = lGetUlong(*pjob, JB_type);
      if(lGetInt(ep, SPA_argval_lIntT)) {
         JOB_TYPE_SET_IMMEDIATE(jb_now);
      } else {
         JOB_TYPE_CLEAR_IMMEDIATE(jb_now);
      }

      lSetUlong(*pjob, JB_type, jb_now);

      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-P"))) {
      /* the old project string is overwritten */
      lSetString(*pjob, JB_project, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-p"))) {
      lSetUlong(*pjob, JB_priority, 
         BASE_PRIORITY + lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-pe"))) {

      lSetString(*pjob, JB_pe, lGetString(ep, SPA_argval_lStringT));

      /* put sublist from parsing into job */
      lSwapList(*pjob, JB_pe_range, ep, SPA_argval_lListT);
      lRemoveElem(cmdline, ep);
   }

   parse_list_hardsoft(cmdline, "-q", *pjob, 
                        JB_hard_queue_list, JB_soft_queue_list);

   parse_list_hardsoft(cmdline, "-masterq", *pjob, 
                        JB_master_hard_queue_list, 0);

   parse_list_simple(cmdline, "-S", *pjob, JB_shell_list, PN_host, PN_path, FLG_LIST_MERGE);

   /* context switches are sensitive to order */
   ep = lFirst(cmdline);
   while(ep)
      if(!strcmp(lGetString(ep, SPA_switch), "-ac") ||
         !strcmp(lGetString(ep, SPA_switch), "-dc") ||
         !strcmp(lGetString(ep, SPA_switch), "-sc")) {
         lListElem* temp;
         if(!lGetList(*pjob, JB_context))
            lSetList(*pjob, JB_context, lCopyList("context", lGetList(ep, SPA_argval_lListT)));
         else
            lAddList(lGetList(*pjob, JB_context), lCopyList("context", lGetList(ep, SPA_argval_lListT)));
         temp = lNext(ep);
         lRemoveElem(cmdline, ep);
         ep = temp;
      }
      else
         ep = lNext(ep);

   /* not needed in job struct */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-soft"))) {
      lRemoveElem(cmdline, ep);
   }

   /*
   ** to be processed in original order, set -V equal to -v
   */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-V"))) {
      lSetString(ep, SPA_switch, "-v");
   }
   parse_list_simple(cmdline, "-v", *pjob, JB_env_list, VA_variable, VA_value, FLG_LIST_MERGE);

   /* -w e is default for interactive jobs */
   /* JG: changed default behaviour: skip verify to be consistant to qsub */
   lSetUlong(*pjob, JB_verify_suitable_queues, SKIP_VERIFY);

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-w"))) {
      lSetUlong(*pjob, JB_verify_suitable_queues, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   /*
   * handling for display:
   * command line (-display) has highest priority
   * then comes environment variable DISPLAY which was given via -v
   * then comes environment variable DISPLAY read from environment
   *
   * If a value for DISPLAY is set, it is checked for completeness:
   * In a GRID environment, it has to contain the name of the display host.
   * DISPLAY variables of form ":id" are not allowed and are deleted.
   *
   * we put the display switch into the variable list
   * otherwise, we'd have to add a field to the job structure
   */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-display"))) {
      lList  *lp;
      lListElem *vep;

      lp = lGetList(*pjob, JB_env_list);
      vep = lCreateElem(VA_Type);
      lSetString(vep, VA_variable, "DISPLAY");
      lSetString(vep, VA_value, lGetString(ep, SPA_argval_lStringT));
      if (!lp) {
         lp = lCreateList("env list", VA_Type);
         lSetList(*pjob, JB_env_list, lp);
      }
      lAppendElem(lp, vep);
      
      lRemoveElem(cmdline, ep);
   }
   cull_compress_definition_list(lGetList(*pjob, JB_env_list), 
                                 VA_variable, VA_value, 0);


   while ((ep = lGetElemStr(cmdline, SPA_switch, "-verify"))) {
      lSetUlong(*pjob, JB_verify, true);
      lRemoveElem(cmdline, ep);
   }

   {
      lList *lp;
      
      lp = lCopyList("job args", lGetList(*pjob, JB_job_args));

      while ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_JOBARG))) {
         lAddElemStr(&lp, ST_name, lGetString(ep, SPA_argval_lStringT), ST_Type);
         
         lRemoveElem(cmdline, ep);
      }
      lSetList(*pjob, JB_job_args, lp);
   }

   
   for_each(ep, cmdline) {
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

   {
      lListElem *ep;

      ep = lGetSubStr(*pjob, VA_variable, "DISPLAY", JB_env_list);

      /* if DISPLAY not set from -display or -v option,
       * try to read it from the environment
       */
      if(ep == NULL) {
         const char *display;
         display = getenv("DISPLAY");

         if(display != NULL) {
            ep = lAddSubStr(*pjob, VA_variable, "DISPLAY", JB_env_list, VA_Type);
            lSetString(ep, VA_value, display);   
         }   
      }
   }

   /* check DISPLAY on the client side before submitting job to qmaster 
    * only needed for qsh 
    */
   if(uti_state_get_mewho() == QSH) {
      job_check_qsh_display(*pjob, &answer, false);
   }

   DEXIT;
   return answer;
}



