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
#include "rmon_io.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_connect.h"

#include "rmon_spy_protocol.h"
#include "rmon_job_protocol.h"
#include "rmon_wait_protocol.h"
#include "rmon_client_protocol.h"
#include "rmon_transition_protocol.h"

#include "rmon_get_stat.h"
#include "msg_rmon.h"
/********** global variables *********************************/

extern volatile int SFD;        /* THIS PUPPY IS CLOSED ON SIGALRM */

/***************************************************************************/

int rmon_get_stat()
{
   int sfd, status = 0;

#undef FUNC
#define FUNC "rmon_get_stat"
   DENTER;

   while (!(status = rmon_connect_rmond(&sfd, MSTAT)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }

   if (status != S_ACCEPTED) {
      printf(MSG_RMON_GOTNOACK );
      DEXIT;
      exit(0);
   }

   /* ============================= */
   /* get client list from rmond */
   /* ============================= */
   if (!rmon_receive_client_list(sfd, request.n_client)) {
      DPRINTF(("cannot receive rmon list\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("rmon list received\n"));

   /* =========================== */
   /* get wait list from rmond */
   /* =========================== */

   if (!rmon_receive_wait_list(sfd, request.n_wait)) {
      DPRINTF(("cannot receive wait list\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("wait list received\n"));

   /* ========================== */
   /* get spy list from rmond */
   /* ========================== */
   if (!rmon_receive_spy_list(sfd, request.n_spy)) {
      DPRINTF(("cannot receive spy list\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("spy list received\n"));

   /* ================================= */
   /* get transition list from rmond */
   /* ================================= */
   if (!rmon_receive_transition_list(sfd, request.n_transition)) {
      DPRINTF(("cannot receive transition list\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("transition list received\n"));

   /* ================================= */
   /* get global job list from rmond */
   /* ================================= */
   if (!rmon_receive_job_list(sfd, &job_list, request.n_job)) {
      DPRINTF(("cannot receive global job list\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("global job list received\n"));

   close(sfd);
   shutdown(sfd, 2);

   DEXIT;
   return 1;
}
