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
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"

#include "sge_attr.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define QINSTANCE_LAYER TOP_LAYER

lListElem * 
qinstance_create(const lListElem *cqueue, lList **answer_list,
                 const char *hostname) 
{
   lListElem *ret = NULL;
   int index;

   DENTER(QINSTANCE_LAYER, "qinstance_create");
   
   ret = lCreateElem(QI_Type);
   lSetHost(ret, QI_hostname, hostname);
   lSetString(ret, QI_name, lGetString(cqueue, CQ_name));

   index = 0;
   while (cqueue_attribute_array[index].cqueue_attr != NoName) {
      qinstance_modify(ret, answer_list, cqueue, 
                       cqueue_attribute_array[index].qinstance_attr,
                       cqueue_attribute_array[index].cqueue_attr, 
                       cqueue_attribute_array[index].href_attr,
                       cqueue_attribute_array[index].value_attr,
                       cqueue_attribute_array[index].primary_key_attr);
      index++;
   }

   DEXIT;
   return ret;
}

bool
qinstance_modify(lListElem *this_elem, lList **answer_list,
                 const lListElem *cqueue, 
                 int attribute_name, 
                 int cqueue_attibute_name,
                 int sub_host_name, int sub_value_name,
                 int subsub_key)
{
   bool ret = true;
   
   DENTER(QINSTANCE_LAYER, "qinstance_modify");

   if (this_elem != NULL && cqueue != NULL && 
       attribute_name != NoName && cqueue_attibute_name != NoName) {
      const char *hostname = lGetHost(this_elem, QI_hostname);
      const lList *attr_list = lGetList(cqueue, cqueue_attibute_name);
      const lDescr *descr = lGetElemDescr(this_elem);
      int pos = lGetPosInDescr(descr, attribute_name);
      int type = lGetPosType(descr, pos);
      bool value_found = true;
      u_long32 ulong32_value;
      const char *str_value = NULL;
      lList *list_value = NULL;
      bool bool_value;

      switch (cqueue_attibute_name) {
         case CQ_qtype:
            qtlist_attr_list_find_value(attr_list, answer_list, 
                                        hostname, &ulong32_value);
            lSetUlong(this_elem, attribute_name, ulong32_value);
            break;
         case CQ_s_fsize:
         case CQ_h_fsize:
         case CQ_s_data:
         case CQ_h_data:
         case CQ_s_stack:
         case CQ_h_stack:
         case CQ_s_core:
         case CQ_h_core:
         case CQ_s_rss:
         case CQ_h_rss:
         case CQ_s_vmem:
         case CQ_h_vmem:
            mem_attr_list_find_value(attr_list, answer_list, 
                                     hostname, &str_value);
            lSetString(this_elem, attribute_name, str_value);
            break;
         case CQ_s_rt:
         case CQ_h_rt:
         case CQ_s_cpu:
         case CQ_h_cpu:
            time_attr_list_find_value(attr_list, answer_list, 
                                      hostname, &str_value);
            lSetString(this_elem, attribute_name, str_value);
            break;
         case CQ_suspend_interval:
         case CQ_min_cpu_interval:
         case CQ_notify:
            inter_attr_list_find_value(attr_list, answer_list, 
                                       hostname, &str_value);
            lSetString(this_elem, attribute_name, str_value);
            break;
         case CQ_ckpt_list:
         case CQ_pe_list:
            strlist_attr_list_find_value(attr_list, answer_list,
                                         hostname, &list_value);

            lSetList(this_elem, attribute_name, list_value);
            break;
         case CQ_owner_list:
         case CQ_acl:
         case CQ_xacl:
            usrlist_attr_list_find_value(attr_list, answer_list,
                                         hostname, &list_value);

            lSetList(this_elem, attribute_name, list_value);
            break;
         case CQ_projects:
         case CQ_xprojects:
            prjlist_attr_list_find_value(attr_list, answer_list,
                                         hostname, &list_value);

            lSetList(this_elem, attribute_name, list_value);
            break;
         case CQ_consumable_config_list:
         case CQ_load_thresholds:
         case CQ_suspend_thresholds:
            celist_attr_list_find_value(attr_list, answer_list,
                                         hostname, &list_value);

            lSetList(this_elem, attribute_name, list_value);
            break;
         case CQ_subordinate_list:
            solist_attr_list_find_value(attr_list, answer_list,
                                         hostname, &list_value);

            lSetList(this_elem, attribute_name, list_value);
            break;
         default:
            value_found = false;
            break;
      }

      if (!value_found) {
         switch (type) {
            case lStringT:
               str_attr_list_find_value(attr_list, answer_list,
                                        hostname, &str_value);
               lSetString(this_elem, attribute_name, str_value);
               break;
            case lUlongT:
               ulng_attr_list_find_value(attr_list, answer_list, 
                                         hostname, &ulong32_value);

               lSetUlong(this_elem, attribute_name, ulong32_value);
               break;
            case lBoolT:
               bool_attr_list_find_value(attr_list, answer_list,
                                         hostname, &bool_value);
               lSetBool(this_elem, attribute_name, bool_value);
               break;
            default:
               DPRINTF(("unhandled attribute\n"));
               break;
         }
      }
   }
   DEXIT;
   return ret;
}


