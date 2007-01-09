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
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#ifdef SGE_PQS_API
#include <dlfcn.h>
#endif

#include "msg_common.h"
#include "basis_types.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_select_queue.h"
#include "sge_select_queueL.h"
#include "sge_parse_num_par.h"
#include "sge_complex_schedd.h"
#include "valid_queue_user.h"
#include "subordinate_schedd.h"
#include "sge_range_schedd.h"
#include "sge_pe_schedd.h"
#include "sge_range.h"
#include "sge_qeti.h"

#include "sge_orderL.h"
#include "sge_pe.h"
#include "sge_ctL.h"
#include "sge_qinstanceL.h"
#include "sge_strL.h"
#ifdef SGE_PQS_API
#include "sge_varL.h"
#endif
#include "sort_hosts.h"
#include "schedd_monitor.h"
#include "schedd_message.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "sge_ja_task.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_host.h"
#include "sge_job.h"
#include "sge_cqueue.h"
#include "sge_qinstance.h"
#include "sge_qinstance_type.h"
#include "sge_userprj.h"
#include "sge_ckpt.h"
#include "sge_centry.h"
#include "sge_object.h"
#include "sge_resource_utilization.h"
#include "sge_qinstance_state.h"
#include "sge_schedd_conf.h"
#include "sge_subordinate.h"
#include "sge_qref.h"
#ifdef SGE_PQS_API
#include "sge_pqs_api.h"
#endif
#include "sge_calendarL.h"
#include "sge_attrL.h"
#include "sched/sge_resource_quota_schedd.h"
#include "sgeobj/sge_userset.h"

/* -- these implement helpers for the category optimization -------- */

typedef struct {
   lListElem *category;          /* ref to the category */
   lListElem *cache;             /* ref to the cache object in th category */
   bool       use_category;      /* if true: use the category */
   bool       use_cviolation;    /* if true: use the cached queue soft request violations */
   bool       compute_violation; /* if true: compute soft request violations */
   bool       mod_category;      /* if true: update the category with new messages, queues, and hosts */
   u_long32  *posible_pe_slots;  /* stores all posible slots settings for a pe job with ranges */  
   bool      is_pe_slots_rev;    /* if it is true, the posible_pe_slots are stored in the category */
} category_use_t;

static void 
fill_category_use_t(const sge_assignment_t *best, category_use_t *use_category, const char *pe_name);

static bool
add_pe_slots_to_category(category_use_t *use_category, u_long32 *max_slotsp, lListElem *pe, 
                         int min_slots, int max_slots, lList *pe_range);
/* -- these implement parallel assignemnt ------------------------- */

static dispatch_t
parallel_reservation_max_time_slots(sge_assignment_t *best);

static dispatch_t
parallel_maximize_slots_pe(sge_assignment_t *best, int *available_slots);

static dispatch_t 
parallel_assignment(sge_assignment_t *a, category_use_t *use_category, int *available_slots);

static dispatch_t 
parallel_available_slots(const sge_assignment_t *a, int *slots, int *slots_qend); 

static dispatch_t
parallel_tag_queues_suitable4job(sge_assignment_t *assignment, category_use_t *use_category, int *available_slots);

static int 
parallel_sort_suitable_queues(lList *queue_list);

static dispatch_t 
parallel_global_slots(const sge_assignment_t *a, int *slots, int *slots_qend, 
                     int *violations);

static dispatch_t
parallel_tag_hosts_queues(sge_assignment_t *a, lListElem *hep, int *slots, 
                   int *slots_qend, int host_soft_violations, bool *master_host, int *host_seqno, 
                   double *previous_load, bool *previous_load_inited, category_use_t *use_category,
                   lList **skip_cqueue_list, lList **unclear_cqueue_list);

static int 
parallel_max_host_slots(sge_assignment_t *a, lListElem *host);

static dispatch_t
parallel_host_slots(sge_assignment_t *a, int *slots, int *slots_qend, int *host_soft_violations,
                   lListElem *hep, bool allow_non_requestable);

static dispatch_t 
parallel_limit_slots_by_time(const sge_assignment_t *a, lList *requests, 
                 int *slots, int *slots_qend, lListElem *centry, lListElem *lirl, dstring rue_name);

static dispatch_t
parallel_rqs_slots_by_time(const sge_assignment_t *a, int *slots, int *slots_qend, const char *host, const char *queue);

static dispatch_t
parallel_queue_slots(sge_assignment_t *a,lListElem *qep, int *slots, int *slots_qend, 
                    int *violations, bool allow_non_requestable);

static int 
parallel_make_granted_destination_id_list(sge_assignment_t *assignment);

static 
void clean_up_parallel_job(sge_assignment_t *a); 

/* -- these implement sequential assignemnt ---------------------- */

static dispatch_t
sequential_tag_queues_suitable4job(sge_assignment_t *a);

static dispatch_t
sequential_tag_queues_suitable4job_by_rqs(sge_assignment_t *a);

static dispatch_t
sequential_queue_time( u_long32 *start, const sge_assignment_t *a, int *violations, lListElem *qep); 

static dispatch_t 
rqs_limitation_reached(sge_assignment_t *a, const lListElem *rule, const char *host, const char *queue, u_long32 *start);

static dispatch_t
sequential_host_time(u_long32 *start, const sge_assignment_t *a, int *violations, lListElem *hep); 

static dispatch_t
sequential_global_time(u_long32 *start_time, const sge_assignment_t *a, int *violations); 

static int 
sequential_update_host_order(lList *host_list, lList *queues);

/* -- base functions ---------------------------------------------- */

static void 
sort_queue_list(lList *queue_list); 

static int 
compute_soft_violations(const sge_assignment_t *a, lListElem *queue, int violation, lList *load_attr, lList *config_attr,
                    lList *actual_attr, u_long32 layer, double lc_factor, u_long32 tag);
 
static dispatch_t 
rc_time_by_slots(const sge_assignment_t *a, lList *requested, lList *load_attr, lList *config_attr, lList *actual_attr, 
                 lListElem *queue, bool allow_non_requestable, dstring *reason, int slots,
                 u_long32 layer, double lc_factor, u_long32 tag, u_long32 *start_time, const char *object_name);

static dispatch_t
parallel_rc_slots_by_time(const sge_assignment_t *a, lList *requests, 
                 int *slots, int *slots_qend, lList *total_list, lList *rue_list, lList *load_attr, 
                 bool force_slots, lListElem *queue, u_long32 layer, double lc_factor, u_long32 tag,
                 bool allow_non_requestable, const char *object_name);

static dispatch_t
ri_time_by_slots(const sge_assignment_t *a, lListElem *request, lList *load_attr, lList *config_attr, lList *actual_attr, 
                lListElem *queue, dstring *reason, bool allow_non_requestable, 
                int slots, u_long32 layer, double lc_factor, u_long32 *start_time, const char *object_name); 

static dispatch_t
ri_slots_by_time(const sge_assignment_t *a, int *slots, int *slots_qend, 
                lList *rue_list, lListElem *request, lList *load_attr, lList *total_list, lListElem *queue, 
                u_long32 layer, double lc_factor, dstring *reason, bool allow_non_requestable, 
                bool no_centry, const char *object_name);

static dispatch_t
match_static_resource(int slots, lListElem *req_cplx, lListElem *src_cplx, dstring *reason, 
             int is_threshold, int force_existence, bool allow_non_requestable);

static int 
resource_cmp(u_long32 relop, double req, double src_dl); 

static bool 
job_is_forced_centry_missing(const lListElem *job, const lList *master_centry_list, const lListElem *queue_or_host);

static void 
clear_resource_tags( lList *resources, u_long32 max_tag); 

static dispatch_t 
find_best_result(dispatch_t r1, dispatch_t r2);

/* ---- helpers for load computation ---------------------------------------------------------- */

static lListElem
*load_locate_elem(lList *load_list, lListElem *global_consumable, lListElem *host_consumable, 
                  lListElem *queue_consumable, const char *limit); 

#ifdef SGE_PQS_API
static int
sge_call_pe_qsort(sge_assignment_t *a, const char *qsort_args);
#endif

static int 
load_check_alarm(char *reason, const char *name, const char *load_value, const char *limit_value, 
                     u_long32 relop, u_long32 type, lListElem *hep, lListElem *hlep, double lc_host, 
                     double lc_global, const lList *load_adjustments, int load_is_value); 

static int 
load_np_value_adjustment(const char* name, lListElem *hep, double *load_correction);

/* ---- Implementation ------------------------------------------------------------------------- */

void assignment_init(sge_assignment_t *a, lListElem *job, lListElem *ja_task, bool is_load_adj)
{
   a->job         = job;
   a->ja_task     = ja_task;
   if (is_load_adj) {
      a->load_adjustments = sconf_get_job_load_adjustments();
   }
   
   if (job != NULL) {
      a->job_id      = lGetUlong(job, JB_job_number);
   }
   
   if (ja_task != NULL) {
      a->ja_task_id  = lGetUlong(ja_task, JAT_task_number);
   }   
}

void assignment_copy(sge_assignment_t *dst, sge_assignment_t *src, bool move_gdil)
{
   if (dst == NULL || src == NULL) {
      return;
   }

   if (dst->load_adjustments != NULL) {
      lFreeList(&dst->load_adjustments);
   }

   if (move_gdil) {
      lFreeList(&(dst->gdil));
   }

   memcpy(dst, src, sizeof(sge_assignment_t));
 
   if (src->load_adjustments != NULL) {
      dst->load_adjustments = lCopyList("cpy_load_adj", src->load_adjustments);
   }
 
   if (!move_gdil) {
      dst->gdil = NULL; 
   }   
   else {
      src->gdil = NULL; 
   }
}

void assignment_release(sge_assignment_t *a)
{
   lFreeList(&(a->load_adjustments));
   lFreeList(&(a->gdil));
}

static dispatch_t 
find_best_result(dispatch_t r1, dispatch_t r2)
{
   DENTER(BASIS_LAYER, "find_best_result");

   if (r1 == DISPATCH_NEVER || 
            r2 == DISPATCH_NEVER) {
      DRETURN(DISPATCH_NEVER);
   }
   else if (r1 == DISPATCH_OK || 
       r2 == DISPATCH_OK) {
      DRETURN(DISPATCH_OK);
   }   
   else if (r1 == DISPATCH_NOT_AT_TIME || 
            r2 == DISPATCH_NOT_AT_TIME) {
      DRETURN(DISPATCH_NOT_AT_TIME);
   }   
   else if (r1 == DISPATCH_NEVER_JOB || 
            r2 == DISPATCH_NEVER_JOB) {
      DRETURN(DISPATCH_NEVER_JOB);
   }
   else if (r1 ==  DISPATCH_NEVER_CAT ||
            r2 == DISPATCH_NEVER_CAT) {
      DRETURN(DISPATCH_NEVER_CAT);
   }
   else if (r1 == DISPATCH_MISSING_ATTR  ||
            r2 == DISPATCH_MISSING_ATTR ) {
      DRETURN(DISPATCH_MISSING_ATTR);
   }

   CRITICAL((SGE_EVENT, MSG_JOBMATCHINGUNEXPECTEDRESULT));
   DRETURN(DISPATCH_NEVER);
}



/****** scheduler/sge_select_parallel_environment() ****************************
*  NAME
*     sge_select_parallel_environment() -- Decide about a PE assignment 
*
*  SYNOPSIS
*     static dispatch_t sge_select_parallel_environment(sge_assignment_t *best, lList 
*     *pe_list) 
*
*  FUNCTION
*     When users use wildcard PE request such as -pe <pe_range> 'mpi8_*' 
*     more than a single parallel environment can match the wildcard expression. 
*     In case of 'now' assignments the PE that gives us the largest assignment 
*     is selected. When scheduling a reservation we search for the earliest 
*     assignment for each PE and then choose that one that finally gets us the 
*     maximum number of slots.
*
* IMPORTANT
*     The scheduler info messages are not cached. They are added globaly and have
*     to be added for each job in the category. When the messages are updated
*     this has to be changed.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*     lList *pe_list         - the list of all parallel environments (PE_Type)
*
*  RESULT
*     dispatch_t - 0 ok got an assignment
*                  1 no assignment at the specified time (???)
*                 -1 assignment will never be possible for all jobs of that category
*                 -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_select_parallel_environment() is not MT safe 
*******************************************************************************/
dispatch_t
sge_select_parallel_environment( sge_assignment_t *best, lList *pe_list) 
{
   int matched_pe_count = 0;
   lListElem *pe;
   const char *pe_request, *pe_name;
   dispatch_t result, best_result = DISPATCH_NEVER_CAT;
   int old_logging = 0;

   DENTER(TOP_LAYER, "sge_select_parallel_environment");

   pe_request = lGetString(best->job, JB_pe);

   DPRINTF(("handling parallel job "sge_u32"."sge_u32"\n", best->job_id, best->ja_task_id)); 

   if (best->is_reservation) { /* reservation scheduling */

      old_logging = schedd_mes_get_logging();
      schedd_mes_set_logging(0);
      
      for_each(pe, pe_list) {

         if (!pe_is_matching(pe, pe_request)) {
            continue;
         }   

         matched_pe_count++;

         if (best->gdil == NULL) { /* first pe run */
            best->pe = pe;

            /* determine earliest start time with that PE */
            result = parallel_reservation_max_time_slots(best);
            if (result != DISPATCH_OK) {
               DPRINTF(("### first ### reservation in PE \"%s\" at "sge_u32" with %d soft violations\n",
                     lGetString(best->pe, PE_name), best->start, best->soft_violations));
               best_result = find_best_result(best_result, result);
               continue;
            }
         } 
         else { /* test with all other pes */
            sge_assignment_t tmp = SGE_ASSIGNMENT_INIT;

            assignment_copy(&tmp, best, false);
            tmp.pe = pe;

            /* try to find earlier assignment again with minimum slot amount */
            tmp.slots = 0;
            result = parallel_reservation_max_time_slots(&tmp); 
            
            if (result != DISPATCH_OK) {
               best_result = find_best_result(best_result, result);
               continue;
            }

            if (tmp.start < best->start || 
                  (tmp.start == best->start && tmp.soft_violations < best->soft_violations)) {
               assignment_copy(best, &tmp, true);
               DPRINTF(("### better ### reservation in PE \"%s\" at "sge_u32" with %d soft violations\n",
                     lGetString(best->pe, PE_name), best->start, best->soft_violations));
            }

            assignment_release(&tmp);
         }
      }
   } 
   else {
      /* now assignments */ 
      for_each(pe, pe_list) {

         if (!pe_is_matching(pe, pe_request)) {
            continue;
         }   

         pe_name = lGetString(pe, PE_name);
         matched_pe_count++;

         if (best->gdil == NULL) {
            int available_slots = 0;
            best->pe = pe;
            result = parallel_maximize_slots_pe(best, &available_slots);

            if (result != DISPATCH_OK) {
               schedd_mes_add(best->job_id, SCHEDD_INFO_PESLOTSNOTINRANGE_SI, pe_name, available_slots); 
               best_result = find_best_result(best_result, result);
               continue;
            }
            DPRINTF(("### first ### assignment in PE \"%s\" with %d soft violations\n",
                  lGetString(best->pe, PE_name), best->soft_violations));
         } 
         else {
            int available_slots = 0;
            sge_assignment_t tmp = SGE_ASSIGNMENT_INIT;

            assignment_copy(&tmp, best, false);
            tmp.pe = pe;

            result = parallel_maximize_slots_pe(&tmp, &available_slots);
                        
            if (result != DISPATCH_OK) {
               schedd_mes_add(best->job_id, SCHEDD_INFO_PESLOTSNOTINRANGE_SI, pe_name, available_slots); 
               best_result = find_best_result(best_result, result);
               continue;
            }

            if ((tmp.slots > best->slots) || 
                (tmp.start == best->start && 
                 tmp.soft_violations < best->soft_violations)) {
               assignment_copy(best, &tmp, true);
               DPRINTF(("### better ### assignment in PE \"%s\" with %d soft violations\n",
                        lGetString(best->pe, PE_name), best->soft_violations));
            }

            assignment_release(&tmp);
         }
      }
   }

   if (matched_pe_count == 0) {
      schedd_mes_add(best->job_id, SCHEDD_INFO_NOPEMATCH_ ); 
      best_result = DISPATCH_NEVER_CAT;
   } 
   else if (best->is_reservation && best->gdil) {
      int available_slots = 0;
      result = parallel_maximize_slots_pe(best, &available_slots);
      if (result != DISPATCH_OK) { /* ... should never happen */
         best_result = DISPATCH_NEVER_CAT;
      }
   }

   if (best->gdil) {
      best_result = DISPATCH_OK;
   }   

   switch (best_result) {
   case DISPATCH_OK:
      DPRINTF(("SELECT PE("sge_u32"."sge_u32") returns PE %s %d slots at "sge_u32")\n", 
            best->job_id, best->ja_task_id, lGetString(best->pe, PE_name), 
            best->slots, best->start));
      break;
   case DISPATCH_NOT_AT_TIME:
      DPRINTF(("SELECT PE("sge_u32"."sge_u32") returns <later>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   case DISPATCH_NEVER_CAT:
      DPRINTF(("SELECT PE("sge_u32"."sge_u32") returns <category_never>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   case DISPATCH_NEVER_JOB:
      DPRINTF(("SELECT PE("sge_u32"."sge_u32") returns <job_never>\n", 
            best->job_id, best->ja_task_id)); 
      break;
   case DISPATCH_MISSING_ATTR:   
   default:
      DPRINTF(("!!!!!!!! SELECT PE("sge_u32"."sge_u32") returns unexpected %d\n", 
            best->job_id, best->ja_task_id, best_result));
      break;
   }

   if (best->is_reservation) {
      schedd_mes_set_logging(old_logging);
   }   

   /* clean up */
   clean_up_parallel_job(best); 
   
   DRETURN(best_result);
}

/****** sge_select_queue/clean_up_parallel_job() *******************************
*  NAME
*     clean_up_parallel_job() -- removes tages
*
*  SYNOPSIS
*     static void clean_up_parallel_job(sge_assignment_t *a) 
*
*  FUNCTION
*     duing pe job dispatch are man queues and hosts taged. This
*     function removes the tages.
*
*  INPUTS
*     sge_assignment_t *a - the resource structure
*
*
*
*  NOTES
*     MT-NOTE: clean_up_parallel_job() is not MT safe 
*
*******************************************************************************/
static
void clean_up_parallel_job(sge_assignment_t *a) 
{
   lListElem *host = NULL;
   qinstance_list_set_tag(a->queue_list, 0);
   for_each(host, a->host_list) {
      lSetUlong(host, EH_tagged, 0);
      lSetUlong(host, EH_master_host, 0); 
   }  

}

/****** scheduler/parallel_reservation_max_time_slots() *****************************************
*  NAME
*     parallel_reservation_max_time_slots() -- Search earliest possible assignment 
*
*  SYNOPSIS
*     static dispatch_t parallel_reservation_max_time_slots(sge_assignment_t *best) 
*
*  FUNCTION
*     The earliest possible assignment is searched for a job assuming a 
*     particular parallel environment be used with a particular slot
*     number. If the slot number passed is 0 we start with the minimum 
*     possible slot number for that job. The search starts with the 
*     latest queue end time if DISPATCH_TIME_QUEUE_END was specified 
*     rather than a real time value.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*
*  RESULT
*     dispatch_t - 0 ok got an assignment
*                  1 no assignment at the specified time (???)
*                 -1 assignment will never be possible for all jobs of that category
*                 -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: parallel_reservation_max_time_slots() is not MT safe 
*******************************************************************************/
static dispatch_t 
parallel_reservation_max_time_slots(sge_assignment_t *best) 
{
   u_long32 pe_time, first_time;
   sge_assignment_t tmp_assignment = SGE_ASSIGNMENT_INIT;
   dispatch_t result = DISPATCH_NEVER_CAT; 
   sge_qeti_t *qeti = NULL;
   bool is_first = true;
   int old_logging = 0;
   category_use_t use_category;
   
   DENTER(TOP_LAYER, "parallel_reservation_max_time_slots");

   /* assemble job category information */
   fill_category_use_t(best, &use_category, lGetString(best->pe, PE_name));  

   assignment_copy(&tmp_assignment, best, false);
   if (best->slots == 0) {
      tmp_assignment.slots = range_list_get_first_id(lGetList(best->job, JB_pe_range), NULL);
   }   

   qeti = sge_qeti_allocate(best->job, best->pe, best->ckpt, 
         best->host_list, best->queue_list, best->centry_list, best->acl_list, best->hgrp_list); 
  
   if (qeti == NULL) {
      ERROR((SGE_EVENT, "could not allocate qeti object needed reservation "
            "scheduling of parallel job "sge_U32CFormat, sge_u32c(best->job_id)));
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if (best->start == DISPATCH_TIME_QUEUE_END) {
      first_time = sge_qeti_first(qeti);
      if (first_time == 0) { /* we need at least one reservation run */
         first_time = sconf_get_now();
      }
   }   
   else {
      /* the first iteration will be done using best->start 
         further ones will use earliert times */
      first_time = best->start;
      sge_qeti_next_before(qeti, best->start);
   }

   old_logging = schedd_mes_get_logging(); /* store logging mode */  
   for (pe_time = first_time ; pe_time; pe_time = sge_qeti_next(qeti)) {
      int available_slots = 0;
      DPRINTF(("SELECT PE TIME(%s, "sge_u32") tries at "sge_u32"\n", 
               lGetString(best->pe, PE_name), best->job_id, pe_time));
      tmp_assignment.start = pe_time;

      /* this is an additional run, we have already at least one posible match,
         all additional scheduling information is not important, since we can
         start the job */
      if (is_first) {
         is_first = false;
      }
      else {
         use_category.mod_category = false;
         schedd_mes_set_logging(0);
      }
      
      result = parallel_assignment(&tmp_assignment, &use_category, &available_slots);

      if (result == DISPATCH_OK) {
         if (best->gdil) {
            DPRINTF(("SELECT PE TIME: earlier assignment at "sge_u32"\n", pe_time));
         }
         assignment_copy(best, &tmp_assignment, true);
      } 
      else {
         DPRINTF(("SELECT PE TIME: no earlier assignment at "sge_u32"\n", pe_time));
         break;
      }
   }
   schedd_mes_set_logging(old_logging); /* restore logging mode */

   sge_qeti_release(&qeti);

   if (best->gdil) {
      result = DISPATCH_OK;
   }   
  
   switch (result) {
   case DISPATCH_OK:
      DPRINTF(("SELECT PE TIME(%s, %d) returns "sge_u32"\n", 
            lGetString(best->pe, PE_name), best->slots, best->start));
      break;
   case DISPATCH_NEVER_CAT:
      DPRINTF(("SELECT PE TIME(%s, %d) returns <category_never>\n", 
            lGetString(best->pe, PE_name), best->slots));
      break;
   case DISPATCH_NEVER_JOB:
      DPRINTF(("SELECT PE TIME(%s, %d) returns <job_never>\n", 
            lGetString(best->pe, PE_name), best->slots));
      break;
   default:
      DPRINTF(("!!!!!!!! SELECT PE TIME(%s, %d) returns unexpected %d\n", 
            lGetString(best->pe, PE_name), best->slots, result));
      break;
   }

   DRETURN(result);
}

/****** scheduler/parallel_maximize_slots_pe() *****************************************
*  NAME
*     parallel_maximize_slots_pe() -- Maximize number of slots for an assignment
*
*  SYNOPSIS
*     static int parallel_maximize_slots_pe(sge_assignment_t *best, lList *host_list, 
*     lList *queue_list, lList *centry_list, lList *acl_list) 
*
*  FUNCTION
*     The largest possible slot amount is searched for a job assuming a 
*     particular parallel environment be used at a particular start time. 
*     If the slot number passed is 0 we start with the minimum 
*     possible slot number for that job.
*
*     To search most efficent for the right slot value, it has three search
*     strategies implemented:
*     - binary search
*     - least slot value first
*     - highest slot value first
*
*     To be able to use binary search all possible slot values are stored in 
*     one array. The slot values in this array are sorted ascending. After the
*     right slot value was found, it is very easy to compute the best strategy
*     from the result. For each strategy it will compute how many iterations
*     would have been needed to compute the correct result. These steps will
*     be stored for the next run and used to figure out the best algorithm.
*     To ensure that we can adapt to rapid changes and also ignore spiks we
*     are using the running avarage algorithm in a 80-20 setting. This means
*     that the algorithm will need 4 (max 5) iterations to adopt to a new
*     secenario. 
*
*  Further enhancements:
*     It might be a good idea to store the derived values with the job cartegories
*     and allow to find the best strategy per category.
*
*  INPUTS
*     sge_assignment_t *best - herein we keep all important in/out information
*     lList *host_list       - a list of all available hosts
*     lList *queue_list      - a list of all available queues
*     lList *centry_list     - a list of all available complex attributes
*     lList *acl_list        - a list of all access lists
*
*  RESULT
*     int - 0 ok got an assignment (maybe without maximizing it) 
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: parallel_maximize_slots_pe() is MT safe as long as the provided 
*              lists are owned be the caller
*
*  SEE ALSO:
*     sconf_best_pe_alg
*     sconf_update_pe_alg
*     add_pe_slots_to_category
*
*******************************************************************************/
static dispatch_t
parallel_maximize_slots_pe(sge_assignment_t *best, int *available_slots) {

   int min_slots, max_slots; 
   int max_pe_slots;
   int first, last;
   lList *pe_range;
   lListElem *pe;
   sge_assignment_t tmp = SGE_ASSIGNMENT_INIT;
   dispatch_t result = DISPATCH_NEVER_CAT; 
   const char *pe_name = lGetString(best->pe, PE_name);
   bool is_first = true;
   int old_logging = 0;
   category_use_t use_category;
   u_long32 max_slotsp = 0;
   int current = 0;
   int match_current = 0;
   int runs = 0;
   schedd_pe_algorithm alg =  sconf_best_pe_alg();
 
   DENTER(TOP_LAYER, "parallel_maximize_slots_pe");

   if ( best == NULL || 
        (pe_range=lGetList(best->job, JB_pe_range)) == NULL || 
        (pe=best->pe) == NULL) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* assemble job category information */
   fill_category_use_t(best, &use_category, pe_name);      
 
   first = range_list_get_first_id(pe_range, NULL);
   last  = range_list_get_last_id(pe_range, NULL);
   max_pe_slots = lGetUlong(pe, PE_slots);

   if (best->slots) {
      min_slots = best->slots;
   }   
   else {
      min_slots = first;
   }   

   if (best->gdil || best->slots == max_pe_slots) { /* already found maximum */
      DRETURN(DISPATCH_OK); 
   }

   DPRINTF(("MAXIMIZE SLOT: FIRST %d LAST %d MAX SLOT %d\n", first, last, max_pe_slots));

   /* this limits max range number RANGE_INFINITY (i.e. -pe pe 1-) to reasonable number */
   max_slots = MIN(last, max_pe_slots);

   DPRINTF(("MAXIMIZE SLOT FOR "sge_u32" using \"%s\" FROM %d TO %d\n", 
      best->job_id, pe_name, min_slots, max_slots));

   assignment_copy(&tmp, best, false);

   old_logging = schedd_mes_get_logging(); /* store logging mode */  

   if ((max_slots < min_slots) ||
      (min_slots <= 0)) {
      ERROR((SGE_EVENT, "invalid pe job range setting for job "sge_u32"\n", best->job_id));
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* --- prepare the posible slots for the binary search */
   max_slotsp = (max_slots - min_slots+1);
   if (!add_pe_slots_to_category(&use_category, &max_slotsp, pe, min_slots, max_slots, pe_range)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NOMEM));
      DRETURN(DISPATCH_NEVER_CAT);
   }
   if (max_slotsp == 0) {
      DPRINTF(("no slots in PE %s available for job "sge_u32"\n", pe_name, best->job_id));     
      DRETURN(DISPATCH_NEVER_CAT);
   }
   
   /* --- work on the different slot ranges and try to find the best one --- */
   if (alg == SCHEDD_PE_BINARY) {
      int min = 0; 
      int max = max_slotsp-1;
         
      do {
         runs++;
         current = (min + max) / 2;
       
         if (is_first) { /* first run, collect information */
            is_first = false;
         }
         else { /* this is an additional run, we have already at least one posible match */
            use_category.mod_category = false;
            schedd_mes_set_logging(0);
         }

         /* we try that slot amount */
         tmp.slots = use_category.posible_pe_slots[current];
         result = parallel_assignment(&tmp, &use_category, available_slots);         
        
         if (result == DISPATCH_OK) {
            assignment_copy(best, &tmp, true);
            match_current = current;
            min = current + 1;
         }
         else {
            max = current - 1;
         }
         
      } while (min <= max && max != -1);
      
   }
   else {
      /* compute how many runs the bin search might have needed */
      /* This will not give us the correct answer in all cases, but
         it is close enough. And on average it is corrent :-) */
      int end = max_slotsp;
      for ( runs=1; runs < end; runs++) {
         end = end - runs;
      }
      runs--;

      if (alg == SCHEDD_PE_LOW_FIRST) {
         for (current = 0; current < max_slotsp; current++) {

            if (is_first) { /* first run, collect information */
               is_first = false;
            }
            else {  /* this is an additional run, we have already at least one posible match */
               use_category.mod_category = false;
               schedd_mes_set_logging(0);
            }

            /* we try that slot amount */ 
            tmp.slots = use_category.posible_pe_slots[current];
            result = parallel_assignment(&tmp, &use_category, available_slots);

            if (result != DISPATCH_OK) { /* we have mismatch, stop */
               break;                    /* the other runs will not work */
            }   
            
            match_current = current;
            assignment_copy(best, &tmp, true);
         }
      }
      else { /* optimistic search */
         for (current = max_slotsp-1; current >= 0; current--) {

            if (is_first) { /* first run, collect information */
               is_first = false;
            }
            else { /* this is an additional run, we have already at least one posible match */
               use_category.mod_category = false;
               schedd_mes_set_logging(0);
            }

            /* we try that slot amount */ 
            tmp.slots = use_category.posible_pe_slots[current];
            result = parallel_assignment(&tmp, &use_category, available_slots);

            if (result == DISPATCH_OK) {          /* we have a match, stop */
               assignment_copy(best, &tmp, true); /* all other runs will also match */
               match_current = current;
               break;
            } /* end if*/
         } /* end for */
      }
      
   } /* end for */
  
   sconf_update_pe_alg(runs, match_current, max_slotsp);

   
   /* --- we are done --- */
   if (!use_category.is_pe_slots_rev) {
      FREE(use_category.posible_pe_slots);
   }

   assignment_release(&tmp);   

   schedd_mes_set_logging(old_logging); /* restore logging mode */

   if (best->gdil) {
      result = DISPATCH_OK;
   }   

   switch (result) {
      case DISPATCH_OK:
         if (!best->is_reservation) { 
            sconf_inc_comprehensive_jobs();
         }   
         
         DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <ok>\n", 
                  pe_name, best->slots));
         break;
      case DISPATCH_NOT_AT_TIME:
         DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <later>\n", 
               pe_name, best->slots));
         break;
      case DISPATCH_NEVER_CAT:
         DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <category_never>\n", 
               pe_name, best->slots));
         break;
      case DISPATCH_NEVER_JOB:
         DPRINTF(("MAXIMIZE SLOT(%s, %d) returns <job_never>\n", 
               pe_name, best->slots));
         break;
      default:
         DPRINTF(("!!!!!!!! MAXIMIZE SLOT(%d, %d) returns unexpected %d\n", 
               best->slots, (int)best->start, result));
         break;
   }

   DRETURN(result);
}

