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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/times.h> 

#ifndef WIN32NATIVE
#	include <sys/time.h>
#else 
#	include <winsock2.h>
#endif 

#include "sge_time.h"
#include "sge_unistd.h"
#include "sge_log.h"

#ifdef WIN32
int gettimeofday(struct timeval *tz, struct timezone *tzp);
#endif

#if defined(SUN4) || defined(AIX32)
#  define TIMES_RETURNS_ZERO
#endif
 
#define NESTLEVEL 5
 
static struct tms begin[NESTLEVEL];
static struct tms end[NESTLEVEL];
 
static time_t wtot[NESTLEVEL];
static time_t wbegin[NESTLEVEL];
static time_t wend[NESTLEVEL];
static time_t wprev[NESTLEVEL];
static time_t wdiff[NESTLEVEL];
 
static int clock_tick;
static int time_log_interval[NESTLEVEL] = { -1, -1, -1, -1, -1 };
 
#ifdef TIMES_RETURNS_ZERO
static time_t inittime;
#endif
 
static void sge_stopwatch_stop(int i);  

static void sge_stopwatch_stop(int i)
{
   time_t wend;
 
   if (i < 0 || i >= NESTLEVEL) {
      return;
   }
   if (time_log_interval[i] == -1) {
      return;
   }
   wend = times(&end[i]);
 
#ifdef TIMES_RETURNS_ZERO
   /* times() returns 0 on these machines */
   wend = (sge_get_gmt() - inittime) * clock_tick;
#endif
 
   end[i].tms_utime =  end[i].tms_utime -  begin[i].tms_utime;
   end[i].tms_stime =  end[i].tms_stime -  begin[i].tms_stime;
   end[i].tms_cutime = end[i].tms_cutime - begin[i].tms_cutime;
   end[i].tms_cstime = end[i].tms_cstime - begin[i].tms_cstime;
 
   wtot[i]  = wend - wbegin[i];
   wdiff[i] = wend - wprev[i];
   wprev[i] = wend;
}

/****** uti/time/sge_get_gmt() ************************************************
*  NAME
*     sge_get_gmt() -- Return current time 
*
*  SYNOPSIS
*     u_long32 sge_get_gmt() 
*
*  FUNCTION
*     Return current time 
*
*  RESULT
*     u_long32 - 32 bit time value
******************************************************************************/
u_long32 sge_get_gmt()
{
#ifndef WIN32NATIVE

   struct timeval now;

#  ifdef SOLARIS

   gettimeofday(&now, NULL);

#  else

   struct timezone tzp;

#     ifdef SINIX

   gettimeofday(&now);

#     else

   gettimeofday(&now, &tzp);

#     endif

#  endif

   return (u_long32)now.tv_sec;
#else
   time_t long_time;
   struct tm *gmtimeval;

	time(&long_time);                  /* Get time as long integer. */
	gmtimeval = gmtime(&long_time);    /* Convert to local time. */
	long_time = mktime(gmtimeval);
	return long_time;
#endif
}

/****** uti/time/sge_ctime() **************************************************
*  NAME
*     sge_ctime() -- Convert time value into string 
*
*  SYNOPSIS
*     char* sge_ctime(time_t i) 
*
*  FUNCTION
*     Convert time value into string 
*
*  INPUTS
*     time_t i - 0 or time value 
*
*  RESULT
*     char* - time string (current time if 'i' was 0) 
*
*  SEE ALSO
*     uti/time/sge_ctime32()
******************************************************************************/
char *sge_ctime(time_t i) 
{
   struct tm *tm;
   static char str[128];

   if (!i)
      i = sge_get_gmt();
   tm = localtime(&i);
   sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d",
           tm->tm_mon + 1, tm->tm_mday, 1900 + tm->tm_year,
           tm->tm_hour, tm->tm_min, tm->tm_sec);

   return str;
}

