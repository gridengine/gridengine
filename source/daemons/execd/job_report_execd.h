#ifndef _JOB_REPORT_EXECD_H_
#define _JOB_REPORT_EXECD_H_
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

extern int flush_jr;

lListElem *add_job_report(u_long32 jobid, u_long32 jataskid, lListElem *jep);
lListElem *get_job_report(u_long32 jobid, u_long32 jataskid, char *pe_task_id_str);
void del_job_report(lListElem *jr);
void cleanup_job_report(u_long32 jobid, u_long32 jataskid);
void flush_job_report(void);
void trace_jr(void);

int add_usage(lListElem *jr, char *name, char *uval_as_str, double val);

#include "dispatcher.h"

int execd_c_ack(struct dispatch_entry *de, sge_pack_buffer *pb, sge_pack_buffer *apb, u_long *rcvtimeout, int *synchron, char *err_str, int answer_error);

#endif /* _JOB_REPORT_EXECD_H_ */
