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
#include "sge_qinstance.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define QINSTANCE_LAYER TOP_LAYER

lListElem * 
qinstance_create(const lListElem *cqueue, const char *hostname) 
{
   lListElem *ret = NULL;

   DENTER(QINSTANCE_LAYER, "qinstance_create");
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
      const char *hostname = lGetString(this_elem, QI_hostname);
      const lList *attr_list = lGetList(this_elem, cqueue_attibute_name);
      const lDescr *descr = lGetElemDescr(this_elem);
      int pos = lGetPosInDescr(descr, attribute_name);
      int type = lGetPosType(descr, pos);

      switch (type) {
         case lUlongT:
            {
               u_long32 value;

               ulng_attr_list_find_value(attr_list, answer_list, 
                                         hostname, &value);

DPRINTF(("##### "u32"\n", value));
            }                                                
            break;
         default:
            DPRINTF(("Attribute %d not handled in qinstance_modify()\n",
                     attribute_name));
            break;
      }
   }
   DEXIT;
   return ret;
}


