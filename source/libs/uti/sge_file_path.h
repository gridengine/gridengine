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

typedef enum {
   SPOOL_DEFAULT               = 0x0000,
   SPOOL_HANDLE_AS_ZOMBIE      = 0x0001,
   SPOOL_WITHIN_EXECD          = 0x0002,
   SPOOL_IGNORE_TASK_INSTANCES = 0x0004
} sge_spool_flags_t; 

typedef enum {
   FORMAT_DEFAULT      = 0x0000,
   FORMAT_DOT_FILENAME = 0x0001,
   FORMAT_FIRST_PART   = 0x0002,
   FORMAT_SECOND_PART  = 0x0004,
   FORMAT_THIRD_PART   = 0x0008
} sge_file_path_format_t;

u_long32 sge_get_ja_tasks_per_directory(void);

u_long32 sge_get_ja_tasks_per_file(void);

char *sge_get_file_path(char *buffer, sge_file_path_id_t,
                        sge_file_path_format_t format_flags,
                        sge_spool_flags_t spool_flags,
                        u_long32 ulong_val1, u_long32 ulong_val2);

int verify_filename(const char *fname); 

#endif /* _SGE_FILE_PATH_H_ */
