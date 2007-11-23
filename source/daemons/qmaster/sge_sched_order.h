#ifndef _SGE_SCHED_ORDER_H_
#define _SGE_SCHED_ORDER_H_
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
 *   Copyright: 2007 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

typedef struct {
   lList *order_list;
   state_gdi_multi *first;
   state_gdi_multi *last;
} gdi_request_queue_t;

extern gdi_request_queue_t Master_Request_Queue;

bool
sge_schedd_add_gdi_order_request(sge_gdi_ctx_class_t *ctx, lList **answer_list, lList **order_list);

bool
sge_schedd_send_orders(sge_gdi_ctx_class_t *ctx, lList **order_list, lList **answer_list, const char *name);

int
sge_schedd_get_unhandled_request_count(sge_gdi_ctx_class_t *ctx,
                                       lList **answer_list);

bool
sge_schedd_block_until_oders_processed(sge_gdi_ctx_class_t *ctx, lList **answer_list);

#endif
