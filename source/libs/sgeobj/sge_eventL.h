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
#include "cull_packL.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* documentation see libs/evc/sge_event_client.c */
typedef enum {
   EV_ID_ANY = 0,            /* qmaster will give the ev a unique id */
   EV_ID_SCHEDD = 1,         /* schedd registers at qmaster */
   EV_ID_FIRST_DYNAMIC = 11  /* first id given by qmaster for EV_ID_ANY registration */ 
}ev_registration_id;


/* documentation see libs/evc/sge_event_client.c */
/* #define EV_NO_FLUSH -1*/

#define EV_NOT_SUBSCRIBED 0 
#define EV_SUBSCRIBED 1 
#define EV_FLUSHED 1 
#define EV_NOT_FLUSHED 0
#define EV_MAX_FLUSH 0x3f
#define EV_NO_FLUSH -1 

/* documentation see libs/evc/sge_event_client.c */

typedef enum {
   EV_BUSY_NO_HANDLING = 0,
   EV_BUSY_UNTIL_ACK,
   EV_BUSY_UNTIL_RELEASED,
   EV_THROTTLE_FLUSH
} ev_busy_handling;


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
   EV_subscribed,            /* a list of subscriped events */
   
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

   NAME("EV_clientdata")
NAMEEND

#define EVS sizeof(EVN)/sizeof(char*)





/* documentation see libs/evc/sge_event_client.c */
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

   sgeE_CENTRY_LIST,                /* + send complex list at registration */
   sgeE_CENTRY_ADD,                 /* + event add complex */
   sgeE_CENTRY_DEL,                 /* + event delete complex */
   sgeE_CENTRY_MOD,                 /* + event modify complex */

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

   /* EB: TODO: QI event */

   sgeE_CQUEUE_LIST,                /* + send cluster queue list at registration */
   sgeE_CQUEUE_ADD,                 /* + event cluster queue add */
   sgeE_CQUEUE_DEL,                 /* + event cluster queue delete */
   sgeE_CQUEUE_MOD,                 /* + event cluster queue modify */

   sgeE_QINSTANCE_ADD,              /* + event queue instance add */
   sgeE_QINSTANCE_DEL,              /* + event queue instance delete */
   sgeE_QINSTANCE_MOD,              /* + event queue instance mod */
   sgeE_QINSTANCE_SOS,              /* + event queue instance sos */
   sgeE_QINSTANCE_USOS,             /* + event queue instance usos */

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
   sgeE_CUSER_LIST,
   sgeE_CUSER_ADD,
   sgeE_CUSER_DEL,
   sgeE_CUSER_MOD,
#endif

   sgeE_HGROUP_LIST,
   sgeE_HGROUP_ADD,
   sgeE_HGROUP_DEL,
   sgeE_HGROUP_MOD,

   sgeE_EVENTSIZE 
}ev_event;

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
  ((x)==sgeE_QMASTER_GOES_DOWN))


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
   SGE_ULONG(EVS_id, CULL_DEFAULT)
   SGE_BOOL(EVS_flush, CULL_DEFAULT)
   SGE_ULONG(EVS_interval, CULL_DEFAULT)
   SGE_OBJECT(EVS_what, PACK_Type, CULL_DEFAULT)
   SGE_OBJECT(EVS_where, PACK_TYPE,CULL_DEFAULT)
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
