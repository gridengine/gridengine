
/*
 * This file is included as part of the Grid Engine source
 * to provide the strptime C library function for the NEC SX
 * platform which does not have this function. This function
 * has been modified and will not work correctly on other
 * platforms.
 */

/*      $NetBSD: strptime.c,v 1.12 1998/01/20 21:39:40 mycroft Exp $    */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$OpenBSD: strptime.c,v 1.7 2001/08/23 16:32:19 espie Exp $";
#endif /* LIBC_SCCS and not lint */

#if 0
#include <sys/localedef.h>
#endif
#include <langinfo.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#include "tzfile.h"

#ifndef INT_MAX
#include <values.h>
#define INT_MAX MAXINT
#endif

#define _ctloc(x)               (_CurrentTimeLocale->x)

typedef struct {
   char *d_t_fmt;
   char *t_fmt;
   char *d_fmt;
   char *day[7];
   char *abday[7];
   char *mon[12];
   char *abmon[12];
   char *am_pm[2];
} current_time_locale_t;

static current_time_locale_t ctl;
static current_time_locale_t *_CurrentTimeLocale;

/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define _ALT_E                  0x01
#define _ALT_O                  0x02
#define _LEGAL_ALT(x)           { if (alt_format & ~(x)) return (0); }


static  int _conv_num (const char **, int *, int, int);
static  char *local_strptime (const char *, const char *, struct tm *, int);


char *
strptime(buf, fmt, tm)
        const char *buf, *fmt;
        struct tm *tm;
{
	if (_CurrentTimeLocale == NULL) {
		ctl.d_t_fmt = nl_langinfo(D_T_FMT);
		ctl.t_fmt = nl_langinfo(T_FMT);
		ctl.d_fmt = nl_langinfo(D_FMT);
		ctl.day[0] = nl_langinfo(DAY_1);
		ctl.day[1] = nl_langinfo(DAY_2);
		ctl.day[2] = nl_langinfo(DAY_3);
		ctl.day[3] = nl_langinfo(DAY_4);
		ctl.day[4] = nl_langinfo(DAY_5);
		ctl.day[5] = nl_langinfo(DAY_6);
		ctl.day[6] = nl_langinfo(DAY_7);
		ctl.abday[0] = nl_langinfo(ABDAY_1);
		ctl.abday[1] = nl_langinfo(ABDAY_2);
		ctl.abday[2] = nl_langinfo(ABDAY_3);
		ctl.abday[3] = nl_langinfo(ABDAY_4);
		ctl.abday[4] = nl_langinfo(ABDAY_5);
		ctl.abday[5] = nl_langinfo(ABDAY_6);
		ctl.abday[6] = nl_langinfo(ABDAY_7);
		ctl.mon[0] = nl_langinfo(MON_1);
		ctl.mon[1] = nl_langinfo(MON_2);
		ctl.mon[2] = nl_langinfo(MON_3);
		ctl.mon[3] = nl_langinfo(MON_4);
		ctl.mon[4] = nl_langinfo(MON_5);
		ctl.mon[5] = nl_langinfo(MON_6);
		ctl.mon[6] = nl_langinfo(MON_7);
		ctl.mon[7] = nl_langinfo(MON_8);
		ctl.mon[8] = nl_langinfo(MON_9);
		ctl.mon[9] = nl_langinfo(MON_10);
		ctl.mon[10] = nl_langinfo(MON_11);
		ctl.mon[11] = nl_langinfo(MON_12);
		ctl.abmon[0] = nl_langinfo(ABMON_1);
		ctl.abmon[1] = nl_langinfo(ABMON_2);
		ctl.abmon[2] = nl_langinfo(ABMON_3);
		ctl.abmon[3] = nl_langinfo(ABMON_4);
		ctl.abmon[4] = nl_langinfo(ABMON_5);
		ctl.abmon[5] = nl_langinfo(ABMON_6);
		ctl.abmon[6] = nl_langinfo(ABMON_7);
		ctl.abmon[7] = nl_langinfo(ABMON_8);
		ctl.abmon[8] = nl_langinfo(ABMON_9);
		ctl.abmon[9] = nl_langinfo(ABMON_10);
		ctl.abmon[10] = nl_langinfo(ABMON_11);
		ctl.abmon[11] = nl_langinfo(ABMON_12);
		ctl.am_pm[0] = nl_langinfo(AM_STR);
		ctl.am_pm[1] = nl_langinfo(PM_STR);
		_CurrentTimeLocale = &ctl;
	}
        return(local_strptime(buf, fmt, tm, 1));
}

