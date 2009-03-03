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
#include "uti/sge_monitor.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* documentation see libs/evc/sge_event_client.c */
typedef enum {
   EV_ID_INVALID = -1,
   EV_ID_ANY = 0,            /* qmaster will give the ev a unique id */
   EV_ID_SCHEDD = 1,         /* schedd registers at qmaster */
   EV_ID_FIRST_DYNAMIC = 11  /* first id given by qmaster for EV_ID_ANY registration */ 
}ev_registration_id;

/*-------------------------------------------*/
/* data structurs for the local event client */
/*-------------------------------------------*/

/** 
 *   this is the definition for the update function that is used by process
 *   internal event clients / mirrors. 
 **/
typedef void (*event_client_update_func_t)(
   u_long32 id,                /* event client id */
   lList **alpp,               /* answer list */
   lList *event_list           /* list of new events stored in the report list */ 
);

/**
 * The event client modification function. Part of the sge_event_master
 *
 * SEE ALSO
 *   EventMaster/sge_mod_event_client
 */
typedef int (*evm_mod_func_t)(
   lListElem *clio,  /* the new event client structure with a set update_func */ 
   lList** alpp,     /* a answer list */ 
   char* ruser,      /* calling user */ 
   char* rhost       /* calling host */
);


/**
 * The event client add function in the sge_event_master
 *
 * SEE ALSO
 *  EventMaster/sge_add_event_client_local
 **/
typedef int (*evm_add_func_t)(
   lListElem *clio,                        /* the new event client */
   lList **alpp,                           /* the answer list */
   lList **eclpp,                          /* list with added event client elem */
   char *ruser,                            /* request user or <admin_user> for internal ec */
   char *rhost,                            /* request host or <master_host> for internal ec */
   event_client_update_func_t update_func, /* the event client update_func */     
   monitoring_t *monitor                   /* the monitoring structure */
);

/**
 * The event client remove function in the sge_event_master
 *
 * SEE ALSO
 *  EventMaster/sge_removce_event_client
 **/
typedef void (*evm_remove_func_t) (
   u_long32 aClientID               /* the event client id to remove */
);

/* documentation see libs/evc/sge_event_client.c */
/* #define EV_NO_FLUSH -1 */

#define EV_NOT_SUBSCRIBED false 
#define EV_SUBSCRIBED true
#define EV_FLUSHED true
#define EV_NOT_FLUSHED false
#define EV_MAX_FLUSH 0x3f
#define EV_NO_FLUSH -1 

/* documentation see libs/evc/sge_event_client.c */

typedef enum {
   EV_BUSY_NO_HANDLING = 0,
   EV_BUSY_UNTIL_ACK,
   EV_BUSY_UNTIL_RELEASED,
   EV_THROTTLE_FLUSH
} ev_busy_handling;

typedef enum {
   EV_subscribing = 0,
   EV_connected,
   EV_closing,
   EV_terminated
} ev_state_handling;

/* documentation see libs/evc/sge_event_client.c */
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
   EV_flush_delay,           /* flush delay paramter */
   /* je kleiner EV_flush_delay eines Event client
      desto unmittelbarer das Drosseln des Event-flushens */
   EV_subscribed,            /* a list of subscribed events */
   
   EV_changed,               /* identifies changes in the subscription list */
   EV_busy_handling,         /* how to handle busy-states */
   EV_session,               /* session key to be used for filtering subscribed events */

   /* dynamic data */
   EV_last_heard_from,         /* used to trash unheard event clients */
   EV_last_send_time,        /* time when last event list has been sent */
   EV_next_send_time,        /* time when next list has to be sent */
   EV_next_number,           /* the number the next event will get */
   EV_busy,                  /* is the client busy? */
   EV_events,                /* used to hold the events that */
                             /* are not acknowledged */
   EV_sub_array,             /* contains an array of subscribed events, only used in qmaster */      
   
   EV_state,                 /* identifies the state the event client is in. Sofar we have: connected, closing, terminated */
   EV_update_function        /* stores an update function for process internal event clients (threads) */
};

LISTDEF(EV_Type)
   JGDI_ROOT_OBJ(EventClient, SGE_EVENT_LIST, GET_LIST)

