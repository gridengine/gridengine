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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef WIN32NATIVE
#	include <grp.h>
#endif 

#include <sys/types.h>

#ifndef WIN32NATIVE
#	include <sys/time.h>
#	include <sys/resource.h>
#	include <sys/wait.h>
#	include <unistd.h>
#endif 

#include "def.h"
#include "symbols.h"
#include "sge_gdi_intern.h"
#include "utility.h"
#include "usage.h"
#include "commlib.h"
#include "sge_jobL.h"
#include "sge_queueL.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "get_path.h"
#include "msg_gdilib.h"

#ifdef WIN32 /* gettimeofday prototype */
int gettimeofday(struct timeval *tz, struct timezone *tzp);
#endif

/************************************************************************/
void sge_show_hold_list(hold, how)
int how;
int hold;

{

   int i = 0;

   DENTER(TOP_LAYER, "sge_show_hold_list");
   if (VALID(SGE_STDOUT, how)) {
      if (VALID(USER, hold)) {
         printf("%c", USER_SYM);
         i++;
      }
      if (VALID(SYSTEM, hold)) {
         printf("%c", SYSTEM_SYM);
         i++;
      }
      if (VALID(OTHER, hold)) {
         printf("%c", OTHER_SYM);
         i++;
      }
      if (VALID(NO_HOLD, hold)) {
         printf("%c", NO_HOLD_SYM);
         i++;
      }
      while (i < 4) {
         printf(" ");
         i++;
      }
      fflush(stdout);
   }

   if (VALID(SGE_STDERR, how)) {
      if (VALID(USER, hold)) {
         fprintf(stderr, "%c", USER_SYM);
         i++;
      }
      if (VALID(SYSTEM, hold)) {
         fprintf(stderr, "%c", SYSTEM_SYM);
         i++;
      }
      if (VALID(OTHER, hold)) {
         fprintf(stderr, "%c", OTHER_SYM);
         i++;
      }
      if (VALID(NO_HOLD, hold)) {
         fprintf(stderr, "%c", NO_HOLD_SYM);
         i++;
      }
      while (i < 4) {
         fprintf(stderr, " ");
         i++;
      }
      fflush(stderr);
   }

   DEXIT;
   return;
}

#if 0
/**************************************************************************/
int sge_is_int(
char *str 

) {

   DENTER(TOP_LAYER, "sge_is_int");;

   SGE_ASSERT((str));

   while (str[0]) {
      if (isalpha((int) str[0])) {
         DEXIT;
         return (-1);
      }
      str++;
   }

   DEXIT;
   return (0);

}


/**************************************************************************/
int sge_atoi(
char *int_str 

) {

   int i;

   DENTER(BASIS_LAYER, "sge_atoi");;

   if (!int_str) {
      CRITICAL((SGE_EVENT, MSG_GDI_STRING_STRINGTOINTFAILED_S, int_str));

#ifndef WIN32NATIVE      
	  sge_usage(stderr);
#endif 

      SGE_EXIT(1);
   }

   i = atol(int_str);

   DEXIT;
   return (i);

}

#endif

/**************************************************************************/
char *sge_malloc(
int size 

) {

   char *cp = NULL;

   DENTER(BASIS_LAYER, "sge_malloc");

   if (!size) {
      DEXIT;
      return (NULL);
   }

   cp = (char *) malloc(size);
   if (!cp) {
      CRITICAL((SGE_EVENT, MSG_GDI_MEMORY_MALLOCFAILED ));
      DEXIT;
      abort();
   }

   memset(cp, 0, size);

   DEXIT;
   return (cp);
}

/**************************************************************************/
char *sge_realloc(
char *ptr,
int size 

) {

   char *cp = NULL;

   DENTER(BASIS_LAYER, "sge_realloc");

   if (!size) {
      if (ptr) {
         FREE(ptr);
      }
      DEXIT;
      return (NULL);
   }


   cp = (char *) realloc(ptr, size);

   if (!cp) {
      CRITICAL((SGE_EVENT, MSG_GDI_MEMORY_REALLOCFAILED ));
      DEXIT;
      abort();
   }

   DEXIT;
   return (cp);

}

