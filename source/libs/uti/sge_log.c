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
#ifndef WIN32NATIVE
#	include <unistd.h>
#	include <sys/time.h>
#	include <time.h>
#  include <stdlib.h>
#  include <errno.h>
#endif /* WIN32NATIVE */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#if defined (SGE_MT)
#include <pthread.h>
#endif

#include "sge.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_dstring.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_uidgid.h"

#include "sge_uti_state.h"

#include "msg_utilib.h"


static void sge_do_log(int log_level, int levelchar, const char *err_str, 
                       const char *newline);

struct log_state_t {
    u_long32         log_level;
    char             log_buffer[4*MAX_STRING_SIZE]; /* formerly known as SGE_EVENT */
    char            *log_file;
    int              log_as_admin_user;
    int              verbose;
    int              gui_log;
    trace_func_type  trace_func;
};

#if defined(SGE_MT)
static pthread_key_t   log_state_key;
#else
static struct log_state_t log_state_opaque = {
   LOG_WARNING, "", TMP_ERR_FILE_SNBU, 0, 1, 1, NULL };
struct log_state_t *log_state = &log_state_opaque;
#endif


#if defined(SGE_MT)
static void log_state_init(struct log_state_t* state) {
   state->log_level      = LOG_WARNING;
   strcpy(state->log_buffer, "");
   state->log_file          = TMP_ERR_FILE_SNBU;
   state->log_as_admin_user = 0;
   state->verbose           = 1;
   state->gui_log           = 1;
   state->trace_func        = NULL;
}

static void log_state_destroy(void* state) {
   free(state);
}
 
static pthread_once_t log_once_control = PTHREAD_ONCE_INIT;
void log_once_init(void) {
   pthread_key_create(&log_state_key, &log_state_destroy);
} 
void log_init_mt(void) {
   pthread_once(&log_once_control, log_once_init);
} 

#endif

/****** uti/log/log_state_get_log_buffer() ******************************************
*  NAME
*     log_state_get_log_buffer() -- Return a buffer that can be used to build logging 
*                                   strings
*
*  SYNOPSIS
*     char *log_state_get_log_buffer(void) 
*
*  FUNCTION
*     Return a buffer that can be used to build logging strings
*
*  RESULT
*     char * 
*
******************************************************************************/
char *log_state_get_log_buffer(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_buffer");
   return log_state->log_buffer;
}

/****** uti/log/log_state_get_log_level() ******************************************
*  NAME
*     log_state_get_log_level() -- Return log level.
*
*  SYNOPSIS
*     u_long32 log_state_get_log_level(void) 
*
*  FUNCTION
*     Return log level
*
*  RESULT
*     u_long32
*
******************************************************************************/
u_long32 log_state_get_log_level(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_level");
   return log_state->log_level;
}

/****** uti/log/log_state_get_log_file() ******************************************
*  NAME
*     log_state_get_log_file() -- Return path to log file in use.
*
*  SYNOPSIS
*     const char *log_state_get_log_file(void) 
*
*  FUNCTION
*     Return path to log file in use.
*
*  RESULT
*     const char * 
*
******************************************************************************/
const char *log_state_get_log_file(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_file");
   return log_state->log_file;
}

/****** uti/log/log_state_get_log_verbose() ******************************************
*  NAME
*     log_state_get_log_verbose() -- Is verbose logging enabled?
*
*  SYNOPSIS
*     int log_state_get_log_verbose(void) 
*
*  FUNCTION
*     Is verbose logging enabled? 
*     With verbose logging enabled not only ERROR/CRITICAL messages are 
*     printed to stderr but also WARNING/INFO.
*
*  RESULT
*     int - 0 or 1 
*
*  SEE ALSO
*     uti/log/log_state_set_log_verbose()
******************************************************************************/
int log_state_get_log_verbose(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_verbose");
   return log_state->verbose;
}

