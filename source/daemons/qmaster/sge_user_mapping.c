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
  This module is used for administrator user mapping
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
#include "sge_user_mapping.h"
#include "sge_stringL.h"
#include "sge_stdlib.h"
#include "commlib.h"
#include "sge_log.h"
#include "msg_common.h"
#include "msg_qmaster.h"
#include "sge_security.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_hostgroup.h"
#include "sge_usermap.h"

static int   sge_isGuiltyMappingEntry(lList *hostGroupList, lList *mapList, char *foreignName, char *hostName);
static char* sge_malloc_map_in_going_username(lList *hostGroupList, lList *userMappingEntryList, char *foreignName, char *hostName);

static int   sge_addHostToHostList(lList* hostGroupList, lList* stringList, const char* newHostName, int doResolving);


static lList* sge_getMappingListForUser(lList *userMappingEntryList, const char *clusterName);
static lList* sge_getHostListForUser(lList *userMappingEntryList, char *clusterName, char *mapName);



/****** src/sge_isGuiltyMappingEntry() **********************************
*
*  NAME
*     sge_isGuiltyMappingEntry() -- search for mapping entry  
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static int sge_isGuiltyMappingEntry(lList* hostGroupList,
*                                         lList* mapList, 
*                                         char*  foreignName, 
*                                         char*  hostName);
*       
*
*  FUNCTION
*     This function is searching for an guilty user mapping entry in
*     the given UM_Type map list.
*
*  INPUTS
*     lList* hostGroupList - pointer to global host group list 
*     lList* mapList       - UM_Type list
*     char* foreignName    - user name, not known in cluster
*     char* hostName       - host of the foreign user
*
*  RESULT
*     TRUE  -  a guilty entry was found  
*     FALSE -  there is no mapping entry in the mapList
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
*     src/sge_isHostInHostList()
*     src/sge_getUserNameForHost()
*     
****************************************************************************
*/
static int sge_isGuiltyMappingEntry(
lList *hostGroupList,
lList *mapList,
char *foreignName,
char *hostName 
) {
  int matches = 0;
  lListElem *ep = NULL;
  DENTER(TOP_LAYER,"sge_isGuiltyMappingEntry" );
  if ((foreignName != NULL) && (hostName != NULL) && (mapList != NULL)) {
    DPRINTF(("searching for user '%s' on host '%s' in list\n",foreignName ,hostName));
    
    for_each( ep , mapList ) {
       const char *tmpName = NULL;
       lList* hostList = NULL;

       tmpName = lGetString(ep , UM_mapped_user);
       hostList = lGetList(ep, UM_host_list);

       if (tmpName != NULL) {
          if (strcmp(foreignName, tmpName) == 0) {
            if (sge_isHostInHostList(hostGroupList, hostList,hostName) == TRUE) {
               DPRINTF(("found host '%s' in mapping list '%s'\n",hostName, tmpName));
               matches++;
            }
          }
       }
    }
  }

  if (matches == 1) {
     DEXIT;
     return TRUE;               
  }

  if (matches > 1) {
     DPRINTF(("ambiguous entries found - mapping is not guilty\n"));
  }

  DEXIT;
  return FALSE;
}



