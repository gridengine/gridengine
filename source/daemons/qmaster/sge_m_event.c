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
#include <string.h>

#include "sge.h"
#include "cull.h"
#include "sge_eventL.h"
#include "sge_feature.h"
#include "sge_time.h"
#include "sge_m_event.h"
#include "sge_host.h"
#include "sge_pe_qmaster.h"
#include "event.h"
#include "sge_all_listsL.h"
#include "sec.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_conf.h"
#include "sge_security.h"
#include "msg_qmaster.h"
#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

extern lList *Master_Sched_Config_List;
extern lList *Master_Complex_List;
extern lList *Master_Userset_List;
extern lList *Master_Sharetree_List;
extern lList *Master_User_List;
extern lList *Master_Project_List;
extern lList *Master_Queue_List;
extern lList *Master_Job_List;
extern lList *Master_Exechost_List;
extern lList *Master_Ckpt_List;
extern lList *Master_Pe_List;

static void total_update(lListElem *event_client);

static void sge_total_update_event(lListElem *event_client, u_long32 type, lList *lp);

static void sge_add_event_( lListElem *event_client, u_long32 type, 
                     u_long32 intkey, u_long32 intkey2, const char *strkey, 
                     lListElem *element );

static int sge_add_list_event_(lListElem *event_client,
                               u_long32 type, u_long32 intkey, 
                               u_long32 intkey2, const char *strkey, 
                               lList *list, int need_copy_elem);

static void sge_flush_events_(lListElem *event_client, int cmd, int now);
void sge_gdi_kill_eventclient_(lListElem *event_client, const char *host, const char *user, sge_gdi_request *answer);

#define FLUSH_INTERVAL 15

/* Minimum and maximum timeout values when waiting for acknowledge */
#define EVENT_ACK_MIN_TIMEOUT 600
#define EVENT_ACK_MAX_TIMEOUT 1200

static int schedule_interval = FLUSH_INTERVAL;

int last_seq_no = -1; 
int scheduler_busy = 0; 

lList *EV_Clients = NULL;

#ifdef QIDL
u_long32 qidl_event_count = 0;
#endif

/*----------------------------------------------------------------*
 * sge_flush_events
 *    problem of this function is that it assumes only one event client
 *    the event flush interval of this client is a copy of 
 *    the scheduler timeout
 * global variables used:
 *    schedule_interval = a copy of the scheduler interval
 *                        get regularly updated
 *    next_flush        = the next time when events should be flushed
 *    last_flushed      = the time events where flusehd the last time
 *    flush_events      = bool flag which indicates if events should be flushed
 *                        may not be set to 0 in this function
 *    flush_submit_sec  = global variable which says how long after
 *                        a submission events should be flushed
 *    flush_finish_sec  = global variable which says how long after
 *                        a job finish events should be flushed
 *     
 * FLUSH_EVENTS_JOB_FINISHED  
 * FLUSH_EVENTS_JOB_SUBMITTED
 * FLUSH_EVENTS_SET
 *    same as FLUSH_EVENTS_DELAYED, but uses the corresponding variables
 *    "flush_finish_sec" or "flush_submit_sec" (if >=0) or 0 (SET)
 *    instead of scheduling interval
 *
 *----------------------------------------------------------------*/


void sge_flush_events(lListElem *event_client, int cmd) {
   int now = sge_get_gmt();

   if(event_client == NULL) {
      for_each(event_client, EV_Clients) {
         sge_flush_events_(event_client, cmd, now);
      }
   } else {
      sge_flush_events_(event_client, cmd, now);
   }
}
 
void sge_flush_events_(lListElem *event_client, int cmd, int now)
{
   int interval = 0;
   u_long32 next_send;

   next_send = lGetUlong(event_client, EV_next_send_time);
   interval  = lGetUlong(event_client, EV_d_time);

   switch(cmd) {
      case FLUSH_EVENTS_JOB_FINISHED:
         if(flush_finish_sec >= 0) {
            interval = flush_finish_sec;
         }
         break;
         
      case FLUSH_EVENTS_JOB_SUBMITTED:
         if(flush_submit_sec >= 0) {
            interval = flush_submit_sec;
         }
         break;
         
      case FLUSH_EVENTS_SET:
         interval = 0;
         break;
      
      default:
         return;
   }

   next_send = MIN(next_send, now + interval);
   lSetUlong(event_client, EV_next_send_time, next_send);

   DPRINTF(("ev_client: %s\tFLUSH_FINISH: %d FLUSH_SUBMIT: %d NOW: %d NEXT FLUSH: %d\n",
            lGetString(event_client, EV_name), flush_finish_sec, flush_submit_sec, now, next_send)); 
}

