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
#define MSG_QSH_GET_CREDSTDERR_S _MESSAGE(43000, _("get_cred stderr: "SFN""))
#define MSG_QSH_QSUBFAILED    _MESSAGE(43001, _("qsub failed\n"))
#define MSG_QSUB_CANTSTARTCOMMANDXTOGETTOKENQSUBFAILED_S    _MESSAGE(43002, _("can't start command "SFQ" to get token - qsub failed\n"))
#define MSG_QSH_CANTSTARTCOMMANDXTOGETCREDENTIALSQSUBFAILED_S    _MESSAGE(43003, _("can't start command "SFQ" to get credentials - qsub failed\n"))
#define MSG_QSH_CANTGETCREDENTIALS    _MESSAGE(43004, _("warning: could not get credentials\n"))
#define MSG_SEC_NOCRED_USSI           _MESSAGE(43005, _("could not get credentials for job " U32CFormat " for execution host "SFN" - command "SFQ" failed with return code %d\n"))
#define MSG_SEC_STARTDELCREDCMD_SU    _MESSAGE(43006, _("can't start command "SFQ" for job " U32CFormat " to delete credentials\n"))
#define MSG_SEC_NOAUTH_U              _MESSAGE(43007, _("job "U32CFormat" rejected because authentication failed\n"))
#define MSG_SEC_NOSTORECRED_USI       _MESSAGE(43008, _("could not store credentials for job " U32CFormat" - command "SFQ" failed with return code %d\n"))
#define MSG_SEC_NOSTORECREDNOBIN_US   _MESSAGE(43009, _("could not store client credentials for job " U32CFormat" - "SFN" binary does not exist\n"))
#define MSG_SEC_DELCREDSTDERR_S       _MESSAGE(43010, _("delete_cred stderr: "SFN""))
#define MSG_SEC_DELCREDRETCODE_USI    _MESSAGE(43011, _("could not delete credentials for job " U32CFormat" - command "SFQ" failed with return code %d\n"))
#define MSG_SEC_DELCREDNOBIN_US       _MESSAGE(43012, _("could not delete credentials for job "U32CFormat" - "SFN" binary does not exist\n"))
#define MSG_SEC_KRBAUTHFAILURE        _MESSAGE(43013, _("job "U32CFormat" rejected because authentication failed\n"))
#define MSG_SEC_KRBAUTHFAILUREONHOST  _MESSAGE(43014, _("job "U32CFormat" rejected because authentication failed on host "SFN"\n"))
#define MSG_SEC_PUTCREDSTDERR_S       _MESSAGE(43015, _("put_cred stderr: "SFN""))
#define MSG_SEC_NOSTARTCMD4GETCRED_SU _MESSAGE(43016, _("can't start command "SFQ" for job " U32CFormat " to get credentials\n"))
#define MSG_SEC_NOCREDNOBIN_US        _MESSAGE(43017, _("could not get client credentials for job " U32CFormat" - "SFN" binary does not exist\n"))
#define MSG_SEC_KRB_CRED_SSSI         _MESSAGE(43018, _("denied: request for user "SFQ" does not match Kerberos credentials for connection <"SFN","SFN",%d>\n") )         
#define MSG_SEC_KRBDECRYPTTGT_US      _MESSAGE(43019, _("could not decrypt TGT for job " U32CFormat "- "SFN"\n"))
#define MSG_SEC_KRBENCRYPTTGT_SSIS    _MESSAGE(43020, _("could not encrypt TGT for client <"SFN","SFN",%d> - "SFN"\n"))
#define MSG_SEC_KRBENCRYPTTGTUSER_SUS _MESSAGE(43021, _("could not encrypt TGT for user "SFN", job "U32CFormat" - "SFN""))
#define MSG_SEC_NOUID_SU              _MESSAGE(43022, _("could not get user ID for "SFN", job "U32CFormat"\n"))


/*
** sge_resource.c
*/
#define MSG_PARSE_NOALLOCREQRES       _MESSAGE(43023, _("unable to alloc space for requested resources\n"))
#define MSG_PARSE_NOALLOCATTRLIST     _MESSAGE(43024, _("unable to alloc space for attrib. list\n"))
#define MSG_PARSE_NOALLOCRESELEM      _MESSAGE(43025, _("unable to alloc space for resource element\n"))
#define MSG_PARSE_NOVALIDSLOTRANGE_S  _MESSAGE(43026, _(""SFQ" must be a valid slot range\n"))
#define MSG_PARSE_NOALLOCATTRELEM     _MESSAGE(43027, _("unable to alloc space for attrib. element\n"))


