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
#include "rmon_rmon.h"
#include "rmon_wait_list.h"

wait_list_type *wait_list = NULL;

static int wl_cmp(wait_list_type *w0, wait_list_type *w1);

static int wl_cmp(
wait_list_type *w0,
wait_list_type *w1 
) {
   int temp;

#undef  FUNC
#define FUNC "wl_cmp"
   DENTER;

   if ((temp = strcmp(w0->programname, w1->programname)) != 0) {
      DEXIT;
      return temp;
   }

   DEXIT;

   return (w0->client - w1->client);
}

/**********************************************************/

int rmon_insert_wl(
wait_list_type *new 
) {
   wait_list_type **wlp;
   int wcmp = 1;

#undef FUNC
#define FUNC "rmon_insert_wl"
   DENTER;

   if (!new) {
      DPRINTF(("invalid new wait list element\n"));
      DEXIT;
      return 0;
   }

   /* insert for key client */
   wlp = &wait_list;
   while (*wlp && (wcmp = wl_cmp(new, *wlp)) > 0)
      wlp = &((*wlp)->next);

   if (wcmp == 0) {
      DPRINTF(("List element already exists !\n"));
      DEXIT;
      return 0;
   }

   /* link for client key */
   new->next = *wlp;
   *wlp = new;

   DEXIT;
   return 1;
}                               /* insert_wl */

/**********************************************************/

wait_list_type **rmon_search_wl_for_client_and_spy(
u_long client,
char *programname 
) {
   wait_list_type **wlp, new;
   int wcmp = 1;

#undef  FUNC
#define FUNC "rmon_search_wl_for_client_and_spy"
   DENTER;

   strncpy(new.programname, programname, STRINGSIZE - 1);
   new.client = client;

   wlp = &wait_list;
   while (*wlp && (wcmp = wl_cmp(&new, *wlp)) > 0)
      wlp = &((*wlp)->next);

   DEXIT;
   return (wcmp == 0) ? wlp : NULL;
}

/**********************************************************/

wait_list_type **rmon_search_wl_for_client(
u_long client 
) {
   wait_list_type **wlp;

#undef FUNC
#define FUNC "rmon_search_wl_for_client"
   DENTER;

   wlp = &wait_list;
   while (*wlp && client > (*wlp)->client)
      wlp = &((*wlp)->next);

   DEXIT;
   return *wlp ? ((*wlp)->client == client ? wlp : NULL) : NULL;
}                               /* search_wl_for_client */

/**********************************************************/

wait_list_type **rmon_search_wl_for_spy(
char *programname 
) {
   wait_list_type **wlp;

#undef FUNC
#define FUNC "rmon_search_wl_for_spy"

   DENTER;

   wlp = &wait_list;
   while (*wlp && strcmp(programname, (*wlp)->programname) > 0)
      wlp = &((*wlp)->next);

   DEXIT;
   return *wlp ? (strcmp((*wlp)->programname, programname) == 0 ?
                  wlp : NULL) : NULL;
}                               /* search_wl_for_spy */

/**********************************************************/

int rmon_delete_wl(
wait_list_type **wlp 
) {
   wait_list_type *wl;

#undef FUNC
#define FUNC "rmon_delete_wl"
   DENTER;

   if (!wlp) {
      DPRINTF(("invalid wait list\n"));
      DEXIT;
      return 0;
   }

   while ((wl = *wlp)) {
      *wlp = wl->next;
      free(wl);
   }

   DEXIT;
   return 1;
}                               /* delete_wl */

/**********************************************************/

void rmon_print_wl(
wait_list_type *wl 
) {
#undef FUNC
#define FUNC "rmon_print_wl"
   DENTER;

   if (!wl) {
      DPRINTF(("wait_list: no entry!\n"));
      DEXIT;
      return;
   }

   DPRINTF(("rmon     programname          level\n"));
   while (wl) {
      rmon_print_wait(wl);
      wl = wl->next;
   }

   DEXIT;
}                               /* print_wl */

/**********************************************************/

void rmon_print_wait(
wait_list_type *wl 
) {
   if (!wl) {
      DPRINTF(("wait = NULL !\n"));
      return;
   }

   DPRINTF(("%6d %22s   %3d\n",
            wl->client, wl->programname, wl->level));

}                               /* rmon_print_wait() */

/**********************************************************/

wait_list_type *rmon_unchain_wl(
wait_list_type **wlp 
) {
   wait_list_type *wl;

#undef FUNC
#define FUNC "rmon_unchain_wl"
   DENTER;

   wl = *wlp;
   *wlp = (*wlp)->next;

   wl->next = NULL;

   DEXIT;
   return wl;
}                               /* unchain_wl */
