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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include "sge_me.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_getpwnam.h"
#include "get_path.h"
#include "msg_execd.h"

static char* expand_path(char *path_in, u_long32 job_id, u_long32 ja_task_id, char *job_name, char *user, char *fqhost);
static int getHomeDir(char *exp_path, char *user);


int sge_get_path(
lList *lp,
char *cwd,
char *owner,
char *job_name,
u_long32 job_number,
u_long32 ja_task_number,
int type,
char *pathstr 
) {
   lListElem *ep;
   char *path = NULL, *host;

   DENTER(TOP_LAYER, "sge_get_path");

   strcpy(pathstr, "");

   /*
   ** check if there's a path for this host
   */

   ep = lGetElemHost(lp, PN_host, me.qualified_hostname);
   if (ep) {
      path = expand_path(lGetString(ep, PN_path), job_number, 
         ja_task_number, job_name, owner, me.qualified_hostname);
      host = lGetString(ep, PN_host);
   }
   else {
      /* 
      ** hostname: wasn't set, look for a default 
      */
      for_each(ep, lp) {
         path = expand_path(lGetString(ep, PN_path), job_number, ja_task_number, 
            job_name, owner, me.qualified_hostname);
         host = lGetString(ep, PN_host);
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

static char* expand_path(
char *in_path,
u_long32 job_id,
u_long32 ja_task_id,
char *job_name,
char *user,
char *host 
) {
   char *t;
   char *s;
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
char *user 
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
