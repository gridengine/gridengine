#ifndef __MSG_QMOD_H
#define __MSG_QMOD_H
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
** qmod.c
*/
#define MSG_QMOD_CLEAR    _("clear")
#define MSG_QMOD_DISABLE     _("disable")
#define MSG_QMOD_RESCHEDULE  _("reschedule")
#define MSG_QMOD_ENABLE     _("enable")
#define MSG_QMOD_SUSPEND     _("suspend")
#define MSG_QMOD_UNSUSPEND    _("unsuspend")
#define MSG_QMOD_UNKNOWNACTION    _("unknown action")
#define MSG_QMOD_XYOFJOBQUEUEZ_SSS    _("%s%s of job/queue %s\n")
#define MSG_QMOD_c_OPT_USAGE    _("clear error state\n")
#define MSG_QMOD_r_OPT_USAGE    _("reschedule jobs (running in queue)\n")
#define MSG_QMOD_d_OPT_USAGE    _("disable\n")
#define MSG_QMOD_e_OPT_USAGE    _("enable\n")
#define MSG_QMOD_f_OPT_USAGE    _("force action\n")
#define MSG_QMOD_help_OPT_USAGE _("print this help\n")
#define MSG_QMOD_s_OPT_USAGE    _("suspend\n")
#define MSG_QMOD_us_OPT_USAGE   _("unsuspend\n")
#define MSG_QMOD_verify_OPT_USAGE _("just print what would be done\n")


#endif /* __MSG_QMOD_H */

