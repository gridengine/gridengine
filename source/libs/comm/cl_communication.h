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

#define CL_DEFINE_READ_TIMEOUT                       15 
#define CL_DEFINE_WRITE_TIMEOUT                      15
#define CL_DEFINE_ACK_TIMEOUT                        60
#define CL_DEFINE_GET_CLIENT_CONNECTION_DATA_TIMEOUT 60   /* default  60  */    /* default timeout for accepting a connection */
#define CL_DEFINE_SYNCHRON_RECEIVE_TIMEOUT           60    /* default 60  */    /* default timeout for synchron send messages */
#define CL_DEFINE_CLIENT_CONNECTION_LIFETIME         600   /* default 600 */   /* Cut off connection when client is not active for this time */


#define CL_DEFINE_DATA_BUFFER_SIZE                   1024 * 4           /* 4 KB buffer for reading/writing messages */
#define CL_DEFINE_MAX_MESSAGE_ID                     4294967295UL       /* max unsigned long value for a 32 bit system */
#define CL_DEFINE_MAX_MESSAGE_LENGTH                 1024 * 1024 * 1024 /* 1GB max message length */

#ifdef MAXHOSTNAMELEN
#define CL_MAXHOSTNAMELEN_LENGTH MAXHOSTNAMELEN
#else
#define CL_MAXHOSTNAMELEN_LENGTH 256
#endif


/* (1) gmsh message (GMSH) */
#define CL_GMSH_MESSAGE      "<gmsh><dl>%ld</dl></gmsh>"
#define CL_GMSH_MESSAGE_SIZE 22         /* size in byte of raw CL_GMSH_MESSAGE without parameter */


/* (2) Message Information Header (MIH) */
#define CL_MIH_MESSAGE          "<mih version=\"%s\"><mid>%ld</mid><dl>%ld</dl><df>%s</df><mat>%s</mat><tag>%ld</tag><rid>%ld</rid></mih>"
#define CL_MIH_MESSAGE_SIZE     84
#define CL_MIH_MESSAGE_VERSION  "0.1"
#define CL_MIH_MESSAGE_ACK_TYPE_NAK     "nak"
#define CL_MIH_MESSAGE_ACK_TYPE_ACK     "ack"
#define CL_MIH_MESSAGE_ACK_TYPE_SYNC    "sync"
#define CL_MIH_MESSAGE_DATA_FORMAT_BIN  "bin"
#define CL_MIH_MESSAGE_DATA_FORMAT_XML  "xml"
#define CL_MIH_MESSAGE_DATA_FORMAT_AM   "am"
#define CL_MIH_MESSAGE_DATA_FORMAT_SIM  "sim"
#define CL_MIH_MESSAGE_DATA_FORMAT_SIRM "sirm"
#define CL_MIH_MESSAGE_DATA_FORMAT_CCM  "ccm"
#define CL_MIH_MESSAGE_DATA_FORMAT_CCRM "ccrm"

/* (3) Acknowledge Message (AM) */
#define CL_AM_MESSAGE       "<am version=\"%s\"><mid>%ld</mid></am>"
#define CL_AM_MESSAGE_SIZE  31
#define CL_AM_MESSAGE_VERSION "0.1"

/* (4) connect message (CM) */
#define CL_CONNECT_MESSAGE         "<cm version=\"%s\"><df>%s</df><ct>%s</ct><src host=\"%s\" comp=\"%s\" id=\"%ld\"></src><dst host=\"%s\" comp=\"%s\" id=\"%ld\"></dst><rdata host=\"%s\" comp=\"%s\" id=\"%ld\"></rdata></cm>"
#define CL_CONNECT_MESSAGE_SIZE    141
#define CL_CONNECT_MESSAGE_VERSION "0.1"
#define CL_CONNECT_MESSAGE_DATA_FORMAT_BIN   "bin"
#define CL_CONNECT_MESSAGE_DATA_FORMAT_XML   "xml"
#define CL_CONNECT_MESSAGE_DATA_FLOW_STREAM  "stream"
#define CL_CONNECT_MESSAGE_DATA_FLOW_MESSAGE "message"



