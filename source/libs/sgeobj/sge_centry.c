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
#include "cull_parse_util.h"
#include "sge_answer.h"
#include "sge_schedd_conf.h"
#include "sge_parse_num_par.h"
#include "sge_host.h"
#include "sge_queue.h"
#include "sge_ulong.h"
#include "sge_centry.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define CENTRY_LAYER BASIS_LAYER

lList *Master_CEntry_List = NULL;

static int
centry_fill_and_check(lListElem *cep, bool allow_empty_boolean,
                      bool allow_neg_consumable);

   const int max_host_resources=24;/* specifies the number of elements in the host_resource array */
   const struct queue2cmplx host_resource[] = {
      {"arch",           0, TYPE_STR},
      {"cpu",            0, TYPE_DOUBLE},
      {"load_avg",       0, TYPE_DOUBLE},
      {"load_long",      0, TYPE_DOUBLE},
      {"load_medium",    0, TYPE_DOUBLE},
      {"load_short",     0, TYPE_DOUBLE},
      {"mem_free",       0, TYPE_MEM},
      {"mem_total",      0, TYPE_MEM},
      {"mem_used",       0, TYPE_MEM},
      {"min_cpu_inter",  0, TYPE_TIM},
      {"np_load_avg",    0, TYPE_DOUBLE},
      {"np_load_long",   0, TYPE_DOUBLE},
      {"np_load_medium", 0, TYPE_DOUBLE},
      {"np_load_short",  0, TYPE_DOUBLE},
      {"num_proc",       0, TYPE_INT},
      {"swap_free",      0, TYPE_MEM},
      {"swap_rate",      0, TYPE_MEM},
      {"swap_rsvd",      0, TYPE_MEM},
      {"swap_total",     0, TYPE_MEM},
      {"swap_used",      0, TYPE_MEM},
      {"tmpdir",         0, TYPE_STR},
      {"virtual_free",   0, TYPE_MEM},
      {"virtual_total",  0, TYPE_MEM},
      {"virtual_used",   0, TYPE_MEM}
   };

   const int max_queue_resources=24; /* specifies the number of elements in the queue_resource array */
   const struct queue2cmplx queue_resource[] = {
      {"qname",            QU_qname,            TYPE_STR },
      {"hostname",         QU_qhostname,        TYPE_HOST},
      {"slots",            QU_job_slots,        TYPE_INT },
      {"tmpdir",           QU_tmpdir,           TYPE_STR },
      {"seq_no",           QU_seq_no,           TYPE_INT }, 
      {"rerun",            QU_rerun,            TYPE_BOO },
      {"calendar",         QU_calendar,         TYPE_STR },
      {"s_rt",             QU_s_rt,             TYPE_TIM },
      {"h_rt",             QU_h_rt,             TYPE_TIM },
      {"s_cpu",            QU_s_cpu,            TYPE_TIM },
      {"h_cpu",            QU_h_cpu,            TYPE_TIM },
      {"s_fsize",          QU_s_fsize,          TYPE_MEM },
      {"h_fsize",          QU_h_fsize,          TYPE_MEM },
      {"s_data",           QU_s_data,           TYPE_MEM },
      {"h_data",           QU_h_data,           TYPE_MEM },
      {"s_stack",          QU_s_stack,          TYPE_MEM },
      {"h_stack",          QU_h_stack,          TYPE_MEM },
      {"s_core",           QU_s_core,           TYPE_MEM },
      {"h_core",           QU_h_core,           TYPE_MEM },
      {"s_rss",            QU_s_rss,            TYPE_MEM },
      {"h_rss",            QU_h_rss,            TYPE_MEM },
      {"s_vmem",           QU_s_vmem,           TYPE_MEM },
      {"h_vmem",           QU_h_vmem,           TYPE_MEM },
      {"min_cpu_interval", QU_min_cpu_interval, TYPE_TIM }
   };


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
               const char *host_name = lGetHost(host, EH_name);

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

const char *
centry_list_append_to_dstring(const lList *this_list, dstring *string)
{
   const char *ret = NULL;

   DENTER(CENTRY_LAYER, "centry_list_append_to_dstring");
   if (string != NULL) {
      lListElem *elem = NULL;
      bool printed = false;

      for_each(elem, this_list) {
         if (printed) {
            sge_dstring_sprintf_append(string, ",");
         }
         sge_dstring_sprintf_append(string, "%s=", lGetString(elem, CE_name));
         if (lGetString(elem, CE_stringval) != NULL) {
            sge_dstring_sprintf_append(string, "%s",
                                       lGetString(elem, CE_stringval));
         } else {
            sge_dstring_sprintf_append(string, "%f", 
                                       lGetString(elem, CE_doubleval));
         }
         printed = true;
      }
      if (!printed) {
         sge_dstring_sprintf_append(string, "NONE");
      }
      ret = sge_dstring_get_string(string);
   }
   DEXIT;
   return ret;
}

