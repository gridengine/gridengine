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

#include "cull.h"
#include "sge_manop.h"

#include "sgermon.h"

lList *Master_Manager_List = NULL;
lList *Master_Operator_List = NULL;

lListElem *manop_list_locate(lList *manop_list, const char *name) 
{
   return lGetElemStr(manop_list, MO_name, name);
}

lListElem* sge_locate_manager(const char *cp) 
{
   return manop_list_locate(Master_Manager_List, cp);
}


lListElem* sge_locate_operator(const char *cp) 
{
   return manop_list_locate(Master_Operator_List, cp);
}

/****** gdi/manop/manop_update_master_list() *****************************
*  NAME
*     manop_update_master_list() -- update the master list of managers/operators
*
*  SYNOPSIS
*     int manop_update_master_list(sge_event_type type, 
*                                  sge_event_action action, 
*                                  lListElem *event, void *clientdata) 
*
*  FUNCTION
*     Update the global master lists of managers and operators
*     based on an event.
*     The function is called from the event mirroring interface.
*     Depending on the event, either the manager or the operator
*     list is updated.
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
int manop_update_master_list(sge_event_type type, sge_event_action action, 
                             lListElem *event, void *clientdata)
{
   lList **list;
   lDescr *list_descr;
   int     key_nm;
   
   const char *key;


   DENTER(TOP_LAYER, "manop_update_master_list");

   list_descr = MO_Type;
   key_nm = MO_name;

   switch(type) {
      case SGE_EMT_MANAGER:
         list = &Master_Manager_List;
         break;
      case SGE_EMT_OPERATOR:  
         list = &Master_Operator_List;
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

/* JG: TODO: naming, ADOC */
int sge_manager(
const char *cp 
) {

   DENTER(TOP_LAYER, "sge_manager");

   if (!cp) {
      DEXIT;
      return -1;
   }

   if (sge_locate_manager(cp)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return -1;

}

/***********************************************************************/
/* JG: TODO: naming, ADOC */
int sge_operator(
const char *cp 

/*
   Note: a manager is implicitly an "operator".
 */

) {

   DENTER(TOP_LAYER, "sge_operator");

   if (!cp) {
      DEXIT;
      return -1;
   }

   if (sge_locate_operator(cp)) {
      DEXIT;
      return 0;
   }

   if (sge_locate_manager(cp)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return -1;

}

