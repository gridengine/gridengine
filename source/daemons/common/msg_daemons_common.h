#ifndef MSG_DAEMONS_COMMON_H
#define MSG_DAEMONS_COMMON_H
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



#define MSG_OBJ_UNKNOWNREASON         _("<unknown reason>")
/*
** global deamon messages
*/
#define MSG_MASTER    _("master")
#define MSG_STARTUP_STARTINGUP_S           _("starting up %s")
#define MSG_SHADOWD_CONTROLLEDSHUTDOWN_S   _("controlled shutdown %s")
#define MSG_ERROR_CANTSWITCHTOADMINUSER    _("can't switch to amin_user")

/*
** path_aliases.c
*/
#define MSG_ALIAS_INVALIDSYNTAXOFPATHALIASFILEX_S    _("invalid syntax of path alias file '%s'\n")
#define MSG_ALIAS_CANTREAD_SS                        _("can't read path aliasing file \"%s\": %s\n")


/*
** unparse_job_cull.c
*/
#define MSG_LIST_ERRORFORMATINGJIDPREDECESSORLISTASHOLDJID       _("Error formatting jid_predecessor_list as -hold_jid\n")
#define MSG_LIST_ERRORFORMATINGJOBIDENTIFIERLISTASJID       _("Error formatting job_identifier_list as -jid\n")
#define MSG_PARSE_ERRORUNPARSINGMAILOPTIONS       _("Error unparsing mail options\n")
#define MSG_LIST_ERRORFORMATTINGMAILLISTASM       _("Error formatting mail list as -M\n")
#define MSG_PROC_INVALIDPROIRITYMUSTBELESSTHAN1025       _("ERROR! invalid priority, priority must be less than 1025\n")
#define MSG_PROC_INVALIDPRIORITYMUSTBEGREATERTHANMINUS1024       _("ERROR! invalid priority, priority must be greater than -1024\n")
#define MSG_LIST_ERRORFORMATINGHARDQUEUELISTASQ       _("Error formatting hard_queue_list as -q\n")
#define MSG_LIST_ERRORFORMATINGSOFTQUEUELISTASQ       _("Error formatting soft_queue_list as -q\n")
#define MSG_LIST_ERRORFORMATINGQSARGSLIST       _("Error formatting qs_args list\n")
#define MSG_LIST_ERRORFORMATINGSHELLLIST       _("Error formatting shell_list\n")
#define MSG_LIST_ERRORFORMATINGENVIRONMENTLISTASV       _("Error formatting environment list as -v\n")
#define MSG_LIST_ERRORFORMATINGJOBARGUMENTS       _("Error formatting job argumentents\n")
#define MSG_JOB_INVALIDVALUEFORCHECKPOINTATTRIBINJOB_U       _("ERROR! invalid value for checkpoint attribute in job "U32CFormat"\n")
#define MSG_LIST_ERRORFORMATINGHARDRESOURCELISTASL       _("Error formatting hard_resource_list as -l\n")
#define MSG_JOB_JOBHASPEWITHNORANGES       _("Job has parallel environment with no ranges\n")
#define MSG_LIST_ERRORFORMATINGRANGESINPE       _("Error formatting ranges in -pe\n")
#define MSG_LIST_ERRORFORMATINGPATHLIST       _("Error formatting path_list\n")
#define MSG_LIST_ERRORFORMATINGIDLIST       _("Error formatting id list\n")
#define MSG_LIST_ERRORFORMATINGACLLIST       _("Error formatting acl list\n")


/*
** startprog.c
*/
#define MSG_STARTUP_STARTINGPROGRAMMX_S       _("starting program: %s")
#define MSG_PROC_CANTFORKPROCESSTOSTARTX_S       _("can't fork process to start: %s")
#define MSG_PROC_CANTEXECPROCESSORPROCESSDIEDTHROUGHSIGNALX_S       _("cannot exec process or process died through signal: %s")
#define MSG_PROC_CANTSTARTPROCESSX_S       _("couldn't start process: %s")
#define MSG_PROC_WAITPIDRETURNEDUNKNOWNSTATUS       _("waitpid() returned unknown status\n")


