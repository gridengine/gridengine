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

#include <stdio.h>
#include <stdlib.h>

#include "sge_unistd.h"
#include "sge_any_request.h"
#include "sge_gdiP.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"

#include "msg_clients_common.h"

#include "sge_mirror.h"
#include "sge_event.h"

bool print_event(sge_object_type type, sge_event_action action, 
                lListElem *event, void *clientdata)
{
   char buffer[1024];
   dstring buffer_wrapper;

   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   DPRINTF(("%s\n", event_text(event, &buffer_wrapper)));


   /* create a callback error to test error handling */
   if(type == SGE_TYPE_GLOBAL_CONFIG) {
      return false;
   }
   
   return true;
}

int main(int argc, char *argv[])
{
   int cl_err = 0;
   lList *alp = NULL;

   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   sge_gdi_param(SET_MEWHO, QEVENT, NULL);
   if (sge_gdi_setup(prognames[QEVENT], &alp)!=AE_OK) {
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QEVENT);

   if (reresolve_me_qualified_hostname() != CL_OK) {
      SGE_EXIT(1);
   }   

   sge_mirror_initialize(EV_ID_ANY, "test_sge_mirror");
   sge_mirror_subscribe(SGE_TYPE_ALL, print_event, NULL, NULL, NULL, NULL);
   
   while(!shut_me_down) {
      sge_mirror_process_events();
   }

   sge_mirror_shutdown();

   DEXIT;
   return EXIT_SUCCESS;
}
