#ifndef __SGE_REQUESTL_H
#define __SGE_REQUESTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* the request himself */
enum {
   RQ_requests = RQ_LOWERBOUND,
   RQ_pe_name,
   RQ_pe_ranges
};

LISTDEF(RQ_Type)
   SGE_LIST(RQ_requests)      /* RE_Type */
   SGE_STRING(RQ_pe_name)
   SGE_LIST(RQ_pe_ranges)
LISTEND 

NAMEDEF(RQN)
   NAME("RQ_requests")
   NAME("RQ_pe_name")
   NAME("RQ_pe_ranges")
NAMEEND

#define RQS sizeof(RQN)/sizeof(char*)

/* the list of ordinary requests */
enum {
   RE_ranges = RE_LOWERBOUND,
   RE_entries                /* CE_Type */
};

SLISTDEF(RE_Type, Request)
   SGE_TLIST(RE_ranges, RN_Type)
   SGE_TLIST(RE_entries, CE_Type)     /* consists of elements with type CE_Type */
LISTEND 

NAMEDEF(REN)
   NAME("RE_ranges")
   NAME("RE_entries")
NAMEEND

/* *INDENT-ON* */

#define RES sizeof(REN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_REQUESTL_H */
