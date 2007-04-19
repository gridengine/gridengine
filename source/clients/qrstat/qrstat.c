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
#include "sig_handlers.h"
#include "qrstat.h"
#include "parse.h"

#include "sge_time.h"
#include "show_job.h"
#include "get_path.h"
#include "qrstat_report_handler.h"
#include "qrstat_report_handler_xml.h"
#include "qrstat_report_handler_stdout.h"
#include "qrstat_filter.h"

#include "sgeobj/cull_parse_util.h"
#include "sgeobj/sge_advance_reservation.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_mailrec.h"

#include "msg_common.h"

extern char **environ;

static bool 
sge_parse_qrstat(lList **answer_list, qrstat_env_t *qrstat_env, lList **cmdline)
{
   bool ret = true;
   
   DENTER(TOP_LAYER, "sge_parse_qrstat");

   qrstat_env->is_summary = true;
   while(lGetNumberOfElem(*cmdline)) {
      u_long32 value;
   
      /* -help */
      if (opt_list_has_X(*cmdline, "-help")) {
         sge_usage(QRSTAT, stdout);
         ret = false;
         break;
      }

      /* -u */
      while (parse_multi_stringlist(cmdline, "-u", answer_list, 
                                    &(qrstat_env->user_list), ST_Type, ST_name)) {
         qrstat_filter_add_core_attributes(qrstat_env);
         qrstat_filter_add_u_where(qrstat_env);
         continue;
      }

      /* -explain */
      while (parse_flag(cmdline, "-explain", answer_list, &value)) {
         qrstat_filter_add_core_attributes(qrstat_env);
         qrstat_filter_add_explain_attributes(qrstat_env);
         qrstat_env->is_explain = (value > 0) ? true : false;
         continue;
      }

      /* -xml */
      while (parse_flag(cmdline, "-xml", answer_list, &value)) {
         qrstat_filter_add_core_attributes(qrstat_env);
         qrstat_filter_add_xml_attributes(qrstat_env);
         qrstat_env->is_xml = (value > 0) ? true : false;
         continue;
      }

      /* -ar */
      while (parse_u_longlist(cmdline, "-ar", answer_list, &(qrstat_env->ar_id_list))) {         
         qrstat_filter_add_core_attributes(qrstat_env);
         qrstat_filter_add_ar_attributes(qrstat_env);
         qrstat_filter_add_ar_where(qrstat_env);
         qrstat_env->is_summary = false;
         continue;      
      }

      if (lGetNumberOfElem(*cmdline)) {
         sge_usage(QRSTAT, stdout);
         answer_list_add(answer_list, MSG_PARSE_TOOMANYOPTIONS, 
                         STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         ret = false;
         break;
      }
   } 
   DRETURN(ret);
}

/************************************************************************/
int main(int argc, char **argv) {
   bool lret = true;
   lList *pcmdline = NULL;
   lList *answer_list = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;
   qrstat_env_t qrstat_env;

   DENTER_MAIN(TOP_LAYER, "qrsub");

   sge_prof_setup();

   /* Set up the program information name */
   sge_setup_sig_handlers(QRSTAT);

   log_state_set_log_gui(1);

   if (sge_gdi2_setup(&ctx, QRSTAT, &answer_list) != AE_OK) {
      answer_list_output(&answer_list);
      goto error_exit;
   }

   qrstat_filter_init(&qrstat_env);
   qrstat_filter_set_ctx(&qrstat_env, ctx);

   /*
    * stage 1: commandline parsing
    */
   answer_list = cull_parse_cmdline(QRSTAT, argv+1, environ, &pcmdline, FLG_USE_PSEUDOS);
   if (answer_list != NULL) {
      answer_list_output(&answer_list);
      lFreeList(&pcmdline);
      goto error_exit;
   }
  
   /* 
    * stage 2: evalutate switches and modify qrstat_env
    */
   lret = sge_parse_qrstat(&answer_list, &qrstat_env, &pcmdline);
   if (!lret) {
      answer_list_output(&answer_list);
      lFreeList(&pcmdline);
      goto error_exit;
   }

   /* 
    * stage 3: fetch data from master 
    */
   {
      answer_list = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_GET, &qrstat_env.ar_list, 
                     qrstat_env.where_AR_Type, qrstat_env.what_AR_Type);

      if (answer_list_has_error(&answer_list)) {
         answer_list_output(&answer_list);
         goto error_exit;
      }
   }

   /*
    * stage 4: create output in correct format
    */
   {
      qrstat_report_handler_t *handler = NULL;

      if (qrstat_env.is_xml) {
         handler = qrstat_create_report_handler_xml(&qrstat_env, &answer_list);
      } else {
         handler = qrstat_create_report_handler_stdout(&qrstat_env, &answer_list);
      }
      qrstat_print(&answer_list, handler, &qrstat_env);
      if (qrstat_env.is_xml) {
         qrstat_destroy_report_handler_xml(&handler, &answer_list); 
      } else {
         qrstat_destroy_report_handler_stdout(&handler, &answer_list); 
      }
   }

   sge_prof_cleanup();
   sge_gdi2_shutdown((void**)&ctx);
   DRETURN(0);

error_exit:
   sge_prof_cleanup();
   sge_gdi2_shutdown((void**)&ctx);
   SGE_EXIT((void**)&ctx, 1);
   DRETURN(1);
}

