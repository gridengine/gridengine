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

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sge_log.h"
#include "sgermon.h"
#include "cull_listP.h"
#include "cull_list.h"
#include "cull_db.h"
#include "cull_parse.h"
#include "cull_multitype.h"
#include "cull_whatP.h"
#include "cull_lerrnoP.h"

#include "pack.h"
#include "cull_pack.h"
#include "cull_packL.h"
#include "msg_gdilib.h"

/****** cull/what/nm_set() ****************************************************
*  NAME
*     nm_set() -- Build a int vector 
*
*  SYNOPSIS
*     void nm_set(int job_field[], int nm) 
*
*  FUNCTION
*     Build a int vector like it is used by lIntVector2What() 
*
*  INPUTS
*     int job_field[] - int vector 
*     int nm          - field name id  
******************************************************************************/
void nm_set(int job_field[], int nm) 
{
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

/****** cull/what/lReduceDescr() **********************************************
*  NAME
*     lReduceDescr() -- Reduce a descriptor 
*
*  SYNOPSIS
*     int lReduceDescr(lDescr **dst_dpp, lDescr *src_dp, lEnumeration *enp) 
*
*  FUNCTION
*     Makes a new descriptor in 'dst_dpp' that containes only those 
*     fields from 'src_dp' that are in 'enp'. 
*
*  INPUTS
*     lDescr **dst_dpp  - destination for reduced descriptor 
*     lDescr *src_dp    - source descriptor 
*     lEnumeration *enp - condition 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
*******************************************************************************/
int lReduceDescr(lDescr **dst_dpp, lDescr *src_dp, lEnumeration *enp) 
{
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
 */
lEnumeration *_lWhat(const char *fmt, const lDescr *dp, 
                            const int *nm_list, int nr_nm) 
{
   int neg = 0;
   int i, j, k, n, size = 0;
   const char *s;
   lEnumeration *ep = NULL;
   lEnumeration *ep2 = NULL;
   int error_status;
   cull_parse_state state;

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

   memset(&state, 0, sizeof(state));
   if (scan(fmt, &state) != TYPE) {
      /* %T expected */
      error_status = LESYNTAX;
      goto error;
   }
   eat_token(&state);                 /* eat %T */
   if (scan(NULL, &state) != BRA) {
      /* ( expected */
      error_status = LESYNTAX;
      goto error;
   }
   eat_token(&state);                 /* eat ( */

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
   switch (scan(NULL, &state)) {

   case CULL_ALL:
      ep[0].pos = WHAT_ALL;
      ep[0].nm = -99;
      ep[0].mt = -99;
      eat_token(&state);
      ep[1].pos = 0;
      ep[1].nm = NoName;
      ep[1].mt = lEndT;
      DEXIT;
      return ep;

   case CULL_NONE:
      ep[0].pos = WHAT_NONE;
      ep[0].nm = -99;
      ep[0].mt = -99;
      eat_token(&state);
      ep[1].pos = 0;
      ep[1].nm = NoName;
      ep[1].mt = lEndT;
      DEXIT;
      return ep;

   default:
      break;
   }

   if (scan(NULL, &state) == NEG) {
      neg = 1;
      if ((size = lCountDescr(dp)) == -1) {
         error_status = LECOUNTDESCR;
         goto error;
      }
      /* subtract the fields that shall not be enumerated */
      size -= n;
      eat_token(&state);              /* eat ! */
      if (scan(NULL, &state) != BRA) {
         /* ( expected */
         error_status = LESYNTAX;
         goto error;
      }
      else
         eat_token(&state);           /* eat_token BRA, if it was a BRA */
   }

   for (i = 0; i < n; i++) {

      ep[i].nm = nm_list[i];
      if ((ep[i].pos = lGetPosInDescr(dp, ep[i].nm)) < 0) {
         error_status = LENAMENOT;
         goto error;
      }
      ep[i].mt = dp[ep[i].pos].mt;

      if (scan(NULL, &state) != FIELD) {
         error_status = LESYNTAX;
         goto error;
      }
      eat_token(&state);

   }

   ep[n].pos = 0;
   ep[n].nm = NoName;
   ep[n].mt = lEndT;

   if (neg) {
      if (scan(NULL, &state) != KET) {
         error_status = LESYNTAX;
         goto error;
      }
      eat_token(&state);

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

   if (scan(NULL, &state) != KET) {
      error_status = LESYNTAX;
      goto error;
   }
   eat_token(&state);                 /* eat ) */

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

/****** cull/what/lWhat() *****************************************************
*  NAME
*     lWhat() -- Creates a enumeration array. 
*
*  SYNOPSIS
*     lEnumeration* lWhat(const char *fmt, ...) 
*
*  FUNCTION
*     Creates a enumeration array. This is used in lSelect and 
*     lJoin to choose special fields of alist element.  
*
*  INPUTS
*     const char *fmt - format string 
*     ...             - additional arguments 
*
*  RESULT
*     lEnumeration* - enumeration 
******************************************************************************/
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

/****** cull/what/lWhatAll() *****************************************************
*  NAME
*     lWhatAll() -- Creates a enumeration array requesting all elements. 
*
*  SYNOPSIS
*     lEnumeration* lWhatAll() 
*
*  FUNCTION
*     Creates a enumeration array that requests complete elements 
*     of whatever typed list. This is a shortcut for 
*     lWhat("%T(ALL)", <List_type>)), cause for all the descriptor is not
*     needed anyway, it is available from the list itself.
*
*  INPUTS
*
*  RESULT
*     lEnumeration* - enumeration 
******************************************************************************/
lEnumeration *lWhatAll()
{
   lEnumeration *ep;
   int error_status;

   DENTER(CULL_LAYER, "lWhatAll");

   if (!(ep = (lEnumeration *) malloc(sizeof(lEnumeration) * 2))) {
      error_status = LEMALLOC;
      goto error;
   }

   ep[0].pos = WHAT_ALL;
   ep[0].nm = -99;
   ep[0].mt = -99;
   ep[1].pos = 0;
   ep[1].nm = NoName;
   ep[1].mt = lEndT;

   DEXIT;
   return ep;

 error:
   LERROR(error_status);
   DPRINTF(("error_status = %d\n", error_status));
   DEXIT;
   return NULL;
}

/****** cull/what/lFreeWhat() *************************************************
*  NAME
*     lFreeWhat() -- Frees a enumeration array 
*
*  SYNOPSIS
*     lEnumeration* lFreeWhat(lEnumeration *ep) 
*
*  FUNCTION
*     Frees a enumeration array 
*
*  INPUTS
*     lEnumeration *ep - enumeration 
*
*  RESULT
*     lEnumeration* - NULL 
******************************************************************************/
lEnumeration *lFreeWhat(lEnumeration *ep) 
{
   DENTER(CULL_LAYER, "lFreeWhat");

   if (!ep) {
      DEXIT;
      return NULL;
   }
   free(ep);

   DEXIT;
   return NULL;
}

/****** cull/what/lWriteWhatTo() **********************************************
*  NAME
*     lWriteWhatTo() -- Writes a enumeration array to a file stream 
*
*  SYNOPSIS
*     void lWriteWhatTo(const lEnumeration *ep, FILE *fp) 
*
*  FUNCTION
*     Writes a enumeration array to a file stream 
*
*  INPUTS
*     const lEnumeration *ep - enumeration 
*     FILE *fp               - file stream 
******************************************************************************/
void lWriteWhatTo(const lEnumeration *ep, FILE *fp) 
{
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

/****** cull/what/lCountWhat() ************************************************
*  NAME
*     lCountWhat() -- Returns size of enumeration 
*
*  SYNOPSIS
*     int lCountWhat(const lEnumeration *enp, const lDescr *dp) 
*
*  FUNCTION
*     Returns size of enumeration 
*
*  INPUTS
*     const lEnumeration *enp - enumeration 
*     const lDescr *dp        - descriptor 
*
*  RESULT
*     int - number of fields in enumeration 
******************************************************************************/
int lCountWhat(const lEnumeration *enp, const lDescr *dp) 
{
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

/****** cull/what/lCopyWhat() *************************************************
*  NAME
*     lCopyWhat() -- Copy a enumeration array 
*
*  SYNOPSIS
*     lEnumeration* lCopyWhat(const lEnumeration *ep) 
*
*  FUNCTION
*     Copy a enumeration array 
*
*  INPUTS
*     const lEnumeration *ep - enumeration 
*
*  RESULT
*     lEnumeration* - new copy of enumeration 
******************************************************************************/
lEnumeration *lCopyWhat(const lEnumeration *ep) 
{
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

/****** cull/what/lIntVector2What() *******************************************
*  NAME
*     lIntVector2What() -- Create a enumeration from int array 
*
*  SYNOPSIS
*     lEnumeration* lIntVector2What(const lDescr *dp, const int intv[]) 
*
*  FUNCTION
*     Create a enumeration from int array 
*
*  INPUTS
*     const lDescr *dp - descriptor 
*     const int intv[] - int array 
*
*  RESULT
*     lEnumeration* - enumeration 
******************************************************************************/
lEnumeration *lIntVector2What(const lDescr *dp, const int intv[]) 
{
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

lListElem *lWhatToElem(const lEnumeration *what){
   lListElem *whatElem = NULL;
   sge_pack_buffer pb;
   int size;
   DENTER(CULL_LAYER, "lWhatToElem");
   
   /* 
    * retrieve packbuffer size to avoid large realloc's while packing 
    */
   init_packbuffer(&pb, 0, 1);
   if (cull_pack_enum(&pb, what) == PACK_SUCCESS) {
      size = pb_used(&pb);
      clear_packbuffer(&pb);

      /*
       * now we do the real packing
       */
      if (init_packbuffer(&pb, size, 0) == PACK_SUCCESS) {
         if (cull_pack_enum(&pb, what) == PACK_SUCCESS) {
            whatElem = lCreateElem(PACK_Type);
            lSetUlong(whatElem, PACK_id, SGE_WHAT);
#ifdef COMMCOMPRESS 
            if(pb.mode == 0) {
               if(flush_packbuffer(&pb) == PACK_SUCCESS){
                  setByteArray( (char*)pb.head_ptr,  pb.cpr.total_out, whereElem, PACK_String);
                  lSetBool(whatElem, PACK_Compressed, true);
                }
                else
                  whatElem = lFreeElem(whatElem);
            }
            else
#endif
            {
               setByteArray( (char*)pb.head_ptr, pb.bytes_used, whatElem, PACK_string);
               lSetBool(whatElem, PACK_compressed, false);
            }
         }
      }
      clear_packbuffer(&pb); 
   }
   DEXIT;
   return whatElem;
}

lEnumeration *lWhatFromElem(const lListElem *what){
   lEnumeration *cond = NULL;
   sge_pack_buffer pb;
   int size=0;
   char *buffer;
   bool compressed = false;
   int ret=0;
   DENTER(CULL_LAYER, "lWhatFromCull");
   
   if (lGetUlong(what, PACK_id) == SGE_WHAT) {
      compressed = lGetBool(what, PACK_compressed);
      size = getByteArray(&buffer, what, PACK_string);
      if (size <= 0){
         ERROR((SGE_EVENT, MSG_PACK_INVALIDPACKDATA ));
      }
      else if ((ret = init_packbuffer_from_buffer(&pb, buffer, size, compressed)) == PACK_SUCCESS){
         cull_unpack_enum(&pb, &cond);
         clear_packbuffer(&pb); 
      }
      else {
         FREE(buffer);
         ERROR((SGE_EVENT, MSG_PACK_ERRORUNPACKING_S, cull_pack_strerror(ret)));
      }
   }
   else {
      ERROR((SGE_EVENT, MSG_PACK_WRONGPACKTYPE_UI, (long)lGetUlong(what, PACK_id), SGE_WHAT));
   }
   DEXIT;
   return cond;
}

