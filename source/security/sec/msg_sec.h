#ifndef __MSG_SEC_H
#define __MSG_SEC_H
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


#define MSG_SEC_CAROOTNOTFOUND_S         _("CA_ROOT directory "SFQ" doesn't exist\n")
#define MSG_SEC_CALOCALROOTNOTFOUND_S    _("CA_LOCAL_ROOT directory "SFQ" doesn't exist\n")
#define MSG_SEC_CAKEYFILENOTFOUND_S      _("CA private key "SFQ" doesn't exist\n")
#define MSG_SEC_CACERTFILENOTFOUND_S     _("CA certificate "SFQ" doesn't exist\n")
#define MSG_SEC_KEYFILENOTFOUND_S        _("key "SFQ" doesn't exist\n")
#define MSG_SEC_RANDFILENOTFOUND_S        _("random data file "SFQ" doesn't exist\n")
#define MSG_SEC_CERTFILENOTFOUND_S       _("certificate "SFQ" doesn't exist\n")
#define MSG_SEC_USERNOTFOUND_S           _("user "SFQ" not found in password database\n")
#define MSG_SEC_RANDFILENOTSET           _("RANDFILE environment variable not set")
#define MSG_SEC_CANTOPENCERTFILE_SS      _("Cannot open Cert_file '%s': %s!!\n")
#define MSG_SEC_FAILEDVERIFYOWNCERT      _("failed verify own certificate\n")
#define MSG_SEC_CANTOPENKEYFILE_SS       _("Cannot open key file '%s': %s!\n")
#define MSG_SEC_INITPACKBUFFERFAILED     _("init_packbuffer failed\n")
#define MSG_SEC_PACKANNOUNCEFAILED       _("sec_pack_announce failed\n")
#define MSG_SEC_UNPACKANNOUNCEFAILED     _("sec_unpack_announce failed\n")
#define MSG_SEC_RESPONSEFAILED_SISIS      _("failed get sec_response from (%s:%d:%s:%d):%s\n")
#define MSG_SEC_MASTERERROR              _("master reports error: ")
#define MSG_SEC_UNEXPECTEDTAG            _("received unexpected TAG from master\n")
#define MSG_SEC_PACKRESPONSEFAILED       _("sec_pack_response failed")
#define MSG_SEC_UNPACKRESPONSEFAILED     _("sec_unpack_response failed")
#define MSG_SEC_PACKMSGFAILED            _("sec_pack_message failed")
#define MSG_SEC_UNPACKMSGFAILED          _("sec_unpack_message failed")
#define MSG_SEC_MASTERCERTREADFAILED     _("failed read master certificate\n")
#define MSG_SEC_CLIENTCERTREADFAILED     _("failed read client certificate\n")
#define MSG_SEC_CLIENTCERTVERIFYFAILED   _("failed verify client certificate\n")
#define MSG_SEC_MASTERGETPUBKEYFAILED    _("cannot extract public key from master certificate\n")
#define MSG_SEC_MASTERBADCHALLENGE       _("challenge from master is bad\n")
#define MSG_SEC_CLIENTGETPUBKEYFAILED    _("cant get client public key\n")
#define MSG_SEC_ENCRYPTCHALLENGEFAILED   _("failed encrypt challenge MAC\n")
#define MSG_SEC_ENCRYPTMACFAILED         _("failed encrypt MAC\n")
#define MSG_SEC_ENCRYPTMSGFAILED         _("failed encrypt message\n")
#define MSG_SEC_SEALINITFAILED           _("EVP_SealInit failed\n")
#define MSG_SEC_ENCRYPTKEYFAILED         _("failed encrypt keys\n")
#define MSG_SEC_INSERTCONNECTIONFAILED   _("failed insert Connection to list\n")
#define MSG_SEC_CONNECTIONNOENTRY        _("no list entry for connection\n")
#define MSG_SEC_CONNECTIONNOENTRY_SSI    _("no list entry for connection (%s:%s:%d)!\n")
#define MSG_SEC_SENDRESPONSEFAILED_SIS   _("Send response to (%s:%d:%s) failed\n")
#define MSG_SEC_RESPONSEFAILED_SIS       _("sec_respond_announce to (%s:%d:%s) failed\n")
#define MSG_SEC_SUMMONSESFAILED_SIS      _("Failed send summonses for announce to (%s:%d:%s)\n")
#define MSG_SEC_ANNOUNCEFAILED           _("Anounce failed\n")
#define MSG_SEC_SETSECDATAFAILED         _("failed to set security data\n")
#define MSG_SEC_MSGENCFAILED             _("failed encrypt message\n")
#define MSG_SEC_CONNIDSETFAILED          _("failed set connection ID\n")
#define MSG_SEC_CONNIDGETFAILED          _("failed get connection ID\n")
#define MSG_SEC_HANDLEANNOUNCEFAILED_SSI _("failed handle announce for (%s:%s:%d)\n")
#define MSG_SEC_MSGDECFAILED_SSI         _("failed decrypt message (%s:%s:%d)\n")
#define MSG_SEC_MSGDECFAILED             _("failed decrypt message\n")
#define MSG_SEC_HANDLEDECERRFAILED       _("failed handle decrypt error\n")
#define MSG_SEC_CERTNOTYETVALID          _("certificate not yet valid\n")
#define MSG_SEC_CERTEXPIRED              _("certificate has expired\n")
#define MSG_SEC_I2DX509FAILED            _("i2d_x509 failed\n")
#define MSG_SEC_EVPOPENINITFAILED        _("EVP_OpenInit failed decrypt keys\n")
#define MSG_SEC_EVPOPENUPDATEFAILED      _("EVP_OpenUpdate failed decrypt keys\n")
#define MSG_SEC_RMRECONNECTFAILED_S      _("failed remove reconnect file '%s'\n")
#define MSG_SEC_ENCRECONNECTFAILED       _("failed encrypt reconnect data\n")
#define MSG_SEC_DECRECONNECTFAILED       _("failed decrypt reconnect data\n")
#define MSG_SEC_UNPACKRECONNECTFAILED    _("failed unpack reconnect data\n")
#define MSG_SEC_PACKRECONNECTFAILED      _("failed pack reconnect data\n")
#define MSG_SEC_INITPBFAILED             _("failed init_packbuffer_from_buffer\n")
#define MSG_SEC_NOCONN_I                 _("no connection %d\n")
#define MSG_SEC_CONNIDSHOULDBE_II        _("connection id is %d, but should be %d!\n")
#define MSG_SEC_DECMACFAILED             _("failed decrypt MAC\n")
#define MSG_SEC_CANTREAD                 _("can't read from file\n")
#define MSG_SEC_CANTWRITE_SS             _("can't open file '%s': %s\n")
#define MSG_SEC_SENDERRFAILED            _("failed send error message\n")
#define MSG_SEC_PACKCONNIDFAILED         _("failed pack ConnID\n")
#define MSG_SEC_UNPACKCONNIDFAILED       _("failed unpack ConnID\n")


#endif /* __MSG_SEC_H */
