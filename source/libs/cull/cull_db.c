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
#include "cull_db.h"
#include "cull_where.h"
#include "cull_listP.h"
#include "cull_whatP.h"
#include "cull_multitypeP.h"
#include "cull_lerrnoP.h"
#include "cull_hash.h"
#include "sge_string.h"

/* -------- intern prototypes --------------------------------- */

static lListElem *lJoinCopyElem(const lDescr *dp, const lListElem *sep0, const lEnumeration *ep0, const lListElem *sep1, const lEnumeration *ep1);

/* =========== implementation ================================= */

/* ------------------------------------------------------------

   returns a combined element with desciptor dp
   uses src0 with mask enp0 and src1 with mask enp1
   as source

 */
static lListElem *lJoinCopyElem(
const lDescr *dp,
const lListElem *src0,
const lEnumeration *enp0,
const lListElem *src1,
const lEnumeration *enp1 
) {
   lListElem *dst;
   int i;

   DENTER(CULL_LAYER, "lJoinCopyElem");

   if (!src0 || !src1) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   if (!(dst = lCreateElem(dp))) {
      LERROR(LECREATEELEM);
      DEXIT;
      return NULL;
   }

   i = 0;
   if (lCopyElemPartial(dst, &i, src0, enp0) == -1) {
      free(dst);
      LERROR(LECOPYELEMPART);
      DEXIT;
      return NULL;
   }
   if (lCopyElemPartial(dst, &i, src1, enp1) == -1) {
      free(dst);
      LERROR(LECOPYELEMPART);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return dst;
}

/* ------------------------------------------------------------

   lJoinSublist() joins a list and one of its sublists
   together. The other parameters are equal to them
   from lJoin(). In the enumeration enp0 the sublist
   field neither may be selected nor enp0 may be NULL.

 */
lList *lJoinSublist(
const char *name,
int nm0,
const lList *lp,
const lCondition *cp0,
const lEnumeration *enp0,
const lDescr *sldp,                   /* sublist descriptor pointer */
const lCondition *cp1,
const lEnumeration *enp1 
) {
   lList *dlp, *tlp, *joinedlist, *sublist;
   lListElem *ep;
   lDescr *dp; 
   const lDescr *tdp;
   int i, pos;

   DENTER(CULL_LAYER, "lJoinSublist");

   /* check different pointers */
   if (!name || !lp || !enp0 || !sldp || !enp1) {
      LERROR(LENULLARGS);
      DEXIT;
      return NULL;
   }

   /* make sure that nm0 is a sublist field of lp */
   if (!(tdp = lGetListDescr(lp))) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }
   if ((pos = lGetPosInDescr(tdp, nm0)) < 0) {
      LERROR(LENAMENOT);
      DEXIT;
      return NULL;
   }

   if (tdp[pos].mt != lListT) {
      LERROR(LEINCTYPE);
      DEXIT;
      return NULL;
   }

   /* is nm0 enumerated in enp0 ? */
   if (enp0[0].pos == WHAT_ALL) {
      LERROR(LEFALSEFIELD);
      DEXIT;
      return NULL;
   }
   for (i = 0; enp0[i].nm != NoName; i++)
      if (enp0[i].nm == nm0) {
         LERROR(LEFALSEFIELD);
         DEXIT;
         return NULL;
      }

   /* create destination list */
   if (!(dp = lJoinDescr(lGetListDescr(lp), sldp, enp0, enp1))) {
      LERROR(LEJOINDESCR);
      DEXIT;
      return NULL;
   }
   if (!(dlp = lCreateList(name, dp))) {
      free(dp);
      LERROR(LECREATELIST);
      DEXIT;
      return NULL;
   }
   /* free dp it has been copied in lCreateList */
   free(dp);

   /* create a temporary list to be used by lJoin */
   if (!(tlp = lCreateList("lJoinSublist: tlp", lGetListDescr(lp)))) {
      lFreeList(dlp);
      LERROR(LECREATELIST);
      DEXIT;
      return NULL;
   }

   for_each_where(ep, lp, cp0) {
      /* is there a sublist for the join */
      if ((sublist = lGetList(ep, nm0)) != NULL) {

         /* put each element in the tlp to be used by lJoin */
         if (lAppendElem(tlp, lCopyElem(ep)) == -1) {
            lFreeList(tlp);
            lFreeList(dlp);
            LERROR(LEAPPENDELEM);
            DEXIT;
            return NULL;
         }

         /* join the tlp with one element together with its sublist */
         joinedlist = lJoin("lJoinSublist: joinedlist", nm0, tlp, NULL, enp0,
                            NoName, sublist, cp1, enp1);

         if (!joinedlist) {
            lFreeList(tlp);
            lFreeList(dlp);
            LERROR(LEJOIN);
            DEXIT;
            return NULL;
         }

         /* joinedlist is freed in lAddList */
         if (joinedlist && lAddList(dlp, joinedlist) == -1) {
            LERROR(LEADDLIST);
            lFreeList(tlp);
            lFreeList(dlp);
            DEXIT;
            return NULL;
         }

         /* dechain the only element from tlp and free it (copy) */
         lFreeElem(lDechainElem(tlp, tlp->first));
      }
   }
   /* temporary list has to be freed */
   lFreeList(tlp);

   /* RETURN AN EMPTY LIST OR NULL THAT'S THE QUESTION */

   if (lGetNumberOfElem(dlp) == 0) {
      lFreeList(dlp);
      dlp = NULL;
   }

   DEXIT;
   return dlp;
}

