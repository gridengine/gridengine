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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "msg_cull.h"
#include "sge_log.h"
#include "sgermon.h"
#include "cull_multitypeP.h"
#include "cull_listP.h"
#include "cull_whatP.h"
#include "cull_lerrnoP.h"
#include "sge_string.h"

#define CULL_BASIS_LAYER CULL_LAYER

/* ---------- global variable --------------------------------- */

static const lNameSpace *lNameStr = NULL;

static char *multitypes[] =
{
   "lEndT",
   "lFloatT",
   "lDoubleT",
   "lUlongT",
   "lLongT",
   "lCharT",
   "lIntT",
   "lStringT",
   "lListT",
   "lRefT"
};

/* =========== implementation ================================= */

/* For test purposes only, replace it */
int incompatibleType(
const char *str 
) {
   int i;

   DENTER(TOP_LAYER, "incompatibleType");

   for (i = 0; i < 5; i++)
      DPRINTF(("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"));
   DPRINTF(("incompatible type in function %s()\n", str));
   for (i = 0; i < 5; i++)
      DPRINTF(("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"));

   abort();
   DEXIT;
   return -1;
}

int incompatibleType2(const char *fmt,...)
{
   va_list ap;
   char buf[BUFSIZ];

   DENTER(TOP_LAYER, "incompatibleType2");
   va_start(ap, fmt);
   vsprintf(buf, fmt, ap);

   CRITICAL((SGE_EVENT, buf));
   fprintf(stderr, buf);

   abort();
   DEXIT;
   return -1;
}

/* ------------------------------------------------------------ */
int unknownType(
const char *str 
) {
   DENTER(CULL_LAYER, "unknownType");

   /* abort is used, so we don't free any memory; if you change this
      function please free your memory                      */
   DPRINTF(("Unknown Type in %s.\n", str));
   LERROR(LEUNKTYPE);
   DEXIT;
   return -1;
   /* abort(); */
}

/* ------------------------------------------------------------ */

int lGetPosViaElem(
const lListElem *element,
int name 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosViaElem");

   if (!element) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return lGetPosInDescr(element->descr, name);
}

/* ------------------------------------------------------------ 

   returns the string representation of a name id

 */
char *lNm2Str(
int nm 
) {
   const lNameSpace *nsp;
   static char noinit[50];
   char *cp;

   DENTER(CULL_BASIS_LAYER, "lNm2Str");

   sprintf(noinit, "Nameindex = %d", nm);
   if (!lNameStr) {
      DPRINTF(("name vector uninitialized !!\n"));
      DEXIT;
      return noinit;
   }

   for (nsp = lNameStr; nsp->lower; nsp++) {
      if ((cp = _lNm2Str(nsp, nm))) {
         DEXIT;
         return cp;
      }
   }

   LERROR(LENAMENOT);
   DEXIT;
   return noinit;
}

char *_lNm2Str(
const lNameSpace *nsp,
int nm 
) {
   DENTER(CULL_BASIS_LAYER, "_lNm2Str");

   if (!nsp) {
      DPRINTF(("name vector uninitialized !!\n"));
      DEXIT;
      return NULL;
   }

   if (nm >= nsp->lower && nm < (nsp->lower + nsp->size)) {
      DEXIT;
      return nsp->namev[nm - nsp->lower];
   }

   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------ 

   returns the int representation of a name

 */

int lStr2Nm(
const char *str 
) {
   const lNameSpace *nsp;
   int ret;

   DENTER(CULL_BASIS_LAYER, "lStr2Nm");

   if (!lNameStr) {
      DPRINTF(("name vector uninitialized !!\n"));
      DEXIT;
      return NoName;
   }

   for (nsp = lNameStr; nsp->lower; nsp++) {
      if ((ret = _lStr2Nm(nsp, str)) != NoName) {
         DPRINTF(("Name: %s Id: %d\n", str, ret));
         DEXIT;
         return ret;
      }
   }

   LERROR(LENAMENOT);
   DEXIT;
   return NoName;
}

int _lStr2Nm(
const lNameSpace *nsp,
const char *str 
) {
   int i;
   int ret = NoName;
   int found = 0;

   DENTER(CULL_BASIS_LAYER, "_lStr2Nm");

   if (nsp) {
      for (i = 0; i < nsp->size; i++) {
         DPRINTF(("%d: %s\n", nsp->namev[i]));
         if (!strcmp(nsp->namev[i], str)) {
            found = 1;
            break;
         }
      }

      if (found)
         ret = nsp->lower + i;
      else
         ret = NoName;
   }

   DEXIT;
   return ret;
}

/* ------------------------------------------------------------ 

   initializes the mechanism for lNm2Str()
   a string vector and its size is needed

 */
void lInit(
const lNameSpace *namev 
) {
   DENTER(CULL_LAYER, "lInit");

   lNameStr = namev;

   DEXIT;
}

/* ------------------------------------------------------------

   returns the size of a descriptor
   excluding lEndT Descr

 */

int lCountDescr(
const lDescr *dp 
) {
   const lDescr *p;
   
   DENTER(CULL_BASIS_LAYER, "lCountDescr");

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return -1;
   }

   p = &dp[0];
   while (p->mt != lEndT)
      p++;

   DEXIT;
   return (p - &dp[0]);
}

/* ------------------------------------------------------------

   returns a pointer to a copied descriptor, has to be freed by the
   user

   returns NULL in cas of error, a pointer otherwise

 */

lDescr *lCopyDescr(
const lDescr *dp 
) {
   int i;
   lDescr *new = NULL;

   DENTER(CULL_BASIS_LAYER, "lCopyDescr");

   if (!dp) {
      LERROR(LEDESCRNULL);
      goto error;
   }

   if ((i = lCountDescr(dp)) == -1) {
      LERROR(LEDESCRNULL);
      goto error;
   }

   if (!(new = (lDescr *) malloc(sizeof(lDescr) * (i + 1)))) {
      LERROR(LEMALLOC);
      goto error;
   }
   memcpy(new, dp, sizeof(lDescr) * (i + 1));

   DEXIT;
   return new;

 error:
   DPRINTF(("lCopyDescr failed\n"));
   DEXIT;
   return NULL;
}

/* ------------------------------------------------------------

   writes a descriptor (for debugging purposes)

 */

void lWriteDescrTo(
const lDescr *dp,
FILE *fp 
) {
   int i;

   DENTER(CULL_LAYER, "lWriteDescr");

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return;
   }

   for (i = 0; dp[i].mt != lEndT; i++) {
      if (!fp)
         DPRINTF(("nm: %d(%-20.20s) mt: %d\n", dp[i].nm, lNm2Str(dp[i].nm), dp[i].mt));
      else
         fprintf(fp, "nm: "U32CFormat"(%-20.20s) mt: "U32CFormat"\n", u32c( (dp[i].nm) ), lNm2Str(dp[i].nm), u32c(dp[i].mt));
   }

   DEXIT;
}
/* ------------------------------------------------------------ 

   returns position of a name in a descriptor array 
   or -1 if not found

 */
