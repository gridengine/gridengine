#ifndef __MSG_QSTAT_H
#define __MSG_QSTAT_H
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
** qstat.c
*/
#define MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S    _("no queues remaining after %s queue selection\n")
#define MSG_QSTAT_NOQUEUESREMAININGAFTERSELECTION _("no queues remaining after selection\n")
#define MSG_GDI_JOBZOMBIESSGEGDIFAILED              _("job zombies: sge_gdi failed\n")
#define MSG_GDI_PESGEGDIFAILED              _("pe: sge_gdi failed\n")
#define MSG_GDI_CKPTSGEGDIFAILED    _("ckpt: sge_gdi failed\n")
#define MSG_GDI_USERSETSGEGDIFAILED    _("userset: sge_gdi failed\n")
#define MSG_QUEUE_UNKNOWNQUEUEX_S    _("error: unknown queue \"%s\"\n")
#define MSG_PE_UNKNOWNPARALLELENVIRONMENTX_S    _("error: unknown parallel environment \"%s\"\n")
#define MSG_PE_NOSUCHPARALLELENVIRONMENT    _("error: no such parallel environment\n")
#define MSG_OPTIONS_WRONGARGUMENTTOSOPT    _("ERROR! wrong argument to -s option\n")
#define MSG_QSTAT_USAGE_SHOWREASONFORQUEUEALARMSTATE    _("show reason for queue alarm state\n")
#define MSG_QSTAT_USAGE_VIEWALSOSCHEDULINGATTRIBUTES    _("view also scheduling attributes\n")
#define MSG_QSTAT_USAGE_FULLOUTPUT    _("full output\n")
#define MSG_QSTAT_USAGE_FULLOUTPUTANDSHOWRESOURCESOFQUEUES    _("full output and show (selected) resources of queue(s)\n")
#define MSG_QSTAT_USAGE_DISPLAYALLJOBARRAYTASKS    _("display all job-array tasks (do not group)\n")
#define MSG_QSTAT_USAGE_PRINTTHISHELP    _("print this help\n")
#define MSG_QSTAT_USAGE_SHOWSCHEDULERJOBINFO    _("show scheduler job information\n")
#define MSG_QSTAT_USAGE_REQUESTTHEGIVENRESOURCES    _("request the given resources\n")
#define MSG_QSTAT_USAGE_HIDEEMPTYQUEUES    _("hide empty queues\n")
#define MSG_QSTAT_USAGE_SELECTONLYQUEESWITHONOFTHESEPE    _("select only queues with one of these parallel environments\n")
#define MSG_QSTAT_USAGE_PRINTINFOONGIVENQUEUE    _("print information on given queue\n")
#define MSG_QSTAT_USAGE_SHOWSTATUSOFFOREIGNQS    _("show the status of a foreign QS\n")
#define MSG_QSTAT_USAGE_SHOWREQUESTEDRESOURCESOFJOB    _("show requested resources of job(s)\n")
#define MSG_QSTAT_USAGE_SHOWPENDINGRUNNINGSUSPENDESZOMBIEJOBS    _("show pending, running, suspended, zombie jobs,\n")
#define MSG_QSTAT_USAGE_JOBSWITHAUSEROPERATORSYSTEMHOLD    _("jobs with a user/operator/system hold, \n")
#define MSG_QSTAT_USAGE_JOBSWITHSTARTTIMEINFUTORE    _("jobs with a start time in future or any combination only. \n")
#define MSG_QSTAT_USAGE_HISABBREVIATIONFORHUHOHSHJHA    _("h is a abbreviation for huhohshjha\n")
#define MSG_QSTAT_USAGE_SHOWTASKINFO    _("show task information\n")
#define MSG_QSTAT_USAGE_VIEWONLYJOBSOFTHISUSER    _("view only jobs of this user\n")
#define MSG_QSTAT_USAGE_SELECTQUEUESWHEREUSERXHAVEACCESS    _("select only queues where these users have access\n")
#define MSG_QSTAT_USAGE_ASSUMEEMPTYCLUSTERFORREQUESTMATCHING    _("assume empty cluster for request matching\n")
#define MSG_QSTAT_USAGE_ADDITIONALDEBUGGINGOPTIONS    _("Additional debugging options: \n")
#define MSG_QSTAT_USAGE_DUMPCOMPLETEJOBLISTTOSTDOUT    _("dump complete job list to stdout\n")
#define MSG_QSTAT_USAGE_DUMPCOMPLETEQUEUELISTTOSTDOUT    _("dump complete queue list to stdout\n")
#define MSG_SCHEDD_SCHEDULINGINFO    _("scheduling info")
#define MSG_QSI_NOVALIDQSIHOSTSPECIFIED    _("no valid QSI host specified")
#define MSG_QSTAT_CANTREACHXCAUSEY_SS    _("can't reach %s cause %s")
#define MSG_QSTAT_CANTSENDQSTATREQTOQSTD_SSSS    _("unable to send %s request to %s@%s: %s\n")
#define MSG_QSTAT_WAINTINGFORREPLYFROMQSTD_SS    _("waiting for reply from %s@%s ")
#define MSG_QSTAT_CANTGETREPLYONQSTATREQFROMQSTD_SSSS    _("unable to get reply on %s request from %s@%s: %s\n")
#define MSG_QSTAT_FOLLOWINGDONOTEXIST _("Following jobs do not exist: ")


#endif /* __MSG_QSTAT_H */

