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

#include "sge.h"
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
#include "sge_profiling.h"

#include "sge_schedd_conf.h"
#include "msg_schedd.h"

#include "cull_parse_util.h"

#include "sge_parse_num_par.h"

#include "msg_sgeobjlib.h"
#include "msg_common.h"


/******************************************************
 *
 * All configuration values are stored in one cull
 * master list: Master_Sched_Config_List. This list
 * has only one element: the config element (SC_Type).
 *
 * The configuratin is stored in a CULL list due to the
 * current configuration handling in the system. There
 * are methods outside this module, which need complete
 * access to the list. Therefore exist two methods to 
 * access the whole configuration:
 *
 * - sconf_get_config_list 
 *     (returns a pointer to the cull list 
 *
 * - sconf_get_config 
 *      returns a const pointer the config object.
 *
 * Both methods should only be used, when there is no
 * other way. Usualy the values in the configuration
 * should be accessed via access-function. The module
 * stores the position for each config element for
 * fast access, validates the configuration, when it
 * is changed, and pre-computes some values to return
 * them in the right way.
 *
 * If the configuration is changed directly in the CULL
 * list without using 
 *                "sconf_set_config"
 * , the 
 *               "sconf_validate_config" 
 * needs to be called to ensure that the internal data 
 * is updated.
 *
 * The access is not thread save. The current implementation
 * of the configuration does not allow an easy way to
 * make it thread save. But if only access functions
 * are used it should be easy to make this module thread
 * save, when the configuration is made thread save.
 *
 ******************************************************/



/* default values for scheduler configuration 
 * not all defaults are defined here :-)
 */
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


/* 
 * addes a parameter to the config_pos.params list and evaluates the settings
 *
 * Parameters:
 * - lList *param_list : target list
 * - lList ** answer_list : error messages
 * - const char* param :  the character version of the paramter
 *
 * Return:
 * - bool : true, when everything was fine, otherwise false
 *
 * See:
 * - sconf_eval_set_profiling
 */
typedef bool (*setParam)(lList *param_list, lList **answer_list, const char* param);

/**
 * specifies an array of valid parameters and its validation functions
 */
typedef struct {
      const char* name;
      setParam setParam; 
}parameters_t;

/**
 * stores the positions of all structure elemens and some
 * precalculated settings.
 */
typedef struct{
   bool empty;          /* marks this structure as empty or set */
   
   int algorithm;       /* SGE settings */
   int schedule_interval;
   int maxujobs;
   int queue_sort_method;
   int job_load_adjustments;
   int load_adjustment_decay_time;
   int load_formula;
   int schedd_job_info;
   int flush_submit_sec;
   int flush_finish_sec;
   int params;
   
   int reprioritize_interval;  /* SGEEE settings */
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

   int weight_tickets_override;
   int share_override_tickets;
   int share_functional_shares;
   int max_functional_jobs_to_schedule;
   int report_pjob_tickets;
   int max_pending_tasks_per_job;
   int halflife_decay_list; 
   int policy_hierarchy;

   int c_is_schedd_job_info;       /* cached configuration */
   lList *c_schedd_job_info_range;
   lList *c_halflife_decay_list;   
   lList *c_params;
   int weight_ticket;
   int weight_waiting_time;
   int weight_deadline;
   int weight_urgency;
   int max_reservation;
   int weight_priority;
}config_pos_type;


static bool sconf_calc_pos(void);

static void sconf_clear_pos(void);

static bool sconf_eval_set_profiling(lList *param_list, lList **answer_list, const char* param); 
static bool sconf_eval_set_monitoring(lList *param_list, lList **answer_list, const char* param);

static char policy_hierarchy_enum2char(policy_type_t value);

static policy_type_t policy_hierarchy_char2enum(char character);

static int policy_hierarchy_verify_value(const char* value);

/* array structure. pre-init. Make sure that a default value is added
 * when the config_pos_type is edited
 */
static config_pos_type pos = {true, 
                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                       -1, -1, -1, -1, -1, -1, -1, -1,
                       SCHEDD_JOB_INFO_UNDEF, NULL, NULL, NULL, 
                       -1, -1, -1, -1, -1};

/*
 * a list of all valid "params" parameters
 */
const parameters_t params[] = {
   {"PROFILE",  sconf_eval_set_profiling},
   {"MONITOR",  sconf_eval_set_monitoring},
   {"NONE",     NULL},
   {NULL,       NULL}
};

/* stores the overall configuraion */
lList *Master_Sched_Config_List = NULL;

extern int do_profiling; 

const char *const policy_hierarchy_chars = "OFS";

/* SG: TODO: should be const */
int load_adjustment_fields[] = { CE_name, CE_stringval, 0 };
/* SG: TODO: should be const */
int usage_fields[] = { UA_name, UA_value, 0 };
const char *delis[] = {"=", ",", ""};

static int max_resources = QS_STATE_FULL;
static bool global_load_correction = 0;
static bool host_order_changed = true;
static u_long32 default_duration = MAX_ULONG32;
static u_long32 now_time;
static bool host_order_changed;
static int last_dispatch_type;


/****** sge_schedd_conf/clear_pos() ********************************************
*  NAME
*     clear_pos() -- resets the position information 
*
*  SYNOPSIS
*     static void clear_pos(void) 
*
*  FUNCTION
*     is needed, when a new configuration is set 
*
*******************************************************************************/
static void sconf_clear_pos(void){

         pos.empty = true;

         pos.algorithm = -1; 
         pos.schedule_interval = -1; 
         pos.maxujobs =  -1;
         pos.queue_sort_method = -1; 
         pos.job_load_adjustments = -1; 
         pos.load_formula =  -1;
         pos.schedd_job_info = -1; 
         pos.flush_submit_sec = -1; 
         pos.flush_finish_sec =  -1;
         pos.params = -1;
         
         pos.reprioritize_interval= -1; 
         pos.halftime = -1; 
         pos.usage_weight_list = -1; 
         pos.compensation_factor = -1; 
         pos.weight_user = -1; 
         pos.weight_project = -1; 
         pos.weight_jobclass = -1; 
         pos.weight_department = -1; 
         pos.weight_job = -1; 
         pos.weight_tickets_functional = -1; 
         pos.weight_tickets_share = -1; 
         pos.weight_tickets_override = -1; 
         pos.share_override_tickets = -1; 
         pos.share_functional_shares = -1; 
         pos.max_functional_jobs_to_schedule = -1; 
         pos.report_pjob_tickets = -1; 
         pos.max_pending_tasks_per_job =  -1;
         pos.halflife_decay_list = -1; 
         pos.policy_hierarchy = -1;

         pos.c_is_schedd_job_info = SCHEDD_JOB_INFO_UNDEF;
         if (pos.c_schedd_job_info_range)
            pos.c_schedd_job_info_range = lFreeList(pos.c_schedd_job_info_range);
            
         if (pos.c_halflife_decay_list)
            pos.c_halflife_decay_list = lFreeList(pos.c_halflife_decay_list);

         if (pos.c_params)
            pos.c_params = lFreeList(pos.c_params);

         pos.weight_ticket = -1;
         pos.weight_waiting_time = -1;
         pos.weight_deadline = -1;
         pos.weight_urgency = -1;
         pos.max_reservation = -1;
         pos.weight_priority = -1;
}

