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
#include <string.h>
#include <fnmatch.h>

#include "sge.h"
#include "sge_pe.h"
#include "sge_ja_task.h"
#include "sge_pe_qmaster.h"
#include "sge_host_qmaster.h"
#include "sge_event_master.h"
#include "config_file.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_job_schedd.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_job.h"
#include "sge_qinstance.h"
#include "sge_userset.h"
#include "sge_utility.h"
#include "sge_utility_qmaster.h"

#include "sge_persistence_qmaster.h"
#include "spool/sge_spooling.h"

#include "msg_common.h"
#include "msg_qmaster.h"


static char object_name[] = "parallel environment";


int pe_mod(
lList **alpp,
lListElem *new_pe,
lListElem *pe, /* reduced */
int add,
const char *ruser,
const char *rhost,
gdi_object_t *object,
int sub_command 
) {
   int ret;
   const char *s, *pe_name;

   DENTER(TOP_LAYER, "pe_mod");

   /* ---- PE_name */
   if (add) {
      if (attr_mod_str(alpp, pe, new_pe, PE_name, object->object_name))
         goto ERROR;
   }
   pe_name = lGetString(new_pe, PE_name);

   /* Name has to be a valid filename without pathchanges */
   if (add && verify_str_key(alpp, pe_name, MSG_OBJ_PE)) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* ---- PE_slots */
   attr_mod_ulong(pe, new_pe, PE_slots, "slots");

   /* ---- PE_control_slaves */
   attr_mod_bool(pe, new_pe, PE_control_slaves, "control_slaves");

   /* ---- PE_job_is_first_task */
   attr_mod_bool(pe, new_pe, PE_job_is_first_task, "job_is_first_task");

   /* ---- PE_user_list */
   if (lGetPosViaElem(pe, PE_user_list)>=0) {
      DPRINTF(("got new PE_user_list\n"));
      /* check user_lists */
      normalize_sublist(pe, PE_user_list);
      if (userset_list_validate_acl_list(lGetList(pe, PE_user_list), alpp)!=STATUS_OK)
         goto ERROR;

      attr_mod_sub_list(alpp, new_pe, PE_user_list, 
         US_name, pe, sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_PE, 0); 
   }

   /* ---- PE_xuser_list */
   if (lGetPosViaElem(pe, PE_xuser_list)>=0) {
      DPRINTF(("got new QU_axcl\n"));
      /* check xuser_lists */
      normalize_sublist(pe, PE_xuser_list);
      if (userset_list_validate_acl_list(lGetList(pe, PE_xuser_list), alpp)!=STATUS_OK)
         goto ERROR;
      attr_mod_sub_list(alpp, new_pe, PE_xuser_list,
         US_name, pe, sub_command, SGE_ATTR_XUSER_LISTS, SGE_OBJ_PE, 0);      
   }

   if (lGetPosViaElem(pe, PE_xuser_list)>=0 || lGetPosViaElem(pe, PE_user_list)>=0) {
      if (multiple_occurances(
            alpp,
            lGetList(new_pe, PE_user_list),
            lGetList(new_pe, PE_xuser_list),
            US_name,
            pe_name, object_name)) {
         goto ERROR;
      }
   }

   if (attr_mod_procedure(alpp, pe, new_pe, PE_start_proc_args, "start_proc_args", pe_variables)) goto ERROR;
   if (attr_mod_procedure(alpp, pe, new_pe, PE_stop_proc_args, "stop_proc_args", pe_variables)) goto ERROR;

   /* -------- PE_allocation_rule */
   if (lGetPosViaElem(pe, PE_allocation_rule)>=0) {
      s = lGetString(pe, PE_allocation_rule);
      if (!s)  {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
               lNm2Str(PE_allocation_rule), "validate_pe"));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }

      if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
         ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
         answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
         DEXIT;
         return STATUS_EEXIST;
      }
      lSetString(new_pe, PE_allocation_rule, s);
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* -------- PE_urgency_slots */
      if (lGetPosViaElem(pe, PE_urgency_slots)>=0) {
         s = lGetString(pe, PE_urgency_slots);
         if (!s)  {
            ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
                  lNm2Str(PE_allocation_rule), "validate_pe"));
            answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
            DEXIT;
            return STATUS_EEXIST;
         }

         if ((ret=pe_validate_urgency_slots(alpp, s))!=STATUS_OK) {
            DEXIT;
            return ret;
         }
         lSetString(new_pe, PE_urgency_slots, s);
      }
   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}


