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
#include <strings.h>

#include "sge_unistd.h"
#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_eventL.h"
#include "sge_event_client.h"
#include "qm_name.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_answer.h"
#include "sge_report.h"

#include "msg_evclib.h"

/****** Eventclient/--Event_Client_Interface **********************************
*  NAME
*     Evenclient -- The Grid Engine Event Client Interface
*
*  FUNCTION
*     The Grid Engine Event Client Interface provides a means to connect 
*     to the Grid Engine qmaster and receive information about objects 
*     (actual object properties and changes).
*
*     It provides a subscribe/unsubscribe mechanism allowing fine grained
*     selection of objects per object types (e.g. jobs, queues, hosts)
*     and event types (e.g. add, modify, delete).
*
*     Flushing triggered by individual events can be set.
*
*     Policies can be set how to handle busy event clients.
*     Clients that receive large numbers of events or possibly take some
*     time for processing the events (e.g. depending on other components
*     like databases), should in any case make use of the busy handling.
*
*     The event client interface should have much less impact on qmaster 
*     performance than polling the same data in regular intervals.
*
*  NOTES
*     The current implementation is a generalized form of the event client
*     interface that already existed in Codine/GRD.
*
*  SEE ALSO
*     Eventclient/--EV_Type
*     Eventclient/-Subscription
*     Eventclient/-Flushing
*     Eventclient/-Busy-state
*     Eventclient/Client/--Event_Client
*     Eventclient/Server/--Event_Client_Server
*******************************************************************************/

/****** Eventclient/Client/--Event_Client **********************************
*  NAME
*     Event Client Interface -- Client Functionality 
*
*  FUNCTION
*     The client side of the event client interface provides functions
*     to register and deregister (before registering, you have to call
*     ec_prepare_registration to set an id and client name).
*  
*     The subscribe / unsubscribe mechanism allows to select the data
*     (object types and events) an event client shall be sent.
*
*     It is possible to set the interval in which qmaster will send
*     new events to an event client.
*
*  EXAMPLE
*     clients/qevent/qevent.c can serve as a simple example.
*     The scheduler (daemons/schedd/sge_schedd.c) is also implemented
*     as event client and uses all mechanisms of the event client 
*     interface.
*
*  SEE ALSO
*     Eventclient/--Event_Client_Interface
*     Eventclient/Client/ec_prepare_registration()
*     Eventclient/Client/ec_register()
*     Eventclient/Client/ec_deregister()
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_get_flush()
*     Eventclient/Client/ec_set_flush()
*     Eventclient/Client/ec_unset_flush()
*     Eventclient/Client/ec_subscribe_flush()
*     Eventclient/Client/ec_set_edtime()
*     Eventclient/Client/ec_get_edtime()
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_get_busy_handling()
*     Eventclient/Client/ec_set_clientdata()
*     Eventclient/Client/ec_get_clientdata()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*     Eventclient/Client/ec_mark4registration()
*     Eventclient/Client/ec_need_new_registration()
*******************************************************************************/

static 
u_long32 ck_event_number(lList *lp, u_long32 *waiting_for, 
                         u_long32 *wrong_number);

static int 
get_event_list(int sync, lList **lp);

static void 
ec_config_changed(void);

/****** Eventclient/Client/-Event_Client_Global_Variables *********************
*  NAME
*     Global_Variables -- global variables in the client 
*
*  SYNOPSIS
*     static int config_changed = 0;
*     static int need_register  = 1;
*     static lListElem *ec      = NULL;
*     static u_long32 ec_reg_id = 0;
*
*  FUNCTION
*     config_changed - the configuration changed (subscription or event 
*                      interval)
*     need_register  - the client is not registered at the server
*     ec             - event client object holding configuration data
*     ec_reg_id      - the id used to register at the qmaster
*
*  NOTES
*     These global variables should be part of the event client object.
*     The only remaining global variable would be a list of event client objects,
*     allowing one client to connect to multiple event client servers.
*
*******************************************************************************/
static int config_changed = 0;
static int need_register  = 1;
static lListElem *ec      = NULL;
static u_long32 ec_reg_id = 0;

