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
#include <stdarg.h>

#if defined(LINUX)
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "basis_types.h"
#include "def.h"
#include "sge_getpwnam.h"
#include "sge_pgrp.h"
#include "sge_string.h"
#include "config_file.h"
#include "sge_stat.h"

#include "msg_daemons_common.h"
#include "msg_qrsh.h"

#define MAX_ENVIRONMENT_LENGTH 4096

#define MAKEEXITSTATUS(x) (x << 8)

static pid_t child_pid = 0;

static void 
qrsh_error(const char *fmt, ...)
{
   char *tmpdir = NULL;
   char *taskid = NULL;
   FILE *file   = NULL;
   char fileName[SGE_PATH_MAX];

   va_list ap;
   char message[MAX_STRING_SIZE];

   va_start(ap, fmt);

   if (fmt == NULL || *fmt == '\0') {
      return;
   }

   vsprintf(message, fmt, ap);

   if ((tmpdir = search_conf_val("qrsh_tmpdir")) == NULL) {
      fprintf(stderr, message);
      fprintf(stderr, MSG_CONF_NOCONFVALUE_S, "qrsh_tmpdir");
      return;
   }

   taskid = search_conf_val("qrsh_task_id");

   if (taskid != NULL) {
      sprintf(fileName, "%s/qrsh_error.%s", tmpdir, taskid);
   } else {
      sprintf(fileName, "%s/qrsh_error", tmpdir);
   }

   if ((file = fopen(fileName, "a")) == NULL) {
      fprintf(stderr, message);
      fprintf(stderr, MSG_QRSH_STARTER_CANNOTOPENFILE_SS, fileName, 
              strerror(errno));
      return;
   }
  
   chmod(fileName, 00744);
   fwrite(message, strlen(message), 1, file);

   fclose(file);
}

/****** Interactive/qrsh_starter/sge_putenv() **********************************
*  NAME
*     sge_putenv() -- put an environment variable to environment
*
*  SYNOPSIS
*     static int sge_putenv(const char *var) 
*
*  FUNCTION
*     Duplicates the given environment variable and calls the system call
*     putenv.
*
*  INPUTS
*     const char *var - variable to put in the form <name>=<value>
*
*  RESULT
*     static int - 1 on success, else 0
*
*  NOTES
*     This function should probably be part of utilib.
*     Or sge_setenv's restrictions could be removed.
*
*  SEE ALSO
*     sge_setenv()
*
*******************************************************************************/
static int sge_putenv(const char *var)
{
   char *duplicate;

   if(var == NULL) {
      return 0;
   }

   duplicate = strdup(var);

   if(duplicate == NULL) {
      qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
      return 0;
   }

   if(putenv(duplicate) != 0) {
      qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
      return 0;
   }

   return 1;
}