static bool sconf_calc_pos(void){
   bool ret = true;

   DENTER(TOP_LAYER, "sconf_calc_pos");

   if (pos.empty) {
      const lListElem *config = sconf_get_config(); 

      if (config) {
         pos.empty = false;
/* SGE */         
         ret &= (pos.algorithm = lGetPosViaElem(config, SC_algorithm )) != -1; 
         ret &= (pos.schedule_interval = lGetPosViaElem(config, SC_schedule_interval)) != -1; 
         ret &= (pos.maxujobs = lGetPosViaElem(config, SC_maxujobs)) != -1;
         ret &= (pos.queue_sort_method = lGetPosViaElem(config, SC_queue_sort_method)) != -1;

         ret &= (pos.job_load_adjustments = lGetPosViaElem(config,SC_job_load_adjustments )) != -1;
         ret &= (pos.load_adjustment_decay_time = lGetPosViaElem(config, SC_load_adjustment_decay_time)) != -1;
         ret &= (pos.load_formula = lGetPosViaElem(config, SC_load_formula)) != -1;
         ret &= (pos.schedd_job_info = lGetPosViaElem(config, SC_schedd_job_info)) != -1;
         ret &= (pos.flush_submit_sec = lGetPosViaElem(config, SC_flush_submit_sec)) != -1;
         ret &= (pos.flush_finish_sec = lGetPosViaElem(config, SC_flush_finish_sec)) != -1;
         ret &= (pos.params = lGetPosViaElem(config, SC_params)) != -1;

/* SGEEE */
         ret &= (pos.reprioritize_interval = lGetPosViaElem(config, SC_reprioritize_interval)) != -1;
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
         ret &= (pos.weight_tickets_override = lGetPosViaElem(config, SC_weight_tickets_override)) != -1;

         ret &= (pos.share_override_tickets = lGetPosViaElem(config, SC_share_override_tickets)) != -1;
         ret &= (pos.share_functional_shares = lGetPosViaElem(config, SC_share_functional_shares)) != -1;
         ret &= (pos.max_functional_jobs_to_schedule = lGetPosViaElem(config, SC_max_functional_jobs_to_schedule)) != -1;
         ret &= (pos.report_pjob_tickets = lGetPosViaElem(config, SC_report_pjob_tickets)) != -1;
         ret &= (pos.max_pending_tasks_per_job = lGetPosViaElem(config, SC_max_pending_tasks_per_job)) != -1;
         ret &= (pos.halflife_decay_list = lGetPosViaElem(config, SC_halflife_decay_list)) != -1;
         ret &= (pos.policy_hierarchy = lGetPosViaElem(config, SC_policy_hierarchy)) != -1;

         ret &= (pos.weight_ticket = lGetPosViaElem(config, SC_weight_ticket)) != -1;
         ret &= (pos.weight_waiting_time = lGetPosViaElem(config, SC_weight_waiting_time)) != -1;
         ret &= (pos.weight_deadline = lGetPosViaElem(config, SC_weight_deadline)) != -1;
         ret &= (pos.weight_urgency = lGetPosViaElem(config, SC_weight_urgency)) != -1;
         ret &= (pos.weight_priority = lGetPosViaElem(config, SC_weight_priority)) != -1;
         ret &= (pos.max_reservation = lGetPosViaElem(config, SC_max_reservation)) != -1;
      }
      else
         ret = false;
   }

   DEXIT;
   return ret;
}

/****** sge_schedd_conf/schedd_conf_set_config() *******************************
*  NAME
*     schedd_conf_set_config() -- overwrites the existing configuration 
*
*  SYNOPSIS
*     bool schedd_conf_set_config(lList **config, lList **answer_list) 
*
*  FUNCTION
*    - validates the new configuration 
*    - precalculates some values and caches them
*    - stores the position of each attribute in the structure
*    - and sets the new configuration, if the validation worked
*
*    If the new configuration is a NULL pointer, the current configuration
*    if deleted.
*
*  INPUTS
*     lList **config      - new configuration (SC_Type)
*     lList **answer_list - error messages 
*
*  RESULT
*     bool - true, if it worked
*******************************************************************************/
bool sconf_set_config(lList **config, lList **answer_list){
   lList *store = Master_Sched_Config_List;
   bool ret = true;

   DENTER(TOP_LAYER,"sconf_set_config"); 
  
   if (config){
      Master_Sched_Config_List = *config;
      if ((ret = sconf_validate_config_(answer_list))){
         lFreeList(store);
         *config = NULL;
      }
      else{
         Master_Sched_Config_List = store;
         if (!Master_Sched_Config_List){
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_USE_DEFAULT_CONFIG)); 
            answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_WARNING);
 
            Master_Sched_Config_List = lCreateList("schedd config list", SC_Type);
            lAppendElem(Master_Sched_Config_List, sconf_create_default());

         }
         sconf_validate_config_(NULL);
      }   
   }
   else{
      Master_Sched_Config_List = lFreeList(Master_Sched_Config_List);
      sconf_clear_pos();
   }
   DEXIT;
   return ret;
}

/****** sge_schedd_conf/sconf_get_param() **************************************
*  NAME
*     sconf_get_param() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_get_param(const char *name) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char *name - ??? 
*
*  RESULT
*     const char* - 
*******************************************************************************/
const char *sconf_get_param(const char *name){
   lListElem *elem = lGetElemStr(pos.c_params, PARA_name, name); 
   
   return lGetString(elem, PARA_value);
}

