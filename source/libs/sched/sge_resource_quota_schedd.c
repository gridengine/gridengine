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

#include "sched/sge_resource_quota_schedd.h"

#include "sched/sge_resource_utilizationL.h"
#include "sched/sge_select_queue.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_strL.h"
#include "sgeobj/sge_jobL.h"
#include "sgeobj/sge_ctL.h"
#include "sgeobj/sge_cqueueL.h"

#include "sgeobj/sge_resource_quota.h"
#include "sgeobj/sge_object.h"
#include "uti/sge_hostname.h"
#include "sge_complex_schedd.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_pe.h"
#include "sgeobj/sge_host.h"
#include "sgermon.h"
#include "sched/sort_hosts.h"

/****** sge_resource_quota_schedd/debit_job_from_rqs() **********************************
*  NAME
*     debit_job_from_rqs() -- debits job in all relevant resource quotas
*
*  SYNOPSIS
*     int debit_job_from_rqs(lListElem *job, lList *granted, lListElem* pe, 
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
*     MT-NOTE: debit_job_from_rqs() is not MT safe 
*
*******************************************************************************/
int
debit_job_from_rqs(lListElem *job, lList *granted, lList *rqs_list, lListElem* pe, lList *centry_list, lList *acl_list, lList *hgrp_list) 
{
   lListElem *gel = NULL;

   DENTER(TOP_LAYER, "debit_job_from_rqs");
  
   /* debit for all hosts */
   for_each(gel, granted) {
      const char* pe_name = NULL;
      lListElem *rqs = NULL;
      int slots = lGetUlong(gel, JG_slots);

      if (pe != NULL) {
         pe_name =  lGetString(pe, PE_name);
      }

      for_each (rqs, rqs_list) {
         rqs_debit_consumable(rqs, job, gel, pe_name, centry_list, acl_list, hgrp_list, slots);
      }
   }

   DRETURN(0);
}

/****** sge_resource_quota_schedd/rqs_set_dynamical_limit() ***********************
*  NAME
*     rqs_set_dynamical_limit() -- evaluate dynamical limit
*
*  SYNOPSIS
*     bool rqs_set_dynamical_limit(lListElem *limit, lListElem 
*     *global_host, lListElem *exec_host, lList *centry) 
*
*  FUNCTION
*     The function evaluates if neccessary the dynamical limit for a host and
*     sets the evaluated double value in the given limitation element (RQRL_dvalue).
*
*     A evaluation is neccessary if the limit boolean RQRL_dynamic is true. This
*     field is set by qmaster during the rule set verification
*
*  INPUTS
*     lListElem *limit       - limitation (RQRL_Type)
*     lListElem *global_host - global host (EH_Type)
*     lListElem *exec_host   - exec host (EH_Type)
*     lList *centry          - consumable resource list (CE_Type)
*
*  RESULT
*     bool - always true
*
*  NOTES
*     MT-NOTE: rqs_set_dynamical_limit() is MT safe 
*
*******************************************************************************/
bool
rqs_set_dynamical_limit(lListElem *limit, lListElem *global_host, lListElem *exec_host, lList *centry) {

   DENTER(TOP_LAYER, "rqs_set_dynamical_limit");

   if (lGetBool(limit, RQRL_dynamic)) {
      double dynamic_limit = scaled_mixed_load(lGetString(limit, RQRL_value), global_host, exec_host, centry);
      DPRINTF(("found a dynamic limit for host %s with value %d\n", lGetHost(exec_host, EH_name), (int)dynamic_limit));
      lSetDouble(limit, RQRL_dvalue, dynamic_limit);
   } 

   DRETURN(true);
}

