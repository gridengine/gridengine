#ifndef __CULL_MULTITYPE_H
#define __CULL_MULTITYPE_H
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

#include "cull_list.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef int (*lCmpFunction)(lListElem *, lListElem *, int);

void lWriteDescrTo(const lDescr *dp, FILE *fp);
int lGetPosInDescr(const lDescr *dp, int name);
int lCountDescr(const lDescr *dp);
lDescr* lCopyDescr(const lDescr *dp);

int lGetPosViaElem(const lListElem *element, int nm);

void lInit(const lNameSpace *namev);
char *lNm2Str(int nm);
char *_lNm2Str(const lNameSpace *nmv, int nm);
int lStr2Nm(const char *str);
int _lStr2Nm(const lNameSpace *nsp, const char *str);

char **lGetPosStringRef(const lListElem *ep, int id);
lList **lGetListRef(const lListElem *ep, int name);
int lGetType(const lDescr *dp, int nm);
int lXchgList(lListElem *ep, int name, lList **lpp);
int lSwapList(lListElem *to, int to_nm, lListElem *from, int from_nm);

lInt lGetPosInt(const lListElem *ep, int id);
lUlong lGetPosUlong(const lListElem *ep, int id);
const char *lGetPosString(const lListElem *ep, int id);
lList *lGetPosList(const lListElem *ep, int id);
lFloat lGetPosFloat(const lListElem *ep, int id);
lDouble lGetPosDouble(const lListElem *ep, int id);
lLong lGetPosLong(const lListElem *ep, int id);
lChar lGetPosChar(const lListElem *ep, int id);
lRef lGetPosRef(const lListElem *ep, int id);

int lSetPosInt(const lListElem *ep, int pos, int value);
int lSetPosUlong(const lListElem *ep, int pos, lUlong value);
int lSetPosString(const lListElem *ep, int pos, const char *value);
int lSetPosList(const lListElem *ep, int pos, lList *value);
int lSetPosFloat(const lListElem *ep, int pos, lFloat value);
int lSetPosDouble(const lListElem *ep, int pos, lDouble value);
int lSetPosLong(const lListElem *ep, int pos, lLong value);
int lSetPosChar(const lListElem *ep, int pos, lChar value);
int lSetPosRef(const lListElem *ep, int pos, lRef value);

lInt lGetInt(const lListElem *ep, int name);
lUlong lGetUlong(const lListElem *ep, int name);
const char *lGetString(const lListElem *ep, int name);
lList *lGetList(const lListElem *ep, int name);
lFloat lGetFloat(const lListElem *ep, int name);
lDouble lGetDouble(const lListElem *ep, int name);
lLong lGetLong(const lListElem *ep, int name);
lChar lGetChar(const lListElem *ep, int name);
lRef lGetRef(const lListElem *ep, int name);

int lSetInt(lListElem *ep, int name, int value);
int lSetUlong(lListElem *ep, int name, lUlong value);
int lSetString(lListElem *ep, int name, const char *value);
int lSetList(lListElem *ep, int name, lList *value);
int lSetFloat(lListElem *ep, int name, lFloat value);
int lSetDouble(lListElem *ep, int name, lDouble value);
int lSetLong(lListElem *ep, int name, lLong value);
int lSetChar(lListElem *ep, int name, lChar value);
int lSetRef(lListElem *ep, int name, lRef value);

int intcmp(lInt i0, lInt i1);
int ulongcmp(lUlong u0, lUlong u1);
int bitmaskcmp(lUlong bm0, lUlong bm1);
int floatcmp(lFloat u0, lFloat u1);
int doublecmp(lDouble u0, lDouble u1);
int charcmp(lChar u0, lChar u1);
int longcmp(lLong u0, lLong u1);
int refcmp(lRef u0, lRef u1);

int incompatibleType(const char *str);
int incompatibleType2(const char *fmt, ...);
int unknownType(const char *str);



/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for lists with a char * as key

*/
lListElem* lAddElemStr(lList **lpp, int nm, const char *str, const lDescr *dp);
int lDelElemStr(lList **lpp, int nm, const char *str); 
lListElem *lGetElemStr(const lList *lp, int nm, const char *str);
lListElem *lGetElemStrFirst(const lList *lp, int nm, const char *str, const void **iterator);
lListElem *lGetElemStrNext(const lList *lp, int nm, const char *str, const void **iterator);
lListElem *lGetElemStrLike(const lList *lp, int nm, const char *str);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for sublists with a char * as key

*/
lListElem* lAddSubStr(lListElem *ep, int nm, const char *str, int snm, const lDescr *dp); 
int lDelSubStr(lListElem *ep, int nm, const char *str, int snm); 
lListElem *lGetSubStr(const lListElem *ep, int nm, const char *str, int snm);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for lists with a ulong as key

*/
lListElem* lAddElemUlong(lList **lpp, int nm, lUlong val, const lDescr *dp);
int lDelElemUlong(lList **lpp, int nm, lUlong val); 
lListElem *lGetElemUlong(const lList *lp, int nm, lUlong val);
lListElem *lGetElemUlongFirst(const lList *lp, int nm, lUlong val, const void **iterator);
lListElem *lGetElemUlongNext(const lList *lp, int nm, lUlong val, const void **iterator);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for sublists with a ulong as key

*/
lListElem* lAddSubUlong(lListElem *ep, int nm, lUlong val, int snm, const lDescr *dp); 
int lDelSubUlong(lListElem *ep, int nm, lUlong val, int snm); 
lListElem *lGetSubUlong(const lListElem *ep, int nm, lUlong val, int snm);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for lists with a case insensitive char * as key

*/
int lDelElemCaseStr(lList **lpp, int nm, const char *str); 
lListElem *lGetElemCaseStr(const lList *lp, int nm, const char *str);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- - 

   functions for sublists with a char * as key

*/
int lDelSubCaseStr(lListElem *ep, int nm, const char *str, int snm); 
lListElem *lGetSubCaseStr(const lListElem *ep, int nm, const char *str, int snm);

/* - -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -

   functions for sublists with a hostname as key

*/

lListElem *lGetElemHost(const lList *lp, int nm, const char *str);
int lDelElemHost(lList **lpp, int nm, const char *str);
lListElem *lGetSubHost(const lListElem *ep, int nm, const char *str, int snm);

#ifdef  __cplusplus
}
#endif

#endif /* __CULL_MULTITYPE_H */
