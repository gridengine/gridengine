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
 *  License at http://www.gridengine.sunsource.net/license.html
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
/*----------------------------------------------------------------
 * sge.c
 *---------------------------------------------------------------*/

#define SGE_INCLUDE_QUEUED_JOBS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#include "sge.h"
#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_copy_append.h"
#include "commlib.h"
#include "sge_time.h"
#include "complex.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "scheduler.h"
/* #include "sge_schedd.h" */
#include "sge_orders.h"
#include "sge_job_schedd.h"

#include "schedd_conf.h"

#include "sgeee.h"
#include "sge_schedconfL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_userprjL.h"
#include "sge_share_tree_nodeL.h"
/* #include "sge_sharetree.h" */
#include "sge_usageL.h"
#include "sge_hostL.h"
#include "sge_queueL.h"
#include "sge_usersetL.h"
#include "sge_eejobL.h"

#include "sort_hosts.h"
#include "msg_schedd.h"
#include "sge_language.h"
#include "sge_string.h"

static sge_Sdescr_t *all_lists;
static lListElem *get_mod_share_tree(lListElem *node, lEnumeration *what, int seqno);

#define SGE_MIN_USAGE 1.0

/*--------------------------------------------------------------------
 * job_is_active - returns true if job is active
 *--------------------------------------------------------------------*/

static int job_is_active(lListElem *job, lListElem *ja_task);

static int
job_is_active(
lListElem *job,
lListElem *ja_task 
) {
   u_long job_status = lGetUlong(ja_task, JAT_status);
#ifdef SGE_INCLUDE_QUEUED_JOBS
   return (job_status == JIDLE ||
       job_status & (JRUNNING | JMIGRATING | JQUEUED | JTRANSITING)) &&
       !(lGetUlong(ja_task, JAT_state) 
         & (JSUSPENDED|JSUSPENDED_ON_THRESHOLD));
#else
   return (job_status & (JRUNNING | JMIGRATING | JTRANSITING)) &&
       !(lGetUlong(ja_task, JAT_state) & 
         (JSUSPENDED|JSUSPENDED_ON_THRESHOLD));
#endif
}


/*--------------------------------------------------------------------
 * locate_user_or_project - locate the user or project object by name
 *--------------------------------------------------------------------*/

static lListElem *locate_user_or_project(lList *user_list, char *name);

static lListElem *
locate_user_or_project(
lList *user_list,
char *name 
) {
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the user or project object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(user_list, UP_name, name);
}


/*--------------------------------------------------------------------
 * locate_department - locate the department object by name
 *--------------------------------------------------------------------*/

static lListElem *locate_department(lList *dept_list, char *name);

static lListElem *
locate_department(
lList *dept_list,
char *name 
) {
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the department object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(dept_list, US_name, name);
}


/*--------------------------------------------------------------------
 * locate_jobclass - locate the job class object by name
 *--------------------------------------------------------------------*/

static lListElem *locate_jobclass(lList *job_class_list, char *name);

static lListElem *
locate_jobclass(
lList *job_class_list,
char *name 
) {
   if (!name)
      return NULL;

   /*-------------------------------------------------------------
    * Look up the job class object by name
    *-------------------------------------------------------------*/

   return lGetElemStr(job_class_list, QU_qname, name);
}


/*--------------------------------------------------------------------
 * sge_set_job_refs - set object references in the job entry
 *--------------------------------------------------------------------*/

void sge_set_job_refs(lListElem *job, lListElem *ja_task, sge_ref_t *ref, sge_Sdescr_t *lists);

void
sge_set_job_refs(
lListElem *job,
lListElem *ja_task,
sge_ref_t *ref,
sge_Sdescr_t *lists 
) {
   lListElem *root;
   lList *granted;
   lListElem *pe, *granted_el;
   char *pe_str;

   if (ref->task_jobclass)
      free(ref->task_jobclass);

   memset(ref, 0, sizeof(sge_ref_t));

   /*-------------------------------------------------------------
    * save job reference
    *-------------------------------------------------------------*/

   ref->job = job;
   ref->ja_task = ja_task;

   /*-------------------------------------------------------------
    * locate user object and save off reference
    *-------------------------------------------------------------*/

   ref->user = locate_user_or_project(lists->user_list,
                                      lGetString(job, JB_owner));
   if (ref->user)
      lSetUlong(ref->user, UP_job_cnt, 0);

   /*-------------------------------------------------------------
    * locate project object and save off reference
    *-------------------------------------------------------------*/

   ref->project = locate_user_or_project(lists->project_list,
                                         lGetString(job, JB_project));
   if (ref->project)
      lSetUlong(ref->project, UP_job_cnt, 0);

   /*-------------------------------------------------------------
    * locate job class object and save off reference
    *-------------------------------------------------------------*/

   ref->jobclass = locate_jobclass(lists->all_queue_list,
                                   lGetString(ja_task, JAT_master_queue));
   if (ref->jobclass)
      lSetUlong(ref->jobclass, QU_job_cnt, 0);

   /*-------------------------------------------------------------
    * for task-controlled PE jobs
    *    for each task
    *       locate job class object and save off reference
    *-------------------------------------------------------------*/

   if ((pe_str=lGetString(ja_task, JAT_granted_pe))
         && (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str))
         && lGetUlong(pe, PE_control_slaves)
         && (granted=lGetList(ja_task, JAT_granted_destin_identifier_list))) {

      ref->num_task_jobclasses = lGetNumberOfElem(granted);

      if (ref->num_task_jobclasses > 0) {
         int i=0;
         ref->task_jobclass = (lListElem **)malloc(ref->num_task_jobclasses * sizeof(lListElem *));
         memset((void *)ref->task_jobclass, 0, ref->num_task_jobclasses * sizeof(lListElem *));

         for_each(granted_el, granted) {
            if (active_subtasks(job, lGetString(granted_el, JG_qname)))
               ref->task_jobclass[i] = locate_jobclass(lists->all_queue_list,
                                                       lGetString(granted_el, JG_qname));
            i++;
         }
      }
   }

   /*-------------------------------------------------------------
    * locate share tree node and save off reference
    *-------------------------------------------------------------*/

   if ((root = lFirst(lists->share_tree))) {
      lListElem *pnode = NULL;
       
      ref->share_tree_type = lGetUlong(root, STN_type);

#ifdef notdef
      if (ref->share_tree_type == STT_PROJECT)
         userprj = ref->project;
      else
         userprj = ref->user;
#endif

      if (ref->user || ref->project) {
         ref->node = search_userprj_node(root,
               ref->user ? lGetString(ref->user, UP_name) : NULL,
               ref->project ? lGetString(ref->project, UP_name) : NULL,
               &pnode);
         /*
          * if the found node is the "default" node, then create a
          * temporary sibling node using the "default" node as a
          * template
          */

         if (ref->user && ref->node && pnode &&
             strcmp(lGetString(ref->node, STN_name), "default") == 0) {
            ref->node = lCopyElem(ref->node);
            lSetString(ref->node, STN_name, lGetString(ref->user, UP_name));
            lSetUlong(ref->node, STN_temp, 1);
            lAppendElem(lGetList(pnode, STN_children), ref->node);
         }
      } else
         ref->node = NULL;
   }

   /*-------------------------------------------------------------
    * locate department object and save off reference in job entry
    *-------------------------------------------------------------*/

   ref->dept = locate_department(lists->dept_list,
                                 lGetString(job, JB_department));
   if (ref->dept)
      lSetUlong(ref->dept, US_job_cnt, 0);
}


/*--------------------------------------------------------------------
 * sge_set_job_cnts - set job counts in the various object entries
 *--------------------------------------------------------------------*/

void sge_set_job_cnts(sge_ref_t *ref);

void
sge_set_job_cnts(
sge_ref_t *ref 
) {
   int i;
   if (ref->user)
      lSetUlong(ref->user, UP_job_cnt, lGetUlong(ref->user, UP_job_cnt)+1);
   if (ref->project)
      lSetUlong(ref->project, UP_job_cnt, lGetUlong(ref->project,UP_job_cnt)+1);
   if (ref->dept)
      lSetUlong(ref->dept, US_job_cnt, lGetUlong(ref->dept, US_job_cnt)+1);
   if (ref->task_jobclass)
      for(i=0; i<ref->num_task_jobclasses; i++) {
         if (ref->task_jobclass[i])
            lSetUlong(ref->task_jobclass[i], QU_job_cnt,
                      lGetUlong(ref->task_jobclass[i], QU_job_cnt)+1);
      }
   else
      if (ref->jobclass)
         lSetUlong(ref->jobclass, QU_job_cnt, lGetUlong(ref->jobclass,
               QU_job_cnt)+1);
   return;
}


/*--------------------------------------------------------------------
 * sge_unset_job_cnts - set job counts in the various object entries
 *--------------------------------------------------------------------*/

void sge_unset_job_cnts(sge_ref_t *ref);

void
sge_unset_job_cnts(
sge_ref_t *ref 
) {
   int i;
   if (ref->user)
      lSetUlong(ref->user, UP_job_cnt, lGetUlong(ref->user, UP_job_cnt)-1);
   if (ref->project)
      lSetUlong(ref->project, UP_job_cnt, lGetUlong(ref->project,UP_job_cnt)-1);
   if (ref->dept)
      lSetUlong(ref->dept, US_job_cnt, lGetUlong(ref->dept, US_job_cnt)-1);
   if (ref->task_jobclass)
      for(i=0; i<ref->num_task_jobclasses; i++) {
         if (ref->task_jobclass[i])
            lSetUlong(ref->task_jobclass[i], QU_job_cnt,
                      lGetUlong(ref->task_jobclass[i], QU_job_cnt)-1);
      }
   else
      if (ref->jobclass)
         lSetUlong(ref->jobclass, QU_job_cnt, lGetUlong(ref->jobclass,
               QU_job_cnt)-1);
   return;
}


/*--------------------------------------------------------------------
 * calculate_m_shares - calculate m_share for share tree node
 *      descendants
 *
 * Calling calculate_m_shares(root_node) will calculate shares for
 * every active node in the share tree.
 *
 * Rather than having to recalculate m_shares on every scheduling
 * interval, we calculate it whenever the scheduler comes up or
 * whenever the share tree itself changes and make adjustments
 * every time a job becomes active or inactive (in adjust_m_shares).
 *--------------------------------------------------------------------*/

void calculate_m_shares(lListElem *parent_node);

void
calculate_m_shares(
lListElem *parent_node 
) {
   u_long sum_of_child_shares = 0;
   lListElem *child_node;
   lList *children;
   double parent_m_share;

   DENTER(TOP_LAYER, "calculate_m_shares");

   children = lGetList(parent_node, STN_children);
   if (!children) {
      DEXIT;
      return;
   }

   /*-------------------------------------------------------------
    * Sum child shares
    *-------------------------------------------------------------*/

   for_each(child_node, children) {

      if (lGetUlong(child_node, STN_job_ref_count) > 0)
         sum_of_child_shares += lGetUlong(child_node, STN_shares);
   }

   /*-------------------------------------------------------------
    * Calculate m_shares for each child and descendants
    *-------------------------------------------------------------*/

   parent_m_share = lGetDouble(parent_node, STN_m_share);

   for_each(child_node, children) {
      if (lGetUlong(child_node, STN_job_ref_count) > 0)
         lSetDouble(child_node, STN_m_share,
                    parent_m_share *
                    (sum_of_child_shares ?
                    ((double)(lGetUlong(child_node, STN_shares)) /
                    (double)sum_of_child_shares) : 0));
      else
         lSetDouble(child_node, STN_m_share, 0);

      calculate_m_shares(child_node);
   }

   DEXIT;
   return;
}


#ifdef notdef

/*--------------------------------------------------------------------
 * adjust_m_shares - adjust m_share for all ancestor nodes with
 * reference count less than or equal to count parameter.
 *--------------------------------------------------------------------*/

void adjust_m_shares(lListElem *root, lListElem *job, u_long count);

void
adjust_m_shares(
lListElem *root,
lListElem *job,
u_long count 
) {
   lListElem *node;
   char *name;
   ancestors_t ancestors;
   int depth;

   if (!root)
      return;

   /*-------------------------------------------------------------
    * Look up share tree node
    *-------------------------------------------------------------*/

   name = lGetString(job, lGetUlong(root, STN_type) == STT_PROJECT ?
                          JB_project : JB_owner);

   if (!name)
      return;

   memset(&ancestors, 0, sizeof(ancestors_t));
   node = search_ancestor_list(root, name, &ancestors);
   depth = ancestors.depth;

   /*-------------------------------------------------------------
    * Search ancestor nodes until a previously active node is found
    *-------------------------------------------------------------*/

   while (depth-- && node && node != root &&
          lGetUlong(node, STN_job_ref_count)<=count) {
      node = ancestors.nodes[depth-1];
   }

   if (ancestors.nodes) free(ancestors.nodes);

   /*-------------------------------------------------------------
    * Calculate m_shares for every descendant of the active node
    *-------------------------------------------------------------*/

   if (node) calculate_m_shares(node);

}

/*

   To know if a node is active, we maintain a reference count in each
   node and do the following to keep it up-to-date.

   o call setup_share_tree when scheduler is initialized or
     if the share tree structure or shares are modified
   o call increment_job_ref_count when a new job is added (i.e. submitted)
   o call decrement_job_ref_count when a job is deleted (i.e. ended, aborted)

*/

