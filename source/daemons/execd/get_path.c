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
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "sge_conf.h"
#include "sge_jobL.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sgermon.h"
#include "sge_log.h"
#include "get_path.h"
#include "sge_uidgid.h"
#include "sge_prog.h"
#include "sge_dstring.h"
#include "msg_execd.h"
#include "sge_unistd.h"

static const char* expand_path(const char *path_in, u_long32 job_id, 
                               u_long32 ja_task_id, const char *job_name, 
                               const char *user, const char *fqhost);

static int getHomeDir(char *exp_path, const char *user);

int sge_get_path(
lList *lp,
const char *cwd,
const char *owner,
const char *job_name,
u_long32 job_number,
u_long32 ja_task_number,
int type,
char *pathstr 
) {
   lListElem *ep;
   const char *path = NULL, *host;

   DENTER(TOP_LAYER, "sge_get_path");

   strcpy(pathstr, "");

   /*
   ** check if there's a path for this host
   */

   ep = lGetElemHost(lp, PN_host, me.qualified_hostname);
   if (ep) {
      path = expand_path(lGetString(ep, PN_path), job_number, 
         ja_task_number, job_name, owner, me.qualified_hostname);
      host = lGetHost(ep, PN_host);
   }
   else {
      /* 
      ** hostname: wasn't set, look for a default 
      */
      for_each(ep, lp) {
         path = expand_path(lGetString(ep, PN_path), job_number, ja_task_number, 
            job_name, owner, me.qualified_hostname);
         host = lGetHost(ep, PN_host);
         if (!host) 
            break;
      }
   }

   /*
   ** prepend cwd to path
   */
   if (path && path[0] != '/')              /* got relative path from -e/-o */
      sprintf(pathstr, "%s/%s", cwd, path);
   else if (path)                           /* got absolute path from -e/-o */
      strcpy(pathstr, path);
   else if (type != SGE_SHELL)              /* no -e/-o directive (but not for shells) */
      strcpy(pathstr, cwd);
 
   DEXIT;
   return 0;
}

static const char* expand_path(
const char *in_path,
u_long32 job_id,
u_long32 ja_task_id,
const char *job_name,
const char *user,
const char *host 
) {
   char *t;
   const char *s;
   static char exp_path[10000];
   char tmp[255];
   
   DENTER(TOP_LAYER, "expand_path");
   
   exp_path[0] = '\0';

   if (in_path) {
      s = in_path;
      /*
      ** handle ~/ and ~username/
      */
      if (s[0] == '~') {
         t = strchr(s, '/');
         strncpy(tmp, s+1, t-s+1);
         strcat(tmp, "");
         if (!strcmp(tmp, "")) {
            if (!getHomeDir(exp_path, user)) {
               DEXIT;
               return NULL;
            }
            s = s + 2;
         }
         else if (!getHomeDir(exp_path, tmp)) {
               s = t;
         }
      }
      t = strchr(s, '$');
      while (t) {
         strncat(exp_path, s, t-s);
         s = t;
         if (!strncmp(t, "$HOME", sizeof("$HOME") - 1)) {
            if (!getHomeDir(exp_path, user)) {
               DEXIT;
               return NULL;
            }
            s = t + sizeof("$HOME") - 1;
         }
         if (!strncmp(t, "$JOB_ID", sizeof("$JOB_ID") - 1)) {
            sprintf(exp_path, "%s" u32, exp_path, job_id);
            s = t + sizeof("$JOB_ID") - 1;
         }
         if (ja_task_id) {
            if (!strncmp(t, "$TASK_ID", sizeof("$TASK_ID") - 1)) {
               sprintf(exp_path, "%s" u32, exp_path, ja_task_id);
               s = t + sizeof("$TASK_ID") - 1;
            }
         }
         if (!strncmp(t, "$JOB_NAME", sizeof("$JOB_NAME") - 1)) {
            sprintf(exp_path, "%s%s", exp_path, job_name);
            s = t + sizeof("$JOB_NAME") - 1;
         }
         if (!strncmp(t, "$USER", sizeof("$USER") - 1)) {
            sprintf(exp_path, "%s%s", exp_path, user);
            s = t + sizeof("$USER") - 1;
         }
         if (!strncmp(t, "$HOSTNAME", sizeof("$HOSTNAME") - 1)) {
            sprintf(exp_path, "%s%s", exp_path, host);
            s = t + sizeof("$HOSTNAME") - 1;
         }
         if (*s == '$')  {
            strncat(exp_path, s, 1);
            s++;
         }
         t = strchr(s, '$');
      }
      strcat(exp_path, s);
   }

   DEXIT;
   return exp_path;
}

