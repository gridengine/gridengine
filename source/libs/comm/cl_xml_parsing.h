#ifndef __CL_XML_PARSING_H
#define __CL_XML_PARSING_H

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

#define CL_DEFINE_MAX_MESSAGE_LENGTH                 1024 * 1024 * 1024 /* 1GB max message length */



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
#define CL_CONNECT_MESSAGE         "<cm version=\"%s\"><df>%s</df><ct>%s</ct><src host=\"%s\" comp=\"%s\" id=\"%ld\"></src><dst host=\"%s\" comp=\"%s\" id=\"%ld\"></dst><rdata host=\"%s\" comp=\"%s\" id=\"%ld\"></rdata><port>%ld</port><ac>%s</ac></cm>"
#define CL_CONNECT_MESSAGE_SIZE    141 + ( 22 )
#define CL_CONNECT_MESSAGE_VERSION "0.2"
#define CL_CONNECT_MESSAGE_DATA_FORMAT_BIN    "bin"
#define CL_CONNECT_MESSAGE_DATA_FORMAT_XML    "xml"
#define CL_CONNECT_MESSAGE_DATA_FLOW_STREAM   "stream"
#define CL_CONNECT_MESSAGE_DATA_FLOW_MESSAGE  "message"
#define CL_CONNECT_MESSAGE_AUTOCLOSE_ENABLED  "enabled"
#define CL_CONNECT_MESSAGE_AUTOCLOSE_DISABLED "disabled"



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


typedef enum cl_bool_def {
   CL_FALSE = 0,
   CL_TRUE
} cl_bool_t;


typedef struct cl_com_endpoint {
   /* internal identification tripple */
   char*         comp_host;           
   char*         comp_name;
   unsigned long comp_id;
} cl_com_endpoint_t ;



typedef enum cl_xml_data_format_def {
   CL_CM_DF_UNDEFINED = 1,
   CL_CM_DF_BIN,
   CL_CM_DF_XML
} cl_xml_data_format_t;
typedef enum cl_xml_mih_data_format_def {
   CL_MIH_DF_UNDEFINED = 1,
   CL_MIH_DF_BIN,
   CL_MIH_DF_XML,
   CL_MIH_DF_AM,
   CL_MIH_DF_SIM,
   CL_MIH_DF_SIRM,
   CL_MIH_DF_CCM,
   CL_MIH_DF_CCRM
} cl_xml_mih_data_format_t;

typedef enum cl_xml_ack_type_def {
   CL_MIH_MAT_UNDEFINED = 1,
   CL_MIH_MAT_NAK,
   CL_MIH_MAT_ACK,
   CL_MIH_MAT_SYNC
} cl_xml_ack_type_t;

typedef enum cl_xml_connection_type_def {
   CL_CM_CT_UNDEFINED = 1,
   CL_CM_CT_STREAM,
   CL_CM_CT_MESSAGE
}cl_xml_connection_type_t ;

typedef enum cl_xml_connection_autoclose_def {
   CL_CM_AC_UNDEFINED = 1,
   CL_CM_AC_ENABLED,
   CL_CM_AC_DISABLED
}cl_xml_connection_autoclose_t;

typedef enum cl_xml_connection_status_def {
   CL_CRM_CS_UNDEFINED = 1,         /* 1 */
   CL_CRM_CS_CONNECTED,             /* 2 */
   CL_CRM_CS_DENIED,                /* 3 */
   CL_CRM_CS_ENDPOINT_NOT_UNIQUE,   /* 4 */
   CL_CRM_CS_UNSUPPORTED            /* 5 */
}cl_xml_connection_status_t ;


/* XML data types */
typedef struct cl_com_GMSH_type {
   unsigned long   dl;
} cl_com_GMSH_t;

typedef struct cl_com_CM_type {
   char*                          version;
   cl_xml_data_format_t           df;
   cl_xml_connection_type_t       ct;
   cl_xml_connection_autoclose_t  ac;
   unsigned long                  port;  
   cl_com_endpoint_t*             src;
   cl_com_endpoint_t*             dst;
   cl_com_endpoint_t*             rdata;
} cl_com_CM_t;


typedef struct cl_com_CRM_type {
   char*                         version;
   cl_xml_connection_status_t    cs_condition;
   char*                         cs_text;
   char*                         formats;   /* each format is seperated with "," not supported TODO  */
   cl_com_endpoint_t*            src;
   cl_com_endpoint_t*            dst;
   cl_com_endpoint_t*            rdata;
} cl_com_CRM_t;

typedef struct cl_com_MIH_type {
   char*                      version;
   unsigned long              mid;
   unsigned long              dl;
   cl_xml_mih_data_format_t   df;
   cl_xml_ack_type_t          mat;
   unsigned long              tag;
   unsigned long              rid;
} cl_com_MIH_t;

typedef struct cl_com_AM_type {
   char*            version;
   unsigned long    mid;
} cl_com_AM_t;

typedef struct cl_com_SIM_type {
   char*            version;
} cl_com_SIM_t;

typedef struct cl_com_SIRM_type {
   char*            version;
   unsigned long    mid;
   unsigned long    starttime;
   unsigned long    runtime;
   unsigned long    application_messages_brm;
   unsigned long    application_messages_bwm;
   unsigned long    application_connections_noc;
   unsigned long    application_status;
   char*            info;
} cl_com_SIRM_t;

typedef struct cl_com_CCM_type {
   char*            version;
} cl_com_CCM_t;

typedef struct cl_com_CCRM_type {
   char*            version;
} cl_com_CCRM_t;



const char* cl_com_get_mih_df_string(cl_xml_mih_data_format_t df);
const char* cl_com_get_mih_mat_string(cl_xml_ack_type_t mat);


/* endpoint helper functions */
cl_com_endpoint_t* cl_com_create_endpoint(const char* host, const char* name, unsigned long id);  /* CR check */
cl_com_endpoint_t* cl_com_dup_endpoint(cl_com_endpoint_t* endpoint);
int  cl_com_free_endpoint(cl_com_endpoint_t** endpoint);                                          /* CR check */



/* message functions */
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


#endif /* __CL_XML_PARSING_H */

