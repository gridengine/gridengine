#ifndef __MSG_GDILIB_H
#define __MSG_GDILIB_H
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
** sge_security.c
*/
#define MSG_QSH_GET_CREDSTDERR_S _("get_cred stderr: "SFN"")
#define MSG_QSH_QSUBFAILED    _("qsub failed\n")
#define MSG_QSUB_CANTSTARTCOMMANDXTOGETTOKENQSUBFAILED_S    _("can't start command "SFQ" to get token - qsub failed\n")
#define MSG_QSH_CANTSTARTCOMMANDXTOGETCREDENTIALSQSUBFAILED_S    _("can't start command "SFQ" to get credentials - qsub failed\n")
#define MSG_QSH_CANTGETCREDENTIALS    _("warning: could not get credentials\n")
#define MSG_SEC_NOCRED_USSI           _("could not get credentials for job " U32CFormat " for execution host "SFN" - command "SFQ" failed with return code %d\n")
#define MSG_SEC_STARTDELCREDCMD_SU    _("can't start command "SFQ" for job " U32CFormat " to delete credentials\n")
#define MSG_SEC_NOAUTH_U              _("job "U32CFormat" rejected because authentication failed\n")
#define MSG_SEC_NOSTORECRED_USI       _("could not store credentials for job " U32CFormat" - command "SFQ" failed with return code %d\n")
#define MSG_SEC_NOSTORECREDNOBIN_US   _("could not store client credentials for job " U32CFormat" - "SFN" binary does not exist\n")
#define MSG_SEC_DELCREDSTDERR_S       _("delete_cred stderr: "SFN"")
#define MSG_SEC_DELCREDRETCODE_USI    _("could not delete credentials for job " U32CFormat" - command "SFQ" failed with return code %d\n")
#define MSG_SEC_DELCREDNOBIN_US       _("could not delete credentials for job "U32CFormat" - "SFN" binary does not exist\n")
#define MSG_SEC_KRBAUTHFAILURE        _("job "U32CFormat" rejected because authentication failed\n")
#define MSG_SEC_KRBAUTHFAILUREONHOST  _("job "U32CFormat" rejected because authentication failed on host "SFN"\n")
#define MSG_SEC_PUTCREDSTDERR_S       _("put_cred stderr: "SFN"")
#define MSG_SEC_NOSTARTCMD4GETCRED_SU _("can't start command "SFQ" for job " U32CFormat " to get credentials\n")
#define MSG_SEC_NOCREDNOBIN_US        _("could not get client credentials for job " U32CFormat" - "SFN" binary does not exist\n")
#define MSG_SEC_KRB_CRED_SSSI         _("denied: request for user "SFQ" does not match Kerberos credentials for connection <"SFN","SFN",%d>\n")         
#define MSG_SEC_KRBDECRYPTTGT_US      _("could not decrypt TGT for job " U32CFormat "- "SFN"\n")
#define MSG_SEC_KRBENCRYPTTGT_SSIS    _("could not encrypt TGT for client <"SFN","SFN",%d> - "SFN"\n")
#define MSG_SEC_KRBENCRYPTTGTUSER_SUS _("could not encrypt TGT for user "SFN", job "U32CFormat" - "SFN"")
#define MSG_SEC_NOUID_SU              _("could not get user ID for "SFN", job "U32CFormat"\n")


/*
** sge_resource.c
*/
#define MSG_PARSE_NOALLOCREQRES       _("unable to alloc space for requested resources\n")
#define MSG_PARSE_NOALLOCATTRLIST     _("unable to alloc space for attrib. list\n")
#define MSG_PARSE_NOALLOCRESELEM      _("unable to alloc space for resource element\n")
#define MSG_PARSE_NOVALIDSLOTRANGE_S  _(""SFQ" must be a valid slot range\n")
#define MSG_PARSE_NOALLOCATTRELEM     _("unable to alloc space for attrib. element\n")


