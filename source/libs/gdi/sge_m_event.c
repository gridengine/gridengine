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
#include "sge_feature.h"
#include "sge_time.h"
#include "sge_m_event.h"
#include "sge_host.h"
#include "sge_pe_qmaster.h"
#include "sge_event.h"
#include "sge_all_listsL.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_conf.h"
#include "sge_security.h"
#include "sge_answer.h"
#include "sge_queue.h"
#include "sge_report.h"
#include "sge_ckpt.h"
#include "sge_pe.h"
#include "sge_userprj.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_userset.h"
#include "sge_complex.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_sharetree.h"
#include "sge_hostgroup.h"
#include "sge_usermap.h"

#ifdef QIDL
#include "qidl_c_gdi.h"
#endif

#include "msg_qmaster.h"

/****** Eventclient/Server/--Event_Client_Server ******************************
*  NAME
*     Event Client Interface -- Server Functionality
*
*  SYNOPSIS
*
*  FUNCTION
*     The server module of the event client interface is used to integrate
*     the capability to server event clients into server daemons.
*     
*     It is used in the Grid Engine qmaster but should be capable to handle
*     any event client server integration.
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/--Event_Client_Interface
*     Eventclient/Server/reinit_event_client()
*     Eventclient/Server/sge_add_event_client()
*     Eventclient/Server/sge_mod_event_client()
*     Eventclient/Server/sge_event_client_exit()
*     Eventclient/Server/sge_gdi_kill_eventclient()
*     Eventclient/Server/sge_eventclient_subscribed()
*     Eventclient/Server/sge_ack_event()
*     Eventclient/Server/ck_4_deliver_events()
*     Eventclient/Server/sge_flush_events()
*     Eventclient/Server/sge_next_flush()
*     Eventclient/Server/sge_add_list_event()
*     Eventclient/Server/sge_add_event()
*     Eventclient/Server/sge_get_next_event_number()
*     Eventclient/Server/sge_gdi_tsm()
*     Eventclient/Server/eventclient_list_locate()
*     Eventclient/Server/set_event_client_busy()
*******************************************************************************/
static void total_update(lListElem *event_client);

static void sge_total_update_event(lListElem *event_client, ev_event type);

static void sge_add_event_( lListElem *event_client, ev_event type, 
                     u_long32 intkey, u_long32 intkey2, const char *strkey, 
                     lListElem *element );

static int sge_add_list_event_(lListElem *event_client,
                               ev_event type, u_long32 intkey, 
                               u_long32 intkey2, const char *strkey, 
                               lList *list, int need_copy_elem);

static void sge_flush_events_(lListElem *event_client, int interval, int now);
void sge_gdi_kill_eventclient_(lListElem *event_client, const char *host, const char *user, uid_t uid, sge_gdi_request *answer);

static void check_send_new_subscribed_list(const char *old_subscription, const char *new_subscription, 
                                           lListElem *event_client, ev_event event);

/* static void dump_subscription(const char *subscription); */

/****** Eventclient/Server/-Event_Client_Server_Defines *****************************************
*  NAME
*     Defines -- Constants used in the module
*
*  SYNOPSIS
*     #define FLUSH_INTERVAL 15
*     #define EVENT_ACK_MIN_TIMEOUT 600
*     #define EVENT_ACK_MAX_TIMEOUT 1200
*
*  FUNCTION
*     FLUSH_INTERVAL is the default event delivery interval, if the client
*     would not set a correct interval.
*
*     EVENT_ACK_MIN/MAX_TIMEOUT is the minimum/maximum timeout value for an event
*     client sending the acknowledge for the delivery of events.
*     The real timeout value depends on the event delivery interval for the 
*     event client (10 * event delivery interval).
*
*******************************************************************************/
#define FLUSH_INTERVAL 15
#define EVENT_ACK_MIN_TIMEOUT 600
#define EVENT_ACK_MAX_TIMEOUT 1200

lList *EV_Clients = NULL;

#ifdef QIDL
u_long32 qidl_event_count = 0;
#endif