/****** sge_select_queue/sge_select_queue() ************************************
*  NAME
*     sge_select_queue() -- checks weather a job fits on a given queue or host 
*
*  SYNOPSIS
*     int sge_select_queue(lList *requested_attr, lListElem *queue, lListElem 
*     *host, lList *exechost_list, lList *centry_list, bool 
*     allow_non_requestable, int slots) 
*
*  FUNCTION
*     Takes the requested attributes from a job and checks if it fits to the given
*     host or queue. Only of of them has to be specified. It both, the function
*     assumes, that the queue belongs to the given host. 
*
*  INPUTS
*     lList *requested_attr     - list of requested attributes 
*     lListElem *queue          - current queue or null if host is set
*     lListElem *host           - current host or null if queue is set
*     lList *exechost_list      - list of all hosts in the system
*     lList *centry_list        - system wide attribut config list
*     bool allow_non_requestable - allow non requestable?
*     int slots                 - number of requested slots
*     lList *queue_user_list    - list of users or null
*     lList *acl_list           - acl_list or null
*     lListElem *job            - job or null
*
*  RESULT
*     int - 1, if okay, QU_tag will be set if a queue is selected
*           0, if not okay 
*  
*  NOTES
*   The caller is responsible for cleaning tags.   
*
*   No range is used. For serial jobs we will need a call for hard and one
*    for soft requests. For parallel jobs we will call this function for each
*   -l request. Because of in serial jobs requests can be simply added.
*   In Parallel jobs each -l requests a different set of queues.
*
*******************************************************************************/
bool 
sge_select_queue(lList *requested_attr, lListElem *queue, lListElem *host, 
                 lList *exechost_list, lList *centry_list, bool allow_non_requestable, 
                 int slots, lList *queue_user_list, lList *acl_list, lListElem *job) 
{
   dispatch_t ret;
   lList *load_attr = NULL;
   lList *config_attr = NULL;
   lList *actual_attr = NULL; 
   lListElem *global = NULL;
   lListElem *qu = NULL;
   int q_access=1;
   lList *projects;
   const char *project;
    
   sge_assignment_t a = SGE_ASSIGNMENT_INIT;
   double lc_factor = 0; /* scaling for load correction */ 
   u_long32 ulc_factor; 
   /* actually we don't care on start time here to this is just a dummy setting */
   u_long32 start_time = DISPATCH_TIME_NOW; 

   DENTER(TOP_LAYER, "sge_select_queue");

   clear_resource_tags(requested_attr, MAX_TAG);
   
   assignment_init(&a, NULL, NULL, false);
   a.centry_list      = centry_list;
   a.host_list        = exechost_list;

   if (acl_list != NULL) {
      /* check if job owner has access rights to the queue */
      DPRINTF(("testing queue access lists\n"));
      for_each(qu, queue_user_list) {
         const char *name = lGetString(qu, ST_name);
         DPRINTF(("-----> checking queue user: %s\n", name ));
         q_access |= (name[0]=='@')?
                     sge_has_access(NULL, &name[1], queue, acl_list): 
                     sge_has_access(name, NULL, queue, acl_list); 
      }
      if (q_access == 0) {
         DPRINTF(("no access\n"));
         DRETURN(false); 
      } else {
         DPRINTF(("ok\n"));
      }
   }

   if (job != NULL) {
      /* check if job can run in queue based on project */
      DPRINTF(("testing job projects lists\n"));
      if ( (project = lGetString(job, JB_project)) ) { 
         if ((!(projects = lGetList(queue, QU_projects)))) {
            DPRINTF(("no access because queue has no project\n"));
            DRETURN(false);
         }
         if ((!userprj_list_locate(projects, project))) {
            DPRINTF(("no access because project not contained in queues project list"));
            DRETURN(false);
         }
         DPRINTF(("ok\n"));

         /* check if job can run in queue based on excluded projects */
         DPRINTF(("testing job xprojects lists\n"));
         if ((projects = lGetList(queue, QU_xprojects))) {
            if (((project = lGetString(job, JB_project)) &&
                 userprj_list_locate(projects, project))) {
               DPRINTF(("no access\n"));
               DRETURN(false);
            }
         }
         DPRINTF(("ok\n"));
      }

      /* 
      *is queue contained in hard queue list ?
      */
      DPRINTF(("queue contained in jobs hard queue list?\n"));
      if (lGetList(job, JB_hard_queue_list)) {
         lList *qref_list = lGetList(job, JB_hard_queue_list);
         const char *qname = NULL;
         const char *qinstance_name = NULL;

         qname = lGetString(queue, QU_qname);
         qinstance_name = lGetString(queue, QU_full_name);
         if ((lGetElemStr(qref_list, QR_name, qname) != NULL) || 
             (lGetElemStr(qref_list, QR_name, qinstance_name) != NULL)) {
            DPRINTF(("ok"));
         } else {
            DPRINTF(("denied because queue \"%s\" is not contained in the hard "
                     "queue list (-q) that was requested by job %d\n",
                     qname, lGetUlong(job, JB_job_number)));
            DRETURN(false);
         }
      }
   }

/* global */
   global = host_list_locate(exechost_list, SGE_GLOBAL_NAME);
   load_attr = lGetList(global, EH_load_list); 
   config_attr = lGetList(global, EH_consumable_config_list);
   actual_attr = lGetList(global, EH_resource_utilization);

   /* is there a multiplier for load correction (may be not in qstat, qmon etc) */
   if (lGetPosViaElem(global, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
      if ((ulc_factor=lGetUlong(global, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   } 

   ret = rc_time_by_slots(&a, requested_attr, load_attr, config_attr, actual_attr, 
            NULL, allow_non_requestable, NULL, slots, DOMINANT_LAYER_HOST, lc_factor, GLOBAL_TAG,
            &start_time, SGE_GLOBAL_NAME);

/* host */
   if(ret == DISPATCH_OK || ret == DISPATCH_MISSING_ATTR){
      if(host == NULL) {
         host = host_list_locate(exechost_list, lGetHost(queue, QU_qhostname));
      }   
      load_attr = lGetList(host, EH_load_list); 
      config_attr = lGetList(host, EH_consumable_config_list);
      actual_attr = lGetList(host, EH_resource_utilization);

      if (lGetPosViaElem(host, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
         if ((ulc_factor=lGetUlong(host, EH_load_correction_factor)))
            lc_factor = ((double)ulc_factor)/100;
      }

      ret = rc_time_by_slots(&a, requested_attr, load_attr, config_attr, 
                             actual_attr, NULL, allow_non_requestable, NULL, 
                             slots, DOMINANT_LAYER_HOST, lc_factor, HOST_TAG, 
                             &start_time, lGetHost(host, EH_name));

/* queue */
     if((ret == DISPATCH_OK || ret == DISPATCH_MISSING_ATTR) && queue){
         config_attr = lGetList(queue, QU_consumable_config_list);
         actual_attr = lGetList(queue, QU_resource_utilization);
   
         ret = rc_time_by_slots(&a, requested_attr, NULL, config_attr, actual_attr,  
               queue, allow_non_requestable, NULL, slots, DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG, 
               &start_time,  lGetString(queue, QU_full_name));
      }
   }

   assignment_release(&a);
   
   DRETURN((ret == DISPATCH_OK) ? true : false);
}


/****** sge_select_queue/rc_time_by_slots() **********************************
*  NAME
*     rc_time_by_slots() -- checks weather all resource requests on one level
*                             are fulfilled 
*
*  SYNOPSIS
*     static int rc_time_by_slots(lList *requested, lList *load_attr, lList 
*     *config_attr, lList *actual_attr, lList *centry_list, lListElem *queue, 
*     bool allow_non_requestable, char *reason, int reason_size, int slots, 
*     u_long32 layer, double lc_factor, u_long32 tag) 
*
*  FUNCTION
*     Checks, weather all requests, default requests and implicit requests on this
*     this level are fulfilled.
*
*     With reservation scheduling the earliest start time due to resources of the 
*     resource container is the maximum of the earliest start times for all 
*     resources comprised by the resource container that requested by a job.
*
*  INPUTS
*     lList *requested          - list of attribute requests 
*     lList *load_attr          - list of load attributes or null on queue level
*     lList *config_attr        - list of user defined attributes 
*     lList *actual_attr        - usage of all consumables (RUE_Type)
*     lList *centry_list        - system wide attribute config. list (CE_Type)
*     lListElem *queue          - current queue or NULL on global/host level
*     bool allow_non_requestable - allow none requestabales? 
*     char *reason              - error message
*     int reason_size           - max error message size
*     int slots                 - number of slots the job is looking for 
*     u_long32 layer            - current layer flag 
*     double lc_factor          - load correction factor 
*     u_long32 tag              - current layer tag 
*     u_long32 *start_time      - in/out argument for start time  
*     u_long32 duration         - jobs estimated total run time
*     const char *object_name   - name of the object used for monitoring purposes
*
*  RESULT
*     dispatch_t - 
*
*  NOTES
*     MT-NOTES: is not thread save. uses a static buffer
*
*  Important:
*     we have some special behavior, when slots is set to -1.
*******************************************************************************/
static dispatch_t
rc_time_by_slots(const sge_assignment_t *a, lList *requested, lList *load_attr, lList *config_attr, 
                 lList *actual_attr, lListElem *queue, bool allow_non_requestable, 
                 dstring *reason, int slots, u_long32 layer, double lc_factor, u_long32 tag,
                 u_long32 *start_time, const char *object_name) 
{
   static lListElem *implicit_slots_request = NULL;
   lListElem *attr;
   u_long32 latest_time = DISPATCH_TIME_NOW;
   u_long32 tmp_start;
   dispatch_t ret;
   bool is_not_found = false;

   DENTER(TOP_LAYER, "rc_time_by_slots");

   clear_resource_tags(requested, QUEUE_TAG); 

   /* ensure availability of implicit slot request */
   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, "slots");
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* match number of free slots */
   if (slots != -1) {
      tmp_start = *start_time;
      ret = ri_time_by_slots(a, implicit_slots_request, load_attr, config_attr, actual_attr, queue,  
                       reason, allow_non_requestable, slots, layer, lc_factor, &tmp_start, object_name);

      if (ret == DISPATCH_OK && *start_time == DISPATCH_TIME_QUEUE_END) {
         DPRINTF(("%s: \"slot\" request delays start time from "sge_U32CFormat
           " to "sge_U32CFormat"\n", object_name, latest_time, MAX(latest_time, tmp_start)));
         latest_time = MAX(latest_time, tmp_start);
      }

      /* we don't care if slots are not specified, except at queue level */
      if (ret == DISPATCH_MISSING_ATTR && tag != QUEUE_TAG) {
         ret = DISPATCH_OK;
      }   
      if (ret != DISPATCH_OK) {
         DRETURN(ret);
      }

   }

   /* ensure all default requests are fulfilled */
   if (slots != -1 && !allow_non_requestable) {
      lListElem *attr;
      dispatch_t ff;
      const char *name;
      double dval=0.0;
      u_long32 valtype;

      for_each (attr, actual_attr) {
         name = lGetString(attr, RUE_name);
         if (!strcmp(name, "slots")) {
            continue;
         }   

         /* consumable && used in this global/host/queue && not requested */
         if (!is_requested(requested, name)) {
            lListElem *default_request = lGetElemStr(a->centry_list, CE_name, name);
            const char *def_req = lGetString(default_request, CE_default);
            valtype = lGetUlong(default_request, CE_valtype);
            parse_ulong_val(&dval, NULL, valtype, def_req, NULL, 0);

            /* ignore default request if the value is 0 */
            if(def_req != NULL && dval != 0.0) {
               dstring tmp_reason;
               char tmp_reason_buf[2048];

               sge_dstring_init(&tmp_reason, tmp_reason_buf, sizeof(tmp_reason_buf));

               /* build the default request */
               lSetString(default_request, CE_stringval, def_req);
               lSetDouble(default_request, CE_doubleval, dval);

               tmp_start = *start_time;
               ff = ri_time_by_slots(a, default_request, load_attr, config_attr, actual_attr, 
                     queue, &tmp_reason, true, slots, layer, lc_factor, &tmp_start, object_name);

               if (ff != DISPATCH_OK) {
                  /* useless to continue in these cases */
                  sge_dstring_append(reason, MSG_SCHEDD_FORDEFAULTREQUEST);
                  sge_dstring_append_dstring(reason, &tmp_reason);
                  DRETURN(ff);
               }

               if (*start_time == DISPATCH_TIME_QUEUE_END) {
                  DPRINTF(("%s: default request \"%s\" delays start time from "sge_U32CFormat 
                        " to "sge_U32CFormat"\n", object_name, name, latest_time, MAX(latest_time, tmp_start)));
                  latest_time = MAX(latest_time, tmp_start);
               }
            } 
         } 
      }/* end for*/
   }
 
   if (slots == -1) {
      slots = 1;
   }   

   /* explicit requests */
   for_each (attr, requested) {
      const char *attr_name = lGetString(attr, CE_name);

      tmp_start = *start_time;
      switch (ri_time_by_slots(a, attr,load_attr, config_attr, actual_attr, queue, 
               reason, allow_non_requestable, slots, layer, lc_factor, &tmp_start, object_name)) {
         
         case DISPATCH_NEVER_CAT : /* will never match */ 
                  DRETURN(DISPATCH_NEVER_CAT);
                  
         case DISPATCH_OK : /* a match was found */
               if (*start_time == DISPATCH_TIME_QUEUE_END) {
                  DPRINTF(("%s: explicit request \"%s\" delays start time from "sge_U32CFormat 
                           "to "sge_U32CFormat"\n", object_name, attr_name, latest_time, 
                           MAX(latest_time, tmp_start)));
                  latest_time = MAX(latest_time, tmp_start);
               }
               if (lGetUlong(attr, CE_tagged) < tag) {
                  lSetUlong(attr, CE_tagged, tag);
               }   
            break;
            
         case DISPATCH_NOT_AT_TIME : /* will match later-on */
                  DPRINTF(("%s: request for %s will match later-on\n", object_name, attr_name));
                  DRETURN(DISPATCH_NOT_AT_TIME);
                  
         case DISPATCH_MISSING_ATTR : /* the requested element does not exist */
            if (tag == QUEUE_TAG && lGetUlong(attr, CE_tagged) == NO_TAG) {
               sge_dstring_sprintf(reason, MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE_S, attr_name);
               DRETURN(DISPATCH_NEVER_CAT);
            }

            if (tag != QUEUE_TAG) {
               is_not_found = true;
            }

            break;
         default: /* error */
            break;
      }
   }

   if (*start_time == DISPATCH_TIME_QUEUE_END) {
      *start_time = latest_time;
   }

   if (is_not_found) {
      DRETURN(DISPATCH_MISSING_ATTR);
   } 

   DRETURN(DISPATCH_OK);
}

static dispatch_t 
match_static_resource(int slots, lListElem *req_cplx, lListElem *src_cplx, dstring *reason,
                      int is_threshold, int force_existence, bool allow_non_requestable)
{
   int match;
   dispatch_t ret = DISPATCH_OK;
   char availability_text[2048];

   DENTER(TOP_LAYER, "match_static_resource");

   /* check whether attrib is requestable */
   if (!allow_non_requestable && lGetUlong(src_cplx, CE_requestable) == REQU_NO) {
      sge_dstring_append(reason, MSG_SCHEDD_JOBREQUESTSNONREQUESTABLERESOURCE);
      sge_dstring_append(reason, lGetString(src_cplx, CE_name));
      sge_dstring_append(reason, "\"");
      DRETURN(DISPATCH_NEVER_CAT);
   }

   match = compare_complexes(slots, req_cplx, src_cplx, availability_text, false, false);

   if (!match) {
      sge_dstring_append(reason, MSG_SCHEDD_ITOFFERSONLY);
      sge_dstring_append(reason, availability_text);
      ret = DISPATCH_NEVER_CAT;
   }

   DRETURN(ret);
}

/****** sge_select_queue/clear_resource_tags() *********************************
*  NAME
*     clear_resource_tags() -- removes the tags from a resouce request. 
*
*  SYNOPSIS
*     static void clear_resource_tags(lList *resouces, u_long32 max_tag) 
*
*  FUNCTION
*     Removes the tages from the given resouce list. A tag is only removed
*     if it is smaller or equal to the given tag value. The tag value "MAX_TAG" results
*     in removing all existing tags, or the value "HOST_TAG" removes queue and host
*     tags but keeps the global tags.
*
*  INPUTS
*     lList *resouces  - list of job requests. 
*     u_long32 max_tag - max tag element 
*
*******************************************************************************/
static void clear_resource_tags( lList *resources, u_long32 max_tag) {

   lListElem *attr=NULL;

   for_each(attr, resources){
      if(lGetUlong(attr, CE_tagged) <= max_tag)
         lSetUlong(attr, CE_tagged, NO_TAG);
   }
}


/****** sge_select_queue/sge_queue_match_static() ************************
*  NAME
*     sge_queue_match_static() -- Do matching that depends not on time.
*
*  SYNOPSIS
*     static int sge_queue_match_static(lListElem *queue, lListElem *job, 
*     const lListElem *pe, const lListElem *ckpt, lList *centry_list, lList 
*     *host_list, lList *acl_list) 
*
*  FUNCTION
*     Checks if a job fits on a queue or not. All checks that depend on the 
*     current load and resource situation must get handled outside. 
*     The queue also gets tagged in QU_tagged4schedule to indicate whether it
*     is specified using -masterq queue_list.
*
*  INPUTS
*     lListElem *queue      - The queue we're matching
*     lListElem *job        - The job
*     const lListElem *pe   - The PE object
*     const lListElem *ckpt - The ckpt object
*     lList *centry_list    - The centry list
*     lList *acl_list       - The ACL list
*
*  RESULT
*     dispatch_t -  0 ok
*                  -1 assignment will never be possible for all jobs of that category
*
*  NOTES
*******************************************************************************/
dispatch_t sge_queue_match_static(lListElem *queue, lListElem *job, const lListElem *pe, 
                                  const lListElem *ckpt, lList *centry_list, lList *acl_list, lList *hgrp_list) 
{
   u_long32 job_id;
   lList *projects;
   const char *project;
   const lList *hard_queue_list, *master_hard_queue_list;
   const char *qinstance_name = lGetString(queue, QU_full_name);

   DENTER(TOP_LAYER, "sge_queue_match_static");

   job_id = lGetUlong(job, JB_job_number);
   /* check if job owner has access rights to the queue */
   if (!sge_has_access(lGetString(job, JB_owner), lGetString(job, JB_group), queue, acl_list)) {
      DPRINTF(("Job %d has no permission for queue %s\n", (int)job_id, qinstance_name));
      schedd_mes_add(job_id, SCHEDD_INFO_HASNOPERMISSION_SS, "queue", qinstance_name);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* check if job can run in queue based on project */
   if ((projects = lGetList(queue, QU_projects))) {
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "queue", qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "queue", qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   /* check if job can run in queue based on excluded projects */
   if ((projects = lGetList(queue, QU_xprojects))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "queue", qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   hard_queue_list = lGetList(job, JB_hard_queue_list);
   master_hard_queue_list = lGetList(job, JB_master_hard_queue_list);
   if (hard_queue_list || master_hard_queue_list) {
      if (!centry_list_are_queues_requestable(centry_list)) {
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTREQUESTABLE_S,
            qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   /* 
    * is queue contained in hard queue list ? 
    */
   if (hard_queue_list) {
      if (qref_list_cq_rejected(hard_queue_list, lGetString(queue, QU_qname),
                     lGetHost(queue, QU_qhostname), hgrp_list)) {
         DPRINTF(("Queue \"%s\" is not contained in the hard "
                  "queue list (-q) that was requested by job %d\n",
                  qinstance_name, (int) job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINHARDQUEUELST_S,
                        qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      } 
   }


   /* 
    * is this queue a candidate for being the master queue? 
    */
   if (master_hard_queue_list) {
      bool is_in_list = true;
      if (qref_list_cq_rejected(master_hard_queue_list, lGetString(queue, QU_qname),
                     lGetHost(queue, QU_qhostname), hgrp_list))
         is_in_list = false;

      /*
       * Tag queue
       */
      lSetUlong(queue, QU_tagged4schedule, is_in_list ? 1 : 0);
      if (!is_in_list) {
         DPRINTF(("Queue \"%s\" is contained in the master hard "
                  "queue list (-masterq) that was requested by job %d\n",
                  qinstance_name, (int) job_id));
      }
   }

   /*
   ** different checks for different job types:
   */

   if (pe) { /* parallel job */
      if (!qinstance_is_parallel_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a parallel queue as requested by " 
                  "job %d\n", qinstance_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELQUEUE_S, qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }

      /*
       * check if the requested PE is named in the PE reference list of Queue
       */
      if (!qinstance_is_pe_referenced(queue, pe)) {
         DPRINTF(("Queue "SFQ" does not reference PE "SFQ"\n",
                  qinstance_name, lGetString(pe, PE_name)));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFPE_SS,
                        qinstance_name, lGetString(pe, PE_name));
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   if (ckpt) { /* ckpt job */
      /* is it a ckpt queue ? */
      if (!qinstance_is_checkointing_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a checkpointing queue as requested by "
                  "job %d\n", qinstance_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTACKPTQUEUE_SS, qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }

      /*
       * check if the requested CKPT is named in the CKPT ref list of Queue
       */
      if (!qinstance_is_ckpt_referenced(queue, ckpt)) {
         DPRINTF(("Queue \"%s\" does not reference checkpointing object "SFQ
                  "\n", qinstance_name, lGetString(ckpt, CK_name)));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFCKPT_SS,  
                        qinstance_name, lGetString(ckpt, CK_name));
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }   

   /* to be activated as soon as immediate jobs are available */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { 
      if (!qinstance_is_interactive_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not an interactive queue as requested by "
                  "job %d\n", qinstance_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTINTERACTIVE_S, qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      } 
   }

   if (!pe && !ckpt && !JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* serial (batch) job */
      /* is it a batch or transfer queue */
      if (!qinstance_is_batch_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a batch queue as "
                  "requested by job %d\n", qinstance_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTASERIALQUEUE_S, qinstance_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   if (ckpt && !pe && lGetString(job, JB_script_file) &&
       qinstance_is_parallel_queue(queue) && !qinstance_is_batch_queue(queue)) {
      DPRINTF(("Queue \"%s\" is not a serial queue as "
               "requested by job %d\n", qinstance_name, (int)job_id));
      schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELJOB_S, qinstance_name);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if (job_is_forced_centry_missing(job, centry_list, queue)) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   DRETURN(DISPATCH_OK);
}

static bool 
job_is_forced_centry_missing(const lListElem *job, 
                             const lList *master_centry_list, 
                             const lListElem *queue_or_host)
{
   bool ret = false;
   lListElem *centry;

   DENTER(TOP_LAYER, "job_is_forced_centry_missing");
   if (job != NULL && master_centry_list != NULL && queue_or_host != NULL) {
      lList *res_list = lGetList(job, JB_hard_resource_list);  

      for_each(centry, master_centry_list) {
         const char *name = lGetString(centry, CE_name);
         bool is_requ = is_requested(res_list, name);
         bool is_forced = lGetUlong(centry, CE_requestable) == REQU_FORCED ? true : false;
         const char *object_name = NULL;
         bool is_qinstance = object_has_type(queue_or_host, QU_Type);
         bool is_host = object_has_type(queue_or_host, EH_Type);

         if (is_forced) {
            if (is_qinstance) {
               is_forced = qinstance_is_centry_a_complex_value(queue_or_host, centry);
               object_name = lGetString(queue_or_host, QU_full_name);
            } else if (is_host) {
               is_forced = host_is_centry_a_complex_value(queue_or_host, centry);
               object_name = lGetHost(queue_or_host, EH_name);
            } else {
               DTRACE;
               is_forced = false;
            }
         }

         if (is_forced && !is_requ) {
            u_long32 job_id = lGetUlong(job, JB_job_number);

            DPRINTF(("job "sge_u32" does not request 'forced' resource "SFQ" of "
                     SFN"\n", job_id, name, object_name));
            if (is_qinstance) {
               schedd_mes_add(job_id, SCHEDD_INFO_NOTREQFORCEDRES_SS, 
                              name, object_name);
            } else if (is_host) {
               schedd_mes_add(job_id, SCHEDD_INFO_NOFORCEDRES_SS, 
                              name, object_name);
            }
            ret = true;
            break;
         }
      }
   }
   DRETURN(ret);
}

/****** sge_select_queue/compute_soft_violations() ********************************
*  NAME
*     compute_soft_violations() -- counts the violations in the request for a given host or queue 
*
*  SYNOPSIS
*     static int compute_soft_violations(lListElem *queue, int violation, lListElem *job,lList *load_attr, lList *config_attr,
*                               lList *actual_attr, lList *centry_list, u_long32 layer, double lc_factor, u_long32 tag)
*
*  FUNCTION
*     this function checks if the current resources can satisfy the requests. The resources come from the global host, a
*     given host or the queue. The function returns the number of violations. 
*
*  INPUTS
*     const sge_assignment_t *a - job info structure
*     lListElem *queue     - should only be set, when one using this method on queue level 
*     int violation        - the number of previous violations. This is needed to get a correct result on queue level. 
*     lList *load_attr     - the load attributs, only when used on hosts or global 
*     lList *config_attr   - a list of custom attributes  (CE_Type)
*     lList *actual_attr   - a list of custom consumables, they contain the current usage of these attributes (RUE_Type)
*     u_long32 layer       - the curent layer flag 
*     double lc_factor     - should be set, when load correction has to be done. 
*     u_long32 tag         - the current layer tag. (GLOGAL_TAG, HOST_TAG, QUEUE_TAG) 
*
*  RESULT
*     static int - the number of violations ( = (prev. violations) + (new violations in this run)). 
*
*******************************************************************************/
static int 
compute_soft_violations(const sge_assignment_t *a, lListElem *queue, int violation, lList *load_attr, lList *config_attr,
                    lList *actual_attr, u_long32 layer, double lc_factor, u_long32 tag) 
{
   u_long32 job_id;
   const char *queue_name = NULL;
   dstring reason;
   char reason_buf[1024 + 1];
   unsigned int soft_violation = violation;
   lList *soft_requests = NULL; 
   lListElem *attr;
   u_long32 start_time = DISPATCH_TIME_NOW; 

   DENTER(TOP_LAYER, "compute_soft_violations");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));

   soft_requests = lGetList(a->job, JB_soft_resource_list);
   clear_resource_tags(soft_requests, tag);

   job_id = lGetUlong(a->job, JB_job_number);
   if (queue) {
      queue_name = lGetString(queue, QU_full_name);
   }   

   /* count number of soft violations for _one_ slot of this job */

   for_each (attr, soft_requests) {
      switch (ri_time_by_slots(a, attr, load_attr, config_attr, actual_attr, queue,
                      &reason, false, 1, layer, lc_factor, &start_time, queue_name?queue_name:"no queue")) {
            /* no match */
            case -1 :   soft_violation++;
               break;
            /* element not found */
            case 1 : 
            case 2 : 
            if (tag == QUEUE_TAG && lGetUlong(attr, CE_tagged) == NO_TAG) {
                           soft_violation++;
            }               
               break;
            /* everything is fine */
            default : if( lGetUlong(attr, CE_tagged) < tag)
                           lSetUlong(attr, CE_tagged, tag);
      }
   }

   if (queue) {
      DPRINTF(("queue %s does not fulfill soft %d requests (first: %s)\n", 
         queue_name, soft_violation, reason_buf));

      /* 
       * check whether queue fulfills soft queue request of the job (-q) 
       */
      if (lGetList(a->job, JB_soft_queue_list)) {
         lList *qref_list = lGetList(a->job, JB_soft_queue_list);
         lListElem *qr;
         const char *qinstance_name = NULL;

         qinstance_name = lGetString(queue, QU_full_name);

         for_each (qr, qref_list) {
            if (qref_cq_rejected(lGetString(qr, QR_name), lGetString(queue, QU_qname),
                lGetHost(queue, QU_qhostname), a->hgrp_list)) {
               DPRINTF(("Queue \"%s\" is not contained in the soft "
                        "queue list (-q) that was requested by job %d\n",
                        qinstance_name, (int) job_id));
               soft_violation++;
            }
         }
      }

      /* store number of soft violations in queue */
      lSetUlong(queue, QU_soft_violation, soft_violation);
   }

   DRETURN(soft_violation);
}

/****** sge_select_queue/sge_host_match_static() ********************************
*  NAME
*     sge_host_match_static() -- Static test whether job fits to host
*
*  SYNOPSIS
*     static int sge_host_match_static(lListElem *job, lListElem *ja_task, 
*     lListElem *host, lList *centry_list, lList *acl_list) 
*
*  FUNCTION
*
*  INPUTS
*     lListElem *job     - ??? 
*     lListElem *ja_task - ??? 
*     lListElem *host    - ??? 
*     lList *centry_list - ??? 
*     lList *acl_list    - ??? 
*
*  RESULT
*     int - 0 ok 
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*******************************************************************************/
dispatch_t
sge_host_match_static(lListElem *job, lListElem *ja_task, lListElem *host, 
                      lList *centry_list, lList *acl_list) 
{
   lList *projects;
   const char *project;
   u_long32 job_id;
   const char *eh_name;

   DENTER(TOP_LAYER, "sge_host_match_static");

   if (!host) {
      DRETURN(DISPATCH_OK);
   }

   job_id = lGetUlong(job, JB_job_number);
   eh_name = lGetHost(host, EH_name);

   /* check if job owner has access rights to the host */
   if (!sge_has_access_(lGetString(job, JB_owner),
         lGetString(job, JB_group), lGetList(host, EH_acl),
         lGetList(host, EH_xacl), acl_list)) {
      DPRINTF(("Job %d has no permission for host %s\n",
               (int)job_id, eh_name));
      schedd_mes_add(job_id, SCHEDD_INFO_HASNOPERMISSION_SS,
         "host", eh_name);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* check if job can run on host based on required projects */
   if ((projects = lGetList(host, EH_prj))) {
   
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "host", eh_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }

      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "host", eh_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   /* check if job can run on host based on excluded projects */
   if ((projects = lGetList(host, EH_xprj))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "host", eh_name);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   if (job_is_forced_centry_missing(job, centry_list, host)) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* RU: */
   /* 
   ** check if job can run on host based on the list of jids/taskids
   ** contained in the reschedule_unknown-list
   */
   if (ja_task) {
      lListElem *ruep;
      lList *rulp;
      u_long32 task_id;

      task_id = lGetUlong(ja_task, JAT_task_number);
      rulp = lGetList(host, EH_reschedule_unknown_list);

      for_each(ruep, rulp) {
         if (lGetUlong(ruep, RU_job_number) == job_id 
             && lGetUlong(ruep, RU_task_number) == task_id) {
            DPRINTF(("RU: Job "sge_u32"."sge_u32" Host "SFN"\n", job_id,
               task_id, eh_name));
            schedd_mes_add(job_id, SCHEDD_INFO_CLEANUPNECESSARY_S,
               eh_name);
            DRETURN(DISPATCH_NEVER_JOB);
         }
      }
   } 

   DRETURN(DISPATCH_OK);
}

/****** sge_select_queue/is_requested() ****************************************
*  NAME
*     is_requested() -- Returns true if specified resource is requested. 
*
*  SYNOPSIS
*     bool is_requested(lList *req, const char *attr) 
*
*  FUNCTION
*     Returns true if specified resource is requested. Both long name
*     and shortcut name are checked.
*
*  INPUTS
*     lList *req       - The request list (CE_Type)
*     const char *attr - The resource name.
*
*  RESULT
*     bool - true if requested, otherwise false 
*
*  NOTES
*     MT-NOTE: is_requested() is MT safe 
*******************************************************************************/
bool is_requested(lList *req, const char *attr) 
{
   if (lGetElemStr(req, CE_name, attr) ||
       lGetElemStr(req, CE_shortcut , attr)) {
      return true;
   }

   return false;
}

static int load_check_alarm(char *reason, const char *name, const char *load_value, 
                                const char *limit_value, u_long32 relop, 
                                u_long32 type, lListElem *hep, 
                                lListElem *hlep, double lc_host, 
                                double lc_global, const lList *load_adjustments, int load_is_value) 
{
   lListElem *job_load;
   double limit, load;
   int match;
#define STR_LC_DIAGNOSIS 1024   
   char lc_diagnosis1[STR_LC_DIAGNOSIS], lc_diagnosis2[STR_LC_DIAGNOSIS];
   
   DENTER(TOP_LAYER, "load_check_alarm");

   switch (type) {
      case TYPE_INT:
      case TYPE_TIM:
      case TYPE_MEM:
      case TYPE_BOO:
      case TYPE_DOUBLE:
         if (!parse_ulong_val(&load, NULL, type, load_value, NULL, 0)) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDLOAD_SS, load_value, name);
            DRETURN(1);
         }
         if (!parse_ulong_val(&limit, NULL, type, limit_value, NULL, 0)) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDTHRESHOLD_SS, name, limit_value);
            DRETURN(1);
         }
         if (load_is_value) { /* we got no load - this is just the complex value */
            sge_strlcpy(lc_diagnosis2, MSG_SCHEDD_LCDIAGNOLOAD, STR_LC_DIAGNOSIS);
         } else if (((hlep && lc_host) || lc_global) &&
            (job_load = lGetElemStr(load_adjustments, CE_name, name))) { /* load correction */
            const char *load_correction_str;
            double load_correction;

            load_correction_str = lGetString(job_load, CE_stringval);
            if (!parse_ulong_val(&load_correction, NULL, type, load_correction_str, NULL, 0)) {
               if (reason)
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDLOADADJUST_SS, name, load_correction_str);
               DRETURN(1);
            }

            if (hlep) {
               int nproc;
               load_correction *= lc_host;

               if ((nproc = load_np_value_adjustment(name, hep,  &load_correction)) > 0) {
                  sprintf(lc_diagnosis1, MSG_SCHEDD_LCDIAGHOSTNP_SFI,
                         load_correction_str, lc_host, nproc);
               }
               else {
                  sprintf(lc_diagnosis1, MSG_SCHEDD_LCDIAGHOST_SF,
                         load_correction_str, lc_host);
               
               }
            
            } 
            else {
               load_correction *= lc_global;
               sprintf(lc_diagnosis1, MSG_SCHEDD_LCDIAGGLOBAL_SF,
                         load_correction_str, lc_global);
            }
            /* it depends on relop in complex config
            whether load_correction is pos/neg */
            switch (relop) {
            case CMPLXGE_OP:
            case CMPLXGT_OP:
               load += load_correction;
               sprintf(lc_diagnosis2, MSG_SCHEDD_LCDIAGPOSITIVE_SS, load_value, lc_diagnosis1);
               break;

            case CMPLXNE_OP:
            case CMPLXEQ_OP:
            case CMPLXLT_OP:
            case CMPLXLE_OP:
            default:
               load -= load_correction;
               sprintf(lc_diagnosis2, MSG_SCHEDD_LCDIAGNEGATIVE_SS, load_value, lc_diagnosis1);
               break;
            }
         } else  {
            sge_strlcpy(lc_diagnosis2, MSG_SCHEDD_LCDIAGNONE, STR_LC_DIAGNOSIS);
         }   

         /* is threshold exceeded ? */
         if (resource_cmp(relop, load, limit)) {
            if (reason) {
               if (type == TYPE_BOO){
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDBOOLVALUE_SSSSS,
                        name, load?MSG_TRUE:MSG_FALSE, lc_diagnosis2, map_op2str(relop), limit_value);
               }         
               else {
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDFLOATVALUE_SFSSS,
                        name, load, lc_diagnosis2, map_op2str(relop), limit_value);
               }         
            }
            DRETURN(1);
         }
         break;

      case TYPE_STR:
      case TYPE_CSTR:
      case TYPE_HOST:
      case TYPE_RESTR:
         match = string_base_cmp(type, limit_value, load_value);
         if (!match) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDSTRINGVALUE_SSSS, name, load_value, map_op2str(relop), limit_value);
            DRETURN(1);
         }
         break;
      default:
         if (reason)
            sprintf(reason, MSG_SCHEDD_WHYEXCEEDCOMPLEXTYPE_S, name);
         DRETURN(1);
   }

   DRETURN(0);
}

/****** sge_select_queue/load_np_value_adjustment() ****************************
*  NAME
*     load_np_value_adjustment() -- adjusts np load values for the number of processors
*
*  SYNOPSIS
*     static int load_np_value_adjustment(const char* name, lListElem *hep, 
*     double *load_correction) 
*
*  FUNCTION
*     Tests the load value name for "np_*". If this pattern is found, it will
*     retrieve the number of processors and adjusts the load_correction accordingly.
*     If the pattern is not found, it does nothing and returns 0 for number of processors.
*
*  INPUTS
*     const char* name        - load value name
*     lListElem *hep          - host object 
*     double *load_correction - current load_correction for further corrections
*
*  RESULT
*     static int - number of processors, or 0 if it was called on a none np load value 
*
*  NOTES
*     MT-NOTE: load_np_value_adjustment() is MT safe 
*
*******************************************************************************/
static int load_np_value_adjustment(const char* name, lListElem *hep, double *load_correction) {
 
   int nproc = 1;
   if (!strncmp(name, "np_", 3)) {
      int nproc = 1;
      lListElem *ep_nproc;

      if ((ep_nproc = lGetSubStr(hep, HL_name, LOAD_ATTR_NUM_PROC, EH_load_list))) {
         const char* cp = lGetString(ep_nproc, HL_value);
         if (cp) {
            nproc = atoi(cp);
      
            if (nproc > 1) {
               *load_correction /= nproc;
            }
         }
      }
   } 
   else {
      nproc = 0;          
   }
  
   return nproc;
}

static int resource_cmp(u_long32 relop, double req, double src_dl) 
{
   int match;

   switch(relop) { 
   case CMPLXEQ_OP :
      match = ( req==src_dl);
      break;
   case CMPLXLE_OP :
      match = ( req<=src_dl);
      break;
   case CMPLXLT_OP :
      match = ( req<src_dl);
      break;
   case CMPLXGT_OP :
      match = ( req>src_dl);
      break;
   case CMPLXGE_OP :
      match = ( req>=src_dl);
      break;
   case CMPLXNE_OP :
      match = ( req!=src_dl);
      break;
   default:
      match = 0; /* default -> no match */
   }

   return match;      
}

/* ----------------------------------------

   sge_load_alarm() 

   checks given threshold of the queue;
   centry_list and exechost_list get used
   therefore

   returns boolean:
      1 yes, the threshold is exceeded
      0 no
*/

int 
sge_load_alarm(char *reason, lListElem *qep, lList *threshold, 
               const lList *exechost_list, const lList *centry_list, 
               const lList *load_adjustments, bool is_check_consumable) 
{
   lListElem *hep, *global_hep, *tep;
   u_long32 ulc_factor; 
   const char *load_value = NULL; 
   const char *limit_value = NULL;
   double lc_host = 0, lc_global = 0;
   int load_is_value = 0;
   
   DENTER(TOP_LAYER, "sge_load_alarm");

   if (!threshold) { 
      /* no threshold -> no alarm */
      DRETURN(0);
   }

   hep = host_list_locate(exechost_list, lGetHost(qep, QU_qhostname));

   if(!hep) { 
      if (reason)
         sprintf(reason, MSG_SCHEDD_WHYEXCEEDNOHOST_S, lGetHost(qep, QU_qhostname));
      /* no host for queue -> ERROR */
      DRETURN(1);
   }

   if ((lGetPosViaElem(hep, EH_load_correction_factor, SGE_NO_ABORT) >= 0)
       && (ulc_factor=lGetUlong(hep, EH_load_correction_factor))) {
      lc_host = ((double)ulc_factor)/100;
   }   

   if ((global_hep = host_list_locate(exechost_list, SGE_GLOBAL_NAME)) != NULL) {
      if ((lGetPosViaElem(global_hep, EH_load_correction_factor, SGE_NO_ABORT) >= 0)
          && (ulc_factor=lGetUlong(global_hep, EH_load_correction_factor)))
         lc_global = ((double)ulc_factor)/100;
   }

   for_each (tep, threshold) {
      lListElem *hlep = NULL, *glep = NULL, *queue_ep = NULL, *cep  = NULL;
      bool need_free_cep = false;
      const char *name;
      u_long32 relop, type;

      name = lGetString(tep, CE_name);
      /* complex attriute definition */

      if (!(cep = centry_list_locate(centry_list, name))) { 
         if (reason)
            sprintf(reason, MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S, name);
         DRETURN(1);
      }
      if (!is_check_consumable && lGetBool(cep, CE_consumable)) { 
         continue;
      }

      if (hep != NULL) {
         hlep = lGetSubStr(hep, HL_name, name, EH_load_list);
      }   

      if (!lGetBool(cep, CE_consumable)) { 
         if (hlep != NULL) {
            load_value = lGetString(hlep, HL_value);
            load_is_value = 0;
         }
         else if ((global_hep != NULL) &&
                  ((glep = lGetSubStr(global_hep, HL_name, name, EH_load_list)) != NULL)) {
               load_value = lGetString(glep, HL_value);
               load_is_value = 0;
         } 
         else {
            queue_ep = lGetSubStr(qep, CE_name, name, QU_consumable_config_list);
            if (queue_ep != NULL) {
               load_value = lGetString(queue_ep, CE_stringval);
               load_is_value = 1;
            } else { 
               if (reason) {
                  sprintf(reason, MSG_SCHEDD_NOVALUEFORATTR_S, name);
               }
               DRETURN(1);
            }
         }      
      }
      else {
         /* load thesholds... */
         if ((cep = get_attribute_by_name(global_hep, hep, qep, name, centry_list, DISPATCH_TIME_NOW, 0)) == NULL ) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S, name);
            DRETURN(1);
         }
         need_free_cep = true;
     
         load_value = lGetString(cep, CE_pj_stringval);
         load_is_value = (lGetUlong(cep, CE_pj_dominant) & DOMINANT_TYPE_MASK) != DOMINANT_TYPE_CLOAD; 
      }

      relop = lGetUlong(cep, CE_relop);
      limit_value = lGetString(tep, CE_stringval);
      type = lGetUlong(cep, CE_valtype);

      if(load_check_alarm(reason, name, load_value, limit_value, relop, 
                              type, hep, hlep, lc_host, lc_global, 
                              load_adjustments, load_is_value)) {
         if (need_free_cep) {
            lFreeElem(&cep);
         }
         DRETURN(1);
      }   
      if (need_free_cep) {
         lFreeElem(&cep);
      }
   } 

   DRETURN(0);
}

/* ----------------------------------------

   sge_load_alarm_reasons() 

   checks given threshold of the queue;
   centry_list and exechost_list get used
   therefore

   fills and returns string buffer containing reasons for alarm states
*/

char *sge_load_alarm_reason(lListElem *qep, lList *threshold, 
                            const lList *exechost_list, const lList *centry_list, 
                            char *reason, int reason_size, 
                            const char *threshold_type) 
{
   DENTER(TOP_LAYER, "sge_load_alarm_reason");

   *reason = 0;

   /* no threshold -> no alarm */
   if (threshold != NULL) {
      lList *rlp = NULL;
      lListElem *tep;
      bool first = true;

      /* get actual complex values for queue */
      queue_complexes2scheduler(&rlp, qep, exechost_list, centry_list);

      /* check all thresholds */
      for_each (tep, threshold) {
         const char *name;             /* complex attrib name */
         lListElem *cep;               /* actual complex attribute */
         char dom_str[5];              /* dominance as string */
         u_long32 dom_val;             /* dominance as u_long */
         char buffer[MAX_STRING_SIZE]; /* buffer for one line */
         const char *load_value;       /* actual load value */
         const char *limit_value;      /* limit defined by threshold */

         name = lGetString(tep, CE_name);

         if ( first == true ) {
            first = false;
         } else {
            strncat(reason, "\n\t", reason_size);
         }

         /* find actual complex attribute */
         if ((cep = lGetElemStr(rlp, CE_name, name)) == NULL) {
            /* no complex attribute for threshold -> ERROR */
            if (qinstance_state_is_unknown(qep)) {
               snprintf(buffer, MAX_STRING_SIZE, MSG_QINSTANCE_VALUEMISSINGMASTERDOWN_S, name);
            } else {
               snprintf(buffer, MAX_STRING_SIZE, MSG_SCHEDD_NOCOMPLEXATTRIBUTEFORTHRESHOLD_S, name);
            }
            strncat(reason, buffer, reason_size);
            continue;
         }

         limit_value = lGetString(tep, CE_stringval);

         if (!(lGetUlong(cep, CE_pj_dominant) & DOMINANT_TYPE_VALUE)) {
            dom_val = lGetUlong(cep, CE_pj_dominant);
            load_value = lGetString(cep, CE_pj_stringval);
         } else {
            dom_val = lGetUlong(cep, CE_dominant);
            load_value = lGetString(cep, CE_stringval);
         }

         monitor_dominance(dom_str, dom_val);

         snprintf(buffer, MAX_STRING_SIZE, "alarm %s:%s=%s %s-threshold=%s",
                 dom_str,
                 name, 
                 load_value,
                 threshold_type,
                 limit_value
                );

         strncat(reason, buffer, reason_size);
      }

      lFreeList(&rlp);
   }   

   DRETURN(reason);
}

/* ----------------------------------------

   sge_split_queue_load()

   splits the incoming queue list (1st arg) into an unloaded and
   overloaded (2nd arg) list according to the load values contained in
   the execution host list (3rd arg) and with respect to the definitions
   in the complex list (4th arg).

   temporarily sets QU_tagged4schedule but sets it to 0 on exit.

   returns:
      0 successful
     -1 errors in functions called by sge_split_queue_load

*/
int sge_split_queue_load(
lList **unloaded,               /* QU_Type */
lList **overloaded,             /* QU_Type */
lList *exechost_list,           /* EH_Type */
lList *centry_list,             /* CE_Type */
const lList *load_adjustments,  /* CE_Type */
lList *granted,                 /* JG_Type */
bool  is_consumable_load_alarm, /* is true, when the consumable evaluation 
                                   set a load alarm */
bool is_comprehensive,          /* do the load evaluation comprehensive (include consumables) */
u_long32 ttype
) {
   lList *thresholds;
   lCondition *where;
   lListElem *qep;
   int ret, load_alarm, nverified = 0;
   char reason[2048];

   DENTER(TOP_LAYER, "sge_split_queue_load");

   /* a job has been dispatched recently,
      but load correction is not in use at all */
   if (granted && !load_adjustments && !is_consumable_load_alarm) {
      DRETURN(0);
   }

   if (!(granted && !load_adjustments)) { 

   /* tag those queues being overloaded */
      for_each(qep, *unloaded) {
         thresholds = lGetList(qep, ttype);
         load_alarm = 0;

         /* do not verify load alarm anew if a job has been dispatched recently
            but not to the host where this queue resides */
         if (!granted || (granted && (sconf_get_global_load_correction() ||
                              lGetElemHost(granted, JG_qhostname, lGetHost(qep, QU_qhostname))))) {
            nverified++;

            if (sge_load_alarm(reason, qep, thresholds, exechost_list, centry_list, load_adjustments, is_comprehensive) != 0) {
               load_alarm = 1;
               if (ttype==QU_suspend_thresholds) {
                  DPRINTF(("queue %s tagged to be in suspend alarm: %s\n", 
                        lGetString(qep, QU_full_name), reason));
                  schedd_mes_add_global(SCHEDD_INFO_QUEUEINALARM_SS, lGetString(qep, QU_full_name), reason);
               } else {
                  DPRINTF(("queue %s tagged to be overloaded: %s\n", 
                        lGetString(qep, QU_full_name), reason));
                  schedd_mes_add_global(SCHEDD_INFO_QUEUEOVERLOADED_SS, lGetString(qep, QU_full_name), reason);
               }
            }
         }
         if (load_alarm) {
            lSetUlong(qep, QU_tagged4schedule, load_alarm);
         }   
      }
   }

   DPRINTF(("verified threshold of %d queues\n", nverified));

   /* split queues in unloaded and overloaded lists */
   where = lWhere("%T(%I == %u)", lGetListDescr(*unloaded), QU_tagged4schedule, 0);
   ret = lSplit(unloaded, overloaded, "overloaded queues", where);
   lFreeWhere(&where);

   if (overloaded) {
      for_each(qep, *overloaded) { /* make sure QU_tagged4schedule is 0 on exit */
         lSetUlong(qep, QU_tagged4schedule, 0);
      }
   }

   if (ret) {
      DRETURN(-1);
   }

   DRETURN(0);
}


/****** sge_select_queue/sge_split_queue_slots_free() **************************
*  NAME
*     sge_split_queue_slots_free() -- ??? 
*
*  SYNOPSIS
*     int sge_split_queue_slots_free(lList **free, lList **full) 
*
*  FUNCTION
*     Split queue list into queues with at least one slots and queues with 
*     less than one free slot. The list optioally returned in full gets the
*     QNOSLOTS queue instance state set.
*
*  INPUTS
*     lList **free - Input queue instance list and return free slots.
*     lList **full - If non-NULL the full queue instances get returned here.
*
*
*  TODO: take a look into list hashing, not needed for temp list
*
*  RESULT
*     int - 0 success 
*          -1 error
*******************************************************************************/
int sge_split_queue_slots_free(lList **free, lList **full) 
{
   lList *full_queues = NULL;
   lListElem *this = NULL;
   lListElem *next = NULL;

   DENTER(TOP_LAYER, "sge_split_queue_nslots_free");

   if (free == NULL) {
      DRETURN(-1);
   }

   for (this=lFirst(*free); ((next=lNext(this))), this ; this = next) {
      if (qinstance_slots_used(this) >= (int) lGetUlong(this, QU_job_slots)) {
          
         this = lDechainElem(*free, this);        
          
         if (!qinstance_state_is_full(this)) {
            schedd_mes_add_global(SCHEDD_INFO_QUEUEFULL_, lGetString(this, QU_full_name));
            qinstance_state_set_full(this, true);

            if (full_queues == NULL) { 
               full_queues = lCreateListHash("full one", lGetListDescr(*free), false);
            }
            lAppendElem(full_queues, this);             
         }   
         else if (full != NULL) {
            if (*full == NULL) {
               *full = lCreateList("full one", lGetListDescr(*free));
            }   
            lAppendElem(*full, this);
         } 
         else {
            lFreeElem(&this);
         }   
      }
   }

   /* dump out the -tsm log and add the new queues to the disabled queue list */
   if (full_queues) {
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESFULLANDDROPPED , full_queues, QU_full_name);
      if (full != NULL) {
         if (*full == NULL) {
            *full = full_queues;
            full_queues = NULL;
         }
         else {
            lAddList(*full, &full_queues);
            full_queues = NULL;
         }
      }
      else {
         lFreeList(&full_queues);
      }
   }

   DRETURN(0);
}


/* ----------------------------------------

   sge_split_suspended()

   splits the incoming queue list (1st arg) into non suspended queues and
   suspended queues (2nd arg) 

   returns:
      0 successful
     -1 error

*/
int sge_split_suspended(
lList **queue_list,        /* QU_Type */
lList **suspended         /* QU_Type */
) {
   lCondition *where;
   int ret;
   lList *lp = NULL;

   DENTER(TOP_LAYER, "sge_split_suspended");

   if (!queue_list) {
      DRETURN(-1);
   }

   if (!suspended) {
       suspended = &lp;
   }

   /* split queues */
   where = lWhere("%T(!(%I m= %u) && !(%I m= %u) && !(%I m= %u) && !(%I m= %u))", 
      lGetListDescr(*queue_list), 
         QU_state, QI_SUSPENDED,
         QU_state, QI_CAL_SUSPENDED,
         QU_state, QI_CAL_DISABLED,
         QU_state, QI_SUSPENDED_ON_SUBORDINATE);

   ret = lSplit(queue_list, &lp, "full queues", where);
   lFreeWhere(&where);

   if (lp != NULL) {
      lListElem* mes_queue;

      for_each(mes_queue, lp) {
         if (!qinstance_state_is_manual_suspended(mes_queue)) {
            qinstance_state_set_manual_suspended(mes_queue, true);
            schedd_mes_add_global(SCHEDD_INFO_QUEUESUSP_, lGetString(mes_queue, QU_full_name));
         }
      }   
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESSUSPENDEDANDDROPPED , lp, QU_full_name);
   }

   if (suspended != NULL) {
      if (*suspended == NULL) {
         *suspended = lp;
         lp = NULL;
      }
      else {
         lAddList(*suspended, &lp);
         lp = NULL;
      }
   }
   else {
      lFreeList(suspended);
   }
      
   DRETURN(ret);
}

