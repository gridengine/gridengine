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

#include <string.h>

#include "sge_conf.h"
#include "gdi_conf.h"
#include "dispatcher.h"
#include "execd_get_new_conf.h"
#include "sge_load_sensor.h"
#include "sgermon.h"
#include "admin_mail.h"
#include "sge_string.h"
#include "sge_log.h"

#include "msg_common.h"

#ifdef TEST_GDI2
#include "sge_gdi_ctx.h"
#else
#include "sge_prog.h"
#include "setup_path.h"
#endif

/*
** DESCRIPTION
**   retrieves new configuration from qmaster, very similar to what is
**   executed on startup. This function is triggered by the execd
**   dispatcher table when the tag TAG_GET_NEW_CONF is received.
*/
int execd_get_new_conf(void *context, 
                       dispatch_entry *de, 
                       sge_pack_buffer *pb, 
                       sge_pack_buffer *apb, 
                       u_long *rcvtimeout, 
                       int *synchron, 
                       char *err_str, 
                       int answer_error)
{
   int ret;
   bool use_qidle = mconf_get_use_qidle();
   u_long32 dummy; /* always 0 */ 
   char* old_spool = NULL;
   char* spool_dir = NULL;

#ifdef TEST_GDI2
   sge_gdi_ctx_class_t *ctx = (sge_gdi_ctx_class_t *)context;
#else
   const char *qualified_hostname = uti_state_get_qualified_hostname();
   const char *cell_root = path_state_get_cell_root();
   u_long32 progid = uti_state_get_mewho();
#endif

   DENTER(TOP_LAYER, "execd_get_new_conf");

   unpackint(pb, &dummy);

   old_spool = mconf_get_execd_spool_dir();  

#ifdef TEST_GDI2
   ret = gdi2_get_merged_configuration(ctx, &Execd_Config_List);
#else
   ret = get_merged_configuration(progid, qualified_hostname, cell_root, &Execd_Config_List);
#endif   
  
   spool_dir = mconf_get_execd_spool_dir(); 
   if (strcmp(old_spool, spool_dir)) {
      WARNING((SGE_EVENT, MSG_WARN_CHANGENOTEFFECTEDUNTILRESTARTOFEXECHOSTS, "execd_spool_dir"));
   }

   /*
   ** admin mail block is released on new conf
   */
   adm_mail_reset(BIT_ADM_NEW_CONF);

   sge_ls_qidle(use_qidle);
   DPRINTF(("use_qidle: %d\n", use_qidle));
   FREE(old_spool);
   FREE(spool_dir);

   DEXIT;
   return ret;
}


