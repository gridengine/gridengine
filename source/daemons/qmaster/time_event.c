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

/* add event into event queue 
*/
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


/* removes all pending events of type 'type' with 'str' as key */
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