/****** uti/log/log_state_get_log_gui() ******************************************
*  NAME
*     log_state_get_log_gui() -- Is GUI logging enabled?
*
*  SYNOPSIS
*     int log_state_get_log_gui(void) 
*
*  FUNCTION
*     Is GUI logging enabled? 
*     With GUI logging enabled messages are printed to stderr/stdout.  
*
*  RESULT
*     int - 0 or 1 
*
*  SEE ALSO
*     uti/log/log_state_set_log_gui()
******************************************************************************/
int log_state_get_log_gui(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_gui");
   return log_state->gui_log;
}

/****** uti/log/log_state_get_log_trace_func() ******************************************
*  NAME
*     log_state_get_log_trace_func() -- Return additional trace function in use.
*
*  SYNOPSIS
*     trace_func_type log_state_get_log_trace_func(void) 
*
*  FUNCTION
*     Return additional trace function in use.
*     E.g. commd uses it to call a function implementing commdcntl -t message logging.
*     The trace func is needed to unify commd trace() concept with sge_log().
*
*  RESULT
*     trace_func_type 
*
*  SEE ALSO
*     uti/log/log_state_set_log_trace_func()
******************************************************************************/
trace_func_type log_state_get_log_trace_func(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_trace_func");
   return log_state->trace_func;
}

/****** uti/log/log_state_get_log_as_admin_user() ******************************************
*  NAME
*     log_state_get_log_as_admin_user() -- Needs sge_log() to change into admin user?
*
*  SYNOPSIS
*     trace_func_type log_state_get_log_as_admin_user(void) 
*
*  FUNCTION
*     Returns whether logging shall be done as admin user.
*
*  RESULT
*     int
*
*  SEE ALSO
*     uti/log/log_state_set_log_as_admin_user()
******************************************************************************/
int log_state_get_log_as_admin_user(void)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_get_log_as_admin_user");
   return log_state->log_as_admin_user;
}

/****** uti/log/log_state_set_log_level() *****************************************
*  NAME
*     log_state_set_log_level() -- Set log level to be used.
*
*  SYNOPSIS
*     void log_state_set_log_level(int i) 
*
*  FUNCTION
*     Set log level to be used.
*
*  INPUTS
*     u_long32 
*
*  SEE ALSO
*     uti/log/log_state_get_log_level() 
******************************************************************************/
void log_state_set_log_level(u_long32 i)
{ 
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_level");
   log_state->log_level = i;
}

/****** uti/log/log_state_set_log_file() *****************************************
*  NAME
*     log_state_set_log_file() -- Set log file to be used.
*
*  SYNOPSIS
*     void log_state_set_log_file(int i) 
*
*  FUNCTION
*     Set path of log file to be used. 
*
*  INPUTS
*     char * 
*
*  SEE ALSO
*     uti/log/log_state_get_log_file() 
******************************************************************************/
void log_state_set_log_file(char *file)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_file");
   log_state->log_file = file;
}


/****** uti/log/log_state_set_log_verbose() *****************************************
*  NAME
*     log_state_set_log_verbose() -- Enable/disable verbose logging 
*
*  SYNOPSIS
*     void log_state_set_log_verbose(int i) 
*
*  FUNCTION
*     Enable/disable verbose logging 
*
*  INPUTS
*     int i - 0 or 1  
*
*  SEE ALSO
*     uti/log/log_state_get_log_verbose() 
******************************************************************************/
void log_state_set_log_verbose(int i)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_verbose");
   log_state->verbose = i;
}

/****** uti/log/log_state_set_log_gui() ********************************************
*  NAME
*     log_state_set_log_gui() -- Enable/disable logging for GUIs 
*
*  SYNOPSIS
*     void log_state_set_log_gui(int i) 
*
*  FUNCTION
*     Enable/disable logging for GUIs 
*     With GUI logging enabled messages are printed to stderr/stdout.  
*
*  INPUTS
*     int i - 0 or 1 
******************************************************************************/
void log_state_set_log_gui(int i)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_gui");
   log_state->gui_log = i;
}   

