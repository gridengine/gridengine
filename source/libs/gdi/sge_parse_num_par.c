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

/* sge_parse_num_par.c - parse numeric strings containing octal, decimal
 *                   (integer and fixed float) and hex constants as
 *    added field processors describing ranges of processor nodes
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <float.h>


#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_answer.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_parse_num_par.h"
#include "symbols.h"
#include "msg_gdilib.h"


#if defined(IRIX6)
   /* to be independent from irix' compiler options */
#  undef RLIM_INFINITY
#  define  RLIM_INFINITY  0x7fffffffffffffffLL
#elif defined(CRAY)
#  define  RLIM_INFINITY  0
#elif defined(ALPHA)
#  define  RLIM_INFINITY  0x7fffffffffffffffL
#elif defined(HPUX)
#  define  RLIM_INFINITY  0x7fffffff
#elif defined(WIN32NATIVE)
#	define RLIM_INFINITY 0
#elif defined(SOLARIS) && !defined(SOLARIS64)
   /* Solaris 2.5 and 2.6 have different definition for RLIM_INFINITY */
#  undef RLIM_INFINITY
#  define  RLIM_INFINITY  0x7fffffff
#endif

#if !defined(CRAY) && !defined(SOLARIS64)
#  define RLIM_MAX RLIM_INFINITY
#else
#  define RLIM_MAX 0x7fffffffffffffff
#endif

#ifdef WIN32NATIVE
#	define strcasecmp( a, b) stricmp( a, b)
#	define strncasecmp( a, b, n) strnicmp( a, b, n)
#endif

#ifdef SUN4
double strtod(const char *str, char **ptr);
#endif

static u_long32 sge_parse_num_val(sge_rlim_t *rlimp, double *dvalp, 
                                  const char *str, const char *where, 
                                  char *err_str, int err_len);

static double get_multiplier(sge_rlim_t *rlimp, char **dptr,
                             const char *where, char *err_str, int err_len);

static sge_rlim_t add_infinity(sge_rlim_t rlim, sge_rlim_t offset);

/* this string is used for generating error msgs */  
static stringT tmp;

/* -----------------------------------------

NAME
   parse_ulong_val()

DESCR
   is a wrapper around sge_parse_num_val()
   for migration to code that returns an
   error and does not exit() 

   parse_ulong_val() should be used instead 
   of sge_parse_num_val()

PARAM
   uvalp - where to write the parsed value
   type  - one of :
            TYPE_INT   any number
            TYPE_TIM   h:m:s
            TYPE_MEM   [mMkK] postfix
            TYPE_BOO   true or false
            TYPE_ACC   user access to queue
           if type == 0 all above types will get accepted

   s     - string to parse

RETURN
      1 - ok, value in *uvalp is valid
      0 - parsing error

*/
int parse_ulong_val(double *dvalp, u_long32 *uvalp, u_long32 type, 
                    const char *s, char *error_str, int error_len) 
{
   return extended_parse_ulong_val(dvalp,uvalp,type,s,error_str,error_len,1);
}

/* enable_infinity enhancement: if 0 no infinity value is allowed */
int extended_parse_ulong_val(double *dvalp, u_long32 *uvalp, u_long32 type,
                             const char *s, char *error_str, int error_len,
                             int enable_infinity) 
{
   int retval = 0; /* error */
   char dummy[10];
   u_long32 dummy_uval;

   DENTER(CULL_LAYER, "parse_ulong_val");

   if (!s) {
      DEXIT;
      return 0;
   }

   if ( (strcasecmp(s,"infinity") == 0) && 
        (enable_infinity == 0 ) ) {
      if (error_str) {
         strncpy(error_str, MSG_GDI_VALUETHATCANBESETTOINF, error_len); 
         error_str[error_len-1] = '\0';
         return 0;
      } 
   }


   if (!uvalp)
      uvalp = &dummy_uval;

   /*
      here we have to convert from string
      representation of the request value
      into an ulong value
   */
   switch (type) {
   case TYPE_LOG:
      retval = sge_parse_loglevel_val(uvalp, s);
      if (retval != 1) {
         if (error_str) {
            strncpy(error_str, "loglevel value", error_len); 
            error_str[error_len-1] = '\0';
         }
      } 
      break;

   case TYPE_INT:
   case TYPE_TIM:
   case TYPE_MEM:
   case TYPE_BOO:
   case TYPE_DOUBLE:
      /* dirty but isolated .. */
      if (error_str) {
         *uvalp = sge_parse_num_val(NULL, dvalp, s, s, error_str, error_len);
         if (!error_str[0]) /* err msg written ? */
            retval = 1; /* no error */
         else {
            if (type==TYPE_TIM)
               strncpy(error_str, "time value", error_len); 
            else if (type==TYPE_BOO )
               strncpy(error_str, "boolean value", error_len); 
            else
               strncpy(error_str, "memory value", error_len); 
            error_str[error_len-1] = '\0';
         }
      } else {
         dummy[0] = '\0';
         *uvalp = sge_parse_num_val(NULL, dvalp, s, s, dummy, sizeof(dummy));
         if (!dummy[0]) { /* err msg written ? */
            retval = 1; /* no error */
         }        
      }
      break;

   default:
      break;
   }
   DEXIT;
   return retval;
}

