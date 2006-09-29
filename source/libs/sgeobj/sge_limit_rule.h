#ifndef __SGE_LIMIT_RULE_H 
#define __SGE_LIMIT_RULE_H 
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

#include "sgeobj/sge_limit_ruleL.h"

bool LIRF_object_parse_from_string(lListElem **filter, const char* buffer, lList **alp);
bool LIRF_object_append_to_dstring(const lListElem *filter, dstring *buffer, lList **alp);

lListElem* limit_rule_set_create(lList **answer_list, const char *name);
lListElem* limit_rule_set_defaults(lListElem* lirs);

bool limit_rule_set_verify_attributes(lListElem *lirs, lList **answer_list, bool in_master);
bool limit_rule_sets_verify_attributes(lList *lirs_list, lList **answer_list, bool in_master);

lListElem *limit_rule_set_list_locate(lList *lp, const char *name);
lListElem *limit_rule_locate(lList *lp, const char *name);

bool lir_xattr_pre_gdi(lList *this_list, lList **answer_list);

bool limit_rule_get_rue_string(dstring *name, const lListElem *rule, const char *user, const char *project, const char *host, const char *queue, const char* pe);

int
lirs_debit_rule_usage(lListElem *job, lListElem *rule, dstring rue_name, int slots, lList *centry_list, const char *obj_name);

int
lirs_debit_consumable(lListElem *lirs, lListElem *job, lListElem *granted, const char *pename, lList *centry_list, lList *acl_list, lList *hgrp_list, int slots);

lListElem *lirs_get_matching_rule(const lListElem *lirs, const char *user, const char *project, const char *pe, const char *host, const char *queue, lList *userset_list, lList* hgroup_list, dstring *rule_name);

bool lirs_is_matching_rule(lListElem *rule, const char *user, const char *project, const char *pe, const char *host, const char *queue, lList *master_userset_list, lList *master_hgroup_list);

bool limit_rule_filter_match(lListElem *filter, int filter_type, const char *value, lList *master_userset_list, lList *master_hgroup_list);

bool sge_user_is_referenced_in_lirs(const lList *lirs, const char *user, lList *acl_list);

#endif /* __SGE_LIMIT_RULE_H */
