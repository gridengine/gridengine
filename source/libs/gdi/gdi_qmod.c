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
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "gdi_qmod.h"
#include "sgermon.h"
#include "parse.h"
#include "msg_gdilib.h"
#include "sge_answer.h"

/*
** NAME
**   gdi_qmod   - client code to en/dis/able or un/suspend queues/jobs 
** PARAMETER
**   ref_list     - queue reference list, ST_Type
**   option_flags  - 0 or BIT_QMOD_FORCE
**   action_flag   - QDISABLED, QENABLED, QSUSPENDED, 
**                   QRUNNING, QERROR, QRESCHEDULED
** RETURN
**   answer list 
** EXTERNAL
**
** DESCRIPTION
**
*/
/*
** problem: list type checking
*/
lList *gdi_qmod(
lList *ref_list,
u_long32 option_flags,
u_long32 action_flag 
) {
   lListElem *ref;
   lListElem *idp;
   const char *name;
   lList *alp = NULL;
   lList *id_list = NULL;
   lEnumeration *what_all;

   DENTER(TOP_LAYER, "gdi_qmod");

   if (!action_flag 
       || ((action_flag != QDISABLED) && (action_flag != QENABLED) && (action_flag != QSUSPENDED) 
           && (action_flag != QRUNNING) && (action_flag != QERROR) && (action_flag != QRESCHEDULED))) {
     answer_list_add(&alp, MSG_GDI_INVALIDACTION , STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
     DEXIT;
     return alp;
   }

   if (option_flags && (option_flags != BIT_QMOD_FORCE)) {
     answer_list_add(&alp, MSG_GDI_INVALIDOPTIONFLAG , STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
     DEXIT;
     return alp;
   }
   
   /*
   ** see if the queue names are correct
   */
   for_each(ref, ref_list) {
      name = lGetString(ref, STR);
      if (!name) {
         answer_list_add(&alp, MSG_GDI_INVALIDIDENCOUNTERED , STATUS_ENOKEY, ANSWER_QUALITY_ERROR);
         DEXIT;
         return alp;
      }
   }

   /*
   ** we put the force flag in every id element to simulate the 
   ** ancient behavior of qmod
   */
   for_each(ref, ref_list) {
      idp = NULL;
      /* job.task? */
      sge_parse_jobtasks(&id_list, &idp, lGetString(ref, STR), &alp);
      /* queue? */
      if (!idp) 
         idp = lAddElemStr(&id_list, ID_str, lGetString(ref, STR), ID_Type);
      if (!idp) {
         answer_list_add(&alp, MSG_GDI_OUTOFMEMORY , STATUS_EMALLOC, ANSWER_QUALITY_ERROR);
         id_list = lFreeList(id_list);
         DEXIT;
         return alp;
      }
      lSetUlong(idp, ID_action, action_flag); 
      lSetUlong(idp, ID_force, option_flags); 
   }
      
   if (id_list) {
      what_all = lWhat("%T(ALL)", ID_Type);
      /*
      ** no type needed here it's a special call not addressed to a 
      ** certain object
      */
      alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_TRIGGER, &id_list, NULL, what_all);
      lFreeWhat(what_all);
      id_list = lFreeList(id_list);
   }

   DEXIT;
   return alp;
}
