#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>



#include <pthread.h>

#define DRMAA_HOOKS

/* this timeout is in effect with SGE commprocs */
#define SGE_COMMPROC_TIMEOUT 60*5

#include "drmaa.h"

#define JOB_CHUNK 8
#define NTHREADS 3
#define NBULKS 3

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

   ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL = 10,
      /* - drmaa_init() is called
         - bulk and sequential jobs are submitted
         - all jobs are waited individually
         - then drmaa_exit() is called */

   ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE = 11,
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) 
           to wait for all jobs to finish
         - then drmaa_exit() is called */

   ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE = 12,
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, no-dispose) 
           to wait for all jobs to finish
         - do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
           DRMAA_ERRNO_INVALID_JOB to reap all jobs
         - then drmaa_exit() is called */

   ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE = 13,
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(all_jobids, dispose) 
           to wait for all jobs to finish
         - then drmaa_exit() is called */

   ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE = 14,
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(all_jobids, no-dispose) 
           to wait for all jobs to finish
         - do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
           DRMAA_ERRNO_INVALID_JOB to reap all jobs
         - then drmaa_exit() is called */

   ST_SUBMIT_PAUSE_SUBMIT_SYNC = 15,
      /* - drmaa_init() is called
         - a job is submitted 
         - do a long sleep(SGE_COMMPROC_TIMEOUT+)
         - another job is submitted 
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) 
         - then drmaa_exit() is called */

   ST_INPUT_BECOMES_OUTPUT = 16,
      /* - drmaa_init() is called
         - job input is prepared in local file 
         - a job is submitted that echoes it's input to output 
         - the job is waited
         - job output must be identical to job input
         (still requires manual testing)
      */

   ST_SUBMIT_IN_HOLD = 17,
      /* - drmaa_init() is called
         - a job is submitted with a user hold 
         - use drmaa_job_ps() to verify user hold state
         - hold state is released *manually*
         - the job is waited
         - then drmaa_exit() is called
         (still requires manual testing)
      */

   ST_BULK_SUBMIT_IN_HOLD = 18,
      /* - drmaa_init() is called
         - a bulk job is submitted with a user hold 
         - hold state is released *manually*
         - the job ids are waited
         - then drmaa_exit() is called
         (still requires manual testing)
      */

   ST_JOB_PS = 19
      /* - drmaa_init() is called
         - drmaa_job_ps() is used to retrieve DRMAA state 
           for each jobid passed *manually* in argv
         - then drmaa_exit() is called 
         (still requires manual testing)
      */
};

#define NO_TEST ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE+1

static int test(int *argc, char **argv[]);
static void *submit_and_wait(void *vp);
static void *submit_sleeper_single(void *vp);
static void *submit_sleeper(int n);
static void *submit_input_mirror(int n);
static void *do_submit(drmaa_job_template_t *jt, int n);
static int wait_all_jobs(void *vp);
static int wait_n_jobs(int n);
static const char *drmaa_state2str(int state);
static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold);

static int test_case;


int main(int argc, char *argv[])
{
   int failed = 0;
   int i; 

   if (argc == 1) {
      fprintf(stderr, "usage: test_drmaa <test_case>\n"
                      "       <test_case> is a number: 0-%d\n"
                      "       see src for a detailed test case description\n", NO_TEST-1);
      return 1;
   }


   while (argc > 1) { 
      test_case = atoi(argv[1]);
      argc--; argv++;

      if (test_case == ALL_TESTS)
         for (i=ST_SUBMIT_WAIT; i<NO_TEST; i++) {
            test_case = i;
            printf("---------------------\n");
            printf("starting test #%d\n", i);
            if (test(&argc, &argv)!=0) {
               printf("test #%d failed\n", i);
               failed = 1;
               break;
            } else
               printf("successfully finished test #%d\n", i);
         }
      else {
         printf("starting test #%d\n", test_case);
         if (test(&argc, &argv)!=0) {
            printf("test #%d failed\n", test_case);
            failed = 1;
            break;
         } else
           printf("successfully finished test #%d\n", test_case);
      }
   } 

   return failed;
}

