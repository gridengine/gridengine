#ifndef __CONNECTION_H
#define __CONNECTION_H
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
#include <sys/types.h>

#include "basis_types.h"
#include "message.h"


/*
 *
 * We have the following structure:
 *   -------------     ------------      ------------
 *  | connection | -> | connection | -> | connection | -> ...
 *   -------------     ------------      ------------
 *     
 *       ||                ||                ||  
 *       \/                \/                \/
 *
 *   -------------     ------------      ------------
 *  | con_member |    | con_member |    | con_member |
 *   -------------     ------------      ------------
 *
 *
 *       ||                ||                ||  
 *       \/                \/                \/
 *
 *       
 *      ...                ...               ... 
 *      
 *
 *
 * ->  Each connection is a connection to a remote commd 
 *
 * ->  Each con_member (connection member) represents a message
 *     which has to be send to the remote commd
 *
 * 
 */

typedef struct con_member {
   struct message           *mp;
   struct con_member *next;
} con_member;

typedef struct connection {
  char       *toHost;
  con_member *member_list;
  con_member *last_member;
  struct connection *next;
} connection;


/* global functions */
connection *get_first_connection(void);
void       delete_connection( connection *to_delete); 
int        add_connection_request(struct message *mp);
connection *search_con_member_in_connections(struct message *mp);
void       del_connection_request(struct message *mp);
u_long32   get_con_member_count(connection *con);
u_long32   get_connection_count(void);


#endif /* __CONNECTION_H */
