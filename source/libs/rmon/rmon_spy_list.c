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
#include "rmon_spy_list.h"
#include "rmon_message_list.h"

spy_list_type *spy_list = NULL;

int rmon_sl_cmp(
spy_list_type *s0,
spy_list_type *s1 
) {

#undef FUNC
#define FUNC "rmon_sl_cmp"

   return strcmp(s0->programname, s1->programname);
}

/**********************************************************/

int rmon_insert_sl(
spy_list_type *new 
) {
   spy_list_type **slp;
   int scmp = 1;

#undef FUNC
#define FUNC "rmon_insert_sl"
   DENTER;

   if (!new) {
      DPRINTF(("invalid new spy\n"));
      DEXIT;
      return 0;
   }

   /* insert for key spy */
   slp = &spy_list;
   while (*slp && (scmp = rmon_sl_cmp(new, *slp)) > 0)
      slp = &((*slp)->next);

   if (scmp == 0) {
      DPRINTF(("List element already exists !\n"));
      DEXIT;
      return 0;
   }

   /* link for spy key */
   new->next = *slp;
   *slp = new;

   DEXIT;
   return 1;
}                               /* insert_sl */

/**********************************************************/

spy_list_type **rmon_search_spy(
char *programname 
) {
   spy_list_type **slp, new;
   int scmp = 1;

#undef FUNC
#define FUNC "rmon_search_spy"
   DENTER;

   strncpy(new.programname, programname, STRINGSIZE - 1);

   slp = &spy_list;
   while (*slp && (scmp = rmon_sl_cmp(&new, *slp)) > 0)
      slp = &((*slp)->next);

   DEXIT;
   return (scmp == 0) ? slp : NULL;
}                               /* search_spy */

/**********************************************************/

int rmon_delete_sl(
spy_list_type **slp 
) {
   spy_list_type *sl;

#undef FUNC
#define FUNC "rmon_delete_sl"

   DENTER;

   if (!slp) {
      DPRINTF(("invalid spy list\n"));
      DEXIT;
      return 0;
   }

   if (*slp)
      DPRINTF(("deleting spy:\n"));
   while (*slp) {
      rmon_print_spy(*slp);
      sl = rmon_unchain_sl(slp);
      rmon_delete_ml(&sl->message_list);
      free(sl);
   }

   DEXIT;
   return 1;
}                               /* delete_sl */

/**********************************************************/

void rmon_print_sl(
spy_list_type *sl 
) {

#undef FUNC
#define FUNC "rmon_print_sl"

   DENTER;

   if (!sl) {
      DPRINTF(("spy_list = NULL\n"));
   }

   DPRINTF(("  internet addr   port     hostname programname\n"));
   while (sl) {
      rmon_print_spy(sl);
      sl = sl->next;
   }

   DEXIT;
   return;
}                               /* print_sl */

/**********************************************************/

void rmon_print_spy(
spy_list_type *sl 
) {
   struct in_addr bert;

   if (!sl) {
      DPRINTF(("spy = NULL\n"));
      return;
   }

   bert.s_addr = htonl(sl->inet_addr);
   DPRINTF(("%15s %6d  <%s>\n",
            inet_ntoa(bert), sl->port, sl->programname));

   return;
}                               /* rmon_print_spy() */

/**********************************************************/

spy_list_type *rmon_unchain_sl(
spy_list_type **slp 
) {
   spy_list_type *sl;

#undef FUNC
#define FUNC "rmon_unchain_sl"

   DENTER;
   sl = *slp;
   *slp = (*slp)->next;

   DEXIT;
   return sl;
}                               /* unchain_sl */
