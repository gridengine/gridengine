#ifndef __SGE_HOSTL_H
#define __SGE_HOSTL_H

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
 * exec host
 */
enum {
   EH_name = EH_LOWERBOUND,  /* directly user controlled - spooled */
   EH_scaling_list,          /* HS_Type - directly user controlled exec
                              * host only Used to scale host load values.
                              * Contains pairs of load value names and
                              * doubles. spooled */
   EH_consumable_config_list,        /* consumable resources of host
                                      * CE_Type */


   EH_usage_scaling_list,    /* HS_Type - directly user controlled exec
                              * host only SGEEE only scaling of usage
                              * spooled */
   EH_load_list,             /* HL_Type exec host only list of load values 
                              * reported for exechost spooled (not
                              * everytime load comes in) */
   EH_lt_heard_from,
   EH_startup,
   EH_processors,            /* for license purposes exec host only
                              * spooled */
   EH_acl,                   /* US_Type - userset access list */
   EH_xacl,                  /* US_Type - userset access list */
   EH_prj,                   /* UP_Type - project access list */
   EH_xprj,                  /* UP_Type - project excluded access list */

   /* scheduling stuff */
   EH_sort_value,            /* combined load value for sorting only
                              * scheduler local not spooled */
   EH_job_slots_free,        /* used for parallel scheduling only
                              * scheduler local not spooled */
   EH_tagged,                /* used for parallel scheduling only
                              * scheduler local not spooled */
   EH_load_correction_factor,        /* a value of 100 (stands for 1)
                                      * means the load values of this host 
                                      * * has to be increased fully by all
                                      * values from
                                      * conf.load_decay_adjustments only
                                      * scheduler local not spooled */
   EH_seq_no,                /* suitability of this host for a job only
                              * scheduler local not spooled */
   EH_real_name,             /* in case of pseudo host: real name spooled */
   EH_sge_tickets,           /* SGEEE - total tickets associated with active 
                              * jobs executing on this host scheduler
                              * local */
   EH_resource_capability_factor,
   /* SGEEE - resource capability of host scheduler local */
   EH_sge_load,              /* SGEEE load calculated from load values
                              * scheduler local */
   EH_sge_ticket_pct,        /* percentage of total SGEEE tickets scheduler
                              * local */
   EH_resource_capability_factor_pct,
   /* percentage of total resource capability scheduler local */
   EH_sge_load_pct,          /* percentage of total SGEEE load scheduler
                              * local */

   EH_featureset_id,         /* supported feature-set id; not spooled */
   EH_scaled_usage_list,     /* scaled usage for jobs on a host - used by
                              * sge_host_mon */
   EH_scaled_usage_pct_list, /* scaled usage for jobs on a host - used by
                              * sge_host_mon */
   EH_num_running_jobs,      /* number of jobs running on a host - used by 
                              * sge_host_mon */
   EH_load_report_interval,  /* used for caching from global/local
                              * configuration */
   EH_consumable_actual_list,        /* CE_Type actually debited amout of
                                      * consumable resources of host */
   EH_cached_complexes,      /* CE_Type used in scheduler for caching
                              * built attributes */
   EH_cache_version,         /* used to decide whether QU_cached_complexes 
                              * needs a refresh */
   EH_master_host,           /* indicates in scheduler whether a exec host 
                              * is suitable as a master host */
   EH_reschedule_unknown,    /* used for caching from global/local conf;
                              * timout after which jobs will be
                              * rescheduled automatically */
   EH_reschedule_unknown_list,       /* after the rundown of
                                      * EH_reschedule_unknown this list
                                      * contains all jobs which will be
                                      * rescheduled automatically */
   EH_report_seqno           /* sequence number of the last report
                              * (job/load/..) qmaster received from the
                              * execd. This seqno is used to detect old
                              * reports, because reports are send
                              * asynchronous and we have no guarantee that 
                              * they arrive in order at qmaster */
};

