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
#endif /* WIN32NATIVE */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "sge.h"
#include "sge_log.h"
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_uidgid.h"

#include "msg_utilib.h"

static void sge_do_log(int log_level, int levelchar, char *err_str, 
                       char *newline);

u_long32 logginglevel = LOG_WARNING;

stringTlong SGE_EVENT;
char *error_file = TMP_ERR_FILE_SNBU;
static int log_as_admin_user = 0;
static int verbose = 1;
static int qmon_log = 0;

/* to unify commd trace() concept with sge_log() */
trace_func_type trace_func = NULL;

#ifndef WIN32NATIVE

static void sge_do_log(int log_level, int levelchar, char *err_str, 
                       char *newline) 
{
   int fd;
   char msg2log[1024], date[256], tmp_date[256];
   time_t now;
   char *tmp_ctime;
   int switch_back = 0;

   /* LOG_CRIT LOG_ERR LOG_WARNING LOG_NOTICE LOG_INFO LOG_DEBUG */

   if (log_as_admin_user && geteuid() == 0) {
      sge_switch2admin_user();
      switch_back = 1;
   }

   if ((fd = open(error_file, O_WRONLY | O_APPEND | O_CREAT, 0666)) >= 0) {
      now = time((time_t *) NULL);
      tmp_ctime = ctime(&now);
      sprintf(tmp_date, "%s", tmp_ctime);
      sscanf(tmp_date, "%[^\n]", date);

      sprintf(msg2log, "%s|%s|%s|%c|%s%s",
              date,
              prognames[me.who],
              me.unqualified_hostname,
              levelchar,
              err_str,
              newline);
      write(fd, msg2log, strlen(msg2log));
      close(fd);
   }

   if (log_as_admin_user && switch_back) {
      sge_switch2start_user();
   }

   return;
}

#endif

/****** uti/log/sge_log_set_qmon() ********************************************
*  NAME
*     sge_log_set_qmon() -- Enable/disable logging for GUIs 
*
*  SYNOPSIS
*     void sge_log_set_qmon(int i) 
*
*  FUNCTION
*     Enable/disable logging for GUIs 
*
*  INPUTS
*     int i - 0 or 1 
******************************************************************************/
void sge_log_set_qmon(int i)
{
   qmon_log = i;
}   

/****** uti/log/sge_log_set_verbose() *****************************************
*  NAME
*     sge_log_set_verbose() -- Enable/disable verbose logging 
*
*  SYNOPSIS
*     void sge_log_set_verbose(int i) 
*
*  FUNCTION
*     Enable/disable verbose logging 
*
*  INPUTS
*     int i - 0 or 1  
*
*  SEE ALSO
*     uti/log/sge_log_is_verbose() 
******************************************************************************/
void sge_log_set_verbose(int i) 
{
   verbose = i;
}

/****** uti/log/sge_log_is_verbose() ******************************************
*  NAME
*     sge_log_is_verbose() -- Is verbose logging enabled?
*
*  SYNOPSIS
*     int sge_log_is_verbose(void) 
*
*  FUNCTION
*     Is verbose logging enabled? 
*
*  RESULT
*     int - 0 or 1 
*
*  SEE ALSO
*     uti/log/sge_log_set_verbose()
******************************************************************************/
int sge_log_is_verbose(void) 
{
   return verbose;
}

/****** uti/log/sge_log_set_auser() *******************************************
*  NAME
*     sge_log_set_auser() -- Enable/Disable logging as admin user 
*
*  SYNOPSIS
*     void sge_log_set_auser(int i)
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
void sge_log_set_auser(int i) {
   log_as_admin_user = i;
}

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
******************************************************************************/
int sge_log(int log_level, char *mesg, char *file__, 
            char *func__, int line__) 
{
   static char buf[128*4];
   char newline[2*4];
   int levelchar;
   char levelstring[32*4];

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
    * of the current logginglevel 
    */
   if (me.who == COMMD && trace_func && log_level >= LOG_DEBUG) {
      trace_func(mesg);
   }

   /* quick exit if nothing to log */
#ifndef WIN32NATIVE
   if (log_level > MAX(logginglevel, LOG_WARNING)) {
      return 0;
   }
#else /* WIN32NATIVE */
   if ((u_long32)log_level > MAX(logginglevel, LOG_WARNING)) {
	   return 0;
   }
#endif /* WIN32NATIVE */

   if (me.who == QMON && !qmon_log) {
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
   if (!me.daemonized && !rmon_condition(LAYER, INFOPRINT) && 
       (verbose || log_level == LOG_ERR || log_level == LOG_CRIT)) {
      fprintf(stderr, "%s%s%s", levelstring, mesg, newline);
   } 
   if (me.who == QMASTER || me.who == EXECD   || me.who == QSTD ||
       me.who == SCHEDD ||  me.who == SHADOWD || me.who == COMMD) {
      sge_do_log(log_level, levelchar, mesg, newline);
   }
#endif /* WIN32NATIVE */

   return 0;
}