/****** src/sge_malloc_map_in_going_username() **********************************
*
*  NAME
*     sge_malloc_map_in_going_username() -- malloc string with mapped username  
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static char* sge_malloc_map_in_going_username(lList* hostGroupList,
*                                                   lList* userMappingEntryList,
*                                                   char* foreignName, 
*                                                   char* hostName);
*       
*
*  FUNCTION
*     This function malloc's memory for the mapped username and returns the
*     pointer to the char* array. The user must call the free() function
*     for the pointer.
*
*  INPUTS
*     lList* hostGroupList        - GRP_Type list (pointer to global host group list)
*     lList* userMappingEntryList - UME_Type list
*     char* foreignName           - username of foreign host
*     char* hostName              - hostname of host, not known in cluster
*
*  RESULT
*     NULL  - No user mapping entry found
*     char* - name of foreign user in the cluster 
*
*  EXAMPLE
*
*
*  NOTES
*     The returned pointer to the char* string must be cleared by the user.
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/sge_malloc_map_out_going_username()
*     src/sge_map_gdi_request()
*     
****************************************************************************
*/
static char* sge_malloc_map_in_going_username(
lList *hostGroupList,
lList *userMappingEntryList,
char *foreignName,
char *hostName 
) { 
  /* malloc of a new string (with new user name) 
     if no user is found the function returns NULL */
  char* clusterName = NULL;
  int matches = 0;
  DENTER(TOP_LAYER,"sge_malloc_map_in_going_username" );

  if ((foreignName != NULL) && (hostName != NULL)) {
    {
      if (userMappingEntryList != NULL) {
         lListElem *ep = NULL;
         DPRINTF(("Looking for mapping entries for user %s from host %s\n",foreignName, hostName));
         
            
         for_each( ep , userMappingEntryList ) {
            lList* mapList = NULL;
            const char*  clusterUser = NULL;

            clusterUser = lGetString(ep , UME_cluster_user);
            if (clusterUser != NULL) {
               /*DPRINTF(("searching for map entry for cluster user '%s'\n", clusterUser));*/
               mapList = lGetList(ep, UME_mapping_list);
               DPRINTF(("examine list for cluster user '%s'\n",clusterUser));
               if (sge_isGuiltyMappingEntry(hostGroupList , mapList, foreignName, hostName) == TRUE) {
                   matches++;
                   DPRINTF(("found guilty mapping entry for '%s' on host '%s' as cluster user '%s'\n", 
                            foreignName, hostName, clusterUser));
                   clusterName = sge_strdup(NULL,clusterUser);
               }
            }
         }
      }
    }
  }

  if (matches > 1) {
    DPRINTF(("ambiguous mapping entry found - perform no mapping\n"));
    free(clusterName);
    clusterName = NULL;
  }

  DEXIT;
  return clusterName;
}



/****** src/sge_malloc_map_out_going_username() **********************************
*
*  NAME
*     sge_malloc_map_out_going_username() --  
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     char* sge_malloc_map_out_going_username(lList* hostGroupList,
*                                             lList* userMappingEntryList, 
*                                             char* clusterName, 
*                                             char* hostName); 
*       
*
*  FUNCTION
*     This function malloc's memory for the mapped username and returns the
*     pointer to the char* array. The user must call the free() function
*     for the pointer.
*
*  INPUTS
*     lList* hostGroupList          - GRP_Type list (pointer to global host group list)
*     lList* userMappingEntryList   - UME_Type list
*     char* clusterName             - name of user in the cluster
*     char* hostName                - hostname where clusteruser should be mapped
*
*  RESULT
*     NULL  - No user mapping entry found
*     char* - name of cluster user on the foreign host
*
*  EXAMPLE
*
*
*  NOTES
*     The returned pointer to the char* string must be cleared by the user.
*
*  BUGS
*     no bugs known
*
*
*  SEE ALSO
*     src/sge_malloc_map_in_going_username()
*     src/sge_map_gdi_request()
*     src/sge_getUserNameForHost()
*     
****************************************************************************
*/
char* sge_malloc_map_out_going_username(
lList *hostGroupList,
lList *userMappingEntryList,
const char *clusterName,
const char *hostName 
) { 
  /* malloc of a new string (with new user name) 
     if no user is found the function returns NULL */
  char* mapName = NULL;
  DENTER(TOP_LAYER,"sge_malloc_map_out_going_username" );
  
  if ((clusterName != NULL) && (hostName != NULL) && (userMappingEntryList != NULL)) {
    {
/*       lListElem *ep = NULL; */
      lList* mapList = NULL;
      DPRINTF(("Received map request for user %s to exec host %s\n",clusterName, hostName)); 
      
      mapList = sge_getMappingListForUser(userMappingEntryList, clusterName);
      if (mapList != NULL) {
         const char*  foreignName = NULL;

         foreignName = sge_getUserNameForHost(hostGroupList, mapList, hostName);
         if (foreignName != NULL) {
            mapName = sge_strdup(NULL, foreignName);
         }
      }
    }
  }

  DEXIT;
  return mapName;
}



