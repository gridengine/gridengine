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

#include <pthread.h>

#include "basis_types.h"

#include "cull/cull.h"

#include "comm/commlib.h"

#include "rmon/sgermon.h"

#include "lck/sge_mtutil.h"

#include "uti/sge_log.h"
#include "uti/sge_thread_ctrl.h"
#include "uti/sge_time.h"

#include "sgeobj/sge_answer.h"

#include "gdi/sge_gdi_ctx.h"
#include "gdi/sge_gdi_packet.h"
#include "gdi/sge_gdi_packet_queue.h"

#define PACKET_QUEUE_MUTEX "packet_queue_mutex"

#define WORKER_WAIT_TIME_S 1
#define WORKER_WAIT_TIME_N 0

/****** gdi/request_internal/Master_Packet_Queue **************************
*  NAME
*     Master_Packet_Queue
*
*  SYNOPSIS
*     sge_gdi_packet_queue_class_t Master_Packet_Queue;
*
*  FUNCTION
*     Global packet queue used in GDI functions to exchange data 
*     and synchronize threads. 
*
*     Packet producers (listener thread, schedd thread, JVM thread) store 
*     new packets by calling sge_gdi_packet_queue_store_notify(). 
*
*     Packet consumers (worker threads) get incoming packets by calling 
*     sge_gdi_packet_queue_wait_for_new_packet().
*
*     Packet producers are responsible to safe a handle to packets they
*     created because they are responsible to free allocated momory 
*     after the packet is not needed anymore.
*
*     Before producers release allocated momory they have wait until
*     consumers do not access the packet structure anymore. This
*     synchronisation can be done via calls to 
*     sge_gdi_packet_wait_till_handled() and 
*     sge_gdi_packet_broadcast_that_handled().
*
*     sge_gdi_packet_queue_wakeup_all_waiting() function is used during
*     the shutdown process of qmaster to notify consumer threads
*     which are waiting in sge_gdi_packet_queue_wait_for_new_packet(). 
*
*  NOTES
*     MT-NOTE: Master_Packet_Queue is MT safe until it is only
*              accessed with the predefined functions
*
*  SEE ALSO
*     gdi/request_internal/sge_gdi_packet_queue_wait_for_new_packet()
*     gdi/request_internal/sge_gdi_packet_queue_wakeup_all_waiting()
*     gdi/request_internal/sge_gdi_packet_queue_store_notify()
*     gdi/request_internal/sge_gdi_packet_wait_till_handled()
*     gdi/request_internal/sge_gdi_packet_broadcast_that_handled()
*****************************************************************************/
sge_gdi_packet_queue_class_t Master_Packet_Queue = {
   PTHREAD_MUTEX_INITIALIZER,
   PTHREAD_COND_INITIALIZER,
   NULL,
   NULL,
   0,
   NULL,
   NULL,
   0,
   false
};

/****** gdi/request_internal/sge_gdi_packet_queue_wakeup_all_waiting() *********
*  NAME
*     sge_gdi_packet_queue_wakeup_all_waiting() -- waitup all waiting 
*
*  SYNOPSIS
*     void sge_gdi_packet_queue_wakeup_all_waiting(
*                                 sge_gdi_packet_queue_class_t *packet_queue) 
*
*  FUNCTION
*     Notifys all threads waiting in sge_gdi_packet_queue_wait_for_new_packet().
*     The threads will check if there is a new packet available or if 
*     the shutdown process of the process were the threads are part of has
*     begun. Threads were both not applys will continue waiting.
*
*  INPUTS
*     sge_gdi_packet_queue_class_t *packet_queue - packet queue 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_gdi_packet_queue_wakeup_all_waiting() is MT safe 
*
*  SEE ALSO
*     gdi/request_internal/sge_gdi_packet_queue_wait_for_new_packet() 
*******************************************************************************/
void
sge_gdi_packet_queue_wakeup_all_waiting(sge_gdi_packet_queue_class_t *packet_queue)
{
   DENTER(TOP_LAYER, "sge_gdi_packet_queue_wakeup_all_waiting");

   sge_mutex_lock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));

   /*
    * Wakeup all threads waiting for a handled packet. 
    */
   pthread_cond_broadcast(&(packet_queue->cond));

   sge_mutex_unlock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));
}

