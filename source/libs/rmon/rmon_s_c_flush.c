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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "rmon_err.h"
#include "rmon_def.h"
#include "rmon_rmon.h"

#include "rmon_request.h"
#include "rmon_message_list.h"
#include "rmon_message_protocol.h"
#include "rmon_s_c_flush.h"

extern u_long message_counter;

extern u_long port;
extern u_long addr;

/*****************************************************************/

int rmon_s_c_flush(
int sfd 
) {
   message_list_type *temp;
   int i, n;

#undef  FUNC
#define FUNC "rmon_s_c_flush"
   DENTER;

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   if (!rmon_send_message_list(sfd, message_list, &n, addr, port)) {
      DPRINTF(("cannot send message list\n"));
      DEXIT;
      return 0;
   }

   /* wait for ack */
   if (!rmon_get_ack(sfd)) {
      DEXIT;
      return 0;
   }

   /* remove sent messages */
   for (i = 0; i < n; i++) {
      temp = message_list;
      message_list = message_list->next;
      free(temp);
   }
   message_counter -= n;

   DEXIT;
   return 1;

}                               /* s_c_flush */