/* ------------------------------------------------------------

   returns a new list joining together the lists lp0 and lp1

   for the join only these 'lines' described in condition
   cp0 and cp1 are used

   the new list gets only these members described in enp0 and enp1
   (NULL means every member of this list)

   the list gets name as listname

 */
lList *lJoin(name, nm0, lp0, cp0, enp0,
             nm1, lp1, cp1, enp1)
const char *name;
const lList *lp0;
int nm0;
const lCondition *cp0;
const lEnumeration *enp0;
const lList *lp1;
int nm1;
const lCondition *cp1;
const lEnumeration *enp1;
{
   lListElem *ep0, *ep1;
   lListElem *ep;
   lList *dlp = NULL;
   lDescr *dp;
   int lp0_pos = 0, lp1_pos = 0;
   int i, j;
   int needed;

   DENTER(CULL_LAYER, "lJoin");

   if (!lp0 || !lp1 || !name || !enp0 || !enp1) {
      LERROR(LENULLARGS);
      DEXIT;
      return NULL;
   }

   if (nm1 != NoName) {
      if ((lp0_pos = lGetPosInDescr(lGetListDescr(lp0), nm0)) < 0) {
         LERROR(LENAMENOT);
         DEXIT;
         return NULL;
      }
      if ((lp1_pos = lGetPosInDescr(lGetListDescr(lp1), nm1)) < 0) {
         LERROR(LENAMENOT);
         DEXIT;
         return NULL;
      }

      if (lp0->descr[lp0_pos].mt != lp1->descr[lp1_pos].mt ||
          lp0->descr[lp0_pos].mt == lListT) {
         LERROR(LEDIFFDESCR);
         DEXIT;
         return NULL;
      }
   }

   /* the real join ?! */
   if (!(dp = lJoinDescr(lGetListDescr(lp0), lGetListDescr(lp1), enp0, enp1))) {
      LERROR(LEJOINDESCR);
      DEXIT;
      return NULL;
   }
   if (!(dlp = lCreateList(name, dp))) {
      LERROR(LECREATELIST);
      free(dp);
      DEXIT;
      return NULL;
   }
   /* free dp it has been copied by lCreateList */
   free(dp);

   for (i = 0, ep0 = lp0->first; i < lp0->nelem; i++, ep0 = ep0->next) {
      if (!lCompare(ep0, cp0))
         continue;
      for (j = 0, ep1 = lp1->first; j < lp1->nelem; j++, ep1 = ep1->next) {
         if (!lCompare(ep1, cp1))
            continue;
         if (nm1 != NoName) {   /* in this case take it always */
            /* This is a comparison of the join fields nm0 , nm1 */
            switch (lp0->descr[lp0_pos].mt) {
            case lIntT:
               needed = (ep0->cont[lp0_pos].i == ep1->cont[lp1_pos].i);
               break;
            case lUlongT:
               needed = (ep0->cont[lp0_pos].ul == ep1->cont[lp1_pos].ul);
               break;
            case lStringT:
               needed = !strcmp(ep0->cont[lp0_pos].str, ep1->cont[lp1_pos].str);
               break;
            case lHostT:
               needed = !strcmp(ep0->cont[lp0_pos].str, ep1->cont[lp1_pos].str);
               break;
            case lLongT:
               needed = (ep0->cont[lp0_pos].l == ep1->cont[lp1_pos].l);
               break;
            case lFloatT:
               needed = (ep0->cont[lp0_pos].fl == ep1->cont[lp1_pos].fl);
               break;
            case lDoubleT:
               needed = (ep0->cont[lp0_pos].db == ep1->cont[lp1_pos].db);
               break;
            case lCharT:
               needed = (ep0->cont[lp0_pos].c == ep1->cont[lp1_pos].c);
               break;
            case lRefT:
               needed = (ep0->cont[lp0_pos].ref == ep1->cont[lp1_pos].ref);
               break;
            default:
               unknownType("lJoin");
               DEXIT;
               return NULL;
            }
            if (!needed)
               continue;
         }
         if (!(ep = lJoinCopyElem(dlp->descr, ep0, enp0, ep1, enp1))) {
            LERROR(LEJOINCOPYELEM);
            lFreeList(dlp);
            DEXIT;
            return NULL;
         }
         else {
            if (lAppendElem(dlp, ep) == -1) {
               LERROR(LEAPPENDELEM);
               lFreeList(dlp);
               DEXIT;
               return NULL;
            }
         }
      }
   }

   /* RETURN AN EMPTY LIST OR NULL THAT'S THE QUESTION */

   if (lGetNumberOfElem(dlp) == 0) {
      lFreeList(dlp);
      dlp = NULL;
   }

   DEXIT;
   return dlp;
}

