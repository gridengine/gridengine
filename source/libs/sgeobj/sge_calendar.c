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
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "cull_list.h"
#include "sge.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sgermon.h"
#include "sge_c_gdi.h"
#include "sge_calendar_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "sge_utility.h"
#include "sge_time.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_calendar.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"
#include "msg_qmaster.h"

#ifdef QIDL
   #include "qidl_c_gdi.h"
#endif

enum {
   DOT = 1,
   COLON,
   EQUAL_SIGN,
   MINUS,
   COMMA,
   SPACE,
   NUMBER,
   STRING,
   NO_TOKEN,
   ERR_TOKEN
};

typedef struct {
   int token;
   char *text;
} token_set_t;


static token_set_t statev[] = {
   { QI_DO_ENABLE,   "on" },
   { QI_DO_DISABLE,  "off" },
   { QI_DO_SUSPEND, "suspended" },
   { -1, NULL },
};

lList *Master_Calendar_List = NULL;

typedef int (*cmp_func_t)(lListElem *t1, lListElem *t2); 

static char old_error[1000];
static char store[1000];
static int number;
static int token_is_valid = 0;
static char parse_error[1000];

/* parsing */
static int 
disabled_year_list(lList **alpp, const char *s, 
                   lList **cal, const char *cal_name);

static int 
disabled_week_list(lList **alpp, const char *s, lList **cal, 
                   const char *cal_name); 

static int 
state_at(time_t now, const lList *ycal, lList *wcal, time_t *then);

static int 
scan(const char *s, token_set_t token_set[]);

static char *
get_string(void);

static int 
get_number(void);

static void 
eat_token(void);

static char *
save_error(void);

static int 
cheap_scan(char *s, token_set_t tokenv[], int n, char *name);

static int 
disabled_year_entry(lListElem **calep);

static int 
year_day_range_list(lList **ydrl);

static int 
year_day_range(lListElem **tmr);

static int 
year_day(lListElem **tm);

static int 
month(int *);

static int 
day(int *);

static int 
year(int *);

static int 
tm_yday_cmp(lListElem *t1, lListElem *t2);

static int 
tm_wday_cmp(lListElem *t1, lListElem *t2);

static void 
cullify_tm(lListElem *tm_ep, struct tm *tm_now);

static void 
uncullify_tm(lListElem *tm_ep, struct tm *tm_now);

static int 
normalize_range_list(lList *rl, cmp_func_t cmp_func);

static int 
in_range_list(lListElem *tm, lList *rl, cmp_func_t cmp_func);

static int 
in_range(lListElem *tm, lListElem *r, cmp_func_t cmp_func);

static int 
daytime_range_list(lList **dtrl);

static int 
daytime_range(lListElem **tmr);

static void 
full_daytime_range(lList **dtrl);

static void 
full_weekday_range(lList **dtrl);

static void 
split_daytime_range(lList *dtrl, lListElem *tmr);

static int 
daytime(lListElem **tm);

static int 
hour(int *);

static int 
minute(int *);

static int 
seconds(int *);

static int 
action(int *sp);

static int 
range_number(int min, int max, int *ip, const char *name);

static int 
tm_daytime_cmp(lListElem *t1, lListElem *t2);

static u_long32 
is_week_entry_active(lListElem *tm, lListElem *week_entry, time_t *limit);

static u_long32 
is_year_entry_active(lListElem *tm, lListElem *year_entry, time_t *limit);

static time_t 
compute_limit(int today, int active, lList *dtrl, lListElem *now);

static int 
disabled_week_entry(lListElem **calep);

static int 
week_day_range_list(lList **wdrl);

static int 
week_day_range(lListElem **tmr);

static void 
split_wday_range(lList *wdrl, lListElem *tmr);

static int 
week_day(lListElem **tm);

lListElem *calendar_list_locate(lList *calendar_list, const char *cal_name) 
{
   return lGetElemStr(calendar_list, CAL_name, cal_name);
}

/*

   NAME
      state_at() - computes state using year and week calendar 
                   and optionally when this state will end

   RETURNS
      The actual state according this calendar
   
   PARAMETERS
      now        - actual time 
      ycal       - year calendar
      wcal       - week calendar
      next_event - if this pointer is not NULL then
                   state_at() will compute the next 
                   time when this calendar has to be checked again

   DESCRIPTION
      Here we have to answer two questions:
         1. Which one is the actual state?
            If both this day and the actual daytime are in an entry
            then this entry is active and the entries state is returned.
            In case there are more than one active entries the 'hardest'
            is dominant ( disabled < suspended < enabled )

         2. When will this state end?
            For all active entries we compute the end of the range. 
            For all inactive entries we compute the start of the range.
            We iterate through all these ranges beginning with that range
            which is closest to 'now' in the future.
          
            In some cases the state does not change but the range which is
            responsible for that state changes: 

               mon-fri=9-18 fri=18-19

   CALENDAR
      year
         CA_yday_range_list
            TMR_begin
               TM_mday
               TM_mon
               TM_year
            TMR_end
               TM_mday
               TM_mon
               TM_year
         CA_daytime_range_list
            TMR_begin
               TM_hour
               TM_min
               TM_sec
            TMR_end
               TM_hour
               TM_min
               TM_sec
         CA_state

      week
         CA_wday_range_list
            TMR_begin
               TM_wday
            TMR_end
               TM_wday
         CA_daytime_range_list
            TMR_begin
               TM_hour
               TM_min
               TM_sec
            TMR_end
               TM_hour
               TM_min
               TM_sec
         CA_state

   FUTURE ENHANCEMENTS
      The compution of 'next_event' could be improved. Actually 
      state_at() returns 24 o'clock of this day as the end of
      this state also when this state is valid beyond this day.

*/

