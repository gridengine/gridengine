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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sgermon.h"
#include "sge.h"
#include "sge_stringL.h"
#include "read_write_host_group.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_feature.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sge_answer.h"
#include "sge_hgroup.h"
#include "sge_href.h"
#include "sge_conf.h"
#include "sge_ulong.h"
#include "sge_centry.h"

int read_centry_work(
lList **alpp,   /* anser list */
lList **clpp,   /* parsed file */
int fields[],   /* not needed */
lListElem *ep,  /* list element to fill of type GRP_Type */
int spool,      /* look above */
int flag,       /* user flag */
int *tag,       /* user return value */
int parsing_type
) {
   int ret = 0;

   DENTER(TOP_LAYER, "read_centry_work");

   /* --------- CE_name */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "name", ep, CE_name)) ?-1:0;
   }

   /* --------- CE_shortcut */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "shortcut", ep, CE_shortcut)) ?-1:0;
   }

   /* --------- CE_valtype */
   if (ret == 0) {
      ret = (!set_conf_centry_type(alpp, clpp, fields, "type", ep, CE_valtype))?-1:0; 
   }

   /* --------- CE_relop */
   if (ret == 0) {
      ret = (!set_conf_centry_relop(alpp, clpp, fields, "relop", ep, CE_relop))?-1:0; 
   }

   /* --------- CE_requestable */
   if (ret == 0) {
      ret = (!set_conf_centry_requestable(alpp, clpp, fields, "requestable", ep, CE_requestable))?-1:0; 
   }

   /* --------- CE_consumable */
   if (ret == 0) {
      ret = (!set_conf_bool(alpp, clpp, fields, "consumable", ep, CE_consumable)) ?-1:0;
   }

   /* --------- CE_default */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "default", ep, CE_default)) ?-1:0;
   }

   DEXIT;
   return ret;
}

lListElem *cull_read_in_centry(
const char *dirname,
const char *filename,
int spool,
int flag,
int *tag,
int fields[]
) {
   lListElem *ep;
   struct read_object_args args = { CE_Type, "", read_centry_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_centry");

   ep = read_object(dirname, filename, spool, 0,RCL_NO_VALUE, &args, tag?tag:&intern_tag, NULL);

   DEXIT;
   return ep;
}

char *write_centry(int spool, int how, const lListElem *ep)
{
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "write_centry");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   strcpy(filename, lGetString(ep, CE_name));
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         if (!sge_tmpnam(filename)) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            DEXIT;
            return NULL;
         }
      } else  {
         sprintf(filename, "%s/.%s", CENTRY_DIR,
            lGetString(ep, CE_name));
         sprintf(real_filename, "%s/%s", CENTRY_DIR,
            lGetString(ep, CE_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, filename, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   if (spool == 1 && sge_spoolmsg_write(fp, COMMENT_CHAR,
                feature_get_product_name(FS_SHORT_VERSION, &ds)) < 0) {
      goto FPRINTF_ERROR;
   }

   /* --------- CE_name */
   FPRINTF((fp, "name        %s\n", lGetString(ep, CE_name)));

   /* --------- CE_shortcut */
   FPRINTF((fp, "shortcut    %s\n", lGetString(ep, CE_shortcut)));

   /* --------- CE_valtype */
   FPRINTF((fp, "type        %s\n", map_type2str(lGetUlong(ep, CE_valtype))));

   /* --------- CE_relop */
   FPRINTF((fp, "relop       %s\n", map_op2str(lGetUlong(ep, CE_relop))));

   /* --------- CE_forced, CE_request */
   FPRINTF((fp, "requestable %s\n", (lGetUlong(ep, CE_requestable) == REQU_FORCED) ? "FORCED" :
                                    (lGetUlong(ep, CE_requestable) == REQU_YES) ? "YES" : "NO"));

   /* --------- CE_consumable */
   FPRINTF((fp, "consumable  %s\n", (lGetBool(ep, CE_consumable)) ? "YES" : "NO"));

   /* --------- CE_default */
   FPRINTF((fp, "default     %s\n", lGetString(ep, CE_default)));

   if (how!=0) {
      fclose(fp);
   }
   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(filename, real_filename);
      }
   }  

   DEXIT;
   return how==1?sge_strdup(NULL, filename):filename;
FPRINTF_ERROR:
   DEXIT;
   return NULL;
}

