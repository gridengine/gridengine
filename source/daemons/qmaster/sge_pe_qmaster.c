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
#include "utility.h"
#include "def.h"
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_eventL.h"
#include "sge_answerL.h"
#include "sge_usersetL.h"
#include "sge_pe_qmaster.h"
#include "job_log.h"
#include "sge_queue_qmaster.h"
#include "sge_host_qmaster.h"
#include "read_write_pe.h"
#include "sge_m_event.h"
#include "config_file.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_job_schedd.h"
#include "gdi_utility_qmaster.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"

extern lList *Master_Pe_List;
extern lList *Master_Userset_List;
extern lList *Master_Job_List;

static char object_name[] = "parallel environment";


int pe_mod(
lList **alpp,
lListElem *new_pe,
lListElem *pe, /* reduced */
int add,
char *ruser,
char *rhost,
gdi_object_t *object,
int sub_command 
) {
   char *s, *pe_name;

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
   attr_mod_ulong(pe, new_pe, PE_control_slaves, "control_slaves");

   /* ---- PE_job_is_first_task */
   attr_mod_ulong(pe, new_pe, PE_job_is_first_task, "job_is_first_task");

   /* ---- PE_queue_list */
   if (lGetPosViaElem(pe, PE_queue_list)>=0) {
      if (verify_qr_list(alpp, lGetList(pe, PE_queue_list), MSG_OBJ_QLIST,
                  MSG_OBJ_PE, pe_name)!=STATUS_OK /* && !startup */)
         goto ERROR;

      attr_mod_sub_list(alpp, new_pe, PE_queue_list, 
            QR_name, pe, sub_command, SGE_ATTR_QUEUE_LIST, SGE_OBJ_PE, 0);
   }

   /* ---- PE_user_list */
   if (lGetPosViaElem(pe, PE_user_list)>=0) {
      DPRINTF(("got new PE_user_list\n"));
      /* check user_lists */
      normalize_sublist(pe, PE_user_list);
      if (verify_acl_list(alpp, lGetList(pe, PE_user_list), MSG_OBJ_USERLIST,
            object_name, pe_name)!=STATUS_OK)
         goto ERROR;

      attr_mod_sub_list(alpp, new_pe, PE_user_list, 
         US_name, pe, sub_command, SGE_ATTR_USER_LISTS, SGE_OBJ_PE, 0); 
   }

   /* ---- PE_xuser_list */
   if (lGetPosViaElem(pe, PE_xuser_list)>=0) {
      DPRINTF(("got new QU_axcl\n"));
      /* check xuser_lists */
      normalize_sublist(pe, PE_xuser_list);
      if (verify_acl_list(alpp, lGetList(pe, PE_xuser_list), MSG_OBJ_XUSERLIST,
            object_name, pe_name)!=STATUS_OK)
         goto ERROR;
      attr_mod_sub_list(alpp, new_pe, PE_xuser_list,
         US_name, pe, sub_command, SGE_ATTR_XUSER_LISTS, SGE_OBJ_PE, 0);      
   }

   if (lGetPosViaElem(pe, PE_xuser_list)>=0 || lGetPosViaElem(pe, PE_user_list)>=0) {
      if (multiple_occurrencies(
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
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }

      if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
         ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EEXIST;
      }
      lSetString(new_pe, PE_allocation_rule, s);
   }

   DEXIT;
   return 0;

ERROR:
   DEXIT;
   return STATUS_EUNKNOWN;
}

int pe_spool(
lList **alpp,
lListElem *pep,
gdi_object_t *object 
) {
   DENTER(TOP_LAYER, "pe_spool");

   if (!write_pe(1, 2, pep)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, 
            object->object_name, lGetString(pep, PE_name)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }
   DEXIT;
   return 0;
}

int pe_success(
lListElem *ep,
lListElem *old_ep,
gdi_object_t *object 
) {
   char *pe_name;

   DENTER(TOP_LAYER, "pe_success");

   pe_name = lGetString(ep, PE_name);

   sge_change_queue_version_qr_list(lGetList(ep, PE_queue_list), 
         old_ep ? lGetList(old_ep, PE_queue_list) : NULL, 
         "parallel environment", pe_name);

   sge_add_event(old_ep?sgeE_PE_MOD:sgeE_PE_ADD, 0, 0, pe_name, ep);

   DEXIT;
   return 0;
}

