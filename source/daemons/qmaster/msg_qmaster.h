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

#define MSG_SGETEXT_CANTSPOOL_SS      _MESSAGE(33001, _("qmaster is unable to spool "SFN" "SFQ"\n"))

/*
** sge_ckptobj.c
*/
#define MSG_OBJ_CKPT                  _MESSAGE(33005, _("checkpointing environment"))

/*
** sge_follow.c
*/
#define MSG_JOB_NOJOBID               _MESSAGE(33014, _("can't get job id\n"))
#define MSG_JOB_NOORDERTASK_US        _MESSAGE(33015, _("invalid task number 0 for job "U32CFormat" in "SFQ" order\n"))
#define MSG_JOB_FINDJOB_U             _MESSAGE(33016, _("unable to find job "U32CFormat" from the scheduler order package\n"))
#define MSG_JOB_FINDJOBTASK_UU        _MESSAGE(33017, _("unable to find task "U32CFormat" of job "U32CFormat"\n"))

#define MSG_ORD_OLDVERSION_UUU        _MESSAGE(33018, _("scheduler sent order for old version "U32CFormat" of job "U32CFormat"."U32CFormat"\n"))
#define MSG_ORD_TWICE_UU              _MESSAGE(33019, _("scheduler tries to schedule job "U32CFormat"."U32CFormat" twice\n"))
#define MSG_ORD_INITIALTICKETS_U      _MESSAGE(33020, _(" with "U32CFormat" initial tickets"))

#define MSG_OBJ_UNABLE2FINDPE_S       _MESSAGE(33021, _("unable to find pe "SFQ"\n"))
#define MSG_OBJ_NOQNAME               _MESSAGE(33022, _("can't get q_name\n"))

#define MSG_ORD_QVERSION_UUS          _MESSAGE(33023, _("orders queue version ("U32CFormat") is not uptodate ("U32CFormat") for queue "SFQ"\n"))
#define MSG_ORD_USRPRJVERSION_UUS          _MESSAGE(33024, _("orders user/project version ("U32CFormat") is not uptodate ("U32CFormat") for user/project "SFQ"\n"))


#define MSG_JOB_JOBACCESSQ_US         _MESSAGE(33025, _("job "U32CFormat" has no access to queue "SFQ"\n"))
#define MSG_JOB_FREESLOTS_USUU        _MESSAGE(33026, _("not enough ("U32CFormat") free slots in queue "SFQ" for job "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_QMARKEDERROR_S        _MESSAGE(33027, _("queue "SFQ" is marked QERROR\n") ) 
#define MSG_JOB_QSUSPCAL_S            _MESSAGE(33028, _("while scheduling queue "SFQ" was suspended on calendar\n"))
#define MSG_JOB_QDISABLECAL_S         _MESSAGE(33029, _("while scheduling queue "SFQ" was disabled on calendar\n"))
#define MSG_JOB_UNABLE2FINDHOST_S     _MESSAGE(33030, _("unable to locate host in exechost list "SFQ"\n"))
#define MSG_JOB_UNABLE2STARTJOB_US    _MESSAGE(33031, _("unable to start job "U32CFormat" before cleanup on host "SFN" has finished\n"))
#define MSG_JOB_HOSTNAMERESOLVE_US    _MESSAGE(33032, _("failed starting job "U32CFormat" - probably hostname resolving problem with "SFQ"\n") ) 
#define MSG_JOB_JOBDELIVER_UU         _MESSAGE(33033, _("failed delivering job "U32CFormat"."U32CFormat))
#define MSG_JOB_UNABLE2FINDJOBORD_U   _MESSAGE(33034, _("unable to find job \""U32CFormat"\" in ticket order\n") )     
#define MSG_JOB_CHANGETICKETS_UU      _MESSAGE(33035, _("scheduler tries to change tickets of a non running job "U32CFormat" task "U32CFormat"\n"))
#define MSG_JOB_CHANGEPTICKETS_UU      _MESSAGE(33036, _("scheduler tries to change pending tickets of a non pending job "U32CFormat" task "U32CFormat"\n"))
#define MSG_JOB_REMOVENOTFINISHED_U   _MESSAGE(33037, _("scheduler tried to remove job "U32CFormat" which is not in state JFINISHED\n"))
#define MSG_JOB_REMOVENONINTERACT_U   _MESSAGE(33038, _("scheduler tried to remove non interactive job "U32CFormat" by use of a ORT_remove_immediate_job order\n"))
#define MSG_JOB_REMOVENONIMMEDIATE_U  _MESSAGE(33039, _("scheduler tried to remove non immediate job "U32CFormat" by use of a ORT_remove_immediate_job order\n"))
#define MSG_JOB_REMOVENOTIDLEIA_U     _MESSAGE(33040, _("scheduler tried to remove interactive job "U32CFormat" but it is not in JIDLE state\n"))
#define MSG_JOB_NOFREERESOURCEIA_UU   _MESSAGE(33041, _("no free resource for interactive job "U32CFormat"."U32CFormat" for user "SFQ"\n"))
#define MSG_MAIL_CREDITLOWSUBJ_SUS    SFN": Credit low for job " U32CFormat " ("SFN")\n"
/* #define MSG_MAIL_CREDITLOWBODY_USSFF  _message(33042, _("Your job " U32CFormat " ("SFN") attached to project "SFQ" has\nfallen below low credit limit.\nActual Credit: %.2f   Low Credit Limit: %.2f\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_SUSPOTNOTRUN_UU       _MESSAGE(33043, _("got ORT_suspend_on_threshold order for non running task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_UNSUSPOTNOTRUN_UU     _MESSAGE(33044, _("got ORT_unsuspend_on_threshold order for non running task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_UNSUSPOT_UUS          _MESSAGE(33045, _("unsuspending job "U32CFormat"."U32CFormat" according to suspend threshold of queue "SFQ"\n"))
#define MSG_JOB_UNABLE2FINDMQ_SU      _MESSAGE(33046, _("unable to find master queue "SFN" of job "U32CFormat"\n"))
#define MSG_JOB_SUSPTQ_UUS            _MESSAGE(33047, _("suspending job "U32CFormat"."U32CFormat" according to suspend threshold of queue "SFQ"\n"))
#define MSG_JOB_MISSINGJOBTASK_UU     _MESSAGE(33048, _("missing job "U32CFormat"."U32CFormat" appeared in ticket orders list"))
#define MSG_ORD_UNABLE2FINDHOST_S     _MESSAGE(33049, _("unable to lookup host for queue "SFN" in parallel job ticket order"))


/*
** parse_onoff.c
*/
#define MSG_TOKEN_UNRECOGNIZEDSTRING_S                  _MESSAGE(33050, _("unrecognized string "SFQ))
#define MSG_ANSWER_ERRORINDISABLYEAROFCALENDARXY_SS     _MESSAGE(33051, _("error in disabled_year of calendar "SFQ": "SFN"\n"))
#define MSG_ANSWER_GOTEQUALWITHOUTDAYTIMERANGEORSTATE   _MESSAGE(33052, _("got \"=\" without daytime_range/state"))
#define MSG_ANSWER_FIRSTYESTERDAYINRANGEMUSTBEBEFORESECONDYESTERDAY     _MESSAGE(33053, _("first yearday in range must be before second yearday"))
#define MSG_PARSE_MISSINGPOINTAFTERDAY      _MESSAGE(33054, _("missing \".\" after day"))
#define MSG_PARSE_MISSINGPOINTAFTERMONTH    _MESSAGE(33055, _("missing \".\" after month"))
#define MSG_PARSE_WOUTSIDEOFRANGEXYZ_SIIS   _MESSAGE(33056, _(SFQ" outside of range %d-%d of "SFN))
#define MSG_PARSE_XISNOTAY_SS               _MESSAGE(33057, _(SFQ" is not a "SFN))
#define MSG_PARSE_MISSINGDASHINDAYTIMERANGE _MESSAGE(33058, _("missing '-' in daytime range"))
#define MSG_PARSE_RANGEBEGISEQUALTOEND      _MESSAGE(33059, _("range begin is equal to end - use 0-24 instead"))
#define MSG_PARSE_DAYTIMESBEYOND24HNOTALLOWED  _MESSAGE(33060, _("daytimes may not be beyond 24:00"))
#define MSG_PARSE_HOURSPEC                  _MESSAGE(33061, _("hour specification"))
#define MSG_PARSE_MINUTESPEC                _MESSAGE(33062, _("minute specification"))
#define MSG_PARSE_SECONDSSPEC               _MESSAGE(33063, _("seconds specification"))
#define MSG_PARSE_XISNOTASTATESPECIFIER_S   _MESSAGE(33064, _(SFQ" is not a state specifier"))
#define MSG_PARSE_UNRECOGNIZEDTOKENATEND    _MESSAGE(33065, _("unrecognized token at end\n"))
#define MSG_PARSE_ERRORINDISABLEDWEEKOFCALENDAR_SS     _MESSAGE(33066, _("error in disabled_week of calendar "SFQ": "SFN""))
#define MSG_PARSE_FOUNDUSELESSWEEKDAYRANGE  _MESSAGE(33067, _("found useless weekday range"))
#define MSG_PARSE_EXPECTEDSTRINGFORWEEKDAY  _MESSAGE(33068, _("expected string for weekday"))
#define MSG_PARSE_XISNOTAWEEKDAY_S          _MESSAGE(33069, _(SFQ" is not a weekday"))
#define MSG_PARSE_OVERFLOWERRORWHILEPARSING _MESSAGE(33070, _("overflow error while parsing\n"))


/*
** global qmaster messages
*/ 


