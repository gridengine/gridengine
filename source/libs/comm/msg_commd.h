#ifndef __MSG_COMMD_H
#define __MSG_COMMD_H
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
** commd/ack.c
*/ 
#define MSG_RESETMESSAGE_READ_POS_0                   _MESSAGE(39000, _("read_ack read()==-1"))
#define MSG_RESETMESSAGE_READ_POS_0_S                 _MESSAGE(39001, _("read_ack read()==-1 errno="SFN""))
#define MSG_RESETMESSAGE_READ_I                       _MESSAGE(39002, _("read ackchar=%d"))

/* 
** commd/commd.c
*/ 
#define MSG_COMMD_POINTERNULL                         _MESSAGE(39003, _("(null)"))
#define MSG_COMMD_STATHOSTALIASFILEFAILED_SS          _MESSAGE(39004, _("can't stat host alias file "SFQ": "SFN""))
#define MSG_COMMD_OPENFILEFORMESSAGELOGFAILED_SS      _MESSAGE(39005, _("can't open file "SFQ" for message logging: "SFN""))
#define MSG_NET_USINGPORT_I                           _MESSAGE(39006, _("using port \"%d\"\n"))
#define MSG_NET_USINGSERVICE_S                        _MESSAGE(39007, _("using service "SFQ"\n"))
#define MSG_NET_OPENSTREAMSOCKETFAILED_S              _MESSAGE(39008, _("can't open stream socket: "SFN""))
#define MSG_NET_RESOLVESERVICEFAILED_SS               _MESSAGE(39009, _("can't resolve service "SFQ": "SFN""))
#define MSG_NET_BINDPORTFAILED_IS                     _MESSAGE(39010, _("can't bind port \"%d\": "SFN""))
#define MSG_NET_BOUNDTOPORT_I                         _MESSAGE(39011, _("bound to port %d\n"))
#define MSG_COMMD_SHUTDOWNDUECONTROLMESSAGE           _MESSAGE(39012, _("shutdown due to control message"))
#define MSG_NET_SELECTSOCKFDFAILED_ABORT_SI           _MESSAGE(39013, _("select error: "SFN" sockfd(%d) - aborting"))
#define MSG_NET_SELECTREADFAILEDMESSAGEFOLLOWS_SI     _MESSAGE(39014, _("select error: "SFN" read(%d) - corresponding message follows"))
#define MSG_NET_SELECTREADFAILEDCOMMPROCFOLLOWS_SI    _MESSAGE(39015, _("select error: "SFN" read(%d) - corresponding commproc follows"))
#define MSG_NET_SELECTREADFAILEDNOTCOMPLETE_SI        _MESSAGE(39016, _("select error: "SFN" read(%d) - not completly enrolled"))
#define MSG_NET_SELECTREADFAILEDNOCORSPDOBJECT_SI     _MESSAGE(39017, _("select error: "SFN" read(%d) - no corresponding object"))
#define MSG_NET_SELECTWRITEFAILEDMESSAGEFOLLOWS_SI    _MESSAGE(39018, _("select error: "SFN" write(%d) - corresponding message follows"))
#define MSG_NET_SELECTWRITEFAILEDNOCORSPDOBJECT_SI    _MESSAGE(39019, _("select error: "SFN" write(%d) - no corresponding object"))
#define MSG_NET_SELECTIGNCOMMPROCRCVANDEOF_I          _MESSAGE(39020, _("select error: ignoring commproc using fd %d because the fd is ready to receive AND ready to send an EOF"))
#define MSG_NET_SELECTERROR_SSS                       _MESSAGE(39021, _("select error: "SFN" ("SFN") ("SFN")"))
#define MSG_USAGE                                     _MESSAGE(39022, _("usage:"))
#define MSG_COMMD_s_OPT_USAGE                         _MESSAGE(39023, _("use this service for connections from other commds"))
#define MSG_COMMD_p_OPT_USAGE                         _MESSAGE(39024, _("use this port for connections from other commds"))
#define MSG_COMMD_S_OPT_USAGE                         _MESSAGE(39025, _("enable port security\n"))
#define MSG_COMMD_ml_OPT_USAGE                        _MESSAGE(39026, _("message logging to file\n"))
#define MSG_COMMD_ll_OPT_USAGE                        _MESSAGE(39027, _("logging level 2-7 (lower numbers log only higher priority messages\n" ))
#define MSG_COMMD_nd_OPT_USAGE                        _MESSAGE(39028, _("do not daemonize\n"   ))
#define MSG_COMMD_a_OPT_USAGE                         _MESSAGE(39029, _("file containing host aliases\n"))
#define MSG_COMMD_dhr_OPT_USAGE                       _MESSAGE(39030, _("disable regular hostname refresh\n"   ))
#define MSG_MEMORY_LACKOFMEMORY                       _MESSAGE(39031, _("LACK OF MEMORY"))
#define MSG_SIGNAL_SIGTERMSHUTDOWN                    _MESSAGE(39032, _("shutdown due to signal SIGTERM"))
#define MSG_COMMD_DUMPTOFILE                          _MESSAGE(39033, _("dump to file /tmp/commd/commd.dump"))
#define MSG_FILE_TOMANYFDSSTART                       _MESSAGE(39034, _("start of \"too may open fds open\" {"))
#define MSG_FILE_TOMANYFDSEND                         _MESSAGE(39035, _("end of \"too may open fds open\" }"))
#define MSG_MEMORY_MALLOCFAILEDFORPATHTOACTQMASTERFILE _MESSAGE(39036, _("can't malloc() for path to act_qmaster file"))
#define MSG_MEMORY_MALLOCFAILEDFORPATHTOPRODMODFILE    _MESSAGE(39037, _("can't malloc() for path to product mode file"))




