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
#include "usage.h"
#include "parse_qconf.h"
#include "sge_exit.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_clients_common.h"
#include "msg_common.h"

extern char **environ;

int main(int argc, char *argv[]);

/************************************************************************/
int main(int argc, char **argv)
{
   int cl_err = 0;
   int tmp_cl_err = 0;
   DENTER_MAIN(TOP_LAYER, "qconf");

   sge_gdi_param(SET_MEWHO, QCONF, NULL);
/*    sge_gdi_param(SET_ISALIVE, 1, NULL); */
   if ((cl_err = sge_gdi_setup(prognames[QCONF]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QCONF);

   tmp_cl_err = reresolve_me_qualified_hostname();
   if (tmp_cl_err != CL_OK) {
      /* fills SGE_EVENT with diagnosis information */
      generate_commd_port_and_service_status_message(tmp_cl_err, SGE_EVENT);
      fprintf(stderr, SGE_EVENT);
      SGE_EXIT(1);
   }   

   if (argc == 1) {
      sge_usage(stderr);
      SGE_EXIT(1);
   }

   if (sge_parse_qconf(++argv))
      SGE_EXIT(1);
   else
      SGE_EXIT(0);
   return 0;
}
