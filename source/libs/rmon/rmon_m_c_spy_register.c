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
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_conf.h"
#include "rmon_rmon.h"
#include "rmon_request.h"

#include "rmon_rmond.h"
#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_m_c_monitoring_level.h"
#include "rmon_m_c_spy_register.h"
#include "msg_rmon.h"

extern u_long max_messages;
extern u_long all_jobs;
/*****************************************************************/

int rmon_m_c_spy_register(
int sfd 
) {
   spy_list_type *new;
   wait_list_type **wlp, *wltemp = NULL, *wl, *t;
   client_list_type **clp, *cl;
   transition_list_type **tlp, *tl;

#undef  FUNC
#define FUNC "rmon_m_c_spy_register"
   DENTER;

   /* 0. First we check wether this spy is still                   */
   /*    registered.                                                                       */

   if (rmon_search_spy(request.programname)) {
      DPRINTF(("spy tries to register twice\n"));
      rmon_send_ack(sfd, S_SPY_EXISTS);
      DEXIT;
      return 0;
   }

   new = (spy_list_type *) malloc(sizeof(spy_list_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   new->uid = request.uid;
   new->last_flush = 0;
   new->childdeath = 0;
   rmon_mlclr(&(new->moritz_level));
   if (job_list || all_jobs)
      rmon_mlset(&(new->moritz_level), JOBTRACE);
   new->port = request.port;
   new->inet_addr = request.inet_addr;
   new->first = 1;
   new->last = 0;
   new->message_list = NULL;
   new->next = NULL;
   strcpy(new->programname, request.programname);

   /* 1. Copy all affected wait_list_entries in a temporary wait list ( wltemp ) and       */
   /*    unchain them from the original wait list.                                                                         */

   wlp = &wait_list;
   while (*wlp) {

      if (strcmp((*wlp)->programname, new->programname) == 0) {
         wl = rmon_unchain_wl(wlp);
         wl->next = wltemp;
         wltemp = wl;
         rmon_mlor(&(new->moritz_level), &(wl->level));
      }
      else
         wlp = &((*wlp)->next);
   }

   /* 3. If we weren't able to send the ack to the Spy, we have to chain the       */
   /*    temporarily stored wait list elements back into the original wait list.           */

   request.conf_type = MAX_MESSAGES;
   request.conf_value = max_messages;
   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      while (wltemp) {
         rmon_insert_wl(wltemp);
         wltemp = wltemp->next;
      }
      free(new);

      DEXIT;
      return 0;
   }

   /* 4. If we were able to send the ack to the Spy, we have to create a           */
   /*    transition list entry for each temporarily stored                                 */
   /*    wait list element.                                                                                                                        */

   while (wltemp) {

      /* Seek for client entry belonging to this wait list entry */
      if (!(clp = rmon_search_client(wltemp->client))) {
         DPRINTF(("corrupt data structures\n"));
         DEXIT;
         return 0;
      }

      /* Make a new transition_list_entry for every waiting client */
      tl = (transition_list_type *) malloc(sizeof(transition_list_type));

      if (!tl)
         rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

      rmon_mlcpy(&(tl->level), &(wltemp->level));
      tl->last_read = 0;
      tl->client = *clp;
      tl->spy = new;
      tl->next_client = NULL;
      tl->next_spy = NULL;

      rmon_print_transition(tl);

      if (!rmon_insert_tl(tl)) {
         DEXIT;
         return 0;
      }

      /* All information is copied. Destroy the temporary wait_list_entry ! */
      t = wltemp;
      wltemp = wltemp->next;
      free(t);
   }

   /* 5. if there is jobtracing: build up transitions with each                    */
   /*    client that traces jobs                                                   */

   if (job_list || all_jobs) {

      for (cl = client_list; cl; cl = cl->next)
         if (cl->job_list || cl->all_jobs) {
            if ((tlp = rmon_search_tl_for_cl_and_sl(cl, new)))
               rmon_mlset(&((*tlp)->level), JOBTRACE);
            else {
               /* Make a new transition_list_entry for every waiting client */
               tl = (transition_list_type *) malloc(sizeof(transition_list_type));

               if (!tl)
                  rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

               rmon_mlclr(&(tl->level));
               rmon_mlset(&(tl->level), JOBTRACE);
               tl->last_read = 0;
               tl->client = cl;
               tl->spy = new;
               tl->next_client = NULL;
               tl->next_spy = NULL;

               rmon_print_transition(tl);
               if (!rmon_insert_tl(tl)) {
                  DEXIT;
                  return 0;
               }
            }                   /* else */
         }                      /* if */
   }                            /* if */

   /* 6. If we succeeded in all this things, we must insert the new spy list entry */
   /*    into the spy list.                                                                                                                */

   if (!rmon_insert_sl(new)) {
      DEXIT;
      return 0;
   }

   /* 2. Send the calculated moritz_level to the Spy.                                                                      */
   if (!rmon_send_monitoring_level_to_spy(new))
      rmon_xchg_tl_with_wait_by_sl(rmon_unchain_sl(rmon_search_spy(new->programname)));

   /* 7. All is done. Return !                                                                                                             */

   DEXIT;
   return 1;
}
