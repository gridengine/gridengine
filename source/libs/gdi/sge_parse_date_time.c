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
#include <string.h>
#include <stdlib.h>

#include "sge_gdi_intern.h"
#include "sgermon.h"
#include "sge_answerL.h"
#include "sge_parse_date_time.h"
#include "sge_string.h"
#include "sge_log.h"
#include "sge_time.h"
#include "msg_gdilib.h"

/*  -------------------------

    taken from sge v3

   -a date_time 
   "touch -t" format
   [[CC]YY]MMDDhhmm[.SS]

*/

u_long sge_parse_date_time(
const char *date_str,
struct tm *time,
lList **alpp 
) {
   int i;
   int year_fieldlen=2;
   const char *seconds;
   const char *non_seconds;
   stringT tmp_str;
   stringT inp_date_str;

   time_t gmt_secs;
   struct tm *tmp_timeptr,timeptr;

   DENTER(TOP_LAYER, "sge_parse_date_time");

   memset(tmp_str, 0, sizeof(tmp_str));

   if (!date_str || date_str[0] == '\0') {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_NODATE)); 
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   if (strlen(date_str) > sizeof(stringT)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_STARTTIMETOOLONG)); 
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   strcpy(inp_date_str,date_str);
   non_seconds=sge_strtok(inp_date_str,".");
   seconds=sge_strtok(NULL,".");

   if(seconds)        /* added by wdg */
      i=strlen(seconds);
   else
      i = 0;

   if ((i!=0)&&(i!=2)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDSECONDS));
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   i=strlen(non_seconds);

   if ((i!=8)&&(i!=10)&&(i!=12)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDHOURMIN)); 
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   memset((char *)&timeptr, 0, sizeof(timeptr));

   if (i==12)
   year_fieldlen=4;

   if (i>=10) {
      memset(tmp_str, 0, sizeof(tmp_str));
      memcpy(tmp_str, non_seconds, year_fieldlen);
      timeptr.tm_year=atoi(tmp_str);
      if (i==12)  {
         timeptr.tm_year -= 1900;
      }
      else {
         /* the date is before 1970, thus we assume, that
            20XX is ment. This works only till 2069, but
            that should be sufficent for now */
         if (timeptr.tm_year < 70)
            timeptr.tm_year += 100;
      }  

      non_seconds+=year_fieldlen;
   }
   else {
      gmt_secs=sge_get_gmt();
      tmp_timeptr=localtime(&gmt_secs);
      timeptr.tm_year=tmp_timeptr->tm_year;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_mon=atoi(tmp_str)-1;/* 00==Jan, we don't like that do we */
   if ((timeptr.tm_mon>11)||(timeptr.tm_mon<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDMONTH));
      if (alpp) 
	 sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   non_seconds+=2;

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_mday=atoi(tmp_str);
   non_seconds+=2;

   if ((timeptr.tm_mday>31)||(timeptr.tm_mday<1)) /* yea, we should do it by months */ {
      /* actually mktime() should frigging do it */
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDDAY));
      if (alpp) 
	 sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_hour=atoi(tmp_str);
   non_seconds+=2;

   if ((timeptr.tm_hour>23)||(timeptr.tm_hour<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDHOUR));
      if (alpp) 
	 sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   memset(tmp_str, 0, sizeof(tmp_str));
   memcpy(tmp_str, non_seconds, 2);
   timeptr.tm_min=atoi(tmp_str);

   if ((timeptr.tm_min>59)||(timeptr.tm_min<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDMINUTE));
      if (alpp) 
	 sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   if (seconds)
      timeptr.tm_sec=atoi(seconds);
   if ((timeptr.tm_sec>59)||(timeptr.tm_mday<0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_INVALIDSECOND));
      if (alpp) 
	 sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   /*
   ** for daylight saving corrections
   */
   timeptr.tm_isdst = -1;

   /*
   ** return struct tm if requested
   */
   if (time) {
      time->tm_year = timeptr.tm_year;
      time->tm_mon  = timeptr.tm_mon;
      time->tm_mday = timeptr.tm_mday;
      time->tm_hour = timeptr.tm_hour;
      time->tm_min  = timeptr.tm_min;
      time->tm_sec  = timeptr.tm_sec;
      time->tm_isdst = timeptr.tm_isdst;
   }

   gmt_secs=mktime(&timeptr);

   DPRINTF(("mktime returned: %ld\n",gmt_secs));

   if (gmt_secs<0) {
      DPRINTF(("input to mktime: %s\n",asctime(&timeptr)));
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_NODATEFROMINPUT)); 
      if (alpp) 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      else 
         fprintf(stderr,"\n%s\n", SGE_EVENT);
      DEXIT;
      return -1;
   }

   DPRINTF(("%s",ctime((time_t *)&gmt_secs)));

   DEXIT;
   return(gmt_secs);
}


