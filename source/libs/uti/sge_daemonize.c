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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef SIGTSTP
#   include <sys/file.h>
#endif

#if defined(SOLARIS)
#   include <sys/termios.h>
#   include <sys/types.h>
#endif

#if defined(__sgi) || defined(ALPHA)
#   include <rpcsvc/ypclnt.h>
#endif

#if defined(AIX32) || defined(AIX41)
#   include <sys/select.h>
#endif

#include "sgermon.h"
#include "sge_pgrp.h"
#include "sge_daemonize.h"
#include "sge_me.h"
#include "sge_log.h"
#include "msg_utilib.h"
#include "sge_stat.h" 
#include "sge_unistd.h"

/*-------------------------------------------------------------
 * sge_daemonize
 *
 * throws ourself into the background and dissassociates 
 * from any controlling ttys
 *-------------------------------------------------------------*/
int sge_daemonize(
fd_set *keep_open 
) {
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX))
   int fd;
#endif

#if defined(__sgi) || defined(ALPHA)
#  if defined(ALPHA)
   extern int getdomainname(char *, int);
#  endif
   char domname[256];
#endif
   pid_t pid;

   DENTER(TOP_LAYER, "sge_daemonize");
   
#ifndef NO_SGE_COMPILE_DEBUG
   if (TRACEON) {
      DEXIT;
      return 0;
   }
#endif

   if (me.daemonized) {
      DEXIT;
      return 1;
   }   

   if ((pid=fork())!= 0) {             /* 1st child not pgrp leader */
      if (pid<0) {
         CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      }
      exit(0);
   }

   SETPGRP;

#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX))
   if ((fd = open("/dev/tty", O_RDWR)) >= 0) {  
      /* disassociate contolling tty */
      ioctl(fd, TIOCNOTTY, (char *) NULL);
      close(fd);
   }
#endif

   if ((pid=fork())!= 0) {
      if (pid<0) {
         CRITICAL((SGE_EVENT, MSG_PROC_SECONDFORKFAILED_S , strerror(errno)));
      }
      exit(0);
   }

#if defined(__sgi) || defined(ALPHA)
   /* The yp library may have open sockets 
      when closing all fds also the socket fd of the yp library gets closed
      when called again yp library functions are confused since they 
      assume fds are already open. Thus we shutdown the yp library regularly 
      before closing all sockets */
  getdomainname(domname, sizeof(domname));
  yp_unbind(domname);
#endif

   /* close all file descriptors */
   sge_close_all_fds(keep_open);

   /* new descriptors acquired for stdin, stdout, stderr should be 0,1,2 */
   if (open("/dev/null",O_RDONLY,0)!=0)
      SGE_EXIT(0);
   if (open("/dev/null",O_WRONLY,0)!=1)
      SGE_EXIT(0);
   if (open("/dev/null",O_WRONLY,0)!=2)
      SGE_EXIT(0);

   SETPGRP;

   me.daemonized = 1;
   
   DEXIT;
   return 1;
}


/*-------------------------------------------------------------
 *  occupy_first_three
 *
 *  returns
 *     -1  on success
 *      0 when there are problems with stdout
 *      1 when there are problems with stdin
 *      2 when there are problems with stderr
 *-------------------------------------------------------------*/
int occupy_first_three()
{
   SGE_STRUCT_STAT buf;

   DENTER(TOP_LAYER, "occupy_first_three");

   if (SGE_FSTAT(0, &buf)) {
      if ((open("/dev/null",O_RDONLY,0))!=0) {
         DEXIT;
         return 0;
      }
   }

   if (SGE_FSTAT(1, &buf)) {
      if ((open("/dev/null",O_WRONLY,0))!=1) {
         DEXIT;
         return 1;
      }
   }

   if (SGE_FSTAT(2, &buf)) {
      if ((open("/dev/null",O_WRONLY,0))!=2) {
         DEXIT;
         return 2;
      }
   }

   DEXIT;
   return -1;
}

/*---------------------------------------------------------------
 * Name:  sge_close_all_fds
 * Descr: close all file descriptors
 *----------------------------------------------------------------*/
void sge_close_all_fds(
fd_set *keep_open
) {
/* JG: trying to close insights (insure) internal fd will be rejected */
#ifdef __INSIGHT__
_Insight_set_option("suppress", "USER_ERROR");
#endif
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
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "USER_ERROR");
#endif
}       

