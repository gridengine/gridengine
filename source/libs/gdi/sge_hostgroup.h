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

#include "sge_hostgroupL.h"

#ifndef __SGE_NO_USERMAPPING__
extern lList *Master_Host_Group_List;
#endif

/* get functions */
lListElem* sge_get_group_elem(lList *groupList, const char *groupName);

/* check functions */
int        sge_is_group(lList *groupList, const char *groupName);
int        sge_is_member_in_group(lList *groupList, const char *groupName, const char *memberName, lList *rec_list);
int        sge_is_group_supergroup(lListElem *groupElem, const char *groupName);
int        sge_is_group_subgroup(lList *groupList, lListElem *groupElem, const char *groupName, lList *rec_list);

/* add functions */
int        sge_add_group_elem(lList *groupList, const char *groupName, const char *subGroupName, const char *superGroupName);
int        sge_add_member2group(lListElem *groupElem, const char *memberName);   
int        sge_add_subgroup2group(lList **alpp, lList *groupList, lListElem *groupElem, const char *subGroupName, int makeChanges);
int        sge_add_supergroup2group(lList *groupList, lListElem *groupElem, const char *superGroupName);

/* del functions */
int        sge_del_subgroup_from_group(lList *groupList, lListElem *groupElem, const char *subGroupName);

/* verify functions */
int        sge_verify_host_group_entry(lList **alpp, lList *hostGroupList, lListElem *hostGroupElem, const char *filename);


#endif /* __SGE_GROUPS_H__ */


