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
/* handle the message data structure */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include "connection.h"
#include "sge_string.h"

static connection *connection_list = NULL;
static connection *last_connection = NULL;

/* functions for connection list */
static connection *create_connection(char* toHost);
static connection *search_connection_to_host(char* toHost);

/* functions for member lists */
static void       create_con_member        (connection *con, struct message    *mp);
static int        delete_con_member        (connection *con, con_member *to_delete);
static int        delete_con_member_via_mp (connection *con, struct message    *mp);
static con_member *search_con_member       (connection *con, struct message    *mp);


/****** connection/--connection ************************************************
*  NAME
*     connection -- commd connection data list
*
*  SYNOPSIS
*     connection *get_first_connection(void);
*     void       delete_connection( connection *to_delete); 
*     int        add_connection_request(struct message *mp);
*     connection *search_con_member_in_connections(struct message *mp);
*     void       del_connection_request(struct message *mp);
*
*
*  FUNCTION
*     The connection list is used to sort the open connection request to other
*     commd hosts. This is necessary, because if we do not the connect()'s in
*     the correct order, some messages can pass other messages. The connection
*     list structure is used to prevent large cpu time by searching in the
*     message list. 
*
*
*     Structure:
*     ==========
*
*       -------------           ------------         ------------
*      | connection  |  ==>    | connection |  ==>  | connection | -> ...
*       -------------           ------------         ------------
*         (to host1)             (to host2)           (to host3)   
*           ||                      ||                   ||  
*           \/                      \/                   \/
*    
*       -------------           ------------         ------------
*      | con_member  |         | con_member |       | con_member |
*       -------------           ------------         ------------
*      (first message          (first message
*         for host1  )            for host2 )
*    
*           ||                      ||                   ||  
*           \/                      \/                   \/
*    
*       -------------  
*      | con_member |                ...                  ... 
*       -------------        
*      (second message 
*         for host1 )
*   
*           ||  
*           \/ 
*           ...               
*    
*    
*     ->  Each connection is a connection to a remote commd 
*    
*     ->  Each con_member (connection member) represents a message
*         which has to be send to the remote commd
*
* 
*
*******************************************************************************/

/****** connection/create_connection() *****************************************
*  NAME
*     create_connection() -- creates a new connection to a remote commd
*
*  SYNOPSIS
*     connection* create_connection(char* toHost) 
*
*  FUNCTION
*     This function creates a new connection struct and appends it to the
*     connection list. Each connection struct represents a commd connection
*     to an other commd host. If the given connection is existing, a pointer
*     to the existing connection is returned.
*
*  INPUTS
*     char* toHost - hostname of remote commd
*
*  RESULT
*     connection* - pointer to connection struct
*
*******************************************************************************/
connection *create_connection(char* toHost)
{
   connection *con;

   if ((con=search_connection_to_host(toHost)) != NULL) {
      return con;
   }

   con = (connection *) malloc(sizeof(connection));
   if (!con)
      return NULL;

   memset(con, 0, sizeof(connection));
   
   con->toHost = sge_strdup(con->toHost,toHost);
   con->next   = NULL;

   if (last_connection) {
      last_connection->next = con;
   } else {
      connection_list = con;
   }
   last_connection = con;
   return con;
}


/****** connection/get_first_connection() **************************************
*  NAME
*     get_first_connection() -- return the first element of the connection list
*
*  SYNOPSIS
*     connection* get_first_connection(void) 
*
*  FUNCTION
*     This function returns the pointer to the first element in the connection
*     list.
*
*  RESULT
*     connection* - pointer to connection struct
*
*******************************************************************************/
connection *get_first_connection(void) 
{
   return connection_list;
}

/****** connection/delete_connection() *****************************************
*  NAME
*     delete_connection() -- delete connection to remote commd
*
*  SYNOPSIS
*     void delete_connection(connection *to_delete) 
*
*  FUNCTION
*     This function deletes the given connection from the connection list. If
*     there are any connection members (messages for the commd host) in the
*     member list of this connection, all connection members are deleted.
*
*  INPUTS
*     connection *to_delete - pointer to connection which should be deleted
*
*******************************************************************************/
void delete_connection( connection *to_delete ) 
{
   connection *con = connection_list, *last = NULL;

   while (con && (con != to_delete)) {
      last = con;
      con = con->next;
   }

   if (!con) {
      return;
   }
   
   if (last)
      last->next = con->next;
   else
      connection_list = con->next;

   if (con == last_connection)
      last_connection = last;

   while(con->member_list != NULL) {
      delete_con_member(con, con->member_list);
   }
   
   if (con->toHost) {
      free(con->toHost);
      con->toHost = NULL; 
   }

   free(con);
   con = NULL;
   return;
}