/*
** sge_parse_date_time.c
*/
#define MSG_PARSE_STARTTIMETOOLONG    _MESSAGE(43028, _("Starttime specifier field length exceeds maximum"))
#define MSG_PARSE_INVALIDSECONDS      _MESSAGE(43029, _("Invalid format of seconds field."))
#define MSG_PARSE_INVALIDHOURMIN      _MESSAGE(43030, _("Invalid format of date/hour-minute field."))
#define MSG_PARSE_INVALIDMONTH        _MESSAGE(43031, _("Invalid month specification."))
#define MSG_PARSE_INVALIDDAY          _MESSAGE(43032, _("Invalid day specification."))
#define MSG_PARSE_INVALIDHOUR         _MESSAGE(43033, _("Invalid hour specification."))
#define MSG_PARSE_INVALIDMINUTE       _MESSAGE(43034, _("Invalid minute specification."))
#define MSG_PARSE_INVALIDSECOND       _MESSAGE(43035, _("Invalid seconds specification."))
#define MSG_PARSE_NODATEFROMINPUT     _MESSAGE(43036, _("Couldn't generate date from input. Perhaps a date before 1970 was specified."))

/*
** parse.c
*/
/* #define MSG_PARSE_XISNOTAVALIDOPTION_S      _MESSAGE(43037, _(""SFQ" is not a valid option\n") ) */
/* #define MSG_PARSE_NOOPTIONARGPROVIDEDTOX_S  _MESSAGE(43038, _("no option argument provided to "SFQ"") ) */
/* #define MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S _MESSAGE(43039, _("ERROR! "SFN" option must have argument\n") ) */
#define MSG_JOB_XISINVALIDJOBTASKID_S       _MESSAGE(43040, _("ERROR! "SFN" is a invalid job-task identifier\n"))





/* 
** gdilib/gdi_qmod.c
*/ 
#define MSG_GDI_INVALIDACTION          _MESSAGE(43041, _("invalid action" ))
#define MSG_GDI_INVALIDOPTIONFLAG      _MESSAGE(43042, _("invalid option flag"))
#define MSG_GDI_INVALIDIDENCOUNTERED   _MESSAGE(43043, _("encountered invalid id"))
#define MSG_GDI_OUTOFMEMORY            _MESSAGE(43044, _("out of memory"))

/* 
** gdilib/sge_any_request.c
*/ 
#define MSG_GDI_INITSECURITYDATAFAILED                _MESSAGE(43045, _("failed initialize security data\n"))
#define MSG_GDI_INITKERBEROSSECURITYDATAFAILED        _MESSAGE(43046, _("failed initialize Kerberos security data\n"))
#define MSG_GDI_ENROLLTOCOMMDFAILED_S                 _MESSAGE(43047, _("can't enroll to commd: "SFN"\n"))
#define MSG_GDI_COMMDUP                               _MESSAGE(43048, _("commd is up"))
#define MSG_GDI_COMMDDOWN_S                           _MESSAGE(43049, _("commd is down: "SFN""))
#define MSG_GDI_RHOSTISNULLFORSENDREQUEST             _MESSAGE(43050, _("parameter rhost = NULL for sge_send_any_request()"))
#define MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS     _MESSAGE(43051, _("can't send "SFN"synchronous message to commproc ("SFN":%d) on host "SFQ": "SFN""))
#define MSG_GDI_RHOSTISNULLFORGETANYREQUEST           _MESSAGE(43052, _("parameter rhost = NULL for sge_get_any_request()"))
#define MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS _MESSAGE(43053, _("can't receive message from commproc ("SFN":%d) on host "SFQ": "SFN""))

/* 
** gdilib/sge_parse_num_par.c
*/ 
#define MSG_GDI_VALUETHATCANBESETTOINF                  _MESSAGE(43055, _("value that can be set to infinity"))
#define MSG_GDI_READCONFIGFILESPECGIVENTWICE_SS         _MESSAGE(43056, _("reading config file: specifier "SFQ" given twice for "SFQ"\n"))
#define MSG_GDI_READCONFIGFILEUNKNOWNSPEC_SS            _MESSAGE(43057, _("reading conf file: unknown specifier "SFQ" for "SFN"\n"))
#define MSG_GDI_READCONFIGFILEEMPTYENUMERATION_S        _MESSAGE(43058, _("reading conf file: empty enumeration for "SFQ"\n " ))


/* 
** gdilib/sge_parse_num_val.c
*/ 
#define MSG_GDI_UNRECOGNIZEDVALUETRAILER_SS             _MESSAGE(43059, _("Error! Unrecognized value-trailer '%20s' near '%20s'\nI expected multipliers k, K, m and M.\nThe value string is probably badly formed!\n" ))
#define MSG_GDI_UNEXPECTEDENDOFNUMERICALVALUE_SC        _MESSAGE(43060, _("Error! Unexpected end of numerical value near "SFN".\nExpected one of ',', '/' or '\\0'. Got '%c'\n" ))
#define MSG_GDI_NUMERICALVALUEFORHOUREXCEEDED_SS         _MESSAGE(43061, _("Error! numerical value near %20s for hour exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALID_SS                _MESSAGE(43062, _("Error! numerical value near %20s invalid.\n'%20s' is no valid time specifier!\n" ))
#define MSG_GDI_NUMERICALVALUEFORMINUTEEXCEEDED_SS       _MESSAGE(43063, _("Error! numerical value near %20s for minute exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNONUMBER_SS         _MESSAGE(43064, _("Error! numerical value near %20s invalid.\n>%20s< contains no valid decimal or fixed float number\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNOHEXOCTNUMBER_SS      _MESSAGE(43065, _("Error! numerical value near "SFN" invalid.\n'"SFN"' contains no valid hex or octal number\n"))



