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

#include "rmon_err.h"
#include "rmon_def.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_s_c_spy_register.h"

#include "rmon_s_c_sleep.h"
#include "rmon_s_c_wake_up.h"

/*****************************************************************/

int rmon_s_c_wake_up(
int sfd,
int *state 
) {

#undef FUNC
#define FUNC "rmon_s_c_wake_up"

   DENTER;

   if (*state != STATE_SLEEPING) {
      rmon_errf(TRIVIAL, MSG_RMON_NOTSLEEPING);
      rmon_send_ack(sfd, S_NOT_SLEEPING);
      return 0;
   }

   rmon_send_ack(sfd, S_ACCEPTED);

   shutdown(sfd, 2);
   close(sfd);

   if (!rmon_s_c_spy_register()) {
      rmon_errf(TRIVIAL, MSG_RMON_CANTREGISTERTOAFTERWAKEUP);
      DPRINTF(("cannot register to after wake up.  \n"));
      DEXIT;
      return 0;
   }

   *state = STATE_AWAKE;
   DEXIT;
   return 1;
}                               /* rmon_s_c_wake_up() */
