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

#include "sge_event_master.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sge.h"
#include "cull.h"
#include "sge_feature.h"
#include "sge_time.h"
#include "sge_host.h"
#include "sge_event.h"
#include "sge_all_listsL.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_conf.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_report.h"
#include "sge_ckpt.h"
#include "sge_pe.h"
#include "sge_userprj.h"
#include "sge_job.h"
#include "sge_hostname.h"
#include "sge_userset.h"
#include "sge_manop.h"
#include "sge_calendar.h"
#include "sge_sharetree.h"
#include "sge_hgroup.h"
#include "sge_cuser.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_object.h"
#include "sge_mtutil.h"
#include "setup_qmaster.h"
#include "cl_errors.h"

#include "msg_common.h"
#include "msg_evmlib.h"

/****** subscription_t definition **********************
 *
 * This is a subscription entry in a two dimensional 
 * definition. The first dimension is for the event
 * clients and changes when the event client subscription
 * changes. The second dimension is of fixed size and 
 * contains one element for each posible event.
 *
 *******************************************************/
typedef struct {
      bool         subscription; /* true -> the event is subscribed           */
      bool         flush;        /* true -> flush is set                      */
      u_long32     flush_time;   /* seconds how much the event can be delayed */
      lCondition   *where;       /* where filter                              */
      lDescr       *descr;       /* target list descriptor                    */
      lEnumeration *what;        /* limits the target element                 */
} subscription_t;

typedef struct {
   pthread_mutex_t  mutex;                 /* used for mutual exclusion                           */
   pthread_cond_t   cond_var;              /* used for waiting                                    */
   pthread_mutex_t  cond_mutex;            /* used for mutual exclusion                           */
   int              clients_deliver_count; /* counts the event clients, which have pending events */
   bool             exit;                  /* true -> exit event master                           */
   lList*           clients;               /* list of event master clients                        */
} event_master_control_t;

/****** Eventclient/Server/-Event_Client_Server_Defines ************************
*  NAME
*     Defines -- Constants used in the module
*
*  SYNOPSIS
*     #define FLUSH_INTERVAL 15
*     #define EVENT_ACK_MIN_TIMEOUT 600
*     #define EVENT_ACK_MAX_TIMEOUT 1200
*
*  FUNCTION
*     EVENT_DELIVERY_INTERVAL_S is the event delivery interval. It is set in seconds.
*     EVENT_DELIVERY_INTERVAL_N same thing but in nano seconds.
*     
*
*     EVENT_ACK_MIN/MAX_TIMEOUT is the minimum/maximum timeout value for an event
*     client sending the acknowledge for the delivery of events.
*     The real timeout value depends on the event delivery interval for the 
*     event client (10 * event delivery interval).
*
*******************************************************************************/
#define EVENT_DELIVERY_INTERVAL_S 1 
#define EVENT_DELIVERY_INTERVAL_N 0
#define EVENT_ACK_MIN_TIMEOUT 600
#define EVENT_ACK_MAX_TIMEOUT 1200

/********************************************************************
 *
 * The next three array are important for lists, which can be
 * subscripted by the event client and which contain a sub-list
 * that can be subscripted by it self again (such as the: job list
 * with the JAT_Task sub list, or the cluster queue list with the
 * queue instances sub-list)
 * All lists, which follow the same structure have to be defined
 * in the special construct.
 *
 * EVENT_LIST:
 *  Contains all events for the main list, which delivers also
 *  the sub-list. 
 *
 * FIELD_LIST:
 *  Contains all attributes in the main list, which contain the
 *  sub-list in question.
 *
 * SOURCE_LIST:
 *  Contains the sub-scription events for the sub-list, which als
 *  contains the filter for the sub-list.
 *
 *
 * This construct and its functions are limited to one sub-scribable
 * sub-list per main list. If multiple sub-lists can be subsribed, the
 * construct has to be exetended.
 *
 *
 * SEE ALSO:
 *     evm/sge_event_master/list_select()
 *     evm/sge_event_master/elem_select() 
 *  and
 *     evm/sge_event_master/add_list_event
 *     evm/sge_event_master/add_event
 *
 *********************************************************************/
#define LIST_MAX 2

const int EVENT_LIST[LIST_MAX][6] = {
   {sgeE_JOB_LIST, sgeE_JOB_ADD, sgeE_JOB_DEL, sgeE_JOB_MOD, sgeE_JOB_MOD_SCHED_PRIORITY, -1},
   {sgeE_CQUEUE_LIST, sgeE_CQUEUE_ADD, sgeE_CQUEUE_DEL, sgeE_CQUEUE_MOD, -1, -1},
};

const int FIELD_LIST[LIST_MAX][3] = {
   {JB_ja_tasks, JB_ja_template, -1},
   {CQ_qinstances, -1, -1},
};

const int SOURCE_LIST[LIST_MAX][3] = {
   {sgeE_JATASK_MOD, sgeE_JATASK_ADD, -1},
   {sgeE_QINSTANCE_ADD, sgeE_QINSTANCE_MOD, -1},
};

/********************************************************************
 *
 * Some events have to be delivered even so they have no date left
 * after filtering for them. These are for example all update list
 * events. 
 * The ensure, that is is done as fast as posible, we define an 
 * array of the size of the number of events, we have a init function
 * which sets the events which will be updated. To add a new event
 * ones has only to update that function.
 * Events which do not contain any data are not affected. They are
 * allways delivered.
 *
 * SEE ALSO:
 * - Array:
 *    SEND_EVENTS
 *
 * - Init function:
 *    evm/sge_event_master/sge_init_send_events()
 *
 *******************************************************************/
static bool SEND_EVENTS[sgeE_EVENTSIZE]; 

static event_master_control_t Master_Control = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 
                                                PTHREAD_MUTEX_INITIALIZER, 0, false, NULL};
static pthread_once_t         Event_Master_Once = PTHREAD_ONCE_INIT;
static pthread_t              Event_Thread;

static void       event_master_once_init(void);
static void       init_send_events(void); 
static void*      send_thread(void*);
static bool       should_exit(void);
static void       send_events(void);
static void       flush_events(lListElem*, int);
static void       total_update(lListElem*);
static void       build_subscription(lListElem*);
static void       check_send_new_subscribed_list(const subscription_t*, const subscription_t*, lListElem*, ev_event);
static int        eventclient_subscribed(const lListElem *, ev_event, const char*);
static int        purge_event_list(lList* aList, ev_event anEvent); 
static int        add_list_event(lListElem*, u_long32, ev_event, u_long32, u_long32, const char*, const char*, lList*, int); 
static void       add_event(lListElem*, u_long32, ev_event, u_long32, u_long32, const char*, const char*, lListElem*);
static void       total_update_event(lListElem*, ev_event);
static bool       list_select(subscription_t*, int, lList**, lList*, const lCondition*, const lEnumeration*, const lDescr*);
static lListElem* elem_select(subscription_t*, lListElem*, const int[], const lCondition*, const lEnumeration*, const lDescr*, int);    
static lListElem* eventclient_list_locate_by_adress(const char*, const char*, u_long32);
static const lDescr* getDescriptor(subscription_t*, const lListElem*, int);
static const lDescr* getDescriptorL(subscription_t*, const lList*, int);


/****** Eventclient/Server/sge_add_event_client() ******************************
*  NAME
*     sge_add_event_client() -- register a new event client
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     int 
*     sge_add_event_client(lListElem *clio, lList **alpp, lList **eclpp, 
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
*     MT-NOTE: sge_add_event_client() is NOT MT safe.
*
*******************************************************************************/
int sge_add_event_client(lListElem *clio, lList **alpp, lList **eclpp, char *ruser, char *rhost)
{
   lListElem *ep=NULL;
   u_long32 now;
   u_long32 id;
   u_long32 ed_time;
   const char *name;
   lList *subscription;
   static u_long32 first_dynamic_id = EV_ID_FIRST_DYNAMIC;
   const char *host;
   const char *commproc;
   u_long32 commproc_id;

   DENTER(TOP_LAYER,"sge_add_event_client");

   pthread_once(&Event_Master_Once, event_master_once_init);

   id = lGetUlong(clio, EV_id);
   name = lGetString(clio, EV_name);
   subscription = lGetList(clio, EV_subscribed);
   ed_time = lGetUlong(clio, EV_d_time);
   host = lGetHost(clio, EV_host);
   commproc = lGetString(clio, EV_commproc);
   commproc_id = lGetUlong(clio, EV_commid);

   if (name == NULL) {
      name = "unnamed";
      lSetString(clio, EV_name, name);
   }
   
   if (id < EV_ID_ANY || id >= EV_ID_FIRST_DYNAMIC) { /* invalid request */
      ERROR((SGE_EVENT, MSG_EVE_ILLEGALIDREGISTERED_U, u32c(id)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);

      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if (subscription == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDSUBSCRIPTION));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);

      DEXIT;
      return STATUS_ESEMANTIC;
   }

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if (id == EV_ID_ANY) {   /* qmaster shall give id dynamically */
      id = first_dynamic_id++;
      lSetUlong(clio, EV_id, id);

      /* Try to find an event client with the very same commd 
         adress triple. If it exists the "new" event client must 
         be result of a reconnect after a timeout that happend at 
         client side. We delete the old event client. */ 
      if ((ep = eventclient_list_locate_by_adress(
            host, commproc, commproc_id))) {
         ERROR((SGE_EVENT, MSG_EVE_CLIENTREREGISTERED_SSSU, name, host, 
            commproc, u32c(commproc_id)));         

         /* delete old event client entry */
         lRemoveElem(Master_Control.clients, ep);
      } else {
         INFO((SGE_EVENT, MSG_EVE_REG_SUU, name, u32c(id), u32c(ed_time)));         
      }
   }

   /* special event clients: we allow only one instance */
   /* if it already exists, delete the old one and register */
   /* the new one */
   if (id > EV_ID_ANY && id < EV_ID_FIRST_DYNAMIC) {
      if ((ep = lGetElemUlong(Master_Control.clients, EV_id, id))) {
         /* we already have this special client */
         ERROR((SGE_EVENT, MSG_EVE_CLIENTREREGISTERED_SSSU, name, host, 
                commproc, u32c(commproc_id)));         

         /* delete old event client entry */
         lRemoveElem(Master_Control.clients, ep);
      } else {
         INFO((SGE_EVENT, MSG_EVE_REG_SUU, name, u32c(id), u32c(ed_time)));         
      }   
   }

   ep=lCopyElem(clio);
   lSetBool(clio, EV_changed, false);

   lAppendElem(Master_Control.clients,ep);
   lSetUlong(ep, EV_next_number, 1);

   /* register this contact */
   now = sge_get_gmt();
   lSetUlong(ep, EV_last_send_time, 0);
   lSetUlong(ep, EV_next_send_time, now + lGetUlong(ep, EV_d_time));
   lSetUlong(ep, EV_last_heard_from, now);

   /* return new event client object to event client */
   if (eclpp != NULL) {
      lListElem *ret_el = lCopyElem(ep);
      if (*eclpp == NULL) {
         *eclpp = lCreateList("new event client", EV_Type);
      }
      lSetBool(ret_el, EV_changed, false);
      lAppendElem(*eclpp, ret_el);
   }

   build_subscription(ep);

   /* build events for total update */
   total_update(ep);

   /* flush initial list events */
   flush_events(ep, 0);

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   INFO((SGE_EVENT, MSG_SGETEXT_ADDEDTOLIST_SSSS,
         ruser, rhost, name, MSG_EVE_EVENTCLIENT));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT; 
   return STATUS_OK;
} /* sge_add_event_client() */

