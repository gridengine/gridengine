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

#ifndef _MSG_DRMAA_H
#define	_MSG_DRMAA_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "basis_types.h"
   
#define MSG_DRMAA_SWITCH_WITH_NO_CAT   _MESSAGE(45001, _("No job category could be found even though -cat was detected\n"))
#define MSG_DRMAA_UNKNOWN_JOB_CAT      _MESSAGE(45002, _("Unknown job category\n"))
#define MSG_DRMAA_INVALID_TIME_STRING  _MESSAGE(45003, _("invalid format for job start time\n"))
#define MSG_DRMAA_TIME_PARSE_ERROR     _MESSAGE(45004, _("Error parsing DRMAA date string\n"))
#define MSG_DRMAA_INC_NOT_ALLOWED      _MESSAGE(45005, _("The $drmaa_inc_ph$ placeholder is not allowed when the DRMAA_NATIVE_SPECIFICATION attribute contains \"-b n\"\n"))

#ifdef	__cplusplus
}
#endif

#endif	/* _MSG_DRMAA_H */