/*--------------------------------------------------------------------
 * adjust_job_ref_count - adjusts all of the corresponding share
 * tree nodes' job reference counts
 *--------------------------------------------------------------------*/

void adjust_job_ref_count(lListElem *root, lListElem *job, long adjustment);

void
adjust_job_ref_count(
lListElem *root,
lListElem *job,
long adjustment 
) {
   lListElem *node=NULL;
   char *name;
   ancestors_t ancestors;
   int depth;

   /*-------------------------------------------------------------
    * Adjust job reference count for share tree node and
    *     ancestor nodes
    *-------------------------------------------------------------*/

   memset(&ancestors, 0, sizeof(ancestors_t));

   name = lGetString(job, lGetUlong(root, STN_type) == STT_PROJECT ?
                          JB_project : JB_owner);

   if (!name)
      return;

   memset(&ancestors, 0, sizeof(ancestors_t));
   node = search_ancestor_list(root, name, &ancestors);
   depth = ancestors.depth;

   while (depth--) {
      node = ancestors.nodes[depth];
      lSetUlong(node, STN_job_ref_count,
         lGetUlong(node, STN_job_ref_count)+adjustment);
   }

   if (ancestors.nodes) free(ancestors.nodes);

   return;
}


/*--------------------------------------------------------------------
 * increment_job_ref_count - increments all of the corresponding share
 * tree nodes' job reference counts and the job's share tree node
 * reference
 *--------------------------------------------------------------------*/

void increment_job_ref_count(lListElem *root, lListElem *job);

void
increment_job_ref_count(
lListElem *root,
lListElem *job 
) {
   adjust_job_ref_count(root, job, 1);
}


/*--------------------------------------------------------------------
 * decrement_job_ref_count - decrements all of the corresponding share
 * tree nodes' job reference counts and the job's share tree node
 * reference
 *--------------------------------------------------------------------*/

void decrement_job_ref_count(lListElem *root, lListElem *job);

void
decrement_job_ref_count(
lListElem *root,
lListElem *job 
) {
   adjust_job_ref_count(root, job, -1);
}


#endif /* notdef */


/*--------------------------------------------------------------------
 * update_job_ref_count - update job_ref_count for node and descendants
 *--------------------------------------------------------------------*/

u_long update_job_ref_count(lListElem *node);

u_long
update_job_ref_count(
lListElem *node 
) {
   int job_count=0;
   lList *children;
   lListElem *child;

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         job_count += update_job_ref_count(child);
      }
      lSetUlong(node, STN_job_ref_count, job_count);
   }

   return lGetUlong(node, STN_job_ref_count);
}


/*--------------------------------------------------------------------
 * zero_job_ref_count - zero job_ref_count for node and descendants
 *--------------------------------------------------------------------*/

void zero_job_ref_count(lListElem *node);

void
zero_job_ref_count(
lListElem *node 
) {
   lList *children;
   lListElem *child;

   lSetUlong(node, STN_job_ref_count, 0);
   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         zero_job_ref_count(child);
      }
   }
   return;
}


/*--------------------------------------------------------------------
 * set_share_tree_project_flags - set the share tree project flag for
 *       node and descendants
 *--------------------------------------------------------------------*/

void set_share_tree_project_flags(lList *project_list, lListElem *node);

void
set_share_tree_project_flags(
lList *project_list,
lListElem *node 
) {
   lList *children;
   lListElem *child;

   if (!project_list || !node)
      return;

   if (lGetElemStr(project_list, UP_name, lGetString(node, STN_name)))
      lSetUlong(node, STN_project, 1);
   else
      lSetUlong(node, STN_project, 0);

   children = lGetList(node, STN_children);
   if (children) {
      for_each(child, children) {
         set_share_tree_project_flags(project_list, child);
      }
   }
   return;
}


/*--------------------------------------------------------------------
 * get_usage - return usage entry based on name
 *--------------------------------------------------------------------*/

lListElem *get_usage(lList *usage_list, char *name);

lListElem *
get_usage(
lList *usage_list,
char *name 
) {
   return lGetElemStr(usage_list, UA_name, name);
}


/*--------------------------------------------------------------------
 * create_usage_elem - create a new usage element
 *--------------------------------------------------------------------*/

lListElem *create_usage_elem(char *name);

lListElem *
create_usage_elem(
char *name 
) {
   lListElem *usage;
    
   usage = lCreateElem(UA_Type);
   lSetString(usage, UA_name, name);
   lSetDouble(usage, UA_value, 0);

   return usage;
}


/*--------------------------------------------------------------------
 * build_usage_list - create a new usage list from an existing list
 *--------------------------------------------------------------------*/

lList *build_usage_list(char *name, lList *old_usage_list);

lList *
build_usage_list(
char *name,
lList *old_usage_list 
) {
   lList *usage_list = NULL;
   lListElem *usage;

   if (old_usage_list) {

      /*-------------------------------------------------------------
       * Copy the old list and zero out the usage values
       *-------------------------------------------------------------*/

      usage_list = lCopyList(name, old_usage_list);
      for_each(usage, usage_list)
         lSetDouble(usage, UA_value, 0);

   } else {

      /*
       * the UA_value fields are implicitly set to 0 at creation
       * time of a new element with lCreateElem or lAddElemStr
       */
        
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_CPU, UA_Type);
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_MEM, UA_Type);
      lAddElemStr(&usage_list, UA_name, USAGE_ATTR_IO, UA_Type);
   }

   return usage_list;
}




/*--------------------------------------------------------------------
 * delete_debited_job_usage - deleted debitted job usage for job
 *--------------------------------------------------------------------*/

void delete_debited_job_usage(sge_ref_t *ref, u_long seqno);

void
delete_debited_job_usage(
sge_ref_t *ref,
u_long seqno 
) {
   lListElem *job=ref->job,
             *user=ref->user,
             *project=ref->project;
   lList *upu_list;
   lListElem *upu;
   
   DENTER(TOP_LAYER, "delete_debited_job_usage");

   DPRINTF(("DDJU (1) "u32"\n", lGetUlong(job, JB_job_number)));

   if (user) {
      upu_list = lGetList(user, UP_debited_job_usage);
      DPRINTF(("DDJU (2) "u32"\n", lGetUlong(job, JB_job_number)));
      if (upu_list) {
         
         /* Note: In order to cause the qmaster to delete the
            usage for this job, we zero out UPU_old_usage_list
            for this job in the UP_debited_job_usage list */
         DPRINTF(("DDJU (3) "u32"\n", lGetUlong(job, JB_job_number))); 

         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                             lGetUlong(job, JB_job_number)))) {
            DPRINTF(("DDJU (4) "u32"\n", lGetUlong(job, JB_job_number)));
            lSetList(upu, UPU_old_usage_list, NULL);
            lSetUlong(user, UP_usage_seqno, seqno);
         }
      }
   }

   if (project) {
      upu_list = lGetList(project, UP_debited_job_usage);
      if (upu_list) {
         
         /* Note: In order to cause the qmaster to delete the
            usage for this job, we zero out UPU_old_usage_list
            for this job in the UP_debited_job_usage list */

         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                             lGetUlong(job, JB_job_number)))) {
            lSetList(upu, UPU_old_usage_list, NULL);
            lSetUlong(project, UP_usage_seqno, seqno);
         }
      }
   }

   DEXIT;
   return;
}


/*--------------------------------------------------------------------
 * combine_usage - combines a node's associated user/project usage 
 * into a single value and stores it in the node.
 *--------------------------------------------------------------------*/

void combine_usage(sge_ref_t *ref);

void
combine_usage(
sge_ref_t *ref 
) {
   double usage_value = 0;

   /*-------------------------------------------------------------
    * Get usage from associated user/project object
    *-------------------------------------------------------------*/

#if 0
   userprj = ref->share_tree_type == STT_PROJECT ? ref->project : ref->user;
#endif

   if (ref->node) {

      lList *usage_weight_list=NULL, *usage_list=NULL;
      lListElem *usage_weight, *config, *usage_elem;
      double sum_of_usage_weights = 0;
      char *usage_name;

      /*-------------------------------------------------------------
       * Sum usage weighting factors
       *-------------------------------------------------------------*/

      if ((config = lFirst(all_lists->config_list))) {
         usage_weight_list = lGetList(config, SC_usage_weight_list);
         if (usage_weight_list) {
            for_each(usage_weight, usage_weight_list)
               sum_of_usage_weights += lGetDouble(usage_weight, UA_value);
         }
      }

      /*-------------------------------------------------------------
       * Combine user/project usage based on usage weighting factors
       *-------------------------------------------------------------*/

      if (usage_weight_list) {

         if (ref->user) {
            if (ref->project) {
               lList *upp_list = lGetList(ref->user, UP_project);
               lListElem *upp;
               if (upp_list &&
                   ((upp = lGetElemStr(upp_list, UPP_name,
                                       lGetString(ref->project, UP_name)))))
                  usage_list = lGetList(upp, UPP_usage);
            } else
               usage_list = lGetList(ref->user, UP_usage);
         } else if (ref->project) /* not sure about this, use it when? */
            usage_list = lGetList(ref->project, UP_usage);

         for_each(usage_elem, usage_list) {
            usage_name = lGetString(usage_elem, UA_name);
            usage_weight = lGetElemStr(usage_weight_list, UA_name,
                                       usage_name);
            if (usage_weight && sum_of_usage_weights>0) {
               usage_value += lGetDouble(usage_elem, UA_value) *
                  (lGetDouble(usage_weight, UA_value) /
                   sum_of_usage_weights);
            }
         }
      }

      /*-------------------------------------------------------------
       * Set combined usage in the node
       *-------------------------------------------------------------*/

      lSetDouble(ref->node, STN_combined_usage, usage_value);

   }

   return;
}


/*--------------------------------------------------------------------
 * decay_and_sum_usage - accumulates and decays usage in the correct
 * user and project objects for the specified job
 *--------------------------------------------------------------------*/

void decay_and_sum_usage(sge_ref_t *ref, u_long seqno, u_long curr_time);

