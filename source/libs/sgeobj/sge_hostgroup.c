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
#include "sge_hostgroup.h"
#include "sge_hostref.h"

#include "msg_common.h"

#define HOSTREF_LAYER TOP_LAYER

lList *Master_Host_Group_List = NULL;

bool sge_is_hostgrp_reference(const char *string) 
{
   bool ret = false;

   if (string != NULL) {
      ret = (string[0] == '@');
   }
   return ret;
}

lListElem *hostgroup_list_locate(const lList *this_list, const char *group) 
{
   return lGetElemHost(this_list, HGRP_name, group);
}

lListElem *hostgroup_create(lList **answer_list, const char *name, 
                            lList *hostref_or_groupref)
{
   lListElem *ret = NULL;

   DENTER(HOSTREF_LAYER, "hostgroup_create");
   if (name != NULL) {
      ret = lCreateElem(HGRP_Type);
      if (ret != NULL) {
         lSetHost(ret, HGRP_name, name);
         lSetList(ret, HGRP_host_list, hostref_or_groupref);
      } else {
         answer_list_add(answer_list, "no memopy",
                         STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
      }
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
   }
   DEXIT;
   return ret; 
}

bool hostgroup_add_used(lListElem *this_elem, lList **answer_list,
                        const lList *hostref_or_groupref) 
{
   bool ret = true;

   DENTER(HOSTREF_LAYER, "hostgroup_add_used");
   if (this_elem != NULL && hostref_or_groupref != NULL) {
      lList *hostref_list = NULL;
      lListElem *hostref;

      lXchgList(this_elem, HGRP_host_list, &hostref_list);
      for_each(hostref, hostref_or_groupref) {
         const char *name = lGetHost(hostref, HR_name);
   
         ret &= hostref_list_add(&hostref_list, answer_list, name);
         if (!ret) {
            break;
         }
      } 
      lXchgList(this_elem, HGRP_host_list, &hostref_list);
   } else {
      /* EB: move to msg file */
      answer_list_add(answer_list, "invalid parameter",
                      STATUS_ERROR1, ANSWER_QUALITY_ERROR);
      ret = false;
   }
   DEXIT;
   return ret;
}
                        



/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/
/*****************************************************/

static bool 
sge_verify_group_entry(lList** alpp, lList* hostGroupList, 
                       lListElem* hostGroupElem, 
                       const char* extraSubgroupCheck, 
                       int ignoreSupergroupLinks);

/****** sgeobj/hostgroup/sge_verify_host_group_entry() ***************************
*  NAME
*     sge_verify_host_group_entry() -- check hostgroup elements 
*
*  SYNOPSIS
*     bool sge_verify_host_group_entry(lList** alpp, 
*                                      lList* hostGroupList, 
*                                      lListElem* hostGroupElem, 
*                                      char* filename);
*       
*
*  FUNCTION
*     This function is used after creating new entries in a lList* 
*     from GRP_List Type elements which are used for creating 
*     hostgroups. All the member entries are hostnames, so this 
*     function tries to resolve the hostnames. It will check
*     if any subtree has a deadlock (pointer to group which is already 
*     a subgroup of itself).
*  
*  INPUTS
*     lList** alpp              - answer list pointer pointer
*     lList* hostGroupList      - pointer to main hostGroupList (is 
*                                 used for checking subgroups)
*                                 if this pointer is NULL, no group 
*                                 checking is made
*     lListElem* hostGroupElem  - new element to check
*     char* filename            - the name in GRP_group_name must be 
*                                 exactly the same like this parameter
*
*  RESULT
*     bool - true on success or false on failure
******************************************************************************/
bool 
sge_verify_host_group_entry(lList **alpp, lList *hostGroupList,
                            lListElem *hostGroupElem, const char *filename) 
{
   const char* groupName = NULL;
   lList* memberList = NULL;
   lList* subGroupList = NULL;
   const char* hostName = NULL;
   const char* superGroupName = NULL;
   char  resolveHost[500];
   lListElem* ep = NULL;
   int back;

   DENTER(TOP_LAYER, "sge_verify_host_group_entry");
   if (hostGroupElem != NULL) {
      /* get values */
      groupName = lGetString(hostGroupElem, GRP_group_name);
      memberList = lGetList(hostGroupElem, GRP_member_list);
      subGroupList = lGetList(hostGroupElem, GRP_subgroup_list);
      superGroupName = lGetString(hostGroupElem, GRP_supergroup);

      /* check if groupName is the same as filename */
      if ((filename != NULL) && (groupName != NULL)) {
         if (strcmp(groupName, filename)  != 0) {
            /* groupName is different to filename */
            INFO((SGE_EVENT, MSG_ANSWER_HOSTGROUPNAMEXDIFFFROMY_SS, groupName, filename));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return false;
         }
      }

      /* resolve all members (hostnames) */ 
      if (memberList != NULL) {
         for_each( ep , memberList ) {
             hostName = lGetString ( ep , STR );
             if (hostName != NULL) {
                back = getuniquehostname(hostName , resolveHost, 0); 
                if (back == 0 ) {
                    /* guilty hostname */
                    DPRINTF(("hostname %s resolved to %s.\n",hostName ,resolveHost ));
                    lSetString(ep , STR , resolveHost); 
                } else {
                    /* hostname not resolved */
                    INFO((SGE_EVENT, MSG_ANSWER_UNKNOWNHOSTNAME_S, hostName));
                    answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                    DEXIT;
                    return false;
                }
             } 
         }
      }
      
      sge_verify_group_entry(alpp, hostGroupList, hostGroupElem, NULL, false); 

      /* all checks done */
      DEXIT;
      return true; 
   } 
   INFO((SGE_EVENT, MSG_NULLPOINTER ));
   answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR); 
   DEXIT;
   return false;
}

