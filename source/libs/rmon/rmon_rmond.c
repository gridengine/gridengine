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
#define MAINPROGRAM
#define DEBUG

#include "rmon_h.h"
#include "rmon_io.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_restart.h"
#include "rmon_siginit.h"
#include "rmon_request.h"
#include "rmon_daemon.h"
#include "rmon_server.h"

#include "rmon_spy_protocol.h"
#include "rmon_client_number.h"

#include "rmon_job_list.h"
#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_monitoring_level.h"

/* rmond cases */
#include "rmon_m_c_client_register.h"
#include "rmon_m_c_flush.h"
#include "rmon_m_c_mdel.h"
#include "rmon_m_c_mjob.h"
#include "rmon_m_c_monitoring_level.h"
#include "rmon_m_c_mquit.h"
#include "rmon_m_c_mstat.h"
#include "rmon_m_c_spy_register.h"
#include "rmon_m_c_spy_exit.h"
#include "rmon_m_c_mconf.h"

#include "rmon_rmond.h"
#include "msg_rmon.h"

int sfd0;
volatile int shut_me_down = 0, SFD = 0;         /* THIS PUPPY IS CLOSED ON SIGALRM */
u_long port;
u_long all_jobs = 0;
struct servent *sp;
struct sockaddr_in from, s_in;

extern char rmond[];

/***** functions ******************************************************/

int main(int argc, char **argv);
static void terminate(int);
static void shut_er_down(void);

/**********************************************************************/

int main(
int argc,
char **argv 
) {
   int i, sfd, fromlen = sizeof(from);
   struct fd_set readfds;
   char myname[81];

#undef FUNC
#define FUNC "main"
   DOPEN("rmond");
   DENTER;

   myname[0] = '\0';

   /* read configurations file */
   rmon_read_configuration();
   gethostname(myname, 80);

   for (i = 0; i < strlen(myname); i++)
      myname[i] = toupper(myname[i]);

   if (rmon_hostcmp(rmond, myname) != 0)
      rmon_errf(TERMINAL, MSG_RMON_XISNOTTHERMONDHOST_S,
                MSG_RMON_RMONDHOSTISX_S, myname, rmond);

   /* read rmond-port from service */
   if (!(sp = getservbyname(RMOND_SERVICE, "tcp")))
      rmon_errf(TERMINAL, MSG_RMON_XBADSERVICE_S, RMOND_SERVICE);

   port = ntohs(sp->s_port) + 5000;

   DPRINTF(("USING PORT %d\n", port));

   /* init field of client numbers */
   rmon_init_cn();

   /* make monitor to daemon */
   rmon_daemon(rmon_mmayclose);

   /* init signal handlers */
   rmon_init_alarm();
   rmon_init_terminate(terminate);

   rmon_startup_server();

   /* lock restart file, restart and unlock restart file */
   if (!rmon_lock_restart_file()) {
      DPRINTF(("cannot lock restart file\n"));
      rmon_errf(TERMINAL, MSG_RMON_CANTLOCKRESTARTFILE);
   }
   rmon_restart();
   if (!rmon_unlock_restart_file()) {
      DPRINTF(("cannot unlock restart file\n"));
      rmon_errf(TERMINAL, MSG_RMON_CANTUNLOCKRESTARTFILE);
   }

/*** MAIN LOOP ***/
   for (;;) {

      DPRINTF(("\n"));
      DPRINTF(("=====================================\n"));

      /* Clear the Bitmask for the select (coming later) */
      FD_ZERO(&readfds);
      FD_SET(sfd0, &readfds);

      /* check all sockets wether there is one ready for reading. */
      /* When a timeout occurs, 'select' returns a 0. */

      if (select(sfd0 + 1, TYPE_OF_READFDS & readfds, NULL, NULL, NULL) == 0)
         continue;

      if ((sfd = accept(sfd0, (struct sockaddr *) &from, &fromlen)) < 0) {
         if (errno != EINTR)
            rmon_errf(TRIVIAL, MSG_RMON_ACCEPTERRORX_S, sys_errlist[errno]);
         continue;
      }

      /* rmon_shutdown_server( ); */

      if (!rmon_get_request(sfd)) {
         rmon_errf(TRIVIAL, MSG_RMON_ERRORREADINGREQUEST);
         shutdown(sfd, 2);
         close(sfd);
         continue;
      }

      switch (request.type) {
      case MESSAGE_FLUSH:
         DPRINTF(("MESSAGE_FLUSH( %d )\n", request.client_number));
         if (!rmon_m_c_flush(sfd))
            DPRINTF(("ERROR in m_c_flush\n"));
         break;

      case CLIENT_REGISTER:
         DPRINTF(("CLIENT_REGISTER\n"));
         if (!rmon_m_c_client_register(sfd))
            DPRINTF(("ERROR in m_c_client_register\n"));
         break;

      case SPY_REGISTER:
         DPRINTF(("SPY_REGISTER: %s\n", request.programname));
         if (!rmon_m_c_spy_register(sfd))
            DPRINTF(("ERROR in m_c_spy_register\n"));
         break;

      case SPY_EXIT:
         DPRINTF(("SPY_EXIT: %s\n", request.programname));
         if (!rmon_m_c_spy_exit(sfd))
            DPRINTF(("ERROR in m_c_spy_exit\n"));
         break;

      case MONITORING_LEVEL:
         DPRINTF(("MONITORING_LEVEL\n"));
         if (!rmon_m_c_monitoring_level(sfd))
            DPRINTF(("ERROR in m_c_monitoring_level\n"));
         break;

      case MSTAT:
         DPRINTF(("MSTAT\n"));
         if (!rmon_m_c_mstat(sfd))
            DPRINTF(("ERROR in m_c_mstat\n"));
         break;

      case MDEL:
         DPRINTF(("MDEL\n"));
         if (!rmon_m_c_mdel(sfd))
            DPRINTF(("ERROR in m_c_del\n"));
         break;

      case MJOB:
         DPRINTF(("MJOB\n"));
         if (!rmon_m_c_mjob(sfd))
            DPRINTF(("ERROR in m_c_mjob\n"));
         break;

      case MCONF:
         DPRINTF(("MCONF\n"));
         if (!rmon_m_c_mconf(sfd))
            DPRINTF(("ERROR in m_c_mconf\n"));
         break;

      case MQUIT:
         DPRINTF(("MQUIT\n"));
         if (!rmon_m_c_mquit(sfd))
            DPRINTF(("ERROR in m_c_mquit\n"));
         rmon_shutdown_server();
         shutdown(sfd, 2);
         close(sfd);

         DCLOSE;
         exit(0);

      default:
         DPRINTF(("!!! Unknown request type %ld !!!\n", request.type));
         break;
      }                         /* switch */

      shutdown(sfd, 2);
      close(sfd);

      /* rmon_startup_server( ); */
   }                            /* for */
}                               /* main */

