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

#include <math.h>
#include <float.h>
#include <time.h>
#include <string.h>

#include "basis_types.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_string.h"
#include "sge_answer.h"
#include "sge_dstring.h"
#include "sge_ulong.h"
#include "sge_centry.h"

#include "msg_sgeobjlib.h"

#define ULONG_LAYER TOP_LAYER

/*
* NOTES
*     MT-NOTE: double_print_infinity_to_dstring() is MT safe
*/
bool double_print_infinity_to_dstring(double value, dstring *string)
{
   bool ret = true;

   DENTER(ULONG_LAYER, "double_print_infinity_to_dstring");
   if (string != NULL) {
      if (value == DBL_MAX) {
         sge_dstring_sprintf_append(string, "infinity");
      } else {
         ret = false;
      }
   }
   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: double_print_time_to_dstring() is MT safe
*/
bool double_print_time_to_dstring(double value, dstring *string) 
{
   bool ret = true;

   DENTER(ULONG_LAYER, "double_print_time_to_dstring");
   if (string != NULL) {
      if (!double_print_infinity_to_dstring(value, string)) {
         const u_long32 minute_in_seconds = 60;
         const u_long32 hour_in_seconds = minute_in_seconds * 60;
         const u_long32 day_in_seconds = hour_in_seconds * 24;
         int seconds, minutes, hours, days;

         seconds = value;
         days = seconds / day_in_seconds;
         seconds -= days * day_in_seconds;
         hours = seconds / hour_in_seconds;
         seconds -= hours * hour_in_seconds;
         minutes = seconds / minute_in_seconds;
         seconds -= minutes * minute_in_seconds;

         if (days > 0) {
            sge_dstring_sprintf_append(string, "%d:%02d:%02d:%02d", 
                                       days, hours, minutes, seconds);
         } else {
            sge_dstring_sprintf_append(string, "%2.2d:%2.2d:%2.2d", 
                                       hours, minutes, seconds);
         } 
      }
   }
   DEXIT;
   return ret; 
}

/*
* NOTES
*     MT-NOTE: double_print_memory_to_dstring() is MT safe
*/
bool double_print_memory_to_dstring(double value, dstring *string)
{
   bool ret = true;

   DENTER(ULONG_LAYER, "double_print_memory_to_dstring");
   if (string != NULL) {
      if (!double_print_infinity_to_dstring(value, string)) {
         const double kilo_byte = 1024;
         const double mega_byte = kilo_byte * 1024;
         const double giga_byte = mega_byte * 1024;
         double absolute_value = fabs(value); 
         char unit = '\0';

         if (absolute_value >= giga_byte) {
            value /= giga_byte;
            unit = 'G';
         } else if (absolute_value >= mega_byte) {
            value /= mega_byte;
            unit = 'M';
         } else if (absolute_value >= kilo_byte) {
            value /= kilo_byte;
            unit = 'K';
         }
         else {
            unit = 'B';
         }
         if (unit != '\0') {
            sge_dstring_sprintf_append(string, "%.3f%c", value, unit);
         } else {
            sge_dstring_sprintf_append(string, "%.3f", absolute_value);
         }
      } 
   }
   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: double_print_to_dstring() is MT safe
*/
bool double_print_to_dstring(double value, dstring *string)
{
   bool ret = true;

   DENTER(ULONG_LAYER, "double_print_to_dstring");
   if (string != NULL) {
      if (!double_print_infinity_to_dstring(value, string)) {
         sge_dstring_sprintf_append(string, "%f", value);
      } 
   }
   DEXIT;
   return ret;
}

bool 
ulong_parse_date_time_from_string(u_long32 *this_ulong, 
                                  lList **answer_list, const char *string) 
{
   int i;
   int year_fieldlen=2;
   const char *seconds;
   const char *non_seconds;
   stringT tmp_str;
   stringT inp_date_str;

   time_t gmt_secs;
   struct tm *tmp_timeptr,timeptr;

   DENTER(TOP_LAYER, "ulong_parse_date_time_from_string");

   memset(tmp_str, 0, sizeof(tmp_str));

   if (!string || string[0] == '\0') {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_NODATE));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;   
      DEXIT;
      return false;
   }

   if (strlen(string) > sizeof(stringT)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_STARTTIMETOOLONG));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   strcpy(inp_date_str, string);
   non_seconds=sge_strtok(inp_date_str,".");
   seconds=sge_strtok(NULL,".");

   if (seconds) {
      i=strlen(seconds);
   } else {
      i = 0;
   }

   if ((i != 0) && (i != 2)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDSECONDS));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   i=strlen(non_seconds);

   if ((i != 8) && (i != 10) && (i != 12)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDHOURMIN));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   memset((char *)&timeptr, 0, sizeof(timeptr));

   if (i==12) {
      year_fieldlen=4;
   }

   if (i>=10) {
      memset(tmp_str, 0, sizeof(tmp_str));
      memcpy(tmp_str, non_seconds, year_fieldlen);
      timeptr.tm_year=atoi(tmp_str);
      if (i==12) {
         timeptr.tm_year -= 1900;
      }
      non_seconds+=year_fieldlen;
   } else {
      gmt_secs=sge_get_gmt();
      tmp_timeptr=localtime(&gmt_secs);
      timeptr.tm_year=tmp_timeptr->tm_year;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_mon=atoi(tmp_str)-1;/* 00==Jan, we don't like that do we */
   if ((timeptr.tm_mon>11)||(timeptr.tm_mon<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDMONTH));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   non_seconds+=2;

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_mday=atoi(tmp_str);
   non_seconds+=2;

   /* yea, we should do it by mon ths */
   if ((timeptr.tm_mday > 31) || (timeptr.tm_mday < 1)) {
      /* actually mktime() should frigging do it */
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDDAY));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_hour=atoi(tmp_str);
   non_seconds+=2;

   if ((timeptr.tm_hour > 23) || (timeptr.tm_hour < 0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDHOUR));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_min=atoi(tmp_str);

   if ((timeptr.tm_min > 59)||(timeptr.tm_min < 0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDMINUTE));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   if (seconds) {
      timeptr.tm_sec=atoi(seconds);
   }
   if ((timeptr.tm_sec>59)||(timeptr.tm_mday<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDSECOND));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   /*
   ** for daylight saving corrections
   */
   timeptr.tm_isdst = -1;

   gmt_secs=mktime(&timeptr);

   DPRINTF(("mktime returned: %ld\n",gmt_secs));

   if (gmt_secs < 0) {
      DPRINTF(("input to mktime: %s\n",asctime(&timeptr)));
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_NODATEFROMINPUT));
      if (answer_list) {
         answer_list_add(answer_list, SGE_EVENT, 
                         STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      } else {
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      }
      *this_ulong = -1;
      DEXIT;
      return false;
   }

   DPRINTF(("%s",ctime((time_t *)&gmt_secs)));

   *this_ulong = gmt_secs;
   DEXIT;
   return true;
}

