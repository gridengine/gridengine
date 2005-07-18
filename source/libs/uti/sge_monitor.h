#ifndef _SGE_MONITOR_H
#define _SGE_MONITOR_H

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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <sys/time.h>

#include "basis_types.h"

/**
 * Monitoring functionality:
 * -------------------------
 *
 * - qping health monitoring
 *
 * - keeping statistics on what is done during a thread loop
 *
 * - outputing the statistics information via message file or
 *   qping
 *
 *
 * Monitoring Usage:
 * -----------------
 *
 * do a normal data definition, call init and free, when you are done.
 * You have to call MONITOR_IDLE_TIME and sge_monitor_output. After that
 * everything is up to you to design...
 *
 * -----start thread --------------
 *    monitoring_t monitor;
 *   
 *    sge_monitor_init(&monitor, "THREAD NAME", <EXTENSION>, <WARNING>, <ERROR>);
 *   
 *    <thread loop> {
 *       
 *       MONITOR_IDLE_TIME(<wait for something>,(&monitor), monitor_time);
 *   
 *      < do your stuff and monitoring >
 *   
 *       sge_monitor_output(&monitor);
 *    }
 *    sge_monitor_free(&monitor);
 * ------end thread----------------
 *
 * Improtant:
 * ----------
 *  The call to MONITOR_IDLE_TIME has to be the first one after the thread loop otherwise
 *  certain parts of the monitoring structure are not correctly initilized.
 *
 * General statistic methods:
 * --------------------------
 *
 * - MONITOR_IDLE_TIME    : counts idle time, very important, nothing works without it
 * - MONITOR_WAIT_TIME    : counts wait time (wait for a lock usually)
 * - MONITOR_MESSAGES     : counts how many times the thread loop is executed
 * - MONITOR_MESSAGES_OUT : counts how many messages are send
 *
 * GDI statistics methods:
 * -----------------------
 *
 * - MONITOR_GDI  : counts GDI requests
 * - MONITOR_ACK  : counts ACKs
 * - MONITOR_LOAD : counts reports
 */


/**
 * qping thread warning times in seconds
 */
typedef enum {
   NO_WARNING    = 0,
   EMT_WARNING   = 10,
   TET_WARNING   = 30,
   MT_WARNING    = 10,
   ST_WARNING    = 0,  /* no timeout for this thread */
   EXECD_WARNING = 10   
}thread_warning_t;

/**
 * qping thread error times in seconds
 **/
typedef enum {
   NO_ERROR    = 0,
   EMT_ERROR   = 600,
   TET_ERROR   = 600,
   MT_ERROR    = 600,
   ST_ERROR    = 0,   /* no timeout for this thread */
   EXECD_ERROR = 600   
}thread_error_t;

/**
 * This function definition is the prototyp for the output function of a data
 * extension
 */
typedef void (*extension_output)(
   char *info_message,       /* target memory buffer*/
   int  size,                /* length of the memory buffer */               
   void *monitor_extension,  /* contains the monitor extension structur */
   double time               /* length of the time inteval */
);

/**
 * This enum identifies all available extensions
 */
typedef enum {
   NONE_EXT = -1,
   GDI_EXT = 0
}extension_t;

/**
 * the monitoring data structure
 */
typedef struct {
   /*--- init data ------------*/
   const char *thread_name;
   u_long32    monitor_time;        /* stores the time interval for the mesuring run */
   /*--- output data ----------*/
   char *output_line1;
   char *output_line2;
   char *work_line;
   int  pos;                        /* position (line) in the qping output structure (kind of thread id) */
   /*--- work data ------------*/
   struct timeval now;              /* start time of mesurement */
   bool        output;              /* if true, triggers qping / message output */
   u_long32    message_in_count;
   u_long32    message_out_count;
   double      idle;                /* idle time*/
   double      wait;                /* wait time*/
   /*--- extension data -------*/
   extension_t       ext_type; 
   void             *ext_data;
   u_long32          ext_data_size;
   extension_output  ext_output; 
} monitoring_t;