/****** src/sge_map_gdi_request() **********************************
*
*  NAME
*     sge_map_gdi_request() -- user mapping on gdi request 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     int   sge_map_gdi_request(lList* hostGroupList,
*                               lList* userMappingEntryList, 
*                               sge_gdi_request* pApiRequest);
*       
*
*  FUNCTION
*     This function is called from the qmaster when he receives an
*     gdi request of an gdi client. If a guilty user mapping is 
*     defined in the user mapping entry list then the user name
*     is mapped.
*
*  INPUTS
*     lList* hostGroupList         - GRP_Type list (pointer to global host group list)
*     lList* userMappingEntryList  - UME_Type list
*     sge_gdi_request* pApiRequest - pointer to gdi request struct
*
*  RESULT
*     TRUE  - user was mapped and pApiRequest is changed 
*     FALSE - pApiRequest is unchanged, no guilty user mapping entry 
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
*     src/sge_malloc_map_in_going_username()
*     src/sge_malloc_map_out_going_username()
*     src/sge_getUserNameForHost()
*     
****************************************************************************
*/
int sge_map_gdi_request(
lList *hostGroupList,
lList *userMappingEntryList,
sge_gdi_request *pApiRequest 
) {  
   
   char* mappedUser = NULL;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];

   DENTER(TOP_LAYER,"sge_map_gdi_request" );


   if ((pApiRequest == NULL) || (userMappingEntryList == NULL)) { 
      DEXIT;
      return FALSE;
   }
   
   if (sge_get_auth_info(pApiRequest, &uid, user, &gid, group) == -1) {
      DEXIT;
      return FALSE;
   }

   if ((user == NULL) || (pApiRequest->host == NULL)) {
      DEXIT;
      return FALSE;
   }

   mappedUser = sge_malloc_map_in_going_username(hostGroupList,
                                                 userMappingEntryList, 
                                                 user, 
                                                 pApiRequest->host);
   if (mappedUser == NULL) {
      DEXIT;
      return FALSE;
   }

   DPRINTF(("master mapping: user %s from host %s mapped to %s\n", user, pApiRequest->host, mappedUser));
/*   INFO((SGE_EVENT, MSG_MAPPING_USERXFROMHOSTYMAPPEDTOZ_SSS, user, pApiRequest->host, mappedUser ));*/

   if (sge_set_auth_info(pApiRequest, uid, mappedUser, gid, group) == -1) {
      FREE(mappedUser);
      DEXIT;
      return FALSE;
   }   

   FREE(mappedUser);
   DEXIT;
   return TRUE;
}



/****** src/sge_getUserNameForHost() **********************************
*
*  NAME
*     sge_getUserNameForHost() -- search username on foreign host 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     char* sge_getUserNameForHost(lList* hostGroupList,
*                                         lList* mapList, 
*                                         char* hostName);
*       
*
*  FUNCTION
*
*
*  INPUTS
*     lList* hostGroupList - GRP_Type list (global host group list)
*     lList* mapList - UM_Type
*     char* hostName - name of foreign host (execution host)
*
*  RESULT
*     NULL  - no user entry found
*     char* - username on foreign host (only pointer referenced in mapList)
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
*     src/sge_isHostInHostList()
*     src/sge_isGuiltyMappingEntry()
*     
****************************************************************************
*/
const char* sge_getUserNameForHost(
lList *hostGroupList,
lList *mapList,
const char *hostName 
) {
  const char* returnUserName = NULL;
  int matches = 0;
  DENTER(TOP_LAYER,"sge_getUserNameForHost" );

  if ((mapList != NULL) && (hostName != NULL)) {
     lListElem *ep = NULL;
     DPRINTF(("searching for username at exec host %s\n", hostName));
     
     for_each( ep , mapList ) {
        const char *userName = NULL;
        lList* hostList = NULL;

        userName = lGetString ( ep , UM_mapped_user );
        hostList = lGetList ( ep , UM_host_list );
        if ((userName != NULL) && (hostList != NULL)) {
           if (sge_isHostInHostList(hostGroupList, hostList,hostName) == TRUE ) {
              returnUserName = userName;
              matches++;
           }
        }
     }
  }

  if (matches > 1) {
     DPRINTF(("ambiguous mapping entry found - perform no mapping\n"));
     returnUserName = NULL;
  }

  DEXIT;
  return returnUserName;
}







