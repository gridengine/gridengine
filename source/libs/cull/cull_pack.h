#ifndef __CULL_PACK_H
#define __CULL_PACK_H
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
#include "pack.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 *
 *   lDescr
 *
 */
int cull_unpack_descr(sge_pack_buffer *pb, lDescr **dpp);
int cull_pack_descr(sge_pack_buffer *pb, const lDescr *dp);

/*
 *
 *   lMultiType
 *
 */
int cull_unpack_cont(sge_pack_buffer *pb, lMultiType **mpp, const lDescr *dp);
int cull_pack_cont(sge_pack_buffer *pb, const lMultiType *mp, const lDescr *dp);

/*
 *
 *   lListElem
 *
 */
int cull_unpack_elem(sge_pack_buffer *pb, lListElem **epp, const lDescr *dp);
int cull_pack_elem(sge_pack_buffer *pb, const lListElem *ep);

/*
 *
 *   lList
 *
 */
int cull_unpack_list(sge_pack_buffer *pb, lList **lpp);
int cull_pack_list(sge_pack_buffer *pb, const lList *lp);

/*
 *
 *   lEnumeration
 *
 */
int cull_unpack_enum(sge_pack_buffer *pb, lEnumeration **epp);
int cull_pack_enum(sge_pack_buffer *pb, const lEnumeration *ep);

/*
 *
 *   lCondition
 *
 */
int cull_unpack_cond(sge_pack_buffer *pb, lCondition **cpp);
int cull_pack_cond(sge_pack_buffer *pb, const lCondition *cp);

#ifdef  __cplusplus
}
#endif

#endif /* #ifndef __CULL_PACK_H */

