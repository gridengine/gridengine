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

#include "sge_queue.h"
#include "sge_qinstance_state.h"

#include "msg_mirlib.h"

#include "sge_mirror.h"
#include "sge_queue_mirror.h"

/****** Eventmirror/queue/queue_update_master_list() ***************************
*  NAME
*     queue_update_master_list() -- update the master list of queues
*
*  SYNOPSIS
*     bool 
*     queue_update_master_list(sge_object_type type, sge_event_action action,
*                              lListElem *event, void *clientdata)
*
*  FUNCTION
*     Update the global master list of queues
*     based on an event.
*     The function is called from the event mirroring interface.
*
*  INPUTS
*     sge_object_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - true, if update is successfull, else false
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
bool 
queue_update_master_list(sge_object_type type, sge_event_action action,
                         lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   lListElem *queue;
   int key_nm;
   const char *key;

   DENTER(TOP_LAYER, "queue_update_master_list");

   list = &Master_Queue_List;
   list_descr = QU_Type;
   key_nm = QU_qname;

   key = lGetString(event, ET_strkey);

   queue = queue_list_locate(*list, key);

   if (action == SGE_EMA_MOD) {
      u_long32 type = lGetUlong(event, ET_type);

      if (type == sgeE_QUEUE_SUSPEND_ON_SUB || 
          type == sgeE_QUEUE_UNSUSPEND_ON_SUB) {
         if (queue == NULL) {
            ERROR((SGE_EVENT, MSG_QUEUE_CANTFINDQUEUEFORUPDATEIN_SS, key, 
                   SGE_FUNC));
            DEXIT;
            return false;
         }

         if (type == sgeE_QUEUE_SUSPEND_ON_SUB) {
            qinstance_state_set_susp_on_sub(queue, true);
         } else {
            qinstance_state_set_susp_on_sub(queue, false);
         }

         DEXIT;
         return true;
      }
   }

   if (sge_mirror_update_master_list(list, list_descr, queue, key, action, 
                                     event) != SGE_EM_OK) {
      DEXIT;
      return false;
   }

   DEXIT;
   return true;
}

