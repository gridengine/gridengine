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
#include "cull.h"
#include "sgermon.h"
#include "sge_log.h"
#include "time_event.h"
#include "sge_time_eventL.h"
#include "sge_time.h"
#include "msg_common.h"
#include "msg_qmaster.h"

static lList* event_queue = NULL;
extern volatile int shut_me_down;


/****** Timeeventmanager/--Concept ******************************
*  NAME
*     Time event manager -- Concept
*
*  SYNOPSIS
*
*  FUNCTION
*     This module implements a queue of future time events. It can
*     be used to efficiently manage time ahead in the future. 
* 
*     Time events are registered using te_add() and unregistered
*     using te_delete(). Time events are executed by te_deliver()
*     when the time specified with the te_add() operation is exceeded.
*   
*  NOTES
*     This module is currently used in Grid Engine qmaster only but 
*     should be usable in any other component.
*
*  SEE ALSO
*     Timeeventmanager/--Event_Client_Interface
*     Timeeventmanager/te_tab_t
*     Timeeventmanager/te_deliver_func_t
*     Timeeventmanager/te_deliver()
*     Timeeventmanager/te_add()
*     Timeeventmanager/te_delete()
*******************************************************************************/

/****** Timeeventmanager/te_tab_t ******************************
*  NAME
*     Time event manager -- Time event mapping table.
*
*  SYNOPSIS
*     typedef struct {
*        u_long32 type;
*        te_deliver_func_t func;
*     } te_tab_t;
*
*  FUNCTION
*     The time event mapping table maps time event types to 
*     functions that are to be called when the time for the
*     time event has exceeded. An array of te_tab_t elements 
*     must be passed to te_deliver(). To keep time event 
*     manager as an indepenent module this table must be part 
*     of the component actually using time event manager module.
* 
*  SEE ALSO
*     Timeeventmanager/te_deliver()
*******************************************************************************/

/****** Timeeventmanager/te_deliver_func_t ******************************
*  NAME
*     Time event manager -- Time event delivery function.
*
*  SYNOPSIS
*     typedef void (*te_deliver_func_t)(
*           u_long32 type, 
*           u_long32 when, 
*           u_long32 uval0, 
*           u_long32 uval1, 
*           const char *sval);
*
*  FUNCTION
*     The time event delivery function is called by te_deliver() 
*     each time when the time for the corresponding time event 
*     has exceeded. The type of the time event, the current time and 
*     the parameters that were associated when te_add() was called are
*     are passed to the function. 
*
*  NOTES
*     When the time event function was called this has no impact on
*     the time event queue. In particular it must be noted that the
*     time event delivery functions is responsible for removing the
*     the time event from the time event queue using te_delete(). 
*     If this is not done the time event remains in the queue and is
*     called again and again.
* 
*  SEE ALSO
*     Timeeventmanager/te_deliver()
*******************************************************************************/


/****** Timeeventmanager/te_deliver() ************************************************
*  NAME
*     te_deliver() -- Execute registered time events
*
*  SYNOPSIS
*     void te_deliver(u_long32 now, te_tab_t *tab) 
*
*  FUNCTION
*     Time events for which time has exceeded are executed. The function 
*     pointer of the function that is associated with each time event is 
*     taken from the time event mapping table that is passed to te_deliver().
*
*  INPUTS
*     u_long32 now  - The current time.
*     te_tab_t *tab - The table event mapping table to be used.
*
*  NOTES
*     MT-NOTE: te_deliver() is not MT safe
*
*  BUGS
*     te_deliver() should be capable to indicate an error condition.
*
*  SEE ALSO
*     Timeeventmanager/te_tab_t
*******************************************************************************/
void te_deliver(
u_long32 now,
te_tab_t *tab 
) {
   int i, n;
   u_long32 this_type;
   lListElem *event;
   const char *sval;
   static u_long32 last = 0;
   
   DENTER(TOP_LAYER, "te_deliver");

   if (last > now) {
      WARNING((SGE_EVENT, MSG_SYSTEM_SYSTEMHASBEENMODIFIEDXSECONDS_I, (int)(now-last)));
      for_each (event, event_queue)
         lSetUlong(event, TE_when, lGetUlong(event, TE_when) - (last - now));
   }
   last = now;

   while ((event = lFirst(event_queue)) && 
         lGetUlong(event, TE_when) <= (now=sge_get_gmt()) && 
         !shut_me_down) {
      n = lGetNumberOfElem(event_queue);
      event = lDechainElem(event_queue, event);
      if (n == lGetNumberOfElem(event_queue)) {
         DPRINTF(("!!!!!!!!!!!!!!!!!!!!!!! still %d events\n", n));
      }
      this_type = lGetUlong(event, TE_type);
      sval = lGetString(event, TE_sval);
      DPRINTF(("TIME_EVENT(t:"u32" w:"u32" u0:"u32" u1:"u32" s:%s seqno:"u32")\n",
            this_type,
            lGetUlong(event, TE_when),
            lGetUlong(event, TE_uval0),
            lGetUlong(event, TE_uval1),
            sval?sval:MSG_SMALLNULL,
            lGetUlong(event, TE_seqno)
            )); 
         
      /* search matching event delivery func */
      for (i=0; tab[i].type && tab[i].type != this_type; i++)
         ;

      if (tab[i].type == this_type)
         tab[i].func(
            this_type,
            lGetUlong(event, TE_when), /* would be 'now' better ?? */
            lGetUlong(event, TE_uval0),
            lGetUlong(event, TE_uval1),
            sval); 
       else
         ERROR((SGE_EVENT, MSG_SYSTEM_RECEIVEDUNKNOWNEVENT));
      event = lFreeElem(event);
   }

   DEXIT;
   return;
}


