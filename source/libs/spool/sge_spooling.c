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

#include "sgermon.h"
#include "sge_log.h"

#include "msg_spoollib.h"

#include "sge_spooling.h"

lListElem *Default_Spool_Context;

/* creation and maintenance of the spooling context */

lListElem *spool_create_context(const char *name)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "spool_create_context");

   if(name == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_CONTEXTNEEDSNAME));
      DEXIT;
      return NULL;
   }

   ep = lCreateElem(SPC_Type);
   lSetString(ep, SPC_name, name);

   DEXIT;
   return ep;
}

lListElem *spool_free_context(lListElem *context)
{
   DENTER(TOP_LAYER, "spool_free_context");
  
   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_free_context"));
      DEXIT;
      return NULL;
   }
  
   spool_shutdown_context(context);
   context = lFreeElem(context);

   DEXIT;
   return context;
}


int spool_startup_context(lListElem *context)
{
   lListElem *rule;
/* JG: TODO: check if all types have a default rule */
   DENTER(TOP_LAYER, "spool_startup_context");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_startup_context"));
      DEXIT;
      return FALSE;
   }
  
   for_each(rule, lGetList(context, SPC_rules)) {
      spooling_startup_func func = (spooling_startup_func)lGetRef(rule, SPR_startup_func);
      if(func != NULL) {
         if(!func(rule)) {
            ERROR((SGE_EVENT, "startup of rule "SFQ" in context "SFQ" failed\n",
                   lGetString(rule, SPR_name), lGetString(context, SPC_name)));
            DEXIT;
            return FALSE;
         }
      }
   }
   
   DEXIT;
   return TRUE;
}

int spool_shutdown_context(lListElem *context)
{
   lListElem *rule;

   DENTER(TOP_LAYER, "spool_shutdown_context");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_shutdown_context"));
      DEXIT;
      return FALSE;
   }
  
   for_each(rule, lGetList(context, SPC_rules)) {
      spooling_shutdown_func func = (spooling_shutdown_func)lGetRef(rule, SPR_shutdown_func);
      if(func != NULL) {
         if(!func(rule)) {
            ERROR((SGE_EVENT, "shutdown of rule "SFQ" in context "SFQ" failed\n",
                   lGetString(rule, SPR_name), lGetString(context, SPC_name)));
            DEXIT;
            return FALSE;
         }
      }
   }
   
   DEXIT;
   return TRUE;
}


void spool_set_default_context(lListElem *context)
{
   Default_Spool_Context = context;
}

lListElem *spool_get_default_context(void)
{
   return Default_Spool_Context;
}

lListElem *spool_context_search_rule(const lListElem *context, const char *name)
{
   return lGetElemStr(lGetList(context, SPC_rules), SPR_name, name);
}

lListElem *spool_context_create_rule(lListElem *context, const char *name, const char *url,
                                     spooling_startup_func startup_func, 
                                     spooling_shutdown_func shutdown_func, 
                                     spooling_list_func list_func, 
                                     spooling_read_func read_func, 
                                     spooling_write_func write_func, 
                                     spooling_delete_func delete_func)
{
   lList *lp;
   lListElem *ep;

   DENTER(TOP_LAYER, "spool_context_create_rule");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_context_create_rule"));
      DEXIT;
      return NULL;
   }
  
   ep = lCreateElem(SPR_Type);
   lSetString(ep, SPR_name, name);
   lSetString(ep, SPR_url, url);
   lSetRef(ep, SPR_startup_func, (void *)startup_func);
   lSetRef(ep, SPR_shutdown_func, (void *)shutdown_func);
   lSetRef(ep, SPR_list_func, (void *)list_func);
   lSetRef(ep, SPR_read_func, (void *)read_func);
   lSetRef(ep, SPR_write_func, (void *)write_func);
   lSetRef(ep, SPR_delete_func, (void *)delete_func);

   lp = lGetList(context, SPC_rules);
   if(lp == NULL) {
      lp = lCreateList("spooling rules", SPR_Type);
      lSetList(context, SPC_rules, lp);
   }

   lAppendElem(lp, ep);

   DEXIT; 
   return ep;
}