/****** connection/get_connection_count() **************************************
*  NAME
*     get_connection_count() -- return number of entries in connection list
*
*  SYNOPSIS
*     u_long32 get_connection_count(void) 
*
*  FUNCTION
*     This function counts the number of connection entries in the connection
*     list.
*
*  RESULT
*     u_long32 - connection count
*
*******************************************************************************/
u_long32 get_connection_count(void) 
{
   connection *con = connection_list;
   u_long32 count = 0;
 
   while (con) {
      count++;
      con = con->next;
   }
   return count;
}


/****** connection/search_connection_to_host() *********************************
*  NAME
*     search_connection_to_host() -- find connection for a specific host name
*
*  SYNOPSIS
*     connection* search_connection_to_host(char* toHost) 
*
*  FUNCTION
*     This function returns the connection struct which matches the given
*     hostname.
*
*  INPUTS
*     char* toHost - hostname of remote commd
*
*  RESULT
*     connection* - pointer to connection struct
*
*******************************************************************************/
connection *search_connection_to_host(char* toHost) 
{
   connection *con = connection_list;
 
   while (con) {
      if (strcasecmp(toHost,con->toHost) == 0) {
         return con;
      } 
      con = con->next;
   }
   return NULL;
}


/****** connection/create_con_member() *****************************************
*  NAME
*     create_con_member() -- add a connection member to a connection
*
*  SYNOPSIS
*     void create_con_member(connection *con, message *mp) 
*
*  FUNCTION
*     This function appends a new connection member struct (con_member) to the
*     member list of a connection.
*
*  INPUTS
*     connection *con - pointer to connection where the member should be added
*     message *mp     - message of the connection member
*
*******************************************************************************/
void create_con_member( connection *con, message *mp ) 
{
   con_member *cm;
   
   if (!con)
      return;

   cm = (con_member *) malloc(sizeof(con_member));
   if (!cm)
      return;
   
   memset(cm, 0, sizeof(con_member));
   
   cm->mp     = mp;
   cm->next   = NULL;

   if (con->last_member) {
      con->last_member->next = cm;
   } else {
      con->member_list = cm;
   }
   con->last_member = cm;
}
/****** connection/delete_con_member() *****************************************
*  NAME
*     delete_con_member() -- Remove a connection member from a connection
*
*  SYNOPSIS
*     int delete_con_member(connection *con, con_member *to_delete) 
*
*  FUNCTION
*     This procedure deletes the connection member from the connection list.
*
*  RESULT
*     int - 1 if deleted, 0 if not found
*
*  INPUTS
*     connection *con       - pointer to connection struct
*     con_member *to_delete - connection member to delete from member list
*
*******************************************************************************/
int delete_con_member (connection *con, con_member *to_delete) 
{
   con_member *cm = NULL;
   con_member *last = NULL;

   if (!con) {
      return 0;
   }

   cm = con->member_list;

   while(cm && (cm != to_delete)) {
      last = cm;
      cm = cm->next;
   }

   if (!cm) {
      return 0;
   }

   if (last)
      last->next = cm->next;
   else
      con->member_list = cm->next;

   if (cm == con->last_member)
      con->last_member = last;

   free(cm);
   cm = NULL;
   return 1;
}

/****** connection/delete_con_member_via_mp() **********************************
*  NAME
*     delete_con_member_via_mp() -- Remove a connection member from a connection
*
*  SYNOPSIS
*     int delete_con_member_via_mp(connection *con, message *mp) 
*
*  FUNCTION
*     This procedure deletes the connection member from the connection list.
*
*  INPUTS
*     connection *con - pointer to connection where the member should be added
*     message    *mp  - message of the connection member
*******************************************************************************/
int delete_con_member_via_mp (connection *con, struct message *mp) 
{
   con_member *cm = NULL;
   con_member *last = NULL;

   if (!con || !mp) {
      return 0;
   }
   cm = con->member_list;
   while(cm && (cm->mp != mp)) {
      last = cm;
      cm = cm->next;
   }
   if (!cm) {
      return 0;
   }

   if (last)
      last->next = cm->next;
   else
      con->member_list = cm->next;

   if (cm == con->last_member)
      con->last_member = last;
   free(cm);
   cm = NULL;
   return 1;
}