/*----------------------------------------------------------------*
 * sge_next_flush
 * return the time when next events should be sent to scheduler
 * weakness of this function is that it only uses the global   
 * stored scheduler interval of the scheduler event client     
 *
 * problem is that we should not return an indication of flushing
 * if the scheduler event client is unknown or has a timeout
 * return
 *    0 in case our event client is unknown or has a timeout
 *    next time time flush otherwise
 *----------------------------------------------------------------*/
int sge_next_flush(
int now 
) {
   int any_client_up = 0;
   int min_next_send = MAX_ULONG32;

   lListElem *event_client;
   
   for_each(event_client, EV_Clients) {
      if(now < (lGetUlong(event_client, EV_last_heard_from) + lGetUlong(event_client, EV_d_time) * 5)) {
         any_client_up = 1;
         min_next_send = MIN(min_next_send, lGetUlong(event_client, EV_next_send_time));
      }
   }
 
   if(any_client_up) {
      return min_next_send;
   }

   return 0;
}   

/*----------------------------------------------------------------*/
void reinit_schedd()
{
   static int reinits = 0;
   lListElem *schedd;

   DENTER(TOP_LAYER, "reinit_schedd");

   if ((schedd=sge_locate_scheduler())) {
      ERROR((SGE_EVENT, MSG_EVE_REINITSCHEDD_I, ++reinits));
      total_update(schedd);
   }

   DEXIT;
}

/* build events for total update of schedd */
static void total_update(
lListElem *event_client 
) {
   DENTER(TOP_LAYER, "total_update");

   sge_add_event(event_client, sgeE_SCHED_CONF, 0, 0, NULL, lFirst(Master_Sched_Config_List));
   sge_total_update_event(event_client, sgeE_QUEUE_LIST, Master_Queue_List);
   sge_total_update_event(event_client, sgeE_USERSET_LIST, Master_Userset_List);
   sge_total_update_event(event_client, sgeE_EXECHOST_LIST, Master_Exechost_List);
   sge_total_update_event(event_client, sgeE_COMPLEX_LIST, Master_Complex_List);
   sge_total_update_event(event_client, sgeE_PE_LIST, Master_Pe_List);
   sge_total_update_event(event_client, sgeE_JOB_LIST, Master_Job_List);

   /* send additional lists for ANY_CLIENT: calendar, admin host, submit host, manager, operators */

   if (feature_is_enabled(FEATURE_SGEEE)) {
      sge_add_event(event_client, sgeE_NEW_SHARETREE, 0, 0, NULL, lFirst(Master_Sharetree_List));
      sge_total_update_event(event_client, sgeE_USER_LIST, Master_User_List);
      sge_total_update_event(event_client, sgeE_PROJECT_LIST, Master_Project_List);
   }

   sge_total_update_event(event_client, sgeE_CKPT_LIST, Master_Ckpt_List);

   DEXIT;
   return;
}

