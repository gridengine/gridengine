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
#include "sig_handlers.h"

#include "msg_common.h"
#include "msg_utilib.h"


/* pipe for sge_daemonize_prepare() and sge_daemonize_finalize() */
static int fd_pipe[2];

static int fd_compare(const void* fd1, const void* fd2);
static void sge_close_fd(int fd);

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
*     pid_t *pids           - pid array
*     int max_pids          - size of pid array
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
      DRETURN(-1);
   }

   while (!feof(fp_out) && num_of_pids < max_pids) {
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
   DRETURN(num_of_pids);
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

   DENTER(TOP_LAYER, "sge_checkprog");

   command_pid = sge_peopen("/bin/sh", 0, pscommand, NULL, NULL, 
                        &fp_in, &fp_out, &fp_err, false);

   if (command_pid == -1) {
      DRETURN(-1);
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

   DRETURN(notfound);
}

/****** uti/os/sge_daemonize_prepare() *****************************************
*  NAME
*     sge_daemonize_prepare() -- prepare daemonize of process
*
*  SYNOPSIS
*     int sge_daemonize_prepare(void) 
*
*  FUNCTION
*     The parent process will wait for the child's successful daemonizing.
*     The client process will report successful daemonizing by a call to
*     sge_daemonize_finalize().
*     The parent process will exit with one of the following exit states:
*
*     typedef enum uti_deamonize_state_type {
*        SGE_DEAMONIZE_OK           = 0,  ok 
*        SGE_DAEMONIZE_DEAD_CHILD   = 1,  child exited before sending state 
*        SGE_DAEMONIZE_TIMEOUT      = 2   timeout whild waiting for state 
*     } uti_deamonize_state_t;
*
*     Daemonize the current application. Throws ourself into the
*     background and dissassociates from any controlling ttys.
*     Don't close filedescriptors mentioned in 'keep_open'.
*      
*     sge_daemonize_prepare() and sge_daemonize_finalize() will replace
*     sge_daemonize() for multithreaded applications.
*     
*     sge_daemonize_prepare() must be called before starting any thread. 
*
*
*  INPUTS
*     void - none
*
*  RESULT
*     int - true on success, false on error
*
*  SEE ALSO
*     uti/os/sge_daemonize_finalize()
*******************************************************************************/
bool sge_daemonize_prepare(sge_gdi_ctx_class_t *ctx) {
   pid_t pid;
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   int fd;
#endif

#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
#  if defined(ALPHA)
   extern int getdomainname(char *, int);
#  endif
   char domname[256];
#endif

   int is_daemonized = ctx->is_daemonized(ctx);

   DENTER(TOP_LAYER, "sge_daemonize_prepare");

#ifndef NO_SGE_COMPILE_DEBUG
   if (TRACEON) {
      DRETURN(false);
   }
#endif

   if (is_daemonized) {
      DRETURN(true);
   }

   /* create pipe */
   if ( pipe(fd_pipe) < 0) {
      CRITICAL((SGE_EVENT, MSG_UTI_DAEMONIZE_CANT_PIPE));
      DRETURN(false);
   }

   if ( fcntl(fd_pipe[0], F_SETFL, O_NONBLOCK) != 0) {
      CRITICAL((SGE_EVENT, MSG_UTI_DAEMONIZE_CANT_FCNTL_PIPE));
      DRETURN(false);
   }

   /* close all fd's except pipe and first 3 */
   {
      int keep_open[5];
      keep_open[0] = 0;
      keep_open[1] = 1;
      keep_open[2] = 2;
      keep_open[3] = fd_pipe[0];
      keep_open[4] = fd_pipe[1];
      sge_close_all_fds(keep_open, 5);
   }

   /* first fork */
   pid=fork();
   if (pid <0) {
      CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      DEXIT;
      exit(1);
   }

   if (pid > 0) {
      char line[256];
      int line_p = 0;
      int retries = 60;
      int exit_status = SGE_DAEMONIZE_TIMEOUT;
      int back;
      int errno_value = 0;

      /* close send pipe */
      close(fd_pipe[1]);

      /* check pipe for message from child */
      while (line_p < 4 && retries-- > 0) {
         errno = 0;
         back = read(fd_pipe[0], &line[line_p], 1);
         errno_value = errno;
         if (back > 0) {
            line_p++;
         } else {
            if (back != -1) {
               if (errno_value != EAGAIN ) {
                  retries=0;
                  exit_status = SGE_DAEMONIZE_DEAD_CHILD;
               }
            }
            DPRINTF(("back=%d errno=%d\n",back,errno_value));
            sleep(1);
         }
      }
         
      if (line_p >= 4) {
         line[3] = 0;
         exit_status = atoi(line);
         DPRINTF(("received: \"%d\"\n", exit_status));
      }

      switch(exit_status) {
         case SGE_DEAMONIZE_OK:
            INFO((SGE_EVENT, MSG_UTI_DAEMONIZE_OK));
            break;
         case SGE_DAEMONIZE_DEAD_CHILD:
            WARNING((SGE_EVENT, MSG_UTI_DAEMONIZE_DEAD_CHILD));
            break;
         case SGE_DAEMONIZE_TIMEOUT:
            WARNING((SGE_EVENT, MSG_UTI_DAEMONIZE_TIMEOUT));
            break;
      }
      /* close read pipe */
      close(fd_pipe[0]);
      DEXIT;
      exit(exit_status); /* parent exit */
   }

   /* child */
   SETPGRP;

#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
      /* disassociate contolling tty */
      ioctl(fd, TIOCNOTTY, (char *) NULL);
      close(fd);
   }
#endif
   

   /* second fork */
   pid=fork();
   if (pid < 0) {
      CRITICAL((SGE_EVENT, MSG_PROC_SECONDFORKFAILED_S , strerror(errno)));
      DEXIT;
      exit(1);
   }
   if ( pid > 0) {
      /* close read and write pipe for second child and exit */
      close(fd_pipe[0]);
      close(fd_pipe[1]);
      exit(0);
   }

   /* child of child */

   /* close read pipe */
   close(fd_pipe[0]);

#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
   /* The yp library may have open sockets
      when closing all fds also the socket fd of the yp library gets closed
      when called again yp library functions are confused since they
      assume fds are already open. Thus we shutdown the yp library regularly
      before closing all sockets */
   getdomainname(domname, sizeof(domname));
   yp_unbind(domname);
#endif

   
   DRETURN(true);
}

