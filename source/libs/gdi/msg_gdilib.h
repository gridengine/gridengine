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
#define MSG_QSH_GET_CREDSTDERR_S _MESSAGE(43000, _("get_cred stderr: %s"))
#define MSG_QSH_QSUBFAILED    _MESSAGE(43001, _("qsub failed\n"))
#define MSG_QSUB_CANTSTARTCOMMANDXTOGETTOKENQSUBFAILED_S    _MESSAGE(43002, _("can't start command \"%s\" to get token - qsub failed\n"))
#define MSG_QSH_CANTSTARTCOMMANDXTOGETCREDENTIALSQSUBFAILED_S    _MESSAGE(43003, _("can't start command \"%s\" to get credentials - qsub failed\n"))
#define MSG_QSH_CANTGETCREDENTIALS    _MESSAGE(43004, _("warning: could not get credentials\n"))
#define MSG_SEC_NOCRED_USSI           _MESSAGE(43005, _("could not get credentials for job " U32CFormat " for execution host %s - command \"%s\" failed with return code %d\n"))
#define MSG_SEC_STARTDELCREDCMD_SU    _MESSAGE(43006, _("can't start command \"%s\" for job " U32CFormat " to delete credentials\n"))
#define MSG_SEC_NOAUTH_U              _MESSAGE(43007, _("job "U32CFormat" rejected because authentication failed\n"))
#define MSG_SEC_NOSTORECRED_USI       _MESSAGE(43008, _("could not store credentials for job " U32CFormat" - command \"%s\" failed with return code %d\n"))
#define MSG_SEC_NOSTORECREDNOBIN_US   _MESSAGE(43009, _("could not store client credentials for job " U32CFormat" - %s binary does not exist\n"))
#define MSG_SEC_DELCREDSTDERR_S       _MESSAGE(43010, _("delete_cred stderr: %s"))
#define MSG_SEC_DELCREDRETCODE_USI    _MESSAGE(43011, _("could not delete credentials for job " U32CFormat" - command \"%s\" failed with return code %d\n"))
#define MSG_SEC_DELCREDNOBIN_US       _MESSAGE(43012, _("could not delete credentials for job "U32CFormat" - %s binary does not exist\n"))
#define MSG_SEC_KRBAUTHFAILURE        _MESSAGE(43013, _("job "U32CFormat" rejected because authentication failed\n"))
#define MSG_SEC_KRBAUTHFAILUREONHOST  _MESSAGE(43014, _("job "U32CFormat" rejected because authentication failed on host %s\n"))
#define MSG_SEC_PUTCREDSTDERR_S       _MESSAGE(43015, _("put_cred stderr: %s"))
#define MSG_SEC_NOSTARTCMD4GETCRED_SU _MESSAGE(43016, _("can't start command \"%s\" for job " U32CFormat " to get credentials\n"))
#define MSG_SEC_NOCREDNOBIN_US        _MESSAGE(43017, _("could not get client credentials for job " U32CFormat" - %s binary does not exist\n"))
#define MSG_SEC_KRB_CRED_SSSI         _MESSAGE(43018, _("denied: request for user \"%s\" does not match Kerberos credentials for connection <%s,%s,%d>\n") )         
#define MSG_SEC_CRED_SSSI         _MESSAGE(43019, _("denied: request for user \"%s\" does not match credentials for connection <%s,%s,%d>\n") )         
#define MSG_SEC_KRBDECRYPTTGT_US      _MESSAGE(43020, _("could not decrypt TGT for job " U32CFormat "- %s\n"))
#define MSG_SEC_KRBENCRYPTTGT_SSIS    _MESSAGE(43021, _("could not encrypt TGT for client <%s,%s,%d> - %s\n"))
#define MSG_SEC_KRBENCRYPTTGTUSER_SUS _MESSAGE(43022, _("could not encrypt TGT for user %s, job "U32CFormat" - %s"))
#define MSG_SEC_NOUID_SU              _MESSAGE(43023, _("could not get user ID for %s, job "U32CFormat"\n"))


/*
** sge_resource.c
*/
#define MSG_PARSE_NOALLOCREQRES       _MESSAGE(43024, _("unable to alloc space for requested resources\n"))
#define MSG_PARSE_NOALLOCATTRLIST     _MESSAGE(43025, _("unable to alloc space for attrib. list\n"))
#define MSG_PARSE_NOALLOCRESELEM      _MESSAGE(43026, _("unable to alloc space for resource element\n"))
#define MSG_PARSE_NOVALIDSLOTRANGE_S  _MESSAGE(43027, _("\"%s\" must be a valid slot range\n"))
#define MSG_PARSE_NOALLOCATTRELEM     _MESSAGE(43028, _("unable to alloc space for attrib. element\n"))


/*
** sge_parse_date_time.c
*/
#define MSG_PARSE_STARTTIMETOOLONG    _MESSAGE(43029, _("Starttime specifier field length exceeds maximum"))
#define MSG_PARSE_INVALIDSECONDS      _MESSAGE(43030, _("Invalid format of seconds field."))
#define MSG_PARSE_INVALIDHOURMIN      _MESSAGE(43031, _("Invalid format of date/hour-minute field."))
#define MSG_PARSE_INVALIDMONTH        _MESSAGE(43032, _("Invalid month specification."))
#define MSG_PARSE_INVALIDDAY          _MESSAGE(43033, _("Invalid day specification."))
#define MSG_PARSE_INVALIDHOUR         _MESSAGE(43034, _("Invalid hour specification."))
#define MSG_PARSE_INVALIDMINUTE       _MESSAGE(43035, _("Invalid minute specification."))
#define MSG_PARSE_INVALIDSECOND       _MESSAGE(43036, _("Invalid seconds specification."))
#define MSG_PARSE_NODATEFROMINPUT     _MESSAGE(43037, _("Couldn't generate date from input. Perhaps a date before 1970 was specified."))

/*
** parse.c
*/
#define MSG_JOB_XISINVALIDJOBTASKID_S       _MESSAGE(43041, _("ERROR! %s is a invalid job-task identifier\n"))





/* 
** gdilib/gdi_qmod.c
*/ 
#define MSG_GDI_INVALIDACTION          _MESSAGE(43042, _("invalid action" ))
#define MSG_GDI_INVALIDOPTIONFLAG      _MESSAGE(43043, _("invalid option flag"))
#define MSG_GDI_INVALIDIDENCOUNTERED   _MESSAGE(43044, _("encountered invalid id"))
#define MSG_GDI_OUTOFMEMORY            _MESSAGE(43045, _("out of memory"))

/* 
** gdilib/sge_any_request.c
*/ 
#define MSG_GDI_INITSECURITYDATAFAILED                _MESSAGE(43046, _("failed initialize security data\n"))
#define MSG_GDI_INITKERBEROSSECURITYDATAFAILED        _MESSAGE(43047, _("failed initialize Kerberos security data\n"))
#define MSG_GDI_COMMDUP                               _MESSAGE(43049, _("commd is up"))
#define MSG_GDI_COMMDDOWN_S                           _MESSAGE(43050, _("commd is down: %s"))
#define MSG_GDI_RHOSTISNULLFORSENDREQUEST             _MESSAGE(43051, _("parameter rhost = NULL for sge_send_any_request()"))
#define MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS     _MESSAGE(43052, _("can't send %ssynchronous message to commproc (%s:%d) on host \"%s\": %s"))
#define MSG_GDI_RHOSTISNULLFORGETANYREQUEST           _MESSAGE(43053, _("parameter rhost = NULL for sge_get_any_request()"))
#define MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS _MESSAGE(43054, _("can't receive message from commproc (%s:%d) on host \"%s\": %s"))


/* 
** gdilib/sge_parse_num_par.c
*/ 
#define MSG_GDI_VALUETHATCANBESETTOINF                  _MESSAGE(43056, _("value that can be set to infinity"))
#define MSG_GDI_READCONFIGFILESPECGIVENTWICE_SS         _MESSAGE(43057, _("reading config file: specifier \"%s\" given twice for \"%s\"\n"))
#define MSG_GDI_READCONFIGFILEUNKNOWNSPEC_SS            _MESSAGE(43058, _("reading conf file: unknown specifier \"%s\" for %s\n"))
#define MSG_GDI_READCONFIGFILEEMPTYENUMERATION_S        _MESSAGE(43059, _("reading conf file: empty enumeration for \"%s\"\n " ))


