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
#include <fnmatch.h>
#include <ctype.h>

#include "sgermon.h"
#include "symbols.h"
#include "sge.h"
#include "sge_gdi.h"
#include "sge_time.h"
#include "sge_log.h"
#include "cull_list.h"
#include "qstat_printing.h"
#include "sge_job_refL.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_str.h"
#include "sge_range.h"

#include "qstat_filter.h"

static lEnumeration *what_JB_Type = NULL;
static lEnumeration *what_JAT_Type_template = NULL;
static lEnumeration *what_JAT_Type_list = NULL;

void qstat_filter_add_core_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_job_number,
      JB_owner,
      JB_type,
      JB_pe,
      JB_jid_predecessor_list,
      JB_job_name,
      JB_submission_time,
      JB_pe_range,
      JB_ja_structure,
      JB_ja_tasks,
      JB_ja_n_h_ids,
      JB_ja_u_h_ids,
      JB_ja_o_h_ids,
      JB_ja_s_h_ids,
      JB_ja_z_ids,
      JB_ja_template,
      JB_execution_time,
      NoName
   };
   const int nm_JAT_Type_template[] = {
      JAT_task_number,
      JAT_prio,
      JAT_hold,
      JAT_state,
      JAT_status,
      JAT_job_restarted,
      JAT_start_time,
      NoName
   };
   const int nm_JAT_Type_list[] = {
      JAT_task_number,
      JAT_status,
      JAT_granted_destin_identifier_list,
      JAT_suitable,
      JAT_granted_pe,
      JAT_state,
      JAT_prio,
      JAT_hold,
      JAT_job_restarted,
      JAT_start_time,
      NoName
   }; 
   
   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_template); 
   lMergeWhat(&what_JAT_Type_template, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_list); 
   lMergeWhat(&what_JAT_Type_list, &tmp_what);
}

void qstat_filter_add_ext_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_project,
      JB_department,
      JB_override_tickets,
      NoName
   };
   const int nm_JAT_Type_template[] = {
      JAT_ntix,
      JAT_scaled_usage_list,
      JAT_granted_pe,
      JAT_tix,
      JAT_oticket,
      JAT_fticket,
      JAT_sticket,
      JAT_share,
      NoName
   };
   const int nm_JAT_Type_list[] = {
      JAT_ntix,
      JAT_scaled_usage_list,
      JAT_task_list,
      JAT_tix,
      JAT_oticket,
      JAT_fticket,
      JAT_sticket,
      JAT_share,
      NoName
   };
   
   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_template); 
   lMergeWhat(&what_JAT_Type_template, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_list); 
   lMergeWhat(&what_JAT_Type_list, &tmp_what);
}

void qstat_filter_add_pri_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_nppri,
      JB_nurg,
      JB_priority,
      NoName
   };
   const int nm_JAT_Type_template[] = {
      JAT_ntix,
      NoName
   };
   const int nm_JAT_Type_list[] = {
      JAT_ntix,
      NoName
   };

   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_template); 
   lMergeWhat(&what_JAT_Type_template, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_list); 
   lMergeWhat(&what_JAT_Type_list, &tmp_what);
}

void qstat_filter_add_urg_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_deadline,
      JB_nurg,
      JB_urg,
      JB_rrcontr,
      JB_dlcontr,
      JB_wtcontr,
      NoName
   };
   
   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
}

void qstat_filter_add_l_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_checkpoint_name,
      JB_hard_resource_list,
      JB_soft_resource_list,
      NoName
   };
   
   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
}

void qstat_filter_add_q_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_checkpoint_name,
      JB_hard_resource_list,
      NoName
   };
   
   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
}

void qstat_filter_add_r_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_checkpoint_name,
      JB_hard_resource_list,
      JB_soft_resource_list,
      JB_hard_queue_list,
      JB_soft_queue_list,
      JB_master_hard_queue_list,
      JB_jid_request_list,
      NoName
   };
   const int nm_JAT_Type_template[] = {
      JAT_granted_pe,
      NoName
   };

   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
   tmp_what = lIntVector2What(JAT_Type, nm_JAT_Type_template); 
   lMergeWhat(&what_JAT_Type_template, &tmp_what);
}

void qstat_filter_add_xml_attributes(void) 
{
   lEnumeration *tmp_what = NULL;
   const int nm_JB_Type[] = {
      JB_jobshare,
      NoName
   };

   tmp_what = lIntVector2What(JB_Type, nm_JB_Type);
   lMergeWhat(&what_JB_Type, &tmp_what);
}

lEnumeration *qstat_get_JB_Type_filter(void) 
{
   if (what_JAT_Type_template != NULL) {
      lWhatSetSubWhat(what_JB_Type, JB_ja_template, &what_JAT_Type_template);
   }
   if (what_JAT_Type_list != NULL) {
      lWhatSetSubWhat(what_JB_Type, JB_ja_tasks, &what_JAT_Type_list);
   }
   return what_JB_Type;
}

