#ifndef __SGE_CQUEUE_H
#define __SGE_CQUEUE_H

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

#include "sge_cqueueL.h"

typedef struct _list_attribute_struct {
   int cqueue_attr;
   int qinstance_attr;
   int href_attr;
   int value_attr;
   int primary_key_attr;
   const char *name;
   bool is_sgeee_attribute;
} list_attribute_struct;

extern lList *Master_CQueue_List;

extern list_attribute_struct cqueue_attribute_array[];

lListElem *
cqueue_create(lList **answer_list, const char *name);

lList **
cqueue_list_get_master_list(void);

bool
cqueue_list_add_cqueue(lListElem *queue);

lListElem *
cqueue_list_locate(const lList *this_list, const char *name);

bool
cqueue_mod_sublist(lListElem *this_elem, lList **answer_list,
                   lListElem *reduced_elem, int sub_command,
                   int attribute_name, int sublist_host_name,
                   int sublist_value_name, int subsub_key,
                   const char *attribute_name_str,
                   const char *object_name_str);

#endif /* __SGE_CQUEUE_H */
