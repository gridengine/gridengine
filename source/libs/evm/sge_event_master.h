#ifndef __SGE_M_EVENT_H
#define __SGE_M_EVENT_H
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

#include "basis_types.h"

#include "sgeobj/sge_eventL.h"

#include "gdi/sge_gdi_ctx.h"

#include "uti/sge_monitor.h"

/*
 ***** event_master_control_t definition ********************
 *
 * This struct contains all the control information needed
 * to have the event master running. It contains the references
 * to all lists, mutexes, and booleans.
 *
 ***********************************************************
 */
typedef struct {
   pthread_mutex_t  transaction_mutex;    /* a mutex ensuring that only one transaction is running at a time */
   lList            *transaction_events;  /* a list storing all events happening, while a transaction is open, a
                                             transaction is not thread specific*/
   pthread_mutex_t  t_add_event_mutex;    /* guarding the transaction_events list and the boolean is_transaction */
   bool             is_transaction;       /* identifies, if a transaction is open, or not */
   pthread_mutex_t  mutex;                 /* used for mutual exclusion. only use in public functions   */
   pthread_cond_t   lockfield_cond_var;    /* used for waiting                                          */
   pthread_cond_t   waitfield_cond_var;    /* used for waiting                                          */
   pthread_cond_t   cond_var;              /* used for waiting                                          */
   pthread_mutex_t  cond_mutex;            /* used for mutual exclusion. only use in internal functions */
   pthread_mutex_t  ack_mutex;             /* guards the ack list                                        */
   pthread_mutex_t  send_mutex;            /* guards the event send list                                 */
   pthread_mutex_t  change_evc_mutex;      /* guards the event client change requests                    */

   u_long32         max_event_clients;     /* contains the max number of custom event clients, the      */
                                           /* scheduler is not accounted for. protected by mutex.       */
   u_long32         len_event_clients;     /* contains the length of the custom event clients array     */
                                           /* protected by mutex.                                       */

   bool             delivery_signaled;     /* signals that an event delivery has been signaled          */
                                           /* protected by cond_mutex.                                  */

   bool             is_prepare_shutdown;   /* is set, when the qmaster is going down. Do not accept     */
                                           /* new event clients, when this is set to false. protected   */
                                           /* by mutex.                                                 */
   bool             lock_all;              /* true -> all event client locks are occupied.              */
                                           /* protected by mutex                                        */
   lList*           clients;               /* list of event master clients                              */
                                           /* protected by mutex                                        */
   lList*           ack_events;            /* list of events to ack                                     */
                                           /* protected by ack_mutex                                    */
   lList*           send_events;           /* list of events to send                                    */
                                           /* protected by send_mutex                                   */
   lList*           change_evc;            /* evc change requests                                       */

   /* 
    * A client locked via the lockfield is only locked for content.  Any changes
    * to the clients list, such as adding and removing clients, requires the
    * locking of the mutex.
    */
   bitfield         *lockfield;            /* bitfield of locks for event clients                       */
                                           /* protected by mutex                                        */
   /* 
    * In order to allow the locking of individual clients, I have added an array
    * of pointers to the client entries in the clients list.  This was needed
    * because otherwise looping through the registered clients requires the
    * locking of the entire list.  (Without the array, the id, which is the key
    * for the lockfield, is part of the content of the client, which has to be
    * locked to be accessed, which is what we're trying to double in the first
    * place!
    */

   /* 
    * For simplicity's sake, rather than completely replacing the clients list
    * with an array, I have made the array an overlay for the list.  This allows
    * all the old code to function while permitting new code to use the array
    * for faster access.  Specifically, the lSelect in add_list_event_direct()
    * continues to work without modification.
    */
   lListElem**      clients_array;         /* array of pointers to event clients                        */
                                           /* protected by lockfield/mutex                              */
   /* 
    * In order better optimize for the normal case, i.e. few event clients, I
    * have added this indices array.  It contains the indices for the non-NULL
    * entries in the clients_array.  It gets rebuilt by the event thread
    * whenever an event client is added or removed.  Now, instead of having to
    * search through 109 (99 dynamic + 10 static) entries to find the 2 or 3
    * that aren't NULL, *every* time through the loop, the event thread only has
    * to search for the non-NULL entries when something changes.
    */
   int*             clients_indices;       /* array of currently active entries in the clients_array    */
                                           /* unprotected -- only used by event thread                  */
   bool             indices_dirty;         /* whether the clients_indices array needs to be updated     */
                                           /* protected by mutex                                        */
} event_master_control_t;

extern event_master_control_t Event_Master_Control;

void sge_event_master_process_sends(monitoring_t *monitor);
void sge_event_master_process_mod_event_client(monitoring_t *monitor);
void sge_event_master_process_acks(monitoring_t *monitor);
void sge_event_master_send_events(sge_gdi_ctx_class_t *ctx, lListElem *report, lList *report_list, monitoring_t *monitor);
void sge_event_master_wait_next(void);

int sge_add_event_client(lListElem *ev,
                         lList **alpp,
                         lList **eclpp,
                         char *ruser,
                         char *rhost,
                         event_client_update_func_t update_func,
                         monitoring_t *monitor);

int sge_mod_event_client(lListElem *clio, lList **alpp, char *ruser, char *rhost);
bool sge_has_event_client(u_long32 aClientID);
void   sge_remove_event_client(u_long32 aClientID);
lList* sge_select_event_clients(const char *aNewList, const lCondition *aCond, const lEnumeration *anEnum);
int sge_shutdown_event_client(u_long32 aClientID, const char* anUser, uid_t anUID, lList **alpp, monitoring_t *monitor);
int sge_shutdown_dynamic_event_clients(const char *anUser, lList **alpp, monitoring_t *monitor);

bool sge_add_event(u_long32 timestamp,
                   ev_event type,
                   u_long32 intkey,
                   u_long32 intkey2,
                   const char *strkey,
                   const char *strkey2, 
                   const char *session,
                   lListElem *element);
                          
bool sge_add_event_for_client(u_long32 aClientID,
                              u_long32 aTimestamp,
                              ev_event type,
                              u_long32 anIntKey1, 
                              u_long32 anIntKey2,
                              const char *aStrKey1,
                              const char *aStrKey2, 
                              const char *aSession,
                              lListElem *element);
                                    
bool sge_add_list_event(u_long32 timestamp,
                        ev_event type, 
                        u_long32 intkey,
                        u_long32 intkey2,
                        const char *strkey, 
                        const char *strkey2,
                        const char *session,
                        lList *list);

void sge_handle_event_ack(u_long32 aClientID, ev_event anEvent);
void sge_deliver_events_immediately(u_long32 aClientID);

int sge_resync_schedd(monitoring_t *monitor);

u_long32 sge_set_max_dynamic_event_clients(u_long32 max);
u_long32 sge_get_max_dynamic_event_clients(void);

void sge_event_master_shutdown(void);
void sge_event_master_init(void);
bool sge_commit(void);
void sge_set_commit_required(void);
bool sge_is_commit_required(void);

#endif /* __SGE_M_EVENT_H */
