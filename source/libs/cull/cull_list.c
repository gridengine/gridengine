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
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "msg_cull.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_sortP.h"
#include "cull_where.h"
#include "cull_listP.h"
#include "cull_multitypeP.h"
#include "cull_whatP.h"
#include "cull_lerrnoP.h"
#include "cull_hash.h"
#include "cull_state.h"
#include "pack.h"

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#define CULL_BASIS_LAYER CULL_LAYER


static void lWriteList_(const lList *lp, int nesting_level, FILE *fp);
static void lWriteElem_(const lListElem *lp, int nesting_level, FILE *fp);


/****** cull/list/-Field_Attributes *************************************************
*  NAME
*     Field_Attributes -- Attributes of cull type fields
*
*  SYNOPSIS
*     When a field of a cull object type is defined, any number of 
*     attributes can be given to the field.
*
*     The syntax of the field definition is
*        <typedef>(<field_name>, <attrib1> [|<attrib2> ...]
*     e.g.
*        SGE_STRING(QU_qname, CULL_HASH | CULL_UNIQUE)
*
*     The following attributes can be given to a field:
*        CULL_DEFAULT       
*           no special settings - default behaviour
*        CULL_PRIMARY_KEY   
*           the field is part of the primary key
*           not yet implemented
*        CULL_HASH          
*           a hash table will be created on the field for lists of the 
*           object type
*        CULL_UNIQUE        
*           the field value has to be unique for all objects in a list
*           currently only used for the definition of hash tables,
*           but it could be used for general consistency checks.
*        CULL_SHOW          
*           the field shall be shown in object output functions
*           not yet implemented
*        CULL_CONFIGURE     
*           the field can be changed by configuration functions
*           not yet implemented
*        CULL_SPOOL         
*           the field shall be spooled
*           not yet implemented
*
*  NOTES
*     Further attributes can be introduced as necessary, e.g.
*        CULL_ARRAY - the field is an array of the specified data type
*
*  SEE ALSO
*     cull/list/mt_get_type()
*     cull/list/mt_do_hashing()
*     cull/list/mt_is_unique()
******************************************************************************/

/****** cull/list/lCopyElem() *************************************************
*  NAME
*     lCopyElem() -- Copies a whole list element 
*
*  SYNOPSIS
*     lListElem* lCopyElem(const lListElem *ep) 
*
*  FUNCTION
*     Copies a whole list element 
*
*  INPUTS
*     const lListElem *ep - element 
*
*  RESULT
*     lListElem* - copy of 'ep'
******************************************************************************/
lListElem *lCopyElem(const lListElem *ep) 
{
   lListElem *new;
   lDescr *p;

   DENTER(CULL_LAYER, "lCopyElem");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   if (!(new = lCreateElem(ep->descr))) {
      LERROR(LECREATEELEM);
      DEXIT;
      return NULL;
   }

   /* for (i = 0; ep->descr[i].nm != NoName; i++)  */
   /*    if (lCopySwitch(ep, new, i, i) != 0)  */
      
   for (p = &(ep->descr[0]); p->nm != NoName; p++) {
      int index =  p - &(ep->descr[0]); 

      if (lCopySwitch(ep, new, index, index) != 0) {
         lFreeElem(new);
         LERROR(LECOPYSWITCH);
         DEXIT;
         return NULL;
      }
      /* copy changed field information */
      if(sge_bitfield_get(ep->changed, index)) {
         sge_bitfield_set(new->changed, index);
      }
   }

   new->status = FREE_ELEM;

   DEXIT;
   return new;
}

/****** cull/list/lModifyWhat() ************************************************
*  NAME
*     lModifyWhat() -- Copy parts of an element 
*
*  SYNOPSIS
*     int lModifyWhat(lListElem *dst, const lListElem *src, 
*                     const lEnumeration *enp) 
*
*  FUNCTION
*     Copies elements from 'src' to 'dst' using the enumeration 'enp'
*     as a mask or copies all elements if 'enp' is NULL 
*
*  INPUTS
*     lListElem *dst          - destination element
*     const lListElem *src    - source element
*     const lEnumeration *enp - mask 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lModifyWhat(lListElem *dst, const lListElem *src, const lEnumeration *enp) 
{
   int ret, i = 0;

   DENTER(CULL_LAYER, "lModifyWhat");

   ret = lCopyElemPartial(dst, &i, src, enp);

   DEXIT;
   return ret;
}

/****** cull/list/lCopyElemPartial() ****************************************
*  NAME
*     lCopyElemPartial() -- Copies parts of an element 
*
*  SYNOPSIS
*     int lCopyElemPartial(lListElem *dst, int *jp, 
*                          const lListElem *src, 
*                          const lEnumeration *enp) 
*
*  FUNCTION
*     Copies elements from list element 'src' to 'dst' using the
*     enumeration 'enp' as a mask or copies all elements if
*     'enp' is NULL. Copying starts at index *jp. 
*
*  INPUTS
*     lListElem *dst          - destination element 
*     int *jp                 - Where should the copy operation start 
*     const lListElem *src    - src element 
*     const lEnumeration *enp - enumeration 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
*******************************************************************************/
int lCopyElemPartial(lListElem *dst, int *jp, const lListElem *src, 
                     const lEnumeration *enp) 
{
   int i;

   DENTER(CULL_LAYER, "lCopyElemPartial");

   if (!enp || !dst || !jp) {
      LERROR(LEENUMNULL);
      DEXIT;
      return -1;
   }

   switch (enp[0].pos) {
   case WHAT_ALL:               /* all fields of element src is copied */
      for (i = 0; src->descr[i].nm != NoName; i++, (*jp)++) {
         if (lCopySwitch(src, dst, i, *jp) != 0) {
            LERROR(LECOPYSWITCH);
            DEXIT;
            return -1;
         }
         /* copy changed field information */
         if(sge_bitfield_get(src->changed, i)) {
            sge_bitfield_set(dst->changed, *jp);
         }
      }
      break;

   case WHAT_NONE:              /* no field of element src is copied */
      break;

   default:                     /* copy only the in enp enumerated elems */
      for (i = 0; enp[i].nm != NoName; i++, (*jp)++) {
         if (lCopySwitch(src, dst, enp[i].pos, *jp) != 0) {
            LERROR(LECOPYSWITCH);
            DEXIT;
            return -1;
         }
         /* copy changed field information */
         if(sge_bitfield_get(src->changed, enp[i].pos)) {
            sge_bitfield_set(dst->changed, *jp);
         }
      }
   }
   DEXIT;
   return 0;
}