/****** Interactive/qrsh_starter/setEnvironment() ******************************
*
*  NAME
*     setEnvironment() -- set environment from file
*
*  SYNOPSIS
*     static char *setEnvironment(const char *jobdir, char **wrapper);
*
*  FUNCTION
*     Reads environment variables and their values from file <envFileName>
*     and sets them in the actual process environment.
*     The file format conforms to the sge environment file format:
*     Each line contains a tuple:
*        <name>=<value>
*     Special handling for variable PWD: tries to change to named
*     directory.
*     Special handling for variable QRSH_COMMAND: is the command to be executed
*     by qrsh_starter. The value of this variable will be returned as command,
*     or NULL, if an error occurs.
*     Special handling for variable QRSH_WRAPPER: this is a wrapper to be called
*     instead of a shell to execute the command.
*     If this variable is contained in the environment, it will be returned in
*     the parameter wrapper. Memory will be allocated to hold the variable, it is
*     in the responsibility of the caller to free this memory.
*
*  INPUTS
*     jobdir - the jobs spool directory
*     wrapper - buffer to take the path and name of a wrapper script
*
*  RESULT
*     command, if all actions could be performed
*     NULL,    if an error occured; possible errors are:
*                 - the environment file cannot be opened
*                 - a PWD entry is found, but changing to the named directory fails
*                 - necessary memory cannot be allocated
*                 - the variable QRSH_COMMAND is not found
*
****************************************************************************
*/
static char *setEnvironment(const char *jobdir, char **wrapper)
{
   char *envFileName = NULL;
   FILE *envFile = NULL;
   char *line = NULL;
   char *command   = NULL;
   SGE_STRUCT_STAT statbuf;
   int size; 

   *wrapper = NULL;

   /* build path of environmentfile */
   envFileName = (char *)malloc(strlen(jobdir) + strlen("environment") + 2);

   if (envFileName == NULL) {
      qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
      return NULL;
   }

   sprintf(envFileName, "%s/environment", jobdir);
 
   /* check if environment file exists and
    * retrieve file size. We will take file size as maximum possible line length
    */
   if (SGE_STAT(envFileName, &statbuf) != 0) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTOPENFILE_SS, envFileName, strerror(errno));
      FREE(envFileName);
      return NULL;
   } 
   
   size = statbuf.st_size;
   line = (char *)malloc(size + 1);
   if (line == NULL) {
      qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
      FREE(envFileName);
      return NULL;
   }

   /* open sge environment file */
   if ((envFile = fopen(envFileName, "r")) == NULL) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTOPENFILE_SS, envFileName, strerror(errno));
      FREE(envFileName);
      FREE(line);
      return NULL;
   }

   /* set all environment variables, change to directory named by PWD */
   while (fgets(line, size, envFile) != NULL) {
      /* clean trailing garbage (\n, \r, EOF ...) */
      char *c = &line[strlen(line)];
      while (iscntrl(*(--c))) {
         *c = 0;
      }

      if (strncmp(line, "QRSH_COMMAND=", 13) == 0) {
         if ((command = (char *)malloc(strlen(line) - 13 + 1)) == NULL) {
            qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
            fclose(envFile); 
            FREE(envFileName);
            FREE(line);
            return NULL;
         }
         strcpy(command, line + 13);
      } else {  
         if (strncmp(line, "QRSH_WRAPPER=", 13) == 0) {
            if (*(line + 13) == 0) {
               fprintf(stderr, MSG_QRSH_STARTER_EMPTY_WRAPPER);
            } else {
               if ((*wrapper = (char *)malloc(strlen(line) - 13 + 1)) == NULL) {
                  qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
                  fclose(envFile); 
                  FREE(envFileName);
                  FREE(line);
                  return NULL;
               }
               strcpy(*wrapper, line + 13);
            }
         } else {
            /* set variable */
            if (!sge_putenv(line)) {
               fclose(envFile); 
               FREE(envFileName);
               FREE(line);
               return NULL;
            }
         }
      }
   }

   fclose(envFile); 
   FREE(envFileName);
   FREE(line);

   /* 
    * Use starter_method if it is supplied
    * and not overridden by QRSH_WRAPPER
    */
    
   if (*wrapper == NULL) {
      char *starter_method = search_conf_val("starter_method");
      if (starter_method != NULL && strcasecmp(starter_method, "none") != 0) { 
         char buffer[128];
         *wrapper = starter_method;
         sprintf(buffer, "%s=%s", "SGE_STARTER_SHELL_PATH", ""); sge_putenv(buffer);
         sprintf(buffer, "%s=%s", "SGE_STARTER_SHELL_START_MODE", "unix_behavior"); sge_putenv(buffer);
         sprintf(buffer, "%s=%s", "SGE_STARTER_USE_LOGIN_SHELL", "false"); sge_putenv(buffer);
      } 
   }
   
   return command;
}

/****** Interactive/qrsh_starter/readConfig() *****************************************
*  NAME
*     readConfig() -- read the jobs configuration
*
*  SYNOPSIS
*     static int readConfig(const char *jobdir) 
*
*  FUNCTION
*     Reads the jobs configuration (<job spool dir>/config).
*
*  INPUTS
*     const char *jobdir - the jobs spool directory
*
*  RESULT
*     static int - 0, if an error occured
*                  1, if function completed without errors
*
*******************************************************************************/
static int readConfig(const char *jobdir)
{
   char *configFileName = NULL;

   /* build path of configfile */
   configFileName = (char *)malloc(strlen(jobdir) + strlen("config") + 2);

   if(configFileName == NULL) {
      qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
      return 0;
   }

   sprintf(configFileName, "%s/config", jobdir);

   /* read jobs config file */
   if(read_config(configFileName) != 0) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTREADCONFIGFROMFILE_S, configFileName);
      free(configFileName);
      return 0;
   }

   free(configFileName);
   return 1;
}

