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
#include "sge_range.h"

#include "sge_schedd_conf.h"
#include "msg_schedd.h"

#include "cull_parse_util.h"

#include "sge_parse_num_par.h"

#include "msg_sgeobjlib.h"

/* default values for scheduler configuration */
#define DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME "0:7:30"
#define _DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME 7*60+30
#define DEFAULT_LOAD_FORMULA                "np_load_avg"
#define SCHEDULE_TIME                       "0:0:15"
#define _SCHEDULE_TIME                      15
#define SGEEE_SCHEDULE_TIME                 "0:2:0"
#define _SGEEE_SCHEDULE_TIME                 2*60
#define MAXUJOBS                            0
#define MAXGJOBS                            0
#define SCHEDD_JOB_INFO                     "true"
#define USER_SORT                           false

lList *Master_Sched_Config_List = NULL;

struct config_pos{
   bool empty;
   int algorithm;
   int schedule_interval;
   int maxujobs;
   int queue_sort_method;
   int user_sort;
   int job_load_adjustments;
   int load_adjustment_decay_time;
   int load_formula;
   int schedd_job_info;
   int is_schedd_job_info;
   int schedd_job_info_range;
   int sgeee_schedule_interval;
   int halftime;
   int usage_weight_list;
   int compensation_factor;
   int weight_user;
   int weight_project;
   int weight_jobclass;
   int weight_department;
   int weight_job;
   int weight_tickets_functional;
   int weight_tickets_share;
   int weight_tickets_deadline;
   int weight_tickets_deadline_active;
   int weight_tickets_override;
};

typedef struct config_pos config_pos_type;

config_pos_type pos = {true, -1, -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1, -1,  -1,  
                        -1,  -1, -1,  -1,  -1,  -1,  -1, -1,  -1,  -1,  -1, -1,  -1};

/* SG: TODO: should be const */
int load_adjustment_fields[] = { CE_name, CE_stringval, 0 };
/* SG: TODO: should be const */
int usage_fields[] = { UA_name, UA_value, 0 };
const char *delis[] = {"=", ",", ""};

static bool calc_pos(void);


bool calc_pos(void){
   bool ret = true;
   if (pos.empty) {
      lListElem *config = lFirst(Master_Sched_Config_List);
      if (config) {
         pos.empty = false;
         ret &= (pos.algorithm = lGetPosViaElem(config, SC_algorithm )) != -1; 
         ret &= (pos.schedule_interval = lGetPosViaElem(config, SC_schedule_interval)) != -1; 
         ret &= (pos.maxujobs = lGetPosViaElem(config, SC_maxujobs)) != -1;
         ret &= (pos.queue_sort_method = lGetPosViaElem(config, SC_queue_sort_method)) != -1;
         ret &= (pos.user_sort = lGetPosViaElem(config, SC_user_sort)) != -1;

         ret &= (pos.job_load_adjustments = lGetPosViaElem(config,SC_job_load_adjustments )) != -1;
         ret &= (pos.load_adjustment_decay_time = lGetPosViaElem(config, SC_load_adjustment_decay_time)) != -1;
         ret &= (pos.load_formula = lGetPosViaElem(config, SC_load_formula)) != -1;
         ret &= (pos.schedd_job_info = lGetPosViaElem(config, SC_schedd_job_info)) != -1;

         ret &= (pos.is_schedd_job_info = lGetPosViaElem(config, SC_is_schedd_job_info)) != -1;
         ret &= (pos.schedd_job_info_range = lGetPosViaElem(config, SC_schedd_job_info_range)) != -1;
         ret &= (pos.sgeee_schedule_interval = lGetPosViaElem(config, SC_sgeee_schedule_interval)) != -1;
         ret &= (pos.halftime = lGetPosViaElem(config, SC_halftime)) != -1;
         ret &= (pos.usage_weight_list = lGetPosViaElem(config, SC_usage_weight_list)) != -1;

         ret &= (pos.compensation_factor = lGetPosViaElem(config, SC_compensation_factor)) != -1;
         ret &= (pos.weight_user = lGetPosViaElem(config, SC_weight_user)) != -1;
         ret &= (pos.weight_project = lGetPosViaElem(config, SC_weight_project)) != -1;
         ret &= (pos.weight_jobclass = lGetPosViaElem(config, SC_weight_jobclass)) != -1;
         ret &= (pos.weight_department = lGetPosViaElem(config, SC_weight_department)) != -1;
         ret &= (pos.weight_job = lGetPosViaElem(config, SC_weight_job)) != -1;

         ret &= (pos.weight_tickets_functional = lGetPosViaElem(config, SC_weight_tickets_functional)) != -1;
         ret &= (pos.weight_tickets_share = lGetPosViaElem(config, SC_weight_tickets_share)) != -1;
         ret &= (pos.weight_tickets_deadline = lGetPosViaElem(config, SC_weight_tickets_deadline)) != -1;
         ret &= (pos.weight_tickets_deadline_active = lGetPosViaElem(config, SC_weight_tickets_deadline_active)) != -1;
         ret &= (pos.weight_tickets_override = lGetPosViaElem(config, SC_weight_tickets_override)) != -1;
      }
      else
         ret = false;
   }
   return ret;
}

