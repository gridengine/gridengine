#ifndef __SGE_QUEUE_H
#define __SGE_QUEUE_H

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

#include "sge_dstring.h"

#include "sge_queueL.h"

typedef enum {
   QUEUE_TAG_DEFAULT         = 0x0000,
   QUEUE_TAG_IGNORE_TEMPLATE = 0x0001
} queue_tag_t;

extern const char *queue_types[];

extern lList *Master_Queue_List;

void queue_or_job_get_states(int nm, char *str, u_long32 op);

void queue_get_state_string(char *str, u_long32 op);

bool 
queue_list_suspends_ja_task(lList *queue_list, lList *granted_queue_list);

lListElem *queue_list_locate(lList *queue_list, const char *queue_name);

void queue_list_set_tag(lList *queue_list,
                        queue_tag_t flags,
                        u_long32 tag_value);

void queue_list_clear_tags(lList *queue_list);

int queue_reference_list_validate(lList **alpp, lList *qr_list, 
                                  const char *attr_name, const char *obj_descr, 
                                  const char *obj_name);  

bool queue_list_add_queue(lListElem *queue);

bool queue_check_owner(const lListElem *queue, const char *user_name);

bool 
queue_validate(lListElem *queue, lList **answer_list);

lListElem *
queue_create_template(void);

bool queue_is_batch_queue(const lListElem *this_elem);

bool queue_is_interactive_queue(const lListElem *this_elem);

bool queue_is_checkointing_queue(const lListElem *this_elem);

bool queue_is_parallel_queue(const lListElem *this_elem);

bool 
queue_print_qtype_to_dstring(const lListElem *this_elem, 
                             dstring *string, bool only_first_char);

bool
queue_parse_qtype_from_string(lListElem *queue, lList **answer_list, 
                              const char *value);

bool 
queue_is_pe_referenced(const lListElem *this_elem, const lListElem *pe);

bool 
queue_is_ckpt_referenced(const lListElem *this_elem, const lListElem *ckpt);

bool
queue_is_centry_referenced(const lListElem *this_elem, const lListElem *centry);

bool
queue_is_centry_a_complex_value(const lListElem *this_elem, 
                                const lListElem *name);

#endif /* __SGE_QUEUE_H */
