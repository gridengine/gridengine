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




#define MSG_GDI_USAGE_USAGESTRING                     _("usage:")

#define MSG_GDI_USAGE_a_OPT_DATE_TIME                    "[-a date_time]"
#define MSG_GDI_UTEXT_a_OPT_DATE_TIME                    _("request a job start time")


#define MSG_GDI_USAGE_ac_OPT_COMPLEX_NAME                "[-ac complex_name]"
#define MSG_GDI_UTEXT_ac_OPT_COMPLEX_NAME                _("add a new complex" )

#define MSG_GDI_USAGE_ac_OPT_CONTEXT_LIST                "[-ac context_list]"
#define MSG_GDI_UTEXT_ac_OPT_CONTEXT_LIST                _("add context variable(s)")

#define MSG_GDI_USAGE_Ac_OPT_COMPLEX_NAME_FNAME          "[-Ac complex_name fname]"
#define MSG_GDI_UTEXT_Ac_OPT_COMPLEX_NAME_FNAME          _("add a new complex from file")

#define MSG_GDI_USAGE_acal_OPT_FNAME                     "[-acal calendar_name]"
#define MSG_GDI_UTEXT_acal_OPT_FNAME                     _("add a new calendar")

#define MSG_GDI_USAGE_Acal_OPT_FNAME                     "[-Acal fname]"
#define MSG_GDI_UTEXT_Acal_OPT_FNAME                     _("add a new calendar from file")

#define MSG_GDI_USAGE_ackpt_OPT_CKPT_NAME                "[-ackpt ckpt_name]"
#define MSG_GDI_UTEXT_ackpt_OPT_CKPT_NAME                _("add a ckpt interface definition")

#define MSG_GDI_USAGE_Ackpt_OPT_FNAME                    "[-Ackpt fname]"
#define MSG_GDI_UTEXT_Ackpt_OPT_FNAME                    _("add a ckpt interface definition from file")

#define MSG_GDI_USAGE_aconf_OPT_HOST_LIST                "[-aconf host_list]"
#define MSG_GDI_UTEXT_aconf_OPT_HOST_LIST                _("add configurations")

#define MSG_GDI_USAGE_Aconf_OPT_FILE_LIST                "[-Aconf file_list]"
#define MSG_GDI_UTEXT_Aconf_OPT_FILE_LIST                _("add configurations from file_list")

#define MSG_GDI_USAGE_ae_OPT_EXEC_SERVER_TEMPLATE        "[-ae [exec_server_template]]"
#define MSG_GDI_UTEXT_ae_OPT_EXEC_SERVER_TEMPLATE        _("add an exec host using a template")

#define MSG_GDI_USAGE_Ae_OPT_FNAME                       "[-Ae fname]"
#define MSG_GDI_UTEXT_Ae_OPT_FNAME                       _("add an exec host from file")

#define MSG_GDI_USAGE_ah_OPT_HOSTNAME                    "[-ah hostname]"
#define MSG_GDI_UTEXT_ah_OPT_HOSTNAME                    _("add an administrative host")

#define MSG_GDI_USAGE_am_OPT_USER_LIST                   "[-am user_list]"
#define MSG_GDI_UTEXT_am_OPT_USER_LIST                   _("add user to manager list")

#define MSG_GDI_USAGE_ao_OPT_USER_LIST                   "[-ao user_list]"
#define MSG_GDI_UTEXT_ao_OPT_USER_LIST                   _("add user to operator list")

#define MSG_GDI_USAGE_ap_OPT_PE_NAME                     "[-ap pe-name]"
#define MSG_GDI_UTEXT_ap_OPT_PE_NAME                     _("add a new parallel environment")

#define MSG_GDI_USAGE_Ap_OPT_FNAME                       "[-Ap fname]"
#define MSG_GDI_UTEXT_Ap_OPT_FNAME                       _("add a new parallel environment from file")

#define MSG_GDI_USAGE_aq_OPT_Q_TEMPLATE                  "[-aq [q_template]]"
#define MSG_GDI_UTEXT_aq_OPT_Q_TEMPLATE                  _("add a new queue using the given template")

#define MSG_GDI_USAGE_as_OPT_HOSTNAME                    "[-as hostname]"
#define MSG_GDI_UTEXT_as_OPT_HOSTNAME                    _("add a submit host")

#define MSG_GDI_USAGE_ASTNODE_NODE_SHARES_LIST           "[-astnode node_shares_list]"
#define MSG_GDI_UTEXT_ASTNODE_NODE_SHARES_LIST           _("add sharetree node(s)")

#define MSG_GDI_USAGE_ASTREE                             "[-astree]"
#define MSG_GDI_UTEXT_ASTREE                             _("create/modify the sharetree")

#define MSG_GDI_USAGE_ASTREE_FNAME                       "[-Astree fname]"
#define MSG_GDI_UTEXT_ASTREE_FNAME                       _("create/modify the sharetree from file")

#define MSG_GDI_USAGE_au_OPT_USER_LIST_LISTNAME_LIST     "[-au user_list listname_list]"
#define MSG_GDI_UTEXT_au_OPT_USER_LIST_LISTNAME_LIST     _("add user(s) to userset list(s)")

