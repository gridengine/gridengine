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

#include "def.h"
#include "sgermon.h"
#include "sge_exit.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_gdi_intern.h"
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_stringL.h"
#include "parse_qsubL.h"
#include "sge_answerL.h"
#include "sge_job_refL.h"
#include "usage.h"
#include "sge_parse_num_par.h"
#include "sge_resource.h"
#include "sge_parse_date_time.h"
#include "parse.h"
#include "sge_options.h"
#include "sge_me.h"
#include "sge_rangeL.h"
#include "sge_identL.h"
#include "parse_range.h"
#include "msg_common.h"
#include "msg_gdilib.h"


int hard = TRUE;

/***************************************************************************/
/* do the opposite of sge_parse_maillist
   result is a string like "user@host,..." or account@host                 */
/***************************************************************************/
int sge_unparse_ma_list(
lList *head,
char *mail_str,
unsigned int mail_str_len 
) {
   int len=0;
   int comma_needed = 0; /* whether we need to insert a comma */
   char tmpstr[1000];    /* need 1000 for brain damaged mail addresse(e)s */
   lListElem *elem;
   const char *h;
   const char *u;
   
   if (!head) {
      strcpy(mail_str, MSG_NONE);
      return 0;
   }

   *mail_str = '\0';

   for_each(elem,head) {
      if (!(u = lGetString(elem, MR_user)))
         u = MSG_SMALLNULL;

      if (!(h = lGetString(elem, MR_host)))
         sprintf(tmpstr, "%s", u);
      else
         sprintf(tmpstr, "%s@%s", u, h);

      if (strlen(tmpstr)+len+1+comma_needed > mail_str_len)
         return 1;              /* forgot the rest */

      if (comma_needed)
         strcat(mail_str, ",");
      else
         comma_needed = 1;      /* need comma after first mailaddress */

      strcat(mail_str, tmpstr);
   }
   return 0;
}

/*-------------------------------------------------------------------------*/
void sge_parse_string_list(lList **lp, const char *str, int field, 
                           lDescr *descr) {
   const char *cp;

   DENTER(TOP_LAYER, "sge_parse_string_list");

   cp = sge_strtok(str, ",");
   lAddElemStr(lp, field, cp, descr);
   while((cp = sge_strtok(NULL, ","))) {
      lAddElemStr(lp, field, cp, descr);
   }

   DEXIT;
}

/*
 * return   -1 no valid JobTask-Identifier
 *          1  ok
 */
int sge_parse_jobtasks(
lList **ipp,          /* ID_Type List */
lListElem **idp,     /* New ID_Type-Elem parsed from str_jobtask */
const char *str_jobtask,   
lList **alpp 
) {
   char *token;
   char *job_str, *str;
   lList *task_id_range_list = NULL;
   lListElem *range;

/*
   Digit = '0' | '1' | ... | '9' .
   JobId = Digit { Digit } .
   TaskIdRange = TaskId [ '-' TaskId [  ':' Digit ] ] .
   JobTasks = JobId [ '.' TaskIdRange ] .
*/

   DENTER(TOP_LAYER, "sge_parse_jobtasks");

   /*
   ** dup the input string for tokenizing
   */
   str = strdup(str_jobtask);
   if (str) {
      token = strtok(str, " ."); 
   }
   job_str = str;

   if ((token = strtok(NULL, ""))) {
      task_id_range_list = parse_ranges(token, 0, 1, alpp, NULL, 
                                          INF_NOT_ALLOWED);
      if (*alpp) {
         /*
         ** free the dupped string
         */
         free(str);
         DEXIT;
         return -1;
      }
      if (!task_id_range_list) {
         range = lAddElemUlong(&task_id_range_list, RN_min, 1, RN_Type);
         lSetUlong(range, RN_max, 1);
         lSetUlong(range, RN_step, 1);
      }
   }

   if (!atol(job_str) && strcmp(job_str, "all")) {
      /*
      ** free the dupped string
      */
      free(str);
      DEXIT;
      return -1;
   }
      
   *idp = lAddElemStr(ipp, ID_str, job_str, ID_Type);
   if (*idp)
      lSetList(*idp, ID_ja_structure, task_id_range_list);
  
   /*
   ** free the dupped string
   */
   free(str); 
   DEXIT;
   return 1;
}