/* 
 
   sge_add_event_client is used by the scheduler 
   for it's initial registration at qmaster AND
   for requesting a total update of the schedulers 
   data in case of a protocol error in the 
   event handling. So there is no difference 
   between add and mod.

   The only difference is a message logging
   to see that the scheduler has reregistered.
   In ideal case there should be no total update
   necessary.

*/ 
int sge_add_event_client(
lListElem *clio,
lList **alpp,
char *ruser,
char *rhost 
) {
   lListElem *ep=NULL;
   u_long32 now;
   u_long32 id;
   const char *name;
   static u_long32 first_dynamic_id = EV_ID_FIRST_DYNAMIC;

   DENTER(TOP_LAYER,"sge_add_event_client");

   id = lGetUlong(clio, EV_id);
   name = lGetString(clio, EV_name);
   if(name == NULL) {
      name = "unnamed";
      lSetString(clio, EV_name, name);
   }
   
   if(id < EV_ID_ANY || id >= EV_ID_FIRST_DYNAMIC) { /* invalid request */
      ERROR((SGE_EVENT, MSG_EVE_ILLEGALIDREGISTERED_U, u32c(id)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if(id == EV_ID_ANY) {   /* qmaster shall give id dynamically */
      id = first_dynamic_id++;
      lSetUlong(clio, EV_id, id);
      INFO((SGE_EVENT, MSG_EVE_REG_SU, name, u32c(id)));         
   }

   if(id == EV_ID_SCHEDD) {
      if ((ep=sge_locate_scheduler())) {
         /* we already have a scheduler */
         ERROR((SGE_EVENT, MSG_EVE_TOTALUPDATE));         

         /* delete old event client entry */
         lRemoveElem(EV_Clients, ep);
      } else {
         INFO((SGE_EVENT, MSG_EVE_REG_SU, name, u32c(id)));         
      }   

      schedule_interval = lGetUlong(clio, EV_d_time);
      scheduler_busy = 0;
      last_seq_no = -1; /* !!!! can this be moved somewhere else? */
      DPRINTF(("EVENT DELIVERY TIME: %d seconds\n", schedule_interval));
   }

   ep=lCopyElem(clio);
   if(!EV_Clients) 
      EV_Clients=lCreateList("EV_Clients", EV_Type); 
   lAppendElem(EV_Clients,ep);
   lSetUlong(ep, EV_next_number, 1);

   /* build events for total update */
   total_update(ep);

   /* register this contact */
   now = sge_get_gmt();
   lSetUlong(ep, EV_last_send_time, 0);
   lSetUlong(ep, EV_next_send_time, now + lGetUlong(ep, EV_d_time));
   lSetUlong(ep, EV_last_heard_from, now);

   /* send it directly */
   sge_flush_events(ep, FLUSH_EVENTS_SET);
   
   INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS,
         ruser, rhost, name, MSG_EVE_EVENTCLIENT));
  
   {
      char buffer[100];
      sprintf(buffer, "registration ok, your id is:"U32CFormat, u32c(id)); /* dont change or I18N: it is parsed in the client! */
      sge_add_answer(alpp, buffer, STATUS_OK, NUM_AN_INFO);
   }

   DEXIT; 
   return STATUS_OK;
}

void sge_event_client_exit(const char *host, const char *commproc, sge_pack_buffer *pb)
{
   u_long32 ec_id;
   lListElem *ec;

   DENTER(TOP_LAYER, "sge_event_client_exit");
   if(unpackint(pb, &ec_id)) {
      ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 1));
      DEXIT;
      return;
   }

   ec = lGetElemUlong(EV_Clients, EV_id, ec_id);

   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_U, u32c(ec_id)));
      DEXIT;
      return;
   }

   INFO((SGE_EVENT, MSG_EVE_UNREG_SU, lGetString(ec, EV_name), u32c(lGetUlong(ec, EV_id))));

   lRemoveElem(EV_Clients, ec);

   DEXIT;
}

int sge_eventclient_subscribed(const lListElem *event_client, int event)
{
   const char *subscription = NULL;

   if(event_client == NULL) {
      return 0;
   }

   subscription = lGetString(event_client, EV_subscription);

   if(subscription == NULL || *subscription == 0) {
      return 0;
   }

   if(subscription[event] == '1') {
      return 1;
   }

   return 0;
}

/* 

   RETURN
   0 normally 
   1 event client needs to be trashed 
     since shutdown event was acked 
 
*/
int sge_ack_event(
lListElem *event_client,
u_long32 event_number 
) {
   int pos;
   lList *lp;
   lListElem *event, *tmp;

   DENTER(TOP_LAYER, "sge_ack_event");

   /* little optimazation */
   pos = lGetPosInDescr(ET_Type, ET_number);
   lp = lGetList(event_client, EV_events);

   /*  
      we delete all messages with lower or 
      equal numbers than event_number
   */
   if (event_number) {
      int trashed = 0;

      event=lFirst(lp);
      while (event) {
         tmp=event;
         event = lNext(event);

         if (lGetPosUlong(tmp, pos) <= event_number) {
            lRemoveElem(lp, tmp);      
            trashed++;
         } else 
            break;
      }
      DPRINTF(("Trashed %d acked events\n", trashed));
   } else
      DPRINTF(("Got empty event ack\n"));


   /* register time of ack */
   lSetUlong(event_client, EV_last_heard_from, sge_get_gmt());
   
   DEXIT;
   return 0;
}