/****** gdi/request_internal/sge_gdi_packet_queue_store_notify() **********
*  NAME
*     sge_gdi_packet_queue_store_notify() -- store a new packet and notify 
*
*  SYNOPSIS
*     void sge_gdi_packet_queue_store_notify(
*                                   sge_gdi_packet_queue_class_t *packet_queue, 
*                                   sge_gdi_packet_class_t *packet,
*                                   monitoring_t *monitor) 
*
*  FUNCTION
*     A call to this function will append "packet" at the end of the 
*     "packet_queue" and notify all threads which have called 
*     sge_gdi_packet_queue_wait_for_new_packet(). 
*
*  INPUTS
*     sge_gdi_packet_queue_class_t *packet_queue - packet queue pointer 
*     sge_gdi_packet_class_t *packet             - handle to a new packet 
*     monitoring_t *monitor                      - monitor object pointer
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_gdi_packet_queue_store_notify() is MT safe 
*
*  SEE ALSO
*     gdi/request_internal/sge_gdi_packet_queue_wait_for_new_packet() 
*******************************************************************************/
void
sge_gdi_packet_queue_store_notify(sge_gdi_packet_queue_class_t *packet_queue,
                                  sge_gdi_packet_class_t *packet, bool high_prio,
                                  monitoring_t *monitor)
{
   cl_thread_settings_t *thread_config = NULL; 

   DENTER(TOP_LAYER, "sge_gdi_packet_queue_store_notify");

   thread_config = cl_thread_get_thread_config();

   sge_mutex_lock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));

   /*    
    * Append the packet at the end of the queue
    */
   if (packet != NULL) {
      if (high_prio) {
         if (packet_queue->first_packet_prio == NULL) {
            packet_queue->first_packet_prio = packet;
         } else {
            packet_queue->last_packet_prio->next = packet;
         }
         packet_queue->last_packet_prio = packet;
         packet_queue->counter_prio++;
      } else {
         if (packet_queue->first_packet == NULL) {
            packet_queue->first_packet = packet;
         } else {
            packet_queue->last_packet->next = packet;
         }
         packet_queue->last_packet = packet;
         packet_queue->counter++;
      }
      MONITOR_SET_QLEN(monitor, packet_queue->counter);
   }

   DPRINTF((SFN" added new packet (packet_queue->counter = "sge_U32CFormat")\n",
            thread_config ? thread_config->thread_name : "-NA-", packet_queue->counter));

   /*
    * Send a signal through the packet_queue->cond. Worker threads are waiting
    * for that signal so that they can handle the packet. 
    */
   DPRINTF((SFN" notifys one worker\n", thread_config ? thread_config->thread_name : "-NA-"));
   if (packet_queue->waiting > 0) {
      pthread_cond_signal(&(packet_queue->cond));
   }

   sge_mutex_unlock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));
}

