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

/* valid values for EV_id */
enum {
   EV_ID_ANY = 0,            /* qmaster will give the ev a unique id */
   EV_ID_SCHEDD = 1,         /* schedd registers at qmaster */
   EV_ID_FIRST_DYNAMIC = 11  /* first id given by qmaster for EV_ID_ANY registration */ 
};

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
   EV_last_heard_from,         /* used to trash unheard event clients */
   EV_last_send_time,        /* time when last event list has been sent */
   EV_next_send_time,        /* time when next list has to be sent */
   EV_next_number,           /* the number the next event will get */
   EV_subscription,          /* subscription information */
   EV_events                 /* used to hold the events that */
                             /* are not acknowledged */
};

LISTDEF(EV_Type)
   SGE_ULONG(EV_id)
   SGE_STRING(EV_name)
   SGE_HOST(EV_host)
   SGE_STRING(EV_commproc)
   SGE_ULONG(EV_commid)
   SGE_ULONG(EV_uid)
   SGE_ULONG(EV_d_time)
   SGE_ULONG(EV_last_heard_from)
   SGE_ULONG(EV_last_send_time)
   SGE_ULONG(EV_next_send_time)
   SGE_ULONG(EV_next_number)
   SGE_STRING(EV_subscription)
   SGE_LIST(EV_events)
LISTEND 

NAMEDEF(EVN)
   NAME("EV_id")
   NAME("EV_name")
   NAME("EV_host")
   NAME("EV_commproc")
   NAME("EV_commid")
   NAME("EV_uid")
   NAME("EV_d_time")
   NAME("EV_last_heard_from")
   NAME("EV_last_send_time")
   NAME("EV_next_send_time")
   NAME("EV_next_number")
   NAME("EV_subscription")
   NAME("EV_events")
NAMEEND

#define EVS sizeof(EVN)/sizeof(char*)

/* valid values for ET_type */
enum {
   sgeE_ALL_EVENTS,

   sgeE_JOB_DEL,
   sgeE_JOB_ADD,
   sgeE_JOB_MOD,
   sgeE_JOB_LIST,
   sgeE_JOB_MOD_SCHED_PRIORITY,
   sgeE_JOB_USAGE,
   sgeE_FINAL_USAGE,

   sgeE_QUEUE_DEL,
   sgeE_QUEUE_ADD,
   sgeE_QUEUE_MOD,
   sgeE_QUEUE_LIST,
   sgeE_QUEUE_SUSPEND_ON_SUB,        /* dont wanna lose */
   sgeE_QUEUE_UNSUSPEND_ON_SUB,      /* any information */

   sgeE_COMPLEX_DEL,
   sgeE_COMPLEX_ADD,
   sgeE_COMPLEX_MOD,
   sgeE_COMPLEX_LIST,

   sgeE_EXECHOST_DEL,
   sgeE_EXECHOST_ADD,
   sgeE_EXECHOST_MOD,
   sgeE_EXECHOST_LIST,

   sgeE_USERSET_DEL,
   sgeE_USERSET_ADD,
   sgeE_USERSET_MOD,
   sgeE_USERSET_LIST,

   sgeE_USER_DEL,
   sgeE_USER_ADD,
   sgeE_USER_MOD,
   sgeE_USER_LIST,

   sgeE_PROJECT_DEL,
   sgeE_PROJECT_ADD,
   sgeE_PROJECT_MOD,
   sgeE_PROJECT_LIST,

   sgeE_PE_DEL,
   sgeE_PE_ADD,
   sgeE_PE_MOD,
   sgeE_PE_LIST,

   sgeE_SHUTDOWN,
   sgeE_QMASTER_GOES_DOWN,
   sgeE_SCHEDDMONITOR,
   sgeE_GLOBAL_CONFIG,

   sgeE_NEW_SHARETREE,       /* replace possibly existing share tree */
   sgeE_SCHED_CONF,          /* replace existing (sge) scheduler
                              * configuration */

   sgeE_CKPT_DEL,
   sgeE_CKPT_ADD,
   sgeE_CKPT_MOD,
   sgeE_CKPT_LIST,

   sgeE_JATASK_DEL,
   sgeE_JATASK_MOD,

   sgeE_EVENTSIZE 
};

enum {
   ET_number = ET_LOWERBOUND,        /* number of the event */
   ET_type,                  /* type the event */
   ET_intkey,                /* a int key for use by a specific event type */
   ET_intkey2,               /* a int key for use by a specific event type */
   ET_strkey,                /* a str key for use by a specific event type */
   ET_new_version            /* new version of the changed object */
};

LISTDEF(ET_Type)
   SGE_ULONG(ET_number)
   SGE_ULONG(ET_type)
   SGE_ULONG(ET_intkey)
   SGE_ULONG(ET_intkey2)
   SGE_STRING(ET_strkey)
   SGE_LIST(ET_new_version)
LISTEND 

NAMEDEF(ETN)
   NAME("ET_number")
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