/* --------------------------------------------

   event delivery can be triggerd on two different ways

   - an event is delivered if someone called sge_flush_events(SET)
   - if nobody calls sge_flush_events() the events are delivered
     every EV_d_time (set by event client)

*/
     

void ck_4_deliver_events(
u_long32 now 
) {
   lListElem *report;
   lList *report_list;
   u_long32 timeout;
   lListElem *event_client, *tmp;
   const char *host;
   const char *commproc;
   int ret, id; 
   int deliver_interval;

   DENTER(TOP_LAYER, "ck_4_deliver_events");

   event_client=lFirst(EV_Clients);
   while (event_client) {

      /* extract address of event client */
      host = lGetHost(event_client, EV_host);
      commproc = lGetString(event_client, EV_commproc);
      id = lGetUlong(event_client, EV_commid);

      deliver_interval = lGetUlong(event_client, EV_d_time);

      /* somone turned the clock back */
      if (lGetUlong(event_client, EV_last_heard_from) > now) {
         lSetUlong(event_client, EV_last_heard_from, now);
         lSetUlong(event_client, EV_next_send_time, now + deliver_interval);
      }

      if(lGetUlong(event_client, EV_last_send_time)  > now) {
         lSetUlong(event_client, EV_last_send_time, now);
      }
      
      /* is the ack timeout expired ? */
      timeout = 10*deliver_interval;
      
      if(timeout < EVENT_ACK_MIN_TIMEOUT) {
         timeout = EVENT_ACK_MIN_TIMEOUT;
      }

      if(timeout > EVENT_ACK_MAX_TIMEOUT) {
         timeout = EVENT_ACK_MAX_TIMEOUT;
      }
      
      if (now > (lGetUlong(event_client, EV_last_heard_from) + timeout)) {
         ERROR((SGE_EVENT, MSG_COM_ACKTIMEOUT4EV_ISIS, 
               (int) timeout, commproc, (int) id, host));
         tmp = event_client;
         event_client = lNext(event_client);
         lRemoveElem(EV_Clients, tmp); 
         continue;
      }

#if 0
      if (last_flush > lGetUlong(event_client, EV_last_heard_from)) {
         DPRINTF(("no flush because no ack to last flush yet: now-last_flush: %d -- last_flush-last_heard_from: %d\n",
                 sge_get_gmt() - last_flush, last_flush - lGetUlong(event_client, EV_last_heard_from)));
         event_client = lNext(event_client);
         continue;
      }            
#endif
      
      /* do we have to deliver events ? */
      if ((now >= lGetUlong(event_client, EV_next_send_time))
         && !((lGetUlong(event_client, EV_id) == EV_ID_SCHEDD) && scheduler_busy)) {
      
         /* put only pointer in report - dont copy */
         report_list = lCreateList("report list", REP_Type);
         report = lCreateElem(REP_Type);
         lSetUlong(report, REP_type, NUM_REP_REPORT_EVENTS);
         lSetHost(report, REP_host, me.qualified_hostname);
         lSetList(report, REP_list, lGetList(event_client, EV_events));
         lAppendElem(report_list, report);

            {
               lList *lp;
               int numevents;

               lp = lGetList(event_client, EV_events);
               numevents = lGetNumberOfElem(lp);
               DPRINTF(("Sending %d events (" u32"-"u32 ") to (%s,%s,%d)\n", 
                  numevents, 
                  numevents?lGetUlong(lFirst(lp), ET_number):0,
                  numevents?lGetUlong(lLast(lp), ET_number):0,
                  host, commproc, id));
            }
            ret = sge_send_reports(host, commproc, id, report_list, 0, NULL);

            if (ret == 0) { /* otherwise retry is triggered */
               if(lGetUlong(event_client, EV_id) == EV_ID_SCHEDD) {
                  scheduler_busy = 1;
               }   
               now = sge_get_gmt();
               lSetUlong(event_client, EV_last_send_time, now);
               lSetUlong(event_client, EV_next_send_time, now + deliver_interval);
            }


         /* don't delete sent events - deletion is triggerd by ack's */
         {
            lList *lp = NULL;

            lXchgList(report, REP_list, &lp);
         }
         lFreeList(report_list);

      }
      event_client = lNext(event_client);
   }

   DEXIT;
   return;
}