/* 
** gdi_utility_qmaster.c 
*/
#define MSG_GDI_SIG_DIGIT_SS         _MESSAGE(33071, _("denied: attribute "SFQ" contains invalid value "SFQ"\n"))
#define MSG_GDI_METHOD_VARS_SS       _MESSAGE(33072, _("parameter "SFQ" of queue: "SFN"\n"))
#define MSG_GDI_APATH_S              _MESSAGE(33073, _("denied: path given for "SFQ" must start with an \"/\"\n"))
#define MSG_GDI_VARS_SS              _MESSAGE(33074, _("parameter "SFQ": "SFN"\n"))
#define MSG_GDI_VALUE_S              _MESSAGE(33075, _("denied: attribute "SFQ" contains invalid value (null)\n"))
#define MSG_GDI_TYPE_MEM_SS          _MESSAGE(33076, _("value for attribute "SFN" "SFQ" is not a memory value\n"))
#define MSG_GDI_TYPE_TIME_SS         _MESSAGE(33077, _("value for attribute "SFN" "SFQ" is not a time value\n"))
/* #define MSG_GDI_KEYSTR_SS            _message(33078, _("invalid "SFN" "SFQ"\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_GDI_MULTIPLE_OCCUR_SSSS  _MESSAGE(33079, _("denied: multiple occurances of "SFN" "SFQ" in "SFN" "SFQ"\n"))
#define MSG_GDI_KEYSTR_FIRSTCHAR_SC  _MESSAGE(33081, _(SFN" (\'%c\') not allowed as first character of objectname\n") ) 
#define MSG_GDI_KEYSTR_FIRSTCHAR_S   _MESSAGE(33082, _(SFN" not allowed as first character of objectname\n") ) 
#define MSG_GDI_KEYSTR_MIDCHAR_SC    _MESSAGE(33083, _(SFN" (\'%c\') not allowed in objectname\n") ) 
#define MSG_GDI_KEYSTR_MIDCHAR_S     _MESSAGE(33084, _(SFN" not allowed in objectname\n") ) 
#define MSG_GDI_KEYSTR_KEYWORD_SS     _MESSAGE(33085, _(SFN" ("SFQ") not allowed as objectname\n") ) 
#define MSG_GDI_KEYSTR_KEYWORD         _MESSAGE(33086, _("Keyword"))
#define MSG_GDI_KEYSTR_DOT             _MESSAGE(33087, _("Dot"))
#define MSG_GDI_KEYSTR_HASH            _MESSAGE(33088, _("Hash"))
#define MSG_GDI_KEYSTR_RETURN          _MESSAGE(33089, _("Return"))
#define MSG_GDI_KEYSTR_TABULATOR       _MESSAGE(33090, _("Tabulator"))
#define MSG_GDI_KEYSTR_CARRIAGERET     _MESSAGE(33091, _("Carriage return"))
#define MSG_GDI_KEYSTR_SPACE           _MESSAGE(33092, _("Space"))
#define MSG_GDI_KEYSTR_SLASH           _MESSAGE(33093, _("Slash"))
#define MSG_GDI_KEYSTR_COLON           _MESSAGE(33094, _("Colon"))
#define MSG_GDI_KEYSTR_QUOTE           _MESSAGE(33095, _("Quote"))
#define MSG_GDI_KEYSTR_DBLQUOTE        _MESSAGE(33096, _("Double quote"))
#define MSG_GDI_KEYSTR_BACKSLASH       _MESSAGE(33097, _("Backslash"))
#define MSG_GDI_KEYSTR_BRACKETS        _MESSAGE(33098, _("Brackets"))
#define MSG_GDI_KEYSTR_BRACES          _MESSAGE(33099, _("Braces"))
#define MSG_GDI_KEYSTR_PARENTHESIS     _MESSAGE(33100, _("Parenthesis"))
#define MSG_GDI_KEYSTR_AT              _MESSAGE(33101, _("At"))
#define MSG_GDI_KEYSTR_PIPE            _MESSAGE(33102, _("Pipe"))

/*
** ck_to_do_qmaster.c
*/

/*
** sge_c_ack.c
*/
#define MSG_COM_ACK_S                _MESSAGE(33105, _("ack event from "SFN"\n") )     
#define MSG_COM_ACKEVENTFORUNKOWNJOB_U _MESSAGE(33106, _("ack event for unknown job "U32CFormat"\n"))
#define MSG_COM_ACKEVENTFORUNKNOWNTASKOFJOB_UU _MESSAGE(33107, _("ack event for unknown task " U32CFormat  " of job " U32CFormat " \n"))
#define MSG_COM_UNKNOWN_TAG          _MESSAGE(33108, _("received unknown ack tag "U32CFormat"\n"))

#define MSG_COM_ACK_U                _MESSAGE(33109, _("signalling acknowledged for unknown job " U32CFormat  "\n"))
/* #define MSG_COM_ACK_UU               _message(33110, _("signalling acknowledged for unknown task " U32CFormat  " of job " U32CFormat " \n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_COM_ACK_QUEUE_U          _MESSAGE(33112, _("ack event for unknown queue " U32CFormat "\n"))
#define MSG_COM_ACK_UNKNOWN          _MESSAGE(33113, _("unknown ack event\n"))

/*
** sge_c_gdi.c
*/
#define MSG_GDI_WRONG_GDI_SSISS  _MESSAGE(33115, _("denied: client ("SFN"/"SFN"/%d) uses old GDI version "SFN" while qmaster uses newer version "SFN"\n"))
#define MSG_GDI_WRONG_GDI_SSIUS  _MESSAGE(33116, _("denied: client ("SFN"/"SFN"/%d) uses newer GDI version "U32CFormat" while qmaster uses older version "SFN"\n"))

#define MSG_GDI_NULL_IN_GDI_SSS  _MESSAGE(33117, _("denied: got NULL in "SFN"/"SFN" of gdi request from host "SFQ"\n"))
#define MSG_GDI_OKNL             _MESSAGE(33118, _("ok\n"))
#define MSG_MEM_MALLOC           _MESSAGE(33121, _("malloc failure\n"))
#define MSG_SGETEXT_UNKNOWNOP    _MESSAGE(33122, _("unknown operation\n"))

#define MSG_SGETEXT_OPNOIMPFORTARGET             _MESSAGE(33125, _("operation not implemented for target\n"))
#define MSG_SGETEXT_NOADMINHOST_S                _MESSAGE(33126, _("denied: host "SFQ" is no admin host\n"))
#define MSG_SGETEXT_NOSUBMITHOST_S               _MESSAGE(33127, _("denied: host "SFQ" is no submit host\n"))
#define MSG_SGETEXT_NOSUBMITORADMINHOST_S        _MESSAGE(33128, _("denied: host "SFQ" is neither submit nor admin host\n"))
#define MSG_SGETEXT_ALREADYEXISTS_SS             _MESSAGE(33129, _(""SFN" "SFQ" already exists\n"))

#define MSG_GDI_EVENTCLIENTIDFORMAT_S      _MESSAGE(33131, _("invalid event client id format " SFQ "\n"))


/*
** sge_calendar_qmaster.c
*/
#define MSG_OBJ_CALENDAR              _MESSAGE(33132, _("calendar"))
#define MSG_EVE_TE4CAL_S              _MESSAGE(33133, _("got timer event for unknown calendar "SFQ"\n"))
#define MSG_SGETEXT_REMOVEDFROMLIST_SSSS        _MESSAGE(33135, _(""SFN"@"SFN" removed "SFQ" from "SFN" list\n"))
#define MSG_INVALID_CENTRY_DEL_S                _MESSAGE(33136, _("The build-in complex "SFQ" cannot be deleted\n"))



/*
** sge_give_jobs.c
*/
#define MSG_COM_CANT_DELIVER_UNHEARD_SSU _MESSAGE(33137, _("got max. unheard timeout for target "SFQ" on host "SFQ", can't delivering job \""U32CFormat"\"\n"))
  
#define MSG_OBJ_UNABLE2FINDCKPT_S     _MESSAGE(33138, _("can't find checkpointing object "SFQ"\n"))
#define MSG_OBJ_UNABLE2CREATECKPT_SU  _MESSAGE(33139, _("can't create checkpointing object "SFQ" for job " U32CFormat "\n"))
/* EB: remove
#define MSG_SEC_NOCRED_USSI           _MESSAGE(33140, _("could not get credentials for job " U32CFormat " for execution host "SFN" - command "SFQ" failed with return code %d\n"))
#define MSG_SEC_NOCREDNOBIN_US        _MESSAGE(33141, _("could not get client credentials for job " U32CFormat" - "SFN" binary does not exist\n"))
*/
#define MSG_COM_SENDJOBTOHOST_US      _MESSAGE(33142, _("can't send job \"" U32CFormat"\" to host "SFQ))
#define MSG_COM_RESENDUNKNOWNJOB_UU   _MESSAGE(33143, _("cannot resend unknown job "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_UNKNOWNGDIL4TJ_UU     _MESSAGE(33144, _("transfering job "U32CFormat"."U32CFormat" has an invalid gdi list --- deleting"))
#define MSG_JOB_NOQUEUE4TJ_SUU        _MESSAGE(33145, _("queue "SFQ" in transiting job "U32CFormat"."U32CFormat" doesn't exist. deleting task"))
#define MSG_JOB_NOHOST4TJ_SUU         _MESSAGE(33146, _("execution host "SFQ" for transfering job "U32CFormat"."U32CFormat" doesn't exist. deleting task"))
#define MSG_JOB_NOPE4TJ_SUU           _MESSAGE(33147, _("parallel environment "SFQ" for transfering job "U32CFormat"." U32CFormat" doesn't exist. deleting job"))
#define MSG_JOB_DELIVER2Q_UUS         _MESSAGE(33148, _("failed to deliver job "U32CFormat"."U32CFormat" to queue "SFQ))
#define MSG_JOB_RESCHEDULE_UU         _MESSAGE(33159, _("rescheduling job "U32CFormat"."U32CFormat) ) 
#define MSG_RU_CANCELED_S             _MESSAGE(33160, _("Due to a modification of the reschedule_unknown timeout rescheduling for host "SFN" was canceled."))

/* CR: don't localize mail subject, until we send it in Mime format!
 *  The message definition is not l10n'ed (no _() macro used)!!!     
 */
