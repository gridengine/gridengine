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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include "basis_types.h"
#include "msg_utilbin.h"

void usage(void)
{
  fprintf(stderr, "%s gethostbyaddr x.x.x.x\n", MSG_UTILBIN_USAGE );
  exit(1);
}

int main(int argc, char *argv[])
{
  struct hostent *he;

#if defined(CRAY)  
  struct sockaddr_in  addr;
#else
  struct in_addr addr;
#endif
    
  char **tp,**tp2;


  if (argc != 2)
    usage();

#if defined(CRAY)
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  he = gethostbyaddr((char *)&(addr.sin_addr), sizeof(struct in_addr), AF_INET);
#else
  addr.s_addr = inet_addr(argv[1]);
  he = gethostbyaddr((char *) &addr, 4, AF_INET);
#endif

  if (!he) {
     perror(MSG_SYSTEM_GETHOSTBYADDRFAILED );
     exit(1);
  };

  printf(MSG_SYSTEM_HOSTNAMEIS_S,he->h_name);
  printf(MSG_SYSTEM_ALIASES );

  for (tp = he->h_aliases; *tp; tp++)
    printf("%s ", *tp);
  printf("\n");
  
  printf(MSG_SYSTEM_ADDRESSES );

  for (tp2 = he->h_addr_list; *tp2; tp2++)
     printf("%s ", inet_ntoa(* (struct in_addr *) *tp2));
  printf("\n");    

  exit(0);
  return 0;
}
