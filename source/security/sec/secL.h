#ifndef SECL_H
#define SECL_H
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

#include "cull.h"
#include "sge_boundaries.h"

enum{ 
   SEC_ConnectionID = SEC_LOWERBOUND,
   SEC_ExpiryDate,
   SEC_SequenceNO,
   SEC_KeyPart0,
   SEC_KeyPart1,
   SEC_KeyPart2,
   SEC_KeyPart3,
   SEC_KeyPart4,
   SEC_KeyPart5,
   SEC_KeyPart6,
   SEC_KeyPart7 
};

LISTDEF( SecurityT )
   ULONG( SEC_ConnectionID )
   STRING( SEC_ExpiryDate )
   ULONG( SEC_SequenceNO )
   ULONG( SEC_KeyPart0 )
   ULONG( SEC_KeyPart1 )
   ULONG( SEC_KeyPart2 )
   ULONG( SEC_KeyPart3 )
   ULONG( SEC_KeyPart4 )
   ULONG( SEC_KeyPart5 )
   ULONG( SEC_KeyPart6 )
   ULONG( SEC_KeyPart7 )
LISTEND

NAMEDEF( SecurityN )
   NAME( "SEC_ConnectionID" )
   NAME( "SEC_ExpiryDate" )
   NAME( "SEC_SequenceNO" )
   NAME( "SEC_KeyPart0" )
   NAME( "SEC_KeyPart1" )
   NAME( "SEC_KeyPart2" )
   NAME( "SEC_KeyPart3" )
   NAME( "SEC_KeyPart4" )
   NAME( "SEC_KeyPart5" )
   NAME( "SEC_KeyPart6" )
   NAME( "SEC_KeyPart7" )
NAMEEND

#define SecurityS sizeof(SecurityN)/sizeof(char*) 


#endif /* SECL_H */