/* #define MSG_RU_MAILSUB_SS             _MESSAGE(33162, _("Pushed rescheduling of "SFN" "SFN)) */
#define MSG_RU_MAILSUB_SS             "Pushed rescheduling of "SFN" "SFN



#define MSG_RU_MAILBODY_SSSS          _MESSAGE(33163, _("Your "SFN" "SFN" is was running on host "SFN". "SFN" manually/automatic rescheduling for this "SFN"."))
#define MSG_RU_MAILTYPE               _MESSAGE(33164, _("job rescheduling"))
#define MSG_RU_PUSHEDR                _MESSAGE(33165, _("Pushed"))
#define MSG_RU_FORCEDR                _MESSAGE(33166, _("Forced"))
#define MSG_RU_MSGFILEINFO            _MESSAGE(33167, _(SFN" rescheduling of "SFN" "SFN" on host "SFN"\n"))
#define MSG_RU_TYPEJOBARRAY           _MESSAGE(33168, _("job-array task"))
#define MSG_RU_TYPEJOB                _MESSAGE(33169, _("job"))
#define MSG_RU_JR_ERRSTR              _MESSAGE(33170, _("manual/auto rescheduling") )  
#define MSG_RU_NOT_RESTARTABLE_SS     _MESSAGE(33171, _("The "SFN" "SFN" is not restartable\n"))
#define MSG_RU_INTERACTIVEJOB_SSS     _MESSAGE(33172, _(SFN" is a qsh, qlogin, qrsh or a qrlogin "SFN". These types of "SFN" are not restartable\n"))
#define MSG_RU_CKPTNOTVALID_SSS       _MESSAGE(33173, _(SFN" requests ckpt object "SFN". Therefore this "SFN" is not restartable.\n"))
#define MSG_RU_CKPTEXIST_SS           _MESSAGE(33174, _(SFN" requests ckpt object "SFN". This ckpt object does not exist.\n"))
#define MSG_RU_INDELETEDSTATE_SS      _MESSAGE(33175, _("The "SFN" "SFN" is already in deleted state. No rescheduling!\n"))
#define MSG_RU_NORERUNQUEUE_SSS       _MESSAGE(33176, _("The "SFN" "SFN" is running in queue "SFN" were jobs are not rerunable.\n"))

/*
** sge_host_qmaster.c
*/
#define MSG_OBJ_DELGLOBALHOST         _MESSAGE(33177, _("denied: pseudo host \"global\" may not be deleted\n"))
#define MSG_OBJ_LOADREPORTIVAL_SS     _MESSAGE(33178, _("host "SFQ": "SFQ" is not a valid time value for \"load_report_time\" - assuming 120 seconds\n") )  
#define MSG_OBJ_RESCHEDULEUNKN_SS     _MESSAGE(33179, _("host "SFQ": "SFQ" is not a valid time value for \"reschedule_unknown\" - assuming 0 => no auto rescheduling\n"))
#define MSG_OBJ_SHUTDOWNPERMS         _MESSAGE(33180, _("shutting down execd requires manager privileges\n"))
#define MSG_OBJ_NOEXECDONHOST_S       _MESSAGE(33181, _("no execd known on host "SFN"\n"))
#define MSG_COM_NONOTIFICATION_SSS    _MESSAGE(33182, _("failed sending "SFN" notification to "SFN" execd host "SFN"\n"))
#define MSG_COM_NOTIFICATION_SSS      _MESSAGE(33184, _("sent "SFN" notification to "SFN" execd host "SFN"\n"))
#define MSG_OBJ_UNKNOWN               _MESSAGE(33186, _("unknown"))
#define MSG_NOTIFY_SHUTDOWNANDKILL    _MESSAGE(33187, _("shutdown and kill"))
#define MSG_NOTIFY_SHUTDOWN           _MESSAGE(33188, _("shutdown"))

/* CR: don't localize mail subject, until we send it in Mime format!
 *  The message definition is not l10n'ed (no _() macro used)!!!     
 */
#define MSG_MAIL_JOBKILLEDSUBJ_US     "Job " U32CFormat " ("SFN") Killed"

#define MSG_MAIL_JOBKILLEDBODY_USS    _MESSAGE(33189, _("Job " U32CFormat " ("SFN") was killed due to a kill execd on host "SFN))
#define MSG_OBJ_INVALIDHOST_S         _MESSAGE(33190, _("invalid hostname "SFQ"\n"))
#define MSG_OBJ_NOADDHOST_S           _MESSAGE(33191, _("adding host "SFQ" failed\n"))
#define MSG_LOG_REGISTER_SS           _MESSAGE(33192, _(SFN" on "SFN" registered"))
#define MSG_OBJ_NOSCALING4HOST_SS     _MESSAGE(33193, _("denied: scaling attribute "SFQ" is not configured for host "SFQ"\n") )  
#define MSG_SGETEXT_ISNOEXECHOST_S              _MESSAGE(33194, _(SFQ" is not an execution host\n"))
#define MSG_SGETEXT_NOEXECHOSTS                 _MESSAGE(33195, _("there are no execution hosts to kill\n"))
#define MSG_SGETEXT_CANTDELADMINQMASTER_S       _MESSAGE(33197, _("denied: can't delete master host "SFQ" from admin host list\n") )   
#define MSG_CANT_ASSOCIATE_LOAD_SS    _MESSAGE(33200, _("got load report from host "SFQ" - reports load value for host "SFQ"\n"))

/*
** sge_job.c
*/
/* #define MSG_JOB_SENDKILLTOXFORJOBYZ_SUU _message(33201, _("send kill to "SFQ" for job "U32CFormat"."U32CFormat)) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_NORESUBPERMS_SSS      _MESSAGE(33202, _("job rejected: "SFN"@"SFN" is not allowed to resubmit jobs of user "SFN"\n"))
#define MSG_JOB_NOPERMS_SS            _MESSAGE(33203, _("job rejected: "SFN"@"SFN" is not allowed to submit jobs\n"))
#define MSG_JOB_MORETASKSTHAN_U       _MESSAGE(33204, _("job rejected: You try to submit a job with more than "U32CFormat" tasks\n"))
#define MSG_JOB_UID2LOW_II            _MESSAGE(33205, _("job rejected: your user id %d is lower than minimum user id %d of cluster configuration\n"))
#define MSG_JOB_GID2LOW_II            _MESSAGE(33206, _("job rejected: your group id %d is lower than minimum group id %d of cluster configuration\n"))
#define MSG_JOB_ALLOWEDJOBSPERUSER_UU  _MESSAGE(33207, _("job rejected: Only "U32CFormat" jobs are allowed per user (current job count: "U32CFormat")\n"))
#define MSG_JOB_ALLOWEDJOBSPERCLUSTER _MESSAGE(33208, _("job rejected: Only "U32CFormat" jobs are allowed per cluster\n"))
#define MSG_JOB_NOSCRIPT              _MESSAGE(33211, _("job rejected: no script in your request\n"))
#define MSG_JOB_PEUNKNOWN_S           _MESSAGE(33212, _("job rejected: the requested parallel environment "SFQ" does not exist\n"))
#define MSG_JOB_CKPTUNKNOWN_S         _MESSAGE(33213, _("job rejected: the requested checkpointing environment "SFQ" does not exist\n"))
#define MSG_JOB_PERANGEMUSTBEGRZERO   _MESSAGE(33214, _("job rejected: pe range must be greater than zero\n"))
#define MSG_JOB_CKPTMINUSC            _MESSAGE(33215, _("job rejected: checkpointing with \"-c n\" requested\n"))
#define MSG_JOB_NOCKPTREQ             _MESSAGE(33216, _("job rejected: checkpointing without checkpointing environment requested\n"))
#define MSG_JOB_CKPTDENIED            _MESSAGE(33217, _("checkpointing denied\n") )   
#define MSG_JOB_NOTINANYQ_S           _MESSAGE(33218, _("warning: "SFN" your job is not allowed to run in any queue\n"))
#define MSG_JOB_PRJUNKNOWN_S          _MESSAGE(33219, _("job rejected: the requested project "SFQ" does not exist\n"))
#define MSG_JOB_USRUNKNOWN_S          _MESSAGE(33220, _("job rejected: the user "SFQ" does not exist\n"))
/* #define MSG_JOB_CRED4PRJLOW_FSF       _message(33221, _("credit %.2f of project '"SFN"' is below credit limit %.2f\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_PRJNOSUBMITPERMS_S    _MESSAGE(33222, _("job rejected: project "SFQ" is not allowed to submit jobs\n"))
#define MSG_JOB_PRJREQUIRED           _MESSAGE(33223, _("job rejected: no project was supplied and only certain projects are allowed to submit jobs\n"))
#define MSG_JOB_NODEADLINEUSER_S      _MESSAGE(33224, _("job rejected: the user "SFQ" is no deadline initiation user\n"))
/* #define MSG_JOB_TASKIDZERO_U          _message(33225, _("job "U32CFormat" was rejected cause the job contains a task with id 0\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_NOJOBNAME_U           _MESSAGE(33226, _("job "U32CFormat" was rejected cause there is no job_name in the request\n"))
#define MSG_JOB_NOWRITE_US            _MESSAGE(33227, _("job "U32CFormat" was rejected cause it can't be written: "SFN"\n"))
#define MSG_JOB_NOWRITE_U             _MESSAGE(33228, _("job "U32CFormat" was rejected cause it couldn't be written\n"))
#define MSG_JOB_DEPENDENCY_CYCLE_UU   _MESSAGE(33229, _("job "U32CFormat" dependency change would generate a dependency cycle with job "U32CFormat"\n"))
#define MSG_JOB_SUBMITJOB_USS         _MESSAGE(33234, _("your job "U32CFormat" (\""SFN SFN"\") has been submitted\n"))
#define MSG_JOB_SUBMITJOBARRAY_UUUUSS _MESSAGE(33235, _("your job-array "U32CFormat"."U32CFormat"-"U32CFormat":"U32CFormat" (\""SFN SFN"\") has been submitted\n"))
#define MSG_LOG_NEWJOB                _MESSAGE(33236, _("new job"))
/* #define MSG_JOB_MODIFYALL             _message(33237, _("modify all jobs")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_DELETEJOB             _MESSAGE(33238, _("delete job"))
#define MSG_JOB_JOB                   _MESSAGE(33239, _("Job"))
#define MSG_JOB_FORCEDDELETEPERMS_S   _MESSAGE(33240, _(SFQ" - forcing a job deletion requires manager privileges\n"))
#define MSG_JOB_DELETEPERMS_SU        _MESSAGE(33241, _(SFN" - you do not have the necessary privileges to delete the job \"" U32CFormat "\"\n"))
#define MSG_JOB_DELETETASK_SUU        _MESSAGE(33242, _(SFN" has deleted job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_DELETETASKS_SSU       _MESSAGE(33243, _(SFN" has deleted job-array tasks "SFN" of job "U32CFormat"\n"))
#define MSG_JOB_DELETEJOB_SU          _MESSAGE(33244, _(SFN" has deleted job "U32CFormat"\n"))
#define MSG_JOB_DISCONTINUEDTRANS_SU  _MESSAGE(33245, _("Discontinued delete transaction of user "SFQ" after job "U32CFormat"\n"))
#define MSG_JOB_UNABLE2FINDQOFJOB_S   _MESSAGE(33246, _("can't locate the queue "SFQ" associated with this job\n"))

/* CR: don't localize mail subject, until we send it in Mime format!
 *  The message definition is not l10n'ed (no _() macro used)!!!     
 */
