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

#include <time.h>
#include <sys/times.h>
#include <limits.h>

#include "basis_types.h"

#include "sge_dstring.h"

#include "sge_profiling.h"

/****** uti/profiling/--Profiling ****************************************
*  NAME
*     Profiling -- A simple profiling utility 
*
*  FUNCTION
*     The profiling module provides a number of utility functions
*     to output some performance metrics about a program, like wallclock,
*     busy time, user and system cpu time etc.
*
*     Profiling can be started and stopped, the measured data can be 
*     reset to 0.
*
*     Profiling for individual code blocks can be done by starting 
*     and stopping a measurement.
*
*     Measured data can be queried individually for each metric variable,
*     or an informational message string can be requested containing the
*     most interesting variables.
*
*  EXAMPLE
*     There is a test program (libs/uti/test_sge_profiling) and the
*     module is used in sge_schedd (daemons/schedd/scheduler.c),
*     in the event mirror module (libs/gdi/sge_mirror.c) and the spooling
*     test program (libs/spool/test_sge_spooling.c).
*
*  SEE ALSO
*     uti/profiling/profiling_start()
*     uti/profiling/profiling_stop()
*     uti/profiling/profiling_start_measurement()
*     uti/profiling/profiling_stop_measurement()
*     uti/profiling/profiling_reset()
*     uti/profiling/profiling_get_measurement_wallclock()
*     uti/profiling/profiling_get_measurement_utime()
*     uti/profiling/profiling_get_measurement_stime()
*     uti/profiling/profiling_get_total_wallclock()
*     uti/profiling/profiling_get_total_busy()
*     uti/profiling/profiling_get_total_utime()
*     uti/profiling/profiling_get_total_stime()
*     uti/profiling/profiling_get_info_string()
*******************************************************************************/

/****** uti/profiling/-Defines *************************************************
*  NAME
*     profiling_start() -- Defines and macros for profiling
*
*  SYNOPSIS
*     #define PROFILING_START_MEASUREMENT ...
*     #define PROFILING_STOP_MEASUREMENT ...
*
*  FUNCTION
*     Macros to start and stop a profiling measurement.
*     They test if profiling is enabled at all and then call the
*     functions to start or stop a measurement.
*
*  SEE ALSO
*     uti/profiling/profiling_start_measurement()
*     uti/profiling/profiling_stop_measurement()
*******************************************************************************/

/****** uti/profiling/-Profiling-Global-Variables() ****************************
*  NAME
*     profiling_start() -- global variables for profiling
*
*  SYNOPSIS
*     int profiling_started = false;
*
*     static clock_t clock_first;
*     static clock_t clock_start;
*     static clock_t clock_end;
*     static clock_t clock_busy;
*     static struct tms tms_first;
*     static struct tms tms_start;
*     static struct tms tms_end;
*
*  FUNCTION
*     profiling_started is set to true, if profiling is enabled.
*     The other global variables are only for internal use.
*******************************************************************************/
bool profiling_started = false;

static clock_t clock_first;
static clock_t clock_start;
static clock_t clock_end;
static clock_t clock_busy;

static struct tms tms_first;
static struct tms tms_start;
static struct tms tms_end;

/****** uti/profiling/profiling_start() ****************************************
*  NAME
*     profiling_start() -- start profiling
*
*  SYNOPSIS
*     void profiling_start(void) 
*
*  FUNCTION
*     Enables profiling. All internal variables are reset to 0.
*     Performance measurement has to be started and stopped by
*     calling profiling_start_measurement or profiling_stop_measurement.
*
*  SEE ALSO
*     uti/profiling/profiling_stop()
*     uti/profiling/profiling_start_measurement()
*     uti/profiling/profiling_stop_measurement()
*******************************************************************************/
void profiling_start(void)
{
   profiling_started = true;
   profiling_reset();
}

/****** uti/profiling/profiling_stop() *****************************************
*  NAME
*     profiling_stop() -- stop profiling
*
*  SYNOPSIS
*     void profiling_stop(void) 
*
*  FUNCTION
*     Profiling is disabled.
*
*     Subsequent calls to profiling_start_measurement or 
*     profiling_stop_measurement will have no effect.
*
*     Profiling can be re-enabled by calling profiling_start.
*
*  SEE ALSO
*     uti/profiling/profiling_start()
*     uti/profiling/profiling_start_measurement()
*     uti/profiling/profiling_stop_measurement()
*******************************************************************************/
void profiling_stop(void)
{
   profiling_started = false;
}

/****** uti/profiling/profiling_start_measurement() ****************************
*  NAME
*     profiling_start_measurement() -- start measurement
*
*  SYNOPSIS
*     void profiling_start_measurement(void) 
*
*  FUNCTION
*     Starts measurement of performance data.
*     Retrieves and stores current time and usage information.
*
*  SEE ALSO
*     uti/profiling/profiling_stop_measurement()
*******************************************************************************/
void profiling_start_measurement(void)
{
   clock_start = times(&tms_start);
}

/****** uti/profiling/profiling_stop_measurement() *****************************
*  NAME
*     profiling_stop_measurement() -- stop measurement
*
*  SYNOPSIS
*     void profiling_stop_measurement(void) 
*
*  FUNCTION
*     Stops measurement for a certain code block.
*     Retrieves and stores current time and usage information.
*     Sums up global usage information.
*
*  SEE ALSO
*     uti/profiling/profiling_start_measurement()
*******************************************************************************/
void profiling_stop_measurement(void)
{
   clock_end = times(&tms_end);
   clock_busy += clock_end - clock_start;
}

