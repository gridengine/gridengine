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
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_gdi.h"
#include "sge_log.h"
#include "read_write_manop.h"
#include "cull.h"
#include "sge_manopL.h"
#include "sge_stdio.h"
#include "sge_stat.h"
#include "msg_common.h" 
#include "sge_spoolmsg.h"
#include "sge_feature.h"

extern lList *Master_Manager_List;
extern lList *Master_Operator_List;

/* ------------------------------------------------------------

   read_manop()

   reads either manager or operator list from disk

*/
int read_manop(
int target 
) {
   lListElem *ep;
   lList **lpp;
   stringT filename;
   char str[256];
   FILE *fp;
   SGE_STRUCT_STAT st;

   DENTER(TOP_LAYER, "read_manop");

   switch (target) {
   case SGE_MANAGER_LIST:
      lpp = &Master_Manager_List;      
      strcpy(filename, MAN_FILE);
      break;
      
   case SGE_OPERATOR_LIST:
      lpp = &Master_Operator_List;      
      strcpy(filename, OP_FILE);
      break;

   default:
      DEXIT;
      return 1;
   }

   /* if no such file exists. ok return without error */
   if (SGE_STAT(filename, &st) && errno==ENOENT) {
      DEXIT;
      return 0;
   }

   fp = fopen(filename, "r");
   if (!fp) {
      ERROR((SGE_EVENT, MSG_FILE_ERROROPENINGX_S, filename));
      DEXIT;
      return 1;
   }
   
   if ( *lpp ) 
      *lpp = lFreeList(*lpp);

   *lpp = lCreateList("man/op list", MO_Type);

   while (fscanf(fp, "%[^\n]\n", str) == 1) {
      ep = lCreateElem(MO_Type);
      if (str[0] != COMMENT_CHAR) {
         lSetString(ep, MO_name, str);
         lAppendElem(*lpp, ep);
      } else {
         lFreeElem(ep);
      }
   }

   fclose(fp);

   DEXIT;
   return 0;
}



/* ------------------------------------------------------------

   write_manop()

   writes either manager or operator list to disk

   spool:
      1 write for spooling
      0 write only user controlled fields
 
*/
 
int write_manop(
int spool,
int target 
) {
   FILE *fp;
   lListElem *ep;
   lList *lp;
   char filename[255], real_filename[255];

   DENTER(TOP_LAYER, "write_manop");

   switch (target) {
   case SGE_MANAGER_LIST:
      lp = Master_Manager_List;      
      strcpy(filename, ".");
      strcat(filename, MAN_FILE);
      strcpy(real_filename, MAN_FILE);
      break;
      
   case SGE_OPERATOR_LIST:
      lp = Master_Operator_List;      
      strcpy(filename, ".");
      strcat(filename, OP_FILE);
      strcpy(real_filename, OP_FILE);
      break;

   default:
      DEXIT;
      return 1;
   }

   fp = fopen(filename, "w");
   if (!fp) {
      ERROR((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, filename, strerror(errno)));
      DEXIT;
      return 1;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   }  

   for_each(ep, lp) {
      FPRINTF((fp, "%s\n", lGetString(ep, MO_name)));
   }

   fclose(fp);

   if (rename(filename, real_filename) == -1) {
      DEXIT;
      return 1;
   } else {
      strcpy(filename, real_filename);
   }     

   DEXIT;
   return 0;

FPRINTF_ERROR:
   DEXIT;
   return 1;  
}