/****** Eventclient/Client/ec_prepare_registration() ***************************
*  NAME
*     ec_prepare_registration() -- prepare registration at server
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_prepare_registration(ev_registration_id id, const char *name) 
*
*  FUNCTION
*     Initializes necessary data structures and sets the data needed for
*     registration at an event server.
*     The events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN are subscribed.
*
*     For valid id's see Eventclient/-ID-numbers.
*
*     The name is informational data that will be included in messages
*     (errors, warnings, infos) and will be shown in the command line tool
*     qconf -sec.
*
*  INPUTS
*     ev_registration_id id  - id used to register
*     const char *name       - name of the event client
*
*  RESULT
*     int - 1, if the function succeeded, else 0 
*
*  SEE ALSO
*     Eventclient/--EV_Type
*     Eventclient/-ID-numbers
*     Eventclient/-Subscription
*     qconf manpage
*     list of events
*******************************************************************************/
int 
ec_prepare_registration(ev_registration_id id, const char *name)
{
   char subscription[sgeE_EVENTSIZE + 1];
   int i;

   DENTER(TOP_LAYER, "ec_prepare_registration");

   if (id >= EV_ID_FIRST_DYNAMIC || name == NULL || *name == 0) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGAL_ID_OR_NAME_US, 
               u32c(id), name != NULL ? name : "NULL" ));
      DEXIT;
      return 0;
   }

   ec = lCreateElem(EV_Type);

   if (ec == NULL) {
      DEXIT; /* error message already comes from lCreateElem */
      return 0;
   }

   /* remember registration id for subsequent registrations */
   ec_reg_id = id;

   /* initialize event client object */
   lSetString(ec, EV_name, name);
   lSetUlong(ec, EV_d_time, DEFAULT_EVENT_DELIVERY_INTERVAL);

   /* initialize subscription "bitfield" */
   for (i = 0; i < sgeE_EVENTSIZE; i++) {
      subscription[i] = EV_NOT_SUBSCRIBED;
   }

   subscription[sgeE_EVENTSIZE] = 0;

   lSetString(ec, EV_subscription, subscription);
   ec_subscribe(sgeE_QMASTER_GOES_DOWN);
   ec_subscribe(sgeE_SHUTDOWN);

   ec_set_busy_handling(EV_BUSY_NO_HANDLING);
   lSetUlong(ec, EV_busy, 0);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_is_initialized() ********************************
*  NAME
*     ec_is_initialized() -- has the client been initialized
*
*  SYNOPSIS
*     int 
*     ec_is_initialized(void) 
*
*  FUNCTION
*     Checks if the event client mechanism has been initialized
*     (if ec_prepare_registration has been called).
*
*  RESULT
*     int - TRUE, if the event client interface has been initialized,
*           else FALSE.
*
*  SEE ALSO
*     Eventclient/Client/ec_prepare_registration()
*******************************************************************************/
int 
ec_is_initialized(void) 
{
   if (ec == NULL) {
      return FALSE;
   } else {
      return TRUE;
   }
}

/****** Eventclient/Client/ec_mark4registration() *****************************
*  NAME
*     ec_mark4registration() -- new registration is required
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     void 
*     ec_mark4registration(void) 
*
*  FUNCTION
*     Tells the event client mechanism, that the connection to the server
*     is broken and it has to reregister.
*
*  NOTES
*     Should be no external interface. The event client mechanism should itself
*     detect such situations and react accordingly.
*
*  SEE ALSO
*     Eventclient/Client/ec_need_new_registration()
*******************************************************************************/
void 
ec_mark4registration(void)
{
   DENTER(TOP_LAYER, "ec_mark4registration");
   need_register = 1;
   DEXIT;
}

/****** Eventclient/Client/ec_need_new_registration() *************************
*  NAME
*     ec_need_new_registration() -- is a reregistration neccessary?
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int ec_need_new_registration(void) 
*
*  FUNCTION
*     Function to check, if a new registration at the server is neccessary.
*
*  RESULT
*     int - 1, if the client has to (re)register, else 0 
*
*******************************************************************************/
int 
ec_need_new_registration(void)
{
   return need_register;
}

