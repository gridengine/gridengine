#ifndef __MSG_QMASTER_H
#define __MSG_QMASTER_H
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

#define MSG_SGETEXT_CANTSPOOL_SS                _("qmaster is unable to spool "SFN" "SFQ"\n")
#define MSG_OBJ_JOBS    _("jobs")
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SSS       _("cannot resolve %s name "SFQ": %s")
#define MSG_CONFIG_CANTRESOLVEHOSTNAMEX_SS       _("cannot resolve %s name "SFQ)



/*
** sge_ckptobj.c
*/
#define MSG_OBJ_CKPT                  _("checkpointing environment")
#define MSG_OBJ_CKPTI                 _("checkpoint interface")
#define MSG_OBJ_CKPTENV               _("parameter %s of ckpt environment \"%s\": %s\n")
#define MSG_SGETEXT_CANT_COPY_ELEM_S                  _("can't copy element "SFQ"\n")   
#define MSG_SGETEXT_CANTCOUNT_CKPT_S                  _("can't count checkpoint objects in "SFN"\n") 
#define MSG_SGETEXT_NO_CKPT_LIC                       _("no checkpoint license available\n")  
#define MSG_SGETEXT_NO_INTERFACE_S                    _("no valid checkpoint interface "SFN"\n")   


/*
** sge_follow.c
*/
#define MSG_IGNORE_ORDER_RETRY_I      _("ignoring retry of recent order #%d\n")
#define MSG_UNKNOWN_ERROR_NL          _("unknown error\n")
#define MSG_JOB_NOJOBID               _("can't get job id\n")
#define MSG_JOB_NOTASKID              _("can't get task id\n")
#define MSG_JOB_FINDJOB_U             _("unable to find job "U32CFormat"\n")
#define MSG_JOB_FINDJOBTASK_UU        _("unable to find task "U32CFormat" of job "U32CFormat"\n")

#define MSG_ORD_OLDVERSION_UUU        _("scheduler sent order for old version "U32CFormat" of job "U32CFormat"."U32CFormat"\n")
#define MSG_ORD_TWICE_UU              _("scheduler tries to schedule job "U32CFormat"."U32CFormat" twice\n")
#define MSG_ORD_INITIALTICKETS_U      _(" with "U32CFormat" initial tickets")

#define MSG_OBJ_UNABLE2FINDPE_S       _("unable to find pe \"%s\"\n")
#define MSG_OBJ_NOQNAME               _("can't get q_name\n")

#define MSG_ORD_QVERSION_UUS          _("orders queue version ("U32CFormat") is not uptodate ("U32CFormat") for queue \"%s\"\n")
#define MSG_ORD_USRPRJVERSION_UUS          _("orders user/project version ("U32CFormat") is not uptodate ("U32CFormat") for user/project \"%s\"\n")


#define MSG_JOB_JOBACCESSQ_US         _("job "U32CFormat" has no access to queue \"%s\"\n")
#define MSG_JOB_FREESLOTS_US          _("not enough ("U32CFormat") free slots in queue \"%s\"\n")
#define MSG_JOB_QMARKEDERROR_S        _("queue \"%s\" is marked QERROR\n") 
#define MSG_JOB_QSUSPCAL_S            _("while scheduling queue \"%s\" was suspended on calendar\n")
#define MSG_JOB_QDISABLECAL_S         _("while scheduling queue \"%s\" was disabled on calendar\n")
#define MSG_JOB_UNABLE2FINDHOST_S     _("unable to locate host in exechost list \"%s\"\n")
#define MSG_JOB_UNABLE2STARTJOB_US    _("unable to start job "U32CFormat" before cleanup on host "SFN" has finished\n")
#define MSG_JOB_HOSTNAMERESOLVE_US    _("failed starting job "U32CFormat" - probably hostname resolving problem with \"%s\"\n") 
#define MSG_JOB_JOBDELIVER_UU         _("failed delivering job "U32CFormat"."U32CFormat)
#define MSG_JOB_UNABLE2FINDJOBORD_U   _("unable to find job \""U32CFormat"\" in ticket order\n")     
#define MSG_JOB_CHANGETICKETS_UU      _("scheduler tries to change tickets of a non running job "U32CFormat" task "U32CFormat"\n")
#define MSG_JOB_CHANGEPTICKETS_UU      _("scheduler tries to change pending tickets of a non pending job "U32CFormat" task "U32CFormat"\n")
#define MSG_JOB_REMOVENOTFINISHED_U   _("scheduler tried to remove job "U32CFormat" which is not in state JFINISHED\n")
#define MSG_JOB_REMOVENONINTERACT_U   _("scheduler tried to remove non interactive job "U32CFormat" by use of a ORT_remove_immediate_job order\n")
#define MSG_JOB_REMOVENONIMMEDIATE_U  _("scheduler tried to remove non immediate job "U32CFormat" by use of a ORT_remove_immediate_job order\n")
#define MSG_JOB_REMOVENOTIDLEIA_U     _("scheduler tried to remove interactive job "U32CFormat" but it is not in JIDLE state\n")
#define MSG_JOB_NOFREERESOURCEIA_U    _("no free resource for interactive job "U32CFormat"\n")
#define MSG_MAIL_CREDITLOWSUBJ_SUS    SFN": Credit low for job " U32CFormat " ("SFN")\n"
#define MSG_MAIL_CREDITLOWBODY_USSFF  _("Your job " U32CFormat " ("SFN") attached to project '%s' has\nfallen below low credit limit.\nActual Credit: %.2f   Low Credit Limit: %.2f\n")
#define MSG_JOB_SUSPOTNOTRUN_UU       _("got ORT_suspend_on_threshold order for non running task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_UNSUSPOTNOTRUN_UU     _("got ORT_unsuspend_on_threshold order for non running task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_UNSUSPOT_UUS          _("unsuspending job "U32CFormat"."U32CFormat" according to suspend threshold of queue \"%s\"\n")
#define MSG_JOB_UNABLE2FINDMQ_SU      _("unable to find master queue %s of job "U32CFormat"\n")
#define MSG_JOB_SUSPTQ_UUS            _("suspending job "U32CFormat"."U32CFormat" according to suspend threshold of queue \"%s\"\n")
#define MSG_JOB_MISSINGJOBTASK_UU     _("missing job "U32CFormat"."U32CFormat" appeared in ticket orders list")
#define MSG_ORD_UNABLE2FINDHOST_S     _("unable to lookup host for queue %s in parallel job ticket order")


/*
** parse_onoff.c
*/
#define MSG_TOKEN_UNRECOGNIZEDSTRING_S                  _("unrecognized string \"%s\"")
#define MSG_ANSWER_ERRORINDISABLYEAROFCALENDARXY_SS     _("error in disabled_year of calendar \"%s\": %s\n")
#define MSG_ANSWER_GOTEQUALWITHOUTDAYTIMERANGEORSTATE   _("got \"=\" without daytime_range/state")
#define MSG_ANSWER_FIRSTYESTERDAYINRANGEMUSTBEBEFORESECONDYESTERDAY     _("first yearday in range must be before second yearday")
#define MSG_PARSE_MISSINGPOINTAFTERDAY      _("missing \".\" after day\n")
#define MSG_PARSE_MISSINGPOINTAFTERMONTH    _("missing \".\" after month\n")
#define MSG_PARSE_WOUTSIDEOFRANGEXYZ_SIIS   _("\"%s\" outside of range %d-%d of %s")
#define MSG_PARSE_XISNOTAY_SS               _("\"%s\" is not a %s")
#define MSG_PARSE_MISSINGDASHINDAYTIMERANGE _("missing '-' in daytime range")
#define MSG_PARSE_RANGEBEGISEQUALTOEND      _("range begin is equal to end - use 0-24 instead")
#define MSG_PARSE_DAYTIMESBEYOND24HNOTALLOWED  _("daytimes may not be beyond 24:00\n")
#define MSG_PARSE_HOURSPEC                  _("hour specification")
#define MSG_PARSE_MINUTESPEC                _("minute specification")
#define MSG_PARSE_SECONDSSPEC               _("seconds specification")
#define MSG_PARSE_XISNOTASTATESPECIFIER_S   _("\"%s\" is not a state specifier")
#define MSG_PARSE_UNRECOGNIZEDTOKENATEND    _("unrecognized token at end\n")
#define MSG_PARSE_ERRORINDISABLEDWEEKOFCALENDAR_SS     _("error in disabled_week of calendar \"%s\": %s\n")
#define MSG_PARSE_FOUNDUSELESSWEEKDAYRANGE  _("found useless weekday range")
#define MSG_PARSE_EXPECTEDSTRINGFORWEEKDAY  _("expected string for weekday")
#define MSG_PARSE_XISNOTAWEEKDAY_S          _("\"%s\" is not a weekday")
#define MSG_PARSE_OVERFLOWERRORWHILEPARSING _("overflow error while parsing\n")


/*
** global qmaster messages
*/ 


