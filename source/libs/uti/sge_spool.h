#ifndef _SGE_FILE_PATH_H_
#define _SGE_FILE_PATH_H_
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

#include "basis_types.h"

#define COMMENT_CHAR '#'

/****** uti/spool/sge_file_path_id_t ******************************************
*  NAME
*     sge_file_path_id_t -- Type of filename or pathname
*
*  SYNOPSIS
*     typedef enum {
*        JOBS_SPOOL_DIR,
*        JOB_SPOOL_DIR,
*        JOB_SPOOL_DIR_AS_FILE,
*        JOB_SPOOL_FILE,
*        TASK_SPOOL_DIR,
*        TASK_SPOOL_FILE,
*        JOB_SCRIPT_DIR,
*        JOB_SCRIPT_FILE,
*        JOB_ACTIVE_DIR 
*     } sge_file_path_id_t;
*
*  FUNCTION
*     Type of filename or pathname if no other sge_spool_flags_t 
*     and/or sge_file_path_format_t are specified 
*     with sge_get_file_path():
*
*     JOBS_SPOOL_DIR - "./jobs"
*
*     JOB_SPOOL_DIR - "./jobs/xx/yyyy/zzzz" 
*                     zzzz is a directory
*                     'xxyyyyzzzz' is a job id
*                    
*     JOB_SPOOL_DIR_AS_FILE - "./jobs/xx/yyyy/zzzz"
*                             zzzz is a file
*                             'xxyyyyzzzz' is a job id  
*
*     JOB_SPOOL_FILE - "./jobs/xx/yyyy/zzzz/common"
*
*     TASK_SPOOL_DIR - "./jobs/xx/yyyy/zzzz/1-4096"
*                      (example for task ids between 1 and 4096)
*                      
*     TASK_SPOOL_FILE - "./jobs/xx/yyyy/zzzz/1-4096/1"
*                       (example for task with id 1)
*
*     JOB_SCRIPT_DIR - "./job_scripts"
*
*     JOB_SCRIPT_FILE - "./job_scripts/1234"
*                       (if job id is 1234)
*
*     JOB_ACTIVE_DIR - "./active_jobs"
******************************************************************************/
typedef enum {
   JOBS_SPOOL_DIR,
   JOB_SPOOL_DIR,
   JOB_SPOOL_DIR_AS_FILE,
   JOB_SPOOL_FILE,
   TASK_SPOOL_DIR,
   TASK_SPOOL_FILE,
   JOB_SCRIPT_DIR,
   JOB_SCRIPT_FILE,
   JOB_ACTIVE_DIR 
} sge_file_path_id_t;

/****** uti/spool/sge_spool_flags_t *******************************************
*  NAME
*     sge_spool_flags_t -- Context information for spooling functions
*
*  SYNOPSIS
*     typedef enum {
*        SPOOL_DEFAULT               = 0x0000,
*        SPOOL_HANDLE_AS_ZOMBIE      = 0x0001,
*        SPOOL_WITHIN_EXECD          = 0x0002,
*        SPOOL_IGNORE_TASK_INSTANCES = 0x0004
*     } sge_spool_flags_t; 
*
*  FUNCTION
*     These constants are necessary to provide spooling functions
*     with context information where they are called and what they 
*     should do. It depends on these spooling functions, how these
*     constants are interpreted. The documentation of these
*     routines may give you a more detailed description than you 
*     may find here.
*
*     SPOOL_DEFAULT - as it says the standard case
*
*     SPOOL_HANDLE_AS_ZOMBIE - used mostly for jobs/array tasks
*                              which are already finished and
*                              stored in the list of zombie jobs.
*
*     SPOOL_WITHIN_EXECD - Used for objects which are spooled
*                          within the execd. 
*
*     SPOOL_IGNORE_TASK_INSTANCES - Dont't handle array tasks.
******************************************************************************/
typedef enum {
   SPOOL_DEFAULT               = 0x0000,
   SPOOL_HANDLE_AS_ZOMBIE      = 0x0001,
   SPOOL_WITHIN_EXECD          = 0x0002,
   SPOOL_IGNORE_TASK_INSTANCES = 0x0004
} sge_spool_flags_t; 

/****** uti/spool/sge_file_path_format_t **************************************
*  NAME
*     sge_file_path_format_t -- Format of filename and pathname
*
*  SYNOPSIS
*     typedef enum {
*        FORMAT_DEFAULT      = 0x0000,
*        FORMAT_DOT_FILENAME = 0x0001,
*        FORMAT_FIRST_PART   = 0x0002,
*        FORMAT_SECOND_PART  = 0x0004,
*        FORMAT_THIRD_PART   = 0x0008
*     } sge_file_path_format_t;
*
*  FUNCTION
*     These constants are used with sge_get_file_path() to retrieve
*     file and pathnames for objects which should be spooled onto
*     a filesystem. 
*
*     FORMAT_DEFAULT - as it says the default format
*  
*     FORMAT_DOT_FILENAME - insert a '.' in front of the filename
*                           (e.g. '/path/path/.filename)
*
*     FORMAT_FIRST_PART   - first part of pathname (e.g /path)
*
*     FORMAT_SECOND_PART  - (e.g /path/part2)
*
*     FORMAT_THIRD_PART   - (e.g /path/part2/part3)
******************************************************************************/
typedef enum {
   FORMAT_DEFAULT      = 0x0000,
   FORMAT_DOT_FILENAME = 0x0001,
   FORMAT_FIRST_PART   = 0x0002,
   FORMAT_SECOND_PART  = 0x0004,
   FORMAT_THIRD_PART   = 0x0008
} sge_file_path_format_t;

typedef enum {
   STATUS_ROTATING_BAR,
   STATUS_DOTS
} washing_machine_t; 

u_long32 sge_get_ja_tasks_per_directory(void);

u_long32 sge_get_ja_tasks_per_file(void);

char *sge_get_file_path(char *buffer, sge_file_path_id_t,
                        sge_file_path_format_t format_flags,
                        sge_spool_flags_t spool_flags,
                        u_long32 ulong_val1, u_long32 ulong_val2);

int sge_is_valid_filename2(const char *fname); 

int sge_is_valid_filename(const char *fname);

int sge_spoolmsg_write(FILE *file, const char comment_char,
                       const char *version);   

char *sge_get_confval(const char *conf_val, const char *file);
 
pid_t sge_readpid(const char *fname);
 
void sge_write_pid(const char *pid_log_file);

void sge_status_set_type(washing_machine_t type);

void sge_status_next_turn(void);

void sge_status_end_turn(void);

void sge_silent_set(int i);

int sge_silent_get(); 

#endif /* _SGE_FILE_PATH_H_ */
