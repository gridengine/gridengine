#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>

#define DRMAA_HOOKS

#include "drmaa.h"

#define JOB_CHUNK 10
#define NTHREADS 3
#define NBULKS 1

#define MIRROR_JOB  "/usr/bin/cat"
/* #define MIRROR_JOB  "/home/ah114088/mirror.sh" */
#define SLEEPER_JOB  "/cod_home/ah114088/SGE53/examples/jobs/sleeper.sh"

enum {
   ALL_TESTS = 0,    

   ST_SUBMIT_WAIT = 1,    
      /* - one thread 
         - submit jobs 
         - wait for jobend */

   MT_SUBMIT_WAIT = 2,        
      /* - multiple submission threads
         - wait is done by main thread */

   MT_SUBMIT_BEFORE_INIT_WAIT = 3,
      /* - no drmaa_init() was called
         - multiple threads try to submit but fail
         - when drmaa_init() is called by main thread
           submission proceed
         - wait is done by main thread */

   ST_MULT_INIT = 4,
      /* - drmaa_init() is called multiple times 
         - first time it must succeed - second time it must fail
         - then drmaa_exit() is called */

   ST_MULT_EXIT = 5,
      /* - drmaa_init() is called
         - then drmaa_exit() is called multiple times
         - first time it must succeed - second time it must fail */

   MT_EXIT_DURING_SUBMIT = 6,
      /* - drmaa_init() is called
         - multiple submission threads submitting (delayed) a series 
           of jobs
         - during submission main thread does drmaa_exit() */

   MT_SUBMIT_MT_WAIT = 7,
      /* - drmaa_init() is called
         - multiple submission threads submit jobs and wait these jobs
         - when all threads are finished main thread calls drmaa_exit() */

   MT_EXIT_DURING_SUBMIT_OR_WAIT = 8,
      /* - drmaa_init() is called
         - multiple submission threads submit jobs and wait these jobs
         - while submission threads are waiting their jobs the main 
           thread calls drmaa_exit() */

   ST_BULK_SUBMIT_WAIT = 9,
      /* - drmaa_init() is called
         - a bulk job is submitted and waited 
         - then drmaa_exit() is called */

   ST_INPUT_BECOMES_OUTPUT = 10,
      /* - drmaa_init() is called
         - job input is prepared in local file 
         - a job is submitted that echoes it's input to output 
         - the job is waited
         - job output must be identical to job input
         (still requires manual testing)
      */

   ST_SUBMIT_IN_HOLD = 11,
      /* - drmaa_init() is called
         - a job is submitted with a user hold 
         - hold state is released (manually)
         - the job is waited
         (still requires manual testing)
      */

   ST_BULK_SUBMIT_IN_HOLD = 12
      /* - drmaa_init() is called
         - a bulk job is submitted with a user hold 
         - hold state is released (manually)
         - the job ids are waited
         (still requires manual testing)
      */

};

#define NO_TEST ST_BULK_SUBMIT_WAIT+1

static int test(void);
static void *submit_and_wait(void *vp);
static void *submit_sleeper_single(void *vp);
static void *submit_sleeper_bulk(void *vp);
static void *submit_sleeper(int n, int bulk, int in_hold);
static void *submit_input_mirror(int n, int as_bulk_job);
static void *do_submit(drmaa_job_template_t *jt, int n, int as_bulk_job);
static int wait_all_jobs(void *vp);
static int wait_n_jobs(int n);

static int test_case;


int main(int argc, char *argv[])
{
   int failed = 0;
   int i; 

   if (argc == 1) {
      fprintf(stderr, "usage: test_japi <test_case>\n"
                      "       <test_case> is a number: 0-%d\n"
                      "       see src for a detailed test case description\n", NO_TEST-1);
      return 1;
   }

   test_case = atoi(argv[1]);
   
   if (test_case == ALL_TESTS)
      for (i=ST_SUBMIT_WAIT; i<NO_TEST; i++) {
         test_case = i;
         printf("---------------------\n");
         printf("starting test #%d\n", i);
         if (test()!=0) {
            printf("test #%d failed\n", i);
            failed = 1;
         }
         printf("successfully finished test #%d\n", i);
      }
   else {
      printf("starting test #%d\n", test_case);
      if (test()!=0) {
         printf("test #%d failed\n", test_case);
         failed = 1;
      }
      printf("successfully finished test #%d\n", test_case);
   }
   
   return failed;
}

