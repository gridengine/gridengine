#ifndef __CL_COMMUNICATION_H
#define __CL_COMMUNICATION_H

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

#include "cl_xml_parsing.h"

#define CL_DEFINE_READ_TIMEOUT                       15 
#define CL_DEFINE_WRITE_TIMEOUT                      15
#define CL_DEFINE_ACK_TIMEOUT                        60
#define CL_DEFINE_GET_CLIENT_CONNECTION_DATA_TIMEOUT 60   /* default timeout for accepting a connection */
#define CL_DEFINE_SYNCHRON_RECEIVE_TIMEOUT           60   /* default timeout for synchron send messages */
#define CL_DEFINE_CLIENT_CONNECTION_LIFETIME         600   /* Cut off connection when client is not active for this time */


#define CL_DEFINE_DATA_BUFFER_SIZE                   1024 * 4           /* 4 KB buffer for reading/writing messages */
#if 0
/* TODO: enable this when application code is not using u_short with client ids !!! */
#define CL_DEFINE_MAX_MESSAGE_ID                     4294967295UL       /* max unsigned long value for a 32 bit system */
#else
#define CL_DEFINE_MAX_MESSAGE_ID                     65535              /* max unsigned short value */
#endif

#ifdef MAXHOSTNAMELEN
#define CL_MAXHOSTNAMELEN_LENGTH MAXHOSTNAMELEN
#else
#define CL_MAXHOSTNAMELEN_LENGTH 256
#endif


int  cl_com_compare_endpoints(cl_com_endpoint_t* endpoint1, cl_com_endpoint_t* endpoint2);
void cl_com_dump_endpoint(cl_com_endpoint_t* endpoint, const char* text);


int cl_com_endpoint_list_refresh(cl_raw_list_t* endpoint_list);

struct in_addr*   cl_com_copy_in_addr(struct in_addr *in_addr);
cl_com_hostent_t* cl_com_copy_hostent(cl_com_hostent_t* hostent);

int cl_com_gethostname(char **unique_hostname,struct in_addr *copy_addr,struct hostent **he_copy, int* system_error_value);
int cl_com_host_list_refresh(cl_raw_list_t* host_list);
int cl_com_cached_gethostbyname( char *host, char **unique_hostname, struct in_addr *copy_addr,struct hostent **he_copy, int* system_error_value);
int cl_com_cached_gethostbyaddr( struct in_addr *addr, char **unique_hostname,struct hostent **he_copy,int* system_error_val );
char* cl_com_get_h_error_string(int h_error);
int cl_com_compare_hosts(char* host1, char* host2);
int cl_com_dup_host(char** host_dest, char* source, cl_host_resolve_method_t method, char* domain);
int cl_com_set_resolve_method(cl_host_resolve_method_t method, char* local_domain_name);

int cl_com_free_hostent(cl_com_hostent_t **hostent_p);                    /* CR check */
int cl_com_free_hostspec(cl_com_host_spec_t **hostspec);
int cl_com_print_host_info(cl_com_hostent_t *hostent_p );                 /* CR check */



/* debug / help functions */
void cl_dump_connection(cl_com_connection_t* connection);                     /* CR check */
void cl_dump_private(cl_com_connection_t* connection);


const char* cl_com_get_framework_type(cl_com_connection_t* connection);       /* CR check */
const char* cl_com_get_connection_type(cl_com_connection_t* connection);      /* CR check */
const char* cl_com_get_service_handler_flag(cl_com_connection_t* connection); /* CR check */
const char* cl_com_get_data_write_flag(cl_com_connection_t* connection);      /* CR check */
const char* cl_com_get_data_read_flag(cl_com_connection_t* connection);       /* CR check */
const char* cl_com_get_connection_state(cl_com_connection_t* connection); /* CR check */
const char* cl_com_get_data_flow_type(cl_com_connection_t* connection);       /* CR check */

/* This can be called by an signal handler to trigger abort of communications */
void cl_com_ignore_timeouts(cl_bool_t flag); 
cl_bool_t cl_com_get_ignore_timeouts_flag(void);


/* message functions */
int cl_com_setup_message(cl_com_message_t** message, cl_com_connection_t* connection, cl_byte_t* data,unsigned long size, cl_xml_ack_type_t ack_type, unsigned long response_id, unsigned long tag);   /* *message must be zero */
int cl_com_create_message(cl_com_message_t** message);
int cl_com_free_message(cl_com_message_t** message);

/* after this line are the main functions used by lib user */
/* ======================================================= */

int cl_com_setup_tcp_connection(cl_com_connection_t** connection, int server_port, int connect_port, int data_flow_type, cl_xml_connection_autoclose_t auto_close_mode );

int cl_com_open_connection(cl_com_connection_t* connection, 
                                            int timeout, 
                             cl_com_endpoint_t* remote_endpoint, 
                             cl_com_endpoint_t* local_endpoint, 
                             cl_com_endpoint_t* receiver_endpoint ,
                             cl_com_endpoint_t* sender_endpoint);    /* CR check */

int cl_com_close_connection(cl_com_connection_t** connection);  /* CR check */

int cl_com_send_message(cl_com_connection_t* connection, 
                                         int timeout_time, 
                                  cl_byte_t* data, 
                               unsigned long size,
                              unsigned long* only_one_write);          /* CR check */

int cl_com_read_GMSH(cl_com_connection_t* connection, unsigned long *only_one_read);

int cl_com_receive_message(cl_com_connection_t* connection, 
                                            int timeout_time, 
                                     cl_byte_t* data_buffer, 
                                  unsigned long data_buffer_size, 
                                 unsigned long* only_one_read);   /* CR check */


/* This functions need service connection pointer = cl_com_connection_request_handler_setup */
/* ======================================================================================== */

/* setup service */
int cl_com_connection_request_handler_setup(cl_com_connection_t* connection,cl_com_endpoint_t* local_endpoint );   /* CR check */
/* check for new service connection clients */
int cl_com_connection_request_handler(cl_com_connection_t* connection,cl_com_connection_t** new_connection ,int timeout_val_sec, int timeout_val_usec ); /* CR check */
/* cleanup service */
int cl_com_connection_request_handler_cleanup(cl_com_connection_t* connection);  /* CR check */

/* check open connection list for new messages */
int cl_com_open_connection_request_handler(int framework_type , cl_raw_list_t* connection_list, cl_com_connection_t* service_connection, int timeout_val_sec, int timeout_val_usec, cl_select_method_t select_mode ); /* CR check */


#endif /* __CL_COMMUNICATION_H */

