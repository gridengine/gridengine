#ifndef __CULL_DB_H
#define __CULL_DB_H
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

#include "cull_list.h"

#ifdef  __cplusplus
extern "C" {
#endif

int lSplit(lList **slp, lList **ulp, const char *ulp_name, const lCondition *cp);

lList *lSelect(const char *name, const lList *slp, const lCondition *cp, const lEnumeration *ep);

lList* lSelectDestroy(lList *slp, const lCondition *cp);

lList *lJoinSublist(const char *name, int nm0, const lList *lp0, const lCondition *cp0, const lEnumeration *enp0, const lDescr *sldp, const lCondition *cp1, const lEnumeration *enp1);

lList *lJoin(const char *name, int nm0, const lList *lp0, const lCondition *cp0, const lEnumeration *enp0, int nm1, const lList *lp1, const lCondition *cp1, const lEnumeration *enp1);

lDescr *lJoinDescr(const lDescr *sdp0, const lDescr *sdp1, const lEnumeration *ep0, const lEnumeration *ep1);

int lPartialDescr(const lEnumeration *enp, const lDescr *sdp, lDescr *ddp, int *indexp);

int lString2List(const char *s, lList **lpp, const lDescr *dp, int nm, const char *delimitor);
int lString2ListNone(char *s, lList **lpp, const lDescr *dp, int nm, const char *delimitor);

int lDiffListStr(int nm, lList **lpp1, lList **lpp2);

#ifdef  __cplusplus
}
#endif

#endif /* __CULL_DB_H */

