#ifndef __COMMPROC_H
#define __COMMPROC_H
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

#ifndef WIN32NATIVE
#	include <unistd.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#else /* WIN32NATIVE */
#	include <winsock2.h>
#endif /* WIN32NATIVE */



#include "commd.h"
#include "host.h"

/* If a commproc does not wait in a receive and doesn't anything else with 
   the commd. commd consideres him gone and cleans his internal structures. 
   5 minutes is a long time a process doesn't wont to communicate. If he 
   doesn't want to communicate he should do a leave() likes to. */
#define COMMPROC_TIMEOUT 60*5

/* Structure used to maintain a list of known communication processes.
   Also used for addressing a communication process */

typedef struct commproc {
  char name[MAXCOMPONENTLEN];
  host *host;         /* pointer to host entry this commproc lives on */
  u_short id;			 /* identifier of this commproc */
  int fd;             /* fd the commproc is using */
  int reserved_port;  /* true if message came from a reserved port */
  struct in_addr fromaddr;	/* address the message actualy came from */

  /* if commproc is waiting for a message the following describes the wait
     conditions */
  int w_fd;			 /* fd commproc is waiting on */
  char w_name[MAXCOMPONENTLEN];	/* commproc name we wait for */
  host *w_host;			/* pointer to hostentry we wait for */
  u_short w_id;			/* commprocid we are waiting for */
  int w_tag;			/* message tag to wait for */

  u_long lastaction;		/* for timeout of commproc enroll */
  int tag_priority_list[10];    /* receiver wants some tags first delivered */

  /* next commproc in list */
  struct commproc *next;
} commproc;

commproc *match_commproc(commproc *to);
commproc *add_commproc(int fd, int res_port, struct in_addr *addr);
commproc *search_commproc(char *h, char *name, u_short id);
commproc *search_commproc_waiting_on_fd(int fd);
commproc *search_commproc_using_fd(int fd);
void disconnect_commproc_using_fd(int fd);
int reconnect_commproc_with_fd(commproc *cop, int fd, char *str);
void delete_commproc(commproc *commp);
void delete_commproc_waiting_on_fd(int fd, char *name);
int delete_commproc_using_fd(int fd);
int setcommprocid(commproc *new, u_short startvalue);
int match2commprocs(commproc *a, commproc *b);
void print_commproc(commproc *commp, FILE *fp);
void print_commprocs(FILE *fp);
void commproc_touch(commproc *commp, u_long now);
void look4timeouts(u_long now);

#endif