/* EB: TODO: CLEANUP: should be replaced by centry_list_append_to_dstring() */
int
centry_list_append_to_string(lList *this_list, char *buff, 
                             u_long32 max_len)
{
   int attr_fields[] = { CE_name, CE_stringval, 0 };
   const char *attr_delis[] = {"=", ",", "\n"};
   int ret;

   DENTER(TOP_LAYER, "centry_list_append_to_string");

   if (buff)
      buff[0] = '\0';

   lPSortList(this_list, "%I+", CE_name);

   ret = uni_print_list(NULL, buff, max_len, this_list, attr_fields, attr_delis, 0);
   if (ret) {
      DEXIT;
      return ret;
   }

   DPRINTF(("buff: %s\n", buff));
   DEXIT;
   return 0;
}

/* EB: TODO: CLEANUP: add answer_list remove SGE_EVENT */
/*
 * NOTE
 *    MT-NOTE: function is not MT safe
 */
lList *
centry_list_parse_from_string(lList *complex_attributes,
                              const char *str, bool check_value) 
{
   const char *cp;

   DENTER(TOP_LAYER, "centry_list_parse_from_string");

   /* allocate space for attribute list if no list is passed */
   if (complex_attributes == NULL) {
      if ((complex_attributes = lCreateList("", CE_Type)) == NULL) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRLIST));
         DEXIT;
         return NULL;
      }
   }

   /* str now points to the attr=value pairs */
   while ((cp = sge_strtok(str, ", "))) {
      lListElem *complex_attribute;
      const char *attr;
      char *value;

      str = NULL;       /* for the next strtoks */

      if ((complex_attribute = lCreateElem(CE_Type)) == NULL) {
         ERROR((SGE_EVENT, MSG_PARSE_NOALLOCATTRELEM));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }

      /*
      ** recursive strtoks didnt work
      */
      attr = cp;
      if ((value = strchr(cp, '='))) {
         *value++ = 0;
      }

      if (attr == NULL || *attr == '\0') {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWN_RESOURCE_S, ""));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }

      if ((check_value) && (value == NULL || *value == '\0')) {
         ERROR((SGE_EVENT, MSG_CPLX_VALUEMISSING_S, attr));
         lFreeList(complex_attributes);
         DEXIT;
         return NULL;
      }

      lSetString(complex_attribute, CE_name, attr);
      lSetString(complex_attribute, CE_stringval, value);

      lAppendElem(complex_attributes, complex_attribute);
   }

   DEXIT;
   return complex_attributes;
}

void
centry_list_remove_duplicates(lList *this_list) 
{
   DENTER(TOP_LAYER, "centry_list_remove_duplicates");
   cull_compress_definition_list(this_list, CE_name, CE_stringval, 0);
   DEXIT;
   return;
}


