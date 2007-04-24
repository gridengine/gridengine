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

#include "basis_types.h"
#include "sge.h"

#include "symbols.h"
#include "sge_bootstrap.h"
#include "sge_centry.h"
#include "cull_parse_util.h"
#include "sge_job.h"
#include "sge_gdi.h"
#include "gdi/sge_gdi_ctx.h"
#include "sge_all_listsL.h"

#include "sgermon.h"
#include "sge_answer.h"
#include "sge_log.h"
#include "read_defaults.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "usage.h"
#include "parse.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "sig_handlers.h"

#include "sge_advance_reservation.h"

extern char **environ;

static bool sge_parse_qrsub(lList *pcmdline, lList **alpp, lListElem **ar);

/************************************************************************/
int main(int argc, char **argv) {
   lList *pcmdline = NULL;
   lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   lListElem *ar = NULL;

   DENTER_MAIN(TOP_LAYER, "qrsub");

   sge_prof_setup();

   /* Set up the program information name */
   sge_setup_sig_handlers(QRSUB);

   log_state_set_log_gui(1);

   if (sge_gdi2_setup(&ctx, QRSUB, &alp) != AE_OK) {
      answer_list_output(&alp);
      goto error_exit;
   }

   /*
   ** stage 1 of commandline parsing
   */
   {
      dstring file = DSTRING_INIT;
      const char *user = ctx->get_username(ctx);
      const char *cell_root = ctx->get_cell_root(ctx);

      /* arguments from SGE_ROOT/common/sge_ar_request file */
      get_root_file_path(&file, cell_root, SGE_COMMON_DEF_AR_REQ_FILE);
      if ((alp = parse_script_file(QRSUB, sge_dstring_get_string(&file), "", &pcmdline, environ, 
         FLG_HIGHER_PRIOR | FLG_IGN_NO_FILE)) == NULL) {
         /* arguments from $HOME/.sge_ar_request file */
         if (get_user_home_file_path(&file, SGE_HOME_DEF_AR_REQ_FILE, user, &alp)) {
            lFreeList(&alp);
            alp = parse_script_file(QRSUB, sge_dstring_get_string(&file), "", &pcmdline, environ, 
            FLG_HIGHER_PRIOR | FLG_IGN_NO_FILE);
         }
      }
      sge_dstring_free(&file); 

      if (alp) {
         answer_list_output(&alp);
         lFreeList(&pcmdline);
         goto error_exit;
      }
   }
   
   alp = cull_parse_cmdline(QRSUB, argv+1, environ, &pcmdline, FLG_USE_PSEUDOS);

   if (answer_list_print_err_warn(&alp, NULL, "qrsub: ", MSG_WARNING) > 0) {
      lFreeList(&pcmdline);
      goto error_exit;
   }

   /*
   ** stage 2 of command line parsing
   */
   ar = lCreateElem(AR_Type);

   if (!sge_parse_qrsub(pcmdline, &alp, &ar)) {
      answer_list_output(&alp);
      lFreeList(&pcmdline);
      goto error_exit;
   }

   /* test code */
   {
      lList *ar_lp = NULL;

      ar_lp = lCreateList(NULL, AR_Type);
      lAppendElem(ar_lp, ar);
      alp = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION, &ar_lp, NULL, NULL);

      if (ar_lp != NULL) {
         /* check if the correct ar_id is returned */
         printf("returned ar_id: "sge_u32, lGetUlong(lFirst(ar_lp), AR_id));
      }
      if (lFirst(ar_lp) != NULL) {
         lWriteElemTo(lFirst(ar_lp), stdout);
      }
      lFreeList(&ar_lp);
/*        */
      answer_list_on_error_print_or_exit(&alp, stdout);
   }

   sge_prof_cleanup();
   sge_gdi2_shutdown((void**)&ctx);
   DEXIT;
   DRETURN(0);

error_exit:
   sge_prof_cleanup();
   sge_gdi2_shutdown((void**)&ctx);
   SGE_EXIT((void**)&ctx, 1);
   DRETURN(1);
}

