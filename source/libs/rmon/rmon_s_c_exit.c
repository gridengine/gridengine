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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_semaph.h"
#include "rmon_spy.h"

#include "rmon_spy_list.h"
#include "rmon_restart.h"
#include "rmon_s_c_exit.h"
#include "rmon_message_list.h"
#include "rmon_message_protocol.h"

extern u_long port;
extern u_long addr;
extern string programname;
extern monitoring_box_type *monitoring_box_down;
extern int semid_down;
extern int mlevel;
extern u_long message_counter;

/*****************************************************************/

int rmon_s_c_exit()
{
   int sfd, status, i;
   int n;
   message_list_type *temp;
   spy_list_type **slp;

#undef  FUNC
#define FUNC "rmon_s_c_exit"
   DENTER;

   strncpy(request.programname, programname, STRINGSIZE - 1);

   if (!(status = rmon_connect_rmond(&sfd, SPY_EXIT))) {
      if (errval != ECONNREFUSED) {
         DPRINTF(("Can't establish connection to rmond !\n"));
         DEXIT;
         return 0;
      }

      /* remove entry from restart file */
      if (!rmon_lock_restart_file()) {
         rmon_shut_down_on_error();
         rmon_errf(TRIVIAL, MSG_RMON_CANTLOCKRESTARTFILE);
      }

      if (!rmon_load_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTLOADRESTARTFILEX_S, sys_errlist[errno]);

      slp = rmon_search_spy(programname);
      if (!slp)
         rmon_errf(TRIVIAL, MSG_RMON_SPYALREADYDELETEDFROMSPYLIST);

      free(rmon_unchain_sl(slp));

      if (!rmon_save_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTSAVERESTARTFILEX_S, sys_errlist[errno]);

      if (!rmon_unlock_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTUNLOCKRESTARTFILE);

      rmon_delete_sl(&spy_list);
   }
   else {
      switch (status) {
      case S_ACCEPTED:
         break;

      case S_SPY_DOESNT_EXIST:
         rmon_errf(TRIVIAL, MSG_RMON_SPYALREADYDELETEDFROMSPYLIST);
         return 1;
         break;

      default:
         rmon_errf(TRIVIAL, MSG_RMON_CANTESTABLISCONNECTION);
         return 0;
      }

      if (!rmon_send_message_list(sfd, message_list, &n, addr, port)) {
         DPRINTF(("cannot send message list\n"));
         DEXIT;
         return 0;
      }

      /* wait for ack */
      if (!rmon_get_ack(sfd)) {
         DEXIT;
         return 0;
      }

      /* remove sent messages */
      for (i = 0; i < n; i++) {
         temp = message_list;
         message_list = message_list->next;
         free(temp);
      }
      message_counter -= n;

      shutdown(sfd, 2);
      close(sfd);
   }

   DEXIT;
   return 1;
}                               /* s_c_exit */
