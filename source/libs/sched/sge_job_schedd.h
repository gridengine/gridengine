#ifndef __SGE_JOB_SCHEDD_H
#define __SGE_JOB_SCHEDD_H
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



#define running_status(x) ((x)!=0 && (x)!=JFINISHED)

/* 
 * get order used for job sorting
 */
lSortOrder *sge_job_sort_order(const lDescr *dp);

int up_resort(lList **ulpp, lListElem *job, lList *job_list, lSortOrder *so);

void sge_inc_jc(lList** jcpp, char *name, int slots);

void sge_dec_jc(lList** jcpp, char *name, int slots);

int rebuild_jc(lList **jcpp, lList *job_list);

int resort_jobs(lList *jc, lList *job_list, char *owner, lSortOrder *so);

int set_user_sort(int foo);

/*
 * drop all running jobs into the running list 
 *
 */
int sge_split_job_running(lList **jobs, lList **running, char *running_name);

/*
 * drop all finished jobs into the finished list 
 *
 */
int sge_split_job_finished(lList **jobs, lList **finished, char *finished_name);

/*
 * drop all jobs waiting for -a into the waiting list 
 *
 */
int sge_split_job_wait_at_time(lList **jobs, lList **waiting, char *waiting_name, u_long32 now);

/*
 * drop all jobs in error state
 *
 */
int sge_split_job_error(lList **jobs, lList **error, char *error_name);

/*
 * drop all jobs in hold state
 *
 */
int sge_split_job_hold(lList **jobs, lList **hold, char *hold_name);

/*
 * drop all jobs waiting for a predecessor into the waiting list 
 *
 */
int sge_split_job_wait_predecessor(lList **jobs, lList **waiting, char *waiting_name);

/*
 * drop all jobs restricted by a ckpt environment
 *
 */
int sge_split_job_ckpt_restricted(lList **jobs, lList **restricted, char *restricted_name, lList *ckpt_list);

void print_job_list(lList *job_list);

lList *filter_max_running_1step(lList *pending_jobs, lList *running_jobs, lList **jct_list, int max_jobs, int elem);

lList *filter_max_running(lList *pending_jobs, lList *jct_list, int max_jobs, int elem);

void trace_job_sort(lList *job_list);
int get_job_contribution(double *dvalp, char *name, lListElem *jep, lListElem *dcep);
int nslots_granted(lList *granted, char *qhostname);
int active_subtasks(lListElem *job, char *qname);
int active_nslots_granted(lListElem *job, lList *granted, char *qhostname);
lListElem *explicit_job_request(lListElem *jep, char *name);
int sge_granted_slots(lList *gdil);

#endif /* __SGE_JOB_SCHEDD_H */