#define MSG_GDI_USAGE_Au_OPT_LISTNAME_LIST               "[-Au fname]"
#define MSG_GDI_UTEXT_Au_OPT_LISTNAME_LIST               _("add userset from file")

#define MSG_GDI_USAGE_AUSER                              "[-auser]"
#define MSG_GDI_UTEXT_AUSER                              _("add user")

#define MSG_GDI_USAGE_Auser                              "[-Auser fname]"
#define MSG_GDI_UTEXT_Auser                              _("add user from file")

#define MSG_GDI_USAGE_APRJ                               "[-aprj]"
#define MSG_GDI_UTEXT_APRJ                               _("add project")

#define MSG_GDI_USAGE_Aprj                               "[-Aprj fname]"
#define MSG_GDI_UTEXT_Aprj                               _("add project from file")

#define MSG_GDI_USAGE_Mprj_OPT_PROJECT                   "[-Mprj fname]"
#define MSG_GDI_UTEXT_Mprj_OPT_PROJECT                   _("modify project from file")

#define MSG_GDI_USAGE_A_OPT_ACCOUNT_STRING               "[-A account_string]"
#define MSG_GDI_UTEXT_A_OPT_ACCOUNT_STRING               _("use account at host")

#define MSG_GDI_USAGE_Aq_OPT_FNAME                       "[-Aq fname]"
#define MSG_GDI_UTEXT_Aq_OPT_FNAME                       _("add a queue from file")

#define MSG_GDI_USAGE_c_OPT_CKPT_SELECTOR                "[-c ckpt_selector]"
#define MSG_GDI_UTEXT_c_OPT_CKPT_SELECTOR                _("define type of checkpointing for job")

#define MSG_GDI_USAGE_c_OPT                              "[-c]"
#define MSG_GDI_UTEXT_c_OPT                              _("clear queue error state")

#define MSG_GDI_USAGE_ckpt_OPT_CKPT_NAME                 "[-ckpt ckpt-name]"
#define MSG_GDI_UTEXT_ckpt_OPT_CKPT_NAME                 _("request checkpoint method")

#define MSG_GDI_USAGE_clear_OPT                          "[-clear]"
#define MSG_GDI_UTEXT_clear_OPT                          _("skip previous definitions for job")

#define MSG_GDI_USAGE_clearusage_OPT                     "[-clearusage]"
#define MSG_GDI_UTEXT_clearusage_OPT                     _("clear all user/project sharetree usage")

#define MSG_GDI_USAGE_cwd_OPT                            "[-cwd]"
#define MSG_GDI_UTEXT_cwd_OPT                            _("use current working directory")

#define MSG_GDI_USAGE_cq_OPT_DESTIN_ID_LIST              "[-cq destin_id_list]"
#define MSG_GDI_UTEXT_cq_OPT_DESTIN_ID_LIST              _("clean queue")

#define MSG_GDI_USAGE_C_OPT_DIRECTIVE_PREFIX             "[-C directive_prefix]"
#define MSG_GDI_UTEXT_C_OPT_DIRECTIVE_PREFIX             _("define command prefix for job script")

#define MSG_GDI_USAGE_d_OPT                              "[-d]"
#define MSG_GDI_UTEXT_d_OPT                              _("disable")

#define MSG_GDI_USAGE_dc_OPT_COMPLEXNAME                 "[-dc complexname]"
#define MSG_GDI_UTEXT_dc_OPT_COMPLEXNAME                 _("remove a complex")

#define MSG_GDI_USAGE_dc_OPT_SIMPLE_COMPLEX_LIST         "[-dc simple_context_list]"
#define MSG_GDI_UTEXT_dc_OPT_SIMPLE_COMPLEX_LIST         _("remove context variable(s)")

#define MSG_GDI_USAGE_dcal_OPT_CALENDAR_NAME             "[-dcal calendar_name]"
#define MSG_GDI_UTEXT_dcal_OPT_CALENDAR_NAME             _("remove a calendar")

#define MSG_GDI_USAGE_dckpt_OPT_CKPT_NAME                "[-dckpt ckpt_name]"
#define MSG_GDI_UTEXT_dckpt_OPT_CKPT_NAME                _("remove a ckpt interface definition")

#define MSG_GDI_USAGE_dconf_OPT_HOST_LIST                "[-dconf host_list]"
#define MSG_GDI_UTEXT_dconf_OPT_HOST_LIST                _("delete local configurations")

#define MSG_GDI_USAGE_de_OPT_HOST_LIST                   "[-de host_list]"
#define MSG_GDI_UTEXT_de_OPT_HOST_LIST                   _("remove an exec server")

#define MSG_GDI_USAGE_display_OPT_DISPLAY                "[-display display]"
#define MSG_GDI_UTEXT_display_OPT_DISPLAY                _("set display to display interactive job")

#define MSG_GDI_USAGE_dh_OPT_HOST_LIST                   "[-dh host_list]"
#define MSG_GDI_UTEXT_dh_OPT_HOST_LIST                   _("remove an administrative host")

