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
/*----------------------------------------------------
 * qsh.c
 *
 *
 *--------------------------------------------------*/

#ifdef SUN4
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

#include <unistd.h>
#include <string.h>
#include <stdlib.h>    /* need prototype for malloc */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "basis_types.h"
#include "symbols.h"
#include "sge_dstring.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "usage.h"
#include "parse_qsub.h"
#include "parse_job_cull.h"
#include "parse_job_qsh.h"
#include "read_defaults.h"
#include "sge_exit.h"
#include "show_job.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "sge_resource.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_me.h"
#include "utility.h"
#include "sge_copy_append.h"
#include "setup_path.h" 
#include "sge_arch.h"
#include "sge_afsutil.h"
#include "sge_conf.h"
#include "sge_jobL.h"
#include "sge_set_def_sig_mask.h"
#include "sge_qexec.h"
#include "qm_name.h"
#include "sge_pgrp.h"
#include "sge_signal.h"

#include "jb_now.h"
#include "sge_security.h"
#include "msg_clients_common.h"
#include "msg_qsh.h"
#include "msg_common.h"

void write_client_name_cache(const char *cache_path, const char *client_name);
static void add2env(lList **envlpp, const char *name, const char *value);
static int open_qrsh_socket(int *port);
static int wait_for_qrsh_socket(int sock, int timeout);
static char *read_from_qrsh_socket(int msgsock);
static int get_remote_exit_code(int sock);
static const char *quote_argument(const char *arg);
static int parse_result_list(lList *alp, int *alp_error);
static int start_client_program(const char *client_name, 
                                lList *opts_qrsh,
                                const char *host,
                                const char *port,
                                const char *job_dir,
                                const char *utilbin_dir,
                                int is_rsh,
                                int is_rlogin,
                                int nostdin,
                                int noshell,
                                int sock);
static int get_client_server_context(int msgsock, char **port, char **job_dir, char **utilbin_dir, const char **host);
/* static char *get_rshd_name(char *hostname); */
static const char *get_client_name(int is_rsh, int is_rlogin, int inherit_job);
/* static char *get_master_host(lListElem *jep); */
static void set_job_info(lListElem *job, const char *name, int is_qlogin, int is_rsh, int is_rlogin);
/* static lList *parse_script_options(lList *opts_cmdline); */
static lList *parse_qrsh_command(lList *opts_cmdline, char *name, const char **hostname, int existing_job);
static lList *merge_and_order_options(lList **opts_defaults, lList **opts_scriptfile, lList **opts_cmdline);

static void remove_unknown_opts(lList *lp, u_long32 jb_now, int tightly_integrated, int error); 
static void delete_job(u_long32 job_id, lList *lp);

int main(int argc, char **argv);

#define VERBOSE_LOG(x) if(sge_is_verbose()) { fprintf x; fflush(stderr); }

/****** Interactive/qsh/--Introduction ***************************************
*
*  NAME
*     qsh -- qsh, qlogin, qrsh, qrlogin, qrexec
*
*  SYNOPSIS
*     qsh    [options]
*     qlogin [options]
*     qrsh   [options] [arguments]
*
*  FUNCTION
*     Start an interactive job or task.
*     Different behavior is achieved by calling qsh with different names
*     (setting the links qlogin and qrsh to the qsh binary).
*
*     qsh starts an xterm on a host determined by the Sge/SGE scheduler
*     
*     qlogin start a telnet session on a host determined by the Sge/SGE
*     scheduler
*
*     qrsh starts an rsh or rlogin session on a host determined by the
*     Sge/SGE scheduler.
*     qrsh can also be used, to start a task in an already scheduled, 
*     parallel job on a specific host.
*
*  INPUTS
*
*  RESULT
*
*  EXAMPLE
*     qsh -l arch=solaris 
*     
*     qlogin -q balrog
*
*     qrsh -l netscape=1 netscape
*     
*     qsub -pe foo 3 job_script.sh
*     within job_script.sh:
*     qrsh -inherit <hostname> <command> <args>
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     Interactive/qrsh_starter/--Introduction
*
****************************************************************************
*
*/

/****** Interactive/qsh/-Global_Variables ***************************************
*
*  NAME
*     Global Variables -- global variables
*
*  SYNOPSIS
*     extern char **environ;
*     pid_t child_pid;
*
*  FUNCTION
*     environ                    - global array with environment of 
*                                  current process
*     child_pid                  - pid of the child rsh/rlogin/telnet client.
*                                  used to pass signals to child.
*
****************************************************************************
*
*/
extern char **environ;
pid_t child_pid = 0;

/****** Interactive/qsh/-Defines ***************************************
*
*  NAME
*     Defines -- defines/constants 
*
*  SYNOPSIS
*     #define MAX_JOB_NAME 128
*     #define QSH_INTERACTIVE_POLLING_MIN 2
*     #define QSH_BATCH_POLLING_MIN 32
*     #define QSH_POLLING_MAX 256
*     #define QSH_SOCKET_FINAL_TIMEOUT 60
*     #define QRSH_CLIENT_CACHE "qrsh_client_cache"
*
*  FUNCTION
*     MAX_JOB_NAME                - maximum length of job names
*     QSH_INTERACTIVE_POLLING_MIN - first polling interval between two polls
*                                   The actual polling interval will be calculated
*                                   by doubling the interval after each poll until
*                                   QSH_POLLING_MAX is reached.
*     QSH_POLLING_QRSH_MIN        - first polling interval in case of qrsh, qlogin
*     QSH_POLLING_MAX             - final polling interval between two polls
*     QSH_SOCKET_FINAL_TIMEOUT    - timeout in second (and final) wait for
*                                   qlogin_starter to connect to our socket
*     QRSH_CLIENT_CACHE           - name of file to cache client name for use
*                                   in qrsh - qrexec mode
*
****************************************************************************
*
*/
#define MAX_JOB_NAME 128
#define QSH_INTERACTIVE_POLLING_MIN 3
#define QSH_BATCH_POLLING_MIN 32
#define QSH_POLLING_MAX 256
#define QSH_SOCKET_FINAL_TIMEOUT 60
#define QRSH_CLIENT_CACHE "qrsh_client_cache"

static void forward_signal(int sig)
{
   DENTER(TOP_LAYER, "forward_signal");
   if(child_pid > 0) {
      DPRINTF(("forwarding signal %d to child %d\n", sig, child_pid));
      kill(child_pid, sig);
   }
   DEXIT;
}

/****** Interactive/qsh/add2env() ***************************************
*
*  NAME
*     add2env -- add entry to environment list
*
*  SYNOPSIS
*     static void add2env(lList **envlpp, const char *name, const char *value);
*
*  FUNCTION
*     Adds an entry to an environment list.
*     If the environment list does not yet exist (envlpp == NULL),
*     a new list is created and passed back to caller.
*
*  INPUTS
*     envlpp - reference to pointer to environment list, the list will be
*              modified by add2env
*     name   - name of the environment variable
*     value  - value of the environment variable
*
*  RESULT
*     no return value, function changes contents of list envlpp
*
*  NOTES
*     Function should be moved to some library - the same code is contained
*     in other modules.
*
****************************************************************************
*
*/
static void
add2env(lList **envlpp, const char *name, const char *value)
{
   lListElem      *vep;

   vep = lAddElemStr(envlpp, VA_variable, name, VA_Type);
   if (value)
      lSetString(vep, VA_value, value);

   return;
}

