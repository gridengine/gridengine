#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>



#include <pthread.h>

/* this timeout is in effect with SGE commprocs */
#define SGE_COMMPROC_TIMEOUT 60*5

#include "drmaa.h"

#define JOB_CHUNK 8
#define NTHREADS 3
#define NBULKS 3

#define NEXT_ARGV(argc, argv) \
      ((*argc)--, (*argv)++, (*argv)[0])

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

   ST_INPUT_FILE_FAILURE = 16,
   ST_OUTPUT_FILE_FAILURE = 17,
   ST_ERROR_FILE_FAILURE = 18,
      /* - drmaa_init() is called
         - a job is submitted with input/output/error path specification 
           that must cause the job to fail
         - use drmaa_synchronize() to ensure job was started
         - drmaa_job_ps() must return DRMAA_PS_FAILED
         - drmaa_wait() must report drmaa_wifaborted() -> true
         - then drmaa_exit() is called */

   ST_INPUT_BECOMES_OUTPUT = 19,
      /* - drmaa_init() is called
         - job input is prepared in local file 
         - a job is submitted that echoes it's input to output 
         - the job is waited
         - then drmaa_exit() is called
         - job output must be identical to job input
           (this requires manual testing) */

   ST_SUBMIT_IN_HOLD = 20,
      /* - drmaa_init() is called
         - a job is submitted with a user hold 
         - use drmaa_job_ps() to verify user hold state
         - hold state is released *manually*
         - the job is waited
         - then drmaa_exit() is called
         (still requires manual testing)
      */

   ST_BULK_SUBMIT_IN_HOLD = 21,
      /* - drmaa_init() is called
         - a bulk job is submitted with a user hold 
         - hold state is released *manually*
         - the job ids are waited
         - then drmaa_exit() is called
         (still requires manual testing)
      */

   ST_JOB_PS = 22,
      /* - drmaa_init() is called
         - drmaa_job_ps() is used to retrieve DRMAA state 
           for each jobid passed *manually* in argv
         - then drmaa_exit() is called 
         (still requires manual testing)
      */

   ST_EXIT_STATUS = 23
      /* - drmaa_init() is called
         - 255 job are submitted
         - job i returns i as exit status (8 bit)
         - drmaa_wait() verifies each job returned the 
           correct exit status
         - then drmaa_exit() is called */
};