/* 
** gdi_utility_qmaster.c 
*/
#define MSG_GDI_SIG_DIGIT_SS         _("denied: attribute "SFQ" contains invalid value "SFQ"\n")
#define MSG_GDI_METHOD_VARS_SS       _("parameter "SFQ" of queue: %s\n")
#define MSG_GDI_APATH_S              _("denied: path given for "SFQ" must start with an \"/\"\n")
#define MSG_GDI_VARS_SS              _("parameter "SFQ": %s\n")
#define MSG_GDI_VALUE_S              _("denied: attribute "SFQ" contains invalid value (null)\n")
#define MSG_GDI_TYPE_MEM_SS          _("value for attribute %s "SFQ" is not a memory value\n")
#define MSG_GDI_TYPE_TIME_SS         _("value for attribute %s "SFQ" is not a time value\n")
#define MSG_GDI_KEYSTR_SS            _("invalid %s "SFQ"\n")
#define MSG_GDI_MULTIPLE_OCCUR_SSSS  _("denied: multiple occurances of %s "SFQ" in %s "SFQ"\n")
#define MSG_GDI_NO_ATTRIBUTE_SSS     _("denied: attribute "SFQ" is not configured for %s "SFQ"\n")
#define MSG_GDI_KEYSTR_FIRSTCHAR_SC  _("%s (\'%c\') not allowed as first character of objectname\n") 
#define MSG_GDI_KEYSTR_FIRSTCHAR_S   _("%s not allowed as first character of objectname\n") 
#define MSG_GDI_KEYSTR_MIDCHAR_SC    _("%s (\'%c\') not allowed in objectname\n") 
#define MSG_GDI_KEYSTR_MIDCHAR_S     _("%s not allowed in objectname\n") 
#define MSG_GDI_KEYSTR_KEYWORD_SS     _("%s ("SFQ") not allowed as objectname\n") 
#define MSG_GDI_KEYSTR_KEYWORD         _("Keyword")
#define MSG_GDI_KEYSTR_DOT             _("Dot")
#define MSG_GDI_KEYSTR_HASH            _("Hash")
#define MSG_GDI_KEYSTR_RETURN          _("Return")
#define MSG_GDI_KEYSTR_TABULATOR       _("Tabulator")
#define MSG_GDI_KEYSTR_CARRIAGERET     _("Carriage return")
#define MSG_GDI_KEYSTR_SPACE           _("Space")
#define MSG_GDI_KEYSTR_SLASH           _("Slash")
#define MSG_GDI_KEYSTR_COLON           _("Colon")
#define MSG_GDI_KEYSTR_QUOTE           _("Quote")
#define MSG_GDI_KEYSTR_DBLQUOTE        _("Double quote")
#define MSG_GDI_KEYSTR_BACKSLASH       _("Backslash")
#define MSG_GDI_KEYSTR_BRACKETS        _("Backslash")
#define MSG_GDI_KEYSTR_BRACES          _("Braces")
#define MSG_GDI_KEYSTR_PARENTHESIS     _("Parenthesis")
#define MSG_GDI_KEYSTR_AT              _("AT")

/*
** ck_to_do_qmaster.c
*/
#define MSG_FILE_OPEN_S              _("can't open "SFQ" for appending system state info")

/*
** sge_c_ack.c
*/
#define MSG_COM_NOQUEUE              _("<no queue>")
#define MSG_COM_UNPACKINT_I          _("unpacking integer %d failed\n")
#define MSG_COM_ACK_S                _("ack event from %s\n")     
#define MSG_COM_ACKEVENTFORUNKOWNJOB_U _("ack event for unknown job "U32CFormat"\n")
#define MSG_COM_ACKEVENTFORUNKNOWNTASKOFJOB_UU _("ack event for unknown task " U32CFormat  " of job " U32CFormat " \n")
#define MSG_COM_UNKNOWN_TAG          _("received unknown ack tag "U32CFormat"\n")

#define MSG_COM_ACK_U                _("signalling acknowledged for unknown job " U32CFormat  "\n")
#define MSG_COM_ACK_UU               _("signalling acknowledged for unknown task " U32CFormat  " of job " U32CFormat " \n")
#define MSG_COM_ACK_US               _("ack event for job " U32CFormat  " can't find queue %s\n")
#define MSG_COM_ACK_QUEUE_U          _("ack event for unknown queue " U32CFormat "\n")
#define MSG_COM_ACK_UNKNOWN          _("unknown ack event\n")
#define MSG_COM_NO_EVCLIENTWITHID_U  _("no event client with id "U32CFormat" registered\n")

/*
** sge_c_gdi.c
*/
#define MSG_GDI_WRONG_GDI_SSISS       _("denied: client ("SFN"/"SFN"/%d) uses old GDI version %s while qmaster uses newer version %s\n")
#define MSG_GDI_WRONG_GDI_SSIUS       _("denied: client ("SFN"/"SFN"/%d) uses newer GDI version "U32CFormat" while qmaster uses older version %s\n")


#define MSG_GDI_NULL_IN_GDI_SSS       _("denied: got NULL in %s/%s of gdi request from host \"%s\"\n")
#define MSG_GDI_OKNL                  _("ok\n")
#define MSG_OBJ_GENQ                  _("internal error - can't get generic queue\n")
#define MSG_OBJ_QLIST                 _("queue list")
#define MSG_MEM_MALLOC                _("malloc failure\n")
#define MSG_SGETEXT_UNKNOWNOP                   _("unknown operation\n")
#define MSG_SGETEXT_FEATURE_NOT_AVAILABLE_FORX_S _("feature not available for "SFN" system\n")
#define MSG_SGETEXT_CANTGET_SS                  _(""SFN" can't get "SFN"\n")
#define MSG_SGETEXT_OPNOIMPFORTARGET            _("operation not implemented for target\n")
#define MSG_SGETEXT_NOADMINHOST_S               _("denied: host "SFQ" is no admin host\n")
#define MSG_SGETEXT_NOSUBMITHOST_S              _("denied: host "SFQ" is no submit host\n")
#define MSG_SGETEXT_NOSUBMITORADMINHOST_S       _("denied: host "SFQ" is neither submit nor admin host\n")
#define MSG_SGETEXT_ALREADYEXISTS_SS            _(""SFN" "SFQ" already exists\n")
#define MSG_SGETEXT_ADDEDTOLIST_SSSS            _(""SFN"@"SFN" added "SFQ" to "SFN" list\n")
#define MSG_SGETEXT_MODIFIEDINLIST_SSSS         _(""SFN"@"SFN" modified "SFQ" in "SFN" list\n")

/*
** sge_calendar_qmaster.c
*/
#define MSG_OBJ_CALENDAR              _("calendar")
#define MSG_EVE_TE4CAL_S              _("got timer event for unknown calendar "SFQ"\n")
#define MSG_SGETEXT_CALENDARSTILLREFERENCEDINQUEUE_SS _("denied: calendar "SFQ" is still referenced in queue "SFQ"\n") 
#define MSG_SGETEXT_REMOVEDFROMLIST_SSSS        _(""SFN"@"SFN" removed "SFQ" from "SFN" list\n")




/*
** sge_give_jobs.c
*/
#define MSG_OBJ_NOREALNAMEINHOST_S    _("missing real name in host \"%s\"\n")
#define MSG_COM_NOTENROLLEDONHOST_SSU _("target \"%s\" not enrolled on host \"%s\" for delivering job \""U32CFormat"\"\n")
#define MSG_OBJ_UNABLE2FINDCKPT_S     _("can't find checkpointing object \"%s\"\n")
#define MSG_OBJ_UNABLE2CREATECKPT_SU  _("can't create checkpointing object \"%s\" for job " U32CFormat "\n")
#define MSG_SEC_NOCRED_USSI           _("could not get credentials for job " U32CFormat " for execution host %s - command \"%s\" failed with return code %d\n")
#define MSG_SEC_NOCREDNOBIN_US        _("could not get client credentials for job " U32CFormat" - %s binary does not exist\n")
#define MSG_COM_SENDJOBTOHOST_US      _("can't send job \"" U32CFormat"\" to host \"%s\"")
#define MSG_COM_RESENDUNKNOWNJOB_UU   _("cannot resend unknown job "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_UNKNOWNGDIL4TJ_UU     _("transfering job "U32CFormat"."U32CFormat" has an invalid gdi list --- deleting")
#define MSG_JOB_NOQUEUE4TJ_SUU        _("queue \"%s\" in transiting job "U32CFormat"."U32CFormat" doesn't exist. deleting task")
#define MSG_JOB_NOHOST4TJ_SUU         _("execution host \"%s\" for transfering job "U32CFormat"."U32CFormat" doesn't exist. deleting task")
#define MSG_JOB_NOPE4TJ_SUU           _("parallel environment \"%s\" for transfering job "U32CFormat"." U32CFormat" doesn't exist. deleting job")
#define MSG_JOB_DELIVER2Q_UUS         _("failed to deliver job "U32CFormat"."U32CFormat" to queue \"%s\"")
#define MSG_SEC_STARTDELCREDCMD_SU    _("can't start command \"%s\" for job " U32CFormat " to delete credentials\n")
#define MSG_SEC_DELCREDSTDERR_S       _("delete_cred stderr: %s")
#define MSG_SEC_DELCREDRETCODE_USI    _("could not delete credentials for job " U32CFormat" - command \"%s\" failed with return code %d\n")
#define MSG_SEC_DELCREDNOBIN_US       _("could not delete credentials for job "U32CFormat" - %s binary does not exist\n")
#define MSG_LOG_SENT2EXECD            _("sent to execd")
#define MSG_LOG_EXITED                _("job exited")
#define MSG_LOG_WAIT4SGEDEL           _("job waits for schedds deletion")
#define MSG_LOG_DELSGE                _("job deleted by schedd")
#define MSG_LOG_DELIMMEDIATE          _("immediate job deleted by schedd")
#define MSG_LOG_JATASKEXIT            _("ja task exited")  
#define MSG_JOB_RESCHEDULE_UU         _("rescheduling job "U32CFormat"."U32CFormat) 
#define MSG_RU_CANCELED_S             _("Due to a modification of the reschedule_unknown timeout rescheduling for host "SFN" was canceled.")
#define MSG_RU_TRIGGER_SU             _("Due to a modification of the reschedule_unknown timeout rescheduling for host "SFN" will be triggerd in "U32CFormat" seconds.")
#define MSG_RU_MAILSUB_SS             _("Pushed rescheduling of "SFN" "SFN)
#define MSG_RU_MAILBODY_SSSS          _("Your "SFN" "SFN" is was running on host "SFN". "SFN" manually/automatic rescheduling for this "SFN".")
#define MSG_RU_MAILTYPE               _("job rescheduling")
#define MSG_RU_PUSHEDR                _("Pushed")
#define MSG_RU_FORCEDR                _("Forced")
#define MSG_RU_MSGFILEINFO            _(SFN" rescheduling of "SFN" "SFN" on host "SFN"\n")
#define MSG_RU_TYPEJOBARRAY           _("job-array task")
#define MSG_RU_TYPEJOB                _("job")
#define MSG_RU_JR_ERRSTR              _("manual/auto rescheduling")  
#define MSG_RU_NOT_RESTARTABLE_SS     _("The "SFN" "SFN" is not restartable\n")
#define MSG_RU_INTERACTIVEJOB_SSS     _(SFN" is a qsh, qlogin, qrsh or a qrlogin "SFN". These types of "SFN" are not restartable\n")
#define MSG_RU_CKPTNOTVALID_SSS       _(SFN" requests ckpt object "SFN". Therefore this "SFN" is not restartable.\n")
#define MSG_RU_CKPTEXIST_SS           _(SFN" requests ckpt object "SFN". This ckpt object does not exist.\n")
#define MSG_RU_INDELETEDSTATE_SS      _("The "SFN" "SFN" is already in deleted state. No rescheduling!\n")
#define MSG_RU_NORERUNQUEUE_SSS       _("The "SFN" "SFN" is running in queue "SFN" were jobs are not rerunable.\n")

