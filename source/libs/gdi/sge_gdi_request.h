#ifndef __SGE_GDI_REQUEST_H
#define __SGE_GDI_REQUEST_H
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

#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef __SGE_GDI_H
#include "sge_gdi.h"
#endif

struct _sge_gdi_request {
   u_long32         op;
   u_long32         target;

   char             *host;
   char             *commproc;
   u_short          id;

   u_long32         version;
   lList            *lp;
   lList            *alp;
   lCondition       *cp;
   lEnumeration     *enp; 
   char             *auth_info;     
   u_long32         sequence_id;
   u_long32         request_id;
   sge_gdi_request  *next;   
};

int sge_send_gdi_request(int sync, const char *rhost, const char *commproc,
                         int id, sge_gdi_request *head,u_long32 *mid,
                         unsigned long response_id, lList **alpp);
int sge_unpack_gdi_request(sge_pack_buffer *pb, sge_gdi_request **arp);
int sge_pack_gdi_request(sge_pack_buffer *pb, sge_gdi_request *ar);
sge_gdi_request* free_gdi_request(sge_gdi_request *ar);
sge_gdi_request* new_gdi_request(void);

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_ANY_REQUEST_H */
