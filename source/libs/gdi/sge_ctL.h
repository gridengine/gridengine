#ifndef __SGE_CTL_H
#define __SGE_CTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */ 

/* 
 * this data structures describes the category list 
 */
enum {
   CT_str = CT_LOWERBOUND,   /* string of category */
   CT_refcount,              /* number of jobs referencing the string */
   CT_rejected,              /* has this category been rejected */
   CT_jobs                   /* jobs which belong to this category */
};


ILISTDEF(CT_Type, Categories, SGE_CT_LIST)
   SGE_STRING(CT_str)
   SGE_ULONG(CT_refcount)
   SGE_ULONG(CT_rejected)
   SGE_XLIST(CT_jobs, JR_Type)
LISTEND 

NAMEDEF(CTN)
   NAME("CT_str")
   NAME("CT_refcount")
   NAME("CT_rejected")
   NAME("CT_jobs")
NAMEEND

/* *INDENT-ON* */

#define CTS sizeof(CTN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_CTL_H */
