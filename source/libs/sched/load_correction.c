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
#include <time.h>

#include "sge_all_listsL.h"
#include "schedd_monitor.h"
#include "load_correction.h"
#include "sgermon.h"
#include "sge_time.h"
#include "schedd_conf.h"
#include "sge_complex_schedd.h"
#include "sge_parse_num_par.h"

int correct_load(
lList *lp_job,
lList *lp_queue,
lList **lpp_host,
u_long32 decay_time 
) {
   u_long32 now;
   double lcf_global = 0, add_lcf; /* additional load correction factor */
   u_long32 jstate, qstate;
   u_long32 jobid, ja_taskid, running_time;
   char *qnm, *hnm;
   lListElem *gdil, *hep;
   u_long32 slots;
   lListElem  *qep, *job, *ja_task;
   
   DENTER(TOP_LAYER, "correct_load");

   now = sge_get_gmt();

   if (!lpp_host) {
      DEXIT;
      return 1;
   }
   for_each (job, lp_job) {

      /* increase host load only for RUNNING jobs             */
      /* SUSPENDED Jobs may not increase the load of the host */
      jobid = lGetUlong(job, JB_job_number);
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         jstate = lGetUlong(ja_task, JAT_state);
         if (!(jstate & JRUNNING) || 
             (jstate & JSUSPENDED) || 
             (jstate & JSUSPENDED_ON_THRESHOLD)) {
            continue;
         }
         ja_taskid = lGetUlong(ja_task, JAT_task_number); 
         /* may be job runs longer than load_adjustment_decay_time */
         running_time = now - lGetUlong(ja_task, JAT_start_time);
#if 0
         DPRINTF(("JOB "u32"."u32" start_time = "u32" running_time "u32
            " decay_time = "u32"\n", jobid, ja_taskid, 
            lGetUlong(ja_task, JAT_start_time), running_time, 
            decay_time));
#endif
         if (running_time > decay_time)
            continue;
         
         /* Problem: Jobs in SUSPENDED queues have no SUSPENDED  */
         /*          flag set. So we have to take this info      */
         /*          directly from the queues state field.       */
         /*
         ** the gdil contains the queue(s) the job runs in
         */
         for_each (gdil, lGetList(ja_task, JAT_granted_destin_identifier_list)) {
            
            /* search for this queue */
            qep=lGetElemStr(lp_queue, QU_qname, qnm=lGetString(gdil, JG_qname));
            if (!qep) {
               DPRINTF(("Unable to find queue \"%s\" from gdil "
                  "list of job "u32"."u32"\n", qnm, jobid, ja_taskid));
               continue;
            }
            
            qstate = lGetUlong(qep, QU_state); 
            if ((qstate & QSUSPENDED) || (qstate & QSUSPENDED_ON_SUBORDINATE))
               continue; /* queue suspended - no load correction */
            
            /* search this host */
            hep = lGetElemHost(*lpp_host, EH_name, 
               hnm=lGetString(gdil, JG_qhostname));
            if (!hep) {
               DPRINTF(("Unable to find host \"%s\" from gdil "
                  "list of job "u32"."u32"\n", hnm, jobid, ja_taskid));
               continue;
            } 

            /* To implement load correction we add values between
               1 (just started) and 0 (load_adjustment_decay_time expired)
               for each job slot in the exec host field 
               EH_load_correction_factor. This field is used later on to:
               - sort hosts concerning load
               - decide about load thresholds of queues
               - resort hosts for each scheduled job          */ 
            
            /* use linear function for additional load correction factor 
                                         t
               correction(t) = 1 - ---------------- 
                                    decay_time
            */
            add_lcf = 1 - ((double) running_time/
                           (double) decay_time);

            lcf_global += add_lcf;

            /* multiply it for each slot on this host */
            slots = lGetUlong(gdil, JG_slots);
            add_lcf *= slots;
            
            /* add this factor (multiplied with 100 for being able to use u_long32) */
            lSetUlong(hep, EH_load_correction_factor, 
                      (100*add_lcf) + lGetUlong(hep, EH_load_correction_factor));

#if 0
            DPRINTF(("JOB "u32"."u32" ["u32" slots] in queue %s increased lc of host "
                     "%s by "u32" to "u32"\n", jobid, ja_taskid, slots, qnm, hnm, 
                     (u_long32)(100*add_lcf), lGetUlong(hep, EH_load_correction_factor)));
#endif
            SCHED_MON((log_string, "JOB "u32"."u32" ["u32"] in queue %s increased "
                        "absolute lc of host "
                       "%s by "u32" to "u32"", jobid, ja_taskid, slots, qnm, hnm, 
                       (u_long32)(100*add_lcf), lGetUlong(hep, 
                        EH_load_correction_factor)));
         }
      }

      hep = lGetElemStr(*lpp_host, EH_name, "global");
      lSetUlong(hep, EH_load_correction_factor, 
                (100*lcf_global) + lGetUlong(hep, EH_load_correction_factor));
   }

   DEXIT;
   return 0;
}