static char *
local_strptime(buf, fmt, tm, initialize)
        const char *buf, *fmt;
        struct tm *tm;
        int initialize;
{
        char c;
        const char *bp;
        int alt_format, i, len;
        static int century, relyear;

        if (initialize) {
                century = TM_YEAR_BASE;
                relyear = -1;
        }

        bp = buf;
        while ((c = *fmt) != '\0') {
                /* Clear `alternate' modifier prior to new conversion. */
                alt_format = 0;

                /* Eat up white-space. */
                if (isspace(c)) {
                        while (isspace(*bp))
                                bp++;

                        fmt++;
                        continue;
                }
                                
                if ((c = *fmt++) != '%')
                        goto literal;


again:          switch (c = *fmt++) {
                case '%':       /* "%%" is converted to "%". */
literal:
                if (c != *bp++)
                        return (NULL);

                break;

                /*
                 * "Alternative" modifiers. Just set the appropriate flag
                 * and start over again.
                 */
                case 'E':       /* "%E?" alternative conversion modifier. */
                        _LEGAL_ALT(0);
                        alt_format |= _ALT_E;
                        goto again;

                case 'O':       /* "%O?" alternative conversion modifier. */
                        _LEGAL_ALT(0);
                        alt_format |= _ALT_O;
                        goto again;
                        
                /*
                 * "Complex" conversion rules, implemented through recursion.
                 */
                case 'c':       /* Date and time, using the locale's format. */
                        _LEGAL_ALT(_ALT_E);
                        if (!(bp = local_strptime(bp, _ctloc(d_t_fmt), tm, 0)))
                                return (NULL);
                        break;

                case 'D':       /* The date as "%m/%d/%y". */
                        _LEGAL_ALT(0);
                        if (!(bp = local_strptime(bp, "%m/%d/%y", tm, 0)))
                                return (NULL);
                        break;
        
                case 'R':       /* The time as "%H:%M". */
                        _LEGAL_ALT(0);
                        if (!(bp = local_strptime(bp, "%H:%M", tm, 0)))
                                return (NULL);
                        break;

                case 'r':       /* The time as "%I:%M:%S %p". */
                        _LEGAL_ALT(0);
                        if (!(bp = local_strptime(bp, "%I:%M:%S %p", tm, 0)))
                                return (NULL);
                        break;

                case 'T':       /* The time as "%H:%M:%S". */
                        _LEGAL_ALT(0);
                        if (!(bp = local_strptime(bp, "%H:%M:%S", tm, 0)))
                                return (NULL);
                        break;

                case 'X':       /* The time, using the locale's format. */
                        _LEGAL_ALT(_ALT_E);
                        if (!(bp = local_strptime(bp, _ctloc(t_fmt), tm, 0)))
                                return (NULL);
                        break;

                case 'x':       /* The date, using the locale's format. */
                        _LEGAL_ALT(_ALT_E);
                        if (!(bp = local_strptime(bp, _ctloc(d_fmt), tm, 0)))
                                return (NULL);
                        break;

                /*
                 * "Elementary" conversion rules.
                 */
                case 'A':       /* The day of week, using the locale's form. */
                case 'a':
                        _LEGAL_ALT(0);
                        for (i = 0; i < 7; i++) {
                                /* Full name. */
                                len = strlen(_ctloc(day[i]));
                                if (strncasecmp(_ctloc(day[i]), bp, len) == 0)
                                        break;

                                /* Abbreviated name. */
                                len = strlen(_ctloc(abday[i]));
                                if (strncasecmp(_ctloc(abday[i]), bp, len) == 0)
                                        break;
                        }

                        /* Nothing matched. */
                        if (i == 7)
                                return (NULL);

                        tm->tm_wday = i;
                        bp += len;
                        break;

                case 'B':       /* The month, using the locale's form. */
                case 'b':
                case 'h':
                        _LEGAL_ALT(0);
                        for (i = 0; i < 12; i++) {
                                /* Full name. */
                                len = strlen(_ctloc(mon[i]));
                                if (strncasecmp(_ctloc(mon[i]), bp, len) == 0)
                                        break;

                                /* Abbreviated name. */
                                len = strlen(_ctloc(abmon[i]));
                                if (strncasecmp(_ctloc(abmon[i]), bp, len) == 0)
                                        break;
                        }

                        /* Nothing matched. */
                        if (i == 12)
                                return (NULL);

                        tm->tm_mon = i;
                        bp += len;
                        break;

                case 'C':       /* The century number. */
                        _LEGAL_ALT(_ALT_E);
                        if (!(_conv_num(&bp, &i, 0, 99)))
                                return (NULL);

                        century = i * 100;
                        break;

                case 'd':       /* The day of month. */
                case 'e':
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_mday, 1, 31)))
                                return (NULL);
                        break;

                case 'k':       /* The hour (24-hour clock representation). */
                        _LEGAL_ALT(0);
                        /* FALLTHROUGH */
                case 'H':
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_hour, 0, 23)))
                                return (NULL);
                        break;

                case 'l':       /* The hour (12-hour clock representation). */
                        _LEGAL_ALT(0);
                        /* FALLTHROUGH */
                case 'I':
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_hour, 1, 12)))
                                return (NULL);
                        break;

                case 'j':       /* The day of year. */
                        _LEGAL_ALT(0);
                        if (!(_conv_num(&bp, &tm->tm_yday, 1, 366)))
                                return (NULL);
                        tm->tm_yday--;
                        break;

                case 'M':       /* The minute. */
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_min, 0, 59)))
                                return (NULL);
                        break;

                case 'm':       /* The month. */
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_mon, 1, 12)))
                                return (NULL);
                        tm->tm_mon--;
                        break;

                case 'p':       /* The locale's equivalent of AM/PM. */
                        _LEGAL_ALT(0);
                        /* AM? */
                        len = strlen(_ctloc(am_pm[0]));
                        if (strncasecmp(_ctloc(am_pm[0]), bp, len) == 0) {
                                if (tm->tm_hour > 12)   /* i.e., 13:00 AM ?! */
                                        return (NULL);
                                else if (tm->tm_hour == 12)
                                        tm->tm_hour = 0;

                                bp += len;
                                break;
                        }
                        /* PM? */
                        len = strlen(_ctloc(am_pm[1]));
                        if (strncasecmp(_ctloc(am_pm[1]), bp, len) == 0) {
                                if (tm->tm_hour > 12)   /* i.e., 13:00 PM ?! */
                                        return (NULL);
                                else if (tm->tm_hour < 12)
                                        tm->tm_hour += 12;

                                bp += len;
                                break;
                        }

                        /* Nothing matched. */
                        return (NULL);

                case 'S':       /* The seconds. */
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_sec, 0, 61)))
                                return (NULL);
                        break;

                case 'U':       /* The week of year, beginning on sunday. */
                case 'W':       /* The week of year, beginning on monday. */
                        _LEGAL_ALT(_ALT_O);
                        /*
                         * XXX This is bogus, as we can not assume any valid
                         * information present in the tm structure at this
                         * point to calculate a real value, so just check the
                         * range for now.
                         */
                         if (!(_conv_num(&bp, &i, 0, 53)))
                                return (NULL);
                         break;

                case 'w':       /* The day of week, beginning on sunday. */
                        _LEGAL_ALT(_ALT_O);
                        if (!(_conv_num(&bp, &tm->tm_wday, 0, 6)))
                                return (NULL);
                        break;

                case 'Y':       /* The year. */
                        _LEGAL_ALT(_ALT_E);
                        if (!(_conv_num(&bp, &i, 0, INT_MAX)))
                                return (NULL);

                        relyear = -1;
                        tm->tm_year = i - TM_YEAR_BASE;
                        break;

                case 'y':       /* The year within the century (2 digits). */
                        _LEGAL_ALT(_ALT_E | _ALT_O);
                        if (!(_conv_num(&bp, &relyear, 0, 99)))
                                return (NULL);
                        break;

                /*
                 * Miscellaneous conversions.
                 */
                case 'n':       /* Any kind of white-space. */
                case 't':
                        _LEGAL_ALT(0);
                        while (isspace(*bp))
                                bp++;
                        break;


                default:        /* Unknown/unsupported conversion. */
                        return (NULL);
                }


        }

        /*
         * We need to evaluate the two digit year spec (%y)
         * last as we can get a century spec (%C) at any time.
         */
        if (relyear != -1) {
                if (century == TM_YEAR_BASE) {
                        if (relyear <= 68)
                                tm->tm_year = relyear + 2000 - TM_YEAR_BASE;
                        else
                                tm->tm_year = relyear + 1900 - TM_YEAR_BASE;
                } else {
                        tm->tm_year = relyear + century - TM_YEAR_BASE;
                }
        }

        return ((char *)bp);
}


static int
_conv_num(buf, dest, llim, ulim)
        const char **buf;
        int *dest;
        int llim, ulim;
{
        *dest = 0;

        if (**buf < '0' || **buf > '9')
                return (0);

        do {
                *dest *= 10;
                *dest += *(*buf)++ - '0';
        } while ((*dest * 10 <= ulim) && **buf >= '0' && **buf <= '9');

        if (*dest < llim || *dest > ulim || isdigit(**buf))
                return (0);

        return (1);
}


#ifdef MODULE_TEST

#include <stdio.h>

int
main(int argc, char **argv)
{
   struct tm t;
   char buf[1024];

   if (argc < 3) {
      fprintf(stderr, "usage: %s date-time format\n", argv[0]);
      exit(1);
   }

   if (strptime(argv[1], argv[2], &t) == NULL) {
      fprintf(stderr, "strptime failed\n");
      exit(2);
   }

   if (strftime(buf, sizeof(buf)-1, argc>3 ? argv[3] : argv[2], &t) <= 0) {
       fprintf(stderr, "strftime failed\n");
       exit(3);
   }

   puts(buf);

   exit(0);
}

#endif /* MODULE_TEST */