/****** sge_schedd_conf/sconf_is_valid_load_formula_() *******************
*  NAME
*     sconf_is_valid_load_formula_() -- ??? 
*
*  SYNOPSIS
*     bool sconf_is_valid_load_formula_(lList **answer_list, lList 
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
bool sconf_is_valid_load_formula_(lList **answer_list,
                                  lList *centry_list)
{
   return sconf_is_valid_load_formula( lFirst(Master_Sched_Config_List),
                                    answer_list, centry_list);
}
/****** sge_schedd_conf/sconf_is_valid_load_formula() ********************
*  NAME
*     sconf_is_valid_load_formula() -- ??? 
*
*  SYNOPSIS
*     bool sconf_is_valid_load_formula(lListElem *schedd_conf, lList 
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
bool sconf_is_valid_load_formula(lListElem *schedd_conf,
                                       lList **answer_list,
                                       lList *centry_list)
{
   const char *load_formula = NULL;
   bool ret = true;
   DENTER(TOP_LAYER, "sconf_is_valid_load_formula");

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

            if (type == TYPE_STR || type == TYPE_CSTR || type == TYPE_HOST || type == TYPE_RESTR) {
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


/****** sge_schedd_conf/sconf_create_default() ***************************
*  NAME
*     sconf_create_default() -- returns a default configuration 
*
*  SYNOPSIS
*     lListElem* sconf_create_default() 
*
*  FUNCTION
*     Creates a default configuration, but does not change the current
*     active configuration. A set config has to be used to make the
*     current configuration the active one.
*
*  RESULT
*     lListElem* - default configuration (SC_Type)
*
*******************************************************************************/
lListElem *sconf_create_default()
{
   lListElem *ep, *added;

   DENTER(TOP_LAYER, "sconf_create_default");

   ep = lCreateElem(SC_Type);

   lSetString(ep, SC_algorithm, "default");
   lSetString(ep, SC_schedule_interval, SCHEDULE_TIME);
   lSetUlong(ep, SC_maxujobs, MAXUJOBS);

   lSetUlong(ep, SC_queue_sort_method, QSM_LOAD);

   added = lAddSubStr(ep, CE_name, "np_load_avg", SC_job_load_adjustments, CE_Type);
   lSetString(added, CE_stringval, "0.50");

   lSetString(ep, SC_load_adjustment_decay_time, 
                     DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME);
   lSetString(ep, SC_load_formula, DEFAULT_LOAD_FORMULA);
   lSetString(ep, SC_schedd_job_info, SCHEDD_JOB_INFO);
   lSetUlong(ep, SC_flush_submit_sec, 0);
   lSetUlong(ep, SC_flush_finish_sec, 0);
   lSetString(ep, SC_params, "none");
   
   lSetString(ep, SC_reprioritize_interval, SGEEE_SCHEDULE_TIME);
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

   lSetBool(ep, SC_share_override_tickets, true);  
   lSetBool(ep, SC_share_functional_shares, true);
   lSetUlong(ep, SC_max_functional_jobs_to_schedule, 200);
   lSetBool(ep, SC_report_pjob_tickets, true);
   lSetUlong(ep, SC_max_pending_tasks_per_job, 50);
   lSetString(ep, SC_halflife_decay_list, "none"); 
   lSetString(ep, SC_policy_hierarchy, policy_hierarchy_chars );

   lSetDouble(ep, SC_weight_ticket, 0.5);
   lSetDouble(ep, SC_weight_waiting_time, 0.278); 
   lSetDouble(ep, SC_weight_deadline, 3600000 );
   lSetDouble(ep, SC_weight_urgency, 0.5 );
   lSetUlong(ep, SC_max_reservation, 0);
   lSetDouble(ep, SC_weight_priority, 0.0 );

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
   const lListElem *sc_ep = sconf_get_config(); 
      
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
   const lListElem *sc_ep =  sconf_get_config();
      
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
   const lListElem *sc_ep =  sconf_get_config();
      
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
   const lListElem *sc_ep =  sconf_get_config();
      
   if (pos.queue_sort_method != -1) 
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
   const lListElem *sc_ep =  sconf_get_config();
      
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
   const lListElem *sc_ep =  sconf_get_config();
      
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


/****** sge_schedd_conf/sconf_reprioritize_interval_str() ********************
*  NAME
*     sconf_reprioritize_interval_str() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_reprioritize_interval_str(void) 
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
const char *sconf_reprioritize_interval_str(void){
   const lListElem *sc_ep =  sconf_get_config();
      
   if (pos.reprioritize_interval!= -1) 
      return lGetPosString(sc_ep, pos.reprioritize_interval);
   else
      return SGEEE_SCHEDULE_TIME;
}

/****** sge_schedd_conf/sconf_get_reprioritize_interval() ********************
*  NAME
*     sconf_get_reprioritize_interval() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_reprioritize_interval(void) 
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
u_long32 sconf_get_reprioritize_interval(void) {
   u_long32 uval;
   const char *time;
   time = sconf_reprioritize_interval_str();

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
/*   const lListElem *sc_ep =  sconf_get_config();
      
   if (pos.is_schedd_job_info != -1) 
      lSetPosUlong(sc_ep, pos.is_schedd_job_info , SCHEDD_JOB_INFO_TRUE);*/
   pos.c_is_schedd_job_info = SCHEDD_JOB_INFO_TRUE;
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
/*   const lListElem *sc_ep =  sconf_get_config();
   
   if (pos.is_schedd_job_info != -1) 
      lSetPosUlong(sc_ep, pos.is_schedd_job_info , SCHEDD_JOB_INFO_FALSE);*/
   pos.c_is_schedd_job_info = SCHEDD_JOB_INFO_FALSE;  
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

   return pos.c_is_schedd_job_info;
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
   
   return  pos.c_schedd_job_info_range;
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
   const lListElem *sc_ep =  sconf_get_config();
   
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
   const lListElem *sc_ep =  sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config(); 

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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

   if (pos.weight_tickets_functional != -1)
      return lGetPosUlong(sc_ep, pos.weight_tickets_functional);
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
   const lListElem *sc_ep =  sconf_get_config();

   if (pos.halftime != -1)
      return lGetPosUlong(sc_ep, pos.halftime);
   else
      return 0;
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
   const lListElem *sc_ep = sconf_get_config();

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
   const lListElem *sc_ep = sconf_get_config();

   if (pos.compensation_factor!= -1)
      return lGetPosDouble(sc_ep, pos.compensation_factor);
   else
      return true;
}

/****** sge_schedd_conf/sconf_get_weight_ticket() ****************************
*  NAME
*     sconf_get_weight_ticket() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_ticket(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*******************************************************************************/
double sconf_get_weight_ticket(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.weight_ticket != -1)
      return lGetPosDouble(sc_ep, pos.weight_ticket);
   else
      return 0;
}     

