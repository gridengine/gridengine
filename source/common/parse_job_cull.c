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
#include <ctype.h> 
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "def.h"
#include "symbols.h"
#include "sge_gdi_intern.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_answerL.h"
#include "sge_rangeL.h"
#include "sge_string.h"
#include "sge_time.h"
#include "parse_qsubL.h"
#include "sge_stringL.h"
#include "sge_identL.h"
#include "sge_job_refL.h"
#include "sge_str_from_file.h"
#include "sge_me.h"
#include "sge_resource.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "path_aliases.h"
#include "parse.h"
#include "sgermon.h"
#include "cull_parse_util.h"
#include "utility.h"
#include "unparse_job_cull.h"
#include "sge_language.h"
#include "sge_feature.h"
#include "msg_common.h"
#include "jb_now.h"
#include "sge_range.h"
#include "job.h"
#include "job.h"

#define USE_CLIENT_QSUB 1

/*
** set the correct defines
** USE_CLIENT_QSUB or
** USE_CLIENT_QALTER or
** USE_CLIENT_QSH
*/

static void strip_quotes(char **pstr);
/* static int skip_line(char *s); */

/* returns true if line has only white spaces */
/* static int skip_line( */
/* char *s  */
/* ) { */
/*    while ( *s != '\0' && *s != '\n' && isspace((int)*s))  */
/*       s++; */
/*    return (*s == '\0' || *s == '\n'); */
/* } */