/****** sge_schedd_conf/schedd_conf_is_valid_load_formula_() *******************
*  NAME
*     schedd_conf_is_valid_load_formula_() -- ??? 
*
*  SYNOPSIS
*     bool schedd_conf_is_valid_load_formula_(lList **answer_list, lList 
*     *centry_list) 
*
*  INPUTS
*     lList **answer_list - ??? 
*     lList *centry_list  - ??? 
*
*  RESULT
*     bool - 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool schedd_conf_is_valid_load_formula_(
                                       lList **answer_list,
                                       lList *centry_list)
{
   return schedd_conf_is_valid_load_formula(
            lFirst(Master_Sched_Config_List), answer_list, centry_list);
}
/****** sge_schedd_conf/schedd_conf_is_valid_load_formula() ********************
*  NAME
*     schedd_conf_is_valid_load_formula() -- ??? 
*
*  SYNOPSIS
*     bool schedd_conf_is_valid_load_formula(lListElem *schedd_conf, lList 
*     **answer_list, lList *centry_list) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lListElem *schedd_conf - ??? 
*     lList **answer_list    - ??? 
*     lList *centry_list     - ??? 
*
*  RESULT
*     bool - 
*
*******************************************************************************/
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


/****** sge_schedd_conf/schedd_conf_create_default() ***************************
*  NAME
*     schedd_conf_create_default() -- ??? 
*
*  SYNOPSIS
*     lListElem* schedd_conf_create_default() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     lListElem* - 
*
*******************************************************************************/
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
   lSetBool(ep, SC_user_sort, USER_SORT);
   lSetUlong(ep, SC_is_schedd_job_info, SCHEDD_JOB_INFO_UNDEF);
   lSetList(ep, SC_schedd_job_info_range, NULL);

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