/****** sge_schedd_conf/sconf_get_weight_waiting_time() ************************
*  NAME
*     sconf_get_weight_waiting_time() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_waiting_time(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*******************************************************************************/
double sconf_get_weight_waiting_time(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.weight_waiting_time != -1)
      return lGetPosDouble(sc_ep, pos.weight_waiting_time);
   else
      return 0;
}     

/****** sge_schedd_conf/sconf_get_weight_deadline() ****************************
*  NAME
*     sconf_get_weight_deadline() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_deadline(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*******************************************************************************/
double sconf_get_weight_deadline(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.weight_deadline != -1)
      return lGetPosDouble(sc_ep, pos.weight_deadline);
   else
      return 0;
}     

/****** sge_schedd_conf/sconf_get_weight_urgency() ****************************
*  NAME
*     sconf_get_weight_urgency() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_urgency(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*******************************************************************************/
double sconf_get_weight_urgency(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.weight_urgency != -1)
      return lGetPosDouble(sc_ep, pos.weight_urgency);
   else
      return 0;
}     

/****** sge_schedd_conf/sconf_get_max_reservations() ***************************
*  NAME
*     sconf_get_max_reservations() -- Max reservation tuning parameter
*
*  SYNOPSIS
*     int sconf_get_max_reservations(void) 
*
*  FUNCTION
*     Tuning parameter. 
*     Returns maximum number of reservations that should be done by 
*     scheduler. If 0 is returned this no single job shall get a reservation
*     and assignments are to be made for 'now' only.
*     
*  RESULT
*     int - Max. number of reservations
*******************************************************************************/
u_long32 sconf_get_max_reservations(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.max_reservation != -1)
      return lGetPosUlong(sc_ep, pos.max_reservation);
   else
      return 0;
}

/****** sge_schedd_conf/sconf_get_weight_priority() ****************************
*  NAME
*     sconf_get_weight_priority() -- ??? 
*
*  SYNOPSIS
*     double sconf_get_weight_priority(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     double - 
*******************************************************************************/
double sconf_get_weight_priority(void) {
   lListElem *sc_ep = lFirst(Master_Sched_Config_List);
   
   if (pos.empty)
      sconf_calc_pos();
      
   if (pos.weight_priority != -1)
      return lGetPosDouble(sc_ep, pos.weight_priority);
   else
      return 0;
}     

/****** sge_schedd_conf/sconf_get_share_override_tickets() *********************
*  NAME
*     sconf_get_share_override_tickets() -- ??? 
*
*  SYNOPSIS
*     bool sconf_get_share_override_tickets(void) 
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
bool sconf_get_share_override_tickets(void) {
   const lListElem *sc_ep = sconf_get_config();

   if (pos.share_override_tickets != -1)
      return lGetPosBool(sc_ep, pos.share_override_tickets);
   else
      return true;
}
/****** sge_schedd_conf/sconf_get_share_functional_shares() ********************
*  NAME
*     sconf_get_share_functional_shares() -- ??? 
*
*  SYNOPSIS
*     bool sconf_get_share_functional_shares(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     bool - 
*******************************************************************************/
bool sconf_get_share_functional_shares(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.share_functional_shares != -1)
      return lGetPosBool(sc_ep, pos.share_functional_shares);
   else
      return true;


}

/****** sge_schedd_conf/sconf_get_report_pjob_tickets() *************************
*  NAME
*     sconf_get_report_pjob_tickets() -- ??? 
*
*  SYNOPSIS
*     bool sconf_get_report_pjob_tickets(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     bool - 
*******************************************************************************/
bool sconf_get_report_pjob_tickets(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.report_pjob_tickets!= -1)
      return lGetPosBool(sc_ep, pos.report_pjob_tickets);
   else
      return true;

}

/****** sge_schedd_conf/sconf_get_flush_submit_sec() ***************************
*  NAME
*     sconf_get_flush_submit_sec() -- ??? 
*
*  SYNOPSIS
*     int sconf_get_flush_submit_sec(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     int - 
*******************************************************************************/
u_long32 sconf_get_flush_submit_sec(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.flush_submit_sec!= -1)
        return lGetPosUlong(sc_ep, pos.flush_submit_sec);
   else
      return -1;
}
   
/****** sge_schedd_conf/sconf_get_flush_finish_sec() ***************************
*  NAME
*     sconf_get_flush_finish_sec() -- ??? 
*
*  SYNOPSIS
*     int sconf_get_flush_finish_sec(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     int - 
*******************************************************************************/
u_long32 sconf_get_flush_finish_sec(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.flush_finish_sec!= -1)
      return lGetPosUlong(sc_ep, pos.flush_finish_sec);
   else
      return -1;
}
   
/****** sge_schedd_conf/sconf_get_max_functional_jobs_to_schedule() ************
*  NAME
*     sconf_get_max_functional_jobs_to_schedule() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_max_functional_jobs_to_schedule(void) 
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
u_long32 sconf_get_max_functional_jobs_to_schedule(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.max_functional_jobs_to_schedule != -1)
      return lGetPosUlong(sc_ep, pos.max_functional_jobs_to_schedule);
   else
      return 200;
}

/****** sge_schedd_conf/sconf_get_max_pending_tasks_per_job() ******************
*  NAME
*     sconf_get_max_pending_tasks_per_job() -- ??? 
*
*  SYNOPSIS
*     u_long32 sconf_get_max_pending_tasks_per_job(void) 
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
u_long32 sconf_get_max_pending_tasks_per_job(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.max_pending_tasks_per_job != -1)
      return lGetPosUlong(sc_ep, pos.max_pending_tasks_per_job);
   else
      return 50;
}

/****** sge_schedd_conf/sconf_get_halflife_decay_list_str() ********************
*  NAME
*     sconf_get_halflife_decay_list_str() -- ??? 
*
*  SYNOPSIS
*     const char* sconf_get_halflife_decay_list_str(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const char* - 
*******************************************************************************/
const char *sconf_get_halflife_decay_list_str(void){
   const lListElem *sc_ep = sconf_get_config();

   if (pos.halflife_decay_list != -1)
      return lGetPosString(sc_ep, pos.halflife_decay_list);
   else
      return "none";

}

