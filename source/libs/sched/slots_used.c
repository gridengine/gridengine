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
#include "sge_log.h"
#include "sgermon.h"
#include "slots_used.h"
#include "sge_qinstance.h"
#include "sge_centry.h"

#include "msg_schedd.h"

int qslots_used(
lListElem *qep 
) {
   lListElem *slots_ep;

   DENTER(CULL_LAYER, "qslots_used");

   if (!(slots_ep = lGetSubStr(qep, CE_name, "slots", 
            QU_consumable_actual_list))) {
      /* aaargh! may never happen */
      CRITICAL((SGE_EVENT, MSG_SLOTSUSED_SLOTSENTRYINQUEUEMISSING_S , lGetString(qep, QU_qname)));
      DEXIT;
      return 1000000;
   }
   DEXIT;
#ifndef WIN32NATIVE
   return lGetDouble(slots_ep, CE_doubleval);
#else
   return (int) lGetDouble(slots_ep, CE_doubleval);
#endif
}

void set_qslots_used(
lListElem *qep,
int slots 
) {
   lListElem *slots_ep;

   DENTER(TOP_LAYER, "set_qslots_used");

   if (!(slots_ep = lGetSubStr(qep, CE_name, "slots", 
            QU_consumable_actual_list))) {
      /* aaargh! may never happen */
      ERROR((SGE_EVENT, MSG_SLOTSUSED_SLOTSENTRYINQUEUEMISSING_S, lGetString(qep, QU_qname)));
      DEXIT;
      return;
   }
   lSetDouble(slots_ep, CE_doubleval, slots);
   DEXIT;
   return;
}

