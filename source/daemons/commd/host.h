#ifndef __HOST_H
#define __HOST_H
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
#include <stdio.h>
#include <sys/types.h>

#ifndef WIN32NATIVE
#	include <sys/socket.h>
#	include <netdb.h>
#else 
#	include <winsock2.h>
#endif 



/* host information based on the hostent structure */

typedef struct host {
    struct hostent he;		/* copy of what we got from gethostbyname */
    char mainname[MAXHOSTLEN];  /* This is what the administrator think it is
                                   the mainname */
    int deleted;                /* if we can no longer resolve this host */
    struct host *alias;		/* chain aliases */
    struct host *next;
} host;

extern host *localhost;

int matches_name(struct hostent *he, char *name);
void host_initialize(void);
host *newhost_addr(const struct in_addr *addr);
host *newhost_name(char *name, int *not_really_new);
host *create_host(void);
void delete_host(host *h);
host *search_host(char *name, char *addr);
int alias_host(host *h1, host *h2);
int alias_hoststr(char *host1, char *host2);
void print_host(host *, FILE *fp);
void print_hostlist(FILE *fp);
int read_aliasfile(char *fname);
void refresh_hostlist(void);
char *get_mainname(host *h);


#endif /* __HOST_H */
