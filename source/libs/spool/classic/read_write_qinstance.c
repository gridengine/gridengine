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
#include "sge_str.h"
#include "sge_dstring.h"
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
#include "sge_centry.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_str.h"

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

   DENTER(TOP_LAYER, "read_qinstance_work");

   /* --------- QI_name */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "qname", ep, QI_name)) ? -1 : 0;
   }

   /* --------- QI_hostlist */
   if (ret == 0) {
      ret = (!set_conf_string(alpp, clpp, fields, "hostname", ep, QI_hostname)) ? -1 : 0;
   }

   DEXIT;
   return ret;
}

char *
write_qinstance(int spool, int how, const lListElem *ep, FILE *fp1) 
{
   FILE *fp;
   char filename[SGE_PATH_MAX];

   DENTER(TOP_LAYER, "write_cqueue");
 
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
      if (!sge_tmpnam(filename)) {
         CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
         DEXIT;
         return NULL;
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, filename, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   case 4:
      fp = fp1;
      break;
   default:
      DEXIT;
      return NULL;
   }

   if (how == 0 || how == 4) {
      FPRINTF((fp, "qname              %s\n", lGetString(ep, QI_name)));
      FPRINTF((fp, "hostname           %s\n", lGetHost(ep, QI_hostname)));
   }
   if (how == 0) {
      {
         FPRINTF((fp, "seq_no             %d\n", 
                  (int) lGetUlong(ep, QI_seq_no)));
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_load_thresholds);

         centry_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "load_thresholds    %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_suspend_thresholds);

         centry_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "suspend_thresholds %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         FPRINTF((fp, "nsuspend           %d\n",
                  (int) lGetUlong(ep, QI_nsuspend)));
      }
      {
         FPRINTF((fp, "suspend_interval   %s\n",
                  lGetString(ep, QI_suspend_interval)));
      }
      {
         FPRINTF((fp, "priority           %s\n",
                  lGetString(ep, QI_suspend_interval)));
      }
      {
         FPRINTF((fp, "min_cpu_interval   %s\n",
                  lGetString(ep, QI_min_cpu_interval)));
      }
      {
         FPRINTF((fp, "processors         %s\n",
                  lGetString(ep, QI_processors)));
      }
      {
         dstring tmp_string = DSTRING_INIT;
         u_long32 qtype = lGetUlong(ep, QI_qtype);

         qtype_append_to_dstring(qtype, &tmp_string);
         FPRINTF((fp, "qtype              %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_ckpt_list);

         str_list_append_to_dstring(list, &tmp_string, ' ');
         FPRINTF((fp, "ckpt_list          %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_pe_list);

         str_list_append_to_dstring(list, &tmp_string, ' ');
         FPRINTF((fp, "pe_list            %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         FPRINTF((fp, "rerun              %s\n",
                  lGetBool(ep, QI_rerun) ? "TRUE" : "FALSE"));
      }
      {
         FPRINTF((fp, "slots              "u32"\n",
                  lGetUlong(ep, QI_job_slots)));
      }
      {
         FPRINTF((fp, "tmpdir             %s\n",
                  lGetString(ep, QI_tmpdir)));
      }
      {
         FPRINTF((fp, "shell              %s\n",
                  lGetString(ep, QI_shell)));
      }
      {
         FPRINTF((fp, "prolog             %s\n",
                  lGetString(ep, QI_prolog)));
      }
      {
         FPRINTF((fp, "epilog             %s\n",
                  lGetString(ep, QI_epilog)));
      }
      {
         FPRINTF((fp, "shell_start_mode   %s\n",
                  lGetString(ep, QI_shell_start_mode)));
      }
      {
         FPRINTF((fp, "starter_method     %s\n",
                  lGetString(ep, QI_starter_method)));
      }
      {
         FPRINTF((fp, "suspend_method     %s\n",
                  lGetString(ep, QI_suspend_method)));
      }
      {
         FPRINTF((fp, "resume_method      %s\n",
                  lGetString(ep, QI_resume_method)));
      }
      {
         FPRINTF((fp, "terminate_method   %s\n",
                  lGetString(ep, QI_terminate_method)));
      }
      {
         FPRINTF((fp, "notify             %s\n",
                  lGetString(ep, QI_notify)));
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_owner_list);

         userset_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "owner_list         %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_acl);

         userset_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "user_lists         %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_xacl);

         userset_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "xuser_lists        %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_subordinate_list);

         so_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "subordinate_list   %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_consumable_config_list);

         centry_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "complex_values     %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_projects);

         userprj_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "projects           %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
         dstring tmp_string = DSTRING_INIT;
         const lList *list = lGetList(ep, QI_xprojects);

         userprj_list_append_to_dstring(list, &tmp_string);
         FPRINTF((fp, "xprojects          %s\n", 
                  sge_dstring_get_string(&tmp_string)));
         sge_dstring_free(&tmp_string);
      }
      {
         FPRINTF((fp, "calendar           %s\n",
                  lGetString(ep, QI_calendar)));
      }
      {
         FPRINTF((fp, "initial_state      %s\n",
                  lGetString(ep, QI_initial_state)));
      }
      if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
         FPRINTF((fp, "fshare             "u32"\n",
                  lGetUlong(ep, QI_fshare)));
         FPRINTF((fp, "oticket            "u32"\n",
                  lGetUlong(ep, QI_oticket)));
      }
      {
         FPRINTF((fp, "s_rt               %s\n", lGetString(ep, QI_s_rt)));
         FPRINTF((fp, "h_rt               %s\n", lGetString(ep, QI_h_rt)));
         FPRINTF((fp, "s_cpu              %s\n", lGetString(ep, QI_s_cpu)));
         FPRINTF((fp, "h_cpu              %s\n", lGetString(ep, QI_h_cpu)));
         FPRINTF((fp, "s_fsize            %s\n", lGetString(ep, QI_s_fsize)));
         FPRINTF((fp, "h_fsize            %s\n", lGetString(ep, QI_h_fsize)));
         FPRINTF((fp, "s_data             %s\n", lGetString(ep, QI_s_data)));
         FPRINTF((fp, "h_data             %s\n", lGetString(ep, QI_h_data)));
         FPRINTF((fp, "s_stack            %s\n", lGetString(ep, QI_s_stack)));
         FPRINTF((fp, "h_stack            %s\n", lGetString(ep, QI_h_stack)));
         FPRINTF((fp, "s_core             %s\n", lGetString(ep, QI_s_core)));
         FPRINTF((fp, "h_core             %s\n", lGetString(ep, QI_h_core)));
         FPRINTF((fp, "s_rss              %s\n", lGetString(ep, QI_s_rss)));
         FPRINTF((fp, "h_rss              %s\n", lGetString(ep, QI_h_rss)));
         FPRINTF((fp, "s_vmem             %s\n", lGetString(ep, QI_s_vmem)));
         FPRINTF((fp, "h_vmem             %s\n", lGetString(ep, QI_h_vmem)));
      }
   } else if (how == 4) {
      /*
       * Spool only non-CQ attributes
       */
   }
   if (how == 0) {
      FPRINTF((fp, "\n"));
   }

   if (how > 0 && how < 4) {
      fclose(fp);
   }
   DEXIT;
   return how==1?sge_strdup(NULL, filename):filename;
FPRINTF_ERROR:
   DEXIT;
   return NULL;  
}


