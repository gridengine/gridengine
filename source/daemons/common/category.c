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
#include "schedd_monitor.h"
#include "unparse_job_cull.h"
#include "sge_dstring.h"
#include "parse_qsubL.h"
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
lList *acl_list,
bool is_resource_cat
) {
   lList *cmdl = NULL;
   lListElem *ep;
   const char *owner, *group;
   bool first;

   DENTER(TOP_LAYER, "sge_build_job_category");
   
   /*
   ** owner -> acl
   */
   owner = lGetString(job, JB_owner);
   group = lGetString(job, JB_group);
   if (sge_unparse_acl(owner, group, "-U", acl_list, &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** -hard -q qlist
   */
   if (sge_unparse_id_list(job, JB_hard_queue_list, "-q",  
                                    &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** -masterq qlist
   */
   if (sge_unparse_id_list(job, JB_master_hard_queue_list, "-masterq",  
                                    &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** -hard -l rlist
   */
   if (sge_unparse_resource_list(job, JB_hard_resource_list, 
                                    &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** -pe pe_name pe_range
   */
   if (sge_unparse_pe(job, &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** -ckpt ckpt_name 
   */
   if (sge_unparse_string_option(job, JB_checkpoint_name, "-ckpt", 
            &cmdl, NULL) != 0) {
      goto ERROR;
   }

   /*
   ** interactive jobs
   */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) {
      ep = sge_add_arg(&cmdl, 0, lIntT, "-I", "y");
      if (ep == NULL) {
         goto ERROR;
      }
      lSetInt(ep, SPA_argval_lIntT, true);
   }
      
   /*
   ** project
   */
   if (sge_unparse_string_option(job, JB_project, "-P", 
            &cmdl, NULL) != 0) {
      goto ERROR;
   }

/* new extension */
   /* only needed, if jobs should be filtered by categories. */ 
   if (is_resource_cat && sconf_is_job_category_filtering()) {
      /* 
       *  deadline
       */
      if (sge_unparse_ulong_option(job, JB_deadline, "-dl", &cmdl, NULL) != 0) {
         goto ERROR;
      }
     
      /*
       * priority
       */
      if (sge_unparse_ulong_option(job, JB_priority, "-p", &cmdl, NULL) != 0) {
         goto ERROR;
      }
    
      if (sge_unparse_ulong_option(job, JB_override_tickets, "-ot", &cmdl, NULL) != 0) {
         goto ERROR;
      }

      if (sge_unparse_ulong_option(job, JB_jobshare, "-js", &cmdl, NULL) != 0) {
         goto ERROR;
      }

      if (sge_unparse_string_option(job, JB_owner, "-u", &cmdl, NULL) != 0) {
         goto ERROR;
      }
    
   }
   /*
   ** create the category string
   */
   lDelElemStr(&cmdl, SPA_switch, "-hard");
   first = true;
   for_each (ep, cmdl) {
      const char *args = lGetString(ep, SPA_switch_arg);

      /* delimiter between multiple requests */
      if (!first) {
         sge_dstring_append(category_str, " ");
      } else {
         first = false;
      }
      
      /* append switch, e.g. -l, -pe */
      sge_dstring_append(category_str, lGetString(ep, SPA_switch));

      /* if switch has arguments, append them, e.g. -l arch=solaris */
      if (args != NULL) {
         sge_dstring_sprintf_append(category_str, " %s", args);
      }
   }

   lFreeList(cmdl);
       
   DEXIT;
   return sge_dstring_get_string(category_str);

ERROR:
   ERROR((SGE_EVENT, MSG_CATEGORY_BUILDINGCATEGORYFORJOBXFAILED_U,  
         u32c(lGetUlong(job, JB_job_number))));
   lFreeList(cmdl);
   DEXIT;
   return NULL;
}
