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
#include <stdlib.h>
#include <string.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sgermon.h"

#include "sge_profiling.h"

#include "cull_list.h"
#include "cull_hash.h"
#include "cull_lerrnoP.h"
#include "cull_listP.h"
#include "cull_multitypeP.h"
#include "cull_whatP.h"
#include "cull_whereP.h"
#include "cull_pack.h"
#include "cull_parse.h"

/* =========== implementation ================================= */

static int cull_unpack_switch(sge_pack_buffer *pb, lMultiType *dst, int type, int flags);
static int cull_pack_switch(sge_pack_buffer *pb, const lMultiType *src, int type, int flags);

static int cull_unpack_descr(sge_pack_buffer *pb, lDescr **dpp);
static int cull_pack_descr(sge_pack_buffer *pb, const lDescr *dp);

static int cull_unpack_cont(sge_pack_buffer *pb, lMultiType **mpp, const lDescr *dp, int flags);
static int cull_pack_cont(sge_pack_buffer *pb, const lMultiType *mp, const lDescr *dp, int flags);

static int cull_unpack_object(sge_pack_buffer *pb, lListElem **epp, int flags);
static int cull_pack_object(sge_pack_buffer *pb, const lListElem *ep, int flags);


/* ------------------------------------------------------------

   cull_unpack_switch() - switch used to unpack a lMulitype

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_unpack_switch(
sge_pack_buffer *pb,
lMultiType *dst,
int type,
int flags
) {
   int ret;
   u_long32 i=0;

   DENTER(CULL_LAYER, "cull_unpack_switch");

   switch (type) {

   case lBoolT:
      ret = unpackint(pb, &i);
      dst->b = i;
      break;

   case lUlongT:
      ret = unpackint(pb, &i);
      dst->ul = i;
      break;

   case lStringT:
      ret = unpackstr(pb, &(dst->str));
      break;

   case lHostT:
      ret = unpackstr(pb, &(dst->host));
      break;

   case lListT:
      ret = cull_unpack_list_partial(pb, &(dst->glp), flags);
      break;

   case lObjectT:
      ret = cull_unpack_object(pb, &(dst->obj), flags);
      break;

   case lDoubleT:
      ret = unpackdouble(pb, &(dst->db));
      break;

   case lRefT:
      dst->ref = NULL;
      ret = PACK_SUCCESS;
      break;
      
   case lIntT:
   case lFloatT:
   case lLongT:
   case lCharT:
      DPRINTF(("type not implemented\n"));
      ret = PACK_FORMAT;
      break;
   default:
      DPRINTF(("unknown type %d\n", type));
      ret = PACK_FORMAT;
      break;
   }

   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   cull_pack_switch() - switch used to pack a lMulitype

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_pack_switch(
sge_pack_buffer *pb,
const lMultiType *src,
int type,
int flags
) {
   int ret;

   DENTER(CULL_LAYER, "cull_pack_switch");

   switch (type) {

   case lBoolT:
      ret = packint(pb, src->b);
      break;

   case lUlongT:
      ret = packint(pb, src->ul);
      break;

   case lStringT:
      ret = packstr(pb, src->str);
      break;

   case lHostT:
      ret = packstr(pb, src->host);
      break;

   case lListT:
      ret = cull_pack_list_partial(pb, src->glp, flags);
      break;

   case lObjectT:
      ret = cull_pack_object(pb, src->obj, flags);
      break;

   case lDoubleT:
      ret = packdouble(pb, src->db);
      break;

   case lRefT:
      ret = PACK_SUCCESS;
      break;

   case lIntT:
   case lFloatT:
   case lLongT:
   case lCharT:
      DPRINTF(("not implemented\n"));
      ret = PACK_FORMAT;
      break;
   default:
      DPRINTF(("unknown type %d\n", type));
      ret = PACK_FORMAT;
      break;
   }

   DEXIT;
   return ret;
}

/****** cull/pack/cull_get_list_packsize() ************************************
*  NAME
*     cull_get_list_packsize() -- Get size of a CULL list
*
*  SYNOPSIS
*     size_t cull_get_list_packsize(const lList *list) 
*
*  FUNCTION
*     Return the number of bytes which "list" would consume in a
*     pack buffer.
*
*  INPUTS
*     const lList *list - Any CULL list 
*
*  RESULT
*     size_t - Size in bytes
*
*  SEE ALSO
*     cull/pack/cull_get_elem_packsize() 
******************************************************************************/
size_t cull_get_list_packsize(const lList *list)
{
   size_t ret = 0;
   DENTER(CULL_LAYER, "cull_get_list_packsize");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if (list != NULL) {
      sge_pack_buffer pb;

      init_packbuffer(&pb, 0, 1);
      if (!cull_pack_list(&pb, list)) {
         ret = pb_used(&pb);
      }
      clear_packbuffer(&pb);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return ret;
}

/****** cull/pack/cull_get_elem_packsize() ************************************
*  NAME
*     cull_get_elem_packsize() -- Get size of a CULL element 
*
*  SYNOPSIS
*     size_t cull_get_elem_packsize(const lListElem *elem) 
*
*  FUNCTION
*     Return the number of bytes which "elem" would consume in a
*     pack buffer. 
*
*  INPUTS
*     const lListElem *elem - Any CULL element
*
*  RESULT
*     size_t - Size in bytes
*
*  SEE ALSO
*     cull/pack/cull_get_list_packsize()
*******************************************************************************/
size_t cull_get_elem_packsize(const lListElem *elem)
{
   size_t ret = 0;
   DENTER(CULL_LAYER, "cull_get_elem_packsize");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if (elem != NULL) {
      sge_pack_buffer pb;

      init_packbuffer(&pb, 0, 1);
      if (!cull_pack_elem(&pb, elem)) {
         ret = pb_used(&pb);
      }
      clear_packbuffer(&pb);
   }
   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   cull_unpack_descr() - unpacks a complete descriptor

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_unpack_descr(
sge_pack_buffer *pb,
lDescr **dpp 
) {
   lDescr *dp;
   int ret;
   u_long32 n=0;
   u_long32 i=0;
   u_long32 temp=0;

   DENTER(CULL_LAYER, "cull_unpack_descr");

   *dpp = NULL;

   /* read in number of lDescr fields (without end mark) */
   if ((ret = unpackint(pb, &n))) {
      DEXIT;
      return ret;
   }

   if ((dp = (lDescr *) malloc(sizeof(lDescr) * (n + 1))) == NULL) {
      LERROR(LEMALLOC);
      DEXIT;
      return PACK_ENOMEM;
   }

   memset(dp, 0, sizeof(lDescr) * (n + 1));

   dp[n].nm = NoName;
   dp[n].mt = lEndT;
   dp[n].ht = NULL;

   /* read in n lDescr fields */
   for (i = 0; i < n; i++) {
      if ((ret = unpackint(pb, &temp))) {
         free(dp);
         DEXIT;
         return ret;
      }
      dp[i].nm = temp;

      if ((ret = unpackint(pb, &temp))) {
         free(dp);
         DEXIT;
         return ret;
      }
      dp[i].mt = temp;

      dp[i].ht = NULL;
   }

   *dpp = dp;

   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_pack_descr() - packs a complete descriptor

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_pack_descr(
sge_pack_buffer *pb,
const lDescr *dp 
) {
   int i, ret;

   DENTER(CULL_LAYER, "cull_pack_descr");

   /* pack the number of lDescr fields (without end mark) */
   if ((ret = packint(pb, lCountDescr(dp)))) {
      DEXIT;
      return ret;
   }
   /* pack the lDescr fields */
   for (i = 0; dp[i].nm != NoName && dp[i].mt != lEndT; i++) {
      if ((ret = packint(pb, dp[i].nm))) {
         DEXIT;
         return ret;
      }
      if ((ret = packint(pb, dp[i].mt))) {
         DEXIT;
         return ret;
      }
   }

   DEXIT;
   return PACK_SUCCESS;
}

/* ================================================================ */

/* ------------------------------------------------------------

   cull_pack_cont() - packs a contents array

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_pack_cont(
sge_pack_buffer *pb,
const lMultiType *cp,
const lDescr *dp,
int flags
) {
   int i, ret;
   int n;

   DENTER(CULL_LAYER, "cull_pack_cont");

   n = lCountDescr(dp);

   for (i = 0; i < n; i++) {
      /* if flags are given, pack only fields matching flags, e.g. CULL_SPOOL */
      if (flags == 0 || (dp[i].mt & flags) != 0) {
         if ((ret = cull_pack_switch(pb, &cp[i], mt_get_type(dp[i].mt), flags))) {
            DEXIT;
            return ret;
         }
      }
   }

   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_unpack_cont() - unpacks a contents array

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_unpack_cont(
sge_pack_buffer *pb,
lMultiType **mpp,
const lDescr *dp,
int flags
) {
   int i, n, ret;

   lMultiType *cp;

   DENTER(CULL_LAYER, "cull_unpack_cont");

   *mpp = NULL;

   n = lCountDescr(dp);
   if ((cp = (lMultiType *) calloc(1, sizeof(lMultiType) * (n + 1))) == NULL) {
      LERROR(LEMALLOC);
      DEXIT;
      return PACK_ENOMEM;
   }

   for (i = 0; i < n; i++) {
      /* if flags are given, unpack only fields matching flags, e.g. CULL_SPOOL */
      if (flags == 0 || (dp[i].mt & flags) != 0) {
         if ((ret = cull_unpack_switch(pb, &(cp[i]), mt_get_type(dp[i].mt), flags))) {
            free(cp);
            DEXIT;
            return ret;
         }
      }
   }

   *mpp = cp;

   DEXIT;
   return PACK_SUCCESS;
}

/* ================================================================ */

/* ------------------------------------------------------------

   cull_pack_elem() - packs a list element

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_pack_elem(sge_pack_buffer *pb, const lListElem *ep) 
{
   int ret;

   DENTER(CULL_LAYER, "cull_pack_elem");
   ret = cull_pack_elem_partial(pb, ep, 0);
   DEXIT;
   return ret;
}


int cull_pack_elem_partial(sge_pack_buffer *pb, const lListElem *ep, int flags)
{
   int ret;

   DENTER(CULL_LAYER, "cull_pack_elem_partial");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if(ep->descr == NULL) {
      DPRINTF(("element descriptor NULL not allowed !!!\n"));
      DEXIT;
      abort();
   }

   if((ret = packint(pb, ep->status)) != PACK_SUCCESS) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if(ep->status == FREE_ELEM) {
      if((ret = cull_pack_descr(pb, ep->descr)) != PACK_SUCCESS) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
   }

   if((ret = packbitfield(pb, &(ep->changed))) != PACK_SUCCESS) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   ret = cull_pack_cont(pb, ep->cont, ep->descr, flags);

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   cull_unpack_elem() - unpacks a list element

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_unpack_elem(
sge_pack_buffer *pb,
lListElem **epp,
const lDescr *dp                  /* has to be NULL in case of free elements 
                                   must be the desriptor in case of bound elements */
) {
   int ret;
   
   DENTER(CULL_LAYER, "cull_unpack_elem");
   ret = cull_unpack_elem_partial(pb, epp, dp, 0);
   DEXIT;
   return ret;
}

int cull_unpack_elem_partial(sge_pack_buffer *pb, lListElem **epp, const lDescr *dp, int flags) 
{
   int ret;
   lListElem *ep;

   DENTER(CULL_LAYER, "cull_unpack_elem_partial");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   *epp = NULL;

   if((ep = (lListElem *) calloc(1, sizeof(lListElem))) == NULL) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_ENOMEM;
   }

   if((ret = unpackint(pb, &(ep->status))) != PACK_SUCCESS) {
      free(ep);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if(ep->status == FREE_ELEM) {
      if((ret = cull_unpack_descr(pb, &(ep->descr))) != PACK_SUCCESS) {
         free(ep);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
   } else {
      /* 
         if it is not a free element we need 
         a descriptor from outside 
       */
      if((ep->descr = (lDescr *) dp) == NULL) {
         free(ep);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return PACK_BADARG;
      }
   }

   /* This is a hack, to avoid aborts in lAppendElem */
   if(ep->status == BOUND_ELEM || ep->status == OBJECT_ELEM)
      ep->status = TRANS_BOUND_ELEM;

   /* 
      here after setting status correctly we got 
      an element in a defined state - so we may
      call lFreeElem() in case of errors 
    */

   if((ret = unpackbitfield(pb, &(ep->changed), lCountDescr(ep->descr))) 
                            != PACK_SUCCESS) {
      if(ep->status == FREE_ELEM || ep->status == OBJECT_ELEM) {
         free(ep->descr);
      }
      free(ep);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }
    
   if((ret = cull_unpack_cont(pb, &(ep->cont), ep->descr, flags))) {
      if(ep->status == FREE_ELEM || ep->status == OBJECT_ELEM) {
         free(ep->descr);
      }   
      free(ep);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   *epp = ep;

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;
}

/* ================================================================ */

/* ------------------------------------------------------------

   cull_pack_object() - packs a sub object

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
static int cull_pack_object(
sge_pack_buffer *pb,
const lListElem *ep,
int flags
) {
   int ret;

   DENTER(CULL_LAYER, "cull_pack_object");

   if((ret = packint(pb, ep != NULL)) != PACK_SUCCESS) {
      DEXIT;
      return ret;
   }

   if(ep != NULL) {
      /* pack descriptor */
      if((ret = cull_pack_descr(pb, ep->descr)) != PACK_SUCCESS) {
         DEXIT;
         return ret;
      }

      /* pack list element */
      if((ret = cull_pack_elem_partial(pb, ep, flags)) != PACK_SUCCESS) {
         DEXIT;
         return ret;
      }
   }

   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_pack_list() - packs a complete list

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_pack_list(sge_pack_buffer *pb, const lList *lp) 
{
   int ret;

   DENTER(CULL_LAYER, "cull_pack_list");
   ret = cull_pack_list_partial(pb, lp, 0);
   DEXIT;
   return ret;
}


int cull_pack_list_partial(sge_pack_buffer *pb, const lList *lp, int flags) 
{
   int ret;
   lListElem *ep;

   DENTER(CULL_LAYER, "cull_pack_list_partial");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if((ret = packint(pb, lp != NULL)) != PACK_SUCCESS) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if(lp != NULL) {
      if((ret = packint(pb, lp->nelem)) != PACK_SUCCESS) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      
      if((ret = packstr(pb, lp->listname)) != PACK_SUCCESS) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      
      if((ret = packint(pb, lp->changed)) != PACK_SUCCESS) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }

      /* pack descriptor */
      if((ret = cull_pack_descr(pb, lp->descr)) != PACK_SUCCESS) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }

      /* pack each list element */
      for_each(ep, lp) {
         if((ret = cull_pack_elem_partial(pb, ep, flags)) != PACK_SUCCESS) {
            PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
            DEXIT;
            return ret;
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_unpack_list() - unpacks a list 

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */

int cull_unpack_list(sge_pack_buffer *pb, lList **lpp)
{
   int ret;

   DENTER(CULL_LAYER, "cull_unpack_list");
   ret = cull_unpack_list_partial(pb, lpp, 0);
   DEXIT;
   return ret;
}

int cull_unpack_list_partial(sge_pack_buffer *pb, lList **lpp, int flags)
{
   int ret;
   lList *lp;
   lListElem *ep;

   u_long32 i=0;
   u_long32 n=0;
   u_long32 c=0;

   DENTER(CULL_LAYER, "cull_unpack_list_partial");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   *lpp = NULL;

   if((ret = unpackint(pb, &i))) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   /* do we have an empty list (NULL) ? */
   if(!i) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_SUCCESS;
   }

   if((lp = (lList *) calloc(1, sizeof(lList))) == NULL) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_ENOMEM;
   }

   if((ret = unpackint(pb, &n)) != PACK_SUCCESS) {
      lFreeList(lp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if((ret = unpackstr(pb, &(lp->listname))) != PACK_SUCCESS) {
      lFreeList(lp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if((ret = unpackint(pb, &c)) != PACK_SUCCESS) {
      lFreeList(lp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }
   lp->changed = c;

   /* unpack descriptor */
   if((ret = cull_unpack_descr(pb, &(lp->descr))) != PACK_SUCCESS) {
      lFreeList(lp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   /* unpack each list element */
   for(i = 0; i < n; i++) {
      if((ret = cull_unpack_elem_partial(pb, &ep, lp->descr, flags)) != PACK_SUCCESS) {
         lFreeList(lp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      lAppendElem(lp, ep);
   }

   cull_hash_create_hashtables(lp);

   *lpp = lp;

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_unpack_object() - unpacks a sub object 

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */

static int cull_unpack_object(
sge_pack_buffer *pb,
lListElem **epp,
int flags
) {
   int ret;
   lDescr *descr;
   lListElem *ep;
   u_long32 i=0;

   DENTER(CULL_LAYER, "cull_unpack_object");

   *epp = NULL;

   if((ret = unpackint(pb, &i))) {
      DEXIT;
      return ret;
   }

   /* do we have an empty object (NULL) ? */
   if(!i) {
      DEXIT;
      return PACK_SUCCESS;
   }

   /* unpack descriptor */
   if((ret = cull_unpack_descr(pb, &descr)) != PACK_SUCCESS) {
      DEXIT;
      return ret;
   }

   /* unpack each element */
   if((ret = cull_unpack_elem_partial(pb, &ep, descr, flags)) != PACK_SUCCESS) {
      free(descr);
      DEXIT;
      return ret;
   }

   ep->status = OBJECT_ELEM;
   *epp = ep;

   DEXIT;
   return PACK_SUCCESS;
}

enum {
   PackWhatAll = 0, PackWhatNone, PackWhatArray
};

/* ------------------------------------------------------------

   cull_pack_enum() - packs an enumeration

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_pack_enum(
sge_pack_buffer *pb,
const lEnumeration *enp 
) {
   int ret;
   u_long32 flag=0;
   int i=0, n=0;

   DENTER(CULL_LAYER, "cull_pack_enum");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if ((ret = packint(pb, enp != NULL)))
      goto error;

   if (!enp) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_SUCCESS;
   }

   /* pack flag indicating ALL, NONE or normal enumeration (array) */
   switch (enp[0].pos) {
   case WHAT_ALL:
      flag = PackWhatAll;
      break;
   case WHAT_NONE:
      flag = PackWhatNone;
      break;
   default:
      flag = PackWhatArray;
   }

   if ((ret = packint(pb, flag)))
      goto error;

   if (flag == PackWhatArray) {
      /* compute and pack number of lEnumeration fields (without end mark) */
      for (n = 0; enp[n].nm != NoName; n++);
      if ((ret = packint(pb, n)))
         goto error;

      /* pack the lEnumeration fields */
      for (i = 0; enp[i].nm != NoName && enp[i].mt != lEndT; i++) {
         if ((ret = packint(pb, enp[i].pos)))
            goto error;
         if ((ret = packint(pb, enp[i].mt)))
            goto error;
         if ((ret = packint(pb, enp[i].nm)))
            goto error;
         if (enp[i].ep == NULL) {
            if ((ret = packint(pb, 0))) {
               goto error;
            }
         } else {
            if ((ret = packint(pb, 1))) {
               goto error;
            }
            if ((ret = cull_pack_enum(pb, enp[i].ep))) {
               goto error;
            }
         }
      }
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;

 error:
   DPRINTF(("error packing enumeration\n"));
   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   cull_unpack_enum() - unpacks an enumeration

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_unpack_enum(
sge_pack_buffer *pb,
lEnumeration **enpp 
) {
   int ret;
   lEnumeration *enp = NULL;
   u_long32 flag=0, i=0, temp=0;
   u_long32 n=0;

   DENTER(CULL_LAYER, "cull_unpack_enum");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   *enpp = NULL;

   if ((ret = unpackint(pb, &i)))
      goto error;

   if (!i) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_SUCCESS;
   }

   if ((ret = unpackint(pb, &flag)))
      goto error;

   if (flag != PackWhatArray) {
      if (!(enp = (lEnumeration *) malloc(2 * sizeof(lEnumeration)))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return PACK_ENOMEM;
      }

      switch (flag) {
      case PackWhatAll:
         enp[0].pos = WHAT_ALL;
         break;

      case PackWhatNone:
         enp[0].pos = WHAT_NONE;
         break;

      default:
         break;
      }
      enp[0].nm = -99;
      enp[0].mt = -99;
      enp[0].ep = NULL;
      enp[1].nm = NoName;
      enp[1].mt = lEndT;
      enp[1].ep = NULL;
   } else {
      /* read in number of lEnumeration fields (without end mark) */
      if ((ret = unpackint(pb, &n)))
         goto error;

      if (!(enp = (lEnumeration *) malloc(sizeof(lEnumeration) * (n + 1)))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return PACK_ENOMEM;
      }

      /* read in n lEnumeration fields */
      for (i = 0; i < n; i++) {
         lEnumeration *tmp2 = NULL;

         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].pos = temp;
         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].mt = temp;
         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].nm = temp;
         if ((ret = unpackint(pb, &temp)))
            goto error;
         if (temp == 1) {
            if ((ret = cull_unpack_enum(pb, &tmp2))) {
               goto error;
            }
         }
         enp[i].ep = tmp2;
         tmp2 = NULL;
      }

      enp[n].nm = NoName;
      enp[n].mt = lEndT;
      enp[n].ep = NULL;
   }

   *enpp = enp;

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;

 error:

   lFreeWhat(enp);
   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return ret;
}

/* ------------------------------------------------------------

   cull_pack_cond() - packs an condition

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_pack_cond(
sge_pack_buffer *pb,
const lCondition *cp 
) {
   int ret;

   DENTER(CULL_LAYER, "cull_pack_cond");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   if ((ret = packint(pb, cp != NULL))) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   if (!cp) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_SUCCESS;
   }

   /* pack operator indicating the following contents */
   if ((ret = packint(pb, cp->op))) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }

   DPRINTF(("cp->op: %d\n", cp->op));

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
   case SUBSCOPE:
   case HOSTNAMECMP:
      DPRINTF(("cp->operand.cmp.pos: %d\n", cp->operand.cmp.pos));
      if ((ret = packint(pb, cp->operand.cmp.pos))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      DPRINTF(("cp->operand.cmp.mt: %d\n", cp->operand.cmp.mt));
      if ((ret = packint(pb, cp->operand.cmp.mt))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      DPRINTF(("cp->operand.cmp.nm: %d\n", cp->operand.cmp.nm));
      if ((ret = packint(pb, cp->operand.cmp.nm))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }

      if (mt_get_type(cp->operand.cmp.mt) != lListT) {
         if ((ret = cull_pack_switch(pb, &(cp->operand.cmp.val), mt_get_type(cp->operand.cmp.mt), 0))) {
            PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
            DEXIT;
            return ret;
         }
      } else {
         if ((ret = cull_pack_cond(pb, cp->operand.cmp.val.cp))) {
            PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
            DEXIT;
            return ret;
         }
      }
      break;

   case NEG:
      if ((ret = cull_pack_cond(pb, cp->operand.log.first))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      break;

   case AND:
   case OR:
      if ((ret = cull_pack_cond(pb, cp->operand.log.first))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      if ((ret = cull_pack_cond(pb, cp->operand.log.second))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      break;

   default:
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_FORMAT;
   }

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;
}

/* ------------------------------------------------------------

   cull_unpack_cond() - unpacks an condition

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_unpack_cond(
sge_pack_buffer *pb,
lCondition **cpp 
) {
   int ret;
   u_long32 i=0;
   lCondition *cp = NULL;

   DENTER(CULL_LAYER, "cull_unpack_cond");

   PROF_START_MEASUREMENT(SGE_PROF_PACKING);
   *cpp = NULL;
   if ((ret = unpackint(pb, &i))) {
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }
   if (!i) {
      *cpp = NULL;
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return 0;
   }

   if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_ENOMEM;
   }

   if ((ret = unpackint(pb, &i))) {
      lFreeWhere(cp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return ret;
   }
   cp->op = i;

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
   case SUBSCOPE:
   case HOSTNAMECMP:
      if ((ret = unpackint(pb, &i))) {
         lFreeWhere(cp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      cp->operand.cmp.pos = i;
      if ((ret = unpackint(pb, &i))) {
         lFreeWhere(cp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      cp->operand.cmp.mt = i;
      if ((ret = unpackint(pb, &i))) {
         lFreeWhere(cp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      cp->operand.cmp.nm = i;

      if (mt_get_type(cp->operand.cmp.mt) != lListT) {
         if ((ret = cull_unpack_switch(pb, &(cp->operand.cmp.val), mt_get_type(cp->operand.cmp.mt), 0))) {
            lFreeWhere(cp);
            PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
            DEXIT;
            return ret;
         }
      } else {
         if ((ret = cull_unpack_cond(pb, &(cp->operand.cmp.val.cp)))) {
            lFreeWhere(cp);
            PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
            DEXIT;
            return ret;
         }
      }
      break;

   case NEG:
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.first)))) {
         lFreeWhere(cp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      break;

   case AND:
   case OR:
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.first)))) {
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         lFreeWhere(cp);
         DEXIT;
         return ret;
      }
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.second)))) {
         lFreeWhere(cp);
         PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
         DEXIT;
         return ret;
      }
      break;

   default:
      lFreeWhere(cp);
      PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
      DEXIT;
      return PACK_FORMAT;
   }

   *cpp = cp;

   PROF_STOP_MEASUREMENT(SGE_PROF_PACKING);
   DEXIT;
   return PACK_SUCCESS;
}

/****** cull_pack/setByteArray() ***********************************************
*  NAME
*     setByteArray() -- takes a byte array, transformes it into ASCII and sets
*                       it as a string into an element
*
*  SYNOPSIS
*     void setByteArray(const char *byteArray, int size, lListElem *elem, int 
*     name) 
*
*  FUNCTION
*     makes a string out of a byte array and sets that string into an element 
*
*  INPUTS
*     const char *byteArray -  byte array
*     int size              - size of the byte array 
*     lListElem *elem       - target element 
*     int name              - target attribute 
*
*  RESULT
*     void - nothing 
*
*******************************************************************************/
void setByteArray(const char *byteArray, int size, lListElem *elem, int name){
   const char *numbers = {"0123456789ABCDEF"};
   int lower_part;
   int upper_part;
   int target_size = size * 2 +1;
   char * z_stream_str=NULL;
   int i=0;
   int y=0;

   if (!byteArray || !elem)
      return;
      
   z_stream_str = malloc(target_size);
   memset(z_stream_str, 0, target_size);

   for (i=0; i < size; i++){
      lower_part = (byteArray[i] & 0x0F);
      upper_part = (byteArray[i] & 0xF0) >> 4;
      z_stream_str[y++] = numbers[lower_part];
      z_stream_str[y++] = numbers[upper_part];
   }
   z_stream_str[y++] = '\0';
   lSetString(elem, name, z_stream_str);
   FREE(z_stream_str);    
} 

/****** cull_pack/getByteArray() ***********************************************
*  NAME
*     getByteArray() -- transforms a string into a byte array. 
*
*  SYNOPSIS
*     int getByteArray(char **byte, const lListElem *elem, int name) 
*
*  FUNCTION
*     extracts a string from an element and changes it into a byte array. The
*     target has to be a pointer to NULL. The array will be created in the function
*     and no memory is freed. The calling functions have to take care of that.
*
*  INPUTS
*     char **byte           - target byte array, has to be a pointer to NULL  
*     const lListElem *elem - the list element, which contains the string
*     int name              - name of the attribute containing the string 
*
*  RESULT
*     int - >= 0 the size of the byte array
*           < 0 the position of the first none hex character
*
*******************************************************************************/
int getByteArray(char **byte, const lListElem *elem, int name){
   const char *numbers = {"0123456789ABCDEF"};
   int lower_part = 0;
   int upper_part = 0;
   int i=0;
   int size=0;
   const char* string;
   if (!byte || !elem){
      return size;
   }

   string = lGetString(elem, name);
   size = strlen(string) /2;
   *byte = malloc(size);
   memset(*byte, 0, size);

   for (i=0; i < size; i++){
      int a=0;
      
      for(a=0; a < 16; a++){
         if (numbers[a] == string[i*2]) {
            lower_part = a;
            break;
         }
      }
      if (a == 16) {
         return (i * -2);
      }
      
      for(a=0; a < 16; a++){
         if (numbers[a] == string[i*2+1]) {
            upper_part = a;
            break;
         }
      }
      if (a == 16) {
         return ((i * -2) -1);
      }
      (*byte)[i] = (upper_part << 4) + lower_part;
   }
   
   return size;
}

