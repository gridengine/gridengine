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
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#ifdef AIX41
#  include <sys/select.h>
#endif

#include "basis_types.h"
#include "config_file.h"
#include "err_trace.h"
#include "qlogin_starter.h"
#include "sge_arch.h"


static char err_str[1024];

/****** shepherd/write_to_qrsh() ***************************************
*
*  NAME
*     write_to_qrsh -- short description
*
*  SYNOPSIS
*     #include "qlogin_starter.h"
*     void write_to_qrsh(char *data);
*
*  FUNCTION
*     Writes the contents of <data> to an other (remote) process over
*     a socket connection.
*     Host and port of the communication partner are read from the 
*     configuration entry "qrsh_control_port".
*     A socket client connection is opened to the named host and port,
*     and the data is written.
*
*  INPUTS
*     data - null terminated string with data to write
*
*  RESULT
*     0, if function finishes correctly
*     1, if the config entry qrsh_control_port does not exist
*     2, if qrsh_control_port contains illegal data
*     3, if opening the socket failed
*     4, if the hostname cannot be resolved
*     5, if connecting to the socket fails
*     6, if writing the data fails
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/
int write_to_qrsh(const char *data)
{
   char *address = NULL;
   char *host;
   char *c;
   int   port    = 0;
   int   sock    = 0;
   int datalen   = 0;
   struct sockaddr_in server;
   struct hostent *hp;

   SHEPHERD_TRACE((err_str, "write_to_qrsh - data = %s", data));

   /* read destination host and port from config */
   address = get_conf_val("qrsh_control_port");

   if(address == NULL) {
      SHEPHERD_TRACE((err_str, "config does not contain entry for qrsh_control_port"));
      return 1;
   }

   SHEPHERD_TRACE((err_str, "write_to_qrsh - address = %s", address));

   c = strchr(address, ':');
   if(c == NULL) {
      SHEPHERD_TRACE((err_str, "illegal value for qrsh_control_port: \"%s\". Should be host:port", address));
      return 2;
   }
  
   *c = 0;
   host = address;
   port = atoi(c + 1);
  
   SHEPHERD_TRACE((err_str, "write_to_qrsh - host = %s, port = %d", host, port));

   /* create socket. */
   sock = socket( AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      SHEPHERD_TRACE(("error opening stream socket: %s", strerror(errno)));
      return 3;
   }

   /* connect socket using name specified by command line. */
   server.sin_family = AF_INET;
   hp = gethostbyname(host);
  
   /*
   * gethostbyname returns a structure including the network address
   * of the specified host.
   */
   if (hp == (struct hostent *) 0) {
      SHEPHERD_TRACE((err_str, "%s: unknown host", host));
      close(sock);
      return 4;
   }

   memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
   server.sin_port = htons(port);
 
   if(connect(sock, (struct sockaddr *) &server, sizeof server) == -1) {
      SHEPHERD_TRACE((err_str, "error connecting stream socket: %s", strerror(errno)));
      close(sock);
      return 5;
   }

   /* write data */
   datalen = strlen(data) + 1;
   if(write(sock, data, datalen) != datalen) {
     SHEPHERD_TRACE((err_str, "error writing data to qrsh_control_port"));
     close(sock);
     return 6;
   }

   /* close connection */
   close(sock);
   return 0;
}

/****** shepherd/write_exit_code_to_qrsh() ***************************************
*
*  NAME
*     write_exit_code_to_qrsh -- write an exit code to qrsh
*
*  SYNOPSIS
*     #include "qlogin_starter.h"
*     void write_exit_code_to_qrsh(int exit_code)
*
*  FUNCTION
*     If the program handled by this shepherd uses rsh mechanism
*     (configuration value "rsh_daemon" is set), then the function
*     writes an exit code to the corresponding qrsh process via a 
*     socket connection.
*
*     The exit code is either taken from parameter <exit_code>, if it is
*     notequal 0, to signal an error condition in the shepherd,
*     or read from a special file ($TMPDIR/qrsh_exit_code).
*
*  INPUTS
*     exit_code - status of the calling process
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     shepherd/write_to_qrsh()
*
****************************************************************************
*/
void write_exit_code_to_qrsh(int exit_code)
{
   char buffer[1024];
   *buffer = 0;

   /* rshd exited with OK: try to get returncode from qrsh_starter file */
   SHEPHERD_TRACE((err_str, "write_exit_code_to_qrsh(%d)", exit_code));

   /* we only have an error file in TMPDIR in case of rsh, 
      otherwise pass exit_code */ 
   if(search_conf_val("rsh_daemon") != NULL) {
      if(exit_code == 0) {
         char *tmpdir;
         char *taskid;
         FILE *errorfile;
   
         exit_code = 1;
         
         tmpdir = getenv("TMPDIR");
         taskid = search_conf_val("qrsh_task_id");
         SHEPHERD_TRACE((err_str, "write_exit_code_to_qrsh - TMPDIR = "
            "%s, qrsh_task_id = %s", tmpdir ? tmpdir : "0", 
            taskid ? taskid : "0"));
         if(tmpdir) {
            if(taskid) {
               sprintf(buffer, "%s/qrsh_exit_code.%s", tmpdir, taskid);
            } else {   
               sprintf(buffer, "%s/qrsh_exit_code", tmpdir);
            }   
            errorfile = fopen(buffer, "r");
            if(errorfile) {
               if(fscanf(errorfile, "%d", &exit_code) == 1) {
                  SHEPHERD_TRACE((err_str, "error code from remote "
                     "command is %d", exit_code));
               }
               fclose(errorfile);
            }
         }
      }
   }

   /* write exit code as string number to qrsh */
   sprintf(buffer, "%d", exit_code);
   if(write_to_qrsh(buffer) != 0) {
      SHEPHERD_TRACE((err_str, "writing exit code to qrsh failed"));
   }
}