/****** sgeobj/hostgroup/sge_add_group_elem() *********************************
*  NAME
*     sge_add_group_elem() -- create and add new group element 
*
*  SYNOPSIS
*     bool sge_add_group_elem(lList* groupList,
*                             char* groupName
*                             char* subGroupName
*                             char* superGroupName);
*       
*  FUNCTION
*     This function is generating a new lListElem* of type GRP_Type 
*     with given groupName as GRP_groupname. It checks following 
*     conditions:
*
*        - does group allready exist
*        - if subGroupName and superGroupName is not NULL it checks if they
*          are existing and adds the sub group and supergroup entries.
*
*  INPUTS
*     lList* groupList     - pointer to global group list
*     char* groupName      - new group name
*     char* subGroupName   - name of sub group (can be NULL)
*     char* superGroupName - name of super group (can be NULL)
*
*  RESULT
*     bool - true on success, false on error
*******************************************************************************/
bool 
sge_add_group_elem(lList *groupList, const char *groupName,
                   const char *subGroupName, const char *superGroupName) 
{
  lListElem* newGroupElem = NULL;
  int error = 0;

  DENTER(TOP_LAYER,"sge_add_group_elem");

  if ((groupList != NULL) && (groupName != NULL)) {
     if (sge_is_group(groupList, groupName) == false) {

        newGroupElem = lCreateElem(GRP_Type);
        lSetString(newGroupElem, GRP_group_name, groupName);
     
        if (subGroupName != NULL) { 
           if (sge_add_subgroup2group(NULL, groupList, newGroupElem, subGroupName, true ) == false) { 
              error++;
           }
        }

        if (superGroupName != NULL) {
           if (sge_add_supergroup2group( groupList, newGroupElem, superGroupName ) == false) { 
              error++;
           }
        }
 
        if (error != 0) {
           lFreeElem(newGroupElem);
           newGroupElem = NULL;
           DEXIT;
           return false;
        }
        lAppendElem(groupList, newGroupElem);
        DEXIT;
        return true;
     }
  }

  DEXIT;
  return false;
}

