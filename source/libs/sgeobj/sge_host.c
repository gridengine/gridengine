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
#include "sge_conf.h"
#include "sge_host.h"
#include "sge_qinstance.h"
#include "commlib.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_centry.h"
#include "sge_load.h"

#include "sgeobj/sge_str.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"
#include "sge_cqueueL.h" 
#include "sge_hgroup.h"
#include "sge_hrefL.h"

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
*     in a cqueue contained in "queue_list" or in a host group. 
*     If this is the case than a corresponding message will be added 
*     to the "answer_list". 
*
*  INPUTS
*     const lListElem *host   - EH_Type, AH_Type or SH_Type object 
*     lList **answer_list     - AN_Type list 
*     const lList *queue_list - CQ_Type list 
*     const lList *hgrp_list  - HGRP_Type list (Master list)
*
*  RESULT
*     int - true (1) or false (0) 
******************************************************************************/
bool host_is_referenced(const lListElem *host, 
                        lList **answer_list,
                        const lList *queue_list,
                        const lList *hgrp_list)
{
   bool ret = false;

   if (host != NULL) {
      lListElem *cqueue = NULL;
      lListElem *queue = NULL;
      const char *hostname = NULL;
      int nm = NoName;

      if (object_has_type(host, EH_Type)) {
         nm = object_get_primary_key(EH_Type);
      } else if (object_has_type(host, AH_Type)) {
         nm = object_get_primary_key(AH_Type);
      } else if (object_has_type(host, SH_Type)) {
         nm = object_get_primary_key(SH_Type);
      }
      hostname = lGetHost(host, nm);

      /* look at all the queue instances and figure out, if one still references
         the host we are looking for */
      for_each(cqueue, queue_list) { 
         queue = lGetSubHost(cqueue, QU_qhostname, hostname, CQ_qinstances); 

         if (queue != NULL) {
            const char *queuename = lGetString(cqueue, CQ_name);

            sprintf(SGE_EVENT, MSG_HOSTREFINQUEUE_SS, hostname, queuename);
            answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                            ANSWER_QUALITY_INFO);
            ret = true;
            break;
         }
      }

      /* if we have not found a reference yet, we keep looking in the host groups, if
         we have an exec host */
      if (!ret && object_has_type(host, EH_Type)) {
         lListElem *hgrp_elem = NULL;
         lList *host_list;

         for_each (hgrp_elem, hgrp_list) {
            hgroup_find_all_references(hgrp_elem, NULL, hgrp_list, &host_list, NULL);
            if (host_list != NULL) {
               if (lGetElemHost(host_list, HR_name, hostname) != NULL) {
                  const char *hgrp_name = lGetHost(hgrp_elem, HGRP_name);

                  sprintf(SGE_EVENT, MSG_HOSTREFINHGRP_SS, hostname, hgrp_name);
                  answer_list_add(answer_list, SGE_EVENT, STATUS_EUNKNOWN,
                            ANSWER_QUALITY_INFO);

                  ret = true;
                  break;
               }
            }
         }
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

   if (host != NULL) {
      load = lGetSubStr(host, HL_name, name, EH_load_list);
      if(load != NULL) {
         value = lGetString(load, HL_value);
      }
   }   
   return value;
}


/* MT-NOTE: sge_resolve_host() is MT safe */
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

   if (ret == CL_RETVAL_OK) {
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

/* MT-NOTE: sge_resolve_hostname() is MT safe */
int sge_resolve_hostname(const char *hostname, char *unique, int nm) 
{
   int ret = CL_RETVAL_OK;

   DENTER(TOP_LAYER, "sge_resolve_hostname");

   if (hostname == NULL) {
      DEXIT;
      return CL_RETVAL_PARAMS;
   }

   if (hostname != NULL) {
      /* 
       * these "spezial" names are resolved:
       *    "global", "unknown", "template")
       */
      switch (nm) {
      case CE_stringval:
         if (!strcmp(hostname, SGE_UNKNOWN_NAME)) {
            strcpy(unique, hostname);
            ret = CL_RETVAL_OK;
         } else {
            ret = getuniquehostname(hostname, unique, 0);
         }
         break;
      case EH_name:
      case CONF_hname:
         if (!strcmp(hostname, SGE_GLOBAL_NAME) || 
             !strcmp(hostname, SGE_TEMPLATE_NAME)) {
            strcpy(unique, hostname);
            ret = CL_RETVAL_OK;
         } else {
            ret = getuniquehostname(hostname, unique, 0);
         }
         break;
      default:
         ret = getuniquehostname(hostname, unique, 0);
         break;
      }
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
      const lList *ce_values = lGetList(this_elem, EH_consumable_config_list);
      const lList *load_list = lGetList(this_elem, EH_load_list);
      const lList *rep_vars = lGetList(this_elem, EH_report_variables);

      /* 
       * centry may be referenced in 
       *    - complex_values
       *    - load_list
       *    - report_variables
       */
      if (lGetElemStr(ce_values, CE_name, name) != NULL ||
          lGetElemStr(load_list, HL_name, name) != NULL ||
          lGetElemStr(rep_vars, STU_name, name) != NULL) {
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
   if (this_elem != NULL) {  
      const char *name = lGetString(centry, CE_name);
      const lList *ce_values = lGetList(this_elem, EH_consumable_config_list);
      const lList *load_list = lGetList(this_elem, EH_load_list);

      /* 
       * centry may be referenced in 
       *    - complex_values
       *    - load_list
       */
      if (lGetElemStr(ce_values, CE_name, name) != NULL ||
          lGetElemStr(load_list, HL_name, name) != NULL) {
         ret = true;
      }  
   }
   DEXIT;
   return ret;
}

bool
host_trash_load_values(lListElem *host)
{
   bool ret = true;

   DENTER(TOP_LAYER, "host_trash_load_values");

   if (host != NULL) {
      lListElem *ep, *next;
      lList *load_list = lGetList(host, EH_load_list);
      const char *host_name = lGetHost(host, EH_name);

      /* loop over load list */
      ep = lFirst(load_list);
      while (ep != NULL) {
         const char *load_name;

         next = lNext(ep);
         load_name = lGetString(ep, HL_name);

         /* we don't trash static load values like "arch" etc. */
         if (!sge_is_static_load_value(load_name)) {
            DPRINTF(("host "SFN": trashing load value "SFQ"\n",
                     host_name,
                     load_name));
            lRemoveElem(load_list, ep);
         }

         /* assign next element */
         ep = next;
      }
   }   

   DEXIT;
   return ret;
}

/****** sgeobj/host/host_list_merge() ******************************************
*  NAME
*     host_list_merge() -- merge global host settings into exec hosts
*
*  SYNOPSIS
*     bool 
*     host_list_merge(lList *this_list) 
*
*  FUNCTION
*     Merges settings from the global host to the exec hosts objects.
*     Currently this applies only to the report_variables attribute.
*
*  INPUTS
*     lList *this_list - the exec host list to work on
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: host_list_merge() is MT safe 
*
*  SEE ALSO
*     sgeobj/host/host_merge()
*******************************************************************************/
bool
host_list_merge(lList *this_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "host_list_merge");
   
   if (this_list != NULL) {
      const lListElem *global_host;

      /* we merge global settings into host settings */
      global_host = lGetElemHost(this_list, EH_name, SGE_GLOBAL_NAME);
      if (global_host != NULL) {
         lListElem *host;

         /* do merge for all hosts except global */
         for_each (host, this_list) {
            if (host != global_host) {
               /* on error continue, but return error status */
               if (!host_merge(host, global_host)) {
                  ret = false;
               }
            }
         }
      }
   }
   
   DEXIT;
   return ret;
}

/****** sgeobj/host/host_merge() **********************************************
*  NAME
*     host_merge() -- merge global host settings into an exec host
*
*  SYNOPSIS
*     bool 
*     host_merge(lListElem *host, const lListElem *global_host) 
*
*  FUNCTION
*     Merges settings from the global host object into a specific exec host.
*     Use the global settings, if no host specific settings are done.
*     Currently this applies only to the report_variables attribute.
*
*  INPUTS
*     lListElem *host              - the host object to hold the merged config
*     const lListElem *global_host - the global host object
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*     MT-NOTE: host_merge() is MT safe 
*
*  SEE ALSO
*     sgeobj/host/host_list_merge()
*******************************************************************************/
bool 
host_merge(lListElem *host, const lListElem *global_host)
{
   bool ret = true;

   DENTER(TOP_LAYER, "host_merge");

   if (host != NULL && global_host != NULL) {
      const lList *local_list = lGetList(host, EH_report_variables);

      /* if we have a local list: use this one */
      if (local_list != NULL && lGetNumberOfElem(local_list) != 0) {
         lSetList(host, EH_merged_report_variables, 
                  lCopyList("", local_list));
      } else {
         const lList *global_list;
      
         global_list = lGetList(global_host, EH_report_variables);
         /* if we have no local list, but a global one, use this one */
         if (global_list != NULL && lGetNumberOfElem(global_list) != 0) {
            lSetList(host, EH_merged_report_variables, 
                     lCopyList("", global_list));
         } else {
            /* if no report variables are configured in local and global object,
             * delete the merged list.
             */
            lSetList(host, EH_merged_report_variables, NULL);
         }
      }
   }
   
   DEXIT;
   return ret;
}

