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
#define MSG_KRB_KRB5INITCONTEXTFAILEDX_S                 _("krb5_init_context failed - %s\n")
#define MSG_KRB_KRB5CCREGISTERFAILEDX_S                  _("krb5_cc_register failed - %s\n")
#define MSG_KRB_COULDNOTGETREALMFORQMASTERHOSTXY_SS      _("could not get realm for qmaster host %s - %s\n")
#define MSG_KRB_COULDNOTFREEREALMLISTX_S                 _("could not free realmlist - %s\n")
#define MSG_KRB_COULDNOTGETDEFAULTREALMX_S               _("could not get default realm - %s\n")
#define MSG_KRB_COULDNOTDETERMINEHOSTSORREALFORQMASTER   _("could not determine host or realm for qmaster\n")
#define MSG_KRB_KRB5PARSENAMEFAILEDX_S                   _("krb5_parse_name failed - %s\n")
#define MSG_KRB_KRB5PARSENAMEFAILED                      _("krb5_parse_name failed\n")
#define MSG_KRB_GETHOSTNAMEFAILED                        _("gethostname failed\n")
#define MSG_KRB_COULDNOTRESOLVEKEYTABX_S                 _("could not resolve keytab - %s\n")
#define MSG_KRB_KRB5SNAMETOPRINCIPALFAILEDX_S            _("krb5_sname_to_principal failed - %s\n")
#define MSG_KRB_KRB5SNAMETOPRINCIPAL                     _("krb5_sname_to_principal\n")
#define MSG_KRB_XCOULDNOTGETDAEMONKEYY_SS                _("%s could not get daemon key - %s")
#define MSB_KRB_CONNECTIONLISTCOULDNOTBECREATED          _("connection list could not be created\n")
#define MSG_KRB_XCOULDNOTGETSGETICKETUSINGKEYTABY_SS     _("%s could not get SGE ticket using keytab - %s")
#define MSG_KRB_KRBGETNEWAUTHCONFAILUREX_S               _("krb_get_new_auth_con failure - %s")
#define MSG_KRB_KRBGETNEWAUTHCONFAILED                   _("krb_get_new_auth_con failed\n")
#define MSG_KRB_KRB5CCDEFAULTFAILEDX_S                   _("krb5_cc_default failed - %s\n")
#define MSG_KRB_COULDNOTGETCLIENTCREDENTIALS             _("Could not get client credentials\n")
#define MSG_KRB_KRB5CCGETPRINCIPALFAILEDX_S              _("krb5_cc_get_principal failed - %s\n")
#define MSG_KRB_COULDNOTGETCLIENTPRINCIPAL               _("Could not get client principal\n")
#define MSG_KRB_INITPACKBUFFERFAILED                     _("init_packbuffer failed\n")
#define MSG_KRB_CALLERDIDNOTCALLKRBINIT                  _("caller did not call krb_init\n")
#define MSG_KRB_KRB5TIMEOFDAYFAILEDX_S                   _("krb5_timeofday failed - %s")
#define MSG_KRB_NOCLIENTENTRYFOR_SSI                     _("No client entry for <%s,%s,%d>\n")
#define MSG_KRB_KRBGETNEWAUTHCONFAILUREX_S               _("krb_get_new_auth_con failure - %s")
#define MSG_KRB_COULDNOTGETNEWAUTHCONTEXT                _("could not get new auth context\n")
#define MSG_KRB_FAILEDCREATINGAP_REQFORWXZY_SSIS         _("failed creating AP_REQ for <%s,%s,%d> - %s\n")
#define MSG_KRB_COULDNOTCREATEAUTHENTICATIONINFO         _("could not create authentication info\n")
#define MSG_KRB_KRB5AUTHCONSETADDRSFIALEDFORWXYZ_SSIS    _("krb5_auth_con_setaddrs failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_COULDNOTSETPORTSINAUTHCONTEXT            _("could not set ports in auth_context\n")
#define MSG_KRB_COULDNOTSETADDRESSESINAUTHCONTEXT        _("could not set addresses in auth_context\n")
#define MSG_KRB_KRB5GENPORTADDRFAILEDFORWXYZ_SSIS        _("krb5_gen_portaddr failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_COULDNOTGENPORTADDR                      _("could not gen portaddr\n")
#define MSG_KRB_KRB5GENREPLAYNAMEFAILEDFORWXYZ_SSIS      _("krb5_gen_replay_name failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_COULDNOTGENREPLAYNAME                    _("could not gen replay name\n")
#define MSG_KRB_KRB5GETSERVERRCACHEFAILEDFORWXYZ_SSIS    _("krb5_get_server_rcache failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_COULDNOTGETREPLAYCACHE                   _("could not get replay cache\n")
#define MSG_KRB_FAILEDENCRYPTINGMSGFORWXYZ_SSIS          _("failed encrypting msg for <%s,%s,%d> - %s\n")
#define MSG_KRB_FAILEDENCRYPTINGMESSAGE                  _("failed encrypting message\n")
#define MSG_KRB_KRB5SNAMETOPRINCIPALFAILEDX_S            _("krb5_sname_to_principal failed - %s\n")
#define MSG_KRB_COULDNOTGETFORWARDABLETGTFORWXYZ_SSIS    _("could not get forwardable TGT for <%s,%s,%d> - %s\n")
#define MSG_KRB_UNABLETOFORWARDTGT                       _("unable to forward TGT\n")
#define MSG_KRB_AUTHENTICATIONFAILURE                    _("Authentication failure")
#define MSG_KRB_DIDNOTCALLKRBINIT                        _("did not call krb_init\n")
#define MSG_KRB_INVALIDTAGAUTHFAILUREMSGRECEIVEDWXYZ_SSI _("Invalid TAG_AUTH_FAILURE msg recvd from <%s,%s,%d>\n")
#define MSG_KRB_AUTHENTICATIONTOQMASTERFAILED            _("Authentication to qmaster failed")
#define MSG_KRB_AUTHENTICATIONFAILED                     _("Authentication failed\n")
#define MSG_KRB_INVALIDMESSAGEUNPACKFAILURE              _("Invalid message - unpack failure")
#define MSG_KRB_FAILEDSENDINGAUTH_FAILUREMESSAGEX_S      _("failed sending AUTH_FAILURE message - %s\n")
#define MSG_KRB_INVALIDMESSAGEPACKINGERROR               _("Invalid message - packing error\n")
#define MSG_KRB_INVALIDMESSAGENOAP_REQ                   _("Invalid message - no AP_REQ" )
#define MSG_KRB_XCOULDNOTCREATEREPLAYCACHEY_SS           _("%s could not create replay cache - %s")
#define MSG_KRB_FAILEDCREATEOFCLIENT                     _("failed create of client")
#define MSG_KRB_KRBGETNEWAUTHCONFAILUREX_S               _("krb_get_new_auth_con failure - %s")
#define MSG_KRB_APPENDELEMFAILUREX_I                     _("lAppendElem failure - %d")
#define MSG_KRB_KRBGETNEWAUTHCONFAILUREX_S               _("krb_get_new_auth_con failure - %s")
#define MSG_KRB_CLIENTWXYFAILEDAUTHENTICATIONZ_SSIS      _("client <%s,%s,%d> failed authentication - %s\n")
#define MSG_KRB_INVALIDMESSAGEHASANAP_REQ                _("Invalid message has an AP_REQ")
#define MSG_KRB_INVALIDMESSAGERECEIVED                   _("Invalid message received\n")
#define MSG_KRB_GETHOSTBYNAMEFAILED                      _("gethostbyname failed\n")
#define MSG_KRB_KRB5AUTHCONSETADDRSFAILEDFORWXYZ_SSIS    _("krb5_auth_con_setaddrs failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_UNABLETODECRYPTFORWARDEDTGTFORCLIENTWXYZ_SSIS  _("unable to decrypt forwarded TGT for client <%s,%s,%d> - %s")
#define MSG_KRB_FAILEDADDINGTGTWTOCLIENTSTGTLISTFORXYZ_ISSI  _("Failed adding TGT %d to client's TGT list for <%s,%s,%d>")
#define MSG_KRB_ILLOGICALFORWARDABLETGTRECEIVEDFROMXYZ_SSI  _("illogical forwardable TGT received from <%s,%s,%d>")

