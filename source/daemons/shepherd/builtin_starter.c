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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>

#include <sge_getpwnam.h>
#include <sge_string.h>
#include <setosjobid.h>

#if defined(CRAY)
#   if !defined(SIGXCPU)
#       define SIGXCPU SIGCPULIM
#   endif
    /* for killm category on Crays */
#   include <sys/category.h>
struct rusage {
   struct timeval ru_stime;
   struct timeval ru_utime;
};
    /* for job/session stuff */
#   include <sys/session.h>
    /* times() */
#   include <sys/times.h>
#endif

#if defined(NECSX4) || defined(NECSX5)
#  include <sys/types.h>
#  include <sys/disp.h>
#  include <sys/rsg.h> 

#  define NEC_UNDEF_VALUE (-999)
#endif 

#include "builtin_starter.h"
#include "err_trace.h"
#include "setrlimits.h"
#include "get_path.h"
#include "basis_types.h"
#include "setenv.h"
#include "execution_states.h"
#include "am_chdir.h"
#include "qlogin_starter.h"

/* from src */
#include "config_file.h"
#include "sge_pgrp.h"

/* from utilib */
#include "sge_getpwnam.h"
#include "sge_set_uid_gid.h"
#include "sge_set_def_sig_mask.h"
#include "sge_switch_user.h"
#include "sge_stat.h" 

/* static functions */
static char **read_job_args(char **args, int extra_args);
static char *build_path(int type);

extern int shepherd_state;
extern char shepherd_job_dir[];

/************************************************************************
 This is the shepherds buitin starter.

 It is also used to start the external starter command .. 
 ************************************************************************/
