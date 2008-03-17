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

#include <string.h>

#include "rmon/sgermon.h"

#include "uti/sge_log.h"

#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_answer.h"

#include "sge_sched_order.h"

#include "msg_common.h"

gdi_request_queue_t Master_Request_Queue = {
   NULL,
   NULL,
   NULL 
};

bool
sge_schedd_send_orders(sge_gdi_ctx_class_t *ctx, order_t *orders, lList **order_list, lList **answer_list, const char *name)
{
   static bool is_initialized = false;
   static int max_unhandled = 0;
   bool ret = true;

   DENTER(TOP_LAYER, "sge_schedd_send_orders");
   if (!is_initialized) {
      max_unhandled = mconf_get_max_order_limit();
      INFO((SGE_EVENT, "Maximum number of unhandled GDI order requests limited to %d. Change this by setting MAX_ORDER_LIMT in qmaster_params and restart qmaster!\n", max_unhandled));
      is_initialized = true;
   }
   if ((order_list != NULL) && (*order_list != NULL) && (lGetNumberOfElem(*order_list) != 0)) {
      int unhandled = 0;

      /*
       * Add the new orders 
       */
      if (Master_Request_Queue.order_list == NULL) {
         Master_Request_Queue.order_list = *order_list;
         *order_list = NULL;
      } else {
         lAddList(Master_Request_Queue.order_list, order_list);
      }

      /*
       * send order list only if maximum unhandled order count is not reached
       */
      unhandled = sge_schedd_get_unhandled_request_count(ctx, answer_list);
      if (unhandled < max_unhandled) {
         sge_schedd_add_gdi_order_request(ctx, orders, answer_list, &Master_Request_Queue.order_list);
      } 
   }
   lFreeList(order_list);

   DRETURN(ret);
}

bool
sge_schedd_add_gdi_order_request(sge_gdi_ctx_class_t *ctx, order_t *orders, lList **answer_list, lList **order_list) 
{
   bool ret = true;
   state_gdi_multi *state = NULL;

   DENTER(TOP_LAYER, "sge_schedd_add_gdi_order_request");
   state = (state_gdi_multi *)sge_malloc(sizeof(state_gdi_multi));
   if (state != NULL) {
      int order_id;

      memset(state, 0, sizeof(state_gdi_multi));
      orders->numberSendOrders += lGetNumberOfElem(*order_list);
      orders->numberSendPackages++;

      /*
       * order_list will be NULL after the call of gdi_multi. This saves a copy operation.
       */
      order_id = ctx->gdi_multi(ctx, answer_list, SGE_GDI_SEND, SGE_ORDER_LIST, SGE_GDI_ADD,
                                order_list, NULL, NULL, state, false);

      if ((answer_list == NULL) && (order_id != -1)) {
         if (Master_Request_Queue.first == NULL) {
            state->next = NULL;
            Master_Request_Queue.first = state;
            Master_Request_Queue.last = state;
         } else {
            state->next = NULL;
            Master_Request_Queue.last->next = state;
            Master_Request_Queue.last = state;
         }
      } else {
         answer_list_log(answer_list, false, false);
         ret = false;
      }
   } else {
      answer_list_add(answer_list, MSG_SGETEXT_NOMEM, STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      ret = false;
   } 
   DRETURN(ret);
}



int
sge_schedd_get_unhandled_request_count(sge_gdi_ctx_class_t *ctx,
                                       lList **answer_list)
{
   int counter = 0;
   state_gdi_multi *current_state, *next_state;

   DENTER(TOP_LAYER, "sge_schedd_get_unhandled_request_count");
   next_state = Master_Request_Queue.first;
   while ((current_state = next_state) != NULL) {
      next_state = current_state->next;

      counter += (sge_gdi2_is_done(ctx, answer_list, current_state) ? 1 : 0);
   }
   DRETURN(counter);
}

bool
sge_schedd_block_until_orders_processed(sge_gdi_ctx_class_t *ctx, 
                                        lList **answer_list)
{
   bool ret = true;
   state_gdi_multi *current_state = NULL;
   state_gdi_multi *next_state = NULL; 
   state_gdi_multi *last_but_one = NULL;

   DENTER(TOP_LAYER, "sge_schedd_block_until_orders_processed");

   /*
    * Before we wait for all order requests to be finished 
    * we will wait for the last package...
    *
    * Reason: The GDI order packages are executed in the same sequence 
    * they are stored in the worker queue. Therefore the last package will
    * be executed last. When the last package is done all previous are 
    * also done.
    *
    * Resulting improvement: pthread_cond_wait() (which is executed in
    * ctx->gdi_wait()) will only be executed once.
    */
   next_state = Master_Request_Queue.first;
   while ((current_state = next_state) != NULL) {
      next_state = current_state->next;
      if (next_state != NULL && next_state->next == NULL) {
         last_but_one = current_state;
         break;
      }
   }
      
   if (last_but_one != NULL) {
      lList *request_answer_list = NULL;
      lList *multi_answer_list = NULL;
      int order_id;

      /* 
       * wait for answer. this call might block if the request
       * has not been handled by any worker until now.
       * subsequent calls to sge_gdi2_wait (== ctx->gdi_wait()) in 
       * this function will only do a boolean compare because the 
       * concerned requests are then already finished
       */
      ctx->gdi_wait(ctx, answer_list, &multi_answer_list, last_but_one->next);

      /*
       * now we have an answer. is it positive? 
       */
      order_id = 1;
      sge_gdi_extract_answer(&request_answer_list, SGE_GDI_ADD, SGE_ORDER_LIST,
                             order_id, multi_answer_list, NULL);
      if (request_answer_list != NULL) {
         answer_list_log(&request_answer_list, false, false);
         ret = false;
      }

      /*
       * memory cleanup
       */
      lFreeList(&request_answer_list);
      lFreeList(&multi_answer_list);
      last_but_one->next = (state_gdi_multi*)sge_free((char*)(last_but_one->next));
      Master_Request_Queue.last = last_but_one;
   }

   /*
    * wait till all GDI order requests are finished
    */
   next_state = Master_Request_Queue.first;
   while ((current_state = next_state) != NULL) {
      lList *request_answer_list = NULL;
      lList *multi_answer_list = NULL;
      int order_id;

      /* 
       * get next element no so that we can destray the current one later 
       */
      next_state = current_state->next;

      /* 
       * remove first element from the list 
       */
      if (Master_Request_Queue.first == Master_Request_Queue.last) {
         Master_Request_Queue.first = NULL;
         Master_Request_Queue.last = NULL;
      } else { 
         Master_Request_Queue.first = current_state->next;
      }

      /* 
       * wait for answer. this call might block if the request
       * has not been handled by any worker until now
       */
      ctx->gdi_wait(ctx, answer_list, &multi_answer_list, current_state);

      /*
       * now we have an answer. is it positive? 
       */
      order_id = 1;
      sge_gdi_extract_answer(&request_answer_list, SGE_GDI_ADD, SGE_ORDER_LIST,
                             order_id, multi_answer_list, NULL);
      if (request_answer_list != NULL) {
         answer_list_log(&request_answer_list, false, false);
         ret = false;
      }

      /*
       * memory cleanup
       */
      lFreeList(&request_answer_list);
      lFreeList(&multi_answer_list);
      current_state = (state_gdi_multi*)sge_free((char*)current_state);
   }
   DRETURN(ret);
}
