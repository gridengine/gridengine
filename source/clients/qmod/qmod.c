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

#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "gdi_qmod.h"
#include "sig_handlers.h"
#include "parse_qsub.h"
#include "parse.h"
#include "usage.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_feature.h"
#include "sge_language.h"
#include "sge_unistd.h"
#include "sge_answer.h"

#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qmod.h"

static lList *sge_parse_cmdline_qmod(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qmod(lList **ppcmdline, lList **ppreflist, u_long32 *pforce, u_long32 *paction, u_long32 *pverify);
static int qmod_usage(FILE *fp, char *what);

extern char **environ;

int main(int argc, char *argv[]);


int main(
int argc,
char **argv 
) {
   u_long32 action = 0, force = 0, verify = 0;
   lList *ref_list = NULL;
   lList *alp = NULL, *pcmdline = NULL;
   lListElem *aep, *rep;
   const char *actionstr;
   int cl_err = 0;
   
   DENTER_MAIN(TOP_LAYER, "qmod");

   sge_gdi_param(SET_MEWHO, QMOD, NULL);
   if ((cl_err = sge_gdi_setup(prognames[QMOD]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QMOD);

   /*
   ** static func for parsing all qmod specific switches
   ** here we get action, force, ref_list, verify
   */
   alp = sge_parse_cmdline_qmod(++argv, environ, &pcmdline);
   if(alp) {
      /*
      ** high level parsing error! show answer list
      */
      for_each(aep, alp) { 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }
   
   alp = sge_parse_qmod(&pcmdline, &ref_list, &force, &action, &verify);
   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) { 
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      lFreeList(ref_list);
      SGE_EXIT(1);
   }

   /*
   ** verify
   */
   if (verify) {
      if (action == QERROR) {
         actionstr = MSG_QMOD_CLEAR;
      } else if (action == QDISABLED) {
         actionstr = MSG_QMOD_DISABLE;
      } else if (action == QENABLED) {
         actionstr = MSG_QMOD_ENABLE;
      } else if (action == QSUSPENDED) {
         actionstr = MSG_QMOD_SUSPEND;
      } else if (action == QRUNNING) {
         actionstr = MSG_QMOD_UNSUSPEND;
      } else if (action == QRUNNING) {
         actionstr = MSG_QMOD_UNSUSPEND;
      } else if (action == QRESCHEDULED) {
         actionstr = MSG_QMOD_RESCHEDULE;
      } else {
         actionstr = MSG_QMOD_UNKNOWNACTION;
      }

      for_each(rep, ref_list) {
         printf(MSG_QMOD_XYOFJOBQUEUEZ_SSS,force?MSG_FORCED:"", actionstr, lGetString(rep, STR));
      }
      SGE_EXIT(0);
   }

   /*
   ** this is the interface to simulate the old behavior
   ** gdilib/gdi_qmod.c
   */
   if(ref_list)
      alp = gdi_qmod(ref_list, force, action);

   /*
   ** show answer list
   */
   for_each(aep, alp) { 
      fprintf(stdout, "%s", lGetString(aep, AN_text));
   }

   lFreeList(alp);
   lFreeList(ref_list);
   lFreeList(pcmdline); 
   SGE_EXIT(0);
   return 0;
}



/****
 **** sge_parse_cmdline_qmod (static)
 ****
 **** 'stage 1' parsing of qmod-options. Parses options
 **** with their arguments and stores them in ppcmdline.
 ****/ 
static lList *sge_parse_cmdline_qmod(
char **argv,
char **envp,
lList **ppcmdline 
) {
char **sp;
char **rp;
stringT str;
lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_parse_cmdline_qmod");
   
   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", "--help", ppcmdline, &alp)) != sp)
         continue;
        
      /* -f option */
      if ((rp = parse_noopt(sp, "-f", "--force", ppcmdline, &alp)) != sp)
         continue;
         
      /* -verify */
      if ((rp = parse_noopt(sp, "-verify", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -c option */
      if ((rp = parse_until_next_opt(sp, "-c", "--clear", ppcmdline, &alp)) != sp)
         continue;

      /* -s option */
      if ((rp = parse_until_next_opt(sp, "-s", "--suspend", ppcmdline, &alp)) != sp)
         continue;

      /* -us option */
      if ((rp = parse_until_next_opt(sp, "-us", "--unsuspend", ppcmdline, &alp)) != sp)
         continue;

      /* -d option */
      if ((rp = parse_until_next_opt(sp, "-d", "--disable", ppcmdline, &alp)) != sp)           
         continue;

      /* -r option */
      if ((rp = parse_until_next_opt(sp, "-r", "--reschedule", ppcmdline, &alp)) != sp)
         continue;

      /* -e option */
      if ((rp = parse_until_next_opt(sp, "-e", "--enable", ppcmdline, &alp)) != sp)
         continue;

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      qmod_usage(stderr, NULL);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;  
}

/****
 **** sge_parse_qmod (static)
 ****
 **** 'stage 2' parsing of qmod-options. Gets the options from 
 **** ppcmdline, sets the force and action flags and puts the
 **** queue/job-names/numbers in ppreflist.
 ****/
static lList *sge_parse_qmod(
lList **ppcmdline,
lList **ppreflist,
u_long32 *pforce,
u_long32 *paction,
u_long32 *pverify 
) {
stringT str;
lList *alp = NULL;
u_long32 helpflag;
int usageshowed = 0;

   DENTER(TOP_LAYER, "sge_parse_qmod");

   /* Loop over all options. Only valid options can be in the
      ppcmdline list. Except f_OPT all options are exclusive.
   */
   while(lGetNumberOfElem(*ppcmdline))
   {
      if(parse_flag(ppcmdline, "-help",  &alp, &helpflag)) {
         usageshowed = qmod_usage(stdout, NULL);
         break;
      }
      if(parse_flag(ppcmdline, "-f", &alp, pforce))
         continue;
      
      if(parse_flag(ppcmdline, "-verify", &alp, pverify))
         continue;

      if(parse_multi_stringlist(ppcmdline, "-c", &alp, ppreflist, ST_Type, STR)) {
         *paction = QERROR;
         break;
      }

      if(parse_multi_stringlist(ppcmdline, "-d", &alp, ppreflist, ST_Type, STR)) {
         *paction = QDISABLED;
         break;
      }

      if(parse_multi_stringlist(ppcmdline, "-r", &alp, ppreflist, ST_Type, STR)) {
         *paction = QRESCHEDULED;
         break;
      }

      if(parse_multi_stringlist(ppcmdline, "-e", &alp, ppreflist, ST_Type, STR)) {
         *paction = QENABLED;
         break;
      }

      if(parse_multi_stringlist(ppcmdline, "-s", &alp, ppreflist, ST_Type, STR)) {
         *paction = QSUSPENDED;
         break;
      }
      if(parse_multi_stringlist(ppcmdline, "-us", &alp, ppreflist, ST_Type, STR)) {
         *paction = QRUNNING;
         break;
      }
   }
   if(lGetNumberOfElem(*ppcmdline)) {
     sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
     if(!usageshowed)
        qmod_usage(stderr, NULL);
     answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
     DEXIT;
     return alp;
   }
   DEXIT;
   return alp;
}

/****
 **** qmod_usage (static)
 ****
 **** displays usage of qmod on file fp.
 **** Is what NULL, full usage will be displayed.
 ****
 **** Returns always 1.
 ****
 **** If what is a pointer to an option-string,
 **** only usage for that option will be displayed.
 ****   ** not implemented yet! **
 ****/
static int qmod_usage(
FILE *fp,
char *what 
) {
   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION));

   if(!what) {
      /* display full usage */
      fprintf(fp,"%s qmod [options]\n", MSG_SRC_USAGE); 
      fprintf(fp, "   [-c job_queue_list]  %s", MSG_QMOD_c_OPT_USAGE);
      fprintf(fp, "   [-d queue_list]      %s", MSG_QMOD_d_OPT_USAGE);
      fprintf(fp, "   [-e queue_list]      %s", MSG_QMOD_e_OPT_USAGE);
      fprintf(fp, "   [-f]                 %s", MSG_QMOD_f_OPT_USAGE);
      fprintf(fp, "   [-help]              %s", MSG_QMOD_help_OPT_USAGE);
      fprintf(fp, "   [-r job_queue_list]  %s", MSG_QMOD_r_OPT_USAGE);
      fprintf(fp, "   [-s job_queue_list]  %s", MSG_QMOD_s_OPT_USAGE);
      fprintf(fp, "   [-us job_queue_list] %s", MSG_QMOD_us_OPT_USAGE);
      fprintf(fp, "   [-verify]            %s", MSG_QMOD_verify_OPT_USAGE);
      fprintf(fp, "job_queue_list          {job_tasks|queue}[{,| }{job_tasks|queue}{,| }...]\n");
      fprintf(fp, "queue_list              {queue}[{,| }{queue}{,| }...]\n");
      fprintf(fp, "job_tasks               job_id['.'task_id_range]\n");
      fprintf(fp, "task_id_range           task_id['-'task_id[':'step]]\n");

   } else {
      /* display option usage */
      fprintf(fp, MSG_QDEL_not_available_OPT_USAGE_S,what);
   }
   return 1;
}
