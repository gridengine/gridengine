#ifndef __CL_DATA_TYPES_H
#define __CL_DATA_TYPES_H
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

/* implemented communication frameworks 
   for cl_com_connection_t->framework_type flag
*/
#include <sys/param.h>
#include <netdb.h>
#include "cl_lists.h"


#define CL_CT_TCP    1   /* on work */
#define CL_CT_GLOBUS 2   /* not done: TODO */
#define CL_CT_JXTA   3   /* not done: TODO */


typedef enum cl_select_method_def {
   CL_RW_SELECT,
   CL_R_SELECT,
   CL_W_SELECT
} cl_select_method_t;

typedef enum cl_host_resolve_method_def {
   CL_SHORT = 1,
   CL_LONG  = 2
} cl_host_resolve_method_t;

typedef enum cl_thread_mode_def {
   CL_NO_THREAD,         /* application must call cl_commlib_trigger() in main loop */
   CL_ONE_THREAD         /* only one trigger thread */
} cl_thread_mode_t;

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

typedef enum cl_xml_connection_status_def {
   CL_CRM_CS_UNDEFINED = 1,
   CL_CRM_CS_CONNECTED,
   CL_CRM_CS_DENIED,
   CL_CRM_CS_UNSUPPORTED
}cl_xml_connection_status_t ;



typedef unsigned char cl_byte_t;
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

/* connection types for cl_com_connection_t->connection_type flag */
#define CL_COM_RECEIVE      1
#define CL_COM_SEND         2
#define CL_COM_SEND_RECEIVE 3
#define CL_COM_UNDEFINED    4

/* connection types for cl_com_connection_t->data_write_flag and data_read_flag flag */
#define CL_COM_DATA_READY     1
#define CL_COM_DATA_NOT_READY 2

/* connection types for cl_com_connection_t->service_handler_flag flag */
#define CL_COM_SERVICE_HANDLER     1
#define CL_COM_CONNECTION          2
#define CL_COM_SERVICE_UNDEFINED   3

/*connection types for cl_com_connection_t->connection_state */
#define CL_COM_DISCONNECTED 1
#define CL_COM_CLOSING      2
#define CL_COM_OPENING      3
#define CL_COM_CONNECTING   4
#define CL_COM_CONNECTED    5

/*connection types for cl_com_connection_t->connection_sub_state */

/* when CL_COM_OPENING */
#define CL_COM_OPEN_INIT      1
#define CL_COM_OPEN_CONNECT   2
#define CL_COM_OPEN_CONNECTED 3


/* when CL_COM_CONNECTING */
#define CL_COM_READ_INIT      1
#define CL_COM_READ_GMSH      2
#define CL_COM_READ_CM        3
#define CL_COM_READ_INIT_CRM  4
#define CL_COM_READ_SEND_CRM  5
#define CL_COM_SEND_INIT      6
#define CL_COM_SEND_CM        7
#define CL_COM_SEND_READ_GMSH 8
#define CL_COM_SEND_READ_CRM  9

/* when CL_COM_CONNECTED */
#define CL_COM_WORK            1
#define CL_COM_RECEIVED_CCM    2
#define CL_COM_SENDING_CCM     3
#define CL_COM_WAIT_FOR_CCRM   5
#define CL_COM_SENDING_CCRM    6
#define CL_COM_CCRM_SENT       7
#define CL_COM_DONE            8
#define CL_COM_DONE_FLUSHED    9

/*#define CL_COM_IDLE  1
#define CL_COM_READ  2
#define CL_COM_WRITE 3 */



typedef struct cl_com_endpoint {
   char*    comp_host;           
   char*    comp_name;
   unsigned long  comp_id;
} cl_com_endpoint_t ;



/*  the struct timeval is defined as follows:  
 *
 *  struct timeval {
 *	    time_t        tv_sec;      seconds 
 *	    suseconds_t   tv_usec;     and microseconds 
 *  };       */

typedef enum cl_message_state_type {
   CL_MS_UNDEFINED = 1,
   CL_MS_INIT_SND,
   CL_MS_SND_GMSH,
   CL_MS_SND_MIH,
   CL_MS_SND,
   CL_MS_INIT_RCV,
   CL_MS_RCV_GMSH,
   CL_MS_RCV_MIH,
   CL_MS_RCV,
   CL_MS_READY,
   CL_MS_PROTOCOL    /* must be higherst name */
}cl_message_state_t;