/* ----------------------------------------

   sge_split_cal_disabled()

   splits the incoming queue list (1st arg) into non disabled queues and
   cal_disabled queues (2nd arg) 

   lList **queue_list,       QU_Type 
   lList **disabled          QU_Type 

   returns:
      0 successful
     -1 errors in functions called by sge_split_queue_load

*/
int 
sge_split_cal_disabled(lList **queue_list, lList **disabled) 
{
   lCondition *where;
   int ret;
   lList *lp = NULL;
   bool do_free_list = false;

   DENTER(TOP_LAYER, "sge_split_disabled");

   if (!queue_list) {
      DRETURN(-1);
   }

   if (disabled == NULL) {
       disabled = &lp;
       do_free_list = true;
   }

   /* split queues */
   where = lWhere("%T(!(%I m= %u))", lGetListDescr(*queue_list), 
                  QU_state, QI_CAL_DISABLED);
   ret = lSplit(queue_list, disabled, "full queues", where);
   lFreeWhere(&where);

   if (*disabled != NULL) {
      lListElem* mes_queue;

      for_each(mes_queue, *disabled) {
         schedd_mes_add_global(SCHEDD_INFO_QUEUEDISABLED_, lGetString(mes_queue, QU_full_name));
      }   
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED , *disabled, QU_full_name);

      if (do_free_list) {
         lFreeList(disabled);
      }
   }
   
   DRETURN(ret);
}

/* ----------------------------------------

   sge_split_disabled()

   splits the incoming queue list (1st arg) into non disabled queues and
   disabled queues (2nd arg) 

   lList **queue_list,       QU_Type 
   lList **disabled          QU_Type 

   returns:
      0 successful
     -1 errors in functions called by sge_split_queue_load

*/
int 
sge_split_disabled(lList **queue_list, lList **disabled) 
{
   lCondition *where;
   int ret;
   lList *lp = NULL;
   bool do_free_list = false;

   DENTER(TOP_LAYER, "sge_split_disabled");

   if (!queue_list) {
      DRETURN(-1);
   }

   if (disabled == NULL) {
       disabled = &lp;
       do_free_list = true;
   }

   /* split queues */
   where = lWhere("%T(!(%I m= %u) && !(%I m= %u))", lGetListDescr(*queue_list), 
                  QU_state, QI_DISABLED, QU_state, QI_CAL_DISABLED);
   ret = lSplit(queue_list, disabled, "full queues", where);
   lFreeWhere(&where);

   if (*disabled != NULL) {
      lListElem* mes_queue;

      for_each(mes_queue, *disabled) {
         schedd_mes_add_global(SCHEDD_INFO_QUEUEDISABLED_, lGetString(mes_queue, QU_full_name));
      }   
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED , *disabled, QU_full_name);

      if (do_free_list) {
         lFreeList(disabled);
      }
   }
   
   DRETURN(ret);
}

/****** sge_select_queue/pe_cq_rejected() **************************************
*  NAME
*     pe_cq_rejected() -- Check, if -pe pe_name rejects cluster queue
*
*  SYNOPSIS
*     static bool pe_cq_rejected(const char *pe_name, const lListElem *cq)
*
*  FUNCTION
*     Match a jobs -pe 'pe_name' with pe_list cluster queue configuration.
*     True is returned if the parallel environment has no access.
*
*  INPUTS
*     const char *project - the pe request of a job (no wildcard)
*     const lListElem *cq - cluster queue (CQ_Type)
*
*  RESULT
*     static bool - True, if rejected
*
*  NOTES
*     MT-NOTE: pe_cq_rejected() is MT safe
*******************************************************************************/
static bool pe_cq_rejected(const char *pe_name, const lListElem *cq)
{
   const lListElem *alist;
   bool rejected;

   DENTER(TOP_LAYER, "pe_cq_rejected");

   if (!pe_name) {
      DRETURN(false);
   }

   rejected = true;
   for_each (alist, lGetList(cq, CQ_pe_list)) {
      if (lGetSubStr(alist, ST_name, pe_name, ASTRLIST_value)) {
         rejected = false;
         break;
      }
   }

   DRETURN(rejected);
}

/****** sge_select_queue/project_cq_rejected() *********************************
*  NAME
*     project_cq_rejected() -- Check, if -P project rejects cluster queue
*
*  SYNOPSIS
*     static bool project_cq_rejected(const char *project, const lListElem *cq)
*
*  FUNCTION
*     Match a jobs -P 'project' with project/xproject cluster queue configuration.
*     True is returned if the project has no access.
*
*  INPUTS
*     const char *project - the project of a job or NULL
*     const lListElem *cq - cluster queue (CQ_Type)
*
*  RESULT
*     static bool - True, if rejected
*
*  NOTES
*     MT-NOTE: project_cq_rejected() is MT safe
*******************************************************************************/
static bool project_cq_rejected(const char *project, const lListElem *cq)
{
   const lList *projects;
   const lListElem *alist;
   bool rejected;

   DENTER(TOP_LAYER, "project_cq_rejected");

   if (!project) {
      /* without project: rejected, if each "project" profile
         does contain project references */
      for_each (alist, lGetList(cq, CQ_projects)) {
         if (!lGetList(alist, APRJLIST_value)) {
            DRETURN(false);
         }
      }

      DRETURN(true);
   }

   /* with project: rejected, if project is exluded by each "xproject" profile */
   rejected = true;
   for_each (alist, lGetList(cq, CQ_xprojects)) {
      projects = lGetList(alist, APRJLIST_value);
      if (!projects || !userprj_list_locate(projects, project)) {
         rejected = false;
         break;
      }
   }
   if (rejected) {
      DRETURN(true);
   }

   /* with project: rejected, if project is not included with each "project" profile */
   rejected = true;
   for_each (alist, lGetList(cq, CQ_projects)) {
      projects = lGetList(alist, APRJLIST_value);
      if (!projects || userprj_list_locate(projects, project)) {
         rejected = false;
         break;
      }
   }
   if (rejected) {
      DRETURN(true);
   }

   DRETURN(false);
}

/****** sge_select_queue/interactive_cq_rejected() *****************************
*  NAME
*     interactive_cq_rejected() --  Check, if -now yes rejects cluster queue
*
*  SYNOPSIS
*     static bool interactive_cq_rejected(const lListElem *cq)
*
*  FUNCTION
*     Returns true if -now yes jobs can not be run in cluster queue
*
*  INPUTS
*     const lListElem *cq - cluster queue (CQ_Type)
*
*  RESULT
*     static bool - True, if rejected
*
*  NOTES
*     MT-NOTE: interactive_cq_rejected() is MT safe
*******************************************************************************/
static bool interactive_cq_rejected(const lListElem *cq)
{
   const lListElem *alist;
   bool rejected;

   DENTER(TOP_LAYER, "interactive_cq_rejected");

   rejected = true;
   for_each (alist, lGetList(cq, CQ_qtype)) {
      if ((lGetUlong(alist, AQTLIST_value) & IQ)) {
         rejected = false;
         break;
      }
   }

   DRETURN(rejected);
}


