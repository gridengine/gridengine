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

#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"

#include "sge_sharetree.h"

#include "msg_gdilib.h"
#include "msg_sgeobjlib.h"

#include "sge_mirror.h"
#include "sge_sharetree_mirror.h"

/****** gdi/sharetree/sharetree_update_master_list() *****************************
*  NAME
*     sharetree_update_master_list() -- update the master sharetree list
*
*  SYNOPSIS
*     int sharetree_update_master_list(sge_event_type type,
*                                      sge_event_action action,
*                                      lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list for the sharetree
*     based on an event.
*     The function is called from the event mirroring interface.
*     Sharetree events always contain the whole sharetree, that
*     replaces an existing sharetree in the master list.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*******************************************************************************/
int sharetree_update_master_list(sge_event_type type, sge_event_action action,
                                 lListElem *event, void *clientdata)
{
   lList *src;

   DENTER(TOP_LAYER, "sharetree_update_master_list");

   /* remove old share tree */
   Master_Sharetree_List = lFreeList(Master_Sharetree_List);

   if ((src = lGetList(event, ET_new_version))) {
      /* install new one */
      Master_Sharetree_List = lCreateList("share tree",
        lGetElemDescr(lFirst(lGetList(event, ET_new_version))));
      lAppendElem(Master_Sharetree_List, lDechainElem(src, lFirst(src)));
   }

   DEXIT;
   return TRUE;
}

