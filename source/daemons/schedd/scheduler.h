#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
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
 *  internal interfaces between modules of default scheduler (private)
 */

/* data model default scheduler */
typedef struct {
   lList *host_list;        /* EH_Type */
   lList *queue_list;       /* QU_Type */
   lList *all_queue_list;   /* QU_Type */
   lList *job_list;         /* JB_Type */
   lList *complex_list;     /* CX_Type */
   lList *acl_list;         /* US_Type */
   lList *pe_list;          /* PE_Type */
   lList *config_list;      /* SC_Type */
   lList *user_list;        /* UP_Type */
   lList *dept_list;        /* US_Type */
   lList *project_list;     /* UP_Type */
   lList *share_tree;       /* STN_Type */
   lList *ckpt_list;        /* CK_Type */
   lList *running_per_user; /* JC_Type */
} sge_Sdescr_t;

/*
 * external interface of default scheduler used schedd framework (public)
 */

typedef int (*default_scheduler_alg_t)(sge_Sdescr_t *);
int scheduler(sge_Sdescr_t *lists);

#ifdef SCHEDULER_SAMPLES
int my_scheduler(sge_Sdescr_t *lists);
#endif

u_long32 sgeee_get_scheduling_run_id(void);

#endif /* _SCHEDULER_H */
