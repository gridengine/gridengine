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
#include <stdlib.h>
#ifndef WIN32NATIVE
#include <unistd.h>
#include <sys/time.h>
#else /* WIN32NATIVE */
#include <winsock2.h>
#endif /* WIN32NATIVE */

#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_security.h"
#include "sge_exit.h"

static exit_func_type exit_func = NULL;


#ifdef SECURE
typedef int (*sec_exit_func_type)(void);
static sec_exit_func_type sec_exit_func = NULL;
static sec_exit_func_type install_sec_exit_func(sec_exit_func_type);

static sec_exit_func_type install_sec_exit_func(
sec_exit_func_type new 
) {
   sec_exit_func_type old;

   old = sec_exit_func;
   sec_exit_func = new;
   return(old);
}
#endif


/*---------------------------------------------------------------
 * Name:  sge_exit
 * Descr: wrapped exit function
 *	  calls exit_func if installed
 *        stops monitoring with DCLOSE
 *----------------------------------------------------------------*/
void sge_exit(
int i 
) {
   if (exit_func) {
      exit_func(i);
   }

#ifdef SECURE
   if (sec_exit_func && !i)
      sec_exit_func();
#endif

   DCLOSE;
   exit(i);
}

/*---------------------------------------------------------------
 * Name:  install_exit_func
 * Descr: installs a new exit handler and returns the old one
 *----------------------------------------------------------------*/
exit_func_type install_exit_func(
exit_func_type new 
) {
   exit_func_type old;

   old = exit_func;
   exit_func = new;

   return old;
}

/*---------------------------------------------------------------
 * Name:  sge_close_all_fds
 * Descr: close all file descriptors
 *----------------------------------------------------------------*/
void sge_close_all_fds(
fd_set *keep_open 
) {
   int fd;
   int maxfd;

#ifndef WIN32NATIVE
   maxfd = sysconf(_SC_OPEN_MAX);
#else /* WIN32NATIVE */
   maxfd = FD_SETSIZE;
   /* detect maximal number of fds under NT/W2000 (env: Files)*/
#endif /* WIN32NATIVE */
   
   for (fd = 0; fd < maxfd; fd++) 
      if (!(keep_open && FD_ISSET(fd, keep_open)))
#ifndef WIN32NATIVE
         close(fd);
#else /* WIN32NATIVE */
	     closesocket(fd);
#endif /* WIN32NATIVE */
   return;
}