static int getHomeDir(
char *exp_path,
const char *user 
) {
   struct passwd *pwd;

   DENTER(TOP_LAYER, "getHomeDir");

   pwd = sge_getpwnam(user);
   if (!pwd) {
      ERROR((SGE_EVENT, MSG_EXECD_INVALIDUSERNAME_S, user));
      DEXIT;
      return 0;
   }
   if (!pwd->pw_dir) {
      ERROR((SGE_EVENT, MSG_EXECD_NOHOMEDIR_S, user));
      DEXIT;
      return 0;

   }
   strcat(exp_path, pwd->pw_dir);

   DEXIT;
   return 1;
}

/****** execd/fileio/sge_get_active_job_file_path() ********************************
*  NAME
*     sge_get_active_job_file_path() -- Create paths in active_jobs dir
*
*  SYNOPSIS
*     const char* sge_get_active_job_file_path(dstring *buffer, 
*        u_long32 job_id, u_long32 ja_task_id, const char *pe_task_id, 
*        const char *filename) 
*
*  FUNCTION
*     Creates paths in the execd's active_jobs directory.
*     Both directory and file paths can be created.
*     The result is placed in a buffer provided by the caller.
*
*  INPUTS
*     dstring *buffer        - buffer to hold the generated path
*     u_long32 job_id        - job id 
*     u_long32 ja_task_id    - array task id
*     const char *pe_task_id - optional pe task id
*     const char *filename   - optional file name
*
*  RESULT
*     const char* - pointer to the string buffer on success, else NULL
*
*  EXAMPLE
*     To create the relative path to a jobs/tasks environment file, the 
*     following call would be used:
*
*     char buffer[SGE_PATH_MAX]
*     sge_get_active_job_file_path(buffer, SGE_PATH_MAX, 
*                                  job_id, ja_task_id, pe_task_id,
*                                  "environment");
*     
*
*  NOTES
*     JG: TODO: The function might be converted to or might use a more 
*     general path creating function (utilib).
*
*  SEE ALSO
*     execd/fileio/sge_make_ja_task_active_dir()
*     execd/fileio/sge_make_pe_task_active_dir()
*******************************************************************************/
const char *sge_get_active_job_file_path(dstring *buffer, u_long32 job_id, u_long32 ja_task_id, const char *pe_task_id, const char *filename) 
{
   DENTER(TOP_LAYER, "sge_get_active_job_file_path");

   if(buffer == NULL) {
      return NULL;
   }

   sge_dstring_sprintf(buffer, "%s/"u32"."u32, ACTIVE_DIR, job_id, ja_task_id);

   if(pe_task_id != NULL) {
      sge_dstring_append(buffer, "/");
      sge_dstring_append(buffer, pe_task_id);
   }

   if(filename != NULL) {
      sge_dstring_append(buffer, "/");
      sge_dstring_append(buffer, filename);
   }

   DEXIT;
   return sge_dstring_get_string(buffer);
}


