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

#include "sge_queueL.h"

typedef enum {
   QUEUE_TAG_DEFAULT         = 0x0000,
   QUEUE_TAG_IGNORE_TEMPLATE = 0x0001
} queue_tag_t;

extern lList *Master_Queue_List;

void queue_or_job_get_states(int nm, char *str, u_long32 op);

void queue_get_state_string(char *str, u_long32 op);

lListElem *queue_list_locate(lList *queue_list, const char *queue_name);

void queue_list_set_tag(lList *queue_list,
                        queue_tag_t flags,
                        u_long32 tag_value);

void queue_list_clear_tags(lList *queue_list);

#endif /* __SGE_QUEUE_H */
