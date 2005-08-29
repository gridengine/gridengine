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
#define MSG_QSH_GET_CREDSTDERR_S      _MESSAGE(43000, _("get_cred stderr: "SFN))
#define MSG_QSH_QSUBFAILED            _MESSAGE(43001, _("qsub failed"))
#define MSG_QSUB_CANTSTARTCOMMANDXTOGETTOKENQSUBFAILED_S    _MESSAGE(43002, _("can't start command "SFQ" to get token - qsub failed"))
#define MSG_QSH_CANTSTARTCOMMANDXTOGETCREDENTIALSQSUBFAILED_S    _MESSAGE(43003, _("can't start command "SFQ" to get credentials - qsub failed"))
#define MSG_QSH_CANTGETCREDENTIALS    _MESSAGE(43004, _("warning: could not get credentials"))
#define MSG_SEC_KRBAUTHFAILURE        _MESSAGE(43013, _("job "sge_U32CFormat" rejected because authentication failed"))
#define MSG_SEC_KRBAUTHFAILUREONHOST  _MESSAGE(43014, _("job "sge_U32CFormat" rejected because authentication failed on host "SFN))
#define MSG_SEC_NOCREDNOBIN_US        _MESSAGE(43017, _("could not get client credentials for job " sge_U32CFormat" - "SFN" binary does not exist"))
#define MSG_SEC_KRB_CRED_SSSI         _MESSAGE(43018, _("denied: request for user "SFQ" does not match Kerberos credentials for connection <"SFN","SFN",%d>"))         
#define MSG_SEC_KRBDECRYPTTGT_US      _MESSAGE(43019, _("could not decrypt TGT for job " sge_U32CFormat "- "SFN))
#define MSG_SEC_KRBENCRYPTTGT_SSIS    _MESSAGE(43020, _("could not encrypt TGT for client <"SFN","SFN",%d> - "SFN))
#define MSG_SEC_KRBENCRYPTTGTUSER_SUS _MESSAGE(43021, _("could not encrypt TGT for user "SFN", job "sge_U32CFormat" - "SFN))
#define MSG_SEC_NOUID_SU              _MESSAGE(43022, _("could not get user ID for "SFN", job "sge_U32CFormat))

/* 
** gdilib/sge_any_request.c
*/ 
#define MSG_GDI_INITSECURITYDATAFAILED                _MESSAGE(43045, _("failed initialize security data"))
#define MSG_GDI_INITCOMMLIBFAILED                     _MESSAGE(43046, _("failed initialize communication library"))
#define MSG_GDI_NO_VALID_PROGRAMM_NAME                _MESSAGE(43047, _("got no valid programname"))
#define MSG_GDI_RHOSTISNULLFORSENDREQUEST             _MESSAGE(43050, _("parameter rhost = NULL for sge_send_any_request()"))
#define MSG_GDI_SENDMESSAGETOCOMMPROCFAILED_SSISS     _MESSAGE(43051, _("can't send "SFN"synchronous message to commproc ("SFN":%d) on host "SFQ": "SFN))
#define MSG_GDI_RHOSTISNULLFORGETANYREQUEST           _MESSAGE(43052, _("parameter rhost = NULL for sge_get_any_request()"))
#define MSG_GDI_RECEIVEMESSAGEFROMCOMMPROCFAILED_SISS _MESSAGE(43053, _("can't receive message from commproc ("SFN":%d) on host "SFQ": "SFN))
#define MSG_GDI_NOCOMMHANDLE                          _MESSAGE(43054, _("can't get communication handle"))
#define MSG_GDI_SENDINGMESSAGE_SIU                    _MESSAGE(43055, _("sending to id: "SFN",%d, size of message: " sge_U32CFormat))

/* 
** gdilib/sge_qexec.c
*/
#define MSG_GDI_INVALIDPARAMETER_SS                      _MESSAGE(43066, _("invalid paramter to "SFQ": "SFQ))
#define MSG_GDI_RESOLVINGUIDTOUSERNAMEFAILED_IS          _MESSAGE(43067, _("failed resolving uid %d to username: "SFN))
#define MSG_GDI_MISSINGINENVIRONMENT_S                   _MESSAGE(43068, _("missing "SFQ" in environment"))
#define MSG_GDI_STRINGISINVALID_SS                       _MESSAGE(43069, _("string "SFQ" in env var "SFQ" is not a valid job/taskid"))
#define MSG_GDI_SENDTASKTOEXECDFAILED_SS                 _MESSAGE(43073, _("failed sending task to execd@"SFN": "SFN))
#define MSG_GDI_TASKNOTEXIST_S                           _MESSAGE(43074, _("task "SFQ" does not exist"))
#define MSG_GDI_MESSAGERECEIVEFAILED_SI                  _MESSAGE(43076, _("failed receiving message from execd: "SFN" %d"))
#define MSG_GDI_TASKNOTFOUND_S                           _MESSAGE(43077, _("cannot find task with taskid "SFQ))
#define MSG_GDI_TASKNOTFOUNDNOIDGIVEN_S                  _MESSAGE(43078, _("cannot find task without taskid - should become task "SFQ))