/*
** NAME
**   cull_parse_job_parameter
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
lList *cull_parse_job_parameter(
lList *cmdline,
lListElem **pjob 
) {
   char *cp;
   lListElem *ep;
   lList *answer = NULL;
   lList *path_alias = NULL;

   DENTER(TOP_LAYER, "cull_parse_job_parameter"); 

   if (!pjob) {
      sge_add_answer(&answer,  MSG_PARSE_NULLPOINTERRECEIVED , STATUS_EUNKNOWN, 0);
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

   if (!lGetList(*pjob, JB_ja_tasks)) {
      lList *tmpl_task_list = NULL; /* JAT_Type */
      lListElem *tmpl_task = NULL;  /* JAT_Type */

      tmpl_task_list = lCreateList("template task list", JAT_Type);
      tmpl_task = lCreateElem(JAT_Type);
      if (!tmpl_task_list || !tmpl_task) {
         sge_add_answer(&answer, MSG_MEM_MEMORYALLOCFAILED, STATUS_EMALLOC, 0);
         DEXIT;
         return answer;
      }
      lAppendElem(tmpl_task_list, tmpl_task);
      lSetList(*pjob, JB_ja_tasks, tmpl_task_list); 
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

   DTRACE;

   /*
    * -t
    */
   {
      lList *range_list = NULL;
      lList *n_h_list, *u_h_list, *o_h_list, *s_h_list;

      ep = lGetElemStr(cmdline, SPA_switch, "-t");
      if (ep) {
         lListElem *range_elem = NULL; /* RN_Type */
         u_long32 start, end, step;

         range_list = lGetList(ep, SPA_argval_lListT);
         range_elem = lFirst(range_list);
         start = lGetUlong(range_elem, RN_min);
         end = lGetUlong(range_elem, RN_max);
         step = lGetUlong(range_elem, RN_step);
         job_set_ja_task_ids(*pjob, start, end, step);
      } else {
         job_set_ja_task_ids(*pjob, 1, 1, 1);
         range_list = lGetList(*pjob, JB_ja_structure);
      }
      n_h_list = lCopyList("range list", range_list);
      u_h_list = lCreateList("user hold list", RN_Type);
      o_h_list = lCreateList("operator hold list", RN_Type);
      s_h_list = lCreateList("system hold list", RN_Type);
      if (!n_h_list || !u_h_list || !o_h_list || !s_h_list) {
         sge_add_answer(&answer, MSG_MEM_MEMORYALLOCFAILED, STATUS_EMALLOC, 0);         DEXIT;
         return answer;
      }     
      lSetList(*pjob, JB_ja_n_h_ids, n_h_list);
      lSetList(*pjob, JB_ja_u_h_ids, u_h_list);
      lSetList(*pjob, JB_ja_o_h_ids, o_h_list);
      lSetList(*pjob, JB_ja_s_h_ids, s_h_list);

#if 0 /* EB: Test */
      {
         lListElem *elem;
         u_long32 id;

         StringBufferT string = {NULL, 0};
         lWriteListTo(range_list, stderr);
      
         range_remove_id(range_list, 5);

         range_print_to_string(range_list, &string);
         fprintf(stderr, "%s\n", string.s);
         sge_string_free(&string);
         for_each_id_in_range_list(id, elem, range_list) {
            fprintf(stderr, u32"\n", id); 
         }
         exit (1); 
      }
#endif


      if (ep) {
         lRemoveElem(cmdline, ep);
      }
   }

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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-a"))) {
      lSetUlong(*pjob, JB_execution_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-A"))) {
      /* the old account string is overwritten */
      lSetString(*pjob, JB_account, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-dl"))) {
      lSetUlong(*pjob, JB_deadline, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(cmdline, ep);
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-c"))) {
      if (lGetUlong(ep, SPA_argtype) == lLongT)
         lSetUlong(*pjob, JB_checkpoint_interval, lGetLong(ep, SPA_argval_lLongT));
      if (lGetUlong(ep, SPA_argtype) == lIntT) 
         lSetUlong(*pjob, JB_checkpoint_attr, lGetInt(ep, SPA_argval_lIntT));

      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-cell"))) {
      lSetString(*pjob, JB_cell, lGetString(ep, SPA_argval_lStringT));
      me.default_cell = sge_strdup(me.default_cell, 
         lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-ckpt"))) {
      lSetString(*pjob, JB_checkpoint_object, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-P"))) {
         lSetString(*pjob, JB_project, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
      }
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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-C"))) {
      lSetString(*pjob, JB_directive_prefix, 
         lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

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
   parse_list_simple(cmdline, "-e", *pjob, JB_stderr_path_list, PN_host, PN_path, FLG_LIST_MERGE);

   /* -h */
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

   /* -hold_jid */
   if (lGetElemStr(cmdline, SPA_switch, "-hold_jid")) {
      lListElem *ep, *sep;
      lList *jref_list = NULL;
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-hold_jid"))) {
         for_each(sep, lGetList(ep, SPA_argval_lListT)) {
            DPRINTF(("-hold_jid %s\n", lGetString(sep, STR)));
            lAddElemStr(&jref_list, JRE_job_name, lGetString(sep, STR), JRE_Type);
         }
         lRemoveElem(cmdline, ep);
      }
      lSetList(*pjob, JB_jid_predecessor_list, jref_list);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-j"))) {
      lSetUlong(*pjob, JB_merge_stderr, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   parse_list_simple(cmdline, "-jid", *pjob, JB_job_identifier_list, 
                        0, 0, FLG_LIST_APPEND);

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

#ifndef USE_CLIENT_QALTER
   if (!lGetList(*pjob, JB_mail_list)) {   
      ep = lAddSubStr(*pjob, MR_user, me.user_name, JB_mail_list, MR_Type);
      lSetString(ep, MR_host, me.qualified_hostname);
   }
#endif

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-N"))) {
      lSetString(*pjob, JB_job_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-notify"))) {
      lSetUlong(*pjob, JB_notify, TRUE);
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
   
   /*
   ** this is exactly the same as for error list and it would be nice
   ** to have generalized funcs, that do all this while stuff and so
   ** on and are encapsulated in a function where I can give in the
   ** job element pointer the field name the option list and the option
   ** name and the field is filled
   */
   parse_list_simple(cmdline, "-o", *pjob, JB_stdout_path_list, PN_host, PN_path, FLG_LIST_MERGE);

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-p"))) {
      lSetUlong(*pjob, JB_priority, 
         BASE_PRIORITY + lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-P"))) {
      lSetString(*pjob, JB_project, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-pe"))) {
      lSetString(*pjob, JB_pe, lGetString(ep, SPA_argval_lStringT));
      lSwapList(*pjob, JB_pe_range, ep, SPA_argval_lListT);
      lRemoveElem(cmdline, ep);
   }

   parse_list_hardsoft(cmdline, "-q", *pjob, 
                        JB_hard_queue_list, JB_soft_queue_list);

   parse_list_hardsoft(cmdline, "-masterq", *pjob, 
                        JB_master_hard_queue_list, 0);
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-qs_args"))) {
      lSwapList(*pjob, JB_qs_args, ep, SPA_argval_lListT);
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-r"))) {
      lSetUlong(*pjob, JB_restart, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   /* not needed in job struct */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-soft"))) {
      lRemoveElem(cmdline, ep);
   }

   parse_list_simple(cmdline, "-S", *pjob, JB_shell_list, PN_host, PN_path, FLG_LIST_MERGE);

   parse_list_simple(cmdline, "-u", *pjob, JB_user_list, 0, 0, FLG_LIST_APPEND);

   /*
   ** to be processed in original order, set -V equal to -v
   */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-V"))) {
      lSetString(ep, SPA_switch, "-v");
   }
   parse_list_simple(cmdline, "-v", *pjob, JB_env_list, VA_variable, VA_value, FLG_LIST_MERGE);
   cull_compress_definition_list(lGetList(*pjob, JB_env_list), VA_variable, VA_value, 0);

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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-verify"))) {
      lSetUlong(*pjob, JB_verify, TRUE);
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-w"))) {
      lSetUlong(*pjob, JB_verify_suitable_queues, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   /*
   ** no switch - must be scriptfile
   ** only for qalter and qsub, not for qsh
   */
   if ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_SCRIPT))) {
      lSetString(*pjob, JB_script_file, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_SCRIPTLEN))) {
      lSetUlong(*pjob, JB_script_size, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(cmdline, ep);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, STR_PSEUDO_SCRIPTPTR))) {
      lSetString(*pjob, JB_script_ptr, lGetString(ep, SPA_argval_lStringT));
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

   cp = lGetString(*pjob, JB_script_file);
   if (!cp || !strcmp(cp, "-")) {
      lSetString(*pjob, JB_script_file, "STDIN");
   }
   cp = lGetString(*pjob, JB_job_name);
   if (!cp) {
      cp = sge_basename(lGetString(*pjob, JB_script_file), '/');
      lSetString(*pjob, JB_job_name,  cp);
   }

#if 0
   {
      lListElem *tmpl_task = NULL;
   
      tmpl_task = lFirst(lGetList(*pjob, JB_ja_tasks));
      if (lGetUlong(tmpl_task, JAT_hold)) {
         lSetUlong(tmpl_task, JAT_state, JHELD);
      }
   }
#endif

   DEXIT;
   return answer;
}

/****** parse_job_cull/parse_script_file() *************************************
*  NAME
*     parse_script_file() -- parse a job script or a job defaults file
*
*  SYNOPSIS
*     lList* parse_script_file(char *script_file, 
*                              const char *directive_prefix, 
*                              lList **lpp_options, 
*                              char **envp, 
*                              u_long32 flags);
*
*  FUNCTION
*     Searches for special comments in script files and parses contained 
*     SGE options or
*     parses SGE options in job defaults files (sge_request).
*     Script files are parsed with directive prefix NULL,
*     default files are parsed with directive prefix "" and FLG_USE_NO_PSEUSOS.
*
*  INPUTS
*     char *script_file      - script file name or NULL or "-"
*                              in the latter two cases the job script is read 
*                              from stdin
*     char *directive_prefix - only lines beginning with this prefix are parsed
*                              NULL causes function to look in the lpp_options 
*                              list whether the -C option has been set. 
*                              If it has, this prefix is used.
*                              If not, the default prefix "#$" is used. 
*                              "" causes the function to parse all lines not 
*                              starting with "#" (comment lines).
*     lList **lpp_options    - list pointer-pointer, SPA_Type
*                              list to store the recognized switches in, is 
*                              created if it doesnt exist but there are 
*                              options to be returned
*     char **envp            - environment pointer
*     u_long32 flags         - FLG_HIGHER_PRIOR:    new options are appended 
*                                                   to list
*                              FLG_LOWER_PRIOR:     new options are inserted 
*                                                   at the beginning of list
*                              FLG_USE_NO_PSEUDOS:  do not create pseudoargs 
*                                                   for script pointer and 
*                                                   length
*                              FLG_DONT_ADD_SCRIPT: do not add the script as a
*                                                   pseudoarg if it is not yet 
*                                                   there
*
*  RESULT
*     lList* - answer list, AN_Type, or NULL if everything was ok, 
*              the following stati can occur:
*                 STATUS_EUNKNOWN - bad internal error like NULL pointer 
*                                   received or no memory
*                 STATUS_EDISK    - file could not be opened
*                 stati returned by cull_parse_cmdline
*
*  NOTES
*     Special comments in script files have to start in the first column of a 
*     line.
*     Comments in job defaults files have to start in the first column of a 
*     line.
*     If a line is longer than MAX_STRING_SIZE bytes, contents (SGE options) 
*     starting from position MAX_STRING_SIZE + 1 are silently ignored.
*     MAX_STRING_SIZE is defined in common/basis_types.h (current value 2048).
*
*  SEE ALSO
*     cull_parse_cmdline()
*     basis_types.h
*     cod_request(5)
*******************************************************************************/
lList *parse_script_file(
const char *script_file,
const char *directive_prefix,
lList **lpp_options, 
char **envp,
u_long32 flags 
) {
   static const char default_prefix[] = "#$";
   unsigned int dpl; /* directive_prefix length */
   FILE *fp;
   char *filestrptr;
   int script_len;
   char **str_table;
   lList *alp, *answer = NULL; 
   lListElem *aep;
   int i;
   int do_exit = 0;
   lListElem *ep_opt;
   lList *lp_new_opts = NULL;
   char buffer[MAX_STRING_SIZE];
   char *s;


   DENTER(TOP_LAYER, "parse_script_file");

   if (!lpp_options) {
      /* no place where to put result */
      sge_add_answer(&answer, MSG_ANSWER_CANTPROCESSNULLLIST, STATUS_EUNKNOWN, 0);
      DEXIT;
      return answer;
   }

   if (script_file && strcmp(script_file, "-")) {
      /* are we able to access this file? */
      if ((fp = fopen(script_file, "r")) == NULL) {
         char str[1024 + 1];

         sprintf(str, MSG_FILE_ERROROPENINGXY_SS, script_file, strerror(errno));
         sge_add_answer(&answer, str, STATUS_EDISK, 0);
         DEXIT;
         return answer;
      }
      
      fclose(fp);

      /* read the script file in one sweep */
      filestrptr = str_from_file(script_file, &script_len);

      if (!filestrptr) {
         char str[1024 + 1];

         sprintf(str, MSG_ANSWER_ERRORREADINGFROMFILEX_S, script_file);
         sge_add_answer(&answer, str, STATUS_EUNKNOWN, 0);
         DEXIT;
         return answer;
      }
   } else {
      /* no script file but input from stdin */
      filestrptr = str_from_stream(stdin, &script_len);
      if (!filestrptr) {
         sge_add_answer(&answer, MSG_ANSWER_ERRORREADINGFROMSTDIN, STATUS_EUNKNOWN, 0);
         DEXIT;
         return answer;
      }
   }

   /* no prefix for special comments given - try to find one in current commandline
   ** or take default
   */
   if (!directive_prefix) {
      lListElem *ep;

      ep = lGetElemStr(*lpp_options, SPA_switch, "-C");
      if (ep) {
         directive_prefix = lGetString(ep, SPA_argval_lStringT);
      }
      if (!directive_prefix) {
         directive_prefix = default_prefix;
      }
   }
   dpl = strlen(directive_prefix);

   /* now look for job parameters in script file */
   s = filestrptr;
   while(*s != 0) {
      int length = 0;
      char *parameters;
      char *d = buffer;

      /* copy MAX_STRING_SIZE bytes maximum */
      while(*s != 0 && *s != '\n' && length < MAX_STRING_SIZE - 1) {
         *d++ = *s++;
      }

      /* terminate target string */
      *d = 0;
      
      /* skip linefeed */
      if(*s == '\n') {
         s++;
      }
      
      parameters = buffer;

      /*
      ** If directive prefix is zero string then all lines except
      ** comment lines are read, this makes it possible to parse
      ** defaults files with this function.
      ** If the line contains no SGE options, we set parameters to NULL.
      */

      if(dpl == 0) {
         /* we parse a settings file (e.g. sge_request): skip comment lines */
         if(*parameters == '#') {
            parameters = NULL;
         }
      } else {
         /* we parse a script file with special comments */
         if(strncmp(parameters, directive_prefix, dpl) == 0) {
            parameters += dpl;
            while(isspace(*parameters)) {
               parameters++;
            }
         } else {
            parameters = NULL;
         }
      }

      /* delete trailing garbage */
      if(parameters != NULL) {
         for(i = strlen(parameters) - 1; i >= 0; i--) {
            if(isspace(parameters[i])) {
               parameters[i] = 0;
            } else {
               break;
            }
         }
      }

      if(parameters != NULL && *parameters != 0) {
         DPRINTF(("parameter in script: %s\n", parameters));

         /*
         ** here cull comes in 
         */

         /* so str_table has to be freed afterwards */
         str_table = string_list(parameters, " \t\n", NULL);
         
         for (i=0; str_table[i]; i++) {
            DPRINTF(("str_table[%d] = '%s'\n", i, str_table[i]));           
         }
         strip_quotes(str_table);
         for (i=0; str_table[i]; i++) {
            DPRINTF(("str_table[%d] = '%s'\n", i, str_table[i]));           
         }

         /*
         ** problem: error handling missing here and above
         */
         alp = cull_parse_cmdline(str_table, envp, &lp_new_opts, 0);

         for_each(aep, alp) {
            u_long32 quality;
            u_long32 status = STATUS_OK;

            status = lGetUlong(aep, AN_status);
            quality = lGetUlong(aep, AN_quality);
            if (quality == NUM_AN_ERROR) {
               DPRINTF(("%s", lGetString(aep, AN_text)));
               do_exit = 1;
            }
            else {
               DPRINTF(("Warning: %s\n", lGetString(aep, AN_text)));
            }
            sge_add_answer(&answer, lGetString(aep, AN_text), status, quality);
         }

         lFreeList(alp);
         if (do_exit) {
            DEXIT;
            return answer;
         }

         free((char*) str_table);

         /*
         ** -C option is ignored in scriptfile - delete all occurences
         */
         {
            lListElem *ep;
            
            while ((ep = lGetElemStr(lp_new_opts, SPA_switch, "-C"))) {
               lRemoveElem(lp_new_opts, ep);
            }
         }
      }
   }
   
   if (!(flags & FLG_USE_NO_PSEUDOS)) {
      /*
      ** if script is not yet there we add it to the command line,
      ** except if the caller requests not to
      */
      if (!(flags & FLG_DONT_ADD_SCRIPT)) {
         if (!lpp_options || !lGetElemStr(*lpp_options, SPA_switch, STR_PSEUDO_SCRIPT)) {
            ep_opt = sge_add_arg(&lp_new_opts, 0, lStringT, STR_PSEUDO_SCRIPT, NULL);
            lSetString(ep_opt, SPA_argval_lStringT, 
               ((!script_file || !strcmp(script_file, "-")) ? "STDIN" : script_file));
         }
      }
      ep_opt = sge_add_arg(&lp_new_opts, 0, lUlongT, STR_PSEUDO_SCRIPTLEN, NULL);
      lSetUlong(ep_opt, SPA_argval_lUlongT, script_len);

      ep_opt = sge_add_arg(&lp_new_opts, 0, lStringT, STR_PSEUDO_SCRIPTPTR, NULL);
      lSetString(ep_opt, SPA_argval_lStringT, filestrptr);
   }

   if (!lp_new_opts) {
      DEXIT;
      return answer;
   }

   if (!*lpp_options) {
      *lpp_options = lp_new_opts;
   } else {
      if (flags & FLG_LOWER_PRIOR) {
         lAddList(lp_new_opts, *lpp_options);
         *lpp_options = lp_new_opts;
      } else {
         lAddList(*lpp_options, lp_new_opts);
         lp_new_opts = NULL;
      }
   }

   DEXIT;
   return answer;
}


static void strip_quotes(
char **pstr 
) {
   char *cp, *cp2;

   DENTER(TOP_LAYER, "strip_quotes");

   if (!pstr) {
      DEXIT;
      return;
   }

   for (; *pstr; pstr++) {
      for (cp2 = cp = *pstr; *cp; cp++) {
         if (*cp == '"') {
            *cp2++ = *cp;
         }
      }
   }

   DEXIT;
   return;
}


/****** src/add_parent_uplink() **********************************
*
*  NAME
*     add_parent_uplink() -- add PARENT jobid to job context 
*
*  SYNOPSIS
*
*     #include "parse_job_cull.h"
*     #include <src/parse_job_cull.h>
* 
*     void add_parent_uplink(lListElem *job);
*       
*
*  FUNCTION
*     If we have JOB_ID in environment implicitly put it into the 
*     job context variable PARENT if was not explicitly set using 
*     "-sc PARENT=$JOBID". By doing this we preserve information 
*     about the relationship between these two jobs.
* 
*  INPUTS
*    job - the job structure
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     src/()
*     src/()
*     
****************************************************************************
*/
void add_parent_uplink(
lListElem *job 
) {
   char *job_id;
   lListElem *cp;
   if ((job_id=getenv("JOB_ID")) && 
            !lGetSubStr(job, VA_variable, CONTEXT_PARENT, JB_context)) { 
      cp = lAddSubStr(job, VA_variable, CONTEXT_PARENT, JB_context, VA_Type);
      lSetString(cp, VA_value, job_id);
   }
}