/* 
** gdilib/sge_qexec.c
*/
#define MSG_GDI_INVALIDPARAMETER_SS                      _MESSAGE(43066, _("invalid paramter to "SFQ": "SFQ))
#define MSG_GDI_RESOLVINGUIDTOUSERNAMEFAILED_IS          _MESSAGE(43067, _("failed resolving uid %d to username: "SFN""))
#define MSG_GDI_MISSINGINENVIRONMENT_S                   _MESSAGE(43068, _("missing "SFQ" in environment"))
#define MSG_GDI_STRINGISINVALID_SS                       _MESSAGE(43069, _("string "SFQ" in env var "SFQ" is not a valid job/taskid"))
#define MSG_GDI_PROPOSEDTASKIDINVALID_SS                 _MESSAGE(43070, _("proposed task id "SFN" "SFQ" is invalid"))
#define MSG_GDI_TASKEXISTS_S                             _MESSAGE(43071, _("task "SFN" already exists"))
#define MSG_GDI_SETUPGDILIBFAILED                        _MESSAGE(43072, _("failed setting up gdi library"))
#define MSG_GDI_SENDTASKTOEXECDFAILED_SS                 _MESSAGE(43073, _("failed sending task to execd@"SFN": "SFN""                  ))
#define MSG_GDI_TASKNOTEXIST_S                           _MESSAGE(43074, _("task "SFQ" does not exist"))
#define MSG_GDI_RCVFROMEXECLOCALCOMMDFAILED_S            _MESSAGE(43075, _("rcv_from_exec(): failed enrolling to local sge commd: "SFN""))
#define MSG_GDI_MESSAGERECEIVEFAILED_SI                  _MESSAGE(43076, _("failed receiving message from execd: "SFN" %d"))
#define MSG_GDI_TASKNOTFOUND_S                           _MESSAGE(43077, _("cannot find task with taskid "SFQ"\n"))
#define MSG_GDI_TASKNOTFOUNDNOIDGIVEN_S                  _MESSAGE(43078, _("cannot find task without taskid - should become task "SFQ""))



/* 
** gdilib/sge_report.c
*/
#define MSG_GDI_REPORTNOMEMORY_I                         _MESSAGE(43079, _("not enough memory for packing report: %d bytes\n"))
#define MSG_GDI_REPORTFORMATERROR                        _MESSAGE(43080, _("format error while packing report\n"))
#define MSG_GDI_REPORTUNKNOWERROR                        _MESSAGE(43081, _("unexpected error while packing report\n"))


/* 
** gdilib/config.c
*/
#define MSG_GDI_CONFIGNOARGUMENTGIVEN_S                  _MESSAGE(43082, _("no argument given in config option: "SFN"\n"))
#define MSG_GDI_CONFIGMISSINGARGUMENT_S                  _MESSAGE(43083, _("missing configuration attribute "SFQ""))
#define MSG_GDI_CONFIGADDLISTFAILED_S                    _MESSAGE(43084, _("can't add "SFQ" to list"))
#define MSG_GDI_CONFIGARGUMENTNOTINTEGER_SS              _MESSAGE(43085, _("value for attribute "SFN" "SFQ" is not an integer\n"))
#define MSG_GDI_CONFIGARGUMENTNOTDOUBLE_SS               _MESSAGE(43086, _("value for attribute "SFN" "SFQ" is not a double\n"))
#define MSG_GDI_CONFIGARGUMENTNOTTIME_SS                 _MESSAGE(43087, _("value for attribute "SFN" "SFQ" is not time\n"))
#define MSG_GDI_CONFIGARGUMENTNOMEMORY_SS                _MESSAGE(43088, _("value for attribute "SFN" "SFQ" is not memory\n"))
#define MSG_GDI_CONFIGINVALIDQUEUESPECIFIED              _MESSAGE(43089, _("reading conf file: invalid queue type specified\n"))
#define MSG_GDI_CONFIGREADFILEERRORNEAR_SS               _MESSAGE(43090, _("reading conf file: "SFN" error near "SFQ"\n"))

/* 
** gdilib/sge_range.c
*/
#define MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S          _MESSAGE(43091, _("Numerical value invalid!\nThe initial portion of string "SFQ" contains no decimal number\n"))
#define MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS      _MESSAGE(43092, _("Range specifier "SFQ" has unknown trailer "SFQ"\n"))
#define MSG_GDI_UNEXPECTEDRANGEFOLLOWINGUNDEFINED        _MESSAGE(43093, _("unexpected range following \"UNDEFINED\"\n"))
#define MSG_GDI_UNEXPECTEDUNDEFINEDFOLLOWINGRANGE        _MESSAGE(43094, _("unexpected \"UNDEFINED\" following range\n"))