void
decay_and_sum_usage(
sge_ref_t *ref,
u_long seqno,
u_long curr_time 
) {
   lList *job_usage_list=NULL,
         *old_usage_list=NULL,
         *user_usage_list=NULL,
         *project_usage_list=NULL,
         *user_long_term_usage_list=NULL,
         *project_long_term_usage_list=NULL;
   lListElem *job=ref->job,
             *ja_task=ref->ja_task,
             *node=ref->node,
             *user=ref->user,
             *project=ref->project,
             *userprj = NULL,
             *task;

   if (!node && !user && !project)
      return;

#if 0
   if (ref->share_tree_type == STT_PROJECT)
      userprj = ref->project;
   else
      userprj = ref->user;
#endif

   if (ref->user)
      userprj = ref->user;
   else if (ref->project)
      userprj = ref->project;

   /*-------------------------------------------------------------
    * Decay the usage for the associated user and project
    *-------------------------------------------------------------*/
    
   if (user)
      decay_userprj_usage(user, seqno, curr_time);

   if (project)
      decay_userprj_usage(project, seqno, curr_time);

   /*-------------------------------------------------------------
    * Note: Since SGE will update job.usage directly, we 
    * maintain the job usage the last time we collected it from
    * the job.  The difference between the new usage and the old
    * usage is what needs to be added to the user or project node.
    * This old usage is maintained in the user or project node
    * depending on the type of share tree.
    *-------------------------------------------------------------*/

   job_usage_list = lCopyList(NULL, lGetList(ja_task, JAT_scaled_usage_list));

   /* sum sub-task usage into job_usage_list */
   if (job_usage_list) {
      for_each(task, lGetList(ja_task, JAT_task_list)) {
         lListElem *dst, *src;
         for_each(src, lGetList(ja_task, JAT_scaled_usage_list)) {
            if ((dst=lGetElemStr(job_usage_list, UA_name, lGetString(src, UA_name))))
               lSetDouble(dst, UA_value, lGetDouble(dst, UA_value) + lGetDouble(src, UA_value));
            else
               lAppendElem(job_usage_list, lCopyElem(src));
         }
      }
   }

   if (userprj) {
      lListElem *upu;
      lList *upu_list = lGetList(userprj, UP_debited_job_usage);
      if (upu_list) {
         if ((upu = lGetElemUlong(upu_list, UPU_job_number,
                                  lGetUlong(job, JB_job_number)))) {
            if ((old_usage_list = lGetList(upu, UPU_old_usage_list))) {
               old_usage_list = lCopyList("", old_usage_list);
            }
         }
      }
   }

   if (!old_usage_list)
      old_usage_list = build_usage_list("old_usage_list", NULL);

   if (user) {

      /* if there is a user & project, usage is kept in the project sub-list */

      if (project) {
         lList *upp_list = lGetList(user, UP_project);
         lListElem *upp;
         char *project_name = lGetString(project, UP_name);

         if (!upp_list) {
            upp_list = lCreateList("", UPP_Type);
            lSetList(user, UP_project, upp_list);
         }
         if (!((upp = lGetElemStr(upp_list, UPP_name, project_name))))
            upp = lAddElemStr(&upp_list, UPP_name, project_name, UPP_Type);
         user_long_term_usage_list = lGetList(upp, UPP_long_term_usage);
         if (!user_long_term_usage_list) {
            user_long_term_usage_list = 
                  build_usage_list("upp_long_term_usage_list", NULL);
            lSetList(upp, UPP_long_term_usage, user_long_term_usage_list);
         }
         user_usage_list = lGetList(upp, UPP_usage);
         if (!user_usage_list) {
            user_usage_list = build_usage_list("upp_usage_list", NULL);
            lSetList(upp, UPP_usage, user_usage_list);
         }

      } else {
         user_long_term_usage_list = lGetList(user, UP_long_term_usage);
         if (!user_long_term_usage_list) {
            user_long_term_usage_list = 
                  build_usage_list("user_long_term_usage_list", NULL);
            lSetList(user, UP_long_term_usage, user_long_term_usage_list);
         }
         user_usage_list = lGetList(user, UP_usage);
         if (!user_usage_list) {
            user_usage_list = build_usage_list("user_usage_list", NULL);
            lSetList(user, UP_usage, user_usage_list);
         }
      }
   }

   if (project) {
      project_long_term_usage_list = lGetList(project, UP_long_term_usage);
      if (!project_long_term_usage_list) {
         project_long_term_usage_list =
              build_usage_list("project_long_term_usage_list", NULL);
         lSetList(project, UP_long_term_usage, project_long_term_usage_list);
      }
      project_usage_list = lGetList(project, UP_usage);
      if (!project_usage_list) {
         project_usage_list = build_usage_list("project_usage_list", 
                                                NULL);
         lSetList(project, UP_usage, project_usage_list);
      }
   }

   if (job_usage_list) {

      lListElem *job_usage;

      /*-------------------------------------------------------------
       * Add to node usage for each usage type
       *-------------------------------------------------------------*/

      for_each(job_usage, job_usage_list) {

         lListElem *old_usage=NULL, 
                   *user_usage=NULL, *project_usage=NULL,
                   *user_long_term_usage=NULL,
                   *project_long_term_usage=NULL;
         char *usage_name = lGetString(job_usage, UA_name);

         /* only copy CPU, memory, and I/O usage */
         if (strcmp(usage_name, USAGE_ATTR_CPU) != 0 &&
             strcmp(usage_name, USAGE_ATTR_MEM) != 0 &&
             strcmp(usage_name, USAGE_ATTR_IO) != 0)
             continue;

         /*---------------------------------------------------------
          * Locate the corresponding usage element for the job
          * usage type in the node usage, old job usage, user usage,
          * and project usage.  If it does not exist, create a new
          * corresponding usage element.
          *---------------------------------------------------------*/

         if (old_usage_list) {
            old_usage = get_usage(old_usage_list, usage_name);
            if (!old_usage) {
               old_usage = create_usage_elem(usage_name);
               lAppendElem(old_usage_list, old_usage);
            }
         }

         if (user_usage_list) {
            user_usage = get_usage(user_usage_list, usage_name);
            if (!user_usage) {
               user_usage = create_usage_elem(usage_name);
               lAppendElem(user_usage_list, user_usage);
            }
         }

         if (user_long_term_usage_list) {
            user_long_term_usage = get_usage(user_long_term_usage_list,
                                                       usage_name);
            if (!user_long_term_usage) {
               user_long_term_usage = create_usage_elem(usage_name);
               lAppendElem(user_long_term_usage_list, user_long_term_usage);
            }
         }

         if (project_usage_list) {
            project_usage = get_usage(project_usage_list, usage_name);
            if (!project_usage) {
               project_usage = create_usage_elem(usage_name);
               lAppendElem(project_usage_list, project_usage);
            }
         }

         if (project_long_term_usage_list) {
            project_long_term_usage =
                  get_usage(project_long_term_usage_list, usage_name);
            if (!project_long_term_usage) {
               project_long_term_usage = create_usage_elem(usage_name);
               lAppendElem(project_long_term_usage_list,
			   project_long_term_usage);
            }
         }

         if (job_usage && old_usage) {

            double usage_value;

            usage_value = MAX(lGetDouble(job_usage, UA_value) -
                              lGetDouble(old_usage, UA_value), 0);
                                     
            /*---------------------------------------------------
             * Add usage to decayed user usage
             *---------------------------------------------------*/

            if (user_usage)
                lSetDouble(user_usage, UA_value,
                      lGetDouble(user_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to long term user usage
             *---------------------------------------------------*/

            if (user_long_term_usage)
                lSetDouble(user_long_term_usage, UA_value,
                      lGetDouble(user_long_term_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to decayed project usage
             *---------------------------------------------------*/

            if (project_usage)
                lSetDouble(project_usage, UA_value,
                      lGetDouble(project_usage, UA_value) +
                      usage_value);

            /*---------------------------------------------------
             * Add usage to long term project usage
             *---------------------------------------------------*/

            if (project_long_term_usage)
                lSetDouble(project_long_term_usage, UA_value,
                      lGetDouble(project_long_term_usage, UA_value) +
                      usage_value);


         }

      }

   }

   /*-------------------------------------------------------------
    * save off current job usage in debitted job usage list
    *-------------------------------------------------------------*/

   if (old_usage_list)
      lFreeList(old_usage_list);

   if (job_usage_list) {
      if (userprj) {
         lListElem *upu;
         u_long jobnum = lGetUlong(job, JB_job_number);
         lList *upu_list = lGetList(userprj, UP_debited_job_usage);
         if (!upu_list) {
            upu_list = lCreateList("", UPU_Type);
            lSetList(userprj, UP_debited_job_usage, upu_list);
         }
         if ((upu = lGetElemUlong(upu_list, UPU_job_number, jobnum))) {
            lSetList(upu, UPU_old_usage_list,
                     lCopyList(lGetListName(job_usage_list), job_usage_list));
         } else {
            upu = lCreateElem(UPU_Type);
            lSetUlong(upu, UPU_job_number, jobnum);
            lSetList(upu, UPU_old_usage_list,
                     lCopyList(lGetListName(job_usage_list), job_usage_list));
            lAppendElem(upu_list, upu);
         }
      }
      lFreeList(job_usage_list);
   }

}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass0 - performs pass 0 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void calc_job_share_tree_tickets_pass0(sge_ref_t *ref, double *sum_m_share, double *sum_proportion, u_long seqno);

void
calc_job_share_tree_tickets_pass0(
sge_ref_t *ref,
double *sum_m_share,
double *sum_proportion,
u_long seqno 
) {
   lListElem *node = ref->node;
   double node_m_share, node_proportion, node_usage=0;

   /*-------------------------------------------------------------
    * Note: The seqno is a global or parameter that is incremented
    * on each sge scheduling interval. It is checked against
    * node.pass0_seqno so that the user or project node
    * calculations are only done once per node.
    *-------------------------------------------------------------*/

   if (node && seqno != lGetUlong(node, STN_pass0_seqno)) {

       node_m_share = lGetDouble(node, STN_m_share);
       node_usage = lGetDouble(node, STN_combined_usage);
       if (node_usage < SGE_MIN_USAGE)
           node_usage = SGE_MIN_USAGE;
       node_proportion = node_m_share * node_m_share / node_usage;
       lSetDouble(node, STN_proportion, node_proportion);
       *sum_proportion += node_proportion;
       *sum_m_share += node_m_share;
       lSetUlong(node, STN_pass0_seqno, seqno);

   }
}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass1 - performs pass 1 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void calc_job_share_tree_tickets_pass1(sge_ref_t *ref, double sum_m_share, double sum_proportion, double *sum_adjusted_proportion, u_long seqno);

void
calc_job_share_tree_tickets_pass1(
sge_ref_t *ref,
double sum_m_share,
double sum_proportion,
double *sum_adjusted_proportion,
u_long seqno 
) {
   lListElem *job = ref->job;
   lListElem *node = ref->node;
   double target_proportion=0, current_proportion=0,
          adjusted_usage, m_share, adjusted_proportion=0,
          compensation_factor;

   if (!node) {
      return;
   }

   /*-------------------------------------------------------
    * calculate targetted proportion of node
    *-------------------------------------------------------*/

   m_share = lGetDouble(node, STN_m_share);
   if (sum_m_share)
      target_proportion = m_share / sum_m_share;
   lSetDouble(node, STN_target_proportion, target_proportion);

   /*-------------------------------------------------------
    * calculate current proportion of node
    *-------------------------------------------------------*/

   if (sum_proportion)
      current_proportion = lGetDouble(node, STN_proportion) / sum_proportion;
   lSetDouble(node, STN_current_proportion, current_proportion);

   /*-------------------------------------------------------
    * adjust proportion based on compensation factor
    *-------------------------------------------------------*/

   compensation_factor = lGetDouble(lFirst(all_lists->config_list),
                  SC_compensation_factor);
   if (target_proportion > 0 && compensation_factor > 0 &&
      current_proportion > (compensation_factor * target_proportion))

      adjusted_usage = MAX(lGetDouble(node, STN_combined_usage),
                           SGE_MIN_USAGE) *
             (current_proportion /
             (compensation_factor * target_proportion));
   else
      adjusted_usage = lGetDouble(node, STN_combined_usage);

   lSetDouble(node, STN_adjusted_usage, adjusted_usage);

   if (adjusted_usage < SGE_MIN_USAGE)
      adjusted_usage = SGE_MIN_USAGE;

   if (seqno != lGetUlong(node, STN_pass1_seqno)) {
      lSetUlong(node, STN_sum_priority, 0);
      if (adjusted_usage > 0)
         adjusted_proportion = m_share * m_share / adjusted_usage;
      lSetDouble(node, STN_adjusted_proportion, adjusted_proportion);
      *sum_adjusted_proportion += adjusted_proportion;
      lSetUlong(node, STN_pass1_seqno, seqno);
   }

   /*-------------------------------------------------------
    * sum POSIX priorities for each job for use in pass 2
    *-------------------------------------------------------*/

   lSetUlong(node, STN_sum_priority,
             lGetUlong(node, STN_sum_priority) +
             lGetUlong(job, JB_priority));
}


/*--------------------------------------------------------------------
 * calc_job_share_tree_tickets_pass2 - performs pass 2 of calculating
 *      the job share tree tickets for the specified job
 *--------------------------------------------------------------------*/

void
calc_job_share_tree_tickets_pass2(sge_ref_t *ref, double sum_adjusted_proportion, u_long total_share_tree_tickets, u_long seqno);

void
calc_job_share_tree_tickets_pass2(
sge_ref_t *ref,
double sum_adjusted_proportion,
u_long total_share_tree_tickets,
u_long seqno 
) {
   u_long share_tree_tickets;
   lListElem *job = ref->job;
   lListElem *ja_task = ref->ja_task;
   lListElem *node = ref->node;

   if (!node) {
      return;
   }

   /*-------------------------------------------------------
    * calculate the number of share tree tickets for this node
    *-------------------------------------------------------*/

   if (seqno != lGetUlong(node, STN_pass2_seqno)) {

       double adjusted_current_proportion=0;

       if (sum_adjusted_proportion>0)
          adjusted_current_proportion =
                  lGetDouble(node, STN_adjusted_proportion) /
                  sum_adjusted_proportion;
       lSetDouble(node, STN_adjusted_current_proportion,
                  adjusted_current_proportion);
       lSetUlong(node, STN_pass2_seqno, seqno);
   }

   /*-------------------------------------------------------
    * calculate the number of share tree tickets for this job
    *-------------------------------------------------------*/

   share_tree_tickets = lGetDouble(node, STN_adjusted_current_proportion) *
                        total_share_tree_tickets;

   if (lGetUlong(node, STN_sum_priority))
      lSetUlong(ja_task, JAT_sticket,
                (u_long)((double)lGetUlong(job, JB_priority) *
                (double)share_tree_tickets /
                lGetUlong(node, STN_sum_priority)));
   else
      lSetUlong(ja_task, JAT_sticket,
                share_tree_tickets / lGetUlong(node, STN_job_ref_count));
}


/*--------------------------------------------------------------------
 * calc_functional_tickets_pass1 - performs pass 1 of calculating
 *      the functional tickets for the specified job
 *--------------------------------------------------------------------*/

void calc_job_functional_tickets_pass1(sge_ref_t *ref, u_long *sum_of_user_functional_shares, u_long *sum_of_project_functional_shares, u_long *sum_of_department_functional_shares, u_long *sum_of_jobclass_functional_shares, u_long *sum_of_job_functional_shares);

void
calc_job_functional_tickets_pass1(
sge_ref_t *ref,
u_long *sum_of_user_functional_shares,
u_long *sum_of_project_functional_shares,
u_long *sum_of_department_functional_shares,
u_long *sum_of_jobclass_functional_shares,
u_long *sum_of_job_functional_shares 
) {
   /*-------------------------------------------------------------
    * Sum user functional shares
    *-------------------------------------------------------------*/

   if (ref->user)
       *sum_of_user_functional_shares += lGetUlong(ref->user, UP_fshare);

   /*-------------------------------------------------------------
    * Sum project functional shares
    *-------------------------------------------------------------*/

   if (ref->project)
       *sum_of_project_functional_shares += lGetUlong(ref->project, UP_fshare);

   /*-------------------------------------------------------------
    * Sum department functional shares
    *-------------------------------------------------------------*/

   if (ref->dept)
       *sum_of_department_functional_shares += lGetUlong(ref->dept,
                                                         US_fshare);

   /*-------------------------------------------------------------
    * Sum job class functional shares
    *-------------------------------------------------------------*/

   if (ref->task_jobclass) {
      int i;
      for(i=0; i<ref->num_task_jobclasses; i++)
         if (ref->task_jobclass[i])
            *sum_of_jobclass_functional_shares +=
                  lGetUlong(ref->task_jobclass[i], QU_fshare);
   } else if (ref->jobclass)
       *sum_of_jobclass_functional_shares += lGetUlong(ref->jobclass,
                                                       QU_fshare);

   /*-------------------------------------------------------------
    * Sum job functional shares
    *-------------------------------------------------------------*/

   lSetUlong(ref->ja_task, JAT_fshare, lGetUlong(ref->job, JB_priority));

   *sum_of_job_functional_shares += lGetUlong(ref->ja_task, JAT_fshare);
}


/*--------------------------------------------------------------------
 * calc_functional_tickets_pass2 - performs pass 2 of calculating
 *      the functional tickets for the specified job
 *--------------------------------------------------------------------*/

u_long
calc_job_functional_tickets_pass2(sge_ref_t *ref, u_long sum_of_user_functional_shares, u_long sum_of_project_functional_shares, u_long sum_of_department_functional_shares, u_long sum_of_jobclass_functional_shares, u_long sum_of_job_functional_shares, u_long total_functional_tickets);

u_long
calc_job_functional_tickets_pass2(
sge_ref_t *ref,
u_long sum_of_user_functional_shares,
u_long sum_of_project_functional_shares,
u_long sum_of_department_functional_shares,
u_long sum_of_jobclass_functional_shares,
u_long sum_of_job_functional_shares,
u_long total_functional_tickets 
) {
   u_long user_functional_tickets=0,
          project_functional_tickets=0,
          department_functional_tickets=0,
          jobclass_functional_tickets=0,
          job_functional_tickets=0,
          total_job_functional_tickets;
   double k_sum=0, k_user=0, k_department=0, k_project=0,
          k_jobclass=0, k_job=0;

   /*-------------------------------------------------------
    * get functional weighting parameters
    *-------------------------------------------------------*/

   lListElem *config = lFirst(all_lists->config_list);
   if (config) {
      if (sum_of_user_functional_shares > 0)
         k_user = lGetDouble(config, SC_weight_user);
      if (sum_of_department_functional_shares > 0)
         k_department = lGetDouble(config, SC_weight_department);
      if (sum_of_project_functional_shares > 0)
         k_project = lGetDouble(config, SC_weight_project);
      if (sum_of_jobclass_functional_shares > 0)
         k_jobclass = lGetDouble(config, SC_weight_jobclass);
      if (sum_of_job_functional_shares > 0)
         k_job = lGetDouble(config, SC_weight_job);
      k_sum = k_user + k_department + k_project + k_jobclass + k_job;
      if (k_sum>0) {
         k_user /= k_sum;
         k_department /= k_sum;
         k_project /= k_sum;
         k_jobclass /= k_sum;
         k_job /= k_sum;
      }
   }

   /*-------------------------------------------------------
    * calculate user functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->user && sum_of_user_functional_shares)
      user_functional_tickets = (lGetUlong(ref->user, UP_fshare) *
                                total_functional_tickets /
                                sum_of_user_functional_shares);

   /*-------------------------------------------------------
    * calculate project functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->project && sum_of_project_functional_shares)
      project_functional_tickets = (lGetUlong(ref->project, UP_fshare) *
                                 total_functional_tickets /
                                 sum_of_project_functional_shares);

   /*-------------------------------------------------------
    * calculate department functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->dept && sum_of_department_functional_shares)
      department_functional_tickets = (lGetUlong(ref->dept, US_fshare) *
                                 total_functional_tickets /
                                 sum_of_department_functional_shares);

   /*-------------------------------------------------------
    * calculate job class functional tickets for this job
    *-------------------------------------------------------*/

   if (ref->task_jobclass && sum_of_jobclass_functional_shares) {
      int i;
      u_long qshares=0;
      for(i=0; i<ref->num_task_jobclasses; i++)
         if (ref->task_jobclass[i])
            qshares += lGetUlong(ref->task_jobclass[i], QU_fshare);

      jobclass_functional_tickets = (qshares *
                                 total_functional_tickets /
                                 sum_of_jobclass_functional_shares);

   } else if (ref->jobclass && sum_of_jobclass_functional_shares)
      jobclass_functional_tickets = (lGetUlong(ref->jobclass, QU_fshare) *
                                 total_functional_tickets /
                                 sum_of_jobclass_functional_shares);

   /*-------------------------------------------------------
    * calculate job functional tickets for this job
    *-------------------------------------------------------*/

   if (sum_of_job_functional_shares)
      job_functional_tickets = ((double)lGetUlong(ref->job, JB_priority) *
                                 (double)total_functional_tickets /
                                  sum_of_job_functional_shares);

   /*-------------------------------------------------------
    * calculate functional tickets for PE tasks
    *-------------------------------------------------------*/

   if (ref->task_jobclass) {
      lListElem *granted_pe;
      int i=0;
      double task_jobclass_functional_tickets;
#if 0
      double total_task_functional_tickets;

      total_task_functional_tickets = (k_user * user_functional_tickets +
                 k_department * department_functional_tickets +
                 k_project * project_functional_tickets +
                 k_job * job_functional_tickets) / ref->num_active_task_jobclasses;
#endif

      for_each(granted_pe, lGetList(ref->ja_task, JAT_granted_destin_identifier_list)) {
         if (ref->task_jobclass[i]) {
            if (sum_of_jobclass_functional_shares)
               task_jobclass_functional_tickets = (lGetUlong(ref->task_jobclass[i], QU_fshare) *
                                        total_functional_tickets /
                                        (double)sum_of_jobclass_functional_shares);
            else
               task_jobclass_functional_tickets = 0;
#if 0
            lSetUlong(granted_pe, JG_fticket,
                        total_task_functional_tickets + k_jobclass * task_jobclass_functional_tickets);
#endif
            lSetUlong(granted_pe, JG_jcfticket, k_jobclass * task_jobclass_functional_tickets);
         } else
            lSetUlong(granted_pe, JG_jcfticket, 0);
         i++;
      }
   }

   /*-------------------------------------------------------
    * calculate functional tickets for this job
    *-------------------------------------------------------*/

   total_job_functional_tickets = k_user * user_functional_tickets +
             k_department * department_functional_tickets +
             k_project * project_functional_tickets +
             k_jobclass * jobclass_functional_tickets +
             k_job * job_functional_tickets;

   ref->total_jobclass_ftickets = k_jobclass * jobclass_functional_tickets;

   lSetUlong(ref->ja_task, JAT_fticket, total_job_functional_tickets);

   return job_functional_tickets;
}


/*--------------------------------------------------------------------
 * calc_deadline_tickets_pass1 - performs pass 1 of calculating
 *      the deadline tickets for the specified job
 *--------------------------------------------------------------------*/

u_long
calc_job_deadline_tickets_pass1(sge_ref_t *ref, u_long total_deadline_tickets, u_long current_time);

u_long
calc_job_deadline_tickets_pass1(
sge_ref_t *ref,
u_long total_deadline_tickets,
u_long current_time 
) {
   lListElem *job = ref->job;
   lListElem *ja_task = ref->ja_task;
   u_long job_deadline_tickets;
   u_long job_start_time,
          job_deadline;        /* deadline initiation time */

   /*-------------------------------------------------------
    * If job has not started, set deadline tickets to zero
    *-------------------------------------------------------*/

   if ((job_start_time = lGetUlong(job, JB_execution_time)) == 0)
       job_start_time = lGetUlong(job, JB_submission_time);
   job_deadline = lGetUlong(job, JB_deadline);

   if (job_deadline == 0 ||
       job_start_time == 0 ||
       current_time <= job_start_time)

      job_deadline_tickets = 0;

   /*-------------------------------------------------------
    * If job has started and deadline is in future, set
    * deadline tickets based on time left till deadline.
    *-------------------------------------------------------*/

   else if (current_time < job_deadline)

      job_deadline_tickets = (double)(current_time - job_start_time) * 
                             (double)total_deadline_tickets /
                             (job_deadline - job_start_time);

   /*-------------------------------------------------------
    * If deadline is in past, set deadline tickets to
    * maximum available deadline tickets.
    *-------------------------------------------------------*/

   else
      job_deadline_tickets = total_deadline_tickets;


   /*-------------------------------------------------------
    * Set the number of deadline tickets in the job
    *-------------------------------------------------------*/

   lSetUlong(ja_task, JAT_dticket, job_deadline_tickets);

   return job_deadline_tickets;
}


/*--------------------------------------------------------------------
 * calc_deadline_tickets_pass2 - performs pass 2 of calculating
 *      the deadline tickets for the specified job.  Only called
 *      if sum_of_deadline_tickets for pass 1 is greater than
 *      total_deadline_tickets.
 *--------------------------------------------------------------------*/

u_long calc_job_deadline_tickets_pass2(sge_ref_t *ref, u_long sum_of_deadline_tickets, u_long total_deadline_tickets);

u_long
calc_job_deadline_tickets_pass2(
sge_ref_t *ref,
u_long sum_of_deadline_tickets,
u_long total_deadline_tickets 
) {
   u_long job_deadline_tickets;

   job_deadline_tickets = lGetUlong(ref->ja_task, JAT_dticket);

   /*-------------------------------------------------------------
    * Scale deadline tickets based on total number of deadline
    * tickets.
    *-------------------------------------------------------------*/

   if (job_deadline_tickets > 0) {
      job_deadline_tickets = job_deadline_tickets *
                             ((double)total_deadline_tickets /
                             (double)sum_of_deadline_tickets);
      lSetUlong(ref->ja_task, JAT_dticket, job_deadline_tickets);
   }

   return job_deadline_tickets;
}


/*--------------------------------------------------------------------
 * calc_override_tickets - calculates the number of override tickets for the
 * specified job
 *--------------------------------------------------------------------*/

u_long calc_job_override_tickets(sge_ref_t *ref);

u_long
calc_job_override_tickets(
sge_ref_t *ref 
) {
   u_long job_override_tickets = 0;
   u_long otickets, job_cnt;

   DENTER(TOP_LAYER, "calc_job_override_tickets");

   /*-------------------------------------------------------
    * job.override_tickets = user.override_tickets +
    *                        project.override_tickets +
    *                        department.override_tickets +
    *                        jobclass.override_tickets +
    *                        job.override_tickets;
    *-------------------------------------------------------*/

   if (ref->user)
      if (((otickets = lGetUlong(ref->user, UP_oticket)) &&
          ((job_cnt = lGetUlong(ref->user, UP_job_cnt)))))
         job_override_tickets += (otickets / job_cnt);

   if (ref->project)
      if (((otickets = lGetUlong(ref->project, UP_oticket)) &&
          ((job_cnt = lGetUlong(ref->project, UP_job_cnt)))))
         job_override_tickets += (otickets / job_cnt);

   if (ref->dept)
      if (((otickets = lGetUlong(ref->dept, US_oticket)) &&
          ((job_cnt = lGetUlong(ref->dept, US_job_cnt)))))
         job_override_tickets += (otickets / job_cnt);

   job_override_tickets += lGetUlong(ref->job, JB_override_tickets);

   if (ref->task_jobclass) {
      lListElem *granted_pe;
      double jobclass_otickets;
      int i=0;
      ref->total_jobclass_otickets=0;
      for_each(granted_pe, lGetList(ref->ja_task, JAT_granted_destin_identifier_list)) {
         jobclass_otickets = 0;
         if (ref->task_jobclass[i]) {
            if (((otickets = lGetUlong(ref->task_jobclass[i], QU_oticket)) &&
                ((job_cnt = lGetUlong(ref->task_jobclass[i], QU_job_cnt)))))
               jobclass_otickets = (otickets / (double)job_cnt);
#if 0
            lSetUlong(granted_pe, JG_oticket,
                  (job_override_tickets/(double)ref->num_active_task_jobclasses) +
                  jobclass_otickets);
#endif
            ref->total_jobclass_otickets += jobclass_otickets;
         }

         lSetUlong(granted_pe, JG_jcoticket, jobclass_otickets);
         i++;
      }
      job_override_tickets += ref->total_jobclass_otickets;
   } else if (ref->jobclass)
      if (((otickets = lGetUlong(ref->jobclass, QU_oticket)) &&
          ((job_cnt = lGetUlong(ref->jobclass, QU_job_cnt)))))
         job_override_tickets += (otickets / job_cnt);

   lSetUlong(ref->ja_task, JAT_oticket, job_override_tickets);

   DEXIT;
   return job_override_tickets;
}


/*--------------------------------------------------------------------
 * calc_job_tickets - calculates the total number of tickets for
 * the specified job
 *--------------------------------------------------------------------*/

u_long calc_job_tickets(sge_ref_t *ref);

u_long
calc_job_tickets(
sge_ref_t *ref 
) {
   lList *granted;
   lListElem *job = ref->job,
             *ja_task = ref->ja_task;
   u_long job_tickets;
   lListElem *pe, *granted_el;
   char *pe_str;

   /*-------------------------------------------------------------
    * Sum up all tickets for job
    *-------------------------------------------------------------*/

   job_tickets = lGetUlong(ja_task, JAT_sticket) +
                 lGetUlong(ja_task, JAT_fticket) +
                 lGetUlong(ja_task, JAT_dticket) +
                 lGetUlong(ja_task, JAT_oticket);

   lSetUlong(ja_task, JAT_ticket, job_tickets);

   /* for PE slave-controlled jobs, set tickets for each granted queue */

   if (     (pe_str=lGetString(ja_task, JAT_granted_pe))
         && (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str))
         && lGetUlong(pe, PE_control_slaves)
         && (granted=lGetList(ja_task, JAT_granted_destin_identifier_list))) {

      double job_tickets_per_slot, job_dtickets_per_slot, job_otickets_per_slot,
             job_ftickets_per_slot, job_stickets_per_slot, nslots, active_nslots;

      /* Here we get the total number of slots granted on all queues (nslots) and the
         total number of slots granted on all queues with active subtasks or
         on which no subtasks have started yet (active_nslots). */

      nslots = nslots_granted(granted, NULL);
      active_nslots = active_nslots_granted(job, granted, NULL);

      /* If there are some active_nslots, then calculate the number of tickets
         per slot based on the active_nslots */

      if (active_nslots > 0)
         nslots = active_nslots;

      if (nslots > 0) {
         job_dtickets_per_slot = (double)lGetUlong(ja_task, JAT_dticket)/nslots;
         job_stickets_per_slot = (double)lGetUlong(ja_task, JAT_sticket)/nslots;
      } else {
         job_dtickets_per_slot = 0;
         job_stickets_per_slot = 0;
      }

      for_each(granted_el, granted) {
         double slots;

         /* Only give tickets to queues with active sub-tasks or sub-tasks
            which have not started yet. */
            
         if (active_nslots > 0 && !active_subtasks(job, lGetString(granted_el, JG_qname)))
            slots = 0;
         else
            slots = lGetUlong(granted_el, JG_slots);
         
         if (nslots > 0) {
            job_ftickets_per_slot = (double)(lGetUlong(ja_task, JAT_fticket) - ref->total_jobclass_ftickets)/nslots;
            job_otickets_per_slot = (double)(lGetUlong(ja_task, JAT_oticket) - ref->total_jobclass_otickets)/nslots;
            job_tickets_per_slot = job_dtickets_per_slot + job_stickets_per_slot + job_ftickets_per_slot + job_otickets_per_slot;
         } else {
            job_ftickets_per_slot = 0;
            job_otickets_per_slot = 0;
            job_tickets_per_slot = 0;
         }

         lSetUlong(granted_el, JG_fticket, job_ftickets_per_slot*slots + lGetUlong(granted_el, JG_jcfticket));
         lSetUlong(granted_el, JG_oticket, job_otickets_per_slot*slots + lGetUlong(granted_el, JG_jcoticket));
         lSetUlong(granted_el, JG_dticket, job_dtickets_per_slot*slots);
         lSetUlong(granted_el, JG_sticket, job_stickets_per_slot*slots);
         lSetUlong(granted_el, JG_ticket, job_tickets_per_slot*slots +
                   lGetUlong(granted_el, JG_jcoticket) + lGetUlong(granted_el, JG_jcfticket));

      }
   }

   return job_tickets;
}


/*--------------------------------------------------------------------
 * sge_clear_job - clear tickets for job
 *--------------------------------------------------------------------*/

void
sge_clear_job(
lListElem *job 
) {
   lListElem *granted_el;
   lListElem *ja_task;

   for_each(ja_task, lGetList(job, JB_ja_tasks)) {
      lSetUlong(ja_task, JAT_ticket, 0);
      lSetUlong(ja_task, JAT_oticket, 0);
      lSetUlong(ja_task, JAT_dticket, 0);
      lSetUlong(ja_task, JAT_fticket, 0);
      lSetUlong(ja_task, JAT_sticket, 0);
      lSetDouble(ja_task, JAT_share, 0);
      for_each(granted_el, lGetList(ja_task, JAT_granted_destin_identifier_list)) {
         lSetUlong(granted_el, JG_ticket, 0);
         lSetUlong(granted_el, JG_oticket, 0);
         lSetUlong(granted_el, JG_fticket, 0);
         lSetUlong(granted_el, JG_dticket, 0);
         lSetUlong(granted_el, JG_sticket, 0);
         lSetUlong(granted_el, JG_jcoticket, 0);
         lSetUlong(granted_el, JG_jcfticket, 0);
      }
   }
}

/*--------------------------------------------------------------------
 * sge_clear_job - clear tickets for job
 *--------------------------------------------------------------------*/

void
sge_clear_ja_task(
lListElem *ja_task 
) {
   lListElem *granted_el;

   lSetUlong(ja_task, JAT_ticket, 0);
   lSetUlong(ja_task, JAT_oticket, 0);
   lSetUlong(ja_task, JAT_dticket, 0);
   lSetUlong(ja_task, JAT_fticket, 0);
   lSetUlong(ja_task, JAT_sticket, 0);
   lSetDouble(ja_task, JAT_share, 0);
   for_each(granted_el, lGetList(ja_task, JAT_granted_destin_identifier_list)) {
      lSetUlong(granted_el, JG_ticket, 0);
      lSetUlong(granted_el, JG_oticket, 0);
      lSetUlong(granted_el, JG_fticket, 0);
      lSetUlong(granted_el, JG_dticket, 0);
      lSetUlong(granted_el, JG_sticket, 0);
      lSetUlong(granted_el, JG_jcoticket, 0);
      lSetUlong(granted_el, JG_jcfticket, 0);
   }
}



/*--------------------------------------------------------------------
 * sge_calc_tickets - calculate proportional shares in terms of tickets
 * for all active jobs.
 *
 * lists->job_list should contain all jobs to be scheduled
 * running_jobs should contain all currently executing jobs
 * finished_jobs should contain all completed jobs
 *--------------------------------------------------------------------*/

int
sge_calc_tickets(
sge_Sdescr_t *lists,
lList *queued_jobs,
lList *running_jobs,
lList *finished_jobs,
int do_usage 
) {
   lListElem *config;
   static u_long sge_scheduling_run = 0;
   u_long sum_of_deadline_tickets = 0,
          sum_of_user_functional_shares = 0,
          sum_of_project_functional_shares = 0,
          sum_of_department_functional_shares = 0,
          sum_of_jobclass_functional_shares = 0,
          sum_of_job_functional_shares = 0,
          sum_of_active_tickets = 0,
          sum_of_active_override_tickets = 0;

   double sum_of_proportions = 0,
          sum_of_m_shares = 0,
          sum_of_adjusted_proportions = 0;

   u_long curr_time, num_jobs, job_ndx;

   sge_ref_t *job_ref = NULL;

   lListElem *job, *ja_tasks;

   lListElem *sge_conf = lFirst(lists->config_list);

   u_long total_share_tree_tickets =
            lGetUlong(sge_conf, SC_weight_tickets_share);
   u_long total_functional_tickets =
            lGetUlong(sge_conf, SC_weight_tickets_functional);
   u_long total_deadline_tickets = 
            lGetUlong(sge_conf, SC_weight_tickets_deadline);

   static int halflife = 0;

   lListElem *root = NULL;

   DENTER(TOP_LAYER, "sge_calc_tickets");

   all_lists = lists;

   curr_time = sge_get_gmt();

   sge_scheduling_run++;
   
   /*-------------------------------------------------------------
    * Decay usage for all users and projects if halflife changes
    *-------------------------------------------------------------*/

   if (do_usage && sge_conf && halflife != lGetUlong(sge_conf, SC_halftime)) {
      lListElem *userprj;
      int oldhalflife = halflife;
      halflife = lGetUlong(sge_conf, SC_halftime);
      /* decay up till now based on old half life (unless it's zero),
         all future decay will be based on new halflife */
      if (oldhalflife == 0)
         calculate_decay_constant(halflife);
      else
         calculate_decay_constant(oldhalflife);
      for_each(userprj, lists->user_list)
         decay_userprj_usage(userprj, sge_scheduling_run, curr_time);
      for_each(userprj, lists->project_list)
         decay_userprj_usage(userprj, sge_scheduling_run, curr_time);
   } else
      calculate_decay_constant(lGetUlong(sge_conf, SC_halftime));

   /*-------------------------------------------------------------
    * Init job_ref_count in each share tree node to zero
    *-------------------------------------------------------------*/

   if ((lists->share_tree))
      if ((root = lFirst(lists->share_tree))) {
         zero_job_ref_count(root);
         set_share_tree_project_flags(lists->project_list, root);
      }

   /*-----------------------------------------------------------------
    * Create and build job reference array
    *-----------------------------------------------------------------*/
    
   num_jobs = 0;
   if (queued_jobs)
      for_each(ja_tasks, queued_jobs)
         num_jobs += lGetNumberOfElem(lGetList(ja_tasks, JB_ja_tasks));
   if (running_jobs)
      for_each(ja_tasks, running_jobs)
         num_jobs += lGetNumberOfElem(lGetList(ja_tasks, JB_ja_tasks));
   if (num_jobs > 0) {
      job_ref = (sge_ref_t *)malloc(num_jobs * sizeof(sge_ref_t));
      memset(job_ref, 0, num_jobs * sizeof(sge_ref_t));
   }

   /*-----------------------------------------------------------------
    * Note: use of the the job_ref_t array assumes that no jobs
    * will be removed from the job lists and the job list order will
    * be maintained during the execution of sge_calc_tickets.
    *-----------------------------------------------------------------*/

   job_ndx = 0;

   if (queued_jobs) {
      for_each(job, queued_jobs) {
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if (!job_is_active(job, ja_task)) {
               sge_clear_ja_task(ja_task);
            } else {
               sge_set_job_refs(job, ja_task, &job_ref[job_ndx], lists);
               if (job_ref[job_ndx].node)
                  lSetUlong(job_ref[job_ndx].node, STN_job_ref_count,
                            lGetUlong(job_ref[job_ndx].node, STN_job_ref_count)+1);
               job_ndx++;
            }
         }
      }
   }

   if (running_jobs) {
      for_each(job, running_jobs) {
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if (!job_is_active(job, ja_task)) {
               sge_clear_job(job);
            } else {
               sge_set_job_refs(job, ja_task, &job_ref[job_ndx], lists);
               if (job_ref[job_ndx].node)
                  lSetUlong(job_ref[job_ndx].node, STN_job_ref_count,
                            lGetUlong(job_ref[job_ndx].node, STN_job_ref_count)+1);
               job_ndx++;
            }
         }
      }
   }

   num_jobs = job_ndx;


   /*-------------------------------------------------------------
    * Calculate the m_shares in the share tree
    *-------------------------------------------------------------*/
     
   if (root) {
      update_job_ref_count(root);
      lSetDouble(root, STN_m_share, 1.0);
      calculate_m_shares(root);
   }

   /*-----------------------------------------------------------------
    * Handle finished jobs
    *-----------------------------------------------------------------*/

   if (do_usage && finished_jobs) {
      for_each(job, finished_jobs) {
         sge_ref_t jref;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            memset(&jref, 0, sizeof(jref));
            sge_set_job_refs(job, ja_task, &jref, lists);
            decay_and_sum_usage(&jref, sge_scheduling_run, curr_time);
            DPRINTF(("DDJU (0) "u32"."u32"\n",
               lGetUlong(job, JB_job_number),
               lGetUlong(ja_task, JAT_task_number))); 
            delete_debited_job_usage(&jref, sge_scheduling_run);
         }
      }
   } else {
      DPRINTF(("\n"));
      DPRINTF(("no DDJU: do_usage: %d finished_jobs %d\n",
         do_usage, (finished_jobs!=NULL)));
      DPRINTF(("\n"));
   }      

   /*-----------------------------------------------------------------
    * PASS 0
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGE Pass 0]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {

      sge_set_job_cnts(&job_ref[job_ndx]);

      /*-------------------------------------------------------------
       * Handle usage
       *-------------------------------------------------------------*/

      if (do_usage)
         decay_and_sum_usage(&job_ref[job_ndx], sge_scheduling_run, curr_time);

      combine_usage(&job_ref[job_ndx]);

      if (total_share_tree_tickets > 0)
         calc_job_share_tree_tickets_pass0(&job_ref[job_ndx],
                                           &sum_of_m_shares,
                                           &sum_of_proportions,
                                           sge_scheduling_run);

   }


   /*-----------------------------------------------------------------
    * PASS 1
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGE Pass 1]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {

      u_long job_deadline_time = lGetUlong(job_ref[job_ndx].job, JB_deadline);

      if (total_share_tree_tickets > 0)
         calc_job_share_tree_tickets_pass1(&job_ref[job_ndx],
                                           sum_of_m_shares,
                                           sum_of_proportions,
                                           &sum_of_adjusted_proportions,
                                           sge_scheduling_run);

      if (total_functional_tickets > 0)
         calc_job_functional_tickets_pass1(&job_ref[job_ndx],
                                          &sum_of_user_functional_shares,
                                          &sum_of_project_functional_shares,
                                          &sum_of_department_functional_shares,
                                          &sum_of_jobclass_functional_shares,
                                          &sum_of_job_functional_shares);

      if (total_deadline_tickets > 0 && job_deadline_time > 0)
         sum_of_deadline_tickets +=
                  calc_job_deadline_tickets_pass1(&job_ref[job_ndx],
                                                  total_deadline_tickets,
                                                  curr_time);

   }

   /*-----------------------------------------------------------------
    * PASS 2
    *-----------------------------------------------------------------*/

   DPRINTF(("=====================[SGE Pass 2]======================\n"));

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {

      job = job_ref[job_ndx].job;

      if (total_share_tree_tickets > 0)
         calc_job_share_tree_tickets_pass2(&job_ref[job_ndx],
                                           sum_of_adjusted_proportions,
                                           total_share_tree_tickets,
                                           sge_scheduling_run);

      if (total_functional_tickets > 0)
         calc_job_functional_tickets_pass2(&job_ref[job_ndx],
                                           sum_of_user_functional_shares,
                                           sum_of_project_functional_shares,
                                           sum_of_department_functional_shares,
                                           sum_of_jobclass_functional_shares,
                                           sum_of_job_functional_shares,
                                           total_functional_tickets);

      if (total_deadline_tickets > 0 && lGetUlong(job, JB_deadline) > 0 &&
            sum_of_deadline_tickets > total_deadline_tickets)
         calc_job_deadline_tickets_pass2(&job_ref[job_ndx],
                                         sum_of_deadline_tickets,
                                         total_deadline_tickets);

      sum_of_active_override_tickets +=
                  calc_job_override_tickets(&job_ref[job_ndx]);

      sum_of_active_tickets += calc_job_tickets(&job_ref[job_ndx]);

   }

   /* set scheduler configuration information to go back to GUI */

   if (lists->config_list && ((config = lFirst(lists->config_list)))) {
      lSetUlong(config, SC_weight_tickets_deadline_active,
		MIN(sum_of_deadline_tickets, total_deadline_tickets));
      lSetUlong(config, SC_weight_tickets_override,
		sum_of_active_override_tickets);
   }

   for(job_ndx=0; job_ndx<num_jobs; job_ndx++) {
      lListElem *ja_task;

      job = job_ref[job_ndx].job;
      ja_task = job_ref[job_ndx].ja_task;

      if (lGetUlong(ja_task, JAT_ticket) > 0)
         lSetDouble(ja_task, JAT_share, (double)lGetUlong(ja_task, JAT_ticket) /
                                   (double)sum_of_active_tickets);
      else
         lSetDouble(ja_task, JAT_share, 0);

   }

   /* update share tree node for finished jobs */
   if (do_usage && finished_jobs) {
      for_each(job, finished_jobs) {
         sge_ref_t jref;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            memset(&jref, 0, sizeof(jref));
            sge_set_job_refs(job, ja_task, &jref, lists);
            if (jref.node) {
               if (sge_scheduling_run != lGetUlong(jref.node, STN_pass2_seqno)) {
                  sge_zero_node_fields(jref.node, NULL);
                  lSetUlong(jref.node, STN_pass2_seqno, sge_scheduling_run);
               }
            }
         }
      }
   }
   
   if (job_ref) {

      for(job_ndx=0; job_ndx<num_jobs; job_ndx++)
         if (job_ref[job_ndx].task_jobclass)
            free(job_ref[job_ndx].task_jobclass);
      free(job_ref);
   }

   DEXIT;
   return sge_scheduling_run;
}

