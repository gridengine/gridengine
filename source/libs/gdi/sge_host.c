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
#include <string.h>

#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_m_event.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"

#include "sge_host.h"

#include "msg_common.h"

lList *Master_Exechost_List = NULL;
lList *Master_Adminhost_List = NULL;
lList *Master_Submithost_List = NULL;

lListElem *host_list_locate(lList *host_list, const char *hostname) 
{
   lListElem *ret = NULL;
   DENTER(TOP_LAYER, "host_list_locate");

   if (hostname != NULL) {
      if (host_list != NULL) {
         const lDescr *descr = lGetListDescr(host_list);
         const lListElem *element = lFirst(host_list);
         int nm = NoName;

         if (element != NULL) {
            if (object_has_type(element, EH_Type)) {
               nm = object_get_primary_key(EH_Type);
            } else if (object_has_type(element, AH_Type)) {
               nm = object_get_primary_key(AH_Type);
            } else if (object_has_type(element, SH_Type)) {
               nm = object_get_primary_key(SH_Type);
            }

            DPRINTF(("%p %d\n", descr, nm));
            ret = lGetElemHost(host_list, nm, hostname);
         }
      }
   } else {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
   }
   DEXIT;
   return ret;
}