/****** sge_resource_quota_schedd/rqs_expand_cqueues() *************************
*  NAME
*     rqs_expand_cqueues() -- Add all matching cqueues to the list
*
*  SYNOPSIS
*     void rqs_expand_cqueues(const lListElem *rule, lList **skip_cqueue_list) 
*
*  FUNCTION
*     The names of all cluster queues that match the rule are added to 
*     the skip list without duplicates.
*
*  INPUTS
*     const lListElem *rule    - RQR_Type
*     lList **skip_cqueue_list - CTI_Type
*
*  NOTES
*     MT-NOTE: rqs_expand_cqueues() is not MT safe 
*******************************************************************************/
void rqs_expand_cqueues(const lListElem *rule, lList **skip_cqueue_list)
{
   const lListElem *cq;
   const char *cqname;
   lListElem *qfilter = lGetObject(rule, RQR_filter_queues);

   DENTER(TOP_LAYER, "rqs_expand_cqueues");

   for_each (cq, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      cqname = lGetString(cq, CQ_name);
      if (lGetElemStr(*skip_cqueue_list, CTI_name, cqname))
         continue;
      if (rqs_filter_match(qfilter, FILTER_QUEUES, cqname, NULL, NULL, NULL))
         lAddElemStr(skip_cqueue_list, CTI_name, cqname, CTI_Type); 
   }

   DEXIT;
   return; 
}

/****** sge_resource_quota_schedd/rqs_expand_hosts() ***************************
*  NAME
*     rqs_expand_hosts() -- Add all matching cqueues to the list
*
*  SYNOPSIS
*     void rqs_expand_hosts(const lListElem *rule, lList **skip_host_list, 
*     const lList *host_list, lList *hgrp_list) 
*
*  FUNCTION
*     The names of all hosts that match the rule are added to 
*     the skip list without duplicates.
*
*  INPUTS
*     const lListElem *rule  - RQR_Type
*     lList **skip_host_list - CTI_Type
*     const lList *host_list - EH_Type
*     lList *hgrp_list       - HGRP_Type
*
*  NOTES
*     MT-NOTE: rqs_expand_hosts() is MT safe 
*******************************************************************************/
void rqs_expand_hosts(const lListElem *rule, lList **skip_host_list, const lList *host_list, lList *hgrp_list)
{
   const lListElem *eh;
   const char *hname;
   lListElem *hfilter = lGetObject(rule, RQR_filter_hosts);

   for_each (eh, host_list) {
      hname = lGetHost(eh, EH_name);
      if (lGetElemStr(*skip_host_list, CTI_name, hname))
         continue;
      if (rqs_filter_match(hfilter, FILTER_HOSTS, hname, NULL, hgrp_list, NULL))
         lAddElemStr(skip_host_list, CTI_name, hname, CTI_Type); 
   }

   return;
}

static bool is_global(const lListElem *rule, int nm)
{
   lListElem *filter = lGetObject(rule, nm);
   if (!filter)
      return true;
   if (lGetSubStr(filter, ST_name, "*", RQRF_scope) && lGetNumberOfElem(lGetList(filter, RQRF_xscope))==0)
      return true;
   return false;
}

/****** sge_resource_quota/is_cqueue_global() **********************************
*  NAME
*     is_cqueue_global() -- Global rule with regards to cluster queues?
*
*  SYNOPSIS
*     bool is_cqueue_global(const lListElem *rule) 
*
*  FUNCTION
*     Return true if cluster queues play no role with the rule
*
*  INPUTS
*     const lListElem *rule - RQR_Type
*
*  RESULT
*     bool - True if cluster queues play no role with the rule
*
*  NOTES
*     MT-NOTE: is_cqueue_global() is MT safe 
*******************************************************************************/
bool is_cqueue_global(const lListElem *rule)
{
   return is_global(rule, RQR_filter_queues);
}

/****** sge_resource_quota/is_host_global() ************************************
*  NAME
*     is_host_global() -- Global rule with regards to hosts?
*
*  SYNOPSIS
*     bool is_host_global(const lListElem *rule) 
*
*  FUNCTION
*     Return true if hosts play no role with the rule
*
*  INPUTS
*     const lListElem *rule - RQR_Type
*
*  RESULT
*     bool - True if hosts play no role with the rule
*
*  NOTES
*     MT-NOTE: is_host_global() is MT safe 
*******************************************************************************/
bool is_host_global(const lListElem *rule)
{
   return is_global(rule, RQR_filter_hosts);
}

static bool is_expand(const lListElem *rule, int nm)
{
   lListElem *filter = lGetObject(rule, nm);
   if (filter && lGetBool(filter, RQRF_expand) == true)
      return true;
   else
      return false;
}

bool is_host_expand(const lListElem *rule)
{
   return is_expand(rule, RQR_filter_hosts);
}
bool is_cqueue_expand(const lListElem *rule)
{
   return is_expand(rule, RQR_filter_queues);
}