#define MSG_MAIL_TASKKILLEDSUBJ_UUS   "Job-array task "U32CFormat"."U32CFormat" ("SFN") Killed"


#define MSG_MAIL_TASKKILLEDBODY_UUSSS _MESSAGE(33247, _("Job-array task "U32CFormat"."U32CFormat" ("SFN") was killed by "SFN"@"SFN))
#define MSG_MAIL_JOBKILLEDBODY_USSS   _MESSAGE(33248, _("Job " U32CFormat " ("SFN")  was killed by "SFN"@"SFN))
#define MSG_MAIL_BECAUSE              _MESSAGE(33249, _("\nbecause "))
#define MSG_JOB_FORCEDDELTASK_SUU     _MESSAGE(33250, _("warning: "SFN" forced the deletion of job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_FORCEDDELJOB_SU       _MESSAGE(33251, _("warning: "SFN" forced the deletion of job "U32CFormat"\n"))
#define MSG_COM_NOSYNCEXECD_SU        _MESSAGE(33252, _(SFN" unable to sync state with remote execd for the deletion of job \"" U32CFormat "\"\n"))
#define MSG_JOB_REGDELTASK_SUU        _MESSAGE(33253, _(SFN" has registered the job-array task "U32CFormat"."U32CFormat" for deletion\n"))
#define MSG_JOB_REGDELJOB_SU          _MESSAGE(33254, _(SFN" has registered the job "U32CFormat" for deletion\n"))
#define MSG_JOB_CHANGEATTR            _MESSAGE(33255, _("change job attributes"))
#define MSG_JOB_NOALTERNOWRITE_U      _MESSAGE(33256, _("alternation of job "U32CFormat" was rejected cause it couldn't be written\n"))
#define MSG_JOB_CHANGESHAREFUNC       _MESSAGE(33257, _("change share of job functional tickets"))
#define MSG_JOB_SETSHAREFUNC_SSUUU    _MESSAGE(33258, _(SFN"@"SFN" sets job functional ticket share of job "U32CFormat"."U32CFormat" to "U32CFormat"\n"))
#define MSG_JOB_RMHOLDMNG             _MESSAGE(33259, _("remove manager hold"))
#define MSG_JOB_SETHOLDMNG            _MESSAGE(33260, _("set manager hold"))
#define MSG_JOB_RMHOLDOP              _MESSAGE(33261, _("remove operator hold"))
#define MSG_JOB_SETHOLDOP             _MESSAGE(33262, _("set operator hold"))
#define MSG_JOB_RMHOLDUSER            _MESSAGE(33263, _("remove user hold"))
#define MSG_JOB_SETHOLDUSER           _MESSAGE(33264, _("set user hold"))
#define MSG_JOB_HOLD                  _MESSAGE(33265, _("hold"))
#define MSG_JOB_NOJOBARRAY_U          _MESSAGE(33266, _(U32CFormat " is no job array\n"))
#define MSG_JOB_CHANGEOVERRIDETICKS   _MESSAGE(33267, _("change amount of override tickets"))
#define MSG_JOB_SETOVERRIDETICKS_SSUU _MESSAGE(33268, _(SFN"@"SFN" sets override tickets of job "U32CFormat" to "U32CFormat"\n"))
#define MSG_JOB_PRIOINC               _MESSAGE(33269, _("increase job priority"))
#define MSG_JOB_JOBSHARESET_SSUU      _MESSAGE(33270, _(SFN"@"SFN" sets job share of job "U32CFormat" to "U32CFormat"\n"))
#define MSG_JOB_ACCOUNT               _MESSAGE(33271, _("account"))
#define MSG_JOB_WD                    _MESSAGE(33272, _("working directory"))
#define MSG_JOB_STARTTIME             _MESSAGE(33273, _("start time"))
#define MSG_JOB_STDERRPATHLIST        _MESSAGE(33274, _("stderr path list"))
#define MSG_JOB_STDOUTPATHLIST        _MESSAGE(33275, _("stdout path list"))
#define MSG_JOB_HOLDLISTMOD_USS       _MESSAGE(33276, _("modified job id hold list of job "U32CFormat"\n   blocking jobs: "SFN"\n   exited jobs:   "SFN"\n"))
#define MSG_JOB_MERGEOUTPUT           _MESSAGE(33277, _("output merge behaviour"))
#define MSG_JOB_RESERVE               _MESSAGE(33699, _("reservation behaviour"))
#define MSG_JOB_HARDRESOURCELIST      _MESSAGE(33278, _("hard resource list"))
#define MSG_JOB_SOFTRESOURCELIST      _MESSAGE(33279, _("soft resource list"))
#define MSG_JOB_MAILOPTIONS           _MESSAGE(33280, _("mail options"))
#define MSG_JOB_MAILLIST              _MESSAGE(33281, _("mail list"))
#define MSG_JOB_JOBNAME               _MESSAGE(33282, _("job name"))
#define MSG_JOB_NOTIFYBEHAVIOUR       _MESSAGE(33283, _("notify behaviour"))
#define MSG_JOB_SLOTRANGE             _MESSAGE(33284, _("slot range"))
#define MSG_JOB_HARDQLIST             _MESSAGE(33286, _("hard queue list"))
#define MSG_JOB_SOFTQLIST             _MESSAGE(33287, _("soft queue list"))
#define MSG_JOB_MASTERHARDQLIST       _MESSAGE(33288, _("master hard queue list"))
#define MSG_JOB_RESTARTBEHAVIOR       _MESSAGE(33289, _("restart behaviour"))
#define MSG_JOB_SHELLLIST             _MESSAGE(33290, _("shell list"))
#define MSG_JOB_ENVLIST               _MESSAGE(33291, _("environment"))
#define MSG_JOB_QSARGS                _MESSAGE(33292, _("qs args"))
#define MSG_JOB_SCRIPTARGS            _MESSAGE(33293, _("script arguments"))
#define MSG_JOB_CONTEXT               _MESSAGE(33294, _("context"))
#define MSG_JOB_NODIRECTSLOTS         _MESSAGE(33295, _("denied: use parallel environments instead of requesting slots explicitly\n"))
#define MSG_JOB_NOSEQNRREAD_SS        _MESSAGE(33296, _("can't read job sequence number in file "SFQ": "SFN))
#define MSG_JOB_NOSEQFILEOPEN_SS      _MESSAGE(33297, _("can't open sequence number file "SFQ": for reading: "SFN" -- guessing next job number"))
#define MSG_JOB_NOSEQFILECREATE_SS    _MESSAGE(33298, _("can't create job sequence number file "SFQ": "SFN" - delaying until next job"))
#define MSG_JOB_NOSUITABLEQ_S         _MESSAGE(33299, _(SFN": no suitable queues\n"))
#define MSG_JOB_VERIFYERROR           _MESSAGE(33300, _("error"))
#define MSG_JOB_VERIFYWARN            _MESSAGE(33301, _("warning"))
#define MSG_JOB_VERIFYVERIFY          _MESSAGE(33302, _("verification"))
#define MSG_JOB_VERIFYFOUNDQ          _MESSAGE(33303, _("verification: found suitable queue(s)\n"))
#define MSG_JOB_VERIFYFOUNDSLOTS_I    _MESSAGE(33304, _("verification: found possible assignment with %d slots\n"))
#define MSG_JOB_MOD_SOFTREQCONSUMABLE_S  _MESSAGE(33307, _("denied: soft requests on consumables like "SFQ" are not supported\n"))
#define MSG_JOB_MOD_MISSINGRUNNINGJOBCONSUMABLE_S     _MESSAGE(33308, _("denied: former resource request on consumable "SFQ" of running job lacks in new resource request\n"))
#define MSG_JOB_MOD_ADDEDRUNNINGJOBCONSUMABLE_S       _MESSAGE(33309, _("denied: resource request on consumable "SFQ" of running job was not contained former resource request\n"))
#define MSG_JOB_MOD_CHANGEDRUNNINGJOBCONSUMABLE_S     _MESSAGE(33310, _("denied: can't change consumable resource request "SFQ" of running job\n"))
#define MSG_JOB_MOD_GOTOWNJOBIDINHOLDJIDOPTION_U      _MESSAGE(33311, _("denied: job \""U32CFormat"\" may not be it's own jobnet predecessor\n"))
#define MSG_JOB_MOD_UNKOWNJOBTOWAITFOR_S              _MESSAGE(33312, _("denied: job "SFQ" not found\n"))
#define MSG_JOB_MOD_NOJOBNAME_S                      _MESSAGE(33314, _("denied: "SFQ" is not a valid job name (job cannot start with a digit)\n"))
#define MSG_SGETEXT_NEEDONEELEMENT_SS                 _MESSAGE(33317, _("denied: request format error: need at least one element in sublist "SFQ" in "SFN"()\n"))
#define MSG_SGETEXT_CANT_MOD_RUNNING_JOBS_U           _MESSAGE(33318, _("job "U32CFormat" can't modify running jobs\n") ) 
#define MSG_SGETEXT_MUST_BE_OPR_TO_SS                 _MESSAGE(33319, _("denied: "SFQ" must be operator to "SFN"\n"))
#define MSG_SGETEXT_MOD_JOBS_SU                       _MESSAGE(33320, _("modified "SFN" of job "U32CFormat"\n") )     
#define MSG_SGETEXT_DOESNOTEXIST_SU                   _MESSAGE(33321, _(""SFN" \"" U32CFormat "\" does not exist\n"))
#define MSG_SGETEXT_DOESNOTEXISTTASK_SUS               _MESSAGE(33322, _("job \""SFN"\" task \"" U32CFormat "\" does not exist for user(s) "SFN"\n"))
#define MSG_SGETEXT_DOESNOTEXISTTASKRANGE_SUUUS        _MESSAGE(33323, _("job \""SFN"\" task id range \"" U32CFormat "-" U32CFormat ":" U32CFormat "\" comprises no tasks under user(s) "SFN"\n"))
#define MSG_SGETEXT_NO_PROJECT                        _MESSAGE(33324, _("job rejected: no project assigned to job\n") )     
#define MSG_SGETEXT_MOD_JATASK_SUU                    _MESSAGE(33325, _("modified "SFN" of job-array task "U32CFormat"."U32CFormat"\n") )  
#define MSG_SGETEXT_MUST_BE_MGR_TO_SS                 _MESSAGE(33326, _("denied: "SFQ" must be manager to "SFN"\n"))
#define MSG_SGETEXT_MUST_BE_JOB_OWN_TO_SUS            _MESSAGE(33327, _("denied: "SFQ" must be at least owner of job "U32CFormat" to "SFN"\n"))
#define MSG_SGETEXT_NOJOBSDELETED                     _MESSAGE(33328, _("No jobs deleted\n"))
#define MSG_SGETEXT_NOJOBSMODIFIED                    _MESSAGE(33329, _("No jobs modified\n"))
#define MSG_SGETEXT_THEREARENOJOBS                    _MESSAGE(33330, _("There are no jobs registered\n"))
#define MSG_SGETEXT_THEREARENOJOBSFORUSERS_S          _MESSAGE(33331, _("There are no jobs registered for following users: "SFN"\n"))
#define MSG_SGETEXT_SPECIFYUSERORJID                  _MESSAGE(33332, _("You have to specify a username or job ids\n"))
#define MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS             _MESSAGE(33334, _("job rejected: no access to project "SFQ" for user "SFQ"\n") ) 
#define MSG_SGETEXT_NOTALLOWEDTOSPECUSERANDJID        _MESSAGE(33335, _("it is not allowed to select users and job ids together\n"))
#define MSG_SGETEXT_MODIFIEDINLIST_SSUS               _MESSAGE(33336, _(""SFN"@"SFN" modified \"" U32CFormat "\" in "SFN" list\n"))
#define MSG_JOB_MOD_JOBDEPENDENCY_MEMORY              _MESSAGE(33337, _("could not create job dependency list\n"))