/*----------------------------------------------------------------------*/
int sge_parse_loglevel_val(
u_long32 *uval, 
const char *s 
) {
  int retval = 1;

  DENTER(BASIS_LAYER, "sge_parse_loglevel_val");

  if (!s) {
     retval = 0;
  } else {
     if (!strcasecmp("log_crit", s))
        *uval = LOG_CRIT;
     else if (!strcasecmp("log_err", s))
        *uval = LOG_ERR;
     else if (!strcasecmp("log_warning", s))
        *uval = LOG_WARNING;
     else if (!strcasecmp("log_notice", s))
        *uval = LOG_NOTICE;
     else if (!strcasecmp("log_info", s))
        *uval = LOG_INFO;
     else if (!strcasecmp("log_debug", s))
        *uval = LOG_DEBUG;
     else
        retval = 0;
   
  }

  DEXIT;
  return retval;

}



/*-----------------------------------------------------------
 * sge_parse_checkpoint_attr
 *    parse checkpoint "when" string
 * return:
 *    bitmask of checkpoint specifers
 *    0 if attr_str == NULL or nothing set or value may be a time value
 *-----------------------------------------------------------*/
int sge_parse_checkpoint_attr(const char *attr_str) 
{
   int opr;

   DENTER(TOP_LAYER, "sge_parse_checkpoint_attr");

   if (attr_str == NULL) {
      DEXIT;
      return 0;
   }

   /* May be it's a time value */
   if (isdigit((int) *attr_str) || (*attr_str == ':')) {
      DEXIT;
      return 0;
   }

   opr = 0;
   while (*attr_str) {
      if (*attr_str == CHECKPOINT_AT_MINIMUM_INTERVAL_SYM)
         opr = opr | CHECKPOINT_AT_MINIMUM_INTERVAL;
      else if (*attr_str == CHECKPOINT_AT_SHUTDOWN_SYM)
         opr = opr | CHECKPOINT_AT_SHUTDOWN;
      else if (*attr_str == CHECKPOINT_SUSPEND_SYM)
         opr = opr | CHECKPOINT_SUSPEND;
      else if (*attr_str == NO_CHECKPOINT_SYM)
         opr = opr | NO_CHECKPOINT;
      else if (*attr_str == CHECKPOINT_AT_AUTO_RES_SYM)
         opr = opr | CHECKPOINT_AT_AUTO_RES;
      else {
         opr = -1;
         break;   
      }   
      attr_str++;
   }

   DEXIT;
   return opr;
}

const char *get_checkpoint_when(int bitmask) 
{
   int i = 0;
   static char when[32];
   DENTER(TOP_LAYER, "get_checkpoint_string");
   
   if (is_checkpoint_when_valid(bitmask) && !(bitmask & NO_CHECKPOINT)) {
      if (bitmask & CHECKPOINT_SUSPEND) {
         when[i++] = CHECKPOINT_SUSPEND_SYM;
      }
      if (bitmask & CHECKPOINT_AT_SHUTDOWN) {
         when[i++] = CHECKPOINT_AT_SHUTDOWN_SYM;
      }
      if (bitmask & CHECKPOINT_AT_MINIMUM_INTERVAL) {
         when[i++] = CHECKPOINT_AT_MINIMUM_INTERVAL_SYM;
      }
      if (bitmask & CHECKPOINT_AT_AUTO_RES) {
         when[i++] = CHECKPOINT_AT_AUTO_RES_SYM;
      }
   } else {
      when[i++] = NO_CHECKPOINT_SYM;
   } 
   when[i] = '\0'; 

   DEXIT;
   return when;
}

