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

#ifndef _MSG_JAPI_H
#define	_MSG_JAPI_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "basis_types.h"
   
#define MSG_JAPI_NO_EVENT_CLIENT   _MESSAGE(45501, _("The event client has not been started.\n"))
#define MSG_JAPI_EVENT_CLIENT_ALREADY_STARTED   _MESSAGE(45502, _("The event client has already been started.\n"))
#define MSG_JAPI_EC_GET_PROBLEM _MESSAGE(45503, _("Problem receiving events from qmaster.\n"))
#define MSG_JAPI_DISCONNECTED   _MESSAGE(45504, _("The qmaster has become unreachable.  Attempting to reconnect.\n"))
#define MSG_JAPI_RECONNECTED    _MESSAGE(45505, _("Reconnected to qmaster.\n"))
#define MSG_JAPI_QMASTER_DOWN   _MESSAGE(45506, _("The qmaster has gone down.  Waiting to reconnect.\n"))
#define MSG_JAPI_NO_HANDLE_S      _MESSAGE(45507, _("Unable to open a connection to the qmaster: "SFN"\n"))

#ifdef	__cplusplus
}
#endif

#endif	/* _MSG_JAPI_H */
