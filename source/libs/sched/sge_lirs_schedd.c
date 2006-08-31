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

#include "sched/sge_lirs_schedd.h"
#include "sched/sge_resource_utilizationL.h"
#include "sched/sge_select_queue.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_strL.h"
#include "sgeobj/sge_jobL.h"
#include "sgeobj/sge_limit_rule.h"
#include "sgeobj/sge_object.h"
#include "uti/sge_hostname.h"
#include "sge_complex_schedd.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_pe.h"
#include "sgeobj/sge_host.h"
#include "sgermon.h"
#include "sched/sort_hosts.h"

/****** sge_lirs_schedd/debit_job_from_lirs() **********************************
*  NAME
*     debit_job_from_lirs() -- debits job in all relevant limitation rules
*
*  SYNOPSIS
*     int debit_job_from_lirs(lListElem *job, lList *granted, lListElem* pe, 
*     lList *centry_list) 
*
*  FUNCTION
*     The function debits in all relevant rule the requested amout of resources.
*
*  INPUTS
*     lListElem *job     - job request (JB_Type)
*     lList *granted     - granted list (JG_Type)
*     lListElem* pe      - granted pe (PE_Type)
*     lList *centry_list - consumable resouces list (CE_Type)
*
*  RESULT
*     int - always 0
*
*  NOTES
*     MT-NOTE: debit_job_from_lirs() is not MT safe 
*
*******************************************************************************/
int
debit_job_from_lirs(lListElem *job, lList *granted, lList *lirs_list, lListElem* pe, lList *centry_list, lList *acl_list, lList *hgrp_list) 
{
   lListElem *gel = NULL;

   DENTER(TOP_LAYER, "debit_job_from_lirs");
  
   /* debit for all hosts */
   for_each(gel, granted) {
      const char* pe_name = NULL;
      lListElem *lirs = NULL;
      int slots = lGetUlong(gel, JG_slots);

      if (pe != NULL) {
         pe_name =  lGetString(pe, PE_name);
      }

      for_each (lirs, lirs_list) {
         lirs_debit_consumable(lirs, job, gel, pe_name, centry_list, acl_list, hgrp_list, slots);
      }
   }

   DRETURN(0);
}

/****** sge_lirs_schedd/limit_rule_set_dynamical_limit() ***********************
*  NAME
*     limit_rule_set_dynamical_limit() -- evaluate dynamical limit
*
*  SYNOPSIS
*     bool limit_rule_set_dynamical_limit(lListElem *limit, lListElem 
*     *global_host, lListElem *exec_host, lList *centry) 
*
*  FUNCTION
*     The function evaluates if neccessary the dynamical limit for a host and
*     sets the evaluated double value in the given limitation element (LIRL_dvalue).
*
*     A evaluation is neccessary if the limit boolean LIRL_dynamic is true. This
*     field is set by qmaster during the rule set verification
*
*  INPUTS
*     lListElem *limit       - limitation (LIRL_Type)
*     lListElem *global_host - global host (EH_Type)
*     lListElem *exec_host   - exec host (EH_Type)
*     lList *centry          - consumable resource list (CE_Type)
*
*  RESULT
*     bool - always true
*
*  NOTES
*     MT-NOTE: limit_rule_set_dynamical_limit() is MT safe 
*
*******************************************************************************/
bool
limit_rule_set_dynamical_limit(lListElem *limit, lListElem *global_host, lListElem *exec_host, lList *centry) {

   DENTER(TOP_LAYER, "limit_rule_set_dynamical_limit");

   if (lGetBool(limit, LIRL_dynamic)) {
      double dynamic_limit = scaled_mixed_load(lGetString(limit, LIRL_value), global_host, exec_host, centry);
      DPRINTF(("found a dynamic limit for host %s with value %d\n", lGetHost(exec_host, EH_name), (int)dynamic_limit));
      lSetDouble(limit, LIRL_dvalue, dynamic_limit);
   } 

   DRETURN(true);
}
