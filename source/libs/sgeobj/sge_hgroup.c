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
/*
  This module is used for group building
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "basis_types.h"
#include "sgermon.h" 
#include "sge_string.h"
#include "sge_stringL.h"
#include "commlib.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_hostname.h"
#include "sge_hgroup.h"
#include "sge_href.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define HOSTREF_LAYER TOP_LAYER

lList *Master_HGroup_List = NULL;

/****** sgeobj/hgroup/correct_hgroup_name() ***********************************
*  NAME
*     correct_hgroup_name() -- convert string to valid hostgroupname 
*
*  SYNOPSIS
*     void correct_hgroup_name(dstring *string, const char *name) 
*
*  FUNCTION
*     Convert string to valid hostgroupname. 
*
*  INPUTS
*     dstring *string  - will be filled with valid hostgroupname 
*     const char *name - valid object name or hostgroupname 
*
*  RESULT
*     void - none 
*******************************************************************************/
void correct_hgroup_name(dstring *string, const char *name) 
{
   if (string != NULL && name != NULL) {
      if (name[0] != HOSTGROUP_INITIAL_CHAR) {
         sge_dstring_sprintf_append(string, "%c", HOSTGROUP_INITIAL_CHAR);
      } 
      sge_dstring_sprintf_append(string, "%s", name);
   } 
}

/****** sgeobj/hgroup/hgroup_list_get_master_list() ***************************
*  NAME
*     hgroup_list_get_master_list() -- Returns master list 
*
*  SYNOPSIS
*     lList **hgroup_list_get_master_list(void) 
*
*  FUNCTION
*     Returns master list containing all existing hostgroup objects. 
*
*  INPUTS
*     void - none 
*
*  RESULT
*     lList** - HGRP_Type list 
*******************************************************************************/
lList **hgroup_list_get_master_list(void) 
{
   return &Master_HGroup_List;
}