/* 
** gdilib/sge_feature.c
*/
#define MSG_GDI_PRODUCTMODENOTSETFORFILE_S               _MESSAGE(43095, _("can't read "SFQ" - product mode not set."))
#define MSG_GDI_INVALIDPRODUCTMODESTRING_S               _MESSAGE(43096, _("invalid product mode string "SFQ"\n"))
#define MSG_GDI_CORRUPTPRODMODFILE_S                     _MESSAGE(43097, _("product mode file "SFQ" is incorrect\n"))
#define MSG_GDI_SWITCHFROMTO_SS                          _MESSAGE(43098, _("switching from "SFQ" to "SFQ" feature set\n"))

/* 
** gdilib/qm_name.c
*/
#define MSG_GDI_NULLPOINTERPASSED                        _MESSAGE(43099, _("NULL pointer passed to \"master_host\" or \"master_file\""))
#define MSG_GDI_OPENMASTERFILEFAILED_S                   _MESSAGE(43100, _("can't open "SFQ" for reading qmaster hostname"))
#define MSG_GDI_READMASTERHOSTNAMEFAILED_S               _MESSAGE(43101, _("can't read qmaster hostname in "SFQ""))
#define MSG_GDI_MASTERHOSTNAMEHASZEROLENGTH_S            _MESSAGE(43102, _("qmaster hostname in "SFQ" has zero length"))
#define MSG_GDI_MASTERHOSTNAMEEXCEEDSCHARS_SI            _MESSAGE(43103, _("qmaster hostname in "SFQ" exceeds %d characters\n"))
#define MSG_GDI_OPENWRITEMASTERHOSTNAMEFAILED_SS         _MESSAGE(43104, _("can't open "SFQ" for writing qmaster hostname: "SFN""))
#define MSG_GDI_WRITEMASTERHOSTNAMEFAILED_S              _MESSAGE(43105, _("can't write qmaster hostname into "SFQ""))
#define MSG_GDI_FOPEN_FAILED                             _MESSAGE(43106, _("fopen("SFQ") failed: "SFN"\n"))


/* 
** gdilib/resolve.c
*/
#define MSG_GDI_READMASTERNAMEFAILED_S                   _MESSAGE(43107, _("unable to read qmaster name: "SFN""))



/* 
** gdilib/setup.c
*/
#define MSG_GDI_HOSTCMPPOLICYNOTSETFORFILE_S             _MESSAGE(43108, _("can't read "SFQ" - host compare policy not set."))
#define MSG_GDI_NOVALIDSGECOMPRESSIONLEVEL_S             _MESSAGE(43109, _(""SFN" is not a valid SGE_COMPRESSION_LEVEL\n"))
#define MSG_GDI_SETCOMPRESSIONLEVEL_D                    _MESSAGE(43110, _("Setting compression level to "U32CFormat"\n"))
#define MSG_GDI_NOVALIDSGECOMPRESSIONTHRESHOLD_S         _MESSAGE(43111, _(""SFN" is not a valid SGE_COMPRESSION_THRESHOLD\n"))
#define MSG_GDI_SETCOMPRESSIONTHRESHOLD_D                _MESSAGE(43112, _("Setting compression threshold to "U32CFormat"\n"))


/* 
** gdilib/setup_path.c
*/
#define MSG_GDI_SGEROOTNOTADIRECTORY_S                   _MESSAGE(43113, _("$SGE_ROOT="SFN" is not a directory\n"))
#define MSG_GDI_DIRECTORYNOTEXIST_S                      _MESSAGE(43114, _("directory doesn't exist: "SFN"\n"))
#define MSG_SGETEXT_NOSGECELL_S                _MESSAGE(43115, _("cell directory "SFQ" doesn't exist\n"))





/* 
** gdilib/utility.c
*/
#define MSG_GDI_STRING_STRINGTOINTFAILED_S               _MESSAGE(43116, _("unable to convert string "SFQ" to integer\n"))


