#ifndef __CL_LIST_TYPES_H
#define __CL_LIST_TYPES_H

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

#define CL_THREAD_LIST 1
#define CL_LOG_LIST    2
typedef struct cl_thread_settings_type  cl_thread_settings_t;
typedef struct cl_thread_condition_type cl_thread_condition_t;
/***********************************************************************/
/* RAW_LIST */
/***********************************************************************/
typedef struct cl_raw_list_elem_type cl_raw_list_elem_t;
struct cl_raw_list_elem_type {
  void* data;                         /* pointer to data */ 
  cl_raw_list_elem_t* next;
  cl_raw_list_elem_t* last;
};


typedef struct cl_raw_list_type {
   char*              list_name;       /* name of list */
   int                list_type;       /* type of list CL_THREAD_LIST, CL_LOG_LIST */
   pthread_mutex_t*   list_mutex;      /* list lock mutex */                 
   unsigned long      elem_count;      /* number of list entries */
   void*              list_data;       /* user list data */
   cl_raw_list_elem_t* first_elem;
   cl_raw_list_elem_t* last_elem;
} cl_raw_list_t;

/***********************************************************************/


/***********************************************************************/
/* THREAD_LIST */
/***********************************************************************/
typedef struct cl_thread_list_elem_type cl_thread_list_elem_t;
struct cl_thread_list_elem_type {
   cl_thread_settings_t* thread_config;   /* data */
   cl_raw_list_elem_t*   raw_elem;
};




/***********************************************************************/

/***********************************************************************/
/* LOG_LIST */
/***********************************************************************/
typedef struct cl_log_list_elem_type cl_log_list_elem_t;
struct cl_log_list_elem_type {                      /* list element specific data */
   char*                 log_parameter;                  /* additional parameter */
   char*                 log_message;                    /* log message */
   char*                 log_thread_name;                /* name of thread  */
   int                   log_thread_id;                  /* thread id   */
   int                   log_thread_state;               /* state of thread */
   int                   log_type;                       /* log level type */
   char*                 log_module_name;                /* name of c - module */
   cl_raw_list_elem_t*   raw_elem;
};


typedef enum cl_log_list_flush_method_type {
   CL_LOG_FLUSHED,        /* flushing is done when ever user calls cl_log_list_flush() */
   CL_LOG_IMMEDIATE       /* cl_log_list_flush() is called on each log  */
} cl_log_list_flush_method_t;

typedef int (*cl_log_func_t)(cl_raw_list_t* log_list);

typedef struct cl_log_list_data_type {                      /* list specific data */
   int                         current_log_level;           /* current log level */
   cl_log_list_flush_method_t  flush_type;                  /* flushing type */
   cl_log_func_t               flush_function;              /* function called for flushing */
   cl_thread_settings_t*       list_creator_settings;       /* log list creator thread settings */
} cl_log_list_data_t;

/***********************************************************************/



#endif /* __CL_LIST_TYPES_H */

