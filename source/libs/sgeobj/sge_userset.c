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
#include "sge_userset.h"
#include "sge_answer.h"

#include "msg_sgeobjlib.h"


lList *Master_Userset_List = NULL;

/*****************************************************************
 is_deadline_user

 ask whether a given user is allowed to sumbit deadline jobs.
 *****************************************************************/
int is_deadline_user(
const char *username,   /* user we ask for */
lList *lp               /* userset list to scan for deadline users */
) {
   lListElem *deadline_users;

   DENTER(TOP_LAYER, "is_deadline_user");

   deadline_users = lGetElemStr(lp, US_name, DEADLINE_USERS);

   if (deadline_users && lGetSubStr(deadline_users, UE_name, username, 
         US_entries)) {
      DEXIT;
      return 1; /* found user in deadline user list */
   }

   DEXIT;
   return 0;
}

lListElem *userset_list_locate(lList *lp, const char *name) 
{
   lListElem *ep;

   DENTER(TOP_LAYER, "userset_list_locate");

   for_each(ep, lp)
      if (!strcasecmp(name, lGetString(ep, US_name))) {
         DEXIT;
         return ep;
      }

   DEXIT;
   return NULL;
}

/****** gdi/userset/userset_list_validate_acl_list() ***************************
*  NAME
*     userset_list_validate_acl_list() -- validate an acl list 
*
*  SYNOPSIS
*     int userset_list_validate_acl_list(lList **alpp, lList *acl_list, 
*                                        const char *attr_name, 
*                                        const char *obj_descr, 
*                                        const char *obj_name) 
*
*  FUNCTION
*     Checks if all entries of an acl list (e.g. user list of a pe) are
*     contained in the master userset list.
*
*  INPUTS
*     lList **alpp          - answer list pointer
*     lList *acl_list       - the acl list to check
*     const char *attr_name - the attribute name in the referencing object
*     const char *obj_descr - the descriptor of the referencing object
*     const char *obj_name  - the name of the referencing object
*
*  RESULT
*     int - STATUS_OK, if everything is OK, else another status code,
*           see libs/gdi/sge_answer.h
*
*******************************************************************************/
int userset_list_validate_acl_list(
lList **alpp,
lList *acl_list,
const char *attr_name, /* e.g. "user_lists" */
const char *obj_descr, /* e.g. "queue"      */
const char *obj_name   /* e.g. "fangorn.q"  */
) {
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

/****** gdi/userset/userset_validate_entries() *******************************
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
*     lList **alpp       - answer list pointer, if answer is expected. In any
*                          case, errors are output using the ERROR macro.
*     int start_up       - are we in the qmaster startup phase?
*
*  RESULT
*     int - STATUS_OK, if everything is OK, else another status code,
*           see libs/gdi/sge_answer.h
*
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

