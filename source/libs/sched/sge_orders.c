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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "sge_orders.h"
#include "sge_all_listsL.h"
#include "sge_feature.h"
#include "sgermon.h"
#include "sge_log.h"
#include "schedd_message.h"
#include "sge_answer.h"
#include "sge_event_client.h"
#include "msg_schedd.h"
#include "msg_common.h"
#include "sge_profiling.h"


/****** sge_orders/sge_add_schedd_info() ***************************************
*  NAME
*     sge_add_schedd_info() -- retrieves the messages and generates an order out 
*                              of it.
*
*  SYNOPSIS
*     lList* sge_add_schedd_info(lList *or_list, int *global_mes_count, int 
*     *job_mes_count) 
*
*  FUNCTION
*     retrieves all messages, puts them into an order package, and frees the
*     orginal messages. It also returns the number of global and job messages.
*
*  INPUTS
*     lList *or_list        - int: the order list to which the message order is added
*     int *global_mes_count - out: global message count
*     int *job_mes_count    - out: job message count
*
*  RESULT
*     lList* - the order list
*
*  NOTES
*     MT-NOTE: sge_add_schedd_info() is not MT safe 
*
*******************************************************************************/
lList *sge_add_schedd_info(lList *or_list, int *global_mes_count, int *job_mes_count) 
{
   lList *jlist;
   lListElem *sme, *ep;

   DENTER(TOP_LAYER, "sge_add_schedd_info");

   sme = schedd_mes_obtain_package(global_mes_count, job_mes_count);

   if (!sme || (lGetNumberOfElem(lGetList(sme, SME_message_list)) < 1 
         && lGetNumberOfElem(lGetList(sme, SME_global_message_list)) < 1)) {
      DEXIT;
      return or_list;
   }
   
   /* create orders list if not existent */
   if (!or_list) {
      or_list = lCreateList("orderlist", OR_Type);
   }   

   /* build order */
   ep=lCreateElem(OR_Type);   
   
   jlist = lCreateList("", SME_Type);
   lAppendElem(jlist, sme);
   lSetList(ep, OR_joker, jlist);
   
   lSetUlong(ep, OR_type, ORT_job_schedd_info);
   lAppendElem(or_list, ep);

   DEXIT;
   return or_list;
}

/*************************************************************
 Create a new order-list or add orders to an existing one.
 or_list==NULL -> create new one.
 or_list!=NULL -> append orders.
 returns updated order-list.

 The granted list contains granted queues.

 is not MT safe
 *************************************************************/

