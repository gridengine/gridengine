#ifndef __SGE_SELECT_QUEUE_H
#define __SGE_SELECT_QUEUE_H
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
 * can this job run on this queue 
 * if not why?
 * 
 */
int sge_select_queue(lList *complex_attributes, lList *resources, int allow_non_requestable, char *reason, int reason_size, int slots);


int sge_match_complex_attributes(lList *queue_attr, lList *host_attr, lList *requested_attr, int quick_exit, int allow_non_requestable, char *reason, int reason_size, int is_a_request, int slots, /*lList *consumable_config_list[3],*/ int force_existence);

/* 
 * is there a load alarm on this queue
 * 
 */
int sge_load_alarm(char *reason, lListElem *queue, lList *threshold, lList *exechost_list, lList *complex_list, lList *load_adjustments);

/* 
 * get reason for alarm state on queue
 * 
 */
char *sge_load_alarm_reason(lListElem *queue, lList *threshold, lList *exechost_list, lList *complex_list, char  *reason, int reason_size, const char *type); 

/* 
 * split queue list into unloaded and overloaded
 * 
 */
int sge_split_queue_load(lList **unloaded, lList **overloaded, lList *exechost_list, lList *complex_list, lList *load_adjustments, lList *granted, u_long32 ttype);

/* 
 * split queue list into queues with at least n slots
 * and queues with less than n free slots
 * 
 */
int sge_split_queue_nslots_free(lList **unloaded, lList **overloaded, int nslots);

int sge_split_disabled(lList **unloaded, lList **overloaded);

int sge_split_suspended(lList **queue_list, lList **suspended);

/* 
 * replicate all queues from the queues list into 
 * the suitable list that are suitable for this
 * job. 
 * 
 * sort order of the queues:
 *    in case of sort by seq_no the queues will 
 *    have the same order as the queues list 
 *
 *    in case of sort by load the queues will
 *    get the same order as the host list
 * 
 */

enum { DISPATCH_TYPE_NONE = 0, DISPATCH_TYPE_FAST, DISPATCH_TYPE_COMPREHENSIVE };

lList *sge_replicate_queues_suitable4job(lList *queues, lListElem *job, lListElem *ja_task, lListElem *pe_list, lListElem *ckpt, int sort_seq_no, lList *complex_list, lList *host_list, lList *acl_list, lList *load_adjustments, int ndispatched, int *last_dispatch_type, int host_order_changed);


/* 
 * which ulong value has the following attribute
 * 
 */
int sge_get_ulong_qattr(u_long32 *uvalp, char *attrname, lListElem *q, lList *exechost_list, lList *complex_list);

/* 
 * which double value has the following attribute
 * 
 */
int sge_get_double_qattr(double *dvalp, char *attrname, lListElem *q, lList *exechost_list, lList *complex_list, bool *has_value_from_object);

/* 
 * which string value has the following attribute
 * 
 */
int sge_get_string_qattr(char *dst, int dst_len, char *attrname, lListElem *q, lList *exechost_list, lList *complex_list);

/* 
 *
 * make debitations on all queues that are necessary 
 *
 */
int debit_job_from_queues(lListElem *job, lList *selected_queue_list, lList *global_queue_list, lList *complex_list, u_long32 *total_slotsp, lList *orders_list);

int debit_queue_consumable(lListElem *jep, lListElem *qep, lList *complex_list, int slots);

/* 
 *
 * check wheater queues are requestable
 *
 */
int queues_are_requestable(lList *complex_list);

char *trace_resource(lListElem *rep);

void trace_resources(lList *resources);

int available_slots_at_queue(lList *host_resources, lListElem *job, lListElem *queue, lListElem *pe, lListElem *ckpt, lList *host_list, lList *cplx_list, lList *acl_list, lList *load_adjustments, int host_slots,/* lList *ccl[3],*/ int ndispatched, lListElem *global_hep,    int total_slots, lListElem *hep);

#endif