/****** Eventclient/Server/sge_mod_event_client() ******************************
*  NAME
*     sge_mod_event_client() -- modify event client
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     int 
*     sge_mod_event_client(lListElem *clio, lList **alpp, lList **eclpp, 
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
*  NOTES
*     MT-NOTE: sge_mod_event_client() is NOT MT safe.
*
*******************************************************************************/
int sge_mod_event_client(lListElem *clio, lList **alpp, lList **eclpp, char *ruser, char *rhost) 
{
   lListElem *event_client=NULL;
   u_long32 id;
   u_long32 busy;
   u_long32 ev_d_time;

   DENTER(TOP_LAYER,"sge_mod_event_client");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   /* try to find event_client */
   id = lGetUlong(clio, EV_id);

   event_client = lGetElemUlong(Master_Control.clients, EV_id, id);

   if (event_client == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(id), "modify"));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);

      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

      DEXIT;
      return STATUS_EEXIST;
   }

   /* these parameters can be changed */
   busy         = lGetUlong(clio, EV_busy);
   ev_d_time    = lGetUlong(clio, EV_d_time);

   /* check for validity */
   if (ev_d_time < 1) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDINTERVAL_U, u32c(ev_d_time)));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);

      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

      DEXIT;
      return STATUS_ESEMANTIC;
   }

   if (lGetList(clio, EV_subscribed) == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_INVALIDSUBSCRIPTION));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);

      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

      DEXIT;
      return STATUS_ESEMANTIC;
   }

   /* event delivery interval changed. 
    * We have to update the next delivery time to 
    * next_delivery_time - old_interval + new_interval 
    */
   if (ev_d_time != lGetUlong(event_client, EV_d_time)) {
      lSetUlong(event_client, EV_next_send_time, 
                lGetUlong(event_client, EV_next_send_time) - 
                lGetUlong(event_client, EV_d_time) + ev_d_time);
      lSetUlong(event_client, EV_d_time, ev_d_time);
   }

   /* subscription changed */
   /*old_subscription = lGetString(event_client, EV_subscription);*/
   if (lGetBool(clio, EV_changed)) {
      subscription_t *new_sub = NULL; 
      subscription_t *old_sub = NULL; 

      build_subscription(clio);
      new_sub = lGetRef(clio, EV_sub_array);
      old_sub = lGetRef(event_client, EV_sub_array);
 
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_ADMINHOST_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CALENDAR_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CKPT_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CENTRY_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CONFIG_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_EXECHOST_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_JOB_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_JOB_SCHEDD_INFO_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_MANAGER_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_OPERATOR_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_NEW_SHARETREE);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_PE_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_PROJECT_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CQUEUE_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_SUBMITHOST_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_USER_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_USERSET_LIST);
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_HGROUP_LIST);

#ifndef __SGE_NO_USERMAPPING__
      check_send_new_subscribed_list(old_sub, new_sub, event_client, sgeE_CUSER_LIST);
#endif      

      lSetList(event_client, EV_subscribed, lCopyList("", lGetList(clio, EV_subscribed)));
      lSetRef(event_client, EV_sub_array, new_sub);
      lSetRef(clio, EV_sub_array, NULL);
      if (old_sub){
         int i;
         for (i=0; i<sgeE_EVENTSIZE; i++){ 
            if (old_sub[i].where)
               lFreeWhere(old_sub[i].where);
            if (old_sub[i].what)
               lFreeWhat(old_sub[i].what);
            if (old_sub[i].descr){
               cull_hash_free_descr(old_sub[i].descr);
               free(old_sub[i].descr);
            }
         } 
         FREE(old_sub);

      }
   }

   /* busy state changed */
   if (busy != lGetUlong(event_client, EV_busy)) {
      lSetUlong(event_client, EV_busy, busy);
   }

   DEBUG((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS,
         ruser, rhost, lGetString(event_client, EV_name), MSG_EVE_EVENTCLIENT));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   /* return modified event client object to event client */
   if (eclpp != NULL) {
      if (*eclpp == NULL) {
         *eclpp = lCreateList("modified event client", EV_Type);
      }

      lAppendElem(*eclpp, lCopyElem(event_client));
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT; 
   return STATUS_OK;
} /* sge_mod_event_client() */

