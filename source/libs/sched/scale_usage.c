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
#include <string.h>
#include "cull.h"
#include "sge_hostL.h"
#include "sge_usageL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_sched.h"
#include "sgermon.h"

/* #define TRACE_INCOMING_USAGE */

int scale_usage(
lListElem *jep,
lListElem *jatep,
lList *scaling, /* HS_Type */
lList *previous  /* HS_Type */
/* UA_Type */
) {
   lListElem *sep, *ep, *prev;
   lList *usage = NULL;

   DENTER(TOP_LAYER, "scale_usage");

#ifndef TRACE_INCOMING_USAGE 
   if (!scaling) {
      DEXIT;
      return 0;
   }
#endif

   for_each (ep, lGetList(jatep, JAT_scaled_usage_list)) {
#ifdef TRACE_INCOMING_USAGE
      DPRINTF(("%s = %f\n", 
         lGetString(ep, UA_name), 
         lGetDouble(ep, UA_value))); 
#endif
      if ((sep=lGetElemStr(scaling, HS_name, lGetString(ep, UA_name)))) 
         lSetDouble(ep, UA_value, 
            lGetDouble(ep, UA_value) * lGetDouble(sep, HS_value));
   }

   /* summarize sge usage */
   for_each (prev, previous) {
      if (!strcmp(lGetString(prev, UA_name), USAGE_ATTR_CPU) ||
          !strcmp(lGetString(prev, UA_name), USAGE_ATTR_IO)  ||
          !strcmp(lGetString(prev, UA_name), USAGE_ATTR_IOW) ||
          !strcmp(lGetString(prev, UA_name), USAGE_ATTR_VMEM) ||
          !strcmp(lGetString(prev, UA_name), USAGE_ATTR_MEM)) {
         if ((ep=lGetSubStr(jatep, UA_name, lGetString(prev, UA_name), 
             JAT_scaled_usage_list)))
            lSetDouble(ep, UA_value, lGetDouble(ep, UA_value) + 
               lGetDouble(prev, UA_value));
         else {
            if (!usage && !(usage = lGetList(jatep, JAT_scaled_usage_list))) {
               usage = lCreateList("usage", UA_Type);
               lSetList(jatep, JAT_scaled_usage_list, usage);
            }
            lAppendElem(usage, lCopyElem(prev));
         }
      }
   }

   DEXIT;
   return 0;
}
