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

#include "sge_arch.h"
#include "sge_confL.h"
#include "sge_answerL.h"

#include "read_object.h"
#include "sge_getpwnam.h"
#include "sge_gdi_intern.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_me.h"
#include "config.h"
#include "exec_wrapper.h"
#include "setup_path.h"

#include "msg_utilib.h"

/* module global variables */
static lList *task_config = NULL;
static int mode_verbose = 0;
static int mode_remote = 1;
static int force_remote = 0;
static int mode_immediate = 1;

/* prevent remote execution before everything is initialized */
static int catch_exec_initialized = 0;



static int init_qtask_config(lList **alpp, print_func_t ostream);

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
   sprintf(fname, "%s/common/qtask", path.cell_root);

   if (!(fp = fopen(fname, "r")) && errno != ENOENT) {
      sprintf(SGE_EVENT, MSG_SGETEXT_CANT_OPEN_SS, fname, strerror(errno));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
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
   if (!(pwd = sge_getpwnam(me.user_name))) {
      sprintf(SGE_EVENT, "invalid user name \"%s\"\n", me.user_name);
      sge_add_answer(alpp, SGE_EVENT, STATUS_ENOSUCHUSER, 0);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   if (!pwd->pw_dir) {
      sprintf(SGE_EVENT, "missing home directory for user \"%s\"\n", me.user_name);
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
      (*ostream)("%s", SGE_EVENT);
      goto Error;
   }
   sprintf(fname, "%s/.qtask", pwd->pw_dir);
   
   if (!(fp = fopen(fname, "r")) && errno != ENOENT) {
      sprintf(SGE_EVENT, MSG_SGETEXT_CANT_OPEN_SS, fname, strerror(errno));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
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
char *path,   /* this is how tcsh tries to start the command */
char *argv[],
char *expath  /* this is how user typed in the command */
) {
   const char *value; 
   char *taskname = NULL, *s, *resreq;
   lListElem *task;
   int i, narg_resreq = 0, narg_argv = 0;
   char **argv_iter, 
        **newargv;
   char qrsh_path[2048];

   /* remote execution only for commands without any path information */
   if (!strchr(expath, '/')) {
      taskname = expath;
   }

#if 1
   if (mode_verbose)
      fprintf(stderr, "sge_execv(path = %s, taskname = %s, expath = %s)\n", 
         path, taskname?taskname:"<no remote execution>", expath);
#endif

   if (!mode_remote || 
         !taskname ||
         !(task=lGetElemStr(task_config, CF_name, taskname))) {
      if (mode_verbose)
         fprintf(stderr, "local ecexution of "SFQ"\n", expath);
      return execv(path, argv);
   }
  
   if ((value = lGetString(task, CF_value))) {
      resreq = (char *)malloc(strlen(value)+1);
      strcpy(resreq, value);
      for (s=strtok(resreq, " \t"); s; s=strtok(NULL, " \t"))
         narg_resreq++;
      free(resreq);
   }

   for (argv_iter=argv; argv_iter[0]; argv_iter++) {
      narg_argv++; 
   }
   newargv = (char **)malloc(sizeof(char *) * (
      1 +                                /* qrsh */
      (mode_verbose?1:0) +               /* -verbose */
      2 +                                /* -now [y|n] */
      narg_resreq +                      /* resource requests to qrsh */
      narg_argv +                        /* argv of command to be started */
      1                                  /* NULL */
   ));

   /* build argv for qrsh */
   i = 0;
   newargv[i++] = strdup("qrsh");

   if (mode_verbose) 
      newargv[i++] = strdup("-verbose");

   if (mode_immediate) {
      newargv[i++] = strdup("-now");
      newargv[i++] = strdup("y");
   } else {
      newargv[i++] = strdup("-now");
      newargv[i++] = strdup("n");
   }

   /* add optionally qrsh arguments from qtask file */
   if (value) {
      const char *s; 
      char *d;
      char quote;
      char *start;
      int finished;
      
      resreq = malloc(strlen(value) + 1);
      d = resreq;
      s = value;
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
            newargv[i++] = strdup(start);   /* assign argument */
            if(!finished) {
               while(isspace(*(++s)));      /* skip any number whitespace */
            }   
            start = d;                      /* assign start of next token */
         } else {
            *d++ = *s++;                    /* copy one character */
         }
      } 
      free(resreq);
   }

   /* add command's arguments */
   {
/*       int n; */

      for (argv_iter=argv; argv_iter[0]; argv_iter++) {
         newargv[i++] = argv_iter[0];
      }
      newargv[i] = NULL;
   }
#if 0
   if (mode_verbose) {
      /* trace exec'd command and arguments */
      for (argv_iter=newargv; argv_iter[0]; argv_iter++)
         fprintf(stderr, "%s ", argv_iter[0]);
      fprintf(stderr, "\n");
      fflush(stderr);
   }
#endif

   sprintf(qrsh_path, "%s/bin/%s/qrsh", sge_get_root_dir(1), sge_get_arch());

   return execvp(qrsh_path, newargv);
}


void sge_init(
print_func_t ostream 
) {
   lListElem *aep;
   lList *alp = NULL;

   sge_gdi_param(SET_EXIT_ON_ERROR, 0, NULL);
   if (sge_gdi_setup("qtcsh")==AE_OK) {
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