/****** Eventclient/Client/ec_set_edtime() ************************************
*  NAME
*     ec_set_edtime() -- set the event delivery interval
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_set_edtime(int interval) 
*
*  FUNCTION
*     Set the interval qmaster will use to send events to the client.
*     Any number > 0 is a valid interval in seconds.
*
*  INPUTS
*     int interval - interval [s]
*
*  RESULT
*     int - 1, if the value was changed, else 0
*
*  NOTES
*     The maximum interval should be limited. A too big interval makes 
*     qmaster spool lots of events and consume a lot of memory.
*
*  SEE ALSO
*     Eventclient/Client/ec_get_edtime()
*******************************************************************************/
int 
ec_set_edtime(int interval) 
{
   int ret;

   DENTER(TOP_LAYER, "ec_set_edtime");
   
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   ret = (lGetUlong(ec, EV_d_time) != interval);

   if (ret) {
      lSetUlong(ec, EV_d_time, interval);
      ec_config_changed();
   }

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_get_edtime() ************************************
*  NAME
*     ec_get_edtime() -- get the current event delivery interval
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_get_edtime(void) 
*
*  FUNCTION
*     Get the interval qmaster will use to send events to the client.
*
*  RESULT
*     int - the interval in seconds
*
*  SEE ALSO
*     Eventclient/Client/ec_set_edtime()
*******************************************************************************/
int 
ec_get_edtime(void) 
{
   int interval;

   DENTER(TOP_LAYER, "ec_get_edtime");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   interval = lGetUlong(ec, EV_d_time);
   DEXIT;
   return interval;
}

/****** Eventclient/Client/ec_set_busy_handling() *****************************
*  NAME
*     ec_set_edtime() -- set the event client busy handling
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_set_busy_handling(ev_busy_handling handling) 
*
*  FUNCTION
*     The event client interface has a mechanism to handle situations in which
*     an event client is busy and will not accept any new events. 
*     The policy to use can be configured using this function.
*     For valid policies see ...
*     This parameter can be changed during runtime and will take effect
*     after a call to ec_commit or ec_get.
*
*  INPUTS
*     ev_busy_handling handling - the policy to use
*
*  RESULT
*     int - 1, if the value was changed, else 0
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_get_busy_handling()
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_set_busy_handling(ev_busy_handling handling) 
{
   int ret;

   DENTER(TOP_LAYER, "ec_set_busy_handling");
   
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   ret = (lGetUlong(ec, EV_busy_handling) != handling);

   if (ret) {
      lSetUlong(ec, EV_busy_handling, handling);
      ec_config_changed();
   }

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_get_busy_handling() *****************************
*  NAME
*     ec_get_busy_handling() -- get configured busy handling policy
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     ev_busy_handling 
*     ec_get_busy_handling(void) 
*
*  FUNCTION
*     Returns the policy currently configured.
*
*  RESULT
*     ev_busy_handling - the current policy
*
*  SEE ALSO
*     Eventclient/Client/ec_set_edtime()
*     Eventclient/-Busy-state
*******************************************************************************/
ev_busy_handling 
ec_get_busy_handling(void) 
{
   ev_busy_handling handling;

   DENTER(TOP_LAYER, "ec_get_busy_handling");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   handling = lGetUlong(ec, EV_busy_handling);
   DEXIT;
   return handling;
}

/****** Eventclient/Client/ec_set_clientdata() ********************************
*  NAME
*     ec_set_clientdata() -- set the clientdata value
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     void 
*     ec_set_clientdata(u_long32 data) 
*
*  FUNCTION
*     An event client (the event client object) has a field EV_clientdata.
*     It is of datatype u_long32 and can be used by the event client for
*     any purpose.
*
*     The sge scheduler uses it to store the last order from scheduler processed
*     by qmaster.
*
*  INPUTS
*     u_long32 data - the clientdata to set 
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_get_clientdata()
*     Eventclient/--EV_Type
*     documentation of scheduler <-> qmaster mechanisms
*******************************************************************************/
void 
ec_set_clientdata(u_long32 data)
{
   DENTER(TOP_LAYER, "ec_set_clientdata");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return;
   }

   lSetUlong(ec, EV_clientdata, data);
   DEXIT;
}

/****** Eventclient/Client/ec_get_clientdata() ********************************
*  NAME
*     ec_get_clientdata() -- get the clientdata value
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     u_long32 
*     ec_get_clientdata() 
*
*  FUNCTION
*     Get the current value of the clientdata for the event client.
*
*  RESULT
*     u_long32 - current value
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_get_clientdata()
*     Eventclient/EV_Type
*******************************************************************************/
u_long32 
ec_get_clientdata(void)
{
   u_long32 clientdata;

   DENTER(TOP_LAYER, "ec_get_clientdata");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   clientdata = lGetUlong(ec, EV_clientdata);

   DEXIT;
   return clientdata;
}

/****** Eventclient/Client/ec_register() ***************************************
*  NAME
*     ec_register() -- register at the event server
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_register(void) 
*
*  FUNCTION
*     Registers the event client at the event server (usually the qmaster). 
*     This function can be called explicitly in the event client at startup
*     or when the connection to qmaster is down.
*
*     It will be called implicitly by ec_get, whenever it detects the neccessity
*     to (re)register the client.
*
*  RESULT
*     int - 1, if the registration succeeded, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_deregister()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_register(void)
{
   int success = 0;
   lList *lp, *alp;
   lListElem *aep;

   DENTER(TOP_LAYER, "ec_register");

   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      WARNING((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      return 0;
   }

   /*
    *   EV_host, EV_commproc and EV_commid get filled
    *  at qmaster side with more secure commd    
    *  informations 
    *
    *  EV_uid gets filled with gdi_request
    *  informations
    */

   lSetUlong(ec, EV_id, ec_reg_id);

   /* initialize, we could do a re-registration */
   lSetUlong(ec, EV_last_heard_from, 0);
   lSetUlong(ec, EV_last_send_time, 0);
   lSetUlong(ec, EV_next_send_time, 0);
   lSetUlong(ec, EV_next_number, 0);


   lp = lCreateList("registration", EV_Type);
   lAppendElem(lp, lCopyElem(ec));

   /* remove possibly pending messages */
   remove_pending_messages(NULL, 0, 0, TAG_REPORT_REQUEST);
   /* commlib call to mark all commprocs as unknown */
   reset_last_heard();


   /*
    *  to add may also means to modify
    *  - if this event client is already enrolled at qmaster
    */

   alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION, &lp, NULL, NULL);
 
   aep = lFirst(alp);
 
   success = (lGetUlong(aep, AN_status)==STATUS_OK);   

   if (success) { 
      lListElem *new_ec;
      u_long32 new_id = 0;

      new_ec = lFirst(lp);
      if(new_ec != NULL) {
         new_id = lGetUlong(new_ec, EV_id);
      }

      if (new_id != 0) {
         lSetUlong(ec, EV_id, new_id);
         DPRINTF(("REGISTERED with id "U32CFormat"\n", new_id));
         lFreeList(lp); 
         lFreeList(alp);

         config_changed = 0;
         
         DEXIT;
         return 1;
      }
   } else {
      if (lGetUlong(aep, AN_quality) == ANSWER_QUALITY_ERROR) {
         ERROR((SGE_EVENT, "%s", lGetString(aep, AN_text)));
         lFreeList(lp);
         lFreeList(alp);
         DEXIT;
         SGE_EXIT(1);
         return 0;
      }   
   }

   lFreeList(lp); 
   lFreeList(alp);
   DPRINTF(("REGISTRATION FAILED\n"));
   DEXIT;
   return 0;
}

