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

#include "sge_unistd.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_gdi_intern.h"
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
#include "sge_complex.h"
#include "sge_conf.h"

static intprt_type intprt_as_load_adjustment[] = { CE_name, CE_stringval, 0 };

static intprt_type intprt_as_usage[] = { UA_name, UA_value, 0 };

static const char *delis[] = {"=", ",", "\n"};

static int str2qsm(const char *qsm_str);

static char *qsm2str(u_long32 qsm_val);

static int read_schedd_conf_work(lList **alpp, lList **clpp, int fields[], 
                                 lListElem *ep, int spool, int flag, int *tag, 
                                 int parsing_type);

static lListElem *cull_read_in_schedd_conf(char *dirname, char *fname, 
                                           int spool, int *tag);

static lListElem *create_default_sched_conf(void);

#define DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME "0:7:30"
#define DEFAULT_LOAD_FORMULA                "np_load_avg"
#define SCHEDULE_TIME                       "0:0:15"
#define SGEEE_SCHEDULE_TIME                 "0:2:0"
#define MAXUJOBS                            0
#define MAXGJOBS                            0
#define SCHEDD_JOB_INFO                     "true"

/****** sge/sched_conf/write_sched_configuration() ****************************
*  NAME
*     write_sched_configuration() -- print scheduler configuration 
*
*  SYNOPSIS
*     char* write_sched_configuration(int spool, int how, lListElem *ep) 
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
*        2 - write into spoolfile 
*
*     lListElem *ep - scheduler configuration (SC_Type)
*
*  RESULT
*     char* 
*        how == 0: NULL
*        how != 0: filename or NULL in case of an error
*******************************************************************************/
char *write_sched_configuration(int spool, int how, lListElem *ep) 
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

   DENTER(TOP_LAYER, "write_sched_configuration");

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
      } else {
         const char *sge_root_dir = sge_get_root_dir(1);
         const char *sge_cell = sge_get_default_cell();

         sprintf(fname, "%s/%s/%s/.%s", sge_root_dir, sge_cell, 
                 COMMON_DIR, SCHED_CONF_FILE);
         sprintf(real_fname, "%s/%s/%s/%s", sge_root_dir, sge_cell, 
                 COMMON_DIR, SCHED_CONF_FILE);
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
             feature_get_product_name(FS_VERSION)) < 0)) {
      goto FPRINTF_ERROR;
   }  

   /* conf values needed for both SGE & SGEEE */
   FPRINTF((fp, "algorithm                  %s\n", lGetString(ep, SC_algorithm)));
   FPRINTF((fp, "schedule_interval          %s\n", lGetString(ep, SC_schedule_interval)));
   FPRINTF((fp, "maxujobs                   " u32 "\n", lGetUlong(ep, SC_maxujobs)));
   FPRINTF((fp, "queue_sort_method          %s\n", qsm2str(lGetUlong(ep, SC_queue_sort_method))));
   FPRINTF((fp, "user_sort                  %s\n", lGetUlong(ep, SC_user_sort)?"true":"false"));
   FPRINTF((fp, "job_load_adjustments       "));
   fret = uni_print_list(fp, NULL, 0, lGetList(ep, SC_job_load_adjustments), intprt_as_load_adjustment, delis, 0);
   if (fret < 0) {
      goto FPRINTF_ERROR;
   }
   FPRINTF((fp, "load_adjustment_decay_time %s\n", lGetString(ep, SC_load_adjustment_decay_time)));
   FPRINTF((fp, "load_formula               %s\n", lGetString(ep, SC_load_formula)));
   FPRINTF((fp, "schedd_job_info            %s\n", lGetString(ep, SC_schedd_job_info)));

   /* conf values needed for SGEEE */
   if (feature_is_enabled(FEATURE_SPOOL_ADD_ATTR)) {
      FPRINTF((fp, "sgeee_schedule_interval    %s\n", lGetString(ep, SC_sgeee_schedule_interval)));
      FPRINTF((fp, "halftime                   " u32 "\n", lGetUlong(ep, SC_halftime)));
      FPRINTF((fp, "usage_weight_list          "));
      fret = uni_print_list(fp, NULL, 0, lGetList(ep, SC_usage_weight_list), intprt_as_usage, delis, 0);
      if (fret < 0) {
         goto FPRINTF_ERROR;
      }
      FPRINTF((fp, "compensation_factor        %.10g\n", lGetDouble(ep, SC_compensation_factor)));
      FPRINTF((fp, "weight_user                %.10g\n", lGetDouble(ep, SC_weight_user)));
      FPRINTF((fp, "weight_project             %.10g\n", lGetDouble(ep, SC_weight_project)));
      FPRINTF((fp, "weight_jobclass            %.10g\n", lGetDouble(ep, SC_weight_jobclass)));
      FPRINTF((fp, "weight_department          %.10g\n", lGetDouble(ep, SC_weight_department)));
      FPRINTF((fp, "weight_job                 %.10g\n", lGetDouble(ep, SC_weight_job)));
      FPRINTF((fp, "weight_tickets_functional  " u32 "\n", lGetUlong(ep, SC_weight_tickets_functional)));
      FPRINTF((fp, "weight_tickets_share       " u32 "\n", lGetUlong(ep, SC_weight_tickets_share)));
      FPRINTF((fp, "weight_tickets_deadline    " u32 "\n", lGetUlong(ep, SC_weight_tickets_deadline)));
   }

   if (how != 0) {
      fclose(fp);
   }

   if (how == 2) {
      if (rename(fname, real_fname) == -1) {
         DEXIT;
         return NULL;
      } else {
         strcpy(fname, real_fname);
      }   
   } 
   DEXIT;
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

      /* --------- SC_weight_jobclass */
      if (!set_conf_double(alpp, clpp, fields, "weight_jobclass", ep, 
               SC_weight_jobclass, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_department */
      if (!set_conf_double(alpp, clpp, fields, "weight_department", ep, 
               SC_weight_department, 0)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_weight_job */
      if (!set_conf_double(alpp, clpp, fields, "weight_job", ep, 
               SC_weight_job, 0)) {
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

      /* --------- SC_weight_tickets_deadline */
      if (!set_conf_ulong(alpp, clpp, fields, "weight_tickets_deadline", ep, 
                           SC_weight_tickets_deadline)) {
         DEXIT;
         return -1;
      }

      /* --------- SC_sgeee_schedule_interval */
      if (!set_conf_timestr(alpp, clpp, fields, "sgeee_schedule_interval", ep, 
                              SC_sgeee_schedule_interval)) {
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
static lListElem *cull_read_in_schedd_conf(char *dirname, char *filename,
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
lList *read_sched_configuration(char *fname, int spool, lList **alpp) 
{
   lList *confl = NULL;
   lListElem *ep;
   int write_default_config = 0;
   SGE_STRUCT_STAT st;

   DENTER(TOP_LAYER, "read_sched_configuration");

   if (!SGE_STAT(path.sched_conf_file, &st)) {
      ep = cull_read_in_schedd_conf(NULL, fname, spool, NULL);
   } else {
      write_default_config = 1;
      ep = create_default_sched_conf();
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
      if (write_sched_configuration(1, 2, lFirst(confl)) == NULL) {
         answer_list_add(alpp, MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
         DEXIT;
         return NULL;
      }
   }

   DEXIT;
   return confl;
}


/******************************************************/
static lListElem *create_default_sched_conf()
{
   lListElem *ep, *added;

   DENTER(TOP_LAYER, "create_default_sched_conf");

   ep = lCreateElem(SC_Type);

   /* 
    * 
    * SGE & SGEEE
    *
    */
   lSetString(ep, SC_algorithm, "default");
   lSetString(ep, SC_schedule_interval, SCHEDULE_TIME);
   lSetUlong(ep, SC_maxujobs, MAXUJOBS);

   if (feature_is_enabled(FEATURE_SGEEE))
      lSetUlong(ep, SC_queue_sort_method, QSM_SHARE);
   else
      lSetUlong(ep, SC_queue_sort_method, QSM_LOAD);

   added = lAddSubStr(ep, CE_name, "np_load_avg", SC_job_load_adjustments, CE_Type);
   lSetString(added, CE_stringval, "0.50");

   lSetString(ep, SC_load_adjustment_decay_time, 
                     DEFAULT_LOAD_ADJUSTMENTS_DECAY_TIME);
   lSetString(ep, SC_load_formula, DEFAULT_LOAD_FORMULA);
   lSetString(ep, SC_schedd_job_info, SCHEDD_JOB_INFO);


   /* 
    * 
    * SGEEE
    *
    */
   if (feature_is_enabled(FEATURE_SGEEE)) {
      lSetString(ep, SC_sgeee_schedule_interval, SGEEE_SCHEDULE_TIME);
      lSetUlong(ep, SC_halftime, 168);

      added = lAddSubStr(ep, UA_name, USAGE_ATTR_CPU, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 1.00);
      added = lAddSubStr(ep, UA_name, USAGE_ATTR_MEM, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 0.0);
      added = lAddSubStr(ep, UA_name, USAGE_ATTR_IO, SC_usage_weight_list, UA_Type);
      lSetDouble(added, UA_value, 0.0);

      lSetDouble(ep, SC_compensation_factor, 5);
      lSetDouble(ep, SC_weight_user, 0.2);
      lSetDouble(ep, SC_weight_project, 0.2);
      lSetDouble(ep, SC_weight_jobclass, 0.2);
      lSetDouble(ep, SC_weight_department, 0.2);
      lSetDouble(ep, SC_weight_job, 0.2);
      lSetUlong(ep, SC_weight_tickets_functional, 0);
      lSetUlong(ep, SC_weight_tickets_share, 0);
      lSetUlong(ep, SC_weight_tickets_deadline, 0);
   }

   DEXIT;
   return ep;
}
