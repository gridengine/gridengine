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


#include "basis_types.h"
#include "sge.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_select_queue.h"
#include "sge_parse_num_par.h"
#include "sge_complex_schedd.h"
#include "valid_queue_user.h"
#include "subordinate_schedd.h"
#include "sge_range_schedd.h"
#include "sge_pe_schedd.h"

#include "sge_orderL.h"
#include "sge_pe.h"
#include "sge_ctL.h"
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
#include "sge_userprj.h"
#include "sge_ckpt.h"
#include "sge_centry.h"
#include "sge_object.h"
#include "sge_resource_utilization.h"
#include "sge_qinstance_state.h"
#include "sge_schedd_conf.h"
#include "sge_subordinate.h"
#include "sge_qref.h"

int scheduled_fast_jobs;
int scheduled_complex_jobs;


/* -- these implement parallel assignemnt ------------------------- */

static int sge_tag_queues_suitable4job_comprehensively(sge_assignment_t *assignment);

static int sge_tag_host_slots(sge_assignment_t *a, lListElem *hep, int *slots, 
   int *slots_qend, int host_soft_violations, bool *master_host, int *host_seqno, 
   double *previous_load, bool *previous_load_inited);

static int sge_sort_suitable_queues(lList *queue_list); 

static int sge_tags2gdil(sge_assignment_t *assignment);


/* -- these implement sequential assignemnt ---------------------- */

static int host_order_2_queue_sequence_number(lList *host_list, lList *queues);
static double sge_max_host_slot_by_theshold(lListElem *hep, lList *queue_list, 
      lList *centry_list, const  lList *load_adjustments);

/* -- base functions ---------------------------------------------- */
 
static int rc_time_by_slots(lList *requested, lList *load_attr, lList *config_attr, lList *actual_attr, 
   lList *centry_list, lListElem *queue, int allow_non_requestable, char *reason, int reason_size, int slots,
    u_long32 layer, double lc_factor, u_long32 tag, u_long32 *start_time, u_long32 duration, const char *object_name);

static int rc_slots_by_time(lList *requests, u_long32 start, u_long32 duration, 
      int *slots, int *slots_qend, lList *total_list, lList *rue_list, lList *load_attr, 
      lList *centry_list, bool force_slots, lListElem *queue, u_long32 layer, double lc_factor, u_long32 tag,
      bool allow_non_requestable, const char *object_name);

static int sge_soft_violations(lListElem *queue, int violation, lListElem *job,lList *load_attr, lList *config_attr,
                               lList *actual_attr, lList *centry_list, u_long32 layer, double lc_factor, u_long32 tag);
 

static int match_static( int slots, lListElem *req_cplx, lListElem *src_cplx, char *reason, size_t reason_size,
      int is_threshold, int force_existence, bool allow_non_requestable);

static int ri_time_by_slots(lListElem *request, lList *load_attr, lList *config_attr, lList *actual_attr, 
      lList *centry_list, lListElem *queue, char *reason, int reason_size, int allow_non_requestable, 
      int slots, u_long32 layer, double lc_factor, u_long32 *start_time, u_long32 duration, 
      const char *object_name); 

static int ri_slots_by_time(u_long32 start, u_long32 duration, int *slots, int *slots_qend, 
   lList *rue_list, lListElem *request, lList *load_attr, lList *total_list, lListElem *queue, 
   lList *centry_list, u_long32 layer, double lc_factor, char *reason, 
   int reason_size, bool allow_non_requestable, bool no_centry, const char *object_name);

static int resource_cmp(u_long32 relop, double req, double src_dl); 

static bool job_is_forced_centry_missing(const lListElem *job,
                             const lList *master_centry_list,
                             const lListElem *queue_or_host);

static int sge_check_load_alarm(char *reason, const char *name, const char *load_value,
                                const char *limit_value, u_long32 relop,
                                u_long32 type, lListElem *hep,
                                lListElem *hlep, double lc_host,
                                double lc_global, const lList *load_adjustments, int load_is_value); 

static void clear_resource_tags( lList *resources, u_long32 max_tag); 


/* -- start of code --------------------------------------------- */

int sge_best_result(int r1, int r2)
{
   if (r1==0 || r2==0)
      return 0;
   if (r1==1 || r2==1)
      return 1;
   if (r1==-2 || r2==-2)
      return -2;
   return -1;
}

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
   
   DENTER(TOP_LAYER, "trace_resources");

   for_each (ep, resources) {
      ret = trace_resource(ep);
      DPRINTF((ret));
   }

   DEXIT;
   return;
}

/****** sge_select_queue/sge_select_queue() ************************************
*  NAME
*     sge_select_queue() -- checks weather a job fits on a given queue or host 
*
*  SYNOPSIS
*     int sge_select_queue(lList *requested_attr, lListElem *queue, lListElem 
*     *host, lList *exechost_list, lList *centry_list, int 
*     allow_non_requestable, char *reason, int reason_size, int slots) 
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
*     int allow_non_requestable - allow non requestable?
*     char *reason              - error message
*     int reason_size           - max error message length
*     int slots                 - number of requested slots
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
int sge_select_queue(lList *requested_attr, lListElem *queue, lListElem *host, lList *exechost_list,
                     lList *centry_list, int allow_non_requestable, char *reason, 
                     int reason_size, int slots) 
{
   int ret;
   lList *load_attr = NULL;
   lList *config_attr = NULL;
   lList *actual_attr = NULL; 
   lListElem *global = NULL;
   double lc_factor = 0; /* scaling for load correction */ 
   u_long32 ulc_factor; 
   /* actually we don't care on start time here to this is just a dummy setting */
   u_long32 start_time = DISPATCH_TIME_NOW; 

   DENTER(TOP_LAYER, "sge_select_queue");

   clear_resource_tags(requested_attr, MAX_TAG);

