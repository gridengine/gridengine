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
#include <stdio.h>
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "valid_queue_user.h"
#include "sge_string.h"
#include "sge_answer.h"

#include "sge_queueL.h"
#include "sge_usersetL.h"
#include "sge_gdi_intern.h"
#include "msg_schedd.h"

static int sge_contained_in_access_list_(const char *user, const char *group, 
                                         lList *acl, lList *acl_list);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

   sge_has_access - determines if a user/group has access to a queue

   returns
      1 for true
      0 for false

*/
int sge_has_access(const char *user, const char *group, lListElem *q,
                   lList *acl_list) 
{
   return sge_has_access_(user, group, 
         lGetList(q, QU_acl), lGetList(q, QU_xacl), acl_list);
}

/* needed to test also sge_queue_type structures without converting the
** whole queue
*/
int sge_has_access_(const char *user, const char *group, lList *q_acl,
                    lList *q_xacl, lList *acl_list) 
{
   int ret;

   DENTER(TOP_LAYER, "sge_has_access_");

   ret = sge_contained_in_access_list_(user, group, q_xacl, acl_list);
   if (ret<0 || ret == 1) { /* also deny when an xacl entry was not found in acl_list */
      DEXIT;
      return 0;
   }

   if (!q_acl) {  /* no entry in allowance list - ok everyone */
       DEXIT;
       return 1;
   }

   ret = sge_contained_in_access_list_(user, group, q_acl, acl_list);
   if (ret<0) {
      DEXIT;
      return 0;
   }
   if (ret) {
      /* we're explicitly allowed to access */
      DEXIT;
      return 1;
   }

   /* there is an allowance list but the owner isn't there -> denied */
   DEXIT;
   return 0;
} 


/* sge_contained_in_access_list_() returns 
   1  yes it is contained in the acl
   0  no 
   -1 got NULL for user/group 

   user, group: may be NULL
*/
static int sge_contained_in_access_list_(const char *user, const char *group,
                                         lList *acl, lList *acl_list) 
{
   lListElem *acl_search, *acl_found;

   DENTER(TOP_LAYER,"sge_contained_in_access_list_");

   for_each (acl_search, acl) {
      if ((acl_found=lGetElemStr(acl_list, US_name,
            lGetString(acl_search,US_name)))) {
         /* ok - there is such an access list */
         if (sge_contained_in_access_list(user, group, acl_found, NULL)) {
            DEXIT;
            return 1;
         } 
      } else {
      	DPRINTF(("cannot find userset list entry \"%s\"\n", 
		         lGetString(acl_search,US_name)));
      }
   }

   DEXIT;
   return 0;
}

/* sge_contained_in_access_list() returns 
   1  yes it is contained in the acl
   0  no 

   user, group: may be NULL
*/   
int sge_contained_in_access_list(const char *user, const char *group, 
                                 lListElem *acl, lList **alpp) 
{
   const char *entry_name;
   lListElem *acl_entry;
   DENTER(TOP_LAYER,"sge_contained_in_access_list");
   for_each (acl_entry, lGetList(acl, US_entries)) {
      entry_name = lGetString(acl_entry,UE_name);
      if (!entry_name)
          continue;
      if (entry_name[0] == '@') {
         if (group && !strcmp(&entry_name[1], group)) {
            if (alpp) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_VALIDQUEUEUSER_GRPXALLREADYINUSERSETY_SS, group, lGetString(acl, US_name)));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            }
            DEXIT;
            return 1;
         }
      } else {
         if (user && !strcmp(entry_name, user)) {
            if (alpp) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_VALIDQUEUEUSER_USRXALLREADYINUSERSETY_SS, user, lGetString(acl, US_name)));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
            }
            DEXIT;
            return 1;
         }
      }
   }
   DEXIT;
   return 0;
}
