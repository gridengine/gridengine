#ifndef __CULL_LIST_H
#define __CULL_LIST_H
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

#include "basis_types.h"
#include "sge_htable.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define NoName -1

typedef struct _lDescr lDescr;
typedef struct _lNameSpace lNameSpace;
typedef struct _lList lList;
typedef struct _lListElem lListElem;
typedef struct _lCondition lCondition;
typedef struct _lEnumeration lEnumeration;
typedef union _lMultiType lMultiType;
typedef struct _lSortOrder lSortOrder;
typedef struct _WhereArg WhereArg, *WhereArgList;

typedef float lFloat;
typedef double lDouble;
typedef u_long32 lUlong;
typedef long lLong;
typedef char lChar;
typedef char lBool;
typedef int lInt;
typedef char *lString;
typedef char *lHost;      /* CR - hostname change */
typedef lListElem *lObject;
typedef void*  lRef;

/* IF YOU CHANGE THIS ENUM, CHANGE cull_multitype.c/multitypes[] */
enum _enum_lMultiType {
   lEndT,                       /* This is the end of the descriptor */
   lFloatT,
   lDoubleT,
   lUlongT,
   lLongT,
   lCharT,
   lBoolT,
   lIntT,
   lStringT,
   lListT,
   lObjectT,
   lRefT,
   lHostT             /* CR - hostname change */
};

/* flags for the field definition 
** reserve 8 bit for data types (currently only 4 bit in use)
*/
#define CULL_DEFAULT       0x00000000
#define CULL_PRIMARY_KEY   0x00000100
#define CULL_HASH          0x00000200
#define CULL_UNIQUE        0x00000400
#define CULL_SHOW          0x00000800
#define CULL_CONFIGURE     0x00001000
#define CULL_SPOOL         0x00002000

#ifdef __SGE_GDI_LIBRARY_HOME_OBJECT_FILE__

#define LISTDEF( name ) lDescr name[] = {
#define XLISTDEF( name, idlname ) lDescr name[] = {
#define ILISTDEF( name, idlname, listname ) lDescr name[] = {
#define SLISTDEF( name, idlname ) lDescr name[] = {
#define LISTEND {NoName, lEndT, NULL}};

#define SGE_INT(name,flags)         { name, lIntT    | flags, NULL },
#define SGE_HOST(name,flags)        { name, lHostT   | flags, NULL },
#define SGE_STRING(name,flags)      { name, lStringT | flags, NULL },
#define SGE_FLOAT(name,flags)       { name, lFloatT  | flags, NULL },
#define SGE_DOUBLE(name,flags)      { name, lDoubleT | flags, NULL },
#define SGE_CHAR(name,flags)        { name, lCharT   | flags, NULL },
#define SGE_LONG(name,flags)        { name, lLongT   | flags, NULL },
#define SGE_ULONG(name,flags)       { name, lUlongT  | flags, NULL },
#define SGE_BOOL(name,flags)        { name, lBoolT   | flags, NULL },
#define SGE_LIST(name,type,flags)   { name, lListT   | flags, NULL },
#define SGE_OBJECT(name,type,flags) { name, lObjectT | flags, NULL },
#define SGE_REF(name,type,flags)    { name, lRefT    | flags, NULL },

/* 
 * For lists, objects and references the type of the subordinate object(s) 
 * must be specified.
 * If multiple types are thinkable or non cull data types are referenced,
 * use the following define CULL_ANY_SUBTYPE as type
 */
#define CULL_ANY_SUBTYPE 0

#define NAMEDEF( name ) char *name[] = {
#define NAME( name ) name ,
#define NAMEEND    };

#else

#define LISTDEF( name ) extern lDescr name[];
#define XLISTDEF( name, idlname ) extern lDescr name[];
#define ILISTDEF( name, idlname, listname ) extern lDescr name[];
#define SLISTDEF( name, idlname ) extern lDescr name[];
#define LISTEND

#define SGE_INT(name,flags)
#define SGE_HOST(name,flags)
#define SGE_STRING(name,flags)
#define SGE_FLOAT(name,flags)
#define SGE_DOUBLE(name,flags)
#define SGE_CHAR(name,flags)
#define SGE_LONG(name,flags)
#define SGE_ULONG(name,flags)
#define SGE_BOOL(name,flags)
#define SGE_LIST(name,type,flags)
#define SGE_OBJECT(name,type,flags)
#define SGE_REF(name,type,flags)