/* ------------------------------------------------------------

   deletes an parallel environment object

*/
int sge_del_pe(
lListElem *pep,
lList **alpp,
char *ruser,
char *rhost 
) {
   int pos;
   lListElem *ep;
   char *pe;

   DENTER(TOP_LAYER, "sge_del_pe");

   if ( !pep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((pos = lGetPosViaElem(pep, PE_name)) < 0) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_name), SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   pe = lGetPosString(pep, pos);
   if (!pe) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if ((ep=sge_locate_pe(pe))==NULL) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, object_name, pe));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* remove host file */
   if (sge_unlink(PE_DIR, pe)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANTSPOOL_SS, object_name, pe));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }
   sge_add_event(sgeE_PE_DEL, 0, 0, pe, NULL);
   sge_change_queue_version_qr_list(lGetList(ep, PE_queue_list), 
      NULL, MSG_OBJ_PE, pe);

   /* delete found pe element */
   lRemoveElem(Master_Pe_List, ep);

   INFO((SGE_EVENT, MSG_SGETEXT_REMOVEDFROMLIST_SSSS, 
         ruser, rhost, pe, object_name ));
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   DEXIT;
   return STATUS_OK;
}

/* try to find a pe that matches with 
   requested wildcard expression for pe */ 
lListElem *sge_match_pe(
char *wildcard 
) {
   lListElem *pep;
   for_each (pep, Master_Pe_List)
      if (!fnmatch(wildcard, lGetString(pep, PE_name), 0))
         return pep;
   return NULL;
}

lListElem *sge_locate_pe(
char *pe_name 
) {
   return lGetElemStr(Master_Pe_List, PE_name, pe_name);
}

void debit_all_jobs_from_pes(
lList *pe_list  
) {
   char *pe_name;
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
                   ISSET(lGetUlong(jatep, JAT_status), JTRANSITING))     /* or transisting  */
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
  
   if (is_active_job_logging()) {
      sprintf(SGE_EVENT, MSG_PE_DEBITSLOTS_IS, slots, lGetString(pep, PE_name));
      job_log(job_id, SGE_EVENT, prognames[me.who], me.unqualified_hostname);
   }

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

   if (is_active_job_logging()) {
      sprintf(SGE_EVENT, MSG_PE_REVERSESLOTS_IS, slots, lGetString(pep, PE_name));
      job_log(job_id, SGE_EVENT, prognames[me.who], me.unqualified_hostname);
   }

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

int validate_pe(
int startup,
lListElem *pep,
lList **alpp 
) {
   char *s;
   char *pe_name;
   int ret;

   DENTER(TOP_LAYER, "validate_pe");

   pe_name = lGetString(pep, PE_name);
   if (pe_name && verify_str_key(alpp, pe_name, MSG_OBJ_PE)) {
      ERROR((SGE_EVENT, "Invalid character in pe name of pe "SFQ, pe_name));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
      DEXIT;
      return STATUS_EEXIST; 
   }

   /* register our error function for use in replace_params() */
   config_errfunc = error;

   /* -------- start_proc_args */
   s = lGetString(pep, PE_start_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STARTPROCARGS_SS, pe_name, err_msg));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0); 
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- stop_proc_args */
   s = lGetString(pep, PE_stop_proc_args);
   if (s && replace_params(s, NULL, 0, pe_variables )) {
      ERROR((SGE_EVENT, MSG_PE_STOPPROCARGS_SS, pe_name, err_msg));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- allocation_rule */
   s = lGetString(pep, PE_allocation_rule);
   if (!s)  {
      ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
            lNm2Str(PE_allocation_rule), "validate_pe"));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   if (replace_params(s, NULL, 0, pe_alloc_rule_variables )) {
      ERROR((SGE_EVENT, MSG_PE_ALLOCRULE_SS, pe_name, err_msg));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   /* -------- PE_queue_list */
   if ((ret=verify_qr_list(alpp, lGetList(pep, PE_queue_list), MSG_OBJ_QLIST, 
               MSG_OBJ_PE, pe_name))!=STATUS_OK && !startup) {
      DEXIT;
      return ret;
   }

   /* -------- PE_user_list */
   if ((ret=verify_acl_list(alpp, lGetList(pep, PE_user_list), MSG_OBJ_USERLIST, 
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   /* -------- PE_xuser_list */
   if ((ret=verify_acl_list(alpp, lGetList(pep, PE_xuser_list), MSG_OBJ_XUSERLIST, 
               MSG_OBJ_PE, pe_name))!=STATUS_OK) {
      DEXIT;
      return ret;
   }

   DEXIT;
   return STATUS_OK;
}
