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
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_conf.h"
#include "sge_sched.h"
#include "sge_signal.h"
#include "sge_subordinate_qmaster.h"
#include "sge_event_master.h"
#include "sge_qmod_qmaster.h"
#include "sge_qinstance_qmaster.h"
#include "msg_qmaster.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_job.h"
#include "sge_cqueue.h"
#include "sge_object.h"
#include "sge_subordinate.h"
#include "sge_qref.h"

/*
   (un)suspend on subordinate using granted_destination_identifier_list

   NOTE:
      we assume the associated job is already/still
      debited on all the queues that are referenced in gdil
*/
bool
cqueue_list_x_on_subordinate_gdil(lList *this_list, bool suspend,
                                  const lList *gdil)
{
   bool ret = true;
   lListElem *gdi = NULL;

   DENTER(TOP_LAYER, "cqueue_list_x_on_subordinate_gdil");
   for_each(gdi, gdil) {
      const char *full_name = lGetString(gdi, JG_qname);
      lListElem *queue = cqueue_list_locate_qinstance(this_list, full_name);

      if (queue != NULL) {
         lList *so_list = lGetList(queue, QU_subordinate_list);
         lList *resolved_so_list = NULL;
         lListElem *so = NULL;

         /*
          * Resolve cluster queue names into qinstance names
          */
         so_list_resolve(so_list, NULL, &resolved_so_list, full_name);

         for_each(so, resolved_so_list) {
            u_long32 slots = lGetUlong(queue, QU_job_slots);
            u_long32 slots_used = qinstance_slots_used(queue);
            u_long32 slots_granted = lGetUlong(gdi, JG_slots);

            /*
             * suspend:
             *    no sos before this job came on this queue AND
             *    sos since job is on this queue
             *
             * unsuspend:
             *    no sos after job gone from this queue AND
             *    sos since job is on this queue
             */
            if (!tst_sos(slots_used - slots_granted, slots, so) && 
                tst_sos(slots_used, slots, so)) {
               const char *so_queue_name = lGetString(so, SO_name);
               lListElem *so_queue = 
                        cqueue_list_locate_qinstance(this_list, so_queue_name);

               if (so_queue != NULL) {

                  /*
                   * Suspend/unsuspend the subordinated queue
                   */
                  ret &= qinstance_x_on_subordinate(so_queue, suspend, false);

               } else {
                  ERROR((SGE_EVENT, MSG_QINSTANCE_NQIFOUND_SS, 
                         so_queue_name, SGE_FUNC));
                  ret = false;
               }
            }
         }
         resolved_so_list = lFreeList(resolved_so_list);
      } else {
         /* should never happen */
         ERROR((SGE_EVENT, MSG_QINSTANCE_NQIFOUND_SS, full_name, SGE_FUNC));
         ret = false;
      } 
   }
   DEXIT;
   return ret;
}

bool
qinstance_x_on_subordinate(lListElem *this_elem, bool suspend,
                           bool rebuild_cache)
{
   int ret = 0;
   u_long32 sos_counter;
   bool do_action;
   bool send_qinstance_signal;
   const char *hostname;
   const char *cqueue_name;
   const char *full_name;
   int signal;
   ev_event event;

   DENTER(TOP_LAYER, "qinstance_x_on_subordinate");

   /* increment sos counter */
   sos_counter = lGetUlong(this_elem, QU_suspended_on_subordinate);
   if (suspend) {
      sos_counter++;
   } else {
      sos_counter--;
   }
   lSetUlong(this_elem, QU_suspended_on_subordinate, sos_counter);

   /* 
    * prepare for operation
    *
    * suspend:  
    *    send a signal if it is not already suspended by admin or calendar 
    *
    * !suspend:
    *    send a signal if not still suspended by admin or calendar
    */
   hostname = lGetHost(this_elem, QU_qhostname);
   cqueue_name = lGetString(this_elem, QU_qname);
   full_name = lGetString(this_elem, QU_full_name);
   send_qinstance_signal = !(qinstance_state_is_manual_suspended(this_elem) ||
                             qinstance_state_is_cal_suspended(this_elem));
   if (suspend) {
      do_action = (sos_counter == 1);
      signal = SGE_SIGSTOP;
      event = sgeE_QINSTANCE_SOS;
   } else {
      send_qinstance_signal = !send_qinstance_signal;
      do_action = (sos_counter == 0);
      signal = SGE_SIGCONT;
      event = sgeE_QINSTANCE_USOS;
   }

   /*
    * do operation
    */
   DPRINTF(("qinstance "SFQ" "SFN" "SFN" on subordinate\n", full_name,
            (do_action ? "" : "already"),
            (suspend ? "suspended" : "unsuspended")));
   if (do_action) {
      if (send_qinstance_signal && !rebuild_cache) {
         ret |= sge_signal_queue(signal, this_elem, NULL, NULL);
      }

      qinstance_state_set_susp_on_sub(this_elem, suspend);

      sge_add_event(NULL, 0, event, 0, 0, cqueue_name, hostname, NULL, NULL);
      lListElem_clear_changed_info(this_elem);
   }

   DEXIT;
   return ret;
}