/* 
** commd/commdcntl.c
*/ 
#define MSG_COMMDCNTL_k_OPT_USAGE                     _MESSAGE(39038, _("kill\n"))
#define MSG_COMMDCNTL_t_OPT_USAGE                     _MESSAGE(39039, _("trace\n"))
#define MSG_COMMDCNTL_d_OPT_USAGE                     _MESSAGE(39040, _("dump structures to"))
#define MSG_COMMDCNTL_p_OPT_USAGE                     _MESSAGE(39041, _("port commd is waiting on\n"))
#define MSG_COMMDCNTL_S_OPT_USAGE                     _MESSAGE(39042, _("secure mode\n"))
#define MSG_COMMDCNTL_U_OPT_USAGE                     _MESSAGE(39043, _("switch off secure mode\n"))
#define MSG_COMMDCNTL_gid_OPT_USAGE                   _MESSAGE(39044, _("get id of commproc\n"))
#define MSG_COMMDCNTL_unreg_OPT_USAGE                 _MESSAGE(39045, _("unregister commproc\n"))
#define MSG_COMMDCNTL_NOCTRLOPERATIONSPECIFIED        _MESSAGE(39046, _("no control operation specified\n"))
#define MSG_COMMDCNTL_SETCOMMLIBPARAM1RETURNED_II     _MESSAGE(39047, _("set_commlib_param(CL_P_COMMDPORT, %d) returns %d\n"))
#define MSG_COMMDCNTL_SETCOMMLIBPARAM2RETURNED_II     _MESSAGE(39048, _("set_commlib_param(CL_P_RESERVED_PORT, %d) returns %d\n"))
#define MSG_CNTL_ERROR_S                              _MESSAGE(39049, _("error: "SFN"\n"))



/* 
** commd/commlib.c
*/ 

