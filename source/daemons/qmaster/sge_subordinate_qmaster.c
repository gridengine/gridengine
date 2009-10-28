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
 *  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *  Copyright: 2001 by Sun Microsystems, Inc.
 *
 *  All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/
#include <string.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_time.h"
#include "sge_conf.h"
#include "sge_sched.h"
#include "sge_signal.h"
#include "sge_event_master.h"
#include "sge_qmod_qmaster.h"
#include "sge_qinstance_qmaster.h"
#include "sge_subordinate_qmaster.h"
#include "msg_qmaster.h"
#include "sge_string.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_qinstance.h"
#include "sge_qinstance_state.h"
#include "sge_job.h"
#include "sge_cqueue.h"
#include "sge_object.h"
#include "sge_subordinate.h"
#include "sge_qref.h"
#include "sge_ja_task_JAT_L.h"
#include "sge_sl.h"

static bool
qinstance_x_on_subordinate(sge_gdi_ctx_class_t *ctx,
                           lListElem *this_elem, bool suspend,
                           bool send_event, monitoring_t *monitor);


/****** sge_subordinate_qmaster/slotwise_x_on_subordinate() ********************
*  NAME
*     slotwise_x_on_subordinate() -- Execute the (un)suspend
*
*  SYNOPSIS
*     static bool slotwise_x_on_subordinate(sge_gdi_ctx_class_t *ctx, lListElem 
*     *qinstance_where_task_is_running, lUlong job_id, lUlong task_id, bool 
*     suspend, bool send_signal, monitoring_t *monitor) 
*
*  FUNCTION
*     Executes the (un)suspend of the specified task. 
*
*  INPUTS
*     sge_gdi_ctx_class_t *ctx                   - GDI context
*     lListElem *qinstance_where_task_is_running - QU_Type Element of the qinstance
*                                                  in which the task to (un)suspend
*                                                  is running/suspended.
*     lUlong job_id                              - Job ID of the task to (un)suspend
*     lUlong task_id                             - Task ID of the task to (un)suspend
*     bool suspend                               - suspend or unsuspend
*     monitoring_t *monitor                      - monitor
*
*  RESULT
*     bool - true:  Task is (un)suspended.
*            false: An error occcurred.
*
*  NOTES
*     MT-NOTE: slotwise_x_on_subordinate() is not MT safe, the global lock
*              must be set outside before calling this function
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
slotwise_x_on_subordinate(sge_gdi_ctx_class_t *ctx,
                          lListElem *qinstance_where_task_is_running,
                          lUlong job_id, lUlong task_id, bool suspend,
                          monitoring_t *monitor)
{
   bool ret = false;
   lListElem *jep = NULL;
   lListElem *jatep = NULL;
   lList *master_job_list = NULL;
   lUlong state = 0;

   DENTER(TOP_LAYER, "slotwise_x_on_subordinate");

   master_job_list = *object_type_get_master_list(SGE_TYPE_JOB);
   jep = lGetElemUlong(master_job_list, JB_job_number, job_id);
   if (jep != NULL) {
      jatep = lGetSubUlong(jep, JAT_task_number, task_id, JB_ja_tasks);
      if (jatep != NULL) {
         sge_signal_queue(ctx, suspend?SGE_SIGSTOP:SGE_SIGCONT,
               qinstance_where_task_is_running, jep, jatep, monitor);
         /* Set status */
         state = lGetUlong(jatep, JAT_state);

         if (suspend == true) {
            SETBIT(JSUSPENDED_ON_SLOTWISE_SUBORDINATE, state);
            DPRINTF(("Setting status JSUSPENDED_ON_SLOTWISE_SUBORDINATE for job %lu.%lu\n",
                     job_id, task_id));
         } else {
            CLEARBIT(JSUSPENDED_ON_SLOTWISE_SUBORDINATE, state);
            DPRINTF(("Clearing status JSUSPENDED_ON_SLOTWISE_SUBORDINATE for job %lu.%lu\n",
                     job_id, task_id)); 
         }
         lSetUlong(jatep, JAT_state, state);
         ret = true;
      } else {
         /* TODO: HP: Add error handling! */
      }
   } else {
      /* TODO: HP: Add error handling! */
   }
   DRETURN(ret);
}

