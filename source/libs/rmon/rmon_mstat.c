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
#include "rmon_request.h"
#include "rmon_connect.h"

#include "rmon_job_list.h"
#include "rmon_spy_list.h"
#include "rmon_wait_list.h"
#include "rmon_client_list.h"
#include "rmon_transition_list.h"
#include "rmon_transition_protocol.h"

#include "rmon_monitoring_level.h"

#include "rmon_get_stat.h"

#define NO_OBJECT                               100
#define GET_CLIENT                              0
#define GET_CLIENT_SPECIFIED    1
#define GET_SPY                                 2
#define GET_SPY_SPECIFIED               3

/* ********************** intern prototypes ************ */

int main(int argc, char *argv[]);
static void usage(void);
static void parser(int argc, char *argv[]);

/********** global variables *********************************/

volatile int SFD = 0;           /* THIS PUPPY IS CLOSED ON SIGALRM */

int object = NO_OBJECT;
u_long client;
string programname;

extern char rmond[];
extern u_long name_is_valid;

/*********** and now ... the functions ! *********************/

int main(
int argc,
char **argv 
) {
   int n;
   transition_type *tl;
   spy_list_type *sl, **slp;
   wait_list_type *wl, **wlp;
   client_list_type *cl, **clp;
   job_list_type *jl;

   monitoring_level mask, level;

   int found;

#undef FUNC
#define FUNC "main"

   DOPEN("mstat");
   DENTER;

   parser(argc, argv);

   rmon_init_alarm();

   if (!rmon_get_stat())
      return 0;

   switch (object) {

   case GET_CLIENT:

      /* ================================= */
      /* display list of available clients */
      /* ================================= */

      if (!client_list) {
         printf(MSG_RMON_NORMONCONNECTEDTORMOND);
         break;
      }

      printf(MSG_RMON_RMONWAITACTIVEJOB);
      for (cl = client_list; cl; cl = cl->next) {

         printf(" %3ld ", cl->client);

         n = 0;
         for (wl = wait_list; wl; wl = wl->next)
            if (cl->client == wl->client)
               n++;
         printf(" %3d ", n);

         n = 0;
         rmon_mlclr(&mask);
         rmon_mlset(&mask, ~JOBTRACE);
         for (tl = transition_list; tl; tl = tl->next) {
            if (cl->client == tl->client) {
               rmon_mlcpy(&level, &tl->level);
               if (rmon_mland(&level, &mask))
                  n++;
            }
         }
         printf("  %3d ", n);

         if (cl->all_jobs)
            printf("all\n");
         else {
            n = 0;
            for (jl = cl->job_list; jl; jl = jl->next)
               n++;
            printf("%3d\n", n);
         }
      }

      break;

   case GET_CLIENT_SPECIFIED:

      /* ========================================== */
      /* display information for a specified client */
      /* ========================================== */

      if (!(clp = rmon_search_client(client))) {
         printf(MSG_RMON_NOSUCHRMON);
         break;
      }

      cl = *clp;

      /* print job information */
      printf(MSG_RMON_MONITOREDJOBS );
      if (!cl->job_list && !cl->all_jobs)
         printf(MSG_RMON_NOJOBISCURRENTLYMONITORED);
      else if (cl->all_jobs)
         printf(MSG_RMON_ALLJOBS);
      else
         for (jl = cl->job_list; jl; jl = jl->next)
            printf("-j %3ld\n", jl->jobid);

      /* print transition information */
      printf(MSG_RMON_MONITOREDPROGS);
      printf(MSG_RMON_STATUSPROGNAME);
      found = 0;
      rmon_mlclr(&mask);
      rmon_mlset(&mask, TRACE | INFOPRINT);
      for (tl = transition_list; tl; tl = tl->next) {
         if (tl->client == client) {
            found = 1;
            printf("-s A   ");
            printf("%-22.22s ", tl->programname);
            rmon_mlcpy(&level, &tl->level);
            rmon_mland(&level, &mask);
            rmon_mlprint(&level);
            printf("\n");
         }
      }

      /* print waiting monitoring level */
      for (wl = wait_list; wl; wl = wl->next) {
         if (wl->client == client) {
            found = 1;
            printf("-s W   ");
            printf("%-22.22s ", wl->programname);
            rmon_mlprint(&wl->level);
            printf("\n");
         }
      }
      if (!found)
         printf(MSG_RMON_NOPROGISCURRENTLYMONITORED);

      break;

   case GET_SPY:

      /* ============================== */
      /* display list of available spys */
      /* ============================== */

      if (!spy_list && !wait_list) {
         printf(MSG_RMON_NOSPYCONNECTEDTORMOND);
         break;
      }
      printf( MSG_RMON_PROGNAMESTATUSRMONJOB );

      for (sl = spy_list; sl; sl = sl->next) {

         printf("%-30.30s ", sl->programname);
         printf("A   ");

         n = 0;
         for (tl = transition_list; tl; tl = tl->next) {
            if (strcmp(sl->programname, tl->programname) == 0) {
               rmon_mlclr(&mask);
               rmon_mlset(&mask, TRACE | INFOPRINT);
               if (rmon_mland(&mask, &tl->level))
                  n++;
            }
         }
         printf("    %3d ", n);

         printf("%3ld\n", request.n_job);
      }

      while (wait_list) {
         strncpy(programname, wait_list->programname, STRINGSIZE - 1);
         printf("%-30.30s ", wait_list->programname);
         printf("W   ");

         /* delete all wait list elements        */
         /* with the same programname            */
         /* while counting them                          */
         n = 0;
         wlp = &wait_list;
         while (*wlp)
            if (strcmp((*wlp)->programname, programname) == 0) {
               free(rmon_unchain_wl(wlp));
               n++;
            }
            else
               wlp = &((*wlp)->next);

         printf("    %3d ", n);

         printf("%3ld\n", request.n_job);
      }
      break;

   case GET_SPY_SPECIFIED:

      /* ======================================= */
      /* display information for a specified spy */
      /* ======================================= */

      if (!(slp = rmon_search_spy(programname))) {
         printf(MSG_RMON_NOSUCHSPY);

         for (wl = wait_list; wl; wl = wl->next) {
            if (strcmp(wl->programname, programname) == 0) {
               printf("rmon: %ld ", wl->client);
               rmon_mlprint(&wl->level);
               printf("\n");
            }
         }
      }
      else {
         printf(MSG_RMON_COMBINEDLEVELS);
         rmon_mlclr(&mask);
         rmon_mlset(&mask, ~JOBTRACE);
         rmon_mland(&mask, &((*slp)->moritz_level));
         rmon_mlprint(&mask);
         printf("\n");

         printf(MSG_RMON_PROGNAME_S , (*slp)->programname);
         for (tl = transition_list; tl; tl = tl->next) {
            if (strcmp(tl->programname, programname) == 0) {
               printf("rmon: %ld ", tl->client);

               printf( MSG_RMON_LEVEL );

               rmon_mlclr(&mask);
               rmon_mlset(&mask, ~JOBTRACE);
               rmon_mland(&mask, &tl->level);
               rmon_mlprint(&mask);
               printf("\n");

               rmon_mlclr(&mask);
               rmon_mlset(&mask, JOBTRACE);
               if (rmon_mland(&mask, &tl->level)) {
                  if (!(clp = rmon_search_client(tl->client)))
                     DPRINTF(("corrupt data structure\n"));
                  else {
                     cl = *clp;
                     if (!cl->job_list)
                        printf(MSG_RMON_NOJOBISCURRENTLYMONITORED);
                     else
                        for (jl = cl->job_list; jl; jl = jl->next)
                           printf("-j %3ld\n", jl->jobid);
                  }
               }
            }
         }
      }
      break;
   default:
      usage();
   }
   DEXIT;
   DCLOSE;

   return 0;
}                               /* main() */

