#ifndef __RMON_CLIENT_LIST_H
#define __RMON_CLIENT_LIST_H
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

#include "rmon_job_list.h"

typedef struct client_list_type {
   u_long uid;
   u_long last_flush;
   u_long client;
   u_long all_jobs;
   job_list_type *job_list;
   struct client_list_type *next;
} client_list_type;

extern client_list_type *client_list;

int rmon_insert_cl(client_list_type *);
client_list_type **rmon_search_client(u_long client);
client_list_type *rmon_unchain_cl(client_list_type **cp);
int rmon_delete_cl(client_list_type **clp);
void rmon_print_cl(client_list_type *);
void rmon_print_client(client_list_type *);

#endif /* __RMON_CLIENT_LIST_H */



