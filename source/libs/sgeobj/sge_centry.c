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

#include "commd_message_flags.h"
#include "sge_answer.h"
#include "sge_schedd_conf.h"
#include "sge_parse_num_par.h"
#include "sge_host.h"
#include "sge_queue.h"
#include "sge_ulong.h"
#include "sge_centry.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CENTRY_LAYER TOP_LAYER

lList *Master_CEntry_List = NULL;

static int
centry_fill_and_check(lListElem *cep, bool allow_empty_boolean,
                      bool allow_neg_consumable);

/****** sge/centry/centry_fill_and_check() ************************************
*  NAME
*     centry_fill_and_check() -- fill and check the attribute
*
*  SYNOPSIS
*     int centry_fill_and_check(lListElem *cep,
*                               bool allow_empty_boolean,
*                               bool allow_neg_consumable)
*
*  FUNCTION
*     fill and check the attribute
*
*  INPUTS
*     lListElem *cep           - CE_Type, this object will be checked
*     int allow_empty_boolean  - boolean
*        true  - NULL values of boolean attributes will
*                be replaced with "true"
*        false - NULL values will be handled as error
*     int allow_neg_consumable - boolean
*        true  - negative values for consumable
*                resources are allowed.
*        false - function will return with -1 if it finds
*                consumable resources with a negative value
*
*  RESULT
*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
******************************************************************************/
static int
centry_fill_and_check(lListElem *this_elem, bool allow_empty_boolean,
                      bool allow_neg_consumable)
{
   static char tmp[1000];
   const char *name, *s;
   u_long32 type;
   double dval;
   int ret;

   DENTER(CENTRY_LAYER, "centry_fill_and_check");

   name = lGetString(this_elem, CE_name);
   s = lGetString(this_elem, CE_stringval);

   if (!s) {
      if (allow_empty_boolean && lGetUlong(this_elem, CE_valtype)==TYPE_BOO) {
         lSetString(this_elem, CE_stringval, "TRUE");
         s = lGetString(this_elem, CE_stringval);
      }
      else {
         ERROR((SGE_EVENT, MSG_CPLX_VALUEMISSING_S, name));
         DEXIT;
         return -1;
      }
   }

   switch ( type = lGetUlong(this_elem, CE_valtype) ) {
      case TYPE_INT:
      case TYPE_TIM:
      case TYPE_MEM:
      case TYPE_BOO:
      case TYPE_DOUBLE:
         if (!parse_ulong_val(&dval, NULL, type, s, tmp, sizeof(tmp)-1)) {
            ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, name, s, tmp));
            DEXIT;
            return -1;
         }
         lSetDouble(this_elem, CE_doubleval, dval);

         /* also the CE_default must be parsable for numeric types */
         if ((s=lGetString(this_elem, CE_default))
            && !parse_ulong_val(&dval, NULL, type, s, tmp, sizeof(tmp)-1)) {
            ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, name, s, tmp));
            DEXIT;
            return -1;
         }

         /* negative values are not allowed for consumable attributes */
         if (!allow_neg_consumable && lGetBool(this_elem, CE_consumable)
             && lGetDouble(this_elem, CE_doubleval) < (double)0.0) {
            ERROR((SGE_EVENT, MSG_CPLX_ATTRIBISNEG_S, name));

            DEXIT;
            return -1;
         }
         break;
      case TYPE_HOST:
         /* resolve hostname and store it */
         ret = sge_resolve_host(this_elem, CE_stringval);
         if (ret) {
            if (ret == COMMD_NACK_UNKNOWN_HOST) {
               ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, s));
            } else {
               ERROR((SGE_EVENT, MSG_SGETEXT_INVALIDHOST_S, s));
            }
            DEXIT;
            return -1;
         }
         break;
      case TYPE_STR:
      case TYPE_CSTR:
         /* no restrictions - so everything is ok */
         break;

      default:
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U, u32c(type)));
         DEXIT;
         return -1;
   }

   DEXIT;
   return 0;
}

const char *
map_op2str(u_long32 op)
{
   static char *opv[] = {
      "??",
      "==", /* CMPLXEQ_OP */
      ">=", /* CMPLXGE_OP */
      ">",  /* CMPLXGT_OP */
      "<",  /* CMPLXLT_OP */
      "<=", /* CMPLXLE_OP */
      "!="  /* CMPLXNE_OP */
   };

   if (op < CMPLXEQ_OP || op > CMPLXNE_OP) {
      op = 0;
   }
   return opv[op];
}