/****** uti/os/sge_daemonize_finalize() ****************************************
*  NAME
*     sge_daemonize_finalize() -- finalize daemonize process
*
*  SYNOPSIS
*     int sge_daemonize_finalize(fd_set *keep_open) 
*
*  FUNCTION
*     report successful daemonizing to the parent process and close
*     all file descriptors. Set file descirptors 0, 1 and 2 to /dev/null 
*
*     sge_daemonize_prepare() and sge_daemonize_finalize() will replace
*     sge_daemonize() for multithreades applications.
*
*     sge_daemonize_finalize() must be called by the thread who have called
*     sge_daemonize_prepare().
*
*  INPUTS
*     fd_set *keep_open - file descriptor set to keep open
*
*  RESULT
*     int - true on success
*
*  SEE ALSO
*     uti/os/sge_daemonize_prepare()
*******************************************************************************/
bool sge_daemonize_finalize(sge_gdi_ctx_class_t *ctx) 
{
   int failed_fd;
   char tmp_buffer[4];
   int is_daemonized = ctx->is_daemonized(ctx);

   DENTER(TOP_LAYER, "sge_daemonize_finalize");

   /* don't call this function twice */
   if (is_daemonized) {
      DRETURN(true);
   }

   /* The response id has 4 byte, send it to father process */
   snprintf(tmp_buffer, 4, "%3d", SGE_DEAMONIZE_OK );
   write(fd_pipe[1], tmp_buffer, 4);

   sleep(2); /* give father time to read the status */

   /* close write pipe */
   close(fd_pipe[1]);

   /* close first three file descriptors */
#ifndef __INSURE__
   close(0);
   close(1);
   close(2);
   
   /* new descriptors acquired for stdin, stdout, stderr should be 0,1,2 */
   failed_fd = sge_occupy_first_three();
   if (failed_fd  != -1) {
      CRITICAL((SGE_EVENT, MSG_CANNOT_REDIRECT_STDINOUTERR_I, failed_fd));
      SGE_EXIT(NULL, 0);
   }
#endif

   SETPGRP;

   /* now have finished daemonizing */
   ctx->set_daemonized(ctx, true);

   DRETURN(true);
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
*     args   optional args
*
*  RESULT
*     int - Successfull?
*         1 - Yes
*         0 - No
*
*  NOTES
*     MT-NOTES: sge_daemonize() is not MT safe
******************************************************************************/
int sge_daemonize(int *keep_open, unsigned long nr_of_fds, sge_gdi_ctx_class_t *ctx)
{

#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
   int fd;
#endif
 
#if defined(__sgi) || defined(ALPHA) || defined(HP1164)
#  if defined(ALPHA)
   extern int getdomainname(char *, int);
#  endif
   char domname[256];
#endif
   pid_t pid;
   int failed_fd;
 
   DENTER(TOP_LAYER, "sge_daemonize");
 
#ifndef NO_SGE_COMPILE_DEBUG
   if (TRACEON) {
      DRETURN(0);
   }
#endif
 
   if (ctx->is_daemonized(ctx)) {
      DRETURN(1);
   }
 
   if ((pid=fork())!= 0) {             /* 1st child not pgrp leader */
      if (pid<0) {
         CRITICAL((SGE_EVENT, MSG_PROC_FIRSTFORKFAILED_S , strerror(errno)));
      }
      exit(0);
   }
 
   SETPGRP;                      
 
#if !(defined(__hpux) || defined(CRAY) || defined(WIN32) || defined(SINIX) || defined(INTERIX))
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
   sge_close_all_fds(keep_open, nr_of_fds);
 
   /* new descriptors acquired for stdin, stdout, stderr should be 0,1,2 */
   failed_fd = sge_occupy_first_three();
   if (failed_fd  != -1) {
      CRITICAL((SGE_EVENT, MSG_CANNOT_REDIRECT_STDINOUTERR_I, failed_fd));
      SGE_EXIT(NULL, 0);
   }

   SETPGRP;
 
   ctx->set_daemonized(ctx, true);
 
   DRETURN(1);
}

/****** uti/os/redirect_to_dev_null() ******************************************
*  NAME
*     redirect_to_dev_null() -- redirect a channel to /dev/null
*
*  SYNOPSIS
*     int redirect_to_dev_null(int target, int mode) 
*
*  FUNCTION
*     Attaches a certain filedescriptor to /dev/null.
*
*  INPUTS
*     int target - file descriptor
*     int mode   - mode for open
*
*  RESULT
*     int - target fd number if everything was ok,
*           else -1
*
*  NOTES
*     MT-NOTE: redirect_to_dev_null() is MT safe 
*
*******************************************************************************/
int redirect_to_dev_null(int target, int mode)
{
   SGE_STRUCT_STAT buf;

   if (SGE_FSTAT(target, &buf)) {
      if ((open("/dev/null", mode, 0)) != target) {
         return target;
      }
   }

   return -1;
}

/****** uti/os/sge_occupy_first_three() ***************************************
*  NAME
*     sge_occupy_first_three() -- Open descriptor 0, 1, 2 to /dev/null
*
*  SYNOPSIS
*     int sge_occupy_first_three(void)
*
*  FUNCTION
*     Occupy the first three filedescriptors, if not available. This is done
*     to be sure that a communication by a socket will not get any "forgotten"
*     print output from code.
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
*
*  SEE ALSO
*     uti/os/redirect_to_dev_null()
*     uti/os/sge_close_all_fds()
******************************************************************************/
int sge_occupy_first_three(void)
{
   int ret = -1;

   DENTER(TOP_LAYER, "occupy_first_three");

   ret = redirect_to_dev_null(0, O_RDONLY);

   if (ret == -1) {
      ret = redirect_to_dev_null(1, O_WRONLY);
   }

   if (ret == -1) {
      ret = redirect_to_dev_null(2, O_WRONLY);
   }

   DRETURN(ret);
}  

#ifdef __INSURE__
extern int _insure_is_internal_fd(int);
#endif

/****** uti/os/sge_get_max_fd() ************************************************
*  NAME
*     sge_get_max_fd() -- get max filedescriptor count
*
*  SYNOPSIS
*     int sge_get_max_fd(void) 
*
*  FUNCTION
*     This function returns the nr of file descriptors which are available 
*     (Where fd 0 is the first one).
*     So the highest file descriptor value is: max_fd - 1.
*
*  INPUTS
*     void - no input paramteres
*
*  RESULT
*     int - max. possible open file descriptor count on this system
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int sge_get_max_fd(void) {

#ifndef WIN32NATIVE
#ifndef USE_POLL
   return sysconf(_SC_OPEN_MAX) > FD_SETSIZE ? FD_SETSIZE : sysconf(_SC_OPEN_MAX);
#else
   return sysconf(_SC_OPEN_MAX);
#endif
#else /* WIN32NATIVE */
   return FD_SETSIZE;
   /* detect maximal number of fds under NT/W2000 (env: Files)*/
#endif /* WIN32NATIVE */
}