/****** sge_subordinate_qmaster/get_slotwise_sos_tree_root() *******************
*  NAME
*     get_slotwise_sos_tree_root() -- Gets the root qinstance of the slotwise
*        suspend on subordinate tree.
*
*  SYNOPSIS
*     static lListElem* get_slotwise_sos_tree_root(lListElem 
*     *node_queue_instance) 
*
*  FUNCTION
*     Returns the qinstance that is the root of the slotwise suspend on
*     subordinate tree where the provided qinstance is a member of.
*     Returns NULL if the give qinstance is not a member of a slotwise suspend
*     on subordinate tree.
*
*  INPUTS
*     lListElem *node_queue_instance -  For this queue instance the slotwise
*                                       suspend on subordinate tree root is
*                                       searched.
*
*  RESULT
*     lListElem* - The root node of the slotwise suspend on subordinate
*                  tree, node_queue_instance if it is the root node,
*                  or NULL if node_queue_instance is not part of any
*                  slotwise suspend on subordinate definition.
*
*  NOTES
*     MT-NOTE: get_slotwise_sos_tree_root() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
/* TODO: HP: Use get_slotwise_super_qinstance() recursively instead of this function */
static lListElem*
get_slotwise_sos_tree_root(lListElem *node_queue_instance)
{
   lUlong    slots_sum = 0;
   lListElem *cqueue = NULL;
   lListElem *root_qinstance = NULL;
   lList *sub_list = NULL;
   lList *cqueue_master_list = *object_type_get_master_list(SGE_TYPE_CQUEUE);
   const char *node_queue_name = NULL;
   const char *node_host_name = NULL;

   DENTER(TOP_LAYER, "get_slotwise_sos_tree_root");

   if (node_queue_instance != NULL) {
      /* For now, assume this qinstance is the root node of the slotwise sos tree. */
      sub_list = lGetList(node_queue_instance, QU_subordinate_list);
      if (sub_list != NULL) {
         slots_sum = lGetUlong(lFirst(sub_list), SO_slots_sum);
         if (slots_sum > 0) {
            /* Slotwise sos; slots_sum == 0 would be queuewise sos. */
            root_qinstance = node_queue_instance;
         }
      }

      /* Search the superordinated queue instance of our node_queue_instance.
       * Then search the superordinated queue instance of this superordinated
       * queue instance, and so on.
       * We have to search in queue instances, not in queues, because possibly
       * for our current host there is no superordinated queue instance of our
       * node_queue_instance, while there might be one on another host.
       */
      node_queue_name = lGetString(node_queue_instance, QU_qname);
      node_host_name  = lGetHost(node_queue_instance, QU_qhostname);

      for_each(cqueue, cqueue_master_list) {
         lListElem *qinstance;
         qinstance = cqueue_locate_qinstance(cqueue, node_host_name);

         if (qinstance == NULL) {
            /* There is no instance of this cluster queue on this host. Continue
             * with another branch of the tree.
             */
            continue;
         }

         sub_list = lGetList(qinstance, QU_subordinate_list);
         if (sub_list != NULL) {
            lListElem *sub = NULL;
            const char *sub_name = NULL;

            for_each(sub, sub_list) {
               sub_name = lGetString(sub, SO_name);
               slots_sum = lGetUlong(sub, SO_slots_sum);
               if (slots_sum > 0 &&
                   strcmp(sub_name, node_queue_name) == 0) {
                  /* Our node queue is mentioned in the subordinate_list of
                   * this queue. This queue is our superordinated queue,
                   * i.e. the parent in the slotwise sos tree!
                   * Now we can look for the parent of our parent.
                   */

                  root_qinstance = get_slotwise_sos_tree_root(qinstance); 
               }
            }
         }
      }
   }
   DRETURN(root_qinstance);
}

/****** sge_subordinate_qmaster/get_slotwise_suspend_superordinate() ***********
*  NAME
*     get_slotwise_suspend_superordinate() -- Get the superordinate of the
*                                             qinstance with the provided name
*
*  SYNOPSIS
*     static lListElem* get_slotwise_suspend_superordinate(const char 
*     *queue_name, const char *hostname) 
*
*  FUNCTION
*     Returns the slotwise superordinated queue instance of the queue instance
*     with the provided name.
*
*  INPUTS
*     const char *queue_name - cluster queue name of the subordinated qeueue
*                              instance
*     const char *hostname   - host name of the subordinated queue instance
*
*  RESULT
*     lListElem* - The slotwise superordinated queue instance (QU_Type) of
*                  the provided queue instance, or NULL if there is none.
*
*  NOTES
*     MT-NOTE: get_slotwise_suspend_superordinate() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static lListElem*
get_slotwise_suspend_superordinate(const char *queue_name, const char *hostname)
{
   lListElem *cqueue = NULL;
   lListElem *qinstance = NULL;
   lListElem *super_qinstance = NULL;
   lList *sub_list = NULL;
   lList *cqueue_master_list = *object_type_get_master_list(SGE_TYPE_CQUEUE);

   DENTER(TOP_LAYER, "get_slotwise_suspend_superordinates");

   for_each(cqueue, cqueue_master_list) {
      qinstance = cqueue_locate_qinstance(cqueue, hostname);

      if (qinstance != NULL) {
          sub_list = lGetList(qinstance, QU_subordinate_list);
          if (sub_list != NULL) {
             /* This queue has subordinates, check if our queue is
              * mentioned in the list of subordinates
              */
             lUlong slots_sum = 0;
             lListElem *sub = NULL;

             /* We are looking for slotwise suspend on subordinate only! */
             sub = lFirst(sub_list);
             if (sub != NULL) {
                slots_sum = lGetUlong(sub, SO_slots_sum);
             }
             if (slots_sum > 0) {
                /* Only if slots_sum > 0 we have slotwise suspend */
                lListElem *so = lGetElemStr(sub_list, SO_name, queue_name);
                if (so != NULL) {
                   /* the queue_name is listed in the subordinate list of this
                    * queue instance, so it's our superordinated queue instance.
                    */
                   super_qinstance = qinstance;
                   break;
                }
             }
          }
       }
   }
   DRETURN(super_qinstance);
}

/****** sge_subordinate_qmaster/get_slotwise_sos_super_qinstance() *************
*  NAME
*     get_slotwise_sos_super_qinstance() -- Get the superordinate of the
*                                           qinstance with the provided name
*
*  SYNOPSIS
*     static lListElem* get_slotwise_sos_super_qinstance(lListElem *qinstance) 
*
*  FUNCTION
*     Returns the slotwise superordinated queue instance of the provided queue
*     instance.
*
*  INPUTS
*     lListElem *qinstance - The subordinated queue instance (QU_Type)
*
*  RESULT
*     lListElem* - The slotwise superordinated queue instance (QU_Type) of
*                  the provided queue instance, or NULL if there is none.
*
*  NOTES
*     MT-NOTE: get_slotwise_sos_super_qinstance() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static lListElem* 
get_slotwise_sos_super_qinstance(lListElem *qinstance)
{
   lListElem  *super_qinstance = NULL;
   const char *qinstance_name = NULL;
   const char *qinstance_host = NULL;

   if (qinstance != NULL) {
      qinstance_name = lGetString(qinstance, QU_qname);
      qinstance_host = lGetHost(qinstance, QU_qhostname);
      super_qinstance = get_slotwise_suspend_superordinate(qinstance_name, qinstance_host);
   }
   return super_qinstance;
}

typedef struct {
   lUlong    job_id;
   lListElem *task; /* JAT_Type */
} ssos_task_t;