/****** Interactive/qrsh_starter/changeDirectory() *****************************************
*  NAME
*     changeDirectory() -- change to directory named in job config
*
*  SYNOPSIS
*     static int changeDirectory(void) 
*
*  FUNCTION
*     Reads the target working directory for a qrsh job from the jobs 
*     configuration and tries to 
*     change the current working directory.
*
*  RESULT
*     static int - 0, if an error occured
*                  1, if function completed without errors
*  SEE ALSO
*     Interactive/qrsh_starter/readConfig()
*
*******************************************************************************/
static int changeDirectory(void) 
{
   char *cwd = NULL;

   /* get jobs target directory */
   cwd = search_conf_val("cwd");

   if(cwd == NULL) {
      qrsh_error("MSG_QRSH_STARTER_NOCWDINCONFIG");
      return 0;
   }

   /* change to dir cwd */
   if(chdir(cwd) == -1) {
      fprintf(stderr, MSG_QRSH_STARTER_CANNOTCHANGEDIR_SS, cwd, 
              strerror(errno));
      return 0;
   }

   return 1;
}


/****** Interactive/qrsh_starter/write_pid_file() ******************************
*
*  NAME
*     write_pid_file()  -- write a pid to file pid in $TMPDIR
*
*  SYNOPSIS
*     static int write_pid_file(pid_t pid);
*
*  FUNCTION
*     Writes the given pid to a file named pid in the directory
*     contained in environment variable TMPDIR
*
*  INPUTS
*     pid - the pid to write
*
*  RESULT
*     1, if all actions could be performed
*     0, if an error occured. Possible error situations are:
*        - the environement variable TMPDIR cannot be read
*        - the file cannot be opened
*
****************************************************************************
*/
static int write_pid_file(pid_t pid) 
{
   char *tmpdir = NULL;
   char *task_id_str = NULL;
   char pid_file_name[SGE_PATH_MAX];
   FILE *pid_file;

   if((tmpdir = search_conf_val("qrsh_tmpdir")) == NULL) {
      qrsh_error(MSG_CONF_NOCONFVALUE_S, "qrsh_tmpdir");
      return 0;
   }

   task_id_str = search_conf_val("qrsh_task_id");

   if(task_id_str != NULL) {
      sprintf(pid_file_name, "%s/pid.%s", tmpdir, task_id_str);
   } else {
      sprintf(pid_file_name, "%s/pid", tmpdir);
   }
   
   if((pid_file = fopen(pid_file_name, "w")) == NULL) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTWRITEPID_S, pid_file_name);
      return 0;
   }

   chmod(pid_file_name, 00744);
   fprintf(pid_file, pid_t_fmt, pid);
   fclose(pid_file);
   return 1;
}

/****** Interactive/qrsh_starter/forward_signal() ***************************************
*
*  NAME
*     forward_signal() -- forward a signal to qrsh_starter's child
*
*  SYNOPSIS
*     static void forward_signal(int sig);
*
*  FUNCTION
*     Forwards the signal <sig> to the process group given
*     in the global variable <child_pid>.
*
*  INPUTS
*     sig - the signal to forward
*
****************************************************************************
*/
static void forward_signal(int sig)
{
   if(child_pid > 0) {
      kill(-child_pid, sig);
   }
}

