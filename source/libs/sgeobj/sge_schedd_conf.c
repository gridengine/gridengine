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
#include "cull.h"
#include "sge_stdio.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_answer.h"
#include "sge_centry.h"
#include "sge_feature.h"
#include "sge_usage.h"

#include "sge_schedd_conf.h"

#include "msg_sgeobjlib.h"

/* default values for scheduler configuration */
#define DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME "0:7:30"
#define DEFAULT_LOAD_FORMULA                "np_load_avg"
#define SCHEDULE_TIME                       "0:0:15"
#define SGEEE_SCHEDULE_TIME                 "0:2:0"
#define MAXUJOBS                            0
#define MAXGJOBS                            0
#define SCHEDD_JOB_INFO                     "true"

lList *Master_Sched_Config_List = NULL;

bool schedd_conf_is_valid_load_formula(lListElem *schedd_conf,
                                       lList **answer_list,
                                       lList *centry_list)
{
   const char *load_formula = NULL;
   bool ret = true;
   DENTER(TOP_LAYER, "schedd_conf_is_valid_load_formula");

   /* Modify input */
   {
      char *new_load_formula = NULL;

      load_formula = lGetString(schedd_conf, SC_load_formula);
      new_load_formula = sge_strdup(new_load_formula, load_formula);
      sge_strip_blanks(new_load_formula);
      lSetString(schedd_conf, SC_load_formula, new_load_formula);
      sge_free(new_load_formula);
   }
   load_formula = lGetString(schedd_conf, SC_load_formula);

   /* Check for keyword 'none' */
   if (ret == true) {
      if (!strcasecmp(load_formula, "none")) {
         answer_list_add(answer_list, MSG_NONE_NOT_ALLOWED, STATUS_ESYNTAX, 
                         ANSWER_QUALITY_ERROR);
         ret = false;
      }
   }

   /* Check complex attributes and type */
   if (ret == true) {
      const char *delimitor = "+-*";
      const char *attr, *next_attr;

      next_attr = sge_strtok(load_formula, delimitor);
      while ((attr = next_attr)) {
         lListElem *cmplx_attr = NULL;

         next_attr = sge_strtok(NULL, delimitor);

         cmplx_attr = centry_list_locate(centry_list, attr);
         if (cmplx_attr != NULL) {
            int type = lGetUlong(cmplx_attr, CE_valtype);

            if (type == TYPE_STR || type == TYPE_CSTR || type == TYPE_HOST) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_WRONGTYPE_ATTRIBUTE_S, attr));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                               ANSWER_QUALITY_ERROR);
               ret = false;
            }
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_NOTEXISTING_ATTRIBUTE_S, attr));
            answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, 
                            ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   }
   DEXIT;
   return ret;
}

/******************************************************/
lListElem *schedd_conf_create_default()
{
   lListElem *ep, *added;

   DENTER(TOP_LAYER, "schedd_conf_create_default");

   ep = lCreateElem(SC_Type);

   /* 
    * 
    * SGE & SGEEE
    *
    */
   lSetString(ep, SC_algorithm, "default");
   lSetString(ep, SC_schedule_interval, SCHEDULE_TIME);
   lSetUlong(ep, SC_maxujobs, MAXUJOBS);

   if (feature_is_enabled(FEATURE_SGEEE))
      lSetUlong(ep, SC_queue_sort_method, QSM_SHARE);
   else
      lSetUlong(ep, SC_queue_sort_method, QSM_LOAD);

   added = lAddSubStr(ep, CE_name, "np_load_avg", SC_job_load_adjustments, CE_Type);
   lSetString(added, CE_stringval, "0.50");

   lSetString(ep, SC_load_adjustment_decay_time, 
                     DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME);
   lSetString(ep, SC_load_formula, DEFAULT_LOAD_FORMULA);
   lSetString(ep, SC_schedd_job_info, SCHEDD_JOB_INFO);


   /* 
    * 
    * SGEEE
    *
    */
   if (feature_is_enabled(FEATURE_SGEEE)) {
      lSetString(ep, SC_sgeee_schedule_interval, SGEEE_SCHEDULE_TIME);
      lSetUlong(ep, SC_halftime, 168);

      added = lAddSubStr(ep, UA_name, USAGE_ATTR_CPU, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 1.00);
      added = lAddSubStr(ep, UA_name, USAGE_ATTR_MEM, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 0.0);
      added = lAddSubStr(ep, UA_name, USAGE_ATTR_IO, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 0.0);

      lSetDouble(ep, SC_compensation_factor, 5);
      lSetDouble(ep, SC_weight_user, 0.2);
      lSetDouble(ep, SC_weight_project, 0.2);
      lSetDouble(ep, SC_weight_jobclass, 0.2);
      lSetDouble(ep, SC_weight_department, 0.2);
      lSetDouble(ep, SC_weight_job, 0.2);
      lSetUlong(ep, SC_weight_tickets_functional, 0);
      lSetUlong(ep, SC_weight_tickets_share, 0);
      lSetUlong(ep, SC_weight_tickets_deadline, 0);
   }

   DEXIT;
   return ep;
}

bool
sconf_is_centry_referenced(const lListElem *this_elem, const lListElem *centry)
{
   bool ret = false;

   DENTER(TOP_LAYER, "sconf_is_centry_referenced");
   if (this_elem != NULL) {
      const char *name = lGetString(centry, CE_name);
      lList *centry_list = lGetList(this_elem, SC_job_load_adjustments);
      lListElem *centry_ref = lGetElemStr(centry_list, CE_name, name);

      if (centry_ref != NULL) {
         ret = true;
      }
   }
   DEXIT;
   return ret;
}