/***************************************************************************/

lListElem *sge_add_noarg(
lList **popt_list,
u_long32 opt_number,
const char *opt_switch,
const char *opt_switch_arg 
) {
   lListElem *ep;

   if (!popt_list) {
      return NULL;
   }
   if (!*popt_list) {
      *popt_list = lCreateList("option list", SPA_Type);
      if (!*popt_list) {
         return NULL;
      }
   }

   ep = lCreateElem(SPA_Type);
   if (!ep) {
      return NULL;
   }
   lSetUlong(ep, SPA_number, opt_number);
   lSetString(ep, SPA_switch, opt_switch);
   lSetString(ep, SPA_switch_arg, opt_switch_arg);
   lSetUlong(ep, SPA_occurrence, BIT_SPA_OCC_NOARG);
   lAppendElem(*popt_list, ep);
   return ep;
}

/***************************************************************************/

lListElem *sge_add_arg(
lList **popt_list,
u_long32 opt_number,
u_long32 opt_type,
const char *opt_switch,
const char *opt_switch_arg 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_add_arg");

   if (!popt_list) {
      DEXIT;
      return NULL;
   }

   ep = lAddElemStr(popt_list, SPA_switch, opt_switch, SPA_Type);

   if (!ep) {
      DEXIT;
      return NULL;
   }
   lSetUlong(ep, SPA_number, opt_number);
   lSetUlong(ep, SPA_argtype, opt_type);
   lSetString(ep, SPA_switch_arg, opt_switch_arg);
   lSetUlong(ep, SPA_occurrence, BIT_SPA_OCC_ARG);

   DEXIT;
   return ep;
}

/***************************************************************************/


/****
 **** parse_noopt
 ****
 **** parse a option from the commandline (sp). The option
 **** is given by shortopt and longopt (optional). 
 **** The parsed option is stored in ppcmdline (SPA_Type).
 **** An errormessage is appended to the answer-list (alpp).
 **** The function returns a pointer to the next argument.
 ****/
char **parse_noopt(
char **sp,
const char *shortopt,
const char *longopt,
lList **ppcmdline,
lList **alpp 
) {

   DENTER (TOP_LAYER, "parse_noopt");

   if ( (!strcmp(shortopt, *sp)) || (longopt && !strcmp(longopt, *sp)) ) {
      if(!lGetElemStr(*ppcmdline, SPA_switch, shortopt)) {
         sge_add_noarg(ppcmdline, 0, shortopt, NULL);
      }
      sp++;
   }
   DEXIT;
   return sp;
}

/****
 **** parse_until_next_opt
 ****
 **** parse an option from the commandline (sp). The option 
 **** is given by shortopt and longopt (optional).
 **** Arguments are parsed until another option is reached
 **** (beginning with '-'). The parsed option is stored
 **** in ppcmdline (SPA_Type). An errormessage will be
 **** appended to the answer-list (alpp).
 **** The function returns a pointer to the next argument.
 **** If shortopt or longopt contains a '*' as its last (!)
 **** character, then all options matching the first given
 **** charcters are valid (e.g. 'OAport' matches 'OA*')
 ****/
