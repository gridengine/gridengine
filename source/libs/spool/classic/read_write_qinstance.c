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
#include "sge_answer.h"
#include "read_write_qinstance.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sge_stdio.h"
#include "sge_io.h"
#include "sge_conf.h"
#include "sge_attr.h"
#include "sge_feature.h"
#include "sge_href.h"
#include "sge_qinstance.h"

#include "msg_common.h"

lListElem *cull_read_in_qinstance(const char *dirname, const char *filename, 
                                  int spool, int flag, int *tag, int fields[]) 
{  
   lListElem *ep;
   struct read_object_args args = { QI_Type, "", read_qinstance_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_qinstance");

   ep = read_object(dirname, filename, spool, 0, 0,&args, 
                    tag?tag:&intern_tag, NULL);
  
   DEXIT;
   return ep;
}

/* ------------------------------------------------------------
   spool:
      1 write for spooling
      0 write only user controlled fields

*/
int read_qinstance_work(
lList **alpp,   /* anser list */
lList **clpp,   /* parsed file */
int fields[],   /* not needed */
lListElem *ep,  /* list element to fill of type CU_Type */
int spool,      /* look above */
int flag,       /* user flag */
int *tag,       /* user return value */
int parsing_type 
) {
   int ret = 0;

   DENTER(TOP_LAYER, "read_cqueue_work");

   /* --------- QI_name */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "qname", ep, QI_name)) ? -1 : 0;
   }

   /* --------- QI_hostlist */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "hostname", ep, QI_hostname)) ? -1 : 0;
   }

   /* --------- QI_seq_no */
   if (ret == 0) {
      ret = (!set_conf_ulong(alpp, clpp, fields, "seq_no", ep, QI_seq_no)) ? -1 : 0;
   }
   
   DEXIT;
   return ret;
}

char *
write_qinstance(int spool, int how, const lListElem *ep) 
{
   FILE *fp;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_cqueue");
 
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
      } else {
         sprintf(filename, "%s/.%s@%s", QINSTANCES_DIR, lGetString(ep, QI_name), lGetHost(ep, QI_hostname));
         sprintf(real_filename, "%s/%s@%s", QINSTANCES_DIR, lGetString(ep, QI_name), lGetHost(ep, QI_hostname));
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

   FPRINTF((fp, "qname              %s\n", lGetString(ep, QI_name))); 
   FPRINTF((fp, "hostname           %s\n", lGetHost(ep, QI_hostname))); 
   FPRINTF((fp, "seq_no             %d\n", (int) lGetUlong(ep, QI_seq_no)));

   if (how != 0) {
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


