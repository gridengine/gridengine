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
#include "sge_schedd_conf.h"
#include "sge_usageL.h"
#include "sched_conf_qmaster.h"
#include "sge_event_master.h"
#include "sge_sched.h"
#include "sge_log.h"
#include "setup_path.h"
#include "sge_answer.h"
#include "sge_centry.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

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
   u_long32 old_SC_weight_tickets_deadline_active, old_SC_weight_tickets_override;
  lList *temp_conf_list = NULL;

   DENTER(TOP_LAYER, "sge_mod_sched_configuration");

   if ( !confp || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   temp_conf_list = lCreateList("sched config", SC_Type);
   lAppendElem(temp_conf_list, lCopyElem(confp));

   /* just check and log */
   if (!sconf_validate_config(alpp, temp_conf_list)) {
      answer_list_output(alpp);
      lDechainElem(temp_conf_list, confp);
      lFreeList(temp_conf_list);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   
   /* save internal values */
   old_SC_weight_tickets_deadline_active = 
      lGetUlong(lFirst(*confl), SC_weight_tickets_deadline_active);
   old_SC_weight_tickets_override = 
      lGetUlong(lFirst(*confl), SC_weight_tickets_override);

   lFreeList(*confl);
   *confl = temp_conf_list;

   lSetUlong(confp, SC_weight_tickets_deadline_active, 
      old_SC_weight_tickets_deadline_active);
   lSetUlong(confp, SC_weight_tickets_override, 
      old_SC_weight_tickets_override);
   lAppendElem(*confl, lCopyElem(confp));

   if (!sge_event_spool(alpp, 0, sgeE_SCHED_CONF, 
                        0, 0, "schedd_conf", NULL, NULL,
                        confp, NULL, NULL, true, true)) {
      answer_list_add(alpp, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return -1;
   }

   INFO((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSSS, ruser, rhost, "scheduler", 
        "scheduler configuration"));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);

   DEXIT;
   return STATUS_OK;
}

