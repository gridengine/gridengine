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

#include "basis_types.h"
#include "sgermon.h" 
#include "sge_string.h"
#include "sge_stringL.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "commlib.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CSTRING_LAYER TOP_LAYER

bool cstring_list_append_to_string(const lList *this_list, dstring *string)
{
   bool ret = true;
   
   DENTER(CSTRING_LAYER, "cstring_list_append_to_string");
   if (this_list != NULL) {
      lListElem *str;
      bool is_first = true;

      for_each(str, this_list) {
         const char *name = lGetString(str, STR);

         if (!is_first) {
            sge_dstring_sprintf_append(string, ",");
         }
         sge_dstring_sprintf_append(string, "%s", name);
         is_first = false;
      }
   }
   DEXIT;
   return ret;
}

bool cstring_list_parse_from_string(lList **this_list, 
                                    const char *string, const char *delimitor)
{
   bool ret = true;

   DENTER(CSTRING_LAYER, "cstring_list_parse_from_string");
   if (this_list != NULL) {
      struct saved_vars_s *context = NULL;
      const char *token;

      token = sge_strtok_r(string, delimitor, &context);
      while (token) {
         lAddElemStr(this_list, STR, token, ST_Type);
         token = sge_strtok_r(NULL, delimitor, &context); 
      }
      sge_free_saved_vars(context);
   }
   DEXIT;
   return ret;
}

