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

#include "def.h"
#include "sge_gdi_intern.h"
#include "sge_c_event.h"
#include "sge_ckptL.h"
#include "sge_complexL.h"
#include "sge_eventL.h"
#include "sge_hostL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_job_schedd.h"
#include "sge_log.h"
#include "sge_peL.h"
#include "sge_schedd.h"
#include "sge_process_events.h"
#include "sge_prognames.h"
#include "sge_queueL.h"
#include "sge_ctL.h"
#include "sge_schedconfL.h"
#include "sge_usersetL.h"
#include "sge_userprjL.h"
#include "sge_time.h"
#include "sgermon.h"
#include "commlib.h"
#include "cull_sort.h"
#include "event.h"
#include "schedd_conf.h"
#include "schedd_monitor.h"
#include "unparse_job_cull.h"
#include "sge_string_append.h"
#include "parse_qsubL.h"
#include "sge_access_tree.h"
#include "parse.h"
#include "sge_category.h"
#include "msg_schedd.h"
#include "category.h"

#include "jb_now.h"

/* Categories of the job are managed here */
lList *CATEGORY_LIST = NULL;

/*-------------------------------------------------------------------*
 * sge_process_all_events
 * 
 *  returns 
 *    1 no events
 *    0 ok trigger scheduling
 *   -1 event protocol error or no qmaster 
 *-------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*    add job´s category to the ´global´ category list, if it doesn´t      */
/*    already exist, and reference the category in the job element         */
/*    The category_list is recreated for every scheduler run               */
/*-------------------------------------------------------------------------*/
int sge_add_job_category( lListElem *job, lList *acl_list) {

   lListElem *cat = NULL;
   char *cstr;
   u_long32 rc = 0, jobid;
   static char no_requests[] = "no-requests";

   DENTER(TOP_LAYER, "sge_add_job_category");
   
   cstr = sge_build_job_category(job, acl_list);

   if (!cstr) 
      cstr = strdup(no_requests);

   jobid = lGetUlong(job, JB_job_number);
   if (!CATEGORY_LIST)
      CATEGORY_LIST = lCreateList("new category list", CT_Type);
   else {
      cat = lGetElemStr(CATEGORY_LIST, CT_str, cstr);
/*       DPRINTF(("Job "u32": Found %s in category list\n", jobid, cstr)); */
   }

   if (!cat) {
/*       DPRINTF(("Job "u32": Added %s to category list\n", jobid, cstr)); */
      cat = lAddElemStr(&CATEGORY_LIST, CT_str, cstr, CT_Type);
   }   
   /* increment ref counter and set reference to this element */
   rc = lGetUlong(cat, CT_refcount);
   lSetUlong(cat, CT_refcount, ++rc);
   lSetRef(job, JB_category, cat);

   /* 
   ** free cstr
   */
   if (cstr)
      free(cstr);

   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*/
/*    delete job´s category if CT_refcount gets 0                          */
/*-------------------------------------------------------------------------*/
int sge_delete_job_category(
lListElem *job 
) {
   lListElem *cat = NULL;
   u_long32 rc = 0;

   DENTER(TOP_LAYER, "sge_delete_job_category");
   
   cat = (lListElem *)lGetRef(job, JB_category);
   if (CATEGORY_LIST && cat) {
      rc = lGetUlong(cat, CT_refcount);
      if (rc > 1) {
         lSetUlong(cat, CT_refcount, --rc);
      }
      else {
         DPRINTF(("Removing %s from category list (refcount: " u32 ")\n", 
                  lGetString(cat, CT_str), lGetUlong(cat, CT_refcount)));
         lRemoveElem(CATEGORY_LIST, cat);
      }
   }
   lSetRef(job, JB_category, NULL);
   
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*/
int sge_is_job_category_rejected(
lListElem *job 
) {
   int ret;

   DENTER(TOP_LAYER, "sge_is_job_category_rejected");
  
   ret = sge_is_job_category_rejected_(lGetRef(job, JB_category));  

   DEXIT;
   return ret;
}

/*-------------------------------------------------------------------------*/
int sge_is_job_category_rejected_(
lRef cat 
) {
   return lGetUlong(cat,  CT_rejected)?TRUE:FALSE;
}

/*-------------------------------------------------------------------------*/
void sge_reject_category(
lRef cat 
) {
   lSetUlong(cat, CT_rejected, 1);
}

/*-------------------------------------------------------------------------*/
/* rebuild the category references                                         */
/*-------------------------------------------------------------------------*/
int sge_rebuild_job_category( lList *job_list, lList *acl_list) {
   lListElem *job;

   DENTER(TOP_LAYER, "sge_rebuild_job_category");

   CATEGORY_LIST = lFreeList(CATEGORY_LIST);
   for_each (job, job_list) {
      sge_add_job_category(job, acl_list);
   } 
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*/
/* reset the category CT_rejected field to 0                               */
/*-------------------------------------------------------------------------*/
int sge_reset_job_category()
{
   lListElem *cat;
   DENTER(TOP_LAYER, "sge_reset_job_category");

   for_each (cat, CATEGORY_LIST) {
      lSetUlong(cat, CT_rejected, 0);
   } 
   DEXIT;
   return 0;
}

/*-------------------------------------------------------------------------*/
/* free module internal data                                               */
/*-------------------------------------------------------------------------*/
void sge_free_job_category()
{
   CATEGORY_LIST = lFreeList(CATEGORY_LIST);
}
