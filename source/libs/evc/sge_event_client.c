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

#include "sge_any_request.h"
#include "sge_ack.h"
#include "sge_unistd.h"
#include "commlib.h"
#include "commproc.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_profiling.h"
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
/****** Eventclient/-ID-numbers ***************************************
*
*  NAME
*     ID-numbers -- id numbers for registration
*
*  SYNOPSIS
*     #include <sge_eventL.h>
*
*  FUNCTION
*     Each event client registered at qmaster has a unique client id.
*     The request for registering at qmaster contains either a concrete
*     id the client wants to occupy, or it asks the qmaster to assign
*     an id.
*     The client sets the id to use at registration with the 
*     ec_prepare_registration function call.
*     
*     The following id's can be used:
*        EV_ID_ANY    - qmaster will assign a unique id
*        EV_ID_SCHEDD - register at qmaster as scheduler
*
*  NOTES
*     As long as an event client does not expect any special handling 
*     within qmaster, it should let qmaster assign an id.
*
*     If a client expects special handling, a new id in the range 
*     [2;10] has to be created and the special handling has to be
*     implemented in qmaster (probably in sge_event_master.c).
*
*  SEE ALSO
*     Eventclient/Client/ec_prepare_registration()
*     Eventclient/Server/sge_add_event_client()
****************************************************************************
*/
/****** Eventclient/-Subscription ***************************************
*
*  NAME
*     Subscription -- Subscription interface for event clients
*
*  FUNCTION
*     An event client is notified, if certain event conditions are 
*     raised in qmaster.
*
*     Which events an event client is interested in can be set through
*     the subscription interface.
*
*     Events have a unique event identification that classifies them, 
*     e.g. "a job has been submitted" or "a queue has been disabled".
*
*     Delivery of events can be switched on/off for each event id
*     individually.
*
*     Subscription can be changed dynamically at runtime through the
*     ec_subscribe/ec_unsubscribe functions.
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Events
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_subscribe_flush()
****************************************************************************
*/

/****** Eventclient/-Flushing ***************************************
*
*  NAME
*     Flushing -- Configuration of event flushing
*
*  FUNCTION
*     In the standard configuration, qmaster will deliver events
*     in certain intervals to event clients.
*     These intervals can be configured using the ec_set_edtime()
*     function call.
*
*     In certain cases, an event client will want to be immediately 
*     notified, if certain events occur. A scheduler for example
*     could be notified immediately, if resources in a cluster become
*     available to allow instant refilling of the cluster.
*
*     The event client interface allows to configure flushing 
*     for each individual event id.
*
*     Flushing can either be switched off (default) for an event, or
*     a delivery time can be configured. 
*     If a delivery time is configured, events for the event client
*     will be delivered by qmaster at latest after this delivery time.
*     A delivery time of 0 means instant delivery.
*
*     Flushing can be changed dynamically at runtime through the
*     ec_set_flush/ec_unset_flush functions.
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_set_edtime()
*     Eventclient/Client/ec_set_flush()
*     Eventclient/Client/ec_unset_flush()
*     Eventclient/Client/ec_subscribe_flush()
****************************************************************************
*/

/****** Eventclient/-List filtering***************************************
*
*  NAME
*     List filtering -- Configuration of the list filtering 
*
*  FUNCTION
*    The date send with an event can be filtered on the master side.
*    Therefore one can set a where and what condition. If the
*    all client date is removed via where condition, no event will
*    be send.
*
*    The method expects a lListElem representation of the lCondition
*    and lEnumeration. The two methods: "lWhatToElem" and 
*    "lWhereToElem" will convert the structures.
*
*  NOTES
*    One has to be carefull reducing the elements via what condition.
*    One has to ensure, that all elements have a custom descriptor
*    and that not elements with different descriptors are mixed in
*    the same list.
*
*    The master and client can benifit (in speed and memory consumption)
*    a lot by requesting only the data, the client needs.
*
*    All registered events for the same cull data structure needs to have
*    the same what and where filter.
*
*    The JAT_Type list is handled special, because it is subscribable as
*    an event, and it is also a sub-structure in the JB_Type list. When
*    a JAT_Type filter is set, the JAT-Lists in the JB-List are filtered
*    as well.
*
*  SEE ALSO
*     Eventclient/Client/ec_mod_subscription_where()
*     cull/cull_what/lWhatToElem()
*     cull/cull_what/lWhatFromElem()
*     cull/cull_where/lWhereToElem()
*     cull/cull_where/lWhereFromElem()
****************************************************************************
*/

