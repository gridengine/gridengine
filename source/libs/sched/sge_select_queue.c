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


#include "basis_types.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_select_queue.h"
#include "sge_parse_num_par.h"
#include "sge_resource.h"
#include "sge_complex_schedd.h"
#include "valid_queue_user.h"
#include "subordinate_schedd.h"
#include "sge_range_schedd.h"
#include "sge_pe_schedd.h"
#include "slots_used.h"

#include "sge_orderL.h"
#include "sge_requestL.h"
#include "sge_pe.h"
#include "sge_complex.h"
#include "sge_schedd_conf.h"
#include "sort_hosts.h"
#include "schedd_monitor.h"
#include "schedd_message.h"
#include "msg_schedd.h"
#include "sge_schedd_text.h"
#include "sge_ja_task.h"
#include "msg_schedd.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_schedd_conf.h"
#include "sge_job.h"
#include "sge_queue.h"
#include "sge_userprj.h"
#include "sge_host.h"
#include "sge_ckpt.h"

int scheduled_fast_jobs;
int scheduled_complex_jobs;

static int is_requested(lList *req, const char *attr);

static lListElem *get_util_max(lListElem *cplx_el, lList *ccl[3]);

static int sge_select_resource(lList *complex_attributes, 
                               lList *resources, int allow_non_requestable, 
                               char *reason, int reason_size, int slots, 
                               lList *ccl[3], int force_attr_existence);

static int available_slots_global(lList **global_resources, lListElem *job, 
                                  lListElem *ja_task, lListElem *pe, 
                                  lList *host_list, lList *complex_list, 
                                  int global_slots, lList *ccl[3], 
                                  lList *acl_list);

static int available_slots_at_host(lList *host_resources, lListElem *job, 
                                   lListElem *ja_task, lListElem *host, 
                                   int global_slots, int minslots, 
                                   int allocation_rule, lList *ccl[3], 
                                   lList *complex_list, lList *acl_list);

static int sge_soft_violations(lList *complex_attributes, lListElem *queue, 
                               lListElem *job, lListElem *pe, lList *cplx_list,
                               lList *ccl[3]);

static int sge_why_not_job2queue_static(lListElem *queue, lListElem *job, 
                                        lListElem *pe, lListElem *ckpt, 
                                        lList *cplx_list, lList *host_list, 
                                        lList *acl_list);

static int sge_why_not_job2host(lListElem *job, lListElem *ja_task, 
                                lListElem *host, lList *complex_list, 
                                lList *acl_list);

static int fulfilled(lListElem *rep, lList *given_attr, char *reason, 
                     int reason_size, int allow_non_requestable, 
                     int is_a_request, int slots, lList *ccl[3], 
                     int force_existence);

static int resource_cmp(u_long32 relop, double req, double src_dl);

static int requested_forced_attr(lListElem *job, lListElem *cplx, 
                                 char *obj_name, const char *obj_key);

static int ensure_forced(lListElem *job, char *main_cplx_name, char *obj_name, 
                         const char *obj_kex, lList *user_complex, 
                         lList *cplx_list);

static int sge_check_load_alarm(char *reason, const char *name, const char *load_value,
                                const char *limit_value, u_long32 relop,
                                u_long32 type, lListElem *hep,
                                lListElem *hlep, double lc_host,
                                double lc_global, lList *load_adjustments, int load_is_value); 

char* trace_resource(lListElem *ep) 
{
   int jl, sl;
   char slot_dom[4], job_dom[4];
   u_long32 dom;
   static char buffer[BUFSIZ];

   strcpy(buffer, "");

   jl = (dom=lGetUlong(ep, CE_pj_dominant)) 
         && ((dom&DOMINANT_TYPE_MASK) != DOMINANT_TYPE_VALUE);
   sl = (dom=lGetUlong(ep, CE_dominant)) 
         && ((dom&DOMINANT_TYPE_MASK) != DOMINANT_TYPE_VALUE);
   monitor_dominance(job_dom, lGetUlong(ep, CE_pj_dominant));
   monitor_dominance(slot_dom, lGetUlong(ep, CE_dominant));
   if (sl && jl) {
      sprintf(buffer, "%-20.20s %10.10s:%-10.10s %10.10s:%-10.10s\n", 
            lGetString(ep, CE_name), 
            slot_dom,
            lGetString(ep, CE_stringval), 
            job_dom,
            lGetString(ep, CE_pj_stringval));
   } else if (sl && !jl) {
      sprintf(buffer, "%-20.20s %10.10s:%-10.10s         NONE\n", 
            lGetString(ep, CE_name), 
            slot_dom,
            lGetString(ep, CE_stringval));
   } else if (!sl && jl) {
      sprintf(buffer, "%-20.20s          NONE         %10.10s:%-10.10s\n", 
            lGetString(ep, CE_name), 
            job_dom,
            lGetString(ep, CE_pj_stringval));
   } else if (!sl && !jl) {
      sprintf(buffer, "%-20.20s          NONE                 NONE\n", 
            lGetString(ep, CE_name));
   }

   return buffer;
}

void trace_resources(lList *resources) 
{
   lListElem *ep;
   char *ret;
   
   for_each (ep, resources) {
      ret = trace_resource(ep);
      DPRINTF((ret));
   }
}
/************************************************************************
   sge_select_queues - selects suitable queues based on what can be done
                       with -l (resources)
   QU_tagged will be set if a queue is selected
   The caller is responsible for cleaning tags.
   No range is used. For serial jobs we will need a call for hard and one
   for soft requests. For parallel jobs we will call this function for each
   -l request. Because of in serial jobs requests can be simply added.
   In Parallel jobs each -l requests a different set of queues.

   queue_list: list of queues from which to select
   complex_list: list of global/host/queue/userdefined complexes
   exechost_list: list of execution hosts (to fill host complex)
                  can be empty if we dont want to use actual values.
   allow_non_requestable: if true check for being requestable is skipped
   resources: resources generated from -l requests
   reason: if non NULL a reason is stored in case of no matching 
   reason_size: max. size of reason char array

complex_attributes: CX_Type
resources: RE_Type
 
************************************************************************/

int sge_select_queue(lList *complex_attributes, lList *resources, 
                     int allow_non_requestable, char *reason, 
                     int reason_size, int slots, lList *ccl[3]) 
{
   int ret;

   DENTER(TOP_LAYER, "sge_select_queue");
   ret = sge_select_resource(complex_attributes, resources, 
                             allow_non_requestable, reason, reason_size, 
                             slots, ccl, 1);

   DEXIT;
   return ret;
}

/*
 * check whether resource can fulfill for the jobs 
 * - explicit hard resource requests
 * - consumablel default requests
 * - jobs implicit slot request 
 *
 * complex_attributes: CX_Type
 * resources: RE_Type
 */ 
static int sge_select_resource(lList *complex_attributes, lList *resources,
                               int allow_non_requestable, char *reason,
                               int reason_size, int slots, lList *ccl[3],
                               int force_attr_existence) 
{
   lListElem *res;
   static lListElem *implicit_slots_request = NULL;

   DENTER(TOP_LAYER, "sge_select_resource");

   /* check amount of slots */
   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, "slots");
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* match number of free slots */
   if (slots != -1 && fulfilled(implicit_slots_request, complex_attributes, reason, reason_size, allow_non_requestable, 1, 
            slots, ccl, force_attr_existence)) {
      DEXIT;
      return 0;
   }

   /* ensure all default requests are fulfilled */
   if (slots != -1 && !allow_non_requestable) {
      lListElem *attr;
      int ff;
      const char *name;
      const char *def_req; 
      double dval;

      for_each (attr, complex_attributes) {
         name = lGetString(attr, CE_name);
         if (!strcmp(name, "slots"))
            continue;

         /* consumable && used in this global/host/queue && not requested */
         if (lGetBool(attr, CE_consumable) &&  !is_requested(resources, name)) {   
            lListElem *default_request;
            char tmp_reason[2048];

            def_req = lGetString(attr, CE_default);
            parse_ulong_val(&dval, NULL, lGetUlong(attr, CE_valtype), def_req, NULL, 0);

            /* ignore default request if the value is 0 */
            if (dval != 0.0) {
               /* build the default request */
               default_request = lCreateElem(CE_Type);
               lSetString(default_request, CE_name, name);
               lSetString(default_request, CE_stringval, def_req);
               lSetDouble(default_request, CE_doubleval, dval);

               tmp_reason[0] = '\0';
               ff = fulfilled(default_request, complex_attributes, tmp_reason, sizeof(tmp_reason)-1, 
                     1, 1, slots, ccl, force_attr_existence);
/*             DPRINTF(("DEFAULT-REQUEST: %s = %s -> %s\n", name, def_req, ff?"NO":"YES"));  */
               lFreeElem(default_request);
               if (ff) {
                  if (reason) {
                     strncpy(reason, MSG_SCHEDD_FORDEFAULTREQUEST , reason_size-1);
                     strncat(reason, tmp_reason, reason_size-1);
                  }
                  DEXIT;
                  return 0;
               }
            } 
         } 
      }
   }
 
   if (slots == -1)
      slots = 1;

   /* explicit requests */
   for_each (res, resources) 
      if (sge_match_complex_attributes(complex_attributes, 
            lGetList(res, RE_entries), 1, allow_non_requestable, reason, reason_size, 1,
           slots, ccl, force_attr_existence)) {
         DEXIT;
         return 0;
      }

   DEXIT;
   return 1;
}

/* 'attrname' can be found in all three lists 
   it depends on dominance layer which 
   ccl is responsible 

   ccl: needed to pass maximum utilization of global, host and queue
*/ 
static lListElem *get_util_max(lListElem *cplx_el, lList *ccl[3])
{
   u_long32 relop, mask;
   int i;
   lListElem *util_max_ep = NULL;

   DENTER(TOP_LAYER, "get_util_max");

   relop = lGetUlong(cplx_el, CE_relop);

   if (relop == CMPLXGE_OP || relop == CMPLXGT_OP) {
      const char *name = lGetString(cplx_el, CE_name);

      switch ((mask=lGetUlong(cplx_el, CE_dominant)) & DOMINANT_LAYER_MASK) {
      case DOMINANT_LAYER_GLOBAL:
         i = 0;
         break;
      case DOMINANT_LAYER_HOST:
         i = 1;
         break;
      case DOMINANT_LAYER_QUEUE:
         i = 2;
         break;
      default:

         CRITICAL((SGE_EVENT,MSG_SCHEDD_RETRLAYERDOMINFMASKXOFATTRYFAILED_US,  
               u32c(mask), lGetString(cplx_el, CE_name)));
         DEXIT;
         return NULL;
      }

      if (ccl[i] && !(util_max_ep = lGetElemStr(ccl[i], CE_name, name))) {
         char dom_str[4];
         monitor_dominance(dom_str, mask);
         /* should be not possible to happen since it is the qmasters
            job to keep these lists uptodate */
         DPRINTF(("cannot locate resource %s:%s in complex_values "
               "list\n", dom_str, name));
      }
   }

   DEXIT;
   return util_max_ep;
}