/****** uti/os/fd_compare() ****************************************************
*  NAME
*     fd_compare() -- file descriptor compare function for qsort()
*
*  SYNOPSIS
*     static int fd_compare(const void* fd1, const void* fd2) 
*
*  FUNCTION
*     qsort() needs a callback function to compare two filedescriptors for
*     sorting them. This is the implementation to value the difference of two
*     file descriptors. If one paramter is NULL, only the pointers are
*     used for the comparsion.
*     Used by sge_close_all_fds().
*
*  INPUTS
*     const void* fd1 - pointer to an int (file descriptor 1)
*     const void* fd2 - pointer to an int (file descriptor 2)
*
*  RESULT
*     static int - compare result (1, 0 or -1)
*                  1  : fd1 > fd2
*                  0  : fd1 == fd2
*                  -1 : fd1 < fd2
*
*  NOTES
*     MT-NOTE: fd_compare() is MT safe 
*
*  SEE ALSO
*     uti/os/sge_close_all_fds()
*******************************************************************************/
static int fd_compare(const void* fd1, const void* fd2) {
   int* i1 = (int*) fd1;
   int* i2 = (int*) fd2;

   /* If there are NULL pointer we also try to compare them */
   if (i1 == NULL || i2 == NULL) {
      if (i1 > i2) {
         return 1;
      }
      if (i1 < i2) {
         return -1;
      }
      return 0;
   }

   if (*i1 > *i2) {
      return 1;
   }
   if (*i1 < *i2) {
      return -1;
   }
   return 0;
}