/* global */
   global = host_list_locate(exechost_list, SGE_GLOBAL_NAME);
   load_attr = lGetList(global, EH_load_list); 
   config_attr = lGetList(global, EH_consumable_config_list);
   actual_attr = lGetList(global, EH_resource_utilization);

   /* is there a multiplier for load correction (may be not in qstat, qmon etc) */
   if (lGetPosViaElem(global, EH_load_correction_factor) >= 0) {
      if ((ulc_factor=lGetUlong(global, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   } 

   ret = rc_time_by_slots(requested_attr, load_attr, config_attr, actual_attr, centry_list,NULL, allow_non_requestable, reason, sizeof(reason)-1, 
            slots, DOMINANT_LAYER_HOST, lc_factor, HOST_TAG, &start_time, 0, SGE_GLOBAL_NAME);

/* host */
   if(!ret){
      if(!host)
         host = host_list_locate(exechost_list, lGetHost(queue, QU_qhostname));
      load_attr = lGetList(host, EH_load_list); 
      config_attr = lGetList(host, EH_consumable_config_list);
      actual_attr = lGetList(host, EH_resource_utilization);

      if (lGetPosViaElem(host, EH_load_correction_factor) >= 0) {
         if ((ulc_factor=lGetUlong(host, EH_load_correction_factor)))
            lc_factor = ((double)ulc_factor)/100;
      }

      ret = rc_time_by_slots(requested_attr, load_attr, config_attr, actual_attr, centry_list,NULL, allow_non_requestable, reason, sizeof(reason)-1, 
               slots, DOMINANT_LAYER_HOST, lc_factor, HOST_TAG, &start_time, 0, lGetHost(host, EH_name));
/* queue */
     if(!ret && queue){
         config_attr = lGetList(queue, QU_consumable_config_list);
         actual_attr = lGetList(queue, QU_resource_utilization);
   
         ret = rc_time_by_slots(requested_attr, NULL, config_attr, actual_attr, centry_list,queue, allow_non_requestable, reason, sizeof(reason)-1, 
               slots, DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG, &start_time, 0, lGetString(queue, QU_full_name));
      }
   }
   DEXIT;
   return !ret;
}

/****** sge_select_queue/rc_time_by_slots() **********************************
*  NAME
*     rc_time_by_slots() -- checks weather all resource requests on one level
*                             are fulfilled 
*
*  SYNOPSIS
*     static int rc_time_by_slots(lList *requested, lList *load_attr, lList 
*     *config_attr, lList *actual_attr, lList *centry_list, lListElem *queue, 
*     int allow_non_requestable, char *reason, int reason_size, int slots, 
*     u_long32 layer, double lc_factor, u_long32 tag) 
*
*  FUNCTION
*     Checks, weather all requests, default requests and implicit requests on this
*     this level are fulfilled 
*
*  INPUTS
*     lList *requested          - list of attribute requests 
*     lList *load_attr          - list of load attributes or null on queue level
*     lList *config_attr        - list of user defined attributes 
*     lList *actual_attr        - usage of all consumables (RUE_Type)
*     lList *centry_list        - system wide attribute config. list (CE_Type)
*     lListElem *queue          - current queue or null on host level
*     int allow_non_requestable - allow none requestabales? 
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
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_QUEUE_END
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTES: is not thread save. uses a static buffer
*
*  Important:
*     we have some special behavior, when slots is set to -1.
*******************************************************************************/
static int rc_time_by_slots(lList *requested, lList *load_attr, lList *config_attr, 
         lList *actual_attr, lList *centry_list, lListElem *queue, int allow_non_requestable, 
         char *reason, int reason_size, int slots, u_long32 layer, double lc_factor, u_long32 tag,
         u_long32 *start_time, u_long32 duration, const char *object_name) 
{
   static lListElem *implicit_slots_request = NULL;
   lListElem *attr;
   u_long32 latest_time = DISPATCH_TIME_NOW;
   u_long32 tmp_start;

   int ret;

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
   if (slots != -1 && queue) {
      tmp_start = *start_time;
      ret = ri_time_by_slots(implicit_slots_request, load_attr, config_attr, actual_attr, centry_list, queue,  
                       reason, reason_size, allow_non_requestable, slots, layer, lc_factor, &tmp_start, duration, object_name);
      if (ret != 0) {
         DEXIT;
         return ret;
      }
      if (*start_time == DISPATCH_TIME_QUEUE_END) {
         DPRINTF(("%s: \"slot\" request delays start time from "U32CFormat
           " to "U32CFormat"\n", object_name, latest_time, MAX(latest_time, tmp_start)));
         latest_time = MAX(latest_time, tmp_start);
      }
   }

   /* ensure all default requests are fulfilled */
   if (slots != -1 && !allow_non_requestable) {
      lListElem *attr;
      int ff;
      const char *name;
      double dval=0.0;
      u_long32 valtype;

      for_each (attr, actual_attr) {
         name = lGetString(attr, RUE_name);
         if (!strcmp(name, "slots"))
            continue;

         /* consumable && used in this global/host/queue && not requested */
         if (!is_requested(requested, name)) {
            lListElem *default_request = lGetElemStr(centry_list, CE_name, name);
            const char *def_req = lGetString(default_request, CE_default);
            valtype = lGetUlong(default_request, CE_valtype);
            parse_ulong_val(&dval, NULL, valtype, def_req, NULL, 0);

            /* ignore default request if the value is 0 */
            if(def_req != NULL && dval != 0.0) {
               char tmp_reason[2048];
               tmp_reason[0] = '\0';

               /* build the default request */
               parse_ulong_val(&dval, NULL, valtype, def_req, NULL, 0);

               lSetString(default_request, CE_stringval, def_req);
               lSetDouble(default_request, CE_doubleval, dval);

               tmp_start = *start_time;
               ff = ri_time_by_slots(default_request, load_attr, config_attr, actual_attr, centry_list, queue,  tmp_reason, 
                  sizeof(tmp_reason)-1, 1, slots, layer, lc_factor, &tmp_start, duration, object_name);

               if (ff != 0) {
                  /* useless to continue in these cases */
                  if (reason) {
                     strncpy(reason, MSG_SCHEDD_FORDEFAULTREQUEST , reason_size-1);
                     strncat(reason, tmp_reason, reason_size-1);
                  }
                  DEXIT;
                  return ff;
               }

               if (*start_time == DISPATCH_TIME_QUEUE_END) {
                  DPRINTF(("%s: default request \"%s\" delays start time from "U32CFormat 
                        " to "U32CFormat"\n", object_name, name, latest_time, MAX(latest_time, tmp_start)));
                  latest_time = MAX(latest_time, tmp_start);
               }
            } 
         } 
      }/* end for*/
   }
 
   if (slots == -1)
      slots = 1;

   /* explicit requests */
   for_each (attr, requested) {
      const char *attr_name = lGetString(attr, CE_name);

      tmp_start = *start_time;
      switch (ri_time_by_slots(attr,load_attr, config_attr, actual_attr, centry_list, queue, 
               reason, reason_size, allow_non_requestable, slots, layer, lc_factor, 
                  &tmp_start, duration, object_name)) {
         case -1: /* will never match */ 
            DEXIT;
            return -1;
         case 0: /* a match was found */
            if (*start_time == DISPATCH_TIME_QUEUE_END) {
               DPRINTF(("%s: explicit request \"%s\" delays start time from "U32CFormat 
                     "to "U32CFormat"\n", object_name, attr_name, latest_time, MAX(latest_time, tmp_start)));
               latest_time = MAX(latest_time, tmp_start);
            }
            if (lGetUlong(attr, CE_tagged) < tag)
               lSetUlong(attr, CE_tagged, tag);
            break;
         case 1: /* will match later-on */
            DPRINTF(("%s: request for %s will match later-on\n", object_name, attr_name));
            DEXIT;
            return 1;
         case 2: /* the requested element does not exist */
            if (tag == QUEUE_TAG && lGetUlong(attr, CE_tagged) == NO_TAG) {
               if (reason){
                  char tmp_reason[2048];
                  tmp_reason[0] = '\0';
                  
                  sprintf(tmp_reason, " (%s)\n", attr_name);
                  strncpy(reason, MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE, reason_size-1);
                  strncat(reason, tmp_reason, reason_size-1);
               }
               DEXIT;
               return -1;
            }
            break;
         default: /* error */
            break;
      }
   }

   if (*start_time == DISPATCH_TIME_QUEUE_END) {
      *start_time = latest_time;
   }

   DEXIT;
   return 0;
}

/* return 0 on success -1 otherwise */
static int match_static(
int slots,
lListElem *req_cplx,
lListElem *src_cplx, 
char *reason,
size_t reason_size,
int is_threshold,
int force_existence,
bool allow_non_requestable)
{
   int match;
   int ret = 0;
   char availability_text[2048];

   DENTER(TOP_LAYER, "match_static");

   /* check whether attrib is requestable */
   if (!allow_non_requestable && 
      lGetUlong(src_cplx, CE_requestable) == REQU_NO) {
      if (reason) {
         strncpy(reason, MSG_SCHEDD_JOBREQUESTSNONREQUESTABLERESOURCE , reason_size);
         strncat(reason, lGetString(src_cplx, CE_name), reason_size);
         strncat(reason, "\"", reason_size);
      }
      DEXIT;
      return -1;
   }

   match = compare_complexes(slots, req_cplx, src_cplx, availability_text, false, false);

   if (!match) {
      if (reason) {
         strncpy(reason, MSG_SCHEDD_ITOFFERSONLY , reason_size);
       strncat(reason, availability_text, reason_size);
      }
      ret = -1;
   }

   DEXIT;
   return ret;
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


/****** sge_select_queue/queue_match_static() ************************
*  NAME
*     queue_match_static() -- Do matching that depends not on time.
*
*  SYNOPSIS
*     static int queue_match_static(lListElem *queue, lListElem *job, 
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
*     int - 0 ok
*          -1 assignment will never be possible for all jobs of that category
*
*  NOTES
*******************************************************************************/
int queue_match_static(
lListElem *queue, 
lListElem *job,
const lListElem *pe, 
const lListElem *ckpt,
lList *centry_list, 
lList *acl_list) 
{
   u_long32 job_id;
   const char *queue_name;
   char reason[1024 + 1];
   char buff[1024 + 1];
   lList *projects;
   const char *project;

   DENTER(TOP_LAYER, "queue_match_static");

   reason[0] = buff[0] = '\0';

   job_id = lGetUlong(job, JB_job_number);
   queue_name = lGetString(queue, QU_full_name);
   /* check if job owner has access rights to the queue */
   if (!sge_has_access(lGetString(job, JB_owner), lGetString(job, JB_group), queue, acl_list)) {
      DPRINTF(("Job %d has no permission for queue %s\n", (int)job_id, queue_name));
      schedd_mes_add(job_id, SCHEDD_INFO_HASNOPERMISSION_SS, "queue", queue_name);
      DEXIT;
      return -1;
   }

   /* check if job can run in queue based on project */
   if ((projects = lGetList(queue, QU_projects))) {
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "queue", queue_name);
         DEXIT;
         return -1;
      }
      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "queue", queue_name);
         DEXIT;
         return -1;
      }
   }

   /* check if job can run in queue based on excluded projects */
   if ((projects = lGetList(queue, QU_xprojects))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "queue", queue_name);
         DEXIT;
         return -1;
      }
   }

   if (lGetList(job, JB_hard_queue_list) ||
       lGetList(job, JB_master_hard_queue_list)) {
      if (!centry_list_are_queues_requestable(centry_list)) {
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTREQUESTABLE_S,
            queue_name);
         DEXIT;
         return -1;
      }
   }

   /* 
    * is queue contained in hard queue list ? 
    */
   if (lGetList(job, JB_hard_queue_list)) {
      lList *master_cqueue_list = NULL;
      lList *master_hgroup_list = NULL;
      lList *qref_list = lGetList(job, JB_hard_queue_list);
      lList *resolved_qref_list = NULL;
      lListElem *resolved_qref = NULL;
      const char *qinstance_name = NULL;
      bool found_something = false;
      bool is_in_list = true;

      master_cqueue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
      master_hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));
      qinstance_name = lGetString(queue, QU_full_name);
      qref_list_resolve(qref_list, NULL, &resolved_qref_list,
                        &found_something, master_cqueue_list,
                        master_hgroup_list, true, true);
      resolved_qref = lGetElemStr(resolved_qref_list, QR_name, qinstance_name); 
      is_in_list = (resolved_qref != NULL);
      resolved_qref_list = lFreeList(resolved_qref_list);
      if (!is_in_list) {
         DPRINTF(("Queue \"%s\" is not contained in the hard "
                  "queue list (-q) that was requested by job %d\n",
                  qinstance_name, (int) job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINHARDQUEUELST_S, 
                        qinstance_name);
         DEXIT; 
         return -1;
      }
   }

   /* 
    * is this queue a candidate for being the master queue? 
    */
   if (lGetList(job, JB_master_hard_queue_list)) {
      lList *master_cqueue_list = NULL;
      lList *master_hgroup_list = NULL;
      lList *qref_list = lGetList(job, JB_master_hard_queue_list);
      lList *resolved_qref_list = NULL;
      lListElem *resolved_qref = NULL;
      const char *qinstance_name = NULL;
      bool found_something = false;
      bool is_in_list = true;

      master_cqueue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
      master_hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));
      qinstance_name = lGetString(queue, QU_full_name);
      qref_list_resolve(qref_list, NULL, &resolved_qref_list,
                        &found_something, master_cqueue_list,
                        master_hgroup_list, true, true);
      resolved_qref = lGetElemStr(resolved_qref_list, QR_name, qinstance_name);
      is_in_list = (resolved_qref != NULL);
      resolved_qref_list = lFreeList(resolved_qref_list);
   
      /*
       * Tag queue
       */
      lSetUlong(queue, QU_tagged4schedule, is_in_list ? 1 : 0);
      if (!is_in_list) {
         DPRINTF(("Queue \"%s\" is contained in the master hard "
                  "queue list (-masterq) that was requested by job %d\n",
                  queue_name, (int) job_id));
      }
   }

   /*
   ** different checks for different job types:
   */

   if (pe) { /* parallel job */
      if (!qinstance_is_parallel_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a parallel queue as requested by " 
                  "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELQUEUE_S, queue_name);
         DEXIT;
         return -1;
      }

      /*
       * check if the requested PE is named in the PE reference list of Queue
       */
      if (!qinstance_is_pe_referenced(queue, pe)) {
         DPRINTF(("Queue "SFQ" does not reference PE "SFQ"\n",
                  queue_name, lGetString(pe, PE_name)));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFPE_SS,
                        queue_name, lGetString(pe, PE_name));
         DEXIT;
         return -1;
      }
   }

   if (ckpt) { /* ckpt job */
      /* is it a ckpt queue ? */
      if (!qinstance_is_checkointing_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a checkpointing queue as requested by "
                  "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTACKPTQUEUE_SS, queue_name);
         DEXIT;
         return -1;
      }

      /*
       * check if the requested CKPT is named in the CKPT ref list of Queue
       */
      if (!qinstance_is_ckpt_referenced(queue, ckpt)) {
         DPRINTF(("Queue \"%s\" does not reference checkpointing object "SFQ
                  "\n", queue_name, lGetString(ckpt, CK_name)));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTINQUEUELSTOFCKPT_SS,  
                        queue_name, lGetString(ckpt, CK_name));
         DEXIT;
         return -1;
      }
   }   

   /* to be activated as soon as immediate jobs are available */
   if (JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* immediate job */
      /* 
       * is it an interactve job and an interactive queue? 
       * or
       * is it a batch job and a batch or transfer queue?
       */
      if (!lGetString(job, JB_script_file) && 
          !qinstance_is_interactive_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not an interactive queue as requested by "
                  "job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_QUEUENOTINTERACTIVE_S, queue_name);
         DEXIT;
         return -1;
      } else if (lGetString(job, JB_script_file) && 
                 !qinstance_is_batch_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a serial batch queue as "
                  "requested by job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTASERIALQUEUE_S, queue_name);
         DEXIT;
         return -1;
      }
   }

   if (!pe && !ckpt && !JOB_TYPE_IS_IMMEDIATE(lGetUlong(job, JB_type))) { /* serial (batch) job */
      /* is it a batch or transfer queue */
      if (!qinstance_is_batch_queue(queue)) {
         DPRINTF(("Queue \"%s\" is not a serial batch queue as "
                  "requested by job %d\n", queue_name, (int)job_id));
         schedd_mes_add(job_id, SCHEDD_INFO_NOTASERIALQUEUE_S, queue_name);
         DEXIT;
         return -1;
      }
   }

   if (ckpt && !pe && lGetString(job, JB_script_file) &&
       qinstance_is_parallel_queue(queue) && !qinstance_is_batch_queue(queue)) {
      DPRINTF(("Queue \"%s\" is not a serial batch queue as "
               "requested by job %d\n", queue_name, (int)job_id));
      schedd_mes_add(job_id, SCHEDD_INFO_NOTPARALLELJOB_S, queue_name);
      DEXIT;
      return -1;
   }

   if (job_is_forced_centry_missing(job, centry_list, queue)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
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
         bool is_forced = (lGetUlong(centry, CE_requestable) == REQU_FORCED);
         const char *object_name = NULL;

         if (is_forced) {
            if (object_has_type(queue_or_host, QU_Type)) {
               is_forced = qinstance_is_centry_a_complex_value(queue_or_host, centry);
               object_name = lGetString(queue_or_host, QU_full_name);
            } else if (object_has_type(queue_or_host, EH_Type)) {
               is_forced = host_is_centry_a_complex_value(queue_or_host, centry);
               object_name = lGetHost(queue_or_host, EH_name);
            } else {
               DTRACE;
               is_forced = false;
            }
         }

         if (is_forced && !is_requ) {
            u_long32 job_id = lGetUlong(job, JB_job_number);

            DPRINTF(("job "u32" does not request 'forced' resource "SFQ" of "
                     SFN"\n", job_id, name, object_name));
            schedd_mes_add(job_id, SCHEDD_INFO_NOTREQFORCEDRES_SS, name, 
                           object_name);
            ret = true;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

/****** sge_select_queue/sge_soft_violations() ***************************************
*  NAME
*     sge_soft_violations() -- counts the violations in the request for a given host or queue 
*
*  SYNOPSIS
*     static int sge_soft_violations(lListElem *queue, int violation, lListElem *job,lList *load_attr, lList *config_attr,
*                               lList *actual_attr, lList *centry_list, u_long32 layer, double lc_factor, u_long32 tag)
*
*  FUNCTION
*     this function checks if the current resources can satisfy the requests. The resources come from the global host, a
*     given host or the queue. The function returns the number of violations. 
*
*  INPUTS
*     lListElem *queue     - should only be set, when one using this method on queue level 
*     int violation        - the number of previous violations. This is needed to get a correct result on queue level. 
*     lListElem *job       - the job which is tested 
*     lList *load_attr     - the load attributs, only when used on hosts or global 
*     lList *config_attr   - a list of custom attributes  (CE_Type)
*     lList *actual_attr   - a list of custom consumables, they contain the current usage of these attributes (RUE_Type)
*     lList *centry_list   - the system wide complex list 
*     u_long32 layer       - the curent layer flag 
*     double lc_factor     - should be set, when load correction has to be done. 
*     u_long32 tag         - the current layer tag. (GLOGAL_TAG, HOST_TAG, QUEUE_TAG) 
*
*  RESULT
*     static int - the number of violations ( = (prev. violations) + (new violations in this run)). 
*
*******************************************************************************/
static int sge_soft_violations(lListElem *queue, int violation, lListElem *job,lList *load_attr, lList *config_attr,
                               lList *actual_attr, lList *centry_list, u_long32 layer, double lc_factor, u_long32 tag) 
{
   u_long32 job_id;
   const char *queue_name = NULL;
   const int reason_size = 1024;
   char reason[1024 + 1];
   unsigned int soft_violation = violation;
   lList *soft_requests = NULL; 
   lListElem *attr;
   u_long32 start_time = DISPATCH_TIME_NOW; 

   DENTER(TOP_LAYER, "sge_soft_violations");

   reason[0] = '\0';

   soft_requests = lGetList(job, JB_soft_resource_list);
   clear_resource_tags(soft_requests, QUEUE_TAG);

   job_id = lGetUlong(job, JB_job_number);
   if (queue)
      queue_name = lGetString(queue, QU_full_name);

   /* count number of soft violations for _one_ slot of this job */

   for_each (attr, soft_requests) {
      /* TODO: AH: How about start time and duration of the soft request stuff ?!? */
      switch (ri_time_by_slots(attr, load_attr, config_attr, actual_attr, centry_list, queue,
                      reason, reason_size, false, 1, layer, lc_factor, &start_time, 0, queue_name)){
            /* no match */
            case -1 :   soft_violation++;
               break;
            /* element not found */
            case 1 : 
            case 2 : 
            if(tag == QUEUE_TAG && lGetUlong(attr, CE_tagged) == NO_TAG)
                           soft_violation++;
               break;
            /* everything is fine */
            default : if( lGetUlong(attr, CE_tagged) < tag)
                           lSetUlong(attr, CE_tagged, tag);
      }

   }

   if (queue) {
      DPRINTF(("queue %s does not fulfill soft %d requests (first: %s)\n", 
         queue_name, soft_violation, reason));

      /* 
       * check whether queue fulfills soft queue request of the job (-q) 
       */
      if (lGetList(job, JB_soft_queue_list)) {
         lList *master_cqueue_list = NULL;
         lList *master_hgroup_list = NULL;
         lList *qref_list = lGetList(job, JB_soft_queue_list);
         lList *resolved_qref_list = NULL;
         lListElem *resolved_qref = NULL;
         const char *qinstance_name = NULL;
         bool found_something = false;
         bool is_in_list = true;

         master_cqueue_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
         master_hgroup_list = *(object_type_get_master_list(SGE_TYPE_HGROUP));
         qinstance_name = lGetString(queue, QU_full_name);
         qref_list_resolve(qref_list, NULL, &resolved_qref_list,
                           &found_something, master_cqueue_list,
                           master_hgroup_list, true, true);
         resolved_qref = lGetElemStr(resolved_qref_list, QR_name, 
                                     qinstance_name); 
         is_in_list = (resolved_qref != NULL);
         resolved_qref_list = lFreeList(resolved_qref_list);
         if (!is_in_list) {
            DPRINTF(("Queue \"%s\" is not contained in the soft "
                     "queue list (-q) that was requested by job %d\n",
                     qinstance_name, (int) job_id));

            soft_violation++;
         }
      }

      /* store number of soft violations in queue */
      lSetUlong(queue, QU_soft_violation, soft_violation);
   }  

   DEXIT;
   return soft_violation;
}

/****** sge_select_queue/host_match_static() ********************************
*  NAME
*     host_match_static() -- Static test whether job fits to host
*
*  SYNOPSIS
*     static int host_match_static(lListElem *job, lListElem *ja_task, 
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
int host_match_static(lListElem *job, lListElem *ja_task, 
      lListElem *host, lList *centry_list, lList *acl_list) 
{
   lList *projects;
   const char *project;
   u_long32 job_id;
   const char *eh_name;

   DENTER(TOP_LAYER, "host_match_static");

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
      return -1;
   }

   /* check if job can run on host based on required projects */
   if ((projects = lGetList(host, EH_prj))) {
   
      if ((!(project = lGetString(job, JB_project)))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASNOPRJ_S,
            "host", eh_name);
         DEXIT;
         return -1;
      }

      if ((!userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_HASINCORRECTPRJ_SSS,
            project, "host", eh_name);
         DEXIT;
         return -1;
      }
   }

   /* check if job can run on host based on excluded projects */
   if ((projects = lGetList(host, EH_xprj))) {
      if (((project = lGetString(job, JB_project)) &&
           userprj_list_locate(projects, project))) {
         schedd_mes_add(job_id, SCHEDD_INFO_EXCLPRJ_SSS,
            project, "host", eh_name);
         DEXIT;
         return -1;
      }
   }

   if (job_is_forced_centry_missing(job, centry_list, host)) {
      DEXIT;
      return -1;
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
            return -2;
         }
      }
   } 

   DEXIT;
   return 0;
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
      return 1;
   }

   return 0;
}