/***************************************************************************/

static void terminate(
int n 
) {
#undef  FUNC
#define FUNC "terminate"
   DENTER;

   shut_er_down();

   DEXIT;
}

/***************************************************************************/

static void shut_er_down(void) {
   int i;
   long _nfile;

#undef  FUNC
#define FUNC "shut_er_down"
   DENTER;

   if (((_nfile = sysconf(_SC_OPEN_MAX))) == -1) {
      rmon_errf(TRIVIAL, MSG_RMON_SHUTERDOWNUNABLETOSYSCONFSCOPENMAX);
      DEXIT;
      return;
   }

   for (i = 0; i < _nfile - 1; i++)
      close(i);

   DEXIT;
   DCLOSE;
   exit(0);
}

/* *************************************************************************

   rmon_calculate_moritz_level() computes a combined monitoring 
   level dmlp for a spy given by it's first transition tl

 */

void rmon_calculate_moritz_level(
monitoring_level *dmlp,
transition_list_type *tl 
) {
   spy_list_type *spy;

#undef  FUNC
#define FUNC "rmon_calculate_moritz_level"

   DENTER;

   rmon_mlclr(dmlp);

   for (spy = tl->spy; tl && tl->spy == spy; tl = tl->next_spy)
      rmon_mlor(dmlp, &(tl->level));

   if (job_list || all_jobs)
      rmon_mlset(dmlp, JOBTRACE);

   DEXIT;

   return;
}

/* ************************************************************************* */

