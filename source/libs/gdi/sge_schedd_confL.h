#ifndef __SGE_SCHEDCONFL_H
#define __SGE_SCHEDCONFL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */ 

/* 
 * valid values for SC_queue_sort_method 
 */
enum {
   QSM_LOAD = 0,
   QSM_SEQNUM = 1,
   QSM_SHARE = 2
};

/* 
 * This is the list type we use to hold the configuration of the scheduler.
 */
enum {
   SC_algorithm = SC_LOWERBOUND,
   SC_schedule_interval,
   SC_maxujobs,
   SC_queue_sort_method,
   SC_user_sort,
   SC_job_load_adjustments,
   SC_load_adjustment_decay_time,
   SC_load_formula,
   SC_schedd_job_info,
   SC_sgeee_schedule_interval,
   SC_halftime,
   SC_usage_weight_list,
   SC_compensation_factor,
   SC_weight_user,
   SC_weight_project,
   SC_weight_jobclass,
   SC_weight_department,
   SC_weight_job,
   SC_weight_tickets_functional,
   SC_weight_tickets_share,
   SC_weight_tickets_deadline,
   SC_weight_tickets_deadline_active,
   SC_weight_tickets_override
};


ILISTDEF(SC_Type, SchedConf, SGE_SC_LIST)
   /*
    * configuration values used by both SGE and SGEEE
    */
   SGE_STRING(SC_algorithm, CULL_DEFAULT)
   SGE_STRING(SC_schedule_interval, CULL_DEFAULT)
   SGE_ULONG(SC_maxujobs, CULL_DEFAULT)
   SGE_ULONG(SC_queue_sort_method, CULL_DEFAULT)    /* see at top of file for valid values */
   SGE_BOOL(SC_user_sort, CULL_DEFAULT)
   SGE_LIST(SC_job_load_adjustments, CE_Type, CULL_DEFAULT)        /* CE_Type */
   SGE_STRING(SC_load_adjustment_decay_time, CULL_DEFAULT)
   SGE_STRING(SC_load_formula, CULL_DEFAULT)
   SGE_STRING(SC_schedd_job_info, CULL_DEFAULT)

   /* 
    * SGEEE specific configuration values
    */
   SGE_STRING(SC_sgeee_schedule_interval, CULL_DEFAULT)
   SGE_ULONG(SC_halftime, CULL_DEFAULT)
   SGE_LIST(SC_usage_weight_list, UA_Type, CULL_DEFAULT)   /* SGEEE - UA_Type; gives *
                                               * weights for building the * 
                                               * usage usage = cpu * w_cpu
                                               * * + xxx * w_xxx + ... */
   SGE_DOUBLE(SC_compensation_factor, CULL_DEFAULT)

   SGE_DOUBLE(SC_weight_user, CULL_DEFAULT) /* gives weights between different *
                               * functional scheduling targets */
   SGE_DOUBLE(SC_weight_project, CULL_DEFAULT)
   SGE_DOUBLE(SC_weight_jobclass, CULL_DEFAULT)
   SGE_DOUBLE(SC_weight_department, CULL_DEFAULT)
   SGE_DOUBLE(SC_weight_job, CULL_DEFAULT)

   SGE_ULONG(SC_weight_tickets_functional, CULL_DEFAULT)    /* weight between different * 
                                               * scheduling targets */
   SGE_ULONG(SC_weight_tickets_share, CULL_DEFAULT)
   SGE_ULONG(SC_weight_tickets_deadline, CULL_DEFAULT)
   SGE_ULONG(SC_weight_tickets_deadline_active, CULL_DEFAULT)   /* prepared for setting by a 
                                                   * schedd order */
   SGE_ULONG(SC_weight_tickets_override, CULL_DEFAULT)          /* not yet implemented */
LISTEND 

NAMEDEF(SCN)
   NAME("SC_algorithm")
   NAME("SC_schedule_interval")
   NAME("SC_maxujobs")
   NAME("SC_queue_sort_method")
   NAME("SC_user_sort")
   NAME("SC_job_load_adjustments")
   NAME("SC_load_adjustment_decay_time")
   NAME("SC_load_formula")
   NAME("SC_schedd_job_info")

   NAME("SC_sgeee_schedule_interval")
   NAME("SC_halftime")
   NAME("SC_usage_weight_list")
   NAME("SC_compensation_factor")

   NAME("SC_weight_user")
   NAME("SC_weight_project")
   NAME("SC_weight_jobclass")
   NAME("SC_weight_department")
   NAME("SC_weight_job")

   NAME("SC_weight_tickets_functional")
   NAME("SC_weight_tickets_share")
   NAME("SC_weight_tickets_deadline")
   NAME("SC_weight_tickets_deadline_active")
   NAME("SC_weight_tickets_override")
NAMEEND

/* *INDENT-ON* */

#define SCS sizeof(SCN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_SCHEDCONFL_H */