/****** connection/search_con_member() *****************************************
*  NAME
*     search_con_member() -- Find a connection member for a specific connection
*
*  SYNOPSIS
*     con_member* search_con_member(connection *con, message *mp) 
*
*  FUNCTION
*     This function returns the connection member of the given connection 
*     which matches the given message pointer.
*
*  INPUTS
*     connection *con - connection struct where the member should be seeked
*     message    *mp  - message to search for in member list
*
*  RESULT
*     con_member* - pointer to the connection member where the message is
*                   associated with
*
*******************************************************************************/
con_member *search_con_member(connection *con, message *mp) 
{
   con_member *cm = NULL;

   if (!con) {
      return NULL;
   }
   cm = con->member_list;

   while (cm) {
      if (cm->mp == mp) {
         return cm;
      }
      cm = cm->next;
   }
   return NULL;
}

/****** connection/get_con_member_count() **************************************
*  NAME
*     get_con_member_count() -- Count the number of members in connection
*
*  SYNOPSIS
*     u_long32 get_con_member_count(connection *con) 
*
*  FUNCTION
*     This function returns the number of connection members for the given
*     connection. (=messages to send to that host)
*
*  INPUTS
*     connection *con - pointer to connection
*
*  RESULT
*     u_long32 - Number of connection members (=messages to send for this 
*                connection)
*
*******************************************************************************/
u_long32   get_con_member_count(connection *con) 
{
   con_member *cm = NULL;
   u_long32 count = 0;

   if (!con) {
      return count;
   }
   cm = con->member_list;
   while (cm) {
      count++;
      cm = cm->next;
   }
   return count;
}


/****** connection/add_connection_request() ************************************
*  NAME
*     add_connection_request() -- append a connection member for a message
*
*  SYNOPSIS
*     int add_connection_request(message *mp) 
*
*  FUNCTION
*     This function appends a new connection member in the connection list. If
*     the connection is not existing, a new connection is created.
*
*  INPUTS
*     message *mp - message for which a connection member is created
*
*  RESULT
*     int - 1: connection member is the first in member list of connection
*           0: the member is not the first member in member list of connection
*
*******************************************************************************/
int add_connection_request(message *mp) 
{
   connection *con = NULL;
   if (mp) {
      con = create_connection( sge_host_get_mainname(mp->to.host) );
      if (con) {
         if (con->member_list == NULL) {
            create_con_member(con,mp);
            return 1;
         }
         /* only create new member when mp isn't already in member list */
         if ( search_con_member(con,mp) == NULL ) {
            create_con_member(con,mp);
            return 0;
         }
      }
   }
   return 0;
}

/****** connection/search_con_member_in_connections() **************************
*  NAME
*     search_con_member_in_connections() -- search connection for a message
*
*  SYNOPSIS
*     connection* search_con_member_in_connections(message *mp) 
*
*  FUNCTION
*     This function returns the connection for a given message
*
*  INPUTS
*     message *mp - pointer to message struct
*
*  RESULT
*     connection* - pointer to connection list entry
*
*******************************************************************************/
connection *search_con_member_in_connections(message *mp) 
{
   connection *con = connection_list;
   con_member *cm = NULL;
   while(con) {
      if ( (cm=search_con_member(con,mp)) != NULL) {
         return con;
      }
      con=con->next; 
   }
   return NULL;
}


/****** connection/del_connection_request() ************************************
*  NAME
*     del_connection_request() -- remove connection member from connection
*
*  SYNOPSIS
*     void del_connection_request(message *mp) 
*
*  FUNCTION
*     If a connection is estabilished the connection member can be deleted from
*     a connection member list. If no more members are in the connection list
*     the whole connection is deleted.
*
*  INPUTS
*     message *mp - message for which a connection member is associated with
*
*******************************************************************************/
void del_connection_request(message *mp) 
{
   connection *con = NULL;

   for (con = connection_list; con; con = con->next) {
      if ( delete_con_member_via_mp(con,mp) ) { 
         if (con->member_list == NULL) {
            delete_connection(con); 
         }
         return;
      }
   }
   return;
}