/* ----------------------------------------

   Matches two complex attribute lists

   This function gets used for two essential test

   - test whether resources 'given_attr' will be enough for   
      'slots' slots of a job with the resource request 'requested_attr'

   - test whether a load/suspend/migrate threshold is exceeded

   The job matches on a queue if all single requests match.

   The flag 'allow_non_requestable' says whether requesting non 
   requestables should cause an error
   The flag 'is_a_request' is used to reverse logic 

   As comparison operator CE_relop of given_attr gets used
   as data type CE_valtype of given_attr gets used 
   from requested_attr we only handle CE_name and CE_stringval
   allow_non_requestable: if true check for being requestable is skipped
   reason: if non NULL a reason is stored in case of no matching 
   reason_size: max. size of reason char array

   return value:
      -1 if req_attr is not in given_attr list.
      -2 an attribute configured as non requestable was requested

      if quick_exit
         returns 1 as soon as first non-matching req_attr is encountered.
         returns 0 if all req_attr match.
      else
         returns number of non-matching req_attrs, 0 if all match.

given_attr: CE_Type list
requested_attr: CE_Type list
ccl: needed to pass maximum utilization of global, host and queue
*/
int sge_match_complex_attributes(lList *given_attr, lList *requested_attr, 
                                 int quick_exit, int allow_non_requestable,
                                 char *reason, int reason_size, 
                                 int is_a_request, int slots, lList *ccl[3],
                                 int force_existence) 
{
   lListElem *rep; /* a requested element */
   int ret=0;

   DENTER(TOP_LAYER, "sge_match_complex_attributes");

   for_each (rep, requested_attr) {
      if (fulfilled(rep, given_attr, reason, reason_size, allow_non_requestable, 
                  is_a_request, slots, ccl, force_existence)) {
         if (quick_exit) {
            DEXIT;
            return -1;
         }
         ret++;
      } 
   }

   DEXIT;
   return ret;
}

/*
ccl: needed to pass maximum utilization of global, host and queue
*/
static int fulfilled(lListElem *rep, lList *given_attr, char *reason,
                     int reason_size, int allow_non_requestable, 
                     int is_a_request, int slots, lList *ccl[3],
                     int force_existence) 
{
   lListElem *cplx_el; /* a complex element */
   const char *attrname; 
   char availability_text[2048];
   int match;
   lListElem *util_max_ep = NULL;

   DENTER(TOP_LAYER, "fulfilled");

   /* resource_attr is a complex_entry (CE_Type) */
   attrname = lGetString(rep, CE_name);
   cplx_el = find_attribute_in_complex_list(attrname, lFirst(given_attr));

   if (!cplx_el ) { /* -l contains a complex we know nothing about -> dont select this queue */
      if (force_existence) {
         if (reason) {
            strncpy(reason, MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE , reason_size);
            strncat(reason, attrname, reason_size);
            strncat(reason, "\"", reason_size);
         }
         DPRINTF(("unknown resource %s\n", attrname));
         DEXIT;
         return -1;
      }

      DEXIT;
      return 0;
   }

   /* check whether attrib is requestable */
   if (!allow_non_requestable && !lGetBool(cplx_el, CE_request)) {
      if (reason) {
         strncpy(reason, MSG_SCHEDD_JOBREQUESTSNONREQUESTABLERESOURCE , reason_size);
         strncat(reason, attrname, reason_size);
         strncat(reason, "\"", reason_size);
      }
      DEXIT;
      return -1;
   }

   /* search for responsible (global, host, queue) maximum utilization */
   if (ccl && lGetBool(cplx_el, CE_consumable))
      util_max_ep = get_util_max(cplx_el, ccl);

#if 0
   if (is_a_request==0) { /* threshold */
      DPRINTF(("attributes     %s", trace_resource(cplx_el)));
      DPRINTF(("load threshold %s", trace_resource(rep)));
   } else { /* request */
      DPRINTF(("attributes     %s", trace_resource(cplx_el)));
      DPRINTF(("request        %s", trace_resource(rep)));
   }
#endif

   match = compare_complexes(slots, util_max_ep, rep, cplx_el, 
         availability_text, !is_a_request, force_existence);

   if (!match) {
      if (reason) {
         strncpy(reason, MSG_SCHEDD_ITOFFERSONLY , reason_size);
         strncat(reason, availability_text, reason_size);
      }
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}


/* -------------------------------------------------------

   Checks if a job fits on a queue or says why not
   does not modify queue or job
   All checks that depend on the number of requested slots 
       get handled outside  

   returns 
   0 ok, job may get scheduled on this queue
   1 sorry, job owner has no access according to acl/xacl of queue
   2 sorry, queue does not fulfill the hard requests of the job
   3 sorry, queue is not named in pe's queue_list or queue is no PQ
   4 sorry, queue is no batch or transfer queue as needed
   5 sorry, queue is not in QU_hard_queue_list (-q)
   6 sorry, queue is not named in ckpt's queue_list or queue is no CQ
   7 sorry, queue is no interactive queue as needed
   8 sorry, job did not request a forced resource
   9 sorry, job has not access according to project list of queue

*/
static int sge_why_not_job2queue_static(lListElem *queue, lListElem *job,
                                        lListElem *pe, lListElem *ckpt,
                                        lList *cplx_list, lList *host_list,
                                        lList *acl_list) 
{
   u_long32 job_id;
   const char *queue_name;
   char reason[1024 + 1];
   char buff[1024 + 1];
   lList *projects;
   const char *project;

   DENTER(TOP_LAYER, "sge_why_not_job2queue_static");

   reason[0] = buff[0] = '\0';

   job_id = lGetUlong(job, JB_job_number);
   queue_name = lGetString(queue, QU_qname);
   /* check if job owner has access rights to the queue */
   if (!sge_has_access(lGetString(job, JB_owner), lGetString(job, JB_group), queue, acl_list)) {
      DPRINTF(("Job %d has no permission for queue %s\n", (int)job_id, queue_name));
      schedd_mes_add(job_id, SCHEDD_INFO_HASNOPERMISSION_SS, "queue", queue_name);
      DEXIT;
      return 1;
   }

   /* check if job can run in queue based on project */
   if ((projects = lGetList(queue, QU_projects))) {
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "queue", queue_name);
         DEXIT;
         return 9;
      }
      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "queue", queue_name);
         DEXIT;
         return 9;
      }
   }

   /* check if job can run in queue based on excluded projects */
   if ((projects = lGetList(queue, QU_xprojects))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "queue", queue_name);
         DEXIT;
         return 9;
      }
   }

   /* is there a hard queue list ? */
   if (lGetList(job, JB_hard_queue_list)) {

      if (!queues_are_requestable(cplx_list)) {
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTREQUESTABLE_S,  
            queue_name);
         DEXIT;
         return 5;
      }

      if (!lGetSubStr(job, QR_name, lGetString(queue, QU_qname), JB_hard_queue_list)) {
         DPRINTF(("Queue \"%s\" is not contained in the hard "
           "queue list (-q) that was requested by job %d\n",
               queue_name, (int) job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINHARDQUEUELST_S,  
            queue_name);
         DEXIT;
         return 5;
      }
   }

   /* is this queue a candidate for being the master queue? */
   if (lGetList(job, JB_master_hard_queue_list)) {
      if (!queues_are_requestable(cplx_list)) {
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTREQUESTABLE_S,  
            queue_name);
         DEXIT;
         return 5;
      }

      if (!lGetSubStr(job, QR_name, lGetString(queue, QU_qname), JB_master_hard_queue_list)) {
         lSetUlong(queue, QU_tagged4schedule, 0);
      } else {
         DPRINTF(("Queue \"%s\" is contained in the master hard "
           "queue list (-masterq) that was requested by job %d\n",
               queue_name, (int) job_id));
         lSetUlong(queue, QU_tagged4schedule, 1);
      }
   } else
      lSetUlong(queue, QU_tagged4schedule, 0);

   /*
   ** different checks for different job types:
   */

   if (pe) { /* parallel job */
      /* is it a parallel queue ? */
      if (!(lGetUlong(queue, QU_qtype) & PQ)) {
         DPRINTF(("Queue \"%s\" is not a parallel queue as requested by " 
            "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELQUEUE_S,  
            queue_name);
   
         DEXIT;
         return 3;
      }

      /* check if the queue is named in queue list 
       * of requested parallel environment 
       * or whether the keyword all is mentioned */
      if (!lGetSubStr(pe, QR_name, queue_name, PE_queue_list)) {
         if (!lGetSubCaseStr(pe, QR_name, SGE_ATTRVAL_ALL, PE_queue_list)) {
            DPRINTF(("Queue \"%s\" is not in queue list of PE \"%s\" requested "
               "by job %d\n", queue_name, lGetString(pe, PE_name), (int)job_id));
            schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFPE_SS,  
               queue_name, lGetString(pe, PE_name));
            DEXIT;
            return 3;
         }
      }
   }

   if (ckpt) { /* ckpt job */
      /* is it a ckpt queue ? */
      if (!(lGetUlong(queue, QU_qtype) & CQ)) {
         DPRINTF(("Queue \"%s\" is not a checkpointing queue as requested by "
                  "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTACKPTQUEUE_SS, queue_name);
         DEXIT;
         return 6;
      }
      /*
         check if the keyword "all" or if the queue is named in 
         queue list of a requested ckpt interface definition 
      */ 
      if (!lGetSubStr(ckpt, QR_name, queue_name, CK_queue_list)) {
         if (!lGetSubCaseStr(ckpt, QR_name, SGE_ATTRVAL_ALL, CK_queue_list)) {
            DPRINTF(("Queue \"%s\" is not in queue list of ckpt \"%s\" "
               "requested by job %d\n", 
               queue_name, lGetString(ckpt, CK_name), (int)job_id));
            schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFCKPT_SS,  
               queue_name, lGetString(ckpt, CK_name));
            DEXIT;
            return 6;
         }
      }
   }   

   /* to be activated as soon as immediate jobs are available */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* immediate job */
      /* is it an interactve job and an interactive queue ? */
      if (!lGetString(job, JB_script_file) && !(lGetUlong(queue, QU_qtype) & IQ)) {
         DPRINTF(("Queue \"%s\" is not an interactive queue as requested by "
                  "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTINTERACTIVE_S, queue_name);
         DEXIT;
         return 7;
      } else /* is it a batch job and a batch or transfer queue ? */
      if (lGetString(job, JB_script_file) &&
          !(lGetUlong(queue, QU_qtype) & BQ) &&
          !(lGetUlong(queue, QU_qtype) & TQ)) {
         DPRINTF(("Queue \"%s\" is not a serial (batch or transfer) queue as "
                  "requested by job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTASERIALQUEUE_S, queue_name);
         DEXIT;
         return 4;
      }
   }

   if (!pe && !ckpt && !JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* serial (batch) job */
      /* is it a batch or transfer queue */
      if (!(lGetUlong(queue, QU_qtype) & BQ) &&
          !(lGetUlong(queue, QU_qtype) & TQ)) {
         DPRINTF(("Queue \"%s\" is not a serial (batch or transfer) queue as "
                  "requested by job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTASERIALQUEUE_S, queue_name);
         DEXIT;
         return 4;
      }
   }

   if (ckpt && !pe && lGetString(job, JB_script_file) &&
         (lGetUlong(queue, QU_qtype) & (PQ|BQ|TQ)) == PQ) {
      DPRINTF(("Queue \"%s\" is not a serial (batch or transfer) queue as "
               "requested by job %d\n", queue_name, (int)job_id));
      schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELJOB_S, queue_name);
      DEXIT;
      return 4;
   }

   if (ensure_forced(job, SGE_QUEUE_NAME, "queue", queue_name, 
         lGetList(queue, QU_complex_list), cplx_list)) {
      DEXIT;
      return 8;
   }

   DEXIT;
   return 0;
}

static int ensure_forced(lListElem* job, char *main_complex, char *obj_name, 
                         const char *obj_key, lList* usercplx, lList* cplx_list)
{

   DENTER(TOP_LAYER, "ensure_forced");

   /* iterate through all complex attributes
      and ensure forced attributes are requested by the job */
   {
      lListElem *cplx, *cplx_name;

      /* queue/host/global complex attributes */
      cplx = lGetElemStr(cplx_list, CX_name, main_complex);
/*       DPRINTF(("%s \"%s\": COMPLEX \"%s\"\n", obj_name, obj_key, main_complex)); */
      if (cplx && !requested_forced_attr(job, cplx, obj_name, obj_key)) {
         DEXIT;
         return -1;
      }

      /* additional complex attributes */
      for_each (cplx_name, usercplx) {
         cplx = lGetElemStr(cplx_list, CX_name, lGetString(cplx_name, CX_name));
/*          DPRINTF(("%s \"%s\": COMPLEX \"%s\"\n", obj_name, obj_key, lGetString(cplx_name, CX_name))); */
         if (cplx && !requested_forced_attr(job, cplx, obj_name, obj_key)) {
            DEXIT;
            return -1;
         }
      }
   }
   
   DEXIT;
   return 0;
}


static int requested_forced_attr(lListElem *job, lListElem *cplx, 
                                 char *obj_name, const char *obj_key)
{
   lListElem *attr;

   DENTER(TOP_LAYER, "requested_forced_attr");

   for_each (attr, lGetList(cplx, CX_entries)) {
      if (lGetBool(attr, CE_forced) &&
         !is_requested(lGetList(job, JB_hard_resource_list), lGetString(attr, CE_name))) {
         DPRINTF(("job "u32" does not request 'forced' resource \"%s\" of %s %s\n",
                  lGetUlong(job, JB_job_number), lGetString(attr, CE_name), obj_name, obj_key));
         schedd_mes_add(lGetUlong(job, JB_job_number), SCHEDD_INFO_NOTREQFORCEDRES_SS, lGetString(attr, CE_name), obj_key);
         DEXIT;
         return 0;
      }
   }

   DEXIT;
   return 1;
}

/* ccl: needed to pass maximum utilization of global, host and queue */

static int sge_soft_violations(lList *complex_attributes, lListElem *queue,
                               lListElem *job, lListElem *pe, lList *cplx_list,
                               lList *ccl[3]) 
{
   u_long32 job_id;
   const char *queue_name;
   char reason[1024 + 1];
   char buff[1024 + 1];
   unsigned int soft_violation = 0; 
   lListElem *res;

   DENTER(TOP_LAYER, "sge_soft_violations");

   reason[0] = buff[0] = '\0';

   job_id = lGetUlong(job, JB_job_number);
   queue_name = lGetString(queue, QU_qname);

   /* count number of soft violations for _one_ slot of this job */
   for_each (res, lGetList(job, JB_soft_resource_list)) {
      int i;
      soft_violation += (i=sge_match_complex_attributes(complex_attributes, 
            lGetList(res, RE_entries), 0, 0, reason, sizeof(reason)-1, 1, 1, ccl, 1));

         DPRINTF(("queue %s does not fulfill soft %d requests (first: %s)\n", 
               queue_name, i, reason));
   }

   if (lGetList(job, JB_soft_queue_list)) {
      /* check whether queue fulfills soft queue request of the job (-q) */
      if (!lGetSubStr(job, QR_name, queue_name, JB_soft_queue_list)) {
         DPRINTF(("Queue \"%s\" is not contained in the soft "
           "queue list (-q) that was requested by job %d\n",
               queue_name, (int) job_id));
         soft_violation++;
      }
   }

   /* store number of soft violations in queue */
   lSetUlong(queue, QU_soft_violation, soft_violation);
     
   DEXIT;
   return 0;
}


static int sge_why_not_job2host(lListElem *job, lListElem *ja_task,
                                lListElem *host, lList *complex_list,
                                lList *acl_list) 
{
   lList *projects;
   const char *project;
   u_long32 job_id;
   const char *eh_name;

   DENTER(TOP_LAYER, "sge_why_not_job2host");

   if (!host) {
      DEXIT;
      return 0;
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
      DEXIT;
      return 1;
   }

   /* check if job can run on host based on required projects */
   if ((projects = lGetList(host, EH_prj))) {
   
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "host", eh_name);
         DEXIT;
         return 2;
      }

      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "host", eh_name);
         DEXIT;
         return 3;
      }
   }

   /* check if job can run on host based on excluded projects */
   if ((projects = lGetList(host, EH_xprj))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "host", eh_name);
         DEXIT;
         return 4;
      }
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
            DPRINTF(("RU: Job "u32"."u32" Host "SFN"\n", job_id,
               task_id, eh_name));
            schedd_mes_add(job_id, SCHEDD_INFO_CLEANUPNECESSARY_S,
               eh_name);
            DEXIT;
            return 5;
         }
      }
  } 

   if (ensure_forced(job, !strcmp(eh_name, SGE_GLOBAL_NAME)?SGE_GLOBAL_NAME:SGE_HOST_NAME, 
         "host", eh_name, lGetList(host, EH_complex_list), complex_list)) {
      DEXIT;
      return 8;
   }

   DEXIT;
   return 0;
}

