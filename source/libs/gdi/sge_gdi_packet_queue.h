#ifndef __SGE_PACKET_QUEUE_H
#define __SGE_PACKET_QUEUE_H
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

#include "uti/sge_monitor.h"

typedef struct _sge_gdi_packet_queue_class_t sge_gdi_packet_queue_class_t;

struct _sge_gdi_packet_queue_class_t {
   /* 
    * mutex to gard all members of this structure 
    */
   pthread_mutex_t mutex;

   /* 
    * used to block/signal threads 
    */
   pthread_cond_t cond;

   /* 
    * pointer to the first and last packet of a packet queue
    */
   sge_gdi_packet_class_t *first_packet;
   sge_gdi_packet_class_t *last_packet;

   /*
    * number of elements in the queue
    */
   u_long32 counter;

   /* 
    * if "true" then at least one thread waits to be signalled
    * with the pthread condition 'cond' part of this structure 
    */
   u_long32 waiting;
};

extern sge_gdi_packet_queue_class_t Master_Packet_Queue;

u_long32
sge_gdi_packet_queue_get_length(sge_gdi_packet_queue_class_t *packet_queue);

void
sge_gdi_packet_queue_store_notify(sge_gdi_packet_queue_class_t *packet_queue,
                                  sge_gdi_packet_class_t *packet, 
                                  monitoring_t *monitor);

void
sge_gdi_packet_queue_wakeup_all_waiting(sge_gdi_packet_queue_class_t *packet_queue);

bool
sge_gdi_packet_queue_wait_for_new_packet(sge_gdi_packet_queue_class_t *packet_queue,
                                         sge_gdi_packet_class_t **packet,
                                         monitoring_t *monitor);

#endif