static int test(void)
{
   int i;
   int job_chunk = JOB_CHUNK;

   switch (test_case) {
   case ST_MULT_INIT:
      { 
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_ALREADY_ACTIVE_SESSION) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case ST_MULT_EXIT:
      { 
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit(1) failed\n");
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_NO_ACTIVE_SESSION) {
            fprintf(stderr, "drmaa_exit(2) failed\n");
            return 1;
         }
      }
      break;

   case ST_SUBMIT_WAIT:
      {
         int n = -1;

         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         for (i=0; i<NTHREADS; i++) 
            submit_sleeper_single(&job_chunk);
         if (wait_all_jobs(&n) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case MT_SUBMIT_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 
         int n = -1;

         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL)) 
               printf("pthread_join() returned != 0\n");
         if (wait_all_jobs(&n) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }

         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case MT_SUBMIT_BEFORE_INIT_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 
         int n = -1;

         for (i=0; i<NTHREADS; i++) {
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         }

         /* delay drmaa_init() */
         sleep(5);
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n"); 
            return 1;
         }
         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");

         if (wait_all_jobs(&n) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case MT_EXIT_DURING_SUBMIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         delay_after_submit = 20;

         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         sleep(1);
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
         printf("drmaa_exit() succeeded\n");
         delay_after_submit = 0;

         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");
      }
      break;

   case MT_SUBMIT_MT_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_and_wait, &job_chunk);

         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");

         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case MT_EXIT_DURING_SUBMIT_OR_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_and_wait, &job_chunk);
         sleep(2);
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
         printf("drmaa_exit() succeeded\n");

         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");
      }
      break;

   case ST_BULK_SUBMIT_WAIT:
      {
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) 
            submit_sleeper_bulk(&job_chunk);
         if (wait_n_jobs(job_chunk) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case ST_INPUT_BECOMES_OUTPUT:
      {
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) 
            submit_input_mirror(1, 0);
         if (wait_n_jobs(1) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case ST_SUBMIT_IN_HOLD:
      {
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) 
            submit_sleeper(1, 0, 1);
         if (wait_n_jobs(1) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   case ST_BULK_SUBMIT_IN_HOLD:
      {
         if (drmaa_init(NULL, NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) 
            submit_sleeper(10, 1, 1);
         if (wait_n_jobs(10) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(NULL, 0) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed\n");
            return 1;
         }
      }
      break;

   default:
      break;
   }

   return 0;
}

static void *submit_and_wait(void *vp)
{
   int n;
   if (vp) 
      n = *(int *)vp;
   else 
      n = 1;

   submit_sleeper_single(&n);
   wait_all_jobs(&n);

   return NULL;
}

static void *submit_sleeper_single(void *vp)
{
   int n;
   if (vp) 
      n = *(int *)vp;
   else 
      n = 1;
   return submit_sleeper(n, 0, 0);
}
static void *submit_sleeper_bulk(void *vp)
{
   int n;
   if (vp) 
      n = *(int *)vp;
   else 
      n = 1;
   return submit_sleeper(n, 1, 0);
}

static void *submit_sleeper(int n, int as_bulk_job, int in_hold)
{
   char *job_argv[2];
   drmaa_job_template_t *jt = NULL;
   void *p;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, SLEEPER_JOB, NULL, 0);
   job_argv[0] = "10"; 
   job_argv[1] = NULL;
   drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   if (!as_bulk_job)
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID", NULL, 0);
   else
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID."DRMAA_PLACEHOLDER_INCR, NULL, 0);

   if (in_hold) 
      drmaa_set_attribute(jt, DRMAA_JS_STATE, DRMAA_SUBMISSION_STATE_HOLD, NULL, 0);

   p = do_submit(jt, n, as_bulk_job);
   drmaa_delete_job_template(jt, NULL, 0);

   return p;
}

static void *submit_input_mirror(int n, int as_bulk_job)
{
   drmaa_job_template_t *jt = NULL;
   void *p;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, MIRROR_JOB, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   if (!as_bulk_job) {
      drmaa_set_attribute(jt, DRMAA_INPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_input", NULL, 0);
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_output", NULL, 0);
   } else {
      drmaa_set_attribute(jt, DRMAA_INPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_input."DRMAA_PLACEHOLDER_INCR, NULL, 0);
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_output."DRMAA_PLACEHOLDER_INCR, NULL, 0);
   }

   p = do_submit(jt, n, as_bulk_job);

   drmaa_delete_job_template(jt, NULL, 0);

   return p;
}

static void *do_submit(drmaa_job_template_t *jt, int n, int as_bulk_job)
{
   int i;
   char diagnosis[1024];
   char jobid[100];
   int drmaa_errno;
   int array_size, submissions;

   if (as_bulk_job) {
      array_size = n;
      submissions = 1;
   } else {
      array_size = 1;
      submissions = n;
   }

   for (i=0; i<submissions; i++) {
      /* submit job */
      if (test_case == MT_SUBMIT_BEFORE_INIT_WAIT) {
         while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)))!=DRMAA_ERRNO_SUCCESS) {
            printf("failed submitting job (%s) retry: %s\n", drmaa_strerror(drmaa_errno), diagnosis);
            sleep(1);
         }
         printf("submitted job \"%s\"\n", jobid);
      } else {
         if (!as_bulk_job) {
            if ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)))!=DRMAA_ERRNO_SUCCESS) {
               printf("failed submitting job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
            } else {
               printf("submitted job \"%s\"\n", jobid);
            }
         } else {
            drmaa_string_vector_t *jobids;
            if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, array_size, 1, diagnosis, sizeof(diagnosis)))!=DRMAA_ERRNO_SUCCESS) {
               printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
            } else {
               int j;
               printf("submitted bulk job with jobids:\n");
               for (j=0; j<array_size; j++) {
                  if (j==0)
                     drmaa_string_vector_get_first(jobids, jobid, sizeof(jobid)-1);
                  else
                     drmaa_string_vector_get_next(jobids, jobid, sizeof(jobid)-1);
                  printf("\t \"%s\"\n", jobid);
               } 
               drmaa_delete_string_vector(jobids);
            }
         }
      }
   }

   return NULL;
}