/****** sge_schedd_conf/sconf_is_centry_referenced() ***************************
*  NAME
*     sconf_is_centry_referenced() -- ??? 
*
*  SYNOPSIS
*     bool sconf_is_centry_referenced(const lListElem *this_elem, const 
*     lListElem *centry) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const lListElem *this_elem - ??? 
*     const lListElem *centry    - ??? 
*
*  RESULT
*     bool - 
*
*******************************************************************************/
bool sconf_is_centry_referenced(const lListElem *this_elem, const lListElem *centry) {
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

/****** sge_schedd_conf/sconf_get_user_sort() **********************************
*  NAME
*     sconf_get_user_sort() -- ??? 
*
*  SYNOPSIS
*     bool sconf_get_user_sort() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     bool - 
*
*******************************************************************************/
bool sconf_get_user_sort() {
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
   if (pos.empty)
      calc_pos();
   if (pos.user_sort != -1) { 
      return lGetPosBool(sc_ep, pos.user_sort);
   }
   else {
      return USER_SORT;
   }
}

/****** sge_schedd_conf/sconf_weight_tickets_deadline() ************************
*  NAME
*     sconf_weight_tickets_deadline() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_weight_tickets_deadline(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*******************************************************************************/
u_long32 sconf_weight_tickets_deadline(void){
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List);
      
   if (pos.empty)
      calc_pos();

   if (pos.weight_tickets_deadline != -1) 
      return lGetPosUlong(sc_ep, pos.weight_tickets_deadline);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_load_adjustment_decay_time_str() *************
*  NAME
*     sconf_get_load_adjustment_decay_time_str() -- ??? 
*
*  SYNOPSIS
*     const char * sconf_get_load_adjustment_decay_time_str() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     const char * - 
*******************************************************************************/
const char * sconf_get_load_adjustment_decay_time_str(){
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.load_adjustment_decay_time != -1) 
      return lGetPosString(sc_ep, pos.load_adjustment_decay_time );
   else
      return DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME;
}

/****** sge_schedd_conf/sconf_get_load_adjustment_decay_time() *****************
*  NAME
*     sconf_get_load_adjustment_decay_time() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_load_adjustment_decay_time() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     u_long32 - 
*******************************************************************************/
u_long32 sconf_get_load_adjustment_decay_time() {
   u_long32 uval;
   const char *time = sconf_get_load_adjustment_decay_time_str();

   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, time, NULL, 0, 0)) {
      return _DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME;
   }
   return uval;
}

/****** sge_schedd_conf/sconf_get_job_load_adjustments() ***********************
*  NAME
*     sconf_get_job_load_adjustments() -- ??? 
*
*  SYNOPSIS
*     const lList* sconf_get_job_load_adjustments(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const lList* - 
*
*******************************************************************************/
const lList *sconf_get_job_load_adjustments(void) {
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.job_load_adjustments!= -1) 
      return lGetPosList(sc_ep, pos.job_load_adjustments); 
   else
      return NULL;
}

/****** sge_schedd_conf/sconf_get_load_formula() *******************************
*  NAME
*     sconf_get_load_formula() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_get_load_formula(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const char* - 
*
*******************************************************************************/
const char* sconf_get_load_formula(void) {
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.load_formula != -1) 
      return lGetPosString(sc_ep, pos.load_formula);
   else
      return DEFAULT_LOAD_FORMULA;
}

/****** sge_schedd_conf/sconf_get_queue_sort_method() **************************
*  NAME
*     sconf_get_queue_sort_method() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_queue_sort_method(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_queue_sort_method(void) {
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.queue_sort_method!= -1) 
      return lGetPosUlong(sc_ep, pos.queue_sort_method);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_maxujobs() ***********************************
*  NAME
*     sconf_get_maxujobs() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_maxujobs(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_maxujobs(void) {
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.maxujobs!= -1) 
      return lGetPosUlong(sc_ep, pos.maxujobs );
   else
      return MAXUJOBS;
}

/****** sge_schedd_conf/sconf_get_schedule_interval_str() **********************
*  NAME
*     sconf_get_schedule_interval_str() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_get_schedule_interval_str(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const char* - 
*
*******************************************************************************/
const char *sconf_get_schedule_interval_str(void){
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.schedule_interval != -1) 
      return lGetPosString(sc_ep, pos.schedule_interval );
   else
      return SCHEDULE_TIME;
}

