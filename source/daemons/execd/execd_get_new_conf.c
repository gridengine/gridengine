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
#include "dispatcher.h"
#include "execd_get_new_conf.h"
#include "sge_load_sensor.h"
#include "sgermon.h"
#include "admin_mail.h"
#include "sge_string.h"

extern lList *execd_config_list;

/*
** DESCRIPTION
**   retrieves new configuration from qmaster, very similar to what is
**   executed on startup. This function is triggered by the execd
**   dispatcher table when the tag TAG_GET_NEW_CONF is received.
*/
int execd_get_new_conf(de, pb, apb, rcvtimeout, synchron, err_str, answer_error)
struct dispatch_entry *de;
sge_pack_buffer *pb, *apb; 
u_long *rcvtimeout; 
int *synchron; 
char *err_str; 
int answer_error;
{
   int ret;

   DENTER(TOP_LAYER, "execd_get_new_conf");

   ret = get_merged_configuration(&execd_config_list);

   /* EB */

   /*
   ** admin mail block is released on new conf
   */
   adm_mail_reset(BIT_ADM_NEW_CONF);

   sge_ls_qidle(use_qidle);
   DPRINTF(("use_qidle: %d\n", use_qidle));

   DEXIT;
   return ret;
}


