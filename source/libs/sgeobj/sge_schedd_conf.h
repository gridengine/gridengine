#ifndef __SGE_SCHEDD_CONF_H
#define __SGE_SCHEDD_CONF_H
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

#include "sge_schedd_confL.h"

enum schedd_job_info_key {
   SCHEDD_JOB_INFO_FALSE=0,
   SCHEDD_JOB_INFO_TRUE,
   SCHEDD_JOB_INFO_JOB_LIST,
   SCHEDD_JOB_INFO_UNDEF
};


extern lList *Master_Sched_Config_List;

lListElem *schedd_conf_create_default(void);

bool schedd_conf_is_valid_load_formula_( lList **answer_list,
                                       lList *cmplx_list);

bool schedd_conf_is_valid_load_formula( lListElem *sc_ep,
                                       lList **answer_list,
                                       lList *cmplx_list);

bool
sconf_is_centry_referenced(const lListElem *this_elem, const lListElem *centry);

bool sconf_validate_config(lList **answer_list, lList *config);

bool sconf_validate_config_(lList **answer_list);

const lListElem *sconf_get_config(void);

bool sconf_is(void);

bool sconf_get_user_sort(void);

u_long32 sconf_get_load_adjustment_decay_time(void);

const lList *sconf_get_job_load_adjustments(void);

const char *sconf_get_schedule_interval_str(void);

const char *sconf_get_load_formula(void);

const char *sconf_get_load_adjustment_decay_time_str(void);

const char *sconf_sgeee_schedule_interval_str(void);

u_long32 sconf_get_queue_sort_method(void);

u_long32 sconf_get_maxujobs(void);

u_long32 sconf_get_schedule_interval(void);

u_long32 sconf_get_sgeee_schedule_interval(void);

u_long32 sconf_get_schedd_job_info(void);

u_long32 sconf_weight_tickets_share(void);

u_long32 sconf_weight_tickets_deadline(void);

void sconf_disable_schedd_job_info(void);

void sconf_enable_schedd_job_info(void);

const lList *sconf_get_schedd_job_info_range(void);

const char *sconf_get_algorithm(void);

const lList *sconf_get_usage_weight_list(void);

double sconf_get_weight_user(void);

double sconf_get_weight_department(void);

double sconf_get_weight_project(void);

double sconf_get_weight_jobclass(void);

double sconf_get_weight_job(void);

u_long32 sconf_get_weight_tickets_share(void);

u_long32 sconf_get_weight_tickets_functional(void);

u_long32 sconf_get_weight_tickets_deadline(void);

u_long32 sconf_get_halftime(void);

void sconf_set_weight_tickets_deadline_active(u_long32 active);

void sconf_set_weight_tickets_override(u_long32 active);

double sconf_get_compensation_factor(void);
#endif /* __SGE_SCHEDD_CONF_H */