/*
** sge_host_qmaster.c
*/
#define MSG_OBJ_DELGLOBALHOST         _("denied: pseudo host \"global\" may not be deleted\n")
#define MSG_OBJ_LOADREPORTIVAL_SS     _("host "SFQ": "SFQ" is not a valid time value for \"load_report_time\" - assuming 120 seconds\n")    
#define MSG_OBJ_RESCHEDULEUNKN_SS     _("host "SFQ": "SFQ" is not a valid time value for \"reschedule_unknown\" - assuming 0 => no auto rescheduling\n")
#define MSG_OBJ_SHUTDOWNPERMS         _("shutting down execd requires manager privileges\n")
#define MSG_OBJ_NOEXECDONHOST_S       _("no execd known on host %s\n")
#define MSG_COM_NONOTIFICATION_SSS    _("failed sending %s notification to %s execd host %s\n")
#define MSG_COM_NONOTIFICATIONQ_SSSS   _("failed sending %s notification to %s %s host %s\n")
#define MSG_COM_NOTIFICATION_SSS      _("sent %s notification to %s execd host %s\n")
#define MSG_COM_NOTIFICATIONQ_SSSS     _("sent %s notification to %s %s host %s\n")
#define MSG_OBJ_UNKNOWN               _("unknown")
#define MSG_NOTIFY_SHUTDOWNANDKILL    _("shutdown and kill")
#define MSG_NOTIFY_SHUTDOWN           _("shutdown")
#define MSG_MAIL_JOBKILLEDSUBJ_US     "Job " U32CFormat " ("SFN") Killed"
#define MSG_MAIL_JOBKILLEDBODY_USS    _("Job " U32CFormat " ("SFN") was killed due to a kill execd on host %s")
#define MSG_OBJ_INVALIDHOST_S         _("invalid hostname "SFQ"\n")
#define MSG_OBJ_NOADDHOST_S           _("adding host "SFQ" failed\n")
#define MSG_LOG_REGISTER_SS           _("%s on %s registered")
#define MSG_OBJ_NOSCALING4HOST_SS     _("denied: scaling attribute "SFQ" is not configured for host "SFQ"\n")  
#define MSG_SGETEXT_ISNOEXECHOST_S              _(SFQ" is not an execution host\n")
#define MSG_SGETEXT_NOEXECHOSTS                 _("there are no execution hosts to kill\n")
#define MSG_SGETEXT_CANTCOUNT_HOSTS_S                 _("can't count adm./subm. host list in "SFN"\n")
#define MSG_SGETEXT_CANTDELADMINQMASTER_S       _("denied: can't delete master host "SFQ" from admin host list\n")   
#define MSG_SGETEXT_CANTDELEXECACTIVQ_S         _("denied: "SFQ" has an active queue - not deleted\n")
#define MSG_SGETEXT_TOOFEWSUBMHLIC_II                 _("not enough submit host licenses; licensed=%d - requested=%d\n") 
#define MSG_CANT_ASSOCIATE_LOAD_SS    _("got load report from host "SFQ" - reports load value for host "SFQ"\n")

