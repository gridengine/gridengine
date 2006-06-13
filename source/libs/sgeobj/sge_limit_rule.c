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

/****** sge_limit_rule/limit_rule_set_defaults() *******************************
*  NAME
*     limit_rule_set_defaults() -- set default values to given lirs
*
*  SYNOPSIS
*     lListElem* limit_rule_set_defaults(lListElem* lirs) 
*
*  FUNCTION
*     This function sets the default values to a limitation rule set.
*     The default rule set is:
*     {
*       name = <must be already set>
*       enabled = true
*       limit to slots=0
*     }
*
*  INPUTS
*     lListElem* lirs - Already created object to be modified.
*
*  RESULT
*     lListElem* - modified Object
*
*  NOTES
*     MT-NOTE: limit_rule_set_defaults() is MT safe 
*******************************************************************************/
lListElem* limit_rule_set_defaults(lListElem* lirs)
{
   DENTER(TOP_LAYER, "limit_rule_set_add");

   if (lirs != NULL) {
      lList *limit_list = NULL;
      lList *rule_list = NULL;
      lListElem *rule = NULL;
      lListElem *limit = NULL;

      /* Free LIRS_rule */
      rule_list = lGetList(lirs, LIRS_rule);
      lFreeList(&rule_list);

      /* Create Rule List */
      rule_list = lCreateList("Rule_List", LIR_Type);
      rule = lCreateElem(LIR_Type);
      limit_list = lCreateList("Limit_List", LIRL_Type);
      limit = lCreateElem(LIRL_Type);
      lSetString(limit, LIRL_name, "slots");
      lSetString(limit, LIRL_value, "0");
      lAppendElem(limit_list, limit);
      lSetList(rule, LIR_limit, limit_list);
      lAppendElem(rule_list, rule);

      /* Set LIRS_enabled */
      lSetBool(lirs, LIRS_enabled, true);
      /* Set LIRS_rule */
      lSetList(lirs, LIRS_rule, rule_list);
   }
   DRETURN(lirs);
}

/****** sge_limit_rule/limit_rule_set_verify_attributes() **********************
*  NAME
*     limit_rule_set_verify_attributes() -- verify the attributes of a lirs Object 
*
*  SYNOPSIS
*     bool limit_rule_set_verify_attributes(lListElem *lirs, lList 
*     **answer_list, bool in_master) 
*
*  FUNCTION
*     This function verifies the attributes of a given lirs object. A valid lirs
*     object has a name and at least one rule set.
*     Addition checks are done if in_master is true.
*
*  INPUTS
*     lListElem *lirs     - Object that should be verified
*     lList **answer_list - answer list in case of errors
*     bool in_master      - flag if called by qmaster or by qconf
*
*  RESULT
*     bool - true on success
*            false on error
*
*  NOTES
*     MT-NOTE: limit_rule_set_verify_attributes() is MT safe 
*******************************************************************************/
bool limit_rule_set_verify_attributes(lListElem *lirs, lList **answer_list, bool in_master)
{
   bool ret = true;
   lList *rules = NULL;

   DENTER(TOP_LAYER, "limit_rule_set_verify_attributes");
   /* every rule set needs a LIRS_name */
   if (lGetString(lirs, LIRS_name) == NULL) {
      sprintf(SGE_EVENT, "limitation rule set has no name");
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false;
   }

   /* every rule set needs at least one rule */
   rules = lGetList(lirs, LIRS_rule);
   if (ret && (rules == NULL || lGetNumberOfElem(rules) < 1)) {
      sprintf(SGE_EVENT, "limitation rule set has no rules");
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false;
   }
  
   if (ret && in_master) {
      /* RD: TODO additional check if dynamical limit is correct */
   }

   DRETURN(ret);
}

/****** sge_limit_rule/limit_rule_sets_verify_attributes() *********************
*  NAME
*     limit_rule_sets_verify_attributes() -- verifies the attributes of a lirs list
*
*  SYNOPSIS
*     bool limit_rule_sets_verify_attributes(lList *lirs_list, lList 
*     **answer_list, bool in_master) 
*
*  FUNCTION
*     This function iterates over a lirs list and checks for every lirs the attributes
*
*  INPUTS
*     lList *lirs_list    - List that should be verified
*     lList **answer_list - answer list 
*     bool in_master      - flag if called by qmaster or qconf
*
*  RESULT
*     bool - true on success
*            false on error
*  NOTES
*     MT-NOTE: limit_rule_sets_verify_attributes() is MT safe 
*
*  SEE ALSO
*     sge_limit_rule/limit_rule_set_verify_attributes()
*******************************************************************************/
bool limit_rule_sets_verify_attributes(lList *lirs_list, lList **answer_list, bool in_master)
{
   bool ret = true;
   
   DENTER(TOP_LAYER, "limit_rule_sets_verify_attributes");
   if (lirs_list != NULL) {
      lListElem *lirs = NULL;

      for_each(lirs, lirs_list) {
         ret = limit_rule_set_verify_attributes(lirs, answer_list, in_master);
         if (!ret) {
            break;
         }
      }
   }
   DRETURN(ret);
}