/****** Interactive/qsh/open_qrsh_socket() ***************************************
*
*  NAME
*     open_qrsh_socket -- create socket for messages to qrsh
*
*  SYNOPSIS
*     static int open_qrsh_socket(int *port)
*
*  FUNCTION
*     Creates a socket (server side) on any free port (assiged by the
*     operating system), allows access of one client per time.
*     If a serious error occurs, the program will exit.
*
*  INPUTS
*     port - reference to port number
*
*  RESULT
*     function returns the socket file descriptor, 
*     or 0, if an error occured
*     [port] - the port number of the socket is returned in the 
*              reference parameter
*
****************************************************************************
*
*/
static int open_qrsh_socket(int *port) {
   int sock, length;
   struct sockaddr_in server;
 
   DENTER(TOP_LAYER, "open_qrsh_socket");

   /* create socket */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1) {
      ERROR((SGE_EVENT, MSG_QSH_ERROROPENINGSTREAMSOCKET_S, strerror(errno)));
      DEXIT;
      SGE_EXIT(1);
   }

   /* bind socket using wildcards */
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = 0;
   if (bind(sock, (struct sockaddr *) &server, sizeof server) == -1) {
      ERROR((SGE_EVENT, MSG_QSH_ERRORBINDINGSTREAMSOCKET_S, strerror(errno)));
      DEXIT;
      SGE_EXIT(1);
   }
   
   /* find out assigned port number and pass it to caller */
   length = sizeof server;
   if (getsockname(sock,(struct sockaddr *) &server,&length) == -1) {
      ERROR((SGE_EVENT, MSG_QSH_ERRORGETTINGSOCKETNAME_S, strerror(errno)));
      DEXIT;
      SGE_EXIT(1);
   }
   
   *port = ntohs(server.sin_port);
   DPRINTF(("qrsh will listen on port %d\n", *port));

   if(listen(sock, 1) == -1) {
      ERROR((SGE_EVENT, MSG_QSH_ERRORLISTENINGONSOCKETCONNECTION_S, strerror(errno)));
      DEXIT;
      return 0;
   }
   
   DEXIT;
   return sock;
}   

/****** Interactive/qsh/wait_for_qrsh_socket() ***************************************
*
*  NAME
*     wait_for_qrsh_socket -- wait for client connecting to socket
*
*  SYNOPSIS
*     static int wait_for_qrsh_socket(int sock, int timeout)
*
*  FUNCTION
*     Waits for a client to connect to the socket passed as parameter sock.
*     A timeout (in seconds) can be specified.
*
*  INPUTS
*     sock    - socket, that has been correctly initialized
*     timeout - timeout in seconds, if 0, wait forever
*
*  RESULT
*     the filedescriptor for the accepted client connection
*     -1, if an error occurs or the timeout expires
*
*  SEE ALSO
*     Interactive/qsh/open_qrsh_socket()
*
****************************************************************************
*
*/
static int wait_for_qrsh_socket(int sock, int timeout)
{
   int msgsock = -1;
   fd_set ready;
   struct timeval to;

   DENTER(TOP_LAYER, "wait_for_qrsh_socket");

   /* wait for anyone connecting to socket, with timeout */
   FD_ZERO(&ready);
   FD_SET(sock, &ready);
   to.tv_sec  = timeout;
   to.tv_usec = 0;
   switch(select(sock + 1, &ready, (fd_set *)0, (fd_set *)0, &to)) {
      case -1:
         ERROR((SGE_EVENT, MSG_QSH_ERRORWAITINGONSOCKETFORCLIENTTOCONNECT_S, strerror(errno)));
         break;
      case 0:
         VERBOSE_LOG((stderr, "timeout (%d s) expired while waiting on socket fd %d\n", timeout, sock));
         break;
      default: 
         if(FD_ISSET(sock, &ready)) {
            /* start accepting connections */
            msgsock = accept(sock,(struct sockaddr *) 0,(int *) 0);
            if (msgsock == -1) {
               ERROR((SGE_EVENT, MSG_QSH_ERRORINACCEPTONSOCKET_S, strerror(errno)));
            }
            DPRINTF(("accepted client connection, fd = %d\n", msgsock));
         }
         break;
   } 
   DEXIT;

   return msgsock;
}               

/****** Interactive/qsh/read_from_qrsh_socket() ********************************
*
*  NAME
*     read_from_qrsh_socket -- read a message from socket
*
*  SYNOPSIS
*     static char *read_from_qrsh_socket(int msgsock)
*
*  FUNCTION
*     Tries to read characters from the socket msgsock until a null byte
*     is received.
*
*  INPUTS
*     msgsock - file/socket descriptor to read from
*
*  RESULT
*     the read message,
*     NULL, if an error occured
*
*  NOTES
*     The result points to a static buffer - in subsequent calls of 
*     read_from_qrsh_socket, this buffer will be overwritten.
*
****************************************************************************
*
*/
static char *read_from_qrsh_socket(int msgsock)
{
   int rval = 0;
   static char buffer[1024];
   char *c  = buffer;
  
   DENTER(TOP_LAYER, "read_from_qrsh_socket");
  
   do {
      *c = 0;

      if ((rval = read(msgsock, c, 1)) == -1) {
         ERROR((SGE_EVENT, MSG_QSH_ERRORREADINGSTREAMMESSAGE_S, strerror(errno)));
         DEXIT;
         return NULL;
      } else {
         if (rval == 0) {
            ERROR((SGE_EVENT, MSG_QSH_ERRORENDINGCONNECTION));
            DEXIT;
            return NULL; 
         } 
      }
   } while (*c++ != 0); 

   close(msgsock);
   return buffer;
}

/****** Interactive/qsh/get_remote_exit() ***************************************
*
*  NAME
*     get_remote_exit -- read returncode from socket
*
*  SYNOPSIS
*     static int get_remote_exit(int sock);
*
*  FUNCTION
*     Reads the returncode of the client program from a socket connection 
*     to qlogin_starter.
*     The message read from the socket connection may contain an additional
*     text separated from the return code by a colon.
*     This text is interpreted as an error message and written to stderr.
*
*     Will wait for some time period (QSH_SOCKET_FINAL_TIMEOUT) for 
*     qlogin_starter to connect.
*
*  INPUTS
*     sock - socket file handle
*
*  RESULT
*     the returncode of the client program,
*     -1, if the read failed
*
*  SEE ALSO
*     Interactive/qsh/-Defines
*     Interactive/qsh/wait_for_qrsh_socket()
*     Interactive/qsh/read_from_qrsh_socket()
*
****************************************************************************
*
*/
static int get_remote_exit_code(int sock)                     
{
   int msgsock;

   DENTER(TOP_LAYER, "get_remote_exit_code");
   
   VERBOSE_LOG((stderr, MSG_QSH_READINGEXITCODEFROMSHEPHERD));

   msgsock = wait_for_qrsh_socket(sock, QSH_SOCKET_FINAL_TIMEOUT);
   if(msgsock != -1) {
      char *s_ret = read_from_qrsh_socket(msgsock);
      if(s_ret && *s_ret) {
         char *message = strchr(s_ret, ':');
         if(message != NULL && strlen(message) > 1) {
            fprintf(stderr, message + 1);
            *message = 0;
         }
         VERBOSE_LOG((stderr, "%s\n", s_ret));
         DEXIT;
         return atoi(s_ret);
      }
   }

   ERROR((SGE_EVENT, MSG_QSH_ERRORREADINGRETURNCODEOFREMOTECOMMAND));
   DEXIT;
   return -1;
}

/****** Interactive/qsh/quote_argument() ***************************************
*
*  NAME
*     quote_argument -- quote an argument
*
*  SYNOPSIS
*     static const char *quote_argument(const char *arg)
*
*  FUNCTION
*     Encloses the contents of string arg into  quotes ('').
*     Memory for the new string is allocated using malloc. 
*     It is in the responsibility of the user to free this memory.
*
*  INPUTS
*     arg - the argument string to quote
*
*  RESULT
*     the quoted string
*
*  EXAMPLE
*     const char *quoted = quote_argument("test");
*     printf("%s\n", quoted);
*     free(quoted);
*
*  NOTES
*     Function might be moved to some library, same code is used in other 
*     modules.
*
****************************************************************************
*
*/