/*
ycal,  CA_Type 
wcal,  CA_Type
*/
static int 
state_at(time_t now, const lList *ycal, lList *wcal, time_t *next_event) 
{
   struct tm *tm_now;
   int state = 0, w_is_active = -1, y_is_active;
   lListElem *yc, *wc, *tm;
   time_t limit;

   DENTER(TOP_LAYER, "state_at");

   if (next_event)
      *next_event = 0;

   /* convert time_t format into struct tm format */
   tm_now = localtime(&now);

   /* cullify struct tm */
   tm = lCreateElem(TM_Type);
   cullify_tm(tm, tm_now);

   /* ycal */
   for_each (yc, ycal) {
      if ((y_is_active = is_year_entry_active(tm, yc, next_event?&limit:NULL)))
         state |= y_is_active;
      if (next_event) {
         if (!*next_event || *next_event>limit)
            *next_event = limit;
      }
   }

   if (!state) {
      /* wcal */
      for_each (wc, wcal) {
         if ((w_is_active = is_week_entry_active(tm, wc, next_event?&limit:NULL)))
            state |= w_is_active;
         if (next_event) {
            if (!*next_event || *next_event>limit)
               *next_event = limit;
         }
      }
   } 
   
   DPRINTF(("got state %d from %s calendar. Now: "u32" Next event: "u32"\n", 
         state, (w_is_active==-1)?"year":"week", now, next_event?*next_event:0));

   lFreeElem(tm);

   if ((state & QI_DO_ENABLE)) {
      DEXIT;
      return QI_DO_ENABLE;
   }
   if ((state & QI_DO_SUSPEND)) {
      DEXIT;
      return QI_DO_SUSPEND;
   }
   if ((state & QI_DO_DISABLE)) {
      DEXIT;
      return QI_DO_DISABLE;
   }

   DEXIT;
   return QI_DO_ENABLE;
}

/* returns state and time when state changes acording this entry */
/*
lListElem *tm,          TM_Type 
lListElem *week_entry,  CA_Type 
*/
static u_long32 
is_week_entry_active(lListElem *tm, lListElem *week_entry, 
                     time_t *limit) 
{
   u_long32 state;
   int in_wday_range, in_daytime_range = 0;

   DENTER(TOP_LAYER, "is_week_entry_active");

   /* compute state */
   if ((in_wday_range=in_range_list(tm, lGetList(week_entry, CA_wday_range_list), tm_wday_cmp)) 
     && (in_daytime_range=in_range_list(tm, lGetList(week_entry, CA_daytime_range_list), tm_daytime_cmp))) {
       /* DPRINTF(("in_range_list(wday) = %d in_range_list(daytime) = %d state = %d\n", 
   in_wday_range, in_daytime_range, lGetUlong(week_entry, CA_state))); */
      state = lGetUlong(week_entry, CA_state);
   } else {
      /* DPRINTF(("in_range_list(wday) = %d in_range_list(daytime) = %d\n", in_wday_range, in_daytime_range)); */
      state = 0;
   }

   if (limit) {  
      *limit = compute_limit(in_wday_range, in_daytime_range, 
            lGetList(week_entry, CA_daytime_range_list), tm);
   }

   DEXIT;
   return state;
}

/* returns state and time when state changes acording this entry */
/*
lListElem *tm,         TM_Type 
lListElem *year_entry, CA_Type 
*/
static u_long32 
is_year_entry_active(lListElem *tm, lListElem *year_entry, time_t *limit) 
{
   u_long32 state;
   int in_yday_range, in_daytime_range = 0;

   DENTER(TOP_LAYER, "is_year_entry_active");

   /* compute state */
   if ((in_yday_range=in_range_list(tm, lGetList(year_entry, CA_yday_range_list), tm_yday_cmp)) 
     && (in_daytime_range=in_range_list(tm, lGetList(year_entry, CA_daytime_range_list), tm_daytime_cmp))) {
      /* DPRINTF(("in_range_list(yday) = %d in_range_list(daytime) = %d state = %d\n", 
            in_yday_range, in_daytime_range, lGetUlong(year_entry, CA_state))); */
      state = lGetUlong(year_entry, CA_state);
   } else {
      /* DPRINTF(("in_range_list(yday) = %d in_range_list(daytime) = %d\n", in_yday_range, in_daytime_range)); */
      state = in_daytime_range?QI_DO_ENABLE:0;
   }

   if (limit) {  
      *limit = compute_limit(in_yday_range, in_daytime_range, 
            lGetList(year_entry, CA_daytime_range_list), tm);
   }

   DEXIT;
   return state;
}

static time_t 
compute_limit(int today, int active, lList *dtrl, lListElem *now) 
{
   lListElem *lep = NULL;
   int end_of_day;
   struct tm tm_limit;
   time_t limit;

   DENTER(TOP_LAYER, "compute_limit");

   /* compute limit */
   if (today) {
      lListElem *r;
      if (active) {
         /* inside of range - seek nearest end of range */
         /* seek end of active daytime range */ 
         for_each (r, dtrl) {
            if (in_range(now, r, tm_daytime_cmp)) {
               lep = lCopyElem(lFirst(lGetList(r, TMR_end))); 
               break;
            }
         }
      } else {
         lListElem *r_begin;
         /* outside of range - seek nearest begin of range */
         for_each (r, dtrl) {
            r_begin = lFirst(lGetList(r, TMR_begin));
            if (tm_daytime_cmp(now, r_begin)<0 && 
               (!lep || tm_daytime_cmp(lep, r_begin)>0)) {
               lep = lFreeElem(lep); 
               lep = lCopyElem(r_begin); 
            }
         }
      }
   } 
   
   /* default limit */
   if (!lep) {
/*       DPRINTF(("NO LIMIT\n")); */
      lep = lCopyElem(now);
      lSetUlong(lep, TM_hour, 24);
      lSetUlong(lep, TM_min, 0);
      lSetUlong(lep, TM_sec, 0);
   }
   /* DPRINTF(("Next event at %d:%d:%d\n", 
      lGetUlong(lep, TM_hour),
      lGetUlong(lep, TM_min),
      lGetUlong(lep, TM_sec))); */

   /* convert from cull struct tm into time_t */
   if ((end_of_day = (lGetUlong(lep, TM_hour)==24))) {
      lSetUlong(lep, TM_hour, 23);
      lSetUlong(lep, TM_min, 59);
      lSetUlong(lep, TM_sec, 59);
   }
   uncullify_tm(now, &tm_limit);
   tm_limit.tm_hour = lGetUlong(lep, TM_hour);
   tm_limit.tm_min = lGetUlong(lep, TM_min);
   tm_limit.tm_sec = lGetUlong(lep, TM_sec);
   lep = lFreeElem(lep);

   /* DPRINTF(("sec: %d min: %d hour: %d mday: %d mon: %d year:%d wday: %d yday: %d isdst: %d\n",
      tm_limit.tm_sec,
      tm_limit.tm_min,
      tm_limit.tm_hour,
      tm_limit.tm_mday,
      tm_limit.tm_mon,
      tm_limit.tm_year,
      tm_limit.tm_wday,
      tm_limit.tm_yday,
      tm_limit.tm_isdst)); */

   limit = mktime(&tm_limit);

/*    DPRINTF(("limit: "u32"\n", limit)); */
   if (end_of_day)
      limit += 1;

   DEXIT;
   return limit;
}