#define MSG_SGETEXT_SPECIFYONEORALLUSER               _MESSAGE(33338, _("The switch for \"all users\" and a specified \"user list\" are not allowed together\n"))
#define MSG_SGETEXT_OPTIONONLEONJOBS_U                _MESSAGE(33339, _("The specified option works only on jobs ("U32CFormat")\n"))
#define MSG_SGETEXT_DEL_JOB_SS                        _MESSAGE(33340, _("The job "SFN" of user(s) "SFN" does not exist\n"))
#define MSG_SGETEXT_DOESNOTEXISTTASK_SU               _MESSAGE(33341, _("job \""SFN"\" task \"" U32CFormat "\" does not exist\n"))
#define MSG_SGETEXT_DOESNOTEXISTTASKRANGE_SUUU        _MESSAGE(33342, _("job \""SFN"\" task id range \"" U32CFormat "-" U32CFormat ":" U32CFormat "\" comprises no tasks\n"))
/*
** sge_manop.c
*/
#define MSG_OBJ_MANAGER               _MESSAGE(33359, _("manager"))
#define MSG_OBJ_OPERATOR              _MESSAGE(33360, _("operator"))
#define MSG_SGETEXT_MAY_NOT_REMOVE_USER_FROM_LIST_SS  _MESSAGE(33361, _("may not remove user "SFQ" from "SFN" list\n"))

/*
** sge_pe_qmaster.c
*/
/* #define MSG_PE_SLOTSTOOLOW_I          _message(33362, _("new number of slots may not be less than %d that are in use\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_PE_DEBITSLOTS_IS          _message(33366, _("debiting %d slots on pe "SFN)) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_PE_USEDSLOTSBELOWZERO_S   _message(33367, _("PE_used_slots of pe "SFN" sunk under 0!\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_PE_REVERSESLOTS_IS        _message(33368, _("reversing %d slots on pe "SFN)) __TS Removed automatically from testsuite!! TS__*/
#define MSG_PE_USEDSLOTSTOOBIG_S      _MESSAGE(33369, _("PE_used_slots of pe "SFN" is greater than PE_slots!\n") )  

/*
** sge_ckpt_qmaster.c
*/ 
#define MSG_CKPT_INVALIDWHENATTRIBUTE_S _MESSAGE(33372, _("Invalid \"when\" attribute for ckpt "SFQ"\n"))

/*
** sge_qmod_qmaster.c
*/
#define MSG_LOG_JOBUNKNOWNQMODCMD_U   _MESSAGE(33373, _("unknown job command type\n" U32CFormat))
#define MSG_QUEUE_INVALIDQ_S          _MESSAGE(33374, _("invalid queue "SFQ"\n"))
#define MSG_QUEUE_INVALIDQORJOB_S     _MESSAGE(33375, _("invalid queue or job "SFQ"\n"))
#define MSG_QUEUE_NOCHANGEQPERMS_SS   _MESSAGE(33376, _(SFN" - you have no permission to modify queue "SFQ"\n"))
#define MSG_LOG_QUNKNOWNQMODCMD_U      _MESSAGE(33377, _("unknown queue command type\n" U32CFormat))
#define MSG_JOB_NOMODJOBPERMS_SU      _MESSAGE(33378, _(SFN" - you have no permission to modify job \"" U32CFormat "\"\n"))
#define MSG_JOB_CLEARERRORTASK_SSUU   _MESSAGE(33379, _(SFN"@"SFN" cleared error state of job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_CLEARERRORJOB_SSU     _MESSAGE(33380, _(SFN"@"SFN" cleared error state of job "U32CFormat"\n"))
#define MSG_JOB_NOERRORSTATETASK_UU   _MESSAGE(33381, _("Job-array task "U32CFormat"."U32CFormat" is not in error state\n"))
#define MSG_JOB_NOERRORSTATEJOB_UU    _MESSAGE(33382, _("Job "U32CFormat" is not in error state\n"))
#define MSG_QUEUE_NORESCHEDULEQPERMS_SS  _MESSAGE(33391, _(SFN" - you have no permission to reschedule jobs of queue instance "SFQ"\n"))
#define MSG_QUEUE_NOCLEANQPERMS       _MESSAGE(33407, _("cleaning a queue requires manager privileges\n"))
#define MSG_QUEUE_CLEANQ_SSS          _MESSAGE(33408, _(SFN"@"SFN" cleaned queue "SFQ"\n"))
#define MSG_JOB_NOFORCESUSPENDTASK_SUU     _MESSAGE(33409, _(SFN" - can't force suspension job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_NOFORCESUSPENDJOB_SU       _MESSAGE(33410, _(SFN" - can't force suspension job "U32CFormat"\n"))
#define MSG_JOB_FORCESUSPENDTASK_SUU  _MESSAGE(33411, _(SFN" - forced suspension of job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_FORCESUSPENDJOB_SU    _MESSAGE(33412, _(SFN" - forced suspension of job "U32CFormat"\n"))
#define MSG_JOB_ALREADYSUSPENDED_SUU  _MESSAGE(33413, _(SFN" - job-array task "U32CFormat"."U32CFormat" is already suspended\n"))
#define MSG_JOB_ALREADYSUSPENDED_SU   _MESSAGE(33414, _(SFN" - job "U32CFormat" is already suspended\n"))
#define MSG_JOB_ALREADYUNSUSPENDED_SUU  _MESSAGE(33415, _(SFN" - job-array task "U32CFormat"."U32CFormat" is already unsuspended\n"))
#define MSG_JOB_ALREADYUNSUSPENDED_SU _MESSAGE(33416, _(SFN" - job "U32CFormat" is already unsuspended\n"))
#define MSG_JOB_NOSUSPENDTASK_SUU     _MESSAGE(33417, _(SFN" - can't suspend job-array task "U32CFormat"."U32CFormat" \n"))
#define MSG_JOB_NOSUSPENDJOB_SU       _MESSAGE(33418, _(SFN" - can't suspend job "U32CFormat" \n"))
#define MSG_JOB_NOUNSUSPENDTASK_SUU   _MESSAGE(33419, _(SFN" - can't unsuspend job-array task "U32CFormat"."U32CFormat" \n"))
#define MSG_JOB_NOUNSUSPENDJOB_SU     _MESSAGE(33420, _(SFN" - can't unsuspend job "U32CFormat" \n"))
#define MSG_JOB_SUSPENDTASK_SUU       _MESSAGE(33421, _(SFN" - suspended job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_SUSPENDJOB_SU         _MESSAGE(33422, _(SFN" - suspended job " U32CFormat "\n"))
/* #define MSG_JOB_RESCHEDULEJOB_SU      _message(33423, _(SFN" - reschedule job " U32CFormat "\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_JOB_UNSUSPENDTASK_SUU     _MESSAGE(33424, _(SFN" - unsuspended job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_UNSUSPENDJOB_SU       _MESSAGE(33425, _(SFN" - unsuspended job " U32CFormat "\n"))
#define MSG_JOB_RMADMSUSPENDTASK_SSUU _MESSAGE(33426, _(SFN"@"SFN" removed administrator suspension of job-array task "U32CFormat"."U32CFormat" (suspend threshold is still active)\n"))
#define MSG_JOB_RMADMSUSPENDJOB_SSU   _MESSAGE(33427, _(SFN"@"SFN" removed administrator suspension of job "U32CFormat" (suspend threshold is still active)\n"))
#define MSG_JOB_NOADMSUSPENDTASK_SUU  _MESSAGE(33428, _(SFN" - job-array task "U32CFormat"."U32CFormat" is not suspended by administrator - modify suspend threshold list of queue to remove suspend state\n"))
#define MSG_JOB_NOADMSUSPENDJOB_SU    _MESSAGE(33429, _(SFN" - job "U32CFormat" is not suspended by administrator - modify suspend threshold list of queue to remove suspend state\n"))
#define MSG_JOB_NOFORCEENABLETASK_SUU _MESSAGE(33430, _(SFN" - can't force enabling of job-array task "U32CFormat"."U32CFormat "\"\n"))
#define MSG_JOB_NOFORCEENABLEJOB_SU   _MESSAGE(33431, _(SFN" - can't force enabling of job "U32CFormat"\n"))
#define MSG_JOB_FORCEENABLETASK_SUU   _MESSAGE(33432, _(SFN" - forced enabling of job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_FORCEENABLEJOB_SU     _MESSAGE(33433, _(SFN" - forced enabling of job "U32CFormat"\n"))
#define MSG_JOB_FORCEUNSUSPTASK_SSUU  _MESSAGE(33434, _(SFN"@"SFN" forced unsuspension of job-array task "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_FORCEUNSUSPJOB_SSU    _MESSAGE(33435, _(SFN"@"SFN" forced unsuspension of job "U32CFormat"\n"))
#define MSG_EVE_RESENTSIGNALTASK_UU   _MESSAGE(33436, _("got resend signal timer event for unknown array task "U32CFormat"."U32CFormat"\n") ) 
#define MSG_EVE_RESENTSIGNALQ_S       _MESSAGE(33437, _("got resend signal timer event for unknown queue "SFN"\n"))
#define MSG_COM_NOUPDATEQSTATE_IS     _MESSAGE(33438, _("can't update remote queue state (%d) on queue "SFQ))
/*
** sge_queue_qmaster.c
*/
#define MSG_OBJ_QUEUE                 _MESSAGE(33457, _("queue"))

