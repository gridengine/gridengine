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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "sge_unistd.h"
#include "sge_conf.h"
#include "sge_all_listsL.h"
#include "sge_host.h"
#include "sge_host_qmaster.h"
#include "sge_give_jobs.h"
#include "sge_event_master.h"
#include "sge_qmod_qmaster.h"
#include "ck_to_do_qmaster.h"
#include "sgermon.h"
#include "sge_time.h"
#include "slots_used.h"
#include "sge_log.h"
#include "time_event.h"
#include "sge_calendar_qmaster.h"
#include "sge_parse_num_par.h"
#include "setup_path.h"
#include "reschedule.h"
#include "msg_qmaster.h"
#include "sge_security.h"
#include "sge_queue.h"
#include "sge_centry.h"

#include "sge_reporting_qmaster.h"
#include "sge_persistence_qmaster.h"

extern lList *Master_Job_List;

/******************************************************************************/
void sge_ck_to_do_qmaster(
int had_free_epoch 
/*
   This module is used by the qmaster.

   This function provides self reauthentication
   services - if so configured.
 */

) {

   u_long32 now;

   DENTER(TOP_LAYER, "sge_ck_to_do_qmaster");

   now = sge_get_gmt();

{ /* here comes the future:
        calendar management uses a timer list  
        this could also be used to implement 
        retry mechanisms efficiently */

   static te_tab_t te_tab[] = {
      { TYPE_CALENDAR_EVENT, calendar_event },
      { TYPE_SIGNAL_RESEND_EVENT, resend_signal_event },
      { TYPE_JOB_RESEND_EVENT, resend_job },
      { TYPE_RESCHEDULE_UNKNOWN_EVENT, reschedule_unknown_event },
      { TYPE_SPOOLING_TRIGGER, deliver_spooling_trigger },
      { TYPE_REPORTING_TRIGGER, reporting_deliver_trigger },
      { TYPE_SHARELOG_TRIGGER, reporting_deliver_trigger },
      { 0, NULL }
   };
   te_deliver(now, te_tab);
}

   sge_load_value_garbage_collector(now);

   ck_4_zombie_jobs(now);
   ck_4_deliver_events(now);

   /*
   ** security hook
   */
   sge_security_ck_to_do();

#ifdef KERBEROS
   krb_renew_tgts(Master_Job_List);
#endif      

   DEXIT;
   return;
}

