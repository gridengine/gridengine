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

#include <stdarg.h>
#include <time.h>
#include <sys/times.h>
#include <limits.h>
#include <unistd.h>

#include "basis_types.h"

#include "sge_dstring.h"
#include "sge_log.h"

#include "sgermon.h"

#include "msg_utilib.h"

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
*     To distinguish profiling data for different modules of a program,
*     (e.g. time/cpu consumed for communication, spooling, etc.),
*     multiple levels have been introduced.
*
*     Besides the predefined levels, there exist 10 custom levels 
*     (SGE_PROF_CUSTOM0 up to SGE_PROF_CUSTOM9) that can
*     be used to profile individual parts of a program.
*
*     The predefined levels shall only be used by the developers of the 
*     corresponding components.
*
*     The level SGE_PROF_OTHER is maintained by the profiling module itself 
*     and collects the remaining usage not covered by other levels. It shall 
*     NOT be used outside the profiling module itself.
*  
*     A measurement is always done for a certain level.
*     If a measurement for a certain level is started, while another
*     measurement for another level is still active, the data of the
*     subordinated measurement is remembered in the superordinated level.
*
*     When retrieving profiling information, a parameter defines whether
*     information for subordinated measurements shall be included or excluded.
*
*     Cyclic starting of measurements is not allowed and will result in 
*     profiling being switched off.
*
*  EXAMPLE
*     There is a test program (libs/uti/test_sge_profiling) and the
*     module is used in sge_schedd (daemons/schedd/scheduler.c),
*     in the event mirror module (libs/gdi/sge_mirror.c) and the spooling
*     test program (libs/spool/test_sge_spooling.c).
*
*  SEE ALSO
*     uti/profiling/prof_is_active()
*     uti/profiling/prof_start()
*     uti/profiling/prof_stop()
*     uti/profiling/prof_start_measurement()
*     uti/profiling/prof_stop_measurement()
*     uti/profiling/prof_reset()
*     uti/profiling/prof_get_measurement_wallclock()
*     uti/profiling/prof_get_measurement_utime()
*     uti/profiling/prof_get_measurement_stime()
*     uti/profiling/prof_get_total_wallclock()
*     uti/profiling/prof_get_total_busy()
*     uti/profiling/prof_get_total_utime()
*     uti/profiling/prof_get_total_stime()
*     uti/profiling/prof_get_info_string()
*
*  NOTES
*     MT-NOTE: this module is not MT safe due to access to global variables
*******************************************************************************/

/****** uti/profiling/-Profiling-Defines ***************************************
*  NAME
*     Defines -- Defines and macros for profiling
*
*  SYNOPSIS
*     #define PROF_START_MEASUREMENT ...
*     #define PROF_STOP_MEASUREMENT ...
*
*  FUNCTION
*     Macros to start and stop a profiling measurement.
*     They test if profiling is enabled at all and then call the
*     functions to start or stop a measurement.
*
*  SEE ALSO
*     uti/profiling/prof_is_active()
*     uti/profiling/prof_start_measurement()
*     uti/profiling/prof_stop_measurement()
*******************************************************************************/

/****** uti/profiling/-Profiling-Global-Variables() ****************************
*  NAME
*     Global Variables -- global variables for profiling
*
*  SYNOPSIS
*     
*  FUNCTION
*     bool prof_is_started
*     true, if profiling has been started, else false.
*     Use the function prof_is_active to retrievs this state.
*
*     clock_t start_clock
*     time when profiling was started.
*
*     prof_level act_level
*     actual profiling level
*
*     prof_info prof_base[]
*     Array holding the profiling information for all levels.
*
*  NOTES
*     MT-NOTE: The use of global variables makes the whole module non thread
*              safe.
*******************************************************************************/

static bool prof_is_started = false;
static clock_t start_clock  = 0;
prof_level akt_level = SGE_PROF_NONE;

typedef struct {
   const char *name;
   int nested_calls;          /* number of nested calls within same level  */
   clock_t start;             /* start time of actual measurement          */
   clock_t end;               /* end time of last measurement              */
   struct tms tms_start;      /* time struct of measurement start          */
   struct tms tms_end;        /* time struct of measurement end            */
   clock_t total;             /* summed up clock ticks of all measurements */
   clock_t total_utime;       /* total user time                           */
   clock_t total_stime;       /* total system time                         */

   prof_level pre;            /* predecessor of this level - ALL = no      */
   clock_t sub;               /* time spent in sub measurements            */
   clock_t sub_utime;         /* utime spent in sub measurements           */
   clock_t sub_stime;         /* stime spent in sub measurements           */
   clock_t sub_total;         /* total sub time                            */
   clock_t sub_total_utime;   /* total sub utime                           */
   clock_t sub_total_stime;   /* total sub stime                           */
} prof_info;

