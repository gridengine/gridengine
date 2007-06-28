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

#include "sched/sge_select_queue.h"
#include "sched/sge_resource_quota_schedd.h"

#include "sched/sge_resource_utilizationL.h"
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
#include "sge_log.h"


static void rqs_can_optimize(const lListElem *rule, bool *host, bool *queue, sge_assignment_t *a);

static void rqs_expand_cqueues(const lListElem *rule, lList **skip_cqueue_list, sge_assignment_t *a);
static void rqs_expand_hosts(const lListElem *rule, lList **skip_host_list, sge_assignment_t *a);

static bool is_cqueue_global(const lListElem *rule);
static bool is_host_global(const lListElem *rule);

static bool is_cqueue_expand(const lListElem *rule);
static bool is_host_expand(const lListElem *rule);

static bool cqueue_shadowed(const lListElem *rule, sge_assignment_t *a);
static bool host_shadowed(const lListElem *rule, sge_assignment_t *a);

static void rqs_excluded_hosts(const lListElem *rule, lList **skip_host_list, sge_assignment_t *a);
static void rqs_excluded_cqueues(const lListElem *rule, lList **skip_cqueue_list, sge_assignment_t *a);



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

static bool rqs_match_assignment(const lListElem *rule, sge_assignment_t *a)
{
   return (rqs_filter_match(lGetObject(rule, RQR_filter_projects), FILTER_PROJECTS, a->project, NULL, NULL, NULL) &&
          rqs_filter_match(lGetObject(rule, RQR_filter_users), FILTER_USERS, a->user, a->acl_list, NULL, a->group) &&
	   rqs_filter_match(lGetObject(rule, RQR_filter_pes), FILTER_PES, NULL, NULL, NULL, NULL))?true:false;
}


/****** sge_resource_quota_schedd/cqueue_shadowed() ****************************
*  NAME
*     cqueue_shadowed() -- Check for cluster queue rule before current rule
*
*  SYNOPSIS
*     static bool cqueue_shadowed(const lListElem *rule, sge_assignment_t *a)
*
*  FUNCTION
*     Check whether there is any cluster queue specific rule before the
*     current rule.
*
*  INPUTS
*     const lListElem *rule - Current rule
*     sge_assignment_t *a   - Scheduler assignment
*
*  RESULT
*     static bool - True if shadowed
*
*  EXAMPLE
*     limit queue Q001 to F001=1
*     limit host gridware to F001=0  (--> returns 'true' due to 'Q001' meaning
*                               that gridware can't be generelly ruled out )
*
*  NOTES
*     MT-NOTE: cqueue_shadowed() is MT safe
*******************************************************************************/
static bool cqueue_shadowed(const lListElem *rule, sge_assignment_t *a)
{
   while ((rule = lPrev(rule))) {
      if (rqs_match_assignment(rule, a) && !is_cqueue_global(rule)) {
         return true;
      }
   }
   return false;
}

/****** sge_resource_quota_schedd/host_shadowed() ******************************
*  NAME
*     host_shadowed() -- Check for host rule before current rule
*
*  SYNOPSIS
*     static bool host_shadowed(const lListElem *rule, sge_assignment_t *a)
*
*  FUNCTION
*     Check whether there is any host specific rule before the
*     current rule.
*
*  INPUTS
*     const lListElem *rule - Current rule
*     sge_assignment_t *a   - Scheduler assignment
*
*  RESULT
*     static bool - True if shadowed
*
*  EXAMPLE
*     limit host gridware to F001=1
*     limit queue Q001 to F001=0  (--> returns 'true' due to 'gridware' meaning
*                               that Q001 can't be generelly ruled out )
*
*  NOTES
*     MT-NOTE: host_shadowed() is MT safe
*******************************************************************************/
static bool host_shadowed(const lListElem *rule, sge_assignment_t *a)
{
   while ((rule = lPrev(rule))) {
      if (rqs_match_assignment(rule, a) && !is_host_global(rule)) {
         return true;
      }
   }
   return false;
}

