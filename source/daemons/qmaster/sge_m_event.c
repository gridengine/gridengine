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
 *  License at http://www.gridengine.sunsource.net/license.html
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

static void total_update_schedd(lListElem *schedd);
static void sge_total_update_event(lListElem *er, u_long32 type, lList *lp);
static int sge_add_list_event_(u_long32 type, u_long32 intkey, u_long32 intkey2, char *strkey, lList *list, int need_copy_elem);

#define FLUSH_INTERVAL 15

/* Minimum and maximum timeout values when waiting for acknowledge */
#define EVENT_ACK_MIN_TIMEOUT 600
#define EVENT_ACK_MAX_TIMEOUT 1200

static int schedule_interval = FLUSH_INTERVAL;
static int flush_events = 0;
static u_long32 last_flush = 0;
static u_long32 next_flush = 0;

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
 * FLUSH_EVENTS_DELAYED
 *    sets "next_flush" to MIN() of next_flush or next scheduling interval
 *    returns 1 if "now" is time to flush events or 
 *             "flush_events" is already set to 1
 *     
 * FLUSH_EVENTS_JOB_FINISHED  
 * FLUSH_EVENTS_JOB_SUBMITTED
 * FLUSH_EVENTS_SET
 *    same as FLUSH_EVENTS_DELAYED, but uses the corresponding variables
 *    "flush_finish_sec" or "flush_submit_sec" (if >=0) or 0 (SET)
 *    instead of scheduling interval
 *
 * FLUSH_ASK_NEXT
 *    return next time when events should be flushed
 *----------------------------------------------------------------*/
int sge_flush_events(
int cmd 
) {
   int interval;
   int now;
   int ret;
   
   switch(cmd) {
   case FLUSH_EVENTS_DELAYED:
   case FLUSH_EVENTS_JOB_FINISHED:
   case FLUSH_EVENTS_JOB_SUBMITTED:
   case FLUSH_EVENTS_SET:
   case FLUSH_ASK_NEXT:

      interval = schedule_interval;
      now = sge_get_gmt();
      
      /* someone has turned the wclock back */
      if (last_flush > now)
         last_flush = now;
      
      if (cmd == FLUSH_EVENTS_JOB_FINISHED && flush_finish_sec > -1)
         interval = flush_finish_sec;
      else if (cmd == FLUSH_EVENTS_JOB_SUBMITTED && flush_submit_sec > -1)
         interval = flush_submit_sec;
      else if (cmd == FLUSH_EVENTS_SET)
         interval = 0;   
         
      if (cmd != FLUSH_ASK_NEXT) {   
         if (next_flush)
            next_flush = MIN(next_flush, last_flush + interval);
         else
            next_flush = last_flush + interval;
      }
            
      if ((cmd == FLUSH_EVENTS_SET) || (now >= next_flush))
         flush_events = 1;         
               
      DPRINTF(("FLUSH_FINISH: %d FLUSH_SUBMIT: %d  DO_FLUSH: %d\n",
             flush_finish_sec, flush_submit_sec, flush_events)); 
             
      if (cmd == FLUSH_ASK_NEXT) {
         if (flush_events) {
            ret = now;
         } else {
            ret = next_flush;   
         }
      } else {
         ret = flush_events;
      }
      break;
   default:
      ret = -1;
      break;
   }
   return ret;
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
   lListElem *er;
   
   if (!(er =lFirst(EV_Clients)) ||
       (now > (lGetUlong(er, EV_lt_heard_from) + lGetUlong(er, EV_d_time)*5)))
      return 0;
   else
      return sge_flush_events(FLUSH_ASK_NEXT);
}   

/*----------------------------------------------------------------*/
void reinit_schedd()
{
   static int reinits = 0;
   lListElem *schedd;

   DENTER(TOP_LAYER, "reinit_schedd");

   if ((schedd=sge_locate_scheduler())) {
      ERROR((SGE_EVENT, MSG_EVE_REINITSCHEDD_I, ++reinits));
      total_update_schedd(schedd);
   }

   DEXIT;
}

