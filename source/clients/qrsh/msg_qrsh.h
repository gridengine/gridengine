#ifndef __MSG_QRSH_H
#define __MSG_QRSH_H
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

#define MSG_QRSH_STARTER_CANNOTOPENFILE_SS   _("qrsh_starter: cannot open file %s: %s\n")
#define MSG_QRSH_STARTER_CANNOTCHANGEDIR_SS  _("qrsh_starter: cannot change to directory %s: %s\n")
#define MSG_QRSH_STARTER_MALLOCFAILED_S      _("qrsh_starter: malloc failed: %s\n")
#define MSG_QRSH_STARTER_CANNOTREADENV_S     _("qrsh_starter: cannot read environment variable %s\n")
#define MSG_QRSH_STARTER_CANNOTWRITEPID_S    _("qrsh_starter: cannot write pid file %s\n")
#define MSG_QRSH_STARTER_CANNOTFORKCHILD_S   _("qrsh_starter: cannot fork child process: %s\n")
#define MSG_QRSH_STARTER_CANNOTGETLOGIN_S    _("qrsh_starter: cannot get login name: %s\n")
#define MSG_QRSH_STARTER_CANNOTGETUSERINFO_S _("qrsh_starter: cannot get user information: %s\n")
#define MSG_QRSH_STARTER_CANNOTDETERMSHELL_S _("qrsh_starter: cannot determine login shell, using \"%s\"\n")
#define MSG_QRSH_STARTER_EMPTY_WRAPPER               _("qrsh_starter: environment variable QRSH_WRAPPER has no value\n")
#define MSG_QRSH_STARTER_EXECCHILDFAILED_S   _("qrsh_starter: executing child process %s failed: %s\n")
#define MSG_QRSH_STARTER_INVALIDCOMMAND      _("qrsh_starter: received invalid command to execute\n")

#endif /* __MSG_QRSH_H */
