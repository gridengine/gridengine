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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

#include "sge_gdi.h"
#include "sge_answer.h"
#include "sge_any_request.h"
#include "sgermon.h"
#include "sge_log.h"
#include "config.h"
#include "sge_qtcsh.h"
#include "setup_path.h"
#include "sge_uidgid.h"
#include "sge_prog.h"
#include "sge_conf.h"

#include "msg_common.h"

/* module global variables */
static lList *task_config = NULL;
static int mode_verbose = 0;
static int mode_remote = 1;
static int force_remote = 0;
static int mode_immediate = 1;

/* prevent remote execution before everything is initialized */
static int catch_exec_initialized = 0;



static int init_qtask_config(lList **alpp, print_func_t ostream);

/****** sge_qtcsh/init_qtask_config() ******************************************
*  NAME
*     init_qtask_config() -- ??? 
*
*  SYNOPSIS
*     static int init_qtask_config(lList **alpp, print_func_t ostream) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **alpp         - ??? 
*     print_func_t ostream - ??? 
*
*  RESULT
*     static int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTES: init_qtask_config() is not MT safe as it uses unsafe 
*     MT-NOTES: sge_getpwnam()
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static int init_qtask_config(
lList **alpp,
print_func_t ostream 
) {
   struct passwd *pwd;
   char fname[SGE_PATH_MAX + 1];
   char buffer[10000];
   FILE *fp;
   lList *clp_cluster = NULL, *clp_user = NULL;
   lListElem *nxt, *cep_dest, *cep, *next;
   const char *task_name;

   /* cell global settings */
   sprintf(fname, "%s/common/qtask", path_state_get_cell_root());

   if (!(fp = fopen(fname, "r")) && errno != ENOENT) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_CANT_OPEN_SS, fname, strerror(errno)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   if (fp) {
      /* read in config file */
      if (read_config_list(fp, &clp_cluster, alpp, CF_Type, CF_name, CF_value,
                        CF_sublist, NULL, RCL_NO_VALUE, buffer, sizeof(buffer)-1)) {
         fclose(fp);
         goto Error;
      }
      fclose(fp);
   }

   /* skip tasknames containing '/' */
   nxt = lFirst(clp_cluster);
   while ((cep=nxt)) {
      nxt = lNext(cep);
      if (strrchr(lGetString(cep, CF_name), '/')) 
         lRemoveElem(clp_cluster, cep);

   }

   /* user settings */
   if (!(pwd = sge_getpwnam(uti_state_get_user_name()))) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_USER_INVALIDNAMEX_S , uti_state_get_user_name()));
      answer_list_add(alpp, SGE_EVENT, STATUS_ENOSUCHUSER, ANSWER_QUALITY_ERROR);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   if (!pwd->pw_dir) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_USER_NOHOMEDIRFORUSERX_S , uti_state_get_user_name()));
      answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   sprintf(fname, "%s/.qtask", pwd->pw_dir);
   
   if (!(fp = fopen(fname, "r")) && errno != ENOENT) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SGETEXT_CANT_OPEN_SS, fname, strerror(errno)));
      answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   if (fp) {
      /* read in config file */
      if (read_config_list(fp, &clp_user, alpp, CF_Type, CF_name, CF_value,
                           CF_sublist, NULL, RCL_NO_VALUE, buffer, sizeof(buffer)-1)) {
         fclose(fp);
         goto Error;
      }
      fclose(fp);
   }

   /* skip tasknames containing '/' */
   nxt = lFirst(clp_user);
   while ((cep=nxt)) {
      nxt = lNext(cep);
      if (strrchr(lGetString(cep, CF_name), '/')) 
         lRemoveElem(clp_user, cep);
   }

#if 0
   if (clp_user)
      for_each (cep, clp_user) {
         (*ostream) ("info: user: command %s request %s\n", lGetString(cep, CF_name), (s=lGetString(cep, CF_value))?s:"");
      }
   else 
      (*ostream) ("info: empty user task list\n");
#endif

   /* merge contents of user list into cluster list */
   next = lFirst(clp_user);
   while ((cep=next)) {
      char *ro_task_name;
      next = lNext(cep);
      task_name = lGetString(cep, CF_name);
   
      /* build task name with leading '!' for search operation */
      ro_task_name = (char *)malloc(strlen(task_name) + 2);
      ro_task_name[0] = '!';
      strcpy(&ro_task_name[1], task_name);

      if ((cep_dest=lGetElemStr(clp_cluster, CF_name, ro_task_name))) {
         /* do not override cluster global task entry */
         lRemoveElem(clp_user, cep);
      } else if ((cep_dest=lGetElemStr(clp_cluster, CF_name, task_name))) {
         /* override cluster global task entry */
         lSetString(cep_dest, CF_value, lGetString(cep, CF_value));
         lRemoveElem(clp_user, cep);
      } else {
         /* no entry in cluster global task list 
            use entry from user task list */
         lDechainElem(clp_user, cep);
         if (!clp_cluster) 
            clp_cluster = lCreateList("cluster config", CF_Type);
         lAppendElem(clp_cluster, cep); 
      }

      free(ro_task_name);
   }
   lFreeList(clp_user);

   if (task_config)
      lFreeList(task_config);
   task_config = clp_cluster;

   /* remove leading '!' from command names */
   for_each (cep, clp_cluster) {
      task_name = lGetString(cep, CF_name);
      if (task_name[0] == '!') {
         char *t = (char *)malloc(strlen(task_name));
         strcpy(t, &task_name[1]);
         lSetString(cep, CF_name, t);
         free(t);
      }
   }