/*
** sge_sharetree_qmaster.c
*/
#define MSG_STREE_ADDSTREE_SSII       _MESSAGE(33474, _(SFN"@"SFN" has added sharetree with %d nodes and %d leafs\n"))
#define MSG_STREE_MODSTREE_SSII       _MESSAGE(33475, _(SFN"@"SFN" has modified sharetree with %d nodes and %d leafs\n"))
#define MSG_STREE_PRJINPTJSUBTREE_SS  _MESSAGE(33476, _("found project "SFQ" in project "SFQ" sub-tree of share tree\n"))
#define MSG_STREE_PRJTWICE_S          _MESSAGE(33477, _("found project "SFQ" twice in share tree\n"))
#define MSG_STREE_USERNONLEAF_S       _MESSAGE(33478, _("found user "SFQ" in share tree as a non-leaf node\n"))
#define MSG_STREE_USERTWICEINPRJSUBTREE_SS _MESSAGE(33479, _("found user "SFQ" twice in project "SFQ" sub-tree of share tree\n"))
#define MSG_STREE_USERTNOACCESS2PRJ_SS   _MESSAGE(33480, _("found user "SFQ" with no access to project "SFQ" sub-tree of share tree\n"))
#define MSG_STREE_USERPRJTWICE_SS     _MESSAGE(33481, _("found "SFN" "SFQ" twice in share tree\n"))
#define MSG_STREE_QMASTERSORCETREE_SS _MESSAGE(33482, _("tree of qmaster has "SFN"node while source tree has "SFN"node\n"))
#define MSG_STREE_NOPLUSSPACE         _MESSAGE(33483, _("no "))
#define MSG_STREE_VERSIONMISMATCH_II  _MESSAGE(33484, _("trees have different versions: qmaster=%d src=%d\n") ) 
#define MSG_STREE_MISSINGNODE_S       _MESSAGE(33485, _("missing node "SFQ" in dst trees\n"))
#define MSG_SGETEXT_DOESNOTEXIST_S              _MESSAGE(33486, _(""SFN" does not exist\n"))
#define MSG_SGETEXT_REMOVEDLIST_SSS         _MESSAGE(33487, _(""SFN"@"SFN" removed "SFN" list\n"))
#define MSG_SGETEXT_FOUND_UP_TWICE_SS                 _MESSAGE(33488, _("denied: found node "SFQ" twice under node "SFQ"\n") )     
#define MSG_SGETEXT_UNKNOWN_SHARE_TREE_REF_TO_SS      _MESSAGE(33489, _("denied: share tree contains reference to unknown "SFN" "SFQ"\n") )  


/*
** sge_userprj_qmaster.c
*/
#define MSG_USERPRJ_PRJXSTILLREFERENCEDINENTRYX_SS _MESSAGE(33490, _("project "SFQ" is still referenced by user "SFQ"\n"))
#define MSG_UP_NOADDDEFAULT_S          _MESSAGE(33491, _("denied: not allowed add a "SFN" with name \"default\"\n") ) 
#define MSG_UP_ALREADYEXISTS_SS        _MESSAGE(33492, _("denied: shared namespace between project and user: there is already a "SFN" which is named "SFQ"\n"))
#define MSG_UM_CLUSTERUSERXNOTGUILTY_S _MESSAGE(33493, _("cluster user name "SFQ" is not valid\n"))
#define MSG_OBJ_PRJ                    _MESSAGE(33509, _("project"))
#define MSG_OBJ_PRJS                  _MESSAGE(33510, _("projects"))
#define MSG_OBJ_XPRJS                 _MESSAGE(33511, _("xprojects"))
#define MSG_OBJ_EH                    _MESSAGE(33512, _("exechost"))
#define MSG_OBJ_CONF                  _MESSAGE(33513, _("configuration"))
#define MSG_OBJ_GLOBAL                _MESSAGE(33514, _("global"))
/* #define MSG_JOB_CREDMOD_SSF           _message(33516, _(SFN"@"SFN" modified credit to %.2f\n")) __TS Removed automatically from testsuite!! TS__*/
/* #define MSG_JOB_CREDMODLOW_SSF        _message(33517, _(SFN"@"SFN" modified low_credit to %.2f\n")) __TS Removed automatically from testsuite!! TS__*/
#define MSG_SGETEXT_CANT_DELETE_UP_IN_SHARE_TREE_SS   _MESSAGE(33518, _("denied: may not remove "SFN" "SFQ" still referenced in share tree\n") ) 
#define MSG_SGETEXT_PROJECTSTILLREFERENCED_SSSS       _MESSAGE(33519, _("denied: project "SFQ" is still referenced in "SFN" of "SFN" "SFQ"\n") )   
#define MSG_SGETEXT_UNKNOWNPROJECT_SSSS               _MESSAGE(33520, _("denied: project "SFQ" referenced in "SFN" of "SFN" "SFQ" does not exist\n") )  


/*
** sge_userset_qmaster.c
*/
#define MSG_SGETEXT_NO_DEPARTMENT4USER_SS             _MESSAGE(33522, _("denied: no matching department for user "SFQ" or group "SFQ"\n") )  
#define MSG_SGETEXT_USERSETSTILLREFERENCED_SSSS       _MESSAGE(33524, _("denied: userset "SFQ" is still referenced in "SFN" of "SFN" "SFQ"\n") ) 


/*
** complex_qmaster.c
*/
#define MSG_NONAME                    _MESSAGE(33533, _("<noname>"))
#define MSG_OBJ_CPLX                  _MESSAGE(33534, _("complex"))

/*
** configuration_qmaster.c
*/
#define MSG_SGETEXT_CANT_DEL_CONFIG_S           _MESSAGE(33541, _("can't delete configuration "SFQ" from list\n"))
#define MSG_CONF_CANTMERGECONFIGURATIONFORHOST_S _MESSAGE(33546, _("can't merge configuration for host "SFQ" \n"))
#define MSG_CONF_CHANGEPARAMETERXONLYSUPONSHUTDOWN_S _MESSAGE(33547, _("Changing parameter "SFQ" only supported in a shut-down cluster.\n"))
#define MSG_CONF_NAMEISNULLINCONFIGURATIONLISTOFX_S _MESSAGE(33548, _("name == NULL in configuration list of "SFQ"\n"))
#define MSG_CONF_VALUEISNULLFORATTRXINCONFIGURATIONLISTOFY_SS _MESSAGE(33549, _("value == NULL for attribute "SFQ" in configuration list of "SFQ"\n"))
#define MSG_CONF_GOTINVALIDVALUEXFORLOGLEVEL_S _MESSAGE(33550, _("denied: got invalid value "SFQ" for loglevel\n"))
#define MSG_CONF_GOTINVALIDVALUEXFORSHELLSTARTMODE_S _MESSAGE(33551, _("denied: got invalid value "SFQ" for shell_start_mode\n"))
#define MSG_CONF_GOTINVALIDVALUEXASADMINUSER_S _MESSAGE(33552, _("denied: got invalid value "SFQ" as admin_user\n"))
#define MSG_CONF_PARAMETERXINCONFIGURATION_SS _MESSAGE(33553, _("denied: parameter "SFQ" in configuration: "SFQ"\n"))
#define MSG_CONF_THEPATHGIVENFORXMUSTSTARTWITHANY_S _MESSAGE(33554, _("denied: the path given for "SFQ" must start with an \"/\"\n"))
#define MSG_CONF_INFNOTALLOWEDFORATTRXINCONFLISTOFY_SS _MESSAGE(33555, _("infinity not allowed for attribute "SFQ" in configuration list of "SFQ"\n"))
#define MSG_CONF_FORMATERRORFORXINYCONFIG_SS _MESSAGE(33556, _("format error for "SFQ" in "SFQ" configuration\n"))