/****** uti/os/sge_close_fd() **************************************************
*  NAME
*     sge_close_fd() -- close a file descriptor
*
*  SYNOPSIS
*     static void sge_close_fd(int fd) 
*
*  FUNCTION
*     This function closes the specified file descriptor on an architecture
*     specific way. If __INSURE__ is defined during compile time and it is an
*     fd used by insure the file descriptor is not closed.
*
*  INPUTS
*     int fd - file descriptor number to close
*
*  RESULT
*     static void - no return value
*
*  SEE ALSO
*     uti/os/sge_close_all_fds()
*******************************************************************************/
static void sge_close_fd(int fd) {
#ifdef __INSURE__
   if (_insure_is_internal_fd(fd)) {
      return;
   }
#endif
#ifndef WIN32NATIVE
   close(fd);
#else
   closesocket(fd);
#endif
}

/****** uti/os/sge_close_all_fds() *********************************************
*  NAME
*     sge_close_all_fds() -- close all open file descriptors
*
*  SYNOPSIS
*     void sge_close_all_fds(int* keep_open, unsigned long nr_of_fds) 
*
*  FUNCTION
*     This function is used to close all possible open file descriptors for
*     the current process. This is done by getting the max. possible file
*     descriptor count and looping over all file descriptors and calling
*     sge_close_fd().
*
*     It is possible to specify a file descriptor set which should not be
*     closed.
*
*  INPUTS
*     int* keep_open          - integer array which contains file descriptor
*                               ids which should not be closed
*                             - if this value is set to NULL nr_of_fds is
*                               ignored
*     unsigned long nr_of_fds - nr of filedescriptors in the keep_open array
*
*  RESULT
*     void - no result
*
*  SEE ALSO
*     uti/os/sge_close_fd()
*******************************************************************************/
void sge_close_all_fds(int* keep_open, unsigned long nr_of_keep_open_entries) {
   int maxfd = sge_get_max_fd();
   int fd = 0;
   if (keep_open == NULL) {
      /* if we do not have any keep_open we can delete all fds */
      for (fd = 0; fd < maxfd; fd++) {
         sge_close_fd(fd);
      }
   } else {
      int           current_fd_keep_open  = 0;
      unsigned long keep_open_array_index = 0;
      int           current_fd_to_close   = 0;

      /* First sort the keep open list */
      qsort((void*) keep_open, nr_of_keep_open_entries, sizeof(int), fd_compare);

      /* Now go over the int array and do a close loop to the current value */
      for (keep_open_array_index = 0; keep_open_array_index < nr_of_keep_open_entries; keep_open_array_index++) {

         /* test if keep open fd is a valid fd */
         current_fd_keep_open = keep_open[keep_open_array_index];
         if (current_fd_keep_open < 0 || current_fd_keep_open >= maxfd) {
            continue;
         }

         /* we can close all fds up to current_fd_keep_open */
         for (fd = current_fd_to_close; fd < current_fd_keep_open; fd++) {
            sge_close_fd(fd);
         }

         /*
          * now we reached current_fd_keep_open, simple set current_fd_to_close to
          * current_fd_keep_open + 1 for the next run (=skip current_fd_keep_open)
          */
         current_fd_to_close = current_fd_keep_open + 1;
      }
      
      /* Now close up to fd nr (max_fd - 1)  */
      for (fd = current_fd_to_close; fd < maxfd; fd++) {
         sge_close_fd(fd);
      }
   }
}

