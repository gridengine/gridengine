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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "msg_cull.h"
#include "sge_string.h"

#include "sgermon.h"
#include "cull_whereP.h"
#include "cull_parse.h"
#include "cull_lerrnoP.h"

/* -------- intern prototypes --------------------------------- */

static lCondition *read_val(lDescr *dp, va_list *app);
static lCondition *factor(lDescr *dp, va_list *app);
static lCondition *product(lDescr *dp, va_list *app);
static lCondition *negfactor(lDescr *dp, va_list *app);
static lCondition *sum(lDescr *dp, va_list *app);
static lCondition *subscope(va_list *app);

static lCondition *_read_val(lDescr *dp, WhereArgList *wapp);
static lCondition *_factor(lDescr *dp, WhereArgList *wapp);
static lCondition *_product(lDescr *dp, WhereArgList *wapp);
static lCondition *_negfactor(lDescr *dp, WhereArgList *wapp);
static lCondition *_sum(lDescr *dp, WhereArgList *wapp);
static lCondition *_subscope(WhereArgList *wapp);

/* =========== implementation ================================= */

/* ------------------------------------------------------------ 

   comines two conditions cp0 and cp1 
   logically with an "OR"  

 */
lCondition *lOrWhere(
const lCondition *cp0,
const lCondition *cp1 
) {
   lCondition *newcp;

   DENTER(TOP_LAYER, "lOrWhere");

   if (!cp0 || !cp1) {
      LERROR(LECONDNULL);
      DEXIT;
      return NULL;
   }

   if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   newcp->op = OR;
   newcp->operand.log.first = (lCondition *)cp0;
   newcp->operand.log.second = (lCondition *)cp1;

   DEXIT;
   return newcp;
}

/* ------------------------------------------------------------ 

   comines two conditions cp0 and cp1 
   logically with an "AND"  

 */
lCondition *lAndWhere(
const lCondition *cp0,
const lCondition *cp1 
) {
   lCondition *newcp;

   DENTER(TOP_LAYER, "lAndWhere");

   if (!cp0 || !cp1) {
      LERROR(LECONDNULL);
      DEXIT;
      return NULL;
   }

   if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   newcp->op = AND;
   newcp->operand.log.first = (lCondition *)cp0;
   newcp->operand.log.second = (lCondition *)cp1;

   DEXIT;
   return newcp;
}

/* ------------------------------------------------------------ 

   writes a lCondition struct (for debugging purposes)

 */