/****** Eventclient/Server/sge_flush_events() *****************************************
*  NAME
*     sge_flush_events() -- set the flushing time for events
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_flush_events(lListElem *event_client, int interval) 
*
*  FUNCTION
*     Sets the timestamp for the next flush of events for all or a specific
*     event client.
*     When events will be next sent to an event client is stored in its
*     event client object in the variable EV_next_send_time.
*
*  INPUTS
*     lListElem *event_client - the event client for which to define flushing,
*                               or NULL (flush all event clients)
*     int interval            - time in seconds until next flush
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Server/ck_4_deliver_events()
*
*******************************************************************************/
void sge_flush_events(lListElem *event_client, int interval) 
{
   int now = sge_get_gmt();

   if(event_client == NULL) {
      for_each(event_client, EV_Clients) {
         sge_flush_events_(event_client, interval, now);
      }
   } else {
      sge_flush_events_(event_client, interval, now);
   }
}
 
static void sge_flush_events_(lListElem *event_client, int interval, int now)
{
   u_long32 next_send;

   next_send = lGetUlong(event_client, EV_next_send_time);

   next_send = MIN(next_send, now + interval);
   lSetUlong(event_client, EV_next_send_time, next_send);

   DPRINTF(("ev_client: %s\tNOW: %d NEXT FLUSH: %d\n",
            lGetString(event_client, EV_name), now, next_send)); 
}

/****** Eventclient/Server/sge_next_flush() *******************************************
*  NAME
*     sge_next_flush() -- when will be the next flush of events?
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int sge_next_flush(int now) 
*
*  FUNCTION
*     Returns the timestamp of the next required flush to any event client 
*     (the minimum of EV_next_send_time for all event clients).
*
*  INPUTS
*     int now - actual timestamp
*
*  RESULT
*     int - the timestamp of the next flush or
*           0, if no event client is connected and up.
*
*******************************************************************************/
int sge_next_flush(int now) 
{
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

/****** Eventclient/Server/reinit_event_client() ********************************************
*  NAME
*     reinit_event_client() -- do a total update for the scheduler
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int reinit_event_client(ev_registration_id id) 
*
*  FUNCTION
*     Does a total update (send all lists) to the event client specified by id
*     and outputs an error message.
*
*  INPUTS
*     ev_registration_id id - the id of the event client to reinitialize.
*
*  RESULT
*     int - 0 if reinitialization failed, e.g. because the event client does
*           not exits, else 0
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Server/total_update()
*
*******************************************************************************/
int reinit_event_client(ev_registration_id id)
{
   lListElem *event_client;

   DENTER(TOP_LAYER, "reinit_event_client");

   if ((event_client=eventclient_list_locate(id)) != NULL) {
      ERROR((SGE_EVENT, MSG_EVE_REINITEVENTCLIENT_S, lGetString(event_client, EV_name)));
      total_update(event_client);
      DEXIT;
      return 1;
   } else {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_U, u32c(id)));
      DEXIT;
      return 0;
   }
}

/****** Eventclient/Server/total_update() *********************************************
*  NAME
*     total_update() -- send all data to eventclient
*
*  SYNOPSIS
*     static void total_update(lListElem *event_client) 
*
*  FUNCTION
*     Sends all complete lists it subscribed to an eventclient.
*     If the event client receives a complete list instead of single events,
*     it should completely update it's database.
*
*  INPUTS
*     lListElem *event_client - the event client to update
*
*  SEE ALSO
*
*******************************************************************************/
static void total_update(
lListElem *event_client 
) {
   DENTER(TOP_LAYER, "total_update");

   sge_total_update_event(event_client, sgeE_ADMINHOST_LIST);
   sge_total_update_event(event_client, sgeE_CALENDAR_LIST);
   sge_total_update_event(event_client, sgeE_CKPT_LIST);
   sge_total_update_event(event_client, sgeE_COMPLEX_LIST);
   sge_total_update_event(event_client, sgeE_CONFIG_LIST);
   sge_total_update_event(event_client, sgeE_EXECHOST_LIST);
   sge_total_update_event(event_client, sgeE_JOB_LIST);
   sge_total_update_event(event_client, sgeE_JOB_SCHEDD_INFO_LIST);
   sge_total_update_event(event_client, sgeE_MANAGER_LIST);
   sge_total_update_event(event_client, sgeE_OPERATOR_LIST);
   sge_total_update_event(event_client, sgeE_PE_LIST);
   sge_total_update_event(event_client, sgeE_QUEUE_LIST);
   sge_total_update_event(event_client, sgeE_SCHED_CONF);
   sge_total_update_event(event_client, sgeE_SUBMITHOST_LIST);
   sge_total_update_event(event_client, sgeE_USERSET_LIST);

   if (feature_is_enabled(FEATURE_SGEEE)) {
      sge_total_update_event(event_client, sgeE_NEW_SHARETREE);
      sge_total_update_event(event_client, sgeE_PROJECT_LIST);
      sge_total_update_event(event_client, sgeE_USER_LIST);
   }

#ifndef __SGE_NO_USERMAPPING__
   sge_total_update_event(event_client, sgeE_HOST_GROUP_LIST);
   sge_total_update_event(event_client, sgeE_USERMAPPING_ENTRY_LIST);
#endif

   DEXIT;
   return;
}

