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

#include "symbols.h"
#include "sge_all_listsL.h"
#include "parse_qsubL.h"
#include "parse_job_cull.h"
#include "sge_mailrec.h"
#include "parse_qsub.h"
#include "sge_feature.h"
#include "sge_userset.h"
#include "sge_parse_num_par.h"
#include "parse.h"
#include "sge_options.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_parse_util.h"
#include "sge_string.h"
#include "sge_stdlib.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_ckpt.h"
#include "sge_ulong.h"
#include "sge_str.h"
#include "sge_centry.h"

#include "msg_common.h"

static int sge_parse_priority(lList **alpp, int *valp, char *priority_str);
static int var_list_parse_from_environment(lList **lpp, char **envp);
static int sge_parse_hold_list(char *hold_str);
static int sge_parse_mail_options(char *mail_str); 
static int cull_parse_destination_identifier_list(lList **lpp, char *dest_str);
static int sge_parse_checkpoint_interval(char *time_str); 
static int parse_hard_soft(int hs);
static int is_hard_soft(void);



static int hard_soft_flag = 0;

/*
** 0 is not yet set
** 1 is hard
** 2 is soft
*/
static int parse_hard_soft(
int hs 
) {
   DENTER(TOP_LAYER, "parse_hard_soft");
   /* be quiet at starting up */
   hard_soft_flag = hs; 

   DEXIT;
   return 0;
}

static int is_hard_soft(void)
{
   DENTER(TOP_LAYER, "usage_hard_soft");
   DEXIT;
   return hard_soft_flag;
}

