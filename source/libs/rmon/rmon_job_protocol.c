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
#include "rmon_io.h"
#include "rmon_err.h"
#include "rmon_conf.h"
#include "rmon_rmon.h"

#include "rmon_convert.h"

#include "rmon_job_list.h"
#include "rmon_job_protocol.h"
#include "msg_rmon.h"
#define JOB_SIZE                1*ULONGSIZE

extern volatile int SFD;

static job_list_type *receive_jl(int sfd);
static int send_jl(int sfd, job_list_type *jl);

/*****************************************************************/

int rmon_receive_job_list(
int sfd,
job_list_type **jlp,
int n 
) {
   int i = 0;

#undef  FUNC
#define FUNC "rmon_receive_job_list"
   DENTER;
   while (i++ < n) {
      if (!(*jlp = receive_jl(sfd))) {
         DEXIT;
         return 0;
      }
      jlp = &((*jlp)->next);
   }

   DEXIT;
   return 1;
}

/*****************************************************************/

static job_list_type *receive_jl(
int sfd 
) {
   u_long i;
   job_list_type *new;
   char *tmp;

#undef FUNC
#define FUNC "receive_jl"
   DENTER;

   new = (job_list_type *) malloc(sizeof(job_list_type));
   if (!new)
      rmon_errf(TERMINAL, MSG_RMON_MALLOCFAILED);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_readnbytes(sfd, ptr, JOB_SIZE);
   alarm(0);
   SFD = 999;

   if (i) {
      free(new);
      DPRINTF((" returning %d\n", i));
      DEXIT;
      return NULL;
   }

   tmp = ptr;
   tmp = rmon_unconvertint(tmp, &new->jobid);
   new->next = NULL;

   DEXIT;
   return new;
}                               /* receive_jl() */

/*****************************************************************/

int rmon_send_job_list(
int sfd,
job_list_type *jl 
) {

#undef FUNC
#define FUNC "rmon_send_job_list"

   DENTER;

   while (jl) {
      if (send_jl(sfd, jl) != 0) {
         DEXIT;
         return 0;
      }
      jl = jl->next;
   }

   DEXIT;
   return 1;
}                               /* rmon_send_job_list() */

/*****************************************************************/

static int send_jl(
int sfd,
job_list_type *jl 
) {
   int i;
   char *tmp;

#undef  FUNC
#define FUNC "send_jl"

   DENTER;

   /* convert the request in a net convertage */
   tmp = ptr;
   tmp = rmon_convertint(tmp, &jl->jobid);

   SFD = sfd;
   alarm(ALARMS);
   i = rmon_writenbytes(sfd, ptr, JOB_SIZE);
   alarm(0);
   SFD = 999;

   DEXIT;
   return i;
}                               /* send_jl() */
