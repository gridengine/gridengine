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
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_siginit.h"
#include "msg_rmon.h"

int main(int argc, char **argv);
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
   int status, sfd;
   int i = 1;

#undef FUNC
#define FUNC "main"
   DOPEN("mquit");
   DENTER;

   if (i < argc) {
      if (argv[i][0] != '-' || argv[i][1] != 'r') {
         printf(MSG_RMON_UNKNOWNOPTION);
         usage();
      }

      if (argv[i][2] != '\0') {
         if (!rmon_make_name(&argv[i][2], rmond)) {
            printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
            usage();
         }
      }
      else {
         if (++i >= argc) {
            printf(MSG_RMON_UNKNOWNOPTION);
            usage();
         }

         if (!rmon_make_name(argv[i], rmond)) {
            printf(MSG_RMON_CANTINTERPRETHOSTNAMEINETADDR);
            usage();
         }
      }
      name_is_valid = 1;
      i++;
   }

   rmon_init_alarm();

   while (!(status = rmon_connect_rmond(&sfd, MQUIT))) {
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }
   }

   shutdown(sfd, 2);
   close(sfd);

   if (status != S_ACCEPTED) {
      printf(MSG_RMON_CANTPROPERLYSHUTDOWNMONSYSTEM);
      DEXIT;
      return 0;
   }

   printf(MSG_RMON_SHUTDWONMONITORINGSYSTEM);

   DEXIT;
   return 1;
}                               /* main */

static void usage()
{
   printf(MSG_RMON_MQUIT_USAGE);
   fflush(stdout);
   DEXIT;
   exit(0);
}
