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
#include "sge_hostname.h"
#include "sge_arch.h"
#include "cl_commlib.h"

void usage(void)
{
  fprintf(stderr, "%s gethostbyaddr x.x.x.x\n", MSG_UTILBIN_USAGE );
  exit(1);
}

#ifdef ENABLE_NGC
int main(int argc, char *argv[])
{
  struct hostent *he = NULL;
  char* resolved_name = NULL;
  int retval = CL_RETVAL_OK;
  char** tp = NULL;
  char** tp2 = NULL;
#if defined(CRAY)  
  struct sockaddr_in  addr;
#else
  struct in_addr addr;
#endif

  if (argc != 2) {
    usage();
  }

  retval = cl_com_setup_commlib(CL_NO_THREAD ,CL_LOG_OFF, NULL );
  if (retval != CL_RETVAL_OK) {
     fprintf(stderr,"%s\n",cl_get_error_text(retval));
     exit(1);
  }

  /* cl_com_set_alias_file(sge_get_alias_path()); */
  /* cl_com_append_host_alias("",""); */
  

#if defined(CRAY)
  addr.sin_addr.s_addr = inet_addr(argv[1]);
#else
  addr.s_addr = inet_addr(argv[1]);
#endif

  retval = cl_com_cached_gethostbyaddr(&addr, &resolved_name, &he);
  if (retval != CL_RETVAL_OK) {
     fprintf(stderr,"%s\n",cl_get_error_text(retval));
     cl_com_cleanup_commlib();
     exit(1);
  }

  if (he == NULL) {
     fprintf(stderr,"%s\n","could not get hostent struct");
  }

  if (he != NULL) {
     printf(MSG_SYSTEM_HOSTNAMEIS_S,he->h_name);
  }

  if (resolved_name != NULL) {
     printf("SGE name: %s\n",resolved_name);
     free(resolved_name);
  } else {
     printf("SGE name: %s\n","unexpected error");
  }

  if (he != NULL) {
     printf(MSG_SYSTEM_ALIASES );

     for (tp = he->h_aliases; *tp; tp++)
       printf("%s ", *tp);
     printf("\n");
  
     printf(MSG_SYSTEM_ADDRESSES );

     for (tp2 = he->h_addr_list; *tp2; tp2++)
        printf("%s ", inet_ntoa(* (struct in_addr *) *tp2));  /* inet_ntoa() is not MT save */
     printf("\n");    
  }
  sge_free_hostent(&he);

  retval = cl_com_cleanup_commlib();
  if (retval != CL_RETVAL_OK) {
     fprintf(stderr,"%s\n",cl_get_error_text(retval));
     exit(1);
  }
  return 0;
}
#else
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
     printf("%s ", inet_ntoa(* (struct in_addr *) *tp2));  /* inet_ntoa() is not MT save */
  printf("\n");    

  exit(0);
  return 0;
}
#endif