/****** sge_resource_quota_schedd/cqueue_shadowed_by() *************************
*  NAME
*     cqueue_shadowed_by() -- Check rules shadowing current cluster queue rule
*
*  SYNOPSIS
*     static bool cqueue_shadowed_by(const char *cqname, const lListElem *rule,
*     sge_assignment_t *a)
*
*  FUNCTION
*     Check if cluster queue in current rule is shadowed.
*
*  INPUTS
*     const char *cqname    - Cluster queue name to check
*     const lListElem *rule - Current rule
*     sge_assignment_t *a   - Assignment
*
*  RESULT
*     static bool - True if shadowed
*
*  EXAMPLE
*     limits queues Q001,Q002 to F001=1
*     limits queues Q002,Q003 to F001=1 (--> returns 'true' for Q002 and 'false' for Q003)
*
*  NOTES
*     MT-NOTE: cqueue_shadowed_by() is MT safe
*******************************************************************************/
static bool cqueue_shadowed_by(const char *cqname, const lListElem *rule, sge_assignment_t *a)
{
   while ((rule = lPrev(rule))) {
      if (rqs_match_assignment(rule, a) &&
          rqs_filter_match(lGetObject(rule, RQR_filter_queues), FILTER_QUEUES, cqname, NULL, NULL, NULL)) {
         return true;
      }
   }

   return false;
}

/****** sge_resource_quota_schedd/host_shadowed_by() ***************************
*  NAME
*     host_shadowed_by() -- ???
*
*  SYNOPSIS
*     static bool host_shadowed_by(const char *host, const lListElem *rule,
*     sge_assignment_t *a)
*
*  FUNCTION
*     Check if host in current rule is shadowed.
*
*  INPUTS
*     const char *cqname    - Host name to check
*     const lListElem *rule - Current rule
*     sge_assignment_t *a   - Assignment
*
*  RESULT
*     static bool - True if shadowed
*
*  EXAMPLE
*     limits hosts host1,host2 to F001=1
*     limits hosts host2,host3 to F001=1 (--> returns 'true' for host2 and 'false' for host3)
*
*  NOTES
*     MT-NOTE: host_shadowed_by() is MT safe
*******************************************************************************/
static bool host_shadowed_by(const char *host, const lListElem *rule, sge_assignment_t *a)
{
   while ((rule = lPrev(rule))) {
      if (rqs_match_assignment(rule, a) &&
          rqs_filter_match(lGetObject(rule, RQR_filter_hosts), FILTER_HOSTS, host, NULL, a->hgrp_list, NULL)) {
         return true;
      }
   }

   return false;
}

/****** sge_resource_quota_schedd/rqs_can_optimize() ***************************
*  NAME
*     rqs_can_optimize() -- Poke whether a queue/host negation can be made
*
*  SYNOPSIS
*     static void rqs_can_optimize(const lListElem *rule, bool *host, bool
*     *queue, sge_assignment_t *a)
*
*  FUNCTION
*     A global limit was hit with 'rule'. This function helps to determine
*     to what exend we can profit from that situation. If there is no
*     previous matching rule within the same rule set any other queue/host
*     can be skipped.
*
*  INPUTS
*     const lListElem *rule - Rule
*     bool *host            - Any previous rule with a host scope?
*     bool *queue           - Any previous rule with a queue scope?
*     sge_assignment_t *a   - Scheduler assignment
*
*  NOTES
*     MT-NOTE: rqs_can_optimize() is MT safe
*******************************************************************************/
static void rqs_can_optimize(const lListElem *rule, bool *host, bool *queue, sge_assignment_t *a)
{
   bool host_shadowed = false, queue_shadowed = false;

   const lListElem *prev = rule;
   while ((prev = lPrev(prev))) {
      if (!rqs_match_assignment(rule, a))
         continue;
      if (!is_host_global(prev))
         host_shadowed = true;
      if (!is_cqueue_global(prev))
         queue_shadowed = true;
   }

   *host = host_shadowed;
   *queue = queue_shadowed;

   return;
}

