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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "sge_me.h"
#include "sge_prognames.h"
#include "sge_answerL.h"
#include "sge_gdi_intern.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "sge_getme.h"
#include "setup_path.h"
#include "qm_name.h"
#include "msg_gdilib.h"
#include "commlib.h"
#include "sge_string.h"
#include "msg_common.h"
#include "sge_switch_user.h"
#include "msg_utilib.h"
#include "sge_feature.h"

extern long compression_level;
extern long compression_threshold;

/*******************************************************************/
void sge_setup(
u_long32 sge_formal_prog_name,
lList **alpp 
) {
   DENTER(TOP_LAYER, "sge_setup");

   /*
   ** for setuid clients we must seteuid to the users uid
   */
   if (run_as_user()) {   
      CRITICAL((SGE_EVENT, MSG_SYSTEM_CANTRUNASCALLINGUSER));
      SGE_EXIT(1);
   }   

   sge_getme(sge_formal_prog_name);
   memset(&path, 0, sizeof(path));
   sge_setup_paths(me.default_cell, &path, alpp);

   if (alpp && *alpp){
      DEXIT;
      return;
   }

   if (feature_initialize_from_file(path.product_mode_file)) {
      SGE_EXIT(1);
   }

      
   /* qmaster and shadowd should not fail on nonexistant act_qmaster file */
   if (!(me.who == QMASTER || me.who == SHADOWD) && !sge_get_master(1)) {
      if (alpp) {
         sprintf(SGE_EVENT, MSG_GDI_READMASTERNAMEFAILED_S,
                     path.act_qmaster_file);
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         DEXIT;
         return;
      }
      else
         SGE_EXIT(1);
   }

#ifdef COMMCOMPRESS
   {
      char* cl;
      cl = getenv("SGE_COMPRESSION_LEVEL");
      if(cl) {
         compression_level = (int)strtol(cl, NULL, 10);
         if(compression_level == LONG_MIN || compression_level == LONG_MAX) {
            sprintf(SGE_EVENT, MSG_GDI_NOVALIDSGECOMPRESSIONLEVEL_S , cl);
            compression_level = Z_DEFAULT_COMPRESSION;
         }
      }
      DPRINTF((MSG_GDI_SETCOMPRESSIONLEVEL_D , u32c (compression_level)));
      
      cl = getenv("SGE_COMPRESSION_THRESHOLD");
      if(cl) {
         compression_threshold = (int)strtol(cl, NULL, 10);
         if(compression_threshold == LONG_MIN || compression_threshold == LONG_MAX || compression_threshold < 0) {
            sprintf(SGE_EVENT, MSG_GDI_NOVALIDSGECOMPRESSIONTHRESHOLD_S , cl);
            compression_threshold = 10 * 1024;
         }
      }
      else
         compression_threshold = 10 * 1024;
      DPRINTF((MSG_GDI_SETCOMPRESSIONTHRESHOLD_D , u32c(compression_threshold)));
   }
#endif

   DEXIT;
   return;
}

int reresolve_me_qualified_hostname()
{
   int ret;
   char unique_hostname[MAXHOSTLEN];

   DENTER(TOP_LAYER, "reresolve_me_qualified_hostname");

   /*
   ** get aliased hostname from commd
   */
   if ((ret=getuniquehostname(me.qualified_hostname, unique_hostname, 0))!=CL_OK) {
      WARNING((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_SS, 
               me.qualified_hostname, cl_errstr(ret)));
      DEXIT;
      return ret;
   }

   me.qualified_hostname = sge_strdup(me.qualified_hostname, unique_hostname);
   DPRINTF(("me.qualified_hostname: %s\n", me.qualified_hostname));
   DEXIT;
   return CL_OK;
}