lList 
*sge_create_orders(lList *or_list, u_long32 type, lListElem *job, lListElem *ja_task,
                   lList *granted , bool no_tickets, bool update_execd) 
{
   static lEnumeration *tix_what = NULL;
   static lEnumeration *tix2_what = NULL;
   static lEnumeration *job_what = NULL;
   lList *ql = NULL;
   lListElem *gel, *ep, *ep2;
   u_long32 qslots;
  
   DENTER(TOP_LAYER, "sge_create_orders");
   
   if (!job) {
      DEXIT;
      return lFreeList(or_list);
   }

   /* create orders list if not existent */
   if (!or_list) {
      or_list = lCreateList("orderlist", OR_Type);
   }   

   /* build sublist of granted */
   if (update_execd) {
      for_each(gel, granted) {
         qslots = lGetUlong(gel, JG_slots);
         if (qslots) { /* ignore Qs with slots==0 */
            ep2=lCreateElem(OQ_Type);

            lSetUlong(ep2, OQ_slots, qslots);
            lSetString(ep2, OQ_dest_queue, lGetString(gel, JG_qname));
            lSetUlong(ep2, OQ_dest_version, lGetUlong(gel, JG_qversion));
            lSetDouble(ep2, OQ_ticket, lGetDouble(gel, JG_ticket));
            lSetDouble(ep2, OQ_oticket, lGetDouble(gel, JG_oticket));
            lSetDouble(ep2, OQ_fticket, lGetDouble(gel, JG_fticket));
            lSetDouble(ep2, OQ_sticket, lGetDouble(gel, JG_sticket));
            if (!ql)
               ql=lCreateList("orderlist",OQ_Type);
            lAppendElem(ql, ep2);
         }
      }
   }

   /* build order */
   ep=lCreateElem(OR_Type);

   if(!no_tickets) {
      lSetDouble(ep, OR_ticket,    lGetDouble(ja_task, JAT_tix));
      lSetDouble(ep, OR_ntix,      lGetDouble(ja_task, JAT_ntix));
      lSetDouble(ep, OR_prio,      lGetDouble(ja_task, JAT_prio));
   }
   else {
      lSetDouble(ep, OR_ticket, 0.0);
      lSetDouble(ep, OR_ntix, 0.0); 
      lSetDouble(ep, OR_prio, 0.0);
   }

   if (type == ORT_tickets || type == ORT_ptickets) {
      lListElem *jep;
      lList *jlist; 
      lList *tlist;

      /* Create a reduced task list with only the required fields */
      {             
         lListElem *tmp_elem;

         if(!tix_what){
            tix_what = lWhat("%T(%I %I %I %I %I %I %I %I %I %I )",
            lGetElemDescr(ja_task), 
            JAT_task_number, 
            JAT_status, 
            JAT_tix,
            JAT_oticket, 
            JAT_fticket, 
            JAT_sticket, 
            JAT_share,
            JAT_prio,
            JAT_ntix,
            JAT_granted_destin_identifier_list);

            tix2_what = lWhat("%T(%I %I %I %I %I %I %I %I %I)",
            lGetElemDescr(ja_task), 
            JAT_task_number, 
            JAT_status, 
            JAT_tix,
            JAT_oticket, 
            JAT_fticket, 
            JAT_sticket, 
            JAT_share,
            JAT_prio,
            JAT_ntix);

            if (!tix_what || !tix2_what) {
               CRITICAL((SGE_EVENT, MSG_SCHEDD_CREATEORDERS_LWHEREFORJOBFAILED));
               /* runtime type error */
               abort();
            }
         }

         if (update_execd){
            tmp_elem = lSelectElem(ja_task, NULL, tix_what, false);
         }
         else {
            tmp_elem = lSelectElem(ja_task, NULL, tix_what, false);
         }
      
         tlist = lCreateList("", lGetElemDescr(tmp_elem));
         lAppendElem(tlist, tmp_elem);
      
         if(no_tickets){
            lSetDouble(tmp_elem, JAT_tix, 0.0);
            lSetDouble(tmp_elem, JAT_oticket, 0.0);
            lSetDouble(tmp_elem, JAT_fticket, 0.0);
            lSetDouble(tmp_elem, JAT_sticket, 0.0);
            lSetDouble(tmp_elem, JAT_share, 0.0);
            lSetDouble(tmp_elem, JAT_prio, 0.0);
            lSetDouble(tmp_elem, JAT_ntix, 0.0);
         }
      }

      /* Create a reduced job list with only the required fields */
      {        
         if(job_what != NULL) {
            job_what = lWhat("%T(%I %I %I %I %I %I %I %I)", 
               lGetElemDescr(job), 
               JB_job_number, 
               JB_nppri, 
               JB_nurg, 
               JB_urg, 
               JB_rrcontr, 
               JB_dlcontr, 
               JB_wtcontr, 
               JB_ja_tasks);
         }
         jep = lSelectElem(job, NULL, job_what, false);
         jlist = lCreateList("", lGetElemDescr(jep));
         lSetList(jep, JB_ja_tasks, tlist);
         lAppendElem(jlist, jep);
      }

      lSetList(ep, OR_joker, jlist);
   }

   lSetUlong(ep, OR_type, type);
   lSetUlong(ep, OR_job_number, lGetUlong(job, JB_job_number));
   lSetUlong(ep, OR_ja_task_number, lGetUlong(ja_task, JAT_task_number));
   lSetUlong(ep, OR_job_version, lGetUlong(job, JB_version));
   lSetList(ep, OR_queuelist, ql);
   {
      const char *s;

      s = lGetString(ja_task, JAT_granted_pe);
      if (s) {
         lSetString(ep, OR_pe, s);
      }   
   }

   lAppendElem(or_list, ep);


   DEXIT;
   return or_list; 
}


/*************************************************************
  
 *************************************************************/
int sge_send_orders2master(
lList *orders 
) {
   int ret = STATUS_OK;
   lList *alp = NULL;
   lList *malp = NULL;

   int set_busy, order_id = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "sge_send_orders2master");

   /* do we have to set event client to "not busy"? */
   set_busy = (ec_get_busy_handling() == EV_BUSY_UNTIL_RELEASED);

   if (orders != NULL) {
      DPRINTF(("SENDING %d ORDERS TO QMASTER\n", lGetNumberOfElem(orders)));

      if(set_busy) {
         order_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_ORDER_LIST, SGE_GDI_ADD,
                                  orders, NULL, NULL, NULL, &state);
      } else {
         order_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_ORDER_LIST, SGE_GDI_ADD,
                                  orders, NULL, NULL, &malp, &state);
      }

      if (alp != NULL) {
         ret = answer_list_handle_request_answer_list(&alp, stderr);
         DEXIT;
         return ret;
      }
   }   

   /* if necessary, set busy state to "not busy" */
   if(set_busy) {
      DPRINTF(("RESETTING BUSY STATE OF EVENT CLIENT\n"));
      ec_set_busy(0);
      ec_commit_multi(&malp, &state);
   }

   /* check result of orders */
   if(order_id > 0) {
      alp = sge_gdi_extract_answer(SGE_GDI_ADD, SGE_ORDER_LIST, order_id, malp, NULL);

      ret = answer_list_handle_request_answer_list(&alp, stderr);
   }

   malp = lFreeList(malp);

   DEXIT;
   return ret;
}



/*--------------------------------------------------------------------
 * build a ORT_remove_job order for each finished job 
 *--------------------------------------------------------------------*/
