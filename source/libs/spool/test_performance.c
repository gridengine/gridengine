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

#include "setup.h"

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

   if (Master_Job_List == NULL) {
      Master_Job_List = lCreateList("job list", JB_Type);
   }

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
      lAppendElem(Master_Job_List, job);

      if ((i % 10) == 0) {
         lAddSubUlong(job, JAT_task_number, 1, JB_ja_tasks, JAT_Type);
      }
   }

   return true;
}

static lList *
copy_jobs()
{
   lList *copy;
   
   copy = lCopyList("copy", Master_Job_List);
   return copy;
}

static lList *
select_jobs(lEnumeration *what_job)
{
   lList *copy;
   copy = lSelect("copy", Master_Job_List, NULL, what_job);
   return copy;
}

static bool spool_data()
{
   lList *answer_list = NULL;
   lListElem *context, *job;
   char key[100];

   context = spool_get_default_context();

   fprintf(stdout, "spooling %d jobs\n", lGetNumberOfElem(Master_Job_List));

   for_each(job, Master_Job_List) {
      sprintf(key, "%ld.0", lGetUlong(job, JB_job_number));
      spool_write_object(&answer_list, context, job, key, SGE_TYPE_JOB);
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
   spool_read_list(&answer_list, context, &Master_Job_List, SGE_TYPE_JOB);
   answer_list_output(&answer_list);
   DPRINTF(("read %d entries to Master_Job_List\n", lGetNumberOfElem(Master_Job_List)));

   return true;
}

int main(int argc, char *argv[])
{
   int cl_err = 0;
   lListElem *spooling_context;
   lList *answer_list = NULL;
   lEnumeration *what_job;
#ifndef TEST_READ_ONLY
   lList *copy;
#endif

   DENTER_MAIN(TOP_LAYER, "test_sge_mirror");

   
#define NM10 "%I%I%I%I%I%I%I%I%I%I"
#define NM5  "%I%I%I%I%I"
#define NM2  "%I%I"
#define NM1  "%I"

   what_job = lWhat("%T(" NM10 NM10 NM10 NM10 NM2")", JB_Type,
         JB_job_number, 
         JB_script_file,
         JB_submission_time,
         JB_owner,
         JB_uid,      /* x*/
         JB_group,
         JB_gid,        /* x*/
         JB_nrunning,
         JB_execution_time,
         JB_checkpoint_attr,     /* x*/

         JB_checkpoint_interval, /* x*/
         JB_checkpoint_name,   
         JB_hard_resource_list,
         JB_soft_resource_list,
         JB_mail_options, /* may be we want to send mail */ /* x*/
         JB_mail_list,  /* x*/
         JB_job_name,   /* x*/
         JB_priority,
         JB_hard_queue_list,
         JB_soft_queue_list,

         JB_master_hard_queue_list,
         JB_pe,
         JB_pe_range,
         JB_jid_predecessor_list,
         JB_soft_wallclock_gmt,
         JB_hard_wallclock_gmt,
         JB_version,
         JB_type,
         JB_project,
/* SGE */ JB_department,

         JB_jobclass, /*x*/
         JB_deadline,
         JB_host,
         JB_override_tickets,
         JB_ja_structure,
         JB_ja_n_h_ids,
         JB_ja_u_h_ids,
         JB_ja_s_h_ids,
         JB_ja_o_h_ids,   
         JB_ja_tasks,

         JB_ja_template,
         JB_category);

   prof_start(SGE_PROF_CUSTOM1, NULL);
   prof_set_level_name(SGE_PROF_CUSTOM1, "performance", NULL);

   /* parse commandline parameters */
   if(argc != 4) {
      ERROR((SGE_EVENT, "usage: test_sge_spooling <method> <shared lib> <arguments>\n"));
      SGE_EXIT(1);
   }

   sge_gdi_param(SET_MEWHO, QEVENT, NULL);
   if ((cl_err = sge_gdi_setup(prognames[QEVENT], NULL))) {
      ERROR((SGE_EVENT, "sge_gdi_setup failed: %s\n", cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QEVENT);

   if (reresolve_me_qualified_hostname() != CL_OK) {
      SGE_EXIT(1);
   }   

#define defstring(str) #str

   /* initialize spooling */
   spooling_context = spool_create_dynamic_context(&answer_list, argv[1], argv[2], argv[3]); 
   answer_list_output(&answer_list);
   if(spooling_context == NULL) {
      SGE_EXIT(EXIT_FAILURE);
   }

   spool_set_default_context(spooling_context);

   if(!spool_startup_context(&answer_list, spooling_context, true)) {
      answer_list_output(&answer_list);
      SGE_EXIT(EXIT_FAILURE);
   }
   answer_list_output(&answer_list);
   
#ifndef TEST_READ_ONLY
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   generate_jobs(1000);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\ngenerating jobs:\n");
   prof_reset(NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = copy_jobs();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = lFreeList(copy);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\ncopy jobs:\n");
   prof_reset(NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = select_jobs(what_job);
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   copy = lFreeList(copy);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\nselect jobs:\n");
   prof_reset(NULL);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   spool_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\nspool jobs:\n");
   prof_reset(NULL);

   Master_Job_List = lFreeList(Master_Job_List);

   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   read_spooled_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\nread jobs (cached):\n");
   prof_reset(NULL);
  
   spool_shutdown_context(&answer_list, spooling_context);
   spool_startup_context(&answer_list, spooling_context, true);
  
   Master_Job_List = lFreeList(Master_Job_List);
#else
   PROF_START_MEASUREMENT(SGE_PROF_CUSTOM1);
   read_spooled_data();
   PROF_STOP_MEASUREMENT(SGE_PROF_CUSTOM1);
   prof_output_info(SGE_PROF_CUSTOM1, true, "\nread jobs (uncached):\n");
   prof_reset(NULL);

   Master_Job_List = lFreeList(Master_Job_List);
#endif

   spool_shutdown_context(&answer_list, spooling_context);
   answer_list_output(&answer_list);

   DEXIT;
   return EXIT_SUCCESS;
}