static int is_requested(lList *req, const char *attr) 
{
   lListElem *res;

   for_each (res, req) {  
      if (lGetSubStr(res, CE_name, attr, RE_entries) ||
          lGetSubStr(res, CE_shortcut , attr, RE_entries)) {
         return 1;
      }
   }
   return 0;
}

static int sge_check_load_alarm(char *reason, const char *name, const char *load_value, 
                                const char *limit_value, u_long32 relop, 
                                u_long32 type, lListElem *hep, 
                                lListElem *hlep, double lc_host, 
                                double lc_global, lList *load_adjustments, int load_is_value) 
{
   lListElem *job_load;
   double limit, load;
   int match;
   char lc_diagnosis1[1024], lc_diagnosis2[1024];
   
   DENTER(TOP_LAYER, "sge_check_load_alarm");

   switch (type) {
      case TYPE_INT:
      case TYPE_TIM:
      case TYPE_MEM:
      case TYPE_BOO:
      case TYPE_DOUBLE:
         if (!parse_ulong_val(&load, NULL, type, load_value, NULL, 0)) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDLOAD_SS, load_value, name);
            DEXIT;
            return 1;
         }
         if (!parse_ulong_val(&limit, NULL, type, limit_value, NULL, 0)) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDTHRESHOLD_SS, name, limit_value);
            DEXIT;
            return 1;
         }
         if (load_is_value) { /* we got no load - this is just the complex value */
            strcpy(lc_diagnosis2, MSG_SCHEDD_LCDIAGNOLOAD);
         } else if (((hlep && lc_host) || lc_global) &&
            (job_load = lGetElemStr(load_adjustments, CE_name, name))) { /* load correction */
            const char *load_correction_str;
            double load_correction;

            load_correction_str = lGetString(job_load, CE_stringval);
            if (!parse_ulong_val(&load_correction, NULL, type, load_correction_str, NULL, 0)) {
               if (reason)
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDINVALIDLOADADJUST_SS, name, load_correction_str);
               DEXIT;
               return 1;
            }

            if (hlep) {
               load_correction *= lc_host;

               if (!strncmp(name, "np_", 3)) {
                  int nproc = 1;
                  lListElem *ep_nproc;
                  const char *cp;

                  if ((ep_nproc = lGetSubStr(hep, HL_name, LOAD_ATTR_NUM_PROC, EH_load_list))) {
                     cp = lGetString(ep_nproc, HL_value);
                     if (cp)
                        nproc = MAX(1, atoi(cp));
                  }
                  if (nproc != 1) {
                     load_correction /= nproc;
                  }
                  sprintf(lc_diagnosis1, MSG_SCHEDD_LCDIAGHOSTNP_SFI,
                         load_correction_str, lc_host, nproc);
               } else {
                  sprintf(lc_diagnosis1, MSG_SCHEDD_LCDIAGHOST_SF,
                         load_correction_str, lc_host);
               }
            } else {
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
         } else 
            strcpy(lc_diagnosis2, MSG_SCHEDD_LCDIAGNONE);

         /* is threshold exceeded ? */
         if (resource_cmp(relop, load, limit)) {
            if (reason) {
               if (type == TYPE_BOO)
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDBOOLVALUE_SSSSS,
                        name, load?MSG_TRUE:MSG_FALSE, lc_diagnosis2, map_op2str(relop), limit_value);
               else
                  sprintf(reason, MSG_SCHEDD_WHYEXCEEDFLOATVALUE_SFSSS,
                        name, load, lc_diagnosis2, map_op2str(relop), limit_value);
            }
            DEXIT;
            return 1;
         }
         break;

      case TYPE_STR:
      case TYPE_CSTR:
      case TYPE_HOST:
         if (type==TYPE_STR)
            match = strcmp(limit_value, load_value);
         else  { 
            if (type==TYPE_CSTR)
               match = strcasecmp(limit_value, load_value);
            else
               match = sge_hostcmp(limit_value, load_value);
         }

         if (!match) {
            if (reason)
               sprintf(reason, MSG_SCHEDD_WHYEXCEEDSTRINGVALUE_SSSS, name, load_value, map_op2str(relop), limit_value);
            DEXIT;
            return 1;
         }

         break;
      default:
         if (reason)
            sprintf(reason, MSG_SCHEDD_WHYEXCEEDCOMPLEXTYPE_S, name);
         DEXIT;
         return 1;
   }

   DEXIT;
   return 0;
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
   complex_list and exechost_list get used
   therefore

   returns boolean:
      1 yes, the threshold is exceeded
      0 no
*/

