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

#include "rmon_c_c_monitoring_level.h"
#include "msg_rmon.h"


extern volatile int SFD;
extern string programname;
extern monitoring_level level;
extern u_long spy_wait;
extern u_long mynumber;

/*****************************************************************/

int rmon_c_c_monitoring_level()
{
   int sfd, status, i = 0;

#undef FUNC
#define FUNC "rmon_c_c_monitoring_level"
   DENTER;

   strncpy(request.programname, programname, STRINGSIZE - 1);
   rmon_mlcpy(&request.level, &level);
   request.client_number = mynumber;
   request.wait = spy_wait;     /* wait on spy */

   while (!(status = rmon_connect_rmond(&sfd, MONITORING_LEVEL)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }

   shutdown(sfd, 2);
   close(sfd);

   switch (status) {
   case S_UNKNOWN_SPY:
      printf(MSG_RMON_UNKNOWNSPY);
      DEXIT;
      exit(0);

   case S_UNKNOWN_CLIENT:
      printf(MSG_RMON_UNKNOWNRMON);
      DEXIT;
      exit(0);

   case S_NOT_ACCEPTED:
      printf(MSG_RMON_VERYHEAVYERROR);
      i = 0;
      break;

   case S_ILLEGAL_LEVEL:
      printf(MSG_RMON_ILLEGALMONITORINGLEVEL);
      i = 0;
      break;

   case S_ACCEPTED:
      i = 1;
      break;

   case S_NEW_TRANSITION:
      printf(MSG_RMON_NEWTRANSITIONLISTELEMENT);
      i = 1;
      break;

   case S_DELETE_TRANSITION:
      printf(MSG_RMON_REMOVEDTRANSITIONLISTELEMENT);
      i = 1;
      break;

   case S_ALTER_TRANSITION:
      printf(MSG_RMON_ALTEREDTRANSITIONLISTELEMENT);
      i = 1;
      break;

   case S_NEW_WAIT:
      printf(MSG_RMON_NEWWAITLISTELEMENT);
      i = 1;
      break;

   case S_DELETE_WAIT:
      printf(MSG_RMON_REMOVEDWAITLISTELEMENT);
      i = 1;
      break;

   case S_ALTER_WAIT:
      printf(MSG_RMON_ALTEREDWAITINGLISTELEMENT );
      i = 1;
      break;

   default:
      rmon_errf(TERMINAL, MSG_RMON_UNEXPECTEDREQUESTSTATUSX_D, request.status);
   }

   DEXIT;
   return i;
}                               /* c_c_monitoring_level  */