#define NAMEDEF( name ) extern char *name[];
#define NAME( name )
#define NAMEEND

#endif

struct _lNameSpace {
   int lower;
   int size;
   char **namev;
};

struct _lDescr {
   int nm;                             /* name */
   int mt;                             /* multitype information */
   htable ht;
};

/* LIST SPECIFIC FUNCTIONS */
const char *lGetListName(const lList *lp);
const lDescr *lGetListDescr(const lList *lp);
int lGetNumberOfElem(const lList *lp);
int lGetNumberOfRemainingElem(const lListElem *ep);
int lGetElemIndex(const lListElem *ep, const lList *lp);

const lDescr *lGetElemDescr(const lListElem *ep);
void lWriteElem(const lListElem *ep);
void lWriteElemTo(const lListElem *ep, FILE *fp);
void lWriteList(const lList *lp);
void lWriteListTo(const lList *lp, FILE *fp);

lListElem *lCreateElem(const lDescr *dp);
lList *lCreateList(const char *listname, const lDescr *descr);
lList *lCreateElemList(const char *listname, const lDescr *descr, int nr_elem);

lListElem *lFreeElem(lListElem *ep);
lList *lFreeList(lList *lp);

int lAddList(lList *lp0, lList *lp1);
int lCompListDescr(const lDescr *dp0, const lDescr *dp1);
lList *lCopyList(const char *name, const lList *src);
lListElem *lCopyElem(const lListElem *src);
int lModifyWhat(lListElem *dst, const lListElem *src, const lEnumeration *enp);

int lCopyElemPartial(lListElem *dst, int *jp, const lListElem *src, const lEnumeration *ep);
int lCopySwitch(const lListElem *sep, lListElem *dep, int src_idx, int dst_idx);

int lAppendElem(lList *lp, lListElem *ep);
lListElem *lDechainElem(lList *lp, lListElem *ep);
lListElem *lDechainObject(lListElem *parent, int name);
int lRemoveElem(lList *lp, lListElem *ep);
int lInsertElem(lList *lp, lListElem *ep, lListElem *new_elem);

int lPSortList(lList *lp, const char *fmt, ...);
int lSortList(lList *lp, const lSortOrder *sp);
int lUniqStr(lList *lp, int keyfield);
int lUniqHost(lList *lp, int keyfield);

lListElem *lFirst(const lList *lp);
lListElem *lLast(const lList *lp);
lListElem *lNext(const lListElem *ep);
lListElem *lPrev(const lListElem *ep);

lListElem *lFindNext(const lListElem *ep, const lCondition *cp);
lListElem *lFindPrev(const lListElem *ep, const lCondition *cp);
lListElem *lFindFirst(const lList *lp, const lCondition *cp);
lListElem *lFindLast(const lList *lp, const lCondition *cp);


int mt_get_type(int mt);
int mt_do_hashing(int mt);
int mt_is_unique(int mt);

/* #define for_each(ep,lp) for (ep=lFirst(lp);ep;ep=lNext(ep)) */
/* #define for_each_rev(ep,lp) for (ep=lLast(lp);ep;ep=lPrev(ep)) */

#ifndef __cplusplus
#define for_each(ep,lp) for (ep = ((lp) ? (lp)->first : (lListElem *) NULL); ep; ep = ep->next)
#else
#define for_each_cpp(ep,lp) for (ep = ((lp) ? (lp)->first : (lListElem *) NULL); ep; ep = ep->next)
#endif
#define for_each_rev(ep,lp) for (ep = ((lp) ? (lp)->last : (lListElem *) NULL); ep; ep = ep->prev)

#define for_each_where(ep,lp,cp) \
   for (ep=lFindFirst(lp,cp);ep;ep=lFindNext(ep,cp))
#define for_each_where_rev(ep,lp,cp) \
   for (ep=lFindLast(lp,cp);ep;ep=lFindPrev(ep,cp))

#ifdef  __cplusplus
}
#endif

#endif /* #ifndef __CULL_LIST_H */