typedef struct {
   lUlong        depth;
   lUlong        seq_no;     /* from the parents QU_subordinate_list */
   lUlong        action;     /* from the parents QU_subordinate_list */
   lListElem     *qinstance; /* QU_Type */
   lListElem     *parent;    /* QU_Type */
   sge_sl_list_t *tasks;     /* JAT_Type? */
} ssos_qinstance_t;

/****** sge_subordinate_qmaster/destroy_slotwise_sos_task_elem() ***************
*  NAME
*     destroy_slotwise_sos_task_elem() -- Destructor for the elements of a 
*                                         sge simple list of ssos_taks_t elements
*
*  SYNOPSIS
*     static bool destroy_slotwise_sos_task_elem(ssos_task_t **ssos_task) 
*
*  FUNCTION
*     Destroys the data members of sge simple list of ssos_task_t elements.
*
*  INPUTS
*     ssos_task_t **ssos_task - The ssos_task_t element to destroy
*
*  RESULT
*     static bool -  Always true to continue the destruction of all list
*                    elements.
*
*  NOTES
*     MT-NOTE: destroy_slotwise_sos_task_elem() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
destroy_slotwise_sos_task_elem(ssos_task_t **ssos_task)
{
   if (ssos_task != NULL && *ssos_task != NULL) {
      FREE(*ssos_task);
   }
   return true;
}

/****** sge_subordinate_qmaster/destroy_slotwise_sos_tree_elem() ***************
*  NAME
*     destroy_slotwise_sos_tree_elem() -- Destructor for the elements of a
*                                         sge simple list of ssos_qinstance_t
*                                         elements
*
*  SYNOPSIS
*     static bool destroy_slotwise_sos_tree_elem(ssos_qinstance_t 
*     **ssos_qinstance) 
*
*  FUNCTION
*     Destroys the data members of a sge simple list of ssos_qinstance_t
*     elements. Takes care of the destruction of the sge simple list sublist of
*     ssos_task_t members.
*
*  INPUTS
*     ssos_qinstance_t **ssos_qinstance - The ssos_qinstance_t element to
*                                         destroy
*
*  RESULT
*     static bool - Always true to continue the destruction of all list
*                   elements.
*
*  NOTES
*     MT-NOTE: destroy_slotwise_sos_tree_elem() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static bool
destroy_slotwise_sos_tree_elem(ssos_qinstance_t **ssos_qinstance)
{
   if (ssos_qinstance != NULL && *ssos_qinstance != NULL) {
      if ((*ssos_qinstance)->tasks != NULL) {
         sge_sl_destroy(&((*ssos_qinstance)->tasks), (sge_sl_destroy_f)destroy_slotwise_sos_task_elem);
      }
      FREE(*ssos_qinstance);
   }
   
   return true;
}

/****** sge_subordinate_qmaster/get_task_to_x_in_depth() ***********************
*  NAME
*     get_task_to_x_in_depth() -- Searches the slotwise subordinate tree in a
*                                 specific depth for a task to (un)suspend
*
*  SYNOPSIS
*     static void get_task_to_x_in_depth(sge_sl_list_t 
*     *slotwise_sos_tree_qinstances, lUlong depth, bool running, bool suspend, 
*     ssos_qinstance_t **ssos_qinstance_to_x, ssos_task_t **ssos_task_to_x) 
*
*  FUNCTION
*     Searches in the provided slotwise subordination tree in a specific depth
*     for a task to (un)suspend.
*
*  INPUTS
*     sge_sl_list_t *slotwise_sos_tree_qinstances - The slotwise suspend on
*                                                   subordinate tree as a list
*     lUlong depth                                - The depth in this tree where
*                                                   the task is to be searched
*     bool running                                - Are we looking for running
*                                                   tasks or for suspended tasks?
*     bool suspend                                - Are we going to suspend or
*                                                   unsuspend a task?
*     ssos_qinstance_t **ssos_qinstance_to_x      - The queue instance where
*                                                   the found task is running
*     ssos_task_t **ssos_task_to_x                - The task we found
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: get_task_to_x_in_depth() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static void
get_task_to_x_in_depth(sge_sl_list_t *slotwise_sos_tree_qinstances,
      lUlong depth, bool running, bool suspend,
      ssos_qinstance_t **ssos_qinstance_to_x, ssos_task_t **ssos_task_to_x)
{
   sge_sl_elem_t    *ssos_tree_elem = NULL;
   lUlong           extreme_seq_no     = running==true ? 0 : (lUlong)-1;
   lUlong           oldest_start_time = (lUlong)-1;
   lUlong           youngest_start_time = 0;

   for_each_sl(ssos_tree_elem, slotwise_sos_tree_qinstances) {
      ssos_qinstance_t *ssos_qinstance = sge_sl_elem_data(ssos_tree_elem);
     
      /* Get highest/lowest seq_no of queues with running/suspended tasks
       * jobs among all queues of current depth
       */
      if (ssos_qinstance->depth == depth &&
          ssos_qinstance->tasks != NULL &&
          ((running == true && ssos_qinstance->seq_no > extreme_seq_no) ||
           (running == false && ssos_qinstance->seq_no < extreme_seq_no))) {
         extreme_seq_no = ssos_qinstance->seq_no;
      }
   }
   
   for_each_sl(ssos_tree_elem, slotwise_sos_tree_qinstances) {
      ssos_qinstance_t *ssos_qinstance = sge_sl_elem_data(ssos_tree_elem);
      
      
      /* Search in all queues in current depth, with highest/lowest seq_no
       * and with running/suspended tasks for the oldest/youngest task
       */
      if (ssos_qinstance->depth == depth &&
          ssos_qinstance->seq_no == extreme_seq_no) {
         sge_sl_elem_t *ssos_task_elem = NULL;
         bool oldest = ssos_qinstance->action == SO_ACTION_LR ? true : false;

         /* If we have to unsuspend and if we would look for the youngest job
          * for suspend, we have to look for the oldest job to unsuspend.
          */
         if (suspend == false) {
            oldest = (bool)!oldest;
         }

         for_each_sl(ssos_task_elem, ssos_qinstance->tasks) {
            lUlong      start_time = 0;
            ssos_task_t *ssos_task = sge_sl_elem_data(ssos_task_elem);

            start_time = lGetUlong(ssos_task->task, JAT_start_time);
            if (oldest == true && start_time < oldest_start_time) {
               oldest_start_time = start_time;
               *ssos_task_to_x = ssos_task;
               *ssos_qinstance_to_x = ssos_qinstance;
            }
            if (oldest == false && start_time > youngest_start_time) {
               youngest_start_time = start_time;
               *ssos_task_to_x = ssos_task;
               *ssos_qinstance_to_x = ssos_qinstance;
            }
         }
      }
   }
}

