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
#include "rmon_request.h"
#include "rmon_client_number.h"

#include "rmon_job_list.h"
#include "rmon_job_protocol.h"

#include "rmon_spy_list.h"
#include "rmon_monitoring_level.h"

#include "rmon_transition_list.h"

#include "rmon_m_c_mjob.h"
#include "rmon_m_c_monitoring_level.h"

#include "rmon_rmond.h"
#include "msg_rmon.h"
extern u_long all_jobs;

/*****************************************************************/

int rmon_m_c_mjob(
int sfd 
) {
   int n;
   u_long clientnumber, flag = 0;
   int n_add, n_delete;
   job_list_type *to_add = NULL;
   job_list_type *to_delete = NULL;
   job_list_type *old_job_list, **jlp, *jl;
   spy_list_type *sl;
   transition_list_type **tlp, *tl;
   client_list_type **clp, *cl, *xcl;
   u_long all, old_all_jobs;
   monitoring_level ml;

#undef  FUNC
#define FUNC "rmon_m_c_mjob"

   DENTER;

   DPRINTF(("rmon_number = %d\n", request.client_number));

   /* is the clientnumber valid */
   if (!client_number[clientnumber = request.client_number]) {
      rmon_send_ack(sfd, S_UNKNOWN_CLIENT);
      DEXIT;
      return 0;
   }

   n_add = request.n_add;
   n_delete = request.n_delete;
   all = request.kind;

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   for (n = 0; !rmon_receive_job_list(sfd, &to_add, n_add) && n < MAX_TRY; n++);
   if (n == MAX_TRY) {
      printf(MSG_RMON_CANTRECEIVEJOBLISTTOADD);
      DEXIT;
      return 0;
   }
   /* DPRINTF(("job list to add received\n")); */

   for (n = 0; !rmon_receive_job_list(sfd, &to_delete, n_delete) && n < MAX_TRY; n++);
   if (n == MAX_TRY) {
      printf(MSG_RMON_CANTRECEIVEJOBLISTTODELETE);
      DEXIT;
      return 0;
   }
   /* DPRINTF(("job list to delete received\n")); */

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   shutdown(sfd, 2);
   close(sfd);

   /* *********************************************************** */

   if (!(clp = rmon_search_client(clientnumber)))
      rmon_errf(TERMINAL,  MSG_RMON_CORRUPTDATASTRUCTURES );

   cl = *clp;
   old_job_list = cl->job_list;
   old_all_jobs = cl->all_jobs;

   if (all == ADD_ALL) {
      all_jobs = 1;
      DPRINTF(("ADD_ALL!\n"));
      cl->all_jobs = 1;
      rmon_delete_jl(&cl->job_list);
      rmon_delete_jl(&to_add);
      rmon_delete_jl(&to_delete);
   }

   if (all == DEL_ALL) {
      xcl = client_list;
      while (xcl) {
         if (xcl->all_jobs) {
            flag = 1;
            break;
         }
         xcl = xcl->next;
      }
      if (flag)
         all_jobs = 0;
      DPRINTF(("DEL_ALL!\n"));
      cl->all_jobs = 0;
      rmon_delete_jl(&cl->job_list);
      rmon_delete_jl(&to_add);
      rmon_delete_jl(&to_delete);
   }

   if (cl->all_jobs) {
      rmon_delete_jl(&to_add);
      rmon_delete_jl(&to_delete);
   }

   while (to_add && !cl->all_jobs) {
      /* insert job element in job list of client */
      jl = rmon_unchain_jl(&to_add);
      if (!rmon_insert_jl_in_jlp(jl, &(cl->job_list))) {
         DPRINTF(("cannot insert job %d for rmon %d\n", jl->jobid, clientnumber));
         free(jl);
      }
      else
         DPRINTF(("job %d inserted for rmon %d\n", jl->jobid, clientnumber));
   }

   while (to_delete && !cl->all_jobs) {
      /* delete job element in job list of client */
      if (!(jlp = rmon_search_job_in_jl(to_delete->jobid, &(cl->job_list))))
         DPRINTF(("cannot delete job %d for rmon %d\n", to_delete->jobid, clientnumber));
      else {
         DPRINTF(("deleting job %d for rmon %d\n", (*jlp)->jobid, clientnumber));
         free(rmon_unchain_jl(jlp));
      }
      free(rmon_unchain_jl(&to_delete));
   }

   if (to_add)
      DPRINTF(("to_add != NULL\n"));
   if (to_delete)
      DPRINTF(("to_delete != NULL\n"));

   DPRINTF(("jobs of rmon %d:\n", clientnumber));
   rmon_print_jl(cl->job_list);

   /* build up new transition if neccessary */
   if ((old_job_list == NULL && cl->job_list != NULL)
       || (old_all_jobs == 0 && cl->all_jobs == 1)) {

      DPRINTF(("build up transitions\n"));
      for (sl = spy_list; sl; sl = sl->next)
         if ((tlp = rmon_search_tl_for_cl_and_sl(cl, sl)))
            rmon_mlset(&((*tlp)->level), JOBTRACE);
         else {
            tl = (transition_list_type *) malloc(sizeof(transition_list_type));
            if (!tl)
               rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED );

            tl->last_read = 0;
            rmon_mlclr(&(tl->level));
            rmon_mlset(&(tl->level), JOBTRACE);
            tl->client = cl;
            tl->spy = sl;
            tl->next_client = NULL;
            tl->next_spy = NULL;

            if (!rmon_insert_tl(tl))
               rmon_errf(TERMINAL, MSG_RMON_CORRUPTDATASTRUCTURES);
         }

   }

   /* delete old transitions if neccessary */
   if ((old_job_list != NULL && cl->job_list == NULL)
       || (old_all_jobs == 1 && cl->all_jobs == 0)) {

      DPRINTF(("remove transitions\n"));
      for (sl = spy_list; sl; sl = sl->next) {
         rmon_print_tl(first_client);

         if (!(tlp = rmon_search_tl_for_cl_and_sl(cl, sl)))
            rmon_errf(TERMINAL, MSG_RMON_CORRUPTDATASTRUCTURES);
         else {
            rmon_mlclr(&ml);
            rmon_mlset(&ml, ~JOBTRACE);
            if (rmon_mland(&((*tlp)->level), &ml) == 0)
               /* delete transition */
               free(rmon_unchain_tl_by_sl(tlp));
         }
/*                              if ( !((*tlp)->level &= ~JOBTRACE) )
   delete transition  
   free( rmon_unchain_tl_by_sl( tlp )); */
      }
   }

   /* make new global job list */
   if (rmon_make_job_list() == 1 || (old_all_jobs != cl->all_jobs)) {
      rmon_make_moritz_level();
      for (sl = spy_list; sl; sl = sl->next) {
         rmon_send_monitoring_level_to_spy(sl);
      }
   }

   DEXIT;
   return 1;
}                               /* rmon_m_c_mjob() */
