#ifndef __SGEEE_H
#define __SGEEE_H
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

#include "sge_boundaries.h"
#include "cull.h"
#include "sge_schedd.h"
#include "scheduler.h"

/* 
 * Following fields are necessary for tasks which are not
 * enrolled in the JB_ja_tasks-list. For these jobs we have no
 * reference to the 'ja_task'-CULL-element. 
 */ 
typedef struct {
   u_long32 job_number;       /* job number */
   u_long32 ja_task_number;   /* ja task id */
   double ja_task_fticket;    /* ftickets for task 'ja_task_id' */ 
   double ja_task_sticket;    /* stickets for task 'ja_task_id' */ 
   double ja_task_dticket;    /* dtickets for task 'ja_task_id' */ 
   double ja_task_oticket;    /* otickets for task 'ja_task_id' */ 
   double ja_task_ticket;     /* tickets for task 'ja_task_id' */ 
   double ja_task_share;      /* share for task 'ja_task_id' */
   u_long32 ja_task_fshare;   /* fshare for task 'ja_task_id' */
} sge_task_ref_t;

/*
 * sge_ref_t - this structure is used in the SGEEE
 * scheduler to keep references to all the objects
 * associated with a job.  An array is built containing
 * one of these structures for each job/array we
 * are scheduling.
 */

typedef struct {
   lListElem *job;		      /* job reference */
   lListElem *ja_task;        /* task reference */
   lListElem *user;		      /* user reference */
   lListElem *project;		   /* project reference */
   lListElem *dept;		      /* department reference */
   lListElem *jobclass;	      /* job class reference */
   lListElem *node;		      /* node reference */
   int queued;                /* =1 if job is a queued job */
   int num_task_jobclasses;   /* number of task job classes */
   lListElem **task_jobclass; /* task job class reference array */
   u_long32  share_tree_type; /* share tree type */
   double total_jobclass_ftickets;
   double total_jobclass_otickets;
   double user_fshare;        /* job's share of user functional shares */
   double dept_fshare;        /* job's share of department functional shares */
   double project_fshare;     /* job's share of project functional shares */
   double jobclass_fshare;    /* job's share of jobclass functional shares */
   double job_fshare;         /* job's share of job functional shares */
   sge_task_ref_t *tref;
} sge_ref_t;

#define SGE_USAGE_INTERVAL 60

int sge_calc_tickets (sge_Sdescr_t *lists,
                      lList *running_jobs,
                      lList *finished_jobs,
	          	       lList *queued_jobs,
                      int do_usage );

void sge_setup_lists ( sge_Sdescr_t *lists,
                       lList *queued_jobs,
                       lList *running_jobs );

void sge_job_active ( lListElem *job,
                      sge_Sdescr_t *lists );

void sge_job_inactive ( lListElem *job,
                        sge_Sdescr_t *lists );

void sge_dump_list ( lList *list );

void dump_list_to_file ( lList *list, const char *file );

void sge_clear_job ( lListElem *job );

void sge_clear_ja_task ( lListElem *ja_task );

lList *sge_build_sge_orders ( sge_Sdescr_t *lists,
                              lList *running_jobs,
                              lList *queued_jobs,
                              lList *finished_jobs,
                              lList *order_list,
                              int update_usage_and_configuration,
                              int seqno );

int sge_scheduler ( sge_Sdescr_t *lists,
                    lList *running_jobs,
                    lList *finished_jobs,
                    lList *pending_jobs,
                    lList **orderlist );

int calculate_host_tickets ( lList **running, lList **hosts );

int  sort_host_list_by_share_load ( lList *host_list,       /* EH_Type */
                                    lList *complex_list );  /* CX_Type */

void print_job_ref_array ( sge_ref_t* ref_list,
                           int max_elem );

void sgeee_resort_pending_jobs(lList **job_list); 
 
#if 0 /* EB: debug */

void task_ref_print_table(void);

void task_ref_print_table_entry(sge_task_ref_t *tref);

#endif

sge_task_ref_t *task_ref_get_first(u_long32 job_number,
                                   u_long32 ja_task_number);

sge_task_ref_t *task_ref_get_first_job_entry(void);

sge_task_ref_t *task_ref_get_next_job_entry(void);

void task_ref_copy_to_ja_task(sge_task_ref_t *tref, lListElem *ja_task);

#endif /*  __SGEEE_H */

