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

#ifdef SOLARISAMD64
#  include <sys/stream.h>
#endif 

#include "sge.h"
#include "cull.h"
#include "sgermon.h"

#include "sge_pe.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_qinstance.h"
#include "sge_centry.h"
#include "sge_hostname.h"
#include "sge_schedd_conf.h"

#include "sge_qeti.h"
#include "sge_resource_utilization.h"

#include "sge_select_queue.h"

/* At that point in time we only keep references to in the iterator that
 * allow for efficiently iterating trough relevant queue end times in 
 * that affect resouce utilization of a particular job. 
 * 
 * Further improvements with sge_qeti_t might allow to notably reduce
 * the time for the actual resource selection: It might be useful for 
 * example to enhance sge_qeti_t towards keeping all information required 
 * for decide very quickly about eligibility of each host/queue.
 */
struct sge_qeti_s {
   lList *cr_refs_pe;
   lList *cr_refs_global;
   lList *cr_refs_host;
   lList *cr_refs_queue;
};

/* 
 *  Assume a parallel job with a license and h_vmem request. At the time
 *  when we seek for reservation the changes with the resource utilization 
 *  diagrams relevant for this job are marked with a '+'
 *
 *  After intializing the time iterator using sge_qeti_allocate() the 
 *  iterator keeps references to all resource instances (QETI_resource_instance) 
 *  shown in the diagram below and all queue end next (QETI_queue_end_next) 
 *  references refer to the very end of those resource diagrams.
 *
 *  mpi_pe  slots      +------+-----------+
 *  Global  license    +------+-----------+-----+
 *  Host1   h_vmem     +------+-----------+
 *  Host2   h_vmem     +------+-----------+
 *  Queue1  slots      +------+---+-------+-----+
 *  Queue2  slots   +--+------+---+-------+
 *                  6  5      4   3       2     1
 *
 *  After sge_qeti_first() returned time mark #1 the queue end next 
 *  references for Global license and the Queue1 slot point to #2.
 *  Then after sge_qeti_next() returned time mark #2 the queue end 
 *  next references for Queue1/2 slot point to #3 whereas all remaining
 *  queue end next references point to #4 ...
 *
 */



/****** sge_qeti/sge_qeti_list_add() *******************************************
*  NAME
*     sge_qeti_list_add() -- Adds a resource utilization to QETI resource list
*
*  SYNOPSIS
*     static int sge_qeti_list_add(lList **lpp, const char *name, lList* 
*     rue_lp, double total, bool must_exist) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **lpp      - QETI resource list
*     const char *name - Name of the resource
*     lList* rue_lp    - Resource utilization entry (RUE_Type)
*     double total     - Total resource amount
*     bool must_exist  - If true the entry must exist in 'lpp'.
*
*  RESULT
*     static int -  0 on success
*
*  NOTES
*     MT-NOTE: sge_qeti_list_add() is not MT safe 
*******************************************************************************/
static int sge_qeti_list_add(lList **lpp, const char *name, lList* rue_lp, double total, bool must_exist)
{
   lListElem *tmp_cr_ref, *ep;

   DENTER(TOP_LAYER, "sge_qeti_list_add");

   if (!(tmp_cr_ref = lGetElemStr(rue_lp, RUE_name, name))) {
      DEXIT;
      return must_exist?-1:0;
   }

   if (!*lpp && !(*lpp = lCreateList("pe_qeti", QETI_Type))) {
      DEXIT;
      return -1;
   }

   if (!(ep = lCreateElem(QETI_Type))) {
      *lpp = lFreeList(*lpp);
      DEXIT;
      return -1;
   }

   lSetRef(ep, QETI_resource_instance, tmp_cr_ref);
   lSetDouble(ep, QETI_total, total);
   lAppendElem(*lpp, ep);

   DEXIT;
   return 0;
}

