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

#include "rmon_job_list.h"
#include "rmon_job_protocol.h"
#include "msg_rmon.h"
/********** function protoypes ************************************/

int main(int argc, char *argv[]);
static void parser(int argc, char *argv[]);
static int c_c_mjob(void);
static void usage(void);

/********** global variables **************************************/

volatile int SFD = 0;           /* THIS PUPPY IS CLOSED ON SIGALRM */

/* variables for command line parameter */
u_long all = 0;
u_long clientnumber;
job_list_type *to_add = NULL;
job_list_type *to_delete = NULL;

extern char ptr[];
extern char rmond[];
extern u_long name_is_valid;
/********** functions *********************************************/

int main(
int argc,
char **argv 
) {
#undef FUNC
#define FUNC "main"
   DOPEN("mjob");
   DENTER;

   rmon_init_alarm();
   parser(argc, argv);

   if (!c_c_mjob())
      printf(MSG_RMON_CANTSENDJOBLIST);
   else
      printf(MSG_RMON_OK);

   DEXIT;
   DCLOSE;
   return 0;
}

/***************************************************************************/

static void parser(
int argc,
char *argv[] 
) {
   int i = 1;
   int jobnumber;
   job_list_type *new, **jlp = NULL;

#undef  FUNC
#define FUNC "parser"
   DENTER;

   /* ============================================================================ */
   /*                                                                                          */
   /* usage:   mjob [-r rmond_host] rmon-number {-|+} {jobnumber [...] | all}     */
   /*                                                                                              */
   /* ============================================================================ */

   if (argc == 1)
      usage();

   if (argv[i][0] == '-') {
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

   if (i >= argc)
      usage();

   if (sscanf(argv[i], "%ld", &clientnumber) != 1) {
      printf(MSG_RMON_UNABLETOREADRMONNUMBER );
      usage();
   }

   i++;

   if (i >= argc)
      usage();

   do {
      switch (argv[i++][0]) {
      case '+':
         jlp = &to_add;
         all = 1;
         break;
      case '-':
         jlp = &to_delete;
         all = 2;
         break;
      default:
         usage();
      }

      if (i >= argc)
         usage();

      do {
         if (argv[i][0] == 'a' || argv[i][0] == 'A') {
            all += 4;
            return;
         }

         if (sscanf(argv[i], "%d", &jobnumber) != 1) {
            printf(MSG_RMON_UNABLETOREADJOBNUMB );
            usage();
         }

         new = (job_list_type *) malloc(sizeof(job_list_type));
         if (!new) {
            printf(MSG_RMON_MALLOCFAILED);
            exit(0);
         }
         new->jobid = jobnumber;

         /* insert in specific list */
         new->next = *jlp;
         *jlp = new;

         i++;
      } while (i < argc && argv[i][0] != '+' && argv[i][0] != '-');

   } while (i < argc);

   DEXIT;
   return;
}

/*****************************************************************/

static void usage()
{
   printf(MSG_RMON_MJOB_USAGE );
   fflush(stdout);

   DEXIT;
   exit(0);
}

/*****************************************************************/

static int c_c_mjob()
{
   int sfd, status;
   int n;
   job_list_type *jl;

#undef FUNC
#define FUNC "c_c_mjob"
   DENTER;

   request.client_number = clientnumber;

   DPRINTF(("rmon-number: %d\n", clientnumber));

   /* how many jobs are to add */
   for (jl = to_add, n = 0; jl; jl = jl->next, n++);
   request.n_add = n;

   /* how many jobs are to delete */
   for (jl = to_delete, n = 0; jl; jl = jl->next, n++);
   request.n_delete = n;
   request.kind = all;

   while (!(status = rmon_connect_rmond(&sfd, MJOB)))
      switch (errval) {
      case EINTR:
         break;
      case ECONNREFUSED:
         printf(MSG_RMON_NORMONDAVAILABLE);
         exit(-1);
      }

   switch (status) {

   case S_ACCEPTED:
      break;

   case S_UNKNOWN_CLIENT:
      printf(MSG_RMON_UNNOWNCLIENT);
      exit(0);

   default:
      printf(MSG_RMON_NOTACCEPTED);
      exit(0);
   }

   /* send list of jobs to add */
   for (n = 0; !rmon_send_job_list(sfd, to_add) && n < MAX_TRY; n++);
   if (n == MAX_TRY) {
      printf(MSG_RMON_CANNOTSENDLISTOFJOBSTOADD);
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("to_add sent\n"));

   /* send list of jobs to delete */
   for (n = 0; !rmon_send_job_list(sfd, to_delete) && n < MAX_TRY; n++);
   if (n == MAX_TRY) {
      printf(MSG_RMON_CANNOTSENDLISTOFJOBSTODEL);
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DPRINTF(("to_delete sent\n"));

   if (!(status = rmon_get_ack(sfd))) {
      DEXIT;
      return 0;
   }

   shutdown(sfd, 2);
   close(sfd);

   switch (status) {
   case S_UNKNOWN_SPY:
      printf(MSG_RMON_UNKNOWNSPY);
      DEXIT;
      exit(0);

   case S_UNKNOWN_CLIENT:
      printf(MSG_RMON_UNKNOWNRMON);
      DEXIT;
      exit(0);

   case S_NOT_ACCEPTED:
      printf(MSG_RMON_VERYHEAVYERROR);
      status = 0;
      break;

   case S_ILLEGAL_LEVEL:
      printf(MSG_RMON_ILLEGALMONITORINGLEVEL);
      status = 0;
      break;

   case S_ACCEPTED:
      status = 1;
      break;

   case S_NEW_TRANSITION:
      printf(MSG_RMON_NEWTRANSITIONLISTELEMENT);
      status = 1;
      break;

   case S_DELETE_TRANSITION:
      printf(MSG_RMON_REMOVEDTRANSITIONLISTELEMENT);
      status = 1;
      break;

   case S_ALTER_TRANSITION:
      printf(MSG_RMON_ALTEREDTRANSITIONLISTELEMENT);
      status = 1;
      break;

   case S_NEW_WAIT:
      printf(MSG_RMON_NEWWAITLISTELEMENT);
      status = 1;
      break;

   case S_DELETE_WAIT:
      printf(MSG_RMON_REMOVEDWAITLISTELEMENT);
      status = 1;
      break;

   case S_ALTER_WAIT:
      printf(MSG_RMON_ALTEREDWAITINGLISTELEMENT);
      status = 1;
      break;

   default:
      DPRINTF(("unexpected request status [%d]\n", request.status));
      exit(0);
   }

   DEXIT;
   return status;
}                               /* c_c_mjob  */
