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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "basis_types.h"
#include "msg_utilbin.h"
#include "sge_string.h"
#include "host.h"
#include "sge_arch.h"

#if defined(SOLARIS)
int gethostname(char *, int);
#endif

int usage(void)
{
  fprintf(stderr, "%s\n gethostname [-name|-aname]\n\n%s", MSG_UTILBIN_USAGE, MSG_COMMAND_USAGE_GETHOSTNAME );
  exit(1);
  return 0;
}

int main(int argc,char *argv[])
{
  struct hostent *he;
  char **tp,**tp2;
  char buf[128];
  int name_only = 0;
  int sge_aliasing = 0;
  

  if (argc == 2) {
     if (!strcmp(argv[1], "-name"))
        name_only = 1;
     else if (!strcmp(argv[1], "-aname")) {
        name_only = 1;
        sge_aliasing = 1;
     } else
        usage();
  }
  else if (argc > 2)
     usage();
      
  if (gethostname(buf, sizeof(buf))) {
     perror(MSG_SYSTEM_GETHOSTNAMEFAILED );
     exit(1);
  }
  
  he = gethostbyname(buf);

  if (!he) {
    perror(MSG_SYSTEM_GETHOSTBYNAMEFAILED );
    exit(1);
  }
  strcpy(buf, he->h_name);

  if (name_only) {
     const char *s;
     if (sge_aliasing && (s=resolve_hostname_local(buf)))
        printf("%s\n", s);
     else /* no aliased name */
        printf("%s\n", buf);
  } else {    
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
