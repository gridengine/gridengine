#ifndef __SGE_JATASKL_H
#define __SGE_JATASKL_H

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

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

enum {
   JAT_task_number = JAT_LOWERBOUND,
   JAT_status,
   JAT_start_time,
   JAT_hold,
   JAT_granted_pe,
   JAT_job_restarted,
   JAT_granted_destin_identifier_list,
   JAT_master_queue,
   JAT_state,
   JAT_pvm_ckpt_pid,
   JAT_pending_signal,
   JAT_pending_signal_delivery_time,
   JAT_pid,
   JAT_osjobid,
   JAT_usage_list,
   JAT_scaled_usage_list,
   JAT_fshare,

   JAT_ticket,
   JAT_oticket,
   JAT_dticket,
   JAT_fticket,
   JAT_sticket,
   JAT_share,

   JAT_suitable,
   JAT_task_list,
   JAT_previous_usage_list
};

SLISTDEF(JAT_Type, Task)
   SGE_KULONGHU(JAT_task_number)
   SGE_RULONG(JAT_status)
   SGE_RULONG(JAT_start_time)
   SGE_RULONG(JAT_hold)       /* -h holdlist specifies hold *
                               * list(qalter,qho ld,qrls */
   SGE_XSTRING(JAT_granted_pe)
   SGE_IOBJECT(JAT_granted_pe, PE_Type)
   SGE_RBOOL(JAT_job_restarted)
   SGE_RLIST(JAT_granted_destin_identifier_list, JG_Type)
   SGE_XSTRING(JAT_master_queue)
   SGE_IOBJECT(JAT_master_queue, QU_Type)
   SGE_XULONG(JAT_state)
   SGE_XULONG(JAT_pvm_ckpt_pid)
   SGE_XULONG(JAT_pending_signal)
   SGE_XULONG(JAT_pending_signal_delivery_time)
   SGE_XULONG(JAT_pid)
   SGE_XSTRING(JAT_osjobid)
   SGE_RLIST(JAT_usage_list, UA_Type)

   /* raw usage from data */
   /* collector no longer used by */
   /* schedd scaled by qmaster to */
   /* JB_scaled_usage_list;not spooled */
   SGE_RLIST(JAT_scaled_usage_list, UA_Type)  /* SGE - scaled usage set by
                                               * qmaster, used by schedd
                                               * not spooled */
   SGE_RULONG(JAT_fshare)

   /* SGE - functional shares */
   /* associated with the job; set and */
   /* used by SGE sge_schedd; stored */
   /* to qmaster for displaying; */
   /* not spooled */
   SGE_RDOUBLE(JAT_ticket)

   /* SGE - total SGE tickets; set by */
   /* schedd, saved to qmaster, sent */
   /* to ptf; not spooled */
   SGE_RDOUBLE(JAT_oticket)

   /* SGE - override tickets; set by */
   /* schedd, saved to qmaster, sent */
   /* to ptf; not spooled */
   SGE_RDOUBLE(JAT_dticket)

   /* SGE - deadline tickets; set by */
   /* schedd, saved to qmaster, sent */
   /* to ptf; not spooled */
   SGE_RDOUBLE(JAT_fticket)

   /* SGE - functional tickets; set by */
   /* schedd, saved to qmaster, sent to */
   /* ptf; not spooled */
   SGE_RDOUBLE(JAT_sticket)

   /* SGE - share-tree tickets; set by */
   /* schedd, saved to qmaster, */
   /* not spooled */
   SGE_RDOUBLE(JAT_share)

   /* SGE - job targetted proportion; */
   /* set by schedd, saved to qmaster, */
   /* not spooled */
   SGE_XULONG(JAT_suitable)
   SGE_XLIST(JAT_task_list, JB_Type)
   /* JB_Type - sublist containing all */
   /* pe task info */
   SGE_LIST(JAT_previous_usage_list)
LISTEND 

NAMEDEF(JATN)
   NAME("JAT_task_number")
   NAME("JAT_status")
   NAME("JAT_start_time")
   NAME("JAT_hold")
   NAME("JAT_granted_pe")
   NAME("JAT_job_restarted")
   NAME("JAT_granted_destin_identifier_list")
   NAME("JAT_master_queue")
   NAME("JAT_state")
   NAME("JAT_pvm_ckpt_pid")
   NAME("JAT_pending_signal")
   NAME("JAT_pending_signal_delivery_time")
   NAME("JAT_pid")
   NAME("JAT_osjobid")
   NAME("JAT_usage_list")
   NAME("JAT_scaled_usage_list")
   NAME("JAT_fshare")

   NAME("JAT_ticket")
   NAME("JAT_oticket")
   NAME("JAT_dticket")
   NAME("JAT_fticket")
   NAME("JAT_sticket")
   NAME("JAT_share")

   NAME("JAT_suitable")
   NAME("JAT_task_list")
   NAME("JAT_previous_usage_list")
NAMEEND

/* *INDENT-ON* */

#define JATS sizeof(JATN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_JATASKL_H */
