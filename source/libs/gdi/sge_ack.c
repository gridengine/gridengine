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

#include "sge_gdi_intern.h"
#include "sge_prog.h"
#include "pack.h"
#include "commlib.h"
#include "qm_name.h"

int sge_send_ack_to_qmaster(
int sync,
u_long32 type,
u_long32 ulong_val,
u_long32 ulong_val_2
) {
   int ret;
   sge_pack_buffer pb;

   /* send an ack to the qmaster for the events */
   if(init_packbuffer(&pb, 3*sizeof(u_long32), 0) != PACK_SUCCESS) {
      return CL_MALLOC;
   }

   packint(&pb, type);
   packint(&pb, ulong_val);
   packint(&pb, ulong_val_2);
   ret = sge_send_any_request(sync, NULL, sge_get_master(0), 
         prognames[QMASTER], 1, &pb, TAG_ACK_REQUEST);
   clear_packbuffer(&pb);

   return ret;
}