static int sge_check_load_alarm(char *reason, const char *name, const char *load_value, 
                                const char *limit_value, u_long32 relop, 
                                u_long32 type, lListElem *hep, 
                                lListElem *hlep, double lc_host, 
                                double lc_global, const lList *load_adjustments, int load_is_value) 
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
      case TYPE_RESTR:
         match = string_base_cmp(type, limit_value, load_value);
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
   centry_list and exechost_list get used
   therefore

   returns boolean:
      1 yes, the threshold is exceeded
      0 no
*/

int 
sge_load_alarm(char *reason, lListElem *qep, lList *threshold, 
               const lList *exechost_list, const lList *centry_list, 
               const lList *load_adjustments) 
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
      lListElem *hlep = NULL, *glep = NULL, *queue_ep = NULL, *cep  = NULL;
      const char *name;
      u_long32 relop, type;

      name = lGetString(tep, CE_name);
      /* complex attriute definition */
      if (!(cep = centry_list_locate(centry_list, name))) {
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
      }
      if ((hlep == NULL) && (global_hep != NULL)) {
         glep = lGetSubStr(global_hep, HL_name, name, EH_load_list);
         if (glep != NULL) {
            load_value = lGetString(glep, HL_value);
            load_is_value = 0;
         }
      } 
      if (glep == NULL && hlep == NULL) {
         queue_ep = lGetSubStr(qep, CE_name, name, QU_consumable_config_list);
         if (queue_ep != NULL) {
            load_value = lGetString(queue_ep, CE_stringval);
            load_is_value = 1;
         } else { 
            if (reason) {
               sprintf(reason, MSG_SCHEDD_NOVALUEFORATTR_S, name);
            }
            DEXIT;
            return 1;
         }
      }

      limit_value = lGetString(tep, CE_stringval);
      type = lGetUlong(cep, CE_valtype);
      if(sge_check_load_alarm(reason, name, load_value, limit_value, relop, 
                              type, hep, hlep, lc_host, lc_global, 
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

         /* find actual complex attribute */
         if ((cep = lGetElemStr(rlp, CE_name, name)) == NULL) {
            /* no complex attribute for threshold -> ERROR */
            sprintf(buffer, MSG_SCHEDD_NOCOMPLEXATTRIBUTEFORTHRESHOLD_S, name);
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

         sprintf(buffer, "\talarm %s:%s=%s %s-threshold=%s\n",
                 dom_str,
                 name, 
                 load_value,
                 threshold_type,
                 limit_value
                );

         strncat(reason, buffer, reason_size);
      }

      lFreeList(rlp);
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
lList *centry_list, /* CE_Type */
const lList *load_adjustments, /* CE_Type */
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
      if (!granted || (granted && (sconf_get_global_load_correction() ||
                           lGetElemHost(granted, JG_qhostname, lGetHost(qep, QU_qhostname))))) {
         nverified++;

         if (sge_load_alarm(reason, qep, thresholds, exechost_list, centry_list, load_adjustments)!=0) {
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
*  RESULT
*     int - 0 success 
*          -1 error
*******************************************************************************/
int sge_split_queue_slots_free(lList **free, lList **full) 
{
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
      if ((qinstance_slots_used(this)+1) > (int) lGetUlong(this, QU_job_slots)) {
         /* chain 'this' into 'full' list */
         this = lDechainElem(*free, this);
         if (full) {
            if (!*full)
               *full = lCreateList("full one", lGetListDescr(*free));
            qinstance_state_set_full(this, true);
            lAppendElem(*full, this);
         } else
            lFreeElem(this);
      }
   }

   if (*full) {
      lListElem* mes_queue;
      bool full_queues = false;

      for_each(mes_queue, *full) {
         if (qinstance_state_is_full(mes_queue)) {
            schedd_mes_add_global(SCHEDD_INFO_QUEUEFULL_, lGetString(mes_queue, QU_full_name));
            full_queues = true;
         }
      }

      if (full_queues)  
         schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESFULLANDDROPPED , *full, QU_full_name);

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
         QU_state, QI_SUSPENDED,
         QU_state, QI_CAL_SUSPENDED,
         QU_state, QI_SUSPENDED_ON_SUBORDINATE);
   ret = lSplit(queue_list, suspended, "full queues", where);
   lFreeWhere(where);

   if (*suspended) {
      lListElem* mes_queue;

      for_each(mes_queue, *suspended)
         schedd_mes_add_global(SCHEDD_INFO_QUEUESUSP_, lGetString(mes_queue, QU_full_name));
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESSUSPENDEDANDDROPPED , *suspended, QU_full_name);
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
   where = lWhere("%T(!(%I m= %u) && !(%I m= %u))", lGetListDescr(*queue_list), 
                  QU_state, QI_DISABLED, QU_state, QI_CAL_DISABLED);
   ret = lSplit(queue_list, disabled, "full queues", where);
   lFreeWhere(where);

   if (*disabled) {
      lListElem* mes_queue;

      for_each(mes_queue, *disabled)
         schedd_mes_add_global(SCHEDD_INFO_QUEUEDISABLED_, lGetString(mes_queue, QU_full_name));
 
      schedd_log_list(MSG_SCHEDD_LOGLIST_QUEUESDISABLEDANDDROPPED , *disabled, QU_full_name);
      if (do_free_list) {
         lFreeList(*disabled);
         *disabled = NULL;
      }
   }

   DEXIT;
   return ret;
}



/****** sge_select_queue/sge_tag_queues_suitable4job_fast_track() **************
*  NAME
*     sge_tag_queues_suitable4job_fast_track() -- ??? 
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
*     sge_assignment_t *assignment - ??? 
*     lList **ignore_hosts         - ??? 
*     lList **ignore_queues        - ??? 
*
*  RESULT
*     int - 0 ok got an assignment 
*             start time(s) and slots are tagged
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_tag_queues_suitable4job_fast_track() is not MT safe 
*******************************************************************************/
static int sge_tag_queues_suitable4job_fast_track(sge_assignment_t *a,
      lList **ignore_hosts, lList **ignore_queues)
{
   lListElem *category = lGetRef(a->job, JB_category);
   bool now_assignment = (a->start == DISPATCH_TIME_NOW);
   bool use_category = now_assignment && (category != NULL) && lGetUlong(category, CT_refcount) > MIN_JOBS_IN_CATEGORY;
   int result;
   u_long32 job_id = lGetUlong(a->job, JB_job_number);
   u_long32 tt_global = a->start;
   int best_queue_result = -1;
   bool fast_track_exit = false;
   int global_violations = 0, host_violations, queue_violations;
   lListElem *hep;

   DENTER(TOP_LAYER, "sge_tag_queues_suitable4job_fast_track");

   if (use_category) {
      schedd_mes_set_tmp_list(category, CT_job_messages, job_id);
   }

/*       qinstance_list_set_tag(queues, 0); */

   result = global_time_by_slots(1, &tt_global, a->duration, 
         &global_violations, a->job, a->gep, a->centry_list, a->acl_list);
   if (result!=0) {
      DEXIT;
      return result;
   }

   for_each(hep, a->host_list) {
      u_long32 tt_host = a->start;
      const char *eh_name = lGetHost(hep, EH_name);
      const char *qname;
      lListElem *qep;
      const void *queue_iterator = NULL;
      lListElem *next_queue;

      host_violations = global_violations;
      if (a->start == DISPATCH_TIME_QUEUE_END)
         tt_host = a->start;

      if (!strcasecmp(eh_name, "global") || !strcasecmp(eh_name, "template"))
         continue;

      if (use_category){
         lList *skip_host_list = lGetList(category, CT_ignore_hosts);
         if (skip_host_list && lGetElemStr(skip_host_list, CTI_name, eh_name)){
            continue;
         }
      }

      result = host_time_by_slots(1, &tt_host, a->duration, &host_violations, 
            a->job, a->ja_task, hep, a->centry_list, a->acl_list);
      if (result != 0) {
         /* right now there is no use in continuing with that host but we 
            don't wanna loose an opportunity for a reservation */
         best_queue_result = sge_best_result(result, best_queue_result); 
         continue;
      }

      next_queue = lGetElemHostFirst(a->queue_list, QU_qhostname, eh_name, &queue_iterator);
      while ((qep = next_queue)) {
         u_long32 tt_queue = a->start;

         next_queue = lGetElemHostNext(a->queue_list, QU_qhostname, eh_name, &queue_iterator);
         qname = lGetString(qep, QU_full_name);

         if (use_category){
            lList *skip_queue_list = lGetList(category, CT_ignore_queues);
            if (skip_queue_list && lGetElemStr(skip_queue_list, CTI_name, qname)){
               continue;
            }
         }

         queue_violations = host_violations;
         result = queue_time_by_slots(1, &tt_queue, a->duration, 
                  &queue_violations, a->job, qep, NULL, a->ckpt, 
                  a->centry_list, a->acl_list);

         if (result==0) {
            /* tag number of slots per queue and time when it will be available */
            lSetUlong(qep, QU_tag, 1);
            if (a->start == DISPATCH_TIME_QUEUE_END) {
               tt_queue = MAX(tt_queue, MAX(tt_host, tt_global));
               lSetUlong(qep, QU_available_at, tt_queue);
            }
            DPRINTF(("    set Q: %s "u32" "u32"\n", lGetString(qep, QU_full_name),
                      lGetUlong(qep, QU_tag), lGetUlong(qep, QU_available_at)));
            best_queue_result = 0;

            if (now_assignment) {
               fast_track_exit = true;
               break;
            }
         } else
            best_queue_result = sge_best_result(result, best_queue_result); 
      }

      if (fast_track_exit == true)
         break;

      if (use_category && ignore_hosts) {
         lAddElemStr(ignore_hosts, CTI_name, eh_name, CTI_Type);
      }
   }

   DEXIT;
   return best_queue_result;
}




/****** sge_select_queue/sge_tag_queues_suitable4job_comprehensively() *********
*  NAME
*     sge_tag_queues_suitable4job_comprehensively() -- Tag queues/hosts for 
*        a comprehensive/parallel assignment
*
*  SYNOPSIS
*     static int sge_tag_queues_suitable4job_comprehensively(sge_assignment_t 
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
*
*  RESULT
*     static int - 0 ok got an assignment
*                  1 no assignment at the specified time
*                 -1 assignment will never be possible for all jobs of that category
*                 -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_tag_queues_suitable4job_comprehensively() is not MT safe 
*******************************************************************************/
static int sge_tag_queues_suitable4job_comprehensively(sge_assignment_t *a) 
{
   lListElem *job = a->job;
   bool now_assignment = (a->start == DISPATCH_TIME_NOW);

   lListElem *category = lGetRef(job, JB_category);
   bool use_category = now_assignment && category && lGetUlong(category, CT_refcount) > MIN_JOBS_IN_CATEGORY;
   bool use_cviolation = use_category && lGetNumberOfElem(lGetList(category, CT_queue_violations)) > 0; 
   bool need_master_host = (lGetList(job, JB_master_hard_queue_list)!=NULL);

   int global_soft_violations = 0;
   int max_slots_all_hosts, accu_host_slots, accu_host_slots_qend;
   bool have_master_host, suited_as_master_host;
   lListElem *hep, *qep;
   int best_result = -1, result, gslots, gslots_qend;
   int host_seqno = 0;
   double previous_load;
   bool previous_load_inited = false;

   DENTER(TOP_LAYER, "sge_tag_queues_suitable4job_comprehensively");

   qinstance_list_set_tag(a->queue_list, 0);

   if (use_category){
      schedd_mes_set_tmp_list(category, CT_job_messages, lGetUlong(job, JB_job_number));
   }

   /* remove reasons from last unsuccesful iteration */ 
   clean_monitor_alp();

   result = global_slots_by_time(a->start, a->duration, &gslots, &gslots_qend,
         use_cviolation? NULL: &global_soft_violations, job, a->gep, a->centry_list, 
            a->acl_list);
   if (gslots < a->slots) {
      best_result = (gslots_qend < a->slots)?-1:1;

      if (best_result == 1) {
         DPRINTF(("GLOBAL will <category_later> get us %d slots (%d)\n", 
            gslots, gslots_qend));
      } else {
         DPRINTF(("GLOBAL will <category_never> get us %d slots (%d)\n", 
            gslots, gslots_qend));
      }   
      DEXIT;
      return best_result;
   }

   accu_host_slots = accu_host_slots_qend = 0;
   have_master_host = false;
   max_slots_all_hosts = 0;

   /* first select hosts with lowest share/load 
      and then select queues with */
   /* tag amount of slots we can get served with resources limited per host */
   for_each (hep, a->host_list) {

      int hslots = 0, hslots_qend = 0;
      const char *eh_name = lGetHost(hep, EH_name);

      if (!strcasecmp(eh_name, "global") || !strcasecmp(eh_name, "template"))
         continue;

      /* do not perform expensive checks for this host if there 
       * is not at least one free queue residing at this host:
       * see if there are queues which are not disbaled/suspended/calender;
       * which have at least one free slot, which are not unknown, several alarms
       */  

      if (!(qep=lGetElemHost(a->queue_list, QU_qhostname, eh_name)))
         continue;   

      result = sge_tag_host_slots(a, hep, &hslots, &hslots_qend, global_soft_violations, 
          &suited_as_master_host, &host_seqno, &previous_load, &previous_load_inited);
#if 0
      if (result != 0)
         best_result = sge_best_result(result, best_result);
#endif
      accu_host_slots      += hslots;
      accu_host_slots_qend += hslots_qend;
      DPRINTF(("HOST(3) %s could get us %d slots (%d later on)\n", 
            eh_name, hslots, hslots_qend));

      /* tag full amount or zero */
      lSetUlong(hep, EH_tagged, hslots); 
      lSetUlong(hep, EH_master_host, suited_as_master_host?1:0); 
      if (suited_as_master_host)
         have_master_host = true;
   } /* for each host */

   if (accu_host_slots >= a->slots && 
      (!need_master_host || (need_master_host && have_master_host))) {
      /* stop looking for smaller slot amounts */
      DPRINTF(("-------------->      BINGO %d slots %s at specified time <--------------\n", 
            a->slots, need_master_host?"plus master host":""));
      best_result = 0;
   } else if (accu_host_slots_qend >= a->slots && (!need_master_host || 
               (need_master_host && have_master_host))) {
      DPRINTF(("-------------->            %d slots %s later             <--------------\n", 
            a->slots, need_master_host?"plus master host":""));
      best_result = 1;
   }

   if (use_category) {  
      lList *temp =  schedd_mes_get_tmp_list();
      if (temp){    
         lSetList(category, CT_job_messages, lCopyList(NULL, temp));
       }
   }

   switch (best_result) {
   case 0:
      DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns "u32"\n", 
            a->slots, a->start));
      break;
   case 1:
      DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <later>\n", 
            a->slots));
      break;
   case -1:
      DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <category_never>\n", 
            a->slots));
      break;
   case -2:
      DPRINTF(("COMPREHSENSIVE ASSIGNMENT(%d) returns <job_never>\n", 
            a->slots));
      break;
   default:
      DPRINTF(("!!!!!!!! COMPREHSENSIVE ASSIGNMENT(%d) returns unexpected %d\n", 
            best_result));
      break;
   }

   DEXIT;
   return best_result;
}