/* build events for total update of schedd */
static void total_update_schedd(
lListElem *schedd 
) {
   DENTER(TOP_LAYER, "total_update_schedd");

   sge_add_event(sgeE_SCHED_CONF, 0, 0, NULL, lFirst(Master_Sched_Config_List));
   sge_total_update_event(schedd, sgeE_QUEUE_LIST, Master_Queue_List);
   sge_total_update_event(schedd, sgeE_USERSET_LIST, Master_Userset_List);
   sge_total_update_event(schedd, sgeE_EXECHOST_LIST, Master_Exechost_List);
   sge_total_update_event(schedd, sgeE_COMPLEX_LIST, Master_Complex_List);
   sge_total_update_event(schedd, sgeE_PE_LIST, Master_Pe_List);
   sge_total_update_event(schedd, sgeE_JOB_LIST, Master_Job_List);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      sge_add_event(sgeE_NEW_SHARETREE, 0, 0, NULL, lFirst(Master_Sharetree_List));
      sge_total_update_event(schedd, sgeE_USER_LIST, Master_User_List);
      sge_total_update_event(schedd, sgeE_PROJECT_LIST, Master_Project_List);
   }

   sge_total_update_event(schedd, sgeE_CKPT_LIST, Master_Ckpt_List);

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
   u_long32 i, now;

   DENTER(TOP_LAYER,"sge_add_event_client");

   if ( (i=lGetUlong(clio, EV_clienttype))!= TYPE_SCHED) {
         ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENTTYPE_I, (int)i));         
         sge_add_answer(alpp,SGE_EVENT,STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
   }

   if ((ep=sge_locate_scheduler())) {

      /* we already have a scheduler */
      ERROR((SGE_EVENT, MSG_EVE_TOTALUPDATE));         

      /* delete old event client entry */
      lRemoveElem(EV_Clients, ep);
   }
   else
      INFO((SGE_EVENT, MSG_EVE_SCHEDDREG));         

   schedule_interval = lGetUlong(clio, EV_d_time);
   scheduler_busy = 0;
   last_seq_no = -1;

   DPRINTF(("EVENT DELIVERY TIME: %d seconds\n", schedule_interval));

   ep=lCopyElem(clio);
   if(!EV_Clients) 
      EV_Clients=lCreateList("EV_Clients", EV_Type); 
   lAppendElem(EV_Clients,ep);
   lSetUlong(ep, EV_next_number, 1);

   /* build events for total update */
   total_update_schedd(ep);

   /* send it directly */
   sge_flush_events(FLUSH_EVENTS_SET);
   
   /* register this contact */
   now = sge_get_gmt();
   lSetUlong(ep, EV_next_send_time, now + schedule_interval);
   lSetUlong(ep, EV_lt_heard_from, now);

   INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS,
         ruser, rhost, rhost, MSG_EVE_EVENTCLIENT));
   
   sge_add_answer(alpp,SGE_EVENT,STATUS_OK, NUM_AN_INFO);

   DEXIT; 
   return STATUS_OK;
}

