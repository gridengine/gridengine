#ifndef __MSG_QLIMIT_H
#define __MSG_QLIMIT_H
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
** qhost.c
*/

#define MSG_HEADER_RULE "limitation rule"
#define MSG_HEADER_LIMIT "limit"
#define MSG_HEADER_FILTER "filter"

#define MSG_QLIMIT_h_OPT_USAGE        _MESSAGE(10000, _("display only selected hosts"))
#define MSG_QLIMIT_l_OPT_USAGE        _MESSAGE(10001, _("request the given resources"))
#define MSG_QLIMIT_u_OPT_USAGE        _MESSAGE(10002, _("display only selected users"))
#define MSG_QLIMIT_pe_OPT_USAGE       _MESSAGE(10003, _("display only selected parallel environments"))
#define MSG_QLIMIT_P_OPT_USAGE        _MESSAGE(10004, _("display only selected projects"))
#define MSG_QLIMIT_q_OPT_USAGE        _MESSAGE(10005, _("display only selected queues"))

#endif /* __MSG_QLIMIT_H */

