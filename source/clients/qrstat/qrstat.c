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

extern char **environ;

/************************************************************************/
int main(int argc, char **argv) {
   lList *pcmdline = NULL;
   lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   DENTER_MAIN(TOP_LAYER, "qrsub");

   sge_prof_setup();

   /* Set up the program information name */
   sge_setup_sig_handlers(QRSTAT);

   log_state_set_log_gui(1);

   if (sge_gdi2_setup(&ctx, QRSTAT, &alp) != AE_OK) {
      answer_list_output(&alp);
      goto error_exit;
   }

   /*
   ** stage 1 of commandline parsing
   */
   if ((alp = cull_parse_cmdline(QRSTAT, argv+1, environ, &pcmdline, FLG_USE_PSEUDOS))) {
      answer_list_output(&alp);
      lFreeList(&pcmdline);
      goto error_exit;
   }
   /*
    * show usage if -help was in commandline
    * AR TBD
    * this function should be in sge_parse_qrstat (analogue to sge_parse_qrsub)
    */
   if (opt_list_has_X(pcmdline, "-help")) {
      sge_usage(QRSTAT, stdout);
      sge_prof_cleanup();
      sge_gdi2_shutdown((void**)&ctx);
      SGE_EXIT(NULL, 0);
   }

   /* this is only debug code */
   {
      lList *ar_list = NULL;
      lEnumeration *what = NULL;
      what = lWhat("%T(ALL)", AR_Type);
      alp = ctx->gdi(ctx, SGE_AR_LIST, SGE_GDI_GET, &ar_list, NULL, what);
      lFreeWhat(&what);

      lWriteListTo(ar_list, stdout);

      if (answer_list_has_error(&alp)) {
         answer_list_output(&alp);
         goto error_exit;
      }

      lFreeList(&ar_list);
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