static int test(int *argc, char **argv[])
{
   int i;
   int job_chunk = JOB_CHUNK;
   char diagnosis[1024];
   drmaa_job_template_t *jt;
   int drmaa_errno;

   switch (test_case) {
   case ST_MULT_INIT:
      { 
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_ALREADY_ACTIVE_SESSION) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_MULT_EXIT:
      { 
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit(1) failed: %s\n", diagnosis);
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_NO_ACTIVE_SESSION) {
            fprintf(stderr, "drmaa_exit(2) failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_SUBMIT_WAIT:
      {
         int n = -1;
         char jobid[1024];

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("submitted job \"%s\"\n", jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         if (wait_all_jobs(&n) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case MT_SUBMIT_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 
         int n = -1;

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
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

         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
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
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis); 
            return 1;
         }
         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");

         if (wait_all_jobs(&n) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case MT_EXIT_DURING_SUBMIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         delay_after_submit = 20;

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         sleep(1);
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
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

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_and_wait, &job_chunk);

         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");

         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case MT_EXIT_DURING_SUBMIT_OR_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_and_wait, &job_chunk);
         sleep(2);
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
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
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            char jobid[100];
            drmaa_job_ids_t *jobids;
            int j;
            if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
               printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
               return 1;
            } 
            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         if (wait_n_jobs(JOB_CHUNK*NBULKS) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL:
      {
         char all_jobids[100][NBULKS*JOB_CHUNK + JOB_CHUNK];
         char jobid[100];
         int pos = 0;
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some bulk jobs
          */
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            drmaa_job_ids_t *jobids;
            int j;

            while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
                       sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
               sleep(1);
            } 
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
               return 1;
            }

            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               strncpy(all_jobids[pos++], jobid, sizeof(jobid)-1);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
            sleep(1);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
               sleep(1);
            }
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            strncpy(all_jobids[pos++], jobid, sizeof(jobid)-1);
            printf("\t \"%s\"\n", jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   wait all those jobs
          */
         for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
            do {
               int stat;
               drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
                  &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);
               if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
                  fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
                  sleep(1);
               }
            } while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
               return 1;
            }
            printf("waited job \"%s\"\n", all_jobids[pos]);
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE:
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) 
           to wait for all jobs to finish
         - then drmaa_exit() is called */
      {
         const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
         char jobid[100];
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some bulk jobs
          */
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            drmaa_job_ids_t *jobids;
            int j;

            while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
                       sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
               sleep(1);
            } 
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
               return 1;
            }

            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
            sleep(1);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
               sleep(1);
            }
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("\t \"%s\"\n", jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   synchronize with all jobs
          */
         drmaa_errno = drmaa_synchronize(session_all, DRMAA_TIMEOUT_WAIT_FOREVER, 1, diagnosis, sizeof(diagnosis)-1);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("waited all jobs\n");
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         break;
      }

   case ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE:

      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, no-dispose) 
           to wait for all jobs to finish
         - do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
           DRMAA_ERRNO_INVALID_JOB to reap all jobs
         - then drmaa_exit() is called */

      {
         const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
         const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };
         char jobid[100];
         int pos = 0;
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some bulk jobs
          */
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            drmaa_job_ids_t *jobids;
            int j;

            while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
                       sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
               sleep(1);
            } 
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
               return 1;
            }

            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               all_jobids[pos++] = strdup(jobid);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
            sleep(1);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
               sleep(1);
            }
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("\t \"%s\"\n", jobid);
            all_jobids[pos++] = strdup(jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   synchronize with all jobs
          */
         drmaa_errno = drmaa_synchronize(session_all, DRMAA_TIMEOUT_WAIT_FOREVER, 0, diagnosis, sizeof(diagnosis)-1);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("synchronized with all jobs\n");

         /*
          *   wait all those jobs
          */
         for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
            do {
               int stat;
               drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
                  &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);
               if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
                  fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
                  sleep(1);
               }
            } while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
               return 1;
            }
            printf("waited job \"%s\"\n", all_jobids[pos]);
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         break;
      }

   case ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE:
      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(all_jobids, dispose) 
           to wait for all jobs to finish
         - then drmaa_exit() is called */
      {
         const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
         char jobid[100];
         int pos = 0;
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some bulk jobs
          */
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            drmaa_job_ids_t *jobids;
            int j;

            while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
                       sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
               sleep(1);
            } 
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
               return 1;
            }

            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               printf("\t \"%s\"\n", jobid);
               all_jobids[pos++] = strdup(jobid);
            } 
            drmaa_release_job_ids(jobids);
            sleep(1);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
               sleep(1);
            }
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("\t \"%s\"\n", jobid);
            all_jobids[pos++] = strdup(jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   synchronize with all jobs
          */
         drmaa_errno = drmaa_synchronize(all_jobids, DRMAA_TIMEOUT_WAIT_FOREVER, 1, diagnosis, sizeof(diagnosis)-1);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("waited all jobs\n");
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         break;
      }

   case ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE:

      /* - drmaa_init() is called
         - submit a mixture of single and bulk jobs
         - do drmaa_synchronize(all_jobids, no-dispose) 
           to wait for all jobs to finish
         - do drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY) until 
           DRMAA_ERRNO_INVALID_JOB to reap all jobs
         - then drmaa_exit() is called */

      {
         const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
         char jobid[100];
         int pos = 0;
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some bulk jobs
          */
         if (!(jt = create_sleeper_job_template(5, 1, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<NBULKS; i++) {
            drmaa_job_ids_t *jobids;
            int j;

            while ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis,
                       sizeof(diagnosis)-1))==DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed - retry: %s\n", diagnosis);
               sleep(1);
            } 
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_bulk_jobs() failed: %s\n", diagnosis);
               return 1;
            }

            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               all_jobids[pos++] = strdup(jobid);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
            sleep(1);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<JOB_CHUNK; i++) {
            while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
               fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
               sleep(1);
            }
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("\t \"%s\"\n", jobid);
            all_jobids[pos++] = strdup(jobid);
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   synchronize with all jobs
          */
         drmaa_errno = drmaa_synchronize(all_jobids, DRMAA_TIMEOUT_WAIT_FOREVER, 0, diagnosis, sizeof(diagnosis)-1);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("synchronized with all jobs\n");

         /*
          *   wait all those jobs
          */
         for (pos=0; pos<NBULKS*JOB_CHUNK + JOB_CHUNK; pos++) {
            do {
               int stat;
               drmaa_errno = drmaa_wait(all_jobids[pos], jobid, sizeof(jobid)-1, 
                  &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);
               if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
                  fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[pos], diagnosis); 
                  sleep(1);
               }
            } while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[pos], diagnosis);
               return 1;
            }
            printf("waited job \"%s\"\n", all_jobids[pos]);
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         break;
      }

   case ST_SUBMIT_PAUSE_SUBMIT_SYNC:
      /* - drmaa_init() is called
         - a job is submitted
         - do a long sleep(ST_SUBMIT_PAUSE_SUBMIT_SYNC+)
         - another job is submitted
         - do drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose)
         - then drmaa_exit() is called */
      {
         const char *all_jobids[2+1];
         char jobid[100];
         int pos = 0;
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          *   submit some sequential jobs
          */
         if (!(jt = create_sleeper_job_template(5, 0, 0))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         for (i=0; i<2; i++) {
            drmaa_errno = drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                     sizeof(diagnosis)-1);
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
               return 1;
            }
            printf("\t \"%s\"\n", jobid);
            all_jobids[pos++] = strdup(jobid);

            /* 
             * enforce SGE commproc timeout 
             * this timeout must be handled transparently by DRMAA implementation
             */
            if (i==0) {
               printf("sleeping %d seconds\n", SGE_COMMPROC_TIMEOUT+30);
               sleep(SGE_COMMPROC_TIMEOUT+30);
            }
         }
         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   synchronize with all jobs
          */
         drmaa_errno = drmaa_synchronize(all_jobids, DRMAA_TIMEOUT_WAIT_FOREVER, 1, diagnosis, sizeof(diagnosis)-1);
         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("waited all jobs\n");

         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_INPUT_BECOMES_OUTPUT:
      {
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         for (i=0; i<NBULKS; i++) 
            submit_input_mirror(1);
         if (wait_n_jobs(1) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_SUBMIT_IN_HOLD:
      {
         char jobid[1024];
         int job_state;

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         if (!(jt = create_sleeper_job_template(5, 0, 1))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }
         if (drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
            return 1;
         }
         drmaa_delete_job_template(jt, NULL, 0);
         printf("submitted job in hold state \"%s\"\n", jobid);
         if (drmaa_job_ps(jobid, &job_state, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_job_ps(\"%s\")) failed: %s\n", jobid, diagnosis);
            return 1;
         }
         if (job_state != DRMAA_PS_USER_ON_HOLD && job_state != DRMAA_PS_USER_SYSTEM_ON_HOLD) {
            fprintf(stderr, "job \"%s\" is not in user hold state: %s\n", 
                     jobid, drmaa_state2str(job_state));
            return 1;
         }
         printf("verified user hold state for job \"%s\"\n", jobid);

         printf("please release hold state of job  \"%s\"\n", jobid);
         sleep(30);

         /* now job should be finished assumed it was scheduled immediately */
         if (drmaa_job_ps(jobid, &job_state, diagnosis, sizeof(diagnosis)-1)!=DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_job_ps(\"%s\")) failed: %s\n", jobid, diagnosis);
            return 1;
         }
         printf("state of job \"%s\" is now %s\n", jobid, drmaa_state2str(job_state));

         if (wait_n_jobs(1) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_BULK_SUBMIT_IN_HOLD:
      {
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

         if (!(jt = create_sleeper_job_template(5, 1, 1))) {
            fprintf(stderr, "create_sleeper_job_template() failed\n");
            return 1;
         }

         {
            drmaa_job_ids_t *jobids;
            int j;
            if ((drmaa_errno=drmaa_run_bulk_jobs(&jobids, jt, 1, JOB_CHUNK, 1, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
               printf("failed submitting bulk job (%s): %s\n", drmaa_strerror(drmaa_errno), diagnosis);
               return 1;
            } 
            printf("submitted bulk job with jobids:\n");
            for (j=0; j<JOB_CHUNK; j++) {
               char jobid[100];
               drmaa_get_next_job_id(jobids, jobid, sizeof(jobid)-1);
               printf("\t \"%s\"\n", jobid);
            } 
            drmaa_release_job_ids(jobids);
         }
         drmaa_delete_job_template(jt, NULL, 0);
         printf("please release hold state of jobs\n");
         if (wait_n_jobs(JOB_CHUNK) != DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_JOB_PS:
      {
         char diagnosis[1024];
/*          int state; */

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }

#if 0
         for (; *argc > 1; *argc--, *argv++) {
            if (drmaa_job_ps(*argv[1], &state, diagnosis, sizeof(diagnosis)-1)
                  !=DRMAA_ERRNO_SUCCESS) {
                fprintf(stderr, "drmaa_job_ps(\"%s\") failed: %s\n", *argv[1], diagnosis);
                return 1;
            }
            printf("%20s %s\n", *argv[1], drmaa_state2str(state));
         }
#endif
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   default:
      break;
   }

   return 0;
}

static const char *drmaa_state2str(int state)
{
  const struct state_descr_s {
     char *descr;
     int state;
  } state_vector[] = {
     { "DRMAA_PS_UNDETERMINED",          DRMAA_PS_UNDETERMINED },
     { "DRMAA_PS_QUEUED_ACTIVE",         DRMAA_PS_QUEUED_ACTIVE },
     { "DRMAA_PS_SYSTEM_ON_HOLD",        DRMAA_PS_SYSTEM_ON_HOLD },
     { "DRMAA_PS_USER_ON_HOLD ",         DRMAA_PS_USER_ON_HOLD },
     { "DRMAA_PS_USER_SYSTEM_ON_HOLD",   DRMAA_PS_USER_SYSTEM_ON_HOLD },
     { "DRMAA_PS_RUNNING",               DRMAA_PS_RUNNING },
     { "DRMAA_PS_SYSTEM_SUSPENDED",      DRMAA_PS_SYSTEM_SUSPENDED },
     { "DRMAA_PS_USER_SUSPENDED",        DRMAA_PS_USER_SUSPENDED },
     { "DRMAA_PS_USER_SYSTEM_SUSPENDED", DRMAA_PS_USER_SYSTEM_SUSPENDED },
     { "DRMAA_PS_DONE",                  DRMAA_PS_DONE },
     { "DRMAA_PS_FAILED",                DRMAA_PS_FAILED },
     { NULL, 0 }
  };
  int i;
  for (i=0; state_vector[i].descr != NULL; i++)
      if (state_vector[i].state == state)
         return state_vector[i].descr;
  return "DRMAA_PS_???UNKNOWN???";
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
   return submit_sleeper(n);
}

static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold)
{
   const char *job_argv[2];
   drmaa_job_template_t *jt = NULL;
   char buffer[100];

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, SLEEPER_JOB, NULL, 0);
   sprintf(buffer, "%d", seconds);
   job_argv[0] = buffer; 
   job_argv[1] = NULL;
   drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

#if 0
   if (!as_bulk_job)
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID", NULL, 0);
   else
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/DRMAA_JOB.$JOB_ID."DRMAA_PLACEHOLDER_INCR, NULL, 0);
#else
   /* no output please */
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, "/dev/null", NULL, 0);
#endif

   if (in_hold) 
      drmaa_set_attribute(jt, DRMAA_JS_STATE, DRMAA_SUBMISSION_STATE_HOLD, NULL, 0);

   return jt;
}

static void *submit_sleeper(int n)
{
   drmaa_job_template_t *jt;
   void *p;

   jt = create_sleeper_job_template(10, 0, 0);
   p = do_submit(jt, n);
   drmaa_delete_job_template(jt, NULL, 0);

   return p;
}

static void *submit_input_mirror(int n)
{
   drmaa_job_template_t *jt = NULL;
   void *p;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, MIRROR_JOB, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   drmaa_set_attribute(jt, DRMAA_INPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_input", NULL, 0);
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_output", NULL, 0);
#if 0
   drmaa_set_attribute(jt, DRMAA_INPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_input."DRMAA_PLACEHOLDER_INCR, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, DRMAA_PLACEHOLDER_HD"/tmp/job_output."DRMAA_PLACEHOLDER_INCR, NULL, 0);
#endif

   p = do_submit(jt, n);

   drmaa_delete_job_template(jt, NULL, 0);

   return p;
}

static void *do_submit(drmaa_job_template_t *jt, int n)
{
   int i;
   char diagnosis[1024];
   char jobid[100];
   int drmaa_errno;

   for (i=0; i<n; i++) {
      /* submit job */
      while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
         if ((test_case == MT_EXIT_DURING_SUBMIT_OR_WAIT || 
              test_case == MT_EXIT_DURING_SUBMIT) 
              && drmaa_errno == DRMAA_ERRNO_NO_ACTIVE_SESSION) {
            break;
         }
         printf("failed submitting job (%s) retry: %s\n", drmaa_strerror(drmaa_errno), diagnosis);
         sleep(1);
      }
      if (drmaa_errno== DRMAA_ERRNO_SUCCESS)
         printf("submitted job \"%s\"\n", jobid);
   }

   return NULL;
}

static int wait_all_jobs(void *vp)
{
   char jobid[100];
   int n, drmaa_errno = DRMAA_ERRNO_SUCCESS;
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
   int i, stat;
   int drmaa_errno = DRMAA_ERRNO_SUCCESS;

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

#if 0
static init_signal_handling()
{
   struct sigaction nact;
   
   nact.sa_handler = SIG_IGN;

   sigaction(SIGPIPE, &act, NULL);
}
#endif
