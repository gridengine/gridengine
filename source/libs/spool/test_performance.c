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
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "sge_unistd.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "sig_handlers.h"
#include "commlib.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"


#include "sge_answer.h"
#include "sge_profiling.h"
#include "sge_host.h"
#include "sge_calendar.h"
#include "sge_ckpt.h"
#include "sge_conf.h"
#include "sge_job.h"
#include "sge_manop.h"
#include "sge_sharetree.h"
#include "sge_pe.h"
#include "sge_schedd_conf.h"
#include "sge_userprj.h"
#include "sge_userset.h"

#include "sge_hgroup.h"


#include "msg_clients_common.h"

#include "sge_mirror.h"
#include "spool/sge_spooling.h"
#include "spool/loader/sge_spooling_loader.h"
#include "sge_event.h"


#ifndef TEST_READ_ONLY
static const char *random_string(int length)
{
   static char buf[1000];
   int i;

   srand(time(0));

   for (i = 0; i < length; i++) {
      buf[i] = rand() % 26 + 64;
   }
   buf[i] = 0;

   return buf;
}

static bool generate_jobs(int num)
{
   int i;

   for (i = 0; i < num; i++) {
      lListElem *job;

      job = lCreateElem(JB_Type);
      lSetUlong(job, JB_job_number, i + 1);
      lSetString(job, JB_job_name, random_string(15));
      lSetString(job, JB_project, random_string(20));
      lSetString(job, JB_department, random_string(20));
      lSetString(job, JB_directive_prefix, random_string(100));
      lSetString(job, JB_exec_file, random_string(500));
      lSetString(job, JB_script_file, random_string(500));
      lSetString(job, JB_owner, random_string(10));
      lSetString(job, JB_group, random_string(10));
      lSetString(job, JB_account, random_string(20));
      lSetString(job, JB_cwd, random_string(100));
      lAppendElem(*object_type_get_master_list(SGE_TYPE_JOB), job);

      if ((i % 10) == 0) {
         lAddSubUlong(job, JAT_task_number, 1, JB_ja_tasks, JAT_Type);
      }
   }

   return true;
}

static bool spool_data(void)
{
   lList *answer_list = NULL;
   lListElem *context, *job;
   char key[100];

   context = spool_get_default_context();

   fprintf(stdout, "spooling %d jobs\n", lGetNumberOfElem(*object_type_get_master_list(SGE_TYPE_JOB)));

   for_each(job, *object_type_get_master_list(SGE_TYPE_JOB)) {
      sprintf(key, sge_U32CFormat".0", sge_u32c(lGetUlong(job, JB_job_number)));
      spool_write_object(&answer_list, context, job, key, SGE_TYPE_JOB, true);
      answer_list_output(&answer_list);
   }

   return true;
}
#endif
static bool read_spooled_data(void)
{  
   lList *answer_list = NULL;
   lListElem *context;

   context = spool_get_default_context();

   /* jobs */
   spool_read_list(&answer_list, context, object_type_get_master_list(SGE_TYPE_JOB), SGE_TYPE_JOB);
   answer_list_output(&answer_list);
/*    DPRINTF(("read %d entries to Master_Job_List\n", lGetNumberOfElem(*object_type_get_master_list(SGE_TYPE_JOB)))); */

   return true;
}

static bool delete_spooled_data(void)
{  
   lList *answer_list = NULL;
   lListElem *job;
   lListElem *context;
   char key[100];

   context = spool_get_default_context();

   /* jobs */
   for_each(job, *object_type_get_master_list(SGE_TYPE_JOB)) {
      sprintf(key, sge_U32CFormat".0", sge_u32c(lGetUlong(job, JB_job_number)));
      spool_delete_object(&answer_list, context, SGE_TYPE_JOB, key, true);
      answer_list_output(&answer_list);
   }

   return true;
}

int main(int argc, char *argv[])
{
   sge_gdi_ctx_class_t *ctx = NULL;
   lListElem *spooling_context;
   lList *answer_list = NULL;
   object_description *object_base;

   DENTER_MAIN(TOP_LAYER, "test_performance");

#define NM10 "%I%I%I%I%I%I%I%I%I%I"
#define NM5  "%I%I%I%I%I"
#define NM2  "%I%I"
#define NM1  "%I"

   prof_mt_init();
   obj_mt_init();
   bootstrap_mt_init();

   prof_start(SGE_PROF_CUSTOM1, NULL);
   prof_set_level_name(SGE_PROF_CUSTOM1, "performance", NULL);

   prof_start(SGE_PROF_SPOOLINGIO, NULL);
   prof_set_level_name(SGE_PROF_SPOOLINGIO, "io", NULL);

   /* parse commandline parameters */
   if(argc != 4) {
      ERROR((SGE_EVENT, "usage: test_sge_spooling <method> <shared lib> <arguments>\n"));
      SGE_EXIT((void**)&ctx, 1);
   }

   object_base = object_type_get_object_description();
   *(object_base[SGE_TYPE_JOB].list) = lCreateList("job list", JB_Type);

#define defstring(str) #str

   /* initialize spooling */
   spooling_context = spool_create_dynamic_context(&answer_list, argv[1], argv[2], argv[3]); 
   answer_list_output(&answer_list);
   if (spooling_context == NULL) {
      SGE_EXIT((void**)&ctx, EXIT_FAILURE);
   }

   spool_set_default_context(spooling_context);

   if (!spool_startup_context(&answer_list, spooling_context, true)) {
      answer_list_output(&answer_list);
      SGE_EXIT((void**)&ctx, EXIT_FAILURE);
   }
   answer_list_output(&answer_list);
  
#ifndef TEST_READ_ONLY
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   generate_jobs(30000);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "generating jobs:\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);
/*
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = copy_jobs();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   lFreeList(&copy);
   prof_output_info(SGE_PROF_CUSTOM1, true, "copy jobs:\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = select_jobs(what_job);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   lFreeList(&copy);
   prof_output_info(SGE_PROF_CUSTOM1, true, "select jobs:\n");
   prof_reset(SGE_PROF_CUSTOM1, SGE_PROF_CUSTOM1, NULL);
*/
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   spool_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "spool jobs:\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);
   prof_output_info(SGE_PROF_SPOOLINGIO, true, "IO:\n");
   prof_reset(SGE_PROF_SPOOLINGIO, NULL);

   lFreeList(object_base[SGE_TYPE_JOB].list);
   *(object_base[SGE_TYPE_JOB].list) = lCreateList("job list", JB_Type);
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   read_spooled_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "read jobs (cached):\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   delete_spooled_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "delete jobs:\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);
  
   spool_shutdown_context(&answer_list, spooling_context);
   spool_startup_context(&answer_list, spooling_context, true);
#else
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   read_spooled_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\nread jobs (uncached):\n");
   prof_reset(SGE_PROF_CUSTOM1, NULL);
#endif

   lFreeList(object_base[SGE_TYPE_JOB].list);

   spool_shutdown_context(&answer_list, spooling_context);
   answer_list_output(&answer_list);

   DRETURN(EXIT_SUCCESS);
}