#ifdef notdef

/*--------------------------------------------------------------------
 * sge_calc_tickets_for_queued_job - calculate proportional shares in
 * terms of tickets for a queued job.  This routine returns the
 * number of tickets that the passed job would get if it were running.
 *
 * This routine is designed to be called for a queued job after the
 * normal sge_calc_tickets routine has been executed for all running
 * jobs. This routine depends on totals and sums which are calculated
 * during the execution of sge_calc_tickets.
 *--------------------------------------------------------------------*/

int
sge_calc_tickets_for_queued_job(
sge_Sdescr_t *lists,
lListElem *job,
u_long curr_time 
) {
   int tickets=0;
   sge_ref_t ref;

   memset(&jref, 0, sizeof(jref));
   sge_set_job_refs(job, &ref, lists);
   sge_clear_job(job);

   /* calculate queued job share tree tickets */

   if (((root = lFirst(lists->share_tree))) && ref->node &&
       total_share_tree_tickets > 0) {

      u_long seqno;
      double job_m_share=0, job_proportion=0, job_adjusted_proportion=0;
      lListElem *node;

      sge_job_active(job, lists);

      /* use copy of the share tree node */
      node = ref.node;
      ref.node = lCopyElem(ref.node);

      calc_job_share_tree_tickets_pass0(&ref,
	     &job_m_share,
	     &job_proportion,
	     sge_scheduling_run);

      calc_job_share_tree_tickets_pass1(&ref,
	     sum_of_m_shares + job_m_share,
	     sum_of_proportions + job_proportion,
	     &job_adjusted_proportion,
	     sge_scheduling_run);

      calc_job_share_tree_tickets_pass2(&ref,
	     sum_of_adjusted_proportions + job_adjusted_proportion,
	     total_share_tree_tickets,
	     sge_scheduling_run);

      lFreeElem(ref.node); /* free the copied node */
      ref.node = node;  /* restore share tree node */

      sge_job_inactive(job, lists);

      /* NOTE: check on STN_pass2_seqno - probably need to create STN_mod_seqno
	 for use when checking to see if share tree node has been modified. */

      /* NOTE: need to make sums and totals global statics */

   }

   /* calculate queued job functional tickets */

   if (total_functional_tickets > 0)
      calc_job_functional_tickets_pass2(&ref,
	    sum_of_user_functional_shares +
		  ref->user ? lGetUlong(ref->user, UP_fshare): 0,
	    sum_of_project_functional_shares +
		  ref->project ? lGetUlong(ref->project, UP_fshare): 0,
	    sum_of_department_functional_shares +
		  ref->dept ? lGetUlong(ref->dept, US_fshare): 0,
	    sum_of_jobclass_functional_shares +
		  ref->jobclass ? lGetUlong(ref->jobclass, QU_fshare): 0,
	    sum_of_job_functional_shares +
		  ref->job ? lGetUlong(ref->job, JB_priority): 0,
	    total_functional_tickets);

   /* calculate queued job override tickets */

   sge_set_job_cnts(&ref);
   calc_job_override_tickets(&ref)
   sge_unset_job_cnts(&ref);


   /* calculate queued job deadline tickets */

   if (total_deadline_tickets > 0 &&
       lGetUlong(job_ref[job_ndx].job, JB_deadline) > 0) {

      u_long job_deadline_tickets;
      u_long job_deadline_time = 

      job_deadline_tickets = calc_job_deadline_tickets_pass1(&ref,
	   total_deadline_tickets, curr_time);
      if ((sum_of_deadline_tickets + job_deadline_tickets) >
	  total_deadline_tickets)
         calc_job_deadline_tickets_pass2(&ref,
	      sum_of_deadline_tickets + job_deadline_tickets,
	      total_deadline_tickets);
   }


   /* calculate queued job tickets */

   tickets = calc_job_tickets(&ref);

   return tickets;
}


