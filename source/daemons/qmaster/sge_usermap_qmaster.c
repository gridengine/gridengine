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
   This is the module for handling usermap.
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
#include "read_write_ume.h"
#include "sge_log.h"
#include "sge_c_gdi.h"
#include "sge_string.h"
#include "sge_utility.h"
#include "sge_user_mapping.h"
#include "sge_usermap_qmaster.h"
#include "sge_answer.h"
#include "sge_unistd.h"
#include "sge_hostgroup.h"
#include "sge_usermap.h"

#include "sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"

#ifndef __SGE_NO_USERMAPPING__

/****** src/usermap_mod() **********************************
*
*  NAME
*     usermap_mod() -- handle gdi SGE_USER_MAPPING_LIST MOD/ADD request 
*
*  SYNOPSIS
*
*     #include "sge_usermap_qmaster.h"
*     #include <src/sge_usermap_qmaster.h>
* 
*     int usermap_mod(lList **alpp, lListElem *modp, lListElem *ep, 
*                     int add, char *ruser, char *rhost, 
*                     gdi_object_t *object);
*       
*
*  FUNCTION
*     This function is called when the sge_c_gdi.c module gets an 
*     SGE_USER_MAPPING_LIST MOD/ADD request. The answer list is for
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
*     lListElem *modp       - empty element to fill up (UME_Type) (wenn add is 1)
*     lListElem *ep         - element to add (UME_Type) 
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
*     src/usermap_success()
*     src/usermap_spool()
*     src/sge_del_usermap()
*     
****************************************************************************
*/
int usermap_mod(
lList **alpp,
lListElem *modp, /* empty element to fill up (UME_Type) (wenn add is 1)*/ 
lListElem *ep,   /* element to add (UME_Type) */
int add,         /* is 1 on ADD mode, 0 on MOD mode */
const char *ruser,     /* user name who starts request */
const char *rhost,     /* host from where the request was started */
gdi_object_t *object,
int sub_command
) {
   const char*  clusterUser = NULL;
   lList* mapList = NULL;
   lListElem* mapElem = NULL;
   lList* tempMapList = NULL; 

   DENTER(TOP_LAYER, "usermap_mod");
   
   /*
   UME_Type list element
   |
   *---UME_cluster_user (SGE_STRING)
   *---UME_mapping_list (SGE_LIST)
             |
             |
             *----UM_mapped_user (SGE_STRING)
             *----UM_host_list   (SGE_LIST)
                       |
                       |
                       *----STR  (SGE_STRING)  String list (ST_Type)
   */

   clusterUser = lGetString(ep, UME_cluster_user);

   if (add == 1) {
      /* UME_cluster_user -------------------------------*/

      if (sge_is_valid_filename(clusterUser) != 0) {
         /* no correct filename */
         ERROR((SGE_EVENT,MSG_UM_CLUSTERUSERXNOTGUILTY_S, clusterUser ));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }  
      lSetString(modp, UME_cluster_user, clusterUser );

      /* UME_mapping_list ---------------------------------*/
      mapList = lGetList(modp, UME_mapping_list);
      if (mapList != NULL) {
         ERROR((SGE_EVENT, MSG_UM_MAPLISTFORXEXISTS_S, clusterUser ));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      mapList = NULL; 
      mapList = lCreateList("mapping user list", UM_Type);
      lSetList(modp, UME_mapping_list, mapList); 

      /* UM_host_list --------------------------------------*/
      /*hostList = lCreateList("host list", ST_Type);
      lSetList(mapList, UM_host_list, hostList);*/


      /* UM_mapped_user ------------------------------------*/

   } /* if (add == 1) */ 

   /* a guilty UME_mapping_list is existing */         
   mapList = NULL;
   mapList = lGetList(modp, UME_mapping_list);
   if (mapList == NULL) {
      ERROR((SGE_EVENT, MSG_UM_NOMAPLISTFORXFOUND_S, clusterUser ));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   } 

   /*
   UME_Type list element
   |
   *---UME_cluster_user (SGE_STRING)
   *---UME_mapping_list (SGE_LIST)
             |
             |
             *----UM_mapped_user (SGE_STRING)
             *----UM_host_list   (SGE_LIST)
                       |
                       |
                       *----STR  (SGE_STRING)  String list (ST_Type)
   */
   if (sge_verifyMappingEntry(alpp, Master_Host_Group_List ,ep, clusterUser,Master_Usermapping_Entry_List) != true) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((tempMapList=lGetList(ep, UME_mapping_list)) != NULL) {
  
     /* toDO: try to resolve only once */
     /* resolving is now done in verifyMappingEntry */ 
     
     for_each(mapElem, tempMapList) {
        /* mapElem is UM_Type element from ep mapping list*/
        const char*  actMapName = NULL;
        lList* actHostList = NULL;

        actMapName  =  lGetString(mapElem, UM_mapped_user);
        actHostList =  lGetList  (mapElem, UM_host_list);
        INFO((SGE_EVENT,MSG_UM_EXIMINEMAPFORX_S, clusterUser ));
        answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_ERROR);

        if (sge_addMappingEntry(alpp, Master_Host_Group_List, mapList , actMapName , actHostList, false) == false) 
        {
           if (actMapName == NULL) {
             actMapName = "unknown";
           }
           ERROR((SGE_EVENT, MSG_UM_ERRORADDMAPENTRYXFORY_SS, actMapName ,clusterUser ));
           answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
           DEXIT;
           return STATUS_EUNKNOWN;
        } 
         
     } 
   }

   /* remove deleted entries in mapList */
   if ( sge_removeOverstock(alpp,modp, ep) == true) {
      DPRINTF(("removed some entr(y/ies)\n"));
   }
   DEXIT;
   return 0;
}
/****** src/usermap_success() **********************************
*
*  NAME
*     usermap_success() -- handle gdi SGE_USER_MAPPING_LIST on success
*
*  SYNOPSIS
*
*     #include "sge_usermap_qmaster.h"
*     #include <src/sge_usermap_qmaster.h>
* 
*     int usermap_success(lListElem *ep, lListElem *old_ep, 
*                         gdi_object_t *object));
*       
*
*  FUNCTION
*     In case of successfully change of an UME_Type element
*     perhaps some events have to take place. This is the function
*     to do that. But until yet no events are specified, so
*     this function does nothing.
*
*  INPUTS
*     lListElem *ep        - new list element (UME_Type)
*     lListElem *old_ep    - old list element (UME_Type)
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
*     src/usermap_spool()
*     src/sge_del_usermap()
*     src/usermap_mod()
*     
****************************************************************************
*/
int usermap_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   DENTER(TOP_LAYER, "usermap_success");
   DPRINTF(("no event specified for user mapping changes!\n"));
   DEXIT;
   return 0;
}