static const char *quote_argument(const char *arg) {
   char *new_arg = NULL;

   DENTER(TOP_LAYER, "quote_argument");
   
   if(arg == NULL) {
      return arg;
   }   

   new_arg = malloc(strlen(arg) + 3);

   if(new_arg == NULL) {
      ERROR((SGE_EVENT, MSG_QSH_MALLOCFAILED));
      DEXIT;
      return NULL;
   }
 
   sprintf(new_arg, "'%s'", arg);
   DEXIT;
   return new_arg;
}

/****** Interactive/qsh/parse_result_list() ***************************************
*
*  NAME
*     parse_result_list -- parse result list from sge function calls
*
*  SYNOPSIS
*     static int parse_result_list(lList *alp, int *alp_error);
*
*  FUNCTION
*     Parses through the list and outputs the errors and warnings contained
*     as list elements.
*     Returns info whether an error was contained or not in alp_error
*     and if qsh must exit in returncode.
*
*  INPUTS
*     alp - pointer to result list
*     [alp_error] - reference to return error status
*
*  RESULT
*     0, if no error was contained in list
*     1, if an error was contained in list
*     alp_error - 1, if alp contains an error condition, else 0
*
****************************************************************************
*
*/
static int parse_result_list(lList *alp, int *alp_error)
{
   lListElem *aep;
   int do_exit = 0;

   DENTER(TOP_LAYER, "parse_result_list");

   *alp_error = 0;

   for_each(aep, alp) {
      u_long32 status  = lGetUlong(aep, AN_status);
      u_long32 quality = lGetUlong(aep, AN_quality);
      if (quality == NUM_AN_ERROR) {
         ERROR((SGE_EVENT, "%s", lGetString(aep, AN_text)));
         *alp_error = 1;

         switch(status) {
            case STATUS_EDENIED2HOST:
            case STATUS_ENOSUCHUSER:
            case STATUS_NOSUCHGROUP:
            case STATUS_DENIED:
            case STATUS_EVERSION:
               do_exit = 1;
            default:
               break;
         }
      }
      else if (quality == NUM_AN_WARNING) {
         WARNING((SGE_EVENT, MSG_WARNING_S, lGetString(aep, AN_text)));
      }
   }

   DEXIT;
   return do_exit;
}


/****** Interactive/qsh/start_client_program() ***************************************
*
*  NAME
*     start_client_program -- start a telnet/rsh/rlogin/... client
*
*  SYNOPSIS
*     static void start_client_program(const char *client_name,
*                                      lList *opts_qrsh,
*                                      const char *host,
*                                      const char *port,
*                                      const char *job_dir,
*                                      const char *utilbin_dir,
*                                      int is_rsh,
*                                      int is_rlogin
*                                      int nostdin,
*                                      int noshell,
*                                      int sock)
*
*  FUNCTION
*     Builds the argument vector for the start of the client program,
*     creates a process (fork) and starts the client program in the 
*     child process.
*     After termination of the child, read exit code of remote process
*     from qlogin_starter.
*
*  INPUTS
*     client_name - name of the program to start
*     opts_qrsh   - arguments passed to a qrsh call
*     host        - host on which to start command
*     port        - port on which to contact server process (telnetd etc.)
*     job_dir     - job directory
*     utilbin_dir - path of the utilbin directory on the remote host
*     is_rsh      - do we call a rsh (or similar) program?
*     is_rlogin   - do we call a rlogin (or similar) program?
*     nostdin     - shall stdin be suppressed (parameter -n to rsh)?
*     noshell     - shall command be executed without wrapping login shell
*     sock        - socket descriptor of connection to qlogin_starter
*
*  RESULT
*     returncode of the executed command or
*     EXIT_FAILURE, if exec failed
*
****************************************************************************
*
*/
static int start_client_program(const char *client_name, 
                                lList *opts_qrsh,
                                const char *host,
                                const char *port,
                                const char *job_dir,
                                const char *utilbin_dir,
                                int is_rsh,
                                int is_rlogin,
                                int nostdin,
                                int noshell,
                                int sock)
{
   DENTER(TOP_LAYER, "start_client_program");

   child_pid = fork();

   if(child_pid == -1) {
      ERROR((SGE_EVENT, MSG_QSH_CANNOTFORKPROCESS_S, strerror(errno))); 
      DEXIT;
      return 1;
   }

   if(child_pid) {
      signal(SIGINT,  forward_signal);
      signal(SIGQUIT, forward_signal);
      signal(SIGTERM, forward_signal);
   
      while(1) {
         int status;

      /* Always try to get exit code (or error) from shepherd.
       * If rsh didn't return EXIT_SUCCESS or didn't exit (!WIFEXITED), 
       * output a message "cleaning up ..." and delete the job
       */
         if(waitpid(child_pid, &status, 0) == child_pid) {
            int ret = -1;  /* default: delete job */

            if(WIFEXITED(status)) {
               ret = WEXITSTATUS(status);
               VERBOSE_LOG((stderr, MSG_QSH_EXITEDWITHCODE_SI, client_name, ret));
            }

            if(WIFSIGNALED(status)) {
               int code = WTERMSIG(status);
               VERBOSE_LOG((stderr, MSG_QSH_EXITEDONSIGNAL_SIS, client_name, code, sys_sig2str(code)));
               /* if not qrsh <command>: use default: delete job */
            }

            /* get exit code from shepherd for qrsh <command> */
            if(is_rsh) {
               ret = get_remote_exit_code(sock);
            }

            DEXIT;
            return ret;
         }
      }
   } else {
      char* args[20]; 
      int i = 0;
      char shellpath[SGE_PATH_MAX];
      char *command = strdup(client_name); /* needn't be freed, as we exec */

      args[i++] = strtok(command, " ");
      while((args[i] = strtok(NULL, " ")) != NULL) {
         i++;
      }

      if(is_rsh || is_rlogin) {
         sge_set_def_sig_mask(0, NULL);
         sge_unblock_all_signals();
         
         if(is_rsh && nostdin) {
            args[i++] = "-n";
         }   

         args[i++] = "-p";
         args[i++] = (char *)port;
         args[i++] = (char *)host;
         if(is_rsh) {
            sprintf(shellpath, "%s/qrsh_starter", utilbin_dir);
            args[i++] = "exec";
            args[i++] = (char *)quote_argument(shellpath);
            args[i++] = (char *)quote_argument(job_dir);
            if(noshell) {
               args[i++] = "noshell";
            }   
         }   
      } else {
         args[i++] = (char *)host;
         args[i++] = (char *)port;
      }
   
      args[i] = NULL;
      
#if 0
      {
      int i;
      fflush(stdout); fflush(stderr);
      for(i = 0; args[i] != NULL; i++) {
         printf("qsh: args[%d] = %s\n", i, args[i]);
      }  
      fflush(stdout); fflush(stderr);
      }
#endif
     
      execvp(args[0], args);
      ERROR((SGE_EVENT, MSG_EXEC_CANTEXECXYZ_SS, args[0], strerror(errno)));
      DEXIT;
      exit(EXIT_FAILURE);
   }

   /* should never be reached */
   return -1;
}

