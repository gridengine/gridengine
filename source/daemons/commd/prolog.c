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
#include <sys/types.h>
#include <errno.h>

#include "message.h"
#include "commd_error.h"
#include "commd_io.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_crc.h"
#include "rwfd.h"
#include "msg_commd.h"

int read_message_prolog(message *mp);
int write_message_prolog(message *mp);
extern void reset_message(message *mp, const char *str);
extern void reset_messages_for_commproc(commproc *commp);

/**********************************************************************
  read message prolog 

  the prolog contains:
    flags      4 bytes
    headerlen  2 bytes
    bufferlen  4 bytes
    checksum   4 bytes

  return 0  if everything is read
         -1 on error
         1  if sender isn't ready

  if an error occurs the message will be dropped
 **********************************************************************/
int read_message_prolog(
message *mp 
) {
   u_long already_read;
   int i, errorcode;
   unsigned char *cp;
   u_long32 crc32 = 0;

   DENTER(TOP_LAYER, "read_message_prolog");
   
   already_read = mp->bufprogress - mp->prolog;

   DEBUG((SGE_EVENT, "read prolog: already read=%ld", already_read));

   while (already_read < PROLOGLEN) {
      i = read(mp->fromfd, mp->bufprogress, PROLOGLEN - already_read);
      errorcode = errno;        /* save */

      DEBUG((SGE_EVENT, "read returned %d", i));

      if (!i) {
         disconnect_commproc_using_fd(mp->fromfd);
         delete_message(mp, NULL);
         DEXIT;
         return -1;
      }

      if (i == -1) {
         if (errorcode == EWOULDBLOCK || errno == EAGAIN) {
            DEXIT;
            return 1;           /* sender isn't ready -> try later */
         }   
         else {                 /* a real error -> cancel message */
            delete_message(mp, "cancel message");
            DEXIT;
            return -1;
         }
      }
      /* we got some data -> process it */
      mp->bufprogress += i;
      already_read += i;
   }

   cp = unpack_ulong(&mp->flags, mp->prolog);
   cp = unpack_ushort(&mp->headerlen, cp);
   cp = unpack_ulong(&mp->buflen, cp);
   cp = unpack_ulong(&crc32, cp);

   DEBUG((SGE_EVENT, "read prolog: flags=0x"x32" headerlen=%d bufferlen="u32" checksum = "u32,
         mp->flags, mp->headerlen, mp->buflen, crc32));

   if(crc32 != cksum((char*)mp->prolog, PROLOGLEN-4)) {
      DEBUG((SGE_EVENT, "delete commproc on fd=%d cause of checksum error", mp->fromfd));
      delete_message(mp, "checksum error");
      delete_commproc_using_fd(mp->fromfd);
      DEXIT;
      return -1;
   }
   
   log_message(mp);
   DEXIT;
   return 0;
}

/**********************************************************************
  write message prolog 

  the prolog contains:
    flags      4 bytes
    headerlen  2 bytes
    bufferlen  4 bytes
    checksum   4 bytes

  return 0  if everything is written
         -1 on error
         1  if receiver isn't ready

  if an error occurs the message will be dropped
 **********************************************************************/
int write_message_prolog(
message *mp 
) {
   u_long already_written;
   int i, errorcode;

   DENTER(TOP_LAYER, "write_message_prolog");
   
   already_written = mp->bufprogress - mp->prolog;

   DEBUG((SGE_EVENT, "write prolog: already written=%ld", already_written));

   while (already_written < PROLOGLEN) {
      i = write(mp->tofd, mp->bufprogress, PROLOGLEN - already_written);
      errorcode = errno;        /* save */

      DEBUG((SGE_EVENT, "write returned %d", i));

      if (!i) {
         reset_message(mp, MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_0 );
         DEXIT;
         return -1;
      }

      if (i == -1) {
         if (errorcode == EWOULDBLOCK || errno == EAGAIN) {
            DEXIT;
            return 1;           /* sender isn't ready -> try later */
         }   
         else {                 /* a real error -> reset message to S_RDY_4_SND */
            disconnect_commproc_using_fd(mp->tofd);
            reset_message(mp, MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_NEG_1 );
            DEXIT;
            return -1;
         }
      }

      mp->bufprogress += i;
      already_written += i;
   }

   /* set progress pointer to header */
   mp->bufprogress = HEADERSTART(mp);

   DEBUG((SGE_EVENT, "written prolog"));

   log_message(mp);
   DEXIT;
   return 0;
}
