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

#include <stdio.h>
#include <string.h>
#include "sge.h"
#include "sgermon.h"
#include "sge_directoy.h"

#define MAX_JA_TASK_PER_DIR 4096 

void get_spool_dir_range(u_long32 ja_task_id, u_long32 *start, u_long32 *end)
{
   u_long32 row = (ja_task_id - 1) / MAX_JA_TASK_PER_DIR; 

   *start = row * MAX_JA_TASK_PER_DIR + 1;
   *end = (row + 1) * MAX_JA_TASK_PER_DIR;
}

char *sge_get_file_path(char *buffer, sge_file_path_id_t id,
                        sge_file_path_format_t format_flags,
                        sge_spool_flags_t spool_flags,
                        u_long32 ulong_val1, u_long32 ulong_val2)
{
   int handle_as_zombie = spool_flags & SPOOL_HANDLE_AS_ZOMBIE;
   int insert_dot = format_flags & FORMAT_DOT_FILENAME;
   int in_execd = spool_flags & SPOOL_WITHIN_EXECD;
   char *spool_dir = (handle_as_zombie ? ZOMBIE_DIR : JOB_DIR);

   if (id == JOBS_SPOOL_DIR) {
      sprintf(buffer, SFN, spool_dir);
   } else if (id == JOB_SPOOL_DIR || id == JOB_SPOOL_FILE ||
              id == TASK_SPOOL_DIR || id == TASK_SPOOL_FILE) {
      stringT job_dir = "";
      stringT file_prefix = "";
      stringT id_range = "";

      if (in_execd) {
         sprintf(job_dir, u32"."u32, ulong_val1, ulong_val2);
      } else {
         sprintf(job_dir, u32, ulong_val1);
      }
      if (insert_dot && (id == JOB_SPOOL_FILE || id == TASK_SPOOL_FILE)) {
         strcpy(file_prefix, "."); 
      }
      if (id == TASK_SPOOL_DIR || id == TASK_SPOOL_FILE) {
         u_long32 start, end;
         get_spool_dir_range(ulong_val2, &start, &end);
         sprintf(id_range, u32"-"u32, start, end);
      }
      if (id == JOB_SPOOL_DIR) {
         sprintf(buffer, SFN"/"SFN, spool_dir, job_dir);
      } else if (id == JOB_SPOOL_FILE) {
         sprintf(buffer, SFN"/"SFN"/"SFN SFN, spool_dir, job_dir, 
            file_prefix, "common");
      } else if (id == TASK_SPOOL_DIR) {
         sprintf(buffer, SFN"/"SFN"/"SFN, spool_dir, job_dir, id_range);
      } else if (id == TASK_SPOOL_FILE) {
         sprintf(buffer, SFN"/"SFN"/"SFN"/"SFN u32, spool_dir, job_dir, 
                 id_range, file_prefix, ulong_val2);
      }
   } else {
      buffer[0] = '0';
   }
   return buffer;
}

