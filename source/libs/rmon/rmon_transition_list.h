#ifndef __RMON_TRANSITION_LIST_H
#define __RMON_TRANSITION_LIST_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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

#include <sys/types.h>



#include "rmon_def.h"
#include "rmon_monitoring_level.h"

#include "rmon_client_list.h"
#include "rmon_spy_list.h"

typedef struct transition_list_type {
   u_long last_read;
   monitoring_level level;
   client_list_type *client;
   spy_list_type *spy;
   struct transition_list_type *next_client;
   struct transition_list_type *next_spy;
} transition_list_type;

extern transition_list_type *first_spy, *first_client;

int rmon_insert_tl(transition_list_type *);
transition_list_type **rmon_search_tl_for_sl(spy_list_type *);
transition_list_type **rmon_search_tl_for_cl(client_list_type *);
transition_list_type **rmon_search_tl_for_cl_and_sl(client_list_type *, spy_list_type *);
transition_list_type *rmon_unchain_tl_by_cl(transition_list_type **cp);
transition_list_type *rmon_unchain_tl_by_sl(transition_list_type **sp);
int rmon_delete_tl(transition_list_type **ctl, transition_list_type **stl);
void rmon_print_tl(transition_list_type *);
void rmon_print_transition(transition_list_type *);
int rmon_delete_tl_by_sl(spy_list_type *);
int rmon_delete_tl_by_cl(client_list_type *);

#endif /* __RMON_TRANSITION_LIST_H */



