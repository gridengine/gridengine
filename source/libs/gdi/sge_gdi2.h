#ifndef __SGE_GDI2_H
#define __SGE_GDI2_H
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

#ifdef  __cplusplus
extern "C" {
#endif


lList
*sge_gdi2(sge_gdi_ctx_class_t *ctx, u_long32 target, u_long32 cmd, lList **lpp, lCondition *cp, lEnumeration *enp);

int
sge_gdi2_multi(sge_gdi_ctx_class_t *ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd, lList **lp, 
              lCondition *cp, lEnumeration *enp, lList **malpp,
              state_gdi_multi *state, bool do_copy);

int
sge_gdi2_multi_sync(sge_gdi_ctx_class_t *ctx, lList **alpp, int mode, u_long32 target, u_long32 cmd, lList **lp,
              lCondition *cp, lEnumeration *enp, lList **malpp,
              state_gdi_multi *state, bool do_copy, bool do_sync);

int sge_gdi2_get_any_request(sge_gdi_ctx_class_t *ctx, sge_pack_buffer *pb, 
                    int *tag, int synchron, u_long32 for_request_mid, u_long32* mid);

int sge_gdi2_send_any_request(int synchron, u_long32 *mid, sge_gdi_ctx_class_t *ctx, 
                         sge_pack_buffer *pb, 
                         int tag, u_long32  response_id, lList **alpp);


int sge_gdi2_send_ack_to_qmaster(sge_gdi_ctx_class_t *ctx, int sync, u_long32 type, u_long32 ulong_val, 
                            u_long32 ulong_val_2, lList **alpp);
/*

typedef struct {

   sge_gdi_ctx_t *gdi_ctx;

} ec2_ctx_t;

bool ec2_register(sge_gdi_ctx_t *ctx, bool exit_on_qmaster_down, lList** alpp, ec2_ctx_t **ec_ctx);

bool ec2_deregister(sge_gdi_ctx_t *ctx);

bool ec2_subscribe(sge_gdi_ctx_t *ctx, ev_event event);

bool ec2_subscribe_all(sge_gdi_ctx_t *ctx);

bool ec2_unsubscribe(sge_gdi_ctx_t *ctx, ev_event event);

bool ec2_unsubscribe_all(sge_gdi_ctx_t *ctx);

int ec_get_flush(ec2_ctx *ctx, ev_event event);
bool ec_set_flush(ec2_ctx *ctx, ev_event event, bool flush, int interval);
bool ec_unset_flush(ec2_ctx *ctx, ev_event event);

*/

#ifdef  __cplusplus
}
#endif

#endif



