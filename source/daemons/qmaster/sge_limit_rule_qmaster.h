#ifndef __SGE_LIMIT_RULE_QMASTER_H
#define __SGE_LIMIT_RULE_QMASTER_H
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

#include "sge_c_gdi.h"
#include "uti/sge_monitor.h"

/* funtions called from within gdi framework in qmaster */
int lirs_mod(lList **alpp, lListElem *new_lirs, lListElem *lirs, int add, const char *ruser, 
           const char *rhost, gdi_object_t *object, int sub_command, monitoring_t *monitor);

int lirs_spool(lList **alpp, lListElem *pep, gdi_object_t *object);

int lirs_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object, lList **ppList, monitoring_t *monitor);

/* funtions called via gdi and inside the qmaster */
int sge_del_limit_rule_set(lListElem *ep, lList **alpp, lList **lirs_list, 
                    char *ruser, char *rhost);

bool lirs_diff_usersets(const lListElem *new_lirs, const lListElem *old_lirs, lList **new_acl,
                        lList **old_acl, lList *master_userset_list);

bool lirs_diff_projects(const lListElem *new_lirs, const lListElem *old_lirs, lList **new_acl,
                        lList **old_acl, lList *master_project_list);

bool
scope_is_referenced_lirs(const lListElem *lirs, int filter_type, const char *name);

#endif