int lGetPosInDescr(
const lDescr *dp,
int name 
) {
   const lDescr *ldp;

   if (!dp) {
      LERROR(LEDESCRNULL);
      return -1;
   }

   for (ldp = dp; ldp->nm != name && ldp->nm != NoName; ldp++);

   if (ldp->nm == NoName) {
      LERROR(LENAMENOT);
      return -1;
   }

   return ldp - dp;
}

lList **lGetListRef(
const lListElem *ep,
int name 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lGetListRef");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_CULL_POINTER_GETLISTREF_NULLELEMENTFORX_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }
   pos = lGetPosViaElem(ep, name);

   if (pos < 0) {
      /* someone has called lGetPosUlong() */
      /* makro with an invalid nm        */
      CRITICAL((SGE_EVENT, MSG_CULL_GETLISTREF_XNOTFOUNDINELEMENT_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }

   if (ep->descr[pos].mt != lListT)
      incompatibleType("lGetPosListRef");

   DEXIT;
   return &(ep->cont[pos].glp);
}

char **lGetPosStringRef(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosStringRef");

   if (ep->descr[pos].mt != lStringT)
      incompatibleType("lGetPosStringRef");

   DEXIT;
   return &(ep->cont[pos].str);
}

/* 
   FOR THE lGet{Type} FUNCTIONS THERE IS NO REAL ERRORHANDLING
   IF EP IS NULL THERE WILL BE A COREDUMP
   THIS IS NECESSARY, OTHERWISE IT WOULD BE DIFFICULT TO CASCADE
   THE GET FUNCTIONS
   SO HIGHER LEVEL FUNCTIONS OR THE USER SHOULD CHECK IF THE
   ARGUMENTS ARE ALLRIGHT.
 */

/* ------------------------------------------------------------ 

   returns the int value at position pos 
   (runtime type checking)

 */
lInt lGetPosInt(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosInt");

   if (ep->descr[pos].mt != lIntT)
      incompatibleType("lGetPosInt");

   DEXIT;
   return (lInt) ep->cont[pos].i;
}

/* ------------------------------------------------------------ 

   returns the int value for field name 
   (runtime type checking)

 */
lInt lGetInt(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetInt");

   pos = lGetPosViaElem(ep, name);
   if (ep->descr[pos].mt != lIntT)
      incompatibleType2(MSG_CULL_GETINT_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);

   DEXIT;
   return (lInt) ep->cont[pos].i;
}

/* ------------------------------------------------------------ 

   returns the ulong value at position pos 
   (runtime type checking)

 */
lUlong lGetPosUlong(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosUlong");

   if (pos < 0) {
      /* someone has called lGetPosUlong() */
      /* makro with an invalid nm        */
      CRITICAL((SGE_EVENT, MSG_CULL_GETPOSULONG_GOTINVALIDPOSITION ));
      DEXIT;
      abort();
   }

   if (ep->descr[pos].mt != lUlongT)
      incompatibleType("lGetPosUlong");
   DEXIT;
   return (lUlong) ep->cont[pos].ul;
}

/* ------------------------------------------------------------ 

   returns the ulong value for field name 
   (runtime type checking)

 */
lUlong lGetUlong(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetUlong");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_CULL_POINTER_GETULONG_NULLELEMENTFORX_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      /* someone has called lGetPosUlong() */
      /* makro with an invalid nm        */
      incompatibleType2(MSG_CULL_GETULONG_NOSUCHNAMEXYINDESCRIPTOR_IS , name, lNm2Str(name));
   }

   if (ep->descr[pos].mt != lUlongT)
      incompatibleType2(MSG_CULL_GETULONG_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);

   DEXIT;
   return (lUlong) ep->cont[pos].ul;
}

/* ------------------------------------------------------------ 

   returns the char * value at position pos 
   but doesn't copy the string 
   (runtime type checking)

 */
char *lGetPosString(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosString");

   if (pos < 0) {
      /* someone has called lGetString() */
      /* makro with an invalid nm        */
      DPRINTF(("!!!!!!!!!!!!!!!! lGetPosString() got an invalid pos !!!!!!!!!\n"));
      DEXIT;
      return NULL;
   }

   if (ep->descr[pos].mt != lStringT)
      incompatibleType("lGetPosString");

   DEXIT;

   return (lString) ep->cont[pos].str;
}

int lGetType(
const lDescr *dp,
int nm 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lGetType");

   pos = lGetPosInDescr(dp, nm);
   if (pos < 0) {
      DEXIT;
      return lEndT;
   }

   DEXIT;
   return dp[pos].mt;
}

/* ------------------------------------------------------------ 

   returns the char * value for field name
   but doesn't copy the string 
   (runtime type checking)

 */
char *lGetString(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetString");

   if (!ep) {
      CRITICAL((SGE_EVENT, MSG_CULL_POINTER_GETSTRING_NULLELEMENTFORX_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      /* someone has called lGetString() */
      /* makro with an invalid nm        */
      incompatibleType2(MSG_CULL_GETSTRING_NOSUCHNAMEXYINDESCRIPTOR_IS ,
                        name, lNm2Str(name));
      DEXIT;
      return NULL;
   }

   if (ep->descr[pos].mt != lStringT)
      incompatibleType2(MSG_CULL_GETSTRING_WRONGTYPEFORFILEDXY_SS ,
                        lNm2Str(name), multitypes[ep->descr[pos].mt]);

   DEXIT;

   return (lString) ep->cont[pos].str;
}

/* ------------------------------------------------------------ 

   returns the List value at position pos 
   but doesn't copy the list
   (runtime type checking)

 */
lList *lGetPosList(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosList");

   if (pos < 0) {
      /* someone has called lGetPosUlong() */
      /* makro with an invalid nm        */
      CRITICAL((SGE_EVENT, MSG_CULL_GETPOSLIST_GOTANINVALIDPOS ));
      DEXIT;
      abort();
   }

   if (ep->descr[pos].mt != lListT)
      incompatibleType("lGetPosList");

   DEXIT;

   return (lList *) ep->cont[pos].glp;
}

/* ------------------------------------------------------------ 

   returns the List value for field name
   but doesn't copy the list
   (runtime type checking)

 */
lList *lGetList(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetList");

   if (!ep) {
      CRITICAL((SGE_EVENT,  MSG_CULL_POINTER_GETLIST_NULLELEMENTFORX_S ,
               lNm2Str(name)));
      DEXIT;   /* CHANGE BACK */
      abort();
   }
   pos = lGetPosViaElem(ep, name);

   if (pos < 0) {
      /* someone has called lGetPosUlong() */
      /* makro with an invalid nm        */
      CRITICAL((SGE_EVENT, MSG_CULL_GETLIST_XNOTFOUNDINELEMENT_S ,
               lNm2Str(name)));
      DEXIT;
      abort();
   }

   if (ep->descr[pos].mt != lListT)
      incompatibleType2(MSG_CULL_GETLIST_WRONGTYPEFORFIELDXY_SS ,
                        lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return (lList *) ep->cont[pos].glp;
}

/* ------------------------------------------------------------ 

   returns the float value at position pos 
   (runtime type checking)

 */
lFloat lGetPosFloat(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosFloat");
   if (ep->descr[pos].mt != lFloatT)
      incompatibleType("lGetPosFloat");
   DEXIT;
   return ep->cont[pos].fl;
}

/* ------------------------------------------------------------ 

   returns the float value for field name
   (runtime type checking)

 */
lFloat lGetFloat(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetFloat");

   pos = lGetPosViaElem(ep, name);

   if (ep->descr[pos].mt != lFloatT)
      incompatibleType2(MSG_CULL_GETFLOAT_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return ep->cont[pos].fl;
}

/* ------------------------------------------------------------ 

   returns the double value at position pos 
   (runtime type checking)

 */
lDouble lGetPosDouble(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosDouble");
   if (ep->descr[pos].mt != lDoubleT)
      incompatibleType("lGetPosDouble");
   DEXIT;
   return ep->cont[pos].db;
}
/* ------------------------------------------------------------ 

   returns the double value for field name
   (runtime type checking)

 */
lDouble lGetDouble(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetDouble");

   pos = lGetPosViaElem(ep, name);

   if (ep->descr[pos].mt != lDoubleT)
      incompatibleType2(MSG_CULL_GETDOUBLE_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return ep->cont[pos].db;
}
/* ------------------------------------------------------------ 

   returns the long value at position pos 
   (runtime type checking)

 */
lLong lGetPosLong(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosLong");
   if (ep->descr[pos].mt != lLongT)
      incompatibleType("lGetPosLong");
   DEXIT;
   return ep->cont[pos].l;
}
/* ------------------------------------------------------------ 

   returns the long value for field name
   (runtime type checking)

 */
lLong lGetLong(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetLong");
   pos = lGetPosViaElem(ep, name);

   if (ep->descr[pos].mt != lLongT)
      incompatibleType2(MSG_CULL_GETLONG_WRONGTYPEFORFIELDXY_SS, lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return ep->cont[pos].l;
}
/* ------------------------------------------------------------ 

   returns the char value at position pos 
   (runtime type checking)

 */
lChar lGetPosChar(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosChar");
   if (ep->descr[pos].mt != lCharT)
      incompatibleType("lGetPosChar");
   DEXIT;
   return ep->cont[pos].c;
}

/* ------------------------------------------------------------ 

   returns the char value for field name
   (runtime type checking)

 */
lChar lGetChar(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetChar");
   pos = lGetPosViaElem(ep, name);

   if (ep->descr[pos].mt != lCharT)
      incompatibleType2(MSG_CULL_GETCHAR_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return ep->cont[pos].c;
}
/* ------------------------------------------------------------ 

   returns the lRef value at position pos 
   (runtime type checking)

 */
lRef lGetPosRef(
const lListElem *ep,
int pos 
) {
   DENTER(CULL_BASIS_LAYER, "lGetPosRef");
   if (ep->descr[pos].mt != lRefT)
      incompatibleType("lGetPosRef");
   DEXIT;
   return ep->cont[pos].ref;
}

/* ------------------------------------------------------------ 

   returns the char value for field name
   (runtime type checking)

 */
lRef lGetRef(
const lListElem *ep,
int name 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lGetRef");
   pos = lGetPosViaElem(ep, name);

   if (ep->descr[pos].mt != lRefT)
      incompatibleType2(MSG_CULL_GETREF_WRONGTYPEFORFIELDXY_SS, lNm2Str(name), multitypes[ep->descr[pos].mt]);
   DEXIT;
   return ep->cont[pos].ref;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the int value   
   (runtime type checking)

 */

int lSetPosInt(
const lListElem *ep,
int pos,
int value 
) {
   DENTER(CULL_BASIS_LAYER, "lSetPosInt");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lIntT) {
      incompatibleType("lSetPosInt");
      DEXIT;
      return -1;
   }
   ep->cont[pos].i = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the int value   
   (runtime type checking)

 */

int lSetInt(
lListElem *ep,
int name,
int value 
) {
   int pos;
   DENTER(CULL_BASIS_LAYER, "lSetInt");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lIntT) {
      incompatibleType2(MSG_CULL_SETINT_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }
   ep->cont[pos].i = value;

   DEXIT;
   return 0;
}
/* ------------------------------------------------------------

   sets in the element ep at position pos the ulong value   
   (runtime type checking)

 */
int lSetPosUlong(
const lListElem *ep,
int pos,
lUlong value 
) {
   DENTER(CULL_BASIS_LAYER, "lSetPosUlong");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lUlongT) {
      incompatibleType("lSetPosUlong");
      DEXIT;
      return -1;
   }

   ep->cont[pos].ul = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------

   sets in the element ep for the field name the ulong value   
   (runtime type checking)

 */
int lSetUlong(
lListElem *ep,
int name,
lUlong value 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetUlong");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      DPRINTF(("!!!!!!!!!! lSetUlong(): %s not found in element !!!!!!!!!!\n",
               lNm2Str(name)));
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lUlongT) {
      incompatibleType2(MSG_CULL_SETULONG_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }

   ep->cont[pos].ul = value;

   DEXIT;
   return 0;
}
/* ------------------------------------------------------------

   sets in the element ep at position pos the char * value   
   also duplicates the pointed char array
   (runtime type checking)

 */
int lSetPosString(
const lListElem *ep,
int pos,
const char *value 
) {
   char *str;

   DENTER(CULL_BASIS_LAYER, "lSetPosString");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lStringT) {
      incompatibleType("lSetPosString");
      DEXIT;
      return -1;
   }

   /* free old string value */
   if (ep->cont[pos].str)
      free(ep->cont[pos].str);

   /* strdup new string value */
   if (value) {
      if (!(str = strdup(value))) {
         LERROR(LESTRDUP);
         DEXIT;
         return -1;
      }
   }                            /* these brackets are required */
   else
      str = NULL;               /* value is NULL */

   ep->cont[pos].str = str;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------

   sets in the element ep for field name the char * value   
   also duplicates the pointed to char array
   (runtime type checking)

 */
int lSetString(
lListElem *ep,
int name,
const char *value 
) {
   char *str;
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetString");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      incompatibleType2(MSG_CULL_SETSTRING_NOSUCHNAMEXYINDESCRIPTOR_IS ,
                        name, lNm2Str(name));
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lStringT) {
      incompatibleType2(MSG_CULL_SETSTRING_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }

   /* free old string value */
   if (ep->cont[pos].str)
      free(ep->cont[pos].str);

   /* strdup new string value */
   if (value) {
      if (!(str = strdup(value))) {
         LERROR(LESTRDUP);
         DEXIT;
         return -1;
      }
   }                            /* these brackets are required */
   else
      str = NULL;               /* value is NULL */

   ep->cont[pos].str = str;

   DEXIT;
   return 0;
}
/* ------------------------------------------------------------ 

   sets in the element ep at position pos the lGenlist value   
   doesn't copy the list
   (runtime type checking)

 */
int lSetPosList(
const lListElem *ep,
int pos,
lList *value 
) {
   DENTER(CULL_BASIS_LAYER, "lSetPosList");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lListT) {
      incompatibleType("lSetPosList");
      DEXIT;
      return -1;
   }

   /* free old list */
   if (ep->cont[pos].glp) {
      lFreeList(ep->cont[pos].glp);
   }
   /* set new list */
   ep->cont[pos].glp = value;

   DEXIT;
   return 0;
}

int lXchgList(
lListElem *ep,
int name,
lList **lpp 
) {
   int pos;
   lList *tmp;

   DENTER(CULL_BASIS_LAYER, "lXchgList");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lListT) {
      incompatibleType2(MSG_CULL_XCHGLIST_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }

   tmp = ep->cont[pos].glp;
   ep->cont[pos].glp = *lpp;
   *lpp = tmp;

   DEXIT;
   return 0;

}

/*-------------------------------------------------------------------------*/
int lSwapList(
lListElem *to,
int nm_to,
lListElem *from,
int nm_from 
) {
   lList *tmp = NULL;

   DENTER(CULL_BASIS_LAYER, "lSwapList");

   if (lXchgList(from, nm_from, &tmp) == -1) {
      DEXIT;
      return -1;
   }
   if (lXchgList(to, nm_to, &tmp) == -1) {
      DEXIT;
      return -1;
   }
   if (lXchgList(from, nm_from, &tmp) == -1) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the lList value   
   doesn't copy the list
   (runtime type checking)

 */
int lSetList(
lListElem *ep,
int name,
lList *value 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetList");

   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }
   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      DPRINTF(("!!!!!!!!!! lSetList(): %s not found in element !!!!!!!!!!\n",
               lNm2Str(name)));
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lListT) {
      incompatibleType2(MSG_CULL_SETLIST_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }

   /* free old list */
   if (ep->cont[pos].glp) {
      lFreeList(ep->cont[pos].glp);
   }
   /* set new list */
   ep->cont[pos].glp = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the float value   
   (runtime type checking)

 */
int lSetPosFloat(const lListElem * ep, int pos, lFloat value)
{
   DENTER(CULL_BASIS_LAYER, "lSetPosFloat");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lFloatT) {
      incompatibleType("lSetPosFloat");
      DEXIT;
      return -1;
   }

   ep->cont[pos].fl = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the float value   
   (runtime type checking)

 */
int lSetFloat(lListElem * ep, int name, lFloat value)
{
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetPosFloat");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lFloatT) {
      incompatibleType2(MSG_CULL_SETFLOAT_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }

   ep->cont[pos].fl = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the double value   
   (runtime type checking)

 */
int lSetPosDouble(
const lListElem *ep,
int pos,
lDouble value 
) {
   DENTER(CULL_BASIS_LAYER, "lSetPosDouble");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lDoubleT) {
      incompatibleType("lSetPosDouble");
      DEXIT;
      return -1;
   }
   ep->cont[pos].db = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the double value   
   (runtime type checking)

 */
int lSetDouble(
lListElem *ep,
int name,
lDouble value 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetPosDouble");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lDoubleT) {
      incompatibleType2(MSG_CULL_SETDOUBLE_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }
   ep->cont[pos].db = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the long value   
   (runtime type checking)

 */
int lSetPosLong(
const lListElem *ep,
int pos,
lLong value 
) {
   DENTER(CULL_BASIS_LAYER, "lSetPosLong");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lLongT) {
      incompatibleType("lSetPosLong");
      DEXIT;
      return -1;
   }
   ep->cont[pos].l = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the long value   
   (runtime type checking)

 */
int lSetLong(
lListElem *ep,
int name,
lLong value 
) {
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetPosLong");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lLongT) {
      incompatibleType2(MSG_CULL_SETLONG_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }
   ep->cont[pos].l = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the char value   
   (runtime type checking)

 */
int lSetPosChar(const lListElem *ep, int pos, lChar value)
{
   DENTER(CULL_BASIS_LAYER, "lSetPosChar");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lCharT) {
      incompatibleType("lSetPosChar");
      DEXIT;
      return -1;
   }
   ep->cont[pos].c = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the char value   
   (runtime type checking)

 */
int lSetChar(lListElem * ep, int name, lChar value)
{
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetPosChar");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lCharT) {
      incompatibleType2(MSG_CULL_SETCHAR_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }
   ep->cont[pos].c = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep at position pos the lRef value   
   (runtime type checking)

 */
int lSetPosRef(const lListElem * ep, int pos, lRef value)
{
   DENTER(CULL_BASIS_LAYER, "lSetPosRef");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lRefT) {
      incompatibleType("lSetPosRef");
      DEXIT;
      return -1;
   }
   ep->cont[pos].ref = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   sets in the element ep for field name the lRef value   
   (runtime type checking)

 */
int lSetRef(lListElem * ep, int name, lRef value)
{
   int pos;

   DENTER(CULL_BASIS_LAYER, "lSetPosRef");
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   pos = lGetPosViaElem(ep, name);
   if (pos < 0) {
      LERROR(LENEGPOS);
      DEXIT;
      return -1;
   }

   if (ep->descr[pos].mt != lRefT) {
      incompatibleType2(MSG_CULL_SETREF_WRONGTYPEFORFIELDXY_SS , lNm2Str(name), multitypes[ep->descr[pos].mt]);
      DEXIT;
      return -1;
   }
   ep->cont[pos].ref = value;

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------ 

   compares two int values i0 and i1 
   return values like strcmp

 */
int intcmp(
int i0,
int i1 
) {
   return i0 == i1 ? 0 : (i0 < i1 ? -1 : 1);
}

/* ------------------------------------------------------------ 

   compares two ulong values u0 and u1 
   return values like strcmp

 */
int ulongcmp(
lUlong u0,
lUlong u1 
) {
   return u0 == u1 ? 0 : (u0 < u1 ? -1 : 1);
}

int bitmaskcmp(
lUlong bm0,
lUlong bm1 
) {
   return ((bm0 & bm1) == bm1) ? 1 : 0;
}

/* ------------------------------------------------------------ 

   compares two lFloat values f0 and f1 
   return values like strcmp

 */
int floatcmp(lFloat f0, lFloat f1)
{
   return f0 == f1 ? 0 : (f0 < f1 ? -1 : 1);
}

/* ------------------------------------------------------------ 

   compares two double values d0 and d1 
   return values like strcmp

 */
int doublecmp(
lDouble d0,
lDouble d1 
) {
   return d0 == d1 ? 0 : (d0 < d1 ? -1 : 1);
}

/* ------------------------------------------------------------ 

   compares two long values l0 and l1 
   return values like strcmp

 */
int longcmp(
lLong l0,
lLong l1 
) {
   return l0 == l1 ? 0 : (l0 < l1 ? -1 : 1);
}

/* ------------------------------------------------------------ 

   compares two char values c0 and c1 
   return values like strcmp

 */
int charcmp(lChar c0, lChar c1)
{
   return c0 == c1 ? 0 : (c0 < c1 ? -1 : 1);
}

/* ------------------------------------------------------------ 

   compares two lRef values c0 and c1 
   return values like strcmp

 */
int refcmp(lRef c0, lRef c1)
{
   return c0 == c1 ? 0 : (c0 < c1 ? -1 : 1);
}

/****** cull_multitype/lAddSubStr() *******************************************
*  NAME
*     lAddSubStr() -- adds a string to the string sublist of element ep 
*
*  SYNOPSIS
*     lListElem* lAddSubStr(lListElem* ep, int nm, char* str, int snm, lDescr* 
*     dp) 
*
*  FUNCTION
*     This function add a new element into a sublist snm of an element ep.
*     The field nm of this added element will get the initial value specified
*     with str. 
*
*  INPUTS
*     lListElem* ep - list element 
*     int nm        - field id contained in the element which will be created
*     char* str     - initial value if nm 
*     int snm       - field id of the sublist within ep 
*     lDescr* dp    - Type of the new element 
*
*  RESULT
*     NULL in case of error
*     otherwise pointer to the added element 
*******************************************************************************
*/
lListElem *lAddSubStr(
lListElem *ep,
int nm,
const char *str,
int snm,
const lDescr *dp 
) {
   lListElem *ret;
   int sublist_pos;

   DENTER(CULL_LAYER, "lAddSubStr");

   if (!ep) {
      DPRINTF(("error: NULL ptr passed to lAddSubStr\n"));
      DEXIT;
      return NULL;
   }

   if (!(ep->descr)) {
      DPRINTF(("NULL descriptor in element not allowed !!!"));
      DEXIT;
      abort();
   }

   /* run time type checking */
   if ((sublist_pos = lGetPosViaElem(ep, snm)) < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_ADDSUBSTRERRORXRUNTIMETYPE_S , lNm2Str(snm)));
      DEXIT;
      return NULL;
   }

   ret = lAddElemStr(&(ep->cont[sublist_pos].glp), nm, str, dp);

   DEXIT;
   return ret;
}

/****** cull_multitype/lAddElemStr() ******************************************
*  NAME
*     lAddElemStr() -- adds a string to the string list  
*
*  SYNOPSIS
*     lListElem* lAddElemStr(lList **lpp, int nm, const char *str, 
*                            const lDescr *dp) 
*
*  FUNCTION
*     This function adds a new element of type dp to the list referenced
*     by lpp. The field nm will get the initial value str.
*
*  INPUTS
*     lList** lpp - list reference 
*     int nm      - field id 
*     char* str   - initial value 
*     lDescr* dp  - Type of the object which will be added (e.g. JB_Type) 
*
*  RESULT
*     lListElem* - 
******************************************************************************
*/
lListElem *lAddElemStr(
lList **lpp,
int nm,
const char *str,
const lDescr *dp 
) {
   lListElem *sep;
   int str_pos;

   DENTER(CULL_LAYER, "lAddElemStr");

   if (!lpp || !str || !dp) {
      DPRINTF(("error: NULL ptr passed to lAddElemStr\n"));
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   str_pos = lGetPosInDescr(dp, nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_ADDELEMSTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   if (!*lpp) {
      /* ensure existence of a str list in ep */
      *lpp = lCreateList("string sublist", dp);
   }

   /* add new host str element to sublist */
   sep = lCreateElem(dp);
   lSetPosString(sep, str_pos, (lString) str);
   lAppendElem(*lpp, sep);

   DEXIT;
   return sep;
}

/****** cull_multitype/lDelSubStr() *******************************************
*  NAME
*     lDelSubStr() -- removes an element from a sublist 
*
*  SYNOPSIS
*     int lDelSubStr(lListElem* ep, int nm, const char* str, int snm) 
*
*  FUNCTION
*     This function removes an element specified by a string field nm and
*     the string str supposed to be in the sublist snm of the element ep.
*
*  INPUTS
*     lListElem* ep - element 
*     int nm        - field id 
*     const char* str     - string 
*     int snm       - field id of a sublist of ep
*
*  RESULT
*     1 element was found and removed
*     0 in case of an error 
*******************************************************************************
*/
int lDelSubStr(
lListElem *ep,
int nm,
const char *str,
int snm 
) {
   int ret, sublist_pos;

   DENTER(CULL_LAYER, "lDelSubStr");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELSUBSTRERRORXRUNTIMETYPEERROR_S , lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lDelElemStr(&(ep->cont[sublist_pos].glp), nm, str);

   DEXIT;
   return ret;
}

/****** cull_multitype/lDelElemStr() ******************************************
*  NAME
*     lDelElemStr() -- removes an element specified by a string field nm 
*
*  SYNOPSIS
*     int lDelElemStr(lList** lpp, int nm, const char* str) 
*
*  FUNCTION
*     This function removes an element from the list referenced by lpp,
*     which is identified by the field nm and the string str 
*
*  INPUTS
*     lList** lpp - list reference 
*     int nm      - field id
*     const char* str   - string
*
*  RESULT
*     1 if the element was found and removed
*     0 in case of an error 
*******************************************************************************
*/
int lDelElemStr(
lList **lpp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;

   DENTER(CULL_LAYER, "lDelElemStr");

   if (!lpp || !str) {
      DPRINTF(("error: NULL ptr passed to lDelElemStr\n"));
      DEXIT;
      return 0;
   }

   /* empty list ? */
   if (!*lpp) {
      DEXIT;
      return 1; 
   }

   /* get position of nm in dp */
   str_pos = lGetPosInDescr(lGetListDescr(*lpp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELELEMSTRERRORXRUNTIMETYPEERROR_S , lNm2Str(nm)));
      DEXIT;
      abort();
   }

   /* seek element */
   ep = lGetElemStr(*lpp, nm, str);
   if (ep) {
      lFreeElem(lDechainElem(*lpp, ep));
      if (lGetNumberOfElem(*lpp) == 0) {
         lFreeList(*lpp);
         *lpp = NULL;
      }
   }

   DEXIT;
   return 1;
}

/****** cull_multitype/lGetSubStr() *******************************************
*  NAME
*     lGetSubStr() -- returns an element specified by a string field nm 
*
*  SYNOPSIS
*     lListElem* lGetSubStr(const lListElem* ep, int nm, const char* str, 
*                           int snm) 
*
*  FUNCTION
*     returns an element specified by a string field nm and the string str
*     from the sublist snm of the element ep 
*
*  INPUTS
*     const lListElem* ep - element pointer 
*     int nm              - field id contained in an sublist element of ep 
*     const char* str     - string 
*     int snm             - field id contained in ep 
*
*  RESULT
*     NULL if element was not found or in case of an error 
*     otherwise pointer to an element
*******************************************************************************
*/
lListElem *lGetSubStr(
const lListElem *ep,
int nm,
const char *str,
int snm 
) {
   int sublist_pos;
   lListElem *ret;

   DENTER(CULL_LAYER, "lGetSubStr");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETSUBSTRERRORXRUNTIMETYPE_S , lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lGetElemStr(ep->cont[sublist_pos].glp, nm, str);

   DEXIT;
   return ret;
}

/****** cull_multitype/lGetElemStr() *******************************************
*  NAME
*     lGetElemStr() -- returns an element specified by a string field nm 
*
*  SYNOPSIS
*     lListElem* lGetElemStr(const lList* lp, int nm, const char* str) 
*
*  FUNCTION
*     returns an element specified by a string field nm from list lp 
*
*  INPUTS
*     const lList* lp - list 
*     int nm    - field id 
*     const char* str - value 
*
*  RESULT
*     NULL when element was not found or if an error occured
*     otherwise pointer to element 
*******************************************************************************
*/
lListElem *lGetElemStr(
const lList *lp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;
   char *s;

   DENTER(CULL_LAYER, "lGetElemStr");
   if (!str) {
      DPRINTF(("error: NULL ptr passed to lGetElemStr\n"));
      DEXIT;
      return NULL;
   }

   /* empty list ? */
   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   str_pos = lGetPosInDescr(lGetListDescr(lp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMSTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   /* seek for element */
   for_each(ep, lp) {
      s = lGetPosString(ep, str_pos);
      if (s && !strcmp(s, str)) {
         DEXIT;
         return ep;
      }
   }

   DEXIT;
   return NULL;
}

/****** cull_multitype/lGetElemStrLike() **************************************
*  NAME
*     lGetElemStrLike() -- returns an element specified by a wildcard string 
*
*  SYNOPSIS
*     lListElem* lGetElemStrLike(const lList* lp, int nm, const char* str) 
*
*  FUNCTION
*     returns an element specified by a string field nm from the list lp and
*     uses a trailing '*' as a wilcard, e.g. 'OAport' matches 'OA*' 
*
*  INPUTS
*     const lList* lp - list pointer 
*     int nm    - field id 
*     const char* str - wildcard string 
*
*  RESULT
*     NULL if element was not found or in case of error
*     otherwise pointer to element 
*******************************************************************************
*/
lListElem *lGetElemStrLike(
const lList *lp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;
   char *s;

   DENTER(CULL_LAYER, "lGetElemStr");
   if (!str) {
      DPRINTF(("error: NULL ptr passed to lGetElemStr\n"));
      DEXIT;
      return NULL;
   }

   /* empty list ? */
   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   str_pos = lGetPosInDescr(lGetListDescr(lp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMSTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   /* seek for element */
   for_each(ep, lp) {
      s = lGetPosString(ep, str_pos);
      if (s && (!strcmp(s, str) ||
            (str[strlen(str)-1] == '*' && !strncmp(s, str, strlen(str)-1)))) {
         DEXIT;
         return ep;
      }
   }

   DEXIT;
   return NULL;
}

/****** cull_multitype/lAddSubUlong() *****************************************
*  NAME
*     lAddSubUlong() -- adds a ulong to the ulong sublist of element ep 
*
*  SYNOPSIS
*     lListElem* lAddSubUlong(lListElem* ep, int nm, lUlong val, int snm, 
*                             const lDescr* dp) 
*
*  FUNCTION
*     This function adds a new element into the sublist snm of the element ep.
*     The field nm of the added element will get the initial value val. 
*
*  INPUTS
*     lListElem* ep       - element 
*     int nm              - field which will get value val 
*     lUlong val          - initial value for nm 
*     int snm             - sublist within ep where the element will be added 
*     const lDescr* dp    - Type of the new element (e.g. JB_Type) 
*
*  RESULT
*     NULL in case of error
*     or the pointer to the new element 
*******************************************************************************
*/
lListElem *lAddSubUlong(
lListElem *ep,
int nm,
lUlong val,
int snm,
const lDescr *dp 
) {
   lListElem *ret;
   int sublist_pos;

   DENTER(CULL_LAYER, "lAddSubUlong");

   if (!ep) {
      DPRINTF(("error: NULL ptr passed to lAddSubUlong\n"));
      DEXIT;
      return NULL;
   }

   if (!(ep->descr)) {
      DPRINTF(("NULL descriptor in element not allowed !!!"));
      DEXIT;
      abort();
   }

   /* run time type checking */
   if ((sublist_pos = lGetPosViaElem(ep, snm)) < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_ADDSUBULONGERRORXRUNTIMETYPE_S , lNm2Str(snm)));
      DEXIT;
      return NULL;
   }

   ret = lAddElemUlong(&(ep->cont[sublist_pos].glp), nm, val, dp);

   DEXIT;
   return ret;
}

/****** cull_multitype/lAddElemUlong() ****************************************
*  NAME
*     lAddElemUlong() -- adds a ulong to the ulong list 
*
*  SYNOPSIS
*     lListElem* lAddElemUlong(lList** lpp, int nm, lUlong val, 
*                              const lDescr* dp) 
*
*  FUNCTION
*     Adds an new element to a list lpp where one field nm within
*     the new element gets an initial value val 
*
*  INPUTS
*     lList** lpp       - list  
*     int nm            - field in the new element which will get value val 
*     lUlong val        - initial value for nm 
*     const lDescr* dp  - type of the list (e.g. JB_Type) 
*
*  RESULT
*     NULL on error
*     or pointer to the added element 
*******************************************************************************
*/
lListElem *lAddElemUlong(
lList **lpp,
int nm,
lUlong val,
const lDescr *dp 
) {
   lListElem *sep;
   int val_pos;

   DENTER(CULL_LAYER, "lAddElemUlong");

   if (!lpp || !dp) {
      DPRINTF(("error: NULL ptr passed to lAddElemUlong\n"));
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   val_pos = lGetPosInDescr(dp, nm);

   /* run time type checking */
   if (val_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_ADDELEMULONGERRORXRUNTIMETYPE_S, 
         lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   if (!*lpp) {
      /* ensure existence of a val list in ep */
      *lpp = lCreateList("ulong sublist", dp);
   }

   /* add new host val element to sublist */
   sep = lCreateElem(dp);
   lSetPosUlong(sep, val_pos, val);
   lAppendElem(*lpp, sep);

   DEXIT;
   return sep;
}

/****** cull_multitype/lDelSubUlong() *****************************************
*  NAME
*     lDelSubUlong() -- removes an element from a sublist 
*
*  SYNOPSIS
*     int lDelSubUlong(lListElem* ep, int nm, lUlong val, int snm) 
*
*  FUNCTION
*     This function removes an element specified by a ulong field nm
*     and the ulong val supposed to be in the sublist snm of the element ep 
*
*  INPUTS
*     lListElem* ep - element 
*     int nm        - field id 
*     lUlong val    - value 
*     int snm       - field id of the sublist in ep 
*
*  RESULT
*     1 element was found and removed
*     0 in case of an error 
*******************************************************************************
*/
int lDelSubUlong(
lListElem *ep,
int nm,
lUlong val,
int snm 
) {
   int ret, sublist_pos;

   DENTER(CULL_LAYER, "lDelSubUlong");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELSUBULONGERRORXRUNTIMETYPE_S  , lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lDelElemUlong(&(ep->cont[sublist_pos].glp), nm, val);

   DEXIT;
   return ret;
}

/****** cull_multitype/lDelElemUlong() ****************************************
*  NAME
*     lDelElemUlong() -- removes an element specified by a ulong field nm 
*
*  SYNOPSIS
*     int lDelElemUlong(lList** lpp, int nm, lUlong val) 
*
*  FUNCTION
*     This function removes an element specified by a ulong field nm with the
*     value val from the list referenced by lpp. 
*
*  INPUTS
*     lList** lpp - reference to a list 
*     int nm      - field id 
*     lUlong val  - value if nm 
*
*  RESULT
*     1 element was found and removed 
*     0 an error occured
*******************************************************************************
*/
int lDelElemUlong(
lList **lpp,
int nm,
lUlong val 
) {
   lListElem *ep;
   int val_pos;

   DENTER(CULL_LAYER, "lDelElemUlong");

   if (!lpp || !val) {
      DPRINTF(("error: NULL ptr passed to lDelElemUlong\n"));
      DEXIT;
      return 0;
   }

   /* empty list ? */
   if (!*lpp) {
      DEXIT;
      return 1;
   }

   /* get position of nm in dp */
   val_pos = lGetPosInDescr(lGetListDescr(*lpp), nm);

   /* run time type checking */
   if (val_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELELEMULONGERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      abort();
   }

   /* seek element */
   ep = lGetElemUlong(*lpp, nm, val);
   if (ep) {
      lFreeElem(lDechainElem(*lpp, ep));
      if (lGetNumberOfElem(*lpp) == 0) {
         lFreeList(*lpp);
         *lpp = NULL;
      }
   }

   DEXIT;
   return 1;
}

/****** cull_multitype/lGetSubUlong() *****************************************
*  NAME
*     lGetSubUlong() -- returns an element specified by a ulong field nm 
*
*  SYNOPSIS
*     lListElem* lGetSubUlong(const lListElem* ep, int nm, lUlong val, int snm) 
*
*  FUNCTION
*     returns an element specified by a ulong field nm an the ulong value
*     val from the sublist snm of the element ep 
*
*  INPUTS
*     const lListElem* ep - element pointer 
*     int nm              - field id which is part of a sublist element of ep 
*     lUlong val          - unsigned long value 
*     int snm             - field id of a list which is part of ep 
*
*  RESULT
*     NULL if element was not found or in case of an error
*     otherwise pointer to the element 
*******************************************************************************
*/
lListElem *lGetSubUlong(
const lListElem *ep,
int nm,
lUlong val,
int snm 
) {
   int sublist_pos;
   lListElem *ret;

   DENTER(CULL_LAYER, "lGetSubUlong");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETSUBULONGERRORXRUNTIMETYPE_S , 
         lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lGetElemUlong(ep->cont[sublist_pos].glp, nm, val);

   DEXIT;
   return ret;
}

/****** cull_multitype/lGetElemUlong() ****************************************
*  NAME
*     lGetElemUlong() -- returns an element specified by a ulong field nm 
*
*  SYNOPSIS
*     lListElem* lGetElemUlong(const lList* lp, int nm, lUlong val) 
*
*  FUNCTION
*     returns an element specified by a ulong field nm an an ulong value
*     val from list lp 
*
*  INPUTS
*     const lList* lp  - list pointer 
*     int nm     - field id 
*     lUlong val - unsigned long value 
*
*  RESULT
*    NULL if element was not found or an error occured
*    otherwise pointer to element 
*******************************************************************************
*/
lListElem *lGetElemUlong(
const lList *lp,
int nm,
lUlong val 
) {
   lListElem *ep;
   int val_pos;
   lUlong s;

   DENTER(CULL_LAYER, "lGetElemUlong");

   /* empty list ? */
   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   val_pos = lGetPosInDescr(lGetListDescr(lp), nm);

   /* run time type checking */
   if (val_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMULONGERRORXRUNTIMETYPE_S, lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   /* seek for element */
   for_each(ep, lp) {
      s = lGetPosUlong(ep, val_pos);
      if (s == val) {
         DEXIT;
         return ep;
      }
   }

   DEXIT;
   return NULL;
}

/****** cull_multitype/lDelSubCaseStr() ***************************************
*  NAME
*     lDelSubCaseStr() -- removes an element specified by a string field nm 
*
*  SYNOPSIS
*     int lDelSubCaseStr(lListElem* ep, int nm, const char* str, int snm) 
*
*  FUNCTION
*     removes an element specified by a string field nm an a string str which
*     is contained in the sublist snm of ep 
*
*  INPUTS
*     lListElem* ep       - element 
*     int nm              - field id 
*     const char* str     - string 
*     int snm             - filed id of the element within element 
*
*  RESULT
*     1 if the element was found an removed 
*     0 in case of error
*******************************************************************************
*/
int lDelSubCaseStr(
lListElem *ep,
int nm,
const char *str,
int snm 
) {
   int ret, sublist_pos;

   DENTER(CULL_LAYER, "lDelSubCaseStr");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELSUBCASESTRERRORXRUNTIMETYPE_S , lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lDelElemCaseStr(&(ep->cont[sublist_pos].glp), nm, str);

   DEXIT;
   return ret;
}

/****** cull_multitype/lDelElemCaseStr() **************************************
*  NAME
*     lDelElemCaseStr() -- removes an element specified by a string field nm 
*
*  SYNOPSIS
*     int lDelElemCaseStr(lList** lpp, int nm, const char* str) 
*
*  FUNCTION
*     This function removes an element specified by nm and str from the list
*     lpp. 
*     If the list does not contain elements after this operation, it will
*     be deleted too.
*
*  INPUTS
*     lList** lpp       - list 
*     int nm            - field id of the element which should be removed 
*     const char* str   - value of the attribute identified by nm 
*
*  RESULT
*     1 if the element was found and removed
*     0 in case of error 
*******************************************************************************
*/
int lDelElemCaseStr(
lList **lpp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;

   DENTER(CULL_LAYER, "lDelElemCaseStr");

   if (!lpp || !str) {
      DPRINTF(("error: NULL ptr passed to lDelElemCaseStr\n"));
      DEXIT;
      return 0;
   }

   /* empty list ? */
   if (!*lpp) {
      DEXIT;
      return 1; 
   }

   /* get position of nm in dp */
   str_pos = lGetPosInDescr(lGetListDescr(*lpp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELELEMCASESTRERRORXRUNTIMETYPE_S , 
         lNm2Str(nm)));
      DEXIT;
      abort();
   }

   /* seek elemtent */
   ep = lGetElemCaseStr(*lpp, nm, str);
   if (ep) {
      lFreeElem(lDechainElem(*lpp, ep));
      if (lGetNumberOfElem(*lpp) == 0) {
         lFreeList(*lpp);
         *lpp = NULL;
      }
   }

   DEXIT;
   return 1;
}

/****** cull_multitype/lGetSubCaseStr() ***************************************
*  NAME
*     lGetSubCaseStr() -- returns an element specified by a string field nm 
*
*  SYNOPSIS
*     lListElem* lGetSubCaseStr(const lListElem* ep, int nm, const char* str, 
*                               int snm) 
*
*  FUNCTION
*     returns an element specified by a string field nm and a string str
*     from a sublist snm of the element ep 
*
*  INPUTS
*     const lListElem* ep - element pointer 
*     int nm              - field within an element of the sublist 
*     const char* str     - string 
*     int snm             - field within ep which identifies the sublist 
*
*  RESULT
*     NULL if element was not found or in case of an error
*     otherwise pointer to element 
*******************************************************************************
*/
lListElem *lGetSubCaseStr(
const lListElem *ep,
int nm,
const char *str,
int snm 
) {
   int sublist_pos;
   lListElem *ret;

   DENTER(CULL_LAYER, "lGetSubStr");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETSUBCASESTRERRORXRUNTIMETYPE_S , 
      lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lGetElemCaseStr(ep->cont[sublist_pos].glp, nm, str);

   DEXIT;
   return ret;
}

/****** cull_multitype/lGetElemCaseStr() **************************************
*  NAME
*     lGetElemCaseStr() -- returns an element specified by a string field 
*
*  SYNOPSIS
*     lListElem* lGetElemCaseStr(const lList* lp, int nm, const char* str) 
*
*  FUNCTION
*     This functions returns an element specified by a string field nm
*     and str from the list lp. 
*
*  INPUTS
*     const lList* lp - Pointer to a list 
*     int nm          - Constant specifying an attribute within an element of 
*                       lp 
*     const char* str - string 
*
*  RESULT
*     NULL when element is not found or an error occured
*     otherwise the pointer to an element 
*
*******************************************************************************
*/
lListElem *lGetElemCaseStr(
const lList *lp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;
   char *s;

   DENTER(CULL_LAYER, "lGetElemCaseStr");
   if (!str) {
      DPRINTF(("error: NULL ptr passed to lGetElemCaseStr\n"));
      DEXIT;
      return NULL;
   }

   /* empty list ? */
   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   str_pos = lGetPosInDescr(lGetListDescr(lp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMCASESTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   /* seek for element */
   for_each(ep, lp) {
      s = lGetPosString(ep, str_pos);
      if (s && !SGE_STRCASECMP(s, str)) {
         DEXIT;
         return ep;
      }
   }

   DEXIT;
   return NULL;
}

/****** cull_multitype/lGetElemHost() *****************************************
*  NAME
*     lGetElemHost() -- returns an element specified by a hostname 
*
*  SYNOPSIS
*     lListElem* lGetElemHost(const lList* lp, int nm, const char* str) 
*
*  FUNCTION
*     returns an element specified by a string field nm and a hostname
*     from the list lp 
*
*  INPUTS
*     const lList* lp - Pointer to an element which contains a hostname 
*     int nm          - String field containing the hostname 
*     const char* str - hostname 
*
*  RESULT
*     NULL when the list does not contain the element or in case of error 
*     otherwise pointer to an element
*******************************************************************************
*/
lListElem *lGetElemHost(
const lList *lp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;
   char *s;
   char uhost[MAXHOSTLEN+1], cmphost[MAXHOSTLEN+1];

   DENTER(TOP_LAYER, "lGetElemHost");
   if (!str) {
      DPRINTF(("error: NULL ptr passed to lGetElemHost\n"));
      DEXIT;
      return NULL;
   }

  /* empty list ? */
   if (!lp) {
      DEXIT;
      return NULL;
   }

   /* get position of nm in sdp */
   str_pos = lGetPosInDescr(lGetListDescr(lp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETELEMHOSTERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      return NULL;
   }

   /* prepare hostname to be searched for */
   hostdup(str, uhost);

   /* seek for element */
   for_each(ep, lp) {
      int equal;
      if ((s = lGetPosString(ep, str_pos))) {
         hostdup(s, cmphost);
         equal = !SGE_STRCASECMP(cmphost, uhost);

         DPRINTF(("hostcmp(%s, %s) equal = %d\n", cmphost, uhost, equal));
         if (equal) {
            DEXIT;
            return ep;
         }
      }
   }

   DEXIT;
   return NULL;
}

/****** cull_multitype/lGetSubHost() ******************************************
*  NAME
*     lGetSubHost() -- returns an element specified by a string field nm 
*
*  SYNOPSIS
*     lListElem* lGetSubHost(const lListElem* ep, int nm, const char* str, 
*                            int snm) 
*
*  FUNCTION
*     returns an element specified by a string field nm and the hostname str
*     from the sublist snm of the element ep 
*
*  INPUTS
*     const lListElem* ep - element pointer 
*     int nm              - field id within an sublist element of ep 
*     const char* str     - hostname 
*     int snm             - field id of a sublist in ep 
*
*  RESULT
*     NULL if element was not found or in case of error
*     otherwise pointer to element 
*******************************************************************************
*/
lListElem *lGetSubHost(
const lListElem *ep,
int nm,
const char *str,
int snm 
) {
   int sublist_pos;
   lListElem *ret;

   DENTER(CULL_LAYER, "lGetSubHost");

   /* get position of sublist in ep */
   sublist_pos = lGetPosViaElem(ep, snm);

   /* run time type checking */
   if (sublist_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_GETSUBCASESTRERRORXRUNTIMETYPE_S , lNm2Str(snm)));
      DEXIT;
      abort();
   }

   ret = lGetElemHost(ep->cont[sublist_pos].glp, nm, str);

   DEXIT;
   return ret;
}

/****** cull_multitype/lDelElemHost() ****************************************
*  NAME
*     lDelElemHost() -- removes an element specified by a string field nm 
*
*  SYNOPSIS
*     int lDelElemHost(lList** lpp, int nm, const char* str) 
*
*  FUNCTION
*     removes an element specified by a string field nm and the hostname
*     str from the list referenced by lpp.
*     If it is the last element within lpp the list itself will be deleted.
*
*  INPUTS
*     lList** lpp       - list 
*     int nm            - field id 
*     const char* str   - string  
*
*  RESULT
*     1 if the host element was found and removed 
*     0 in case of an error
******************************************************************************
*/
int lDelElemHost(
lList **lpp,
int nm,
const char *str 
) {
   lListElem *ep;
   int str_pos;

   DENTER(CULL_LAYER, "lDelElemHost");

   if (!lpp || !str) {
      DPRINTF(("error: NULL ptr passed to lDelElemCaseStr\n"));
      DEXIT;
      return 0;
   }

   /* empty list ? */
   if (!*lpp) {
      DEXIT;
      return 1;
   }

   /* get position of nm in dp */
   str_pos = lGetPosInDescr(lGetListDescr(*lpp), nm);

   /* run time type checking */
   if (str_pos < 0) {
      CRITICAL((SGE_EVENT, MSG_CULL_DELELEMCASESTRERRORXRUNTIMETYPE_S , lNm2Str(nm)));
      DEXIT;
      abort();
   }

   /* seek elemtent */
   ep = lGetElemHost(*lpp, nm, str);
   if (ep) {
      lFreeElem(lDechainElem(*lpp, ep));
      if (lGetNumberOfElem(*lpp) == 0) {
         lFreeList(*lpp);
         *lpp = NULL;
      }
   }

   DEXIT;
   return 1;
}
