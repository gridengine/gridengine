#ifndef __MSG_KRB_H
#define __MSG_KRB_H
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
** krb/src/krb_lib.c 
*/
#define MSG_KRB_KRB5INITCONTEXTFAILEDX_S                 _MESSAGE(53000, _("krb5_init_context failed - %s\n"))
#define MSG_KRB_KRB5CCREGISTERFAILEDX_S                  _MESSAGE(53001, _("krb5_cc_register failed - %s\n"))
#define MSG_KRB_COULDNOTGETREALMFORQMASTERHOSTXY_SS      _MESSAGE(53002, _("could not get realm for qmaster host %s - %s\n"))
#define MSG_KRB_COULDNOTFREEREALMLISTX_S                 _MESSAGE(53003, _("could not free realmlist - %s\n"))
#define MSG_KRB_COULDNOTGETDEFAULTREALMX_S               _MESSAGE(53004, _("could not get default realm - %s\n"))
#define MSG_KRB_COULDNOTDETERMINEHOSTSORREALFORQMASTER   _MESSAGE(53005, _("could not determine host or realm for qmaster\n"))
#define MSG_KRB_KRB5PARSENAMEFAILEDX_S                   _MESSAGE(53006, _("krb5_parse_name failed - %s\n"))
#define MSG_KRB_KRB5PARSENAMEFAILED                      _MESSAGE(53007, _("krb5_parse_name failed\n"))
#define MSG_KRB_GETHOSTNAMEFAILED                        _MESSAGE(53008, _("gethostname failed\n"))
#define MSG_KRB_COULDNOTRESOLVEKEYTABX_S                 _MESSAGE(53009, _("could not resolve keytab - %s\n"))
#define MSG_KRB_KRB5SNAMETOPRINCIPALFAILEDX_S            _MESSAGE(53010, _("krb5_sname_to_principal failed - %s\n"))
#define MSG_KRB_KRB5SNAMETOPRINCIPAL                     _MESSAGE(53011, _("krb5_sname_to_principal\n"))
#define MSG_KRB_XCOULDNOTGETDAEMONKEYY_SS                _MESSAGE(53012, _("%s could not get daemon key - %s"))
#define MSB_KRB_CONNECTIONLISTCOULDNOTBECREATED          _MESSAGE(53013, _("connection list could not be created\n"))
#define MSG_KRB_XCOULDNOTGETSGETICKETUSINGKEYTABY_SS     _MESSAGE(53014, _("%s could not get SGE ticket using keytab - %s"))
#define MSG_KRB_KRBGETNEWAUTHCONFAILED                   _MESSAGE(53016, _("krb_get_new_auth_con failed\n"))
#define MSG_KRB_KRB5CCDEFAULTFAILEDX_S                   _MESSAGE(53017, _("krb5_cc_default failed - %s\n"))
#define MSG_KRB_COULDNOTGETCLIENTCREDENTIALS             _MESSAGE(53018, _("Could not get client credentials\n"))
#define MSG_KRB_KRB5CCGETPRINCIPALFAILEDX_S              _MESSAGE(53019, _("krb5_cc_get_principal failed - %s\n"))
#define MSG_KRB_COULDNOTGETCLIENTPRINCIPAL               _MESSAGE(53020, _("Could not get client principal\n"))
#define MSG_KRB_INITPACKBUFFERFAILED                     _MESSAGE(53021, _("init_packbuffer failed\n"))
#define MSG_KRB_CALLERDIDNOTCALLKRBINIT                  _MESSAGE(53022, _("caller did not call krb_init\n"))
#define MSG_KRB_KRB5TIMEOFDAYFAILEDX_S                   _MESSAGE(53023, _("krb5_timeofday failed - %s"))
#define MSG_KRB_NOCLIENTENTRYFOR_SSI                     _MESSAGE(53024, _("No client entry for <%s,%s,%d>\n"))
#define MSG_KRB_KRBGETNEWAUTHCONFAILUREX_S               _MESSAGE(53025, _("krb_get_new_auth_con failure - %s"))
#define MSG_KRB_COULDNOTGETNEWAUTHCONTEXT                _MESSAGE(53026, _("could not get new auth context\n"))
#define MSG_KRB_FAILEDCREATINGAP_REQFORWXZY_SSIS         _MESSAGE(53027, _("failed creating AP_REQ for <%s,%s,%d> - %s\n"))
#define MSG_KRB_COULDNOTCREATEAUTHENTICATIONINFO         _MESSAGE(53028, _("could not create authentication info\n"))
#define MSG_KRB_KRB5AUTHCONSETADDRSFIALEDFORWXYZ_SSIS    _MESSAGE(53029, _("krb5_auth_con_setaddrs failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_COULDNOTSETPORTSINAUTHCONTEXT            _MESSAGE(53030, _("could not set ports in auth_context\n"))
#define MSG_KRB_COULDNOTSETADDRESSESINAUTHCONTEXT        _MESSAGE(53031, _("could not set addresses in auth_context\n"))
#define MSG_KRB_KRB5GENPORTADDRFAILEDFORWXYZ_SSIS        _MESSAGE(53032, _("krb5_gen_portaddr failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_COULDNOTGENPORTADDR                      _MESSAGE(53033, _("could not gen portaddr\n"))
#define MSG_KRB_KRB5GENREPLAYNAMEFAILEDFORWXYZ_SSIS      _MESSAGE(53034, _("krb5_gen_replay_name failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_COULDNOTGENREPLAYNAME                    _MESSAGE(53035, _("could not gen replay name\n"))
#define MSG_KRB_KRB5GETSERVERRCACHEFAILEDFORWXYZ_SSIS    _MESSAGE(53036, _("krb5_get_server_rcache failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_COULDNOTGETREPLAYCACHE                   _MESSAGE(53037, _("could not get replay cache\n"))
#define MSG_KRB_FAILEDENCRYPTINGMSGFORWXYZ_SSIS          _MESSAGE(53038, _("failed encrypting msg for <%s,%s,%d> - %s\n"))
#define MSG_KRB_FAILEDENCRYPTINGMESSAGE                  _MESSAGE(53039, _("failed encrypting message\n"))
#define MSG_KRB_COULDNOTGETFORWARDABLETGTFORWXYZ_SSIS    _MESSAGE(53041, _("could not get forwardable TGT for <%s,%s,%d> - %s\n"))
#define MSG_KRB_UNABLETOFORWARDTGT                       _MESSAGE(53042, _("unable to forward TGT\n"))
#define MSG_KRB_AUTHENTICATIONFAILURE                    _MESSAGE(53043, _("Authentication failure"))
#define MSG_KRB_DIDNOTCALLKRBINIT                        _MESSAGE(53044, _("did not call krb_init\n"))
#define MSG_KRB_INVALIDTAGAUTHFAILUREMSGRECEIVEDWXYZ_SSI _MESSAGE(53045, _("Invalid TAG_AUTH_FAILURE msg recvd from <%s,%s,%d>\n"))
#define MSG_KRB_AUTHENTICATIONTOQMASTERFAILED            _MESSAGE(53046, _("Authentication to qmaster failed"))
#define MSG_KRB_AUTHENTICATIONFAILED                     _MESSAGE(53047, _("Authentication failed\n"))
#define MSG_KRB_INVALIDMESSAGEUNPACKFAILURE              _MESSAGE(53048, _("Invalid message - unpack failure"))
#define MSG_KRB_FAILEDSENDINGAUTH_FAILUREMESSAGEX_S      _MESSAGE(53049, _("failed sending AUTH_FAILURE message - %s\n"))
#define MSG_KRB_INVALIDMESSAGEPACKINGERROR               _MESSAGE(53050, _("Invalid message - packing error\n"))
#define MSG_KRB_INVALIDMESSAGENOAP_REQ                   _MESSAGE(53051, _("Invalid message - no AP_REQ" ))
#define MSG_KRB_XCOULDNOTCREATEREPLAYCACHEY_SS           _MESSAGE(53052, _("%s could not create replay cache - %s"))
#define MSG_KRB_FAILEDCREATEOFCLIENT                     _MESSAGE(53053, _("failed create of client"))
#define MSG_KRB_APPENDELEMFAILUREX_I                     _MESSAGE(53055, _("lAppendElem failure - %d"))
#define MSG_KRB_CLIENTWXYFAILEDAUTHENTICATIONZ_SSIS      _MESSAGE(53057, _("client <%s,%s,%d> failed authentication - %s\n"))
#define MSG_KRB_INVALIDMESSAGEHASANAP_REQ                _MESSAGE(53058, _("Invalid message has an AP_REQ"))
#define MSG_KRB_INVALIDMESSAGERECEIVED                   _MESSAGE(53059, _("Invalid message received\n"))
#define MSG_KRB_GETHOSTBYNAMEFAILED                      _MESSAGE(53060, _("gethostbyname failed\n"))
#define MSG_KRB_KRB5AUTHCONSETADDRSFAILEDFORWXYZ_SSIS    _MESSAGE(53061, _("krb5_auth_con_setaddrs failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_UNABLETODECRYPTFORWARDEDTGTFORCLIENTWXYZ_SSIS  _MESSAGE(53062, _("unable to decrypt forwarded TGT for client <%s,%s,%d> - %s"))
#define MSG_KRB_FAILEDADDINGTGTWTOCLIENTSTGTLISTFORXYZ_ISSI  _MESSAGE(53063, _("Failed adding TGT %d to client's TGT list for <%s,%s,%d>"))
#define MSG_KRB_ILLOGICALFORWARDABLETGTRECEIVEDFROMXYZ_SSI  _MESSAGE(53064, _("illogical forwardable TGT received from <%s,%s,%d>"))

