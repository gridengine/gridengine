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

#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"

#include "sge_job.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"

bool 
gqueue_is_suspended(const lList *this_list, const lList *qinstance_list) 
{
   bool ret = false;
   lListElem *gqueue;

   DENTER(TOP_LAYER, "gqueue_is_suspended");
   for_each(gqueue, this_list) {
      const char *queue_name = lGetString(gqueue, JG_qname);
      lListElem *qinstance = qinstance_list_locate2(qinstance_list, queue_name);
   
      if ((qinstance != NULL) &&
          (qinstance_state_is_manual_suspended(qinstance) ||
           qinstance_state_is_susp_on_sub(qinstance) ||
           qinstance_state_is_cal_suspended(qinstance))) {
         ret = true;
         break;
      }
   }
   DEXIT;
   return ret;   
}

