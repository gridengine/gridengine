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


lList *sge_add_schedd_info(lList *or_list) 
{
   lList *jlist;
   lListElem *sme, *ep;

   DENTER(TOP_LAYER, "sge_add_schedd_info");

   sme = schedd_mes_obtain_package();
   if (!sme || (lGetNumberOfElem(lGetList(sme, SME_message_list))<1 
         && lGetNumberOfElem(lGetList(sme, SME_global_message_list))<1)) {
      DEXIT;
      return or_list;
   }
   
   /* create orders list if not existent */
   if (!or_list)
      or_list = lCreateList("orderlist", OR_Type);

   /* build order */
   ep=lCreateElem(OR_Type);   
   
   jlist = lCreateList("", SME_Type);
   lAppendElem(jlist, sme);
   lSetList(ep, OR_joker, jlist);
   
   lSetUlong(ep, OR_seq_no, get_seq_nr());
   lSetUlong(ep, OR_type, ORT_job_schedd_info);
   lAppendElem(or_list, ep);

   DEXIT;
   return or_list;
}


int get_seq_nr()
{
   static int seq_nr=1;
  
   return seq_nr++;
}
/*************************************************************
 Create a new order-list or add orders to an existing one.
 or_list==NULL -> create new one.
 or_list!=NULL -> append orders.
 returns updated order-list.

 The granted list contains granted queues.
 *************************************************************/

lList *sge_create_orders(
lList *or_list,
u_long32 type,
lListElem *job,
lListElem *ja_task,
lList *granted ,
bool no_tickets
) {
   static lEnumeration *tix_what = NULL;
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
   if (!or_list) 
      or_list = lCreateList("orderlist", OR_Type);

   /* build sublist of granted */
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
         lSetDouble(ep2, OQ_dticket, lGetDouble(gel, JG_dticket));
         lSetDouble(ep2, OQ_sticket, lGetDouble(gel, JG_sticket));
         
         if (!ql)
            ql=lCreateList("orderlist",OQ_Type);
         lAppendElem(ql, ep2);
      }
   }

   /* build order */
   /* OR_force is set to zero */
   ep=lCreateElem(OR_Type);
   if (feature_is_enabled(FEATURE_SGEEE)) {
      if(!no_tickets)
         lSetDouble(ep, OR_ticket, lGetDouble(ja_task, JAT_ticket));
      else
         lSetDouble(ep, OR_ticket, 0.0);

      if (type == ORT_tickets|| type == ORT_ptickets) {
         lListElem *jep;
         lList *jlist = lCreateList("", lGetElemDescr(job));
         lList *tlist;

         /* Create a reduced task list with only the required fields */

         tlist = lCreateList("", lGetElemDescr(ja_task));
         lAppendElem(tlist, lCopyElem(ja_task));
         {             
            lList *tmp_list;
               if(!tix_what)
                  tix_what = lWhat("%T(%I %I %I %I %I %I %I %I %I)",
                  lGetElemDescr(ja_task), 
                  JAT_task_number, 
                  JAT_status, 
                  JAT_ticket,
                  JAT_oticket, 
                  JAT_dticket, 
                  JAT_fticket, 
                  JAT_sticket, 
                  JAT_share,
                  JAT_granted_destin_identifier_list);

            if ((tmp_list = lSelect("", tlist, NULL, tix_what))) {
               lFreeList(tlist);
               tlist = tmp_list;
               if(no_tickets){
                  lListElem *tmp;
                  for_each(tmp, tlist){                 
                     lSetDouble(tmp, JAT_ticket, 0.0);
                     lSetDouble(tmp, JAT_oticket, 0.0);
                     lSetDouble(tmp, JAT_dticket, 0.0);
                     lSetDouble(tmp, JAT_fticket, 0.0);
                     lSetDouble(tmp, JAT_sticket, 0.0);
                     lSetDouble(tmp, JAT_share, 0.0);
                  }
               }
            }
         }

         /* Create a reduced job list with only the required fields */

         jep = lCreateElem(lGetElemDescr(job));
         lSetUlong(jep, JB_job_number, lGetUlong(job, JB_job_number));
         lSetList(jep, JB_ja_tasks, tlist);
         lAppendElem(jlist, jep);
         {        
            lList *tmp_list;
            if(!job_what)
               job_what = lWhat("%T(%I %I)", 
                  lGetElemDescr(job), 
                  JB_job_number, 
                  JB_ja_tasks);
            if ((tmp_list = lSelect("", jlist, NULL, job_what))) {
               lFreeList(jlist);
               jlist = tmp_list;
            }
         }

         lSetList(ep, OR_joker, jlist);
      }
   }
   
   lSetUlong(ep, OR_seq_no, get_seq_nr());
   lSetUlong(ep, OR_type, type);
   lSetUlong(ep, OR_job_number, lGetUlong(job, JB_job_number));
   lSetUlong(ep, OR_ja_task_number, lGetUlong(ja_task, JAT_task_number));
   lSetUlong(ep, OR_job_version, lGetUlong(job, JB_version));
   lSetList(ep, OR_queuelist, ql);
   {
      const char *s;

      s = lGetString(ja_task, JAT_granted_pe);
      if (s)
         lSetString(ep, OR_pe, s);
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

   for_each(job, finished_jobs) {
      for_each(ja_task, lGetList(job, JB_ja_tasks)) {
         DPRINTF(("DELETE JOB "u32"."u32"\n", lGetUlong(job, JB_job_number),
            lGetUlong(ja_task, JAT_task_number)));
         order_list = sge_create_orders(order_list, ORT_remove_job, job, 
            ja_task, NULL, false);
      }
   }

   return order_list;
}




