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

#include "sge_string.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "sge_userprj.h"

lList *Master_Project_List = NULL;
lList *Master_User_List = NULL;

/****** gdi/userprj/userprj_list_locate() *************************************
*  NAME
*     userprj_list_locate() -- Find a user/project in a list 
*
*  SYNOPSIS
*     lListElem* userprj_list_locate(lList *userprj_list, 
*                                    const char *uerprj_name) 
*
*  FUNCTION
*     Find a user/project with the primary key "userprj_name" in
*     the "userprj_list". 
*
*  INPUTS
*     lList *userprj_list     - UP_Type list 
*     const char *uerprj_name - User or project name 
*
*  RESULT
*     lListElem* - pointer to user or project element
*******************************************************************************/
lListElem *userprj_list_locate(lList *userprj_list, 
                               const char *uerprj_name) 
{
   return lGetElemStr(userprj_list, UP_name, uerprj_name);
}

/****** gdi/userprj/userprj_update_master_list() *****************************
*  NAME
*     userprj_update_master_list() -- update the master lists of users and projects
*
*  SYNOPSIS
*     int userprj_update_master_list(sge_event_type type, 
*                                    sge_event_action action, 
*                                    lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master lists of users and projects
*     based on an event.
*     The function is called from the event mirroring interface.
*     Depending on the event received, either the user list or
*     the project list is updated.
*
*  INPUTS
*     sge_event_type type     - event type
*     sge_event_action action - action to perform
*     lListElem *event        - the raw event
*     void *clientdata        - client data
*
*  RESULT
*     int - TRUE, if update is successfull, else FALSE
*
*  NOTES
*     The function should only be called from the event mirror interface.
*
*  SEE ALSO
*     Eventmirror/--Eventmirror
*     Eventmirror/sge_mirror_update_master_list()
*     Eventmirror/sge_mirror_update_master_list_str_key()
*******************************************************************************/
int userprj_update_master_list(sge_event_type type, sge_event_action action, 
                              lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "userprj_update_master_list");

   list_descr = UP_Type;
   key_nm = UP_name;

   switch(type) {
      case SGE_EMT_PROJECT:
         list = &Master_Project_List;
         break;
      case SGE_EMT_USER:  
         list = &Master_User_List;
         break;
      default:
         return FALSE;
   }

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}