#if 0
   if (task_config)
      for_each (cep, task_config) {
         (*ostream) ("info: session: command %s request %s\n", lGetString(cep, CF_name), (s=lGetString(cep, CF_value))?s:"");
      }
   else 
      (*ostream) ("info: empty task list\n");
#endif

   return 0;

Error:
   lFreeList(clp_cluster);
   lFreeList(clp_user);
   return -1;
}

int sge_execv(
char *path,    /* this is how tcsh tries to start the command */
char *argv[],
char *expath,  /* this is how user typed in the command */
int close_stdin /* use of qrsh's -nostdin option */
) {
   const char *value; 
   char *taskname = NULL;
   lListElem *task;
   int i, narg_resreq = 0, narg_argv = 0;
   char **argv_iter, 
        **newargv;
   char qrsh_path[2048];

   /* remote execution only for commands without any path information */
   if (!strchr(expath, '/')) {
      taskname = expath;
   }

   if (mode_verbose) {
      fprintf(stderr, "sge_execv(path = %s, taskname = %s, expath = %s, close_stdin = %d)\n", 
         path, taskname?taskname:"<no remote execution>", expath, close_stdin);
   }

   if (!mode_remote || 
         !taskname ||
         !(task=lGetElemStr(task_config, CF_name, taskname))) {
      if (mode_verbose)
         fprintf(stderr, "local execution of "SFQ"\n", expath);
      return execv(path, argv);
   }
  
   if ((value = lGetString(task, CF_value))) {
      narg_resreq = sge_quick_count_num_args (value);
   }

   for (argv_iter=argv; argv_iter[0] != NULL; argv_iter++) {
      narg_argv++; 
   }
   
   newargv = (char **)malloc(sizeof(char *) * (
      1 +                                /* qrsh */
      (close_stdin?1:0) +                /* -nostdin */
      (mode_verbose?1:0) +               /* -verbose */
      2 +                                /* -now [y|n] */
      narg_resreq +                      /* resource requests to qrsh */
      narg_argv +                        /* argv of command to be started */
      1                                  /* NULL */
   ));

   /* build argv for qrsh */
   i = 0;
   newargv[i++] = strdup("qrsh");

   if (close_stdin) 
      newargv[i++] = strdup("-nostdin");

   if (mode_verbose) 
      newargv[i++] = strdup("-verbose");

   if (mode_immediate) {
      newargv[i++] = strdup("-now");
      newargv[i++] = strdup("y");
   } else {
      newargv[i++] = strdup("-now");
      newargv[i++] = strdup("n");
   }

   /* add optional qrsh arguments from qtask file */
   if (value) {
      sge_parse_args (value, &newargv[i]);
      i += narg_resreq;
   }
	 
   /* add command's arguments */
   for (argv_iter=argv; argv_iter[0] != NULL; argv_iter++) {
      newargv[i++] = argv_iter[0];
   }
      
   newargv[i] = NULL;

   sprintf(qrsh_path, "%s/bin/%s/qrsh", sge_get_root_dir(1, NULL, 0, 1), sge_get_arch());

   return execvp(qrsh_path, newargv);
}

/* This method counts the number of arguments in the string using a quick and
 * dirty algorithm.  The algorithm may incorrectly report the number of arguments
 * to be too large because it does not parse quotations correctly. */
int sge_quick_count_num_args (
const char* args /* The argument string to count by whitespace tokens */
) {
   int num_args = 0;
   char *resreq = (char *)malloc (strlen (args)+1);
   char *s;
   
   DENTER (TOP_LAYER, "count_num_qtask_args");
   
   /* This function may return a larger number than required since it does not
    * parse quotes.  This is ok, however, since it's current usage is for
    * mallocing arrays, and a little too big is fine. */
   strcpy (resreq, args);
   for (s=strtok (resreq, " \t"); s; s=strtok(NULL, " \t"))
      num_args++;
   free(resreq);
   
   DEXIT;
   return num_args;
}

/* This method should probably be moved out of this file into somewhere more
 * common so that other routines can use it. */