static int 
normalize_range_list(lList *rl, cmp_func_t cmp_func) 
{
   lListElem *r1, *r2, *r;
   lListElem *q1, *q2, *q;
   lListElem *t1, *t2, *t3, *t4;
   int i1, i2, i3, i4;

   DENTER(TOP_LAYER, "normalize_range_list");

   i1 = i2 = i3 = i4 = -1;

   for_each(r, rl) {

      r1 = lFirst(lGetList(r, TMR_begin));
      r2 = lFirst(lGetList(r, TMR_end));

      for (q = lNext(r); q; q = lNext(q)) {

         q1 = lFirst(lGetList(q, TMR_begin));
         q2 = lFirst(lGetList(q, TMR_end));

         /* overlapping ranges ? */
         if ((i1=in_range(r1, q, cmp_func))         || /* r1 in q */
             (i2=(r2 && in_range(r2, q, cmp_func))) || /* r2 in q */
             (i3=in_range(q1, r, cmp_func))         || /* q1 in r */
             (i4=(q2 && in_range(q2, r, cmp_func))))   /* q2 in r */ {

/*             DPRINTF(("%d %d %d %d\n", i1,i2,i3,i4)); */
            t1=lFirst(lGetList(r, TMR_begin));
            t2=lFirst(lGetList(r, TMR_end));
            t3=lFirst(lGetList(q, TMR_begin));
            t4=lFirst(lGetList(q, TMR_end));

            DPRINTF(("overlapping ranges %d:%d:%d-%d:%d:%d and %d:%d:%d-%d:%d:%d\n",
               lGetUlong(t1, TM_hour),
               lGetUlong(t1, TM_min),
               lGetUlong(t1, TM_sec),

               lGetUlong(t2, TM_hour),
               lGetUlong(t2, TM_min),
               lGetUlong(t2, TM_sec),

               lGetUlong(t3, TM_hour),
               lGetUlong(t3, TM_min),
               lGetUlong(t3, TM_sec),

               lGetUlong(t4, TM_hour),
               lGetUlong(t4, TM_min),
               lGetUlong(t4, TM_sec)));

            /* combine both overlapping ranges */
            /* r1 = min(r1, q1) */
            if (cmp_func(r1, q1)>0)
               lSwapList(r, TMR_begin, q, TMR_begin);

            /* r2 = max(r2, q2) */
            if ( (q2 && !r2) || cmp_func(r2, q2)<0)
               lSwapList(r, TMR_end, q, TMR_end);
            
            t1=lFirst(lGetList(r, TMR_begin));
            t2=lFirst(lGetList(r, TMR_end));

            DPRINTF(("resulting range %d:%d:%d-%d:%d:%d\n",
               lGetUlong(t1, TM_hour),
               lGetUlong(t1, TM_min),
               lGetUlong(t1, TM_sec),

               lGetUlong(t2, TM_hour),
               lGetUlong(t2, TM_min),
               lGetUlong(t2, TM_sec)));

            /* remove q */
            lRemoveElem(rl, q);

            /* prepare next loop iteration */
            q = r;

            /* r1, r2 are no longer valid after swapping */
            r1 = lFirst(lGetList(r, TMR_begin));
            r2 = lFirst(lGetList(r, TMR_end));
         } 
      }
   }

   DEXIT;
   return 0;
}

/*
lListElem *tm, TM_Type 
*/
static int 
in_range_list(lListElem *tm, lList *rl, cmp_func_t cmp_func) 
{
   lListElem *r;

   DENTER(TOP_LAYER, "in_range_list");

   if (!rl) {
      DEXIT;
      return 1;
   }

   for_each(r, rl) {
      if (in_range(tm, r, cmp_func)) {
         DEXIT;
         return 1;
      }
   }

   DEXIT;
   return 0;
}

/*
lListElem *tm, TM_Type
*/
static int 
in_range(lListElem *tm, lListElem *r, cmp_func_t cmp_func) 
{
   lListElem *t1, *t2;

   DENTER(TOP_LAYER, "in_range");

   t1 = lFirst(lGetList(r, TMR_begin));
   t2 = lFirst(lGetList(r, TMR_end));

   if (t2) {
      if (cmp_func(t1, tm)<=0 && cmp_func(tm, t2)<=0) { /* mon-fri */
#if 0
      DPRINTF(("%d:%d:%d is in range %d:%d:%d-%d:%d:%d\n",
         lGetUlong(tm, TM_hour),
         lGetUlong(tm, TM_min),
         lGetUlong(t1, TM_sec),

         lGetUlong(t1, TM_hour),
         lGetUlong(t1, TM_min),
         lGetUlong(t1, TM_sec),

         lGetUlong(t2, TM_hour),
         lGetUlong(t2, TM_min),
         lGetUlong(t2, TM_sec)));
#endif
         DEXIT;
         return 1;
      }

   } else {
      if (!cmp_func(t1, tm)) { /* must be equal for hit */
         DEXIT;
         return 1;
      }
   }

   DEXIT;
   return 0;
}

/* disabled_year_list := disabled_year_entry[<space>disabled_year_entry] */
static int 
disabled_year_list(lList **alpp, const char *s, lList **cal, 
                   const char *cal_name) { 
   lListElem *calep;

   static token_set_t token_set[] = {
      { DOT,             "." }, 
      { COLON,           ":" }, 
      { EQUAL_SIGN,      "=" },
      { MINUS,           "-" },
      { COMMA,           "," },
      { SPACE,           " " },
      { STRING,          NULL },
      { NUMBER,          NULL },
      { 0,               NULL }
   };

   DENTER(TOP_LAYER, "disabled_year_list");

   if (cal)
      *cal = NULL;

   if (!s || !strcasecmp(s, "none")) {
      DEXIT;
      return 0;
   }

   scan(s, token_set);

   if (disabled_year_entry(cal?&calep:NULL))
      goto ERROR;

   if (cal) {
      *cal = lCreateList("year list", CA_Type);
      lAppendElem(*cal, calep);
   }
   while (scan(NULL, NULL)==SPACE) {
      eat_token();

      if (disabled_year_entry(cal?&calep:NULL))
         goto ERROR;
      if (cal)
         lAppendElem(*cal, calep);
   }  

   /* complain about still unused tokens */
   if (scan(NULL, NULL)!=NO_TOKEN) {
      sprintf(parse_error, MSG_TOKEN_UNRECOGNIZEDSTRING_S , get_string());
      goto ERROR;
   }

   DEXIT;
   return 0;

ERROR:
   if (cal)
      lFreeList(*cal);
   SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ANSWER_ERRORINDISABLYEAROFCALENDARXY_SS, 
         save_error(), cal_name));
   answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
   DEXIT;
   return -1;
}

