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
#define DEBUG

/* for getting the typedef of sigset_t */
/* with ANSI at IRIX 4                                          */
#if (IRIX4)
#define _POSIX_SOURCE
#endif
#include <sys/signal.h>

#include "rmon_h.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "rmon_siginit.h"
#include "msg_rmon.h"
/***************************************************************************/

static void alarmclock(int);
static void sig_set(void);

extern volatile int SFD;        /* THIS PUPPY IS CLOSED ON SIGALRM */

struct sigaction sigterm_vec, sigterm_ovec;
struct sigaction sigalrm_vec, sigalrm_ovec;
struct sigaction sigcld_vec, sigcld_ovec;
sigset_t omask, default_mask;

/***************************************************************************/

static void sig_set()
{
   sigfillset(&default_mask);
   sigdelset(&default_mask, SIGALRM);
   sigdelset(&default_mask, SIGINT);
   sigdelset(&default_mask, SIGKILL);
   sigdelset(&default_mask, SIGTERM);
   sigprocmask(SIG_SETMASK, &default_mask, &omask);
}

/***************************************************************************/

void rmon_init_alarm()
{
#undef FUNC
#define FUNC "rmon_init_alarm"
   DENTER;

   sig_set();

   sigalrm_vec.sa_handler = alarmclock;
   sigfillset(&sigalrm_vec.sa_mask);
   sigalrm_vec.sa_flags = 0;
   sigaction(SIGALRM, &sigalrm_vec, &sigalrm_ovec);

   DEXIT;
}

/***************************************************************************/

void rmon_init_terminate(
handler terminate 
) {
#undef FUNC
#define FUNC "rmon_init_terminate"
   DENTER;

   sig_set();

   sigterm_vec.sa_handler = terminate;
   sigfillset(&sigterm_vec.sa_mask);
   sigdelset(&sigterm_vec.sa_mask, SIGKILL);
   sigdelset(&sigterm_vec.sa_mask, SIGTERM);
   sigterm_vec.sa_flags = 0;
   sigaction(SIGKILL, &sigterm_vec, &sigterm_ovec);
   sigaction(SIGTERM, &sigterm_vec, &sigterm_ovec);

   DEXIT;
}

/***************************************************************************/

void rmon_init_sig_int(
handler sig_int 
) {
#undef FUNC
#define FUNC "rmon_init_sig_int"
   DENTER;

   sig_set();

   sigterm_vec.sa_handler = sig_int;
   sigfillset(&sigterm_vec.sa_mask);
   sigdelset(&sigterm_vec.sa_mask, SIGINT);
   sigterm_vec.sa_flags = 0;
   sigaction(SIGINT, &sigterm_vec, &sigterm_ovec);

   DEXIT;
}

/***************************************************************************/

static void alarmclock(
int n 
) {
#undef  FUNC
#define FUNC "alarmclock"

   DENTER;

   printf(MSG_RMON_ALARMCLOCK);
   if (SFD < 900) {
      shutdown(SFD, 2);
      close(SFD);
   }

   DEXIT;
}
