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

#include <fnmatch.h>
#include <string.h>

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif   

#include "sgermon.h"
#include "sge_string.h"
#include "sge_log.h"
#include "cull_list.h"
#include "symbols.h"
#include "sge.h"

#include "commlib.h"
#include "sge_hostname.h"
#include "sge_parse_num_par.h"

#include "sge_manop.h"

#include "parse.h"
#include "sge_dstring.h"
#include "sge_answer.h"
#include "sge_attr.h"
#include "sge_calendar.h"
#include "sge_centry.h"
#include "sge_ckpt.h"
#include "sge_cqueue.h"
#include "sge_job.h"
#include "sge_object.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_ja_task.h"
#include "sge_mesobj.h"
#include "sge_pe.h"
#include "sge_qref.h"
#include "sge_range.h"
#include "sge_str.h"
#include "sge_userset.h"
#include "sge_subordinate.h"
#include "sge_host.h"
#include "sge_load.h"
#include "sge_utility.h"

#include "sge_select_queue.h"
#include "sge_resource_utilization.h"

#include "msg_common.h"
#include "msg_sgeobjlib.h"

#define QINSTANCE_LAYER TOP_LAYER

/* EB: ADOC: add commets */

/****** sgeobj/qinstance/qinstance_list_locate() ******************************
*  NAME
*     qinstance_list_locate() -- find a qinstance 
*
*  SYNOPSIS
*     lListElem * 
*     qinstance_list_locate(const lList *this_list, 
*                           const char *hostname, const char *cqueue_name) 
*
*  FUNCTION
*     Find a qinstance in "this_list" which is part of the cluster queue
*     with the name "cqueue_name" and resides on the host with the name 
*     "hostname".
*
*  INPUTS
*     const lList *this_list  - QU_Type list
*     const char *hostname    - hostname 
*     const char *cqueue_name - cluster queue name 
*
*  RESULT
*     lListElem * - QU_Type element
*
*  NOTES
*     MT-NOTE: qinstance_list_locate() is MT safe 
*******************************************************************************/
lListElem *
qinstance_list_locate(const lList *this_list, const char *hostname,
                      const char *cqueue_name) 
{
   lListElem *ret = NULL;

   if (cqueue_name == NULL) {
      ret = lGetElemHost(this_list, QU_qhostname, hostname);
   } else {
      for_each(ret, this_list) {
         const char *qname = lGetString(ret, QU_qname);
         const char *hname = lGetHost(ret, QU_qhostname);

         if (!strcmp(qname, cqueue_name) && !sge_hostcmp(hname, hostname)) {
            break;
         }
      }
   }
   return ret;
}

/****** sgeobj/qinstance/qinstance_list_locate2() *****************************
*  NAME
*     qinstance_list_locate2() -- find a qinstance using the fullname 
*
*  SYNOPSIS
*     lListElem * 
*     qinstance_list_locate2(const lList *queue_list, 
*                            const char *full_name) 
*
*  FUNCTION
*     find a qinstance using the fullname 
*
*  INPUTS
*     const lList *queue_list - QU_Type list 
*     const char *full_name   - fullname of the qinstance (<cqueue>@<hostname>)
*
*  RESULT
*     lListElem * - QU_type element
*
*  NOTES
*     MT-NOTE: qinstance_list_locate2() is MT safe 
*******************************************************************************/
lListElem *
qinstance_list_locate2(const lList *queue_list, const char *full_name)
{
   return lGetElemStr(queue_list, QU_full_name, full_name);
}

/****** sgeobj/qinstance/qinstance_get_name() *********************************
*  NAME
*     qinstance_get_name() -- returns the fullname of a qinstance object 
*
*  SYNOPSIS
*     const char * 
*     qinstance_get_name(const lListElem *this_elem, 
*                        dstring *string_buffer) 
*
*  FUNCTION
*     Returns the fullname of a qinstance object 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type 
*     dstring *string_buffer     - dynamic string buffer 
*
*  RESULT
*     const char * - pointer to the internal string buffer of "string_buffer"
*
*  NOTES
*     MT-NOTE: qinstance_get_name() is MT safe 
*******************************************************************************/
const char *
qinstance_get_name(const lListElem *this_elem, dstring *string_buffer)
{
   const char *ret = NULL;

   if (this_elem != NULL && string_buffer != NULL) {
      ret = sge_dstring_sprintf(string_buffer, SFN"@"SFN,
                                lGetString(this_elem, QU_qname),
                                lGetHost(this_elem, QU_qhostname));
   } 
   return ret;
}

