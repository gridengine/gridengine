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


#define MSG_SEC_CAROOTNOTFOUND_S         _MESSAGE(55000, _("CA_ROOT directory "SFQ" doesn't exist\n"))
#define MSG_SEC_CALOCALROOTNOTFOUND_S    _MESSAGE(55001, _("CA_LOCAL_ROOT directory "SFQ" doesn't exist\n"))
#define MSG_SEC_CAKEYFILENOTFOUND_S      _MESSAGE(55002, _("CA private key "SFQ" doesn't exist\n"))
#define MSG_SEC_CACERTFILENOTFOUND_S     _MESSAGE(55003, _("CA certificate "SFQ" doesn't exist\n"))
#define MSG_SEC_KEYFILENOTFOUND_S        _MESSAGE(55004, _("key "SFQ" doesn't exist\n"))
#define MSG_SEC_RANDFILENOTFOUND_S        _MESSAGE(55005, _("random data file "SFQ" doesn't exist\n"))
#define MSG_SEC_CERTFILENOTFOUND_S       _MESSAGE(55006, _("certificate "SFQ" doesn't exist\n"))
#define MSG_SEC_USERNOTFOUND_S           _MESSAGE(55007, _("user "SFQ" not found in password database\n"))
#define MSG_SEC_RANDFILENOTSET           _MESSAGE(55008, _("RANDFILE environment variable not set"))
#define MSG_SEC_CANTOPENCERTFILE_SS      _MESSAGE(55009, _("Cannot open Cert_file "SFQ": "SFN"!!\n"))
#define MSG_SEC_FAILEDVERIFYOWNCERT      _MESSAGE(55010, _("failed verify own certificate\n"))
#define MSG_SEC_CANTOPENKEYFILE_SS       _MESSAGE(55011, _("Cannot open key file "SFQ": "SFN"!\n"))
#define MSG_SEC_INITPACKBUFFERFAILED     _MESSAGE(55012, _("init_packbuffer failed\n"))
#define MSG_SEC_PACKANNOUNCEFAILED       _MESSAGE(55013, _("sec_pack_announce failed\n"))
#define MSG_SEC_UNPACKANNOUNCEFAILED     _MESSAGE(55014, _("sec_unpack_announce failed\n"))
#define MSG_SEC_RESPONSEFAILED_SISIS      _MESSAGE(55015, _("failed get sec_response from ("SFN":%d:"SFN":%d):"SFN"\n"))
#define MSG_SEC_MASTERERROR              _MESSAGE(55016, _("master reports error: "))
#define MSG_SEC_UNEXPECTEDTAG            _MESSAGE(55017, _("received unexpected TAG from master\n"))
#define MSG_SEC_PACKRESPONSEFAILED       _MESSAGE(55018, _("sec_pack_response failed"))
#define MSG_SEC_UNPACKRESPONSEFAILED     _MESSAGE(55019, _("sec_unpack_response failed"))
#define MSG_SEC_PACKMSGFAILED            _MESSAGE(55020, _("sec_pack_message failed"))
#define MSG_SEC_UNPACKMSGFAILED          _MESSAGE(55021, _("sec_unpack_message failed"))
#define MSG_SEC_MASTERCERTREADFAILED     _MESSAGE(55022, _("failed read master certificate\n"))
#define MSG_SEC_CLIENTCERTREADFAILED     _MESSAGE(55023, _("failed read client certificate\n"))
#define MSG_SEC_CLIENTCERTVERIFYFAILED   _MESSAGE(55024, _("failed verify client certificate\n"))
#define MSG_SEC_MASTERGETPUBKEYFAILED    _MESSAGE(55025, _("cannot extract public key from master certificate\n"))
#define MSG_SEC_MASTERBADCHALLENGE       _MESSAGE(55026, _("challenge from master is bad\n"))
#define MSG_SEC_CLIENTGETPUBKEYFAILED    _MESSAGE(55027, _("cant get client public key\n"))
#define MSG_SEC_ENCRYPTCHALLENGEFAILED   _MESSAGE(55028, _("failed encrypt challenge MAC\n"))
#define MSG_SEC_ENCRYPTMACFAILED         _MESSAGE(55029, _("failed encrypt MAC\n"))
#define MSG_SEC_ENCRYPTMSGFAILED         _MESSAGE(55030, _("failed encrypt message\n"))
#define MSG_SEC_SEALINITFAILED           _MESSAGE(55031, _("EVP_SealInit failed\n"))
#define MSG_SEC_ENCRYPTKEYFAILED         _MESSAGE(55032, _("failed encrypt keys\n"))
#define MSG_SEC_INSERTCONNECTIONFAILED   _MESSAGE(55033, _("failed insert Connection to list\n"))
#define MSG_SEC_CONNECTIONNOENTRY        _MESSAGE(55034, _("no list entry for connection\n"))
#define MSG_SEC_CONNECTIONNOENTRY_SSI    _MESSAGE(55035, _("no list entry for connection ("SFN":"SFN":%d)!\n"))
#define MSG_SEC_SENDRESPONSEFAILED_SIS   _MESSAGE(55036, _("Send response to ("SFN":%d:"SFN") failed\n"))
#define MSG_SEC_RESPONSEFAILED_SIS       _MESSAGE(55037, _("sec_respond_announce to ("SFN":%d:"SFN") failed\n"))
#define MSG_SEC_SUMMONSESFAILED_SIS      _MESSAGE(55038, _("Failed send summonses for announce to ("SFN":%d:"SFN")\n"))
#define MSG_SEC_ANNOUNCEFAILED           _MESSAGE(55039, _("Anounce failed\n"))
#define MSG_SEC_SETSECDATAFAILED         _MESSAGE(55040, _("failed to set security data\n"))
#define MSG_SEC_MSGENCFAILED             _MESSAGE(55041, _("failed encrypt message\n"))
#define MSG_SEC_CONNIDSETFAILED          _MESSAGE(55042, _("failed set connection ID\n"))
#define MSG_SEC_CONNIDGETFAILED          _MESSAGE(55043, _("failed get connection ID\n"))
#define MSG_SEC_HANDLEANNOUNCEFAILED_SSI _MESSAGE(55044, _("failed handle announce for ("SFN":"SFN":%d)\n"))
#define MSG_SEC_MSGDECFAILED_SSI         _MESSAGE(55045, _("failed decrypt message ("SFN":"SFN":%d)\n"))
#define MSG_SEC_MSGDECFAILED             _MESSAGE(55046, _("failed decrypt message\n"))
#define MSG_SEC_HANDLEDECERRFAILED       _MESSAGE(55047, _("failed handle decrypt error\n"))
#define MSG_SEC_CERTNOTYETVALID          _MESSAGE(55048, _("certificate not yet valid\n"))
#define MSG_SEC_CERTEXPIRED              _MESSAGE(55049, _("certificate has expired\n"))
#define MSG_SEC_I2DX509FAILED            _MESSAGE(55050, _("i2d_x509 failed\n"))
#define MSG_SEC_EVPOPENINITFAILED        _MESSAGE(55051, _("EVP_OpenInit failed decrypt keys\n"))
#define MSG_SEC_EVPOPENUPDATEFAILED      _MESSAGE(55052, _("EVP_OpenUpdate failed decrypt keys\n"))
#define MSG_SEC_RMRECONNECTFAILED_S      _MESSAGE(55053, _("failed remove reconnect file "SFQ"\n"))
#define MSG_SEC_ENCRECONNECTFAILED       _MESSAGE(55054, _("failed encrypt reconnect data\n"))
#define MSG_SEC_DECRECONNECTFAILED       _MESSAGE(55055, _("failed decrypt reconnect data\n"))
#define MSG_SEC_UNPACKRECONNECTFAILED    _MESSAGE(55056, _("failed unpack reconnect data\n"))
#define MSG_SEC_PACKRECONNECTFAILED      _MESSAGE(55057, _("failed pack reconnect data\n"))
#define MSG_SEC_INITPBFAILED             _MESSAGE(55058, _("failed init_packbuffer_from_buffer\n"))
#define MSG_SEC_NOCONN_I                 _MESSAGE(55059, _("no connection %d\n"))
#define MSG_SEC_CONNIDSHOULDBE_II        _MESSAGE(55060, _("connection id is %d, but should be %d!\n"))
#define MSG_SEC_DECMACFAILED             _MESSAGE(55061, _("failed decrypt MAC\n"))
#define MSG_SEC_CANTREAD                 _MESSAGE(55062, _("can't read from file\n"))
#define MSG_SEC_CANTWRITE_SS             _MESSAGE(55063, _("can't open file "SFQ": "SFN"\n"))
#define MSG_SEC_SENDERRFAILED            _MESSAGE(55064, _("failed send error message\n"))
#define MSG_SEC_PACKCONNIDFAILED         _MESSAGE(55065, _("failed pack ConnID\n"))
#define MSG_SEC_UNPACKCONNIDFAILED       _MESSAGE(55066, _("failed unpack ConnID\n"))


#endif /* __MSG_SEC_H */
