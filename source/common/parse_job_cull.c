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

#include "symbols.h"
#include "sge_ja_task.h"
#include "sge_string.h"
#include "sge_time.h"
#include "parse_qsubL.h"
#include "sge_str.h"
#include "sge_identL.h"
#include "sge_job_refL.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "sge_path_alias.h"
#include "parse.h"
#include "sgermon.h"
#include "cull_parse_util.h"
#include "unparse_job_cull.h"
#include "sge_language.h"
#include "sge_feature.h"
#include "msg_common.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_stdlib.h"
#include "sge_io.h"
#include "sge_prog.h"
#include "sge_var.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_mailrec.h"
#include "sge_centry.h"

#define USE_CLIENT_QSUB 1

/*
** set the correct defines
** USE_CLIENT_QSUB or
** USE_CLIENT_QALTER or
** USE_CLIENT_QSH
*/

const char *default_prefix = "#$";

static char *reroot_path (lListElem* pjob, const char *path, lList **alpp);
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
lList *cull_parse_job_parameter(lList *cmdline, lListElem **pjob) 
{
   const char *cp;
   lListElem *ep;
   lList *answer = NULL;
   lList *path_alias = NULL;
   char error_string[1024 + 1];

   DENTER(TOP_LAYER, "cull_parse_job_parameter"); 

   if (!pjob) {
      answer_list_add(&answer,  MSG_PARSE_NULLPOINTERRECEIVED, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   if (!*pjob) {
      *pjob = lCreateElem(JB_Type);
      if (!*pjob) {
         sprintf(SGE_EVENT, MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
         answer_list_add(&answer, SGE_EVENT,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
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
         sprintf(SGE_EVENT, MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
         answer_list_add(&answer, SGE_EVENT, 
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
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


   /*
    * -b
    */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-b"))) {
      if (lGetInt(ep, SPA_argval_lIntT) == 1) {
         u_long32 jb_now = lGetUlong(*pjob, JB_type);

         JOB_TYPE_SET_BINARY(jb_now);
         lSetUlong(*pjob, JB_type, jb_now);
      }
      lRemoveElem(cmdline, ep);
   }

   /*
    * -t
    */
   ep = lGetElemStr(cmdline, SPA_switch, "-t");
   if (ep != NULL) {
      lList *range_list = lGetList(ep, SPA_argval_lListT);
      lList *new_range_list = lCopyList("",  range_list);

      lSetList(*pjob, JB_ja_structure, new_range_list);
      lRemoveElem(cmdline, ep);
   
      {
         u_long32 job_type = lGetUlong(*pjob, JB_type);
         JOB_TYPE_SET_ARRAY(job_type);
         lSetUlong(*pjob, JB_type, job_type);
      }
   } else {
      job_set_submit_task_ids(*pjob, 1, 1, 1);
   }
   job_initialize_id_lists(*pjob, &answer);
   if (answer != NULL) {
      DEXIT;
      return answer;
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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-a"))) {
      lSetUlong(*pjob, JB_execution_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-A"))) {
      /* the old account string is overwritten */
      lSetString(*pjob, JB_account, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-dl"))) {
         lSetUlong(*pjob, JB_deadline, lGetUlong(ep, SPA_argval_lUlongT));
         lRemoveElem(cmdline, ep);
      }
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-c"))) {
      if (lGetUlong(ep, SPA_argtype) == lLongT)
         lSetUlong(*pjob, JB_checkpoint_interval, lGetLong(ep, SPA_argval_lLongT));
      if (lGetUlong(ep, SPA_argtype) == lIntT) 
         lSetUlong(*pjob, JB_checkpoint_attr, lGetInt(ep, SPA_argval_lIntT));

      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-ckpt"))) {
      lSetString(*pjob, JB_checkpoint_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-cwd"))) {
      char tmp_str[SGE_PATH_MAX + 1];
      char *path = NULL;

      if (!getcwd(tmp_str, sizeof(tmp_str))) {
         /* If getcwd() fails... */
         answer_list_add(&answer, MSG_ANSWER_GETCWDFAILED, 
                         STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }
      
      path = reroot_path (*pjob, tmp_str, &answer);
      
      if (path == NULL) {
         DEXIT;
         return answer;
      }
      
      lSetString(*pjob, JB_cwd, path);
      lRemoveElem(cmdline, ep);
      
      lSetList(*pjob, JB_path_aliases, lCopyList("PathAliases", path_alias));
      
      FREE (path);
   }

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
      if (lGetInt(ep, SPA_argval_lIntT) & MINUS_H_TGT_USER) {
         lSetList(*pjob, JB_ja_u_h_ids, lCopyList("user hold ids",
                  lGetList(*pjob, JB_ja_n_h_ids)));
         lSetList(*pjob, JB_ja_n_h_ids, NULL);
      }
      lRemoveElem(cmdline, ep);
   }

   /* not needed in job struct */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-hard"))) {
      lRemoveElem(cmdline, ep);
   }

   if ((ep = lGetElemStr(cmdline, SPA_switch, "-help"))) {
      lRemoveElem(cmdline, ep);
      sprintf(error_string, MSG_ANSWER_HELPNOTALLOWEDINCONTEXT);
      answer_list_add(&answer, error_string, 
                      STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
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

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-j"))) {
      lSetBool(*pjob, JB_merge_stderr, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   parse_list_simple(cmdline, "-jid", *pjob, JB_job_identifier_list, 
                        0, 0, FLG_LIST_APPEND);

   parse_list_hardsoft(cmdline, "-l", *pjob, 
                        JB_hard_resource_list, JB_soft_resource_list);

   centry_list_remove_duplicates(lGetList(*pjob, JB_hard_resource_list));
   centry_list_remove_duplicates(lGetList(*pjob, JB_soft_resource_list));

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
      ep = lAddSubStr(*pjob, MR_user, uti_state_get_user_name(), JB_mail_list, MR_Type);
      lSetHost(ep, MR_host, uti_state_get_qualified_hostname());
   }
#endif

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-N"))) {
      lSetString(*pjob, JB_job_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(cmdline, ep);
   }
   
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-notify"))) {
      lSetBool(*pjob, JB_notify, TRUE);
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
   
   /*
   ** this is exactly the same as for error list and it would be nice
   ** to have generalized funcs, that do all this while stuff and so
   ** on and are encapsulated in a function where I can give in the
   ** job element pointer the field name the option list and the option
   ** name and the field is filled
   */
   parse_list_simple(cmdline, "-o", *pjob, JB_stdout_path_list, PN_host, 
                     PN_path, FLG_LIST_MERGE);
   parse_list_simple(cmdline, "-i", *pjob, JB_stdin_path_list, PN_host, 
                     PN_path, FLG_LIST_MERGE);

   while ((ep = lGetElemStr(cmdline, SPA_switch, "-p"))) {
      lSetUlong(*pjob, JB_priority, 
         BASE_PRIORITY + lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(cmdline, ep);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      while ((ep = lGetElemStr(cmdline, SPA_switch, "-P"))) {
         lSetString(*pjob, JB_project, lGetString(ep, SPA_argval_lStringT));
         lRemoveElem(cmdline, ep);
      }
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

   /* There is technically no conflict between -cwd and -wd.  They both set the
    * working directory, and by virtue of it's position in this method, -wd will
    * always override -cwd.  However, at least at the moment, -wd can only come
    * from the DRMAA attributes, and -cwd cannot come from the DRMAA attributes.
    * Therefore, -wd should override -cwd.  Lucky me. */
   while ((ep = lGetElemStr(cmdline, SPA_switch, "-wd"))) {
      const char *wd = lGetString(ep, SPA_argval_lStringT);

/* I've added a -wd option to cull_parse_job_parameter() to deal with the
 * DRMAA_WD attribute.  It makes sense to me that since -wd exists and is
 * handled by cull_parse_job_parameter() that -cwd should just become an alias
 * for -wd.  The code to do that is ifdef'ed out below just in case we decide
 * it's a good idea. */
#if 0
      if (strcmp (wd, SGE_HOME_DIRECTORY) == 0) {
         wd = (char *)malloc (sizeof (char) * (SGE_PATH_MAX + 1));

         if (!getcwd (wd, sizeof (wd))) {
            /* If getcwd() fails... */
            answer_list_add (&answer, MSG_ANSWER_GETCWDFAILED, 
                             STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
      }
#endif

      char *path = reroot_path (*pjob, wd, &answer);
      
      if (path == NULL) {
         DEXIT;
         return answer;
      }
      
      lSetString(*pjob, JB_cwd, path);
      lRemoveElem(cmdline, ep);
   
      /* If -cwd didn't already set the JB_path_aliases field, set it. */
      if (lGetList (*pjob, JB_path_aliases) == NULL) {
         lSetList(*pjob, JB_path_aliases, lCopyList("PathAliases", path_alias));
      }
      
      FREE (path);
   }
   
   path_alias = lFreeList(path_alias);
   
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
         lAddElemStr(&lp, ST_name, lGetString(ep, SPA_argval_lStringT), ST_Type);
         
         lRemoveElem(cmdline, ep);
      }
      lSetList(*pjob, JB_job_args, lp);
   }
   
   for_each(ep, cmdline) {
      sprintf(error_string, MSG_ANSWER_UNKOWNOPTIONX_S, 
         lGetString(ep, SPA_switch));
      cp = lGetString(ep, SPA_switch_arg);
      if (cp) {
         strcat(error_string, " ");
         strcat(error_string, cp);
      }
      strcat(error_string, "\n");
      answer_list_add(&answer, error_string, 
                      STATUS_ENOIMP, ANSWER_QUALITY_ERROR);
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

   DEXIT;
   return answer;
}

/****** client/parse_job_cull/parse_script_file() *****************************
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
*
*  RESULT
*     lList* - answer list, AN_Type, or NULL if everything was ok, 
*              the following stati can occur:
*                 STATUS_EUNKNOWN - bad internal error like NULL pointer 
*                                   received or no memory
*                 STATUS_EDISK    - file could not be opened
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
*     centry_list_parse_from_string()
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
   unsigned int dpl; /* directive_prefix length */
   FILE *fp;
   char *filestrptr = NULL;
   int script_len;
   int parsed_chars = 0;
   char **str_table;
   lList *alp, *answer = NULL; 
   lListElem *aep;
   int i;
   int do_exit = 0;
   lListElem *ep_opt;
   lList *lp_new_opts = NULL;
   char buffer[MAX_STRING_SIZE];
   char *s;
   char error_string[1024 + 1];


   DENTER(TOP_LAYER, "parse_script_file");

   if (!lpp_options) {
      /* no place where to put result */
      answer_list_add(&answer, MSG_ANSWER_CANTPROCESSNULLLIST, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   if (!(flags & FLG_IGNORE_EMBEDED_OPTS)) {
      if (script_file && strcmp(script_file, "-")) {
         /* are we able to access this file? */
         if ((fp = fopen(script_file, "r")) == NULL) {
            sprintf(error_string, MSG_FILE_ERROROPENINGXY_SS, script_file, 
                    strerror(errno));
            answer_list_add(&answer, error_string, 
                            STATUS_EDISK, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         
         fclose(fp);

         /* read the script file in one sweep */
         filestrptr = sge_file2string(script_file, &script_len);

         if (!filestrptr) {
            sprintf(error_string, MSG_ANSWER_ERRORREADINGFROMFILEX_S, 
                    script_file);
            answer_list_add(&answer, error_string, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
      } else {
         /* no script file but input from stdin */
         filestrptr = sge_stream2string(stdin, &script_len);
         if (!filestrptr) {
            answer_list_add(&answer, MSG_ANSWER_ERRORREADINGFROMSTDIN, 
                            STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
      }

      if (directive_prefix == NULL) {
         DPRINTF(("directive prefix = <null> - will skip parsing of script\n"));
         dpl = 0;
      } else {
         DPRINTF(("directive prefix = "SFQ"\n", directive_prefix));
         dpl = strlen(directive_prefix);
      }

      if (directive_prefix != NULL) {
         /* now look for job parameters in script file */
         s = filestrptr;
         while(*s != 0) {
            int length = 0;
            char *parameters = NULL;
            char *d = buffer;

            /* copy MAX_STRING_SIZE bytes maximum */
            while(*s != 0 && *s != '\n' && length < MAX_STRING_SIZE - 1) {
               *d++ = *s++;
               length++;
               parsed_chars++;
            }

            /* terminate target string */
            *d = 0;
            
            /* skip linefeed */
            if(*s == '\n') {
               s++;
               parsed_chars++;
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
               sge_strip_quotes(str_table);
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
                  if (quality == ANSWER_QUALITY_ERROR) {
                     DPRINTF(("%s", lGetString(aep, AN_text)));
                     do_exit = 1;
                  }
                  else {
                     DPRINTF(("Warning: %s\n", lGetString(aep, AN_text)));
                  }
                  answer_list_add(&answer, lGetString(aep, AN_text), status, quality);
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

static char *reroot_path (lListElem* pjob, const char *path, lList **alpp) {
   const char *home = job_get_env_string(pjob, VAR_PREFIX "O_HOME");
   char tmp_str[SGE_PATH_MAX + 1];
   char tmp_str2[SGE_PATH_MAX + 1];
   char tmp_str3[SGE_PATH_MAX + 1];
   
   DENTER (TOP_LAYER, "reroot_path");
   
   strcpy (tmp_str, path);
   
   if (!chdir(home)) {
      /* If chdir() succeeds... */
      if (!getcwd(tmp_str2, sizeof(tmp_str2))) {
         /* If getcwd() fails... */
         answer_list_add(alpp, MSG_ANSWER_GETCWDFAILED, 
                         STATUS_EDISK, ANSWER_QUALITY_ERROR);
         DEXIT;
         return NULL;
      }

      chdir(tmp_str);

      if (!strncmp(tmp_str2, tmp_str, strlen(tmp_str2))) {
         /* If they are equal, build a new CWD using the value of the HOME
          * as the root instead of whatever that directory is called by
          * the -wd path. */
         sprintf(tmp_str3, "%s%s", home, (char *) tmp_str + strlen(tmp_str2));
         strcpy(tmp_str, tmp_str3);
      }
   }
   
   DEXIT;
   return strdup (tmp_str);
}