void sge_add_list_event(
lListElem *event_client,
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
const char *strkey,
lList *list 
) {
   if(event_client != NULL) {
      if(sge_eventclient_subscribed(event_client, type)) {
         sge_add_list_event_(event_client, type, intkey, intkey2, strkey, list, 1);
      }
   } else {
      for_each (event_client, EV_Clients) {
         if(sge_eventclient_subscribed(event_client, type)) {
            sge_add_list_event_(event_client, type, intkey, intkey2, strkey, list, 1);
         }
      }
   }
}

void sge_add_event(
lListElem *event_client,
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
const char *strkey,
lListElem *element 
) {

   DENTER(TOP_LAYER, "sge_add_event"); 
   
#ifndef QIDL
   if (!EV_Clients) {
      DEXIT;
      return;
   }
#endif

   if(event_client != NULL) {
      if(sge_eventclient_subscribed(event_client, type)) {
         sge_add_event_(event_client, type, intkey, intkey2, strkey, element);
      }   
   } else {
      for_each(event_client, EV_Clients) {
         if(sge_eventclient_subscribed(event_client, type)) {
            sge_add_event_(event_client, type, intkey, intkey2, strkey, element);
         }
      }
   }

   DEXIT;
}

static void sge_add_event_(
lListElem *event_client,
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
const char *strkey,
lListElem *element 
) {
   const lDescr *dp;
   lList *lp = NULL;

   /* build a list from the element */
   if (element) {
      dp = lGetElemDescr(element);
      lp = lCreateList("changed element", dp);
      lAppendElem(lp, lCopyElem(element));
   }

   if (!sge_add_list_event_(event_client, type, intkey, intkey2, strkey, lp, 0)) {
      lp = lFreeList(lp); 
   }

   return;
}

u_long32 sge_get_next_event_number(u_long32 client_id) {
   lListElem *event_client;
   u_long32 ret = 0;

   DENTER(TOP_LAYER, "sge_get_next_event_number");

   event_client = lGetElemUlong(EV_Clients, EV_id, client_id);
   if(event_client != NULL) {
      ret = lGetUlong(event_client, EV_next_number);
   }

   DEXIT;
   return ret;
}

