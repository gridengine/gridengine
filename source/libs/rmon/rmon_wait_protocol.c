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
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_conf.h"
#include "rmon_rmon.h"
#include "rmon_convert.h"
#include "rmon_wait_list.h"
#include "rmon_wait_protocol.h"

#define WAIT_SIZE           (1+N_LAYER)*ULONGSIZE+STRINGSIZE

extern volatile int SFD;

static wait_list_type *receive_wl(int sfd);
static int send_wl(int sfd, wait_list_type *wl);

/* ************************************************************** */

int rmon_receive_wait_list(
int sfd,
int n 
) {
   int i = 0;
   wait_list_type **wlp;

#undef FUNC
#define FUNC "rmon_receive_wait_list"
   DENTER;

   wlp = &wait_list;

   while (i++ < n) {
      if (!(*wlp = receive_wl(sfd))) {
         DEXIT;
         return 0;
      }
      wlp = &((*wlp)->next);
   }

   DEXIT;
   return 1;
}                               /* rmon_receive_wait_list() */

/****************************************************************/

static wait_list_type *receive_wl(
int sfd 
) {
   u_long i;
   wait_list_type *new;
   char *tmp;

#undef FUNC
#define FUNC "receive_wl"
   DENTER;

   new = (wait_list_type *) malloc(sizeof(wait_list_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, WAIT_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      free(new);
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertml(tmp, &new->level);
   tmp = rmon_unconvertstr(tmp, new->programname);
   tmp = rmon_unconvertint(tmp, &new->client);

   new->next = NULL;

   DEXIT;
   return new;
}                               /* receive_wl */

/* ************************************************************** */

int rmon_send_wait_list(
int sfd 
) {
   wait_list_type *wl;

#undef FUNC
#define FUNC "rmon_send_wait_list"
   DENTER;

   /* send wait list elements */
   wl = wait_list;
   while (wl) {
      if (!send_wl(sfd, wl)) {
         DEXIT;
         return 0;
      }
      wl = wl->next;
   }

   DEXIT;
   return 1;
}                               /* rmon_send_wait_list() */

/****************************************************************/

static int send_wl(
int sfd,
wait_list_type *wl 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "send_wl"
   DENTER;

   /* convert the wait in a net convertage */
   tmp = ptr;
   tmp = rmon_convertml(tmp, &wl->level);
   tmp = rmon_convertstr(tmp, wl->programname);
   tmp = rmon_convertint(tmp, &wl->client);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, WAIT_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return 0;
   }

   DEXIT;
   return (i == 0);
}                               /* send_wl() */
