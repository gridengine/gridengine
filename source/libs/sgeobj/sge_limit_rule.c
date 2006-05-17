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

#include <string.h>

#include "sgeobj/sge_limit_rule.h"
#include "sgeobj/sge_strL.h"
#include "sgeobj/msg_sgeobjlib.h"
#include "sgeobj/sge_answer.h"
#include "uti/sge_log.h"
#include "rmon/sgermon.h"


/****** sge_limit_rule/LIRF_object_parse_from_string() *************************
*  NAME
*     LIRF_object_parse_from_string() -- parse a LIRF Object from string
*
*  SYNOPSIS
*     bool LIRF_object_parse_from_string(lListElem **filter, const char* buffer, 
*     lList **alp) 
*
*  FUNCTION
*     Converts a spooled LIRF Object to a CULL Element
*
*  INPUTS
*     lListElem **filter - resulting LIRF object
*     const char* buffer - string to be converted
*     lList **alp        - answer_list
*
*  RESULT
*     bool - true on success
*            false on error
*
*  NOTES
*     MT-NOTE: LIRF_object_parse_from_string() is MT safe 
*
*******************************************************************************/
bool LIRF_object_parse_from_string(lListElem **filter, const char* buffer, lList **alp) {
   lListElem *tmp_filter = NULL;
   lListElem *scope = NULL;
   lList *lp = NULL;
   lList *scope_list = NULL;
   lList *xscope_list = NULL;
   char delims[] = "\t \v\r,`"; 

   DENTER(TOP_LAYER, "LIRF_object_parse_from_string");

   if (buffer == NULL) {
     DRETURN(false);
   }

   tmp_filter = lCreateElem(LIRF_Type);

   if ( buffer[0] == '`' ) {
      /* We have a expanded list */
      lSetBool(tmp_filter, LIRF_expand, true);
      if (buffer[strlen(buffer)-1] != '`') {
         ERROR((SGE_EVENT, MSG_LIMITRULE_NOVALIDEXPANDEDLIST));
         answer_list_add(alp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DRETURN(false);
      }
   } else {
      lSetBool(tmp_filter, LIRF_expand, false);
   }

   lString2List(buffer, &lp, ST_Type, ST_name, delims); 

   for_each(scope, lp) {
      const char *name = lGetString(scope, ST_name);
      if ( name[0] == '!' ) {
         lAddElemStr(&xscope_list, ST_name, name+1, ST_Type);
      } else {
         lAddElemStr(&scope_list, ST_name, name, ST_Type);
      }
   }

   lFreeList(&lp);

   lSetList(tmp_filter, LIRF_scope, scope_list);
   lSetList(tmp_filter, LIRF_xscope, xscope_list);

   *filter = tmp_filter;
   
   DRETURN(true);
}

/****** sge_limit_rule/LIRF_object_append_to_dstring() *************************
*  NAME
*     LIRF_object_append_to_dstring() -- LIRF Element to string
*
*  SYNOPSIS
*     bool LIRF_object_append_to_dstring(lListElem *filter, dstring *buffer, 
*     lList **alp) 
*
*  FUNCTION
*     Converts a LIRF element to string for spooling 
*
*  INPUTS
*     lListElem *filter - Element to be converted
*     dstring *buffer   - buffer for the element string
*     lList **alp       - answer_list
*
*  RESULT
*     bool - true on success
*            false on error
*
*  NOTES
*     MT-NOTE: LIRF_object_append_to_dstring() is MT safe 
*
*******************************************************************************/
bool LIRF_object_append_to_dstring(const lListElem *filter, dstring *buffer, lList **alp){
   lList *tlp = NULL;
   lListElem *scope = NULL;
   bool first = true;
   bool expand = false;
   bool ret = false;

   if (filter == NULL) {
      return ret;
   }

   expand = lGetBool(filter, LIRF_expand) ? true : false;

   if (expand) {
      sge_dstring_append(buffer, "`");
   }

   tlp = lGetList(filter, LIRF_scope);
   for_each(scope, tlp) {
      ret = true;
      if (!first) {
         sge_dstring_append(buffer, ",");
      } else {
         first = false;
      }
      sge_dstring_append(buffer, lGetString(scope, ST_name));
   }

   tlp = lGetList(filter, LIRF_xscope);
   for_each(scope, tlp) {
      ret = true;
      if (!first) {
         sge_dstring_append(buffer, ",");
      } else {
         first = false;
      }
      sge_dstring_append(buffer, "!");
      sge_dstring_append(buffer, lGetString(scope, ST_name));
   }

   if (expand) {
      sge_dstring_append(buffer, "`");
   }

   return ret; 
}