/****** sge_schedd_conf/sconf_get_halflife_decay_list() ************************
*  NAME
*     sconf_get_halflife_decay_list() -- ??? 
*
*  SYNOPSIS
*     const lList* sconf_get_halflife_decay_list(void) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     void - ??? 
*
*  RESULT
*     const lList* - 
*******************************************************************************/
const lList* sconf_get_halflife_decay_list(void){

      return pos.c_halflife_decay_list;
}

/****** sge_schedd_conf/sconf_is() *********************************************
*  NAME
*     sconf_is() -- checks, if a configuration exists
*
*  SYNOPSIS
*     bool sconf_is(void) 
*
*  RESULT
*     bool - true, if a configuration exists
*
*******************************************************************************/
bool sconf_is(void) {
   const lListElem *sc_ep = NULL;
   
   if (Master_Sched_Config_List)
      sc_ep = lFirst(Master_Sched_Config_List);

   return sc_ep != NULL;
}

/****** sge_schedd_conf/sconf_get_config() *************************************
*  NAME
*     sconf_get_config() -- returns a config object.  
*
*  SYNOPSIS
*     const lListElem* sconf_get_config(void) 
*
*  RESULT
*     const lListElem* - the current config object
*
* NOTE
*  DO NOT USE this method. ONLY when there is NO OTHER way. All config
*  settings can be accessed via access function.
*
*******************************************************************************/
const lListElem *sconf_get_config(void){
   return lFirst(Master_Sched_Config_List);
}

/****** sge_schedd_conf/sconf_get_config_list() ********************************
*  NAME
*     sconf_get_config_list() -- returns a pointer to the list, which contains
*                                the configuration
*
*  SYNOPSIS
*     lList** sconf_get_config_list(void) 
*
*  RESULT
*     lList** - pointer to the config list
*
* NOTE
*  DO NOT USE this method. ONLY when there is NO OTHER way. All config
*  settings can be accessed via access function. The config can be set
*  via sconf_set_config(...)
*
* IMPORTANT
*
*  If you modify the configuration by directly accessing, you have to call
*  sconf_validate_config_ afterwards to ensure, that the caches reflect
*  your changes.
*
*******************************************************************************/
lList **sconf_get_config_list(void){
   return &Master_Sched_Config_List;
}

/****** sge_schedd_conf/sconf_print_config() ***********************************
*  NAME
*     sconf_print_config() -- prints the current configuration to the INFO stream 
*
*  SYNOPSIS
*     void sconf_print_config() 
*
*******************************************************************************/
void sconf_print_config(void){

   char tmp_buffer[1024];
   u_long32 uval;
   const char *s;
   const lList *lval= NULL;
   double dval;

   DENTER(TOP_LAYER, "sconf_print_config");

   sconf_clear_pos();

   if (!sconf_is()){
      ERROR((SGE_EVENT, "sconf_printf_config: no config to validate\n"));
      return;
   }

   if (pos.empty){
      sconf_validate_config_(NULL);
   }

   /* --- SC_algorithm */
   s = sconf_get_algorithm();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXASY_SS , s, "algorithm"));
     
   /* --- SC_schedule_interval */
   s = sconf_get_schedule_interval_str();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS , s, "schedule_interval"));

   /* --- SC_maxujobs */
   uval = sconf_get_maxujobs();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US, u32c( uval), "maxujobs"));

   /* --- SC_queue_sort_method (was: SC_sort_seq_no) */
   uval = sconf_get_queue_sort_method();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "queue_sort_method"));

   /* --- SC_flush_submit_sec */
   uval = sconf_get_flush_submit_sec();
   INFO((SGE_EVENT,MSG_ATTRIB_USINGXFORY_US,  u32c (uval) , "flush_submit_sec"));

   /* --- SC_flush_finish_sec */
   uval = sconf_get_flush_finish_sec();
   INFO((SGE_EVENT,MSG_ATTRIB_USINGXFORY_US,  u32c (uval) , "flush_finish_sec"));

   /* --- SC_job_load_adjustments */
   lval = sconf_get_job_load_adjustments();
   uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), lval, load_adjustment_fields, delis, 0);
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "job_load_adjustments"));

   /* --- SC_load_adjustment_decay_time */
   s = sconf_get_load_adjustment_decay_time_str();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "load_adjustment_decay_time"));

   /* --- SC_load_formula */
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, sconf_get_load_formula(), "load_formula"));

   /* --- SC_schedd_job_info */
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, lGetString(lFirst(Master_Sched_Config_List), SC_schedd_job_info), "schedd_job_info"));

   /* --- SC_params */
   s=lGetString(lFirst(Master_Sched_Config_List), SC_params);
   INFO((SGE_EVENT, MSG_READ_PARAM_S, s)); 

   /* --- SC_reprioritize_interval */
   s = sconf_reprioritize_interval_str();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "reprioritize_interval"));

   /* --- SC_halftime */
   uval = sconf_get_halftime();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US ,  u32c (uval), "halftime"));

   /* --- SC_usage_weight_list */
   uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), sconf_get_usage_weight_list(), usage_fields, delis, 0);
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

   /* --- SC_share_override_tickets */
   uval = sconf_get_share_override_tickets();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "share_override_tickets"));
   
   /* --- SC_share_functional_shares */
   uval = sconf_get_share_functional_shares();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "share_functional_shares"));

   /* --- SC_max_functional_jobs_to_schedule */
   uval = sconf_get_max_functional_jobs_to_schedule();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "max_functional_jobs_to_schedule"));
   
   /* --- SC_report_job_tickets */
   uval = sconf_get_report_pjob_tickets();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "report_pjob_tickets"));
   
   /* --- SC_max_pending_tasks_per_job */
   uval = sconf_get_max_pending_tasks_per_job();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "max_pending_tasks_per_job"));

   /* --- SC_halflife_decay_list_str */
   s = sconf_get_halflife_decay_list_str();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "halflife_decay_list"));
  
   /* --- SC_policy_hierarchy */
   s = lGetString(lFirst(Master_Sched_Config_List), SC_policy_hierarchy);
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "policy_hierarchy"));

   /* --- SC_weight_ticket */
   dval = sconf_get_weight_ticket();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "weight_ticket"));

   /* --- SC_weight_waiting_time */
   dval = sconf_get_weight_waiting_time();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "weight_waiting_time"));

   /* --- SC_weight_deadline */
   dval = sconf_get_weight_deadline();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "weight_deadline"));

   /* --- SC_weight_urgency */
   dval = sconf_get_weight_urgency();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "weight_urgency"));

   /* --- SC_weight_priority */
   dval = sconf_get_weight_priority();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "weight_priority"));

   /* --- SC_max_reservation */
   dval = sconf_get_max_reservations();
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS,  dval, "max_reservation"));

   DEXIT;
   return;
}