/*
** sge_parse_date_time.c
*/
#define MSG_PARSE_STARTTIMETOOLONG    _("Starttime specifier field length exceeds maximum")
#define MSG_PARSE_INVALIDSECONDS      _("Invalid format of seconds field.")
#define MSG_PARSE_INVALIDHOURMIN      _("Invalid format of date/hour-minute field.")
#define MSG_PARSE_INVALIDMONTH        _("Invalid month specification.")
#define MSG_PARSE_INVALIDDAY          _("Invalid day specification.")
#define MSG_PARSE_INVALIDHOUR         _("Invalid hour specification.")
#define MSG_PARSE_INVALIDMINUTE       _("Invalid minute specification.")
#define MSG_PARSE_INVALIDSECOND       _("Invalid seconds specification.")
#define MSG_PARSE_NODATEFROMINPUT     _("Couldn't generate date from input. Perhaps a date before 1970 was specified.")

/*
** parse.c
*/
/* #define MSG_PARSE_XISNOTAVALIDOPTION_S      _(""SFQ" is not a valid option\n") */
/* #define MSG_PARSE_NOOPTIONARGPROVIDEDTOX_S  _("no option argument provided to "SFQ"") */
/* #define MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S _("ERROR! "SFN" option must have argument\n") */
#define MSG_JOB_XISINVALIDJOBTASKID_S       _("ERROR! "SFN" is a invalid job-task identifier\n")





/* 
** gdilib/gdi_qmod.c
*/ 
#define MSG_GDI_INVALIDACTION          _("invalid action" )
#define MSG_GDI_INVALIDOPTIONFLAG      _("invalid option flag")
#define MSG_GDI_INVALIDIDENCOUNTERED   _("encountered invalid id")
#define MSG_GDI_OUTOFMEMORY            _("out of memory")

/* 
** gdilib/sge_any_request.c
*/ 
#define MSG_GDI_INITSECURITYDATAFAILED                _("failed initialize security data\n")
#define MSG_GDI_INITKERBEROSSECURITYDATAFAILED        _("failed initialize Kerberos security data\n")
#define MSG_GDI_ENROLLTOCOMMDFAILED_S                 _("can't enroll to commd: "SFN"\n")
#define MSG_GDI_COMMDUP                               _("commd is up")
#define MSG_GDI_COMMDDOWN_S                           _("commd is down: "SFN"")
#define MSG_GDI_RHOSTISNULLFORSENDREQUEST             _("parameter rhost = NULL for sge_send_any_request()")
#define MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS     _("can't send "SFN"synchronous message to commproc ("SFN":%d) on host "SFQ": "SFN"")
#define MSG_GDI_RHOSTISNULLFORGETANYREQUEST           _("parameter rhost = NULL for sge_get_any_request()")
#define MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS _("can't receive message from commproc ("SFN":%d) on host "SFQ": "SFN"")

/* 
** gdilib/exec_wrapper.c
*/ 
#define MSG_GDI_READCONFIGFILESPECGIVENTWICE_SS         _("reading config file: specifier "SFQ" given twice for "SFQ"\n")


/* 
** gdilib/sge_parse_num_par.c
*/ 
#define MSG_GDI_VALUETHATCANBESETTOINF                  _("value that can be set to infinity")
#define MSG_GDI_READCONFIGFILESPECGIVENTWICE_SS         _("reading config file: specifier "SFQ" given twice for "SFQ"\n")
#define MSG_GDI_READCONFIGFILEUNKNOWNSPEC_SS            _("reading conf file: unknown specifier "SFQ" for "SFN"\n")
#define MSG_GDI_READCONFIGFILEEMPTYENUMERATION_S        _("reading conf file: empty enumeration for "SFQ"\n " )


/* 
** gdilib/sge_parse_num_val.c
*/ 
#define MSG_GDI_UNRECOGNIZEDVALUETRAILER_SS             _("Error! Unrecognized value-trailer '%20s' near '%20s'\nI expected multipliers k, K, m and M.\nThe value string is probably badly formed!\n" )
#define MSG_GDI_UNEXPECTEDENDOFNUMERICALVALUE_SC        _("Error! Unexpected end of numerical value near "SFN".\nExpected one of ',', '/' or '\\0'. Got '%c'\n" )
#define MSG_GDI_NUMERICALVALUEFORHOUREXCEEDED_SS         _("Error! numerical value near %20s for hour exceeded.\n'%20s' is no valid time specifier!\n")
#define MSG_GDI_NUMERICALVALUEINVALID_SS                _("Error! numerical value near %20s invalid.\n'%20s' is no valid time specifier!\n" )
#define MSG_GDI_NUMERICALVALUEFORMINUTEEXCEEDED_SS       _("Error! numerical value near %20s for minute exceeded.\n'%20s' is no valid time specifier!\n")
#define MSG_GDI_NUMERICALVALUEINVALIDNONUMBER_SS         _("Error! numerical value near %20s invalid.\n>%20s< contains no valid decimal or fixed float number\n")
#define MSG_GDI_NUMERICALVALUEINVALIDNOHEXOCTNUMBER_SS      _("Error! numerical value near "SFN" invalid.\n'"SFN"' contains no valid hex or octal number\n")