/* (5) connect response message (CRM) */
#define CL_CONNECT_RESPONSE_MESSAGE                              "<crm version=\"%s\"><cs condition=\"%s\">%s</cs><src host=\"%s\" comp=\"%s\" id=\"%ld\"></src><dst host=\"%s\" comp=\"%s\" id=\"%ld\"></dst><rdata host=\"%s\" comp=\"%s\" id=\"%ld\"></rdata></crm>"
#define CL_CONNECT_RESPONSE_MESSAGE_SIZE                         147
#define CL_CONNECT_RESPONSE_MESSAGE_VERSION                      "0.1"
#define CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_OK         "connected"
#define CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_DENIED     "access denied"
#define CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_STATUS_NOT_UNIQUE "endpoint not unique"
#define CL_CONNECT_RESPONSE_MESSAGE_CONNECTION_UNSUP_DATA_FORMAT "unsupported data format"

/* (6) status information message (SIM) */
#define CL_SIM_MESSAGE               "<sim version=\"%s\"></sim>"
#define CL_SIM_MESSAGE_SIZE          sizeof(CL_SIM_MESSAGE) - 3  /* 22   TODO: can we make this easier? */ 
#define CL_SIM_MESSAGE_VERSION       "0.1"

/* (7) status information response message (SIRM) */
#define CL_SIRM_MESSAGE            "<sirm version=\"%s\"><mid>%ld</mid><starttime>%ld</starttime><runtime>%ld</runtime><application><messages><brm>%ld</brm><bwm>%ld</bwm></messages><connections><noc>%ld</noc></connections><status>%ld</status></application><info>%s</info></sirm>"
#define CL_SIRM_MESSAGE_SIZE       sizeof(CL_SIRM_MESSAGE) - 20 - 3 - 3
#define CL_SIRM_MESSAGE_VERSION    "0.1"


/* (8) connection close message (CCM) */
#define CL_CCM_MESSAGE                              "<ccm version=\"%s\"></ccm>" 
#define CL_CCM_MESSAGE_SIZE                         22
#define CL_CCM_MESSAGE_VERSION                      "0.1"

/* (9) connection close response message (CCRM) */
#define CL_CCRM_MESSAGE                              "<ccrm version=\"%s\"></ccrm>"
#define CL_CCRM_MESSAGE_SIZE                         24
#define CL_CCRM_MESSAGE_VERSION                      "0.1"





#if 0
/*
 * IPv4 Internet address
 *	This definition contains obsolete fields for compatibility
 *	with SunOS 3.x and 4.2bsd.  The presence of subnets renders
 *	divisions into fixed fields misleading at best.  New code
 *	should use only the s_addr field.
 */

#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
#define	_S_un_b	S_un_b
#define	_S_un_w	S_un_w
#define	_S_addr	S_addr
#define	_S_un	S_un
#endif /* !defined(_XPG4_2) || defined(__EXTENSIONS__) */

struct in_addr {
	union {
		struct { uint8_t s_b1, s_b2, s_b3, s_b4; } _S_un_b;
		struct { uint16_t s_w1, s_w2; } _S_un_w;
#if !defined(_XPG4_2) || defined(__EXTENSIONS__)
		uint32_t _S_addr;
#else
		in_addr_t _S_addr;
#endif /* !defined(_XPG4_2) || defined(__EXTENSIONS__) */
	} _S_un;
#define	s_addr	_S_un._S_addr		/* should be used for all code */
#define	s_host	_S_un._S_un_b.s_b2	/* OBSOLETE: host on imp */
#define	s_net	_S_un._S_un_b.s_b1	/* OBSOLETE: network */
#define	s_imp	_S_un._S_un_w.s_w2	/* OBSOLETE: imp */
#define	s_impno	_S_un._S_un_b.s_b4	/* OBSOLETE: imp # */
#define	s_lh	_S_un._S_un_b.s_b3	/* OBSOLETE: logical host */
};
#endif






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