/************************************************************************/
char *sge_free(
char *cp 
) {
   FREE(cp);
   return (NULL);
}

/************************************************************************/
void sge_show_checkpoint(
int how,
int op 
) {
   int i = 0;
   int count = 0;
   stringT tmp_str;

   DENTER(TOP_LAYER, "sge_show_checkpoint");

   memset(tmp_str, 0, sizeof(tmp_str));

   if (VALID(CHECKPOINT_AT_MINIMUM_INTERVAL, op)) {
      tmp_str[count] = CHECKPOINT_AT_MINIMUM_INTERVAL_SYM;
      count++;
   }

   if (VALID(CHECKPOINT_AT_SHUTDOWN, op)) {
      tmp_str[count] = CHECKPOINT_AT_SHUTDOWN_SYM;
      count++;
   }

   if (VALID(CHECKPOINT_SUSPEND, op)) {
      tmp_str[count] = CHECKPOINT_SUSPEND_SYM;
      count++;
   }

   if (VALID(NO_CHECKPOINT, op)) {
      tmp_str[count] = NO_CHECKPOINT_SYM;
      count++;
   }

   if (VALID(SGE_STDOUT, how)) {
      printf("%s", tmp_str);
      for (i = count; i < 4; i++)
         printf(" ");
   }

   if (VALID(SGE_STDERR, how)) {
      fprintf(stderr, "%s", tmp_str);
      for (i = count; i < 4; i++)
         fprintf(stderr, " ");
   }

   DEXIT;

   return;
}

/************************************************************************/
void sge_show_y_n(
int op,
int how 

) {

   stringT tmp_str;

   DENTER(TOP_LAYER, "sge_show_y_n");

   if (op)
      sprintf(tmp_str, "y");
   else
      sprintf(tmp_str, "n");

   if (VALID(how, SGE_STDOUT))
      printf("%s", tmp_str);

   if (VALID(how, SGE_STDERR))
      fprintf(stderr, "%s", tmp_str);

   DEXIT;

   return;

}

#if 0
/************************************************************************/
void sge_show_keep_list(
int op,
int how 

) {

   int i = 0;
   int count = 0;
   stringT tmp_str;

   DENTER(TOP_LAYER, "sge_show_keep_list");

   if (VALID(KEEP_NONE, op)) {
      tmp_str[count] = KEEP_NONE_SYM;
      count++;
   }

   if (VALID(KEEP_STD_ERROR, op)) {
      tmp_str[count] = KEEP_STD_ERROR_SYM;
      count++;
   }

   if (VALID(KEEP_STD_OUTPUT, op)) {
      tmp_str[count] = KEEP_STD_OUTPUT_SYM;
      count++;
   }

   if (VALID(SGE_STDOUT, how)) {
      printf("%s", tmp_str);
      for (i = count; i < 2; i++)
         printf(" ");
   }

   if (VALID(SGE_STDERR, how)) {
      fprintf(stderr, "%s", tmp_str);
      for (i = count; i < 2; i++)
         fprintf(stderr, " ");
   }

   DEXIT;

   return;

}
#endif

/************************************************************************/
void sge_show_mail_options(
int op,
int how 

) {

   int i = 0;
   int count = 0;
   stringT tmp_str;

   DENTER(TOP_LAYER, "sge_show_mail_list");

   if (VALID(MAIL_AT_ABORT, op)) {
      tmp_str[count] = MAIL_AT_ABORT_SYM;
      count++;
   }

   if (VALID(MAIL_AT_BEGINNING, op)) {
      tmp_str[count] = MAIL_AT_BEGINNING_SYM;
      count++;
   }

   if (VALID(MAIL_AT_EXIT, op)) {
      tmp_str[count] = MAIL_AT_EXIT_SYM;
      count++;
   }

   if (VALID(NO_MAIL, op)) {
      tmp_str[count] = NO_MAIL_SYM;
      count++;
   }

   if (VALID(MAIL_AT_SUSPENSION, op)) {
      tmp_str[count] = MAIL_AT_SUSPENSION_SYM;
      count++;
   }

   tmp_str[count] = '\0';       /* ensure string terminator */

   if (VALID(SGE_STDOUT, how)) {
      printf("%s", tmp_str);
      for (i = count; i < 4; i++)
         printf(" ");

   }

   if (VALID(SGE_STDERR, how)) {
      fprintf(stderr, "%s", tmp_str);
      for (i = count; i < 4; i++)
         fprintf(stderr, " ");

   }

   DEXIT;

   return;

}

