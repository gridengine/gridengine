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

#include "def.h"
#include "symbols.h"
#include "sge_gdi_intern.h"
#include "sge_jobL.h"
#include "sge_answerL.h"
#include "sge_string.h"
#include "sge_time.h"
#include "parse_qsubL.h"
#include "sge_hostL.h"
#include "sge_stringL.h"
#include "sge_resource.h"
#include "dispatcher.h"
#include "parse_job_cull.h"
#include "parse_qsub.h"
#include "parse_job_qsh.h"
#include "sge_me.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_parse_util.h"
#include "utility.h"
#include "resolve_host.h"
#include "path_aliases.h"
#include "msg_common.h"

#include "jb_now.h"

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
lList *cull_parse_qsh_parameter(
lList *cmdline,
lListElem **pjob 
) {
   char *cp;
   lListElem *ep;
   lList *answer = NULL;
   lList *path_alias = NULL;
   u_long32 job_now;

   DENTER(TOP_LAYER, "cull_parse_qsh_parameter"); 

   if (!pjob) {
      sge_add_answer(&answer, MSG_PARSE_NULLPOINTERRECEIVED, STATUS_EUNKNOWN, 0);
      DEXIT;
      return answer;
   }

   if (!*pjob) {
      *pjob = lCreateElem(JB_Type);
      if (!*pjob) {
      sge_add_answer(&answer, MSG_MEM_MEMORYALLOCFAILED, STATUS_EMALLOC, 0);
      DEXIT;
      return answer;
      }
   }

   if (!lGetUlong(*pjob, JB_submission_time)) {
      lSetUlong(*pjob, JB_submission_time, sge_get_gmt());
   }
   if (!lGetString(*pjob, JB_owner)) {
      lSetString(*pjob, JB_owner, me.user_name);
   }
   lSetUlong(*pjob, JB_uid, me.uid);
   lSetString(*pjob, JB_sge_o_home, sge_getenv("HOME"));
   lSetString(*pjob, JB_sge_o_log_name, sge_getenv("LOGNAME"));
   lSetString(*pjob, JB_sge_o_path, sge_getenv("PATH"));
   lSetString(*pjob, JB_sge_o_mail, sge_getenv("MAIL"));
   lSetString(*pjob, JB_sge_o_shell, sge_getenv("SHELL"));
   lSetString(*pjob, JB_sge_o_tz, sge_getenv("TZ"));
   /*
   ** path aliasing
   */
   if (build_path_aliases(&path_alias, me.user_name, me.qualified_hostname, 
                        &answer) == -1) {
      DEXIT;
      return answer;
   }
   { 
      static char cwd_out[SGE_PATH_MAX + 1];
      static char tmp_str[SGE_PATH_MAX + 1];
      if (!getcwd(tmp_str, sizeof(tmp_str))) {
         sge_add_answer(&answer, MSG_ANSWER_GETCWDFAILED , STATUS_EDISK, 0);
         DEXIT;
         return answer;
      }
      get_path_alias(tmp_str, cwd_out, 
                        SGE_PATH_MAX, path_alias, 
                        me.qualified_hostname, NULL);  
      lSetString(*pjob, JB_sge_o_workdir, cwd_out);
   }
   lSetString(*pjob, JB_sge_o_host, 
      ((cp = sge_getenv("HOST")) ? cp : me.unqualified_hostname));
   if (lGetString(*pjob, JB_cell)) {
      lSetString(*pjob, JB_cell, me.default_cell);
   }

   lSetUlong(*pjob, JB_priority, BASE_PRIORITY);
   lSetUlong(*pjob, JB_verify_suitable_queues, SKIP_VERIFY);

   /*
   ** it does not make sense to rerun interactive jobs
   ** the default is to decide via the queue configuration
   ** we turn this off here explicitly (2 is no)
   */
   job_now = lGetUlong(*pjob, JB_now);
   if (JB_NOW_IS_QSH(job_now) || JB_NOW_IS_QLOGIN(job_now)
       || JB_NOW_IS_QRSH(job_now) || JB_NOW_IS_QRLOGIN(job_now)) {
      lSetUlong(*pjob, JB_restart, 2);
   }

   /* 
   ** turn on immediate scheduling as default - can be overwriten by option -now n 
   */
   lSetUlong(*pjob, JB_now, JB_NOW_IMMEDIATE);

   DTRACE;


   /*
   ** -clear option is special, is sensitive to order
   ** kills all options that come before
   ** there might be more than one -clear
   */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-clear"))) {
      lListElem *ep_run;
      char *cp_switch;

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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-cell"))) {
      lSetString(*pjob, JB_cell, lGetString(ep, SPA_argval_lStringT));
      me.default_cell = sge_strdup(me.default_cell, 
         lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-cwd"))) {
      char tmp_str[SGE_PATH_MAX + 1];
      char tmp_str2[SGE_PATH_MAX + 1];
      char tmp_str3[SGE_PATH_MAX + 1];

      if (!getcwd(tmp_str, sizeof(tmp_str))) {
         sge_add_answer(&answer, MSG_ANSWER_GETCWDFAILED, STATUS_EDISK, 0);
         DEXIT;
         return answer;
      }
      if (!chdir(lGetString(*pjob, JB_sge_o_home))) {
         if (!getcwd(tmp_str2, sizeof(tmp_str2))) {
            sge_add_answer(&answer, MSG_ANSWER_GETCWDFAILED, STATUS_EDISK, 0);
            DEXIT;
            return answer;
         }
         chdir(tmp_str);
         if (!strncmp(tmp_str2, tmp_str, strlen(tmp_str2))) {
            sprintf(tmp_str3, "%s%s", lGetString(*pjob, JB_sge_o_home), 
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


   /* not needed in job struct */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-hard"))) {
      lRemoveElem(cmdline, ep);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, "-help"))) {
      char str[1024];

      lRemoveElem(cmdline, ep);
      sprintf(str, MSG_ANSWER_HELPNOTALLOWEDINCONTEXT);
      sge_add_answer(&answer, str, STATUS_ENOIMP, 0);
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
      ep = lAddSubStr(*pjob, MR_user, me.user_name, JB_mail_list, MR_Type);
      lSetString(ep, MR_host, me.qualified_hostname);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-N"))) {
      lSetString(*pjob, JB_job_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-now"))) {
      u_long32 jb_now = lGetUlong(*pjob, JB_now);
      if(lGetInt(ep, SPA_argval_lIntT)) {
         JB_NOW_SET_IMMEDIATE(jb_now);
      } else {
         JB_NOW_CLEAR_IMMEDIATE(jb_now);
      }

      lSetUlong(*pjob, JB_now, jb_now);

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
   ** handling for display:
   ** command line has highest priority
   ** then comes environment variable DISPLAY which was given via -v
   ** then comes environment variable DISPLAY read from environment
   ** if we have none of that, we must resolve the host
   ** we put the display switch into the variable list
   ** otherwise, we'd have to add a field to the job structure
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
      lSetUlong(*pjob, JB_verify, TRUE);
      lRemoveElem(cmdline, ep);
   }

   {
      lList *lp;
      
      lp = lCopyList("job args", lGetList(*pjob, JB_job_args));

      while ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_JOBARG))) {
         lAddElemStr(&lp, STR, lGetString(ep, SPA_argval_lStringT), ST_Type);
         
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
      sge_add_answer(&answer, str, STATUS_ENOIMP, 0);
   } 

   if (!(ep = lGetElemStr(lGetList(*pjob, JB_env_list), VA_variable, 
         "DISPLAY"))) {
      char *display;
      lList  *lp;
      lListElem *vep;
      char str[1024 + 1];

      display = sge_getenv("DISPLAY");
      if (!display || (*display == ':')) {
         lListElem *hep;
         char *ending = NULL;

         if (display) {
            ending = display;
         }

         hep = lCreateElem(EH_Type);
         lSetString(hep, EH_name, me.unqualified_hostname);
            
         switch (sge_resolve_host(hep, EH_name)) {
            case 0:
               break;
            case -1:
               sprintf(str, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S,
                  cl_errstr(-1));
            sge_add_answer(&answer, str, STATUS_EUNKNOWN, 0);
               lFreeElem(hep);
               DEXIT;
            return answer;
            default:
               sprintf(str, MSG_SGETEXT_CANTRESOLVEHOST_S, lGetString(hep, EH_name));
            sge_add_answer(&answer, str, STATUS_EUNKNOWN, 0);
               lFreeElem(hep);
            DEXIT;
            return answer;
         }
         display = malloc(strlen(lGetString(hep, EH_name)) + 4 + 1 +
                           (ending ? strlen(ending) : 0));
         strcpy(display, lGetString(hep, EH_name));
         strcat(display, (ending ? ending : ":0.0"));
         lFreeElem(hep);
      }

      lp = lGetList(*pjob, JB_env_list);
      vep = lCreateElem(VA_Type);
      lSetString(vep, VA_variable, "DISPLAY");
      lSetString(vep, VA_value, display);
      if (!lp) {
         lp = lCreateList("env list", VA_Type);
         lSetList(*pjob, JB_env_list, lp);
      }
      lAppendElem(lp, vep);
   }


   DEXIT;
   return answer;
}