/****** evm/sge_event_master/sge_event_client_registered() *********************
*  NAME
*     sge_event_client_registered() -- check if event client is registered
*
*  SYNOPSIS
*     bool sge_event_client_registered(u_long32 aClientID) 
*
*  FUNCTION
*     Check if event client is registered. 
*
*  INPUTS
*     u_long32 aClientID - event client id to check
*
*  RESULT
*     true  - client is registered 
*     false - otherwise
*
*  NOTES
*     MT-NOTE: sge_event_client_registered() is NOT MT safe. 
*
*******************************************************************************/
bool sge_event_client_registered(u_long32 aClientID)
{
   bool res = false;

   DENTER(TOP_LAYER, "sge_event_client_registered");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if (Master_Control.clients != NULL &&
       lGetElemUlong(Master_Control.clients, EV_id, aClientID) != NULL) {
      res = true;
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* sge_event_client_registered() */

/****** evm/sge_event_master/sge_remove_event_client() *************************
*  NAME
*     sge_remove_event_client() -- remove event client 
*
*  SYNOPSIS
*     void sge_remove_event_client(u_long32 aClientID) 
*
*  FUNCTION
*     Remove event client. Fetch event client from event client list. Purge
*     event subscription array. Remove event client from event client list.
*
*  INPUTS
*     u_long32 aClientID - event client id 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_remove_event_client() is NOT MT safe. 
*
*******************************************************************************/
void sge_remove_event_client(u_long32 aClientID)
{
   lListElem *client;

   DENTER(TOP_LAYER, "sge_remove_event_client");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((Master_Control.clients == NULL) ||
      (client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL) {

      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));
   }
   else {
      int i;
      subscription_t *old_sub = lGetRef(client, EV_sub_array);

      INFO((SGE_EVENT, MSG_EVE_UNREG_SU, lGetString(client, EV_name), u32c(lGetUlong(client, EV_id))));

      if (old_sub) {
         for (i=0; i<sgeE_EVENTSIZE; i++){ 
            if (old_sub[i].where)
               lFreeWhere(old_sub[i].where);
            if (old_sub[i].what)
               lFreeWhat(old_sub[i].what);
            if (old_sub[i].descr){
               cull_hash_free_descr(old_sub[i].descr);
               free(old_sub[i].descr);
            }
         } 
         free(old_sub);
         lSetRef(client, EV_sub_array, NULL);
      }

      lRemoveElem(Master_Control.clients, client);
   }
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return;
} /* sge_remove_event_client() */

/****** evm/sge_event_master/sge_select_event_clients() ************************
*  NAME
*     sge_select_event_clients() -- select event clients 
*
*  SYNOPSIS
*     lList* sge_select_event_clients(const char *aNewList, const lCondition 
*     *aCond, const lEnumeration *anEnum) 
*
*  FUNCTION
*     Select event clients.  
*
*  INPUTS
*     const char *aNewList       - name of the result list returned. 
*     const lCondition *aCond    - where condition 
*     const lEnumeration *anEnum - what enumeration
*
*  RESULT
*     lList* - list with elements of type 'EV_Type'.
*
*  NOTES
*     MT-NOTE: sge_select_event_clients() is MT safe 
*     MT-NOTE:
*     MT-NOTE: The elements contained in the result list are copies of the
*     MT-NOTE: respective event client list elements.
*
*******************************************************************************/
lList* sge_select_event_clients(const char *aNewList, const lCondition *aCond, const lEnumeration *anEnum)
{
   lList *lst = NULL;

   DENTER(TOP_LAYER, "sge_select_event_clients");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if (Master_Control.clients != NULL) {
      lst = lSelect(aNewList, (const lList*)Master_Control.clients, aCond, anEnum);
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return lst;
} /* sge_select_event_clients() */

/****** evm/sge_event_master/sge_shutdown_event_client() ***********************
*  NAME
*     sge_shutdown_event_client() -- shutdown an event client 
*
*  SYNOPSIS
*     int sge_shutdown_event_client(u_long32 aClientID, const char* anUser, 
*     uid_t anUID) 
*
*  FUNCTION
*     Shutdown an event client. Send the event client denoted by 'aClientID' 
*     a shutdown event.
*
*     Shutting down an event client is only permitted if 'anUser' does have
*     manager privileges OR is the owner of event client 'aClientID'.
*
*  INPUTS
*     u_long32 aClientID - event client ID 
*     const char* anUser - user which did request this operation 
*     uid_t anUID        - user id of request user
*
*  RESULT
*     EPERM - operation not permitted  
*     ESRCH - client with given client id is unknown
*     0     - otherwise
*
*  NOTES
*     MT-NOTE: sge_shutdown_event_client() is not MT safe. 
*
*******************************************************************************/
int sge_shutdown_event_client(u_long32 aClientID, const char* anUser, uid_t anUID)
{
   lListElem *client = NULL;
   int ret = 0;
   DENTER(TOP_LAYER, "sge_shutdown_event_client");
   
   if (!manop_is_manager(anUser) && (anUID != lGetUlong(client, EV_uid))) {

      ERROR((SGE_EVENT, MSG_COM_NOSHUTDOWNPERMS));
      DEXIT;
      return EPERM;
   }

   ret = sge_add_event_for_client(aClientID, 0, sgeE_SHUTDOWN, 0,0,NULL,NULL,NULL,NULL);

   if (ret == 0){
      lListElem *client;
      sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);
      client = lGetElemUlong(Master_Control.clients, EV_id, aClientID);
      if (client != NULL) {
         INFO((SGE_EVENT, MSG_COM_SHUTDOWNNOTIFICATION_SUS, lGetString(client, EV_name), lGetUlong(client, EV_id), lGetHost(client, EV_host))); 
      }
      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   }

   DEXIT;
   return ret;
} /* sge_shutdown_event_client */

/****** evm/sge_event_master/sge_shutdown_dynamic_event_clients() **************
*  NAME
*     sge_shutdown_dynamic_event_clients() -- shutdown all dynamic event clients
*
*  SYNOPSIS
*     int sge_shutdown_dynamic_event_clients(const char *anUser) 
*
*  FUNCTION
*     Shutdown all dynamic event clients. Each dynamic event client known will
*     be send a shutdown event.
*
*     An event client is a dynamic event client if it's client id is greater
*     than or equal to 'EV_ID_FIRST_DYNAMIC'. 
*
*     Shutting down all dynamic event clients is only permitted if 'anUser' does
*     have manager privileges.
*
*  INPUTS
*     const char *anUser - user which did request this operation 
*
*  RESULT
*     EPERM - operation not permitted 
*     0     - otherwise
*
*  NOTES
*     MT-NOTES: sge_shutdown_dynamic_event_clients() is NOT MT safe. 
*
*******************************************************************************/
int sge_shutdown_dynamic_event_clients(const char *anUser)
{
   lListElem *client;

   DENTER(TOP_LAYER, "sge_shutdown_dynamic_event_clients");


   if (!manop_is_manager(anUser)) {
      ERROR((SGE_EVENT, MSG_COM_NOSHUTDOWNPERMS));
      DEXIT;
      return EPERM;
   }

   for_each (client, Master_Control.clients) {
      u_long32 id = lGetUlong(client, EV_id);

      if (id < EV_ID_FIRST_DYNAMIC) {
         continue;
      }
      
      add_event(client, 0, sgeE_SHUTDOWN, 0, 0, NULL, NULL, NULL);
      
      INFO((SGE_EVENT, MSG_COM_SHUTDOWNNOTIFICATION_SUS, lGetString(client, EV_name), 
      id, lGetHost(client, EV_host)));

/*      sge_add_event_for_client(id, 0, sgeE_SHUTDOWN, 0, 0, NULL, NULL, NULL, NULL); */

  }

   DEXIT;
   return 0;
} /* sge_shutdown_dynamic_event_clients() */

/****** evm/sge_event_master/sge_get_event_client_data() ***************************
*  NAME
*     sge_get_event_client_data() -- get event client data 
*
*  SYNOPSIS
*     u_long32 sge_get_event_client_data(u_long32 aClientID) 
*
*  FUNCTION
*     Get event client data. 
*
*  INPUTS
*     u_long32 aClientID - event client id 
*
*  RESULT
*     u_long32 - event client data
*
*  NOTES
*     MT-NOTE: sge_get_event_client_data() is NOT MT safe. 
*
*******************************************************************************/
u_long32 sge_get_event_client_data(u_long32 aClientID)
{
   u_long32 res = 0;
   lListElem *client = NULL;

   DENTER(TOP_LAYER, "sge_get_event_client_data");

   pthread_once(&Event_Master_Once, event_master_once_init);

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL)
   {
      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));

      DEXIT;
      return 0;
   }

   res = lGetUlong(client, EV_clientdata); /* will fail if client == NULL */

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* sge_get_event_client_data() */

/****** evm/sge_event_master/sge_set_event_client_data() ***********************
*  NAME
*     sge_set_event_client_data() -- set event client data 
*
*  SYNOPSIS
*     int sge_set_event_client_data(u_long32 aClientID, u_long32 theData) 
*
*  FUNCTION
*     Set event client data. 
*
*  INPUTS
*     u_long32 aClientID - event client id 
*     u_long32 theData   - data 
*
*  RESULT
*     EACCES - failed to set data
*     0      - success
*
*  NOTES
*     MT-NOTE: sge_set_event_client_data() is NOT MT safe. 
*
*******************************************************************************/
int sge_set_event_client_data(u_long32 aClientID, u_long32 theData)
{
   int res = EACCES;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "sge_set_event_client_data");

   pthread_once(&Event_Master_Once, event_master_once_init);

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((ep = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL)
   {
      sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));

      DEXIT;
      return EACCES;
   }

   res = (lSetUlong(ep, EV_clientdata, theData) == 0) ? 0 : EACCES;

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* sge_set_event_client_data() */

/****** Eventclient/Server/sge_add_event() *************************************
*  NAME
*     sge_add_event() -- add an object as event
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     void 
*     sge_add_event( u_long32 timestamp, ev_event type,
*                   u_long32 intkey, u_long32 intkey2, const char *strkey, 
*                   const char *strkey2, lListElem *element) 
*
*  FUNCTION
*     Adds an object to the list of events to deliver. Called, if an event 
*     occurs to that object, e.g. it was added to Grid Engine, modified or 
*     deleted.
*  
*     Internally, a list with that single object is created and passed to 
*     sge_add_list_event().
*
*  INPUTS
*     u_long32 timestamp      - time stamp in gmt for the even; if 0 is passed,
*                               sge_add_list_event will insert the actual time
*     ev_event type           - the event id
*     u_long32 intkey         - additional data
*     u_long32 intkey2        - additional data
*     const char *strkey      - additional data
*     const char *strkey2     - additional data
*     const char *session     - events session key
*     lListElem *element      - the object to deliver as event
*
*  NOTES
*     MT-NOTE: sge_add_event() is NOT MT safe.
*
*******************************************************************************/
void sge_add_event(u_long32 timestamp, ev_event type, u_long32 intkey,
   u_long32 intkey2, const char *strkey, const char *strkey2, const char *session, lListElem *element) 
{
   lListElem *event_client = NULL;
   DENTER(TOP_LAYER, "sge_add_event"); 

   if (timestamp == 0) {
      timestamp = sge_get_gmt();
   }

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   for_each (event_client, Master_Control.clients)
   {
      if (eventclient_subscribed(event_client, type, session))
      {
         add_event(event_client, timestamp, type, intkey, intkey2, strkey, strkey2, element);

         DPRINTF(("%s: added event for %s with id " u32 "\n", SGE_FUNC, lGetString(event_client, EV_name), lGetUlong(event_client, EV_id)));
      }
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return;
} /* sge_add_event() */

/****** sge_event_master/sge_add_event_for_client() ****************************
*  NAME
*     sge_add_event_for_client() -- add an event for a given object
*
*  SYNOPSIS
*     int sge_add_event_for_client(u_long32 aClientID, u_long32 aTimestamp, 
*     ev_event anID, u_long32 anIntKey1, u_long32 anIntKey2, const char 
*     *aStrKey1, const char *aStrKey2, const char *aSeesion, lListElem 
*     *anObject) 
*
*  FUNCTION
*     Add an event for a given event client.
*
*  INPUTS
*     u_long32 aClientID   - event client id 
*     u_long32 aTimestamp  - event delivery time, 0 -> deliver now 
*     ev_event anID        - event id 
*     u_long32 anIntKey1   - 1st numeric key 
*     u_long32 anIntKey2   - 2nd numeric key 
*     const char *aStrKey1 - 1st alphanumeric key 
*     const char *aStrKey2 - 2nd alphanumeric key 
*     const char *aSession - event session 
*     lListElem *anObject  - object to be delivered with the event 
*
*  RESULT
*     EACCES - failed to add event
*     0      - success
*
*  NOTES
*     MT-NOTE: sge_add_event_for_client() is not MT safe 
*
*******************************************************************************/
int sge_add_event_for_client(u_long32 aClientID, u_long32 aTimestamp, ev_event anID, u_long32 anIntKey1, 
                             u_long32 anIntKey2, const char *aStrKey1, const char *aStrKey2, 
                             const char *aSession, lListElem *anObject)
{
   int res = EACCES;
   lListElem *client = NULL;

   DENTER(TOP_LAYER, "sge_add_event_for_client");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL) {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));
   }
   else {
      if (0 == aTimestamp) {
         aTimestamp = time(NULL);
      }

      if (eventclient_subscribed(client, anID, aSession))
      {
         add_event(client, aTimestamp, anID, anIntKey1, anIntKey2, aStrKey1, aStrKey2, anObject);
         res = 0;
      }
   }
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* sge_add_event_for_client() */