int rmon_make_moritz_level()
{
   spy_list_type *sl;
   transition_list_type **stlp;
   monitoring_level new_moritz;
   int changed = 0;

#undef  FUNC
#define FUNC "rmon_make_moritz_level"
   DENTER;

   for (sl = spy_list; sl; sl = sl->next) {
      stlp = rmon_search_tl_for_sl(sl);

      if (!stlp) {
         if (!rmon_mliszero(&sl->moritz_level)) {
            rmon_mlclr(&sl->moritz_level);
            changed = 1;
         }
      }
      else {
         rmon_calculate_moritz_level(&new_moritz, *stlp);
         if (rmon_mlcmp(&(sl->moritz_level), &new_moritz) != 0) {
            rmon_mlcpy(&sl->moritz_level, &new_moritz);
            changed = 1;
         }
      }
   }

   DEXIT;
   return changed;
}

/* ************************************************************************* */
/* return: 0 - job list has not changed
 *         1 - job list has changed
 */
int rmon_make_job_list()
{
   job_list_type *jl, *jl_old, *old_job_list, *new;
   client_list_type *cl;

#undef  FUNC
#define FUNC "rmon_make_job_list"
   DENTER;

   old_job_list = job_list;
   job_list = NULL;
   cl = client_list;
   while (cl) {
      if (cl->all_jobs) {
         all_jobs = 1;
         rmon_delete_jl(&job_list);
         return (old_job_list != job_list);
      }
      cl = cl->next;
   }

   cl = client_list;

   while (cl) {
      jl = cl->job_list;

      while (jl) {
         if (!rmon_search_job_in_jl(jl->jobid, &job_list)) {
            new = (job_list_type *) malloc(sizeof(job_list_type));
            if (!new)
               rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

            new->jobid = jl->jobid;
            new->next = NULL;
            rmon_insert_jl_in_jlp(new, &job_list);
         }

         jl = jl->next;
      }
      cl = cl->next;
   }

   /* has job list changed */
   jl = job_list;
   jl_old = old_job_list;

   while (jl && jl_old && jl->jobid == jl_old->jobid) {
      jl = jl->next;
      jl_old = jl_old->next;
   }

   rmon_delete_jl(&old_job_list);

   DPRINTF(("job_list of rmond:\n"));
   rmon_print_jl(job_list);

   DEXIT;
   return (!jl && !jl_old && !all_jobs) ? 0 : 1;
}

/***************************************************************************/

int rmon_delete_client(
client_list_type *cl 
) {
#undef  FUNC
#define FUNC "rmon_delete_client"
   DENTER;

   if (!cl)
      return 0;

   rmon_delete_tl_by_cl(cl);
   rmon_delete_jl(&cl->job_list);
   rmon_free_cn(cl->client);
   free(cl);

   DEXIT;
   return 1;
}

int rmon_xchg_tl_with_wait_by_sl(
spy_list_type *sl 
) {
   transition_list_type **tlp, *tl;
   wait_list_type *wl;

#undef FUNC
#define FUNC "rmon_xchg_tl_with_wait_by_sl"

   DENTER;

   if (!sl)
      return 0;

   for (tlp = rmon_search_tl_for_sl(sl); tlp && *tlp && (*tlp)->spy == sl; /* made by unchain_tl_by_sl */ ) {

      /* unchain transition element */
      tl = rmon_unchain_tl_by_sl(tlp);

      /* copy transition data in a new wait list element */
      wl = (wait_list_type *) malloc(sizeof(wait_list_type));
      if (!wl) {
         DPRINTF(("ERROR: malloc failure\n"));
         DEXIT;
         return 0;
      }

      strncpy(wl->programname, tl->spy->programname, STRINGSIZE - 1);
      rmon_mlcpy(&wl->level, &tl->level);
      wl->client = tl->client->client;
      wl->next = NULL;

      if (!rmon_insert_wl(wl)) {
         DEXIT;
         return 0;
      }

      free(tl);
   }

   return 1;
}
/***************************************************************************/

int rmon_delete_spy(
spy_list_type *sl 
) {
#undef  FUNC
#define FUNC "rmon_delete_spy"
   DENTER;

   if (!sl)
      return 0;

   rmon_delete_tl_by_sl(sl);
   rmon_delete_ml(&sl->message_list);
   free(sl);

   DEXIT;
   return 1;
}
