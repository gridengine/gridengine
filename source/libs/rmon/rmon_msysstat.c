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
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_siginit.h"

#include "rmon_job_list.h"
#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_transition_protocol.h"

#include "rmon_monitoring_level.h"

#include "rmon_get_stat.h"

/* ********************** intern prototypes ************ */

int main(int argc, char *argv[]);

static void print_wait_table(wait_list_type *wl);
static void print_spy_table(spy_list_type *sl);
static void print_client_table(client_list_type *cl);
static void print_transition_table(transition_type *tl);
static void print_job_table(job_list_type *jl);
static void usage(void);

/********** global variables *********************************/

volatile int SFD = 0;           /* THIS PUPPY IS CLOSED ON SIGALRM */

extern u_long name_is_valid;
extern char rmond[];

/*********** and now ... the functions ! *********************/

int main(
int argc,
char **argv 
) {
   int i = 1, all_flag;
   client_list_type *cl;

#undef FUNC
#define FUNC "main"
   DOPEN("msysstat");
   DENTER;

   if (i < argc && argv[i][0] == '-') {
      if (argv[i][1] == 'r') {
         if (argv[i][2] != '\0') {
            if (!rmon_make_name(&argv[i][2], rmond)) {
               printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
               usage();
            }
         }
         else {
            if (++i >= argc)
               usage();
            if (!rmon_make_name(argv[i], rmond)) {
               printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
               usage();
            }
         }
         name_is_valid = 1;
         i++;
      }
      else {
         printf(MSG_RMON_UNKNOWNOPTION);
         usage();
      }
   }

   rmon_init_alarm();

   if (!rmon_get_stat())
      return 0;

   printf(MSG_RMON_RMONLIST);
   print_client_table(client_list);
   printf("--------------------------------------------------\n");

   printf(MSG_RMON_WAITLIST);
   print_wait_table(wait_list);
   printf("--------------------------------------------------\n");

   printf(MSG_RMON_SPYLIST);
   print_spy_table(spy_list);
   printf("--------------------------------------------------\n");

   printf(MSG_RMON_TRANSITIONLIST);
   print_transition_table(transition_list);
   printf("--------------------------------------------------\n");

   printf(MSG_RMON_GLOBALJOBLIST);
   all_flag = 0;
   cl = client_list;
   while (cl) {
      if (cl->all_jobs)
         all_flag = 1;
      cl = cl->next;
   }
   if (all_flag)
      printf(MSG_RMON_ALL);
   else {
      print_job_table(job_list);
      printf("\n");
   }

   DEXIT;
   return 0;
}                               /* main() */

/* ================================ */
/*                                 */
/* functions for printing of lists */
/*                                 */
/* ================================ */

static void print_spy_table(
spy_list_type *sl 
) {
   struct in_addr bert;

#undef FUNC
#define FUNC "print_spy_table"
   DENTER;

   if (!sl) {
      printf("no entry\n");
      DEXIT;
      return;
   }

   printf(MSG_RMON_SPYTABLEHEADER);
   while (sl) {

      printf("%-22s ", sl->programname);
      bert.s_addr = htonl(sl->inet_addr);
      printf("%-15.15s ", inet_ntoa(bert));
      printf("%6ld ", sl->port);
      printf("%6ld ", sl->uid);
      printf("  %1ld ", sl->childdeath);
      printf("%6ld ", sl->last_flush);
      rmon_mlprint(&sl->moritz_level);
      printf("%5ld ", sl->first);
      printf(" %5ld\n", sl->last);

      sl = sl->next;
   }
   DEXIT;
   return;
}                               /* print_spy_table() */

/***************************************************************************/

static void print_job_table(
job_list_type *jl 
) {

#undef FUNC
#define FUNC "print_job_table"
   DENTER;

   if (!jl) {
      printf(MSG_RMON_NOENTRY);
      DEXIT;
      return;
   }

   while (jl) {
      printf("%3ld ", jl->jobid);
      jl = jl->next;
   }
   DEXIT;
   return;
}                               /* print_job_table() */

/***************************************************************************/

static void print_client_table(
client_list_type *cl 
) {

#undef FUNC
#define FUNC "print_client_table"
   DENTER;

   if (!cl) {
      printf(MSG_RMON_NOENTRY);
      DEXIT;
      return;
   }

   printf( MSG_RMON_CLIENTTABLEHEADER );

   while (cl) {

      printf("%3ld ", cl->client);
      printf("%6ld ", cl->uid);
      printf("%6ld ", cl->last_flush);

      if (cl->job_list) {
         print_job_table(cl->job_list);
         printf("\n");
      }
      else if (cl->all_jobs)
         printf(MSG_RMON_ALL);
      else
         printf("\n");

      cl = cl->next;
   }
   DEXIT;
   return;
}                               /* print_client_table() */

/***************************************************************************/

static void print_wait_table(
wait_list_type *wl 
) {

   int i;

#undef FUNC
#define FUNC "print_wait_table"

   DENTER;

   if (!wl) {
      printf(MSG_RMON_NOENTRY);
      DEXIT;
      return;
   }

   printf(MSG_RMON_WAITTABLEHEADER);
   for (i = 0; i < rmon_mlnlayer(); i++)
      printf("  L%1d", i);
   printf("\n");

   while (wl) {

      printf("%-22s ", wl->programname);
      printf("%6ld ", wl->client);
      rmon_mlprint(&wl->level);
      printf("\n");

      wl = wl->next;
   }
   DEXIT;
   return;
}                               /* print_wait_table() */

/***************************************************************************/

static void print_transition_table(
transition_type *tl 
) {
#undef FUNC
#define FUNC "print_transition_table"
   DENTER;

   if (!tl) {
      printf(MSG_RMON_NOENTRY);
      DEXIT;
      return;
   }

   printf(MSG_RMON_TRANSITIONTABLEHEADER);
   while (tl) {

      printf("%6ld ", tl->client);
      printf("%-22s ", tl->programname);
      rmon_mlprint(&tl->level);
      printf(" %5ld\n", tl->last_read);

      tl = tl->next;
   }
   DEXIT;
   return;
}                               /* print_transition_table() */

/***************************************************************************/

static void usage()
{
   printf(MSG_RMON_MSYSSTAT_USAGE);
   DEXIT;
   exit(0);
}
