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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_rmond.h"
#include "rmon_request.h"
#include "rmon_connect.h"

#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_job_protocol.h"
#include "rmon_monitoring_level.h"

#include "rmon_m_c_monitoring_level.h"

/*****************************************************************/
static int make_or_alter_transition(spy_list_type *sl, client_list_type *cl, u_long *status);

static int make_or_alter_wait(char *programname, u_long client, u_long *status);

extern u_long all_jobs;

/* *************************************************************** */

int rmon_m_c_monitoring_level(
int sfd 
) {
   spy_list_type **slp;
   client_list_type **clp;
   transition_list_type **tlp;

   u_long status;
   monitoring_level new_moritz_level;

   int j, reached, second_sfd;

#undef  FUNC
#define FUNC "rmon_m_c_monitoring_level"
   DENTER;

   /* check boundaries of received monitoring_level */
   /* if ( !mlcheck( &request.level ) ) { --- not implemented 
      if( !rmon_send_ack( sfd, S_ILLEGAL_LEVEL )) { DEXIT; return 0; }
      DEXIT;
      return 1;
      } --- see below --- */

   for (j = 0; j < rmon_mlnlayer(); j++) {
      if (rmon_mlgetl(&request.level, j) >= JOBTRACE) {
         if (!rmon_send_ack(sfd, S_ILLEGAL_LEVEL)) {
            DEXIT;
            return 0;
         }
         DEXIT;
         return 1;
      }
   }

   /* is this a registered client ? */
   if (!(clp = rmon_search_client(request.client_number))) {
      if (!rmon_send_ack(sfd, S_UNKNOWN_CLIENT)) {
         DEXIT;
         return 0;
      }
      return 1;
   }

   /* is that spy registered ? */
   if (!(slp = rmon_search_spy(request.programname))) {

      if (!request.wait) {

         /* wake up possibly sleeping spy */
         reached = rmon_connect_anyone(&second_sfd, WAKE_UP, request.inet_addr, request.port);

         if (!reached) {
            if (!rmon_send_ack(sfd, S_UNKNOWN_SPY)) {
               DEXIT;
               return 0;
            }
            DEXIT;
            return 1;
         }
         else {
            shutdown(second_sfd, 2);
            close(second_sfd);
         }
      }

      if (!make_or_alter_wait(request.programname, request.client_number, &status)) {
         DEXIT;
         return 0;
      }

      if (!rmon_send_ack(sfd, status)) {
         DEXIT;
         return 0;
      }

      DEXIT;
      return 1;

   }                            /* if spy not registered */

   if (!make_or_alter_transition(*slp, *clp, &status)) {
      DEXIT;
      return 0;
   }

   if (!rmon_send_ack(sfd, status)) {
      DEXIT;
      return 0;
   }

   /* --------------------------------------- */
   /* Communication to Spy                    */
   /* --------------------------------------- */

   rmon_print_sl(spy_list);

   /* compute combined monitoring level for this spy */
   if ((tlp = rmon_search_tl_for_sl(*slp)))
      rmon_calculate_moritz_level(&new_moritz_level, *tlp);
   else
      rmon_mlclr(&new_moritz_level);

   /* has the monitoring level changed */
   if (rmon_mlcmp(&((*slp)->moritz_level), &new_moritz_level) == 0) {

      /* no, nothing more to do */
      DEXIT;
      return 1;
   }

   /* store new monitoring level of this spy */
   rmon_mlcpy(&((*slp)->moritz_level), &new_moritz_level);

   if (!rmon_send_monitoring_level_to_spy(*slp)) {
      DEXIT;
      return 0;
   }

   return 1;
}

/* ************************************************************ */

