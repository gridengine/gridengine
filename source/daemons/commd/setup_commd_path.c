
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

#include "msg_common.h"
#include "msg_commd.h"


static char  commd_product_mode[128] = "";  
static char  commd_version_string[255] = "";
extern char* product_mode_file; 
extern char* actmasterfile;

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

   if (actmasterfile == NULL) {
      /* expect act_master file at default location */
      actmasterfile = get_act_master_path(uti_state_get_default_cell());
   }
   if (product_mode_file == NULL) {
      /* expect product_mode_file file at default location */
      product_mode_file = get_product_mode_file_path(uti_state_get_default_cell());
   }

   read_product_mode_file(product_mode_file);

   DEXIT;
   return;
}

int read_product_mode_file(const char *filename) 
{

   FILE *fp = NULL;
   struct stat file_info;
   char buf[128] = "";

   if (filename == NULL) {
      return -1;
   } 
   if (stat(filename, &file_info) || !(fp=fopen(filename,"r"))) {
      return -1;
   }    
   fgets(buf, 127, fp);
   fclose(fp);
    
   sscanf(buf, "%s", commd_product_mode);
   return 0;
}

int use_reserved_port(void) {
   DENTER(TOP_LAYER, "use_reserved_port");

   if (commd_product_mode == NULL) { 
      read_product_mode_file(product_mode_file);
   }
   if ( strlen(commd_product_mode) == 0) {
      read_product_mode_file(product_mode_file);
   }
   DPRINTF(("product mode is \"%s\"\n",commd_product_mode ));
   if (strstr(commd_product_mode, "reserved_port") != NULL) {
      DPRINTF(("use_reserved_port return: 1\n" ));
      DEXIT;
      return 1;
   }   
   DEXIT;
   return 0;
}

const char* get_short_product_name(void) {
   if (commd_product_mode == NULL) { 
      read_product_mode_file(product_mode_file);
   }
   if ( strlen(commd_product_mode) == 0) {
      read_product_mode_file(product_mode_file);
   }
   if (strstr(commd_product_mode, "sgeee") != NULL) {
      sprintf(commd_version_string,""SFN" "SFN"", "SGEEE", GDI_VERSION);
   } else {
      sprintf(commd_version_string,""SFN" "SFN"", "SGE", GDI_VERSION);
   }   
   return commd_version_string;
}


/*-----------------------------------------------------------------------
 * get_act_master_path
 *-----------------------------------------------------------------------*/
char *get_act_master_path( const char *sge_cell ) {
   const char *sge_root;
   char *cp;
   int len;
   SGE_STRUCT_STAT sbuf;
      
   DENTER(TOP_LAYER, "get_act_master_path");
   
   sge_root = sge_get_root_dir(1, NULL, 0, 1);
   if (SGE_STAT(sge_root, &sbuf)) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_SGEROOTNOTFOUND_S , sge_root));
      SGE_EXIT(1);
   }

   len = strlen(sge_root) + strlen(sge_cell) 
         + strlen(COMMON_DIR) + strlen(ACT_QMASTER_FILE) + 5;

   if (!(cp = malloc(len))) {
      CRITICAL((SGE_EVENT, MSG_MEMORY_MALLOCFAILEDFORPATHTOACTQMASTERFILE ));
      SGE_EXIT(1);
   }

   sprintf(cp, "%s/%s/%s/%s", sge_root, sge_cell, COMMON_DIR,ACT_QMASTER_FILE ); 
   DEXIT;
   return cp;
}



/*-----------------------------------------------------------------------
 * get_product_mode_file_path
 *-----------------------------------------------------------------------*/
char *get_product_mode_file_path( const char *sge_cell ) {
   const char *sge_root;
   char *cp;
   int len;
   SGE_STRUCT_STAT sbuf;
      
   DENTER(TOP_LAYER, "get_product_mode_file_path");
   
   sge_root = sge_get_root_dir(1, NULL, 0, 1); 
   if (SGE_STAT(sge_root, &sbuf)) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_SGEROOTNOTFOUND_S , sge_root));
      SGE_EXIT(1);
   }

   len = strlen(sge_root) + strlen(sge_cell) + strlen(COMMON_DIR) + strlen(PRODUCT_MODE_FILE) + 5;

   if (!(cp = malloc(len))) {
      CRITICAL((SGE_EVENT, MSG_MEMORY_MALLOCFAILEDFORPATHTOPRODMODFILE ));
      SGE_EXIT(1);
   }

   sprintf(cp, "%s/%s/%s/%s", sge_root, sge_cell, COMMON_DIR,PRODUCT_MODE_FILE ); 
   DEXIT;
   return cp;
}







