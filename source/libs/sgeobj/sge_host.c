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

#include "sge.h"
#include "sge_object.h"
#include "sge_host.h"
#include "sge_qinstance.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_host.h"
#include "sge_centry.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

lList *Master_Exechost_List = NULL;
lList *Master_Adminhost_List = NULL;
lList *Master_Submithost_List = NULL;

lListElem *
host_list_locate(const lList *host_list, const char *hostname) 
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

/****** sgeobj/host/host_is_referenced() **************************************
*  NAME
*     host_is_referenced() -- Is a given host referenced in other objects? 
*
*  SYNOPSIS
*     bool host_is_referenced(const lListElem *host, 
*                             lList **answer_list, 
*                             const lList *queue_list) 
*
*  FUNCTION
*     This function returns true if the given "host" is referenced
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
bool host_is_referenced(const lListElem *host, 
                        lList **answer_list,
                        const lList *queue_list)
{
   bool ret = false;

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
         ret = true;
      }
   }
   return ret;
}

/****** sgeobj/host/host_get_load_value() *************************************
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

int sge_resolve_host(lListElem *ep, int nm) 
{
   int pos, ret;
   int dataType;
   char unique[MAXHOSTLEN];
   const char *hostname;

   DENTER(TOP_LAYER, "sge_resolve_host");

   if (ep == NULL) {
      DEXIT;
      return -1;
   }

   /* ep is no host element, if ep has no nm */
   if ((pos = lGetPosViaElem(ep, nm)) < 0) {
      DEXIT;
      return -1;
   }

   dataType = lGetPosType(lGetElemDescr(ep),pos);
   switch (dataType) {
       case lStringT:
          hostname = lGetPosString(ep, pos);
          DPRINTF(("!!!!!!! sge_resolve_host: WARNING call with old lStringT data type,\n"));
          DPRINTF(("!!!!!!! this data type should be replaced with lHostT data type in\n"));
          DPRINTF(("!!!!!!! the future! Nevertheless, just a warning! Function works fine!\n"));
          break;

       case lHostT:
          hostname = lGetPosHost(ep, pos);
          break;

       default:
          hostname = NULL;
          break;
   }
   ret = sge_resolve_hostname(hostname, unique, nm);

   if (ret == CL_OK) {
      switch (dataType) {
       case lStringT:
          lSetPosString(ep, pos, unique);
          break;

       case lHostT:
          lSetPosHost(ep, pos, unique);
          break;
      }
   }
   DEXIT;
   return ret;
}

int sge_resolve_hostname(const char *hostname, char *unique, int nm) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "sge_resolve_hostname");

   if (hostname != NULL) {
      /* 
       * these "spezial" names are resolved:
       *    "global", "unknown", "template")
       */
      switch (nm) {
      case CE_stringval:
         if (!strcmp(hostname, SGE_UNKNOWN_NAME)) {
            strcpy(unique, hostname);
            ret = 0;
         } else {
            ret = getuniquehostname(hostname, unique, 0);
         }
         break;
      case EH_name:
         if (!strcmp(hostname, SGE_GLOBAL_NAME) || 
             !strcmp(hostname, SGE_TEMPLATE_NAME)) {
            strcpy(unique, hostname);
            ret = 0;
         } else {
            ret = getuniquehostname(hostname, unique, 0);
         }
         break;
      default:
         ret = getuniquehostname(hostname, unique, 0);
         break;
      }
   } else {
      ret = CL_RANGE;
   }

   DEXIT;
   return ret;
}

bool
host_is_centry_referenced(const lListElem *this_elem, const lListElem *centry)
{
   bool ret = false;

   DENTER(TOP_LAYER, "host_is_centry_referenced");
   if (this_elem != NULL) {
      const char *name = lGetString(centry, CE_name);
      lList *centry_list = lGetList(this_elem, EH_consumable_config_list);
      lListElem *centry_ref = lGetElemStr(centry_list, CE_name, name);

      if (centry_ref != NULL) {
         ret = true;
      }
   }
   DEXIT;
   return ret;
}

bool
host_is_centry_a_complex_value(const lListElem *this_elem, 
                               const lListElem *centry)
{
   bool ret = false;

   DENTER(TOP_LAYER, "host_is_centry_a_complex_value");
   ret = host_is_centry_referenced(this_elem, centry);
   DEXIT;
   return ret;
}

