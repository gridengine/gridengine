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

#include <fnmatch.h>

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge.h"

#include "sge_dstring.h"
#include "sge_signal.h"

#include "sge_answer.h"
#include "sge_object.h"
#include "sge_cqueue.h"
#include "sge_event.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_qref.h"
#include "sge_subordinate.h"

#include "msg_sgeobjlib.h"

/* -----------------------------------------------

   test suspend on subordinate (nice neuron)

    A1            C1
      \          /
       \        /
        \      /
  A2--------->B-------> C2
        /      \
       /        \
      /          \
    A3            C3

   a queue C subordinated by B must be suspended if
   the used slots of queue B meet the thresold for C

*/
/*
int used,      umber of slots actually used in queue B    
int total,     total number of slots in queue B              
lListElem *so  SO_Type referencing to a queue C              
*/
int
tst_sos(int used, int total, lListElem *so)
{
   u_long32 threshold;

   DENTER(TOP_LAYER, "tst_sos");

   /*
      then check if B's usage meets the threshold
      for suspension of the subordinated queue C
   */
   if (!(threshold=lGetUlong(so, SO_threshold))) {
      /* queue must be full for suspend of queue C */
      DPRINTF(("TSTSOS: %sfull -> %ssuspended\n", (used>=total)?"":"not ",
         (used>=total)?"":"not "));
      DEXIT;
         return (used>=total);
   }

   /* used slots greater or equal threshold */
   DPRINTF(("TSTSOS: "u32" slots used (limit "u32") -> %ssuspended\n",
      used, threshold, ( (u_long32)(used) >= threshold)?"":"not "));
   DEXIT;
   return ( (u_long32) (used) >= threshold);
}

const char *
so_list_append_to_dstring(const lList *this_list, dstring *string)
{
   const char *ret = NULL;

   DENTER(BASIS_LAYER, "so_list_append_to_dstring");
   if (string != NULL) {
      lListElem *elem = NULL;
      bool printed = false;

      for_each(elem, this_list) {
         sge_dstring_sprintf_append(string, "%s", lGetString(elem, SO_name));
         if (lGetUlong(elem, SO_threshold)) {
            sge_dstring_sprintf_append(string, "="u32"%s",
                                       lGetUlong(elem, SO_threshold),
                                       lNext(elem) ? "," : "");
         }
         printed = true;
      }
      if (!printed) {
         sge_dstring_sprintf_append(string, "NONE");
      }
      ret = sge_dstring_get_string(string);
   }
   DEXIT;
   return ret;
}

/*
   add new so elements
   if element already exists then possibly overwrite threshold value
   (lower values are prefered)
*/
bool
so_list_add(lList **this_list, lList **answer_list, const char *so_name,
            u_long32 threshold)
{
   bool ret = true;
      
   DENTER(TOP_LAYER, "sol_list_add");
   if (this_list != NULL && so_name != NULL) {
      lListElem *elem = lGetElemStr(*this_list, SO_name, so_name);
   
      if (elem != NULL) {
         u_long32 current_threshold = lGetUlong(elem, SO_threshold);

         if (threshold != 0 && threshold < current_threshold) {
            lSetUlong(elem, SO_threshold, threshold);
         }
      } else {
         elem = lAddElemStr(this_list, SO_name, so_name, SO_Type);
         lSetUlong(elem, SO_threshold, threshold);
      }
   }
   return ret;
}

bool
so_list_resolve(const lList *so_list, lList **answer_list,
                lList **resolved_so_list, const char *full_name)
{
   bool ret = true;

   DENTER(TOP_LAYER, "so_list_resolve");
   if (so_list != NULL) {
      lListElem *so;

      for_each(so, so_list) {
         lList *queue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
         lList *hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));
         const char *so_name = lGetString(so, SO_name);
         lList *qref_list = NULL;
         lList *resolved_qref_list = NULL;
         bool found_something = false;

         ret &= qref_list_add(&qref_list, answer_list, so_name);
         ret &= qref_list_resolve(qref_list, answer_list, &resolved_qref_list,
                                  &found_something, queue_list, hgroup_list,
                                  true, true);
         ret &= qref_list_trash_some_elemts(&resolved_qref_list, full_name);
         if (ret) {  
            lListElem *resolved_qref = NULL;
            u_long32 threshold = lGetUlong(so, SO_threshold);

            for_each(resolved_qref, resolved_qref_list) {
               const char *qref_name = lGetString(resolved_qref, QR_name);

               ret &= so_list_add(resolved_so_list, answer_list,
                                  qref_name, threshold); 
               if (!ret) {
                  break;
               }
            } 
         }
         qref_list = lFreeList(qref_list);
         resolved_qref_list = lFreeList(resolved_qref_list);
      }
   }
   DEXIT;
   return ret;
}
