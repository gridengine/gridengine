#ifndef _SGE_RESOURCE_QUOTA_SCHEDD_H
#define __SGE_RESOURCE_QUOTA_SCHEDD_H
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

#include "sgeobj/sge_resource_quota.h"

bool rqs_set_dynamical_limit(lListElem *limit, lListElem *global_host, lListElem *exec_host, lList *centry);

bool rqs_exceeded_sort_out(sge_assignment_t *a, const lListElem *rule, const dstring *rule_name,
   const char* queue_name, const char* host_name);
void rqs_exceeded_sort_out_par(sge_assignment_t *a, const lListElem *rule, const dstring *rule_name,
   const char* queue_name, const char* host_name);

bool sge_user_is_referenced_in_rqs(const lList *rqs, const char *user, const char *group, lList *acl_list);

dispatch_t parallel_rqs_slots_by_time(sge_assignment_t *a, int *slots, int *slots_qend, const char *host, const char *queue);
void parallel_check_and_debit_rqs_slots(sge_assignment_t *a, const char *host, const char *queue, 
      int *slots, int *slots_qend, dstring *rule_name, dstring *rue_name, dstring *limit_name);

dispatch_t rqs_by_slots(sge_assignment_t *a, const char *queue, const char *host, 
  u_long32 *tt_rqs_all, bool *is_global, dstring *rue_string, dstring *limit_name, dstring *rule_name, u_long32 tt_best);

#endif /* __SGE_RESOURCE_QUOTA_SCHEDD_H*/
