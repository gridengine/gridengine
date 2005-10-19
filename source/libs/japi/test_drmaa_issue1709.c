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
#include <unistd.h>
#include <string.h>

#include "drmaa.h"

static drmaa_job_template_t *create_job_template(const char *job_path);

int main(int argc, char *argv[])
{
   char diagnosis[DRMAA_ERROR_STRING_BUFFER + 1];
   const char *all_jobids[2];
   char jobid[DRMAA_JOBNAME_BUFFER + 1];
   int drmaa_errno = DRMAA_ERRNO_SUCCESS;
   const char *job_path = NULL;
   drmaa_job_template_t *jt = NULL;
   int new_id = 0;
   int exit_code = 0;
   
   job_path = argv[1];

   if (drmaa_init("", diagnosis, DRMAA_ERROR_STRING_BUFFER) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
      return 1;
   }

   /* Submit a job to find out what the current job id is. */
   jt = create_job_template(job_path);
   
   if (jt == NULL) {
      fprintf(stderr, "create_job_template() failed\n");
      exit_code = 1;
      goto error;
   }
   
   drmaa_errno = drmaa_run_job(jobid, DRMAA_JOBNAME_BUFFER, jt, diagnosis,
                               DRMAA_ERROR_STRING_BUFFER);
   
   if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_run_job() failed: %s %s\n", diagnosis,
              drmaa_strerror(drmaa_errno));
      exit_code = 1;
      goto error;
   }
   
   drmaa_delete_job_template(jt, diagnosis, DRMAA_ERROR_STRING_BUFFER);

   /* Convert job id into a number and add 1. */
   new_id = strtol(jobid, NULL, 10) + 1;
   printf ("Last job id is %s.  Using %d.\n", jobid, new_id);
   
   /* Build job id list. */
   sprintf(jobid, "%d", new_id);
   all_jobids[0] = jobid;
   all_jobids[1] = NULL;
   
   /* Synchronize on the new job id. */
   drmaa_errno = drmaa_synchronize(all_jobids, DRMAA_TIMEOUT_WAIT_FOREVER, 0,
                                   diagnosis, DRMAA_ERROR_STRING_BUFFER);
   
   if (drmaa_errno == DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "Synchronize on non-existant job id succeeded.  Issue 1709 still exists\n");
      exit_code = 1;
      goto error;
   }
   
error:
   if (drmaa_exit(diagnosis, DRMAA_ERROR_STRING_BUFFER) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
      exit_code = 1;
   }
   
   return exit_code;
}


static drmaa_job_template_t *create_job_template(const char *job_path)
{
   drmaa_job_template_t *jt = NULL;

   if (drmaa_allocate_job_template(&jt, NULL, 0) != DRMAA_ERRNO_SUCCESS)
      return NULL;

   /* the job to be run */
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, job_path, NULL, 0);

   /* join output/error file */
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   /* path for output */
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null", NULL, 0);

   return jt;
}