/****** Timeeventmanager/te_add() ****************************************************
*  NAME
*     te_add() -- Add a time event into the time event queue.
*
*  SYNOPSIS
*     void te_add(u_long32 type, u_long32 when, u_long32 uval0, u_long32 uval1, 
*     const char *sval) 
*
*  FUNCTION
*     A new time event is registed. It will be executed by te_deliver()
*     when the specified time has come. Two ulong keys and one string key
*     can be associated with the time event.
*
*  INPUTS
*     u_long32 type    - The type of the time event.
*     u_long32 when    - The time when the time event is to be executed.
*     u_long32 uval0   - The first u_long32 key.
*     u_long32 uval1   - The second u_long32 key.
*     const char *sval - The string key (may be NULL).
*
*  BUGS
*     te_add() should be capable to indicate an error condition.
*
*  NOTES
*     MT-NOTE: te_add() is not MT safe
*
*  SEE ALSO
*     Timeeventmanager/te_tab_t
*******************************************************************************/
void te_add(
u_long32 type,
u_long32 when,
u_long32 uval0,
u_long32 uval1,
const char *sval 
) {
   static lSortOrder *te_so = NULL;
   static int seqno = 0;
   lListElem *event;

   event = lCreateElem(TE_Type);
   lSetUlong(event, TE_type, type);
   lSetUlong(event, TE_when, when);
   lSetUlong(event, TE_uval0, uval0);
   lSetUlong(event, TE_uval1, uval1);
   lSetString(event, TE_sval, sval);
   lSetUlong(event, TE_seqno, seqno++);

   if (!te_so)
      te_so = lParseSortOrderVarArg(TE_Type, "%I+", TE_when);
   if (!event_queue)
      event_queue = lCreateList("event queue", TE_Type);

   lInsertSorted(te_so, event, event_queue);

   DPRINTF(("te_add(t:"u32" w:"u32" u0:"u32" u1:"u32" s:%s seqno:"u32")\n",
         lGetUlong(event, TE_type),
         lGetUlong(event, TE_when),
         lGetUlong(event, TE_uval0),
         lGetUlong(event, TE_uval1),
         sval?sval:MSG_SMALLNULL,
         lGetUlong(event, TE_seqno))); 
   return;
}



/****** Timeeventmanager/te_delete() *************************************************
*  NAME
*     te_delete() -- Removes spending events from time event queue.
*
*  SYNOPSIS
*     int te_delete(u_long32 type, const char *str, u_long32 uval0, u_long32 
*     uval1) 
*
*  FUNCTION
*     The specified time events are removed from the time event queue.
*
*  INPUTS
*     u_long32 type   - Specifies type of time events.
*     const char *str - Speciefies a str key associated with the time event.
*                       May be NULL.
*     u_long32 uval0  - Specifies first u_long32 key associated with time event.
*     u_long32 uval1  - Specifies second u_long32 key associated with time event.
*
*  RESULT
*     int - Returns number of time events that were removed.
*
*  NOTES
*     MT-NOTE: te_delete() is not MT safe
*******************************************************************************/
int te_delete(
u_long32 type,
const char *str,
u_long32 uval0,
u_long32 uval1 
) {
   int n;
   lCondition *where;

   DENTER(TOP_LAYER, "te_delete");

   n = lGetNumberOfElem(event_queue);

   if (str) {
      where = lWhere("%T(%I != %s || %I != %u || %I != %u || %I != %u )",
        TE_Type, TE_sval, str, TE_type, type, TE_uval0, uval0, TE_uval1, uval1);
   } else {
      where = lWhere("%T(%I != %u || %I != %u || %I != %u )",
            TE_Type, TE_type, type, TE_uval0, uval0, TE_uval1, uval1);
   }
   event_queue = lSelectDestroy(event_queue, where);
   where = lFreeWhere(where);
 
   DEXIT;
   return n - lGetNumberOfElem(event_queue);
}

