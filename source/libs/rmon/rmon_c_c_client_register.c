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

#include "rmon_c_c_client_register.h"
#include "msg_rmon.h"
extern u_long mynumber;

/*****************************************************************/

int rmon_c_c_client_register()
{
   int sfd, i;

#undef  FUNC
#define FUNC "rmon_c_c_client_register"
   DENTER;

   request.uid = (u_long) getuid();

   while (!(i = rmon_connect_rmond(&sfd, CLIENT_REGISTER)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE );
         rmon_errf(TERMINAL, MSG_RMON_NORMONDAVAILABLE );
      }

   switch (i) {
   case S_NOT_ACCEPTED:
      return 0;

   case S_ACCEPTED:
      mynumber = request.client_number;
      break;

   default:
      printf(MSG_RMON_UNEXPECTEDREQUESTSTATUSX_D , (u32c) request.status);
      DEXIT;
      exit(0);
   }

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }
   shutdown(sfd, 2);
   close(sfd);

   DEXIT;
   return 1;
}                               /* c_c_client_register */
