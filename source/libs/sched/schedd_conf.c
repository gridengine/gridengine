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
#include "sge_string.h"
#include "sge_usageL.h"
#include "sge_schedd_conf.h"
#include "cull.h"
#include "commlib.h"
#include "sge_parse_num_par.h"
#include "schedd_conf.h"
#include "sge_feature.h"
#include "cull_parse_util.h"
#include "sge_log.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_schedd_conf.h"
#include "sge_centry.h"

#include "msg_schedd.h"

sge_schedd_conf_type scheddconf = { 
   NULL, 0, 0, 0, 0, 0, NULL, 0, NULL 
};
lList *schedd_config_list = NULL;

typedef struct {
  char *name;              /* name of parameter */                              
  char *value;             /* value of parameter */            
  int isSet;               /* 0 | 1 -> is already set */        
  char *envp;              /* pointer to environment variable */
} tScheddConfEntry;


static intprt_type load_adjustment_fields[] = { CE_name, CE_stringval, 0 };
static intprt_type usage_fields[] = { UA_name, UA_value, 0 };
static const char *delis[] = {"=", ",", ""};

int sc_set(
lList **alpp,             /* AN_Type */ 
sge_schedd_conf_type *sc, /* if NULL we just check sc_ep */
lListElem *sc_ep,         /* SC_Type */  
u_long32 *si,             /* here scheduling interval gets written */
lList *centry_list
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
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
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
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
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
   uval = lGetBool(sc_ep, SC_user_sort);
   if(sc) {
      sc->user_sort = uval;
   }
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

   ret=centry_list_fill_request(lGetList(qep, nm), Master_CEntry_List, true, false, true);
   if (ret) {
   /* error message gets written by centry_list_fill_request into SGE_EVENT */
   answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
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
      answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
      DEXIT;
      return -1;
   }
   if (sc)
      sc->load_adjustment_decay_time = uval;
   INFO((SGE_EVENT, MSG_ATTRIB_USINGXFORY_SS, s, "load_adjustment_decay_time"));

   /* --- SC_load_formula */
   if (centry_list != NULL &&
       !schedd_conf_is_valid_load_formula(sc_ep, alpp, centry_list)) {
      DEXIT;
      return -1;
   }
   if (sc) {
      sc->load_formula = sge_strdup(sc->load_formula, 
         lGetString(sc_ep, SC_load_formula));
      sge_strip_blanks(sc->load_formula);
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
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
         DEXIT;
         return -1;
      }
      /* check list of groups */
      if (ikey == SCHEDD_JOB_INFO_JOB_LIST) {
         key = strtok(NULL, "\n");
         range_list_parse_from_string(&rlp, &alp, key, 0, 0, INF_NOT_ALLOWED);
         if (rlp == NULL) {
            lFreeList(alp);
            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_ATTRIB_SCHEDDJOBINFONOVALIDJOBLIST));
            answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
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
         answer_list_add(alpp, SGE_EVENT, STATUS_ESYNTAX, ANSWER_QUALITY_ERROR);
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

int set_user_sort(int user_sort) 
{
   scheddconf.user_sort = user_sort;
   return scheddconf.user_sort;
}

int get_user_sort(void)
{
   return scheddconf.user_sort;
}
