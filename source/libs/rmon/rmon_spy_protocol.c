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
#define DEBUG

#include "rmon_h.h"
#include "rmon_io.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"

#include "rmon_convert.h"

#include "rmon_spy_list.h"
#include "rmon_spy_protocol.h"

#define SPY_SIZE            (7+N_LAYER)*ULONGSIZE+2*STRINGSIZE

extern volatile int SFD;

static spy_list_type *receive_sl(int sfd);
static int send_sl(int sfd, spy_list_type *sl);

/****************************************************************/

int rmon_receive_spy_list(
int sfd,
int n 
) {
   int i = 0;
   spy_list_type **slp;

#undef FUNC
#define FUNC "rmon_receive_spy_list"
   DENTER;

   slp = &spy_list;

   while (i++ < n) {
      if (!(*slp = receive_sl(sfd))) {
         DEXIT;
         return 0;
      }
      slp = &((*slp)->next);
   }

   DEXIT;
   return 1;
}                               /* rmon_receive_spy_list() */

/****************************************************************/

static spy_list_type *receive_sl(
int sfd 
) {
   u_long i;
   spy_list_type *new;
   char *tmp;

#undef FUNC
#define FUNC "receive_sl"
   DENTER;

   new = (spy_list_type *) malloc(sizeof(spy_list_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, SPY_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      free(new);
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, &new->uid);
   tmp = rmon_unconvertint(tmp, &new->last_flush);
   tmp = rmon_unconvertint(tmp, &new->childdeath);
   tmp = rmon_unconvertml(tmp, &new->moritz_level);
   tmp = rmon_unconvertint(tmp, &new->port);
   tmp = rmon_unconvertint(tmp, &new->inet_addr);
   tmp = rmon_unconvertstr(tmp, new->programname);
   tmp = rmon_unconvertint(tmp, &new->first);
   tmp = rmon_unconvertint(tmp, &new->last);

   new->next = NULL;

   DEXIT;

   return new;
}                               /* receive_sl */

/* ************************************************************** */

int rmon_send_spy_list(
int sfd 
) {
   spy_list_type *sl;

#undef FUNC
#define FUNC "rmon_send_spy_list"
   DENTER;

   /* send spy list elements */
   sl = spy_list;
   while (sl) {
      if (!send_sl(sfd, sl)) {
         DEXIT;
         return 0;
      }
      sl = sl->next;
   }

   DEXIT;
   return 1;
}                               /* rmon_send_spy_list() */

/****************************************************************/

static int send_sl(
int sfd,
spy_list_type *sl 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "send_sl"
   DENTER;

   /* convert the request in a net convertage */
   tmp = ptr;
   tmp = rmon_convertint(tmp, &sl->uid);
   tmp = rmon_convertint(tmp, &sl->last_flush);
   tmp = rmon_convertint(tmp, &sl->childdeath);
   tmp = rmon_convertml(tmp, &sl->moritz_level);
   tmp = rmon_convertint(tmp, &sl->port);
   tmp = rmon_convertint(tmp, &sl->inet_addr);
   tmp = rmon_convertstr(tmp, sl->programname);
   tmp = rmon_convertint(tmp, &sl->first);
   tmp = rmon_convertint(tmp, &sl->last);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, SPY_SIZE);
   alarm(0);
   SFD = 999;

   DEXIT;
   return i == 0;
}                               /* send_sl() */