/****** sgeobj/qinstance/qinstance_list_set_tag() *****************************
*  NAME
*     qinstance_list_set_tag() -- tag a list of qinstances 
*
*  SYNOPSIS
*     void 
*     qinstance_list_set_tag(lList *this_list, u_long32 tag_value) 
*
*  FUNCTION
*     Tag a list of qinstances ("this_list") with "tag_value". 
*
*  INPUTS
*     lList *this_list   - QU_Type list
*     u_long32 tag_value - unsingned long value (not a bitmask) 
*
*  RESULT
*     void - None
*
*  NOTES
*     MT-NOTE: qinstance_list_set_tag() is MT safe 
*******************************************************************************/
void
qinstance_list_set_tag(lList *this_list, u_long32 tag_value)
{
   if (this_list != NULL) {
      lListElem *qinstance = NULL;

      for_each(qinstance, this_list) {
         lSetUlong(qinstance, QU_tag, tag_value);
      }
   }
}

/****** sgeobj/qinstance/qinstance_increase_qversion() ************************
*  NAME
*     qinstance_increase_qversion() -- increase the qinstance queue version 
*
*  SYNOPSIS
*     void qinstance_increase_qversion(lListElem *this_elem) 
*
*  FUNCTION
*     Increase the queue version of the given qinstance "this_elem". 
*
*  INPUTS
*     lListElem *this_elem - QU_Type element 
*
*  RESULT
*     void - None
*
*  NOTES
*     MT-NOTE: qinstance_increase_qversion() is MT safe 
*******************************************************************************/
void
qinstance_increase_qversion(lListElem *this_elem)
{
   u_long32 current_version;

   DENTER(TOP_LAYER, "qinstance_increase_qversion");
   current_version = lGetUlong(this_elem, QU_version);
   lSetUlong(this_elem, QU_version, current_version + 1);
   DEXIT;
}