#define MSG_GDI_USAGE_dl_OPT_DATE_TIME                   "[-dl date_time]"
#define MSG_GDI_UTEXT_dl_OPT_DATE_TIME                   _("request a deadline initiation time")

#define MSG_GDI_USAGE_dm_OPT_USER_LIST                   "[-dm user_list]"
#define MSG_GDI_UTEXT_dm_OPT_USER_LIST                   _("remove user from manager list")

#define MSG_GDI_USAGE_do_OPT_USER_LIST                   "[-do user_list]"
#define MSG_GDI_UTEXT_do_OPT_USER_LIST                   _("remove user from operator list")

#define MSG_GDI_USAGE_dp_OPT_PE_NAME                     "[-dp pe-name]"
#define MSG_GDI_UTEXT_dp_OPT_PE_NAME                     _("remove a parallel environment")

#define MSG_GDI_USAGE_dq_OPT_DESTIN_ID_LIST              "[-dq destin_id_list]"
#define MSG_GDI_UTEXT_dq_OPT_DESTIN_ID_LIST              _("remove a queue")

#define MSG_GDI_USAGE_ds_OPT_HOST_LIST                   "[-ds host_list]"
#define MSG_GDI_UTEXT_ds_OPT_HOST_LIST                   _("remove submit host")

#define MSG_GDI_USAGE_DSTNODE_NODELIST                   "[-dstnode node_list]"
#define MSG_GDI_UTEXT_DSTNODE_NODELIST                   _("remove sharetree node(s)")

#define MSG_GDI_USAGE_DSTREE                             "[-dstree]"
#define MSG_GDI_UTEXT_DSTREE                             _("delete the sharetree")

#define MSG_GDI_USAGE_du_OPT_USER_LIST_LISTNAME_LIST     "[-du user_list listname_list]"
#define MSG_GDI_UTEXT_du_OPT_USER_LIST_LISTNAME_LIST     _("remove user(s) from userset list(s)")

#define MSG_GDI_USAGE_dul_OPT_LISTNAME_LIST              "[-dul listname_list]"
#define MSG_GDI_UTEXT_dul_OPT_LISTNAME_LIST              _("remove userset list(s) completely")

#define MSG_GDI_USAGE_DUSER_USER                         "[-duser user_list]"
#define MSG_GDI_UTEXT_DUSER_USER                         _("delete user")

#define MSG_GDI_USAGE_dprj_OPT_PROJECT                   "[-dprj project_list]"
#define MSG_GDI_UTEXT_dprj_OPT_PROJECT                   _("delete project")

#define MSG_GDI_USAGE_e_OPT                              "[-e]"
#define MSG_GDI_UTEXT_e_OPT                              _("enable")

#define MSG_GDI_USAGE_e_OPT_PATH_LIST                    "[-e path_list]"
#define MSG_GDI_UTEXT_e_OPT_PATH_LIST                    _("specify standard error stream path(s)")

#define MSG_GDI_USAGE_ext_OPT                            "[-ext]"
#define MSG_GDI_UTEXT_ext_OPT                            _("view also scheduling attributes")

#define MSG_GDI_USAGE_f_OPT                              "[-f]"
#define MSG_GDI_UTEXT_f_OPT_FULL_OUTPUT                  _("full output")
#define MSG_GDI_UTEXT_f_OPT_FORCE_ACTION                 _("force action"                              )

#define MSG_GDI_USAGE_h_OPT_HOLD_LIST                    "[-h hold_list]"
#define MSG_GDI_UTEXT_h_OPT_HOLD_LIST                    _("assign holds for jobs or tasks")

#define MSG_GDI_USAGE_h_OPT                              "[-h]"
#define MSG_GDI_UTEXT_h_OPT                              _("place user hold on job")

#define MSG_GDI_USAGE_hard_OPT                           "[-hard]"
#define MSG_GDI_UTEXT_hard_OPT                           _("consider following requests \"hard\"")

#define MSG_GDI_USAGE_help_OPT                           "[-help]"
#define MSG_GDI_UTEXT_help_OPT                           _("print this help")

#define MSG_GDI_USAGE_hold_jid_OPT                       "[-hold_jid job_identifier_list]"
#define MSG_GDI_UTEXT_hold_jid_OPT                       _("define jobnet interdependencies")

#define MSG_GDI_USAGE_j_OPT_YN                           "[-j y|n]"
#define MSG_GDI_UTEXT_j_OPT_YN                           _("merge stdout and stderr stream of job")

#define MSG_GDI_USAGE_jid_OPT_JOB_ID_LIST                "[-jid job_id_list]"
#define MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_PRINTED        _("jobs to be printed")
#define MSG_GDI_UTEXT_jid_OPT_JOB_ID_LIST_ALTERED        _("jobs to be altered")

#define MSG_GDI_USAGE_jid_OPT_JID                        "[-jid jid]"


#define MSG_GDI_USAGE_ke_OPT_HOSTS                       "[-ke[j] host_list"
#define MSG_GDI_UTEXT_ke_OPT_HOSTS                       _("shutdown execution daemon(s)")