void lWriteWhereTo(
const lCondition *cp,
FILE *fp 
) {
   int i;
   static int depth = -1;
   char space[80];
   static char out[256];

   DENTER(CULL_LAYER, "lWriteWhere");

   if (!cp) {
      LERROR(LECONDNULL);
      DEXIT;
      return;
   }
   depth++;

   out[0] = '\0';

   space[0] = '\0';
   for (i = 0; i < depth; i++)
      strcat(space, "   ");

   switch (cp->op) {

   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case BITMASK:
   case STRCASECMP:
   case PATTERNCMP:
   case HOSTNAMECMP:
   case SUBSCOPE:

      sprintf(&out[strlen(out)], "%s %s(%d) ", space, 
               lNm2Str(cp->operand.cmp.nm), cp->operand.cmp.nm);

      switch (cp->op) {
      case EQUAL:
         sprintf(&out[strlen(out)], "==");
         break;
      case NOT_EQUAL:
         sprintf(&out[strlen(out)], "!=");
         break;
      case LOWER_EQUAL:
         sprintf(&out[strlen(out)], "<=");
         break;
      case LOWER:
         sprintf(&out[strlen(out)], "<");
         break;
      case GREATER_EQUAL:
         sprintf(&out[strlen(out)], ">=");
         break;
      case GREATER:
         sprintf(&out[strlen(out)], ">");
         break;

      case BITMASK:
         sprintf(&out[strlen(out)], "m=");
         break;

      case STRCASECMP:
         sprintf(&out[strlen(out)], "c=");
         break;

      case PATTERNCMP:
         sprintf(&out[strlen(out)], "p=");
         break;

      case HOSTNAMECMP:
         sprintf(&out[strlen(out)], "h=");
         break;

      case SUBSCOPE:
         if (!fp) {
            DPRINTF(("%s ->\n", out));
         }
         else {
            fprintf(fp, "%s ->\n", out);
         }
         lWriteWhereTo(cp->operand.cmp.val.cp, fp);
         break;

      default:
         LERROR(LEOPUNKNOWN);
         DEXIT;
         return;
      }

      switch (cp->operand.cmp.mt) {
      case lIntT:
         if (!fp) {
            DPRINTF(("%s %d\n", out, cp->operand.cmp.val.i));
         }
         else {
            fprintf(fp, "%s %d\n", out, cp->operand.cmp.val.i);
         }
         break;
      case lUlongT:
         if (!fp) {
            DPRINTF(("%s "u32"\n", out, cp->operand.cmp.val.ul));
         }
         else {
            fprintf(fp, "%s "u32"\n", out, cp->operand.cmp.val.ul);
         }
         break;
      case lStringT:
         if (!fp) {
            DPRINTF(("%s \"%s\"\n", out, cp->operand.cmp.val.str));
         }
         else {
            fprintf(fp, "%s \"%s\"\n", out, cp->operand.cmp.val.str);
         }
         break;
      case lListT:
/*          DPRINTF(("lWriteWhere error\n")); */
         break;
      case lFloatT:
         if (!fp) {
            DPRINTF(("%s %f\n", out, cp->operand.cmp.val.fl));
         }
         else {
            fprintf(fp, "%s %f\n", out, cp->operand.cmp.val.fl);
         }
         break;
      case lDoubleT:
         if (!fp) {
            DPRINTF(("%s %f\n", out, cp->operand.cmp.val.db));
         }
         else {
            fprintf(fp, "%s %f\n", out, cp->operand.cmp.val.db);
         }
         break;
      case lLongT:
         if (!fp) {
            DPRINTF(("%s %ld\n", out, cp->operand.cmp.val.l));
         }
         else {
            fprintf(fp, "%s %ld\n", out, cp->operand.cmp.val.l);
         }
         break;
      case lCharT:
         if (!fp) {
            DPRINTF(("%s %c\n", out, cp->operand.cmp.val.c));
         }
         else {
            fprintf(fp, "%s %c\n", out, cp->operand.cmp.val.c);
         }
         break;
      case lRefT:
         if (!fp) {
            DPRINTF(("%s %p\n", out, cp->operand.cmp.val.ref));
         }
         else {
            fprintf(fp, "%s %p\n", out, cp->operand.cmp.val.ref);
         }
         break;
      default:
         unknownType("lWriteWhere");
         DEXIT;
         return;
      }
      break;

   case AND:
      if (!fp) {
         DPRINTF(("%s(\n", space));
      }
      else {
         fprintf(fp,"%s(\n", space);
      }
      lWriteWhereTo(cp->operand.log.first, fp);
      if (!fp) {
         DPRINTF(("%s&&\n", space));
      }
      else {
         fprintf(fp, "%s&&\n", space);
      }
      lWriteWhereTo(cp->operand.log.second, fp);
      if (!fp) {
         DPRINTF(("%s)\n", space));
      }
      else {
         fprintf(fp, "%s)\n", space);
      }
      break;

   case OR:
      if (!fp) {
         DPRINTF(("%s(\n", space));
      }
      else {
         fprintf(fp, "%s(\n", space);
      }
      lWriteWhereTo(cp->operand.log.first, fp);
      if (!fp) {
         DPRINTF(("%s||\n", space));
      }
      else {
         fprintf(fp, "%s||\n", space);
      }
      lWriteWhereTo(cp->operand.log.second, fp);
      if (!fp) {
         DPRINTF(("%s)\n", space));
      }
      else {
         fprintf(fp, "%s)\n", space);
      }
      break;

   case NEG:
      if (!fp) {
         DPRINTF(("%s!(\n", space));
      }
      else {
         fprintf(fp, "%s!(\n", space);
      }
      lWriteWhereTo(cp->operand.log.first, fp);
      if (!fp) {
         DPRINTF(("%s)\n", space));
      }
      else {
         fprintf(fp, "%s)\n", space);
      }

      break;

   default:
      LERROR(LEOPUNKNOWN);
      DPRINTF(("error: LEOPUNKNOWN\n"));
      DEXIT;
      return;
   }

   depth--;
   DEXIT;
   return;
}

