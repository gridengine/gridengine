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

#include "rmon_transition_list.h"
#include "rmon_client_list.h"
#include "rmon_spy_list.h"

transition_list_type *first_spy = NULL, *first_client = NULL;

/**********************************************************/
static int cl_cmp(client_list_type *, client_list_type *);
static int client_cmp(transition_list_type *, transition_list_type *);
static int spy_cmp(transition_list_type *, transition_list_type *);
/**********************************************************/
/* compares two spy entities like strcmp() */
/*  0 <  1 : ret < 0
   0 == 1 : ret = 0
   0 >  1 : ret > 0
 */

/**********************************************************/

static int cl_cmp(
client_list_type *c0,
client_list_type *c1 
) {
#undef FUNC
#define FUNC "cl_cmp"

   /* DENTER;
      DEXIT; */
   return (c0->client - c1->client);
}

/**********************************************************/

static int client_cmp(
transition_list_type *t0,
transition_list_type *t1 
) {
   int temp;

#undef FUNC
#define FUNC "client_cmp"

   /* DENTER; */

   if ((temp = cl_cmp(t0->client, t1->client))) {
      /* DEXIT; */
      return temp;
   }

   /* DEXIT; */
   return rmon_sl_cmp(t0->spy, t1->spy);
}

/**********************************************************/

static int spy_cmp(
transition_list_type *t0,
transition_list_type *t1 
) {
   int temp;

#undef FUNC
#define FUNC "spy_cmp"

   /* DENTER; */

   if ((temp = rmon_sl_cmp(t0->spy, t1->spy))) {
      /* DEXIT; */
      return temp;
   }

   /* DEXIT; */
   return cl_cmp(t0->client, t1->client);
}

/**********************************************************/

int rmon_insert_tl(
transition_list_type *new 
) {
   transition_list_type **ctp, **stp;
   int ccmp = 1, scmp = 1;

#undef FUNC
#define FUNC "rmon_insert_tl"
   DENTER;

   if (!new || !new->client || !new->spy) {
      rmon_errf(TRIVIAL, MSG_RMON_INVALIDNEWTRANSITION);
      DEXIT;
      return 0;
   }

   /* seek for key client */
   ctp = &first_client;
   while (*ctp && (ccmp = client_cmp(new, *ctp)) > 0)
      ctp = &((*ctp)->next_client);

   if (ccmp == 0) {
      DPRINTF(("no double entries\n"));
      DEXIT;
      return 0;
   }

   /* seek for key spy */
   stp = &first_spy;
   while (*stp && (scmp = spy_cmp(new, *stp)) > 0)
      stp = &((*stp)->next_spy);

   if (scmp == 0) {
      DPRINTF(("very, very, very heavy error\n"));
      DEXIT;
      return 0;
   }

   /* link for client key */
   new->next_client = *ctp;
   *ctp = new;

   /* link for spy key */
   new->next_spy = *stp;
   *stp = new;

   DEXIT;
   return 1;
}                               /* insert_tl */

/**********************************************************/

transition_list_type **rmon_search_tl_for_cl(
client_list_type *cl 
) {
   int ccmp = 1;
   transition_list_type **ctp;

#undef FUNC
#define FUNC "rmon_search_tl_for_cl"
   DENTER;

   ctp = &first_client;
   while (*ctp && (ccmp = cl_cmp(cl, (*ctp)->client)) > 0)
      ctp = &((*ctp)->next_client);

   DEXIT;
   return (ccmp == 0) ? ctp : NULL;

}                               /* search_tl_for_cl */

/**********************************************************/

transition_list_type **rmon_search_tl_for_sl(
spy_list_type *sl 
) {
   int scmp = 1;
   transition_list_type **stp;

#undef FUNC
#define FUNC "rmon_search_tl_for_sl"
   DENTER;

   stp = &first_spy;

   while (*stp && (scmp = rmon_sl_cmp(sl, (*stp)->spy)) > 0)
      stp = &((*stp)->next_spy);

   DEXIT;
   return (scmp == 0) ? stp : NULL;
}                               /* search_tl_for_sl */

/**********************************************************/

transition_list_type **rmon_search_tl_for_cl_and_sl(
client_list_type *cl,
spy_list_type *sl 
) {
   transition_list_type **stp;
   transition_list_type temp;
   int scmp = 1;

#undef FUNC
#define FUNC "rmon_search_tl_for_cl_and_sl"

   DENTER;

   temp.spy = sl;
   temp.client = cl;

   stp = &first_spy;
   while (*stp && (scmp = spy_cmp(&temp, *stp)) > 0)
      stp = &((*stp)->next_spy);

   DEXIT;
   return scmp == 0 ? stp : NULL;

}                               /* search_tl_for_cl_and_sl */