typedef struct cl_com_handle_statistic_type {
   struct timeval   last_update;               /* last calculation time */
   unsigned long    new_connections;           /* nr of new connections since last_update */
   unsigned long    access_denied;             /* nr of connections, where access was denied */
   unsigned long    nr_of_connections;         /* nr of open connections */
   unsigned long    bytes_sent ;               /* bytes send since last_update */
   unsigned long    bytes_received;            /* bytes received since last_update */
   unsigned long    real_bytes_sent ;          /* bytes send since last_update */
   unsigned long    real_bytes_received;       /* bytes received since last_update */
   unsigned long    unsend_message_count;      /* nr of messages to send */
   unsigned long    unread_message_count;      /* nr of buffered received messages, waiting for application to pick it up */
   unsigned long    application_status;        /* status of application */
} cl_com_handle_statistic_t;

typedef struct cl_com_connection_type cl_com_connection_t;

typedef struct cl_com_handle {
   int framework;                   /* framework type CL_CT_TCP, CL_CT_JXTA */
   int data_flow_type;              /* data_flow type CL_COM_STREAM, CL_COM_MESSAGE */
   int service_provider;            /* if true this component will provide a service for clients (server port) */
   int connect_port;                /* used port number to connect to other service */
   int service_port;                /* local used service port */
   cl_com_endpoint_t* local;        /* local endpoint id of this handle */
   cl_com_handle_statistic_t* statistic; /* statistic data of handle */

/* Threads */
   cl_thread_condition_t* read_condition;  /* condition variable for data write */
   cl_thread_condition_t* write_condition; /* condition variable for data read */
   cl_thread_settings_t*  service_thread;  /* pointer to cl_com_handle_service_thread() thread pointer */
   cl_thread_settings_t*  read_thread;   
   cl_thread_settings_t*  write_thread;
/* Threads done */
   
   pthread_mutex_t* messages_ready_mutex;
   unsigned long messages_ready_for_read;

   pthread_mutex_t* connection_list_mutex;
   cl_raw_list_t* connection_list;  /* connections of this handle */
   cl_raw_list_t* allowed_host_list; /* string list with hostnames allowed to connect */

   int max_open_connections; /* maximum number of open connections  */
   int max_con_close_state;  /* enabled/disabled state of auto close at max connection count */
   int max_write_threads;    /* maximum number of send threads */
   int max_read_threads;     /* maximum number of receive threas */
   int select_sec_timeout;
   int select_usec_timeout;
   int connection_timeout;   /* timeout to shutdown connected clients when no messages arive */ 
   int read_timeout;
   int write_timeout;
   int open_connection_timeout; 
   int acknowledge_timeout;
   int synchron_receive_timeout;
   
   /* service specific */
   int do_shutdown;                        /* set when this handle wants to shutdown */
   int max_connection_count_reached;       /* set when max connection count is reached */
   long shutdown_timeout;                  /* used when shutting down handle */
   cl_com_connection_t* service_handler;   /* service handler of this handle */
   struct timeval start_time;

} cl_com_handle_t;




typedef struct cl_com_hostent {
   struct hostent *he;              /* pointer of type struct hostent (defined in netdb.h) */
   char*  he_data_buffer;           /* all struct member pointers point to data in this buffer */
} cl_com_hostent_t;

/*  the hostent struct should be defined in the following way (system header)
      struct hostent {
         char    *h_name;          canonical name of host 
         char    **h_aliases;      alias list 
         int     h_addrtype;       host address type 
         int     h_length;         length of address 
         char    **h_addr_list;    list of addresses 
     };
*/

typedef struct cl_com_host_spec_type {
   cl_com_hostent_t* hostent;
   int               resolve_error;     /* CL_RETVAL_XXX from  cl_com_gethostbyname() call */
   char*             unresolved_name;
   char*             resolved_name;
   struct in_addr*   in_addr;
   long              last_resolve_time; 
   long              creation_time;

} cl_com_host_spec_t;


/* XML data types */
typedef struct cl_com_GMSH_type {
   unsigned long   dl;
} cl_com_GMSH_t;