static bool sge_verify_group_entry(
lList **alpp,                 /* answer list pointer reference */
lList *hostGroupList,         /* master GRP_Type list  */
lListElem *hostGroupElem,     /* pointer to GRP_Type element */
const char *extraSubgroupCheck,
int ignoreSupergroupLinks 
) {  
   const char* groupName = NULL;
   lListElem* ep = NULL;
   lList* subGroupList = NULL;
   lListElem* tmp_ep = NULL;
   const char* subGroupName = NULL;
   const char* superGroupName = NULL;

   DENTER(TOP_LAYER, "sge_verify_group_entry");


   /* WARNING: do not forget to delete (lFreeList) subGroupList */
   subGroupList = lCopyList("copy of sub group list",lGetList(hostGroupElem, GRP_subgroup_list));
   if (extraSubgroupCheck != NULL) {
      lAddElemStr(&subGroupList, STR,extraSubgroupCheck , ST_Type);
      DPRINTF(("check if group '%s' would match in sub group list\n",extraSubgroupCheck ));
   }

   groupName = lGetString(hostGroupElem, GRP_group_name);
   superGroupName = lGetString(hostGroupElem, GRP_supergroup);

   
 
   /* check group names only if hostGroupList is given */
   if (hostGroupList != NULL) {

      /* check sub group names */
      if (subGroupList != NULL) {
         for_each( ep , subGroupList ) {
            subGroupName = lGetString ( ep , STR );
            if (sge_is_group(hostGroupList, subGroupName) == true) {
               /* sub group is guilty */
               DPRINTF(("sub group %s found.\n", subGroupName ));
               /* check if sub group has supergroup entry for this group */
               tmp_ep = sge_get_group_elem(hostGroupList, subGroupName);
               if (ignoreSupergroupLinks != true) {
                  if (sge_is_group_supergroup(tmp_ep, groupName) == false) {
                      if (sge_add_supergroup2group(hostGroupList,tmp_ep, groupName) == false) { 
                         /* sub group has no supergroup entry for this group */
                         INFO((SGE_EVENT, MSG_ANSWER_SUBGROUPHASNOSUPERGROUP_SS, subGroupName, groupName));
                         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                         lFreeList(subGroupList);
                         subGroupList = NULL;
                         DEXIT;
                         return false;
                      }
                  }
               }
               DPRINTF(("-->Checking if group %s has %s as subgroup\n", subGroupName, groupName));
               /* check for deadlock */
               if (sge_is_group_subgroup(hostGroupList,sge_get_group_elem(hostGroupList,subGroupName) ,groupName,NULL) == true) {
                  /* subtree deadlock */
                  
                  INFO((SGE_EVENT, MSG_ANSWER_SUBGROUPXHASLINKTOGROUPY_SS,  subGroupName, groupName));
                  answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                  lFreeList(subGroupList);
                  subGroupList = NULL;
                  DEXIT;
                  return false; 
               }                
            } else {
               /* sub group is not guilty */
               INFO((SGE_EVENT, MSG_ANSWER_UNKNOWNGROUPNAME_S, subGroupName));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               lFreeList(subGroupList);
               subGroupList = NULL;
               DEXIT;
               return false;
            }
         }
      }
      
      /* check super group names */
      if (superGroupName != NULL) {
         if (sge_is_group(hostGroupList, superGroupName) == true) {
            /* super group is guilty */
            DPRINTF(("super group %s found.\n", superGroupName ));

            /* check if super group has subgroup entry for this group */
            tmp_ep = sge_get_group_elem(hostGroupList, superGroupName);
            if (ignoreSupergroupLinks != true) {
               if (sge_is_group_subgroup(hostGroupList,tmp_ep, groupName,NULL) == false) {
                   if (sge_add_subgroup2group(alpp,hostGroupList,tmp_ep, groupName, true) == false) { 
                      /* super group has no subgroup entry for this group */
                      INFO((SGE_EVENT, MSG_ANSWER_SUPERGROUPHASNOSUBGROUP_SS, superGroupName, groupName));
                      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                      lFreeList(subGroupList);
                      subGroupList = NULL;
                      DEXIT;
                      return false;
                   }
               }
            }                
         } else {
            /* super group is not guilty */
            INFO((SGE_EVENT, MSG_ANSWER_UNKNOWNGROUPNAME_S, superGroupName));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            lFreeList(subGroupList);
            subGroupList = NULL;            
            DEXIT;
            return false;
         }
      }
      lFreeList(subGroupList);
      subGroupList = NULL; 
      DEXIT;
      return true;
   } 

   lFreeList(subGroupList);
   subGroupList = NULL;
   DEXIT;
   return false;
}

