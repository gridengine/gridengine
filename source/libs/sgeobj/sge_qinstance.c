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

const char *queue_types[] = {
   "BATCH",
   "INTERACTIVE",
   ""
};

static bool
qinstance_has_type(const lListElem *this_elem, u_long32 type);

static bool qinstance_has_type(const lListElem *this_elem, u_long32 type)
{
   bool ret = false;

   if (lGetUlong(this_elem, QU_qtype) & type) {
      ret = true;
   }
   return ret;
}


const char *
qtype_append_to_dstring(u_long32 qtype, dstring *string)
{
   const char *ret = NULL;

   DENTER(BASIS_LAYER, "qtype_append_to_dstring");
   if (string != NULL) {
      const char **ptr = NULL;
      u_long32 bitmask = 1;
      bool qtype_defined = false;

      for (ptr = queue_types; **ptr != '\0'; ptr++) {
         if (bitmask & qtype) {
            if (qtype_defined) {
               sge_dstring_sprintf_append(string, " ");
            }
            sge_dstring_sprintf_append(string, "%s", *ptr);
            qtype_defined = true;
         }
         bitmask <<= 1;
      };
      if (!qtype_defined) {
         sge_dstring_sprintf_append(string, "NONE");
      }
      ret = sge_dstring_get_string(string);
   }
   DEXIT;
   return ret;
}

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

lListElem *
qinstance_list_locate2(const lList *queue_list, const char *full_name)
{
   return lGetElemStr(queue_list, QU_full_name, full_name);
}

void
qinstance_increase_qversion(lListElem *this_elem)
{
   u_long32 current_version;

   DENTER(TOP_LAYER, "qinstance_increase_qversion");
   current_version = lGetUlong(this_elem, QU_version);
   lSetUlong(this_elem, QU_version, current_version + 1);
   DEXIT;
}

/****** sgeobj/queue/qinstance_check_owner() **********************************
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
*
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

bool
qinstance_print_qtype_to_dstring(const lListElem *this_elem,
                                 dstring *string, bool only_first_char)
{
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_print_qtype_to_dstring");
   if (this_elem != NULL && string != NULL) {
      const char **ptr = NULL;
      u_long32 bitmask = 1;
      bool qtype_defined = false;

      for (ptr = queue_types; **ptr != '\0'; ptr++) {
         if (bitmask & lGetUlong(this_elem, QU_qtype)) {
            qtype_defined = true;
            if (only_first_char) {
               sge_dstring_sprintf_append(string, "%c", (*ptr)[0]);
            } else {
               sge_dstring_sprintf_append(string, "%s ", *ptr);
            }
         }
         bitmask <<= 1;
      };
      if (only_first_char) {
         if (qinstance_is_parallel_queue(this_elem)) {
            sge_dstring_sprintf_append(string, "%c", 'P');
            qtype_defined = true;
         }
         if (qinstance_is_checkointing_queue(this_elem)) {
            sge_dstring_sprintf_append(string, "%c", 'C');
            qtype_defined = true;
         }
      }
      if (!qtype_defined) {
         if (only_first_char) {
            sge_dstring_sprintf_append(string, "N");
         } else {
            sge_dstring_sprintf_append(string, "NONE");
         }
      }
   }
   DEXIT;
   return ret;
}

bool
qinstance_parse_qtype_from_string(lListElem *this_elem, lList **answer_list,
                                  const char *value)
{
   bool ret = true;
   u_long32 type = 0;

   DENTER(TOP_LAYER, "qinstance_parse_qtype_from_string");
   SGE_CHECK_POINTER_FALSE(this_elem);
   if (value != NULL && *value != 0) {
      if (!sge_parse_bitfield_str(value, queue_types, &type,
                                  "queue type", NULL, true)) {
         ret = false;
      }
   }

   lSetUlong(this_elem, QU_qtype, type);
   DEXIT;
   return ret;
}

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

bool qinstance_is_batch_queue(const lListElem *this_elem)
{
   return qinstance_has_type(this_elem, BQ);
}

bool qinstance_is_interactive_queue(const lListElem *this_elem)
{
   return qinstance_has_type(this_elem, IQ);
}

bool qinstance_is_checkointing_queue(const lListElem *this_elem)
{
   return qinstance_is_a_ckpt_referenced(this_elem);
}

bool qinstance_is_parallel_queue(const lListElem *this_elem)
{
   return qinstance_is_a_pe_referenced(this_elem);
}

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
      }
   }
   DEXIT;
   return ret;
}

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
            }
         }
      }
   }
   DEXIT;
   return ret;
}

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
qinstance_debit_consumable(lListElem *qep, lListElem *jep, lList *centry_list, int slots)
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
*     int rc_debit_consumable(lListElem *jep, lListElem *ep, lList *centry_list, 
*             int slots, int config_nm, int actual_nm, const char *obj_name)
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
rc_debit_consumable(lListElem *jep, lListElem *ep, lList *centry_list, int slots, 
                 int config_nm, int actual_nm, const char *obj_name) 
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