int pe_spool(lList **alpp, lListElem *pep, gdi_object_t *object) 
{
   lList *answer_list = NULL;
   bool dbret;

   DENTER(TOP_LAYER, "pe_spool");

   dbret = spool_write_object(&answer_list, spool_get_default_context(), pep, 
                              lGetString(pep, PE_name), SGE_TYPE_PE);
   answer_list_output(&answer_list);

   if (!dbret) {
      answer_list_add_sprintf(alpp, STATUS_EUNKNOWN, 
                              ANSWER_QUALITY_ERROR, 
                              MSG_PERSISTENCE_WRITE_FAILED_S,
                              lGetString(pep, PE_name));
   }

   DEXIT;
   return dbret ? 0 : 1;
}

int pe_success(lListElem *ep, lListElem *old_ep, gdi_object_t *object) 
{
   const char *pe_name;

   DENTER(TOP_LAYER, "pe_success");

   pe_name = lGetString(ep, PE_name);

   sge_add_event(NULL, 0, old_ep?sgeE_PE_MOD:sgeE_PE_ADD, 0, 0, 
                 pe_name, NULL, NULL, ep);
   lListElem_clear_changed_info(ep);

   DEXIT;
   return 0;
}

int sge_del_pe(lListElem *pep, lList **alpp, char *ruser, char *rhost) 
{
   int pos;
   lListElem *ep = NULL;
   const char *pe = NULL;

   DENTER(TOP_LAYER, "sge_del_pe");

   if ( !pep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((pos = lGetPosViaElem(pep, PE_name)) < 0) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_name), SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   pe = lGetPosString(pep, pos);
   if (!pe) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((ep=pe_list_locate(Master_Pe_List, pe))==NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, object_name, pe));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* 
    * Try to find references in other objects
    */
   {
      lList *local_answer_list = NULL;

      if (pe_is_referenced(ep, &local_answer_list, Master_Job_List,
                           *(object_type_get_master_list(SGE_TYPE_CQUEUE)))) {
         lListElem *answer = lFirst(local_answer_list);

         ERROR((SGE_EVENT, "denied: %s", lGetString(answer, AN_text)));
         answer_list_add(alpp, SGE_EVENT, STATUS_EUNKNOWN, 
                         ANSWER_QUALITY_ERROR);
         local_answer_list = lFreeList(local_answer_list);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   /* remove host file */
   if (!sge_event_spool(alpp, 0, sgeE_PE_DEL,
                        0, 0, pe, NULL, NULL, NULL, NULL, NULL, true, true)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object_name, pe));
      answer_list_add(alpp, SGE_EVENT, STATUS_EEXIST, ANSWER_QUALITY_ERROR);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* delete found pe element */
   lRemoveElem(Master_Pe_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost, pe, object_name ));
   answer_list_add(alpp, SGE_EVENT, STATUS_OK, ANSWER_QUALITY_INFO);
   DEXIT;
   return STATUS_OK;
}

void debit_all_jobs_from_pes(
lList *pe_list  
) {
   const char *pe_name;
   lListElem *jep, *pep;
   int slots;

   DENTER(TOP_LAYER, "debit_all_jobs_from_pes");

   for_each (pep, pe_list) {
  
      pe_name = lGetString(pep, PE_name);
      DPRINTF(("debiting from pe %s:\n", pe_name));

      for_each(jep, Master_Job_List) {
         lListElem *jatep;

         slots = 0;
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if ((ISSET(lGetUlong(jatep, JAT_status), JRUNNING) ||      /* is job running  */
                   ISSET(lGetUlong(jatep, JAT_status), JTRANSFERING))     /* or transfering  */
                && lGetString(jatep, JAT_granted_pe)                     /* is job parallel */
                && !strcmp(pe_name, lGetString(jatep, JAT_granted_pe))) {/* this pe         */
               slots += sge_granted_slots(lGetList(jatep, JAT_granted_destin_identifier_list));
            }  
         }
         debit_job_from_pe(pep, slots, lGetUlong(jep, JB_job_number));
      }
   }
   DEXIT;
   return;
}



void debit_job_from_pe(
lListElem *pep, 
int slots,
u_long32 job_id  /* needed for job logging */
) {
   int n;

   DENTER(TOP_LAYER, "debit_job_from_pe");

   if (pep) {
      n = (int)lGetUlong(pep, PE_used_slots);
      n += slots;

      lSetUlong(pep, PE_used_slots, (u_long32)n);
   }
   DEXIT;
   return;
}


void reverse_job_from_pe(
lListElem *pep,
int slots,
u_long32 job_id  /* needed for job logging */
) {
   int n;

   DENTER(TOP_LAYER, "reverse_job_from_pe");

   if (pep) {
      n = (int)lGetUlong(pep, PE_used_slots);
      n -= slots;

      if (n < 0) {
         ERROR((SGE_EVENT, MSG_PE_USEDSLOTSTOOBIG_S, lGetString(pep, PE_name)));
      }

      lSetUlong(pep, PE_used_slots, (u_long32)n);
   }

   DEXIT;
   return;
}

