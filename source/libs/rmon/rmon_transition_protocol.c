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
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_convert.h"
#include "rmon_transition_list.h"
#include "rmon_transition_protocol.h"

#define TRANSITION_SIZE     (2+N_LAYER)*ULONGSIZE+STRINGSIZE

transition_type *transition_list = NULL;

extern volatile int SFD;

static int send_tl(int sfd, transition_list_type *tl);
static transition_type *receive_tl(int sfd);

/* ************************************************************** */

int rmon_receive_transition_list(
int sfd,
int n 
) {
   int i = 0;
   transition_type **tlp;

#undef FUNC
#define FUNC "rmon_receive_transition_list"
   DENTER;

   tlp = &transition_list;
   while (i++ < n) {
      if (!(*tlp = receive_tl(sfd))) {
         DEXIT;
         return 0;
      }
      tlp = &((*tlp)->next);
   }

   DEXIT;
   return 1;
}                               /* rmon_receive_transition_list() */

/* ************************************************************** */

static transition_type *receive_tl(
int sfd 
) {
   u_long i;
   transition_type *new;
   char *tmp;

#undef FUNC
#define FUNC "receive_tl"
   DENTER;

   new = (transition_type *) malloc(sizeof(transition_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, TRANSITION_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      free(new);
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, &new->last_read);
   tmp = rmon_unconvertml(tmp, &new->level);
   tmp = rmon_unconvertint(tmp, &new->client);
   tmp = rmon_unconvertstr(tmp, new->programname);

   new->next = NULL;

   DEXIT;
   return new;
}                               /* receive_tl() */

/* ************************************************************** */

int rmon_send_transition_list(
int sfd 
) {
   transition_list_type *tl;

#undef FUNC
#define FUNC "rmon_send_transition_list"
   DENTER;

   /* send transition list elements */
   tl = first_client;
   while (tl) {
      if (!send_tl(sfd, tl)) {
         DEXIT;
         return 0;
      }
      tl = tl->next_client;
   }

   DEXIT;
   return 1;
}                               /* rmon_send_transition_list() */

/* ************************************************************** */

static int send_tl(
int sfd,
transition_list_type *tl 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "send_tl"
   DENTER;

   /* convert the transition in a net convertage */
   tmp = ptr;
   tmp = rmon_convertint(tmp, &tl->last_read);
   tmp = rmon_convertml(tmp, &tl->level);
   tmp = rmon_convertint(tmp, &tl->client->client);
   tmp = rmon_convertstr(tmp, tl->spy->programname);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, TRANSITION_SIZE);
   alarm(0);
   SFD = 999;

   DEXIT;
   return (i == 0);
}                               /* send_tl() */