/****** cull/list/lCopySwitch() ***********************************************
*  NAME
*     lCopySwitch() -- Copy parts of elements indedendent from type 
*
*  SYNOPSIS
*     int lCopySwitch(const lListElem *sep, lListElem *dep, 
*                     int src_idx, int dst_idx) 
*
*  FUNCTION
*     Copies from the element 'sep' (using index 'src_idx') to
*     the element 'dep' (using index 'dst_idx') in dependence 
*     of the type. 
*
*  INPUTS
*     const lListElem *sep - source element 
*     lListElem *dep       - destination element 
*     int src_idx          - source index 
*     int dst_idx          - destination index 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
*******************************************************************************/
int lCopySwitch(const lListElem *sep, lListElem *dep, 
                int src_idx, int dst_idx) 
{
   lList *tlp;
   lListElem *tep;

   DENTER(CULL_LAYER, "lCopySwitch");

   if (!dep || !sep) {
      DEXIT;
      return -1;
   }

   switch (mt_get_type(dep->descr[dst_idx].mt)) {
   case lUlongT:
      dep->cont[dst_idx].ul = sep->cont[src_idx].ul;
      break;
   case lStringT:
      if (!sep->cont[src_idx].str)
         dep->cont[dst_idx].str = NULL;
      else
         dep->cont[dst_idx].str = strdup(sep->cont[src_idx].str);
      break;
   case lHostT:
      if (!sep->cont[src_idx].host)
         dep->cont[dst_idx].host = NULL;
      else
         dep->cont[dst_idx].host = strdup(sep->cont[src_idx].host);
      break;

   case lDoubleT:
      dep->cont[dst_idx].db = sep->cont[src_idx].db;
      break;
   case lListT:
      if ((tlp = sep->cont[src_idx].glp) == NULL) 
         dep->cont[dst_idx].glp = NULL;
      else  
         dep->cont[dst_idx].glp = lCopyList(NULL, tlp);
      break;
   case lObjectT:
      if ((tep = sep->cont[src_idx].obj) == NULL) {
         dep->cont[dst_idx].obj = NULL;
      } else {
         lListElem *new = lCopyElem(tep);
         new->status = OBJECT_ELEM;
         dep->cont[dst_idx].obj = new;
      }   
      break;
   case lIntT:
      dep->cont[dst_idx].i = sep->cont[src_idx].i;            
      break;
   case lFloatT:
      dep->cont[dst_idx].fl = sep->cont[src_idx].fl;
      break;
   case lLongT:
      dep->cont[dst_idx].l = sep->cont[src_idx].l;
      break;
   case lCharT:
      dep->cont[dst_idx].c = sep->cont[src_idx].c;
      break;
   case lBoolT:
      dep->cont[dst_idx].b = sep->cont[src_idx].b;
      break;
   case lRefT:
      dep->cont[dst_idx].ref = sep->cont[src_idx].ref;
      break;
   default:
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

/****** cull/list/lGetListName() **********************************************
*  NAME
*     lGetListName() -- returns the user defined name of a list 
*
*  SYNOPSIS
*     const char* lGetListName(const lList *lp) 
*
*  FUNCTION
*     Returns the user defined name of a list. 
*
*  INPUTS
*     const lList *lp - list pointer 
*
*  RESULT
*     const char* - list name
******************************************************************************/
const char *lGetListName(const lList *lp) 
{
   DENTER(CULL_LAYER, "lGetListName");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return "No List specified";
   }

   if(!lp->listname) {
      LERROR(LENULLSTRING);
      DEXIT;
      return "No list name specified";
   }

   DEXIT;
   return lp->listname;
}

/****** cull/list/lGetListDescr() *********************************************
*  NAME
*     lGetListDescr() -- Returns the descriptor of a list 
*
*  SYNOPSIS
*     const lDescr* lGetListDescr(const lList *lp) 
*
*  FUNCTION
*     Returns the descriptor of a list 
*
*  INPUTS
*     const lList *lp - list pointer 
*
*  RESULT
*     const lDescr* - destriptor 
******************************************************************************/
const lDescr *lGetListDescr(const lList *lp) 
{
   DENTER(CULL_LAYER, "lGetListDescr");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return NULL;
   }
   DEXIT;
   return lp->descr;
}

/****** cull/list/lGetNumberOfElem() ******************************************
*  NAME
*     lGetNumberOfElem() -- Returns the number of elements in a list 
*
*  SYNOPSIS
*     int lGetNumberOfElem(const lList *lp) 
*
*  FUNCTION
*     Returns the number of elements in a list 
*
*  INPUTS
*     const lList *lp - list pointer 
*
*  RESULT
*     int - number of elements 
******************************************************************************/
int lGetNumberOfElem(const lList *lp) 
{
   DENTER(CULL_LAYER, "lGetNumberOfElem");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return 0;
   }

   DEXIT;
   return lp->nelem;
}

/****** cull/list/lGetElemIndex() *********************************************
*  NAME
*     lGetElemIndex() -- returns the index of element in list lp 
*
*  SYNOPSIS
*     int lGetElemIndex(const lListElem* ep, const lList* lp) 
*
*  FUNCTION
*     returns the index of element in list lp 
*
*  INPUTS
*     lListElem* ep - element 
*     lList* lp     - list 
*
*  RESULT
*     index number 
******************************************************************************/
int lGetElemIndex(const lListElem *ep, const lList *lp) 
{
   int i = -1;
   lListElem *ep2;

   DENTER(CULL_LAYER, "lGetElemIndex");

   if (!ep || ep->status != BOUND_ELEM) {
      DEXIT;
      return -1;
   }

   for_each(ep2, lp) {
      i++;
      if (ep2 == ep)
         break;
   }

   DEXIT;
   return i;
}

/****** cull/list/lGetNumberOfRemainingElem() *********************************
*  NAME
*     lGetNumberOfRemainingElem() -- Number of following elements 
*
*  SYNOPSIS
*     int lGetNumberOfRemainingElem(const lListElem *ep) 
*
*  FUNCTION
*     Returns the number of elements behind an element 
*
*  INPUTS
*     const lListElem *ep - element 
*
*  RESULT
*     int - number of elements 
******************************************************************************/
int lGetNumberOfRemainingElem(const lListElem *ep) 
{
   int i = 0;

   DENTER(CULL_LAYER, "lGetNumberOfElem");

   if (!ep) {
      DEXIT;
      return 0;
   }

   while ((ep = lNext(ep)))
      i++;

   DEXIT;
   return i;
}

/****** cull/list/lGetElemDescr() **********************************************
*  NAME
*     lGetElemDescr() -- returns the descriptor of a list element 
*
*  SYNOPSIS
*     const lDescr* lGetElemDescr(const lListElem* ep) 
*
*  FUNCTION
*     returns the descriptor of a list element 
*
*  INPUTS
*     lListElem* ep - CULL element 
*
*  RESULT
*     Pointer to descriptor 
*
******************************************************************************/
const lDescr *lGetElemDescr(const lListElem *ep) 
{
   DENTER(CULL_LAYER, "lGetListDescr");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }
   DEXIT;
   return ep->descr;
}

/****** cull/list/lWriteElem() ************************************************
*  NAME
*     lWriteElem() -- Write a element to monitoring level CULL_LAYER 
*
*  SYNOPSIS
*     void lWriteElem(const lListElem *ep) 
*
*  FUNCTION
*     Write a element to monitoring level CULL_LAYER a info message 
*
*  INPUTS
*     const lListElem *ep - element 
******************************************************************************/
void lWriteElem(const lListElem *ep) 
{
   DENTER(CULL_LAYER, "lWriteElem");

   lWriteElem_(ep, 0, NULL);

   DEXIT;
}