/*--------------------------------------------------------------------
 * sge_treat_queued_job_as_running - this routine updates certain
 * internal counters to handle treating a queued job as running.  
 * It needs to be called by the job placement code after a decision
 * to execute a job in a particular queue has been made and before
 * making any other decisions. This routine keeps the totals and
 * sums which are used in sge_calc_tickets_for_queued_jobs up to date.
 *--------------------------------------------------------------------*/

int
sge_treat_queued_job_as_running(
sge_Sdescr_t *lists,
lListElem *job 
) {
   sge_ref_t ref;
   lListElem *root;

   memset(&jref, 0, sizeof(jref));
   sge_set_job_refs(job, &ref, lists);

   /* update share tree ticket totals */

   if (((root = lFirst(lists->share_tree))) && ref.node &&
       total_share_tree_tickets > 0) {

      sge_job_active(job, lists);

      calc_job_share_tree_tickets_pass0(&ref,
					&sum_of_m_shares,
					&sum_of_proportions,
					sge_scheduling_run);

      calc_job_share_tree_tickets_pass1(&ref,
					sum_of_m_shares,
					sum_of_proportions,
					&sum_of_adjusted_proportions,
					sge_scheduling_run);

      calc_job_share_tree_tickets_pass2(&ref,
					sum_of_adjusted_proportions,
					total_share_tree_tickets,
					sge_scheduling_run);
   }

   /* update functional ticket totals */

   sum_of_user_functional_shares +=
	 ref->user ? lGetUlong(ref->user, UP_fshare): 0,
   sum_of_project_functional_shares +=
	 ref->project ? lGetUlong(ref->project, UP_fshare): 0,
   sum_of_department_functional_shares +=
	 ref->dept ? lGetUlong(ref->dept, US_fshare): 0,
   sum_of_jobclass_functional_shares +=
	 ref->jobclass ? lGetUlong(ref->jobclass, QU_fshare): 0,
   sum_of_job_functional_shares +=
	 ref->job ? lGetUlong(ref->job, JB_priority): 0,

   /* update override ticket totals */

   sge_set_job_cnts(&ref);

   /* update deadline ticket totals */

   sum_of_deadline_tickets + job_deadline_tickets,

   return 0;
}


