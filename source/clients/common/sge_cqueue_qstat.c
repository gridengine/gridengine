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


#include "sge.h"
#include "sgermon.h"
#include "sge_hostL.h"
#include "sge_cqueueL.h"
#include "sge_qinstanceL.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_select_queue.h"
#include "sge_cqueue_qstat.h"


bool cqueue_calculate_summary(const lListElem *cqueue, 
                                     const lList *exechost_list,
                                     const lList *centry_list,
                                     double *load, 
                                     bool *is_load_available, 
                                     u_long32 *used,
                                     u_long32 *total,
                                     u_long32 *suspend_manual, 
                                     u_long32 *suspend_threshold,
                                     u_long32 *suspend_on_subordinate,
                                     u_long32 *suspend_calendar,
                                     u_long32 *unknown,
                                     u_long32 *load_alarm, 
                                     u_long32 *disabled_manual,
                                     u_long32 *disabled_calendar,
                                     u_long32 *ambiguous,
                                     u_long32 *orphaned,
                                     u_long32 *error,
                                     u_long32 *available,
                                     u_long32 *temp_disabled,
                                     u_long32 *manual_intervention)

{
   bool ret = true;
   
   DENTER(TOP_LAYER, "cqueue_calculate_summary");
   if (cqueue != NULL) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL;
      double host_load_avg = 0.0;
      u_long32 load_slots = 0;

      *load = 0.0;
      *is_load_available = false;
      *used = *total = 0;
      *available = *temp_disabled = *manual_intervention = 0;
      *suspend_manual = *suspend_threshold = *suspend_on_subordinate = 0;
      *suspend_calendar = *unknown = *load_alarm = 0;
      *disabled_manual = *disabled_calendar = *ambiguous = 0;
      *orphaned = *error = 0; 
      for_each(qinstance, qinstance_list) {
         u_long32 slots = lGetUlong(qinstance, QU_job_slots);
         bool has_value_from_object;

         (*used) += qinstance_slots_used(qinstance);
         (*total) += slots;

         if (!sge_get_double_qattr(&host_load_avg, LOAD_ATTR_LOAD_AVG, 
                                   qinstance, exechost_list, centry_list, 
                                   &has_value_from_object)) {
            if (has_value_from_object) {
               *is_load_available = true;
               load_slots += slots;
               *load += host_load_avg * slots;
            } 
         } 

         /*
          * manual_intervention: cdsuE
          * temp_disabled: aoACDS
          */
         if (qinstance_state_is_manual_suspended(qinstance) ||
             qinstance_state_is_unknown(qinstance) ||
             qinstance_state_is_manual_disabled(qinstance) ||
             qinstance_state_is_ambiguous(qinstance) ||
             qinstance_state_is_error(qinstance)) {
            *manual_intervention += slots;
         } else if (qinstance_state_is_alarm(qinstance) ||
                    qinstance_state_is_cal_disabled(qinstance) ||
                    qinstance_state_is_orphaned(qinstance) ||
                    qinstance_state_is_susp_on_sub(qinstance) ||
                    qinstance_state_is_cal_suspended(qinstance) ||
                    qinstance_state_is_suspend_alarm(qinstance)) {
            *temp_disabled += slots;
         } else {
            *available += slots;
         }
         if (qinstance_state_is_unknown(qinstance)) {
            *unknown += slots;
         }
         if (qinstance_state_is_alarm(qinstance)) {
            *load_alarm += slots;
         }
         if (qinstance_state_is_manual_disabled(qinstance)) {
            *disabled_manual += slots;
         }
         if (qinstance_state_is_cal_disabled(qinstance)) {
            *disabled_calendar += slots;
         }
         if (qinstance_state_is_ambiguous(qinstance)) {
            *ambiguous += slots;
         }
         if (qinstance_state_is_orphaned(qinstance)) {
            *orphaned += slots;
         }
         if (qinstance_state_is_manual_suspended(qinstance)) {
            *suspend_manual += slots;
         }
         if (qinstance_state_is_susp_on_sub(qinstance)) {
            *suspend_on_subordinate += slots;
         }
         if (qinstance_state_is_cal_suspended(qinstance)) {
            *suspend_calendar += slots;
         }
         if (qinstance_state_is_suspend_alarm(qinstance)) {
            *suspend_threshold += slots;
         }
         if (qinstance_state_is_error(qinstance)) {
            *error += slots;
         }
      }  
      *load /= load_slots;
   }
   DEXIT;
   return ret;
}
