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
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>   
#include <sys/time.h>
#include <sys/resource.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef SIGTSTP
#   include <sys/file.h>
#endif
 
#if defined(SOLARIS)
#   include <sys/termios.h>
#endif
 
#if defined(__sgi) || defined(ALPHA)
#   include <rpcsvc/ypclnt.h>
#endif
 
#if defined(AIX)
#   include <sys/select.h>
#endif    

#include "sgermon.h"
#include "sge_unistd.h"
#include "sge_stdio.h"
#include "sge_os.h"
#include "sge_prog.h"  
#include "sge_log.h"

#include "msg_utilib.h"

/****** uti/os/sge_get_pids() *************************************************
*  NAME
*     sge_get_pids() -- Return all "pids" of a running processes 
*
*  SYNOPSIS
*     int sge_get_pids(pid_t *pids, int max_pids, const char *name, 
*                      const char *pscommand) 
*
*  FUNCTION
*     Return all "pids" of a running processes with given "name". 
*     Only first 8 characters of "name" are significant.
*     Checks only basename of command after "/".
*
*  INPUTS
*     pid_t *pids           - pid list 
*     int max_pids          - size of pid list 
*     const char *name      - name 
*     const char *pscommand - ps commandline
*
*  RESULT
*     int - Result 
*         0 - No program with given name found
*        >0 - Number of processes with "name" 
*        -1 - Error
*
*  NOTES
*     MT-NOTES: sge_get_pids() is not MT safe
******************************************************************************/
int sge_get_pids(pid_t *pids, int max_pids, const char *name, 
             const char *pscommand) 
{
   FILE *fp_in, *fp_out, *fp_err;
   char buf[10000], *ptr;
   int num_of_pids = 0, last, len;
   pid_t pid, command_pid;

   DENTER(TOP_LAYER, "sge_get_pids");
   
   command_pid = sge_peopen("/bin/sh", 0, pscommand, NULL, NULL, 
                        &fp_in, &fp_out, &fp_err, false);

   if (command_pid == -1) {
      DEXIT;
      return -1;
   }

   while (!feof(fp_out)) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {

            /* handles first line of ps command */
            if ((pid = (pid_t) atoi(buf)) <= 0)
               continue;

            /* strip off trailing white spaces */
            last = len - 1;
            while (last >= 0 && isspace((int) buf[last])) {
               buf[last] = '\0';
               last--;
            }
            
            /* set pointer to first character of process name */
            while (last >= 0 && !isspace((int) buf[last]))
               last--;
            last++;

            /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */
            
            /* get basename of program */
            ptr = strrchr(&buf[last], '/');
            if (ptr)
               ptr++;
            else
               ptr = &buf[last];                  
   
            /* check if process has given name */
            if (!strncmp(ptr, name, 8))
               pids[num_of_pids++] = pid;
         }
      }
   }            

   sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);
   return num_of_pids;
}

/****** uti/os/sge_contains_pid() *********************************************
*  NAME
*     sge_contains_pid() -- Checks whether pid array contains pid 
*
*  SYNOPSIS
*     int sge_contains_pid(pid_t pid, pid_t *pids, int npids) 
*
*  FUNCTION
*     whether pid array contains pid 
*
*  INPUTS
*     pid_t pid   - process id 
*     pid_t *pids - pid array 
*     int npids   - number of pids in array 
*
*  RESULT
*     int - result state
*         0 - pid was not found
*         1 - pid was found
*
*  NOTES
*     MT-NOTES: sge_contains_pid() is MT safe
******************************************************************************/
int sge_contains_pid(pid_t pid, pid_t *pids, int npids) 
{
   int i;

   for (i = 0; i < npids; i++) {
      if (pids[i] == pid) {
         return 1;
      }
   }
   return 0;
}

