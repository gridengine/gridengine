#ifndef __MSG_SPOOLLIB_DYNAMIC_H
#define __MSG_SPOOLLIB_DYNAMIC_H
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

#include "basis_types.h"

#define MSG_SPOOL_ERROROPENINGSHAREDLIB_SS   _MESSAGE(68000, _("error opening shared lib "SFQ": "SFN"\n"))
#define MSG_SPOOL_SHLIBDOESNOTCONTAINSPOOLING_SS   _MESSAGE(68001, _(SFQ" does not contain a valid Grid Engine spooling method: "SFN"\n"))
#define MSG_SPOOL_LOADINGSPOOLINGMETHOD_SS   _MESSAGE(68002, _("loading spooling method "SFQ" from "SFQ"\n"))
#define MSG_SPOOL_ERRORCREATINGCONTEXT_S   _MESSAGE(68003, _("error creating a "SFQ" spooling context\n"))

#endif /* __MSG_SPOOLLIB_DYNAMIC_H */

