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

#include <stdlib.h>

#include "sge_string.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge_answer.h"
#include "sge_stringL.h"

#include "sge_hostname.h"

#include "commlib.h"

#include "sge_hostgroup.h"

#include "msg_common.h"
#include "sge_usermap.h"

#ifndef __SGE_NO_USERMAPPING__
lList *Master_Usermapping_Entry_List = NULL;


static bool sge_resolveHostList(lList **alpp, lList *hostGroupList, lList *hostList);

/****** src/sge_verifyMappingEntry() **********************************
*
*  NAME
*     sge_verifyMappingEntry() -- verify mapping entries 
*
*  SYNOPSIS
*
*     int sge_verifyMappingEntry(lList** alpp,
*                                     lList* hostGroupList, 
*                                     lListElem* mapEntry, 
*                                     char* filename,
*                                     lList* userMappingEntryList); 
*       
*
*  FUNCTION
*     This function checks the user mapping entries for not ambiguous
*     entries. And fills the given answer list pointer (alpp) with 
*     error messages.
*
*  INPUTS
*     lList** alpp                 - answer list pointer pointer
*     lList* hostGroupList         - GRP_Type (global host group list)
*     lListElem* mapEntry          - new map entry
*     char* filename               - filename to save entry (must be cluster user name)
*     lList* userMappingEntryList  - optional pointer to master mapping list (to
*                                    check incoming mappings (can be NULL))
*
*  RESULT
*     bool - true on success else false
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
*     
****************************************************************************
*/
bool sge_verifyMappingEntry(alpp, hostGroupList,mapEntry, filename, userMappingEntryList)
lList **alpp;        /* answer list pointer reference */
lList* hostGroupList;
lListElem* mapEntry; /* pointer to UME_Type element */
const char*      filename; /* filename for spooling (usermapping dir of qmaster) */
lList* userMappingEntryList;  /* UME_Type list (can be NULL) */
{
  /* toDO: - resolve all hostnames                    ok
           - compare UME_Cluster_user with filename   ok
           - ambiguous tests                          ok  */
  lListElem *ep = NULL;
  lList* list = NULL;
  DENTER(TOP_LAYER, "sge_verifyMappingEntry");

  if (mapEntry != NULL) {
     const char* clusterName = NULL;

     clusterName = lGetString(mapEntry,UME_cluster_user);
     if ((clusterName != NULL) && (filename != NULL)) {
        
        /* compare UME_Cluster_user with filename */
        if (strcmp(clusterName, filename)  != 0) {
            /* clusterName and filename different */
            INFO((SGE_EVENT, MSG_ANSWER_CLUSTERUNAMEXDIFFFROMY_SS, clusterName, filename));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
            DEXIT;
            return false;
        }
        /* now resolve the hostnames */ 
        list = lGetList(mapEntry, UME_mapping_list);
        if (sge_resolveMappingList(alpp, hostGroupList ,list) == false) {
            DEXIT;
            return false; 
        }
          
        /* check for unique mapping 
           this is done by get the count of matching mapping entries for
           in and outgoing requests */
        if (list != NULL) {
           for_each ( ep ,  list ) {
              lListElem* lep = NULL;
              lList* hostList = NULL;
              const char *mapname = NULL; 
              hostList = lGetList(ep, UM_host_list);
              mapname = lGetString(ep, UM_mapped_user);
            
              if ((hostList != NULL) && (mapname != NULL)) {
                 int matches = 0;
                 char *lastMapName = NULL;

                 for_each ( lep , hostList ) {
                    const char* hostName = NULL;
                    lListElem* leep = NULL;
      
                    hostName = lGetString ( lep , STR ); 
                    matches = 0; 
                    for_each ( leep ,  list ) {
                       const char *tmpMapName = NULL;
                       
                       tmpMapName = lGetString(leep, UM_mapped_user);
                       if (tmpMapName != NULL) {
                          
                          if (sge_isHostInMappingListForUser(hostGroupList,list, tmpMapName, hostName) == true) {
                             matches++;
                             lastMapName = sge_strdup(NULL, tmpMapName);  /* ATTENTION: Please don't forget to free() */
                          } 
                       }
                    }
                    
                    if (matches != 1) {
                         /* intern entries in mapping for cluster user not ambiguous (outgoing mapping) */
                         INFO((SGE_EVENT, MSG_ANSWER_CLUSTERUNAMEXNOTAMBIGUOUSMAPFORYATZ_SSS, filename,lastMapName ,hostName ));
                         DPRINTF(("matches is %d\n",matches));
                         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                         free(lastMapName);
                         lastMapName = NULL;
                         DEXIT;
                         return false;
                    }
                    free(lastMapName);
                    lastMapName = NULL;
                 }
              }

           }
        }
        /* check entires for ambiguous mappings to the other cluster users (incoming mapping request) */
        /* master mapping list pointer requested !! */
#ifndef NO_USER_MAPPING_AMBIGUOUS_IN_CHECK
        if (userMappingEntryList != NULL) {
           lListElem *ep = NULL;

           for_each( ep, userMappingEntryList ) { 
              lList* mapList = NULL;
              const char*  clusterUser = NULL;
              const char*  newClusterUser = NULL;

              clusterUser = lGetString(ep , UME_cluster_user);
              newClusterUser = filename;
              if ((clusterUser != NULL) && (newClusterUser != NULL)) {
                 if (strcmp(clusterUser,newClusterUser) == 0)
                    continue;
                 mapList = lGetList(ep, UME_mapping_list);
                
                 if (list != NULL) {
                    lListElem* new_ep = NULL;
                    for_each ( new_ep ,  list ) {
                       const char*  newMappingName = NULL; 
                       newMappingName = lGetString(new_ep, UM_mapped_user);
                       
                       if (sge_isNameInMappingList(mapList, newMappingName) == true) {
                          
                          lListElem* host_ep = NULL;
                          lList* newHostList = NULL;
                          newHostList = lGetList(new_ep, UM_host_list);
                          if (newHostList != NULL) {
                             for_each ( host_ep , newHostList ) {
                                const char *hostName = NULL;

                                hostName = lGetString (host_ep , STR);
                                if (hostName != NULL) {
                                   if (sge_isHostInHostList(hostGroupList ,
                                                            sge_getHostListForMappedUser(mapList, newMappingName), 
                                                            hostName) == true) { 
                                      /* mapping for cluster user not ambiguous (incoming mapping) */
                                      INFO((SGE_EVENT, MSG_ANSWER_DUPLICATEDMAPPINGENTRY_SSS, newMappingName, hostName, clusterUser ));
                                      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
                                      DEXIT;
                                      return false;
                                   }
                                }
                             }
                          }
                       }
                    }
                 }
              }
           }
        }
#endif 
        DEXIT;
        return true; 
     }
  }

  INFO((SGE_EVENT, MSG_NULLPOINTER ));
  answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR); 
  DEXIT;
  return false;
}