/****** uti/log/log_state_set_log_trace_func() ******************************************
*  NAME
*     log_state_set_log_trace_func() -- Install additional trace function to be used.
*
*  SYNOPSIS
*     void log_state_set_log_trace_func(void) 
*
*  FUNCTION
*     Return additional trace function in use.
*     E.g. commd uses it to have sge_log() call a function implementing 
*     commdcntl -t message logging.
*
*  INPUTS
*     trace_func_type
*
*  SEE ALSO
*     uti/log/log_state_get_log_trace_func()
******************************************************************************/
void log_state_set_log_trace_func(trace_func_type func)
{
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_trace_func");
   log_state->trace_func = func;
}


/****** uti/log/log_state_set_log_as_admin_user() *****************************
*  NAME
*     log_state_set_log_as_admin_user() -- Enable/Disable logging as admin user 
*
*  SYNOPSIS
*     void log_state_set_log_as_admin_user(int i)
*
*  FUNCTION
*     This function enables/disables logging as admin user. This 
*     means that the function/macros switches from start user to 
*     admin user before any messages will be written. After that 
*     they will switch back to 'start' user. 
*
*  INPUTS
*     int i - 0 or 1 
*  
*  SEE ALSO
*     uti/uidgid/sge_switch2admin_user
*     uti/uidgid/sge_switch2start_user
******************************************************************************/
void log_state_set_log_as_admin_user(int i) {
   GET_SPECIFIC(struct log_state_t, log_state, log_state_init, log_state_key, "log_state_set_log_as_admin_user");
   log_state->log_as_admin_user = i;
}


#ifndef WIN32NATIVE

/****** sge_log/sge_do_log() ***************************************************
*  NAME
*     sge_do_log() -- SGE logging function
*
*  SYNOPSIS
*     static void sge_do_log(int log_level, int levelchar, const char *err_str, 
*     const char *newline) 
*
*  INPUTS
*     int log_level       - ??? 
*     int levelchar       - ??? 
*     const char *err_str - ??? 
*     const char *newline - ??? 
*
*  NOTES
*     MT-NOTE: sge_do_log() is not MT safe due to sge_switch2admin_user()
*     MT-NOTE: sge_do_log() can be used however in MT applications if no 
*     MT-NOTE: admin user switching takes place
*******************************************************************************/
static void sge_do_log(int log_level, int levelchar, const char *err_str, 
                       const char *newline) 
{
   int fd;
   int switch_back = 0;

   /* LOG_CRIT LOG_ERR LOG_WARNING LOG_NOTICE LOG_INFO LOG_DEBUG */

   if (log_state_get_log_as_admin_user() && geteuid() == 0) {
      sge_switch2admin_user();
      switch_back = 1;
   }

   if ((fd = open(log_state_get_log_file(), O_WRONLY | O_APPEND | O_CREAT, 0666)) >= 0) {
      char msg2log[4*MAX_STRING_SIZE];
      char date[256], tmp_date[256], time_buf[256];
      dstring ds, msg;
      sge_dstring_init(&ds, time_buf, sizeof(time_buf));
      sge_dstring_init(&msg, msg2log, sizeof(msg2log));
      sprintf(tmp_date, "%s", sge_ctime(0, &ds));
      sscanf(tmp_date, "%[^\n]", date);

      sge_dstring_sprintf(&msg, "%s|%s|%s|%c|%s%s",
              date,
              uti_state_get_sge_formal_prog_name(),
              uti_state_get_unqualified_hostname(),
              levelchar,
              err_str,
              newline);
      write(fd, msg2log, strlen(msg2log));
      close(fd);
   }

   if (log_state_get_log_as_admin_user() && switch_back) {
      sge_switch2start_user();
   }

   return;
}
#endif