/****** sge_limit_rule/limit_rule_set_list_locate() ****************************
*  NAME
*     limit_rule_set_list_locate() -- locate a specific rule set by name
*
*  SYNOPSIS
*     lListElem* limit_rule_set_list_locate(lList *lp, const char *name) 
*
*  FUNCTION
*     This function searches the rule set list for a specific rule set. The
*     search criteria is the name of the rule set.
*
*  INPUTS
*     lList *lp        - rule set list to be searched in
*     const char *name - rule set name of interest
*
*  RESULT
*     lListElem* - if found the reference to the rule set, else NULL
*
*  NOTES
*     MT-NOTE: limit_rule_set_list_locate() is MT safe 
*******************************************************************************/
lListElem *limit_rule_set_list_locate(lList *lp, const char *name)
{
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "limit_rule_set_list_locate");

   ep = lGetElemStr(lp, LIRS_name, name);

   DRETURN(ep);
}

/****** sge_limit_rule/limit_rule_locate() *************************************
*  NAME
*     limit_rule_locate() -- locate a specific rule by name 
*
*  SYNOPSIS
*     lListElem* limit_rule_locate(lList *lp, const char *name) 
*
*  FUNCTION
*     This function searches a rule list for a specific rule. The search criteria
*     is the name or the index of the rule.
*     The index is used if the name was successfully converted to an integer by
*     the atoi() function. If atoi() was not able to convert the name the rule is
*     searched by a sting compare.
*
*  INPUTS
*     lList *lp        - list to be searched in
*     const char *name - rule name of interest
*
*  RESULT
*     lListElem* - reference to found rule
*                  NULL if rule was not found
*
*  NOTES
*     MT-NOTE: limit_rule_locate() is MT safe 
*******************************************************************************/
lListElem *limit_rule_locate(lList *lp, const char *name)
{
   lListElem *ep = NULL;
   int get_pos = 0;
   int act_pos = 1;

   DENTER(TOP_LAYER, "limit_rule_locate");

   if (name == NULL) {
      DRETURN(NULL);
   }

   get_pos = atoi(name);

   for_each(ep, lp) {
      const char *rule_name = lGetString(ep, LIR_name);
      if (get_pos != -1 && act_pos == get_pos) {
         break;
      } else if (rule_name != NULL && !strcasecmp(name, lGetString(ep, LIR_name))) {
         break;
      }
      act_pos++;
   }

   DRETURN(ep);
}

/****** sge_limit_rule/lir_xattr_pre_gdi() *************************************
*  NAME
*     lir_xattr_pre_gdi() -- qconf xattr list preformat
*
*  SYNOPSIS
*     bool lir_xattr_pre_gdi(lList *this_list, lList **answer_list) 
*
*  FUNCTION
*     This function preformates the given list created by xattr. The xattr switch
*     allows to address single rules by using the special rule set name "rule_set_name/rule_name".
*     The rule name can be the name for named rules or the index of the rule.
*     This function splits such a name into the single rule set name and set the correct
*     rule name.
*
*  INPUTS
*     lList *this_list    - list to be modified
*     lList **answer_list - answer list
*
*  RESULT
*     bool - true on success
*            false on error
*
*  NOTES
*     MT-NOTE: lir_xattr_pre_gdi() is MT safe 
*******************************************************************************/
bool lir_xattr_pre_gdi(lList *this_list, lList **answer_list) 
{
   bool ret = true;
   char delim[] = "/";

   DENTER(TOP_LAYER, "lir_xattr_pre_gdi");
   if (this_list != NULL) {
      lListElem *lirs = NULL;

      for_each(lirs, this_list) {
         lList *lp = NULL;
         const char *name = lGetString(lirs, LIRS_name);
         
         lString2List(name, &lp, ST_Type, ST_name, delim);
         if (lGetNumberOfElem(lp) == 2) {
            lListElem *ep = NULL;
            lListElem *rule = NULL;
            lList *rules = lGetList(lirs, LIRS_rule);

            ep = lFirst(lp);
            lSetString(lirs, LIRS_name, lGetString(ep, ST_name));
            ep = lNext(ep);
            for_each(rule, rules) {
               lSetString(rule, LIR_name, lGetString(ep, ST_name));
            }
         }
         lFreeList(&lp);
      }
   }
   DRETURN(ret);
}