/****** shepherd/get_exit_code_of_qrsh_starter() ***************************************
*
*  NAME
*     get_exit_code_of_qrsh_starter -- short description
*
*  SYNOPSIS
*     #include "qlogin_starter.h"
*     int get_exit_code_of_qrsh_starter(void);
*
*  FUNCTION
*     Reads the exit code from a process started via qrsh - qrsh_starter
*     from a file in the jobs TMPDIR.
*
*  RESULT
*     the exit code of the process
*     1, if an error occured while reading the file
*
****************************************************************************
*/
int get_exit_code_of_qrsh_starter(void)
{
   char buffer[1024];
   int exit_code = 1;

   *buffer = 0;

   /* rshd exited with OK: try to get returncode from qrsh_starter file */
   SHEPHERD_TRACE((err_str, "get_exit_code_of_qrsh_starter()")); 

   /* we only have an error file in TMPDIR in case of rsh, 
    * otherwise pass exit_code */
   if(search_conf_val("rsh_daemon") != NULL) {
      char *tmpdir;
      char *taskid;
      FILE *errorfile;

      /* ### */

      tmpdir = search_conf_val("qrsh_tmpdir");
      taskid = search_conf_val("qrsh_task_id");
      SHEPHERD_TRACE((err_str, "get_exit_code_of_qrsh_starter - TMPDIR = %s,"
         " qrsh_task_id = %s", tmpdir ? tmpdir : "0", taskid ? taskid : "0"));
      if(tmpdir) {
         if(taskid) {
            sprintf(buffer, "%s/qrsh_exit_code.%s", tmpdir, taskid);
         } else {
            sprintf(buffer, "%s/qrsh_exit_code", tmpdir);
         }

         errorfile = fopen(buffer, "r");
         if(errorfile) {
            if(fscanf(errorfile, "%d", &exit_code) == 1) {
               SHEPHERD_TRACE((err_str, "error code from remote command "
                  "is %d", exit_code));
            }
            fclose(errorfile);
         }
      }
   }
   return exit_code;        
}

/****** shepherd/qlogin_starter() ***************************************
*
*  NAME
*     qlogin_starter -- short description
*
*  SYNOPSIS
*     #include "qlogin_starter.h"
*     int qlogin_starter(const char *cwd, char *daemon);
*
*  FUNCTION
*     The function is called from shepherd to start a protocol daemon
*     like telnetd, rshd or rlogind.
*     The mechanism used to call these daemons is that of inetd:
*        - a socket is created (server side, any free port is assigned 
*          by the operating system)
*        - qlogin_starter waits for someone to connect to this socket
*        - the socket file handles are redirected to stdin, stdout 
*          and stderr
*        - the daemon process is started
*     Additionally to the inetd mechanism, the port number and some 
*     other information is sent to the qrsh process that initiated
*     (over qmaster, schedd, execd, shepherd) the qlogin_starter call.
*
*  INPUTS
*     cwd    - the current working directory (the active_jobs directory)
*     daemon - name and path of the daemon to start
*
*  RESULT
*     on success, the function will not return (it exec's)
*      4, if there is a problem with permissions
*      5, if a socket cannot be allocated
*      6, if a socket bind fails
*      7, if socket name (port) cannot be determined
*      8, if environment (to be passed to qrsh) cannot be read
*      9, if sending information to qrsh fails
*     10, if nobody connects to the socket within a one minute
*     11, if the acception of a connecting client fails
*     12, if the execution of the daemon fails
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/
int qlogin_starter(const char *cwd, char *daemon)
{
   int ret;
   int port;
   int fd;
   int sockfd;
   int on = 1;
   int sso = 1;
   int newsfd;
   fd_set fds;
   struct sockaddr_in serv_addr;
   struct timeval timeout;
   char buffer[2048];
   char *args[20]; /* !!!! should be dynamically allocated */
   int argc = 0;
   const char *sge_root = NULL;
   char *arch = NULL;
#ifdef AIX51
   unsigned long length;
   unsigned long len;
#else
   int length;
   int len;
#endif

   len = sizeof(serv_addr);
   SHEPHERD_TRACE((err_str, "uid = " uid_t_fmt ", euid = " uid_t_fmt ", gid = " gid_t_fmt ", egid = " gid_t_fmt "", 
                   getuid(), geteuid(), getgid(), getegid()));
   
   /* must be root because we must access /dev/something */
   if(setgid(0) || setuid(0) || setegid(0) || seteuid(0)) {
      SHEPHERD_TRACE((err_str, "cannot change uid/gid\n"));
      return 4;
   }

