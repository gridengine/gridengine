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
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_conf.h"
#include "rmon_restart.h"
#include "rmon_lock.h"
#include "rmon_connect.h"
#include "rmon_request.h"
#include "rmon_spy_list.h"
#include "msg_rmon.h"

#define MAXLINE 80
#define MAXTRY 20

extern char rmon_root[];
char *restart_path_and_file = NULL;
char *restart_path_and_file_lock = NULL;

int rmon_restart()
{
   FILE *fp;
   int reached, sfd;
   struct in_addr bert;
   u_long addr, port;
   char line[MAXLINE + 1], addr_s[MAXLINE + 1];
   string programname;
   int lc = 1;

#undef FUNC
#define FUNC "rmon_restart"

   DENTER;

   /* build name of file */
   if (!restart_path_and_file) {
      restart_path_and_file = (char *) malloc(100);
      sprintf(restart_path_and_file, "%sconf/%s", rmon_root, RMON_RESTART_SPY_FILE);
   }

   if (!(fp = fopen(restart_path_and_file, "r"))) {
      rmon_errf(TRIVIAL, MSG_RMON_CANTOPENXFORCONFIGDATA_S,
                restart_path_and_file);
      DEXIT;
      return 0;
   }

   /* read a line from file */
   for (lc = 1; fgets(line, MAXLINE + 1, fp); lc++) {

      /* parse addr-string and portnumber from line */
      if (sscanf(line, "%s %s %ld", programname, addr_s, &port) != 3) {
         DPRINTF(("<%s> <%s>, <%ld>\n", programname, addr_s, port));
         rmon_errf(TRIVIAL, MSG_RMON_FORMATERRORINXLINEY_SI ,
                   restart_path_and_file, lc);

         fclose(fp);
         DEXIT;
         return 0;
      }
      addr = inet_addr(addr_s);
      if (addr == (u_long) - 1) {
         rmon_errf(TRIVIAL, MSG_RMON_FORMATERRORINXLINYCANNOTREADINAD_SI,
                   restart_path_and_file, lc);

         fclose(fp);
         DEXIT;
         return 0;
      }

      /* try to wake up each spy */
      reached = rmon_connect_anyone(&sfd, WAKE_UP, addr, port);

      bert.s_addr = ntohl(addr);
      DPRINTF(("PROGRAM: %-22.22s HOST: %-15.15s PORT: %ld -> %s\n",
               programname, inet_ntoa(bert), port,
               (reached) ? "waked up" : "not reached"));

   }

   fclose(fp);
   free(restart_path_and_file);
   restart_path_and_file = NULL;

   DEXIT;
   return 1;
}                               /* restart */

int rmon_save_restart_file()
{
   spy_list_type *sl;
   FILE *fp;
   struct in_addr bert;

#undef FUNC
#define FUNC "rmon_save_restart_file"

   if (!restart_path_and_file) {
      restart_path_and_file = (char *) malloc(100);
      sprintf(restart_path_and_file, "%sconf/%s", rmon_root, RMON_RESTART_SPY_FILE);
   }

   if (!(fp = fopen(restart_path_and_file, "w"))) {
      DPRINTF(("cannot open %s for writing\n", restart_path_and_file));
      DEXIT;
      return 0;
   }

   for (sl = spy_list; sl; sl = sl->next) {
      bert.s_addr = ntohl(sl->inet_addr);
      if (fprintf(fp, "%s %s %ld\n", sl->programname, inet_ntoa(bert), sl->port) == EOF) {
         DPRINTF(("cannot write in %s\n", restart_path_and_file));
         fclose(fp);
         DEXIT;
         return 0;
      }
   }

   fclose(fp);

   free(restart_path_and_file);
   restart_path_and_file = NULL;

   DEXIT;
   return 1;
}

int rmon_load_restart_file()
{
   spy_list_type *sl;
   FILE *fp;
   /* struct in_addr      bert; */

   u_long addr, port;
   char line[MAXLINE + 1], addr_s[MAXLINE + 1];
   string programname;
   int lc = 1;

#undef FUNC
#define FUNC "rmon_load_restart_file"

   DENTER;

   /* build name of file */
   if (!restart_path_and_file) {
      restart_path_and_file = (char *) malloc(100);
      sprintf(restart_path_and_file, "%sconf/%s", rmon_root, RMON_RESTART_SPY_FILE);
   }

   if (!(fp = fopen(restart_path_and_file, "r"))) {
      DPRINTF(("cannot open %s for writing\n", restart_path_and_file));
      DEXIT;
      return 0;
   }

   /* read a line from file */
   for (lc = 1; fgets(line, MAXLINE + 1, fp); lc++) {

      /* parse addr-string and portnumber from line */
      if (sscanf(line, "%s %s %ld", programname, addr_s, &port) != 3) {
         printf("<%s> <%s>, <%ld>\n", programname, addr_s, port);

         rmon_errf(TRIVIAL, MSG_RMON_FORMATERRORINXLINEY_SI,
                   restart_path_and_file, lc);
         fclose(fp);
         DEXIT;
         return 0;
      }
      addr = inet_addr(addr_s);
      if (addr == (u_long) - 1) {
         rmon_errf(TRIVIAL, MSG_RMON_FORMATERRORINXLINYCANNOTREADINAD_SI, restart_path_and_file, lc);

         fclose(fp);
         DEXIT;
         return 0;
      }

      sl = (spy_list_type *) malloc(sizeof(spy_list_type));
      if (!sl)
         rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

      sl->inet_addr = addr;
      sl->port = port;
      strncpy(sl->programname, programname, STRINGSIZE - 1);
      sl->message_list = NULL;
      sl->next = NULL;

      if (!rmon_insert_sl(sl))
         rmon_errf(TERMINAL, MSG_RMON_CORRUPTDATASTRUCTURES);

      /* bert.s_addr = ntohl(addr);
         DPRINTF(("HOST: %-15.15s PORT: %ld\n", inet_ntoa(bert), port )); */
   }

   fclose(fp);

   DEXIT;
   return 1;
}

int rmon_lock_restart_file()
{

   int i;

#undef FUNC
#define FUNC "rmon_lock_restart_file"

   DENTER;

   /* build name of lockfile */
   if (!restart_path_and_file_lock) {
      restart_path_and_file_lock = (char *) malloc(100);
      sprintf(restart_path_and_file_lock, "%sconf/%s", rmon_root, RMON_RESTART_SPY_FILE_LOCK);
   }

   for (i = 0; i < MAXTRY; i++) {
      if (rmon_lock(restart_path_and_file_lock) == 0)
         break;
      DPRINTF(("waiting for access on restart file\n"));
      sleep(1);
   }

   if (i == MAXTRY) {
      DPRINTF(("name of lockfile: <%s>\n", restart_path_and_file_lock));
      DEXIT;
      return 0;
   }

/*      DPRINTF(("locked restart file\n")); */

   DEXIT;
   return 1;
}                               /* lock_restart_file */

int rmon_unlock_restart_file()
{
   int temp;

#undef FUNC
#define FUNC "rmon_unlock_restart_file"

   DENTER;

   temp = rmon_unlock(restart_path_and_file_lock);

   free(restart_path_and_file_lock);
   restart_path_and_file_lock = NULL;

   if (temp) {
      DPRINTF(("cannot unlock restart file\n"));
      rmon_errf(TRIVIAL, MSG_RMON_CANTUNLOCKRESTARTFILE);
   }

/*      DPRINTF(("unlocked restart file\n")); */

   DEXIT;
   return (temp == 0) ? 1 : 0;
}                               /* unlock_restart_file */