const struct test_name2number_map {
   char *test_name;       /* name of the test                                    */
   int test_number;       /* number the test is internally mapped to             */
   int nargs;             /* number of test case arguments required              */
   char *opt_arguments;   /* description of test case arguments for usage output */
} test_map[] = {

   /* a automated tests - ST_* and MT_* tests */
   { "ALL_AUTOMATED",                            ALL_TESTS,                              2, "<sleeper_job> <exit_arg_job>" },

   /* one application thread - automated tests only */
   { "ST_SUBMIT_WAIT",                           ST_SUBMIT_WAIT,                         1, "<sleeper_job>" },
   { "ST_MULT_INIT",                             ST_MULT_INIT,                           0, "" },
   { "ST_MULT_EXIT",                             ST_MULT_EXIT,                           0, "" },
   { "ST_BULK_SUBMIT_WAIT",                      ST_BULK_SUBMIT_WAIT,                    1, "<sleeper_job>" },
   { "ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL",     ST_BULK_SINGLESUBMIT_WAIT_INDIVIDUAL,   1, "<sleeper_job>" },
   { "ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE",        ST_SUBMITMIXTURE_SYNC_ALL_DISPOSE,      1, "<sleeper_job>" },
   { "ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE",      ST_SUBMITMIXTURE_SYNC_ALL_NODISPOSE,    1, "<sleeper_job>" },
   { "ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE",     ST_SUBMITMIXTURE_SYNC_ALLIDS_DISPOSE,   1, "<sleeper_job>" },
   { "ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE",   ST_SUBMITMIXTURE_SYNC_ALLIDS_NODISPOSE, 1, "<sleeper_job>" },
   { "ST_SUBMIT_PAUSE_SUBMIT_SYNC",              ST_SUBMIT_PAUSE_SUBMIT_SYNC,            1, "<sleeper_job>" },
   { "ST_EXIT_STATUS",                           ST_EXIT_STATUS,                         1, "<exit_arg_job>" },
   { "ST_INPUT_FILE_FAILURE",                    ST_INPUT_FILE_FAILURE,                  1, "<sleeper_job>" },
   { "ST_OUTPUT_FILE_FAILURE",                   ST_OUTPUT_FILE_FAILURE,                 1, "<sleeper_job>" },
   { "ST_ERROR_FILE_FAILURE",                    ST_ERROR_FILE_FAILURE,                  1, "<sleeper_job>" },

   /* multiple application threads - automated tests only */
   { "MT_SUBMIT_WAIT",                           MT_SUBMIT_WAIT,                         1, "<sleeper_job>" },
   { "MT_SUBMIT_BEFORE_INIT_WAIT",               MT_SUBMIT_BEFORE_INIT_WAIT,             1, "<sleeper_job>" },
   { "MT_EXIT_DURING_SUBMIT",                    MT_EXIT_DURING_SUBMIT,                  1, "<sleeper_job>" },
   { "MT_SUBMIT_MT_WAIT",                        MT_SUBMIT_MT_WAIT,                      1, "<sleeper_job>" },
   { "MT_EXIT_DURING_SUBMIT_OR_WAIT",            MT_EXIT_DURING_SUBMIT_OR_WAIT,          1, "<sleeper_job>" },
   
   /* tests that require test suite to be run in an automated fashion */
   { "ST_INPUT_BECOMES_OUTPUT",                  ST_INPUT_BECOMES_OUTPUT,                3, "<mirror_job> <input_path> <output_path>" },

   /* tests that test_drmaa can't test in an automated fashion (so far) */
   { "ST_SUBMIT_IN_HOLD",                        ST_SUBMIT_IN_HOLD,                      1, "<sleeper_job>" },
   { "ST_BULK_SUBMIT_IN_HOLD",                   ST_BULK_SUBMIT_IN_HOLD,                 1, "<sleeper_job>" },
   { "ST_JOB_PS",                                ST_JOB_PS,                              1, "<jobid> ..."   },

   { NULL,                                       0 }
};
#define FIRST_NON_AUTOMATED_TEST ST_INPUT_BECOMES_OUTPUT

static int test(int *argc, char **argv[], int parse_args);
static void *submit_and_wait(void *vp);
static void *submit_sleeper_single(void *vp);
static void *submit_sleeper(int n);
static void *submit_input_mirror(int n, const char *mirror_job, 
      const char *input_path, const char *output_path, const char *error_path, int join);
static void *do_submit(drmaa_job_template_t *jt, int n);
static int wait_all_jobs(void *vp);
static int wait_n_jobs(int n);
static const char *drmaa_state2str(int state);
static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold);
static drmaa_job_template_t *create_exit_job_template(const char *exit_job, int as_bulk_job);
static void report_session_key(void);

static int test_case;
static int is_sun_grid_engine;

/* global test case parameters */
char *sleeper_job = NULL,
     *exit_job = NULL,
     *mirror_job = NULL,
     *input_path = NULL,
     *output_path = NULL;

static void usage(void)
{
   int i;
   fprintf(stderr, "usage: test_drmaa <test_case>\n");

   fprintf(stderr, "  <test_case> is one of the keywords below including the enlisted test case arguments\n");
   for (i=0; test_map[i].test_name; i++)
      fprintf(stderr, "\t%-40.40s %s\n", test_map[i].test_name, test_map[i].opt_arguments);

   fprintf(stderr, "  <sleeper_job>  is an executable job that sleeps <argv1> seconds\n");
   fprintf(stderr, "                 the job must be executable at the target machine\n");
   fprintf(stderr, "  <mirror_job>   is an executable job that returns it's stdin stream to stdout (e.g. /bin/cat)\n");
   fprintf(stderr, "                 the job must be executable at the target machine\n\n");
   fprintf(stderr, "  <exit_arg_job> is an executable job that exits <argv1> as exit status\n");
   fprintf(stderr, "                 the job must be executable at the target machine\n");
   fprintf(stderr, "  <input_path>   is the path of an input file\n");
   fprintf(stderr, "                 the user must have read access to this file at the target machine\n");
   fprintf(stderr, "  <output_path>  is the path of an output file\n");
   fprintf(stderr, "                 the user must have write access to this file at the target machine\n");

   exit(1);
}