/* 
** gdilib/sge_qexec.c
*/
#define MSG_GDI_INVALIDPARAMETER_SS                      _("invalid paramter to "SFQ": "SFQ)
#define MSG_GDI_RESOLVINGUIDTOUSERNAMEFAILED_IS          _("failed resolving uid %d to username: "SFN"")
#define MSG_GDI_MISSINGINENVIRONMENT_S                   _("missing "SFQ" in environment")
#define MSG_GDI_STRINGISINVALID_SS                       _("string "SFQ" in env var "SFQ" is not a valid job/taskid")
#define MSG_GDI_PROPOSEDTASKIDINVALID_SS                 _("proposed task id "SFN" "SFQ" is invalid")
#define MSG_GDI_TASKEXISTS_S                             _("task "SFN" already exists")
#define MSG_GDI_SETUPGDILIBFAILED                        _("failed setting up gdi library")
#define MSG_GDI_SENDTASKTOEXECDFAILED_SS                 _("failed sending task to execd@"SFN": "SFN""                  )
#define MSG_GDI_TASKNOTEXIST_S                           _("task "SFQ" does not exist")
#define MSG_GDI_RCVFROMEXECLOCALCOMMDFAILED_S            _("rcv_from_exec(): failed enrolling to local sge commd: "SFN"")
#define MSG_GDI_MESSAGERECEIVEFAILED_SI                  _("failed receiving message from execd: "SFN" %d")
#define MSG_GDI_TASKNOTFOUND_S                           _("cannot find task with taskid "SFQ"\n")
#define MSG_GDI_TASKNOTFOUNDNOIDGIVEN_S                  _("cannot find task without taskid - should become task "SFQ"")



/* 
** gdilib/sge_report.c
*/
#define MSG_GDI_REPORTNOMEMORY_I                         _("not enough memory for packing report: %d bytes\n")
#define MSG_GDI_REPORTFORMATERROR                        _("format error while packing report\n")
#define MSG_GDI_REPORTUNKNOWERROR                        _("unexpected error while packing report\n")


/* 
** gdilib/config.c
*/
#define MSG_GDI_CONFIGNOARGUMENTGIVEN_S                  _("no argument given in config option: "SFN"\n")
#define MSG_GDI_CONFIGMISSINGARGUMENT_S                  _("missing configuration attribute "SFQ"")
#define MSG_GDI_CONFIGADDLISTFAILED_S                    _("can't add "SFQ" to list")
#define MSG_GDI_CONFIGARGUMENTNOTINTEGER_SS              _("value for attribute "SFN" "SFQ" is not an integer\n")
#define MSG_GDI_CONFIGARGUMENTNOTDOUBLE_SS               _("value for attribute "SFN" "SFQ" is not a double\n")
#define MSG_GDI_CONFIGARGUMENTNOTTIME_SS                 _("value for attribute "SFN" "SFQ" is not time\n")
#define MSG_GDI_CONFIGARGUMENTNOMEMORY_SS                _("value for attribute "SFN" "SFQ" is not memory\n")
#define MSG_GDI_CONFIGINVALIDQUEUESPECIFIED              _("reading conf file: invalid queue type specified\n")
#define MSG_GDI_CONFIGREADFILEERRORNEAR_SS               _("reading conf file: "SFN" error near "SFQ"\n")