/****** sge_select_queue/cqueue_match_static() *********************************
*  NAME
*     cqueue_match_static() -- Does cluster queue match the job?
*
*  SYNOPSIS
*     static dispatch_t cqueue_match_static(const char *cqname,
*     sge_assignment_t *a)
*
*  FUNCTION
*     The function tries to find reasons (-q, -l and -P) why the
*     entire cluster is not suited for the job.
*
*  INPUTS
*     const char *cqname  - Cluster queue name
*     sge_assignment_t *a - ???
*
*  RESULT
*     static dispatch_t - Returns DISPATCH_OK  or DISPATCH_NEVER_CAT
*
*  NOTES
*     MT-NOTE: cqueue_match_static() is MT safe
*******************************************************************************/
static dispatch_t cqueue_match_static(const char *cqname, sge_assignment_t *a)
{
   const lList *hard_resource_list;
   const lListElem *cq;
   const char *project, *pe_name;

   DENTER(TOP_LAYER, "cqueue_match_static");

   /* detect if entire cluster queue ruled out due to -q */
   if (qref_list_cq_rejected(lGetList(a->job, JB_hard_queue_list), cqname, NULL, NULL)) {
      DPRINTF(("Cluster Queue \"%s\" is not contained in the hard queue list (-q) that "
            "was requested by job %d\n", cqname, (int)a->job_id));
      schedd_mes_add(a->job_id, SCHEDD_INFO_NOTINHARDQUEUELST_S, cqname);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   cq = lGetElemStr(*(object_type_get_master_list(SGE_TYPE_CQUEUE)), CQ_name, cqname);

   /* detect if entire cluster queue ruled out due to -l */
   if ((hard_resource_list = lGetList(a->job, JB_hard_resource_list))) {
      dstring unsatisfied = DSTRING_INIT;
      if (request_cq_rejected(hard_resource_list, cq, a->centry_list, &unsatisfied)) {
         DPRINTF(("Cluster Queue \"%s\" can not fulfill resource request (-l %s) that "
               "was requested by job %d\n", cqname, sge_dstring_get_string(&unsatisfied), (int)a->job_id));
         schedd_mes_add(a->job_id, SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, sge_dstring_get_string(&unsatisfied),
               cqname, "of cluster queue");
         sge_dstring_free(&unsatisfied);
         DRETURN(DISPATCH_NEVER_CAT);
      }
   }

   /* detect if entire cluster queue ruled out due to -P */
   project = lGetString(a->job, JB_project);
   if (project_cq_rejected(project, cq)) {
      DPRINTF(("Cluster queue \"%s\" does not work for -P %s job %d\n",
         cqname, project?project:"<no project>", (int)a->job_id));
      schedd_mes_add(a->job_id, SCHEDD_INFO_HASNOPRJ_S, "cluster queue", cqname);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* detect if entire cluster queue ruled out due to -pe */
   if (a->pe && (pe_name=lGetString(a->pe, PE_name)) &&
         pe_cq_rejected(pe_name, cq)) {
      DPRINTF(("Cluster queue "SFQ" does not reference PE "SFQ"\n", cqname, pe_name));
      schedd_mes_add(a->job_id, SCHEDD_INFO_NOTINQUEUELSTOFPE_SS, cqname, pe_name);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   /* detect if entire cluster queue ruled out due to -I y aka -now yes */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(a->job, JB_type)) && interactive_cq_rejected(cq)) {
      DPRINTF(("Queue \"%s\" is not an interactive queue as requested by job %d\n",
               cqname, (int)a->job_id));
      schedd_mes_add(a->job_id, SCHEDD_INFO_QUEUENOTINTERACTIVE_S, cqname);
      DRETURN(DISPATCH_NEVER_CAT);
   }

   DRETURN(DISPATCH_OK);
}


/****** sge_select_queue/sequential_tag_queues_suitable4job() **************
*  NAME
*     sequential_tag_queues_suitable4job() -- ??? 
*
*  SYNOPSIS
*
*  FUNCTION
*     The start time of a queue is always returned using the QU_available_at 
*     field.
*
*     The overall behaviour of this function is somewhat dependent on the 
*     value that gets passed to assignment->start and whether soft requests 
*     were specified with the job: 
*
*     (1) In case of now assignemnts (DISPATCH_TIME_NOW) only the first queue 
*         suitable for jobs without soft requests is tagged. When soft requests 
*         are specified all queues must be verified and tagged in order to find 
*         the queue that fits best. 
*
*     (2) In case of reservation assignments (DISPATCH_TIME_QUEUE_END) the earliest
*         time is searched when the resources of global/host/queue are sufficient
*         for the job. The time-wise iteration is then done for each single resources 
*         instance.
*
*  INPUTS
*     sge_assignment_t *assignment - job info structure
*
*  RESULT
*     dispatch_t - 0 ok got an assignment 
*                    start time(s) and slots are tagged
*                  1 no assignment at the specified time
*                 -1 assignment will never be possible for all jobs of that category
*                 -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sequential_tag_queues_suitable4job() is not MT safe 
*******************************************************************************/
static dispatch_t
sequential_tag_queues_suitable4job(sge_assignment_t *a)
{
   lList *skip_host_list = NULL;
   lList *skip_queue_list = NULL;
   bool soft_requests = job_has_soft_requests(a->job);

   lList *skip_cqueue_list = NULL;
   lList *unclear_cqueue_list = NULL;

   category_use_t use_category;
   
   dispatch_t result;
   u_long32 tt_global = a->start;
   dispatch_t best_queue_result = DISPATCH_NEVER_CAT;
   int global_violations = 0, queue_violations;
   lListElem *qep;

   DENTER(TOP_LAYER, "sequential_tag_queues_suitable4job");

   /* assemble job category information */
   fill_category_use_t(a, &use_category, "NONE");   
   
   /* restore job messages from previous dispatch runs of jobs of the same category */
   if (use_category.use_category) {
      schedd_mes_set_tmp_list(use_category.cache, CCT_job_messages, a->job_id);
      skip_host_list = lGetList(use_category.cache, CCT_ignore_hosts);
      skip_queue_list = lGetList(use_category.cache, CCT_ignore_queues);
   }

   result = sequential_global_time(&tt_global, a, (use_category.compute_violation?&global_violations:NULL)); 
   if (result != DISPATCH_OK && result != DISPATCH_MISSING_ATTR) {
      DRETURN(result);
   }
   
   if (use_category.use_cviolation) {
      for_each(qep, a->queue_list) { 
         lListElem *queue_violation = lGetElemStr(lGetList(use_category.cache, CCT_queue_violations), 
                                                  CTQV_name, lGetString(qep, QU_full_name)); 
         if (queue_violation){
            lSetUlong(qep, QU_soft_violation, lGetUlong(queue_violation, CTQV_count));
         }
      }
      
      sort_queue_list(a->queue_list);
   }

   for_each(qep, a->queue_list) {   
      u_long32 tt_host = a->start;
      u_long32 tt_queue = a->start;   
      const char *eh_name;
      const char *qname, *cqname;

      lListElem *hep;

      /* Ignore not tagged cluster queues because they are already denied by limitation rules */
      /* Tagged cqueues need to be resetted because they will maybe newly tagged by this function */
      if (lGetUlong(qep, QU_tag) == 0) {
         continue;
      } else {
         lSetUlong(qep, QU_tag, 0);
      }

      qname = lGetString(qep, QU_full_name);
      cqname = lGetString(qep, QU_qname);

      /* try to foreclose the cluster queue */
      if (lGetElemStr(skip_cqueue_list, CTI_name, cqname)) {
         DPRINTF(("skip cluster queue %s\n", cqname));
         continue;
      }
      if (!lGetElemStr(unclear_cqueue_list, CTI_name, cqname)) {
         if (cqueue_match_static(cqname, a) != DISPATCH_OK) {
            lAddElemStr(&skip_cqueue_list, CTI_name, cqname, CTI_Type);
            best_queue_result = find_best_result(DISPATCH_NEVER_CAT, best_queue_result);
            continue;
         }
         lAddElemStr(&unclear_cqueue_list, CTI_name, cqname, CTI_Type);
      } else
         DPRINTF(("checked cluster queue %s already\n", cqname));

      if (skip_queue_list && lGetElemStr(skip_queue_list, CTI_name, qname)){
         DPRINTF(("job category skip queue %s\n", qname));
         continue;
      }

      eh_name = lGetHost(qep, QU_qhostname);
      hep = lGetElemHost(a->host_list, EH_name, eh_name);

      if (hep != NULL) {

         if (skip_host_list && lGetElemStr(skip_host_list, CTI_name, eh_name)){
            DPRINTF(("job category skip host %s\n", eh_name));          
            continue;
         }

         /* match the none resources */
         if (sge_queue_match_static(qep, a->job, NULL, a->ckpt, a->centry_list, a->acl_list, a->hgrp_list) != DISPATCH_OK) {
            if (skip_queue_list)
               lAddElemStr(&skip_queue_list, CTI_name, qname, CTI_Type);
            best_queue_result = find_best_result(DISPATCH_NEVER_CAT, best_queue_result);
            continue;
         }
         
         queue_violations = global_violations;

         result = sequential_host_time(&tt_host, a, use_category.compute_violation?&queue_violations:NULL, 
                                     hep);

         if (result != DISPATCH_OK && result != DISPATCH_MISSING_ATTR) {

            if (skip_host_list && result != DISPATCH_NEVER_JOB) { 
               lAddElemStr(&skip_host_list, CTI_name, eh_name, CTI_Type);
            }

            DPRINTF(("host %s returned %d\n", eh_name, result));         
            /* right now there is no use in continuing with that host but we 
               don't wanna loose an opportunity for a reservation */
            best_queue_result = find_best_result(result, best_queue_result); 
            continue;
         }

         result = sequential_queue_time(&tt_queue, a, use_category.compute_violation?&queue_violations:NULL, 
                                      qep);

         /* the soft request violations can be cached in the categories. If they have been computed for a previous job
            in the same category, they are cached and we can used reuse the same values. */

         if (use_category.compute_violation) { /* storing the new computed values. */
            lListElem *queue_violation = lAddSubStr (use_category.cache, CTQV_name, qname, CCT_queue_violations, CTQV_Type);
            lSetUlong(queue_violation, CTQV_count, queue_violations); 
         }

         if (result == DISPATCH_OK) {
            lSetUlong(qep, QU_tag, 1); /* tag number of slots per queue and time when it will be available */
            
            if (a->start == DISPATCH_TIME_QUEUE_END) {
               DPRINTF(("    global "sge_u32" host "sge_u32" queue "sge_u32"\n", tt_global, tt_host, tt_queue));
               tt_queue = MAX(lGetUlong(qep, QU_available_at), MAX(tt_queue, MAX(tt_host, tt_global)));
               lSetUlong(qep, QU_available_at, tt_queue);
            }
            DPRINTF(("    set Q: %s number="sge_u32" when="sge_u32" violations="sge_u32"\n", qname,
                   lGetUlong(qep, QU_tag), lGetUlong(qep, QU_available_at), lGetUlong(qep, QU_soft_violation)));
            best_queue_result = DISPATCH_OK;

            if (!a->is_reservation) {
               if (!soft_requests || use_category.use_cviolation) {
                  break;
               }
            }
         } 
         else {
            DPRINTF(("queue %s reported %d\n", qname, result));
            if (skip_queue_list) {
               lAddElemStr(&skip_queue_list, CTI_name, qname, CTI_Type);
            }
            best_queue_result = find_best_result(result, best_queue_result); 
         }
      }
      else {
         ERROR((SGE_EVENT, MSG_SCHEDD_UNKNOWN_HOST_SS, qname, eh_name));
         if (skip_queue_list != NULL) {
            lAddElemStr(&skip_queue_list, CTI_name, qname, CTI_Type);
         }
      }
   }  

   lFreeList(&skip_cqueue_list);
   lFreeList(&unclear_cqueue_list);

   /* cache so far generated messages with the job category */
   if (use_category.use_category) {  
      lList *temp = schedd_mes_get_tmp_list();
      if (temp){    
         lSetList(use_category.cache, CCT_job_messages, lCopyList(NULL, temp));
      }
   }

   /* we calculated the violations for soft requests. Now we need to sort the queues
      to have the one with the least violations at the top of the list */
   if (soft_requests && !use_category.use_cviolation) {
      sort_queue_list(a->queue_list);
   }
   
   DRETURN(best_queue_result);
}

/****** sge_select_queue/sequential_tag_queues_suitable4job_by_rqs() **********
*  NAME
*     sequential_tag_queues_suitable4job_by_rqs() -- tag queues where no 
*                                                     limitation is reached
*
*  SYNOPSIS
*     static dispatch_t 
*     sequential_tag_queues_suitable4job_by_rqs(sge_assignment_t *a) 
*
*  FUNCTION
*     The function iteratates over all limitation rules sets for every
*     cluster queue. The queue is tagged if no limit is reached.
*
*  INPUTS
*     sge_assignment_t *a - job info structure
*
*  RESULT
*     static dispatch_t - DISPATCH_OK        got an assignment
*                       - DISPATCH_NEVER_CAT no assignment for all jobs af that category
*
*  NOTES
*     MT-NOTE: sequential_tag_queues_suitable4job_by_rqs() is not MT safe 
*
*  SEE ALSO
*     sge_select_queue/sequential_tag_queues_suitable4job()
*******************************************************************************/
static dispatch_t
sequential_tag_queues_suitable4job_by_rqs(sge_assignment_t *a)
{
   lListElem *queue_instance = NULL;
   lListElem *rqs = NULL;
   bool no_check = false;
   dstring rule_name = DSTRING_INIT;

   dispatch_t result = DISPATCH_NEVER_CAT;

   DENTER(TOP_LAYER, "sequential_tag_queues_suitable4job_by_rqs");

   if (a->rqs_list == NULL || lGetNumberOfElem(a->rqs_list) == 0) {
      no_check = true;
   }

   for_each(queue_instance, a->queue_list) {
      u_long32 tt_rqs_all = 0;

      if (no_check == false) {
         dispatch_t rqs_result = DISPATCH_OK;

         for_each(rqs, a->rqs_list) {
            const char *user = NULL;
            const char *project = NULL;
            const char *host_name = NULL;
            const char *queue_name = NULL;
            const lListElem *rule = NULL;
            u_long32 tt_rqs = a->start;

            if (!lGetBool(rqs, RQS_enabled)) {
               continue;
            }

            user = lGetString(a->job, JB_owner);
            project = lGetString(a->job, JB_project);
            host_name = lGetHost(queue_instance, QU_qhostname);
            queue_name = lGetString(queue_instance, QU_qname);

            sge_dstring_clear(&rule_name);
            rule = rqs_get_matching_rule(rqs, user, project, NULL, host_name, queue_name, a->acl_list, a->hgrp_list, &rule_name);
            if (rule != NULL) {
               /* Check booked usage */
               rqs_result = rqs_limitation_reached(a, rule, host_name, queue_name, &tt_rqs);

               if (rqs_result == DISPATCH_MISSING_ATTR) {
                  rqs_result = DISPATCH_OK;
                  continue;
               }
               
               if (rqs_result != DISPATCH_OK) {
                  schedd_mes_add(a->job_id, SCHEDD_INFO_CANNOTRUNRQS_SSS, queue_name, host_name, sge_dstring_get_string(&rule_name));
                  DPRINTF(("resource quota set %s deny job execution\n on %s@%s\n", sge_dstring_get_string(&rule_name), queue_name, host_name));
                  break;
               }
               tt_rqs_all = MAX(tt_rqs_all, tt_rqs);
            }
         }

         result = find_best_result(rqs_result, result);

         if (rqs == NULL) { /* all rule sets checked successfully */
            lSetUlong(queue_instance, QU_tag, 1);
            if (a->is_reservation && (tt_rqs_all != 0)) {
               lSetUlong(queue_instance, QU_available_at, tt_rqs_all);
            }
         } else {
            lSetUlong(queue_instance, QU_tag, 0);
         }
      } else {
         lSetUlong(queue_instance, QU_tag, 1);
         result = DISPATCH_OK;
      }
   }

   sge_dstring_free(&rule_name);
   DRETURN(result);
}

/****** sge_select_queue/rqs_limitation_reached() *****************************
*  NAME
*     rqs_limitation_reached() -- is the limitation reached for a queue instance
*
*  SYNOPSIS
*     static bool rqs_limitation_reached(sge_assignment_t *a, lListElem *rule, 
*     const char* host, const char* queue) 
*
*  FUNCTION
*     The function verifies no limitation is reached for the specific job request
*     and queue instance
*
*  INPUTS
*     sge_assignment_t *a    - job info structure
*     const lListElem *rule        - rqsource quota rule (RQR_Type)
*     const char* host       - host name
*     const char* queue      - queue name
*    u_long32 *start         - start time of job
*
*  RESULT
*     static dispatch_t - DISPATCH_OK job can be scheduled
*                         DISPATCH_NEVER_CAT no jobs of this category will be scheduled
*                         DISPATCH_NOT_AT_TIME job can be scheduled later
*                         DISPATCH_MISSING_ATTR rule does not match requested attributes
*
*  NOTES
*     MT-NOTE: rqs_limitation_reached() is not MT safe 
*
*******************************************************************************/
static dispatch_t rqs_limitation_reached(sge_assignment_t *a, const lListElem *rule, const char* host, const char* queue, u_long32 *start) {
   dispatch_t ret = DISPATCH_MISSING_ATTR;
   lList *limit_list = NULL;
   lListElem * limit = NULL;
   static lListElem *implicit_slots_request = NULL;
   lListElem *global_host = host_list_locate(a->host_list, SGE_GLOBAL_NAME);
   lListElem *exec_host = host_list_locate(a->host_list, host);
   dstring rue_name = DSTRING_INIT;
   dstring reason = DSTRING_INIT;

   DENTER(TOP_LAYER, "rqs_limitation_reached");

    if (implicit_slots_request == NULL) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, SGE_ATTR_SLOTS);
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   limit_list = lGetList(rule, RQR_limit);
   for_each(limit, limit_list) {
      
      const char *limit_name = lGetString(limit, RQRL_name);

      lListElem *raw_centry = centry_list_locate(a->centry_list, limit_name);
      bool is_forced = lGetUlong(raw_centry, CE_requestable) == REQU_FORCED ? true : false;
      lList *job_centry_list = lGetList(a->job, JB_hard_resource_list);
      lListElem *job_centry = centry_list_locate(job_centry_list, limit_name);

      if (raw_centry == NULL) {
         DPRINTF(("ignoring limit %s because not defined", limit_name));
         continue;
      } else {
         DPRINTF(("checking limit %s\n", lGetString(raw_centry, CE_name)));
      }

      /* check for implicit slot and default request */
      if (job_centry == NULL) {
         if (strcmp(lGetString(raw_centry, CE_name), "slots") == 0) {
            job_centry = implicit_slots_request;
         } else if (lGetString(raw_centry, CE_default) != NULL && lGetBool(raw_centry, CE_consumable)) {
            double request;
            parse_ulong_val(&request, NULL, lGetUlong(raw_centry, CE_valtype), lGetString(raw_centry, CE_default), NULL, 0);

            /* default requests with zero value are ignored */
            if (request == 0.0) {
               continue;
            }
            lSetString(raw_centry, CE_stringval, lGetString(raw_centry, CE_default));
            lSetDouble(raw_centry, CE_doubleval, request);
            job_centry = raw_centry; 
            DPRINTF(("using default request for %s!\n", lGetString(raw_centry, CE_name)));
         } else if (is_forced == true) {
            schedd_mes_add(a->job_id, SCHEDD_INFO_NOTREQFORCEDRES); 
            ret = DISPATCH_NEVER_CAT;
            break;
         } else {
            /* ignoring because centry was not requested and is no consumable */
            DPRINTF(("complex not requested!\n"));
            continue;
         }
      }

      {
         lList *tmp_centry_list = lCreateList("", CE_Type);
         lList *tmp_rue_list = lCreateList("", RUE_Type);
         lListElem *tmp_centry_elem = NULL;
         lListElem *tmp_rue_elem = NULL;
            
         if (rqs_set_dynamical_limit(limit, global_host, exec_host, a->centry_list)) {
            lList *rue_list = lGetList(limit, RQRL_usage);
            u_long32 tmp_time = a->start;

            /* create tmp_centry_list */
            tmp_centry_elem = lCopyElem(raw_centry);
            lSetString(tmp_centry_elem, CE_stringval, lGetString(limit, RQRL_value));
            lSetDouble(tmp_centry_elem, CE_doubleval, lGetDouble(limit, RQRL_dvalue));
            lAppendElem(tmp_centry_list, tmp_centry_elem);

            /* create tmp_rue_list */
            rqs_get_rue_string(&rue_name, rule, lGetString(a->job, JB_owner),
                                      lGetString(a->job, JB_project),
                                      host, queue, NULL);
            tmp_rue_elem = lCopyElem(lGetElemStr(rue_list, RUE_name, sge_dstring_get_string(&rue_name)));
            if (tmp_rue_elem == NULL) {
               tmp_rue_elem = lCreateElem(RUE_Type);
            }
            lSetString(tmp_rue_elem, RUE_name, limit_name);
            lAppendElem(tmp_rue_list, tmp_rue_elem);
           
            sge_dstring_clear(&reason);
            ret = ri_time_by_slots(a, job_centry, NULL, tmp_centry_list,  tmp_rue_list,
                                       NULL, &reason, false, 1, DOMINANT_LAYER_RQS, 0.0, &tmp_time,
                                       SGE_RQS_NAME);
            if (ret != DISPATCH_OK) {
               DPRINTF(("denied because: %s\n", sge_dstring_get_string(&reason)));
               lFreeList(&tmp_rue_list);
               lFreeList(&tmp_centry_list);
               break;
            }

            if (a->is_reservation && ret == DISPATCH_OK) {
               *start = tmp_time;
            }

            lFreeList(&tmp_rue_list);
            lFreeList(&tmp_centry_list);
         }
      }
   }

   sge_dstring_free(&reason);
   sge_dstring_free(&rue_name);

   DRETURN(ret);
}

/****** sge_select_queue/sort_queue_list() *************************************
*  NAME
*     sort_queue_list() -- sorts thh queue list 
*
*  SYNOPSIS
*     static void sort_queue_list(lList *queue_list) 
*
*  FUNCTION
*     Sorts the queue list based on the queue sort method
*
*  INPUTS
*     lList *queue_list - queue list
*
*  NOTES
*     MT-NOTE: sort_queue_list() is MT safe 
*
*******************************************************************************/
static void sort_queue_list(lList *queue_list) 
{
   if (sconf_get_queue_sort_method() == QSM_LOAD) {
      lPSortList(queue_list, "%I+ %I+ %I+", QU_soft_violation, QU_host_seq_no, QU_seq_no);
   }   
   else {
      lPSortList(queue_list, "%I+ %I+ %I+", QU_soft_violation, QU_seq_no, QU_host_seq_no);
   } 
}

/****** sge_select_queue/add_pe_slots_to_category() ****************************
*  NAME
*     add_pe_slots_to_category() -- defines an array of valid slot values
*
*  SYNOPSIS
*     static bool add_pe_slots_to_category(category_use_t *use_category, 
*     u_long32 *max_slotsp, lListElem *pe, int min_slots, int max_slots, lList 
*     *pe_range) 
*
*  FUNCTION
*     In case of pe ranges does this function alocate memory and filles it wil
*     valid pe slot values. If a category is set, it stores them the category
*     for further jobs. 
*
*  INPUTS
*     category_use_t *use_category - category caching structure, must not be NULL
*     u_long32 *max_slotsp         - number of different slot settings
*     lListElem *pe                - pe, must not be NULL
*     int min_slots                - min slot setting (pe range)
*     int max_slots                - max slot setting (pe range)
*     lList *pe_range              - pe range, must not be NULL
*
*  RESULT
*     static bool - true, if successfull 
*
*  NOTES
*     MT-NOTE: add_pe_slots_to_category() is MT safe 
*
*******************************************************************************/
static bool 
add_pe_slots_to_category(category_use_t *use_category, u_long32 *max_slotsp, lListElem *pe, 
                         int min_slots, int max_slots, lList *pe_range)
{
   if (use_category->cache != NULL) {
      use_category->posible_pe_slots = lGetRef(use_category->cache, CCT_pe_job_slots); 
   }
   
   /* --- prepare the posible slots for the binary search */
   if (use_category->posible_pe_slots == NULL) {
      int slots;

      *max_slotsp = 0;
      for (slots = min_slots; slots <= max_slots; slots++) {

         /* sort out slot numbers that would conflict with allocation rule */
         if (sge_pe_slots_per_host(pe, slots) == 0) {
            continue;
         }   

         /* only slot numbers from jobs PE range request */
         if (range_list_is_id_within(pe_range, slots) == 0) {
            continue;
         }   

         if (use_category->posible_pe_slots == NULL) {
            use_category->posible_pe_slots = malloc((max_slots - min_slots + 1) * sizeof(u_long32));
            if (use_category->posible_pe_slots == NULL) {
               return false;
            }
         }
         use_category->posible_pe_slots[(*max_slotsp)] = slots;
         (*max_slotsp)++;
      }
      
      if (use_category->cache != NULL) {
         lSetRef(use_category->cache, CCT_pe_job_slots, use_category->posible_pe_slots);
         lSetUlong(use_category->cache, CCT_pe_job_slot_count, *max_slotsp);
         use_category->is_pe_slots_rev = true;
      }
      else {
         use_category->is_pe_slots_rev = false; 
      }
   }
   else {
      use_category->is_pe_slots_rev = true;
      *max_slotsp = lGetUlong(use_category->cache, CCT_pe_job_slot_count);
   }
   return true;
}

/****** sge_select_queue/fill_category_use_t() **************
*  NAME
*     fill_category_use_t() -- fills the category_use_t structure.
*
*  SYNOPSIS
*     void fill_category_use_t(sge_assignment_t *a, category_use_t 
*     *use_category, const char *pe_name) 
*
*  FUNCTION
*     The category structure got a bit complicated, therefor it
*     retrieves all important information from the category and
*     stores it for easy access in the category_use_t structure.
*     
*     If a cache structure for the given PE does not exist, it
*     will generate the neccissary data structures. 
*
*  INPUTS
*     sge_assignment_t *a          - job info structure (in)
*     category_use_t *use_category - category info structure (out)
*     const char* pe_name          - the current pe name or "NONE"
*
*  NOTES
*     MT-NOTE: fill_category_use_t() is MT safe 
*******************************************************************************/
static void fill_category_use_t(const sge_assignment_t *a, category_use_t *use_category, const char *pe_name) {
   lListElem *job = a->job;

   DENTER(TOP_LAYER, "fill_category_use_t");

   use_category->category = lGetRef(job, JB_category);
   if (use_category->category != NULL) { 
      use_category->cache = lGetElemStr(lGetList(use_category->category, CT_cache), CCT_pe_name, pe_name);
      if (use_category->cache == NULL) {
         use_category->cache = lCreateElem(CCT_Type);

         lSetString(use_category->cache, CCT_pe_name, pe_name);
         lSetList(use_category->cache, CCT_ignore_queues, lCreateList("", CTI_Type));
         lSetList(use_category->cache, CCT_ignore_hosts, lCreateList("", CTI_Type));
         lSetList(use_category->cache, CCT_queue_violations, lCreateList("", CTQV_Type));   
         lSetList(use_category->cache, CCT_job_messages, lCreateList("", MES_Type));
            
         if (lGetList(use_category->category, CT_cache) == NULL) {
            lSetList(use_category->category, CT_cache, lCreateList("pe_cache", CCT_Type));
         }
         lAppendElem(lGetList(use_category->category, CT_cache), use_category->cache);
      }

      use_category->mod_category = true; 

      use_category->use_category = ((a->start == DISPATCH_TIME_NOW) && 
                                  (use_category->category != NULL)  && 
                                  lGetUlong(use_category->category, CT_refcount) > MIN_JOBS_IN_CATEGORY) ? true : false;
                          
      use_category->use_cviolation = (job_has_soft_requests(job) && 
                                    use_category->use_category && 
                                    lGetNumberOfElem(lGetList(use_category->cache, CCT_queue_violations)) > 0) ? true : false; 
                            
      use_category->compute_violation = (!use_category->use_cviolation && 
                                       job_has_soft_requests(job)) ? true : false;
   }
   else {
      use_category->cache = NULL;
      use_category->mod_category = false;
      use_category->use_category = false;
      use_category->use_cviolation = false;
      use_category->compute_violation = false;
   }
   use_category->posible_pe_slots = NULL;
   use_category->is_pe_slots_rev = false;

   DRETURN_VOID;
}