/* 
** gdilib/sge_gdi_request.c
*/
#define MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT   _MESSAGE(43117, _("NULL pointer passed to sge_gdi_multi()"))
#define MSG_GDI_CANTCREATEGDIREQUEST                     _MESSAGE(43118, _("can't create gdi request"))
#define MSG_GDI_NEEDUIDINEVIRONMENT                      _MESSAGE(43119, _("need UID in environment\n"))
#define MSG_GDI_NEEDUSERNAMEINENVIRONMENT                _MESSAGE(43120, _("need USERNAME in environment\n"))
#define MSG_GDI_NEEDGIDINENVIRONMENT                     _MESSAGE(43121, _("need GID in environment\n"))
#define MSG_GDI_NEEDGROUPNAMEINENVIRONMENT               _MESSAGE(43122, _("need GROUPNAME in environment\n"))
#define MSG_GDI_GETPWUIDXFAILEDERRORX_IS                 _MESSAGE(43123, _("failed to getpwuid(%d): "SFN"\n"))
#define MSG_GDI_GETGRGIDXFAILEDERRORX_IS                 _MESSAGE(43124, _("failed to getgrgid(%d): "SFN"\n"))
#define MSG_GDI_SENDINGGDIREQUESTFAILED                  _MESSAGE(43125, _("failed sending gdi request\n"))
#define MSG_GDI_RECEIVEGDIREQUESTFAILED                  _MESSAGE(43126, _("failed receiving gdi request\n"))
#define MSG_GDI_SIGNALED                                 _MESSAGE(43127, _("signaled\n"))
#define MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I     _MESSAGE(43128, _("general error (%d) sending and receiving gdi request\n"))
#define MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST  _MESSAGE(43129, _("NULL list passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST   _MESSAGE(43130, _("NULL rhost passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST  _MESSAGE(43131, _("not enough memory for packing gdi request\n"))
#define MSG_GDI_REQUESTFORMATERROR                          _MESSAGE(43132, _("format error while packing gdi request\n"))
#define MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST       _MESSAGE(43133, _("unexpected error while packing gdi request\n"))
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORUNPACKINGGDIREQUEST _MESSAGE(43134, _("not enough memory for unpacking gdi request\n"))
#define MSG_GDI_REQUESTFORMATERRORWHILEUNPACKING              _MESSAGE(43135, _("format error while unpacking gdi request\n"))
#define MSG_GDI_UNEXPECTEDERRORWHILEUNPACKINGGDIREQUEST _MESSAGE(43136, _("unexpected error while unpacking gdi request\n"))
#define MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D             _MESSAGE(43137, _("invalid value ("U32CFormat") for ar->op\n"))
#define MSG_GDI_CANTUNPACKGDIREQUEST                       _MESSAGE(43138, _("can't unpack gdi request"    ))
#define MSG_JOB_REJECTED_NO_TASK_LIST     _MESSAGE(43139, _("job "u32" was rejected because it was not possible to create task list\n"))
#define MSG_JOB_JLPPNULL                  _MESSAGE(43140, _("jlpp == NULL in job_add_job()\n"))
#define MSG_JOB_JEPNULL                   _MESSAGE(43141, _("jep == NULL in job_add_job()\n"))
#define MSG_JOB_JOBALREADYEXISTS_S        _MESSAGE(43142, _("can't add job "SFN" - job already exists\n") ) 
#define MSG_JOB_NULLNOTALLOWEDT           _MESSAGE(43143, _("job rejected: 0 is an invalid task id\n"))
#define MSG_JOB_NOIDNOTALLOWED            _MESSAGE(43144, _("job rejected: Job comprises no tasks in its id lists") ) 
#define MSG_JOB_JOB_ID_U                  _MESSAGE(43145, _(U32CFormat))
#define MSG_JOB_JOB_JATASK_ID_UU          _MESSAGE(43146, _(U32CFormat"."U32CFormat))
#define MSG_JOB_JOB_JATASK_PETASK_ID_UUS  _MESSAGE(43147, _(U32CFormat"."U32CFormat" task "SFN))

/* 
** sge_c_event.c
*/ 
#define MSG_COMMD_FAILEDTOSENDACKEVENTDELIVERY        _MESSAGE(43148, _("failed to send ACK_EVENT_DELIVERY\n"))
#define MSG_EVENT_HIGHESTEVENTISXWHILEWAITINGFORY_UU  _MESSAGE(43149, _("highest event number is "U32CFormat" while waiting for "U32CFormat"\n"))
#define MSG_EVENT_SMALLESTEVENTXISGRTHYWAITFOR_UU     _MESSAGE(43150, _("smallest event number "U32CFormat" is greater than number "U32CFormat" i'm waiting for\n"))
#define MSG_EVENT_EVENTSWITHNOINCREASINGNUMBERS       _MESSAGE(43151, _("got events with not increasing numbers\n"))
#define MSG_LIST_FAILEDINCULLUNPACKREPORT             _MESSAGE(43152, _("Failed in cull_unpack report\n"))
#define MSG_EVENT_ILLEGAL_ID_OR_NAME_US               _MESSAGE(43153, _("Illegal id "U32CFormat" or name "SFQ" in event client registration\n"))
#define MSG_EVENT_UNINITIALIZED_EC                    _MESSAGE(43154, _("event client not properly initialized (ec_prepare_registration)\n"))
#define MSG_EVENT_ILLEGALEVENTID_I                    _MESSAGE(43155, _("illegal event id %d\n"))
#define MSG_EVENT_ILLEGALFLUSHTIME_I                  _MESSAGE(43156, _("illegal flush time %d - must be in the range [0:63]\n"))
#define MSG_EVENT_NOTREGISTERED                       _MESSAGE(43157, _("event client not registered\n"))
#define MSG_EVENT_HAVETOHANDLEEVENTS                  _MESSAGE(43158, _("you have to handle the events sgeE_QMASTER_GOES_DOWN and sgeE_SHUTDOWN\n"))

/*
 * sge_schedd_conf.c
 */
#define MSG_NONE_NOT_ALLOWED                    _MESSAGE(43159, _("The keyword \"none\" is not allowed in \"load_formula\"\n"))
#define MSG_NOTEXISTING_ATTRIBUTE_S             _MESSAGE(43160, _("\"load_formula\" references not existing complex attribute "SFQ"\n"))
#define MSG_WRONGTYPE_ATTRIBUTE_S               _MESSAGE(43161, _("String, CString or Host attributes are not allowed in \"load_formula\": " SFQ "\n"))


