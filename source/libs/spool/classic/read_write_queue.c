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
#include <stdlib.h>
#include <errno.h>

#include "sge.h"
#include "sge_feature.h"
#include "commlib.h"
#include "sge_gdi_request.h"
#include "sge_answer.h"
#include "read_object.h"
#include "config.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_stdio.h"
#include "sge_spool.h"
#include "sge_io.h"
#include "sched_conf.h"
#include "sge_range.h"
#include "sge_qinstance.h"
#include "sge_userprj.h"
#include "sge_userset.h"
#include "sge_conf.h"
#include "sge_str.h"
#include "sge_centry.h"

#include "msg_common.h"

static char *true_false[] =
{
   "FALSE",  
   "TRUE",   
   "unknown",
};

static int intprt_as_load_thresholds[] = { CE_name, CE_stringval, 0 };


/****
 **** read_queue_work
 ****
 ****/
int read_queue_work(
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
   const char *str;
   const char *qname;
   int ret, generic_queue = 0;
   char unique[MAXHOSTLEN];
   
   DENTER(TOP_LAYER, "read_queue_work");

   /* --------- QU_name */
   if (!set_conf_string(alpp, clpp, fields, "qname", ep, QU_qname)) {
      DEXIT;
      return -1;
   }
   if ((qname = lGetString(ep, QU_qname)) 
        && !strcmp(qname, SGE_TEMPLATE_NAME)) {
      generic_queue = 1;
   }
   if (!qname) 
      qname = "<reduced queue>";

   /* --------- QU_qhostname */
   if (!(str=get_conf_value(fields?NULL:alpp, *clpp, CF_name, CF_value, 
               "hostname"))) {
      if (!fields) {
         DEXIT;
         return -1;
      }
   } 
   else {
      if (generic_queue) {
         lSetHost(ep, QU_qhostname, str);
      } 
      else {
         /* build unique hostname when it comes in */
         ret = getuniquehostname(str, unique, 0);
#ifdef ENABLE_NGC
         if (ret != CL_RETVAL_OK) {
            if ( ret != CL_RETVAL_GETHOSTNAME_ERROR) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS,
                  str, cl_errstr(ret)));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               DEXIT;
               return -1;
            }
         } 
         if (ret == CL_RETVAL_OK) {
            lSetHost(ep, QU_qhostname, unique);
         } else {
            /* ignore NACK_UNKNOWN_HOST error */
            lSetHost(ep, QU_qhostname, str);
         }
 
#else
         if (ret != CL_OK) {
            if (ret != COMMD_NACK_UNKNOWN_HOST) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS,
                  str, cl_errstr(ret)));
               answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
               DEXIT;
               return -1;
            }
         }
         if (ret == CL_OK) {
            lSetHost(ep, QU_qhostname, unique);
         } else {
            /* ignore NACK_UNKNOWN_HOST error */
            lSetHost(ep, QU_qhostname, str);
         }