/****** sge_select_queue/sge_tag_host_slots() **********************************
*  NAME
*     sge_tag_host_slots() -- Determine host slots and tag queue(s) accordingly
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
*
*  RESULT
*     static int - 0 ok got an assignment
*                  1 no assignment at the specified time
*                 -1 assignment will never be possible for all jobs of that category
*                 -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_tag_host_slots() is not MT safe 
*******************************************************************************/
static int sge_tag_host_slots(
sge_assignment_t *a,
lListElem *hep, 
int *slots,
int *slots_qend,
int global_soft_violations, 
bool *master_host, 
int *host_seqno, 
double *previous_load,
bool *previous_load_inited)
{
   bool suited_as_master_host = false;
   int min_host_slots, max_host_slots;
   int accu_queue_slots, accu_queue_slots_qend;
   int qslots, qslots_qend, hslots, hslots_qend, best_result = -1;
   int host_soft_violations, queue_soft_violations;
   const char *qname, *eh_name = lGetHost(hep, EH_name);
   lListElem *qep, *next_queue; 
   int result;
   const void *queue_iterator = NULL;
   int allocation_rule; 
   
   DENTER(TOP_LAYER, "sge_tag_host_slots");

   allocation_rule = sge_pe_slots_per_host(a->pe, a->slots);

   if (ALLOC_RULE_IS_BALANCED(allocation_rule))
      min_host_slots = max_host_slots = allocation_rule;
   else {
      min_host_slots = 1;
      max_host_slots = a->slots;
   }

   host_soft_violations = global_soft_violations;

   result = host_slots_by_time(a->start, a->duration, &hslots, &hslots_qend, 
      &host_soft_violations, a->job, a->ja_task, hep, a->queue_list, a->centry_list, 
      a->acl_list, a->load_adjustments, false); 
#if 0
   if (result != 0) {
      DEXIT;
      return result;
   }
#endif

   DPRINTF(("HOST %s itself (and queue threshold) will get us %d slots (%d later) ... "
         "we need %d\n", eh_name, hslots, hslots_qend, min_host_slots));

   hslots      = MIN(hslots,      max_host_slots);
   hslots_qend = MIN(hslots_qend, max_host_slots);

   if (hslots >= min_host_slots || hslots_qend >= min_host_slots) {

      accu_queue_slots = accu_queue_slots_qend = 0;

      for (next_queue = lGetElemHostFirst(a->queue_list, QU_qhostname, eh_name, &queue_iterator); 
          (qep = next_queue);
           next_queue = lGetElemHostNext(a->queue_list, QU_qhostname, eh_name, &queue_iterator)) {

         qname = lGetString(qep, QU_full_name);
         queue_soft_violations = host_soft_violations;

         result = queue_slots_by_time(a->start, a->duration, &qslots, &qslots_qend, 
               &queue_soft_violations, a->job, qep, a->pe, a->ckpt, 
               a->centry_list, a->acl_list, false);
         best_result = sge_best_result(result, best_result);

         if (result == 0 && (MAX(qslots, qslots_qend)>=min_host_slots)) {

            /* in case the load of two hosts is equal this
               must be also reflected by the sequence number */
            if (*previous_load_inited && (*previous_load < lGetDouble(hep, EH_sort_value)))
               (*host_seqno)++;
            else {
               if (!previous_load_inited) {
                  *previous_load_inited = true;
               } else
                  /* DPRINTF(("SKIP INCREMENTATION OF HOST_SEQNO\n")) */ ;
            }
            *previous_load = lGetDouble(hep, EH_sort_value);
            lSetUlong(qep, QU_host_seq_no, *host_seqno);

            /* could this host be a master host */
            if (!suited_as_master_host && lGetUlong(qep, QU_tagged4schedule)) {
               DPRINTF(("HOST %s can be master host because of queue %s\n", eh_name, qname));    
               suited_as_master_host = true; 
            }

            /* prepare sort by sequence number of queues */
            lSetUlong(qep, QU_host_seq_no, *host_seqno);

            DPRINTF(("QUEUE %s TIME: %d + %d -> %d  QEND: %d + %d -> %d\n", qname, 
               accu_queue_slots,      qslots,      accu_queue_slots+       qslots, 
               accu_queue_slots_qend, qslots_qend, accu_queue_slots_qend + qslots_qend)); 
            accu_queue_slots      += qslots;
            accu_queue_slots_qend += qslots_qend;
            lSetUlong(qep, QU_tag,      qslots);
         } else {
            DPRINTF(("HOST(1.5) %s will get us nothing\n", eh_name)); 
         }

      } /* for each queue of the host */

      hslots      = MIN(accu_queue_slots,      hslots);
      hslots_qend = MIN(accu_queue_slots_qend, hslots_qend);

      DPRINTF(("HOST %s and it's queues will get us %d slots (%d later) ... we need %d\n", 
            eh_name, hslots, hslots_qend, min_host_slots));
   }

   *slots      = hslots;
   *slots_qend = hslots_qend;

   DEXIT; 
   return 0;
}

