#ifndef __CL_COMMLIB_H
#define __CL_COMMLIB_H


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



#include "cl_lists.h"
#include "cl_data_types.h"
#include "cl_communication.h"




/* commlib init and hostlist functions */
cl_raw_list_t* cl_com_get_host_list(void);
cl_raw_list_t* cl_com_get_log_list(void);
cl_raw_list_t* cl_com_get_endpoint_list(void);

/* application log functions */
int cl_commlib_push_application_error(int cl_error, const char* cl_info);

int cl_com_setup_commlib(cl_thread_mode_t t_mode, cl_log_t debug_level , cl_log_func_t flush_func);
int cl_com_cleanup_commlib(void);


/* local handle functions */
cl_com_handle_t* cl_com_create_handle(int*                      commlib_error,
                                      cl_framework_t            framework,
                                      cl_xml_connection_type_t  data_flow_type ,
                                      cl_bool_t                 service_provider ,
                                      int                       port,
                                      char*                     component_name, 
                                      unsigned long             component_id, 
                                      int                       select_sec_timeout, 
                                      int                       select_usec_timeout);

int cl_commlib_shutdown_handle   (cl_com_handle_t* handle, cl_bool_t return_for_messages );

cl_com_handle_t* cl_com_get_handle(char*         component_name, 
                                   unsigned long component_id);  /* CR check */

/* commlib parameter functions */
int cl_com_set_synchron_receive_timeout(cl_com_handle_t* handle, int timeout);

int cl_com_set_handle_fds(cl_com_handle_t* handle, fd_set* file_descriptor_set);
int cl_com_get_service_port(cl_com_handle_t* handle, int* port);
int cl_com_get_connect_port(cl_com_handle_t* handle, int* port);

int cl_com_add_allowed_host    (cl_com_handle_t* handle, char* hostname);
int cl_com_remove_allowed_host (cl_com_handle_t* handle, char* hostname);

int cl_com_set_alias_file(char* alias_file);
int cl_com_set_alias_file_dirty(void);
int cl_com_append_host_alias(char* local_resolved_name, char* alias_name);
int cl_com_remove_host_alias(char* alias_name);


int cl_com_append_known_endpoint_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, int service_port, cl_xml_connection_autoclose_t autoclose, cl_bool_t is_static );
int cl_com_remove_known_endpoint_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id);
int cl_com_get_known_endpoint_port_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, int* service_port );
int cl_com_get_known_endpoint_autoclose_mode_from_name(char* unresolved_comp_host, char* comp_name, unsigned long comp_id, cl_xml_connection_autoclose_t* auto_close_mode );

int cl_com_append_known_endpoint(cl_com_endpoint_t* endpoint, int service_port, cl_xml_connection_autoclose_t autoclose, cl_bool_t is_static );
int cl_com_remove_known_endpoint(cl_com_endpoint_t* endpoint);
int cl_com_get_known_endpoint_port(cl_com_endpoint_t* endpoint, int* service_port );
int cl_com_get_known_endpoint_autoclose_mode(cl_com_endpoint_t* endpoint, cl_xml_connection_autoclose_t* auto_close_mode );


int cl_com_set_max_connections (cl_com_handle_t* handle, int value);
int cl_com_get_max_connections (cl_com_handle_t* handle, int* value);
int cl_com_set_auto_close_mode(cl_com_handle_t* handle, cl_xml_connection_autoclose_t  mode );
int cl_com_get_auto_close_mode(cl_com_handle_t* handle, cl_xml_connection_autoclose_t* mode );
int cl_com_set_max_connection_close_mode(cl_com_handle_t* handle, cl_max_count_t mode);
int cl_com_get_max_connection_close_mode(cl_com_handle_t* handle, cl_max_count_t* mode);

/* application can set application status for SIRM messages */
int cl_com_set_status_func(cl_app_status_func_t status_func);
/* application can set an error function */
int cl_com_set_error_func(cl_error_func_t error_func);
int cl_com_set_tag_name_func(cl_tag_name_func_t tag_name_func);
int cl_com_set_debug_client_mode(cl_com_handle_t* handle, cl_debug_client_t mode);
int cl_com_setup_callback_functions(cl_com_connection_t* connection);




cl_thread_mode_t cl_commlib_get_thread_state(void);


int cl_com_setup_connection      (cl_com_handle_t* handle, 
                                  cl_com_connection_t** connection );

/* 
TODO: cleanup function names !!!
TODO: ADOC Header !!!
*/

/* typical user function calls */

int cl_commlib_trigger           (cl_com_handle_t* handle);


int cl_commlib_close_connection  (cl_com_handle_t* handle, 
                                  char* un_resolved_hostname, char* component_name, unsigned long component_id,
                                  cl_bool_t return_for_messages );
int cl_commlib_open_connection   (cl_com_handle_t* handle,
                                  char* un_resolved_hostname, char* component_name, unsigned long component_id);


int cl_commlib_receive_message   (cl_com_handle_t* handle, 
                                  char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                                  cl_bool_t synchron, 
                                  unsigned long response_mid, 
                                  cl_com_message_t** message, 
                                  cl_com_endpoint_t** sender );

int cl_commlib_send_message       (cl_com_handle_t* handle,
                                  char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                                  cl_xml_ack_type_t ack_type , 
                                  cl_byte_t* data, unsigned long size , 
                                  unsigned long* mid , 
                                  unsigned long response_mid, 
                                  unsigned long tag, 
                                  cl_bool_t copy_data, 
                                  cl_bool_t wait_for_ack );

int cl_commlib_check_for_ack      (cl_com_handle_t* handle, 
                                   char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                                   unsigned long mid , 
                                   cl_bool_t do_block);

int cl_commlib_get_endpoint_status(cl_com_handle_t* handle,
                                   char* un_resolved_hostname, char* component_name, unsigned long component_id,
                                   cl_com_SIRM_t** status);
                                   

int cl_commlib_search_endpoint    (cl_com_handle_t* handle,
                                   char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                                   cl_bool_t only_connected,
                                   cl_raw_list_t** endpoint_list);

/* defines from old commlib */

#define HEARD_FROM_TIMEOUT 1  /* dummy parameter */
int cl_commlib_set_connection_param(cl_com_handle_t* handle, int parameter, int value);
int cl_commlib_get_connection_param(cl_com_handle_t* handle, int parameter, int* value);
int cl_commlib_get_last_message_time(cl_com_handle_t* handle, char* un_resolved_hostname, char* component_name, unsigned long component_id, unsigned long* msg_time);


/* dummy defines for old lib compatibility */

#define COMMD_NACK_UNKNOWN_HOST CL_RETVAL_UNKOWN_HOST_ERROR  /* TODO: check this define */
#define COMMD_NACK_CONFLICT CL_RETVAL_ENDPOINT_NOT_UNIQUE
#define CL_FIRST_FREE_EC    32

#ifndef MAXHOSTLEN
#define MAXHOSTLEN CL_MAXHOSTNAMELEN_LENGTH   /* TODO: remove this define */
#endif

int getuniquehostname(const char *hostin, char *hostout, int refresh_aliases);

#endif /* __CL_COMMLIB_H */