/****** Eventclient/Client/ec_deregister() ************************************
*  NAME
*     ec_deregister() -- deregister from the event server
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_deregister(void) 
*
*  FUNCTION
*     Deregister from the event server (usually the qmaster).
*     This function should be called when an event client exits. 
*
*     If an event client does not deregister, qmaster will spool events for this
*     client until it times out (it did not acknowledge events sent by qmaster).
*     After the timeout, it will be deleted.
*
*  RESULT
*     int - TRUE, if the deregistration succeeded, else FALSE
*
*  SEE ALSO
*     Eventclient/Client/ec_register()
*******************************************************************************/
int 
ec_deregister(void)
{
   int ret;
   sge_pack_buffer pb;

   DENTER(TOP_LAYER, "ec_deregister");

   /* not yet initialized? Nothing to shutdown */
   if (ec == NULL) {
      DEXIT;
      return TRUE;
   }

   if (init_packbuffer(&pb, sizeof(u_long32), 0) != PACK_SUCCESS) {
      /* error message is output from init_packbuffer */
      DEXIT;
      return FALSE;
   }

   packint(&pb, lGetUlong(ec, EV_id));

   ret = sge_send_any_request(0, NULL, sge_get_master(0), 
         prognames[QMASTER], 1, &pb, TAG_EVENT_CLIENT_EXIT);
   
   clear_packbuffer(&pb);

   if (ret != CL_OK) {
      /* error message is output from sge_send_any_request */
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/****** Eventclient/Client/ec_subscribe() *************************************
*  NAME
*     ec_subscribe() -- Subscribe an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_subscribe(ev_event event) 
*
*  FUNCTION
*     Subscribe a certain event.
*     See Eventclient/-Events for a list of all events.
*     The subscription will be in effect after calling ec_commit or ec_get.
*     It is possible / sensible to subscribe all events wanted and then call
*     ec_commit.
*
*  INPUTS
*     ev_event event - the event number
*
*  RESULT
*     int - 1 on success, else 0
*
*  SEE ALSO
*     Eventclient/-Events
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_subscribe(ev_event event)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_subscribe");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         subscription[i] = EV_SUBSCRIBED;
      }
   } else {
      subscription[event] = EV_SUBSCRIBED;
   }

   if (strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }

   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_subscribe_all() *********************************
*  NAME
*     ec_subscribe_all() -- subscribe all events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_subscribe_all(void) 
*
*  FUNCTION
*     Subscribe all possible event.
*     The subscription will be in effect after calling ec_commit or ec_get.
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*     Subscribing all events can cause a lot of traffic and may 
*     decrease performance of qmaster.
*     Only subscribe all events, if you really need them.
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_subscribe_all(void)
{
   return ec_subscribe(sgeE_ALL_EVENTS);
}

/****** Eventclient/Client/ec_unsubscribe() ***********************************
*  NAME
*     ec_unsubscribe() -- unsubscribe an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_unsubscribe(ev_event event) 
*
*  FUNCTION
*     Unsubscribe a certain event.
*     See ... for a list of all events.
*     The change will be in effect after calling ec_commit or ec_get.
*     It is possible / sensible to unsubscribe all events 
*     no longer needed and then call ec_commit.
*
*  INPUTS
*     ev_event event - the event to unsubscribe
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*     The events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN cannot
*     be unsubscribed. ec_unsubscribe will output an error message if
*     you try to unsubscribe sgeE_QMASTER_GOES_DOWN or sgeE_SHUTDOWN
*     and return 0.
*
*  SEE ALSO
*     Eventclient/-Events
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_unsubscribe(ev_event event)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_unsubscribe");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         subscription[i] = EV_NOT_SUBSCRIBED;
      }
      subscription[sgeE_QMASTER_GOES_DOWN] = EV_FLUSHED;
      subscription[sgeE_SHUTDOWN] = EV_FLUSHED;
   } else {
      if (event == sgeE_QMASTER_GOES_DOWN || event == sgeE_SHUTDOWN) {
         ERROR((SGE_EVENT, MSG_EVENT_HAVETOHANDLEEVENTS));
         free(subscription);
         DEXIT;
         return 0;
      } else {
         subscription[event] = EV_NOT_SUBSCRIBED;
      }
   }

   if (strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }
   
   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_unsubscribe_all() *******************************
*  NAME
*     ec_unsubscribe_all() -- unsubscribe all events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_unsubscribe_all(void) 
*
*  FUNCTION
*     Unsubscribe all possible event.
*     The change will be in effect after calling ec_commit or ec_get.
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*     The events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN will not be 
*     unsubscribed.
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_unsubscribe_all(void)
{
   return ec_unsubscribe(sgeE_ALL_EVENTS);
}

/****** Eventclient/Client/ec_get_flush() *************************************
*  NAME
*     ec_get_flush() -- get flushing information for an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_get_flush(ev_event event) 
*
*  FUNCTION
*     An event client can request flushing of events from qmaster 
*     for any number of the events subscribed.
*     This function returns the flushing information for an 
*     individual event.
*
*  INPUTS
*     ev_event event - the event id to query 
*
*  RESULT
*     int - EV_NO_FLUSH(-1) or the number of seconds used for flushing
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Flushing
*     Eventclient/Client/ec_set_flush()
*******************************************************************************/
int 
ec_get_flush(ev_event event)
{
   const char *subscription;
   
   DENTER(TOP_LAYER, "ec_get_flush");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
      DEXIT;
      return EV_NO_FLUSH;
   }

   subscription = lGetString(ec, EV_subscription);

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return EV_NO_FLUSH;
   }

   if ((subscription[event] & EV_FLUSHED) == EV_FLUSHED) {
      DEXIT;
      return subscription[event] >> 2;
   }

   DEXIT;
   return EV_NO_FLUSH;
}