/****** Eventclient/-Busy-state ***************************************
*
*  NAME
*     Busy-state -- Handling of busy event clients
*
*  FUNCTION
*     An event client may have time periods where it is busy doing some
*     processing and can not accept new events.
*     
*     In this case, qmaster should not send out new events but spool them,
*     as otherwise timeouts would occur waiting for acknowledges.
*
*     Therefore an event client can set a policy that describes how busy
*     states are set and unset.
*
*     Which policy to use is set with the ev_set_busy_handling() function 
*     call. The policy can be changed dynamically at runtime.
*
*     The following policies are implemented at present:
*        EV_BUSY_NO_HANDLING    - busy state is not handled automatically
*                                 by the event client interface
*        EV_BUSY_UNTIL_ACK      - when delivering events qmaster will set
*                                 the eventclient to busy and will unset 
*                                 the busy state when the event client
*                                 acknowledges receipt of the events.
*        EV_BUSY_UNTIL_RELEASED - when delivering events qmaster will set
*                                 the eventclient to busy.
*                                 It will stay in the busy state until it
*                                 is explicitly released by the client 
*                                 (calling ec_set_busy(0)) 
*        EV_THROTTLE_FLUSH      - when delivering events qmaster sends
*                                 events in the regular event delivery 
*                                 intervals. Each time events are sent the 
*                                 busy counter (EV_busy) is increased. 
*                                 The busy counter is set to 0 only when events 
*                                 are acknowledged by the event client. Event 
*                                 flushing is delayed depending on the busy 
*                                 counter the more event flushing is delayed.
*                                 
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_set_busy()
****************************************************************************
*/
/****** Eventclient/-Session filtering *************************************
*
*  NAME
*     Session filtering -- Filtering of events using a session key.
*
*  FUNCTION
*     For JAPI event clients event subscription is not sufficient to
*     track only those events that are related to a JAPI session.
*     
*     When session filtering is used the following rules are applied 
*     to decide whether subscribed events are sent or not:
*
*     - Events that are associated with a specific session are not
*       sent when session filtering is used but the session does not 
*       match. 
*     - Events that are not associated with a specific session are 
*       not sent. The only exception are total update events. If subscribed
*       these events are always sent and a more fine grained filtering is 
*       applied.
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_set_session()
*     Eventclient/Client/ec_get_session()
****************************************************************************
*/
/****** Eventclient/--EV_Type ***************************************
*
*  NAME
*     EV_Type -- event client object
*
*  ELEMENTS
*     SGE_ULONG(EV_id)
*        event client id (see Eventclient/-ID-numbers)
*
*     SGE_STRING(EV_name)
*        event client name (any string characterizing the client)
*     
*     SGE_HOST(EV_host)
*        host on which the event client resides
*
*     SGE_STRING(EV_commproc)
*        addressing information
*
*     SGE_ULONG(EV_commid)
*        unique id of the event client in commd
*
*     SGE_ULONG(EV_uid)
*        user id under which the event client is running
*     
*     SGE_ULONG(EV_d_time)
*        event delivery time (interval used by qmaster to deliver events)
*
*     SGE_List(EV_subscribed)
*        information about subscription and flushing
*        (see Eventclient/-Subscription and Eventclient/-Flushing)
*
*     SGE_ULONG(EV_busy_handling)
*        information about handling of busy states
*        (see Eventclient/-Busy-state)
*     
*     SGE_ULONG(EV_last_heard_from)
*        time when qmaster heard from the event client for the last time
*
*     SGE_ULONG(EV_last_send_time)
*        time of the last delivery of events
*
*     SGE_ULONG(EV_next_send_time)
*        next time scheduled for the delivery of events
*
*     SGE_ULONG(EV_next_number)
*        next sequential number for events (each event gets a unique number)
*
*     SGE_ULONG(EV_busy)
*        is the event client busy? (0 = false, 1 = true)
*        no events will be delivered to a busy client
*
*     SGE_LIST(EV_events)
*        list of events - they will be spooled until the event client 
*        acknowledges receipt
*  
*     SGE_ULONG(EV_clientdata)
*        a 32bit ulong for use by the event client or special handling 
*        for certain event clients
*
*  FUNCTION
*     An event client creates and initializes such an object and passes
*     it to qmaster for registration.
*     Qmaster will fill some of the fields (e.g. the EV_id) and send
*     the object back to the event client on successful registration.
*     Whenever the event client wants to change some configuration
*     parameters, it changes the object and sends it to qmaster
*     with a modification request.
*  
*     Qmaster uses the object internally to track sequential numbers, 
*     delivery times, timeouts etc.
*
*  SEE ALSO
*     Eventclient/--Event_Client_Interface
*     Eventclient/-ID-numbers
*     Eventclient/-Subscription
*     Eventclient/-Flushing
*     Eventclient/-Busy-state
****************************************************************************
*/
/****** Eventclient/-Events ***************************************
*
*  NAME
*     Events -- events available from qmaster
*
*  FUNCTION
*     The following events can be raised in qmaster and be subscribed
*     by an event client:
*
*        sgeE_ALL_EVENTS                  
*     
*        sgeE_ADMINHOST_LIST              send admin host list at registration
*        sgeE_ADMINHOST_ADD               event add admin host
*        sgeE_ADMINHOST_DEL               event delete admin host
*        sgeE_ADMINHOST_MOD               event modify admin host
*     
*        sgeE_CALENDAR_LIST               send calendar list at registration
*        sgeE_CALENDAR_ADD                event add calendar
*        sgeE_CALENDAR_DEL                event delete calendar
*        sgeE_CALENDAR_MOD                event modify calendar
*     
*        sgeE_CKPT_LIST                   send ckpt list at registration
*        sgeE_CKPT_ADD                    event add ckpt
*        sgeE_CKPT_DEL                    event delete ckpt
*        sgeE_CKPT_MOD                    event modify ckpt
*     
*        sgeE_CENTRY_LIST                 send complex entry list at reg.
*        sgeE_CENTRY_ADD                  event add complex entry
*        sgeE_CENTRY_DEL                  event delete complex entry
*        sgeE_CENTRY_MOD                  event modify complex entry
*     
*        sgeE_CONFIG_LIST                 send config list at registration
*        sgeE_CONFIG_ADD                  event add config
*        sgeE_CONFIG_DEL                  event delete config
*        sgeE_CONFIG_MOD                  event modify config
*     
*        sgeE_EXECHOST_LIST               send exec host list at registration
*        sgeE_EXECHOST_ADD                event add exec host
*        sgeE_EXECHOST_DEL                event delete exec host
*        sgeE_EXECHOST_MOD                event modify exec host
*     
*        sgeE_GLOBAL_CONFIG               global config changed, replace by sgeE_CONFIG_MOD
*     
*        sgeE_JATASK_ADD                  event add array job task
*        sgeE_JATASK_DEL                  event delete array job task
*        sgeE_JATASK_MOD                  event modify array job task
*     
*        sgeE_PETASK_ADD,                 event add a new pe task
*        sgeE_PETASK_DEL,                 event delete a pe task
*
*        sgeE_JOB_LIST                    send job list at registration
*        sgeE_JOB_ADD                     event job add (new job)
*        sgeE_JOB_DEL                     event job delete
*        sgeE_JOB_MOD                     event job modify
*        sgeE_JOB_MOD_SCHED_PRIORITY      event job modify priority
*        sgeE_JOB_USAGE                   event job online usage
*        sgeE_JOB_FINAL_USAGE             event job final usage report after job end
*        sgeE_JOB_FINISH                  job finally finished or aborted (user view) 
*        sgeE_JOB_SCHEDD_INFO_LIST        send job schedd info list at registration
*        sgeE_JOB_SCHEDD_INFO_ADD         event jobs schedd info added
*        sgeE_JOB_SCHEDD_INFO_DEL         event jobs schedd info deleted
*        sgeE_JOB_SCHEDD_INFO_MOD         event jobs schedd info modified
*     
*        sgeE_MANAGER_LIST                send manager list at registration
*        sgeE_MANAGER_ADD                 event add manager
*        sgeE_MANAGER_DEL                 event delete manager
*        sgeE_MANAGER_MOD                 event modify manager
*     
*        sgeE_OPERATOR_LIST               send operator list at registration
*        sgeE_OPERATOR_ADD                event add operator
*        sgeE_OPERATOR_DEL                event delete operator
*        sgeE_OPERATOR_MOD                event modify operator
*     
*        sgeE_NEW_SHARETREE               replace possibly existing share tree
*     
*        sgeE_PE_LIST                     send pe list at registration
*        sgeE_PE_ADD                      event pe add
*        sgeE_PE_DEL                      event pe delete
*        sgeE_PE_MOD                      event pe modify
*     
*        sgeE_PROJECT_LIST                send project list at registration
*        sgeE_PROJECT_ADD                 event project add
*        sgeE_PROJECT_DEL                 event project delete
*        sgeE_PROJECT_MOD                 event project modify
*     
*        sgeE_QMASTER_GOES_DOWN           qmaster notifies all event clients, before
*                                         it exits
*     
*        sgeE_QUEUE_LIST                  send queue list at registration
*        sgeE_QUEUE_ADD                   event queue add
*        sgeE_QUEUE_DEL                   event queue delete
*        sgeE_QUEUE_MOD                   event queue modify
*        sgeE_QUEUE_SUSPEND_ON_SUB        queue is suspended by subordinate mechanism
*        sgeE_QUEUE_UNSUSPEND_ON_SUB      queue is unsuspended by subordinate mechanism
*     
*        sgeE_SCHED_CONF                  replace existing (sge) scheduler configuration
*     
*        sgeE_SCHEDDMONITOR               trigger scheduling run
*     
*        sgeE_SHUTDOWN                    request shutdown of an event client
*     
*        sgeE_SUBMITHOST_LIST             send submit host list at registration
*        sgeE_SUBMITHOST_ADD              event add submit host
*        sgeE_SUBMITHOST_DEL              event delete submit host
*        sgeE_SUBMITHOST_MOD              event modify submit host
*     
*        sgeE_USER_LIST                   send user list at registration
*        sgeE_USER_ADD                    event user add
*        sgeE_USER_DEL                    event user delete
*        sgeE_USER_MOD                    event user modify
*     
*        sgeE_USERSET_LIST                send userset list at registration
*        sgeE_USERSET_ADD                 event userset add
*        sgeE_USERSET_DEL                 event userset delete
*        sgeE_USERSET_MOD                 event userset modify
*  
*     If user mapping is enabled (compile time option), the following
*     additional events can be subscribed:
*
*        sgeE_CUSER_ENTRY_LIST            send list of user mappings
*        sgeE_CUSER_ENTRY_ADD             a new user mapping was added
*        sgeE_CUSER_ENTRY_DEL             a user mapping was deleted
*        sgeE_CUSER_ENTRY_MOD             a user mapping entry was changed
*     
*        sgeE_HGROUP_LIST                 send list of host groups
*        sgeE_HGROUP_ADD                  a host group was added
*        sgeE_HGROUP_DEL                  a host group was deleted
*        sgeE_HGROUP_MOD                  a host group was changed
*
*  NOTES
*     This list of events will increase as further event situations
*     are identified and interfaced.
*     
*     IF YOU ADD EVENTS HERE, ALSO UPDATE sge_mirror!
*
*  SEE ALSO
*     Eventclient/-Subscription
****************************************************************************
*/

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
bool ck_event_number(lList *lp, u_long32 *waiting_for, 
                     u_long32 *wrong_number);

