#ifndef _SGE_QMASTER_PROCESS_MESSAGE_H_
#define _SGE_QMASTER_PROCESS_MESSAGE_H_

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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "sge_monitor.h"

#include "gdi/sge_gdi_ctx.h"
#include "gdi/sge_gdi_packet.h"

/***************************************************
 *
 * The next section ensures, that GDI multi request
 * will be handled atomic and that other requests do
 * not interfer with the GDI multi get requsts. 
 *
 * Some assumption have been made for the current
 * implementation. They should minimize the performance
 * impact of this serialisation.
 *
 * Assumption:
 * 1) If the first GDI multi request is a get request
 *    all GDI request in the GDI multi are get requests
 *
 * 2) if the first GDI multi request is not a get request
 *    all GDI requests are not a get request
 * 
 * Based on this assumption we can create the following
 * execution matrix (GDI is used for atomic GDI requests
 * and load/job reports:
 *
 *          |  GDI     |  M-GDI-R  | M-GDI-W
 *  --------|----------|-----------|---------
 *  GDI     | parallel |  seriel   | parallel
 *  --------|----------|-----------|---------
 *  M-GDI-R | seriel   | parallel  | seriel
 *  --------|----------|-----------|---------
 *  M-GDI-W | parallel | seriel    | parallel
 *          |          |           |
 *
 * states: 
 *  NONE     0
 *  GDI      1
 *  M-GDI-R  2
 *  M-GDI-W  1 
 *
 * Based on the matrix, we do not need seperated
 * states for GDI and M-GDI-W.
 *
 * The implementation will allow a new requst to
 * execute, when no other request is executed or
 * the exectuted request as the same state as the
 * new one. If that is not the case, the new request
 * will be blocked until the others have finished.
 *
 * Implementation:
 *
 *  eval_message_and_block - eval message and assign states
 *  eval_gdi_and_block     - eval gdi and assign states
 *
 *  eval_atomic            - check current execution and block
 *
 *  eval_atomic_end        - release current block
 */
typedef enum {
   ATOMIC_NONE = 0,
   ATOMIC_SINGLE = 1,
   ATOMIC_MULTIPLE_WRITE = 1,
   ATOMIC_MULTIPLE_READ = 2
} request_handling_t;

typedef struct {
   request_handling_t type;      /* execution type*/
   int                counter;   /* number of requests executed of type */
   pthread_cond_t     cond_var;  /* used to block other threads */
   bool               signal;    /* need to signal? */
   pthread_mutex_t    mutex;     /* mutex to gard this structure */
} message_control_t;

request_handling_t
eval_gdi_and_block(sge_gdi_task_class_t *packet);

void       
eval_atomic_end(request_handling_t type);

void 
sge_qmaster_process_message(sge_gdi_ctx_class_t *ctx, monitoring_t *monitor);

#endif /* _SGE_QMASTER_PROCESS_MESSAGE_H_ */
