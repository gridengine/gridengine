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
/***************************************************************
 Messages in state of delivering for this commproc are reset.
 They may be delivered to this commproc later.
 ***************************************************************/

#include <unistd.h>
#include <sys/socket.h>

#include "message.h"
#include "commproc.h"
#include "rwfd.h"

extern message *message_list;

void reset_messages_for_commproc(commproc *commp);

void reset_messages_for_commproc(
commproc *commp 
) {
   message *mp = message_list;

   while (mp) {
      if (mp->sending_to == commp) {
         mp->sending_to = NULL;
         if (mp->tofd != -1) {
            fd_close(mp->tofd, "to reset commproc messages");
            mp->tofd = -1;
         }
         SET_MESSAGE_STATUS(mp, S_RDY_4_SND);
      }
      mp = mp->next;
   }
}
