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

#include "sge.h"
#include "cull.h"
#include "sge_ckptL.h"
#include "sge_peL.h"         
#include "sge_confL.h"
#include "config.h"
#include "sge_answerL.h"
#include "read_write_ckpt.h"
#include "sge_tmpnam.h"
#include "read_object.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "sge_feature.h"

/****
 **** cull_read_in_ckpt
 ****/
lListElem *cull_read_in_ckpt(
const char *dirname,
const char *filename,
int spool,
int flag,
int *tag,
int fields[] 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   lListElem *ep;
   struct read_object_args args = { CK_Type, "ckpt", read_ckpt_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_ckpt");

   ep = read_object(dirname, filename, spool, 0,0, &args, tag?tag:&intern_tag, fields);
  
   DEXIT;
   return ep;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}


/* ------------------------------------------------------------

   read_ckpt_work - read ckpt objekt elem from configuration

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
int read_ckpt_work(
lList **alpp,
lList **clpp,
int fields[],
lListElem *ep,
int spool,
int flag,
int *tag,
int parsing_type  
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   DENTER(TOP_LAYER, "read_ckpt_work");

   /* --------- CK_name */
   if (!set_conf_string(alpp, clpp, fields, "ckpt_name", ep, CK_name)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_interface */
   if (!set_conf_string(alpp, clpp, fields, "interface", ep, CK_interface)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_ckpt_command */
   if (!set_conf_string(alpp, clpp, fields, "ckpt_command", ep, CK_ckpt_command)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_migr_command */
   if (!set_conf_string(alpp, clpp, fields, "migr_command", ep, CK_migr_command)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_rest_command */
   if (!set_conf_string(alpp, clpp, fields, "restart_command", ep, CK_rest_command)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_clean_command */
   if (!set_conf_string(alpp, clpp, fields, "clean_command", ep, CK_clean_command)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_ckpt_dir */
   if (!set_conf_string(alpp, clpp, fields, "ckpt_dir", ep, CK_ckpt_dir)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_queue_list */
   if (!set_conf_list(alpp, clpp, fields, "queue_list", ep, CK_queue_list, 
                        QR_Type, QR_name)) {
      DEXIT;
      return -1;
   }
   
   /* --------- CK_signal */
   if (!set_conf_string(alpp, clpp, fields, "signal", ep, CK_signal)) {
      DEXIT;
      return -1;
   }

   /* --------- CK_when */
   if (!set_conf_string(alpp, clpp, fields, "when", ep, CK_when)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}


/* ------------------------------------------------------------

   write_ckpt - write 3rd argument ckptobj element to file

   returns tmpfile name in case of creating a tempfile

   spool:
      1 write for spooling
      0 write only user controlled fields

   how:
      0 use stdout
      1 write into tmpfile
      2 write into spoolfile

*/
char *write_ckpt(
int spool,
int how,
lListElem *ep 
) {
   FILE *fp;
   lListElem *sep;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_ckpt");

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
         sprintf(filename, "%s/.%s", CKPTOBJ_DIR, lGetString(ep, CK_name));
         sprintf(real_filename, "%s/%s", CKPTOBJ_DIR, lGetString(ep, CK_name));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_S, filename));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   } 

   /* --------- CK_name */
   FPRINTF((fp, "ckpt_name          %s\n", lGetString(ep, CK_name)));

   /* --------- CK_interface */
   FPRINTF((fp, "interface          %s\n", lGetString(ep, CK_interface)));

   /* --------- CK_ckpt_command */
   FPRINTF((fp, "ckpt_command       %s\n", lGetString(ep, CK_ckpt_command)));

   /* --------- CK_migr_command */
   FPRINTF((fp, "migr_command       %s\n", lGetString(ep, CK_migr_command)));

   /* --------- CK_rest_command */
   FPRINTF((fp, "restart_command    %s\n", lGetString(ep, CK_rest_command)));

   /* --------- CK_clean_command */
   FPRINTF((fp, "clean_command      %s\n", lGetString(ep, CK_clean_command)));

   /* --------- CK_ckpt_dir */
   FPRINTF((fp, "ckpt_dir           %s\n", lGetString(ep, CK_ckpt_dir)));

   /* --------- CK_queue_list */
   FPRINTF((fp, "queue_list         "));
   sep = lFirst(lGetList(ep, CK_queue_list));
   if (sep) {
      do {
         FPRINTF((fp, "%s", lGetString(sep, QR_name)));
         sep = lNext(sep);
         if (sep) 
             FPRINTF((fp, " "));
      } while (sep);
      FPRINTF((fp, "\n"));
   } else {
      FPRINTF((fp, "NONE\n"));
   }

   /* --------- CK_signal */
   FPRINTF((fp, "signal             %s\n", lGetString(ep, CK_signal)));

   FPRINTF((fp, "when               %s\n", lGetString(ep, CK_when)));
   if (how!=0)
      fclose(fp);

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

/***f** src/sge_generic_ckpt() **********************************************
*
*  NAME
*     sge_generic_ckpt -- build up a generic ckpt object
*
*  SYNOPSIS
*     lListElem* sge_generic_ckpt(
*        char *ckpt_name
*     );
*
*  FUNCTION
*     build up a generic ckpt object
*
*  INPUTS
*     ckpt_name - name used for the CK_name attribute of the generic
*               pe object. If NULL then "template" is the default name.
*
*  RESULT
*     !NULL - Pointer to a new CULL object of type CK_Type
*     NULL - Error
*
*  EXAMPLE
*
*  NOTES
*
*  BUGS
*
*  SEE ALSO
*
*****************************************************************************/   
lListElem* sge_generic_ckpt(
char *ckpt_name 
) {
   lListElem *ep;

   DENTER(TOP_LAYER, "sge_generic_ckpt");

   ep = lCreateElem(CK_Type);

   if (ckpt_name)
      lSetString(ep, CK_name, ckpt_name);
   else
      lSetString(ep, CK_name, "template");

   lSetString(ep, CK_interface, "userdefined");
   lSetString(ep, CK_ckpt_command, "none");
   lSetString(ep, CK_migr_command, "none");
   lSetString(ep, CK_rest_command, "none");
   lSetString(ep, CK_clean_command, "none");
   lSetString(ep, CK_ckpt_dir, "/tmp");
   lSetString(ep, CK_when, "sx");
   lSetString(ep, CK_signal, "none");
   lSetUlong(ep, CK_job_pid, 0);

   /* use the keyword "all" for the "queue_list" attribute */
   {
      lList *new_qr_list;
      lListElem *new_qr;

      new_qr_list = lCreateList("", QR_Type);
      new_qr = lCreateElem(QR_Type);

      lSetString(new_qr, QR_name, SGE_ATTRVAL_ALL);
      lAppendElem(new_qr_list, new_qr);

      lSetList(ep, CK_queue_list, new_qr_list);
   }             

   DEXIT;
   return ep;
}
