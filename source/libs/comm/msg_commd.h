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
#define MSG_RESETMESSAGE_READ_POS_0                   _("read_ack read()==-1")
#define MSG_RESETMESSAGE_READ_POS_0_S                 _("read_ack read()==-1 errno=%s")
#define MSG_RESETMESSAGE_READ_I                       _("read ackchar=%d")

/* 
** commd/commd.c
*/ 
#define MSG_COMMD_POINTERNULL                         _("(null)")
#define MSG_COMMD_STATHOSTALIASFILEFAILED_SS          _("can't stat host alias file \"%s\": %s")
#define MSG_COMMD_OPENFILEFORMESSAGELOGFAILED_SS      _("can't open file \"%s\" for message logging: %s")
#define MSG_NET_USINGPORT_I                           _("using port \"%d\"\n")
#define MSG_NET_USINGSERVICE_S                        _("using service \"%s\"\n")
#define MSG_NET_OPENSTREAMSOCKETFAILED_S              _("can't open stream socket: %s")
#define MSG_NET_RESOLVESERVICEFAILED_SS               _("can't resolve service \"%s\": %s")
#define MSG_NET_BINDPORTFAILED_IS                     _("can't bind port \"%d\": %s")
#define MSG_NET_BOUNDTOPORT_I                         _("bound to port %d\n")
#define MSG_COMMD_SHUTDOWNDUECONTROLMESSAGE           _("shutdown due to control message")
#define MSG_NET_SELECTSOCKFDFAILED_ABORT_SI           _("select error: %s sockfd(%d) - aborting")
#define MSG_NET_SELECTREADFAILEDMESSAGEFOLLOWS_SI     _("select error: %s read(%d) - corresponding message follows")
#define MSG_NET_SELECTREADFAILEDCOMMPROCFOLLOWS_SI    _("select error: %s read(%d) - corresponding commproc follows")
#define MSG_NET_SELECTREADFAILEDNOTCOMPLETE_SI        _("select error: %s read(%d) - not completly enrolled")
#define MSG_NET_SELECTREADFAILEDNOCORSPDOBJECT_SI     _("select error: %s read(%d) - no corresponding object")
#define MSG_NET_SELECTWRITEFAILEDMESSAGEFOLLOWS_SI    _("select error: %s write(%d) - corresponding message follows")
#define MSG_NET_SELECTWRITEFAILEDNOCORSPDOBJECT_SI    _("select error: %s write(%d) - no corresponding object")
#define MSG_NET_SELECTDELCOMMPROCRCVANDEOF_I          _("select error: deleting commproc using fd %d because the fd is ready to receive AND ready to send an EOF")
#define MSG_NET_SELECTERROR_SSS                       _("select error: %s (%s) (%s)")
#define MSG_USAGE                                     _("usage:")
#define MSG_COMMD_s_OPT_USAGE                         _("use this service for connections from other commds")
#define MSG_COMMD_p_OPT_USAGE                         _("use this port for connections from other commds")
#define MSG_COMMD_S_OPT_USAGE                         _("enable port security\n")
#define MSG_COMMD_ml_OPT_USAGE                        _("message logging to file\n")
#define MSG_COMMD_ll_OPT_USAGE                        _("logging level 2-7 (lower numbers log only higher priority messages\n" )
#define MSG_COMMD_nd_OPT_USAGE                        _("do not daemonize\n"   )
#define MSG_COMMD_a_OPT_USAGE                         _("file containing host aliases\n")
#define MSG_COMMD_dhr_OPT_USAGE                       _("disable regular hostname refresh\n"   )
#define MSG_MEMORY_LACKOFMEMORY                       _("LACK OF MEMORY")
#define MSG_SIGNAL_SIGTERMSHUTDOWN                    _("shutdown due to signal SIGTERM")
#define MSG_COMMD_DUMPTOFILE                          _("dump to file /tmp/commd/commd.dump")
#define MSG_FILE_TOMANYFDSSTART                       _("start of \"too may open fds open\" {")
#define MSG_FILE_TOMANYFDSEND                         _("end of \"too may open fds open\" }")
#define MSG_MEMORY_MALLOCFAILEDFORPATHTOHOSTALIASFILE _("can't malloc() for path to host alias file")

