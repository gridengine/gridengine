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
#include <time.h>
#include <sys/types.h>
#ifndef WIN32NATIVE
#	include <sys/time.h>
#else 
#	include <winsock2.h>
#endif 

#include "sge_time.h"

#ifdef WIN32
int gettimeofday(struct timeval *tz, struct timezone *tzp);
#endif

/************************************************************************/
u_long32 sge_get_gmt()
{
#ifndef WIN32NATIVE

   struct timeval now;

#ifdef SOLARIS
   gettimeofday(&now, NULL);
#else
   struct timezone tzp;
#ifdef SINIX
   gettimeofday(&now);
#else
   gettimeofday(&now, &tzp);
#endif
#endif

   return (u_long32)now.tv_sec;
#else
   time_t long_time;
struct tm *gmtimeval;

	time( &long_time );                /* Get time as long integer. */
	gmtimeval = gmtime(&long_time);    /* Convert to local time. */
	long_time = mktime(gmtimeval);
	return long_time;
#endif
}

/************************************************************************/
char *sge_ctime(
time_t i 
) {
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

/************************************************************************/
/* this function is needed for systems with a 64 bit time_t             */
/* because the ctime function would otherwise try to interpret the      */
/* u_long32 value as a 64 bit value => $&&%$§$%!                        */
char *sge_ctime32(
u_long32 *i 
) {
#if SOLARIS64
   volatile
#endif
   time_t temp = *i;

   return ctime((time_t *)&temp);
}

/************************************************************************/
char *sge_at_time(
time_t i 
) {
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