/*
** job_exit.c
*/
#define MSG_OBJ_UNKNOWNQ              _MESSAGE(33557, _("<unknown queue>"))
#define MSG_OBJ_UNKNOWNHOST           _MESSAGE(33558, _("<unknown host>"))
#define MSG_JOB_WRITEJFINISH_S        _MESSAGE(33559, _("writing job finish information: can't locate queue "SFQ"\n"))
#define MSG_JOB_JFINISH_UUS           _MESSAGE(33560, _("job "U32CFormat"."U32CFormat" finished on host "SFN))
#define MSG_JOB_FAILEDONHOST_UUSSSS   _MESSAGE(33561, _("job "U32CFormat"."U32CFormat" failed on host "SFN" "SFN SFN" because: %-.512s"))
#define MSG_GENERAL                   _MESSAGE(33562, _("general "))
#define MSG_JOB_JEXITNOTRUN_UU        _MESSAGE(33563, _("received JOB_EXIT for job "U32CFormat"."U32CFormat" which ist NOT running\n"))
#define MSG_LOG_JREMOVED              _MESSAGE(33564, _("job removed "))
#define MSG_LOG_JERRORSET             _MESSAGE(33565, _("job set in error state "))
#define MSG_LOG_JNOSTARTRESCHEDULE    _MESSAGE(33566, _("job never ran -> schedule it again"))
#define MSG_LOG_JRERUNRESCHEDULE      _MESSAGE(33567, _("job rerun/checkpoint specified -> schedule it again"))
#define MSG_LOG_JCKPTRESCHEDULE       _MESSAGE(33568, _("job was checkpointed -> schedule it again"))
#define MSG_LOG_JNORESRESCHEDULE      _MESSAGE(33569, _("job didn't get resources -> schedule it again"))
#define MSG_LOG_QERRORBYJOB_SU        _MESSAGE(33570, _("queue "SFN" marked QERROR as result of job "U32CFormat"'s failure\n") ) 
#define MSG_LOG_QERRORBYJOBHOST_SUS   _MESSAGE(33571, _("queue "SFN" marked QERROR as result of job "U32CFormat"'s failure at host "SFN"\n"))


/*
** job_report_qmaster.c
*/
#define MSG_JOB_REPORTEXITQ_SUUSSSSS   _MESSAGE(33574, _("execd "SFN" reports exiting job ("U32CFormat"."U32CFormat"/"SFN" in queue "SFQ" that was supposed to be in queue "SFQ" at "SFQ" (state = "SFN")"))
#define MSG_JOB_REPORTRUNQ_SUUSSU      _MESSAGE(33575, _("execd "SFN" reports running state for job ("U32CFormat"."U32CFormat"/"SFN") in queue "SFQ" while job is in state "U32CFormat" "))
#define MSG_JOB_REPORTRUNFALSE_SUUSS   _MESSAGE(33576, _("execd@"SFN" reports running job ("U32CFormat"."U32CFormat"/"SFN") in queue "SFQ" that was not supposed to be there - killing"))
#define MSG_JOB_REPORTEXITJ_UUU        _MESSAGE(33577, _("JEXITING report for job "U32CFormat"."U32CFormat": which is in status "U32CFormat"\n"))
#define MSG_JOB_FILESIZEEXCEED_SSUU    _MESSAGE(33578, _("file size resource limit exceeded by task "SFN" at "SFN" of job "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_CPULIMEXCEED_SSUU      _MESSAGE(33579, _("cpu time resource limit exceeded by task "SFN" at "SFN" of job "U32CFormat"."U32CFormat"\n"))
#define MSG_JOB_DIEDTHROUGHSIG_SSUUS   _MESSAGE(33580, _("task "SFN" at "SFN" of job "U32CFormat"."U32CFormat" died through signal "SFN"\n"))
#define MSG_JOB_TASKFAILED_SSUUU       _MESSAGE(33581, _("task "SFN" at "SFN" of job "U32CFormat"."U32CFormat" failed "U32CFormat"\n"))
#define MSG_JOB_TASKFINISHED_SSUU      _MESSAGE(33582, _("task "SFN" at "SFN" of job "U32CFormat"."U32CFormat" finished\n"))
#define MSG_JOB_JOBTASKFAILED_S       _MESSAGE(33583, _("tightly integrated parallel task "SFN" failed - killing job\n"))
#define MSG_OBJ_NOTRUNNING             _MESSAGE(33584, _("<not running>"))
#define MSG_EXECD_UNKNOWNJ_SUUSUS      _MESSAGE(33585, _("execd "SFN" reports unknown job ("U32CFormat"."U32CFormat"/"SFN") with unknown state "U32CFormat" in queue "SFQ))


/*
** sge_qmaster_main.c
*/
#define MSG_STARTUP_BEGINWITHSTARTUP              _MESSAGE(33590, _("begin with start up"))
#define MSG_STARTUP_SETUPFAILED              _MESSAGE(33591, _("setup failed"))
#define MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI           _MESSAGE(33594, _("Failed in sge_unpack_gdi_request ("SFN"/"SFN"/%d)\n"))
#define MSG_CULL_FAILEDINCULLUNPACKLISTREPORT              _MESSAGE(33595, _("Failed in cull_unpack_list report\n"))
#define MSG_SHUTDOWN_SHUTTINGDOWNQMASTERREQUIRESMANAGERPRIVILEGES              _MESSAGE(33596, _("shutting down qmaster requires manager privileges\n"))
#define MSG_GOTSTATUSREPORTOFUNKNOWNCOMMPROC_S    _MESSAGE(33597, _("got load report of unknown commproc "SFQ"\n"))
#define MSG_GOTSTATUSREPORTOFUNKNOWNEXECHOST_S    _MESSAGE(33598, _("got load report of unknown exec host "SFQ"\n"))
#define MSG_CONF_CANTNOTIFYEXECHOSTXOFNEWCONF_S    _MESSAGE(33599, _("can't notify exec host "SFQ" of new conf\n"))
#define MSG_LICENCE_ERRORXUPDATINGLICENSEDATA_I    _MESSAGE(33600, _("error %d updating license data\n"))
#define MSG_HEARTBEAT_FAILEDTOINCREMENTHEARBEATFILEXINSPOOLDIR_S    _MESSAGE(33601, _("failed to increment heartbeat file "SFQ" in spool directory"))
#define MSG_QMASTER_LOCKFILE_ALREADY_EXISTS  _MESSAGE(33602, _("Unable to create lock file. Found existing one."))

/*
** qmaster_to_execd.c
*/
#define MSG_NOXKNOWNONHOSTYTOSENDCONFNOTIFICATION_SS  _MESSAGE(33603, _("no "SFN" known on host "SFN" to send conf notification\n"))

/*
** setup_qmaster.c
*/
#define MSG_SETUP_SETUPMAYBECALLEDONLYATSTARTUP       _MESSAGE(33615, _("setup may be called only at startup"))
#define MSG_CONFIG_ERRORXMERGINGCONFIGURATIONY_IS     _MESSAGE(33618, _("Error %d merging configuration "SFQ"\n"))
#define MSG_CONFIG_ADDINGHOSTTEMPLATETOEXECHOSTLIST   _MESSAGE(33619, _("adding host template to exechost_list\n"))
#define MSG_CONFIG_ADDINGHOSTGLOBALTOEXECHOSTLIST     _MESSAGE(33620, _("adding host global to exechost_list\n"))
#define MSG_CONFIG_CANTWRITEMANAGERLIST               _MESSAGE(33621, _("can't write manager list\n"))
#define MSG_CONFIG_CANTWRITEOPERATORLIST              _MESSAGE(33622, _("can't write operator list\n"))
#define MSG_CONFIG_NOLOCAL_S                          _MESSAGE(33623, _("local configuration "SFN" not defined - using global configuration"))
#define MSG_CONFIG_NOGLOBAL                           _MESSAGE(33624, _("global configuration not defined"))
#define MSG_CONFIG_CANTFINDQUEUEXREFERENCEDINJOBY_SU  _MESSAGE(33648, _("can't find queue "SFQ" referenced in job "U32CFormat))

/*
** sge_qmaster_timed_event.c
*/
#define MSG_SYSTEM_SYSTEMHASBEENMODIFIEDXSECONDS_I       _MESSAGE(33663, _("system clock has been put back (%d seconds)\n"))
#define MSG_SYSTEM_RECEIVEDUNKNOWNEVENT_I       _MESSAGE(33664, _("received unkown event: %d\n"))

/*
 * misc
 */
#define MSG_SEC_CRED_SSSI                          _MESSAGE(33673, _("denied: request for user "SFQ" does not match credentials for connection <"SFN","SFN",%d>\n") )  
#define MSG_JOB_STDINPATHLIST                      _MESSAGE(33676, _("stdin path list"))
#define MSG_QMASTER_AUTODEFDEPARTMENT              _MESSAGE(33677, _("all users are assigned to the \"defaultdepartment\" automatically\n"))
#define MSG_QMASTER_DEPTFORDEFDEPARTMENT           _MESSAGE(33678, _("the \"defaultdepartment\" has to be of type \"DEPT\"\n"))
#define MSG_QMASTER_ACLNOSHARE                     _MESSAGE(33679, _("not allowed to set \"fshare\" for ACL lists\n"))
#define MSG_QMASTER_ACLNOTICKET                    _MESSAGE(33680, _("not allowed to set \"oticket\" for ACL lists\n"))
#define MSG_SUSERCNTISALREADYZERO_S                _MESSAGE(33681, _("Job counter of user "SFQ" is already 0\n"))