/****** Eventclient/Client/ec_set_flush() *************************************
*  NAME
*     ec_set_flush() -- set flushing information for an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_set_flush(ev_event event, int flush) 
*
*  FUNCTION
*     An event client can request flushing of events from qmaster 
*     for any number of the events subscribed.
*     This function sets the flushing information for an individual 
*     event.
*
*  INPUTS
*     ev_event event - id of the event to configure
*     int flush      - the number of seconds between creation of 
*                      the event and flushing of the messages.
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Flushing
*     Eventclient/Client/ec_get_flush()
*     Eventclient/Client/ec_unset_flush()
*     Eventclient/Client/ec_subscribe_flush()
*******************************************************************************/
int 
ec_set_flush(ev_event event, int flush)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_set_flush");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
      DEXIT;
      return 0;
   }

   if (flush == EV_NO_FLUSH) {
      DEXIT;
      return ec_unset_flush(event);
   }

   if (flush < 0 || flush > EV_MAX_FLUSH) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALFLUSHTIME_I, flush));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }
   
   if (subscription[event] & EV_SUBSCRIBED) {
      subscription[event] = (flush << 2) | EV_FLUSHED;
   }

   if (strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }
   
   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_unset_flush() ***********************************
*  NAME
*     ec_unset_flush() -- unset flushing information
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_unset_flush(ev_event event) 
*
*  FUNCTION
*     Switch of flushing of an individual event.
*
*  INPUTS
*     ev_event event - if of the event to configure
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Flushing
*     Eventclient/Client/ec_set_flush()
*     Eventclient/Client/ec_get_flush()
*     Eventclient/Client/ec_subscribe_flush()
******************************************************************************/
int 
ec_unset_flush(ev_event event)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_unset_flush");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }
   
   if (subscription[event] & EV_SUBSCRIBED) {
      subscription[event] = EV_SUBSCRIBED;
   } else {
      subscription[event] = EV_NOT_SUBSCRIBED;
   }

   if (strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }
   
   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_subscribe_flush() *******************************
*  NAME
*     ec_subscribe_flush() -- subscribe an event and set flushing
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_subscribe_flush(ev_event event, int flush) 
*
*  FUNCTION
*     Subscribes and event and configures flushing for this event.
*
*  INPUTS
*     ev_event event - id of the event to subscribe and flush
*     int flush      - number of seconds between event creation 
*                      and flushing of events
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Subscription
*     Eventclient/-Flushing
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_set_flush()
******************************************************************************/
int 
ec_subscribe_flush(ev_event event, int flush) 
{
   int ret;
   
   ret = ec_subscribe(event);
   if (ret) {
      ret = ec_set_flush(event, flush);
   }

   return ret;
}