/****** sgeobj/hostgroup/sge_add_subgroup2group() *****************************
*  NAME
*     sge_add_subgroup2group() -- add sub group name to GRP_subgroup_list 
*
*  SYNOPSIS
*     bool sge_add_subgroup2group(lList* groupList,
*                                 lListElem* groupElem
*                                 char* subGroupName,
*                                 bool makeChanges);
*
*  FUNCTION
*     This function adds subGroupName to the GRP_subgroup_list in the
*     given groupElem. Following checkes are made:
*
*          - if groupList is NOT NULL: check if sub group exists
*          - if groupList is NOT NULL: make super group entry in 
*            sub group
*          - given groupElem must have group name
*          - group name of groupElem can not be subGroupName
* 
*  INPUTS
*     lList* groupList     - global group list (can be NULL)
*     lListElem* groupElem - pointer to lListElem* (can be NULL)
*     char* subGroupName   - new subgroup entry
*     bool makeChanges     - true means that groupList can be changed, 
*                            false make no changes in groupList 
*                            (supergroup references)
*
*  RESULT
*     int true on success, false on error
*******************************************************************************/
bool 
sge_add_subgroup2group(lList **alpp, lList *groupList, 
                       lListElem *groupElem, const char *subGroupName,
                       bool makeChanges) 
{
  lList* subGroupList = NULL;
  lListElem* subGroupElem = NULL;
  const char*  groupName = NULL;
  DENTER(TOP_LAYER,"sge_add_subgroup2group");

  if ( (groupElem != NULL) && (subGroupName != NULL)) {

     /* do check only if groupList is not NULL */
     if (groupList != NULL) { 
        if (sge_is_group(groupList, subGroupName) == false) {
           INFO((SGE_EVENT, MSG_ANSWER_NOGUILTYSUBGROUPNAME_S, subGroupName));
           answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
           DEXIT;
           return false;
        }
     }

     groupName = lGetString(groupElem, GRP_group_name);
     if (groupName == NULL) {
        INFO((SGE_EVENT, MSG_ANSWER_NOGROUPNAMESPECIFIED));
        answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
        DEXIT;
        return false;
     }
          
     if (strcasecmp(groupName, subGroupName) == 0) {
        INFO((SGE_EVENT, MSG_ANSWER_XCANTBESUBGROUPOFITSELF_S, groupName));
        answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
        DEXIT;
        return false;
     }

     if (groupList != NULL) {
        if (sge_verify_group_entry(alpp, groupList, groupElem, subGroupName , true) == false) {
           DEXIT;
           return false;
        }
     }
    
     /*  add supergroup entry in subgroup (if groupList is not NULL)*/ 
     if (groupList != NULL) {
        subGroupElem = sge_get_group_elem(groupList, subGroupName);
        if (subGroupElem == NULL) {
           DPRINTF(("cant get subgroup elem\n"));
           INFO((SGE_EVENT, MSG_ANSWER_CANTGETSUBGROUPELEMX_S, subGroupName));
           answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
           DEXIT;
           return false;
        }
        if (makeChanges == true) {
          lSetString(subGroupElem, GRP_supergroup, groupName);
        } else {
          DPRINTF(("no supergroup entry is made in subgroup list because makeChanges is false\n"));
        }
     } else {
        DPRINTF(("no supergroup entry is made in subgroup list because groupList is NULL\n"));
     }

     /* group does exist, ok we can insert the name */
     subGroupList = lGetList(groupElem,GRP_subgroup_list);
  
     if (subGroupList == NULL) {
        subGroupList = lCreateList("subgroups", ST_Type );
        lSetList(groupElem, GRP_subgroup_list, subGroupList);
     } 
     if (lGetElemStr(subGroupList, STR, subGroupName) == NULL) {
        lAddElemStr(&subGroupList, STR, subGroupName,ST_Type);
     } else {
        DPRINTF(("group entry allready exits\n"));
     }
    
 
     DEXIT;
     return true;
  }
  DPRINTF(("error in sge_add_subgroup2group()\n"));  
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_del_subgroup_from_group() ************************
*  NAME
*     sge_del_subgroup_from_group() -- delete sub group name  
*
*  SYNOPSIS
*     bool sge_del_subgroup_from_group(lList* groupList,
*                                      lListElem* groupElem
*                                      char* subGroupName);
*
*  FUNCTION
*     This function dels subGroupName from the GRP_subgroup_list in the
*     given groupElem. Following checkes are made:
*
*          - super group entry in subgroup is removed
* 
*  INPUTS
*     lList* groupList     - global group list (can be NULL)
*     lListElem* groupElem - pointer to lListElem* (can be NULL)
*     char* subGroupName   - subgroup entry to remove
*
*  RESULT
*     int true on success, false on error
*******************************************************************************/
bool 
sge_del_subgroup_from_group(lList *groupList, lListElem *groupElem,
                             const char *subGroupName) 
{
  lList*  subgroupList = NULL;
  lListElem* ep = NULL; 

  DENTER(TOP_LAYER,"sge_del_subgroup_from_group");

  if ( (groupElem != NULL) && (subGroupName != NULL)) {
     subgroupList = lGetList(groupElem, GRP_subgroup_list);
     if (subgroupList == NULL) {
        DPRINTF(("cant get subgroup list\n"));
        DEXIT;
        return false;
     } 

     for_each(ep,subgroupList) {
        const char* tmpName = NULL;

        tmpName = lGetString(ep,STR);
        if (tmpName != NULL) {
           if(strcasecmp(tmpName,subGroupName) == 0) {
              /* get subgroup and delete supergroup entry */
              lListElem* subGroupElem = NULL;
              
              subGroupElem = sge_get_group_elem(groupList,subGroupName);
              if (subGroupElem != NULL) {
                lSetString(subGroupElem, GRP_supergroup, NULL);
                DPRINTF(("delete super group entry in subgroup %s\n", subGroupName));
              }

              /* delete tmpName from list */
              lDechainElem(subgroupList,ep);
              lFreeElem(ep);
              DEXIT;
              return true;
           }
        }
     }

  }
  DPRINTF(("error in sge_del_subgroup_from_group()\n"));   
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_add_supergroup2group() ******************************
*  NAME
*     sge_add_supergroup2group() -- set super group name in GRP_supergroup 
*
*  SYNOPSIS
*     bool sge_add_supergroup2group(lList* groupList,
*                                   lListElem* groupElem
*                                   char* superGroupName);
*
*  FUNCTION
*     This function set superGroupName in GRP_supergroup in the
*     given groupElem. Following checkes are made: 
* 
*        - if groupList is NOT NULL: check if super group exists
*        - if groupList is NOT NULL: make sub group entry in super group
*        - given groupElem must have group name
*        - group name of groupElem can not be superGroupName 
*
*  INPUTS
*     lList* groupList     - global group list (can be NULL)
*     lListElem* groupElem - pointer to lListElem* (can be NULL)
*     char* superGroupName   - new supergroup entry
*
*  RESULT
*     int true on success, false on error
*******************************************************************************/
bool 
sge_add_supergroup2group(lList *groupList, lListElem *groupElem, 
                         const char *superGroupName) 
{
  lList* subGroupList = NULL;
  lListElem* superGroupElem = NULL;
  const char*  groupName = NULL;

  DENTER(TOP_LAYER,"sge_add_supergroup2group");

  if ( (groupElem != NULL) && (superGroupName != NULL)) {

     /* do check only if groupList is not NULL */
     if (groupList != NULL) { 
        if (sge_is_group(groupList, superGroupName) == false) {
           DPRINTF(("no guilty subgroup name\n"));
           DEXIT;
           return false;
        }
     }
     groupName = lGetString(groupElem, GRP_group_name);
     if (groupName == NULL) {
        DPRINTF(("no group name\n"));
        DEXIT;
        return false;
     }
          
     if (strcasecmp(groupName, superGroupName) == 0) {
        DPRINTF(("can't be supergroup of me\n"));
        DEXIT;
        return false;
     }
     /*  add subgroup entry in supergroup (if groupList is not NULL) */ 
     if (groupList != NULL) {
        superGroupElem = sge_get_group_elem(groupList, superGroupName);
        if (superGroupElem == NULL) {
           DPRINTF(("can't get supergroup elem\n"));
           DEXIT;
           return false;
        }
       
        subGroupList = lGetList(superGroupElem,GRP_subgroup_list);   
        if (subGroupList == NULL) {
           subGroupList = lCreateList("subgroups", ST_Type );
           lSetList(superGroupElem, GRP_subgroup_list, subGroupList);
        } 
        if (lGetElemStr(subGroupList, STR, groupName) == NULL) {
           lAddElemStr(&subGroupList, STR, groupName,ST_Type);
        } else {
           DPRINTF(("group entry allready exits\n"));
        }
     } else {
        DPRINTF(("no subgroup entry is made in supergroup list because groupList is NULL\n"));
     }
 

     /* group does exist, ok we can insert the name */
     lSetString(groupElem, GRP_supergroup, superGroupName);
     
     DEXIT;
     return true;
  }
  DPRINTF(("error in sge_add_supergroup2group()\n"));  
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_is_group_supergroup() ****************************
*  NAME
*     sge_is_group_supergroup() -- check if group is supergroup  
*
*  SYNOPSIS
*     bool sge_is_group_supergroup(lListElem* groupElem, char* groupName);
*
*  FUNCTION
*     If the super group name of the given groupElem is groupName the 
*     function returns true.
*
*  INPUTS
*     lListElem* groupElem - pointer to lListElem* of type GRP_Type
*     char* groupName      - name of group to compare
*******************************************************************************/
bool sge_is_group_supergroup(lListElem *groupElem, const char *groupName) 
{
  const char*  superGroupName = NULL;
  DENTER(TOP_LAYER,"sge_is_group_in_supergroup");

  if ( (groupElem != NULL) && (groupName != NULL) ) {
     superGroupName = lGetString(groupElem, GRP_supergroup);
     if (superGroupName != NULL) {
        if(strcmp(groupName, superGroupName) == 0 ) {
           DEXIT;
           return true;
        }
     }
  }
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_is_group_subgroup() ******************************
*
*  NAME
*     sge_is_group_subgroup() -- check if group is subgroup  
*
*  SYNOPSIS
*     bool sge_is_group_subgroup(lList *groupList, 
*                                lListElem *groupElem, 
*                                char *groupName, 
*                                lList *rec_list);
*
*  FUNCTION
*     This function checks if groupName is sub group from groupElem. 
*     It will make recursive calls to get the subgroups.
*
*  INPUTS
*     lList*      groupList - pointer to global group list
*     lListElem*  groupElem - element pointer to the group we are 
*                             looking for a sub group
*     char*       groupName - sub group name 
*     lList*      rec_list  - used for recursive calls (must be NULL, 
*                             or can be ST_Type list with group names. 
*                             If a list is given the group names in 
*                             the list are not searched. The caller 
*                             must delete the list. In case of 
*                             rec_list = NULL, the function itself will
*                             create and delete the list. If a list is 
*                             searched the name of the list is appeded 
*                             to rec_list. So no deadlock will happen
*                             in recursive subcalls.)
*
*  RESULT
*     bool - true on success, false on error
*******************************************************************************/
bool 
sge_is_group_subgroup(lList *hostGroupList, lListElem *groupElem,
                      const char *groupName, lList *rec_list) 
{
  lListElem* ep = NULL;
  lList* subGroupList = NULL;
  const char* tmpSubGroupName = NULL;
  int answer = false;
  lList* rec_groupList = NULL;
  bool listCreated = false;
  DENTER(TOP_LAYER,"sge_is_group_in_subgroup");


  /* this is for recursive termination: 
     for all groupnames in the list there was already a search */
  if (rec_list == NULL) {
    rec_groupList = lCreateList("recursive list", ST_Type );
    DPRINTF(("creating allready searched list\n"));
    listCreated = true;  /* just remember to delete the list */
  } else {
    rec_groupList = rec_list; 
  }


  /* main part */
  if ( (groupElem != NULL) && (groupName != NULL) ) {
     subGroupList = lGetList(groupElem, GRP_subgroup_list );
     

     /* this is for recursive termination:
        if groupname was already searched return false */
     if (lGetElemStr( rec_groupList, STR, lGetString(groupElem, GRP_group_name )) != NULL) {
        /* this group is allready searched */
        if(listCreated == true) { 
           DPRINTF(("delete allready searched list\n"));
           lFreeList(rec_groupList);
           rec_groupList = NULL;
           lFreeList(rec_list);
           rec_list = NULL;
        }
        DEXIT;
        return false;
     }


     /* check if groupname is in subgroup list */
     if (lGetElemStr( subGroupList, STR, groupName) != NULL) {
        if(listCreated == true) { 
           DPRINTF(("delete allready searched list\n"));
           lFreeList(rec_groupList);
           rec_groupList = NULL;
           lFreeList(rec_list);
           rec_list = NULL;
        }
        DEXIT;
        return true;
     } 
    
     /* mark group as allready searched for recursive call*/
     lAddElemStr(&rec_groupList, STR,lGetString(groupElem, GRP_group_name ) , ST_Type); 
     DPRINTF(("adding %s to allready searched list\n",lGetString(groupElem, GRP_group_name ) ));
   
     /* recursive search in all subgroubs */     
     if (subGroupList != NULL) {
        for_each(ep,subGroupList) {
           tmpSubGroupName = lGetString(ep, STR); 
           if (tmpSubGroupName != NULL) {
              answer = sge_is_group_subgroup(hostGroupList, 
                                             sge_get_group_elem(hostGroupList,tmpSubGroupName), 
                                             groupName,
                                             rec_groupList);  /* list with groups allready searched for */
              if (answer == true) {
                  DPRINTF(("recursive search[%s]: found '%s' in sub group '%s'\n",lGetString(groupElem,GRP_group_name),
                           groupName,tmpSubGroupName));
                  if(listCreated == true) { 
                     DPRINTF(("delete allready searched list\n"));
                     lFreeList(rec_groupList);
                     rec_groupList = NULL;
                     lFreeList(rec_list);
                     rec_list = NULL;
                  }
                  DEXIT;
                  return true;
              } 
           }
        }
     }
  }
  
  if(listCreated == true) { 
     DPRINTF(("delete allready searched list\n"));
     lFreeList(rec_groupList);
     rec_groupList = NULL;
     lFreeList(rec_list);
     rec_list = NULL;
  }
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_add_member2group() ********************************
*  NAME
*     sge_add_member2group() -- add new member entry to group element 
*
*  SYNOPSIS
*     bool sge_add_member2group(lListElem* groupElem, char* memberName);
*       
*
*  FUNCTION
*     This function will add memberName to the member list of groupElem.
*     If the member allready exits no entry is made.  
*
*  INPUTS
*     lListElem* groupElem - pointer to GRP_Type element
*     char* memberName     - new member name
*
*  RESULT
*     bool -  true on success, false on error
*******************************************************************************/
bool sge_add_member2group(lListElem *groupElem, const char *memberName) 
{
  lList* memberList = NULL;
  DENTER(TOP_LAYER,"sge_add_member2group");

  if ((groupElem != NULL) && (memberName != NULL)) {
 
     memberList = lGetList(groupElem,GRP_member_list);
  
     if (memberList == NULL) {
        memberList = lCreateList("members", ST_Type );
        lSetList(groupElem, GRP_member_list, memberList );
     } 
     if (lGetElemStr(memberList, STR, memberName) == NULL) {
        lAddElemStr(&memberList, STR, memberName,ST_Type);
     } else {
        DPRINTF(("member entry allready exits\n")); 
     }
     DEXIT;
     return true;
  }
  DPRINTF(("error in sge_add_member2group()\n"));  
  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_is_group() ******************************************
*  NAME
*     sge_is_group() -- check any element 
*
*  SYNOPSIS
*     bool sge_is_group(lList* groupList, char* groupName);
*
*  FUNCTION
*     check if any element in grouplist has the given group name
*
*  INPUTS
*     lList* groupList - pointer to global group list
*     char*  groupName - group name to look for
*      
*  RESULT
*     true on success, false on error (group not existing)   
*******************************************************************************/
bool sge_is_group(lList *groupList, const char *groupName) 
{
  lListElem* ep = NULL;
  const char* tmpName = NULL;
  int matches = 0;
  DENTER(TOP_LAYER,"sge_is_group");
 
  if ((groupList != NULL) && (groupName != NULL)) {
     for_each(ep, groupList ) {
        tmpName = lGetString(ep, GRP_group_name);
        if (tmpName != NULL) {
           if (strcasecmp(tmpName, groupName) == 0) {
              matches++;
           }
        }
     }
  }

  if (matches == 1) {
     DEXIT;
     return true;
  } 

  if (matches > 1) {
     DPRINTF(("duplicate group name found - ignoring group name\n"));
  }

  DEXIT;
  return false;
}

/****** sgeobj/hostgroup/sge_get_group_elem() *********************************
*  NAME
*     sge_get_group_elem() -- get lListElem pointer for given groupname 
*
*  SYNOPSIS
*     lListElem* sge_get_group_elem(lList* groupList, char* groupName);
*
*  FUNCTION
*     get element pointer for groupName     
*
*  INPUTS
*     lList* groupList - pointer to global group list
*     char*  groupName - groupname we are looking for
*    
*  RESULT
*     pointer to lListElem or NULL
*******************************************************************************/
lListElem* sge_get_group_elem(lList *groupList, const char *groupName) 
{
  lListElem* ep = NULL;
  lListElem* answer = NULL;
  const char* tmpName = NULL;
  int matches = 0;
  

  DENTER(TOP_LAYER,"sge_get_group_elem");
 
  if ((groupList != NULL) && (groupName != NULL)) {
     for_each(ep, groupList ) {
        tmpName = lGetString(ep, GRP_group_name);
        if (tmpName != NULL) {
           if (strcasecmp(tmpName, groupName) == 0) {
              matches++;
              answer = ep;
           }
        }
     }
  }

  if (matches == 1) {
     DEXIT;
     return answer;
  } 

  if (matches > 1) {
     DPRINTF(("duplicate group name found - ignoring group name\n"));
  }

  DEXIT;
  return NULL;
}

/****** sgeobj/hostgroup/sge_is_member_in_group() ******************************
*  NAME
*     sge_is_member_in_group() -- check if member is in list of group 
*
*  SYNOPSIS
*     bool sge_is_member_in_group(lList* groupList,
*                                 char*  groupName, 
*                                 char*  memberName, 
*                                 lList* rec_list);
*
*  FUNCTION
*     Look if member is in group. This function will also look 
*     recursive in all sub groups. 
*
*  INPUTS
*     lList*     groupList  - pointer to global group list
*     char*      groupName  - name of group where member should be in
*     char*     memberName  - name of member we are looking for
*     lList*      rec_list  - used for recursive calls (must be NULL, 
*                             or can be ST_Type list with group names. 
*                             If a list is given the group names
*                             in the list are not searched. The caller 
*                             must delete the list. In case of 
*                             rec_list = NULL, the function itself will
*                             create and delete the list. If a list is 
*                             searched the name of the list is appeded 
*                             to rec_list. So no deadlock will happen
*                             in recursive subcalls.)
*******************************************************************************/
bool 
sge_is_member_in_group(lList *hostGroupList, const char *groupName,
                       const char *memberName, lList *rec_list) 
{
   lListElem* ep = NULL;
   lListElem* group_ep = NULL;
   lListElem* subgroup_ep = NULL;
   lList*     memberList = NULL;
   lList*     subGroupList = NULL;
   lList*     rec_groupList = NULL;
   const char* tmpName = NULL;
   const char* tmpSubGroupName = NULL;
   bool       answer = false;
   bool       listCreated = false;

   DENTER(TOP_LAYER,"sge_is_member_in_group");

   /* this is for recursive termination: 
     for all groupnames in the list there was already a search */
   if (rec_list == NULL) {
     rec_groupList = lCreateList("recursive list", ST_Type );
     listCreated = true;  /* just remember to delete the list */
     DPRINTF(("creating allready searched list\n"));
   } else {
     rec_groupList = rec_list; 
   }

   /* main part */
   if ((hostGroupList != NULL) && ( groupName != NULL) && (memberName != NULL) ) {

     /* this is for recursive termination:
        if groupname was already searched return false */
     if (lGetElemStr( rec_groupList, STR, groupName) != NULL) {
        /* this group is allready searched */
        if(listCreated == true) { 
           DPRINTF(("delete allrady searched list\n"));
           lFreeList(rec_groupList);
           rec_groupList = NULL;
           lFreeList(rec_list);
           rec_list = NULL;
        }
        DEXIT;
        return false;
     }
   
     /* get element pointer to given groupName */
     group_ep = sge_get_group_elem(hostGroupList, groupName);
     if (group_ep == NULL) {
        if(listCreated == true) { 
           DPRINTF(("delete allrady searched list\n"));
           lFreeList(rec_groupList);
           rec_groupList = NULL;
           lFreeList(rec_list);
           rec_list = NULL;
        }
        DEXIT;
        return false;
     }      
     
     /* search for member in memberlist */
     memberList = lGetList(group_ep, GRP_member_list);
     if (memberList != NULL) {
        for_each(ep, memberList ) {
           tmpName = lGetString(ep, STR);
           if (tmpName != NULL) { 
              if (strcasecmp(tmpName, memberName) == 0) {
                 if(listCreated == true) { 
                    DPRINTF(("delete allrady searched list\n"));
                    lFreeList(rec_groupList);
                    rec_groupList = NULL;
                    lFreeList(rec_list);
                    rec_list = NULL;
                 }
                 DEXIT;
                 return true;
              } 
           }    
        }
     }

     /* mark group as allready searched for recursive call*/
     lAddElemStr(&rec_groupList, STR, groupName, ST_Type); 
     DPRINTF(("adding %s to allready searched list\n",groupName)); 
 
     /* recursive search in all subgroubs */      
     subGroupList = lGetList(group_ep, GRP_subgroup_list);
     if (subGroupList != NULL) {
     
        for_each(subgroup_ep, subGroupList) {
           tmpSubGroupName = lGetString(subgroup_ep, STR); 
           if (tmpSubGroupName != NULL) {
             answer = sge_is_member_in_group(hostGroupList, 
                                             tmpSubGroupName, 
                                             memberName, 
                                             rec_groupList );/* list with groups allready searched for */
             if (answer == true) {
                DPRINTF(("recursive search[%s]: found '%s' in sub group '%s'\n", groupName,
                         memberName,tmpSubGroupName));
                if(listCreated == true) { 
                   DPRINTF(("delete allrady searched list\n"));
                   lFreeList(rec_groupList);
                   rec_groupList = NULL;
                   lFreeList(rec_list);
                   rec_list = NULL;
                }
                DEXIT;
                return true;
             } 
           }
        }
     }
   }
   if(listCreated == true) { 
      DPRINTF(("delete allrady searched list\n"));
      lFreeList(rec_groupList);
      rec_groupList = NULL;
      lFreeList(rec_list);
      rec_list = NULL;
   }
   DEXIT;
   return false;
}