/* 
** gdilib/sge_report.c
*/
#define MSG_GDI_REPORTNOMEMORY_I                         _MESSAGE(43079, _("not enough memory for packing report: %d bytes"))
#define MSG_GDI_REPORTFORMATERROR                        _MESSAGE(43080, _("format error while packing report"))
#define MSG_GDI_REPORTUNKNOWERROR                        _MESSAGE(43081, _("unexpected error while packing report"))

/* 
** gdilib/qm_name.c
*/
#define MSG_GDI_NULLPOINTERPASSED                        _MESSAGE(43099, _("NULL pointer passed to \"master_host\" or \"master_file\""))
#define MSG_GDI_OPENMASTERFILEFAILED_S                   _MESSAGE(43100, _("can't open "SFQ" for reading qmaster hostname"))
#define MSG_GDI_READMASTERHOSTNAMEFAILED_S               _MESSAGE(43101, _("can't read qmaster hostname in "SFQ))
#define MSG_GDI_MASTERHOSTNAMEHASZEROLENGTH_S            _MESSAGE(43102, _("qmaster hostname in "SFQ" has zero length"))
#define MSG_GDI_MASTERHOSTNAMEEXCEEDSCHARS_SI            _MESSAGE(43103, _("qmaster hostname in "SFQ" exceeds %d characters"))
#define MSG_GDI_OPENWRITEMASTERHOSTNAMEFAILED_SS         _MESSAGE(43104, _("can't open "SFQ" for writing qmaster hostname: "SFN))
#define MSG_GDI_WRITEMASTERHOSTNAMEFAILED_S              _MESSAGE(43105, _("can't write qmaster hostname into "SFQ))
#define MSG_GDI_FOPEN_FAILED                             _MESSAGE(43106, _("fopen("SFQ") failed: "SFN))


/* 
** gdilib/resolve.c
*/
#define MSG_GDI_READMASTERNAMEFAILED_S                   _MESSAGE(43107, _("unable to read qmaster name: "SFN))



/* 
** gdilib/sge_gdi_request.c
*/
#define MSG_GDI_POINTER_NULLPOINTERPASSEDTOSGEGDIMULIT   _MESSAGE(43117, _("NULL pointer passed to sge_gdi_multi()"))
#define MSG_GDI_CANTCREATEGDIREQUEST                     _MESSAGE(43118, _("can't create gdi request"))
#define MSG_GDI_CANT_SEND_MESSAGE_TO_PORT_ON_HOST_SUSS   _MESSAGE(43121, _("unable to send message to "SFN" using port "sge_U32CFormat" on host "SFQ": "SFN))
#define MSG_GDI_UNABLE_TO_CONNECT_SUS                    _MESSAGE(43122, _("unable to contact "SFN" using port "sge_U32CFormat" on host "SFQ))
#define MSG_GDI_GETGRGIDXFAILEDERRORX_U                  _MESSAGE(43124, _("unable to resolve group name for group ID, "sge_U32CFormat))
#define MSG_GDI_SENDINGGDIREQUESTFAILED                  _MESSAGE(43125, _("failed sending gdi request"))
#define MSG_GDI_RECEIVEGDIREQUESTFAILED                  _MESSAGE(43126, _("failed receiving gdi request"))
#define MSG_GDI_SIGNALED                                 _MESSAGE(43127, _("signaled"))
#define MSG_GDI_GENERALERRORXSENDRECEIVEGDIREQUEST_I     _MESSAGE(43128, _("general error (%d) sending and receiving gdi request"))
#define MSG_GDI_POINTER_NULLLISTPASSEDTOSGESENDRECEIVGDIREQUEST   _MESSAGE(43129, _("NULL list passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_POINTER_NULLRHOSTPASSEDTOSGESENDRECEIVEGDIREQUEST _MESSAGE(43130, _("NULL rhost passed to sge_send_receive_gdi_request()"))
#define MSG_GDI_MEMORY_NOTENOUGHMEMORYFORPACKINGGDIREQUEST        _MESSAGE(43131, _("not enough memory for packing gdi request"))
#define MSG_GDI_REQUESTFORMATERROR                       _MESSAGE(43132, _("format error while packing gdi request"))
#define MSG_GDI_UNEXPECTEDERRORWHILEPACKINGGDIREQUEST    _MESSAGE(43133, _("unexpected error while packing gdi request"))
#define MSG_GDI_ERRORUNPACKINGGDIREQUEST_S               _MESSAGE(43134, _("error unpacking gdi request: "SFN))
#define MSG_GDI_ERROR_INVALIDVALUEXFORARTOOP_D           _MESSAGE(43137, _("invalid value ("sge_U32CFormat") for ar->op"))
#define MSG_GDI_CANTUNPACKGDIREQUEST                     _MESSAGE(43138, _("can't unpack gdi request"))
#define MSG_GDI_GDI_ALREADY_SETUP                        _MESSAGE(43139, _("GDI already setup"))

