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
#include "rmon_io.h"
#include "rmon_err.h"
#include "rmon_conf.h"
#include "rmon_rmon.h"

#include "rmon_convert.h"

#include "rmon_client_list.h"
#include "rmon_client_protocol.h"
#include "rmon_job_list.h"
#include "rmon_job_protocol.h"
#include "msg_rmon.h"
/* one u_long is used for the number of job list entries */
#define CLIENT_SIZE         (1 + 4)*ULONGSIZE

extern volatile int SFD;

static client_list_type *receive_cl(int sfd);
static int send_cl(int sfd, client_list_type *cl);

/*****************************************************************/
int rmon_receive_client_list(
int sfd,
int n 
) {
   int i = 0;
   client_list_type **clp;

#undef FUNC
#define FUNC "rmon_receive_client_list"
   DENTER;

   clp = &client_list;

   while (i++ < n) {
      if (!(*clp = receive_cl(sfd))) {
         DEXIT;
         return 0;
      }
      clp = &((*clp)->next);
   }

   DEXIT;
   return 1;
}                               /* rmon_receive_client_list() */

/*****************************************************************/
static client_list_type *receive_cl(
int sfd 
) {
   u_long i, n;
   client_list_type *new;
   char *tmp;

#undef FUNC
#define FUNC "receive_cl"

   DENTER;

   new = (client_list_type *) malloc(sizeof(client_list_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, CLIENT_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      free(new);
      DPRINTF((" returning %d\n", i));
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, &new->all_jobs);
   tmp = rmon_unconvertint(tmp, &new->uid);
   tmp = rmon_unconvertint(tmp, &new->client);
   tmp = rmon_unconvertint(tmp, &new->last_flush);
   tmp = rmon_unconvertint(tmp, &n);    /* length of job list */

   new->job_list = NULL;
   if (n > 0)
      if (!rmon_receive_job_list(sfd, &(new->job_list), (int) n)) {
         free(new);
         DEXIT;
         return NULL;
      }

   new->next = NULL;

   DEXIT;
   return new;
}                               /* receive_cl */

/*****************************************************************/

int rmon_send_client_list(
int sfd 
) {
   client_list_type *cl;

#undef FUNC
#define FUNC "rmon_send_client_list"

   DENTER;

   /* send client list elements */
   cl = client_list;
   while (cl) {
      if (!send_cl(sfd, cl)) {
         DEXIT;
         return 0;
      }
      cl = cl->next;
   }

   DEXIT;
   return 1;
}                               /* rmon_send_client_list() */

/*****************************************************************/

static int send_cl(
int sfd,
client_list_type *cl 
) {
   int i;
   u_long n;
   char *tmp;
   job_list_type *jl;

#undef  FUNC
#define FUNC "send_cl"
   DENTER;

   /* count # of job list elements */
   for (n = 0, jl = cl->job_list; jl; n++, jl = jl->next);

   /* convert the client in a net convertage */
   tmp = ptr;
   tmp = rmon_convertint(tmp, &cl->all_jobs);
   tmp = rmon_convertint(tmp, &cl->uid);
   tmp = rmon_convertint(tmp, &cl->client);
   tmp = rmon_convertint(tmp, &cl->last_flush);
   tmp = rmon_convertint(tmp, &n);      /* length of job list */

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, CLIENT_SIZE);
   alarm(0);
   SFD = 999;

   if (i != 0) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return rmon_send_job_list(sfd, cl->job_list);
}                               /* send_cl() */
