#ifndef __SGE_KRBL_H
#define __SGE_KRBL_H
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

/*
 * This is the list type we use to hold the client
 *  connection list for kerberos authentication
 */

enum {
   KRB_commproc=KRB_LOWERBOUND,
   KRB_id,
   KRB_host,
   KRB_timestamp,
   KRB_auth_context,
   KRB_tgt_list
};

LISTDEF( KRB_Type )
   SGE_STRING( KRB_commproc )
   SGE_ULONG( KRB_id )
   SGE_HOST( KRB_host )                    /* CR - hostname change */
   SGE_ULONG( KRB_timestamp )
   SGE_STRING( KRB_auth_context )
   SGE_LIST( KRB_tgt_list )
LISTEND

NAMEDEF( KRBN )
    NAME( "KRB_commproc" )
    NAME( "KRB_id" )
    NAME( "KRB_host" )
    NAME( "KRB_timestamp" )
    NAME( "KRB_auth_context" )
    NAME( "KRB_tgt_list" )
NAMEEND

#define KRBS sizeof(KRBN)/sizeof(char*)

/*
 * This is the list type we use to hold the client
 * TGT list for kerberos TGT forwarding
 */

enum {
    KTGT_id=KTGT_LOWERBOUND,
    KTGT_tgt
};

LISTDEF( KTGT_Type )
   SGE_ULONG( KTGT_id )
   SGE_STRING( KTGT_tgt )
LISTEND

NAMEDEF( KTGTN )
   NAME( "KTGT_id" )
   NAME( "KTGT_tgt" )
NAMEEND

/* *INDENT-ON* */

#define KTGTS sizeof(KTGTN)/sizeof(char*)

#ifdef  __cplusplus
}
#endif

#endif /* __SGE_KRBL_H*/