/****** cull/list/lWriteElemTo() **********************************************
*  NAME
*     lWriteElemTo() -- Write a element to file stream 
*
*  SYNOPSIS
*     void lWriteElemTo(const lListElem *ep, FILE *fp) 
*
*  FUNCTION
*     Write a element to file stream 
*
*  INPUTS
*     const lListElem *ep - element 
*     FILE *fp            - file stream 
*     ???/???
******************************************************************************/
void lWriteElemTo(const lListElem *ep, FILE *fp) 
{
   DENTER(CULL_LAYER, "lWriteElemTo");

   lWriteElem_(ep, 0, fp);

   DEXIT;
}

static void lWriteElem_(const lListElem *ep, int nesting_level, FILE *fp) 
{
   int i;
   char space[128];
   lList *tlp;
   lListElem *tep;
   const char *str;

   DENTER(CULL_LAYER, "lWriteElem");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return;
   }

   for (i = 0; i < nesting_level * 3; i++)
      space[i] = ' ';
   space[i] = '\0';

   if (!fp) {
      DPRINTF(("%s-------------------------------\n", space));
   } else {
      fprintf(fp, "%s-------------------------------\n", space);
   }

   for (i = 0; ep->descr[i].mt != lEndT; i++) {
      int changed = sge_bitfield_get(ep->changed, i);
      switch (mt_get_type(ep->descr[i].mt)) {
      case lIntT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Integer) %c = %d\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosInt(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Integer) %c = %d\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosInt(ep, i));
         }
         break;
      case lUlongT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Ulong)   %c = " u32"\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosUlong(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Ulong)   %c = " u32"\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosUlong(ep, i));
         }
         break;
      case lStringT:
         str = lGetPosString(ep, i);
         if (!fp) {
            DPRINTF(("%s%-20.20s (String)  %c = %s\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', str ? str : "(null)"));
         } else {
            fprintf(fp, "%s%-20.20s (String)  %c = %s\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', str ? str : "(null)");
         }
         break;

      case lHostT:
         str = lGetPosHost(ep, i);
         if (!fp) {
            DPRINTF(("%s%-20.20s (Host)    %c = %s\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', str ? str : "(null)"));
         } else {
            fprintf(fp, "%s%-20.20s (Host)    %c = %s\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', str ? str : "(null)");
         }
         break;

      case lListT:
         tlp = lGetPosList(ep, i);
         if (!fp) {
            DPRINTF(("%s%-20.20s (List)    %c = %s\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', tlp ? "full {" : "empty"));
         } else {
            fprintf(fp, "%s%-20.20s (List)    %c = %s\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', tlp ? "full {" : "empty");
         }
         if (tlp) {
            lWriteList_(tlp, nesting_level + 1, fp);
            if (!fp) {
               DPRINTF(("%s}\n", space));
            } else {
               fprintf(fp, "%s}\n", space);
            }
         }
         break;

      case lObjectT:
         tep = lGetPosObject(ep, i);
         if (!fp) {
            DPRINTF(("%s%-20.20s (Object)  %c = %s\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', tep ? "object {" : "none"));
         } else {
            fprintf(fp, "%s%-20.20s (Object)  %c = %s\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', tep ? "object {" : "none");
         }
         if (tep) {
            lWriteElem_(tep, nesting_level + 1, fp);
            if (!fp) {
               DPRINTF(("%s}\n", space));
            } else {
               fprintf(fp, "%s}\n", space);
            }
         }
         break;
      case lFloatT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Float)   %c = %f\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosFloat(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Float)   %c = %f\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosFloat(ep, i));
         }
         break;
      case lDoubleT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Double)  %c = %f\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosDouble(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Double)  %c = %f\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosDouble(ep, i));
         }
         break;
      case lLongT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Long)    %c = %ld\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosLong(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Long)    %c = %ld\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosLong(ep, i));
         }
         break;
      case lBoolT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Bool)    %c = %s\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosBool(ep, i) ? "true" : "false"));
         } else {
            fprintf(fp, "%s%-20.20s (Bool)    %c = %s\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosBool(ep, i) ? "true" : "false");
         }
         break;
      case lCharT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Char)    %c = %c\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosChar(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Char)    %c = %c\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosChar(ep, i));
         }
         break;
      case lRefT:
         if (!fp) {
            DPRINTF(("%s%-20.20s (Ref)     %c = %p\n", space,
                     lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosRef(ep, i)));
         } else {
            fprintf(fp, "%s%-20.20s (Ref)     %c = %p\n", space,
                    lNm2Str(ep->descr[i].nm), changed ? '*' : ' ', lGetPosRef(ep, i));
         }
         break;
      default:
         unknownType("lWriteElem");
      }
   }

   DEXIT;
   return;
}

/****** cull/list/lWriteList() ************************************************
*  NAME
*     lWriteList() -- Write a list to monitoring level CULL_LAYER 
*
*  SYNOPSIS
*     void lWriteList(const lList *lp) 
*
*  FUNCTION
*     Write a list to monitoring level CULL_LAYER as info message. 
*
*  INPUTS
*     const lList *lp - list 
******************************************************************************/
void lWriteList(const lList *lp) 
{
   DENTER(CULL_LAYER, "lWriteList");

   lWriteList_(lp, 0, NULL);

   DEXIT;
   return;
}

/****** cull/list/lWriteListTo() **********************************************
*  NAME
*     lWriteListTo() -- Write a list to a file stream 
*
*  SYNOPSIS
*     void lWriteListTo(const lList *lp, FILE *fp) 
*
*  FUNCTION
*     Write a list to a file stream 
*
*  INPUTS
*     const lList *lp - list 
*     FILE *fp        - file stream 
*******************************************************************************/
void lWriteListTo(const lList *lp, FILE *fp) 
{
   DENTER(CULL_LAYER, "lWriteListTo");

   lWriteList_(lp, 0, fp);

   DEXIT;
   return;
}

static void lWriteList_(const lList *lp, int nesting_level, FILE *fp) 
{
   lListElem *ep;
   char indent[128];
   int i;

   DENTER(CULL_LAYER, "lWriteList_");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return;
   }

   for (i = 0; i < nesting_level * 3; i++)
      indent[i] = ' ';
   indent[i] = '\0';

   if (!fp) {
      DPRINTF(("\n%sList: <%s> %c #Elements: %d\n",
               indent,
               lGetListName(lp),
               lp->changed ? '*' : ' ',
               lGetNumberOfElem(lp)));
   }
   else {
      fprintf(fp, "\n%sList: <%s> %c #Elements: %d\n",
              indent,
              lGetListName(lp),
              lp->changed ? '*' : ' ',
              lGetNumberOfElem(lp));
   }

   for_each(ep, lp) {
      if (!fp)
         lWriteElem_(ep, nesting_level, NULL);
      else
         lWriteElem_(ep, nesting_level, fp);
   }
   DEXIT;
}