lCondition *qstat_get_JB_Type_selection(lList *user_list, u_long32 show)
{
   lCondition *jw = NULL;
   lCondition *nw = NULL;

   /*
    * Retrieve jobs only for those users specified via -u switch
    */
   {
      lListElem *ep = NULL;

      for_each(ep, user_list) {
         nw = lWhere("%T(%I p= %s)", JB_Type, JB_owner, lGetString(ep, ST_name));
         if (jw == NULL) {
            jw = nw;
         } else {
            jw = lOrWhere(jw, nw);
         }
      }
   }

   /*
    * Select jobs according to current state
    */
   {
      lCondition *tmp_nw = NULL;

      /*
       * Pending jobs (all that are not running) 
       */
      if ((show & QSTAT_DISPLAY_PENDING) == QSTAT_DISPLAY_PENDING) {
         const u_long32 all_pending_flags = (QSTAT_DISPLAY_USERHOLD|QSTAT_DISPLAY_OPERATORHOLD|
                    QSTAT_DISPLAY_SYSTEMHOLD|QSTAT_DISPLAY_JOBHOLD|
                    QSTAT_DISPLAY_STARTTIMEHOLD|QSTAT_DISPLAY_PEND_REMAIN);
         /*
          * Fine grained stated selection for pending jobs
          * or simply all pending jobs
          */
         if (((show & all_pending_flags) == all_pending_flags) ||
             ((show & all_pending_flags) == 0)) {
            /*
             * All jobs not running (= all pending)
             */
            tmp_nw = lWhere("%T(!(%I -> %T((%I m= %u))))", JB_Type, JB_ja_tasks, 
                        JAT_Type, JAT_status, JRUNNING);
            if (nw == NULL) {
               nw = tmp_nw;
            } else {
               nw = lOrWhere(nw, tmp_nw);
            } 
         } else {
            /*
             * User Hold 
             */
            if ((show & QSTAT_DISPLAY_USERHOLD) == QSTAT_DISPLAY_USERHOLD) {
               tmp_nw = lWhere("%T(%I -> %T((%I > %u)))", JB_Type, JB_ja_u_h_ids, 
                           RN_Type, RN_min, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
            /*
             * Operator Hold 
             */
            if ((show & QSTAT_DISPLAY_OPERATORHOLD) == QSTAT_DISPLAY_OPERATORHOLD) {
               tmp_nw = lWhere("%T(%I -> %T((%I > %u)))", JB_Type, JB_ja_o_h_ids, 
                           RN_Type, RN_min, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
            /*
             * System Hold 
             */
            if ((show & QSTAT_DISPLAY_SYSTEMHOLD) == QSTAT_DISPLAY_SYSTEMHOLD) {
               tmp_nw = lWhere("%T(%I -> %T((%I > %u)))", JB_Type, JB_ja_s_h_ids, 
                           RN_Type, RN_min, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
            /*
             * Start Time Hold 
             */
            if ((show & QSTAT_DISPLAY_STARTTIMEHOLD) == QSTAT_DISPLAY_STARTTIMEHOLD) {
               tmp_nw = lWhere("%T(%I > %u)", JB_Type, JB_execution_time, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
            /*
             * Job Dependency Hold 
             */
            if ((show & QSTAT_DISPLAY_JOBHOLD) == QSTAT_DISPLAY_JOBHOLD) {
               tmp_nw = lWhere("%T(%I -> %T((%I > %u)))", JB_Type, JB_jid_predecessor_list, JRE_Type, JRE_job_number, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
            /*
             * Rescheduled and jobs in error state (not in hold/no start time/no dependency) 
             * and regular pending jobs
             */
            if ((show & QSTAT_DISPLAY_PEND_REMAIN) == QSTAT_DISPLAY_PEND_REMAIN) {
               tmp_nw = lWhere("%T(%I -> %T((%I != %u)))", JB_Type, JB_ja_tasks, 
                           JAT_Type, JAT_job_restarted, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
               tmp_nw = lWhere("%T(%I -> %T((%I m= %u)))", JB_Type, JB_ja_tasks, 
                           JAT_Type, JAT_state, JERROR);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
               tmp_nw = lWhere("%T(%I -> %T((%I > %u)))", JB_Type, JB_ja_n_h_ids, 
                           RN_Type, RN_min, 0);
               if (nw == NULL) {
                  nw = tmp_nw;
               } else {
                  nw = lOrWhere(nw, tmp_nw);
               } 
            }
         }
      }
      /*
       * Running jobs (which are not suspended) 
       *
       * NOTE: 
       *    This code is not quite correct. It select jobs
       *    which are running and not suspended (qmod -s)
       * 
       *    Jobs which are suspended due to other mechanisms
       *    (suspend on subordinate, thresholds, calendar)
       *    should be rejected too, but this is not possible
       *    because this information is not stored within
       *    job or job array task.
       *    
       *    As a result to many jobs will be requested by qsub.
       */   
      if ((show & QSTAT_DISPLAY_RUNNING) == QSTAT_DISPLAY_RUNNING) {
         tmp_nw = lWhere("%T(((%I -> %T(%I m= %u)) || (%I -> %T(%I m= %u))) && !(%I -> %T((%I m= %u))))", JB_Type, 
                         JB_ja_tasks, JAT_Type, JAT_status, JRUNNING,
                         JB_ja_tasks, JAT_Type, JAT_status, JTRANSFERING,
                         JB_ja_tasks, JAT_Type, JAT_state, JSUSPENDED);
         if (nw == NULL) {
            nw = tmp_nw;
         } else {
            nw = lOrWhere(nw, tmp_nw);
         } 
      }

      /*
       * Suspended jobs
       *
       * NOTE:
       *    see comment above
       */
      if ((show & QSTAT_DISPLAY_SUSPENDED) == QSTAT_DISPLAY_SUSPENDED) {
         tmp_nw = lWhere("%T((%I -> %T(%I m= %u)) || (%I -> %T(%I m= %u)) || (%I -> %T(%I m= %u)))", JB_Type, 
                         JB_ja_tasks, JAT_Type, JAT_status, JRUNNING,
                         JB_ja_tasks, JAT_Type, JAT_status, JTRANSFERING,
                         JB_ja_tasks, JAT_Type, JAT_state, JSUSPENDED);
         if (nw == NULL) {
            nw = tmp_nw;
         } else {
            nw = lOrWhere(nw, tmp_nw);
         } 
      }

      if (jw == NULL) {
         jw = nw;
      } else {
         jw = lAndWhere(jw, nw);
      } 
   }
   return jw;
}