/****** sge_schedd_conf/sconf_get_schedule_interval() **************************
*  NAME
*     sconf_get_schedule_interval() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_schedule_interval(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_schedule_interval(void) {
   u_long32 uval;   
   const char *time;
   time = sconf_get_schedule_interval_str();
  
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, time, NULL, 0, 0) ) {
         return _SCHEDULE_TIME;
   }
   return uval;
} 


/****** sge_schedd_conf/sconf_sgeee_schedule_interval_str() ********************
*  NAME
*     sconf_sgeee_schedule_interval_str() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_sgeee_schedule_interval_str(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const char* - 
*
*******************************************************************************/
const char *sconf_sgeee_schedule_interval_str(void){
   const lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
      
   if (pos.empty)
      calc_pos();

   if (pos.sgeee_schedule_interval != -1) 
      return lGetPosString(sc_ep, pos.sgeee_schedule_interval);
   else
      return SGEEE_SCHEDULE_TIME;
}

/****** sge_schedd_conf/sconf_get_sgeee_schedule_interval() ********************
*  NAME
*     sconf_get_sgeee_schedule_interval() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_sgeee_schedule_interval(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_sgeee_schedule_interval(void) {
   u_long32 uval;
   const char *time;
   time = sconf_sgeee_schedule_interval_str();

   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM,time, NULL, 0 ,0)) {
      return _SGEEE_SCHEDULE_TIME;
   }

   return uval;
}

/****** sge_schedd_conf/sconf_enable_schedd_job_info() *************************
*  NAME
*     sconf_enable_schedd_job_info() -- ??? 
*
*  SYNOPSIS
*     void sconf_enable_schedd_job_info() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     void - 
*
*******************************************************************************/
void sconf_enable_schedd_job_info() {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);    
      
   if (pos.empty)
      calc_pos();

   if (pos.is_schedd_job_info != -1) 
      lSetPosUlong(sc_ep, pos.is_schedd_job_info , SCHEDD_JOB_INFO_TRUE);
}

/****** sge_schedd_conf/sconf_disable_schedd_job_info() ************************
*  NAME
*     sconf_disable_schedd_job_info() -- ??? 
*
*  SYNOPSIS
*     void sconf_disable_schedd_job_info() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     void - 
*
*******************************************************************************/
void sconf_disable_schedd_job_info() {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
   
   if (pos.empty)
      calc_pos();

   if (pos.is_schedd_job_info != -1) 
      lSetPosUlong(sc_ep, pos.is_schedd_job_info , SCHEDD_JOB_INFO_FALSE);
}

/****** sge_schedd_conf/sconf_get_schedd_job_info() ****************************
*  NAME
*     sconf_get_schedd_job_info() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_schedd_job_info(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_schedd_job_info(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
   u_long32 is_info = SCHEDD_JOB_INFO_UNDEF;
   
   if (pos.empty)
      calc_pos();

   if (pos.is_schedd_job_info != -1) 
      is_info = lGetPosUlong(sc_ep, pos.is_schedd_job_info) ;

   if (is_info == SCHEDD_JOB_INFO_UNDEF){
      const char *range = lGetPosString(sc_ep, pos.schedd_job_info);
      char *buf = malloc (strlen(range) +1);
      char *key;
      lList *rlp=NULL;

      strcpy(buf, range);
      key = strtok(buf, " \t");
      if (!strcmp("true", key)) 
         is_info = SCHEDD_JOB_INFO_TRUE;
      else if (!strcmp("false", key)) 
         is_info = SCHEDD_JOB_INFO_FALSE;
      else if (!strcmp("job_list", key)) 
         is_info = SCHEDD_JOB_INFO_JOB_LIST;

      /* check list of groups */
      if (is_info == SCHEDD_JOB_INFO_JOB_LIST) {
         key = strtok(NULL, "\n");
         range_list_parse_from_string(&rlp, NULL, key, 0, 0, INF_NOT_ALLOWED);
      }
      lSetPosUlong(sc_ep, pos.is_schedd_job_info, is_info);
      lSetList(sc_ep, pos.schedd_job_info_range, NULL);

      free(buf); 
   }

   return is_info;
}