/*--------------------------------------------------------------------
 * sge_setup_share_tree - set up share tree for SGE
 *--------------------------------------------------------------------*/

void sge_setup_share_tree(sge_Sdescr_t *lists, lList *queued_jobs, lList *running_jobs);

void
sge_setup_share_tree(
sge_Sdescr_t *lists,
lList *queued_jobs,
lList *running_jobs 
) {
   lListElem *job;
   lListElem *root=NULL;

   if (!lists->share_tree) 
      return;

   root = lFirst(lists->share_tree);

   if (!root) 
      return;

   /*-------------------------------------------------------------
    * Init job_ref_count in each share tree node to zero and
    * set reference to node parent in each node
    *-------------------------------------------------------------*/

   zero_job_ref_count(root);


   /*-------------------------------------------------------------
    * Increment job_ref_count for each active job
    *-------------------------------------------------------------*/
     
   if (queued_jobs)
      for_each(job, queued_jobs) {
         if (job_is_active(job))
            increment_job_ref_count(root, job);
      }
   
   if (running_jobs)
      for_each(job, running_jobs) {
         if (job_is_active(job))
            increment_job_ref_count(root, job);
      }

   /*-------------------------------------------------------------
    * Calculate the m_shares in the share tree
    *-------------------------------------------------------------*/
     
   lSetDouble(root, STN_m_share, 1.0);
   calculate_m_shares(root);

   return;
}




#if 0
/*--------------------------------------------------------------------
 * sge_setup_lists - set up sge info in lists.  Called when the
 * scheduler is initialized.
 *--------------------------------------------------------------------*/

void
sge_setup_lists(
sge_Sdescr_t *lists,
lList *queued_jobs,
lList *running_jobs 
) {

   /*-------------------------------------------------------------
    * Set up the share tree
    *-------------------------------------------------------------*/

   sge_setup_share_tree(lists, queued_jobs, running_jobs);

   /*-------------------------------------------------------------
    * Set decay constants
    *-------------------------------------------------------------*/

   calculate_decay_constant(lists->config_list);
}
#endif


/*--------------------------------------------------------------------
 * sge_job_active - update SGE data structures based on a job becoming
 * active. Called when a job becomes active or has been added to
 * SGE
 *--------------------------------------------------------------------*/

void
sge_job_active(
lListElem *job,
sge_Sdescr_t *lists 
) {
   lListElem *root=NULL;

   /*-------------------------------------------------------------
    * Set references to other objects in the job element
    *-------------------------------------------------------------*/

   if (lists->share_tree && (root = lFirst(lists->share_tree))) {

      /*-------------------------------------------------------------
       * increment job reference count in share tree
       *-------------------------------------------------------------*/

      increment_job_ref_count(root, job);

      /*-------------------------------------------------------------
       * adjust m_shares for affected share tree nodes
       *-------------------------------------------------------------*/

      adjust_m_shares(root, job, 1);

   }
}


/*--------------------------------------------------------------------
 * sge_job_inactive - update SGE data structures based on an active job
 * becoming inactive. Called when a job becomes inactive, ends, or is
 * aborted.
 *--------------------------------------------------------------------*/

void
sge_job_inactive(
lListElem *job,
sge_Sdescr_t *lists 
) {
   lListElem *root=NULL;

   if (lists->share_tree && (root = lFirst(lists->share_tree))) {

      /*-------------------------------------------------------------
       * decrement job reference count in share tree
       *-------------------------------------------------------------*/

      decrement_job_ref_count(root, job);

      /*-------------------------------------------------------------
       * adjust m_shares for affected share tree nodes
       *-------------------------------------------------------------*/

      adjust_m_shares(root, job, 0);

   }
}

#endif /* notdef */

/*--------------------------------------------------------------------
 * sge_dump_list - dump list to stdout (for calling while in debugger)
 *--------------------------------------------------------------------*/

void
sge_dump_list(lList *list)
{
   FILE *f;

   if (!(f=fdopen(1, "w"))) {
      fprintf(stderr, MSG_FILE_OPENSTDOUTASFILEFAILED );
      exit(1);
   }

   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_SGE_UNABLETODUMPJOBLIST );
   }
}