int main(int argc, char *argv[])
{
   int failed = 0;
   int i; 

   if (argc == 1) 
      usage();

   /* figure out which DRM system we are using */
   {
      char drm_name[DRMAA_DRM_SYSTEM_BUFFER];
      drmaa_get_DRM_system(drm_name, 255 /* , NULL, 0 */);
      printf("Connected to DRM system \"%s\"\n", drm_name);
      if (!strncmp(drm_name, "SGE", 3))
         is_sun_grid_engine = 1;
      else
         is_sun_grid_engine = 0;
   }

   while (argc > 1) {
      /* map test name to test number */
      for (i=0; test_map[i].test_name; i++)
         if (!strcasecmp(argv[1], test_map[i].test_name))
            break;
      if (!test_map[i].test_name) {
         fprintf(stderr, "test_drmaa: %s is not a valid test name\n", argv[1]);
         usage();
      }
      test_case = test_map[i].test_number;
      argc--; argv++;
      if ((argc-1) < test_map[i].nargs)
         usage();

      if (test_case == ALL_TESTS) {
         sleeper_job = NEXT_ARGV(&argc, &argv);
         exit_job    = NEXT_ARGV(&argc, &argv);

         for (i=1; test_map[i].test_name && i<FIRST_NON_AUTOMATED_TEST; i++) {
            test_case = test_map[i].test_number;
            printf("---------------------\n");
            printf("starting test #%d (%s)\n", i, test_map[i].test_name);
            if (test(&argc, &argv, 0)!=0) {
               printf("test #%d failed\n", i);
               failed = 1;
               drmaa_exit(NULL, 0);
               break;
            } else
               printf("successfully finished test #%d\n", i);
         }
      } else {
         printf("starting test #%d (%s)\n", test_case, test_map[i].test_name);
         if (test(&argc, &argv, 1)!=0) {
            printf("test #%d failed\n", test_case);
            failed = 1;
            drmaa_exit(NULL, 0);
            break;
         } else
           printf("successfully finished test #%d\n", test_case);
      }
   } 

   return failed;
}