static prof_info prof_base[SGE_PROF_ALL] = {
   { "other",           0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "communication",   0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "packing",         0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "eventclient",     0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "eventmaster",     0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "mirror",          0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "spooling",        0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "spooling-io",        0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { "gdi",             0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
/* SGE_PROF_CUSTOM levels */
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
/* SGE_PROF_SCHEDLIB levels */
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 },
   { NULL,              0, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, SGE_PROF_NONE, 0, 0, 0, 0, 0, 0 }
};

static const char *prof_add_error_sprintf(dstring *buffer, const char *fmt, ...);

/****** uti/profiling/prof_set_level_name() ************************************
*  NAME
*     prof_set_level_name() -- set name of a custom level
*
*  SYNOPSIS
*     bool prof_set_level_name(prof_level level, const char *name) 
*
*  FUNCTION
*     Set the name of a custom profiling level.
*
*  INPUTS
*     prof_level level - level to edit
*     const char *name - new name for level
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_set_level_name() is not MT safe
*
*  SEE ALSO
*     uti/profiling/--Profiling
*******************************************************************************/
bool prof_set_level_name(prof_level level, const char *name, dstring *error)
{
   bool ret = false;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_set_level_name", level);
   } else {
      prof_base[level].name = name;
      ret = true;
   }
   
   return ret;
}

/****** uti/profiling/prof_is_active() ****************************************
*  NAME
*     prof_is_active() -- is profiling active?
*
*  SYNOPSIS
*     bool prof_is_active(void) 
*
*  FUNCTION
*     Returns true, if profiling is active, else false.
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  SEE ALSO
*     uti/profiling/prof_stop()
*     uti/profiling/prof_stop()
*
*  NOTES
*     MT-NOTE: prof_is_active() is not MT safe
*******************************************************************************/
bool prof_is_active(void)
{
   return prof_is_started;
}

/****** uti/profiling/prof_start() ****************************************
*  NAME
*     prof_start() -- start profiling
*
*  SYNOPSIS
*     bool prof_start(dstring *error) 
*
*  FUNCTION
*     Enables profiling. All internal variables are reset to 0.
*     Performance measurement has to be started and stopped by
*     calling profiling_start_measurement or profiling_stop_measurement.
*
*  INPUTS
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  SEE ALSO
*     uti/profiling/prof_stop()
*     uti/profiling/prof_start_measurement()
*     uti/profiling/prof_stop_measurement()
*
*  NOTES
*     MT-NOTE: prof_start() is not MT safe
*******************************************************************************/
bool prof_start(dstring *error)
{
   bool ret = false;

   if (prof_is_started) {
      prof_add_error_sprintf(error, MSG_PROF_ALREADYACTIVE_S, "prof_start");
   } else {
      struct tms tms_buffer;

      /* we have no actual profiling level */
      akt_level = SGE_PROF_NONE;

      /* get start time */
      start_clock = times(&tms_buffer);

      /* initialize names and reset all data */
      ret = prof_reset(error);

      prof_is_started = true;

      /* implicitly start the OTHER level */
      prof_start_measurement(SGE_PROF_OTHER, error);
   }

   return ret;
}

/****** uti/profiling/prof_stop() *****************************************
*  NAME
*     prof_stop() -- stop profiling
*
*  SYNOPSIS
*     bool prof_stop(dstring *error) 
*
*  FUNCTION
*     Profiling is disabled.
*
*     Subsequent calls to profiling_start_measurement or 
*     profiling_stop_measurement will have no effect.
*
*     Profiling can be re-enabled by calling profiling_start.
*
*  INPUTS
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  SEE ALSO
*     uti/profiling/prof_start()
*     uti/profiling/prof_start_measurement()
*     uti/profiling/prof_stop_measurement()
*
*  NOTES
*     MT-NOTE: prof_start() is not MT safe
*******************************************************************************/
bool prof_stop(dstring *error)
{  
   bool ret = false;

   if (!prof_is_started) {
      prof_add_error_sprintf(error, MSG_PROF_NOTACTIVE_S, "prof_stop");
   } else {
      prof_stop_measurement(SGE_PROF_OTHER, error);
      prof_is_started = false;
      ret = true;
   }

   return ret;
}

