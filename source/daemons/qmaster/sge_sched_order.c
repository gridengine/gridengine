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

#include "gdi/sge_gdi2.h"
#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_answer.h"

#include "sge_sched_order.h"

gdi_request_queue_t Master_Request_Queue = {
   NULL,
   NULL 
};

bool
sge_schedd_send_orders(sge_gdi_ctx_class_t *ctx, lList **order_list, lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_schedd_send_orders");

   if ((order_list != NULL) && (*order_list != NULL) && (lGetNumberOfElem(*order_list) != 0)) {
      state_gdi_multi *state = NULL;
      
      state = (state_gdi_multi *)sge_malloc(sizeof(state_gdi_multi));
      if (state != NULL) {
         int order_id;

         memset(state, 0, sizeof(state_gdi_multi));
         order_id = ctx->gdi_multi(ctx, answer_list, SGE_GDI_SEND, SGE_ORDER_LIST, SGE_GDI_ADD,
                                   order_list, NULL, NULL, state, false);

         if ((answer_list == NULL) && (order_id != -1)) {
            /*
             * Evaluation of the asyncron internal GDI request is in function 
             * sge_schedd_block_until_oders_processed() which will be called 
             * later on. The only remaining thing to do is to store the state
             * variable into the request queue Master_Request_Queue
             */
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
            answer_list_handle_request_answer_list(answer_list, stderr);
            ret = false;
         }
      } else {
         /* EB: TODO: ST: do error handling */
      }
   }

   DRETURN(ret);
}

bool
sge_schedd_block_until_oders_processed(sge_gdi_ctx_class_t *ctx, 
                                       lList **answer_list)
{
   bool ret = true;
   state_gdi_multi *current_state, *next_state;

   DENTER(TOP_LAYER, "sge_schedd_block_until_oders_processed");

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
       * now we have an answer is it positive? 
       */
      order_id = 1;
      sge_gdi_extract_answer(&request_answer_list, SGE_GDI_ADD, SGE_ORDER_LIST,
                             order_id, multi_answer_list, NULL);
      if (request_answer_list != NULL) {
         /* EB: TODO: ST: put errors into message file */
         answer_list_handle_request_answer_list(&request_answer_list, stderr);
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
