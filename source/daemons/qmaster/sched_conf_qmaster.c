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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sgermon.h"
#include "sge_answerL.h"
#include "sge_confL.h"
#include "sge_schedconfL.h"
#include "sge_usageL.h"
#include "sge_hostL.h"
#include "sge_complexL.h"
#include "sched_conf.h"
#include "sched_conf_qmaster.h"
#include "sge_m_event.h"
#include "sge_sched.h"
#include "sge_log.h"
#include "setup_path.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "msg_common.h"

/************************************************************
  sge_mod_sched_configuration - Master code

  Modify scheduler configuration. We have only one entry in
  Master_Sched_Config_List. So we replace it with the new one.
 ************************************************************/
int sge_mod_sched_configuration(
lListElem *confp,
lList **confl,    /* list to change */
lList **alpp,
char *ruser,
char *rhost 
) {
   u_long32 si;
/*    lListElem *schedd = NULL; */
   u_long32 old_SC_weight_tickets_deadline_active, old_SC_weight_tickets_override;

   DENTER(TOP_LAYER, "sge_mod_sched_configuration");

   if ( !confp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* just check and log */
   if (sc_set(alpp, NULL, confp, &si)) {
      /* alpp gets filled by sc_set() */
      ERROR((SGE_EVENT, lGetString(lLast(*alpp), AN_text))); 
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* save internal values */
   old_SC_weight_tickets_deadline_active = 
      lGetUlong(lFirst(*confl), SC_weight_tickets_deadline_active);
   old_SC_weight_tickets_override = 
      lGetUlong(lFirst(*confl), SC_weight_tickets_override);

   lFreeList(*confl);

   *confl = lCreateList("sched config", SC_Type);

   lSetUlong(confp, SC_weight_tickets_deadline_active, 
      old_SC_weight_tickets_deadline_active);
   lSetUlong(confp, SC_weight_tickets_override, 
      old_SC_weight_tickets_override);
   lAppendElem(*confl, lCopyElem(confp));

   if (write_sched_configuration(1, 2, lFirst(*confl)) == NULL) {
      sge_add_answer(alpp, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION, STATUS_ESEMANTIC, 0);
      DEXIT;
      return -1;
   }


   sge_add_event(NULL, sgeE_SCHED_CONF, 0, 0, NULL, confp);

   INFO((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS, ruser, rhost, "scheduler", 
        "scheduler configuration"));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);

   DEXIT;
   return STATUS_OK;
}