void
dump_list_to_file(lList *list, char *file)
{
   FILE *f;

   if (!(f=fopen(file, "w+"))) {
      fprintf(stderr, MSG_FILE_OPENSTDOUTASFILEFAILED);
      exit(1);
   }

   if (lDumpList(f, list, 0) == EOF) {
      fprintf(stderr, MSG_SGE_UNABLETODUMPJOBLIST );
   }

   fclose(f);
}

/*--------------------------------------------------------------------
 * get_mod_share_tree - return reduced modified share tree
 *--------------------------------------------------------------------*/


static lListElem *
get_mod_share_tree(
lListElem *node,
lEnumeration *what,
int seqno 
) {
   lListElem *new_node=NULL;
   lList *children;
   lListElem *child;

   if (((children = lGetList(node, STN_children))) &&
       lGetNumberOfElem(children) > 0) {

      lList *child_list=NULL;

      for_each(child, children) {
         lListElem *child_node;
         child_node = get_mod_share_tree(child, what, seqno);
         if (child_node) {
            if (!child_list)
               child_list = lCreateList("", STN_Type);
            lAppendElem(child_list, child_node);
         }
      }

      if (child_list) {
#ifdef lmodifywhat_shortcut
         new_node = lCreateElem(STN_Type);
         lModifyWhat(new_node, node, what);
#else
         new_node = lCopyElem(node);
#endif
         lSetList(new_node, STN_children, child_list);
      }
      
   } else {

      if (lGetUlong(node, STN_pass2_seqno) == (u_long32)seqno &&
          lGetUlong(node, STN_temp) == 0) {
#ifdef lmodifywhat_shortcut
         new_node = lCreateElem(STN_Type);
         lModifyWhat(new_node, node, what);
#else
         new_node = lCopyElem(node);
#endif
      }

   }

   return new_node;
}


/*--------------------------------------------------------------------
 * sge_build_sge_orders - build orders for updating qmaster
 *--------------------------------------------------------------------*/

lList *
sge_build_sge_orders(
sge_Sdescr_t *lists,
lList *running_jobs,
lList *finished_jobs,
lList *order_list,
int seqno 
) {
   lCondition *where=NULL;
   lEnumeration *what=NULL;
   lList *up_list;
   lList *config_list;
   lListElem *order;
   lListElem *root;
   lListElem *job;
   int norders; 

   DENTER(TOP_LAYER, "sge_build_sge_orders");

   if (!order_list)
      order_list = lCreateList("orderlist", OR_Type);

   DPRINTF(("   got %d running jobs\n", lGetNumberOfElem(running_jobs)));

   /*-----------------------------------------------------------------
    * build ticket orders for running jobs
    *-----------------------------------------------------------------*/

   if (running_jobs) {
      norders = lGetNumberOfElem(order_list);

      for_each(job, running_jobs) {
         char *pe_str;
         lList *granted;
         lListElem *pe;
         lListElem *ja_task;

         for_each(ja_task, lGetList(job, JB_ja_tasks)) {
            if ((pe_str=lGetString(ja_task, JAT_granted_pe))
                  && (pe=lGetElemStr(all_lists->pe_list, PE_name, pe_str))
                  && lGetUlong(pe, PE_control_slaves))

               granted=lGetList(ja_task, JAT_granted_destin_identifier_list);
            else
               granted=NULL;

            order_list = sge_create_orders(order_list, ORT_tickets, job, ja_task, granted);
         }
      }
      DPRINTF(("   added %d ticket orders for running jobs\n", 
         lGetNumberOfElem(order_list) - norders));
   }

   /*-----------------------------------------------------------------
    * build delete job orders for finished jobs
    *-----------------------------------------------------------------*/
   if (finished_jobs) {
      DPRINTF(("### splited finished "u32" jobs\n", 
         (u_long32) lGetNumberOfElem(finished_jobs))); 
      order_list  = create_delete_job_orders(finished_jobs, order_list);
   }

   /*-----------------------------------------------------------------
    * build update user usage order
    *-----------------------------------------------------------------*/

   what = lWhat("%T(%I %I %I %I %I %I)", UP_Type,
                UP_name, UP_usage, UP_usage_time_stamp,
                UP_long_term_usage, UP_project, UP_debited_job_usage);

   /* NOTE: make sure we get all usage entries which have been decayed
      or have accumulated additional usage */

   where = lWhere("%T(%I == %u)", UP_Type, UP_usage_seqno, seqno);

   if (lists->user_list) {
      norders = lGetNumberOfElem(order_list); 
      if ((up_list = lSelect("", lists->user_list, where, what))) {
         if (lGetNumberOfElem(up_list)>0) {
            order = lCreateElem(OR_Type);
            lSetUlong(order, OR_seq_no, get_seq_nr());
            lSetUlong(order, OR_type, ORT_update_user_usage);
            lSetList(order, OR_joker, up_list);
            lAppendElem(order_list, order);
         } else
            lFreeList(up_list);
      }
      DPRINTF(("   added %d orders for updating usage of user\n",
         lGetNumberOfElem(order_list) - norders));      
   }

   /*-----------------------------------------------------------------
    * build update project usage order
    *-----------------------------------------------------------------*/

   if (lists->project_list) {
      norders = lGetNumberOfElem(order_list); 
      if ((up_list = lSelect("", lists->project_list, where, what))) {
         if (lGetNumberOfElem(up_list)>0) {
            order = lCreateElem(OR_Type);
            lSetUlong(order, OR_seq_no, get_seq_nr());
            lSetUlong(order, OR_type, ORT_update_project_usage);
            lSetList(order, OR_joker, up_list);
            lAppendElem(order_list, order);
         } else
            lFreeList(up_list);
      }
      DPRINTF(("   added %d orders for updating usage of project\n",
         lGetNumberOfElem(order_list) - norders));
   }

   lFreeWhat(what);
   lFreeWhere(where);

   /*-----------------------------------------------------------------
    * build update share tree order
    *-----------------------------------------------------------------*/

   if (lists->share_tree && ((root = lFirst(lists->share_tree)))) {
      lListElem *node;
      lEnumeration *so_what;
      norders = lGetNumberOfElem(order_list);

      so_what = lWhat("%T(%I %I %I %I %I %I)", STN_Type,
                      STN_version, STN_name, STN_job_ref_count, STN_m_share,
                      STN_last_actual_proportion,
                      STN_adjusted_current_proportion);

      if ((node = get_mod_share_tree(root, so_what, seqno))) {
         up_list = lCreateList("", STN_Type);
         lAppendElem(up_list, node);
         order = lCreateElem(OR_Type);
         lSetUlong(order, OR_seq_no, get_seq_nr());
         lSetUlong(order, OR_type, ORT_share_tree);
         lSetList(order, OR_joker, up_list);
         lAppendElem(order_list, order);
      }
      DPRINTF(("   added %d orders for updating share tree\n",
         lGetNumberOfElem(order_list) - norders)); 

      lFreeWhat(so_what);
   } 

   /*-----------------------------------------------------------------
    * build update scheduler configuration order
    *-----------------------------------------------------------------*/

   what = lWhat("%T(%I %I)", SC_Type,
		SC_weight_tickets_deadline_active,
		SC_weight_tickets_override);

   if (lists->config_list) {
      norders = lGetNumberOfElem(order_list); 
      if ((config_list = lSelect("", lists->config_list, NULL, what))) {
         if (lGetNumberOfElem(config_list)>0) {
            order = lCreateElem(OR_Type);
            lSetUlong(order, OR_seq_no, get_seq_nr());
            lSetUlong(order, OR_type, ORT_sched_conf);
            lSetList(order, OR_joker, config_list);
            lAppendElem(order_list, order);
         } else
            lFreeList(config_list);
      }
      DPRINTF(("   added %d orders for scheduler configuration\n",
         lGetNumberOfElem(order_list) - norders));   
   }

   lFreeWhat(what);

   return order_list;
}

/*--------------------------------------------------------------------
 * sge_sort_jobs - sort jobs according the task-tickets and job number 
 *--------------------------------------------------------------------*/

void sge_sort_jobs(
lList **job_list              /* JB_Type */
) {
   lListElem *job = NULL, *nxt_job = NULL;     
   lList *tmp_list = NULL;    /* SGEJ_Type */

   DENTER(TOP_LAYER, "sge_sort_jobs");

   if (!job_list || !*job_list) {
      DEXIT;
      return;
   }

#if 0
   DPRINTF(("+ + + + + + + + + + + + + + + + \n"));
   DPRINTF(("     SORTING SGE JOB LIST       \n"));
   DPRINTF(("+ + + + + + + + + + + + + + + + \n"));
#endif

   /*-----------------------------------------------------------------
    * Create tmp list 
    *-----------------------------------------------------------------*/
   tmp_list = lCreateList("tmp list", SGEJ_Type);

   nxt_job = lFirst(*job_list); 
   while((job=nxt_job)) {
      lListElem *tmp_sge_job = NULL;   /* SGEJ_Type */
      
      nxt_job = lNext(nxt_job);
      tmp_sge_job = lCreateElem(SGEJ_Type);
      lSetUlong(tmp_sge_job, SGEJ_ticket, 
         lGetUlong(lFirst(lGetList(job, JB_ja_tasks)), JAT_ticket));
      lSetUlong(tmp_sge_job, SGEJ_job_number, lGetUlong(job, JB_job_number));
      lSetRef(tmp_sge_job, SGEJ_job_reference, job);
#if 0
      DPRINTF(("JOB: "u32" TICKETS: "u32"\n", 
         lGetUlong(tmp_sge_job, SGEJ_job_number), 
         lGetUlong(tmp_sge_job, SGEJ_ticket)));
#endif
      lAppendElem(tmp_list, tmp_sge_job);
      
      lDechainElem(*job_list, job);
   }

   /*-----------------------------------------------------------------
    * Sort tmp list
    *-----------------------------------------------------------------*/
   lPSortList(tmp_list, "%I- %I+", SGEJ_ticket, SGEJ_job_number);

   /*-----------------------------------------------------------------
    * rebuild job_list according sort order
    *-----------------------------------------------------------------*/
   for_each(job, tmp_list) {
      lAppendElem(*job_list, lGetRef(job, SGEJ_job_reference)); 
   } 

   /*-----------------------------------------------------------------
    * Release tmp list
    *-----------------------------------------------------------------*/
   lFreeList(tmp_list);

   DEXIT;
   return;
}


/*--------------------------------------------------------------------
 * sge_scheduler
 *--------------------------------------------------------------------*/

int sge_scheduler(
sge_Sdescr_t *lists,
lList *running_jobs,   
lList *finished_jobs,   
lList **orderlist    
) {
   static u_long32 next = 0;
   u_long32 now;
   u_long seqno;
   lListElem *job;

   DENTER(TOP_LAYER, "sge_scheduler");

#if 0
   sge_setup_lists(lists, NULL, running_jobs); /* resetup each time */
#endif

   /* clear SGE fields for queued jobs */

   if (lists->job_list)
      for_each(job, lists->job_list)
         sge_clear_job(job);

   /*

      On a "normal" scheduling interval:

	 calculate tickets for new and running jobs

	 don't decay and sum usage

	 don't update qmaster

      On a SGE scheduling interval:

	 calculate tickets for new and running jobs

	 decay and sum usage

	 handle finished jobs

	 update qmaster

   */

   if ((now = sge_get_gmt())<next) {

      seqno = sge_calc_tickets(lists, NULL, running_jobs, NULL, 0);

   } else {

      DPRINTF(("=-=-=-=-=-=-=-=-=-=-=-=-=-=  SGE ORDER   =-=-=-=-=-=-=-=-=-=-=-=-=-=\n"));
      seqno = sge_calc_tickets(lists, NULL, running_jobs, finished_jobs, 1);

      *orderlist = sge_build_sge_orders(lists, running_jobs, finished_jobs,
                                        *orderlist, seqno);
      next = now + scheddconf.sgeee_schedule_interval;
   }

   DEXIT;
   return 0;
}


/* ----------------------------------------

   calculate_host_tickets()

   calculates the total number of tickets on a host from the
   JB_ticket field for jobs associated with the host


   returns:
      0 successful
     -1 errors in functions called by calculate_host_tickets

*/
int calculate_host_tickets(
lList **running,   /* JB_Type */
lList **hosts      /* EH_Type */
) {
char *host_name;
u_long host_sge_tickets;
lListElem *hep, *running_job_elem, *rjq;
   DENTER(TOP_LAYER, "calculate_host_tickets");

   if (!hosts) {
      DEXIT;
      return -1;
   }

  if (!running) {
      for_each (hep, *hosts) {
         lSetUlong(hep, EH_sge_tickets, 0);
      }
      DEXIT;
      return 0;
   }

   for_each (hep, *hosts) {
      host_name = lGetString(hep, EH_name);
      lSetUlong(hep, EH_sge_tickets, 0);
      host_sge_tickets = 0;
      for_each (running_job_elem, *running) {
         lListElem* ja_task;

         for_each(ja_task, lGetList(running_job_elem, JB_ja_tasks)) {
            for_each (rjq, lGetList(ja_task, JAT_granted_destin_identifier_list)) {
               if (hostcmp(lGetString(rjq, JG_qhostname), host_name) == 0)
                  host_sge_tickets += lGetUlong(ja_task, JAT_ticket);
            }
         }
      }
      lSetUlong(hep, EH_sge_tickets, host_sge_tickets);
   }

   DEXIT;
   return 0;
}



