#ifndef __SGE_QUEUE_QMASTER_H
#define __SGE_QUEUE_QMASTER_H
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

#include "sge_c_gdi.h"

int sge_gdi_add_mod_queue(lListElem *qep, lList **alpp, char *ruser, char *rhost, int add, int sub_command);

int sge_gdi_delete_queue(lListElem *qep, lList **alpp, char *ruser, char *rhost);

int sge_change_queue_version(lListElem *qep, int add, int write_history);

/* old functions underlaying the gdi functions */
int sge_add_queue(lListElem *qep);
int sge_del_queue(const char *qname);

void sge_add_queue_event(u_long32 type, lListElem *qep);

int check_qsiq_lic(int licensed_qsi_queues, int verbose);

#if 0
int verify_project_list(lList **alpp, char *obj_name, char *qname, lList *project_list);
#endif

int verify_qr_list(lList **alpp, lList *qr_list, const char *attr_name, const char *obj_descr, const char *obj_name);  

void queue_list_set_state_to_unknown(lList *queue_list, 
                                     const char *hostname,
                                     int send_events);

#endif /* __SGE_QUEUE_QMASTER_H */