/*
** sge_job.c
*/
#define MSG_JOB_SENDKILLTOXFORJOBYZ_SUU _("send kill to "SFQ" for job "U32CFormat"."U32CFormat)
#define MSG_JOB_NORESUBPERMS_SSS      _("job rejected: %s@%s is not allowed to resubmit jobs of user %s\n")
#define MSG_JOB_NOPERMS_SS            _("job rejected: %s@%s is not allowed to submit jobs\n")
#define MSG_JOB_MORETASKSTHAN_U       _("job rejected: You try to submit a job with more than "U32CFormat" tasks\n")
#define MSG_JOB_UID2LOW_II            _("job rejected: your user id %d is lower than minimum user id %d of cluster configuration\n")
#define MSG_JOB_GID2LOW_II            _("job rejected: your group id %d is lower than minimum group id %d of cluster configuration\n")
#define MSG_JOB_ALLOWEDJOBSPERUSER    _("job rejected: Only "U32CFormat" jobs are allowed per user\n")
#define MSG_JOB_ALLOWEDJOBSPERCLUSTER _("job rejected: Only "U32CFormat" jobs are allowed per cluster\n")
#define MSG_JOB_QNOTREQUESTABLE       _("job was rejected because job requests a queue while queues are not configured as requestable\n")
#define MSG_JOB_QUNKNOWN_S            _("job was rejected because job requests unknown queue "SFQ"\n")
#define MSG_JOB_NOSCRIPT              _("job rejected: no script in your request\n")
#define MSG_JOB_PEUNKNOWN_S           _("job rejected: the requested parallel environment "SFQ" does not exist\n")
#define MSG_JOB_CKPTUNKNOWN_S         _("job rejected: the requested checkpointing environment "SFQ" does not exist\n")
#define MSG_JOB_PERANGEMUSTBEGRZERO   _("job rejected: pe range must be greater than zero\n")
#define MSG_JOB_CKPTMINUSC            _("job rejected: checkpointing with \"-c n\" requested\n")
#define MSG_JOB_NOCKPTREQ             _("job rejected: checkpointing without checkpointing environment requested\n")
#define MSG_JOB_CKPTDENIED            _("checkpointing denied\n")   
#define MSG_JOB_NOTINANYQ_S           _("warning: %s your job is not allowed to run in any queue\n")
#define MSG_JOB_PRJUNKNOWN_S          _("job rejected: the requested project "SFQ" does not exist\n")
#define MSG_JOB_USRUNKNOWN_S          _("job rejected: the user "SFQ" does not exist\n")
#define MSG_JOB_CRED4PRJLOW_FSF       _("credit %.2f of project '"SFN"' is below credit limit %.2f\n")
#define MSG_JOB_PRJNOSUBMITPERMS_S    _("job rejected: project "SFQ" is not allowed to submit jobs\n")
#define MSG_JOB_PRJREQUIRED           _("job rejected: no project was supplied and only certain projects are allowed to submit jobs\n")
#define MSG_JOB_NODEADLINEUSER_S      _("job rejected: the user "SFQ" is no deadline initiation user\n")
#define MSG_JOB_TASKIDZERO_U          _("job "U32CFormat" was rejected cause the job contains a task with id 0\n")
#define MSG_JOB_NOJOBNAME_U           _("job "U32CFormat" was rejected cause there is no job_name in the request\n")
#define MSG_JOB_NOWRITE_US            _("job "U32CFormat" was rejected cause it can't be written: %s\n")
#define MSG_JOB_NOWRITE_U             _("job "U32CFormat" was rejected cause it couldn't be written\n")
#define MSG_SEC_NOAUTH_U              _("job "U32CFormat" rejected because authentication failed\n")
#define MSG_SEC_NOSTARTCMD4GETCRED_SU _("can't start command \"%s\" for job " U32CFormat " to get credentials\n")
#define MSG_SEC_PUTCREDSTDERR_S       _("put_cred stderr: %s")
#define MSG_SEC_NOSTORECRED_USI       _("could not store credentials for job " U32CFormat" - command \"%s\" failed with return code %d\n")
#define MSG_SEC_NOSTORECREDNOBIN_US   _("could not store client credentials for job " U32CFormat" - %s binary does not exist\n")
#define MSG_JOB_SUBMITJOB_USS         _("your job "U32CFormat" (\""SFN SFN"\") has been submitted\n")
#define MSG_JOB_SUBMITJOBARRAY_UUUUSS _("your job-array "U32CFormat"."U32CFormat"-"U32CFormat":"U32CFormat" (\""SFN SFN"\") has been submitted\n")
#define MSG_LOG_NEWJOB                _("new job")
#define MSG_JOB_MODIFYALL             _("modify all jobs")
#define MSG_JOB_DELETEJOB             _("delete job")
#define MSG_JOB_JOB                   _("Job")
#define MSG_JOB_FORCEDDELETEPERMS_S   _(SFQ" - forcing a job deletion requires manager privileges\n")
#define MSG_JOB_DELETEPERMS_SU        _("%s - you do not have the necessary privileges to delete the job \"" U32CFormat "\"\n")
#define MSG_JOB_DELETETASK_SUU        _("%s has deleted job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_DELETETASKS_SSU       _("%s has deleted job-array tasks %s of job "U32CFormat"\n")
#define MSG_JOB_DELETEJOB_SU          _("%s has deleted job "U32CFormat"\n")
#define MSG_JOB_DISCONTINUEDTRANS_SU  _("Discontinued delete transaction of user "SFQ" after job "U32CFormat"\n")
#define MSG_JOB_UNABLE2FINDQOFJOB_S   _("can't locate the queue "SFQ" associated with this job\n")
#define MSG_MAIL_TASKKILLEDSUBJ_UUS   "Job-array task "U32CFormat"."U32CFormat" ("SFN") Killed"
#define MSG_MAIL_TASKKILLEDBODY_UUSSS _("Job-array task "U32CFormat"."U32CFormat" ("SFN") was killed by %s@%s")
#define MSG_MAIL_JOBKILLEDBODY_USSS   _("Job " U32CFormat " ("SFN")  was killed by %s@%s")
#define MSG_MAIL_BECAUSE              _("\nbecause ")
#define MSG_JOB_FORCEDDELTASK_SUU     _("warning: %s forced the deletion of job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_FORCEDDELJOB_SU       _("warning: %s forced the deletion of job "U32CFormat"\n")
#define MSG_COM_NOSYNCEXECD_SU        _("%s unable to sync state with remote execd for the deletion of job \"" U32CFormat "\"\n")
#define MSG_JOB_REGDELTASK_SUU        _("%s has registered the job-array task "U32CFormat"."U32CFormat" for deletion\n")
#define MSG_JOB_REGDELJOB_SU          _("%s has registered the job "U32CFormat" for deletion\n")
#define MSG_JOB_CHANGEATTR            _("change job attributes")
#define MSG_JOB_NOALTERNOWRITE_U      _("alternation of job "U32CFormat" was rejected cause it couldn't be written\n")
#define MSG_JOB_CHANGESHAREFUNC       _("change share of job functional tickets")
#define MSG_JOB_SETSHAREFUNC_SSUUU    _("%s@%s sets job functional ticket share of job "U32CFormat"."U32CFormat" to "U32CFormat"\n")
#define MSG_JOB_RMHOLDMNG             _("remove manager hold")
#define MSG_JOB_SETHOLDMNG            _("set manager hold")
#define MSG_JOB_RMHOLDOP              _("remove operator hold")
#define MSG_JOB_SETHOLDOP             _("set operator hold")
#define MSG_JOB_RMHOLDUSER            _("remove user hold")
#define MSG_JOB_SETHOLDUSER           _("set user hold")
#define MSG_JOB_HOLD                  _("hold")
#define MSG_JOB_NOJOBARRAY_U          _(U32CFormat " is no job array\n")
#define MSG_JOB_CHANGEOVERRIDETICKS   _("change amount of override tickets")
#define MSG_JOB_SETOVERRIDETICKS_SSUU _("%s@%s sets override tickets of job "U32CFormat" to "U32CFormat"\n")
#define MSG_JOB_PRIOINC               _("increase job priority")
#define MSG_JOB_PRIOSET_SSUU          _("%s@%s sets scheduling priority of job "U32CFormat" to "U32CFormat"\n")
#define MSG_JOB_ACCOUNT               _("account")
#define MSG_JOB_WD                    _("working directory")
#define MSG_JOB_STARTTIME             _("start time")
#define MSG_JOB_STDERRPATHLIST        _("stderr path list")
#define MSG_JOB_STDOUTPATHLIST        _("stdout path list")
#define MSG_JOB_HOLDLISTMOD_USS       _("modified job id hold list of job "U32CFormat"\n   blocking jobs: "SFN"\n   exited jobs:   "SFN"\n")
#define MSG_JOB_MERGEOUTPUT           _("output merge behaviour")
#define MSG_JOB_HARDRESOURCELIST      _("hard resource list")
#define MSG_JOB_SOFTRESOURCELIST      _("soft resource list")
#define MSG_JOB_MAILOPTIONS           _("mail options")
#define MSG_JOB_MAILLIST              _("mail list")
#define MSG_JOB_JOBNAME               _("job name")
#define MSG_JOB_NOTIFYBEHAVIOUR       _("notify behaviour")
#define MSG_JOB_SLOTRANGE             _("slot range")
#define MSG_JOB_QNOTREQUESTABLE2      _("denied: queues are not configured to be requestable\n")
#define MSG_JOB_HARDQLIST             _("hard queue list")
#define MSG_JOB_SOFTQLIST             _("soft queue list")
#define MSG_JOB_MASTERHARDQLIST       _("master hard queue list")
#define MSG_JOB_RESTARTBEHAVIOR       _("restart behaviour")
#define MSG_JOB_SHELLLIST             _("shell list")
#define MSG_JOB_ENVLIST               _("environment")
#define MSG_JOB_QSARGS                _("qs args")
#define MSG_JOB_SCRIPTARGS            _("script arguments")
#define MSG_JOB_CONTEXT               _("context")
#define MSG_JOB_NODIRECTSLOTS         _("denied: use parallel environments instead of requesting slots explicitly\n")
#define MSG_JOB_NOSEQNRREAD_SS        _("can't read job sequence number in file "SFQ": %s")
#define MSG_JOB_NOSEQFILEOPEN_SS      _("can't open sequence number file "SFQ": for reading: %s -- guessing next job number")
#define MSG_JOB_NOSEQFILECREATE_SS    _("can't create job sequence number file "SFQ": %s - delaying until next job")
#define MSG_JOB_NOSUITABLEQ_S         _("%s: no suitable queues\n")
#define MSG_JOB_VERIFYERROR           _("error")
#define MSG_JOB_VERIFYWARN            _("warning")
#define MSG_JOB_VERIFYVERIFY          _("verification")
#define MSG_JOB_VERIFYFOUNDQ          _("verification: found suitable queue(s)\n")
#define MSG_JOB_VERIFYFOUNDSLOTS_I    _("verification: found possible assignment with %d slots\n")
#define MSG_OBJ_PE                    _("parallel environment")
#define MSG_OBJ_CKPT                  _("checkpointing environment")
#define MSG_JOB_MOD_MISSINGRUNNINGJOBCONSUMABLE_S     _("denied: former resource request on consumable "SFQ" of running job lacks in new resource request\n")
#define MSG_JOB_MOD_ADDEDRUNNINGJOBCONSUMABLE_S       _("denied: resource request on consumable "SFQ" of running job was not contained former resource request\n")
#define MSG_JOB_MOD_CHANGEDRUNNINGJOBCONSUMABLE_S     _("denied: can't change consumable resource request "SFQ" of running job\n")
#define MSG_JOB_MOD_GOTOWNJOBIDINHOLDJIDOPTION_U      _("denied: job \""U32CFormat"\" may not be it's own jobnet predecessor\n")
#define MSG_JOB_MOD_GOTOWNJOBIDINHOLDJIDOPTION_S      _("denied: job "SFQ" may not be it's own jobnet predecessor\n")
#define MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU      _("denied: non-ambiguous jobnet predecessor "SFQ" (found jobs "U32CFormat" and "U32CFormat")\n")
#define MSG_JOB_MOD_NOJOBNAME_SS                      _("denied: "SFQ" is not a valid job name for %s\n")
#define MSG_JOB_MOD_JOBNAMEVIOLATESJOBNET_SSUU        _("denied: using job name "SFQ" for %s violates reference unambiguousness in jobnet from job "U32CFormat" to "U32CFormat"\n")
#define MSG_JOB_MOD_CHGJOBNAMEDESTROYSREF_UU          _("denied: changing job name of job "U32CFormat" destroys jobnet predecessor reference from job "U32CFormat"\n")
#define MSG_SGETEXT_NEEDONEELEMENT_SS                 _("denied: request format error: need at least one element in sublist "SFQ" in "SFN"()\n")
#define MSG_SGETEXT_CANT_MOD_RUNNING_JOBS_U           _("job "U32CFormat" can't modify running jobs\n") 
#define MSG_SGETEXT_MUST_BE_OPR_TO_SS                 _("denied: "SFQ" must be operator to "SFN"\n")
#define MSG_SGETEXT_MOD_JOBS_SU                       _("modified "SFN" of job "U32CFormat"\n")     
#define MSG_SGETEXT_DOESNOTEXIST_SU                   _(""SFN" \"" U32CFormat "\" does not exist\n")
#define MSG_SGETEXT_DOESNOTEXISTTASK_UU               _("job \"" U32CFormat "\" task \"" U32CFormat "\" does not exist\n")
#define MSG_SGETEXT_DOESNOTEXISTTASKRANGE_UUUU        _("job \"" U32CFormat "\" task id range \"" U32CFormat "-" U32CFormat ":" U32CFormat "\" comprises no tasks\n")
#define MSG_SGETEXT_NO_PROJECT                        _("job rejected: no project assigned to job\n")     
#define MSG_SGETEXT_MOD_JATASK_SUU                    _("modified "SFN" of job-array task "U32CFormat"."U32CFormat"\n")  
#define MSG_SGETEXT_MUST_BE_MGR_TO_SS                 _("denied: "SFQ" must be manager to "SFN"\n")
#define MSG_SGETEXT_MUST_BE_JOB_OWN_TO_SUS            _("denied: "SFQ" must be at least owner of job "U32CFormat" to "SFN"\n")
#define MSG_SGETEXT_NOJOBSDELETED                     _("No jobs deleted\n")
#define MSG_SGETEXT_NOJOBSMODIFIED                    _("No jobs modified\n")
#define MSG_SGETEXT_THEREARENOJOBS                    _("There are no jobs registered\n")
#define MSG_SGETEXT_THEREARENOJOBSFORUSERS_S          _("There are no jobs registered for following users: "SFN"\n")
#define MSG_SGETEXT_SPECIFYUSERORJID                  _("You have to specify a username or job ids\n")
#define MSG_SGETEXT_SPECIFYONEORALLUSER               _("The switch for \"all users\" and a specified \"user list\" are not allowed together\n")
#define MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS             _("job rejected: no access to project "SFQ" for user "SFQ"\n") 
#define MSG_SGETEXT_NOTALLOWEDTOSPECUSERANDJID        _("it is not allowed to select users and job ids together\n")
#define MSG_SGETEXT_MODIFIEDINLIST_SSUS               _(""SFN"@"SFN" modified \"" U32CFormat "\" in "SFN" list\n")


#define MSG_SGETEXT_SPECIFYUSERORJID            _("You have to specify a username or job ids\n")
#define MSG_SGETEXT_SPECIFYONEORALLUSER         _("The switch for \"all users\" and a specified \"user list\" are not allowed together\n")
#define MSG_SGETEXT_THEREARENOJOBS              _("There are no jobs registered\n")
#define MSG_SGETEXT_NOJOBSDELETED               _("No jobs deleted\n")
#define MSG_SGETEXT_NOJOBSMODIFIED              _("No jobs modified\n")

/*
** sge_m_event.c
*/
#define MSG_EVE_UNKNOWNDUMMYREQUEST   _("unknown dummy request\n")
#define MSG_EVE_REINITEVENTCLIENT_S   _("reinitialization of "SFQ"\n")
#define MSG_EVE_UNKNOWNEVCLIENT_U     _("no event client known with id "U32CFormat"\n")
#define MSG_EVE_CLIENTREREGISTERED_S  _("event client "SFQ" reregistered - it will need a total update\n")
#define MSG_EVE_REG_SU                _(SFQ" registers as event client with id "U32CFormat"\n")
#define MSG_EVE_UNREG_SU              _("event client "SFQ" with id "U32CFormat" deregistered")
#define MSG_EVE_EVENTCLIENT           _("event client")

#define MSG_EVE_ILLEGALEVENTCLIENTID_S _("illegal event client id "SFQ"\n")
#define MSG_EVE_ILLEGALIDREGISTERED_U _("illegal event client id "U32CFormat" for registration\n")

#define MSG_EVE_INVALIDSUBSCRIPTION   _("invalid subscription information\n")
#define MSG_EVE_INVALIDINTERVAL_U     _("invalid event interval "U32CFormat"\n")

