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
   This is the module for handling hostgroup.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_conf.h"
#include "read_write_host_group.h"
#include "sge_log.h"
#include "sge_c_gdi.h"
#include "sge_stringL.h"
#include "sge_string.h"
#include "gdi_utility_qmaster.h"
#include "sge_usermap.h"
#include "sge_hostgroup_qmaster.h"
#include "sge_user_mapping.h"
#include "sge_answer.h"
#include "sge_unistd.h"
#include "sge_hostgroup.h"

#ifndef __SGE_NO_USERMAPPING__

#include "msg_common.h"
#include "msg_qmaster.h"

/****** src/hostgrp_mod() **********************************
*
*  NAME
*     hostgrp_mod() -- handle gdi SGE_HOST_GROUP_LIST MOD/ADD request 
*
*  SYNOPSIS
*
*     #include "sge_hostgroup_qmaster.h"
*     #include <src/sge_hostgroup_qmaster.h>
* 
*     int hostgrp_mod(lList**       alpp, 
*                           lListElem*    modp, 
*                           lListElem*    ep, 
*                           int           add, 
*                           char*         ruser,
*                           char*         rhost, 
*                           gdi_object_t* object);
*       
*
*  FUNCTION
*     This function is called when the sge_c_gdi.c module gets an 
*     SGE_HOST_GROUP_LIST MOD/ADD request. The answer list is for
*     status report. The parameter modp is the current element of
*     the qmaster. This element should get all information from
*     parameter ep. The parameter add specifies if the MOD or ADD
*     request was expected. On ADD the parameter modp has no entries.
*     The parameter ruser and rhost specifies from where the gdi 
*     request was sent. And the object parameter is a pointer to
*     the qmasters gdi_object definitions.
*     
*
*  INPUTS
*     lList **alpp          - answer list pointer reference
*     lListElem *modp       - empty element to fill up (GRP_Type) (wenn add is 1)
*     lListElem *ep         - element to add (GRP_Type) 
*     int add               - is 1 on ADD mode, 0 on MOD mode 
*     char *ruser           - user name who starts request 
*     char *rhost           - host from where the request was started 
*     gdi_object_t *objecti - pointer to gdi_object struct
*
*  RESULT
*     int 0 on success, != 0 on error
*     
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/hostgrp_success()
*     src/hostgrp_spool()
*     src/sge_del_hostgroup()
*     
****************************************************************************
*/
int hostgrp_mod(
lList **alpp,
lListElem *modp, /* empty element to fill up (GRP_Type) (wenn add is 1)*/ 
lListElem *ep,   /* element to add (GRP_Type) */
int add,         /* is 1 on ADD mode, 0 on MOD mode */
char *ruser,     /* user name who starts request */
char *rhost,     /* host from where the request was started */
gdi_object_t *object,
int sub_command 
) {
   const char* groupName = NULL;
   lList* memberList = NULL;
   lList* newMemberList = NULL;
   lList* subgroupList = NULL;
   lList* newSubgroupList = NULL;
   lListElem* tmp_ep = NULL;

   DENTER(TOP_LAYER, "hostgrp_mod");
   
/*   GRP_Type list element
*   |
*   *---GRP_group_name      (SGE_STRING)
*   |
*   *---GRP_member_list       (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_subgroup_list   (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_supergroup (SGE_STRING)
*/ 
   groupName = lGetString(ep, GRP_group_name);

   if (add == 1) {
      /* GRP_group_name -------------------------------*/

      if (sge_is_valid_filename(groupName) != 0) {
         /* no correct filename */
         ERROR((SGE_EVENT,MSG_HGRP_GROUPXNOTGUILTY_S, groupName ));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }  
      lSetString(modp, GRP_group_name , groupName );

      /* GRP_member_list ---------------------------------*/
      memberList = lGetList(modp, GRP_member_list);
      if (memberList != NULL) {
         ERROR((SGE_EVENT, MSG_HGRP_MEMBERLISTFORXEXISTS_S, groupName ));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      memberList = NULL; 
      memberList = lCreateList("member list", ST_Type);
      lSetList(modp,GRP_member_list, memberList); 

      /* GRP_subgroup_list --------------------------------------*/
      subgroupList = lCreateList("subgroup list", ST_Type);
      lSetList(modp, GRP_subgroup_list, subgroupList);
   } /* if (add == 1) */ 

   /* a guilty GRP_group_list exists */         

  
   if (sge_verify_host_group_entry(alpp, NULL ,ep, groupName) != TRUE) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   
   /* now copy the entries from ep to modp */ 


   /* GRP_group_name */
   lSetString(modp, GRP_group_name, groupName);

   /* GRP_supergroup */
   lSetString(modp,GRP_supergroup, lGetString(ep, GRP_supergroup)); 

   /* GRP_member_list */
   newMemberList = lGetList(ep, GRP_member_list);
   if (newMemberList != NULL) {
      for_each(tmp_ep, newMemberList ) {
         const char* tmpMember = NULL;
         tmpMember = lGetString(tmp_ep, STR);
         if (tmpMember != NULL) {
            if (sge_add_member2group(modp, tmpMember) != TRUE) {
               ERROR((SGE_EVENT, MSG_HGRP_CANTADDMEMBERXTOGROUPY_SS, tmpMember ,groupName ));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
         }
      } 
   }

   memberList = lGetList(modp, GRP_member_list);
   if (memberList != NULL) {
      tmp_ep = memberList->first;
      while (tmp_ep != NULL) {
         const char* actMember = NULL;
         actMember =  lGetString(tmp_ep,STR);
         if (actMember != NULL) { 
            if (lGetElemStr( newMemberList , STR , actMember ) == NULL) {
               /* delete the element */
               DPRINTF(("removing entry '%s' from member_list\n", actMember));
               lDechainElem(memberList, tmp_ep);
               lFreeElem(tmp_ep);
               tmp_ep = NULL;
               tmp_ep = memberList->first;
            } else {
               tmp_ep = tmp_ep->next;
            }
         } else {
           tmp_ep = tmp_ep->next;
         }
      }
   }

   /* GRP_subgroup_list */
   newSubgroupList = lGetList(ep, GRP_subgroup_list);
   for_each(tmp_ep, newSubgroupList ) {
      const char* tmpGroup = NULL;
      tmpGroup = lGetString(tmp_ep, STR);
      if (tmpGroup != NULL) {
         /* do not make changes in Master_Host_Group_List, so we set param. 4 to FALSE */
         if (sge_add_subgroup2group(alpp, Master_Host_Group_List , modp, tmpGroup, FALSE) != TRUE) {
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
   }

   /* There was no error, we can make sublinks in Master_Host_Group_List
      so we do it again */

   for_each(tmp_ep, newSubgroupList ) {
      const char* tmpGroup = NULL;
      tmpGroup = lGetString(tmp_ep, STR);
      if (tmpGroup != NULL) {
         /* yes, now make changes in Master_Host_Group_List, so we set param. 4 to TRUE */
         sge_add_subgroup2group(alpp, Master_Host_Group_List , modp, tmpGroup, TRUE);
      }
   }
   subgroupList = lGetList(modp, GRP_subgroup_list);
   if (subgroupList != NULL) {
      tmp_ep = subgroupList->first;
      while (tmp_ep != NULL) {
         const char* actSubgroup = NULL;
         actSubgroup =  lGetString(tmp_ep,STR);
   
         if (lGetElemStr( newSubgroupList , STR , actSubgroup ) == NULL) {
            /* delete the element */
            const char* tmpSubgroup = NULL;
            tmpSubgroup = lGetString(tmp_ep, STR);
            if (tmpSubgroup != NULL) {
               DPRINTF(("removing entry '%s' from subgroup_list\n", tmpSubgroup));
   
               sge_del_subgroup_from_group(Master_Host_Group_List, modp, tmpSubgroup );
            
               tmp_ep = NULL;
               tmp_ep = subgroupList->first;
            } else {
               tmp_ep = tmp_ep->next;
            }
         } else {
           tmp_ep = tmp_ep->next;
         }
      }
   }

   if (sge_verify_host_group_entry(alpp, Master_Host_Group_List ,modp, groupName) != TRUE) {
      DPRINTF(("\n\n\n******** very very bad failure!!!\n\n\n"));
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
/*  *---GRP_group_name      (SGE_STRING)
*   |
*   *---GRP_member_list       (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_subgroup_list   (SGE_LIST)    
*   |          |
*   |          |
*   |          *----STR  (SGE_STRING)  String list (ST_Type)
*   |
*   |
*   *---GRP_supergroup (SGE_STRING)  
*/


   DEXIT;
   return 0;
}


/****** src/hostgrp_success() **********************************
*
*  NAME
*     hostgrp_success() -- handle gdi SGE_HOST_GROUP_LIST on success
*
*  SYNOPSIS
*
*     #include "sge_hostgroup_qmaster.h"
*     #include <src/sge_hostgroup_qmaster.h>
* 
*     int hostgrp_success(lListElem *ep, 
*                               lListElem *old_ep, 
*                               gdi_object_t *object);
*       
*
*  FUNCTION
*     In case of successfully change of an GRP_Type element
*     perhaps some events have to take place. This is the function
*     to do that. But until yet no events are specified, so
*     this function does nothing.
*
*  INPUTS
*     lListElem *ep        - new list element (GRP_Type)
*     lListElem *old_ep    - old list element (GRP_Type)
*     gdi_object_t *object - pointer to gdi_object struct  
*
*  RESULT
*     int 0 - ok
*
*  EXAMPLE
*
* 
*  NOTES
*     Until yet no events are specified, so this function does nothing.
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/hostgrp_spool()
*     src/sge_del_hostgroup()
*     src/hostgrp_mod()
*     
****************************************************************************
*/
int hostgrp_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   DENTER(TOP_LAYER, "hostgrp_success");
   DPRINTF(("no event specified for host group changes!\n"));
   DEXIT;
   return 0;
}


/****** src/hostgrp_spool() **********************************
*
*  NAME
*     hostgrp_spool() -- handle gdi SGE_HOST_GROUP_LIST spooling
*
*  SYNOPSIS
*
*     #include "sge_hostgroup_qmaster.h"
*     #include <src/sge_hostgroup_qmaster.h>
* 
*     int hostgrp_spool(lList **alpp,
*                             lListElem *upe, 
*                             gdi_object_t *object);
*       
*
*  FUNCTION
*     Write hostgroupping entry to qmaster hostgroupping spooling file. 
*
*  INPUTS
*     lList **alpp         - answer pointer reference
*     lListElem *upe       - pointer to list element to write
*     gdi_object_t *object - pointer to gdi_object struct       
*
*  RESULT
*     int 0 ok, != 0 on error
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/hostgrp_success()
*     src/sge_del_hostgroup()
*     src/hostgrp_mod()
*     
****************************************************************************
*/
int hostgrp_spool(
lList **alpp,
lListElem *upe,
gdi_object_t *object 
) {  
   /*char fname[1000];*/
   DENTER(TOP_LAYER, "hostgrp_spool");
 
   if (write_host_group( 1 , 2 , upe ) == NULL) {
      const char* groupName = NULL;
      groupName = lGetString(upe, GRP_group_name); 
      ERROR((SGE_EVENT, MSG_HGRP_ERRORWRITESPOOLFORGROUP_S, groupName ));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }
 
   DEXIT;
   return 0;
}

/****** src/sge_del_hostgrp() **********************************
*
*  NAME
*     sge_del_hostgrp() -- handle gdi SGE_HOST_GROUP_LIST DEL request
*
*  SYNOPSIS
*
*     #include "sge_hostgroup_qmaster.h"
*     #include <src/sge_hostgroup_qmaster.h>
* 
*     int sge_del_hostgrp(lListElem *cep, 
*                              lList **alpp, 
*                              char *ruser,
*                              char *rhost);
*       
*
*  FUNCTION
*     Delete the complete host group entry.
*
*  INPUTS
*     lListElem *cep - list element to delete (GRP_Type)
*     lList **alpp   - answer list pointer reference
*     char *ruser    - user name who starts request 
*     char *rhost    - host from where the request was started 
*    
*  RESULT
*     int - STATUS_OK (=1) on success 
*           != 1 on error (STATUS_XX)
*
*  EXAMPLE
*
*
*  NOTES
*
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/hostgrp_success()
*     src/hostgrp_spool()
*     src/hostgrp_mod()
*     
****************************************************************************
*/
int sge_del_hostgrp(
lListElem *cep,
lList **alpp,
char *ruser,
char *rhost 
) {
   const char* groupName = NULL;
   lListElem* ep = NULL;
   lListElem* tmp_ep = NULL;
   lList* subgroupList = NULL;

   

   DENTER(TOP_LAYER, "sge_del_hostgroup");
   if ( !cep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
    
   DPRINTF(("ruser '%s'\n",ruser));
   DPRINTF(("rhost '%s'\n",rhost));

   groupName = lGetString(cep, GRP_group_name);
   if (groupName == NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(GRP_group_name), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }   
 
   ep = sge_get_group_elem(Master_Host_Group_List,groupName);
   if (ep == NULL) { 
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, "host group entry", groupName ));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;  
   }   

         
   /* check if group is referenced in user mapping */
   for_each(tmp_ep,Master_Usermapping_Entry_List ) {
       const char* clusterName = NULL;
       lList* mapList = NULL;
       clusterName = lGetString(tmp_ep, UME_cluster_user);
       if (clusterName == NULL) {
          clusterName = "unknown";
       }
       mapList = lGetList(tmp_ep, UME_mapping_list);
       if (mapList != NULL) {
          if (sge_getUserNameForHost(Master_Host_Group_List, mapList, groupName) != NULL) {
             /* found reference in user mapping */
             ERROR((SGE_EVENT, MSG_ANSER_CANTDELETEHGRPXREFERENCEDINUSERMAPPINGFORCLUSTERUSERY_SS, groupName, clusterName ));
             answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
             DEXIT;
             return STATUS_EEXIST;
          }
       }
   } 


   /* remove host file */
   if (sge_unlink(HOSTGROUP_DIR, groupName)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, "host group entry",groupName ));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }
 

 
   /* delete found pe element */
   subgroupList = lGetList(ep, GRP_subgroup_list);
   if (subgroupList != NULL) {
      tmp_ep = subgroupList->first;
      while (tmp_ep != NULL) {
         const char* tmpSubgroup = NULL;
         tmpSubgroup = lGetString(tmp_ep, STR);
         if (tmpSubgroup != NULL) {
            DPRINTF(("remove subgroup entrie %s in subgroup list of %s\n", tmpSubgroup, groupName));
            sge_del_subgroup_from_group(Master_Host_Group_List,ep, tmpSubgroup);
         }
         subgroupList = lGetList(ep, GRP_subgroup_list);
         if (subgroupList != NULL) {
           tmp_ep = subgroupList->first;
         } else {
           tmp_ep = NULL; 
         }
      }
   }   

   lRemoveElem(Master_Host_Group_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost,groupName , "host group entry"  ));

   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;

   
}

#endif /* __SGE_NO_USERMAPPING__ */

