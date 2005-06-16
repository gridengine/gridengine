#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "drmaa.h"

#define JOB_MAX 100

typedef struct {
   char *name;
   char *ns;
   char *jid;
} job_info_t;

job_info_t job[JOB_MAX];

void usage(FILE* fp) {
	fprintf(fp, "usage: raimk <aimk_options>");
}

int main(int argc, char *argv[])
{
   int ret = 0;
   char *s;
   char diagnosis[DRMAA_ERROR_STRING_BUFFER];
   char jobwd[1024*4];
   char jobid[1024];
   char line[2*1024];
   int drmaa_errno;
   drmaa_job_template_t *jt = NULL;
   int stat;
   int aborted, exited, exit_status, signaled;
   int i, j, njobs = 0;
   FILE *fp;

   if (!getwd(jobwd)) {
      fprintf(stderr, "getwd() failed: %s\n", strerror(errno));
      ret = 2;
      goto Finish;
   }

   if (!(fp = fopen("compile.conf", "r"))) {
      fprintf(stderr, "fopen(\"compile.conf\") failed: %s\n", strerror(errno));
      ret = 2;
      goto Finish;
   }

   if (drmaa_init(NULL, diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_init() failed: %s\n", diagnosis);
      return 2;
   }

   printf("--- start cluster session --------------------------------\n");

   while (fscanf(fp, "%[^\n]\n", line) == 1) {
      char nat_spec[1024];
      char name[1024];
      char ns[1024];
      char dummy[1024];

      if (line[0] == '#')
         continue;

/* printf("line \"%s\"\n", line); */
      if (sscanf(line, "%[^\t ]%[\t ]%[^\n]\n", name, dummy, ns) != 3) {
         fprintf(stderr, "parsing error in compile.conf\n");
         continue;
      }
/* printf("using name \"%s\" and native spec \"%s\"\n", name, ns); */

      if (drmaa_allocate_job_template(&jt, NULL, 0)!=DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
         ret = 2;
         goto Finish;
      }

      drmaa_set_attribute(jt, DRMAA_WD, jobwd, NULL, 0);
      drmaa_set_attribute(jt, DRMAA_REMOTE_COMMAND, argv[1], NULL, 0);
      drmaa_set_attribute(jt, DRMAA_JOIN_FILES, "y", NULL, 0);
      drmaa_set_attribute(jt, DRMAA_JOB_NAME, name, NULL, 0);
      sprintf(nat_spec, "-b no -S /bin/csh -now yes %s", ns);
      drmaa_set_attribute(jt, DRMAA_NATIVE_SPECIFICATION, nat_spec, NULL, 0);
      drmaa_set_attribute(jt, DRMAA_OUTPUT_PATH, ":"DRMAA_PLACEHOLDER_WD"/$JOB_ID", NULL, 0);
      drmaa_set_vector_attribute(jt, DRMAA_V_ARGV, (const char **)&argv[2], NULL, 0);

      if ((drmaa_errno=drmaa_run_job(jobid, sizeof(jobid)-1, jt, diagnosis,
               sizeof(diagnosis)-1)) != DRMAA_ERRNO_SUCCESS) {
         drmaa_delete_job_template(jt, NULL, 0);

         if (drmaa_errno == DRMAA_ERRNO_DENIED_BY_DRM) {
            printf("--- job \"%s\" using \"%s\" wasn't accepted: %s\n", name, ns, diagnosis);
            continue;
         }

         fprintf(stderr, "drmaa_run_job() failed: %s\n", diagnosis);
         ret = 2;
         goto Finish;
      }
      drmaa_delete_job_template(jt, NULL, 0);
      
      printf("    submitted job \"%s\" as %s\n", name, jobid);

      job[njobs].jid = strdup(jobid);
      job[njobs].name = strdup(name);
      job[njobs].ns = strdup(ns);
      njobs++;
   }
   fclose(fp);

   for (i=0; i<njobs; i++) {
      drmaa_errno = drmaa_wait(DRMAA_JOB_IDS_SESSION_ANY, jobid, sizeof(jobid)-1, 
         &stat, DRMAA_TIMEOUT_WAIT_FOREVER, NULL, diagnosis, sizeof(diagnosis)-1);
      if (drmaa_errno != DRMAA_ERRNO_SUCCESS) {
         fprintf(stderr, "drmaa_wait() failed: %s\n", diagnosis);
         ret = 2;
         goto Finish;
      }

      for (j=0; j<njobs; j++)
         if (!strcmp(jobid, job[j].jid))
            break;
      if (j==njobs)
         fprintf(stderr, "drmaa_wait() returns unknown job ... ?\n");

      /*
       * report how job finished 
       */
      drmaa_wifaborted(&aborted, stat, NULL, 0);
      if (aborted)
         printf("--- run \"%s\" stopped or never started\n", job[j].name);
      else {
         char path[1024];
         int failed = 1;

         drmaa_wifexited(&exited, stat, NULL, 0);
         if (exited) {
            drmaa_wexitstatus(&exit_status, stat, NULL, 0);
            if (exit_status == 0) {
               printf("+++ run \"%s\" was successful\n", job[j].name);
               failed = 0;
            } else
               printf("### run \"%s\" broken ##################################\n", job[j].name);
         } else {
            drmaa_wifsignaled(&signaled, stat, NULL, 0);
            if (signaled) {
               char termsig[DRMAA_SIGNAL_BUFFER+1];
               drmaa_wtermsig(termsig, DRMAA_SIGNAL_BUFFER, stat, NULL, 0);
               printf("job \"%s\" finished due to signal %s\n", job[j].name, termsig);
            } else
               printf("job \"%s\" finished with unclear conditions\n", job[j].name);
         }

         sprintf(path, "%s/%s", jobwd, job[j].jid);
         if (failed) {
            char tail_cmd[1024];
            sprintf(tail_cmd, "tail -15 %s", path);
            system(tail_cmd);
         }
         if (unlink(path)) {
            fprintf(stderr, "couldn't unlink \"%s\" job output file %s: %s\n",
                  job[j].name, path, strerror(errno));
         }
      }
   }

   printf("--- end cluster session --------------------------------\n");

Finish:
   if (drmaa_exit(diagnosis, sizeof(diagnosis)-1) != DRMAA_ERRNO_SUCCESS) {
      fprintf(stderr, "drmaa_exit() failed: %s\n", diagnosis);
      return 1;
   }
   return ret;
}
