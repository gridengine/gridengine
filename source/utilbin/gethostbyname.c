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
#include <string.h>
#include <stdlib.h>
#include "basis_types.h"
#include "msg_utilbin.h"

#ifndef h_errno
extern int h_errno;
#endif

void usage(void)
{
  fprintf(stderr, "%s gethostbyname [-name] <name>\n",MSG_UTILBIN_USAGE);
  exit(1);
}

int main(int argc, char *argv[])
{
  struct hostent *he;
  char **tp,**tp2;
  int name_only = 0;
  

  if (argc < 2)
    usage();

  if (!strcmp(argv[1], "-name")) {
     if (argc != 3)
        usage(); 
     name_only = 1;
  }   
     
  he = gethostbyname(argv[1+name_only]);

  if (!he) {
    fprintf(stderr, "h_errno = %s\n", 
	(h_errno == HOST_NOT_FOUND)?"HOST_NOT_FOUND":
	(h_errno == TRY_AGAIN)?"TRY_AGAIN":
	(h_errno == NO_RECOVERY)?"NO_RECOVERY":
	(h_errno == NO_DATA)?"NO_DATA":
	(h_errno == NO_ADDRESS)?"NO_ADDRESS":"<unknown error>");
    perror(MSG_SYSTEM_GETHOSTBYADDRFAILED );
    exit(1);
  }

  if (name_only)
     printf("%s\n", he->h_name);
  else {
     printf(MSG_SYSTEM_HOSTNAMEIS_S , he->h_name);
     printf(MSG_SYSTEM_ALIASES );

     for (tp = he->h_aliases; *tp; tp++)
       printf("%s ", *tp);
     printf("\n");
  
     printf(MSG_SYSTEM_ADDRESSES );
     for (tp2 = he->h_addr_list; *tp2; tp2++)
        printf("%s ", inet_ntoa(* (struct in_addr *) *tp2));
     printf("\n");  
  }
  
  return 0;
}
