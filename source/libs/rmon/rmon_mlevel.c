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
#include "rmon_def.h"
#include "rmon_conf.h"
#include "rmon_rmon.h"
#include "rmon_siginit.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_monitoring_level.h"

#include "rmon_c_c_monitoring_level.h"
#include "msg_rmon.h"
/********** function protoypes ************************************/

int main(int argc, char *argv[]);
static void parser(int argc, char *argv[]);
static void usage(void);

/********** global variables **************************************/

volatile int SFD = 0;

/* variables for command line parameter */
u_long mynumber;
string programname;
u_long spy_wait = 0;
monitoring_level level;

extern char rmond[];
extern u_long name_is_valid;

/********** functions *********************************************/

int main(
int argc,
char **argv 
) {
#undef FUNC
#define FUNC "main"

   DOPEN("mlevel");
   DENTER;

   parser(argc, argv);
   rmon_init_alarm();

   if (!rmon_c_c_monitoring_level())
      printf(MSG_RMON_CANTSENDMONITORINGLEVELTORMOND );

   DEXIT;
   return 0;
}

/***************************************************************************/

static void parser(
int argc,
char *argv[] 
) {
   int j, i = 1;
   char *s;
   u_long layer;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ======================================================================== */
   /*                                                                                      */
   /* usage:   mlevel [-r rmond_host] rmon-number programname monlev [-w]          */
   /*                                                                                      */
   /* ======================================================================== */

   if (argc == 1)
      usage();

   if (argv[i][0] == '-') {
      if (argv[i][1] == 'r') {

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
      }
      else {
         printf(MSG_RMON_UNKNOWNOPTION);
         usage();
      }
   }

   if (i >= argc)
      usage();

   if (sscanf(argv[i], "%ld", &mynumber) != 1) {
      printf(MSG_RMON_UNABLETOREADRMONNUMBER);
      usage();
   }

   if (++i >= argc)
      usage();

   strncpy(programname, argv[i], STRINGSIZE - 1);

   for (j = 0; j < N_LAYER; j++) {
      if (++i >= argc)
         usage();

      if (sscanf(argv[i], "%ld", &layer) != 1) {
         printf(MSG_RMON_UNABLETOREADMONITORINGLEVEL);
         usage();
      }
      rmon_mlputl(&level, j, layer);
/*              printf("level(%d): %ld\n",j, rmon_mlgetl( &level, j)); */
   }

   if (++i >= argc)
      return;

   if (argv[i][0] == '-' && argv[i][1] == 'w')
      spy_wait = 1;
   else {
      printf(MSG_RMON_UNKNOWNOPTION);
      usage();
   }

   DEXIT;
}

static void usage()
{
   printf(MSG_RMON_MLEVEL_USAGE);
   fflush(stdout);
   DEXIT;
   exit(0);
}
