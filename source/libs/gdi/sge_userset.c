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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "sgermon.h"
#include "sge_log.h"

#include "sge_answer.h"

#include "msg_gdilib.h"

#include "sge_userset.h"

lList *Master_Userset_List = NULL;

/*****************************************************************
 is_deadline_user

 ask whether a given user is allowed to sumbit deadline jobs.
 *****************************************************************/
int is_deadline_user(
char *username,         /* user we ask for */
lList *lp               /* userset list to scan for deadline users */
) {
   lListElem *deadline_users;

   DENTER(TOP_LAYER, "is_deadline_user");

   deadline_users = lGetElemStr(lp, US_name, DEADLINE_USERS);

   if (deadline_users && lGetSubStr(deadline_users, UE_name, username, 
         US_entries)) {
      DEXIT;
      return 1; /* found user in deadline user list */
   }

   DEXIT;
   return 0;
}

lListElem *userset_list_locate(lList *lp, const char *name) 
{
   lListElem *ep;

   DENTER(TOP_LAYER, "userset_list_locate");

   for_each(ep, lp)
      if (!strcasecmp(name, lGetString(ep, US_name))) {
         DEXIT;
         return ep;
      }

   DEXIT;
   return NULL;
}

/****** gdi/userset/userset_update_master_list() *****************************
*  NAME
*     userset_update_master_list() -- update the master list of usersets
*
*  SYNOPSIS
*     int userset_update_master_list(sge_event_type type, 
*                                    sge_event_action action, 
*                                    lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master list of usersets
*     based on an event.
*     The function is called from the event mirroring interface.
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
int userset_update_master_list(sge_event_type type, sge_event_action action, 
                               lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "userset_update_master_list");

   list = &Master_Userset_List;
   list_descr = US_Type;
   key_nm = US_name;

   key = lGetString(event, ET_strkey);

   if(sge_mirror_update_master_list_str_key(list, list_descr, key_nm, key, action, event) != SGE_EM_OK) {
      DEXIT;
      return FALSE;
   }

   DEXIT;
   return TRUE;
}

/* JG: TODO: naming, ADOC */
int verify_acl_list(
lList **alpp,
lList *acl_list,
const char *attr_name, /* e.g. "user_lists" */
const char *obj_descr, /* e.g. "queue"      */
const char *obj_name   /* e.g. "fangorn.q"  */
) {
   lListElem *usp;

   DENTER(TOP_LAYER, "verify_acl_list");

   for_each (usp, acl_list) {
      if (!lGetElemStr(Master_Userset_List, US_name, lGetString(usp, US_name))) {
         ERROR((SGE_EVENT, MSG_SGETEXT_UNKNOWNUSERSET_SSSS, lGetString(usp, US_name), 
               attr_name, obj_descr, obj_name));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

/******************************************************
   sge_verify_userset_entries()
      resolves user set/department

   usep
      cull list (UE_Type)
   alpp
      may be NULL
      is used to build up an answer
      element in case of error

   returns
      STATUS_OK         - on success
      STATUS_ESEMANTIC  - on error
 ******************************************************/
 /* JG: TODO: naming, ADOC */
int sge_verify_userset_entries(
lList *userset_entries,
lList **alpp,
int start_up 
) {
   lListElem *ep;
   int name_pos;

   DENTER(TOP_LAYER, "sge_verify_userset_entries");

   /*
      resolve cull names to positions
      for faster access in loop
   */
   name_pos = lGetPosInDescr(UE_Type, UE_name);

   for_each(ep, userset_entries)
      if (!lGetPosString(ep, name_pos)) {
         ERROR((SGE_EVENT, MSG_US_INVALIDUSERNAME));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_ESEMANTIC;
      }

   DEXIT;
   return STATUS_OK;
}

