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
#include "gdi_tsm.h"
#include "sge_answerL.h"
#include "sgermon.h"

/*
** NAME
**   gdi_tsm   - trigger scheduler monitoring 
** PARAMETER
**   schedd_name   - scheduler name  - ignored!
**   cell          - ignored!
** RETURN
**   answer list 
** EXTERNAL
**
** DESCRIPTION
**
*/
lList *gdi_tsm(
const char *schedd_name,
const char *cell 
) {
   lList *alp = NULL;

   DENTER(TOP_LAYER, "gdi_tsm");

   alp = sge_gdi(SGE_SC_LIST, SGE_GDI_TRIGGER, NULL, NULL, NULL); 

   DEXIT;
   return alp;
}

/*
** NAME
**   gdi_kill  - send shutdown/kill request to scheduler, master, execds 
** PARAMETER
**   host_list     - host list, EH_Type
**   cell          - cell, ignored!!!
**   option_flags  - 0
**   action_flag   - combination of MASTER_KILL, SCHEDD_KILL, EXECD_KILL, 
**                                       JOB_KILL 
** RETURN
**   answer list
** EXTERNAL
**
** DESCRIPTION
**
*/
lList *gdi_kill(
lList *host_list,
const char *cell,
u_long32 option_flags,
u_long32 action_flag 
) {
   lListElem *hlep, *hep = NULL;
   lList *hlp = NULL, *alp = NULL, *tmpalp;

   DENTER(TOP_LAYER, "gdi_kill");

   alp = lCreateList("answer", AN_Type);

   if (action_flag & MASTER_KILL) {
      tmpalp = sge_gdi(SGE_MASTER_EVENT, SGE_GDI_TRIGGER, NULL, NULL, NULL);
      lAddList(alp, tmpalp);
   }

   if (action_flag & SCHEDD_KILL) {
      tmpalp = sge_gdi(SGE_EVENT_LIST, SGE_GDI_TRIGGER, NULL, NULL, NULL);
      lAddList(alp, tmpalp);  
   }

   if ((action_flag & EXECD_KILL) || (action_flag & JOB_KILL)) {
      if(host_list) {
         /*
         ** we have to convert the EH_Type to ID_Type
         ** It would be better to change the call to use ID_Type!
         */
         for_each(hep, host_list) {
            hlep = lAddElemStr(&hlp, ID_str, lGetHost(hep, EH_name), ID_Type);
            lSetUlong(hlep, ID_force, (action_flag & JOB_KILL)?1:0);
         }
      } else {
         hlp = lCreateList("kill all hosts", ID_Type);
         hlep = lCreateElem(ID_Type);
         lSetString(hlep, ID_str, NULL);
         lSetUlong(hlep, ID_force, (action_flag & JOB_KILL)?1:0);
         lAppendElem(hlp, hlep);
      }
      tmpalp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_TRIGGER, &hlp, NULL, NULL);
      lAddList(alp, tmpalp);
   }

   DEXIT;
   return alp;
}
