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
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <stdlib.h>

#include "sge_timestop.h"
#include "sgermon.h"
#include "sge_log.h"

#if defined(SUN4) || defined(AIX32)
#define TIMES_RETURNS_ZERO      
#include "sge_time.h"
#endif

#define NESTLEVEL 5

static struct tms begin[NESTLEVEL], end[NESTLEVEL];
static time_t wtot[NESTLEVEL], wbegin[NESTLEVEL], wend[NESTLEVEL], wprev[NESTLEVEL], wdiff[NESTLEVEL];
static int clock_tick, time_log_interval[NESTLEVEL] = { -1, -1, -1, -1, -1 };

#ifdef TIMES_RETURNS_ZERO
static time_t inittime;
#endif

/*---------------------------------------------------------------*/
void starttime(
int i 
) {
   static int first = 1;
   int j;
   char *cp;
   
   if (first) {
      char buf[24];
      clock_tick = sysconf(_SC_CLK_TCK);
      for (j = 0; j < NESTLEVEL; j++) {
         wtot[j] = wbegin[j] = wend[j] = wprev[j] = wdiff[j] = 0;
         sprintf(buf, "SGE_TIMELOG%d", j);      
         if ((cp = getenv(buf)) && (atoi(cp) >= 0))
            time_log_interval[j] = atoi(cp);
         else
            time_log_interval[j] = -1;
      }
#if defined(SUN4) || defined(AIX32)
      inittime = sge_get_gmt();
#endif            
      first = 0;
   }

   if (i < 0 || i >= NESTLEVEL)
      return;
      
   if (time_log_interval[i] == -1)
      return;
 
   wbegin[i] = times(&begin[i]);

#ifdef TIMES_RETURNS_ZERO
   /* times() return 0 on these machines */
   wbegin[i] = (sge_get_gmt() - inittime) * clock_tick;
#endif
 
   wprev[i]  = wbegin[i];
} 
  
/*---------------------------------------------------------------*/
void endtime(
int i 
) {
   time_t wend;

   if (i < 0 || i >= NESTLEVEL)
      return;   

   if (time_log_interval[i] == -1)
      return;
    
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

/*---------------------------------------------------------------*/
void log_time(
int i,
char *str 
) {

   if (i < 0 || i >= NESTLEVEL)
      return;
      
   if (time_log_interval[i] == -1)
     return;

   endtime(i);

   if ((wdiff[i] * 1000) / clock_tick >= time_log_interval[i]) {
      static char SGE_FUNC[] = "";
      WARNING((SGE_EVENT, "%-30s: %d/%d/%d",
           str, 
           (int) ((wtot[i] * 1000) / clock_tick), 
           (int) ((end[i].tms_utime * 1000)  / clock_tick),
           (int) ((end[i].tms_stime  * 1000) / clock_tick)));
   }
}
