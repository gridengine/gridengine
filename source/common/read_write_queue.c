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
#include "def.h"
#include "sge_feature.h"
#include "commlib.h"
#include "sge_gdi_intern.h"
#include "sge_queueL.h"
#include "sge_answerL.h"
#include "sge_confL.h"
#include "sge_hostL.h"
#include "sge_usersetL.h"
#include "sge_complexL.h"
#include "sge_userprjL.h"
#include "read_write_queue.h"
#include "read_object.h"
#include "config.h"
#include "sge_tmpnam.h"
#include "parse_range.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "utility.h"
#include "sge_stdio.h"
#include "msg_common.h"
#include "sched_conf.h"
#include "sge_spoolmsg.h"

static char *queue_types[] = {
   "BATCH",        
   "INTERACTIVE",  
   "CHECKPOINTING",
   "PARALLEL",
   "TRANSFER",
   ""
};

static char *true_false[] =
{
   "FALSE",  
   "TRUE",   
   "unknown",
};

static intprt_type intprt_as_load_thresholds[] = { CE_name, CE_stringval, 0 };

static lListElem *create_template_queue(void);

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
         lSetString(ep, QU_qhostname, str);
      } 
      else {
         /* build unique hostname when it comes in */
         if ((ret = getuniquehostname(str, unique, 0)) != CL_OK) {
            if (ret != NACK_UNKNOWN_HOST) {
               sprintf(SGE_EVENT, MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS,
                  str, cl_errstr(ret));
               sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, NUM_AN_ERROR);
               DEXIT;
               return -1;
            }
         }
         if (ret == CL_OK)
            lSetString(ep, QU_qhostname, unique);
         else
            /* ignore NACK_UNKNOWN_HOST error */
            lSetString(ep, QU_qhostname, str);
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
   if (!set_conf_ulong(alpp, clpp, fields, "priority", ep, QU_priority)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_max_migr_time */
   if (!set_conf_string(alpp, clpp, fields, "max_migr_time", ep, QU_max_migr_time)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_migr_load_thresholds */
   if (parsing_type == 0) {
      if (!set_conf_deflist(alpp, clpp, fields, "migr_load_thresholds", ep, 
               QU_migr_load_thresholds, CE_Type, intprt_as_load_thresholds)) {
         DEXIT;
         return -1;
      }
   } else {
      if (!set_conf_list(alpp, clpp, fields, "migr_load_thresholds", ep,
         QU_migr_load_thresholds, CE_Type, CE_name)) {
         DEXIT;
         return -1;
      }     
   }

   /* --------- QU_max_no_migr */
   if (!set_conf_timestr(alpp, clpp, fields, "max_no_migr", ep, QU_max_no_migr)) {
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
      parse_ranges(str, JUST_PARSE, 0, alpp, NULL, INF_ALLOWED);
      if (*alpp) {
         DEXIT;
         return -1;
      }
   }

   /* --------- QU_qtype */
   if (!set_conf_enum(alpp, clpp, fields, "qtype", ep, QU_qtype, queue_types)) {
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

   /* --------- QU_klog */
   if (!set_conf_string(alpp, clpp, fields, "klog", ep, QU_klog)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_reauth_time */
   if (!set_conf_timestr(alpp, clpp, fields, "reauth_time", ep, QU_reauth_time)) {
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
            QU_subordinate_list, SO_Type, SO_qname, SO_threshold)) {
      DEXIT;
      return -1;
   }

   /* --------- QU_complex_list */
   if (!set_conf_list(alpp, clpp, fields, "complex_list", ep, 
            QU_complex_list, CX_Type, CX_name)) {
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
      ep = create_template_queue();
      template = lCopyElem(ep);
   } else 
      ep = read_object(dirname, filename, spool, 0,0 , &args, tag?tag:&intern_tag, fields);

   DEXIT;
   return ep;
}

/****
 **** create_template_queue (static)
 ****/
static lListElem *create_template_queue(void)
{
lListElem *qep, *ep;
lList *lp;

   qep = lCreateElem(QU_Type);
   lSetString(qep, QU_qname, "template");
   lSetString(qep, QU_qhostname, "unknown");

   lp = lCreateList("load_thresholds", CE_Type); 
   ep = lCreateElem(CE_Type);
   lSetString(ep, CE_name, "np_load_avg");
   lSetString(ep, CE_stringval, "1.75"); 
   lAppendElem(lp, ep);
   lSetList(qep, QU_load_thresholds, lp);

   lSetString(qep, QU_max_migr_time, "0");
   lp = lCreateList("migr_load_thresholds", CE_Type);
   ep = lCreateElem(CE_Type);
   lSetString(ep, CE_name, "np_load_avg");
   lSetString(ep, CE_stringval, "5.00");
   lAppendElem(lp, ep);
   lSetList(qep, QU_migr_load_thresholds, lp);
    
   lSetString(qep, QU_suspend_interval, "00:05:00");
   lSetUlong(qep, QU_nsuspend, 1);
   lSetString(qep, QU_max_no_migr, "00:02:00");
   lSetString(qep, QU_min_cpu_interval, "00:05:00");
   lSetString(qep, QU_processors, "UNDEFINED");

   lSetUlong(qep, QU_qtype, BQ|IQ);
   lSetUlong(qep, QU_job_slots, 1);
   lSetString(qep, QU_tmpdir, "/tmp");
   lSetString(qep, QU_shell, "/bin/csh");
   lSetString(qep, QU_klog, "/usr/local/bin/klog");
   lSetString(qep, QU_reauth_time, "01:40:00");
   lSetString(qep, QU_notify, "00:00:60");
   lSetString(qep, QU_initial_state, "default");

   lSetString(qep, QU_s_rt, "INFINITY");
   lSetString(qep, QU_h_rt, "INFINITY");
   lSetString(qep, QU_s_cpu, "INFINITY");
   lSetString(qep, QU_h_cpu, "INFINITY");
   lSetString(qep, QU_s_fsize, "INFINITY");
   lSetString(qep, QU_h_fsize, "INFINITY");
   lSetString(qep, QU_s_data, "INFINITY");
   lSetString(qep, QU_h_data, "INFINITY");
   lSetString(qep, QU_s_stack, "INFINITY");
   lSetString(qep, QU_h_stack, "INFINITY");
   lSetString(qep, QU_s_core, "INFINITY");
   lSetString(qep, QU_h_core, "INFINITY");
   lSetString(qep, QU_s_rss, "INFINITY");
   lSetString(qep, QU_h_rss, "INFINITY");
   lSetString(qep, QU_s_vmem, "INFINITY");
   lSetString(qep, QU_h_vmem, "INFINITY");

   return qep;
}

int cull_write_qconf(
int spool,
int write_2_stdout,
const char *file_prefix,
const char *file_name,
char *rfile,            /* has to be allocated by caller; can be NULL */
lListElem *qep 
) {
   char **ptr;
   FILE *fp;
   const char *s, *cp = NULL;
   char filename[SGE_PATH_MAX], real_filename[SGE_PATH_MAX]; 
   u_long32 bitmask;
   int ret;

   DENTER(TOP_LAYER, "cull_write_qconf");

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
             feature_get_product_name(FS_VERSION)) < 0) {
         goto FPRINTF_ERROR;
      }
   }

   FPRINTF((fp, "qname                %s\n", lGetString(qep, QU_qname)));

   /* for transfer queues qhostname is name of pseudohost
      that points (real hostname) to hostname to be printed
      for user */

   FPRINTF((fp, "hostname             %s\n", lGetString(qep, QU_qhostname)));
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
   FPRINTF((fp, "priority             %d\n", 
      (int) lGetUlong(qep, QU_priority)));
   FPRINTF((fp, "max_migr_time        %s\n", 
      lGetString(qep, QU_max_migr_time)));
   fprint_thresholds(fp, "migr_load_thresholds ", 
      lGetList(qep, QU_migr_load_thresholds), 1);
   FPRINTF((fp, "max_no_migr          %s\n", lGetString(qep, QU_max_no_migr)));
   FPRINTF((fp, "min_cpu_interval     %s\n", 
      lGetString(qep, QU_min_cpu_interval)));
   FPRINTF((fp, "processors           %s\n", lGetString(qep, QU_processors)));
   FPRINTF((fp, "qtype                "));
   bitmask = 1;
   for (ptr = queue_types; **ptr != '\0'; ptr++) {
     if (bitmask & lGetUlong(qep, QU_qtype))
       FPRINTF((fp,"%s ",*ptr));
     bitmask <<= 1;
   };
   FPRINTF((fp,"\n"));
   FPRINTF((fp, "rerun                %s\n", 
      true_false[lGetUlong(qep, QU_rerun)]));
   FPRINTF((fp, "slots                %d\n", 
      (int) lGetUlong(qep, QU_job_slots)));
   FPRINTF((fp, "tmpdir               %s\n", lGetString(qep, QU_tmpdir)));
   FPRINTF((fp, "shell                %s\n", lGetString(qep, QU_shell)));
   FPRINTF((fp, "shell_start_mode     %s\n", 
      (s=lGetString(qep, QU_shell_start_mode))?s:"NONE"));
   FPRINTF((fp, "klog                 %s\n", lGetString(qep, QU_klog)));
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
   FPRINTF((fp, "reauth_time          %s\n", lGetString(qep, QU_reauth_time)));
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
         FPRINTF((fp, " %s", lGetString(ep, SO_qname))); 
         t = lGetUlong(ep, SO_threshold);
         if (t) {
            FPRINTF((fp, "="u32"%s", t, lNext(ep)?",":""));
         }
      }
      FPRINTF((fp, "\n"));
   }
   ret = fprint_cull_list(fp,  "complex_list         ", 
      lGetList(qep, QU_complex_list), CX_name);
   if (ret == -1) {
      goto FPRINTF_ERROR;
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

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      FPRINTF((fp, "fshare               %d\n", 
         (int)lGetUlong(qep, QU_fshare)));
      FPRINTF((fp, "oticket              %d\n", 
         (int)lGetUlong(qep, QU_oticket)));
   }

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