typedef struct cl_com_CM_type {
   char*                    version;
   cl_xml_data_format_t     df;
   cl_xml_connection_type_t ct;
   cl_com_endpoint_t*       src;
   cl_com_endpoint_t*       dst;
   cl_com_endpoint_t*       rdata;
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



typedef struct cl_com_message_type {
   cl_message_state_t       message_state;
   cl_xml_mih_data_format_t message_df;
   cl_xml_ack_type_t        message_mat;
   int                      message_ack_flag;
   cl_com_SIRM_t*           message_sirm;  /* if NOT NULL this was the response to a SIM */
   unsigned long            message_tag;
   unsigned long            message_id;
   unsigned long            message_response_id;  /* if set, this message is a response for this message_id */
   unsigned long            message_length;
   unsigned long            message_snd_pointer;
   unsigned long            message_rcv_pointer;
   struct timeval           message_receive_time;
   struct timeval           message_send_time;
   cl_byte_t*               message;
} cl_com_message_t;


typedef struct cl_com_con_statistic_type {
   struct timeval   last_update;                 /* last calculation time */
   unsigned long    bytes_sent ;                 /* bytes send since last_update */
   unsigned long    bytes_received;              /* bytes received since last_update */
   unsigned long    real_bytes_sent;
   unsigned long    real_bytes_received;
} cl_com_con_statistic_t;



struct cl_com_connection_type {

  
   cl_com_endpoint_t* remote;   /* dst on local host in CM */
   cl_com_endpoint_t* local;    /* src on local host in CM */
   cl_com_endpoint_t* sender;   /* for routing */
   cl_com_endpoint_t* receiver; /* for routing  ( rdata ) */

   unsigned long    last_send_message_id;
   cl_raw_list_t*   received_message_list;
   
   cl_raw_list_t*   send_message_list;
   cl_com_handle_t* handler;           /* this points to the handler of the connection */
   int           ccm_received;
   int           ccm_sent;
   int           ccrm_sent;
   int           ccrm_received;
   int           framework_type;          /* CL_CT_TCP, ... */
   int           connection_type;         /* CL_COM_RECEIVE, CL_COM_SEND or CL_COM_SEND_RECEIVE  */
   int           service_handler_flag;    /* CL_COM_SERVICE_HANDLER or CL_COM_CONNECTION or CL_COM_SERVICE_UNDEFINED*/
   int           data_write_flag;         /* CL_COM_DATA_READY or CL_COM_DATA_NOT_READY */ 
   int           fd_ready_for_write;      /* set by cl_com_open_connection_request_handler() when data_write_flag is CL_COM_DATA_READY 
                                             and the write is possible (values are CL_COM_DATA_READY or CL_COM_DATA_NOT_READY) */
   int           data_read_flag;          /* CL_COM_DATA_READY or CL_COM_DATA_NOT_READY */
   int           connection_state;        /* CL_COM_DISCONNECTED,CL_COM_CLOSING ,CL_COM_CONNECTED ,CL_COM_CONNECTING */
   int           connection_sub_state;    /* depends on connection_state */
   int           is_client;               /* is set when this is a client connection (from accept) */
   char*         client_host_name;        /* this is the resolved client host name */
   cl_xml_connection_status_t crm_state;  /* state of connection response message (if server) */
   
   /* dataflow */
   cl_xml_connection_type_t data_flow_type;       /* CL_CM_CT_STREAM or CL_CM_CT_MESSAGE */   
   cl_xml_data_format_t     data_format_type;     /* CL_CM_DF_BIN or CL_CM_DF_XML */
 
   /* data buffer */
   unsigned long  data_buffer_size;             /* connection data buffer size for read/write messages */
   cl_byte_t*     data_read_buffer;             /* connection data buffer for read operations */
   cl_byte_t*     data_write_buffer;            /* connection data buffer for write operations */
   cl_com_GMSH_t* read_gmsh_header;             /* used to store gmsh data length for reading */

   long          read_buffer_timeout_time;     /* timeout for current read */
   long          write_buffer_timeout_time;    /* timeout for current write */

   unsigned long data_write_buffer_pos;        /* actual position in data write buffer */
   unsigned long data_write_buffer_processed;  /* actual position in data write buffer which is processed */
   unsigned long data_write_buffer_to_send;        /* position of last data byte to write */

   unsigned long data_read_buffer_pos;         /* actual position in data read buffer */
   unsigned long data_read_buffer_processed;   /* actual position in data read buffer which is processed */
 
   struct timeval last_transfer_time;           /* time when last message arived/was sent */
   long           shutdown_timeout;             /* used for shutdown of connection */
 
   /* connection specific */
   cl_com_con_statistic_t* statistic;
   void*         com_private;
};



#endif /* __CL_DATA_TYPES_H */
