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
#define MAINPROGRAM
#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_siginit.h"
#include "rmon_request.h"
#include "rmon_connect.h"

#include "rmon_message_list.h"
#include "rmon_message_protocol.h"

/* client cases */
#include "rmon_c_c_client_register.h"
#include "rmon_c_c_flush.h"
#include "rmon_c_c_monitoring_level.h"
#include "msg_rmon.h"
/********** function protoypes ************************************/

int main(int argc, char *argv[]);
static void parser(int argc, char *argv[]);
static void usage(void);
static void terminate(int n);
static void sig_int(int n);
static void shut_down_client(void);

/********** global variables **************************************/

int message_counter, sfd0;
volatile int SFD = 0;           /* THIS PUPPY IS CLOSED ON SIGALRM */
long wait_time = 0;
long max_wait_time = MAX_WAIT_TIME;
u_long mynumber;

struct servent *sp;

/* variables for command line parameter */
u_long print_host = 0;
u_long print_port = 0;
u_long print_pid = 0;
u_long print_numbers = 0;

string programname;
u_long flush_rate = 1;
u_long spy_wait = 0;
monitoring_level level;

extern char rmond[];
extern u_long name_is_valid;

/********** functions *********************************************/

int main(
int argc,
char **argv 
) {
   struct timeval out_time;

#undef FUNC
#define FUNC "main"
   DOPEN("rmon");
   DENTER;

   parser(argc, argv);

   rmon_init_alarm();
   rmon_init_terminate(terminate);
   rmon_init_sig_int(sig_int);

   if (!(sp = getservbyname(RMOND_SERVICE, "tcp")))
      rmon_errf(TERMINAL, MSG_RMON_XBADSERVICE_S, RMOND_SERVICE);

   if (!rmon_c_c_client_register())
      rmon_errf(TERMINAL, MSG_RMON_CANTREGISTERATRMOND);

   printf(MSG_RMON_NUMBEROFTHISRMONX_D , (u32c) mynumber);

   if (!rmon_mliszero(&level) && !rmon_c_c_monitoring_level())
      rmon_errf(TERMINAL, MSG_RMON_CANTSENDMONITORINGLEVELTORMOND);

   if (flush_rate < 1)
      flush_rate = 1;

   for (;;) {

      /* Reset a timeout-period for the select */
      out_time.tv_sec = wait_time;
      out_time.tv_usec = 0;

      /* 'select' is used as an alarmclock */

      if (select(0, NULL, NULL, NULL, &out_time) == 0) {

         if (!rmon_c_c_flush()) {
            DPRINTF(("ERROR in c_c_flush\n"));
            DPRINTF(("=========================================\n"));
         }
      }
   }
}

/***************************************************************************/

static void terminate(
int n 
) {
#undef  FUNC
#define FUNC "terminate"
   DENTER;

   shut_down_client();

   DEXIT;
}

/***************************************************************************/

static void sig_int(
int n 
) {
#undef  FUNC
#define FUNC "sig_int"
   DENTER;

   shut_down_client();

   DEXIT;
}

/***************************************************************************/

static void shut_down_client(void) {
   int sfd, status;

#undef  FUNC
#define FUNC "shut_down_client"
   DENTER;

   request.kind = CLIENT;
   request.client_number = mynumber;

   while (!(status = rmon_connect_rmond(&sfd, MDEL)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         rmon_errf(TERMINAL, MSG_RMON_NORMONDAVAILABLE);
      }

   shutdown(sfd, 2);
   close(sfd);

   DEXIT;
   DCLOSE;
   exit(0);
}

/***************************************************************************/

static void parser(
int argc,
char *argv[] 
) {
   int i = 1, j;
   char c, *s;
   u_long temp;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ==================================================================== */
   /*                                                                      */
   /* usage:  rmon [-hpin] [-r rmond-host] [-f flush-intervall]                    */
   /*                              [-m max_wait_time]                                                                              */
   /*                              [programname monlev [-w]]                                                               */
   /*                                                                      */
   /* ==================================================================== */

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

         case 'f':
            if (argv[i][2] != '\0')
               sscanf(&argv[i][2], "%ld", &flush_rate);
            else {
               if (++i >= argc)
                  usage();
               sscanf(argv[i], "%ld", &flush_rate);
            }
            i++;
            continue;

         case 'm':
            if (argv[i][2] != '\0')
               sscanf(&argv[i][2], "%ld", &max_wait_time);
            else {
               if (++i >= argc)
                  usage();
               sscanf(argv[i], "%ld", &max_wait_time);
            }
            i++;
            continue;

         case 'h':
         case 'p':
         case 'i':
         case 'n':
            j = 1;
            while ((c = argv[i][j++])) {

               print_host |= (c == 'h');
               print_port |= (c == 'p');
               print_pid |= (c == 'i');
               print_numbers |= (c == 'n');

               if (c != 'h' && c != 'p' && c != 'i' && c != 'n') {
                  DEXIT;
                  usage();
               }
            }
            i++;
            continue;

         case 'w':
            spy_wait = 1;
            i++;
            continue;
         }
      }

      strncpy(programname, argv[i], STRINGSIZE - 1);

      if (++i == argc) {
         printf(MSG_RMON_TOOFEWARGUMENTS);
         usage();
      }

      /* monitoring level */
      rmon_mlclr(&level);
      for (j = 0; j < rmon_mlnlayer(); j++) {

         if (sscanf(argv[i], "%ld", &temp) != 1) {
            printf(MSG_RMON_UNABLETOREADMONITORINGLEVEL);
            usage();
         }
         printf(MSG_RMON_LEVEL_D, temp);
         rmon_mlputl(&level, j, temp);
      }
      i++;
   }

   DEXIT;
   return;
}

static void usage()
{
   printf(MSG_RMON_RMON_USAGE);
   fflush(stdout);
   exit(0);
}