/****** sge_schedd_conf/sconf_validate_config_() *******************************
*  NAME
*     sconf_validate_config_() -- validates the current config 
*
*  SYNOPSIS
*     bool sconf_validate_config_(lList **answer_list) 
*
*  FUNCTION
*     validates the current config and updates the caches. 
*
*  INPUTS
*     lList **answer_list - error messages 
*
*  RESULT
*     bool - false for invalid scheduler configuration
*
*******************************************************************************/
bool sconf_validate_config_(lList **answer_list){
   char tmp_buffer[1024], tmp_error[1024];
   u_long32 uval;
   const char *s;
   const lList *lval= NULL;
   bool ret = true;

   DENTER(TOP_LAYER, "sconf_validate_config_");

   sconf_clear_pos();

   if (!sconf_is()){
      DPRINTF(("sconf_validate: no config to validate\n"));
      return true;
   }

   if (!sconf_calc_pos()){
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
     
   /* --- SC_schedule_interval */
   s = sconf_get_schedule_interval_str();
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0) ) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , "schedule_interval", tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret =  false;
   }

   /* --- SC_job_load_adjustments */
   lval = sconf_get_job_load_adjustments();
   if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), lval, load_adjustment_fields, delis, 0) < 0) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SCHEDD_JOB_LOAD_ADJUSTMENTS_S, s)); 
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false;
   }

   /* --- SC_load_adjustment_decay_time */
   s = sconf_get_load_adjustment_decay_time_str();
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS, "load_adjustment_decay_time", tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false; 
   }

   /* --- SC_load_formula */
   if (Master_CEntry_List != NULL && !sconf_is_valid_load_formula_(answer_list, Master_CEntry_List )) {
      ret = false; 
   }

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
         else{
            pos.c_is_schedd_job_info = ikey;
            pos.c_schedd_job_info_range = rlp;
         }
      }
      else{
         pos.c_is_schedd_job_info = ikey;
      }
   }

   /* --- SC_params */
   {
      const char *sparams = lGetString(lFirst(Master_Sched_Config_List), SC_params); 
      char *s = NULL; 
      if (sparams) {
         for (s=sge_strtok(sparams, ",; "); s; s=sge_strtok(NULL, ",; ")) {
            int i = 0;
            bool added = false;
            for(i=0; params[i].name ;i++ ){
               if (!strncasecmp(s, params[i].name, sizeof(params[i].name)-1)){
                  if (params[i].setParam) {
                     ret &= params[i].setParam(pos.c_params, answer_list, s);
                  }
                  added = true;
               }               
            }
            if (!added){
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_UNKNOWN_PARAM_S, s));
               answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      } else {
         lSetString(lFirst(Master_Sched_Config_List), SC_params, "none");
      }     
   }

   /* --- SC_reprioritize_interval */
   s = sconf_reprioritize_interval_str();
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, s, tmp_error, sizeof(tmp_error),0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , "reprioritize_interval", tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false; 
   }

   /* --- SC_usage_weight_list */
   if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer), sconf_get_usage_weight_list(), usage_fields, delis, 0) < 0) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SCHEDD_USAGE_WEIGHT_LIST_S, tmp_error));    
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);      
      ret = false; 
   }

   /* --- SC_halflife_decay_list_str */
   {
      s = sconf_get_halflife_decay_list_str();
      if (!s || !strcasecmp(s, "none")) {
      } else {
         lList *halflife_decay_list = NULL;
         lListElem *ep;
         const char *s0,*s1,*s2,*s3;
         double value;
         struct saved_vars_s *sv1=NULL, *sv2=NULL;
         s0 = s; 
         for(s1=sge_strtok_r(s0, ":", &sv1); s1;
             s1=sge_strtok_r(NULL, ":", &sv1)) {
            if ((s2=sge_strtok_r(s1, "=", &sv2)) &&
                (s3=sge_strtok_r(NULL, "=", &sv2)) &&
                (sscanf(s3, "%lf", &value)==1)) {
               ep = lAddElemStr(&halflife_decay_list, UA_name, s2, UA_Type);
               lSetDouble(ep, UA_value, value);
            }
            if (sv2)
               free(sv2);
         }
         if (sv1)
            free(sv1);
         pos.c_halflife_decay_list = halflife_decay_list;   
      } 
   }
  
   /* --- SC_policy_hierarchy */
   {
      const char *value_string = lGetString(lFirst(Master_Sched_Config_List), SC_policy_hierarchy);
      if (value_string) {
         policy_hierarchy_t hierarchy[POLICY_VALUES];

         if (policy_hierarchy_verify_value(value_string) != 0) {
            answer_list_add(answer_list, MSG_GDI_INVALIDPOLICYSTRING, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);  
            ret = false;
            lSetString(lFirst(Master_Sched_Config_List), SC_policy_hierarchy, policy_hierarchy_chars);
         }
         sconf_ph_fill_array(hierarchy);
      } 
   }

   DEXIT;
   return ret;
}