/* 
** gdilib/sge_range.c
*/
#define MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S          _("Numerical value invalid!\nThe initial portion of string "SFQ" contains no decimal number\n")
#define MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS      _("Range specifier "SFQ" has unknown trailer "SFQ"\n")
#define MSG_GDI_UNEXPECTEDRANGEFOLLOWINGUNDEFINED        _("unexpected range following \"UNDEFINED\"\n")
#define MSG_GDI_UNEXPECTEDUNDEFINEDFOLLOWINGRANGE        _("unexpected \"UNDEFINED\" following range\n")


/* 
** gdilib/sge_feature.c
*/
#define MSG_GDI_PRODUCTMODENOTSETFORFILE_S               _("can't read "SFQ" - product mode not set.")
#define MSG_GDI_INVALIDPRODUCTMODESTRING_S               _("invalid product mode string "SFQ"\n")
#define MSG_GDI_CORRUPTPRODMODFILE_S                     _("product mode file "SFQ" is incorrect\n")
#define MSG_GDI_SWITCHFROMTO_SS                          _("switching from "SFQ" to "SFQ" feature set\n")

/* 
** gdilib/qm_name.c
*/
#define MSG_GDI_NULLPOINTERPASSED                        _("NULL pointer passed to \"master_host\" or \"master_file\"")
#define MSG_GDI_OPENMASTERFILEFAILED_S                   _("can't open "SFQ" for reading qmaster hostname")
#define MSG_GDI_READMASTERHOSTNAMEFAILED_S               _("can't read qmaster hostname in "SFQ"")
#define MSG_GDI_MASTERHOSTNAMEHASZEROLENGTH_S            _("qmaster hostname in "SFQ" has zero length")
#define MSG_GDI_MASTERHOSTNAMEEXCEEDSCHARS_SI            _("qmaster hostname in "SFQ" exceeds %d characters\n")
#define MSG_GDI_OPENWRITEMASTERHOSTNAMEFAILED_SS         _("can't open "SFQ" for writing qmaster hostname: "SFN"")
#define MSG_GDI_WRITEMASTERHOSTNAMEFAILED_S              _("can't write qmaster hostname into "SFQ"")
#define MSG_GDI_FOPEN_FAILED                             _("fopen("SFQ") failed: "SFN"\n")


/* 
** gdilib/resolve.c
*/
#define MSG_GDI_READMASTERNAMEFAILED_S                   _("unable to read qmaster name: "SFN"")



/* 
** gdilib/setup.c
*/
#define MSG_GDI_HOSTCMPPOLICYNOTSETFORFILE_S             _("can't read "SFQ" - host compare policy not set.")
#define MSG_GDI_NOVALIDSGECOMPRESSIONLEVEL_S             _(""SFN" is not a valid SGE_COMPRESSION_LEVEL\n")
#define MSG_GDI_SETCOMPRESSIONLEVEL_D                    _("Setting compression level to "U32CFormat"\n")
#define MSG_GDI_NOVALIDSGECOMPRESSIONTHRESHOLD_S         _(""SFN" is not a valid SGE_COMPRESSION_THRESHOLD\n")
#define MSG_GDI_SETCOMPRESSIONTHRESHOLD_D                _("Setting compression threshold to "U32CFormat"\n")


/* 
** gdilib/setup_path.c
*/
#define MSG_GDI_SGEROOTNOTADIRECTORY_S                   _("$SGE_ROOT="SFN" is not a directory\n")
#define MSG_GDI_DIRECTORYNOTEXIST_S                      _("directory doesn't exist: "SFN"\n")
#define MSG_SGETEXT_NOSGECELL_S                _("cell directory "SFQ" doesn't exist\n")





/* 
** gdilib/utility.c
*/
#define MSG_GDI_STRING_STRINGTOINTFAILED_S               _("unable to convert string "SFQ" to integer\n")


