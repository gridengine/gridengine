#ifndef __MSG_GSS_H
#define __MSG_GSS_H
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
** gss/acquire_cred.c 
*/ 
#define MSG_GSS_ACQUIREX_USAGE_S        _("Usage: %s [-s service]\n")
#define MSG_GSS_ACQUIREX_s_OPT_USAGE    _("       -s service      Get credentials for service\n")

/* 
** gss/sge_gsslib.c 
*/ 
#define MSG_GSS_APIERRORXY_SS          _("GSS-API error %s: %s\n")
#define MSG_GSS_CONTEXTFLAG_GSS_C_DELEG_FLAG       _("context flag: GSS_C_DELEG_FLAG\n"             )
#define MSG_GSS_CONTEXTFLAG_GSS_C_MUTUAL_FLAG      _("context flag: GSS_C_MUTUAL_FLAG\n")
#define MSG_GSS_CONTEXTFLAG_GSS_C_REPLAY_FLAG      _("context flag: GSS_C_REPLAY_FLAG\n")
#define MSG_GSS_CONTEXTFLAG_GSS_C_SEQUENCE_FLAG    _("context flag: GSS_C_SEQUENCE_FLAG\n")
#define MSG_GSS_CONTEXTFLAG_GSS_C_CONF_FLAG        _("context flag: GSS_C_CONF_FLAG \n")
#define MSG_GSS_CONTEXTFLAG_GSS_C_INTEG_FLAG       _("context flag: GSS_C_INTEG_FLAG \n")
#define MSG_GSS_ACCEPTSECCONTEXTREQUIRESTOKENTOBESENTBACK  _("accept_sec_context requires token to be sent back\n" )
#define MSG_GSS_CLIENTNAMEXDOESNOTMATCHUNAMEY_SS   _("client name \"%.*s\" does not match user name \"%s\"\n")
#define MSG_GSS_DISPLAYSTATUS_PARSINGNAME          _("parsing name")
#define MSG_GSS_DISPLAYSTATUS_INITIALIZINGCONTEXT  _("initializing context")
#define MSG_GSS_DISPLAYSTATUS_ACQUIRINGCREDENTIALS _("acquiring credentials")
#define MSG_GSS_DISPLAYSTATUS_IMPORTINGNAME        _("importing name")
#define MSG_GSS_DISPLAYSTATUS_REGISTERINGIDENTITY  _("registering identity")
#define MSG_GSS_DISPLAYSTATUS_ACCEPTINGCONTEXT     _("accepting context")
#define MSG_GSS_DISPLAYSTATUS_DISPLAYINGNAME       _("displaying name")
#define MSG_GSS_DISPLAYSTATUS_COMPARINGNAME       _("comparing name")
#define MSG_GSS_DISPLAYSTATUS_RELEASINGNAME        _("releasing name")
#define MSG_GSS_DISPLAYSTATUS_GETTINGKRB5CONTEXT   _("getting krb5 context")
#define MSG_GSS_DISPLAYSTATUS_GSSDCESETCREDCONTEXTOWNERSHIP _("gssdce_set_cred_context_ownership")
#define MSG_GSS_DISPLAYSTATUS_GSSDCECREDTOLOGINCONTEXT      _("gssdce_cred_to_login_context")
#define MSG_GSS_PRINTERROR_CREDENTIALBUFFERLENGTHISZERO  _("credential buffer length is zero\n")
#define MSG_GSS_PRINTERROR_CREDENTIALDUMP          _("Credential dump\n")
#define MSG_GSS_PRINTERROR_COULDNOTDISABLEDELEGATIONX_S  _("Could not disable delegation - %s\n")
#define MSG_GSS_PRINTERROR_COULDNOTCERTIFYIDENTITYX_S    _("Could not certify identity - %s\n")
#define MSG_GSS_PRINTERROR_COULDNOTSETUPLOGINCONTEXTX_S  _("Could not set up login context - %s\n")
#define MSG_GSS_PRINTERROR_NEWKRB5CCNAMEISX_S            _("New KRB5CCNAME=%s\n")
#define MSG_GSS_PRINTERROR_KRB5CCNAMENOTFOUND            _("KRB5CCNAME not found\n")
#define MSG_GSS_PRINTERROR_GETTINGKRB5CONTEXT            _("getting krb5 context")
#define MSG_GSS_PRINTERROR_KRB5PARSENAMERETURNEDX_I      _("krb5_parse_name returned %d\n")
#define MSG_GSS_PRINTERROR_KRB5CCDEFAULTRETURNEDX_I      _("krb5_cc_default returned %d\n")
#define MSG_GSS_PRINTERROR_KRB5CCINITIALIZERETURNEDX_I   _("krb5_cc_initialize returned %d\n")
#define MSG_GSS_PRINTERROR_COPYINGDELEGATEDCREDSTOCC     _("copying delegated creds to ccache")



