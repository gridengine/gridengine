#ifndef __SGE_HOST_H
#define __SGE_HOST_H
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

#include "sge_hostL.h"

extern lList *Master_Exechost_List;
extern lList *Master_Adminhost_List;
extern lList *Master_Submithost_List;


bool host_is_referenced(const lListElem *host, lList **answer_list,
                        const lList *queue_list);

const char *host_get_load_value(lListElem *host, const char *name);

int sge_resolve_host(lListElem *ep, int nm);

int sge_resolve_hostname(const char *hostname, char *unique, int nm);

bool
host_is_centry_referenced(const lListElem *this_elem, const lListElem *centry);

bool
host_is_centry_a_complex_value(const lListElem *this_elem, 
                               const lListElem *centry);

bool
host_trash_load_values(lListElem *host);

lListElem *
host_list_locate(const lList *this_list, const char *hostname);

#endif /* __SGE_HOST_H */