/* disabled_year_entry := year_day_range_list[=daytime_range_list][=state] */
static int 
disabled_year_entry(lListElem **cal) 
{
   lList *ydrl = NULL,
         *dtrl = NULL;
   int state = QI_DO_DISABLE;
  
   DENTER(TOP_LAYER, "disabled_year_entry");

   if (scan(NULL, NULL)==NUMBER) {
      if (year_day_range_list(&ydrl))
         goto ERROR;
     
      if (scan(NULL, NULL)!=EQUAL_SIGN)
         goto SUCCESS;
      eat_token();
   }

   if (scan(NULL, NULL)==NUMBER) {
      if (daytime_range_list(&dtrl))
         goto ERROR;
      normalize_range_list(dtrl, tm_daytime_cmp);
      if (scan(NULL, NULL)!=EQUAL_SIGN)
         goto SUCCESS;
      eat_token();
   }
   
   if (scan(NULL, NULL)==STRING) {
      if (action(&state)) {
         DEXIT;
         return -1;
      }
   } else {
      sprintf(parse_error, MSG_ANSWER_GOTEQUALWITHOUTDAYTIMERANGEORSTATE );
      goto ERROR;
   }

SUCCESS:
   if (cal) {
      *cal = lCreateElem(CA_Type);
      lSetList(*cal, CA_yday_range_list, ydrl);
      if (!dtrl)
         full_daytime_range(&dtrl);
      lSetList(*cal, CA_daytime_range_list, dtrl);
      DPRINTF(("state = %d\n", state));
      lSetUlong(*cal, CA_state, state);
   }

   DEXIT;
   return 0;

ERROR:
   lFreeList(ydrl);
   lFreeList(dtrl);
   DEXIT;
   return -1;
}

static void 
full_daytime_range(lList **dtrl) 
{
   lListElem *tmr;

   DENTER(TOP_LAYER, "full_daytime_range");

   if (!*dtrl)
      *dtrl = lCreateList("full day", TMR_Type);

/*    DPRINTF(("FULL_DAYTIME_RANGE\n")); */

   tmr = lCreateElem(TMR_Type);
   lAddSubUlong(tmr, TM_hour, 0,  TMR_begin, TM_Type);
   lAddSubUlong(tmr, TM_hour, 24, TMR_end,   TM_Type);

   lAppendElem(*dtrl, tmr);   

   DEXIT;
   return;
}

static void 
full_weekday_range(lList **dtrl) 
{
   lListElem *tmr;

   DENTER(TOP_LAYER, "full_weekday_range");

   if (!*dtrl)
      *dtrl = lCreateList("full week", TMR_Type);

/*    DPRINTF(("FULL_WEEKDAY_RANGE\n")); */

   tmr = lCreateElem(TMR_Type);
   lAddSubUlong(tmr, TM_wday, 0,  TMR_begin, TM_Type);
   lAddSubUlong(tmr, TM_wday, 6, TMR_end,   TM_Type);

   lAppendElem(*dtrl, tmr);   

   DEXIT;
   return;
}


/* year_day_range := year_day_range[,year_day_range] */
static int 
year_day_range_list(lList **ydrl) 
{ 
   lListElem *tmr;

   DENTER(TOP_LAYER, "year_day_range_list");

   if (year_day_range(&tmr)) {
      DEXIT;
      return -1;
   }
   if (ydrl) {
      *ydrl = lCreateList("year_day_range_list", TMR_Type);
      lAppendElem(*ydrl, tmr);
   }

   while (scan(NULL, NULL)==COMMA) {
      eat_token();
      if (year_day_range(&tmr)) {
         if (ydrl)
            *ydrl = lFreeList(*ydrl);
         DEXIT;
         return -1;
      }
      if (ydrl)
         lAppendElem(*ydrl, tmr);
   }

   DEXIT;
   return 0;
}

static int 
year_day_range(lListElem **tmr) 
{ 
   lListElem *t1, *t2 = NULL;
 
   DENTER(TOP_LAYER, "year_day_range");

   if (year_day(&t1)) {
      DEXIT;
      return -1;
   }

   if (scan(NULL, NULL)==MINUS) {   
      eat_token();

      if (year_day(&t2)) {
         lFreeElem(t1);
         DEXIT;
         return -1;
      }   
      if (tm_yday_cmp(t1, t2)>0) {
         sprintf(parse_error, MSG_ANSWER_FIRSTYESTERDAYINRANGEMUSTBEBEFORESECONDYESTERDAY );
         lFreeElem(t1);
         DEXIT;
         return -1;   
      }
   }

   if (tmr) {
      lList *tmlp;
      *tmr = lCreateElem(TMR_Type);

      tmlp = lCreateList("tm_list", TM_Type);
      lAppendElem(tmlp, t1);
      t1 = NULL;
      lSetList(*tmr, TMR_begin, tmlp);

      if (t2) {
         tmlp = lCreateList("tm_list", TM_Type);
         lAppendElem(tmlp, t2);
         t2 = NULL;
         lSetList(*tmr, TMR_end, tmlp);
      }
   }

   lFreeElem(t1);
   lFreeElem(t2);


   DEXIT;
   return 0;
}

static int 
year_day(lListElem **tm) 
{ 
   int y, m, d;

   DENTER(TOP_LAYER, "year_day");

   if (day(&d)) {
      DEXIT;
      return -1;
   }

   if (scan(NULL, NULL)!=DOT) {   
      sprintf(parse_error, MSG_PARSE_MISSINGPOINTAFTERDAY);
      DEXIT;
      return -1;
   }
   eat_token();

   if (month(&m)) {
      DEXIT;
      return -1;
   }

   if (scan(NULL, NULL)!=DOT) {   
      sprintf(parse_error, MSG_PARSE_MISSINGPOINTAFTERMONTH );
      DEXIT;
      return -1;
   }
   eat_token();

   if (year(&y)) {
      DEXIT;
      return -1;
   }

   if (tm) {
      *tm = lCreateElem(TM_Type);
      lSetUlong(*tm, TM_year, y - 1900);
      lSetUlong(*tm, TM_mon, m);
      lSetUlong(*tm, TM_mday, d);
   }

   DEXIT;
   return 0;
}