#define MSG_COMMLIB_CL_OK             _MESSAGE(39050, _("CL_OK" ))
#define MSG_COMMLIB_CL_RANGE          _MESSAGE(39051, _("INVALID RANGE" ))
#define MSG_COMMLIB_CL_CREATESOCKET   _MESSAGE(39052, _("CANNOT CREATE SOCKET" ))
#define MSG_COMMLIB_CL_RESOLVE        _MESSAGE(39053, _("RESOLVING PROBLEM" ))
#define MSG_COMMLIB_CL_CONNECT        _MESSAGE(39054, _("CANNOT CONNECT" ))
#define MSG_COMMLIB_CL_WRITE          _MESSAGE(39055, _("WRITE ERROR" ))
#define MSG_COMMLIB_CL_ALREADYDONE    _MESSAGE(39056, _("SAME OPERATION DONE AGAIN" ))
#define MSG_COMMLIB_CL_LOCALHOSTNAME  _MESSAGE(39057, _("LOCALHOSTNAME"    ))
#define MSG_COMMLIB_CL_NOTENROLLED    _MESSAGE(39058, _("NOT ENROLLED" ))
#define MSG_COMMLIB_CL_SERVICE        _MESSAGE(39059, _("CANT GET SERVICE"))
#define MSG_COMMLIB_CL_READ           _MESSAGE(39060, _("READ ERROR" ))
#define MSG_COMMLIB_CL_MALLOC         _MESSAGE(39061, _("OUT OF MEMORY" ))
#define MSG_COMMLIB_CL_UNKNOWN_PARAM  _MESSAGE(39062, _("UNKNOWN PARAMETER" ))
#define MSG_COMMLIB_CL_INTR           _MESSAGE(39063, _("INTERUPTED BY SIGNAL" ))
#define MSG_COMMLIB_CL_READ_TIMEOUT   _MESSAGE(39064, _("READ TIMEOUT" ))
#define MSG_COMMLIB_CL_WRITE_TIMEOUT  _MESSAGE(39065, _("WRITE TIMEOUT"    ))
#define MSG_COMMLIB_CL_CHKSUM         _MESSAGE(39066, _("CHECKSUM ERROR" ))
#define MSG_COMMLIB_CL_RRESVPORT      _MESSAGE(39067, _("CANNOT GET RESERVED PORT"))
#define MSG_COMMLIB_SEC_SEND          _MESSAGE(39068, _("SEC_SEND FAILED" ))
#define MSG_COMMLIB_SEC_RECEIVE       _MESSAGE(39069, _("SEC_RECEIVE FAILED" ))
#define MSG_COMMLIB_SEC_ANNOUNCE      _MESSAGE(39070, _("SEC_ANNOUNCE FAILED" ))
#define MSG_COMMLIB_CL_PERM               _MESSAGE(39071, _("PERMISSION DENIED"))
#define MSG_COMMLIB_CL_UNKNOWN_TARGET     _MESSAGE(39072, _("UNKNOWN TARGET" ))
#define MSG_COMMLIB_CL_UNKNOWN_RECEIVER   _MESSAGE(39073, _("UNKNOWN RECEIVER"))
#define MSG_COMMLIB_NACK_COMMD_NOT_READY  _MESSAGE(39074, _("COMMD NOT READY"))
#define MSG_COMMLIB_NACK_UNKNOWN_HOST     _MESSAGE(39075, _("UNKNOWN_HOST"))
#define MSG_COMMLIB_NACK_NO_MESSAGE       _MESSAGE(39076, _("NO MESSAGE AVAILABLE"))
#define MSG_COMMLIB_NACK_ENROLL           _MESSAGE(39077, _("NOT ENROLLED"))
#define MSG_COMMLIB_NACK_OPONCE           _MESSAGE(39078, _("OPERATION ALLOWED ONLY ONCE AT A TIME"))
#define MSG_COMMLIB_NACK_DELIVERY         _MESSAGE(39079, _("COULD NOT DELIVER"))
#define MSG_COMMLIB_NACK_TIMEOUT          _MESSAGE(39080, _("CONNECTION TIMED OUT"))
#define MSG_COMMLIB_NACK_CONFLICT         _MESSAGE(39081, _("COMMPROC ALREADY REGISTERED"))
#define MSG_COMMLIB_UNKNOWN_ERROR         _MESSAGE(39082, _("UNKNOWN ERROR"))
#define MSG_COMMLIB_LOST_CONNECTION       _MESSAGE(39083, _("lost connection\n"))
#define MSG_COMMLIB_BITFLIPPER_FS         _MESSAGE(39084, _("-----------> bitflipper %f: "SFN"\n"))