int is_checkpoint_when_valid(int bitmask) 
{
   int ret = 0;
   int mask = 0;
   DENTER(TOP_LAYER, "is_checkpoint_when_valid");

   mask = CHECKPOINT_SUSPEND | CHECKPOINT_AT_SHUTDOWN 
      | CHECKPOINT_AT_MINIMUM_INTERVAL | CHECKPOINT_AT_AUTO_RES;

   if (bitmask == NO_CHECKPOINT 
       || ((bitmask & mask) == bitmask)) {
      ret = 1;
   }

   DEXIT;
   return ret;
}

char *resource_descr(double dval, u_long32 type, char *buffer) 
{
   int secs, minutes, hours, days;
   char c;
   static char text[100];

   DENTER(GDI_LAYER, "resource_descr");

   if (!buffer)
      buffer = text;

   if (dval==DBL_MAX) {
      DEXIT;
      return "infinity";
   }

   switch (type) {
   case TYPE_TIM:
      secs = dval;

      days    = secs/(60*60*24);
      secs   -= days*(60*60*24);

      hours   = secs/(60*60);
      secs   -= hours*(60*60);

      minutes = secs/60;
      secs   -= minutes*60;

      if (days) 
         sprintf(buffer, "%d:%02d:%02d:%02d", days, hours, minutes, secs);
      else
         sprintf(buffer, "%2.2d:%2.2d:%2.2d", hours, minutes, secs);
      break;

   case TYPE_MEM:
      c = '\0';

      if (fabs(dval) >= (double)1024*1024*1024) {
         dval /= 1024*1024*1024;
         c = 'G';
      } else if (fabs(dval) >= (double)1024*1024) {
         dval /= 1024*1024;
         c = 'M';
      } else if (fabs(dval) >= (double)1024) {
         dval /= 1024;
         c = 'K';
      }
      if (c) {
         sprintf(buffer, "%.2f%c", dval, c);
         break;
      }
   default:
      sprintf(buffer, "%f", dval);
      break;
   }

   DEXIT;
   return buffer;
}


/* 
 * compare two sge_rlim_t values  
 * 
 * returns
 * r1 <  r2      < 0   
 * r1 == r2      == 0   
 * r1 >  r2      > 0   
 */
int rlimcmp(sge_rlim_t r1, sge_rlim_t r2) {
   if (r1 == r2)
      return 0;
   if (r1==RLIM_INFINITY)
      return 1;
   if (r2==RLIM_INFINITY)
      return -1;
   return (r1>r2)?1:-1;
}

/* 
 * return rlim multiplied with muli 
 * 
 * if the result would exceed sge_rlim_t
 * the result is set to RLIM_INFINITY
 */
sge_rlim_t mul_infinity(sge_rlim_t rlim, sge_rlim_t muli) 
{
   if (rlim == RLIM_INFINITY ||
       muli == RLIM_INFINITY )
      return RLIM_INFINITY;

   if ((sge_rlim_t)(RLIM_MAX/muli)<rlim)
      rlim = RLIM_INFINITY;
   else
      rlim *= muli;
   return rlim;
}

/* 
 * return rlim added to offset 
 * 
 * if the result would exceed sge_rlim_t
 * the result is set to RLIM_INFINITY
 */
static sge_rlim_t add_infinity(sge_rlim_t rlim, sge_rlim_t offset) 
{
   if (rlim == RLIM_INFINITY ||
       offset == RLIM_INFINITY )
      return RLIM_INFINITY;

   if ((sge_rlim_t)(RLIM_MAX-offset)<rlim)
      rlim = RLIM_INFINITY;
   else
      rlim += offset;
   return rlim;
}

/***********************************************************************
 * get_multiplier -
 *       get a multiplier for number value attribute requests
 *
 *       Attribute requests may look like -l sc=123x,...
 *       with x being one of
 *
 *                k : Multiplier = 1000
 *                K : Multiplier = 1024
 *                m : Multiplier = 1000*1000
 *                M : Multiplier = 1024*1024
 *                g : Multiplier = 1000*1000*1000
 *                G : Multiplier = 1024*1024*1024
 **********************************************************************/