/****** Eventclient/Server/sge_add_list_event() ********************************
*  NAME
*     sge_add_list_event() -- add a list as event
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     void 
*     sge_add_list_event( u_long32 timestamp,
*                        ev_event type, 
*                        u_long32 intkey, u_long32 intkey2, const char *strkey,
*                        const char *session, lList *list) 
*
*  FUNCTION
*     Adds a list of objects to the list of events to deliver, e.g. the 
*     sgeE*_LIST events.
*
*  INPUTS
*     u_long32 timestamp      - time stamp in gmt for the even; if 0 is passed,
*                               sge_add_list_event will insert the actual time
*     ev_event type           - the event id
*     u_long32 intkey         - additional data
*     u_long32 intkey2        - additional data
*     const char *strkey      - additional data
*     const char *session     - events session key
*     lList *list             - the list to deliver as event
*
*  NOTES
*     MT-NOTE: sge_add_list_event() is MT safe.
*
*******************************************************************************/
void sge_add_list_event( u_long32 timestamp, ev_event type, 
                   u_long32 intkey, u_long32 intkey2, const char *strkey, 
                   const char *strkey2, const char *session, lList *list) 
{
   lListElem *event_client = NULL;
   DENTER(TOP_LAYER, "sge_add_list_event");

   if (timestamp == 0) {
      timestamp = sge_get_gmt();
   }

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   for_each (event_client, Master_Control.clients)
   {
      if (eventclient_subscribed(event_client, type, session)) {
         add_list_event(event_client, timestamp, type, intkey, intkey2, strkey, strkey2, list, true);
      }
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return;
} /* sge_add_list_event() */

/****** Eventclient/Server/sge_handle_event_ack() ******************************
*  NAME
*     sge_handle_event_ack() -- acknowledge event delivery
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     void 
*     sge_handle_event_ack(u_long32 aClientID, ev_event anEvent) 
*
*  FUNCTION
*     After the server sent events to an event client, it has to acknowledge
*     their receipt. 
*     Acknowledged events are deleted from the list of events to deliver, 
*     otherwise they will be resent after the next event delivery interval.
*     If the handling of a busy state of the event client is enabled and set to 
*     EV_BUSY_UNTIL_ACK, the event client will be set to "not busy".
*
*  INPUTS
*     u_long32 aClientID - event client sending acknowledge
*     ev_event anEvent   - serial number of the last event to acknowledge
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_handle_event_ack() is MT safe.
*
*  SEE ALSO
*     Eventclient/Server/sge_get_next_event_number()
*
*******************************************************************************/
int sge_handle_event_ack(u_long32 aClientID, ev_event anEvent)
{
   int res = 0;
   lList *list = NULL;
   lListElem *client = NULL;

   DENTER(TOP_LAYER, "sge_handle_event_ack");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((Master_Control.clients != NULL) &&
      (client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL) {

      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));
   }
   else {
      list = lGetList(client, EV_events);

      res = purge_event_list(list, anEvent);

      if (lGetNumberOfElem(list) == 0) {
      
         sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);
         Master_Control.clients_deliver_count--;
         if (Master_Control.clients_deliver_count < 0)
            Master_Control.clients_deliver_count = 0;
         sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);
         
      }

      DPRINTF(("%s: purged %d acknowleded events\n", SGE_FUNC, res));

      lSetUlong(client, EV_last_heard_from, sge_get_gmt()); /* note time of ack */

      switch (lGetUlong(client, EV_busy_handling))
      {
      case EV_BUSY_UNTIL_ACK:
      case EV_THROTTLE_FLUSH:
         lSetUlong(client, EV_busy, 0); /* clear busy state */
         break;
      default:
         break;
      }
   }
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* sge_handle_event_ack() */

/****** evm/sge_event_master/sge_deliver_events_immediately() ******************
*  NAME
*     sge_deliver_events_immediately() -- deliver events immediately 
*
*  SYNOPSIS
*     void sge_deliver_events_immediately(u_long32 aClientID) 
*
*  FUNCTION
*     Deliver all events for the event client denoted by 'aClientID'
*     immediately.
*
*  INPUTS
*     u_long32 aClientID - event client id 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_deliver_events_immediately() is NOT MT safe. 
*
*******************************************************************************/
void sge_deliver_events_immediately(u_long32 aClientID)
{
   lListElem *client = NULL;

   DENTER(TOP_LAYER, "sge_event_immediate_delivery");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((Master_Control.clients != NULL) &&
      (client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) == NULL) {

      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));

   }
   else {
      flush_events(client, 0);
      pthread_cond_signal(&Master_Control.cond_var);
   }   

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return;
} /* sge_deliver_event_immediately() */

/****** Eventclient/Server/sge_get_next_event_number() *************************
*  NAME
*     sge_get_next_event_number() -- next event number for an event client
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     u_long32 
*     sge_get_next_event_number(u_long32 client_id) 
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
*  MT-NOTE: sge_get_next_event_number() is NOT MT safe.
*
*  BUGBUG-AD: Change signature of this function to allow for better error 
*  BUGBUG-AD: handling!
*
*******************************************************************************/
u_long32 sge_get_next_event_number(u_long32 aClientID) 
{
   lListElem *client;
   u_long32 ret = EACCES;

   DENTER(TOP_LAYER, "sge_get_next_event_number");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((Master_Control.clients != NULL) && 
      (client = lGetElemUlong(Master_Control.clients, EV_id, aClientID)) != NULL) {
      ret = lGetUlong(client, EV_next_number);
   }
   else {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(aClientID), SGE_FUNC));
   }

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return ret;
} /* sge_get_next_event_number() */

/****** evm/sge_event_master/sge_resync_schedd() *******************************
*  NAME
*     sge_resync_schedd() -- resync schedd 
*
*  SYNOPSIS
*     int sge_resync_schedd(void) 
*
*  FUNCTION
*     Does a total update (send all lists) to schedd and outputs an error
*     message.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     0 - resync successful
*    -1 - otherwise
*
*  NOTES
*     MT-NOTE: sge_resync_schedd() in NOT MT safe. 
*
*******************************************************************************/
int sge_resync_schedd(void)
{
   lListElem *client;
   int ret = -1;
   DENTER(TOP_LAYER, "sge_sync_schedd");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   if ((Master_Control.clients != NULL) &&
      (client = lGetElemUlong(Master_Control.clients, EV_id, EV_ID_SCHEDD)) != NULL) {

      ERROR((SGE_EVENT, MSG_EVE_REINITEVENTCLIENT_S,lGetString(client, EV_name)));
      
      total_update(client);
      
      ret = 0;
   }
   else {
      ERROR((SGE_EVENT, MSG_EVE_UNKNOWNEVCLIENT_US, u32c(EV_ID_SCHEDD), SGE_FUNC));
      ret = -1;
   }
   
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return ret;
} /* sge_resync_schedd() */

/****** evm/sge_event_master/sge_event_shutdown() ******************************
*  NAME
*     sge_event_shutdown() -- shutdown event delivery 
*
*  SYNOPSIS
*     void sge_event_shutdown(void) 
*
*  FUNCTION
*     Shutdown event delivery. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: sge_event_shutdown() is NOT MT safe. 
*
*******************************************************************************/
void sge_event_shutdown(void)
{
   DENTER(TOP_LAYER, "sge_event_shutdown");

   pthread_once(&Event_Master_Once, event_master_once_init);

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   Master_Control.exit = true;

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DPRINTF(("%s: wait for send thread termination\n", SGE_FUNC));

   pthread_join(Event_Thread, NULL);

   DEXIT;
   return;
} /* sge_event_shutdown() */

/****** evm/sge_event_master/event_master_once_init() **************************
*  NAME
*     event_master_once_init() -- one-time event master initialization
*
*  SYNOPSIS
*     static void event_master_once_init(void) 
*
*  FUNCTION
*     Initialize the event master control structure. Initialize permanent
*     event array. Create and kick off event send thread.
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none
*
*  NOTES
*     MT-NOTE: event_master_once_init() is MT safe 
*     MT-NOTE:
*     MT-NOTE: This function must only be used as a one-time initialization
*     MT-NOTE: function in conjunction with 'pthread_once()'.
*
*******************************************************************************/
static void event_master_once_init(void)
{
   pthread_attr_t attr;

   DENTER(TOP_LAYER, "event_master_once_init");

   Master_Control.clients = lCreateList("EV_Clients", EV_Type);

   init_send_events();

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&Event_Thread, &attr, send_thread, NULL);

   DEXIT;
   return;
} /* event_master_one_init() */

