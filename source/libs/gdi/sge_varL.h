#ifndef __SGE_VARL_H
#define __SGE_VARL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/****** gdi/var/--VA_Type *****************************************************
*  NAME
*     VA_Type - CULL variable element
*
*  ELEMENTS
*     SGE_STRING(VA_variable)
*        name of variable
*
*     SGE_STRING(VA_value)
*        value of variable
*
*  FUNCTION
*     CULL element holding a variable/value pair (variable[=value])
*
*  SEE ALSO
*     gdi/var/--VariableList
******************************************************************************/

enum {
   VA_variable = VA_LOWERBOUND,
   VA_value
};

SLISTDEF(VA_Type, Variable)
   SGE_STRINGHU(VA_variable)
   SGE_STRING(VA_value)
LISTEND

NAMEDEF(VAN)
   NAME("VA_variable")
   NAME("VA_value")
NAMEEND

#define VAS sizeof(VAN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_VARL_H */
