#ifndef __SGE_EVENTL_H
#define __SGE_EVENTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

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
typedef enum {
   EV_ID_ANY = 0,            /* qmaster will give the ev a unique id */
   EV_ID_SCHEDD = 1,         /* schedd registers at qmaster */
   EV_ID_FIRST_DYNAMIC = 11  /* first id given by qmaster for EV_ID_ANY registration */ 
}ev_registration_id;


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
#define EV_NO_FLUSH -1

#define EV_NOT_SUBSCRIBED 0x01
#define EV_SUBSCRIBED 0x02
#define EV_FLUSHED 0x03
#define EV_MAX_FLUSH 0x3f

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
*                                 
*  NOTES
*
*  SEE ALSO
*     Eventclient/Client/ec_set_busy_handling()
*     Eventclient/Client/ec_set_busy()
****************************************************************************
*/

typedef enum {
   EV_BUSY_NO_HANDLING = 0,
   EV_BUSY_UNTIL_ACK,
   EV_BUSY_UNTIL_RELEASED
} ev_busy_handling;

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
*     SGE_STRING(EV_subscription)
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

enum {
   /* identification */
   EV_id = EV_LOWERBOUND,    /* unique id requested by client or given by qmaster */
   EV_name,                  /* name of event client (non unique) */

   /* addressing informations */
   EV_host,                  /* host to deliver the events */
   EV_commproc,              /* used to deliver events */
   EV_commid,                /* unique id delivered by the sge_commd */

   /* authentication informations */
   EV_uid,

   /* event request description */
   EV_d_time,                /* delivery interval for events */
   EV_subscription,          /* subscription information */
   EV_busy_handling,         /* how to handle busy-states */

   /* dynamic data */
   EV_last_heard_from,         /* used to trash unheard event clients */
   EV_last_send_time,        /* time when last event list has been sent */
   EV_next_send_time,        /* time when next list has to be sent */
   EV_next_number,           /* the number the next event will get */
   EV_busy,                  /* is the client busy? */
   EV_events,                /* used to hold the events that */
                             /* are not acknowledged */
   /* for free use */
   EV_clientdata             /* can be used by client or master for any purposes */
};

LISTDEF(EV_Type)
   SGE_ULONG(EV_id, CULL_DEFAULT)
   SGE_STRING(EV_name, CULL_DEFAULT)
   
   SGE_HOST(EV_host, CULL_DEFAULT)
   SGE_STRING(EV_commproc, CULL_DEFAULT)
   SGE_ULONG(EV_commid, CULL_DEFAULT)
   
   SGE_ULONG(EV_uid, CULL_DEFAULT)
   
   SGE_ULONG(EV_d_time, CULL_DEFAULT)
   SGE_STRING(EV_subscription, CULL_DEFAULT)
   SGE_ULONG(EV_busy_handling, CULL_DEFAULT)
   
   SGE_ULONG(EV_last_heard_from, CULL_DEFAULT)
   SGE_ULONG(EV_last_send_time, CULL_DEFAULT)
   SGE_ULONG(EV_next_send_time, CULL_DEFAULT)
   SGE_ULONG(EV_next_number, CULL_DEFAULT)
   SGE_ULONG(EV_busy, CULL_DEFAULT)
   SGE_LIST(EV_events, ET_Type, CULL_DEFAULT)

   SGE_ULONG(EV_clientdata, CULL_DEFAULT)
LISTEND 

NAMEDEF(EVN)
   NAME("EV_id")
   NAME("EV_name")

   NAME("EV_host")
   NAME("EV_commproc")
   NAME("EV_commid")

   NAME("EV_uid")

   NAME("EV_d_time")
   NAME("EV_subscription")
   NAME("EV_busy_handling")

   NAME("EV_last_heard_from")
   NAME("EV_last_send_time")
   NAME("EV_next_send_time")
   NAME("EV_next_number")
   NAME("EV_busy")
   NAME("EV_events")

   NAME("EV_clientdata")
NAMEEND