static int sge_add_list_event_(
lListElem *event_client,
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
const char *strkey,
lList *list,
int need_copy_list  /* to reduce overhead */ 
) {
   lListElem *event;
   u_long32 i;
   lList *lp;
   int consumed = 0;

   DENTER(TOP_LAYER, "sge_add_list_event_"); 

   event = lCreateElem(ET_Type); 

   /* 
      fill in event number and increment 
      EV_next_number of event recipient 
   */
   i = lGetUlong(event_client, EV_next_number);

   lSetUlong(event, ET_number, i++);
   lSetUlong(event_client, EV_next_number, i);

   lSetUlong(event, ET_type, type); 
   lSetUlong(event, ET_intkey, intkey); 
   lSetUlong(event, ET_intkey2, intkey2); 
   lSetString(event, ET_strkey, strkey);
  
   lSetList(event, ET_new_version, 
            need_copy_list?lCopyList(lGetListName(list), list):list);
   need_copy_list = 1;
   consumed = 1;

   /* build a new event list if not exists */
   lp = lGetList(event_client, EV_events); 
   if (!lp) {
      lp=lCreateList("", ET_Type);
      lSetList(event_client, EV_events, lp);
   }

   /* chain in new event */
   lAppendElem(lp, event);

   DPRINTF((event_text(event)));

#ifdef QIDL
   /* send CORBA event */
   /* the supplied list usually contains exactly one element
   ** except for sgeE_JOB_USAGE, and sgeE_FINAL_USAGE, where there
   ** may be more. So I increase the event counter by 1 and send
   ** only lFirst(list).
   ** if the usage lists should be sent via CORBA events, too, it is
   ** probably necessary to add a sub-counter for these cases
   */
   
   qidl_event_count++;
   
   switch (type) {
   /* -------------------- */
   case sgeE_JOB_DEL:
      deleteObjectByID(SGE_JOB_LIST, intkey);
      break;
   case sgeE_JOB_ADD:
      addObjectByID(SGE_JOB_LIST, intkey);
      break;
   case sgeE_JOB_MOD:
      job_changed(lFirst(list));
      break;
   case sgeE_JOB_LIST:
      break;
   case sgeE_JOB_MOD_SCHED_PRIORITY:
      break;
   case sgeE_JOB_USAGE:
      break;
   case sgeE_FINAL_USAGE:
      deleteObjectByID(SGE_JOB_LIST, intkey);
      break;

   /* -------------------- */
   case sgeE_QUEUE_DEL:
      deleteObjectByName(SGE_QUEUE_LIST, strkey);
      break;
   case sgeE_QUEUE_ADD:
      /* cannot be handled here since the cull list */
      /* does not exist at this point */
      /* handled in sge_queue_qmaster.c */
      break;
   case sgeE_QUEUE_MOD:
      queue_changed(lFirst(list));
      break;
   case sgeE_QUEUE_LIST:
      break;
   case sgeE_QUEUE_UNSUSPEND_ON_SUB:
      break;
   case sgeE_QUEUE_SUSPEND_ON_SUB:
      break;

   /* -------------------- */
   case sgeE_COMPLEX_DEL:
      deleteObjectByName(SGE_COMPLEX_LIST, lFirst(list) ? lGetString(lFirst(list),CX_name) : strkey);
      break;
  case sgeE_COMPLEX_ADD:
      /* done by handle generic gdi object */
      break;
   case sgeE_COMPLEX_MOD:
      if (lFirst(list))
         complex_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_COMPLEX_LIST:
      break;

   /* -------------------- */
   case sgeE_EXECHOST_DEL:
      deleteObjectByName(SGE_EXECHOST_LIST, lFirst(list) ? lGetHost(lFirst(list),EH_name) : strkey);
   case sgeE_EXECHOST_ADD:
      /* done by handle generic gdi object */
      break;
   case sgeE_EXECHOST_MOD:
      /* exechost_changed(lFirst(list)); */
      break;
   case sgeE_EXECHOST_LIST:
      break;

   /* -------------------- */
   case sgeE_USERSET_DEL:
      deleteObjectByName(SGE_USERSET_LIST, lFirst(list) ? lGetString(lFirst(list),US_name) : strkey);
      break;
   case sgeE_USERSET_ADD:
      addObjectByName(SGE_USERSET_LIST, lFirst(list) ? lGetString(lFirst(list),US_name) : strkey);
      break;
   case sgeE_USERSET_MOD:
      if (lFirst(list))
         userset_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_USERSET_LIST:
      break;

   /* -------------------- */
   case sgeE_USER_DEL:
      deleteObjectByName(SGE_USER_LIST, lFirst(list) ? lGetString(lFirst(list),UP_name) : strkey);
      break;
   case sgeE_USER_ADD:
      /* done by handle generic gdi object */
      break;
   case sgeE_USER_MOD:
      if (lFirst(list))
         user_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_USER_LIST:
      break;

   /* -------------------- */
   case sgeE_PROJECT_DEL:
      deleteObjectByName(SGE_PROJECT_LIST, lFirst(list) ? lGetString(lFirst(list),UP_name) : strkey);
      break;
   case sgeE_PROJECT_ADD:
      /* done by handle generic gdi object */
      break;
   case sgeE_PROJECT_MOD:
      if (lFirst(list))
         project_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_PROJECT_LIST:
      break;

   /* -------------------- */
   case sgeE_PE_DEL:
      deleteObjectByName(SGE_PE_LIST, lFirst(list) ? lGetString(lFirst(list),PE_name) : strkey);
      break;
   case sgeE_PE_ADD:
      addObjectByName(SGE_PE_LIST, lFirst(list) ? lGetString(lFirst(list),PE_name) : strkey);
      break;
   case sgeE_PE_MOD:
      if (lFirst(list))
         parallelenvironment_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_PE_LIST:
      break;

   /* -------------------- */
   case sgeE_SHUTDOWN:
      break;
   case sgeE_QMASTER_GOES_DOWN:
      break;
   case sgeE_SCHEDDMONITOR:
      break;

   /* -------------------- */
   case sgeE_NEW_SHARETREE:
      if (lFirst(list))
         sharetree_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;

   /* -------------------- */
   case sgeE_SCHED_CONF:
      break;

   /* -------------------- */
   case sgeE_CKPT_DEL:
      deleteObjectByName(SGE_CKPT_LIST, lFirst(list) ? lGetString(lFirst(list),CK_name) : strkey);
      break;
   case sgeE_CKPT_ADD:
      addObjectByName(SGE_CKPT_LIST, lFirst(list) ? lGetString(lFirst(list),CK_name) : strkey);
      break;
   case sgeE_CKPT_MOD:
      if (lFirst(list))
      checkpoint_changed(lFirst(list));
      else 
         DPRINTF(("first element is NULL"));
      break;
   case sgeE_CKPT_LIST:
      break;

   default:
      break;
   }
#endif

   DEXIT;
   return EV_Clients?consumed:1;
}



