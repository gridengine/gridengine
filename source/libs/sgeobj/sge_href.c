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
#include "sge_str.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_hostname.h"
#include "sge_href.h"
#include "sge_hgroup.h"
#include "commlib.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define HOSTREF_LAYER BASIS_LAYER

/****** sgeobj/href/href_list_add() *******************************************
*  NAME
*     href_list_add() -- Add host or hostgroup reference.
*
*  SYNOPSIS
*     bool 
*     href_list_add(lList **this_list, lList **answer_list, 
*                   const char *host_or_group) 
*
*  FUNCTION
*     Add a host or hostgroup given by 'host_or_group' into the list 
*     'this_list'. If the function is successfull then the function
*     returns 'true' otherwise it will add an entry into 'answer_list'
*     and return with 'false'. If 'this_list' does not exist than it
*     will be created.
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
bool 
href_list_add(lList **this_list, lList **answer_list, const char *host_or_group)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_add");
   if (this_list != NULL && host_or_group != NULL) {
      if (!href_list_has_member(*this_list, host_or_group)) {
         lListElem *h_or_g;   /* HR_Type */

         h_or_g = lAddElemHost(this_list, HR_name, host_or_group, HR_Type);
         if (h_or_g == NULL) {
            answer_list_add(answer_list, MSG_GDI_OUTOFMEMORY,
                            STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
            ret = false;
         }
      }
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT, 
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_has_member() ************************************
*  NAME
*     href_list_has_member() -- Is reference already in list 
*
*  SYNOPSIS
*     bool 
*     href_list_has_member(const lList *this_list, 
*                          const char *host_or_group) 
*
*  FUNCTION
*     Is the given host or hostgroup ('host_or_group') already
*     contained in the reference list?
*
*  INPUTS
*     const lList *this_list    - HR_Type list 
*     const char *host_or_group - hostname or hgroup 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool href_list_has_member(const lList *this_list, const char *host_or_group)
{
   bool ret = false;

   DENTER(HOSTREF_LAYER, "href_list_has_member");
   if (this_list != NULL && host_or_group != NULL) {
      if (href_list_locate(this_list, host_or_group) != NULL) {
         ret = true;
      }
   } else {
      /*
       * If one of the argumets was not given by the callee we may be sure
       * that 'host_or_group' is not member of 'this_list'!
       */
      ;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_compare() ***************************************
*  NAME
*     href_list_compare() -- Finds additional entries in list 
*
*  SYNOPSIS
*     bool 
*     href_list_compare(const lList *this_list, lList **answer_list, 
*                       const lList *list, lList **add_hosts, 
*                       lList **add_groups, lList **equity_hosts,
*                       lList **equity_groups) 
*
*  FUNCTION
*     This function will find some differences between two hostref lists
*     given by 'this_list' and 'list'. Hosts and hostgroups which are
*     only in 'this_list' can be found in 'add_hosts' and 'add_groups'.
*     References which are contained in both lists can be found in 
*     'equity_hosts' and 'equity_groups' after a call to this function.
*
*     If the calling function is not interested in one ore more of the
*     result lists than NULL should be used as parameter. The calling
*     function is responsible to free all result lists. 
*
*     If the callee is also interested in the references which are
*     only part of 'list' than this function can not be used.
*     href_list_find_diff() should be used in this case. 
*
*  INPUTS
*     const lList *this_list - HR_Type list to comapre
*     lList **answer_list    - AN_Type list 
*     const lList *list      - 2nd HR_Type list to be compared
*     lList **add_hosts      - HR_Type list 
*     lList **add_groups     - HR_Type list 
*     lList **equity_hosts   - HR_Type list
*     lList **equity_groups  - HR_Type list
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*
*  SEE ALSO
*     sgeobj/href/href_list_find_diff()
*******************************************************************************/
bool href_list_compare(const lList *this_list, lList **answer_list,
                       const lList *list, lList **add_hosts,
                       lList **add_groups, lList **equity_hosts,
                       lList **equity_groups) 
{
   bool ret = true;
   lListElem *this_elem;   /* HR_Type */
  
   DENTER(HOSTREF_LAYER, "href_list_compare"); 

   for_each(this_elem, this_list) {
      const char *host_or_group = lGetHost(this_elem, HR_name);

      if (!href_list_has_member(list, host_or_group)) {
         if (sge_is_hgroup_ref(host_or_group)) {
            if (add_groups != NULL) {
               ret &= href_list_add(add_groups, answer_list, host_or_group);
            }
         } else {
            if (add_hosts != NULL) {
               ret &= href_list_add(add_hosts, answer_list, host_or_group);
            }
         }
      } else {
         if (sge_is_hgroup_ref(host_or_group)) {
            if (equity_groups != NULL) {
               ret &= href_list_add(equity_groups, answer_list, host_or_group);
            }
         } else {
            if (equity_hosts != NULL) {
               ret &= href_list_add(equity_hosts, answer_list, host_or_group);
            }
         }
      }
      if (!ret) {
         break;
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_find_diff() *************************************
*  NAME
*     href_list_find_diff() -- difference between two lists 
*
*  SYNOPSIS
*     bool 
*     href_list_find_diff(const lList *this_list, lList **answer_list, 
*                         const lList *list, lList **add_hosts, 
*                         lList **rem_hosts, lList **add_groups, 
*                         lList **rem_groups) 
*
*  FUNCTION
*     Will identify differences between 'this_list' and 'list'.
*     hosts which are only in 'this_list' will be copied into 'add_hosts'
*     hosts which are only in 'list' will be copied into 'rem_hosts'
*     groups which are only in 'this_list' will be copied to 'add_groups'
*     groups which are only in 'this' will be copied to 'rem_groups'
*
*     The calling context is responsible to free all result lists.
*     If the callee is not interested in one or more of the resultlist
*     than NULL should be used as parameter for this function.
*
*  INPUTS
*     const lList *this_list - HR_Type list to be compared
*     lList **answer_list    - AN_Type list 
*     const lList *list      - 2nd HR_Type list to be compared
*     lList **add_hosts      - HR_Type list 
*     lList **rem_hosts      - HR_Type list 
*     lList **add_groups     - HR_Type list 
*     lList **rem_groups     - HR_Type list 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*
*  SEE ALSO
*     sgeobj/href/href_list_find_diff()
*******************************************************************************/
bool href_list_find_diff(const lList *this_list, lList **answer_list,
                         const lList *list, lList **add_hosts,
                         lList **rem_hosts, lList **add_groups,
                         lList **rem_groups) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_find_diff");
   ret &= href_list_compare(this_list, answer_list, list,
                            add_hosts, add_groups, NULL, NULL);
   ret &= href_list_compare(list, answer_list, this_list,
                               rem_hosts, rem_groups, NULL, NULL);
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_locate() ****************************************
*  NAME
*     href_list_locate() -- Find an entry in the reference list 
*
*  SYNOPSIS
*     lListElem* 
*     href_list_locate(const lList *this_list, const char *name) 
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
lListElem *href_list_locate(const lList *this_list, const char *name) 
{
   lListElem *ret = NULL;  /* HR_Type */

   DENTER(HOSTREF_LAYER, "href_list_locate");
   if (this_list != NULL && name != NULL) {
      ret = lGetElemHost(this_list, HR_name, name);
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_find_references() *******************************
*  NAME
*     href_list_find_references() -- Find referenced hosts and groups 
*
*  SYNOPSIS
*     bool 
*     href_list_find_references(const lList *this_list, 
*                               lList **answer_list, 
*                               const lList *master_list, 
*                               lList **referenced_hosts, 
*                               lList **referenced_groups) 
*
*  FUNCTION
*     Finds hosts and hostgroups which are directy referenced
*     in the hostgroups mentioned in 'this_list'. 'master_list' is
*     the list of all existing hostgroups. Directly referenced hosts
*     and hostgroups will be added to 'used_hosts' and 'used_groups'.
*     In case of any errors 'answer_list' will be filled.
*
*  INPUTS
*     const lList *this_list   - HR_Type 
*     lList **answer_list      - AN_Type 
*     const lList *master_list - HGRP_Type
*     lList **used_hosts       - HR_Type 
*     lList **used_groups      - HR_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
href_list_find_references(const lList *this_list, lList **answer_list,
                          const lList *master_list, lList **used_hosts,
                          lList **used_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_find_references");
   if (this_list != NULL && master_list != NULL) { 
      lListElem *href;  /* HR_Type */

      /*
       * Handle each reference which was given by the calling context
       */
      for_each(href, this_list) {
         const char *name = lGetHost(href, HR_name);
         bool is_group = sge_is_hgroup_ref(name);
         lListElem *hgroup = NULL;  /* HGRP_name */

         /*
          * Try to locate the concerned hgroup object
          */
         if (is_group) {
            hgroup = hgroup_list_locate(master_list, name);
         }

         if (hgroup != NULL) {
            lList *href_list2 = lGetList(hgroup, HGRP_host_list);
            lListElem *href2;    /* HR_Type */

            /* 
             * Add each element contained in the sublist of the hostgroup
             * we found previously to one of the result lists.
             */
            for_each(href2, href_list2) {
               const char *name2 = lGetHost(href2, HR_name);

               if (sge_is_hgroup_ref(name2)) {
                  if (used_groups != NULL) {
                     href_list_add(used_groups, answer_list, name2); 
                  }
               } else {
                  if (used_hosts != NULL) {
                     href_list_add(used_hosts, answer_list, name2); 
                  }
               }   
            }
         }
      } 
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_find_all_references() ***************************
*  NAME
*     href_list_find_all_references() -- Find referenced hosts and hgroups 
*
*  SYNOPSIS
*     bool 
*     href_list_find_all_references(const lList *this_list, 
*                                   lList **answer_list, 
*                                   const lList *master_list, 
*                                   lList **used_hosts, 
*                                   lList **used_groups) 
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
*     const lList *master_list - HGRP_Type 
*     lList **used_hosts       - RN_Type 
*     lList **used_groups      - RN_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
href_list_find_all_references(const lList *this_list, lList **answer_list,
                              const lList *master_list, lList **used_hosts,
                              lList **used_groups)
{
   bool ret = true;

   DENTER(TOP_LAYER, "href_list_find_all_references");
   if (this_list != NULL && master_list != NULL) {
      lList *tmp_used_groups = NULL;
      bool free_tmp_list = false;

      if (used_groups == NULL) {
         used_groups = &tmp_used_groups;
         free_tmp_list = true;
      }

      /*
       * Find all direct referenced hgroups and hosts
       */
      ret &= href_list_find_references(this_list, answer_list, master_list,
                                       used_hosts, used_groups);

      /* 
       * If there are subgroups then try to find their direct referenced
       * groups and hosts, subgroups ...
       *
       * Recursive!
       */
      if (ret && used_groups != NULL && *used_groups != NULL) {
         lList *used_sub_groups = NULL;
         lList *used_sub_hosts = NULL;

         ret &= href_list_find_all_references(*used_groups, answer_list,
                                              master_list, &used_sub_hosts,
                                              &used_sub_groups);
         if (ret) {
            if (used_hosts != NULL && used_sub_hosts != NULL) {
               if (*used_hosts != NULL) {
                  lAddList(*used_hosts, used_sub_hosts);
               } else {
                  *used_hosts = used_sub_hosts;
                  used_sub_hosts = NULL;
               }
            }
            if (used_groups != NULL && used_sub_groups != NULL) {
               if (*used_groups != NULL) {
                  lAddList(*used_groups, used_sub_groups);
               } else {
                  *used_groups = used_sub_groups;
                  used_sub_groups = NULL;
               }
            }
         } 
      }

      if (free_tmp_list) {
         tmp_used_groups = lFreeList(tmp_used_groups);
      }
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_find_referencees() ******************************
*  NAME
*     href_list_find_referencees() --  Find occupying hosts and hgroups
*
*  SYNOPSIS
*     bool 
*     href_list_find_referencees(const lList *this_list, 
*                                lList **answer_list, 
*                                const lList *master_list, 
*                                lList **occupant_groups) 
*
*  FUNCTION
*     Finds hostgroup references which directy occupy at least one of the
*     hostgroups mentioned in 'this_list'. 'master_list' is the list of i
*     all existing hostgroups. Directly occupying hostgroups will be 
*     added to 'occupant_groups'. In case of any errors 'answer_list' 
*     will be filled.
*
*  INPUTS
*     const lList *this_list   - HR_Type 
*     lList **answer_list      - AN_Type 
*     const lList *master_list - HGRP_Type 
*     lList **occupant_groups  - HR_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
href_list_find_referencees(const lList *this_list, lList **answer_list,
                           const lList *master_list, lList **occupant_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_find_referencees");
   if (this_list != NULL && occupant_groups != NULL) {
      lListElem *href;  /* HR_Type */

      for_each(href, this_list) {
         const char *name = lGetHost(href, HR_name);

         if (sge_is_hgroup_ref(name)) {
            lListElem *hgroup;   /* HGRP_Type */

            for_each(hgroup, master_list) {
               lList *href_list = lGetList(hgroup, HGRP_host_list);
               lListElem *href = href_list_locate(href_list, name);

               if (href != NULL) {
                  const char *name = lGetHost(hgroup, HGRP_name);

                  href_list_add(occupant_groups, answer_list, name);
               }
            }
         } 
      }
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_find_all_referencees() **************************
*  NAME
*     href_list_find_all_referencees() -- Find occupying hosts and groups 
*
*  SYNOPSIS
*     bool 
*     href_list_find_all_referencees(const lList *this_list, 
*                                    lList **answer_list, 
*                                    const lList *master_list, 
*                                    lList **occupant_groups) 
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
*     const lList *master_list - HGRP_Type 
*     lList **occupant_groups  - RH_Type 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool 
href_list_find_all_referencees(const lList *this_list, lList **answer_list,
                               const lList *master_list, 
                               lList **occupant_groups)
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_find_all_referencees");
   if (this_list != NULL && occupant_groups != NULL) {

      /*
       * Find parents of all given hgroups
       */
      ret &= href_list_find_referencees(this_list, answer_list,
                                        master_list, occupant_groups);

      if (*occupant_groups != NULL && ret) {
         lList *occupant_sub_groups = NULL;  /* HR_Type */

         /*
          * Find grandparents, ...
          *
          * Recursive!
          */
         ret &= href_list_find_all_referencees(*occupant_groups, answer_list,
                                               master_list, 
                                               &occupant_sub_groups);

         if (occupant_sub_groups != NULL && ret) {
            lAddList(*occupant_groups, occupant_sub_groups);
            occupant_sub_groups = NULL;
         } 
      }
   } else {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_INAVLID_PARAMETER_IN_S, SGE_FUNC));
      answer_list_add(answer_list, SGE_EVENT,
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_append_to_string() ******************************
*  NAME
*     href_list_append_to_string() -- append hrefs to dstring 
*
*  SYNOPSIS
*     bool 
*     href_list_append_to_string(const lList *this_list, 
*                                dstring *string) 
*
*  FUNCTION
*     Append all host and hostgroup references contained in 'this_list'
*     to 'string'. One space character separated ech entry in 'string'. 
*
*  INPUTS
*     const lList *this_list - RN_Type list 
*     dstring *string        - dynamic string 
*
*  RESULT
*     bool - error state 
*        true  - Success
*        false - Error
*******************************************************************************/
bool href_list_append_to_string(const lList *this_list, dstring *string)
{
   bool ret = true;
   
   DENTER(HOSTREF_LAYER, "href_list_print_to_string");
   if (this_list != NULL || string != NULL) {
      lListElem *href;

      for_each(href, this_list) {
         const char *name = lGetHost(href, HR_name);

         if (name != NULL) {
            sge_dstring_sprintf_append(string, "%s ", name);
         }
      }
   } 
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_resolve_hostnames() *****************************
*  NAME
*     href_list_resolve_hostnames() -- resolve hostnames 
*
*  SYNOPSIS
*     bool 
*     href_list_resolve_hostnames(lList *this_list, lList **answer_list) 
*
*  FUNCTION
*     Resolve hostnames contained in 'this_list'. 
*
*  INPUTS
*     lList *this_list    - HR_Type list 
*     lList **answer_list - AN_Type list 
*
*  RESULT
*     bool - error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool href_list_resolve_hostnames(lList *this_list, lList **answer_list) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_resolve_hostnames");
   if (this_list != NULL) {
      lListElem *href = NULL;

      for_each(href, this_list) {
         const char *name = lGetHost(href, HR_name);

         if (!sge_is_hgroup_ref(name)) {
            char resolved_name[500];

            int back = getuniquehostname(name, resolved_name, 0);

            if (back == 0) {
               lSetHost(href, HR_name, resolved_name);
            } else {
               INFO((SGE_EVENT, MSG_HGRP_UNKNOWNHOST, name));
               answer_list_add(answer_list, SGE_EVENT, 
                               STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               ret = false;
            }
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/href/href_list_append_to_dstring() *****************************
*  NAME
*     href_list_append_to_dstring() -- Print href-list to dstring 
*
*  SYNOPSIS
*     bool 
*     href_list_append_to_dstring(const lList *this_list, 
*                                 dstring *string) 
*
*  FUNCTION
*     Print href-list to dstring 
*
*  INPUTS
*     const lList *this_list - HR_Type  
*     dstring *string        - dynamic string 
*
*  RESULT
*     bool - Error state
*        true  - Success
*        false - Error
*******************************************************************************/
bool href_list_append_to_dstring(const lList *this_list, dstring *string)
{
   const char *const delim = " ";
   bool ret = true;

   DENTER(HOSTREF_LAYER, "href_list_append_to_dstring");
   if (this_list != NULL && string != NULL) {
      lListElem *href;  /* HR_Type */
      bool is_first = true;

      for_each(href, this_list) {
         const char *name = lGetHost(href, HR_name);

         if (!is_first) {
            sge_dstring_sprintf_append(string, "%s", delim);
         }
         sge_dstring_sprintf_append(string, "%s", name);
         is_first = false; 
      }
   } else {
      ret = false;
   } 
   DEXIT;
   return ret;
}

