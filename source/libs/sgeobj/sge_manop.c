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

#include "cull.h"
#include "sge_manop.h"

#include "sgermon.h"

lList *Master_Manager_List = NULL;
lList *Master_Operator_List = NULL;

/****** gdi/manop/manop_is_manager() *******************************************
*  NAME
*     manop_is_manager() -- is a certain user manager?
*
*  SYNOPSIS
*     int manop_is_manager(const char *user_name) 
*
*  FUNCTION
*     Checks if the user given by user name is a manager.
*
*  INPUTS
*     const char *user_name - user name
*
*  RESULT
*     int - TRUE, if the user is manager, else FALSE
*
*  NOTES
*     Operator/Manager should be a property of a user.
*     Then the function would be user_is_manager - much more plausible
*
*  SEE ALSO
*     gdi/manop/manop_is_operator()
*******************************************************************************/
int manop_is_manager(const char *user_name) 
{
   DENTER(TOP_LAYER, "manop_is_manager");

   if (user_name == NULL) {
      DEXIT;
      return FALSE;
   }

   if (lGetElemStr(Master_Manager_List, MO_name, user_name) != NULL) {
      DEXIT;
      return TRUE;
   }

   DEXIT;
   return FALSE;

}

/****** gdi/manop/manop_is_operator() ******************************************
*  NAME
*     manop_is_operator() -- is a certain user operator?
*
*  SYNOPSIS
*     int manop_is_operator(const char *user_name) 
*
*  FUNCTION
*     Checks if the user given by user name is a operator.
*     A manager is implicitly also an operator.
*
*  INPUTS
*     const char *user_name - user name
*
*  RESULT
*     int - TRUE, if the user is operator, else FALSE
*
*  NOTES
*     Operator/Manager should be a property of a user.
*     Then the function would be user_is_operator - much more plausible
*
*  SEE ALSO
*     gdi/manop/manop_is_manager()
*******************************************************************************/
int manop_is_operator(const char *user_name) {

   DENTER(TOP_LAYER, "manop_is_operator");

   if (user_name == NULL) {
      DEXIT;
      return FALSE;
   }

   if (lGetElemStr(Master_Operator_List, MO_name, user_name) != NULL) {
      DEXIT;
      return TRUE;
   }

   if (lGetElemStr(Master_Manager_List, MO_name, user_name) != NULL) {
      DEXIT;
      return TRUE;
   }

   DEXIT;
   return FALSE;
}

