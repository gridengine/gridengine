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

#include "resolve_host.h"
#include "commd_message_flags.h"
#include "sge_answer.h"
#include "sge_complex.h"
#include "sge_schedd_conf.h"
#include "sge_parse_num_par.h"

#include "msg_common.h"
#include "msg_gdilib.h"

lList *Master_Complex_List = NULL;

/****** gdi/complex/sge_fill_requests() ***************************************
*  NAME
*     sge_fill_requests() -- fills and checks list of complex entries 
*
*  SYNOPSIS
*     int sge_fill_requests(lList *re_entries, 
*                           lList *complex_list, 
*                           int allow_non_requestable, 
*                           int allow_empty_boolean, 
*                           int allow_neg_consumable) 
*
*  FUNCTION
*     This function fills a given list of complex entries with missing
*     attributes which can be found in the complex. It checks also 
*     wether the given in the re_entries-List are valid. 
*
*  INPUTS
*     lList *re_entries         - resources as complex list CE_Type 
*     lList *complex_list       - the global complex list 
*     int allow_non_requestable - needed for qstat -l or qmon customize 
*                                 dialog 
*     int allow_empty_boolean   - boolean 
*        1 => NULL values of boolean attributes will 
*             be replaced with "TRUE" 
*        0 => NULL values will be handled as error 
*     int allow_neg_consumable  - boolean
*        1 => negative values for consumable 
*             resources are allowed. 
*        0 => function will return with -1 if it finds 
*             consumable resources with a negative value
*
*  RESULT
*     int - error
*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
*******************************************************************************/
int sge_fill_requests(lList *re_entries, lList *complex_list, 
                      int allow_non_requestable, int allow_empty_boolean,  
                      int allow_neg_consumable) 
{
   lListElem *c, *entry, *cep;
   const char *name;

   DENTER(TOP_LAYER, "sge_fill_requests");

   for_each(entry, re_entries) {
      name = lGetString(entry, CE_name);
   
      cep = NULL;
      for_each (c, complex_list) {
         if ((cep = find_attribute_in_complex_list(name, 
                                             lFirst(lGetList(c, CX_entries)))))
            break;
      }
      if (!cep) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_RESOURCE_S, name));
         DEXIT;
         return -1;
      }

      if (!allow_non_requestable && !lGetUlong(cep, CE_request)) {
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
      lSetUlong(entry, CE_consumable, lGetUlong(cep, CE_consumable)); 

      if (fill_and_check_attribute(entry, allow_empty_boolean, allow_neg_consumable)) {
         /* no error msg here - fill_and_check_attribute() makes it */
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}

/****** gdi/complex/fill_and_check_attribute() ********************************
*  NAME
*     fill_and_check_attribute() -- fill and check the attribute 
*
*  SYNOPSIS
*     int fill_and_check_attribute(lListElem *cep, 
*                                  int allow_empty_boolean, 
*                                  int allow_neg_consumable) 
*
*  FUNCTION
*     fill and check the attribute 
*
*  INPUTS
*     lListElem *cep           - CE_Type, this object will be checked 
*     int allow_empty_boolean  - boolean
*        1 => NULL values of boolean attributes will 
*             be replaced with "TRUE" 
*        0 => NULL values will be handled as error 
*     int allow_neg_consumable - boolean
*        1 => negative values for consumable 
*             resources are allowed. 
*        0 => function will return with -1 if it finds 
*             consumable resources with a negative value
*
*  RESULT

*        0 on success
*       -1 on error
*        an error message will be written into SGE_EVENT
*******************************************************************************/
int fill_and_check_attribute(lListElem *cep, int allow_empty_boolean,
                             int allow_neg_consumable) 
{
   static char tmp[1000];
   const char *name, *s;
   u_long32 type;
   double dval;
   int ret;

   DENTER(TOP_LAYER, "fill_and_check_attribute");

   name = lGetString(cep, CE_name);
   s = lGetString(cep, CE_stringval);

   if (!s) {
      if (allow_empty_boolean && lGetUlong(cep, CE_valtype)==TYPE_BOO) {
         lSetString(cep, CE_stringval, "TRUE");
         s = lGetString(cep, CE_stringval);
      }
      else {
         ERROR((SGE_EVENT, MSG_CPLX_VALUEMISSING_S, name));
         DEXIT;
         return -1;
      }
   }

   switch ( type = lGetUlong(cep, CE_valtype) ) {
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
         lSetDouble(cep, CE_doubleval, dval);
   
         /* also the CE_default must be parsable for numeric types */ 
         if ((s=lGetString(cep, CE_default)) 
            && !parse_ulong_val(&dval, NULL, type, s, tmp, sizeof(tmp)-1)) {
            ERROR((SGE_EVENT, MSG_CPLX_WRONGTYPE_SSS, name, s, tmp));
            DEXIT;
            return -1;
         }

         /* negative values are not allowed for consumable attributes */
         if (!allow_neg_consumable && lGetUlong(cep, CE_consumable)
             && lGetDouble(cep, CE_doubleval) < (double)0.0) {
            ERROR((SGE_EVENT, MSG_CPLX_ATTRIBISNEG_S, name));

            DEXIT;
            return -1;
         }     
         break;
      case TYPE_HOST:
         /* resolve hostname and store it */
         ret = sge_resolve_host(cep, CE_stringval);
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

/****** gdi/complex/complex_list_init_double_attr() ****************************
*  NAME
*     complex_list_init_double_attr() -- initialize double from string 
*
*  SYNOPSIS
*     void complex_list_init_double_attr(lList *cl) 
*
*  FUNCTION
*     Initialize all double values contained in "cl". 
*
*  INPUTS
*     lList *cl - complex list 
*******************************************************************************/
void complex_list_init_double_attr(lList *cl) 
{
   lListElem *cle, *cattr;

   if (cl) {
      for_each(cle, cl) {
         for_each(cattr, lGetList(cle,CX_entries)) {
            double new_val;

            parse_ulong_val(&new_val, NULL, lGetUlong(cattr, CE_valtype),
               lGetString(cattr, CE_stringval), NULL, 0);
            lSetDouble(cattr, CE_doubleval, new_val);
         }
      }
   }
}

/****** gdi/complex/complex_list_locate_attr() *********************************
*  NAME
*     complex_list_locate_attr() -- find a attribute in the complex list 
*
*  SYNOPSIS
*     lListElem* complex_list_locate_attr(lList *complex_list, 
*                                         const char *name) 
*
*  FUNCTION
*     Find the complex attribute identified by "name" in the 
*     "complex_list". 
*
*  INPUTS
*     lList *complex_list - complex list 
*     const char* name    - attribute name 
*
*  RESULT
*     lListElem* - found element or NULL
*******************************************************************************/
lListElem* complex_list_locate_attr(lList *complex_list, const char* name)
{
   lListElem *cep = NULL;
   lListElem *ret = NULL;

   DENTER(CULL_LAYER, "complex_list_locate_attr");

   for_each (cep, complex_list) {
      lListElem *first_attr = lFirst(lGetList(cep, CX_entries));
      
      ret = find_attribute_in_complex_list(name, first_attr);
      if (ret != NULL) {
         break;
      }
   }

   DEXIT;
   return ret;
}

/* complex_list: CX_Type */
int complex_list_verify(lList *complex_list, lList **alpp,
                        const char *obj_name, const char *qname) 
{
   lListElem *cep;
   const char *s;
   int ret = STATUS_OK;

   DENTER(TOP_LAYER, "complex_list_verify");

   for_each (cep, complex_list) {
      s = lGetString(cep, CX_name);

      /* it is not allowed to put standard complexes into a complex list */
      if (!strcmp(s, "global") ||
          !strcmp(s, "host")   ||
          !strcmp(s, "queue")) {
         ERROR((SGE_EVENT, MSG_SGETEXT_COMPLEXNOTUSERDEFINED_SSS,
               s, obj_name, qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }

      /* verify that all complex names in the queues complex list exist */
      if (!lGetElemStr(Master_Complex_List, CX_name, s)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNCOMPLEX_SSS,
               s, obj_name, qname));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         ret = STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return ret;
}       