int sge_load_alarm(char *reason, lListElem *qep, lList *threshold, lList *exechost_list, 
                   lList *complex_list, lList *load_adjustments) {
   lListElem *hep, *global_hep, *tep;
   u_long32 ulc_factor; 
   const char *load_value = NULL; 
   const char *limit_value = NULL;
   double lc_host = 0, lc_global = 0;
   int load_is_value = 0;
   
   DENTER(TOP_LAYER, "sge_load_alarm");

   if (!threshold) { 
      /* no threshold -> no alarm */
      DEXIT;
      return 0;
   }

   hep = host_list_locate(exechost_list, lGetHost(qep, QU_qhostname));

   if(!hep) { 
      if (reason)
         sprintf(reason, MSG_SCHEDD_WHYEXCEEDNOHOST_S, lGetHost(qep, QU_qhostname));
      /* no host for queue -> ERROR */
      DEXIT;
      return 1;
   }

   if ((lGetPosViaElem(hep, EH_load_correction_factor) >= 0)
       && (ulc_factor=lGetUlong(hep, EH_load_correction_factor))) {
      lc_host = ((double)ulc_factor)/100;
   }   

   if ((global_hep = host_list_locate(exechost_list, "global")) != NULL) {
      if ((lGetPosViaElem(global_hep, EH_load_correction_factor) >= 0)
          && (ulc_factor=lGetUlong(global_hep, EH_load_correction_factor)))
         lc_global = ((double)ulc_factor)/100;
   }

   for_each (tep, threshold) {
      lListElem *hlep = NULL, *glep = NULL, *cep  = NULL;
      const char *name;
      u_long32 relop, type;

      name = lGetString(tep, CE_name);

      /* complex attriute definition */
      if (!(cep = complex_list_locate_attr(complex_list, name))) {
         if (reason)
            sprintf(reason, MSG_SCHEDD_WHYEXCEEDNOCOMPLEX_S, name);
         /* no complex attribute for threshold -> ERROR */
         DEXIT;
         return 1;
      }

      relop = lGetUlong(cep, CE_relop);

      if (hep != NULL) {
         hlep = lGetSubStr(hep, HL_name, name, EH_load_list);
         if (hlep != NULL) {
            load_value = lGetString(hlep, HL_value);
            load_is_value = 0;
         }
      } else if (global_hep != NULL) {
         glep = lGetSubStr(global_hep, HL_name, name, EH_load_list);
         if (glep != NULL) {
            load_value = lGetString(glep, HL_value);
            load_is_value = 0;
         }
      } 
      if (glep == NULL && hlep == NULL) {
         /* use complex default as value */
         load_value = lGetString(cep, CE_stringval);
         load_is_value = 1;
      }

      limit_value = lGetString(tep, CE_stringval);
      type = lGetUlong(cep, CE_valtype);

      if(sge_check_load_alarm(reason, name, load_value, limit_value, relop, type,
                              hep, hlep, lc_host, lc_global, 
                              load_adjustments, load_is_value)) {
         DEXIT;
         return 1;
      }   
   } 

   DEXIT;
   return 0;
}

/* ----------------------------------------

   sge_load_alarm_reasons() 

   checks given threshold of the queue;
   complex_list and exechost_list get used
   therefore

   fills and returns string buffer containing reasons for alarm states
*/

char *sge_load_alarm_reason(lListElem *qep, lList *threshold, 
                            lList *exechost_list, lList *complex_list, 
                            char *reason, int reason_size, 
                            const char *threshold_type) 
{
   lListElem *hep, *global_hep, *tep;
   const char *load_value; 
   const char *limit_value;
   char buffer[1024];  /* buffer for one line */
   
   DENTER(TOP_LAYER, "sge_load_alarm_reason");

   *reason = 0;

   if (!threshold) { 
      /* no threshold -> no alarm */
      DEXIT;
      return reason;
   }

   hep = host_list_locate(exechost_list, lGetHost(qep, QU_qhostname));

   if(!hep) { 
      /* no host for queue -> ERROR */
      strncat(reason, MSG_SCHEDD_NOHOSTFORQUEUE, reason_size);
      DEXIT;
      return reason;
   }

   global_hep = host_list_locate(exechost_list, "global");

   for_each (tep, threshold) {
      lListElem *hlep = NULL, *glep = NULL, *cep  = NULL;
      const char *name;
      u_long32 relop, type;

      name = lGetString(tep, CE_name);

      /* complex attriute definition */
      if (!(cep = complex_list_locate_attr(complex_list, name))) {
         /* no complex attribute for threshold -> ERROR */
         sprintf(buffer, MSG_SCHEDD_NOCOMPLEXATTRIBUTEFORTHRESHOLD_S, name);
         strncat(reason, buffer, reason_size);
         DEXIT;
         continue;
      }

      relop = lGetUlong(cep, CE_relop);

      if((hlep = lGetSubStr(hep, HL_name, name, EH_load_list)) == NULL) {
         if((glep = lGetSubStr(global_hep, HL_name, name, EH_load_list)) == NULL) {
            /* no host or global load value -> ERROR */
            sprintf(buffer, MSG_SCHEDD_NOLOADVALUEFORTHRESHOLD_S, name);
            strncat(reason, buffer, reason_size);
            DEXIT;
            continue;
         }   
      }

      if (hlep) 
         load_value = lGetString(hlep, HL_value);
      else
         load_value = lGetString(glep, HL_value);

      limit_value = lGetString(tep, CE_stringval);
      type = lGetUlong(cep, CE_valtype);

      if(sge_check_load_alarm(NULL, name, load_value, limit_value, relop, type,
                              hep, hlep, 0, 0,
                              NULL, 0)) {
         char dom_str[5];    /* dominance as string */
         u_long32 dom_val;   /* dominance as u_long */

         if (!(lGetUlong(cep, CE_pj_dominant)&DOMINANT_TYPE_VALUE)) {
            dom_val = lGetUlong(cep, CE_pj_dominant);
         } else {
            dom_val = lGetUlong(cep, CE_dominant);
         }
   
         monitor_dominance(dom_str, dom_val);
         sprintf( buffer, "\talarm %s:%s=%s %s-threshold=%s\n",
                  dom_str,
                  name, 
                  load_value,
                  threshold_type,
                  limit_value
                );
         strncat(reason, buffer, reason_size);
      }   
   } 

   DEXIT;
   return reason;
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
lList **unloaded,    /* QU_Type */
lList **overloaded,  /* QU_Type */
lList *exechost_list, /* EH_Type */
lList *complex_list, /* CX_Type */
lList *load_adjustments, /* CE_Type */
lList *granted,      /* JG_Type */
u_long32 ttype       /* may be QU_suspend_thresholds or QU_load_thresholds */
) {
   lList *thresholds;
   lCondition *where;
   lListElem *qep;
   int ret, load_alarm, nverified = 0;
   char reason[2048];

   DENTER(TOP_LAYER, "sge_split_queue_load");

   /* a job has been dispatched recently,
      but load correction is not in use at all */
   if (granted && !load_adjustments) {
      DEXIT;
      return 0;
   }

   /* tag those queues being overloaded */
   for_each(qep, *unloaded) {
      thresholds = lGetList(qep, ttype);
      load_alarm = 0;

      /* do not verify load alarm anew if a job has been dispatched recently
         but not to the host where this queue resides */
      if (!granted || (granted && (get_global_load_correction() ||
                           lGetElemHost(granted, JG_qhostname, lGetHost(qep, QU_qhostname))))) {
         nverified++;

         if (sge_load_alarm(reason, qep, thresholds, exechost_list, complex_list, load_adjustments)!=0) {
            load_alarm = 1;
            if (ttype==QU_suspend_thresholds) {
               DPRINTF(("queue %s tagged to be in suspend alarm: %s\n", 
                     lGetString(qep, QU_qname), reason));
               schedd_mes_add_global(SCHEDD_INFO_QUEUEINALARM_SS, lGetString(qep, QU_qname), reason);
            } else {
               DPRINTF(("queue %s tagged to be overloaded: %s\n", 
                     lGetString(qep, QU_qname), reason));
               schedd_mes_add_global(SCHEDD_INFO_QUEUEOVERLOADED_SS, lGetString(qep, QU_qname),
reason);
            }
         }
      }
      lSetUlong(qep, QU_tagged4schedule, load_alarm);
   }

   DPRINTF(("verified threshold of %d queues\n", nverified));

   /* split queues in unloaded and overloaded lists */
   where = lWhere("%T(%I == %u)", lGetListDescr(*unloaded), QU_tagged4schedule, 0);
   ret = lSplit(unloaded, overloaded, "overloaded queues", where);
   lFreeWhere(where);

   if (overloaded)
      for_each(qep, *overloaded) /* make sure QU_tagged4schedule is 0 on exit */
         lSetUlong(qep, QU_tagged4schedule, 0);
   if (ret) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

/* ----------------------------------------

   sge_split_queue_nslots()

   splits the incoming queue list (1st arg) into queues with free slots and
   queues without free slots (2nd arg) 

   returns:
      0 successful
     -1 errors in functions called by sge_split_queue_load

*/
int sge_split_queue_nslots_free(
lList **free,        /* QU_Type */
lList **full,        /* QU_Type */
int nslots 
) {
   lList *lp = NULL;
   int do_free_list = 0;
   lListElem *this, *next;

   DENTER(TOP_LAYER, "sge_split_queue_nslots_free");

   if (!free) {
      DEXIT;
      return -1;
   }

   if (!full) {
       full = &lp;
       do_free_list = 1;
   }

   for (this=lFirst(*free); ((next=lNext(this))), this ; this = next) {
      if ((qslots_used(this)+nslots) > (int) lGetUlong(this, QU_job_slots)) {
         /* chain 'this' into 'full' list */
         this = lDechainElem(*free, this);
         if (full) {
            if (!*full)
               *full = lCreateList("full one", lGetListDescr(*free));
            lAppendElem(*full, this);
         } else 
            lFreeElem(this);
      }
   }

   if (*full) {
      lListElem* mes_queue;

      for_each(mes_queue, *full)
         schedd_mes_add_global(SCHEDD_INFO_QUEUEFULL_, lGetString(mes_queue, QU_qname));

      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESFULLANDDROPPED , *full, QU_qname);
      if (do_free_list) {
         lFreeList(*full);
         *full = NULL;
      }
   }

   DEXIT;
   return 0;
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
   int do_free_list = 0;

   DENTER(TOP_LAYER, "sge_split_suspended");

   if (!queue_list) {
      DEXIT;
      return -1;
   }

   if (!suspended) {
       suspended = &lp;
       do_free_list = 1;
   }

   /* split queues */
   where = lWhere("%T(!(%I m= %u) && !(%I m= %u) && !(%I m= %u))", 
      lGetListDescr(*queue_list), 
         QU_state, QSUSPENDED,
         QU_state, QCAL_SUSPENDED,
         QU_state, QSUSPENDED_ON_SUBORDINATE);
   ret = lSplit(queue_list, suspended, "full queues", where);
   lFreeWhere(where);

   if (*suspended) {
      lListElem* mes_queue;

      for_each(mes_queue, *suspended)
         schedd_mes_add_global(SCHEDD_INFO_QUEUESUSP_, lGetString(mes_queue, QU_qname));
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESSUSPENDEDANDDROPPED , *suspended, QU_qname);
      if (do_free_list) {
         lFreeList(*suspended);
         *suspended = NULL;
      }
   }

   DEXIT;
   return ret;
}