/****** cull/list/lCreateElem() ***********************************************
*  NAME
*     lCreateElem() -- Create an element for a specific list 
*
*  SYNOPSIS
*     lListElem* lCreateElem(const lDescr *dp) 
*
*  FUNCTION
*     Create an element for a specific list 
*
*  INPUTS
*     const lDescr *dp - descriptor 
*
*  RESULT
*     lListElem* - element pointer or NULL
******************************************************************************/
lListElem *lCreateElem(const lDescr *dp) 
{
   int n, i;
   lListElem *ep;

   DENTER(CULL_LAYER, "lCreateElem");

   if ((n = lCountDescr(dp)) <= 0) {
      LERROR(LECOUNTDESCR);
      DEXIT;
      return NULL;
   }

   if (!(ep = (lListElem *) malloc(sizeof(lListElem)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   ep->next = NULL;
   ep->prev = NULL;
 
   ep->descr = (lDescr *) malloc(sizeof(lDescr) * (n + 1));
   if (!ep->descr) {
      LERROR(LEMALLOC);
      free(ep);
      DEXIT;
      return NULL;
   }
   memcpy(ep->descr, dp, sizeof(lDescr) * (n + 1));

   /* new descr has no htables yet */
   for (i = 0; i <= n; i++) {
      ep->descr[i].ht = NULL;
   }
                  
   ep->status = FREE_ELEM;
   if (!(ep->cont = (lMultiType *) calloc(1, sizeof(lMultiType) * n))) {
      LERROR(LEMALLOC);
      free(ep->descr);
      free(ep);
      DEXIT;
      return NULL;
   }

   if((ep->changed = sge_bitfield_new(n)) == NULL) {
      LERROR(LEMALLOC);
      free(ep->cont);
      free(ep->descr);
      free(ep);
      return NULL;
   }

   DEXIT;
   return ep;
}

/****** cull/list/lCreateList() ***********************************************
*  NAME
*     lCreateList() -- Create an empty list 
*
*  SYNOPSIS
*     lList* lCreateList(const char *listname, const lDescr *descr) 
*
*  FUNCTION
*     Create an empty list with a given descriptor and a user defined
*     listname. 
*
*  INPUTS
*     const char *listname - list name 
*     const lDescr *descr  - descriptor 
*
*  RESULT
*     lList* - list pointer or NULL 
*******************************************************************************/
lList *lCreateList(const char *listname, const lDescr *descr) 
{
   lList *lp;
   int i, n;

   DENTER(CULL_LAYER, "lCreateList");

   if (!listname) {
      listname = "No list name specified";
   }

   if (!descr || descr[0].mt == lEndT) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (!(lp = (lList *) malloc(sizeof(lList)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }
   if (!(lp->listname = strdup(listname))) {
      LERROR(LESTRDUP);
      DEXIT;
      return NULL;
   }

   lp->nelem = 0;
   if ((n = lCountDescr(descr)) <= 0) {
      LERROR(LECOUNTDESCR);
      DEXIT;
      return NULL;
   }

   lp->first = NULL;
   lp->last = NULL;
   if (!(lp->descr = (lDescr *) malloc(sizeof(lDescr) * (n + 1)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }
   /* copy descriptor array */
   for (i = 0; i <= n; i++) {
      lp->descr[i].mt = descr[i].mt;
      lp->descr[i].nm = descr[i].nm;

      /* create hashtable if necessary */
      if(mt_do_hashing(lp->descr[i].mt)) {
         lp->descr[i].ht = cull_hash_create(&descr[i]);
      } else {
         lp->descr[i].ht = NULL;
      }
   }

   lp->changed = false;

   DEXIT;
   return lp;
}

/****** cull/list/lCreateElemList() *******************************************
*  NAME
*     lCreateElemList() -- Create a list with n elements 
*
*  SYNOPSIS
*     lList* lCreateElemList(const char *listname, const lDescr *descr, 
*                            int nr_elem) 
*
*  FUNCTION
*     Create a list with a given descriptor and insert 'nr_elem' 
*     only initialized elements 
*
*  INPUTS
*     const char *listname - list name
*     const lDescr *descr  - descriptor 
*     int nr_elem          - number of elements 
*
*  RESULT
*     lList* - list or NULL
*******************************************************************************/
lList *lCreateElemList(const char *listname, const lDescr *descr, int nr_elem) 
{
   lList *lp = NULL;
   lListElem *ep = NULL;
   int i;

   DENTER(CULL_LAYER, "lCreateElemList");

   if (!(lp = lCreateList(listname, descr))) {
      LERROR(LECREATELIST);
      DEXIT;
      return NULL;
   }

   for (i = 0; i < nr_elem; i++) {
      if (!(ep = lCreateElem(descr))) {
         LERROR(LECREATEELEM);
         lFreeList(lp);
         DEXIT;
         return NULL;
      }
      lAppendElem(lp, ep);
   }

   DEXIT;
   return lp;
}

/****** cull/list/lFreeElem() *************************************************
*  NAME
*     lFreeElem() -- Free a element including strings and sublists 
*
*  SYNOPSIS
*     lListElem* lFreeElem(lListElem *ep) 
*
*  FUNCTION
*     Free a element including strings and sublists 
*
*  INPUTS
*     lListElem *ep - element 
*
*  RESULT
*     lListElem* - NULL 
******************************************************************************/
lListElem *lFreeElem(lListElem *ep) 
{
   int i = 0;

   DENTER(CULL_LAYER, "lFreeElem");

   if (ep == NULL) {
      DEXIT;
      return NULL;
   }

   if (ep->descr == NULL) {
      LERROR(LEDESCRNULL);
      DPRINTF(("NULL descriptor not allowed !!!\n"));
      abort();
   }

   for (i = 0; ep->descr[i].mt != lEndT; i++) {
      /* remove element from hash tables */
      if(ep->descr[i].ht != NULL) {
         cull_hash_remove(ep, i);
      }
      
      switch (mt_get_type(ep->descr[i].mt)) {

      case lIntT:
      case lUlongT:
      case lFloatT:
      case lDoubleT:
      case lLongT:
      case lCharT:
      case lBoolT:
      case lRefT:
         break;

      case lStringT:
         if (ep->cont[i].str != NULL)
            free(ep->cont[i].str);
         break;

      case lHostT:
         if (ep->cont[i].host != NULL)
            free(ep->cont[i].host);
         break;

      case lListT:
         if (ep->cont[i].glp != NULL)
            lFreeList(ep->cont[i].glp);
         break;

      case lObjectT:
         if (ep->cont[i].obj != NULL)
            lFreeElem(ep->cont[i].obj);
         break;

      default:
         unknownType("lFreeElem");
         break;
      }
   }

   /* lFreeElem is not responsible for list descriptor array */
   if(ep->status == FREE_ELEM || ep->status == OBJECT_ELEM) {
      cull_hash_free_descr(ep->descr); 
      free(ep->descr);
   }   

   if(ep->cont != NULL) {
      free(ep->cont);
   }   

   if(ep->changed != NULL) {
      ep->changed = sge_bitfield_free(ep->changed);
   }

   free(ep);

   DEXIT;
   return NULL;
}

/****** cull/list/lFreeList() *************************************************
*  NAME
*     lFreeList() -- Frees a list including all elements  
*
*  SYNOPSIS
*     lList* lFreeList(lList *lp) 
*
*  FUNCTION
*     Frees a list including all elements 
*
*  INPUTS
*     lList *lp - list 
*
*  RESULT
*     lList* - NULL 
******************************************************************************/
lList *lFreeList(lList *lp) 
{
   DENTER(CULL_LAYER, "lFreeList");

   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* 
    * remove all hash tables, 
    * it is more efficient than removing it at the end 
    */
   if (lp->descr != NULL) {
      cull_hash_free_descr(lp->descr);
   }   

   while (lp->first) {
      lRemoveElem(lp, lp->first);
   }   

   if (lp->descr) {
      free(lp->descr);
   }   

   if (lp->listname) {
      free(lp->listname);
   }

   free(lp);

   DEXIT;
   return NULL;
}

/****** cull/list/lAddList() **************************************************
*  NAME
*     lAddList() -- Concatenate two lists 
*
*  SYNOPSIS
*     int lAddList(lList *lp0, lList *lp1) 
*
*  FUNCTION
*     Concatenate two lists of equal type throwing away the second list 
*
*  INPUTS
*     lList *lp0 - first list 
*     lList *lp1 - second list 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lAddList(lList *lp0, lList *lp1) 
{
   lListElem *ep;
   const lDescr *dp0, *dp1;

   DENTER(CULL_LAYER, "lAddList");

   if (!lp1 || !lp0) {
      LERROR(LELISTNULL);
      DEXIT;
      return -1;
   }

   /* Check if the two lists are equal */
   dp0 = lGetListDescr(lp0);
   dp1 = lGetListDescr(lp1);
   if (lCompListDescr(dp0, dp1)) {
      LERROR(LEDIFFDESCR);
      DEXIT;
      return -1;
   }

   while (lp1->first) {
      if (!(ep = lDechainElem(lp1, lp1->first))) {
         LERROR(LEDECHAINELEM);
         DEXIT;
         return -1;
      }
      if (lAppendElem(lp0, ep) == -1) {
         LERROR(LEAPPENDELEM);
         DEXIT;
         return -1;
      }
   }

   lFreeList(lp1);

   DEXIT;
   return 0;
}

/****** cull/list/lCompListDescr() ********************************************
*  NAME
*     lCompListDescr() -- Compare two descriptors 
*
*  SYNOPSIS
*     int lCompListDescr(const lDescr *dp0, const lDescr *dp1) 
*
*  FUNCTION
*     Compare two descriptors 
*
*  INPUTS
*     const lDescr *dp0 - descriptor one 
*     const lDescr *dp1 - descriptor two 
*
*  RESULT
*     int - Result of compare operation
*         0 - equivalent
*        -1 - not equivalent
******************************************************************************/
int lCompListDescr(const lDescr *dp0, const lDescr *dp1) 
{
   int i, n, m;

   DENTER(CULL_LAYER, "lCompListDescr");

   if (!dp0 || !dp1) {
      LERROR(LELISTNULL);
      DEXIT;
      return -1;
   }

   /* Check if the two lists are equal */
   if ((n = lCountDescr(dp0)) <= 0) {
      LERROR(LECOUNTDESCR);
      DEXIT;
      return -1;
   }
   if ((m = lCountDescr(dp1)) <= 0) {
      LERROR(LECOUNTDESCR);
      DEXIT;
      return -1;
   }
   if (n == m) {
      for (i = 0; i < n; i++) {
         /* 
          * comparing the mt field might be too restrictive
          * former implementation mt only contained the type information 
          * now also the various flags 
          */
         if (dp0[i].nm != dp1[i].nm || dp0[i].mt != dp1[i].mt) {
            LERROR(LEDIFFDESCR);
            DEXIT;
            return -1;
         }
      }
   }
   else {
      LERROR(LEDIFFDESCR);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

/****** cull/list/lCopyList() *************************************************
*  NAME
*     lCopyList() -- Copy a list including strings and sublists 
*
*  SYNOPSIS
*     lList* lCopyList(const char *name, const lList *src) 
*
*  FUNCTION
*     Copy a list including strings and sublists. The new list will
*     get 'name' as user defined name 
*
*  INPUTS
*     const char *name - list name 
*     const lList *src - source list 
*
*  RESULT
*     lList* - Copy of 'src' or NULL 
******************************************************************************/
lList *lCopyList(const char *name, const lList *src) 
{
   lList *dst = NULL;
   lListElem *sep;

   DENTER(CULL_LAYER, "lCopyList");

   if (!src) {
      LERROR(LELISTNULL);
      DEXIT;
      return NULL;
   }

   if (!name)
      name = src->listname;

   if (!name)
      name = "No list name specified";


   if (!(dst = lCreateList(name, src->descr))) {
      LERROR(LECREATELIST);
      DEXIT;
      return NULL;
   }

   for (sep = src->first; sep; sep = sep->next)
      if (lAppendElem(dst, lCopyElem(sep)) == -1) {
         lFreeList(dst);
         LERROR(LEAPPENDELEM);
         DEXIT;
         return NULL;
      }

   DEXIT;
   return dst;
}

/****** cull/list/lInsertElem() ***********************************************
*  NAME
*     lInsertElem() -- Insert element after another in a list 
*
*  SYNOPSIS
*     int lInsertElem(lList *lp, lListElem *ep, lListElem *new) 
*
*  FUNCTION
*     Insert a 'new' element after element 'ep' into list 'lp'.
*     If 'ep' is NULL then 'new' will be the first element in 'lp'.
*
*  INPUTS
*     lList *lp      - list 
*     lListElem *ep  - element 
*     lListElem *new - new element 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lInsertElem(lList *lp, lListElem *ep, lListElem *new) 
{
   DENTER(CULL_LAYER, "lInsertElem");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return -1;
   }
   
   if (!new) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   
   /* is the element new still chained in an other list, this is not allowed ? */
   if (new->status == BOUND_ELEM || new->status == OBJECT_ELEM) {
      DPRINTF(("WARNING: tried to insert chained element\n"));
      lWriteElem(new);
      DEXIT;
      abort();
   }

   if (ep) {
      new->prev = ep;
      new->next = ep->next;
      ep->next = new;
      if (new->next)            /* the new element has successors */
         new->next->prev = new;
      else                      /* the new element is the last element */
         lp->last = new;
   }
   else {                       /* insert as first element */
      new->prev = NULL;
      new->next = lp->first;
      if (!lp->first)           /* empty list ? */
         lp->last = new;
      else
         lp->first->prev = new;
      lp->first = new;
   }

   if (new->status == FREE_ELEM) {
      cull_hash_free_descr(new->descr);
      free(new->descr);
   }   
   new->status = BOUND_ELEM;
   new->descr = lp->descr;

   cull_hash_elem(new);
   
   lp->nelem++;
   lp->changed = true;

   DEXIT;
   return 0;
}

/****** cull/list/lAppendElem() ***********************************************
*  NAME
*     lAppendElem() -- Append element at the end of a list 
*
*  SYNOPSIS
*     int lAppendElem(lList *lp, lListElem *ep) 
*
*  FUNCTION
*     Append element 'ep' at the end of list 'lp' 
*
*  INPUTS
*     lList *lp     - list 
*     lListElem *ep - element 
*
*  RESULT
*     int - error state 
*         0 - OK
*        -1 - Error
******************************************************************************/
int lAppendElem(lList *lp, lListElem *ep) 
{
#ifdef __INSIGHT__
/* JG: this code is thorougly tested and really should be ok, but insure complains */
_Insight_set_option("suppress", "LEAK_ASSIGN");
#endif
   DENTER(CULL_LAYER, "lAppendElem");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return -1;
   }
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   /* is the element ep still chained in an other list, this is not allowed ? */
   if (ep->status == BOUND_ELEM || ep->status == OBJECT_ELEM) {
      DPRINTF(("WARNING: tried to append chained element\n"));
      lWriteElem(ep);
      DEXIT;
      abort();
   }

   if (lp->last) {
      lp->last->next = ep;
      ep->prev = lp->last;
      lp->last = ep;
      ep->next = NULL;
   }
   else {
      lp->last = lp->first = ep;
      ep->prev = ep->next = NULL;
   }

   if (ep->status == FREE_ELEM) {
      cull_hash_free_descr(ep->descr);
      free(ep->descr);
   }
   ep->status = BOUND_ELEM;
   ep->descr = lp->descr;

   cull_hash_elem(ep);
   lp->nelem++;
   lp->changed = true;

   DEXIT;
   return 0;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "LEAK_ASSIGN");
#endif
}

/****** cull/list/lRemoveElem() ***********************************************
*  NAME
*     lRemoveElem() -- Delete a element from a list 
*
*  SYNOPSIS
*     int lRemoveElem(lList *lp, lListElem *ep) 
*
*  FUNCTION
*     Remove element 'ep' from list 'lp'. 'ep' gets deleted.
*
*  INPUTS
*     lList *lp     - list 
*     lListElem *ep - element 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error 
*******************************************************************************/
int lRemoveElem(lList *lp, lListElem *ep) 
{
   DENTER(CULL_LAYER, "lRemoveElem");

   if (!lp || !ep) {
      DEXIT;
      return -1;
   }

   if (lp->descr != ep->descr) {
      DPRINTF(("Dechaining element from other list !!!\n"));
      abort();
   }

   if (ep->prev) {
      ep->prev->next = ep->next;
   }
   else {
      lp->first = ep->next;
   }

   if (ep->next) {
      ep->next->prev = ep->prev;
   }
   else {
      lp->last = ep->prev;
   }

   /* NULL the ep next and previous pointers */
   ep->prev = ep->next = NULL;

   lp->nelem--;
   lp->changed = true;

   lFreeElem(ep);

   DEXIT;
   return 0;
}

/****** cull/list/lDechainElem() **********************************************
*  NAME
*     lDechainElem() -- Remove a element from a list 
*
*  SYNOPSIS
*     lListElem* lDechainElem(lList *lp, lListElem *ep) 
*
*  FUNCTION
*     Remove element 'ep' from list 'lp'. 'ep' gets not deleted. 
*
*  INPUTS
*     lList *lp     - list 
*     lListElem *ep - element 
*
*  RESULT
*     lListElem* - dechained element or NULL
******************************************************************************/
lListElem *lDechainElem(lList *lp, lListElem *ep) 
{
   int i;

   DENTER(CULL_LAYER, "lDechainElem");

   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return NULL;
   }
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }
   if (lp->descr != ep->descr) {
      DPRINTF(("Dechaining element from other list !!!\n"));
      abort();
   }

   if (ep->prev)
      ep->prev->next = ep->next;
   else
      lp->first = ep->next;

   if (ep->next)
      ep->next->prev = ep->prev;
   else
      lp->last = ep->prev;

   /* remove hash entries */
   for(i = 0; ep->descr[i].mt != lEndT; i++) {
      if(ep->descr[i].ht != NULL) {
         cull_hash_remove(ep, i);
      }
   }

   /* NULL the ep next and previous pointers */
   ep->prev = ep->next = (lListElem *) NULL;
   ep->descr = lCopyDescr(ep->descr);
   ep->status = FREE_ELEM;
   lp->nelem--;
   lp->changed = true;

   DEXIT;
   return ep;
}

/****** cull/list/lDechainObject() **********************************************
*  NAME
*     lDechainObject() -- Remove a element from a list 
*
*  SYNOPSIS
*     lListElem* lDechainObject(lList *lp, int name) 
*
*  FUNCTION
*     Remove element 'ep' from list 'lp'. 'ep' gets not deleted. 
*
*  INPUTS
*     lList *lp     - list 
*     int name      - attribute name 
*
*  RESULT
*     lListElem* - dechained element or NULL
******************************************************************************/
lListElem *lDechainObject(lListElem *parent, int name) 
{
   int pos;
   lListElem *dep;

   DENTER(CULL_LAYER, "lDechainObject");

   if (parent == NULL) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   pos = lGetPosViaElem(parent, name);
   if (pos < 0) {
      /* someone has called lDechainObject() with an invalid name */
      CRITICAL((SGE_EVENT, MSG_CULL_DECHAINOBJECT_XNOTFOUNDINELEMENT_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }

   if(mt_get_type(parent->descr[pos].mt) != lObjectT) {
      incompatibleType2(MSG_CULL_DECHAINOBJECT_WRONGTYPEFORFIELDXY_S,
                        lNm2Str(name));
      DEXIT;
      abort();
   }
  
   dep = (lListElem *) parent->cont[pos].obj;

   if(dep != NULL) {
      dep->status = FREE_ELEM;
      parent->cont[pos].obj = NULL;

      /* remember that field changed */
      sge_bitfield_set(parent->changed, pos);
   }

   DEXIT;
   return dep;
}

/****** cull/list/lFirst() ****************************************************
*  NAME
*     lFirst() -- Return the first element of a list 
*
*  SYNOPSIS
*     lListElem* lFirst(const lList *slp) 
*
*  FUNCTION
*     Return the first element of a list. 
*
*  INPUTS
*     const lList *slp - list 
*
*  RESULT
*     lListElem* - first element or NULL 
******************************************************************************/
lListElem *lFirst(const lList *slp) 
{
   DENTER(CULL_LAYER, "lFirst");
   DEXIT;
   return slp ? slp->first : NULL;
}

/****** cull/list/lLast() *****************************************************
*  NAME
*     lLast() -- Returns the last element of a list 
*
*  SYNOPSIS
*     lListElem* lLast(const lList *slp) 
*
*  FUNCTION
*     Returns the last element of a list. 
*
*  INPUTS
*     const lList *slp - list 
*
*  RESULT
*     lListElem* - last element or NULL 
******************************************************************************/
lListElem *lLast(const lList *slp) 
{
   DENTER(CULL_LAYER, "lLast");
   DEXIT;
   return slp ? slp->last : NULL;
}

/****** cull/list/lNext() *****************************************************
*  NAME
*     lNext() -- Returns the next element or NULL
*
*  SYNOPSIS
*     lListElem* lNext(const lListElem *sep) 
*
*  FUNCTION
*     Returns the next element of 'sep' or NULL 
*
*  INPUTS
*     const lListElem *sep - element 
*
*  RESULT
*     lListElem* - next element or NULL 
*******************************************************************************/
lListElem *lNext(const lListElem *sep) 
{
   DENTER(CULL_LAYER, "lNext");
   DEXIT;
   return sep ? sep->next : NULL;
}

/****** cull/list/lPrev() *****************************************************
*  NAME
*     lPrev() -- Returns the previous element or NULL 
*
*  SYNOPSIS
*     lListElem* lPrev(const lListElem *sep) 
*
*  FUNCTION
*     Returns the previous element or NULL. 
*
*  INPUTS
*     const lListElem *sep - element 
*
*  RESULT
*     lListElem* - previous element 
******************************************************************************/
lListElem *lPrev(const lListElem *sep) 
{
   DENTER(CULL_LAYER, "lPrev");
   DEXIT;
   return sep ? sep->prev : NULL;
}

/****** cull/list/lFindFirst() ************************************************
*  NAME
*     lFindFirst() -- Returns first element fulfilling condition 
*
*  SYNOPSIS
*     lListElem* lFindFirst(const lList *slp, const lCondition *cp) 
*
*  FUNCTION
*     Returns the first element fulfilling the condition 'cp' or
*     NULL if nothing is found. If the condition is NULL the first
*     element is delivered. 
*
*  INPUTS
*     const lList *slp     - list 
*     const lCondition *cp - condition 
*
*  RESULT
*     lListElem* - element or NULL 
******************************************************************************/
lListElem *lFindFirst(const lList *slp, const lCondition *cp) 
{
   lListElem *ep;

   DENTER(CULL_LAYER, "lFindFirst");

   if (!slp) {
      LERROR(LELISTNULL);
      DEXIT;
      return NULL;
   }

   /* ep->next=NULL for the last element */
   for (ep = slp->first; ep && !lCompare(ep, cp); ep = ep->next);

   DEXIT;
   return ep;
}

/****** cull/list/lFindLast() *************************************************
*  NAME
*     lFindLast() -- Returns last element fulfilling condition 
*
*  SYNOPSIS
*     lListElem* lFindLast(const lList *slp, const lCondition *cp) 
*
*  FUNCTION
*     Retruns the last element fulfilling the condition 'cp' or NULL
*     if nothing is found. If the condition is NULL then the last
*     element is delivered.
*
*  INPUTS
*     const lList *slp     - list 
*     const lCondition *cp - condition 
*
*  RESULT
*     lListElem* - element or NULL
******************************************************************************/
lListElem *lFindLast(const lList *slp, const lCondition *cp) 
{
   lListElem *ep;

   DENTER(CULL_LAYER, "lFindLast");

   if (!slp) {
      LERROR(LELISTNULL);
      DEXIT;
      return NULL;
   }

   /* ep->prev=NULL for the first element */
   for (ep = slp->last; ep && !lCompare(ep, cp); ep = ep->prev);

   DEXIT;
   return ep;
}

/****** cull/list/lFindNext() *************************************************
*  NAME
*     lFindNext() -- Returns the next element fulfilling condition 
*
*  SYNOPSIS
*     lListElem* lFindNext(const lListElem *ep, const lCondition *cp) 
*
*  FUNCTION
*     Returns the next element fulfilling the condition 'cp' or NULL
*     if nothing is found. If condition is NULL than the following
*     element is delivered. 
*
*  INPUTS
*     const lListElem *ep  - element 
*     const lCondition *cp - condition 
*
*  RESULT
*     lListElem* - element or NULL 
*******************************************************************************/
lListElem *lFindNext(const lListElem *ep, const lCondition *cp) 
{
   DENTER(CULL_LAYER, "lFindNext");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   do {
      ep = ep->next;
   } while (ep && (lCompare(ep, cp) == 0));

   DEXIT;
   return (lListElem *)ep;
}

/****** cull/list/lFindPrev() *************************************************
*  NAME
*     lFindPrev() -- Returns previous element fulfilling condition 
*
*  SYNOPSIS
*     lListElem* lFindPrev(const lListElem *ep, const lCondition *cp) 
*
*  FUNCTION
*     Returns the previous element fulfilling the condition 'cp' or
*     NULL if nothing is found. If condition is NULL than the following
*     element is delivered. 
*
*  INPUTS
*     const lListElem *ep  - element 
*     const lCondition *cp - condition 
*
*  RESULT
*     lListElem* - element or NULL 
******************************************************************************/
lListElem *lFindPrev(const lListElem *ep, const lCondition *cp) 
{
   DENTER(CULL_LAYER, "lFindNext");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return NULL;
   }

   do {
      ep = ep->prev;
   } while (ep && (lCompare(ep, cp) == 0));

   DEXIT;
   return (lListElem *) ep;
}

/****** cull/list/lPSortList() ************************************************
*  NAME
*     lPSortList() -- Sort a given list 
*
*  SYNOPSIS
*     int lPSortList(lList * lp, const char *fmt, ...) 
*
*  FUNCTION
*     Sort a given list. The sorting order is given by the format 
*     string and additional arguments. 
*
*  INPUTS
*     lList * lp      - list 
*     const char *fmt - format string (see lParseSortOrder()) 
*     ...             - additional arguments (see lParseSortOrder()) 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
*
*  SEE ALSO
*     cull/list/lParseSortOrder() 
******************************************************************************/
int lPSortList(lList * lp, const char *fmt,...)
{
   va_list ap;

   lSortOrder *sp;

   DENTER(CULL_LAYER, "lPSortList");

   va_start(ap, fmt);
   if (!lp || !fmt) {
      LERROR(LELISTNULL);
      va_end(ap);
      DEXIT;
      return -1;
   }
   if (!(sp = lParseSortOrder(lp->descr, fmt, ap))) {
      LERROR(LEPARSESORTORD);
      va_end(ap);
      DEXIT;
      return -1;
   }

   lSortList(lp, sp);

   va_end(ap);
   lFreeSortOrder(sp);

   DEXIT;
   return 0;
}

/****** cull/list/lSortList() *************************************************
*  NAME
*     lSortList() -- Sort list according to sort order object 
*
*  SYNOPSIS
*     int lSortList(lList *lp, const lSortOrder *sp) 
*
*  FUNCTION
*     Sort list according to sort order object. 
*
*  INPUTS
*     lList *lp            - list 
*     const lSortOrder *sp - sort order object 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lSortList(lList *lp, const lSortOrder *sp) 
{
   lListElem *ep;
   lListElem **pointer;
   int i, n;

   DENTER(CULL_LAYER, "lSortList");

   if (!lp) {
      DEXIT;
      return 0;                 /* ok list is sorted */
   }

   /* 
    * step 1: build up a pointer array for use of qsort
    */

   n = lGetNumberOfElem(lp);
   if (n < 2) {
      DEXIT;
      return 0;                 /* ok list is sorted */
   }

   if (!(pointer = (lListElem **) malloc(sizeof(lListElem *) * n))) {
      DEXIT;
      return -1;                /* low memory */
   }

#ifdef RANDOMIZE_QSORT_ELEMENTS

   for (i = 0, ep = lFirst(lp); ep; i++, ep = lNext(ep)) {
      int j = (int)((double)i*rand()/(RAND_MAX+1.0));
      pointer[i] = pointer[j];
      pointer[j] = ep;
   }

#else

   for (i = 0, ep = lFirst(lp); ep; i++, ep = lNext(ep))
      pointer[i] = ep;

#endif

   /* 
    * step 2: sort the pointer array using parsed sort order 
    */
   cull_state_set_global_sort_order(sp);
   /* this is done to pass the sort order */
   /* to the lSortCompare function called */
   /* by lSortCompareUsingGlobal          */
   qsort((void *) pointer, n, sizeof(lListElem *), lSortCompareUsingGlobal);

   /* 
    * step 3: relink elements in list according pointer array
    */
   lp->first = pointer[0];
   lp->last = pointer[n - 1];

   /* handle first element separatly */
   pointer[0]->prev = NULL;
   pointer[n - 1]->next = NULL;

   if (n > 1) {
      pointer[0]->next = pointer[1];
      pointer[n - 1]->prev = pointer[n - 2];
   }

   for (i = 1; i < n - 1; i++) {
      pointer[i]->prev = pointer[i - 1];
      pointer[i]->next = pointer[i + 1];
   }

   free(pointer);

   /* JG: TODO: is sorting changing the list? */

   DEXIT;
   return 0;
}

/****** cull/list/lUniqStr() **************************************************
*  NAME
*     lUniqStr() -- Uniq a string key list 
*
*  SYNOPSIS
*     int lUniqStr(lList *lp, int keyfield) 
*
*  FUNCTION
*     Uniq a string key list 
*
*  INPUTS
*     lList *lp    - list 
*     int keyfield - string field name id 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lUniqStr(lList *lp, int keyfield) 
{
   lListElem *ep;
   lListElem *rep;

   DENTER(CULL_LAYER, "lUniqStr");

   /*
    * sort the list first to make our algorithm work
    */
   if (lPSortList(lp, "%I+", keyfield)) {
      DEXIT;
      return -1;
   }

   /*
    * go over all elements and remove following elements
    */
   ep = lFirst(lp);
   while (ep) {
      rep = lNext(ep);
      while (rep &&
          !strcmp(lGetString(rep, keyfield), lGetString(ep, keyfield))) {
         lRemoveElem(lp, rep);
         rep = lNext(ep);
      }
      ep = lNext(ep);
   }

   DEXIT;
   return 0;
}

/****** cull/list/lUniqHost() *************************************************
*  NAME
*     lUniqHost() -- Uniq a host key list 
*
*  SYNOPSIS
*     int lUniqHost(lList *lp, int keyfield) 
*
*  FUNCTION
*     Uniq a hostname key list. 
*
*  INPUTS
*     lList *lp    - list 
*     int keyfield - host field 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int lUniqHost(lList *lp, int keyfield) 
{
   lListElem *ep;
   lListElem *rep;

   DENTER(CULL_LAYER, "lUniqHost");

   /*
    * sort the list first to make our algorithm work
    */
   if (lPSortList(lp, "%I+", keyfield)) {
      DEXIT;
      return -1;
   }

   /*
    * go over all elements and remove following elements
    */
   ep = lFirst(lp);
   while (ep) {
      rep = lNext(ep);
      while (rep &&
          !strcmp(lGetHost(rep, keyfield), lGetHost(ep, keyfield))) {
         lRemoveElem(lp, rep);
         rep = lNext(ep);
      }
      ep = lNext(ep);
   }

   DEXIT;
   return 0;
}