static double get_multiplier(sge_rlim_t *rlimp, char **dptr, 
                             const char *where, char *err_str, int err_len)
{
   double mul = 1;
   *rlimp = 1;

   /* parse for k,K,m,M multipliers at the end of the value
    * string; strtod returns dptr to point to this location
    */
   switch (**dptr) {
   case 'k':
      mul = 1000;
      *rlimp = 1000;
      (*dptr)++;
      break;
   case 'K':
      mul = 1024;
      *rlimp = 1024;
      (*dptr)++;
      break;
   case 'm':
      mul = 1000 * 1000;
      *rlimp = 1000 * 1000;
      (*dptr)++;
      break;
   case 'M':
      mul = 1024 * 1024;
      *rlimp = 1024 * 1024;
      (*dptr)++;
      break;
   case 'g':
      mul = 1000 * 1000 * 1000;
      *rlimp = mul_infinity(mul_infinity(1000, 1000), 1000);
      (*dptr)++;
      break;
   case 'G':
      mul = 1024 * 1024 * 1024;
      *rlimp = mul_infinity(mul_infinity(1024, 1024), 1024);
      (*dptr)++;
      break;
   case ',':                    /* no multiplier */
   case '\0':                   /* no multiplier */
   case '/':                    /* no multiplier */
      break;
   default:
      sprintf(tmp, MSG_GDI_UNRECOGNIZEDVALUETRAILER_SS , *dptr, where);
      strncpy(err_str, tmp, err_len-1);
      err_str[err_len-1] = '\0';
      return 0;
   }

   if ((**dptr != ',') && (**dptr != '\0') && (**dptr != '/')) {
      sprintf(tmp, MSG_GDI_UNEXPECTEDENDOFNUMERICALVALUE_SC , where, **dptr);
      strncpy(err_str, tmp, err_len-1);
      err_str[err_len-1] = '\0';
      return 0;
   }

   return mul;
}

/***********************************************************************
 * sge_parse_num_val -
 *    parse numeric attribute values from command-line
 *
 *    Numeric attribute values may look like -l <attr>=<num><m>,...
 *    with <num> being a decimal (both integer and fixed float),
 *    hex and octal constant. <m> is a multiplier (see get_multiplier
 *    above).
 *
 * !new feature!
 *    
 *    call the good old parse_num_val() with a non null argument for err_str
 *    and it will ++NOT++ abort your daemon.
 *
 *    in case of a parsing error the err_str gets filled with an error
 *    message
 *
 **********************************************************************/

