#/*___INFO__MARK_BEGIN__*/
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

#include "sge_unistd.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_gdi_request.h"
#include "sge_usageL.h"
#include "sge_schedd_conf.h"
#include "config.h"
#include "sched_conf.h"
#include "read_object.h"
#include "sge_feature.h"
#include "sge_log.h"
#include "setup_path.h"
#include "msg_common.h"
#include "sge_feature.h"
#include "sge_stdio.h"
#include "sge_schedd.h"
#include "sge_spool.h"
#include "sge_string.h"
#include "sge_prog.h"
#include "sge_io.h"
#include "sge_answer.h"
#include "sge_conf.h"
#include "sge_centry.h"

static int intprt_as_load_adjustment[] = { CE_name, CE_stringval, 0 };

static int intprt_as_usage[] = { UA_name, UA_value, 0 };

static const char *delis[] = {"=", ",", "\n"};

static int str2qsm(const char *qsm_str);

static char *qsm2str(u_long32 qsm_val);

static int read_schedd_conf_work(lList **alpp, lList **clpp, int fields[], 
                                 lListElem *ep, int spool, int flag, int *tag, 
                                 int parsing_type);


/****** spool/classic/write_sched_configuration() ****************************
*  NAME
*     write_sched_configuration() -- print scheduler configuration 
*
*  SYNOPSIS
*     char* write_sched_configuration(int spool, int how, 
*                                     const char *common_dir, lListElem *ep) 
*
*  FUNCTION
*     This function is used to print the current scheduler configuration
*     in human readable form either to stdout or into a spoolfile or tmpfile. 
*
*  INPUTS
*     int spool - which fields should be written 
*        1 - write for spooling
*        2 - write only user controlled fields
*
*     int how - destination of the write operation 
*        0 - use stdout
*        1 - write into tmpfile
*        2 - write into spoolfile (use absolute path
*            common_dir/sched_configuration)
*
*     const char *common_dir - absolute path to common directory,
*                              used if how == 2
*
*     lListElem *ep - scheduler configuration (SC_Type)
*
*  RESULT
*     char* 
*        how == 0: NULL
*        how != 0: filename or NULL in case of an error
*******************************************************************************/
char *write_sched_configuration(int spool, int how, const char *common_dir, const lListElem *ep) 
{
/* JG: suppress READ_DANGLING. sge_root comes from a getenv() call.
 *     this should be handled properly in underlying function, e.g. by
 *     strdupping the value returned by getenv().
 */
#ifdef __INSIGHT__
_Insight_set_option("suppress", "READ_DANGLING");
#endif
   FILE *fp; 
   char fname[SGE_PATH_MAX], real_fname[SGE_PATH_MAX];
   int fret;
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "write_sched_configuration");

   sge_dstring_init(&ds, buffer, sizeof(buffer));
   switch (how) {
   case 0:
      fp = stdout;
      break;
   case 1:
   case 2:
      if (how==1) {
         if (!sge_tmpnam(fname)) {
            CRITICAL((SGE_EVENT, MSG_TMPNAM_GENERATINGTMPNAM));
            DEXIT;
            return NULL;
         }
      } else if(how == 2) {
         sprintf(fname, "%s/.%s", common_dir, SCHED_CONF_FILE);
         sprintf(real_fname, "%s/%s", common_dir, SCHED_CONF_FILE);
      }

      fp = fopen(fname, "w");
      if (!fp) {
         CRITICAL((SGE_EVENT, MSG_FILE_ERRORWRITING_SS, fname, strerror(errno)));
         DEXIT;
         return NULL;
      }
      break;
   default:
      DEXIT;
      return NULL;
   }  

   if (spool && (sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION, &ds)) < 0)) {
      goto FPRINTF_ERROR;
   }  

   /* conf values needed for both SGE & SGEEE */
   FPRINTF((fp, "algorithm                        %s\n", lGetString(ep, SC_algorithm)));
   FPRINTF((fp, "schedule_interval                %s\n", lGetString(ep, SC_schedule_interval)));
   FPRINTF((fp, "maxujobs                         " u32 "\n", lGetUlong(ep, SC_maxujobs)));
   FPRINTF((fp, "queue_sort_method                %s\n", qsm2str(lGetUlong(ep, SC_queue_sort_method))));
   FPRINTF((fp, "user_sort                        %s\n", lGetBool(ep, SC_user_sort)?"true":"false"));
   FPRINTF((fp, "job_load_adjustments             "));
   fret = uni_print_list(fp, NULL, 0, lGetList(ep, SC_job_load_adjustments), intprt_as_load_adjustment, delis, 0);
   if (fret < 0) {
      goto FPRINTF_ERROR;
   }
   FPRINTF((fp, "load_adjustment_decay_time       %s\n", lGetString(ep, SC_load_adjustment_decay_time)));
   FPRINTF((fp, "load_formula                     %s\n", lGetString(ep, SC_load_formula)));
   FPRINTF((fp, "schedd_job_info                  %s\n", lGetString(ep, SC_schedd_job_info)));
   FPRINTF((fp, "flush_submit_sec                 " u32 "\n", lGetUlong(ep, SC_flush_submit_sec)));
   FPRINTF((fp, "flush_finish_sec                 " u32 "\n", lGetUlong(ep, SC_flush_finish_sec)));
   FPRINTF((fp, "params                           %s\n", lGetString(ep, SC_params)));
   
   /* conf values needed for SGEEE */
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      FPRINTF((fp, "reprioritize_interval            %s\n", lGetString(ep, SC_reprioritize_interval)));
      FPRINTF((fp, "halftime                         " u32 "\n", lGetUlong(ep, SC_halftime)));
      FPRINTF((fp, "usage_weight_list                "));
      fret = uni_print_list(fp, NULL, 0, lGetList(ep, SC_usage_weight_list), intprt_as_usage, delis, 0);
      if (fret < 0) {
         goto FPRINTF_ERROR;
      }
      FPRINTF((fp, "compensation_factor              %.10g\n", lGetDouble(ep, SC_compensation_factor)));
      FPRINTF((fp, "weight_user                      %.10g\n", lGetDouble(ep, SC_weight_user)));
      FPRINTF((fp, "weight_project                   %.10g\n", lGetDouble(ep, SC_weight_project)));
      FPRINTF((fp, "weight_jobclass                  %.10g\n", lGetDouble(ep, SC_weight_jobclass)));
      FPRINTF((fp, "weight_department                %.10g\n", lGetDouble(ep, SC_weight_department)));
      FPRINTF((fp, "weight_job                       %.10g\n", lGetDouble(ep, SC_weight_job)));
      FPRINTF((fp, "weight_tickets_functional        " u32 "\n", lGetUlong(ep, SC_weight_tickets_functional)));
      FPRINTF((fp, "weight_tickets_share             " u32 "\n", lGetUlong(ep, SC_weight_tickets_share)));

      FPRINTF((fp, "share_override_tickets           %s\n", lGetBool(ep, SC_share_override_tickets)?"true":"false"));
      FPRINTF((fp, "share_functional_shares          %s\n", lGetBool(ep, SC_share_functional_shares)?"true":"false"));
      FPRINTF((fp, "max_functional_jobs_to_schedule  " u32 "\n", lGetUlong(ep, SC_max_functional_jobs_to_schedule)));
      FPRINTF((fp, "report_pjob_tickets              %s\n", lGetBool(ep, SC_report_pjob_tickets)?"true":"false"));
      FPRINTF((fp, "max_pending_tasks_per_job        " u32 "\n", lGetUlong(ep, SC_max_pending_tasks_per_job)));
      FPRINTF((fp, "halflife_decay_list              %s\n", lGetString(ep, SC_halflife_decay_list)));
      FPRINTF((fp, "policy_hierarchy                 %s\n", lGetString(ep, SC_policy_hierarchy)));
      FPRINTF((fp, "weight_ticket                    %.10g\n", lGetDouble(ep, SC_weight_ticket)));
      FPRINTF((fp, "weight_waiting_time              %.10g\n", lGetDouble(ep, SC_weight_waiting_time)));
      FPRINTF((fp, "weight_deadline                  %.10g\n", lGetDouble(ep, SC_weight_deadline)));
      FPRINTF((fp, "weight_urgency                   %.10g\n", lGetDouble(ep, SC_weight_urgency)));
   }

   if (how != 0) {
      fclose(fp);
   }

   if (how == 2 || how == 3) {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(fname, real_fname);
      }   
   } 
   DEXIT;
 
   /* JG: TODO: ERROR: fname is returned, but is a stack variable!! */
   return how==1?sge_strdup(NULL, fname):fname;
