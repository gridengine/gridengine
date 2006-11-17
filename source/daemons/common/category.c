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
#include "lck/sge_mtutil.h"
#include "sge_userprjL.h"
#include "sge_resource_quota.h"

#include "msg_daemons_common.h"

/* struct containing the cull field position of the job target structures
   and the reduced order elements */
typedef struct {
   int JB_hard_queue_list_pos;
   int JB_master_hard_queue_list_pos;
   int JB_hard_resource_list_pos;
   int JB_soft_resource_list_pos;
   int JB_checkpoint_name_pos;
   int JB_type_pos;
   int JB_owner_pos;
   int JB_group_pos;
   int JB_project_pos;
   int JB_pe_pos;
   int JB_range_pos;
} order_pos_t;

typedef struct {
   pthread_mutex_t cull_order_mutex; /* gards the last_update access */
   order_pos_t cull_order_pos;        /* stores cull positions in the job, ja-task, and order structure */
} sge_category_t;

static sge_category_t Category_Control = {PTHREAD_MUTEX_INITIALIZER, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

/*-------------------------------------------------------------------------*/
/* build the category string                                               */
/* the category string includes now the soft requests                      */
/*-------------------------------------------------------------------------*/
/****** category/sge_build_job_category_dstring() ******************************
*  NAME
*     sge_build_job_category_dstring() -- build the category string   
*
*  SYNOPSIS
*     void sge_build_job_category_dstring(dstring *category_str, lListElem 
*     *job, lList *acl_list) 
*
*  FUNCTION
*     The following parameter are put into the category:
*        hard_queue_list
*        master_hard_queue_list
*        hard_resource_list
*        soft_resource_list
*        checkpoint_name
*        type
*
*        owner/group: -U user_lists 
*           Omitted, if user_lists/xuser_lists were not used in
*           host_conf(5), sge_pe(5) and queue_conf(5). In sge_conf(5) 
*           user_lists/xuser_lists still can be used, as it causes
*           jobs already be rejected at submit time.
*
*        project: -P user_lists 
*           Omitted, if projects/xprojects were not used in
*           host_conf(5), sge_pe(5) and queue_conf(5). In sge_conf(5) 
*           projects/xprojects still can be used, as it cuases
*           jobs already be rejected at submit time.
*
*        pe
*
*  INPUTS
*     dstring *category_str - target string, contains the category or nothing
*     lListElem *job        - the job for the category creating
*     lList *acl_list       - global access list
*
*  NOTES
*     MT-NOTE: sge_build_job_category_dstring() is MT safe as long as the caller is
*
*******************************************************************************/
void sge_build_job_category_dstring(dstring *category_str, lListElem *job, lList *acl_list, const lList *prj_list, bool *did_project, const lList *rqs_list) 
{

   const char *owner = NULL;
   const char *group = NULL;

   DENTER(TOP_LAYER, "sge_build_job_category_dstring");

   sge_mutex_lock("cull_order_mutex", SGE_FUNC, __LINE__, &Category_Control.cull_order_mutex);

   if (Category_Control.cull_order_pos.JB_hard_queue_list_pos == -1) {
      Category_Control.cull_order_pos.JB_hard_queue_list_pos = lGetPosViaElem(job, JB_hard_queue_list, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_master_hard_queue_list_pos = lGetPosViaElem(job, JB_master_hard_queue_list, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_hard_resource_list_pos = lGetPosViaElem(job, JB_hard_resource_list, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_soft_resource_list_pos = lGetPosViaElem(job, JB_soft_resource_list, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_checkpoint_name_pos = lGetPosViaElem(job, JB_checkpoint_name, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_type_pos = lGetPosViaElem(job, JB_type, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_owner_pos = lGetPosViaElem(job, JB_owner, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_group_pos = lGetPosViaElem(job, JB_group, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_project_pos = lGetPosViaElem(job, JB_project, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_pe_pos = lGetPosViaElem(job, JB_pe, SGE_NO_ABORT);
      Category_Control.cull_order_pos.JB_range_pos = lGetPosViaElem(job, JB_pe_range, SGE_NO_ABORT);
   }
   sge_mutex_unlock("cull_order_mutex", SGE_FUNC, __LINE__, &Category_Control.cull_order_mutex);

   /*
   ** owner -> acl
   */
   owner = lGetPosString(job, Category_Control.cull_order_pos.JB_owner_pos);
   group = lGetPosString(job, Category_Control.cull_order_pos.JB_group_pos);
   sge_unparse_acl_dstring(category_str, owner, group, acl_list, "-U");
 
   /* 
   ** -u if referenced in resource quota sets
   */
   if (sge_user_is_referenced_in_rqs(rqs_list, lGetString(job, JB_owner), acl_list)) {
      if (sge_dstring_strlen(category_str) > 0) {
         sge_dstring_append(category_str, " ");
      }
      sge_dstring_append(category_str, "-u ");
      sge_dstring_append(category_str, lGetString(job, JB_owner));
   }

   /*
   ** -hard -q qlist
   */
   sge_unparse_queue_list_dstring(category_str, job, Category_Control.cull_order_pos.JB_hard_queue_list_pos, "-q");  

   /*
   ** -masterq qlist
   */
   sge_unparse_queue_list_dstring(category_str, job, Category_Control.cull_order_pos.JB_master_hard_queue_list_pos, "-masterq");

   /*
   ** -l rlist (hard resource list)
   */
   sge_unparse_resource_list_dstring(category_str, job, Category_Control.cull_order_pos.JB_hard_resource_list_pos, "-l");
   
   /*
   ** -soft -l rlist
   */
   sge_unparse_resource_list_dstring(category_str, job, Category_Control.cull_order_pos.JB_soft_resource_list_pos, "-soft -l");

   /*
   ** -pe pe_name pe_range
   */
   sge_unparse_pe_dstring(category_str, job, Category_Control.cull_order_pos.JB_pe_pos, 
                          Category_Control.cull_order_pos.JB_range_pos, "-pe");
   /*
   ** -ckpt ckpt_name 
   */
   sge_unparse_string_option_dstring(category_str, job, Category_Control.cull_order_pos.JB_checkpoint_name_pos, "-ckpt");

   /*
   ** interactive jobs
   */
   if (JOB_TYPE_IS_IMMEDIATE(lGetPosUlong(job, Category_Control.cull_order_pos.JB_type_pos))) {
      if (sge_dstring_strlen(category_str) > 0) {
         sge_dstring_append(category_str, " -I y");
      }
      else {
         sge_dstring_append(category_str, "-I y");
      }
   }
      
   /*
   ** project
   */
   {
      const char *project = lGetPosString(job, Category_Control.cull_order_pos.JB_project_pos);

      const lListElem *prj;
      if (project && (prj=lGetElemStr(prj_list, UP_name, project)) && lGetBool(prj, UP_consider_with_categories)) {
         if (did_project)
            *did_project = true;
         sge_unparse_string_option_dstring(category_str, job, Category_Control.cull_order_pos.JB_project_pos, "-P");
      } else
         if (did_project)
            *did_project = false;
   }

   DRETURN_VOID;
}

/****** category/sge_build_job_cs_category() ***********************************
*  NAME
*     sge_build_job_cs_category() -- generates the category string for the sc
*
*  SYNOPSIS
*     const char* sge_build_job_cs_category(dstring *category_str, lListElem 
*     *job, lListElem *cat_obj) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     dstring *category_str - storage forthe c string
*     lListElem *job        - job for which to create a category
*     lListElem *cat_obj    - regular category for the job.
*
*  RESULT
*     const char* - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: sge_build_job_cs_category() is MT safe 
*
*******************************************************************************/
const char* 
sge_build_job_cs_category(dstring *category_str, lListElem *job, lListElem *cat_obj, bool did_project) 
{
   const char *p;

   DENTER(TOP_LAYER, "sge_build_job_cs_category");

   /* 
    *  deadline
    */
   sge_dstring_sprintf_append(category_str, "-dl "sge_u32, lGetUlong(job, JB_deadline));
   /*
    * priority
    */
   sge_dstring_sprintf_append(category_str, "-p "sge_u32, lGetUlong(job, JB_priority));
   sge_dstring_sprintf_append(category_str, "-ot "sge_u32, lGetUlong(job, JB_override_tickets));
   sge_dstring_sprintf_append(category_str, "-js "sge_u32, lGetUlong(job, JB_jobshare));
   sge_dstring_sprintf_append(category_str, "-u %s", lGetString(job, JB_owner));

   /*
    * ticket assignment can depend on a jobs project.
    * If the project was not added to the resource categories
    * it gets added directly to the cs category string 
    */
   if ((p=lGetString(job, JB_project)) && !did_project)
      sge_dstring_sprintf_append(category_str, " -P %s", p);

   /*
    * id for the resource category
    * I use teh address of the resource category as a hash value for its string.
    */
   sge_dstring_sprintf_append(category_str, " %x", cat_obj);

   DEXIT;
   return sge_dstring_get_string(category_str);
}
