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
u_long type, value;
extern char rmond[];
extern u_long name_is_valid;

/*************************************************************/

int main(
int argc,
char **argv 
) {
   int sfd, status;

#undef FUNC
#define FUNC "main"
   DOPEN("mconf");
   DENTER;

   rmon_init_alarm();
   parser(argc, argv);

   request.conf_type = type;
   request.conf_value = value;

   while (!(status = rmon_connect_rmond(&sfd, MCONF)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }

   shutdown(sfd, 2);
   close(sfd);

   switch (status) {
   case S_ACCEPTED:
      printf(MSG_RMON_CONFIGHASBINACCEPTED);
      break;

   case S_NOT_ACCEPTED:
      printf(MSG_RMON_CONFIGHASNOTBENACCEPTED);
      break;
   }

   DEXIT;
   DCLOSE;
   return 1;
}                               /* main() */

/***************************************************************************/

static int parser(
int argc,
char *argv[] 
) {
   int i = 1, flag = 0;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ==================================================================== */
   /*                                                                                                                                              */
   /* usage:   mconf [-r rmond-host] type value                                                    */
   /*                                                                                                                                              */
   /* ==================================================================== */

   while (i < argc) {

      if (argv[i][0] == '-') {
         switch (argv[i][1]) {
         case 'r':
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
            continue;

         default:
            usage();
         }
      }
      else {
         switch (argv[i][0]) {
         case 'm':
            if (++i >= argc) {
               printf(MSG_RMON_TOOFEWARGUMENTS);
               usage();
            }
            type = MAX_MESSAGES;
            sscanf(argv[i], "%ld", &value);
            i++;
            flag = 1;
            continue;

         default:
            usage();
         }
      }
   }

   if (!flag)
      usage();

   DEXIT;
   return 1;
}

static void usage()
{
   printf(MSG_RMON_MCONF_USAGE );
   /*printf("where type is one of:\n");
   printf("m[essages]\n");*/
   DEXIT;
   exit(0);
}
