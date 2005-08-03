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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "drmaa.h"

/* program defaults */
int njobs    = 100;
int nthreads = 1;
int dowait   = 1;
int asbulk   = 0;
char *native_spec = "-w n";
char *job_path = NULL;
char **job_args = NULL;

drmaa_job_template_t *jt;


static drmaa_job_template_t *create_job_template(const char *job_path, int as_bulk_job);
static void *submit_jobs(void *arg);
static void get_gmt(struct timeval *);

static void usage(void) 
{
   fprintf(stderr, "usage: test_drmaa_perf [ options ] <path-to-job> [<job_args>]\n");
/*    fprintf(stderr, "            -bulk       [yes|no]     single jobs or array jobs (default no)\n"); */
   fprintf(stderr, "            -jobs       <njobs>      number of jobs per thread (default 100)\n");
   fprintf(stderr, "            -native     <nativespec> native specification passed (default \"-w n\")\n");
   fprintf(stderr, "            -threads    <nthreads>   number of submission thread (default 1)\n");
   fprintf(stderr, "            -wait       [yes|no]     wait for job completion (default yes)\n");
   return;
}

#define DELTA_SECONDS(t1, t2) (((double)t2.tv_sec - (double)t1.tv_sec) + ((double)t2.tv_usec - (double)t1.tv_usec)/1000000)

int main(int argc, char *argv[])
{
   char diagnosis[DRMAA_ERROR_STRING_BUFFER];
   char jobid[100];
   int drmaa_errno, i;
   int ret = 0;
   struct timeval start_s, finish_s, wait_s;
  
   if (argc<2) {
      usage();
      return 1;
   }

   i = 1;
   do {
      if (!strcmp("-help", argv[i]) ||
          !strcmp("-h", argv[i])) {
         usage();
         return 0;

#if 0
      } else if (!strcmp("-bulk", argv[i])) {
         i++; 
         if (argc < i+1) {
            usage();
            return 1;
         }
         if (!strcmp("yes", argv[i]) || !strcmp("y", argv[i])) 
            asbulk = 1;
         else if (!strcmp("no", argv[i]) || !strcmp("n", argv[i])) 
            asbulk = 0;
         else {
            usage();
            return 1;
         }
         i++; 
#endif

      } else if (!strcmp("-jobs", argv[i])) {
         i++; 
         if (argc < i+1) {
            usage();
            return 1;
         }
         njobs = atoi(argv[i]);
         i++; 

      } else if (!strcmp("-native", argv[i])) {
         i++; 
         if (argc < i+1) {
            usage();
            return 1;
         }
         native_spec = argv[i];
         i++; 

      } else if (!strcmp("-threads", argv[i])) {
         if (argc < i+1) {
            usage();
            return 1;
         }
         i++; 
         nthreads = atoi(argv[i]);
         i++; 

      } else if (!strcmp("-wait", argv[i])) {
         i++; 
         if (argc < i+1) {
            usage();
            return 1;
         }
         if (!strcmp("yes", argv[i]) || !strcmp("y", argv[i])) 
            dowait = 1;
         else if (!strcmp("no", argv[i]) || !strcmp("n", argv[i])) 
            dowait = 0;
         else {
            usage();
            return 1;
         }
         i++; 

      } else {
         job_path = argv[i];
         i++; 
         if (job_path[0]=='-') {
            usage();
            return 1;
         }

         if (argv[i]) {
            job_args = &argv[i];
         }
      }
   } while (i < argc && !job_path);

   if (!job_path) {
      usage();
      return 1;
   }

#if 1
   printf("job_path: \"%s\"\n", job_path);
   printf("njobs:    %d\n", njobs);
   printf("nthreads: %d\n", nthreads);
   printf("native:   %s\n", native_spec);
   printf("dowait:   %s\n", dowait?"yes":"no");
/*    printf("bulk:     %s\n", asbulk?"yes":"no"); */
   printf("1st arg:  %s\n", job_args?job_args[0]:"<noargs>");
#endif

   if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
      return 1;
   }

   get_gmt(&start_s);

   if (!(jt = create_job_template(job_path, 0))) {
      fprintf(stderr, "create_sleeper_job_template() failed\n");
      return 1;
   }

   if (nthreads==1) {
      if (submit_jobs(&argv[i]))
          return 1;
   } else {
      pthread_t *ids = NULL;
      ids = (pthread_t *)malloc(sizeof (pthread_t) * nthreads);

      for (i = 0; i < nthreads; i++) {
         if (pthread_create(&ids[i], NULL, submit_jobs, NULL)) {
            fprintf(stderr, "pthread_create() failed: %s\n", strerror(errno));
            return 1;
         }
      }

      for (i = 0; i < nthreads; i++) {
         pthread_join(ids[i], NULL);
      }
   }

   drmaa_delete_job_template(jt, NULL, 0);

   get_gmt(&finish_s);
   printf("submission took %8.3f seconds\n", DELTA_SECONDS(start_s, finish_s)); 

   if (dowait) {
      int success = 1;

      for (i=0; i<njobs * nthreads; i++) {
         int stat;
         int aborted, exited, exit_status, signaled;

         drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, 
            &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);

         if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
            fprintf(stderr, "drmaa_wait() failed: %s\n", diagnosis);
            return 1;
         }

         /*
          * report how job finished 
          */
         drmaa_wifaborted(&aborted, stat, NULL, 0);
         if (aborted) {
            printf("job \"%s\" never ran\n", jobid);
            success = 0;
         } else {
            drmaa_wifexited(&exited, stat, NULL, 0);
            if (exited) {
               drmaa_wexitstatus(&exit_status, stat, NULL, 0);
               if (exit_status != 0) {
                  success = 0;
                  printf("job \"%s\" with exit status %d\n", jobid, exit_status);
               } else {
                  printf("job \"%s\" finished regularly\n", jobid);
               }
            } else {
               success = 0;
               drmaa_wifsignaled(&signaled, stat, NULL, 0);
               if (signaled) {
                  char termsig[DRMAA_SIGNAL_BUFFER+1];
                  drmaa_wtermsig(termsig, DRMAA_SIGNAL_BUFFER, stat, NULL, 0);
                  printf("job \"%s\" finished due to signal %s\n", jobid, termsig);
               } else
                  printf("job \"%s\" finished with unclear conditions\n", jobid);
            }
         }
      }

      if (!success)
         ret = 1;

      get_gmt(&wait_s);
      printf("wait took %8.3f seconds\n", DELTA_SECONDS(finish_s, wait_s)); 
      printf("jobs took %8.3f seconds\n", DELTA_SECONDS(start_s, wait_s)); 
   }

   if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
      return 1;
   }
   
  return ret;
}