/* 
 * Determine maximum number of host_slots as limited 
 * by queue load thresholds. The maximum only considers
 * thresholds with load adjustments
 * 
 * for each queue Q at this host {
 *    for each threshold T of a queue {
 *       avail(Q, T) = threshold - load / adjustment      
 *    }
 *    avail(Q) = MIN(all avail(Q, T))
 * }
 * host_slot_max_by_T = MAX(all min(Q))
 */
static double sge_max_host_slot_by_theshold(lListElem *hep, lList *queue_list, lList *centry_list, const  lList *load_adjustments)
{
   double avail_h = 0, avail_q;
   int avail;
   lListElem *next_queue, *qep;
   lListElem *lv, *lc, *tr, *fv, *cep;
   bool load_is_value;
   const char *eh_name = lGetHost(hep, EH_name);
   const void *queue_iterator = NULL;
   const char *load_value, *limit_value, *adj_value;
   u_long32 type;

   for (next_queue = lGetElemHostFirst(queue_list, QU_qhostname, eh_name, &queue_iterator); 
       (qep = next_queue);
        next_queue = lGetElemHostNext(queue_list, QU_qhostname, eh_name, &queue_iterator)) {

      avail_q = DBL_MAX;
      for_each (lc, load_adjustments) {
         const char *name = lGetString(lc, CE_name);
         if ((tr=lGetSubStr(qep, CE_name, name, QU_load_thresholds))) {
            double load, threshold, adjustment;
            cep = centry_list_locate(centry_list, name);
            if ((lv=lGetSubStr(hep, HL_name, name, EH_load_list))) {
               load_value = lGetString(lv, HL_value);
               load_is_value = true;
            } else {
               fv = lGetSubStr(qep, CE_name, name, QU_consumable_config_list);
               load_value = lGetString(fv, CE_stringval);
               load_is_value = false;
            }

            limit_value = lGetString(tr, CE_stringval);
            adj_value = lGetString(lc, CE_stringval);
            type = lGetUlong(cep, CE_valtype);

            switch (type) {
               case TYPE_INT:
               case TYPE_TIM:
               case TYPE_MEM:
               case TYPE_BOO:
               case TYPE_DOUBLE:

                  if (!parse_ulong_val(&load, NULL, type, load_value, NULL, 0) ||
                      !parse_ulong_val(&threshold, NULL, type, limit_value, NULL, 0) ||
                      !parse_ulong_val(&adjustment, NULL, type, adj_value, NULL, 0)) {
                     continue;
                  }

                  break;

               default:
                  continue;    
            }

            avail = (int)((threshold - load)/adjustment);
            avail_q = MIN(avail, avail_q);
         }
      }
      avail_h = MAX(avail_h, avail_q);
   }

   return avail_h;
}


/****** sge_select_queue/sge_sequential_assignment() ***************************
*  NAME
*     sge_sequential_assignment() -- Make an assignment for a sequential job.
*
*  SYNOPSIS
*     int sge_sequential_assignment(sge_assignment_t *assignment, 
*                      lList **ignore_hosts, lList **ignore_queues) 
*
*  FUNCTION
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
*     lList **ignore_hosts         - ??? 
*     lList **ignore_queues        - ??? 
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
int sge_sequential_assignment(
sge_assignment_t *a,
lList **ignore_hosts,
lList **ignore_queues
) {
   bool need_master_host;
   u_long32 job_id;
   int result;
   bool now_assignment;
   lListElem *job;

   DENTER(TOP_LAYER, "sge_sequential_assignment");

   if (!a) {
      DEXIT;
      return -1;
   }

   job = a->job;
   now_assignment = (a->start == DISPATCH_TIME_NOW);
   need_master_host = (lGetList(job, JB_master_hard_queue_list)!=NULL);
   job_id = lGetUlong(job, JB_job_number);

   /* untag all queues */
   qinstance_list_set_tag(a->queue_list, 0);

   host_order_2_queue_sequence_number(a->host_list, a->queue_list);

   if (sconf_get_qs_state()!=QS_STATE_EMPTY) {
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
         if (sconf_get_queue_sort_method() == QSM_LOAD)
            lPSortList(a->queue_list, "%I+ %I+", QU_host_seq_no, QU_seq_no);
         else
            lPSortList(a->queue_list, "%I+ %I+", QU_seq_no, QU_host_seq_no);
      }
      sconf_set_last_dispatch_type(DISPATCH_TYPE_FAST);
   }

   result = sge_tag_queues_suitable4job_fast_track(a, ignore_hosts, ignore_queues);

   if (result == 0) {
      lListElem *category = lGetRef(job, JB_category);
      bool use_category = now_assignment && (category != NULL) && lGetUlong(category, CT_refcount) > MIN_JOBS_IN_CATEGORY;
      lListElem *qep;
      u_long32 job_start_time = MAX_ULONG32;

      if (!now_assignment) {
         lListElem *earliest_queue = NULL;
         for_each (qep, a->queue_list) {
            DPRINTF(("    Q: %s "u32" "u32" (jst: "u32")\n", lGetString(qep, QU_full_name), 
                     lGetUlong(qep, QU_tag), lGetUlong(qep, QU_available_at), job_start_time));
            if (lGetUlong(qep, QU_tag) && job_start_time > lGetUlong(qep, QU_available_at)) {
               DPRINTF(("--> yep!\n"));
               earliest_queue = qep;
               job_start_time = lGetUlong(qep, QU_available_at);
            }
         }
         if (earliest_queue) {
            qep = earliest_queue;
            DPRINTF(("earliest queue \"%s\" at "u32"\n", lGetString(qep, QU_full_name), job_start_time));
         } else {
            DPRINTF(("no earliest queue found!\n"));
         }
      } else {
         for_each (qep, a->queue_list)
            if (lGetUlong(qep, QU_tag)) {
               job_start_time = lGetUlong(qep, QU_available_at);
               break;
            }
      }

      if (!qep) {
         DEXIT;
         return -1; /* should never happen */
      }
      {
         lListElem *gdil_ep;
         lList *gdil = NULL;
         const char *qname = lGetString(qep, QU_full_name);
         const char *eh_name = lGetHost(qep, QU_qhostname);

         DPRINTF((u32": 1 slot in queue %s@%s user %s %s for "u32"\n",
            job_id, qname, eh_name, lGetString(job, JB_owner), 
                  now_assignment?"scheduled":"reserved", job_start_time));

         gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
         lSetUlong(gdil_ep, JG_qversion, lGetUlong(qep, QU_version));
         lSetHost(gdil_ep, JG_qhostname, eh_name);
         lSetUlong(gdil_ep, JG_slots, 1);

         if (now_assignment) 
            scheduled_fast_jobs++;

         if (use_category) {  
            lList *temp = schedd_mes_get_tmp_list();
            if (temp){    
               lSetList(category, CT_job_messages, lCopyList(NULL, temp));
            }
         }

         a->gdil = lFreeList(a->gdil);
         a->gdil = gdil;
         if (a->start == DISPATCH_TIME_QUEUE_END) 
            a->start = job_start_time;
   
         result = 0;
      }
   }

   switch (result) {
   case 0:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("u32"."u32") returns <time> "u32"\n", 
            a->job_id, a->ja_task_id, a->start));
      break;
   case 1:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("u32"."u32") returns <later>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   case -1:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("u32"."u32") returns <category_never>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   case -2:
      DPRINTF(("SEQUENTIAL ASSIGNMENT("u32"."u32") returns <job_never>\n", 
            a->job_id, a->ja_task_id)); 
      break;
   default:
      DPRINTF(("!!!!!!!! SEQUENTIAL ASSIGNMENT("u32"."u32") returns unexpected %d\n", 
            a->job_id, a->ja_task_id, result));
      break;
   }

   DEXIT;
   return result;
}

/*------------------------------------------------------------------
 *  FAST TRACK FOR SEQUENTIAL JOBS WITHOUT A SOFT REQUEST 
 *
 *  It is much faster not to review slots in a comprehensive fashion 
 *  for jobs of this type.
 * ------------------------------------------------------------------*/
static int host_order_2_queue_sequence_number(lList *host_list, lList *queues)
{
   lListElem *hep, *qep;
   double previous_load = 0;
   int previous_load_inited = 0;
   int host_seqno = 0;
   const char *eh_name;
   const void *iterator = NULL;

   DENTER(TOP_LAYER, "host_order_2_queue_sequence_number");

   if (!sconf_get_host_order_changed()) {
      DEXIT;
      return 0;
   }

   sconf_set_host_order_changed(false);

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
         sconf_set_host_order_changed(true);
         lSetUlong(hep, EH_seq_no, host_seqno);
      }
   }

   DEXIT;
   return 0;
}

/****** sge_select_queue/sge_parallel_assignment() *****************************
*  NAME
*     sge_parallel_assignment() -- Can we assign with a fixed PE/slot/time
*
*  SYNOPSIS
*     int sge_parallel_assignment(sge_assignment_t *assignment) 
*
*  FUNCTION
*     Returns if possible an assignment for a particular PE with a 
*     fixed slot at a fixed time.
*
*  INPUTS
*     sge_assignment_t *a - 
*
*  RESULT
*     int - 0 ok got an assignment
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*          -2 assignment will never be possible for that particular job
*
*  NOTES
*     MT-NOTE: sge_parallel_assignment() is not MT safe 
*******************************************************************************/
int sge_parallel_assignment(
sge_assignment_t *a 
) {
   int ret;
   int pslots, pslots_qend;

   DENTER(TOP_LAYER, "sge_parallel_assignment");

   if (!a) {
      DEXIT;
      return -1;
   }

   if ((ret = pe_slots_by_time(a->start, a->duration, &pslots, &pslots_qend, a->job, a->pe, a->acl_list))) {
      DEXIT;
      return ret; 
   }
   if (a->slots > pslots ) {
      DEXIT;
      return (a->slots > pslots_qend)?-1:1;
   }

   ret = sge_tag_queues_suitable4job_comprehensively(a);
   if (ret!=0) {
      DEXIT;
      return ret;
   }

   /* must be understood in the context of changing queue sort orders */
   sconf_set_last_dispatch_type(DISPATCH_TYPE_COMPREHENSIVE);

   if (sge_sort_suitable_queues(a->queue_list)) {
      DEXIT;
      return -1;
   }

   if (sge_tags2gdil(a)) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return ret;
}