/* 
** gdilib/sge_parse_num_val.c
*/ 
#define MSG_GDI_UNRECOGNIZEDVALUETRAILER_SS             _MESSAGE(43060, _("Error! Unrecognized value-trailer '%20s' near '%20s'\nI expected multipliers k, K, m and M.\nThe value string is probably badly formed!\n" ))
#define MSG_GDI_UNEXPECTEDENDOFNUMERICALVALUE_SC        _MESSAGE(43061, _("Error! Unexpected end of numerical value near %s.\nExpected one of ',', '/' or '\\0'. Got '%c'\n" ))
#define MSG_GDI_NUMERICALVALUEFORHOUREXCEEDED_SS         _MESSAGE(43062, _("Error! numerical value near %20s for hour exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALID_SS                _MESSAGE(43063, _("Error! numerical value near %20s invalid.\n'%20s' is no valid time specifier!\n" ))
#define MSG_GDI_NUMERICALVALUEFORMINUTEEXCEEDED_SS       _MESSAGE(43064, _("Error! numerical value near %20s for minute exceeded.\n'%20s' is no valid time specifier!\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNONUMBER_SS         _MESSAGE(43065, _("Error! numerical value near %20s invalid.\n>%20s< contains no valid decimal or fixed float number\n"))
#define MSG_GDI_NUMERICALVALUEINVALIDNOHEXOCTNUMBER_SS      _MESSAGE(43066, _("Error! numerical value near %s invalid.\n'%s' contains no valid hex or octal number\n"))



/* 
** gdilib/sge_qexec.c
*/ 
#define MSG_GDI_RETRIEVINGLOCALHOSTNAMEFAILED_S          _MESSAGE(43067, _("failed retrieving local hostname: %s"))
#define MSG_GDI_RESOLVINGUIDTOUSERNAMEFAILED_IS          _MESSAGE(43068, _("failed resolving uid %d to username: %s"))
#define MSG_GDI_MISSINGJOBIDENVIRONMENT                  _MESSAGE(43069, _("missing JOB_ID in environment"))
#define MSG_GDI_STRINGINJOBIDISINVALID_S                 _MESSAGE(43070, _("string \"%s\" in env var JOB_ID is not a valid jobid"))
#define MSG_GDI_PROPOSEDTASKIDINVALID_SS                 _MESSAGE(43071, _("proposed task id %s \"%s\" is invalid"))
#define MSG_GDI_TASKEXISTS_S                             _MESSAGE(43072, _("task %s already exists"))
#define MSG_GDI_SETUPGDILIBFAILED                        _MESSAGE(43073, _("failed setting up gdi library"))
#define MSG_GDI_SENDTASKTOEXECDFAILED_SS                 _MESSAGE(43074, _("failed sending task to execd@%s: %s"                  ))
#define MSG_GDI_TASKNOTEXIST_I                           _MESSAGE(43075, _("task %d does not exist"))
#define MSG_GDI_RCVFROMEXECLOCALCOMMDFAILED_S            _MESSAGE(43076, _("rcv_from_exec(): failed enrolling to local sge commd: %s"))
#define MSG_GDI_MESSAGERECEIVEFAILED_SI                  _MESSAGE(43077, _("failed receiving message from execd: %s %d"))
#define MSG_GDI_TASKNOTFOUND_S                           _MESSAGE(43078, _("cannot find task with taskid \"%s\"\n"))
#define MSG_GDI_TASKNOTFOUNDNOIDGIVEN_S                  _MESSAGE(43079, _("cannot find task without taskid - should become task \"%s\""))



/* 
** gdilib/sge_report.c
*/
#define MSG_GDI_REPORTNOMEMORY_I                         _MESSAGE(43080, _("not enough memory for packing report: %d bytes\n"))
#define MSG_GDI_REPORTFORMATERROR                        _MESSAGE(43081, _("format error while packing report\n"))
#define MSG_GDI_REPORTUNKNOWERROR                        _MESSAGE(43082, _("unexpected error while packing report\n"))


/* 
** gdilib/config.c
*/
#define MSG_GDI_CONFIGNOARGUMENTGIVEN_S                  _MESSAGE(43083, _("no argument given in config option: %s\n"))
#define MSG_GDI_CONFIGMISSINGARGUMENT_S                  _MESSAGE(43084, _("missing configuration attribute \"%s\""))
#define MSG_GDI_CONFIGADDLISTFAILED_S                    _MESSAGE(43085, _("can't add \"%s\" to list"))
#define MSG_GDI_CONFIGARGUMENTNOTINTEGER_SS              _MESSAGE(43086, _("value for attribute %s \"%s\" is not an integer\n"))
#define MSG_GDI_CONFIGARGUMENTNOTDOUBLE_SS               _MESSAGE(43087, _("value for attribute %s \"%s\" is not a double\n"))
#define MSG_GDI_CONFIGARGUMENTNOTTIME_SS                 _MESSAGE(43088, _("value for attribute %s \"%s\" is not time\n"))
#define MSG_GDI_CONFIGARGUMENTNOMEMORY_SS                _MESSAGE(43089, _("value for attribute %s \"%s\" is not memory\n"))
#define MSG_GDI_CONFIGINVALIDQUEUESPECIFIED              _MESSAGE(43090, _("reading conf file: invalid queue type specified\n"))
#define MSG_GDI_CONFIGREADFILEERRORNEAR_SS               _MESSAGE(43091, _("reading conf file: %s error near \"%s\"\n"))




/* 
** gdilib/parse_range.c
*/
#define MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S          _MESSAGE(43092, _("Numerical value invalid!\nThe initial portion of string \"%s\" contains no decimal number\n"))
#define MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS      _MESSAGE(43093, _("Range specifier \"%s\" has unknown trailer \"%s\"\n"))
#define MSG_GDI_UNEXPECTEDRANGEFOLLOWINGUNDEFINED        _MESSAGE(43094, _("unexpected range following \"UNDEFINED\"\n"))
#define MSG_GDI_UNEXPECTEDUNDEFINEDFOLLOWINGRANGE        _MESSAGE(43095, _("unexpected \"UNDEFINED\" following range\n"))


/* 
** gdilib/sge_feature.c
*/
#define MSG_GDI_PRODUCTMODENOTSETFORFILE_S               _MESSAGE(43096, _("can't read \"%s\" - product mode not set."))
#define MSG_GDI_INVALIDPRODUCTMODESTRING_S               _MESSAGE(43097, _("invalid product mode string "SFQ"\n"))
#define MSG_GDI_CORRUPTPRODMODFILE_S                     _MESSAGE(43098, _("product mode file "SFQ" is incorrect\n"))
#define MSG_GDI_SWITCHFROMTO_SS                          _MESSAGE(43099, _("switching from "SFQ" to "SFQ" feature set\n"))

/* 
** gdilib/qm_name.c
*/
#define MSG_GDI_NULLPOINTERPASSED                        _MESSAGE(43100, _("NULL pointer passed to \"master_host\" or \"master_file\""))
#define MSG_GDI_OPENMASTERFILEFAILED_S                   _MESSAGE(43101, _("can't open \"%s\" for reading qmaster hostname"))
#define MSG_GDI_READMASTERHOSTNAMEFAILED_S               _MESSAGE(43102, _("can't read qmaster hostname in \"%s\""))
#define MSG_GDI_MASTERHOSTNAMEHASZEROLENGTH_S            _MESSAGE(43103, _("qmaster hostname in \"%s\" has zero length"))
#define MSG_GDI_MASTERHOSTNAMEEXCEEDSCHARS_SI            _MESSAGE(43104, _("qmaster hostname in \"%s\" exceeds %d characters\n"))
#define MSG_GDI_OPENWRITEMASTERHOSTNAMEFAILED_SS         _MESSAGE(43105, _("can't open \"%s\" for writing qmaster hostname: %s"))
#define MSG_GDI_WRITEMASTERHOSTNAMEFAILED_S              _MESSAGE(43106, _("can't write qmaster hostname into \"%s\""))
#define MSG_GDI_FOPEN_FAILED                             _MESSAGE(43107, _("fopen("SFQ") failed: %s\n"))


/* 
** gdilib/resolve.c
*/
#define MSG_GDI_READMASTERNAMEFAILED_S                   _MESSAGE(43108, _("unable to read qmaster name: %s"))



/* 
** gdilib/setup.c
*/
#define MSG_GDI_HOSTCMPPOLICYNOTSETFORFILE_S             _MESSAGE(43109, _("can't read \"%s\" - host compare policy not set."))
#define MSG_GDI_NOVALIDSGECOMPRESSIONLEVEL_S             _MESSAGE(43110, _("%s is not a valid SGE_COMPRESSION_LEVEL\n"))
#define MSG_GDI_SETCOMPRESSIONLEVEL_D                    _MESSAGE(43111, _("Setting compression level to "U32CFormat"\n"))
#define MSG_GDI_NOVALIDSGECOMPRESSIONTHRESHOLD_S         _MESSAGE(43112, _("%s is not a valid SGE_COMPRESSION_THRESHOLD\n"))
#define MSG_GDI_SETCOMPRESSIONTHRESHOLD_D                _MESSAGE(43113, _("Setting compression threshold to "U32CFormat"\n"))


/* 
** gdilib/setup_path.c
*/
#define MSG_GDI_SGEROOTNOTADIRECTORY_S                   _MESSAGE(43114, _("$SGE_ROOT=%s is not a directory\n"))
#define MSG_GDI_DIRECTORYNOTEXIST_S                      _MESSAGE(43115, _("directory doesn't exist: %s\n"))
#define MSG_SGETEXT_NOSGECELL_S                _MESSAGE(43116, _("cell directory "SFQ" doesn't exist\n"))