static u_long32 sge_parse_num_val(sge_rlim_t *rlimp, double *dvalp, 
                                  const char *str, const char *where, 
                                  char *err_str, int err_len)
{
   double dummy;
   sge_rlim_t rlim, rlmuli;
   double dval;
   u_long32 ldummy;
   char *dptr;
   double muli;

   DENTER(CULL_LAYER, "sge_parse_num_val");

   if (!rlimp)
      rlimp = &rlim;
   if (!dvalp)
      dvalp = &dval;

   if (err_str)
      err_str[0] = '\0';

   if (!strcasecmp(str, "true")) {
      /* C-language says bool is a numeric. For reasons of simplicity we 
         agree. */
      *dvalp = 1;
      *rlimp = 1;
      DEXIT;
      return 1;
   }
   else if (!strcasecmp(str, "false")) {
      *dvalp = 0;
      *rlimp = 0;
      DEXIT;
      return 0;
   }
   else if (!strcasecmp(str, "infinity")) {
      *dvalp = DBL_MAX;       /* use this for comparing limits */
      *rlimp = RLIM_INFINITY; /* use this for limit setting */
      DEXIT;
#ifndef CRAY      
      return 0xFFFFFFFF;      /* return max ulong in 32 bit */
#else
      return RLIM_MAX;
#endif            
   }
   else if (strchr(str, ':')) {
      /* This is a time value in the format hr:min:sec */

      /* find the hours first */
      double t = strtod(str, &dptr);
      if (t > 0x7fffffff) {
         sprintf(tmp, MSG_GDI_NUMERICALVALUEFORHOUREXCEEDED_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }
      ldummy = (u_long32)(3600 * t);
      *rlimp = (sge_rlim_t)(long)mul_infinity(t, 3600.0);
      *dvalp = 3600 * t;

      if ((*dptr) != ':') {  
         sprintf(tmp, MSG_GDI_NUMERICALVALUEINVALID_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }
      /* now go for the minutes */
      dptr++;
      t = strtod(dptr, &dptr);
      if (t > 0x7fffffff) {
         sprintf(tmp, MSG_GDI_NUMERICALVALUEFORMINUTEEXCEEDED_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }
      ldummy += (u_long32)(60 * t);
      *rlimp = add_infinity(*rlimp, (sge_rlim_t)(60*t));
      *dvalp += 60 * t;

      if ((*dptr) != ':') {
         sprintf(tmp, MSG_GDI_NUMERICALVALUEINVALID_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }
      /* the seconds finally */
      dptr++;

      t = strtod(dptr, &dptr);
      ldummy += (u_long32)t;
      *rlimp = (sge_rlim_t)(long)add_infinity(*rlimp, t);
      *dvalp += t;

      while(*dptr) {
         if (!isspace((int) *dptr)) {
            sprintf(tmp, MSG_GDI_NUMERICALVALUEINVALID_SS , where, str);
            strncpy(err_str, tmp, err_len-1);
            err_str[err_len-1] = '\0';
            return 0;
         }
         dptr++;
      }

      DEXIT;
      return (ldummy);

   }
   else if (strchr(str, '.') || *str != '0') {
      /* obviously this is no hex and no oct
       * ==> allow for both decimal and fixed float
       */
       
      double t = strtod(str, &dptr);
      
      if (t > 0x7fffffff)
         dummy = 0x7fffffff;
      else
         dummy = t;
   
      if (t > RLIM_MAX)
         *rlimp = RLIM_INFINITY;
      else 
         *rlimp = (sge_rlim_t)t;
      *dvalp = t;

      if ((dummy == 0.0) && (dptr == str)) {    /* no valid number ==> bail */
         sprintf(tmp, MSG_GDI_NUMERICALVALUEINVALIDNONUMBER_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }

      /* OK, we got it */
      if (!(muli = get_multiplier(&rlmuli, &dptr, where, err_str, err_len)))
         return 0;
      dummy =    (u_long32) (dummy * muli);
      *dvalp =              *dvalp * muli;
      *rlimp = mul_infinity(*rlimp, rlmuli);

      DEXIT;
      return (u_long32)dummy;

   }
   else {                       /* if ( strchr(str,'.') || *str != '0' ) */
      /* This is either a hex or an octal ==> no fixed float allowed;
       * just use strtol
       */
      u_long32 t = strtol(str, &dptr, 0);   /* base=0 will handle both hex and oct */
      ldummy = t;
      *rlimp = t;
      *dvalp = t;

      if (dptr == str) {        /* no valid number ==> bail */
         sprintf(tmp, MSG_GDI_NUMERICALVALUEINVALIDNOHEXOCTNUMBER_SS , where, str);
         strncpy(err_str, tmp, err_len-1);
         err_str[err_len-1] = '\0';
         DEXIT;
         return 0;
      }
      /* OK, we got it */
      if (!(muli = get_multiplier(&rlmuli, &dptr, where, err_str, err_len)))
         return 0;
      ldummy *= (u_long32)muli;
      *rlimp = mul_infinity(*rlimp, rlmuli);
      *dvalp *= muli;

      DEXIT;
      return (u_long32)ldummy;
   }
}


/* -----------------------------------------

NAME
   sge_parse_limit()

DESCR
   is a wrapper around sge_parse_num_val()
   for migration to code that returns an
   error and does not exit() 

PARAM
   uvalp - where to write the parsed value
   s     - string to parse

RETURN
      1 - ok, value in *uvalp is valid
      0 - parsing error

*/
int sge_parse_limit(sge_rlim_t *rlvalp, char *s, char *error_str, 
                    int error_len) 
{
   DENTER(CULL_LAYER, "sge_parse_limit");

   /*
   ** error handling should be added here, but how ????
   */
   sge_parse_num_val(rlvalp, NULL, s, s, error_str, error_len);

   DEXIT;
   return 1;
}