#define MSG_GDI_USAGE_k_OPT_MASTERORSCHEDULINGDAEMON     "[-k{m|s}]"
#define MSG_GDI_UTEXT_k_OPT_MASTERORSCHEDULINGDAEMON     _("shutdown master|scheduling daemon")

#define MSG_GDI_USAGE_kqs_OPT                            "[-kqs]"
                         

#define MSG_GDI_USAGE_l_OPT_RESOURCE_LIST                "[-l resource_list]"
#define MSG_GDI_UTEXT_l_OPT_RESOURCE_LIST                _("request the given resources")

#define MSG_GDI_USAGE_lj_OPT_LOG_FILE                    "[-lj log_file]"
#define MSG_GDI_UTEXT_lj_OPT_LOG_FILE                    _("write job logging to log file")

#define MSG_GDI_USAGE_m_OPT_MAIL_OPTIONS                 "[-m mail_options]"
#define MSG_GDI_UTEXT_m_OPT_MAIL_OPTIONS                 _("define mail notification events")

#define MSG_GDI_USAGE_mc_OPT_COMPLEX                     "[-mc [complex]]"
#define MSG_GDI_UTEXT_mc_OPT_COMPLEX                     _("modify a complex")

#define MSG_GDI_USAGE_mckpt_OPT_CKPT_NAME                "[-mckpt ckpt_name]"
#define MSG_GDI_UTEXT_mckpt_OPT_CKPT_NAME                _("modify a ckpt interface definition")

#define MSG_GDI_USAGE_Mc_OPT_COMPLEX_NAME_FNAME          "[-Mc complex_name fname]"
#define MSG_GDI_UTEXT_Mc_OPT_COMPLEX_NAME_FNAME          _("modify a complex from file")

#define MSG_GDI_USAGE_mcal_OPT_CALENDAR_NAME             "[-mcal calendar_name]"
#define MSG_GDI_UTEXT_mcal_OPT_CALENDAR_NAME             _("modify calendar")

#define MSG_GDI_USAGE_Mcal_OPT_FNAME                     "[-Mcal fname]"
#define MSG_GDI_UTEXT_Mcal_OPT_FNAME                     _("modify calendar from file")

#define MSG_GDI_USAGE_Mckpt_OPT_FNAME                    "[-Mckpt fname]"
#define MSG_GDI_UTEXT_Mckpt_OPT_FNAME                    _("modify a ckpt interface definition from file")

#define MSG_GDI_USAGE_mconf_OPT_HOSTLISTORGLOBAL         "[-mconf [host_list|global]]"
#define MSG_GDI_UTEXT_mconf_OPT_HOSTLISTORGLOBAL         _("modify configurations")

#define MSG_GDI_USAGE_msconf_OPT                         "[-msconf]"
#define MSG_GDI_UTEXT_msconf_OPT                         _("modify scheduler configuration")

#define MSG_GDI_USAGE_me_OPT_SERVER                      "[-me server]"
#define MSG_GDI_UTEXT_me_OPT_SERVER                      _("modify exec server")

#define MSG_GDI_USAGE_Me_OPT_FNAME                       "[-Me fname]"
#define MSG_GDI_UTEXT_Me_OPT_FNAME                       _("modify exec server from file")

#define MSG_GDI_USAGE_mp_OPT_PE_NAME                     "[-mp pe-name]"  
#define MSG_GDI_UTEXT_mp_OPT_PE_NAME                     _("modify a parallel environment")

#define MSG_GDI_USAGE_Mp_OPT_FNAME                       "[-Mp fname]"
#define MSG_GDI_UTEXT_Mp_OPT_FNAME                       _("modify a parallel environment from file")

#define MSG_GDI_USAGE_mq_OPT_QUEUE                       "[-mq [queue]]"
#define MSG_GDI_UTEXT_mq_OPT_QUEUE                       _("modify a queue")

#define MSG_GDI_USAGE_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST   "[-mqattr attr_name value destin_id_list]"
#define MSG_GDI_UTEXT_mqattr_OPT_ATTR_NAME_VALUE_DESTIN_ID_LIST   _("modify particular queue attributes")

#define MSG_GDI_USAGE_Mq_OPT_FNAME                       "[-Mq fname]"
#define MSG_GDI_UTEXT_Mq_OPT_FNAME                       _("modify a queue from file")

#define MSG_GDI_USAGE_Mqattr_OPT_FNAME_DESTIN_ID_LIST    "[-Mqattr fname destin_id_list]"
#define MSG_GDI_UTEXT_Mqattr_OPT_FNAME_DESTIN_ID_LIST    _("modify particular queue attributes from file")

#define MSG_GDI_USAGE_mu_OPT_LISTNAME_LIST               "[-mu listname_list]"
#define MSG_GDI_UTEXT_mu_OPT_LISTNAME_LIST               _("modify the given userset list")

#define MSG_GDI_USAGE_Mu_OPT_LISTNAME_LIST               "[-Mu fname]"
#define MSG_GDI_UTEXT_Mu_OPT_LISTNAME_LIST               _("modify userset from file")

