#ifndef __SGE_LIMIT_RULEL_H
#define __SGE_LIMIT_RULEL_H

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

/* Rule Set Object */
enum {
   LIRS_name = LIRS_LOWERBOUND,
   LIRS_description,
   LIRS_enabled,
   LIRS_rule
};

LISTDEF(LIRS_Type)
 JGDI_ROOT_OBJ(LimitationRuleSet, SGE_LIRS_LIST, ADD | MODIFY | DELETE | GET | GET_LIST)
 SGE_STRING(LIRS_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
 SGE_STRING(LIRS_description, CULL_DEFAULT | CULL_SPOOL)
 SGE_BOOL(LIRS_enabled, CULL_DEFAULT | CULL_SPOOL)
 SGE_LIST(LIRS_rule, LIR_Type, CULL_DEFAULT | CULL_SPOOL)
LISTEND

NAMEDEF(LIRSN)
   NAME("LIRS_name")
   NAME("LIRS_description")
   NAME("LIRS_enabled")
   NAME("LIRS_rule")
NAMEEND

#define LIRSS sizeof(LIRSN)/sizeof(char*)

/* Rule Object */
enum {
   LIR_name = LIR_LOWERBOUND,
   LIR_filter_users,
   LIR_filter_projects,
   LIR_filter_pes,
   LIR_filter_queues,
   LIR_filter_hosts,
   LIR_limit,
   LIR_level
};

LISTDEF(LIR_Type)
 JGDI_OBJ(LimitationRule)
 SGE_STRING(LIR_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
 SGE_OBJECT(LIR_filter_users, LIRF_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_OBJECT(LIR_filter_projects, LIRF_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_OBJECT(LIR_filter_pes, LIRF_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_OBJECT(LIR_filter_queues, LIRF_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_OBJECT(LIR_filter_hosts, LIRF_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_LIST(LIR_limit, LIRL_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_ULONG(LIR_level, CULL_DEFAULT | CULL_JGDI_RO)
LISTEND

NAMEDEF(LIRN)
   NAME("LIR_name")
   NAME("LIR_filter_users")
   NAME("LIR_filter_projects")
   NAME("LIR_filter_pes")
   NAME("LIR_filter_queues")
   NAME("LIR_filter_hosts")
   NAME("LIR_limit")
   NAME("LIR_level")
NAMEEND

#define LIRS sizeof(LIRN)/sizeof(char*)

enum {
   FILTER_USERS = 0,
   FILTER_PROJECTS,
   FILTER_PES,
   FILTER_QUEUES,
   FILTER_HOSTS
};

enum {
   LIR_ALL = 0,
   LIR_GLOBAL,
   LIR_CQUEUE,
   LIR_HOST,
   LIR_QUEUEI
};

/* Rule Filter Object */
enum {
   LIRF_expand = LIRF_LOWERBOUND,
   LIRF_scope,
   LIRF_xscope
};

LISTDEF(LIRF_Type)
 JGDI_OBJ(LimitationRuleFilter)
 SGE_BOOL(LIRF_expand, CULL_DEFAULT | CULL_SPOOL)
 SGE_LIST(LIRF_scope, ST_Type, CULL_DEFAULT | CULL_SPOOL)
 SGE_LIST(LIRF_xscope, ST_Type, CULL_DEFAULT | CULL_SPOOL)
LISTEND

NAMEDEF(LIRFN)
   NAME("LIRF_expand")
   NAME("LIRF_scope")
   NAME("LIRF_xscope")
NAMEEND

#define LIRFS sizeof(LIRFN)/sizeof(char*)

/* Rule Limit Object */
enum {
   LIRL_name = LIRL_LOWERBOUND,
   LIRL_value,
   LIRL_type,
   LIRL_dvalue,
   LIRL_usage,
   LIRL_dynamic
};

LISTDEF(LIRL_Type)
 JGDI_OBJ(LimitationRuleLimit)
 SGE_STRING(LIRL_name, CULL_PRIMARY_KEY | CULL_HASH | CULL_UNIQUE | CULL_SPOOL)
 SGE_STRING(LIRL_value, CULL_DEFAULT | CULL_SPOOL)
 SGE_ULONG(LIRL_type, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)
 SGE_DOUBLE(LIRL_dvalue, CULL_DEFAULT | CULL_SPOOL | CULL_JGDI_RO)
 SGE_LIST(LIRL_usage, RUE_Type, CULL_DEFAULT | CULL_JGDI_RO)
 SGE_BOOL(LIRL_dynamic, CULL_DEFAULT | CULL_JGDI_RO)
LISTEND
 
NAMEDEF(LIRLN)
   NAME("LIRL_name")
   NAME("LIRL_value")
   NAME("LIRL_type")
   NAME("LIRL_dvalue")
   NAME("LIRL_usage")
   NAME("LIRL_dynamic")
NAMEEND

#define LIRLS sizeof(LIRLN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif
#endif /* __SGE_LIMIT_RULEL_H */
