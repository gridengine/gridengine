#ifndef __SGE_JOB_JATASK_H 
#define __SGE_JOB_JATASK_H 
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

#include "sge_htable.h"
#include "sge_dstring.h"

int job_is_enrolled(const lListElem *job, 
                    u_long32 ja_task_number);

u_long32 job_get_ja_tasks(const lListElem *job);

u_long32 job_get_not_enrolled_ja_tasks(const lListElem *job);

u_long32 job_get_enrolled_ja_tasks(const lListElem *job);

u_long32 job_get_submit_ja_tasks(const lListElem *job);
 
void job_enroll(lListElem *job, lList **answer_list, 
                u_long32 task_number);  

void job_delete_not_enrolled_ja_task(lListElem *job, lList **answer_list,
                                     u_long32 ja_task_number);

int job_has_job_pending_tasks(lListElem *job);

int job_is_ja_task_defined(const lListElem *job, u_long32 ja_task_number);

void job_set_hold_state(lListElem *job, 
                        lList **answer_list, u_long32 ja_task_id,
                        u_long32 new_hold_state);

u_long32 job_get_hold_state(lListElem *job, u_long32 ja_task_id);

/* int job_add_job(lList **job_list, char *name, lListElem *job, int check,
                 int hash, htable* Job_Hash_Table); */

void job_list_print(lList *job_list);

lListElem *job_get_ja_task_template(const lListElem *job, u_long32 ja_task_id); 

lListElem *job_get_ja_task_template_hold(const lListElem *job,
                                         u_long32 ja_task_id, 
                                         u_long32 hold_state);

lListElem *job_get_ja_task_template_pending(const lListElem *job,
                                            u_long32 ja_task_id);

lListElem *job_search_task(lListElem *job, lList **answer_list, 
                           u_long32 ja_task_id, int enroll_if_not_existing);

void job_add_as_zombie(lListElem *zombie, lList **answer_list, 
                       u_long32 ja_task_id);

int job_list_add_job(lList **job_list, const char *name, lListElem *job, 
                     int check); 

int job_has_tasks(lListElem *job);

u_long32 job_get_ja_task_hold_state(const lListElem *job, u_long32 ja_task_id);

void job_destroy_hold_id_lists(const lListElem *job, lList *id_list[8]);

void job_create_hold_id_lists(const lListElem *job, lList *id_list[8],
                              u_long32 hold_state[8]);

int job_is_zombie_job(const lListElem *job); 

const char *job_get_shell_start_mode(const lListElem *job,
                                     const lListElem *queue,
                                     const char *conf_shell_start_mode);

int job_is_array(const lListElem *job); 

int job_is_parallel(const lListElem *job);

int job_is_tight_parallel(const lListElem *job, const lList *pe_list);

int job_might_be_tight_parallel(const lListElem *job, const lList *pe_list);

void job_get_submit_task_ids(const lListElem *job, u_long32 *start, 
                             u_long32 *end, u_long32 *step); 

int job_set_submit_task_ids(lListElem *job, u_long32 start, u_long32 end,
                            u_long32 step);

u_long32 job_get_smallest_unenrolled_task_id(const lListElem *job);

u_long32 job_get_smallest_enrolled_task_id(const lListElem *job);

u_long32 job_get_biggest_unenrolled_task_id(const lListElem *job);

u_long32 job_get_biggest_enrolled_task_id(const lListElem *job);

int job_list_register_new_job(const lList *job_list, u_long32 max_jobs,
                              int force_registration);   

void jatask_list_print_to_string(const lList *task_list, dstring *range_string);

lList* ja_task_list_split_group(lList **task_list);

void job_initialize_id_lists(lListElem *job, lList **answer_list);

void job_initialize_env(lListElem *job, lList **answer_list,
                        const lList* path_alias_list);

const char* job_get_env_string(const lListElem *job, const char *variable);

void job_set_env_string(lListElem *job, const char *variable, 
                        const char *value);

void job_check_correct_id_sublists(lListElem *job, lList **answer_list);

const char *job_get_id_string(u_long32 job_id, u_long32 ja_task_id, 
                              const char *pe_task_id);

int job_is_pe_referenced(const lListElem *job, const char* pe_name);

#endif /* __SGE_JOB_JATASK_H */    