static void sge_total_update_event(
lListElem *event_client,
u_long32 type,
lList *lp 
) {
   u_long32 i;
   lListElem *event;

   DENTER(TOP_LAYER, "sge_total_update_event");

   if(sge_eventclient_subscribed(event_client, type)) {
      event = lCreateElem(ET_Type); 

      /* fill in event number and increment EV_next_number of event recipient */
      i = lGetUlong(event_client, EV_next_number);
      lSetUlong(event, ET_number, i++);
      lSetUlong(event_client, EV_next_number, i);
      
      lSetUlong(event, ET_type, type); 

      lSetList(event, ET_new_version, lCopyList("updating list", lp));

      /* build a new event list if not exists */
      lp = lGetList(event_client, EV_events); 
      if (!lp) {
         lp=lCreateList("", ET_Type);
         lSetList(event_client, EV_events, lp);
      }

      DPRINTF((event_text(event)));
      /* chain in new event */
      lAppendElem(lp, event);
   }

   DEXIT;
   return;
}


lListElem* sge_locate_scheduler()
{
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_locate_scheduler");

   ep = lGetElemUlong(EV_Clients, EV_id, EV_ID_SCHEDD);

   DEXIT;
   return ep;
}

void sge_gdi_kill_eventclient(const char *host, sge_gdi_request *request, sge_gdi_request *answer) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   lListElem *idep, *event_client;

   DENTER(GDI_LAYER, "sge_gdi_kill_eventclient");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (sge_manager(user)) {
      ERROR((SGE_EVENT, MSG_COM_NOSHUTDOWNPERMS));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   for_each(idep, request->lp) {
      int id;
      const char *idstr = lGetString(idep, ID_str);

      if(idstr == NULL) {
         id = EV_ID_ANY;
      } else {
         char *dptr;
         id = strtol(idstr, &dptr, 0); 
         if(dptr == idstr) {
            ERROR((SGE_EVENT, MSG_EVE_ILLEGALEVENTCLIENTID_S, idstr));
            sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_EEXIST, 0);
            continue;
         }
      }

      if(id == EV_ID_ANY) {
         /* kill all event clients except schedd */
         for_each(event_client, EV_Clients) {
            if(lGetUlong(event_client, EV_id) >= EV_ID_FIRST_DYNAMIC) {
               sge_gdi_kill_eventclient_(event_client, host, user, answer);
            }
         }
      } else {
         event_client = lGetElemUlong(EV_Clients, EV_id, id);
         if(event_client == NULL) {
            ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_U, u32c(id)));
            sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_EEXIST, 0);
            continue;
         }
         sge_gdi_kill_eventclient_(event_client, host, user, answer);
      }
   }
}
   
void sge_gdi_kill_eventclient_(lListElem *event_client, const char *host, const char *user, sge_gdi_request *answer) 
{
   DENTER(GDI_LAYER, "sge_gdi_kill_eventclient_");
   /* add a sgeE_SHUTDOWN event */
   sge_add_event(event_client, sgeE_SHUTDOWN, 0, 0, NULL, NULL);
   sge_flush_events(event_client, FLUSH_EVENTS_SET);

   INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, user, host, lGetString(event_client, EV_name)));
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
}

void sge_gdi_tsm(char *host, sge_gdi_request *request, sge_gdi_request *answer) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   lListElem *scheduler;

   DENTER(GDI_LAYER, "sge_gdi_tsm");

   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (sge_manager(user)) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDMONPERMS));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, NUM_AN_WARNING);
      DEXIT;
      return;
   }
     
   scheduler = sge_locate_scheduler();
     
   if (scheduler == NULL) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_WARNING);
      DEXIT;
      return;
   }

   /* add a sgeE_SCHEDDMONITOR event */
   sge_add_event(scheduler, sgeE_SCHEDDMONITOR, 0, 0, NULL, NULL);
   sge_flush_events(scheduler, FLUSH_EVENTS_SET);

   INFO((SGE_EVENT, MSG_COM_SCHEDMON_SS, user, host));
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
}
