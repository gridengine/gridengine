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
#include "sge_usage.h"

/****** sgeobj/usage/usage_list_get_ulong_usage() *****************************
*  NAME
*     usage_list_get_ulong_usage() -- return ulong usage value
*
*  SYNOPSIS
*     u_long32 
*     usage_list_get_ulong_usage(lList *usage_list, const char *name, 
*                                u_long32 def) 
*
*  FUNCTION
*     Searches a usage object with the given name in the given usage 
*     list. If such an element is found, returns the value of the 
*     usage object as u_long32 value.
*     If no such element is found, return the given default value.
*
*  INPUTS
*     lList *usage_list - the usage list
*     const char *name  - name of the element to search
*     u_long32 def      - default value
*
*  RESULT
*     u_long32 - value of found object or default
*
*  SEE ALSO
*     gdi/usage/usage_list_get_double_usage()
*******************************************************************************/
u_long32 usage_list_get_ulong_usage (lList *usage_list, 
                                     const char *name, 
                                     u_long32 def)
{
   lListElem *ep = lGetElemStr(usage_list, UA_name, name);
   if(ep != NULL) {
      return (u_long32)lGetDouble(ep, UA_value);
   } else {
      return def;
   }
}

/****** sgeobj/usage/usage_list_get_double_usage() ****************************
*  NAME
*     usage_list_get_double_usage() -- return double usage value
*
*  SYNOPSIS
*     u_long32 
*     usage_list_get_double_usage(lList *usage_list, const char *name, 
*                                 double def) 
*
*  FUNCTION
*     Searches a usage object with the given name in the given usage 
*     list. If such an element is found, returns the value of the 
*     usage object as double value.
*     If no such element is found, return the given default value.
*
*  INPUTS
*     lList *usage_list - the usage list
*     const char *name  - name of the element to search
*     double def        - default value
*
*  RESULT
*     double - value of found object or default
*
*  SEE ALSO
*     gdi/usage/usage_list_get_ulong_usage()
*******************************************************************************/
double usage_list_get_double_usage(lList *usage_list, const char *name, 
                                   double def)
{
   lListElem *ep = lGetElemStr(usage_list, UA_name, name);
   if(ep != NULL) {
      return lGetDouble(ep, UA_value);
   } else {
      return def;
   }
}

/****** sgeobj/usage/usage_list_set_ulong_usage() ******************************
*  NAME
*     usage_list_set_ulong_usage() -- create/update a usage record
*
*  SYNOPSIS
*     void
*     usage_list_set_ulong_usage(lList *usage_list, const char *name, 
*                                u_long32 value) 
*
*  FUNCTION
*     Updates the value of a usage record. If no usage record exists with the
*     given name in usage_list, a new record is created.
*
*  INPUTS
*     lList *usage_list - list containing the usage record to update
*     const char *name  - name of the usage record to update
*     u_long32 value    - the new value
*
*  NOTES
*     MT-NOTE: usage_list_set_ulong_usage() is MT safe 
*
*  SEE ALSO
*     sgeobj/usage/usage_list_set_double_usage()
*     sgeobj/usage/usage_list_get_ulong_usage()
*     sgeobj/usage/usage_list_get_double_usage()
*******************************************************************************/
void
usage_list_set_ulong_usage(lList *usage_list, const char *name, u_long32 value)
{
   usage_list_set_double_usage(usage_list, name, value);
}

/****** sgeobj/usage/usage_list_set_double_usage() ******************************
*  NAME
*     usage_list_set_double_usage() -- create/update a usage record
*
*  SYNOPSIS
*     void
*     usage_list_set_double_usage(lList *usage_list, const char *name, 
*                                 double value) 
*
*  FUNCTION
*     Updates the value of a usage record. If no usage record exists with the
*     given name in usage_list, a new record is created.
*
*  INPUTS
*     lList *usage_list - list containing the usage record to update
*     const char *name  - name of the usage record to update
*     double value      - the new value
*
*  NOTES
*     MT-NOTE: usage_list_set_double_usage() is MT safe 
*
*  SEE ALSO
*     sgeobj/usage/usage_list_set_ulong_usage()
*     sgeobj/usage/usage_list_get_ulong_usage()
*     sgeobj/usage/usage_list_get_double_usage()
*******************************************************************************/
void
usage_list_set_double_usage(lList *usage_list, const char *name, double value)
{
   lListElem *ep = lGetElemStr(usage_list, UA_name, name);
   if (ep == NULL) {
      ep = lAddElemStr(&usage_list, UA_name, name, UA_Type);
   }

   lSetDouble(ep, UA_value, value);
}

