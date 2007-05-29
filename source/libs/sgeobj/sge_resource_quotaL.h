#ifndef __SGE_RESOURCE_QUOTAL_H
#define __SGE_RESOURCE_QUOTAL_H

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

/* Resource Quota Set */
enum {
   RQS_name = RQS_LOWERBOUND,
   RQS_description,
   RQS_enabled,
   RQS_rule
};

LISTDEF(RQS_Type)
 JGDI_ROOT_OBJ(ResourceQuotaSet, SGE_RQS_LIST, ADD | MODIFY | DELETE | GET | GET_LIST)
 JGDI_EVENT_OBJ(ADD(sgeE_RQS_ADD) | MODIFY(sgeE_RQS_MOD) | DELETE(sgeE_RQS_DEL) | GET_LIST(sgeE_RQS_LIST))
 SGE_STRING(RQS_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_CONFIGURE)
 SGE_STRING(RQS_description, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_BOOL(RQS_enabled, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_LIST(RQS_rule, RQR_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
LISTEND

NAMEDEF(RQSN)
   NAME("RQS_name")
   NAME("RQS_description")
   NAME("RQS_enabled")
   NAME("RQS_rule")
NAMEEND

#define RQSS sizeof(RQSN)/sizeof(char*)

/* Resource Quota Rule */
enum {
   RQR_name = RQR_LOWERBOUND,
   RQR_filter_users,
   RQR_filter_projects,
   RQR_filter_pes,
   RQR_filter_queues,
   RQR_filter_hosts,
   RQR_limit,
   RQR_level
};

LISTDEF(RQR_Type)
 JGDI_OBJ(ResourceQuotaRule)
 SGE_STRING(RQR_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_CONFIGURE)
 SGE_OBJECT(RQR_filter_users, RQRF_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_OBJECT(RQR_filter_projects, RQRF_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_OBJECT(RQR_filter_pes, RQRF_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_OBJECT(RQR_filter_queues, RQRF_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_OBJECT(RQR_filter_hosts, RQRF_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_LIST(RQR_limit, RQRL_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_ULONG(RQR_level, CULL_DEFAULT | CULL_JGDI_RO) /* maintained by qmaster needed only in parallel 
                                                      scheduling for accumulating slots after queues were tagged */
LISTEND

NAMEDEF(RQRN)
   NAME("RQR_name")
   NAME("RQR_filter_users")
   NAME("RQR_filter_projects")
   NAME("RQR_filter_pes")
   NAME("RQR_filter_queues")
   NAME("RQR_filter_hosts")
   NAME("RQR_limit")
   NAME("RQR_level")
NAMEEND

#define RQRS sizeof(RQRN)/sizeof(char*)

enum {
   FILTER_USERS = 0,
   FILTER_PROJECTS,
   FILTER_PES,
   FILTER_QUEUES,
   FILTER_HOSTS
};

/* values found in RQR_level */
enum {
   RQR_ALL = 0,
   RQR_GLOBAL,
   RQR_CQUEUE,
   RQR_HOST,
   RQR_QUEUEI
};

/* Resource Quota Rule Filter */
enum {
   RQRF_expand = RQRF_LOWERBOUND,
   RQRF_scope,
   RQRF_xscope
};

LISTDEF(RQRF_Type)
 JGDI_OBJ(ResourceQuotaRuleFilter)
 SGE_BOOL(RQRF_expand, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_LIST(RQRF_scope, ST_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_LIST(RQRF_xscope, ST_Type, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
LISTEND

NAMEDEF(RQRFN)
   NAME("RQRF_expand")
   NAME("RQRF_scope")
   NAME("RQRF_xscope")
NAMEEND

#define RQRFS sizeof(RQRFN)/sizeof(char*)

/* Resource Quota Rule Limit */
enum {
   RQRL_name = RQRL_LOWERBOUND,
   RQRL_value,
   RQRL_type,
   RQRL_dvalue,
   RQRL_usage,
   RQRL_dynamic
};

LISTDEF(RQRL_Type)
 JGDI_OBJ(ResourceQuotaRuleLimit)
 SGE_STRING(RQRL_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL | CULL_CONFIGURE)
 SGE_STRING(RQRL_value, CULL_DEFAULT | CULL_SPOOL | CULL_CONFIGURE)
 SGE_ULONG(RQRL_type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)
 SGE_DOUBLE(RQRL_dvalue, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)
 SGE_LIST(RQRL_usage, RUE_Type, CULL_DEFAULT | CULL_JGDI_RO)
 SGE_BOOL(RQRL_dynamic, CULL_DEFAULT | CULL_JGDI_RO)
LISTEND
 
NAMEDEF(RQRLN)
   NAME("RQRL_name")
   NAME("RQRL_value")
   NAME("RQRL_type")
   NAME("RQRL_dvalue")
   NAME("RQRL_usage")
   NAME("RQRL_dynamic")
NAMEEND

#define RQRLS sizeof(RQRLN)/sizeof(char*)


/* Resource Quota Limit - used only for caching scheduler-internally */
enum {
   RQL_name = RQL_LOWERBOUND,
   RQL_result,
   RQL_time
};

LISTDEF(RQL_Type)
 SGE_STRING(RQL_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE )
 SGE_INT(RQL_result, CULL_DEFAULT )
 SGE_ULONG(RQL_time, CULL_DEFAULT )
LISTEND
 
NAMEDEF(RQLN)
   NAME("RQL_name")
   NAME("RQL_result")
   NAME("RQL_time")
NAMEEND

#define RQLS sizeof(RQLN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif
#endif /* __SGE_RESOURCE_QUOTAL_H */