static int sge_add_qeti_resource_container(lList **qeti_to_add, lList* rue_list, 
      lList* total_list, lList* centry_list, lList* requests, bool force_slots) 
{
   lListElem *req, *actual, *tep;
   const char *name;
   lListElem *centry_config;

   DENTER(TOP_LAYER, "sge_add_qeti_resource_container");

   /* implicit slot request */
   if ( ((tep = lGetElemStr(total_list, CE_name, SGE_ATTR_SLOTS)) != NULL) && force_slots) {
      DEXIT;
      return -1;
   }

   if (tep && sge_qeti_list_add(qeti_to_add, SGE_ATTR_SLOTS, rue_list, 
                  lGetDouble(tep, CE_doubleval), true)) {
      DEXIT;
      return -1;
   }

   /* default request */
   for_each (actual, rue_list) {
      name = lGetString(actual, RUE_name);
      if (strcmp(name, SGE_ATTR_SLOTS) != 0) {
         continue;
      }   

      centry_config = lGetElemStr(centry_list, CE_name, name);

      if (lGetBool(centry_config, CE_consumable)==true && !is_requested(requests, name)) {
         if (!(tep = lGetElemStr(total_list, CE_name, name)) ||
            sge_qeti_list_add(qeti_to_add, name, rue_list, lGetDouble(tep, CE_doubleval), true)) {
            DEXIT;
            return -1;
         }
      } 
   }

   /* explicit requests */
   for_each (req, requests) {
      name = lGetString(req, CE_name);
      centry_config = lGetElemStr(centry_list, CE_name, name);

      if ((centry_config && lGetBool(centry_config, CE_consumable)==true) && 
            (tep = lGetElemStr(total_list, CE_name, name))) {
         if (sge_qeti_list_add(qeti_to_add, name, rue_list, lGetDouble(tep, CE_doubleval), false)) {
            DEXIT;
            return -1;
         }
      }
   }

   DEXIT;
   return 0;
}

sge_qeti_t *sge_qeti_allocate2(lListElem *cr)
{
   sge_qeti_t *iter;
   lList *rue_list;

   if (!(iter = calloc(1, sizeof(sge_qeti_t)))) {
      return NULL;
   }

   rue_list = lCreateList("", lGetElemDescr(cr));
   lAppendElem(rue_list, cr);
   sge_qeti_list_add(&iter->cr_refs_pe, "slots", rue_list, 10, true);
   return iter;
}

sge_qeti_t *sge_qeti_allocate(lListElem *job, lListElem *pe, lListElem *ckpt, 
      lList *host_list, lList *queue_list, lList *centry_list, lList *acl_list)
{
   sge_qeti_t *iter;
   lListElem *next_queue, *qep, *hep;
   lList *requests = lGetList(job, JB_hard_resource_list);

   DENTER(TOP_LAYER, "sge_qeti_allocate");

   if (!(iter = calloc(1, sizeof(sge_qeti_t)))) {
      DEXIT;
      return NULL;
   }
 
   /* add "slot" resource utilization entry of parallel environment */
   if (sge_qeti_list_add(&iter->cr_refs_pe, SGE_ATTR_SLOTS, 
                  lGetList(pe, PE_resource_utilization), lGetUlong(pe, PE_slots), true)) {
      sge_qeti_release(iter);
      DEXIT;
      return NULL;
   }

   /* add references to global resource utilization entries 
      that might affect jobs queue end time */
   if ((hep = host_list_locate(host_list, SGE_GLOBAL_NAME))) {
      if (sge_add_qeti_resource_container(&iter->cr_refs_global, 
               lGetList(hep, EH_resource_utilization), lGetList(hep, EH_consumable_config_list), 
               centry_list, requests, false)!=0) {
         sge_qeti_release(iter);
         DEXIT;
         return NULL;
      }
   }

   /* add references to per host resource utilization entries 
      that might affect jobs queue end time */
   for_each (hep, host_list) {
      const char *eh_name;
      int is_relevant;
      const void *queue_iterator = NULL;

      if (sge_host_match_static(job, NULL, hep, centry_list, acl_list) == DISPATCH_NEVER_CAT) {
         continue;
      }   
      if (!strcmp((eh_name=lGetHost(hep, EH_name)), SGE_GLOBAL_NAME)) {
         continue;
      }   

      /* There must be at least one queue referenced with the parallel 
         environment that resides at this host. And secondly we only 
         consider those hosts that match this job (statically) */
      is_relevant = false;
      for (next_queue = lGetElemHostFirst(queue_list, QU_qhostname, eh_name, &queue_iterator); 
          (qep = next_queue);
           next_queue = lGetElemHostNext(queue_list, QU_qhostname, eh_name, &queue_iterator)) {

         if (qinstance_is_pe_referenced(qep, pe)) {
            is_relevant = true;
            break;
         }
      }
      if (!is_relevant) {
         continue;
      }   

      if (sge_add_qeti_resource_container(&iter->cr_refs_host, 
               lGetList(hep, EH_resource_utilization), lGetList(hep, EH_consumable_config_list), 
                     centry_list, requests, false)!=0) {
         sge_qeti_release(iter);
         DEXIT;
         return NULL;
      }
   }

   /* add references to per queue resource utilization entries 
      that might affect jobs queue end time */
   for_each (qep, queue_list) {
      if (!strcmp(lGetString(qep, QU_qname), SGE_TEMPLATE_NAME))
         continue;

      /* consider only those queues that match this job (statically) */
      if (sge_queue_match_static(qep, job, pe, ckpt, centry_list, acl_list) != DISPATCH_OK) { 
         continue;
      }   

      if (sge_add_qeti_resource_container(&iter->cr_refs_queue, 
               lGetList(qep, QU_resource_utilization), lGetList(qep, QU_consumable_config_list), 
                     centry_list, requests, false)!=0) {
         sge_qeti_release(iter);
         DEXIT;
         return NULL;
      }
   }

   DPRINTF(("QETI: P %d G %d H %d Q %d\n", 
      lGetNumberOfElem(iter->cr_refs_pe),
      lGetNumberOfElem(iter->cr_refs_global),
      lGetNumberOfElem(iter->cr_refs_host),
      lGetNumberOfElem(iter->cr_refs_queue)));

   DEXIT;
   return iter;
}


