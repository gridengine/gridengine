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
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "sge_conf.h"
#include "mail.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_jobL.h"
#include "sig_handlers.h"
#include "sge_pgrp.h"
#include "sge_me.h"
#include "sge_exit.h"
#include "msg_daemons_common.h"


#if defined(SOLARIS) || defined(ALPHA)
/* ALPHA only has wait3() prototype if _XOPEN_SOURCE_EXTENDED is defined */
pid_t wait3(int *, int, struct rusage *);
#endif

static void sge_send_mail(const char *mailer, int mailer_has_subj_line, 
                          const char *user, const char *host, const char *subj,
                          const char *buf);

/*
** NAME
**   cull_mail
** PARAMETER
**   user_list     -   mail recipients list (MR_Type)
**   subj          -   subject line

** CR: don't localize mail subject, until we send it in Mime format!
**     The message definition is not l10n'ed (no _() macro used)!!!        

**   buf           -   message contents
** RETURN
**   none
** EXTERNAL
**   sge_send_mail
** DESCRIPTION
**   sends a mail to each of the recipients in the list
*/
void cull_mail(lList *user_list, char *subj, char *buf, const char *mail_type) {
   char *mailer;
   int mailer_has_subj_line;
   lListElem *ep;
   const char *user, *host;

   DENTER(TOP_LAYER, "cull_mail");

   mailer = conf.mailer;
   mailer_has_subj_line = 1;

   if (!buf) {
      buf = subj;
   }

   if (user_list) {
      for_each(ep, user_list) {
         user = lGetString(ep, MR_user);
         host = lGetHost(ep, MR_host);
         if (!user && !host) {
            ERROR((SGE_EVENT, MSG_MAIL_EMPTYUSERHOST));
            DEXIT;
            return;
         } else if (!host) {
            INFO((SGE_EVENT, MSG_MAIL_MAILUSER_SSSS, 
                  mail_type, user, mailer, subj ? subj : MSG_MAIL_NOSUBJ));
         } else {
            INFO((SGE_EVENT, MSG_MAIL_MAILUSERHOST_SSSSS, 
                  mail_type, user, host, mailer, 
                  subj ? subj : MSG_MAIL_NOSUBJ));
         }
         sge_send_mail(mailer, mailer_has_subj_line, user, host, subj, buf);
      }
   } 

   DEXIT;
   return;
}

/************************************************************/

static void sge_send_mail(
const char *mailer,
int mailer_has_subj_line,
const char *user,
const char *host,
const char *subj,
const char *buf 
) {
   int pid;
   int pid2;
   int exit_status;
   int pipefds[2];
   FILE *fp;
   stringT user_str;

#if !(defined(HPUX) || defined(HP10_01) || defined(HPCONVEX) || defined(CRAY) || defined(SINIX))
   struct rusage rusage;
#endif

#if defined(SVR3) || defined(_BSD)
   union wait status;
#else
   int status;
#endif

   DENTER(TOP_LAYER, "sge_send_mail");

   if (!user) {
      DEXIT;
      return;
   }

   alarm(0);


   if (fork()) {
      DPRINTF(("PARENT RETURNS\n"));
      DEXIT;
      return;
   }

   DPRINTF(("CHILD CONTINUES\n"));
   SETPGRP;

   sge_close_all_fds(NULL);

   /* 
      may never call SGE_EXIT() here because
      leave_commd() gets called by a the mailer child
      and leave_commd() unregisters the commproc
   */
   if (pipe(pipefds) < 0) {
      ERROR((SGE_EVENT, MSG_MAIL_NOPIPE));
      exit(1);
   }
   if ((pid = fork()) < 0) {
      ERROR((SGE_EVENT, MSG_MAIL_NOFORK));
      exit(1);
   }
   if (!pid) {
      if (host)
         sprintf(user_str, "%s@%s", user, host);
      else
         sprintf(user_str, "%s", user);

      if (dup2(pipefds[0], 0) < 0) {
         CRITICAL((SGE_EVENT, MSG_MAIL_NODUP));
         exit(1);
      }

      close(pipefds[1]);

      if (mailer_has_subj_line) {
         DPRINTF(("%s mail -s %s %s", mailer, subj, user_str));  
         execl(mailer, "mail", "-s", subj, user_str, 0);
      }
      else {
         DPRINTF(("%s mail %s", mailer, user_str));  
         execl(mailer, "mail", user_str, 0);
      }
      CRITICAL((SGE_EVENT, MSG_MAIL_NOEXEC_S, mailer));
      exit(1);
   }

   close(pipefds[0]);
   fp = fdopen(pipefds[1], "w");
   fprintf(fp, "%s\n", buf);
   fclose(fp);

   sge_setup_sig_handlers(me.who);

   while (1) {
      alarm(60);                /* max time to allow for mail */
      sigprocmask(SIG_SETMASK, &io_mask, &omask);
      sigaction(SIGALRM, &sigalrm_vec, &sigalrm_ovec);

#if defined(HPUX) || defined(HP10_01) || defined(HPCONVEX) || defined(CRAY) || defined(SINIX)
      pid2 = waitpid(pid, &status, 0);
#else
      pid2 = wait3(&status, 0, &rusage);
#endif

      alarm(0);
      if (pid2 == 0) {          /* how could this happen? */
         kill(pid, SIGKILL);
         ERROR((SGE_EVENT, MSG_MAIL_NOMAIL1));
         exit(1);
      }

      if (pid2 == -1) {         /* alarm must have went off */
         kill(pid, SIGKILL);
         ERROR((SGE_EVENT, MSG_MAIL_NOMAIL2));
         exit(1);
      }

      if (WIFSTOPPED(status)) { /* how could this happen? */
         kill(pid, SIGKILL);
         ERROR((SGE_EVENT, MSG_MAIL_NOMAIL3_I, WSTOPSIG(status)));
         exit(1);
      }

#if defined(SVR3) || defined(_BSD)
      exit_status = status.w_retcode;
#else
      exit_status = status;
#endif
      DPRINTF(("mailer exited with exit status %d\n", exit_status));
      exit(exit_status);
   }
}
