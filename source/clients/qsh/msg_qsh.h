#ifndef __MSG_QSH_H
#define __MSG_QSH_H
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

#define MSG_QSH_WAITINGFORINTERACTIVEJOBTOBESCHEDULED    _MESSAGE(17000, _("waiting for interactive job to be scheduled ..."))
#define MSG_QSH_REQUESTFORINTERACTIVEJOBHASBEENCANCELED    _MESSAGE(17001, _("\nRequest for interactive job has been canceled.\n"))
#define MSG_QSH_REQUESTCANTBESCHEDULEDTRYLATER_S    _MESSAGE(17002, _("Your "SFQ" request could not be scheduled, try again later.\n"))
#define MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_S    _MESSAGE(17003, _("\nYour interactive job "SFN" has been successfully scheduled.\n"))
#define MSG_QSH_ESTABLISHINGREMOTESESSIONTO_SS    _MESSAGE(17004, _("Establishing "SFN" session to host "SFN" ...\n"))
#define MSG_CONFIG_CANTGETCONFIGURATIONFROMQMASTER    _MESSAGE(17005, _("\nCannot get configuration from qmaster.\n"))
#define MSG_JOB_CANTGETHOSTOFJOB    _MESSAGE(17006, _("Cannot get host of job.\n"))
#define MSG_EXEC_CANTEXECXYZ_SS    _MESSAGE(17007, _("\nCould not exec "SFN": "SFN"\n"))
#define MSG_QSH_CANTSTARTINTERACTIVEJOB    _MESSAGE(17008, _("\nCould not start interactive job.\n"))
#define MSG_QSH_ERROROPENINGSTREAMSOCKET_S   _MESSAGE(17009, _("error opening stream socket: "SFN"\n"))
#define MSG_QSH_ERRORBINDINGSTREAMSOCKET_S   _MESSAGE(17010, _("error binding stream socket: "SFN"\n"))
#define MSG_QSH_ERRORGETTINGSOCKETNAME_S     _MESSAGE(17011, _("error getting socket name: "SFN"\n"))
#define MSG_QSH_ERRORLISTENINGONSOCKETCONNECTION_S  _MESSAGE(17012, _("error listening on socket connection: "SFN"\n"))
#define MSG_QSH_ERRORWAITINGONSOCKETFORCLIENTTOCONNECT_S _MESSAGE(17013, _("error waiting on socket for client to connect: "SFN"\n"))
#define MSG_QSH_ERRORINACCEPTONSOCKET_S  _MESSAGE(17014, _("error in accept on socket: "SFN"\n"))
#define MSG_QSH_ERRORREADINGSTREAMMESSAGE_S _MESSAGE(17015, _("error reading stream message: "SFN"\n"))
#define MSG_QSH_ERRORENDINGCONNECTION _MESSAGE(17016, _("error: ending connection before all data received\n"))
#define MSG_QSH_ERRORREADINGRETURNCODEOFREMOTECOMMAND _MESSAGE(17017, _("error reading returncode of remote command\n"))
#define MSG_QSH_MALLOCFAILED _MESSAGE(17018, _("malloc failed!\n"))
#define MSG_QSH_CANNOTFORKPROCESS_S _MESSAGE(17019, _("cannot fork process: "SFN))
#define MSG_QSH_EXITEDWITHCODE_SI _MESSAGE(17020, _(SFN" exited with exit code %d\n"))
#define MSG_QSH_EXITEDONSIGNAL_SIS _MESSAGE(17021, _(SFN" exited on signal %d ("SFN")\n"))
#define MSG_QSH_INHERITBUTJOB_IDNOTSET_SSS _MESSAGE(17022, _(SFQ" called with option "SFQ", but "SFQ" not set in environment\n"))
#define MSG_QSH_INVALIDJOB_ID_SS _MESSAGE(17023, _("invalid "SFQ" "SFN"\n"))
#define MSG_QSH_INHERITUSAGE_SS _MESSAGE(17024, _("usage with "SFQ" option: "SFQ"\n"))
#define MSG_QSH_EXECUTINGTASKOFJOBFAILED_IS _MESSAGE(17025, _("executing task of job %d failed: "SFN"\n"))
#define MSG_QSH_CANNOTGETCONNECTIONTOQLOGIN_STARTER_SS _MESSAGE(17026, _("\ncannot get connection to "SFQ" at host "SFQ"\n"))
#define MSG_QSH_ERRORREADINGCONTEXTFROMQLOGIN_STARTER_S _MESSAGE(17027, _("\nerror reading job context from "SFQ"\n"))
#define MSG_QSH_LINEFEED _MESSAGE(17028, _("\n"))
#define MSG_QSH_UNKNOWNJOBSTATUS_X _MESSAGE(17029, _("unknown job status " X32CFormat "\n"))
#define MSG_QSH_SENDINGTASKTO_S _MESSAGE(17030, _("Starting server daemon at host "SFQ"\n"))
#define MSG_QSH_SERVERDAEMONSUCCESSFULLYSTARTEDWITHTASKID_S _MESSAGE(17031, _("Server daemon successfully started with task id "SFQ"\n"))
#define MSG_QSH_CLEANINGUPAFTERABNORMALEXITOF_S _MESSAGE(17032, _("cleaning up after abnormal exit of "SFN"\n"))
#define MSG_QSH_READINGEXITCODEFROMSHEPHERD  _MESSAGE(17033, _("reading exit code from shepherd ... "))

/* --- */
#define MSG_QSH_INHERIT_BN_NOT_ALLOWED_S _MESSAGE(17034, _(SFQ" -inherit is not allowed in combination with -b \'n\'"))



#endif /* __MSG_QSH_H */