char **parse_until_next_opt(
char **sp,
const char *shortopt,
const char *longopt,
lList **ppcmdline,
lList **alpp 
) {
char **rp;
stringT str;
lListElem *ep; /* SPA_Type */

   DENTER (TOP_LAYER, "parse_until_next_opt");

   rp = sp;
   if ( (!strcmp(shortopt, *sp)) || (longopt && !strcmp(longopt, *sp)) 
        || ((shortopt[strlen(shortopt)-1] == '*')
           && !strncmp(shortopt, *sp, strlen(shortopt)-1)) 
        || (longopt && (longopt[strlen(longopt)-1] == '*')
           && !strncmp(longopt, *sp, strlen(longopt)-1)) ) {
      if(!*(++rp) || (**rp == '-')) {
         sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, *sp);
         sge_add_answer(alpp, str, STATUS_ESEMANTIC, 0);
         DEXIT;
         return rp;
      }
      ep = sge_add_arg(ppcmdline, 0, lListT, shortopt, NULL);
      while (*rp && **rp != '-') {
         /* string at *rp is argument to current option */
         lAddSubStr(ep, STR, *rp, SPA_argval_lListT, ST_Type);
         rp++;
      }
   }
   DEXIT;
   return rp;
}


/****
 **** parse_until_next_opt2
 ****
 **** parse an option from the commandline (sp). The option 
 **** is given by shortopt and longopt (optional).
 **** Arguments are parsed until another option is reached
 **** (beginning with '-'). The parsed option is stored
 **** in ppcmdline (SPA_Type). An errormessage will be
 **** appended to the answer-list (alpp).
 **** The function returns a pointer to the next argument.
 ****/
char **parse_until_next_opt2(
char **sp,
const char *shortopt,
const char *longopt,
lList **ppcmdline,
lList **alpp 
) {
   char **rp;
   lListElem *ep; /* SPA_Type */

   DENTER (TOP_LAYER, "parse_until_next_opt2");

   rp = sp;
   if ( (!strcmp(shortopt, *sp)) || (longopt && !strcmp(longopt, *sp)) ) {
      ++rp;
      
#if 0
      string str;
      if(!*() || (**rp == '-')) {
         sprintf(str, MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S, *sp);
         sge_add_answer(alpp, str, STATUS_ESEMANTIC, 0);
         DEXIT;
         return rp;
      }
#endif
      ep = sge_add_arg(ppcmdline, 0, lListT, shortopt, NULL);
      while (*rp && **rp != '-') {
         /* string at *rp is argument to current option */
         lAddSubStr(ep, STR, *rp, SPA_argval_lListT, ST_Type);
         rp++;
      }
   }
   DEXIT;
   return rp;
}

/****
 **** parse_param
 ****
 **** parse a list of parameters from the commandline (sp).
 **** Parameters are parsed until the next option ("-...")
 **** is reached. The parsed parameters are stored in
 **** ppcmdline (SPA_Type).
 **** The function returns a pointer to the next argument.
 ****/
char **parse_param(
char **sp,
const char *opt,
lList **ppcmdline,
lList **alpp 
) {
char **rp;
lListElem *ep = NULL; /* SPA_Type */

   DENTER (TOP_LAYER, "parse_param");

   rp = sp;
   while( (*rp) && (**rp != '-') ) {
      /* string under rp is parameter, no option! */
      if(!ep)
         ep = sge_add_arg(ppcmdline, 0, lListT, opt, NULL);
      lAddElemStr(lGetListRef(ep, SPA_argval_lListT), STR, *rp, ST_Type);
      rp++;
   }
   DEXIT;
   return rp;
}

/****
 **** parse_flag
 ****
 **** look in the ppcmdline-list for a flag given
 **** by 'opt'. If it is found, TRUE is returned and
 **** flag is set to 1. If it is not found, FALSE
 **** is returned and flag will be untouched.
 ****
 **** If the switch occures more than one time, every
 **** occurence is removed from the ppcmdline-list.
 ****
 **** The answerlist ppal is not used yet.
 ****/