bool
ulong_parse_centry_type_from_string(u_long32 *this_ulong,
                                    lList **answer_list, const char *string)
{
   bool ret = true;
   int i;
   DENTER(TOP_LAYER, "ulong_parse_centry_type_from_string");

   *this_ulong = 0;
   for (i = TYPE_FIRST; i <= TYPE_CE_LAST; i++) {
      if (!strcasecmp(string, map_type2str(i))) {
         *this_ulong = i;
         break;
      }
   }
   if (*this_ulong == 0) {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                              MSG_INVALID_CENTRY_TYPE_S, string);
      ret = false;
   }
   DEXIT;
   return ret;
}

bool
ulong_parse_centry_relop_from_string(u_long32 *this_ulong,
                                     lList **answer_list, const char *string)
{
   bool ret = true;
   int i;
   DENTER(TOP_LAYER, "ulong_parse_centry_relop_from_string");

   *this_ulong = 0;
   for (i = CMPLXEQ_OP; i <= CMPLXNE_OP; i++) {
      if (!strcasecmp(string, map_op2str(i))) {
         *this_ulong = i;
         break;
      }
   }
   if (*this_ulong == 0) {
      answer_list_add_sprintf(answer_list, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR,
                              MSG_INVALID_CENTRY_RELOP_S, string);
      ret = false;
   }
   DEXIT;
   return ret;
}

