#ifndef __SGE_GIVE_JOBS_H
#define __SGE_GIVE_JOBS_H
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

#define MAX_JOB_DELIVER_TIME (5*60)

typedef enum {
   COMMIT_DEFAULT          = 0x0000,
   COMMIT_NO_SPOOLING      = 0x0001,   /* don't spool the job */
   COMMIT_NO_EVENTS        = 0x0002,   /* don't create events */
   COMMIT_UNENROLLED_TASK  = 0x0004    /* handle unenrolled pending tasks */ 
} sge_commit_flags_t; 

int sge_give_job(lListElem *jep, lListElem *jatep, lListElem *master_qep, lListElem *pep, lListElem *hep);

void sge_commit_job(lListElem *jep, lListElem *jatep, int mode, sge_commit_flags_t commit_flags);

void ck_4_zombie_jobs(u_long now);

void resend_job(u_long32 type, u_long32 when, u_long32 jobid, u_long32 jataskid, const char *queue);

#if 0

void reschedule_unknown_event(u_long32 type, u_long32 when, u_long32 jobid, u_long32 jataskid, char *queue);   

int skip_restarted_job(lListElem *host, lListElem *job_report, u_long32 job_number, u_long32 task_number);

int reschedule_jobs(lListElem *ep, lList **answer);   

int reschedule_job(lListElem *jep, lListElem *jatep, lListElem *qep, lList **answer);

lListElem* add_to_reschedule_unknown_list(lListElem *hostr, u_long32 job_number, u_long32 task_number, u_long32 tag);

lListElem* get_from_reschedule_unknown_list(lListElem *host, u_long32 job_number, u_long32 task_number);

void delete_from_reschedule_unknown_list(lListElem *host); 

void update_reschedule_unknown_list(lListElem *host);

void update_reschedule_unknown_list_for_job(lListElem *host, u_long32 job_number, u_long32 task_number);  

#endif /* 0 */

void trigger_job_resend(u_long32 now, lListElem *hep, u_long32 jid, u_long32 tid);

void cancel_job_resend(u_long32 jid, u_long32 tid);

#endif /* __SGE_GIVE_JOBS_H */