/****** uti/log/sge_log() *****************************************************
*  NAME
*     sge_log() -- Low level logging function 
*
*  SYNOPSIS
*     int sge_log(int log_level, char *mesg, char *file__, 
*                 char *func__, int line__) 
*
*  FUNCTION
*     Low level logging function. Used by various macros.
*     Do not use this function directly. 
*
*  INPUTS
*     int log_level - Logging Level
*     char *mesg    - Message
*     char *file__  - Filename
*     char *func__  - Function Name
*     int line__    - Line within 'file__'
*
*  RESULT
*     int - 0
*
*  SEE ALSO
*     uti/log/CRITICAL
*     uti/log/ERROR
*     uti/log/WARNING
*     uti/log/NOTICE
*     uti/log/INFO
*     uti/log/DEBUG
*
*  NOTES
*     MT-NOTE: sge_log() is not MT safe due to sge_switch2admin_user()
*     MT-NOTE: sge_log() can be used in clients where no admin user switching 
*     MT-NOTE: takes place
*     MT-NOTE: sge_log() is not MT safe due rmon_condition()
*     MT-NOTE: sge_log() can be used if DENTER_MAIN() is called only by one 
*     MT-NOTE: thread
******************************************************************************/
int sge_log(int log_level, const char *mesg, const char *file__, 
            const char *func__, int line__) 
{
   char buf[128*4];
   char newline[2*4];
   int levelchar;
   char levelstring[32*4];
   trace_func_type t;

   /* Make sure to have at least a one byte logging string */
   if (!mesg || mesg[0] == '\0') {
      sprintf(buf, MSG_LOG_CALLEDLOGGINGSTRING_S, 
              mesg ? MSG_LOG_ZEROLENGTH : MSG_POINTER_NULL);
      mesg = buf;
   }
   if (mesg[strlen(mesg)-1] != '\n') {
      strcpy(newline,"\n");
   } else {
      strcpy(newline, "\0");
   }

   DPRINTF(("%s %d %s%s", file__, line__, mesg, newline));

   /* 
    * commd remote monitoring is in effect independently 
    * of the current log_level 
    */
   t = log_state_get_log_trace_func();
   if (t)
      t(mesg);

   /* quick exit if nothing to log */
#ifndef WIN32NATIVE
   if (log_level > MAX(log_state_get_log_level(), LOG_WARNING)) {
      return 0;
   }
#else /* WIN32NATIVE */
   if ((u_long32)log_level > MAX(log_state_get_log_level(), LOG_WARNING)) {
	   return 0;
   }
#endif /* WIN32NATIVE */

   if (!log_state_get_log_gui()) {
      return 0;
   }

   switch(log_level) {
      case LOG_CRIT:
         strcpy(levelstring, MSG_LOG_CRITICALERROR);
         levelchar = 'C';
         break;
      case LOG_ERR:
         strcpy(levelstring, MSG_LOG_ERROR);
         levelchar = 'E';
         break;
      case LOG_WARNING:
         strcpy(levelstring, "");
         levelchar = 'W';
         break;
      case LOG_NOTICE:
         strcpy(levelstring, "");
         levelchar = 'N';
         break;
      case LOG_INFO:
         strcpy(levelstring, "");
         levelchar = 'I';
         break;
      case LOG_DEBUG:
         strcpy(levelstring, "");
         levelchar = 'D';
         break;
      default:
         strcpy(levelstring, "");
         levelchar = 'L';
         break;
   }

#ifndef WIN32NATIVE
   /* avoid double output in debug mode */
   if (!uti_state_get_daemonized() && !rmon_condition(LAYER, INFOPRINT) && 
       (log_state_get_log_verbose() || log_level == LOG_ERR || log_level == LOG_CRIT)) {
      fprintf(stderr, "%s%s%s", levelstring, mesg, newline);
   } 
   if (uti_state_get_mewho() == QMASTER || uti_state_get_mewho() == EXECD   || uti_state_get_mewho() == QSTD ||
       uti_state_get_mewho() == SCHEDD ||  uti_state_get_mewho() == SHADOWD || uti_state_get_mewho() == COMMD) {
      sge_do_log(log_level, levelchar, mesg, newline);
   }
#endif /* WIN32NATIVE */

   return 0;
}