/****** sgeobj/hgroup/hgroup_list_locate() ************************************
*  NAME
*     hgroup_list_locate() -- Find a group by name 
*
*  SYNOPSIS
*     lListElem* hgroup_list_locate(const lList *this_list, 
*                                      const char *group) 
*
*  FUNCTION
*     Find a 'group' in 'this_list'. 
*
*  INPUTS
*     const lList *this_list - HGRP_Type list 
*     const char *group      - group name 
*
*  RESULT
*     lListElem* - found element or NULL 
******************************************************************************/
lListElem *hgroup_list_locate(const lList *this_list, const char *group) 
{
   lListElem *ret;
   
   DENTER(HOSTREF_LAYER, "hgroup_list_locate");
   ret = lGetElemHost(this_list, HGRP_name, group);
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_correct_name() ***********************************
*  NAME
*     hgroup_correct_name() -- Correct groupname if necessary 
*
*  SYNOPSIS
*     bool hgroup_correct_name(lListElem *this_elem) 
*
*  FUNCTION
*     Correct groupname if necessary 
*
*  INPUTS
*     lListElem *this_elem - HGRP_Type element 
*
*  RESULT
*     bool - status
*        true  - success
*        false - error 
*******************************************************************************/
bool hgroup_correct_name(lListElem *this_elem) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_correct_name");
   if (this_elem != NULL) {
      const char *name = lGetHost(this_elem, HGRP_name);

      if (name != NULL && name[0] != HOSTGROUP_INITIAL_CHAR) {
         dstring string = DSTRING_INIT;

         correct_hgroup_name(&string, name);   
         lSetHost(this_elem, HGRP_name, sge_dstring_get_string(&string));
         sge_dstring_free(&string);
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_create() *****************************************
*  NAME
*     hgroup_create() -- Create a new hgroup. 
*
*  SYNOPSIS
*     lListElem* 
*     hgroup_create(lList **answer_list, const char *name, 
*                   lList *href_or_groupref) 
*
*  FUNCTION
*     Create a new hostgroup.
*
*  INPUTS
*     lList **answer_list     - AN_Type list 
*     const char *name        - name 
*     lList *href_or_groupref - groupname 
*
*  RESULT
*     lListElem* - new element or NULL 
*******************************************************************************/
lListElem *
hgroup_create(lList **answer_list, const char *name, lList *href_or_groupref)
{
   lListElem *ret = NULL;  /* HGRP_Type */

   DENTER(HOSTREF_LAYER, "hgroup_create");
   if (name != NULL) {
      ret = lCreateElem(HGRP_Type);
      if (ret != NULL) {
         lSetHost(ret, HGRP_name, name);
         lSetList(ret, HGRP_host_list, href_or_groupref);
         hgroup_correct_name(ret);         
      } else {
         sprintf(SGE_EVENT, MSG_MEM_MEMORYALLOCFAILED_S, SGE_FUNC);
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      }
   } else {
      sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC);
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
   }
   DEXIT;
   return ret; 
}

/****** sgeobj/hgroup/hgroup_add_used() ***************************************
*  NAME
*     hgroup_add_used() -- Add a host or group reference 
*
*  SYNOPSIS
*     bool hgroup_add_used(lListElem *this_elem, 
*                             lList **answer_list, 
*                             const lList *href_or_groupref) 
*
*  FUNCTION
*     Add a host or group reference. 
*
*  INPUTS
*     lListElem *this_elem          - HGRP_Type elem
*     lList **answer_list           - AN_Type list 
*     const lList *href_or_groupref - HR_Type list
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error 
******************************************************************************/
bool 
hgroup_add_used(lListElem *this_elem, lList **answer_list, 
                const lList *href_or_groupref) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_add_used");
   if (this_elem != NULL && href_or_groupref != NULL) {
      lList *href_list = NULL;   /* HR_Type */
      lListElem *href;           /* HR_Type */

      lXchgList(this_elem, HGRP_host_list, &href_list);
      for_each(href, href_or_groupref) {
         const char *name = lGetHost(href, HR_name);
   
         ret &= href_list_add(&href_list, answer_list, name);
         if (!ret) {
            break;
         }
      } 
      lXchgList(this_elem, HGRP_host_list, &href_list);
   } else {
      sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC);
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_find_all_used() **********************************
*  NAME
*     hgroup_find_all_used() -- Find referenced host and groups 
*
*  SYNOPSIS
*     bool 
*     hgroup_find_all_used(const lListElem *this_elem, 
*                          lList **answer_list, lList *master_list, 
*                          lList **used_hosts, lList **used_groups) 
*
*  FUNCTION
*     Find directly or indirectly referenced hgroup names. 
*     'master_list' has to be the list of all existing hgroups.
*     'used_hosts' and 'used_groups' will contain the names of
*     hosts and groups referenced by 'this_elem'.
*
*  INPUTS
*     const lListElem *this_elem - HGRP_Type 
*     lList **answer_list        - AN_Type list 
*     lList *master_list         - HGRP_Type list 
*     lList **used_hosts         - HR_Type list 
*     lList **used_groups        - HR_Type list 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
******************************************************************************/
bool 
hgroup_find_all_used(const lListElem *this_elem, lList **answer_list,
                     lList *master_list, lList **used_hosts,
                     lList **used_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_find_all_used");
   if (this_elem != NULL && master_list != NULL) {
      lList *href_list = NULL;   /* HR_Type */
      const char *name;

      name = lGetHost(this_elem, HGRP_name);
      ret &= href_list_add(&href_list, answer_list, name);

      if (ret) {
         ret &= href_list_find_all_used(href_list, answer_list, master_list, 
                                        used_hosts, used_groups);
      }
      href_list = lFreeList(href_list);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_find_used() **************************************
*  NAME
*     hgroup_find_used() -- find directly referenced hosts and groups 
*
*  SYNOPSIS
*     bool 
*     hgroup_find_used(const lListElem *this_elem, lList **answer_list, 
*                      lList *master_list, lList **used_hosts, 
*                      lList **used_groups) 
*
*  FUNCTION
*     Find all hgroups which are directly referenced by 'this_elem'
*     'master_list' has to be the list of all existing hgroups.
*     'used_hosts' and 'used_groups' will contain the names of
*     hosts and groups after a call to this function.
*
*  INPUTS
*     const lListElem *this_elem - HGRP_Type 
*     lList **answer_list        - AN_Type 
*     lList *master_list         - HGRP_Type 
*     lList **used_hosts         - HR_Type 
*     lList **used_groups        - HR_Type 
*
*  RESULT
*     bool - Error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
hgroup_find_used(const lListElem *this_elem, lList **answer_list,
                 lList *master_list, lList **used_hosts,
                 lList **used_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_find_all_used");
   if (this_elem != NULL && master_list != NULL) {
      const char *name = lGetHost(this_elem, HGRP_name);
      lList *href_list = NULL;   /* HR_Type */

      ret &= href_list_add(&href_list, answer_list, name);

      if (ret) {
         ret &= href_list_find_used(href_list, answer_list, master_list, 
                                    used_hosts, used_groups);
      }
      href_list = lFreeList(href_list);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_find_all_occupants() *****************************
*  NAME
*     hgroup_find_all_occupants() -- find groups refering to this group 
*
*  SYNOPSIS
*     bool 
*     hgroup_find_all_occupants(const lListElem *this_elem, 
*                               lList **answer_list, 
*                               lList *master_list, 
*                               lList **occupants_groups) 
*
*  FUNCTION
*     Find all hostgroups from 'master_list' which reference the
*     hostgroup 'this_elem'. The name of these hostgroups will be
*     returned in the hreference list 'occupants_groups'.
*     'answer_list' will contain error messages if the function is
*     not successfull
*      
*
*  INPUTS
*     const lListElem *this_elem - HGRP_Type element 
*     lList **answer_list        - AN_Type list 
*     lList *master_list         - list of all existing HGRP_Type elements 
*     lList **occupants_groups   - HR_Type list 
*
*  RESULT
*     bool - exit state
*        true  - Success
*        false - Error 
*******************************************************************************/
bool hgroup_find_all_occupants(const lListElem *this_elem, 
                               lList **answer_list, lList *master_list, 
                               lList **occupants_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_find_all_occupants");
   if (this_elem != NULL && occupants_groups != NULL) {
      lList *href_list = NULL;
      const char *name;

      name = lGetHost(this_elem, HGRP_name);
      ret &= href_list_add(&href_list, answer_list, name);

      if (ret) {
         ret &= href_list_find_all_occupants(href_list, answer_list,
                                             master_list, occupants_groups);
      }
      href_list = lFreeList(href_list);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hgroup/hgroup_find_occupants() *********************************
*  NAME
*     hgroup_find_occupants() -- Find groups refering to this group 
*
*  SYNOPSIS
*     bool 
*     hgroup_find_occupants(const lListElem *this_elem, 
*                           lList **answer_list, 
*                           lList *master_list, 
*                           lList **occupants_groups) 
*
*  FUNCTION
*     Find all hostgroups from 'master_list' which reference the
*     hostgroup 'this_elem'. The name of these hostgroups will be
*     returned in the hreference list 'occupants_groups'.
*     'answer_list' will contain error messages if the function is
*     not successfull 
*
*  INPUTS
*     const lListElem *this_elem - HGRP_Type 
*     lList **answer_list        - AN_Type 
*     lList *master_list         - HGRP_Type master list 
*     lList **occupants_groups   - HR_Type 
*
*  RESULT
*     bool - Error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
hgroup_find_occupants(const lListElem *this_elem, lList **answer_list,
                      lList *master_list, lList **occupants_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_find_all_occupants");
   if (this_elem != NULL && occupants_groups != NULL) {
      lList *href_list = NULL;
      const char *name;

      name = lGetHost(this_elem, HGRP_name);
      ret &= href_list_add(&href_list, answer_list, name);

      if (ret) {
         ret &= href_list_find_occupants(href_list, answer_list,
                                         master_list, occupants_groups);
      }
      href_list = lFreeList(href_list);
   }
   DEXIT;
   return ret;
}

bool
hgroup_list_exists(const lList *this_list, lList **answer_list,
                   const lList *href_list)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hgroup_list_exists");
   if (this_list != NULL && href_list != NULL) {
      lListElem *href;

      for_each(href, href_list) {
         const char *name = lGetHost(href, HR_name);
         lListElem *hgroup = hgroup_list_locate(this_list, name);
      
         if (hgroup == NULL) {
            ret = false;
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, "host group", name));
            answer_list_add(answer_list, SGE_EVENT,
                            STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            break;      
         }
      }
   }
   DEXIT;
   return ret;
}

