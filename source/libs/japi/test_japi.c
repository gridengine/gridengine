#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>

#include "japi.h"

/* just for debugging purposes */
#include "sgermon.h"

#define JOB_CHUNK 20
enum {
   ST_SUBMIT_WAIT = 0, 
   MT_SUBMIT_WAIT,
   MT_SUBMIT_BEFORE_INIT_WAIT
};

static int test_case = MT_SUBMIT_BEFORE_INIT_WAIT;

static void *submit_sleeper(void *vp)
{
   int i;
   char jobid[100];
   char *job_argv[2];
   job_template_t *jt = NULL;

   drmaa_allocate_job_template(&jt);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, "/cod_home/ah114088/SGE53/examples/jobs/sleeper.sh");
   job_argv[0] = "10"; 
   job_argv[1] = NULL;
   drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv);

   for (i=0; i<JOB_CHUNK; i++) {
      /* submit job */
      if (test_case == MT_SUBMIT_BEFORE_INIT_WAIT) {
         while (drmaa_run_job(jobid, sizeof(jobid)-1, jt)!=DRMAA_ERRNO_SUCCESS) {
            printf("failed submitting job - retry\n");
         }
         printf("submitted job \"%s\"\n", jobid);
      } else {
         if (drmaa_run_job(jobid, sizeof(jobid)-1, jt)!=DRMAA_ERRNO_SUCCESS) {
            printf("failed submitting job\n");
         } else {
            printf("submitted job \"%s\"\n", jobid);
         }
      }
   }

   drmaa_delete_job_template(jt);

   return NULL;
}
int main(int argc, char *argv[])
{
   char jobid[100];
   int i, stat;
   int drmaa_errno;

   DENTER_MAIN(TOP_LAYER, "test_japi");

   
   if (test_case != MT_SUBMIT_BEFORE_INIT_WAIT) {
      if (drmaa_init(NULL) != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_init() failed\n");
         DEXIT;
         return 1;
      }
   }

   if (test_case == ST_SUBMIT_WAIT) {
      for (i=0; i<3; i++) 
         submit_sleeper(NULL);
   } else { /* MT_SUBMIT_WAIT */
      pthread_t submitter_threads[3];
      for (i=0; i<3; i++) {
         pthread_create(&submitter_threads[i], NULL, submit_sleeper, NULL);
      }

      if (test_case == MT_SUBMIT_BEFORE_INIT_WAIT) {
         /* delay drmaa_init() */
         sleep(10);
         printf("before drmaa_init()\n"); 
         if (drmaa_init(NULL) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n"); 
            DEXIT;
            return 1;
         }
      }
      for (i=0; i<3; i++)
         pthread_join(submitter_threads[i], NULL);
   }

   /* now wait all jobs */
   do {
      drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL);
      if (drmaa_errno == DRMAA_ERRNO_SUCCESS)
         printf("waited job \"%s\"\n", jobid);
   } while (drmaa_errno != DRMAA_ERRNO_INVALID_JOB);

   drmaa_exit();

   DEXIT;
   return 0;
}
      