/****** sge_select_queue/parallel_tag_queues_suitable4job() *********
*  NAME
*     parallel_tag_queues_suitable4job() -- Tag queues/hosts for 
*        a comprehensive/parallel assignment
*
*  SYNOPSIS
*     static int parallel_tag_queues_suitable4job(sge_assignment_t 
*                *assignment) 
*
*  FUNCTION
*     We tag the amount of available slots for that job at global, host and 
*     queue level under consideration of all constraints of the job. We also 
*     mark those queues that are suitable as a master queue as possible master 
*     queues and count the number of violations of the job's soft request. 
*     The method below is named comprehensive since it does the tagging game
*     for the whole parallel job and under consideration of all available 
*     resources that could help to suffice the jobs request. This is necessary 
*     to prevent consumable resource limited at host/global level multiple 
*     times. 
*
*     While tagging we also set queues QU_host_seq_no based on the sort 
*     order of each host. Assumption is the host list passed is sorted 
*     according the load forumla. 
*
*  INPUTS
*     sge_assignment_t *assignment - ??? 
*     category_use_t use_category - information on how to use the job category
*
*  RESULT
*     static dispatch_t - 0 ok got an assignment
*                         1 no assignment at the specified time
*                        -1 assignment will never be possible for all jobs of that category
*                        -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: parallel_tag_queues_suitable4job() is not MT safe 
*******************************************************************************/
static dispatch_t
parallel_tag_queues_suitable4job(sge_assignment_t *a, category_use_t *use_category, int *available_slots) 
{
   lListElem *job = a->job;
   bool need_master_host = (lGetList(job, JB_master_hard_queue_list) != NULL) ? true : false;

   int global_soft_violations = 0;
   int accu_host_slots, accu_host_slots_qend;
   bool have_master_host, suited_as_master_host;
   lListElem *hep;
   dispatch_t best_result = DISPATCH_NEVER_CAT; 
   int gslots, gslots_qend;
   int host_seqno = 0;
   double previous_load;
   bool previous_load_inited = false;
   int allocation_rule, minslots;

   DENTER(TOP_LAYER, "parallel_tag_queues_suitable4job");

   clean_up_parallel_job(a); 

   if (use_category->use_category) {
      schedd_mes_set_tmp_list(use_category->cache, CCT_job_messages, lGetUlong(job, JB_job_number));
   }

   /* remove reasons from last unsuccesful iteration */ 
   clean_monitor_alp();

   parallel_global_slots(a, &gslots, &gslots_qend,
                        use_category->compute_violation? &global_soft_violations:NULL); 

   if (gslots < a->slots) {
      best_result = (gslots_qend < a->slots) ? DISPATCH_NEVER_CAT : DISPATCH_NOT_AT_TIME;

      if (best_result == DISPATCH_NOT_AT_TIME) {
         DPRINTF(("GLOBAL will <category_later> get us %d slots (%d)\n", 
            gslots, gslots_qend));
      } else {
         DPRINTF(("GLOBAL will <category_never> get us %d slots (%d)\n", 
            gslots, gslots_qend));
      }   
   } else {
      lList *skip_host_list = NULL;
      lList *skip_cqueue_list = NULL;
      lList *unclear_cqueue_list = NULL;
  
      if (use_category->use_category) {
         skip_host_list = lGetList(use_category->cache, CCT_ignore_hosts);
      }   
  
      accu_host_slots = accu_host_slots_qend = 0;
      have_master_host = false;
      allocation_rule = sge_pe_slots_per_host(a->pe, a->slots);
      minslots = ALLOC_RULE_IS_BALANCED(allocation_rule)?allocation_rule:1;

      /*
       * first select hosts with lowest share/load and then select queues with
       * tag amount of slots we can get served with resources limited per host
       */

      for_each (hep, a->host_list) {

         int hslots = 0, hslots_qend = 0;
         const char *eh_name = lGetHost(hep, EH_name);

         if (!strcasecmp(eh_name, SGE_GLOBAL_NAME) || !strcasecmp(eh_name, SGE_TEMPLATE_NAME)) {
            continue;
         }   

         /* this host does not work for this category, skip it */
         if (skip_host_list && lGetElemStr(skip_host_list, CTI_name, eh_name)){
            continue;
         }

         /* 
          * do not perform expensive checks for this host if there 
          * is not at least one free queue residing at this host:
          * see if there are queues which are not disbaled/suspended/calender;
          * which have at least one free slot, which are not unknown, several alarms
          */

         if (lGetElemHost(a->queue_list, QU_qhostname, eh_name)) {

            parallel_tag_hosts_queues(a, hep, &hslots, &hslots_qend, global_soft_violations, 
                &suited_as_master_host, &host_seqno, &previous_load, &previous_load_inited, use_category,
                &skip_cqueue_list, &unclear_cqueue_list);

            if (hslots >= minslots) {
               accu_host_slots += hslots;
            }   
            if (hslots_qend >= minslots) {
               accu_host_slots_qend += hslots_qend;
            }

            DPRINTF(("HOST(3) %s could get us %d slots (%d later on)\n", 
                  eh_name, hslots, hslots_qend));

            /* tag full amount or zero */
            lSetUlong(hep, EH_tagged, hslots); 
            lSetUlong(hep, EH_master_host, suited_as_master_host?1:0); 
            have_master_host |= suited_as_master_host;
         }

         /* mark host as not suitable */
         /* at least when accu_host_slots and accu_host_slots_qend < a->slots */
         /* and only, if modify category is set */
         if (skip_host_list &&  use_category->mod_category ) {
            if (hslots < minslots && hslots_qend < minslots) {
               lAddElemStr(&skip_host_list, CTI_name, eh_name, CTI_Type);
            }
         }
      } /* for each host */

      lFreeList(&skip_cqueue_list);
      lFreeList(&unclear_cqueue_list);

      if (accu_host_slots >= a->slots && 
         (!need_master_host || (need_master_host && have_master_host))) {
         /* stop looking for smaller slot amounts */
         DPRINTF(("-------------->      BINGO %d slots %s at specified time <--------------\n", 
               a->slots, need_master_host?"plus master host":""));
         best_result = DISPATCH_OK;
      } else if (accu_host_slots_qend >= a->slots && (!need_master_host || 
                  (need_master_host && have_master_host))) {
         DPRINTF(("-------------->            %d slots %s later             <--------------\n", 
               a->slots, need_master_host?"plus master host":""));
         best_result = DISPATCH_NOT_AT_TIME;
      } else {
         DPRINTF(("-------------->   NEVER ONLY %d but %d needed              <--------------\n", 
               accu_host_slots, a->slots));
         best_result = DISPATCH_NEVER_CAT;
      }

      *available_slots = accu_host_slots_qend;

      if (rmon_condition(xaybzc, INFOPRINT)) {
         switch (best_result) {
         case DISPATCH_OK:
            DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns "sge_u32"\n", 
                  a->slots, a->start));
            break;
         case DISPATCH_NOT_AT_TIME:
            DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <later>\n", 
                  a->slots));
            break;
         case DISPATCH_NEVER_CAT:
            DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <category_never>\n", 
                  a->slots));
            break;
         case DISPATCH_NEVER_JOB:
            DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <job_never>\n", 
                  a->slots));
            break;
         default:
            DPRINTF(("!!!!!!!! COMPREHSENSIVE ASSIGNMENT(%d) returns unexpected %d\n", 
                  best_result));
            break;
         }
      }
   } 

   if (use_category->use_category) {  
      lList *temp = schedd_mes_get_tmp_list();
      if (temp){    
         lSetList(use_category->cache, CCT_job_messages, lCopyList(NULL, temp));
       }
   }

   DRETURN(best_result);
}


/****** sge_select_queue/parallel_tag_hosts_queues() **********************************
*  NAME
*     parallel_tag_hosts_queues() -- Determine host slots and tag queue(s) accordingly
*
*  SYNOPSIS
*
*  FUNCTION
*     For a particular job the maximum number of slots that could be served 
*     at that host is determined in accordance with the allocation rule and
*     returned. The time of the assignment can be either DISPATCH_TIME_NOW
*     or a specific time, but never DISPATCH_TIME_QUEUE_END.
*
*     In those cases when the allocation rule allows more than one slot be 
*     served per host it is necessary to also consider per queue possibly 
*     specified load thresholds. This is because load is global/per host 
*     concept while load thresholds are a queue attribute.
*
*     In those cases when the allocation rule gives us neither a fixed amount 
*     of slots required nor an upper limit for the number per host slots (i.e. 
*     $fill_up and $round_robin) we must iterate through all slot numbers from 
*     1 to the maximum number of slots "total_slots" and check with each slot
*     amount whether we can get it or not. Iteration stops when we can't get 
*     more slots the host based on the queue limitations and load thresholds. 
*
*     As long as only one single queue at the host is eligible for the job the
*     it is sufficient to check with each iteration whether the corresponding 
*     number of slots can be served by the host and it's queue or not. The 
*     really sick case however is when multiple queues are eligable for a host: 
*     Here we have to determine in each iteration step also the maximum number 
*     of slots each queue could get us by doing a per queue iteration from the 
*     1 up to the maximum number of slots we're testing. The optimization in 
*     effect here is to check always only if we could get more slots than with
*     the former per host slot amount iteration. 
*
*  INPUTS
*     sge_assignment_t *a          -
*     lListElem *hep               - current host
*     lListElem *global            - global host
*     int *slots                   - out: # free slots
*     int *slots_qend              - out: # free slots in the far far future
*     int global_soft_violations   - # of global soft violations
*     bool *master_host            - out: if true, found a master host
*     int  *host_seqno             -  
*     double *previous_load        -
*     bool *previous_load_inited   -
*     category_use_t *use_category - int/out : how to use the job category
*
*  RESULT
*     static dispatch_t -  0 ok got an assignment
*                          1 no assignment at the specified time
*                         -1 assignment will never be possible for all jobs of that category
*                         -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: parallel_tag_hosts_queues() is not MT safe 
*******************************************************************************/
static dispatch_t
parallel_tag_hosts_queues(sge_assignment_t *a, lListElem *hep, int *slots, int *slots_qend, 
                          int global_soft_violations, bool *master_host, int *host_seqno, 
                          double *previous_load, bool *previous_load_inited,
                          category_use_t *use_category, 
                          lList **skip_cqueue_list, lList **unclear_cqueue_list) 
{

   bool suited_as_master_host = false;
   int min_host_slots, max_host_slots;
   int accu_queue_slots, accu_queue_slots_qend;
   int qslots, qslots_qend, hslots, hslots_qend;
   int host_soft_violations, queue_soft_violations;
   const char *cqname, *qname, *eh_name = lGetHost(hep, EH_name);
   lListElem *qep, *next_queue; 
   dispatch_t result;
   const void *queue_iterator = NULL;
   int allocation_rule; 
   
   DENTER(TOP_LAYER, "parallel_tag_hosts_queues");

   allocation_rule = sge_pe_slots_per_host(a->pe, a->slots);

   if (ALLOC_RULE_IS_BALANCED(allocation_rule)) {
      min_host_slots = max_host_slots = allocation_rule;
   }   
   else {
      min_host_slots = 1;
      max_host_slots = a->slots;
   }

   host_soft_violations = global_soft_violations;

   result = parallel_host_slots(a, &hslots, &hslots_qend, 
                               (use_category->compute_violation?&host_soft_violations:NULL), 
                               hep, false);

   DPRINTF(("HOST %s itself (and queue threshold) will get us %d slots (%d later) ... "
         "we need %d\n", eh_name, hslots, hslots_qend, min_host_slots));

   hslots      = MIN(hslots,      max_host_slots);
   hslots_qend = MIN(hslots_qend, max_host_slots);

   if (hslots >= min_host_slots || hslots_qend >= min_host_slots) {
      lList *skip_queue_list = NULL;
      if (use_category->use_category) {
         skip_queue_list = lGetList(use_category->cache, CCT_ignore_queues);
      }
      
      accu_queue_slots = accu_queue_slots_qend = 0;

      for (next_queue = lGetElemHostFirst(a->queue_list, QU_qhostname, eh_name, &queue_iterator); 
          (qep = next_queue);
           next_queue = lGetElemHostNext(a->queue_list, QU_qhostname, eh_name, &queue_iterator)) {

         qname = lGetString(qep, QU_full_name);
         cqname = lGetString(qep, QU_qname);

         /* try to foreclose the cluster queue */
         if (lGetElemStr(*skip_cqueue_list, CTI_name, cqname)) {
            DPRINTF(("skip cluster queue %s\n", cqname));             
            continue;
         }
         if (!lGetElemStr(*unclear_cqueue_list, CTI_name, cqname)) {
            if (cqueue_match_static(cqname, a) != DISPATCH_OK) {
               lAddElemStr(skip_cqueue_list, CTI_name, cqname, CTI_Type);
               continue;
            }
            lAddElemStr(unclear_cqueue_list, CTI_name, cqname, CTI_Type);
         } else
            DPRINTF(("checked cluster queue %s already\n", cqname));             

         if (skip_queue_list && lGetElemStr(skip_queue_list, CTI_name, qname)){
            continue;
         }
         
         queue_soft_violations = host_soft_violations;

         result = parallel_queue_slots(a, qep, &qslots, &qslots_qend, 
                                     (use_category->compute_violation?&queue_soft_violations:NULL), 
                                     false);

         /* the soft request violations can be cached in the categories. If they have been computed for a previous job
            in the same category, they are cached and we can used reuse the same values. */
         if (use_category->use_cviolation) { /* reusing the prev. computed values. */
            lListElem *queue_violation = lGetElemStr(lGetList(use_category->cache, CCT_queue_violations), CTQV_name, qname); 
            if (queue_violation){
               lSetUlong(qep, QU_soft_violation, lGetUlong(queue_violation, CTQV_count));
            }
         }   
         else if (use_category->compute_violation) { /* storing the new computed values. */
            lListElem *queue_violation = lAddSubStr (use_category->cache, CTQV_name, qname, CCT_queue_violations, CTQV_Type);
            lSetUlong(queue_violation, CTQV_count, queue_soft_violations); 
         }

         if (result == DISPATCH_OK && (qslots > 0 || qslots_qend > 0)) {

            /* in case the load of two hosts is equal this
               must be also reflected by the sequence number */
            if (*previous_load_inited && (*previous_load < lGetDouble(hep, EH_sort_value))) {
               (*host_seqno)++;
            }   
            *previous_load_inited = true;
            *previous_load = lGetDouble(hep, EH_sort_value);
            lSetUlong(qep, QU_host_seq_no, *host_seqno);

            /* could this host be a master host */
            if (!suited_as_master_host && lGetUlong(qep, QU_tagged4schedule)) {
               DPRINTF(("HOST %s can be master host because of queue %s\n", eh_name, qname));    
               suited_as_master_host = true; 
            }


            DPRINTF(("QUEUE %s TIME: %d + %d -> %d  QEND: %d + %d -> %d (%d soft violations)\n", qname, 
                     accu_queue_slots,      qslots,      accu_queue_slots+       qslots, 
                     accu_queue_slots_qend, qslots_qend, accu_queue_slots_qend + qslots_qend,
                     (int)lGetUlong(qep, QU_soft_violation))); 
            accu_queue_slots      += qslots;
            accu_queue_slots_qend += qslots_qend;
            lSetUlong(qep, QU_tag, qslots);
         } 
         else {
            if (skip_queue_list) {
               lAddElemStr(&skip_queue_list, CTI_name, qname, CTI_Type);
            }         
            DPRINTF(("HOST(1.5) %s will get us nothing\n", eh_name)); 
         }

      } /* for each queue of the host */

      hslots      = MIN(accu_queue_slots,      hslots);
      hslots_qend = MIN(accu_queue_slots_qend, hslots_qend);

      DPRINTF(("HOST %s and it's queues will get us %d slots (%d later) ... we need %d\n", 
            eh_name, hslots, hslots_qend, min_host_slots));
   }

   *slots       = hslots;
   *slots_qend  = hslots_qend;
   *master_host = suited_as_master_host;

   DRETURN(DISPATCH_OK);
}

/* 
 * Determine maximum number of host_slots as limited 
 * by queue load thresholds. The maximum only considers
 * thresholds with load adjustments
 * 
 * for each queue Q at this host {
 *    for each threshold T of a queue {
 *       if compare operator > or >=
 *          avail(Q, T) = <threshold - load / adjustment) + 1     
 *       else
 *          avail(Q, T) = (threshold - load / -adjustMent) + 1
 *    }
 *    avail(Q) = MIN(all avail(Q, T))
 * }
 * host_slot_max_by_T = MAX(all min(Q))
 */
static int 
parallel_max_host_slots(sge_assignment_t *a, lListElem *host) {
   int avail_h = 0, avail_q;
   int avail;
   lListElem *next_queue, *qep, *centry = NULL;
   lListElem *lv, *lc, *tr, *fv, *cep;
   const char *eh_name = lGetHost(host, EH_name);
   const void *queue_iterator = NULL;
   const char *load_value, *limit_value, *adj_value;
   u_long32 type;
   bool is_np_adjustment = false;
   lList *requests = lGetList(a->job, JB_hard_resource_list);

   DENTER(TOP_LAYER, "parallel_max_host_slots");

   for (next_queue = lGetElemHostFirst(a->queue_list, QU_qhostname, eh_name, &queue_iterator); 
       (qep = next_queue);
        next_queue = lGetElemHostNext(a->queue_list, QU_qhostname, eh_name, &queue_iterator)) {
      lList *load_thresholds = lGetList(qep, QU_load_thresholds);
      avail_q = INT_MAX;

      for_each(tr, load_thresholds) {
         double load, threshold, adjustment;
         const char *name = lGetString(tr, CE_name);
         if ((cep = centry_list_locate(a->centry_list, name)) == NULL) {
            continue;
         }   
         
         if (!lGetBool(cep, CE_consumable)) { /* work on the load values */
            if ((lc = lGetElemStr(a->load_adjustments, CE_name, name)) == NULL) {
               continue;
            } 
            if ((lv=lGetSubStr(host, HL_name, name, EH_load_list)) != NULL) {
               load_value = lGetString(lv, HL_value);
            }
            else if ((lv = lGetSubStr(a->gep, HL_name, name, EH_load_list)) != NULL) {
               load_value = lGetString(lv, HL_value);
            } 
            else {
               fv = lGetSubStr(qep, CE_name, name, QU_consumable_config_list);
               load_value = lGetString(fv, CE_stringval);
            }
            adj_value = lGetString(lc, CE_stringval);
            is_np_adjustment = true;
            adj_value = lGetString(lc, CE_stringval);

         }
         else { /* work on a consumable */
            
            if ((lc = lGetElemStr(requests, CE_name, name)) != NULL) {
               adj_value = lGetString(lc, CE_stringval);
            }
            else { /* is default value */
               adj_value = lGetString(cep, CE_default);
            }

            if ((centry = get_attribute_by_name(a->gep, host, qep, name, a->centry_list, a->start, a->duration)) == NULL) {
                /* no load value, no assigned consumable to queue, host, or global */
                DPRINTF(("the consumable "SFN" used in queue "SFN" as load threshold has no instance at queue, host or global level\n", 
                         name, lGetString(qep, QU_full_name))); 
                DRETURN(0);
            }    

            load_value = lGetString(centry, CE_pj_stringval);
         }

         limit_value = lGetString(tr, CE_stringval);
         type = lGetUlong(cep, CE_valtype);

         /* get the needed values. If the load value is not a number, ignore it */
         switch (type) {
            case TYPE_INT:
            case TYPE_TIM:
            case TYPE_MEM:
            case TYPE_BOO:
            case TYPE_DOUBLE:

               if (!parse_ulong_val(&load, NULL, type, load_value, NULL, 0) ||
                   !parse_ulong_val(&threshold, NULL, type, limit_value, NULL, 0) ||
                   !parse_ulong_val(&adjustment, NULL, type, adj_value, NULL, 0)) {
                   lFreeElem(&centry);
                  continue;
               }

               break;

            default:
               lFreeElem(&centry);
               continue;    
         }
         /* the string in load_value is not needed anymore, we can free the element */
         lFreeElem(&centry);

         /* a adjustment of NULL is ignored here. We can dispatch unlimited jobs based
            on the load value */
         if (adjustment == 0) {
            continue;
         }

         switch (lGetUlong(cep, CE_relop)) {
            case CMPLXEQ_OP :
            case CMPLXNE_OP : continue; /* we cannot compute a usefull range */
            case CMPLXLT_OP :
            case CMPLXLE_OP : adjustment *= -1;
               break;
         }

         if (is_np_adjustment) {
            load_np_value_adjustment(name, host, &adjustment);
         }

         /* load alarm is defined in a way, that a host can go with one
            used slot into alarm and not that the dispatching prevents
            the alarm state. For this behavior we have to add 1 to the
            available slots. However, this is not true for consumables
            as load threshold*/
         avail = (threshold - load)/adjustment;
         if (!lGetBool(cep, CE_consumable)) {
            avail++;
         }
         avail_q = MIN(avail, avail_q);         
      }

      avail_h = MAX(avail_h, avail_q);
   }
   DRETURN(avail_h);
}

/****** sge_select_queue/parallel_rqs_slots_by_time() *************************
*  NAME
*     parallel_rqs_slots_by_time() -- Dertermine number of slots avail within
*                                      time frame
*
*  SYNOPSIS
*     static dispatch_t parallel_rqs_slots_by_time(const sge_assignment_t *a, 
*     int *slots, int *slots_qend, const char *host, const char *queue) 
*
*  FUNCTION
*     This function iterates for a queue instance over all resource quota sets
*     and evaluates the number of slots available.
*
*  INPUTS
*     const sge_assignment_t *a - job info structure (in)
*     int *slots                - out: # free slots
*     int *slots_qend           - out: # free slots in the far far future
*     const char *host          - host name
*     const char *queue         - queue name
*
*  RESULT
*     static dispatch_t - DISPATCH_OK        got an assignment
*                       - DISPATCH_NEVER_CAT no assignment for all jobs af that category
*
*  NOTES
*     MT-NOTE: parallel_rqs_slots_by_time() is not MT safe 
*
*  SEE ALSO
*     ri_slots_by_time()
*     
*******************************************************************************/
static dispatch_t
parallel_rqs_slots_by_time(const sge_assignment_t *a, int *slots, int *slots_qend, const char *host, const char *queue)
{
   dispatch_t result = DISPATCH_OK;
   int tslots = INT_MAX;
   int tslots_qend = INT_MAX;

   DENTER(TOP_LAYER, "parallel_rqs_slots_by_time");

   /* We have to set at first the slots for the case no ruleset is defined or all are enabled */
   *slots = tslots;
   *slots = tslots_qend;

   if (lGetNumberOfElem(a->rqs_list) == 0) {
      DRETURN(result);
   } else {
      const char* user = lGetString(a->job, JB_owner);
      const char* project = lGetString(a->job, JB_project);
      const char* pe = lGetString(a->job, JB_pe);
      lListElem *rqs = NULL;
      bool first = true;
      dstring rule_name = DSTRING_INIT;
      dstring rue_name = DSTRING_INIT;

      for_each(rqs, a->rqs_list) {
         lListElem *rule = NULL;
         lListElem *global_host = host_list_locate(a->host_list, SGE_GLOBAL_NAME);
         lListElem *exec_host = host_list_locate(a->host_list, host);

         /* ignore disabled rule sets */
         if (!lGetBool(rqs, RQS_enabled)) {
            continue;
         }
         sge_dstring_clear(&rule_name);
         rule = rqs_get_matching_rule(rqs, user, project, pe, host, queue, a->acl_list, a->hgrp_list, &rule_name);
         if (rule != NULL) {
            lList *limit_list = lGetList(rule, RQR_limit);
            lListElem *limit = NULL;

            for_each(limit, limit_list) {
               const char *limit_name = lGetString(limit, RQRL_name);

               lListElem *raw_centry = centry_list_locate(a->centry_list, limit_name);
               lList *job_centry_list = lGetList(a->job, JB_hard_resource_list);
               lListElem *job_centry = centry_list_locate(job_centry_list, limit_name);

               if (raw_centry == NULL) {
                  DPRINTF(("ignoring limit %s because not defined", limit_name));
                  continue;
               } else {
                  DPRINTF(("checking limit %s\n", lGetString(raw_centry, CE_name)));
               }

               /* found a rule, now check limit */
               if (lGetBool(raw_centry, CE_consumable)) {

                  rqs_get_rue_string(&rue_name, rule, user, project, host, queue, pe);

                  if (rqs_set_dynamical_limit(limit, global_host, exec_host, a->centry_list)) {
                     result = parallel_limit_slots_by_time(a, job_centry_list, &tslots, &tslots_qend, raw_centry, limit, rue_name);
                     if (first) {
                        first = false;
                        *slots = tslots;
                        *slots_qend = tslots_qend;
                     } else {
                        *slots = MIN(*slots, tslots);
                        *slots_qend = MIN(*slots_qend, tslots_qend);
                     }
                     if (result != DISPATCH_OK) {
                        sge_dstring_free(&rule_name);
                        sge_dstring_free(&rue_name);
                        DRETURN(result);
                     }
                  } else {
                     schedd_mes_add(a->job_id, SCHEDD_INFO_JOBDYNAMICALLIMIT_SS, limit_name, sge_dstring_get_string(&rule_name));
                     sge_dstring_free(&rule_name);
                     DRETURN(DISPATCH_NEVER_CAT);
                  }
               } else {
                  char availability_text[2048];

                  lSetString(raw_centry, CE_stringval, lGetString(limit, RQRL_value));
                  if (compare_complexes(1, raw_centry, job_centry, availability_text, false, false) != 1) {
                     schedd_mes_add(a->job_id, SCHEDD_INFO_CANNOTRUNRQS_SSS, queue, host, sge_dstring_get_string(&rule_name));
                     sge_dstring_free(&rule_name);
                     DRETURN(DISPATCH_NEVER_CAT);
                  }
               }
            }
         }
      }
      sge_dstring_free(&rue_name);
      sge_dstring_free(&rule_name);
   }

   DRETURN(result);
}

/****** sge_select_queue/parallel_limit_slots_by_time() ************************
*  NAME
*     parallel_limit_slots_by_time() -- Determine number of slots avail. within
*                                       time frame
*
*  SYNOPSIS
*     static dispatch_t parallel_limit_slots_by_time(const sge_assignment_t *a, 
*     lList *requests, int *slots, int *slots_qend, lListElem *centry, lListElem 
*     *limit, dstring rue_name) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const sge_assignment_t *a - job info structure (in)
*     lList *requests           - Job request list (CE_Type)
*     int *slots                - out: free slots
*     int *slots_qend           - out: free slots in the far far future
*     lListElem *centry         - Load information for the resource
*     lListElem *limit          - limitation (RQRL_Type)
*     dstring rue_name          - rue_name saved in limit sublist RQRL_usage
*
*  RESULT
*     static dispatch_t - DISPATCH_OK        got an assignment
*                       - DISPATCH_NEVER_CAT no assignment for all jobs af that category
*
*  NOTES
*     MT-NOTE: parallel_limit_slots_by_time() is not MT safe 
*
*  SEE ALSO
*     parallel_rc_slots_by_time
*******************************************************************************/
static dispatch_t 
parallel_limit_slots_by_time(const sge_assignment_t *a, lList *requests, 
                 int *slots, int *slots_qend, lListElem *centry, lListElem *limit, dstring rue_name)
{
   lList *tmp_centry_list = lCreateList("", CE_Type);
   lList *tmp_rue_list = lCreateList("", RUE_Type);
   lListElem *tmp_centry_elem = NULL;
   lListElem *tmp_rue_elem = NULL;
   lList *rue_list = lGetList(limit, RQRL_usage);
   dispatch_t result = DISPATCH_NEVER_CAT;

   DENTER(TOP_LAYER, "parallel_limit_slots_by_time");

   /* create tmp_centry_list */
   tmp_centry_elem = lCopyElem(centry);
   lSetDouble(tmp_centry_elem, CE_doubleval, lGetDouble(limit, RQRL_dvalue));
   lAppendElem(tmp_centry_list, tmp_centry_elem);

   /* create tmp_rue_list */
   tmp_rue_elem = lCopyElem(lGetElemStr(rue_list, RUE_name, sge_dstring_get_string(&rue_name)));
   if (tmp_rue_elem == NULL) {
      tmp_rue_elem = lCreateElem(RUE_Type);
   }

   lSetString(tmp_rue_elem, RUE_name, lGetString(limit, RQRL_name));
   lAppendElem(tmp_rue_list, tmp_rue_elem);

   result = parallel_rc_slots_by_time(a, requests, slots, 
                                      slots_qend, tmp_centry_list, tmp_rue_list, NULL,  
                                      false, NULL, DOMINANT_LAYER_RQS, 0.0, RQS_TAG,
                                      false, SGE_RQS_NAME);
   
   lFreeList(&tmp_centry_list);
   lFreeList(&tmp_rue_list);

   DRETURN(result);
}