static bool 
get_event_list(int sync, lList **lp);

static void ec_add_subscriptionElement(lListElem *event_el, ev_event event, bool flush, int interval); 

static void ec_remove_subscriptionElement(lListElem *event_el, ev_event event); 

static void ec_mod_subscription_flush(lListElem *event_el, ev_event event, bool flush, 
                                       int intervall);
                                      
static void ec_config_changed(void);

/****** Eventclient/Client/-Event_Client_Global_Variables *********************
*  NAME
*     Global_Variables -- global variables in the client 
*
*  SYNOPSIS
*     static bool config_changed = false;
*     static bool need_register  = true;
*     static lListElem *ec      = NULL;
*     static u_long32 ec_reg_id = 0;
*     static u_long32 next_event = 1;
*
*  FUNCTION
*     config_changed - the configuration changed (subscription or event 
*                      interval)
*     need_register  - the client is not registered at the server
*     ec             - event client object holding configuration data
*     ec_reg_id      - the id used to register at the qmaster
*     next_event     - the sequence number of the next event we are waiting for
*
*  NOTES
*     These global variables should be part of the event client object.
*     The only remaining global variable would be a list of event client objects,
*     allowing one client to connect to multiple event client servers.
*
*******************************************************************************/
static bool config_changed  = false;
static bool need_register   = true;
static lListElem *ec       = NULL;
static u_long32 ec_reg_id  = 0;
static u_long32 next_event = 1;