/***************************************************************************/

static void parser(
int argc,
char *argv[] 
) {
   int i = 1;
   u_long flag = 0;
   char *s;

#undef  FUNC
#define FUNC "parser"

   DENTER;

   /* ============================================================ */
   /*                                                              */
   /* usage:   mstat [-r rmond-host]                                                               */
   /*                                {-n [rmon-number] | -s [programname] }                */
   /*                                                              */
   /* ============================================================ */

   while (i < argc) {
      if (argv[i][0] == '-') {
         switch (argv[i][1]) {
         case 'r':
            if (argv[i][2] != '\0')
               s = &argv[i][2];
            else {
               if (++i >= argc)
                  usage();
               s = argv[i];
            }

            if (!rmon_make_name(&argv[i][2], rmond)) {
               printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
               usage();
            }

            name_is_valid = 1;
            i++;
            continue;

         case 's':
            if (flag) {
               printf(MSG_RMON_CANTGIVEINFOTOSPYANDRMONBOTH);
               usage();
            }

            if (argv[i][2] != '\0')
               s = &argv[i][2];
            else {
               if (++i >= argc) {
                  object = GET_SPY;
                  flag = 1;
                  continue;
               }
               s = argv[i];
            }

            strncpy(programname, s, STRINGSIZE - 1);

            object = GET_SPY_SPECIFIED;
            flag = 1;
            i++;
            continue;

         case 'n':

            if (flag) {
               printf(MSG_RMON_CANTGIVEINFOTOSPYANDRMONBOTH);
               usage();
            }

            if (argv[i][2] != '\0')
               s = &argv[i][2];
            else {
               if (++i >= argc) {
                  object = GET_CLIENT;
                  flag = 1;
                  continue;
               }
               s = argv[i];
            }

            if (sscanf(s, "%ld", &client) != 1) {
               printf(MSG_RMON_UNABLETOREADRMONNUMBER);
               usage();
            }

            object = GET_CLIENT_SPECIFIED;
            flag = 1;
            i++;
            continue;

         default:
            usage();
         }
      }
      else {
         usage();
      }
   }

   if (object == NO_OBJECT)
      usage();

   DEXIT;
}

static void usage()
{
   printf(MSG_RMON_MSTAT_USAGE);
   fflush(stdout);
   DEXIT;
   exit(0);
}
