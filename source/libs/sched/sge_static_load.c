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
#include <ctype.h>
#ifndef WIN32NATIVE
#	include <unistd.h>
#endif
#include <string.h>

#include "sgermon.h"
#include "sge_all_listsL.h"
#include "sge_static_load.h"

/* EB: TODO: remove this file */

#if 0

int sge_is_static_load_value(const char *name) 
{
   DENTER(BASIS_LAYER, "sge_is_static_load_value");

   if (!name) {
      DEXIT;
      return 0;
   }

   if (!strcmp(name, LOAD_ATTR_ARCH) || 
       !strcmp(name, LOAD_ATTR_NUM_PROC) ||
       !strcmp(name, LOAD_ATTR_MEM_TOTAL) ||
       !strcmp(name, LOAD_ATTR_SWAP_TOTAL) ||
       !strcmp(name, LOAD_ATTR_VIRTUAL_TOTAL)) {
     DEXIT;
     return 1;
   }

   DEXIT;
   return 0;
}

#endif