static int 
range_number(int min, int max, int *ip, const char *name) {
   DENTER(TOP_LAYER, "range_number");

   if (scan(NULL, NULL)==NUMBER) {
      int this_number = get_number();
      eat_token();

      if (this_number > max || this_number < min) {
         sprintf(parse_error, MSG_PARSE_WOUTSIDEOFRANGEXYZ_SIIS, 
               get_string(),  min, max, name);
         DEXIT;
         return -1;   
      } else {
         if (ip)
            *ip = this_number; 
         DEXIT;
         return 0;
      }
   }

   sprintf(parse_error, MSG_PARSE_XISNOTAY_SS , get_string(), name);
   DEXIT;
   return -1;
}


static int 
month(int *mp) 
{
   int m;

   static token_set_t monthv[] = {
      { 1,  "january" },
      { 2,  "february" },
      { 3,  "march" },
      { 4,  "april" },
      { 5,  "may" },
      { 6,  "june" },
      { 7,  "july" },
      { 8,  "august" },
      { 9,  "september" },
      { 10, "october" },
      { 11, "november" },
      { 12, "december" },
      { -1, NULL }
   };

   DENTER(TOP_LAYER, "month");

   if (scan(NULL, NULL)==STRING) {
      char *s = get_string();
      if ((m = cheap_scan(s, monthv, 3, "month"))<0) {
         DEXIT;
         return -1;
      }
      eat_token();
   } else {
      if (range_number(1, 12, &m, "month")<0) {
         DEXIT;
         return -1;
      }
   }
   *mp = m - 1;

   DEXIT;
   return 0;
}

static int 
day(int *dp) 
{ 
   return range_number(1, 31, dp, "day");
}

static int 
year(int *yp) 
{ 
   return range_number(1970, 2037, yp, "year"); 
}

/* daytime_range_list   := daytime_range[,daytime_range] ... */
static int 
daytime_range_list(lList **dtrl) 
{ 
   lListElem *tmr;

   DENTER(TOP_LAYER, "daytime_range_list");

   if (daytime_range(&tmr)) {
      DEXIT;
      return -1;
   }
   if (dtrl) {
      *dtrl = lCreateList("daytime_range_list", TMR_Type);
      lAppendElem(*dtrl, tmr);
      split_daytime_range(*dtrl, tmr);   
   }

   while (scan(NULL, NULL)==COMMA) {
      eat_token();
      if (daytime_range(&tmr)) {
         if (dtrl)
            *dtrl = lFreeList(*dtrl);
         DEXIT;
         return -1;
      }
      if (dtrl) {
         lAppendElem(*dtrl, tmr);
         split_daytime_range(*dtrl, tmr);   
      }
   }

   DEXIT;
   return 0;
}

static void 
split_daytime_range(lList *dtrl, lListElem *tmr) 
{
   lListElem *t2, *t1, *t3, *t4, *tmr2;

   DENTER(TOP_LAYER, "split_daytime_range");

   if ((t2=lFirst(lGetList(tmr, TMR_end)))) {
      t1=lFirst(lGetList(tmr, TMR_begin));

      if (tm_daytime_cmp(t1, t2)>0) {
         /* split it into two ranges */
         tmr2 = lCreateElem(TMR_Type);
         lAddSubUlong(tmr2, TM_hour, 0, TMR_begin, TM_Type);
         lAddSubUlong(tmr2, TM_hour, 24, TMR_end, TM_Type);
         lSwapList(tmr, TMR_end, tmr2, TMR_end);
         lAppendElem(dtrl, tmr2);
      
         t1=lFirst(lGetList(tmr, TMR_begin));
         t2=lFirst(lGetList(tmr, TMR_end));
         t3=lFirst(lGetList(tmr2, TMR_begin));
         t4=lFirst(lGetList(tmr2, TMR_end));

         DPRINTF(("splitted %d:%d:%d-%d:%d:%d into %d:%d:%d-%d:%d:%d and %d:%d:%d-%d:%d:%d\n",
            lGetUlong(t1, TM_hour),
            lGetUlong(t1, TM_min),
            lGetUlong(t1, TM_sec),

            lGetUlong(t4, TM_hour),
            lGetUlong(t4, TM_min),
            lGetUlong(t4, TM_sec),

            lGetUlong(t1, TM_hour),
            lGetUlong(t1, TM_min),
            lGetUlong(t1, TM_sec),

            lGetUlong(t2, TM_hour),
            lGetUlong(t2, TM_min),
            lGetUlong(t2, TM_sec),

            lGetUlong(t3, TM_hour),
            lGetUlong(t3, TM_min),
            lGetUlong(t3, TM_sec),

            lGetUlong(t4, TM_hour),
            lGetUlong(t4, TM_min),
            lGetUlong(t4, TM_sec)));
      }
   }

   DEXIT;
   return;
}

/* daytime_range  := daytime-daytime */
static int 
daytime_range(lListElem **tmr) 
{
   lListElem *t1 = NULL, *t2 = NULL;
  
   DENTER(TOP_LAYER, "daytime_range");

   if (daytime(&t1)) {
      DEXIT;
      goto ERROR;
   }
   if (scan(NULL, NULL)!=MINUS) {
      sprintf(parse_error, MSG_PARSE_MISSINGDASHINDAYTIMERANGE);   
      goto ERROR;
   }
   eat_token();
   if (daytime(&t2)) {
      goto ERROR;
   }
   if (!tm_daytime_cmp(t1, t2)) {
      sprintf(parse_error, MSG_PARSE_RANGEBEGISEQUALTOEND);   
      goto ERROR;
   }

   if (tmr) {
      lList *tmlp;

      *tmr = lCreateElem(TMR_Type);

      tmlp = lCreateList("tm_list", TM_Type);
      lAppendElem(tmlp, t1);
      t1 = NULL;
      lSetList(*tmr, TMR_begin, tmlp);

      tmlp = lCreateList("tm_list", TM_Type);
      lAppendElem(tmlp, t2);
      t2 = NULL;
      lSetList(*tmr, TMR_end, tmlp);
   }

   lFreeElem(t1);
   lFreeElem(t2);

   DEXIT;
   return 0;

ERROR:
   lFreeElem(t1);
   lFreeElem(t2);
   DEXIT;
   return -1;
}


