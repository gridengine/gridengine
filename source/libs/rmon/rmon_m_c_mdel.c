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
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_m_c_mdel.h"
#include "rmon_rmond.h"

#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"

#include "rmon_m_c_monitoring_level.h"
#include "msg_rmon.h"
/*****************************************************************/

int rmon_m_c_mdel(
int sfd 
) {
   spy_list_type **slp = NULL, *sl = NULL;
   wait_list_type **wlp;
   client_list_type **clp = NULL;

   u_long c_number, who;
   u_long found = 0;

#undef  FUNC
#define FUNC "rmon_m_c_mdel"
   DENTER;

   who = request.kind;
   c_number = request.client_number;

   switch (who) {
   case SPY:

      /* search in spy list */
      if ((slp = rmon_search_spy(request.programname)) != NULL) {
         found = 1;
         sl = *slp;
         break;
      }

      for (wlp = &wait_list; *wlp;) {
         if (strcmp(request.programname, (*wlp)->programname) == 0)
            free(rmon_unchain_wl(wlp));
         else
            wlp = &((*wlp)->next);
      }

      break;

   case CLIENT:
      clp = rmon_search_client(c_number);
      if (clp)
         found = 1;
      break;

   default:
      printf(MSG_RMON_UNEXPECTEDREQUESTTYPE);
   }                            /* switch */

   if (!found) {
      if (!rmon_send_ack(sfd, S_NOT_ACCEPTED)) {
         DEXIT;
         return 0;
      }
      DEXIT;
      return 1;
   }

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   shutdown(sfd, 2);
   close(sfd);

   switch (who) {
   case SPY:
      rmon_mlclr(&request.level);       /* ??? */
      request.n_job = 0;

      if (!rmon_connect_anyone(&sfd, MQUIT, sl->inet_addr, sl->port))
         rmon_errf(TRIVIAL, MSG_RMON_CANTESTABLISCONNECTION);

      rmon_delete_spy(rmon_unchain_sl(slp));
      break;

   case CLIENT:
      /* First we must delete all wait-list-entries of this client */
      while ((wlp = rmon_search_wl_for_client(c_number)))
         free(rmon_unchain_wl(wlp));

      /* Then we can delete the client-list-entry:    */
      rmon_delete_client(rmon_unchain_cl(clp));

      /* .... and finally, finally the global job_list must be made new */

      if (rmon_make_job_list() == 1 || rmon_make_moritz_level() == 1)
         for (sl = spy_list; sl; sl = sl->next)
            rmon_send_monitoring_level_to_spy(sl);

   }                            /* switch */

   DEXIT;
   return 1;
}