/*    SGE_ULONG(EV_id, CULL_DEFAULT) */
   SGE_ULONG(EV_id, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_DEFAULT)
   SGE_STRING(EV_name, CULL_DEFAULT)
   
   SGE_HOST(EV_host, CULL_DEFAULT)
   SGE_STRING(EV_commproc, CULL_DEFAULT)
   SGE_ULONG(EV_commid, CULL_DEFAULT)
   
   SGE_ULONG(EV_uid, CULL_DEFAULT)
   
   SGE_ULONG(EV_d_time, CULL_DEFAULT)
   SGE_ULONG(EV_flush_delay, CULL_DEFAULT)
   SGE_LIST(EV_subscribed, EVS_Type, CULL_DEFAULT)
   SGE_BOOL(EV_changed, CULL_DEFAULT)
   SGE_ULONG(EV_busy_handling, CULL_DEFAULT)
   SGE_STRING(EV_session, CULL_DEFAULT)
   
   SGE_ULONG(EV_last_heard_from, CULL_DEFAULT)
   SGE_ULONG(EV_last_send_time, CULL_DEFAULT)
   SGE_ULONG(EV_next_send_time, CULL_DEFAULT)
   SGE_ULONG(EV_next_number, CULL_DEFAULT)
   SGE_ULONG(EV_busy, CULL_DEFAULT)
   SGE_LIST(EV_events, ET_Type, CULL_DEFAULT)
   SGE_REF(EV_sub_array, CULL_ANY_SUBTYPE, CULL_DEFAULT)

   SGE_ULONG(EV_state, CULL_DEFAULT)
   SGE_REF(EV_update_function, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND 

NAMEDEF(EVN)
   NAME("EV_id")
   NAME("EV_name")

   NAME("EV_host")
   NAME("EV_commproc")
   NAME("EV_commid")

   NAME("EV_uid")

   NAME("EV_d_time")
   NAME("EV_flush_delay")
   NAME("EV_subscribed")
   NAME("EV_changed")
   NAME("EV_busy_handling")
   NAME("EV_session")

   NAME("EV_last_heard_from")
   NAME("EV_last_send_time")
   NAME("EV_next_send_time")
   NAME("EV_next_number")
   NAME("EV_busy")
   NAME("EV_events")
   NAME("EV_sub_array")

   NAME("EV_state")
   NAME("EV_update_function")
NAMEEND

#define EVS sizeof(EVN)/sizeof(char*)





/* documentation see libs/evc/sge_event_client.c       */
/*                                                     */
/* If this enum is changed, one has to be aware of the */
/* the following arrays in libs/evm/sge_event_master.c */
/*    block_events                                     */
/*    total_update_events                              */
/*    EVENT_LIST                                       */
/*    FIELD_LIST                                       */
/*    SOURCE_LIST                                      */
/* They might have to be addapted as well.             */

typedef enum {
   sgeE_ALL_EVENTS,                 /*0 + = impl. and tested, - = not available */

   sgeE_ADMINHOST_LIST,             /*1 + send admin host list at registration */
   sgeE_ADMINHOST_ADD,              /*2 + event add admin host */
   sgeE_ADMINHOST_DEL,              /*3 + event delete admin host */
   sgeE_ADMINHOST_MOD,              /*4 - event modify admin host */

   sgeE_CALENDAR_LIST,              /*5 + send calendar list at registration */
   sgeE_CALENDAR_ADD,               /*6 + event add calendar */
   sgeE_CALENDAR_DEL,               /*7 + event delete calendar */
   sgeE_CALENDAR_MOD,               /*8 + event modify calendar */

   sgeE_CKPT_LIST,                  /*9  + send ckpt list at registration */
   sgeE_CKPT_ADD,                   /*10 + event add ckpt */
   sgeE_CKPT_DEL,                   /*11 + event delete ckpt */
   sgeE_CKPT_MOD,                   /*12 + event modify ckpt */

   sgeE_CENTRY_LIST,                /*13 + send complex list at registration */
   sgeE_CENTRY_ADD,                 /*14 + event add complex */
   sgeE_CENTRY_DEL,                 /*15 + event delete complex */
   sgeE_CENTRY_MOD,                 /*16 + event modify complex */

   sgeE_CONFIG_LIST,                /*17 + send config list at registration */
   sgeE_CONFIG_ADD,                 /*18 + event add config */
   sgeE_CONFIG_DEL,                 /*19 + event delete config */
   sgeE_CONFIG_MOD,                 /*20 + event modify config */

   sgeE_EXECHOST_LIST,              /*21 + send exec host list at registration */
   sgeE_EXECHOST_ADD,               /*22 + event add exec host */
   sgeE_EXECHOST_DEL,               /*23 + event delete exec host */
   sgeE_EXECHOST_MOD,               /*24 + event modify exec host */

   sgeE_GLOBAL_CONFIG,              /*25 + global config changed, replace by sgeE_CONFIG_MOD */

   sgeE_JATASK_ADD,                 /*26 + event add array job task */
   sgeE_JATASK_DEL,                 /*27 + event delete array job task */
   sgeE_JATASK_MOD,                 /*28 + event modify array job task */

   sgeE_PETASK_ADD,                 /*29   event add a new pe task */
   sgeE_PETASK_DEL,                 /*30   event delete a pe task */

   sgeE_JOB_LIST,                   /*31 + send job list at registration */
   sgeE_JOB_ADD,                    /*32 + event job add (new job) */
   sgeE_JOB_DEL,                    /*33 + event job delete */
   sgeE_JOB_MOD,                    /*34 + event job modify */
   sgeE_JOB_MOD_SCHED_PRIORITY,     /*35 + event job modify priority */
   sgeE_JOB_USAGE,                  /*36 + event job online usage */
   sgeE_JOB_FINAL_USAGE,            /*37 + event job final usage report after job end */
   sgeE_JOB_FINISH,                 /*38 + job finally finished or aborted (user view) */

   sgeE_JOB_SCHEDD_INFO_LIST,       /*39 + send job schedd info list at registration */
   sgeE_JOB_SCHEDD_INFO_ADD,        /*40 - event jobs schedd info added */
   sgeE_JOB_SCHEDD_INFO_DEL,        /*41 - event jobs schedd info deleted */
   sgeE_JOB_SCHEDD_INFO_MOD,        /*42 + event jobs schedd info modified */

   sgeE_MANAGER_LIST,               /*43 + send manager list at registration */
   sgeE_MANAGER_ADD,                /*44 + event add manager */
   sgeE_MANAGER_DEL,                /*45 + event delete manager */
   sgeE_MANAGER_MOD,                /*46 - event modify manager */

   sgeE_OPERATOR_LIST,              /*47 + send operator list at registration */
   sgeE_OPERATOR_ADD,               /*48 + event add operator */
   sgeE_OPERATOR_DEL,               /*49 + event delete operator */
   sgeE_OPERATOR_MOD,               /*50 - event modify operator */

   sgeE_NEW_SHARETREE,              /*51 + replace possibly existing share tree */

   sgeE_PE_LIST,                    /*52 + send pe list at registration */
   sgeE_PE_ADD,                     /*53 + event pe add */
   sgeE_PE_DEL,                     /*54 + event pe delete */
   sgeE_PE_MOD,                     /*55 + event pe modify */

   sgeE_PROJECT_LIST,               /*56 + send project list at registration */
   sgeE_PROJECT_ADD,                /*57 + event project add */
   sgeE_PROJECT_DEL,                /*58 + event project delete */
   sgeE_PROJECT_MOD,                /*59 + event project modify */

   sgeE_QMASTER_GOES_DOWN,          /*60 + qmaster notifies all event clients, before
                                         it exits */

   sgeE_CQUEUE_LIST,                /*61 + send cluster queue list at registration */
   sgeE_CQUEUE_ADD,                 /*62 + event cluster queue add */
   sgeE_CQUEUE_DEL,                 /*63 + event cluster queue delete */
   sgeE_CQUEUE_MOD,                 /*64 + event cluster queue modify */

   sgeE_QINSTANCE_ADD,              /*65 + event queue instance add */
   sgeE_QINSTANCE_DEL,              /*66 + event queue instance delete */
   sgeE_QINSTANCE_MOD,              /*67 + event queue instance mod */
   sgeE_QINSTANCE_SOS,              /*68 + event queue instance sos */
   sgeE_QINSTANCE_USOS,             /*69 + event queue instance usos */

   sgeE_SCHED_CONF,                 /*70 + replace existing (sge) scheduler configuration */

   sgeE_SCHEDDMONITOR,              /*71 + trigger scheduling run */

   sgeE_SHUTDOWN,                   /*72 + request shutdown of an event client */

   sgeE_SUBMITHOST_LIST,            /*73 + send submit host list at registration */
   sgeE_SUBMITHOST_ADD,             /*74 + event add submit host */
   sgeE_SUBMITHOST_DEL,             /*75 + event delete submit host */
   sgeE_SUBMITHOST_MOD,             /*76 - event modify submit host */

   sgeE_USER_LIST,                  /*77 + send user list at registration */
   sgeE_USER_ADD,                   /*78 + event user add */
   sgeE_USER_DEL,                   /*79 + event user delete */
   sgeE_USER_MOD,                   /*80 + event user modify */

   sgeE_USERSET_LIST,               /*81 + send userset list at registration */
   sgeE_USERSET_ADD,                /*82 + event userset add */
   sgeE_USERSET_DEL,                /*83 + event userset delete */
   sgeE_USERSET_MOD,                /*84 + event userset modify */
   
#ifndef __SGE_NO_USERMAPPING__
   sgeE_CUSER_LIST,
   sgeE_CUSER_ADD,
   sgeE_CUSER_DEL,
   sgeE_CUSER_MOD,
#endif

   sgeE_HGROUP_LIST,
   sgeE_HGROUP_ADD,
   sgeE_HGROUP_DEL,
   sgeE_HGROUP_MOD,

   sgeE_RQS_LIST,
   sgeE_RQS_ADD,
   sgeE_RQS_DEL,
   sgeE_RQS_MOD,

   sgeE_AR_LIST,
   sgeE_AR_ADD,
   sgeE_AR_DEL,
   sgeE_AR_MOD,

   sgeE_ACK_TIMEOUT,

   sgeE_EVENTSIZE 
} ev_event;

/**
 * The event client event ack function. Part of the sge_event_master
 *
 * SEE ALSO
 *   EventMaster/sge_handle_event_ack
 */
typedef bool (*evm_ack_func_t)(
   u_long32,         /* the event client id */
   u_long32          /* the last event to ack */
);

#define IS_TOTAL_UPDATE_EVENT(x) \
  (((x)==sgeE_ADMINHOST_LIST) || \
  ((x)==sgeE_CALENDAR_LIST) || \
  ((x)==sgeE_CKPT_LIST) || \
  ((x)==sgeE_CENTRY_LIST) || \
  ((x)==sgeE_CONFIG_LIST) || \
  ((x)==sgeE_EXECHOST_LIST) || \
  ((x)==sgeE_JOB_LIST) || \
  ((x)==sgeE_JOB_SCHEDD_INFO_LIST) || \
  ((x)==sgeE_MANAGER_LIST) || \
  ((x)==sgeE_OPERATOR_LIST) || \
  ((x)==sgeE_PE_LIST) || \
  ((x)==sgeE_PROJECT_LIST) || \
  ((x)==sgeE_CQUEUE_LIST) || \
  ((x)==sgeE_SUBMITHOST_LIST) || \
  ((x)==sgeE_USER_LIST) || \
  ((x)==sgeE_USERSET_LIST) || \
  ((x)==sgeE_HGROUP_LIST) || \
  ((x)==sgeE_SHUTDOWN) || \
  ((x)==sgeE_QMASTER_GOES_DOWN) || \
  ((x)==sgeE_ACK_TIMEOUT))


enum {
   ET_number = ET_LOWERBOUND,/* number of the event */
   ET_timestamp,             /* unix time stamp (gmt) when the event occured */
   ET_type,                  /* type the event */
   ET_intkey,                /* a int key for use by a specific event type */
   ET_intkey2,               /* a int key for use by a specific event type */
   ET_strkey,                /* a str key for use by a specific event type */
   ET_strkey2,               /* a str key for use by a specific event type */
   ET_new_version            /* new version of the changed object */
                             /* JG: TODO: we should have different fields for 
                              *           objects (SGE_OBJECT) and
                              *           lists   (SGE_LIST), as we now have
                              *           proper handling for objects.
                              */
};

LISTDEF(ET_Type)
   JGDI_OBJ(Event)
   SGE_ULONG(ET_number, CULL_DEFAULT)
   SGE_ULONG(ET_timestamp, CULL_DEFAULT)
   SGE_ULONG(ET_type, CULL_DEFAULT)
   SGE_ULONG(ET_intkey, CULL_DEFAULT)
   SGE_ULONG(ET_intkey2, CULL_DEFAULT)
   SGE_STRING(ET_strkey, CULL_DEFAULT)
   SGE_STRING(ET_strkey2, CULL_DEFAULT)
   SGE_LIST(ET_new_version, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND 

NAMEDEF(ETN)
   NAME("ET_number")
   NAME("ET_timestamp")
   NAME("ET_type")
   NAME("ET_intkey")
   NAME("ET_intkey2")
   NAME("ET_strkey")
   NAME("ET_strkey2")
   NAME("ET_new_version")
NAMEEND
#define ETS sizeof(ETN)/sizeof(char*)

enum {
   EVS_id = EVS_LOWERBOUND,
   EVS_flush,
   EVS_interval,
   EVS_what,
   EVS_where
};

LISTDEF(EVS_Type)
   JGDI_OBJ(EventSubcribtion)
   SGE_ULONG(EVS_id, CULL_DEFAULT)
   SGE_BOOL(EVS_flush, CULL_DEFAULT)
   SGE_ULONG(EVS_interval, CULL_DEFAULT)
   SGE_OBJECT(EVS_what, CULL_ANY_SUBTYPE, CULL_DEFAULT)
   SGE_OBJECT(EVS_where, CULL_ANY_SUBTYPE, CULL_DEFAULT)
LISTEND

NAMEDEF(EVSN)
   NAME("EVS_id") 
   NAME("EVS_flush")
   NAME("EVS_interval")
   NAME("EVS_what")
   NAME("EVS_where")
NAMEEND   
#define EVSS sizeof(EVSN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_EVENTL_H */