/*
** NAME
**   cull_parse_cmdline
** PARAMETER
**   arg_list            - argument string list, e.g. argv,
**                         knows qsub, qalter and qsh options
**   envp                - pointer to environment
**   pcmdline            - NULL or pointer to list, SPA_Type
**                         set to contain the parsed options
**   flags               - FLG_USE_PSEUDOS: apply the following syntax:
**                         the first non-switch token is the job script, what
**                         follows are job arguments, mark the tokens as "script"
**                         or "jobarg" in the SPA_switch field
**                         0: dont do that, if a non-switch occurs, add it to
**                         the list with SPA_switch "" (an argument to a non-existing
**                         switch)
**                         FLG_QALTER: change treatment of non-options to qalter-specific
**                         pseudo options (only if FLG_USE_PSEUDOS is given) and changes
**                         parsing of -h
**
** RETURN
**   answer list, AN_Type or NULL if everything ok, the following stati can occur:
**   STATUS_EUNKNOWN   - bad internal error like NULL pointer received or no memory
**   STATUS_EEXIST     - option has been specified more than once, should be treated
**                       as a warning
**   STATUS_ESEMANTIC  - option that should have an argument had none
**   STATUS_ESYNTAX    - error parsing option argument
** DESCRIPTION
*/
lList *cull_parse_cmdline(
char **arg_list,
char **envp,
lList **pcmdline,
u_long32 flags 
) {
   char **sp;
   lList *answer = NULL;
   char str[1024 + 1];
   char arg[1024 + 1];
   lListElem *ep_opt;
   int i_ret;
   u_long32 is_qalter = flags & FLG_QALTER;
   bool is_hold_option = false;

   DENTER(TOP_LAYER, "cull_parse_cmdline");

   if (!arg_list || !pcmdline) {
      answer_list_add(&answer, MSG_PARSE_NULLPOINTERRECEIVED, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return answer;
   }

   sp = arg_list;

   /* reset hard/soft flag */
   parse_hard_soft(0);
   while (*sp) {

/*----------------------------------------------------------------------------*/
      /* "-a date_time */

      if (!strcmp("-a", *sp)) {
         u_long32 timeval;

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, 
                            STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field(s) is "date_time" */
         sp++;
         if (!*sp) {
            sprintf(str,
               MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-a");
            answer_list_add(&answer, str, 
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         if (!ulong_parse_date_time_from_string(&timeval, NULL, *sp))
         {
            sprintf(str,
               MSG_ANSWER_WRONGTIMEFORMATEXSPECIFIEDTOAOPTION_S,
            *sp);
            answer_list_add(&answer, str, 
                            STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         ep_opt = sge_add_arg(pcmdline, a_OPT, lUlongT, *(sp - 1), *sp);
         lSetUlong(ep_opt, SPA_argval_lUlongT, timeval);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-A account_string" */

      if (!strcmp("-A", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is account_string */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S
             ,"-A");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-A %s\"\n", *sp));

         ep_opt = sge_add_arg(pcmdline, A_OPT, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ac context_list */
      if (!strcmp("-ac", *sp)) {
         lList *variable_list = NULL;
         lListElem* lep = NULL;

         /* next field is context_list */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_ACOPTIONMUSTHAVECONTEXTLISTLISTARGUMENT);
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-ac %s\"\n", *sp));
         i_ret = var_list_parse_from_string(&variable_list, *sp, 0);
         if (i_ret) {
             sprintf(str, MSG_ANSWER_WRONGCONTEXTLISTFORMATAC_S , *sp);
             answer_list_add(&answer, str, 
                             STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, ac_OPT, lListT, *(sp - 1), *sp);
         lep = lCreateElem(VA_Type);
         lSetString(lep, VA_variable, "+");
         lInsertElem(variable_list, NULL, lep);
         lSetList(ep_opt, SPA_argval_lListT, variable_list);

         sp++;
         continue;
      }


/*----------------------------------------------------------------------------*/
      /* "-b y|n" */

      if (!is_qalter && !strcmp("-b", *sp)) {
         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str, MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S, *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, 
                            ANSWER_QUALITY_WARNING);
         }

         /* next filed is "y|n" */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-b");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, 
                             ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-b %s\"\n", *sp));

         if (!strcmp("y", *sp)) {
            ep_opt = sge_add_arg(pcmdline, b_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, 1);
         } else if (!strcmp("n", *sp)) {
            ep_opt = sge_add_arg(pcmdline, b_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, 2);
         } else {
             sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTBX_S, *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, 
                             ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;
      }


/*-----------------------------------------------------------------------------*/
      /* "-c [op] interval */

      if (!strcmp("-c", *sp)) {
          int attr;
          long interval;

         /* next field is [op] interval */
         sp++;
         if (!*sp) {
            sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, "-c");
            answer_list_add(&answer, str, 
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         DPRINTF(("\"%s %s\"\n", *(sp - 1), *sp));

         attr = sge_parse_checkpoint_attr(*sp);
         if (attr == 0) {
            interval = sge_parse_checkpoint_interval(*sp);
            if (!interval) {
               sprintf(str, MSG_PARSE_CARGUMENTINVALID);
               answer_list_add(&answer, str, 
                               STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
               DEXIT;
               return answer;
            }
            ep_opt = sge_add_arg(pcmdline, c_OPT, lLongT, *(sp - 1), *sp);
            lSetLong(ep_opt, SPA_argval_lLongT, (long) interval);
         }
         else if (attr == -1) {
            sprintf(str, MSG_PARSE_CSPECIFIERINVALID);
            answer_list_add(&answer, str, 
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         else {
            ep_opt = sge_add_arg(pcmdline, c_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, attr);
         }

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-cell cell_name or -ckpt ckpt_object" */

      /* This is another small change that needed to be made to allow DRMAA to
       * resuse this method.  The -cat option doesn't actually exist for any
       * command line utility.  It is a qsub style representation of the
       * DRMAA_JOB_CATEGORY attribute.  It will be processed and removed by the
       * drmaa_job2sge_job method. */
      if (!strcmp ("-cat", *sp) || !strcmp("-cell", *sp) || !strcmp("-ckpt", *sp)) {

         /* next field is job_cat or cell_name or ckpt_object*/
         sp++;
         if (!*sp) {
            sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S
               , *(sp - 1));
            answer_list_add(&answer, str, 
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         DPRINTF(("\"%s %s\"\n", *(sp - 1), *sp));

         ep_opt = sge_add_arg(pcmdline, 0, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-clear" */

      if (!strcmp("-clear", *sp)) {

         DPRINTF(("\"%s\"\n", *sp));
         ep_opt = sge_add_noarg(pcmdline, clear_OPT, *sp, NULL);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-cwd" */

      if (!strcmp("-cwd", *sp)) {
/* I've added a -wd option to cull_parse_job_parameter() to deal with the
 * DRMAA_WD attribute.  It makes sense to me that since -wd exists and is
 * handled by cull_parse_job_parameter() that -cwd should just become an alias
 * for -wd.  The code to do that is ifdef'ed out below just in case we decide
 * it's a good idea. */
#if 0
         ep_opt = sge_add_arg (args, wd_OPT, lStringT, "-wd", SGE_HOME_DIRECTORY);
         lSetString (ep, SPA_argval_lStringT, SGE_HOME_DIRECTORY);
         DPRINTF(("\"%s\" => -wd\n", *sp));
#endif         
         ep_opt = sge_add_noarg(pcmdline, cwd_OPT, *sp, NULL);
         DPRINTF(("\"%s\"\n", *sp));
         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-C directive_prefix" */

      if (!strcmp("-C", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str, MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S
               ,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is directive_prefix */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, "-C");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-C %s\"\n", *sp));

         ep_opt = sge_add_arg(pcmdline, C_OPT, lStringT, *(sp - 1), *sp);

         if (strlen(*sp) > 0) {
            lSetString(ep_opt, SPA_argval_lStringT, *sp);
         }

         sp++;
         continue;
      }


/*----------------------------------------------------------------------------*/
      /* "-dc simple_context_list */
      if (!strcmp("-dc", *sp)) {
         lList *variable_list = NULL;
         lListElem* lep = NULL;

         /* next field is simple_context_list */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_DCOPTIONMUSTHAVESIMPLECONTEXTLISTARGUMENT);
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-dc %s\"\n", *sp));
         i_ret = var_list_parse_from_string(&variable_list, *sp, 0);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGCONTEXTLISTFORMATDC_S
             , *sp);
             answer_list_add(&answer, str, 
                             STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, dc_OPT, lListT, *(sp - 1), *sp);
         lep = lCreateElem(VA_Type);
         lSetString(lep, VA_variable, "-");
         lInsertElem(variable_list, NULL, lep);
         lSetList(ep_opt, SPA_argval_lListT, variable_list);

         sp++;
         continue;
      }


      
/*----------------------------------------------------------------------------*/
      /* "-display -- only for qsh " */

      if (!strcmp("-display", *sp)) {
         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str, MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S
               ,
               *sp);
            answer_list_add(&answer, str, 
                            STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is display to redirect interactive jobs to */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_DISPLAYOPTIONMUSTHAVEARGUMENT );
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-display %s\"\n", *sp));

         ep_opt = sge_add_arg(pcmdline, display_OPT, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;

      }

#ifdef METACOMPUTING
/*-----------------------------------------------------------------------------*/
      /* "-show-hosts -- only for qsh " */

      if (!strcmp("-show-hosts", *sp)) {

         DPRINTF(("\"-show-hosts\n"));

         /* dummy opt for qsh -- Metacomputing */
         ep_opt = sge_add_noarg(pcmdline, 0, *sp, NULL);

         sp++;
         continue;

      }
#endif

/*----------------------------------------------------------------------------*/
      /* "-dl date_time */

      if (!strcmp("-dl", *sp)) {
         u_long32 timeval;

/* We do not have the userset_list here for the moment.
   So rely on the master deny if we are not a deadline user.

         if (!userset_is_deadline_user(userset_list, uti_state_get_user_name())) {
            sprintf(str, "%s you are not a deadline user\n", uti_state_get_user_name());
            answer_list_add(&answer, str, STATUS_DENIED, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
*/

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field(s) is "date_time" */
         sp++;
         if (!*sp) {
            sprintf(str,
               MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-dl");
            answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         if (!ulong_parse_date_time_from_string(&timeval, NULL, *sp))
         {
            sprintf(str, MSG_PARSE_WRONGTIMEFORMATXSPECTODLOPTION_S ,
            *sp);
            answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         ep_opt = sge_add_arg(pcmdline, dl_OPT, lUlongT, *(sp - 1), *sp);
         lSetUlong(ep_opt, SPA_argval_lUlongT, timeval);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-e path_name" */

      if (!strcmp("-e", *sp)) {
         lList *path_list = NULL;

         /* next field is path_name */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-e");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-e %s\"\n", *sp));

         i_ret = cull_parse_path_list(&path_list, *sp);
         if (i_ret) {
             sprintf(str,
             MSG_PARSE_WRONGPATHLISTFORMATXSPECTOEOPTION_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, e_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, path_list);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-i path_name" */

      if (!strcmp("-i", *sp)) {
         lList *path_list = NULL;

         /* next field is path_name */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-i");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-i %s\"\n", *sp));

         i_ret = cull_parse_path_list(&path_list, *sp);
         if (i_ret) {
             sprintf(str,
             MSG_PARSE_WRONGPATHLISTFORMATXSPECTOEOPTION_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, i_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, path_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-h [hold_list]" */

      if (!strcmp("-h", *sp)) {
            int hold;
            char *cmd_switch;
            char *cmd_arg = "";
         is_hold_option = true;
         cmd_switch = *sp;

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         if (is_qalter) {
            /* next field is hold_list */
            sp++;
            if (!*sp) {
                sprintf(str,
                MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-h");
                answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                DEXIT;
                return answer;
            }
            hold = sge_parse_hold_list(*sp);
            if (hold == -1) {
                sprintf(str,MSG_PARSE_UNKNOWNHOLDLISTXSPECTOHOPTION_S ,
                *sp);
                answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                DEXIT;
                return answer;
            }
            cmd_arg = *sp;
         }
         else {
            hold = MINUS_H_TGT_USER;
         }
         DPRINTF(("\"-h %s\"\n", *sp));

         ep_opt = sge_add_arg(pcmdline, h_OPT, lIntT, cmd_switch, cmd_arg);
         lSetInt(ep_opt, SPA_argval_lIntT, hold);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-hard" */

      if (!strcmp("-hard", *sp)) {

         DPRINTF(("\"%s\"\n", *sp));

         parse_hard_soft(1);
         ep_opt = sge_add_noarg(pcmdline, hard_OPT, *sp, NULL);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-help" */

      if (!strcmp("-help", *sp)) {
         DPRINTF(("\"%s\"\n", *sp));

         ep_opt = sge_add_noarg(pcmdline, help_OPT, *sp, NULL);

         sp++;
         continue;

      }

/*-----------------------------------------------------------------------------*/
      /* "-hold_jid jid[,jid,...]" */

      if (!strcmp("-hold_jid", *sp)) {
         lList *jid_hold_list = NULL;

         /* next field is hold_list */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-hold_jid");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-hold_jid %s\"\n", *sp));
         i_ret = cull_parse_jid_hold_list(&jid_hold_list, *sp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGJIDHOLDLISTFORMATXSPECTOHOLDJIDOPTION_S ,
             *sp);
             answer_list_add(&answer, str, 
                             STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, hold_jid_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, jid_hold_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-j y|n" */

      if (!strcmp("-j", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is "y|n" */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-j");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, 
                             ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-j %s\"\n", *sp));

         if (!strcmp("y", *sp)) {
            ep_opt = sge_add_arg(pcmdline, j_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, TRUE);
         }
         else if (!strcmp("n", *sp)) {
            ep_opt = sge_add_arg(pcmdline, j_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, FALSE);
         }
         else {
             sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTJX_S ,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-js jobshare */

      if (!strcmp("-js", *sp)) {
         u_long32 jobshare;

         sp++;
         if (!*sp) {
            sprintf(str,
            MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, "-js");
            answer_list_add(&answer, str, 
                            STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         if (!parse_ulong_val(NULL, &jobshare, TYPE_INT, *sp, NULL, 0)) {
            answer_list_add(&answer, MSG_PARSE_INVALIDJOBSHAREMUSTBEUINT,
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }

         ep_opt = sge_add_arg(pcmdline, js_OPT, lUlongT, *(sp - 1), *sp);
         lSetUlong(ep_opt, SPA_argval_lUlongT, jobshare);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-lj filename     log job processing */

      if (!strcmp("-lj", *sp)) {

         /* next field is filename */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-lj");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         /*
         ** problem: lj_OPT is not in list because someone ...
         ** ... see comment in parse.c
         */
         ep_opt = sge_add_arg(pcmdline, lj_OPT, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;
      }

/*---------------------------------------------------------------------------*/
      /* "-l resource_list" */

      if (!strcmp("-l", *sp)) {
         lList *resource_list = NULL;

         /* next field is resource_list */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-l");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-l %s\"\n", *sp));

         resource_list = centry_list_parse_from_string(NULL, *sp, false);
         if (!resource_list) {
             sprintf(str,MSG_PARSE_WRONGRESOURCELISTFORMATXSPECTOLOPTION_S ,
             *sp);
             answer_list_add(&answer, str, 
                             STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         ep_opt = sge_add_arg(pcmdline, l_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, resource_list);
         lSetInt(ep_opt, SPA_argval_lIntT, is_hard_soft());

         sp++;
         continue;
      }


/*----------------------------------------------------------------------------*/
      /* "-m mail_options */

      if (!strcmp("-m", *sp)) {
         int mail_options;

         /* next field is mail_options */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-m");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-m %s\"\n", *sp));
         mail_options = sge_parse_mail_options(*sp);
         if (!mail_options) {
             sprintf(str,
             MSG_PARSE_WRONGMAILOPTIONSLISTFORMATXSPECTOMOPTION_S ,
             *sp);
             answer_list_add(&answer, str, 
                             STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         ep_opt = sge_add_arg(pcmdline, m_OPT, lIntT, *(sp - 1), *sp);
         lSetInt(ep_opt, SPA_argval_lIntT, mail_options);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-masterq destination_identifier_list" */

      if (!strcmp("-masterq", *sp)) {
         lList *id_list = NULL;

         /* next field is destination_identifier */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_QOPTIONMUSTHAVEDESTIDLISTARGUMENT);
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-masterq %s\"\n", *sp));
         i_ret = cull_parse_destination_identifier_list(&id_list, *sp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGDESTIDLISTFORMATXSPECTOQOPTION_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, masterq_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, id_list);
         lSetInt(ep_opt, SPA_argval_lIntT, 0);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-M mail_list" */

      if (!strcmp("-M", *sp)) {
         lList *mail_list = NULL;

         /* next field is mail_list */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-M");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-M %s\"\n", *sp));

         i_ret = mailrec_parse(&mail_list, *sp);
          if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGMAILLISTFORMATXSPECTOMOPTION_S ,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, M_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, mail_list);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-N name" */

      if (!strcmp("-N", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is name */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-N");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         if (strchr(*sp, '/')) {
             sprintf(str,MSG_PARSE_ARGUMENTTONOPTIONMUSTNOTCONTAINBSL);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         if (*sp == '\0') {
             sprintf(str,MSG_PARSE_EMPTYSTRINGARGUMENTTONOPTIONINVALID);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-N %s\"\n", *sp));
         ep_opt = sge_add_arg(pcmdline, N_OPT, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-notify" */

      if (!strcmp("-notify", *sp)) {

         DPRINTF(("\"-notify\"\n"));

         ep_opt = sge_add_noarg(pcmdline, notify_OPT, *sp, NULL);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
     /*  -now y[es]|n[o] */

      if(!strcmp("-now", *sp)) {
         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }
         /* next field is yes/no switch */
         sp++;
         if(!*sp) {
            sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-now");
            answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         
         DPRINTF(("\"-now %s\"\n", *sp));
         
         if(!strcmp(*sp, "y") || !strcmp(*sp, "yes")){
            ep_opt = sge_add_arg(pcmdline, now_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, TRUE);
         }
         else if (!strcmp(*sp, "n") || !strcmp(*sp, "no")) {
            ep_opt = sge_add_arg(pcmdline, now_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, FALSE);
         }
         else {
             sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTNOW_S , *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;
      }
            
/*----------------------------------------------------------------------------*/
      /* "-o path_name" */

      if (!strcmp("-o", *sp)) {
         lList *stdout_path_list = NULL;

         /* next field is path_name */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-o");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-o %s\"\n", *sp));
         i_ret = cull_parse_path_list(&stdout_path_list, *sp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGSTDOUTPATHLISTFORMATXSPECTOOOPTION_S ,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, o_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, stdout_path_list);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-ot override tickets */

      if (!strcmp("-ot", *sp)) {
         int otickets;

         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-ot");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         otickets = atol(*sp);

         ep_opt = sge_add_arg(pcmdline, ot_OPT, lIntT, *(sp - 1), *sp);
         lSetInt(ep_opt, SPA_argval_lIntT, otickets);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* "-p priority */

      if (!strcmp("-p", *sp)) {
         int priority;

         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-p");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         if (sge_parse_priority(&answer, &priority, *sp)) {
            DEXIT;
            return answer;
         }

         ep_opt = sge_add_arg(pcmdline, p_OPT, lIntT, *(sp - 1), *sp);
         lSetInt(ep_opt, SPA_argval_lIntT, priority);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/

      /* "-P name" */

      if (!strcmp("-P", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str, MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is the projects name */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-P");
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-P %s\"\n", *sp));
         ep_opt = sge_add_arg(pcmdline, P_OPT, lStringT, *(sp - 1), *sp);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      if (!strcmp("-pe", *sp)) {
         lList *pe_range = NULL;

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }
DTRACE;
         /* next field must be the pe_name ... */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_PEOPTIONMUSTHAVEPENAMEARGUMENT);
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         /* ... followed by a range */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_PEOPTIONMUSTHAVERANGEAS2NDARGUMENT);
             answer_list_add(&answer, str, 
                             STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         range_list_parse_from_string(&pe_range, &answer, *sp, 
                                      0, 0, INF_ALLOWED);

         if (!pe_range) {
            DEXIT;
            return answer;
         }

         strcpy(arg, *(sp - 1));
         strcat(arg, " ");
         strcat(arg, *sp);
         ep_opt = sge_add_arg(pcmdline, pe_OPT, lStringT, *(sp - 2), arg);
         lSetString(ep_opt, SPA_argval_lStringT, *(sp - 1));
         lSetList(ep_opt, SPA_argval_lListT, pe_range);

         sp++;


         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-q destination_identifier_list" */

      if (!strcmp("-q", *sp)) {
         lList *id_list = NULL;

         /* next field is destination_identifier */
         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_QOPTIONMUSTHAVEDESTIDLISTARGUMENT);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, 0);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-q %s\"\n", *sp));
         i_ret = cull_parse_destination_identifier_list(&id_list, *sp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGDESTIDLISTFORMATXSPECTOQOPTION_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, q_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, id_list);
         lSetInt(ep_opt, SPA_argval_lIntT, is_hard_soft());

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-r y|n" */

      if (!strcmp("-r", *sp)) {


         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next filed is "y|n" */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-r");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-r %s\"\n", *sp));

         if (!strcmp("y", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, 1);
         }
         else if (!strcmp("n", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, 2);
         }
         else {
             sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTRX_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;

      }

/*-----------------------------------------------------------------------------*/
      /* "-R y|n" */

      if (!strcmp("-R", *sp)) {

         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next field is "y|n" */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-R");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, 
                             ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-R %s\"\n", *sp));

         if (!strcmp("y", *sp)) {
            ep_opt = sge_add_arg(pcmdline, R_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, TRUE);
         }
         else if (!strcmp("n", *sp)) {
            ep_opt = sge_add_arg(pcmdline, R_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, FALSE);
         }
         else {
             sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTJX_S ,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-sc variable_list" */
      /* set context */

      if (!strcmp("-sc", *sp)) {
         lList *variable_list = NULL;
         lListElem* lep = NULL;

         /* next field is context_list  [ == variable_list ] */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_SCOPTIONMUSTHAVECONTEXTLISTARGUMENT);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-sc %s\"\n", *sp));
         i_ret = var_list_parse_from_string(&variable_list, *sp, 0);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGCONTEXTLISTFORMATSCX_S, *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, sc_OPT, lListT, *(sp - 1), *sp);
         lep = lCreateElem(VA_Type);
         lSetString(lep, VA_variable, "=");
         lInsertElem(variable_list, NULL, lep);
         lSetList(ep_opt, SPA_argval_lListT, variable_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-soft" */

      if (!strcmp("-soft", *sp)) {

         DPRINTF(("\"%s\"\n", *sp));
         parse_hard_soft(2);
         ep_opt = sge_add_noarg(pcmdline, soft_OPT, *sp, NULL);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
     /*  -sync y[es]|n[o] */

      if(!strcmp("-sync", *sp)) {
         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }
         /* next field is yes/no switch */
         sp++;
         if(!*sp) {
            sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-sync");
            answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         
         DPRINTF(("\"-sync %s\"\n", *sp));
         
         if(!strcmp(*sp, "y") || !strcmp(*sp, "yes")){
            ep_opt = sge_add_arg(pcmdline, sync_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, TRUE);
         }
         else if (!strcmp(*sp, "n") || !strcmp(*sp, "no")) {
            ep_opt = sge_add_arg(pcmdline, sync_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, FALSE);
         }
         else {
             sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTNOW_S , *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;
      }
            
/*----------------------------------------------------------------------------*/
      /* "-S path_name" */

      if (!strcmp("-S", *sp)) {
         lList *shell_list = NULL;

         /* next field is path_name */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_SOPTIONMUSTHAVEPATHNAMEARGUMENT);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-S %s\"\n", *sp));

         i_ret = cull_parse_path_list(&shell_list, *sp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGSHELLLISTFORMATXSPECTOSOPTION_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, S_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, shell_list);

         sp++;
         continue;
      }

/*----------------------------------------------------------------------------*/
      /* -t <TaskIdRange>
       */

      if (!strcmp("-t", *sp)) {
         lList *task_id_range_list = NULL;

         /* next field is path_name */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_TOPTIONMUSTHAVEALISTOFTASKIDRANGES);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-t %s\"\n", *sp));

         range_list_parse_from_string(&task_id_range_list, &answer, *sp,
                                      0, 1, INF_NOT_ALLOWED);
         if (!task_id_range_list) {
            DEXIT;
            return answer;
         }

         range_list_sort_uniq_compress(task_id_range_list, &answer);
         if (lGetNumberOfElem(task_id_range_list) > 1) {
            answer_list_add(&answer, MSG_QCONF_ONLYONERANGE, STATUS_ESYNTAX, 0);
            DEXIT;
            return answer;
         }


         ep_opt = sge_add_arg(pcmdline, t_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, task_id_range_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-u" */
      if (!strcmp("-u", *sp)) {
         lList *user_list = NULL;

         /* next field is user_list */
         sp++;
         if (!*sp) {
            sprintf(str, MSG_PARSE_UOPTMUSTHAVEALISTUSERNAMES);
            answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
 
         DPRINTF(("\"-u %s\"\n", *sp));

         str_list_parse_from_string(&user_list, *sp, ",");

         ep_opt = sge_add_arg(pcmdline, u_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, user_list);  
         sp++;

         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-uall " */
      if (!strcmp("-uall", *sp)) {
         ep_opt = sge_add_noarg(pcmdline, u_OPT, *sp, NULL);

         DPRINTF(("\"-uall \"\n"));
         sp++;
  
         continue;
      }           

/*-----------------------------------------------------------------------------*/
      /* "-v variable_list" */

      if (!strcmp("-v", *sp)) {
         lList *variable_list = NULL;

         /* next field is variable_list */
         sp++;
         if (!*sp) {
             sprintf(str,MSG_PARSE_VOPTMUSTHAVEVARIABLELISTARGUMENT);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-v %s\"\n", *sp));

         i_ret = var_list_parse_from_string(&variable_list, *sp, 1);
         if (i_ret) {
             sprintf(str,MSG_PARSE_WRONGVARIABLELISTFORMATVORENVIRONMENTVARIABLENOTSET_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, v_OPT, lListT, *(sp - 1), *sp);
         lSetList(ep_opt, SPA_argval_lListT, variable_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-verify" */

      if (!strcmp("-verify", *sp)) {

         DPRINTF(("\"%s\"\n", *sp));
         ep_opt = sge_add_noarg(pcmdline, verify_OPT, *sp, NULL);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-V" */

      if (!strcmp("-V", *sp)) {
         lList *env_list = NULL;

         DPRINTF(("\"%s\"\n", *sp));
         i_ret = var_list_parse_from_environment(&env_list, envp);
         if (i_ret) {
             sprintf(str,MSG_PARSE_COULDNOTPARSEENVIRIONMENT);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }
         ep_opt = sge_add_arg(pcmdline, V_OPT, lListT, *sp, "");
         lSetList(ep_opt, SPA_argval_lListT, env_list);

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-w e|w|n|v" */

      if (!strcmp("-w", *sp)) {


         if (lGetElemStr(*pcmdline, SPA_switch, *sp)) {
            sprintf(str,
               MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S,
               *sp);
            answer_list_add(&answer, str, STATUS_EEXIST, ANSWER_QUALITY_WARNING);
         }

         /* next filed is "y|n" */
         sp++;
         if (!*sp) {
             sprintf(str,
             MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S,"-w");
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-w %s\"\n", *sp));

         if (!strcmp("e", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, ERROR_VERIFY);
         }
         else if (!strcmp("w", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, WARINING_VERIFY);
         }
         else if (!strcmp("n", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, SKIP_VERIFY);
         }
         else if (!strcmp("v", *sp)) {
            ep_opt = sge_add_arg(pcmdline, r_OPT, lIntT, *(sp - 1), *sp);
            lSetInt(ep_opt, SPA_argval_lIntT, JUST_VERIFY);
         }
         else {
             sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTWX_S,
             *sp);
             answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         sp++;
         continue;

      }
/*-----------------------------------------------------------------------------*/
      /* "-@" */
      /* reentrancy is built upon here */

      if (!strcmp("-@", *sp)) {
         lList *alp;
         lListElem *aep;
         int do_exit = 0;

         sp++;
         if (!*sp) {
             sprintf(str, MSG_PARSE_ATSIGNOPTIONMUSTHAVEFILEARGUMENT);
             answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
             DEXIT;
             return answer;
         }

         DPRINTF(("\"-@ %s\"\n", *sp));

         alp = parse_script_file(*sp, "", pcmdline, envp, FLG_USE_NO_PSEUDOS);
         for_each(aep, alp) {
            u_long32 quality;

            quality = lGetUlong(aep, AN_quality);
            if (quality == ANSWER_QUALITY_ERROR) {
               do_exit = 1;
            }
            lAppendElem(answer, lCopyElem(aep));
         }

         lFreeList(alp);
         if (do_exit) {
            DEXIT;
            return answer;
         }

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* "-verbose" - accept, but do nothing, must be handled by caller */

      if(!strcmp("-verbose", *sp)) {
         ep_opt = sge_add_noarg(pcmdline, verbose_OPT, *sp, NULL);
         sp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-inherit" - accept, but do nothing, must be handled by caller */

      if(!strcmp("-inherit", *sp)) {
         ep_opt = sge_add_noarg(pcmdline, verbose_OPT, *sp, NULL);
         sp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-nostdin" - accept, but do nothing, must be handled by caller */

      if(!strcmp("-nostdin", *sp)) {
         ep_opt = sge_add_noarg(pcmdline, nostdin_OPT, *sp, NULL);
         sp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-noshell" - accept, but do nothing, must be handled by caller */

      if(!strcmp("-noshell", *sp)) {
         ep_opt = sge_add_noarg(pcmdline, noshell_OPT, *sp, NULL);
         sp++;
         continue;
      }
/*-----------------------------------------------------------------------------*/
      /* "-huh?" */

      if (!strncmp("-", *sp, 1) && strcmp("--", *sp)) {
         sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
         answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return answer;
      }

/*-----------------------------------------------------------------------------*/
      /* - for read script from stdin */

      if (!strcmp("-", *sp)) {
         DPRINTF(("\"%s\"\n", *sp));

         if ((flags & FLG_USE_PSEUDOS)) {
            if (!is_qalter) {
               ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_SCRIPT, NULL);
               lSetString(ep_opt, SPA_argval_lStringT, *sp);
               for (sp++; *sp; sp++) {
                  ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_JOBARG, NULL);
                  lSetString(ep_opt, SPA_argval_lStringT, *sp);
               }
            }
            else {
               sprintf(str,MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
               answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
               DEXIT;
               return answer;
            }
            continue;
         }
         else {
            ep_opt = sge_add_noarg(pcmdline, 0, *sp, NULL);
         }

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/
      /* -- separator for qalter between job ids and job args */

      if (!strcmp("--", *sp)) {
         DPRINTF(("\"%s\"\n", *sp));

         if (is_qalter) {
            sprintf(str,MSG_PARSE_NOJOBIDGIVENBEFORESEPARATOR);
            answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            DEXIT;
            return answer;
         }
         else if ((flags & FLG_USE_PSEUDOS)) {
            sp++;
            if (!*sp) {
               sprintf(str,MSG_PARSE_OPTIONMUSTBEFOLLOWEDBYJOBARGUMENTS);
               answer_list_add(&answer, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
               DEXIT;
               return answer;
            }
            if  (!*sp) {
               continue;
            }
            for (; *sp; sp++) {
               ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_JOBARG, NULL);
               lSetString(ep_opt, SPA_argval_lStringT, *sp);
            }
            continue;
         }
         else {
            ep_opt = sge_add_noarg(pcmdline, 0, *sp, NULL);
         }

         sp++;
         continue;
      }

/*-----------------------------------------------------------------------------*/

      DPRINTF(("===%s===\n", *sp));

      /*
      ** with qsub and qsh this can only
      ** be a script file, remember this
      ** in case of qalter we need jobids
      */
      if ((flags & FLG_USE_PSEUDOS)) {
         if (!is_qalter) {
            ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_SCRIPT, NULL);
            lSetString(ep_opt, SPA_argval_lStringT, *sp);
            for (sp++; *sp; sp++) {
               ep_opt = sge_add_arg(pcmdline, 0, lStringT, STR_PSEUDO_JOBARG, NULL);
               lSetString(ep_opt, SPA_argval_lStringT, *sp);
            }
         }
         else {
            lList *jid_list = NULL;

            if (!strcmp(*sp, "--")) {
               ep_opt = sge_add_noarg(pcmdline, 0, *sp, NULL);
               break;
            }
            i_ret = cull_parse_jid_hold_list(&jid_list, *sp);

            if (i_ret) {
               sprintf(str,MSG_PARSE_WRONGJOBIDLISTFORMATXSPECIFIED_S,
                       *sp);
               answer_list_add(&answer, str, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               DEXIT;
               return answer;
            }
            ep_opt = sge_add_arg(pcmdline, 0, lListT, STR_PSEUDO_JOBID, *sp);
            lSetList(ep_opt, SPA_argval_lListT, jid_list);
            sp++;
         }
         continue;
      }
      else {
         ep_opt = sge_add_arg(pcmdline, 0, lStringT, "", NULL);
         lSetString(ep_opt, SPA_argval_lStringT, *sp);
      }

      sp++;
   }

   if (!is_hold_option) {
      if (uti_state_get_mewho() == QHOLD) { 
         ep_opt = sge_add_arg(pcmdline, h_OPT, lIntT, "-h", "u");
         lSetInt(ep_opt, SPA_argval_lIntT, MINUS_H_TGT_USER);

      }
      else if (uti_state_get_mewho() == QRLS) {
         ep_opt = sge_add_arg(pcmdline, h_OPT, lIntT, "-h", "n");
         lSetInt(ep_opt, SPA_argval_lIntT, MINUS_H_TGT_NONE);
      }
   }
   DEXIT;
   return answer;

}


/***********************************************************************/
/* "-h [hold_list]" */
static int sge_parse_hold_list(
char *hold_str 
) {
   int i, j;
   int target = 0;
   int op_code = 0;

   DENTER(TOP_LAYER, "sge_parse_hold_list");

   i = strlen(hold_str);

   for (j = 0; j < i; j++) {
      switch (hold_str[j]) {
      case 'n':
         if ((uti_state_get_mewho() == QHOLD)  || 
             (uti_state_get_mewho() == QRLS) || 
             (op_code && op_code != MINUS_H_CMD_SUB)) {
            target = -1;
            break;
         }
         op_code = MINUS_H_CMD_SUB;
         target = MINUS_H_TGT_USER|MINUS_H_TGT_OPERATOR|MINUS_H_TGT_SYSTEM;
         break;
      case 's':
         if (uti_state_get_mewho() == QRLS) {
            if (op_code && op_code != MINUS_H_CMD_SUB) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_SUB;
            target = target|MINUS_H_TGT_SYSTEM;         
         }
         else {
            if (op_code && op_code != MINUS_H_CMD_ADD) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_ADD;
            target = target|MINUS_H_TGT_SYSTEM;
         }   
         break;
      case 'o':
         if (uti_state_get_mewho() == QRLS) {
            if (op_code && op_code != MINUS_H_CMD_SUB) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_SUB;
            target = target|MINUS_H_TGT_OPERATOR;         
         }
         else {
            if (op_code && op_code != MINUS_H_CMD_ADD) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_ADD;
            target = target|MINUS_H_TGT_OPERATOR;
         }
         break;
         
      case 'u':
         if (uti_state_get_mewho() == QRLS) {
            if (op_code && op_code != MINUS_H_CMD_SUB) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_SUB;
            target = target|MINUS_H_TGT_USER;
         }
         else {
            if (op_code && op_code != MINUS_H_CMD_ADD) {
               target = -1;
               break;
            }
            op_code = MINUS_H_CMD_ADD;
            target = target|MINUS_H_TGT_USER;
         }
         break;
      case 'S':
         if ((uti_state_get_mewho() == QHOLD)  || 
             (uti_state_get_mewho() == QRLS) || 
             (op_code && op_code != MINUS_H_CMD_SUB)) {
            target = -1;
            break;
         }
         op_code = MINUS_H_CMD_SUB;
         target = target|MINUS_H_TGT_SYSTEM;
         break;
      case 'U':
         if ((uti_state_get_mewho() == QHOLD)  || 
             (uti_state_get_mewho() == QRLS) || 
             (op_code && op_code != MINUS_H_CMD_SUB)) {
            target = -1;
            break;
         }
         op_code = MINUS_H_CMD_SUB;
         target = target|MINUS_H_TGT_USER;
         break;
      case 'O':
         if ((uti_state_get_mewho() == QHOLD)  || 
             (uti_state_get_mewho() == QRLS) || 
             (op_code && op_code != MINUS_H_CMD_SUB)) {
            target = -1;
            break;
         }
         op_code = MINUS_H_CMD_SUB;
         target = target|MINUS_H_TGT_OPERATOR;
         break;
      default:
         target = -1;
      }

      if (target == -1)
         break;
   }

   if (target != -1)
      target |= op_code;

   DEXIT;
   return target;
}

/***********************************************************************/
static int sge_parse_mail_options(
char *mail_str 

) {

   int i, j;
   int mail_opt = 0;

   DENTER(TOP_LAYER, "sge_parse_mail_options");

   i = strlen(mail_str);

   for (j = 0; j < i; j++) {
      if ((char) mail_str[j] == 'a')
         mail_opt = mail_opt | MAIL_AT_ABORT;
      else if ((char) mail_str[j] == 'b')
         mail_opt = mail_opt | MAIL_AT_BEGINNING;
      else if ((char) mail_str[j] == 'e')
         mail_opt = mail_opt | MAIL_AT_EXIT;
      else if ((char) mail_str[j] == 'n')
         mail_opt = mail_opt | NO_MAIL;
      else if ((char) mail_str[j] == 's')
         mail_opt = mail_opt | MAIL_AT_SUSPENSION;
      else {
         return 0;
      }
   }

   DEXIT;
   return (mail_opt);

}

/***************************************************************************/
static int sge_parse_checkpoint_interval(
char *time_str 
) {
   u_long32 seconds;

   DENTER(TOP_LAYER, "sge_parse_checkpoint_interval");

   DPRINTF(("--------time_string: %s\n", time_str));
   if (!parse_ulong_val(NULL, &seconds, TYPE_TIM, time_str, NULL, 0))
      seconds = 0;

     DPRINTF(("-------- seconds: %d\n", (int) seconds));
   DEXIT;
   return seconds;
}

/***************************************************************************/


/****** parse_qsub/cull_parse_path_list() **************************************
*  NAME
*     cull_parse_path_list() -- ??? 
*
*  SYNOPSIS
*     int cull_parse_path_list(lList **lpp, char *path_str) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **lpp    - ??? 
*     char *path_str - ??? 
*
*  RESULT
*     int - error code 
*        0 = okay
*        1 = error 
*
*******************************************************************************/
int cull_parse_path_list(
lList **lpp,
char *path_str 

/*
   [[host]:]path[,[[host]:]path...]

   str0 - path
   str1 - host
   str3 - temporary
   int  - temporary
 */

) {
   char *path = NULL;
   char *cell = NULL;
   char **str_str = NULL;
   char **pstr = NULL;
   lListElem *ep = NULL;
   char *path_string = NULL;
   bool ret_error=false;

   DENTER(TOP_LAYER, "cull_parse_path_list");

   ret_error = !lpp;
/*
   if (!lpp) {
      DEXIT;
      return 1;
   }
*/

   if(!ret_error){
      path_string = sge_strdup(NULL, path_str);
      ret_error = !path_string;
   }
/*
   if (!path_string) {
      *lpp = NULL;
      DEXIT;
      return 2;
   }
*/
   if(!ret_error){
      str_str = string_list(path_string, ",", NULL);
      ret_error = !str_str || !*str_str;
   }
/*
   if (!str_str || !*str_str) {
      *lpp = NULL;
      FREE(path_string);
      DEXIT;
      return 3;
   }
*/
   if ( (!ret_error) && (!*lpp)) {
      *lpp = lCreateList("path list", PN_Type);
      ret_error = !*lpp;
/*
      if (!*lpp) {
         FREE(path_string);
         FREE(str_str);
         DEXIT;
         return 4;
      }
*/
   }

   if(!ret_error){
      for (pstr = str_str; *pstr; pstr++) {
      /* cell given ? */
         if (*pstr[0] == ':') {  /* :path */
            cell = NULL;
            path = *pstr+1;
         } else if ((path = strstr(*pstr, ":"))){ /* host:path */
            path[0] = '\0';
            cell = strdup(*pstr);
            path[0] = ':';
            path += 1;
         } else { /* path */
            cell = NULL;
            path = *pstr;
         }

         SGE_ASSERT((path));
         ep = lCreateElem(PN_Type);
         /* SGE_ASSERT(ep); */
         lAppendElem(*lpp, ep);

         lSetString(ep, PN_path, path);
        if (cell) {
            lSetHost(ep, PN_host, cell);
            FREE(cell);
         }
      }
   }
   if(path_string)
      FREE(path_string);
   if(str_str)
      FREE(str_str);
   DEXIT;
   if(ret_error)
      return 1;
   else
      return 0;
}


/***************************************************************************/
static int sge_parse_priority(
lList **alpp,
int *valp,
char *priority_str 
) {
   char *s;

   DENTER(TOP_LAYER, "sge_parse_priority");

   *valp = strtol(priority_str, &s, 10);
   if (priority_str==s || *valp > 1024 || *valp < -1023) {
       SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDPRIORITYMUSTBEINNEG1023TO1024));
       answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
       DEXIT;
       return -1;
   }
   DEXIT;
   return 0;
}

/****** client/var/var_list_parse_from_environment() **************************
*  NAME
*     var_list_parse_from_environment() -- create var list from env 
*
*  SYNOPSIS
*     static int var_list_parse_from_environment(lList **lpp, char **envp) 
*
*  FUNCTION
*     Create a list of variables from the given environment. 
*
*  INPUTS
*     lList **lpp - VA_Type list 
*     char **envp - environment pointer 
*
*  RESULT
*     static int - error state
*         0 - OK
*        >0 - Error   
*******************************************************************************/
static int var_list_parse_from_environment(lList **lpp, char **envp) 
{
   DENTER(TOP_LAYER, "var_list_parse_from_environment");

   if (!lpp || !envp) {
      DEXIT;
      return 1;
   }

   if (!*lpp) {
      *lpp = lCreateList("env list", VA_Type);
      if (!*lpp) {
         DEXIT;
         return 3;
      }
   }

   for (; *envp; envp++) {
      char *env_name;
      char *env_description;
      char *env_entry;
      lListElem *ep;

      ep = lCreateElem(VA_Type);
      lAppendElem(*lpp, ep);

      env_entry = sge_strdup(NULL, *envp);
      SGE_ASSERT((env_entry));

      env_name = sge_strtok(env_entry, "=");
      SGE_ASSERT((env_name));
      lSetString(ep, VA_variable, env_name);

      env_description = sge_strtok((char *) 0, "\n");
      if (env_description)
         lSetString(ep, VA_value, env_description);
      FREE(env_entry);
   }

   DEXIT;
   return 0;
}

/***************************************************************************/
static int cull_parse_destination_identifier_list(
lList **lpp, /* QR_Type */
char *dest_str 
) {
   int rule[] = {QR_name, 0};
   char **str_str = NULL;
   int i_ret;
   char *s;

   DENTER(TOP_LAYER, "cull_parse_destination_identifier_list");

   if (!lpp) {
      DEXIT;
      return 1;
   }

   s = sge_strdup(NULL, dest_str);
   if (!s) {
      *lpp = NULL;
      DEXIT;
      return 3;
   }
   str_str = string_list(s, ",", NULL);
   if (!str_str || !*str_str) {
      *lpp = NULL;
      FREE(s);
      DEXIT;
      return 2;
   }

   i_ret = cull_parse_string_list(str_str, "destin ident list", QR_Type, rule, lpp);
   if (i_ret) {
      FREE(s);
      FREE(str_str);
      DEXIT;
      return 3;
   }

   FREE(s);
   FREE(str_str);
   DEXIT;
   return 0;
}

/***************************************************************************/
int cull_parse_jid_hold_list(
lList **lpp,
char *str 

/*   jid[,jid,...]

   int0 - jid

 */

) {
   int rule[] = {ST_name, 0};
   char **str_str = NULL;
   int i_ret;
   char *s;

   DENTER(TOP_LAYER, "cull_parse_jid_hold_list");

   if (!lpp) {
      DEXIT;
      return 1;
   }

   s = sge_strdup(NULL, str);
   if (!s) {
      *lpp = NULL;
      DEXIT;
      return 3;
   }
   str_str = string_list(s, ",", NULL);
   if (!str_str || !*str_str) {
      *lpp = NULL;
      FREE(s);
      DEXIT;
      return 2;
   }
   i_ret = cull_parse_string_list(str_str, "jid_hold list", ST_Type, rule, lpp);
   
   if (i_ret) {
      FREE(s);
      FREE(str_str);
      DEXIT;
      return 3;
   }

   FREE(s);
   FREE(str_str);
   DEXIT;
   return 0;
}

/****** client/var/var_list_parse_from_string() *******************************
*  NAME
*     var_list_parse_from_string() -- parse vars from string list 
*
*  SYNOPSIS
*     int var_list_parse_from_string(lList **lpp, 
*                                    const char *variable_str, 
*                                    int check_environment) 
*
*  FUNCTION
*     Parse a list of variables ("lpp") from a comma separated 
*     string list ("variable_str"). The boolean "check_environment"
*     defined wether the current value of a variable is taken from
*     the environment of the calling process.
*
*  INPUTS
*     lList **lpp              - VA_Type list 
*     const char *variable_str - source string 
*     int check_environment    - boolean
*
*  RESULT
*     int - error state
*         0 - OK
*        >0 - Error
*******************************************************************************/
int var_list_parse_from_string(lList **lpp, const char *variable_str,
                               int check_environment)
{
   char *variable;
   char *value;
   stringT str;
   char **str_str;
   char **pstr;
   lListElem *ep;
   char *va_string;

   DENTER(TOP_LAYER, "var_list_parse_from_string");

   if (!lpp) {
      DEXIT;
      return 1;
   }

   va_string = sge_strdup(NULL, variable_str);
   if (!va_string) {
      *lpp = NULL;
      DEXIT;
      return 2;
   }
   str_str = string_list(va_string, ",", NULL);
   if (!str_str || !*str_str) {
      *lpp = NULL;
      FREE(va_string);
      DEXIT;
      return 3;
   }

   if (!*lpp) {
      *lpp = lCreateList("variable list", VA_Type);
      if (!*lpp) {
         FREE(va_string);
         FREE(str_str);
         DEXIT;
         return 4;
      }
   }

   for (pstr = str_str; *pstr; pstr++) {
      ep = lCreateElem(VA_Type);
      /* SGE_ASSERT(ep); */
      lAppendElem(*lpp, ep);

      variable = sge_strtok(*pstr, "=");
      SGE_ASSERT((variable));
      memset(str, 0, sizeof(str));
      sprintf(str, "%s=", variable);
      lSetString(ep, VA_variable, variable);
      value = sge_strtok((char *) NULL, "=");
      if (value)
         lSetString(ep, VA_value, value);
      else if(check_environment)
         lSetString(ep, VA_value, sge_getenv(variable));
      else
         lSetString(ep, VA_value, NULL);
   }

   FREE(va_string);
   FREE(str_str);
   DEXIT;
   return 0;
}

