
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "basis_types.h"
#include "version.h"
#include "commd.h"
#include "setup_commd_path.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_prog.h"
#include "sge.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"

#include "sge_bootstrap.h"
#include "setup_path.h"

#include "msg_common.h"
#include "msg_commd.h"


void sge_commd_setup( u_long32 sge_formal_prog_name ) {
   DENTER(TOP_LAYER, "sge_commd_setup");

   /*
   ** for setuid clients we must seteuid to the users uid
   */
   if (sge_run_as_user()) {   
      CRITICAL((SGE_EVENT, MSG_SYSTEM_CANTRUNASCALLINGUSER));
      SGE_EXIT(1);
   }   

   sge_getme(sge_formal_prog_name);
   if (uti_state_get_mewho() != COMMD) {
       SGE_EXIT(1);
   }

   sge_setup_paths(uti_state_get_default_cell(), NULL);

   sge_bootstrap(NULL);

   DEXIT;
   return;
}

int use_reserved_port(void) {
   const char *product_mode;

   DENTER(TOP_LAYER, "use_reserved_port");

   product_mode = bootstrap_get_product_mode();

   DPRINTF(("product mode is \"%s\"\n",product_mode ));
   if (strstr(product_mode, "reserved_port") != NULL) {
      DPRINTF(("use_reserved_port return: 1\n" ));
      DEXIT;
      return 1;
   }   
   DEXIT;
   return 0;
}

const char* get_short_product_name(void) {
   static char *commd_version_string;

   const char *product_mode = bootstrap_get_product_mode();

   if (strstr(product_mode, "sgeee") != NULL) {
      sprintf(commd_version_string,""SFN" "SFN"", "SGEEE", GDI_VERSION);
   } else {
      sprintf(commd_version_string,""SFN" "SFN"", "SGE", GDI_VERSION);
   }   
   return commd_version_string;
}


