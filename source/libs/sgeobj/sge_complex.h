#ifndef __SGE_COMPLEX_H 
#define __SGE_COMPLEX_H 
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

#include "sge_complexL.h"

extern lList *Master_Complex_List;

int sge_fill_requests(lList *reql, lList *complex_list, int allow_non_requestable, int allow_empty_boolean, int allow_neg_consumable);

int fill_and_check_attribute(lListElem *alp, int allow_empty_boolean, int allow_neg_consumable);

void complex_list_init_double_attr(lList *cl);

lListElem* complex_list_locate_attr(lList *complex_list, const char *name);

int complex_list_verify(lList *complex_list, lList **alpp,
                        const char *obj_name, const char *qname); 

bool
centry_print_resource_to_dstring(const lListElem *this_elem, dstring *string);

/* mapping functions - useful for in-/output */
const char *map_op2str(u_long32 op);

const char *map_type2str(u_long32 type);

#endif /* __SGE_COMPLEX_H */
