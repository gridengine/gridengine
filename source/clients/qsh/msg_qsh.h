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


#include "basis_types.h"

/*
** qsh.c
*/
#define MSG_QSH_WAITINGFORINTERACTIVEJOBTOBESCHEDULED    _("waiting for interactive job to be scheduled ...")
#define MSG_QSH_REQUESTFORINTERACTIVEJOBHASBEENCANCELED    _("\nRequest for interactive job has been canceled.\n")
#define MSG_QSH_REQUESTCANTBESCHEDULEDTRYLATER_S    _("Your \"%s\" request could not be scheduled, try again later.\n")
#define MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_D    _("\nYour interactive job " U32CFormat" has been successfully scheduled.\n")
#define MSG_QSH_ESTABLISHINGREMOTESESSIONTO_SS    _("Establishing %s session to host %s ...\n")
#define MSG_CONFIG_CANTGETCONFIGURATIONFROMQMASTER    _("\nCannot get configuration from qmaster.\n")
#define MSG_JOB_CANTGETHOSTOFJOB    _("Cannot get host of job.\n")
#define MSG_EXEC_CANTEXECXYZ_SS    _("\nCould not exec %s: %s\n")
#define MSG_QSH_CANTSTARTINTERACTIVEJOB    _("\nCould not start interactive job.\n")
#define MSG_QSH_ERROROPENINGSTREAMSOCKET_S   _("error opening stream socket: %s\n")
#define MSG_QSH_ERRORBINDINGSTREAMSOCKET_S   _("error binding stream socket: %s\n")
#define MSG_QSH_ERRORGETTINGSOCKETNAME_S     _("error getting socket name: %s\n")
#define MSG_QSH_ERRORLISTENINGONSOCKETCONNECTION_S  _("error listening on socket connection: %s\n")
#define MSG_QSH_ERRORWAITINGONSOCKETFORCLIENTTOCONNECT_S _("error waiting on socket for client to connect: %s\n")
#define MSG_QSH_ERRORINACCEPTONSOCKET_S  _("error in accept on socket: %s\n")
#define MSG_QSH_ERRORREADINGSTREAMMESSAGE_S _("error reading stream message: %s\n")
#define MSG_QSH_ERRORENDINGCONNECTION _("error: ending connection before all data received\n")
#define MSG_QSH_ERRORREADINGRETURNCODEOFREMOTECOMMAND _("error reading returncode of remote command\n")
#define MSG_QSH_MALLOCFAILED _("malloc failed!\n")
#define MSG_QSH_CANNOTFORKPROCESS_S _("cannot fork process: %s")
#define MSG_QSH_EXITEDONSIGNAL_SI _("%s exited on signal %d\n")
#define MSG_QSH_INHERITBUTJOB_IDNOTSET_SSS _("\"%s\" called with option \"%s\", but \"%s\" not set in environment\n")
#define MSG_QSH_INVALIDJOB_ID_SS _("invalid \"%s\" %s\n")
#define MSG_QSH_INHERITUSAGE_SS _("usage with \"%s\" option: \"%s\"\n")
#define MSG_QSH_EXECUTINGTASKOFJOBFAILED_IS _("executing task of job %d failed: %s\n")
#define MSG_QSH_CANNOTGETCONNECTIONTOQLOGIN_STARTER_SS _("\ncannot get connection to \"%s\" at host \"%s\"\n")
#define MSG_QSH_ERRORREADINGCONTEXTFROMQLOGIN_STARTER_S _("\nerror reading job context from \"%s\"\n")
#define MSG_QSH_LINEFEED _("\n")
#define MSG_QSH_UNKNOWNJOBSTATUS_X _("unknown job status " x32 "\n")
#define MSG_QSH_SENDINGTASKTO_S _("Starting server daemon at host \"%s\"\n")
#define MSG_QSH_SERVERDAEMONSUCCESSFULLYSTARTEDWITHTASKID_U _("Server daemon successfully started with task id " U32CFormat "\n")



#endif /* __MSG_QSH_H */