/****** Interactive/qsh/get_client_server_context() ***************************************
*
*  NAME
*     get_client_server_context -- get parameters for client/server connection
*
*  SYNOPSIS
*     static int get_client_server_context(int msgsock, 
*                                          char **port, 
*                                          char **job_dir,
*                                          char **utilbin_dir,
*                                          const char **host);
*
*  FUNCTION
*     Tries to read the parameters port and job_dir from the socket connection
*     to qlogin_starter (msgsock).
*     They are sent from qlogin_starter in the format
*     0:<port>:<utilbin_dir>:<job_dir>
*    
*     The communication partner may also report an error, using the format
*     <error>:<message> where error is a number != 0
*     In this case, the error code and message will be written to stderr.
*
*  INPUTS
*     msgsock     - socket descriptor of connection to qlogin_starter
*     port        - reference to port number, see RESULT
*     job_dir     - reference to job_dir, see RESULT
*     utilbin_dir - reference to utilbin_dir, see RESULT
*     host        - reference to host, see RESULT
*
*  RESULT
*     1, if everything ok,
*     0, if the required parameters are not found
*
*     [port]        - used to return the port number to caller
*     [job_dir]     - used to return the job directory to caller
*     [utilbin_dir] - used to return the utilbin directory on the
*                     remote host
*     [host]        - used to return the host, to which to connect
*                     with rsh/rlogin/telnet call
*
*  NOTES
*     port and data point to a static buffer in function
*     read_from_qrsh_socket(). Subsequent calls to this function
*     will overwrite this buffer.
*
*  SEE ALSO
*     Interactive/qsh/read_from_qrsh_socket()
*
****************************************************************************
*
*/
static int get_client_server_context(int msgsock, char **port, char **job_dir, char **utilbin_dir, const char **host)
{
   char *s_code = NULL;
   char *data   = NULL;
   
   DENTER(TOP_LAYER, "get_client_server_context");
   
   data = read_from_qrsh_socket(msgsock);

   if(data == NULL || *data == 0) {
      ERROR((SGE_EVENT, MSG_QSH_ERRORREADINGCONTEXTFROMQLOGIN_STARTER_S,"qlogin_starter"));
      DEXIT;
      return 0;
   }

   DPRINTF(("qlogin_starter sent: %s\n", data));
  
   s_code = strtok(data, ":");

   if(s_code == NULL) {
      ERROR((SGE_EVENT, MSG_QSH_ERRORREADINGCONTEXTFROMQLOGIN_STARTER_S,"qlogin_starter"));
      DEXIT;
      return 0;
   }
   
   if(strcmp(s_code, "0") == 0) {  /* qlogin_starter reports valid data */
      if((*port = strtok(NULL, ":")) != NULL) {
         if((*utilbin_dir = strtok(NULL, ":")) != NULL) {
            if((*job_dir = strtok(NULL, ":")) != NULL) {
               if((*host = strtok(NULL, ":")) != NULL) {
                  return 1;
               }
            }
         }
      }
      ERROR((SGE_EVENT, MSG_QSH_ERRORREADINGCONTEXTFROMQLOGIN_STARTER_S,"qlogin_starter"));
      DEXIT;
      return 0;
   } else {                        /* qlogin_starter reports error */
      char *message = strtok(NULL, "\n");
      ERROR((SGE_EVENT, "%s: %s\n", s_code, message == NULL ? "" : message));
   }

   DEXIT;
   return 0;
}

/****** Interactive/qsh/get_client_name() ***************************************
*
*  NAME
*     get_client_name -- get path and name of client program to start
*
*  SYNOPSIS
*     static char *get_client_name(int is_rsh, int is_rlogin, 
*                                  int inherit_job);
*
*  FUNCTION
*     Determines path and name of the client program to start 
*     (telnet, rsh, rlogin etc.).
*     Takes into consideration the cluster and host configuration.
*     If no value is set for the client program (value none), use
*     telnet in qlogin mode (try to find in path) or
*     $SGE_ROOT/utilbin/$ARCH/[rsh|rlogin] in rsh/rlogin mode.
*
*     In case of qrexec (inherit_job), try to read client name from file
*     ($TMPDIR/qrsh_client_program) to avoid qmaster contact in 
*     qrexec mode. 
*     If file is not found, determine info from qmaster
*     and write it to file.
*
*  INPUTS
*     is_rsh      - are we treating a qrsh call
*     is_rlogin   - are we treating a qrsh call without commands 
*                   (-> rlogin)
*     inherit_job - we have inherited an existing environment
*
*  RESULT
*     path and name of client program
*     NULL, if an error occures
*
****************************************************************************
*
*/
static const char *get_client_name(int is_rsh, int is_rlogin, int inherit_job)
{
   lList     *conf_list       = NULL;
   lListElem *global          = NULL; 
   lListElem *local           = NULL;
   lListElem *qlogin_cmd_elem = NULL;

   static char *session_type = "telnet";
   char        *config_name  = "qlogin_command";
   const char  *client_name  = NULL;
   static char cache_name[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "get_client_name");

   if(is_rsh) { 
      session_type = "rsh"; 
      config_name  = "rsh_command";
   }
              
   if(is_rlogin) { 
      session_type = "rlogin"; 
      config_name  = "rlogin_command";
   }
  
   /* if rsh, try to read from file */
   if(inherit_job) {
      char *tmpdir = getenv("TMPDIR");
      *cache_name = 0;
      if(tmpdir) {
         FILE *cache;
         sprintf(cache_name, "%s/%s", tmpdir, QRSH_CLIENT_CACHE);
         if((cache = fopen(cache_name, "r")) != NULL) {
            if(fgets(cache_name, SGE_PATH_MAX, cache) != NULL) {
               fclose(cache);
               DPRINTF(("found cached client name: %s\n", cache_name));
               DEXIT;
               return cache_name;
            }
         }
      }
   }
  
   /* get configuration from qmaster */
   if(get_configuration(me.qualified_hostname, &global, &local) ||
      merge_configuration(global, local, &conf, &conf_list)) {
      ERROR((SGE_EVENT, MSG_CONFIG_CANTGETCONFIGURATIONFROMQMASTER));
      DEXIT;
      return NULL;
   }

   /* search for config entry */
   qlogin_cmd_elem = lGetElemStr(conf_list, CF_name, config_name);
   if(!qlogin_cmd_elem) {
      ERROR((SGE_EVENT, MSG_CONFIG_CANTGETCONFIGURATIONFROMQMASTER));
      DEXIT;
      return NULL;
   }

   client_name = lGetString(qlogin_cmd_elem, CF_value);

   /* default handling */
   if(client_name == NULL || !strcmp(client_name, "none")) {
      if(is_rsh || is_rlogin) {
         /* use path $ROOT/utilbin/$ARCH/sessionType */
         static char cmdpath[1024];

         sprintf(cmdpath, "%s/utilbin/%s/%s", path.sge_root, sge_get_arch(), session_type);
         client_name = cmdpath;
      } else {
         /* try to find telnet in PATH */
         client_name = session_type;
      }
   } 

   if(inherit_job) {
      /* cache client_name for use of further qrexec calls */
      write_client_name_cache(cache_name, client_name);
   }

   return client_name;
}

/****** Interactive/qsh/write_client_name_cache() ******************************
*  NAME
*     write_client_name_cache() -- cache name of qrsh client command
*
*  SYNOPSIS
*     void write_client_name_cache(const char *cache_path, 
*                                  const char *client_name) 
*
*  FUNCTION
*     Name and path of the remote client command started by qrsh is configured
*     in the cluster configuration, parameter rsh_client/rlogin_client.
*     This configuration is retrieved from qmaster before qrsh can start the
*     client command.
*     In case of multiple calls to qrsh -inherit, e.g. from a qmake application, 
*     this would meen a lot of communication to qmaster to retrieve the same
*     parameter over and over again.
*     To avoid this unneccessary communication overhead, the name of the client
*     command is cached in a file in the jobs temporary directory (TMPDIR) when
*     it is retrieved for the first time.
*     Following calls to qrsh -inherit can then just read the cache file.
*
*  INPUTS
*     const char *cache_path  - name and path of the cache file 
*     const char *client_name - name of the client command 
*
*  RESULT
*     void - no error handling
*
*  SEE ALSO
*     Interactive/qsh/get_client_name()
*
*******************************************************************************/
void write_client_name_cache(const char *cache_path, const char *client_name) 
{
   if(cache_path && *cache_path && client_name && *client_name) {
      FILE *cache = NULL;
   
      if((cache = fopen(cache_path, "w")) != NULL) {
         fprintf(cache, client_name);
         fclose(cache);
      }

#if 0
      fprintf(stderr, "Wrote cached client name %s to file %s\n", client_name, cache_path);
#endif      
   }
}

