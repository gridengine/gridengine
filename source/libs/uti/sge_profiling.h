#ifndef __SGE_PROFILING_H
#define __SGE_PROFILING_H
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

#include "basis_types.h"

#include "sge_dstring.h"

typedef enum {
   SGE_PROF_NONE = -1,
   SGE_PROF_OTHER = 0,
   SGE_PROF_COMMUNICATION,
   SGE_PROF_EVENTCLIENT,
   SGE_PROF_EVENTMASTER,
   SGE_PROF_MIRROR,
   SGE_PROF_SPOOLING,
   SGE_PROF_GDI,
   SGE_PROF_CUSTOM0,
   SGE_PROF_CUSTOM1,
   SGE_PROF_CUSTOM2,
   SGE_PROF_CUSTOM3,
   SGE_PROF_CUSTOM4,
   SGE_PROF_CUSTOM5,
   SGE_PROF_CUSTOM6,
   SGE_PROF_CUSTOM7,
   SGE_PROF_CUSTOM8,
   SGE_PROF_CUSTOM9,
   SGE_PROF_ALL
} prof_level;

bool prof_set_level_name(prof_level level, const char *name, dstring *error);

bool prof_is_active(void);

bool prof_start(dstring *error);
bool prof_stop(dstring *error);

bool prof_start_measurement(prof_level level, dstring *error);
bool prof_stop_measurement(prof_level level, dstring *error);

#define PROF_START_MEASUREMENT(level) \
   if(prof_is_active()) {\
      prof_start_measurement(level, NULL);\
   }   

#define PROF_STOP_MEASUREMENT(level) \
   if(prof_is_active()) {\
      prof_stop_measurement(level, NULL);\
   }   

bool prof_reset(dstring *error);

double prof_get_measurement_wallclock(prof_level level, bool with_sub, dstring *error);
double prof_get_measurement_utime(prof_level level, bool with_sub, dstring *error);
double prof_get_measurement_stime(prof_level level, bool with_sub, dstring *error);

double prof_get_total_wallclock(dstring *error);
double prof_get_total_busy(prof_level level, bool with_sub, dstring *error);
double prof_get_total_utime(prof_level level, bool with_sub, dstring *error);
double prof_get_total_stime(prof_level level, bool with_sub, dstring *error);

const char *prof_get_info_string(prof_level level, bool with_sub, dstring *error);

bool prof_output_info(prof_level level, bool with_sub, const char *info);

#endif /* __SGE_PROFILING_H */