#endif
      }

      lDelElemStr(clpp, CF_name, "hostname");
      add_nm_to_set(fields, QU_qhostname);
   }

   /* --------- QU_seq_no */
   if (!set_conf_ulong(alpp, clpp, fields, "seq_no", ep, QU_seq_no)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_load_thresholds */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields, "load_thresholds", ep, 
          QU_load_thresholds, CE_Type, intprt_as_load_thresholds)) {
         DEXIT;
         return -1;
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields, "load_thresholds", ep,
         QU_load_thresholds, CE_Type, CE_name)) {
         DEXIT;
         return -1;
      }
   }

   /* --------- QU_suspend_thresholds */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields, "suspend_thresholds", ep, 
               QU_suspend_thresholds, CE_Type, intprt_as_load_thresholds)) {
         DEXIT;
         return -1;
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields, "suspend_thresholds", ep,
         QU_suspend_thresholds, CE_Type, CE_name)) {
         DEXIT;
         return -1;
      }          
   }

   /* --------- QU_nsuspend */
   if (!set_conf_ulong(alpp, clpp, fields, "nsuspend", ep, QU_nsuspend)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_suspend_interval */
   if (!set_conf_string(alpp, clpp, fields, "suspend_interval", ep, 
            QU_suspend_interval)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_priority */
   if (!set_conf_string(alpp, clpp, fields, "priority", ep, QU_priority)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_min_cpu_interval */
   if (!set_conf_timestr(alpp, clpp, fields, "min_cpu_interval", ep, 
            QU_min_cpu_interval)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_processors */
   if (!set_conf_string(alpp, clpp, fields, "processors", ep, QU_processors)) {
      DEXIT;
      return -1;
   }

   if ((str = lGetString(ep, QU_processors))) {
      lList *range_list = NULL;
      range_list_parse_from_string(&range_list, alpp, str, 
                                   JUST_PARSE, 0, INF_ALLOWED);
      range_list = lFreeList(range_list);
      if (*alpp) {
         DEXIT;
         return -1;
      }
   }

   /* --------- QU_qtype */
   if (!set_conf_enum_none(alpp, clpp, fields, "qtype", ep, QU_qtype, queue_types)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_pe_list */
   if (!set_conf_list(alpp, clpp, fields, "pe_list", ep, 
            QU_pe_list, ST_Type, ST_name)) {
      DEXIT;
      return -1;
   }
   
   /* --------- QU_ckpt_list */
   if (!set_conf_list(alpp, clpp, fields, "ckpt_list", ep, 
            QU_ckpt_list, ST_Type, ST_name)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_rerun */
   if (!set_conf_bool(alpp, clpp, fields, "rerun", ep, QU_rerun)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_job_slots */
   if (!set_conf_ulong(alpp, clpp, fields, "slots", ep, QU_job_slots)) {
      DEXIT;
      return -1;
   }
  
   /* --------- QU_tmpdir */
   if (!set_conf_string(alpp, clpp, fields, "tmpdir", ep, QU_tmpdir)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_shell */
   if (!set_conf_string(alpp, clpp, fields, "shell", ep, QU_shell)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_notify */
   if (!set_conf_timestr(alpp, clpp, fields, "notify", ep, QU_notify)) {
      DEXIT;
      return -1;
   }   

   /* --------- QU_owner_list */
   if (!set_conf_list(alpp, clpp, fields, "owner_list", ep, QU_owner_list, 
            US_Type, US_name)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_acl */
   if (!set_conf_list(alpp, clpp, fields, "user_lists", ep, QU_acl, 
            US_Type, US_name)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_xacl */
   if (!set_conf_list(alpp, clpp, fields, "xuser_lists", ep, QU_xacl, 
            US_Type, US_name)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_subordinate_list */
   if (!set_conf_subordlist(alpp, clpp, fields, "subordinate_list", ep, 
            QU_subordinate_list, SO_Type, SO_name, SO_threshold)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_consumable_config_list */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields, "complex_values", 
               ep, QU_consumable_config_list, 
            CE_Type, intprt_as_load_thresholds)) {
         DEXIT;
         return -1;
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields, "complex_values", ep,
         QU_consumable_config_list, CE_Type, CE_name)) {
         DEXIT;
         return -1;
      }    
   }

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      /* --------- QU_projects */
      if (!set_conf_list(alpp, clpp, fields, "projects", ep, 
               QU_projects, UP_Type, UP_name)) {
         DEXIT;
         return -1;
      }
      /* --------- QU_xprojects */
      if (!set_conf_list(alpp, clpp, fields, "xprojects", ep, 
               QU_xprojects, UP_Type, UP_name)) {
         DEXIT;
         return -1;
      }

#if 0
      /* --------- QU_fshare */
      if (!set_conf_ulong(alpp, clpp, fields, "fshare", ep, QU_fshare)) {
         DEXIT;
         return -1;
      }
      /* --------- QU_oticket */
      if (!set_conf_ulong(alpp, clpp, fields, "oticket", ep, QU_oticket)) {
         DEXIT;
         return -1;
      }
#endif

   }

   /* --------- QU_calendar */
   if (!set_conf_string(alpp, clpp, fields, "calendar", ep, QU_calendar)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_calendar);

   /* --------- QU_prolog */
   if (!set_conf_string(alpp, clpp, fields, "prolog", 
         ep, QU_prolog)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_prolog);

   /* --------- QU_epilog */
   if (!set_conf_string(alpp, clpp, fields, "epilog", 
         ep, QU_epilog)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_epilog);

   /* --------- QU_shell_start_mode */
   if (!set_conf_string(alpp, clpp, fields, "shell_start_mode", 
         ep, QU_shell_start_mode)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_shell_start_mode);
   
   /* --------- QU_starter_method */
   if (!set_conf_string(alpp, clpp, fields, "starter_method", ep, QU_starter_method)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_starter_method);

   /* --------- QU_suspend_method */
   if (!set_conf_string(alpp, clpp, fields, "suspend_method", ep, QU_suspend_method)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_suspend_method);

   /* --------- QU_resume_method */
   if (!set_conf_string(alpp, clpp, fields, "resume_method", ep, QU_resume_method)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_resume_method);

   /* --------- QU_terminate_method */
   if (!set_conf_string(alpp, clpp, fields, "terminate_method", ep, QU_terminate_method)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_terminate_method);

   /* --------- QU_initial_state */
   if (!set_conf_string(alpp, clpp, fields, "initial_state", 
         ep, QU_initial_state)) {
      DEXIT;
      return -1;
   }
   NULL_OUT_NONE(ep, QU_initial_state);

   /* --------- QU_s_rt */
   if (!set_conf_timestr(alpp, clpp, fields, "s_rt", ep, QU_s_rt)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_rt */
   if (!set_conf_timestr(alpp, clpp, fields, "h_rt", ep, QU_h_rt)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_cpu */
   if (!set_conf_timestr(alpp, clpp, fields, "s_cpu", ep, QU_s_cpu)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_cpu */
   if (!set_conf_timestr(alpp, clpp, fields, "h_cpu", ep, QU_h_cpu)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_fsize */
   if (!set_conf_memstr(alpp, clpp, fields, "s_fsize", ep, QU_s_fsize)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_fsize */
   if (!set_conf_memstr(alpp, clpp, fields, "h_fsize", ep, QU_h_fsize)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_data */
   if (!set_conf_memstr(alpp, clpp, fields, "s_data", ep, QU_s_data)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_data */
   if (!set_conf_memstr(alpp, clpp, fields, "h_data", ep, QU_h_data)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_stack */
   if (!set_conf_memstr(alpp, clpp, fields, "s_stack", ep, QU_s_stack)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_stack */
   if (!set_conf_memstr(alpp, clpp, fields, "h_stack", ep, QU_h_stack)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_core */
   if (!set_conf_memstr(alpp, clpp, fields, "s_core", ep, QU_s_core)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_core */
   if (!set_conf_memstr(alpp, clpp, fields, "h_core", ep, QU_h_core)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_rss */
   if (!set_conf_memstr(alpp, clpp, fields, "s_rss", ep, QU_s_rss)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_rss */
   if (!set_conf_memstr(alpp, clpp, fields, "h_rss", ep, QU_h_rss)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_s_vmem */
   /* this code should be tolerant with old configurations without this attribute
      thus the 's_vmem' field is optional in all cases */
   if (!set_conf_memstr(alpp, clpp, fields, "s_vmem", ep, QU_s_vmem)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_h_vmem */
   /* this code should be tolerant with old configurations without this attribute
      thus the 'h_vmem' field is optional in all cases */
   if (!set_conf_memstr(alpp, clpp, fields, "h_vmem", ep, QU_h_vmem)) {
      DEXIT;
      return -1;
   }

   if (spool && !generic_queue) {

      /* --------- QU_state */
      if (!set_conf_ulong(alpp, clpp, fields, "state", ep, QU_state)) {
         DEXIT;
         return -1;
      }

      /* --------- QU_pending_signal */
      if (!set_conf_ulong(alpp, clpp, fields, "pending_signal", 
               ep, QU_pending_signal)) {
         DEXIT;
         return -1;
      }

      /* --------- QU_pending_signal_delivery_time */
      if (!set_conf_ulong(alpp, clpp, fields, "pending_signal_deli", ep, 
               QU_pending_signal_delivery_time)) {
         DEXIT;
         return -1;
      }

      /* --------- QU_version */
      if (!set_conf_ulong(alpp, clpp, fields, "version", ep, QU_version)) {
         DEXIT;
         return -1;
      }

      /* --------- QU_queue_number */
      if (!set_conf_ulong(alpp, clpp, fields, "queue_number", ep, QU_queue_number)) {
         DEXIT;
         return -1;
      }
   }
   /* uff */
   DEXIT;
   return 0;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}


/****
 **** cull_read_in_qconf
 ****/
lListElem *cull_read_in_qconf(
const char *dirname,
const char *filename,
int spool,
int type,
int *tag,
int fields[] 
) {
#ifdef __INSIGHT__
/* JG: NULL is OK for fields */
_Insight_set_option("suppress", "PARM_NULL");
#endif
   lListElem *ep;
   struct read_object_args args = { QU_Type, "queue", read_queue_work};
   static lListElem *template = NULL;
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_qconf");

   /* user want's template queue? */
   if(!dirname && !filename) {
      if (template) {
         DEXIT;
         return lCopyElem(template);
      }
      ep = queue_create_template();
      template = lCopyElem(ep);
   } else 
      ep = read_object(dirname, filename, spool, 0,0 , &args, tag?tag:&intern_tag, fields);

   DEXIT;
   return ep;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "PARM_NULL");
#endif
}

int cull_write_qconf(
int spool,
bool write_2_stdout,
const char *file_prefix,
const char *file_name,
char *rfile,            /* has to be allocated by caller; can be NULL */
const lListElem *qep 
) {
   FILE *fp;
   const char *s, *cp = NULL;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX]; 
   int ret;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "cull_write_qconf");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   if (write_2_stdout) {
      fp = stdout;
   } else {
      if (!file_name) {
         cp = sge_tmpnam(filename);
         if (!cp) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            return -1;
         }
         DPRINTF(("sge_tmpnam() = %s\n", cp));
      } else if (file_prefix) {
         sprintf(filename, "%s/.%s", file_prefix, file_name);
         sprintf(real_filename, "%s/%s", file_prefix, file_name);
      } else {
         strcpy(filename, ".");
         strcat(filename, file_name);
         strcpy(real_filename, file_name);
      }

      fp = fopen(filename, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, 
               filename, strerror(errno)));
         DEXIT;
         return -1;
      }
   }

   if (spool) {
      if (sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION, &ds)) < 0) {
         goto FPRINTF_ERROR;
      }
   }

   FPRINTF((fp, "qname                %s\n", lGetString(qep, QU_qname)));
   FPRINTF((fp, "hostname             %s\n", lGetHost(qep, QU_qhostname)));
   FPRINTF((fp, "seq_no               %d\n", (int) lGetUlong(qep, QU_seq_no)));
 
   ret = fprint_thresholds(fp, "load_thresholds      ", 
      lGetList(qep, QU_load_thresholds), 1);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   }
   ret = fprint_thresholds(fp, "suspend_thresholds   ", 
      lGetList(qep, QU_suspend_thresholds), 1);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   FPRINTF((fp, "nsuspend             %d\n", 
      (int) lGetUlong(qep, QU_nsuspend)));
   FPRINTF((fp, "suspend_interval     %s\n", 
      lGetString(qep, QU_suspend_interval) ? 
      lGetString(qep, QU_suspend_interval) : "00:05:00"));
   FPRINTF((fp, "priority             %s\n", lGetString(qep, QU_priority)));
   FPRINTF((fp, "min_cpu_interval     %s\n", 
      lGetString(qep, QU_min_cpu_interval)));
   FPRINTF((fp, "processors           %s\n", lGetString(qep, QU_processors)));
   {
      dstring qtype_buffer = DSTRING_INIT;

      qinstance_print_qtype_to_dstring(qep, &qtype_buffer, false);
      FPRINTF((fp, "qtype                "));
      FPRINTF((fp,"%s\n", sge_dstring_get_string(&qtype_buffer)));
      sge_dstring_free(&qtype_buffer);
   }
   ret = fprint_cull_list(fp,  "ckpt_list            ", 
      lGetList(qep, QU_ckpt_list), ST_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   ret = fprint_cull_list(fp,  "pe_list              ", 
      lGetList(qep, QU_pe_list), ST_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   FPRINTF((fp, "rerun                %s\n", 
      true_false[(int)lGetBool(qep, QU_rerun)]));
   FPRINTF((fp, "slots                %d\n", 
      (int) lGetUlong(qep, QU_job_slots)));
   FPRINTF((fp, "tmpdir               %s\n", lGetString(qep, QU_tmpdir)));
   FPRINTF((fp, "shell                %s\n", lGetString(qep, QU_shell)));
   FPRINTF((fp, "shell_start_mode     %s\n", 
      (s=lGetString(qep, QU_shell_start_mode))?s:"NONE"));
   FPRINTF((fp, "prolog               %s\n", 
      (s=lGetString(qep, QU_prolog))?s:"NONE"));
   FPRINTF((fp, "epilog               %s\n", 
      (s=lGetString(qep, QU_epilog))?s:"NONE"));
   FPRINTF((fp, "starter_method       %s\n", 
      (s=lGetString(qep, QU_starter_method))?s:"NONE"));
   FPRINTF((fp, "suspend_method       %s\n", 
      (s=lGetString(qep, QU_suspend_method))?s:"NONE"));
   FPRINTF((fp, "resume_method        %s\n", 
      (s=lGetString(qep, QU_resume_method))?s:"NONE"));
   FPRINTF((fp, "terminate_method     %s\n", 
      (s=lGetString(qep, QU_terminate_method))?s:"NONE"));
   FPRINTF((fp, "notify               %s\n", lGetString(qep, QU_notify)));
   ret = fprint_cull_list(fp, "owner_list           ", 
      lGetList(qep, QU_owner_list), US_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   }
   ret = fprint_cull_list(fp, "user_lists           ", 
      lGetList(qep, QU_acl), US_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   ret = fprint_cull_list(fp, "xuser_lists          ", 
      lGetList(qep, QU_xacl), US_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   FPRINTF((fp, "subordinate_list    "));
   if (!lGetList(qep, QU_subordinate_list)) {
      FPRINTF((fp, " NONE\n"));
   } else {
      /*  qname[=n], ...  */
      u_long32 t;
      lListElem *ep;

      for_each (ep, lGetList(qep, QU_subordinate_list)) {
         FPRINTF((fp, " %s", lGetString(ep, SO_name))); 
         t = lGetUlong(ep, SO_threshold);
         if (t) {
            FPRINTF((fp, "="u32"%s", t, lNext(ep)?",":""));
         }
      }
      FPRINTF((fp, "\n"));
   }
   ret = fprint_thresholds(fp, "complex_values       ", 
      lGetList(qep, QU_consumable_config_list), 0);
   if (ret == -1) {
      goto FPRINTF_ERROR;
   } 
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      ret = fprint_cull_list(fp, "projects             ", 
         lGetList(qep, QU_projects), UP_name);
      if (ret == -1) {
         goto FPRINTF_ERROR;
      } 
      ret = fprint_cull_list(fp, "xprojects            ", 
         lGetList(qep, QU_xprojects), UP_name);
      if (ret == -1) {
         goto FPRINTF_ERROR;
      } 
   }
   FPRINTF((fp, "calendar             %s\n", 
      (s=lGetString(qep, QU_calendar))?s:"NONE"));
   FPRINTF((fp, "initial_state        %s\n", 
      lGetString(qep, QU_initial_state)));

#if 0
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      FPRINTF((fp, "fshare               %d\n", 
         (int)lGetUlong(qep, QU_fshare)));
      FPRINTF((fp, "oticket              %d\n", 
         (int)lGetUlong(qep, QU_oticket)));
   }
#endif

   FPRINTF((fp, "s_rt                 %s\n", lGetString(qep, QU_s_rt)));
   FPRINTF((fp, "h_rt                 %s\n", lGetString(qep, QU_h_rt)));
   FPRINTF((fp, "s_cpu                %s\n", lGetString(qep, QU_s_cpu)));
   FPRINTF((fp, "h_cpu                %s\n", lGetString(qep, QU_h_cpu)));
   FPRINTF((fp, "s_fsize              %s\n", lGetString(qep, QU_s_fsize)));
   FPRINTF((fp, "h_fsize              %s\n", lGetString(qep, QU_h_fsize)));
   FPRINTF((fp, "s_data               %s\n", lGetString(qep, QU_s_data)));
   FPRINTF((fp, "h_data               %s\n", lGetString(qep, QU_h_data)));
   FPRINTF((fp, "s_stack              %s\n", lGetString(qep, QU_s_stack)));
   FPRINTF((fp, "h_stack              %s\n", lGetString(qep, QU_h_stack)));
   FPRINTF((fp, "s_core               %s\n", lGetString(qep, QU_s_core)));
   FPRINTF((fp, "h_core               %s\n", lGetString(qep, QU_h_core)));
   FPRINTF((fp, "s_rss                %s\n", lGetString(qep, QU_s_rss)));
   FPRINTF((fp, "h_rss                %s\n", lGetString(qep, QU_h_rss)));
   FPRINTF((fp, "s_vmem               %s\n", lGetString(qep, QU_s_vmem)));
   FPRINTF((fp, "h_vmem               %s\n", lGetString(qep, QU_h_vmem)));

   /* these fields get only written in case of spooling */
   if (spool) {
      FPRINTF((fp, "state               %d\n", 
         (int)lGetUlong(qep, QU_state)));
      FPRINTF((fp, "pending_signal      %d\n", 
         (int)lGetUlong(qep, QU_pending_signal)));
      FPRINTF((fp, "pending_signal_deli %d\n", 
         (int)lGetUlong(qep, QU_pending_signal_delivery_time)));
      FPRINTF((fp, "version             %d\n", 
         (int)lGetUlong(qep, QU_version)));
      FPRINTF((fp, "queue_number        %d\n", 
         (int)lGetUlong(qep, QU_queue_number)));

      DPRINTF(("spooled queue %s\n", lGetString(qep, QU_qname))); 
   }

   if (write_2_stdout && getenv("MORE_INFO")) {
      FPRINTF((fp, "suspended_on_subordinate "u32"\n", 
         lGetUlong(qep, QU_suspended_on_subordinate)));
      ret = fprint_thresholds(fp, "complex_values_actual  ", 
         lGetList(qep, QU_consumable_actual_list), 1);
      if (ret == -1) {
         goto FPRINTF_ERROR;
      } 
   }
   if (!write_2_stdout) {
      fclose(fp);
      if (rfile)
         strcpy(rfile, filename);
   }

   if (file_name) {
      if (rename(filename, real_filename) == -1) {
         DEXIT;
         return -1;
      } 
   }

   DEXIT;
   return 0;

FPRINTF_ERROR:
   DEXIT;
   return -1; 
}


