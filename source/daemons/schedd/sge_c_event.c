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

#ifndef WIN32NATIVE
#	include <unistd.h>
#endif

#include "sge_gdi_intern.h"
#include "commlib.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_answerL.h"
#include "sge_eventL.h"
#include "sge_reportL.h"
#include "sge_c_event.h"
#include "qm_name.h"
#include "sge_log.h"
#include "sge_time.h"
#include "msg_schedd.h"
#include "sge_exit.h"

/****** Eventclient/--Introduction **********************************
*  NAME
*     Evenclient -- The Grid Engine Event Client Interface
*
*  FUNCTION
*     The Grid Engine Event Client Interface provides a means to connect 
*     to the Grid Engine qmaster and receive information about objects 
*     (actual object properties and changes).
*     It provides a subscribe/unsubscribe mechanism allowing fine grained
*     selection of objects per object types (e.g. jobs, queues, hosts)
*     and event types (e.g. add, modify, delete).
*     It should have much less impact on qmaster performance than polling
*     the same data in regular intervals.
*
*  NOTES
*     The current implementation is a generalized form of the event client
*     interface that already existed in Codine/GRD.
*     It still contains code handling scheduler specific behavior, like
*     the flushing of certain events. 
*     A further cleanup and generalization step should be done.
*     The code should also be extracted from qmaster, scheduler and libsched
*     directories and could either form a new event client library or 
*     be integrated into another lib, e.g. the libgdi.
*
*  SEE ALSO
*     Eventclient/Client/--Introduction
*     Eventclient/Server/--Introduction
*******************************************************************************/

/****** Eventclient/Client/--Introduction **********************************
*  NAME
*     Event Client Interface -- Client Functionality 
*
*  SYNOPSIS
*     int ec_prepare_registration(u_long32 id, const char *name);
*     int ec_register(void);
*     int ec_deregister(void);
*     
*     int ec_subscribe(int event);
*     int ec_subscribe_all(void);
*     
*     int ec_unsubscribe(int event);
*     int ec_unsubscribe_all(void);
*     
*     int ec_set_edtime(int intval);
*     int ec_get_edtime(void);
*     
*     int ec_commit(void);
*     
*     int ec_get(lList **);
*     
*     void ec_mark4registration(void);
*     int ec_need_new_registration(void);
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
*     Eventclient/--Introduction
*******************************************************************************/

static u_long32 ck_event_number(lList *lp, u_long32 *waiting_for, u_long32 *wrong_number);
static int get_event_list(int sync, lList **lp);
static void ec_config_changed(void);

/****** Eventclient/Client/-Global_Variables **********************************
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
*     config_changed - the configuration changed (subscription or event interval)
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

/****** Eventclient/Client/ec_prepare_registration() **********************************
*  NAME
*     ec_prepare_registration() -- prepare registration at server
*
*  SYNOPSIS
*     int ec_prepare_registration(u_long32 id, const char *name) 
*
*  FUNCTION
*     Initializes necessary data structures and sets the data needed for
*     registration at an event server.
*     The events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN are subscribed.
*
*     For valid id's see ....
*     The name is informational data that will be included in messages
*     (errors, warnings, infos) and will be shown in the command line tool
*     qconf -sec.
*
*  INPUTS
*     u_long32 id      - id used to register
*     const char *name - name of the event client
*
*  RESULT
*     int - 1,if the function succeeded, else 0 
*
*  SEE ALSO
*     qconf manpage
*     list of events
*     list of ids
*******************************************************************************/
int ec_prepare_registration(u_long32 id, const char *name)
{
   char subscription[sgeE_EVENTSIZE + 1];
   int i;

   DENTER(TOP_LAYER, "ec_prepare_registration");

   if(id >= EV_ID_FIRST_DYNAMIC || name == NULL || *name == 0) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGAL_ID_OR_NAME_US, u32c(id), name != NULL ? name : "NULL" ));
      DEXIT;
      return 0;
   }

   ec = lCreateElem(EV_Type);

   if(ec == NULL) {
      DEXIT; /* error message already comes from lCreateElem */
      return 0;
   }

   /* remember registration id for subsequent registrations */
   ec_reg_id = id;

   /* initialize event client object */
   lSetString(ec, EV_name, name);
   lSetUlong(ec, EV_d_time, DEFAULT_EVENT_DELIVERY_INTERVAL);

   /* initialize subscription "bitfield" */
   for(i = 0; i < sgeE_EVENTSIZE; i++) {
      subscription[i] = '0';
   }

   subscription[sgeE_EVENTSIZE] = 0;

   lSetString(ec, EV_subscription, subscription);
   ec_subscribe(sgeE_QMASTER_GOES_DOWN);
   ec_subscribe(sgeE_SHUTDOWN);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_mark4registration() *************************************