/****** Interactive/qsh/set_job_info() ***************************************
*
*  NAME
*     set_job_info -- info about interactive job for execd/shepherd
*
*  SYNOPSIS
*     static void set_job_info(lListElem *job, const char *name,
*                              int is_qlogin, int is_rsh, 
*                              int is_rlogin);
*
*  FUNCTION
*     Sets the flag JB_now and the jobname JB_job_name to reasonable
*     values depending on the startmode of the qsh binary:
*
*  INPUTS
*     job       - job data structure
*     name      - name of the program to start - if called with path, 
*                 the path is stripped
*     is_qlogin - are we treating a qlogin call
*     is_rsh    - are we treating a qrsh call
*     is_rlogin - are we treating a qrsh call without commands (-> rlogin)
*
****************************************************************************
*
*/
static void set_job_info(lListElem *job, const char *name, int is_qlogin, int is_rsh, int is_rlogin)
{
   u_long32 jb_now = lGetUlong(job, JB_now);
   const char *job_name  = lGetString(job, JB_job_name);
   
   if(is_rlogin) {
      JB_NOW_SET_QRLOGIN(jb_now);
      if(!job_name) {
         job_name =JB_NOW_STR_QRLOGIN;
      } 
   } else {   
      if(is_rsh) {
         JB_NOW_SET_QRSH(jb_now);
         if(!job_name) {
           job_name = name;
         } 
      } else {
         if(is_qlogin && !is_rsh) {
            JB_NOW_SET_QLOGIN(jb_now);
            if(!job_name) {
               job_name = JB_NOW_STR_QLOGIN;
            }   
         } else {
            JB_NOW_SET_QSH(jb_now);
            if(!job_name) {
               job_name = JB_NOW_STR_QSH;
            }   
         } 
      }   
   }

   lSetUlong(job, JB_now, jb_now);
   lSetString(job, JB_job_name, job_name);
}

/****** Interactive/qsh/parse_qrsh_command() ***************************************
*
*  NAME
*     parse_qrsh_command -- separate qsh and command options
*
*  SYNOPSIS
*     lList *parse_qrsh_command(lList *opts_cmdline, char *name,
*                               char **hostname, int existing_job);
*
*  FUNCTION
*     Parses the commandline options to qsh, if they contain a call of 
*     a script or program,
*      - set the name of the program to call (path is stripped)
*      - move options to the scriptfile to a new list
*     If the flag <existing_job> is set, the first argument in
*     the command line is interpreted as hostname.
*
*  INPUTS
*     opts_cmdline - list of command line options
*     name         - buffer to hold program name
*     hostname     - reference to hostname pointer
*     existing_job - flag, do we handle an existing job environment 
*                    (-inherit)?
*
*  RESULT
*     A list of options to the program to call by rsh,
*     NULL, if no script or program is called or an error occured
*     The reference parameter hostname is set to NULL, or a valid 
*     hostname, in case of existing_job set.
*
****************************************************************************
*
*/
static lList *parse_qrsh_command(lList *opts_cmdline, char *name, const char **hostname, int existing_job)
{
   lList *opts_qrsh = NULL;
   lListElem *ep    = NULL;
      
   DENTER(TOP_LAYER, "parse_qrsh_command");

   strcpy(name, "QRSH");
   *hostname = NULL;

   opts_qrsh = lCreateList("opts_qrsh", lGetListDescr(opts_cmdline));

   if((ep = lGetElemStr(opts_cmdline, SPA_switch, "script"))) {
      if(existing_job) {
         *hostname = lGetString(ep, SPA_argval_lStringT);
         lDechainElem(opts_cmdline, ep);
         ep = lGetElemStr(opts_cmdline, SPA_switch, "jobarg");
      }  

      if(ep) {
         const char *new_name = NULL;

         lDechainElem(opts_cmdline, ep);
         lAppendElem(opts_qrsh, ep);
        
         new_name = sge_basename(lGetString(ep, SPA_argval_lStringT), '/');
         if(new_name != NULL) {
            strncpy(name, new_name, MAX_JOB_NAME); 
         }

         while((ep = lGetElemStr(opts_cmdline, SPA_switch, "jobarg"))) {
            lDechainElem(opts_cmdline, ep);
            lAppendElem(opts_qrsh, ep);
         }   
      }
   }

   if(name) {
      name = strtok(name, " ;:");
   }

   DEXIT;
   return opts_qrsh;
}

/****** Interactive/qsh/merge_and_order_options() ***************************************
*
*  NAME
*     merge_and_order_options -- merge options from different sources
*
*  SYNOPSIS
*     static lList *merge_and_order_options(lList **opts_defaults,
*                                           lList **opts_scriptfile,
*                                           lList **opts_cmdline);
*
*  FUNCTION
*     Options to a sge submit can come from different sources:
*      - default settings (sge/sge_request)
*      - special comments in scriptfiles (override default settings)
*      - command line options (override default settings and special comments)
*     The function merge_and_order_options merges options from all three 
*     sources into one list in the correct order for the override sequence
*     and sets the input lists to NULL.
*
*  INPUTS
*     opts_defaults   - default settings
*     opts_scriptfile - special comments
*     opts_cmdline    - command line options
*
*  RESULT
*     the resulting list of all options
*     NULL, if no options are set in any of the input lists
*
****************************************************************************
*
*/
static lList *merge_and_order_options(lList **opts_defaults, 
                                      lList **opts_scriptfile, 
                                      lList **opts_cmdline)
{
   lList *opts_all = NULL;

   /*
   ** order is very important here
   */
   if (*opts_defaults) {
      opts_all = *opts_defaults;
      *opts_defaults = NULL;
   }
   if (*opts_scriptfile) {
      if (!opts_all) {
         opts_all = *opts_scriptfile;
      }
      else {
         lAddList(opts_all, *opts_scriptfile);
      }
      *opts_scriptfile = NULL;
   }
   if (*opts_cmdline) {
      if (!opts_all) {
         opts_all = *opts_cmdline;
      }
      else {
         lAddList(opts_all, *opts_cmdline);
      }
      *opts_cmdline = NULL;
   }

   return opts_all;
}   