#define MSG_EVE_TOTALUPDATENOTHANDLINGEVENT_I _("event number %d is not handled by sge_total_update_event\n")

#define MSG_COM_ACKTIMEOUT4EV_ISIS    _("acknowledge timeout after %d seconds for event client (%s:%d) on host \"%s\"")
#define MSG_COM_NOSHUTDOWNPERMS       _("shutdown requires manager privileges\n")
#define MSG_COM_NOSCHEDDREGMASTER     _("no scheduler registered at qmaster\n")
#define MSG_COM_NOSCHEDMONPERMS       _("starting scheduler monitoring requires manager privileges\n")
#define MSG_COM_SCHEDMON_SS           _("%s@%s triggers scheduler monitoring\n")
/*
** sge_manop.c
*/
#define MSG_OBJ_MANAGER               _("manager")
#define MSG_OBJ_OPERATOR              _("operator")
#define MSG_SGETEXT_MAY_NOT_REMOVE_USER_FROM_LIST_SS  _("may not remove user "SFQ" from "SFN" list\n")

/*
** sge_pe_qmaster.c
*/
#define MSG_PE_SLOTSTOOLOW_I          _("new number of slots may not be less than %d that are in use\n")
#define MSG_OBJ_USERLIST              _("user list")
#define MSG_OBJ_XUSERLIST             _("xuser list")
#define MSG_PE_ALLOCRULE_SS           _("parameter allocation_rule of pe "SFQ": %s\n")
#define MSG_PE_DEBITSLOTS_IS          _("debiting %d slots on pe %s")
#define MSG_PE_USEDSLOTSBELOWZERO_S   _("PE_used_slots of pe %s sunk under 0!\n")
#define MSG_PE_REVERSESLOTS_IS        _("reversing %d slots on pe %s")
#define MSG_PE_USEDSLOTSTOOBIG_S      _("PE_used_slots of pe %s is greater than PE_slots!\n")  
#define MSG_PE_STARTPROCARGS_SS       _("parameter start_proc_args of pe "SFQ": %s\n")
#define MSG_PE_STOPPROCARGS_SS        _("parameter stop_proc_args of pe "SFQ": %s\n")

/*
** sge_ckpt_qmaster.c
*/ 
#define MSG_CKPT_INVALIDWHENATTRIBUTE_S _("Invalid \"when\" attribute for ckpt "SFQ"\n")
#define MSG_CKPT_XISNOTASIGNALSTRING_S  _("\"%s\" is not a signal string (like HUP, INT, " "WINCH, ..)\n")

/*
** sge_qmod_qmaster.c
*/
#define MSG_QUEUE_INVALIDQ_S          _("invalid queue "SFQ"\n")
#define MSG_QUEUE_INVALIDQORJOB_S     _("invalid queue or job "SFQ"\n")
#define MSG_QUEUE_NOCHANGEQPERMS_SS   _("%s - you have no permission to modify queue "SFQ"\n")
#define MSG_LOG_UNKNOWNQMODCMD_U      _("unknown command type\n" U32CFormat)
#define MSG_JOB_NOMODJOBPERMS_SU      _("%s - you have no permission to modify job \"" U32CFormat "\"\n")
#define MSG_JOB_CLEARERRORTASK_SSUU   _("%s@%s cleared error state of job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_CLEARERRORJOB_SSU     _("%s@%s cleared error state of job "U32CFormat"\n")
#define MSG_JOB_NOERRORSTATETASK_UU   _("Job-array task "U32CFormat"."U32CFormat" is not in error state\n")
#define MSG_JOB_NOERRORSTATEJOB_UU    _("Job "U32CFormat" is not in error state\n")
#define MSG_QUEUE_NOCLEARERRORPERMS_SS   _("%s - you have no permission to clear error state of queue "SFQ"\n")   
#define MSG_QUEUE_NOERRORSTATEQ_SS    _("%s - queue "SFQ" is not in error state\n")
#define MSG_QUEUE_NOTMODIFIEDSPOOL_S  _("Queue state of "SFQ" not modified: impossible to write spoolfile\n")
#define MSG_QUEUE_CLEARERRORSTATE_SSS _("The error state of queue "SFQ" has been cleared by %s@%s\n")
#define MSG_QUEUE_NOENABLEQPERMS_SS   _("%s - you have no permission to enable queue "SFQ"\n")
#define MSG_QUEUE_ALREADYENABLED_SS   _("%s - queue "SFQ" is already enabled\n")
#define MSG_QUEUE_ENABLEQ_SSS         _("Queue "SFQ" has been enabled by %s@%s\n")
#define MSG_QUEUE_NODISABLEQPERMS_SS  _("%s - you have no permission to disable queue "SFQ"\n")
#define MSG_QUEUE_NORESCHEDULEQPERMS_SS  _("%s - you have no permission to reschedule jobs of queue "SFQ"\n")
#define MSG_QUEUE_ALREADYDISABLED_SS  _("%s - queue "SFQ" is already disabled\n")
#define MSG_QUEUE_DISABLEQ_SSS        _("Queue "SFQ" has been disabled by %s@%s\n")
#define MSG_QUEUE_RESCHEDULEQ_SSS     _("Jobs in queue "SFQ" have been rescheduled by %s@%s\n")
#define MSG_QUEUE_NOFORCESUSPENDQ_SS  _("%s - can't force suspension of queue "SFQ"\n")
#define MSG_QUEUE_FORCESUSPENDQ_SS    _("%s - forced suspension of queue "SFQ"\n")
#define MSG_QUEUE_ALREADYSUSPENDED_SS _("%s - queue "SFQ" is already suspended\n")
#define MSG_QUEUE_NOSUSPENDQ_SS       _("%s - can't suspend queue "SFQ"\n")
#define MSG_QUEUE_SUSPENDQ_SSS        _("Queue "SFQ" was suspended by %s@%s\n")
#define MSG_QUEUE_NOFORCEENABLEQ_SS   _("%s - can't force enabling of queue "SFQ"\n")
#define MSG_QUEUE_FORCEENABLEQ_SSS     _("%s@%s - forced enabling of queue "SFQ"\n")
#define MSG_QUEUE_NOUNSUSP4SOS_SS     _("%s - can't unsuspend queue "SFQ" - queue is suspended because of suspend on subordinate\n")
#define MSG_QUEUE_NOUNSUSP4SOC_SS     _("%s - can't unsuspend queue "SFQ" - queue is suspended on calendar\n")
#define MSG_QUEUE_ALREADYUNSUSP_SS    _("%s - queue "SFQ" is already unsuspended\n")
#define MSG_QUEUE_NOUNSUSPQ_SS        _("%s - can't unsuspend queue "SFQ"\n")
#define MSG_QUEUE_UNSUSPENDQ_SSS      _("%s@%s unsuspended queue "SFQ"\n")
#define MSG_QUEUE_NOCLEANQPERMS       _("cleaning a queue requires manager privileges\n")
#define MSG_QUEUE_CLEANQ_SSS          _("%s@%s cleaned queue "SFQ"\n")
#define MSG_JOB_NOFORCESUSPENDTASK_SUU     _("%s - can't force suspension job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_NOFORCESUSPENDJOB_SU       _("%s - can't force suspension job "U32CFormat"\n")
#define MSG_JOB_FORCESUSPENDTASK_SUU  _("%s - forced suspension of job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_FORCESUSPENDJOB_SU    _("%s - forced suspension of job "U32CFormat"\n")
#define MSG_JOB_ALREADYSUSPENDED_SUU  _("%s - job-array task "U32CFormat"."U32CFormat" is already suspended\n")
#define MSG_JOB_ALREADYSUSPENDED_SU   _("%s - job "U32CFormat" is already suspended\n")
#define MSG_JOB_ALREADYUNSUSPENDED_SUU  _("%s - job-array task "U32CFormat"."U32CFormat" is already unsuspended\n")
#define MSG_JOB_ALREADYUNSUSPENDED_SU _("%s - job "U32CFormat" is already unsuspended\n")
#define MSG_JOB_NOSUSPENDTASK_SUU     _("%s - can't suspend job-array task "U32CFormat"."U32CFormat" \n")
#define MSG_JOB_NOSUSPENDJOB_SU       _("%s - can't suspend job "U32CFormat" \n")
#define MSG_JOB_NOUNSUSPENDTASK_SUU   _("%s - can't unsuspend job-array task "U32CFormat"."U32CFormat" \n")
#define MSG_JOB_NOUNSUSPENDJOB_SU     _("%s - can't unsuspend job "U32CFormat" \n")
#define MSG_JOB_SUSPENDTASK_SUU       _("%s - suspended job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_SUSPENDJOB_SU         _("%s - suspended job " U32CFormat "\n")
#define MSG_JOB_RESCHEDULEJOB_SU      _("%s - reschedule job " U32CFormat "\n")
#define MSG_JOB_UNSUSPENDTASK_SUU     _("%s - unsuspended job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_UNSUSPENDJOB_SU       _("%s - unsuspended job " U32CFormat "\n")
#define MSG_JOB_RMADMSUSPENDTASK_SSUU _("%s@%s removed administrator suspension of job-array task "U32CFormat"."U32CFormat" (suspend threshold is still active)\n")
#define MSG_JOB_RMADMSUSPENDJOB_SSU   _("%s@%s removed administrator suspension of job "U32CFormat" (suspend threshold is still active)\n")
#define MSG_JOB_NOADMSUSPENDTASK_SUU  _("%s - job-array task "U32CFormat"."U32CFormat" is not suspended by administrator - modify suspend threshold list of queue to remove suspend state\n")
#define MSG_JOB_NOADMSUSPENDJOB_SU    _("%s - job "U32CFormat" is not suspended by administrator - modify suspend threshold list of queue to remove suspend state\n")
#define MSG_JOB_NOFORCEENABLETASK_SUU _("%s - can't force enabling of job-array task "U32CFormat"."U32CFormat "\"\n")
#define MSG_JOB_NOFORCEENABLEJOB_SU   _("%s - can't force enabling of job "U32CFormat"\n")
#define MSG_JOB_FORCEENABLETASK_SUU   _("%s - forced enabling of job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_FORCEENABLEJOB_SU     _("%s - forced enabling of job "U32CFormat"\n")
#define MSG_JOB_FORCEUNSUSPTASK_SSUU  _("%s@%s forced unsuspension of job-array task "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_FORCEUNSUSPJOB_SSU    _("%s@%s forced unsuspension of job "U32CFormat"\n")
#define MSG_EVE_RESENTSIGNALTASK_UU   _("got resend signal timer event for unknown array task "U32CFormat"."U32CFormat"\n") 
#define MSG_EVE_RESENTSIGNALQ_S       _("got resend signal timer event for unknown queue %s\n")
#define MSG_COM_NOUPDATEQSTATE_IS     _("can't update remote queue state (%d) on queue "SFQ)
#define MSG_QUEUE_ENABLEQCAL_S        _("enable queue "SFQ" on calendar\n")
#define MSG_QUEUE_SOSNOUNSUSPCAL_S    _("suspension on subordinate prevents unsuspension according to calendar for queue "SFQ"\n")
#define MSG_QUEUE_AMDSNOUNSUSPCAL_S   _("administrator suspension prevents unsuspension according to calendar for queue "SFQ"\n")
#define MSG_QUEUE_SUSPENDQCAL_S       _("suspend queue "SFQ" on calendar\n")
#define MSG_QUEUE_DISABLEQCAL_S       _("disable queue "SFQ" on calendar\n")
#define MSG_QUEUE_UNSUSPENDQCAL_S     _("unsuspend queue "SFQ" on calendar\n")
#define MSG_QUEUE_NOSUSP4SOS_S        _("no need to suspend queue "SFQ" it's already suspended on subordinate\n")
#define MSG_QUEUE_NOSUSP4ADMS_S       _("no need to suspend queue "SFQ" it's already suspended by administrator\n")
#define MSG_QUEUE_ADDENABLED_S        _("adding queue "SFQ" it gets enabled according to initial_state\n")
#define MSG_QUEUE_ADDDISABLED_S       _("adding queue "SFQ" it gets disabled according to initial_state\n")
#define MSG_QUEUE_EXECDRESTARTENABLEQ_SS    _("execd restart at "SFQ" enables queue "SFQ" according to initial_state\n")
#define MSG_QUEUE_EXECDRESTARTDISABLEQ_SS   _("execd restart at "SFQ" disables queue "SFQ" according to initial_state\n")
/*
** sge_queue_qmaster.c
*/
#define MSG_QUEUE_PRIORITYRANGE                 _("priority not in range -20 to +20\n")
#define MSG_QUEUE_CANTLOCATEQUEUEX_S            _("can't locate queue %s\n")
#define MSG_QUEUE_DELQUEUETEMPLATE              _("deleting queue \"template\"")
#define MSG_QUEUE_NULLPTRPASSEDTOSGE_DEL_QUEUE  _("NULL ptr passed to sge_del_queue()\n")
#define MSG_CALENDAR_CALENDARXREFERENCEDINQUEUEYNOTEXISTS_SS _("calendar "SFQ" referenced in queue "SFQ" does not exist\n")
#define MSG_QUEUE_NULLPTR             _("NULL ptr passed to sge_add_queue()\n")
#define MSG_OBJ_QUEUE                 _("queue")
#define MSG_UNABLETODELQUEUEXREFERENCEDINCHKPTY_SS _("unable to delete queue "SFQ", referenced in checkpoint definition "SFQ"\n")
#define MSG_UNABLETODELQUEUEXREFERENCEDINPEY_SS _("unable to delete queue "SFQ", referenced in parallel environment definition "SFQ"\n")
#define MSG_SGETEXT_NOTPOSSIBLETOMODHOSTNAME          _("it is not possible to modify the hostname attribute\n")
#define MSG_SGETEXT_TOOFEWQSIQLIC_II                  _("not enough qsi queue licenses; licensed=%d - requested=%d\n")  
#define MSG_SGETEXT_OPNOTALLOWED_S              _("operation not allowed: "SFN"\n")
#define MSG_NOTALLOWEDTODELSUBORDINATE_SS              _("deleting queue "SFQ" is not allowed because it is subordinate queue of "SFQ"\n")
#define MSG_SGETEXT_ACTIVEUNITS_SSIS            _(""SFN" "SFQ" has %d active "SFN"\n")
#define MSG_SGETEXT_COMPLEXNOTUSERDEFINED_SSS         _("denied: complex "SFQ" referenced in "SFN" "SFQ" is not a user complex\n")  
#define MSG_SGETEXT_UNKNOWNQUEUE_SSSS                 _("denied: queue "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n")
#define MSG_SGETEXT_QUEUEALLANDQUEUEARENOT_SSSS        _("queuenames and keyword "SFQ" are not allowed in "SFQ" of "SFQ" "SFQ"\n")
#define MSG_SGETEXT_CANTCOUNT_QSIQ_S                  _("can't count qsi queues in "SFN"\n") 
#define MSG_SGETEXT_UNKNOWNCOMPLEX_SSS                _("denied: complex "SFQ" referenced in "SFN" "SFQ" does not exist\n")  
#define MSG_AT_LEASTONEQTYPE                          _("At least one queue type must be selected\n")

