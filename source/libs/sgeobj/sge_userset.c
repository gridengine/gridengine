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
#include <stdlib.h>
#include <errno.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "parse.h"
#include "sge_utility.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#include "sge_userset.h"

lList *Master_Userset_List = NULL;

static const char* userset_types[] = {
   "ACL",   /* US_ACL   */
   "DEPT",  /* US_DEPT  */
   ""
};

lList **userset_list_get_master_list(void)
{
   return &Master_Userset_List;
}

/****** sgeobj/userset/userset_is_deadline_user() ******************************
*  NAME
*     userset_is_deadline_user() -- may user sumbit deadline jobs. 
*
*  SYNOPSIS
*     bool userset_is_deadline_user(lList *lp, const char *username) 
*
*  FUNCTION
*     Ask whether a given user is allowed to sumbit deadline jobs. 
*
*  INPUTS
*     lList *lp            - US_Type 
*     const char *username - user name
*
*  RESULT
*     bool - result 
*******************************************************************************/
bool userset_is_deadline_user(lList *lp, const char *username)
{
   lListElem *deadline_users;

   DENTER(TOP_LAYER, "userset_is_deadline_user");

   deadline_users = lGetElemStr(lp, US_name, DEADLINE_USERS);

   if (deadline_users && lGetSubStr(deadline_users, UE_name, username, 
         US_entries)) {
      DEXIT;
      return 1; /* found user in deadline user list */
   }

   DEXIT;
   return 0;
}

/****** sgeobj/userset/userset_list_locate() **********************************
*  NAME
*     userset_list_locate() -- Find user in list 
*
*  SYNOPSIS
*     lListElem* userset_list_locate(lList *lp, const char *name) 
*
*  FUNCTION
*     Find user in list. 
*
*  INPUTS
*     lList *lp        - US_Type list 
*     const char *name - name 
*
*  RESULT
*     lListElem* - NULL or element pointer
*******************************************************************************/
lListElem *userset_list_locate(lList *lp, const char *name) 
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "userset_list_locate");

   for_each(ep, lp) {
      if (!strcasecmp(name, lGetString(ep, US_name))) {
         break; 
      }
   }

   DEXIT;
   return ep;
}

