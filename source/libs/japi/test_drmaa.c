#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>

#define DRMAA_HOOKS

#include "drmaa.h"

#define JOB_CHUNK 2
#define NTHREADS 10

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

   NO_TEST
};


static int test(void);
static void *submit_and_wait(void *vp);
static void *submit_sleeper(void *vp);
static int wait_all_jobs(void *vp);

static int test_case;


int main(int argc, char *argv[])
{
   int failed, i; 

   if (argc == 1) {
      fprintf(stderr, "usage: test_japi <test_case>\n"
                      "       <test_case> is a number: 0-4\n"
                      "       see src for a detailed test case description\n");
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
            submit_sleeper(&job_chunk);
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
            pthread_create(&submitter_threads[i], NULL, submit_sleeper, &job_chunk);
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
            pthread_create(&submitter_threads[i], NULL, submit_sleeper, &job_chunk);
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
            pthread_create(&submitter_threads[i], NULL, submit_sleeper, &job_chunk);
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

   submit_sleeper(&n);
   wait_all_jobs(&n);

   return NULL;
}

static void *submit_sleeper(void *vp)
{
   int i, n;
   char diagnosis[1024];
   char jobid[100];
   char *job_argv[2];
   drmaa_job_template_t *jt = NULL;
   int drmaa_errno;

   if (vp) 
      n = *(int *)vp;
   else 
      n = 1;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, "/cod_home/ah114088/SGE53/examples/jobs/sleeper.sh", NULL, 0);
   job_argv[0] = "10"; 
   job_argv[1] = NULL;
   drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);

   for (i=0; i<n; i++) {
      /* submit job */
      if (test_case == MT_SUBMIT_BEFORE_INIT_WAIT) {
         while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)))!=DRMAA_ERRNO_SUCCESS) {
            printf("failed submitting job (%s) retry: %s\n", drmaa_strerror(drmaa_errno), diagnosis);
            sleep(1);
         }
         printf("submitted job \"%s\"\n", jobid);
      } else {
         if ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)))!=DRMAA_ERRNO_SUCCESS) {
            printf("failed submitting job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
         } else {
            printf("submitted job \"%s\"\n", jobid);
         }
      }
   }

   drmaa_delete_job_template(jt, NULL, 0);

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
      drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, &stat, DRMAA_TIMEOUT_WAIT_FOREVER, /* NULL, */ NULL, 0);
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