/****** src/sge_getMappingListForUser() **********************************
*
*  NAME
*     sge_getMappingListForUser() -- returns UM_Type list for cluster user x
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static lList* sge_getMappingListForUser(lList* userMappingEntryList, 
*                                             char* clusterName); 
*       
*
*  FUNCTION
*     The function is looking in the userMappingEntryList for the 
*     UME_cluster_user specified in clusterName. It returns the pointer
*     to the user's UM_Type list (User Mapping List). 
*
*  INPUTS
*     lList* userMappingEntryList - Pointer to UME_Type list (defined in qmaster)
*     char*  clusterName          - String with name of cluster user
*
*  RESULT
*     lList* - Pointer to UM_Type list (User Mapping list) 
*     NULL   - No entry found
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
*     src/sge_getHostListForMappedUser()
*     src/sge_getHostListForUser()
*     
****************************************************************************
*/
static lList* sge_getMappingListForUser(
lList *userMappingEntryList,
const char *clusterName 
) {
  lListElem* ep = NULL;
  DENTER(TOP_LAYER,"sge_getMappingListForUser" );
  
  if ( (ep=sge_getElementFromMappingEntryList(userMappingEntryList,clusterName)) != NULL) {
     DEXIT;
     return lGetList(ep , UME_mapping_list );
  }
  DEXIT;
  return NULL;
}




/****** src/sge_getElementFromMappingEntryList() **********************************
*
*  NAME
*     sge_getElementFromMappingEntryList() -- returns list element for cluster user 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     lListElem* sge_getElementFromMappingEntryList(lList* userMappingEntryList,
*                                                   char* clusterName); 
*       
*
*  FUNCTION
*     searches for the UME_cluster_user specified in clusterName in the given 
*     UME_Type list userMappingEntryList and returns a pointer to the 
*     list element. If the list element is not found a NULL pointer is given back.
*
*  INPUTS
*     lList* userMappingEntryList  -  list of UME_Type
*     char* clusterName            -  name of a cluster user
*
*  RESULT
*     lListElem*  - pointer to list element with UME_cluster_user = clusterName
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
*     /()
*     
****************************************************************************
*/
lListElem* sge_getElementFromMappingEntryList(
lList *userMappingEntryList,
const char *clusterName 
) {
  DENTER(TOP_LAYER,"sge_getElementFromMappingEntryList" );

  if ((userMappingEntryList != NULL) && (clusterName != NULL)) {
     lListElem* ep = NULL;
     for_each ( ep , userMappingEntryList ) {
        const char *tmpName = NULL;

        tmpName = lGetString ( ep , UME_cluster_user );
        if (tmpName != NULL) {
          if (strcmp(clusterName, tmpName) == 0 ) {
             /* found cluster user mapping entry */
             DEXIT;
             return ep;
          } 
        }
     } 
  }
  DEXIT;
  return NULL;
}




/****** src/sge_getHostListForUser() **********************************
*
*  NAME
*     sge_getHostListForUser() -- get user mapping host list for cluster user 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static lList* sge_getHostListForUser(lList* userMappingEntryList, 
*                                        char* clusterName , 
*                                        char* mapName);
*       
*
*  FUNCTION
*     This function is looking in the userMappingEntryList for the 
*     cluster user clusterName. Then the mapping name mapName for
*     this user is searched. If a host list entry is found this
*     function returns the pointer to the ST_Type string list with
*     the hostnames, on which the user is mapped.
*      
*
*  INPUTS
*     lList* userMappingEntryList - pointer to a UME_Type list
*     char* clusterName           - name of the cluster user
*     char* mapName               - mapped user name
*
*  RESULT
*     lList*  -  pointer to a ST_Type list (hostlist)
*     NULL    -  entry not found
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
*     src/sge_getMappingListForUser()
*     src/sge_getHostListForMappedUser()
*     
****************************************************************************
*/
static lList* sge_getHostListForUser(
lList *userMappingEntryList,
char *clusterName,
char *mapName 
) {
  DENTER(TOP_LAYER,"sge_getHostListForUser" );

  if ((userMappingEntryList != NULL) && (clusterName != NULL) && (mapName != NULL)) {
     lList* mapList = NULL;
     lList* hostList = NULL;
     
     mapList = sge_getMappingListForUser (userMappingEntryList, clusterName ); 
     hostList = sge_getHostListForMappedUser(mapList, mapName );
     DEXIT;
     return hostList;
  }

  DEXIT;
  return NULL;
}