/****** Eventclient/Client/ec_prepare_registration() ***************************
*  NAME
*     ec_prepare_registration() -- prepare registration at server
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true, if the function succeeded, else false 
*
*  SEE ALSO
*     Eventclient/--EV_Type
*     Eventclient/-ID-numbers
*     Eventclient/-Subscription
*     qconf manpage
*     list of events
*******************************************************************************/
bool 
ec_prepare_registration(ev_registration_id id, const char *name)
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_prepare_registration");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (id >= EV_ID_FIRST_DYNAMIC || name == NULL || *name == 0) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGAL_ID_OR_NAME_US, 
               u32c(id), name != NULL ? name : "NULL" ));
   } else {
      ec = lCreateElem(EV_Type);

      if (ec != NULL) {
         /* remember registration id for subsequent registrations */
         ec_reg_id = id;

         /* initialize event client object */
         lSetString(ec, EV_name, name);
         lSetUlong(ec, EV_d_time, DEFAULT_EVENT_DELIVERY_INTERVAL);

         ec_subscribe_flush(sgeE_QMASTER_GOES_DOWN, 0);
         ec_subscribe_flush(sgeE_SHUTDOWN, 0);

         ec_set_busy_handling(EV_BUSY_NO_HANDLING);
         lSetUlong(ec, EV_busy, 0);

         ret = true;
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
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
*     int - true, if the event client interface has been initialized,
*           else false.
*
*  SEE ALSO
*     Eventclient/Client/ec_prepare_registration()
*******************************************************************************/
bool 
ec_is_initialized(void) 
{
   if (ec == NULL) {
      return false;
   } else {
      return true;
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
   need_register = true;
   lSetBool(ec, EV_changed, true);
   DEXIT;
}

/****** Eventclient/Client/ec_need_new_registration() *************************
*  NAME
*     ec_need_new_registration() -- is a reregistration neccessary?
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool ec_need_new_registration(void) 
*
*  FUNCTION
*     Function to check, if a new registration at the server is neccessary.
*
*  RESULT
*     bool - true, if the client has to (re)register, else false 
*
*******************************************************************************/
bool 
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
*     Any number > 0 is a valid interval in seconds. However the interval 
*     my not be larger than the commd commproc timeout. Otherwise the event 
*     client encounters receive timeouts and this is returned as an error 
*     by ec_get().
*
*  INPUTS
*     int interval - interval [s]
*
*  RESULT
*     int - 1, if the value was changed, else 0
*
*  NOTES
*     The maximum interval is limited to the commd commproc timeout.
*     The maximum interval should be limited also by the application. 
*     A too big interval makes qmaster spool lots of events and consume 
*     a lot of memory.
*
*  SEE ALSO
*     Eventclient/Client/ec_get_edtime()
*******************************************************************************/
int ec_set_edtime(int interval) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "ec_set_edtime");
   
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      ret = (lGetUlong(ec, EV_d_time) != interval);

      if (ret > 0) {
#ifdef ENABLE_NGC
         lSetUlong(ec, EV_d_time, MIN(interval, CL_DEFINE_CLIENT_CONNECTION_LIFETIME-5));
#else
         lSetUlong(ec, EV_d_time, MIN(interval, COMMPROC_TIMEOUT-5));
#endif
         ec_config_changed();
      }
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
   int interval = 0;

   DENTER(TOP_LAYER, "ec_get_edtime");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      interval = lGetUlong(ec, EV_d_time);
   }

   DEXIT;
   return interval;
}

/****** Eventclient/Client/ec_set_flush_delay() *****************************
*  NAME
*     ec_set_flush_delay() -- set flush delay parameter
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
*     ec_set_flush_delay(u_long32 flush_delay) 
*
*  FUNCTION
*
*  INPUTS
*     int flush_delay - flush delay parameter
*
*  RESULT
*     bool - true, if the value was changed, else false
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_get_flush_delay()
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_set_flush_delay(int flush_delay) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_set_flush_delay");
   
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      ret = (lGetUlong(ec, EV_flush_delay) != flush_delay);

      if (ret) {
         lSetUlong(ec, EV_flush_delay, flush_delay);
         ec_config_changed();
      }
   }

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_get_flush_delay() *****************************
*  NAME
*     ec_get_flush_delay() -- get configured flush delay paramter
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     int 
*     ec_get_flush_delay(void) 
*
*  FUNCTION
*     Returns the policy currently configured.
*
*  RESULT
*     flush_delay - current flush delay parameter setting
*
*  SEE ALSO
*     Eventclient/Client/ec_set_flush_delay()
*     Eventclient/-Busy-state
*******************************************************************************/
int 
ec_get_flush_delay(void) 
{
   int flush_delay = 0;

   DENTER(TOP_LAYER, "ec_get_flush_delay");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      flush_delay = lGetUlong(ec, EV_flush_delay);
   }

   DEXIT;
   return flush_delay;
}


/****** Eventclient/Client/ec_set_busy_handling() *****************************
*  NAME
*     ec_set_edtime() -- set the event client busy handling
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true, if the value was changed, else false
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_get_busy_handling()
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_set_busy_handling(ev_busy_handling handling) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_set_busy_handling");
   
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      ret = (lGetUlong(ec, EV_busy_handling) != handling);

      if (ret) {
         lSetUlong(ec, EV_busy_handling, handling);
         ec_config_changed();
      }
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
   ev_busy_handling handling = EV_BUSY_NO_HANDLING;

   DENTER(TOP_LAYER, "ec_get_busy_handling");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      handling = lGetUlong(ec, EV_busy_handling);
   }

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
   } else {
      lSetUlong(ec, EV_clientdata, data);
   }

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
   u_long32 clientdata = 0;

   DENTER(TOP_LAYER, "ec_get_clientdata");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      clientdata = lGetUlong(ec, EV_clientdata);
   }

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
*     bool 
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
*     bool - true, if the registration succeeded, else false
*
*  SEE ALSO
*     Eventclient/Client/ec_deregister()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_register(bool exit_on_qmaster_down, lList** alpp)
{
   bool ret = false;
   cl_com_handle_t* com_handle = NULL;

   DENTER(TOP_LAYER, "ec_register");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      WARNING((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      lList *lp, *alp;
      lListElem *aep;
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


#ifdef ENABLE_NGC
      com_handle = cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name(), 0);
      if (com_handle != NULL) {
         cl_commlib_remove_messages(cl_com_get_handle((char*)uti_state_get_sge_formal_prog_name(),0));
      }
#else
      /* remove possibly pending messages */
      remove_pending_messages(NULL, 0, 0, TAG_REPORT_REQUEST);
      /* commlib call to mark all commprocs as unknown */
      reset_last_heard();