/****** Interactive/qsh/set_command_to_env() ***************************************
*
*  NAME
*     set_command_to_env() -- set command to execute in environment variable
*
*  SYNOPSIS
*     static void set_command_to_env(lList *envlp, lList *opts_qrsh);
*
*  FUNCTION
*     Creates an environment variable QRSH_COMMAND in the environment list
*     <envlp>, that contains the commandline passed in the argument list
*     <opts_qrsh>.
*     Arguments are separated by the character code 0xff.
*
*  INPUTS
*     envlp     - environment list
*     opts_qrsh - list of commandline arguments
*
*  BUGS
*     In general, passing the commandline via environment variable is only
*     a workaround for the problems occuring when passing the commandline
*     via rsh command (quoting, backquotes etc. through several shells).
*
*     Better would be to pass the commandline over a save (not modifying) 
*     mechanism, e.g. passing each argument as a line over file (not possible)
*     or a socket connection.
*
****************************************************************************
*
*/
void set_command_to_env(lList *envlp, lList *opts_qrsh)
{
   dstring buffer = DSTRING_INIT;

   if(opts_qrsh) {
      lListElem *ep;
     
      ep = lFirst(opts_qrsh);
      if(ep) {
         char delimiter[2];
         const char *help = NULL;
         sprintf(delimiter, "%c", 0xff);
         help = lGetString(ep, SPA_argval_lStringT);
         if (help != NULL) {
            sge_dstring_copy_string(&buffer, (char *)help); 
         } 
         while((ep = lNext(ep)) != NULL) {
            const char *arg = lGetString(ep, SPA_argval_lStringT);
            sge_dstring_append(&buffer, delimiter);
            if (arg != NULL) {
               sge_dstring_append(&buffer, arg);
            } 
         }   
      }
   } 

#if 0
   fflush(stdout); fflush(stderr);
   fprintf(stderr, "qsh: QRSH_COMMAND = %s\n", buffer);
   fflush(stdout); fflush(stderr);
#endif

   add2env(&envlp, "QRSH_COMMAND", sge_dstring_get_string(&buffer));
   sge_dstring_free(&buffer);
}