/****** src/sge_addHostToMappingList() **********************************
*
*  NAME
*     sge_addHostToMappingList() -- insert new host or groupname in hostlist 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     int   sge_addHostToMappingList(lList* hostGroupList,
*                                         lList* userMappingEntryList, 
*                                         char* clusterName, 
*                                         char* mapName, 
*                                         char* newHostName);
*     
*       
*
*  FUNCTION
*     This function generates a new lListElem* and stores it in the 
*     userMappingEntryList. This function does host name resolving
*     and calls getuniquehostname() in the commd library.
*
*  INPUTS
*     lList* hostGroupList        - GRP_Type list (global host group list)
*     lList* userMappingEntryList - UME_Type list (defined in qmaster)
*     char*  clusterName          - Name of cluster user
*     char*  mapName              - Mapping name of cluster user on host 
*                                   newHostName
*     char*  newHostName          - hostname where user mapName exist
*
*  RESULT
*     TRUE  - success
*     FALSE - on error
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
*     commd/getuniquehostname()
*     
****************************************************************************
*/
int sge_addHostToMappingList(
lList *hostGroupList,
lList *userMappingEntryList,
char *clusterName,
char *mapName,
char *newHostName 
) {  
   
   DENTER(TOP_LAYER,"sge_addHostToMappingList" );
    
   if ( (userMappingEntryList != NULL) && (clusterName != NULL)
        && (mapName != NULL) && (newHostName != NULL) ) {
      lList* hostList = NULL;
      int back;

      /* adding hostname */   
      hostList = sge_getHostListForUser(userMappingEntryList, clusterName, mapName); 
      back = sge_addHostToHostList(hostGroupList,hostList, newHostName,TRUE); 
      DEXIT;
      return back;
   }

   DEXIT;
   return FALSE;
}




/****** src/sge_addMappingEntry() **********************************
*
*  NAME
*     sge_addMappingEntry() -- add a mapping entry in UM_Type list 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     int   sge_addMappingEntry(lList** alpp,
*                                    lList* hostGroupList, 
*                                    lList* mapList, 
*                                    char* actMapName , 
*                                    lList* actHostList,
*                                    int doResolving);
*       
*  FUNCTION
*     tries to add mapping name actMapName for actHostList to the given
*     mapList. 
*
*  INPUTS
*     lList** alpp         - reverence to pointer of answer list
*     lList* hostGroupList - GRP_Type list (global host group list) 
*     lList* mapList       - pointer to mapping list of UM_Type
*     char* actMapName     - new mapping name
*     lList* actHostList   - new hostlist for mapping name
*     int doResolving      - TRUE if the function should perform hostname resolving
*
*  RESULT
*    int  - TRUE new entry added,  FALSE - on error
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
*     /()
*     
****************************************************************************
*/
int sge_addMappingEntry(
lList **alpp,            /* Answer List pointer reference */
lList *hostGroupList,
lList *mapList,
const char *actMapName,
lList *actHostList,
int doResolving 
) {
  DENTER(TOP_LAYER,"sge_addMappingEntry" );
  /*
   UME_Type list element
   |
   *---UME_cluster_user (SGE_STRING)
   *---UME_mapping_list (SGE_LIST) UM_Type
             |
             |
             *----UM_mapped_user (SGE_STRING)
             *----UM_host_list   (SGE_LIST)
                       |
                       |
                       *----STR  (SGE_STRING)  String list (ST_Type)
   */
  if ((mapList != NULL) && (actMapName != NULL) && (actHostList != NULL)) {
    const char* tmpName = NULL;
    lList* tmpHostList = NULL;
    int back = TRUE;
    lListElem* mapElem = NULL;
    lCondition* where = NULL;
    
    where = lWhere("%T(%I==%s)", UM_Type, UM_mapped_user, actMapName );
    
    mapElem = lFindFirst(mapList, where);
    if (mapElem == NULL) {
      mapElem = lCreateElem(UM_Type);
      lAppendElem(mapList, mapElem);
    }

    /* set actMapName ----------------------------*/
    DPRINTF(("setting map name\n")); 
    tmpName = lGetString(mapElem, UM_mapped_user); 
    if (tmpName == NULL) {
      /* new element will generate new entry */
      lSetString(mapElem, UM_mapped_user, actMapName );
      INFO((SGE_EVENT,MSG_UMAP_ADDEDENTRY_S, actMapName));
      answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
    } else {
      /* user allready exists - ok */
      INFO((SGE_EVENT,MSG_UMAP_EXAMINEMAPENTRY_S, actMapName ));
      answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
    }
    
    INFO((SGE_EVENT,MSG_UMAP_EXAMINEHOSTLISTFORMAPNAME_S, actMapName ));
    answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
    /* set actHostList --------------------------*/
    tmpHostList = lGetList(mapElem, UM_host_list);
    if (tmpHostList == NULL) {
       /* list is not existing, creating a new one an add it to the element */
       lList* newHostList = NULL;
       lListElem* tep = NULL;
       
       newHostList = lCreateList("host list",ST_Type);        

       for_each(tep, actHostList) {
          const char* actHostName = NULL;

          actHostName = lGetString(tep,STR);
          /* we don't want group resolving, so we use NULL as grouplist */
          if (sge_isHostInHostList(NULL,newHostList, actHostName) == FALSE) {
            if (sge_addHostToHostList(hostGroupList,newHostList, actHostName, doResolving) == FALSE) {
              WARNING((SGE_EVENT,MSG_UMAP_CANTADDHOSTX_S, actHostName ));
              answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
              back = FALSE;
            } else {
              INFO((SGE_EVENT,MSG_UMAP_XADDED_S , actHostName ));
              answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
            }
          } else {
            DPRINTF(("host '%s' allready in list\n",actHostName));
          }
          
       } 
       lSetList(mapElem, UM_host_list, newHostList);
    } else {
      /* list is existing, just add new hosts */ 
      lListElem* tep = NULL;
      for_each(tep, actHostList) {
         const char* actHostName = NULL;
         actHostName = lGetString(tep,STR);
         /* we don't want group resolving, so we use NULL as grouplist */
         if (sge_isHostInHostList(NULL,tmpHostList, actHostName) == FALSE) {
            if ( sge_addHostToHostList(hostGroupList,tmpHostList, actHostName, doResolving) == FALSE) {
               WARNING((SGE_EVENT,MSG_UMAP_CANTADDHOSTX_S, actHostName ));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               back = FALSE;
            } else {
              INFO((SGE_EVENT,MSG_UMAP_XADDED_S, actHostName ));
              answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
            }
         } else {
            DPRINTF(("host '%s' allready in list\n",actHostName));
         }
      }
    }
    DEXIT;
    return back;
  } /* NULL test */ 

  DEXIT;
  return FALSE;
}