/* 

   RETURN
   0 normally 
   1 event client needs to be trashed 
     since shutdown event was acked 
 
*/
int sge_ack_event(
lListElem *er,
u_long32 event_number 
) {
   int pos;
   lList *lp;
   lListElem *event, *tmp;

   DENTER(TOP_LAYER, "sge_ack_event");

   /* little optimazation */
   pos = lGetPosInDescr(ET_Type, ET_number);
   lp = lGetList(er, EV_events);

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
   lSetUlong(er, EV_lt_heard_from, sge_get_gmt());

   
   if (lGetUlong(er, EV_shutdown_event) != 0 && 
      lGetUlong(er, EV_shutdown_event)<=event_number) {
      INFO((SGE_EVENT, MSG_COM_ACK4SHUTDOWN));
      DEXIT;
      return 1;
   }
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
   lListElem *er, *tmp;
   char *host, *commproc;
   int ret, id; 

   DENTER(TOP_LAYER, "ck_4_deliver_events");

   er=lFirst(EV_Clients);
   while (er) {

      /* extract address of event client */
      host = lGetString(er, EV_host);
      commproc = lGetString(er, EV_commproc);
      id = lGetUlong(er, EV_id);

      /* update global variable schedule_interval */
      schedule_interval = lGetUlong(er, EV_d_time);

      /* somone turned the clock back */
      if (lGetUlong(er, EV_lt_heard_from) > now) {
         lSetUlong(er, EV_lt_heard_from, now);
         lSetUlong(er, EV_next_send_time, now + schedule_interval);
      }   
      if (last_flush > now)
         last_flush = now;
      
      /* is the ack timeout expired ? */
      timeout = 10*schedule_interval;
      
      if(timeout < EVENT_ACK_MIN_TIMEOUT) {
         timeout = EVENT_ACK_MIN_TIMEOUT;
      }

      if(timeout > EVENT_ACK_MAX_TIMEOUT) {
         timeout = EVENT_ACK_MAX_TIMEOUT;
      }

      if (now > (lGetUlong(er, EV_lt_heard_from) + timeout)) {
         ERROR((SGE_EVENT, MSG_COM_ACKTIMEOUT4EV_ISIS, 
               (int) timeout, commproc, (int) id, host));
         tmp = er;
         er = lNext(er);
         lRemoveElem(EV_Clients, tmp); 
         continue;
      }

#if 0
      if (last_flush > lGetUlong(er, EV_lt_heard_from)) {
         DPRINTF(("no flush because no ack to last flush yet: now-last_flush: %d -- last_flush-lt_heard_from: %d\n",
                 sge_get_gmt() - last_flush, last_flush - lGetUlong(er, EV_lt_heard_from)));
         er = lNext(er);
         continue;
      }            
#endif
      
      /* do we have to deliver events ? */
      if ((flush_events ||
         (next_flush && (now >= next_flush)) ||
         (now >= lGetUlong(er, EV_next_send_time)))
         && !scheduler_busy) {
      
         /* put only pointer in report - dont copy */
         report_list = lCreateList("report list", REP_Type);
         report = lCreateElem(REP_Type);
         lSetUlong(report, REP_type, NUM_REP_REPORT_EVENTS);
         lSetString(report, REP_host, me.qualified_hostname);
         lSetList(report, REP_list, lGetList(er, EV_events));
         lAppendElem(report_list, report);

         switch ( lGetUlong(er, EV_clienttype)) {

         case TYPE_SCHED:
            {
               lList *lp;
               int numevents;

               lp = lGetList(er, EV_events);
               numevents = lGetNumberOfElem(lp);
               DPRINTF(("Sending %d events (" u32"-"u32 ") to (%s,%s,%d)%s\n", 
                  numevents, 
                  numevents?lGetUlong(lFirst(lp), ET_number):0,
                  numevents?lGetUlong(lLast(lp), ET_number):0,
                  host, commproc, id,
                  flush_events?" flush":""));
            }
            ret = sge_send_reports(host, commproc, id, report_list, 0, NULL);

            if (ret == 0) { /* otherwise retry is triggered */
               scheduler_busy = 1;
               flush_events = 0;
            }
            break;

         default:
            DPRINTF(("EV_Clients contains other types than TYPE_SCHED in ck_4_deliver_events()\n"));
            break;
         }
  
         scheduler_busy = 1; 
         flush_events = 0;

         /* don't delete sent events - deletion is triggerd by ack's */
         {
            lList *lp = NULL;

            lXchgList(report, REP_list, &lp);
         }
         lFreeList(report_list);

         last_flush = sge_get_gmt();
         lSetUlong(er, EV_next_send_time, last_flush + schedule_interval);
         next_flush = last_flush + schedule_interval;
         
      }
      er = lNext(er);
   }

   DEXIT;
   return;
}

void sge_add_list_event(
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
char *strkey,
lList *list 
) {
   sge_add_list_event_(type, intkey, intkey2, strkey, list, 1);
}

void sge_add_event(
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
char *strkey,
lListElem *element 
) {
   const lDescr *dp;
   lList *lp = NULL;

   DENTER(TOP_LAYER, "sge_add_event"); 
   
#ifndef QIDL
   if (!EV_Clients) {
      DEXIT;
      return;
   }
#endif

   /* build a list from the element */
   if (element) {
      dp = lGetElemDescr(element);
      lp = lCreateList("changed element", dp);
      lAppendElem(lp, lCopyElem(element));
   }

   if (!sge_add_list_event_(type, intkey, intkey2, strkey, lp, 0)) {
      lp = lFreeList(lp);
   }

   DEXIT;
   return;
}

u_long32 sge_get_next_event_number(u_long32 client_type) {
   lListElem *er; /* event recipient */
   u_long32 ret = 0;

   DENTER(TOP_LAYER, "sge_get_next_event_number");
   for_each (er, EV_Clients) {
      if (lGetUlong(er, EV_clienttype) == client_type) {
         ret = lGetUlong(er, EV_next_number);
         break;
      }
   }
   DEXIT;
   return ret;
}

