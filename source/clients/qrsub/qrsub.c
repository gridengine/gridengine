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

#include "sge_bootstrap.h"

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

   lWriteElemTo(ar, stdout);

   /* test code */
   {
      lList *ar_lp = NULL;

      lSetString(ar, AR_name, "test_ar");
      
      ar_lp = lCreateList(NULL, AR_Type);
      lAppendElem(ar_lp, ar);
      alp = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_ADD, &ar_lp, NULL, NULL);

      lFreeList(&ar_lp);
      lWriteListTo(alp, stdout);
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

   if ((ep = lGetElemStr(pcmdline, SPA_switch, "-help"))) {
      lRemoveElem(pcmdline, &ep);

      sge_usage(QRSUB, stdout);
      DEXIT;
      SGE_EXIT(NULL, 0);
   }

   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-a"))) {
      lSetUlong(*ar, AR_start_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-e"))) {
      lSetUlong(*ar, AR_end_time, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   while ((ep = lGetElemStr(pcmdline, SPA_switch, "-d"))) {
      lSetUlong(*ar, AR_duration, lGetUlong(ep, SPA_argval_lUlongT));
      lRemoveElem(pcmdline, &ep);
   }

   for_each(ep, pcmdline) {
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