static int make_or_alter_transition(
spy_list_type *sl,
client_list_type *cl,
u_long *status 
) {
   transition_list_type **tlp, *tl;

   if (cl->job_list || cl->all_jobs)
      rmon_mlset(&request.level, JOBTRACE);

   /* new transition list element or ... */
   if (!(tlp = rmon_search_tl_for_cl_and_sl(cl, sl))) {

      DPRINTF(("making new transition list element\n"));
      tl = (transition_list_type *) malloc(sizeof(transition_list_type));
      if (!tl) {
         rmon_errf(TRIVIAL, MSG_RMON_MALLOCFAILED);
         *status = S_NOT_ACCEPTED;
         DEXIT;
         return 0;
      }

      tl->last_read = sl->first > 0 ? sl->first - 1 : 0;
      rmon_mlcpy(&tl->level, &request.level);
      tl->client = cl;
      tl->spy = sl;
      tl->next_client = NULL;
      tl->next_spy = NULL;

      if (!rmon_insert_tl(tl)) {
         DEXIT;
         return 0;
      }

      *status = S_NEW_TRANSITION;
   }
   else {
      /* ... alter transition list element or ... */
      if (!rmon_mliszero(&request.level)) {
         DPRINTF(("altering transition element\n"));
         rmon_mlcpy(&(*tlp)->level, &request.level);
         *status = S_ALTER_TRANSITION;
      }
      else {
         /* ... delete transition list element */
         DPRINTF(("deleting transition element\n"));
         free(rmon_unchain_tl_by_sl(tlp));

         *status = S_DELETE_TRANSITION;
      }
   }
   return 1;
}

/* ************************************************************ */

static int make_or_alter_wait(
char *programname,
u_long client,
u_long *status 
) {
   wait_list_type **wlp, *wl;

   /* is the client still waiting */
   if (!(wlp = rmon_search_wl_for_client_and_spy(client, programname))) {

      DPRINTF(("making new wait list element\n"));
      wl = (wait_list_type *) malloc(sizeof(wait_list_type));
      if (!wl) {
         DPRINTF(("ERROR: malloc failure\n"));
         *status = S_NOT_ACCEPTED;
         DEXIT;
         return 0;
      }

      strncpy(wl->programname, programname, STRINGSIZE - 1);
      rmon_mlcpy(&(wl->level), &request.level);
      wl->client = client;
      wl->next = NULL;

      if (!rmon_insert_wl(wl)) {
         DEXIT;
         return 0;
      }

      *status = S_NEW_WAIT;
   }
   else {
      /* ... alter wait list element or ... */
      if (!rmon_mliszero(&request.level)) {
         DPRINTF(("altering list element\n"));
         rmon_mlcpy(&(*wlp)->level, &request.level);
         *status = S_ALTER_WAIT;
      }
      else {
         /* ... delete wait list element */
         DPRINTF(("deleting list element\n"));
         wl = *wlp;
         free(rmon_unchain_wl(wlp));

         *status = S_DELETE_WAIT;
      }
   }
   return 1;
}

/* ************************************************************ */

int rmon_send_monitoring_level_to_spy(
spy_list_type *sl 
) {
   int n_job = 0, i, sfd;
   job_list_type *jl;

#undef FUNC
#define FUNC "rmon_send_monitoring_level_to_spy"
   DENTER;

   /* Count number of job_list_entries */
   for (jl = job_list; jl; jl = jl->next, n_job++);

   rmon_mlcpy(&request.level, &(sl->moritz_level));
   request.kind = all_jobs;
   request.n_job = n_job;

   DPRINTF(("All-Jobs = %ld\n", all_jobs));
   if (!rmon_connect_anyone(&sfd, MONITORING_LEVEL, sl->inet_addr, sl->port)) {
      DPRINTF(("Can't establish connection.\n"));
      DEXIT;
      return 0;
   }

   /* send list of jobs */
   if (!rmon_send_job_list(sfd, job_list)) {
      shutdown(sfd, 2);
      close(sfd);
      rmon_errf(TRIVIAL, MSG_RMON_CANTSENDJOBLIST );
      DEXIT;
      return 0;
   }

   DPRINTF(("new job_list sent\n"));

   if (!(i = rmon_get_ack(sfd))) {
      DEXIT;
      return 0;
   }
   shutdown(sfd, 2);
   close(sfd);

   if (i != S_ACCEPTED) {
      rmon_errf(TRIVIAL, MSG_RMON_UNEXPECTEDREQUESTSTATUSX_D, request.status);
      DEXIT;
      return 0;
   }

   sl->childdeath = request.childdeath;

   DEXIT;
   return 1;
}
