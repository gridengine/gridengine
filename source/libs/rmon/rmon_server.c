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
#include "rmon_rmon.h"
#include "rmon_server.h"

extern int sfd0;
extern u_long port;

/***************************************************************/

int rmon_shutdown_server()
{
   shutdown(sfd0, 2);
   close(sfd0);
   return 1;
}

/***************************************************************/

int rmon_startup_server()
{
   struct sockaddr_in s_in;
   int on = 1;

   s_in.sin_family = AF_INET;
   s_in.sin_port = htons(port);
   s_in.sin_addr.s_addr = htonl(INADDR_ANY);

   if ((sfd0 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      rmon_errf(TERMINAL, MSG_RMON_SOCKETCREATIONERROR);

   if (setsockopt(sfd0, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
      rmon_errf(TERMINAL, MSG_RMON_SOCKETOPERATIONFAILUREX_S, sys_errlist[errno]);

   if (bind(sfd0, (struct sockaddr *) &s_in, sizeof(s_in)))
      if (errno == EADDRINUSE) {
         DPRINTF(("Bind error: Port %ld is already in use !\n", port));
         rmon_errf(TERMINAL, MSG_RMON_BINDERRORPORTXISALREADYINUSE_D, (u32c) port);
      }
      else {
         DPRINTF(("Bind error !\n"));
         rmon_errf(TERMINAL, MSG_RMON_BINDERROR);
      }

   if (listen(sfd0, 1))
      rmon_errf(TERMINAL, MSG_RMON_LISTENFAILURE_S, sys_errlist[errno]);

   return 1;
}

/***************************************************************/

int rmon_startup_spy_server(u_long * port)
{
   struct sockaddr_in s_in;
   int on = 1;

   if ((sfd0 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      rmon_errf(TERMINAL, MSG_RMON_SOCKETCREATIONERROR);

   if (setsockopt(sfd0, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
      rmon_errf(TERMINAL, MSG_RMON_SOCKETOPERATIONFAILUREX_S, sys_errlist[errno]);

   s_in.sin_family = AF_INET;
   s_in.sin_port = htons(*port);
   s_in.sin_addr.s_addr = htonl(INADDR_ANY);

   while (bind(sfd0, (struct sockaddr *) &s_in, sizeof(s_in)))
      if (errno == EADDRINUSE)
         s_in.sin_port = htons(++*port);
      else {
         DPRINTF(("Bind error !\n"));
         rmon_errf(TERMINAL, MSG_RMON_BINDERROR);
      }
   DPRINTF(("Got port #%ld.\n", *port));

   if (listen(sfd0, 1))
      rmon_errf(TERMINAL, MSG_RMON_LISTENFAILURE_S, sys_errlist[errno]);

   return 1;
}
