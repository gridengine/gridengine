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
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_m_c_flush.h"
#include "rmon_rmond.h"

#include "rmon_spy_list.h"
#include "rmon_client_list.h"
#include "rmon_message_list.h"
#include "rmon_transition_list.h"
#include "rmon_monitoring_level.h"
#include "rmon_message_protocol.h"
/****** global variables ******************************************/

u_long now = 0;

/****** intern prototypes ****************************************/

static int send_for_transition(int sfd, transition_list_type *tl);
static void update_transition(transition_list_type *tl);
static int q_send_message_list(int sfd, message_list_type *ml, u_long addr, u_long port, u_long childdeath, monitoring_level *level, client_list_type *cl);

/*****************************************************************/

int rmon_m_c_flush(
int sfd 
) {
   client_list_type *cl, **clp;
   spy_list_type *sl, **slp;
   transition_list_type *tl, **tlp;
   int n;

#undef  FUNC
#define FUNC "rmon_m_c_flush"
   DENTER;

   /* search client in client_list */
   if (!(clp = rmon_search_client(request.client_number))) {
      DPRINTF(("unknown rmon !\n"));
      if (!rmon_send_ack(sfd, S_UNKNOWN_CLIENT)) {
         DEXIT;
         return 0;
      }
      DEXIT;
      return 1;
   }
   cl = *clp;

   /* are there assoziated spys ? */
   if (!(tlp = rmon_search_tl_for_cl(cl))) {
      /* send header with number=0 of spys */
      request.n_spy = 0;

      if (!rmon_send_ack(sfd, S_STILL_WAITING)) {
         DEXIT;
         return 0;
      }
      DEXIT;
      return 1;
   }

   /* count associated spys with counter n */
   for (n = 0, tl = *tlp; tl && tl->client == cl; tl = tl->next_client, n++);

   /* send header with number of spys */
   request.n_spy = n;
   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      DEXIT;
      return 0;
   }

   /* send messages for each spy */
   for (tl = *tlp; tl && tl->client == cl; tl = tl->next_client) {
      if (!send_for_transition(sfd, tl)) {
         DEXIT;
         return 0;
      }
   }

   if (!rmon_get_ack(sfd)) {
      DEXIT;
      return 0;
   }

   /* update each spy */
   for (tl = *tlp; tl && tl->client == cl; tl = tl->next_client)
      update_transition(tl);

   /* ------------------------------ */
   /* End of Communication to Client */
   /* ------------------------------ */

   DPRINTF(("-------------------------------------\n"));

   /* check childdeath of each associated spy */
   for (tl = *tlp; tl && tl->client == cl; tl = tl->next_client) {
      sl = tl->spy;

      if (sl->childdeath && !sl->message_list) {

         if (!(slp = rmon_search_spy(sl->programname)) || !*slp) {
            DPRINTF(("very, very corrupt data structures\n"));
            DEXIT;
            exit(-1);
         }
         if (!rmon_xchg_tl_with_wait_by_sl(rmon_unchain_sl(slp))) {
            DPRINTF(("very, very corrupt data structures\n"));
            DEXIT;
            exit(-1);
         }
      }
   }

   DPRINTF(("-------------------------------------\n"));

   /* send flushes for spys */
   for (tl = *tlp; tl && tl->client == cl; tl = tl->next_client) {

      sl = tl->spy;

      if (sl->last_flush <= cl->last_flush && !sl->childdeath) {
         DPRINTF(("flushing to:\n"));
         rmon_print_spy(sl);

         if (!rmon_flush_spy(sl)) {
            if (errval == ECONNREFUSED)
               sl->childdeath = 1;
            DPRINTF(("ERROR in flush_spy !\n"));
            DEXIT;
            return 0;
         }
      }
   }                            /* for */

   cl->last_flush = now++;

   DEXIT;
   return 1;
}                               /* rmon_m_c_flush() */

/*****************************************************************/

/* send for transition tl */
static int send_for_transition(
int sfd,
transition_list_type *tl 
) {
   int i;

   spy_list_type *sl;
   message_list_type *ml;

#undef FUNC
#define FUNC "send_for_transition"
   DENTER;

   /* select spy */
   sl = tl->spy;

   /* find first message to send to client */
   ml = sl->message_list;
   for (i = sl->first; i <= tl->last_read; i++)
      ml = ml->next;

   if (!q_send_message_list(sfd, ml, sl->inet_addr, sl->port, sl->childdeath, &(tl->level), tl->client))
      return 0;

   DEXIT;
   return 1;
}                               /* send_for_transition */

/*****************************************************************/