lList *create_delete_job_orders(
lList *finished_jobs, 
lList *order_list  
) {
   lListElem *job, *ja_task;

   DENTER(TOP_LAYER, "create_delete_job_orders");

   for_each(job, finished_jobs) {
      for_each(ja_task, lGetList(job, JB_ja_tasks)) {
         DPRINTF(("DELETE JOB "u32"."u32"\n", lGetUlong(job, JB_job_number),
            lGetUlong(ja_task, JAT_task_number)));
         order_list = sge_create_orders(order_list, ORT_remove_job, job, 
            ja_task, NULL, false, true);
      }
   }

   DEXIT;
   return order_list;
}



/****** sge_orders/sge_join_orders() ******************************************
*  NAME
*     sge_join_orders() -- generates one order list from the order structure 
*
*  SYNOPSIS
*     lLlist* sge_join_orders(order_t orders) 
*
*  FUNCTION
*      generates one order list from the order structure, and cleans the
*      the order structure. The orders, which have been send allready, are
*      removed.
*
*  INPUTS
*     order_t orders - the order strucutre
*
*  RESULT
*     lLlist* - a order list
*
*  NOTES
*     MT-NOTE: sge_join_orders() is not  safe 
*
*******************************************************************************/
lList *sge_join_orders(order_t *orders){
      lList *orderlist=NULL;
   
      orderlist = orders->configOrderList;
      orders->configOrderList = NULL;
  
      if (orderlist == NULL) {
         orderlist = lCreateList("orderlist", OR_Type);
      }

      lAddList(orderlist, orders->jobStartOrderList);
      orders->jobStartOrderList = NULL; 
      
      lAddList(orderlist, orders->pendingOrderList);
      orders->pendingOrderList= NULL;

      /* they have been send earlier, so we can remove them */
      orders->sent_job_StartOrderList = lFreeList(orders->sent_job_StartOrderList);

      return orderlist;
}


/****** sge_orders/sge_GetNumberOfOrders() *************************************
*  NAME
*     sge_GetNumberOfOrders() -- returns the number of orders generated
*
*  SYNOPSIS
*     int sge_GetNumberOfOrders(order_t *orders) 
*
*  FUNCTION
*     returns the number of orders generated
*
*  INPUTS
*     order_t *orders - a structure of orders
*
*  RESULT
*     int - number of orders in the structure
*
*  NOTES
*     MT-NOTE: sge_GetNumberOfOrders() is  MT safe 
*
*******************************************************************************/
int sge_GetNumberOfOrders(order_t *orders) {
   int count = 0;

   count += lGetNumberOfElem(orders->configOrderList);
   count += lGetNumberOfElem(orders->pendingOrderList);
   count += lGetNumberOfElem(orders->jobStartOrderList);
   count += lGetNumberOfElem(orders->sent_job_StartOrderList);

   return count;
}


/****** sge_orders/sge_send_job_start_orders() *********************************
*  NAME
*     sge_send_job_start_orders() -- sends the job start orders to the master
*
*  SYNOPSIS
*     int sge_send_job_start_orders(order_t *orders) 
*
*  FUNCTION
*     If many jobs are dispatched during one scheduling run, this function
*     can submit the allready generated job start orders to the master, so it
*     can start the jobs and does not need for the scheduling cycle to finish.
*
*     The config orders are submitted as well (queue suspend or unsuspend, job
*     suspend or unsuspend. These orders are deleted in the function and the
*     job start orders are moved to the send_job_StartOrderList.
*
*  INPUTS
*     order_t *orders - the order structure
*
*  RESULT
*     int - STATUS_OK
*
*  NOTES
*     MT-NOTE: sge_send_job_start_orders() is MT safe 
*
*******************************************************************************/
int sge_send_job_start_orders(order_t *orders) {
   int ret = STATUS_OK;
   lList *alp = NULL;
   lList *malp = NULL;
   static bool is_executed_once = false;

   int order_id = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "sge_send_orders2master");

   if (is_executed_once || orders->jobStartOrderList == NULL ) {
      DEXIT;
      return ret;
   }

   is_executed_once = true;

   if (orders->configOrderList != NULL) {

      order_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_ORDER_LIST, SGE_GDI_ADD,
                                  orders->configOrderList, NULL, NULL, &malp, &state);
   }        

   order_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_ORDER_LIST, SGE_GDI_ADD,
                                  orders->jobStartOrderList, NULL, NULL, &malp, &state);

   if (orders->sent_job_StartOrderList == NULL) {
      orders->sent_job_StartOrderList =  orders->jobStartOrderList;
   }
   else {
      lAddList(orders->sent_job_StartOrderList, orders->jobStartOrderList); 
   }
   orders->jobStartOrderList = NULL;
  
   orders->configOrderList = lFreeList(orders->configOrderList);

   if (alp != NULL) {
      ret = answer_list_handle_request_answer_list(&alp, stderr);
      DEXIT;
      return ret;
   }
   else {
      /* check result of orders */
      if(order_id > 0) {
         alp = sge_gdi_extract_answer(SGE_GDI_ADD, SGE_ORDER_LIST, order_id, malp, NULL);

         ret = answer_list_handle_request_answer_list(&alp, stderr);
      }
   }
   malp = lFreeList(malp);

   DEXIT;
   return ret;
}

