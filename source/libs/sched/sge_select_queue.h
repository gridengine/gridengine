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

#include "sge_orders.h"

int
sge_select_queue(lList *reqested_attr, lListElem *queue, lListElem *host, lList *exechost_list,
                 lList *centry_list, int allow_non_requestable, int slots); 

/* --- is there a load alarm on this queue ---------------------------- */

int sge_load_alarm(char *reason, lListElem *queue, lList *threshold, 
                   const lList *exechost_list, const lList *complex_list, 
                   const lList *load_adjustments, bool is_check_consumable);

void sge_create_load_list(const lList *queue_list, const lList *host_list, 
                          const lList *centry_list, lList **load_list); 

bool sge_load_list_alarm(lList *load_list, const lList *host_list, 
                         const lList *centry_list);

void sge_remove_queue_from_load_list(lList **load_list, const lList *queue_list);

void sge_free_load_list(lList **load_list); 
 
char *sge_load_alarm_reason(lListElem *queue, lList *threshold, const lList *exechost_list, 
                            const lList *complex_list, char  *reason, int reason_size, 
                            const char *type); 

int sge_split_queue_load(lList **unloaded, lList **overloaded, lList *exechost_list, 
                         lList *complex_list, const lList *load_adjustments, 
                         lList *granted, bool is_consumable_load_alarm, bool is_comprehensive,
                         u_long32 ttype);

int sge_split_queue_slots_free(lList **unloaded, lList **overloaded);

int sge_split_disabled(lList **unloaded, lList **overloaded);

int sge_split_suspended(lList **queue_list, lList **suspended);

/* --- job assignment methods ---------------------------- */

enum { 
   DISPATCH_TIME_NOW = 0, 
   DISPATCH_TIME_QUEUE_END = MAX_ULONG32 
};

enum { 
   MATCH_NOW = 0x01, 
   MATCH_LATER = 0x02, 
   MATCH_NEVER = 0x04 
};

typedef struct {
   /* ------ this section determines the assignment ------------------------------- */
   u_long32    job_id;            /* job id (convenience reasons)                   */
   u_long32    ja_task_id;        /* job array task id (convenience reasons)        */
   lListElem  *job;               /* the job (JB_Type)                              */
   lListElem  *ja_task;           /* the task (JAT_Type) (if NULL only reschedule   */
                                  /* unknown verification is missing)               */
   lListElem  *ckpt;              /* the checkpoint interface (CK_Type)             */
   lListElem  *gep;               /* the global host (EH_Type)                      */
   u_long32   duration;           /* jobs time of the assignment                    */
   const lList *load_adjustments; /* shall load adjustmend be considered (CE_Type)  */
   lList      *host_list;         /* the hosts (EH_Type)                            */
   lList      *queue_list;        /* the queues (QU_Type)                           */
   lList      *centry_list;       /* the complex entries (CE_Type)                  */
   lList      *acl_list;          /* the user sets (US_Type)                        */
   bool       is_reservation;      /* true, if a reservation for this job should be done */
   /* ------ this section is the resulting assignment ----------------------------- */
   lListElem  *pe;                /* the parallel environment (PE_Type)             */
   lList      *gdil;              /* the resources (JG_Type)                        */
   int        slots;              /* total number of slots                          */
   u_long32   start;              /* jobs start time                                */
   int        soft_violations;    /* number of soft request violations              */
} sge_assignment_t;

void assignment_init(sge_assignment_t *a, lListElem *job, lListElem *ja_task);
void assignment_copy(sge_assignment_t *dst, sge_assignment_t *src, bool move_gdil);
void assignment_release(sge_assignment_t *a);

/* -------------------------------------------------------------------------------- */


typedef enum {
   DISPATCH_NEVER = 4,  /* an error happend, no dispatch will ever work again */
   DISPATCH_MISSING_ATTR = 2, /* attribute does not exist */
   DISPATCH_NOT_AT_TIME = 1,  /* no assignment at the specified time */
   DISPATCH_OK = 0,           /* ok got an assignment + set time for DISPATCH_TIME_QUEUE_END */
   DISPATCH_NEVER_CAT = -1,   /* assignment will never be possible for all jobs of that category */
   DISPATCH_NEVER_JOB = -2,   /* assignment will never be possible for that particular job */
}dispatch_t;

dispatch_t
sge_sequential_assignment(sge_assignment_t *a);

dispatch_t
sge_select_parallel_environment(sge_assignment_t *best, lList *pe_list);

/* -------------------------------------------------------------------------------- */

bool is_requested(lList *req, const char *attr);

dispatch_t 
sge_queue_match_static(lListElem *queue, lListElem *job, const lListElem *pe, 
                       const lListElem *ckpt, lList *centry_list, lList *acl_list);

dispatch_t
sge_host_match_static(lListElem *job, lListElem *ja_task, lListElem *host, lList *centry_list, 
                      lList *acl_list);

/* ------ DEBUG / output methods --------------------------------------------------- */

/* not used */
/* int sge_get_ulong_qattr(u_long32 *uvalp, char *attrname, lListElem *q, lList *exechost_list, lList *complex_list); */

int sge_get_double_qattr(double *dvalp, char *attrname, lListElem *q, 
                         const lList *exechost_list, const lList *complex_list,
                         bool *has_value_from_object);

int sge_get_string_qattr(char *dst, int dst_len, char *attrname, lListElem *q, const lList *exechost_list, const lList *complex_list);

char *trace_resource(lListElem *rep);

void trace_resources(lList *resources);

#endif