#define MSG_KRB_FAILEDDECRYPTINGMSGFORWXYZ_SSIS             _("failed decrypting msg for <%s,%s,%d> - %s\n")
#define MSG_KRB_FAILEDSENDINGAUTH_FAILUREMESSAGE_S       _("failed sending AUTH_FAILURE message - %s\n")
#define MSG_KRB_KRB5AUTHCONGETAUTHENTICATORFAILEDFORWXYZ_SSIS  _("krb5_auth_con_getauthenticator failed for <%s,%s,%d> - %s\n")
#define MSG_KRB_TGTCREDSHOSTORCOMPROCISNULL              _("tgt_creds, host, or comproc is NULL")
#define MSG_KRB_NOCLIENTENTRYFORXYZ_SSI                  _("No client entry for <%s,%s,%d>\n")
#define MSG_KRB_NOTGTFOUNDFORWXYWITHIDZ_SSID            _("No TGT found for <%s,%s,%d> with ID %ld\n")
#define MSG_KRB_NOTCLIENTENTRYFORXYZUNABLETOSTORETGT_SSI _("No client entry for <%s,%s,%d> - unable to store TGT\n")
#define MSG_KRB_FAILEDSTORINGFORWARDEDTGTFORUIDXJOBYZ_IIS  _("failed storing forwarded TGT for uid %d, job %d - %s\n" )
#define MSG_KRB_FAILEDDELETINGTGTFORJOBXY_IS             _("failed deleting TGT for job %d - %s\n")




/* 
** krb/src/krb_util.c
*/
#define MSG_KRB_COULDNOTDECRYPTTGTFORJOBXY_DS   _("could not decrypt TGT for job "U32CFormat" - %s" )
#define MSG_KRB_COULDNOTRENEWTGTFORJOBXY_DS     _("could not renew TGT for job "U32CFormat" - %s")
#define MSG_KRB_COULDNOTECRYPTTGTFORJOBXY_DS    _("could not encrypt TGT for job "U32CFormat" - %s")
#define MSG_KRB_COULDNOTSTORERENEWEDTGTFORXJOBY_SD _("could not store renewed TGT for %s - job "U32CFormat"\n")
#define MSG_KRB_COULDNOTGETUSERIDFORXY_SD       _("could not get user ID for %s - job "U32CFormat"\n")


#endif /* __MSG_KRB_H  */ 