#define EVS sizeof(EVN)/sizeof(char*)

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
*        sgeE_COMPLEX_LIST                send complex list at registration
*        sgeE_COMPLEX_ADD                 event add complex
*        sgeE_COMPLEX_DEL                 event delete complex
*        sgeE_COMPLEX_MOD                 event modify complex
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
*        sgeE_USERMAPPING_ENTRY_LIST      send list of user mappings
*        sgeE_USERMAPPING_ENTRY_ADD       a new user mapping was added
*        sgeE_USERMAPPING_ENTRY_DEL       a user mapping was deleted
*        sgeE_USERMAPPING_ENTRY_MOD       a user mapping entry was changed
*     
*        sgeE_HOST_GROUP_LIST             send list of host groups
*        sgeE_HOST_GROUP_ADD              a host group was added
*        sgeE_HOST_GROUP_DEL              a host group was deleted
*        sgeE_HOST_GROUP_MOD              a host group was changed
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

typedef enum {
   sgeE_ALL_EVENTS,                 /* + = impl. and tested, - = not available */

   sgeE_ADMINHOST_LIST,             /* + send admin host list at registration */
   sgeE_ADMINHOST_ADD,              /* + event add admin host */
   sgeE_ADMINHOST_DEL,              /* + event delete admin host */
   sgeE_ADMINHOST_MOD,              /* - event modify admin host */

   sgeE_CALENDAR_LIST,              /* + send calendar list at registration */
   sgeE_CALENDAR_ADD,               /* + event add calendar */
   sgeE_CALENDAR_DEL,               /* + event delete calendar */
   sgeE_CALENDAR_MOD,               /* + event modify calendar */

   sgeE_CKPT_LIST,                  /* + send ckpt list at registration */
   sgeE_CKPT_ADD,                   /* + event add ckpt */
   sgeE_CKPT_DEL,                   /* + event delete ckpt */
   sgeE_CKPT_MOD,                   /* + event modify ckpt */

   sgeE_COMPLEX_LIST,               /* + send complex list at registration */
   sgeE_COMPLEX_ADD,                /* + event add complex */
   sgeE_COMPLEX_DEL,                /* + event delete complex */
   sgeE_COMPLEX_MOD,                /* + event modify complex */

   sgeE_CONFIG_LIST,                /* + send config list at registration */
   sgeE_CONFIG_ADD,                 /* + event add config */
   sgeE_CONFIG_DEL,                 /* + event delete config */
   sgeE_CONFIG_MOD,                 /* + event modify config */

   sgeE_EXECHOST_LIST,              /* + send exec host list at registration */
   sgeE_EXECHOST_ADD,               /* + event add exec host */
   sgeE_EXECHOST_DEL,               /* + event delete exec host */
   sgeE_EXECHOST_MOD,               /* + event modify exec host */

   sgeE_GLOBAL_CONFIG,              /* + global config changed, replace by sgeE_CONFIG_MOD */

   sgeE_JATASK_ADD,                 /* + event add array job task */
   sgeE_JATASK_DEL,                 /* + event delete array job task */
   sgeE_JATASK_MOD,                 /* + event modify array job task */

   sgeE_PETASK_ADD,                 /*   event add a new pe task */
   sgeE_PETASK_DEL,                 /*   event delete a pe task */

   sgeE_JOB_LIST,                   /* + send job list at registration */
   sgeE_JOB_ADD,                    /* + event job add (new job) */
   sgeE_JOB_DEL,                    /* + event job delete */
   sgeE_JOB_MOD,                    /* + event job modify */
   sgeE_JOB_MOD_SCHED_PRIORITY,     /* + event job modify priority */
   sgeE_JOB_USAGE,                  /* + event job online usage */
   sgeE_JOB_FINAL_USAGE,            /* + event job final usage report after job end */
   sgeE_JOB_FINISH,                 /* + job finally finished or aborted (user view) */

   sgeE_JOB_SCHEDD_INFO_LIST,       /* + send job schedd info list at registration */
   sgeE_JOB_SCHEDD_INFO_ADD,        /* - event jobs schedd info added */
   sgeE_JOB_SCHEDD_INFO_DEL,        /* - event jobs schedd info deleted */
   sgeE_JOB_SCHEDD_INFO_MOD,        /* + event jobs schedd info modified */

   sgeE_MANAGER_LIST,               /* + send manager list at registration */
   sgeE_MANAGER_ADD,                /* + event add manager */
   sgeE_MANAGER_DEL,                /* + event delete manager */
   sgeE_MANAGER_MOD,                /* - event modify manager */

   sgeE_OPERATOR_LIST,              /* + send operator list at registration */
   sgeE_OPERATOR_ADD,               /* + event add operator */
   sgeE_OPERATOR_DEL,               /* + event delete operator */
   sgeE_OPERATOR_MOD,               /* - event modify operator */

   sgeE_NEW_SHARETREE,              /* + replace possibly existing share tree */

   sgeE_PE_LIST,                    /* + send pe list at registration */
   sgeE_PE_ADD,                     /* + event pe add */
   sgeE_PE_DEL,                     /* + event pe delete */
   sgeE_PE_MOD,                     /* + event pe modify */

   sgeE_PROJECT_LIST,               /* + send project list at registration */
   sgeE_PROJECT_ADD,                /* + event project add */
   sgeE_PROJECT_DEL,                /* + event project delete */
   sgeE_PROJECT_MOD,                /* + event project modify */

   sgeE_QMASTER_GOES_DOWN,          /* + qmaster notifies all event clients, before
                                         it exits */

   sgeE_QUEUE_LIST,                 /* + send queue list at registration */
   sgeE_QUEUE_ADD,                  /* + event queue add */
   sgeE_QUEUE_DEL,                  /* + event queue delete */
   sgeE_QUEUE_MOD,                  /* + event queue modify */
   sgeE_QUEUE_SUSPEND_ON_SUB,       /* + queue is suspended by subordinate mechanism */
   sgeE_QUEUE_UNSUSPEND_ON_SUB,     /* + queue is unsuspended by subordinate mechanism */

   sgeE_SCHED_CONF,                 /* + replace existing (sge) scheduler configuration */

   sgeE_SCHEDDMONITOR,              /* + trigger scheduling run */

   sgeE_SHUTDOWN,                   /* + request shutdown of an event client */

   sgeE_SUBMITHOST_LIST,            /* + send submit host list at registration */
   sgeE_SUBMITHOST_ADD,             /* + event add submit host */
   sgeE_SUBMITHOST_DEL,             /* + event delete submit host */
   sgeE_SUBMITHOST_MOD,             /* - event modify submit host */

   sgeE_USER_LIST,                  /* + send user list at registration */
   sgeE_USER_ADD,                   /* + event user add */
   sgeE_USER_DEL,                   /* + event user delete */
   sgeE_USER_MOD,                   /* + event user modify */

   sgeE_USERSET_LIST,               /* + send userset list at registration */
   sgeE_USERSET_ADD,                /* + event userset add */
   sgeE_USERSET_DEL,                /* + event userset delete */
   sgeE_USERSET_MOD,                /* + event userset modify */
   
#ifndef __SGE_NO_USERMAPPING__
   sgeE_USERMAPPING_ENTRY_LIST,
   sgeE_USERMAPPING_ENTRY_ADD,
   sgeE_USERMAPPING_ENTRY_DEL,
   sgeE_USERMAPPING_ENTRY_MOD,
#endif

   sgeE_HOST_GROUP_LIST,
   sgeE_HOST_GROUP_ADD,
   sgeE_HOST_GROUP_DEL,
   sgeE_HOST_GROUP_MOD,

   sgeE_EVENTSIZE 
}ev_event;