const char *
map_req2str(u_long32 op)
{
   static char *opv[] = {
      "??",
      "NO",       /* REQU_NO */
      "YES",      /* REQU_YES */
      "FORCED",   /* REQU_FORCED */
   };

   if (op < REQU_NO || op > REQU_FORCED) {
      op = 0;
   }
   return opv[op];
}

const char *
map_type2str(u_long32 type)
{
   static char *typev[] = {
      "??????",
      "INT",     /* TYPE_INT */
      "STRING",  /* TYPE_STR */
      "TIME",    /* TYPE_TIM */
      "MEMORY",  /* TYPE_MEM */
      "BOOL",    /* TYPE_BOO */
      "CSTRING", /* TYPE_CSTR */
      "HOST",    /* TYPE_HOST */
      "DOUBLE",  /* TYPE_DOUBLE */

      "TYPE_ACC",/* TYPE_ACC */
      "TYPE_LOG",/* TYPE_LOG */
      "TYPE_LOF" /* TYPE_LOF */
   };

   if (type < TYPE_FIRST || type > TYPE_LAST) {
      type = 0;
   }
   return typev[type];
}

/****** sge/centry/centry_create() ********************************************
*  NAME
*     centry_create() -- Create a preinitialized centry element 
*
*  SYNOPSIS
*     lListElem *
*     centry_create(lList **answer_list, const char *name)
*
*  FUNCTION
*     Create a preinitialized centry element with the given "name".
*
*  INPUTS
*     lList **answer_list  - AN_Type 
*     const char *name     - full name 
*
*  RESULT
*     lListElem * - CE_Type element
*******************************************************************************/
lListElem *
centry_create(lList **answer_list, const char *name)
{
   lListElem *ret = NULL;  /* CE_Type */

   DENTER(CENTRY_LAYER, "centry_create");
   if (name != NULL) {
      ret = lCreateElem(CE_Type);
      if (ret != NULL) {
         lSetString(ret, CE_name, name);
         lSetString(ret, CE_shortcut, name);
         lSetUlong(ret, CE_valtype, TYPE_INT);
         lSetUlong(ret, CE_relop, CMPLXLE_OP);
         lSetUlong(ret, CE_requestable, REQU_NO);
         lSetBool(ret, CE_consumable, false);
         lSetString(ret, CE_default, "1");
      } else {
         answer_list_add_sprintf(answer_list, STATUS_EMALLOC, 
                                 ANSWER_QUALITY_ERROR,
                                 MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_ERROR1, ANSWER_QUALITY_ERROR,
                              MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC);
   }
   DEXIT;
   return ret;
}

/****** sge/centry/centry_is_referenced() *************************************
*  NAME
*     centry_is_referenced() -- Is centry element referenced?
*
*  SYNOPSIS
*     bool 
*     centry_is_referenced(const lListElem *centry, 
*                          lList **answer_list, 
*                          const lList *master_queue_list, 
*                          const lList *master_exechost_list, 
*                          const lList *master_sconf_list) 
*
*  FUNCTION
*     Is the centry element referenced in a sublist of
*     "master_queue_list", "master_exechost_list" or 
*     "master_sconf_list". 
*
*  INPUTS
*     const lListElem *centry           - CE_Type 
*     lList **answer_list               - AN_Type 
*     const lList *master_queue_list    - QU_Type 
*     const lList *master_exechost_list - EH_Type 
*     const lList *master_sconf_list    - SC_Type 
*
*  RESULT
*     bool - true or false
*******************************************************************************/
bool 
centry_is_referenced(const lListElem *centry, lList **answer_list,
                     const lList *master_queue_list,
                     const lList *master_exechost_list,
                     const lList *master_sconf_list)
{
   bool ret = false;

   DENTER(CENTRY_LAYER, "centry_is_referenced");
   if (!ret) {
      const char *centry_name = lGetString(centry, CE_name);

      if (!ret) {
         lListElem *queue = NULL;   /* QU_Type */

         for_each(queue, master_queue_list) {
            if (queue_is_centry_referenced(queue, centry)) {
               const char *queue_name = lGetString(queue, QU_qname);

               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_INFO, 
                                       MSG_CENTRYREFINQUEUE_SS,
                                       centry_name, queue_name);
               ret = true;
               break;
            }
         }
      }
      if (!ret) {
         lListElem *host = NULL;    /* EH_Type */

         for_each(host, master_exechost_list) {
            if (host_is_centry_referenced(host, centry)) {
               const char *host_name = lGetString(host, EH_name);

               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                       ANSWER_QUALITY_INFO, 
                                       MSG_CENTRYREFINHOST_SS,
                                       centry_name, host_name);
               ret = true;
               break;
            }
         }
      }
      if (!ret) {
         lListElem *sconf = lFirst(master_sconf_list);   /* SC_Type */
      
         if (sconf_is_centry_referenced(sconf, centry)) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_INFO, 
                                    MSG_CENTRYREFINSCONF_S, centry_name);
            ret = true;
         }
      } 
   }
   DEXIT;
   return ret;
}