/****** Interactive/qrsh_starter/split_command() ******************************
*  NAME
*     split_command() -- split commandline into tokens
*
*  SYNOPSIS
*     static int split_command(char *command, char ***cmdargs) 
*
*  FUNCTION
*     The command to be executed by qrsh_starter may contain multiple 
*     arguments, quotes, double quotes, back quotes etc. within the 
*     arguments ...
*     To preserve all this information, qrsh writes the command line arguments
*     to an environment variable QRSH_COMMAND and separates the arguments by
*     the character with code 255 (0xff).
*     split_command splits the resulting string into the original arguments
*     and writes them to a string array.
*
*  INPUTS
*     char *command   - arguments separated by 0xff
*     char ***cmdargs - pointer to string array to be filled with arguments
*
*  RESULT
*     static int - the number of arguments or 0 if an error occured
*
*  SEE ALSO
*     Interactive/qrsh_starter/join_command()
*
*******************************************************************************/
static int split_command(char *command, char ***cmdargs) {
   /* count number of arguments */
   int counter = 1;
   char *s = command;
   int argc;
   char **args;
   int i,end;
   char delimiter[2];
/* FILE *debug; */

/* debug = fopen("/tmp/qrsh_debug.txt", "a"); */
/* fprintf(debug, "\n\n qrsh run with strlen(command) = %d\n", strlen(command)); */
   sprintf(delimiter, "%c", 0xff);

   while(*s) {
      if(*s++ == delimiter[0]) {
         counter++;
      }
   }
/* fprintf(debug, "command consists of %d arguments\n", counter); */
   /* copy arguments */
   argc = 0;
   args = (char **)malloc(counter * sizeof(char *));
   
   if(args == NULL) {
/* fclose(debug); */
      return 0;
   }

   /* do not use strtok(), strtok() is seeing 2 or more delimiters as one !!! */
   s=command;
   args[argc++] = s;
   end =  strlen(command);
   for(i = 0; i < end ; i++) {
      if (s[i] == delimiter[0]) {
         s[i] = 0;
            args[argc++] = &s[i+1];
      }
   } 

#if 0
   /* debug code */
   fflush(stderr);
   fprintf(stdout, "counter = %d\n", counter);
   for(i = 0; i < argc; i++) {
      fprintf(stdout, "split_command: args[%d] = %s\n", i, args[i]);
   }
   fflush(stdout);
#endif

/* fclose(debug); */
   *cmdargs = args;
   return argc;
}

/****** Interactive/qrsh_starter/join_command() ********************************
*  NAME
*     join_command() -- join arguments to a single string
*
*  SYNOPSIS
*     static char* join_command(int argc, char **argv) 
*
*  FUNCTION
*     Joins arguments given in an argument vector (string array) to a single
*     character string where the arguments are separated by a single blank.
*     This is used, to pass an argument vector as one argument to a call
*     <shell> -c <commandline>.
*
*  INPUTS
*     int argc    - argument count
*     char **argv - argument vector
*
*  RESULT
*     static char* - the resulting commandline or NULL, if an error occured.
*
*  SEE ALSO
*     Interactive/qrsh_starter/split_command()
*
*******************************************************************************/
static char *join_command(int argc, char **argv) {
   int i;
   int length = 0;
   char *buffer;
   
   /* calculate needed size */
   for(i = 0; i < argc; i++) {
      length += strlen(argv[i]);
   }

   /* add spaces and \0 */
   length += argc;

   buffer = malloc(length * sizeof(char));

   if(buffer == NULL) {
      return 0;
   }

   strcpy(buffer, argv[0]);
   for(i = 1; i < argc; i++) {
      strcat(buffer, " ");
      strcat(buffer, argv[i]);
   }

   return buffer;
}


