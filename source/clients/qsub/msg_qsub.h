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
#define MSG_QSUB_WAITINGFORIMMEDIATEJOBTOBESCHEDULED    _MESSAGE(21001, _("Waiting for immediate job to be scheduled.\n"))
#define MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER    _MESSAGE(21003, _("\nYour qsub request could not be scheduled, try again later.\n"))
#define MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U    _MESSAGE(21004, _("\nYour immediate job %s has been successfully scheduled.\n"))
#define MSG_QSUB_COULDNOTINITIALIZEENV_U    _MESSAGE(21005, _("\nUnable to initialize environment because of error: %s\nExiting.\n"))
#define MSG_QSUB_COULDNOTRUNJOB_U    _MESSAGE(21006, _("\nUnable to run job because of error: %s.\nExiting.\n"))
#define MSG_QSUB_COULDNOTWAITFORJOB_U    _MESSAGE(21007, _("\nUnable to wait for job because of error: %s\nExiting.\n"))
#define MSG_QSUB_JOBNEVERRAN_U    _MESSAGE(21008, _("Unable to run job %s\n"))
#define MSG_QSUB_JOBRECEIVEDSIGNAL_SS    _MESSAGE(21009, _("Job %s exited because of signal %s\n"))
#define MSG_QSUB_JOBFINISHUNCLEAR_U    _MESSAGE(21010, _("No information available on job %s's exit status.\n"))
#define MSG_QSUB_COULDNOTFINALIZEENV_U    _MESSAGE(21011, _("\nUnable to finalize environment because of error: %s\nExiting.\n"))
#define MSG_QSUB_YOURJOBHASBEENSUBMITTED_SS    _MESSAGE(21012, _("Your job "SFN" ("SFQ") has been submitted.\n"))
#define MSG_QSUB_JOBEXITED_U    _MESSAGE(210013, _("Job %s exited with exit code %d.\n"))
#define MSG_QSUB_INTERRUPTED_U    _MESSAGE(210015, _("\nInterrupted!\n"))
#define MSG_QSUB_TERMINATING_U    _MESSAGE(210016, _("Please wait while qsub shuts down.\n"))

#endif /* __MSG_QSUB_H */