/****** sgeobj/userset/userset_list_validate_acl_list() ***********************
*  NAME
*     userset_list_validate_acl_list() -- validate an acl list 
*
*  SYNOPSIS
*     int 
*     userset_list_validate_acl_list(lList **alpp, lList *acl_list, 
*                                    const char *attr_name, 
*                                    const char *obj_descr, 
*                                    const char *obj_name) 
*
*  FUNCTION
*     Checks if all entries of an acl list (e.g. user list of a pe) 
*     are contained in the master userset list.
*
*  INPUTS
*     lList **alpp          - answer list pointer
*     lList *acl_list       - the acl list to check
*     const char *attr_name - the attribute name in the referencing 
*                             object (e.g. "user_lists")
*     const char *obj_descr - the descriptor of the referencing 
*                             object (e.g. "queue")
*     const char *obj_name  - the name of the referencing object
*                             (e.g. "fangorn.q")
*
*  RESULT
*     int - STATUS_OK, if everything is OK
*******************************************************************************/
int 
userset_list_validate_acl_list(lList **alpp, lList *acl_list, 
                               const char *attr_name, const char *obj_descr, 
                               const char *obj_name) 
{
   lListElem *usp;

   DENTER(TOP_LAYER, "userset_list_validate_acl_list");

   for_each (usp, acl_list) {
      if (!lGetElemStr(Master_Userset_List, US_name, lGetString(usp, US_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNUSERSET_SSSS, lGetString(usp, US_name), 
               attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

/****** sgeobj/userset/userset_validate_entries() *******************************
*  NAME
*     userset_validate_entries() -- verify entries of a user set
*
*  SYNOPSIS
*     int userset_validate_entries(lListElem *userset, lList **alpp, 
*                                  int start_up) 
*
*  FUNCTION
*     Validates all entries of a userset.
*
*  INPUTS
*     lListElem *userset - the userset to check
*     lList **alpp       - answer list pointer, if answer is expected. 
*                          In any case, errors are output using the 
*                          ERROR macro.
*     int start_up       - are we in the qmaster startup phase?
*
*  RESULT
*     int - STATUS_OK, if everything is OK
*******************************************************************************/
int userset_validate_entries(lListElem *userset, lList **alpp, int start_up)
{
   lListElem *ep;
   int name_pos;

   DENTER(TOP_LAYER, "userset_validate_entries");

   /*
      resolve cull names to positions
      for faster access in loop
   */

   name_pos = lGetPosInDescr(UE_Type, UE_name);

   for_each(ep, lGetList(userset, US_entries)) {
      if (!lGetPosString(ep, name_pos)) {
         ERROR((SGE_EVENT, MSG_US_INVALIDUSERNAME));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, 
                         ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_ESEMANTIC;
      }
   }

   DEXIT;
   return STATUS_OK;
}

/****** sgeobj/userset/userset_get_type_string() **********************************
*  NAME
*     userset_get_type_string() -- get readable type definition
*
*  SYNOPSIS
*     const char* 
*     userset_get_type_string(const lListElem *userset, lList **answer_list,
*                           dstring *buffer) 
*
*  FUNCTION
*     Returns a readable string of the userset type bitfield.
*
*  INPUTS
*     const lListElem *userset - the userset containing the requested 
*                                information
*     dstring *buffer          - string buffer to hold the result string
*
*  RESULT
*     const char* - resulting string
*
*  SEE ALSO
*     sgeobj/userset/userset_set_type_string()
*******************************************************************************/
const char *
userset_get_type_string(const lListElem *userset, lList **answer_list, 
                        dstring *buffer)
{
   u_long32 type;
   int i;
   bool append = false;
   const char *ret;

   DENTER(TOP_LAYER, "userset_get_type_string");

   
   SGE_CHECK_POINTER_NULL(userset);
   SGE_CHECK_POINTER_NULL(buffer);

   type = lGetUlong(userset, US_type);
   sge_dstring_clear(buffer);

   for (i = 0; userset_types[i] != NULL; i++) {
      if ((type & (1 << i)) != 0) {
         if (append) {
            sge_dstring_append(buffer, " ");
         }
         sge_dstring_append(buffer, userset_types[i]);
         append = true;
      }
   }

   ret = sge_dstring_get_string(buffer);
   DEXIT;
   return ret;
}

/****** sgeobj/userset/userset_set_type_string() ******************************
*  NAME
*     userset_set_type_string() -- set userset type from string 
*
*  SYNOPSIS
*     bool 
*     userset_set_type_string(lListElem *userset, lList **answer_list, 
*                           const char *value) 
*
*  FUNCTION
*     Takes a string representation for the userset type, 
*
*  INPUTS
*     lListElem *userset    - the userset to change
*     lList **answer_list - errors will be reported here
*     const char *value   - new value for userset type
*
*  RESULT
*     bool - true on success, 
*            false on error, error message will be in answer_list
*
*  SEE ALSO
*     sgeobj/userset/userset_get_type_string()
******************************************************************************/
bool 
userset_set_type_string(lListElem *userset, lList **answer_list, 
                        const char *value)
{
   bool ret = true;
   u_long32 type = 0;
 
   DENTER(TOP_LAYER, "userset_set_type_string");

   SGE_CHECK_POINTER_FALSE(userset);

   if (value != NULL && *value != 0) {
      if (!sge_parse_bitfield_str(value, userset_types, &type, 
                                  "userset type", NULL, false)) {
         ret = false;
      }
   }

   lSetUlong(userset, US_type, type);

   DEXIT;
   return ret;
}

const char *
userset_list_append_to_dstring(const lList *this_list, dstring *string)
{
   const char *ret = NULL;

   DENTER(BASIS_LAYER, "userset_list_append_to_dstring");
   if (string != NULL) {
      lListElem *elem = NULL;
      bool printed = false;

      for_each(elem, this_list) {
         sge_dstring_sprintf_append(string, "%s", lGetString(elem, US_name));
         if (lNext(elem)) {
            sge_dstring_sprintf_append(string, " ");
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