/****** uti/time/sge_ctime32() ************************************************
*  NAME
*     sge_ctime32() -- Convert time value into string (64 bit time_t) 
*
*  SYNOPSIS
*     char* sge_ctime32(u_long32 *i) 
*
*  FUNCTION
*     Convert time value into string. This function is needed for 
*     systems with a 64 bit time_t because the ctime function would
*     otherwise try to interpret the u_long32 value as 
*     a 64 bit value => $&&%$§$%!
*
*  INPUTS
*     u_long32 *i - 0 or time value 
*
*  RESULT
*     char* - time string (current time if 'i' was 0)
* 
*  SEE ALSO
*     uti/time/sge_ctime()
******************************************************************************/
char *sge_ctime32(u_long32 *i) 
{
#if SOLARIS64
   volatile
#endif
   time_t temp = *i;

   return ctime((time_t *)&temp);
}

/****** uti/time/sge_at_time() ************************************************
*  NAME
*     sge_at_time() -- ??? 
*
*  SYNOPSIS
*     char* sge_at_time(time_t i) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     time_t i - 0 or time value 
*
*  RESULT
*     char* - time string (current time if 'i' was 0) 
*
*  SEE ALSO
*     uti/time/sge_ctime() 
******************************************************************************/
char *sge_at_time(time_t i) 
{
   struct tm *tm;
   static char str[128];

   if (!i)
      i = sge_get_gmt();
   tm = localtime(&i);
   sprintf(str, "%04d%02d%02d%02d%02d.%02d",
           tm->tm_year+1900, tm->tm_mon + 1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec);

   return str;
}

/****** uti/time/sge_stopwatch_log() ******************************************
*  NAME
*     sge_stopwatch_log() -- ??? 
*
*  SYNOPSIS
*     void sge_stopwatch_log(int i, const char *str) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int i           - ??? 
*     const char *str - ??? 
*
*  SEE ALSO
*     uti/time/sge_stopwatch_start() 
******************************************************************************/
void sge_stopwatch_log(int i, const char *str)
{
   if (i < 0 || i >= NESTLEVEL) {
      return;
   }
   if (time_log_interval[i] == -1) {
      return;
   }
 
   sge_stopwatch_stop(i);
 
   if ((wdiff[i] * 1000) / clock_tick >= time_log_interval[i]) {
      static char SGE_FUNC[] = "";
 
      WARNING((SGE_EVENT, "%-30s: %d/%d/%d", str,
               (int) ((wtot[i] * 1000) / clock_tick),
               (int) ((end[i].tms_utime * 1000)  / clock_tick),
               (int) ((end[i].tms_stime  * 1000) / clock_tick)));
   }
}          

/****** uti/time/sge_stopwatch_start() ****************************************
*  NAME
*     sge_stopwatch_start() -- ??? 
*
*  SYNOPSIS
*     void sge_stopwatch_start(int i) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int i - ??? 
*
*  SEE ALSO
*     uti/time/sge_stopwatch_log() 
******************************************************************************/
void sge_stopwatch_start(int i)
{
   static int first = 1;
   int j;
   char *cp;
 
   if (first) {
      char buf[24];
 
      clock_tick = sysconf(_SC_CLK_TCK);
      for (j = 0; j < NESTLEVEL; j++) {
         wtot[j] = wbegin[j] = wend[j] = wprev[j] = wdiff[j] = 0;
         sprintf(buf, "SGE_TIMELOG%d", j);
         if ((cp = getenv(buf)) && (atoi(cp) >= 0)) {
            time_log_interval[j] = atoi(cp);
         } else {
            time_log_interval[j] = -1;
         }
      }
#if defined(SUN4) || defined(AIX32)
      inittime = sge_get_gmt();
#endif
      first = 0;
   }
 
   if (i < 0 || i >= NESTLEVEL) {
      return;
   }
   if (time_log_interval[i] == -1) {
      return;
   }
   wbegin[i] = times(&begin[i]);
 
#ifdef TIMES_RETURNS_ZERO
   /* times() return 0 on these machines */
   wbegin[i] = (sge_get_gmt() - inittime) * clock_tick;
#endif
 
   wprev[i]  = wbegin[i];
}                                                                              
 