static void sge_qeti_init_refs(lList *cref_lp)
{
   lListElem *cr_ep;
   lList *utilization_diagram;
   lListElem *rue_ep;

   DENTER(TOP_LAYER, "sge_qeti_init_refs");

   for_each (cr_ep, cref_lp) {
      rue_ep = lGetRef(cr_ep, QETI_resource_instance);
      utilization_diagram = lGetList((lListElem *)lGetRef(cr_ep, QETI_resource_instance), RUE_utilized);
      DPRINTF(("   QETI INIT: %s %p\n", lGetString(rue_ep, RUE_name), utilization_diagram));
      /* lLast() correctly returns a NULL reference 
         in case of an empty resource utilization diagram */
      lSetRef(cr_ep, QETI_queue_end_next, lLast(utilization_diagram));
   }

   DEXIT;
}

/* an empty resource utilization diagrams actually means the resource
   is available now - thus we can skip it when determining the maximum */
static void sge_qeti_max_end_time(u_long32 *max_time, const lList *cref_lp)
{
   lListElem *cr_ep, *ref;
   u_long32 tmp_time = *max_time;
   lListElem *rue_ep;

   DENTER(TOP_LAYER, "sge_qeti_max_end_time");

   for_each (cr_ep, cref_lp) {
      rue_ep = lGetRef(cr_ep, QETI_resource_instance);
      if (!(ref = lGetRef(cr_ep, QETI_queue_end_next))) {
         DPRINTF(("   QETI END: %s\n", lGetString(rue_ep, RUE_name)));
         continue;
      }
      DPRINTF(("   QETI END: %s "U32CFormat" ("U32CFormat")\n", 
            lGetString(rue_ep, RUE_name), lGetUlong(ref, RDE_time), tmp_time));
      tmp_time = MAX(tmp_time, lGetUlong(ref, RDE_time));
   }
   *max_time = tmp_time;

   DEXIT;
   return;
}

/* switch queue end next references to the next entry 
   whose time is larger or equal the specified time */
static void sge_qeti_switch_to_next(u_long32 time, lList *cref_lp)
{
   lListElem *cr_ep, *ref;
   lListElem *rue_ep;
   
   DENTER(TOP_LAYER, "sge_qeti_switch_to_next");
   
   time--;

   for_each (cr_ep, cref_lp) {
      rue_ep = lGetRef(cr_ep, QETI_resource_instance);
      if (!(ref = lGetRef(cr_ep, QETI_queue_end_next))) {
         DPRINTF(("   QETI NEXT: %s (finished)\n", lGetString(rue_ep, RUE_name)));
         continue;
      }

      while (ref && time < lGetUlong(ref, RDE_time)) {
         ref = lPrev(ref);
      }

      DPRINTF(("   QETI NEXT: %s set to "U32CFormat" (%p)\n", 
            lGetString(rue_ep, RUE_name), ref?lGetUlong(ref, RDE_time):0, ref));
      lSetRef(cr_ep, QETI_queue_end_next, ref);
   }

   DEXIT;
}