/****** sge/complex/centry_elem_validate() ********************************
*  NAME
*     centry_elem_validate() -- validates a given element and checks for duplicates 
*
*  SYNOPSIS
*     int centry_list_fill_request(lListElem *centry,
*                                  lList *centry_list,
*                                  lList *answer_list)
*
*  FUNCTION
*     Checks weather the configuration within the new centry is okay or not. A centry
*     is valid, when it satisfies the following rules:   
*         name 	  : has to be unique
*         Short cu  : has to be unique
*         Type	     : every type from the list (string, host, cstring, int, double,
*                                                boolean, memory, time)
*         Consumable : can only be defined for: int, double, memory, time
*
*         Relational operator:
*         - for consumables:              only <=
*         - for non consumables:
*            - string, host, cstring:     only ==, !=
*            - boolean:	                  only ==
*            - int, double, memory, time: ==, !=, <=, <, =>, >
*
*         Requestable	   : for all attribute
*         default value 	: only for consumables
*
*     The type for build in attributes is not allowed to be changed!
*
*     When no centy list is passed in, the check for uniqie name and short cuts is skipt.
*
*  INPUTS
*     lListElem *centry     - the centry list, which should be validated
*     lList *centry_list    - if not null, the function checks, if the centry element
*                             is already in the list
*     lList *answer_list    - contains the error messages
*
*  RESULT   bool  false - error (the anwer_list contains the error message)
*                 true - okay
*
*******************************************************************************/
bool centry_elem_validate(lListElem *centry, lList *centry_list, lList **answer_list){
   u_long32 relop = lGetUlong(centry, CE_relop);
   u_long32 type = lGetUlong(centry, CE_valtype);
   const char *attrname = lGetString(centry, CE_name);
   const char *temp;
   bool ret = true;

   DENTER(TOP_LAYER, "centry_elem_validate");

   switch(type){
      case TYPE_INT :
      case TYPE_MEM :
      case TYPE_DOUBLE:
      case TYPE_TIM : /* no checks, they can have everything */
         break;
      
      case TYPE_STR :
      case TYPE_CSTR :
      case TYPE_HOST : if ( !(relop == CMPLXEQ_OP || relop == CMPLXNE_OP) ) {
                           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR,
                                                   MSG_INVALID_CENTRY_TYPE_RELOP_S, attrname);  
                           ret = false;
                       }
                       if (lGetBool(centry, CE_consumable)) {
                           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                                   MSG_INVALID_CENTRY_CONSUMABLE_TYPE_SS, attrname, 
                                                   map_type2str(type));
                           ret = false;
                       }
         break;

      case TYPE_BOO : if ( relop != CMPLXEQ_OP ){
                           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR,
                                                   MSG_INVALID_CENTRY_TYPE_RELOP_S, attrname); 
                           ret = false;
                       } 
                       if (lGetBool(centry, CE_consumable)) {
                           answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR,
                                                   MSG_INVALID_CENTRY_CONSUMABLE_TYPE_SS, attrname, 
                                                   map_type2str(type));
                           ret = false;
                       }

         break;

      default : /* error unknown type */
                  answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                    MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U, u32c(type));
                  ret = false;
         break;
   } 

   {
      double dval;
      char error_msg[200];
      error_msg[0] = '\0';

      if (lGetBool(centry, CE_consumable)) {
   
         if (relop != CMPLXLE_OP) {
            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR,
                                    MSG_INVALID_CENTRY_CONSUMABLE_RELOP_S , attrname);
            ret = false;
         }

         if (lGetUlong(centry, CE_requestable) == REQU_NO) {
            if(!parse_ulong_val(&dval, NULL, type, lGetString(centry, CE_default), error_msg, 199)){
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                       MSG_INVALID_CENTRY_PARSE_DEFAULT_SS, attrname, error_msg);
               ret = false;
            }
            if (dval == 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                       MSG_INVALID_CENTRY_CONSUMABLE_REQ1_S, attrname);
               ret = false;
            }
         }
         else if (lGetUlong(centry, CE_requestable) == REQU_FORCED) {
            if(!parse_ulong_val(&dval, NULL, type, lGetString(centry, CE_default), error_msg, 199)){
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                    MSG_INVALID_CENTRY_PARSE_DEFAULT_SS, attrname, error_msg);
               ret = false;
            }
            if (dval != 0) {
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                       MSG_INVALID_CENTRY_CONSUMABLE_REQ2_S, attrname);
               ret = false;
            }
         }
      }
      else if ( (temp = lGetString(centry, CE_default)) ) {
      
         switch(type){
            case TYPE_INT:
            case TYPE_TIM:
            case TYPE_MEM:
            case TYPE_BOO:
            case TYPE_DOUBLE:
                              if(!parse_ulong_val(&dval, NULL, type, temp, error_msg, 199)){
                                 answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                                   MSG_INVALID_CENTRY_PARSE_DEFAULT_SS, attrname, error_msg);
                                 ret = false;
                              }
                              if (dval != 0) {
                                 answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                                   MSG_INVALID_CENTRY_DEFAULT_S, attrname);
                                 ret = false;
                              }
   
               break;
            case TYPE_HOST:
            case TYPE_STR:
            case TYPE_CSTR:
                           if (strcasecmp(temp, "NONE") != 0 ) {
                              answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                                   MSG_INVALID_CENTRY_DEFAULT_S, attrname);
                              ret = false;
                           }
               break;
            default:
               answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR, 
                                       MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U, u32c(type));
               ret = false;
         }
      }
   }

   /* check if its a build in value and if the type is correct */
   {
      int i;
      for ( i=0; i< max_queue_resources; i++){
         if (strcmp(queue_resource[i].name, attrname) == 0 &&
            queue_resource[i].type != lGetUlong(centry, CE_valtype)
         ){
             answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                       MSG_INVALID_CENTRY_TYPE_CHANGE_S, attrname);

            ret = false;
            break; 
         }
      }

      for ( i=0; i< max_host_resources; i++){
         if (strcmp(host_resource[i].name, attrname) == 0 &&
            host_resource[i].type != lGetUlong(centry, CE_valtype)
         ){
             answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                       MSG_INVALID_CENTRY_TYPE_CHANGE_S, attrname);

            ret = false;
            break; 
         }
      }
   }

   /* check for duplicates */
   if (centry_list) {
      const char *shortcut = lGetString(centry, CE_shortcut); 
      lListElem *current = lFirst(centry_list);
      while (current){

         if( strcmp(attrname, (temp = lGetString(current, CE_name))) == 0 ||
             strcmp(shortcut, temp) == 0 ||
             strcmp(attrname, (temp = lGetString(current, CE_shortcut))) == 0 ||
             strcmp(shortcut, temp) == 0) {
               if(current != centry){
                  answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN , ANSWER_QUALITY_ERROR, 
                                          MSG_ANSWER_COMPLEXXALREADYEXISTS_S, attrname);
                  ret = false;
                  break;
               }
         }        
         current = lNext(current); 
      }
   }
   DEXIT;
   return ret;
}