/****** src/sge_resolveMappingList() **********************************
*
*  NAME
*     sge_resolveMappingList() -- resolve hostnames in UM_Type list 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     bool   sge_resolveMappingList(lList **alpp, 
*                                       lList* hostGroupList, 
*                                       lList* mapList);
*       
*
*  FUNCTION
*     This function tries to resolve all hostnames in the given UM_Type
*     user mapping list. sge_resolveHostList() is called for each entry 
*     in the host list.
*
*  INPUTS
*     lList **alpp - pointer to answer list pointer
*     lList* hostGroupList - GRP_Type (global host group list)
*     lList* mapList - UM_Type list with user mapping entries
*
*  RESULT
*     bool  - true on success else false
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
*     src/sge_resolveHostList()
*     
****************************************************************************
*/
bool sge_resolveMappingList(
lList **alpp,        /* answer list pointer reference */
lList *hostGroupList,
lList *mapList 
) {
  int back = true;
  lListElem *ep = NULL;
  DENTER(TOP_LAYER,"sge_resolveMappingList" );
  if ((mapList != NULL)) {
    for_each( ep , mapList ) {
       lList* hostList = NULL;
       hostList = lGetList(ep, UM_host_list);
       if (sge_resolveHostList(alpp, hostGroupList,hostList) == false) {
          back = false;
       }     
    }
    DEXIT;
    return back;
  }
  DEXIT;
  return false;
}

