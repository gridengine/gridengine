#ifndef __SGE_ORDERL_H
#define __SGE_ORDERL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* 
 * valid values for OR_type 
 */
enum {
   ORT_start_job = 1,               /* SGE & SGEEE */
   ORT_tickets,                     /*       SGEEE */
   ORT_remove_job,                  /* SGE & SGEEE */
   ORT_update_project_usage,        /*       SGEEE */
   ORT_update_user_usage,           /*       SGEEE */
   ORT_share_tree,                  /*       SGEEE */
   ORT_remove_immediate_job,        /* SGE & SGEEE */
   ORT_sched_conf,                  /* SGE & SGEEE */
   ORT_suspend_on_threshold,        /* SGE & SGEEE */
   ORT_unsuspend_on_threshold,      /* SGE & SGEEE */
   ORT_job_schedd_info              /* SGE & SGEEE */
};

enum {
   OR_seq_no = OR_LOWERBOUND,
   OR_type,
   OR_force,
   OR_job_number,
   OR_ja_task_number,
   OR_job_version,
   OR_queuelist,
   OR_ticket,
   OR_joker,
   OR_pe
};

LISTDEF(OR_Type)
   SGE_ULONG(OR_seq_no)       /* sequence number */
   SGE_ULONG(OR_type)         /* command */
   SGE_ULONG(OR_force)        /* set if force */
   SGE_ULONG(OR_job_number)   /* which job */
   SGE_ULONG(OR_ja_task_number)       /* which JobArray task */
   SGE_ULONG(OR_job_version)  /* which job version */
   SGE_LIST(OR_queuelist)     /* associated queue list */
   SGE_ULONG(OR_ticket)       /* SGEEE job tickets */
   SGE_LIST(OR_joker)         /* type of this sublist depends on OR_type:  
                               * ORT_start_job empty ORT_remove_job empty  
                               * ORT_tickets reduced job element JB_Type
                               * ORT_update_*_usage reduced user/prj object
                               * UP_Type ORT_share_tree reduced share tree
                               * root node STN_Type *
                               * ORT_remove_immediate_job empty *
                               * ORT_job_schedd_info SME_Type */
   SGE_STRING(OR_pe)          /* which pe */
LISTEND 

NAMEDEF(ORN)
     NAME("OR_seq_no")
     NAME("OR_type")
     NAME("OR_force")
     NAME("OR_job_number")
     NAME("OR_ja_task_number")
     NAME("OR_job_version")
     NAME("OR_queuelist")
     NAME("OR_ticket")
     NAME("OR_joker")
     NAME("OR_pe")
     NAMEEND
#define ORS sizeof(ORN)/sizeof(char*)

/* entries for the OR_queuelist-field */
enum {
   OQ_slots = OQ_LOWERBOUND,
   OQ_dest_queue,
   OQ_dest_version,
   OQ_ticket,
   OQ_oticket,
   OQ_fticket,
   OQ_dticket,
   OQ_sticket
};

LISTDEF(OQ_Type)
   SGE_ULONG(OQ_slots)        /* number of slots on this queue */
   SGE_STRING(OQ_dest_queue)  /* queue where job has to run */
   SGE_ULONG(OQ_dest_version) /* version of this queue */
   SGE_ULONG(OQ_ticket)       /* total SGEEE tickets for slots */
   SGE_ULONG(OQ_oticket)      /* total SGEEE override tickets */
   SGE_ULONG(OQ_fticket)      /* total SGEEE functional tickets */
   SGE_ULONG(OQ_dticket)      /* total SGEEE deadline tickets */
   SGE_ULONG(OQ_sticket)      /* total SGEEE sharetree tickets */
LISTEND 

NAMEDEF(OQN)
   NAME("OQ_slots")
   NAME("OQ_dest_queue")
   NAME("OQ_dest_version")
   NAME("OQ_ticket")
   NAME("OQ_oticket")
   NAME("OQ_fticket")
   NAME("OQ_dticket")
   NAME("OQ_sticket")
NAMEEND

/* *INDENT-ON* */ 

#define OQS sizeof(OQN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_ORDERL_H */