/* 
** commd/commdcntl.c
*/ 
#define MSG_COMMDCNTL_k_OPT_USAGE                     _("kill\n")
#define MSG_COMMDCNTL_t_OPT_USAGE                     _("trace\n")
#define MSG_COMMDCNTL_d_OPT_USAGE                     _("dump structures to")
#define MSG_COMMDCNTL_p_OPT_USAGE                     _("port commd is waiting on\n")
#define MSG_COMMDCNTL_S_OPT_USAGE                     _("secure mode\n")
#define MSG_COMMDCNTL_gid_OPT_USAGE                   _("get id of commproc\n")
#define MSG_COMMDCNTL_unreg_OPT_USAGE                 _("unregister commproc\n")
#define MSG_COMMDCNTL_NOCTRLOPERATIONSPECIFIED        _("no control operation specified\n")
#define MSG_COMMDCNTL_SETCOMMLIBPARAM1RETURNED_II     _("set_commlib_param(CL_P_COMMDPORT, %d) returns %d\n")
#define MSG_COMMDCNTL_SETCOMMLIBPARAM2RETURNED_II     _("set_commlib_param(CL_P_RESERVED_PORT, %d) returns %d\n")
#define MSG_CNTL_ERROR_S                              _("error: %s\n")



/* 
** commd/commlib.c
*/ 

#define MSG_COMMLIB_CL_OK             _("CL_OK" )
#define MSG_COMMLIB_CL_RANGE          _("INVALID RANGE" )
#define MSG_COMMLIB_CL_CREATESOCKET   _("CANNOT CREATE SOCKET" )
#define MSG_COMMLIB_CL_RESOLVE        _("RESOLVING PROBLEM" )
#define MSG_COMMLIB_CL_CONNECT        _("CANNOT CONNECT" )
#define MSG_COMMLIB_CL_WRITE          _("WRITE ERROR" )
#define MSG_COMMLIB_CL_ALREADYDONE    _("SAME OPERATION DONE AGAIN" )
#define MSG_COMMLIB_CL_LOCALHOSTNAME  _("LOCALHOSTNAME"    )
#define MSG_COMMLIB_CL_NOTENROLLED    _("NOT ENROLLED" )
#define MSG_COMMLIB_CL_SERVICE        _("CANT GET SERVICE")
#define MSG_COMMLIB_CL_READ           _("READ ERROR" )
#define MSG_COMMLIB_CL_MALLOC         _("OUT OF MEMORY" )
#define MSG_COMMLIB_CL_UNKNOWN_PARAM  _("UNKNOWN PARAMETER" )
#define MSG_COMMLIB_CL_INTR           _("INTERUPTED BY SIGNAL" )
#define MSG_COMMLIB_CL_READ_TIMEOUT   _("READ TIMEOUT" )
#define MSG_COMMLIB_CL_WRITE_TIMEOUT  _("WRITE TIMEOUT"    )
#define MSG_COMMLIB_CL_CHKSUM         _("CHECKSUM ERROR" )
#define MSG_COMMLIB_CL_RRESVPORT      _("CANNOT GET RESERVED PORT")
#define MSG_COMMLIB_SEC_SEND          _("SEC_SEND FAILED" )
#define MSG_COMMLIB_SEC_RECEIVE       _("SEC_RECEIVE FAILED" )
#define MSG_COMMLIB_SEC_ANNOUNCE      _("SEC_ANNOUNCE FAILED" )
#define MSG_COMMLIB_CL_PERM               _("PERMISSION DENIED")
#define MSG_COMMLIB_CL_UNKNOWN_TARGET     _("UNKNOWN TARGET" )
#define MSG_COMMLIB_CL_UNKNOWN_RECEIVER   _("UNKNOWN RECEIVER")
#define MSG_COMMLIB_NACK_COMMD_NOT_READY  _("COMMD NOT READY")
#define MSG_COMMLIB_NACK_UNKNOWN_HOST     _("UNKNOWN_HOST")
#define MSG_COMMLIB_NACK_NO_MESSAGE       _("NO MESSAGE AVAILABLE")
#define MSG_COMMLIB_NACK_ENROLL           _("NOT ENROLLED")
#define MSG_COMMLIB_NACK_OPONCE           _("OPERATION ALLOWED ONLY ONCE AT A TIME")
#define MSG_COMMLIB_NACK_DELIVERY         _("COULD NOT DELIVER")
#define MSG_COMMLIB_NACK_TIMEOUT          _("CONNECTION TIMED OUT")
#define MSG_COMMLIB_NACK_CONFLICT         _("COMMPROC ALREADY REGISTERED")
#define MSG_COMMLIB_UNKNOWN_ERROR         _("UNKNOWN ERROR")
#define MSG_COMMLIB_LOST_CONNECTION       _("lost connection\n")
#define MSG_COMMLIB_BITFLIPPER_FS         _("-----------> bitflipper %f: %s\n")