/**********************************************************/

int rmon_delete_tl(ctl, stl)
transition_list_type **ctl, **stl;
{
   transition_list_type *tl;

#undef FUNC
#define FUNC "rmon_delete_tl"
   DENTER;

   if (!ctl || !stl) {
      rmon_errf(TRIVIAL, MSG_RMON_INVALIDTRANSITIONLIST);
      DEXIT;
      return 0;
   }

   while ((tl = *ctl)) {
      *ctl = tl->next_client;
      free(tl);
   }

   DEXIT;
   return 1;
}                               /* delete_tl */

/**********************************************************/

void rmon_print_tl(
transition_list_type *tl 
) {
#undef FUNC
#define FUNC "rmon_print_tl"
   DENTER;

   if (!tl) {
      DPRINTF(("transition_list = NULL !\n"));
      DEXIT;
      return;
   }

   DPRINTF(("rmon     internet addr   port level last_read\n"));
   while (tl) {
      rmon_print_transition(tl);
      tl = tl->next_client;
   }

   DEXIT;
   return;
}                               /* print_tl */

/**********************************************************/

void rmon_print_transition(
transition_list_type *tl 
) {
   struct in_addr bert;
   char cstr[7];
   char astr[16];
   char pstr[7];

   if (!tl) {
      DPRINTF(("transition = NULL !\n"));
      return;
   }

   if (tl->client)
      sprintf(cstr, "%6ld", tl->client->client);
   else
      strcpy(cstr, " NULL ");

   if (tl->spy) {
      bert.s_addr = htonl(tl->spy->inet_addr);
      sprintf(astr, "%15s", inet_ntoa(bert));
      sprintf(pstr, "%6ld", tl->spy->port);
   }
   else {
      strcpy(astr, "           NULL ");
      strcpy(pstr, "  NULL ");
   }

   if (tl->spy)
      DPRINTF(("%s %s %s   %3d     %5d\n",
               cstr,
               astr,
               pstr,
               rmon_mlgetl(&(tl->level), 0),
               tl->last_read));

   return;
}                               /* print_tl */

/**********************************************************/

transition_list_type *rmon_unchain_tl_by_cl(
transition_list_type **ctlp 
) {
   transition_list_type *tl;
   transition_list_type **stlp;

#undef FUNC
#define FUNC "rmon_unchain_tl_by_cl"
   DENTER;

   tl = *ctlp;

   stlp = &first_spy;
   while (*stlp != tl)
      stlp = &((*stlp)->next_spy);

   *stlp = tl->next_spy;
   *ctlp = tl->next_client;

   DEXIT;
   return tl;
}                               /* unchain_tl_by_cl */

/**********************************************************/

transition_list_type *rmon_unchain_tl_by_sl(
transition_list_type **stlp 
) {
   transition_list_type *tl;
   transition_list_type **ctlp;

#undef FUNC
#define FUNC "rmon_unchain_tl_by_sl"
   DENTER;

   tl = *stlp;

   ctlp = &first_client;
   while (*ctlp != tl)
      ctlp = &((*ctlp)->next_client);

   *stlp = tl->next_spy;
   *ctlp = tl->next_client;

   DEXIT;
   return tl;
}                               /* unchain_tl_by_sl */

/**********************************************************/

int rmon_delete_tl_by_sl(
spy_list_type *sl 
) {
   int ret = 0;
   transition_list_type **tlp;

#undef FUNC
#define FUNC "rmon_delete_tl_by_sl"
   DENTER;

   if (sl) {

      tlp = rmon_search_tl_for_sl(sl);
      if (tlp) {
         while (*tlp && (*tlp)->spy == sl)
            free(rmon_unchain_tl_by_sl(tlp));
      }

      ret = 1;
   }

   DEXIT;
   return ret;
}                               /* rmon_delete_tl_by_sl() */

/**********************************************************/

int rmon_delete_tl_by_cl(
client_list_type *cl 
) {
   int ret = 0;
   transition_list_type **tlp;

#undef FUNC
#define FUNC "rmon_delete_tl_by_cl"
   DENTER;

   if (cl) {
      tlp = rmon_search_tl_for_cl(cl);
      if (tlp) {
         while (*tlp && (*tlp)->client == cl)
            free(rmon_unchain_tl_by_cl(tlp));
      }

      ret = 1;
   }

   DEXIT;
   return ret;
}                               /* rmon_delete_tl_by_cl() */
