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
#include "cull_list.h"
#include "cull_hash.h"
#include "cull_lerrnoP.h"
#include "cull_listP.h"
#include "cull_hashP.h"
#include "cull_multitypeP.h"
#include "cull_whatP.h"
#include "cull_whereP.h"
#include "cull_pack.h"
#include "cull_parse.h"

/* =========== implementation ================================= */

static int cull_unpack_switch(sge_pack_buffer *pb, lMultiType *dst, int type);
static int cull_pack_switch(sge_pack_buffer *pb, const lMultiType *src, int type);

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
int type 
) {
   int ret;
   u_long32 i=0;

   DENTER(CULL_LAYER, "cull_unpack_switch");

   switch (type) {

   case lUlongT:
      ret = unpackint(pb, &i);
      dst->ul = i;
      break;

   case lStringT:
      ret = unpackstr(pb, &(dst->str));
      break;

   case lListT:
      ret = cull_unpack_list(pb, &(dst->glp));
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
int type 
) {
   int ret;

   DENTER(CULL_LAYER, "cull_pack_switch");

   switch (type) {

   case lUlongT:
      ret = packint(pb, src->ul);
      break;

   case lStringT:
      ret = packstr(pb, src->str);
      break;

   case lListT:
      ret = cull_pack_list(pb, src->glp);
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

/* ------------------------------------------------------------

   cull_unpack_descr() - unpacks a complete descriptor

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_unpack_descr(
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
   dp[n].hash = NULL;

   /* read in n lDescr fields */
   for (i = 0; i < n; i++) {
      if ((ret = unpackint(pb, &temp))) {
         cull_hash_free_descr(dp);
         free(dp);
         DEXIT;
         return ret;
      }
      dp[i].nm = temp;

      if ((ret = unpackint(pb, &temp))) {
         cull_hash_free_descr(dp);
         free(dp);
         DEXIT;
         return ret;
      }
      dp[i].mt = temp;

      if ((ret = unpackint(pb, &temp))) {
         cull_hash_free_descr(dp);
         free(dp); 
         DEXIT;
      }
      if(temp < 0) {  /* no hashing */
         dp[i].hash = NULL;  
      } else {         /* create hashing info */
         if((dp[i].hash = (lHash *) malloc(sizeof(lHash))) == NULL) {
            cull_hash_free_descr(dp);
            free(dp);
            LERROR(LEMALLOC);
            DEXIT;
            return PACK_ENOMEM;
         }
         dp[i].hash->unique = temp;
         dp[i].hash->table = NULL;
      } 
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
int cull_pack_descr(
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
      /* pack hashing information: -1 = no hash, 0 = non unique, 1 = unique */
      if(dp[i].hash == NULL) {
         if((ret = packint(pb, -1))) {
            DEXIT;
            return ret;
         }
      } else {
         if((ret = packint(pb, dp[i].hash->unique))) {
            DEXIT;
            return ret;
         }
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
int cull_pack_cont(
sge_pack_buffer *pb,
const lMultiType *cp,
const lDescr *dp 
) {
   int i, ret;
   int n;

   DENTER(CULL_LAYER, "cull_pack_cont");

   n = lCountDescr(dp);

   for (i = 0; i < n; i++) {
      if ((ret = cull_pack_switch(pb, &cp[i], dp[i].mt))) {
         DEXIT;
         return ret;
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
int cull_unpack_cont(
sge_pack_buffer *pb,
lMultiType **mpp,
const lDescr *dp 
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
      if ((ret = cull_unpack_switch(pb, &(cp[i]), dp[i].mt))) {
         free(cp);
         DEXIT;
         return ret;
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
int cull_pack_elem(
sge_pack_buffer *pb,
const lListElem *ep 
) {
   int ret;

   DENTER(CULL_LAYER, "cull_pack_elem");

   if (!(ep->descr)) {
      DPRINTF(("element descriptor NULL not allowed !!!\n"));
      DEXIT;
      abort();
   }

   if ((ret = packint(pb, ep->status))) {
      DEXIT;
      return ret;
   }

   if (ep->status == FREE_ELEM) {
      if ((ret = cull_pack_descr(pb, ep->descr))) {
         DEXIT;
         return ret;
      }
   }

   ret = cull_pack_cont(pb, ep->cont, ep->descr);

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
   lListElem *ep;

   DENTER(CULL_LAYER, "cull_unpack_elem");

   *epp = NULL;

   if ((ep = (lListElem *) calloc(1, sizeof(lListElem))) == NULL) {
      DEXIT;
      return PACK_ENOMEM;
   }

   if ((ret = unpackint(pb, &(ep->status)))) {
      free(ep);
      DEXIT;
      return ret;
   }

   if (ep->status == FREE_ELEM) {
      if ((ret = cull_unpack_descr(pb, &(ep->descr)))) {
         free(ep);
         DEXIT;
         return ret;
      }
   }
   else {
      /* 
         if it is not a free element we need 
         an descriptor from outside 
       */
      if (!(ep->descr = (lDescr *) dp)) {
         free(ep);
         DEXIT;
         return PACK_BADARG;
      }
   }

   /* This is a hack, to avoid aborts in lAppendElem */
   if (ep->status == BOUND_ELEM)
      ep->status = TRANS_BOUND_ELEM;

   /* 
      here after setting status correctly we got 
      an element in a defined state - so we may
      call lFreeElem() in case of errors 
    */
   if ((ret = cull_unpack_cont(pb, &(ep->cont), ep->descr))) {
      free(ep);
      DEXIT;
      return ret;
   }

   *epp = ep;

   DEXIT;
   return PACK_SUCCESS;
}

/* ================================================================ */

/* ------------------------------------------------------------

   cull_pack_list() - packs a complete list

   return values:
   PACK_SUCCESS
   PACK_ENOMEM
   PACK_FORMAT

 */
int cull_pack_list(
sge_pack_buffer *pb,
const lList *lp 
) {
   int ret;
   lListElem *ep;

   DENTER(CULL_LAYER, "cull_pack_list");

   if ((ret = packint(pb, lp != NULL))) {
      DEXIT;
      return ret;
   }

   if (lp) {

      if ((ret = packint(pb, lp->nelem))) {
         DEXIT;
         return ret;
      }
      if ((ret = packstr(pb, lp->listname))) {
         DEXIT;
         return ret;
      }

      /* pack descriptor */
      if ((ret = cull_pack_descr(pb, lp->descr))) {
         DEXIT;
         return ret;
      }

      /* pack each list element */
      for_each(ep, lp)
         if ((ret = cull_pack_elem(pb, ep))) {
         DEXIT;
         return ret;
      }
   }

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

int cull_unpack_list(
sge_pack_buffer *pb,
lList **lpp 
) {
   int ret;
   lList *lp;
   lListElem *ep;

   u_long32 i=0;
   u_long32 n=0;

   DENTER(CULL_LAYER, "cull_unpack_list");

   *lpp = NULL;

   if ((ret = unpackint(pb, &i))) {
      DEXIT;
      return ret;
   }

   /* do we have an empty list (NULL) ? */
   if (!i) {
      DEXIT;
      return PACK_SUCCESS;
   }

   if ((lp = (lList *) calloc(1, sizeof(lList))) == NULL) {
      DEXIT;
      return PACK_ENOMEM;
   }

   if ((ret = unpackint(pb, &n))) {
      lFreeList(lp);
      DEXIT;
      return ret;
   }

   if ((ret = unpackstr(pb, &(lp->listname)))) {
      lFreeList(lp);
      DEXIT;
      return ret;
   }

   /* unpack descriptor */
   if ((ret = cull_unpack_descr(pb, &(lp->descr)))) {
      lFreeList(lp);
      DEXIT;
      return ret;
   }

   cull_hash_create_hashtables(lp);

   /* unpack each list element */
   for (i = 0; i < n; i++) {
      if ((ret = cull_unpack_elem(pb, &ep, lp->descr))) {
         lFreeList(lp);
         DEXIT;
         return ret;
      }
      lAppendElem(lp, ep);
   }

   *lpp = lp;

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

   if ((ret = packint(pb, enp != NULL)))
      goto error;

   if (!enp) {
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
      }
   }

   DEXIT;
   return PACK_SUCCESS;

 error:
   DPRINTF(("error packing enumeration\n"));
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

   *enpp = NULL;

   if ((ret = unpackint(pb, &i)))
      goto error;

   if (!i) {
      DEXIT;
      return PACK_SUCCESS;
   }

   if ((ret = unpackint(pb, &flag)))
      goto error;

   if (flag != PackWhatArray) {
      if (!(enp = (lEnumeration *) malloc(2 * sizeof(lEnumeration)))) {
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
      enp[1].nm = NoName;
      enp[1].mt = lEndT;

   }
   else {
      /* read in number of lEnumeration fields (without end mark) */
      if ((ret = unpackint(pb, &n)))
         goto error;

      if (!(enp = (lEnumeration *) malloc(sizeof(lEnumeration) * (n + 1)))) {
         DEXIT;
         return PACK_ENOMEM;
      }

      /* read in n lEnumeration fields */
      for (i = 0; i < n; i++) {
         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].pos = temp;
         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].mt = temp;
         if ((ret = unpackint(pb, &temp)))
            goto error;
         enp[i].nm = temp;
      }

      enp[n].nm = NoName;
      enp[n].mt = lEndT;
   }

   *enpp = enp;

   DEXIT;
   return PACK_SUCCESS;

 error:

   lFreeWhat(enp);
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

   if ((ret = packint(pb, cp != NULL))) {
      DEXIT;
      return ret;
   }

   if (!cp) {
      DEXIT;
      return PACK_SUCCESS;
   }

   /* pack operator indicating the following contents */
   if ((ret = packint(pb, cp->op))) {
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
         DEXIT;
         return ret;
      }
      DPRINTF(("cp->operand.cmp.mt: %d\n", cp->operand.cmp.mt));
      if ((ret = packint(pb, cp->operand.cmp.mt))) {
         DEXIT;
         return ret;
      }
      DPRINTF(("cp->operand.cmp.nm: %d\n", cp->operand.cmp.nm));
      if ((ret = packint(pb, cp->operand.cmp.nm))) {
         DEXIT;
         return ret;
      }

      if (cp->operand.cmp.mt != lListT) {
         if ((ret = cull_pack_switch(pb, &(cp->operand.cmp.val), cp->operand.cmp.mt))) {
            DEXIT;
            return ret;
         }
      }
      else {
         if ((ret = cull_pack_cond(pb, cp->operand.cmp.val.cp))) {
            DEXIT;
            return ret;
         }
      }
      break;

   case NEG:
      if ((ret = cull_pack_cond(pb, cp->operand.log.first))) {
         DEXIT;
         return ret;
      }
      break;

   case AND:
   case OR:
      if ((ret = cull_pack_cond(pb, cp->operand.log.first))) {
         DEXIT;
         return ret;
      }
      if ((ret = cull_pack_cond(pb, cp->operand.log.second))) {
         DEXIT;
         return ret;
      }
      break;

   default:
      DEXIT;
      return PACK_FORMAT;
   }

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

   *cpp = NULL;
   if ((ret = unpackint(pb, &i))) {
      DEXIT;
      return ret;
   }
   if (!i) {
      *cpp = NULL;
      DEXIT;
      return 0;
   }

   if (!(cp = (lCondition *) calloc(1, sizeof(lCondition)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return PACK_ENOMEM;
   }

   if ((ret = unpackint(pb, &i))) {
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
         DEXIT;
         return ret;
      }
      cp->operand.cmp.pos = i;
      if ((ret = unpackint(pb, &i))) {
         DEXIT;
         return ret;
      }
      cp->operand.cmp.mt = i;
      if ((ret = unpackint(pb, &i))) {
         DEXIT;
         return ret;
      }
      cp->operand.cmp.nm = i;

      if (cp->operand.cmp.mt != lListT) {
         if ((ret = cull_unpack_switch(pb, &(cp->operand.cmp.val), cp->operand.cmp.mt))) {
            DEXIT;
            return ret;
         }
      }
      else {
         if ((ret = cull_unpack_cond(pb, &(cp->operand.cmp.val.cp)))) {
            DEXIT;
            return ret;
         }
      }
      break;

   case NEG:
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.first)))) {
         DEXIT;
         return ret;
      }
      break;

   case AND:
   case OR:
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.first)))) {
         DEXIT;
         return ret;
      }
      if ((ret = cull_unpack_cond(pb, &(cp->operand.log.second)))) {
         DEXIT;
         return ret;
      }
      break;

   default:
      DEXIT;
      return PACK_FORMAT;
   }

   *cpp = cp;

   DEXIT;
   return PACK_SUCCESS;
}
