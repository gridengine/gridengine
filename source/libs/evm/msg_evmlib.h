#ifndef __MSG_EVMLIB_H
#define __MSG_EVMLIB_H
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
** sge_event_master.c
*/
#define MSG_EVE_REINITEVENTCLIENT_S   _MESSAGE(66000, _("reinitialization of "SFQ"\n"))
#define MSG_EVE_UNKNOWNEVCLIENT_US    _MESSAGE(66001, _("no event client known with id "U32CFormat" to %s\n"))
#define MSG_EVE_CLIENTREREGISTERED_SSSU _MESSAGE(66002, _("event client "SFQ" (%s/%s/"U32CFormat") reregistered - it will need a total update\n"))
#define MSG_EVE_REG_SUU               _MESSAGE(66003, _(SFQ" registers as event client with id "U32CFormat" event delivery interval "U32CFormat"\n"))
#define MSG_EVE_UNREG_SU              _MESSAGE(66004, _("event client "SFQ" with id "U32CFormat" deregistered"))
#define MSG_EVE_EVENTCLIENT           _MESSAGE(66005, _("event client"))

#define MSG_EVE_ILLEGALEVENTCLIENTID_S _MESSAGE(66006, _("illegal event client id "SFQ"\n"))
#define MSG_EVE_ILLEGALIDREGISTERED_U _MESSAGE(66007, _("illegal event client id "U32CFormat" for registration\n"))

#define MSG_EVE_INVALIDSUBSCRIPTION   _MESSAGE(66008, _("invalid subscription information\n"))
#define MSG_EVE_INVALIDINTERVAL_U     _MESSAGE(66009, _("invalid event interval "U32CFormat"\n"))

#define MSG_EVE_TOTALUPDATENOTHANDLINGEVENT_I _MESSAGE(66010, _("event number %d is not handled by sge_total_update_event\n"))

#define MSG_COM_ACKTIMEOUT4EV_ISIS    _MESSAGE(66011, _("acknowledge timeout after %d seconds for event client ("SFN":%d) on host "SFQ))
#define MSG_COM_NOSHUTDOWNPERMS       _MESSAGE(66012, _("shutdown requires manager privileges\n"))

#endif /* __MSG_EVMLIB_H */
