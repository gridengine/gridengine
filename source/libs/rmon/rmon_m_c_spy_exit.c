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
#include "rmon_rmond.h"
#include "rmon_request.h"

#include "rmon_spy_list.h"
#include "rmon_message_list.h"
#include "rmon_message_protocol.h"

#include "rmon_m_c_spy_exit.h"

/*****************************************************************/

int rmon_m_c_spy_exit(
int sfd 
) {
   spy_list_type **slp;
   message_list_type **mlp;
   int i;

#undef  FUNC
#define FUNC "rmon_m_c_spy_exit"
   DENTER;

   /* 1. First we check wether a spy with this addr and port is still      */
   /*    registered.                                                                       */

   if (!(slp = rmon_search_spy(request.programname))) {
      DPRINTF(("spy is not registered !\n"));
      rmon_send_ack(sfd, S_SPY_DOESNT_EXIST);
      DEXIT;
      return 0;
   }

   if (!(i = rmon_send_ack(sfd, S_ACCEPTED))) {
      DEXIT;
      return 0;
   }

   /* 2. Then we receive the last messages from this spy.  */

   mlp = &(*slp)->message_list;
   while (*mlp)
      mlp = &((*mlp)->next);

   if (!rmon_receive_message_list(sfd, mlp))
      return 0;

   (*slp)->last += eol.n;

   DPRINTF(("gelesen: %d\n", eol.n));

   /* 3. After this we must ACK to the spy for the received messages.      */

   if (!rmon_send_ack(sfd, S_ACCEPTED))
      return 0;

   /* (*slp)->last_flush = now++; */

   /* 4. At last we can delete the spy_list_entry if the message_list is empty,    */
   /*        or we set the childdeath-flag to show that the spy can be deleted when        */
   /*        the last client has read it's messages.                                                                       */

   if (!(*slp)->message_list)
      rmon_xchg_tl_with_wait_by_sl(rmon_unchain_sl(slp));
   else
      (*slp)->childdeath = 1;

   DEXIT;
   return 1;
}