/************************************************************************/
void sge_get_states(
int nm,
char *str,
u_long32 op 
) {
   int count = 0;

   DENTER(TOP_LAYER, "sge_get_states");

   if (nm==QU_qname) {
      if (VALID(QALARM, op))
         str[count++] = ALARM_SYM;
      if (VALID(QSUSPEND_ALARM, op))
         str[count++] = SUSPEND_ALARM_SYM;
      if (VALID(QCAL_SUSPENDED, op))
         str[count++] = SUSPENDED_ON_CALENDAR_SYM;
      if (VALID(QCAL_DISABLED, op))
         str[count++] = DISABLED_ON_CALENDAR_SYM;
      if (VALID(QDISABLED, op))
         str[count++] = DISABLED_SYM;
      if (!VALID(!QDISABLED, op))
         str[count++] = ENABLED_SYM;
      if (VALID(QUNKNOWN, op))
         str[count++] = UNKNOWN_SYM;
      if (VALID(QERROR, op)) 
         str[count++] = ERROR_SYM;
      if (VALID(QSUSPENDED_ON_SUBORDINATE, op))
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }

   if (nm==JB_job_number) {
      if (VALID(JDELETED, op)) 
         str[count++] = DISABLED_SYM;
      if (VALID(JERROR, op)) 
         str[count++] = ERROR_SYM;
      if (VALID(JSUSPENDED_ON_SUBORDINATE, op)) 
         str[count++] = SUSPENDED_ON_SUBORDINATE_SYM;
   }

   if (VALID(JSUSPENDED_ON_THRESHOLD, op)) {
      str[count++] = SUSPENDED_ON_THRESHOLD_SYM;
   }

   if (VALID(JHELD, op)) {
      str[count++] = HELD_SYM;
   }

   if (VALID(JMIGRATING, op)) {
      str[count++] = RESTARTING_SYM;
   }

   if (VALID(JQUEUED, op)) {
      str[count++] = QUEUED_SYM;
   }

   if (VALID(JRUNNING, op)) {
      str[count++] = RUNNING_SYM;
   }

   if (VALID(JSUSPENDED, op)) {
      str[count++] = SUSPENDED_SYM;
   }

   if (VALID(JTRANSITING, op)) {
      str[count++] = TRANSISTING_SYM;
   }

   if (VALID(JWAITING, op)) {
      str[count++] = WAITING_SYM;
   }

   if (VALID(JEXITING, op)) {
      str[count++] = EXITING_SYM;
   }

   str[count++] = '\0';

   DEXIT;
   return;
}

/************************************************************************/
void sge_show_states(
int nm,
u_long32 how,
u_long32 states 
) {

   stringT tmp_str;

   DENTER(TOP_LAYER, "sge_show_states");

   sge_get_states(nm, tmp_str, states);

   while (strlen(tmp_str) < 6)
      strcat(tmp_str, " ");

   if (VALID(SGE_STDOUT, how)) {
      printf("%s", tmp_str);
   }

   if (VALID(SGE_STDERR, how)) {
      fprintf(stderr, "%s", tmp_str);
   }

   DEXIT;
   return;
}

#if 0
/************************************************************************/
#ifndef WIN32NATIVE
unsigned int sge_rand(
int min,
int max 

) {

   static int first_pass = 1;
   unsigned int i, j;
   struct timeval now;
#ifndef SINIX
   struct timezone tzp;
#endif

   DENTER(TOP_LAYER, "sge_rand");

   if (first_pass) {
#ifdef SINIX
      gettimeofday(&now);
#else
      gettimeofday(&now, &tzp);
#endif
      srand((u_long) now.tv_usec);
      first_pass = 0;
   }

   i = rand();
   i = i % max;

   if (i < (unsigned int) min) {
      j = rand();
      j = j % min;
      i = min + j;
   }

   DEXIT;
   return (i);

}
#endif
#endif