/****** sge/centry/centry_print_resource_to_dstring() *************************
*  NAME
*     centry_print_resource_to_dstring() -- Print to dstring 
*
*  SYNOPSIS
*     bool 
*     centry_print_resource_to_dstring(const lListElem *this_elem, 
*                                      dstring *string) 
*
*  FUNCTION
*     Print resource string (memory, time) to dstring.
*
*  INPUTS
*     const lListElem *this_elem - CE_Type 
*     dstring *string            - dynamic string 
*
*  RESULT
*     bool - error state 
*        true  - success
*        false - error
*******************************************************************************/
bool
centry_print_resource_to_dstring(const lListElem *this_elem, dstring *string)
{
   bool ret = true;

   DENTER(CENTRY_LAYER, "centry_print_resource_to_dstring");
   if (this_elem != NULL && string != NULL) {
      u_long32 type = lGetUlong(this_elem, CE_valtype);
      double val = lGetDouble(this_elem, CE_doubleval);

      switch (type) {
      case TYPE_TIM:
         double_print_time_to_dstring(val, string);
         break;
      case TYPE_MEM:
         double_print_memory_to_dstring(val, string);
         break;
      default:
         double_print_to_dstring(val, string);
         break;
      }
   }
   DEXIT;
   return ret;
}

/****** sge/centry/centry_list_get_master_list() ******************************
*  NAME
*     centry_list_get_master_list() -- return master list 
*
*  SYNOPSIS
*     lList ** centry_list_get_master_list(void) 
*
*  FUNCTION
*     Return master list. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     lList ** - CE_Type master list
*******************************************************************************/
lList **
centry_list_get_master_list(void)
{
   return &Master_CEntry_List;
}

/****** sge/centry/centry_list_locate() ***************************************
*  NAME
*     centry_list_locate() -- Find Centry element 
*
*  SYNOPSIS
*     lListElem *centry_list_locate(const lList *this_list, const char *name) 
*
*  FUNCTION
*     Find CEntry element with "name" in "this_list". 
*
*  INPUTS
*     const lList *this_list - CE_Type list 
*     const char *name       - name of an CE_Type entry 
*
*  RESULT
*     lListElem * - CE_Type element 
*******************************************************************************/
lListElem *
centry_list_locate(const lList *this_list, const char *name)
{
   lListElem *ret = NULL;   /* CE_Type */

/*   DENTER(CENTRY_LAYER, "centry_list_locate");*/
   if (this_list != NULL && name != NULL) {
      ret = lGetElemStr(this_list, CE_name, name);
      if (ret == NULL) {
         ret = lGetElemStr(this_list, CE_shortcut, name);
      }
   }
/*   DEXIT;*/
   return ret;
}

/****** sge/centry/centry_list_sort() ****************************************
*  NAME
*     centry_list_sort() -- Sort a CE_Type list 
*
*  SYNOPSIS
*     bool centry_list_sort(lList *this_list) 
*
*  FUNCTION
*     Sort a CE_Type list 
*
*  INPUTS
*     lList *this_list - CE_Type list 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*******************************************************************************/
bool
centry_list_sort(lList *this_list)
{
   bool ret = true;

   DENTER(CENTRY_LAYER, "centry_list_sort");
   if (this_list != NULL) {
      lSortOrder *order = NULL;

      order = lParseSortOrderVarArg(lGetListDescr(this_list), "%I+", CE_name);
      lSortList(this_list, order);
      order = lFreeSortOrder(order);
   }
   DEXIT;
   return ret;
}