/****** Eventclient/Server/sge_add_event_client() *************************************
*  NAME
*     sge_add_event_client() -- register a new event client
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int sge_add_event_client(lListElem *clio, lList **alpp, lList **eclpp, 
*     char *ruser, char *rhost) 
*
*  FUNCTION
*     Registeres a new event client. 
*     If it requested a dynamic id, a new id is created and assigned.
*     If it is a special client (with fixed id) and an event client
*     with this id already exists, the old instance is deleted and the
*     new one registered.
*     If the registration succees, the event client is sent all data
*     (sgeE*_LIST events) according to its subscription.
*
*  INPUTS
*     lListElem *clio - the event client object used as registration data
*     lList **alpp    - answer list pointer for answer to event client
*     lList **eclpp   - list pointer to return new event client object
*     char *ruser     - user that tries to register an event client
*     char *rhost     - host on which the event client runs
*
*  RESULT
*     int - AN_status value. STATUS_OK on success, else error code
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Server/sge_mod_event_client()
*     Eventclient/Server/sge_gdi_kill_eventclient()
*     Eventclient/Server/total_update()
*
*******************************************************************************/
int sge_add_event_client(
lListElem *clio,
lList **alpp,
lList **eclpp,
char *ruser,
char *rhost 
) {
   lListElem *ep=NULL;
   u_long32 now;
   u_long32 id;
   const char *name;
   const char *subscription;
   static u_long32 first_dynamic_id = EV_ID_FIRST_DYNAMIC;

   DENTER(TOP_LAYER,"sge_add_event_client");

   id = lGetUlong(clio, EV_id);
   name = lGetString(clio, EV_name);
   subscription = lGetString(clio, EV_subscription);

   if(name == NULL) {
      name = "unnamed";
      lSetString(clio, EV_name, name);
   }
   
   if(id < EV_ID_ANY || id >= EV_ID_FIRST_DYNAMIC) { /* invalid request */
      ERROR((SGE_EVENT, MSG_EVE_ILLEGALIDREGISTERED_U, u32c(id)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if(subscription == NULL || strlen(subscription) != sgeE_EVENTSIZE) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDSUBSCRIPTION));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if(id == EV_ID_ANY) {   /* qmaster shall give id dynamically */
      id = first_dynamic_id++;
      lSetUlong(clio, EV_id, id);
      INFO((SGE_EVENT, MSG_EVE_REG_SU, name, u32c(id)));         
   }

   /* special event clients: we allow only one instance */
   /* if it already exists, delete the old one and register */
   /* the new one */
   if(id > EV_ID_ANY && id < EV_ID_FIRST_DYNAMIC) {
      if ((ep=eventclient_list_locate(id))) {
         /* we already have this special client */
         ERROR((SGE_EVENT, MSG_EVE_CLIENTREREGISTERED_S, name));         

         /* delete old event client entry */
         lRemoveElem(EV_Clients, ep);
      } else {
         INFO((SGE_EVENT, MSG_EVE_REG_SU, name, u32c(id)));         
      }   
   }

   ep=lCopyElem(clio);
   if(!EV_Clients) 
      EV_Clients=lCreateList("EV_Clients", EV_Type); 
   lAppendElem(EV_Clients,ep);
   lSetUlong(ep, EV_next_number, 1);

   /* register this contact */
   now = sge_get_gmt();
   lSetUlong(ep, EV_last_send_time, 0);
   lSetUlong(ep, EV_next_send_time, now + lGetUlong(ep, EV_d_time));
   lSetUlong(ep, EV_last_heard_from, now);

   /* return new event client object to event client */
   if(eclpp != NULL) {
      if(*eclpp == NULL) {
         *eclpp = lCreateList("new event client", EV_Type);
      }

      lAppendElem(*eclpp, lCopyElem(ep));
   }

   /* build events for total update */
   total_update(ep);

   /* flush initial list events */
   sge_flush_events(ep, 0);
   
   INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS,
         ruser, rhost, name, MSG_EVE_EVENTCLIENT));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT; 
   return STATUS_OK;
}