/*************************************************************************

   sort_host_list_by_share_load

   purpose:
      sort host list according to a share load evaluation formula.

   return values:
      0 on success; -1 otherwise

   input parameters:
      hl             :  the host list to be sorted
      cplx_list      :  the complex list
      formula        :  the share load evaluation formula (containing no blanks)

   output parameters:
      hl             :  the sorted host list

*************************************************************************/
int sort_host_list_by_share_load(
lList *hl,       /* EH_Type */
lList *cplx_list /* CX_Type */
) {
   lListElem *hlp;
   char *host;
   u_long total_SGE_tickets = 0;
   double total_resource_capability_factor = 0.0;
   double resource_allocation_factor = 0.0;
   double s_load = 0.0;
   u_long total_load = 0;
#ifdef notdef
   lListElem *host_complex;
   lList *host_complex_attributes = NULL;
#endif

   DENTER(TOP_LAYER, "sort_host_list_by_share_load");

#ifdef notdef
   /*
      don't panic if there is no host_complex
       a given attributename does not exist -
      error handling is done in scale_load_value()
   */
   if ((host_complex = lGetElemStr(cplx_list, CX_name, SGE_HOST_NAME)))
      host_complex_attributes = lGetList(host_complex, CX_entries);

#endif

   /* collect  share load parameter totals  for each host*/

   for_each (hlp, hl) {
      /* EH_sort_value (i.e., load) should not be less than 1 */
      host = lGetString(hlp,EH_name);
      if (strcmp(host, "global")) { /* don't treat global */
         if (lGetDouble(hlp, EH_sort_value) < 1) {
            lSetDouble(hlp, EH_sort_value, 1);
         }
      }

      total_SGE_tickets += lGetUlong(hlp, EH_sge_tickets);

      if (strcmp(host, "global") && !(lGetDouble(hlp, EH_sort_value) == ERROR_LOAD_VAL)) { /* don't treat global */
         total_resource_capability_factor +=  lGetDouble(hlp, EH_resource_capability_factor);
      }   /* don't treat global */


      if (!(lGetDouble(hlp, EH_sort_value) == ERROR_LOAD_VAL)){
         total_load += lGetDouble(hlp, EH_sort_value);
      }
   }

   if ( total_resource_capability_factor == 0.0)  {
      for_each(hlp, hl)  {
         host = lGetString(hlp,EH_name);
         if (strcmp(host, "global") && !(lGetDouble(hlp, EH_sort_value) == ERROR_LOAD_VAL)) { /* don't treat global */
            lSetDouble(hlp, EH_resource_capability_factor, 1.0);
            total_resource_capability_factor += 1.0;
         } /* for_each(hlp, hl) */
      }  /* don't treat global */
   }  /* total_resource_capability_factor == 0.0 */ 
 

   /* Calculate share load parameter percentages for each host*/

   for_each (hlp, hl) {
      host = lGetString(hlp,EH_name);
      if (strcmp(host, "global")) { /* don't treat global */

         if (!(total_SGE_tickets == 0)){
             lSetDouble(hlp, EH_sge_ticket_pct, (((double) lGetUlong(hlp, EH_sge_tickets))/((double) total_SGE_tickets))*100.0);
         }
         else {
            lSetDouble(hlp, EH_sge_ticket_pct, 0.0);
         }
         lSetDouble(hlp, EH_resource_capability_factor_pct, (lGetDouble(hlp,EH_resource_capability_factor)/total_resource_capability_factor)*100);

      if (!(lGetDouble(hlp, EH_sort_value)==ERROR_LOAD_VAL)){
         if (!total_load == 0) {
            lSetDouble(hlp, EH_sge_load_pct, ((lGetDouble(hlp,EH_sort_value))/((double) total_load))*100.0);
         }
         else {
         lSetDouble(hlp, EH_sge_load_pct, 1.0);
         }
      }
      else {
         lSetDouble(hlp, EH_sge_load_pct, (double) ERROR_LOAD_VAL);
      }

       } /* don't treat global */
   } /* for_each(hlp, hl) */

   /* Calculate share load quantities for each job */

   for_each (hlp, hl) {
      host = lGetString(hlp,EH_name);
      if (strcmp(host,"global")) { /* don't treat global */
         if (!(lGetDouble(hlp, EH_sort_value)==ERROR_LOAD_VAL)){
            resource_allocation_factor = (( lGetDouble(hlp, EH_resource_capability_factor_pct)) - ( lGetDouble(hlp,EH_sge_ticket_pct)));

#ifdef notdef
/*  REMOVE AFTER TESTING */
        lSetUlong(hlp, EH_sge_load, (u_long) (((( lGetDouble(hlp, EH_resource_capability_factor_pct)) - ( lGetDouble(hlp,EH_sge_ticket_pct)))/( lGetDouble(hlp,EH_sge_load_pct)))*100.0 + 1000.0));
#endif /* notdef */

         if (resource_allocation_factor < 0) {

            s_load = (((resource_allocation_factor)/(101.0 -  lGetDouble(hlp,EH_sge_load_pct)))*100.0);


            if (s_load < -100000.0) {

               lSetUlong(hlp, EH_sge_load, 1);

            }
            else  {

               lSetUlong(hlp, EH_sge_load, (u_long) (((resource_allocation_factor)/(101.0 -  lGetDouble(hlp,EH_sge_load_pct)))*100.0 +
100000.0));
            }
         }
         else {

            lSetUlong(hlp, EH_sge_load, (u_long) (((resource_allocation_factor)/( lGetDouble(hlp,EH_sge_load_pct)))*100.0 + 100000.0));


         } /* if resource_allocation_factor */

         }
         else {
            lSetUlong(hlp, EH_sge_load, 0);
         } /* ERROR_LOAD ? */

#ifdef notdef
       lSetDouble(hlp, EH_sort_value,
       load = scaled_mixed_load(lGetList(hlp, EH_load_list),
       lGetList(hlp, EH_scaling_list),
       host_complex_attributes,
       (double)lGetUlong(hlp, EH_load_correction_factor)/100));
#endif /* notdef */

      }
      else {
         lSetUlong(hlp, EH_sge_load, 0);
      } /* don't treat global */
   }

  /* sort host list in descending order according to sge_load */

   if (lPSortList(hl,"%I- %I+", EH_sge_load, EH_sort_value)) {
      DEXIT;
      return -1;
   } else {
      DEXIT;
      return 0;
   }
}

void print_job_ref_array(
sge_ref_t *ref_list,
int max_elem 
) {
   int i;

   for(i=0; i<max_elem; i++) {
      fprintf(stderr, "###"
         " Job: "u32
         " Task: "u32 
         " t: "u32
         " JobClasses: %d"
         " TreeType: "u32
         " JobClasse-FTicket: %f"
         " JobClasse-OTicket: %f"
         "\n",
         lGetUlong(ref_list[i].job, JB_job_number),
         lGetUlong(ref_list[i].ja_task, JAT_task_number),
         lGetUlong(ref_list[i].ja_task, JAT_ticket),
         ref_list[i].num_task_jobclasses,
         ref_list[i].share_tree_type,
         ref_list[i].total_jobclass_ftickets,
         ref_list[i].total_jobclass_otickets);

   }
}

#ifdef MODULE_TEST

int
main(int argc, char **argv)
{
   int job_number;
   sge_Sdescr_t *lists;
   lList *child_list = NULL;
   lList *group_list = NULL;
   lListElem *job, *config, *node, *usage;

   lListElem *ep;

   lists = (sge_Sdescr_t *)malloc(sizeof(sge_Sdescr_t));
   memset(lists, 0, sizeof(sge_Sdescr_t));


#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   lInit(nmv);

   /* build host list */
   lAddElemStr(&(lists->host_list), EH_name, "racerx", EH_Type); 
   lAddElemStr(&(lists->host_list), EH_name, "yosemite_sam", EH_Type); 
   lAddElemStr(&(lists->host_list), EH_name, "fritz", EH_Type); 

   /* build queue list */
   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "racerx.q", QU_Type); 
   lSetString(ep, QU_qhostname, "racerx");

   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "yosemite_sam.q", QU_Type); 
   lSetString(ep, QU_qhostname, "yosemite_sam");

   ep = lAddElemStr(&(lists->all_queue_list), QU_qname, "fritz.q", QU_Type); 
   lSetString(ep, QU_qhostname, "fritz");
   
   /* build configuration */

   config = lCreateElem(SC_Type);
   lSetUlong(config, SC_halftime, 60*60);
   lSetList(config, SC_usage_weight_list,
       build_usage_list("usageweightlist", NULL));
   for_each(usage, lGetList(config, SC_usage_weight_list))
      lSetDouble(usage, UA_value, 1.0/3.0);
   lSetDouble(config, SC_compensation_factor, 2);
   lSetDouble(config, SC_weight_user, 0.25);
   lSetDouble(config, SC_weight_project, 0.25);
   lSetDouble(config, SC_weight_jobclass, 0.25);
   lSetDouble(config, SC_weight_department, 0.25);
   lSetUlong(config, SC_weight_tickets_functional, 10000);
   lSetUlong(config, SC_weight_tickets_share, 10000);
   lSetUlong(config, SC_weight_tickets_deadline, 10000);
   lists->config_list = lCreateList("config_list", SC_Type);
   lAppendElem(lists->config_list, config);

   /* build user list */

   ep = lAddElemStr(&(lists->user_list), UP_name, "davidson", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 200);

   ep = lAddElemStr(&(lists->user_list), UP_name, "garrenp", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);

   ep = lAddElemStr(&(lists->user_list), UP_name, "stair", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);

   /* build project list */
    
   ep = lAddElemStr(&(lists->project_list), UP_name, "sge", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 200);

   ep = lAddElemStr(&(lists->project_list), UP_name, "ms", UP_Type); 
   lSetUlong(ep, UP_oticket, 0);
   lSetUlong(ep, UP_fshare, 100);


   /* build department list */

   ep = lAddElemStr(&(lists->dept_list), US_name, "software", US_Type); 
   lSetUlong(ep, US_oticket, 0);
   lSetUlong(ep, US_fshare, 200);

   ep = lAddElemStr(&(lists->dept_list), US_name, "hardware", US_Type); 
   lSetUlong(ep, US_oticket, 0);
   lSetUlong(ep, US_fshare, 100);

   /* build share tree list */

   child_list = lCreateList("childlist", STN_Type);

   group_list = lCreateList("grouplist", STN_Type);

   node = lAddElemStr(&child_list, STN_name, "davidson", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&child_list, STN_name, "garrenp", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&group_list, STN_name, "groupA", STN_Type);
   lSetUlong(node, STN_shares, 1000);
   lSetList(node, STN_children, child_list);


   child_list = lCreateList("childlist", STN_Type);

   node = lAddElemStr(&child_list, STN_name, "stair", STN_Type);
   lSetUlong(node, STN_shares, 100);

   node = lAddElemStr(&group_list, STN_name, "groupB", STN_Type);
   lSetUlong(node, STN_shares, 100);
   lSetList(node, STN_children, child_list);

   node = lAddElemStr(&(lists->share_tree), STN_name, "root", STN_Type);
   lSetUlong(node, STN_type, STT_USER);
   lSetList(node, STN_children, group_list);

   /* build job list */

   job_number = 1;


   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "davidson");
   lSetString(job, JB_project, "sge");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetString(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 1000);
   lSetString(job, JB_owner, "davidson");
   lSetString(job, JB_project, "ms");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetString(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_job_number, job_number++);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "stair");
   lSetString(job, JB_project, "sge");
   lSetString(job, JB_department, "hardware");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, 0);
   lSetString(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());

   job = lAddElemUlong(&(lists->job_list), JB_job_number, job_number++, 
                           JB_Type);
   lSetUlong(job, JB_priority, 0);
   lSetString(job, JB_owner, "garrenp");
   lSetString(job, JB_project, "sge");
   lSetString(job, JB_department, "software");
   lSetUlong(job, JB_submission_time, sge_get_gmt() - 60*2);
   lSetUlong(job, JB_deadline, sge_get_gmt() + 60*2);
   lSetString(job, JB_host, "racerx");
   lSetUlong(job, JB_override_tickets, 0);
   lSetList(job, JB_scaled_usage_list, build_usage_list("jobusagelist", NULL));
   for_each(usage, lGetList(job, JB_scaled_usage_list))
      lSetDouble(usage, UA_value, drand48());


   /* call the SGE scheduler */

   sge_setup_lists(lists, lists->job_list, NULL);

   sge_calc_tickets(lists, lists->job_list, NULL, NULL);

   lWriteListTo(lists->job_list, stdout);

   sleep(1);

   sge_calc_tickets(lists, lists->job_list, NULL, NULL);

   lWriteListTo(lists->job_list, stdout);

   sge_calc_share_tree_proportions( lists->share_tree,
                                    lists->user_list,
                                    lists->project_list,
                                    lists->config_list);

   lWriteListTo(lists->share_tree, stdout);

   return 0;
}

#endif


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