lListElem *spool_context_search_type(const lListElem *context, const sge_event_type event_type)
{
   lListElem *ep;

   /* search fitting rule */
   ep = lGetElemUlong(lGetList(context, SPC_types), SPT_type, event_type);

   /* if no specific rule is found, return default rule */
   if(ep == NULL) {
      ep = lGetElemUlong(lGetList(context, SPC_types), SPT_type, SGE_EMT_ALL);
   }
   
   return ep;
}

lListElem *spool_context_create_type(lListElem *context, const sge_event_type event_type)
{
   lList *lp;
   lListElem *ep;

   DENTER(TOP_LAYER, "spool_context_create_type");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_context_create_type"));
      DEXIT;
      return NULL;
   }
  
   ep = lCreateElem(SPT_Type);
   lSetUlong(ep, SPT_type, event_type);
   lSetString(ep, SPT_name, sge_mirror_get_type_name(event_type));
  
   lp = lGetList(context, SPC_types);
   if(lp == NULL) {
      lp = lCreateList("spooling object types", SPT_Type);
      lSetList(context, SPC_types, lp);
   }

   lAppendElem(lp, ep);

   DEXIT;
   return ep;
}

lListElem *spool_type_search_default_rule(const lListElem *spool_type)
{  
   lList *lp;
   lListElem *ep;

   lp = lGetList(spool_type, SPT_rules);
   for_each(ep, lp) {
      if(lGetBool(ep, SPTR_default)) {
         return (lListElem *)lGetRef(ep, SPTR_rule);
      }
   }

   return NULL;
}

lListElem *spool_type_add_rule(lListElem *spool_type, const lListElem *rule, lBool is_default)
{
   lList *lp;
   lListElem *ep;
/* JG: TODO: check for former default rules */
   DENTER(TOP_LAYER, "spool_type_add_rule");

   if(spool_type == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDSPOOLTYPE_S, "spool_type_add_rule"));
      DEXIT;
      return NULL;
   }

   if(rule == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDRULE_S, "spool_type_add_rule"));
      DEXIT;
      return NULL;
   }

   ep = lCreateElem(SPTR_Type);
   lSetBool(ep, SPTR_default, is_default);
   lSetString(ep, SPTR_rule_name, lGetString(rule, SPR_name));
   lSetRef(ep, SPTR_rule, (void *)rule);

   lp = lGetList(spool_type, SPT_rules);
   if(lp == NULL) {
      lp = lCreateList("spooling object type rules", SPTR_Type);
      lSetList(spool_type, SPT_rules, lp);
   }
   
   lAppendElem(lp, ep);
  
   DEXIT;
   return ep;
}

/* reading */
int spool_read_list(const lListElem *context, lList **list, const sge_event_type event_type)
{
   lListElem *type;
   lListElem *rule;
   spooling_list_func func;

   int ret;

   DENTER(TOP_LAYER, "spool_read_list");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_read_list"));
      DEXIT;
      return FALSE;
   }
  
   type = spool_context_search_type(context, event_type);
   if(type == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_UNKNOWNOBJECTTYPEINCONTEXT_SS, 
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   /* use the default rule to read list */
   rule = spool_type_search_default_rule(type);
   if(rule == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NODEFAULTRULEFORTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   func = (spooling_list_func)lGetRef(rule, SPR_list_func);
   if(func == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_CORRUPTRULEINCONTEXT_SSS,
             lGetString(rule, SPR_name), lGetString(context, SPC_name),
             "spooling_list_func"));
      DEXIT;
      return FALSE;
   }


   ret = func(type, rule, list, event_type);

   DEXIT;
   return ret;
}

lListElem *spool_read_object(const lListElem *context, const sge_event_type event_type, const char *key)
{
   lListElem *type;
   lListElem *rule;
   lListElem *result;
   spooling_read_func func;

   DENTER(TOP_LAYER, "spool_read_object");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_read_object"));
      DEXIT;
      return NULL;
   }
  
   type = spool_context_search_type(context, event_type);
   if(type == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_UNKNOWNOBJECTTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   /* use the default rule to read object */
   rule = spool_type_search_default_rule(type);
   if(rule == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NODEFAULTRULEFORTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   func = (spooling_read_func)lGetRef(rule, SPR_read_func);
   if(func == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_CORRUPTRULEINCONTEXT_SSS,
             lGetString(rule, SPR_name), lGetString(context, SPC_name),
             "spooling_read_func"));
      DEXIT;
      return FALSE;
   }

   result = func(type, rule, key, event_type);

   DEXIT;
   return result;
}

