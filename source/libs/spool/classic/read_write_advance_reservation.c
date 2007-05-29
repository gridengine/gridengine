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
#include "sge_cqueue.h"
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
#include "sge_strL.h"
#include "sge_jobL.h"

#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_mailrec.h"

static int intprt_resourcelist[] = { CE_name, CE_stringval, 0 };
static int intprt_mail_recipiants[] = { MR_user, MR_host, 0 };
static int intprt_range_list[] = { RN_min, RN_max, 0 };
static int intprt_queue[] = { JG_qname, JG_slots, 0 }; 
static int intprt_acl[] = { ARA_name, ARA_group, 0 }; 
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
   const char *s = NULL;

   DENTER(TOP_LAYER, "read_ar_work");

   opt[0] = NoName;

   /* --------- AR_id */
   if (!set_conf_ulong(alpp, clpp, fields, "id", ep, AR_id)) {
      DPRINTF(("Read AR, error read AR_id\n"));
      DRETURN(-1);
   }
   DPRINTF(("Read AR, id:                  "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_id))));

   /* --------- AR_name */
   if (!set_conf_string(alpp, clpp, fields, "name", ep, AR_name)) {
      DPRINTF(("Read AR, error read AR_name\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_name);
   s = lGetString(ep, AR_name);
   DPRINTF(("Read AR, name:                %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_account */
   if (!set_conf_string(alpp, clpp, fields, "account", ep, AR_account)) {
      DPRINTF(("Read AR, error read AR_account\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_account);
   s = lGetString(ep, AR_account);
   DPRINTF((    "Read AR, account:             %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_owner */
   if (!set_conf_string(alpp, clpp, fields, "owner", ep, AR_owner)) {
      DPRINTF(("Read AR, error read AR_owner\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_owner);
   s = lGetString(ep, AR_owner);
   DPRINTF((    "Read AR, owner:               %s\n", (s != NULL) ? s : "NONE"));


   /* --------- AR_group */
   if (!set_conf_string(alpp, clpp, fields, "group", ep, AR_group)) {
      DPRINTF(("Read AR, error read AR_group\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_group);
   s = lGetString(ep, AR_group);
   DPRINTF((    "Read AR, group:               %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_submission_time */
   if (!set_conf_ulong(alpp, clpp, fields, "submission_time", ep, AR_submission_time)) {
      DPRINTF(("Read AR, error read AR_submission_time\n"));
      DRETURN(-1);
   }
   DPRINTF(("Read AR, submission_time:     "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_submission_time))));


   /* --------- AR_start_time */
   if (!set_conf_ulong(alpp, clpp, fields, "start_time", ep, AR_start_time)) {
      DPRINTF(("Read AR, error read AR_start_time\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, start_time:          "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_start_time))));

   /* --------- AR_end_time */
   if (!set_conf_ulong(alpp, clpp, fields, "end_time", ep, AR_end_time)) {
      DPRINTF(("Read AR, error read AR_end_time\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, end_time:            "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_end_time))));

   /* --------- AR_duration */
   if (!set_conf_ulong(alpp, clpp, fields, "duration", ep, AR_duration)) {
      DPRINTF(("Read AR, error read AR_duration\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, duration:            "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_duration))));


   /* --------- AR_verify */
   if (!set_conf_ulong(alpp, clpp, fields, "verify", ep, AR_verify)) {
      DPRINTF(("Read AR, error read AR_verify\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, verify:              "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_verify))));

   /* --------- AR_error_handling */
   if (!set_conf_ulong(alpp, clpp, fields, "error_handling", ep, AR_error_handling)) {
      DPRINTF(("Read AR, error read AR_error_handling\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, error_handling:      "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_error_handling))));

   /* --------- AR_state */ 
   if (!set_conf_ulong(alpp, clpp, fields, "state", ep, AR_state)) {
      DPRINTF(("Read AR, error read AR_state"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, state:          "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_state))));

   /* --------- AR_checkpoint_name */
   if (!set_conf_string(alpp, clpp, fields, "checkpoint_name", ep, AR_checkpoint_name)) {
      DPRINTF(("Read AR, error read AR_checkpoint_name\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_checkpoint_name);
   s = lGetString(ep, AR_checkpoint_name);
   DPRINTF((    "Read AR, checkpoint_name:           %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_resource_list */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "resource_list",
                            ep, AR_resource_list, CE_Type, intprt_resourcelist)) {
         DPRINTF(("Read AR, error read default AR_resource_list\n"));
         DRETURN(-1);
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields?fields:opt, "resource_list",
                         ep, AR_resource_list, CE_Type, CE_name)) {
         DPRINTF(("Read AR, error read AR_resource_list\n"));
         DRETURN(-1);
      }
   }
   { /* restore double value and complex tzype */
      object_description *object_base = object_type_get_object_description();
      lList *master_centry_list = *object_base[SGE_TYPE_CENTRY].list;

      
      if (centry_list_fill_request(lGetList(ep, AR_resource_list), alpp, 
          master_centry_list, false, true, false)) {
          DRETURN(-1);
      }
   }
 
   /* --------- AR_resource_utilization NOSPOOL */

   /* --------- AR_queue_list */
   if (!set_conf_list(alpp, clpp, fields, "queue_list", ep, AR_queue_list,
                      QR_Type, QR_name)) {
      DPRINTF(("Read AR, error read AR_queue_list\n"));
      DRETURN(-1);
   }
   
   /* --------- AR_granted_slots */
   if (!set_conf_deflist(alpp, clpp, fields, "granted_slots",
                  ep,  AR_granted_slots, JG_Type, intprt_queue)){
      DPRINTF(("Read AR, error read AR_granted_slots\n"));
      DRETURN(-1);
                   
   }
   DPRINTF((    "granted_slots:\n"));
   {  /* extract the host name from qname */
      lListElem *jg;
      dstring cqueue_buffer = DSTRING_INIT;
      dstring hostname_buffer = DSTRING_INIT;
      for_each(jg, lGetList(ep, AR_granted_slots)){
         const char *hostname = NULL;
         bool has_hostname = false;
         bool has_domain = false;

         s =  lGetString(jg, JG_qname);
         cqueue_name_split(s, &cqueue_buffer, &hostname_buffer,
                           &has_hostname, &has_domain);
         hostname = sge_dstring_get_string(&hostname_buffer);
         lSetHost(jg, JG_qhostname, hostname);
      }
      sge_dstring_free(&cqueue_buffer);
      sge_dstring_free(&hostname_buffer);
         
   } /* end of granted_slots */
   
   /* --------- AR_mail_options */
   if (!set_conf_ulong(alpp, clpp, fields, "mail_options", ep, AR_mail_options)) {
      DPRINTF(("Read AR, error read AR_mail_options\n"));
      DRETURN(-1);
   }

   /* --------- AR_mail_list */
   if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "mail_list",
                         ep, AR_mail_list, MR_Type, intprt_mail_recipiants)) {
      DPRINTF(("Read AR, error read AR_mail_list\n"));
      DRETURN(-1);
   }
   {
      lListElem *mail;
      const char * s;
      DPRINTF(("Read AR, mail_list\n"));                      
      for_each(mail, lGetList(ep, AR_mail_list)) {
         NULL_OUT_NONE(mail, MR_user);
         s = lGetHost(mail, MR_host);
         if ((s != NULL) && (strcasecmp(s, "none") == 0)) { 
            lSetHost(mail, MR_host, NULL); 
         }
      }
   }
   /* --------- AR_pe */
   if (!set_conf_string(alpp, clpp, fields, "pe", ep, AR_pe)) {
      DPRINTF(("Read AR, error read AR_pe\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_pe);
   s = lGetString(ep, AR_pe);
   DPRINTF((    "Read AR, pe                   %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_pe_range */
   if (!set_conf_deflist(alpp, clpp, fields?fields:opt, "pe_range",
                      ep, AR_pe_range, RN_Type, intprt_range_list)) {
      DPRINTF(("Read AR, error default read AR_pe_range\n"));
      DRETURN(-1);
   } 
   
   /* --------- AR_granted_pe */
   if (!set_conf_string(alpp, clpp, fields, "granted_pe", ep, AR_granted_pe)) {
      DPRINTF(("Read AR, error read AR_granted_pe\n"));
      DRETURN(-1);
   }
   NULL_OUT_NONE(ep, AR_granted_pe);
   s = lGetString(ep, AR_granted_pe);
   DPRINTF((    "Read AR, granted_pe                   %s\n", (s != NULL) ? s : "NONE"));

   
   /* --------- AR_master_queue_list */
   if (!set_conf_list(alpp, clpp, fields?fields:opt, "master_queue_list", ep,
                      AR_master_queue_list, QR_Type, QR_name)) {
      DPRINTF(("Read AR, error read AR_master_queue_list\n"));
      DRETURN(-1);
   }

     
   /* --------- AR_acl_list */
   if (!set_conf_deflist(alpp, clpp, fields, "acl_list",
                         ep, AR_acl_list, ARA_Type, intprt_acl)) {
      DPRINTF(("Read AR, error read AR_acl_list\n"));
      DRETURN(-1);
   }

   /* --------- AR_xacl_list  */
   if (!set_conf_deflist(alpp, clpp, fields, "xacl_list",
                         ep, AR_xacl_list, ARA_Type, intprt_acl)) {
      DPRINTF(("Read AR, error read AR_xacl_list\n"));
      DRETURN(-1);
   }

   /* --------- AR_type */
   if (!set_conf_ulong(alpp, clpp, fields, "type", ep, AR_type)) {
      DPRINTF(("Read AR, error read AR_type\n"));
      DRETURN(-1);
   }
   DPRINTF((    "Read AR, type                 "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_type))));

   DRETURN(0);
}

/****** read_write_advance_reservation/write_ar() ******************************
*  NAME
*     write_ar() --  write an advance reservation
*
* SYNOPSIS
*     char * write_ar(int spool, int how, const lListElem *ep) 
*
*  FUNCTION
*     write an advance reservation
*
*  INPUTS
*     int spool           -  1 write for spooling
*                            0 write only user controlled fields
*     int how             -  0 use stdout
*                            1 write into tmpfile
*                            2 write into spoolfile
*     const lListElem *ep -  object pointer of the AR 
*
*  RESULT
*     char * a character stream on success, NULL otherwise 
*
*  NOTES
*     MT-NOTE: write_ar() is not MT safe 
*
*******************************************************************************/
char *write_ar(int spool, int how, const lListElem *ep)
{
   FILE *fp;
   const char *s = NULL;
   const char *delis[] = {"=", ",", NULL};
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
   DPRINTF((    "id:                  "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_id))));
   FPRINTF((fp, "id                   "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_id))));

   /* --------- AR_name */
   s = lGetString(ep, AR_name);
   DPRINTF((    "name:                %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "name                 %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_account */
   s = lGetString(ep, AR_account);
   DPRINTF((    "account:             %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "account              %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_owner */
   s = lGetString(ep, AR_owner);
   DPRINTF((    "owner:               %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "owner                %s\n", (s != NULL) ? s : "NONE"));
   
   /* --------- AR_group */
   s = lGetString(ep, AR_group);
   DPRINTF((    "group:               %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "group                %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_submission_time */
   DPRINTF((    "submission_time:     "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_submission_time))));
   FPRINTF((fp, "submission_time      "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_submission_time))));

   /* --------- AR_start_time */
   DPRINTF((    "start_time:          "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_start_time))));
   FPRINTF((fp, "start_time           "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_start_time))));

   /* --------- AR_end_time */
   DPRINTF((    "end_time:            "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_end_time))));
   FPRINTF((fp, "end_time             "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_end_time))));

   /* --------- AR_duration */
   DPRINTF((    "duration:            "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_duration))));
   FPRINTF((fp, "duration             "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_duration))));

   /* --------- AR_verify */
   DPRINTF((    "verify:              "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_verify))));
   FPRINTF((fp, "verify               "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_verify))));

   /* --------- AR_error_handling */
   DPRINTF((    "error_handling:      "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_error_handling))));
   FPRINTF((fp, "error_handling       "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_error_handling))));

   /* --------- AR_state */
   DPRINTF((    "state:               "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_state))));
   FPRINTF((fp, "state                "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_state))));
   
   /* --------- AR_checkpoint_name */
   s = lGetString(ep, AR_checkpoint_name);
   DPRINTF((    "checkpoint_name:     %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "checkpoint_name      %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_resource_list */
   DPRINTF((             "resource_list:      \n"));
   fprint_thresholds(fp, "resource_list        ", lGetList(ep, AR_resource_list), 1);
   
   /* --------- AR_resource_utilization NOSPOOL */
   
   /* --------- AR_queue_list */
   DPRINTF((    "queue_list:        \n"));
   FPRINTF((fp, "queue_list           "));
   sep = lFirst(lGetList(ep, AR_queue_list));
   if (sep) {
      do {
         DPRINTF((    "%s", lGetString(sep, QR_name)));
         FPRINTF((fp, "%s", lGetString(sep, QR_name)));
         sep = lNext(sep);
         if (sep) { 
            DPRINTF((    " "));
            FPRINTF((fp, " "));
         }
      } while (sep);
      DPRINTF((    "\n"));
      FPRINTF((fp, "\n"));
   } else {
      DPRINTF((    "NONE\n"));
      FPRINTF((fp, "NONE\n"));
   }  

   /* --------- AR_granted_slots */
   DPRINTF((    "granted_slots:       "));
   FPRINTF((fp, "granted_slots        ")); 
   sep = lFirst(lGetList(ep, AR_granted_slots));
   if (sep) {
      do {
         const char *qname;
         lUlong slots;
        
         qname = lGetString(sep, JG_qname);
         slots = lGetUlong(sep, JG_slots);
      
         /* --------------- slots ------------------ */
         DPRINTF((    "%s="sge_U32CFormat, qname, sge_u32c(slots)));
         FPRINTF((fp, "%s="sge_U32CFormat, qname, sge_u32c(slots)));

         sep = lNext(sep);
         if (sep) { 
            DPRINTF((    ","));
            FPRINTF((fp, ","));
         }
      } while (sep);
      DPRINTF((    "\n"));
      FPRINTF((fp, "\n"));
   } else {
      DPRINTF((    "NONE\n"));
      FPRINTF((fp, "NONE\n"));
   }  

   /* --------- AR_mail_options */
   DPRINTF((    "mail_options:        "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_mail_options))));
   FPRINTF((fp, "mail_options         "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_mail_options))));

   /* --------- AR_mail_list */
   DPRINTF((    "mail_list:           "));
   FPRINTF((fp, "mail_list            "));
   sep = lFirst(lGetList(ep, AR_mail_list));
   if (sep) {
      do {
         const char *user = NULL;
         const char *host = NULL;

         user=lGetString(sep, MR_user);
         host=lGetHost(sep, MR_host); 
         DPRINTF((    "%s %s", user, (host != NULL) ? host : "NONE"));
         FPRINTF((fp, "%s %s", user, (host != NULL) ? host : "NONE"));
         sep = lNext(sep);
         if (sep) { 
            DPRINTF((    ",\n"));
            FPRINTF((fp, ","));
         }
      } while (sep);
      DPRINTF((    "\n"));
      FPRINTF((fp, "\n"));
   } else {
      DPRINTF((    "NONE\n"));
      FPRINTF((fp, "NONE\n"));
   }  

   /* --------- AR_pe */
   s = lGetString(ep, AR_pe);
   DPRINTF((    "pe                   %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "pe                   %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_pe_range */
   DPRINTF((    "pe_range \n"));
   fprint_range_list(fp, "pe_range             ", lGetList(ep, AR_pe_range));

   /* --------- AR_granted_pe */
   s = lGetString(ep, AR_granted_pe);
   DPRINTF((    "granted_pe           %s\n", (s != NULL) ? s : "NONE"));
   FPRINTF((fp, "granted_pe           %s\n", (s != NULL) ? s : "NONE"));

   /* --------- AR_master_queue_list */
   DPRINTF((    "master_queue_list \n"));
   lret = fprint_cull_list(fp,  "master_queue_list    ", lGetList(ep, AR_master_queue_list), QR_name);
   if (lret == -1) {
      goto FPRINTF_ERROR;
   }

   /* --------- AR_acl_list */
   DPRINTF((    "acl_list \n"));
   FPRINTF((fp, "acl_list             "));
   if (uni_print_list(fp, NULL, 0, lGetList(ep, AR_acl_list), intprt_acl, delis, 0) < 0) {
      goto FPRINTF_ERROR;
   }
   FPRINTF((fp, "\n"));

   /* --------- AR_xacl_list */
   DPRINTF((    "xacl_list \n"));
   FPRINTF((fp, "xacl_list            "));
   if (uni_print_list(fp, NULL, 0, lGetList(ep, AR_xacl_list), intprt_acl, delis, 0) < 0) {
      goto FPRINTF_ERROR;
   }
   FPRINTF((fp, "\n"));

   /* --------- AR_type */
   DPRINTF((    "type                 "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_type))));
   FPRINTF((fp, "type                 "sge_U32CFormat"\n", sge_u32c(lGetUlong(ep, AR_type))));

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