/****** Eventclient/Client/ec_set_busy() **************************************
*  NAME
*     ec_set_busy() -- set the busy state
*
*  SYNOPSIS
*     int 
*     ec_set_busy(int busy) 
*
*  FUNCTION
*     Sets the busy state of the client. This has to be done if 
*     the busy policy has been set to EV_BUSY_UNTIL_RELEASED.
*     An event client can set or unset the busy state at any time.
*     While it is marked busy at the qmaster, qmaster will not 
*     deliver events to this client.
*     The changed busy state will be communicated to qmaster with 
*     the next call to ec_commit (implicitly called by the next 
*     ec_get).
*
*  INPUTS
*     int busy - 1 = event client busy, 0 = event client idle
*
*  RESULT
*     int - 1 = success, 0 = failed
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_get_busy()
*******************************************************************************/
int 
ec_set_busy(int busy)
{
   DENTER(TOP_LAYER, "ec_set_busy");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   lSetUlong(ec, EV_busy, busy);

   /* force communication to qmaster - we may be out of sync */
   ec_config_changed();

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_get_busy() **************************************
*  NAME
*     ec_get_busy() -- get the busy state
*
*  SYNOPSIS
*     int 
*     ec_get_busy(void) 
*
*  FUNCTION
*     Reads the busy state of the event client.
*
*  RESULT
*     int - 1: the client is busy, 0: the client is idle 
*
*  NOTES
*     The function only returns the local busy state in the event 
*     client itself. If this state changes, it will be reported to 
*     qmaster with the next communication, but not back from 
*     qmaster to the client.
*
*  SEE ALSO
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_set_busy()
*******************************************************************************/
int 
ec_get_busy(void)
{
   int ret;

   DENTER(TOP_LAYER, "ec_get_busy");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }
  
   ret = lGetUlong(ec, EV_busy);

   DEXIT;
   return ret;
}
/****** Eventclient/Client/ec_config_changed() ********************************
*  NAME
*     ec_config_changed() -- tell system the config has changed
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     static void 
*     ec_config_changed(void) 
*
*  FUNCTION
*     Checkes whether the configuration has changes.
*     Configuration changes can either be changes in the subscription
*     or change of the event delivery interval.
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_set_edtime()
******************************************************************************/
static void 
ec_config_changed(void) 
{
   config_changed = 1;
}

/****** Eventclient/Client/ec_commit() ****************************************
*  NAME
*     ec_commit() -- commit configuration changes
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_commit(void) 
*
*  FUNCTION
*     Configuration changes (subscription and/or event delivery 
*     time) will be sent to the event server.
*     The function should be called after (multiple) configuration 
*     changes have been made.
*     If it is not explicitly called by the event client program, 
*     the next call of ec_get will commit configuration changes 
*     before looking for new events.
*
*  RESULT
*     int - 1 on success, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_commit_multi()
*     Eventclient/Client/ec_config_changed()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_commit(void) 
{
   int success;
   lList *lp, *alp;

   DENTER(TOP_LAYER, "ec_commit");

   /* not yet initialized? Cannot send modification to qmaster! */
   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      DPRINTF((MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   /* not (yet) registered? Cannot send modification to qmaster! */
   if (ec_need_new_registration()) {
      DPRINTF((MSG_EVENT_NOTREGISTERED));
      DEXIT;
      return 0;
   }

   if (!config_changed) {
      DPRINTF(("no changes to commit\n"));
      DEXIT;
      return 0;
   }

   lp = lCreateList("change configuration", EV_Type);
   lAppendElem(lp, lCopyElem(ec));

   /*
    *  to add may also means to modify
    *  - if this event client is already enrolled at qmaster
    */
   alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
   lFreeList(lp); 
   
   success = (lGetUlong(lFirst(alp), AN_status)==STATUS_OK);   
   lFreeList(alp);
   
   if (success) {
      config_changed = 0;
      DEXIT;
      return 1;
   }

   DPRINTF(("CHANGE EVENT CLIENT CONFIGURATION FAILED\n"));
   DEXIT;
   return 0;
}

/****** Eventclient/Client/ec_commit_multi() ****************************
*  NAME
*     ec_commit() -- commit configuration changes via gdi multi request
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_commit_multi(lList **malpp) 
*
*  FUNCTION
*     Similar to ec_commit configuration changes will be sent to qmaster.
*     But unless ec_commit which uses sge_gdi to send the change request, 
*     ec_commit_multi uses a sge_gdi_multi call to send the configuration
*     change along with other gdi requests.
*     The ec_commit_multi call has to be the last request of the multi 
*     request and will trigger the communication of the requests.
*
*  INPUTS
*     malpp - answer list for the whole gdi multi request
*
*  RESULT
*     int - TRUE on success, else FALSE
*
*  SEE ALSO
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_config_changed()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int 
ec_commit_multi(lList **malpp, state_gdi_multi *state) 
{
   int commit_id, ret;
   lList *lp, *alp = NULL;

   DENTER(TOP_LAYER, "ec_commit_multi");

   /* not yet initialized? Cannot send modification to qmaster! */
   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      DPRINTF((MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return FALSE;
   }

   /* not (yet) registered? Cannot send modification to qmaster! */
   if (ec_need_new_registration()) {
      DPRINTF((MSG_EVENT_NOTREGISTERED));
      DEXIT;
      return FALSE;
   }

   /* do not check, if anything has changed.
    * we have to send the request in any case to finish the 
    * gdi multi request
    */

   lp = lCreateList("change configuration", EV_Type);
   lAppendElem(lp, lCopyElem(ec));

   /*
    *  to add may also means to modify
    *  - if this event client is already enrolled at qmaster
    */
   commit_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_EVENT_LIST, SGE_GDI_MOD, 
                             lp, NULL, NULL, malpp, state);
   lp = lFreeList(lp); 

   if (alp != NULL) {
      answer_list_handle_request_answer_list(&alp, stderr);
      DEXIT;
      return FALSE;
   }

   alp = sge_gdi_extract_answer(SGE_GDI_ADD, SGE_ORDER_LIST, commit_id, 
                                *malpp, NULL);

   ret = answer_list_handle_request_answer_list(&alp, stderr);

   if (ret == STATUS_OK) {
      config_changed = 0;
      DEXIT;
      return TRUE;
   }

   DPRINTF(("CHANGE EVENT CLIENT CONFIGURATION FAILED\n"));
   DEXIT;
   return FALSE;
}

