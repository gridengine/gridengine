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
#include "rmon_rmon.h"
#include "rmon_request.h"
#include "rmon_client_number.h"
#include "msg_rmon.h"
#include "rmon_client_list.h"
#include "rmon_m_c_client_register.h"

/*****************************************************************/

int rmon_m_c_client_register(
int sfd 
) {
   int status;
   u_long number, uid;
   client_list_type *cl;

#undef  FUNC
#define FUNC "rmon_m_c_client_register"
   DENTER;

   /* seek for a free client number */
   if ((number = rmon_get_cn()) == 0xffffffff) {
      DPRINTF(("ERROR: no more rmon numbers are free\n"));
      rmon_send_ack(sfd, S_NOT_ACCEPTED);
      DEXIT;
      return 0;
   }

   uid = request.uid;

   /* send ack */
   request.client_number = number;
   if (!rmon_send_ack(sfd, S_ACCEPTED)) {
      rmon_free_cn(number);
      DEXIT;
      return 0;
   }
   if (!(status = rmon_get_ack(sfd))) {
      rmon_free_cn(number);
      DEXIT;
      return 0;
   }

   if (status != S_ACCEPTED) {
      rmon_free_cn(number);
      DEXIT;
      return 1;
   }

   /* build up new client list element */
   cl = (client_list_type *) malloc(sizeof(client_list_type));
   if (!cl)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   cl->uid = uid;
   cl->client = number;
   cl->last_flush = 0;
   cl->job_list = NULL;
   cl->all_jobs = 0;
   cl->next = NULL;

   if (!rmon_insert_cl(cl)) {
      DPRINTF(("can't insert into rmon list\n"));
      DEXIT;
      return 0;
   }

   rmon_print_cl(cl);

   DEXIT;
   return 1;
}