/****** sge_select_queue/sge_sequential_assignment() ***************************
*  NAME
*     sge_sequential_assignment() -- Make an assignment for a sequential job.
*
*  SYNOPSIS
*     int sge_sequential_assignment(sge_assignment_t *assignment)
*
*  FUNCTION
*     For sequential job assignments all the earliest job start time
*     is determined with each queue instance and the earliest one gets
*     chosen. Secondary criterion for queue selection minimizing jobs 
*     soft requests.
*
*     The overall behaviour of this function is somewhat dependent on the 
*     value that gets passed to assignment->start and whether soft requests 
*     were specified with the job: 
*
*     (1) In case of now assignemnts (DISPATCH_TIME_NOW) only the first queue 
*         suitable for jobs without soft requests is tagged. When soft requests 
*         are specified all queues must be verified and tagged in order to find 
*         the queue that fits best. On success the start time is set 
*     
*     (2) In case of queue end assignments (DISPATCH_TIME_QUEUE_END) 
*
*
*  INPUTS
*     sge_assignment_t *assignment - ??? 
*
*  RESULT
*     int - 0 ok got an assignment + time (DISPATCH_TIME_NOW and DISPATCH_TIME_QUEUE_END)
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_sequential_assignment() is not MT safe 
*******************************************************************************/
dispatch_t
sge_sequential_assignment(sge_assignment_t *a) 
{
   dispatch_t result;
   lListElem *job;
   int old_logging = 0;

   DENTER(TOP_LAYER, "sge_sequential_assignment");

   if (a == NULL) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if (a->is_reservation){
      /* turn off messages for reservation scheduling */
      old_logging = schedd_mes_get_logging();
      schedd_mes_set_logging(0);
   } 

   job = a->job;

   /* check limitation rule sets */
   result = sequential_tag_queues_suitable4job_by_rqs(a);

   if (result == DISPATCH_OK) {

      sequential_update_host_order(a->host_list, a->queue_list);

      if (sconf_get_qs_state() != QS_STATE_EMPTY) {
         /*------------------------------------------------------------------
          *  There is no need to sort the queues after each dispatch in 
          *  case:
          *
          *    1. The last dispatch was also a sequential job without
          *       soft requests. If not then the queues are sorted by
          *       other criterions (soft violations, # of tagged slots, 
          *       masterq).
          *    2. The hosts sort order has not changed since last dispatch.
          *       Load correction or consumables in the load formula can
          *       change the order of the hosts. We detect changings in the
          *       host order by comparing the host sequence number with the
          *       sequence number from previous run.
          * ------------------------------------------------------------------*/
         if (sconf_get_last_dispatch_type() != DISPATCH_TYPE_FAST || sconf_get_host_order_changed()) {
            DPRINTF(("SORTING HOSTS!\n"));
            if (sconf_get_queue_sort_method() == QSM_LOAD) {
               lPSortList(a->queue_list, "%I+ %I+", QU_host_seq_no, QU_seq_no);
            }   
            else {
               lPSortList(a->queue_list, "%I+ %I+", QU_seq_no, QU_host_seq_no);
            }
         }
         if (job_has_soft_requests(a->job)) {
            sconf_set_last_dispatch_type(DISPATCH_TYPE_FAST_SOFT_REQ);
         }
         else {
            sconf_set_last_dispatch_type(DISPATCH_TYPE_FAST);
         }
      }

      result = sequential_tag_queues_suitable4job(a);

      if (result == DISPATCH_OK) {
         bool soft_requests = job_has_soft_requests(a->job);
         lListElem *qep;
         u_long32 job_start_time = MAX_ULONG32;
         u_long32 min_soft_violations = MAX_ULONG32;
         lListElem *best_queue = NULL;

         if (a->is_reservation) {
            for_each (qep, a->queue_list) {
               DPRINTF(("    Q: %s "sge_u32" "sge_u32" (jst: "sge_u32")\n", lGetString(qep, QU_full_name), 
                        lGetUlong(qep, QU_tag), lGetUlong(qep, QU_available_at), job_start_time));
               if (lGetUlong(qep, QU_tag) != 0) {
                  u_long32 temp_job_start_time = lGetUlong(qep, QU_available_at);

                  if ((job_start_time > temp_job_start_time) ||
                       (soft_requests && 
                           min_soft_violations > lGetUlong(qep, QU_soft_violation) && 
                           job_start_time == temp_job_start_time)
                     ) {

                     best_queue = qep;
                     job_start_time = temp_job_start_time;
                     min_soft_violations = lGetUlong(qep, QU_soft_violation);
                  }
               }
            }
            if (best_queue) {
               DPRINTF(("earliest queue \"%s\" at "sge_u32"\n", lGetString(best_queue, QU_full_name), job_start_time));
            } 
            else {
               DPRINTF(("no earliest queue found!\n"));
            }
         } 
         else {
            for_each (qep, a->queue_list) {
               if (lGetUlong(qep, QU_tag)) {
                  job_start_time = lGetUlong(qep, QU_available_at);
                  best_queue = qep;
                  break;
               }
            }
         }

         if (!best_queue) {
            DRETURN(DISPATCH_NEVER_CAT); /* should never happen */
         }
         {
            lListElem *gdil_ep;
            lList *gdil = NULL;
            const char *qname = lGetString(best_queue, QU_full_name);
            const char *eh_name = lGetHost(best_queue, QU_qhostname);

            DPRINTF((sge_u32": 1 slot in queue %s@%s user %s %s for "sge_u32"\n",
               a->job_id, qname, eh_name, lGetString(job, JB_owner), 
                     !a->is_reservation?"scheduled":"reserved", job_start_time));

            gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
            lSetUlong(gdil_ep, JG_qversion, lGetUlong(best_queue, QU_version));
            lSetHost(gdil_ep, JG_qhostname, eh_name);
            lSetUlong(gdil_ep, JG_slots, 1);

            if (!a->is_reservation) {
               sconf_inc_fast_jobs();
            }

            lFreeList(&(a->gdil));
            a->gdil = gdil;
            if (a->start == DISPATCH_TIME_QUEUE_END) {
               a->start = job_start_time;
            }   
      
            result = DISPATCH_OK;
         }
      }

   }

   switch (result) {
   case DISPATCH_OK:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("sge_u32"."sge_u32") returns <time> "sge_u32"\n", 
            a->job_id, a->ja_task_id, a->start));
      break;
   case DISPATCH_NOT_AT_TIME:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("sge_u32"."sge_u32") returns <later>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   case DISPATCH_NEVER_CAT:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("sge_u32"."sge_u32") returns <category_never>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   case DISPATCH_NEVER_JOB:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("sge_u32"."sge_u32") returns <job_never>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   case DISPATCH_MISSING_ATTR:    
   default:
      DPRINTF(("!!!!!!!! SEQUENTIAL ASSIGNMENT("sge_u32"."sge_u32") returns unexpected %d\n", 
            a->job_id, a->ja_task_id, result));
      break;
   }

   if (a->is_reservation) {
      schedd_mes_set_logging(old_logging);
   }   

   DRETURN(result);
}

/*------------------------------------------------------------------
 *  FAST TRACK FOR SEQUENTIAL JOBS WITHOUT A SOFT REQUEST 
 *
 *  It is much faster not to review slots in a comprehensive fashion 
 *  for jobs of this type.
 * ------------------------------------------------------------------*/
static int sequential_update_host_order(lList *host_list, lList *queues)
{
   lListElem *hep, *qep;
   double previous_load = 0;
   int previous_load_inited = 0;
   int host_seqno = 0;
   const char *eh_name;
   const void *iterator = NULL;
   bool host_order_changed = false;

   DENTER(TOP_LAYER, "sequential_update_host_order");

   if (!sconf_get_host_order_changed()) {
      DRETURN(0);
   }

   sconf_set_host_order_changed(host_order_changed);

   for_each (hep, host_list) { /* in share/load order */

      /* figure out host_seqno
         in case the load of two hosts is equal this
         must be also reflected by the sequence number */
      if (!previous_load_inited) {
         host_seqno = 0;
         previous_load = lGetDouble(hep, EH_sort_value);
         previous_load_inited = 1;
      } else {
         if (previous_load < lGetDouble(hep, EH_sort_value)) {
            host_seqno++;
            previous_load = lGetDouble(hep, EH_sort_value);
         }
      }

      /* set host_seqno for all queues of this host */
      eh_name = lGetHost(hep, EH_name);
      
      qep = lGetElemHostFirst(queues, QU_qhostname, eh_name, &iterator); 
      while (qep != NULL) {
          lSetUlong(qep, QU_host_seq_no, host_seqno);
          qep = lGetElemHostNext(queues, QU_qhostname, eh_name, &iterator);
      }

      /* detect whether host_seqno has changed since last dispatch operation */
      if (host_seqno != lGetUlong(hep, EH_seq_no)) {
         DPRINTF(("HOST SORT ORDER CHANGED FOR HOST %s FROM %d to %d\n", 
                  eh_name, lGetUlong(hep, EH_seq_no), host_seqno));
         host_order_changed = true;
         lSetUlong(hep, EH_seq_no, host_seqno);
      }
   }

   sconf_set_host_order_changed(host_order_changed);

   DRETURN(0);
}

/****** sge_select_queue/parallel_assignment() *****************************
*  NAME
*     parallel_assignment() -- Can we assign with a fixed PE/slot/time
*
*  SYNOPSIS
*     int parallel_assignment(sge_assignment_t *assignment) 
*
*  FUNCTION
*     Returns if possible an assignment for a particular PE with a 
*     fixed slot at a fixed time.
*
*  INPUTS
*     sge_assignment_t *a - 
*     category_use_t *use_category - has information on how to use the job category
*
*  RESULT
*     dispatch_t -  0 ok got an assignment
*                   1 no assignment at the specified time
*                  -1 assignment will never be possible for all jobs of that category
*                  -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: parallel_assignment() is not MT safe 
*******************************************************************************/
static dispatch_t
parallel_assignment(sge_assignment_t *a, category_use_t *use_category, int *available_slots) {
   dispatch_t ret;
   int pslots, pslots_qend;

#ifdef SGE_PQS_API
   const char *qsort_args;
#endif

   DENTER(TOP_LAYER, "parallel_assignment");

   if (a == NULL) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if ((ret = parallel_available_slots(a, &pslots, &pslots_qend)) != DISPATCH_OK) {
      DRETURN(ret); 
   }
   if (a->slots > pslots ) {
      DRETURN((a->slots > pslots_qend)? DISPATCH_NEVER_CAT : DISPATCH_NOT_AT_TIME);
   }

   /* depends on a correct host order */
   ret = parallel_tag_queues_suitable4job(a, use_category, available_slots);
   
   if (ret != DISPATCH_OK) {
      DRETURN(ret);
   }

   /* must be understood in the context of changing queue sort orders */
   sconf_set_last_dispatch_type(DISPATCH_TYPE_COMPREHENSIVE);

#ifdef SGE_PQS_API
   /* if dynamic qsort function was supplied, call it */
   if ((qsort_args=lGetString(a->pe, PE_qsort_args)) != NULL) {

      ret = sge_call_pe_qsort(a, qsort_args);
      if (ret!=0) {
         DRETURN(ret);
      }
   }
#endif

   if (parallel_sort_suitable_queues(a->queue_list)) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if (parallel_make_granted_destination_id_list(a) != DISPATCH_OK) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   DRETURN(ret);
}


/****** sge_select_queue/parallel_make_granted_destination_id_list() ***************************************
*  NAME
*     parallel_make_granted_destination_id_list() -- Select slots in queues for the assignment 
*
*  SYNOPSIS
*     static int parallel_make_granted_destination_id_list(sge_assignment_t *assignment, lList *host_list, 
*     lList *queue_list) 
*
*  FUNCTION
*     Make a gdil list from tagged/sorted queue/host list. Major assumption
*     with that function is that it gets called only if an assignment 
*     considering all constraints of the job is actually possible.
*
*     We enter selection code with a queuelist sorted according 'sort_formula' 
*     and 'queue_sort_method'. But for the placement of parallel jobs it is 
*     necessary to select hosts and then select queues. Thus we use the sorted 
*     queue list to find the best suited host for the job, the second best host 
*     and so on.
*
*     Then we iterate through the hosts starting with the best suited and 
*     allocate slots of the best queues at each host according our allocation 
*     rule.
*
*  INPUTS
*     sge_assignment_t *assignment - 
*     lList *host_list             - ??? 
*     lList *queue_list            - ??? 
*
*  RESULT
*     static int - 0 success
*                 -1 error
*
*  NOTES
*     MT-NOTE: parallel_make_granted_destination_id_list() is not MT safe 
*******************************************************************************/
static int parallel_make_granted_destination_id_list( sge_assignment_t *a) 
{
   int max_host_seq_no, start_seq_no, last_accu_host_slots, accu_host_slots = 0;
   int host_slots = 0;
   lList *gdil = NULL;
   const char *eh_name = NULL;
   const char *qname = NULL;
   lListElem *hep = NULL;
   lListElem *qep = NULL;
   int allocation_rule = 0;
   int minslots = 0;
   bool need_master_host = (lGetList(a->job, JB_master_hard_queue_list) != NULL) ? true : false;
   int host_seq_no = 1;
   int total_soft_violations = 0;
   lList *jc_list = NULL;
   dstring rule_name = DSTRING_INIT;
   dstring rue_name = DSTRING_INIT;
   
   DENTER(TOP_LAYER, "parallel_make_granted_destination_id_list");

   allocation_rule = sge_pe_slots_per_host(a->pe, a->slots);
   minslots = ALLOC_RULE_IS_BALANCED(allocation_rule)?allocation_rule:1;

   /* derive suitablility of host from queues suitability */
   for_each (hep, a->host_list) {
      lSetUlong(hep, EH_seq_no, -1);
   }   

   DPRINTF(("minslots = %d\n", minslots));

   /* change host sort order */
   for_each (qep, a->queue_list) {

      if (!lGetUlong(qep, QU_tag)) {
         continue;
      }   

      /* ensure host of this queue has enough slots */
      eh_name = lGetHost(qep, QU_qhostname);
      hep = host_list_locate(a->host_list, eh_name);
      if ((int) lGetUlong(hep, EH_tagged) >= minslots && 
          (int) lGetUlong(hep, EH_seq_no) == -1) {
         lSetUlong(hep, EH_seq_no, host_seq_no++);
         DPRINTF(("%d. %s selected! %d\n", lGetUlong(hep, EH_seq_no), eh_name, lGetUlong(hep, EH_tagged)));
      } 
      else {
         DPRINTF(("%d. %s (%d tagged)\n", lGetUlong(hep, EH_seq_no), eh_name, lGetUlong(hep, EH_tagged)));
      }
   }

   max_host_seq_no = host_seq_no;

   /* find best suited master host */ 
   if (need_master_host) {
      lListElem *master_hep = NULL;
      const char *master_eh_name;

      /* find master host with the lowest host seq no */
      for_each (hep, a->host_list) {
         if (lGetUlong(hep, EH_seq_no) != -1 && lGetUlong(hep, EH_master_host)) {
            if (!master_hep || lGetUlong(hep, EH_seq_no) < lGetUlong(master_hep, EH_seq_no) ) {
               master_hep = hep;
            }
         }
      }

      /* should be impossible to reach here without a master host */
      if (!master_hep) { 
         ERROR((SGE_EVENT, MSG_SCHEDD_NOMASTERHOST_U,
            lGetUlong(a->job, JB_job_number)));
         DRETURN(MATCH_LATER);
      }

      /* change order of queues in a way causing the best suited master 
         queue of the master host to be at the first position */
      master_eh_name = lGetHost(master_hep, EH_name);
      for_each (qep, a->queue_list) {
         if (sge_hostcmp(master_eh_name, lGetHost(qep, QU_qhostname))) {
            continue;
         }   
         if (lGetUlong(qep, QU_tagged4schedule)) {
            break;
         }   
      }

      if (qep == NULL) {
         ERROR((SGE_EVENT, MSG_SCHEDD_NOMASTERQUEUE_SU, master_eh_name, 
               sge_u32c(lGetUlong(a->job, JB_job_number))));
         DRETURN(MATCH_LATER);
      }

      lDechainElem(a->queue_list, qep);
      lInsertElem(a->queue_list, NULL, qep);

      DPRINTF(("MASTER HOST %s MASTER QUEUE %s\n", 
            master_eh_name, lGetString(qep, QU_full_name)));
      /* this will cause the master host to be selected first */
      lSetUlong(master_hep, EH_seq_no, 0);
      start_seq_no = 0;
   } 
   else {
      start_seq_no = 1;
   }

   do { /* loop only needed round robin allocation rule */
      last_accu_host_slots = accu_host_slots;

      for (host_seq_no = start_seq_no; host_seq_no<max_host_seq_no; host_seq_no++) { /* iterate through hosts */
         int available, slots;
         lListElem *gdil_ep;

         if (!(hep=lGetElemUlong(a->host_list, EH_seq_no, host_seq_no))) {
            continue; /* old position of master host */
         }
         eh_name = lGetHost(hep, EH_name);

         /* how many slots to alloc in this step */
         if ((available=lGetUlong(hep, EH_tagged)) < minslots) {
            DPRINTF(("%s no more free slots at this machine\n", eh_name));
            continue; /* no more free slots at this machine */
         }
         if (allocation_rule==ALLOC_RULE_ROUNDROBIN) {
            host_slots = 1;
         } 
         else if (allocation_rule==ALLOC_RULE_FILLUP) {
            host_slots = available;
         } 
         else {
            host_slots = allocation_rule;
         }
         lSetUlong(hep, EH_tagged, available - host_slots);

         DPRINTF(("allocating %d of %d slots at host %s (seqno = %d)\n", 
               host_slots, a->slots, eh_name, host_seq_no));

         for_each (qep, a->queue_list) {
            int qtagged;

            if (sge_hostcmp(eh_name, lGetHost(qep, QU_qhostname))) {
               continue;
            }


            qname = lGetString(qep, QU_full_name);

            /* how many slots ? */
            qtagged = lGetUlong(qep, QU_tag);
            slots = MIN(a->slots-accu_host_slots, MIN(host_slots, qtagged));

            {
            /* Because of limitation rules for a sum of hosts we can not guarantee here
             * that all requests are fullfilled. A rule that might not be fullfilled is
             * for example: limit hosts * slots=2
             *
             * In the first step we tagged two slots for every host in an empty cluster. A parallel job with
             * 4 slots can not be scheduled because the first task would only get 2 slots and the
             * following tasks 0.
             * 
             * To handle the host/queue dependency in one rule we save the already granted slots in
             * a hashtable with the key "ruleset_name/rule_name@host_name" or "ruleset_name/rule_name@queue_name"
             * 
             */

            /* get rqs slots for this queue instance */
               lListElem *rqs = NULL;
               const char* user = lGetString(a->job, JB_owner);
               const char* project = lGetString(a->job, JB_project);
               const char *pe = lGetString(a->job, JB_pe);
               const char *queue = lGetString(qep, QU_qname); 
               const char *host = lGetHost(qep, QU_qhostname);

               /* first step - verify no rule that counts for a sum of 
                  hosts or queues will be violated and adjust slots */
               for_each(rqs, a->rqs_list) {
                  lListElem *rule = NULL;

                  /* ignore disabled rule sets */
                  if (!lGetBool(rqs, RQS_enabled)) {
                     continue;
                  }
                  sge_dstring_clear(&rule_name);
                  rule = rqs_get_matching_rule(rqs, user, project, pe, host, queue, a->acl_list, a->hgrp_list, &rule_name);
                  if (rule != NULL) {
                     u_long32 lir_level = lGetUlong(rule, RQR_level);
                     lListElem *jc_tmp = NULL;
                     if (lir_level != RQR_QUEUEI && jc_list != NULL) {
                        sge_dstring_append(&rule_name, "@");
                        if (lir_level == RQR_GLOBAL) {
                           sge_dstring_append(&rule_name, SGE_GLOBAL_NAME);
                        } else if (lir_level == RQR_CQUEUE) {
                           sge_dstring_append(&rule_name, queue);
                        } else {
                           sge_dstring_append(&rule_name, host);
                        }
                        jc_tmp = lGetElemStr(jc_list, JC_name, sge_dstring_get_string(&rule_name));
                     }

                     if (jc_tmp != NULL) {
                        int lslots = host_slots;
                        int lslots_qend = host_slots;
                        lList *limit_list = lGetList(rule, RQR_limit);
                        lListElem *limit = NULL;

                        for_each(limit, limit_list) {
                           bool first = true;
                           lListElem *raw_centry = centry_list_locate(a->centry_list, lGetString(limit, RQRL_name));

                           if (raw_centry == NULL) {
                              DPRINTF(("ignoring limit %s because not defined", lGetString(limit, RQRL_name)));
                              continue;
                           } 

                           if (lGetBool(raw_centry, CE_consumable)) {
                              /* found a rule, now check limit */
                              int tslots = 0;
                              int tslots_qend = 0;
                              lList *job_centry_list = lGetList(a->job, JB_hard_resource_list);

                              rqs_get_rue_string(&rue_name, rule, user, project, host, queue, pe);

                              parallel_limit_slots_by_time(a, job_centry_list, &tslots, &tslots_qend, raw_centry, limit, rue_name);
                              if (first) {
                                 first = false;
                                 lslots = tslots;
                                 lslots_qend = tslots_qend;
                              } else {
                                 lslots = MIN(lslots, tslots);
                                 lslots_qend = MIN(lslots_qend, tslots_qend);
                              }
                           }
                        }
                        /* now we have the absolute limit (lslots) for this rule and the already granted slots for this pe (jc_tmp).
                           The slots for this pe must be smaller than (tslots - jc_tmp_slots) */
                        if (slots > (lslots - lGetUlong(jc_tmp, JC_jobs))) {
                           slots = lslots - lGetUlong(jc_tmp, JC_jobs);
                        }  
                     }
                  }
               }


               /* second step - higher the granted slots for every matching rule set */
               if (slots != 0) {
                  for_each(rqs, a->rqs_list) {
                     lListElem *rule = NULL;

                     /* ignore disabled rule sets */
                     if (!lGetBool(rqs, RQS_enabled)) {
                        continue;
                     }
                     sge_dstring_clear(&rule_name);
                     rule = rqs_get_matching_rule(rqs, user, project, pe, host, queue, a->acl_list, a->hgrp_list, &rule_name);
                     if (rule != NULL) {
                        lListElem *jc_tmp = NULL;
                        u_long32 lir_level = lGetUlong(rule, RQR_level);
                        if (lir_level != RQR_QUEUEI) {
                           sge_dstring_append(&rule_name, "@");
                           if (lir_level == RQR_GLOBAL) {
                              sge_dstring_append(&rule_name, SGE_GLOBAL_NAME);
                           } else if (lir_level == RQR_CQUEUE) {
                              sge_dstring_append(&rule_name, queue);
                           } else {
                              sge_dstring_append(&rule_name, host);
                           }

                           if (jc_list == NULL) {
                              jc_list = lCreateList("", JC_Type);
                           } else {
                              jc_tmp = lGetElemStr(jc_list, JC_name, sge_dstring_get_string(&rule_name));
                           }

                           if (jc_tmp == NULL) {
                              jc_tmp = lCreateElem(JC_Type);
                              lSetString(jc_tmp, JC_name, sge_dstring_get_string(&rule_name));
                              lAppendElem(jc_list, jc_tmp);
                           }
                        }
                        DPRINTF(("book %d additional slots for %s\n", slots, sge_dstring_get_string(&rule_name)));
                        lAddUlong(jc_tmp, JC_jobs, slots);
                     }
                  }
               }
            }

            if (slots != 0) {
               accu_host_slots += slots;
               host_slots -= slots;
               total_soft_violations += slots * lGetUlong(qep, QU_soft_violation);

               /* build gdil for that queue */
               DPRINTF((sge_u32": %d slots in queue %s user %s (host_slots = %d)\n", 
                  a->job_id, slots, qname, lGetString(a->job, JB_owner), host_slots));

               if (!(gdil_ep=lGetElemStr(gdil, JG_qname, qname))) {
                  gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
                  lSetUlong(gdil_ep, JG_qversion, lGetUlong(qep, QU_version));
                  lSetHost(gdil_ep, JG_qhostname, eh_name);
                  lSetUlong(gdil_ep, JG_slots, slots);
               } 
               else {
                  lSetUlong(gdil_ep, JG_slots, lGetUlong(gdil_ep, JG_slots) + slots);
               }   

               /* untag */
               lSetUlong(qep, QU_tag, qtagged - slots);
            }

            if (!host_slots) {
               break; 
            }   
         }
      }

      DPRINTF(("- - - accu_host_slots %d total_slots %d\n", accu_host_slots, a->slots));
      if (last_accu_host_slots == accu_host_slots) {
         DPRINTF(("!!! NO MORE SLOTS !!!\n"));
         lFreeList(&jc_list);
         sge_dstring_free(&rule_name);
         sge_dstring_free(&rue_name);
         lFreeList(&gdil); 
         DRETURN(MATCH_LATER);
      }
   } while (allocation_rule == ALLOC_RULE_ROUNDROBIN && accu_host_slots < a->slots);

   /* because of the limitation rule sets it's could be not all requested slots could be assigned. */
   if (accu_host_slots < a->slots) {
         DPRINTF(("!!! NOT ENOUGH SLOTS !!!\n"));
         lFreeList(&jc_list);
         sge_dstring_free(&rule_name);
         sge_dstring_free(&rue_name);
         lFreeList(&gdil); 
         DRETURN(MATCH_LATER);
   }

   lFreeList(&jc_list);
   sge_dstring_free(&rule_name);
   sge_dstring_free(&rue_name);
   lFreeList(&(a->gdil));
   a->gdil = gdil;
   a->soft_violations = total_soft_violations;

   DRETURN(0);
}



/****** sge_select_queue/parallel_sort_suitable_queues() ****************************
*  NAME
*     parallel_sort_suitable_queues() -- Sort queues according queue sort method
*
*  SYNOPSIS
*     static int parallel_sort_suitable_queues(lList *queue_list) 
*
*  FUNCTION
*     The passed queue list gets sorted according the queue sort method.
*     Before this can happen the following queue fields must contain
*     related values:
*
*     o number of violations of the jobs soft request in QU_soft_violation
*     o sequence number of the host in the sorted host list in QU_host_seq_no
*     o number of tagged slots in the queue in QU_tag - we prefer queues 
*       with many slots because we dont want to distribute the slots to 
*       multiple queues without a need. If this is not convenient it is possible 
*       to use the queues sequence numbers to override this behaviour 
*       irrespecitive of the queue sort method.
*     o the queue sequence number in QU_seq_no $
*  
*     The valency of these fields depends on the queue sort order 
* 
*       QSM_LOAD
*       QSM_SHARE
*          1. QU_soft_violation
*          2. QU_host_seq_no 
*          3. QU_seq_no 
*          4. QU_tag
* 
*        QSM_SEQNUM
*          1. QU_soft_violation
*          2. QU_seq_no 
*          3. QU_host_seq_no 
*          4. QU_tag
*
*  INPUTS
*     lList *queue_list - The queue list that gets sorted (QU_Type)
*
*  RESULT
*     static int - 0 on succes 
*                 -1 on error
*  NOTES
*     MT-NOTE: parallel_sort_suitable_queues() is not MT safe 
*
*******************************************************************************/
static int parallel_sort_suitable_queues(lList *queue_list)
{
   u_long32 qsm = sconf_get_queue_sort_method();
  
   DENTER(TOP_LAYER, "parallel_sort_suitable_queues");

   /* Don't do this when the code is used only for dry runs to check whether
      assignments would be possible */
   if (sconf_get_qs_state() == QS_STATE_EMPTY) {
      DRETURN(0);
   }

   if (qsm == QSM_LOAD) {
      lPSortList(queue_list, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_host_seq_no, QU_seq_no, QU_tag);
   }   
   else {
      lPSortList(queue_list, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_seq_no, QU_host_seq_no, QU_tag);
   }   

   DRETURN(0);
}

/****** sched/select_queue/parallel_queue_slots() *************************
*  NAME
*     parallel_queue_slots() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_NOW and 
*             DISPATCH_TIME_QUEUE_END (only with fixed_slot equals true)
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
static dispatch_t
parallel_queue_slots(sge_assignment_t *a,lListElem *qep, int *slots, int *slots_qend, 
                    int *violations, bool allow_non_requestable) 
{
   lList *hard_requests = lGetList(a->job, JB_hard_resource_list);
   lList *config_attr = lGetList(qep, QU_consumable_config_list);
   lList *actual_attr = lGetList(qep, QU_resource_utilization);
   const char *qname = lGetString(qep, QU_full_name);
   int qslots = 0, qslots_qend = 0;
   int lslots = 0, lslots_qend = 0;

   dispatch_t result = DISPATCH_NEVER_CAT;
   bool is_reset = false; /* This function computes future slots and current slots
                             in one run. If the calendar returns later, we have to
                             compute the slots, and reset the current slots to 0, even
                             so the resurces are available, when the queue would not 
                             have been cal disabled*/

   DENTER(TOP_LAYER, "parallel_queue_slots");

   if (sge_queue_match_static(qep, a->job, a->pe, a->ckpt, a->centry_list, a->acl_list, a->hgrp_list) == DISPATCH_OK) {

      result = parallel_rqs_slots_by_time(a, &lslots, &lslots_qend, lGetHost(qep, QU_qhostname), lGetString(qep, QU_qname));

      if (result == DISPATCH_OK) {
         result = parallel_rc_slots_by_time(a, hard_requests, &qslots, &qslots_qend, 
               config_attr, actual_attr, NULL, true, qep, 
               DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG, false, lGetString(qep, QU_full_name));

         if (is_reset) {
            qslots = 0;
         }
      }

      if (violations != NULL) {
         *violations = compute_soft_violations(a, qep, *violations, NULL, config_attr, 
                                           actual_attr, DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG);
      }
   }

   *slots = MIN(qslots, lslots);
   *slots_qend = MIN(qslots_qend, lslots_qend);

   if (result == DISPATCH_OK) {
      DPRINTF(("\tparallel_queue_slots(%s) returns %d/%d\n", qname, qslots, qslots_qend));
   } 
   else {
      DPRINTF(("\tparallel_queue_slots(%s) returns <error>\n", qname));
   }

   DRETURN(result);
}

/****** sched/select_queue/sequential_queue_time() *************************
*  NAME
*     sequential_queue_time() -- 
*
*  RESULT
*      dispatch_t - 0 ok got an assignment + set time for DISPATCH_TIME_NOW and 
*                     DISPATCH_TIME_QUEUE_END (only with fixed_slot equals true)
*                   1 no assignment at the specified time
*                  -1 assignment will never be possible for all jobs of that category
******************************************************************************/
static dispatch_t 
sequential_queue_time(u_long32 *start, const sge_assignment_t *a,
                                int *violations, lListElem *qep) 
{
   dstring reason; 
   char reason_buf[1024];
   dispatch_t result;
   u_long32 tmp_time = *start;
   lList *hard_requests = lGetList(a->job, JB_hard_resource_list);
   lList *config_attr = lGetList(qep, QU_consumable_config_list);
   lList *actual_attr = lGetList(qep, QU_resource_utilization);
   const char *qname = lGetString(qep, QU_full_name);

   DENTER(TOP_LAYER, "queue_time_by_slots");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));

   /* match the resources */
   result = rc_time_by_slots(a, hard_requests, NULL, config_attr, actual_attr, 
                            qep, false, &reason, 1, DOMINANT_LAYER_QUEUE, 
                            0, QUEUE_TAG, &tmp_time, qname);

   if (result == DISPATCH_OK) {
      if (violations != NULL) {
         *violations = compute_soft_violations(a, qep, *violations, NULL, config_attr, actual_attr, 
                                           DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG);
      }      
   } else {
      char buff[1024 + 1];
      centry_list_append_to_string(hard_requests, buff, sizeof(buff) - 1);
      if (*buff && (buff[strlen(buff) - 1] == '\n')) {
         buff[strlen(buff) - 1] = 0;
      }   
      schedd_mes_add(lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, buff, qname, reason_buf);
   }

   if (a->is_reservation && result == DISPATCH_OK) {
      *start = tmp_time;
      DPRINTF(("queue_time_by_slots(%s) returns earliest start time "sge_u32"\n", qname, *start));
   } else if (result == DISPATCH_OK) {
      DPRINTF(("queue_time_by_slots(%s) returns <at specified time>\n", qname));
   } else {
      DPRINTF(("queue_time_by_slots(%s) returns <later>\n", qname));
   }

   DRETURN(result); 
}