/****** sge_resource_quota_schedd/rqs_excluded_cqueues() ***********************
*  NAME
*     rqs_excluded_cqueues() -- Find excluded queues
*
*  SYNOPSIS
*     static void rqs_excluded_cqueues(const lListElem *rule, lList
*     **skip_cqueue_list, sge_assignment_t *a)
*
*  FUNCTION
*     Find queues that are excluded by previous rules.
*
*  INPUTS
*     const lListElem *rule    - The rule
*     lList **skip_cqueue_list - List of already excluded queues
*     sge_assignment_t *a      - Scheduler assignement
*
*  EXAMPLE
*      limit        projects {*} queues !Q001 to F001=1
*      limit        to F001=0   ( ---> returns Q001 in skip_cqueue_list)
*
*  NOTES
*     MT-NOTE: rqs_excluded_cqueues() is MT safe
*******************************************************************************/
static void rqs_excluded_cqueues(const lListElem *rule, lList **skip_cqueue_list, sge_assignment_t *a)
{
   lListElem *cq;
   const lListElem *prev;
   int ignored = 0, excluded = 0;

   DENTER(TOP_LAYER, "rqs_excluded_cqueues");

   for_each (cq, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      const char *cqname = lGetString(cq, CQ_name);
      bool exclude = true;

      if (lGetElemStr(*skip_cqueue_list, CTI_name, cqname)) {
         ignored++;
         continue;
      }

      prev = rule;
      while ((prev = lPrev(prev))) {
         if (!rqs_match_assignment(rule, a))
            continue;

         if (rqs_filter_match(lGetObject(prev, RQR_filter_queues), FILTER_QUEUES, cqname, NULL, NULL, NULL)) {
            exclude = false;
            break;
         }
      }
      if (exclude) {
         lAddElemStr(skip_cqueue_list, CTI_name, cqname, CTI_Type);
         excluded++;
      }
   }

   if (ignored + excluded == 0) {
      CRITICAL((SGE_EVENT, "not a single queue excluded in rqs_excluded_cqueues()\n"));
   }

   DRETURN_VOID;
}