int main(
int argc,
char **argv 
) { 
   u_long32 my_who = QSH; 
   lList *opts_cmdline = NULL;
   lList *opts_defaults = NULL;
   lList *opts_scriptfile = NULL;
   lList *opts_all = NULL;
   lList *opts_qrsh = NULL;
   lListElem *job = NULL;
   lList *lp_jobs = NULL;
   lList *alp = NULL;

   lListElem *aep = NULL;
   u_long32 status = STATUS_OK;
   u_long32 quality;
   u_long32 job_id = 0;
   int do_exit = 0;
   int exit_status = 0;
   int is_qlogin = 0;
   int is_rsh    = 0;
   int is_rlogin = 0;
   int just_verify = 0;
   int inherit_job  = 0;
   int existing_job = 0;
   int nostdin = 0;
   int noshell = 0;

   const char *host = NULL;
   char name[MAX_JOB_NAME + 1];
   const char *client_name = NULL;
   char *port = NULL;
   char *job_dir = NULL;
   char *utilbin_dir = NULL;

   int alp_error;

   lListElem *ep = NULL;

   int sock;
   int cl_err = 0;

   DENTER_MAIN(TOP_LAYER, "qsh");

   /*
   ** get command name: qlogin, qrsh or qsh
   */
   if(!strcmp(sge_basename(argv[0], '/'), "qlogin")) {
      is_qlogin = 1;
      my_who = QLOGIN;
   } else {
      if(!strcmp(sge_basename(argv[0], '/'), "qrsh")) {
         is_qlogin = 1;
         is_rsh    = 1;
         my_who = QRSH;
      }   
   }

   sge_gdi_param(SET_MEWHO, my_who, NULL);
/*    sge_gdi_param(SET_ISALIVE, 1, NULL); */
   if ((cl_err = sge_gdi_setup(prognames[my_who]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(my_who);

   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);

   /*
   ** begin to work
   */

   

   /* 
   ** set verbosity default: qlogin and qsh: verbose to be backwards compatible,
   ** qrsh: quiet
   */
   if(is_rsh) {
      sge_log_verbose(0); 
   } else {
      sge_log_verbose(1);
   }

   /*
   ** read switches from the various defaults files
   */
   alp = get_all_defaults_files(&opts_defaults, environ);
   do_exit = parse_result_list(alp, &alp_error);
   lFreeList(alp);

   if (alp_error) {
      SGE_EXIT(1);
   }

   /*
   ** append the commandline switches to the list
   */
   alp = cull_parse_cmdline(argv + 1, environ, &opts_cmdline, FLG_USE_PSEUDOS);
   do_exit = parse_result_list(alp, &alp_error);
   lFreeList(alp);

   if (alp_error) {
      SGE_EXIT(1);
   }

   if (lGetElemStr(opts_cmdline, SPA_switch, "-help")) {
      /* me.who = my_who; */
      sge_usage(stdout);
      SGE_EXIT(0);
   }

   /* set verbosity */
   while ((ep = lGetElemStr(opts_cmdline, SPA_switch, "-verbose"))) {
      lRemoveElem(opts_cmdline, ep);
      sge_log_verbose(1);
   }

   /* parse -noshell */
   while ((ep = lGetElemStr(opts_cmdline, SPA_switch, "-noshell"))) {
      lRemoveElem(opts_cmdline, ep);
      noshell = 1;
   }

   /*
   ** if qrsh, parse command to call
   */
   if(is_rsh) {
      while ((ep = lGetElemStr(opts_cmdline, SPA_switch, "-nostdin"))) {
         lRemoveElem(opts_cmdline, ep);
         nostdin = 1;
      }
     
      while ((ep = lGetElemStr(opts_cmdline, SPA_switch, "-inherit"))) {
         lRemoveElem(opts_cmdline, ep);
         inherit_job = 1;
      }
     
      if(inherit_job) {
         char *s_existing_job = getenv("JOB_ID");
         if(s_existing_job != NULL) {
            if(sscanf(s_existing_job, "%d", &existing_job) != 1) {
               ERROR((SGE_EVENT, MSG_QSH_INVALIDJOB_ID_SS, "JOB_ID",s_existing_job));
               SGE_EXIT(1);
            }
         } else {
            ERROR((SGE_EVENT, MSG_QSH_INHERITBUTJOB_IDNOTSET_SSS,"qrsh","-inherit","JOB_ID"));
            SGE_EXIT(1);
         }
      }   

      opts_qrsh = parse_qrsh_command(opts_cmdline, name, &host, existing_job);


      if(lGetNumberOfElem(opts_qrsh) <= 0) {
         if(inherit_job) {
            ERROR((SGE_EVENT, MSG_QSH_INHERITUSAGE_SS, "-inherit", "qrsh -inherit ... <host> <command> [args]"));
            SGE_EXIT(1);
         } else {
            is_rsh = 0;
            is_rlogin = 1;
         }
      }   
   }

   if (!job) {
      lList *answer = NULL;

      job = lCreateElem(JB_Type);
      if (!job) {
         sge_add_answer(&answer, MSG_MEM_MEMORYALLOCFAILED, STATUS_EMALLOC, 0);
         do_exit = parse_result_list(alp, &alp_error);
         lFreeList(answer);
         if (alp_error) {
            SGE_EXIT(1);
         }
      }
   }
   if(!existing_job) {
      set_job_info(job, name, is_qlogin, is_rsh, is_rlogin); 
   }

   remove_unknown_opts(opts_cmdline, lGetUlong(job, JB_now), existing_job, TRUE);
   remove_unknown_opts(opts_defaults, lGetUlong(job, JB_now), existing_job, FALSE);
   remove_unknown_opts(opts_scriptfile, lGetUlong(job, JB_now), existing_job, FALSE);

   opts_all = merge_and_order_options(&opts_defaults, &opts_scriptfile, &opts_cmdline);

   alp = cull_parse_qsh_parameter(opts_all, &job);
   do_exit = parse_result_list(alp, &alp_error);
   lFreeList(alp);
   lFreeList(opts_all);
   if (alp_error) {
      SGE_EXIT(1);
   }

   if(is_qlogin) {
      /* get configuration from qmaster */
      if((client_name = get_client_name(is_rsh, is_rlogin, inherit_job)) == NULL) {
         SGE_EXIT(1);
      } 
   }   
   
   if(!existing_job) {
      set_job_info(job, name, is_qlogin, is_rsh, is_rlogin); 
      DPRINTF(("Everything ok\n"));
#ifndef NO_SGE_COMPILE_DEBUG
      if (rmon_mlgetl(&DEBUG_ON, TOP_LAYER) & INFOPRINT) { 
         lWriteElemTo(job, stdout);
      }
#endif

      if (lGetUlong(job, JB_verify)) {
         cull_show_job(job, 0);
         SGE_EXIT(0);
      }

      /*
      ** security hook
      */
      if (set_sec_cred(job) != 0) {
         fprintf(stderr, MSG_SEC_SETJOBCRED);
         SGE_EXIT(1);
      }   

      just_verify = (lGetUlong(job, JB_verify_suitable_queues)==JUST_VERIFY);
   }

   /*
   ** open socket for qlogin communication and set host and port in environment QRSH_PORT 
   */
   {
      char buffer[1024];
      int my_port = 0;
      lList *envlp = NULL;

      sock = open_qrsh_socket(&my_port);
      sprintf(buffer, "%s:%d", me.qualified_hostname, my_port);

      if((envlp = lGetList(job, JB_env_list)) == NULL) {
         envlp = lCreateList("environment list", VA_Type);
         lSetList(job, JB_env_list, envlp);
      }   

      add2env(&envlp, "QRSH_PORT", buffer);
      set_command_to_env(envlp, opts_qrsh);
   }

   /* 
   ** if environment QRSH_WRAPPER is set, pass it trough environment
   */
   {
      char *wrapper;
      lList *envlp = lGetList(job, JB_env_list);

      if((wrapper = getenv("QRSH_WRAPPER")) != NULL) {
         add2env(&envlp, "QRSH_WRAPPER", wrapper);
      }
   }

   /* lWriteElemTo(job, stderr); */

   /* 
   ** add the job
   */
   if(existing_job) {
      int msgsock   = -1;
      sge_tid_t tid = -1;
      const char *cwd = NULL;
     
      VERBOSE_LOG((stderr, MSG_QSH_SENDINGTASKTO_S, host)); 

      /* if we had a connection to qmaster commd (to get configuration), 
       * close it and reset commproc id */
      leave_commd();
      set_commlib_param(CL_P_ID, 0, NULL, NULL);
   
      cwd = lGetString(job, JB_cwd);

      tid = sge_qexecve(host, NULL, cwd, NULL, lGetList(job, JB_env_list), 1); 

      if(tid <= 0) {
         ERROR((SGE_EVENT, MSG_QSH_EXECUTINGTASKOFJOBFAILED_IS, existing_job,
            qexec_last_err() ? qexec_last_err() : "unknown"));
         SGE_EXIT(EXIT_FAILURE);
      }

      VERBOSE_LOG((stderr, MSG_QSH_SERVERDAEMONSUCCESSFULLYSTARTEDWITHTASKID_U, u32c(tid))); 

      if((msgsock = wait_for_qrsh_socket(sock, QSH_SOCKET_FINAL_TIMEOUT)) == -1) {
         ERROR((SGE_EVENT,MSG_QSH_CANNOTGETCONNECTIONTOQLOGIN_STARTER_SS,"shepherd", host));
         SGE_EXIT(EXIT_FAILURE);
      }

      if(!get_client_server_context(msgsock, &port, &job_dir, &utilbin_dir, &host)) {
         SGE_EXIT(EXIT_FAILURE);
      }

      VERBOSE_LOG((stderr, MSG_QSH_ESTABLISHINGREMOTESESSIONTO_SS, client_name, host));

      exit_status = start_client_program(client_name, opts_qrsh, host, port, job_dir, utilbin_dir,
                                         is_rsh, is_rlogin, nostdin, noshell, sock);
   } else {
      int polling_interval;
      
      srand(getpid());

      /* initialize first polling interval, batch jobs have longer polling interval */
      if(JB_NOW_IS_IMMEDIATE(lGetUlong(job, JB_now))) {
         polling_interval = QSH_INTERACTIVE_POLLING_MIN;
      } else {
         polling_interval = QSH_BATCH_POLLING_MIN;
      }   

      add_parent_uplink(job);

      lp_jobs = lCreateList("submitted jobs", JB_Type);
      lAppendElem(lp_jobs, job);
   
      DPRINTF(("B E F O R E     S E N D I N G! ! ! ! ! ! ! ! ! ! ! ! ! !\n"));
      DPRINTF(("=====================================================\n"));

      alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_ADD + SGE_GDI_RETURN_NEW_VERSION , &lp_jobs, NULL, NULL);

      /* reinitialize 'job' with pointer to new version from qmaster */
      job = lFirst(lp_jobs);
 
      for_each(aep, alp) {
         status = lGetUlong(aep, AN_status);
         quality = lGetUlong(aep, AN_quality);
         if (quality == NUM_AN_ERROR) {
            ERROR((SGE_EVENT, "%s", lGetString(aep, AN_text)));
            do_exit = 1;
         }
         else if (quality == NUM_AN_WARNING) {
            WARNING((SGE_EVENT, "%s", lGetString(aep, AN_text)));
         } else {
            INFO((SGE_EVENT, "%s", lGetString(aep, AN_text)));
            if (job) {
               job_id = lGetUlong(job, JB_job_number );
            } else {
               job_id = 0;
            }
            DPRINTF(("job id is: %ld\n", job_id));
         }
      }

      if (just_verify)
         do_exit = 1;
   
      if (do_exit) {
         lFreeList(alp);
         lFreeList(lp_jobs);
         if (status == STATUS_NOTOK_DOAGAIN) {
            SGE_EXIT(status);
         } else {
            SGE_EXIT(1);
         }      
      }
      
      VERBOSE_LOG((stderr, MSG_QSH_WAITINGFORINTERACTIVEJOBTOBESCHEDULED));

      DPRINTF(("R E A D I N G    J O B ! ! ! ! ! ! ! ! ! ! !\n"));
      DPRINTF(("============================================\n"));
      
      while(!do_exit) {
         lCondition *where;
         lEnumeration *what;
         u_long32 job_status;
         int do_shut = 0;
         lList *lp_poll = NULL;
         lListElem *ja_task = NULL;
         lListElem* jep;
         int msgsock = -1;
         int random_poll = polling_interval + (rand() % polling_interval);

         DPRINTF(("random polling set to %d\n", random_poll));

         /* leave commd while waiting for connection / sleeping while polling */
         leave_commd();
         /* next enroll will _not_ ask commd to get same client id as before  */
         set_commlib_param(CL_P_ID, 0, NULL, NULL);
   
         if(is_qlogin) {
            /* if qlogin_starter is used (qlogin, rsh, rlogin): wait for context */
            msgsock = wait_for_qrsh_socket(sock, random_poll); 

            /* qlogin_starter reports "ready to start" */
            if(msgsock > 0) {
               if(!get_client_server_context(msgsock, &port, &job_dir, &utilbin_dir, &host)) {
                  delete_job(job_id, lp_jobs);
                  do_exit = 1;
                  exit_status = 1;
                  break;
               }
   
               VERBOSE_LOG((stderr, MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_D, u32c(job_id)));
               VERBOSE_LOG((stderr, MSG_QSH_ESTABLISHINGREMOTESESSIONTO_SS, client_name, host));

               exit_status = start_client_program(client_name, opts_qrsh, host, port, job_dir, utilbin_dir,
                                                  is_rsh, is_rlogin, nostdin, noshell, sock);
               if(exit_status < 0) {
                  WARNING((SGE_EVENT, MSG_QSH_CLEANINGUPAFTERABNORMALEXITOF_S, client_name));
                  delete_job(job_id, lp_jobs);
                  exit_status = EXIT_FAILURE;
               }

               do_exit = 1;
               continue;
            }   
         } else {
            /* wait for qsh job to be scheduled */
            sleep(random_poll);
         }   
   
         /* get job from qmaster: to handle qsh and to detect deleted qrsh job */
         what = lWhat("%T(%I)", JB_Type, JB_ja_tasks); 
         where = lWhere("%T(%I==%u)", JB_Type, JB_job_number, job_id); 
   
         alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_GET, &lp_poll, where, what);
   
         do_exit = parse_result_list(alp, &alp_error);
   
         lFreeWhere(where);
         lFreeWhat(what);
         alp = lFreeList(alp);
   
         do_shut = shut_me_down;
         if (do_shut || do_exit) {
            WARNING((SGE_EVENT, MSG_QSH_REQUESTFORINTERACTIVEJOBHASBEENCANCELED));
            delete_job(job_id, lp_jobs);
            do_exit = 1;
            exit_status = 1;
            continue;
         }
  
         if(alp_error) {
            continue;
         }
  
         if (!lp_poll || !(jep = lFirst(lp_poll))) {
            WARNING((SGE_EVENT, "\n"));
            sge_log_verbose(1);
            WARNING((SGE_EVENT, MSG_QSH_REQUESTCANTBESCHEDULEDTRYLATER_S, me.sge_formal_prog_name));
            do_exit = 1;
            exit_status = 1;
            continue;
         }
         
         ja_task = lFirst(lGetList(jep, JB_ja_tasks)); 
         if (ja_task) {
            job_status = lGetUlong(ja_task, JAT_status);
            DPRINTF(("Job Status is: %lx\n", job_status));
         } else {
            job_status = JIDLE;
            DPRINTF(("Job Status is: %lx (unenrolled)\n", job_status));
         }
   
         switch(job_status) {
            /* qsh or future -wait case */
            case JIDLE:
               VERBOSE_LOG((stderr, "."));
               break;
   
            case JRUNNING:
            case JTRANSITING:
               VERBOSE_LOG((stderr, MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_D, u32c(job_id)));
   
               /* in case of qlogin: has been scheduled / is transitting just after */
               /* timeout -> loop */
               if(is_qlogin) {
                  continue;
               } else {
               /* qsh: xterm has been started -> exit */   
                  do_exit = 1;
               }
               break;
   
            case JFINISHED:
               WARNING((SGE_EVENT, MSG_QSH_CANTSTARTINTERACTIVEJOB));
               do_exit = 1;
               exit_status = 1;
               break;
            default:
               ERROR((SGE_EVENT, MSG_QSH_UNKNOWNJOBSTATUS_X, x32c(job_status)));
               do_exit = 1;
               exit_status = 1;
               break;
         }
   
         lp_poll = lFreeList(lp_poll);

         if(!do_exit && polling_interval < QSH_POLLING_MAX) {
            polling_interval *= 2;
            DPRINTF(("polling_interval set to %d\n", polling_interval));
         }
      } /* end of while (1) polling */
   
      lp_jobs = lFreeList(lp_jobs);
   
   }

   SGE_EXIT(exit_status);
   return exit_status;
}