/*
** qmaster.c
*/
#define MSG_SGETEXT_KILL_SSS                    _(""SFN"@"SFN" kills "SFN"\n")
#define MSG_SETTING_PRIORITY_TAGS_S             _("setting SGE_PRIORITY_TAGS to %s\n")
#define MSG_TOO_MANY_PRIORITY_TAGS_S            _("SGE_PRIORITY_TAGS %s contains too many tags (max 9)\n")


/*
** sge_sharetree_qmaster.c
*/
#define MSG_STREE_ADDSTREE_SSII       _("%s@%s has added sharetree with %d nodes and %d leafs\n")
#define MSG_STREE_MODSTREE_SSII       _("%s@%s has modified sharetree with %d nodes and %d leafs\n")
#define MSG_STREE_PRJINPTJSUBTREE_SS  _("found project "SFQ" in project "SFQ" sub-tree of share tree\n")
#define MSG_STREE_PRJTWICE_S          _("found project "SFQ" twice in share tree\n")
#define MSG_STREE_USERNONLEAF_S       _("found user "SFQ" in share tree as a non-leaf node\n")
#define MSG_STREE_USERTWICEINPRJSUBTREE_SS _("found user "SFQ" twice in project "SFQ" sub-tree of share tree\n")
#define MSG_STREE_USERTNOACCESS2PRJ_SS   _("found user "SFQ" with no access to project "SFQ" sub-tree of share tree\n")
#define MSG_STREE_USERPRJTWICE_SS     _("found %s "SFQ" twice in share tree\n")
#define MSG_STREE_QMASTERSORCETREE_SS _("tree of qmaster has %snode while source tree has %snode\n")
#define MSG_STREE_NOPLUSSPACE         _("no ")
#define MSG_STREE_VERSIONMISMATCH_II  _("trees have different versions: qmaster=%d src=%d\n") 
#define MSG_STREE_MISSINGNODE_S       _("missing node "SFQ" in dst trees\n")
#define MSG_SGETEXT_DOESNOTEXIST_S              _(""SFN" does not exist\n")
#define MSG_SGETEXT_REMOVEDLIST_SSS         _(""SFN"@"SFN" removed "SFN" list\n")
#define MSG_SGETEXT_FOUND_UP_TWICE_SS                 _("denied: found node "SFQ" twice under node "SFQ"\n")     
#define MSG_SGETEXT_UNKNOWN_SHARE_TREE_REF_TO_SS      _("denied: share tree contains reference to unknown "SFN" "SFQ"\n")  


/*
** sge_userprj_qmaster.c
*/
#define MSG_USERPRJ_PRJXSTILLREFERENCEDINENTRYX_SS _("project "SFQ" is still referenced by user "SFQ"\n")
#define MSG_UP_NOADDDEFAULT_S          _("denied: not allowed add a %s with name \"default\"\n") 
#define MSG_UP_ALREADYEXISTS_SS        _("denied: shared namespace between project and user: there is already a "SFN" which is named "SFQ"\n")
#define MSG_UM_CLUSTERUSERXNOTGUILTY_S _("cluster user name '%s' is not guilty\n")
#define MSG_HGRP_GROUPXNOTGUILTY_S     _("host group name '%s' is not guilty\n")
#define MSG_HGRP_CANTADDMEMBERXTOGROUPY_SS      _("can't add member '%s' to host group '%s'\n")
#define MSG_HGRP_CANTNOTADDSUBGROUPXTOGROUPY_SS _("can't add subgroup '%s' to host group '%s': subgroup not guilty\n")
#define MSG_HGRP_INCONSISTENTHOSTGROUPENTRYX_S  _("inconsistent host group entry '%s'\n")
#define MSG_UM_MAPLISTFORXEXISTS_S     _("mapping list for '%s' allready exist\n")
#define MSG_HGRP_MEMBERLISTFORXEXISTS_S _("member list for '%s' allready exist\n")
#define MSG_UM_NOMAPLISTFORXFOUND_S    _("no mapping list for '%s' found\n")
#define MSG_HGRP_NOMEMBERLISTFORXFOUND_S _("no member list for '%s' found\n")
#define MSG_GRP_NOSUBGROUPLISTFORXFOUND_S _("no subgroup list for '%s' found\n")
#define MSG_UM_EXIMINEMAPFORX_S        _("examine mapping for cluster user '%s'\n")
#define MSG_UM_USERMAPPINGDISABLED     _("user mapping is disabled\n")
#define MSG_GRP_HOSTGROUPDISABLED      _("host groups are disabled\n")
#define MSG_UM_ERRORADDMAPENTRYXFORY_SS _("error adding mapping entry '%s' for cluster user '%s'")
#define MSG_UM_ERRORWRITESPOOLFORUSER_S _("error writing spoolfile for cluster user '%s'")
#define MSG_HGRP_ERRORWRITESPOOLFORGROUP_S  _("error writing spoolfile for host group '%s'")
#define MSG_OBJ_PRJ                    _("project")
#define MSG_OBJ_PRJS                  _("projects")
#define MSG_OBJ_XPRJS                 _("xprojects")
#define MSG_OBJ_EH                    _("exechost")
#define MSG_OBJ_CONF                  _("configuration")
#define MSG_OBJ_GLOBAL                _("global")
#define MSG_FILE_RM_S                 _("cant remove %s from disk")
#define MSG_JOB_CREDMOD_SSF           _("%s@%s modified credit to %.2f\n")
#define MSG_JOB_CREDMODLOW_SSF        _("%s@%s modified low_credit to %.2f\n")
#define MSG_SGETEXT_CANT_DELETE_UP_IN_SHARE_TREE_SS   _("denied: may not remove "SFN" "SFQ" still referenced in share tree\n") 
#define MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS       _("denied: project "SFQ" is still referenced in "SFN" of "SFN" "SFQ"\n")   
#define MSG_SGETEXT_UNKNOWNPROJECT_SSSS               _("denied: project "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n")  