/* daytime := hour[:minute][:second] */
static int 
daytime(lListElem **tm) 
{ 
   int h, m = 0, s = 0;

   DENTER(TOP_LAYER, "daytime");

   if (hour(&h)) {
      DEXIT;
      return -1;
   } 

   if (scan(NULL, NULL)!=COLON) {
      goto SUCCESS;
   }
   eat_token();
   if (minute(&m)) {
      DEXIT;
      return -1;
   }

   if (scan(NULL, NULL)!=COLON) {
      goto SUCCESS;
   }
   eat_token();
   if (seconds(&s)) {
      DEXIT;
      return -1;
   }

SUCCESS:
   if (h==24) {
      if (m || s) {
         sprintf(parse_error, MSG_PARSE_DAYTIMESBEYOND24HNOTALLOWED);
         DEXIT;
         return -1;
      }
   }

   if (tm) {
      *tm = lCreateElem(TM_Type);
      lSetUlong(*tm, TM_hour, h);
      lSetUlong(*tm, TM_min, m);
      lSetUlong(*tm, TM_sec, s);
   }

   DEXIT;
   return 0;
}

static int 
hour(int *hp) 
{ 
   return range_number(0, 24, hp, MSG_PARSE_HOURSPEC); 
}

static int 
minute(int *mp) 
{ 
   return range_number(0, 59, mp, MSG_PARSE_MINUTESPEC); 
}

static int 
seconds(int *sp) 
{
   return range_number(0, 59, sp, MSG_PARSE_SECONDSSPEC); 
}

static int 
action(int *sp) 
{
   int state;
   char *s;
   DENTER(TOP_LAYER, "action");

   if (scan(NULL, NULL)!=STRING) {
      sprintf(parse_error, MSG_PARSE_XISNOTASTATESPECIFIER_S, get_string());
      DEXIT;
      return -1;
   } 

   s = get_string();
   if ((state = cheap_scan(s, statev, 3, "state specifier"))<0) {
      sprintf(parse_error, MSG_PARSE_XISNOTASTATESPECIFIER_S, s);
      DEXIT;
      return -1;
   }
   eat_token();
   *sp = state;

   DEXIT;
   return 0;
}

static int 
disabled_week_list(lList **alpp, const char *s, lList **cal, 
                   const char *cal_name) 
{
   lListElem *calep;
   static token_set_t token_set[] = {
      { COLON,        ":" }, 
      { EQUAL_SIGN,   "=" },
      { MINUS,        "-" },
      { COMMA,        "," },
      { SPACE,        " " },
      { STRING,       NULL },
      { NUMBER,       NULL },
      { 0,            NULL }
   };
   
   DENTER(TOP_LAYER, "disabled_week_list");

   if (cal)
      *cal = NULL;

   if (!s || !strcasecmp(s, "none")) {
      DEXIT;
      return 0;
   }

   scan(s, token_set);

   if (disabled_week_entry(cal?&calep:NULL))
      goto ERROR;

   if (cal) {
      *cal = lCreateList("week list", CA_Type);
      lAppendElem(*cal, calep);
   }

   while (scan(NULL, NULL)==SPACE) {
      eat_token();

      if (disabled_week_entry(cal?&calep:NULL))
         goto ERROR;
      if (cal)
         lAppendElem(*cal, calep);
   }  

   /* complain about still unused tokens */
   if (scan(NULL, NULL)!=NO_TOKEN) {
      sprintf(parse_error, MSG_PARSE_UNRECOGNIZEDTOKENATEND);
      goto ERROR;
   }

   DEXIT;
   return 0;

ERROR:
   if (cal)
      lFreeList(*cal);
   SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_PARSE_ERRORINDISABLEDWEEKOFCALENDAR_SS, 
        cal_name, save_error()));
   answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
   DEXIT;
   return -1;
}

/* disabled_week_entry := week_day_range_list[=daytime_range_list][=state] */
static int 
disabled_week_entry(lListElem **cal) 
{
   /* default values */
   lList *wdrl = NULL,
         *dtrl = NULL;
       int state = QI_DO_DISABLE;

   DENTER(TOP_LAYER, "disabled_week_entry");

   if (scan(NULL, NULL)==STRING &&
         cheap_scan(get_string(), statev, 3, "state specifier")<0) {
      if (week_day_range_list(&wdrl)) 
         goto ERROR;
     
      if (scan(NULL, NULL)!=EQUAL_SIGN)
         goto SUCCESS;
      eat_token();
   }

   if (scan(NULL, NULL)==NUMBER) {
      if (daytime_range_list(&dtrl))
         goto ERROR;
      normalize_range_list(dtrl, tm_daytime_cmp);
      if (scan(NULL, NULL)!=EQUAL_SIGN)
         goto SUCCESS;
      eat_token();
   }

   if (scan(NULL, NULL)==STRING) {
      state = 0;
      if (action(&state))
         goto ERROR;
   } else {
      sprintf(parse_error, MSG_ANSWER_GOTEQUALWITHOUTDAYTIMERANGEORSTATE);
      goto ERROR;
   }

SUCCESS:
   if (cal) {
      *cal = lCreateElem(CA_Type);
      if (!wdrl)
         full_weekday_range(&wdrl);
      lSetList(*cal, CA_wday_range_list, wdrl);
      if (!dtrl)
         full_daytime_range(&dtrl);
      lSetList(*cal, CA_daytime_range_list, dtrl);
      lSetUlong(*cal, CA_state, state);
   }

   DEXIT;
   return 0;

ERROR:
   lFreeList(wdrl);
   lFreeList(dtrl);
   DEXIT;
   return -1;
}

/* week_day_range := week_day_range[,week_day_range] */
static int 
week_day_range_list(lList **wdrl) 
{ 
   lListElem *tmr;

   DENTER(TOP_LAYER, "week_day_range_list");

   if (week_day_range(&tmr)) {
      DEXIT;
      return -1;
   }
   if (wdrl) {
      *wdrl = lCreateList("week_day_range_list", TMR_Type);
      lAppendElem(*wdrl, tmr);
      split_wday_range(*wdrl, tmr);
   }

   while (scan(NULL, NULL)==COMMA) {
      eat_token();
      if (week_day_range(&tmr)) {
         if (wdrl)
            *wdrl = lFreeList(*wdrl);
         DEXIT;
         return -1;
      }
      if (wdrl) {
         lAppendElem(*wdrl, tmr);
         split_wday_range(*wdrl, tmr);
      }
   }  

   DEXIT;
   return 0;
}