/* 
** gdilib/sge_gdi_request.c
*/
#define MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT   _("NULL pointer passed to sge_gdi_multi()")
#define MSG_GDI_CANTCREATEGDIREQUEST                     _("can't create gdi request")
#define MSG_GDI_NEEDUIDINEVIRONMENT                      _("need UID in environment\n")
#define MSG_GDI_NEEDUSERNAMEINENVIRONMENT                _("need USERNAME in environment\n")
#define MSG_GDI_NEEDGIDINENVIRONMENT                     _("need GID in environment\n")
#define MSG_GDI_NEEDGROUPNAMEINENVIRONMENT               _("need GROUPNAME in environment\n")
#define MSG_GDI_GETPWUIDXFAILEDERRORX_IS                 _("failed to getpwuid(%d): "SFN"\n")
#define MSG_GDI_GETGRGIDXFAILEDERRORX_IS                 _("failed to getgrgid(%d): "SFN"\n")
#define MSG_GDI_SENDINGGDIREQUESTFAILED                  _("failed sending gdi request\n")
#define MSG_GDI_RECEIVEGDIREQUESTFAILED                  _("failed receiving gdi request\n")
#define MSG_GDI_SIGNALED                                 _("signaled\n")
#define MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I     _("general error (%d) sending and receiving gdi request\n")
#define MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST  _("NULL list passed to sge_send_receive_gdi_request()")
#define MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST   _("NULL rhost passed to sge_send_receive_gdi_request()")
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST  _("not enough memory for packing gdi request\n")
#define MSG_GDI_REQUESTFORMATERROR                          _("format error while packing gdi request\n")
#define MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST       _("unexpected error while packing gdi request\n")
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORUNPACKINGGDIREQUEST _("not enough memory for unpacking gdi request\n")
#define MSG_GDI_REQUESTFORMATERRORWHILEUNPACKING              _("format error while unpacking gdi request\n")
#define MSG_GDI_UNEXPECTEDERRORWHILEUNPACKINGGDIREQUEST _("unexpected error while unpacking gdi request\n")
#define MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D             _("invalid value ("U32CFormat") for ar->op\n")
#define MSG_GDI_CANTUNPACKGDIREQUEST                       _("can't unpack gdi request"    )


/* sge_job_jatask.c */

#define MSG_JOB_REJECTED_NO_TASK_LIST     _("job "u32" was rejected because it was not possible to create task list\n")
#define MSG_JOB_JLPPNULL                  _("jlpp == NULL in job_add_job()\n")
#define MSG_JOB_JEPNULL                   _("jep == NULL in job_add_job()\n")
#define MSG_JOB_JOBALREADYEXISTS_S        _("can't add job "SFN" - job already exists\n") 
#define MSG_JOB_NULLNOTALLOWEDT           _("job rejected: 0 is an invalid task id\n")
#define MSG_JOB_NOIDNOTALLOWED            _("job rejected: Job comprises no tasks in its id lists") 
#define MSG_JOB_JOB_ID_U                  _(U32CFormat)
#define MSG_JOB_JOB_JATASK_ID_UU          _(U32CFormat"."U32CFormat)
#define MSG_JOB_JOB_JATASK_PETASK_ID_UUS  _(U32CFormat"."U32CFormat" task "SFN)

/* 
** sge_c_event.c
*/ 
#define MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY        _("failed to send ACK_EVENT_DELIVERY\n")
#define MSG_EVENT_HIGHESTEVENTISXWHILEWAITINGFORY_UU  _("highest event number is "U32CFormat" while waiting for "U32CFormat"\n")
#define MSG_EVENT_SMALLESTEVENTXISGRTHYWAITFOR_UU     _("smallest event number "U32CFormat" is greater than number "U32CFormat" i'm waiting for\n")
#define MSG_EVENT_EVENTSWITHNOINCREASINGNUMBERS       _("got events with not increasing numbers\n")
#define MSG_LIST_FAILEDINCULLUNPACKREPORT             _("Failed in cull_unpack report\n")
#define MSG_EVENT_ILLEGAL_ID_OR_NAME_US               _("Illegal id "U32CFormat" or name "SFQ" in event client registration\n")
#define MSG_EVENT_UNINITIALIZED_EC                    _("event client not properly initialized (ec_prepare_registration)\n")
#define MSG_EVENT_ILLEGALEVENTID_I                    _("illegal event id %d\n")
#define MSG_EVENT_ILLEGALFLUSHTIME_I                  _("illegal flush time %d - must be in the range [0:63]\n")
#define MSG_EVENT_NOTREGISTERED                       _("event client not registered\n")
#define MSG_EVENT_HAVETOHANDLEEVENTS                  _("you have to handle the events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN\n")

/*
 * sge_schedd_conf.c
 */