/****** sge_schedd_conf/sconf_validate_config() ********************************
*  NAME
*     sconf_validate_config() -- validate a given configuration 
*
*  SYNOPSIS
*     bool sconf_validate_config(lList **answer_list, lList *config) 
*
*  INPUTS
*     lList **answer_list - error messages 
*     lList *config       - config to validate 
*
*  RESULT
*     bool - true, if the config is valid
*
*******************************************************************************/
bool sconf_validate_config(lList **answer_list, lList *config){
   lList *store = Master_Sched_Config_List;
   bool ret = true;

   DENTER(TOP_LAYER, "sconf_validate_config");
   
   if (config){
      Master_Sched_Config_List = config;
      ret = sconf_validate_config_(answer_list);
      
      Master_Sched_Config_List = store;
      sconf_validate_config_(NULL);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/conf/policy_hierarchy_verify_value() ***************************
*  NAME
*     policy_hierarchy_verify_value() -- verify a policy string 
*
*  SYNOPSIS
*     int policy_hierarchy_verify_value(const char* value) 
*
*  FUNCTION
*     The function tests whether the given policy string (value) is i
*     valid. 
*
*  INPUTS
*     const char* value - policy string 
*
*  RESULT
*     int - 0 -> OK
*           1 -> ERROR: one char is at least twice in "value"
*           2 -> ERROR: invalid char in "value"
*           3 -> ERROR: value == NULL
******************************************************************************/
static int policy_hierarchy_verify_value(const char* value) 
{
   int ret = 0;

   DENTER(TOP_LAYER, "policy_hierarchy_verify_value");

   if (value != NULL) {
      if (strcmp(value, "") && strcasecmp(value, "NONE")) {
         int is_contained[POLICY_VALUES]; 
         int i;

         for (i = 0; i < POLICY_VALUES; i++) {
            is_contained[i] = 0;
         }

         for (i = 0; i < strlen(value); i++) {
            char c = value[i];
            int index = policy_hierarchy_char2enum(c);

            if (is_contained[index]) {
               DPRINTF(("character \'%c\' is contained at least twice\n", c));
               ret = 1;
               break;
            } 

            is_contained[index] = 1;

            if (is_contained[INVALID_POLICY]) {
               DPRINTF(("Invalid character \'%c\'\n", c));
               ret = 2;
               break;
            }
         }
      }
   } else {
      ret = 3;
   }

   DEXIT;
   return ret;
}

/****** sgeobj/conf/sconf_ph_fill_array() *****************************
*  NAME
*     sconf_ph_fill_array() -- fill the policy array 
*
*  SYNOPSIS
*     void sconf_ph_fill_array(policy_hierarchy_t array[], 
*                                      const char *value) 
*
*  FUNCTION
*     Fill the policy "array" according to the characters given by 
*     "value".
*
*     value == "FODS":
*        policy_hierarchy_t array[4] = {
*            {FUNCTIONAL_POLICY, 1},
*            {OVERRIDE_POLICY, 1},
*            {DEADLINE_POLICY, 1},
*            {SHARE_TREE_POLICY, 1}
*        };
*
*     value == "FS":
*        policy_hierarchy_t array[4] = {
*            {FUNCTIONAL_POLICY, 1},
*            {SHARE_TREE_POLICY, 1},
*            {OVERRIDE_POLICY, 0},
*            {DEADLINE_POLICY, 0}
*        };
*
*     value == "OFS":
*        policy_hierarchy_t hierarchy[4] = {
*            {OVERRIDE_POLICY, 1},
*            {FUNCTIONAL_POLICY, 1},
*            {SHARE_TREE_POLICY, 1},
*            {DEADLINE_POLICY, 0}
*        }; 
*
*     value == "NONE":
*        policy_hierarchy_t hierarchy[4] = {
*            {OVERRIDE_POLICY, 0},
*            {FUNCTIONAL_POLICY, 0},
*            {SHARE_TREE_POLICY, 0},
*            {DEADLINE_POLICY, 0}
*        }; 
*
*  INPUTS
*     policy_hierarchy_t array[] - array with at least POLICY_VALUES 
*                                  values 
*     const char* value          - "NONE" or any combination
*                                  of the first letters of the policy 
*                                  names (e.g. "OFSD")
*
*  RESULT
*     "array" will be modified 
******************************************************************************/
void sconf_ph_fill_array(policy_hierarchy_t array[])
{
   int is_contained[POLICY_VALUES];
   int index = 0;
   int i;
   const char *policy_hierarchy_string = lGetPosString(lFirst(Master_Sched_Config_List), pos.policy_hierarchy);

   DENTER(TOP_LAYER, "sconf_ph_fill_array");

   for (i = 0; i < POLICY_VALUES; i++) {
      is_contained[i] = 0;
   }     
   if (policy_hierarchy_string != NULL && strcmp(policy_hierarchy_string, "") && strcasecmp(policy_hierarchy_string, "NONE")) {
      for (i = 0; i < strlen(policy_hierarchy_string); i++) {
         char c = policy_hierarchy_string[i];
         int enum_value = policy_hierarchy_char2enum(c); 

         is_contained[enum_value] = 1;
         array[index].policy = enum_value;
         array[index].dependent = 1;
         index++;
      }
   }
   for (i = INVALID_POLICY + 1; i < LAST_POLICY_VALUE; i++) {
      if (!is_contained[i])  {
         array[index].policy = i;
         array[index].dependent = 0;
         index++;
      }
   }

   DEXIT;
}

/****** sgeobj/conf/policy_hierarchy_char2enum() ******************************
*  NAME
*     policy_hierarchy_char2enum() -- Return value for a policy char
*
*  SYNOPSIS
*     policy_type_t policy_hierarchy_char2enum(char character) 
*
*  FUNCTION
*     This function returns a enum value for the first letter of a 
*     policy name. 
*
*  INPUTS
*     char character - "O", "F" or "S"
*
*  RESULT
*     policy_type_t - enum value 
******************************************************************************/
static policy_type_t policy_hierarchy_char2enum(char character)
{
   const char *pointer;
   policy_type_t ret;
   
   pointer = strchr(policy_hierarchy_chars, character);
   if (pointer != NULL) {
      ret = (pointer - policy_hierarchy_chars) + 1;
   } else {
      ret = INVALID_POLICY;
   }
   return ret;
}


/****** sgeobj/conf/sconf_ph_print_array() ****************************
*  NAME
*     sconf_ph_print_array() -- print hierarchy array 
*
*  SYNOPSIS
*     void sconf_ph_print_array(policy_hierarchy_t array[]) 
*
*  FUNCTION
*     Print hierarchy array in the debug output 
*
*  INPUTS
*     policy_hierarchy_t array[] - array with at least 
*                                  POLICY_VALUES values 
******************************************************************************/
void sconf_ph_print_array(policy_hierarchy_t array[])
{
   int i;

   DENTER(TOP_LAYER, "sconf_ph_print_array");
   
   for (i = INVALID_POLICY + 1; i < LAST_POLICY_VALUE; i++) {
      char character = policy_hierarchy_enum2char(array[i-1].policy);
      
      DPRINTF(("policy: %c; dependent: %d\n", character, array[i-1].dependent));
   }   

   DEXIT;
}

/****** sgeobj/conf/policy_hierarchy_enum2char() ******************************
*  NAME
*     policy_hierarchy_enum2char() -- Return policy char for a value 
*
*  SYNOPSIS
*     char policy_hierarchy_enum2char(policy_type_t value) 
*
*  FUNCTION
*     Returns the first letter of a policy name corresponding to the 
*     enum "value".
*
*  INPUTS
*     policy_type_t value - enum value 
*
*  RESULT
*     char - "O", "F", "S", "D"
******************************************************************************/
static char policy_hierarchy_enum2char(policy_type_t value) 
{
   return policy_hierarchy_chars[value - 1];
}

/****** sge_schedd_conf/sconf_eval_set_profiling() *****************************
*  NAME
*     sconf_eval_set_profiling() -- ??? 
*
*  SYNOPSIS
*     static bool sconf_eval_set_profiling(lList *param_list, lList 
*     **answer_list, const char* param) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList *param_list   - ??? 
*     lList **answer_list - ??? 
*     const char* param   - ??? 
*
*  RESULT
*     static bool - 
*******************************************************************************/
static bool sconf_eval_set_profiling(lList *param_list, lList **answer_list, const char* param){
   bool ret = true;
   lListElem *elem = NULL;
   DENTER(TOP_LAYER, "sconf_eval_set_profiling");

   do_profiling = false;

   if (!strncasecmp(param, "PROFILE=1", sizeof("PROFILE=1")-1) || 
       !strncasecmp(param, "PROFILE=TRUE", sizeof("PROFILE=FALSE")-1) ) {
      do_profiling = true;
      elem = lCreateElem(PARA_Type);
      lSetString(elem, PARA_name, "profile");
      lSetString(elem, PARA_value, "true");
   }      
   else if (!strncasecmp(param, "PROFILE=0", sizeof("PROFILE=1")-1) ||
            !strncasecmp(param, "PROFILE=FALSE", sizeof("PROFILE=FALSE")-1) ) {
      elem = lCreateElem(PARA_Type);
      lSetString(elem, PARA_name, "profile");
      lSetString(elem, PARA_value, "false");
   }
   else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INVALID_PARAM_SETTING_S, param)); 
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   if (elem){
      lAppendElem(pos.c_params, elem);
   }

   if(do_profiling && !prof_is_active()) {
         prof_start(NULL);
      }
   if(!do_profiling && prof_is_active()) {
      prof_stop(NULL);
   }
   
   DEXIT;
   return ret;
}

