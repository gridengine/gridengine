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

#include "sge_gdi.h"

#include "sge_object.h"
#include "sge_answer.h"
#include "sge_attr.h"
#include "sge_centry.h"
#include "sge_cqueue.h"
#include "sge_queue.h"
#include "sge_stringL.h"
#include "sge_userprj.h"
#include "sge_userset.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CQUEUE_LAYER TOP_LAYER

typedef struct _list_attribute_struct {
   int cqueue_attr;
   bool is_sublist_type;
   int href_attr;
   int value_attr;
   int primary_key_attr;
   const char *name;
} list_attribute_struct;

/* *INDENT-OFF* */

static list_attribute_struct array[] = {
   { CQ_consumable_config_list,  true,  ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_COMPLEX_VALUES    },
   { CQ_load_thresholds,         true,  ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_LOAD_THRESHOLD    },
   { CQ_suspend_thresholds,      true,  ACELIST_href,  ACELIST_value,    CE_name,    SGE_ATTR_SUSPEND_THRESHOLD },
   { CQ_projects,                true,  APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_PROJECTS          },
   { CQ_xprojects,               true,  APRJLIST_href, APRJLIST_value,   UP_name,    SGE_ATTR_XPROJECTS         },
   { CQ_acl,                     true,  AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_USER_LISTS        },
   { CQ_xacl,                    true,  AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_XUSER_LISTS       },
   { CQ_owner_list,              true,  AUSRLIST_href, AUSRLIST_value,   US_name,    SGE_ATTR_OWNER_LIST        },
   { CQ_ckpt_list,               true,  ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_CKPT_LIST         },
   { CQ_pe_list,                 true,  ASTRLIST_href, ASTRLIST_value,   ST_name,    SGE_ATTR_PE_LIST           },
   { CQ_subordinate_list,        true,  ASOLIST_href,  ASOLIST_value,    SO_qname,   SGE_ATTR_SUBORDINATE_LIST  },

   { NoName,                     false, NoName,        NoName,           NoName,     NULL                       }
};

/* *INDENT-ON* */

lList *Master_CQueue_List = NULL;

static int
cqueue_attr_get_pos(int cqueue_attr);

static int
cqueue_attr_get_pos(int cqueue_attr) 
{
   int index = 0;

   while (array[index].cqueue_attr != NoName && 
          array[index].cqueue_attr != cqueue_attr) {
      index++; 
   } 
   return index;
}

bool 
cqueue_attr_is_sublist_type(int cqueue_attr)
{
   return array[cqueue_attr_get_pos(cqueue_attr)].is_sublist_type;
}

int 
cqueue_attr_get_href_attr(int cqueue_attr)
{
fprintf(stderr, "##### %d\n", cqueue_attr_get_pos(cqueue_attr));
   return array[cqueue_attr_get_pos(cqueue_attr)].href_attr;
}

int 
cqueue_attr_get_value_attr(int cqueue_attr)
{
   return array[cqueue_attr_get_pos(cqueue_attr)].value_attr;
}

int
cqueue_attr_get_primary_key_attr(int cqueue_attr)
{
   return array[cqueue_attr_get_pos(cqueue_attr)].primary_key_attr;
}

const char*
cqueue_attr_get_name(int cqueue_attr) 
{
   return array[cqueue_attr_get_pos(cqueue_attr)].name;
}

lList **cqueue_list_get_master_list(void)
{
   return &Master_CQueue_List;
}

lListElem *
cqueue_create(lList **answer_list, const char *name)
{
   lListElem *ret = NULL;

   DENTER(CQUEUE_LAYER, "cuser_create");
   if (name != NULL) {
      ret = lCreateElem(CQ_Type);

      if (ret != NULL) {
         lSetString(ret, CQ_name, name);
      } else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, 
                                MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC));
         answer_list_add(answer_list, SGE_EVENT,
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      }
   }
   DEXIT;
   return ret;
}

bool
cqueue_list_add_cqueue(lListElem *queue)
{
   bool ret = false;
   static lSortOrder *so = NULL;

   DENTER(TOP_LAYER, "cqueue_list_add_cqueue");

   if (queue != NULL) {
      if (so == NULL) {
         so = lParseSortOrderVarArg(CQ_Type, "%I+", CQ_name);
      }

      if (Master_CQueue_List == NULL) {
         Master_CQueue_List = lCreateList("", CQ_Type);
      }

      lInsertSorted(so, queue, Master_CQueue_List);
      ret = true;
   } 
   DEXIT;
   return ret;
}

lListElem *
cqueue_list_locate(const lList *this_list, const char *name)
{
   return lGetElemStr(this_list, CQ_name, name);
}


bool
cqueue_mod_sublist(lListElem *this_elem, lList **answer_list,
                   lListElem *reduced_elem, int sub_command,
                   int attribute_name, int sublist_host_name,
                   int sublist_value_name, int subsub_key,
                   const char *attribute_name_str, 
                   const char *object_name_str) 
{
   bool ret = true;
   int pos;

   DENTER(TOP_LAYER, "cqueue_mod_sublist");
  
   pos = lGetPosViaElem(reduced_elem, attribute_name);
   if (pos >= 0) {
      lList *mod_list = lGetPosList(reduced_elem, pos);
      lList *org_list = lGetList(this_elem, attribute_name);
      lList *new_org_list = NULL;
      lListElem *mod_elem;

      /* 
       * Delete all configuration lists except the default-configuration
       */
      if (sub_command == SGE_GDI_SET_ALL) {
         lListElem *elem, *next_elem;

         next_elem = lFirst(org_list);
         while ((elem = next_elem)) {
            const char *name = lGetHost(elem, sublist_host_name);

            next_elem = lNext(elem); 
            mod_elem = lGetElemHost(mod_list, sublist_host_name, name);
            if (mod_elem == NULL) {
               const char *name = lGetHost(elem, sublist_host_name);

               DPRINTF(("Removing attribute list for "SFQ"\n", name));
               lRemoveElem(org_list, elem);
            }
         }
      }

      /*
       * Do modifications for all given domain/host-configuration list
       */
      for_each(mod_elem, mod_list) {
         const char *name = lGetHost(mod_elem, sublist_host_name);
         lListElem *org_elem = lGetElemHost(org_list, sublist_host_name, name);

         /*
          * Create default-element if it does not exist
          */
         if (org_elem == NULL && 
             (!strcmp(name, HOSTREF_DEFAULT) || sub_command == SGE_GDI_SET_ALL)) {
            if (org_list == NULL) {
               DTRACE;
               org_list = lCreateList("", lGetElemDescr(mod_elem));
               lSetList(this_elem, attribute_name, org_list);
            } 
            DTRACE;
            org_elem = lCreateElem(lGetElemDescr(mod_elem));
            lSetHost(org_elem, sublist_host_name, name);
            lAppendElem(org_list, org_elem);
         }

         /*
          * Modify sublist according to subcommand
          */
         if (org_elem != NULL) {
            DTRACE;
            attr_mod_sub_list(answer_list, org_elem, sublist_value_name, 
                              subsub_key, mod_elem, sub_command, 
                              attribute_name_str, object_name_str, 0);
         }
      }
DTRACE;
lWriteListTo(mod_list, stderr);
DTRACE;
lWriteListTo(org_list, stderr);
   }
 
   DEXIT;
   return ret;
}

