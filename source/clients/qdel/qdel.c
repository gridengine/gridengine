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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <unistd.h>
#include "sgermon.h"
#include "sge_exit.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "usage.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "sge_prognames.h"
#include "sig_handlers.h"
#include "parse.h"

#include "msg_gdilib.h"
#include "gdi_checkpermissions.h"
#include "sge_feature.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qdel.h"

static lList *sge_parse_cmdline_qdel(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qdel(lList **ppcmdline, lList **ppreflist, u_long32 *pforce, u_long32 *pverify, u_long32 *all_jobs, u_long32 *all_users, lList **ppuserlist);
static int qdel_usage(FILE *fp, char *what);

extern char **environ;

int main(int argc, char *argv[]);

/************************************************************************/
int main(
int argc,
char **argv 
) {
   /* lListElem *rep, *nxt_rep, *jep, *aep, *jrep, *idep; */
   lListElem *aep, *idep;
   lList *jlp = NULL, *alp = NULL, *pcmdline = NULL, *ref_list = NULL, *user_list=NULL;
   u_long32 force = 0, verify = 0, all_users = 0, all_jobs = 0;
   int cmd;
   int wait;
   unsigned long status = 0;
   lListElem* ref_elem = NULL;
   const char* jobName = NULL;
   int have_master_privileges;
   int cl_err = 0;

   DENTER_MAIN(TOP_LAYER, "qdel");

   sge_gdi_param(SET_MEWHO, QDEL, NULL);
/*    sge_gdi_param(SET_ISALIVE, 1, NULL); */
   if ((cl_err = sge_gdi_setup(prognames[QDEL]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   alp = sge_parse_cmdline_qdel(++argv, environ, &pcmdline);
   if(alp) {
      /*
      ** high level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stdout, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }

   alp = sge_parse_qdel(&pcmdline, &ref_list, &force, &verify, &all_jobs, &all_users, &user_list);

   DPRINTF(("force     = "u32"\n", force));
   DPRINTF(("verify    = "u32"\n", verify));
   DPRINTF(("all_users = "u32"\n", all_users));
   DPRINTF(("all_jobs  = "u32"\n", all_jobs));

   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stdout, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      lFreeList(ref_list);
      SGE_EXIT(1);
   } 
   

#if 0
   jlp = lCreateList("mortui te salutant", JB_Type);
   jep = lCreateElem(JB_Type);
   lSetUlong(jep, JB_force, force);
   lAppendElem(jlp, jep); 
   lSetList(jep, JB_job_identifier_list, lCreateList("job_identifier_list", JRE_Type));
#endif

   if (user_list) {
      lListElem *id;

      id = lAddElemStr(&ref_list, ID_str, "0", ID_Type);
      lSetList(id, ID_user_list, user_list);
   }
   if (all_users) {
      lAddElemStr(&ref_list, ID_str, "0", ID_Type);
   }
   for_each(idep, ref_list) {
      lSetUlong(idep, ID_force, force);
   }

#if 0  
   nxt_rep = lFirst(ref_list);
   while((rep = nxt_rep)) {
      nxt_rep = lNext(rep);
      jrep = lCreateElem(JRE_Type);
      job_number = (u_long32)strtol(lGetString(rep, STR), NULL, 10);
      if(!job_number) {
         ERROR((SGE_EVENT, "\"%s\" is not a job number\n", lGetString(rep, STR)));
         lRemoveElem(ref_list, rep);
      } else {
         lSetUlong(jrep, JRE_job_number, job_number);
         lAppendElem(lGetList(jep, JB_job_identifier_list), jrep); 
      }
   }
#endif   
   if (verify) {
      for_each(idep, ref_list) {
         printf(MSG_JOB_XDELETIONOFJOBY_SS,force?MSG_FORCED:"",lGetString(idep, ID_str));
      }
      SGE_EXIT(0);
   }

   /* set timeout */
   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);   

   /* del */
   cmd = SGE_GDI_DEL;
   if (all_users)
      cmd |= SGE_GDI_ALL_USERS;
   if (all_jobs)
      cmd |= SGE_GDI_ALL_JOBS;

   if (ref_list) {
      if (force != 1) {
         /* standard request without force */
         alp = sge_gdi(SGE_JOB_LIST, cmd, &ref_list, NULL, NULL);
      } else {
         /* request with force flag */
         DPRINTF(("force is specified\n"));
         if ( (have_master_privileges = sge_gdi_check_permission(MANAGER_CHECK)) == TRUE) {
            /* manager is making the request, old behaviour */
            DPRINTF(("manager can do force\n"));
         alp = sge_gdi(SGE_JOB_LIST, cmd, &ref_list, NULL, NULL);
         } else {
            /* no connection to qmaster */
            if (have_master_privileges == -10) {   
               fprintf(stderr, MSG_SGETEXT_NOQMASTER);
               lFreeList(alp);
               lFreeList(jlp);
               lFreeList(ref_list);
               sge_gdi_shutdown();
               SGE_EXIT(1);
            }

            /* ordinary user has secified the force flag */
            DPRINTF(("user call with force flag\n"));

            /* set force flag to 0 (no force in first call) */
            for_each(idep, ref_list) {
               lSetUlong(idep, ID_force, 0);
            }
            alp = sge_gdi(SGE_JOB_LIST, cmd, &ref_list, NULL, NULL);
        
            /* AN_status can have following states:
               STATUS_EEXIST - element does not exist (job deleted)
               STATUS_OK     - everything was fine 
            */
            lFreeList(alp); 
            alp = NULL;        
           
            /* wait for qmaster and execd to remove job */ 
            printf("%s ",MSG_ANSWER_SUCCESSCHECKWAIT);
            for(wait=12;wait>0;wait--) {
               printf(".");
               fflush(stdout);
               sleep(5); 
            }  
            printf( "\n"); 
            /* set force flag for second call */ 
            for_each(idep, ref_list) {
               lSetUlong(idep, ID_force, force);
            }       
            alp = sge_gdi(SGE_JOB_LIST, cmd, &ref_list, NULL, NULL);
            
            ref_elem = lFirst(ref_list);
            for_each(aep , alp) {
               status = lGetUlong(aep, AN_status);
               if (ref_elem != NULL) {
                  jobName = lGetString(ref_elem, ID_str);
                  ref_elem = lNext(ref_elem);
               } else {
                  jobName = MSG_ANSWER_UNKNOWN;
               }
               if ( (status == STATUS_OK) || (status == STATUS_EEXIST) ) {
                       
                  printf( MSG_ANSWER_JOBXREMOVED_S , jobName);
               } else {
                  fprintf(stderr, MSG_ANSWER_CANTDELETEJOB_S , jobName);
                  lFreeList(alp);
                  lFreeList(jlp);
                  lFreeList(ref_list);
                  sge_gdi_shutdown();
                  SGE_EXIT(1);
               }
            }

            lFreeList(alp);
            lFreeList(jlp);
            lFreeList(ref_list);
            sge_gdi_shutdown();
            SGE_EXIT(0);
            /* here stops the ordinary user call */
         }
      }
   } else {
      printf(MSG_PARSE_NOOPTIONARGUMENT);
      qdel_usage(stderr, NULL);
      SGE_EXIT(1);
   }
   for_each(aep, alp) 
      printf("%s", lGetString(aep, AN_text) );

   lFreeList(alp);
   lFreeList(jlp);
   lFreeList(ref_list);
   sge_gdi_shutdown();
   exit(0);
   return 0;
   /* SGE_EXIT(0); */
}

/****
 **** sge_parse_cmdline_qdel (static)
 ****
 **** 'stage 1' parsing of qdel-options. parses options
 **** with their arguments and stores them in ppcmdline.
 ****/
static lList *sge_parse_cmdline_qdel(
char **argv,
char **envp,
lList **ppcmdline 
) {
char **sp;
char **rp;
stringT str;
lList *alp = NULL;

   DENTER(TOP_LAYER, "sge_parse_cmdline_qdel");

   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;
      
      /* -verify */
      if ((rp = parse_noopt(sp, "-verify", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -f option */
      if ((rp = parse_noopt(sp, "-f", "--force", ppcmdline, &alp)) != sp)
         continue;

      /* -uall */
      if ((rp = parse_noopt(sp, "-uall", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -u */
      if (!strcmp("-u", *sp)) {
         lList *user_list = NULL;
         lListElem *ep_opt;

         sp++;
         if (*sp) {
            sge_parse_string_list(&user_list, *sp, STR, ST_Type);

            ep_opt = sge_add_arg(ppcmdline, 0, lListT, *(sp - 1), *sp);
            lSetList(ep_opt, SPA_argval_lListT, user_list);
            sp++;
         }
         rp = sp;
         continue;
      }
 
      /* job id's */
      if ((rp = parse_param(sp, "jobs", ppcmdline, &alp)) != sp) {
         continue;
      }

      /* oops */
      sprintf(str, MSG_PARSE_INVALIDOPTIONARGUMENTX_S, *sp);
      qdel_usage(stderr, NULL);
      sge_add_answer(&alp, str, STATUS_ESEMANTIC, 0);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}

/****
 **** sge_parse_qdel (static)
 ****
 **** 'stage 2' parsing of qdel-options. Gets the options
 **** from ppcmdline, sets the force-flag and puts the
 **** job-numbers (strings) in ppreflist.
 ****/
static lList *sge_parse_qdel(
lList **ppcmdline,
lList **ppreflist,
u_long32 *pforce,
u_long32 *pverify,
u_long32 *palljobs,
u_long32 *pallusers,
lList **ppuserlist 
) {
lList *alp = NULL;
u_long32 helpflag;
lListElem *ep;

   DENTER(TOP_LAYER, "sge_parse_qdel");

   /* Loop over all options. Only valid options can be in the
      ppcmdline list. 
   */

   while(lGetNumberOfElem(*ppcmdline))
   {
      if(parse_flag(ppcmdline, "-help",  &alp, &helpflag)) {
         qdel_usage(stdout, NULL);
         SGE_EXIT(0);
         break;
      }
      if(parse_flag(ppcmdline, "-f", &alp, pforce)) 
         continue;

      if(parse_flag(ppcmdline, "-verify", &alp, pverify)) 
         continue;

      if(parse_flag(ppcmdline, "-uall", &alp, pallusers))
         continue;

      while ((ep = lGetElemStr(*ppcmdline, SPA_switch, "-u"))) {
         lXchgList(ep, SPA_argval_lListT, ppuserlist);
         lRemoveElem(*ppcmdline, ep);
      } 

      if(parse_multi_stringlist(ppcmdline, "-u", &alp, ppuserlist, ST_Type, STR)) {
         continue;  
      }

      if(parse_multi_jobtaskslist(ppcmdline, "jobs", &alp, ppreflist)) {
         if (lGetNumberOfElem(*ppreflist) == 1
             && !strcmp(lGetString(lFirst(*ppreflist), ID_str), "all")) 
            (*palljobs) = 1; 
         continue;
      }
   }
   if ((*pallusers || *ppuserlist) && *ppreflist) {
      sge_add_answer(&alp, MSG_OPTION_SELECTUSERSANDJOBIDSTOGETHERNOTALLOWED, STATUS_EUNKNOWN, 0);
   }
   if (*ppuserlist && *pallusers) {
      sge_add_answer(&alp, MSG_OPTION_OPTUANDOPTUALLARENOTALLOWDTOGETHER, STATUS_EUNKNOWN, 0);
   }

   DEXIT;
   return alp;
}

/****
 **** qdel_usage (static)
 ****
 **** displays usage of qdel on file fp.
 **** Is what NULL, full usage will be displayed.
 ****
 **** Returns always 1.
 ****
 **** If what is a pointer to an option-string,
 **** only usage for that option will be displayed.
 ****   ** not implemented yet! **
 ****/
static int qdel_usage(
FILE *fp,
char *what 
) {
   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION));

   if(!what) {
      /* display full usage */
      fprintf(fp, "%s qdel [options]\n", MSG_SRC_USAGE);      

      
      fprintf(fp, "  [-f]             %s",  MSG_QDEL_f_OPT_USAGE);
      fprintf(fp, "  [-help]          %s",  MSG_QDEL_help_OPT_USAGE);
      fprintf(fp, "  [-verify]        %s",  MSG_QDEL_verify_OPT_USAGE);
      fprintf(fp, "  job_task_list|   %s",  MSG_QDEL_del_list_1_OPT_USAGE);
      fprintf(fp, "  'all'|           %s",  MSG_QDEL_del_list_2_OPT_USAGE);
      fprintf(fp, "  -u user_list|    %s",  MSG_QDEL_del_list_3_OPT_USAGE);
      fprintf(fp, "  -uall            %s\n",  MSG_QDEL_del_list_4_OPT_USAGE);
      fprintf(fp, "job_task_list      job_tasks{job_tasks}\n");
      fprintf(fp, "job_tasks          job_id['.'task_id_range]\n");
      fprintf(fp, "task_id_range      task_id['-'task_id[':'step]]\n");
      fprintf(fp, "user_list          user{','user}\n");
   } else {
      /* display option usage */
      fprintf(fp, MSG_QDEL_not_available_OPT_USAGE_S, what);
   }
   return 1;
}