/* ----------------------------------------

   sge_split_disabled()

   splits the incoming queue list (1st arg) into non disabled queues and
   disabled queues (2nd arg) 

   returns:
      0 successful
     -1 errors in functions called by sge_split_queue_load

*/
int sge_split_disabled(
lList **queue_list,        /* QU_Type */
lList **disabled         /* QU_Type */
) {
   lCondition *where;
   int ret;
   lList *lp = NULL;
   int do_free_list = 0;

   DENTER(TOP_LAYER, "sge_split_disabled");

   if (!queue_list) {
      DEXIT;
      return -1;
   }

   if (!disabled) {
       disabled = &lp;
       do_free_list = 1;
   }

   /* split queues */
   where = lWhere("%T(!(%I m= %u) && !(%I m= %u))", 
      lGetListDescr(*queue_list), 
         QU_state, QDISABLED,
         QU_state, QCAL_DISABLED);
   ret = lSplit(queue_list, disabled, "full queues", where);
   lFreeWhere(where);

   if (*disabled) {
      lListElem* mes_queue;

      for_each(mes_queue, *disabled)
         schedd_mes_add_global(SCHEDD_INFO_QUEUEDISABLED_, lGetString(mes_queue, QU_qname));
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED , *disabled, QU_qname);
      if (do_free_list) {
         lFreeList(*disabled);
         *disabled = NULL;
      }
   }

   DEXIT;
   return ret;
}

