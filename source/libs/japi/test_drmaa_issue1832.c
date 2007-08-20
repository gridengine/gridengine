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
 *   The Initial Developer of the Original Code is: Rising Sun Pictures
 * 
 *   Copyright: 2001 by Rising Sun Pictures
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drmaa.h"

/*
 * This test validates the required DRMAA functionality for the -hold_jid_ad option.
 */

#define BULK_SIZE 5

static char errorbuf[DRMAA_ERROR_STRING_BUFFER];

static
drmaa_job_template_t *create_job_template(const char *job_path, const char *job_name, const char *pred_name);

void
validate_jobs(drmaa_job_ids_t *jobids)
{
   int j, status, drmaa_errno;
   char jobid[512];

   for(j = 0; j < BULK_SIZE; j++) {
      drmaa_errno = drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
      if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_get_next_job_id failed: %s\n", errorbuf);
         exit(1);
      }
      /* drmaa_job_ps(3) to return either DRMAA_PS_SYSTEM_ON_HOLD or DRMAA_PS_USER_SYSTEM_ON_HOLD for 
       * array tasks that are in hold due to -hold_jid_ad wc_job_list */
      drmaa_errno = drmaa_job_ps(jobid, &status, errorbuf, sizeof(errorbuf)-1);
      if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_job_ps failed: %s\n", errorbuf);
         exit(1);
      }
      if (status != DRMAA_PS_SYSTEM_ON_HOLD &&
          status != DRMAA_PS_USER_SYSTEM_ON_HOLD) {
         fprintf(stderr, "drmaa_job_ps failed: didn't return DRMAA_PS_SYSTEM_ON_HOLD or DRMAA_PS_USER_SYSTEM_ON_HOLD\n");
         exit(1);
      } else {
         printf("Job `%s' is held (okay)\n", jobid);
      }
   }
}

int main(int argc, char **argv)
{
   drmaa_job_template_t *jt_a = NULL;
   drmaa_job_template_t *jt_b = NULL;
   drmaa_job_ids_t *jobids_a = NULL;
   drmaa_job_ids_t *jobids_b = NULL;
   int drmaa_errno = 0;

   if (argc != 2) {
      printf("Usage: %s path_to_sleeper_script\n", argv[0]);
      exit(1);
   }

   if (strstr(argv[1], "sleeper.sh") == NULL) {
      printf("Usage: %s path_to_sleeper_script\n", argv[0]);
      exit(1);
   }

   if (drmaa_init(NULL, errorbuf, sizeof(errorbuf)-1) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_init failed: %s\n", errorbuf);
      exit(EXIT_FAILURE);
   }

   jt_a = create_job_template(argv[1], "JobA", NULL);
   jt_b = create_job_template(argv[1], "JobB", "JobA");
   
   drmaa_errno = drmaa_run_bulk_jobs(&jobids_a, jt_a, 1, BULK_SIZE, 1, errorbuf, sizeof(errorbuf)-1);
   if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_run_bulk_jobs failed: %s\n", errorbuf);
      return 1;
   }

   drmaa_errno = drmaa_run_bulk_jobs(&jobids_b, jt_b, 1, BULK_SIZE, 1, errorbuf, sizeof(errorbuf)-1);
   if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_run_bulk_jobs failed: %s\n", errorbuf);
      return 1;
   }
  
   /* sleeper will sleep 60s by default, so this test should be okay */
   validate_jobs(jobids_b);

   drmaa_release_job_ids(jobids_a);
   drmaa_release_job_ids(jobids_b);
   
   drmaa_errno = drmaa_delete_job_template(jt_a, errorbuf, sizeof(errorbuf)-1);
   if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_delete_job_template failed: %s\n", errorbuf);
      return 1;
   }

   drmaa_errno = drmaa_delete_job_template(jt_b, errorbuf, sizeof(errorbuf)-1);
   if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_delete_job_template failed: %s\n", errorbuf);
      return 1;
   }

   drmaa_errno = drmaa_exit(errorbuf, sizeof(errorbuf)-1);
   if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_exit failed: %s\n", errorbuf);
      return 1;
   }
    
   printf ("OK\n");

   return 0;
}

static 
drmaa_job_template_t *create_job_template(const char *job_path, const char *job_name, const char *pred_name) 
{
   drmaa_job_template_t *jt = NULL;
   int drmaa_errno;
   char buf[512];

   if (drmaa_allocate_job_template(&jt, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "error: failed to create job template %s.\n", job_name);
      exit(1);
   }

   /* the job to be run */
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, job_path, NULL, 0);

   /* join output/error file */
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   /* path for output */
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null", NULL, 0);

   /* job name for hold_jid_ad list */
   drmaa_set_attribute(jt, DRMAA_JOB_NAME, job_name, NULL, 0);

   /* drmaa_run_bulk_job(3) must accept -hold_jid_ad wc_job_list when passed through job  
      template attribute drmaa_native_specification */
   if (pred_name != NULL) {
      if (strlen(pred_name) > 256) {
         fprintf(stderr, "error: predecessor name too long!\n");
         exit(1);
      }
      strcpy(buf, "-hold_jid_ad ");
      strcat(buf, pred_name);
      drmaa_errno = drmaa_set_attribute(jt, DRMAA_NATIVE_SPECIFICATION, buf, errorbuf, sizeof(errorbuf)-1);
      if(drmaa_errno != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_set_attribute failed: %s\n", errorbuf);
         exit(1);
      }
   }
   return jt;
}