/****** evm/sge_event_master/init_send_events() ********************************
*  NAME
*     init_send_events() -- sets the events, that should allways be delivered 
*
*  SYNOPSIS
*     void init_send_events() 
*
*  FUNCTION
*     sets the events, that should allways be delivered 
*
*  NOTES
*     MT-NOTE: init_send_events() is not MT safe 
*
*******************************************************************************/
static void init_send_events(void)
{
   DENTER(TOP_LAYER, "init_send_events");

   memset(SEND_EVENTS, false, sizeof(bool) * sgeE_EVENTSIZE);

   SEND_EVENTS[sgeE_ADMINHOST_LIST] = true;
   SEND_EVENTS[sgeE_CALENDAR_LIST] = true;
   SEND_EVENTS[sgeE_CKPT_LIST] = true;
   SEND_EVENTS[sgeE_CENTRY_LIST] = true;
   SEND_EVENTS[sgeE_CONFIG_LIST] = true;
   SEND_EVENTS[sgeE_EXECHOST_LIST] = true;
   SEND_EVENTS[sgeE_JOB_LIST] = true;
   SEND_EVENTS[sgeE_JOB_SCHEDD_INFO_LIST] = true;
   SEND_EVENTS[sgeE_MANAGER_LIST] = true;
   SEND_EVENTS[sgeE_OPERATOR_LIST] = true;
   SEND_EVENTS[sgeE_PE_LIST] = true;
   SEND_EVENTS[sgeE_PROJECT_LIST] = true;
   SEND_EVENTS[sgeE_QMASTER_GOES_DOWN] = true;
   SEND_EVENTS[sgeE_CQUEUE_LIST] = true;
   SEND_EVENTS[sgeE_SUBMITHOST_LIST] = true;
   SEND_EVENTS[sgeE_USER_LIST] = true;
   SEND_EVENTS[sgeE_USERSET_LIST] = true;
   SEND_EVENTS[sgeE_HGROUP_LIST] = true;
#ifndef __SGE_NO_USERMAPPING__      
   SEND_EVENTS[sgeE_CUSER_LIST] = true;
#endif      

   DEXIT;
   return;
} /* init_send_events() */

/****** evm/sge_event_master/send_thread() *************************************
*  NAME
*     send_thread() -- send events due 
*
*  SYNOPSIS
*     static void* send_thread(void *anArg) 
*
*  FUNCTION
*     Event send thread. Do common thread initialization. Send events until
*     shutdown is requestet. 
*
*  INPUTS
*     void *anArg - none 
*
*  RESULT
*     void* - none 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: send_thread() is a thread function. Do NOT use this function
*     MT-NOTE: in any other way!
*
*******************************************************************************/
static void* send_thread(void *anArg)
{
   DENTER(TOP_LAYER, "send_thread");

   sge_qmaster_thread_init();

   while (!should_exit())
   {
      send_events();
   }

   DEXIT;
   return NULL;
} /* send_thread() */

/****** evm/sge_event_master/should_exit() *************************************
*  NAME
*     should_exit() -- should thread exit? 
*
*  SYNOPSIS
*     static bool should_exit(void) 
*
*  FUNCTION
*     Determine if send thread should exit. Does return value of the Master
*     Control struct 'exit' flag. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     true   - exit
*     false  - continue 
*
*  NOTES
*     MT-NOTE: should_exit() is MT safe 
*
*******************************************************************************/
static bool should_exit(void)
{
   bool res = false;

   DENTER(TOP_LAYER, "should_exit");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);
   res = Master_Control.exit;
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return res;
} /* should_exit() */