/****** sge_subordinate_qmaster/remove_task_from_slotwise_sos_tree() ***********
*  NAME
*     remove_task_from_slotwise_sos_tree() -- Removes a task from the slotwise
*                                             suspend on subordinate tree list
*
*  SYNOPSIS
*     static void remove_task_from_slotwise_sos_tree(sge_sl_list_t 
*     *slotwise_sos_tree_qinstances, lUlong job_id, lUlong task_id) 
*
*  FUNCTION
*     Removes a specific task from the slotwiese suspend on subordinate tree
*     list. For this, it searches the task in all queue instances of the
*     list.
*
*  INPUTS
*     sge_sl_list_t *slotwise_sos_tree_qinstances - The slotwise suspend on
*                                                   subordinate tree as a list
*     lUlong job_id                               - The job ID of the task
*     lUlong task_id                              - The task ID of the task
*
*  RESULT
*     static void - nonne 
*
*  NOTES
*     MT-NOTE: remove_task_from_slotwise_sos_tree() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
static void
remove_task_from_slotwise_sos_tree(sge_sl_list_t *slotwise_sos_tree_qinstances,
     lUlong job_id, lUlong task_id)
{
   sge_sl_elem_t    *ssos_tree_elem = NULL;
   sge_sl_elem_t    *ssos_task_elem = NULL;
   ssos_qinstance_t *ssos_qinstance = NULL;
   ssos_task_t      *ssos_task = NULL;

   for_each_sl(ssos_tree_elem, slotwise_sos_tree_qinstances) {
      ssos_qinstance = sge_sl_elem_data(ssos_tree_elem);
      for_each_sl(ssos_task_elem, ssos_qinstance->tasks) {
         ssos_task = sge_sl_elem_data(ssos_task_elem);
         if (ssos_task->job_id == job_id &&
             lGetUlong(ssos_task->task, JAT_task_number) == task_id) {
            sge_sl_dechain(ssos_qinstance->tasks, ssos_task_elem);
            if (sge_sl_get_elem_count(ssos_qinstance->tasks) == 0) {
               sge_sl_destroy(&(ssos_qinstance->tasks), NULL);
            }
            sge_sl_elem_destroy(&ssos_task_elem, (sge_sl_destroy_f)destroy_slotwise_sos_task_elem);
         }
      }
   }
}

static bool
x_most_extreme_task(sge_gdi_ctx_class_t *ctx, sge_sl_list_t *slotwise_sos_tree_qinstances,
      bool suspend, monitoring_t *monitor)
{
   bool             suspended_a_task = false;
   lUlong           depth = 0;
   lUlong           i;
   sge_sl_elem_t    *ssos_tree_elem = NULL;
   ssos_qinstance_t *ssos_qinstance = NULL;
   ssos_task_t      *ssos_task = NULL;

   /* Walk over the whole list and find biggest depth */
   for_each_sl(ssos_tree_elem, slotwise_sos_tree_qinstances) {
      ssos_qinstance = sge_sl_elem_data(ssos_tree_elem);
      depth = MAX(ssos_qinstance->depth, depth);
   }

   ssos_qinstance = NULL;
   if (suspend == true) {
      /* Walk over list, get oldest (youngest) job from qinstances of biggest depth */
      /* If there was no running job in one of the qinstances of biggest depth, repeat
       * with qinstances of max_depth-1, and so on.
       */
      for (i=depth; i>0 && ssos_task==NULL; i--) {
         /* find youngest running task to suspend */
         get_task_to_x_in_depth(slotwise_sos_tree_qinstances, i, true, suspend,
               &ssos_qinstance, &ssos_task);
      }
   } else {
      for (i=1; i<=depth && ssos_task==NULL; i++) {
         /* find oldest suspended task to unsuspend */
         get_task_to_x_in_depth(slotwise_sos_tree_qinstances, i, false, suspend,
               &ssos_qinstance, &ssos_task);
      }
   }

   if (ssos_task != NULL && ssos_qinstance != NULL) {
       /* (un)suspend this task */
       suspended_a_task = slotwise_x_on_subordinate(ctx, ssos_qinstance->qinstance,
                     ssos_task->job_id, lGetUlong(ssos_task->task, JAT_task_number),
                     suspend, monitor);
       if (suspended_a_task == true) {
          remove_task_from_slotwise_sos_tree(slotwise_sos_tree_qinstances, ssos_task->job_id,
                lGetUlong(ssos_task->task, JAT_task_number));
       }
   }
   return suspended_a_task;
}

static lUlong
get_slotwise_sos_threshold(lListElem *qinstance)
{
   lUlong    slots_sum = 0;
   lList     *so_list = NULL;
   lListElem *so = NULL;

   if (qinstance != NULL) {
      so_list = lGetList(qinstance, QU_subordinate_list);
      if (so_list != NULL) {
         so = lFirst(so_list);
         if (so != NULL) {
            slots_sum = lGetUlong(so, SO_slots_sum);
         }
      }
   }
   return slots_sum;
}

