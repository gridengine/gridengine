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
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

#include "sgermon.h"
#include "sge_peopen.h"
#include "sge_exit.h"
#include "basis_types.h"
#include "msg_utilib.h"
#include "sge_log.h"

#ifdef NO_SGE_COMPILE_DEBUG
#   undef SGE_EXIT
#   define SGE_EXIT(x)     exit(x)
#endif

static void addenv(char *, char *);

/* advanced popen with additional parameters:
   free shell usage
   login shell if wanted
   user under which to start (for root only)
   stdin and stderr fps

   and last but not least: wait for exactly the process we started
*/

pid_t peopen(
char *shell,     /* which shell to use */    
int login_shell, /* make it a login shell ? */
char *command,   /* name of the program */
char *user,      /* user under which to start (for root only) */
char **env,      /* environment variables to add to child */
FILE **fp_in, 
FILE **fp_out, 
FILE **fp_err 
) {
   static pid_t pid;
   int pipefds[3][2];
   const char *could_not = MSG_SYSTEM_EXECBINSHFAILED;
   const char *not_root = MSG_SYSTEM_NOROOTRIGHTSTOSWITCHUSER;
   int i;
   char arg0[256];
   struct passwd *pw;
   char err_str[256];
#ifndef WIN32 /* var not needed */
   int res;
#endif /* WIN32 */
   uid_t myuid;

   DENTER(TOP_LAYER, "peopen");

   /* open pipes - close on failure */
   for (i=0; i<3; i++)
      if (pipe(pipefds[i])) {
         while (--i >= 0) {
            close(pipefds[i][0]);
            close(pipefds[i][1]);
         }
         ERROR((SGE_EVENT, MSG_SYSTEM_FAILOPENPIPES_SS, command, strerror(errno)));
         DEXIT;
         return -1;
      }

   if (!(pid = fork())) {	/* son */

      close(pipefds[0][1]);
      close(pipefds[1][0]);
      close(pipefds[2][0]);
      close(0);
      close(1);
      close(2);
      dup(pipefds[0][0]);
      dup(pipefds[1][1]);
      dup(pipefds[2][1]);

      if (user) {

         pw = getpwnam(user);
         if (!pw) {
            sprintf(err_str, MSG_SYSTEM_NOUSERFOUND_SS , user, strerror(errno));
            write(2, err_str, strlen(err_str));
            SGE_EXIT(1);
         }

         myuid = geteuid();

         if (myuid != pw->pw_uid) {

            /* Only change user if we differ from the wanted user */

            if (myuid != 0) {
               write(2, not_root, sizeof(not_root));
               SGE_EXIT(1);
            }
            sprintf(err_str, "%s %d\n", pw->pw_name, (int)pw->pw_gid);
            write(2, err_str, strlen(err_str));
#ifndef WIN32 /* initgroups not called */
            res = initgroups(pw->pw_name,pw->pw_gid);
#  if defined(SVR3) || defined(sun)
            if (res<0)
#  else
            if (res)
#  endif
            {
               sprintf(err_str, MSG_SYSTEM_INITGROUPSFORUSERFAILED_ISS ,
                     res, user, strerror(errno));
               write(2, err_str, strlen(err_str));
               SGE_EXIT(1);
            }
#endif /* WIN32 */

            if (setuid(pw->pw_uid)) {
               sprintf(err_str, MSG_SYSTEM_SWITCHTOUSERFAILED_SS , user,
                     strerror(errno));
               write(2, err_str, strlen(err_str));
               SGE_EXIT(1);
            }
         }

         addenv("HOME", pw->pw_dir);
         addenv("SHELL", pw->pw_shell);
         addenv("USER", pw->pw_name);
         addenv("LOGNAME", pw->pw_name);
         addenv("PATH", "/usr/local/bin:/usr/ucb:/bin:/usr/bin:");
      }

      if (login_shell)
         strcpy(arg0, "-");
      else
         strcpy(arg0, "");
      strcat(arg0, shell);

      if (env)
         for(; *env; env++)
            putenv(*env);

      execlp(shell, arg0, "-c", command, NULL);

      write(2, could_not, sizeof(could_not));
      SGE_EXIT(1);
   }

   if (pid == -1) {
      for (i=0; i<3; i++) {
         close(pipefds[i][0]);
         close(pipefds[i][1]);
      }
      DEXIT;
      return -1;
   }

   *fp_in  = fdopen(pipefds[0][1], "a");
   *fp_out = fdopen(pipefds[1][0], "r");
   *fp_err = fdopen(pipefds[2][0], "r");
   close(pipefds[0][0]);
   close(pipefds[1][1]);
   close(pipefds[2][1]);

   DEXIT;
   return pid;
}

/**************************************************/
int peclose(pid, fp_in, fp_out, fp_err, timeout)
pid_t pid;
FILE *fp_in, *fp_out, *fp_err;
struct timeval *timeout;
{
   int i, status;

   DENTER(TOP_LAYER, "peclose");

   fclose(fp_in);
   fclose(fp_out);
   fclose(fp_err);

   do {
      i = waitpid(pid, &status, timeout?WNOHANG:0);
      if (i==-1) {
         DEXIT;
         return -1;
      }
      if (i==0) { /* not yet exited */
         if (timeout->tv_sec == 0) {
#ifdef WIN32 /* kill not called */
            /* CygWin has no kill command */
            DPRINTF(("killing not yet implemented\n"));
            timeout = NULL;
            /* kill(pid, SIGKILL); */
#else
            DPRINTF(("killing\n"));
            timeout = NULL;
            kill(pid, SIGKILL);
#endif /* WIN32 */
         } else {
            DPRINTF(("%d seconds waiting for exit\n", timeout->tv_sec));
            sleep(1);
            timeout->tv_sec -= 1;
         }
      }
   } while (i!=pid);

   if (status & 0xff) {		/* terminated by signal */
      DEXIT;
      return -1;
   }

   DEXIT;
   return (status&0xff00) >> 8;	/* return exitcode */
}

static void addenv(key, value)
char *key, *value;
{
   char *str;

   DENTER(TOP_LAYER, "addenv");
   str = malloc(strlen(key) + strlen(value) + 2);
   if (!str) {
      DEXIT;
      return;
   }

   strcpy(str, key);
   strcat(str, "=");
   strcat(str, value);
   putenv(str);

   /* there is intentionally no free(str) */

   DEXIT;
   return;
}
