#ifndef _SGE_ACCESS_TREE_H_
#define _SGE_ACCESS_TREE_H_
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

/*
** ------------ to be called from within the data model layer 
*/

/* needed to build up sort orders */
int at_init(void);
void at_finish(void);


/* register a new job array which has tasks in runnable state */
void at_register_job_array(lListElem *job_array);

/* unregister a formerly runnable job array */
void at_unregister_job_array(lListElem *job_array);

/* notify access tree of jobs state transitions between 
   running/pending per user and per priority group */
void at_inc_job_counter(u_long32 priority, char *owner, int slots);
void at_dec_job_counter(u_long32 priority, char *owner, int slots);

/*
** ------------ to be called from within the decision-making layer 
*/

/* tell the access tree which job arrays are to be taken into respect */
void at_notice_runnable_job_arrays(lList *job_list);

/* get the actual job array to be scheduled */
lListElem *at_get_actual_job_array(void);

/* a task has been dispatched - notify the access tree of this event */
void at_dispatched_a_task(lListElem *job, int slots);

/* a job array  has been dispatched completely */
void at_finished_array_dispatching(lListElem *job);

#endif /* _SGE_ACCESS_TREE_H_ */
