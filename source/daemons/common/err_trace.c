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
/* Error and trace handling for shepherd 
 * Error/Trace files will go to the actual directory at first time 
 * shepherd_error() and trace() are called 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>

#include "basis_types.h"
#include "err_trace.h"
#include "sge_time.h"
#include "sge_uidgid.h"
#include "config_file.h"
#include "qlogin_starter.h"
#include "sge_unistd.h"
#include "sge_dstring.h"

static int sh_str2file(char *header_str, const char *str, char *file);

extern int shepherd_state;  /* holds exit status for shepherd_error() */
int foreground = 1;           /* usability of stderr/out */
extern pid_t coshepherd_pid;

static int shep_log_as_admin_user = 0;
void shepherd_log_as_admin_user(void);

/*-----------------------------------------------------------------*/
void shepherd_log_as_admin_user(void)
{
   shep_log_as_admin_user = 1;
}


/*-----------------------------------------------------------------*/

void shepherd_error_sprintf(char *format, ...)
{
   dstring message = DSTRING_INIT;
   va_list ap;

   va_start(ap, format);
   if (format) {
      sge_dstring_vsprintf(&message, format, ap);
      shepherd_error_impl((char*)sge_dstring_get_string(&message), 1);
      sge_dstring_free(&message);
   }
}


void shepherd_error(char *str) 
{
   shepherd_error_impl(str, 1);
}

void shepherd_error_impl(char *str, int do_exit) 
{
   char header_str[256];
     
   sprintf(header_str, "%s ["uid_t_fmt":"pid_t_fmt"]: ", sge_ctime(0), geteuid(), getpid());
         
   sh_str2file(header_str, str, "error");

   if (foreground)
      fprintf(stderr, "%s%s\n", header_str, str);

   if (shep_log_as_admin_user && geteuid() == 0)
      sge_switch2admin_user();

   sprintf(header_str, "%d", shepherd_state);
   sh_str2file(header_str, NULL, "exit_status");

   if (coshepherd_pid > 0) {
      sge_switch2start_user();
      kill(coshepherd_pid, SIGTERM);
      sge_switch2admin_user();
   }   
     
   if(search_conf_val("qrsh_control_port") != NULL) {
      char buffer[1024];
      strcpy(buffer, "1:");
      strncat(buffer, str, 1021);
      write_to_qrsh(buffer);  
   }
     
   if (do_exit)
      exit(shepherd_state);
   return;
}


int shepherd_trace_sprintf(const char *format, ...)
{
   int ret = 1;
   dstring message = DSTRING_INIT;
   va_list ap;

   va_start(ap, format);
   if (format) {
      sge_dstring_vsprintf(&message, format, ap);
      ret = shepherd_trace(sge_dstring_get_string(&message));
      sge_dstring_free(&message);
   }
   return ret;
}

/*-----------------------------------------------------------------*/
int shepherd_trace(const char *str) 
{
   int ret, switch_back = 0;
   char header_str[256];

   sprintf(header_str, "%s ["uid_t_fmt":"pid_t_fmt"]: ", sge_ctime(0), geteuid(), getpid());

   if (shep_log_as_admin_user && geteuid() == 0) {
      sge_switch2admin_user();
      switch_back = 1;
   }

   ret = sh_str2file(header_str, str, "trace");

   if (foreground) {
      printf("%s%s\n", header_str, str);
      fflush(stdout);
   }

   if (shep_log_as_admin_user && switch_back)
      sge_switch2start_user();

   return ret;
}


/*-----------------------------------------------------------------*/
static int sh_str2file(char *header_str, const char *str, char *file) 
{
   static char path[SGE_PATH_MAX], tmppath[SGE_PATH_MAX];
   static int called = 0;
   FILE *fp;
   int old_umask;

   /* 
    *  after changing into the jobs cwd we need an 
    *  absolute path to the error/trace file 
    */
   if (!called) { 
      getcwd(path, sizeof(path)); 
      called = 1;
   }

   sprintf(tmppath, "%s/%s", path, file);
   
   old_umask = umask(0);
   fp = fopen(tmppath, "a");
   umask(old_umask);
   if (!fp) {
      FILE *panic_fp;
      char panic_file[255];
      sprintf(panic_file, "/tmp/shepherd."pid_t_fmt, getpid());
      panic_fp = fopen(panic_file, "a");
      if (panic_fp) {
         fprintf(panic_fp, "%s ["uid_t_fmt":"pid_t_fmt"]: "
                 "PANIC: can't open %s file \"%s\": %s\n",
                 sge_ctime(0), geteuid(), getpid(), file, tmppath, strerror(errno));
         fclose(panic_fp);
      }
      return 1;
   }
   
   if (!str && !header_str)
      fprintf(fp, "function sh_str2file() called with NULL arguments\n");
   else if (!header_str && str)
      fprintf(fp, "%s\n", str);
   else if (header_str && !str)
      fprintf(fp, "%s\n", header_str);
   else
      fprintf(fp, "%s%s\n", header_str, str);

   fclose(fp);   

   return 0;
}


/********************************************/
/* reown files so that user can write to it */
/********************************************/
void err_trace_chown_files(
int uid 
) {
   int fd;
   SGE_STRUCT_STAT statbuf;
   int old_umask;
   char err_str[1024];

   old_umask = umask(0);
   if (SGE_STAT("error", &statbuf)) {
      if ((fd=creat("error", 0666))>=0)
         close(fd);
      else
         SHEPHERD_ERROR((err_str, "can't create \"error\" file: %s\n", strerror(errno)));
   }

   if (SGE_STAT("trace", &statbuf)) {
      if ((fd=creat("trace", 0666))>=0)
         close(fd);
      else
         SHEPHERD_ERROR((err_str, "can't create \"trace\" file: %s\n", strerror(errno)));
   }

   if (SGE_STAT("exit_status", &statbuf)) {
      if ((fd=creat("exit_status", 0666))>=0)
         close(fd);
      else
         SHEPHERD_ERROR((err_str, "can't create \"exit_status\" file: %s\n", strerror(errno)));
   }
   umask(old_umask);
}

/*********************************************/
/* return number entries in exit status file */
/*********************************************/
int count_exit_status(void)
{
   int n = 0;
   SGE_STRUCT_STAT sbuf;
   char buf[1024];

   if (!SGE_STAT("exit_status", &sbuf) && sbuf.st_size) {
      FILE *fp;
      int dummy;

      if ((fp = fopen("exit_status", "r"))) {
         while (fgets(buf, sizeof(buf), fp)) 
            if (sscanf(buf, "%d\n", &dummy)==1)
               n++;
         fclose(fp);
      } 
   }

   return n;
}