static void delete_job(u_long32 job_id, lList *jlp) 
{
   lListElem *jep;
   lList* idlp = NULL;
   char job_str[128];

   if (!jlp) {
      return;
   }
   jep = lFirst(jlp);
   if (!jep) {
      return;
   }

   sprintf(job_str, u32, job_id);
   lAddElemStr(&idlp, ID_str, job_str, ID_Type);

   sge_gdi(SGE_JOB_LIST, SGE_GDI_DEL, &idlp, NULL, NULL);
   /*
   ** no error handling here, we try to delete the job
   ** if we can
   */
}

static void remove_unknown_opts(lList *lp, u_long32 jb_now, int tightly_integrated, int error)
{
   lListElem *ep, *next;

   DENTER(TOP_LAYER, "remove_unknown_opts");

   /* no options given - nothing to remove */
   if (lp == NULL) {
      DEXIT;
      return;
   }

   /* loop over all options and remove the ones that are not allowed */
   next = lFirst(lp);
   while ((ep = next) != NULL) {
      const char *cp;
      
      next = lNext(ep);
      cp = lGetString(ep, SPA_switch);

      if(cp != NULL) {
         /* these are the options allowed for all flavors of interactive jobs
          * all other will be deleted
          */
         if(strcmp(cp, "jobarg") && strcmp(cp, "script") &&
            strcmp(cp, "-A") && strcmp(cp, "-cell") && strcmp(cp, "-clear") && 
            strcmp(cp, "-cwd") && strcmp(cp, "-hard") && strcmp(cp, "-help") &&
            strcmp(cp, "-hold_jid") && strcmp(cp, "-h") &&
            strcmp(cp, "-l") && strcmp(cp, "-m") && strcmp(cp, "-masterq") &&
            strcmp(cp, "-N") && strcmp(cp, "-noshell") && strcmp(cp, "-now") &&
            strcmp(cp, "-P") &&
            strcmp(cp, "-p") && strcmp(cp, "-pe") && strcmp(cp, "-q") && strcmp(cp, "-v") &&
            strcmp(cp, "-V") && strcmp(cp, "-display") && strcmp(cp, "-verify") &&
            strcmp(cp, "-soft") && strcmp(cp, "-M") && strcmp(cp, "-verbose") &&
            strcmp(cp, "-ac") && strcmp(cp, "-dc") && strcmp(cp, "-sc") &&
            strcmp(cp, "-S") && strcmp(cp, "-w")
           ) {
            if(error) {
               ERROR((SGE_EVENT, MSG_ANSWER_UNKOWNOPTIONX_S, cp));
               SGE_EXIT(EXIT_FAILURE);
            } else {
               lRemoveElem(lp, ep);
               continue;
            }
         }

         /* the login type interactive jobs do not allow some options - delete these ones */
         if(JB_NOW_IS_QLOGIN(jb_now) || JB_NOW_IS_QRLOGIN(jb_now)) {
            if(strcmp(cp, "-display") == 0 ||
               strcmp(cp, "-cwd") == 0 ||
               strcmp(cp, "-v") == 0 ||
               strcmp(cp, "-V") == 0
              ) {
               if(error) {
                  ERROR((SGE_EVENT, MSG_ANSWER_UNKOWNOPTIONX_S, cp));
                  SGE_EXIT(EXIT_FAILURE);
               } else {
                  lRemoveElem(lp, ep);
                  continue;
               }
            }
         }

         /* the -S switch is only allowed for qsh */
         if(!JB_NOW_IS_QSH(jb_now)) {
            if(strcmp(cp, "-S") == 0) {
               if(error) {
                  ERROR((SGE_EVENT, MSG_ANSWER_UNKOWNOPTIONX_S, cp));
                  SGE_EXIT(EXIT_FAILURE);
               } else {
                  lRemoveElem(lp, ep);
                  continue;
               }
            }
         }

         /* qrsh -inherit only allowes -cwd and setting of environment */
         if(tightly_integrated) {
            if(strcmp(cp, "-cwd") &&
               strcmp(cp, "-display") &&
               strcmp(cp, "-v") &&
               strcmp(cp, "-V")
              ) {
               if(error) {
                  ERROR((SGE_EVENT, MSG_ANSWER_UNKOWNOPTIONX_S, cp));
                  SGE_EXIT(EXIT_FAILURE);
               } else {
                  lRemoveElem(lp, ep);
                  continue;
               }
            }
         }
         
         /* set defaults for mail delivery */
         if (strcmp(cp, "-m") == 0) {
            int m;
            
            m = lGetInt(ep, SPA_argval_lIntT);
            m &= ~MAIL_AT_BEGINNING;
            m &= ~MAIL_AT_EXIT;
            lSetInt(ep, SPA_argval_lIntT, m);
         }
      }
   }

   DEXIT;
} 
