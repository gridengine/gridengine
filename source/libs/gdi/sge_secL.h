#ifdef SECURE
#ifndef __SGE_SECL_H
#define __SGE_SECL_H

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

#ifndef __CULL_H
#include "cull.h"
#endif
#include "sge_boundaries.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

enum {
   SEC_ConnectionID = SEC_LOWERBOUND,
   SEC_Host,
   SEC_Commproc,
   SEC_Id,
   SEC_ExpiryDate,
   SEC_SeqNoSend,
   SEC_SeqNoReceive,
   SEC_KeyPart0,
   SEC_KeyPart1,
   SEC_KeyPart2,
   SEC_KeyPart3,
   SEC_KeyPart4,
   SEC_KeyPart5,
   SEC_KeyPart6,
   SEC_KeyPart7
};

LISTDEF(SecurityT)
   SGE_ULONG(SEC_ConnectionID)
   SGE_HOST(SEC_Host)               /* CR - hostname change */
   SGE_STRING(SEC_Commproc)
   SGE_INT(SEC_Id)
   SGE_STRING(SEC_ExpiryDate)
   SGE_ULONG(SEC_SeqNoSend)
   SGE_ULONG(SEC_SeqNoReceive)
   SGE_ULONG(SEC_KeyPart0)
   SGE_ULONG(SEC_KeyPart1)
   SGE_ULONG(SEC_KeyPart2)
   SGE_ULONG(SEC_KeyPart3)
   SGE_ULONG(SEC_KeyPart4)
   SGE_ULONG(SEC_KeyPart5)
   SGE_ULONG(SEC_KeyPart6)
   SGE_ULONG(SEC_KeyPart7)
LISTEND 

NAMEDEF(SecurityN)
   NAME("SEC_ConnectionID")
   NAME("SEC_Host")
   NAME("SEC_Commproc")
   NAME("SEC_Id")
   NAME("SEC_ExpiryDate")
   NAME("SEC_SeqNoSend")
   NAME("SEC_SeqNoReceive")
   NAME("SEC_KeyPart0")
   NAME("SEC_KeyPart1")
   NAME("SEC_KeyPart2")
   NAME("SEC_KeyPart3")
   NAME("SEC_KeyPart4")
   NAME("SEC_KeyPart5")
   NAME("SEC_KeyPart6")
   NAME("SEC_KeyPart7")
NAMEEND

/* *INDENT-ON* */   

#define SecurityS sizeof(SecurityN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* SECL_H */
#endif