static void
get_slotwise_sos_sub_tree_qinstances(lListElem *qinstance,
      sge_sl_list_t **tree_qinstances, lUlong depth)
{
   lList     *so_list = NULL;
   lList     *cqueue_master_list = *object_type_get_master_list(SGE_TYPE_CQUEUE);
   lListElem *so = NULL;
   lListElem *sub_qinstance = NULL;

   /* get all qinstances in the slotwise sos tree excluding the super qinstance
    * (i.e. the root node of the tree), it was already added in the iteration before.
    */
   if (depth == 0) {
      ssos_qinstance_t *ssos_qinstance = NULL;

      if (tree_qinstances == NULL || *tree_qinstances != NULL) {
         return;
      }

      /* special handling for the root node of the tree */
      sge_sl_create(tree_qinstances);

      /* first add the super qinstance to the list */
      ssos_qinstance = (ssos_qinstance_t*)calloc(1, sizeof(ssos_qinstance_t));
      ssos_qinstance->seq_no    = 0;    /* the super qinstance has always top priority */
      ssos_qinstance->depth     = 0;    /* the super qinstance is on top */
      ssos_qinstance->action    = 0;    /* the super qinstances tasks don't get modified */
      ssos_qinstance->qinstance = qinstance;
      ssos_qinstance->parent    = NULL; /* the super qinstance has no parent */
      ssos_qinstance->tasks     = NULL; /* gets filled later */ 

      sge_sl_insert(*tree_qinstances, ssos_qinstance, SGE_SL_FORWARD);
      depth++;
   }

   so_list = lGetList(qinstance, QU_subordinate_list);
   for_each(so, so_list) {
      const char *so_name = NULL;
      const char *so_full_name = NULL;
      dstring dstr_so_full_name = DSTRING_INIT;
      ssos_qinstance_t *ssos_qinstance = NULL;
     
      /* get the pointer to this subordinated qinstance list elem */
      so_name = lGetString(so, SO_name);
      if (strstr(so_name, "@") == NULL) {
         const char *host_name = NULL;

         host_name = lGetHost(qinstance, QU_qhostname);
         sge_dstring_sprintf(&dstr_so_full_name, "%s@%s", so_name, host_name);
         so_full_name = sge_dstring_get_string(&dstr_so_full_name);
      } else {
         so_full_name = so_name;
      }
      sub_qinstance = cqueue_list_locate_qinstance(cqueue_master_list, so_full_name);
      sge_dstring_free(&dstr_so_full_name);

      if (sub_qinstance != NULL) {
          ssos_qinstance = (ssos_qinstance_t*)calloc(1, sizeof(ssos_qinstance_t));
          ssos_qinstance->seq_no    = lGetUlong(so, SO_seq_no);
          ssos_qinstance->action    = lGetUlong(so, SO_action);
          ssos_qinstance->depth     = depth;
          ssos_qinstance->qinstance = sub_qinstance;
          ssos_qinstance->parent    = qinstance;
          ssos_qinstance->tasks     = NULL; /* gets filled later */

          sge_sl_insert(*tree_qinstances, ssos_qinstance, SGE_SL_FORWARD);

          get_slotwise_sos_sub_tree_qinstances(sub_qinstance, tree_qinstances, depth+1);
      }
   }
}