/****** src/sge_isHostInMappingListForUser() **********************************
*
*  NAME
*     sge_isHostInMappingListForUser() -- search for host in hostlist for mapped user 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static bool   sge_isHostInMappingListForUser(lList* hostGroupList,
*                                                 lList* mapList, 
*                                                 char* mappedName, 
*                                                 char* host); 
*       
*
*  FUNCTION
*     This function is searching in the given UM_Type list mapList for an element
*     with the UM_mapped_user name given in argument mappedName. If this element
*     exist, the function is looking if the host parameter is in the hostlist for
*     this element.
*
*  INPUTS
*     lList* hostGroupList - GRP_Type (global host group list)
*     lList* mapList       - pointer to mapping list (UM_Type)
*     char* mappedName     - name of the mapped user where the hostlist is stored
*     char* host           - name of the host to look for in the hostlist
*
*  RESULT
*     int true if host was found, false on error or host not found
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
bool sge_isHostInMappingListForUser(
lList *hostGroupList,
lList *mapList,
const char *mappedName,
const char *host  
) {
   DENTER(TOP_LAYER, "sge_isHostInMappingListForUser");
   if ((mapList!= NULL) && (mappedName!= NULL) && (host!= NULL)) {
     
     lListElem* ep = NULL;

     ep = lGetElemStr(mapList, UM_mapped_user, mappedName);
     if (ep != NULL) {
        lList* hostList = NULL;
    
        hostList = lGetList(ep,UM_host_list);
        DEXIT;
        return sge_isHostInHostList(hostGroupList,hostList,host );
     }

   }
   DEXIT;
   return false;
}

/****** src/sge_isHostInHostList() **********************************
*
*  NAME
*     sge_isHostInHostList() -- search for host in the given hostlist 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static bool sge_isHostInHostList(lList* hostGroupList, lList* hostList,
*                                     char* hostName);
*       
*
*  FUNCTION
*     This function tries to find the given host (char* hostName) in
*     the given lList* ST_Type list. A given group name in the hostList 
*     will also be observed. 
* 
*  INPUTS
*     lList* hostGroupList - pointer to global group list (GRP_Type) 
*     char* hostName       - name of host we are looking for
*     lList* hostList      - list of host or group names (ST_Type)
*
*  RESULT
*     true  - if hostName was found in hostList or group
*     false - if hostName is not in hostList or group
*
*  EXAMPLE
*
*  NOTES
*     - function does strcasecmp 
*     - if hostName is a group name the function is looking for a name
*       that matches the group name in the list.
*
*  BUGS
*     no bugs known
*
*  SEE ALSO
*     src/sge_isGuiltyMappingEntry()
*     src/sge_getUserNameForHost()
*     
****************************************************************************
*/
bool sge_isHostInHostList(lList *hostGroupList, lList *hostList, 
                          const char *hostName) 
{ 
  lListElem* ep = NULL;
  DENTER(TOP_LAYER,"sge_isHostInHostList" );
  if ( (hostList != NULL) && (hostName != NULL)) {
    /* DPRINTF(("searching for '%s' in list\n", hostName)); */
   
    /* check if hostName is group name */
    if (sge_is_group(hostGroupList,hostName) != true) {
       /* hostName is host name */ 
       for_each ( ep, hostList ) {
          const char* tmpHost = NULL;
          
          tmpHost = lGetString ( ep , STR ); 
          if (tmpHost != NULL) {
             /* check if tmpHost is group name */
             if (sge_is_group(hostGroupList, tmpHost) == true) {
                /* tmpHost is group name */
                if (sge_is_member_in_group( hostGroupList, tmpHost, hostName, NULL) == true) {
                   DEXIT;
                   return true;
                } 
             } else {
                /* tmpHost is host name */ 
                if (sge_hostcmp(tmpHost,hostName) == 0 ) {
                   DEXIT;
                   return true;
                } 
             }
          }
       }
    } else {
       /* hostName is group name */ 
       DPRINTF(("got group name as host name, checking if group name is in host list\n"));
       
       for_each ( ep, hostList ) {
          const char *tmpHost = NULL;
          
          tmpHost = lGetString ( ep , STR ); 
          if (tmpHost != NULL) {
             /* check if tmpHost is group name */
             if (sge_is_group(hostGroupList, tmpHost) == true) {
                /* tmpHost is group name */
                if (sge_hostcmp(tmpHost, hostName) == 0 ) {
                   DPRINTF(("found group '%s' in hostlist\n", hostName));
                   DEXIT;
                   return true;
                } 
             }
          }
       }
    }
  }  
  DEXIT;
  return false;
}