int parse_flag(
lList **ppcmdline,
const char *opt,
lList **ppal,
u_long32 *pflag 
) {
lListElem *ep;
char* actual_opt;

   DENTER(BASIS_LAYER, "parse_flag");

   if((ep = lGetElemStrLike(*ppcmdline, SPA_switch, opt))) {
      actual_opt = sge_strdup(NULL, lGetString(ep, SPA_switch));
      while(ep) {
         /* remove _all_ flags of same type */
         lRemoveElem(*ppcmdline, ep);
         ep = lGetElemStrLike(*ppcmdline, SPA_switch, actual_opt);
      }
      free(actual_opt);
      *pflag = 1;
      DEXIT;
      return TRUE;
   } else {
      DEXIT;
      return FALSE;
   }
}

/****
 **** parse_multi_stringlist
 ****
 **** looks in the ppcmdline-list for a option given
 **** by 'opt'. If it is found, TRUE is returned,
 **** otherwise FALSE.
 **** Arguments after the option switch are parsed
 **** into the ppdestlist-list (given field and type).
 **** There can be multiple occurences of this switch.
 **** The arguments are collected. 
 **** The arguments can be eiter comma-separated. 
 ****/ 
int parse_multi_stringlist(
lList **ppcmdline,
const char *opt,
lList **ppal,
lList **ppdestlist,
lDescr *type,
int field 
) {
   lListElem *ep, *sep;

   DENTER(TOP_LAYER, "parse_multi_stringlist");

   if((ep = lGetElemStr(*ppcmdline, SPA_switch, opt))) {
      while(ep) {
         /* collect all opts of same type, this is what 'multi' means in funcname!  */
         for_each(sep, lGetList(ep, SPA_argval_lListT)) {
            sge_parse_string_list(ppdestlist, lGetString(sep, STR), field, type);
         }
         lRemoveElem(*ppcmdline, ep);
         ep = lGetElemStr(*ppcmdline, SPA_switch, opt);
      }
      DEXIT;
      return TRUE;
   } else {
      DEXIT;
      return FALSE;
   }
}

int parse_multi_jobtaskslist(
lList **ppcmdline,
const char *opt,
lList **alpp,
lList **ppdestlist 
) {
   lListElem *ep, *sep, *idp;
   char str[256];
   int ret = FALSE;

   DENTER(TOP_LAYER, "parse_multi_jobtaskslist");
   while ((ep = lGetElemStr(*ppcmdline, SPA_switch, opt))) {
      ret = TRUE;
      for_each(sep, lGetList(ep, SPA_argval_lListT)) {
         lList *tmp_alp = NULL;
      
         if (sge_parse_jobtasks(ppdestlist, &idp, 
               lGetString(sep, STR), &tmp_alp) == -1) {
            sprintf(str,  MSG_JOB_XISINVALIDJOBTASKID_S, 
               lGetString(sep, STR));
            sge_add_answer(alpp, str, STATUS_ESEMANTIC, 0);

            lRemoveElem(*ppcmdline, ep);
            DEXIT;
            return FALSE;
         }
      }
      lRemoveElem(*ppcmdline, ep);
   }
   DEXIT;
   return ret;
}

int parse_string(
lList **ppcmdline,
const char *opt,
lList **ppal,
char **str 
) {
   lListElem *ep, *ep2;

   DENTER(TOP_LAYER, "parse_string");

   if((ep = lGetElemStr(*ppcmdline, SPA_switch, opt))) {
      ep2 = lFirst(lGetList(ep, SPA_argval_lListT));
      if (ep2)
         *str = sge_strdup(NULL, lGetString(ep2, STR));
      else
         *str = NULL;   
      lRemoveElem(*ppcmdline, ep);

      DEXIT;
      return TRUE;
   } else {
      DEXIT;
      return FALSE;
   }
}


u_long32 parse_group_options(
lList *string_list 
) {
   u_long32 group_opt = GROUP_TASK_GROUPS;
   lListElem *str_elem;

   DENTER(TOP_LAYER, "sge_parse_group_options");
   for_each(str_elem, string_list) {
      if ((char) lGetString(str_elem, STR)[0] == 'd')
         group_opt = GROUP_NO_TASK_GROUPS;
   }
   DEXIT; 
   return (group_opt);
}