/****** src/usermap_spool() **********************************
*
*  NAME
*     usermap_spool() -- handle gdi SGE_USER_MAPPING_LIST spooling
*
*  SYNOPSIS
*
*     #include "sge_usermap_qmaster.h"
*     #include <src/sge_usermap_qmaster.h>
* 
*     int usermap_spool(lList **alpp, lListElem *upe, gdi_object_t *object);
*       
*
*  FUNCTION
*     Write usermapping entry to qmaster usermapping spooling file. 
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
*     src/usermap_success()
*     src/sge_del_usermap()
*     src/usermap_mod()
*     
****************************************************************************
*/
int usermap_spool(
lList **alpp,
lListElem *upe,
gdi_object_t *object 
) {  
   DENTER(TOP_LAYER, "usermap_spool");
 
   if (!spool_write_object(spool_get_default_context(), upe, lGetString(upe, UME_cluster_user), SGE_TYPE_USERMAPPING)) {
      const char* clusterUser = NULL;
      clusterUser = lGetString(upe, UME_cluster_user); 
      ERROR((SGE_EVENT, MSG_UM_ERRORWRITESPOOLFORUSER_S, clusterUser ));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return 1;
   }
 
   DEXIT;
   return 0;
}

/****** src/sge_del_usermap() **********************************
*
*  NAME
*     sge_del_usermap() -- handle gdi SGE_USER_MAPPING_LIST DEL request
*
*  SYNOPSIS
*
*     #include "sge_usermap_qmaster.h"
*     #include <src/sge_usermap_qmaster.h>
* 
*     int sge_del_usermap(lListElem *cep, lList **alpp, char *ruser,
*                         char *rhost);
*       
*
*  FUNCTION
*     Delete the complete user mapping entry.
*
*  INPUTS
*     lListElem *cep - list element to delete (UME_Type)
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
*     src/usermap_success()
*     src/usermap_spool()
*     src/usermap_mod()
*     
****************************************************************************
*/
int sge_del_usermap(
lListElem *cep,
lList **alpp,
char *ruser,
char *rhost 
) {
   const char* clusterUser = NULL;
   lListElem* ep = NULL;

   DENTER(TOP_LAYER, "sge_del_usermap");
   if ( !cep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
    
   DPRINTF(("ruser '%s'\n",ruser));
   DPRINTF(("rhost '%s'\n",rhost));

   clusterUser = lGetString(cep, UME_cluster_user);
   if (clusterUser == NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(UME_cluster_user), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }   
 
   ep = sge_getElementFromMappingEntryList(Master_Usermapping_Entry_List, clusterUser);
   if (ep == NULL) { 
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, "user mapping entry", clusterUser ));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;  
   }   

   /* remove host file */
   if (sge_unlink(UME_DIR, clusterUser)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, "user mapping entry",clusterUser ));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }
  
   /* delete found pe element */
   lRemoveElem(Master_Usermapping_Entry_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost,clusterUser , "user mapping entry"  ));

   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;
   
}
#endif /* __SGE_NO_USERMAPPING__ */