/****** sge_select_queue/parallel_host_slots() ******************************
*  NAME
*     parallel_host_slots() -- Return host slots available at time period
*
*  SYNOPSIS
*  FUNCTION
*     The maximum amount available at the host for the specified time period
*     is determined. 
*
*
*  INPUTS
*
*  RESULT
*******************************************************************************/
static dispatch_t 
parallel_host_slots(sge_assignment_t *a, int *slots, int *slots_qend, int *host_soft_violations,
                   lListElem *hep, bool allow_non_requestable) {

   int hslots = 0, hslots_qend = 0;
   const char *eh_name;
   dispatch_t result = DISPATCH_NEVER_CAT;
   lList *hard_requests = lGetList(a->job, JB_hard_resource_list);
   lList *load_list = lGetList(hep, EH_load_list); 
   lList *config_attr = lGetList(hep, EH_consumable_config_list);
   lList *actual_attr = lGetList(hep, EH_resource_utilization);
   double lc_factor = 0;

   DENTER(TOP_LAYER, "parallel_host_slots");

   eh_name = lGetHost(hep, EH_name);

   clear_resource_tags(hard_requests, HOST_TAG);

   if (sge_host_match_static(a->job, a->ja_task, hep, a->centry_list, a->acl_list) == DISPATCH_OK) {

      /* cause load be raised artificially to reflect load correction when
         checking job requests */
      if (lGetPosViaElem(hep, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
         u_long32 ulc_factor;
         if ((ulc_factor=lGetUlong(hep, EH_load_correction_factor))) {
            lc_factor = ((double)ulc_factor)/100;
         }      
      }

      result = parallel_rc_slots_by_time(a, hard_requests, &hslots, &hslots_qend, 
            config_attr, actual_attr, load_list, false, NULL, 
               DOMINANT_LAYER_HOST, lc_factor, HOST_TAG, false, lGetHost(hep, EH_name));

      if (hslots > 0) {
         int t_max = parallel_max_host_slots(a, hep);
         if (t_max<hslots) {
            DPRINTF(("\tparallel_host_slots(%s) threshold load adjustment reduces slots"
                  " from %d to %d\n", eh_name, hslots, t_max));
            hslots = t_max;
         }
      }
   }

   *slots = hslots;
   *slots_qend = hslots_qend;

   if (host_soft_violations != NULL) {
      *host_soft_violations= compute_soft_violations(a, NULL,*host_soft_violations, load_list, config_attr, 
                                                 actual_attr, DOMINANT_LAYER_HOST, 0, HOST_TAG);
   }      

   if (result == DISPATCH_OK) {
      DPRINTF(("\tparallel_host_slots(%s) returns %d/%d\n", eh_name, hslots, hslots_qend));
   } 
   else {
      DPRINTF(("\tparallel_host_slots(%s) returns <error>\n", eh_name));
   }

   DRETURN(result); 
}



/****** sge_select_queue/host_time_by_slots() ******************************
*  NAME
*     host_time_by_slots() -- Return time when host slots are available
*
*  SYNOPSIS
*     int host_time_by_slots(int slots, u_long32 *start, u_long32 duration, 
*     int *host_soft_violations, lListElem *job, lListElem *ja_task, lListElem 
*     *hep, lList *centry_list, lList *acl_list) 
*
*  FUNCTION
*     The time when the specified slot amount is available at the host 
*     is determined. Behaviour depends on input/output parameter start
*
*     DISPATCH_TIME_NOW 
*           0 an assignment is possible now
*           1 no assignment now but later
*          -1 assignment never possible for all jobs of the same category
*          -2 assignment never possible for that particular job
*
*     <any other time>
*           0 an assignment is possible at the specified time
*           1 no assignment at specified time but later
*          -1 assignment never possible for all jobs of the same category
*          -2 assignment never possible for that particular job
*
*     DISPATCH_TIME_QUEUE_END
*           0 an assignment is possible and the start time is returned
*          -1 assignment never possible for all jobs of the same category
*          -2 assignment never possible for that particular job
*
*  INPUTS
*     int slots                 - ??? 
*     u_long32 *start           - ??? 
*     u_long32 duration         - ??? 
*     int *host_soft_violations - ??? 
*     lListElem *job            - ??? 
*     lListElem *ja_task        - ??? 
*     lListElem *hep            - ??? 
*     lList *centry_list        - ??? 
*     lList *acl_list           - ??? 
*
*  RESULT
*******************************************************************************/
static dispatch_t
sequential_host_time(u_long32 *start, const sge_assignment_t *a,
                              int *violations, lListElem *hep) 
{
   lList *hard_requests = lGetList(a->job, JB_hard_resource_list);
   lList *load_attr = lGetList(hep, EH_load_list); 
   lList *config_attr = lGetList(hep, EH_consumable_config_list);
   lList *actual_attr = lGetList(hep, EH_resource_utilization);
   double lc_factor = 0;
   u_long32 ulc_factor;
   dispatch_t result;
   u_long32 tmp_time = *start;
   const char *eh_name = lGetHost(hep, EH_name);
   dstring reason; char reason_buf[1024];
  
   DENTER(TOP_LAYER, "host_time_by_slots");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));

   clear_resource_tags(hard_requests, HOST_TAG);

   if ((result=sge_host_match_static(a->job, a->ja_task, hep, a->centry_list, a->acl_list))) {
      DRETURN(result);
   }

   /* cause load be raised artificially to reflect load correction when
      checking job requests */
   if (lGetPosViaElem(hep, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
      if ((ulc_factor=lGetUlong(hep, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   }

   result = rc_time_by_slots(a, hard_requests, load_attr, 
         config_attr, actual_attr, NULL, false, 
         &reason, 1, DOMINANT_LAYER_HOST, 
         lc_factor, HOST_TAG, &tmp_time, eh_name);

   if (result == DISPATCH_OK || result == DISPATCH_MISSING_ATTR) {
      if (violations != NULL) {
         *violations = compute_soft_violations(a, NULL, *violations, load_attr, config_attr, 
                                           actual_attr, DOMINANT_LAYER_HOST, 0, HOST_TAG);
      }      
   } else {
      char buff[1024 + 1];
      centry_list_append_to_string(hard_requests, buff, sizeof(buff) - 1);
      if (*buff && (buff[strlen(buff) - 1] == '\n'))
         buff[strlen(buff) - 1] = 0;
      schedd_mes_add(lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNATHOST_SSS, buff, eh_name, reason_buf);
   }

   if (a->is_reservation && (result == DISPATCH_OK || result == DISPATCH_MISSING_ATTR)) {
      *start = tmp_time;
      DPRINTF(("host_time_by_slots(%s) returns earliest start time "sge_u32"\n", eh_name, *start));
   } else if (result == DISPATCH_OK || result == DISPATCH_MISSING_ATTR) {
      DPRINTF(("host_time_by_slots(%s) returns <at specified time>\n", eh_name));
   } else {
      DPRINTF(("host_time_by_slots(%s) returns <later>\n", eh_name));
   }

   DRETURN(result); 
}

/****** sched/select_queue/sequential_global_time() ***************************
*  NAME
*     sequential_global_time() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_QUEUE_END
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
static dispatch_t
sequential_global_time(u_long32 *start, const sge_assignment_t *a, int *violations)
{
   dstring reason; char reason_buf[1024];
   dispatch_t result = DISPATCH_NEVER_CAT;
   u_long32 tmp_time = *start; 
   lList *hard_request = lGetList(a->job, JB_hard_resource_list);
   lList *load_attr = lGetList(a->gep, EH_load_list); 
   lList *config_attr = lGetList(a->gep, EH_consumable_config_list);
   lList *actual_attr = lGetList(a->gep, EH_resource_utilization);
   double lc_factor=0.0;
   u_long32 ulc_factor;

   DENTER(TOP_LAYER, "global_time_by_slots");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));

   clear_resource_tags(hard_request, GLOBAL_TAG);

   /* check if job has access to any hosts globally */
   if ((result=sge_host_match_static(a->job, NULL, a->gep, a->centry_list, 
            a->acl_list)) != 0) {
      DRETURN(result);
   }
   
   /* cause global load be raised artificially to reflect load correction when
      checking job requests */
   if (lGetPosViaElem(a->gep, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
      if ((ulc_factor=lGetUlong(a->gep, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   }

   result = rc_time_by_slots(a, hard_request, load_attr, config_attr, actual_attr, NULL, false, &reason, 
                             1, DOMINANT_LAYER_GLOBAL, lc_factor, GLOBAL_TAG, &tmp_time, SGE_GLOBAL_NAME);

   if ((result == DISPATCH_OK) || (result == DISPATCH_MISSING_ATTR)) {
      if (violations != NULL) {
         *violations = compute_soft_violations(a, NULL, *violations, load_attr, config_attr, 
                                           actual_attr, DOMINANT_LAYER_GLOBAL, 0, GLOBAL_TAG);
      }      
   } else {
      char buff[1024 + 1];
      centry_list_append_to_string(hard_request, buff, sizeof(buff) - 1);
      if (*buff && (buff[strlen(buff) - 1] == '\n')) {
         buff[strlen(buff) - 1] = 0;
      }   
      schedd_mes_add (lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNGLOBALLY_SS,
                      buff, reason_buf);
   }

   if (a->is_reservation && (result == DISPATCH_OK || result == DISPATCH_MISSING_ATTR)) {
      *start = tmp_time;
      DPRINTF(("global_time_by_slots() returns earliest start time "sge_u32"\n", *start));
   } 
   else if (result == DISPATCH_OK || result == DISPATCH_MISSING_ATTR) {
      DPRINTF(("global_time_by_slots() returns <at specified time>\n"));
   } 
   else {
      DPRINTF(("global_time_by_slots() returns <later>\n"));
   }

   DRETURN(result);
}

/****** sched/select_queue/parallel_global_slots() ***************************
*  NAME
*     parallel_global_slots() -- 
*
*  RESULT
*     dispatch_t -  0 ok got an assignment + set time for DISPATCH_TIME_QUEUE_END
*                   1 no assignment at the specified time
*                  -1 assignment will never be possible for all jobs of that category
******************************************************************************/
static dispatch_t 
parallel_global_slots(const sge_assignment_t *a, int *slots, int *slots_qend, int *violations)
{
   dispatch_t result = DISPATCH_NEVER_CAT;
   lList *hard_request = lGetList(a->job, JB_hard_resource_list);
   lList *load_attr = lGetList(a->gep, EH_load_list); 
   lList *config_attr = lGetList(a->gep, EH_consumable_config_list);
   lList *actual_attr = lGetList(a->gep, EH_resource_utilization);
   double lc_factor=0.0;
   int gslots = 0, gslots_qend = 0;

   DENTER(TOP_LAYER, "parallel_global_slots");

   clear_resource_tags(hard_request, GLOBAL_TAG);

   /* check if job has access to any hosts globally */
   if (sge_host_match_static(a->job, NULL, a->gep, a->centry_list, a->acl_list) == DISPATCH_OK) {
      /* cause global load be raised artificially to reflect load correction when
         checking job requests */

      if (lGetPosViaElem(a->gep, EH_load_correction_factor, SGE_NO_ABORT) >= 0) {
         u_long32 ulc_factor;
         if ((ulc_factor=lGetUlong(a->gep, EH_load_correction_factor))) {
            lc_factor = ((double)ulc_factor)/100;
         }
      }

      result = parallel_rc_slots_by_time(a, hard_request, &gslots, 
                                &gslots_qend, config_attr, actual_attr, load_attr,  
                                false, NULL, DOMINANT_LAYER_GLOBAL, lc_factor, GLOBAL_TAG, false, SGE_GLOBAL_NAME);
   }

   *slots      = gslots;
   *slots_qend = gslots_qend;

   if (violations != NULL) {
      *violations = compute_soft_violations(a, NULL, *violations, load_attr, config_attr, 
                                        actual_attr, DOMINANT_LAYER_GLOBAL, 0, GLOBAL_TAG);
   }

   if (result == DISPATCH_OK) {
      DPRINTF(("\tparallel_global_slots() returns %d/%d\n", gslots, gslots_qend));
   } 
   else {
      DPRINTF(("\tparallel_global_slots() returns <error>\n"));
   }

   DRETURN(result);
}

/****** sge_select_queue/parallel_available_slots() **********************************
*  NAME
*     parallel_available_slots() -- Check if number of PE slots is available 
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*
*  RESULT
*     dispatch_t - 0 ok got an assignment
*                  1 no assignment at the specified time
*                 -1 assignment will never be possible for all jobs of that category
*
*  NOTES
*     MT-NOTE: parallel_available_slots() is not MT safe 
*******************************************************************************/
static dispatch_t 
parallel_available_slots(const sge_assignment_t *a, int *slots, int *slots_qend) 
{
   dstring reason; 
   char reason_buf[1024];
   dispatch_t result;
   int total = lGetUlong(a->pe, PE_slots);
   int pslots=0; 
   int pslots_qend=0;
   static lListElem *implicit_slots_request = NULL;
   static lList *implicit_total_list = NULL;
   lListElem *tep = NULL;
   char strbuf[100];
   dstring slots_as_str;

   DENTER(TOP_LAYER, "parallel_available_slots");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));
   
   if ((result=pe_match_static(a->job, a->pe, a->acl_list)) != DISPATCH_OK) {
      DRETURN(result);
   }

   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, SGE_ATTR_SLOTS);
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* PE slots should be stored in a PE_consumable_config_list ... */
   if (!implicit_total_list) {
      tep = lAddElemStr(&implicit_total_list, CE_name, SGE_ATTR_SLOTS, CE_Type);
   }
   
   if (!tep && !(tep = lGetElemStr(implicit_total_list, CE_name, SGE_ATTR_SLOTS))) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   total = lGetUlong(a->pe, PE_slots);
   lSetDouble(tep, CE_doubleval, total);
   sge_dstring_init(&slots_as_str, strbuf, sizeof(strbuf));
   sge_dstring_sprintf(&slots_as_str, "%d", total);
   lSetString(tep, CE_stringval, strbuf);

   if (ri_slots_by_time(a, &pslots, &pslots_qend, 
         lGetList(a->pe, PE_resource_utilization), implicit_slots_request, 
         NULL, implicit_total_list, NULL, 0, 0, &reason, true, true,
         lGetString(a->pe, PE_name))) {
      DRETURN(DISPATCH_NEVER_CAT);
   }

   if (slots) {
      *slots = pslots;
   }

   if (slots_qend) {
      *slots_qend = pslots_qend;
   }   

   DPRINTF(("\tparallel_available_slots(%s) returns %d/%d\n", lGetString(a->pe, PE_name),
         pslots, pslots_qend));

   DRETURN(DISPATCH_OK);
}


/* ----------------------------------------

   sge_get_double_qattr() 

   writes actual value of the queue attriute into *uvalp 

   returns:
      0 ok, value in *uvalp is valid
      -1 the queue has no such attribute
      -2 type error: cant compute uval from actual string value 

*/      
int 
sge_get_double_qattr(double *dvalp, char *attrname, lListElem *q, 
                     const lList *exechost_list, const lList *centry_list, 
                     bool *has_value_from_object) 
{
   int ret = -1;
   lListElem *ep;
   u_long32 type;
   double tmp_dval;
   char dom_str[4];
   lListElem *global = NULL;
   lListElem *host = NULL;

   DENTER(TOP_LAYER, "sge_get_double_qattr");

   global = host_list_locate(exechost_list, SGE_GLOBAL_NAME); 
   host = host_list_locate(exechost_list, lGetHost(q, QU_qhostname));

   /* find matching */
   *has_value_from_object = false;
   if (( ep = get_attribute_by_name(global, host, q, attrname, centry_list, DISPATCH_TIME_NOW, 0)) &&
         ((type=lGetUlong(ep, CE_valtype)) != TYPE_STR) && 
         (type != TYPE_CSTR) && (type != TYPE_RESTR) && (type != TYPE_HOST) ) {
         
         if ((lGetUlong(ep, CE_pj_dominant)&DOMINANT_TYPE_MASK)!=DOMINANT_TYPE_VALUE ) {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_pj_stringval), NULL, 0);
            monitor_dominance(dom_str, lGetUlong(ep, CE_pj_dominant));
            *has_value_from_object = true;
         } else {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_stringval), NULL, 0); 
            monitor_dominance(dom_str, lGetUlong(ep, CE_dominant));
            *has_value_from_object = ((lGetUlong(ep, CE_dominant) & DOMINANT_TYPE_MASK) == DOMINANT_TYPE_VALUE) ? false : true;
         }
      ret = 0;
      if (dvalp)
         *dvalp = tmp_dval;
      DPRINTF(("resource %s: %f\n", dom_str, tmp_dval));
   }

   /* free */
   lFreeElem(&ep);

   DRETURN(ret);
}


/* ----------------------------------------

   sge_get_string_qattr() 

   writes string value into dst

   returns:
      -1    if the queue has no such attribute
      0 
*/      
int sge_get_string_qattr(
char *dst,
int dst_len,
char *attrname,
lListElem *q,
const lList *exechost_list,
const lList *centry_list 
) {
   lListElem *ep;
   lListElem *global = NULL;
   lListElem *host = NULL;
   int ret = 0;

   DENTER(TOP_LAYER, "sge_get_string_qattr");

   global = host_list_locate(exechost_list, SGE_GLOBAL_NAME); 
   host = host_list_locate(exechost_list, lGetHost(q, QU_qhostname));

   ep = get_attribute_by_name(global, host, q, attrname, centry_list, DISPATCH_TIME_NOW, 0);

   /* first copy ... */
   if (ep && dst)
      sge_strlcpy(dst, lGetString(ep, CE_stringval), dst_len);

   if(ep){
      lFreeElem(&ep);
      ret = 0;
   }
   else
      ret = -1;
   /* ... and then free */

   DRETURN(ret); 
}

/****** sge_select_queue/ri_time_by_slots() ******************************************
*  NAME
*     ri_time_by_slots() -- Determine availability time through slot number
*
*  SYNOPSIS
*     static int ri_time_by_slots(lListElem *rep, lList *load_attr, lList 
*     *config_attr, lList *actual_attr, lList *centry_list, lListElem *queue, 
*     char *reason, int reason_size, bool allow_non_requestable, int slots, 
*     u_long32 layer, double lc_factor) 
*
*  FUNCTION
*     Checks for one level, if one request is fulfilled or not. 
* 
*     With reservation scheduling the earliest start time due to 
*     availability of the resource instance is determined by ensuring 
*     non-consumable resource requests are fulfilled or by finding the 
*     earliest time utilization of a consumable resource is below the
*     threshold required for the request.
*
*  INPUTS
*     sge_assignment_t *a       - assignment object that holds job specific scheduling relevant data
*     lListElem *rep            - requested attribute 
*     lList *load_attr          - list of load attributes or null on queue level 
*     lList *config_attr        - list of user defined attributes (CE_Type)
*     lList *actual_attr        - usage of user consumables (RUE_Type)
*i    lListElem *queue          - the current queue, or null on host level 
*     dstring *reason           - target for error message 
*     bool allow_non_requestable - allow none requestable attributes? 
*     int slots                 - the number of slotes the job is looking for? 
*     u_long32 layer            - the current layer 
*     double lc_factor          - load correction factor 
*     u_long32 *start_time      - in/out argument for start time  
*     const char *object_name   - name of the object used for monitoring purposes
*
*  RESULT
*     dispatch_t - 
*
*******************************************************************************/
static dispatch_t
ri_time_by_slots(const sge_assignment_t *a, lListElem *rep, lList *load_attr, lList *config_attr, lList *actual_attr, 
                 lListElem *queue, dstring *reason, bool allow_non_requestable, 
                 int slots, u_long32 layer, double lc_factor, u_long32 *start_time, const char *object_name) 
{
   lListElem *cplx_el=NULL;
   const char *attrname; 
   dispatch_t ret = DISPATCH_OK;
   lListElem *actual_el;
   u_long32 ready_time;
   double util, total, request = 0;
   lListElem *capacitiy_el;
   bool schedule_based;
   u_long32 now = sconf_get_now();

   DENTER(TOP_LAYER, "ri_time_by_slots");

   attrname = lGetString(rep, CE_name);
   actual_el = lGetElemStr(actual_attr, RUE_name, attrname);
   ready_time = *start_time;

   /*
    * Consumables are treated futher below in schedule based mode 
    * thus we always assume zero consumable utilization here 
    */ 

   /* We need to dispatch scheduled based, when we have the reservation enabled. Therefore the
      duration will be always bigger than 0. But this code is also called from the qmaster to 
      verify, if a job can run. In this case, reservation can be enabled, but the verifcation is
      not scheduled based. For this case, we need this test.
   */ 
   schedule_based = (a->duration != 0 && sconf_get_max_reservations() > 0) ? true : false;

   if (!(cplx_el = get_attribute(attrname, config_attr, actual_attr, load_attr, a->centry_list, queue,layer, 
                        lc_factor, reason, schedule_based, DISPATCH_TIME_NOW, 0))) {
      DRETURN(DISPATCH_MISSING_ATTR);

   }

   ret = match_static_resource(slots, rep, cplx_el, reason, false, false, allow_non_requestable);
   if (ret != DISPATCH_OK || !schedule_based) {
      lFreeElem(&cplx_el);
      DRETURN(ret);
   }

   DPRINTF(("ri_time_by_slots(%s) consumable = %s\n", 
         attrname, (lGetBool(cplx_el, CE_consumable)?"true":"false")));

   if (!lGetBool(cplx_el, CE_consumable)) {
      if (ready_time == DISPATCH_TIME_QUEUE_END) {
         *start_time = now;
      }   
      DPRINTF(("%s: ri_time_by_slots(%s) <is no consumable>\n", object_name, attrname));
      lFreeElem(&cplx_el);
      DRETURN(DISPATCH_OK); /* already checked */
   }
      
   /* we're done if there is no consumable capacity */
   if (!(capacitiy_el = lGetElemStr(config_attr, CE_name, attrname))) {
      DPRINTF(("%s: ri_time_by_slots(%s) <does not exist>\n", object_name, attrname));
      lFreeElem(&cplx_el);
      DRETURN(DISPATCH_MISSING_ATTR); /* does not exist */
   }
      
   /* determine 'total' and 'request' values */
   total = lGetDouble(capacitiy_el, CE_doubleval);

   if (!parse_ulong_val(&request, NULL, lGetUlong(cplx_el, CE_valtype), 
      lGetString(rep, CE_stringval), NULL, 0)) {
      sge_dstring_append(reason, "wrong type");
      lFreeElem(&cplx_el);
      DRETURN(DISPATCH_NEVER_CAT);
   }
   lFreeElem(&cplx_el);

   if (ready_time == DISPATCH_TIME_QUEUE_END) {
      double threshold = total - request * slots;

      /* verify there are enough resources in principle */
      if (threshold < 0) {
         ret = DISPATCH_NEVER_CAT;
      }   
      else {
         /* seek for the time near queue end where resources are sufficient */
         u_long32 when = utilization_below(actual_el, threshold, object_name);
         if (when == 0) {
            /* may happen only if scheduler code is run outside scheduler with 
               DISPATCH_TIME_QUEUE_END time spec */
            *start_time = now;
         } 
         else {
            *start_time = when;
         }   
         ret = DISPATCH_OK;

         DPRINTF(("\t\t%s: time_by_slots: %d of %s=%f can be served %s\n", 
               object_name, slots, attrname, request,
                  ret == DISPATCH_OK ? "at time" : "never"));

      }
      DRETURN(ret);
   } 

   /* here we handle DISPATCH_TIME_NOW + any other time */
   if (*start_time == DISPATCH_TIME_NOW) {
      ready_time = now;
   }   
   else {
      ready_time = *start_time;
   }   

   util = utilization_max(actual_el, ready_time, a->duration);

   DPRINTF(("\t\t%s: time_by_slots: %s total = %f util = %f from "sge_u32" plus "sge_u32" seconds\n", 
            object_name, attrname, total, util, ready_time, a->duration));

   /* ensure resource is sufficient from now until finish */
   if (request * slots > total - util) {
      char dom_str[5];
      dstring availability; char availability_text[2048];

      sge_dstring_init(&availability, availability_text, sizeof(availability_text));
      
      /* we can't assign right now - maybe later ? */  
      if (request * slots > total ) {
         DPRINTF(("\t\t%s: time_by_slots: %s %f > %f (never)\n", object_name, attrname,
            request * slots, total));
         ret = DISPATCH_NEVER_CAT; /* surely not */
      } 
      else if (request * slots > total - utilization_queue_end(actual_el)) {
         DPRINTF(("\t\t%s: time_by_slots: %s %f <= %f (but booked out!!)\n", object_name, attrname,
            request * slots, total));
         ret = DISPATCH_NEVER_CAT; /* booked out until infinity */
      } 
      else {
         DPRINTF(("\t\t%s: time_by_slots: %s %f > %f (later)\n", object_name, attrname, 
            request * slots, total - util));
         ret = DISPATCH_NOT_AT_TIME;
      }

      monitor_dominance(dom_str, DOMINANT_TYPE_CONSUMABLE | layer);
      sge_dstring_sprintf(&availability, "%s:%s=%f", dom_str, attrname, total - util);
      sge_dstring_append(reason, MSG_SCHEDD_ITOFFERSONLY);
      sge_dstring_append(reason, availability_text);

      if ((a->duration != DISPATCH_TIME_NOW) &&
          (request * slots <= total - utilization_max(actual_el, ready_time, DISPATCH_TIME_NOW))) {
         sge_dstring_append(reason, MSG_SCHEDD_DUETORR);
      }
   } 
   else {
      ret = DISPATCH_OK;
   }

   DPRINTF(("\t\t%s: time_by_slots: %d of %s=%f can be served %s\n", 
         object_name, slots, attrname, request, 
            ret == DISPATCH_OK ? "at time" : ((ret == DISPATCH_NOT_AT_TIME)? "later":"never")));

   DRETURN(ret);                                                       
}

/****** sge_select_queue/ri_slots_by_time() ************************************
*  NAME
*     ri_slots_by_time() -- Determine number of slots avail. within time frame
*
*  SYNOPSIS
*     static dispatch_t ri_slots_by_time(const sge_assignment_t *a, int *slots, 
*     int *slots_qend, lList *rue_list, lListElem *request, lList *load_attr, 
*     lList *total_list, lListElem *queue, u_long32 layer, double lc_factor, 
*     dstring *reason, bool allow_non_requestable, bool no_centry, const char 
*     *object_name) 
*
*  FUNCTION
*     The number of slots available with a resource can be zero for static
*     resources or is determined based on maximum utilization within the
*     specific time frame, the total amount of the resource and the per
*     task request of the parallel job (ri_slots_by_time())
*
*  INPUTS
*     const sge_assignment_t *a  - ??? 
*     int *slots                 - Returns maximum slots that can be served 
*                                  within the specified time frame.
*     int *slots_qend            - Returns the maximum possible number of slots
*     lList *rue_list            - Resource utilization (RUE_Type)
*     lListElem *request         - Job request (CE_Type)
*     lList *load_attr           - Load information for the resource
*     lList *total_list          - Total resource amount (CE_Type)
*     lListElem *queue           - Queue instance (QU_Type) for queue-based resources
*     u_long32 layer             - DOMINANT_LAYER_{GLOBAL|HOST|QUEUE}
*     double lc_factor           - load correction factor
*     dstring *reason            - diagnosis information if no rsrc available
*     bool allow_non_requestable - ??? 
*     bool no_centry             - ??? 
*     const char *object_name    - ??? 
*
*  RESULT
*     static dispatch_t - 
*
*  NOTES
*     MT-NOTE: ri_slots_by_time() is not MT safe 
*******************************************************************************/
static dispatch_t
ri_slots_by_time(const sge_assignment_t *a, int *slots, int *slots_qend, 
   lList *rue_list, lListElem *request, lList *load_attr, lList *total_list, 
   lListElem *queue, u_long32 layer, double lc_factor, dstring *reason, 
   bool allow_non_requestable, bool no_centry, const char *object_name) 
{
   const char *name;
   lListElem *cplx_el, *uep, *tep = NULL;
   u_long32 start = a->start;

   /* always assume zero consumable utilization in schedule based mode */

   /* We need to desipatch scheduled based, when we have the reservation enabled. Therefore the
      duration will be always bigger than 0. But this code is also called from the qmaster to 
      verify, if a job can run. In this case, reservation can be enabled, but the verifcation is
      not scheduled based. For this case, we need this test.
   */ 
   bool schedule_based = (a->duration != 0 && sconf_get_max_reservations()>0) ? true : false;

   dispatch_t ret;
   double used, total, request_val;

   DENTER(TOP_LAYER, "ri_slots_by_time");

   name = lGetString(request, CE_name);
   uep = lGetElemStr(rue_list, RUE_name, name);

   DPRINTF(("\t\t%s: ri_slots_by_time(%s)\n", object_name, name));

   if (!no_centry) {
      if (!(cplx_el = get_attribute(name, total_list, rue_list, load_attr, a->centry_list, queue,layer, 
                           lc_factor, reason, schedule_based, DISPATCH_TIME_NOW, 0))) {
         DRETURN(DISPATCH_MISSING_ATTR); /* does not exist */
      }

      ret = match_static_resource(1, request, cplx_el, reason, false, false, allow_non_requestable);
      if (ret != DISPATCH_OK) {
         lFreeElem(&cplx_el);
         DRETURN(ret);
      }

      if (ret == DISPATCH_OK && !lGetBool(cplx_el, CE_consumable)) {
         lFreeElem(&cplx_el);
         *slots      = INT_MAX;
         *slots_qend = INT_MAX;
         DRETURN(DISPATCH_OK); /* no limitations */
      }

      /* we're done if there is no consumable capacity */
      if (!(tep=lGetElemStr(total_list, CE_name, name))) {
         lFreeElem(&cplx_el);
         *slots      = INT_MAX;
         *slots_qend = INT_MAX;
         DRETURN(DISPATCH_OK);
      }

      lFreeElem(&cplx_el);
   }

   if (!tep && !(tep=lGetElemStr(total_list, CE_name, name))) {
      DRETURN(DISPATCH_NEVER_CAT);
   }
   total = lGetDouble(tep, CE_doubleval);

   if (sconf_get_qs_state()==QS_STATE_EMPTY) {
      used = 0;
   } else if (schedule_based) {
      if (!a->is_reservation) {
         start = sconf_get_now();
      }   
      used = utilization_max(uep, start, a->duration);
      DPRINTF(("\t\t%s: ri_slots_by_time: utilization_max("sge_u32", "sge_u32") returns %f\n", 
            object_name, start, a->duration, used));
   } else {
      used = lGetDouble(lGetElemStr(rue_list, RUE_name, name), RUE_utilized_now);
   }

   request_val = lGetDouble(request, CE_doubleval);

   if ((request_val != 0) && (total < DBL_MAX)) {
      *slots      = (int)((total - used) / request_val);
      *slots_qend = (int)(total / request_val);
   }
   else {
      *slots = INT_MAX;
      *slots_qend = INT_MAX;
   }   

   if (*slots == 0 || *slots_qend == 0) {
      char dom_str[5];
      dstring availability; char availability_text[1024];

      sge_dstring_init(&availability, availability_text, sizeof(availability_text));
      
      monitor_dominance(dom_str, DOMINANT_TYPE_CONSUMABLE | layer);
      sge_dstring_sprintf(&availability, "%s:%s=%f", dom_str, lGetString(request, CE_name), (total - used));
      sge_dstring_append(reason, MSG_SCHEDD_ITOFFERSONLY);
      sge_dstring_append(reason, availability_text);

      if ((a->duration != DISPATCH_TIME_NOW) &&
          (*slots != 0 && *slots_qend == 0)) {
         sge_dstring_append(reason, MSG_SCHEDD_DUETORR);
      }
   }
   
   DPRINTF(("\t\t%s: ri_slots_by_time: %s=%f has %d (%d) slots at time "sge_U32CFormat"%s (avail: %f total: %f)\n", 
         object_name, lGetString(uep, RUE_name), request_val, *slots, *slots_qend, start,
           !a->is_reservation?" (= now)":"", total - used, total));

   DRETURN(DISPATCH_OK);
}