#endif

      /*
       *  to add may also means to modify
       *  - if this event client is already enrolled at qmaster
       */

      alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_ADD | SGE_GDI_RETURN_NEW_VERSION, 
                    &lp, NULL, NULL);
    
      aep = lFirst(alp);
    
      ret = (lGetUlong(aep, AN_status)==STATUS_OK);   

      if (ret) { 
         lListElem *new_ec;
         u_long32 new_id = 0;

         new_ec = lFirst(lp);
         if(new_ec != NULL) {
            new_id = lGetUlong(new_ec, EV_id);
         }

         if (new_id != 0) {
            lSetUlong(ec, EV_id, new_id);
            DPRINTF(("REGISTERED with id "U32CFormat"\n", new_id));
            lSetBool(ec, EV_changed, false);
            config_changed = false;
            need_register = false;
            
         }
      } else {
         if (lGetUlong(aep, AN_quality) == ANSWER_QUALITY_ERROR) {
            ERROR((SGE_EVENT, "%s", lGetString(aep, AN_text)));
            answer_list_add(alpp, lGetString(aep, AN_text), 
                  lGetUlong(aep, AN_status), lGetUlong(aep, AN_quality));
            lFreeList(lp); 
            lFreeList(alp);
            if (exit_on_qmaster_down) {
               DPRINTF(("exiting in ec_register()\n"));
               SGE_EXIT(1);
            } else {
               DEXIT;
               return false;
            }
         }   
      }

      lFreeList(lp); 
      lFreeList(alp);
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
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
*     int - true, if the deregistration succeeded, else false
*
*  SEE ALSO
*     Eventclient/Client/ec_register()
*******************************************************************************/
bool 
ec_deregister(void)
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_deregister");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   /* not yet initialized? Nothing to shutdown */
   if (ec != NULL) {
      sge_pack_buffer pb;

      if (init_packbuffer(&pb, sizeof(u_long32), 0) == PACK_SUCCESS) {
         /* error message is output from init_packbuffer */
         int send_ret; 

         packint(&pb, lGetUlong(ec, EV_id));

         send_ret = sge_send_any_request(0, NULL, sge_get_master(0), prognames[QMASTER], 1, &pb, TAG_EVENT_CLIENT_EXIT,0);
         
         clear_packbuffer(&pb);
#ifdef ENABLE_NGC
         if (send_ret == CL_RETVAL_OK) 
#else
         if (send_ret == CL_OK) 
#endif
         {
            /* error message is output from sge_send_any_request */
            /* clear state of this event client instance */
            ec = lFreeElem(ec);
            need_register = true;
            ec_reg_id = 0;
            lSetBool(ec, EV_changed, false);
            config_changed = false;
            next_event = 1;

            ret = true;
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_subscribe() *************************************
*  NAME
*     ec_subscribe() -- Subscribe an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true on success, else false
*
*  SEE ALSO
*     Eventclient/-Events
*     Eventclient/Client/ec_subscribe_all()
*     Eventclient/Client/ec_unsubscribe()
*     Eventclient/Client/ec_unsubscribe_all()
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_subscribe(ev_event event)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "ec_subscribe");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
         WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event));
   }

   if (event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         ec_add_subscriptionElement(ec, i, EV_NOT_FLUSHED, -1);
      }
   } else {
      ec_add_subscriptionElement(ec, event, EV_NOT_FLUSHED, -1);
   }
   
   if (lGetBool(ec, EV_changed)) {
      ec_config_changed();
      ret = true;
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

static void ec_add_subscriptionElement(lListElem *event_el, ev_event event, bool flush, int interval) {

   lList *subscribed = NULL; 
   lListElem *sub_el = NULL;

   DENTER(TOP_LAYER, "ec_add_subscriptionElement");

   subscribed = lGetList(ec, EV_subscribed);
   
   if (event != sgeE_ALL_EVENTS){
      if (!subscribed) {
         subscribed = lCreateList("subscrition list", EVS_Type);
         lSetList(ec,EV_subscribed, subscribed);
      }
      else {
         sub_el = lGetElemUlong(subscribed, EVS_id, event);
      }
      
      if (!sub_el) {
         sub_el =  lCreateElem(EVS_Type);
         lAppendElem(subscribed, sub_el);
         
         lSetUlong(sub_el, EVS_id, event);
         lSetBool(sub_el, EVS_flush, flush);
         lSetUlong(sub_el, EVS_interval, interval);

         lSetBool(ec, EV_changed, true);
      }
   }
   DEXIT;
}

static void ec_mod_subscription_flush(lListElem *event_el, ev_event event, bool flush, 
                                       int intervall) {
   lList *subscribed = NULL;
   lListElem *sub_el = NULL;

   DENTER(TOP_LAYER, "ec_mod_subscription_flush");
  
   subscribed = lGetList(ec, EV_subscribed);
  
   if (event != sgeE_ALL_EVENTS){
      if (subscribed) {

         sub_el = lGetElemUlong(subscribed, EVS_id, event);
      
         if (sub_el) {
            lSetBool(sub_el, EVS_flush, flush);
            lSetUlong(sub_el, EVS_interval, intervall);
            lSetBool(ec, EV_changed, true);
         }
      }
   }

   DEXIT;
}

/****** sge_event_client/ec_mod_subscription_where() ***************************
*  NAME
*     ec_mod_subscription_where() -- adds an element filter to the event 
*
*  SYNOPSIS
*     bool ec_mod_subscription_where(ev_event event, const lListElem *what, 
*     const lListElem *where) 
*
*  FUNCTION
*     Allows to filter the event date on the master side to reduce the
*     date, which is send to the clients.
*
*  INPUTS
*     ev_event event         - event type 
*     const lListElem *what  - what condition 
*     const lListElem *where - where condition 
*
*  RESULT
*     bool - true, if everything went fine 
*
*  SEE ALSO
*     cull/cull_what/lWhatToElem()
*     cull/cull_what/lWhatFromElem()
*     cull/cull_where/lWhereToElem()
*     cull/cull_where/lWhereFromElem()
*******************************************************************************/
bool ec_mod_subscription_where(ev_event event, const lListElem *what, const lListElem *where) {
   lList *subscribed = NULL;
   lListElem *sub_el = NULL;
   bool ret = false;
   DENTER(TOP_LAYER, "ec_mod_subscription_where");
 
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event <= sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
         WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event));
   }
 
   subscribed = lGetList(ec, EV_subscribed);
  
   if (event != sgeE_ALL_EVENTS){
      if (subscribed) {

         sub_el = lGetElemUlong(subscribed, EVS_id, event);
      
         if (sub_el) {
            lSetObject(sub_el, EVS_what, lCopyElem(what));
            lSetObject(sub_el, EVS_where, lCopyElem(where));
            lSetBool(ec, EV_changed, true);
            ret = true;
         }
      }
   }

   DEXIT;
   return ret;
}
static void ec_remove_subscriptionElement(lListElem *event_el, ev_event event) {

   lList *subscribed =NULL; 
   lListElem *sub_el = NULL;
   
   DENTER(TOP_LAYER, "ec_remove_subscriptionElement");

   subscribed = lGetList(ec, EV_subscribed);
   
   if (event != sgeE_ALL_EVENTS){
      if (subscribed) {

         sub_el = lGetElemUlong(subscribed, EVS_id, event);
      
         if (sub_el) {
            if (lRemoveElem(subscribed, sub_el) == 0){
               lSetBool(ec, EV_changed, true);
            }
         }
      }
   }
   DEXIT;
}
/****** Eventclient/Client/ec_subscribe_all() *********************************
*  NAME
*     ec_subscribe_all() -- subscribe all events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
*     ec_subscribe_all(void) 
*
*  FUNCTION
*     Subscribe all possible event.
*     The subscription will be in effect after calling ec_commit or ec_get.
*
*  RESULT
*     bool - true on success, else false
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
bool 
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
*     bool 
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
*     bool - true on success, else false
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
bool 
ec_unsubscribe(ev_event event)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "ec_unsubscribe");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
   }

   if (event == sgeE_ALL_EVENTS) {
      int i;
      for(i = 0; i < sgeE_EVENTSIZE; i++) {
         ec_remove_subscriptionElement(ec, i);
      }
      ec_add_subscriptionElement(ec, sgeE_QMASTER_GOES_DOWN, EV_FLUSHED, 0);
      ec_add_subscriptionElement(ec, sgeE_SHUTDOWN, EV_FLUSHED, 0);

   } else {
      if (event == sgeE_QMASTER_GOES_DOWN || event == sgeE_SHUTDOWN) {
         ERROR((SGE_EVENT, MSG_EVENT_HAVETOHANDLEEVENTS));
      } else {
         ec_remove_subscriptionElement(ec, event);
      }
   }

   if (lGetBool(ec, EV_changed)) {
      ec_config_changed();
      ret = true;
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_unsubscribe_all() *******************************
*  NAME
*     ec_unsubscribe_all() -- unsubscribe all events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
*     ec_unsubscribe_all(void) 
*
*  FUNCTION
*     Unsubscribe all possible event.
*     The change will be in effect after calling ec_commit or ec_get.
*
*  RESULT
*     bool - true on success, else false
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
bool 
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
*     int - EV_NO_FLUSH or the number of seconds used for flushing
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
   int ret = EV_NO_FLUSH;
   
   DENTER(TOP_LAYER, "ec_get_flush");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
   } else {
      lListElem *sub_event = lGetElemUlong(lGetList(ec, EV_subscribed), EVS_id, event);

      if (sub_event == NULL) {
         ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      } else if (lGetBool(sub_event, EVS_flush)) {
         ret = lGetUlong(sub_event, EVS_interval);
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_set_flush() *************************************
*  NAME
*     ec_set_flush() -- set flushing information for an event
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool flush     - true for flushing
*     int interval   - flush interval in sec.
*                      
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Flushing
*     Eventclient/Client/ec_get_flush()
*     Eventclient/Client/ec_unset_flush()
*     Eventclient/Client/ec_subscribe_flush()
*******************************************************************************/
bool 
ec_set_flush(ev_event event, bool flush, int interval)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "ec_set_flush");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
   } else {
      if (!flush ) {
         PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);
         ret = ec_unset_flush(event);
         ec_mod_subscription_flush(ec, event, EV_NOT_FLUSHED, EV_NO_FLUSH);
         PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);
      } else if (interval < 0 || interval > EV_MAX_FLUSH) {
         WARNING((SGE_EVENT, MSG_EVENT_ILLEGALFLUSHTIME_I, interval));
      } else {
         lListElem *sub_event = lGetElemUlong(lGetList(ec, EV_subscribed), EVS_id, event);

         if (sub_event == NULL) {
            ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
         } 
         else { 
            ec_mod_subscription_flush(ec, event, EV_FLUSHED, interval);
         }
         if (lGetBool(ec, EV_changed)) {
            ec_config_changed();
            ret = true;
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_unset_flush() ***********************************
*  NAME
*     ec_unset_flush() -- unset flushing information
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
*     ec_unset_flush(ev_event event) 
*
*  FUNCTION
*     Switch of flushing of an individual event.
*
*  INPUTS
*     ev_event event - if of the event to configure
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Flushing
*     Eventclient/Client/ec_set_flush()
*     Eventclient/Client/ec_get_flush()
*     Eventclient/Client/ec_subscribe_flush()
******************************************************************************/
bool 
ec_unset_flush(ev_event event)
{
   bool ret = false;
   
   DENTER(TOP_LAYER, "ec_unset_flush");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else if (event < sgeE_ALL_EVENTS || event >= sgeE_EVENTSIZE) {
      WARNING((SGE_EVENT, MSG_EVENT_ILLEGALEVENTID_I, event ));
   } else {
      lListElem *sub_event = lGetElemUlong(lGetList(ec, EV_subscribed), EVS_id, event);

      if (sub_event == NULL) {
         ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      } 
      else { 
         ec_mod_subscription_flush(ec, event, EV_NOT_FLUSHED, EV_NO_FLUSH);
      } 

      if (lGetBool(ec, EV_changed)) {
         ec_config_changed();
         ret = true;
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_subscribe_flush() *******************************
*  NAME
*     ec_subscribe_flush() -- subscribe an event and set flushing
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true on success, else false
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Subscription
*     Eventclient/-Flushing
*     Eventclient/Client/ec_subscribe()
*     Eventclient/Client/ec_set_flush()
******************************************************************************/
bool 
ec_subscribe_flush(ev_event event, int flush) 
{
   bool ret;
   
   ret = ec_subscribe(event);
   if (ret) {
      if (flush >= 0)
         ret = ec_set_flush(event, true, flush);
      else
         ret = ec_set_flush(event, false, flush);
   }

   return ret;
}

/****** Eventclient/Client/ec_set_busy() **************************************
*  NAME
*     ec_set_busy() -- set the busy state
*
*  SYNOPSIS
*     bool 
*     ec_set_busy(bool busy) 
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
*     bool busy - true = event client busy, true = event client idle
*
*  RESULT
*     bool - true = success, false = failed
*
*  NOTES
*
*  SEE ALSO
*     Eventclient/-Busy-state
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_get_busy()
*******************************************************************************/
bool 
ec_set_busy(int busy)
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_set_busy");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      lSetUlong(ec, EV_busy, busy);

      /* force communication to qmaster - we may be out of sync */
      ec_config_changed();
      ret = true;
   }

   DEXIT;
   return ret;
}

/****** Eventclient/Client/ec_get_busy() **************************************
*  NAME
*     ec_get_busy() -- get the busy state
*
*  SYNOPSIS
*     bool 
*     ec_get_busy(void) 
*
*  FUNCTION
*     Reads the busy state of the event client.
*
*  RESULT
*     bool - true: the client is busy, false: the client is idle 
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
bool 
ec_get_busy(void)
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_get_busy");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      /* JG: TODO: EV_busy should be boolean datatype */
      ret = lGetUlong(ec, EV_busy) > 0;
   }

   DEXIT;
   return ret;
}

/****** sge_event_client/ec_set_session() **************************************
*  NAME
*     ec_set_session() -- Specify session key for event filtering.
*
*  SYNOPSIS
*     bool ec_set_session(const char *session) 
*
*  FUNCTION
*     Specifies a session that is used in event master for event
*     filtering.
*
*  INPUTS
*     const char *session - the session key
*
*  RESULT
*     bool - true = success, false = failed
*
*  SEE ALSO
*     Eventclient/-Session filtering
*     Eventclient/Client/ec_get_session()
*******************************************************************************/
bool ec_set_session(const char *session)
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_set_session");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      lSetString(ec, EV_session, session);

      /* force communication to qmaster - we may be out of sync */
      ec_config_changed();
      ret = true;
   }

   DEXIT;
   return ret;
}