static int test(int *argc, char **argv[], int parse_args)
{
   int i;
   int job_chunk = JOB_CHUNK;
   char diagnosis[DRMAA_ERROR_STRING_BUFFER];
   drmaa_job_template_t *jt;
   int drmaa_errno;

   switch (test_case) {
   case ST_MULT_INIT:
      { 
         /* no test case arguments */

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();
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
         /* no test case arguments */

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();
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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         for (i=0; i<NTHREADS; i++) {
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         }

         /* delay drmaa_init() */
         sleep(5);
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis); 
            return 1;
         }
         report_session_key();

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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         putenv("SGE_DELAY_AFTER_SUBMIT=20");

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

         for (i=0; i<NTHREADS; i++)
            pthread_create(&submitter_threads[i], NULL, submit_sleeper_single, &job_chunk);
         sleep(1);
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         printf("drmaa_exit() succeeded\n");
         
         putenv("SGE_DELAY_AFTER_SUBMIT=0");

         for (i=0; i<NTHREADS; i++)
            if (pthread_join(submitter_threads[i], NULL))
               printf("pthread_join() returned != 0\n");
      }
      break;

   case MT_SUBMIT_MT_WAIT:
      {
         pthread_t submitter_threads[NTHREADS]; 

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();
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
         const char *all_jobids[NBULKS*JOB_CHUNK + JOB_CHUNK+1];
         char jobid[100];
         int pos = 0;
         
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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
            all_jobids[pos++] = strdup(jobid);
            printf("\t \"%s\"\n", jobid);
         }

         /* set string array end mark */
         all_jobids[pos] = NULL;

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
         
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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
         
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         /* set string array end mark */
         all_jobids[pos] = NULL;

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
         
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         /* set string array end mark */
         all_jobids[pos] = NULL;

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
         
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         /* set string array end mark */
         all_jobids[pos] = NULL;

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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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
         /* set string array end mark */
         all_jobids[pos] = NULL;

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

   case ST_INPUT_FILE_FAILURE:
   case ST_ERROR_FILE_FAILURE:
   case ST_OUTPUT_FILE_FAILURE:
      {
         int aborted, stat, remote_ps;
         char jobid[100];
         const char *session_all[] = { DRMAA_JOB_IDS_SESSION_ALL, NULL };

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();
        
         /* submit a job that must fail */
         drmaa_allocate_job_template(&jt, NULL, 0);
         drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, sleeper_job, NULL, 0);

         switch (test_case) {
         case ST_OUTPUT_FILE_FAILURE:
            drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);
            drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, "/etc/passwd", NULL, 0);
            break;

         case ST_ERROR_FILE_FAILURE:
            drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "n", NULL, 0);
            drmaa_set_attribute(jt, DRMAA_ERROR_PATH, "/etc/passwd", NULL, 0);
            break;

         case ST_INPUT_FILE_FAILURE:
            drmaa_set_attribute(jt, DRMAA_INPUT_PATH, "<not existing file>", NULL, 0);
            break;
         }

         if ((drmaa_errno = drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
                  sizeof(diagnosis)-1)) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
            return 1;
         }
         drmaa_delete_job_template(jt, NULL, 0);
         printf("submitted job \"%s\"\n", jobid);

         /* synchronize with job to finish but do not dispose job finish information */
         if ((drmaa_errno = drmaa_synchronize(session_all, DRMAA_TIMEOUT_WAIT_FOREVER, 0, 
                     diagnosis, sizeof(diagnosis)-1))!=DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_synchronize(DRMAA_JOB_IDS_SESSION_ALL, dispose) failed: %s\n", diagnosis);
            return 1;
         }
         printf("synchronized with job finish\n");

         /* get job state */
         drmaa_errno = drmaa_job_ps(jobid, &remote_ps, diagnosis, sizeof(diagnosis)-1);
         if (remote_ps != DRMAA_PS_FAILED) {
            fprintf(stderr, "job \"%s\" is not in failed state: %s\n", 
                     jobid, drmaa_state2str(remote_ps));
            return 1;
         }

         /* wait job */
         if ((drmaa_errno = drmaa_wait(jobid, NULL, 0, &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, 
               diagnosis, sizeof(diagnosis)-1)) != DRMAA_ERRNO_SUCCESS) {
            printf("drmaa_wait() failed %s: %s\n", drmaa_strerror(drmaa_errno), diagnosis);
            return 1;
         }

         /* job finish information */
         drmaa_wifaborted(&aborted, stat, diagnosis, sizeof(diagnosis)-1);
         if (!aborted) {
            fprintf(stderr, "job \"%s\" failed but drmaa_wifaborted() returns false\n", 
                  jobid);
            return 1;
         }
         printf("waited job \"%s\" that never ran\n", jobid);

         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
      }
      break;

   case ST_INPUT_BECOMES_OUTPUT:
      {

         if (parse_args) {
            mirror_job  = NEXT_ARGV(argc, argv);
            input_path  = NEXT_ARGV(argc, argv);
            output_path = NEXT_ARGV(argc, argv);
         }

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

         if (submit_input_mirror(1, mirror_job, input_path, output_path, NULL, 1)!=DRMAA_ERRNO_SUCCESS) {
            return 1;
         }
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

         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();
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
         if (parse_args)
            sleeper_job = NEXT_ARGV(argc, argv);

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

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

         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

#if 0
         for (; *argc > 1; *argc--, *argv++) {
/*          int state; */

            /* for this test args must always be parsed */
            job_id = NEXT_ARGV(argc, argv);
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

   case ST_EXIT_STATUS:
      /* - drmaa_init() is called
         - 255 job are submitted
         - job i returns i as exit status (8 bit)
         - drmaa_wait() verifies each job returned the
           correct exit status
         - then drmaa_exit() is called */
      {
         char diagnosis[1024];
         char *all_jobids[256];
         const char *job_argv[2];
         char jobid[1024];
         char buffer[100];

         if (parse_args)
            exit_job = NEXT_ARGV(argc, argv);
         
         if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
            return 1;
         }
         report_session_key();

         /*
          *   submit sequential jobs
          */
         if (!(jt = create_exit_job_template(exit_job, 0))) {
            fprintf(stderr, "create_exit_job_template() failed\n");
            return 1;
         }
         for (i=0; i<255; i++) {
  
            /* parametrize exit job with job argument */
            sprintf(buffer, "%d", i);
            job_argv[0] = buffer; 
            job_argv[1] = NULL;
            drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);

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
            all_jobids[i] = strdup(jobid);
         }

         /* set string array end mark */
         all_jobids[i] = NULL;

         drmaa_delete_job_template(jt, NULL, 0);

         /*
          *   wait for all jobs and verify exit status
          */
         
         for (i=0; i<255; i++) {
            int stat, exit_status, exited;

            do {
               drmaa_errno = drmaa_wait(all_jobids[i], jobid, sizeof(jobid)-1, 
                  &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);
               if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
                  fprintf(stderr, "drmaa_wait(%s) failed - retry: %s\n", all_jobids[i], diagnosis); 
                  sleep(1);
               }
            } while (drmaa_errno == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE);

            printf("job %d with job id %s finished\n", i, all_jobids[i]);
            if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
               fprintf(stderr, "drmaa_wait(%s) failed: %s\n", all_jobids[i], diagnosis);
               return 1;
            }
          
            drmaa_wifexited(&exited, stat, NULL, 0);
            if (!exited) {
               fprintf(stderr, "job \"%s\" did not exit cleanly\n", all_jobids[i]);
               return 1;
            }
            drmaa_wexitstatus(&exit_status, stat, NULL, 0);
            if (exit_status != i) {
               fprintf(stderr, "job \"%s\" returned wrong exit status %d instead of %d\n", 
                                 all_jobids[i], exit_status, i);
               return 1;
            }

         }

         printf("waited all jobs\n");
         if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
            return 1;
         }
         break;
      }


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

