#ifndef _MSG_JUTI_H
#define	_MSG_JUTI_H
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

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef WINDOWS
#  include "basis_types.h"
#else
   /* save string format non-quoted */
#  define SFN  "%-.100s"
#  define _(x)              (x)
#  define _MESSAGE(x,y)     (y)
#endif

#define MSG_JUTI_PAM_NOT_AVAILABLE    _MESSAGE(213001, _("pam not available"))
#define MSG_JUTI_USER_UNKNOWN_S       _MESSAGE(213002, _("user "SFN" unknown")) 
#define MSG_JUTI_NO_SHADOW_ENTRY_S    _MESSAGE(213003, _("user "SFN" has not shadow entry"))
#define MSG_JUTI_CRYPT_FAILED_S       _MESSAGE(213004, _("crypt failed: "SFN))
#define MSG_JUTI_INVALID_PASSWORD     _MESSAGE(213005, _("invalid password"))
#define MSG_JUTI_PAM_ERROR_S          _MESSAGE(213006, _("PAM error: "SFN)) 

#define MSG_AUTUSER_INVAILD_ARG_COUNT     _MESSAGE(213101, _("invalid number of arguments"))
#define MSG_AUTUSER_MISSING_PAM_SERVICE   _MESSAGE(213102, _("missing pam service name"))
#define MSG_AUTUSER_UNKNOWN_PARAM_S       _MESSAGE(213103, _("Unknown param "SFN))
#define MSG_AUTUSER_UNKNOWN_AUTH_METHOD_S _MESSAGE(213104, _("Unknown <auth_method> "SFN))
#define MSG_AUTHUSER_NO_PW_ENTRY_SS       _MESSAGE(213105, _("password: can not get password entry of user "SFN": "SFN))
#define MSG_AUTHUSER_ERROR                _MESSAGE(213106, _("Error: "))
#define MSG_AUTHUSER_WRONG_USER_OR_PASSWORD   _MESSAGE(213106, _("Wrong user or password"))
#ifdef	__cplusplus
}
#endif

#endif	/* _MSG_JUTI_H */
