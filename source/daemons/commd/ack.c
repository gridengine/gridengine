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
#include <errno.h>
#include <string.h>

#include "message.h"
#include "commd_error.h"
#include "sgermon.h"
#include "sge_log.h"
#include "msg_commd.h"

int write_ack(message *mp, int tofd);
int read_ack(message *mp);

void reset_message(message *mp, const char *str);

/************************************************************
  write a (N)Ack to the sender(receiver if tofd==1) of the message
  return:
     -1 error
      1  try again
      0  OK-> message deleted if del_message=1
 ************************************************************/
int write_ack(message *mp, int tofd) {
   int i;

   DENTER(TOP_LAYER, "write_ack");

   DEBUG((SGE_EVENT, "send ack: %d", mp->ackchar));

   i = write(tofd ? mp->tofd : mp->fromfd, &(mp->ackchar), 1);

   if (i == 0) {
      DEXIT;
      return -1;
   }
   else if (i == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
         DEXIT;
         return 1;              /* sender isn't ready -> try later */
      }
      else {                    /* a real error -> cancel message */
         DPRINTF(("write failed sending ack\n"));
         DEXIT;
         return -1;
      }
   }
   DEXIT;
   return 0;
}


/************************************************************
  read a (N)Ack from the receiver of the message
  return:
     -1 error-> try to send message later may be to another receiver 
      1  try again
      0  OK
 ************************************************************/
int read_ack(message *mp) {
   int i;
   char err_str[512];
   
   DENTER(TOP_LAYER, "read_ack");
   
   i = read(mp->tofd, &(mp->ackchar), 1);

   if (i == 0) {
      disconnect_commproc_using_fd(mp->tofd);
      reset_message(mp, MSG_RESETMESSAGE_READ_POS_0);
      DEXIT;
      return -1;
   }
   else if (i == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
         DEXIT;
         return 1;              /* sender isn't ready -> try later */
      }
      else {                    /* a real error -> try the whole thing later */
         sprintf(err_str, MSG_RESETMESSAGE_READ_POS_0_S, strerror(errno));
         reset_message(mp, err_str);
         DEXIT;
         return -1;
      }
   }

   DEBUG((SGE_EVENT, MSG_RESETMESSAGE_READ_I , (int) mp->ackchar));
   DEXIT;
   return 0;
}