/* Determine maximum number of host_slots as limited
   by job request to this host 

   for each resource at this host requested by the job {
      avail(R) = (total - used) / request
   }
   host_slot_max_by_R = MIN(all avail(R))

   host_slot = MIN(host_slot_max_by_T, host_slot_max_by_R)

*/
static dispatch_t
parallel_rc_slots_by_time(const sge_assignment_t *a, lList *requests,  int *slots, int *slots_qend, 
                 lList *total_list, lList *rue_list, lList *load_attr, bool force_slots, 
                 lListElem *queue, u_long32 layer, double lc_factor, u_long32 tag, 
                 bool allow_non_requestable, const char *object_name)
{
   dstring reason; 
   char reason_buf[1024];
   int avail = 0;
   int avail_qend = 0;
   int max_slots = INT_MAX, max_slots_qend = INT_MAX;
   const char *name;
   static lListElem *implicit_slots_request = NULL;
   lListElem *tep, *cep, *actual, *req;
   dispatch_t result;

   DENTER(TOP_LAYER, "parallel_rc_slots_by_time");

   sge_dstring_init(&reason, reason_buf, sizeof(reason_buf));

   clear_resource_tags(requests, QUEUE_TAG); 

   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, SGE_ATTR_SLOTS);
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* --- implicit slot request */
   name = SGE_ATTR_SLOTS;
   if (!(tep = lGetElemStr(total_list, CE_name, name)) && force_slots) {
      DRETURN(DISPATCH_OK);
   }
   if (tep) {
      if (ri_slots_by_time(a, &avail, &avail_qend, 
            rue_list, implicit_slots_request, load_attr, total_list, queue, layer, lc_factor, 
            &reason, allow_non_requestable, false, object_name)) {
         char buff[1024 + 1];
         centry_list_append_to_string(requests, buff, sizeof(buff) - 1);
         if (*buff && (buff[strlen(buff) - 1] == '\n')) {
            buff[strlen(buff) - 1] = 0;
         }   
         schedd_mes_add(lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, buff, object_name, reason_buf);
         DRETURN(DISPATCH_NEVER_CAT);
      }
      max_slots      = MIN(max_slots,      avail);
      max_slots_qend = MIN(max_slots_qend, avail_qend);
      DPRINTF(("%s: parallel_rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
            (int)max_slots, (int)max_slots_qend));
   }


   /* --- default request */
   for_each (actual, rue_list) {
      name = lGetString(actual, RUE_name);
      if (!strcmp(name, SGE_ATTR_SLOTS)) {
         continue;
      }
      cep = centry_list_locate(a->centry_list, name);

      if (!is_requested(requests, name)) {
         double request;
         const char *def_req = lGetString(cep, CE_default);
         if (def_req) {
            parse_ulong_val(&request, NULL, lGetUlong(cep, CE_valtype), def_req, NULL, 0);

            if (request != 0) {

               lSetString(cep, CE_stringval, def_req);
               lSetDouble(cep, CE_doubleval, request);

               if (ri_slots_by_time(a, &avail, &avail_qend, 
                        rue_list, cep, load_attr, total_list, queue, layer, lc_factor,   
                        &reason, allow_non_requestable, false, object_name)==-1) {
                  char buff[1024 + 1];
                  centry_list_append_to_string(requests, buff, sizeof(buff) - 1);
                  if (*buff && (buff[strlen(buff) - 1] == '\n')) {
                     buff[strlen(buff) - 1] = 0;
                  }   
                  schedd_mes_add(lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, buff, object_name, reason_buf);
                  DRETURN(DISPATCH_NEVER_CAT);
               }
               max_slots      = MIN(max_slots,      avail);
               max_slots_qend = MIN(max_slots_qend, avail_qend);
               DPRINTF(("%s: parallel_rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
                     (int)max_slots, (int)max_slots_qend));
            }
         }
      } 
   }

   /* --- explicit requests */
   for_each (req, requests) {
      name = lGetString(req, CE_name);
      result = ri_slots_by_time(a, &avail, &avail_qend, 
               rue_list, req, load_attr, total_list, queue, layer, lc_factor,   
               &reason, allow_non_requestable, false, object_name);

      if (result == DISPATCH_NEVER_CAT || result == DISPATCH_NEVER_JOB) {
         char buff[1024 + 1];
         centry_list_append_to_string(requests, buff, sizeof(buff) - 1);
         if (*buff && (buff[strlen(buff) - 1] == '\n')) {
            buff[strlen(buff) - 1] = 0;
         }   
         schedd_mes_add(lGetUlong(a->job, JB_job_number), SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, buff, object_name, reason_buf);
      }
      
      switch (result) {
         case DISPATCH_OK: /* the requested element does not exist */
         case DISPATCH_NOT_AT_TIME: /* will match later-on */

            DPRINTF(("%s: explicit request for %s gets us %d slots (%d later)\n", 
                  object_name, name, avail, avail_qend));
            if (lGetUlong(req, CE_tagged) < tag)
               lSetUlong(req, CE_tagged, tag);

            max_slots      = MIN(max_slots,      avail);
            max_slots_qend = MIN(max_slots_qend, avail_qend);
            DPRINTF(("%s: parallel_rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
                  (int)max_slots, (int)max_slots_qend));
            break;

         case DISPATCH_NEVER_CAT: /* the requested element does not exist */
   
            DPRINTF(("%s: parallel_rc_slots_by_time(%s) <never>\n", object_name, name));
            *slots = *slots_qend = 0;
            DRETURN(DISPATCH_NEVER_CAT);

         case DISPATCH_NEVER_JOB: /* the requested element does not exist */

            DPRINTF(("%s: parallel_rc_slots_by_time(%s) <never>\n", object_name, name));
            *slots = *slots_qend = 0;
            DRETURN(DISPATCH_NEVER_JOB);

            
         case DISPATCH_MISSING_ATTR: /* the requested element does not exist */
            if (tag == QUEUE_TAG && lGetUlong(req, CE_tagged) == NO_TAG) {
               DPRINTF(("%s: parallel_rc_slots_by_time(%s) <never found>\n", object_name, name));
               *slots = *slots_qend = 0;
               DRETURN(DISPATCH_NEVER_CAT);
            }
            DPRINTF(("%s: parallel_rc_slots_by_time(%s) no such resource, but already satisified\n", 
                     object_name, name));
            break;
         case DISPATCH_NEVER:
         default :
            DPRINTF(("unexpected return code\n")); 
      }
   }

   *slots = max_slots;
   *slots_qend = max_slots_qend;

   DRETURN(DISPATCH_OK);
}

/****** sge_select_queue/sge_create_load_list() ********************************
*  NAME
*     sge_create_load_list() -- create the controll structure for consumables as
*                               load thresholds
*
*  SYNOPSIS
*     void sge_create_load_list(const lList *queue_list, const lList 
*     *host_list, const lList *centry_list, lList **load_list) 
*
*  FUNCTION
*     scanes all queues for consumables as load thresholds. It builds a 
*     consumable category for each queue which is using consumables as a load
*     threshold. 
*     If no consumables are used, the *load_list is set to NULL.
*
*  INPUTS
*     const lList *queue_list  - a list of queue instances
*     const lList *host_list   - a list of hosts
*     const lList *centry_list - a list of complex entries
*     lList **load_list        - a ref to the target load list
*
*  NOTES
*     MT-NOTE: sge_create_load_list() is MT safe 
*
*  SEE ALSO
*     sge_create_load_list
*     load_locate_elem
*     sge_load_list_alarm
*     sge_remove_queue_from_load_list
*     sge_free_load_list
*
*******************************************************************************/
void sge_create_load_list(const lList *queue_list, const lList *host_list, 
                          const lList *centry_list, lList **load_list) {
   lListElem *queue;
   lListElem *load_threshold;
   lListElem *centry;
   lList * load_threshold_list;
   const char *load_threshold_name;
   const char *limit_value;
   lListElem *global;
   lListElem *host;

   DENTER(TOP_LAYER, "sge_create_load_list");

   if (load_list == NULL){
      CRITICAL((SGE_EVENT, "no load_list specified\n"));
      DEXIT;
      abort();
   }

   if (*load_list != NULL){
      sge_free_load_list(load_list);
   }

   if ((global = host_list_locate(host_list, SGE_GLOBAL_NAME)) == NULL) {
      ERROR((SGE_EVENT, "no global host in sge_create_load_list"));
      DRETURN_VOID;
   }

   for_each(queue, queue_list) {
      load_threshold_list = lGetList(queue, QU_load_thresholds);
      for_each(load_threshold, load_threshold_list) {
         load_threshold_name = lGetString(load_threshold, CE_name);
         limit_value = lGetString(load_threshold, CE_stringval);
         if ((centry = centry_list_locate(centry_list, load_threshold_name)) == NULL) {
            ERROR((SGE_EVENT, MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S, load_threshold_name));
            goto error;
         }

         if (lGetBool(centry, CE_consumable)) {
            lListElem *global_consumable = NULL;
            lListElem *host_consumable = NULL;
            lListElem *queue_consumable = NULL;

            lListElem *load_elem = NULL;
            lListElem *queue_ref_elem = NULL;
            lList *queue_ref_list = NULL;


            if ((host = host_list_locate(host_list, lGetHost(queue, QU_qhostname))) == NULL){  
               ERROR((SGE_EVENT, MSG_SGETEXT_INVALIDHOSTINQUEUE_SS, 
                      lGetHost(queue, QU_qhostname), lGetString(queue, QU_full_name)));
               goto error;
            }

            global_consumable = lGetSubStr(global, RUE_name, load_threshold_name, 
                                           EH_resource_utilization);
            host_consumable = lGetSubStr(host, RUE_name, load_threshold_name, 
                                         EH_resource_utilization);
            queue_consumable = lGetSubStr(queue, RUE_name, load_threshold_name, 
                                          QU_resource_utilization);

            if (*load_list == NULL) {
               *load_list = lCreateList("load_ref_list", LDR_Type);
               if (*load_list == NULL) {
                  goto error;
               }   
            }
            else {
               load_elem = load_locate_elem(*load_list, global_consumable, 
                                            host_consumable, queue_consumable,
                                            limit_value);
            }
            if (load_elem == NULL) {
               load_elem = lCreateElem(LDR_Type);
               if (load_elem == NULL) {
                  goto error;
               }   
               lSetPosRef(load_elem, LDR_global_pos, global_consumable);
               lSetPosRef(load_elem, LDR_host_pos, host_consumable);
               lSetPosRef(load_elem, LDR_queue_pos, queue_consumable);
               lSetPosString(load_elem, LDR_limit_pos, limit_value);
               lAppendElem(*load_list, load_elem);
            }
         
            queue_ref_list = lGetPosList(load_elem, LDR_queue_ref_list_pos);
            if (queue_ref_list == NULL) {
               queue_ref_list = lCreateList("", QRL_Type);
               if (queue_ref_list == NULL) {
                  goto error;
               }   
               lSetPosList(load_elem, LDR_queue_ref_list_pos, queue_ref_list);
            }
               
            queue_ref_elem = lCreateElem(QRL_Type);
            if (queue_ref_elem == NULL) {
               goto error;
            }   
            lSetRef(queue_ref_elem, QRL_queue, queue);
            lAppendElem(queue_ref_list, queue_ref_elem);

            /* reset the changed bit in the consumables */
            if (global_consumable != NULL){
               sge_bitfield_reset(&(global_consumable->changed));
            }
            if (host_consumable != NULL){
               sge_bitfield_reset(&(host_consumable->changed));
            }
            if (queue_consumable != NULL){
               sge_bitfield_reset(&(queue_consumable->changed));
            }

         }
      }
   }

   DRETURN_VOID;

error:
   DPRINTF(("error in sge_create_load_list!"));
   ERROR((SGE_EVENT, MSG_SGETEXT_CONSUMABLE_AS_LOAD));
   sge_free_load_list(load_list);
   DRETURN_VOID;

}

/****** sge_select_queue/load_locate_elem() ************************************
*  NAME
*     load_locate_elem() -- locates a consumable category in the given load list
*
*  SYNOPSIS
*     static lListElem* load_locate_elem(lList *load_list, lListElem 
*     *global_consumable, lListElem *host_consumable, lListElem 
*     *queue_consumable) 
*
*  INPUTS
*     lList *load_list             - the load list to work on
*     lListElem *global_consumable - a ref to the global consumable
*     lListElem *host_consumable   - a ref to the host consumable
*     lListElem *queue_consumable  - a ref to the qeue consumable
*
*  RESULT
*     static lListElem* - NULL, or the category element from the load list
*
*  NOTES
*     MT-NOTE: load_locate_elem() is MT safe 
*
*  SEE ALSO
*     sge_create_load_list
*     load_locate_elem
*     sge_load_list_alarm
*     sge_remove_queue_from_load_list
*     sge_free_load_list
*     
*******************************************************************************/
static lListElem *load_locate_elem(lList *load_list, lListElem *global_consumable, 
                            lListElem *host_consumable, lListElem *queue_consumable,
                            const char *limit) {
   lListElem *load_elem = NULL;
   lListElem *load = NULL;

   for_each(load, load_list) {
      if ((lGetPosRef(load, LDR_global_pos) == global_consumable) &&
          (lGetPosRef(load, LDR_host_pos) == host_consumable) &&
          (lGetPosRef(load, LDR_queue_pos) == queue_consumable) &&
          ( strcmp(lGetPosString(load, LDR_limit_pos), limit) == 0)) {
         load_elem = load;
         break;
      }
   }
   
   return load_elem;
}

/****** sge_select_queue/sge_load_list_alarm() *********************************
*  NAME
*     sge_load_list_alarm() -- checks if queues went into an alarm state
*
*  SYNOPSIS
*     bool sge_load_list_alarm(lList *load_list, const lList *host_list, const 
*     lList *centry_list) 
*
*  FUNCTION
*     The function uses the cull bitfield to identify modifications in one of
*     the consumable elements. If the consumption has changed, the load for all
*     queue referencing the consumable is recomputed. If a queue exceeds it
*     load threshold, QU_tagged4schedule is set to 1.
*
*  INPUTS
*     lList *load_list         - ??? 
*     const lList *host_list   - ??? 
*     const lList *centry_list - ??? 
*
*  RESULT
*     bool - true, if at least one queue was set into alarm state 
*
*  NOTES
*     MT-NOTE: sge_load_list_alarm() is MT safe 
*
*  SEE ALSO
*     sge_create_load_list
*     load_locate_elem
*     sge_load_list_alarm
*     sge_remove_queue_from_load_list
*     sge_free_load_list
*     
*******************************************************************************/
bool sge_load_list_alarm(lList *load_list, const lList *host_list, 
                         const lList *centry_list) {   
   lListElem *load;
   lListElem *queue;
   lListElem *queue_ref;
   lList *queue_ref_list;
   char reason[2048];
   bool is_alarm = false;
   
   DENTER(TOP_LAYER, "sge_load_list_alarm");

   if (load_list == NULL) {
      DRETURN(is_alarm);
   }
   
   for_each(load, load_list) {
      bool is_recalc=false;
      lListElem * elem;

      elem = lGetPosRef(load, LDR_global_pos);
      if (elem != NULL) {
         if ( sge_bitfield_changed(&(elem->changed))) {
            is_recalc = true;
            sge_bitfield_reset(&(elem->changed)); 
         }
      }
      
      elem = lGetPosRef(load, LDR_host_pos);
      if (elem != NULL) {
         if ( sge_bitfield_changed(&(elem->changed))) {
            is_recalc = true;
            sge_bitfield_reset(&(elem->changed)); 
         }
      }
      
      elem = lGetPosRef(load, LDR_queue_pos);
      if (elem != NULL) {
         if ( sge_bitfield_changed(&(elem->changed))) {
            is_recalc = true;
            sge_bitfield_reset(&(elem->changed)); 
         }
      }
     
      if (is_recalc) {
         bool is_category_alarm = false;
         queue_ref_list = lGetPosList(load, LDR_queue_ref_list_pos);
         for_each(queue_ref, queue_ref_list) {
            queue = lGetRef(queue_ref, QRL_queue);
            if (is_category_alarm) {
               lSetUlong(queue, QU_tagged4schedule, 1); 
            }
            else if (sge_load_alarm(reason, queue, lGetList(queue, QU_load_thresholds), host_list, 
                               centry_list, NULL, true)) {

               DPRINTF(("queue %s tagged to be overloaded: %s\n", 
                                     lGetString(queue, QU_full_name), reason));
               schedd_mes_add_global(SCHEDD_INFO_QUEUEOVERLOADED_SS, 
                                     lGetString(queue, QU_full_name), reason);
               lSetUlong(queue, QU_tagged4schedule, 1); 
               is_alarm = true;
               is_category_alarm = true;
            }
            else {
               break;
            }
         }
      }
   }
   
   DRETURN(is_alarm); 
}

/****** sge_select_queue/sge_remove_queue_from_load_list() *********************
*  NAME
*     sge_remove_queue_from_load_list() -- removes queues from the load list 
*
*  SYNOPSIS
*     void sge_remove_queue_from_load_list(lList **load_list, const lList 
*     *queue_list) 
*
*  INPUTS
*     lList **load_list       - load list structure
*     const lList *queue_list - queues to be removed from it.
*
*  NOTES
*     MT-NOTE: sge_remove_queue_from_load_list() is MT safe 
*
*  SEE ALSO
*     sge_create_load_list
*     load_locate_elem
*     sge_load_list_alarm
*     sge_remove_queue_from_load_list
*     sge_free_load_list
*     
*******************************************************************************/
void sge_remove_queue_from_load_list(lList **load_list, const lList *queue_list){
   lListElem* queue = NULL;
   lListElem *load = NULL;
   
   DENTER(TOP_LAYER, "sge_remove_queue_from_load_list");
   
   if (load_list == NULL){
      CRITICAL((SGE_EVENT, "no load_list specified\n"));
      DEXIT;
      abort();
   }

   if (*load_list == NULL) {
      DRETURN_VOID;
   }

   for_each(queue, queue_list) {
      bool is_found = false;
      lList *queue_ref_list = NULL;
      lListElem *queue_ref = NULL;
   
      for_each(load, *load_list) {
         queue_ref_list = lGetPosList(load, LDR_queue_ref_list_pos);
         for_each(queue_ref, queue_ref_list) {
            if (queue == lGetRef(queue_ref, QRL_queue)) {
               is_found = true;
               break;
            }
         }
         if (is_found) {
            lRemoveElem(queue_ref_list, &queue_ref);

            if (lGetNumberOfElem(queue_ref_list) == 0) {
               lRemoveElem(*load_list, &load);
            }            
            break;
         }
      }

      if (lGetNumberOfElem(*load_list) == 0) {
         lFreeList(load_list);
         DRETURN_VOID;
      }
   }

   DRETURN_VOID;
}


/****** sge_select_queue/sge_free_load_list() **********************************
*  NAME
*     sge_free_load_list() -- frees the load list and sets it to NULL
*
*  SYNOPSIS
*     void sge_free_load_list(lList **load_list) 
*
*  INPUTS
*     lList **load_list - the load list
*
*  NOTES
*     MT-NOTE: sge_free_load_list() is MT safe 
*
*  SEE ALSO
*     sge_create_load_list
*     load_locate_elem
*     sge_load_list_alarm
*     sge_remove_queue_from_load_list
*     sge_free_load_list
*     
*******************************************************************************/
void sge_free_load_list(lList **load_list)
{
   DENTER(TOP_LAYER, "sge_free_load_list");

   lFreeList(load_list);
   
   DRETURN_VOID;
}

#ifdef SGE_PQS_API

typedef struct lib_cache_s {
   char *key;
   char *lib_name;
   char *fn_name;
   void *lib_handle;
   void *fn_handle;
   struct lib_cache_s *next;
} lib_cache_t;

/****** sge_dlib() *************************************************************
*  NAME
*     sge_dlib() -- lookup, load, and cache function from a dynamic library
*
*  SYNOPSIS
*     void *sge_dlib(const char *key, const char *lib_name, const char *fn_name,
*                    lib_cache_t **lib_cache_list)
*
*  INPUTS
*     const char *key - unique key for identifying function
*     const char *lib_name - dynamic library name
*     const char *fn_nam - function name
*     lib_cache_t **lib_cache_list - cache list (if NULL, we use a global cache)
*
*  RETURNS
*     void * - the address of the function
*
*  NOTES
*     MT-NOTE: sge_free_load_list() is not MT safe 
*
*  SEE ALSO
*     
*******************************************************************************/
void *
sge_dlib(const char *key, const char *lib_name, const char *fn_name,
         lib_cache_t **lib_cache_list)
{
   static lib_cache_t *static_lib_cache_list = NULL;
   lib_cache_t **cache_list = NULL;
   lib_cache_t *cache = NULL;
   lib_cache_t *prev = NULL;
   lib_cache_t *new_cache = NULL;
   int replace = 0;
   void *new_lib_handle = NULL;
   void *new_fn_handle = NULL; 
   const char *error = NULL;

   DENTER(TOP_LAYER, "sge_dlib");

   /* Use user cache list if supplied */
   if (lib_cache_list)
      cache_list = lib_cache_list;
   else
      cache_list = &static_lib_cache_list;

   /*
    * Search based on supplied key. If found and the library name and function
    * name match, return the function address. If the library or function
    * do not match, then we will reload the library.
    */
   for (cache=*cache_list; cache; prev=cache, cache=cache->next) {
      if (strcmp(key, cache->key)==0) {
         if (strcmp(lib_name, cache->lib_name)==0 &&
             strcmp(fn_name, cache->fn_name)==0) {
            DRETURN(cache->fn_handle);
         } else {
            replace=1;
            break;
         }
      }
   }

   /* open the library */
   new_lib_handle = dlopen(lib_name, RTLD_LAZY);
   if (!new_lib_handle) {
      error = dlerror();
      ERROR((SGE_EVENT, "Unable to open library %s for %s - %s\n",
             lib_name, key, error));
      DRETURN(NULL);
   }

   /* search library for the function name */
   new_fn_handle = dlsym(new_lib_handle, fn_name);
   if (((error = dlerror()) != NULL) || !new_fn_handle) {
      dlclose(new_lib_handle);
      ERROR((SGE_EVENT, "Unable to locate function %s in library %s for %s - %s\n",
             fn_name, lib_name, key, error));
      DRETURN(NULL);
   }

   /* If we're replacing the old function, just delete it */
   if (replace) {
      dlclose(cache->lib_handle);
      FREE(cache->key);
      FREE(cache->lib_name);
      FREE(cache->fn_name);
      if (prev == NULL) {
         *cache_list = cache->next;
      }   
      else {
         prev->next = cache->next;
      }   
      FREE(cache);
   }

   /* cache the new function address */
   if ((new_cache = (lib_cache_t *)malloc(sizeof(lib_cache_t))) == NULL ||
       (new_cache->key = strdup(key)) == NULL ||
       (new_cache->lib_name = strdup(lib_name)) == NULL ||
       (new_cache->fn_name = strdup(fn_name)) == NULL) {
      ERROR((SGE_EVENT, "Memory allocation problem in sge_dl\n"));
      DRETURN(NULL);
   }
   new_cache->lib_handle = new_lib_handle;
   new_cache->fn_handle = new_fn_handle;
   new_cache->next = *cache_list;
   *cache_list = new_cache;

   /* return the cached function address */
   DRETURN(new_cache->fn_handle);
}

static void
strcpy_replace(char *dp, const char *sp, lList *rlist)
{
   char *name = NULL;
   int done = 0;
   int curly = 0;
   const char *es = NULL;

   if (rlist == NULL) {
      strcpy(dp, sp);
      return;
   }

   while (!done) {
      if (*sp == 0) /* done means we make one last pass */
         done = 1;
      if (name && !isalnum(*sp) && *sp != '_') {
         lListElem *res;
         const char *s = "";
         *dp = 0;
         if ((res = lGetElemStr(rlist, CE_name, name)))
            s = lGetString(res, CE_stringval);
         /* handle ${varname} */
         if (curly && *sp == '}')
            sp++;
         /* handle ${varname:-value} and ${varname:+value} */
         else if (curly && *sp == ':') {
            if (sp[1] == '-' || sp[1] == '+') {
               const char *ep;
               for (ep=sp+2; *ep; ep++) {
                  if (*ep == '}') {
                     if ((!*s && sp[1] == '-') ||
                         (*s  && sp[1] == '+')) {
                        s = sp+2;   /* mark substition string */
                        es = ep;    /* mark end of substition string */
                     }
                     sp = ep+1;  /* point past varname */
                     break;
                  }
               }
            }
         }
         dp = name;
         while (*s && s != es)
            *dp++ = *s++;
         name = NULL;
         *dp = 0;
         curly = 0;
         es = NULL;
      } else if (*sp == '$') {
         sp++;
         name = dp;
         if (*sp == '{') {
            curly = 1;
            sp++;
         }
      } else
         *dp++ = *sp++;
   }
}

/****** sge_select_queue/sge_call_pe_qsort() **********************************
*  NAME
*     sge_call_pe_qsort() -- call the Parallel Environment qsort plug-in
*
*  SYNOPSIS
*     void sge_call_pe_qsort(sge_assignment_t *a, const char *qsort_args)
*
*  INPUTS
*     sge_assignment_t *a - PE assignment
*     qsort_args - the PE qsort_args attribute
*
*  NOTES
*     MT-NOTE: sge_call_pe_qsort() is not MT safe 
*
*  SEE ALSO
*     
*******************************************************************************/
static int
sge_call_pe_qsort(sge_assignment_t *a, const char *qsort_args)
{
   int ret = 0;
   struct saved_vars_s *cntx = NULL;
   char *tok;
   int argc = 0;
   pqs_qsort_t pqs_qsort;
   const char *pe_name = lGetString(a->pe, PE_name);
   const char *lib_name;
   const char *fn_name;
   int num_queues = lGetNumberOfElem(a->queue_list);
   char qsort_args_buf[4096];
   const char *qsort_argv[1024];
   char err_str[1024];

   DENTER(TOP_LAYER, "sge_call_pe_qsort");

   err_str[0] = 0;

   /*
    * Copy qsort_args substituting any references to $<resource>
    * with the corresponding requested value from the hard resource
    * list of the job.
    */

   strcpy_replace(qsort_args_buf, qsort_args, lGetList(a->job, JB_hard_resource_list));

   if ((lib_name = sge_strtok_r(qsort_args_buf, " ", &cntx)) &&
       (fn_name = sge_strtok_r(NULL, " ", &cntx)) &&
       (pqs_qsort = sge_dlib(pe_name, lib_name, fn_name, NULL))) {

      pqs_params_t pqs_params;
      pqs_queue_t *qp;
      lListElem *q;

      /*
       * Build queue sort parameters
       */
      pqs_params.job_id = a->job_id;
      pqs_params.task_id = a->ja_task_id;
      pqs_params.slots = a->slots;
      pqs_params.pe_name = pe_name;
      pqs_params.pe_qsort_args = qsort_args_buf;
      pqs_params.pe_qsort_argv = qsort_argv;
      pqs_params.num_queues = num_queues;
      pqs_params.qlist = (pqs_queue_t *)malloc(num_queues * sizeof(pqs_queue_t));

      qp = pqs_params.qlist;
      for_each(q, a->queue_list) {
         qp->queue_name = lGetString(q, QU_qname);
         qp->host_name = lGetHost(q, QU_qhostname);
         qp->host_seqno = lGetUlong(q, QU_host_seq_no);
         qp->queue_seqno = lGetUlong(q, QU_seq_no);
         qp->available_slots = lGetUlong(q, QU_tag);
         qp->soft_violations = lGetUlong(q, QU_soft_violation);
         qp->new_seqno = 0;
         qp++;
      }

      /*
       * For convenience, convert qsort_args into an argument vector qsort_argv.
       */
      qsort_argv[argc++] = lib_name;
      qsort_argv[argc++] = fn_name;
      while ((tok = sge_strtok_r(NULL, " ", &cntx)) &&
             argc<(sizeof(qsort_argv)/sizeof(char *)-1)) {
         qsort_argv[argc++] = tok;
      }

      /*
       * Call the dynamic queue sort function
       */

      ret = (*pqs_qsort)(&pqs_params, 0, err_str, sizeof(err_str)-1);

      if (err_str[0]) {
         ERROR((SGE_EVENT, err_str));
      }   

      /*
       * Update the queue list with the new sort order
       */

      if (ret == PQS_ASSIGNED) {
         qp = pqs_params.qlist;
         for_each(q, a->queue_list) {
            lSetUlong(q, QU_host_seq_no, qp->new_seqno);
            qp++;
         }
      }

      FREE(pqs_params.qlist);
   } 
   else {
      ERROR((SGE_EVENT, "Unable to dynamically load PE qsort_args %s\n", qsort_args));
      ret = 0; /* schedule anyway with normal queue/host sort */
   }

   if (cntx) {
      sge_free_saved_vars(cntx);
   }   

   DRETURN(ret);
}

#endif