/****** sge/centry/centry_list_init_double() **********************************
*  NAME
*     centry_list_init_double() -- Initialize double from string 
*
*  SYNOPSIS
*     bool centry_list_init_double(lList *this_list) 
*
*  FUNCTION
*     Initialize all double values contained in "this_list" 
*
*  INPUTS
*     lList *this_list - CE_Type list 
*
*  RESULT
*     bool - true
*******************************************************************************/
bool
centry_list_init_double(lList *this_list)
{
   bool ret = true;

   if (this_list != NULL) {
      lListElem *centry;

      for_each(centry, this_list) {
         double new_val = 0.0; /* 
                                * parse_ulong_val will not set it for all 
                                * data types! 
                                */
         parse_ulong_val(&new_val, NULL, lGetUlong(centry, CE_valtype),
                         lGetString(centry, CE_stringval), NULL, 0);
         lSetDouble(centry, CE_doubleval, new_val);
      }
   }
   return ret;
}

/****** sge/complex/centry_list_fill_request() ********************************
*  NAME
*     centry_list_fill_request() -- fills and checks list of complex entries
*
*  SYNOPSIS
*     int centry_list_fill_request(lList *centry_list,
*                                  lList *master_centry_list,
*                                  bool allow_non_requestable,
*                                  bool allow_empty_boolean,
*                                  bool allow_neg_consumable)
*
*  FUNCTION
*     This function fills a given list of complex entries with missing
*     attributes which can be found in the complex. It checks also
*     wether the given in the centry_list-List are valid.
*
*  INPUTS
*     lList *this_list           - resources as complex list CE_Type
*     lList *master_centry_list  - the global complex list
*     bool allow_non_requestable - needed for qstat -l or qmon customize
*                                 dialog
*     int allow_empty_boolean    - boolean
*        true  - NULL values of boolean attributes will
*                be replaced with "true"
*        false - NULL values will be handled as error
*     int allow_neg_consumable  - boolean
*        true  - negative values for consumable
*                resources are allowed.
*        false - function will return with -1 if it finds
*                consumable resources with a negative value
*
*  RESULT
*     int - error
*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
*******************************************************************************/
int
centry_list_fill_request(lList *this_list, lList *master_centry_list,
                         bool allow_non_requestable, bool allow_empty_boolean,
                         bool allow_neg_consumable)
{
   lListElem *entry, *cep;

   DENTER(CENTRY_LAYER, "centry_list_fill_request");

   for_each(entry, this_list) {
      const char *name = lGetString(entry, CE_name);
      u_long32 requestable;

      cep = centry_list_locate(master_centry_list, name);
      if (cep != NULL) {
         requestable = lGetUlong(cep, CE_requestable);
         if (!allow_non_requestable && requestable == REQU_NO) {
            ERROR((SGE_EVENT, MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S, name));
            DEXIT;
            return -1;
         }

         /* replace name in request/threshold/consumable list,
            it may have been a shortcut */
         lSetString(entry, CE_name, lGetString(cep, CE_name));

         /* we found the right complex attrib */
         /* so we know the type of the requested data */
         lSetUlong(entry, CE_valtype, lGetUlong(cep, CE_valtype));

         /* we also know wether it is a consumable attribute */
         lSetBool(entry, CE_consumable, lGetBool(cep, CE_consumable));

         if (centry_fill_and_check(entry, allow_empty_boolean, allow_neg_consumable)) {
            /* no error msg here - centry_fill_and_check() makes it */
            DEXIT;
            return -1;
         }
      } else {
         /* EB: TODO: message should be put into answer_list and
            returned via argument. */
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_RESOURCE_S, name));
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}

bool
centry_list_are_queues_requestable(const lList *this_list) 
{
   bool ret = false;
   
   DENTER(CENTRY_LAYER, "centry_list_are_queues_requestable");
   if (this_list != NULL) {
      lListElem *centry = centry_list_locate(this_list, "qname");
      
      if (centry != NULL) {
         ret = (lGetUlong(centry, CE_requestable) != REQU_NO);
      }
   }
   DEXIT;
   return ret;
}


