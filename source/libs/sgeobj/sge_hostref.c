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

#include "basis_types.h"
#include "sgermon.h" 
#include "sge_string.h"
#include "sge_stringL.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_hostref.h"
#include "sge_hostgroup.h"

#define HOSTREF_LAYER TOP_LAYER

/****** sgeobj/hostref/hostref_list_add() *************************************
*  NAME
*     hostref_list_add() -- Add host or hostgroup reference.
*
*  SYNOPSIS
*     bool 
*     hostref_list_add(lList **this_list, lList **answer_list, 
*                      const char *host_or_group) 
*
*  FUNCTION
*     Add a host or hostgroup given by 'host_or_group' into the list 
*     'this_list'. If the function is successfull then the function
*     returns 'true' otherwise it will add an entry into 'answer_list'
*     and return with 'false' 
*
*  INPUTS
*     lList **this_list         - HR_Type list 
*     lList **answer_list       - AN_Type list 
*     const char *host_or_group - host or group name  
*
*  RESULT
*     bool - error state
*        true - Success
*        false - Error
*******************************************************************************/
bool hostref_list_add(lList **this_list, lList **answer_list, 
                      const char *host_or_group)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_add");
   if (this_list != NULL && host_or_group != NULL) {
      if (!hostref_list_has_member(*this_list, host_or_group)) {
         lListElem *h_or_g;

         h_or_g = lAddElemHost(this_list, HR_name, host_or_group, HR_Type);
         if (h_or_g == NULL) {
            /* EB: move to msg file */
            answer_list_add(answer_list, "no memopy",
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_has_member() ******************************
*  NAME
*     hostref_list_has_member() -- Is reference already in list 
*
*  SYNOPSIS
*     bool 
*     hostref_list_has_member(const lList *this_list, 
*                             const char *host_or_group) 
*
*  FUNCTION
*     Is the given host or hostgroup ('host_or_group') already
*     contained in the reference list?
*
*  INPUTS
*     const lList *this_list    - HR_Type list 
*     const char *host_or_group - hostname or hostgroup 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_has_member(const lList *this_list, const char *host_or_group)
{
   bool ret = false;

   DENTER(HOSTREF_LAYER, "hostref_list_has_member");
   if (this_list != NULL && host_or_group != NULL) {
      if (hostref_list_locate(this_list, host_or_group) != NULL) {
         ret = true;
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_additional() *************************
*  NAME
*     hostref_list_find_additional() -- difference between two lists 
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_additional(const lList *this_list, 
*                                  lList **answer_list, 
*                                  const lList *list, 
*                                  lList **add_hosts, 
*                                  lList **add_groups) 
*
*  FUNCTION
*     This function will find all hosts and hostgroups which are
*     contained in 'this_list' but not in 'list'. These elements
*     will be copied. Hosts will be added to 'add_hosts' and
*     hostgroups will be added to 'add_groups'. If the function is
*     not successfull then errors will be reported via 'answer_list'. 
*
*  INPUTS
*     const lList *this_list - HR_Type list
*     lList **answer_list    - AN_Type list 
*     const lList *list      - HR_Type list 
*     lList **add_hosts      - HR_Type list 
*     lList **add_groups     - HR_Type list 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_additional(const lList *this_list, lList **answer_list,
                                  const lList *list, lList **add_hosts,
                                  lList **add_groups) {
   bool ret = true;
  
   DENTER(HOSTREF_LAYER, "hostref_list_find_additional"); 
   if (this_list != NULL && list != NULL && 
       add_hosts != NULL && add_groups != NULL) {
      lListElem *this_elem;

      for_each(this_elem, this_list) {
         const char *host_or_group = lGetHost(this_elem, HR_name);

         if (!hostref_list_has_member(list, host_or_group)) {
            if (sge_is_hostgrp_reference(host_or_group)) {
               ret &= hostref_list_add(add_groups, answer_list, host_or_group);
            } else {
               ret &= hostref_list_add(add_hosts, answer_list, host_or_group);
            }
            if (!ret) {
               break;
            }
         }
      }
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_diff() *******************************
*  NAME
*     hostref_list_find_diff() -- difference between two lists 
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_diff(const lList *this_list, lList **answer_list, 
*                            const lList *list, lList **add_hosts, 
*                            lList **rem_hosts, lList **add_groups, 
*                            lList **rem_groups) 
*
*  FUNCTION
*     Will identify differences between 'this_list' and 'list'.
*     hosts which are only in 'this_list' will be copied into 'add_hosts'
*     hosts which are only in 'list' will be copied into 'rem_hosts'
*     groups which are only in 'this_list' will be copied to 'add_groups'
*     groups which are only in 'this' will be copied to 'rem_groups'
*
*  INPUTS
*     const lList *this_list - HR_Type list 
*     lList **answer_list    - AN_Type list 
*     const lList *list      - HR_Type list 
*     lList **add_hosts      - HR_Type list 
*     lList **rem_hosts      - HR_Type list 
*     lList **add_groups     - HR_Type list 
*     lList **rem_groups     - HR_Type list 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_diff(const lList *this_list, lList **answer_list,
                            const lList *list, lList **add_hosts,
                            lList **rem_hosts, lList **add_groups,
                            lList **rem_groups) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_find_diff");
   if (this_list != NULL && list != NULL && add_hosts != NULL &&
       rem_hosts != NULL && add_groups != NULL && rem_groups != NULL) {
      hostref_list_find_additional(this_list, answer_list, list,
                                   add_hosts, add_groups);
      hostref_list_find_additional(list, answer_list, this_list,
                                   rem_hosts, rem_groups);
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_locate() **********************************
*  NAME
*     hostref_list_locate() -- Find an entry in the reference list 
*
*  SYNOPSIS
*     lListElem* 
*     hostref_list_locate(const lList *this_list, const char *name) 
*
*  FUNCTION
*     Find an entry in the reference list. 
*
*  INPUTS
*     const lList *this_list - HR_Type 
*     const char *name       - host or groupname 
*
*  RESULT
*     lListElem* - Pointer to host or hostgroup element or NULL 
*******************************************************************************/
lListElem *hostref_list_locate(const lList *this_list, const char *name) 
{
   lListElem *ret = NULL;

   DENTER(HOSTREF_LAYER, "hostref_list_locate");
   ret = lGetElemHost(this_list, HR_name, name);
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_used() *******************************
*  NAME
*     hostref_list_find_used() -- Find used hosts and hostgroups 
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_used(const lList *this_list, lList **answer_list, 
*                            const lList *master_list, lList **used_hosts, 
*                            lList **used_groups) 
*
*  FUNCTION
*     Finds all hosts and hostgroups which are directy referenced
*     by the hostgroups mentioned in 'this_list'. 'master_list' is
*     the list of all existing hostgroups. Directly referenced hosts
*     and hostgroups will be added to 'used_hosts' and 'used_groups'.
*     In case of any errors 'answer_list' will be filled.
*
*  INPUTS
*     const lList *this_list   - HR_Type 
*     lList **answer_list      - AN_Type 
*     const lList *master_list - GRP_Type
*     lList **used_hosts       - HR_Type 
*     lList **used_groups      - HR_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_used(const lList *this_list, lList **answer_list,
                            const lList *master_list, lList **used_hosts,
                            lList **used_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_find_used");
   if (this_list != NULL && master_list != NULL && 
       used_hosts != NULL && used_groups != NULL) {
      lListElem *hostref;

      for_each(hostref, this_list) {
         const char *name = lGetHost(hostref, HR_name);
         bool is_group = sge_is_hostgrp_reference(name);
         lListElem *hostgroup = NULL;

         if (is_group) {
            hostgroup = hostgroup_list_locate(master_list, name);
         }

         if (hostgroup != NULL) {
            lList *hostref_list2 = lGetList(hostgroup, HGRP_host_list);
            lListElem *hostref2;

            for_each(hostref2, hostref_list2) {
               const char *name2 = lGetHost(hostref2, HR_name);

               if (sge_is_hostgrp_reference(name)) {
                  hostref_list_add(used_groups, answer_list, name2);            
               } else {
                  hostref_list_add(used_hosts, answer_list, name2);            
               }   
            }
         }
         
   
      } 
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_all_used() ***************************
*  NAME
*     hostref_list_find_all_used() -- Find used hosts and hostgroups 
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_all_used(const lList *this_list, 
*                                lList **answer_list, 
*                                const lList *master_list, 
*                                lList **used_hosts, 
*                                lList **used_groups) 
*
*  FUNCTION
*     Finds all hosts and hostgroups which are directly and inderectly
*     referenced by the hostgroups mentioned in 'this_list'. 
*     'master_list' is the list of all existing hostgroups. Referenced 
*     hosts and hostgroups will be added to 'used_hosts' and 'used_groups'.
*     In case of any errors 'answer_list' will be filled.
*
*  INPUTS
*     const lList *this_list   - RN_Type 
*     lList **answer_list      - AN_Type 
*     const lList *master_list - GRP_Type 
*     lList **used_hosts       - RN_Type 
*     lList **used_groups      - RN_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_all_used(const lList *this_list, lList **answer_list,
                                const lList *master_list, lList **used_hosts,
                                lList **used_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_find_all_used");
   if (this_list != NULL && master_list != NULL &&
       used_hosts != NULL && used_groups != NULL) {

      ret &= hostref_list_find_used(this_list, answer_list, master_list,
                                    used_hosts, used_groups);
      if (*used_groups != NULL && ret) {
         lList *used_sub_groups = NULL;
         lList *used_sub_hosts = NULL;

         ret &= hostref_list_find_all_used(*used_groups, answer_list,
                                           master_list, &used_sub_hosts,
                                           &used_sub_groups);

         if (ret) {
            if (used_sub_hosts != NULL) {
               lAddList(*used_hosts, used_sub_hosts);
               used_sub_hosts = NULL; 
            }
            if (used_sub_groups != NULL) {
               lAddList(*used_groups, used_sub_groups);
               used_sub_groups = NULL;
            }
         } 
      }
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_occupants() **************************
*  NAME
*     hostref_list_find_occupants() --  Find occupying hosts and hostgroups
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_occupants(const lList *this_list, 
*                                 lList **answer_list, 
*                                 const lList *master_list, 
*                                 lList **occupant_groups) 
*
*  FUNCTION
*     Finds all hostgroups which directy occupy the hostgroups 
*     mentioned in 'this_list'. 'master_list' is the list of all 
*     existing hostgroups. Directly occupying hostgroups will be 
*     added to 'occupant_groups'. In case of any errors 'answer_list' 
*     will be filled.
*
*  INPUTS
*     const lList *this_list   - HR_Type 
*     lList **answer_list      - AN_Type 
*     const lList *master_list - GRP_Type 
*     lList **occupant_groups  - HR_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_occupants(const lList *this_list, lList **answer_list,
                                 const lList *master_list, 
                                 lList **occupant_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_find_occupants");
   if (this_list != NULL && master_list != NULL && occupant_groups != NULL) {
      lListElem *hostref;

      for_each(hostref, this_list) {
         const char *name = lGetHost(hostref, HR_name);

         if (sge_is_hostgrp_reference(name)) {
            lListElem *hostgroup;

            for_each(hostgroup, master_list) {
               lList *hostref_list = lGetList(hostgroup, HGRP_host_list);
               lListElem *hostref = hostref_list_locate(hostref_list, name);

               if (hostref != NULL) {
                  hostref_list_add(occupant_groups, answer_list,
                                   lGetHost(hostgroup, HGRP_name));
               }
            }
         } 
      }
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/hostref/hostref_list_find_all_occupants() **********************
*  NAME
*     hostref_list_find_all_occupants() -- Find occupying hosts and groups 
*
*  SYNOPSIS
*     bool 
*     hostref_list_find_all_occupants(const lList *this_list, 
*                                     lList **answer_list, 
*                                     const lList *master_list, 
*                                     lList **occupant_groups) 
*
*  FUNCTION
*     Finds all hostgroups which occupy the hostgroups mentioned in 
*     'this_list'. 'master_list' is the list of all
*     existing hostgroups. Directly occupying hostgroups will be
*     added to 'occupant_groups'. In case of any errors 'answer_list'
*     will be filled.
*
*  INPUTS
*     const lList *this_list   - RH_Type 
*     lList **answer_list      - AH_Type 
*     const lList *master_list - GRP_Type 
*     lList **occupant_groups  - RH_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool hostref_list_find_all_occupants(const lList *this_list, 
                                     lList **answer_list,
                                     const lList *master_list, 
                                     lList **occupant_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostref_list_find_all_occupants");
   if (this_list != NULL && master_list != NULL && occupant_groups != NULL) {
      ret &= hostref_list_find_occupants(this_list, answer_list,
                                         master_list, occupant_groups);

      if (*occupant_groups != NULL && ret) {
         lList *occupant_sub_groups = NULL;

         ret &= hostref_list_find_all_occupants(*occupant_groups, answer_list,
                                            master_list, &occupant_sub_groups);

         if (occupant_sub_groups != NULL && ret) {
            lAddList(*occupant_groups, occupant_sub_groups);
            occupant_sub_groups = NULL;
         } 
      }

   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