/****** uti/profiling/prof_start_measurement() ****************************
*  NAME
*     prof_start_measurement() -- start measurement
*
*  SYNOPSIS
*     bool prof_start_measurement(prof_level level, dstring *error) 
*
*  FUNCTION
*     Starts measurement of performance data.
*     Retrieves and stores current time and usage information.
*
*  INPUTS
*     prof_level level - level to process
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  SEE ALSO
*     uti/profiling/prof_stop_measurement()
*
*  NOTES
*     MT-NOTE: prof_start_measurement() is not MT safe
*******************************************************************************/
bool prof_start_measurement(prof_level level, dstring *error)
{
   bool ret = false;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_start_measurement", level);
   } else if (!prof_is_started) { 
      prof_add_error_sprintf(error, MSG_PROF_NOTACTIVE_S, "prof_start_measurement");
   } else {
      prof_info *info = &prof_base[level];

      if (akt_level == level) {
         /* multiple start_measurement calls within one level are allowed */
         info->nested_calls++;
         ret = true;
      } else if (info->pre != SGE_PROF_NONE) {
         /* we cannot yet handle cyclic measurements between multiple levels
          * produce an error and stop profiling
          */
         prof_add_error_sprintf(error, MSG_PROF_CYCLICNOTALLOWED_SD, "prof_start_measurement", level);
         prof_stop(error);
      } else {
         info->pre = akt_level;
         akt_level = level;

         info->start = times(&(info->tms_start));

         /* when we start a level, we have no sub usage */
         info->sub = 0;
         info->sub_utime = 0;
         info->sub_utime= 0;

         ret = true;
      }
   }

   return ret;
}

/****** uti/profiling/prof_stop_measurement() *****************************
*  NAME
*     prof_stop_measurement() -- stop measurement
*
*  SYNOPSIS
*     bool prof_stop_measurement(prof_level level, dstring *error) 
*
*  FUNCTION
*     Stops measurement for a certain code block.
*     Retrieves and stores current time and usage information.
*     Sums up global usage information.
*
*  INPUTS
*     prof_level level - level to process
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  SEE ALSO
*     uti/profiling/prof_start_measurement()
*
*  NOTES
*     MT-NOTE: prof_stop_measurement() is not MT safe
*******************************************************************************/
bool prof_stop_measurement(prof_level level, dstring *error)
{
   bool ret = false;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_stop_measurement", level);
   } else if (!prof_is_started) {
      prof_add_error_sprintf(error, MSG_PROF_NOTACTIVE_S, "prof_stop_measurement");
   } else {
      prof_info *info = &prof_base[level];
      clock_t time, utime, stime;

      if (info->nested_calls > 0) {
         info->nested_calls--;
         ret = true;
      } else {
         info->end = times(&(info->tms_end));
         time  = info->end - info->start;
         utime = info->tms_end.tms_utime - info->tms_start.tms_utime;
         stime = info->tms_end.tms_stime - info->tms_start.tms_stime;

         info->total += time;
         info->total_utime += utime;
         info->total_stime += stime;

         if (info->pre != SGE_PROF_NONE) {
            prof_info *pre = &prof_base[info->pre];

            pre->sub += time;
            pre->sub_utime += utime;
            pre->sub_stime += stime;

            pre->sub_total += time;
            pre->sub_total_utime += utime;
            pre->sub_total_stime += stime;
            
            akt_level = info->pre;
            info->pre = SGE_PROF_NONE;
         } else {
            akt_level = SGE_PROF_NONE;
         }

         ret = true;
      }
   }

   return ret;
}