/****** Eventclient/Client/ec_get() ******************************************
*  NAME
*     ec_get() -- look for new events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_get(lList **event_list) 
*
*  FUNCTION
*     ec_get looks for new events. 
*     If new events have arrived, they are passed back to the 
*     caller in the parameter event_list.
*     
*     If the event client is not yet registered at the event server, 
*     the registration will be done before looking for events.
*
*     If the configuration changed since the last call of ec_get 
*     and has not been committed, ec_commit will be called before 
*     looking for events.
*
*  INPUTS
*     lList **event_list - pointer to an event list to hold arriving 
*                          events
*
*  RESULT
*     int - 0, if events or an empty event list was received, or
*              if no data was received within a certain time period
*          <0 (commlib error code), if an error occured
*
*  NOTES
*     The event_list has to be freed by the calling function.
*
*  SEE ALSO
*     Eventclient/Client/ec_register()
*     Eventclient/Client/ec_commit()
*******************************************************************************/
int 
ec_get(lList **event_list) 
{  
   int ret = -1;
   lList *report_list = NULL;
   static u_long32 next_event = 1;
   u_long32 wrong_number;

   DENTER(TOP_LAYER, "ec_get");
 
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return -1;
   }
 
   if (ec_need_new_registration()) {
      next_event = 1;
      if (!ec_register()) {
         DEXIT;
         return -1;
      } else {
         need_register = 0;
      }
   }
      
   if (config_changed) {
      ec_commit();
   }
   
   /* receive event message(s) 
    * The following problems exists here:
    * - there might be multiple event reports at commd - so fetching only one
    *   is not sufficient
    * - if we fetch reports until fetching fails, we can run into an endless
    *   loop, if qmaster sends lots of event reports (e.g. due to flushing)
    * - so what number of events shall we fetch?
    *   Let's assume that qmaster will send a maximum of 1 event message per
    *   second. Then our maximum number of messages to fetch is the number of
    *   seconds passed since last fetch.
    *   For the first fetch after registration, we can take the event delivery
    *   interval.
    * - To make sure this algorithm works in all cases, we could restrict both
    *   event delivery time and flush time to intervals greater than 1 second.
    */

   {
      static time_t last_fetch_time = 0;
      time_t now;

      bool done = false;
      int max_fetch;
      int sync = 1;

      now = sge_get_gmt();

      /* initialize the maximum number of fetches */
      if (last_fetch_time == 0) {
         max_fetch = ec_get_edtime();
      } else {
         max_fetch = now - last_fetch_time;
      }

      last_fetch_time = now;

      DPRINTF(("ec_get retrieving events - will do max %d fetches\n", 
               max_fetch));

      /* fetch data until nothing left or maximum reached */
      while (!done) {
         DPRINTF(("doing %s fetch for messages, %d still to do\n", 
                  sync ? "sync" : "async", max_fetch));

         if ((ret = get_event_list(sync, &report_list)) == CL_OK) {
            lList *new_events = NULL;
            lXchgList(lFirst(report_list), REP_list, &new_events);
            report_list = lFreeList(report_list);

            if (ck_event_number(new_events, &next_event, &wrong_number)) {
               /*
                *  may be we got an old event, that was sent before
                *  reregistration at qmaster
                */
               lFreeList(*event_list);
               lFreeList(new_events);
               ec_mark4registration();
               DEXIT;
               return -1;
            }

            DPRINTF(("got %d events till "u32"\n", 
                     lGetNumberOfElem(new_events), next_event-1));

            if (*event_list != NULL) {
               lAddList(*event_list, new_events);
            } else {
               *event_list = new_events;
            }

         } else {
            /* get_event_list failed - we are through */
            done = true;
            continue; 
         }

         sync = 0;

         if (--max_fetch <= 0) {
            /* maximum number of fetches reached - stop fetching reports */
            done = true;
         }
      }

      /* if first synchronous get_event_list failed, return error */
      if (sync && ret != CL_OK) {
         DPRINTF(("first syncronous get_event_list failed\n"));
         DEXIT;
         return ret;
      }

      /* send an ack to the qmaster for all received events */
      if (sge_send_ack_to_qmaster(0, ACK_EVENT_DELIVERY, next_event-1, 
                                  lGetUlong(ec, EV_id))) {
         WARNING((SGE_EVENT, MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY ));
      } else {
         DPRINTF(("Sent ack for all events lower or equal %d\n", 
                  (next_event-1)));
      }           
   } 

   DPRINTF(("ec_get - received %d events\n", lGetNumberOfElem(*event_list)));

   DEXIT;
   return 0;
}