static bool sge_parse_qrsub(lList *pcmdline, lList **alpp, lListElem **ar)
{
   lListElem *ep = NULL;
   DENTER(TOP_LAYER, "sge_parse_qrsub");

   /*  -help 	 print this help */
   if ((ep = lGetElemStr(pcmdline, SPA_switch, "-help"))) {
      lRemoveElem(pcmdline, &ep);
      sge_usage(QRSUB, stdout);
      DEXIT;
      SGE_EXIT(NULL, 0);
   }

   /*  -a date_time 	 start time in [[CC]YY]MMDDhhmm[.SS] SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-a"))) {
      lSetUlong(*ar, AR_start_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   /*  -e date_time 	 end time in [[CC]YY]MMDDhhmm[.SS] SGE_ULONG*/
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-e"))) {
      lSetUlong(*ar, AR_end_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   /*  -d time 	 duration in TIME format SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-d"))) {
      lSetUlong(*ar, AR_duration, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }
   
   /*  -w e/v 	 validate availability of AR request, default e SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-w"))) {
      lSetUlong(*ar, AR_verify, lGetInt(ep, SPA_argval_lIntT));
      lRemoveElem(pcmdline, &ep);
   }
  
   /*  -N name 	 AR name SGE_STRING */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-N"))) {
      lSetString(*ar, AR_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(pcmdline, &ep);
   }
      
   /*  -A account_string 	 AR name in accounting record SGE_STRING */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-A"))) {
      lSetString(*ar, AR_account, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(pcmdline, &ep);
   }
      
   /*  -l resource_list 	 request the given resources  SGE_LIST */
   parse_list_simple(pcmdline, "-l", *ar, AR_resource_list, 0, 0, FLG_LIST_APPEND);
   centry_list_remove_duplicates(lGetList(*ar, AR_resource_list));

   /*  -u wc_user 	       access list SGE_LIST */
   /*  -u ! wc_user TBD: Think about eval_expression support in compare allowed and excluded lists */
   parse_list_simple(pcmdline, "-u", *ar, AR_acl_list, 0, 0, FLG_LIST_APPEND);
   /*  -u ! list separation */
   ep = lFirst(lGetList(*ar,  AR_acl_list));
   if (ep) {
      lList *xacl=NULL; 
      do  {
         const char *s = NULL;
         s = lGetString(ep, ST_name);
         if (s[0] == '!') { /* move this element to xacl_list */
            lListElem *new=NULL;
            if(xacl == NULL) { /* the xacl_list does not exist */
               xacl = lCreateList("xacl_list", ST_Type );
            }  
            new = lCreateElem(ST_Type);
            s++;
            lSetString(new, ST_name, s);
            lAppendElem(xacl, new);
            lRemoveElem(lGetList(*ar,  AR_acl_list), &ep);
            ep = lFirst(lGetList(*ar,  AR_acl_list));
         } else {
           ep = lNext(ep);
         }  
      } while (ep);   
      if(xacl != NULL) {
         lSetList(*ar, AR_xacl_list, xacl);
      }
   }

   /*  -q wc_queue_list 	 reserve in queue(s) SGE_LIST */
   parse_list_simple(pcmdline, "-q", *ar, AR_queue_list, 0, 0, FLG_LIST_APPEND);

  /*    -pe pe_name slot_range reserve slot range for parallel jobs */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-pe"))) {
      lSetString(*ar, AR_pe, lGetString(ep, SPA_argval_lStringT)); /* SGE_STRING, */
      lSwapList(*ar, AR_pe_range, ep, SPA_argval_lListT);       /* SGE_LIST */
      lRemoveElem(pcmdline, &ep);
   }
   /*   AR_master_queue_list  -masterq wc_queue_list, SGE_LIST bind master task to queue(s) */
   parse_list_simple(pcmdline, "-masterq", *ar, AR_master_queue_list, 0, 0, FLG_LIST_APPEND);

   /*  -ckpt ckpt-name 	 reserve in queue with ckpt method SGE_STRING */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-ckpt"))) {
      lSetString(*ar, AR_checkpoint_name, lGetString(ep, SPA_argval_lStringT));
      lRemoveElem(pcmdline, &ep);
   }
   
   /*  -m b/e/a/n 	 define mail notification events SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-m"))) {
      u_long32 ul;
      u_long32 old_mail_opts;

      ul = lGetInt(ep, SPA_argval_lIntT);
      if  ((ul & NO_MAIL)) {
         lSetUlong(*ar, AR_mail_options, 0);
      } else {
         old_mail_opts = lGetUlong(*ar, AR_mail_options);
         lSetUlong(*ar, AR_mail_options, ul | old_mail_opts);
      }
      lRemoveElem(pcmdline, &ep);
   }

   /*   -M user[@host],... 	 notify these e-mail addresses SGE_LIST*/
   parse_list_simple(pcmdline, "-M", *ar, AR_mail_list, MR_host, MR_user, FLG_LIST_MERGE);

   /*  -he yes/no 	 hard error handling SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-he"))) {
      lSetUlong(*ar, AR_error_handling, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   /*   -now 	 reserve in queues with qtype interactive  SGE_ULONG */
   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-now"))) {
      u_long32 ar_now = lGetUlong(*ar, AR_type);
      if(lGetInt(ep, SPA_argval_lIntT)) {
         JOB_TYPE_SET_IMMEDIATE(ar_now);
      } else {
         JOB_TYPE_CLEAR_IMMEDIATE(ar_now);
      }

      lSetUlong(*ar, AR_type, ar_now);

      lRemoveElem(pcmdline, &ep);
   }

  /* Remove the script elements. They are not stored in the ar structure */
  if ((ep = lGetElemStr(pcmdline, SPA_switch, STR_PSEUDO_SCRIPT))) {
      lRemoveElem(pcmdline, &ep);
   }

   if ((ep = lGetElemStr(pcmdline, SPA_switch, STR_PSEUDO_SCRIPTLEN))) {
      lRemoveElem(pcmdline, &ep);
   }

   if ((ep = lGetElemStr(pcmdline, SPA_switch, STR_PSEUDO_SCRIPTPTR))) {
      lRemoveElem(pcmdline, &ep);
   }

   ep = lFirst(pcmdline);   
   if(ep) {
      answer_list_add_sprintf(alpp, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR,
                              MSG_PARSE_INVALIDOPTIONARGUMENTX_S,
                              lGetString(ep,SPA_switch)); 
      DRETURN(false);
   }

   if (lGetUlong(*ar, AR_start_time) == 0 && lGetUlong(*ar, AR_end_time) != 0 && lGetUlong(*ar, AR_duration) != 0) {
      lSetUlong(*ar, AR_start_time, lGetUlong(*ar, AR_end_time) - lGetUlong(*ar, AR_duration));
   } else if (lGetUlong(*ar, AR_start_time) != 0 && lGetUlong(*ar, AR_end_time) == 0 && lGetUlong(*ar, AR_duration) != 0) {
      lSetUlong(*ar, AR_end_time, lGetUlong(*ar, AR_start_time) + lGetUlong(*ar, AR_duration));
   } else if (lGetUlong(*ar, AR_start_time) != 0 && lGetUlong(*ar, AR_end_time) != 0 && lGetUlong(*ar, AR_duration) == 0) {
      lSetUlong(*ar, AR_duration, lGetUlong(*ar, AR_end_time) - lGetUlong(*ar, AR_start_time));
   }

   DRETURN(true);
}