/****** Eventclient/Server/sge_mod_event_client() *************************************
*  NAME
*     sge_mod_event_client() -- modify event client
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int sge_mod_event_client(lListElem *clio, lList **alpp, lList **eclpp, 
*     char *ruser, char *rhost) 
*
*  FUNCTION
*     An event client object is modified.
*     It is possible to modify the event delivery time and
*     the subscription.
*     If the subscription is changed, and new sgeE*_LIST events are subscribed,
*     these lists are sent to the event client.
*
*  INPUTS
*     lListElem *clio - object containing the data to change
*     lList **alpp    - answer list pointer
*     lList **eclpp   - list pointer to return changed object
*     char *ruser     - user that triggered the modify action
*     char *rhost     - host that triggered the modify action
*
*  RESULT
*     int - AN_status code. STATUS_OK on success, else error code
*
*  SEE ALSO
*     Eventclient/Server/check_send_new_subscribed_list()
*
*******************************************************************************/
int sge_mod_event_client(
lListElem *clio,
lList **alpp,
lList **eclpp,
char *ruser,
char *rhost 
) {
   lListElem *event_client=NULL;
   u_long32 id;
   u_long32 busy;
   u_long32 ev_d_time;
   const char *subscription;
   const char *old_subscription;

   DENTER(TOP_LAYER,"sge_mod_event_client");

   /* try to find event_client */
   id = lGetUlong(clio, EV_id);
   event_client = lGetElemUlong(EV_Clients, EV_id, id);

   if(event_client == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_U, u32c(id)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* these parameters can be changed */
   busy         = lGetUlong(clio, EV_busy);
   ev_d_time    = lGetUlong(clio, EV_d_time);
   subscription = lGetString(clio, EV_subscription);

   /* check for validity */
   if(ev_d_time < 1) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDINTERVAL_U, u32c(ev_d_time)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if(subscription == NULL || strlen(subscription) != sgeE_EVENTSIZE) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDSUBSCRIPTION));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_ESEMANTIC;
   }

   /* event delivery interval changed. 
    * We have to update the next delivery time to 
    * next_delivery_time - old_interval + new_interval 
    */
   if(ev_d_time != lGetUlong(event_client, EV_d_time)) {
      lSetUlong(event_client, EV_next_send_time, 
                lGetUlong(event_client, EV_next_send_time) - 
                lGetUlong(event_client, EV_d_time) + ev_d_time);
      lSetUlong(event_client, EV_d_time, ev_d_time);
   }

   /* subscription changed */
   old_subscription = lGetString(event_client, EV_subscription);
   if(strcmp(subscription, old_subscription) != 0) {
      lSetString(event_client, EV_subscription, subscription);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_ADMINHOST_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_CALENDAR_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_CKPT_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_COMPLEX_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_CONFIG_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_EXECHOST_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_JOB_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_JOB_SCHEDD_INFO_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_MANAGER_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_OPERATOR_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_NEW_SHARETREE);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_PE_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_PROJECT_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_QUEUE_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_SUBMITHOST_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_USER_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_USERSET_LIST);
      
#ifndef __SGE_NO_USERMAPPING__
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_HOST_GROUP_LIST);
      check_send_new_subscribed_list(old_subscription, subscription, event_client, sgeE_USERMAPPING_ENTRY_LIST);
#endif      
   }

   /* busy state changed */
   if(busy != lGetUlong(event_client, EV_busy)) {
      lSetUlong(event_client, EV_busy, busy);
   }

   DEBUG((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS,
         ruser, rhost, lGetString(event_client, EV_name), MSG_EVE_EVENTCLIENT));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   /* return modified event client object to event client */
   if(eclpp != NULL) {
      if(*eclpp == NULL) {
         *eclpp = lCreateList("modified event client", EV_Type);
      }

      lAppendElem(*eclpp, lCopyElem(event_client));
   }

   DEXIT; 
   return STATUS_OK;
}