/****** sge_qeti/sge_qeti_next_before() ****************************************
*  NAME
*     sge_qeti_next_before() -- ??? 
*
*  SYNOPSIS
*     void sge_qeti_next_before(sge_qeti_t *qeti, u_long32 start) 
*
*  FUNCTION
*     All queue end next references are set in a way that will 
*     sge_qeti_next() return a time value that is before (i.e. less than)
*     start.
*
*  INPUTS
*     sge_qeti_t *qeti - ??? 
*     u_long32 start   - ??? 
*
*  NOTES
*     MT-NOTE: sge_qeti_next_before() is MT safe 
*******************************************************************************/
void sge_qeti_next_before(sge_qeti_t *qeti, u_long32 start)
{
   sge_qeti_switch_to_next(start, qeti->cr_refs_pe);
   sge_qeti_switch_to_next(start, qeti->cr_refs_global);
   sge_qeti_switch_to_next(start, qeti->cr_refs_host);
   sge_qeti_switch_to_next(start, qeti->cr_refs_queue);
}


/****** sge_resource_utilization/sge_qeti_first() ******************************
*  NAME
*     sge_qeti_first() -- 
*
*  SYNOPSIS
*     u_long32 sge_qeti_first(sge_qeti_t *qeti) 
*
*  FUNCTION
*     Initialize/Reinitialize Queue End Time Iterator. All queue end next 
*     references are initialized to the queue end of all resourece instances.
*     Before we return the time that is most in the future queue end next
*     references are switched to the next entry that is earlier than the time
*     that was returned.
*
*  INPUTS
*     sge_qeti_t *qeti - ??? 
*
*  RESULT
*     u_long32 - 
*
*  NOTES
*     MT-NOTE: sge_qeti_first() is MT safe 
*******************************************************************************/
u_long32 sge_qeti_first(sge_qeti_t *qeti)
{
   u_long32 all_resources_queue_end_time = 0;

   DENTER(TOP_LAYER, "sge_qeti_first");

   /* (re)init all queue end next references */
   sge_qeti_init_refs(qeti->cr_refs_pe);
   sge_qeti_init_refs(qeti->cr_refs_global);
   sge_qeti_init_refs(qeti->cr_refs_host);
   sge_qeti_init_refs(qeti->cr_refs_queue);

   /* determine all resources queue end time */
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_pe);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_global);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_host);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_queue);

   DPRINTF(("sge_qeti_first() determines "u32"\n", all_resources_queue_end_time));

   /* switch to the next entry with all queue end next references whose 
      time is larger (?) or equal to all resources queue end time */
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_pe);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_global);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_host);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_queue);
     
   DEXIT;
   return all_resources_queue_end_time;
}

/****** sge_resource_utilization/sge_qeti_next() *******************************
*  NAME
*     sge_qeti_next() -- ??? 
*
*  SYNOPSIS
*     u_long32 sge_qeti_next(sge_qeti_t *qeti) 
*
*  FUNCTION
*     Return next the time that is most in the future. Then queue end next
*     references are switched to the next entry that is earlier than the time
*     that was returned.
*
*  INPUTS
*     sge_qeti_t *qeti - ??? 
*
*  RESULT
*     u_long32 - 
*
*  NOTES
*     MT-NOTE: sge_qeti_next() is MT safe 
*******************************************************************************/
u_long32 sge_qeti_next(sge_qeti_t *qeti)
{
   u_long32 all_resources_queue_end_time = DISPATCH_TIME_NOW;

   DENTER(TOP_LAYER, "sge_qeti_next");

   /* determine all resources queue end time */
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_pe);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_global);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_host);
   sge_qeti_max_end_time(&all_resources_queue_end_time, qeti->cr_refs_queue);

   DPRINTF(("sge_qeti_next() determines "u32"\n", all_resources_queue_end_time));

   /* switch to the next entry with all queue end next references whose 
      time is larger (?) or equal to all resources queue end time */
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_pe);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_global);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_host);
   sge_qeti_switch_to_next(all_resources_queue_end_time, qeti->cr_refs_queue);

   DEXIT;
   return all_resources_queue_end_time;
}

/****** sge_resource_utilization/sge_qeti_release() ****************************
*  NAME
*     sge_qeti_release() -- Release queue end time iterator
*
*  SYNOPSIS
*     void sge_qeti_release(sge_qeti_t *qeti) 
*
*  FUNCTION
*     Release all resources of the queue end time iterator. Refered 
*     resource utilization diagrams are not affected.
*
*  INPUTS
*     sge_qeti_t *qeti - ??? 
*
*  NOTES
*     MT-NOTE: sge_qeti_release() is MT safe 
*******************************************************************************/
void sge_qeti_release(sge_qeti_t *qeti)
{
   if (!qeti)
      return;

   lFreeList(qeti->cr_refs_pe);
   lFreeList(qeti->cr_refs_global);
   lFreeList(qeti->cr_refs_host);
   lFreeList(qeti->cr_refs_queue);
   free(qeti);
}