/* 
** gdilib/usage.c
*/
#define MSG_GDI_ARGUMENTSYNTAX_OA_ACCOUNT_STRING       "account_string          account_name"
#define MSG_GDI_ARGUMENTSYNTAX_OA_COMPLEX_LIST         "complex_list            complex[,complex,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_CONTEXT_LIST         "context_list            variable[=value][,variable[=value],...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_CKPT_SEL             "ckpt_selector           `n' `s' `m' `x' <interval> "
#define MSG_GDI_ARGUMENTSYNTAX_OA_DATE_TIME            "date_time               [[CC]YY]MMDDhhmm[.SS]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_DESTIN_ID_LIST       "destin_id_list          queue[ queue ...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_HOLD_LIST            "hold_list               `n' `u' `s' `o' `U' `S' `O'"
#define MSG_GDI_ARGUMENTSYNTAX_OA_HOST_ID_LIST         "host_id_list            host[ host ...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_JOB_ID_LIST          "job_id_list             job_id[,job_id,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_JOB_IDENTIFIER_LIST  "job_identifier_list     {job_id|job_name}[,{job_id|job_name},...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_JOB_QUEUE_DEST       "job_queue_list          {job|queue}[{,| }{job|queue}{,| }...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_LISTNAME_LIST        "listname_list           listname[,listname,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_ADDRESS         "mail_address            username[@host]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_LIST            "mail_list               mail_address[,mail_address,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_MAIL_OPTIONS         "mail_options            `e' `b' `a' `n' `s'"
#define MSG_GDI_ARGUMENTSYNTAX_OA_NODE_LIST            "node_list               node_path[,node_path,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_NODE_PATH            "node_path               [/]node_name[[/.]node_name...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_NODE_SHARES_LIST     "node_shares_list        node_path=shares[,node_path=shares,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_PATH_LIST            "path_list               [host:]path[,[host:]path,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_PRIORITY             "priority                -1023 - 1024"
#define MSG_GDI_ARGUMENTSYNTAX_OA_RESOURCE_LIST        "resource_list           resource[=value][,resource[=value],...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_SERVER               "server                  hostname"
#define MSG_GDI_ARGUMENTSYNTAX_OA_SERVER_LIST          "server_list             server[,server,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_SIGNAL               "signal                  -int_val, symbolic names"
#define MSG_GDI_ARGUMENTSYNTAX_OA_SIMPLE_CONTEXT_LIST  "simple_context_list     variable[,variable,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_SLOT_RANGE           "slot_range              [n[-m]|[-]m] - n,m > 0"
#define MSG_GDI_ARGUMENTSYNTAX_OA_STATES               "states                  `e' `q' `r' `t' `h' `w' `m' `s'"
#define MSG_GDI_ARGUMENTSYNTAX_OA_JOB_TASK_LIST        "job_task_list           job_tasks [,job_tasks, ...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_JOB_TASKS            "job_tasks               job_id['.'task_id_range]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_TASK_ID_RANGE        "task_id_range           task_id['-'task_id[':'step]]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_USER_LIST            "user_list               user[,user,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_VARIABLE_LIST        "variable_list           variable[=value][,variable[=value],...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_PROJECT_LIST         "project_list            project[,project,...]"
#define MSG_GDI_ARGUMENTSYNTAX_OA_OBJECT_NAME          "obj_nm                  \"queue\"|\"exechost\"|\"pe\"|\"ckpt\""
#define MSG_GDI_ARGUMENTSYNTAX_OA_ATTRIBUTE_NAME       "attr_nm                 (see man pages)"
#define MSG_GDI_ARGUMENTSYNTAX_OA_OBJECT_ID_LIST       "obj_id_lst              objectname [ objectname ...]" 
#define MSG_GDI_ARGUMENTSYNTAX_OA_EVENTCLIENT_LIST     "evid_list               all | evid[,evid,...]" 
#define MSG_GDI_ARGUMENTSYNTAX_OA_HOST_LIST     "host_list               all | hostname[,hostname,...]" 




#define MSG_GDI_USAGE_USAGESTRING                     _MESSAGE(43117, _("usage:"))

#define MSG_GDI_USAGE_a_OPT_DATE_TIME                    "[-a date_time]"
#define MSG_GDI_UTEXT_a_OPT_DATE_TIME                    _MESSAGE(43118, _("request a job start time"))


#define MSG_GDI_USAGE_ac_OPT_COMPLEX_NAME                "[-ac complex_name]"
#define MSG_GDI_UTEXT_ac_OPT_COMPLEX_NAME                _MESSAGE(43119, _("add a new complex" ))

#define MSG_GDI_USAGE_ac_OPT_CONTEXT_LIST                "[-ac context_list]"
#define MSG_GDI_UTEXT_ac_OPT_CONTEXT_LIST                _MESSAGE(43120, _("add context variable(s)"))

#define MSG_GDI_USAGE_Ac_OPT_COMPLEX_NAME_FNAME          "[-Ac complex_name fname]"
#define MSG_GDI_UTEXT_Ac_OPT_COMPLEX_NAME_FNAME          _MESSAGE(43121, _("add a new complex from file"))

#define MSG_GDI_USAGE_acal_OPT_FNAME                     "[-acal calendar_name]"
#define MSG_GDI_UTEXT_acal_OPT_FNAME                     _MESSAGE(43122, _("add a new calendar"))

#define MSG_GDI_USAGE_Acal_OPT_FNAME                     "[-Acal fname]"
#define MSG_GDI_UTEXT_Acal_OPT_FNAME                     _MESSAGE(43123, _("add a new calendar from file"))

#define MSG_GDI_USAGE_ackpt_OPT_CKPT_NAME                "[-ackpt ckpt_name]"
#define MSG_GDI_UTEXT_ackpt_OPT_CKPT_NAME                _MESSAGE(43124, _("add a ckpt interface definition"))

#define MSG_GDI_USAGE_Ackpt_OPT_FNAME                    "[-Ackpt fname]"
#define MSG_GDI_UTEXT_Ackpt_OPT_FNAME                    _MESSAGE(43125, _("add a ckpt interface definition from file"))

#define MSG_GDI_USAGE_aconf_OPT_HOST_LIST                "[-aconf host_list]"
#define MSG_GDI_UTEXT_aconf_OPT_HOST_LIST                _MESSAGE(43126, _("add configurations"))

#define MSG_GDI_USAGE_Aconf_OPT_FILE_LIST                "[-Aconf file_list]"
#define MSG_GDI_UTEXT_Aconf_OPT_FILE_LIST                _MESSAGE(43127, _("add configurations from file_list"))

#define MSG_GDI_USAGE_ae_OPT_EXEC_SERVER_TEMPLATE        "[-ae [exec_server_template]]"
#define MSG_GDI_UTEXT_ae_OPT_EXEC_SERVER_TEMPLATE        _MESSAGE(43128, _("add an exec host using a template"))

#define MSG_GDI_USAGE_Ae_OPT_FNAME                       "[-Ae fname]"
#define MSG_GDI_UTEXT_Ae_OPT_FNAME                       _MESSAGE(43129, _("add an exec host from file"))

#define MSG_GDI_USAGE_ah_OPT_HOSTNAME                    "[-ah hostname]"
#define MSG_GDI_UTEXT_ah_OPT_HOSTNAME                    _MESSAGE(43130, _("add an administrative host"))

#define MSG_GDI_USAGE_am_OPT_USER_LIST                   "[-am user_list]"
#define MSG_GDI_UTEXT_am_OPT_USER_LIST                   _MESSAGE(43131, _("add user to manager list"))

#define MSG_GDI_USAGE_ao_OPT_USER_LIST                   "[-ao user_list]"
#define MSG_GDI_UTEXT_ao_OPT_USER_LIST                   _MESSAGE(43132, _("add user to operator list"))

#define MSG_GDI_USAGE_ap_OPT_PE_NAME                     "[-ap pe-name]"
#define MSG_GDI_UTEXT_ap_OPT_PE_NAME                     _MESSAGE(43133, _("add a new parallel environment"))

#define MSG_GDI_USAGE_Ap_OPT_FNAME                       "[-Ap fname]"
#define MSG_GDI_UTEXT_Ap_OPT_FNAME                       _MESSAGE(43134, _("add a new parallel environment from file"))

#define MSG_GDI_USAGE_aq_OPT_Q_TEMPLATE                  "[-aq [q_template]]"
#define MSG_GDI_UTEXT_aq_OPT_Q_TEMPLATE                  _MESSAGE(43135, _("add a new queue using the given template"))

#define MSG_GDI_USAGE_as_OPT_HOSTNAME                    "[-as hostname]"
#define MSG_GDI_UTEXT_as_OPT_HOSTNAME                    _MESSAGE(43136, _("add a submit host"))

