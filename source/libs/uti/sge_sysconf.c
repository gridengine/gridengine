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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <stdio.h>
#include <unistd.h>

#include "basis_types.h"
#include "sgermon.h"
#include "sge_sysconf.h"
#include "sge_log.h"
#include "msg_utilib.h"

#if defined(LINUX)
#  include <limits.h>
#endif

u_long32 sge_sysconf (sge_sysconf_id id) {
   u_long32 ret = 0;

   DENTER(BASIS_LAYER, "sge_sysconf");
   switch (id) {
      case sge_sysconf_NGROUPS_MAX:   
#if defined(AIX42)
         ret = NGROUPS;
#else
         ret = sysconf(_SC_NGROUPS_MAX);
#endif
      break;
      default:
         CRITICAL((SGE_EVENT, MSG_SYSCONF_UNABLETORETRIEVE_I, (int) id));
      break;
   }
   DEXIT;
   return ret;
}
