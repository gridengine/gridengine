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

#include "sgermon.h"
#include "sge_log.h"
#include "sge_feature.h"
#include "shutdown.h"
#include "sge_exit.h"
#include "setup_path.h"
#include "sge_getme.h"
#include "msg_daemons_common.h"

void starting_up()
{
   extern u_long32 logginglevel;
   u_long32 old_ll = logginglevel;

   DENTER(TOP_LAYER, "starting_up");

   logginglevel = LOG_INFO;

   INFO((SGE_EVENT, MSG_STARTUP_STARTINGUP_S, feature_get_product_name(FS_VERSION)));

   logginglevel = old_ll;

   DEXIT;
   return;
}

/******************************************************************************/
void sge_shutdown()
{
   extern u_long32 logginglevel;
   u_long32 old_ll = logginglevel;

   DENTER(TOP_LAYER, "sge_shutdown");

   logginglevel = LOG_INFO;
   INFO((SGE_EVENT, MSG_SHADOWD_CONTROLLEDSHUTDOWN_S, feature_get_product_name(FS_VERSION)));
   logginglevel = old_ll;

#ifdef WIN32NATIVE 
   /*
   ** free previously allocated mem to make win compiler happy
   */
   sge_delete_paths();
   sge_deleteme();
#endif   

   DEXIT;

   SGE_EXIT(0); /* call sge_exit() */
}