/****** uti/os/sge_dup_fd_above_stderr() **************************************
*  NAME
*     sge_dup_fd_above_stderr() -- Make sure a fd is >=3
*
*  SYNOPSIS
*     int sge_dup_fd_above_stderr(int *fd) 
*
*  FUNCTION
*     This function checks if the given fd is <3, if yes it dups it to be >=3.
*     The fd obtained by open(), socket(), pipe(), etc. can be <3 if stdin, 
*     stdout and/or stderr are closed. As it is difficult for an application
*     to determine if the first three fds are connected to the std-handles or
*     something else and because many programmers just rely on these three fds
*     to be connected to the std-handles, this function makes sure that it 
*     doesn't use these three fds.
*
*  INPUTS
*     int *fd - pointer to the fd which is to be checked and dupped.
*
*  RESULT
*     int - 0: Ok
*          >0: errno
*
*  SEE ALSO
*******************************************************************************/
int sge_dup_fd_above_stderr(int *fd) 
{
   if (fd == NULL) {
      return EINVAL;
   }
   /* 
    * make sure the provided *fd is not 0, 1 or 2  - anyone can close
    * stdin, stdout or stderr without checking what these really are
    */
   if (*fd < 3) {
      int tmp_fd;
      if ((tmp_fd = fcntl(*fd, F_DUPFD, 3)) == -1) {
         return errno;
      } 
      close(*fd);
      *fd = tmp_fd;
   }
   return 0;
}