bool
cqueue_list_x_on_subordinate_so(lList *this_list, lList **answer_list,
                                bool suspend, const lList *resolved_so_list,
                                bool do_recompute_caches)
{
   bool ret = true;
   const lListElem *so = NULL;

   DENTER(TOP_LAYER, "cqueue_list_x_on_subordinate_qref");

   /*
    * Locate all qinstances which are mentioned in resolved_so_list and 
    * (un)suspend them
    */
   for_each(so, resolved_so_list) {
      const char *full_name = lGetString(so, SO_name);

      lListElem *qinstance = cqueue_list_locate_qinstance(this_list, full_name);

      if (qinstance != NULL) {
         ret &= qinstance_x_on_subordinate(qinstance, suspend,
                                           do_recompute_caches);
         if (!ret) {
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

bool
qinstance_find_suspended_subordinates(const lListElem *this_elem,
                                      lList **answer_list,
                                      lList **resolved_so_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_find_suspended_subordinates");
   if (this_elem != NULL && resolved_so_list != NULL) {
      lList *so_list = lGetList(this_elem, QU_subordinate_list);
      lListElem *next_so = NULL;
      lListElem *so = NULL;
      const char *full_name = lGetString(this_elem, QU_full_name);

      /*
       * Resolve cluster queue names into qinstance names
       */
      so_list_resolve(so_list, NULL, resolved_so_list, full_name);

      /*
       * Remove all subordinated queues from "resolved_so_list" which
       * are not actually suspended by "this_elem" 
       */
      next_so = lFirst(*resolved_so_list);
      while ((so = next_so) != NULL) {
         u_long32 slots = lGetUlong(this_elem, QU_job_slots);
         u_long32 slots_used = qinstance_slots_used(this_elem);

         next_so = lNext(so);
         if (!tst_sos(slots_used, slots, so)) {
            lRemoveElem(*resolved_so_list, so);
         }
      }
   }
   DEXIT;
   return ret;
}

bool
qinstance_initialize_sos_attr(lListElem *this_elem) 
{
   bool ret = true;
   lList *master_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
   lListElem *cqueue = NULL;

   DENTER(TOP_LAYER, "qinstance_initialize_sos_attr");
   for_each(cqueue, master_list) {
      const char *full_name = lGetString(this_elem, QU_full_name);
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL; 

      for_each(qinstance, qinstance_list) {
         lList *so_list = lGetList(qinstance, QU_subordinate_list);
         lList *resolved_so_list = NULL;
         lListElem *so = NULL;

         /*
          * Resolve cluster queue names into qinstance names
          */
         so_list_resolve(so_list, NULL, &resolved_so_list, full_name);

         for_each(so, resolved_so_list) {
            const char *so_full_name = lGetString(so, SO_name);

            if (!strcmp(full_name, so_full_name)) {
               qinstance_x_on_subordinate(this_elem, true, false); 
            } 
         }
         resolved_so_list = lFreeList(resolved_so_list);
      }
   }
   DEXIT;
   return ret;
}