static void update_transition(
transition_list_type *tl 
) {
   spy_list_type *sl;
   message_list_type *ml;
   transition_list_type **stlp, *stl;
   u_long min = (u_long) - 1;

#undef FUNC
#define FUNC "update_transition"

   DENTER;

   /* select spy */
   sl = tl->spy;

   /* update last_read */
   tl->last_read = sl->last;

   /* remove all message-list-entries which are no longer wanted by anyone */
   if (!(stlp = rmon_search_tl_for_sl(sl))) {
      DPRINTF(("corrupt data structure\n"));
      DEXIT;
      exit(0);
   }

   /* get minimum index of last read values */
   for (stl = *stlp; stl && stl->spy == sl; stl = stl->next_spy)
      min = (min <= stl->last_read) ? min : stl->last_read;

   /* delete obsolete messages */
   while (sl->first <= min) {
      ml = sl->message_list;
      sl->message_list = ml->next;
      free(ml);

      sl->first++;
   }

   /* reinitiailize last_read of transitions */
   if (sl->message_list == NULL) {
      sl->first = 1;
      sl->last = 0;
      stl = *stlp;
      while (stl && stl->spy == sl) {
         stl->last_read = 0;
         stl = stl->next_spy;
      }
   }

   DEXIT;
   return;

}                               /* update_transition() */

/*****************************************************************/

static int q_send_message_list(
int sfd,
message_list_type *ml,
u_long addr,
u_long port,
u_long childdeath,
monitoring_level *levelp,
client_list_type *cl 
) {
   int n = 0;
   int condition;
   monitoring_level tml, no_jobtrace, jobtrace;

   /* initialize 'constant' for no jobtracing */
   rmon_mlclr(&no_jobtrace);
   rmon_mlset(&no_jobtrace, ~JOBTRACE);

   /* initialize 'constant' for jobtracing */
   rmon_mlclr(&jobtrace);
   rmon_mlset(&jobtrace, JOBTRACE);

#undef FUNC
#define FUNC "q_send_message_list"
   DENTER;

/* ----------------------------------------------------- 

   if the message is needed because of its regular
   level ( and not because of jobtracing ) then

   send it

   ----------------------------------------------------- 

   if (   ( (ml->level & ~JOBTRACE) & level)    
   conition = 1;                                                                        

   ----------------------------------------------------- */

   while (ml) {
      condition = 0;

      if (condition == 0) {
         /* put ml of msg in accu */
         rmon_mlcpy(&tml, &(ml->level));

         /* clear jobtrace bit if set */
         rmon_mland(&tml, &no_jobtrace);

         /* message sent because of actual level ? */
         condition = rmon_mland(&tml, levelp);
      }

/* ----------------------------------------------------- 

   if ( the message is a jobtracing message ) and
   ( (every job message is needed ) or 
   ( this jobtracing message is needed )) then

   send it

   ----------------------------------------------------- 

   if (                 ((ml->level & JOBTRACE)                                                                                 
   && (         cl->all_jobs                                                                                            
   || rmon_search_job_in_jl( ml->jobid, &cl->job_list )))  
   condition = 1;                                                                                                               

   ----------------------------------------------------- */

      if (condition == 0) {
         /* put ml of msg in accu */
         rmon_mlcpy(&tml, &(ml->level));

         if (rmon_mland(&tml, &jobtrace))
            if (cl->all_jobs || rmon_search_job_in_jl(ml->jobid, &cl->job_list))
               condition = 1;
      }

/* 
   if (   ( (ml->level & ~JOBTRACE) & level)
   || ( (ml->level & JOBTRACE) && cl->all_jobs)
   || ( (ml->level & JOBTRACE) && rmon_search_job_in_jl( ml->jobid, &cl->job_list ))) { */

      if (condition) {
         if (!rmon_send_message(sfd, ml)) {
            DEXIT;
            return 0;
         }
         n++;
      }
      ml = ml->next;
   }

   /* add # of messages to  eol */
   eol.n = n;
   eol.port = port;
   eol.inet_addr = addr;

   if (!rmon_send_eol(sfd)) {
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}

/* ************************************************************** */

int rmon_flush_spy(
spy_list_type *sl 
) {
   int sfd;
   message_list_type **mlp;
   int i;

#undef FUNC
#define FUNC "rmon_flush_spy"
   /* DENTER; */

   /* --------------------------------------- */
   /* Communication to Spy                    */
   /* --------------------------------------- */

   if (!(i = rmon_connect_anyone(&sfd, MESSAGE_FLUSH, sl->inet_addr, sl->port)))
      return 0;

   if (i != S_ACCEPTED) {
      shutdown(sfd, 2);
      close(sfd);
      return 0;
   }

   /* go to last element of message_list */
   mlp = &sl->message_list;
   while (*mlp)
      mlp = &((*mlp)->next);

   if (!rmon_receive_message_list(sfd, mlp)) {
      shutdown(sfd, 2);
      close(sfd);
      return 0;
   }
   sl->last += eol.n;

   DPRINTF(("gelesen: %d\n", eol.n));

   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      shutdown(sfd, 2);
      close(sfd);
      return 0;
   }

   sl->last_flush = now++;

   shutdown(sfd, 2);
   close(sfd);

   DEXIT;
   return 1;
}                               /* rmon_flush_spy() */