/****** sge_select_queue/sge_tags2gdil() ***************************************
*  NAME
*     sge_tags2gdil() -- Select slots in queues for the assignment 
*
*  SYNOPSIS
*     static int sge_tags2gdil(sge_assignment_t *assignment, lList *host_list, 
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
*     MT-NOTE: sge_tags2gdil() is not MT safe 
*******************************************************************************/
static int sge_tags2gdil(
sge_assignment_t *a
) {
   int max_host_seq_no, start_seq_no, last_accu_host_slots, accu_host_slots = 0;
   int host_slots;
   lList *gdil = NULL;
   const char *eh_name;
   const char *qname;
   lListElem *hep, *qep;
   int allocation_rule, minslots;
   bool need_master_host = (lGetList(a->job, JB_master_hard_queue_list)!=NULL);
   int host_seq_no = 1;
   
   DENTER(TOP_LAYER, "sge_tags2gdil");

   allocation_rule = sge_pe_slots_per_host(a->pe, a->slots);
   minslots = ALLOC_RULE_IS_BALANCED(allocation_rule)?allocation_rule:1;

   /* derive suitablility of host from queues suitability */
   for_each (hep, a->host_list) 
      lSetUlong(hep, EH_seq_no, -1);

   DPRINTF(("minslots = %d\n", minslots));

   /* change host sort order */
   for_each (qep, a->queue_list) {

      if (!lGetUlong(qep, QU_tag)) 
         continue;

      /* ensure host of this queue has enough slots */
      eh_name = lGetHost(qep, QU_qhostname);
      hep = host_list_locate(a->host_list, eh_name);
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
      for_each (hep, a->host_list) {
         if (lGetUlong(hep, EH_seq_no) != -1 && lGetUlong(hep, EH_master_host)) {
            if (!master_hep || lGetUlong(hep, EH_seq_no) < lGetUlong(master_hep, EH_seq_no) ) {
               master_hep = hep;
            }
         }
      }

      /* should be impossible to reach here without a master host */
      if (!master_hep) { 
         ERROR((SGE_EVENT, "no master host for job "u32"\n", 
            lGetUlong(a->job, JB_job_number)));
         DEXIT;
         return MATCH_LATER;
      }

      /* change order of queues in a way causing the best suited master 
         queue of the master host to be at the first position */
      master_eh_name = lGetHost(master_hep, EH_name);
      for_each (qep, a->queue_list) {
         if (sge_hostcmp(master_eh_name, lGetHost(qep, QU_qhostname)))
            continue;
         if (lGetUlong(qep, QU_tagged4schedule))
            break;
      }
      lDechainElem(a->queue_list, qep);
      lInsertElem(a->queue_list, NULL, qep);

      DPRINTF(("MASTER HOST %s MASTER QUEUE %s\n", 
            master_eh_name, lGetString(qep, QU_full_name)));
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
         } else if (allocation_rule==ALLOC_RULE_FILLUP) {
            host_slots = available;
         } else 
            host_slots = allocation_rule;
         lSetUlong(hep, EH_tagged, available - host_slots);

         DPRINTF(("allocating %d of %d slots at host %s (seqno = %d)\n", 
               host_slots, a->slots, eh_name, host_seq_no));

         for_each (qep, a->queue_list) {
            int qtagged;

            if (sge_hostcmp(eh_name, lGetHost(qep, QU_qhostname)))
               continue;

            qname = lGetString(qep, QU_full_name);
            /* how many slots ? */
            qtagged = lGetUlong(qep, QU_tag);
            slots = MIN(a->slots-accu_host_slots, 
               MIN(host_slots, qtagged));
            accu_host_slots += slots;
            host_slots -= slots;

            /* build gdil for that queue */
            DPRINTF((u32": %d slots in queue %s@%s user %s (host_slots = %d)\n", 
               a->job_id, slots, qname, eh_name, lGetString(a->job, JB_owner), host_slots));
            if (!(gdil_ep=lGetElemStr(gdil, JG_qname, qname))) {
               gdil_ep = lAddElemStr(&gdil, JG_qname, qname, JG_Type);
               lSetUlong(gdil_ep, JG_qversion, lGetUlong(qep, QU_version));
               lSetHost(gdil_ep, JG_qhostname, eh_name);
               lSetUlong(gdil_ep, JG_slots, slots);
            } else 
               lSetUlong(gdil_ep, JG_slots, lGetUlong(gdil_ep, JG_slots) + slots);

            /* untag */
            lSetUlong(qep, QU_tag, qtagged - slots);

            if (!host_slots) 
               break; 
         }
      }

      DPRINTF(("- - - accu_host_slots %d total_slots %d\n", accu_host_slots, a->slots));
      if (last_accu_host_slots == accu_host_slots) {
         DPRINTF(("!!! NO MORE SLOTS !!!\n"));
         lFreeList(gdil); 
         DEXIT;
         return MATCH_LATER;
      }
   } while (allocation_rule==ALLOC_RULE_ROUNDROBIN && accu_host_slots < a->slots);

   scheduled_complex_jobs++; /* ?? */

   a->gdil = lFreeList(a->gdil);
   a->gdil = gdil;
/*      ??? a->start = job_start_time; */

   DEXIT;
   return 0;
}



/****** sge_select_queue/sge_sort_suitable_queues() ****************************
*  NAME
*     sge_sort_suitable_queues() -- Sort queues according queue sort method
*
*  SYNOPSIS
*     static int sge_sort_suitable_queues(lList *queue_list) 
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
*     MT-NOTE: sge_sort_suitable_queues() is not MT safe 
*
*******************************************************************************/
static int sge_sort_suitable_queues(lList *queue_list)
{
   u_long32 qsm = sconf_get_queue_sort_method();
  
   DENTER(TOP_LAYER, "sge_sort_suitable_queues");

   /* Don't do this when the code is used only for dry runs to check whether
      assignments would be possible */
   if (sconf_get_qs_state()==QS_STATE_EMPTY) {
      return 0;
   }

   if (qsm == QSM_LOAD)
      lPSortList(queue_list, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_host_seq_no, QU_seq_no, QU_tag);
   else
      lPSortList(queue_list, "%I+ %I+ %I+ %I-", QU_soft_violation, QU_seq_no, QU_host_seq_no, QU_tag);

   DEXIT;
   return 0;
}