static int wait_all_jobs(void *vp)
{
   char jobid[100];
   int n, drmaa_errno;
   int stat;

   if (vp) 
      n = *(int *)vp;
   else 
      n = -1; /* all */

   do {
      drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, NULL, 0);
      if (drmaa_errno == DRMAA_ERRNO_SUCCESS) {
         printf("waited job \"%s\"\n", jobid);
         if (n != -1)
            if (--n == 0) {
               printf("waited for last job\n");
               break;
            }
      } else if (drmaa_errno != DRMAA_ERRNO_INVALID_JOB) {
         printf("drmaa_wait() returned %s\n", drmaa_strerror(drmaa_errno));
      }
   } while (drmaa_errno == DRMAA_ERRNO_SUCCESS);

   /* that means we got all */
   if (drmaa_errno == DRMAA_ERRNO_INVALID_JOB) {
      printf("no more jobs to wait\n");
      drmaa_errno = DRMAA_ERRNO_SUCCESS;
   }
   if (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE  && 
         test_case == MT_EXIT_DURING_SUBMIT_OR_WAIT) {
      drmaa_errno = DRMAA_ERRNO_SUCCESS;
   }

   return drmaa_errno;
}

static int wait_n_jobs(int n)
{
   char jobid[100];
   int drmaa_errno;
   int i, stat;

   for (i=0; i<n; i++) {
      do {
         drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, &stat, 
                  DRMAA_TIMEOUT_WAIT_FOREVER, NULL, NULL, 0);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            printf("drmaa_wait() returned %s\n", drmaa_strerror(drmaa_errno));
            sleep(1);
         }
      } while (drmaa_errno != DRMAA_ERRNO_SUCCESS);
      printf("waited job \"%s\"\n", jobid);
   }

   return drmaa_errno;
}