/*
 */
#define MSG_HGRP_NONAMECHANGE    _MESSAGE(33682, _("unable to change hostgroup name\n"))
#define MSG_UME_NONAMECHANGE     _MESSAGE(33683, _("unable to change user mapping name\n"))

#define MSG_QMODJOB_NOTENROLLED_UU _MESSAGE(33684, _("Modify operation can not be applied on job-array task " U32CFormat"."U32CFormat " in pending/hold state\n"))
#define MSG_QMODJOB_NOTENROLLED_U  _MESSAGE(33685, _("Modify operation can not be applied on job "U32CFormat " in pending/hold state\n"))

#define MSG_JOB_MASTERTASKFAILED_S       _MESSAGE(33686, _("master task of job "SFN" failed - killing job\n"))

#define MSG_CQUEUE_NAMENOTGUILTY_S       _MESSAGE(33687, _("cluster queue name "SFQ" is not valid\n"))
#define MSG_CQUEUE_NONAMECHANGE          _MESSAGE(33688, _("unable to change cluster queue name\n"))
#define MSG_HGROUP_CYCLEINDEF_SS        _MESSAGE(33690, _("Hostgroup "SFQ" in specification of "SFQ" would create a cycle\n"))
#define MSG_HGROUP_REFINHGOUP_SS        _MESSAGE(33691, _("denied: following hostgroups still reference "SFQ": "SFN"\n"))
#define MSG_HGROUP_REFINCUSER_SS        _MESSAGE(33692, _("denied: following user mapping entries still reference "SFQ": "SFN"\n"))

#define MSG_QINSTANCE_NOSSOS_S          _MESSAGE(33694, _("Subordinate suspension prevents unsuspension due to calendar for queue "SFQ"\n"))
#define MSG_QINSTANCE_NOSADM_S          _MESSAGE(33695, _("Administrator suspension prevents unsuspension due to calendar for queue "SFQ"\n"))
#define MSG_QINSTANCE_NOUSSOS_S         _MESSAGE(33696, _("no need to suspend queue "SFQ" it's already suspended on subordinate\n"))
#define MSG_QINSTANCE_NOUSADM_S         _MESSAGE(33697, _("no need to suspend queue "SFQ" it's already suspended by administrator\n"))
#define MSG_QINSTANCE_STATENOTMOD_S     _MESSAGE(33720, _("Queue instance state of "SFQ" not modified: Spooling framework failed\n"))
#define MSG_QINSTANCE_STATENOTMODPERM_S _MESSAGE(33721, _("Queue instance state of "SFQ" not modified: No permission\n") ) 
#define MSG_QINSTANCE_HASSTATE_SS       _MESSAGE(33722, _("Queue instance "SFQ" is already in the specified state: "SFN"\n"))
#define MSG_QINSTANCE_FORCEDSTATE_SSSS  _MESSAGE(33723, _(SFN"@"SFN" forced state change of "SFQ" ("SFN")\n"))
#define MSG_QINSTANCE_CHANGEDST_SSSS    _MESSAGE(33724, _(SFN"@"SFN" changed state of "SFQ" ("SFN")\n"))
#define MSG_QINSTANCE_QIALREADYHERE_S   _MESSAGE(33725, _("Should create queue instance "SFQ" which is already here\n"))
#define MSG_QINSTANCE_NQIFOUND_SS       _MESSAGE(33726, _("queue instance "SFQ" not found in "SFQ"\n"))
#define MSG_JOB_WILD_RANGE_AMBIGUOUS    _MESSAGE(33727, _("job rejected: PEs matching wildcard and jobs slot range would cause ambiguous urgency slot amount\n"))
#define MSG_ATTR_HASAMBVAL_SSS          _MESSAGE(33728, _("warning: "SFQ" has ambiguous value ("SFQ", "SFQ")\n"))
#define MSG_CQUEUE_REFINHGOUP_SS        _MESSAGE(33729, _("denied: following cluster queues still reference "SFQ": "SFN"\n"))
#define MSG_LOG_DELETED               _MESSAGE(33800, _("job deleted"))
#define MSG_LOG_SENT2EXECD            _MESSAGE(33801, _("sent to execd"))
#define MSG_LOG_DELIVERED             _MESSAGE(33802, _("job received by execd"))
#define MSG_LOG_EXITED                _MESSAGE(33803, _("job exited"))
#define MSG_LOG_WAIT4SGEDEL           _MESSAGE(33804, _("job waits for schedds deletion"))
#define MSG_LOG_DELSGE                _MESSAGE(33805, _("job deleted by schedd"))
#define MSG_LOG_DELIMMEDIATE          _MESSAGE(33806, _("immediate job deleted by schedd"))
#define MSG_QMASTER_UNEXPECTED_SIGNAL_I    _MESSAGE(33810, _("received unexpected signal %d\n"))
#define MSG_JOB_DEADLINETIME               _MESSAGE(33811, _("deadline time"))
#define MSG_COM_NOSCHEDMONPERMS   _MESSAGE(33815, _("starting scheduler monitoring requires manager privileges\n"))
#define MSG_COM_NOSCHEDDREGMASTER _MESSAGE(33816, _("no scheduler registered at qmaster\n"))
#define MSG_COM_SCHEDMON_SS       _MESSAGE(33817, _(SFN"@"SFN" triggers scheduler monitoring\n"))
#define MSG_QINSTANCE_STILLJOBS   _MESSAGE(33818, _("There are still running jobs in the queue. Deletion denied.\n"))

/*
 * sge_persistence_qmaster.c
 */

#define MSG_PERSISTENCE_WRITE_FAILED_S _MESSAGE(33820, _("error writing object "SFQ" to spooling database\n"))
#define MSG_PERSISTENCE_DELETE_FAILED_S _MESSAGE(33821, _("error deleting object "SFQ" from spooling database\n"))
#define MSG_PERSISTENCE_OPENTRANSACTION_FAILED _MESSAGE(33822, _("error starting a transaction in the spooling database\n"))
#define MSG_PERSISTENCE_CLOSINGTRANSACTION_FAILED _MESSAGE(33823, _("error closing a transaction in the spooling database\n"))

/* 
 * sge_reporting_qmaster.c
 */
#define MSG_JOBLOG_ACTION_UNKNOWN      _MESSAGE(33850, _("unknown"))
#define MSG_JOBLOG_ACTION_PENDING      _MESSAGE(33851, _("pending"))
#define MSG_JOBLOG_ACTION_SENT         _MESSAGE(33852, _("sent"))
#define MSG_JOBLOG_ACTION_RESENT       _MESSAGE(33853, _("resent"))
#define MSG_JOBLOG_ACTION_DELIVERED    _MESSAGE(33854, _("delivered"))
#define MSG_JOBLOG_ACTION_RUNNING      _MESSAGE(33855, _("running"))
#define MSG_JOBLOG_ACTION_SUSPENDED    _MESSAGE(33856, _("suspended"))
#define MSG_JOBLOG_ACTION_UNSUSPENDED  _MESSAGE(33857, _("unsuspended"))
#define MSG_JOBLOG_ACTION_HELD         _MESSAGE(33858, _("held"))
#define MSG_JOBLOG_ACTION_RELEASED     _MESSAGE(33859, _("released"))
#define MSG_JOBLOG_ACTION_RESTART      _MESSAGE(33860, _("restart"))
#define MSG_JOBLOG_ACTION_MIGRATE      _MESSAGE(33861, _("migrate"))
#define MSG_JOBLOG_ACTION_DELETED      _MESSAGE(33862, _("deleted"))
#define MSG_JOBLOG_ACTION_FINISHED     _MESSAGE(33863, _("finished"))
#define MSG_JOBLOG_ACTION_ERROR        _MESSAGE(33864, _("error"))

#define MSG_REPORTING_INTERMEDIATE_SS  _MESSAGE(33870, _("write intermediate accounting record for job "SFQ" at "SFN"\n"))

/*
 * other
 */
#define MSG_JOB_CHANGEJOBSHARE            _MESSAGE(33900, _("change job share"))
#define MSG_JOB_PRIOSET_SSUU              _MESSAGE(33901, _(SFN"@"SFN" sets scheduling priority of job "U32CFormat" to "U32CFormat"\n"))

#define MSG_JOB_PERANGE_ONLY_FOR_PARALLEL _MESSAGE(33902, _("rejected: change request for PE range supported only for parallel jobs\n"))

#define MSG_QMASTER_MAX_FILE_DESCRIPTORS_LIMIT_U _MESSAGE(33903, _("qmaster will use max. "U32CFormat" file descriptors for communication\n"))

#define MSG_OBJ_MAXUNHEARDVALUE_SS    _MESSAGE(33904, _("host "SFQ": "SFQ" is not a valid time value for \"max_unheard\" - assuming 120 seconds\n") )  
#define MSG_QMASTER_MAX_EVC_LIMIT_U              _MESSAGE(33905, _("qmaster will accept max. "U32CFormat" dynamic event clients\n"))
#define MSG_QMASTER_COMMUNICATION_ERRORS  _MESSAGE(33906, _("abort qmaster startup due to communication errors\n"))

#define MSG_QMASTER_APPL_STATE_OK                _MESSAGE(33907, _("ok"))
#define MSG_QMASTER_APPL_STATE_TIMEOUT_WARNING   _MESSAGE(33908, _("thread timeout warning"))
#define MSG_QMASTER_APPL_STATE_TIMEOUT_ERROR     _MESSAGE(33909, _("thread timeout error"))
#define MSG_QMASTER_APPL_STATE_CFCFCFCFS         _MESSAGE(33910, _("EDT: %c (%.2f) | TET: %c (%.2f) | MT: %c (%.2f) | SIGT: %c (%.2f) | %s"))

#endif

