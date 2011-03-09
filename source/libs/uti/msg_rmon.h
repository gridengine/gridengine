#ifndef __MSG_RMON_H
#define __MSG_RMON_H
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

/*
 * rmon/src/rmon_macros.c 
 */
#define MSG_RMON_INVALIDERRNO                   "invalid errno"
#define MSG_RMON_DEBUGLAZERSAREINCONSISTENT     "mpush_layer: debug layers are inconsistent"
#define MSG_RMON_TRIEDTOSETTOLARGELAYER         "mpush_layer: tried to set too large layer"
#define MSG_RMON_TRIEDTOPOPHIGHESTLAYER         "mpop_layer: tried to pop highest layer"
#define MSG_RMON_MKSTDNAMEUNABLETOGETMYHOSTNAME_S  "mkstdname: unable to get my hostname: "SFN
#define MSG_RMON_MKSTDNAMESTRINGTOOLONG         "mkstdname: string too long"
#define MSG_RMON_ILLEGALDBUGLEVELFORMAT         "illegal debug level format"
#define MSG_RMON_ILLEGALDBUGTARGETFORMAT        "illegal debug target format"
#define MSG_RMON_UNABLETOOPENXFORWRITING_S      "unable to open "SFN" for writing"
#define MSG_RMON_ERRNOXY_DS                     "    ERRNO: %d, "SFN
#define MSG_RMON_XERRORINASHAREDMEMOPERATION_I  "(%d) Error in a Shared Memory Operation !"
#define MSG_RMON_XERRORINASEMAPHOREOPERATION_I  "(%d) Error in a Semaphore Operation !"
#define MSG_RMON_FILEXLINEY_SI                  "    File: "SFN", Line: %d"

#endif /* __MSG_RMON_H   */