/****** sge_event_client/ec_get_session() **************************************
*  NAME
*     ec_get_session() -- Get session key used for event filtering.
*
*  SYNOPSIS
*     const char* ec_get_session(void) 
*
*  FUNCTION
*     Returns session key that is used in event master for event
*     filtering.
*
*  RESULT
*     const char* - the session key
*
*  SEE ALSO
*     Eventclient/-Session filtering
*     Eventclient/Client/ec_set_session()
*******************************************************************************/
const char *ec_get_session(void)
{
   const char *ret = NULL;

   DENTER(TOP_LAYER, "ec_get_session");

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
   } else {
      ret = lGetString(ec, EV_session);
   }

   DEXIT;
   return ret;
}

/****** sge_event_client/ec_get_id() *******************************************
*  NAME
*     ec_get_id() -- Return event client id.
*
*  SYNOPSIS
*     ev_registration_id ec_get_id(void) 
*
*  FUNCTION
*     Return event client id.
*
*  RESULT
*     ev_registration_id - the event client id
*******************************************************************************/
ev_registration_id ec_get_id(void)
{
   DENTER(TOP_LAYER, "ec_get_id");
   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      DEXIT;
      return -1;
   }
   
   DEXIT;
   return lGetUlong(ec, EV_id);
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
   config_changed = true;
}