/* ------------------------------------------------------------
   lSplit unchains the list elements from the list slp 
   !___not___! fulfilling the condition cp and returns a list
   containing the unchained elems in ulp.

   if ulp is a NULL pointer the unchained elements are freed

 */
int lSplit(
lList **slp,
lList **ulp,
const char *ulp_name,
const lCondition *cp 
) {

   lListElem *ep, *next;
   int has_been_allocated = 0;

   DENTER(CULL_LAYER, "lSplit");

   /*
      iterate through the source list call lCompare and chain all elems
      that don't fullfill the condition into a new list.
    */
   if (!slp || (ulp && *ulp)) {
      DEXIT;
      return -1;
   }

   for (ep = lFirst(*slp); ep; ep = next) {
      next = ep->next;          /* this is important, cause the elem is dechained */
      if (!lCompare(ep, cp)) {
         if (ulp && !*ulp) {
            *ulp = lCreateList(ulp_name ? ulp_name : "ulp", (*slp)->descr);
            if (!*ulp) {
               DEXIT;
               return -1;
            }
            has_been_allocated = 1;
         }
         if (ulp) {
            ep = lDechainElem(*slp, ep);
            lAppendElem(*ulp, ep);
         }
         else
            lRemoveElem(*slp, ep);
      }
   }

   /* if no elements remain, free the list and return NULL */
   if (*slp && lGetNumberOfElem(*slp) == 0) {
      *slp = lFreeList(*slp);
   }
   if (has_been_allocated && *ulp && lGetNumberOfElem(*ulp) == 0) {
      *ulp = lFreeList(*ulp);
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------
   lSelectDestroy removes the not needed list elements from the list slp 
   not fulfilling the condition cp.
 */
lList *lSelectDestroy(
lList *slp,
const lCondition *cp 
) {

   DENTER(CULL_LAYER, "lSelectDestroy");

   if (lSplit(&slp, NULL, NULL, cp)) {
      lFreeList(slp);
      DEXIT;
      return NULL;
   }
   DEXIT;
   return slp;
}

/* ------------------------------------------------------------
   lSelect creates a new list from the list slp extracting
   the elements fulfilling the condition cp.
   The name argument gives the new list the listname name.
 */
lList *lSelect(
const char *name,
const lList *slp,
const lCondition *cp,
const lEnumeration *enp 
) {

   lListElem *ep, *new;
   lList *dlp = (lList *) NULL;
   lDescr *dp;
   int n, index;

   DENTER(CULL_LAYER, "lSelect");

   if (!slp || !enp) {
      DEXIT;
      return NULL;
   }

   /* create new lList with partial descriptor */
   if ((n = lCountWhat(enp, slp->descr)) <= 0) {
      LERROR(LECOUNTWHAT);
      DEXIT;
      return NULL;
   }

   if (!(dp = (lDescr *) malloc(sizeof(lDescr) * (n + 1)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   /* INITIALIZE THE INDEX IF YOU BUILD A NEW DESCRIPTOR */
   index = 0;
   if (lPartialDescr(enp, slp->descr, dp, &index) < 0) {
      LERROR(LEPARTIALDESCR);
      free(dp);
      DEXIT;
      return NULL;
   }
   if (!(dlp = lCreateList(name, dp))) {
      LERROR(LECREATELIST);
      free(dp);
      DEXIT;
      return NULL;
   }

   /* free the descriptor, it has been copied by lCreateList */
   cull_hash_free_descr(dp);
   free(dp);

   /*
      iterate through the source list call lCompare and add
      depending on result of lCompare
    */
   for (ep = slp->first; ep; ep = ep->next) {
      if (lCompare(ep, cp)) {
         if (!(new = lCreateElem(dlp->descr))) {
            LERROR(LEELEMNULL);
            lFreeList(dlp);
            DEXIT;
            return NULL;
         }
         index = 0;
         if (lCopyElemPartial(new, &index, ep, enp)) {
            LERROR(LECOPYELEMPART);
            lFreeElem(new);
            lFreeList(dlp);
            DEXIT;
            return NULL;
         }
         if (lAppendElem(dlp, new) == -1) {
            LERROR(LEAPPENDELEM);
            lFreeElem(new);
            lFreeList(dlp);
            DEXIT;
            return NULL;
         }
      }
   }

   /* 
      This is a question of philosophy.
      To return an empty list or not to return.
    */
   if (lGetNumberOfElem(dlp) == 0) {
      LERROR(LEGETNROFELEM);
      lFreeList(dlp);
      dlp = NULL;
   }

   DEXIT;
   return dlp;
}

/* ------------------------------------------------------------

   extracts some fields of the source descripor sdp
   masked by an enumeration ep of needed fields

 */
int lPartialDescr(
const lEnumeration *ep,
const lDescr *sdp,
lDescr *ddp,
int *indexp 
) {
   int i;

   DENTER(CULL_LAYER, "lPartialDescr");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   if (!sdp || !ddp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return -1;
   }
   if (!indexp) {
      LERROR(LENULLARGS);
      DEXIT;
      return -1;
   }

   switch (ep[0].pos) {
   case WHAT_NONE:
      DEXIT;
      return 0;
   case WHAT_ALL:
      for (i = 0; sdp[i].mt != lEndT; i++) {
         ddp[*indexp].mt = sdp[i].mt;
         ddp[*indexp].nm = sdp[i].nm;
         if(sdp[i].hash != NULL) {
            ddp[*indexp].hash = cull_hash_copy_descr(&sdp[i]);
         } else {
            ddp[*indexp].hash = NULL;
         }
         (*indexp)++;
      }
      break;
   default:
      /* copy and check descr */
      for (i = 0; ep[i].mt != lEndT; i++) {
         if (ep[i].mt == sdp[ep[i].pos].mt &&
             ep[i].nm == sdp[ep[i].pos].nm) {
            ddp[*indexp].mt = sdp[ep[i].pos].mt;
            ddp[*indexp].nm = sdp[ep[i].pos].nm;
            if(sdp[ep[i].pos].hash != NULL) {
               ddp[*indexp].hash = cull_hash_copy_descr(&sdp[ep[i].pos]);
            } else {
               ddp[*indexp].hash = NULL;
            }
 
            (*indexp)++;
         }
         else {
            LERROR(LEENUMDESCR);
            DEXIT;
            return -1;
         }
      }
   }
   /* copy end mark */
   ddp[*indexp].mt = lEndT;
   ddp[*indexp].nm = NoName;
   ddp[*indexp].hash = NULL;

   /* 
      We don't do (*indexp)++ in order to end up correctly if
      nothing follows and to overwrite at the end position if
      we concatenate two descriptors
    */

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------

   builds from two given descriptors sdp0 and sdp1 a new
   descriptor masked by the enumerations ep0 and ep1

 */
lDescr *lJoinDescr(
const lDescr *sdp0,
const lDescr *sdp1,
const lEnumeration *ep0,
const lEnumeration *ep1 
) {
   int n, m, index;
   lDescr *ddp;

   DENTER(CULL_LAYER, "lJoinDescr");

   if (!sdp0 || !sdp1) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (!ep0 || !ep1) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   /* compute size of new descr */
   n = lCountWhat(ep0, sdp0);
   m = lCountWhat(ep1, sdp1);

   if (n == -1 || m == -1) {
      LERROR(LECOUNTWHAT);
      DEXIT;
      return NULL;
   }

   /* There is WHAT_NONE specified in both lEnumeration ptr's */
   if (!n && !m) {
      LERROR(LEENUMBOTHNONE);
      DEXIT;
      return NULL;
   }

   if (!(ddp = (lDescr *) malloc(sizeof(lDescr) * (n + m + 1)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }
   /* INITIALIZE THE INDEX IF YOU BUILD A NEW DESCRIPTOR */
   index = 0;
   if (lPartialDescr(ep0, sdp0, ddp, &index) < 0) {
      LERROR(LEPARTIALDESCR);
      free(ddp);
      DEXIT;
      return NULL;
   }
   /* This one is appended */
   if (lPartialDescr(ep1, sdp1, ddp, &index) < 0) {
      LERROR(LEPARTIALDESCR);
      free(ddp);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return ddp;
}

/****** cull/list/lString2List() **********************************************
*  NAME
*     lString2List() -- convert char* string into cull list 
*
*  SYNOPSIS
*
*     #include "cull_db.h"
*     #include <cull/src/cull_db.h>
* 
*     int lString2List(const char *s, lList **lpp, const lDescr *dp, int nm, 
*                      const char *delimitor); 
*
*  FUNCTION
*     parses separated strings and adds them into the cull list *lpp
*     the string is a unique key for the list and resides at field nm
*  
*     if delimitor==NULL
*        use isspace()
*     else
*        use delimitor
*
*  INPUTS
*     const char *s         - String to parse   
*     lList **lpp           - reference to lList*      
*     const lDescr *dp      - list Type     
*     int nm                - list field       
*     const char *delimitor - string delimitor        
*
*  RESULT
*     0 on error
*     1 ok
*
*  EXAMPLE
*     lList* stringList = NULL;
*     lString2List("host1, host2 host3", &stringList, ST_Type, STR, ", ");
******************************************************************************/
int lString2List(const char *s, lList **lpp, const lDescr *dp, int nm, 
                 const char *dlmt) {

   int pos;
   int dataType;


   DENTER(TOP_LAYER, "lString2List");

   if (!s) {
      DEXIT;
      return 1;
   }

   pos = lGetPosInDescr(dp, nm);
   dataType = lGetPosType(dp, pos);
   switch (dataType) {
      case lStringT:
         DPRINTF(("lString2List: got lStringT data type\n"));
         for (s = sge_strtok(s, dlmt); s; s = sge_strtok(NULL, dlmt)) {
            if (lGetElemStr(*lpp, nm, s)) {
               /* silently ignore multiple occurencies */
               continue;
            }
            if (!lAddElemStr(lpp, nm, s, dp)) {
               lFreeList(*lpp);
               *lpp = NULL;
               DEXIT;
               return 1;
            }
         }

         break;
      case lHostT:
         DPRINTF(("lString2List: got lHostT data type\n"));
         for (s = sge_strtok(s, dlmt); s; s = sge_strtok(NULL, dlmt)) {
            if (lGetElemHost(*lpp, nm, s)) {
               /* silently ignore multiple occurencies */
               continue;
            }
            if (!lAddElemHost(lpp, nm, s, dp)) {
               lFreeList(*lpp);
               *lpp = NULL;
               DEXIT;
               return 1;
            }
         }

         break;
      default:
         DPRINTF(("lString2List: unexpected data type\n"));
         break;
   }

   DEXIT;
   return 0;
}

int lString2ListNone(
const char *s,
lList **lpp,
const lDescr *dp,
int nm,
const char *dlmt 
) {
   int pos;
   int dataType;
   if (lString2List(s, lpp, dp, nm, dlmt))
      return 1;


   pos = lGetPosInDescr(dp, nm);
   dataType = lGetPosType(dp, pos);
   switch(dataType) {
      case lStringT:
         DPRINTF(("lString2ListNone: got lStringT data type\n"));
         if (lGetNumberOfElem(*lpp) > 1 && lGetElemCaseStr(*lpp, nm, "none")) {
            *lpp = lFreeList(*lpp);
            return 1;
         }

         if (lGetNumberOfElem(*lpp) == 1 && lGetElemCaseStr(*lpp, nm, "none"))
            *lpp = lFreeList(*lpp);
         break;
      case lHostT:
         DPRINTF(("lString2ListNone: got lHostT data type\n"));
         if (lGetNumberOfElem(*lpp) > 1 && lGetElemHost(*lpp, nm, "none")) {
            *lpp = lFreeList(*lpp);
            return 1;
         }

         if (lGetNumberOfElem(*lpp) == 1 && lGetElemHost(*lpp, nm, "none"))
            *lpp = lFreeList(*lpp);
         break;

      default:
         DPRINTF(("lString2ListNone: unexpected data type\n"));
         break;
   }

   return 0;
}

/* ----------------------------------------

   Remove elements in both lists with the
   same string key in field nm

   ---------------------------------------- */
int lDiffListStr(
int nm,
lList **lpp1,
lList **lpp2 
) {
   const char *key;
   lListElem *ep, *to_check, *to_del;

   DENTER(CULL_LAYER, "lDiffListStr");

   if (!lpp1 || !lpp2) {
      DEXIT;
      return -1;
   }

   if (!*lpp1 || !*lpp2) {
      DEXIT;
      return 0;
   }

   ep = lFirst(*lpp1);
   while (ep) {
      to_check = ep;
      key = lGetString(to_check, nm);

      ep = lNext(ep);           /* point to next element before del */

      if ((to_del = lGetElemStr(*lpp2, nm, key))) {
         lDelElemStr(lpp2, nm, key);
         lDelElemStr(lpp1, nm, key);
      }
   }

   DEXIT;
   return 0;
}
