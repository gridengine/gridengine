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

#include "rmon_request.h"
#include "rmon_io.h"
#include "rmon_convert.h"

#define REQUEST_SIZE        (18+N_LAYER)*ULONGSIZE+1*STRINGSIZE

extern volatile int SFD;

u_long timeout_value = ALARMS;
request_type request;

/***************************************************************/

int rmon_send_request(
int sfd 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "rmon_send_request"
   DENTER;

   tmp = ptr;
   tmp = rmon_convertint(tmp, &request.type);
   tmp = rmon_convertint(tmp, &request.status);
   tmp = rmon_convertint(tmp, &request.uid);

   tmp = rmon_convertint(tmp, &request.client_number);
   tmp = rmon_convertint(tmp, &request.inet_addr);
   tmp = rmon_convertint(tmp, &request.port);
   tmp = rmon_convertml(tmp, &request.level);
   tmp = rmon_convertint(tmp, &request.childdeath);
   tmp = rmon_convertint(tmp, &request.wait);
   tmp = rmon_convertint(tmp, &request.kind);
   tmp = rmon_convertint(tmp, &request.n_client);
   tmp = rmon_convertint(tmp, &request.n_spy);
   tmp = rmon_convertint(tmp, &request.n_transition);
   tmp = rmon_convertint(tmp, &request.n_wait);
   tmp = rmon_convertint(tmp, &request.n_job);
   tmp = rmon_convertint(tmp, &request.n_add);
   tmp = rmon_convertint(tmp, &request.n_delete);
   tmp = rmon_convertint(tmp, &request.conf_type);
   tmp = rmon_convertint(tmp, &request.conf_value);

   tmp = rmon_convertstr(tmp, request.programname);

   SFD = sfd;
   alarm(timeout_value + (rand() & 1));
   i = rmon_writenbytes(sfd, ptr, REQUEST_SIZE);
   alarm(0);
   SFD = 999;

   DEXIT;
   return (i == 0);
}

/*****************************************************************/

int rmon_get_request(
int sfd 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "rmon_get_request"
   DENTER;

   SFD = sfd;
   alarm(timeout_value + (rand() & 1));
   i = rmon_readnbytes(sfd, ptr, REQUEST_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return (i == 0);
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, &request.type);
   tmp = rmon_unconvertint(tmp, &request.status);
   tmp = rmon_unconvertint(tmp, &request.uid);

   tmp = rmon_unconvertint(tmp, &request.client_number);
   tmp = rmon_unconvertint(tmp, &request.inet_addr);
   tmp = rmon_unconvertint(tmp, &request.port);
   tmp = rmon_unconvertml(tmp, &request.level);
   tmp = rmon_unconvertint(tmp, &request.childdeath);
   tmp = rmon_unconvertint(tmp, &request.wait);
   tmp = rmon_unconvertint(tmp, &request.kind);
   tmp = rmon_unconvertint(tmp, &request.n_client);
   tmp = rmon_unconvertint(tmp, &request.n_spy);
   tmp = rmon_unconvertint(tmp, &request.n_transition);
   tmp = rmon_unconvertint(tmp, &request.n_wait);
   tmp = rmon_unconvertint(tmp, &request.n_job);
   tmp = rmon_unconvertint(tmp, &request.n_add);
   tmp = rmon_unconvertint(tmp, &request.n_delete);
   tmp = rmon_unconvertint(tmp, &request.conf_type);
   tmp = rmon_unconvertint(tmp, &request.conf_value);

   tmp = rmon_unconvertstr(tmp, request.programname);

   DEXIT;
   return (i == 0);
}

/* ************************************************************** */

int rmon_get_ack(
int sfd 
) {
#undef  FUNC
#define FUNC "rmon_get_ack"
   DENTER;

   if (!rmon_get_request(sfd)) {
      DPRINTF(("ERROR: unable to get acknowledge\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DEXIT;
   return request.status;
}

/* ************************************************************** */

int rmon_send_ack(
int sfd,
u_long status 
) {
#undef  FUNC
#define FUNC "rmon_send_ack"
   DENTER;

   request.status = status;
   if (!rmon_send_request(sfd)) {
      DPRINTF(("ERROR: unable to contact peer\n"));
      shutdown(sfd, 2);
      close(sfd);
      DEXIT;
      return 0;
   }
   DEXIT;
   return 1;
}