static lUlong
count_running_jobs_in_slotwise_sos_tree(sge_sl_list_t *qinstances_in_slotwise_sos_tree,
      bool suspend)
{
   /* Walk over job list and get the tasks that are running in the qinstances
    * of the slotwise sos tree. Store informations about these tasks in the tree list.
    */
   lList *master_job_list = NULL;
   lListElem *job = NULL;
   lUlong sum = 0;

   if (qinstances_in_slotwise_sos_tree != NULL &&
       sge_sl_get_elem_count(qinstances_in_slotwise_sos_tree) > 0) {
      const char *host_name = NULL;
      ssos_qinstance_t *first_qinstance = NULL;

      sge_sl_data(qinstances_in_slotwise_sos_tree, (void**)&first_qinstance, SGE_SL_FORWARD);
      host_name = lGetHost(first_qinstance->qinstance, QU_qhostname);

      master_job_list = *object_type_get_master_list(SGE_TYPE_JOB);

      for_each(job, master_job_list) {
         lList     *task_list = NULL;
         lListElem *task = NULL;

         task_list = lGetList(job, JB_ja_tasks);
         for_each(task, task_list) {
            lListElem *task_gdi = NULL;
            lList     *task_gdi_list = NULL;

            task_gdi_list = lGetList(task, JAT_granted_destin_identifier_list);
            for_each(task_gdi, task_gdi_list) {
               lUlong state = 0;
               const char *task_gdi_qname = NULL;
               const char *qinstance_name = NULL;
               const char *task_gdi_qhost_name = NULL;
               sge_sl_elem_t *sl_elem = NULL;

               /* Only compare tasks on the right host */
               task_gdi_qhost_name = lGetHost(task_gdi, JG_qhostname);
/* TODO: HP: Use lGetElemHostFirst()/lGetElemHostNext() instead */
               if (strcmp(task_gdi_qhost_name, host_name) == 0) {
                  /* Is this task in state JRUNNING? We don't look for other tasks.
                   */
                  state = lGetUlong(task, JAT_state);
                  if (ISSET(state, JRUNNING) == true &&
                      ISSET(state, JSUSPENDED) == false &&
                      ISSET(state, JSUSPENDED_ON_THRESHOLD) == false &&
                      ISSET(state, JSUSPENDED_ON_SUBORDINATE) == false &&
                      ISSET(state, JSUSPENDED_ON_SLOTWISE_SUBORDINATE) == false) {
                     task_gdi_qname = lGetString(task_gdi, JG_qname);

                     /* Check if the qinstance name where the task is running is
                      * in the list of qinstances in the slotwise sos tree.
                      */
                     for_each_sl(sl_elem, qinstances_in_slotwise_sos_tree) {
                        ssos_qinstance_t *ssos_qinstance = NULL;

                        ssos_qinstance = sge_sl_elem_data(sl_elem);
                        qinstance_name = lGetString(ssos_qinstance->qinstance, QU_full_name);
                        if (strcmp(task_gdi_qname, qinstance_name) == 0) {
                           sum++;
                           /* We always count the running task, but we only store them
                            * when we have to suspend job. If we have to unsuspend jobs,
                            * we store the suspended jobs (the below).
                            */
                           if (suspend == true) {
                              ssos_task_t *ssos_task = (ssos_task_t*)calloc(1, sizeof(ssos_task_t));

                              if (ssos_qinstance->tasks == NULL) {
                                 sge_sl_create(&(ssos_qinstance->tasks));
                              }
                             
                              ssos_task->job_id = lGetUlong(job, JB_job_number);
                              ssos_task->task   = task;
                              sge_sl_insert(ssos_qinstance->tasks, ssos_task, SGE_SL_FORWARD);
                              break;
                           }
                        }
                     }
                  } else if (ISSET(state, JSUSPENDED_ON_SLOTWISE_SUBORDINATE) == true &&
                             ISSET(state, JSUSPENDED) == false &&
                             ISSET(state, JSUSPENDED_ON_THRESHOLD) == false &&
                             ISSET(state, JSUSPENDED_ON_SUBORDINATE) == false &&
                             suspend == false) {
                     /* JSUSPENDED_ON_SLOTWISE_SUBORDINATE is set and cleared by the ssos logic only,
                      * JSUSPENDED_ON_SUBORDINATE is set by queue-wise subordination, and so on.
                      */
                     task_gdi_qname = lGetString(task_gdi, JG_qname);

                     /* Check if the qinstance name where the task is slotwise suspended is
                      * in the list of qinstances in the slotwise sos tree.
                      */
                     for_each_sl(sl_elem, qinstances_in_slotwise_sos_tree) {
                        ssos_qinstance_t *ssos_qinstance = NULL;

                        ssos_qinstance = sge_sl_elem_data(sl_elem);
                        qinstance_name = lGetString(ssos_qinstance->qinstance, QU_full_name);
                        if (strcmp(task_gdi_qname, qinstance_name) == 0) {
                           ssos_task_t *ssos_task = (ssos_task_t*)calloc(1, sizeof(ssos_task_t));
                           if (ssos_qinstance->tasks == NULL) {
                              sge_sl_create(&(ssos_qinstance->tasks));
                           }
                          
                           ssos_task->job_id = lGetUlong(job, JB_job_number);
                           ssos_task->task   = task;
                           sge_sl_insert(ssos_qinstance->tasks, ssos_task, SGE_SL_FORWARD);
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }
   return sum;
}

static bool
is_slotwise_sos_super_qinstance(lListElem *qinstance)
{
   bool  ret = false;
   lList *so_list = NULL;

   so_list = lGetList(qinstance, QU_subordinate_list);
   if (so_list != NULL) {
      if (lGetUlong(lFirst(so_list), SO_slots_sum) > 0) {
         ret = true;
      }
   }
   return ret;
}

/*
 * called_by_qmod: If this function call was not triggered by qmod/qconf but by
 * job end, the job that just ended is still in the job list and must be subtracted.
 * TODO: Can we detect this automatically by the job states?
 */
bool
do_slotwise_x_on_subordinate_check(sge_gdi_ctx_class_t *ctx, lListElem *qinstance,
      bool suspend, bool called_by_qmod, monitoring_t *monitor)
{
   sge_sl_list_t *qinstances_in_slotwise_sos_tree = NULL;
   lListElem *super_qinstance = NULL;
   lListElem *super_super = NULL;
   lUlong    running_jobs = 0;
   lUlong    slots_sum    = 0;

   if (suspend == true) {
      /* Always check a sub tree from a tree node, don't do checking from 
       * a leaf node.
       */
      if (is_slotwise_sos_super_qinstance(qinstance) == false) {
         super_qinstance = get_slotwise_sos_super_qinstance(qinstance);
      } else {
         super_qinstance = qinstance;
      }
   } else {
      super_qinstance = get_slotwise_sos_tree_root(qinstance);
   }

   if (super_qinstance == NULL) {
      return false; 
   }

   slots_sum = get_slotwise_sos_threshold(super_qinstance);
   if (slots_sum == 0) {
      /* no slotwise suspend on subordinate! */
      return false;
   }

   /* get the slotwise sos tree as a list */
   get_slotwise_sos_sub_tree_qinstances(super_qinstance, &qinstances_in_slotwise_sos_tree, 0);

   /* count the number and store informations about all running tasks in the list */
   running_jobs = count_running_jobs_in_slotwise_sos_tree(qinstances_in_slotwise_sos_tree, suspend);
   if ((suspend == true  && running_jobs > slots_sum) ||
       (suspend == false &&
          (called_by_qmod == true ? running_jobs < slots_sum : running_jobs <= slots_sum))) {
      bool   ret = false;
      int    diff = 0;
      /* we have to (un)suspend as many running/suspended jobs as new jobs
       * were scheduled/finished or (un)suspended by other ways.
       */
      diff = running_jobs > slots_sum ? running_jobs - slots_sum : slots_sum - running_jobs;
      do {
         /* suspend/unsuspend the highest/lowest running/suspended task */
         ret = x_most_extreme_task(ctx, qinstances_in_slotwise_sos_tree, suspend, monitor);
      } while (ret == true && (--diff) > 0);
   } 
   sge_sl_destroy(&qinstances_in_slotwise_sos_tree, (sge_sl_destroy_f)destroy_slotwise_sos_tree_elem);

   if (suspend == true) {
      /* Walk the tree from the leaves to the root */
      super_super = get_slotwise_sos_super_qinstance(super_qinstance);
      if (super_super != NULL) {
         do_slotwise_x_on_subordinate_check(ctx, super_super, suspend, called_by_qmod, monitor);
      }
   }
   return true;
}

/*
   (un)suspend on subordinate using granted_destination_identifier_list

   NOTE:
      we assume the associated job is already/still
      debited on all the queues that are referenced in gdil
*/
bool
cqueue_list_x_on_subordinate_gdil(sge_gdi_ctx_class_t *ctx,
                                  lList *this_list, bool suspend,
                                  const lList *gdil, monitoring_t *monitor)
{
   bool ret = true;
   lListElem *gdi = NULL;

   DENTER(TOP_LAYER, "cqueue_list_x_on_subordinate_gdil");

   for_each(gdi, gdil) {
      const char *full_name = lGetString(gdi, JG_qname);
      const char *hostname = lGetHost(gdi, JG_qhostname);
      lListElem *queue = cqueue_list_locate_qinstance(this_list, full_name);

      if (queue != NULL) {
         lList *so_list = lGetList(queue, QU_subordinate_list);
         lList *resolved_so_list = NULL;
         lListElem *so = NULL;
         u_long32 slots = lGetUlong(queue, QU_job_slots);
         u_long32 slots_used = qinstance_slots_used(queue);
         u_long32 slots_granted = lGetUlong(gdi, JG_slots);
         bool slotwise = false;

         slotwise = do_slotwise_x_on_subordinate_check(ctx, queue, suspend, false, monitor);
         if (slotwise == false) {
            /* Do queue wise suspend on subordinate */
            /*
             * Resolve cluster queue names into qinstance names
             */
            so_list_resolve(so_list, NULL, &resolved_so_list, NULL, hostname);

            for_each(so, resolved_so_list) {
               const char *so_queue_name = lGetString(so, SO_name);
               
               /* We have to check this because so_list_resolve() didn't. */
               if (strcmp(full_name, so_queue_name) == 0) {
                  /* Queue can't be subordinate to itself. */
                  DPRINTF (("Removing circular reference.\n"));
                  continue;
               }
                  
               /*
                * suspend:
                *    no sos before this job came on this queue AND
                *    sos since job is on this queue
                *
                * unsuspend:
                *    no sos after job gone from this queue AND
                *    sos since job is on this queue
                */
               if (!tst_sos(slots_used - slots_granted, slots, so) &&
                   tst_sos(slots_used, slots, so)) {
                  lListElem *so_queue =               
                            cqueue_list_locate_qinstance(this_list, so_queue_name);

                  if (so_queue != NULL) {
                     /* Suspend/unsuspend the subordinated queue instance */
                     ret &= qinstance_x_on_subordinate(ctx, so_queue, suspend, true, monitor);
                     /* This change could also trigger slotwise (un)suspend on
                      * subordinate in related queue instances. If it was a
                      * queuewise suspend, it must be a slotwise unsuspend,
                      * and vice versa.
                      */
                     do_slotwise_x_on_subordinate_check(ctx, so_queue, (bool)!suspend, false, monitor);
                  } else {
                     ERROR((SGE_EVENT, MSG_QINSTANCE_NQIFOUND_SS, 
                            so_queue_name, SGE_FUNC));
                     ret = false;
                  }
               }
            }
            lFreeList(&resolved_so_list);
         }
      } else {
         /* should never happen */
         ERROR((SGE_EVENT, MSG_QINSTANCE_NQIFOUND_SS, full_name, SGE_FUNC));
         ret = false;
      } 
   }
   DRETURN(ret);
}

static bool
qinstance_x_on_subordinate(sge_gdi_ctx_class_t *ctx,
                           lListElem *this_elem, bool suspend,
                           bool send_event, monitoring_t *monitor)
{
   bool ret = true;
   u_long32 sos_counter;
   bool do_action;
   bool send_qinstance_signal;
   const char *hostname;
   const char *cqueue_name;
   const char *full_name;
   int signal;
   ev_event event;

   DENTER(TOP_LAYER, "qinstance_x_on_subordinate");

   /* increment sos counter */
   sos_counter = lGetUlong(this_elem, QU_suspended_on_subordinate);
   if (suspend) {
      sos_counter++;
   } else {
      sos_counter--;
   }
   lSetUlong(this_elem, QU_suspended_on_subordinate, sos_counter);

   /* 
    * prepare for operation
    *
    * suspend:  
    *    send a signal if it is not already suspended by admin or calendar 
    *
    * !suspend:
    *    send a signal if not still suspended by admin or calendar
    */
   hostname = lGetHost(this_elem, QU_qhostname);
   cqueue_name = lGetString(this_elem, QU_qname);
   full_name = lGetString(this_elem, QU_full_name);
   send_qinstance_signal = (qinstance_state_is_manual_suspended(this_elem) ||
                            qinstance_state_is_cal_suspended(this_elem)) ? false : true;
   if (suspend) {
      do_action = (sos_counter == 1) ? true : false;
      signal = SGE_SIGSTOP;
      event = sgeE_QINSTANCE_SOS;
   } else {
      do_action = (sos_counter == 0) ? true : false;
      signal = SGE_SIGCONT;
      event = sgeE_QINSTANCE_USOS;
   }

   /*
    * do operation
    */
   DPRINTF(("qinstance "SFQ" "SFN" "SFN" on subordinate\n", full_name,
            (do_action ? "" : "already"),
            (suspend ? "suspended" : "unsuspended")));
   if (do_action) {
      DPRINTF(("Due to other suspend states signal will %sbe delivered\n",
               send_qinstance_signal ? "NOT " : "")); 
      if (send_qinstance_signal) {
         ret = (sge_signal_queue(ctx, signal, this_elem, NULL, NULL, monitor) == 0) ? true : false;
      }

      sge_qmaster_qinstance_state_set_susp_on_sub(this_elem, suspend);
      if (send_event) {
         sge_add_event(0, event, 0, 0, cqueue_name, hostname, NULL, NULL);
      }
      lListElem_clear_changed_info(this_elem);
   }
   DRETURN(ret);
}

bool
cqueue_list_x_on_subordinate_so(sge_gdi_ctx_class_t *ctx,
                                lList *this_list, lList **answer_list,
                                bool suspend, const lList *resolved_so_list,
                                monitoring_t *monitor)
{
   bool ret = true;
   const lListElem *so = NULL;

   DENTER(TOP_LAYER, "cqueue_list_x_on_subordinate_qref");

   /*
    * Locate all qinstances which are mentioned in resolved_so_list and 
    * (un)suspend them
    */
   for_each(so, resolved_so_list) {
      const char *full_name = lGetString(so, SO_name);
      lListElem *qinstance = cqueue_list_locate_qinstance(this_list, full_name);

      if (qinstance != NULL) {
         ret &= qinstance_x_on_subordinate(ctx, qinstance, suspend,
                                           true, monitor);
         if (!ret) {
            break;
         }
      }
   }
   DRETURN(ret);
}

bool
qinstance_find_suspended_subordinates(const lListElem *this_elem,
                                      lList **answer_list,
                                      lList **resolved_so_list)
{
   /* Return value */
   bool ret = true;

   DENTER(TOP_LAYER, "qinstance_find_suspended_subordinates");
   
   if (this_elem != NULL && resolved_so_list != NULL) {
      /* Temporary storage for subordinates */
      lList *so_list = lGetList(this_elem, QU_subordinate_list);
      lListElem *so = NULL;
      lListElem *next_so = NULL;
      const char *hostname = lGetHost(this_elem, QU_qhostname);
      /* Slots calculations */
      u_long32 slots = lGetUlong(this_elem, QU_job_slots);
      u_long32 slots_used = qinstance_slots_used(this_elem);
      /*
       * Resolve cluster queue names into qinstance names
       */
      so_list_resolve(so_list, answer_list, resolved_so_list, NULL,
                      hostname);
      /* 
       * If the number of used slots on this qinstance is greater than a
       * subordinate's threshold (if it has one), this subordinate should
       * be suspended.
       *
       * Remove all subordinated queues from "resolved_so_list" which
       * are not actually suspended by "this_elem" 
       */
      DTRACE;
      next_so = lFirst(*resolved_so_list);
      while ((so = next_so) != NULL) {
         next_so = lNext(so);
         if (!tst_sos(slots_used, slots, so)) {
            DPRINTF (("Removing %s because it's not suspended\n",
                      lGetString (so, SO_name)));
            lRemoveElem(*resolved_so_list, &so);
         }
      }
   }

   DRETURN(ret);
}

bool
qinstance_initialize_sos_attr(sge_gdi_ctx_class_t *ctx, lListElem *this_elem, monitoring_t *monitor) 
{
   bool ret = true;
   lListElem *cqueue = NULL;
   lList *master_list = NULL;
   const char *full_name = NULL;
   const char *qinstance_name = NULL;
   const char *hostname = NULL;
   /* Slots calculations */
   u_long32 slots = 0;
   u_long32 slots_used = 0;

   DENTER(TOP_LAYER, "qinstance_initialize_sos_attr");
   
   master_list = *(object_type_get_master_list(SGE_TYPE_CQUEUE));
   full_name = lGetString(this_elem, QU_full_name);
   qinstance_name = lGetString(this_elem, QU_qname);
   hostname = lGetHost(this_elem, QU_qhostname);
   
   for_each(cqueue, master_list) {
      lList *qinstance_list = lGetList(cqueue, CQ_qinstances);
      lListElem *qinstance = NULL; 

      qinstance = lGetElemHost(qinstance_list, QU_qhostname, hostname);
      if (qinstance != NULL) {
         lList     *so_list = lGetList(qinstance, QU_subordinate_list);
         lListElem *so = NULL;
         lList     *resolved_so_list = NULL;
         u_long32  slots_sum = 0;

         if (so_list != NULL && (so = lFirst(so_list)) != NULL) {
            slots_sum = lGetUlong(so, SO_slots_sum);
         }

         if (slots_sum > 0) {
            do_slotwise_x_on_subordinate_check(ctx, this_elem, true, true, monitor);
/* TODO: HP: It seems this is never called. */
#if 0
            /* slot-wise suspend on subordinate */
            lUlong slots_used = 0;
            char   *qinstance_name = NULL;
            lListElem *qinstance_where_task_is_running = NULL;
            lList     *cqueue_master_list = *object_type_get_master_list(SGE_TYPE_CQUEUE);
            sge_sl_list_t *tasks_to_x = NULL;
            sge_sl_elem_t *task = NULL;

            /*
             * Resolve cluster queue names into qinstance names
             */
            so_list_resolve(so_list, NULL, &resolved_so_list, qinstance_name,
                            hostname);

            slots_used = get_nr_of_running_tasks_and_the_tasks_to_x(resolved_so_list,
                  this_elem, true, &tasks_to_x);

            for_each_sl(task, tasks_to_x) {
               job_location_t *job_location = sge_sl_elem_data(task);

               qinstance_where_task_is_running = cqueue_list_locate_qinstance(
                      cqueue_master_list, job_location->qinstance_name);

               if (slots_used > slots_sum) {
                  slotwise_x_on_subordinate(ctx, qinstance_where_task_is_running,
                        job_location->job_id, job_location->jatask, true, false, monitor);
               }
            }
            sge_sl_destroy(&tasks_to_x, (sge_sl_destroy_f)destroy_job_location_list);
#endif
         } else {
            /* queue instance-wise suspend on subordinate */
            slots = lGetUlong(qinstance, QU_job_slots);
            slots_used = qinstance_slots_used(qinstance);

            /*
             * Resolve cluster queue names into qinstance names
             */
            so_list_resolve(so_list, NULL, &resolved_so_list, qinstance_name,
                            hostname);

            for_each(so, resolved_so_list) {
               const char *so_full_name = lGetString(so, SO_name);

               if (!strcmp(full_name, so_full_name)) {
                  /* suspend the queue if neccessary */
                  if (tst_sos(slots_used, slots, so)) {
                     qinstance_x_on_subordinate(ctx, this_elem, true, false, monitor); 
                  }
               } 
            }
         }
         lFreeList(&resolved_so_list);
      }
   }
   DRETURN(ret);
}
