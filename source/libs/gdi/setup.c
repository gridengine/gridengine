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

#include "sge_bootstrap.h"
#include "sge_prog.h"
#include "sge_gdiP.h"
#include "sgermon.h"
#include "sge_log.h"
#include "setup_path.h"
#include "qm_name.h"
#include "commlib.h"
#include "sge_string.h"
#include "sge_feature.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"
#include "sge_parse_num_par.h"
#include "sge_hostname.h"
#include "sge_spool.h"
#include "sge_answer.h"

#include "msg_common.h"
#include "msg_gdilib.h"

extern long compression_level;
extern long compression_threshold;

/****** setup/sge_setup() ******************************************************
*  NAME
*     sge_setup() -- Setup of some SGE components
*
*  SYNOPSIS
*     int sge_setup(u_long32 sge_formal_prog_name, lList **alpp) 
*
*  FUNCTION
*     Setup of some SGE components
*
*  INPUTS
*     u_long32 sge_formal_prog_name - The formal program name.
*     lList **alpp                  - returns diagnosis information 
*                                     in case of an error.
*
*  RESULT
*     int - 0 on success
*
*  NOTES
*     MT-NOTE: sge_setup() is MT safe
*******************************************************************************/
int sge_setup(
u_long32 sge_formal_prog_name,
lList **alpp 
) {
   dstring error_dstring = DSTRING_INIT;

   DENTER(TOP_LAYER, "sge_setup");

   /*
   ** for setuid clients we must seteuid to the users uid
   */
   if (sge_run_as_user()) {   
      CRITICAL((SGE_EVENT, MSG_SYSTEM_CANTRUNASCALLINGUSER));
      if (!uti_state_get_exit_on_error()) {
         answer_list_add(alpp, SGE_EVENT, STATUS_DENIED, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
      SGE_EXIT(1);
   }   

   sge_getme(sge_formal_prog_name);

   if (!sge_setup_paths(uti_state_get_default_cell(), &error_dstring)) {
      if (alpp == NULL) {
         CRITICAL((SGE_EVENT, sge_dstring_get_string(&error_dstring)));
      } else {
         answer_list_add(alpp, sge_dstring_get_string(&error_dstring), 
                         STATUS_NOCONFIG, ANSWER_QUALITY_ERROR);
      }
      sge_dstring_free(&error_dstring);
      DEXIT;
      return -1;
   }

   if (!sge_bootstrap(&error_dstring)) {
      if (alpp == NULL) {
         CRITICAL((SGE_EVENT, sge_dstring_get_string(&error_dstring)));
      } else {
         answer_list_add(alpp, sge_dstring_get_string(&error_dstring), 
                         STATUS_NOCONFIG, ANSWER_QUALITY_ERROR);
      }
      sge_dstring_free(&error_dstring);
      if (!uti_state_get_exit_on_error()) {
         DEXIT;
         return -1;
      }
      SGE_EXIT(1);
   }

   if (feature_initialize_from_string(bootstrap_get_product_mode())) {
      if (alpp == NULL) {
         CRITICAL((SGE_EVENT, sge_dstring_get_string(&error_dstring)));
      } else {
         answer_list_add(alpp, sge_dstring_get_string(&error_dstring), 
                         STATUS_NOCONFIG, ANSWER_QUALITY_ERROR);
      }
      sge_dstring_free(&error_dstring);
      if (!uti_state_get_exit_on_error()) {
         DEXIT;
         return -1;
      }
      SGE_EXIT(1);
   }

   /* qmaster and shadowd should not fail on nonexistant act_qmaster file */
   if (!(uti_state_get_mewho() == QMASTER || uti_state_get_mewho() == SHADOWD) && !sge_get_master(1)) {
      if (!uti_state_get_exit_on_error()) {
         if (alpp) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_READMASTERNAMEFAILED_S,
                        path_state_get_act_qmaster_file()));
            answer_list_add(alpp, SGE_EVENT, STATUS_EDISK, ANSWER_QUALITY_ERROR);
         }
         DEXIT;
         return -1;
      }
      SGE_EXIT(1);
   }

#ifdef COMMCOMPRESS
   {
      char* cl;
      cl = getenv("SGE_COMPRESSION_LEVEL");
      if(cl) {
         compression_level = (int)strtol(cl, NULL, 10);
         if(compression_level == LONG_MIN || compression_level == LONG_MAX) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_NOVALIDSGECOMPRESSIONLEVEL_S , cl));
            compression_level = Z_DEFAULT_COMPRESSION;
         }
      }
      DPRINTF((MSG_GDI_SETCOMPRESSIONLEVEL_D , u32c (compression_level)));
      
      cl = getenv("SGE_COMPRESSION_THRESHOLD");
      if(cl) {
         compression_threshold = (int)strtol(cl, NULL, 10);
         if(compression_threshold == LONG_MIN || compression_threshold == LONG_MAX || compression_threshold < 0) {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_GDI_NOVALIDSGECOMPRESSIONTHRESHOLD_S , cl));
            compression_threshold = 10 * 1024;
         }
      }
      else
         compression_threshold = 10 * 1024;
      DPRINTF((MSG_GDI_SETCOMPRESSIONTHRESHOLD_D , u32c(compression_threshold)));
   }
#endif

   DEXIT;
   return 0;
}

/****** setup/reresolve_me_qualified_hostname() ********************************
*  NAME
*     reresolve_me_qualified_hostname() -- resolve the own hostname
*
*  SYNOPSIS
*     int reresolve_me_qualified_hostname(void) 
*
*  FUNCTION
*     Resolves hostname using commlib getuniquehostname(). This is usually
*     done to get hostname resolving results that is in compliance with
*     sge_hostalias file in common directory.
*
*  RESULT
*     int - CR_OK on success
*
*  NOTES
*     MT-NOTE: reresolve_me_qualified_hostname() is MT safe
*******************************************************************************/
#ifdef ENABLE_NGC
int reresolve_me_qualified_hostname(void)
{
   int ret;
   char unique_hostname[MAXHOSTLEN];

   DENTER(TOP_LAYER, "reresolve_me_qualified_hostname");

   /*
   ** get aliased hostname from commd
   */
   if ((ret=getuniquehostname(uti_state_get_qualified_hostname(), unique_hostname, 0))!=CL_RETVAL_OK) {
      WARNING((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_SS, 
               uti_state_get_qualified_hostname(), cl_get_error_text(ret)));
      DEXIT;
      return ret;
   }

   uti_state_set_qualified_hostname(unique_hostname);
   DPRINTF(("me.qualified_hostname: %s\n", uti_state_get_qualified_hostname()));
   DEXIT;
   return CL_RETVAL_OK;
}
#else
int reresolve_me_qualified_hostname(void)
{
   int ret;
   char unique_hostname[MAXHOSTLEN];

   DENTER(TOP_LAYER, "reresolve_me_qualified_hostname");

   /*
   ** get aliased hostname from commd
   */
   if ((ret=getuniquehostname(uti_state_get_qualified_hostname(), unique_hostname, 0))!=CL_OK) {
      WARNING((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_SS, 
               uti_state_get_qualified_hostname(), cl_errstr(ret)));
      DEXIT;
      return ret;
   }

   uti_state_set_qualified_hostname(unique_hostname);
   DPRINTF(("me.qualified_hostname: %s\n", uti_state_get_qualified_hostname()));
   DEXIT;
   return CL_OK;
}
#endif