/****** uti/profiling/profiling_reset() ****************************************
*  NAME
*     profiling_reset() -- reset usage information
*
*  SYNOPSIS
*     void profiling_reset(void) 
*
*  FUNCTION
*     Reset usage and timing information to 0.
*******************************************************************************/
void profiling_reset(void)
{
   clock_first = times(&tms_first);
   clock_start = times(&tms_start);
   clock_end = times(&tms_end);

   clock_busy  = 0;
}

/****** uti/profiling/profiling_get_measurement_wallclock() ********************
*  NAME
*     profiling_get_measurement_wallclock() -- return wallclock of a measurement
*
*  SYNOPSIS
*     double profiling_get_measurement_wallclock(void) 
*
*  FUNCTION
*     Returns the wallclock of the last measurement in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - the wallclock time
*
*******************************************************************************/
double profiling_get_measurement_wallclock(void)
{
   return (clock_end - clock_start) * 1.0 / CLK_TCK;
}

/****** uti/profiling/profiling_get_measurement_utime() ************************
*  NAME
*     profiling_get_measurement_utime() -- return user cpu time of measurement
*
*  SYNOPSIS
*     double profiling_get_measurement_utime(void) 
*
*  FUNCTION
*     Returns the user cpu time of the last measurement in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - the utime consumed during the last measurement
*
*******************************************************************************/
double profiling_get_measurement_utime(void)
{
   return (tms_end.tms_utime - tms_start.tms_utime) * 1.0 / CLK_TCK;
}

/****** uti/profiling/profiling_get_measurement_stime() ************************
*  NAME
*     profiling_get_measurement_stime() -- return system cpu time of measurement
*
*  SYNOPSIS
*     double profiling_get_measurement_stime(void) 
*
*  FUNCTION
*     Returns the system cpu time of the last measurement in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - the system cpu time 
*
*******************************************************************************/
double profiling_get_measurement_stime(void)
{
   return (tms_end.tms_stime - tms_start.tms_stime) * 1.0 / CLK_TCK;
}


/****** uti/profiling/profiling_get_total_wallclock() **************************
*  NAME
*     profiling_get_total_wallclock() -- get total wallclock time
*
*  SYNOPSIS
*     double profiling_get_total_wallclock(void) 
*
*  FUNCTION
*     Returns the wallclock time since profiling was enabled in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - total wallclock time 
*
*******************************************************************************/
double profiling_get_total_wallclock(void)
{
   return (clock_end - clock_first) * 1.0 / CLK_TCK;
}

/****** uti/profiling/profiling_get_total_busy() *******************************
*  NAME
*     profiling_get_total_busy() -- return total busy time
*
*  SYNOPSIS
*     double profiling_get_total_busy(void) 
*
*  FUNCTION
*     Returns the total busy time since profiling was enabled in seconds.
*     Busy time is the time between starting and stopping a measurement.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - total busy time
*******************************************************************************/
double profiling_get_total_busy(void)
{
   return clock_busy * 1.0 / CLK_TCK;
}

/****** uti/profiling/profiling_get_total_utime() ******************************
*  NAME
*     profiling_get_total_utime() -- get total user cpu time
*
*  SYNOPSIS
*     double profiling_get_total_utime(void) 
*
*  FUNCTION
*     Returns the user cpu time since profiling was enabled in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - total user cpu time
*
*******************************************************************************/
double profiling_get_total_utime(void)
{
   return (tms_end.tms_utime - tms_first.tms_utime) * 1.0 / CLK_TCK;
}

/****** uti/profiling/profiling_get_total_stime() ******************************
*  NAME
*     profiling_get_total_stime() -- get total system cpu time
*
*  SYNOPSIS
*     double profiling_get_total_stime(void) 
*
*  FUNCTION
*     Returns the total system cpu time since profiling was enabled in seconds.
*     Resolution is clock ticks (CLK_TCK).
*
*  RESULT
*     double - total system cpu time
*
*******************************************************************************/
double profiling_get_total_stime(void)
{
   return (tms_end.tms_stime - tms_first.tms_stime) * 1.0 / CLK_TCK;
}


/****** uti/profiling/profiling_get_info_string() ******************************
*  NAME
*     profiling_get_info_string() -- get informational message 
*
*  SYNOPSIS
*     const char* profiling_get_info_string(void) 
*
*  FUNCTION
*     Returns a string containing the most interesting data, both for the
*     last measurement and for the total runtime:
*        - wallclock of measurement
*        - user cpu time of measurement
*        - system cpu time of measurement
*        - total wallclock time (runtime)
*        - total busys time
*        - utilization (busy time / wallclock time) * 100 %
*
*  RESULT
*     const char* - pointer to result string. It is valid until the next
*                   call of profiling_get_info_string()
*
*  EXAMPLE
*     The result can look like the following:
*     "wc = 0.190s, utime = 0.120s, stime = 0.000s, runtime 9515s, busy 105s, 
*     utilization 1%"
*
*******************************************************************************/
const char *profiling_get_info_string(void)
{   
   static dstring info_string = DSTRING_INIT;

   double wallclock = profiling_get_total_wallclock();
   double busy      = profiling_get_total_busy();
   double utilization = wallclock > 0 ? busy / wallclock * 100 : 0;

   sge_dstring_sprintf(&info_string, 
      "wc = %.3fs, utime = %.3fs, stime = %.3fs, runtime %.0fs, busy %.0fs, utilization %.0f%%",
      profiling_get_measurement_wallclock(),
      profiling_get_measurement_utime(),
      profiling_get_measurement_stime(),
      wallclock,
      busy,
      utilization);

   return sge_dstring_get_string(&info_string);
}

