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
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_connect.h"
#include "rmon_request.h"
#include "msg_rmon.h"


extern volatile int SFD;
extern int sfd0;
extern char rmond[];

int name_is_valid = 0;

u_long errval;

/***************************************************************/

char *rmon_get_host_name(
u_long i_addr 
) {
   struct hostent *he;
   u_long addr;

#undef  FUNC
#define FUNC "rmon_get_host_name"
   DENTER;

   addr = htonl(i_addr);
   he = gethostbyaddr((char *) &addr, sizeof(i_addr), AF_INET);

   DEXIT;
   return (char *) he->h_name;
}

/***************************************************************/

int rmon_connect_rmond(
int *sfd,
u_long type 
) {
   int i;
   struct servent *sp;
   static u_long rmond_port;
   static int port_is_valid = 0;

#undef FUNC
#define FUNC "rmon_connect_rmond"
   DENTER;

   if (!name_is_valid) {
      rmon_read_configuration();
      DPRINTF(("read configuration (%s)\n", rmond));
      name_is_valid = 1;
   }

   if (!port_is_valid) {
      if (!(sp = getservbyname(RMOND_SERVICE, "tcp")))
         rmon_errf(TERMINAL, MSG_RMON_XBADSERVICE_S , RMOND_SERVICE);

      rmond_port = ntohs(sp->s_port) + 5000;
      DPRINTF(("RMOND_PORT: %d\n", rmond_port));
   }

   if ((*sfd = rmon_open_tcp_by_name(rmond, rmond_port, 0, 0)) < 0) {
      switch (*sfd) {
      case -1:
         errval = ECONNREFUSED;
         break;

      case -2:
         errval = EINTR;
         break;
      }
      DEXIT;
      return 0;
   }

   request.type = type;
   if (!rmon_send_request(*sfd)) {
      DPRINTF(("ERROR: unable to contact rmond\n"));
      shutdown(*sfd, 2);
      close(*sfd);
      *sfd = -1;
      return 0;
   }

   if (!(i = rmon_get_ack(*sfd))) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return i;
}

/***************************************************************/

int rmon_connect_anyone(
int *sfd,
u_long type,
u_long i_addr,
u_long i_port 
) {
   int status;

#undef  FUNC
#define FUNC "rmon_connect_anyone"
   DENTER;

   if ((*sfd = rmon_open_tcp_by_addr(i_addr, i_port, 0)) < 0) {
      switch (*sfd) {

      case -1:
         errval = ECONNREFUSED;
         break;

      case -2:
         errval = EINTR;
         break;
      }
      DEXIT;
      return 0;
   }

   request.type = type;

   if (!rmon_send_request(*sfd)) {
      DPRINTF(("Fehler bei send_request\n"));
      shutdown(*sfd, 2);
      close(*sfd);
      *sfd = -1;
      DEXIT;
      return 0;
   }

   if (!(status = rmon_get_ack(*sfd))) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return status;
}