#define MSG_GDI_USAGE_ASTNODE_NODE_SHARES_LIST           "[-astnode node_shares_list]"
#define MSG_GDI_UTEXT_ASTNODE_NODE_SHARES_LIST           _MESSAGE(43137, _("add sharetree node(s)"))

#define MSG_GDI_USAGE_ASTREE                             "[-astree]"
#define MSG_GDI_UTEXT_ASTREE                             _MESSAGE(43138, _("create/modify the sharetree"))

#define MSG_GDI_USAGE_au_OPT_USER_LIST_LISTNAME_LIST     "[-au user_list listname_list]"
#define MSG_GDI_UTEXT_au_OPT_USER_LIST_LISTNAME_LIST     _MESSAGE(43139, _("add user(s) to userset list(s)"))

#define MSG_GDI_USAGE_Au_OPT_LISTNAME_LIST               "[-Au fname]"
#define MSG_GDI_UTEXT_Au_OPT_LISTNAME_LIST               _MESSAGE(43140, _("add userset from file"))

#define MSG_GDI_USAGE_AUSER                              "[-auser]"
#define MSG_GDI_UTEXT_AUSER                              _MESSAGE(43141, _("add user"))

#define MSG_GDI_USAGE_Auser                              "[-Auser fname]"
#define MSG_GDI_UTEXT_Auser                              _MESSAGE(43142, _("add user from file"))

#define MSG_GDI_USAGE_APRJ                               "[-aprj]"
#define MSG_GDI_UTEXT_APRJ                               _MESSAGE(43143, _("add project"))

#define MSG_GDI_USAGE_Aprj                               "[-Aprj fname]"
#define MSG_GDI_UTEXT_Aprj                               _MESSAGE(43144, _("add project from file"))

#define MSG_GDI_USAGE_Mprj_OPT_PROJECT                   "[-Mprj fname]"
#define MSG_GDI_UTEXT_Mprj_OPT_PROJECT                   _MESSAGE(43145, _("modify project from file"))

#define MSG_GDI_USAGE_A_OPT_ACCOUNT_STRING               "[-A account_string]"
#define MSG_GDI_UTEXT_A_OPT_ACCOUNT_STRING               _MESSAGE(43146, _("use account at host"))

#define MSG_GDI_USAGE_Aq_OPT_FNAME                       "[-Aq fname]"
#define MSG_GDI_UTEXT_Aq_OPT_FNAME                       _MESSAGE(43147, _("add a queue from file"))

#define MSG_GDI_USAGE_c_OPT_CKPT_SELECTOR                "[-c ckpt_selector]"
#define MSG_GDI_UTEXT_c_OPT_CKPT_SELECTOR                _MESSAGE(43148, _("define type of checkpointing for job"))

#define MSG_GDI_USAGE_c_OPT                              "[-c]"
#define MSG_GDI_UTEXT_c_OPT                              _MESSAGE(43149, _("clear queue error state"))

#define MSG_GDI_USAGE_ckpt_OPT_CKPT_NAME                 "[-ckpt ckpt-name]"
#define MSG_GDI_UTEXT_ckpt_OPT_CKPT_NAME                 _MESSAGE(43150, _("request checkpoint method"))

#define MSG_GDI_USAGE_clear_OPT                          "[-clear]"
#define MSG_GDI_UTEXT_clear_OPT                          _MESSAGE(43151, _("skip previous definitions for job"))

#define MSG_GDI_USAGE_clearusage_OPT                     "[-clearusage]"
#define MSG_GDI_UTEXT_clearusage_OPT                     _MESSAGE(43152, _("clear all user/project sharetree usage"))

#define MSG_GDI_USAGE_cwd_OPT                            "[-cwd]"
#define MSG_GDI_UTEXT_cwd_OPT                            _MESSAGE(43153, _("use current working directory"))

#define MSG_GDI_USAGE_cq_OPT_DESTIN_ID_LIST              "[-cq destin_id_list]"
#define MSG_GDI_UTEXT_cq_OPT_DESTIN_ID_LIST              _MESSAGE(43154, _("clean queue"))

#define MSG_GDI_USAGE_C_OPT_DIRECTIVE_PREFIX             "[-C directive_prefix]"
#define MSG_GDI_UTEXT_C_OPT_DIRECTIVE_PREFIX             _MESSAGE(43155, _("define command prefix for job script"))

#define MSG_GDI_USAGE_d_OPT                              "[-d]"
#define MSG_GDI_UTEXT_d_OPT                              _MESSAGE(43156, _("disable"))

#define MSG_GDI_USAGE_dc_OPT_COMPLEXNAME                 "[-dc complexname]"
#define MSG_GDI_UTEXT_dc_OPT_COMPLEXNAME                 _MESSAGE(43157, _("remove a complex"))

#define MSG_GDI_USAGE_dc_OPT_SIMPLE_COMPLEX_LIST         "[-dc simple_context_list]"
#define MSG_GDI_UTEXT_dc_OPT_SIMPLE_COMPLEX_LIST         _MESSAGE(43158, _("remove context variable(s)"))

#define MSG_GDI_USAGE_dcal_OPT_CALENDAR_NAME             "[-dcal calendar_name]"
#define MSG_GDI_UTEXT_dcal_OPT_CALENDAR_NAME             _MESSAGE(43159, _("remove a calendar"))

#define MSG_GDI_USAGE_dckpt_OPT_CKPT_NAME                "[-dckpt ckpt_name]"
#define MSG_GDI_UTEXT_dckpt_OPT_CKPT_NAME                _MESSAGE(43160, _("remove a ckpt interface definition"))

#define MSG_GDI_USAGE_dconf_OPT_HOST_LIST                "[-dconf host_list]"
#define MSG_GDI_UTEXT_dconf_OPT_HOST_LIST                _MESSAGE(43161, _("delete local configurations"))

#define MSG_GDI_USAGE_de_OPT_HOST_LIST                   "[-de host_list]"
#define MSG_GDI_UTEXT_de_OPT_HOST_LIST                   _MESSAGE(43162, _("remove an exec server"))

#define MSG_GDI_USAGE_display_OPT_DISPLAY                "[-display display]"
#define MSG_GDI_UTEXT_display_OPT_DISPLAY                _MESSAGE(43163, _("set display to display interactive job"))

#define MSG_GDI_USAGE_dh_OPT_HOST_LIST                   "[-dh host_list]"
#define MSG_GDI_UTEXT_dh_OPT_HOST_LIST                   _MESSAGE(43164, _("remove an administrative host"))

#define MSG_GDI_USAGE_dl_OPT_DATE_TIME                   "[-dl date_time]"
#define MSG_GDI_UTEXT_dl_OPT_DATE_TIME                   _MESSAGE(43165, _("request a deadline initiation time"))

#define MSG_GDI_USAGE_dm_OPT_USER_LIST                   "[-dm user_list]"
#define MSG_GDI_UTEXT_dm_OPT_USER_LIST                   _MESSAGE(43166, _("remove user from manager list"))

#define MSG_GDI_USAGE_do_OPT_USER_LIST                   "[-do user_list]"
#define MSG_GDI_UTEXT_do_OPT_USER_LIST                   _MESSAGE(43167, _("remove user from operator list"))

#define MSG_GDI_USAGE_dp_OPT_PE_NAME                     "[-dp pe-name]"
#define MSG_GDI_UTEXT_dp_OPT_PE_NAME                     _MESSAGE(43168, _("remove a parallel environment"))

#define MSG_GDI_USAGE_dq_OPT_DESTIN_ID_LIST              "[-dq destin_id_list]"
#define MSG_GDI_UTEXT_dq_OPT_DESTIN_ID_LIST              _MESSAGE(43169, _("remove a queue"))

#define MSG_GDI_USAGE_ds_OPT_HOST_LIST                   "[-ds host_list]"
#define MSG_GDI_UTEXT_ds_OPT_HOST_LIST                   _MESSAGE(43170, _("remove submit host"))

#define MSG_GDI_USAGE_DSTNODE_NODELIST                   "[-dstnode node_list]"
#define MSG_GDI_UTEXT_DSTNODE_NODELIST                   _MESSAGE(43171, _("remove sharetree node(s)"))

#define MSG_GDI_USAGE_DSTREE                             "[-dstree]"
#define MSG_GDI_UTEXT_DSTREE                             _MESSAGE(43172, _("delete the sharetree"))

#define MSG_GDI_USAGE_du_OPT_USER_LIST_LISTNAME_LIST     "[-du user_list listname_list]"
#define MSG_GDI_UTEXT_du_OPT_USER_LIST_LISTNAME_LIST     _MESSAGE(43173, _("remove user(s) from userset list(s)"))

#define MSG_GDI_USAGE_dul_OPT_LISTNAME_LIST              "[-dul listname_list]"
#define MSG_GDI_UTEXT_dul_OPT_LISTNAME_LIST              _MESSAGE(43174, _("remove userset list(s) completely"))