static void *submit_jobs(void *arg)
{
   char diagnosis[DRMAA_ERROR_STRING_BUFFER];
   char jobid[100];
   int drmaa_errno, i;

   for (i=0; i<njobs; i++) {
      while ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
               sizeof(diagnosis)-1)) == DRMAA_ERRNO_DRM_COMMUNICATION_FAILURE) {
         fprintf(stderr, "drmaa_run_job() failed - retry: %s\n", diagnosis);
         sleep(1);
      }
      if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
         return (void *)1;
      }
      printf("\t \"%s\"\n", jobid);
   }

   return NULL;
}

static drmaa_job_template_t *create_job_template(const char *job_path, int as_bulk_job)
{
   drmaa_job_template_t *jt = NULL;

   if (drmaa_allocate_job_template(&jt, NULL, 0)!=DRMAA_ERRNO_SUCCESS)
      return NULL;

   /* run in users home directory */
   drmaa_set_attribute(jt, DRMAA_WD, DRMAA_PLACEHOLDER_HD, NULL, 0);
   drmaa_set_attribute(jt, DRMAA_NATIVE_SPECIFICATION, native_spec, NULL, 0);

   /* the job to be run */
   drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, job_path, NULL, 0);

   /* the job's arguments if any */
   if (job_args)
      drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, (const char **)job_args, NULL, 0);

   /* join output/error file */
   drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);

   /* path for output */
#if 0
   if (!as_bulk_job)
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB", NULL, 0);
   else
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_HD"/DRMAA_JOB."DRMAA_PLACEHOLDER_INCR, NULL, 0);
#else
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":/dev/null", NULL, 0);
#endif

   return jt;
}

static void get_gmt(struct timeval *now)
{
#  ifdef SOLARIS
   gettimeofday(now, NULL);
#  else
#     ifdef SINIX
   gettimeofday(now);
#     else
   struct timezone tzp;
   gettimeofday(now, &tzp);
#     endif
#  endif
   return;
}