/****** sched/select_queue/sge_replicate_queues_suitable4job() ****************
*  NAME
*     sge_replicate_queues_suitable4job() -- select res. for a job
*
*  RESULT
*     A JG_Type list refering the selected resources (queues) for 
*     that job
******************************************************************************/
lList* sge_replicate_queues_suitable4job(
lList *queues,       /* QU_Type */
lListElem *job,      /* JB_Type */
lListElem *ja_task,  /* JAT_Type */
lListElem *pe,       /* PE_Type */
lListElem *ckpt,     /* CK_Type */
int queue_sort_method,     
lList *complex_list, /* CX_Type */
lList *host_list,    /* EH_Type */
lList *acl_list,     /* US_Type */
lList *load_adjustments, /* CE_Type */
int ndispatched,
int *last_dispatch_type, 
int host_order_changed) {
   int allocation_rule = 0, total_slots;
   int minslots = 0;
   int need_master_host;
   u_long32 job_id;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_replicate_queues_suitable4job");

   if (!job || !queues || !complex_list || !host_list) {
      DEXIT;
      return NULL;
   }
   
   need_master_host = (lGetList(job, JB_master_hard_queue_list)!=NULL);
   job_id = lGetUlong(job, JB_job_number);

   {
      lListElem *hep, *qep, *global_hep;
      lList *global_resources = NULL, *host_resources = NULL;
      int success = 0;
      lList *ccl[3];
      int queue_slots, accu_queue_slots;
      int host_slots;

      double previous_load = 0;
      int previous_load_inited = 0;
      int host_seqno = 0;

      int max_slots_all_hosts, accu_host_slots;
      int have_master_host, suited_as_master_host;
      const char *eh_name, *qname;

      /* untag all queues */
      queue_list_clear_tags(queues);

      global_hep = host_list_locate(host_list, "global");

      ccl[0] = lGetList(global_hep, EH_consumable_config_list);
      ccl[1] = NULL;
      ccl[2] = NULL;

      if (!pe && !lGetList(job, JB_soft_resource_list) &&
                 !lGetList(job, JB_soft_queue_list)) {
         /*------------------------------------------------------------------
          *  FAST TRACK FOR SEQUENTIAL JOBS WITHOUT A SOFT REQUEST 
          *
          *  It is much faster not to review slots in a comprehensive fashion 
          *  for jobs of this type.
          * ------------------------------------------------------------------*/
         if(host_order_changed) {
            lListElem *hep, *qep;
            double previous_load = 0;
            int previous_load_inited = 0;
            int host_seqno = 0;
            const char *eh_name;

            host_order_changed = 0;

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
                  DPRINTF(("HOST SORT ORDER CHANGED FOR HOST %s FROM %d to %d\n", eh_name, lGetUlong(hep, EH_seq_no), host_seqno));
                  host_order_changed = 1;
                  lSetUlong(hep, EH_seq_no, host_seqno);
               }
            }
         }   
         if (get_qs_state()!=QS_STATE_EMPTY) {
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
            if (*last_dispatch_type != DISPATCH_TYPE_FAST || host_order_changed) {
               DPRINTF(("SORTING HOSTS!\n"));
               if (queue_sort_method == QSM_LOAD)
                  lPSortList(queues, "%I+ %I+", QU_host_seq_no, QU_seq_no);
               else
                  lPSortList(queues, "%I+ %I+", QU_seq_no, QU_host_seq_no);
            }

            *last_dispatch_type = DISPATCH_TYPE_FAST;

         }

         /* do static and dynamic checks for "global" host */ 
         if (!sge_why_not_job2host(job, ja_task, global_hep, complex_list, acl_list)) {
            for_each (qep, queues) { /* in queue sort order */
               hep = host_list_locate(host_list, lGetHost(qep, QU_qhostname));
               ccl[1] = lGetList(hep, EH_consumable_config_list);
               ccl[2] = lGetList(qep, QU_consumable_config_list);

               if (!sge_why_not_job2host(job, ja_task, hep, complex_list, acl_list) &&
                     available_slots_at_queue(NULL, job, qep, pe, ckpt, host_list, complex_list, acl_list,
                              load_adjustments, 1, ccl, ndispatched, global_hep, 1, hep)) {
                  lListElem *gdil_ep;
                  lList *gdil = NULL;

                  qname = lGetString(qep, QU_qname);
                  eh_name = lGetHost(qep, QU_qhostname);

                  DPRINTF((u32": 1 slot in queue %s@%s user %s\n",
                     job_id, qname, eh_name, lGetString(job, JB_owner)));

                  gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
                  lSetUlong(gdil_ep, JG_qversion, lGetUlong(qep, QU_version));
                  lSetHost(gdil_ep, JG_qhostname, eh_name);
                  lSetUlong(gdil_ep, JG_slots, 1);
                  scheduled_fast_jobs++;
                  DEXIT;
                  return gdil;
               }
            }
         }
         DEXIT;
         return NULL;
      } else {
         /*------------------------------------------------------------------
          *  REVIEW ALL AVAILABLE SLOTS FOR THAT JOB COMPREHENSIVELY
          *
          *  Concept in use for unified scheduling for sequential and parallel 
          *  jobs under consideration of the allocation rule:
          *
          *  It is done by tagging the amount of available slots for that
          *  job at global, host and queue level. We also mark queues suitable 
          *  for being master queue as possible master queues and down in
          *  available_slots_at_queue() we count the number of violations of
          *  the job's soft request.
          *
          *  At the same time we fill into QU_host_seq_no a sequence number
          *  representing the host suitability due to the placement from
          *  the load formula.
          * 
          *  Deciding which queues are suitable in a general fashion becomes
          *  costly: 
          *
          *    Before the introduction of consumable resources it was 
          *    convenient to check all queues separately whether their 
          *    resource limit is enough for a parallel job. With consumable
          *    resources this scheme is no longer allowed. This scheme would
          *    dispatch a consumable resource limited at host level to two 
          *    queues and thus would dispatch the resource twice. 
          *
          *    Since consumable resources are also possible at "global" host
          *    level (i.e. sliding licenses) the first step must be to 
          *    decide how many slots can get served with the globally limited
          *    resources. This amount must comply with the slot range 
          *    ("-pe <pe> <slot range>") of the jobs request. In case it
          *    is not possible to find a number 'total_slots' satisfying 
          *    both slot range and globally available resources the job
          *    is not schedulable actually.
          *
          *    Then we have to play the same game for each host. The aim
          *    is to get in knowledge the amount of slots that can get
          *    served at host level.
          *
          *    If the resources limited at host level are enough according the 
          *    allocation rule we visit each queue residing at the host.
          *
          *    Once again we ask: How many slots can we get from this
          *    queue according the consumable resources? This amount
          *    gets tagged in the queue and is summed up in 'accu_queue_slots'
          *
          *    The minimum of 'accu_queue_slots' and the amount of slots
          *    that can served at host level results in the amount of
          *    slots available at this host. The host is not a candidate 
          *    for scheduling if this amount is below the necessary amount 
          *    per host given with the allocation rule. If it is a candidate
          *    the amount gets tagged in the host and it increases 
          *    'accu_host_slots'.
          *
          *    Having visited all hosts and accumulated slots in 'accu_host_slots'
          *    we know how many slots are available and we have a list of 
          *    hosts/queues with tagged fields containing the amount of slots 
          *    available.
          *
          * ------------------------------------------------------------------ */
         *last_dispatch_type = DISPATCH_TYPE_COMPREHENSIVE;

         if (get_qs_state()==QS_STATE_EMPTY) { /* no caching */
            global_complexes2scheduler(&global_resources, global_hep, complex_list, 0);
         } else {
            if (!(global_resources=lGetList(global_hep, EH_cached_complexes))) {
/*                DPRINTF(("--- GLOBAL\n")); */
               global_complexes2scheduler(&global_resources, global_hep, complex_list, 0);
               lSetList(global_hep, EH_cached_complexes, global_resources);
               lSetUlong(global_hep, EH_cache_version, ndispatched);
            } else {
               if ((int) lGetUlong(global_hep, EH_cache_version)!=ndispatched) {
/*                   DPRINTF(("=== GLOBAL\n")); */
                  /* recompute attributes only if meanwhile job has been dispatched */
                  global_complexes2scheduler(&global_resources, global_hep, complex_list, 1);
                  lSetUlong(global_hep, EH_cache_version, ndispatched);
               } else {
/*                   DPRINTF(("+++ GLOBAL\n")); */
               }
            }
         }


         /* iterate through all global amounts of slots */
         for (total_slots = (pe ?  
               (get_qs_state()==QS_STATE_EMPTY ?  lGetUlong(pe, PE_slots): 
                (lGetUlong(pe, PE_slots) - lGetUlong(pe, PE_used_slots)))
               :1)
              ;!success && 
                  (total_slots = available_slots_global(&global_resources, 
                        job, ja_task,
                        pe, host_list, complex_list, total_slots, ccl, acl_list))
              ; total_slots = (max_slots_all_hosts!=0) ?  
                  MIN(max_slots_all_hosts, total_slots-1):
                  total_slots-1) {


            DPRINTF((u32": global %d slots\n", job_id, total_slots));
            
            if (!(allocation_rule = sge_pe_slots_per_host(pe, total_slots))) {
               max_slots_all_hosts = total_slots;
               schedd_mes_add(job_id, SCHEDD_INFO_PEALLOCRULE_S, lGetString(pe, PE_name));
               continue;
            }
            minslots = ALLOC_RULE_IS_BALANCED(allocation_rule)?allocation_rule:1;

            /* remove reasons from last unsuccesful iteration */ 
            clean_monitor_alp();

            accu_host_slots = 0;
            have_master_host = 0;
            max_slots_all_hosts = 0;

            /* first select hosts with lowest share/load 
               and then select queues with */
            /* tag amount of slots we can get served with resources limited per host */
            for_each (hep, host_list) {

               int max_slots_this_host = 0;

               eh_name = lGetHost(hep, EH_name);
               if (!strcasecmp(eh_name, "global") || !strcasecmp(eh_name, "template"))
                  continue;

               /* do not perform expensive checks for this host if there 
                * is not at least one free queue residing at this host:
                * see if there are queues which are not disbaled/suspended/calender;
                * which have at least one free slot, which are not unknown, several alarms
                */  
               if (!(qep=lGetElemHost(queues, QU_qhostname, eh_name)))
                  continue;   

               ccl[1] = lGetList(hep, EH_consumable_config_list);

               if (get_qs_state()==QS_STATE_EMPTY) { /* no caching */
                  host_complexes2scheduler(&host_resources, hep, host_list, complex_list, 0);
               } else {
                  if (!(host_resources=lGetList(hep, EH_cached_complexes))) {
/*                      DPRINTF(("--- HOST %s\n", eh_name)); */
                     host_resources = lCopyList(eh_name, global_resources); 
                     host_complexes2scheduler(&host_resources, hep, host_list, complex_list, 0);
                     lSetList(hep, EH_cached_complexes, host_resources);
                     lSetUlong(hep, EH_cache_version, ndispatched);
                  } else {
                     if ((int) lGetUlong(hep, EH_cache_version)!=ndispatched) {
/*                         DPRINTF(("=== HOST %s\n", eh_name)); */
                        /* recompute attributes only if meanwhile job has been dispatched */
                        host_complexes2scheduler(&host_resources, hep, host_list, complex_list, 1);
                        lSetUlong(hep, EH_cache_version, ndispatched);
                     } else {
/*                         DPRINTF(("+++ HOST %s\n", eh_name)); */
                     }
                  }
               }
         
               /* try it decreasinly for each slots amount which makes sense  
                *  
                * the loop body is passed multiple times only in case of non 
                * balanced allocation rules like 
                * $fill_up and $round_robin where we do not know in 
                * advance the exact number of needed slots 
                *  
                */ 
               suited_as_master_host = 0;
               for ( host_slots = ALLOC_RULE_IS_BALANCED(allocation_rule)?
                         allocation_rule:total_slots;
                     (host_slots = available_slots_at_host(host_resources, 
                         job, ja_task, hep, host_slots, minslots, 
                         allocation_rule, ccl, complex_list, acl_list));
                     host_slots--) {
                  max_slots_this_host = host_slots;
                  accu_queue_slots = 0;

                  if (host_slots>=minslots) {
                     /* tag amount of slots we can get served with resources limited per queue */
                     for_each (qep, queues) {
                        if (sge_hostcmp(lGetHost(qep, QU_qhostname), eh_name))
                           continue;
                        qname = lGetString(qep, QU_qname);

                        ccl[2] = lGetList(qep, QU_consumable_config_list);
       
                        if ((queue_slots = available_slots_at_queue(host_resources, job, qep, pe, 
                                 ckpt, host_list, complex_list, acl_list, load_adjustments, host_slots, ccl, ndispatched, 
                                       global_hep, total_slots, hep))) {   
                           /* in case the load of two hosts is equal this
                              must be also reflected by the sequence number */
                           if (previous_load_inited && (previous_load < lGetDouble(hep, EH_sort_value)))
                              host_seqno++;
                           else {
                              if (!previous_load_inited) {
                                 previous_load_inited = 1;
                              } else
                                 DPRINTF(("SKIP INCREMENTATION OF HOST_SEQNO\n"));
                           }
                           previous_load = lGetDouble(hep, EH_sort_value);

                           /* could this host be a master host */
                           if (!suited_as_master_host && lGetUlong(qep, QU_tagged4schedule)) {
                              DPRINTF(("HOST %s can be master host because of queue %s\n", eh_name, qname));    
                              suited_as_master_host = 1; 
                           }

                           /* prepare sort by sequence number of queues */ 
                           lSetUlong(qep, QU_host_seq_no, host_seqno);
                           accu_queue_slots += queue_slots;

                        }
                        lSetUlong(qep, QU_tagged, queue_slots);
                     } /* for each queue */

                  } /* if enough slots at host */

                  if (accu_queue_slots >= minslots) {
                     if (allocation_rule>0)
                        host_slots = allocation_rule;
                     host_slots = MIN(accu_queue_slots, host_slots);
                     DPRINTF(("HOST %s %d slots\n", eh_name, host_slots));
                     accu_host_slots += host_slots;
                     DPRINTF(("accu_host_slots %d\n", accu_host_slots));
                     break;
                  } else {
                     DPRINTF(("CAN'T SERVE MORE THAN %d SLOTS AT HOST %s\n", max_slots_this_host, eh_name));
                  }
               }

               max_slots_all_hosts += max_slots_this_host;

               /* tag full amount or zero */
               lSetUlong(hep, EH_tagged, host_slots); 
               lSetUlong(hep, EH_master_host, suited_as_master_host); 
               if (suited_as_master_host)
                  have_master_host = 1;

               if (get_qs_state()==QS_STATE_EMPTY) { /* no caching */
                  host_resources = lFreeList(host_resources);
               } else 
                  host_resources = NULL; /* reference is saved in host */

            } /* for each host */

            if (accu_host_slots >= total_slots && 
               (!need_master_host || (need_master_host && have_master_host))) {
               success = 1;
               /* stop looking for smaller slot amounts */
               DPRINTF(("-------------->      BINGO %d slots %s  <--------------\n", 
                     total_slots, need_master_host?"plus master host":""));
               break;
            }

         } /* for all slot amounts */

         if (get_qs_state()==QS_STATE_EMPTY) /* no caching */
            global_resources = lFreeList(global_resources);
         else 
            global_resources = NULL; /* reference is saved in global host */

         if (!success) {
            DEXIT;
            return NULL;
         }

         /*------------------------------------------------------------------
          * SORTING THE SUITABLE QUEUES 
          * 
          * This is not needed in qmaster who calls us in state QS_STATE_EMPTY.
          *
          * While visiting all the hosts and queues we have filled the fields
          * with values containing 
          *   - the number of violations of the jobs soft request 
          *     (QU_soft_violation)
          *   - the sequence number of the host in the sorted host list 
          *     (QU_host_seq_no)
          *   - the number of tagged slots in the queue (QU_tagged)
          *     (-> We prefer queues with many slots because we dont want 
          *     to distribute the slots to multiple queues if not necessary.
          *     If this is not convenient then it will be possible to use
          *     the queues seq. number to override this behaviour also in 
          *     case of sort by load/share (new!))
          *   - QU_seq_no is already valid
          *   The valency of these fields depends on the queue sort order 
          * 
          *   QSM_LOAD
          *   QSM_SHARE
          *      1. QU_soft_violation
          *      2. QU_host_seq_no 
          *      3. QU_seq_no 
          *      4. QU_tagged
          * 
          *    QSM_SEQNUM
          *      1. QU_soft_violation
          *      2. QU_seq_no 
          *      3. QU_host_seq_no 
          *      4. QU_tagged
          *------------------------------------------------------------------*/
        
         if (get_qs_state()!=QS_STATE_EMPTY) {
            if (queue_sort_method == QSM_LOAD)
               lPSortList(queues, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_host_seq_no, QU_seq_no, QU_tagged);
            else
               lPSortList(queues, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_seq_no, QU_host_seq_no, QU_tagged);

#if 0
            /* monitor queue sorting */
            if  (queue_sort_method == QSM_LOAD)
               DPRINTF(("QUEUE               \tMASTER\tSOFT\tLOAD\tSEQNO\tTAGGED\n"));
            else
               DPRINTF(("QUEUE               \tMASTER\tSOFT\tSEQNO\tLOAD\tTAGGED\n"));
            for_each (qep, queues)
               if (lGetUlong(qep, QU_tagged)) {
                  if (queue_sort_method == QSM_LOAD)
                     DPRINTF(("%-20.20s\t%s\t%d\t%d\t%d\t%d\n", 
                        lGetString(qep, QU_qname),
                        lGetUlong(qep, QU_tagged4schedule)?"master":"slave",
                        lGetUlong(qep, QU_soft_violation),
                        lGetUlong(qep, QU_host_seq_no),
                        lGetUlong(qep, QU_seq_no),
                        lGetUlong(qep, QU_tagged)));
                  else
                     DPRINTF(("%s %s %d %d %d %d\n", 
                        lGetString(qep, QU_qname),
                        lGetUlong(qep, QU_tagged4schedule)?"master":"slave",
                        lGetUlong(qep, QU_soft_violation),
                        lGetUlong(qep, QU_seq_no),
                        lGetUlong(qep, QU_host_seq_no),
                        lGetUlong(qep, QU_tagged)));
               }         
#endif
         }
      }
   }

   /*------------------------------------------------------------------
    * SELECT SLOT(S) IN QUEUE(S)
    *
    * build up a granted destination identifier list
    *
    * We enter selection code with a queuelist sorted according 
    * 'sort_formula' and 'queue_sort_method'. But for the placement of 
    * parallel jobs it is necessary to select hosts and then select
    * queues. Thus we use the sorted queue list to find the best suited 
    * host for the job, the second best host and so on.
    *
    * Then we iterate through the hosts starting with the best suited 
    * and allocate slots of the best queues at each host according  
    * our allocation rule.
    * 
    *------------------------------------------------------------------*/
   {
      int max_host_seq_no, start_seq_no, last_accu_host_slots, accu_host_slots = 0;
      int host_slots;
      lList *gdil = NULL;
      const char *eh_name;
      const char *qname;
      lListElem *hep, *qep;

      int host_seq_no = 1;

            
      /* derive suitablility of host from queues suitability */
      for_each (hep, host_list) 
         lSetUlong(hep, EH_seq_no, -1);

      DPRINTF(("minslots = %d\n", minslots));
      for_each (qep, queues) {

         if (!lGetUlong(qep, QU_tagged)) 
            continue;

         /* ensure host of this queue has enough slots */
         eh_name = lGetHost(qep, QU_qhostname);
         hep = host_list_locate(host_list, eh_name);
         if ((int) lGetUlong(hep, EH_tagged) >= minslots && 
               (int) lGetUlong(hep, EH_seq_no)==-1) {
            lSetUlong(hep, EH_seq_no, host_seq_no++);
            DPRINTF(("%d. %s selected! %d\n", lGetUlong(hep, EH_seq_no), eh_name, lGetUlong(hep, EH_tagged)));
         } else {
            DPRINTF(("%d. %s (%d tagged)\n", lGetUlong(hep, EH_seq_no), eh_name, lGetUlong(hep, EH_tagged)));
         }
      }
      max_host_seq_no = host_seq_no;

      /* find best suited master host */ 
      if (need_master_host) {
         lListElem *master_hep = NULL;
         const char *master_eh_name;

         /* find master host with the lowest host seq no */
         for_each (hep, host_list) {
            if (lGetUlong(hep, EH_seq_no) != -1 && lGetUlong(hep, EH_master_host)) {
               if (!master_hep || lGetUlong(hep, EH_seq_no) < lGetUlong(master_hep, EH_seq_no) ) {
                  master_hep = hep;
               }
            }
         }

         /* should be impossible to reach here without a master host */
         if (!master_hep) { 
            ERROR((SGE_EVENT, "no master host for job "u32"\n", 
               lGetUlong(job, JB_job_number)));
            DEXIT;
            return NULL;
         }

         /* change order of queues in a way causing the best suited master 
            queue of the master host to be at the first position */
         master_eh_name = lGetHost(master_hep, EH_name);
         for_each (qep, queues) {
            if (sge_hostcmp(master_eh_name, lGetHost(qep, QU_qhostname)))
               continue;
            if (lGetUlong(qep, QU_tagged4schedule))
               break;
         }
         lDechainElem(queues, qep);
         lInsertElem(queues, NULL, qep);

         DPRINTF(("MASTER HOST %s MASTER QUEUE %s\n", 
               master_eh_name, lGetString(qep, QU_qname)));
         /* this will cause the master host to be selected first */
         lSetUlong(master_hep, EH_seq_no, 0);
         start_seq_no = 0;
      } else
         start_seq_no = 1;

      do { /* loop only needed round robin allocation rule */
         last_accu_host_slots = accu_host_slots;

         for (host_seq_no = start_seq_no; host_seq_no<max_host_seq_no; host_seq_no++) { /* iterate through hosts */
            int available, slots;
            lListElem *gdil_ep;

            if (!(hep=lGetElemUlong(host_list, EH_seq_no, host_seq_no))) {
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
            } else if (allocation_rule==ALLOC_RULE_FILLUP) {
               host_slots = available;
            } else 
               host_slots = allocation_rule;
            lSetUlong(hep, EH_tagged, available - host_slots);

            DPRINTF(("allocating %d of %d slots at host %s (seqno = %d)\n", 
                  host_slots, total_slots, eh_name, host_seq_no));

            for_each (qep, queues) {
               int qtagged;

               if (sge_hostcmp(eh_name, lGetHost(qep, QU_qhostname)))
                  continue;

               qname = lGetString(qep, QU_qname);
               /* how many slots ? */
               qtagged = lGetUlong(qep, QU_tagged);
               slots = MIN(total_slots-accu_host_slots, 
                  MIN(host_slots, qtagged));
               accu_host_slots += slots;
               host_slots -= slots;

               /* build gdil for that queue */
               DPRINTF((u32": %d slots in queue %s@%s user %s (host_slots = %d)\n", 
                  job_id, slots, qname, eh_name, lGetString(job, JB_owner), host_slots));
               if (!(gdil_ep=lGetElemStr(gdil, JG_qname, qname))) {
                  gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
                  lSetUlong(gdil_ep, JG_qversion, lGetUlong(qep, QU_version));
                  lSetHost(gdil_ep, JG_qhostname, eh_name);
                  lSetUlong(gdil_ep, JG_slots, slots);
               } else 
                  lSetUlong(gdil_ep, JG_slots, lGetUlong(gdil_ep, JG_slots) + slots);

               /* untag */
               lSetUlong(qep, QU_tagged, qtagged - slots);

               if (!host_slots) 
                  break; 
            }
         }

         DPRINTF(("- - - accu_host_slots %d total_slots %d\n", accu_host_slots, total_slots));
         if (last_accu_host_slots == accu_host_slots) {
            DPRINTF(("!!! NO MORE SLOTS !!!\n"));
            lFreeList(gdil); 
            DEXIT;
            return NULL;
         }
      } while (allocation_rule==ALLOC_RULE_ROUNDROBIN && accu_host_slots < total_slots);

      scheduled_complex_jobs++;
      DEXIT;
      return gdil;
   }
}