/*
** sge_userset_qmaster.c
*/
#define MSG_US_INVALIDUSERNAME        _("userset contains invalid (null) user name")
#define MSG_SGETEXT_NO_DEPARTMENT4USER_SS             _("denied: no matching department for user "SFQ" or group "SFQ"\n")  
#define MSG_SGETEXT_UNKNOWNUSERSET_SSSS               _("denied: userset "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n") 
#define MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS       _("denied: userset "SFQ" is still referenced in "SFN" of "SFN" "SFQ"\n") 


/*
** complex_qmaster.c
*/
#define MSG_CPLX_ATTRIBISNULL_SS      _("attribute name "SFQ" in complex "SFQ" contains invalid (null) values\n")
#define MSG_CPLX_ATTRIBALREADY_SS     _("attribute name "SFQ" is already in use in complex "SFQ"\n")
#define MSG_CPLX_SHORTCUTALREADY_SS   _("shortcut name "SFQ" is already in use in complex "SFQ"\n")
#define MSG_CPLX_ATTRIBNOCONSUM_S     _("denied: attribute "SFQ" is a string type and thus may not be consumable\n")
#define MSG_CPLX_ATTRIBNOCONSUMEH_SS  _("denied: attribute "SFQ" in 'complex_list' of exec host "SFQ" has a string value and thus it may not be consumable\n")
#define MSG_CPLX_ATTRIBNOCONSUMQ_SS   _("denied: attribute "SFQ" in 'complex_list' of queue "SFQ" has a string value and thus it may not be consumable\n")
#define MSG_FILE_NOOPENDIR_S          _("can't open directory "SFQ)
#define MSG_FILE_NOWRITEHIST_S        _("can't write history for complex "SFQ)
#define MSG_NONAME                    _("<noname>")
#define MSG_OBJ_CPLX                  _("complex")
#define MSG_ATTRSTILLREFINQUEUE_SS    _("attribute "SFQ" still referenced in queue "SFQ" - complex not modified")
#define MSG_ATTRSTILLREFINHOST_SS     _("attribute "SFQ" still referenced in host "SFQ" - complex not modified")
#define MSG_ATTRSTILLREFINSCHED_S     _("attribute "SFQ" still referenced in scheduler configuration - complex not modified")
#define MSG_SGETEXT_CANTDELCMPLX_S              _("can't delete complex "SFQ" from list\n")
#define MSG_SGETEXT_CANTDELCMPLXDISK_S          _("can't delete complex "SFQ" from disk\n")
#define MSG_SGETEXT_COMPLEXSTILLREFERENCED_SSS        _("denied: complex "SFQ" is still referenced in complex_list of "SFN" "SFQ"\n")







/*
** configuration_qmaster.c
*/
#define MSG_SGETEXT_CANT_DEL_CONFIG_S           _("can't delete configuration "SFQ" from list\n")
#define MSG_CONF_DELLOCCONFFORXWITHEXECDSPOOLDENIED_S  _("Deleting local configuration for "SFQ" with a local execd_spool_dir setting only supported in a shut-down cluster.\n")
#define MSG_SGETEXT_CANT_DEL_CONFIG_DISK_SS     _("can't delete configuration "SFQ" from disk: %s\n")
#define MSG_CREATINGGLOBALCONF                    _("creating new global configuration with default values\n")
#define MSG_CONF_CANTSELECTCONFIGURATIONFORHOST_SI _("can't select configuration for host "SFQ": %d")
#define MSG_CONF_CANTMERGECONFIGURATIONFORHOST_SI _("can't merge configuration for host "SFQ": %d")
#define MSG_CONF_CHANGEPARAMETERXONLYSUPONSHUTDOWN_S _("Changing parameter "SFQ" only supported in a shut-down cluster.\n")
#define MSG_CONF_NAMEISNULLINCONFIGURATIONLISTOFX_S _("name == NULL in configuration list of "SFQ"\n")
#define MSG_CONF_VALUEISNULLFORATTRXINCONFIGURATIONLISTOFY_SS _("value == NULL for attribute "SFQ" in configuration list of "SFQ"\n")
#define MSG_CONF_GOTINVALIDVALUEXFORLOGLEVEL_S _("denied: got invalid value "SFQ" for loglevel\n")
#define MSG_CONF_GOTINVALIDVALUEXFORSHELLSTARTMODE_S _("denied: got invalid value "SFQ" for shell_start_mode\n")
#define MSG_CONF_GOTINVALIDVALUEXASADMINUSER_S _("denied: got invalid value "SFQ" as admin_user\n")
#define MSG_CONF_PARAMETERXINCONFIGURATION_SS _("denied: parameter "SFQ" in configuration: "SFQ"\n")
#define MSG_CONF_THEPATHGIVENFORXMUSTSTARTWITHANY_S _("denied: the path given for "SFQ" must start with an \"/\"\n")
#define MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS _("infinity not allowed for attribute "SFQ" in configuration list of "SFQ"\n")
#define MSG_CONF_FORMATERRORFORXINYCONFIG_SS _("format error for "SFQ" in "SFQ" configuration\n")


/*
** job_exit.c
*/
#define MSG_OBJ_UNKNOWNQ              _("<unknown queue>")
#define MSG_OBJ_UNKNOWNHOST           _("<unknown host>")
#define MSG_JOB_WRITEJFINISH_S        _("writing job finish information: can't locate queue \"%s\"\n")
#define MSG_JOB_JFINISH_UUS           _("job "U32CFormat"."U32CFormat" finished on host %s")
#define MSG_JOB_FAILEDONHOST_UUSSSS   _("job "U32CFormat"."U32CFormat" failed on host "SFN" "SFN SFN" because: %-.512s")
#define MSG_GENERAL                   _("general ")
#define MSG_JOB_JEXITNOTRUN_UU        _("received JOB_EXIT for job "U32CFormat"."U32CFormat" which ist NOT running\n")
#define MSG_LOG_JREMOVED              _("job removed ")
#define MSG_LOG_JERRORSET             _("job set in error state ")
#define MSG_LOG_JNOSTARTRESCHEDULE    _("job never ran -> schedule it again")
#define MSG_LOG_JRERUNRESCHEDULE      _("job rerun/checkpoint specified -> schedule it again")
#define MSG_LOG_JCKPTRESCHEDULE       _("job was checkpointed -> schedule it again")
#define MSG_LOG_JNORESRESCHEDULE      _("job didn't get resources -> schedule it again")
#define MSG_LOG_QERRORBYJOB_SU        _("queue %s marked QERROR as result of job "U32CFormat"'s failure\n") 
#define MSG_LOG_QERRORBYJOBHOST_SUS   _("queue %s marked QERROR as result of job "U32CFormat"'s failure at host %s\n")
#define MSG_FILE_WRITE_S              _("writing to %s\n")
#define MSG_FILE_WRITEACCT            _("can't write to acct_file\n")


/*
** job_report_qmaster.c
*/
#define MSG_JOB_REPORTEXITQ_SUUSSSSS   _("execd %s reports exiting job ("U32CFormat"."U32CFormat"/%s in queue "SFQ" that was supposed to be in queue "SFQ" at "SFQ" (state = %s)")
#define MSG_JOB_REPORTRUNQ_SUUSSU      _("execd %s reports running state for job ("U32CFormat"."U32CFormat"/%s) in queue "SFQ" while job is in state "U32CFormat" ")
#define MSG_JOB_REPORTRUNFALSE_SUUSS   _("execd@%s reports running job ("U32CFormat"."U32CFormat"/%s) in queue "SFQ" that was not supposed to be there - killing")
#define MSG_JOB_REPORTEXITJ_UUU        _("JEXITING report for job "U32CFormat"."U32CFormat": which is in status "U32CFormat"\n")
#define MSG_JOB_FILESIZEEXCEED_SSUU    _("file size resource limit exceeded by task %s at %s of job "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_CPULIMEXCEED_SSUU      _("cpu time resource limit exceeded by task %s at %s of job "U32CFormat"."U32CFormat"\n")
#define MSG_JOB_DIEDTHROUGHSIG_SSUUS   _("task %s at %s of job "U32CFormat"."U32CFormat" died through signal %s\n")
#define MSG_JOB_TASKFAILED_SSUUU       _("task %s at %s of job "U32CFormat"."U32CFormat" failed "U32CFormat"\n")
#define MSG_JOB_TASKFINISHED_SSUU      _("task %s at %s of job "U32CFormat"."U32CFormat" finished\n")
#define MSG_JOB_JOBTASKFAILED_SU       _("task %s of job "U32CFormat" failed - killing job\n")
#define MSG_OBJ_NOTRUNNING             _("<not running>")
#define MSG_EXECD_UNKNOWNJ_SUUSUS      _("execd %s reports unknown job ("U32CFormat"."U32CFormat"/%s) with unknown state "U32CFormat" in queue "SFQ)