/****** sge_resource_quota_schedd/rqs_excluded_hosts() *************************
*  NAME
*     rqs_excluded_hosts() -- Find excluded hosts
*
*  SYNOPSIS
*     static void rqs_excluded_hosts(const lListElem *rule, lList
*     **skip_host_list, sge_assignment_t *a)
*
*  FUNCTION
*     Find hosts that are excluded by previous rules.
*
*  INPUTS
*     const lListElem *rule    - The rule
*     lList **skip_host_list   - List of already excluded hosts
*     sge_assignment_t *a      - Scheduler assignement
*
*  EXAMPLE
*      limit        projects {*} queues !gridware to F001=1
*      limit        to F001=0   ( ---> returns gridware in skip_host_list)
*
*  NOTES
*     MT-NOTE: rqs_excluded_hosts() is MT safe
*******************************************************************************/
static void rqs_excluded_hosts(const lListElem *rule, lList **skip_host_list, sge_assignment_t *a)
{
   lListElem *eh;
   const lListElem *prev;
   int ignored = 0, excluded = 0;

   DENTER(TOP_LAYER, "rqs_excluded_hosts");

   for_each (eh, a->host_list) {
      const char *hname = lGetHost(eh, EH_name);
      bool exclude = true;

      if (lGetElemStr(*skip_host_list, CTI_name, hname)) {
         ignored++;
         continue;
      }

      prev = rule;
      while ((prev = lPrev(prev))) {
         if (!rqs_match_assignment(rule, a))
            continue;

         if (rqs_filter_match(lGetObject(prev, RQR_filter_hosts), FILTER_HOSTS, hname, NULL, a->hgrp_list, NULL)) {
            exclude = false;
            break;
         }
      }
      if (exclude) {
         lAddElemStr(skip_host_list, CTI_name, hname, CTI_Type);
         excluded++;
      }
   }

   if (ignored + excluded == 0) {
      CRITICAL((SGE_EVENT, "not a single host excluded in rqs_excluded_hosts()\n"));
   }

   DRETURN_VOID;
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
static void rqs_expand_cqueues(const lListElem *rule, lList **skip_cqueue_list, sge_assignment_t *a)
{
   const lListElem *cq;
   const char *cqname;
   lListElem *qfilter = lGetObject(rule, RQR_filter_queues);

   DENTER(TOP_LAYER, "rqs_expand_cqueues");

   for_each (cq, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
      cqname = lGetString(cq, CQ_name);
      if (lGetElemStr(*skip_cqueue_list, CTI_name, cqname))
         continue;
      if (rqs_filter_match(qfilter, FILTER_QUEUES, cqname, NULL, NULL, NULL) && !cqueue_shadowed_by(cqname, rule, a))
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
static void rqs_expand_hosts(const lListElem *rule, lList **skip_host_list, sge_assignment_t *a)
{
   const lListElem *eh;
   const char *hname;
   lListElem *hfilter = lGetObject(rule, RQR_filter_hosts);

   for_each (eh, a->host_list) {
      hname = lGetHost(eh, EH_name);
      if (lGetElemStr(*skip_host_list, CTI_name, hname))
         continue;
      if (rqs_filter_match(hfilter, FILTER_HOSTS, hname, NULL, a->hgrp_list, NULL) && !host_shadowed_by(hname, rule, a))
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

/****** sge_resource_quota_schedd/is_cqueue_global() ***************************
*  NAME
*     is_cqueue_global() -- Global rule with regards to cluster queues?
*
*  SYNOPSIS
*     bool is_cqueue_global(const lListElem *rule)
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
static bool is_cqueue_global(const lListElem *rule)
{
   return is_global(rule, RQR_filter_queues);
}


/****** sge_resource_quota_schedd/is_host_global() *****************************
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
static bool is_host_global(const lListElem *rule)
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


/****** sge_resource_quota_schedd/is_host_expand() *****************************
*  NAME
*     is_host_expand() -- Returns true if rule expands on hosts
*
*  SYNOPSIS
*     bool is_host_expand(const lListElem *rule)
*
*  FUNCTION
*     Returns true if rule expands on hosts.
*
*  INPUTS
*     const lListElem *rule - RQR_Type
*
*  RESULT
*     bool - True if rule expands on hosts
*
*  EXAMPLE
*      "hosts {*}" returns true
*      "hosts @allhosts" returns false
*
*  NOTES
*     MT-NOTE: is_host_expand() is MT safe
*******************************************************************************/
static bool is_host_expand(const lListElem *rule)
{
   return is_expand(rule, RQR_filter_hosts);
}

/****** sge_resource_quota_schedd/is_cqueue_expand() ***************************
*  NAME
*     is_cqueue_expand() -- Returns true if rule expands on cluster queues
*
*  SYNOPSIS
*     bool is_cqueue_expand(const lListElem *rule)
*
*  FUNCTION
*     Returns true if rule expands on cluster queues.
*
*  INPUTS
*     const lListElem *rule - RQR_Type
*
*  RESULT
*     bool - True if rule expands on hosts
*
*  EXAMPLE
*      "queues {*}" returns true
*      "queues Q001,Q002" returns false
*
*  NOTES
*     MT-NOTE: is_cqueue_expand() is MT safe
*******************************************************************************/
static bool is_cqueue_expand(const lListElem *rule)
{
   return is_expand(rule, RQR_filter_queues);
}

/****** sge_resource_quota_schedd/rqs_exceeded_sort_out() **********************
*  NAME
*     rqs_exceeded_sort_out() -- Rule out queues/hosts whenever possible
*
*  SYNOPSIS
*     bool rqs_exceeded_sort_out(sge_assignment_t *a, const lListElem *rule,
*     const dstring *rule_name, const char* queue_name, const char* host_name,
*     lList **skip_cqueue_list, lList **skip_host_list)
*
*  FUNCTION
*     This function tries to rule out hosts and cluster queues after a
*     quota exeeding was found for a limitation rule with specific queue
*     instance.
*
*     When a limitation was exeeded that applies to the entire
*     cluster 'true' is returned, 'false' otherwise.
*
*  INPUTS
*     sge_assignment_t *a      - Scheduler assignment type
*     const lListElem *rule    - The exeeded rule
*     const dstring *rule_name - Name of the rule (monitoring only)
*     const char* queue_name   - Cluster queue name
*     const char* host_name    - Host name
*     lList **skip_cqueue_list - List of ruled out cluster queues
*     lList **skip_host_list   - List of ruled out hosts
*
*  RESULT
*     bool - True upon global limits exceeding
*
*  NOTES
*     MT-NOTE: rqs_exceeded_sort_out() is MT safe
*******************************************************************************/
bool rqs_exceeded_sort_out(sge_assignment_t *a, const lListElem *rule, const dstring *rule_name,
   const char* queue_name, const char* host_name, lList **skip_cqueue_list, lList **skip_host_list)
{
   bool cq_global = is_cqueue_global(rule);
   bool eh_global = is_host_global(rule);

   DENTER(TOP_LAYER, "rqs_exceeded_sort_out");

   if ((!cq_global && !eh_global) || (cq_global && eh_global &&
         (is_cqueue_expand(rule) || is_host_expand(rule)))) { /* failure at queue instance limit */
      DPRINTF(("QUEUE INSTANCE: resource quota set %s deny job execution on %s@%s\n", 
            sge_dstring_get_string(rule_name), queue_name, host_name));
      DRETURN(false);
   }

   if (cq_global && eh_global) { /* failure at a global limit */
      bool host_shadowed, queue_shadowed;

      rqs_can_optimize(rule, &host_shadowed, &queue_shadowed, a);
      if (!host_shadowed && !queue_shadowed) {
         DPRINTF(("GLOBAL: resource quota set %s deny job execution globally\n", 
               sge_dstring_get_string(rule_name)));
         DRETURN(true);
      }

      if (host_shadowed && queue_shadowed) {
         DPRINTF(("QUEUE INSTANCE: resource quota set %s deny job execution on %s@%s\n", 
               sge_dstring_get_string(rule_name), queue_name, host_name));
         DRETURN(false);
      }

      if (queue_shadowed) {
         rqs_excluded_cqueues(rule, skip_cqueue_list, a);
         DPRINTF(("QUEUE: resource quota set %s deny job execution in all its queues\n", 
               sge_dstring_get_string(rule_name)));
      } else { /* must be host_shadowed */
         rqs_excluded_hosts(rule, skip_host_list, a);
         DPRINTF(("HOST: resource quota set %s deny job execution in all its queues\n", 
               sge_dstring_get_string(rule_name)));
      }

      DRETURN(false);
   }

   if (!cq_global) { /* failure at a cluster queue limit */

      if (host_shadowed(rule, a)) {
         DPRINTF(("QUEUE INSTANCE: resource quota set %s deny job execution on %s@%s\n", 
               sge_dstring_get_string(rule_name), queue_name, host_name));
         DRETURN(false);
      }

      if (lGetBool(lGetObject(rule, RQR_filter_queues), RQRF_expand) == true) {
         lAddElemStr(skip_cqueue_list, CTI_name, queue_name, CTI_Type);
         DPRINTF(("QUEUE: resource quota set %s deny job execution in queue %s\n", 
               sge_dstring_get_string(rule_name), queue_name));
      } else {
         rqs_expand_cqueues(rule, skip_cqueue_list, a);
         DPRINTF(("QUEUE: resource quota set %s deny job execution in all its queues\n", 
               sge_dstring_get_string(rule_name)));
      }

      DRETURN(false);
   }

   /* must be (!eh_global) */
   { /* failure at a host limit */

      if (cqueue_shadowed(rule, a)) {
         DPRINTF(("QUEUE INSTANCE: resource quota set %s deny job execution on %s@%s\n", 
               sge_dstring_get_string(rule_name), queue_name, host_name));
         DRETURN(false);
      }

      if (lGetBool(lGetObject(rule, RQR_filter_hosts), RQRF_expand) == true) {
         lAddElemStr(skip_host_list, CTI_name, host_name, CTI_Type);
         DPRINTF(("HOST: resource quota set %s deny job execution at host %s\n",    
               sge_dstring_get_string(rule_name), host_name));
      } else {
         rqs_expand_hosts(rule, skip_host_list, a);
         DPRINTF(("HOST: resource quota set %s deny job execution at all its hosts\n", 
               sge_dstring_get_string(rule_name)));
      }

      DRETURN(false);
   }
}
/****** sge_resource_quota/sge_user_is_referenced_in_rqs() ************************
*  NAME
*     sge_user_is_referenced_in_rqs() -- search for user reference in rqs 
*
*  SYNOPSIS
*     bool sge_user_is_referenced_in_rqs(const lList *rqs, const char *user, 
*     lList *acl_list) 
*
*  FUNCTION
*     Search for a user reference in the resource quota sets
*
*  INPUTS
*     const lList *rqs - resource quota set list
*     const char *user  - user to search
*     const char *group - user's group
*     lList *acl_list   - acl list for user resolving
*
*  RESULT
*     bool - true if user was found
*            false if user was not found
*
*  NOTES
*     MT-NOTE: sge_user_is_referenced_in_rqs() is MT safe 
*
*******************************************************************************/
bool sge_user_is_referenced_in_rqs(const lList *rqs, const char *user, const char *group, lList *acl_list)
{
   bool ret = false;
   lListElem *ep;

   for_each(ep, rqs) {
      lList *rule_list = lGetList(ep, RQS_rule);
      lListElem *rule;
      for_each(rule, rule_list) {
         /* there may be no per-user limitation and also not limitation that is special for this user */
         if ((is_expand(rule, RQR_filter_users) || !is_global(rule, RQR_filter_users)) &&
             rqs_filter_match(lGetObject(rule, RQR_filter_users), FILTER_USERS, user,
             acl_list, NULL, group)) {
            ret = true;
            break;
         }
      }
      if (ret == true) {
         break;
      }
   }
   return ret;
}