#define MSG_GDI_USAGE_muser_OPT_USER                     "[-muser user]"
#define MSG_GDI_UTEXT_muser_OPT_USER                     _("modify a user")

#define MSG_GDI_USAGE_Muser_OPT_USER                     "[-Muser fname]"
#define MSG_GDI_UTEXT_Muser_OPT_USER                     _("modify a user from file")

#define MSG_GDI_USAGE_mprj_OPT_PROJECT                   "[-mprj project]"
#define MSG_GDI_UTEXT_mprj_OPT_PROJECT                   _("modify a project")

#define MSG_GDI_USAGE_MSTNODE_NODE_SHARES_LIST           "[-mstnode node_shares_list]"
#define MSG_GDI_UTEXT_MSTNODE_NODE_SHARES_LIST           _("modify sharetree node(s)")

#define MSG_GDI_USAGE_MSTREE                             "[-mstree]"
#define MSG_GDI_UTEXT_MSTREE                             _("modify/create the sharetree")

#define MSG_GDI_USAGE_MSTREE_FNAME                       "[-Mstree fname]"
#define MSG_GDI_UTEXT_MSTREE_FNAME                       _("modify/create the sharetree from file")

#define MSG_GDI_USAGE_notify_OPT                         "[-notify]"
#define MSG_GDI_UTEXT_notify_OPT                         _("notify job before killing/suspending it")

#define MSG_GDI_USAGE_now_OPT_YN                         "[-now y[es]|n[o]]"
#define MSG_GDI_UTEXT_now_OPT_YN                         _("start job immediately or not at all")

#define MSG_GDI_USAGE_M_OPT_MAIL_LIST                    "[-M mail_list]"
#define MSG_GDI_UTEXT_M_OPT_MAIL_LIST                    _("notify these e-mail addresses")

#define MSG_GDI_USAGE_N_OPT_NAME                         "[-N name]"
#define MSG_GDI_UTEXT_N_OPT_NAME                         _("specify job name")

#define MSG_GDI_USAGE_o_OPT_PATH_LIST                    "[-o path_list]"
#define MSG_GDI_UTEXT_o_OPT_PATH_LIST                    _("specify standard output stream path(s)")

#define MSG_GDI_USAGE_ot_OPT_TICKETS                     "[-ot tickets]"
#define MSG_GDI_UTEXT_ot_OPT_TICKETS                     _("set job's override tickets")

#define MSG_GDI_USAGE_P_OPT_PROJECT_NAME                 "[-P project_name]"
#define MSG_GDI_UTEXT_P_OPT_PROJECT_NAME                 _("set job's project")

#define MSG_GDI_USAGE_p_OPT_PRIORITY                     "[-p priority]"
#define MSG_GDI_UTEXT_p_OPT_PRIORITY                     _("define job's relative priority")

#define MSG_GDI_USAGE_pe_OPT_PE_NAME_SLOT_RANGE          "[-pe pe-name slot_range]"
#define MSG_GDI_UTEXT_pe_OPT_PE_NAME_SLOT_RANGE          _("request slot range for parallel jobs")

#define MSG_GDI_USAGE_passwd_OPT                         "[-passwd]"
                       
#define MSG_GDI_USAGE_masterq_OPT_DESTIN_ID_LIST         "[-masterq destin_id_list]"
#define MSG_GDI_UTEXT_masterq_OPT_DESTIN_ID_LIST_BIND    _("bind master task to queue(s)")


#define MSG_GDI_USAGE_q_OPT_DESTIN_ID_LIST               "[-q destin_id_list]"
#define MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_BIND          _("bind job to queue(s)")
#define MSG_GDI_UTEXT_q_OPT_DESTIN_ID_LIST_INFO          _("print information on given queue")


#define MSG_GDI_USAGE_qmon_OPT                           "[-qmon]"
                        

#define MSG_GDI_USAGE_qs_args_OPT_ARGS_QS_END            "[-qs_args [arg [...]] -qs_end]"
#define MSG_GDI_UTEXT_qs_args_OPT_ARGS_QS_END            _("deliver args to foreign qs")

#define MSG_GDI_USAGE_r_OPT_YN                           "[-r y|n]"
#define MSG_GDI_UTEXT_r_OPT_YN                           _("define job as (not) restartable" )

#define MSG_GDI_USAGE_res_OPT                            "[-r]"
#define MSG_GDI_UTEXT_res_OPT                            _("show requested resources of job(s)")

#define MSG_GDI_USAGE_reauthh_OPT_XSECONDS               "[-reautht #seconds]"
            

#define MSG_GDI_USAGE_s_OPT_STATES                       "[-s states]"
                  

#define MSG_GDI_USAGE_s_OPT                              "[-s]"
#define MSG_GDI_UTEXT_s_OPT                              _("suspend")

#define MSG_GDI_USAGE_s_OPT_SIGNAL                       "[-s signal]"

#define MSG_GDI_USAGE_sc_OPT_COMPLEX_LIST                "[-sc complex_list]"
#define MSG_GDI_UTEXT_sc_OPT_COMPLEX_LIST_SHOW           _("show given complexes")
#define MSG_GDI_USAGE_sc_OPT_CONTEXT_LIST                "[-sc context_list]"
#define MSG_GDI_UTEXT_sc_OPT_CONTEXT_LIST_SET            _("set job context (replaces old context)"    )

