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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32NATIVE
#	include <unistd.h>
#endif


#include "sge_conf.h"
#include "sgermon.h"
#include "sge_gdi_intern.h"
#include "sge_string.h"
#include "sge_complexL.h"
#include "sge_hostL.h"
#include "sge_usageL.h"
#include "sge_schedconfL.h"
#include "sge_answerL.h"
#include "cull.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "schedd_conf.h"
/* #include "sge_c_event.h" */
#include "sge_feature.h"
#include "cull_parse_util.h"
#include "sge_log.h"
#include "parse_range.h"
#include "msg_schedd.h"
#include "sge_dirent.h"
#include "sge_complex_schedd.h"

extern lList *Master_Complex_List;

sge_schedd_conf_type scheddconf = { 
   NULL, 0, 0, 0, 0, NULL, 0, NULL 
};
lList *schedd_config_list = NULL;

typedef struct {
  char *name;              /* name of parameter */                              
  char *value;             /* value of parameter */            
  int isSet;               /* 0 | 1 -> is already set */        
  char *envp;              /* pointer to environment variable */
} tScheddConfEntry;

static int schedd_conf_is_valid_load_formula(lListElem *sc, 
                                             lList **answer_list,
                                             lList *cmplx_list);

static intprt_type load_adjustment_fields[] = { CE_name, CE_stringval, 0 };
static intprt_type usage_fields[] = { UA_name, UA_value, 0 };
static const char *delis[] = {"=", ",", ""};

int sc_set(
lList **alpp,             /* AN_Type */ 
sge_schedd_conf_type *sc, /* if NULL we just check sc_ep */
lListElem *sc_ep,         /* SC_Type */  
u_long32 *si,             /* here scheduling interval gets written */
lList *cmplx_list
) {
   char tmp_buffer[1024], tmp_error[1024];
   u_long32 uval;
   const char *s;
   lList *lval;
   double dval;

   DENTER(TOP_LAYER, "sc_set");

   /* --- SC_algorithm */
   s = lGetString(sc_ep, SC_algorithm);
   if ( !s || (strcmp(s, "default") && strcmp(s, "simple_sched") 
      && strncmp(s, "ext_", 4))) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_ALGORITHMNOVALIDNAME_S, s)); 
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
      DEXIT;
      return -1;
   }
   if (sc)
      sc->algorithm = sge_strdup(sc->algorithm, s);
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXASY_SS , s, "algorithm"));
     
   /* --- SC_schedule_interval */
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, 
       s=lGetString(sc_ep, SC_schedule_interval), tmp_error, 
       sizeof(tmp_error),0) ) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , 
         "schedule_interval", tmp_error));    
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return -1;
   }
   if (sc)
      sc->schedule_interval = uval;
   if (si)
      *si = uval;
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS , s, "schedule_interval"));

   /* --- SC_maxujobs */
   uval = lGetUlong(sc_ep, SC_maxujobs);
   if (sc)
      sc->maxujobs = uval;
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US, u32c( uval), "maxujobs"));

   /* --- SC_queue_sort_method (was: SC_sort_seq_no) */
   uval = lGetUlong(sc_ep, SC_queue_sort_method);
   if (sc)
      sc->queue_sort_method = uval;
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "queue_sort_method"));

   /* --- SC_user_sort */
   uval = lGetUlong(sc_ep, SC_user_sort);
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS , uval?MSG_TRUE:MSG_FALSE, "user_sort"));

   /* --- SC_job_load_adjustments */