/* 
 * event.c
 */ 
#define MSG_EVENT_DELJOB_III                       _MESSAGE(43162, _("%d. EVENT DEL JOB %d.%d\n"))
#define MSG_EVENT_ADDJOB_III                       _MESSAGE(43163, _("%d. EVENT ADD JOB %d.%d\n"))
#define MSG_EVENT_MODJOB_III                       _MESSAGE(43164, _("%d. EVENT MOD JOB %d.%d\n"))
#define MSG_EVENT_JOBLISTXELEMENTS_II              _MESSAGE(43165, _("%d. EVENT JOB LIST %d Elements\n"))
#define MSG_EVENT_DELJOB_SCHEDD_INFO_III                       _MESSAGE(43166, _("%d. EVENT DEL JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_ADDJOB_SCHEDD_INFO_III                       _MESSAGE(43167, _("%d. EVENT ADD JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_MODJOB_SCHEDD_INFO_III                       _MESSAGE(43168, _("%d. EVENT MOD JOB_SCHEDD_INFO %d.%d\n"))
#define MSG_EVENT_JOB_SCHEDD_INFOLISTXELEMENTS_II              _MESSAGE(43169, _("%d. EVENT JOB_SCHEDD_INFO LIST %d Elements\n"))
#define MSG_EVENT_DELZOMBIE_III                       _MESSAGE(43170, _("%d. EVENT DEL ZOMBIE %d.%d\n"))
#define MSG_EVENT_ADDZOMBIE_III                       _MESSAGE(43171, _("%d. EVENT ADD ZOMBIE %d.%d\n"))
#define MSG_EVENT_MODZOMBIE_III                       _MESSAGE(43172, _("%d. EVENT MOD ZOMBIE %d.%d\n"))
#define MSG_EVENT_ZOMBIELISTXELEMENTS_II              _MESSAGE(43173, _("%d. EVENT ZOMBIE LIST %d Elements\n"))
#define MSG_EVENT_MODSCHEDDPRIOOFJOBXTOY_IDI       _MESSAGE(43174, _("%d. EVENT MODIFY SCHEDULING PRIORITY OF JOB "U32CFormat" TO %d\n"))
#define MSG_EVENT_JOBXUSAGE_II                     _MESSAGE(43175, _("%d. EVENT JOB %d USAGE\n"))
#define MSG_EVENT_JOBXFINALUSAGE_II                _MESSAGE(43176, _("%d. EVENT JOB %d FINAL USAGE\n"))
#define MSG_EVENT_DELQUEUEX_IS                     _MESSAGE(43177, _("%d. EVENT DEL QUEUE "SFN"\n"))
#define MSG_EVENT_ADDQUEUEX_IS                     _MESSAGE(43178, _("%d. EVENT ADD QUEUE "SFN"\n"))
#define MSG_EVENT_MODQUEUEX_IS                     _MESSAGE(43179, _("%d. EVENT MOD QUEUE "SFN"\n"))
#define MSG_EVENT_QUEUELISTXELEMENTS_II            _MESSAGE(43180, _("%d. EVENT QUEUE LIST %d Elements\n"))
#define MSG_EVENT_UNSUSPENDQUEUEXONSUBORDINATE_IS  _MESSAGE(43181, _("%d. EVENT UNSUSPEND QUEUE "SFN" ON SUBORDINATE\n"))
#define MSG_EVENT_SUSPENDQUEUEXONSUBORDINATE_IS    _MESSAGE(43182, _("%d. EVENT SUSPEND QUEUE "SFN" ON SUBORDINATE\n"))
#define MSG_EVENT_DELCOMPLEXX_IS                   _MESSAGE(43183, _("%d. EVENT DEL COMPLEX "SFN"\n"))
#define MSG_EVENT_ADDCOMPLEXX_IS                   _MESSAGE(43184, _("%d. EVENT ADD COMPLEX "SFN"\n"))
#define MSG_EVENT_MODCOMPLEXX_IS                   _MESSAGE(43185, _("%d. EVENT MOD COMPLEX "SFN"\n"))
#define MSG_EVENT_COMPLEXLISTXELEMENTS_II          _MESSAGE(43186, _("%d. EVENT COMPLEX LIST %d Elements\n"))
#define MSG_EVENT_DELCONFIGX_IS                   _MESSAGE(43187, _("%d. EVENT DEL CONFIG "SFN"\n"))
#define MSG_EVENT_ADDCONFIGX_IS                   _MESSAGE(43188, _("%d. EVENT ADD CONFIG "SFN"\n"))
#define MSG_EVENT_MODCONFIGX_IS                   _MESSAGE(43189, _("%d. EVENT MOD CONFIG "SFN"\n"))
#define MSG_EVENT_CONFIGLISTXELEMENTS_II          _MESSAGE(43190, _("%d. EVENT CONFIG LIST %d Elements\n"))
#define MSG_EVENT_DELCALENDARX_IS                  _MESSAGE(43191, _("%d. EVENT DEL CALENDAR "SFN"\n"))
#define MSG_EVENT_ADDCALENDARX_IS                  _MESSAGE(43192, _("%d. EVENT ADD CALENDAR "SFN"\n"))
#define MSG_EVENT_MODCALENDARX_IS                  _MESSAGE(43193, _("%d. EVENT MOD CALENDAR "SFN"\n"))
#define MSG_EVENT_CALENDARLISTXELEMENTS_II         _MESSAGE(43194, _("%d. EVENT CALENDAR LIST %d Elements\n"))
#define MSG_EVENT_DELADMINHOSTX_IS                  _MESSAGE(43195, _("%d. EVENT DEL ADMINHOST "SFN"\n"))
#define MSG_EVENT_ADDADMINHOSTX_IS                  _MESSAGE(43196, _("%d. EVENT ADD ADMINHOST "SFN"\n"))
#define MSG_EVENT_MODADMINHOSTX_IS                  _MESSAGE(43197, _("%d. EVENT MOD ADMINHOST "SFN"\n"))
#define MSG_EVENT_ADMINHOSTLISTXELEMENTS_II         _MESSAGE(43198, _("%d. EVENT ADMINHOST LIST %d Elements\n"))
#define MSG_EVENT_DELEXECHOSTX_IS                  _MESSAGE(43199, _("%d. EVENT DEL EXECHOST "SFN"\n"))
#define MSG_EVENT_ADDEXECHOSTX_IS                  _MESSAGE(43200, _("%d. EVENT ADD EXECHOST "SFN"\n"))
#define MSG_EVENT_MODEXECHOSTX_IS                  _MESSAGE(43201, _("%d. EVENT MOD EXECHOST "SFN"\n"))
#define MSG_EVENT_EXECHOSTLISTXELEMENTS_II         _MESSAGE(43202, _("%d. EVENT EXECHOST LIST %d Elements\n"))
#define MSG_EVENT_DELFEATURE_SETX_IS                  _MESSAGE(43203, _("%d. EVENT DEL FEATURE_SET "SFN"\n"))
#define MSG_EVENT_ADDFEATURE_SETX_IS                  _MESSAGE(43204, _("%d. EVENT ADD FEATURE_SET "SFN"\n"))
#define MSG_EVENT_MODFEATURE_SETX_IS                  _MESSAGE(43205, _("%d. EVENT MOD FEATURE_SET "SFN"\n"))
#define MSG_EVENT_FEATURE_SETLISTXELEMENTS_II         _MESSAGE(43206, _("%d. EVENT FEATURE_SET LIST %d Elements\n"))
#define MSG_EVENT_DELMANAGERX_IS                  _MESSAGE(43207, _("%d. EVENT DEL MANAGER "SFN"\n"))
#define MSG_EVENT_ADDMANAGERX_IS                  _MESSAGE(43208, _("%d. EVENT ADD MANAGER "SFN"\n"))
#define MSG_EVENT_MODMANAGERX_IS                  _MESSAGE(43209, _("%d. EVENT MOD MANAGER "SFN"\n"))
#define MSG_EVENT_MANAGERLISTXELEMENTS_II         _MESSAGE(43210, _("%d. EVENT MANAGER LIST %d Elements\n"))
#define MSG_EVENT_DELOPERATORX_IS                  _MESSAGE(43211, _("%d. EVENT DEL OPERATOR "SFN"\n"))
#define MSG_EVENT_ADDOPERATORX_IS                  _MESSAGE(43212, _("%d. EVENT ADD OPERATOR "SFN"\n"))
#define MSG_EVENT_MODOPERATORX_IS                  _MESSAGE(43213, _("%d. EVENT MOD OPERATOR "SFN"\n"))
#define MSG_EVENT_OPERATORLISTXELEMENTS_II         _MESSAGE(43214, _("%d. EVENT OPERATOR LIST %d Elements\n"))
#define MSG_EVENT_DELSUBMITHOSTX_IS                  _MESSAGE(43215, _("%d. EVENT DEL SUBMITHOST "SFN"\n"))
#define MSG_EVENT_ADDSUBMITHOSTX_IS                  _MESSAGE(43216, _("%d. EVENT ADD SUBMITHOST "SFN"\n"))
#define MSG_EVENT_MODSUBMITHOSTX_IS                  _MESSAGE(43217, _("%d. EVENT MOD SUBMITHOST "SFN"\n"))
#define MSG_EVENT_SUBMITHOSTLISTXELEMENTS_II         _MESSAGE(43218, _("%d. EVENT SUBMITHOST LIST %d Elements\n"))
#define MSG_EVENT_DELUSERSETX_IS                   _MESSAGE(43219, _("%d. EVENT DEL USER SET "SFN"\n"))
#define MSG_EVENT_ADDUSERSETX_IS                   _MESSAGE(43220, _("%d. EVENT ADD USER SET "SFN"\n"))
#define MSG_EVENT_MODUSERSETX_IS                   _MESSAGE(43221, _("%d. EVENT MOD USER SET "SFN"\n"))
#define MSG_EVENT_USERSETLISTXELEMENTS_II          _MESSAGE(43222, _("%d. EVENT USER SET LIST %d Elements\n"))
#define MSG_EVENT_DELUSERX_IS                      _MESSAGE(43223, _("%d. EVENT DEL USER "SFN"\n"))
#define MSG_EVENT_ADDUSERX_IS                      _MESSAGE(43224, _("%d. EVENT ADD USER "SFN"\n"))
#define MSG_EVENT_MODUSERX_IS                      _MESSAGE(43225, _("%d. EVENT MOD USER "SFN"\n"))
#define MSG_EVENT_USERLISTXELEMENTS_II             _MESSAGE(43226, _("%d. EVENT USER LIST %d Elements\n"))
#define MSG_EVENT_DELPROJECTX_IS                   _MESSAGE(43227, _("%d. EVENT DEL PROJECT "SFN"\n"))
#define MSG_EVENT_ADDPROJECTX_IS                   _MESSAGE(43228, _("%d. EVENT ADD PROJECT "SFN"\n"))
#define MSG_EVENT_MODPROJECTX_IS                   _MESSAGE(43229, _("%d. EVENT MOD PROJECT "SFN"\n"))
#define MSG_EVENT_PROJECTLISTXELEMENTS_II          _MESSAGE(43230, _("%d. EVENT PROJECT LIST %d Elements\n"))
#define MSG_EVENT_DELPEX_IS                        _MESSAGE(43231, _("%d. EVENT DEL PE "SFN"\n"))
#define MSG_EVENT_ADDPEX_IS                        _MESSAGE(43232, _("%d. EVENT ADD PE "SFN"\n"))
#define MSG_EVENT_MODPEX_IS                        _MESSAGE(43233, _("%d. EVENT MOD PE "SFN"\n"))
#define MSG_EVENT_PELISTXELEMENTS_II               _MESSAGE(43234, _("%d. EVENT PE LIST %d Elements\n"))
#define MSG_EVENT_SHUTDOWN_I                       _MESSAGE(43235, _("%d. EVENT SHUTDOWN\n"))
#define MSG_EVENT_QMASTERGOESDOWN_I                _MESSAGE(43236, _("%d. EVENT QMASTER GOES DOWN\n"))
#define MSG_EVENT_TRIGGERSCHEDULERMONITORING_I     _MESSAGE(43237, _("%d. EVENT TRIGGER SCHEDULER MONITORING\n"))
#define MSG_EVENT_SHARETREEXNODESYLEAFS_III        _MESSAGE(43238, _("%d. EVENT SHARETREE %d nodes %d leafs\n"))
#define MSG_EVENT_SCHEDULERCONFIG_I                _MESSAGE(43239, _("%d. EVENT SCHEDULER CONFIG \n"))
#define MSG_EVENT_GLOBAL_CONFIG_I                  _MESSAGE(43240, _("%d. EVENT NEW GLOBAL CONFIG\n"))
#define MSG_EVENT_DELCKPT_IS                       _MESSAGE(43241, _("%d. EVENT DEL CKPT "SFN"\n"))
#define MSG_EVENT_ADDCKPT_IS                       _MESSAGE(43242, _("%d. EVENT ADD CKPT "SFN"\n"))
#define MSG_EVENT_MODCKPT_IS                       _MESSAGE(43243, _("%d. EVENT MOD CKPT "SFN"\n"))
#define MSG_EVENT_CKPTLISTXELEMENTS_II             _MESSAGE(43244, _("%d. EVENT CKPT LIST %d Elements\n"))
#define MSG_EVENT_DELJATASK_UUU                    _MESSAGE(43245, _(U32CFormat". EVENT DEL JATASK "U32CFormat"."U32CFormat"\n"))
#define MSG_EVENT_MODJATASK_UUU                    _MESSAGE(43246, _(U32CFormat". EVENT MOD JATASK "U32CFormat"."U32CFormat"\n"))
#define MSG_EVENT_ADDPETASK_UUUS                   _MESSAGE(43247, _(U32CFormat". EVENT ADD PETASK "U32CFormat"."U32CFormat" task "SFN"\n"))
#define MSG_EVENT_DELPETASK_UUUS                   _MESSAGE(43248, _(U32CFormat". EVENT DEL PETASK "U32CFormat"."U32CFormat" task "SFN"\n"))
#define MSG_EVENT_MODPETASK_UUUS                   _MESSAGE(43249, _(U32CFormat". EVENT MOD PETASK "U32CFormat"."U32CFormat" task "SFN"\n"))
#define MSG_EVENT_NOTKNOWN_I                       _MESSAGE(43250, _("%d. EVENT ????????\n"))
#define MSG_PEREFINJOB_SU                          _MESSAGE(43251, _("Pe "SFQ" is still referenced in job "U32CFormat".\n"))
#define MSG_CKPTREFINJOB_SU                        _MESSAGE(43252, _("Checkpointing object "SFQ" is still referenced in job " U32CFormat".\n"))

#endif /* __MSG_GDILIB_H */

