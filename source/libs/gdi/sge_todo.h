#ifndef __SGE_TODO_H
#define __SGE_TODO_H
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

#include "sge_answer.h"
#include "sge_feature.h"
#include "sge_mirror.h"

void queue_list_set_unknown_state_to(lList *queue_list,
                                     const char *hostname,
                                     int send_events,
                                     int new_state);

void sge_add_queue_event(u_long32 type, lListElem *qep);

int validate_ckpt(lListElem *ep, lList **alpp);

int pe_validate(int startup, lListElem *pep, lList **alpp);

int userprj_update_master_list(sge_event_type type, sge_event_action action,
                              lListElem *event, void *clientdata);

int job_update_master_list(sge_event_type type,
                           sge_event_action action,
                           lListElem *event,
                           void *clientdata);

int
job_schedd_info_update_master_list(sge_event_type type,
                                   sge_event_action action,
                                   lListElem *event, void *clientdata);
int manop_update_master_list(sge_event_type type, sge_event_action action,
                             lListElem *event, void *clientdata);

int host_update_master_list(sge_event_type type, sge_event_action action,
                            lListElem *event, void *clientdata);

int complex_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata);

int pe_update_master_list(sge_event_type type, sge_event_action action,
                          lListElem *event, void *clientdata);

int queue_update_master_list(sge_event_type type, sge_event_action action,
                             lListElem *event, void *clientdata);

int sharetree_update_master_list(sge_event_type type, sge_event_action action,
                                 lListElem *event, void *clientdata);

int schedd_conf_update_master_list(sge_event_type type, sge_event_action action,
                                       lListElem *event, void *clientdata);

int config_update_master_list(sge_event_type type, sge_event_action action,
                              lListElem *event, void *clientdata);

int ja_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata);

int ja_task_update_master_list_usage(lListElem *event);

int ckpt_update_master_list(sge_event_type type, sge_event_action action,
                            lListElem *event, void *clientdata);

int calendar_update_master_list(sge_event_type type, sge_event_action action,
                                lListElem *event, void *clientdata);

int report_list_send(const lList *rlp, const char *rhost,
                     const char *commproc, int id,
                     int synchron, u_long32 *mid);

int pe_task_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata);

int pe_task_update_master_list_usage(lListElem *event);

int userset_update_master_list(sge_event_type type, sge_event_action action,
                               lListElem *event, void *clientdata);


#endif /* __SGE_TODO_H */