/*#ifdef SOLARIS
   if(setreuid(0, 0)) {
      SHEPHERD_TRACE((err_str, "cannot change uid/gid\n"));
      return 4;
   } 
#endif
*/
   SHEPHERD_TRACE((err_str, "uid = " uid_t_fmt ", euid = " uid_t_fmt ", gid = " gid_t_fmt ", egid = " gid_t_fmt "", 
                   getuid(), geteuid(), getgid(), getegid()));
   
   /* socket stuff from here */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if(sockfd == -1) {
      SHEPHERD_TRACE((err_str, "cannot open socket."));
      return 5;
   }
   SHEPHERD_TRACE((err_str, "using sfd %d", sockfd));

   setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
   
   /* bind an address to socket */
   /* we start somewhere between 1100 and 1224 and count upwards until one
      is free. i hope this doesn't interfere too badly with other
      programs, but i didn't find out how to use a kernel-assigned port */
   /* port = 1100;  */
   /* port = 511; */
   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   /* serv_addr.sin_port = htons(port); */
   serv_addr.sin_port = 0; 
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;

   ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); 
   /* while (((ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) && errno == EINVAL)
      serv_addr.sin_port = htons(++port); */
      /* serv_addr.sin_port = htons(--port); */
   
   if(ret != 0) {
      SHEPHERD_TRACE((err_str, "cannot bind socket: %s", strerror(errno)));
      close(sockfd);
      return 6;
   }

   /* find out assigned port number and pass it to caller */
   length = sizeof(serv_addr);
   if (getsockname(sockfd,(struct sockaddr *) &serv_addr, &length) == -1) {
      SHEPHERD_TRACE((err_str, "getting socket name failed: %s", strerror(errno)));
      close(sockfd);
      return 7;
   }
   
   port = ntohs(serv_addr.sin_port);
   SHEPHERD_TRACE((err_str, "bound to port %d\n", port));
 
   /* send necessary info to qrsh: port + utilbin directory + active job directory */
   sge_root = sge_get_root_dir(0);
   arch = getenv("ARC");
   
   if(sge_root == NULL || arch == NULL) {
      SHEPHERD_TRACE((err_str, "reading environment SGE_ROOT and ARC failed"));
      close(sockfd);
      return 8;
   }
   
   sprintf(buffer, "0:%d:%s/utilbin/%s:%s:%s", port, sge_root, arch, cwd, get_conf_val("host"));
   if(write_to_qrsh(buffer) != 0) {
      SHEPHERD_TRACE((err_str, "communication with qrsh failed"));
      close(sockfd);
      return 9;
   }
   
   /* wait for connection */
   SHEPHERD_TRACE((err_str, "waiting for connection."));
   listen(sockfd, 1);
   /* use a reasonable timeout (60 seconds) to prevent hanging here forever */
   FD_ZERO(&fds);
   FD_SET(sockfd, &fds);
   timeout.tv_sec = 60;
   timeout.tv_usec = 0;
   if(select(sockfd+1, &fds, NULL, NULL, &timeout) < 1) {
      SHEPHERD_TRACE((err_str, "nobody connected to the socket"));
      shutdown(sockfd, 2);
      close(sockfd);
      return 10;
   }
   newsfd = accept(sockfd, (struct sockaddr *)(&serv_addr), &len);
   if(newsfd == -1) {
      SHEPHERD_TRACE((err_str, "error when accepting socket conection"));
      shutdown(sockfd, 2);
      close(sockfd);
      return 11;
   }
   shutdown(sockfd, 2);
   close(sockfd);

   SHEPHERD_TRACE((err_str, "accepted connection on fd %d", newsfd));

   /* don't close on exec */
   fcntl( newsfd, F_SETFD, 0 );

   /* speed up ;-) */
   setsockopt(newsfd, IPPROTO_TCP, TCP_NODELAY, (const char *) &sso, sizeof(int));

   /* use this fd as stdin,out,err */
   dup2( newsfd, 0 );
   dup2( newsfd, 1 );
   dup2( newsfd, 2 );
   
   /* close all the rest */
   for(fd=3; fd<FD_SETSIZE; fd++)
      close(fd);

   SHEPHERD_TRACE((err_str, "daemon to start: |%s|", daemon));

   /* split daemon commandline into single arguments */
   args[argc++] = strtok(daemon, " ");
   while((args[argc++] = strtok(NULL, " ")) != NULL);

{
   int i = 0;
   SHEPHERD_TRACE((err_str, "daemon commandline split to %d arguments", argc));
   while(args[i] != NULL) {
      SHEPHERD_TRACE((err_str, "daemon argv[%d] = |%s|", i, args[i]));
      i++;
   }
}
   /* that it. */
   execv(args[0], args);

   /* oh oh, exec failed */
   /* no way to tell anyone, becuase all FDs are closed */
   /* last chance -> tell parent process */
   return 12;
}