#define MSG_GDI_USAGE_scal_OPT_CALENDAR_NAME             "[-scal calendar_name]"
#define MSG_GDI_UTEXT_scal_OPT_CALENDAR_NAME             _("show given calendar" )

#define MSG_GDI_USAGE_scall_OPT                          "[-scall]"
#define MSG_GDI_UTEXT_scall_OPT                          _("show a list of all calendar names")

#define MSG_GDI_USAGE_sckpt_OPT_CKPT_NAME                "[-sckpt ckpt_name]"
#define MSG_GDI_UTEXT_sckpt_OPT_CKPT_NAME                _("show ckpt interface definition")

#define MSG_GDI_USAGE_sckptl_OPT                         "[-sckptl]"
#define MSG_GDI_UTEXT_sckptl_OPT                         _("show all ckpt interface definitions")

#define MSG_GDI_USAGE_scl_OPT                            "[-scl]"
#define MSG_GDI_UTEXT_scl_OPT                            _("show a list of all complexes")

#define MSG_GDI_USAGE_sconf_OPT_HOSTLISTORGLOBAL         "[-sconf [host_list|global]]"
#define MSG_GDI_UTEXT_sconf_OPT_HOSTLISTORGLOBAL         _("show configurations")

#define MSG_GDI_USAGE_sconfl_OPT                         "[-sconfl]"
#define MSG_GDI_UTEXT_sconfl_OPT                         _("show a list of all local configurations")

#define MSG_GDI_USAGE_se_OPT_SERVER                      "[-se server]"
#define MSG_GDI_UTEXT_se_OPT_SERVER                      _("show given exec server")

#define MSG_GDI_USAGE_sel_OPT                            "[-sel]"
#define MSG_GDI_UTEXT_sel_OPT                            _("show a list of all exec servers")

#define MSG_GDI_USAGE_sep_OPT                            "[-sep]"
#define MSG_GDI_UTEXT_sep_OPT                            _("show a list of all licensed processors")

#define MSG_GDI_USAGE_sh_OPT                             "[-sh]"
#define MSG_GDI_UTEXT_sh_OPT                             _("show a list of all administrative hosts")

#define MSG_GDI_USAGE_sm_OPT                             "[-sm]"
#define MSG_GDI_UTEXT_sm_OPT                             _("show a list of all managers")

#define MSG_GDI_USAGE_so_OPT                             "[-so]"
#define MSG_GDI_UTEXT_so_OPT                             _("show a list of all operators")

#define MSG_GDI_USAGE_soft_OPT                           "[-soft]"
#define MSG_GDI_UTEXT_soft_OPT                           _("consider following requests as soft")

#define MSG_GDI_USAGE_sp_OPT_PE_NAME                     "[-sp pe-name]"
#define MSG_GDI_UTEXT_sp_OPT_PE_NAME                     _("show a parallel environment")

#define MSG_GDI_USAGE_spl_OPT                            "[-spl]"
#define MSG_GDI_UTEXT_spl_OPT                            _("show all parallel environments")

#define MSG_GDI_USAGE_sq_OPT_DESTIN_ID_LIST              "[-sq [destin_id_list]]"
#define MSG_GDI_UTEXT_sq_OPT_DESTIN_ID_LIST              _("show the given queue"  )

#define MSG_GDI_USAGE_sql_OPT                            "[-sql]"
#define MSG_GDI_UTEXT_sql_OPT                            _("show a list of all queues")

#define MSG_GDI_USAGE_ss_OPT                             "[-ss]"
#define MSG_GDI_UTEXT_ss_OPT                             _("show a list of all submit hosts")

#define MSG_GDI_USAGE_sss_OPT                            "[-sss]"
#define MSG_GDI_UTEXT_sss_OPT                            _("show scheduler state")

#define MSG_GDI_USAGE_ssconf_OPT                         "[-ssconf]"
#define MSG_GDI_UTEXT_ssconf_OPT                         _("show scheduler configuration")

#define MSG_GDI_USAGE_sstnode_OPT_NODE_LIST              "[-sstnode node_list]"
#define MSG_GDI_UTEXT_sstnode_OPT_NODE_LIST              _("show sharetree node(s)")

#define MSG_GDI_USAGE_rsstnode_OPT_NODE_LIST              "[-rsstnode node_list]"
#define MSG_GDI_UTEXT_rsstnode_OPT_NODE_LIST              _("show sharetree node(s) and its children")

#define MSG_GDI_USAGE_sstree_OPT                         "[-sstree]"
#define MSG_GDI_UTEXT_sstree_OPT                         _("show the sharetree")

#define MSG_GDI_USAGE_aumap_OPT                          "[-aumap user]"
#define MSG_GDI_UTEXT_aumap_OPT                          _("add new user mapping entry") 

#define MSG_GDI_USAGE_Aumap_OPT                          "[-Aumap mapfile]"
#define MSG_GDI_UTEXT_Aumap_OPT                          _("add new user mapping entry from file") 

