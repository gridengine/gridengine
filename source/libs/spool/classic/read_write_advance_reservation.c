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
#include <unistd.h>

#include "sge.h"
#include "sge_advance_reservation.h"
#include "sge_answer.h"
#include "read_write_advance_reservation.h"
#include "sge_string.h"
#include "sge_log.h"
#include "config.h"
#include "read_object.h"
#include "sgermon.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "spool/classic/msg_spoollib_classic.h"
#include "sge_feature.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sge_userset.h"
#include "sge_resource_utilization.h"
#include "sge_qinstance.h"
#include "sge_qref.h"

#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_mailrec.h"

static int intprt_as_load_thresholds[] = { CE_name, CE_stringval, 0 };
static int intprt_as_mail_recipiants[] = { MR_user, MR_host, 0 };
static int intprt_as_range_list[] = { RN_min, RN_max, RN_step, 0 };

lListElem *
cull_read_in_ar(const char *dirname, const char *filename, int spool,
                int type, int *tag, int fields[])
{
   lListElem *ep;
   struct read_object_args args = { AR_Type, "ar", read_ar_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_ar");

   ep = read_object(dirname, filename, spool, 0, 0,&args, tag?tag:&intern_tag, fields);
 
   DRETURN(ep); 
}


/* ------------------------------------------------------------

   read_ar_work()

   spool:
      1 write for spooling
      0 write only user controlled fields

*/
int 
read_ar_work(lList **alpp, lList **clpp, int fields[], lListElem *ep, 
             int spool, int flag, int *tag, int parsing_type) 
{
   int opt[10];
   DENTER(TOP_LAYER, "read_ar_work");

   opt[0] = NoName;

   /* --------- AR_id */
   if (!set_conf_ulong(alpp, clpp, fields, "id", ep, AR_id)) {
      DRETURN(-1);
   }

   /* --------- AR_name */
   if (!set_conf_string(alpp, clpp, fields, "name", ep, AR_name)) {
      DRETURN(-1);
   }

   /* --------- AR_account */
   if (!set_conf_string(alpp, clpp, fields, "account", ep, AR_account)) {
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_account);

   /* --------- AR_owner */
   if (!set_conf_string(alpp, clpp, fields, "owner", ep, AR_owner)) {
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_owner);

   /* --------- AR_submission_time */
   if (!set_conf_ulong(alpp, clpp, fields, "submission_time", ep, AR_submission_time)) {
      DRETURN(-1);
   }

   /* --------- AR_start_time */
   if (!set_conf_ulong(alpp, clpp, fields, "start_time", ep, AR_start_time)) {
      DRETURN(-1);
   }

   /* --------- AR_end_time */
   if (!set_conf_ulong(alpp, clpp, fields, "end_time", ep, AR_end_time)) {
      DRETURN(-1);
   }

   /* --------- AR_duration */
   if (!set_conf_ulong(alpp, clpp, fields, "duration", ep, AR_duration)) {
      DRETURN(-1);
   }

   /* --------- AR_verify */
   if (!set_conf_ulong(alpp, clpp, fields, "verify", ep, AR_verify)) {
      DRETURN(-1);
   }

   /* --------- AR_error_handling */
   if (!set_conf_ulong(alpp, clpp, fields, "error_handling", ep, AR_error_handling)) {
      DRETURN(-1);
   }

   /* --------- AR_state */
   if (!set_conf_ulong(alpp, clpp, fields, "state", ep, AR_state)) {
      DRETURN(-1);
   }

   /* --------- AR_checkpoint_name */
   if (!set_conf_string(alpp, clpp, fields, "ckpt_name", ep, AR_checkpoint_name)) {
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_checkpoint_name);

   /* --------- AR_resource_list */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "resource_list",
                            ep, AR_resource_list, CE_Type, intprt_as_load_thresholds)) {
         DRETURN(-1);
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "resource_list",
                         ep, AR_resource_list, CE_Type, CE_name)) {
         DRETURN(-1);
      }
   }

   /* --------- AR_queue_list */
   if (!set_conf_list(alpp, clpp, fields, "queue_list", ep, AR_queue_list,
                      QR_Type, QR_name)) {
      DRETURN(-1);
   }

   /* --------- AR_mail_options */
   if (!set_conf_ulong(alpp, clpp, fields, "mail_options", ep, AR_mail_options)) {
      DRETURN(-1);
   }

   /* --------- AR_mail_list */
   if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "mail_list",
                         ep, AR_resource_list, MR_Type, intprt_as_mail_recipiants)) {
      DRETURN(-1);
   }

   /* --------- AR_pe */
   if (!set_conf_string(alpp, clpp, fields, "pe", ep, AR_checkpoint_name)) {
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_checkpoint_name);

   /* --------- AR_pe_range */
   if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "pe_range",
                         ep, AR_pe_range, RN_Type, intprt_as_range_list)) {
      DRETURN(-1);
   }

   /* --------- AR_acl_list  */
   if (!set_conf_list(alpp, clpp, fields?fields:opt, "user_lists", ep,
                      AR_acl_list, US_Type, US_name)) {
      DRETURN(-1);
   }

   /* --------- AR_xacl_list  */
   if (!set_conf_list(alpp, clpp, fields?fields:opt, "xuser_lists", ep,
                      AR_xacl_list, US_Type, US_name)) {
      DRETURN(-1);
   }

   /* --------- AR_type */
   if (!set_conf_ulong(alpp, clpp, fields, "type", ep, AR_type)) {
      DRETURN(-1);
   }

   DRETURN(0);
}