/*
 * Do load scaling and capacity correction for all consumable 
 *  attributes where also load values are available
 *
 */
int correct_capacities(
lList *host_list,
lList *complex_list 
) {
   lListElem *hep, *ep, *cep; 
   lListElem *job_load, *scaling, *total, *inuse_rms;
   u_long32 type, relop;
   double dval, inuse_ext, full_capacity, sc_factor;
   double load_correction;

   DENTER(TOP_LAYER, "correct_capacities");
 
   for_each (hep, host_list) {
      char *host_name = lGetString(hep, EH_name);
      for_each (ep, lGetList(hep, EH_load_list)) {
         char *attr_name = lGetString(ep, HL_name);
 
         /* seach for appropriate complex attribute */
         if (!(cep=sge_locate_complex_attr(attr_name, complex_list)))
            continue;

         type = lGetUlong(cep, CE_valtype);
         if (type != TYPE_INT &&
             type != TYPE_TIM &&
             type != TYPE_MEM &&  
             type != TYPE_BOO &&  
             type != TYPE_DOUBLE)
            continue;
        
         if (!parse_ulong_val(&dval, NULL, type, lGetString(ep, HL_value), NULL, 0))
            continue;

         /* do load scaling */
         if ((scaling=lGetSubStr(hep, HS_name, attr_name, EH_scaling_list))) {
            char sval[20];
            sc_factor = lGetDouble(scaling, HS_value);
            dval *= sc_factor;
            sprintf(sval, "%8.3f", dval);
            lSetString(ep, HL_value, sval);
         }

         if (!lGetUlong(cep, CE_consumable))
            continue;
         if (!(total=lGetSubStr(hep, CE_name, attr_name, EH_consumable_config_list)))
            continue;
         if (!(inuse_rms=lGetSubStr(hep, CE_name, attr_name, EH_consumable_actual_list)))
            continue;

         relop = lGetUlong(cep, CE_relop);
         if (relop != CMPLXEQ_OP &&
             relop != CMPLXLT_OP &&
             relop != CMPLXLE_OP &&
             relop != CMPLXNE_OP)
            continue;

         /* do load correction */
         load_correction = 0;
         if ((job_load=lGetElemStr(scheddconf.job_load_adjustments, CE_name, attr_name))) {
            double lc_factor;
            char *s = lGetString(job_load, CE_stringval);
            if (parse_ulong_val(&load_correction, NULL, type, s, NULL, 0)) {
               lc_factor = ((double)lGetUlong(hep, EH_load_correction_factor))/100.0;
               load_correction *= lc_factor;
               DPRINTF(("%s:%s %s %8.3f %8.3f\n", 
                  host_name, attr_name, s, load_correction, lc_factor));
               dval -= load_correction;
            }
         }

         /* use scaled load value to deduce the amount */
         full_capacity = lGetDouble(total, CE_doubleval);
         inuse_ext = full_capacity - lGetDouble(inuse_rms, CE_doubleval) - dval;

         if (inuse_ext > 0.0) {
            lSetDouble(total, CE_doubleval, full_capacity - inuse_ext);

            DPRINTF(("%s:%s %8.3f --> %8.3f (ext: %8.3f = all %8.3f - ubC %8.3f - load %8.3f) lc = %8.3f\n",
               host_name, attr_name, full_capacity, lGetDouble(total, CE_doubleval),
               inuse_ext, full_capacity, lGetDouble(inuse_rms, CE_doubleval), dval, load_correction));
         } else
            DPRINTF(("ext: %8.3f <= 0\n", inuse_ext));
      }
   }

   DEXIT;
   return 0;
}