#define MSG_GDI_USAGE_DUSER_USER                         "[-duser user_list]"
#define MSG_GDI_UTEXT_DUSER_USER                         _MESSAGE(43175, _("delete user"))

#define MSG_GDI_USAGE_dprj_OPT_PROJECT                   "[-dprj project_list]"
#define MSG_GDI_UTEXT_dprj_OPT_PROJECT                   _MESSAGE(43176, _("delete project"))

#define MSG_GDI_USAGE_e_OPT                              "[-e]"
#define MSG_GDI_UTEXT_e_OPT                              _MESSAGE(43177, _("enable"))

#define MSG_GDI_USAGE_e_OPT_PATH_LIST                    "[-e path_list]"
#define MSG_GDI_UTEXT_e_OPT_PATH_LIST                    _MESSAGE(43178, _("specify standard error stream path(s)"))

#define MSG_GDI_USAGE_ext_OPT                            "[-ext]"
#define MSG_GDI_UTEXT_ext_OPT                            _MESSAGE(43179, _("view also scheduling attributes"))

#define MSG_GDI_USAGE_f_OPT                              "[-f]"
#define MSG_GDI_UTEXT_f_OPT_FULL_OUTPUT                  _MESSAGE(43180, _("full output"))
#define MSG_GDI_UTEXT_f_OPT_FORCE_ACTION                 _MESSAGE(43181, _("force action"                              ))

#define MSG_GDI_USAGE_h_OPT_HOLD_LIST                    "[-h hold_list]"
#define MSG_GDI_UTEXT_h_OPT_HOLD_LIST                    _MESSAGE(43182, _("assign holds for jobs or tasks"))

#define MSG_GDI_USAGE_h_OPT                              "[-h]"
#define MSG_GDI_UTEXT_h_OPT                              _MESSAGE(43183, _("place user hold on job"))

#define MSG_GDI_USAGE_hard_OPT                           "[-hard]"
#define MSG_GDI_UTEXT_hard_OPT                           _MESSAGE(43184, _("consider following requests \"hard\""))

#define MSG_GDI_USAGE_help_OPT                           "[-help]"
#define MSG_GDI_UTEXT_help_OPT                           _MESSAGE(43185, _("print this help"))

#define MSG_GDI_USAGE_hold_jid_OPT                       "[-hold_jid job_identifier_list]"
#define MSG_GDI_UTEXT_hold_jid_OPT                       _MESSAGE(43186, _("define jobnet interdependencies"))

#define MSG_GDI_USAGE_j_OPT_YN                           "[-j y|n]"
#define MSG_GDI_UTEXT_j_OPT_YN                           _MESSAGE(43187, _("merge stdout and stderr stream of job"))

#define MSG_GDI_USAGE_jid_OPT_JOB_ID_LIST                "[-jid job_id_list]"
#define MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_PRINTED        _MESSAGE(43188, _("jobs to be printed"))
#define MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_ALTERED        _MESSAGE(43189, _("jobs to be altered"))

#define MSG_GDI_USAGE_jid_OPT_JID                        "[-jid jid]"


#define MSG_GDI_USAGE_ke_OPT_HOSTS                       "[-ke[j] host_list"
#define MSG_GDI_UTEXT_ke_OPT_HOSTS                       _MESSAGE(43190, _("shutdown execution daemon(s)"))

#define MSG_GDI_USAGE_k_OPT_MASTERORSCHEDULINGDAEMON     "[-k{m|s}]"
#define MSG_GDI_UTEXT_k_OPT_MASTERORSCHEDULINGDAEMON     _MESSAGE(43191, _("shutdown master|scheduling daemon"))

#define MSG_GDI_USAGE_kqs_OPT                            "[-kqs]"
                         

#define MSG_GDI_USAGE_l_OPT_RESOURCE_LIST                "[-l resource_list]"
#define MSG_GDI_UTEXT_l_OPT_RESOURCE_LIST                _MESSAGE(43192, _("request the given resources"))

#define MSG_GDI_USAGE_lj_OPT_LOG_FILE                    "[-lj log_file]"
#define MSG_GDI_UTEXT_lj_OPT_LOG_FILE                    _MESSAGE(43193, _("write job logging to log file"))

#define MSG_GDI_USAGE_m_OPT_MAIL_OPTIONS                 "[-m mail_options]"
#define MSG_GDI_UTEXT_m_OPT_MAIL_OPTIONS                 _MESSAGE(43194, _("define mail notification events"))

#define MSG_GDI_USAGE_mc_OPT_COMPLEX                     "[-mc [complex]]"
#define MSG_GDI_UTEXT_mc_OPT_COMPLEX                     _MESSAGE(43195, _("modify a complex"))

#define MSG_GDI_USAGE_mckpt_OPT_CKPT_NAME                "[-mckpt ckpt_name]"
#define MSG_GDI_UTEXT_mckpt_OPT_CKPT_NAME                _MESSAGE(43196, _("modify a ckpt interface definition"))

#define MSG_GDI_USAGE_Mc_OPT_COMPLEX_NAME_FNAME          "[-Mc complex_name fname]"
#define MSG_GDI_UTEXT_Mc_OPT_COMPLEX_NAME_FNAME          _MESSAGE(43197, _("modify a complex from file"))

#define MSG_GDI_USAGE_mcal_OPT_CALENDAR_NAME             "[-mcal calendar_name]"
#define MSG_GDI_UTEXT_mcal_OPT_CALENDAR_NAME             _MESSAGE(43198, _("modify calendar"))

#define MSG_GDI_USAGE_Mcal_OPT_FNAME                     "[-Mcal fname]"
#define MSG_GDI_UTEXT_Mcal_OPT_FNAME                     _MESSAGE(43199, _("modify calendar from file"))

#define MSG_GDI_USAGE_Mckpt_OPT_FNAME                    "[-Mckpt fname]"
#define MSG_GDI_UTEXT_Mckpt_OPT_FNAME                    _MESSAGE(43200, _("modify a ckpt interface definition from file"))

#define MSG_GDI_USAGE_mconf_OPT_HOSTLISTORGLOBAL         "[-mconf [host_list|global]]"
#define MSG_GDI_UTEXT_mconf_OPT_HOSTLISTORGLOBAL         _MESSAGE(43201, _("modify configurations"))

#define MSG_GDI_USAGE_msconf_OPT                         "[-msconf]"
#define MSG_GDI_UTEXT_msconf_OPT                         _MESSAGE(43202, _("modify scheduler configuration"))

#define MSG_GDI_USAGE_me_OPT_SERVER                      "[-me server]"
#define MSG_GDI_UTEXT_me_OPT_SERVER                      _MESSAGE(43203, _("modify exec server"))

#define MSG_GDI_USAGE_Me_OPT_FNAME                       "[-Me fname]"
#define MSG_GDI_UTEXT_Me_OPT_FNAME                       _MESSAGE(43204, _("modify exec server from file"))

#define MSG_GDI_USAGE_mp_OPT_PE_NAME                     "[-mp pe-name]"  
#define MSG_GDI_UTEXT_mp_OPT_PE_NAME                     _MESSAGE(43205, _("modify a parallel environment"))

#define MSG_GDI_USAGE_Mp_OPT_FNAME                       "[-Mp fname]"
#define MSG_GDI_UTEXT_Mp_OPT_FNAME                       _MESSAGE(43206, _("modify a parallel environment from file"))

#define MSG_GDI_USAGE_mq_OPT_QUEUE                       "[-mq [queue]]"
#define MSG_GDI_UTEXT_mq_OPT_QUEUE                       _MESSAGE(43207, _("modify a queue"))

#define MSG_GDI_USAGE_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST   "[-mqattr attr_name value destin_id_list]"
#define MSG_GDI_UTEXT_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST   _MESSAGE(43208, _("modify particular queue attributes"))

#define MSG_GDI_USAGE_Mq_OPT_FNAME                       "[-Mq fname]"
#define MSG_GDI_UTEXT_Mq_OPT_FNAME                       _MESSAGE(43209, _("modify a queue from file"))

#define MSG_GDI_USAGE_Mqattr_OPT_FNAME_DESTIN_ID_LIST    "[-Mqattr fname destin_id_list]"
#define MSG_GDI_UTEXT_Mqattr_OPT_FNAME_DESTIN_ID_LIST    _MESSAGE(43210, _("modify particular queue attributes from file"))

#define MSG_GDI_USAGE_mu_OPT_LISTNAME_LIST               "[-mu listname_list]"
#define MSG_GDI_UTEXT_mu_OPT_LISTNAME_LIST               _MESSAGE(43211, _("modify the given userset list"))

#define MSG_GDI_USAGE_Mu_OPT_LISTNAME_LIST               "[-Mu fname]"
#define MSG_GDI_UTEXT_Mu_OPT_LISTNAME_LIST               _MESSAGE(43212, _("modify userset from file"))

