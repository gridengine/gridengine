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
#define SETPGRP setpgid(getpid(),getpid())

/* SETPGRP - set process group */
/* #if defined(_UNICOS) || defined(SOLARIS) || defined(HPUX)
   #define SETPGRP setpgrp()
   #else
   #ifdef sgi
   #define SETPGRP BSDsetpgrp(getpid(),getpid())
   #else
   #define SETPGRP setpgrp(getpid(),getpid())
   #endif
   #endif */

/* SETPRIORITY - set process priority */
#ifdef __convex__
#define SETPRIORITY(niceval) setpriority(PRIO_PGRP,getpgrp(),niceval)
#else
#if defined(_UNICOS) || defined(SOLARIS)
#define SETPRIORITY(niceval) nice(niceval + 20)
#else
#define SETPRIORITY(niceval) setpriority(PRIO_PGRP,0,niceval)
#endif
#endif

#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"

#include "rmon_daemon.h"
#include "rmon_err.h"
#include "rmon_rmon.h"
#include "msg_rmon.h"


/*************************************************************************/
int rmon_daemon(
mayclose_function_type *mayclose 
) {
   int fd;
   long _nfile;

#define FUNC "rmon_daemon"

   DENTER;

   if (((_nfile = sysconf(_SC_OPEN_MAX))) == -1) {
      rmon_errf(TRIVIAL, MSG_RMON_DAEMONUNABLETOSYSCONFSCOPENMAX);
      DEXIT;
      return 0;
   }

   umask((unsigned short) 0);
   if (TRACEON) {
      DEXIT;
      return (0);
   }
   /* chdir("/"); */
   if (DFORK() != 0) {          /* 1st child not pgrp leader */
      DEXIT;
      exit(0);
   }

   SETPGRP;
#if !(defined(__hpux) || defined(_UNICOS) || defined(SOLARIS))
   if ((fd = open("/dev/tty", O_RDWR)) >= 0) {  /* disassociate contolling tty */
      ioctl(fd, TIOCNOTTY, (char *) NULL);
      close(fd);
   }
#endif
   if (DFORK()) {
      DEXIT;
      exit(0);
   }

   for (fd = 0; fd < _nfile; fd++)
      if (mayclose(fd))
         close(fd);
   SETPGRP;
   SETPRIORITY(-10);

   DEXIT;
   return (0);
}