/****** gdi/request_internal/sge_gdi_packet_queue_wait_for_new_packet() ********
*  NAME
*     sge_gdi_packet_queue_wait_for_new_packet() -- wait for a new packet 
*
*  SYNOPSIS
*     bool 
*     sge_gdi_packet_queue_wait_for_new_packet(
*                                   sge_gdi_packet_queue_class_t *packet_queue, 
*                                   sge_gdi_packet_class_t **packet) 
*
*  FUNCTION
*     This function tests if there is a new "packet" in the "packet_queue".
*     If this case it returns immidiately with a handle to the first packet and
*     the packet is dechained from the queue before. If there is no packet the
*     functions blocks for maximally WORKER_WAIT_TIME_S seconds and tests 
*     again if there is a packet available or if the shutdown process has begun.
*
*     find more information in sge_gdi_packet_queue_store_notify() and
*     sge_gdi_packet_queue_wakeup_all_waiting()
* 
*      
* 
*     the thread is not in the process of terminatiion then the thread will block 
*     until someone calls either sge_gdi_packet_queue_store_notify() or
*     sge_gdi_packet_queue_wakeup_all_waiting().
*
*  INPUTS
*     sge_gdi_packet_queue_class_t *packet_queue - packet queue pointer 
*     sge_gdi_packet_class_t **packet            - pointer to an unhandled packet
*                                                  or NULL 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error occured
*
*  NOTES
*     MT-NOTE: sge_gdi_packet_queue_wait_for_new_packet() is MT safe 
*
*  SEE ALSO
*     gdi/request_internal/sge_gdi_packet_queue_store_notify()
*     gdi/request_internal/sge_gdi_packet_queue_wakeup_all_waiting() 
*******************************************************************************/
bool
sge_gdi_packet_queue_wait_for_new_packet(sge_gdi_packet_queue_class_t *packet_queue,
                                         sge_gdi_packet_class_t **packet,
                                         monitoring_t *monitor)
{
   bool ret = true;
   DENTER(TOP_LAYER, "sge_gdi_packet_queue_wait_for_new_packet");

   if (packet != NULL) {
      cl_thread_settings_t *thread_config = cl_thread_get_thread_config();
      sge_mutex_lock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));

      /* 
       * Wait until there is a packet in the queue or until we get the information 
       * that shutdown has begun. If none of the events occured then wait
       * that someone calls sge_gdi_packet_queue_wakeup_all_waiting() or
       * sge_gdi_packet_queue_store_notify()
       */
      if (packet_queue->first_packet == NULL && packet_queue->first_packet_prio == NULL) {
         packet_queue->waiting++;
         DPRINTF((SFN" is waiting for packet (packet_queue->waiting = "
                  sge_U32CFormat")\n", thread_config ? thread_config->thread_name : "-NA-", 
                  packet_queue->waiting));
         do {
            struct timespec ts;
            u_long32 current_time = 0; 

            current_time = sge_get_gmt();
            ts.tv_sec = (time_t)(current_time + WORKER_WAIT_TIME_S);
            ts.tv_nsec = WORKER_WAIT_TIME_N;;
            pthread_cond_timedwait(&(packet_queue->cond), &(packet_queue->mutex), &ts);
         } while (packet_queue->first_packet == NULL && packet_queue->first_packet_prio == NULL && 
                  sge_thread_has_shutdown_started() == false);
         packet_queue->waiting--;
      }

      /* 
       * If there is a packet then dechain it an return it to the caller 
       */
      if (packet_queue->first_packet_prio != NULL) {
         *packet = packet_queue->first_packet_prio;

         if (packet_queue->first_packet_prio == packet_queue->last_packet_prio) {
            packet_queue->last_packet_prio = NULL;
            packet_queue->first_packet_prio = NULL;
         } else {
            packet_queue->first_packet_prio = (*packet)->next;
         }
         (*packet)->next = NULL;
         packet_queue->counter_prio--;
         DPRINTF((SFN" takes packet from priority queue. ("
                  "packet_queue->counter_prio = "sge_U32CFormat
                  "; packet_queue->counter = "sge_U32CFormat
                  "; packet_queue->waiting = "sge_U32CFormat")\n",
                  thread_config ? thread_config->thread_name : "-NA-", 
                  packet_queue->counter_prio, 
                  packet_queue->counter, 
                  packet_queue->waiting));
      } else if (packet_queue->first_packet != NULL) {
         *packet = packet_queue->first_packet;

         if (packet_queue->first_packet == packet_queue->last_packet) {
            packet_queue->last_packet = NULL;
            packet_queue->first_packet = NULL;
         } else {
            packet_queue->first_packet = (*packet)->next;
         }
         (*packet)->next = NULL;
         packet_queue->counter--;
         DPRINTF((SFN" takes packet from priority queue. ("
                  "packet_queue->counter_prio = "sge_U32CFormat
                  "; packet_queue->counter = "sge_U32CFormat
                  "; packet_queue->waiting = "sge_U32CFormat")\n",
                  thread_config ? thread_config->thread_name : "-NA-", 
                  packet_queue->counter_prio, 
                  packet_queue->counter, 
                  packet_queue->waiting));
      } else {
         *packet = NULL;
         DPRINTF((SFN" wokeup but got no packet. ("
                  "packet_queue->counter_prio = "sge_U32CFormat
                  "; packet_queue->counter = "sge_U32CFormat
                  "; packet_queue->waiting = "sge_U32CFormat")\n",
                  thread_config ? thread_config->thread_name : "-NA-", 
                  packet_queue->counter_prio, 
                  packet_queue->counter, 
                  packet_queue->waiting));
      }

      MONITOR_SET_QLEN(monitor, ((packet_queue != NULL) ?  packet_queue->counter_prio +  packet_queue->counter : 0));

      sge_mutex_unlock(PACKET_QUEUE_MUTEX, SGE_FUNC, __LINE__, &(packet_queue->mutex));
   }

   DRETURN(ret);
}

