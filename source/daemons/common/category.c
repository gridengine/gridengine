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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sge_gdi_intern.h"
#include "sge_ja_task.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_pe.h"
#include "sge_schedd.h"
#include "sge_process_events.h"
#include "sge_prog.h"
#include "sge_ctL.h"
#include "sge_schedd_conf.h"
#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "cull_sort.h"
#include "sge_event.h"
#include "sge_feature.h"
#include "schedd_conf.h"
#include "schedd_monitor.h"
#include "unparse_job_cull.h"
#include "sge_dstring.h"
#include "parse_qsubL.h"
#include "sge_access_tree.h"
#include "parse.h"
#include "category.h"
#include "sge_job.h"

#include "msg_daemons_common.h"

/*-------------------------------------------------------------------------*/
/* build the category string                                               */
/*-------------------------------------------------------------------------*/
const char* sge_build_job_category(
dstring *category_str,
lListElem *job,
lList *acl_list 
) {
   const char *cats = NULL;
   lList *cmdl = NULL;
   lListElem *ep;
   const char *owner, *group;

   DENTER(TOP_LAYER, "sge_build_job_category");
   
   /*
   ** owner -> acl
   */
   owner = lGetString(job, JB_owner);
   group = lGetString(job, JB_group);
   if (sge_unparse_acl(owner, group, "-U", acl_list, &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** -hard -q qlist
   */
   if (sge_unparse_id_list(job, JB_hard_queue_list, "-q",  
                                    &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** -masterq qlist
   */
   if (sge_unparse_id_list(job, JB_master_hard_queue_list, "-masterq",  
                                    &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** -hard -l rlist
   */
   if (sge_unparse_resource_list(job, JB_hard_resource_list, 
                                    &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** -pe pe_name pe_range
   */
   if (sge_unparse_pe(job, &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** -ckpt ckpt_name 
   */
   if (sge_unparse_string_option(job, JB_checkpoint_name, "-ckpt", 
            &cmdl, NULL) != 0) {
      DEXIT;
      goto ERROR;
   }

   /*
   ** interactive jobs
   */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) {
      ep = sge_add_arg(&cmdl, 0, lIntT, "-I", "y");
      if (!ep) {
         DEXIT;
         goto ERROR;
      }
      lSetInt(ep, SPA_argval_lIntT, true);
   }
      
   /*
   ** job type
   */

   /*
   ** SGE only
   */
   if (feature_is_enabled(FEATURE_SGEEE)) {
      /*
      ** project
      */
      if (sge_unparse_string_option(job, JB_project, "-P", 
               &cmdl, NULL) != 0) {
         DEXIT;
         goto ERROR;
      }
   }

   /*
   ** create the category string
   */
   lDelElemStr(&cmdl, SPA_switch, "-hard");
   for_each (ep, cmdl) {
      char buf[20];
      strcpy(buf, lGetString(ep, SPA_switch));
      strcat(buf, " ");
      cats = sge_dstring_append(category_str, buf);
      if (lGetString(ep, SPA_switch_arg))
         cats = sge_dstring_append(category_str, lGetString(ep, SPA_switch_arg));
      cats = sge_dstring_append(category_str, " ");
   }
   lFreeList(cmdl);
       
   DEXIT;
   return sge_dstring_get_string(category_str);

ERROR:
   ERROR((SGE_EVENT, MSG_CATEGORY_BUILDINGCATEGORYFORJOBXFAILED_U,  
         u32c(lGetUlong(job, JB_job_number))));
   lFreeList(cmdl);
   return NULL;
}
