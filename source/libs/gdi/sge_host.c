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

/* #include "sge_all_listsL.h" */
#include "sge_object.h"
#include "sge_host.h"
#include "sge_queue.h"
#include "sge_m_event.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_host.h"

#include "msg_common.h"
#include "msg_gdilib.h"

lList *Master_Exechost_List = NULL;
lList *Master_Adminhost_List = NULL;
lList *Master_Submithost_List = NULL;

lListElem *host_list_locate(lList *host_list, const char *hostname) 
{
   lListElem *ret = NULL;
   DENTER(TOP_LAYER, "host_list_locate");

   if (hostname != NULL) {
      if (host_list != NULL) {
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

            ret = lGetElemHost(host_list, nm, hostname);
         }
      }
   } else {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
   }
   DEXIT;
   return ret;
}

/****** gdi/host/host_is_referenced() ******************************************
*  NAME
*     host_is_referenced() -- Is a given host referenced in other objects? 
*
*  SYNOPSIS
*     int host_is_referenced(const lListElem *host, 
*                            lList **answer_list, 
*                            const lList *queue_list) 
*
*  FUNCTION
*     This function returns true (1) if the given "host" is referenced
*     in a queue contained in "queue_list". If this is the case than
*     a corresponding message will be added to the "answer_list". 
*
*  INPUTS
*     const lListElem *host   - EH_Type, AH_Type or SH_Type object 
*     lList **answer_list     - AN_Type list 
*     const lList *queue_list - QU_Type list 
*
*  RESULT
*     int - true (1) or false (0) 
******************************************************************************/
int host_is_referenced(const lListElem *host, 
                       lList **answer_list,
                       const lList *queue_list)
{
   int ret = 0;

   if (host != NULL) {
      lListElem *queue = NULL;
      const char *hostname = NULL;
      int nm = NoName;
      int pos = -1;

      if (object_has_type(host, EH_Type)) {
         nm = object_get_primary_key(EH_Type);
      } else if (object_has_type(host, AH_Type)) {
         nm = object_get_primary_key(AH_Type);
      } else if (object_has_type(host, SH_Type)) {
         nm = object_get_primary_key(SH_Type);
      }
      pos = lGetPosViaElem(host, nm);
      hostname = lGetPosHost(host, pos);
      queue = lGetElemHost(queue_list, QU_qhostname, hostname); 

      if (queue != NULL) {
         const char *queuename = lGetString(queue, QU_qname);

         sprintf(SGE_EVENT, MSG_HOSTREFINQUEUE_SS, hostname, queuename);
         answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                         ANSWER_QUALITY_INFO);
         ret = 1;
      }
   }
   return ret;
}

/****** gdi/host/host_get_load_value() *****************************************
*  NAME
*     host_get_load_value() -- return a load value of an exec host
*
*  SYNOPSIS
*     const char* host_get_load_value(lListElem *host, const char *name) 
*
*  FUNCTION
*     Returns a certain load value for a certain host.
*
*  INPUTS
*     lListElem *host  - the host to query
*     const char *name - the name of the load value
*
*  RESULT
*     const char* - string describing the load value
*
*  EXAMPLE
*     lListElem *host = lGetElemHost(Master_Host_List, EH_name, "myhost");
*     const char *value = host_get_load_value(host, "np_load_avg");
*     printf("The load on host myhost is %s\n", value);
*
*******************************************************************************/
const char *host_get_load_value(lListElem *host, const char *name)
{
   lListElem *load;
   const char *value = NULL;

   load = lGetSubStr(host, HL_name, name, EH_load_list);
   if(load != NULL) {
      value = lGetString(load, HL_value);
   }
   
   return value;
}

/****** gdi/host/host_update_master_list() *****************************
*  NAME
*     host_update_master_list() -- update the master hostlists
*
*  SYNOPSIS
*     int host_update_master_list(sge_event_type type, 
*                                 sge_event_action action, 
*                                 lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master lists of hosts
*     based on an event.
*     The function is called from the event mirroring interface.
*     Updates admin, submit or execution host list depending
*     on the event received.
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
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_host_key()
*******************************************************************************/
int host_update_master_list(sge_event_type type, sge_event_action action, 
                            lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "host_update_master_list");

   switch(type) {
      case SGE_EMT_ADMINHOST:
         list = &Master_Adminhost_List;
         list_descr = AH_Type;
         key_nm = AH_name;
         break;
      case SGE_EMT_EXECHOST:
         list = &Master_Exechost_List;
         list_descr = EH_Type;
         key_nm = EH_name;
         break;
      case SGE_EMT_SUBMITHOST:
         list = &Master_Submithost_List;
         list_descr = SH_Type;
         key_nm = SH_name;
         break;
      default:
         return FALSE;
   }

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_host_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}