FPRINTF_ERROR:
   DEXIT;
   return NULL;
#ifdef __INSIGHT__
_Insight_set_option("unsuppress", "READ_DANGLING");
#endif
}

/* mapping func for SC_queue_sort_method uval -> str */
static char *qsm2str(u_long32 qsm_val) {
   switch (qsm_val) {
   case QSM_SEQNUM:
      return "seqno";
   case QSM_SHARE:
      return "share";
   default: 
      return "load";
   }
}

/* mapping func for SC_queue_sort_method str -> uval */
static int str2qsm(const char *qsm_str) 
{
   if (!qsm_str)
      return -1;

   if (!strcasecmp(qsm_str, "load"))
      return QSM_LOAD;
   if (!strcasecmp(qsm_str, "seqno"))
      return QSM_SEQNUM;
   if (!strcasecmp(qsm_str, "share"))
      return QSM_SHARE;

   return -1; /* error */
}


/****
 **** read_schedd_conf_work
 ****
 ****/
static int read_schedd_conf_work(lList **alpp, lList **clpp, int fields[], 
                                 lListElem *ep, int spool, int flag, int *tag,
                                 int parsing_type) {
   const char *str;
   lList *alp = NULL;
   u_long32 ul;
   
   DENTER(TOP_LAYER, "read_schedd_conf_work");

   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      /* --------- SC_halftime */
      if (!set_conf_ulong(alpp, clpp, fields, "halftime", ep, SC_halftime)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_usage_weight_list */
      if (!set_conf_deflist(alpp, clpp, fields, "usage_weight_list", ep, 
               SC_usage_weight_list, UA_Type, intprt_as_usage)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_compensation_factor */
      if (!set_conf_double(alpp, clpp, fields, "compensation_factor", ep, 
               SC_compensation_factor, 0)) {
         DEXIT;
         return -1;
      }
      
      /* --------- SC_weight_user */
      if (!set_conf_double(alpp, clpp, fields, "weight_user", ep, 
               SC_weight_user, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_project */
      if (!set_conf_double(alpp, clpp, fields, "weight_project", ep, 
               SC_weight_project, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_share_override_tickets */
      if (!set_conf_bool(alpp, clpp, fields, "share_override_tickets", ep, 
               SC_share_override_tickets)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_share_functional_shares */
      if (!set_conf_bool(alpp, clpp, fields, "share_functional_shares", ep, 
               SC_share_functional_shares)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_report_pjob_tickets*/
      if (!set_conf_bool(alpp, clpp, fields, "report_pjob_tickets", ep, 
               SC_report_pjob_tickets)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_max_functional_jobs_to_schedule*/
      if (!set_conf_ulong(alpp, clpp, fields, "max_functional_jobs_to_schedule", ep, 
               SC_max_functional_jobs_to_schedule)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_max_pending_tasks_per_job */
      if (!set_conf_ulong(alpp, clpp, fields, "max_pending_tasks_per_job", ep, 
               SC_max_pending_tasks_per_job)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_halflife_decay_list */
      if (!set_conf_string(alpp, clpp, fields, "halflife_decay_list", ep, 
               SC_halflife_decay_list)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_jobclass */
      if (!set_conf_double(alpp, clpp, fields, "weight_jobclass", ep, 
               SC_weight_jobclass, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_department */
      if (!set_conf_double(alpp, clpp, fields, "weight_department", ep, 
               SC_weight_department,0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_job */
      if (!set_conf_double(alpp, clpp, fields, "weight_job", ep, 
               SC_weight_job, 045)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_tickets_functional */
      if (!set_conf_ulong(alpp, clpp, fields, "weight_tickets_functional", ep, 
                           SC_weight_tickets_functional)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_tickets_share */
      if (!set_conf_ulong(alpp, clpp, fields, "weight_tickets_share", ep, 
                           SC_weight_tickets_share)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_ticket */
      if (!set_conf_double(alpp, clpp, fields, "weight_ticket", ep, 
               SC_weight_ticket, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_waiting_time */
      if (!set_conf_double(alpp, clpp, fields, "weight_waiting_time", ep, 
               SC_weight_waiting_time, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_deadline */
      if (!set_conf_double(alpp, clpp, fields, "weight_deadline", ep, 
               SC_weight_deadline, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_urgency */
      if (!set_conf_double(alpp, clpp, fields, "weight_urgency", ep, 
               SC_weight_urgency, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_reprioritize_interval*/
      if (!set_conf_timestr(alpp, clpp, fields, "reprioritize_interval", ep, 
                              SC_reprioritize_interval)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_policy_hierarchy */
      if (!set_conf_string(alpp, clpp, fields, "policy_hierarchy", ep,
                           SC_policy_hierarchy)) {
         DEXIT;
         return -1;
      }
   }


   /* --------- SC_schedule_interval */
   if (!set_conf_timestr(alpp, clpp, fields, "schedule_interval", ep, 
                              SC_schedule_interval)) {
      DEXIT;
      return -1;
   }

   /* --------- SC_algorithm */
   if (!set_conf_string(alpp, clpp, fields, "algorithm", ep, SC_algorithm)) {
      DEXIT;
      return -1;
   }

   /* --------- SC_maxujobs */
   if (!set_conf_ulong(alpp, clpp, fields, "maxujobs", ep, SC_maxujobs)) {
      DEXIT;
      return -1;
   }

   /* --------- SC_flush_submit_sec*/
   if (!set_conf_ulong(alpp, clpp, fields, "flush_submit_sec", ep, 
            SC_flush_submit_sec)) {
      DEXIT;
      return -1;
   }    
  
   if (!set_conf_string(alpp, clpp, fields, "params", ep,
            SC_params)) {
      DEXIT;
      return -1;
   }
  
   /* --------- SC_flush_finish_sec*/
   if (!set_conf_ulong(alpp, clpp, fields, "flush_finish_sec", ep, 
            SC_flush_finish_sec)) {
      DEXIT;
      return -1;
   }   

   /* --------- SC_queue_sort_method */
   str = get_conf_value(&alp, *clpp, CF_name, CF_value, "queue_sort_method");
   alp = lFreeList(alp);
   if (str) {
      ul = str2qsm(str);
      if (ul == (u_long32) -1) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SCHEDCONF_INVALIDVALUEXFORQUEUESORTMETHOD_S, str));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1; 
      }
      lSetUlong(ep, SC_queue_sort_method, ul);
      lDelElemStr(clpp, CF_name, "queue_sort_method");
      add_nm_to_set(fields, SC_queue_sort_method);
   }
   else {
      str = get_conf_value(&alp, *clpp, CF_name, CF_value, "sort_seq_no");
      alp = lFreeList(alp);
      if (!str) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_SCHEDCONF_INVALIDVALUEXFORQUEUESORTMETHOD_S, str));
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1; 
      }
      ul = !strcasecmp(str, "true") ? QSM_SEQNUM:QSM_LOAD;
      lSetUlong(ep, SC_queue_sort_method, ul);
      lDelElemStr(clpp, CF_name, "sort_seq_no");
      add_nm_to_set(fields, SC_queue_sort_method);
   }
      
   /* --------- SC_user_sort */
   if (!set_conf_bool(alpp, clpp, fields, "user_sort", ep, SC_user_sort)) {
      DEXIT;
      return -1;
   }
 

   /* ---------  SC_job_load_adjustments */
   if (!set_conf_deflist(alpp, clpp, fields, "job_load_adjustments", ep, 
         SC_job_load_adjustments, CE_Type, intprt_as_load_adjustment)) {
      DEXIT;
      return -1;
   }

   /* --------- SC_load_adjustment_decay_time */
   if (!set_conf_timestr(alpp, clpp, fields, "load_adjustment_decay_time", ep, 
                              SC_load_adjustment_decay_time)) {
      DEXIT;
      return -1;
   }


   /* --------- SC_load_formula */
   if (!set_conf_string(alpp, clpp, fields, "load_formula", ep, SC_load_formula)) {
      DEXIT;
      return -1;
   }

   /* --------- SC_schedd_job_info */

   if (!set_conf_string(alpp, clpp, fields, "schedd_job_info", ep, SC_schedd_job_info)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

/****
 **** cull_read_in_schedd_conf
 ****/
lListElem *cull_read_in_schedd_conf(char *dirname, const char *filename,
                                    int spool, int *tag) 
{
   lListElem *ep;
   struct read_object_args args = { SC_Type, "schedd_conf", read_schedd_conf_work };
   int intern_tag = 0;

   DENTER(TOP_LAYER, "cull_read_in_schedd_conf");

   ep = read_object(dirname, filename, spool, 0, 0, &args, tag?tag:&intern_tag, NULL);

   DEXIT;
   return ep;
}


/***************************************************
 Read scheduler configuration

 spool: from spooled file (may contain additional fields)
 ***************************************************/
lList *read_sched_configuration(const char *common_dir, const char *fname, int spool, lList **alpp) 
{
   lList *confl = NULL;
   lListElem *ep;
   int write_default_config = 0;
   SGE_STRUCT_STAT st;

   DENTER(TOP_LAYER, "read_sched_configuration");

   if (!SGE_STAT(fname, &st)) {
      ep = cull_read_in_schedd_conf(NULL, fname, spool, NULL);
   } else {
      write_default_config = 1;
      ep = sconf_create_default();
   }

   if (ep) {
      confl = lCreateList("scheduler config", SC_Type);
      lAppendElem(confl, ep);
   }
   else {
      CRITICAL((SGE_EVENT, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION));
      answer_list_add(alpp, SGE_EVENT, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return NULL;
   }

   if (write_default_config) {
      if (write_sched_configuration(1, 2, common_dir, lFirst(confl)) == NULL) {
         answer_list_add(alpp, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return NULL;
      }
   }

   DEXIT;
   return confl;
}


