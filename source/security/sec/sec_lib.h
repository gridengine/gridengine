#ifndef __SEC_LIB_H 
#define __SEC_LIB_H
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

#include "cl_data_types.h"
void sec_mt_init(void);

int sec_init(const char *progname);
int sec_exit(void);

int sec_verify_user(const char *user, const char *commproc);

int sec_clear_connectionlist(void);

int sec_send_message(cl_com_handle_t* handle,
                     char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                     cl_xml_ack_type_t ack_type, 
                     cl_byte_t* data, unsigned long size , 
                     unsigned long* mid, unsigned long response_mid, unsigned long tag ,
                     int copy_data,
                     int wait_for_ack);
int sec_receive_message(cl_com_handle_t* handle,
                        char* un_resolved_hostname, char* component_name, unsigned long component_id, 
                        int synchron,
                        unsigned long response_mid, 
                        cl_com_message_t** message, 
                        cl_com_endpoint_t** sender);


#endif /* __SEC_LIB_H */
