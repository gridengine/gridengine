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
#include "msg_utilib.h"

extern lList *Master_Exechost_List;
extern lList *Master_Adminhost_List;
extern lList *Master_Submithost_List;


/* ------------------------------------------------------------ */

lListElem *sge_locate_host(
char *unique,
u_long32 target 
) {
   lListElem *ep = NULL;
   lList *host_list = NULL;
   int nm = 0;
   lDescr *type = NULL;

   DENTER(CULL_LAYER, "sge_locate_host");

   if (!unique) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      DEXIT;
      return NULL;
   }

   switch ( target ) {
   case SGE_EXECHOST_LIST:
      host_list = Master_Exechost_List;
      nm = EH_name;
      type = EH_Type;
      break;
   case SGE_ADMINHOST_LIST:
      host_list = Master_Adminhost_List;
      nm = AH_name;
      type = AH_Type;
      break;
   case SGE_SUBMITHOST_LIST:
      host_list = Master_Submithost_List;
      nm = SH_name;
      type = SH_Type;
      break;
   default:
     DEXIT;
     return NULL;
   }

   ep = lGetElemHost(host_list, nm, unique);

   DEXIT;
   return ep;
}