#define MSG_GDI_USAGE_muser_OPT_USER                     "[-muser user]"
#define MSG_GDI_UTEXT_muser_OPT_USER                     _MESSAGE(43213, _("modify a user"))

#define MSG_GDI_USAGE_Muser_OPT_USER                     "[-Muser fname]"
#define MSG_GDI_UTEXT_Muser_OPT_USER                     _MESSAGE(43214, _("modify a user from file"))

#define MSG_GDI_USAGE_mprj_OPT_PROJECT                   "[-mprj project]"
#define MSG_GDI_UTEXT_mprj_OPT_PROJECT                   _MESSAGE(43215, _("modify a project"))

#define MSG_GDI_USAGE_MSTNODE_NODE_SHARES_LIST           "[-mstnode node_shares_list]"
#define MSG_GDI_UTEXT_MSTNODE_NODE_SHARES_LIST           _MESSAGE(43216, _("modify sharetree node(s)"))

#define MSG_GDI_USAGE_MSTREE                             "[-mstree]"
#define MSG_GDI_UTEXT_MSTREE                             _MESSAGE(43217, _("modify/create the sharetree"))

#define MSG_GDI_USAGE_notify_OPT                         "[-notify]"
#define MSG_GDI_UTEXT_notify_OPT                         _MESSAGE(43218, _("notify job before killing/suspending it"))

#define MSG_GDI_USAGE_now_OPT_YN                         "[-now y[es]|n[o]]"
#define MSG_GDI_UTEXT_now_OPT_YN                         _MESSAGE(43219, _("start job immediately or not at all"))

#define MSG_GDI_USAGE_M_OPT_MAIL_LIST                    "[-M mail_list]"
#define MSG_GDI_UTEXT_M_OPT_MAIL_LIST                    _MESSAGE(43220, _("notify these e-mail addresses"))

#define MSG_GDI_USAGE_N_OPT_NAME                         "[-N name]"
#define MSG_GDI_UTEXT_N_OPT_NAME                         _MESSAGE(43221, _("specify job name"))

#define MSG_GDI_USAGE_o_OPT_PATH_LIST                    "[-o path_list]"
#define MSG_GDI_UTEXT_o_OPT_PATH_LIST                    _MESSAGE(43222, _("specify standard output stream path(s)"))

#define MSG_GDI_USAGE_ot_OPT_TICKETS                     "[-ot tickets]"
#define MSG_GDI_UTEXT_ot_OPT_TICKETS                     _MESSAGE(43223, _("set job's override tickets"))

#define MSG_GDI_USAGE_P_OPT_PROJECT_NAME                 "[-P project_name]"
#define MSG_GDI_UTEXT_P_OPT_PROJECT_NAME                 _MESSAGE(43224, _("set job's project"))

#define MSG_GDI_USAGE_p_OPT_PRIORITY                     "[-p priority]"
#define MSG_GDI_UTEXT_p_OPT_PRIORITY                     _MESSAGE(43225, _("define job's relative priority"))

#define MSG_GDI_USAGE_pe_OPT_PE_NAME_SLOT_RANGE          "[-pe pe-name slot_range]"
#define MSG_GDI_UTEXT_pe_OPT_PE_NAME_SLOT_RANGE          _MESSAGE(43226, _("request slot range for parallel jobs"))

#define MSG_GDI_USAGE_passwd_OPT                         "[-passwd]"
                       
#define MSG_GDI_USAGE_masterq_OPT_DESTIN_ID_LIST         "[-masterq destin_id_list]"
#define MSG_GDI_UTEXT_masterq_OPT_DESTIN_ID_LIST_BIND    _MESSAGE(43227, _("bind master task to queue(s)"))


#define MSG_GDI_USAGE_q_OPT_DESTIN_ID_LIST               "[-q destin_id_list]"
#define MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_BIND          _MESSAGE(43228, _("bind job to queue(s)"))
#define MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_INFO          _MESSAGE(43229, _("print information on given queue"))

#define MSG_GDI_USAGE_qs_args_OPT_ARGS_QS_END            "[-qs_args [arg [...]] -qs_end]"
#define MSG_GDI_UTEXT_qs_args_OPT_ARGS_QS_END            _MESSAGE(43230, _("deliver args to foreign qs"))

#define MSG_GDI_USAGE_r_OPT_YN                           "[-r y|n]"
#define MSG_GDI_UTEXT_r_OPT_YN                           _MESSAGE(43231, _("define job as (not) restartable" ))

#define MSG_GDI_USAGE_res_OPT                            "[-r]"
#define MSG_GDI_UTEXT_res_OPT                            _MESSAGE(43232, _("show requested resources of job(s)"))

#define MSG_GDI_USAGE_reauthh_OPT_XSECONDS               "[-reautht #seconds]"
            

#define MSG_GDI_USAGE_s_OPT_STATES                       "[-s states]"
                  

#define MSG_GDI_USAGE_s_OPT                              "[-s]"
#define MSG_GDI_UTEXT_s_OPT                              _MESSAGE(43233, _("suspend"))

#define MSG_GDI_USAGE_s_OPT_SIGNAL                       "[-s signal]"

#define MSG_GDI_USAGE_sc_OPT_COMPLEX_LIST                "[-sc complex_list]"
#define MSG_GDI_UTEXT_sc_OPT_COMPLEX_LIST_SHOW           _MESSAGE(43234, _("show given complexes"))
#define MSG_GDI_USAGE_sc_OPT_CONTEXT_LIST                "[-sc context_list]"
#define MSG_GDI_UTEXT_sc_OPT_CONTEXT_LIST_SET            _MESSAGE(43235, _("set job context (replaces old context)"    ))

#define MSG_GDI_USAGE_scal_OPT_CALENDAR_NAME             "[-scal calendar_name]"
#define MSG_GDI_UTEXT_scal_OPT_CALENDAR_NAME             _MESSAGE(43236, _("show given calendar" ))

#define MSG_GDI_USAGE_scall_OPT                          "[-scall]"
#define MSG_GDI_UTEXT_scall_OPT                          _MESSAGE(43237, _("show a list of all calendar names"))

#define MSG_GDI_USAGE_sckpt_OPT_CKPT_NAME                "[-sckpt ckpt_name]"
#define MSG_GDI_UTEXT_sckpt_OPT_CKPT_NAME                _MESSAGE(43238, _("show ckpt interface definition"))

#define MSG_GDI_USAGE_sckptl_OPT                         "[-sckptl]"
#define MSG_GDI_UTEXT_sckptl_OPT                         _MESSAGE(43239, _("show all ckpt interface definitions"))

#define MSG_GDI_USAGE_scl_OPT                            "[-scl]"
#define MSG_GDI_UTEXT_scl_OPT                            _MESSAGE(43240, _("show a list of all complexes"))

#define MSG_GDI_USAGE_sconf_OPT_HOSTLISTORGLOBAL         "[-sconf [host_list|global]]"
#define MSG_GDI_UTEXT_sconf_OPT_HOSTLISTORGLOBAL         _MESSAGE(43241, _("show configurations"))

#define MSG_GDI_USAGE_sconfl_OPT                         "[-sconfl]"
#define MSG_GDI_UTEXT_sconfl_OPT                         _MESSAGE(43242, _("show a list of all local configurations"))

#define MSG_GDI_USAGE_se_OPT_SERVER                      "[-se server]"
#define MSG_GDI_UTEXT_se_OPT_SERVER                      _MESSAGE(43243, _("show given exec server"))

#define MSG_GDI_USAGE_sel_OPT                            "[-sel]"
#define MSG_GDI_UTEXT_sel_OPT                            _MESSAGE(43244, _("show a list of all exec servers"))

#define MSG_GDI_USAGE_sep_OPT                            "[-sep]"
#define MSG_GDI_UTEXT_sep_OPT                            _MESSAGE(43245, _("show a list of all licensed processors"))

#define MSG_GDI_USAGE_sh_OPT                             "[-sh]"
#define MSG_GDI_UTEXT_sh_OPT                             _MESSAGE(43246, _("show a list of all administrative hosts"))

#define MSG_GDI_USAGE_sm_OPT                             "[-sm]"
#define MSG_GDI_UTEXT_sm_OPT                             _MESSAGE(43247, _("show a list of all managers"))

#define MSG_GDI_USAGE_so_OPT                             "[-so]"
#define MSG_GDI_UTEXT_so_OPT                             _MESSAGE(43248, _("show a list of all operators"))

#define MSG_GDI_USAGE_soft_OPT                           "[-soft]"
#define MSG_GDI_UTEXT_soft_OPT                           _MESSAGE(43249, _("consider following requests as soft"))

#define MSG_GDI_USAGE_sp_OPT_PE_NAME                     "[-sp pe-name]"
#define MSG_GDI_UTEXT_sp_OPT_PE_NAME                     _MESSAGE(43250, _("show a parallel environment"))

#define MSG_GDI_USAGE_spl_OPT                            "[-spl]"
#define MSG_GDI_UTEXT_spl_OPT                            _MESSAGE(43251, _("show all parallel environments"))

