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

#include "sge_string.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge_answer.h"
#include "sge_userprj.h"

#include "msg_sgeobjlib.h"

lList *Master_Project_List = NULL;
lList *Master_User_List = NULL;

/****** sgeobj/userprj/userprj_list_locate() **********************************
*  NAME
*     userprj_list_locate() -- Find a user/project in a list 
*
*  SYNOPSIS
*     lListElem* userprj_list_locate(lList *userprj_list, 
*                                    const char *uerprj_name) 
*
*  FUNCTION
*     Find a user/project with the primary key "userprj_name" in
*     the "userprj_list". 
*
*  INPUTS
*     lList *userprj_list     - UP_Type list 
*     const char *uerprj_name - User or project name 
*
*  RESULT
*     lListElem* - pointer to user or project element
******************************************************************************/
lListElem *
userprj_list_locate(const lList *userprj_list, const char *uerprj_name) 
{
   return lGetElemStr(userprj_list, UP_name, uerprj_name);
}

const char *
userprj_list_append_to_dstring(const lList *this_list, dstring *string)
{
   const char *ret = NULL;

   DENTER(BASIS_LAYER, "userprj_list_append_to_dstring");
   if (string != NULL) {
      lListElem *elem = NULL;
      bool printed = false;

      for_each(elem, this_list) {
         sge_dstring_sprintf_append(string, "%s", lGetString(elem, UP_name));
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

bool
prj_list_do_all_exist(const lList *this_list, lList **answer_list,
                      const lList *prj_list)
{
   bool ret = true;
   lListElem *prj = NULL;

   DENTER(TOP_LAYER, "prj_list_do_all_exist");
   for_each(prj, prj_list) {
      const char *name = lGetString(prj, UP_name);

      if (userprj_list_locate(this_list, name) == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EEXIST,
                                 ANSWER_QUALITY_ERROR,
                                 MSG_CQUEUE_UNKNOWNPROJECT_S, name);
         DTRACE;
         ret = false;
         break;
      }
   }
   DEXIT;
   return ret;
}

lList **
prj_list_get_master_list(void)
{
   return &Master_Project_List;
}


