#ifndef __SGE_GROUPS_H__
#define __SGE_GROUPS_H__
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

#include "sge_hgroupL.h"

extern lList *Master_HGroup_List;

void correct_hgroup_name(dstring *string, const char *name);

/* --- */

bool hgroup_correct_name(lListElem *this_elem);

lListElem *
hgroup_create(lList **answer_list, const char *name, 
              lList *hostref_or_groupref);

bool 
hgroup_add_used(lListElem *this_elem, lList **answer_list,
                const lList *hostref_or_groupref);

bool 
hgroup_find_used(const lListElem *this_elem, lList **answer_list,
                 lList *master_list, lList **used_hosts,
                 lList **used_groups);

bool 
hgroup_find_all_used(const lListElem *this_elem, lList **answer_list,
                     lList *master_list, lList **used_hosts,
                     lList **used_groups);

bool 
hgroup_find_occupants(const lListElem *this_elem,
                      lList **answer_list,
                      lList *master_list, lList **occupants_groups);

bool 
hgroup_find_all_occupants(const lListElem *this_elem, 
                          lList **answer_list,
                          lList *master_list, lList **used_groups);

/* --- */

lListElem *
hgroup_list_locate(const lList *this_list, const char *group);

lList **
hgroup_list_get_master_list(void);

bool
hgroup_list_exists(const lList *this_list, lList **answer_list,
                   const lList *href_list);


#endif /* __SGE_GROUPS_H__ */


