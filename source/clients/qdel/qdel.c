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

#include "sgermon.h"
#include "sge_log.h"
#include "sge_gdi.h"
#include "usage.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sig_handlers.h"
#include "parse.h"
#include "sge_answer.h"
#include "gdi_checkpermissions.h"
#include "sge_feature.h"
#include "sge_unistd.h"
#include "sge_str.h"

#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qdel.h"

static lList *sge_parse_cmdline_qdel(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qdel(lList **ppcmdline, lList **ppreflist, u_long32 *pforce, u_long32 *pverify, u_long32 *all_jobs, u_long32 *all_users, lList **ppuserlist);
static int qdel_usage(FILE *fp, char *what);

extern char **environ;

int main(int argc, char *argv[]);

/************************************************************************/
int main(int argc, char **argv) {
   /* lListElem *rep, *nxt_rep, *jep, *aep, *jrep, *idep; */
   lListElem *aep, *idep;
   lList *jlp = NULL, *alp = NULL, *pcmdline = NULL, *ref_list = NULL, *user_list=NULL;
   u_long32 force = 0, verify = 0, all_users = 0, all_jobs = 0;
   int cmd;
   int wait;
   unsigned long status = 0;
   bool have_master_privileges;

   DENTER_MAIN(TOP_LAYER, "qdel");

   sge_gdi_param(SET_MEWHO, QDEL, NULL);
   if (sge_gdi_setup(prognames[QDEL], &alp) != AE_OK) {
      answer_exit_if_not_recoverable(lFirst(alp));
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
      goto error_exit;
   } 
   
   if (user_list) {
      lListElem *id;

      id = lAddElemStr(&ref_list, ID_str, "0", ID_Type);
      lSetList(id, ID_user_list, user_list);
   }
   if (all_users) {
      lAddElemStr(&ref_list, ID_str, "0", ID_Type);
   }

   if (verify) {
      for_each(idep, ref_list) {
         printf(MSG_JOB_XDELETIONOFJOBY_SS,force?MSG_FORCED:"",lGetString(idep, ID_str));
      }
      goto default_exit;
   }

   /* set timeout */
   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);   

   /* prepare gdi request for 'all' and '-uall' parameters */
   cmd = SGE_GDI_DEL;
   if (all_users) {
      cmd |= SGE_GDI_ALL_USERS;
   }
   if (all_jobs) {
      cmd |= SGE_GDI_ALL_JOBS;
   }

   /* Are there jobs which should be deleted? */
   if (!ref_list) {
      printf(MSG_PARSE_NOOPTIONARGUMENT);
      qdel_usage(stderr, NULL);
      goto error_exit;
   }

   /* Has the user the permission to use the the '-f' (forced) flag */
   have_master_privileges = false;
   if (force == 1) {
      have_master_privileges = sge_gdi_check_permission(&alp, MANAGER_CHECK);
      if (have_master_privileges == -10) {
         /* -10 indicates no connection to qmaster */

         /* fills SGE_EVENT with diagnosis information */
         if (alp != NULL) {
            if (lGetUlong(aep = lFirst(alp), AN_status) != STATUS_OK) {
               fprintf(stderr, "%s", lGetString(aep, AN_text));
            }
            lFreeList(alp);
            alp = NULL;
         }
         goto error_exit;
      }  
      if (alp != NULL) {
         lFreeList(alp);
         alp = NULL;
      }
   }

   /* delete the job */
   {
      int delete_mode;

      /* 
       * delete_mode:
       *    1 => admin user used '-f'     
       *         -> forced deletion
       *    7 => non admin user used '-f' 
       *         -> first try normal deletion
       *         -> wait a minute
       *         -> forced deletion (delete_mode==5)
       *    3 => normal qdel
       *         -> normal deletion
       */
      if (force == 1) {
         if (have_master_privileges == true) {
            delete_mode = 1;
         } else {
            delete_mode = 7;
         }
      } else {
         delete_mode = 3;
      }
      while (delete_mode) {
         int no_forced_deletion = delete_mode & 2;
         int do_again;
         int error_occured;
         int first_try = 1;

         for_each(idep, ref_list) {
            lSetUlong(idep, ID_force, !no_forced_deletion);
         } 

         /*
          * Send delete request to master. If the master is not able to
          * execute the whole request when the 'all' or '-uall' flag was
          * specified, then the master may discontinue the 
          * transaction (STATUS_OK_DOAGAIN). In this case the client has 
          * to redo the transaction.
          */ 
         do_again = 0;
         do {
            if (do_again) {
               /*
                * Give other clients (gdi requests) the chance to
                * be handled by the commd before we repeat this request
                */
               sleep(1);
            }
            do_again = 0;
            error_occured = 0;
            alp = sge_gdi(SGE_JOB_LIST, cmd, &ref_list, NULL, NULL);
            for_each(aep, alp) {
               status = lGetUlong(aep, AN_status);

               if (delete_mode != 5 && 
                   ((first_try == 1 && status != STATUS_OK_DOAGAIN) ||
                    (first_try == 0 && status == STATUS_OK))) {
                  printf("%s", lGetString(aep, AN_text) );
               }
               if (status != STATUS_OK && 
                   status != STATUS_OK_DOAGAIN) {
                  error_occured = 1;
               }
               if (status == STATUS_OK_DOAGAIN) {
                  do_again = 1;
               }
            }
            first_try = 0;
         } while ((user_list != NULL || all_users || all_jobs) && do_again && !error_occured);

         if (delete_mode == 7) {
            /* 
             * loop for one minute
             * this should prevent non-admin-users from using the '-f'
             * option regularly
             */
            for(wait = 12; wait > 0; wait--) {
               printf(".");
               fflush(stdout);
               sleep(5);
            } 
            printf("\n");

            delete_mode = 5;
         } else {
            delete_mode = 0;
         } 
      }
   }

default_exit:
   lFreeList(alp);
   lFreeList(jlp);
   lFreeList(ref_list);
   sge_gdi_shutdown();
   SGE_EXIT(0);
   return 0;

error_exit:
   lFreeList(alp);
   lFreeList(jlp);
   lFreeList(ref_list);
   sge_gdi_shutdown();
   SGE_EXIT(1); 
   DEXIT;
   return 1;
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
            str_list_parse_from_string(&user_list, *sp, ",");

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
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
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

      if(parse_multi_stringlist(ppcmdline, "-u", &alp, ppuserlist, ST_Type, ST_name)) {
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
      answer_list_add(&alp, MSG_OPTION_SELECTUSERSANDJOBIDSTOGETHERNOTALLOWED, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
   }
   if (*ppuserlist && *pallusers) {
      answer_list_add(&alp, MSG_OPTION_OPTUANDOPTUALLARENOTALLOWDTOGETHER, 
                      STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
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
   dstring ds;
   char buffer[256];

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));

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