/****** evm/sge_event_master/send_events() *************************************
*  NAME
*     send_events() -- send events to event clients 
*
*  SYNOPSIS
*     static void send_events(void) 
*
*  FUNCTION
*     Loop over all event clients and send events due. If an event client did
*     time out, it will be removed from the list of registered event clients.
*
*     Events will be delivered only, if the so called 'busy handling' of a 
*     client does allow it. Events will be delivered as a report (REP_Type)
*     with a report list of type ET_Type. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: send_events() is MT safe 
*     MT-NOTE:
*     MT-NOTE: After all events for all clients have been sent. This function
*     MT-NOTE: will wait on the condition variable 'Master_Control.cond_var'
*
*******************************************************************************/
static void send_events(void)
{
   lListElem *report;
   lList *report_list;
   u_long32 timeout, busy_handling;
   lListElem *event_client, *tmp;
   const char *host;
   const char *commproc;
   int ret, id; 
   int deliver_interval;
   time_t now = time(NULL);
   struct timespec ts;

   DENTER(TOP_LAYER, "send_events");

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);
   
   do{ 
      ts.tv_sec = sge_get_gmt() + EVENT_DELIVERY_INTERVAL_S;
      ts.tv_nsec = EVENT_DELIVERY_INTERVAL_N;
      pthread_cond_timedwait(&Master_Control.cond_var, &Master_Control.cond_mutex, &ts);
   } while(Master_Control.clients_deliver_count == 0); 

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   event_client = lFirst(Master_Control.clients);
         
   while (event_client)
   {
      /* extract address of event client */
      host = lGetHost(event_client, EV_host);
      commproc = lGetString(event_client, EV_commproc);
      id = lGetUlong(event_client, EV_commid);

      deliver_interval = lGetUlong(event_client, EV_d_time);
      busy_handling = lGetUlong(event_client, EV_busy_handling);

      /* somone turned the clock back */
      if (lGetUlong(event_client, EV_last_heard_from) > now) {
         lSetUlong(event_client, EV_last_heard_from, now);
         lSetUlong(event_client, EV_next_send_time, now + deliver_interval);
      }
      else if (lGetUlong(event_client, EV_last_send_time)  > now) {
         lSetUlong(event_client, EV_last_send_time, now);
      }
  
      /* if set, use qmaster_params SCHEDULER_TIMEOUT */
      if (scheduler_timeout > 0) {
          timeout = scheduler_timeout;
      }
      else {
         /* is the ack timeout expired ? */
         timeout = 10*deliver_interval;
         
         if (timeout < EVENT_ACK_MIN_TIMEOUT) {
            timeout = EVENT_ACK_MIN_TIMEOUT;
         }
         else if (timeout > EVENT_ACK_MAX_TIMEOUT) {
            timeout = EVENT_ACK_MAX_TIMEOUT;
         }
      }

      if (now > (lGetUlong(event_client, EV_last_heard_from) + timeout)) {
         ERROR((SGE_EVENT, MSG_COM_ACKTIMEOUT4EV_ISIS, 
               (int) timeout, commproc, (int) id, host));
         tmp = event_client;
         event_client = lNext(event_client);
         lRemoveElem(Master_Control.clients, tmp); 
         continue;
      }

      /* do we have to deliver events ? */
      if ((now >= lGetUlong(event_client, EV_next_send_time)) 
           && (busy_handling == EV_THROTTLE_FLUSH 
               || !lGetUlong(event_client, EV_busy))
         ) {
         lList *lp;
         int numevents;  

         /* put only pointer in report - dont copy */
         report_list = lCreateList("report list", REP_Type);
         report = lCreateElem(REP_Type);
         lSetUlong(report, REP_type, NUM_REP_REPORT_EVENTS);
         lSetHost(report, REP_host, uti_state_get_qualified_hostname());
         lSetList(report, REP_list, lGetList(event_client, EV_events));
         lAppendElem(report_list, report);

         lp = lGetList(event_client, EV_events);
         numevents = lGetNumberOfElem(lp);
            
         ret = report_list_send(report_list, host, commproc, id, 0, NULL);

         /* on failure retry is triggered automatically */
#ifdef ENABLE_NGC
         if (ret == CL_RETVAL_OK) {
#else
         if (ret == 0) {
#endif
            switch (busy_handling) {
            case EV_THROTTLE_FLUSH:
               /* increase busy counter */
               lSetUlong(event_client, EV_busy, lGetUlong(event_client, EV_busy)+1); 
               break;
            case EV_BUSY_UNTIL_RELEASED:
            case EV_BUSY_UNTIL_ACK:
               lSetUlong(event_client, EV_busy, 1);
               break;
            default: 
               /* EV_BUSY_NO_HANDLING */
               break;
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

   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
   return;
} /* send_events() */
 
static void flush_events(lListElem *event_client, int interval)
{
   u_long32 next_send, flush_delay;
   int now = time(NULL);
   DENTER(TOP_LAYER, "flush_events");

   SGE_ASSERT(NULL != event_client);

   next_send = lGetUlong(event_client, EV_next_send_time);
   next_send = MIN(next_send, now + interval);

   /* never send out two event packages in the very same second */
   flush_delay = 1;

   if (lGetUlong(event_client, EV_busy_handling) == EV_THROTTLE_FLUSH) {
      u_long32 busy_counter = lGetUlong(event_client, EV_busy);
      u_long32 ed_time = lGetUlong(event_client, EV_d_time);
      u_long32 flush_delay_rate = MAX(lGetUlong(event_client, EV_flush_delay), 1);
      if (busy_counter >= flush_delay_rate) {
         /* busy counters larger than flush delay cause events being 
            sent out in regular event delivery interval for alive protocol 
            purposes with event client */
         flush_delay = MAX(flush_delay, ed_time);
      } else {
         /* for smaller busy counters event delivery interval is scaled 
            down with the busy counter */
         flush_delay = MAX(flush_delay, ed_time * busy_counter / flush_delay_rate);
      }
   }

   next_send = MAX(next_send, lGetUlong(event_client, EV_last_send_time) + flush_delay);

   lSetUlong(event_client, EV_next_send_time, next_send);
   
   DPRINTF(("%s: %s %d (ID:"u32")\tNOW: %d NEXT FLUSH: %d (%s,%s,%d)\n", 
         SGE_FUNC, lGetString(event_client, EV_name), lGetUlong(event_client, EV_id), 
         lGetUlong(lFirst(lGetList(event_client, EV_events)), ET_number),
         now, next_send, 
         lGetHost(event_client, EV_host), 
         lGetString(event_client, EV_commproc),
         lGetUlong(event_client, EV_commid))); 

   DEXIT;
   return;
} /* flush_events() */

/****** Eventclient/Server/total_update() **************************************
*  NAME
*     total_update() -- send all data to eventclient
*
*  SYNOPSIS
*     static void 
*     total_update(lListElem *event_client) 
*
*  FUNCTION
*     Sends all complete lists it subscribed to an eventclient.
*     If the event client receives a complete list instead of single events,
*     it should completely update it's database.
*
*  INPUTS
*     lListElem *event_client - the event client to update
*
*  NOTES
*     MT-NOTE: total_update() is MT safe, IF the function is invoked with
*     MT-NOTE: 'LOCK_EVENT_CLIENT_LST' locked! This is in accordance with
*     MT-NOTE: the acquire/release protocol as defined by the Grid Engine
*     MT-NOTE: Locking API.
*
*  SEE ALSO
*     libs/lck/sge_lock.h
*     libs/lck/sge_lock.c
*
*******************************************************************************/
static void 
total_update(lListElem *event_client)
{
   DENTER(TOP_LAYER, "total_update");

   total_update_event(event_client, sgeE_ADMINHOST_LIST);
   total_update_event(event_client, sgeE_CALENDAR_LIST);
   total_update_event(event_client, sgeE_CKPT_LIST);
   total_update_event(event_client, sgeE_CENTRY_LIST);
   total_update_event(event_client, sgeE_CONFIG_LIST);
   total_update_event(event_client, sgeE_EXECHOST_LIST);
   total_update_event(event_client, sgeE_JOB_LIST);
   total_update_event(event_client, sgeE_JOB_SCHEDD_INFO_LIST);
   total_update_event(event_client, sgeE_MANAGER_LIST);
   total_update_event(event_client, sgeE_OPERATOR_LIST);
   total_update_event(event_client, sgeE_PE_LIST);
   total_update_event(event_client, sgeE_CQUEUE_LIST);
   total_update_event(event_client, sgeE_SCHED_CONF);
   total_update_event(event_client, sgeE_SUBMITHOST_LIST);
   total_update_event(event_client, sgeE_USERSET_LIST);

   total_update_event(event_client, sgeE_NEW_SHARETREE);
   total_update_event(event_client, sgeE_PROJECT_LIST);
   total_update_event(event_client, sgeE_USER_LIST);

   total_update_event(event_client, sgeE_HGROUP_LIST);

#ifndef __SGE_NO_USERMAPPING__
   total_update_event(event_client, sgeE_CUSER_LIST);
#endif

   DEXIT;
   return;
} /* total_update() */


/****** evm/sge_event_master/build_subscription() ******************************
*  NAME
*     build_subscription() -- generates an array out of the cull registration
*                                 structure
*
*  SYNOPSIS
*     static void build_subscription(lListElem *event_el) 
*
*  FUNCTION
*      generates an array out of the cull registration
*      structure. The array contains all event elements and each of them 
*      has an identifier, if it is subscribed or not. Before that is done, it is
*      tested, the EV_changed flag is set. If not, the function simply returns.
*
*
*  INPUTS
*     lListElem *event_el - the event element, which event structure will be transformed 
*
*******************************************************************************/
static void build_subscription(lListElem *event_el)
{
   lList *subscription = lGetList(event_el, EV_subscribed);
   lListElem *sub_el = NULL;
   subscription_t *sub_array = NULL; 

   DENTER(TOP_LAYER, "build_subscription");


   if (!lGetBool(event_el, EV_changed)) {
      DEXIT;
      return;
   }

   DPRINTF(("rebuild event mask for a client\n"));

   sub_array = (subscription_t *) malloc(sizeof(subscription_t) * sgeE_EVENTSIZE);
   
   memset(sub_array, 0, sizeof(subscription_t) * sgeE_EVENTSIZE); 
   {
      int i;
      for (i=0; i<sgeE_EVENTSIZE; i++){
         sub_array[i].subscription = EV_NOT_SUBSCRIBED;
      }
   } 
   for_each(sub_el, subscription){
      const lListElem *temp = NULL;
      u_long32 event = lGetUlong(sub_el, EVS_id);   
      
      sub_array[event].subscription = EV_SUBSCRIBED; 
      sub_array[event].flush = lGetBool(sub_el, EVS_flush);
      sub_array[event].flush_time = lGetUlong(sub_el, EVS_interval);
     
      if ((temp = lGetObject(sub_el, EVS_where)))
         sub_array[event].where = lWhereFromElem(temp);

      if ((temp = lGetObject(sub_el, EVS_what))) {      
         sub_array[event].what = lWhatFromElem(temp);
      }   
      
   }
   {
      subscription_t *old_sub_array = lGetRef(event_el, EV_sub_array);
      if (old_sub_array) {
         int i;
         for (i=0; i<sgeE_EVENTSIZE; i++){ 
            if (old_sub_array[i].where)
               lFreeWhere(old_sub_array[i].where);
            if (old_sub_array[i].what)
               lFreeWhat(old_sub_array[i].what);
            if (old_sub_array[i].descr){
               cull_hash_free_descr(old_sub_array[i].descr);
               free(old_sub_array[i].descr);
            }
         }
         free(old_sub_array);
      }
      lSetRef(event_el, EV_sub_array, sub_array);
      lSetBool(event_el, EV_changed, false);
   }
   DEXIT;
} /* build_subscription() */

/****** Eventclient/Server/check_send_new_subscribed_list() ********************
*  NAME
*     check_send_new_subscribed_list() -- check suscription for new list events
*
*  SYNOPSIS
*     static void 
*     check_send_new_subscribed_list(const subscription_t *old_subscription, 
*                                    const subscription_t *new_subscription, 
*                                    lListElem *event_client, 
*                                    ev_event event) 
*
*  FUNCTION
*     Checks, if sgeE*_LIST events have been added to the subscription of a
*     certain event client. If yes, send these lists to the event client.
*
*  INPUTS
*     const subscription_t *old_subscription - former subscription
*     const subscription_t *new_subscription - new subscription
*     lListElem *event_client      - the event client object
*     ev_event event               - the event to check
*
*  SEE ALSO
*     Eventclient/Server/total_update_event()
*
*******************************************************************************/
static void 
check_send_new_subscribed_list(const subscription_t *old_subscription, 
                               const subscription_t *new_subscription, 
                               lListElem *event_client, ev_event event)
{
   if ((new_subscription[event].subscription & EV_SUBSCRIBED) && 
       (old_subscription[event].subscription == EV_NOT_SUBSCRIBED)) {
      total_update_event(event_client, event);
   }   
}

/****** Eventclient/Server/eventclient_subscribed() ************************
*  NAME
*     eventclient_subscribed() -- has event client subscribed an event?
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     int 
*     eventclient_subscribed(const lListElem *event_client, ev_event event) 
*
*  FUNCTION
*     Checks if the given event client has a certain event subscribed.
*     For event clients that use session filtering additional conditions
*     must be fulfilled otherwise the event counts not as subscribed.
*
*  INPUTS
*     const lListElem *event_client - event client to check
*     ev_event event                - event to check
*     const char *session           - session key of this event
*
*  RESULT
*     int - 0 = not subscribed, 1 = subscribed
*
*  SEE ALSO
*     Eventclient/-Session filtering
*******************************************************************************/
static int 
eventclient_subscribed(const lListElem *event_client, ev_event event, 
   const char *session)
{
   const subscription_t *subscription = NULL;
   const char *ec_session;

   DENTER(TOP_LAYER, "eventclient_subscribed");

   if (event_client == NULL) {
      DEXIT;
      return 0;
   }

   subscription = lGetRef(event_client, EV_sub_array);
   ec_session = lGetString(event_client, EV_session);

   if (subscription == NULL) {
      DEXIT;
      return 0;
   }

   if (ec_session) {
      if (session) {
         /* events that belong to a specific session are not subscribed 
            in case the event client is not interested in that session */
         if (strcmp(session, ec_session)) {
            DEXIT;
            return 0;
         }
      } else {
         /* events that do not belong to a specific session are not 
            subscribed if the event client is interested in events of a 
            specific session. 
            The only exception are list events, because list events do not 
            belong to a specific session. These events require filtering on 
            a more fine grained level */
         if (!IS_TOTAL_UPDATE_EVENT(event)) {
            DEXIT;
            return 0;
         }
      }
   }
   if (subscription[event].subscription == EV_SUBSCRIBED) {
      DEXIT;
      return 1;
   }

   DEXIT;
   return 0;
}

/****** evm/sge_event_master/purge_event_list() ********************************
*  NAME
*     purge_event_list() -- purge event list
*
*  SYNOPSIS
*     static int purge_event_list(lList* aList, ev_event anEvent) 
*
*  FUNCTION
*     Remove all events from 'aList' which do have an event id less than or
*     equal to 'anEvent'.
*
*  INPUTS
*     lList* aList     - event list
*     ev_event anEvent - event
*
*  RESULT
*     int - number of events purged.
*
*  NOTES
*     MT-NOTE: purge_event_list() is NOT MT safe. 
*     MT-NOTE: 
*     MT-NOTE: Do not call this function without having 'aList' locked!
*
*  BUGS
*     BUGBUG-AD: If 'anEvent' == 0, not events will be purged. However zero is
*     BUGBUG-AD: also the id of 'sgeE_ALL_EVENTS'. Is this behaviour correct?
*
*******************************************************************************/
static int purge_event_list(lList* aList, ev_event anEvent) 
{
   int purged = 0, pos = 0;
   lListElem *ev = NULL;

   DENTER(TOP_LAYER, "purge_event_list");

   if (0 == anEvent) {
      DEXIT;
      return 0;
   }

   pos = lGetPosInDescr(ET_Type, ET_number);

   ev = lFirst(aList);

   while (ev)
   {
      lListElem *tmp = ev;

      ev = lNext(ev); /* fetch next event, before the old one will be deleted */

      if (lGetPosUlong(tmp, pos) > anEvent) {
         break;
      }

      lRemoveElem(aList, tmp);
      purged++;
   }

   DEXIT;
   return purged;
} /* remove_events_from_client() */

static int 
add_list_event(lListElem *event_client, u_long32 timestamp, ev_event type,
                    u_long32 intkey, u_long32 intkey2, const char *strkey, 
                    const char *strkey2, lList *list, int need_copy_list) 
{
   lListElem *event = NULL;
   u_long32 i;
   lList *lp;
   int consumed = 0;
   char buffer[1024];
   dstring buffer_wrapper;

   DENTER(TOP_LAYER, "add_list_event"); 


   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));

   { 
      lList *cp_list;
      if (list && need_copy_list) {
         subscription_t *subscription = lGetRef(event_client, EV_sub_array);
         const lCondition *selection = subscription[type].where;
         const lEnumeration *fields = subscription[type].what;
         const lDescr *descr = getDescriptorL(subscription, list, type);

         DPRINTF(("deliver event: %d with where filter=%d and what filter=%d\n", 
         type, (selection!=NULL), (fields!=NULL)));

         if (fields) {
            if (!list_select(subscription, type, &cp_list, list, selection, fields, descr)){
               cp_list = lSelectD("updating list", list, selection,descr, fields); 
            }   
            
            /* no elements in the event list, no need for an event */
            if (!SEND_EVENTS[ET_type] && lGetNumberOfElem(cp_list) == 0){
               if (cp_list != NULL)
                  lFreeList(cp_list);
               return 0;
            }
         }
         else {
            cp_list = lCopyList(lGetListName(list), list);
         }
         consumed = 0;
      }   
      else {
         cp_list = list;
         consumed = 1;
      }   

      event = lCreateElem(ET_Type); 
      /* 
         fill in event number and increment 
         EV_next_number of event recipient 
      */
      i = lGetUlong(event_client, EV_next_number);
      lSetUlong(event_client, EV_next_number, (i + 1));

      lSetUlong(event, ET_number, i);
      lSetUlong(event, ET_timestamp, timestamp);
      lSetUlong(event, ET_type, type); 
      lSetUlong(event, ET_intkey, intkey); 
      lSetUlong(event, ET_intkey2, intkey2); 
      lSetString(event, ET_strkey, strkey);
      lSetString(event, ET_strkey2, strkey2);
      lSetList(event, ET_new_version, cp_list );
   }     

   /* build a new event list if not exists */
   lp = lGetList(event_client, EV_events); 
   if (!lp) {
      lp=lCreateList("", ET_Type);
      lSetList(event_client, EV_events, lp);
   }

   sge_mutex_lock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);
   if (lGetNumberOfElem(lp) == 0 || Master_Control.clients_deliver_count == 0) {
      Master_Control.clients_deliver_count++;
   }
   sge_mutex_unlock("event_master_mutex", SGE_FUNC, __LINE__, &Master_Control.cond_mutex);

   /* chain in new event */
   lAppendElem(lp, event);

   DPRINTF(("%d %s\n", lGetUlong(event_client, EV_id), 
            event_text(event, &buffer_wrapper)));

   /* check if event clients wants flushing */
   {
      const subscription_t *subscription = lGetRef(event_client, EV_sub_array);

      if (type == sgeE_QMASTER_GOES_DOWN) {
         lSetUlong(event_client, EV_busy, 0); /* can't be too busy for shutdown */
         flush_events(event_client, 0);
         pthread_cond_signal(&Master_Control.cond_var);
      }
      else if (type == sgeE_SHUTDOWN) {
         flush_events(event_client, 0);
         pthread_cond_signal(&Master_Control.cond_var);         
      }
      else if (subscription[type].flush) {
         DPRINTF(("flushing event client\n"));
         flush_events(event_client, subscription[type].flush_time);
         if ( subscription[type].flush_time == 0) {
            pthread_cond_signal(&Master_Control.cond_var);         
         }   
      }

   }
   DEXIT;
   return Master_Control.clients?consumed:1;
}

static void 
add_event(lListElem *event_client, u_long32 timestamp, ev_event type, 
               u_long32 intkey, u_long32 intkey2, const char *strkey, 
               const char *strkey2, lListElem *element) 
{
   lList *lp = NULL;
   bool sub_filter = false;
   int entry_counter;
   int event_counter;
   for (entry_counter = 0; entry_counter < LIST_MAX; entry_counter++) {
      event_counter = -1;
      while (EVENT_LIST[entry_counter][++event_counter] != -1){
         sub_filter = (type == EVENT_LIST[entry_counter][event_counter]); 
         if (sub_filter)
            break;
      }
      if (sub_filter)
         break;
   }

   /* build a list from the element */
   if (element) {
      lListElem *el = NULL; 
      subscription_t *subscription = lGetRef(event_client, EV_sub_array);
      const lCondition *selection = subscription[type].where;
      const lEnumeration *fields = subscription[type].what;
      const lDescr *dp = getDescriptor(subscription, element, type);

      if (fields) {
         /* special handling for the JAT_Type lists stored in the job
          * structure. 
          */
         if (sub_filter) {
            int sub_type = -1;
            int i=-1;
            while (SOURCE_LIST[entry_counter][++i] != -1) {
               if (subscription[SOURCE_LIST[entry_counter][i]].what){
                  sub_type = SOURCE_LIST[entry_counter][i];
                  break;
               }
            }
            el = elem_select(subscription, element, 
                              FIELD_LIST[entry_counter], selection, 
                              fields, dp, sub_type);
         }
         else {
             el = lSelectElemD(element, selection, dp, fields);
         }
         
         /* do not send empty elements */
         if (!el)
            return;
      }
      else {
         el = lCopyElem(element);
         dp = lGetElemDescr(element);
      }
      if (el) {
         lp = lCreateList("changed element", dp);
         lAppendElem(lp, el);
      }       
   }
  
   add_list_event(event_client, timestamp, type, 
                       intkey, intkey2, strkey, strkey2, lp, false);
   
 
   return;
}

/****** Eventclient/Server/total_update_event() *******************************
*  NAME
*     total_update_event() -- create a total update event
*
*  SYNOPSIS
*     static void 
*     total_update_event(lListElem *event_client, ev_event type) 
*
*  FUNCTION
*     Creates an event delivering a certain list of objects for an event client.
*     For event clients that have subscribed a session list filtering can be done
*     here.
*
*  INPUTS
*     lListElem *event_client - event client to receive the list
*     ev_event type           - event describing the list to update
*
*******************************************************************************/
static void total_update_event(lListElem *event_client, ev_event type) 
{
   u_long32 i;
   lListElem *event;
   lList *lp = NULL;
   char buffer[1024];
   dstring buffer_wrapper;
   const char *session;
   lCondition *selection = NULL;
   lEnumeration *fields = NULL;
   const lDescr *descr=NULL;

   DENTER(TOP_LAYER, "total_update_event");
   sge_dstring_init(&buffer_wrapper, buffer, sizeof(buffer));
   session = lGetString(event_client, EV_session);

   if (eventclient_subscribed(event_client, type, NULL)) {
      switch (type) {
         case sgeE_ADMINHOST_LIST:
            lp = Master_Adminhost_List;
            break;
         case sgeE_CALENDAR_LIST:
            lp = Master_Calendar_List;
            break;
         case sgeE_CKPT_LIST:
            lp = Master_Ckpt_List;
            break;
         case sgeE_CENTRY_LIST:
            lp = Master_CEntry_List;
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
         case sgeE_CQUEUE_LIST:
            lp = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
            break;
         case sgeE_SCHED_CONF:
            lp = *sconf_get_config_list();
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
         case sgeE_HGROUP_LIST:
            lp = Master_HGroup_List;
            break;
#ifndef __SGE_NO_USERMAPPING__
         case sgeE_CUSER_LIST:
            lp = Master_Cuser_List;
            break;
#endif
         default:
            WARNING((SGE_EVENT, MSG_EVE_TOTALUPDATENOTHANDLINGEVENT_I, type));
            DEXIT;
            return;
      }

      {
         subscription_t *subscription = lGetRef(event_client, EV_sub_array);
         selection = subscription[type].where;
         fields = subscription[type].what;
         descr = getDescriptorL(subscription, lp, type);
         
         DPRINTF(("deliver event: %d with where filter=%s and what filter=%s\n", 
                  type, selection?"true":"false", fields?"true":"false"));
      
      
         event = lCreateElem(ET_Type); 

         /* fill in event number and increment EV_next_number of event recipient */
         i = lGetUlong(event_client, EV_next_number);
         lSetUlong(event, ET_number, i++);
         lSetUlong(event_client, EV_next_number, i);
         
         lSetUlong(event, ET_type, type); 

         if (fields) {
            lList *reduced_lp = NULL;
            if (!list_select(subscription, type, &reduced_lp, lp, selection, fields, descr)){
               reduced_lp = lSelectD("updating list", lp, selection,descr, fields); 
            }   
            lSetList(event, ET_new_version, reduced_lp);
         } else {
            lSetList(event, ET_new_version, lCopyList("updating list", lp));
         }
      } 
      /* build a new event list if not exists */
      {
         lList *llp = NULL;
         llp = lGetList(event_client, EV_events); 
         if (!llp) {
            llp=lCreateList("", ET_Type);
            lSetList(event_client, EV_events, llp);
         }
   
         DPRINTF(("%d %s\n", lGetUlong(event_client, EV_id), 
                  event_text(event, &buffer_wrapper)));
         /* chain in new event */
         lAppendElem(llp, event);
      }
   }

   DEXIT;
   return;
} /* total_update_event() */


/****** evm/sge_event_master/list_select() *************************************
*  NAME
*     list_select() -- makes a reduced job list dublication 
*
*  SYNOPSIS
*     static bool list_select(subscription_t *subscription, int type, lList 
*     **reduced_lp, lList *lp, const lCondition *selection, const lEnumeration 
*     *fields, const lDescr *descr) 
*
*  FUNCTION
*     Only works on job events. All others are ignored. The job events
*     need some special handling and this is done in this function. The
*     JAT_Type list can be subscribed by its self and it is also part
*     of the JB_Type. If a JAT_Type filter is set, this function also
*     filters the JAT_Type lists in the JB_Type lists.
*
*  INPUTS
*     subscription_t *subscription - subscription array 
*     int type                     - event type 
*     lList **reduced_lp           - target list (has to be an empty list) 
*     lList *lp                    - source list (will be modified) 
*     const lCondition *selection  - where filter 
*     const lEnumeration *fields   - what filter 
*     const lDescr *descr          - reduced descriptor 
*
*  RESULT
*     static bool - true, if it was a job event 
*
*******************************************************************************/
static bool list_select(subscription_t *subscription, int type, lList **reduced_lp, lList *lp, const lCondition *selection, const lEnumeration *fields,  const lDescr *descr)
{
   bool ret = false;
   int entry_counter;
   int event_counter;

   DENTER(TOP_LAYER, "list_select");
   
   for (entry_counter = 0; entry_counter < LIST_MAX; entry_counter++) {
      event_counter = -1;
      while (EVENT_LIST[entry_counter][++event_counter] != -1){
         if (type == EVENT_LIST[entry_counter][event_counter]) {
            int sub_type = -1;
            int i=-1;

            while (SOURCE_LIST[entry_counter][++i] != -1) {
               if (subscription[SOURCE_LIST[entry_counter][i]].what){
                  sub_type = SOURCE_LIST[entry_counter][i];
                  break;
               }
            }
  
            if (sub_type != -1) {
               lListElem *element = NULL;
               lListElem *reduced_el = NULL;

               
               ret = true;
               *reduced_lp = lCreateList("update", descr);        
               
               for_each(element, lp) {
                  reduced_el = elem_select(subscription, element, 
                               FIELD_LIST[entry_counter], selection, 
                               fields, descr, sub_type);
               
                  lAppendElem(*reduced_lp, reduced_el);
               }
            }
            else {
               DPRINTF(("no sub type filter specified\n"));
            }
            goto end;
         } /* end if */
      } /* end while */
   } /* end for */
end:   
   DEXIT;
   return ret;        
}

/****** sge_event_master/elem_select() ******************************************
*  NAME
*     elem_select() -- makes a reduced copy of an element with reducing sublists
*                      as well
*
*  SYNOPSIS
*     static lListElem *elem_select(subscription_t *subscription, lListElem *element, 
*                              const int ids[], const lCondition *selection, 
*                              const lEnumeration *fields, const lDescr *dp, int sub_type)
*
*  FUNCTION
*     The function will apply the given filters for the element. Before the element
*     is reduced, all attribute sub lists named in "ids" will be removed from the list and
*     reduced. The reduced sub lists will be added the the reduced element and the original
*     element will be restored. The sub-lists will only be reduced, if the reduced element
*     still contains their attributes.
*
*  INPUTS
*     subscription_t *subscription - subscription array 
*     lListElem *element           - the element to reduce
*     const int ids[]              - attribute with sublists to be reduced as well
*     const lCondition *selection  - where filter 
*     const lEnumeration *fields   - what filter 
*     const lDescr *descr          - reduced descriptor 
*     int sub_type                 - list type of the sublists.
*
*  RESULT
*     bool - the reduced element, or NULL if something went wrong
*
*  NOTE:
*  MT-NOTE: works only on the variables -> thread save
*
*******************************************************************************/
static lListElem *elem_select(subscription_t *subscription, lListElem *element, 
                              const int ids[], const lCondition *selection, 
                              const lEnumeration *fields, const lDescr *dp, int sub_type) {
   const lCondition *sub_selection = NULL; 
   const lEnumeration *sub_fields = NULL;
   const lDescr *sub_descr = NULL;
   lList **sub_list;
   lListElem *el = NULL;
   int counter;
 
   DENTER(TOP_LAYER, "elem_select");
 
   if (!element) {
      DEXIT;
      return NULL;
   }
 
   if (sub_type <= sgeE_ALL_EVENTS || sub_type >= sgeE_EVENTSIZE){
      
      /* TODO: SG: add error message */
      DPRINTF(("wrong event sub type\n"));
      DEXIT;
      return NULL;
   }
   
   /* get the filters for the sub lists */
   if (sub_type>=0){
      sub_selection = subscription[sub_type].where;
      sub_fields = subscription[sub_type].what; 
   }
  
   if (sub_fields){ /* do we have a sub list filter, otherwise ... */
      int ids_size = 0;   

      /* allocate memory to store the sub-lists, which should be handeled special */
      while (ids[ids_size] != -1)
         ids_size++;
      sub_list = malloc(ids_size * sizeof(lList*));
      memset(sub_list, 0 , ids_size * sizeof(lList*));
      
      /* remove the sub-lists from the main element */
      for(counter = 0; counter < ids_size; counter ++){
         lXchgList(element, ids[counter], &(sub_list[counter]));
      } 
      
      /* get descriptor for reduced sub-lists */
      if (!sub_descr){
         for(counter = 0; counter < ids_size; counter ++){
            if (sub_list[counter]){
               sub_descr = getDescriptorL(subscription, sub_list[counter], sub_type);
               break;
            }
         } 
      }

      /* copy the main list */
      if (!fields) /* there might be no filter for the main element, but for the sub-lists */
         el = lCopyElem(element);
      else if (!dp) /* for soem reason, we did not get a descriptor for the target element */
         el = lSelectElem(element, selection, fields);
      else   
         el = lSelectElemD(element, selection, dp, fields);

      /* if we have a new reduced main element */
      if (el) {  /* copy the sub-lists, if they are still part of the reduced main element */
         for(counter = 0; counter < ids_size; counter ++){
            if (sub_list[counter] && (lGetPosViaElem(el, ids[counter]) != -1)){
               lSetList(el, ids[counter], lSelectD("", sub_list[counter], sub_selection, sub_descr, sub_fields));
            }
            
         } 
      }
      
      /* restore the old sub_list */
      for(counter = 0; counter < ids_size; counter ++){
         lXchgList(element, ids[counter], &(sub_list[counter]));
      } 

      FREE(sub_list);
   }
   else /* .... do a simple select */{
      DPRINTF(("no sub filter specified\n"));
      el = lSelectElemD(element, selection, dp, fields);
   }   

   DEXIT;
   return el;
}

/****** Eventclient/Server/eventclient_list_locate() **************************
*  NAME
*     eventclient_list_locate_by_adress() -- search event client by adress
*
*  SYNOPSIS
*     #include "sge_event_master.h"
*
*     lListElem *
*     eventclient_list_locate_by_adress(const char *host, 
*                     const char *commproc, u_long32 id) 
*
*  FUNCTION
*     Searches the event client list for an event client with the
*     specified commlib adress.
*     Returns a pointer to the event client object or
*     NULL, if no such event client is registered.
*
*  INPUTS
*     const char *host     - hostname of the event client to search
*     const char *commproc - commproc of the event client to search
*     u_long32 id          - id of the event client to search
*
*  RESULT
*     lListElem* - event client object or NULL.
*
*  NOTES
*
*******************************************************************************/
static lListElem *
eventclient_list_locate_by_adress(const char *host, const char *commproc, u_long32 id)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "eventclient_list_locate_by_adress");

   for_each (ep, Master_Control.clients)
      if (lGetUlong(ep, EV_commid) == id &&
          !sge_hostcmp(lGetHost(ep, EV_host), host) &&
           !strcmp(lGetString(ep, EV_commproc), commproc))
           break;

   DEXIT;
   return ep;
}