/****** cull/list/mt_get_type() ************************************************
*  NAME
*     mt_get_type() -- get data type for cull object field
*
*  SYNOPSIS
*     int mt_get_type(int mt) 
*
*  FUNCTION
*     Returns the data type of a cull object field given the multitype 
*     attribute of a cull descriptor.
*
*  INPUTS
*     int mt - mt (multitype) struct element of a field descriptor
*
*  RESULT
*     int - cull data type enum value (from _enum_lMultiType)
*
*  EXAMPLE
*     switch(mt_get_type(descr[i].mt)) {
*        case lFloatT:
*           ...
*     }
*
*  SEE ALSO
*     
*******************************************************************************/
int mt_get_type(int mt)
{
   return mt & 0x000000FF;
}

/****** cull/list/mt_do_hashing() ************************************************
*  NAME
*     mt_do_hashing() -- is there hash access for a field
*
*  SYNOPSIS
*     int mt_do_hashing(int mt) 
*
*  FUNCTION
*     Returns the information if hashing is active for a cull object field 
*     given the multitype attribute of a cull descriptor.
*
*  INPUTS
*     int mt - mt (multitype) struct element of a field descriptor
*
*  RESULT
*     int - 1, if hashing is requested, else 0
*
*  SEE ALSO
*     
*******************************************************************************/
int mt_do_hashing(int mt)
{
   return mt & CULL_HASH;
}