/* ------------------------------------------------------------

   returns tmpfile name in case of creating a tempfile

   spool:
      1 write for spooling
      0 write only user controlled fields

   how:
      0 use stdout
      1 write into tmpfile
      2 write into spoolfile

*/         

char *
write_ar(int spool, int how, const lListElem *ep)
{
   FILE *fp;
   const char *s = NULL;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX];
   dstring ds;
   char buffer[256];
   lListElem *sep;
   int lret;

   DENTER(TOP_LAYER, "write_ar");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         if (!sge_tmpnam(filename)) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            DRETURN(NULL);
         }
      } else {
         sprintf(filename, "%s/."sge_U32CFormat, AR_DIR, 
                 sge_u32c(lGetUlong(ep, AR_id)));
         sprintf(real_filename, "%s/"sge_U32CFormat, AR_DIR, 
                 sge_u32c(lGetUlong(ep, AR_id)));
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_ERRORWRITINGFILE_SS, 
                   filename, strerror(errno)));
         DRETURN(NULL);
      }
      break;
   default:
      DRETURN(NULL);
   }

   if (spool && sge_spoolmsg_write(fp, COMMENT_CHAR, 
         feature_get_product_name(FS_VERSION, &ds)) < 0) {
      goto FPRINTF_ERROR;
   }

   /* --------- AR_id */
   FPRINTF((fp, "id                   "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_id))));

   /* --------- AR_name */
   FPRINTF((fp, "name                 %s\n", lGetString(ep, AR_name)));

   /* --------- AR_account */
   s = lGetString(ep, AR_account);
   FPRINTF((fp, "account              %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_owner */
   s = lGetString(ep, AR_owner);
   FPRINTF((fp, "owner                %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_submission_time */
   FPRINTF((fp, "submission_time      "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_submission_time))));

   /* --------- AR_start_time */
   FPRINTF((fp, "start_time           "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_start_time))));

   /* --------- AR_end_time */
   FPRINTF((fp, "end_time             "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_end_time))));

   /* --------- AR_duration */
   FPRINTF((fp, "duration             "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_duration))));

   /* --------- AR_verify */
   FPRINTF((fp, "verify               "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_verify))));

   /* --------- AR_error_handling */
   FPRINTF((fp, "error_handling       "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_error_handling))));

   /* --------- AR_state */
   FPRINTF((fp, "state                "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_state))));

   /* --------- AR_checkpoint_name */
   s = lGetString(ep, AR_checkpoint_name);
   FPRINTF((fp, "ckpt_name            %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_resource_list */
   fprint_thresholds(fp, "resource_list        ", 
                     lGetList(ep, AR_resource_list), 1);
   
   /* --------- AR_queue_list */
   FPRINTF((fp, "queue_list           "));
   sep = lFirst(lGetList(ep, AR_queue_list));
   if (sep) {
      do {
         FPRINTF((fp, "%s", lGetString(sep, QR_name)));
         sep = lNext(sep);
         if (sep) { 
            FPRINTF((fp, " "));
         }
      } while (sep);
      FPRINTF((fp, "\n"));
   } else {
      FPRINTF((fp, "NONE\n"));
   }  

   /* --------- AR_mail_options */
   FPRINTF((fp, "mail_options         "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_mail_options))));

   /* --------- AR_mail_list */
   FPRINTF((fp, "mail_list            "));
   sep = lFirst(lGetList(ep, AR_mail_list));
   if (sep) {
      do {
         FPRINTF((fp, "%s %s", lGetString(sep, MR_user), lGetString(sep, MR_host)));
         sep = lNext(sep);
         if (sep) { 
            FPRINTF((fp, ","));
         }
      } while (sep);
      FPRINTF((fp, "\n"));
   } else {
      FPRINTF((fp, "NONE\n"));
   }  

   /* --------- AR_pe */
   s = lGetString(ep, AR_pe);
   FPRINTF((fp, "pe                   %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_pe_range */
   fprint_range_list(fp, "pe_range             ", lGetList(ep, AR_pe_range));

   /* --------- AR_acl_list */
   lret = fprint_cull_list(fp,  "user_lists           ",
                           lGetList(ep, AR_acl_list), US_name);
   if (lret == -1) {
      goto FPRINTF_ERROR;
   }

   /* --------- AR_xacl_list */
   lret = fprint_cull_list(fp,  "xuser_lists          ",
                           lGetList(ep, AR_xacl_list), US_name);
   if (lret == -1) {
      goto FPRINTF_ERROR;
   }

   /* --------- AR_type */
   FPRINTF((fp, "type                 "sge_U32CFormat"\n", 
            sge_u32c(lGetUlong(ep, AR_type))));

   if (how!=0) {
      FCLOSE(fp);
   }

   if (how == 2) {
      if (rename(filename, real_filename) == -1) {
         DRETURN(NULL);
      } else {
         strcpy(filename, real_filename);
      }
   }   
   DRETURN(how==1?sge_strdup(NULL, filename):filename);
FPRINTF_ERROR:
FCLOSE_ERROR:
   DRETURN(NULL);
}
