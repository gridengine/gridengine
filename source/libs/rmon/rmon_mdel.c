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
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_siginit.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "msg_rmon.h"
/********** functions ****************************************/

int main(int argc, char *argv[]);
static int parser(int argc, char *argv[]);
static void usage(void);

/*************************************************************/

volatile int SFD = 0;           /* THIS PUPPY IS CLOSED ON SIGALRM */
u_long client_number, error_counter = 0, result = 999;
string programname;

extern int name_is_valid;
extern char rmond[];

/*************************************************************/

int main(
int argc,
char **argv 
) {
   int sfd, status;

#undef FUNC
#define FUNC "main"
   DOPEN("mdel");
   DENTER;

   rmon_init_alarm();
   parser(argc, argv);

   switch (result) {
   case SPY:
      request.kind = SPY;
      strncpy(request.programname, programname, STRINGSIZE - 1);
      break;

   case CLIENT:
      request.kind = CLIENT;
      request.client_number = client_number;
      break;

   default:
      usage();
   }

   while (!(status = rmon_connect_rmond(&sfd, MDEL)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }

   shutdown(sfd, 2);
   close(sfd);

   switch (result) {
   case SPY:
      if (status != S_ACCEPTED) {
         printf(MSG_RMON_SPYNOTFOUND);
         return 0;
      }
      printf(MSG_RMON_SPYDELETEDSUCCESSFULLY);
      break;

   case CLIENT:
      if (status != S_ACCEPTED) {
         printf(MSG_RMON_RMONNOTFOUND);
         return 0;
      }
      printf(MSG_RMON_RMONHASBEENSUCCESFULLYDELETED);
      break;
   }

   DEXIT;
   return 1;
}                               /* main() */

/***************************************************************************/

static int parser(
int argc,
char *argv[] 
) {
   int flag = 0, i = 1;
   char *s;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ==================================================================== */
   /*                                                                                                                                              */
   /* usage:   mdel [-s programname]                                                                               */
   /*                               [-n rmon_number]                                                                               */
   /*                               [-r rmond-host]                                                                                */
   /*                                                                                                                                              */
   /* ==================================================================== */

   while (i < argc) {
      if (argv[i][0] != '-')
         usage();

      switch (argv[i][1]) {
      case 'r':
         if (argv[i][2] != '\0')
            s = &argv[i][2];
         else {
            if (++i >= argc)
               usage();
            s = argv[i];
         }

         if (!rmon_make_name(s, rmond)) {
            printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
            usage();
         }

         name_is_valid = 1;
         i++;
         continue;

      case 's':
         if (flag) {
            printf(MSG_RMON_ONLYSPYORRMONCANBEDELETED );
            usage();
         }

         if (argv[i][2] != '\0')
            s = &argv[i][2];
         else {
            if (++i >= argc)
               usage();
            s = argv[i];
         }

         strncpy(programname, s, STRINGSIZE - 1);

         result = SPY;
         flag = 1;
         i++;
         continue;

      case 'n':
         if (flag) {
            printf(MSG_RMON_ONLYSPYORRMONCANBEDELETED);
            usage();
         }

         if (argv[i][2] != '\0')
            s = &argv[i][2];
         else {
            if (++i >= argc)
               usage();
            s = argv[i];
         }

         sscanf(s, "%ld", &client_number);

         result = CLIENT;
         flag = 1;
         i++;
         continue;

      default:
         usage();
      }
   }
   DEXIT;
   return 1;
}

static void usage()
{
   printf(MSG_RMON_MDEL_USAGE);
   DEXIT;
   exit(0);
}