void sge_parse_args (
const char* args, /* The argument string to parse by whitespace and quotes */
char** pargs /* The array to contain the parsed arguments */
) {
   const char *s; 
   char *d;
   char quote;
   char *start;
   char *resreq;
   int finished, count = 0;

   DENTER (TOP_LAYER, "sge_parse_args");
   
   resreq = malloc (strlen (args) + 1);
   d = resreq;
   s = args;
   start = resreq;
   finished = 0;
   
   while(!finished) {
      if(*s == '"' || *s == '\'') {      /* copy quoted arguments */
         quote = *s++;                   /* without quotes */
         while(*s && *s != quote) 
           *d++ = *s++;
         if(*s == quote) 
            s++;
      }

      if(*s == 0) finished = 1;          /* line end ? */

      if(finished || isspace(*s)) {      /* found delimiter or line end */
         *d++ = 0;                       /* terminate token */
         pargs[count++] = strdup(start);   /* assign argument */
         if(!finished) {
            while(isspace(*(++s)));      /* skip any number whitespace */
         }   
         start = d;                      /* assign start of next token */
      } else {
         *d++ = *s++;                    /* copy one character */
      }
   } 
   free(resreq);
   
   DEXIT;
}

/* This function was added to enable the DRMAA library to handle job
 * categories.  It is similar to sge_init() except that it doesn't do any
 * initialization. */
char** sge_get_qtask_args (
char *taskname, /* The name of the task to look for in the qtask files */
lList *alp /* For returning error information */
) {
   const char *value; 
   int num_args = 0;
   lListElem *task;
   char** args = NULL;
   
   DENTER (TOP_LAYER, "sge_get_qtask_args");
   
   if (mode_verbose) {
      fprintf(stderr, "sge_get_qtask_args(taskname = %s)\n", taskname);
   }

   /* If the task_config has not been filled yet, fill it.  We call
    * init_qtask_config() instead of sge_init() because we don't need to setup
    * the GDI.  We just need the qtask arguments. */
   if (!task_config) {
      /* Just using printf here since we don't really have an exciting function
       * like xprintf to pass in.  This was really meant for use with qtsch. */
      if (init_qtask_config (&alp, (print_func_t)printf)) {
         const char *s;
         lListElem *aep;
         
         if (!alp || !(aep=lFirst(alp)) || !(s=lGetString(aep, AN_text))) {
            s = "unknown reason";
         }

         DEXIT;
         return args;
      }
   }

   if (!(task=lGetElemStr(task_config, CF_name, taskname))) {
      if (mode_verbose)
         fprintf(stderr, "unknown qtask name: %s\n", taskname);
      DEXIT;
      return args;
   }
  
   if ((value = lGetString(task, CF_value))) {
      num_args = sge_quick_count_num_args (value);
   }
   
   args = (char **)malloc (sizeof (char *) * (num_args + 1));   
   
   sge_parse_args (value, args);
   args[num_args] = NULL;
   
   DEXIT;
   return args;
}

void sge_init(
print_func_t ostream 
) {
   lListElem *aep;
   lList *alp = NULL;

   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if (sge_gdi_setup("qtcsh", NULL)==AE_OK) {
      if (init_qtask_config(&alp, ostream)) {
         const char *s;
         if (!alp || !(aep=lFirst(alp)) || !(s=lGetString(aep, AN_text)))
            s = "unknown reason";
         mode_remote = 0;          
      } else {
         /* Remote execution is default.

            Turn off remote execution only in case we were 
            started in the context of an already running job.
            This is done to prevent recursive 

              qrsh -> qtcsh -> qrsh -> qtcsh -> ...

            submission via SGE/SGE in case qtcsh
            is the login shell at the execution server.
          */
         mode_remote = force_remote?mode_remote:!getenv("JOB_ID");          
/*          (*ostream) ("mode_remote = %d\n", mode_remote); */
      }
      lFreeList(alp);
   } else {
      mode_remote = 0;          
/*       (*ostream) ("no $SGE_ROOT, running as normal tcsh\n"); */
   }

   catch_exec_initialized = 1;
   return;
}

void set_sgemode(
int addr,
int value 
) {
   switch (addr) {
   case CATCH_EXEC_MODE_REMOTE:
      mode_remote = value;
      break;
   case CATCH_EXEC_MODE_VERBOSE:
      mode_verbose = value;
      break;
   case CATCH_EXEC_MODE_IMMEDIATE:
      mode_immediate = value;
      break;
   case CATCH_EXEC_MODE_FORCE_REMOTE:
      force_remote = value;
      break;
   default:
      break;
  }      
  return;
}

int get_sgemode(
int addr 
) {
   int value = -1;

   switch (addr) {
   case CATCH_EXEC_MODE_REMOTE:
      value = mode_remote;
      break;
   case CATCH_EXEC_MODE_VERBOSE:
      value = mode_verbose;
      break;
   case CATCH_EXEC_MODE_IMMEDIATE:
      value = mode_immediate;
   default:
      break;
  }      
  return value;
}
