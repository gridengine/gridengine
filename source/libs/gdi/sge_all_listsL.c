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

#define __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__
#include "cull_list.h"
#include "sge_all_listsL.h"

int object_has_type(const lListElem *ep, const lDescr *descr) 
{
   int ret = 0;
 
   /*
    * we assume that "ep" is of the given type when the 
    * primary key is contained in the element
    *
    * --> make sure that your object is handled in object_get_primary_key() 
    */
   if (lGetPosInDescr(ep->descr, object_get_primary_key(descr)) != -1) {
      ret = 1;
   }
   return ret;
} 

int object_get_primary_key(const lDescr *descr)
{
   int ret = NoName;

   if (descr == EH_Type) {
      ret = EH_name;
   } else if (descr == AH_Type) {
      ret = AH_name;
   } else if (descr == SH_Type) {
      ret = SH_name;
   } else if (descr == QU_Type) {
      ret = QU_qname;
   } else if (descr == JB_Type) {
      ret = JB_job_number;
   } else if (descr == JAT_Type) {
      ret = JAT_task_number;
   } else if (descr == PET_Type) {
      ret = PET_id;
   } else if (descr == RN_Type) {
      ret = RN_min;
   } else if (descr == PE_Type) {
      ret = PE_name;
   } else if (descr == VA_Type) {
      ret = VA_variable;
   }
   return ret;
}
 