/* ------------------------------------------------------------

   lWhere builds the lCondition tree.
   The condition is stated as a format string and an associated 
   va_alist of the following form:

   %I field ( unequal lList: J_id)
   == comparison operator ( <, >, <=, >=, !=, ==)
   %d value type descriptor (%s string, %d int, %u ulong) : 375

 */
lCondition *lWhere(const char *fmt,...)
{
   lCondition *cond;
   va_list ap;

   DENTER(CULL_LAYER, "lWhere");
   va_start(ap, fmt);

   if (!fmt) {
      LERROR(LENOFORMATSTR);
      DEXIT;
      return NULL;
   }
   /* 
      initialize scan function, the actual token is scanned again
      in the subscope fuction. There we call eat_token to go ahead
    */
   scan(fmt);

   /* parse */
   cond = subscope(&ap);

   if (!cond) {
      LERROR(LEPARSECOND);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return cond;
}

static lCondition *subscope(
va_list *app 
) {
   lDescr *dp = NULL;
   lCondition *cp = NULL;

   DENTER(CULL_LAYER, "subscope");

   if (scan(NULL) != TYPE) {
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat %T */
   if (!(dp = va_arg(*app, lDescr *))) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (scan(NULL) != BRA) {
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat ( */

   if (!(cp = sum(dp, app))) {
      LERROR(LESUM);
      DEXIT;
      return NULL;
   }

   if (scan(NULL) != KET) {
      LERROR(LESYNTAX);
      lFreeWhere(cp);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat ) */

   DEXIT;
   return cp;
}

static lCondition *sum(
lDescr *dp,
va_list *app 
) {
   lCondition *cp, *newcp;

   DENTER(CULL_LAYER, "sum");

   cp = product(dp, app);

   while (scan(NULL) == OR) {
      eat_token();              /* eat || */

      if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         LERROR(LEMALLOC);
         lFreeWhere(cp);
         DEXIT;
         return NULL;
      }
      newcp->op = OR;
      newcp->operand.log.first = cp;
      newcp->operand.log.second = product(dp, app);
      cp = newcp;
   }

   DEXIT;
   return cp;
}

static lCondition *product(
lDescr *dp,
va_list *app 
) {
   lCondition *cp, *newcp;

   DENTER(CULL_LAYER, "product");

   cp = factor(dp, app);

   while (scan(NULL) == AND) {
      eat_token();

      if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         lFreeWhere(cp);
         LERROR(LEMALLOC);
         DEXIT;
         return NULL;
      }
      newcp->op = AND;
      newcp->operand.log.first = cp;
      newcp->operand.log.second = factor(dp, app);
      cp = newcp;
   }

   DEXIT;
   return cp;
}

static lCondition *factor(
lDescr *dp,
va_list *app 
) {
   lCondition *cp;

   DENTER(CULL_LAYER, "factor");

   if (scan(NULL) == NEG) {
      eat_token();

      if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         lFreeWhere(cp);
         LERROR(LEMALLOC);
         DEXIT;
         return NULL;
      }
      cp->operand.log.first = negfactor(dp, app);
      cp->operand.log.second = NULL;
      cp->op = NEG;
   }
   else
      cp = negfactor(dp, app);

   DEXIT;
   return cp;
}

static lCondition *negfactor(
lDescr *dp,
va_list *app 
) {
   lCondition *cp;

   DENTER(CULL_LAYER, "negfactor");

   if (scan(NULL) == BRA) {
      eat_token();

      cp = sum(dp, app);

      if (scan(NULL) != KET) {
         LERROR(LESYNTAX);
         DEXIT;
         return NULL;
      }
      eat_token();

      DEXIT;
      return cp;
   }

   DEXIT;
   return read_val(dp, app);

}

static lCondition *read_val(
lDescr *dp,
va_list *app 
) {
   /*       
      without the usage of s insight throws
      a READ_OVERFLOW 
    */
   volatile char *s;

   lCondition *cp;
   int token;

   DENTER(CULL_LAYER, "read_val");

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   if ((token = scan(NULL)) != FIELD) {
      cp = lFreeWhere(cp);
      LERROR(LESYNTAX);
      DEXIT;
      return 0;
   }
   eat_token();

   cp->operand.cmp.nm = va_arg(*app, int);
   if ((cp->operand.cmp.pos = lGetPosInDescr(dp, cp->operand.cmp.nm)) < 0) {
      cp = lFreeWhere(cp);
      LERROR(LENAMENOT);
      DEXIT;
      return NULL;
   }
   cp->operand.cmp.mt = dp[cp->operand.cmp.pos].mt;

   switch (token = scan(NULL)) {
   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case BITMASK:
   case STRCASECMP:
   case PATTERNCMP:
   case HOSTNAMECMP:
      cp->op = token;
      eat_token();
      break;

   case SUBSCOPE:
      cp->op = token;
      eat_token();
      if (cp->operand.cmp.mt != lListT) {
         cp = lFreeWhere(cp);
         LERROR(LEINCTYPE);
         DEXIT;
         return NULL;
      }
      cp->operand.cmp.val.cp = subscope(app);
      DEXIT;
      return cp;

   default:
      cp = lFreeWhere(cp);
      LERROR(LEOPUNKNOWN);
      DEXIT;
      return NULL;
   }

   switch (scan(NULL)) {
   case INT:
      if (cp->operand.cmp.mt != lIntT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEINTT );
      cp->operand.cmp.val.i = va_arg(*app, lInt);
      break;

   case STRING:
      if (cp->operand.cmp.mt != lStringT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBESTRINGT );
      s = va_arg(*app, char *);

      cp->operand.cmp.val.str = strdup((char *) s);
/*       cp->operand.cmp.val.str = strdup(va_arg(*app, char *)); */
      break;

   case ULONG:
      if (cp->operand.cmp.mt != lUlongT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEULONGT);
      cp->operand.cmp.val.ul = va_arg(*app, lUlong);
      break;

   case FLOAT:
      if (cp->operand.cmp.mt != lFloatT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEFLOATT);
      /* a float value is stored as a double in the va_list */
      /* so we have to read it as a double value                              */
      cp->operand.cmp.val.fl = (lFloat) va_arg(*app, lDouble);
      break;

   case DOUBLE:
      if (cp->operand.cmp.mt != lDoubleT)
         incompatibleType( MSG_CULL_WHERE_SHOULDBEDOUBLET);
      cp->operand.cmp.val.db = va_arg(*app, lDouble);
      break;

   case LONG:
      if (cp->operand.cmp.mt != lLongT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBELONGT);
      cp->operand.cmp.val.l = va_arg(*app, lLong);
      break;

   case CHAR:
      if (cp->operand.cmp.mt != lCharT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBECHART);
      cp->operand.cmp.val.c = va_arg(*app, lChar);
      break;

   case REF:
      if (cp->operand.cmp.mt != lRefT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEREFT );
      cp->operand.cmp.val.ref = va_arg(*app, lRef);
      break;

   default:
      cp = lFreeWhere(cp);
      unknownType("lWhere");
      DEXIT;
      return NULL;
   }
   eat_token();

   DEXIT;
   return cp;
}

/* ############################################################# */
/* ------------------------------------------------------------

   _lWhere builds the lCondition tree.
   The condition is stated as a format string and an associated 
   WhereArgList of the following form:

   %I field ( unequal lList: J_id)
   == comparison operator ( <, >, <=, >=, !=, ==)
   %d value type descriptor (%s string, %d int, %u ulong) : 375

   Ansi Prototype:
   lCondition* _lWhere( char *fmt, WhereArgList wap) 

   The WhereArg struct is built as follows:
   struct _WhereArg {
   lDescr      *descriptor;
   int         field;
   lMultitype  *value;
   };
   e.g. where = lWhere("%T( %I == %s && %I -> %T ( %I < %d ) )", QueueT,
   Q_hostname, "durin.q", Q_ownerlist, OwnerT, O_ownerage, 22);

   -- the corresponding WhereArgList is:
   WhereArg whereargs[20];

   whereargs[0].descriptor = QueueT;
   whereargs[1].field      = Q_hostname;
   whereargs[1].value.str  = "durin.q";
   whereargs[2].field      = Q_ownerlist;
   whereargs[2].descriptor = OwnerT;
   whereargs[3].field      = O_ownerage;
   whereargs[3].value.i    = 22;

   where = _lWhere("%T( %I == %s && %I -> %T ( %I < %d ) )", whereargs);
 */
lCondition *_lWhere(
const char *fmt,
WhereArgList wap 
) {
   lCondition *cond;

   DENTER(CULL_LAYER, "_lWhere");

   if (!fmt) {
      LERROR(LENOFORMATSTR);
      DEXIT;
      return NULL;
   }
   /* 
      initialize scan function, the actual token is scanned again
      in the subscope fuction. There we call eat_token to go ahead
    */
   scan(fmt);

   /* parse */
   cond = _subscope(&wap);

   if (!cond) {
      LERROR(LEPARSECOND);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return cond;
}

static lCondition *_subscope(
WhereArgList *wapp 
) {
   lDescr *dp = NULL;
   lCondition *cp = NULL;

   DENTER(CULL_LAYER, "_subscope");

   if (scan(NULL) != TYPE) {
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat %T */
   /* Deliver descriptor & increment the WhereArgList to the next element */
/*    DPRINTF(("(*wapp) = %p\n", *wapp)); */
   dp = (*wapp)++->descriptor;
   if (!(dp)) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (scan(NULL) != BRA) {
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat ( */

   if (!(cp = _sum(dp, wapp))) {
      LERROR(LESUM);
      DEXIT;
      return NULL;
   }

   if (scan(NULL) != KET) {
      LERROR(LESYNTAX);
      lFreeWhere(cp);
      DEXIT;
      return NULL;
   }
   eat_token();                 /* eat ) */

   DEXIT;
   return cp;
}

static lCondition *_sum(
lDescr *dp,
WhereArgList *wapp 
) {
   lCondition *cp, *newcp;

   DENTER(CULL_LAYER, "_sum");

   cp = _product(dp, wapp);

   while (scan(NULL) == OR) {
      eat_token();              /* eat || */

      if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         LERROR(LEMALLOC);
         lFreeWhere(cp);
         DEXIT;
         return NULL;
      }
      newcp->op = OR;
      newcp->operand.log.first = cp;
      newcp->operand.log.second = _product(dp, wapp);
      cp = newcp;
   }

   DEXIT;
   return cp;
}

static lCondition *_product(
lDescr *dp,
WhereArgList *wapp 
) {
   lCondition *cp, *newcp;

   DENTER(CULL_LAYER, "_product");

   cp = _factor(dp, wapp);

   while (scan(NULL) == AND) {
      eat_token();

      if (!(newcp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         lFreeWhere(cp);
         LERROR(LEMALLOC);
         DEXIT;
         return NULL;
      }
      newcp->op = AND;
      newcp->operand.log.first = cp;
      newcp->operand.log.second = _factor(dp, wapp);
      cp = newcp;
   }

   DEXIT;
   return cp;
}

static lCondition *_factor(
lDescr *dp,
WhereArgList *wapp 
) {
   lCondition *cp;

   DENTER(CULL_LAYER, "_factor");

   if (scan(NULL) == NEG) {
      eat_token();

      if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
         lFreeWhere(cp);
         LERROR(LEMALLOC);
         DEXIT;
         return NULL;
      }
      cp->operand.log.first = _negfactor(dp, wapp);
      cp->operand.log.second = NULL;
      cp->op = NEG;
   }
   else
      cp = _negfactor(dp, wapp);

   DEXIT;
   return cp;
}

static lCondition *_negfactor(
lDescr *dp,
WhereArgList *wapp 
) {
   lCondition *cp;

   DENTER(CULL_LAYER, "_negfactor");

   if (scan(NULL) == BRA) {
      eat_token();

      cp = _sum(dp, wapp);

      if (scan(NULL) != KET) {
         LERROR(LESYNTAX);
         DEXIT;
         return NULL;
      }
      eat_token();

      DEXIT;
      return cp;
   }

   DEXIT;
   return _read_val(dp, wapp);

}

static lCondition *_read_val(
lDescr *dp,
WhereArgList *wapp 
) {
   lCondition *cp;
   int token;

   DENTER(CULL_LAYER, "_read_val");

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   if ((token = scan(NULL)) != FIELD) {
      lFreeWhere(cp);
      LERROR(LESYNTAX);
      DEXIT;
      return 0;
   }
   eat_token();

/*    DPRINTF(("(*wapp) = %p\n", *wapp)); */
/*    DPRINTF(("(*wapp)->field = %d\n", (*wapp)->field)); */
   cp->operand.cmp.nm = (*wapp)->field;
   if ((cp->operand.cmp.pos = lGetPosInDescr(dp, cp->operand.cmp.nm)) < 0) {
      lFreeWhere(cp);
      LERROR(LENAMENOT);
      DEXIT;
      return NULL;
   }
   cp->operand.cmp.mt = dp[cp->operand.cmp.pos].mt;

   switch (token = scan(NULL)) {
   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case BITMASK:
   case STRCASECMP:
   case PATTERNCMP:
   case HOSTNAMECMP:
      cp->op = token;
      eat_token();
      break;

   case SUBSCOPE:
      cp->op = token;
      eat_token();
      if (cp->operand.cmp.mt != lListT) {
         LERROR(LEINCTYPE);
         DEXIT;
         return NULL;
      }
      cp->operand.cmp.val.cp = _subscope(wapp);
      DEXIT;
      return cp;

   default:
      lFreeWhere(cp);
      LERROR(LEOPUNKNOWN);
      DEXIT;
      return NULL;
   }

   switch (scan(NULL)) {
   case INT:
      if (cp->operand.cmp.mt != lIntT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEINTT);
/*       DPRINTF(("(*wapp)->value.i = %d\n", (*wapp)->value.i)); */
      cp->operand.cmp.val.i = (*wapp)++->value.i;
      break;

   case STRING:
      if (cp->operand.cmp.mt != lStringT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBESTRINGT);
/*       DPRINTF(("(*wapp)->value.str = %s\n", (*wapp)->value.str)); */
      cp->operand.cmp.val.str = (*wapp)++->value.str;
      break;

   case ULONG:
      if (cp->operand.cmp.mt != lUlongT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEULONGT);
/*       DPRINTF(("(*wapp)->value.ul = %ul\n", (*wapp)->value.ul)); */
      cp->operand.cmp.val.ul = (*wapp)++->value.ul;
      break;

   case FLOAT:
      if (cp->operand.cmp.mt != lFloatT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEFLOATT);
      /* a float value is stored as a double in the va_list */
      /* so we have to read it as a double value                              */
/*       DPRINTF(("(*wapp)->value.fl = %f\n", (*wapp)->value.fl)); */
      cp->operand.cmp.val.fl = (*wapp)++->value.fl;
      break;

   case DOUBLE:
      if (cp->operand.cmp.mt != lDoubleT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEDOUBLET);
/*       DPRINTF(("(*wapp)->value.db = %f\n", (*wapp)->value.db)); */
      cp->operand.cmp.val.db = (*wapp)++->value.db;
      break;

   case LONG:
      if (cp->operand.cmp.mt != lLongT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBELONGT);
/*       DPRINTF(("(*wapp)->value.l = %ld\n", (*wapp)->value.l)); */
      cp->operand.cmp.val.l = (*wapp)++->value.l;
      break;

   case CHAR:
      if (cp->operand.cmp.mt != lCharT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBECHART);
/*       DPRINTF(("(*wapp)->value.c = %c\n", (*wapp)->value.c)); */
      cp->operand.cmp.val.c = (*wapp)++->value.c;
      break;

   case REF:
      if (cp->operand.cmp.mt != lRefT)
         incompatibleType(MSG_CULL_WHERE_SHOULDBEREFT);
/*       DPRINTF(("(*wapp)->value.ref = %p\n", (*wapp)->value.ref)); */
      cp->operand.cmp.val.ref = (*wapp)++->value.ref;
      break;

   default:
      unknownType("_lWhere");
      DEXIT;
      return NULL;
   }

   /* 
      "m=" as operator is not allowed with other types than lUlongT
      "c=" as operator is not allowed with other types than lStringT
      "p=" as operator is not allowed with other types than lStringT
      "h=" as operator is not allowed with other types than lStringT
    */
   if ((cp->op == BITMASK && cp->operand.cmp.mt != lUlongT) ||
       (cp->op == STRCASECMP && cp->operand.cmp.mt != lStringT)||
       (cp->op == HOSTNAMECMP && cp->operand.cmp.mt != lStringT)||
       (cp->op == PATTERNCMP && cp->operand.cmp.mt != lStringT))
      incompatibleType(MSG_CULL_WHERE_OPERANDHITNOTOPERATORERROR );

   eat_token();

   DEXIT;
   return cp;
}

/* ############################################################# */
/* ------------------------------------------------------------ 

   lFreeWhere frees all conditions depending on cp.
 */
lCondition *lFreeWhere(
lCondition *cp 
) {

   DENTER(CULL_LAYER, "lFreeWhere");

   if (!cp) {
      DEXIT;
      return NULL;
   }

   switch (cp->op) {
   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case BITMASK:
   case STRCASECMP:
   case PATTERNCMP:

      if (cp->operand.cmp.mt == lStringT) {
         if (cp->operand.cmp.val.str) {
            free(cp->operand.cmp.val.str);
         }
      }
   case SUBSCOPE:
      if (cp->operand.cmp.mt == lListT) {
         lFreeWhere(cp->operand.cmp.val.cp);
      }
      break;
   case AND:
   case OR:
      lFreeWhere(cp->operand.log.second);
   case NEG:
      lFreeWhere(cp->operand.log.first);
      break;

   default:
      LERROR(LEOPUNKNOWN);
      DEXIT;
      return NULL;
   }

   free(cp);

   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------ 

   lCompare returns true(1) or false(0). It decides if the element ep
   suffices the condition cp.
 */

int lCompare(
const lListElem *ep,
const lCondition *cp 
) {
   int result = 0;
   char *str1, *str2;

   DENTER(CULL_LAYER, "lCompare");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return 0;
   }

   /* no conditions ok */
   if (!cp) {
      DEXIT;
      return 1;
   }

   switch (cp->op) {
   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case SUBSCOPE:

      switch (cp->operand.cmp.mt) {
      case lIntT:
         result = intcmp(lGetPosInt(ep, cp->operand.cmp.pos), cp->operand.cmp.val.i);
         break;
      case lStringT:
         if (!(str1 = lGetPosString(ep, cp->operand.cmp.pos))) {
            LERROR(LENULLSTRING);
            DPRINTF(("lGetPosString in lCompare\n"));
            DEXIT;
            return 0;
         }
         if (!(str2 = cp->operand.cmp.val.str)) {
            DPRINTF(("cp->operand.cmp.val.str in lCompare\n"));
            LERROR(LENULLSTRING);
            DEXIT;
            return 0;
         }
         result = strcmp(str1, str2);
         DPRINTF(("strcmp(%s, %s) = %d\n", str1, str2, result));
         break;
      case lUlongT:
         result = ulongcmp(lGetPosUlong(ep, cp->operand.cmp.pos), cp->operand.cmp.val.ul);
         break;
      case lListT:
         result = (lFindFirst(lGetPosList(ep, cp->operand.cmp.pos), cp->operand.cmp.val.cp) != NULL);
         DEXIT;
         return result;
      case lFloatT:
         result = floatcmp(lGetPosFloat(ep, cp->operand.cmp.pos), cp->operand.cmp.val.fl);
         break;
      case lDoubleT:
         result = doublecmp(lGetPosDouble(ep, cp->operand.cmp.pos), cp->operand.cmp.val.db);
         break;
      case lLongT:
         result = longcmp(lGetPosLong(ep, cp->operand.cmp.pos), cp->operand.cmp.val.l);
         break;
      case lCharT:
         result = charcmp(lGetPosChar(ep, cp->operand.cmp.pos), cp->operand.cmp.val.c);
         break;
      case lRefT:
         result = refcmp(lGetPosRef(ep, cp->operand.cmp.pos), cp->operand.cmp.val.ref);
         break;
      default:
         unknownType("lCompare");
         DEXIT;
         return 0;
      }

      switch (cp->op) {
      case EQUAL:
         result = (result == 0);
         break;
      case NOT_EQUAL:
         result = (result != 0);
         break;
      case LOWER_EQUAL:
         result = (result == -1 || result == 0);
         break;
      case LOWER:
         result = (result == -1);
         break;
      case GREATER_EQUAL:
         result = (result == 1 || result == 0);
         break;
      case GREATER:
         result = (result == 1);
         break;
      default:
         LERROR(LEOPUNKNOWN);
         DEXIT;
         return 0;
      }
      break;

   case STRCASECMP:
   case HOSTNAMECMP:
      if (cp->operand.cmp.mt != lStringT) {
         unknownType("lCompare");
         DEXIT;
         return 0;
      }

      if (cp->op == STRCASECMP ) {
         result = SGE_STRCASECMP(lGetPosString(ep, cp->operand.cmp.pos),
                             cp->operand.cmp.val.str);
      } else {
         result = hostcmp(lGetPosString(ep, cp->operand.cmp.pos),
                          cp->operand.cmp.val.str);
      }

      result = (result == 0);
      break;

   case PATTERNCMP:
      if (cp->operand.cmp.mt != lStringT) {
         unknownType("lCompare");
         DEXIT;
         return 0;
      }
      result = !fnmatch(cp->operand.cmp.val.str,
                           lGetPosString(ep, cp->operand.cmp.pos), 0);
      break;


   case BITMASK:
      if (cp->operand.cmp.mt != lUlongT) {
         unknownType("lCompare");
         DEXIT;
         return 0;
      }
      result = bitmaskcmp(lGetPosUlong(ep, cp->operand.cmp.pos), cp->operand.cmp.val.ul);
      break;

   case AND:
      if (!lCompare(ep, cp->operand.log.first)) {
         result = 0;
         break;
      }
      result = lCompare(ep, cp->operand.log.second);
      break;

   case OR:
      if (lCompare(ep, cp->operand.log.first)) {
         result = 1;
         break;
      }
      result = lCompare(ep, cp->operand.log.second);
      break;

   case NEG:
      result = !lCompare(ep, cp->operand.log.first);
      break;

   default:
      DPRINTF(("lCompare(): unknown operator %d\n", cp->op));
      DEXIT;
      exit(-1);

   }
   DEXIT;
   return result;
}

/* ------------------------------------------------------------ 

   lCopyWhere copy a lCondition tree 
 */
lCondition *lCopyWhere(
const lCondition *cp 
) {

   lCondition *new = NULL;

   DENTER(CULL_LAYER, "lCopyWhere");

   if (!cp) {
      DEXIT;
      return NULL;
   }

   if (!(new = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   new->op = cp->op;

   switch (cp->op) {
   case EQUAL:
   case NOT_EQUAL:
   case LOWER_EQUAL:
   case LOWER:
   case GREATER_EQUAL:
   case GREATER:
   case BITMASK:
   case STRCASECMP:
   case HOSTNAMECMP:
   case PATTERNCMP:
      new->operand.cmp.pos = cp->operand.cmp.pos;
      new->operand.cmp.mt = cp->operand.cmp.mt;
      new->operand.cmp.nm = cp->operand.cmp.nm;

      switch (cp->operand.cmp.mt) {
      case lIntT:
         new->operand.cmp.val.i = cp->operand.cmp.val.i;
         break;
      case lUlongT:
         new->operand.cmp.val.ul = cp->operand.cmp.val.ul;
         break;
      case lStringT:
         new->operand.cmp.val.str = strdup(cp->operand.cmp.val.str);
         break;
      case lListT:
         break;
      case lFloatT:
         new->operand.cmp.val.fl = cp->operand.cmp.val.fl;
         break;
      case lDoubleT:
         new->operand.cmp.val.db = cp->operand.cmp.val.db;
         break;
      case lLongT:
         new->operand.cmp.val.l = cp->operand.cmp.val.l;
         break;
      case lCharT:
         new->operand.cmp.val.c = cp->operand.cmp.val.c;
         break;
      case lRefT:
         break;
      default:
         unknownType("lCopyWhere");
         DEXIT;
         return NULL;
      }
   case SUBSCOPE:
      if (cp->operand.cmp.mt == lListT) {
         new->operand.cmp.pos = cp->operand.cmp.pos;
         new->operand.cmp.mt = cp->operand.cmp.mt;
         new->operand.cmp.nm = cp->operand.cmp.nm;
         new->operand.cmp.val.cp = lCopyWhere(cp->operand.cmp.val.cp);
      }
      break;
   case AND:
   case OR:
      new->operand.log.second = lCopyWhere(cp->operand.log.second);
   case NEG:
      new->operand.log.first = lCopyWhere(cp->operand.log.first);
      break;

   default:
      LERROR(LEOPUNKNOWN);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return new;
}