/****** sched/select_queue/available_slots_at_queue() *************************
*  NAME
*     available_slots_at_queue() -- return # of slots in queue for job
*
*  FUNCTION
*     Returns the number of available slots in this queue for the job.
*     Note that this function will not look for more slots than needed 
*     by the job.
*
*  RESULT
*     int - the # of slots
******************************************************************************/
int available_slots_at_queue(host_resources, job, qep, pe, ckpt, host_list, 
complex_list, acl_list, load_adjustments, host_slots, ccl, ndispatched, 
global_hep, total_slots, hep)
lList *host_resources;
lListElem *job;
lListElem *qep;
lListElem *pe;
lListElem *ckpt;
lList *host_list;
lList *complex_list;
lList *acl_list;
lList *load_adjustments;
int host_slots; /* maximum amount of slots at this host */
lList *ccl[3];
int ndispatched;
lListElem *global_hep;
int total_slots; /* global amount of slots we want to dispatch */
lListElem *hep;
{
   int qslots;
   lListElem *cep;
   const char *qname; 
   char reason[1024];
   lList *queue_resources = NULL;
   u_long32 job_id;

   DENTER(TOP_LAYER, "available_slots_at_queue");

   if (sge_why_not_job2queue_static(qep, job, pe, ckpt, complex_list, host_list, acl_list)) {
      DEXIT;
      return 0;
   }

   job_id = lGetUlong(job, JB_job_number);
   qname = lGetString(qep, QU_qname);

   /* ensure we are causing no load alarm - but only if 
      we are thinking about more than one slot 
       
      We may not used cached attributes for this purpose!
      Load is higher because of 'look ahead' load correction.
   */
   if (host_slots>1 && get_qs_state()!=QS_STATE_EMPTY) {
      u_long32 old_host_lc_factor = 0, old_global_lc_factor = 0;
      int load_alarm;

      /* do global load correction for total_slots-1 slots */
      if (global_hep) {
         old_global_lc_factor = lGetUlong(global_hep, EH_load_correction_factor); 
         lSetUlong(global_hep, EH_load_correction_factor, 
                  (total_slots-1)*100 + old_global_lc_factor);
         /* debit global consumables */
         debit_host_consumable(job, global_hep, complex_list, total_slots);
      } 

      if (hep) {
         /* do load correction for host_slots-1 slots */
         old_host_lc_factor = lGetUlong(hep, EH_load_correction_factor); 
         lSetUlong(hep, EH_load_correction_factor, (host_slots-1)*100 + old_host_lc_factor);
         /* debit host consumables */
         debit_host_consumable(job, hep, complex_list, host_slots);
      }

      load_alarm = sge_load_alarm(NULL, qep, lGetList(qep, QU_load_thresholds), 
            host_list, complex_list, load_adjustments);

      if (global_hep) {
         /* undebit global consumables */
         debit_host_consumable(job, global_hep, complex_list, -total_slots);
         /* undo virtual load correction */ 
         lSetUlong(global_hep, EH_load_correction_factor, old_global_lc_factor);
      }

      if (hep) {
         /* undebit host consumables */
         debit_host_consumable(job, hep, complex_list, -host_slots);
         /* undo load correction */
         lSetUlong(hep, EH_load_correction_factor, old_host_lc_factor);
      }

      if (load_alarm) { 
         DPRINTF(("%s (%d global slots/%d host slots) would set queue \"%s\" in load alarm state\n", 
               job_descr(job_id), total_slots, host_slots, qname));
         schedd_mes_add(job_id, SCHEDD_INFO_WOULDSETQEUEINALARM_DS,
            host_slots, qname);
         DEXIT;
         return 0;
      }
   }

   if (get_qs_state()==QS_STATE_EMPTY) {
      queue_complexes2scheduler(&queue_resources, qep, host_list, complex_list, 0);
   } else {
      if (!(queue_resources=lGetList(qep, QU_cached_complexes))) {
         DPRINTF(("--- QUEUE %s\n", qname));
         queue_resources = lCopyList(qname, host_resources); 
         queue_complexes2scheduler(&queue_resources, qep, host_list, complex_list, 0);
         lSetList(qep, QU_cached_complexes, queue_resources);
         lSetUlong(qep, QU_cache_version, ndispatched);
      } else {
         if ((int) lGetUlong(qep, QU_cache_version)!=ndispatched) {
            DPRINTF(("=== QUEUE %s\n", qname));
            queue_complexes2scheduler(&queue_resources, qep, host_list, complex_list, 1);
            lSetUlong(qep, QU_cache_version, ndispatched);
         } else {
            DPRINTF(("+++ QUEUE %s\n", qname));
         }
      }
   }

   /* get QU_job_slots_used of queue */
   if (!(cep=lGetElemStr(queue_resources, CE_name, "slots"))) {
      DEXIT;
      return 0;
   }
   if (!(qslots = lGetDouble(cep, CE_doubleval))) {
      schedd_mes_add(job_id, SCHEDD_INFO_NOSLOTSINQUEUE_S, qname);
   }

   /* get QU_job_slots of queue */
   get_util_max(cep, ccl);

   qslots = MIN(host_slots, qslots);

   for (; qslots; qslots--) {

      /* check if queue fulfills hard request of the job */
      if (!sge_select_queue(queue_resources, lGetList(job, JB_hard_resource_list), 
               0, reason, sizeof(reason)-1, qslots, ccl)) {

         if (qslots==1) {
            char buff[1024 + 1];
            unparse_resources(NULL, buff, sizeof(buff) - 1,
                     lGetList(job, JB_hard_resource_list));
            if (*buff && (buff[strlen(buff) - 1] == '\n'))
               buff[strlen(buff) - 1] = 0;
            schedd_mes_add(job_id, SCHEDD_INFO_CANNOTRUNINQUEUE_SSS, buff, qname, reason);
            
         } 
         continue;
      }

      if (sge_soft_violations(queue_resources, qep, job, pe, complex_list, ccl)) {
         continue;
      }

      break;
   }

   if (get_qs_state()==QS_STATE_EMPTY) {
      lFreeList(queue_resources);
   } 

   DEXIT;
   return qslots;
}


/****** sched/select_queue/available_slots_at_host() **************************
*  NAME
*     available_slots_at_host() -- return # of slots at host for job
*
*  FUNCTION
*     Returns the number of available slots at this host for the job.
*     Note that this function will not look for more slots than needed 
*     by the job.
*
*  RESULT
*     int - the # of slots
******************************************************************************/
static int available_slots_at_host(lList *host_resources, lListElem *job,
                                   lListElem *ja_task, lListElem *host,
                                   int hslots, int minslots, 
                                   int allocation_rule, lList *ccl[3],
                                   lList *complex_list, lList *acl_list) 
{
   lListElem *cep;
   char reason[1024];
   const char *eh_name;
   u_long32 job_id;

   DENTER(TOP_LAYER, "available_slots_at_host");

   job_id = lGetUlong(job, JB_job_number);
   eh_name = lGetHost(host, EH_name);

   /* check if job has access to host */
   if (sge_why_not_job2host(job, ja_task, host, complex_list, acl_list)>0) {
      DEXIT;
      return 0;
   }

   /* get job_slots_free from host */
   if ((cep=lGetElemStr(host_resources, CE_name, "slots")) && 
   !(lGetUlong(cep, CE_dominant) & DOMINANT_TYPE_VALUE)) {
      /* we hope that slots are limited at queue level */
      /* no limitations at host level */
      hslots = MIN((u_long32)hslots, (u_long32)lGetDouble(cep, CE_doubleval));
   }

   for (; hslots>=minslots; hslots--) {
      /* check if host fulfills hard request of the job */
      if (!sge_select_resource(host_resources, lGetList(job, JB_hard_resource_list), 
            0, reason, sizeof(reason)-1, hslots, ccl, 0)) {

         if (hslots==minslots) {
            char buff[1024 + 1];
            unparse_resources(NULL, buff, sizeof(buff) - 1,
                     lGetList(job, JB_hard_resource_list));
            if (*buff && (buff[strlen(buff) - 1] == '\n'))
               buff[strlen(buff) - 1] = 0;

            schedd_mes_add(job_id, SCHEDD_INFO_CANNOTRUNATHOST_SSS, 
               buff, eh_name, reason);
         }
/*          DPRINTF(("HOST failed %s %d slots: %s\n", eh_name, hslots, reason));    */
         continue;
      }
      break;
   }

   DEXIT;
   return (hslots<minslots)?0:hslots;
}