/* 
** gss/delete_cred.c
*/ 
#define MSG_GSS_PERROR_UNLINK             _("unlink")
#define MSG_GSS_PERROR_GETTINGFILESTATUS  _("getting file status")
#define MSG_GSS_PERROR_GETTINGLINK        _("getting link"   )
#define MSG_GSS_DELETECREDKRB5CCNAMEENVVARHASINVALIDFORMAT  _("delete_cred: KRB5CCNAME environment variable has an invalid format\n")
#define MSG_GSS_DELETECREDKRB5CCNAMEENVVARNOTSET  _("delete_cred: KRB5CCNAME environment variable not set\n" )
#define MSG_GSS_DELETECREDCOULDNOTDELCREDENTIALS   _("delete_cred: could not delete credentials\n")


/* 
** gss/get_cred.c
*/ 
#define MSG_GSS_WRITINGXBYTESTOSTDOUT_I   _("writing %d bytes to stdout\n")
#define MSG_GSS_GETCREDNOCREDENTIALSFOUND _("get_cred: no credentials found")

/* 
** gss/put_cred.c
*/ 
#define MSG_GSS_PUTCRED_USAGE       _( \
"Usage: %s [-u user] [-b user] [-s service] [-c cmd]\n" \
"   -b user     Become user before storing credentials and/or executing cmd\n" \
"   -o user     Change ownership of credentials cache files to user\n" \
"   -u user     Authenticate user\n" \
"   -s service  Get credentials for service\n" \
"   -c cmd      Run command as user\n" \
"   -e cmd      Execute command as user\n" )

#define MSG_GSS_PUTCRED_ARGUMENTS   _("Arguments: ")
#define MSG_GSS_FAILEDREADINGCREDENTIALLENGTHFROMSTDIN   _("failed reading credential length from stdin\n")
#define MSG_GSS_COULDNOTALLOCATEXBYTESFORCREDENTIALS_I   _("could not allocate %d bytes for credentials\n")
#define MSG_GSS_FAILEDREADINGCREDENTIALFROMSTDIN         _("failed reading credential from stdin\n")
#define MSG_GSS_COULDNOTGETUSERIDFORXY_SS                _("could not get user ID for %s - %s\n")
#define MSG_GSS_COULDNOTCHANGEOWNERSHIPOFCREDENTIALSCACHETOXINVALIDKRB5CCNAME_S _("could not change ownership of credentials cache to %s - invalid KRB5CCNAME\n" )
#define MSG_GSS_COULDNOTCHANGEOWNERSHIPOFXTOYZ_SSS       _("could not change ownership of %s to %s - %s\n")
#define MSG_GSS_PERROR_SETGID                            _("setgid")
#define MSG_GSS_PERROR_SETUID                            _("setuid")
#define MSG_GSS_PERROR_EXECFAILED                        _("exec failed")
#define MSG_GSS_COULDNOTLINKXTODCECREDENTIALSCACHEFILEYZ_SSS   _("Could not link %s to DCE credentials cache file %s - %s\n")
#define MSG_GSS_COULDNOTLINKXTODCECREDENTIALSCACHEFILEYINVALIDKRB5CCNAMEENVIRONMENTVARIABLEFORMAT_SS  _("Could not link %s to DCE credentials cache file %s - invalid KRB5CCNAME environment variable format\n")


/* 
** gss/write_cred.c
*/ 
#define MSG_GSS_WRITECRED_USAGE_S         _("Usage: %s file\n")
#define MSG_GSS_COULDNOTOPENXY_SS         _("could not open %s - %s\n")
#define MSG_GSS_READINGCREDENTIALLENGTH   _("reading credential length\n")
#define MSG_GSS_READLENGTHOFX_I           _("read length of %d\n")
#define MSG_GSS_READXBYTES_I              _("read %d bytes\n")
#define MSG_GSS_WROTEXBYTES_I             _("wrote %d bytes\n")
#define MSG_GSS_WRITECREDNOCREDENTIALSFOUND  _("write_cred: no credentials found")
#endif /* __MSG_GSS_H */