/*
** qmaster.c
*/
#define MSG_STARTUP_PROGRAMCALLEDWITHINVALIDNAME_S        _("program called with invalid name: '%s'\n")
#define MSG_STARTUP_PROGRAMDOESNOTMATCHPRODUCTMODE_S      _("program name '%s' does not match product mode")
#define MSG_COMMD_CANTENROLLTOCOMMD_S              _("can't enroll to commd: %s")
#define MSG_COMMD_FOUNDRUNNINGCOMMDONLOCALHOST              _("found running commd on local host")
#define MSG_STARTUP_BEGINWITHSTARTUP              _("begin with start up")
#define MSG_STARTUP_SETUPFAILED              _("setup failed")
#define MSG_QIDL_CANTINITIALIZEQIDL              _("cannot initialze qidl")
#define MSG_QIDL_CANTSPAWNTHREADFORCORBASERVER              _("cannot spawn thread for corba server")
#define MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI           _("Failed in sge_unpack_gdi_request (%s/%s/%d)\n")
#define MSG_CULL_FAILEDINCULLUNPACKLISTREPORT              _("Failed in cull_unpack_list report\n")
#define MSG_SHUTDOWN_SHUTTINGDOWNQMASTERREQUIRESMANAGERPRIVILEGES              _("shutting down qmaster requires manager privileges\n")
#define MSG_GOTSTATUSREPORTOFUNKNOWNCOMMPROC_S    _("got load report of unknown commproc "SFQ"\n")
#define MSG_GOTSTATUSREPORTOFUNKNOWNEXECHOST_S    _("got load report of unknown exec host "SFQ"\n")
#define MSG_CONF_CANTNOTIFYEXECHOSTXOFNEWCONF_S    _("can't notify exec host "SFQ" of new conf\n")
#define MSG_LICENCE_ERRORXUPDATINGLICENSEDATA_I    _("error %d updating license data\n")
#define MSG_HEARTBEAT_FAILEDTOINCREMENTHEARBEATFILEXINSPOOLDIR_S    _("failed to increment heartbeat file "SFQ" in spool directory")
#define MSG_GDI_FAILEDTOEXTRACTAUTHINFO    _("failed to extract authentication information")

/*
** qmaster_to_execd.c
*/
#define MSG_NOXKNOWNONHOSTYTOSENDCONFNOTIFICATION_SS  _("no %s known on host %s to send conf notification\n")

/*
** read_write_manop.c
*/
#define MSG_FILE_ERROROPENINGX_S    _("error opening %s\n")

/*
** setup_qmaster.c
*/
#define MSG_SETUP_QUEUE_S                          _("\tQueue "SFQ".\n")
#define MSG_SETUP_USERSET_S                        _("\tUserset "SFQ".\n")
#define MSG_SETUP_USER_S                           _("\tUser "SFQ".\n")
#define MSG_SETUP_PROJECT_S                        _("\tProject "SFQ".\n")
#define MSG_SETUP_MAPPINGETRIES_S                  _("\tMapping entries for "SFQ".\n")
#define MSG_SETUP_HOSTGROUPENTRIES_S               _("\tHost group entries for group "SFQ".\n")
#define MSG_SETUP_PE_S                             _("\tPE "SFQ".\n")
#define MSG_SETUP_CALENDAR_S                       _("\tCalendar "SFQ".\n")
#define MSG_SETUP_COMPLEX_S                        _("\tComplex "SFQ".\n")
#define MSG_SETUP_CKPT_S                           _("\tCKPT "SFQ".\n")
#define MSG_SETUP_SETUPMAYBECALLEDONLYATSTARTUP    _("setup may be called only at startup")
#define MSG_CONFIG_FOUNDNOLOCALCONFIGFORQMASTERHOST_S    _("found no local configuration for qmaster host "SFQ)
#define MSG_CONFIG_ERRORXSELECTINGCONFIGY_IS    _("Error %d selecting configuration "SFQ"\n")
#define MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS    _("Error %d merging configuration "SFQ"\n")
#define MSG_CONFIG_ADDINGHOSTTEMPLATETOEXECHOSTLIST    _("adding host template to exechost_list\n")
#define MSG_CONFIG_ADDINGHOSTGLOBALTOEXECHOSTLIST    _("adding host global to exechost_list\n")
#define MSG_CONFIG_CANTWRITEMANAGERLIST       _("can't write manager list\n")
#define MSG_CONFIG_CANTWRITEOPERATORLIST      _("can't write operator list\n")
#define MSG_CONFIG_READINGINUSERSETS       _("Reading in usersets:\n")
#define MSG_CONFIG_READINGFILE_SS       _("reading file %s/%s\n")
#define MSG_CONFIG_READINGINQUEUES       _("Reading in queues:\n")
#define MSG_CONFIG_QUEUEXUPDATED_S       _("Queue %s updated\n")
#define MSG_CONFIG_OBSOLETEQUEUETEMPLATEFILEDELETED       _("obsolete queue template file deleted\n")
#define MSG_CONFIG_FOUNDQUEUETEMPLATEBUTNOTINFILETEMPLATEIGNORINGIT       _("found queue 'template', but not in file 'template'; ignoring it!\n")
#define MSG_CONFIG_CANTRECREATEQEUEUEXFROMDISKBECAUSEOFUNKNOWNHOSTY_SS       _("cannot recreate queue %s from disk because of unknown host %s\n")
#define MSG_CONFIG_CANTWRITEHISTORYFORQUEUEX_S       _("can't write history for queue "SFQ"\n")
#define MSG_CONFIG_READINGINSCHEDULERCONFIG       _("Reading in scheduler configuration\n")
#define MSG_CONFIG_READINGINUSERS       _("Reading in users:\n")
#define MSG_CONFIG_READINGINPROJECTS       _("Reading in projects:\n")
#define MSG_CONFIG_CANTLOADSHARETREEXSTARTINGUPWITHEMPTYSHARETREE_S       _("cant load sharetree (%s), starting up with empty sharetree")
#define MSG_CONFIG_READINGINEXECUTIONHOSTS       _("Reading in execution hosts.\n")
#define MSG_CONFIG_CANTRESOLVEEXECHOSTNAMEX_S       _("cannot resolve exechost name "SFQ)
#define MSG_CONFIG_CANTWRITEHISTORYFORHOSTX_S       _("cannot write history for host "SFQ"\n")
#define MSG_CONFIG_READINGINADMINHOSTS       _("Reading in administrative hosts.\n")
#define MSG_CONFIG_READINGINSUBMITHOSTS       _("Reading in submit hosts.\n")
#define MSG_CONFIG_READINGINGPARALLELENV       _("Reading in parallel environments:\n")
#define MSG_CONFIG_READINGUSERMAPPINGENTRY _("Reading in user mapping entries:\n")
#define MSG_CONFIG_READINGHOSTGROUPENTRYS  _("Reading in host group entries:\n")
#define MSG_CONFIG_READINGINCALENDARS       _("Reading in calendars:\n")
#define MSG_CONFIG_READINGINCOMPLEXES       _("Reading in complexes:\n")
#define MSG_CONFIG_FAILEDPARSINGYEARENTRYINCALENDAR_SS       _("failed parsing year entry in calendar "SFQ": %s\n")
#define MSG_UNKNOWNREASON       _("unknown reason")
#define MSG_CONFIG_READINGINCKPTINTERFACEDEFINITIONS       _("Reading in ckpt interface definitions:\n")
#define MSG_CONFIG_CANTFINDQUEUEXREFERENCEDINJOBY_SU       _("can't find queue "SFQ" referenced in job "U32CFormat)
#define MSG_CONFIG_FAILEDREMOVINGBADJOBFILEREASONXPLEASEDELETEYMANUALY_SS       _("failed removing bad jobfile (reason: %s): please delete "SFQ" manually\n")
#define MSG_CONFIG_REMOVEDBADJOBFILEX_S       _("removed bad jobfile "SFQ"\n")
#define MSG_CONFIG_FAILEDREMOVINGSCRIPT_SS       _("failed removing script of bad jobfile (reason: %s): please delete "SFQ" manually\n")
#define MSG_CONFIG_REMOVEDSCRIPTOFBADJOBFILEX_S       _("removed script of bad jobfile "SFQ"\n")
#define MSG_CONFIG_READINGINX_S       _("Reading in %s.\n")
#define MSG_CONFIG_CANTFINDSCRIPTFILE_U       _("can't find script file for job " U32CFormat " - deleting\n")
#define MSG_CONFIG_JOBFILEXHASWRONGFILENAMEDELETING_U       _("job file \""U32CFormat"\" has wrong file name - deleting\n")
#define MSG_CONFIG_NODIRECTORY_S                              _(SFQ" is no directory - skipping the entry\n")


/*
** subordinate_qmaster.c
*/
#define MSG_JOB_SOSUSINGGDILFORJOBXCANTFINDREFERENCEQUEUEY_US       _("sos_using_gdil for job "U32CFormat": can't find referenced queue "SFQ)
#define MSG_JOB_USOSUSINGGDILFORJOBXCANTFINDREFERENCEQUEUEY_US       _("usos_using_gdil for job "U32CFormat": can't find referenced queue "SFQ)
#define MSG_SGETEXT_SUBITSELF_S                 _("queue "SFQ" can't get subordinated by itself\n")
#define MSG_SGETEXT_UNKNOWNSUB_SS               _("subordinated queue "SFQ" referenced in queue "SFQ" does not exist\n")
#define MSG_SGETEXT_SUBTHRESHOLD_EXCEEDS_SLOTS_SUSU   _("queue "SFQ": threshold of "U32CFormat" for subordinated " "queue "SFQ" exceeds job slots of "U32CFormat"\n")
#define MSG_SGETEXT_SUBHOSTDIFF_SSS                   _("queue "SFQ": subordinated queue "SFQ" resides on other host "SFQ"\n")


/*
** time_event.c
*/
#define MSG_SYSTEM_SYSTEMHASBEENMODIFIEDXSECONDS_I       _("system time has been modified (%d seconds)\n")
#define MSG_SYSTEM_RECEIVEDUNKNOWNEVENT       _("received unkown event\n")


/* sge_user_mapping.c */
#define MSG_UMAP_ADDEDENTRY_S                      _("added mapping entry '%s'\n")
#define MSG_UMAP_EXAMINEMAPENTRY_S                 _("examine mapping entry '%s'\n")
#define MSG_UMAP_EXAMINEHOSTLISTFORMAPNAME_S       _("examine hostlist for user mapping name '%s'\n")
#define MSG_UMAP_CANTADDHOSTX_S                    _("can't add host or group '%s'\n")
#define MSG_UMAP_XADDED_S                          _("host or group '%s' added\n")
#define MSG_UMAP_REMOVEDMAPENTRYXFORCLUSERUSERY_SS      _("removed mapping entry '%s' for cluster user '%s'\n")
#define MSG_UMAP_REMOVEDXFROMMAPENTRYYFORCLUSERUSERZ_SSS      _("removed '%s' from mapping entry '%s' for cluster user '%s'\n")
#define MSG_UMAP_HOSTNAMEXNOTRESOLVEDY_SS          _("hostname '%s' not resolved: %s\n")

/*
** misc
*/
#define MSG_SEC_CRED_SSSI         _("denied: request for user "SFQ" does not match credentials for connection <"SFN","SFN",%d>\n")         

#endif /* __MSG_QMASTER_H */

