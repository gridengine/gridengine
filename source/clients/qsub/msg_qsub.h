#ifndef __MSG_QSUB_H
#define __MSG_QSUB_H
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

#define MSG_QSUB_WARNING    _MESSAGE(21000, _("qsub: warning: "))
#define MSG_QSUB_WAITINGFORIMMEDIATEJOBTOBESCHEDULED    _MESSAGE(21001, _("waiting for immediate job to be scheduled ..."))
#define MSG_QSUB_REQUESTFORIMMEDIATEJOBHASBEENCANCELED    _MESSAGE(21002, _("\nRequest for immediate job has been canceled.\n"))
#define MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER    _MESSAGE(21003, _("\nYour qsub request could not be scheduled, try again later.\n"))
#define MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U    _MESSAGE(21004, _("\nYour immediate job " U32CFormat" has been successfully scheduled.\n"))


#endif /* __MSG_QSUB_H */