#define MSG_GDI_USAGE_dumap_OPT                          "[-dumap user]"
#define MSG_GDI_UTEXT_dumap_OPT                          _("delete user mapping entry") 

#define MSG_GDI_USAGE_mumap_OPT                          "[-mumap user]"
#define MSG_GDI_UTEXT_mumap_OPT                          _("modify user mapping entries") 

#define MSG_GDI_USAGE_sumap_OPT                          "[-sumap user]"
#define MSG_GDI_UTEXT_sumap_OPT                          _("show user mapping entry") 

#define MSG_GDI_USAGE_sumapl_OPT                         "[-sumapl]"
#define MSG_GDI_UTEXT_sumapl_OPT                         _("show user mapping entry list") 

#define MSG_GDI_USAGE_Mumap_OPT                          "[-Mumap mapfile]"
#define MSG_GDI_UTEXT_Mumap_OPT                          _("modify user mapping entry from file")

#define MSG_GDI_USAGE_shgrp_OPT                          "[-shgrp group]"
#define MSG_GDI_UTEXT_shgrp_OPT                          _("show host group")  

#define MSG_GDI_USAGE_shgrpl_OPT                         "[-shgrpl]"
#define MSG_GDI_UTEXT_shgrpl_OPT                         _("show host group list")  

#define MSG_GDI_USAGE_ahgrp_OPT                          "[-ahgrp group]"
#define MSG_GDI_UTEXT_ahgrp_OPT                          _("add new host group entry") 

#define MSG_GDI_USAGE_Ahgrp_OPT                          "[-Ahgrp file]"
#define MSG_GDI_UTEXT_Ahgrp_OPT                          _("add new host group entry from file") 

#define MSG_GDI_USAGE_dhgrp_OPT                          "[-dhgrp group]"
#define MSG_GDI_UTEXT_dhgrp_OPT                          _("delete host group entry") 

#define MSG_GDI_USAGE_mhgrp_OPT                          "[-mhgrp group]"
#define MSG_GDI_UTEXT_mhgrp_OPT                          _("modify host group entry") 

#define MSG_GDI_USAGE_Mhgrp_OPT                          "[-Mhgrp file]"
#define MSG_GDI_UTEXT_Mhgrp_OPT                          _("modify host group entry from file")

#define MSG_GDI_USAGE_su_OPT_LISTNAME_LIST               "[-su listname_list]"
#define MSG_GDI_UTEXT_su_OPT_LISTNAME_LIST               _("show the given userset list" )

#define MSG_GDI_USAGE_suser_OPT_USER                     "[-suser user_list]"
#define MSG_GDI_UTEXT_suser_OPT_USER                     _("show user(s)")

#define MSG_GDI_USAGE_sprj_OPT_PROJECT                   "[-sprj project]"
#define MSG_GDI_UTEXT_sprj_OPT_PROJECT                   _("show a project")

#define MSG_GDI_USAGE_sul_OPT                            "[-sul]"
#define MSG_GDI_UTEXT_sul_OPT                            _("show a list of all userset lists")

#define MSG_GDI_USAGE_suserl_OPT                         "[-suserl]"
#define MSG_GDI_UTEXT_suserl_OPT                         _("show a list of all users")

#define MSG_GDI_USAGE_sprjl_OPT                          "[-sprjl]"
#define MSG_GDI_UTEXT_sprjl_OPT                          _("show a list of all projects")

#define MSG_GDI_USAGE_S_OPT_PATH_LIST                    "[-S path_list]"
#define MSG_GDI_UTEXT_S_OPT_PATH_LIST                    _("command interpreter to be used")

#define MSG_GDI_USAGE_t_OPT_TASK_ID_RANGE                "[-t task_id_range]"
#define MSG_GDI_UTEXT_t_OPT_TASK_ID_RANGE                _("create a job-array with these tasks")

#define MSG_GDI_USAGE_tsm_OPT                            "[-tsm]"
#define MSG_GDI_UTEXT_tsm_OPT                            _("trigger scheduler monitoring")

#define MSG_GDI_USAGE_u_OPT_USERLISTORUALL               "[-u user_list|-uall]"
#define MSG_GDI_UTEXT_u_OPT_USERLISTORUALL               _("specify a list of users")

#define MSG_GDI_USAGE_us_OPT                             "[-us]"
#define MSG_GDI_UTEXT_us_OPT                             _("unsuspend")

#define MSG_GDI_USAGE_v_OPT_VARIABLE_LIST                "[-v variable_list]"
#define MSG_GDI_UTEXT_v_OPT_VARIABLE_LIST                _("export these environment variables")

#define MSG_GDI_USAGE_verify_OPT                         "[-verify]"
#define MSG_GDI_UTEXT_verify_OPT                         _("do not submit just verify")

#define MSG_GDI_USAGE_V_OPT                              "[-V]"
#define MSG_GDI_UTEXT_V_OPT                              _("export all environment variables")