/**
 * cull/cull_where.c
 */
#define MSG_PACK_ERRORUNPACKING_S      _MESSAGE(43150, _("error unpacking: "SFN))
#define MSG_PACK_INVALIDPACKDATA       _MESSAGE(43151, _("the pack string contained invalid characters"))
#define MSG_PACK_WRONGPACKTYPE_UI      _MESSAGE(43152, _("wrong pack type (got: "sge_U32CFormat" / expected %d"))
 
/*
 * common/usage.c
 */
#define MSG_GDI_USAGE_SILENT          _MESSAGE(43293, _("startup silently"))

#define MSG_GDI_GENERAL_COM_ERROR_SS   _MESSAGE(43294, _("commlib error: "SFN" ("SFN2")"))
#define MSG_GDI_GENERAL_COM_ERROR_S    _MESSAGE(43292, _("commlib error: "SFN))

#define MSG_GDI_MULTI_THREADED_STARTUP       _MESSAGE(43295, _("starting up multi thread communication"))
#define MSG_GDI_SINGLE_THREADED_STARTUP      _MESSAGE(43296, _("starting up communication without threads"))
#define MSG_GDI_CANT_GET_COM_HANDLE_SSUUS    _MESSAGE(43297, _("communication error for \""SFN"/"SFN"/"sge_U32CFormat"\" running on port "sge_U32CFormat": "SFQ))
#define MSG_GDI_CANT_CONNECT_HANDLE_SSUUS    _MESSAGE(43298, _("communication error for \""SFN"/"SFN"/"sge_U32CFormat"\" using connect port "sge_U32CFormat": "SFQ))
#define MSG_GDI_HANDLE_CREATED_FOR_S         _MESSAGE(43300, _("created communication handle for component name "SFQ))
#define MSG_GDI_COULD_NOT_GET_COM_HANDLE_S   _MESSAGE(43301, _("alive check of qmaster failed for component "SFQ))
#define MSG_GDI_QMASTER_STILL_RUNNING        _MESSAGE(43302, _("qmaster is still running"))
#define MSG_GDI_ENDPOINT_UPTIME_UU           _MESSAGE(43303, _("endpoint is up since "sge_U32CFormat" seconds and has status "sge_U32CFormat))
#define MSG_GDI_ALREADY_CONECTED_SSU         _MESSAGE(43304, _("there is already a client endpoint %s/%s/"sge_U32CFormat" connected to qmaster service"))
#define MSG_GDI_ACCESS_DENIED_SSU            _MESSAGE(43305, _("qmaster service denies access from local endpoint %s/%s/"sge_U32CFormat))
#define MSG_GDI_CANT_CREATE_HANDLE_TOEXECD_S _MESSAGE(43306, _("can't create handle to execd \"%s\""))



#define MSG_SEC_CAROOTNOTFOUND_S         _MESSAGE(55000, _("CA_ROOT directory "SFQ" doesn't exist"))
#define MSG_SEC_CALOCALROOTNOTFOUND_S    _MESSAGE(55001, _("CA_LOCAL_ROOT directory "SFQ" doesn't exist"))
#define MSG_SEC_CAKEYFILENOTFOUND_S      _MESSAGE(55002, _("CA private key "SFQ" doesn't exist"))
#define MSG_SEC_CACERTFILENOTFOUND_S     _MESSAGE(55003, _("CA certificate "SFQ" doesn't exist"))
#define MSG_SEC_USERNOTFOUND_S           _MESSAGE(55007, _("user "SFQ" not found in password database"))
#define MSG_SEC_KEYFILENOTFOUND_S        _MESSAGE(55004, _("key "SFQ" doesn't exist"))
#define MSG_SEC_CERTFILENOTFOUND_S       _MESSAGE(55006, _("certificate "SFQ" doesn't exist"))
#define MSG_SEC_RANDFILENOTFOUND_S       _MESSAGE(55005, _("random data file "SFQ" doesn't exist"))
#define MSG_SEC_CERT_VERIFY_FUNC_NO_VAL  _MESSAGE(55011, _("certificate verify callback function called without value"))


#endif /* __MSG_GDILIB_H */

