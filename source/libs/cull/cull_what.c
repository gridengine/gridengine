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
 *  License at http://www.gridengine.sunsource.net/license.html
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

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sgermon.h"
#include "cull_list.h"
#include "cull_db.h"
#include "cull_parse.h"
#include "cull_multitype.h"
#include "cull_whatP.h"
#include "cull_lerrnoP.h"

  
/* ------------------------------------------------------------ 

   nm_set() is a utility function that can help to build int 
   vectors like it is used by lIntVector2What() 

 */
void nm_set(
int job_field[],
int nm 
) {
   int i;

   DENTER(TOP_LAYER, "nm_set");

   /* seek it */
   for (i=0; job_field[i]!=NoName; i++)
      if (job_field[i] == nm) {
         DEXIT;
         return; /* found */
      }

   /* set it */
   DPRINTF(("%s\n", lNm2Str(nm)));
   job_field[i++] = nm;
   job_field[i] = NoName;

   DEXIT;
   return;
}


/* ------------------------------------------------------------ 

   lReduceDescr makes a new descriptor in *dst_dpp that 
   contains only those fields from src_dp that are in
   enp.

 */
int lReduceDescr(
lDescr **dst_dpp,
lDescr *src_dp,
lEnumeration *enp 
) {
   int n, index = 0;

   DENTER(TOP_LAYER, "lReduceDescr");

   if (!dst_dpp || !src_dp || !enp) {
      DEXIT;
      return -1;
   }

   n = lCountWhat(enp, src_dp);

   /* create new partial descriptor */
   if (!(*dst_dpp = (lDescr *) malloc(sizeof(lDescr) * (n + 1)))) {
      DEXIT;
      return -1;
   }
   lPartialDescr(enp, src_dp, *dst_dpp, &index);

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   _lWhat creates an enumeration array. This is used in lWhat
   to choose special fields of a list element.
   lEnumeration* _lWhat( char *fmt, lDescr *dp, int* nm_list, int nr_nm) 
 */
lEnumeration *_lWhat(
const char *fmt,
const lDescr *dp,
const int *nm_list,
int nr_nm 
) {
   int neg = 0;
   int i, j, k, n, size = 0;
   const char *s;
   lEnumeration *ep = NULL;
   lEnumeration *ep2 = NULL;
   int error_status;

   DENTER(CULL_LAYER, "_lWhat");

   if (!fmt) {
      error_status = LENOFORMATSTR;
      goto error;
   }
   /* how many fields are selected (for malloc) */
   for (n = 0, s = fmt; *s; s++)
      if (*s == '%')
         n++;

   /* are the number of %'s the number of names - 1 ? */
   if (nr_nm != n - 1) {
      error_status = LESYNTAX;
      goto error;
   }

   if (scan(fmt) != TYPE) {
      /* %T expected */
      error_status = LESYNTAX;
      goto error;
   }
   eat_token();                 /* eat %T */
   if (scan(NULL) != BRA) {
      /* ( expected */
      error_status = LESYNTAX;
      goto error;
   }
   eat_token();                 /* eat ( */

   /* Nr of fields to malloc */
   /* for CULL_ALL and NONE we need one lEnumeration struct */
   if (n > 1)
      n--;
   if (!(ep = (lEnumeration *) malloc(sizeof(lEnumeration) * (n + 1)))) {
      error_status = LEMALLOC;
      goto error;
   }

   /* Special cases CULL_ALL fields are selected or NONE is selected */
   /* Normally the arguments nm_list and nr_nm should be NULL(0)     */
   switch (scan(NULL)) {

   case CULL_ALL:
      ep[0].pos = WHAT_ALL;
      ep[0].nm = -99;
      ep[0].mt = -99;
      eat_token();
      ep[1].pos = 0;
      ep[1].nm = NoName;
      ep[1].mt = lEndT;
      DEXIT;
      return ep;

   case CULL_NONE:
      ep[0].pos = WHAT_NONE;
      ep[0].nm = -99;
      ep[0].mt = -99;
      eat_token();
      ep[1].pos = 0;
      ep[1].nm = NoName;
      ep[1].mt = lEndT;
      DEXIT;
      return ep;

   default:
      break;
   }

   if (scan(NULL) == NEG) {
      neg = 1;
      if ((size = lCountDescr(dp)) == -1) {
         error_status = LECOUNTDESCR;
         goto error;
      }
      /* subtract the fields that shall not be enumerated */
      size -= n;
      eat_token();              /* eat ! */
      if (scan(NULL) != BRA) {
         /* ( expected */
         error_status = LESYNTAX;
         goto error;
      }
      else
         eat_token();           /* eat_token BRA, if it was a BRA */
   }

   for (i = 0; i < n; i++) {

      ep[i].nm = nm_list[i];
      if ((ep[i].pos = lGetPosInDescr(dp, ep[i].nm)) < 0) {
         error_status = LENAMENOT;
         goto error;
      }
      ep[i].mt = dp[ep[i].pos].mt;

      if (scan(NULL) != FIELD) {
         error_status = LESYNTAX;
         goto error;
      }
      eat_token();

   }

   ep[n].pos = 0;
   ep[n].nm = NoName;
   ep[n].mt = lEndT;

   if (neg) {
      if (scan(NULL) != KET) {
         error_status = LESYNTAX;
         goto error;
      }
      eat_token();

      if (!(ep2 = (lEnumeration *) malloc(sizeof(lEnumeration) * (size + 1)))) {
         error_status = LEMALLOC;
         goto error;
      }

      k = 0;
      for (i = 0; dp[i].nm != NoName; i++) {
         /* remove the elements listed in the format string */
         for (j = 0; ep[j].nm != NoName && ep[j].nm != dp[i].nm; j++);
         if (ep[j].nm != NoName)
            continue;
         ep2[k].pos = i;
         ep2[k].nm = dp[i].nm;
         ep2[k].mt = dp[i].mt;
         k++;
      }
      ep2[k].pos = 0;
      ep2[size].nm = NoName;
      ep2[size].mt = lEndT;
      lFreeWhat(ep);
      ep = ep2;
   }

   if (scan(NULL) != KET) {
      error_status = LESYNTAX;
      goto error;
   }
   eat_token();                 /* eat ) */

   DEXIT;
   return ep;

 error:
   LERROR(error_status);
   if (ep)
      lFreeWhat(ep);
   DPRINTF(("error_status = %d\n", error_status));
   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------ 

   lWhat creates an enumeration array. This is used in lSelect and
   lJoin to choose special field of a list element.
   lEnumeration* lWhat( char *fmt, ...) 
 */
lEnumeration *lWhat(const char *fmt,...)
{
   int i, n;
   const char *s;
   lDescr *dp = NULL;
   lEnumeration *ep;
   int *nm_list = NULL;
   int nr_nm;
   va_list ap;
   int error_status;

   DENTER(CULL_LAYER, "lWhat");
   va_start(ap, fmt);

   if (!fmt) {
      error_status = LENOFORMATSTR;
      goto error;
   }
   /* how many fields are selected (for malloc) */
   for (n = 0, s = fmt; *s; s++)
      if (*s == '%')
         n++;

   if (n <= 0) {
      /* possibly %T is missing */
      error_status = LESYNTAX;
      goto error;
   }

   dp = va_arg(ap, lDescr *);

   /* malloc space for an int array */
   nr_nm = n - 1;
   if (nr_nm) {
      if (!(nm_list = (int *) malloc(sizeof(int) * nr_nm))) {
         error_status = LEMALLOC;
         goto error;
      }
   }
   /* read variable argument list into int array nm_list */
   for (i = 0; i < nr_nm; i++) {
      nm_list[i] = va_arg(ap, int);
   }

   /* this function does parse and create the lEnumeration structure */
   ep = _lWhat(fmt, dp, nm_list, nr_nm);

   if (nm_list)
      free(nm_list);
   va_end(ap);

   DEXIT;
   return ep;

 error:
   LERROR(error_status);
   va_end(ap);
   DPRINTF(("error_status = %d\n", error_status));
   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------ 

   lFreeWhat frees the enumeration array ep.
 */

lEnumeration *lFreeWhat(
lEnumeration *ep 
) {
   DENTER(CULL_LAYER, "lFreeWhat");

   if (!ep) {
      DEXIT;
      return NULL;
   }
   free(ep);

   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------

   writes a lEnumeration array  (for debugging purposes)

 */
void lWriteWhatTo(
const lEnumeration *ep,
FILE *fp 
) {
   int i;

   DENTER(CULL_LAYER, "lWriteWhatTo");

   if (!ep) {
      LERROR(LEENUMNULL);
      DEXIT;
      return;
   }
   for (i = 0; ep[i].mt != lEndT; i++) {
      switch (ep[i].pos) {
      case WHAT_NONE:
         if (!fp) {
            DPRINTF(("nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  "NONE", ep[i].mt, ep[i].pos));
         }
         else {
            fprintf(fp, "nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  "NONE", ep[i].mt, ep[i].pos);
         }
         break;
      case WHAT_ALL:
         if (!fp) {
            DPRINTF(("nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  "ALL", ep[i].mt, ep[i].pos));
         }
         else {
            fprintf(fp, "nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  "ALL", ep[i].mt, ep[i].pos);
         }
         break;
      default:
         if (!fp) {
            DPRINTF(("nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  lNm2Str(ep[i].nm), ep[i].mt, ep[i].pos));
         }
         else {
            fprintf(fp, "nm: %6d %-20.20s mt: %d pos: %d\n", ep[i].nm,
                  lNm2Str(ep[i].nm), ep[i].mt, ep[i].pos);
         }
      }
   }

   DEXIT;
   return;
}

int lCountWhat(
const lEnumeration *enp,
const lDescr *dp 
) {
   int n;

   DENTER(CULL_LAYER, "lCountWhat");

   if (!enp) {
      LERROR(LEENUMNULL);
      DEXIT;
      return -1;
   }
   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return -1;
   }
   switch (enp[0].pos) {
   case WHAT_NONE:
      n = 0;
      break;
   case WHAT_ALL:
      if ((n = lCountDescr(dp)) == -1) {
         LERROR(LECOUNTDESCR);
         DEXIT;
         return -1;
      }
      break;
   default:
      for (n = 0; enp[n].nm != NoName; n++);
   }

   DEXIT;
   return n;
}

/* ------------------------------------------------------------

   copies a lEnumeration array

 */
lEnumeration *lCopyWhat(
const lEnumeration *ep 
) {
   int i, n;
   lEnumeration *copy = NULL;

   DENTER(CULL_LAYER, "lCopyWhat");

   if (!ep) {
      LERROR(LEENUMNULL);
      DEXIT;
      return NULL;
   }

   for (n = 0; ep[n].mt != lEndT; n++);

   if (!(copy = (lEnumeration *) malloc(sizeof(lEnumeration) * (n + 1)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   for (i = 0; i <= n; i++) {
      copy[i].pos = ep[i].pos;
      copy[i].nm = ep[i].nm;
      copy[i].mt = ep[i].mt;
   }

   DEXIT;
   return copy;
}

lEnumeration *lIntVector2What(
const lDescr *dp,
const int intv[] 
) {
   lEnumeration *what;
   char fmtstr[2000];
   int i;

   DENTER(CULL_LAYER, "lIntVector2What");

   /*
      reduce a descriptor to get one with only
      these fields that are needed for modify
    */
   strcpy(fmtstr, "%T(");
   for (i = 0; intv[i] != NoName; i++) {
      strcat(fmtstr, "%I");
   }
   strcat(fmtstr, ")");

   DPRINTF(("fmtstr: %s\n", fmtstr));

   what = _lWhat(fmtstr, dp, intv, i);

   DEXIT;
   return what;
}
