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
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_request.h"

#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_job_list.h"

#include "rmon_spy_protocol.h"
#include "rmon_wait_protocol.h"
#include "rmon_client_protocol.h"
#include "rmon_transition_protocol.h"
#include "rmon_job_protocol.h"

#include "rmon_m_c_mstat.h"
#include "msg_rmon.h"
extern volatile int SFD;

/* ********************** intern prototypes ************ */

int rmon_m_c_mstat(
int sfd 
) {
   wait_list_type *wl;
   spy_list_type *sl;
   client_list_type *cl;
   transition_list_type *tl;
   job_list_type *jl;
   int n;

#undef  FUNC
#define FUNC "rmon_m_c_mstat"
   DENTER;

   for (n = 0, cl = client_list; cl; n++, cl = cl->next);
   request.n_client = n;
   for (n = 0, wl = wait_list; wl; n++, wl = wl->next);
   request.n_wait = n;
   for (n = 0, sl = spy_list; sl; n++, sl = sl->next);
   request.n_spy = n;
   for (n = 0, tl = first_client; tl; n++, tl = tl->next_client);
   request.n_transition = n;
   for (n = 0, jl = job_list; jl; n++, jl = jl->next);
   request.n_job = n;

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      rmon_errf(TRIVIAL, MSG_RMON_UNABLETOSENDACKNOWLEDGE);
      return 0;
   }

   /* ==================== */
   /* send list of clients */
   /* ==================== */

   if (!rmon_send_client_list(sfd)) {
      DPRINTF(("cannot send rmon list\n"));
      DEXIT;
      return 0;
   }

   /* ============================ */
   /* send list of waiting clients */
   /* ============================ */

   if (!rmon_send_wait_list(sfd)) {
      DPRINTF(("cannot send wait list\n"));
      DEXIT;
      return 0;
   }

   /* ================= */
   /* send list of spys */
   /* ================= */

   if (!rmon_send_spy_list(sfd)) {
      DPRINTF(("cannot send spy list\n"));
      DEXIT;
      return 0;
   }

   /* ======================== */
   /* send list of transitions */
   /* ======================== */

   if (!rmon_send_transition_list(sfd)) {
      DPRINTF(("cannot send transition list\n"));
      DEXIT;
      return 0;
   }

   /* ================= */
   /* send list of jobs */
   /* ================= */

   if (!rmon_send_job_list(sfd, job_list)) {
      DPRINTF(("cannot send global job list\n"));
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}
