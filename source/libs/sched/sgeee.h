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
/*----------------------------------------------------
 *
 */

typedef struct {
    lListElem *job;		/* job reference */
    lListElem *ja_task; /* task reference */
    lListElem *user;		/* user reference */
    lListElem *project;		/* project reference */
    lListElem *dept;		/* department reference */
    lListElem *jobclass;	/* job class reference */
    lListElem *node;		/* node reference */
    int num_task_jobclasses;    /* number of task job classes */
    lListElem **task_jobclass;  /* task job class reference array */
    u_long32  share_tree_type; /* share tree type */
    double    total_jobclass_ftickets;
    double    total_jobclass_otickets;
} sge_ref_t ;

#define SGE_USAGE_INTERVAL 60

int sge_calc_tickets(sge_Sdescr_t *lists, lList *queued_jobs, lList *running_jobs, lList *finished_jobs, int do_usage);

void sge_setup_lists(sge_Sdescr_t *lists, lList *queued_jobs, lList *running_jobs);

void sge_job_active(lListElem *job, sge_Sdescr_t *lists);

void sge_job_inactive(lListElem *job, sge_Sdescr_t *lists);

void sge_dump_list(lList *list);

void _sge_calc_share_tree_proportions(lList *share_tree, lList *user_list, lList *project_list, lList *config_list, u_long curr_time);

void sge_calc_share_tree_proportions(lList *share_tree, lList *user_list, lList *project_list, lList *config_list);

void dump_list_to_file(lList *list, char *file);

typedef int (*sge_node_func_t)(lListElem *node, void *ptr);

int sge_for_each_share_tree_node(lListElem *node, sge_node_func_t func, void *ptr);

int sge_zero_node_fields(lListElem *node, void *ptr);

int sge_init_node_fields(lListElem *root);

void sge_clear_job(lListElem *job);

void sge_clear_ja_task(lListElem *ja_task);

typedef struct {
   int depth;
   lListElem **nodes;
} ancestors_t;

#ifdef notdef

lListElem *search_ancestor_list(lListElem *ep, char *name, ancestors_t *ancestors);
#endif

lListElem *search_named_node(lListElem *ep, char *name);

lListElem *search_named_node_path(lListElem *ep, char *path, ancestors_t *ancestors);

void free_ancestors(ancestors_t *ancestors);

lListElem *search_userprj_node(lListElem *ep, char *username, char *projname, lListElem **pep);

void calculate_decay_constant(int halftime);

void decay_userprj_usage(lListElem *userprj, u_long seqno, u_long curr_time);

lList *sge_build_sge_orders(sge_Sdescr_t *lists, lList *running_jobs, lList *finished_jobs, lList *order_list, int seqno);
int sge_scheduler(sge_Sdescr_t *lists, lList *running_jobs, lList *finished_jobs, lList **orderlist);

int calculate_host_tickets(lList **running, lList **hosts);

int  sort_host_list_by_share_load(lList *host_list, lList *complex_list);

void print_job_ref_array(sge_ref_t *ref_list, int max_elem);

void sge_sort_jobs(lList **job_list);

#endif /* __SGEEE_H */






