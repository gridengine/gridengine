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
#include <string.h>
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "gdi_checkpermissions.h"
#include "sge_answerL.h"
#include "sge_permissionL.h"
#include "sgermon.h"


/****** gdi/sge/sge_gdi_get_mapping_name() **********************************
*
*  NAME
*     sge_gdi_get_mapping_name() -- get username for host 
*
*  SYNOPSIS
*
*     #include "gdi_checkpermissions.h"
*     #include <gdilib/gdi_checkpermissions.h>
* 
*     int sge_gdi_get_mapping_name(char* requestedHost,char* buf, int buflen)
*
*  FUNCTION
*     This function sends a PERM_Type list to the qmaster. The requestedHost
*     is stored in the PERM_req_host list entry. The qmaster will fill up
*     the PERM_Type list. The mapped user name is stored in the PERM_req_username
*     field. The function will strcpy the name into the "buf" char array if the
*     name is shorter than the given "buflen". On success the function returns
*     TRUE. 
* 
*  INPUTS
*     char* requestedHost - pointer to char array; this is the name of the host
*                           were the caller wants to get his username.
*     char* buf           - char array buffer to store the username
*     int   buflen        - length (sizeof) buf
*
*  RESULT
*     int TRUE on success, FALSE if not
* 
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     gdilib/sge_gdi_check_permission()
*     gdilib/PERM_LOWERBOUND
*     
****************************************************************************
*/
/* requestedHost is for getting information for this host */
int sge_gdi_get_mapping_name(
const char *requestedHost,
char *buf,
int buflen 
) {  
   lList* alp = NULL;
   lList* permList = NULL;
   lListElem *ep = NULL;
   const char* mapName = NULL;
   
   DENTER(TOP_LAYER, "sge_gdi_get_mapping_name");

   if (requestedHost == NULL) {
      DEXIT;
      return FALSE;
   }
   
   permList = lCreateList("permissons", PERM_Type);
   ep = lCreateElem(PERM_Type);
   lAppendElem(permList,ep);
   lSetHost(ep, PERM_req_host, requestedHost); 

   alp = sge_gdi(SGE_DUMMY_LIST, SGE_GDI_PERMCHECK ,  &permList , NULL,NULL );

   
   if (permList != NULL) {
      ep = permList->first;
      if (ep != NULL) {
         mapName = lGetString(ep, PERM_req_username ); 
      } 
   }
  
   if (mapName != NULL) {
      if ((strlen(mapName) + 1) <= buflen) {
         strcpy(buf,mapName);
         DPRINTF(("Mapping name is: '%s'\n", buf));
   
         lFreeList(permList);
         permList = NULL;
         lFreeList(alp);
         alp = NULL;
  
         DEXIT;
         return TRUE;
      }
   } 

   DPRINTF(("No mapname found!\n"));
   strcpy(buf,"");
   
   lFreeList(permList);
   permList = NULL;
   lFreeList(alp);
   alp = NULL;
   
   DEXIT;
   return FALSE;
}



/****** gdi/sge/sge_gdi_check_permission() **********************************
*
*  NAME
*     sge_gdi_check_permission() -- check permissions of gdi request 
*
*  SYNOPSIS
*
*     #include "gdi_checkpermissions.h"
*     #include <gdilib/gdi_checkpermissions.h>
* 
*     int sge_gdi_check_permission(int option);
*       
*
*  FUNCTION
*     This function asks the qmaster for the permission (PERM_Type) list. 
*     The option flag specifies which right should be checked. It can
*     be MANAGER_CHECK or/and OPERATOR_CHECK at this time. If the caller
*     has access the function returns TRUE.
* 
*  INPUTS
*     int option - check flag (MANAGER_CHECK or OPERATOR_CHECK)
*
*  RESULT
*     int TRUE if caller has the right, FALSE if not (-10 if qmaster not reachable)
* 
*  EXAMPLE
*     if (sge_gdi_check_permission( MANAGER_CHECK | OPERATOR_CHECK ) == TRUE) {
*        printf("I have manager and operator privileges\n");
*     }
* 
*     if (sge_gdi_check_permission( OPERATOR_CHECK ) == TRUE) {
*        printf("I have operator privileges\n");
*     } 
*   
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     gdilib/sge_gdi_get_mapping_name()
*     gdilib/PERM_LOWERBOUND
*     
****************************************************************************
*/
/* option is one of : MANAGER_CHECK  
   return is TRUE or FALSE on failure */
int sge_gdi_check_permission(
int option 
) {
  int access_status = FALSE;
  int failed_checks = 0;
  lList* alp = NULL;
  lList* permList = NULL;
  lUlong value;
  
  DENTER(TOP_LAYER, "sge_gdi_check_permission");

  permList = NULL; 
  alp = sge_gdi(SGE_DUMMY_LIST, SGE_GDI_PERMCHECK ,  &permList , NULL,NULL );

  if (permList == NULL) {
     DPRINTF(("Permlist is NULL\n"));
     lFreeList(alp);
     alp = NULL;
     failed_checks++;
     return -10;
  } else {
     if (permList->first == NULL) {
       DPRINTF(("Permlist has no entries\n")); 
       failed_checks++;
     } else {
       /* check permissions */
  
       /* manager check */
       if (option & MANAGER_CHECK) { 
          value = 0;
          value = lGetUlong(permList->first, PERM_manager);
          if (value != 1) { 
             failed_checks++;
          }
          DPRINTF(("MANAGER_CHECK: %ld\n", value));
       }

       /* operator check */
       if (option & OPERATOR_CHECK) { 
          value = 0;
          value = lGetUlong(permList->first, PERM_operator);
          if (value != 1) { 
             failed_checks++;
          }
          DPRINTF(("OPERATOR_CHECK: %ld\n", value));
       }
       
     }
  }

  lFreeList(permList);
  permList = NULL;
  lFreeList(alp);
  alp = NULL;

  if (failed_checks == 0) {
    access_status = TRUE;
  }

  DEXIT;
  return access_status;
}