/* 
** commd/commlib_last_heard.c
*/ 
#define MSG_COMMLIB_LAST_HEARD_USIS       _(U32CFormat" = last_heard_from(commproc=%s, id=%d, host=%s)\n")
#define MSG_COMMLIB_SET_LAST_HEARD_ISIU   _("%d = set_last_heard_from(commproc=%s, id=%d, host=%s, time="U32CFormat")\n")
#define MSG_COMMLIB_DROPPING_SISUU         _("dropping (commproc=%s, id=%d, host=%s, time="U32CFormat") now = "U32CFormat"\n")
#define MSG_COMMLIB_RESET_LAST_HEARD_SISU _("reset_last_heard drops (commproc=%s, id=%d, host=%s, time="U32CFormat")\n")

/* 
** commd/commproc.c
*/ 
#define MSG_COMMPROC_NAMEANDID_SI                 _("name=%s id=%d\n")
#define MSG_COMMPROC_HOST_S                       _("host=%s\n")
#define MSG_COMMPROC_USING_FD_I                   _("using fd=%d\n")
#define MSG_COMMPROC_WAITING_ON_FD_I              _("waiting on fd=%d\n")
#define MSG_COMMPROC_WAITING_FOR_COMPONENT_ID_SI  _("waiting for component %s with id %d\n")
#define MSG_COMMPROC_ANY                          _("any")
#define MSG_COMMPROC_NONE                         _("none")
#define MSG_COMMPROC_ON_HOST_S                    _("on host %s\n")
#define MSG_COMMPROC_INACTIVEFOR_SIU              _("commproc %s:%d was inactive for "U32CFormat" seconds" )


/* 
** commd/host.c
*/ 
#define MSG_NET_GETHOSTNAMEFAILED                  _("gethostname failed")
#define MSG_NET_RESOLVINGLOCALHOSTFAILED           _("failed resolving local host")

/* 
** commd/process_received_message.c
*/ 
#define MSG_PROC_RECEIVED_MESS_OUTOFCOMMPROCIDS    _("out of commproc ids")
#define MSG_PROC_RECEIVED_MESS_RECEIVEQUERYMESSFROMUNOWNHOST_S    _("receive query: asking for a message from an unknown host: %s"   )
#define MSG_PROC_RECEIVED_MESS_ALLREADYINRECEIVECLOSEFD_ISISI           _("commproc already in receive - closing old fd (w_fd=%d, w_name=%s, w_id=%d, w_host=%s, w_tag=%d)" )
#define MSG_PROC_RECEIVED_MESS_UNKNOWNRECEIVERHOST_S              _("send message: unknown receiver host >%s<"  )
#define MSG_PROC_RECEIVED_MESS_UNKNOWNSENDERHOST_S                _("send message: unknown sender host %s")


/* 
** commd/prolog.c
*/ 
#define MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_0       _("write_message_prolog write()==0")
#define MSG_RESETMESSAGE_WRITE_MESSAGE_PROLOG_WRITE_NEG_1   _("write_message_prolog write()==-1")