/*
** qmaster_running.c
*/
#define MSG_QMASTER_FOUNDRUNNINGQMASTERWITHPIDXNOTSTARTING_I    _("found running qmaster with pid %d - not starting")
#define MSG_QMASTER_ALREADYENROLLEDSHOULDNOTHAPPEN    _("already enrolled - should not happen")
#define MSG_QMASTER_CANTRESOLVEHOSTNAMEXFROMACTQMASTERFILE_S    _("can't resolve hostname "SFQ" from act_qmaster file")
#define MSG_QMASTER_CANTRESOLVESERVEICESGECOMMDTCP    _("can't resolve service \"sge_commd/tcp\"")
#define MSG_QMASTER_COMMDONHOSTXEXPECTSRESERVEDPORT_S    _("commd on host "SFQ" expects reserved port")
#define MSG_QMASTER_COMMDONHOSTXCANTRESOLVEOURHOSTNAME_S    _("commd on host "SFQ" can't resolve our hostname verify your resolving (reverse mapping) or start commd with an alias file")
#define MSG_QMASTER_COMMUNICATIONPROBLEONHOSTX_SS    _("communication problem with commd on host "SFQ": %s")
#define MSG_QMASTER_FOUNDRUNNINGQMASTERONHOSTXNOTSTARTING_S    _("found running qmaster on host "SFQ"- not starting")
#define MSG_QMASTER_CANTCHECKFORRUNNINGQMASTERX_S    _("can't check for running qmaster: %s")
#define MSG_COMMD_CANTCONTACTCOMMDX_S    _("can't contact commd: %s")



/* 
** qmaster_heartbeat.c 
*/
#define MSG_HEART_CANNOTOPEN _("can't open file %s: %s\n")

/*
** mail.c
*/
#define MSG_MAIL_EMPTYUSERHOST         _("sending mail is not possible since user and host are empty")
#define MSG_MAIL_MAILUSER_SSSS         _("sending %s mail to user \"%s\"|mailer \"%s\"|"SFQ)
#define MSG_MAIL_MAILUSERHOST_SSSSS    _("sending %s mail to user \"%s@%s\"|mailer \"%s\"|"SFQ)
#define MSG_MAIL_NOPIPE                _("pipe() for mail failed\n")
#define MSG_MAIL_NOFORK                _("fork() for mail failed\n")
#define MSG_MAIL_NODUP                 _("dup() failed\n")
#define MSG_MAIL_NOEXEC_S              _("exec of mailer \"%s\" failed\n")
#define MSG_MAIL_NOMAIL1               _("wait for mailer returned 0 - killing\n") 
#define MSG_MAIL_NOMAIL2               _("mailer had timeout - killing")
#define MSG_MAIL_NOMAIL3_I             _("mailer was stopped due to signal %d - killing")
#define MSG_MAIL_NOSUBJ                _("<no subject>")


/*
** admin_mail.c 
*/
#define MSG_MAIL_PARSE_S           _("Error parsing mail list >%s<\n")
#define MSG_MAIL_SUBJECT_SUU       "%s: Job-array task "U32CFormat"."U32CFormat" failed"
#define MSG_MAIL_SUBJECT_SU        "%s: Job " U32CFormat " failed"
#define MSG_MAIL_BODY_USSSSSSSS    "Job " U32CFormat " caused action: "SFN"\n User        = "SFN"\n Queue       = "SFN"\n Host        = "SFN"\n Start Time  = "SFN"\n End Time    = "SFN"\nfailed "SFN":"SFN
#define MSG_MAIL_PARSE_S           _("Error parsing mail list >%s<\n")
#define MSG_GFSTATE_QUEUE_S        _("Queue \"%s\" set to ERROR")
#define MSG_GFSTATE_HOST_S         _("All Queues on host \"%s\" set to ERROR")
#define MSG_GFSTATE_JOB_UU         _("Job-array task "U32CFormat"."U32CFormat" set to ERROR")
#define MSG_GFSTATE_JOB_U          _("Job "U32CFormat" set to ERROR")

/*
** config_file.c
*/
#define MSG_CONF_NOCONFVALUE_S        _("can't get configuration value for \"%s\"")
#define MSG_CONF_ATLEASTONECHAR       _("variables need at least one character")
#define MSG_CONF_REFVAR_S             _("referenced variable %20.20s... expands max. length")
#define MSG_CONF_UNKNOWNVAR_S         _("unknown variable \"%s\"")

#endif /* MSG_DAEMONS_COMMON_H */