/* endpoint helper functions */
cl_com_endpoint_t* cl_com_create_endpoint(const char* host, const char* name, unsigned long id);  /* CR check */
int  cl_com_free_endpoint(cl_com_endpoint_t** endpoint);                                          /* CR check */
int  cl_com_compare_endpoints(cl_com_endpoint_t* endpoint1, cl_com_endpoint_t* endpoint2);        /* CR check */


/* debug / help functions */
void cl_dump_connection(cl_com_connection_t* connection);                     /* CR check */
void cl_dump_private(cl_com_connection_t* connection);
void cl_com_dump_endpoint(cl_com_endpoint_t* endpoint, const char* text);                       /* CR check */
const char* cl_com_get_framework_type(cl_com_connection_t* connection);       /* CR check */
const char* cl_com_get_connection_type(cl_com_connection_t* connection);      /* CR check */
const char* cl_com_get_service_handler_flag(cl_com_connection_t* connection); /* CR check */
const char* cl_com_get_data_write_flag(cl_com_connection_t* connection);      /* CR check */
const char* cl_com_get_data_read_flag(cl_com_connection_t* connection);       /* CR check */
const char* cl_com_get_connection_state(cl_com_connection_t* connection); /* CR check */
const char* cl_com_get_data_flow_type(cl_com_connection_t* connection);       /* CR check */
const char* cl_com_get_mih_df_string(cl_xml_mih_data_format_t df);
const char* cl_com_get_mih_mat_string(cl_xml_ack_type_t mat);

/* This can be called by an signal handler to trigger abort of communications */
void cl_com_ignore_timeouts(cl_bool_t flag); 

cl_bool_t cl_com_get_ignore_timeouts_flag(void);


/* message functions */
int cl_com_setup_message(cl_com_message_t** message, cl_com_connection_t* connection, cl_byte_t* data,unsigned long size, cl_xml_ack_type_t ack_type, unsigned long response_id, unsigned long tag);   /* *message must be zero */
int cl_com_create_message(cl_com_message_t** message);
int cl_com_free_message(cl_com_message_t** message);            /* CR check */
int cl_com_free_gmsh_header(cl_com_GMSH_t** header);
int cl_com_free_cm_message(cl_com_CM_t** message);
int cl_com_free_crm_message(cl_com_CRM_t** message);
int cl_com_free_mih_message(cl_com_MIH_t** message);
int cl_com_free_am_message(cl_com_AM_t** message);
int cl_com_free_sim_message(cl_com_SIM_t** message);
int cl_com_free_sirm_message(cl_com_SIRM_t** message);
int cl_com_free_ccm_message(cl_com_CCM_t** message);
int cl_com_free_ccrm_message(cl_com_CCRM_t** message);


/* xml parsing functions */
int cl_xml_parse_GMSH(unsigned char* buffer, unsigned long buffer_length, cl_com_GMSH_t* header,unsigned long *used_buffer_length );
int cl_xml_parse_CM(unsigned char* buffer, unsigned long buffer_length, cl_com_CM_t** connection_message );
int cl_xml_parse_CRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CRM_t** connection_message );
int cl_xml_parse_MIH(unsigned char* buffer, unsigned long buffer_length, cl_com_MIH_t** message );
int cl_xml_parse_AM(unsigned char* buffer, unsigned long buffer_length, cl_com_AM_t** message );
int cl_xml_parse_SIM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIM_t** message );
int cl_xml_parse_SIRM(unsigned char* buffer, unsigned long buffer_length, cl_com_SIRM_t** message );
int cl_xml_parse_CCM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCM_t** message );
int cl_xml_parse_CCRM(unsigned char* buffer, unsigned long buffer_length, cl_com_CCRM_t** message );



/* after this line are the main functions used by lib user */
/* ======================================================= */

int cl_com_setup_tcp_connection(cl_com_connection_t** connection, int server_port, int connect_port, int data_flow_type);   /* CR check */
/* int cl_com_setup_jxta_connection(cl_com_connection_t* connection, int server_port, int connect_port); */  /* TODO: implement jxta framework */

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