/* 
** commd/rwfd.c
*/ 
#define MSG_RWFD_CONNECTINGFAILED_SSISSIS                   _("connecting %s:%s:%d->%s:%s:%d failed: %s")
#define MSG_RWFD_NOCOMMPROCWAITINGONFD_I                    _("no commproc waiting on fd %d")
#define MSG_RWFD_DELETECOMMPROCBROKENCONNECTIONFILE_S       _("delete commproc %s: broken connection while waiting for message ack")
#define MSG_RWFD_BIGXSENDSMESSAGEYBYTESTOZ_USSIIUSSI        _("BIG(SGE_MSG_LOG_SIZE="U32CFormat"): (%s/%s/%d) sends message(tag=%d) "U32CFormat" bytes to (%s/%s/%d)")
#define MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_0       _("write_message write()==0")
#define MSG_RESETMESSAGE_WRITE_MESSAGE_WRITE_NEG_1_S _("write_message write()==-1: %s")
#define MSG_NET_ACCEPTFAILED_S                              _("accept failed: %s")
#define MSG_NET_OPENSTREAMSOCKETFAILED                      _("can't open stream socket")
#define MSG_NET_ERRORCONNECTINGTOHOST                       _("error connecting to host ")
#define MSG_RWFD_DELIVERMESSAGEFAILEDBUTALIVE               _("can't deliver message (target host alive, but cant contact commd)")
#define MSG_RWFD_DELIVERMESSAGEFAILEDMAXDELTIMEEXCEEDED     _("can't deliver message (MAXDELIVERTIME exceeded) -> ack if synchron and delete")


/* 
** commd/tstrcv.c
*/ 
#define MSG_TSTRCV_s_OPT_USAGE         _("receive synchron (wait until message arrives or TIMEOUT_SRCV happens)\n")
#define MSG_TSTRCV_t_OPT_USAGE         _("set timeout TIMEOUT_SRCV and TIMEOUT\n        TIMEOUT_SRCV is the time we maximal wait in a synchron receive\n        TIMEOUT is the time we maximal wait in a read on a communication file desc.\n")
#define MSG_TSTRCV_host_OPT_USAGE      _("specify target\n")
#define MSG_TSTRCV_enroll_OPT_USAGE    _("do enroll\n")
#define MSG_TSTRCV_S_OPT_USAGE         _("secure mode (use reserved ports)\n")
#define MSG_TSTRCV_r_OPT_USAGE         _("repeat whole stuff includsive enrolling\n")
#define MSG_TSTRCV_cfd_OPT_USAGE       _("close file descriptors between messages\n")
#define MSG_ERROR_S                    _("error: %s\n")


/* 
** commd/tstsnd.c
*/ 
#define MSG_TSTSND_s_OPT_USAGE            _("synchron send\n" )
#define MSG_TSTSND_host_OPT_USAGE         _("address of receiver\n")
#define MSG_TSTSND_mt_OPT_USAGE           _("message tag\n")
#define MSG_TSTSND_enrollname_OPT_USAGE   _("enroll with this address\n")
#define MSG_TSTSND_t_OPT_USAGE            _("set timeout for communication\n")
#define MSG_TSTSND_p_OPT_USAGE            _("port under which to contact commd\n")
#define MSG_TSTSND_S_OPT_USAGE            _("secure mode\n")
#define MSG_TSTSND_r_OPT_USAGE            _("repetitions\n")
#define MSG_TSTSND_cfd_OPT_USAGE          _("close file descriptors between messages\n")
#define MSG_MEMORY_MALLOCSIZEFAILED_D     _("error can't malloc for buffer size "U32CFormat"\n")
#define MSG_TSTSND_ENROLLED               _("enrolled\n")
#define MSG_TSTSND_NOTENROLLED            _("not enrolled\n")
#define MSG_NET_UNIQUEHOSTNAMEIS_S        _("unique hostname = \"%s\"\n")

#endif /* __MSG_COMMD_H */
