#ifndef __SGE_RESOURCE_UTILIZATION_H 
#define __SGE_RESOURCE_UTILIZATION_H 
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

#include "sge_resource_utilizationL.h"
#include "sge_select_queue.h"

/* those are for treating resource utilization */
bool utilization_print_to_dstring(const lListElem *this_elem, dstring *string);
void utilization_print(const lListElem *cr, const char *object_name);
int utilization_add(lListElem *cr, u_long32 start_time, u_long32 duration, double utilization, 
   u_long32 job_id, u_long32 ja_taskid, u_long32 level, const char *object_name, const char *type);
double utilization_max(const lListElem *cr, u_long32 start_time, u_long32 end_time);
void utilization_print_all(const lList* pe_list, lList *host_list, const lList *queue_list);
u_long32 utilization_below(const lListElem *cr, double max_util, const char *object_name);

int add_job_utilization(const sge_assignment_t *a, const char *type);
double utilization_queue_end(const lListElem *cr);

#endif /* __SGE_RESOURCE_UTILIZATION_H */