#define MSG_GDI_USAGE_sq_OPT_DESTIN_ID_LIST              "[-sq [destin_id_list]]"
#define MSG_GDI_UTEXT_sq_OPT_DESTIN_ID_LIST              _MESSAGE(43252, _("show the given queue"  ))

#define MSG_GDI_USAGE_sql_OPT                            "[-sql]"
#define MSG_GDI_UTEXT_sql_OPT                            _MESSAGE(43253, _("show a list of all queues"))

#define MSG_GDI_USAGE_ss_OPT                             "[-ss]"
#define MSG_GDI_UTEXT_ss_OPT                             _MESSAGE(43254, _("show a list of all submit hosts"))

#define MSG_GDI_USAGE_sss_OPT                            "[-sss]"
#define MSG_GDI_UTEXT_sss_OPT                            _MESSAGE(43255, _("show scheduler state"))

#define MSG_GDI_USAGE_ssconf_OPT                         "[-ssconf]"
#define MSG_GDI_UTEXT_ssconf_OPT                         _MESSAGE(43256, _("show scheduler configuration"))

#define MSG_GDI_USAGE_sstnode_OPT_NODE_LIST              "[-sstnode node_list]"
#define MSG_GDI_UTEXT_sstnode_OPT_NODE_LIST              _MESSAGE(43257, _("show sharetree node(s)"))

#define MSG_GDI_USAGE_rsstnode_OPT_NODE_LIST              "[-rsstnode node_list]"
#define MSG_GDI_UTEXT_rsstnode_OPT_NODE_LIST              _MESSAGE(43258, _("show sharetree node(s) and its children"))

#define MSG_GDI_USAGE_sstree_OPT                         "[-sstree]"
#define MSG_GDI_UTEXT_sstree_OPT                         _MESSAGE(43259, _("show the sharetree"))

#define MSG_GDI_USAGE_aumap_OPT                          "[-aumap user]"
#define MSG_GDI_UTEXT_aumap_OPT                          _MESSAGE(43260, _("add new user mapping entry") ) 

#define MSG_GDI_USAGE_Aumap_OPT                          "[-Aumap mapfile]"
#define MSG_GDI_UTEXT_Aumap_OPT                          _MESSAGE(43261, _("add new user mapping entry from file") ) 

#define MSG_GDI_USAGE_dumap_OPT                          "[-dumap user]"
#define MSG_GDI_UTEXT_dumap_OPT                          _MESSAGE(43262, _("delete user mapping entry") ) 

#define MSG_GDI_USAGE_mumap_OPT                          "[-mumap user]"
#define MSG_GDI_UTEXT_mumap_OPT                          _MESSAGE(43263, _("modify user mapping entries") ) 

#define MSG_GDI_USAGE_sumap_OPT                          "[-sumap user]"
#define MSG_GDI_UTEXT_sumap_OPT                          _MESSAGE(43264, _("show user mapping entry") ) 

#define MSG_GDI_USAGE_sumapl_OPT                         "[-sumapl]"
#define MSG_GDI_UTEXT_sumapl_OPT                         _MESSAGE(43265, _("show user mapping entry list") ) 

#define MSG_GDI_USAGE_Mumap_OPT                          "[-Mumap mapfile]"
#define MSG_GDI_UTEXT_Mumap_OPT                          _MESSAGE(43266, _("modify user mapping entry from file"))

#define MSG_GDI_USAGE_shgrp_OPT                          "[-shgrp group]"
#define MSG_GDI_UTEXT_shgrp_OPT                          _MESSAGE(43267, _("show host group") )  

#define MSG_GDI_USAGE_shgrpl_OPT                         "[-shgrpl]"
#define MSG_GDI_UTEXT_shgrpl_OPT                         _MESSAGE(43268, _("show host group list") )  

#define MSG_GDI_USAGE_ahgrp_OPT                          "[-ahgrp group]"
#define MSG_GDI_UTEXT_ahgrp_OPT                          _MESSAGE(43269, _("add new host group entry") ) 

#define MSG_GDI_USAGE_Ahgrp_OPT                          "[-Ahgrp file]"
#define MSG_GDI_UTEXT_Ahgrp_OPT                          _MESSAGE(43270, _("add new host group entry from file") ) 

#define MSG_GDI_USAGE_dhgrp_OPT                          "[-dhgrp group]"
#define MSG_GDI_UTEXT_dhgrp_OPT                          _MESSAGE(43271, _("delete host group entry") ) 

#define MSG_GDI_USAGE_mhgrp_OPT                          "[-mhgrp group]"
#define MSG_GDI_UTEXT_mhgrp_OPT                          _MESSAGE(43272, _("modify host group entry") ) 

#define MSG_GDI_USAGE_Mhgrp_OPT                          "[-Mhgrp file]"
#define MSG_GDI_UTEXT_Mhgrp_OPT                          _MESSAGE(43273, _("modify host group entry from file"))

#define MSG_GDI_USAGE_su_OPT_LISTNAME_LIST               "[-su listname_list]"
#define MSG_GDI_UTEXT_su_OPT_LISTNAME_LIST               _MESSAGE(43274, _("show the given userset list" ))

#define MSG_GDI_USAGE_suser_OPT_USER                     "[-suser user_list]"
#define MSG_GDI_UTEXT_suser_OPT_USER                     _MESSAGE(43275, _("show user(s)"))

#define MSG_GDI_USAGE_sprj_OPT_PROJECT                   "[-sprj project]"
#define MSG_GDI_UTEXT_sprj_OPT_PROJECT                   _MESSAGE(43276, _("show a project"))

#define MSG_GDI_USAGE_sul_OPT                            "[-sul]"
#define MSG_GDI_UTEXT_sul_OPT                            _MESSAGE(43277, _("show a list of all userset lists"))

#define MSG_GDI_USAGE_suserl_OPT                         "[-suserl]"
#define MSG_GDI_UTEXT_suserl_OPT                         _MESSAGE(43278, _("show a list of all users"))

#define MSG_GDI_USAGE_sprjl_OPT                          "[-sprjl]"
#define MSG_GDI_UTEXT_sprjl_OPT                          _MESSAGE(43279, _("show a list of all projects"))

#define MSG_GDI_USAGE_S_OPT_PATH_LIST                    "[-S path_list]"
#define MSG_GDI_UTEXT_S_OPT_PATH_LIST                    _MESSAGE(43280, _("command interpreter to be used"))

#define MSG_GDI_USAGE_t_OPT_TASK_ID_RANGE                "[-t task_id_range]"
#define MSG_GDI_UTEXT_t_OPT_TASK_ID_RANGE                _MESSAGE(43281, _("create a job-array with these tasks"))

#define MSG_GDI_USAGE_tsm_OPT                            "[-tsm]"
#define MSG_GDI_UTEXT_tsm_OPT                            _MESSAGE(43282, _("trigger scheduler monitoring"))

#define MSG_GDI_USAGE_u_OPT_USERLISTORUALL               "[-u user_list|-uall]"
#define MSG_GDI_UTEXT_u_OPT_USERLISTORUALL               _MESSAGE(43283, _("specify a list of users"))

#define MSG_GDI_USAGE_us_OPT                             "[-us]"
#define MSG_GDI_UTEXT_us_OPT                             _MESSAGE(43284, _("unsuspend"))

#define MSG_GDI_USAGE_v_OPT_VARIABLE_LIST                "[-v variable_list]"
#define MSG_GDI_UTEXT_v_OPT_VARIABLE_LIST                _MESSAGE(43285, _("export these environment variables"))

#define MSG_GDI_USAGE_verify_OPT                         "[-verify]"
#define MSG_GDI_UTEXT_verify_OPT                         _MESSAGE(43286, _("do not submit just verify"))

#define MSG_GDI_USAGE_V_OPT                              "[-V]"
#define MSG_GDI_UTEXT_V_OPT                              _MESSAGE(43287, _("export all environment variables"))

#define MSG_GDI_USAGE_w_OPT_EWNV                         "[-w e|w|n|v]"
#define MSG_GDI_UTEXT_w_OPT_EWNV                         _MESSAGE(43288, _("verify mode (error|warning|none|just verify) for jobs"))

#define MSG_GDI_USAGE_AT_OPT_FILE                        "[-@ file]"
#define MSG_GDI_UTEXT_AT_OPT_FILE                        _MESSAGE(43289, _("read commandline input from file"))

#define MSG_GDI_USAGE_nohist_OPT                         "[-nohist]"
#define MSG_GDI_UTEXT_nohist_OPT                         _MESSAGE(43290, _("do not write history" ))

#define MSG_GDI_USAGE_noread_argfile_OPT                 "[-noread-argfile]"
#define MSG_GDI_UTEXT_noread_argfile_OPT                 _MESSAGE(43291, _("do not read \"qmaster_args\" file"))

