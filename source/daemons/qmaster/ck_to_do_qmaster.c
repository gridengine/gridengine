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
#include "sge_m_event.h"
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

static void log_consumables(FILE *fp, lList *actual, lList *total); 
static void log_stat_file(u_long32 now);

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
      { 0, NULL }
   };
   te_deliver(now, te_tab);
}

   log_stat_file(now);
   sge_load_value_garbage_collector(now);

   ck_4_zombie_jobs(now);
   ck_4_deliver_events(now);

   /*
   ** security hook
   */
   sge_security_ck_to_do();

   DEXIT;
   return;
}

static void log_stat_file(
u_long32 now 
) {
   stringT str;
   FILE *fp;
   static u_long32 last_stat_log_time = 0;
   double load_avg = 0.0, vmem = 0.0;
   SGE_STRUCT_STAT statbuf;
   lListElem *hep, *ep, *qep;

   DENTER(TOP_LAYER, "log_stat_file");

   if (!last_stat_log_time || now > (last_stat_log_time + conf.stat_log_time)) {
      if (SGE_STAT(path.stat_file, &statbuf)) {
         close(creat(path.stat_file, 0644));
      }
      fp = fopen(path.stat_file, "a");
      if (fp) {
         for_each(qep, Master_Queue_List) {
            memset(str, 0, sizeof(str));

            if (( hep = host_list_locate(Master_Exechost_List, lGetHost(qep, QU_qhostname)))) {
                /* use load avg */
                if (( ep = lGetSubStr(hep, HL_name, LOAD_ATTR_LOAD_AVG, EH_load_list)))
                   load_avg = strtod(lGetString(ep, HL_value), NULL);
                /* use vmem */
                /* JG: TODO: outputs load instead of vmem. Which value to use? */
                if (( ep = lGetSubStr(hep, HL_name, LOAD_ATTR_LOAD_AVG, EH_load_list)))
                   vmem = strtod(lGetString(ep, HL_value), NULL);
            }

            fprintf(fp, "%u:%s:%s:%.2f:%.2f:", 
                     (unsigned) now,
                     lGetHost(qep, QU_qhostname), 
                     lGetString(qep, QU_qname),
                     load_avg, 
                     vmem);
                     /* ,(int) lGetUlong(qep, QU_job_slots),
                     (int) qslots_used(qep) */

            /* states */
            queue_get_state_string(str, lGetUlong(qep, QU_state));
            fprintf(fp, "%s:", str);

            /* queue consumables */
            log_consumables(fp, lGetList(qep, QU_consumable_actual_list), lGetList(qep, QU_consumable_config_list));
            fprintf(fp, ":");

            /* host consumables */
            log_consumables(fp, lGetList(hep, EH_consumable_actual_list), lGetList(hep, EH_consumable_config_list));
            fprintf(fp, ":");

            /* global consumables */
            if ((hep = host_list_locate(Master_Exechost_List, "global")))
               log_consumables(fp, lGetList(hep, EH_consumable_actual_list), lGetList(hep, EH_consumable_config_list));
            fprintf(fp, "\n");

         }
         fclose(fp);
      }
      else {
         ERROR((SGE_EVENT, MSG_FILE_OPEN_S, path.stat_file));
      }
      last_stat_log_time = now;
   }

   DEXIT;
   return;
}

static void log_consumables(
FILE *fp,
lList *actual, 
lList *total  
) {
   char *act, *tot;

   lListElem *cep, *total_ep; 
   for_each (cep, actual) {
      total_ep = lGetElemStr(total, CE_name, lGetString(cep, CE_name));
      act = strdup(resource_descr(lGetDouble(cep, CE_doubleval), lGetUlong(cep, CE_valtype), NULL));
      tot = strdup(resource_descr(lGetDouble(total_ep, CE_doubleval), lGetUlong(total_ep, CE_valtype), NULL));
      fprintf(fp, "%s=%s=%s", lGetString(cep, CE_name), tot?tot:"", act?act:""); 
      if (tot)
         free(tot);
      if (act)
         free(act);
      if (lNext(cep))
         fprintf(fp, ","); 
   }
}
