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
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_err.h"
#include "rmon_io.h"

extern int rresvport(int *);
extern volatile int SFD;

/********************************/
/* if port==0 we use service    */
/* else we use port number              */

int rmon_open_tcp_by_name(
char *host,
u_long port,
char *service,
int reserved_port 
) {
   static struct sockaddr_in addr;
   struct hostent *he;
   struct servent *se;
   int i, sfd;
   static char oldname[STRINGSIZE] = "";

#undef FUNC
#define FUNC "rmon_open_tcp_by_name"

   /* NOTE: DON'T ATTEMPT TO LOG ANY ERRORS IN THIS PACKAGE !! */
   /* ATTEMPTING TO DO SO COULD LEAD TO AN INFINITE(TILL PROG CORE DUMPS) LOOP */

   if (strcmp(host, oldname) != 0) {

      if (!(he = gethostbyname(host))) {
         DPRINTF(("unable to resolve host\n"));
         DPRINTF(("host: %s\n", host));
         DPRINTF(("oldname: %s\n", oldname));
         return -1;
      }

      bzero((char *) &addr, sizeof(addr));
      addr.sin_family = AF_INET;
      bcopy((char *) he->h_addr, (char *) &addr.sin_addr, he->h_length);
      strcpy(oldname, host);
   }

   if (port) {
      /* DPRINTF(("using port %d\n",port)); */
      addr.sin_port = htons(port);
   }
   else {                       /* lookup service */
      DPRINTF(("using service %s\n", service));

      if ((se = getservbyname(service, "tcp")) == NULL) {
         DPRINTF(("getservbyname() failed\n"));
         return -1;
      }
      addr.sin_port = se->s_port;
   }

   if (reserved_port) {

      DPRINTF(("reserved port\n"));
      i = IPPORT_RESERVED - 1;

      if ((sfd = rresvport(&i)) < 0) {
         DPRINTF(("rresvport() failed\n"));
         return -1;
      }
   }
   else {
      if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         DPRINTF(("unable to create socket\n"));
         return -1;
      }
   }

   SFD = sfd;
   alarm(ALARMS);
   i = connect(sfd, (struct sockaddr *) &addr, sizeof(addr));
   alarm(0);
   SFD = 999;

   if (i < 0) {
      switch (errno) {
      case ECONNREFUSED:

         DPRINTF(("connect() failed [%d]: %s\n", errno, sys_errlist[errno]));
         close(sfd);
         return -1;

      case EINTR:
         DPRINTF(("Connect: %s\n", sys_errlist[errno]));
         return -2;

      default:
         DPRINTF(("errno = %s\n", sys_errlist[errno]));
      }
   }
   return sfd;
}                               /* open_tcp_by_name */

/* ************************************************************************** */

int rmon_open_tcp_by_addr(
u_long addr,
u_long port,
int reserved_port 
) {
   struct sockaddr_in serv_addr;
   int i, sfd;

#undef FUNC
#define FUNC "rmon_open_tcp_by_addr"

   /* NOTE: DON'T ATTEMPT TO LOG ANY ERRORS IN THIS PACKAGE !! */
   /* ATTEMPTING TO DO SO COULD LEAD TO AN INFINITE(TILL PROG CORE DUMPS) LOOP */

   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(addr);
   serv_addr.sin_port = htons(port);

   if (reserved_port) {

      DPRINTF(("reserved port\n"));
      i = IPPORT_RESERVED - 1;

      if ((sfd = rresvport(&i)) < 0) {
         DPRINTF(("rresvport() failed\n"));
         return -1;
      }
   }
   else {

      if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         DPRINTF(("unable to create socket\n"));
         return -1;
      }
   }

   SFD = sfd;
   alarm(ALARMS);
   i = connect(sfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   alarm(0);
   SFD = 999;

   if (i < 0) {
      switch (errno) {
      case ECONNREFUSED:
         DPRINTF(("connect() failed [%d]: %s\n", errno, sys_errlist[errno]));
         close(sfd);
         return -1;

      case EINTR:
         DPRINTF(("Connect: %s\n", sys_errlist[errno]));
         close(sfd);
         return -2;

      default:
         DPRINTF(("errno = %s\n", sys_errlist[errno]));
      }
   }
   return sfd;
}                               /* open_tcp_by_addr */

/***************************************************************/

int rmon_writenbytes(
register int sfd,
register char *ptr,
register unsigned int n 
) {
   int i;
   unsigned int nleft;
#undef FUNC
#define FUNC "rmon_writenbytes"

/* DENTER; */
   nleft = n;

   while (nleft > 0) {
      i = write(sfd, ptr, nleft);
      /* DPRINTF(("   wrote %d bytes on fd %d\n",i,sfd)); */
      if (i <= 0) {
         /* DPRINTF(("   returning %d\n",i)); */
         /* DEXIT; */
         return i;
      }
      nleft -= i;
      ptr += i;
   }
/* DEXIT; */
   return 0;
}

/***************************************************************/

int rmon_readnbytes(
register int sfd,
register char *ptr,
register unsigned int n 
) {
   int j;
   unsigned int i;

#undef FUNC
#define FUNC "rmon_readnbytes"

   /*DENTER; */
   i = n;
   while (i > 0) {
      j = read(sfd, ptr, i);
      /*DPRINTF(("   read %d bytes on fd %d\n",j,sfd)); */
      /*DPRINTF(("=*=*=*=*=*=> %s\n", ptr)); */

      if (j < 0) {
         /*DPRINTF(("   returning %d\n",j)); */
         /*DEXIT; */
         return (j);
      }
      else if (j == 0)
         break;
      i -= j;
      ptr += j;
   }
   /*DPRINTF(("   returning %d\n",i)); */
   /*DEXIT; */
   return (i);
}

int rmon_hostcmp(
char *host0,
char *host1 
) {
   struct hostent *he;
   u_long addr;

   struct in_addr b0, b1;

#undef FUNC
#define FUNC "rmon_hostcmp"

   DENTER;

   /* DPRINTF(("h0: %s h1: %s\n", host0, host1 )); */

   /* try direct compare of names */
   if (strcmp(host0, host1) == 0) {
      DEXIT;
      return 0;
   }

   /* resolved adresses */
   if (!(he = gethostbyname(host0))) {
      DEXIT;
      return 1;
   }
   bcopy((char *) he->h_addr, (char *) &addr, he->h_length);
   b0.s_addr = addr;

   if (!(he = gethostbyname(host1))) {
      DEXIT;
      return 1;
   }
   bcopy((char *) he->h_addr, (char *) &addr, he->h_length);
   b1.s_addr = addr;

/*      DPRINTF(("h0: %s\n", inet_ntoa(b0) )); */
/*      DPRINTF(("h1: %s\n", inet_ntoa(b1) )); */

   DEXIT;

   /* compare resolved adresses */
   return bcmp((char *) &b0.s_addr, (char *) &b1.s_addr, he->h_length);
}