#define MSG_GDI_USAGE_nowrite_argfile_OPT                "[-nowrite-argfile]"
#define MSG_GDI_UTEXT_nowrite_argfile_OPT                _MESSAGE(43292, _("do not write \"qmaster_args\" file"))

#define MSG_GDI_USAGE_truncate_argfile_OPT               "[-truncate-argfile]"
#define MSG_GDI_UTEXT_truncate_argfile_OPT               _MESSAGE(43293, _("truncate  \"qmaster_args\" file"))

#define MSG_GDI_USAGE_nostart_schedd_OPT                 "[-nostart-schedd]"
#define MSG_GDI_UTEXT_nostart_schedd_OPT                 _MESSAGE(43294, _("do not start schedd"))

#define MSG_GDI_USAGE_nostart_commd_OPT                  "[-nostart-commd]"
#define MSG_GDI_UTEXT_nostart_commd_OPT                  _MESSAGE(43295, _("do not start commd"))

#define MSG_GDI_USAGE_verbose_OPT                        "[-verbose]"
#define MSG_GDI_UTEXT_verbose_OPT                        _MESSAGE(43296, _("verbose information output"))

#define MSG_GDI_USAGE_secl_OPT                           "[-secl]"
#define MSG_GDI_UTEXT_secl_OPT                           _MESSAGE(43297, _("show event client list)"))

#define MSG_GDI_USAGE_kec_OPT                            "[-kec evid_list]"
#define MSG_GDI_UTEXT_kec_OPT                            _MESSAGE(43298, _("kill event client"))

#define MSG_GDI_USAGE_inherit_OPT                        "[-inherit]"
#define MSG_GDI_UTEXT_inherit_OPT                        _MESSAGE(43299, _("inherit existing job environment JOB_ID for rsh"))

#define MSG_GDI_USAGE_nostdin_OPT                        "[-nostdin]"
#define MSG_GDI_UTEXT_nostdin_OPT                        _MESSAGE(43300, _("suppress stdin for rsh"))

#define MSG_GDI_USAGE_noshell_OPT                        "[-noshell]"
#define MSG_GDI_UTEXT_noshell_OPT                        _MESSAGE(43301, _("do start command without wrapping <loginshell> -c"))

#define MSG_GDI_USAGE_mattr_OPT                          "[-mattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_mattr_OPT                          _MESSAGE(43302, _("modify an attribute (or element in a sublist) of an object"))

#define MSG_GDI_USAGE_rattr_OPT                          "[-rattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_rattr_OPT                          _MESSAGE(43303, _("replace a list attribute of an object"))

#define MSG_GDI_USAGE_dattr_OPT                          "[-dattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_dattr_OPT                          _MESSAGE(43304, _("delete from a list attribute of an object"))

#define MSG_GDI_USAGE_aattr_OPT                          "[-aattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_aattr_OPT                          _MESSAGE(43305, _("add to a list attribute of an object") ) 

#define MSG_GDI_USAGE_Mattr_OPT                          "[-Mattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Mattr_OPT                          _MESSAGE(43306, _("modify an attribute (or element in a sublist) of an object"))

#define MSG_GDI_USAGE_Rattr_OPT                          "[-Rattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Rattr_OPT                          _MESSAGE(43307, _("replace a list attribute of an object"))

#define MSG_GDI_USAGE_Dattr_OPT                          "[-Dattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Dattr_OPT                          _MESSAGE(43308, _("delete from a list attribute of an object"))

#define MSG_GDI_USAGE_Aattr_OPT                          "[-Aattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Aattr_OPT                          _MESSAGE(43309, _("add to a list attribute of an object") ) 

#define MSG_GDI_USAGE_show_license_OPT                   "[-show-license]"
#define MSG_GDI_UTEXT_show_license_OPT                   _MESSAGE(43310, _("show license information"))

#define MSG_GDI_USAGE_JQ_DEST_OPR                        "job_queue_list"
#define MSG_GDI_USAGE_SRVR_NM_OPR                        "server_name"
#define MSG_GDI_USAGE_MESSAGE_OPR                        "message"
#define MSG_GDI_USAGE_JOB_ID_OPR                         "job_task_list"
#define MSG_GDI_USAGE_SCRIPT_OPR                         "[{script|-} [script_args]]"
#define MSG_GDI_USAGE_SCRIPT_OPR_ARGS                    "[-- script_args]"
#define MSG_GDI_UTEXT_JOB_ID_OPR                         _MESSAGE(43311, _("jobid's (and taskid's) of jobs to be altered"))
#define MSG_GDI_UTEXT_SCRIPT_OPR_ARGS                    _MESSAGE(43312, _("arguments to be used"))
#define MSG_GDI_UTEXT_ATTACH__u_OPT_USERLISTORUALL       _MESSAGE(43313, _("(not allowed in combination with job_task_list)"))
/* 
** gdilib/utility.c
*/
#define MSG_GDI_STRING_STRINGTOINTFAILED_S               _MESSAGE(43314, _("unable to convert string \"%s\" to integer\n"))
#define MSG_GDI_MEMORY_MALLOCFAILED                      _MESSAGE(43315, _("malloc() failure\n"))
#define MSG_GDI_MEMORY_REALLOCFAILED                     _MESSAGE(43316, _("realloc() failure\n"))
#define MSG_GDI_POINTER_SUFFIXISNULLINSGEUNLINK          _MESSAGE(43317, _("suffix == NULL in sge_unlink()\n"))
#define MSG_GDI_STRING_LENGTHEXCEEDSMAXSTRINGSIZE_SI     _MESSAGE(43318, _("strlen($%s) exceeds MAX_STRING_SIZE %d\n"))


/* 
** gdilib/sge_gdi_request.c
*/
#define MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT   _MESSAGE(43319, _("NULL pointer passed to sge_gdi_multi()"))
#define MSG_GDI_CANTCREATEGDIREQUEST                     _MESSAGE(43320, _("can't create gdi request"))
#define MSG_GDI_NEEDUIDINEVIRONMENT                      _MESSAGE(43321, _("need UID in environment\n"))
#define MSG_GDI_NEEDUSERNAMEINENVIRONMENT                _MESSAGE(43322, _("need USERNAME in environment\n"))
#define MSG_GDI_NEEDGIDINENVIRONMENT                     _MESSAGE(43323, _("need GID in environment\n"))
#define MSG_GDI_NEEDGROUPNAMEINENVIRONMENT               _MESSAGE(43324, _("need GROUPNAME in environment\n"))
#define MSG_GDI_GETPWUIDXFAILEDERRORX_IS                 _MESSAGE(43325, _("failed to getpwuid(%d): %s\n"))
#define MSG_GDI_GETGRGIDXFAILEDERRORX_IS                 _MESSAGE(43326, _("failed to getgrgid(%d): %s\n"))
#define MSG_GDI_SENDINGGDIREQUESTFAILED                  _MESSAGE(43327, _("failed sending gdi request\n"))
#define MSG_GDI_RECEIVEGDIREQUESTFAILED                  _MESSAGE(43328, _("failed receiving gdi request\n"))
#define MSG_GDI_SIGNALED                                 _MESSAGE(43329, _("signaled\n"))
#define MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I     _MESSAGE(43330, _("general error (%d) sending and receiving gdi request\n"))
#define MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST  _MESSAGE(43331, _("NULL list passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST   _MESSAGE(43332, _("NULL rhost passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST  _MESSAGE(43333, _("not enough memory for packing gdi request\n"))
#define MSG_GDI_REQUESTFORMATERROR                          _MESSAGE(43334, _("format error while packing gdi request\n"))
#define MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST       _MESSAGE(43335, _("unexpected error while packing gdi request\n"))
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORUNPACKINGGDIREQUEST _MESSAGE(43336, _("not enough memory for unpacking gdi request\n"))
#define MSG_GDI_REQUESTFORMATERRORWHILEUNPACKING              _MESSAGE(43337, _("format error while unpacking gdi request\n"))
#define MSG_GDI_UNEXPECTEDERRORWHILEUNPACKINGGDIREQUEST _MESSAGE(43338, _("unexpected error while unpacking gdi request\n"))
#define MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D             _MESSAGE(43339, _("invalid value ("U32CFormat") for ar->op\n"))
#define MSG_GDI_CANTUNPACKGDIREQUEST                       _MESSAGE(43340, _("can't unpack gdi request"    ))


/* sge_job_jatask.c */

#define MSG_JOB_REJECTED_NO_TASK_LIST  _MESSAGE(43341, _("job "u32" was rejected because it was not possible to create task list\n"))
#define MSG_JOB_JLPPNULL              _MESSAGE(43342, _("jlpp == NULL in job_add_job()\n"))
#define MSG_JOB_JEPNULL               _MESSAGE(43343, _("jep == NULL in job_add_job()\n"))
#define MSG_JOB_JOBALREADYEXISTS_U    _MESSAGE(43344, _("can't add job \"" U32CFormat "\" - job already exists\n") ) 

#endif /* __MSG_GDILIB_H */