void sge_monitor_init(monitoring_t *monitor, const char *thread_name, extension_t ext, 
                 thread_warning_t warning_timeout, thread_error_t error_timeout);
void sge_monitor_free(monitoring_t *monitor);
u_long32 sge_monitor_status(char **info_message, u_long32 monitor_time);
void sge_set_last_wait_time(monitoring_t *monitor, struct timeval after); 

void sge_monitor_output(monitoring_t *monitor); 
void sge_monitor_reset(monitoring_t *monitor);


/****************
 * MACRO section
 ****************/

#define MONITOR_IDLE_TIME(execute, monitor, output_time)    { \
                                 struct timeval before;  \
                                 gettimeofday(&before, NULL); \
                                 sge_set_last_wait_time(monitor, before); \
                                 if (output_time > 0) { \
                                    struct timeval before;  \
                                    struct timeval after; \
                                    double time; \
                                    \
                                    monitor->monitor_time = output_time; \
                                    gettimeofday(&before, NULL); \
                                    if (monitor->now.tv_sec == 0) { \
                                       monitor->now = before; \
                                    } \
                                    execute; \
                                    gettimeofday(&after, NULL);  \
                                    monitor->output = ((after.tv_sec-monitor->now.tv_sec) >= monitor->monitor_time)?true:false; \
                                    time = after.tv_usec - before.tv_usec; \
                                    time = after.tv_sec - before.tv_sec + (time/1000000); \
                                    monitor->idle += time; \
                                 } \
                                 else { \
                                    execute; \
                                 } \
                              } \

/**
 * This might pose a problem if it is called with another makro. 
 *
 * TODO: it should be customized for read/write locks.
 */
#define MONITOR_WAIT_TIME(execute, monitor)    if (monitor->monitor_time > 0){ \
                                    struct timeval before;  \
                                    struct timeval after; \
                                    double time; \
                                    \
                                    gettimeofday(&before, NULL); \
                                    execute; \
                                    gettimeofday(&after, NULL);  \
                                    time = after.tv_usec - before.tv_usec; \
                                    time = after.tv_sec - before.tv_sec + (time/1000000); \
                                    monitor->wait += time; \
                                 } \
                                 else { \
                                    execute; \
                                 } \

#define MONITOR_MESSAGES(monitor) if (monitor->monitor_time > 0) monitor->message_in_count++

#define MONITOR_MESSAGES_OUT(monitor) if ((monitor != NULL) && (monitor->monitor_time > 0)) monitor->message_out_count++

/*--------------------------------*/
/*   EXTENSION SECTION            */
/*--------------------------------*/

/**
 * What you need to do to create a new extension:
 *
 * - create a new extension_t in the enum
 * - define a extension data structure
 * - modifiy the sge_monitor_init method to handle the new extension type
 *   Example:
 *     case GDI_EXT :
 *          monitor->ext_data_size = sizeof(m_gdi_t);
 *          monitor->ext_data = malloc(sizeof(m_gdi_t));
 *          monitor->ext_output = &ext_gdi_output;
 *       break;
 *
 * - write the extension output function
 * - write the measurement makros
 * - remember, that the entire extension structure is reset to 0 after the data is printed
 *
 **/


/* GDI message thread extensions */

typedef struct {
   u_long32    gdi_count; /* counts the gdi requests (a gdi-multi is only one request */
   u_long32    load_count;/* counts the execd load/job reports*/
   u_long32    act_count; /* counts all kind of aknowledges */
}m_gdi_t;

#define MONITOR_GDI(monitor)     if ((monitor->monitor_time > 0) && (monitor->ext_type == GDI_EXT)) ((m_gdi_t*)(monitor->ext_data))->gdi_count++

#define MONITOR_ACK(monitor)     if ((monitor->monitor_time > 0) && (monitor->ext_type == GDI_EXT)) ((m_gdi_t*)(monitor->ext_data))->act_count++

#define MONITOR_LOAD(monitor)    if ((monitor->monitor_time > 0) && (monitor->ext_type == GDI_EXT)) ((m_gdi_t*)(monitor->ext_data))->load_count++

#endif /* _SGE_MONITIR_H */
