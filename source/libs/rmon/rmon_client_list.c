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
#include "rmon_client_list.h"
#include "rmon_rmon.h"

client_list_type *client_list = NULL;

int rmon_insert_cl(
client_list_type *new 
) {
   client_list_type **clp;

#undef FUNC
#define FUNC "rmon_insert_cl"
   DENTER;

   if (!new) {
      DPRINTF(("invalid new rmon\n"));
      DEXIT;
      return 0;
   }

   /* insert for key client */
   clp = &client_list;
   while (*clp && new->client > (*clp)->client)
      clp = &((*clp)->next);

   if (*clp && (*clp)->client == new->client) {
      DPRINTF(("no double entries\n"));
      DEXIT;
      return 0;
   }

   /* link for client key */
   new->next = *clp;
   *clp = new;

   DEXIT;
   return 1;
}                               /* rmon_insert_cl() */

/**********************************************************/

client_list_type **rmon_search_client(
u_long client 
) {
   client_list_type **clp;

#undef FUNC
#define FUNC "rmon_search_client"
   DENTER;

   clp = &client_list;
   while (*clp && client > (*clp)->client)
      clp = &((*clp)->next);

   DEXIT;
   return *clp ? ((*clp)->client == client ? clp : NULL) : NULL;
}                               /* search_client */

/**********************************************************/

int rmon_delete_cl(
client_list_type **clp 
) {
   client_list_type *cl;

#undef FUNC
#define FUNC "rmon_delete_cl"

   DENTER;

   if (!clp) {
      DPRINTF(("invalid rmon list\n"));
      DEXIT;
      return 0;
   }

   DPRINTF(("deleting rmon:\n"));
   while (*clp) {
      rmon_print_client(*clp);
      cl = rmon_unchain_cl(clp);
      rmon_delete_jl(&cl->job_list);
      free(cl);
   }

   DEXIT;
   return 1;
}                               /* delete_cl */

/**********************************************************/

void rmon_print_cl(
client_list_type *cl 
) {
#undef FUNC
#define FUNC "rmon_print_cl"

   DENTER;

   if (!cl) {
      DPRINTF(("client_list = NULL !\n"));
      DEXIT;
   }

   DPRINTF(("rmon last_flush\n"));
   while (cl) {
      rmon_print_client(cl);
      cl = cl->next;
   }

   DEXIT;
   return;
}                               /* print_cl */

/**********************************************************/

void rmon_print_client(
client_list_type *cl 
) {
   if (!cl) {
      DPRINTF(("rmon = NULL !\n"));
      return;
   }

   DPRINTF(("%6d     %6d\n", cl->client, cl->last_flush));

   return;
}                               /* rmon_print_client() */

/**********************************************************/

client_list_type *rmon_unchain_cl(
client_list_type **clp 
) {
   client_list_type *cl;

#undef FUNC
#define FUNC "rmon_unchain_cl"

   DENTER;
   cl = *clp;
   *clp = (*clp)->next;
   cl->next = NULL;

   DEXIT;
   return cl;
}                               /* unchain_cl */
