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
#include "sge_log.h"
#include "def.h"   
#include "cull_list.h"

#include "sge_pe.h"

lList *Master_Pe_List = NULL;

/****** gdi/pe/pe_match() *****************************************************
*  NAME
*     pe_match() -- Find a PE matching a wildcard expression 
*
*  SYNOPSIS
*     lListElem* pe_match(const char *wildcard) 
*
*  FUNCTION
*     Try to find a PE that matches the given "wildcard" expression.
*
*  INPUTS
*     const char *wildcard - Wildcard expression 
*
*  RESULT
*     lListElem* - PE_Type object or NULL
*******************************************************************************/
lListElem *pe_match(const char *wildcard) 
{
   lListElem *pep;

   for_each (pep, Master_Pe_List) {
      if (!fnmatch(wildcard, lGetString(pep, PE_name), 0)) {
         return pep;
      }
   }
   return NULL;
}

/****** gdi/pe/pe_locate() ****************************************************
*  NAME
*     pe_locate() -- Locate a certain PE 
*
*  SYNOPSIS
*     lListElem* pe_locate(const char *pe_name) 
*
*  FUNCTION
*     Locate the PE with the name "pe_name". 
*
*  INPUTS
*     const char *pe_name - PE name 
*
*  RESULT
*     lListElem* - PE_Type object or NULL
*******************************************************************************/
lListElem *pe_locate(const char *pe_name) 
{
   return lGetElemStr(Master_Pe_List, PE_name, pe_name);
}