/****** sched/select_queue/queue_slots_by_time() *************************
*  NAME
*     queue_slots_by_time() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_NOW and 
*             DISPATCH_TIME_QUEUE_END (only with fixed_slot equals true)
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
int queue_slots_by_time(
u_long32 start,
u_long32 duration,
int *slots,
int *slots_qend, 
int *violations,
lListElem *job,
lListElem *qep,
const lListElem *pe,
const lListElem *ckpt,
lList *centry_list,
lList *acl_list,
bool allow_non_requestable)
{
   lList *hard_requests = lGetList(job, JB_hard_resource_list);
   lList *config_attr = lGetList(qep, QU_consumable_config_list);
   lList *actual_attr = lGetList(qep, QU_resource_utilization);
   const char *qname = lGetString(qep, QU_full_name);
   int qslots = 0, qslots_qend = 0;
   int result = -1;

   DENTER(TOP_LAYER, "queue_slots_by_time");

   if (queue_match_static(qep, job, pe, ckpt, centry_list, acl_list)==0) {
      result = rc_slots_by_time(hard_requests, start, duration, &qslots, &qslots_qend, 
            config_attr, actual_attr, NULL, centry_list, true, qep, 
            DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG, false, lGetString(qep, QU_full_name));
   }

   *slots = qslots;
   *slots_qend = qslots_qend;
   *violations = sge_soft_violations(NULL, *violations, job, NULL, config_attr, 
         actual_attr, centry_list, DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG);

   if (result == 0) {
      DPRINTF(("\tqueue_slots_by_time(%s) returns %d/%d\n", qname, qslots, qslots_qend));
   } else {
      DPRINTF(("\tqueue_slots_by_time(%s) returns <error>\n", qname));
   }

   DEXIT;
   return 0;
}

/****** sched/select_queue/queue_time_by_slots() *************************
*  NAME
*     queue_time_by_slots() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_NOW and 
*             DISPATCH_TIME_QUEUE_END (only with fixed_slot equals true)
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
int queue_time_by_slots(
int slots, 
u_long32 *start, 
u_long32 duration,
int *violations,
lListElem *job,
lListElem *qep,
const lListElem *pe,
const lListElem *ckpt,
lList *centry_list,
lList *acl_list)
{
   char reason[1024];
   int result;
   u_long32 tmp_time = *start;
   lList *hard_requests = lGetList(job, JB_hard_resource_list);
   lList *config_attr = lGetList(qep, QU_consumable_config_list);
   lList *actual_attr = lGetList(qep, QU_resource_utilization);
   const char *qname = lGetString(qep, QU_full_name);
   bool qend_assignment = (*start == DISPATCH_TIME_QUEUE_END);

   DENTER(TOP_LAYER, "queue_time_by_slots");

   if (queue_match_static(qep, job, pe, ckpt, centry_list, acl_list)!=0) {
      DEXIT;
      return -1;
   }

   result = rc_time_by_slots(hard_requests, NULL, config_attr, actual_attr, 
               centry_list, qep, 0, reason, sizeof(reason)-1, slots, DOMINANT_LAYER_QUEUE, 
               0, QUEUE_TAG, &tmp_time, duration, qname);
   if (result == 0) {
      *violations = sge_soft_violations(qep, *violations, job, NULL, config_attr, actual_attr, 
            centry_list, DOMINANT_LAYER_QUEUE, 0, QUEUE_TAG);
   }

   if (qend_assignment && result == 0) {
      *start = tmp_time;
      DPRINTF(("queue_time_by_slots(%s) returns earliest start time "u32"\n", qname, *start));
   } else if (result == 0) {
      DPRINTF(("queue_time_by_slots(%s) returns <at specified time>\n", qname));
   } else {
      DPRINTF(("queue_time_by_slots(%s) returns <later>\n", qname));
   }

   DEXIT;
   return result; 
}


/****** sge_select_queue/host_slots_by_time() ******************************
*  NAME
*     host_slots_by_time() -- Return host slots available at time period
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
int host_slots_by_time(   
u_long32 start, 
u_long32 duration,
int *slots, 
int *slots_qend, 
int *violations, 
lListElem *job, 
lListElem *ja_task, 
lListElem *hep, 
lList *queue_list, 
lList *centry_list, 
lList *acl_list,
const lList *load_adjustments, 
bool allow_non_requestable) 
{
   int hslots = 0, hslots_qend = 0;
   const char *eh_name;
   int result = -1;
   lList *hard_requests = lGetList(job, JB_hard_resource_list);
   lList *load_list = lGetList(hep, EH_load_list); 
   lList *config_attr = lGetList(hep, EH_consumable_config_list);
   lList *actual_attr = lGetList(hep, EH_resource_utilization);
   double lc_factor = 0;

   DENTER(TOP_LAYER, "host_slots_by_time");

   eh_name = lGetHost(hep, EH_name);

   if (host_match_static(job, ja_task, hep, centry_list, acl_list)==0) {

      /* cause load be raised artificially to reflect load correction when
         checking job requests */
      if (lGetPosViaElem(hep, EH_load_correction_factor) >= 0) {
         u_long32 ulc_factor;
         if ((ulc_factor=lGetUlong(hep, EH_load_correction_factor)))
            lc_factor = ((double)ulc_factor)/100;
      }

      result = rc_slots_by_time(hard_requests, start, duration, &hslots, &hslots_qend, 
            config_attr, actual_attr, load_list, centry_list, false, NULL, 
               DOMINANT_LAYER_HOST, lc_factor, HOST_TAG, false, lGetHost(hep, EH_name));

      if (hslots>0 && start == DISPATCH_TIME_NOW) {
         /* 
          * TODO: We consider queue thresholds here only for now assignments. Well, 
          *       this is not yet consistent with how adjusted load is treated 
          *       in ri_slots_by_time().
          */
         int t_max = sge_max_host_slot_by_theshold(hep, queue_list, centry_list, load_adjustments);
         if (t_max<hslots) {
            DPRINTF(("\thost_slots_by_time(%s) threshold load adjustment reduces slots"
                  " from %d to %d\n", eh_name, hslots, t_max));
            hslots = t_max;
         }
      }
   }

   *slots = hslots;
   *slots_qend = hslots_qend;
   *violations = sge_soft_violations(NULL, *violations, job, NULL, config_attr, 
         actual_attr, centry_list, DOMINANT_LAYER_HOST, 0, HOST_TAG);

   if (result == 0) {
      DPRINTF(("\thost_slots_by_time(%s) returns %d/%d\n", eh_name, hslots, hslots_qend));
   } else {
      DPRINTF(("\thost_slots_by_time(%s) returns <error>\n", eh_name));
   }

   DEXIT;
   return result; /* ?? */
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
int host_time_by_slots(   
int slots, 
u_long32 *start, 
u_long32 duration,
int *violations, 
lListElem *job, 
lListElem *ja_task, 
lListElem *hep, 
lList *centry_list, 
lList *acl_list)
{
   lList *hard_requests = lGetList(job, JB_hard_resource_list);
   lList *load_attr = lGetList(hep, EH_load_list); 
   lList *config_attr = lGetList(hep, EH_consumable_config_list);
   lList *actual_attr = lGetList(hep, EH_resource_utilization);
   double lc_factor = 0;
   u_long32 ulc_factor;
   int result;
   u_long32 tmp_time = *start;
   bool qend_assignment = (tmp_time == DISPATCH_TIME_QUEUE_END);
   const char *eh_name = lGetHost(hep, EH_name);
  
   DENTER(TOP_LAYER, "host_time_by_slots");

   if ((result=host_match_static(job, ja_task, hep, centry_list, acl_list))) {
      DEXIT;
      return result;
   }

   /* cause load be raised artificially to reflect load correction when
      checking job requests */
   if (lGetPosViaElem(hep, EH_load_correction_factor) >= 0) {
      if ((ulc_factor=lGetUlong(hep, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   }

   clear_resource_tags(hard_requests, HOST_TAG);

   result = rc_time_by_slots(hard_requests, load_attr, 
         config_attr, actual_attr, centry_list, NULL, 0, 
         NULL, 0, slots, DOMINANT_LAYER_HOST, 
         lc_factor, HOST_TAG, &tmp_time, duration, eh_name);
   if (result == 0) {
      *violations = sge_soft_violations(NULL, *violations, job, NULL, config_attr, 
            actual_attr, centry_list, DOMINANT_LAYER_HOST, 0, HOST_TAG);
   }

   if (qend_assignment && result == 0) {
      *start = tmp_time;
      DPRINTF(("host_time_by_slots(%s) returns earliest start time "u32"\n", eh_name, *start));
   } else if (result == 0) {
      DPRINTF(("host_time_by_slots(%s) returns <at specified time>\n", eh_name));
   } else {
      DPRINTF(("host_time_by_slots(%s) returns <later>\n", eh_name));
   }

   DEXIT;
   return result; 
}

/****** sched/select_queue/global_time_by_slots() ***************************
*  NAME
*     global_time_by_slots() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_QUEUE_END
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
int global_time_by_slots(
int slots,
u_long32 *start, 
u_long32 duration,
int *violations,
lListElem *job,
lListElem *gep,
lList *centry_list,
lList *acl_list) 
{
   char reason[1024];
   int result = -1;
   u_long32 tmp_time = *start; 
   lList *hard_request = lGetList(job, JB_hard_resource_list);
   lList *load_attr = lGetList(gep, EH_load_list); 
   lList *config_attr = lGetList(gep, EH_consumable_config_list);
   lList *actual_attr = lGetList(gep, EH_resource_utilization);
   double lc_factor=0.0;
   u_long32 ulc_factor;
   bool qend_assignment = (tmp_time == DISPATCH_TIME_QUEUE_END);

   DENTER(TOP_LAYER, "global_time_by_slots");

   if (!slots) {
      DEXIT;
      return -1;
   }

   /* check if job has access to any hosts globally */
   if ((result=host_match_static(job, NULL, gep, centry_list, 
            acl_list))!=0) {
      DEXIT;
      return result;
   }
   
   /* cause global load be raised artificially to reflect load correction when
      checking job requests */
   if (lGetPosViaElem(gep, EH_load_correction_factor) >= 0) {
      if ((ulc_factor=lGetUlong(gep, EH_load_correction_factor)))
         lc_factor = ((double)ulc_factor)/100;
   }

   clear_resource_tags(hard_request, GLOBAL_TAG);

   result = rc_time_by_slots(hard_request, load_attr, config_attr, actual_attr, centry_list, NULL, 0, reason, 
       sizeof(reason)-1, slots, DOMINANT_LAYER_GLOBAL, lc_factor, GLOBAL_TAG, &tmp_time, duration, SGE_GLOBAL_NAME);
   if (result == 0) {
      *violations = sge_soft_violations(NULL, *violations, job, NULL, config_attr, 
            actual_attr, centry_list, DOMINANT_LAYER_GLOBAL, 0, GLOBAL_TAG);
   }

   if (qend_assignment && result == 0) {
      *start = tmp_time;
      DPRINTF(("global_time_by_slots() returns earliest start time "u32"\n", *start));
   } else if (result == 0) {
      DPRINTF(("global_time_by_slots() returns <at specified time>\n"));
   } else {
      DPRINTF(("global_time_by_slots() returns <later>\n"));
   }


   DEXIT;
   return result;
}

/****** sched/select_queue/global_slots_by_time() ***************************
*  NAME
*     global_slots_by_time() -- 
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_QUEUE_END
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
******************************************************************************/
int global_slots_by_time(
u_long32 start, 
u_long32 duration,
int *slots, 
int *slots_qend, 
int *violations,
lListElem *job,
lListElem *gep,
lList *centry_list,
lList *acl_list) 
{
   int result = -1;
   lList *hard_request = lGetList(job, JB_hard_resource_list);
   lList *load_attr = lGetList(gep, EH_load_list); 
   lList *config_attr = lGetList(gep, EH_consumable_config_list);
   lList *actual_attr = lGetList(gep, EH_resource_utilization);
   double lc_factor=0.0;
   u_long32 ulc_factor;
   int gslots = 0, gslots_qend = 0;

   DENTER(TOP_LAYER, "global_slots_by_time");

   /* check if job has access to any hosts globally */
   if (host_match_static(job, NULL, gep, centry_list, 
            acl_list)==0) {
      /* cause global load be raised artificially to reflect load correction when
         checking job requests */
      if (lGetPosViaElem(gep, EH_load_correction_factor) >= 0)
         if ((ulc_factor=lGetUlong(gep, EH_load_correction_factor)))
            lc_factor = ((double)ulc_factor)/100;

      result = rc_slots_by_time(hard_request, start, duration, &gslots, 
            &gslots_qend, config_attr, actual_attr, load_attr, centry_list, 
                  false, NULL, DOMINANT_LAYER_GLOBAL, lc_factor, GLOBAL_TAG, false, SGE_GLOBAL_NAME);
   }

   *slots      = gslots;
   *slots_qend = gslots_qend;
   *violations = sge_soft_violations(NULL, *violations, job, NULL, config_attr, 
                  actual_attr, centry_list, DOMINANT_LAYER_GLOBAL, 0, GLOBAL_TAG);

   if (result == 0) {
      DPRINTF(("\tglobal_slots_by_time() returns %d/%d\n", gslots, gslots_qend));
   } else {
      DPRINTF(("\tglobal_slots_by_time() returns <error>\n"));
   }

   DEXIT;
   return result; /* ?? */
}

/****** sge_select_queue/pe_slots_by_time() **********************************
*  NAME
*     pe_slots_by_time() -- Check if number of PE slots is available 
*
*  SYNOPSIS
*
*  FUNCTION
*
*  INPUTS
*
*  RESULT
*     int - 0 ok got an assignment
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*
*  NOTES
*     MT-NOTE: pe_slots_by_time() is not MT safe 
*******************************************************************************/
int pe_slots_by_time(
u_long32 start, 
u_long32 duration,
int *slots, 
int *slots_qend, 
lListElem *job,
lListElem *pe,
lList *acl_list)
{
   int result;
   int total = lGetUlong(pe, PE_slots);
   int pslots, pslots_qend;
   static lListElem *implicit_slots_request = NULL;
   static lList *implicit_total_list = NULL;
   lListElem *tep = NULL;
   char strbuf[100];
   dstring slots_as_str;

   DENTER(TOP_LAYER, "pe_slots_by_time");

   if ((result=pe_match_static(job, pe, acl_list))) {
      DEXIT;
      return result;
   }

   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, SGE_ATTR_SLOTS);
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* PE slots should be stored in a PE_consumable_config_list ... */
   if (!implicit_total_list)
      tep = lAddElemStr(&implicit_total_list, CE_name, SGE_ATTR_SLOTS, CE_Type);
   if (!tep && !(tep = lGetElemStr(implicit_total_list, CE_name, SGE_ATTR_SLOTS))) {
      DEXIT;
      return -1;
   }

   total = lGetUlong(pe, PE_slots);
   lSetDouble(tep, CE_doubleval, total);
   sge_dstring_init(&slots_as_str, strbuf, sizeof(strbuf));
   sge_dstring_sprintf(&slots_as_str, "%d", total);
   lSetString(tep, CE_stringval, strbuf);

   if (ri_slots_by_time(start, duration, &pslots, &pslots_qend, 
         lGetList(pe, PE_resource_utilization), implicit_slots_request, 
         NULL, implicit_total_list, NULL, NULL, 0, 0, NULL, 0, true, true,
         lGetString(pe, PE_name))) {
      DEXIT;
      return -1;
   }

   if (slots)
      *slots = pslots;
   if (slots_qend)
      *slots_qend = pslots_qend;

   DPRINTF(("\tpe_slots_by_time(%s) returns %d/%d\n", lGetString(pe, PE_name),
         pslots, pslots_qend));

   DEXIT;
   return 0;
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

   global = host_list_locate(exechost_list, "global"); 
   host = host_list_locate(exechost_list, lGetHost(q, QU_qhostname));

   /* find matching */
   *has_value_from_object = false;
   if (( ep = get_attribute_by_name(global, host, q, attrname, centry_list, NULL, 0)) &&
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
   ep = lFreeElem(ep);

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
const lList *exechost_list,
const lList *centry_list 
) {
   lListElem *ep;
   lListElem *global = NULL;
   lListElem *host = NULL;
   int ret = 0;

   DENTER(TOP_LAYER, "sge_get_string_qattr");

   global = host_list_locate(exechost_list, "global"); 
   host = host_list_locate(exechost_list, lGetHost(q, QU_qhostname));

   ep = get_attribute_by_name(global, host, q, attrname, centry_list, NULL, 0);

   /* first copy ... */
   if (ep && dst)
      strncpy(dst, lGetString(ep, CE_stringval), dst_len);

   if(ep){
      ep = lFreeElem(ep);
      ret = 0;
   }
   else
      ret = -1;
   /* ... and then free */

   DEXIT;
   return ret; 
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
lList *centry_list,
lList *orders_list    /* needed to warn on jobs that get dispatched and suspended
                        on subordinate in the very same interval */
) {
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
         qep = lGetElemStr(global_queue_list, QU_full_name, qname);

         /* increase used slots */
         qslots = qinstance_slots_used(qep);

         /* precompute suspensions for subordinated queues */
         total = lGetUlong(qep, QU_job_slots);
         for_each (so, lGetList(qep, QU_subordinate_list)) {
            if (!tst_sos(qslots,        total, so)  &&  /* not suspended till now */
                 tst_sos(qslots+tagged, total, so)) {   /* but now                */
               ret |= sos_schedd(lGetString(so, SO_name), global_queue_list);

               /* warn on jobs that were dispatched into that queue in
                  the same scheduling interval based on the orders list */
               {
                  lListElem *order;
                  for_each (order, orders_list) {
                     if (lGetUlong(order, OR_type) != ORT_start_job)
                        continue;
                     if (lGetSubStr(order, OQ_dest_queue, lGetString(so, SO_name), OR_queuelist)) {
                        WARNING((SGE_EVENT, MSG_SUBORDPOLICYCONFLICT_UUSS, u32c(lGetUlong(job, JB_job_number)),
                        u32c(lGetUlong(order, OR_job_number)), qname, lGetString(so, SO_name)));
                     }
                  }
               }
            }
         }

         DPRINTF(("REDUCING SLOTS OF QUEUE %s BY %d\n", qname, tagged));

         qinstance_debit_consumable(qep, job, centry_list, tagged);
      }
   }

   DEXIT;
   return ret;
}