static drmaa_job_template_t *create_exit_job_template(const char *exit_job, int as_bulk_job)
{
   const char *job_argv[2];
   drmaa_job_template_t *jt = NULL;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, exit_job, NULL, 0);

   job_argv[0] = "0"; 
   job_argv[1] = NULL;
   drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, job_argv, NULL, 0);

   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   /* no output please */
   drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, "/dev/null", NULL, 0);

   return jt;
}
static drmaa_job_template_t *create_sleeper_job_template(int seconds, int as_bulk_job, int in_hold)
{
   const char *job_argv[2];
   drmaa_job_template_t *jt = NULL;
   char buffer[100];

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, sleeper_job, NULL, 0);
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

static void *submit_input_mirror(int n, const char *mirror_job, 
      const char *input_path, const char *output_path, const char *error_path, int join)
{
   drmaa_job_template_t *jt = NULL;
   void *p;

   drmaa_allocate_job_template(&jt, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, mirror_job, NULL, 0);
   if (join)
      drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);
   else
      drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "n", NULL, 0);

   if (input_path)
      drmaa_set_attribute(jt, DRMAA_INPUT_PATH, input_path, NULL, 0);
   if (output_path)
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, output_path, NULL, 0);
   if (error_path)
      drmaa_set_attribute(jt, DRMAA_ERROR_PATH, error_path, NULL, 0);

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

static void report_session_key(void) 
{
   if (is_sun_grid_engine) {
      const char *session_key = getenv("SGE_SESSION_KEY");
      if (session_key) 
         printf("got \"%s\" as session key\n", session_key);
      else
         printf("no session key set\n");
      
   }
}



#if 0
static init_signal_handling()
{
   struct sigaction nact;
   
   nact.sa_handler = SIG_IGN;

   sigaction(SIGPIPE, &act, NULL);
}
#endif