/****** src/sge_addHostToHostList() **********************************
*
*  NAME
*     sge_addHostToHostList() -- add new host to host list 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static int   sge_addHostToHostList(lList* hostGroupList, 
*                                        lList* stringList, 
*                                        char* newHostName);
*       
*
*  FUNCTION
*     Append newHostName to the given ST_Type list. The function tries 
*     to resolve the host name by using the function getuniquehostname().
*     If the hostname is not resolved and the name is no group name the name 
*     will be not added.
*
*  INPUTS
*     lList* hostGroupList - GRP_Type (gobal host group list)
*     lList* stringList    - list of ST_Type with host or groupnames 
*     char* newHostName    - host or groupname to add
*
*  RESULT
*     int TRUE on success, FALSE on error      
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
*     /()
*     
****************************************************************************
*/
static int sge_addHostToHostList(
lList *hostGroupList,
lList *stringList,
const char *newHostName,
int doResolving 
) {  
   DENTER(TOP_LAYER,"sge_addHostToHostList" );
    
   if ( (stringList != NULL) && (newHostName != NULL) ) {
      int back=1;
      char resolveHost[500];

      if (sge_is_group(hostGroupList , newHostName ) == TRUE) {
         /* adding group */
         lAddElemStr(&stringList, STR , newHostName , ST_Type);
         DPRINTF(("name %s is guilty host group, adding it\n",newHostName));
         DEXIT;
         return TRUE;
      }

      if (doResolving == TRUE) {
        back = getuniquehostname(newHostName, resolveHost, 0);
      }
      if (back == 0) {     
         /* adding hostname */
         lAddElemStr(&stringList, STR , resolveHost , ST_Type);
         DPRINTF(("hostname %s resolved to %s and added\n",newHostName,resolveHost));
         DEXIT;
         return TRUE;
      } else {
         if (doResolving != TRUE) {
           /* adding hostname when no resolving is expected */
           lAddElemStr(&stringList, STR , newHostName , ST_Type);
           DPRINTF(("hostname %s added\n",newHostName)); 
           DEXIT;
           return TRUE;
         }
         /* not resolved hostname and no guilty group group */
         WARNING((SGE_EVENT,MSG_UMAP_HOSTNAMEXNOTRESOLVEDY_SS, newHostName, cl_errstr(back)));  
         DPRINTF(("name %s is no host and no host group, name rejected!\n",newHostName));
         DEXIT;
         return FALSE;
      }
   }
   DEXIT;
   return FALSE;
}






