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

#include "sge_job.h"
#include "sge_ja_task.h"

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