/****** src/sge_isNameInMappingList() **********************************
*
*  NAME
*     sge_isNameInMappingList() -- check if mapping name is in mapping list 
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static bool sge_isNameInMappingList(lList* mapList, char* mappedName);
*       
*
*  FUNCTION
*     This function is looking in the given UM_Type list for the UM_mapped_user
*     entry. If the entry matches the mappedName the function returns true.
*
*  INPUTS
*     lList* mapList   - pointer to UM_Type list
*     char* mappedName - name for mapped user to search in mapList
*
*  RESULT
*     int true on success, false on error
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
bool sge_isNameInMappingList(
lList *mapList,
const char *mappedName 
) { 
  DENTER(TOP_LAYER, "sge_isNameInMappingList");
  if ( (mapList != NULL) && (mappedName != NULL) ) {
    lListElem* ep = NULL;
  
    for_each(ep,mapList) {
       const char* tmpName = NULL;

       tmpName = lGetString(ep, UM_mapped_user);
       if (tmpName != NULL) {
          if (strcmp(tmpName, mappedName) == 0) {
             DEXIT;
             return true;
          }
       }     
    }
  }
  DEXIT;
  return false;
}



/****** src/sge_resolveHostList() **********************************
*
*  NAME
*     sge_resolveHostList() -- resolve hostnames in ST_Type list
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static bool sge_resolveHostList(lList** alpp,
*                                       lList* hostGroupList,
*                                       lList* hostList);
*       
*
*  FUNCTION
*     This function does host name resolving for all hostnames in the
*     list by calling getuniquehostname() in the commd library.
*
*  INPUTS
*     lList** alpp - pointer to answer list pointer
*     lList* hostGroupList - GRP_Type (global host group list)
*     lList* hostList - list of hostnames in a ST_Type list
*
*  RESULT
*     true  - on success
*     false - on error
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
*     src/sge_resolveMappingList()
*     
****************************************************************************
*/
static bool sge_resolveHostList(
lList **alpp,
lList *hostGroupList,
lList *hostList 
) {
  lListElem *ep = NULL;
  int answer = true;
  DENTER(TOP_LAYER,"sge_resolveHostList" );
  if ((hostList != NULL)) {
    for_each( ep , hostList ) {
       const char *tmpHost = NULL;
       int back;
       char resolveHost[500];
       
       tmpHost = lGetString ( ep , STR ); 

       /* check if tmpHost is group */
       if (sge_is_group(hostGroupList,tmpHost) == true) {
          /* tmpHost is group name */
          DPRINTF(("name %s is group name, not resolved.\n", tmpHost));
       } else {
          /* tmpHost is no group name, try to resolve the name */
          back = getuniquehostname(tmpHost, resolveHost, 0); 
          if (back == 0) {
             /* hostname */ 
             DPRINTF(("hostname %s resolved to %s.\n",tmpHost ,resolveHost ));
             lSetString(ep , STR , resolveHost);
          } else {
             /* no guilty group or hostname */
             answer =  false;
             INFO((SGE_EVENT, MSG_ANSWER_UNKNOWNHOSTORGROUPNAME_S, tmpHost));
             answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
          }
       } 
    }
    DEXIT;
    return answer;
  }
  DEXIT;
  return false;
}


/****** src/sge_getHostListForMappedUser() **********************************
*
*  NAME
*     sge_getHostListForMappedUser() -- returns host list for mapped user x
*
*  SYNOPSIS
*
*     #include "sge_user_mapping.h"
*     #include <src/sge_user_mapping.h>
* 
*     static lList* sge_getHostListForMappedUser(lList* userMappingList, 
*                                              char* mapName); 
*       
*
*  FUNCTION
*     The function is looking in the userMappingList for the 
*     UM_mapped_user specified in mapName. It returns the pointer
*     to the user's ST_Type list (string list with host names).
*
*  INPUTS
*     lList* userMappingList  - Pointer to UM_Type list (sublist of UME_Type list)
*     char* mapName           - String with mapping name of cluster user
*
*  RESULT
*     lList* - Pointer to ST_Type list
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
*     src/sge_getHostListForUser()
*     src/sge_getMappingListForUser()
*     
****************************************************************************
*/
lList* sge_getHostListForMappedUser(
lList *userMappingList,
const char *mapName 
) {
  DENTER(TOP_LAYER,"sge_getHostListForMappedUser" );
  
  if ((userMappingList != NULL) && (mapName != NULL)) {
     lListElem* ep = NULL;

     for_each ( ep , userMappingList ) {
        const char *tmpName = NULL;

        tmpName = lGetString ( ep , UM_mapped_user );
        if (tmpName != NULL) {
          if (strcmp(mapName, tmpName) == 0 ) {
             /* found hostlist for mapping entry */
             DEXIT;
             return lGetList(ep , UM_host_list );
          } 
        }
     } 
  }
  DEXIT;
  return NULL;
}



#endif
