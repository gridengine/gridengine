#ifndef __SGE_CALENDARL_H
#define __SGE_CALENDARL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */   

/* 
 * this data structure represents the SGE calendar object
 */
enum {
   CAL_name = CAL_LOWERBOUND,
   CAL_year_calendar,
   CAL_week_calendar,
   CAL_parsed_year_calendar,
   CAL_parsed_week_calendar
};

ILISTDEF(CAL_Type, Calendar, SGE_CALENDAR_LIST)
   SGE_KSTRINGHU(CAL_name)
   SGE_STRING(CAL_year_calendar)
   SGE_STRING(CAL_week_calendar)
   /* non spooling fields */
   SGE_XLIST(CAL_parsed_year_calendar, CA_Type)
   SGE_XLIST(CAL_parsed_week_calendar, CA_Type)
LISTEND 

NAMEDEF(CALN)
   NAME("CAL_name")
   NAME("CAL_year_calendar")
   NAME("CAL_week_calendar")
   NAME("CAL_parsed_year_calendar")
   NAME("CAL_parsed_week_calendar")
NAMEEND

#define CALS sizeof(CALN)/sizeof(char*)

/* 
 * this data structure is used for parsing calendar functionality
 */
enum {
   CA_yday_range_list = CA_LOWERBOUND,
   CA_wday_range_list,
   CA_daytime_range_list,
   CA_state
};

LISTDEF(CA_Type)
   SGE_LIST(CA_yday_range_list)       /* TMR_Type with begin/end of type *
                                       * TM_Type using *
                                       * TM_mday/TM_mon/TM_year */
   SGE_LIST(CA_wday_range_list)       /* TMR_Type with begin/end of type *
                                       * TM_Type using TM_wday */
   SGE_LIST(CA_daytime_range_list)    /* TMR_Type with begin/end of type *
                                       * TM_Type using *
                                       * TM_sec/TM_min/TM_hour */
   SGE_ULONG(CA_state)
LISTEND 

NAMEDEF(CAN)
   NAME("CA_yday_range_list")
   NAME("CA_wday_range_list")
   NAME("CA_daytime_range_list")
   NAME("CA_state")
NAMEEND

#define CAS sizeof(CAN)/sizeof(char*)

/* 
 * this data structure is used for ranges of TM_Type
 */
enum {
   TMR_begin = TMR_LOWERBOUND,
   TMR_end
};

LISTDEF(TMR_Type)
   SGE_LIST(TMR_begin)        /* TM_Type */
   SGE_LIST(TMR_end)          /* TM_Type */
LISTEND 

NAMEDEF(TMRN)
   NAME("TMR_begin")
   NAME("TMR_end")
NAMEEND

#define TMRS sizeof(TMRN)/sizeof(char*)

/* 
 * this data structure is used for 
 *    yeardays 
 *        TM_mday
 *        TM_mon
 *        TM_year
 *    weekdays 
 *        TM_wday
 *    daytimes 
 *        TM_hour
 *        TM_min
 *        TM_sec
 *        
 *   TM_Type borrows most fields and meaning from struct tm
 */
enum {
   TM_mday = TM_LOWERBOUND,
   TM_mon,
   TM_year,
   TM_sec,
   TM_min,
   TM_hour,
   TM_wday,
   TM_yday,
   TM_isdst
};

LISTDEF(TM_Type)
   SGE_ULONG(TM_mday)         /* 1-32 */
   SGE_ULONG(TM_mon)          /* 0-11 */
   SGE_ULONG(TM_year)         /* The number of years since 1900. */
   SGE_ULONG(TM_sec)          /* 0-59 */
   SGE_ULONG(TM_min)          /* 0-59 */
   SGE_ULONG(TM_hour)         /* 0-23 */
   SGE_ULONG(TM_wday)         /* 0-6 */
   SGE_ULONG(TM_yday)         /* ?? */
   SGE_ULONG(TM_isdst)        /* 1 or 0 */
LISTEND 

NAMEDEF(TMN)
   NAME("TM_mday")
   NAME("TM_mon")
   NAME("TM_year")
   NAME("TM_sec")
   NAME("TM_min")
   NAME("TM_hour")
   NAME("TM_wday")
   NAME("TM_yday")
   NAME("TM_isdst")
NAMEEND

#define TMS sizeof(TMN)/sizeof(char*)

/* *INDENT-ON* */ 

#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_CALENDARL_H */
