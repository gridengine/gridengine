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
#include "sge_gdi.h"
#include "gdi_qmod.h"
#include "sgermon.h"
#include "parse.h"
#include "sge_answer.h"

#include "sge_str.h"
#include "sge_qinstance_state.h"
#include "sge_id.h"

#include "msg_common.h"
#include "msg_gdilib.h"


/* EB: TODO: remove */

#if 0
/*
** NAME
**   gdi_qmod   - client code to en/dis/able or un/suspend queues/jobs 
** PARAMETER
**   ref_list     - queue reference list, ST_Type
**   option_flags  - 0 or QI_TRANSITION_OPTION
**   action_flag   - state transition
**
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
lList *
gdi_qmod(lList *ref_list, u_long32 option_flags, u_long32 action_flag) 
{
   lList *alp = NULL;
   lList *id_list = NULL;

   DENTER(TOP_LAYER, "gdi_qmod");

   if (!id_list_build_from_str_list(&id_list, &alp, ref_list, action_flag, option_flags)) {
      DEXIT;
      return alp;
   }

   if (id_list) {
      alp = sge_gdi(SGE_CQUEUE_LIST, SGE_GDI_TRIGGER, &id_list, NULL, NULL);
      id_list = lFreeList(id_list);
   }

   DEXIT;
   return alp;
}
#endif