/****** sge_select_queue/ri_time_by_slots() ******************************************
*  NAME
*     ri_time_by_slots() -- Determine availability time through slot number
*
*  SYNOPSIS
*     static int ri_time_by_slots(lListElem *rep, lList *load_attr, lList 
*     *config_attr, lList *actual_attr, lList *centry_list, lListElem *queue, 
*     char *reason, int reason_size, int allow_non_requestable, int slots, 
*     u_long32 layer, double lc_factor) 
*
*  FUNCTION
*     Checks for one level, if one request is fulfilled or not. 
*
*  INPUTS
*     lListElem *rep            - requested attribut 
*     lList *load_attr          - list of load attributes or null on queue level 
*     lList *config_attr        - list of user defined attributes (CE_Type)
*     lList *actual_attr        - usage of user consumables (RUE_Type)
*     lList *centry_list        - the system wide attribut configuration list 
*i    lListElem *queue          - the current queue, or null on host level 
*     char *reason              - target for error message 
*     int reason_size           - max length for error message 
*     int allow_non_requestable - allow none requestable attributes? 
*     int slots                 - the number of slotes the job is looking for? 
*     u_long32 layer            - the current layer 
*     double lc_factor          - load correction factor 
*     u_long32 *start_time      - in/out argument for start time  
*     u_long32 duration         - jobs estimated total run time
*     const char *object_name   - name of the object used for monitoring purposes
*
*  RESULT
*     int - 0 ok got an assignment + set time for DISPATCH_TIME_NOW and 
*             DISPATCH_TIME_QUEUE_END
*           2 attribute does not exist 
*           1 no assignment at the specified time
*          -1 assignment will never be possible for all jobs of that category
*******************************************************************************/
static int ri_time_by_slots(lListElem *rep, lList *load_attr, lList *config_attr, lList *actual_attr, 
      lList *centry_list, lListElem *queue, char *reason, int reason_size, int allow_non_requestable, 
      int slots, u_long32 layer, double lc_factor, u_long32 *start_time, u_long32 duration, 
      const char *object_name) 
{
   lListElem *cplx_el=NULL;
   const char *attrname; 
   int ret = 0;
   lListElem *actual_el;
   u_long32 ready_time;
   double util, total, request = 0;
   lListElem *capacitiy_el;
   bool schedule_based;
   const char *request_str; 
   u_long32 now = sconf_get_now();

   DENTER(TOP_LAYER, "ri_time_by_slots");

   attrname = lGetString(rep, CE_name);
   actual_el = lGetElemStr(actual_attr, RUE_name, attrname);
   ready_time = *start_time;

   /*
    * Consumables are treated futher below in schedule based mode 
    * thus we always assume zero consumable utilization here 
    */
   schedule_based = (duration != 0 && sconf_get_max_reservations()>0);

   if (!(cplx_el = get_attribute(attrname, config_attr, actual_attr, load_attr, centry_list, queue,layer, 
                        lc_factor, reason, reason_size, schedule_based))) {
      DEXIT;
      return 2; 
   }

   ret = match_static(slots, rep, cplx_el, reason, reason_size, false, false, allow_non_requestable);
   if (ret != 0 || !schedule_based) {
      DEXIT;
      return ret;
   }

   DPRINTF(("ri_time_by_slots(%s) consumable = %s\n", 
         attrname, (lGetBool(cplx_el, CE_consumable)?"true":"false")));

   if (!lGetBool(cplx_el, CE_consumable)) {
      if (ready_time == DISPATCH_TIME_QUEUE_END)
         *start_time = now;
      DPRINTF(("%s: ri_time_by_slots(%s) <is no consumable>\n", object_name, attrname));
      DEXIT;
      return 0; /* already checked */
   }
      
   /* we're done if there is no consumable capacity */
   if (!(capacitiy_el = lGetElemStr(config_attr, CE_name, attrname))) {
      DPRINTF(("%s: ri_time_by_slots(%s) <does not exist>\n", object_name, attrname));
      DEXIT;
      return 2; /* does not exist */
   }
      
   /* determine 'total' and 'request' values */
   total = lGetDouble(capacitiy_el, CE_doubleval);

   request_str = lGetString(rep, CE_stringval);
   if (!parse_ulong_val(&request, NULL, lGetUlong(cplx_el, CE_valtype), 
      lGetString(rep, CE_stringval), NULL, 0)) {
      if (reason)
         strncpy(reason, "error", reason_size);

      lFreeElem(cplx_el);
      DEXIT;
      return -1;
   }
   lFreeElem(cplx_el);

   if (ready_time == DISPATCH_TIME_QUEUE_END) {
      double threshold = total - request * slots;

      /* verify there are enough resources in principle */
      if (threshold<0) 
         ret = -1;
      else {
         /* seek for the time near queue end where resources are sufficient */
         u_long32 when = utilization_below(actual_el, threshold, object_name);
         if (when == 0) {
            /* may happen only if scheduler code is run outside scheduler with 
               DISPATCH_TIME_QUEUE_END time spec */
            *start_time = now;
         } else
            *start_time = when;
         ret = 0;

         DPRINTF(("\t\t%s: time_by_slots: %d of %s=%f can be served %s\n", 
               object_name, slots, attrname, request,
                  ret == 0 ? "at time" : "never"));

      }

      DEXIT;
      return ret;
   } 

   /* here we handle DISPATCH_TIME_NOW + any other time */
   if (*start_time == DISPATCH_TIME_NOW)
      ready_time = now;
   else
      ready_time = *start_time;

   util = utilization_max(actual_el, ready_time, duration);

DPRINTF(("\t\t%s: time_by_slots: %s total = %f util = %f\n", object_name, attrname, total, util));

   /* ensure resource is sufficient from now until finish */
   if (request * slots > total - util) {
      /* we can't assign right now - maybe later ? */  
      if (request * slots > total) {
         DPRINTF(("\t\t%s: time_by_slots: %s %f > %f\n", object_name, attrname,
            request * slots, total));
         ret = -1; /* surely not */
      } else {
         DPRINTF(("\t\t%s: time_by_slots: %s %f > %f\n", object_name, attrname, 
            request * slots, total - util));
         ret = 1;
      }
   } else 
      ret = 0;

   DPRINTF(("\t\t%s: time_by_slots: %d of %s=%f can be served %s\n", 
         object_name, slots, attrname, request, 
            ret == 0 ? "at time" : ((ret == 1)? "later":"never")));

   DEXIT;
   return ret;
}

static int ri_slots_by_time(u_long32 start, u_long32 duration, int *slots, int *slots_qend, 
   lList *rue_list, lListElem *request, lList *load_attr, lList *total_list, 
   lListElem *queue, lList *centry_list, u_long32 layer, double lc_factor, char *reason, 
   int reason_size, bool allow_non_requestable, bool no_centry, const char *object_name) 
{
   const char *name;
   lListElem *cplx_el, *uep, *tep = NULL;
   bool now_assignment = (start == DISPATCH_TIME_NOW);

   /* always assume zero consumable utilization in schedule based mode */
   bool schedule_based = (duration != 0 && sconf_get_max_reservations()>0);
   int ret;
   double used, total, request_val;

   DENTER(TOP_LAYER, "ri_slots_by_time");

   name = lGetString(request, CE_name);
   uep = lGetElemStr(rue_list, RUE_name, name);

   DPRINTF(("\t\t%s: ri_slots_by_time(%s)\n", object_name, name));

   if (!no_centry) {
      if (!(cplx_el = get_attribute(name, total_list, rue_list, load_attr, centry_list, queue,layer, 
                           lc_factor, reason, reason_size, schedule_based))) {
         DEXIT;
         return 2; /* does not exist */
      }

      ret = match_static(1, request, cplx_el, reason, reason_size, false, false, allow_non_requestable);
      if (ret != 0) {
         DEXIT;
         return ret;
      }

      if (ret == 0 && !lGetBool(cplx_el, CE_consumable)) {
         *slots      = INT_MAX;
         *slots_qend = INT_MAX;
         DEXIT;
         return 0; /* no limitations */
      }

      /* we're done if there is no consumable capacity */
      if (!(tep=lGetElemStr(total_list, CE_name, name))) {
         lFreeElem(cplx_el);
         *slots      = INT_MAX;
         *slots_qend = INT_MAX;
         DEXIT;
         return 0;
      }

      lFreeElem(cplx_el);
   }

   if (!tep && !(tep=lGetElemStr(total_list, CE_name, name))) {
      DEXIT;
      return -1;
   }
   total = lGetDouble(tep, CE_doubleval);

   if (sconf_get_qs_state()==QS_STATE_EMPTY) {
      used = 0;
   } else if (schedule_based) {
      if (start == DISPATCH_TIME_NOW)
         start = sconf_get_now();
      used = utilization_max(uep, start, duration);
      DPRINTF(("\t\t%s: ri_slots_by_time: utilization_max("u32", "u32") returns %f\n", 
            object_name, start, duration, used));
   } else {
      used = lGetDouble(lGetElemStr(rue_list, RUE_name, name), RUE_utilized_now);
   }

   request_val = lGetDouble(request, CE_doubleval);
   *slots      = (int)((total - used) / request_val);
   *slots_qend = (int)(total / request_val);

   DPRINTF(("\t\t%s: ri_slots_by_time: %s=%f has %d (%d) slots at time "U32CFormat"%s (avail: %f total: %f)\n", 
         object_name, lGetString(uep, RUE_name), request_val, *slots, *slots_qend, start,
           now_assignment?" (= now)":"", total - used, total));

   DEXIT;
   return 0;
}


/* Determine maximum number of host_slots as limited
   by job request to this host 

   for each resource at this host requested by the job {
      avail(R) = (total - used) / request
   }
   host_slot_max_by_R = MIN(all avail(R))

   host_slot = MIN(host_slot_max_by_T, host_slot_max_by_R)

*/
static int rc_slots_by_time(lList *requests, u_long32 start, u_long32 duration, 
      int *slots, int *slots_qend, lList *total_list, lList *rue_list, lList *load_attr, 
      lList *centry_list, bool force_slots, lListElem *queue, u_long32 layer, double lc_factor, 
      u_long32 tag, bool allow_non_requestable, const char *object_name)
{
   int avail, avail_qend;
   int max_slots = INT_MAX, max_slots_qend = INT_MAX;
   const char *name;
   static lListElem *implicit_slots_request = NULL;
   lListElem *tep, *cep, *actual, *req;
   int result;

   DENTER(TOP_LAYER, "rc_slots_by_time");

   if (!implicit_slots_request) {
      implicit_slots_request = lCreateElem(CE_Type);
      lSetString(implicit_slots_request, CE_name, SGE_ATTR_SLOTS);
      lSetString(implicit_slots_request, CE_stringval, "1");
      lSetDouble(implicit_slots_request, CE_doubleval, 1);
   }

   /* --- implicit slot request */
   name = SGE_ATTR_SLOTS;
   if (!(tep = lGetElemStr(total_list, CE_name, name)) && force_slots) {
      DEXIT;
      return 0;
   }
   if (tep) {
      if (ri_slots_by_time(start, duration, &avail, &avail_qend, 
            rue_list, implicit_slots_request, load_attr, total_list, queue, centry_list, layer, lc_factor, 
            NULL, 0, allow_non_requestable, false, object_name)) {
         DEXIT;
         return -1;
      }
      max_slots      = MIN(max_slots,      avail);
      max_slots_qend = MIN(max_slots_qend, avail_qend);
      DPRINTF(("%s: rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
            (int)max_slots, (int)max_slots_qend));
   }


   /* --- default request */
   for_each (actual, rue_list) {
      name = lGetString(actual, RUE_name);
      if (!strcmp(name, SGE_ATTR_SLOTS))
         continue;
      cep = centry_list_locate(centry_list, name);

      if (!is_requested(requests, name)) {
         double request;
         const char *def_req = lGetString(cep, CE_default);
         if (def_req) {
            parse_ulong_val(&request, NULL, lGetUlong(cep, CE_valtype), def_req, NULL, 0);

            if (request != 0) {

               lSetString(cep, CE_stringval, def_req);
               lSetDouble(cep, CE_doubleval, request);

               if (ri_slots_by_time(start, duration, &avail, &avail_qend, 
                        rue_list, cep, load_attr, total_list, queue, centry_list, layer, lc_factor,   
                        NULL, 0, allow_non_requestable, false, object_name)==-1) {
                  DEXIT;
                  return -1;
               }
               max_slots      = MIN(max_slots,      avail);
               max_slots_qend = MIN(max_slots_qend, avail_qend);
               DPRINTF(("%s: rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
                     (int)max_slots, (int)max_slots_qend));
            }
         }
      } 
   }

   /* --- explicit requests */
   for_each (req, requests) {
      name = lGetString(req, CE_name);
      result = ri_slots_by_time(start, duration, &avail, &avail_qend, 
               rue_list, req, load_attr, total_list, queue, centry_list, layer, lc_factor,   
               NULL, 0, allow_non_requestable, false, object_name);

      switch (result) {
         case 0: /* the requested element does not exist */
         case 1: /* will match later-on */

            DPRINTF(("%s: explicit request for %s gets us %d slots (%d later)\n", 
                  object_name, name, avail, avail_qend));
            if (lGetUlong(req, CE_tagged) < tag)
               lSetUlong(req, CE_tagged, tag);

            max_slots      = MIN(max_slots,      avail);
            max_slots_qend = MIN(max_slots_qend, avail_qend);
            DPRINTF(("%s: rc_slots_by_time(%s) %d (%d later)\n", object_name, name, 
                  (int)max_slots, (int)max_slots_qend));
            break;

         case -1: /* the requested element does not exist */
   
            DPRINTF(("%s: rc_slots_by_time(%s) <never>\n", object_name, name));
            *slots = *slots_qend = 0;
            DEXIT;
            return -1;

         case 2: /* the requested element does not exist */
            if (tag == QUEUE_TAG && lGetUlong(req, CE_tagged) == NO_TAG) {
               DPRINTF(("%s: rc_slots_by_time(%s) <never found>\n", object_name, name));
               *slots = *slots_qend = 0;
               DEXIT;
               return -1;
            }
            DPRINTF(("%s: rc_slots_by_time(%s) no such resource, but already satisified\n", 
                     object_name, name));
            break;
      }
   }

   *slots = max_slots;
   *slots_qend = max_slots_qend;

   DEXIT;
   return 0;
}