/****** sched/select_queue/available_slots_global() ***************************
*  NAME
*     available_slots_global() -- return # of global slots for job
*
*  FUNCTION
*     Returns the number of globally available slots for the job.
*     Note that this function will not look for more slots than 
*     needed by the job.
*
*  RESULT
*     int - the # of slots
******************************************************************************/
static int available_slots_global(global_resources, job, ja_task, pe_object, host_list, complex_list, global_slots, ccl, acl_list)
lList **global_resources;
lListElem *job;
lListElem *ja_task; /* either JAT_Type or NULL */
lListElem *pe_object;
lList *host_list;
lList *complex_list;
int global_slots; /* 0 reinitializes other values serve as the first start value for checks */
lList *ccl[3];
lList *acl_list;
{
   char reason[1024];
   int not_select_resource = 0;

   DENTER(TOP_LAYER, "available_slots_global");

   if (!global_slots) { /* need to finish loop */
      DEXIT;
      return 0;
   }

   /* check if job has access to any hosts globally */
   if (sge_why_not_job2host(job, ja_task, 
         host_list_locate(host_list, SGE_GLOBAL_NAME ), complex_list, acl_list)>0) {
      DEXIT;
      return 0;
   }

   /* sequential jobs */
   if (pe_object) {
      /* try to find the highest amount of slots in conformance with
         - max amount of slots we can get served with global resources
         - max amount of slots still available with this parallel environment
         - the users' range specification for the parallel environment */
      global_slots = num_in_range(global_slots, lGetList(job, JB_pe_range));

      if (!global_slots) {
         schedd_mes_add (lGetUlong(job, JB_job_number) , SCHEDD_INFO_NOSLOTSUPPORTBYPE_S, 
                  lGetString(pe_object, PE_name));
         DEXIT;
         return 0;
      }
   } else
      global_slots = 1;

   /* initially looked at global slots */
   if (!*global_resources)
      global_complexes2scheduler(global_resources, host_list_locate(host_list, "global"), complex_list, 0);
 
   for (; global_slots; 
            global_slots = num_in_range(global_slots-1, lGetList(job, JB_pe_range))) {

      /* How many slots can get served with resources limited at the global layer?
       *
       * check for:              
       * - explicit hard resource requests 
       * - consumable default requests 
       * - jobs implicit slot request 
       */
      not_select_resource = 0;

      if (!sge_select_resource(*global_resources, lGetList(job, JB_hard_resource_list),
               0, reason, sizeof(reason)-1, global_slots, ccl, 0)) {

         not_select_resource = 1;
         continue;
      }

      break;
   }
   if (not_select_resource && global_slots == 0) {
      char buff[1024 + 1];

      unparse_resources(NULL, buff, sizeof(buff) - 1, lGetList(job, JB_hard_resource_list));
      if (*buff && (buff[strlen(buff) - 1] == '\n'))
         buff[strlen(buff) - 1] = 0;
      schedd_mes_add (lGetUlong(job, JB_job_number), SCHEDD_INFO_CANNOTRUNGLOBALLY_SS,
              buff, reason);
   }

   DEXIT;
   return global_slots; 
}

/* ----------------------------------------

   sge_get_ulong_qattr() 

   writes actual value of the queue attriute into *uvalp 

   returns:
      0 ok, value in *uvalp is valid
      -1 the queue has no such attribute
      -2 type error: cant compute uval from actual string value 

*/      
int sge_get_ulong_qattr(
u_long32 *uvalp,
char *attrname,
lListElem *q,
lList *exechost_list,
lList *complex_list 
) {
   int ret = -1;
   lListElem *ep;
   lList *attributes = NULL;
   u_long32 type;
   double tmp_dval;
   char dom_str[4];

   DENTER(TOP_LAYER, "sge_get_ulong_qattr");

   /* fill in complexes */
   queue_complexes2scheduler(&attributes, q, exechost_list, complex_list, 0);

   /* find matching */
   if ((ep = find_attribute_in_complex_list(attrname, lFirst(attributes))) &&
         (type=lGetUlong(ep, CE_valtype))!=TYPE_STR) {
         
         if ((lGetUlong(ep, CE_pj_dominant)&DOMINANT_TYPE_MASK)!=DOMINANT_TYPE_VALUE ) {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_pj_stringval), NULL, 0);
            monitor_dominance(dom_str, lGetUlong(ep, CE_pj_dominant));
         } else {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_stringval), NULL, 0); 
            monitor_dominance(dom_str, lGetUlong(ep, CE_dominant));
         }
      ret = 0;
      if (uvalp)
         *uvalp = (u_long32)tmp_dval;
      DPRINTF(("resource %s:"u32"\n", dom_str, (u_long32) tmp_dval));
   }

   /* free */
   lFreeList(attributes);

   DEXIT; 
   return ret;
}


/* ----------------------------------------

   sge_get_double_qattr() 

   writes actual value of the queue attriute into *uvalp 

   returns:
      0 ok, value in *uvalp is valid
      -1 the queue has no such attribute
      -2 type error: cant compute uval from actual string value 

*/      
int sge_get_double_qattr(
double *dvalp,
char *attrname,
lListElem *q,
lList *exechost_list,
lList *complex_list 
) {
   int ret = -1;
   lListElem *ep;
   lList *attributes = NULL;
   u_long32 type;
   double tmp_dval;
   char dom_str[4];

   DENTER(TOP_LAYER, "sge_get_ulong_qattr");

   /* fill in complexes */
   queue_complexes2scheduler(&attributes, q, exechost_list, complex_list, 0);

   /* find matching */
   if ((ep = find_attribute_in_complex_list(attrname, lFirst(attributes))) &&
         (type=lGetUlong(ep, CE_valtype))!=TYPE_STR) {
         
         if ((lGetUlong(ep, CE_pj_dominant)&DOMINANT_TYPE_MASK)!=DOMINANT_TYPE_VALUE ) {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_pj_stringval), NULL, 0);
            monitor_dominance(dom_str, lGetUlong(ep, CE_pj_dominant));
         } else {
            parse_ulong_val(&tmp_dval, NULL, type, lGetString(ep, CE_stringval), NULL, 0); 
            monitor_dominance(dom_str, lGetUlong(ep, CE_dominant));
         }
      ret = 0;
      if (dvalp)
         *dvalp = tmp_dval;
      DPRINTF(("resource %s: %f\n", dom_str, tmp_dval));
   }

   /* free */
   lFreeList(attributes);

   DEXIT; 
   return ret;
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
lList *exechost_list,
lList *complex_list 
) {
   lListElem *ep;
   lList *attributes = NULL;

   DENTER(TOP_LAYER, "sge_get_string_qattr");

   /* fill in complexes */
   queue_complexes2scheduler(&attributes, q, exechost_list, complex_list, 0);

   /* find matching */
   ep = find_attribute_in_complex_list(attrname, lFirst(attributes));

   /* first copy ... */
   if (ep && dst)
      strncpy(dst, lGetString(ep, CE_stringval), dst_len);

   /* ... and then free */
   lFreeList(attributes);

   DEXIT;
   return ep?0:-1;
}


/*
 * Here
 *
 *   - we reduce the amount of free slots in the queue.
 *   - we activte suspend_on_subordinate to prevent
 *     scheduling on queues that will get suspended
 *   - we debit consumable resouces of queue
 *
 * to represent the job again we use the tagged selected queue list
 * (same game as calling sge_create_orders())
 * (would be better to use the granted_destin_identifier_list of the job)
 *
 */
int debit_job_from_queues(
lListElem *job,
lList *granted,
lList *global_queue_list,
lList *complex_list,
u_long32 *total_slotsp,
lList *orders_list   /* needed to warn on jobs that get dispatched and suspended
                        on subordinate in the very same interval */
) {
   int pe_slots = 0;
   int qslots, total;
   unsigned int tagged;
   const char *qname;
   lListElem *gel, *qep, *so;
   int ret = 0;

   DENTER(TOP_LAYER, "debit_job_from_queue");

   /* use each entry in sel_q_list as reference into the global_queue_list */
   for_each(gel, granted ) {

      tagged = lGetUlong(gel, JG_slots);
      if (tagged) {
         /* find queue */
         qname = lGetString(gel, JG_qname);
         qep=lGetElemStr(global_queue_list, QU_qname, qname);

         /* increase used slots */
         qslots = qslots_used(qep);

         /* precompute suspensions for subordinated queues */
         total = lGetUlong(qep, QU_job_slots);
         for_each (so, lGetList(qep, QU_subordinate_list)) {
            /*
               suppose we are not suspended on subordinate
               (therefore used 0 for own_sos parameter of tst_sos())
            */
            if (!tst_sos(qslots,        total, 0, so)  &&  /* not suspended till now */
                 tst_sos(qslots+tagged, total, 0, so)) {   /* but now                */
               ret |= sos_schedd(lGetString(so, SO_qname), global_queue_list);

               /* warn on jobs that were dispatched into that queue in
                  the same scheduling interval based on the orders list */
               {
                  lListElem *order;
                  for_each (order, orders_list) {
                     if (lGetUlong(order, OR_type) != ORT_start_job)
                        continue;
                     if (lGetSubStr(order, OQ_dest_queue, lGetString(so, SO_qname), OR_queuelist)) {
                        WARNING((SGE_EVENT, MSG_SUBORDPOLICYCONFLICT_UUSS, u32c(lGetUlong(job, JB_job_number)),
                        u32c(lGetUlong(order, OR_job_number)), qname, lGetString(so, SO_qname)));
                     }
                  }
               }
            }
         }

         /* count sum of slots (only for returning them) */
         pe_slots += tagged;

         DPRINTF(("REDUCING SLOTS OF QUEUE %s BY %d\n", qname, tagged));

         debit_queue_consumable(job, qep, complex_list, tagged);
      }
   }

   if (total_slotsp)
      *total_slotsp = pe_slots;

   /* 
    * here we could remove queues that became 
    * overloaded by putting the job on the queues 
    * if debiting on host is done before
    */

   DEXIT;
   return ret;
}

int debit_queue_consumable(
lListElem *jep,
lListElem *qep,
lList *complex_list,
int slots 
) {
   return debit_consumable(jep, qep, complex_list, slots,
         QU_consumable_config_list, QU_consumable_actual_list,
         lGetString(qep, QU_qname));
}


/* ---------------------------------------
   returns 
      true  (=1) if queues are requestable
   else
      false (=0) is returned
*/
int queues_are_requestable(
lList *complex_list 
) {
   lListElem *cxep, *ceep;

   if (     !(cxep=lGetElemStr(complex_list, CX_name, SGE_QUEUE_NAME))
        ||  !(ceep=lGetElemStr(lGetList(cxep, CX_entries), CE_name, "qname"))
        ||  !lGetBool(ceep, CE_request)) 
      return 0;  
   else 
      return 1;  
}

