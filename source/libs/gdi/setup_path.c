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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "def.h"
#include "sgermon.h"
#include "basis_types.h"
#include "sge_prognames.h"
#include "sge_gdi_intern.h"
#include "sge_answerL.h"
#include "sge_me.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "sge_string.h"
#include "utility.h"
#include "msg_utilib.h"
#include "msg_gdilib.h"
#include "msg_common.h"
#include "sge_arch.h"
#include "setup_path.h"
#include "sge_stat.h" 

sge_path_type path = { NULL };

/*-----------------------------------------------------------------------
 * sge_setup_paths
 * set SGE_ROOT and SGE_CELL dependent path components
 * spool directory may later be overridden by global configuration
 * This routine may be called as often as is is necessary.
 *-----------------------------------------------------------------------*/
void sge_setup_paths(
const char *sge_cell,
sge_path_type *p,
lList **alpp 
) {
   char *cell_root;
   const char *sge_root;
   char *common_dir;
   SGE_STRUCT_STAT sbuf;
   int cell_root_len, common_len;
   
   DENTER(TOP_LAYER, "sge_setup_paths");
   
   sge_root = sge_sge_root();

   if (SGE_STAT(sge_root, &sbuf)) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_SGEROOTNOTFOUND_S, sge_root));
      if (alpp) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         DEXIT;
         return;
      }   
      else
         SGE_EXIT(1);
   }
   
   if (!S_ISDIR(sbuf.st_mode)) {
      CRITICAL((SGE_EVENT, MSG_GDI_SGEROOTNOTADIRECTORY_S , sge_root));
      if (alpp) { 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         DEXIT;
         return;
      }
      else
         SGE_EXIT(1);
      DEXIT;
   } 

   cell_root = sge_malloc(strlen(sge_root) + strlen(sge_cell) + 2);
   if (!cell_root) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NOMEM));
      if (alpp) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         DEXIT;
         return;
      }
      else
         SGE_EXIT(1);
      DEXIT;
   }

   sprintf(cell_root, "%s"PATH_SEPARATOR"%s", sge_root, sge_cell);   

   if (SGE_STAT(cell_root, &sbuf)) {
      if (me.who != QMASTER) {
         CRITICAL((SGE_EVENT, MSG_SGETEXT_NOSGECELL_S, cell_root));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
            DEXIT;
            return;
         }
         else
            SGE_EXIT(1);
         DEXIT;
      }   
   }

   common_dir = malloc(strlen(cell_root) + strlen(COMMON_DIR) + 2);
   sprintf(common_dir, "%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR);
   if (SGE_STAT(common_dir, &sbuf)) {
      if (me.who != QMASTER) {  
         CRITICAL((SGE_EVENT, MSG_GDI_DIRECTORYNOTEXIST_S , common_dir));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
            DEXIT;
            return;
         }
         else
            SGE_EXIT(1);
         DEXIT;
      }   
   }       
   FREE(common_dir);

   cell_root_len = strlen(cell_root) + 2;               /* slash and 0 Byte */
   common_len    = cell_root_len + strlen(COMMON_DIR) + 1; /* slash */

   FREE(p->sge_root);
   p->sge_root = sge_strdup(p->sge_root, sge_root);
   
   FREE(p->cell_root);
   p->cell_root = cell_root;
   
   FREE(p->conf_file);        
   p->conf_file = malloc(cell_root_len + common_len + strlen(CONF_FILE));
   sprintf(p->conf_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, CONF_FILE);

   FREE(p->sched_conf_file);
   p->sched_conf_file = 
      malloc(cell_root_len + common_len + strlen(SCHED_CONF_FILE));
   sprintf(p->sched_conf_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, 
           SCHED_CONF_FILE);
   
   FREE(p->act_qmaster_file);
   p->act_qmaster_file = malloc(cell_root_len + common_len + 
                                strlen(ACT_QMASTER_FILE));
   sprintf(p->act_qmaster_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, 
           ACT_QMASTER_FILE);
   
   FREE(p->acct_file);
   p->acct_file = malloc(cell_root_len + common_len + strlen(ACCT_FILE));
   sprintf(p->acct_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, ACCT_FILE);

   FREE(p->stat_file);        
   p->stat_file = malloc(cell_root_len + common_len + strlen(STAT_FILE));
   sprintf(p->stat_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, STAT_FILE);
   
   FREE(p->local_conf_dir);
   p->local_conf_dir = malloc(cell_root_len + common_len + strlen(LOCAL_CONF_DIR) + 1);
   sprintf(p->local_conf_dir, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, LOCAL_CONF_DIR);

   FREE(p->history_dir);
   p->history_dir = malloc(cell_root_len + common_len + strlen(HISTORY_DIR));
   sprintf(p->history_dir, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, HISTORY_DIR);

   FREE(p->shadow_masters_file);        
   p->shadow_masters_file = malloc(cell_root_len + common_len + strlen(SHADOW_MASTERS_FILE) + 1);
   sprintf(p->shadow_masters_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, SHADOW_MASTERS_FILE);

   FREE(p->license_file);        
   p->license_file = malloc(cell_root_len + common_len + strlen(LICENSE_FILE) + 1);
   sprintf(p->license_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, LICENSE_FILE);

   FREE(p->product_mode_file);        
   p->product_mode_file = malloc(cell_root_len + common_len + strlen(PRODUCT_MODE_FILE) + 1);
   sprintf(p->product_mode_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, PRODUCT_MODE_FILE);

   FREE(p->master_ior_file);        
   p->master_ior_file = malloc(cell_root_len + common_len + strlen(MASTER_IOR_FILE) + 1);
   sprintf(p->master_ior_file, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, MASTER_IOR_FILE);

   FREE(p->qmaster_args);
   p->qmaster_args = malloc(cell_root_len + common_len + strlen(QMASTER_ARG_FILE));
   sprintf(p->qmaster_args, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, QMASTER_ARG_FILE);

   FREE(p->schedd_args);
   p->schedd_args = malloc(cell_root_len + common_len + strlen(SCHEDD_ARG_FILE));
   sprintf(p->schedd_args, "%s"PATH_SEPARATOR"%s"PATH_SEPARATOR"%s", cell_root, COMMON_DIR, SCHEDD_ARG_FILE);

   DPRINTF(("sge_root         >%s<\n", p->sge_root));
   DPRINTF(("cell_root           >%s<\n", p->cell_root));
   DPRINTF(("conf_file           >%s<\n", p->conf_file));
   DPRINTF(("act_qmaster_file    >%s<\n", p->act_qmaster_file));
   DPRINTF(("acct_file           >%s<\n", p->acct_file));
   DPRINTF(("stat_file           >%s<\n", p->stat_file));
   DPRINTF(("local_conf_dir      >%s<\n", p->local_conf_dir));
   DPRINTF(("history_dir         >%s<\n", p->history_dir));
   DPRINTF(("shadow_masters_file >%s<\n", p->shadow_masters_file));
   DPRINTF(("product_mode_file   >%s<\n", p->product_mode_file));
   DPRINTF(("qmaster_args        >%s<\n", p->qmaster_args));
   DPRINTF(("schedd_args         >%s<\n", p->schedd_args));
   DPRINTF(("master_ior_file     >%s<\n", p->master_ior_file));
   
   DEXIT;
}

#ifdef WIN32NATIVE
void sge_delete_paths ()
{
	FREE(path.sge_root);
	FREE(path.cell_root);
	FREE(path.conf_file);
	FREE(path.sched_conf_file);
	FREE(path.act_qmaster_file);
	FREE(path.acct_file);
	FREE(path.stat_file);
	FREE(path.local_conf_dir);
	FREE(path.history_dir);
	FREE(path.shadow_masters_file);
	FREE(path.license_file);
	FREE(path.product_mode_file);
	FREE(path.qmaster_args);
	FREE(path.schedd_args);
	FREE(path.master_ior_file); 
}
#endif /* WIN32NATIVE */
