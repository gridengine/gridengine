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
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "basis_types.h"
#include "msg_utilib.h"
#include "sge_set_def_sig_mask.h"

#if defined(ALPHA)
#undef NSIG
#define NSIG (SIGUSR2+1)
#endif

/*--------------------------------------------------------------------
 * set_def_sig_mask
 * set signal mask to default for all signals except given signal
 *--------------------------------------------------------------------*/
void sge_set_def_sig_mask(
int sig_num,
err_func_t err_func 
) {
   int i;
   struct sigaction sig_vec;
   sigset_t sigmask;
   char err_str[256];
   
   errno = 0;
   for (i=1; i < NSIG; i++) {
#if !defined(HP10) && !defined(HP10_01) && !defined(HPCONVEX) && !defined(HP11)
      if (i != SIGKILL && i != SIGSTOP && i != sig_num)
#else
      if (i != SIGKILL && i != SIGSTOP && i != _SIGRESERVE && i != SIGDIL && i != sig_num)
#endif            
      {
         sigemptyset(&sig_vec.sa_mask);
         sig_vec.sa_flags = 0;
         sig_vec.sa_handler = 0;
         sig_vec.sa_handler = SIG_DFL;
         if (sigaction(i, &sig_vec, NULL)) {
            sprintf(err_str, MSG_PROC_SIGACTIONFAILED_IS, 
                    i, strerror(errno));
            if (err_func)
               err_func(err_str);
         }
      }
   }  

   /* unblock all signals */
   /* without this we depend on shell to unblock the signals */
   /* result is that SIGXCPU was not delivered with several shells */
   sigemptyset(&sigmask);
   sigprocmask(SIG_SETMASK, &sigmask, NULL);
}