/* 
** commd/commlib_last_heard.c
*/ 
#define MSG_COMMLIB_LAST_HEARD_USIS       _MESSAGE(39085, _(U32CFormat" = last_heard_from(commproc="SFN", id=%d, host="SFN")\n"))
#define MSG_COMMLIB_SET_LAST_HEARD_ISIU   _MESSAGE(39086, _("%d = set_last_heard_from(commproc="SFN", id=%d, host="SFN", time="U32CFormat")\n"))
#define MSG_COMMLIB_DROPPING_SISUU         _MESSAGE(39087, _("dropping (commproc="SFN", id=%d, host="SFN", time="U32CFormat") now = "U32CFormat"\n"))
#define MSG_COMMLIB_RESET_LAST_HEARD_SISU _MESSAGE(39088, _("reset_last_heard drops (commproc="SFN", id=%d, host="SFN", time="U32CFormat")\n"))

/* 
** commd/commproc.c
*/ 
#define MSG_COMMPROC_NAMEANDID_SI                 _MESSAGE(39089, _("name="SFN" id=%d\n"))
#define MSG_COMMPROC_HOST_S                       _MESSAGE(39090, _("host="SFN"\n"))
#define MSG_COMMPROC_USING_FD_I                   _MESSAGE(39091, _("using fd=%d\n"))
#define MSG_COMMPROC_WAITING_ON_FD_I              _MESSAGE(39092, _("waiting on fd=%d\n"))
#define MSG_COMMPROC_WAITING_FOR_COMPONENT_ID_SI  _MESSAGE(39093, _("waiting for component "SFN" with id %d\n"))
#define MSG_COMMPROC_ANY                          _MESSAGE(39094, _("any"))
#define MSG_COMMPROC_NONE                         _MESSAGE(39095, _("none"))
#define MSG_COMMPROC_ON_HOST_S                    _MESSAGE(39096, _("on host "SFN"\n"))
#define MSG_COMMPROC_INACTIVEFOR_SIU              _MESSAGE(39097, _("commproc "SFN":%d was inactive for "U32CFormat" seconds" ))

/* 
** commd/process_received_message.c
*/ 
#define MSG_PROC_RECEIVED_MESS_OUTOFCOMMPROCIDS    _MESSAGE(39098, _("out of commproc ids"))
#define MSG_PROC_RECEIVED_MESS_RECEIVEQUERYMESSFROMUNOWNHOST_S    _MESSAGE(39099, _("receive query: asking for a message from an unknown host: "SFN""   ))
#define MSG_PROC_RECEIVED_MESS_ALLREADYINRECEIVECLOSEFD_ISISI           _MESSAGE(39100, _("commproc already in receive - closing old fd (w_fd=%d, w_name="SFN", w_id=%d, w_host="SFN", w_tag=%d)" ))
#define MSG_PROC_RECEIVED_MESS_UNKNOWNRECEIVERHOST_S              _MESSAGE(39101, _("send message: unknown receiver host >"SFN"<"  ))
#define MSG_PROC_RECEIVED_MESS_UNKNOWNSENDERHOST_S                _MESSAGE(39102, _("send message: unknown sender host "SFN""))


/* 
** commd/prolog.c
*/ 
#define MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_0       _MESSAGE(39103, _("write_message_prolog write()==0"))
#define MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_NEG_1   _MESSAGE(39104, _("write_message_prolog write()==-1"))