/****** Eventclient/Client/ck_event_number() **********************************
*  NAME
*     ck_event_number() -- test event numbers
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     static u_long32 
*     ck_event_number(lList *lp, u_long32 *waiting_for, 
*                                     u_long32 *wrong_number) 
*
*  FUNCTION
*     Tests list of events if it contains right numbered events.
*
*     Events with numbers lower than expected get trashed.
*
*     In cases the master has added no new events to the event list 
*     and the acknowledge we sent was lost also a list with events lower 
*     than "waiting_for" is correct. 
*     But the number of the last event must be at least "waiting_for"-1.
*
*     On success *waiting_for will contain the next number we wait for.
*
*     On failure *waiting_for gets not changed and if wrong_number is not
*     NULL *wrong_number contains the wrong number we got.
*
*  INPUTS
*     lList *lp              - event list to check
*     u_long32 *waiting_for  - next number to wait for
*     u_long32 *wrong_number - event number that causes a failure
*
*  RESULT
*     static u_long32 - 0 on success, else -1
*
*******************************************************************************/
static u_long32 
ck_event_number(lList *lp, u_long32 *waiting_for, u_long32 *wrong_number)
{
   lListElem *ep, *tmp;
   u_long32 i, j;
   int skipped;
  
   DENTER(TOP_LAYER, "ck_event_number");

   i = *waiting_for;

   if (!lp || !lGetNumberOfElem(lp)) {
      /* got a dummy event list for alive protocol */
      DPRINTF(("received empty event list\n"));
      DEXIT;
      return 0;
   }
   DPRINTF(("Checking %d events (" u32"-"u32 ") while waiting for #"u32"\n",
         lGetNumberOfElem(lp), 
         lGetUlong(lFirst(lp), ET_number),
         lGetUlong(lLast(lp), ET_number),
         i));
 
   /* ensure number of last event is "waiting_for"-1 or higher */ 
   if ((j=lGetUlong(lLast(lp), ET_number)) < i-1) {
      /* error in event numbers */
      /* could happen if the order of two event messages was exchanged */
      if (wrong_number)
         *wrong_number = j;

      ERROR((SGE_EVENT, MSG_EVENT_HIGHESTEVENTISXWHILEWAITINGFORY_UU , 
                  u32c(j), u32c(i)));
      /* 
      DEXIT;
      return -1;
      */
   }

   /* ensure number of first event is lower or equal "waiting_for" */
   if ((j=lGetUlong(lFirst(lp), ET_number)) > i) {
      /* error in event numbers */
      if (wrong_number)
         *wrong_number = j;
      ERROR((SGE_EVENT, MSG_EVENT_SMALLESTEVENTXISGRTHYWAITFOR_UU,
               u32c(j), u32c(i)));
      DEXIT;
      return -1;
   }

   /* skip leading events till event number is "waiting_for"
      or there are no more events */ 
   skipped = 0;
   ep = lFirst(lp);
   while (ep && lGetUlong(ep, ET_number) < i) {
      tmp = lNext(ep);
      lRemoveElem(lp, ep);
      ep = tmp;
      skipped++;
   }
   if (skipped)
      DPRINTF(("Skipped %d events, still %d in list\n", skipped, 
               lGetNumberOfElem(lp)));

   /* ensure number of events increase */
   for_each (ep, lp) {
      if ((j=lGetUlong(ep, ET_number)) != i++) {
         /* 
            do not change waiting_for because 
            we still wait for this number 
         */
         ERROR((SGE_EVENT, MSG_EVENT_EVENTSWITHNOINCREASINGNUMBERS ));
         if (wrong_number)
            *wrong_number = j;
         DEXIT;
         return -1;
      }
   }
 
   /* that's the new number we wait for */
   *waiting_for = i;
   DPRINTF(("check complete, %d events in list\n", lGetNumberOfElem(lp)));
   DEXIT;
   return 0;
}

/***i** Eventclient/Client/get_event_list() ***********************************
*  NAME
*     get_event_list() -- get event list via gdi call
*
*  SYNOPSIS
*     static int get_event_list(int sync, lList **report_list) 
*
*  FUNCTION
*     Tries to retrieve the event list.
*     Returns the incoming data and the commlib status/error code.
*     This function is used by ec_get.
*
*  INPUTS
*     int sync            - synchronous transfer
*     lList **report_list - pointer to returned list
*
*  RESULT
*     static int - commlib status/error code
*
*  SEE ALSO
*     Eventclient/Client/ec_get()
******************************************************************************/
static int 
get_event_list(int sync, lList **report_list)
{
   int tag, ret;
   u_short id;
   sge_pack_buffer pb;

   DENTER(TOP_LAYER, "get_event_list");

   id = 0;
   tag = TAG_REPORT_REQUEST;
   /* FIX_CONST */
   ret = sge_get_any_request((char*)sge_get_master(0), 
                             (char*)prognames[QMASTER], &id, &pb, &tag, sync);

   if (ret == 0) {
      if (cull_unpack_list(&pb, report_list)) {
         clear_packbuffer(&pb);
         ERROR((SGE_EVENT, MSG_LIST_FAILEDINCULLUNPACKREPORT ));
         DEXIT;
         return -1;
      }
      clear_packbuffer(&pb);
   }
   
   DPRINTF(("commlib returns %s (%d)\n", cl_errstr(ret), ret));

   DEXIT;
   return ret;
}