/****** sge_schedd_conf/sconf_get_schedd_job_info_range() **********************
*  NAME
*     sconf_get_schedd_job_info_range() -- ??? 
*
*  SYNOPSIS
*     const lList* sconf_get_schedd_job_info_range(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const lList* - 
*
*******************************************************************************/
const lList *sconf_get_schedd_job_info_range(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      calc_pos();
   
   if (pos.schedd_job_info_range != -1)
      return lGetPosList(sc_ep, pos.schedd_job_info_range);
   else
      return NULL;
}

/****** sge_schedd_conf/sconf_get_algorithm() **********************************
*  NAME
*     sconf_get_algorithm() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_get_algorithm(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const char* - 
*
*******************************************************************************/
const char *sconf_get_algorithm(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List); 
   
   if (pos.empty)
      calc_pos();
   
   if (pos.algorithm!= -1)
      return lGetPosString(sc_ep, pos.algorithm);
   else
      return "default";
}


/****** sge_schedd_conf/sconf_get_usage_weight_list() **************************
*  NAME
*     sconf_get_usage_weight_list() -- ??? 
*
*  SYNOPSIS
*     const lList* sconf_get_usage_weight_list(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const lList* - 
*
*******************************************************************************/
const lList *sconf_get_usage_weight_list(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List); 

   if (pos.empty)
      calc_pos();
   
   if (pos.usage_weight_list != -1)
      return lGetPosList(sc_ep, pos.usage_weight_list );
   else
      return NULL;
}

/****** sge_schedd_conf/sconf_get_weight_user() ********************************
*  NAME
*     sconf_get_weight_user() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_user(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_weight_user(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_user!= -1)
      return lGetPosDouble(sc_ep, pos.weight_user);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_department() **************************
*  NAME
*     sconf_get_weight_department() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_department(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_weight_department(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_department != -1)
      return lGetPosDouble(sc_ep, pos.weight_department);
   else  
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_project() *****************************
*  NAME
*     sconf_get_weight_project() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_project(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_weight_project(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_project != -1)
      return lGetPosDouble(sc_ep, pos.weight_project);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_jobclass() ****************************
*  NAME
*     sconf_get_weight_jobclass() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_jobclass(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_weight_jobclass(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_jobclass != -1)
      return lGetPosDouble(sc_ep, pos.weight_jobclass );
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_job() *********************************
*  NAME
*     sconf_get_weight_job() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_job(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_weight_job(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_job != -1)
      return lGetPosDouble(sc_ep, pos.weight_job);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_tickets_share() ***********************
*  NAME
*     sconf_get_weight_tickets_share() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_weight_tickets_share(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_weight_tickets_share(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_tickets_share != -1)
      return lGetPosUlong(sc_ep, pos.weight_tickets_share );
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_tickets_functional() ******************
*  NAME
*     sconf_get_weight_tickets_functional() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_weight_tickets_functional(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_weight_tickets_functional(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_tickets_functional != -1)
      return lGetPosUlong(sc_ep, pos.weight_tickets_functional);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_tickets_deadline() ********************
*  NAME
*     sconf_get_weight_tickets_deadline() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_weight_tickets_deadline(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_weight_tickets_deadline(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_tickets_deadline != -1)
      return lGetPosUlong(sc_ep, pos.weight_tickets_deadline);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_halftime() ***********************************
*  NAME
*     sconf_get_halftime() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_halftime(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     u_long32 - 
*
*******************************************************************************/
u_long32 sconf_get_halftime(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.halftime != -1)
      return lGetPosUlong(sc_ep, pos.halftime);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_set_weight_tickets_deadline_active() *************
*  NAME
*     sconf_set_weight_tickets_deadline_active() -- ??? 
*
*  SYNOPSIS
*     void sconf_set_weight_tickets_deadline_active(u_long32 active) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     u_long32 active - ??? 
*
*  RESULT
*     void - 
*
*******************************************************************************/
void sconf_set_weight_tickets_deadline_active(u_long32 active) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_tickets_deadline_active != -1)
      lSetPosUlong(sc_ep, pos.weight_tickets_deadline_active, active);
}