/****** Eventclient/Server/check_send_new_subscribed_list() ***************************
*  NAME
*     check_send_new_subscribed_list() -- check suscription for new list events
*
*  SYNOPSIS
*     static void check_send_new_subscribed_list(const char *old_subscription, 
*                                                const char *new_subscription, 
*                                                lListElem *event_client, 
*                                                ev_event event) 
*
*  FUNCTION
*     Checks, if sgeE*_LIST events have been added to the subscription of a
*     certain event client. If yes, send these lists to the event client.
*
*  INPUTS
*     const char *old_subscription - former subscription
*     const char *new_subscription - new subscription
*     lListElem *event_client      - the event client object
*     ev_event event               - the event to check
*
*  SEE ALSO
*     Eventclient/Server/sge_total_update_event()
*
*******************************************************************************/
static void check_send_new_subscribed_list(const char *old_subscription, const char *new_subscription, 
                                    lListElem *event_client, ev_event event)
{
   if((new_subscription[event] & EV_SUBSCRIBED) && (old_subscription[event] == EV_NOT_SUBSCRIBED)) {
      sge_total_update_event(event_client, event);
   }   
}



/****** Eventclient/Server/sge_event_client_exit() ************************************
*  NAME
*     sge_event_client_exit() -- event client deregisters
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_event_client_exit(const char *host, const char *commproc, 
*                                sge_pack_buffer *pb) 
*
*  FUNCTION
*     Deregistration of an event client.
*     The event client tells qmaster that it wants to deregister - usually before
*     it exits.
*     The event client is removed from the list of all event clients.
*
*  INPUTS
*     const char *host     - host that sent the exit message
*     const char *commproc - commproc of the sender
*     sge_pack_buffer *pb  - packbuffer containing information about event client
*
*******************************************************************************/
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

/****** Eventclient/Server/sge_eventclient_subscribed() *******************************
*  NAME
*     sge_eventclient_subscribed() -- has event client subscribed a certain event?
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int sge_eventclient_subscribed(const lListElem *event_client, ev_event event) 
*
*  FUNCTION
*     Checks if the given event client has a certain event subscribed.
*
*  INPUTS
*     const lListElem *event_client - event client to check
*     ev_event event                - event to check
*
*  RESULT
*     int - 0 = not subscribed, 1 = subscribed
*
*******************************************************************************/
int sge_eventclient_subscribed(const lListElem *event_client, ev_event event)
{
   const char *subscription = NULL;

   if(event_client == NULL) {
      return 0;
   }

   subscription = lGetString(event_client, EV_subscription);

   if(subscription == NULL || *subscription == 0) {
      return 0;
   }

   if(subscription[event] & EV_SUBSCRIBED) {
      return 1;
   }

   return 0;
}

/****** Eventclient/Server/sge_ack_event() ********************************************
*  NAME
*     sge_ack_event() -- process acknowledge to event delivery
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     int sge_ack_event(lListElem *event_client, ev_event event_number) 
*
*  FUNCTION
*     After the server sent events to an event client, it has to acknowledge
*     their receipt. 
*     Acknowledged events are deleted from the list of events to deliver, otherwise
*     they will be resent after the next event delivery interval.
*     If the handling of a busy state of the event client is enabled and set to 
*     EV_BUSY_UNTIL_ACK, the event client will be set to "not busy".
*
*  INPUTS
*     lListElem *event_client - event client sending acknowledge
*     ev_event event_number   - serial number of the last event to acknowledge
*
*  RESULT
*     int - always 0
*
*  NOTES
*     Returncode makes no sense anymore. Either improve error handling or
*     make function return void.
*
*  SEE ALSO
*     Eventclient/Server/sge_get_next_event_number()
*
*******************************************************************************/
int sge_ack_event(
lListElem *event_client,
ev_event event_number 
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
   } else {
      DPRINTF(("Got empty event ack\n"));
   }

   /* register time of ack */
   lSetUlong(event_client, EV_last_heard_from, sge_get_gmt());

   /* unset busy if configured accordingly */
   if(lGetUlong(event_client, EV_busy_handling) == EV_BUSY_UNTIL_ACK) {
      lSetUlong(event_client, EV_busy, 0);
   }
   
   DEXIT;
   return 0;
}