#define MSG_GDI_USAGE_w_OPT_EWNV                         "[-w e|w|n|v]"
#define MSG_GDI_UTEXT_w_OPT_EWNV                         _("verify mode (error|warning|none|just verify) for jobs")

#define MSG_GDI_USAGE_AT_OPT_FILE                        "[-@ file]"
#define MSG_GDI_UTEXT_AT_OPT_FILE                        _("read commandline input from file")

#define MSG_GDI_USAGE_nohist_OPT                         "[-nohist]"
#define MSG_GDI_UTEXT_nohist_OPT                         _("do not write history" )

#define MSG_GDI_USAGE_noread_argfile_OPT                 "[-noread-argfile]"
#define MSG_GDI_UTEXT_noread_argfile_OPT                 _("do not read \"qmaster_args\" file")

#define MSG_GDI_USAGE_nowrite_argfile_OPT                "[-nowrite-argfile]"
#define MSG_GDI_UTEXT_nowrite_argfile_OPT                _("do not write \"qmaster_args\" file")

#define MSG_GDI_USAGE_truncate_argfile_OPT               "[-truncate-argfile]"
#define MSG_GDI_UTEXT_truncate_argfile_OPT               _("truncate  \"qmaster_args\" file")

#define MSG_GDI_USAGE_nostart_schedd_OPT                 "[-nostart-schedd]"
#define MSG_GDI_UTEXT_nostart_schedd_OPT                 _("do not start schedd")

#define MSG_GDI_USAGE_nostart_commd_OPT                  "[-nostart-commd]"
#define MSG_GDI_UTEXT_nostart_commd_OPT                  _("do not start commd")

#define MSG_GDI_USAGE_verbose_OPT                        "[-verbose]"
#define MSG_GDI_UTEXT_verbose_OPT                        _("verbose information output")

#define MSG_GDI_USAGE_secl_OPT                           "[-secl]"
#define MSG_GDI_UTEXT_secl_OPT                           _("show event client list")

#define MSG_GDI_USAGE_kec_OPT                            "[-kec evid_list]"
#define MSG_GDI_UTEXT_kec_OPT                            _("kill event client")

#define MSG_GDI_USAGE_inherit_OPT                        "[-inherit]"
#define MSG_GDI_UTEXT_inherit_OPT                        _("inherit existing job environment JOB_ID for rsh")

#define MSG_GDI_USAGE_nostdin_OPT                        "[-nostdin]"
#define MSG_GDI_UTEXT_nostdin_OPT                        _("suppress stdin for rsh")

#define MSG_GDI_USAGE_noshell_OPT                        "[-noshell]"
#define MSG_GDI_UTEXT_noshell_OPT                        _("do start command without wrapping <loginshell> -c")

#define MSG_GDI_USAGE_mattr_OPT                          "[-mattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_mattr_OPT                          _("modify an attribute (or element in a sublist) of an object")

#define MSG_GDI_USAGE_rattr_OPT                          "[-rattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_rattr_OPT                          _("replace a list attribute of an object")

#define MSG_GDI_USAGE_dattr_OPT                          "[-dattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_dattr_OPT                          _("delete from a list attribute of an object")

#define MSG_GDI_USAGE_aattr_OPT                          "[-aattr obj_nm attr_nm val obj_id_lst]"
#define MSG_GDI_UTEXT_aattr_OPT                          _("add to a list attribute of an object") 

#define MSG_GDI_USAGE_Mattr_OPT                          "[-Mattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Mattr_OPT                          _("modify an attribute (or element in a sublist) of an object")

#define MSG_GDI_USAGE_Rattr_OPT                          "[-Rattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Rattr_OPT                          _("replace a list attribute of an object")

#define MSG_GDI_USAGE_Dattr_OPT                          "[-Dattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Dattr_OPT                          _("delete from a list attribute of an object")

#define MSG_GDI_USAGE_Aattr_OPT                          "[-Aattr obj_nm fname obj_id_lst]"
#define MSG_GDI_UTEXT_Aattr_OPT                          _("add to a list attribute of an object") 

#define MSG_GDI_USAGE_show_license_OPT                   "[-show-license]"
#define MSG_GDI_UTEXT_show_license_OPT                   _("show license information")

#define MSG_GDI_USAGE_JQ_DEST_OPR                        "job_queue_list"
#define MSG_GDI_USAGE_SRVR_NM_OPR                        "server_name"
#define MSG_GDI_USAGE_MESSAGE_OPR                        "message"
#define MSG_GDI_USAGE_JOB_ID_OPR                         "job_task_list"
#define MSG_GDI_USAGE_SCRIPT_OPR                         "[{script|-} [script_args]]"
#define MSG_GDI_USAGE_SCRIPT_OPR_ARGS                    "[-- script_args]"
#define MSG_GDI_UTEXT_JOB_ID_OPR                         _("jobid's (and taskid's) of jobs to be altered")
#define MSG_GDI_UTEXT_SCRIPT_OPR_ARGS                    _("arguments to be used")
#define MSG_GDI_UTEXT_ATTACH__u_OPT_USERLISTORUALL       _("(not allowed in combination with job_task_list)")
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


#endif /* __MSG_GDILIB_H */