/****** sge_schedd_conf/sconf_set_weight_tickets_override() ********************
*  NAME
*     sconf_set_weight_tickets_override() -- ??? 
*
*  SYNOPSIS
*     void sconf_set_weight_tickets_override(u_long32 active) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     u_long32 active - ??? 
*
*  RESULT
*     void - 
*
*******************************************************************************/
void sconf_set_weight_tickets_override(u_long32 active) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.weight_tickets_override!= -1)
      lSetPosUlong(sc_ep, pos.weight_tickets_override, active);
}

/****** sge_schedd_conf/sconf_get_compensation_factor() ************************
*  NAME
*     sconf_get_compensation_factor() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_compensation_factor(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*
*******************************************************************************/
double sconf_get_compensation_factor(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);

   if (pos.empty)
      calc_pos();
   
   if (pos.compensation_factor != -1)
      return lGetPosDouble(sc_ep, pos.compensation_factor);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_is() *********************************************
*  NAME
*     sconf_is() -- ??? 
*
*  SYNOPSIS
*     bool sconf_is(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     bool - 
*
*******************************************************************************/
bool sconf_is(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   return sc_ep != NULL;
}

/****** sge_schedd_conf/sconf_get_config() *************************************
*  NAME
*     sconf_get_config() -- ??? 
*
*  SYNOPSIS
*     const lListElem* sconf_get_config(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const lListElem* - 
*
*******************************************************************************/
const lListElem *sconf_get_config(void){
   return lFirst(Master_Sched_Config_List);
}

/****** sge_schedd_conf/sconf_validate_config_() *******************************
*  NAME
*     sconf_validate_config_() -- ??? 
*
*  SYNOPSIS
*     bool sconf_validate_config_(lList **answer_list) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list - ??? 
*
*  RESULT
*     bool - 
*
*******************************************************************************/
bool sconf_validate_config_(lList **answer_list){
   char tmp_buffer[1024], tmp_error[1024];
   u_long32 uval;
   const char *s;
   const lList *lval= NULL;
   double dval;
   bool ret = true;

   DENTER(TOP_LAYER, "sconf_validate_config_");


   pos.empty = true;

   if (!calc_pos()){
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INCOMPLETE_SCHEDD_CONFIG)); 
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false; 
   }

   /* --- SC_algorithm */
   s = sconf_get_algorithm();
   if ( !s || (strcmp(s, "default") && strcmp(s, "simple_sched") && strncmp(s, "ext_", 4))) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_ALGORITHMNOVALIDNAME_S, s)); 
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false; 
   }
   else
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXASY_SS , s, "algorithm"));
     
   /* --- SC_schedule_interval */
   s = sconf_get_schedule_interval_str();
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0) ) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , "schedule_interval", tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret =  false;
   }
   else
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS , s, "schedule_interval"));

   /* --- SC_maxujobs */
   uval = sconf_get_maxujobs();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US, u32c( uval), "maxujobs"));

   /* --- SC_queue_sort_method (was: SC_sort_seq_no) */
   uval = sconf_get_queue_sort_method();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "queue_sort_method"));

   /* --- SC_user_sort */
   uval = sconf_get_user_sort();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS , uval?MSG_TRUE:MSG_FALSE, "user_sort"));

   /* --- SC_job_load_adjustments */
   lval = sconf_get_job_load_adjustments();
   if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), lval, load_adjustment_fields, delis, 0) < 0) {
      ret = false;
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "job_load_adjustments"));

   /* --- SC_load_adjustment_decay_time */
   s = sconf_get_load_adjustment_decay_time_str();
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS, "load_adjustment_decay_time", tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false; 
   }
   else
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "load_adjustment_decay_time"));

   /* --- SC_load_formula */
   if (Master_CEntry_List != NULL && !schedd_conf_is_valid_load_formula_(answer_list, Master_CEntry_List )) {
      ret = false; 
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, sconf_get_load_formula(), "load_formula"));

   /* --- SC_schedd_job_info */
   {
      char buf[4096];
      char* key;
      int ikey = 0;
      lList *rlp=NULL, *alp=NULL;

      strcpy(buf, lGetString(lFirst(Master_Sched_Config_List), SC_schedd_job_info));
      /* on/off or watch a set of jobs */
      key = strtok(buf, " \t");
      if (!strcmp("true", key)) 
         ikey = SCHEDD_JOB_INFO_TRUE;
      else if (!strcmp("false", key)) 
         ikey = SCHEDD_JOB_INFO_FALSE;
      else if (!strcmp("job_list", key)) 
         ikey = SCHEDD_JOB_INFO_JOB_LIST;
      else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_SCHEDDJOBINFONOVALIDPARAM ));
         answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         ret = false; 
      }
      /* check list of groups */
      if (ikey == SCHEDD_JOB_INFO_JOB_LIST) {
         key = strtok(NULL, "\n");
         range_list_parse_from_string(&rlp, &alp, key, 0, 0, INF_NOT_ALLOWED);
         if (rlp == NULL) {
            lFreeList(alp);
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_SCHEDDJOBINFONOVALIDJOBLIST));
            answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            ret = false; 
         }   
      }
      lFreeList(rlp);
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, lGetString(lFirst(Master_Sched_Config_List), SC_schedd_job_info), "schedd_job_info"));
 
   /**
    * check for SGEEE scheduler configurations
    */
   if (feature_is_enabled(FEATURE_SGEEE)) {

      /* --- SC_sgeee_schedule_interval */
      s = sconf_sgeee_schedule_interval_str();
      if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , "sgeee_schedule_interval", tmp_error));    
         answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         ret = false; 
      }
      else
         INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "sgeee_schedule_interval"));

      /* --- SC_halftime */
      uval = sconf_get_halftime();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US ,  u32c (uval), "halftime"));

      /* --- SC_usage_weight_list */
      
      if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), sconf_get_usage_weight_list(), usage_fields, delis, 0) < 0) {
         ret = false; 
      }
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "usage_weight_list"));

      /* --- SC_compensation_factor */
      dval = sconf_get_compensation_factor();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "compensation_factor"));

      /* --- SC_weight_user */
      dval = sconf_get_weight_user();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_user"));

      /* --- SC_weight_project */
      dval = sconf_get_weight_project();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_project"));

      /* --- SC_weight_jobclass */
      dval = sconf_get_weight_jobclass();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_jobclass"));

      /* --- SC_weight_department */
      dval = sconf_get_weight_department();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_department"));

      /* --- SC_weight_job */
      dval = sconf_get_weight_job();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_job"));

      /* --- SC_weight_tickets_functional */
      uval = sconf_get_weight_tickets_functional();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_functional"));

      /* --- SC_weight_tickets_share */
      uval = sconf_get_weight_tickets_share();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_share"));

      /* --- SC_weight_tickets_deadline */
      uval = sconf_weight_tickets_deadline();
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_deadline"));
   }

   DEXIT;
   return ret;

}


/****** sge_schedd_conf/sconf_validate_config() ********************************
*  NAME
*     sconf_validate_config() -- ??? 
*
*  SYNOPSIS
*     bool sconf_validate_config(lList **answer_list, lList *config) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list - ??? 
*     lList *config       - ??? 
*
*  RESULT
*     bool - 
*
*******************************************************************************/
bool sconf_validate_config(lList **answer_list, lList *config){
   lList *store = Master_Sched_Config_List;
   bool ret;
   Master_Sched_Config_List = config;
   ret = sconf_validate_config_(answer_list);
   Master_Sched_Config_List = store;
   return ret;
}

