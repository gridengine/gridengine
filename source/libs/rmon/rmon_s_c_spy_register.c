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
#include "rmon_conf.h"
#include "rmon_rmon.h"
#include "rmon_restart.h"
#include "rmon_request.h"
#include "rmon_connect.h"
#include "rmon_semaph.h"
#include "rmon_monitoring_level.h"
#include "rmon_s_c_sleep.h"
#include "rmon_spy_list.h"

#include "rmon_s_c_spy_register.h"
#include "rmon_spy.h"

extern char *rmon_root, *rmon_config_file_and_path      /* , 
                                                           *rmon_config_file_and_path_lock */ ;
extern monitoring_box_type *monitoring_box_down;
extern string programname;
extern u_long port;
extern u_long addr;
extern int state;

extern u_long max_messages;


/*****************************************************************/

static int register_at_rmond(void);

/*****************************************************************/

int rmon_s_c_spy_register()
{

   spy_list_type *sl;

#undef  FUNC
#define FUNC "rmon_s_c_spy_register"

   DENTER;

   if (!register_at_rmond()) {

      /* register in restart file */
      if (!rmon_lock_restart_file()) {
         rmon_shut_down_on_error();
         rmon_errf(TERMINAL, MSG_RMON_CANTLOCKRESTARTFILE);
      }

      if (!rmon_load_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTLOADRESTARTFILEX_S, strerror(errno));

      sl = (spy_list_type *) malloc(sizeof(spy_list_type));
      if (!sl)
         rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

      sl->inet_addr = addr;
      sl->port = port;
      strncpy(sl->programname, programname, STRINGSIZE - 1);
      sl->message_list = NULL;
      sl->next = NULL;

      if (!rmon_insert_sl(sl))
         rmon_errf(TERMINAL, MSG_RMON_SPYTRIEDTOREGISTERWICE);

      if (!rmon_save_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTSAVERESTARTFILE, strerror(errno));

      if (!rmon_unlock_restart_file())
         rmon_errf(TERMINAL, MSG_RMON_CANTUNLOCKRESTARTFILE);

      rmon_delete_sl(&spy_list);

      rmon_mlclr(&monitoring_box_down->level);
      monitoring_box_down->valid = 1;
      monitoring_box_down->quit = 0;
      state = STATE_SLEEPING;

   }
   else
      state = STATE_AWAKE;

   DEXIT;
   return 1;
}                               /* s_c_spy_register */

/*****************************************************************/

static int register_at_rmond()
{
   int i, sfd;

#undef FUNC
#define FUNC "register_at_rmond"

   DENTER;

   request.port = port;         /* Port                 */
   request.inet_addr = htonl(addr);     /* Inet-Addr.   */
   request.uid = (u_long) getuid();     /* User-ID              */
   strcpy(request.programname, programname);

   if (!(i = rmon_connect_rmond(&sfd, SPY_REGISTER))) {
      switch (errval) {
      case ECONNREFUSED:
      case EINTR:
         DEXIT;
         return 0;
      }
   }

   if (i != S_ACCEPTED) {
      printf(MSG_RMON_CANTESTABLISCONNECTION);
      DEXIT;
      return 0;
   }

   if (request.conf_type == MAX_MESSAGES)
      max_messages = request.conf_value;

   shutdown(sfd, 2);
   close(sfd);

   DEXIT;
   return 1;
}                               /* register_at_rmond */