*  NAME
*     ec_mark4registration() -- new registration is required
*
*  SYNOPSIS
*     void ec_mark4registration(void) 
*
*  FUNCTION
*     Tells the event client mechanism, that the connection to the server
*     is broken and it has to reregister.
*
*  NOTES
*     Should be no external interface. The event client mechanism should itself
*     detect such situations and react accordingly.
*     Should not reference a global variable from scheduler (new_global_config)!
*
*  SEE ALSO
*     Eventclient/Client/ec_need_new_registration()
*******************************************************************************/
void ec_mark4registration(void)
{
   extern int new_global_config;
   DENTER(TOP_LAYER, "ec_mark4registration");
   need_register = 1;
   new_global_config = 1;
   DEXIT;
}

/****** Eventclient/Client/ec_need_new_registration() *********************************
*  NAME
*     ec_need_new_registration() -- is a reregistration neccessary
*
*  SYNOPSIS
*     int ec_need_new_registration(void) 
*
*  FUNCTION
*     Function to check, if a new registration at the server is neccessary.
*
*  RESULT
*     int - 1, if the client has to (re)register, else 0 
*
*******************************************************************************/
int ec_need_new_registration(void)
{
   return need_register;
}

/* return 1 if parameter has changed 
          0 otherwise                */
/****** Eventclient/Client/ec_set_edtime() ********************************************
*  NAME
*     ec_set_edtime() -- set the event delivery interval
*
*  SYNOPSIS
*     int ec_set_edtime(int interval) 
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
int ec_set_edtime(int interval) {
   int ret;

   DENTER(TOP_LAYER, "ec_set_edtime");
   
   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   ret = (lGetUlong(ec, EV_d_time) != interval);

   if(ret) {
      lSetUlong(ec, EV_d_time, interval);
      ec_config_changed();
   }

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_get_edtime() ********************************************
*  NAME
*     ec_get_edtime() -- get the current event delivery interval
*
*  SYNOPSIS
*     int ec_get_edtime(void) 
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
int ec_get_edtime(void) {
   int interval;

   DENTER(TOP_LAYER, "ec_get_edtime");

   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   interval = lGetUlong(ec, EV_d_time);
   DEXIT;
   return interval;
}

/****** Eventclient/Client/ec_register() **********************************************
*  NAME
*     ec_register() -- register at the event server
*
*  SYNOPSIS
*     int ec_register(void) 
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
int ec_register(void)
{
   int success = 0;
   lList *lp, *alp;
   lListElem *aep;

   DENTER(TOP_LAYER, "ec_register");

   if(ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
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

      if(new_id != 0) {
         lSetUlong(ec, EV_id, new_id);
         DPRINTF(("REGISTERED with id "U32CFormat"\n", new_id));
         lFreeList(lp); 
         lFreeList(alp);

         config_changed = 0;
         
         DEXIT;
         return 1;
      }
   } else {
      if (lGetUlong(aep, AN_quality) == NUM_AN_ERROR) {
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

/****** Eventclient/Client/ec_deregister() ********************************************
*  NAME
*     ec_deregister() -- deregister from the event server
*
*  SYNOPSIS
*     int ec_deregister(void) 
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
*     int - 1, if the deregistration succeeded, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_register()
*******************************************************************************/
int ec_deregister(void)
{
   int ret;
   sge_pack_buffer pb;

   DENTER(TOP_LAYER, "ec_deregister");

   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if(init_packbuffer(&pb, sizeof(u_long32), 0) != PACK_SUCCESS) {
      /* error message is output from init_packbuffer */
      DEXIT;
      return 0;
   }

   packint(&pb, lGetUlong(ec, EV_id));

   ret = sge_send_any_request(0, NULL, sge_get_master(0), 
         prognames[QMASTER], 1, &pb, TAG_EVENT_CLIENT_EXIT);
   
   clear_packbuffer(&pb);

   if(ret != CL_OK) {
      /* error message is output from sge_send_any_request */
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_subscribe() *********************************************
*  NAME
*     ec_subscribe() -- Subscribe an event
*
*  SYNOPSIS
*     int ec_subscribe(int event) 
*
*  FUNCTION
*     Subscribe a certain event.
*     See ... for a list of all events.
*     The subscription will be in effect after calling ec_commit or ec_get.
*     It is possible / sensible to subscribe all events wanted and then call
*     ec_commit.
*
*  INPUTS
*     int event - the event number
*
*  RESULT
*     int - 1 on success, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int ec_subscribe(int event)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_subscribe");

   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if(event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if(subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if(event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         subscription[i] = '1';
      }
   } else {
      subscription[event] = '1';
   }

   if(strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }

   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_subscribe_all() *****************************************
*  NAME
*     ec_subscribe_all() -- subscribe all events
*
*  SYNOPSIS
*     int ec_subscribe_all(void) 
*
*  FUNCTION
*     Subscribe all possible event.
*     The subscription will be in effect after calling ec_commit or ec_get.
*
*  RESULT
*     int - 1 on success, else 0
*
*  NOTES
*     Subscribing all events can cause a lot of traffic and may decrease performance
*     of qmaster.
*     Only subscribe all events, if you really need them.
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int ec_subscribe_all(void)
{
   return ec_subscribe(sgeE_ALL_EVENTS);
}

/****** Eventclient/Client/ec_unsubscribe() *******************************************
*  NAME
*     ec_unsubscribe() -- unsubscribe an event
*
*  SYNOPSIS
*     int ec_unsubscribe(int event) 
*
*  FUNCTION
*     Unsubscribe a certain event.
*     See ... for a list of all events.
*     The change will be in effect after calling ec_commit or ec_get.
*     It is possible / sensible to unsubscribe all events 
*     no longer needed and then call ec_commit.
*
*  INPUTS
*     int event - the event to unsubscribe
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
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int ec_unsubscribe(int event)
{
   char *subscription;
   
   DENTER(TOP_LAYER, "ec_unsubscribe");

   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if(event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
      DEXIT;
      return 0;
   }

   subscription = strdup(lGetString(ec, EV_subscription));

   if(subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   if(event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         subscription[i] = '0';
      }
      subscription[sgeE_QMASTER_GOES_DOWN] = '1';
      subscription[sgeE_SHUTDOWN] = '1';
   } else {
      if(event == sgeE_QMASTER_GOES_DOWN || event == sgeE_SHUTDOWN) {
         ERROR((SGE_EVENT, MSG_EVENT_HAVETOHANDLEEVENTS));
         free(subscription);
         DEXIT;
         return 0;
      } else {
         subscription[event] = '0';
      }
   }

   if(strcmp(subscription, lGetString(ec, EV_subscription)) != 0) {
      lSetString(ec, EV_subscription, subscription);
      ec_config_changed();
   }
   
   free(subscription);

   DEXIT;
   return 1;
}

/****** Eventclient/Client/ec_unsubscribe_all() ***************************************
*  NAME
*     ec_unsubscribe_all() -- unsubscribe all events
*
*  SYNOPSIS
*     int ec_unsubscribe_all(void) 
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
int ec_unsubscribe_all(void)
{
   return ec_unsubscribe(sgeE_ALL_EVENTS);
}

/****** Eventclient/Client/ec_config_changed() ****************************************
*  NAME
*     ec_config_changed() -- has the configuration changed?
*
*  SYNOPSIS
*     static void ec_config_changed(void) 
*
*  FUNCTION
*     Checkes whether the configuration has changes.
*     Configuration changes can either be changes in the subscription
*     or change of the event delivery interval.
*
*  RESULT
*     static void - 1, if the config has been changed, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_set_edtime()
*******************************************************************************/
static void ec_config_changed(void) 
{
   config_changed = 1;
}

/****** Eventclient/Client/ec_commit() ************************************************
*  NAME
*     ec_commit() -- commit configuration changes
*
*  SYNOPSIS
*     int ec_commit(void) 
*
*  FUNCTION
*     Configuration changes (subscription and/or event delivery time) will be
*     sent to the event server.
*     The function should be called after (multiple) configuration changes 
*     have been made.
*     If it is not explicitly called by the event client program, the next 
*     call of ec_get will commit configuration changes before looking for new
*     events.
*
*  RESULT
*     int - 1 on success, else 0
*
*  SEE ALSO
*     Eventclient/Client/ec_config_changed()
*     Eventclient/Client/ec_get()
*******************************************************************************/
int ec_commit(void) 
{
   int success;
   lList *lp, *alp;

   DENTER(TOP_LAYER, "ec_commit");

   /* not yet initialized? Cannot send modification to qmaster! */
   if(ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      DPRINTF((MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return 0;
   }

   /* not (yet) registered? Cannot send modification to qmaster! */
   if(ec_need_new_registration()) {
      DPRINTF((MSG_EVENT_NOTREGISTERED));
      DEXIT;
      return 0;
   }

   if(!config_changed) {
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
      DPRINTF(("CHANGE CONFIGURATION SUCCEEDED\n"));
      DEXIT;
      return 1;
   }

   DPRINTF(("CHANGE CONFIGURATION FAILED\n"));
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------*
 * ec_get
 * 
 * returns
 *    0 ok got events or got empty event list or got no message
 *   <0 commlib error
 *-------------------------------------------------------------------*/
/****** Eventclient/Client/ec_get() ***************************************************
*  NAME
*     ec_get() -- look for new events
*
*  SYNOPSIS
*     int ec_get(lList **event_list) 
*
*  FUNCTION
*     ec_get looks for new events. 
*     If new events have arrived, they are passed back to the caller in the
*     parameter event_list.
*     
*     If the event client is not yet registered at the event server, 
*     the registration will be done before looking for events.
*
*     If the configuration changed since the last call of ec_get and has
*     not been committed, ec_commit will be called before looking for
*     events.
*
*  INPUTS
*     lList **event_list - pointer to an event list to hold arriving events
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
*
*******************************************************************************/
int ec_get(
lList **event_list
) {  
   int ret;
   lList *report_list = NULL;
   static u_long32 next_event = 1;
   u_long32 wrong_number;
   int sync;
   
   DENTER(TOP_LAYER, "ec_get");
 
   if(ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return -1;
   }
 
   if (ec_need_new_registration()) {
      next_event = 1;
      if (!ec_register()) {
         DEXIT;
         return -1;
      }
      else
         need_register = 0;
   }
      
   if(config_changed) {
      ec_commit();
   }
      
   /* receive_message blocks in the first loop */
   for (sync = 1; !(ret = get_event_list(sync, &report_list)); sync = 0) {
      
      lList *new_events = NULL;
      lXchgList(lFirst(report_list), REP_list, &new_events);
      report_list = lFreeList(report_list);

      if (ck_event_number(new_events, &next_event, &wrong_number)) {
         /*
          *  may be got an old event, that was sent before
          *  reregistration at qmaster
          */
         lFreeList(*event_list);
         lFreeList(new_events);
         ec_mark4registration();
         DEXIT;
         return -1;
      }

      DPRINTF(("got events till "u32"\n", next_event-1));

      /* append new_events to event_list */
      if (*event_list) 
         lAddList(*event_list, new_events);
      else 
         *event_list = new_events;
   }

   /* commlib error during first synchronous get_event_list() */
   if (sync && ret) {
      DEXIT;
      return ret;
   }
      
   /* send an ack to the qmaster for all received events */
   if (!sync) {
      if (sge_send_ack_to_qmaster(0, ACK_EVENT_DELIVERY, next_event-1, lGetUlong(ec, EV_id))) {
         WARNING((SGE_EVENT, MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY ));
      }
      else
         DPRINTF(("Sent ack for all events lower or equal %d\n",
                 (next_event-1)));
   }

   DEXIT;
   return 0;
}


/****** Eventclient/Client/ck_event_number() ******************************************
*  NAME
*     ck_event_number() -- test event numbers
*
*  SYNOPSIS
*     static u_long32 ck_event_number(lList *lp, u_long32 *waiting_for, 
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
static u_long32 ck_event_number(
lList *lp,
u_long32 *waiting_for,
u_long32 *wrong_number 
) {
   lListElem *ep, *tmp;
   u_long32 i, j;
   int skipped;
  
   DENTER(TOP_LAYER, "ck_event_number");

   i = *waiting_for;

   if (!lp || !lGetNumberOfElem(lp)) {
      /* got a dummy event list for alive protocol */
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
      DPRINTF(("Skipped %d events\n", skipped));

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

   DEXIT;
   return 0;
}


/*-----------------------------------------------------------------------*
 * get_event_list
 *
 * returns   0 on success
 *           commlib error (always positive)
 *-----------------------------------------------------------------------*/
/***i** Eventclient/Client/get_event_list() *******************************************
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
*******************************************************************************/
static int get_event_list(
int sync,
lList **report_list 
) {
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