/****** sge_schedd_conf/sconf_eval_set_monitoring() ****************************
*  NAME
*     sconf_eval_set_monitoring() -- Control SERF on/off via MONITOR param 
*
*  SYNOPSIS
*     static bool sconf_eval_set_monitoring(lList *param_list, lList 
*     **answer_list, const char* param) 
*
*  FUNCTION
*     The MONITOR param allows schedule entry recording facility module 
*     be switched on/off. 
*
*  INPUTS
*     lList *param_list   - ??? 
*     lList **answer_list - ??? 
*     const char* param   - ??? 
*
*  RESULT
*     static bool - parsing error
*
*  NOTES
*     MT-NOTE: sconf_eval_set_monitoring() is not MT safe 
*******************************************************************************/
static bool sconf_eval_set_monitoring(lList *param_list, lList **answer_list, const char* param){
   bool ret = true;
   lListElem *elem = NULL;
   const char mon_true[] = "MONITOR=TRUE", mon_one[] = "MONITOR=1";
   const char mon_false[] = "MONITOR=FALSE", mon_zero[] = "MONITOR=0";
   bool do_monitoring = false;

   DENTER(TOP_LAYER, "sconf_eval_set_monitoring");


   if (!strncasecmp(param, mon_one, sizeof(mon_one)-1) || 
       !strncasecmp(param, mon_true, sizeof(mon_true)-1) ) {
      do_monitoring = true;
      elem = lCreateElem(PARA_Type);
      lSetString(elem, PARA_name, "monitor");
      lSetString(elem, PARA_value, "true");
   }      
   else if (!strncasecmp(param, mon_zero, sizeof(mon_zero)-1) ||
            !strncasecmp(param, mon_false, sizeof(mon_false)-1) ) {
      elem = lCreateElem(PARA_Type);
      lSetString(elem, PARA_name, "monitor");
      lSetString(elem, PARA_value, "false");
   }
   else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INVALID_PARAM_SETTING_S, param)); 
      answer_list_add(answer_list, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   if (elem){
      lAppendElem(pos.c_params, elem);
   }

   serf_set_active(do_monitoring);

   DEXIT;
   return ret;
}


/* 
   QS_STATE_FULL
      All debitations caused by running jobs are in effect.
   QS_STATE_EMPTY
      We ignore all debitations caused by running jobs.
      Ignore all but static load values.
*/
void sconf_set_qs_state(qs_state_t qs_state) 
{
   max_resources = qs_state;
}
qs_state_t sconf_get_qs_state(void) 
{
   return max_resources;
}
void sconf_set_global_load_correction(bool flag) 
{
   global_load_correction = flag;
}
bool sconf_get_global_load_correction(void) 
{
   return global_load_correction;
}

void sconf_set_default_duration(u_long32 duration) 
{
   default_duration = duration;
}
u_long32 sconf_get_default_duration(void) 
{
   return default_duration;
}

void sconf_set_now(u_long32 now)
{
   now_time = now;
}

u_long32 sconf_get_now(void)
{
   return now_time;
}

bool sconf_get_host_order_changed(void)
{
   return host_order_changed;
}

void sconf_set_host_order_changed(bool changed)
{
   host_order_changed = changed;
}

int sconf_get_last_dispatch_type(void)
{
   return last_dispatch_type;
}
void sconf_set_last_dispatch_type(int last)
{
   last_dispatch_type = last;
}

/****** sge_resource_utilization/serf_control() ********************************
*  NAME
*     serf_control() -- Switch recording on/off.
*
*  SYNOPSIS
*     void serf_control(bool on_off) 
*
*  FUNCTION
*     Allows recording be switched on/off.
*
*  INPUTS
*     bool on_off - true = on 
*                   false = off
*
*  NOTES
*     MT-NOTE: serf_control() is not MT safe 
*
*     Actually belongs to sge_serf.c but this would cause a link dependency 
*     libsgeobj -> libschedd !! 
*******************************************************************************/
static bool current_serf_do_monitoring = false;
void serf_set_active(bool on_off)
{
   current_serf_do_monitoring = on_off;
}
/****** sge_resource_utilization/serf_control() ********************************
*  NAME
*     serf_get_active() -- Retrieve whether SERF is active or not
*
*  SYNOPSIS
*     bool serf_get_active(void);
*
*  FUNCTION
*     Returns whether SERF is active or not
*
*  RETURN
*     bool - true = on 
*            false = off
*
*  NOTES
*     MT-NOTE: serf_get_active() is not MT safe 
*
*     Actually belongs to sge_serf.c but this would cause a link dependency 
*     libsgeobj -> libschedd !! 
*******************************************************************************/
bool serf_get_active(void)
{
   return current_serf_do_monitoring;
}