#define MSG_NONE_NOT_ALLOWED                    _("The keyword \"none\" is not allowed in \"load_formula\"\n")
#define MSG_NOTEXISTING_ATTRIBUTE_S             _("\"load_formula\" references not existing complex attribute "SFQ"\n")
#define MSG_WRONGTYPE_ATTRIBUTE_S               _("String, CString or Host attributes are not allowed in \"load_formula\": " SFQ "\n")


/* 
 * event.c
 */ 
#define MSG_EVENT_DELJOB_III                       _("%d. EVENT DEL JOB %d.%d\n")
#define MSG_EVENT_ADDJOB_III                       _("%d. EVENT ADD JOB %d.%d\n")
#define MSG_EVENT_MODJOB_III                       _("%d. EVENT MOD JOB %d.%d\n")
#define MSG_EVENT_JOBLISTXELEMENTS_II              _("%d. EVENT JOB LIST %d Elements\n")
#define MSG_EVENT_DELJOB_SCHEDD_INFO_III                       _("%d. EVENT DEL JOB_SCHEDD_INFO %d.%d\n")
#define MSG_EVENT_ADDJOB_SCHEDD_INFO_III                       _("%d. EVENT ADD JOB_SCHEDD_INFO %d.%d\n")
#define MSG_EVENT_MODJOB_SCHEDD_INFO_III                       _("%d. EVENT MOD JOB_SCHEDD_INFO %d.%d\n")
#define MSG_EVENT_JOB_SCHEDD_INFOLISTXELEMENTS_II              _("%d. EVENT JOB_SCHEDD_INFO LIST %d Elements\n")
#define MSG_EVENT_DELZOMBIE_III                       _("%d. EVENT DEL ZOMBIE %d.%d\n")
#define MSG_EVENT_ADDZOMBIE_III                       _("%d. EVENT ADD ZOMBIE %d.%d\n")
#define MSG_EVENT_MODZOMBIE_III                       _("%d. EVENT MOD ZOMBIE %d.%d\n")
#define MSG_EVENT_ZOMBIELISTXELEMENTS_II              _("%d. EVENT ZOMBIE LIST %d Elements\n")
#define MSG_EVENT_MODSCHEDDPRIOOFJOBXTOY_IDI       _("%d. EVENT MODIFY SCHEDULING PRIORITY OF JOB "U32CFormat" TO %d\n")
#define MSG_EVENT_JOBXUSAGE_II                     _("%d. EVENT JOB %d USAGE\n")
#define MSG_EVENT_JOBXFINALUSAGE_II                _("%d. EVENT JOB %d FINAL USAGE\n")
#define MSG_EVENT_DELQUEUEX_IS                     _("%d. EVENT DEL QUEUE "SFN"\n")
#define MSG_EVENT_ADDQUEUEX_IS                     _("%d. EVENT ADD QUEUE "SFN"\n")
#define MSG_EVENT_MODQUEUEX_IS                     _("%d. EVENT MOD QUEUE "SFN"\n")
#define MSG_EVENT_QUEUELISTXELEMENTS_II            _("%d. EVENT QUEUE LIST %d Elements\n")
#define MSG_EVENT_UNSUSPENDQUEUEXONSUBORDINATE_IS  _("%d. EVENT UNSUSPEND QUEUE "SFN" ON SUBORDINATE\n")
#define MSG_EVENT_SUSPENDQUEUEXONSUBORDINATE_IS    _("%d. EVENT SUSPEND QUEUE "SFN" ON SUBORDINATE\n")
#define MSG_EVENT_DELCOMPLEXX_IS                   _("%d. EVENT DEL COMPLEX "SFN"\n")
#define MSG_EVENT_ADDCOMPLEXX_IS                   _("%d. EVENT ADD COMPLEX "SFN"\n")
#define MSG_EVENT_MODCOMPLEXX_IS                   _("%d. EVENT MOD COMPLEX "SFN"\n")
#define MSG_EVENT_COMPLEXLISTXELEMENTS_II          _("%d. EVENT COMPLEX LIST %d Elements\n")
#define MSG_EVENT_DELCONFIGX_IS                   _("%d. EVENT DEL CONFIG "SFN"\n")
#define MSG_EVENT_ADDCONFIGX_IS                   _("%d. EVENT ADD CONFIG "SFN"\n")
#define MSG_EVENT_MODCONFIGX_IS                   _("%d. EVENT MOD CONFIG "SFN"\n")
#define MSG_EVENT_CONFIGLISTXELEMENTS_II          _("%d. EVENT CONFIG LIST %d Elements\n")
#define MSG_EVENT_DELCALENDARX_IS                  _("%d. EVENT DEL CALENDAR "SFN"\n")
#define MSG_EVENT_ADDCALENDARX_IS                  _("%d. EVENT ADD CALENDAR "SFN"\n")
#define MSG_EVENT_MODCALENDARX_IS                  _("%d. EVENT MOD CALENDAR "SFN"\n")
#define MSG_EVENT_CALENDARLISTXELEMENTS_II         _("%d. EVENT CALENDAR LIST %d Elements\n")
#define MSG_EVENT_DELADMINHOSTX_IS                  _("%d. EVENT DEL ADMINHOST "SFN"\n")
#define MSG_EVENT_ADDADMINHOSTX_IS                  _("%d. EVENT ADD ADMINHOST "SFN"\n")
#define MSG_EVENT_MODADMINHOSTX_IS                  _("%d. EVENT MOD ADMINHOST "SFN"\n")
#define MSG_EVENT_ADMINHOSTLISTXELEMENTS_II         _("%d. EVENT ADMINHOST LIST %d Elements\n")
#define MSG_EVENT_DELEXECHOSTX_IS                  _("%d. EVENT DEL EXECHOST "SFN"\n")
#define MSG_EVENT_ADDEXECHOSTX_IS                  _("%d. EVENT ADD EXECHOST "SFN"\n")
#define MSG_EVENT_MODEXECHOSTX_IS                  _("%d. EVENT MOD EXECHOST "SFN"\n")
#define MSG_EVENT_EXECHOSTLISTXELEMENTS_II         _("%d. EVENT EXECHOST LIST %d Elements\n")
#define MSG_EVENT_DELFEATURE_SETX_IS                  _("%d. EVENT DEL FEATURE_SET "SFN"\n")
#define MSG_EVENT_ADDFEATURE_SETX_IS                  _("%d. EVENT ADD FEATURE_SET "SFN"\n")
#define MSG_EVENT_MODFEATURE_SETX_IS                  _("%d. EVENT MOD FEATURE_SET "SFN"\n")
#define MSG_EVENT_FEATURE_SETLISTXELEMENTS_II         _("%d. EVENT FEATURE_SET LIST %d Elements\n")
#define MSG_EVENT_DELMANAGERX_IS                  _("%d. EVENT DEL MANAGER "SFN"\n")
#define MSG_EVENT_ADDMANAGERX_IS                  _("%d. EVENT ADD MANAGER "SFN"\n")
#define MSG_EVENT_MODMANAGERX_IS                  _("%d. EVENT MOD MANAGER "SFN"\n")
#define MSG_EVENT_MANAGERLISTXELEMENTS_II         _("%d. EVENT MANAGER LIST %d Elements\n")
#define MSG_EVENT_DELOPERATORX_IS                  _("%d. EVENT DEL OPERATOR "SFN"\n")
#define MSG_EVENT_ADDOPERATORX_IS                  _("%d. EVENT ADD OPERATOR "SFN"\n")
#define MSG_EVENT_MODOPERATORX_IS                  _("%d. EVENT MOD OPERATOR "SFN"\n")
#define MSG_EVENT_OPERATORLISTXELEMENTS_II         _("%d. EVENT OPERATOR LIST %d Elements\n")
#define MSG_EVENT_DELSUBMITHOSTX_IS                  _("%d. EVENT DEL SUBMITHOST "SFN"\n")
#define MSG_EVENT_ADDSUBMITHOSTX_IS                  _("%d. EVENT ADD SUBMITHOST "SFN"\n")
#define MSG_EVENT_MODSUBMITHOSTX_IS                  _("%d. EVENT MOD SUBMITHOST "SFN"\n")
#define MSG_EVENT_SUBMITHOSTLISTXELEMENTS_II         _("%d. EVENT SUBMITHOST LIST %d Elements\n")
#define MSG_EVENT_DELUSERSETX_IS                   _("%d. EVENT DEL USER SET "SFN"\n")
#define MSG_EVENT_ADDUSERSETX_IS                   _("%d. EVENT ADD USER SET "SFN"\n")
#define MSG_EVENT_MODUSERSETX_IS                   _("%d. EVENT MOD USER SET "SFN"\n")
#define MSG_EVENT_USERSETLISTXELEMENTS_II          _("%d. EVENT USER SET LIST %d Elements\n")
#define MSG_EVENT_DELUSERX_IS                      _("%d. EVENT DEL USER "SFN"\n")
#define MSG_EVENT_ADDUSERX_IS                      _("%d. EVENT ADD USER "SFN"\n")
#define MSG_EVENT_MODUSERX_IS                      _("%d. EVENT MOD USER "SFN"\n")
#define MSG_EVENT_USERLISTXELEMENTS_II             _("%d. EVENT USER LIST %d Elements\n")
#define MSG_EVENT_DELPROJECTX_IS                   _("%d. EVENT DEL PROJECT "SFN"\n")
#define MSG_EVENT_ADDPROJECTX_IS                   _("%d. EVENT ADD PROJECT "SFN"\n")
#define MSG_EVENT_MODPROJECTX_IS                   _("%d. EVENT MOD PROJECT "SFN"\n")
#define MSG_EVENT_PROJECTLISTXELEMENTS_II          _("%d. EVENT PROJECT LIST %d Elements\n")
#define MSG_EVENT_DELPEX_IS                        _("%d. EVENT DEL PE "SFN"\n")
#define MSG_EVENT_ADDPEX_IS                        _("%d. EVENT ADD PE "SFN"\n")
#define MSG_EVENT_MODPEX_IS                        _("%d. EVENT MOD PE "SFN"\n")
#define MSG_EVENT_PELISTXELEMENTS_II               _("%d. EVENT PE LIST %d Elements\n")
#define MSG_EVENT_SHUTDOWN_I                       _("%d. EVENT SHUTDOWN\n")
#define MSG_EVENT_QMASTERGOESDOWN_I                _("%d. EVENT QMASTER GOES DOWN\n")
#define MSG_EVENT_TRIGGERSCHEDULERMONITORING_I     _("%d. EVENT TRIGGER SCHEDULER MONITORING\n")
#define MSG_EVENT_SHARETREEXNODESYLEAFS_III        _("%d. EVENT SHARETREE %d nodes %d leafs\n")
#define MSG_EVENT_SCHEDULERCONFIG_I                _("%d. EVENT SCHEDULER CONFIG \n")
#define MSG_EVENT_GLOBAL_CONFIG_I                  _("%d. EVENT NEW GLOBAL CONFIG\n")
#define MSG_EVENT_DELCKPT_IS                       _("%d. EVENT DEL CKPT "SFN"\n")
#define MSG_EVENT_ADDCKPT_IS                       _("%d. EVENT ADD CKPT "SFN"\n")
#define MSG_EVENT_MODCKPT_IS                       _("%d. EVENT MOD CKPT "SFN"\n")
#define MSG_EVENT_CKPTLISTXELEMENTS_II             _("%d. EVENT CKPT LIST %d Elements\n")
#define MSG_EVENT_DELJATASK_UUU                    _(U32CFormat". EVENT DEL JATASK "U32CFormat"."U32CFormat"\n")
#define MSG_EVENT_MODJATASK_UUU                    _(U32CFormat". EVENT MOD JATASK "U32CFormat"."U32CFormat"\n")
#define MSG_EVENT_ADDPETASK_UUUS                   _(U32CFormat". EVENT ADD PETASK "U32CFormat"."U32CFormat" task "SFN"\n")
#define MSG_EVENT_DELPETASK_UUUS                   _(U32CFormat". EVENT DEL PETASK "U32CFormat"."U32CFormat" task "SFN"\n")
#define MSG_EVENT_MODPETASK_UUUS                   _(U32CFormat". EVENT MOD PETASK "U32CFormat"."U32CFormat" task "SFN"\n")
#define MSG_EVENT_NOTKNOWN_I                       _("%d. EVENT ????????\n")

#endif /* __MSG_GDILIB_H */