/****** sge_event_master/getDescriptor() **************************************
*  NAME
*     getDescriptor() -- returns a reduced desciptor 
*
*  SYNOPSIS
*     static const lDescr* getDescriptor(subscription_t *subscription, const 
*     lListElem* element, int type) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     subscription_t *subscription - subscription array 
*     const lListElemÜ* element    - source element (used to extract the descriptor) 
*     int type                     - event type 
*
*  RESULT
*     static const lDescr* - reduced descriptor or NULL, if no what exists 
*
*  NOTE
*   MT-NOTE: thread save, works only on the submitted variables.
*******************************************************************************/
static const lDescr* getDescriptor(subscription_t *subscription, const lListElem* element, int type){
   const lDescr *dp = NULL;
   if (subscription[type].what) {
      if (!(dp = subscription[type].descr)) {
         subscription[type].descr = lGetReducedDescr(lGetElemDescr(element), subscription[type].what );
         dp = subscription[type].descr; 
      }
   }
   return dp;
}

/****** sge_event_master/getDescriptorL() **************************************
*  NAME
*     getDescriptorL() -- returns a reduced desciptor 
*
*  SYNOPSIS
*     static const lDescr* getDescriptorL(subscription_t *subscription, const 
*     lList* list, int type) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     subscription_t *subscription - subscription array 
*     const lList* list            - source list 
*     int type                     - event type 
*
*  RESULT
*     static const lDescr* - reduced descriptor or NULL, if no what exists 
*
*  NOTE
*   MT-NOTE: thread save, works only on the submitted variables.
*******************************************************************************/
static const lDescr* getDescriptorL(subscription_t *subscription, const lList* list, int type){
   const lDescr *dp = NULL;
   if (subscription[type].what) {
      if (!(dp = subscription[type].descr)) {
         subscription[type].descr = lGetReducedDescr(lGetListDescr(list), subscription[type].what );
         dp = subscription[type].descr; 
      }
   }
   return dp;
}