/****** Eventclient/Client/ec_commit() ****************************************
*  NAME
*     ec_commit() -- commit configuration changes
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true on success, else false
*
*  SEE ALSO
*     Eventclient/Client/ec_commit_multi()
*     Eventclient/Client/ec_config_changed()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_commit(void) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_commit");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   /* not yet initialized? Cannot send modification to qmaster! */
   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      DPRINTF((MSG_EVENT_UNINITIALIZED_EC));
   } else if (ec_need_new_registration()) {
      /* not (yet) registered? Cannot send modification to qmaster! */
      DPRINTF((MSG_EVENT_NOTREGISTERED));
   } else if (!config_changed) {
      DPRINTF(("no changes to commit\n"));
   } else {
      lList *lp, *alp;

      lp = lCreateList("change configuration", EV_Type);
      lAppendElem(lp, lCopyElem(ec));

      /*
       *  to add may also means to modify
       *  - if this event client is already enrolled at qmaster
       */
      alp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_MOD, &lp, NULL, NULL);
      lFreeList(lp); 
      
      ret = (lGetUlong(lFirst(alp), AN_status)==STATUS_OK);   
      lFreeList(alp);
      
      if (ret) {
         config_changed = false;
         lSetBool(ec, EV_changed, false);
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);
   DEXIT;
   return ret;
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
*     int - true on success, else false
*
*  SEE ALSO
*     Eventclient/Client/ec_commit()
*     Eventclient/Client/ec_config_changed()
*     Eventclient/Client/ec_get()
*******************************************************************************/
bool 
ec_commit_multi(lList **malpp, state_gdi_multi *state) 
{
   bool ret = false;

   DENTER(TOP_LAYER, "ec_commit_multi");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   /* not yet initialized? Cannot send modification to qmaster! */
   if (ec_reg_id >= EV_ID_FIRST_DYNAMIC || ec == NULL) {
      DPRINTF((MSG_EVENT_UNINITIALIZED_EC));
   } else if (ec_need_new_registration()) {
      /* not (yet) registered? Cannot send modification to qmaster! */
      DPRINTF((MSG_EVENT_NOTREGISTERED));
   } else {
      int commit_id, gdi_ret;
      lList *lp, *alp = NULL;

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
      } else {
         alp = sge_gdi_extract_answer(SGE_GDI_ADD, SGE_ORDER_LIST, commit_id, 
                                      *malpp, NULL);

         gdi_ret = answer_list_handle_request_answer_list(&alp, stderr);

         if (gdi_ret == STATUS_OK) {
            config_changed = false;
            ret = true;
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return false;
}

/****** Eventclient/Client/ec_get() ******************************************
*  NAME
*     ec_get() -- look for new events
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     bool 
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
*     bool - true, if events or an empty event list was received, or
*                  if no data was received within a certain time period
*            false, if an error occured
*
*  NOTES
*     The event_list has to be freed by the calling function.
*
*  SEE ALSO
*     Eventclient/Client/ec_register()
*     Eventclient/Client/ec_commit()
*******************************************************************************/
bool 
ec_get(lList **event_list, bool exit_on_qmaster_down) 
{
   bool ret = true;
   lList *report_list = NULL;
   u_long32 wrong_number;

   DENTER(TOP_LAYER, "ec_get");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   if (ec == NULL) {
      ERROR((SGE_EVENT, MSG_EVENT_UNINITIALIZED_EC));
      ret = false;
   } else if (ec_need_new_registration()) {
      next_event = 1;
      ret = ec_register(exit_on_qmaster_down, NULL);
   }
  
   if (ret) {
      if (config_changed) {
         ret = ec_commit();
      }
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
   if (ret) {
      static time_t last_fetch_time = 0;
      time_t now;

      bool done = false;
      bool fetch_ok = false;
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

         if ((fetch_ok = get_event_list(sync, &report_list))) {
            lList *new_events = NULL;
            lXchgList(lFirst(report_list), REP_list, &new_events);
            report_list = lFreeList(report_list);
            if (!ck_event_number(new_events, &next_event, &wrong_number)) {
               /*
                *  may be we got an old event, that was sent before
                *  reregistration at qmaster
                */
               lFreeList(*event_list);
               *event_list = NULL;
               lFreeList(new_events);
               ec_mark4registration();
               ret = false;
               done = true;
               continue;
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
      if (sync && !fetch_ok) {
         DPRINTF(("first syncronous get_event_list failed\n"));
         ret = false;
      } else {
         /* send an ack to the qmaster for all received events */
#ifdef ENABLE_NGC
         if (sge_send_ack_to_qmaster(0, ACK_EVENT_DELIVERY, next_event - 1, lGetUlong(ec, EV_id)) != CL_RETVAL_OK) {
#else
         if (sge_send_ack_to_qmaster(0, ACK_EVENT_DELIVERY, next_event - 1, lGetUlong(ec, EV_id))) {
#endif
            WARNING((SGE_EVENT, MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY ));
         } else {
            DPRINTF(("Sent ack for all events lower or equal %d\n", 
                     (next_event - 1)));
         }
      }
   } 

   if (*event_list != NULL) {
      DPRINTF(("ec_get - received %d events\n", lGetNumberOfElem(*event_list)));
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}


/****** Eventclient/Client/ck_event_number() **********************************
*  NAME
*     ck_event_number() -- test event numbers
*
*  SYNOPSIS
*     #include "sge_event_client.h"
*
*     static bool 
*     ck_event_number(lList *lp, u_long32 *waiting_for, 
*                     u_long32 *wrong_number) 
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
*     static bool - true on success, else false
*
*******************************************************************************/
static bool 
ck_event_number(lList *lp, u_long32 *waiting_for, u_long32 *wrong_number)
{
   bool ret = true;
   lListElem *ep, *tmp;
   u_long32 i, j;
   int skipped;
  
   DENTER(TOP_LAYER, "ck_event_number");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   i = *waiting_for;

   if (!lp || !lGetNumberOfElem(lp)) {
      /* got a dummy event list for alive protocol */
      DPRINTF(("received empty event list\n"));
   } else {
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
      }

      /* ensure number of first event is lower or equal "waiting_for" */
      if ((j=lGetUlong(lFirst(lp), ET_number)) > i) {
         /* error in event numbers */
         if (wrong_number) {
            *wrong_number = j;
         }   
         ERROR((SGE_EVENT, MSG_EVENT_SMALLESTEVENTXISGRTHYWAITFOR_UU,
                  u32c(j), u32c(i)));
         ret = false;
      } else {

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

         if (skipped) {
            DPRINTF(("Skipped %d events, still %d in list\n", skipped, 
                     lGetNumberOfElem(lp)));
         }

         /* ensure number of events increase */
         for_each (ep, lp) {
            if ((j=lGetUlong(ep, ET_number)) != i++) {
               /* 
                  do not change waiting_for because 
                  we still wait for this number 
               */
               ERROR((SGE_EVENT, MSG_EVENT_EVENTSWITHNOINCREASINGNUMBERS ));
               if (wrong_number) {
                  *wrong_number = j;
               }   
               ret = false;
               break;
            }
         }
         
         if (ret) {
            /* that's the new number we wait for */
            *waiting_for = i;
            DPRINTF(("check complete, %d events in list\n", 
                     lGetNumberOfElem(lp)));
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/***i** Eventclient/Client/get_event_list() ***********************************
*  NAME
*     get_event_list() -- get event list via gdi call
*
*  SYNOPSIS
*     static bool 
*     get_event_list(int sync, lList **report_list) 
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
*     static bool - true on success, else false
*
*  SEE ALSO
*     Eventclient/Client/ec_get()
******************************************************************************/
static bool 
get_event_list(int sync, lList **report_list)
{
   int tag;
   bool ret = true;
   u_short id;
   sge_pack_buffer pb;
   int help;

   DENTER(TOP_LAYER, "get_event_list");

   PROF_START_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   id = 0;
   tag = TAG_REPORT_REQUEST;
   /* FIX_CONST */
   
#ifdef ENABLE_NGC
   id = 1;
   DPRINTF(("try to get request form %s, id %d\n",(char*)prognames[QMASTER], id ));
   if ( (help=sge_get_any_request((char*)sge_get_master(0), (char*)prognames[QMASTER], &id, &pb, &tag, sync,0,0)) != CL_RETVAL_OK) {
      WARNING(("commlib returns %s (%d)\n", cl_get_error_text(help), help ));
      ret = false;
#else
   if (sge_get_any_request((char*)sge_get_master(0), 
                           (char*)prognames[QMASTER], &id, &pb, &tag, sync) 
                           != CL_OK) {
      DPRINTF(("commlib returns %s (%d)\n", cl_errstr(ret), ret));
      ret = false;
#endif
   } else {
      if (cull_unpack_list(&pb, report_list)) {
         ERROR((SGE_EVENT, MSG_LIST_FAILEDINCULLUNPACKREPORT ));
         ret = false;
      }
      clear_packbuffer(&pb);
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_EVENTCLIENT);

   DEXIT;
   return ret;
}

/****** Eventclient/event_text() **********************************************
*  NAME
*     event_text() -- deliver event description
*
*  SYNOPSIS
*     const char* event_text(const lListElem *event) 
*
*  FUNCTION
*     Deliveres a short description of an event object.
*
*  INPUTS
*     const lListElem *event - the event to describe
*
*  RESULT
*     const char* - pointer to the descriptive string.
*
*  NOTES
*     The result points to a static buffer. Subsequent calls to i
*     event_text will overwrite previous results.
*******************************************************************************/
/* function see libs/sgeobj/sge_event.c */
