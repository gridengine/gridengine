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

#ifndef __SGE_H
#   include "sge.h"
#endif

int sge_select_queue(lList *reqested_attr, lListElem *queue, lListElem *host, lList *exechost_list,
                     lList *centry_list, int allow_non_requestable, char *reason, 
                     int reason_size, int slots); 

/* 
 * is there a load alarm on this queue
 * 
 */
int sge_load_alarm(char *reason, lListElem *queue, lList *threshold, const lList *exechost_list, const lList *complex_list, const lList *load_adjustments);

/* 
 * get reason for alarm state on queue
 * 
 */
char *sge_load_alarm_reason(lListElem *queue, lList *threshold, const lList *exechost_list, const lList *complex_list, char  *reason, int reason_size, const char *type); 

/* 
 * split queue list into unloaded and overloaded
 * 
 */
int sge_split_queue_load(lList **unloaded, lList **overloaded, lList *exechost_list, lList *complex_list, const lList *load_adjustments, lList *granted, u_long32 ttype);

int sge_split_queue_slots_free(lList **unloaded, lList **overloaded);

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
enum { DISPATCH_TIME_NOW = 0, DISPATCH_TIME_QUEUE_END = MAX_ULONG32 };
enum { MATCH_NOW = 0x01, MATCH_LATER = 0x02, MATCH_NEVER = 0x04 };

typedef struct {
   /* ------ this section determines the assignment ------------------------------- */
   u_long32    job_id;            /* job id (convenience reasons)                   */
   u_long32    ja_task_id;        /* job array task id (convenience reasons)        */
   lListElem  *job;               /* the job (JB_Type)                              */
   lListElem  *ja_task;           /* the task (JAT_Type)                            */
   lListElem  *ckpt;              /* the checkpoint interface (CK_Type)             */
   lListElem  *gep;               /* the global host (EH_Type)                      */
   u_long32   duration;           /* jobs time of the assignment                    */
   const lList *load_adjustments; /* shall load adjustmend be considered (CE_Type)  */
   lList      *host_list;         /* the hosts (EH_Type)                            */
   lList      *queue_list;        /* the queues (QU_Type)                           */
   lList      *centry_list;       /* the complex entries (CE_Type)                  */
   lList      *acl_list;          /* the user sets (US_Type)                        */

   /* ------ this section is the resulting assignment ----------------------------- */
   lListElem  *pe;                /* the parallel environment (PE_Type)             */
   lList      *gdil;              /* the resources (JG_Type)                        */
   int        slots;              /* total number of slots                          */
   u_long32   start;              /* jobs start time                                */
} sge_assignment_t;

int sge_sequential_assignment(sge_assignment_t *a, lList **ignore_hosts, lList **ignore_queues);
int sge_parallel_assignment(sge_assignment_t *a);


/* 
 * which ulong value has the following attribute
 * 
 */
/* not used */
/*
int sge_get_ulong_qattr(u_long32 *uvalp, char *attrname, lListElem *q, lList *exechost_list, lList *complex_list);
*/
/* 
 * which double value has the following attribute
 * 
 */
int sge_get_double_qattr(double *dvalp, char *attrname, lListElem *q, 
                         const lList *exechost_list, const lList *complex_list,
                         bool *has_value_from_object);

/* 
 * which string value has the following attribute
 * 
 */
int sge_get_string_qattr(char *dst, int dst_len, char *attrname, lListElem *q, const lList *exechost_list, const lList *complex_list);

/* 
 *
 * make debitations on all queues that are necessary 
 *
 */
int debit_job_from_queues(lListElem *job, lList *selected_queue_list, lList *global_queue_list, 
    lList *complex_list, lList *orders_list);

char *trace_resource(lListElem *rep);

void trace_resources(lList *resources);

bool is_requested(lList *req, const char *attr);
int sge_best_result(int r1, int r2);

/* -------------------------------------------------------------------------------- */

int pe_slots_by_time(u_long32 start, u_long32 duration, int *slots, int *slots_qend, 
      lListElem *job, lListElem *pe, lList *acl_list);

int global_time_by_slots(int slots, u_long32 *start_time, u_long32 duration, 
      int *violations, lListElem *job, lListElem *gep, lList *centry_list, 
      lList *acl_list);

int global_slots_by_time(u_long32 start, u_long32 duration, int *slots, 
      int *slots_qend, int *violations, lListElem *job, lListElem *gep,
      lList *centry_list, lList *acl_list);

int host_time_by_slots(int slots, u_long32 *start, u_long32 duration, 
   int *host_soft_violations, lListElem *job, lListElem *ja_task, lListElem *hep, 
   lList *centry_list, lList *acl_list);

int host_slots_by_time(u_long32 start, u_long32 duration, int *slots, 
   int *slots_qend, int *host_soft_violations, lListElem *job, lListElem *ja_task, 
   lListElem *hep, lList *queue_list, lList *centry_list, lList *acl_list, 
   const lList *load_adjustments, bool allow_non_requestable);

int queue_slots_by_time( u_long32 start, u_long32 duration, int *slots, int *slots_qend, 
   int *violations, lListElem *job, lListElem *qep, const lListElem *pe, const lListElem *ckpt,
   lList *centry_list, lList *acl_list, bool allow_non_requestable);

int queue_time_by_slots( int slots, u_long32 *start, u_long32 duration, int *violations,
   lListElem *job, lListElem *qep, const lListElem *pe, const lListElem *ckpt, lList *centry_list, 
   lList *acl_list);

int queue_match_static(lListElem *queue, lListElem *job, const lListElem *pe, 
      const lListElem *ckpt, lList *centry_list, lList *acl_list);

int host_match_static(lListElem *job, lListElem *ja_task, 
                                lListElem *host, lList *centry_list, 
                                lList *acl_list);

#endif