/****** cull/list/mt_is_unique() ************************************************
*  NAME
*     mt_is_unique() -- is the cull object field unique 
*
*  SYNOPSIS
*     int mt_is_unique(int mt) 
*
*  FUNCTION
*     Returns the information if a certain cull object field is unique within
*     a cull list given the multitype attribute of a cull descriptor.
*
*  INPUTS
*     int mt - mt (multitype) struct element of a field descriptor
*
*  RESULT
*     int - 1 = unique, 0 = not unique
*
*  EXAMPLE
*     if(mt_is_unique(descr[i].mt)) {
*        // check for uniqueness before inserting new elemente into a list
*        if(lGetElemUlong(....) != NULL) {
*           WARNING((SGE_EVENT, MSG_DUPLICATERECORD....));
*           return NULL;
*        }
*     }
*
*  SEE ALSO
*     
*******************************************************************************/
int mt_is_unique(int mt)
{
   return mt & CULL_UNIQUE;
}

bool 
lListElem_is_pos_changed(const lListElem *ep, int pos)
{
   return (sge_bitfield_get(ep->changed, pos) > 0); 
}

/****** cull_list/lList_clear_changed_info() ***********************************
*  NAME
*     lList_clear_changed_info() -- clear changed info of a list
*
*  SYNOPSIS
*     bool 
*     lList_clear_changed_info(lList *lp) 
*
*  FUNCTION
*     Clears in a list the information, if the list itself has been changed
*     (objects added/removed) and which fields have changed in the lists
*     objects.
*
*  INPUTS
*     lList *lp - the list to update
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     cull_list/lListElem_clear_changed_info()
*******************************************************************************/
bool
lList_clear_changed_info(lList *lp)
{
   bool ret = true;

   if (lp == NULL) {
      ret = false;
   } else {
      lListElem *ep;

      lp->changed = false;

      for_each(ep, lp) {
         lListElem_clear_changed_info(ep);
      }
   }

   return ret;
}

/****** cull_list/lListElem_clear_changed_info() *******************************
*  NAME
*     lListElem_clear_changed_info() -- clear changed info of an object
*
*  SYNOPSIS
*     bool 
*     lListElem_clear_changed_info(lListElem *ep) 
*
*  FUNCTION
*     clears in an object the information, which fields have been changed.
*     Recursively walks down sublists.
*
*  INPUTS
*     lListElem *ep - the list element to update
*
*  RESULT
*     bool - true on success, else false
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*     cull_list/lList_clear_changed_info()
*******************************************************************************/
bool 
lListElem_clear_changed_info(lListElem *ep)
{
   bool ret = true;

   if (ep == NULL) {
      ret = false;
   } else {
      int i;
      lDescr *descr = ep->descr;

      for (i = 0; ep->descr[i].nm != NoName; i++) {
         int type = mt_get_type(descr[i].mt);

         sge_bitfield_clear(ep->changed, i);

         if (type == lListT) {
            lList_clear_changed_info(ep->cont[i].glp);
         } else if (type == lObjectT) {
            lListElem_clear_changed_info(ep->cont[i].obj);
         }
      }
   }

   return ret;
}
