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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_connect.h"
#include "rmon_request.h"
#include "rmon_m_c_mquit.h"
#include "rmon_restart.h"
#include "rmon_conf.h"

#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "msg_rmon.h"
/*****************************************************************/

int rmon_m_c_mquit(
int sfd 
) {
   spy_list_type *sl;
   int ret_val;

#undef  FUNC
#define FUNC "rmon_m_c_mquit"
   DENTER;

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   rmon_delete_cl(&client_list);
   rmon_delete_wl(&wait_list);
   rmon_delete_tl(&first_spy, &first_client);

   DPRINTF(("Program                Host            Port  Status\n"));
   for (sl = spy_list; sl; sl = sl->next) {

      /* send each spy a SLEEP request ... */

      ret_val = rmon_connect_anyone(&sfd, SLEEP, sl->inet_addr, sl->port);

      DPRINTF(("%-22.22s %-15.15s %ld           %s\n",
            sl->programname, rmon_get_host_name(sl->inet_addr), sl->port,
               (!ret_val) ? "not reached" : "reached"));

      shutdown(sfd, 2);
      close(sfd);
   }

   if (!rmon_lock_restart_file()) {
      DPRINTF(("cannot lock restart file\n"));
      rmon_errf(TERMINAL, MSG_RMON_CANTLOCKRESTARTFILE );
   }

   if (!rmon_save_restart_file())
      rmon_errf(TRIVIAL, MSG_RMON_CANTSAVERESTARTFILE );

   if (!rmon_unlock_restart_file()) {
      DPRINTF(("cannot unlock restart file\n"));
      rmon_errf(TERMINAL, MSG_RMON_CANTUNLOCKRESTARTFILE );
   }

   if (!rmon_save_conf_file())
      rmon_errf(TRIVIAL, MSG_RMON_CANTSAVECONFFILE);

   rmon_delete_sl(&spy_list);

   printf(MSG_RMON_SHUTDOWNALLREACHABLESPYSQUIT);

   DEXIT;
   return 1;
}
