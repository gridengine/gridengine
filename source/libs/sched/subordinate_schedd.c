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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include "cull.h"
#include "subordinate_schedd.h"
#include "sge_queueL.h"
#include "sgermon.h"

/* -----------------------------------------------

   test suspend on subordinate (nice neuron)

    A1            C1
      \          /
       \        /
        \      /
  A2--------->B-------> C2
        /      \
       /        \
      /          \
    A3            C3

   a queue C subordinated by B must be suspended if
   the used slots of queue B meet the thresold for C

*/
int tst_sos(
int used,      /* number of slots actually used in queue B     */
int total,     /* total number of slots in queue B               */
int own_sos,   /* this queue B is suspended because of a queue A */
lListElem *so  /* SO_Type referencing to a queue C               */
) {
   u_long32 threshold;

   DENTER(TOP_LAYER, "tst_sos");

   /* --- no recursive suspends
      1st check if B is suspended by
      one or more other queues A
   */
#if RECURSIVE_SUBORDINATES
   if (own_sos) {
      DPRINTF(("TSTSOS: self suspended\n"));
      DEXIT;
      return 1;
   }
#endif

   /*
      then check if B's usage meets the threshold
      for suspension of the subordinated queue C
   */
   if (!(threshold=lGetUlong(so, SO_threshold))) {
      /* queue must be full for suspend of queue C */
      DPRINTF(("TSTSOS: %sfull -> %ssuspended\n", (used>=total)?"":"not ", 
         (used>=total)?"":"not "));
      DEXIT;
         return (used>=total);
   } 

   /* used slots greater or equal threshold */
   DPRINTF(("TSTSOS: "u32" slots used (limit "u32") -> %ssuspended\n", 
      used, threshold, ( (u_long32)(used) >= threshold)?"":"not "));
   DEXIT;
   return ( (u_long32) (used) >= threshold);
}

int sos_schedd(
char *qname, /* name of a queue that needs suspension on subordinate */
lList *qlist /* complete queue list for recursivly suspension of other queues
 */ 
) {
   lListElem *q;
   u_long32 sos, state;
   int ret = 0;

   DENTER(TOP_LAYER, "sos_schedd");

   q = lGetElemStr(qlist, QU_qname, qname);
   if (!q) {
      /* 
         In the qlist we got is only a subset of all
         queues. If the rest of the queues is really
         suspended then this is no error because
         they are already suspended.
      */
      DEXIT;
      return 1;
   }

   /* increment sos counter */
   sos = lGetUlong(q, QU_suspended_on_subordinate);
   lSetUlong(q, QU_suspended_on_subordinate, ++sos);

   /* first sos ? */
   if (sos==1) {
      DPRINTF(("QUEUE %s GETS SUSPENDED ON SUBORDINATE\n", qname));
      /* state transition */
      state = lGetUlong(q, QU_state);
      state |= QSUSPENDED_ON_SUBORDINATE;
      lSetUlong(q, QU_state, state);
   }

   DEXIT;
   return ret;
}