#if 1
   lval = lGetList(sc_ep, SC_job_load_adjustments);
   if (sc) {
      lFreeList(sc->job_load_adjustments);
      sc->job_load_adjustments = lCopyList("", lval);
   }
   if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer),    
      lval, load_adjustment_fields, delis, 0) < 0) {
      DEXIT;
      return -1;
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "job_load_adjustments"));
#else
   lval = lGetList(sc_ep, SC_job_load_adjustments);

   ret=sge_fill_requests(lGetList(qep, nm), Master_Complex_List, 1, 0, 1);
   if (ret) {
   /* error message gets written by sge_fill_requests into SGE_EVENT */
   sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, NUM_AN_ERROR);
   DEXIT;
   return STATUS_EUNKNOWN;
   }
 
   if (sc) {
      lFreeList(sc->job_load_adjustments);
      sc->job_load_adjustments = lCopyList("", lval);
   }
   if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer),    
      lval, load_adjustment_fields, delis, 0) < 0) {
      DEXIT;
      return -1;
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "job_load_adjustments"));
#endif

   /* --- SC_load_adjustment_decay_time */
   if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, 
         s=lGetString(sc_ep, SC_load_adjustment_decay_time), tmp_error, 
         sizeof(tmp_error),0)) {
      SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS, 
         "load_adjustment_decay_time", tmp_error));    
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
      DEXIT;
      return -1;
   }
   if (sc)
      sc->load_adjustment_decay_time = uval;
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "load_adjustment_decay_time"));

   /* --- SC_load_formula */
   if (cmplx_list != NULL &&
       !schedd_conf_is_valid_load_formula(sc_ep, alpp, cmplx_list)) {
      DEXIT;
      return -1;
   }
   if (sc) {
      sc->load_formula = sge_strdup(sc->load_formula, 
         lGetString(sc_ep, SC_load_formula));
      strip_blanks(sc->load_formula);
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, 
         lGetString(sc_ep, SC_load_formula), "load_formula"));

   /* --- SC_schedd_job_info */
   {
      char buf[4096];
      char* key;
      int ikey = 0;
      lList *rlp=NULL, *alp=NULL;

      strcpy(buf, lGetString(sc_ep, SC_schedd_job_info));
      /* on/off or watch a set of jobs */
      key = strtok(buf, " \t");
      if (!strcmp("true", key)) 
         ikey = SCHEDD_JOB_INFO_TRUE;
      else if (!strcmp("false", key)) 
         ikey = SCHEDD_JOB_INFO_FALSE;
      else if (!strcmp("job_list", key)) 
         ikey = SCHEDD_JOB_INFO_JOB_LIST;
      else {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_SCHEDDJOBINFONOVALIDPARAM ));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return -1;
      }
      /* check list of groups */
      if (ikey == SCHEDD_JOB_INFO_JOB_LIST) {
         key = strtok(NULL, "\n");
         if (!(rlp=parse_ranges(key, 0, 0, &alp, NULL, INF_NOT_ALLOWED))) {
            lFreeList(alp);
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_SCHEDDJOBINFONOVALIDJOBLIST));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            DEXIT;
            return -1;
         }   
      }
      if (sc) {
         if (sc->schedd_job_info_list) 
            sc->schedd_job_info_list = lFreeList(sc->schedd_job_info_list);
         sc->schedd_job_info_list = rlp;
         sc->schedd_job_info = ikey;
      }
      else
         lFreeList(rlp);
   }
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, lGetString(sc_ep, SC_schedd_job_info), 
      "schedd_job_info"));
  
   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* --- SC_sgeee_schedule_interval */
      if (!extended_parse_ulong_val(NULL, &uval, TYPE_TIM, 
            s=lGetString(sc_ep, SC_sgeee_schedule_interval), tmp_error, 
            sizeof(tmp_error),0)) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_XISNOTAY_SS , 
            "sgeee_schedule_interval", tmp_error));    
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
         DEXIT;
         return -1;
      }
      if (sc)
         sc->sgeee_schedule_interval = uval;
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "sgeee_schedule_interval"));

      /* --- SC_halftime */
      uval = lGetUlong(sc_ep, SC_halftime);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US ,  u32c (uval), "halftime"));

      /* --- SC_usage_weight_list */
      if (uni_print_list(NULL, tmp_buffer, sizeof(tmp_buffer),    
         lGetList(sc_ep, SC_usage_weight_list), usage_fields, delis, 0) < 0) {
         DEXIT;
         return -1;
      }
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, tmp_buffer, "usage_weight_list"));

      /* --- SC_compensation_factor */
      dval = lGetDouble(sc_ep, SC_compensation_factor);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "compensation_factor"));

      /* --- SC_weight_user */
      dval = lGetDouble(sc_ep, SC_weight_user);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_user"));

      /* --- SC_weight_project */
      dval = lGetDouble(sc_ep, SC_weight_project);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_project"));

      /* --- SC_weight_jobclass */
      dval = lGetDouble(sc_ep, SC_weight_jobclass);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_jobclass"));

      /* --- SC_weight_department */
      dval = lGetDouble(sc_ep, SC_weight_department);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_department"));

      /* --- SC_weight_job */
      dval = lGetDouble(sc_ep, SC_weight_job);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_6FS, dval, "weight_job"));

      /* --- SC_weight_tickets_functional */
      uval = lGetUlong(sc_ep, SC_weight_tickets_functional);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_functional"));

      /* --- SC_weight_tickets_share */
      uval = lGetUlong(sc_ep, SC_weight_tickets_share);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_share"));

      /* --- SC_weight_tickets_deadline */
      uval = lGetUlong(sc_ep, SC_weight_tickets_deadline);
      INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_US,  u32c (uval), "weight_tickets_deadline"));
   }

   DEXIT;
   return 0;
}

static int schedd_conf_is_valid_load_formula(lListElem *schedd_conf,
                                             lList **answer_list,
                                             lList *cmplx_list) 
{
   const char *load_formula = NULL;
   int ret = 1;
   DENTER(TOP_LAYER, "schedd_conf_is_valid_load_formula");

   /* Modify input */
   {
      char *new_load_formula = NULL;
   
      load_formula = lGetString(schedd_conf, SC_load_formula);
      new_load_formula = sge_strdup(new_load_formula, load_formula);
      strip_blanks(new_load_formula);
      lSetString(schedd_conf, SC_load_formula, new_load_formula);
      free(new_load_formula);
   }
   load_formula = lGetString(schedd_conf, SC_load_formula);

   /* Check for keyword 'none' */
   if (ret == 1) {
      if (!strcasecmp(load_formula, "none")) {
         sge_add_answer(answer_list, MSG_NONE_NOT_ALLOWED, STATUS_ESYNTAX, 0);
         ret = 0;
      }
   }

   /* Check complex attributes and type */
   if (ret == 1) {
      const char *delimitor = "+-*";
      const char *attr, *next_attr;

      next_attr = sge_strtok(load_formula, delimitor);
      while ((attr = next_attr)) {
         lListElem *cmplx_attr = NULL;

         next_attr = sge_strtok(NULL, delimitor);

         cmplx_attr = sge_locate_complex_attr(attr, cmplx_list);
         if (cmplx_attr != NULL) {
            int type = lGetUlong(cmplx_attr, CE_valtype);
   
            if (type == TYPE_STR || type == TYPE_CSTR || type == TYPE_HOST) {
               SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_WRONGTYPE_ATTRIBUTE_S, attr));
               sge_add_answer(answer_list, SGE_EVENT, STATUS_ESYNTAX, 0); 
               ret = 0;
            }
         } else {
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_NOTEXISTING_ATTRIBUTE_S, attr));
            sge_add_answer(answer_list, SGE_EVENT, STATUS_ESYNTAX, 0);
            ret = 0;
         }
      }
   }
   DEXIT;
   return ret;
}




