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
#include <sys/types.h>

#include "message.h"
#include "commd_io.h"
#include "sge_time.h"
#include "sge_io.h"

void deliver_message(message *mp, int local);

/**********************************************************************/
/* deliver message                                                    */
/* connection is done -> we have an ready to write fd                 */
/* if this is initiated by the receiver he is already acknowledged    */
/* so prepare message to be delivered                                 */
/* if delivering local no receiver is placed in the header            */
/**********************************************************************/
void deliver_message(
message *mp,
int local 
) {
   u_short len;
   unsigned char *cp;
   char *tohostname = NULL, *fromhostname = NULL;

   /* mp->tofd contains the fd over which we are connected with the receiver */

   /* setup header 
      The problem with setting up the header is, that we want to have it
      contiguous with the messagebody */

   /* first calculate headerlen */
   len = 0;
   if (!local) {
      tohostname = sge_host_get_mainname(mp->to.host);
      len += pack_string_len(tohostname);
      len += pack_string_len(mp->to.name);
      len += pack_ushort_len(mp->to.id);
   }
   len += pack_ulong_len(mp->mid);
   fromhostname = sge_host_get_mainname(mp->from.host);
   len += pack_string_len(fromhostname);
   len += pack_string_len(mp->from.name);
   len += pack_ushort_len(mp->from.id);
   len += pack_ushort_len(mp->tag);
   len += pack_ushort_len(mp->compressed);
   mp->headerlen = len;

   cp = mp->bufstart + (HEADERLEN - len);
   cp = pack_ulong(mp->mid, cp);
   if (!local) {
      cp = pack_string(tohostname, cp);
      cp = pack_string(mp->to.name, cp);
      cp = pack_ushort(mp->to.id, cp);
   }
   cp = pack_string(fromhostname, cp);
   cp = pack_string(mp->from.name, cp);
   cp = pack_ushort(mp->from.id, cp);
   cp = pack_ushort((ushort) mp->tag, cp);
   cp = pack_ushort(mp->compressed, cp);

   /* build prolog */
   mp->flags |= COMMD_SCOMMD;
   cp = pack_ulong(mp->flags, mp->prolog);
   cp = pack_ushort(mp->headerlen, cp);
   cp = pack_ulong(mp->buflen, cp);
   cp = pack_ulong(sge_cksum((char*)mp->prolog, PROLOGLEN-4), cp);

   mp->bufprogress = mp->prolog;

   if (local) {
      mp->ackchar = COMMD_CACK;
      SET_MESSAGE_STATUS(mp, S_ACK_THEN_PROLOG);
   }
   else
      SET_MESSAGE_STATUS(mp, S_WRITE_PROLOG);
}