/****** execd/fileio/sge_make_ja_task_active_dir() *********************************
*  NAME
*     sge_make_ja_task_active_dir() -- create a jatask's active job directory
*
*  SYNOPSIS
*     const char* sge_make_ja_task_active_dir(const lListElem *job, 
*                                             const lListElem *ja_task, 
*                                             dstring *err_str) 
*
*  FUNCTION
*     Creates the active jobs sub directory for a job array task.
*     If it already exists (because the task has been restarted)
*     and the execd is configured to keep the active job directories
*     (execd_param KEEP_ACTIVE), it is renamed to <old_name>.n, where
*     n is a number from 0 to 9. If a job is restarted more than 10 times,
*     the old active job dir will be removed and only the first 10 be kept.
*
*  INPUTS
*     const lListElem *job     - job object
*     const lListElem *ja_task - ja task object
*     dstring *err_str         - optional buffer to hold error strings. If it is
*                                NULL, errors are output.
*
*  RESULT
*     const char* - the path of the jobs/jatasks active job directory, or NULL if
*                   the function call failed.
*
*  SEE ALSO
*     execd/fileio/sge_get_active_job_file_path()
*     execd/fileio/sge_make_pe_task_active_dir()
*******************************************************************************/
const char *sge_make_ja_task_active_dir(const lListElem *job, const lListElem *ja_task, dstring *err_str)
{
   static dstring path_buffer = DSTRING_INIT;
   const char *path;
   int result;

   DENTER(TOP_LAYER, "sge_make_ja_task_active_dir");
   
   if(err_str != NULL) {
      sge_dstring_clear(err_str);
   }
   
   if(job == NULL || ja_task == NULL) {
      DEXIT;
      return NULL;
   }

   /* build path to active dir */
   path = sge_get_active_job_file_path(&path_buffer,
                                       lGetUlong(job, JB_job_number), 
                                       lGetUlong(ja_task, JAT_task_number), 
                                       NULL, NULL);   

   /* try to create it */
   result = mkdir(path, 0755);
   if(result == -1) {
      /* if it already exists and keep_active: try to rename it */
      if(errno == EEXIST && keep_active && lGetUlong(ja_task, JAT_job_restarted) > 0) {
         dstring new_path = DSTRING_INIT;
         int i, success = 0;

         for(i = 0; i < 10; i++) {
            sge_dstring_sprintf(&new_path, "%s.%d", path, i);
            if(rename(path, sge_dstring_get_string(&new_path)) == 0) {
               success = 1;
               break;
            } else {
               if(errno == EEXIST) {
                  continue;
               }
            }
         }
         
         /* if it couldn't be renamed: try to remove it */
         if(success == 0) {
            DPRINTF(("could not rename old active job dir "SFN" - removing it\n", path));

            if(sge_rmdir(path, SGE_EVENT)) {
               if(err_str != NULL) {
                  sge_dstring_sprintf(err_str, MSG_FILE_RMDIR_SS, path, SGE_EVENT);
               } else {
                  ERROR((SGE_EVENT, MSG_FILE_RMDIR_SS, path, SGE_EVENT));
                  DEXIT;
                  return NULL;
               }
            }
         }

         sge_dstring_free(&new_path);

         result = mkdir(path, 0755);
      }
   }   

   if(result == -1) {
      /* error creating directory */
      if(err_str != NULL) {
         sge_dstring_sprintf(err_str, MSG_FILE_CREATEDIR_SS, path, strerror(errno));
      } else {
         ERROR((SGE_EVENT, MSG_FILE_CREATEDIR_SS, path, strerror(errno)));
      }
      DEXIT;
      return NULL;
   }

   DEXIT;
   return path;
}

/****** execd/fileio/sge_make_pe_task_active_dir() *********************************
*  NAME
*     sge_make_pe_task_active_dir() -- create a petask's active job directory
*
*  SYNOPSIS
*     const char* sge_make_pe_task_active_dir(const lListElem *job, 
*                                             const lListElem *ja_task, 
*                                             const lListElem *pe_task, 
*                                             dstring *err_str) 
*
*  FUNCTION
*     Creates the active job sub directory for a pe task.
*
*  INPUTS
*     const lListElem *job     - the job object
*     const lListElem *ja_task - the ja task object
*     const lListElem *pe_task - the pe task object
*     dstring *err_str         - optional buffer to hold error strings. If it is
*                                NULL, errors are output.
*
*  RESULT
*     const char* - the path of the jobs/jatasks active job directory, or NULL if
*                   the function call failed.
*
*  SEE ALSO
*     execd/fileio/sge_get_active_job_file_path()
*     execd/fileio/sge_make_ja_task_active_dir()
*******************************************************************************/
const char *sge_make_pe_task_active_dir(const lListElem *job, const lListElem *ja_task, const lListElem *pe_task, dstring *err_str)
{
   static dstring path_buffer = DSTRING_INIT;
   const char *path;

   DENTER(TOP_LAYER, "sge_make_pe_task_active_dir");
  
   if(err_str != NULL) {
      sge_dstring_clear(err_str);
   }
  
   if(job == NULL || ja_task == NULL || pe_task == NULL) {
      DEXIT;
      return NULL;
   }

   /* build path to active dir */
   path = sge_get_active_job_file_path(&path_buffer,
                                       lGetUlong(job, JB_job_number), 
                                       lGetUlong(ja_task, JAT_task_number), 
                                       lGetString(pe_task, PET_id), 
                                       NULL);   

   /* try to create it */
   if (mkdir(path, 0755) == -1) {
      /* error creating directory */
      if(err_str != NULL) {
         sge_dstring_sprintf(err_str, MSG_FILE_CREATEDIR_SS, path, strerror(errno));
      } else {
         ERROR((SGE_EVENT, MSG_FILE_CREATEDIR_SS, path, strerror(errno)));
      }
      DEXIT;
      return NULL;
   }

   DEXIT;
   return path;
}