static void 
split_wday_range(lList *wdrl, lListElem *tmr) 
{
   lListElem *t2, *t1, *t3, *t4, *tmr2;

   DENTER(TOP_LAYER, "split_wday_range");

   if ((t2=lFirst(lGetList(tmr, TMR_end)))) {
      t1=lFirst(lGetList(tmr, TMR_begin));

      if (tm_wday_cmp(t1, t2)>0) {
         /* split it into two ranges */
         tmr2 = lCreateElem(TMR_Type);
         lAddSubUlong(tmr2, TM_wday, 0, TMR_begin, TM_Type);
         lAddSubUlong(tmr2, TM_wday, 6, TMR_end, TM_Type);
         lSwapList(tmr, TMR_end, tmr2, TMR_end);
         lAppendElem(wdrl, tmr2);

         t1=lFirst(lGetList(tmr, TMR_begin));
         t2=lFirst(lGetList(tmr, TMR_end));
         t3=lFirst(lGetList(tmr2, TMR_begin));
         t4=lFirst(lGetList(tmr2, TMR_end));

#if 0
      DPRINTF(("splitted wday %d-%d into %d-%d and %d-%d\n",
         lGetUlong(t1, TM_wday),
         lGetUlong(t4, TM_wday),

         lGetUlong(t1, TM_wday),
         lGetUlong(t2, TM_wday),

         lGetUlong(t3, TM_wday),
         lGetUlong(t4, TM_wday)));
#endif
      }
   }

   DEXIT;
}

/* week_day_range  := week_day[-week_day] */
static int 
week_day_range(lListElem **tmr) 
{
   lListElem *t1 = NULL;
   lListElem *t2 = NULL;

   DENTER(TOP_LAYER, "week_day_range");

   if (week_day(&t1)) 
      goto ERROR;

   if (scan(NULL, NULL)==MINUS) {   
      eat_token();

      if (week_day(&t2))
         goto ERROR;

      if (tm_wday_cmp(t1, t2)==0) {
         sprintf(parse_error, MSG_PARSE_FOUNDUSELESSWEEKDAYRANGE);
         goto ERROR;
      }
   }


   if (tmr) {
      lList *tmlp;
      *tmr = lCreateElem(TMR_Type);

      tmlp = lCreateList("tm_list", TM_Type);
      lAppendElem(tmlp, t1);
      t1 = NULL;
      lSetList(*tmr, TMR_begin, tmlp);

      if (t2) {
         tmlp = lCreateList("tm_list", TM_Type);
         lAppendElem(tmlp, t2);
         t2 = NULL;
         lSetList(*tmr, TMR_end, tmlp);
      }
   }
   lFreeElem(t1);
   lFreeElem(t2);

   DEXIT;
   return 0;

ERROR:
   lFreeElem(t1);
   lFreeElem(t2);
   DEXIT;
   return -1;
}

static int 
week_day(lListElem **tm) 
{
   int wday;
   char *s;

   static token_set_t weekdayv[] = {
      { 0, "sunday" },
      { 1, "monday" },
      { 2, "tuesday" },
      { 3, "wednesday" },
      { 4, "thursday" },
      { 5, "friday" },
      { 6, "saturday" },
      { -1, NULL }
   };

   DENTER(TOP_LAYER, "week_day");

   if (scan(NULL, NULL)!=STRING) {
      sprintf(parse_error, MSG_PARSE_EXPECTEDSTRINGFORWEEKDAY);
      DEXIT;
      return -1;
   }
   s = get_string();
   if ((wday = cheap_scan(s, weekdayv, 3, "weekday"))<0) {
      sprintf(parse_error, MSG_PARSE_XISNOTAWEEKDAY_S, s);
      DEXIT;
      return -1;
   }
   eat_token();

   if (tm) {
      *tm = lCreateElem(TM_Type);
      lSetUlong(*tm, TM_wday, wday);
   }

   DEXIT;
   return 0;
}

static char *get_string()
{
   return store;
}

static int get_number()
{
   return number;
}
static char *save_error()
{
   strcpy(old_error, parse_error);
   return old_error;
}

/* ------------------------------------------------------------

   eat_token() informs parsing system that the current token
   is proceed

 */
static void 
eat_token()
{
/*    DENTER(TOP_LAYER, "eat_token"); */
/*    DPRINTF(("token \"%s\"\n", store)); */
   token_is_valid = 0;
/*    DEXIT; */
}

