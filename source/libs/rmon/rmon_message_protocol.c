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
#include "rmon_message_list.h"
#include "rmon_message_protocol.h"
#include "msg_rmon.h"
#define MESSAGE_SIZE (N_LAYER + 1 + 5)*ULONGSIZE+3*STRINGSIZE

eol_type eol;
extern volatile int SFD;

/* ************************************************************** */

int rmon_receive_message_list(
int sfd,
message_list_type **mlp 
) {
   u_long status;
   u_long n = 0;
   message_list_type *neu;

#undef FUNC
#define FUNC "rmon_receive_message_list"

   DENTER;

   do {
      neu = rmon_receive_message(sfd, &status);
      switch (status) {
      case MESSAGE:
         *mlp = neu;
         mlp = &(neu->next);
         n++;
         break;

      case ERROR:

         DPRINTF(("Daten nicht gelesen !\n"));
         shutdown(sfd, 2);
         close(sfd);

         DEXIT;
         return 0;
      }

   } while (status != EOL);

   if (eol.n != n) {
      DPRINTF(("eol.n != n\n"));
      DPRINTF(("n     = %d\n", n));
      DPRINTF(("eol.n = %d\n", eol.n));

      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}

/* ************************************************************** */

int rmon_send_message_list(
int sfd,
message_list_type *ml,
int *count,
u_long addr,
u_long port 
) {
   int n = 0;

#undef FUNC
#define FUNC "rmon_send_message_list"
   DENTER;

   while (ml) {
      if (!rmon_send_message(sfd, ml)) {
         DEXIT;
         return 0;
      }
      ml = ml->next;
      n++;
   }

   /* add # of messages to  eol */
   eol.n = *count = n;
   eol.port = port;
   eol.inet_addr = addr;

   if (!rmon_send_eol(sfd)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}

/* ************************************************************** */

int rmon_send_eol(
int sfd 
) {
   char *tmp;
   int i;
   u_long status = EOL;

#undef FUNC
#define FUNC "rmon_send_eol"
   DENTER;

   /* first we convert the request in a net package */
   tmp = ptr;
   tmp = rmon_convertint(tmp, &status);
   tmp = rmon_convertint(tmp, &eol.n);
   tmp = rmon_convertint(tmp, &eol.port);
   tmp = rmon_convertint(tmp, &eol.inet_addr);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, MESSAGE_SIZE);
   alarm(0);
   SFD = 999;

   if (i)
      DPRINTF(("   returning %d\n", i));

   DEXIT;
   return (i == 0);

}                               /* send_eol */

/* ************************************************************** */

int rmon_send_message(
int sfd,
message_list_type *ml 
) {
   int i;
   u_long status = MESSAGE;
   char *tmp;

#undef FUNC
#define FUNC "rmon_send_message"
   DENTER;

   tmp = ptr;
   tmp = rmon_convertint(tmp, &status);
   tmp = rmon_convertint(tmp, &ml->inet_addr);
   tmp = rmon_convertint(tmp, &ml->port);
   tmp = rmon_convertint(tmp, &ml->pid);
   tmp = rmon_convertint(tmp, &ml->jobid);
   tmp = rmon_convertml(tmp, &ml->level);
   tmp = rmon_convertlstr(tmp, ml->data);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, MESSAGE_SIZE);
   alarm(0);
   SFD = 999;

   if (i)
      DPRINTF(("   returning %d\n", i));

   DEXIT;
   return (i == 0);
}                               /* send_message */

/* ************************************************************** */

message_list_type *rmon_receive_message(
int sfd,
u_long *status 
) {
   message_list_type *ml;
   int i;
   char *tmp;

#undef FUNC
#define FUNC "rmon_receive_message"
   DENTER;

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, MESSAGE_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      DPRINTF(("   returning %d\n", i));
      *status = ERROR;
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, status);

   switch (*status) {
   case MESSAGE:
      ml = (message_list_type *) malloc(sizeof(message_list_type));
      if (!ml)
         rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

      tmp = rmon_unconvertint(tmp, &ml->inet_addr);
      tmp = rmon_unconvertint(tmp, &ml->port);
      tmp = rmon_unconvertint(tmp, &ml->pid);
      tmp = rmon_unconvertint(tmp, &ml->jobid);
      tmp = rmon_unconvertml(tmp, &ml->level);
      tmp = rmon_unconvertlstr(tmp, ml->data);
      ml->next = NULL;

      DEXIT;
      return ml;

   case EOL:

      tmp = rmon_unconvertint(tmp, &eol.n);
      tmp = rmon_unconvertint(tmp, &eol.port);
      tmp = rmon_unconvertint(tmp, &eol.inet_addr);

      DEXIT;
      return NULL;

   default:
      *status = ERROR;
      rmon_errf(TRIVIAL, MSG_RMON_UNEXPECTEDSTATUSVALUE);
      DEXIT;
      return NULL;
   }

}                               /* receive_message */