ILISTDEF(EH_Type, ExecHost, SGE_EXECHOST_LIST)
   SGE_HOST(EH_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
   SGE_LIST(EH_scaling_list, HS_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_consumable_config_list, CE_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_usage_scaling_list, HS_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_load_list, HL_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_ULONG(EH_lt_heard_from, CULL_DEFAULT)
   SGE_ULONG(EH_startup, CULL_DEFAULT)
   SGE_ULONG(EH_processors, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_acl, US_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_xacl, US_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_prj, UP_Type, CULL_DEFAULT | CULL_SPOOL)
   SGE_LIST(EH_xprj, UP_Type, CULL_DEFAULT | CULL_SPOOL)

   /* scheduling stuff */
   SGE_DOUBLE(EH_sort_value, CULL_DEFAULT)
   SGE_ULONG(EH_job_slots_free, CULL_DEFAULT)
   SGE_ULONG(EH_tagged, CULL_DEFAULT)
   SGE_ULONG(EH_load_correction_factor, CULL_DEFAULT)
   SGE_ULONG(EH_seq_no, CULL_DEFAULT)

   SGE_STRING(EH_real_name, CULL_DEFAULT)

   SGE_DOUBLE(EH_sge_tickets, CULL_DEFAULT)
   SGE_DOUBLE(EH_resource_capability_factor, CULL_DEFAULT | CULL_SPOOL)
   SGE_ULONG(EH_sge_load, CULL_DEFAULT)
   SGE_DOUBLE(EH_sge_ticket_pct, CULL_DEFAULT)
   SGE_DOUBLE(EH_resource_capability_factor_pct, CULL_DEFAULT)
   SGE_DOUBLE(EH_sge_load_pct, CULL_DEFAULT)

   SGE_ULONG(EH_featureset_id, CULL_DEFAULT)

   SGE_LIST(EH_scaled_usage_list, UA_Type, CULL_DEFAULT)
   SGE_LIST(EH_scaled_usage_pct_list, UA_Type, CULL_DEFAULT)
   SGE_ULONG(EH_num_running_jobs, CULL_DEFAULT)

   SGE_ULONG(EH_load_report_interval, CULL_DEFAULT)
   SGE_LIST(EH_consumable_actual_list, CE_Type, CULL_DEFAULT)
   SGE_LIST(EH_cached_complexes, CE_Type, CULL_DEFAULT)
   SGE_ULONG(EH_cache_version, CULL_DEFAULT)
   SGE_ULONG(EH_master_host, CULL_DEFAULT)
   SGE_ULONG(EH_reschedule_unknown, CULL_DEFAULT)
   SGE_LIST(EH_reschedule_unknown_list, RU_Type, CULL_DEFAULT) /* JG: TODO: shall it be spooled? Problem: composed primary key */
   SGE_ULONG(EH_report_seqno, CULL_DEFAULT)
LISTEND 

NAMEDEF(EHN)
   NAME("EH_name")
   NAME("EH_scaling_list")
   NAME("EH_consumable_config_list")
   NAME("EH_usage_scaling_list")
   NAME("EH_load_list")
   NAME("EH_lt_heard_from")
   NAME("EH_startup")
   NAME("EH_processors")
   NAME("EH_acl")
   NAME("EH_xacl")
   NAME("EH_prj")
   NAME("EH_xprj")

   /* scheduling stuff */
   NAME("EH_sort_value")
   NAME("EH_job_slots_free")
   NAME("EH_tagged")
   NAME("EH_load_correction_factor")
   NAME("EH_seq_no")

   NAME("EH_real_name")

   NAME("EH_sge_tickets")
   NAME("EH_resource_capability_factor")
   NAME("EH_sge_load")
   NAME("EH_sge_ticket_pct")
   NAME("EH_resource_capability_factor_pct")
   NAME("EH_sge_load_pct")

   NAME("EH_featureset_id")

   NAME("EH_scaled_usage_list")
   NAME("EH_scaled_usage_pct_list")
   NAME("EH_num_running_jobs")

   NAME("EH_load_report_interval")
   NAME("EH_consumable_actual_list")
   NAME("EH_cached_complexes")
   NAME("EH_cache_version")
   NAME("EH_master_host")
   NAME("EH_reschedule_unknown")
   NAME("EH_reschedule_unknown_list")
   NAME("EH_report_seqno")
NAMEEND

#define EHS sizeof(EHN)/sizeof(char*)

/*
 * reschedule unknown list
 */
enum {
   RU_job_number = RU_LOWERBOUND,
   RU_task_number,
   RU_state
};

LISTDEF(RU_Type)
   SGE_ULONG(RU_job_number, CULL_DEFAULT)
   SGE_ULONG(RU_task_number, CULL_DEFAULT)
   SGE_ULONG(RU_state, CULL_DEFAULT)
LISTEND 

NAMEDEF(RUN)
   NAME("RU_job_number")
   NAME("RU_task_number")
   NAME("RU_state")
NAMEEND

#define RUS sizeof(RUN)/sizeof(char*)

/*
 *  admin host
 */
enum {
   AH_name = AH_LOWERBOUND
};

LISTDEF(AH_Type)
   SGE_HOST(AH_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
LISTEND 

NAMEDEF(AHN)
   NAME("AH_name")
NAMEEND

#define AHS sizeof(AHN)/sizeof(char*)

/*
 * submit host
 */
enum {
   SH_name = SH_LOWERBOUND
};

LISTDEF(SH_Type)
   SGE_HOST(SH_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
LISTEND 

NAMEDEF(SHN)
   NAME("SH_name")
NAMEEND

#define SHS sizeof(SHN)/sizeof(char*)

/* 
 * sge standard load value names
 *
 * use these defined names for refering
 */

/* static load parameters */
#define LOAD_ATTR_ARCH           "arch"
#define LOAD_ATTR_NUM_PROC       "num_proc"

/* raw load parameters */
#define LOAD_ATTR_LOAD_SHORT     "load_short"
#define LOAD_ATTR_LOAD_MEDIUM    "load_medium"
#define LOAD_ATTR_LOAD_LONG      "load_long"
#define LOAD_ATTR_LOAD_AVG       "load_avg"

/* values divided by LOAD_ATTR_NUM_PROC */
#define LOAD_ATTR_NP_LOAD_SHORT  "np_load_short"
#define LOAD_ATTR_NP_LOAD_MEDIUM "np_load_medium"
#define LOAD_ATTR_NP_LOAD_LONG   "np_load_long"
#define LOAD_ATTR_NP_LOAD_AVG    "np_load_avg"
#define LOAD_ATTR_MEM_FREE       "mem_free"
#define LOAD_ATTR_SWAP_FREE      "swap_free"
#define LOAD_ATTR_VIRTUAL_FREE   "virtual_free"
#define LOAD_ATTR_MEM_TOTAL      "mem_total"
#define LOAD_ATTR_SWAP_TOTAL     "swap_total"
#define LOAD_ATTR_VIRTUAL_TOTAL  "virtual_total"
#define LOAD_ATTR_MEM_USED       "mem_used"
#define LOAD_ATTR_SWAP_USED      "swap_used"
#define LOAD_ATTR_VIRTUAL_USED   "virtual_used"
#define LOAD_ATTR_SWAP_RSVD      "swap_rsvd"

/*
 * host load
 */
enum {
   HL_name = HL_LOWERBOUND,
   HL_value,
   HL_last_update
};

SLISTDEF(HL_Type, HostLoad)
   SGE_STRING(HL_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SUBLIST)
   SGE_STRING(HL_value, CULL_DEFAULT | CULL_SUBLIST)
   SGE_ULONG(HL_last_update, CULL_DEFAULT)
LISTEND 

NAMEDEF(HLN)
   NAME("HL_name")
   NAME("HL_value")
   NAME("HL_last_update")
NAMEEND

#define HLS sizeof(HLN)/sizeof(char*)

/*
 * load scaling
 */
enum {
   HS_name = HS_LOWERBOUND,
   HS_value
};

SLISTDEF(HS_Type, LoadScaling)
   SGE_STRING(HS_name, CULL_PRIMARY_KEY | CULL_DEFAULT | CULL_SUBLIST)
   SGE_DOUBLE(HS_value, CULL_DEFAULT | CULL_SUBLIST)
LISTEND 

NAMEDEF(HSN)
   NAME("HS_name")
   NAME("HS_value")
NAMEEND

/* *INDENT-ON* */

#define HSS sizeof(HSN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_HOSTL_H */