/****** src/sge_removeOverstock() **********************************
*
*  NAME
*     sge_removeOverstock() -- remove overstock entries in UME_Type element 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     int sge_removeOverstock(lList** alpp, 
*                                  lListElem* newListElem, 
*                                  lListElem* origListElem);
*       
*
*  FUNCTION
*     This function compares two UME_Type list elements. All entries
*     in the newListElem element are compared with the entries in the
*     origListElem. If there are more elements in newListElem element, 
*     the overstock elements are removed from newListElem. 
*
*
*  INPUTS
*     lList** alpp            - reference to answer list pointer
*     lListElem* newListElem  - UME_Type element to remove entries
*     lListElem* origListElem - UME_Type element to compare with newListElem
*
*  RESULT
*     int FALSE if no changes were made, TRUE if entries was deleted
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
*     /()
*     
****************************************************************************
*/
int sge_removeOverstock(
lList **alpp,            /* Answer List pointer reference */
lListElem *newListElem,     /* ListElement with perhaps to much entries */
lListElem *origListElem     /* ListElement to compare with */
) { 
  int dirty = FALSE;
  const char* clusterUser = NULL;
  DENTER(TOP_LAYER,"sge_removeOverstock" );

  if ((newListElem != NULL) && (origListElem != NULL)) {
    /* lists are not NULL -> let's start comparing */ 
    lList* newMap = NULL;
    lList* orgMap = NULL;
    lListElem* ep = NULL;

    clusterUser = lGetString(newListElem, UME_cluster_user);
    if (clusterUser == NULL) {
      clusterUser = "unknown";
    }
    newMap = lGetList(newListElem, UME_mapping_list);
    orgMap = lGetList(origListElem, UME_mapping_list);

    /* check UM_mapped_user */
    ep = lFirst(newMap);
    while (ep != NULL) {
      const char* mapName = NULL;
      
      mapName = lGetString(ep,UM_mapped_user);
      if (sge_isNameInMappingList(orgMap, mapName) == FALSE) {
          /* this name must be removed */
        lListElem* next = NULL;
        
        dirty = TRUE;
        INFO((SGE_EVENT,MSG_UMAP_REMOVEDMAPENTRYXFORCLUSERUSERY_SS, mapName, clusterUser ));
        answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
        next = lNext(ep);
        ep = lDechainElem(newMap,ep);
        lFreeElem(ep);
        ep = next;
      } else {
        /* nothing to remove */
        /* check UM_host_list */
        lListElem* ephost = NULL;
        lList* newHostList = NULL;

        newHostList = lGetList(ep, UM_host_list);
        ephost = lFirst(newHostList);
        while (ephost != NULL) {
           const char* hostName = NULL;
 
           hostName = lGetString(ephost, STR);
         
           /* do not check inside groups , use NULL as grouplist*/ 
           if (  sge_isHostInMappingListForUser(NULL,orgMap, mapName, hostName) == FALSE) {
              /* this host must be removed */
              lListElem* nexthost = NULL;
              dirty = TRUE;
              INFO((SGE_EVENT,MSG_UMAP_REMOVEDXFROMMAPENTRYYFORCLUSERUSERZ_SSS, 
                    hostName, mapName, clusterUser ));
              answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);
              nexthost = lNext(ephost);
              ephost = lDechainElem(newHostList, ephost);
              lFreeElem(ephost);
              ephost = nexthost;
           } else { 
              /* nothing to remove */
              DPRINTF(("will not remove host: %s\n",hostName));
              ephost = lNext(ephost);
           }
        }     
        ep = lNext(ep);
      }
    }

  }
  DEXIT;
  return dirty;  
}