/****** uti/profiling/prof_reset() ****************************************
*  NAME
*     prof_reset() -- reset usage information
*
*  SYNOPSIS
*     bool prof_reset(dstring *error) 
*
*  FUNCTION
*     Reset usage and timing information to 0.
*
*  INPUTS
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     bool - true on success, else false is returned and an error message 
*            is returned in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_reset() is not MT safe
*******************************************************************************/
bool prof_reset(dstring *error)
{
   bool ret = true;
   prof_level l;

   if (akt_level > SGE_PROF_OTHER) {
      prof_add_error_sprintf(error, MSG_PROF_RESETWHILEMEASUREMENT_S, "prof_reset");
      ret = false;
   } else {
      struct tms tms_buffer;

      if (prof_is_started) {
         ret = prof_stop_measurement(SGE_PROF_OTHER, error);
      }

      for (l = SGE_PROF_OTHER; l < SGE_PROF_ALL; l++) {
         prof_base[l].start = 0;
         prof_base[l].end = 0;
         prof_base[l].tms_start.tms_utime = 0;
         prof_base[l].tms_start.tms_stime = 0;
         prof_base[l].tms_start.tms_cutime = 0;
         prof_base[l].tms_start.tms_cstime = 0;
         prof_base[l].tms_end.tms_utime = 0;
         prof_base[l].tms_end.tms_stime = 0;
         prof_base[l].tms_end.tms_cutime = 0;
         prof_base[l].tms_end.tms_cstime = 0;
         prof_base[l].total = 0;
         prof_base[l].total_utime = 0;
         prof_base[l].total_stime = 0;

         prof_base[l].pre = SGE_PROF_NONE;
         prof_base[l].sub = 0;
         prof_base[l].sub_utime = 0;
         prof_base[l].sub_stime = 0;
         prof_base[l].sub_total = 0;
         prof_base[l].sub_total_utime = 0;
         prof_base[l].sub_total_stime = 0;
      }

      start_clock = times(&tms_buffer);

      if (prof_is_started) {
         ret = prof_start_measurement(SGE_PROF_OTHER, error);
      }
   }

   return ret;
}
/****** uti/profiling/prof_get_measurement_wallclock() ********************
*  NAME
*     prof_get_measurement_wallclock() -- return wallclock of a measurement
*
*  SYNOPSIS
*     double prof_get_measurement_wallclock(prof_level level, bool with_sub, dstring *error) 
*
*  FUNCTION
*     Returns the wallclock of the last measurement in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the wallclock time of the last measurement
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  RESULT
*     double - the wallclock time
*
*  NOTES
*     MT-NOTE: prof_get_measurement_wallclock() is not MT safe
*******************************************************************************/
double prof_get_measurement_wallclock(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_get_measurement_wallclock", level);
   } else {
      prof_info *info = &prof_base[level];
      
      clock = info->end - info->start;
   
      if (!with_sub) {
         clock -= info->sub;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

/****** uti/profiling/prof_get_measurement_utime() ************************
*  NAME
*     prof_get_measurement_utime() -- return user cpu time of measurement
*
*  SYNOPSIS
*     double prof_get_measurement_utime(prof_level level, bool with_sub, dstring *error) 
*
*  FUNCTION
*     Returns the user cpu time of the last measurement in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the user cpu time of the last measurement
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_measurement_utime() is not MT safe
*******************************************************************************/
double prof_get_measurement_utime(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_get_measurement_utime", level);
   } else {
      prof_info *info = &prof_base[level];
      
      clock = (info->tms_end.tms_utime - info->tms_start.tms_utime);

      if (!with_sub) {
         clock -= info->sub_utime;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

/****** uti/profiling/prof_get_measurement_stime() ************************
*  NAME
*     prof_get_measurement_stime() -- return system cpu time of measurement
*
*  SYNOPSIS
*     double prof_get_measurement_stime(prof_level level, bool with_sub, dstring *error) 
*
*  FUNCTION
*     Returns the system cpu time of the last measurement in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the system cpu time of the last measurement
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_measurement_stime() is not MT safe
*******************************************************************************/
double prof_get_measurement_stime(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_get_measurement_stime", level);
   } else {
      prof_info *info = &prof_base[level];
      
      clock = (info->tms_end.tms_stime - info->tms_start.tms_stime);

      if (!with_sub) {
         clock -= info->sub_stime;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

/****** uti/profiling/prof_get_total_wallclock() **************************
*  NAME
*     prof_get_total_wallclock() -- get total wallclock time
*
*  SYNOPSIS
*     double prof_get_total_wallclock(dstring *error) 
*
*  FUNCTION
*     Returns the wallclock time since profiling was enabled in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the total wallclock time of the profiling run
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_total_wallclock() is not MT safe
*******************************************************************************/

double prof_get_total_wallclock(dstring *error)
{
   double ret = 0.0;

   if (!prof_is_started) {
      prof_add_error_sprintf(error, MSG_PROF_NOTACTIVE_S, "prof_get_total_wallclock");
   } else {
      struct tms tms_buffer;
      clock_t now;

      now = times(&tms_buffer);

      ret = (now - start_clock) * 1.0 / sysconf(_SC_CLK_TCK);
   }

   return ret;
}

/****** uti/profiling/prof_get_total_busy() *******************************
*  NAME
*     prof_get_total_busy() -- return total busy time
*
*  SYNOPSIS
*     double prof_get_total_busy(prof_level level, bool with_sub, dstring *error) 
*
*  FUNCTION
*     Returns the total busy time since profiling was enabled in seconds.
*     Busy time is the time between starting and stopping a measurement.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the total busy time of the profiling run
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_total_busy() is not MT safe
*******************************************************************************/
static double _prof_get_total_busy(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "_prof_get_total_busy", level);
   } else {
      clock = prof_base[level].total;

      if (!with_sub) {
         clock -= prof_base[level].sub_total;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

double prof_get_total_busy(prof_level level, bool with_sub, dstring *error)
{
   double ret = 0.0;

   if (level == SGE_PROF_ALL) {
      prof_level i;

      for (i = 0; i < SGE_PROF_ALL; i++) {
         ret += _prof_get_total_busy(i, with_sub, error);
      }
   } else {
      ret = _prof_get_total_busy(level, with_sub, error);
   }

   return ret;
}

/****** uti/profiling/prof_get_total_utime() ******************************
*  NAME
*     prof_get_total_utime() -- get total user cpu time
*
*  SYNOPSIS
*     double prof_get_total_utime(prof_level level, bool with_sub, dstring *error) 
*
*  FUNCTION
*     Returns the user cpu time since profiling was enabled in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the total user cpu time of the profiling run
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_total_utime() is not MT safe
*******************************************************************************/
static double _prof_get_total_utime(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_get_total_utime", level);
   } else {
      clock = prof_base[level].total_utime;

      if (!with_sub) {
         clock -= prof_base[level].sub_total_utime;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

double prof_get_total_utime(prof_level level, bool with_sub, dstring *error)
{
   double ret = 0.0;

   if (level == SGE_PROF_ALL) {
      prof_level i;

      for (i = 0; i < SGE_PROF_ALL; i++) {
         ret += _prof_get_total_utime(i, with_sub, error);
      }
   } else {
      ret = _prof_get_total_utime(level, with_sub, error);
   }

   return ret;
}

/****** uti/profiling/prof_get_total_stime() ******************************
*  NAME
*     prof_get_total_stime() -- get total system cpu time
*
*  SYNOPSIS
*     double prof_get_total_stime(prof_level level, bool with_sub) 
*
*  FUNCTION
*     Returns the total system cpu time since profiling was enabled in seconds.
*     Resolution is clock ticks (_SC_CLK_TCK).
*
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     double - the total system cpu time of the profiling run
*              on error, 0 is returned and an error message is written 
*              to the buffer given in parameter error, if error != NULL
*
*  NOTES
*     MT-NOTE: prof_get_total_stime() is not MT safe
*******************************************************************************/
static double _prof_get_total_stime(prof_level level, bool with_sub, dstring *error)
{
   clock_t clock = 0;

   if (level >= SGE_PROF_ALL) {
      prof_add_error_sprintf(error, MSG_PROF_INVALIDLEVEL_SD, "prof_get_total_stime", level);
   } else {
      clock = prof_base[level].total_stime;

      if (!with_sub) {
         clock -= prof_base[level].sub_total_stime;
      }
   }

   return clock * 1.0 / sysconf(_SC_CLK_TCK);
}

double prof_get_total_stime(prof_level level, bool with_sub, dstring *error)
{
   double ret = 0.0;

   if (level == SGE_PROF_ALL) {
      prof_level i;

      for (i = 0; i < SGE_PROF_ALL; i++) {
         ret += _prof_get_total_stime(i, with_sub, error);
      }
   } else {
      ret = _prof_get_total_stime(level, with_sub, error);
   }

   return ret;
}

/****** uti/profiling/prof_get_info_string() ******************************
*  NAME
*     prof_get_info_string() -- get informational message 
*
*  SYNOPSIS
*     const char* prof_get_info_string(prof_level level, bool with_sub, dstring *error) 
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
*  INPUTS
*     prof_level level - level to process
*     bool with_sub    - include usage of subordinated measurements?
*     dstring *error   - if != NULL, error messages will be put here
*
*  RESULT
*     const char* - pointer to result string. It is valid until the next
*                   call of prof_get_info_string()
*                   on error, 0 is returned and an error message is written 
*                   to the buffer given in parameter error, if error != NULL
*
*  EXAMPLE
*     The result can look like the following:
*     "wc = 0.190s, utime = 0.120s, stime = 0.000s, runtime 9515s, busy 105s, 
*     utilization 1%"
*
*  NOTES
*     MT-NOTE: prof_get_info_string() is not MT safe
*******************************************************************************/

#define PROF_GET_INFO_FORMAT "PROF: %-15.15s: wc = %10.3fs, utime = %10.3fs, stime = %10.3fs, utilization = %3.0f%%\n"

static const char *
_prof_get_info_string(prof_level level, dstring *info_string, bool with_sub, dstring *error)
{  
   dstring level_string = DSTRING_INIT;
   const char *ret = NULL;

   double busy        = prof_get_total_busy(level, with_sub, error);
   double utime       = prof_get_total_utime(level, with_sub, error);
   double stime       = prof_get_total_stime(level, with_sub, error);
   double utilization = busy > 0 ? (utime + stime) / busy * 100 : 0;

   sge_dstring_sprintf(&level_string, PROF_GET_INFO_FORMAT,
     prof_base[level].name, busy, utime, stime, utilization);

   ret = sge_dstring_append_dstring(info_string, &level_string);
   sge_dstring_free(&level_string);

   return ret;
}


const char *
prof_get_info_string(prof_level level, bool with_sub, dstring *error)
{   
   static dstring info_string = DSTRING_INIT;
   const char *ret = NULL;

   /* clear previous contents */
   sge_dstring_clear(&info_string);

   /* total summary: one line for each level, one line for summary */
   if (level == SGE_PROF_ALL) {
      double busy, utime, stime, utilization;
      prof_level i;
      dstring total_string = DSTRING_INIT;

      prof_stop_measurement(SGE_PROF_OTHER, error);

      busy        = prof_get_total_busy(SGE_PROF_ALL, with_sub, error);
      utime       = prof_get_total_utime(SGE_PROF_ALL, with_sub, error);
      stime       = prof_get_total_stime(SGE_PROF_ALL, with_sub, error);
      utilization = busy > 0 ? (utime + stime) / busy * 100 : 0;
      
      for (i = 0; i < SGE_PROF_ALL; i++) {
         if (prof_base[i].name != NULL) {
            _prof_get_info_string(i, &info_string, with_sub, error);
         }
      }

      prof_start_measurement(SGE_PROF_OTHER, error);

      sge_dstring_sprintf(&total_string, PROF_GET_INFO_FORMAT,
         "total", busy, utime, stime, utilization);

      ret = sge_dstring_append_dstring(&info_string, &total_string);
      sge_dstring_free(&total_string);
   } else {
      if (prof_base[level].name != NULL) {
         ret = _prof_get_info_string(level, &info_string, with_sub, error);
      }
   }

   return ret;
}

static const char *prof_add_error_sprintf(dstring *buffer, const char *fmt, ...)
{
   const char *ret = NULL;

   if (buffer != NULL) {
      dstring new_buffer = DSTRING_INIT;
      va_list ap;

      va_start(ap, fmt);
      ret = sge_dstring_vsprintf(&new_buffer, fmt, ap);
      if (ret != NULL) {
         ret = sge_dstring_append_dstring(buffer, &new_buffer);
      }

      sge_dstring_free(&new_buffer);
   }

   return ret;
}

bool prof_output_info(prof_level level, bool with_sub, const char *info)
{
   bool ret = false;

   DENTER(TOP_LAYER, "prof_output_info");

   if (prof_is_active()) {
      const char *info_message;
      u_long32 saved_logginglevel = log_state_get_log_level();

      log_state_set_log_level(LOG_INFO);
      info_message = prof_get_info_string(level, with_sub, NULL);
      INFO((SGE_EVENT, "PROF: %s%s", info, info_message));
      log_state_set_log_level(saved_logginglevel);
      ret = true;
   }
  
   DEXIT;
   return ret;
}