/* writing */
int spool_write_object(const lListElem *context, const lListElem *object, const char *key, const sge_event_type event_type)
{
   lListElem *type;
   lListElem *type_rule, *rule;
   lList *type_rules;

   int ret = TRUE;

   DENTER(TOP_LAYER, "spool_write_object");

   /* we cannot spool free objects */
   if(object->status == FREE_ELEM) {
      ERROR((SGE_EVENT, MSG_SPOOL_CANNOTSPOOLFREEOBJECT));
      DEXIT;
      return FALSE;
   }

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_write_object"));
      DEXIT;
      return FALSE;
   }
  
   type = spool_context_search_type(context, event_type);
   if(type == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_UNKNOWNOBJECTTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   type_rules = lGetList(type, SPT_rules);
   if(type_rules == NULL || lGetNumberOfElem(type_rules) == 0) {
      ERROR((SGE_EVENT, MSG_SPOOL_NORULESFORTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   /* spool using multiple rules */
   for_each(type_rule, type_rules) {
      spooling_write_func func;
      rule = (lListElem *)lGetRef(type_rule, SPTR_rule);
      func = (spooling_write_func)lGetRef(rule, SPR_write_func);
      if(func == NULL) {
         ERROR((SGE_EVENT, MSG_SPOOL_CORRUPTRULEINCONTEXT_SSS,
                lGetString(rule, SPR_name), lGetString(context, SPC_name),
                "spooling_write_func"));
         DEXIT;
         return FALSE;
      }

      if(!func(type, rule, object, key, event_type)) {
         WARNING((SGE_EVENT, MSG_SPOOL_RULEINCONTEXTFAILEDWRITING_SS,
                  lGetString(rule, SPR_name), lGetString(context, SPC_name)));
         ret = FALSE;
      }
   }
  
   DEXIT;
   return ret;
}

/* deleting */
int spool_delete_object(const lListElem *context, const sge_event_type event_type, const char *key)
{
   lListElem *type;
   lListElem *type_rule, *rule;
   lList *type_rules;

   int ret = TRUE;

   DENTER(TOP_LAYER, "spool_delete_object");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_delete_object"));
      DEXIT;
      return FALSE;
   }
  
   type = spool_context_search_type(context, event_type);
   if(type == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_UNKNOWNOBJECTTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   type_rules = lGetList(type, SPT_rules);
   if(type_rules == NULL || lGetNumberOfElem(type_rules) == 0) {
      ERROR((SGE_EVENT, MSG_SPOOL_NORULESFORTYPEINCONTEXT_SS,
             sge_mirror_get_type_name(event_type), lGetString(context, SPC_name)));
      DEXIT;
      return FALSE;
   }

   /* delete object using all spooling rules */
   for_each(type_rule, type_rules) {
      spooling_delete_func func;
      rule = (lListElem *)lGetRef(type_rule, SPTR_rule);
      func = (spooling_delete_func)lGetRef(rule, SPR_delete_func);
      if(func == NULL) {
         ERROR((SGE_EVENT, MSG_SPOOL_CORRUPTRULEINCONTEXT_SSS,
                lGetString(rule, SPR_name), lGetString(context, SPC_name),
                "spooling_delete_func"));
         DEXIT;
         return FALSE;
      }

      if(!func(type, rule, key, event_type)) {
         WARNING((SGE_EVENT, MSG_SPOOL_RULEINCONTEXTFAILEDWRITING_SS,
                  lGetString(rule, SPR_name), lGetString(context, SPC_name)));
         ret = FALSE;
      }
   }
  
   DEXIT;
   return ret;
}

int spool_compare_objects(const lListElem *context, const sge_event_type event_type, const lListElem *ep1, const lListElem *ep2)
{
   DENTER(TOP_LAYER, "spool_compare_objects");

   if(context == NULL) {
      ERROR((SGE_EVENT, MSG_SPOOL_NOVALIDCONTEXT_S, "spool_compare_objects"));
      DEXIT;
      return 0;
   }

   DEXIT; 
   return 1;
}
