#ifndef __MSG_QDEL_H
#define __MSG_QDEL_H
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
** qdel.c
*/
#define MSG_ANSWER_SUCCESSCHECKWAIT   _MESSAGE(7000, _("removing job(s) - please wait"))
#define MSG_ANSWER_UNKNOWN            _MESSAGE(7001, _("unknown"))
#define MSG_ANSWER_JOBXREMOVED_S      _MESSAGE(7002, _("job \"%s\": ok\n"))
#define MSG_ANSWER_CANTDELETEJOB_S    _MESSAGE(7003, _("can't delete job \"%s\"\n"))
#define MSG_JOB_XDELETIONOFJOBY_SS                      _MESSAGE(7004, _("%sdeletion of job %s\n"))
#define MSG_PARSE_NOOPTIONARGUMENT                      _MESSAGE(7005, _("ERROR! no option argument \n"))
#define MSG_OPTION_SELECTUSERSANDJOBIDSTOGETHERNOTALLOWED              _MESSAGE(7006, _("it is not allowed to select users and job ids together\n"))

#define MSG_QDEL_f_OPT_USAGE             _MESSAGE(7007, _("force action\n"))
#define MSG_QDEL_help_OPT_USAGE          _MESSAGE(7008, _("print this help\n"))
#define MSG_QDEL_verify_OPT_USAGE        _MESSAGE(7009, _("just print what would be done\n"))
#define MSG_QDEL_del_list_1_OPT_USAGE    _MESSAGE(7010, _("delete all jobs given in list\n"))
#define MSG_QDEL_del_list_2_OPT_USAGE    _MESSAGE(7011, _("delete all jobs of current user\n"))
#define MSG_QDEL_del_list_3_OPT_USAGE    _MESSAGE(7012, _("delete all jobs of users specified in list\n"))
#define MSG_QDEL_del_list_4_OPT_USAGE    _MESSAGE(7013, _("delete all jobs of all users\n"))


#endif /* __MSG_QDEL_H */