/****** uti/os/sge_checkprog() ************************************************
*  NAME
*     sge_checkprog() -- Has "pid" of a running process the given "name" 
*
*  SYNOPSIS
*     int sge_checkprog(pid_t pid, const char *name, 
*                       const char *pscommand) 
*
*  FUNCTION
*     Check if "pid" of a running process has given "name".
*     Only first 8 characters of "name" are significant.
*     Check only basename of command after "/". 
*
*  INPUTS
*     pid_t pid             - process id 
*     const char *name      - process name 
*     const char *pscommand - ps commandline 
*
*  RESULT
*     int - result state
*         0 - Process with "pid" has "name"
*         1 - No such pid or pid has other name
*        -1 - error occurred (mostly sge_peopen() failed) 
*
*  NOTES
*     MT-NOTES: sge_checkprog() is not MT safe
******************************************************************************/
int sge_checkprog(pid_t pid, const char *name, const char *pscommand) 
{
   FILE *fp_in, *fp_out, *fp_err;
   char buf[1000], *ptr;
   pid_t command_pid, pidfound;
   int len, last, notfound;
#if defined(QIDL) && defined(SOLARIS64)
   sigset_t sigset, osigset;
#endif

   DENTER(TOP_LAYER, "sge_checkprog");

#if defined(QIDL) && defined(SOLARIS64)
   {
      sigemptyset(&sigset);
      sigaddset(&sigset, SIGCLD);
      sigprocmask(SIG_BLOCK, &sigset, &osigset);
   }
#endif

   command_pid = sge_peopen("/bin/sh", 0, pscommand, NULL, NULL, 
                        &fp_in, &fp_out, &fp_err, false);

   if (command_pid == -1) {
      DEXIT;
#if defined(QIDL) && defined(SOLARIS64) 
      sigprocmask(SIG_SETMASK, &osigset, NULL);
#endif  
      return -1;
   }

   notfound = 1;
   while (!feof(fp_out)) {
      if ((fgets(buf, sizeof(buf), fp_out))) {
         if ((len = strlen(buf))) {
            pidfound = (pid_t) atoi(buf);

            if (pidfound == pid) {
               last = len - 1;
               DPRINTF(("last pos in line: %d\n", last));
               while (last >= 0 && isspace((int) buf[last])) {
                  buf[last] = '\0';
                  last--;
               }

               /* DPRINTF(("last pos in line now: %d\n", last)); */
               
               while (last >= 0 && !isspace((int) buf[last]))
                  last--;
               last++;

               /* DPRINTF(("pid: %d - progname: >%s<\n", pid, &buf[last])); */ 

               /* get basename of program */
               ptr = strrchr(&buf[last], '/');
	       if (ptr)
	          ptr++;
	       else
	          ptr = &buf[last];

               if (!strncmp(ptr, name, 8)) {
                  notfound = 0;
                  break;
               }
               else
                  break;
            }
         }
      }
   }

   sge_peclose(command_pid, fp_in, fp_out, fp_err, NULL);

#if defined(QIDL) && defined(SOLARIS64) 
   sigprocmask(SIG_SETMASK, &osigset, NULL);
#endif  
   
   DEXIT;
   return notfound;
}

/****** uti/os/sge_daemonize() ************************************************
*  NAME
*     sge_daemonize() -- Daemonize the current application
*
*  SYNOPSIS
*     int sge_daemonize(fd_set *keep_open)
*
*  FUNCTION
*     Daemonize the current application. Throws ourself into the
*     background and dissassociates from any controlling ttys.
*     Don't close filedescriptors mentioned in 'keep_open'.
*
*  INPUTS
*     fd_set *keep_open - bitmask
*
*  RESULT
*     int - Successfull?
*         1 - Yes
*         0 - No
*
*  NOTES
*     MT-NOTES: sge_daemonize() is not MT safe
******************************************************************************/
int sge_daemonize(fd_set *keep_open)
{
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX))
   int fd;
#endif
 
#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
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
 
   if (uti_state_get_daemonized()) {
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
 
#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
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
 
   uti_state_set_daemonized(1);
 
   DEXIT;
   return 1;
}     

/****** uti/os/sge_occupy_first_three() ***************************************
*  NAME
*     sge_occupy_first_three() -- Open descriptor 0, 1, 2 to /dev/null
*
*  SYNOPSIS
*     int sge_occupy_first_three(void)
*
*  FUNCTION
*     Occupy the first three filedescriptors.
*
*  RESULT
*     int - error state
*        -1 - OK
*         0 - there are problems with stdin
*         1 - there are problems with stdout
*         2 - there are problems with stderr
*
*  NOTES
*     MT-NOTE: sge_occupy_first_three() is MT safe
******************************************************************************/
int sge_occupy_first_three(void)
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

/****** uti/os/sge_close_all_fds() ********************************************
*  NAME
*     sge_close_all_fds() -- close (all) file descriptors
*
*  SYNOPSIS
*     void sge_close_all_fds(fd_set *keep_open)
*
*  FUNCTION
*     Close all filedescriptors but ignore those mentioned
*     in 'keep_open'.
*
*  INPUTS
*     fd_set *keep_open - bitmask
*
*  NOTES
*     MT-NOTE: sge_close_all_fds() is MT safe
******************************************************************************/
void sge_close_all_fds(fd_set *keep_open)
{
/* JG: trying to close insights (insure) internal fd will be rejected */
#ifdef __INSIGHT__
_Insight_set_option("suppress", "USER_ERROR");
#endif
   int fd;
   int maxfd;
 
#ifndef WIN32NATIVE
   maxfd = sysconf(_SC_OPEN_MAX) > FD_SETSIZE ? \
     FD_SETSIZE : sysconf(_SC_OPEN_MAX);
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