static int sge_add_list_event_(
u_long32 type,
u_long32 intkey,
u_long32 intkey2,
char *strkey,
lList *list,
int need_copy_list  /* to reduce overhead */ 
) {
   lListElem *event;
   lListElem *er; /* event recipient */
   u_long32 i;
   lList *lp;
   int consumed = 0;

   DENTER(TOP_LAYER, "sge_add_list_event_"); 

   /* 
      Here we have to loop over all event clients 
      that have to get this event
      actually only the schedd event client gets 
      events.
   */
   for_each (er,EV_Clients) {

      switch ( lGetUlong(er, EV_clienttype)) {
      case TYPE_SCHED:
         event = lCreateElem(ET_Type); 

         /* 
            fill in event number and increment 
            EV_next_number of event recipient 
         */
         i = lGetUlong(er, EV_next_number);

         /* register when to delete event client 
            has to be checked when event ack arrives */
         if (type == sgeE_SCHEDDDOWN) 
            lSetUlong(er, EV_shutdown_event, i); 

         lSetUlong(event, ET_number, i++);
         lSetUlong(er, EV_next_number, i);

         lSetUlong(event, ET_type, type); 
         lSetUlong(event, ET_intkey, intkey); 
         lSetUlong(event, ET_intkey2, intkey2); 
         lSetString(event, ET_strkey, strkey);
        
         lSetList(event, ET_new_version, 
                  need_copy_list?lCopyList(lGetListName(list), list):list);
         need_copy_list = 1;
         consumed = 1;

         /* build a new event list if not exists */
         lp = lGetList(er, EV_events); 
         if (!lp) {
            lp=lCreateList("", ET_Type);
            lSetList(er, EV_events, lp);
         }

         /* chain in new event */
         lAppendElem(lp, event);

         DPRINTF((event_text(event)));


         break; 

      default:
         DPRINTF(("Oops! EV_Clients contains other types "
               "than TYPE_SCHED in sge_add_event()\n"));
         break;
      }
   }

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
      deleteObjectByName(SGE_EXECHOST_LIST, lFirst(list) ? lGetString(lFirst(list),EH_name) : strkey);
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
   case sgeE_SCHEDDDOWN:
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
lListElem *er,
u_long32 type,
lList *lp 
) {
   u_long32 i;
   lListElem *event;

   DENTER(TOP_LAYER, "sge_total_update_event");

   event = lCreateElem(ET_Type); 

   /* fill in event number and increment EV_next_number of event recipient */
   i = lGetUlong(er, EV_next_number);
   lSetUlong(event, ET_number, i++);
   lSetUlong(er, EV_next_number, i);
   
   lSetUlong(event, ET_type, type); 

   lSetList(event, ET_new_version, lCopyList("updating list", lp));

   /* build a new event list if not exists */
   lp = lGetList(er, EV_events); 
   if (!lp) {
      lp=lCreateList("", ET_Type);
      lSetList(er, EV_events, lp);
   }

   DPRINTF((event_text(event)));
   /* chain in new event */
   lAppendElem(lp, event);

   DEXIT;
   return;
}


lListElem* sge_locate_scheduler()
{
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_locate_scheduler");

   /* seek for scheduler in list of EV_Clients */
   for_each(ep, EV_Clients) {
      if (lGetUlong(ep,EV_clienttype)==TYPE_SCHED) {
         DEXIT;
         return ep;
      }
   }
   DEXIT;
   return NULL;
}

void sge_gdi_kill_sched(char *host, sge_gdi_request *request, sge_gdi_request *answer) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(GDI_LAYER, "sge_gdi_kill_sched");

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

   if (!sge_locate_scheduler()) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_WARNING);
      DEXIT;
      return;
   }

   /* add a sgeE_SCHEDDDOWN event */
   sge_add_event(sgeE_SCHEDDDOWN, 0, 0, NULL, NULL);
   sge_flush_events(FLUSH_EVENTS_SET);

   INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, user, host, prognames[SCHEDD]));
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
}

void sge_gdi_dummy_request(
char *host,
sge_gdi_request *request,
sge_gdi_request *answer 
) {
   DENTER(GDI_LAYER, "sge_gdi_dummy_request");
  
   WARNING((SGE_EVENT, MSG_EVE_UNKNOWNDUMMYREQUEST));         
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, NUM_AN_WARNING);
   DEXIT;
   return;
}



void sge_gdi_tsm(char *host, sge_gdi_request *request, sge_gdi_request *answer) 
{
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

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
      
   if (!sge_locate_scheduler()) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_WARNING);
      DEXIT;
      return;
   }

   /* add a sgeE_SCHEDDDOWN event */
   sge_add_event(sgeE_SCHEDDMONITOR, 0, 0, NULL, NULL);
   sge_flush_events(FLUSH_EVENTS_SET);

   INFO((SGE_EVENT, MSG_COM_SCHEDMON_SS, user, host));
   sge_add_answer(&(answer->alp), SGE_EVENT, STATUS_OK, NUM_AN_INFO);
}