static int 
scan(const char *s, token_set_t token_set[]) 
{
#ifdef __INSIGHT__
/* JG: NULL is OK for token_set */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   static const char *t = NULL;
   static token_set_t *ts = NULL;
   static int token;

   int i, j, k, len = 0;
   int found;

   DENTER(CULL_LAYER, "scan");

   if (s) {                     /* initialize scan() with a new string to parse */
      t = s;
      token_is_valid = 0;
      old_error[0] = parse_error[0] = '\0';
   }
   if (token_set)
      ts = token_set;

   if (token_is_valid) {        /* no need for a new token to parse */
      DEXIT;
      return token;
   }

   if (!*t) {
      token_is_valid = 1;
      token = NO_TOKEN;
      DEXIT;
      return token;
   }

   /* try all possible tokens */
   for (i = 0; ts[i].token; i++) {
      switch (ts[i].token) {

      case NUMBER: {
         int old_number = 0;
         /* parse number 0-9* and store it */
         for (j=0; t[j] && isdigit((int) t[j]); j++) {
            number =  old_number * 10 + (t[j]-'0');
            if (number<old_number) {
               ERROR((SGE_EVENT, MSG_PARSE_OVERFLOWERRORWHILEPARSING));
               token = ERR_TOKEN;
               token_is_valid = 1;
               DEXIT;
               return token;
            } else 
               old_number = number;
         }
         if ((found = (j!=0))) {
            len = j; 
            strncpy(store, t, len);
            store[len] = '\0';
         }
         break;
      }

      case STRING:
      default:
         if (ts[i].token == STRING ) { /* strings and shortcut strings */
            /* parse string a-z,A-Z[a-z,A-Z,0-9]* and store it */
            j = k = 0;
            if (isalpha((int)t[j])) {
               store[k++] = t[j];
               for (j++; t[j] && isalnum((int)t[j]); j++) {
                  store[k++] = t[j];
               }
               store[k] = '\0';
            }

            len = j; 
            if (ts[i].token == STRING) {
               found = (j!=0);
               break;
            }
   
            found = !strcasecmp(ts[i].text, store);
             
         } else { /* exact matching strings */
            len = strlen(ts[i].text);
            found = !strncasecmp(ts[i].text, t, len);
            strncpy(store, t, len);
            store[len] = '\0';
         }
         break;
      }
      if (found) {
         t += len;
         token_is_valid = 1;
         DEXIT;
         return (token = ts[i].token);
      }
   }

   /* nothing found */
   token_is_valid = 1;
   token = ERR_TOKEN;
   DEXIT;
   return token;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

static int 
cheap_scan(char *s, token_set_t tokenv[], int n, char *name) 
{
   int i;
   int len;
   int match_all_chars = 0;

   DENTER(TOP_LAYER, "cheap_scan");

   if ((len=strlen(s))<n)
      match_all_chars = 1;

   for (i=0; tokenv[i].text; i++) {
      if (match_all_chars ? 
            !strcasecmp(tokenv[i].text, s):
            !strncasecmp(tokenv[i].text, s, len)) {
/*             DPRINTF(("recognized \"%s\" == \"%s\" with %d\n", 
   tokenv[i].text, s, tokenv[i].token));  */
               
         DEXIT;
         return tokenv[i].token;
      }
   }

   DEXIT;
   return tokenv[i].token;
}

static int 
tm_yday_cmp(lListElem *t1, lListElem *t2) 
{
   int t;

/*   DPRINTF(("tm_yday_cmp(y%d:m%d:d%d, y%d:m%d:d%d)\n", 
      lGetUlong(t1, TM_year),
      lGetUlong(t1, TM_mon),
      lGetUlong(t1, TM_mday),
      lGetUlong(t2, TM_year),
      lGetUlong(t2, TM_mon),
      lGetUlong(t2, TM_mday))); */

   if ((t=(lGetUlong(t1, TM_year) - lGetUlong(t2, TM_year))))
      return t;
   if ((t=(lGetUlong(t1, TM_mon) - lGetUlong(t2, TM_mon))))
      return t;
   return lGetUlong(t1, TM_mday) - lGetUlong(t2, TM_mday);
}

static int 
tm_wday_cmp(lListElem *t1, lListElem *t2) 
{
   return lGetUlong(t1, TM_wday) - lGetUlong(t2, TM_wday);
}

static int 
tm_daytime_cmp(lListElem *t1, lListElem *t2) 
{
   int t;

   if ((t=(lGetUlong(t1, TM_hour) - lGetUlong(t2, TM_hour))))
      return t;
   if ((t=(lGetUlong(t1, TM_min) - lGetUlong(t2, TM_min))))
      return t;
   return lGetUlong(t1, TM_sec) - lGetUlong(t2, TM_sec);
}

static void 
cullify_tm(lListElem *tm_ep, struct tm *tm_now) 
{
   lSetUlong(tm_ep, TM_mday,  tm_now->tm_mday);
   lSetUlong(tm_ep, TM_mon,   tm_now->tm_mon);
   lSetUlong(tm_ep, TM_year,  tm_now->tm_year);
   lSetUlong(tm_ep, TM_sec,   tm_now->tm_sec);
   lSetUlong(tm_ep, TM_min,   tm_now->tm_min);
   lSetUlong(tm_ep, TM_hour,  tm_now->tm_hour);
   lSetUlong(tm_ep, TM_wday,  tm_now->tm_wday);
   lSetUlong(tm_ep, TM_yday,  tm_now->tm_yday);
   lSetUlong(tm_ep, TM_isdst, tm_now->tm_isdst);
}

static void 
uncullify_tm(lListElem *tm_ep, struct tm *tm_now) 
{
   tm_now->tm_mday =  lGetUlong(tm_ep, TM_mday);
   tm_now->tm_mon  =  lGetUlong(tm_ep, TM_mon);
   tm_now->tm_year =  lGetUlong(tm_ep, TM_year);
   tm_now->tm_sec  =  lGetUlong(tm_ep, TM_sec);
   tm_now->tm_min  =  lGetUlong(tm_ep, TM_min);
   tm_now->tm_hour =  lGetUlong(tm_ep, TM_hour);
   tm_now->tm_wday =  lGetUlong(tm_ep, TM_wday);
   tm_now->tm_yday =  lGetUlong(tm_ep, TM_yday);
   tm_now->tm_isdst = lGetUlong(tm_ep, TM_isdst);
}

bool 
calendar_parse_year(lListElem *cal, lList **answer_list) 
{
   int ret = true;
   lList *yc = NULL;

   DENTER(TOP_LAYER, "calendar_parse_year");
   if (disabled_year_list(answer_list, lGetString(cal, CAL_year_calendar), 
                          &yc, lGetString(cal, CAL_name))) {
      ret = false;               
   } else {
      lXchgList(cal, CAL_parsed_year_calendar, &yc);
      lFreeList(yc); 
   }

   DEXIT;
   return ret;
}

bool 
calendar_parse_week(lListElem *cal, lList **answer_list) 
{
   bool ret = true;
   lList *wc = NULL;

   DENTER(TOP_LAYER, "calendar_parse_week");
   if (disabled_week_list(answer_list, lGetString(cal, CAL_week_calendar), 
            &wc, lGetString(cal, CAL_name))) {
      ret = false;
   } else {
      lXchgList(cal, CAL_parsed_week_calendar, &wc);
      lFreeList(wc); 
   }
   DEXIT;
   return ret;
}

u_long32 
calendar_get_current_state_and_end(const lListElem *cep, time_t *then) 
{
   u_long32 new_state;

   DENTER(TOP_LAYER, "calendar_get_current_state_and_end");

   new_state = state_at(sge_get_gmt(),
      cep?lGetList(cep, CAL_parsed_year_calendar):NULL, 
      cep?lGetList(cep, CAL_parsed_week_calendar):NULL,
      then);
   
   switch (new_state) {
   case QI_DO_DISABLE:
      new_state = QI_DO_CAL_DISABLE;
      break;
   case QI_DO_SUSPEND:
      new_state = QI_DO_CAL_SUSPEND;
      break;
   default:
      new_state = QI_DO_NOTHING;
      break;
   }

   DEXIT;
   return new_state;
}

bool 
calendar_is_referenced(const lListElem *calendar, lList **answer_list,
                       const lList *master_cqueue_list)
{
   bool ret = false;
   lListElem *cqueue = NULL;

   for_each(cqueue, master_cqueue_list) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL;

      for_each(qinstance, qinstance_list) {
         if (qinstance_is_calendar_referenced(qinstance, calendar)) {
            const char *calendar_name = lGetString(calendar, CAL_name);
            const char *name = lGetString(qinstance, QU_full_name);

            answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN,
                                    ANSWER_QUALITY_INFO, 
                                    MSG_CALENDAR_REFINQUEUE_SS, 
                                    calendar_name, name);
            ret = true;
            break;
         }
      }
   }
   return ret;
}