/****** Interactive/qrsh_starter/startJob() ************************************
*
*  NAME
*     startJob() -- start a shell with commands to execute
*
*  SYNOPSIS
*     static int startJob(char *command, char *wrapper, int noshell);
*
*  FUNCTION
*     Starts the commands and arguments to be executed as 
*     specified in parameter <command>. 
*     If the parameter noshell is set to 1, the command is directly called
*     by exec.
*     If a wrapper is specified (parameter wrapper, set by environment
*     variable QRSH_WRAPPER), this wrapper is called and is passed the 
*     command to execute as commandline parameters.
*     If neither noshell nor wrapper is set, a users login shell is called
*     with the parameters -c <command>.
*     The child process creates an own process group.
*     The pid of the child process is written to a pid file in $TMPDIR.
*
*  INPUTS
*     command - commandline to be executed
*     wrapper - name and path of a wrapper script
*     noshell - if != 0, call the command directly without shell
*
*  RESULT
*     status of the child process after it terminated
*     or EXIT_FAILURE, if the process of starting the child 
*     failed because of one of the following error situations:
*        - fork failed
*        - the pid of the child process cannot be written to pid file
*        - the name of actual user cannot be determined
*        - info about the actual user cannot be determined (getpwnam)
*        - necessary memory cannot be allocated
*        - executing the shell failed
*
*  SEE ALSO
*     Interactive/qrsh_starter/write_pid_file()
*     Interactive/qrsh_starter/split_command()
*     Interactive/qrsh_starter/join_command()
*
****************************************************************************
*/
static int startJob(char *command, char *wrapper, int noshell)
{

   child_pid = fork();
   if(child_pid == -1) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTFORKCHILD_S, strerror(errno));
      return EXIT_FAILURE;
   }

   if(child_pid) {
      /* parent */
      int status;

#if defined(LINUX)
      int ttyfd;
#endif

      signal(SIGINT,  forward_signal);
      signal(SIGQUIT, forward_signal);
      signal(SIGTERM, forward_signal);

      /* preserve pseudo terminal */
#if defined(LINUX)
      ttyfd = open("/dev/tty", O_RDWR);
      if (ttyfd != -1) {
         tcsetpgrp(ttyfd, child_pid);
         close(ttyfd); 
      }
#endif

      while(waitpid(child_pid, &status, 0) != child_pid && errno == EINTR);
      return(status);
   } else {
      /* child */
      char *shell    = NULL;
      char *userName = NULL;
      int    argc = 0;
      const char **args = NULL;
      struct passwd *pw = NULL;
      char *cmd = NULL;
      int cmdargc;
      char **cmdargs = NULL;
      int i;

      if(!write_pid_file(getpid())) {
         qrsh_error(MSG_QRSH_STARTER_CANNOTWRITEPID_S, "");
         exit(EXIT_FAILURE);
      }

      cmdargc = split_command(command, &cmdargs);

      if(cmdargc == 0) {
         qrsh_error(MSG_QRSH_STARTER_INVALIDCOMMAND);
         exit(EXIT_FAILURE);
      }

      if(!noshell) {
         if((userName = search_conf_val("job_owner")) == NULL) {
            qrsh_error(MSG_QRSH_STARTER_CANNOTGETLOGIN_S, strerror(errno));
            exit(EXIT_FAILURE);
         }

         if((pw = sge_getpwnam(userName)) == NULL) {
            qrsh_error(MSG_QRSH_STARTER_CANNOTGETUSERINFO_S, strerror(errno));
            exit(EXIT_FAILURE);
         }
         
         shell = pw->pw_shell;
         
         if(shell == NULL) { 
            qrsh_error(MSG_QRSH_STARTER_CANNOTDETERMSHELL_S,"/bin/sh");
            shell = "/bin/sh";
         } 
      }
     
      if((args = malloc((cmdargc + 3) * sizeof(char *))) == NULL) {
         qrsh_error(MSG_QRSH_STARTER_MALLOCFAILED_S, strerror(errno));
         exit(EXIT_FAILURE);
      }         
    
      if(wrapper == NULL) {
         if(noshell) {
            cmd = cmdargs[0];
            for(i = 0; i < cmdargc; i++) {
               args[argc++] = cmdargs[i];
            }
         } else {
            cmd = shell;
            args[argc++] = sge_basename(shell, '/');
            args[argc++] = "-c";
            args[argc++] = join_command(cmdargc, cmdargs);
         }
      } else {
         cmd = wrapper;
         args[argc++] = sge_basename(wrapper, '/');
         for(i = 0; i < cmdargc; i++) {
            args[argc++] = cmdargs[i];
         }
      }

      args[argc++] = NULL;

#if 0
{
   /* debug code */
   int i;
   
   fflush(stdout) ; fflush(stderr);
   printf("qrsh_starter: executing %s\n", cmd);
   for(i = 1; args[i] != NULL; i++) {
      printf("args[%d] = %s\n", i, args[i]);
   }
   printf("\n");
   fflush(stdout) ; fflush(stderr); 
} 
#endif

      SETPGRP;
      execvp(cmd, (char *const *)args);
      /* exec failed */
      fprintf(stderr, MSG_QRSH_STARTER_EXECCHILDFAILED_S, args[0], 
              strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* will never be reached */
   return EXIT_FAILURE; 
}

/****** Interactive/qrsh_starter/writeExitCode() *******************************
*
*  NAME
*    writeExitCode() -- write exit code of child process to file
*
*  SYNOPSIS
*     static int writeExitCode(int myExitCode, int programExitCode)
*
*  FUNCTION
*     If myExitCode != EXIT_SUCCESS, that means, if an error occured in
*     qrsh_starter, write this exit code to file,
*     else write the exit code of the child process (programExitCode).
*     The exit code is written to a file "qrsh_exit_code" in the
*     directory $TMPDIR.
*
*  INPUTS
*     myExitCode      - status of qrsh_starter
*     programExitCode - status of the child process
*
*  RESULT
*     EXIT_SUCCESS, if all actions could be performed,
*     EXIT_FAILURE, if one of the following errors occured:
*        - the environment variable TMPDIR cannot be read
*        - the file $TMPDIR/qrsh_exit_code cannot be written
*
****************************************************************************
*/
static int writeExitCode(int myExitCode, int programExitCode) 
{   
   int exitCode;
   char *tmpdir = NULL;
   char *taskid = NULL;
   FILE *file   = NULL;
   char fileName[SGE_PATH_MAX];

   if(myExitCode != EXIT_SUCCESS) {
      exitCode = MAKEEXITSTATUS(myExitCode);
   } else {
      exitCode = programExitCode;
   }

   if((tmpdir = search_conf_val("qrsh_tmpdir")) == NULL) {
      qrsh_error(MSG_CONF_NOCONFVALUE_S, "qrsh_tmpdir");
      return EXIT_FAILURE;
   }
  
   taskid = search_conf_val("qrsh_task_id");
   
   if(taskid != NULL) {
      sprintf(fileName, "%s/qrsh_exit_code.%s", tmpdir, taskid);
   } else {
      sprintf(fileName, "%s/qrsh_exit_code", tmpdir);
   }

   if((file = fopen(fileName, "w")) == NULL) {
      qrsh_error(MSG_QRSH_STARTER_CANNOTOPENFILE_SS, fileName, strerror(errno));
      return EXIT_FAILURE;
   }
  
   chmod(fileName, 00744);
   fprintf(file, "%d", exitCode);

   fclose(file);
   
   return EXIT_SUCCESS;
}

/****** Interactive/qrsh_starter/--qrsh_starter ********************************
*
*  NAME
*     qrsh_starter -- start a command special correct environment
*
*  SYNOPSIS
*     qrsh_starter <environment file> <noshell>
*     int main(int argc, char **argv[])
*
*  FUNCTION
*     qrsh_starter is used to start a command, optionally with additional
*     arguments, in a special environment.
*     The environment is read from the given <environment file>.
*     The command to be executed is read from the environment variable
*     
*     script (environment  variable QRSH_WRAPPER) or (default) in a users login
*     shell (<shell> -c <command>).
*     On exit of the command, or if an error occurs, an exit code is written
*     to the file $TMPDIR/qrsh_exit_code.
*
*     qrsh_starter is called from qrsh to start the remote processes in 
*     the correct environment.
*
*  INPUTS
*     environment file - file with environment information, each line 
*                        contains a tuple <name>=<value>
*     noshell          - if this parameter is passed, the command will be
*                        executed standalone
*
*  RESULT
*     EXIT_SUCCESS, if all actions could be performed,
*     EXIT_FAILURE, if an error occured
*
*  EXAMPLE
*     setenv QRSH_COMMAND "echo test"
*     env > ~/myenvironment
*     rsh <hostname> qrsh_starter ~/myenvironment 
*
*  SEE ALSO
*     Interactive/qsh/--Introduction
*
****************************************************************************
*/

int main(int argc, char *argv[])
{
   int   exitCode = 0;
   char *command  = NULL;
   char *wrapper = NULL;
   int  noshell  = 0;

#if 0
   fprintf(stderr, "qrsh_starter can write to qrsh's stderr\n");
#endif

   /* check for correct usage */
   if(argc < 2) {
      fprintf(stderr, "usage: %s <job spooldir> [noshell]\n", argv[0]);
      exit(EXIT_FAILURE);        
   }

   /* check for noshell */
   if(argc > 2) {
      if(strcmp(argv[2], "noshell") == 0) {
         noshell = 1;
      }
   }

#if 0
   qrsh_error("test error - has to appear in qrsh and qmaster messages file\n");
#endif

   if(!readConfig(argv[1])) {
      writeExitCode(EXIT_FAILURE, 0);
      exit(EXIT_FAILURE);
   }

   /* setup environment */
   command = setEnvironment(argv[1], &wrapper);
   if(command == NULL) {
      writeExitCode(EXIT_FAILURE, 0);
      exit(EXIT_FAILURE);
   }   

   if(!changeDirectory()) {
      writeExitCode(EXIT_FAILURE, 0);
      exit(EXIT_FAILURE);
   }

   /* start job */
   exitCode = startJob(command, wrapper, noshell);

   /* write exit code and exit */
   return writeExitCode(EXIT_SUCCESS, exitCode);
}