void son(
char *childname,
char *script_file,
int truncate_stderr_out 
) {
   int in, out, err;          /* hold fds */
   char *stdout_path, *stderr_path = NULL, *shell_path;
   char *cwd;
   struct passwd *pw=NULL;
   char err_str[256];
   int merge_stderr;
   pid_t pid, ppid, pgrp, newpgrp;
   int i;
   int min_gid, min_uid;
   gid_t add_grp_id = 0;
   char *target_user = NULL, *intermediate_user = NULL;
   char *shell_start_mode, *cp, *shell_basename, argv0[256];
   int use_login_shell;
   int ret;
   int is_interactive  = 0;
   int is_qlogin  = 0;
   int is_rsh = 0;
   int is_rlogin = 0;
   int qlogin_starter = 0;
   char str_title[512]="";
   int use_qsub_gid;
   gid_t gid;
   char *tmp_str;
   char *starter_method;

   foreground = 0; /* VX sends SIGTTOU if trace messages go to foreground */

   /* From here only the son --------------------------------------*/
   if (!script_file)
      shepherd_error("received NULL als script file");

   /*
   ** interactive jobs have script_file name interactive and
   ** as exec_file the configuration value for xterm
   */
   
   if (!strcasecmp(script_file, "INTERACTIVE")) {
      is_interactive = 1;

      shepherd_trace("processing interactive job");
      script_file = get_conf_val("exec_file");
   }

   /*
   ** login or rsh or rlogin jobs have the qlogin_starter as their job script
   */
   
   if(!strcasecmp(script_file, "QLOGIN") || !strcasecmp(script_file, "QRSH") || !strcasecmp(script_file, "QRLOGIN")) {
      is_qlogin = 1;

      if(!strcasecmp(script_file, "QRSH")) {
         is_rsh = 1;
      }   

      if(!strcasecmp(script_file, "QRLOGIN")) {
         is_rlogin = 1;
      }   

      shepherd_trace("processing qlogin job");
      
      qlogin_starter = 1;
   } 

   if(qlogin_starter) {
      /* must force to run the qlogin starter as root, since it needs
         access to /dev/something */
      target_user = "root";
   }

   pid = getpid();
   ppid = getppid();
   pgrp = GETPGRP;

#ifdef SOLARIS
   if(!qlogin_starter || is_rsh)
#endif
   if ((newpgrp = setsid()) < 0) {
      sprintf(err_str, "setsid() failed, errno=%d", errno);
      shepherd_error(err_str);
   }

   /* run these procedures under a different user ? */
   if (!strcmp(childname, "pe_start") ||
       !strcmp(childname, "pe_stop") ||
       !strcmp(childname, "prolog") ||
       !strcmp(childname, "epilog")) {
      /* syntax is [<user>@]<path> <arguments> */
      char *s;
      s = strpbrk(script_file, "@ ");
      if (s && *s == '@') {
         *s = '\0';
         target_user = script_file; 
         script_file = &s[1];
      }
   }

   if (!target_user)
      target_user = get_conf_val("job_owner");
   else {
      /* 
       *  The reason for using the job owner as intermediate user
       *  is that for output of prolog/epilog the same files are
       *  used as for the job. This causes problems if the output
       *  file is created by a prolog running under a user different
       *  from the job owner, because the output files are then owned
       *  by the prolog user and then the job owner has no permissions
       *  to write into this file...
       *  
       *  We work around this problem by opening output files as 
       *  job owner and then changing to the prolog user. 
       *
       *  Additionally it prevents that a root procedures write to
       *  files which may not be accessable by the job owner 
       *  (e.g. /etc/passwd)
       */
      intermediate_user = get_conf_val("job_owner");
   }

#if defined(ALPHA)
   /* setlogin() stuff */
   switch2start_user();
   if (!geteuid()) {
      if (setlogin(target_user)) {
         switch2admin_user();
         sprintf(err_str, "setlogin(%s) failed: %s", target_user, strerror(errno));
         shepherd_error(err_str);
      }
      switch2admin_user();
   }
#endif

   sprintf(err_str, "pid="pid_t_fmt" pgrp="pid_t_fmt" sid="pid_t_fmt" old pgrp="pid_t_fmt" getlogin()=%s", 
           pid, newpgrp, newpgrp, pgrp, (cp = getlogin()) ? cp : "<no login set>");
   shepherd_trace(err_str);

   pw = sge_getpwnam(target_user);
   if (!pw) {
      sprintf(err_str, "can't get password entry for user \"%s\"", target_user);
      shepherd_error(err_str);
   }

   setrlimits(!strcmp(childname, "job"));

   umask(022);

   if (!strcmp(childname, "job")) {
      char *write_osjob_id = get_conf_val("write_osjob_id");
      if(write_osjob_id != NULL && atoi(write_osjob_id) != 0) {
         setosjobid(newpgrp, &add_grp_id, pw);
      }   
   }
   
   set_environment();

   /* make job owner the owner of error/trace file. So we can write
    *  diagnostics after changing to this user
    *
    * also makes job owner own the exit_status file
    * the "exit_status" is created here and indicates that son is started
    */
   err_trace_chown_files(pw->pw_uid);

   min_gid = atoi(get_conf_val("min_gid"));
   min_uid = atoi(get_conf_val("min_uid"));

   /** 
    ** Set uid and gid and
    ** (add additional group id), switches to start user (root) 
    **/
    tmp_str = search_conf_val("qsub_gid");
    if (strcmp(tmp_str, "no")) {
       use_qsub_gid = 1;   
       gid = atol(tmp_str);
    }
    else {
       use_qsub_gid = 0;
       gid = 0;
    }
                                                                      
   if(qlogin_starter) { 
      ret = setuidgidaddgrp(target_user, intermediate_user, 0, 0, 
                            add_grp_id, err_str, use_qsub_gid, gid);
   } else {   
      ret = setuidgidaddgrp(target_user, intermediate_user, min_gid, min_uid, 
                            add_grp_id, err_str, use_qsub_gid, gid);
   }   
   if (ret < 0) {
      shepherd_trace(err_str);
      sprintf(err_str, "try running further with uid=%d", (int)getuid());
      shepherd_trace(err_str);
   } 
   else if (ret > 0) {
      /*
      ** violation of min_gid or min_uid
      */
      shepherd_error(err_str);
   }
   shell_start_mode = get_conf_val("shell_start_mode");

   shepherd_trace("closing all filedescriptors");
   shepherd_trace("further messages are in \"error\" and \"trace\"");
   fflush(stdin);
   fflush(stdout);
   fflush(stderr);
   
   {
      int fdmax = sysconf(_SC_OPEN_MAX);
      for (i = 0; i < fdmax; i++)
         close(i);
   }
   foreground = 0;

   /* We have different possiblities to start the job script:
      - We can start it as login shell or not
        Login shell start means that argv[0] starts with a '-'
      - We can try to be posix compliant and not to evaluate #!/bin/sh
        as it is done by normal shellscript starts
      - We can feed the shellscript to stdin of a started shell */

   merge_stderr = atoi(get_conf_val("merge_stderr"));

   if (!strcmp(childname, "pe_start") ||
       !strcmp(childname, "pe_stop") ||
       !strcmp(childname, "prolog") ||
       !strcmp(childname, "epilog")) {

      /* use login shell */
      use_login_shell = 1; 

      /* take shell from passwd */
      shell_path = strdup(pw->pw_shell);
      sprintf(err_str, "using \"%s\" as shell of user \"%s\"", pw->pw_shell, target_user);
      shepherd_trace(err_str);
      
      /* use -c for starting */
      shell_start_mode = "start_as_command";

      if (!strcmp(childname, "prolog") || !strcmp(childname, "epilog")) {
         stdout_path  = build_path(SGE_STDOUT);
         if (!merge_stderr)
            stderr_path  = build_path(SGE_STDERR);
      } else {
         stdout_path  = build_path(SGE_PAR_STDOUT);
         if (!merge_stderr)
            stderr_path  = build_path(SGE_PAR_STDERR);
      }
   } 
   else {
      shell_path = get_conf_val("shell_path");
      use_login_shell = atoi(get_conf_val("use_login_shell"));
  
      stdout_path  = build_path(SGE_STDOUT);
      if (!merge_stderr)
         stderr_path  = build_path(SGE_STDERR);
   }

   cwd = get_conf_val("cwd");

   {
      char *tmp_str;
      int tmp_value;

      if ((tmp_str = get_conf_val("set_sge_env")) 
          && (tmp_value = atoi(tmp_str)) 
          && tmp_value) {
         sge_setenv("SGE_STDOUT_PATH", stdout_path);
         sge_setenv("SGE_STDERR_PATH", merge_stderr?stdout_path:stderr_path);
         sge_setenv("SGE_CWD_PATH", cwd);
      }
      if ((tmp_str = get_conf_val("set_cod_env")) 
          && (tmp_value = atoi(tmp_str)) 
          && tmp_value) {
         sge_setenv("COD_STDOUT_PATH", stdout_path);
         sge_setenv("COD_STDERR_PATH", merge_stderr?stdout_path:stderr_path);
         sge_setenv("COD_CWD_PATH", cwd);
      }
      if ((tmp_str = get_conf_val("set_grd_env")) 
          && (tmp_value = atoi(tmp_str)) 
          && tmp_value) {
         sge_setenv("GRD_STDOUT_PATH", stdout_path);
         sge_setenv("GRD_STDERR_PATH", merge_stderr?stdout_path:stderr_path);
         sge_setenv("GRD_CWD_PATH", cwd);
      }
   }

   /*
   ** for interactive jobs, we disregard the current shell_start_mode
   */
   if (is_interactive || is_qlogin)
      shell_start_mode = "unix_behaviour";

   in = 0;
   if (!strcasecmp(shell_start_mode, "script_from_stdin")) {
      in = open(script_file, O_RDONLY);
      if (in == -1) {
         sprintf(err_str,  "error: can't open %s script file \"%s\": %s", 
               childname, script_file, strerror(errno));
         shepherd_error(err_str);
      }
   } 
   else {
      /* need to open a file as fd0 */
      in = open("/dev/null", O_RDONLY); 

      if (in == -1) {
         shepherd_state = SSTATE_OPEN_OUTPUT;
         shepherd_error("error: can't open /dev/null as dummy input file");
      }
   }

   if (in != 0)
      shepherd_error("error: fd for in is not 0");

   if(!qlogin_starter) {
      /* -cwd or from pw->pw_dir */
      if (sge_cwd_chdir(cwd)) {
         shepherd_state = SSTATE_NO_CWD;
         sprintf(err_str, "error: can't chdir to %s: %s", cwd, strerror(errno));
         shepherd_error(err_str);
      }
   }

   /* open stdout - not for interactive jobs */
   if (!is_interactive && !is_qlogin) {
      if (truncate_stderr_out)
         out = open(stdout_path, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0644);
      else
         out = open(stdout_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (out==-1) {
         sprintf(err_str, "error: can't open output file \"%s\": %s", 
                 stdout_path, strerror(errno));
         shepherd_state = SSTATE_OPEN_OUTPUT;
         shepherd_error(err_str);
      }
      if (out!=1)
         shepherd_error("error: fd out is not 1");

      /* open stderr */
      if (merge_stderr) {
         shepherd_trace("using stdout as stderr");
         dup2(1, 2);
      }
      else {
         if (truncate_stderr_out) 
            err = open(stderr_path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
         else
            err = open(stderr_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
         if (err == -1) {
            sprintf(err_str, "error: can't open output file \"%s\": %s", 
                    stderr_path, strerror(errno));
            shepherd_state = SSTATE_OPEN_OUTPUT;
            shepherd_error(err_str);
         }

#ifndef __INSURE__
         if (err!=2) {
            shepherd_trace("unexpected fd");
            shepherd_error("error: fd err is not 2");
         }
#endif
      }
   }

   /*
    * Use starter_method if it is supplied
    */

   if (!is_interactive && !is_qlogin && !qlogin_starter &&
       !strcmp(childname, "job") &&
       (starter_method = get_conf_val("starter_method")) &&
       strcasecmp(starter_method, "none")) {

      sge_setenv("SGE_STARTER_SHELL_PATH", shell_path);
      shell_path = starter_method;
      sge_setenv("SGE_STARTER_SHELL_START_MODE", shell_start_mode);
      if (!strcasecmp("unix_behavior", shell_start_mode))
         shell_start_mode = "posix_compliant";
      if (use_login_shell)
         sge_setenv("SGE_STARTER_USE_LOGIN_SHELL", "true");
   }

   /* get basename of shell for building argv[0] */
   cp = strrchr(shell_path, '/');
   if (!cp)
      shell_basename = shell_path;
   else
      shell_basename = cp+1;

   {
      SGE_STRUCT_STAT sbuf;

      if ((!strcasecmp("script_from_stdin", shell_start_mode) ||
           !strcasecmp("posix_compliant", shell_start_mode) ||
           !strcasecmp("start_as_command", shell_start_mode)) && 
           !is_interactive && !is_qlogin) { 
         if (SGE_STAT(shell_path, &sbuf)) {
            sprintf(err_str, "unable to find shell \"%s\"", shell_path);
            shepherd_state = SSTATE_NO_SHELL;
            shepherd_error(err_str);
         }
      }
   }

   if (use_login_shell) {
      strcpy(argv0, "-");
      strcat(argv0, shell_basename);
   }
   else
      strcpy(argv0, shell_basename);

   sge_set_def_sig_mask(0, NULL);
   
   /*
   ** prepare xterm title for interactive jobs
   */
   if (is_interactive) {
      char *queue;
      char *host;
      char *job_id;

      queue = get_conf_val("queue");
      host = get_conf_val("host");
      job_id = get_conf_val("job_id");
      sprintf(str_title,
              "SGE Interactive Job %s on %s in Queue %s",
              job_id, host, queue);
   }

   if (intermediate_user) {
      if(qlogin_starter) {
         ret = setuidgidaddgrp(target_user, NULL, 0, 0, add_grp_id, err_str, use_qsub_gid, gid);
      } else {
         ret = setuidgidaddgrp(target_user, NULL, min_gid, min_uid, add_grp_id, err_str, use_qsub_gid, gid);
      }
      if (ret < 0) {
         shepherd_trace(err_str);
         sprintf(err_str, "try running further with uid=%d", (int)getuid());
         shepherd_trace(err_str);
      } 
      else if (ret > 0) {
         /*
         ** violation of min_gid or min_uid
         */
         shepherd_error(err_str);
      }
   }

   /*
   ** if we dont check if the script_file exists, then in case of
   ** "start_as_command" the result is an error report
   ** saying that the script exited with 1
   */
   if (!is_qlogin) {
      if (strcasecmp(shell_start_mode, "raw_exec")) {
         SGE_STRUCT_STAT sbuf;
         char file[SGE_PATH_MAX + 1];
         char *pc;
   
         strncpy(file, script_file, SGE_PATH_MAX);
         file[sizeof(file) - 1] = 0;
         pc = strchr(file, ' ');
         if (pc) {
            *pc = 0;
         }
   
         if (SGE_STAT(file, &sbuf)) {
            /*
            ** generate a friendly error messages especially for interactive jobs
            */
            if (is_interactive) {
               sprintf(err_str, "unable to find xterm executable \"%s\" for interactive job", file);
            }
            else {
               sprintf(err_str, "unable to find %s file \"%s\"", childname, file);
            }
   
            shepherd_error(err_str);
         }
   
         if (!(sbuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            sprintf(err_str, "%s file \"%s\" is not executable", childname, file);
            shepherd_error(err_str);
         }
      }
   }

   start_command(shell_path, script_file, argv0, shell_start_mode, is_interactive, is_qlogin, is_rsh, is_rlogin, str_title);
   return;
}

/*******************************************************************
 read environment from "environment"-file and make it actual
 *******************************************************************/
int set_environment()
{
   FILE *fp;
   char buf[10000], *name, *value, err_str[10000];
   int line=0;
   
   if (!(fp = fopen("environment", "r"))) {
      sprintf(err_str, "can't open environment file: %s",
              strerror(errno));
      shepherd_error(err_str);
      return 1;
   }

#if defined(IRIX6)
   {
      FILE *fp;
      ash_t ash;

      fp = fopen("osjobid", "r");
      if (fp) {
         fscanf(fp, "%lld", &ash);
         fclose(fp);
         if (ash) {
            char s[100];
            sprintf(s, "%lld", ash);
            sge_setenv("OSJOBID", s);
         }
      }
   }
#elif defined(CRAY)
   {
      FILE *fp;
      int jobid;
      int i;

      fp = fopen("osjobid", "r");
      if (fp) {
         fscanf(fp, "%d", &jobid);
         fclose(fp);
         if (jobid) {
            char s[100];
            sprintf(s, "%d", jobid);
            sge_setenv("OSJOBID", s);
         }
      }
   }
#elif defined(NECSX4) || defined(NECSX5)
   {
      FILE *fp;
      id_t jobid;

      fp = fopen("osjobid", "r");
      if (fp) {
         fscanf(fp, "%d", &jobid);
         fclose(fp);
         if (jobid) {
            char s[100];
            sprintf(s, "%ld", jobid);
            sge_setenv("OSJOBID", s);
         }                            
      }
   }
#endif

   while (fgets(buf, sizeof(buf), fp)) {

      line++;

      if (strlen(buf) <= 1)     /* empty line or lastline */
         continue;

      name = strtok(buf, "=");
      if (!name) {
         fclose(fp);
         sprintf(err_str, 
                 "error reading environment file: line=%d, contents:%s",
                 line, buf);
         shepherd_error(err_str);
         return 1;
      }
      value = strtok(NULL, "\n");
      if (!value)
         value = "";

      sge_setenv(name, value);
   }

   fclose(fp);
   return 0;
}

static char **read_job_args(char **preargs, int extra_args)
{
   int n_preargs = 0;
   unsigned long n_job_args = 0;
   unsigned long i;
   char **pstr;
   char **args;
   char conf_val[256];
   char *cp;

   for (pstr = preargs; pstr && *pstr; pstr++)
      n_preargs++;
   
   n_job_args = atoi(get_conf_val("njob_args"));
   
   if (!(n_preargs + n_job_args))
      return NULL;
   
   args = malloc((n_preargs + n_job_args + extra_args + 1)*sizeof(char *));
   if (!args)
      return NULL;
   
   for (i = 0; i < n_preargs; i++)
      args[i] = strdup(preargs[i]);

   args[i] = NULL;

   for (i = 0; i < n_job_args; i++) {
      sprintf(conf_val, "job_arg%lu", i + 1);
      cp = get_conf_val(conf_val);
     
      if(cp != NULL) {
         args[i + n_preargs] = strdup(cp);
      } else {
         args[i + n_preargs] = "";
      }
   }
   args[i + n_preargs] = NULL;

   return args;
}


/*--------------------------------------------------------------------
 * set_shepherd_signal_mask
 * set signal mask that shpher can handle signals from execd
 *--------------------------------------------------------------------*/
void start_command(
char *shell_path,
char *script_file,
char *argv0,
char *shell_start_mode,
int is_interactive,
int is_qlogin,
int is_rsh,
int is_rlogin,
char *str_title 
) {
   char **args;
   char **pstr;
   char *pc;
   char *pre_args[10];
   char **pre_args_ptr;
   char err_str[2048];

   pre_args_ptr = &pre_args[0];

   /*
   ** prepare job arguments
   */
   if (!strcasecmp("script_from_stdin", shell_start_mode)) {
      /*
      ** -s makes it possible to make the shell read from stdin
      ** and yet have job arguments
      ** problem: if arguments are options that shell knows it
      ** will interpret and eat them
      */
      pre_args_ptr[0] = argv0;
      pre_args_ptr[1] = "-s";
      pre_args_ptr[2] = NULL;
      args = read_job_args(pre_args, 0);
   } 
   else if (!strcasecmp("posix_compliant", shell_start_mode)) {
      pre_args_ptr[0] = argv0;
      pre_args_ptr[1] = script_file;
      pre_args_ptr[2] = NULL;
      args = read_job_args(pre_args, 0);
   } 
   else if (!strcasecmp("start_as_command", shell_start_mode)) {
      pre_args_ptr[0] = argv0;
      sprintf(err_str, "start_as_command: pre_args_ptr[0] = argv0; \"%s\" shell_path = \"%s\"", argv0, shell_path); 
      shepherd_trace(err_str);
      pre_args_ptr[1] = "-c";
      pre_args_ptr[2] = script_file;
      pre_args_ptr[3] = NULL;
      args = pre_args;
   } 
   else if (is_interactive) {
      int njob_args;
      pre_args_ptr[0] = script_file;
      pre_args_ptr[1] = "-display";
      pre_args_ptr[2] = get_conf_val("display");
      pre_args_ptr[3] = "-n";
      pre_args_ptr[4] = str_title;
      pre_args_ptr[5] = NULL;   
      args = read_job_args(pre_args, 2);
      njob_args = atoi(get_conf_val("njob_args"));
      args[njob_args + 5] = "-e";
      args[njob_args + 6] = get_conf_val("shell_path");
      args[njob_args + 7] = NULL;
   }
   else if (is_qlogin) {
      pre_args_ptr[0] = script_file;
      if(is_rsh) {
         pre_args_ptr[1] = get_conf_val("rsh_daemon");
      } else {
         if(is_rlogin) {
            pre_args_ptr[1] = get_conf_val("rlogin_daemon");
         } else {
            pre_args_ptr[1] = get_conf_val("qlogin_daemon");
         }
      }
      pre_args_ptr[2] = "-d"; 
      pre_args_ptr[3] = NULL;
      args = read_job_args(pre_args, 0);
   }
   else {
      /*
      ** unix_behaviour/raw_exec
      */
      pre_args_ptr[0] = script_file;
      pre_args_ptr[1] = NULL;   
      args = read_job_args(pre_args, 0);
   }

   if(is_qlogin) {
      /* build trace string */
      sprintf(err_str, "calling qlogin_starter(%s, %s);", shepherd_job_dir, args[1]);
      shepherd_trace(err_str);
      qlogin_starter(shepherd_job_dir, args[1]);
   } else {
      /* build trace string */
      pc = err_str;
      sprintf(pc, "execvp(%s,", (pre_args_ptr[0] == argv0) ? shell_path : script_file);
      pc += strlen(pc);
      for (pstr = args; pstr && *pstr; pstr++) {
      
         /* calculate rest length in string - 15 is just lazyness for blanks, "..." string, etc. */
         if (strlen(*pstr) < ((sizeof(err_str)  - (pc - err_str) - 15))) {
            sprintf(pc, " %s", *pstr);
            pc += strlen(pc);
         }
         else {
            sprintf(pc, " ...");
            pc += strlen(pc);
            break;
         }      
      }

      sprintf(pc, ")");
      shepherd_trace(err_str);

      execvp((pre_args_ptr[0] == argv0) ? shell_path : script_file, args);

      /* Aaaah - execvp() failed */
      {
         char failed_str[2048+128];
         sprintf(failed_str, "%s failed: %s", err_str, strerror(errno));

         /* most of the problems here are related to the shell
            i.e. -S /etc/passwd */
         shepherd_state = SSTATE_NO_SHELL;
         /* EXIT HERE IN CASE IF FAILURE */
         shepherd_error(failed_str);
      }
   }
}

static char *build_path(
int type 
) {
   SGE_STRUCT_STAT statbuf;
   char *path, *base;
   char *postfix, *name, *job_id, *job_name, *ja_task_id;
   int pathlen;
   char err_str[SGE_PATH_MAX+128];

   switch (type) {
   case SGE_PAR_STDOUT:
      name = "pe_stdout_path";
      postfix = "po";
      break;
   case SGE_PAR_STDERR:
      name = "pe_stderr_path";
      postfix = "pe";
      break;
   case SGE_STDOUT:
      name = "stdout_path";
      postfix = "o";
      break;
   case SGE_STDERR:
      name = "stderr_path";
      postfix = "e";
      break;
   default:
      return NULL;
   }

   base = get_conf_val(name);
   if (SGE_STAT(base, &statbuf)) {
      if (errno != ENOENT) {
         char *t;
         sprintf(err_str, "can't stat() \"%s\" as %s: %s",
            base, name, strerror(errno));
         sprintf(err_str+strlen(err_str), " KRB5CCNAME=%s uid="uid_t_fmt" gid="uid_t_fmt" ",
                 (t=getenv("KRB5CCNAME"))?t:"none", getuid(), getgid());
         {
            gid_t groups[10];
            int i, ngid;
            ngid = getgroups(10, groups);
            for(i=0; i<ngid; i++)
               sprintf(err_str+strlen(err_str), uid_t_fmt" ", groups[i]);
         }
         shepherd_state = SSTATE_OPEN_OUTPUT; /* job's failure */
         shepherd_error(err_str);
      }
      return base; /* does not exist - must be path of file to be created */
   }

   if (!(S_ISDIR(statbuf.st_mode)))
      return base;

   job_name = get_conf_val("job_name");
   job_id = get_conf_val("job_id");
   ja_task_id = get_conf_val("ja_task_id");
   if (!(path = (char *)malloc(pathlen=(strlen(base) 
         + strlen(job_name) + strlen(job_id) + strlen(ja_task_id) + 25)))) {
      sprintf(err_str, "malloc(%d) failed for %s@",
         pathlen, name);
      shepherd_error(err_str);
   }

   if (atoi(ja_task_id))
      sprintf(path, "%s/%s.%s%s.%s", base, job_name, postfix, job_id,
         ja_task_id);
   else
      sprintf(path, "%s/%s.%s%s", base, job_name, postfix, job_id);

   return path;
}