/* 
** commd/rwfd.c
*/ 
#define MSG_RWFD_CONNECTINGFAILED_SSISSIS                   _MESSAGE(39105, _("connecting "SFN":"SFN":%d->"SFN":"SFN":%d failed: "SFN""))
#define MSG_RWFD_NOCOMMPROCWAITINGONFD_I                    _MESSAGE(39106, _("no commproc waiting on fd %d"))
#define MSG_RWFD_DELETECOMMPROCBROKENCONNECTIONFILE_S       _MESSAGE(39107, _("delete commproc "SFN": broken connection while waiting for message ack"))
#define MSG_RWFD_BIGXSENDSMESSAGEYBYTESTOZ_USSIIUSSI        _MESSAGE(39108, _("BIG(SGE_MSG_LOG_SIZE="U32CFormat"): ("SFN"/"SFN"/%d) sends message(tag=%d) "U32CFormat" bytes to ("SFN"/"SFN"/%d)"))
#define MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_0       _MESSAGE(39109, _("write_message write()==0"))
#define MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_NEG_1_S _MESSAGE(39110, _("write_message write()==-1: "SFN""))
#define MSG_NET_ACCEPTFAILED_S                              _MESSAGE(39111, _("accept failed: "SFN""))
#define MSG_NET_OPENSTREAMSOCKETFAILED                      _MESSAGE(39112, _("can't open stream socket"))
#define MSG_NET_ERRORCONNECTINGTOHOST                       _MESSAGE(39113, _("error connecting to host "))
#define MSG_RWFD_DELIVERMESSAGEFAILEDBUTALIVE               _MESSAGE(39114, _("can't deliver message (target host alive, but cant contact commd)"))
#define MSG_RWFD_DELIVERMESSAGEFAILEDMAXDELTIMEEXCEEDED     _MESSAGE(39115, _("can't deliver message (MAXDELIVERTIME exceeded) -> ack if synchron and delete"))


/* 
** commd/tstrcv.c
*/ 
#define MSG_TSTRCV_s_OPT_USAGE         _MESSAGE(39116, _("receive synchron (wait until message arrives or TIMEOUT_SRCV happens)\n"))
#define MSG_TSTRCV_t_OPT_USAGE         _MESSAGE(39117, _("set timeout TIMEOUT_SRCV and TIMEOUT\n        TIMEOUT_SRCV is the time we maximal wait in a synchron receive\n        TIMEOUT is the time we maximal wait in a read on a communication file desc.\n"))
#define MSG_TSTRCV_host_OPT_USAGE      _MESSAGE(39118, _("specify target\n"))
#define MSG_TSTRCV_enroll_OPT_USAGE    _MESSAGE(39119, _("do enroll\n"))
#define MSG_TSTRCV_S_OPT_USAGE         _MESSAGE(39120, _("secure mode (use reserved ports)\n"))
#define MSG_TSTRCV_r_OPT_USAGE         _MESSAGE(39121, _("repeat whole stuff includsive enrolling\n"))
#define MSG_TSTRCV_cfd_OPT_USAGE       _MESSAGE(39122, _("close file descriptors between messages\n"))
#define MSG_ERROR_S                    _MESSAGE(39123, _("error: "SFN"\n"))


/* 
** commd/tstsnd.c
*/ 
#define MSG_TSTSND_s_OPT_USAGE            _MESSAGE(39124, _("synchron send\n" ))
#define MSG_TSTSND_host_OPT_USAGE         _MESSAGE(39125, _("address of receiver\n"))
#define MSG_TSTSND_mt_OPT_USAGE           _MESSAGE(39126, _("message tag\n"))
#define MSG_TSTSND_enrollname_OPT_USAGE   _MESSAGE(39127, _("enroll with this address\n"))
#define MSG_TSTSND_t_OPT_USAGE            _MESSAGE(39128, _("set timeout for communication\n"))
#define MSG_TSTSND_p_OPT_USAGE            _MESSAGE(39129, _("port under which to contact commd\n"))
#define MSG_TSTSND_S_OPT_USAGE            _MESSAGE(39130, _("secure mode\n"))
#define MSG_TSTSND_r_OPT_USAGE            _MESSAGE(39131, _("repetitions\n"))
#define MSG_TSTSND_cfd_OPT_USAGE          _MESSAGE(39132, _("close file descriptors between messages\n"))
#define MSG_MEMORY_MALLOCSIZEFAILED_D     _MESSAGE(39133, _("error can't malloc for buffer size "U32CFormat"\n"))
#define MSG_TSTSND_ENROLLED               _MESSAGE(39134, _("enrolled\n"))
#define MSG_TSTSND_NOTENROLLED            _MESSAGE(39135, _("not enrolled\n"))
#define MSG_NET_UNIQUEHOSTNAMEIS_S        _MESSAGE(39136, _("unique hostname = "SFQ"\n"))

#endif /* __MSG_COMMD_H */