/****** Eventclient/Server/ck_4_deliver_events() **************************************
*  NAME
*     ck_4_deliver_events() -- deliver events if necessary
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void ck_4_deliver_events(u_long32 now) 
*
*  FUNCTION
*     Checks delivery time of each event client - if it has been reached, deliver
*     all events for that client.
*
*     In addition, timed out event clients are removed. An event client times out,
*     if it doesn't acknowledge events within 10 * EV_ed_time 
*     (respecting EVENT_ACK_MIN_TIMEOUT and EVENT_ACK_MAX_TIMEOUT).
*
*  INPUTS
*     u_long32 now - actual timestamp
*
*  SEE ALSO
*     Eventclient/Server/-Event_Client_Server_Global_Variables
*     Eventclient/Server/sge_ack_event()
*
*******************************************************************************/
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

      /* do we have to deliver events ? */
      if ((now >= lGetUlong(event_client, EV_next_send_time))
         && !lGetUlong(event_client, EV_busy)) {
      
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
            ret = report_list_send(report_list, host, commproc, id, 0, NULL);

            if (ret == 0) { /* otherwise retry is triggered */
               if(lGetUlong(event_client, EV_busy_handling) != EV_BUSY_NO_HANDLING) {
                  lSetUlong(event_client, EV_busy, 1);
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

/****** Eventclient/Server/sge_add_list_event() ***************************************
*  NAME
*     sge_add_list_event() -- add a list as event
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_add_list_event(lListElem *event_client, ev_event type, u_long32 
*     intkey, u_long32 intkey2, const char *strkey, lList *list) 
*
*  FUNCTION
*     Adds a list of objects to the list of events to deliver, e.g. the sgeE*_LIST 
*     events.
*
*  INPUTS
*     lListElem *event_client - the event client to receive the event, if NULL,
*                               all event clients will receive the event
*     ev_event type           - the event id
*     u_long32 intkey         - additional data
*     u_long32 intkey2        - additional data
*     const char *strkey      - additional data
*     lList *list             - the list to deliver as event
*
*  NOTES
*     Do we need the additional data fields?
*
*  SEE ALSO
*     Eventclient/Server/sge_add_event()
*
*******************************************************************************/
void sge_add_list_event(
lListElem *event_client,
ev_event type,
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

static int sge_add_list_event_(
lListElem *event_client,
ev_event type,
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

   /* check if event clients wants flushing */
   {
      const char *subscription = lGetString(event_client, EV_subscription);
/*       dump_subscription(subscription); */
      if((subscription[type] & EV_FLUSHED) == EV_FLUSHED) {
         DPRINTF(("flushing event client"));
         sge_flush_events(event_client, subscription[type] >> 2);
      }
   }

#ifdef QIDL
   /* send CORBA event */
   /* the supplied list usually contains exactly one element
   ** except for sgeE_JOB_USAGE, and sgeE_JOB_FINAL_USAGE, where there
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
   case sgeE_JOB_FINAL_USAGE:
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

/****** Eventclient/Server/sge_add_event() ********************************************
*  NAME
*     sge_add_event() -- add an object as event
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_add_event(lListElem *event_client, ev_event type, u_long32 
*     intkey, u_long32 intkey2, const char *strkey, lListElem *element) 
*
*  FUNCTION
*     Adds an object to the list of events to deliver. Called, if an event occurs to 
*     that object, e.g. it was added to Grid Engine, modified or deleted.
*  
*     Internally, a list with that single object is created and passed to 
*     sge_add_list_event().
*
*  INPUTS
*     lListElem *event_client - the event client to receive the event, if NULL,
*                               all event clients will receive the event
*     ev_event type           - the event id
*     u_long32 intkey         - additional data
*     u_long32 intkey2        - additional data
*     const char *strkey      - additional data
*     lListElem *element      - the object to deliver as event
*
*  NOTES
*     Do we need the additional data fields?
*
*  SEE ALSO
*     Eventclient/Server/sge_add_list_event()
*
*******************************************************************************/
void sge_add_event(
lListElem *event_client,
ev_event type,
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
ev_event type,
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

/****** Eventclient/Server/sge_get_next_event_number() ********************************
*  NAME
*     sge_get_next_event_number() -- next event number for an event client
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     u_long32 sge_get_next_event_number(u_long32 client_id) 
*
*  FUNCTION
*     Retrieves the next serial event number for an event client.
*
*  INPUTS
*     u_long32 client_id - id of the event client
*
*  RESULT
*     u_long32 - serial number for next event to deliver
*
*******************************************************************************/
u_long32 sge_get_next_event_number(u_long32 client_id) 
{
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


/****** Eventclient/Server/sge_total_update_event() ***********************************
*  NAME
*     sge_total_update_event() -- create a total update event
*
*  SYNOPSIS
*     static void sge_total_update_event(lListElem *event_client, ev_event type) 
*
*  FUNCTION
*     Creates an event delivering a certain list of objects for an event client.
*
*  INPUTS
*     lListElem *event_client - event client to receive the list
*     ev_event type           - event describing the list to update
*
*******************************************************************************/
static void sge_total_update_event(lListElem *event_client, ev_event type) 
{
   u_long32 i;
   lListElem *event;
   lList *lp = NULL;

   DENTER(TOP_LAYER, "sge_total_update_event");

   if(sge_eventclient_subscribed(event_client, type)) {
      switch(type) {
         case sgeE_ADMINHOST_LIST:
            lp = Master_Adminhost_List;
            break;
         case sgeE_CALENDAR_LIST:
            lp = Master_Calendar_List;
            break;
         case sgeE_CKPT_LIST:
            lp = Master_Ckpt_List;
            break;
         case sgeE_COMPLEX_LIST:
            lp = Master_Complex_List;
            break;
         case sgeE_CONFIG_LIST:
            lp = Master_Config_List;
            break;
         case sgeE_EXECHOST_LIST:
            lp = Master_Exechost_List;
            break;
         case sgeE_JOB_LIST:
            lp = Master_Job_List;
            break;
         case sgeE_JOB_SCHEDD_INFO_LIST:
            lp = Master_Job_Schedd_Info_List;
            break;
         case sgeE_MANAGER_LIST:
            lp = Master_Manager_List;
            break;
         case sgeE_NEW_SHARETREE:
            lp = Master_Sharetree_List;
            break;
         case sgeE_OPERATOR_LIST:
            lp = Master_Operator_List;
            break;
         case sgeE_PE_LIST:
            lp = Master_Pe_List;
            break;
         case sgeE_PROJECT_LIST:
            lp = Master_Project_List;
            break;
         case sgeE_QUEUE_LIST:
            lp = Master_Queue_List;
            break;
         case sgeE_SCHED_CONF:
            lp = Master_Sched_Config_List;
            break;
         case sgeE_SUBMITHOST_LIST:
            lp = Master_Submithost_List;
            break;
         case sgeE_USER_LIST:
            lp = Master_User_List;
            break;
         case sgeE_USERSET_LIST:
            lp = Master_Userset_List;
            break;

#ifndef __SGE_NO_USERMAPPING__
         case sgeE_HOST_GROUP_LIST:
            lp = Master_Host_Group_List;
            break;
         case sgeE_USERMAPPING_ENTRY_LIST:
            lp = Master_Usermapping_Entry_List;
            break;
#endif
         default:
            WARNING((SGE_EVENT, MSG_EVE_TOTALUPDATENOTHANDLINGEVENT_I, type));
            DEXIT;
            return;
      }

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


/****** Eventclient/Server/eventclient_list_locate() **************************
*  NAME
*     eventclient_list_locate() -- search for the scheduler
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     lListElem* eventclient_list_locate(ev_registration_id id) 
*
*  FUNCTION
*     Searches the event client list for an event client with the
*     specified id.
*     Returns a pointer to the event client object or
*     NULL, if no such event client is registered.
*
*  INPUTS
*     ev_registration_id id - id of the event client to search
*
*  RESULT
*     lListElem* - event client object or NULL.
*
*  NOTES
*
*******************************************************************************/
lListElem* eventclient_list_locate(ev_registration_id id)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "eventclient_list_locate");

   ep = lGetElemUlong(EV_Clients, EV_id, id);

   DEXIT;
   return ep;
}

/****** Eventclient/Server/sge_gdi_kill_eventclient() *************************
*  NAME
*     sge_gdi_kill_eventclient() -- kill an event client
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_gdi_kill_eventclient(const char *host, sge_gdi_request *request, 
*     sge_gdi_request *answer) 
*
*  FUNCTION
*     Kills one or all dynamic event clients.
*     If a certain event client id is contained in the request, an event client
*     with that id is killed.
*     If the requested id is EC_ID_ANY (0), all event clients with dynamic ids
*     are killed.
*     Killing an event client is done by sending it the special event
*     sgeE_SHUTDOWN and flushing immediately.
*
*  INPUTS
*     const char *host         - host that sent the kill request
*     sge_gdi_request *request - request containing the event client id
*     sge_gdi_request *answer  - answer structure to return an answer to the 
*                                client issuing the kill command (usually qconf)
*
*  SEE ALSO
*     qconf  -kec
*
*******************************************************************************/
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
      answer_list_add(&(answer->alp), SGE_EVENT, 
                      STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
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
            answer_list_add(&(answer->alp), SGE_EVENT, 
                            STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            continue;
         }
      }

      if(id == EV_ID_ANY) {
         /* kill all event clients except schedd */
         for_each(event_client, EV_Clients) {
            if(lGetUlong(event_client, EV_id) >= EV_ID_FIRST_DYNAMIC) {
               sge_gdi_kill_eventclient_(event_client, host, user, uid, answer);
            }
         }
      } else {
         event_client = lGetElemUlong(EV_Clients, EV_id, id);
         if(event_client == NULL) {
            if(id == EV_ID_SCHEDD) {
               ERROR((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
            } else {
               ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_U, u32c(id)));
            }
            answer_list_add(&(answer->alp), SGE_EVENT, 
                            STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            continue;
         }
         sge_gdi_kill_eventclient_(event_client, host, user, uid, answer);
      }
   }
}
   
void sge_gdi_kill_eventclient_(lListElem *event_client, const char *host, const char *user, uid_t uid, sge_gdi_request *answer) 
{
   DENTER(GDI_LAYER, "sge_gdi_kill_eventclient_");

   /* must be either manager or owner of the event client */
   if (sge_manager(user) && uid != lGetUlong(event_client, EV_uid)) {
      ERROR((SGE_EVENT, MSG_COM_NOSHUTDOWNPERMS));
      answer_list_add(&(answer->alp), SGE_EVENT, 
                      STATUS_ENOMGR, ANSWER_QUALITY_ERROR);
      DEXIT;
      return;
   }

   /* add a sgeE_SHUTDOWN event */
   sge_add_event(event_client, sgeE_SHUTDOWN, 0, 0, NULL, NULL);
   /* sge_flush_events(event_client, 0); !!!! not really necessary */

   INFO((SGE_EVENT, MSG_SGETEXT_KILL_SSS, user, host, lGetString(event_client, EV_name)));
   answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
}

/****** Eventclient/Server/sge_gdi_tsm() **********************************************
*  NAME
*     sge_gdi_tsm() -- trigger scheduling
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void sge_gdi_tsm(char *host, sge_gdi_request *request, sge_gdi_request 
*     *answer) 
*
*  FUNCTION
*     Triggers a scheduling run for the scheduler as special event client.
*
*  INPUTS
*     char *host               - host that triggered scheduling
*     sge_gdi_request *request - request structure
*     sge_gdi_request *answer  - answer structure to return to client
*
*  NOTES
*     This function should not be part of the core event client interface.
*     Or it should be possible to trigger any event client.
*
*  SEE ALSO
*     qconf -tsm
*
*******************************************************************************/
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
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return;
   }

   if (sge_manager(user)) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDMONPERMS));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_ENOMGR, ANSWER_QUALITY_WARNING);
      DEXIT;
      return;
   }
     
   scheduler = eventclient_list_locate(EV_ID_SCHEDD);
     
   if (scheduler == NULL) {
      WARNING((SGE_EVENT, MSG_COM_NOSCHEDDREGMASTER));
      answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_WARNING);
      DEXIT;
      return;
   }

   /* add a sgeE_SCHEDDMONITOR event */
   sge_add_event(scheduler, sgeE_SCHEDDMONITOR, 0, 0, NULL, NULL);

   INFO((SGE_EVENT, MSG_COM_SCHEDMON_SS, user, host));
   answer_list_add(&(answer->alp), SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
}


/*
static void dump_subscription(const char *subscription)
{
   fprintf(stderr, "Subscription =");
   while(*subscription) {
      fprintf(stderr, " %03d", *subscription++);
   }
   fprintf(stderr, "\n");
}
*/

/****** Eventclient/Server/set_event_client_busy() ************************************
*  NAME
*     set_event_client_busy() -- set the busy state of event clients
*
*  SYNOPSIS
*     #include "sge_m_event.h"
*
*     void set_event_client_busy(lListElem *event_client, int busy) 
*
*  FUNCTION
*     Sets the busy state of one or all event clients.
*
*  INPUTS
*     lListElem *event_client - the event client to modify - NULL to modify all 
*     int busy                - busy state - 0 = not busy, 1 = busy
*
*  NOTES
*     The busy state should better be a boolean if it this datatype was available
*     in the cull library.
*
*  SEE ALSO
*     
*******************************************************************************/
void set_event_client_busy(lListElem *event_client, int busy)
{
   if(event_client == NULL) {
      for_each(event_client, EV_Clients) {
         lSetUlong(event_client, EV_busy, busy);
      }
   } else {
      lSetUlong(event_client, EV_busy, busy);
   }
}
