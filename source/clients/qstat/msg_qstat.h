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
#define MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S    _MESSAGE(19000, _("no queues remaining after %s queue selection\n"))
#define MSG_QSTAT_NOQUEUESREMAININGAFTERSELECTION _MESSAGE(19001, _("no queues remaining after selection\n"))
#define MSG_GDI_JOBZOMBIESSGEGDIFAILED              _MESSAGE(19002, _("job zombies: sge_gdi failed\n"))
#define MSG_GDI_PESGEGDIFAILED              _MESSAGE(19003, _("pe: sge_gdi failed\n"))
#define MSG_GDI_CKPTSGEGDIFAILED    _MESSAGE(19004, _("ckpt: sge_gdi failed\n"))
#define MSG_GDI_USERSETSGEGDIFAILED    _MESSAGE(19005, _("userset: sge_gdi failed\n"))
#define MSG_QUEUE_UNKNOWNQUEUEX_S    _MESSAGE(19006, _("error: unknown queue \"%s\"\n"))
#define MSG_PE_NOSUCHPARALLELENVIRONMENT    _MESSAGE(19008, _("error: no such parallel environment\n"))
#define MSG_OPTIONS_WRONGARGUMENTTOSOPT    _MESSAGE(19009, _("ERROR! wrong argument to -s option\n"))
#define MSG_QSTAT_USAGE_SHOWREASONFORQUEUEALARMSTATE    _MESSAGE(19010, _("show reason for queue alarm state\n"))
#define MSG_QSTAT_USAGE_VIEWALSOSCHEDULINGATTRIBUTES    _MESSAGE(19011, _("view also scheduling attributes\n"))
#define MSG_QSTAT_USAGE_FULLOUTPUT    _MESSAGE(19012, _("full output\n"))
#define MSG_QSTAT_USAGE_FULLOUTPUTANDSHOWRESOURCESOFQUEUES    _MESSAGE(19013, _("full output and show (selected) resources of queue(s)\n"))
#define MSG_QSTAT_USAGE_DISPLAYALLJOBARRAYTASKS    _MESSAGE(19014, _("display all job-array tasks (do not group)\n"))
#define MSG_QSTAT_USAGE_PRINTTHISHELP    _MESSAGE(19015, _("print this help\n"))
#define MSG_QSTAT_USAGE_SHOWSCHEDULERJOBINFO    _MESSAGE(19016, _("show scheduler job information\n"))
#define MSG_QSTAT_USAGE_REQUESTTHEGIVENRESOURCES    _MESSAGE(19017, _("request the given resources\n"))
#define MSG_QSTAT_USAGE_HIDEEMPTYQUEUES    _MESSAGE(19018, _("hide empty queues\n"))
#define MSG_QSTAT_USAGE_SELECTONLYQUEESWITHONOFTHESEPE    _MESSAGE(19019, _("select only queues with one of these parallel environments\n"))
#define MSG_QSTAT_USAGE_PRINTINFOONGIVENQUEUE    _MESSAGE(19020, _("print information on given queue\n"))
#define MSG_QSTAT_USAGE_SHOWREQUESTEDRESOURCESOFJOB    _MESSAGE(19022, _("show requested resources of job(s)\n"))
#define MSG_QSTAT_USAGE_SHOWPENDINGRUNNINGSUSPENDESZOMBIEJOBS    _MESSAGE(19023, _("show pending, running, suspended, zombie jobs,\n"))
#define MSG_QSTAT_USAGE_JOBSWITHAUSEROPERATORSYSTEMHOLD    _MESSAGE(19024, _("jobs with a user/operator/system hold, \n"))
#define MSG_QSTAT_USAGE_JOBSWITHSTARTTIMEINFUTORE    _MESSAGE(19025, _("jobs with a start time in future or any combination only. \n"))
#define MSG_QSTAT_USAGE_HISABBREVIATIONFORHUHOHSHJHA    _MESSAGE(19026, _("h is a abbreviation for huhohshjha\n"))
#define MSG_QSTAT_USAGE_SHOWTASKINFO    _MESSAGE(19027, _("show task information\n"))
#define MSG_QSTAT_USAGE_VIEWONLYJOBSOFTHISUSER    _MESSAGE(19028, _("view only jobs of this user\n"))
#define MSG_QSTAT_USAGE_SELECTQUEUESWHEREUSERXHAVEACCESS    _MESSAGE(19029, _("select only queues where these users have access\n"))
#define MSG_QSTAT_USAGE_ADDITIONALDEBUGGINGOPTIONS    _MESSAGE(19031, _("Additional debugging options: \n"))
#define MSG_QSTAT_USAGE_DUMPCOMPLETEJOBLISTTOSTDOUT    _MESSAGE(19032, _("dump complete job list to stdout\n"))
#define MSG_QSTAT_USAGE_DUMPCOMPLETEQUEUELISTTOSTDOUT    _MESSAGE(19033, _("dump complete queue list to stdout\n"))
#define MSG_SCHEDD_SCHEDULINGINFO    _MESSAGE(19034, _("scheduling info"))
#define MSG_QSTAT_FOLLOWINGDONOTEXIST _MESSAGE(19040, _("Following jobs do not exist: "))


#endif /* __MSG_QSTAT_H */