/****** sgeobj/qinstance/qinstance_check_owner() ******************************
*  NAME
*     qinstance_check_owner() -- check if a user is queue owner
*
*  SYNOPSIS
*     bool 
*     qinstance_check_owner(const lListElem *queue, const char *user_name) 
*
*  FUNCTION
*     Checks if the given user is an owner of the given queue.
*     Managers and operators are implicitly owner of all queues.
*
*  INPUTS
*     const lListElem *queue - the queue to check
*     const char *user_name  - the user name to check
*
*  RESULT
*     bool - true, if the user is owner, else false
******************************************************************************/
bool qinstance_check_owner(const lListElem *this_elem, const char *user_name)
{
   bool ret = false;
   lListElem *ep;

   DENTER(TOP_LAYER, "qinstance_check_owner");
   if (this_elem == NULL) {
      ret = false;
   } else if (user_name == NULL) {
      ret = false;
   } else if (manop_is_operator(user_name)) {
      ret = true;
   } else {
      for_each(ep, lGetList(this_elem, QU_owner_list)) {
         DPRINTF(("comparing user >>%s<< vs. owner_list entry >>%s<<\n",
                  user_name, lGetString(ep, US_name)));
         if (!strcmp(user_name, lGetString(ep, US_name))) {
            ret = true;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_pe_referenced() *************************
*  NAME
*     qinstance_is_pe_referenced() -- Is the PE object referenced 
*
*  SYNOPSIS
*     bool 
*     qinstance_is_pe_referenced(const lListElem *this_elem, 
*                                const lListElem *pe) 
*
*  FUNCTION
*     Is the given PE ("pe") referenced in the qinstance element "this_elem". 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element 
*     const lListElem *pe        - PE_Type element 
*
*  RESULT
*     bool - test result 
*        true  - is referenced 
*        false - is not referenced 
*
*  NOTES
*     MT-NOTE: qinstance_is_pe_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_pe_referenced(const lListElem *this_elem, const lListElem *pe)
{
   bool ret = false;
   lListElem *re_ref_elem;

   DENTER(TOP_LAYER, "qinstance_is_pe_referenced");
   for_each(re_ref_elem, lGetList(this_elem, QU_pe_list)) {
      if (pe_is_matching(pe, lGetString(re_ref_elem, ST_name))) {
         ret = true;
         break;
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_calendar_referenced() *******************
*  NAME
*     qinstance_is_calendar_referenced() -- is the calendar referenced 
*
*  SYNOPSIS
*     bool 
*     qinstance_is_calendar_referenced(const lListElem *this_elem, 
*                                      const lListElem *calendar) 
*
*  FUNCTION
*     Is the "calendar" referenced in the qinstance "this_elem". 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element 
*     const lListElem *calendar  - CAL_Type element 
*
*  RESULT
*     bool - test result 
*        true  - is referenced
*        false - is not referenced 
*
*  NOTES
*     MT-NOTE: qinstance_is_calendar_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_calendar_referenced(const lListElem *this_elem, 
                                 const lListElem *calendar)
{
   bool ret = false;
   const char *queue_calendar = NULL;

   DENTER(TOP_LAYER, "qinstance_is_calendar_referenced");
   queue_calendar = lGetString(this_elem, QU_calendar);
   if (queue_calendar != NULL) {
      const char *calendar_name = lGetString(calendar, CAL_name);

      if (!strcmp(queue_calendar, calendar_name)) {
         ret = true;
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_a_pe_referenced() ***********************
*  NAME
*     qinstance_is_a_pe_referenced() -- is a PE referenced
*
*  SYNOPSIS
*     bool qinstance_is_a_pe_referenced(const lListElem *this_elem) 
*
*  FUNCTION
*     Test is at least one PE is referenced by qinstance "this_elem" 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type 
*
*  RESULT
*     bool - test result
*        true  - an PE is referenced
*        false - no PE is referenced ("NONE")
*
*  NOTES
*     MT-NOTE: qinstance_is_a_pe_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_a_pe_referenced(const lListElem *this_elem)
{
   bool ret = false;

   DENTER(TOP_LAYER, "qinstance_is_a_pe_referenced");
   if (lGetNumberOfElem(lGetList(this_elem, QU_pe_list))) {
      ret = true;
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_ckpt_referenced() ***********************
*  NAME
*     qinstance_is_ckpt_referenced() -- Is the CKTP referenced 
*
*  SYNOPSIS
*     bool 
*     qinstance_is_ckpt_referenced(const lListElem *this_elem, 
*                                  const lListElem *ckpt) 
*
*  FUNCTION
*     Tests if the given CKPT object ("ckpt") is referenced in
*     the qinstance "this_elem". 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element
*     const lListElem *ckpt      - CKPT_Type element
*
*  RESULT
*     bool - test result
*        true  - CKPT is referenced
*        false - CKPT is not referenced
*
*  NOTES
*     MT-NOTE: qinstance_is_ckpt_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_ckpt_referenced(const lListElem *this_elem, const lListElem *ckpt)
{
   bool ret = false;
   lListElem *re_ref_elem;

   DENTER(TOP_LAYER, "qinstance_is_ckpt_referenced");
   for_each(re_ref_elem, lGetList(this_elem, QU_ckpt_list)) {
      if (!strcmp(lGetString(ckpt, CK_name), 
                  lGetString(re_ref_elem, ST_name))) {
         ret = true;
         break;
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_a_ckpt_referenced() *********************
*  NAME
*     qinstance_is_a_ckpt_referenced() -- Is an CKPT object referenced 
*
*  SYNOPSIS
*     bool qinstance_is_a_ckpt_referenced(const lListElem *this_elem) 
*
*  FUNCTION
*     Is an CKPT object referenced in "this_elem". 
*
*  INPUTS
*     const lListElem *this_elem - CKPT_Type element 
*
*  RESULT
*     bool - test result
*        true  - a CKPT is referenced
*        false - no CKPT is referenced
*
*  NOTES
*     MT-NOTE: qinstance_is_a_ckpt_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_a_ckpt_referenced(const lListElem *this_elem)
{
   bool ret = false;

   DENTER(TOP_LAYER, "qinstance_is_a_ckpt_referenced");
   if (lGetNumberOfElem(lGetList(this_elem, QU_ckpt_list))) {
      ret = true;
   }
   DEXIT;
   return ret;
} 

/****** sgeobj/qinstance/qinstance_is_centry_referenced() *********************
*  NAME
*     qinstance_is_centry_referenced() -- Is the given CENTRY object referenced 
*
*  SYNOPSIS
*     bool 
*     qinstance_is_centry_referenced(const lListElem *this_elem, 
*                                    const lListElem *centry) 
*
*  FUNCTION
*     Is the given CENTRY object ("centry") referenced by the qinstance
*     "this_elem". 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element 
*     const lListElem *centry    - CE_Type element
*
*  RESULT
*     bool - test result
*        true  - is referenced
*        fasle - is not referenced
*
*  NOTES
*     MT-NOTE: qinstance_is_centry_referenced() is MT safe 
*******************************************************************************/
bool
qinstance_is_centry_referenced(const lListElem *this_elem, 
                               const lListElem *centry)
{
   bool ret = false;

   DENTER(TOP_LAYER, "qinstance_is_centry_referenced");
   if (this_elem != NULL) {
      const char *name = lGetString(centry, CE_name);
      lList *centry_list = lGetList(this_elem, QU_consumable_config_list);
      lListElem *centry_ref = lGetElemStr(centry_list, CE_name, name);

      if (centry_ref != NULL) {
         ret = true;
      } else {
         int i;
      
         for (i = 0; i < max_queue_resources; i++) {
            if (strcmp(queue_resource[i].name, name) == 0) {
               ret = true;
               break;
            }
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_is_centry_a_complex_value() ****************
*  NAME
*     qinstance_is_centry_a_complex_value() -- Is it a complex_value 
*
*  SYNOPSIS
*     bool 
*     qinstance_is_centry_a_complex_value(const lListElem *this_elem, 
*                                         const lListElem *centry) 
*
*  FUNCTION
*     Is the given "centry" in the list of complex_values of "this_elem".
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element 
*     const lListElem *centry    - CE_Type element 
*
*  RESULT
*     bool - test result
*        true  - it is a complex value
*        false - no complex value
*  NOTES
*     MT-NOTE: qinstance_is_centry_a_complex_value() is MT safe 
*******************************************************************************/
bool
qinstance_is_centry_a_complex_value(const lListElem *this_elem,
                                    const lListElem *centry)
{
   bool ret = false;

   DENTER(TOP_LAYER, "qinstance_is_centry_a_complex_value");
   ret = qinstance_is_centry_referenced(this_elem, centry);
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_reinit_consumable_actual_list() ************
*  NAME
*     qinstance_reinit_consumable_actual_list() -- as it says 
*
*  SYNOPSIS
*     bool 
*     qinstance_reinit_consumable_actual_list(lListElem *this_elem, 
*                                             lList **answer_list) 
*
*  FUNCTION
*     Reinitialize the consumable actual values. 
*
*  INPUTS
*     lListElem *this_elem - QU_Type element 
*     lList **answer_list  - AN_Type element 
*
*  RESULT
*     bool - error result
*        true  - success
*        false - error
*
*  NOTES
*     MT-NOTE: qinstance_reinit_consumable_actual_list() is MT safe 
*******************************************************************************/
bool
qinstance_reinit_consumable_actual_list(lListElem *this_elem,
                                        lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_reinit_consumable_actual_list");

   if (this_elem != NULL) {
      lList *job_list = *(object_type_get_master_list(SGE_TYPE_JOB));
      lList *centry_list = *(object_type_get_master_list(SGE_TYPE_CENTRY));
      lListElem *job = NULL;

      lSetList(this_elem, QU_resource_utilization, NULL);
      qinstance_set_conf_slots_used(this_elem);
      qinstance_debit_consumable(this_elem, NULL, centry_list, 0);

      for_each(job, job_list) {
         lList *ja_task_list = lGetList(job, JB_ja_tasks);
         lListElem *ja_task = NULL;
         int slots = 0;

         for_each(ja_task, ja_task_list) {
            dstring buffer = DSTRING_INIT;
            const char *name = qinstance_get_name(this_elem, &buffer);

            lListElem *gdil_ep = lGetSubStr(ja_task, JG_qname, name,
                                            JAT_granted_destin_identifier_list);
            sge_dstring_free(&buffer);
            if (gdil_ep != NULL) {
               slots += lGetUlong(gdil_ep, JG_slots);
            }
         }
         if (slots > 0) {
            qinstance_debit_consumable(this_elem, job, centry_list, slots);
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_list_find_matching() ***********************
*  NAME
*     qinstance_list_find_matching() -- find certain qinstances 
*
*  SYNOPSIS
*     bool 
*     qinstance_list_find_matching(const lList *this_list, 
*                                  lList **answer_list, 
*                                  const char *hostname_pattern, 
*                                  lList **qref_list) 
*
*  FUNCTION
*     Finds all qinstances in "this_list" whose hostname part matches
*     the "hostname_pattern" (fnmatch pattern) and stores the
*     qinstance name in "qref_list". In case of any error "answer_list"
*     will be filled.
*
*  INPUTS
*     const lList *this_list       - QU_Type list 
*     lList **answer_list          - AN_Type list 
*     const char *hostname_pattern - fnmatch hostname pattern 
*     lList **qref_list            - QR_Type list
*
*  RESULT
*     bool - error result
*        true  - success
*        false - error
*
*  NOTES
*     MT-NOTE: qinstance_list_find_matching() is MT safe 
*******************************************************************************/
bool
qinstance_list_find_matching(const lList *this_list, lList **answer_list,
                             const char *hostname_pattern, lList **qref_list)
{
   bool ret = true;

   DENTER(QINSTANCE_LAYER, "qinstance_list_find_matching");
   if (this_list != NULL && hostname_pattern != NULL) {
      lListElem *qinstance;

      for_each(qinstance, this_list) {
         const char *hostname = lGetHost(qinstance, QU_qhostname);

         if (!fnmatch(hostname_pattern, hostname, 0)) {
            if (qref_list != NULL) {
               dstring buffer = DSTRING_INIT;
               const char *qi_name = qinstance_get_name(qinstance, &buffer);

               lAddElemStr(qref_list, QR_name, qi_name, QR_Type);
               sge_dstring_free(&buffer);
            }
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_slots_used() *******************************
*  NAME
*     qinstance_slots_used() -- Returns the number of currently used slots 
*
*  SYNOPSIS
*     int qinstance_slots_used(const lListElem *this_elem) 
*
*  FUNCTION
*     Returns the number of currently used slots. 
*
*  INPUTS
*     const lListElem *this_elem - QU_Type element 
*
*  RESULT
*     int - number of slots
*
*  NOTES
*     MT-NOTE: qinstance_slots_used() is MT safe 
*******************************************************************************/
int
qinstance_slots_used(const lListElem *this_elem) 
{
   int ret = 1000000;
   lListElem *slots;

   DENTER(QINSTANCE_LAYER, "qinstance_slots_used");
   slots = lGetSubStr(this_elem, RUE_name, "slots", QU_resource_utilization);
   if (slots != NULL) {
      ret = lGetDouble(slots, RUE_utilized_now);
   } else {
      /* may never happen */
      CRITICAL((SGE_EVENT, MSG_QINSTANCE_MISSLOTS_S, 
                lGetString(this_elem, QU_full_name)));
   }
   DEXIT;
   return ret;
}

/****** sgeobj/qinstance/qinstance_set_slots_used() ***************************
*  NAME
*     qinstance_set_slots_used() -- Modifies the number of used slots 
*
*  SYNOPSIS
*     void qinstance_set_slots_used(lListElem *this_elem, int new_slots) 
*
*  FUNCTION
*     Modifies the number of used slots 
*
*  INPUTS
*     lListElem *this_elem - QU_Type 
*     int new_slots        - new slot value 
*
*  RESULT
*     void - NONE 
*
*  NOTES
*     MT-NOTE: qinstance_set_slots_used() is MT safe 
*******************************************************************************/
void
qinstance_set_slots_used(lListElem *this_elem, int new_slots) 
{
   lListElem *slots;

   DENTER(QINSTANCE_LAYER, "qinstance_set_slots_used");
   slots = lGetSubStr(this_elem, RUE_name, "slots", QU_resource_utilization);
   if (slots != NULL) {
      lSetDouble(slots, RUE_utilized_now, new_slots);
   } else {
      /* may never happen */
      CRITICAL((SGE_EVENT, MSG_QINSTANCE_MISSLOTS_S, 
                lGetString(this_elem, QU_full_name)));
   }
   DEXIT;
}

/* slots2config_list(lListElem *qep) */
void 
qinstance_set_conf_slots_used(lListElem *this_elem)
{
   lListElem *slots;

   DENTER(QINSTANCE_LAYER, "qinstance_set_conf_slots_used");
   slots = lGetSubStr(this_elem, CE_name, "slots", 
                      QU_consumable_config_list);
   if (slots == NULL) {
      slots = lAddSubStr(this_elem, CE_name, "slots", 
                         QU_consumable_config_list, CE_Type);
   }
   if (slots != NULL) {
      dstring buffer = DSTRING_INIT;
      u_long32 slots_value = lGetUlong(this_elem, QU_job_slots);

      sge_dstring_sprintf(&buffer, u32, slots_value);
      lSetDouble(slots, CE_doubleval, slots_value);
      lSetString(slots, CE_stringval, sge_dstring_get_string(&buffer));
      sge_dstring_free(&buffer);
   }
   DEXIT;
}

void
qinstance_check_unknown_state(lListElem *this_elem)
{
   const char *hostname = NULL;
   const char *full_name = NULL;
   lList *load_list = NULL;
   lListElem *host = NULL;
   lListElem *load = NULL;

   DENTER(QINSTANCE_LAYER, "qinstance_check_unknown_state");
   hostname = lGetHost(this_elem, QU_qhostname);
   full_name = lGetString(this_elem, QU_full_name);
   host = host_list_locate(Master_Exechost_List, hostname);
   if (host != NULL) {
      load_list = lGetList(host, EH_load_list);

      for_each(load, load_list) {
         const char *load_name = lGetString(load, HL_name);

         if (!sge_is_static_load_value(load_name)) {
            qinstance_state_set_unknown(this_elem, false);
            DTRACE;
            break;
         }
      } 
   }
   DEXIT;
   return;
}

int 
qinstance_debit_consumable(lListElem *qep, lListElem *jep, lList *centry_list, 
                           int slots)
{
   return rc_debit_consumable(jep, qep, centry_list, slots,
                              QU_consumable_config_list, 
                              QU_resource_utilization,
                              lGetString(qep, QU_qname));
}

/****** lib/sgeobj/debit_consumable() ****************************************
*  NAME
*     rc_debit_consumable() -- Debit/Undebit consumables from resource container
*
*  SYNOPSIS
*     int 
*     rc_debit_consumable(lListElem *jep, lListElem *ep, lList *centry_list, 
*                         int slots, int config_nm, int actual_nm, 
*                         const char *obj_name)
*
*  FUNCTION
*     Updates all consumable actual values of a resource container
*     for 'slots' slots of the given job. Positive slots numbers 
*     cause debiting, negative ones cause undebiting.
*
*  INPUTS
*     lListElem *jep       - The job (JB_Type) defining which resources and how
*                            much of them need to be (un)debited
*                            
*     lListElem *ep        - The resource container (global/host/queue) 
*                            that owns the resources (EH_Type).
* 
*     lList *centry_list   - The global complex list that is needed to interpret
*                            the jobs' resource requests.
*
*     int slots            - The number of slots for which we are debiting.
*                            Positive slots numbers cause debiting, negative 
*                            ones cause undebiting.
*
*     int config_nm        - The CULL field of the 'ep' object that contains a
*                            CE_Type list of configured complex values.
* 
*     int actual_nm        - The CULL field of the 'ep' object that contains a
*                            CE_Type list of actual complex values.
*
*     const char *obj_name - The name of the object we are debiting from. This
*                            is only used for monitoring/diagnosis purposes.
*
*  RESULT
*     Returns -1 in case of an error. Otherwise the number of (un)debitations 
*     that actually took place is returned. If 0 is returned that means the
*     consumable resources of the 'ep' object has not changed.
********************************************************************************
*/
int 
rc_debit_consumable(lListElem *jep, lListElem *ep, lList *centry_list, 
                    int slots, int config_nm, int actual_nm, 
                    const char *obj_name) 
{
   lListElem *cr, *cr_config, *dcep;
   double dval;
   const char *name;
   int mods = 0;

   DENTER(TOP_LAYER, "rc_debit_consumable");

   if (!ep) {
      DEXIT;
      return 0;
   }

   for_each (cr_config, lGetList(ep, config_nm)) {
      name = lGetString(cr_config, CE_name);
      dval = 0;

      /* search default request */  
      if (!(dcep = centry_list_locate(centry_list, name))) {
         ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
         DEXIT; 
         return -1;
      } 

      if (!lGetBool(dcep, CE_consumable)) {
         /* no error */
         continue;
      }

      /* ensure attribute is in actual list */
      if (!(cr = lGetSubStr(ep, RUE_name, name, actual_nm))) {
         cr = lAddSubStr(ep, RUE_name, name, actual_nm, RUE_Type);
         /* RUE_utilized_now is implicitly set to zero */
      }
   
      if (jep) {
         bool tmp_ret = job_get_contribution(jep, NULL, name, &dval, dcep);

         if (tmp_ret && dval != 0.0) {
            DPRINTF(("debiting %f of %s on %s %s for %d slots\n", dval, name,
                     (config_nm==QU_consumable_config_list)?"queue":"host",
                     obj_name, slots));
            lAddDouble(cr, RUE_utilized_now, slots * dval);
            mods++;
         }  
      }
   }

   DEXIT;
   return mods;
}

/* EB: ADOC: add commets */

bool
qinstance_message_add(lListElem *this_elem, u_long32 type, const char *message)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_message_add");
   object_message_add(this_elem, QU_message_list, type, message);
   DEXIT;
   return ret;
}

bool
qinstance_message_trash_all_of_type_X(lListElem *this_elem, u_long32 type)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_message_trash_all_of_type_X");
   object_message_trash_all_of_type_X(this_elem, QU_message_list, type);
   DEXIT;
   return ret;
}

void
qinstance_set_full_name(lListElem *this_elem) 
{
   dstring buffer = DSTRING_INIT;
   const char *cqueue_name = lGetString(this_elem, QU_qname);
   const char *hostname = lGetHost(this_elem, QU_qhostname);

   sge_dstring_sprintf(&buffer, "%s@%s", cqueue_name, hostname);
   lSetString(this_elem, QU_full_name, 
              sge_dstring_get_string(&buffer));
   sge_dstring_free(&buffer);
}

bool
qinstance_validate(lListElem *this_elem, lList **answer_list)
{
   bool ret = true;
   lList *centry_master_list = *(centry_list_get_master_list());

   DENTER(TOP_LAYER, "qinstance_validate");

   /* QU_full_name isn't spooled, if it is not set, create it */
   if (lGetString(this_elem, QU_full_name) == NULL) {
      qinstance_set_full_name(this_elem);
   }
   
   /* handle slots from now on as a consumble attribute of queue */
   qinstance_set_conf_slots_used(this_elem); 

   /* remove all queue message, which are regenerated during the unspooling
      the queue */
   qinstance_message_trash_all_of_type_X(this_elem, ~QI_ERROR);   

   /* setup actual list of queue */
   qinstance_debit_consumable(this_elem, NULL, centry_master_list, 0);

   /* init double values of consumable configuration */
   centry_list_fill_request(lGetList(this_elem, QU_consumable_config_list), 
                     centry_master_list, true, false, true);

      if (ensure_attrib_available(NULL, this_elem, 
                                  QU_load_thresholds) ||
          ensure_attrib_available(NULL, this_elem, 
                                  QU_suspend_thresholds) ||
          ensure_attrib_available(NULL, this_elem, 
                                  QU_consumable_config_list)) {
         ret = false;
      }

   if (ret) {
      qinstance_state_set_unknown(this_elem, true);
      qinstance_state_set_cal_disabled(this_elem, false);
      qinstance_state_set_cal_suspended(this_elem, false);
      qinstance_set_slots_used(this_elem, 0);
      
      if (host_list_locate(Master_Exechost_List, 
                           lGetHost(this_elem, QU_qhostname)) == NULL) {
         answer_list_add_sprintf(answer_list, STATUS_EUNKNOWN, 
                                 ANSWER_QUALITY_ERROR, 
                                 MSG_QINSTANCE_HOSTFORQUEUEDOESNOTEXIST_SS,
                                 lGetString(this_elem, QU_qname), 
                                 lGetHost(this_elem, QU_qhostname));
         ret = false;
      }
   }
   
   DEXIT;
   return ret;
}

bool
qinstance_list_validate(lList *this_list, lList **answer_list)
{
   bool ret = true;
   lListElem *qinstance;

   DENTER(TOP_LAYER, "qinstance_list_validate");

   for_each(qinstance, this_list) {
      if (!qinstance_validate(qinstance, answer_list)) {
         ret = false;
         break;
      }
   }

   DEXIT;
   return ret;
}