enum {
   ET_number = ET_LOWERBOUND,/* number of the event */
   ET_timestamp,             /* unix time stamp (gmt) when the event occured */
   ET_type,                  /* type the event */
   ET_intkey,                /* a int key for use by a specific event type */
   ET_intkey2,               /* a int key for use by a specific event type */
   ET_strkey,                /* a str key for use by a specific event type */
   ET_new_version            /* new version of the changed object */
                             /* JG: TODO: we should have different fields for 
                              *           objects (SGE_OBJECT) and
                              *           lists   (SGE_LIST), as we now have
                              *           proper handling for objects.
                              */
};

LISTDEF(ET_Type)
   SGE_ULONG(ET_number, CULL_DEFAULT)
   SGE_ULONG(ET_timestamp, CULL_DEFAULT)
   SGE_ULONG(ET_type, CULL_DEFAULT)
   SGE_ULONG(ET_intkey, CULL_DEFAULT)
   SGE_ULONG(ET_intkey2, CULL_DEFAULT)
   SGE_STRING(ET_strkey, CULL_DEFAULT)
   SGE_LIST(ET_new_version, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND 

NAMEDEF(ETN)
   NAME("ET_number")
   NAME("ET_timestamp")
   NAME("ET_type")
   NAME("ET_intkey")
   NAME("ET_intkey2")
   NAME("ET_strkey")
   NAME("ET_new_version")
NAMEEND

/* *INDENT-ON* */ 

#define ETS sizeof(ETN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_EVENTL_H */