/*****************************************************************/
int sge_unlink(
char *prefix,
char *suffix 

) {
   int status;
   stringT str;

   DENTER(TOP_LAYER, "sge_unlink");

   if (!suffix) {
      ERROR((SGE_EVENT, MSG_GDI_POINTER_SUFFIXISNULLINSGEUNLINK ));
      DEXIT;
      return -1;
   }

   if (prefix)
      sprintf(str, "%s/%s", prefix, suffix);
   else
      sprintf(str, "%s", suffix);

   DPRINTF(("file to unlink: \"%s\"\n", str));
   status = unlink(str);

   if (status) {
      DEXIT;
      return -1;
   }
   else {
      DEXIT;
      return 0;
   }   
}

/************************************************************************/
char *sge_getenv(
char *env_str 

) {

   char *cp=NULL;

   DENTER(BASIS_LAYER, "sge_getenv");

   cp = (char *) getenv(env_str);
   if (!cp) {
      DEXIT;
      return (cp);
   }

   if (strlen(cp) >= MAX_STRING_SIZE) {
      CRITICAL((SGE_EVENT, MSG_GDI_STRING_LENGTHEXCEEDSMAXSTRINGSIZE_SI, env_str, (int) MAX_STRING_SIZE));
      DCLOSE;
      abort();
   }

   DEXIT;
   return (cp);

}

/*******************************************************************************************/
void sge_sleep(
int sec,
int usec 

) {

   static struct timeval timeout;

   timeout.tv_sec = sec;
   timeout.tv_usec = usec;

#if !(defined(HPUX) || defined(HP10_01) || defined(HPCONVEX))
   select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout);
#else
   select(0, (int *) 0, (int *) 0, (int *) 0, &timeout);
#endif
}




/***************************************************
 Verify the applicability of a file name.
 We dont like:
 - names longer than 256 chars including '\0'
 - blanks or other ugly chars

 We like digits, chars and '_'.

 returning 0 means OK.
 ***************************************************/
int verify_filename(
char *fname 
) {
   int i=0;

   /* dont allow "." ".." and "../tralla" */
   if (*fname == '.') {
      fname++;
      if (!*fname || (*fname == '.' && ((!*(fname+1)) || (!*fname+1 == '/'))))
         return 1;
   }
   while (*fname && i++<256) {
      if (!isalnum((int) *fname) && !(*fname=='_') && !(*fname=='.'))
         return 1;
      fname++;
   }
   if (i>=256)
      return 1;

   return 0;
}


/*-------------------------------------------------------------------------*
 * check_isalive 
 *    check if master is registered and alive                 
 *    calls is_commd_alive() and ask_commproc()
 * returns
 *    0                    if qmaster is enrolled at sge_commd
 *    > 0                  commlib error number (always positve)
 *    CL_FIRST_FREE_EC+1   can't get masterhost
 *    CL_FIRST_FREE_EC+2   can't connect to commd
 *-------------------------------------------------------------------------*/
int check_isalive(
char *masterhost 
) {
   int alive = 0;

   DENTER(TOP_LAYER, "check_isalive");

   if (!masterhost) {
      DPRINTF(("can't get masterhost\n"));
      DEXIT;
      return CL_FIRST_FREE_EC+1;
   }

   /* check if prog is alive */
   if (me.who == QMON) {
      if (!is_commd_alive()) {
         DPRINTF(("can't connect to commd\n"));
         DEXIT;
         return CL_FIRST_FREE_EC+2;
      }
   } 

   alive = ask_commproc(masterhost, prognames[QMASTER], 1);

   if (alive) {
      DPRINTF(("no qmaster: ask_commproc(\"%s\", \"%s\", %d): %s\n",
               masterhost, prognames[QMASTER], 1, cl_errstr(alive)));
   }

   DEXIT;
   return alive;
}