#define MSG_KRB_FAILEDDECRYPTINGMSGFORWXYZ_SSIS             _MESSAGE(53065, _("failed decrypting msg for <%s,%s,%d> - %s\n"))
#define MSG_KRB_FAILEDSENDINGAUTH_FAILUREMESSAGE_S       _MESSAGE(53066, _("failed sending AUTH_FAILURE message - %s\n"))
#define MSG_KRB_KRB5AUTHCONGETAUTHENTICATORFAILEDFORWXYZ_SSIS  _MESSAGE(53067, _("krb5_auth_con_getauthenticator failed for <%s,%s,%d> - %s\n"))
#define MSG_KRB_TGTCREDSHOSTORCOMPROCISNULL              _MESSAGE(53068, _("tgt_creds, host, or comproc is NULL"))
#define MSG_KRB_NOCLIENTENTRYFORXYZ_SSI                  _MESSAGE(53069, _("No client entry for <%s,%s,%d>\n"))
#define MSG_KRB_NOTGTFOUNDFORWXYWITHIDZ_SSID            _MESSAGE(53070, _("No TGT found for <%s,%s,%d> with ID %ld\n"))
#define MSG_KRB_NOTCLIENTENTRYFORXYZUNABLETOSTORETGT_SSI _MESSAGE(53071, _("No client entry for <%s,%s,%d> - unable to store TGT\n"))
#define MSG_KRB_FAILEDSTORINGFORWARDEDTGTFORUIDXJOBYZ_IIS  _MESSAGE(53072, _("failed storing forwarded TGT for uid %d, job %d - %s\n" ))
#define MSG_KRB_FAILEDDELETINGTGTFORJOBXY_IS             _MESSAGE(53073, _("failed deleting TGT for job %d - %s\n"))




/* 
** krb/src/krb_util.c
*/
#define MSG_KRB_COULDNOTDECRYPTTGTFORJOBXY_DS   _MESSAGE(53074, _("could not decrypt TGT for job "U32CFormat" - %s" ))
#define MSG_KRB_COULDNOTRENEWTGTFORJOBXY_DS     _MESSAGE(53075, _("could not renew TGT for job "U32CFormat" - %s"))
#define MSG_KRB_COULDNOTECRYPTTGTFORJOBXY_DS    _MESSAGE(53076, _("could not encrypt TGT for job "U32CFormat" - %s"))
#define MSG_KRB_COULDNOTSTORERENEWEDTGTFORXJOBY_SD _MESSAGE(53077, _("could not store renewed TGT for %s - job "U32CFormat"\n"))
#define MSG_KRB_COULDNOTGETUSERIDFORXY_SD       _MESSAGE(53078, _("could not get user ID for %s - job "U32CFormat"\n"))


#endif /* __MSG_KRB_H  */ 
