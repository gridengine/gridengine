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
#include <stdlib.h>
#include <ctype.h>

#include "sge.h"
#include "sgermon.h"
#include "sge_file_path.h"

#define MAX_JA_TASKS_PER_DIR  (4096l)
#define MAX_JA_TASKS_PER_FILE (1l)  

static void get_spool_dir_range(u_long32 ja_task_id, u_long32 *start, 
                                u_long32 *end);

static void get_spool_dir_parts(u_long32 job_id, char *first, char *second, 
                                char *third);


static void get_spool_dir_range(u_long32 ja_task_id, u_long32 *start, 
                                u_long32 *end)
{
   u_long32 row = (ja_task_id - 1) / sge_get_ja_tasks_per_directory(); 

   *start = row * sge_get_ja_tasks_per_directory() + 1;
   *end = (row + 1) * sge_get_ja_tasks_per_directory();
}

static void get_spool_dir_parts(u_long32 job_id, char *first, char *second, 
                                char *third)
{
   sprintf(third, "%04d", (int)(job_id % 10000l));
   job_id /= 10000l;
   sprintf(second, "%04d", (int)(job_id % 10000l));
   job_id /= 10000l;
   sprintf(first, "%02d", (int)(job_id % 10000l));  
}

u_long32 sge_get_ja_tasks_per_directory(void) {
   static u_long32 tasks_per_directory = 0;

   if (!tasks_per_directory) {
      char *env_string;
     
      env_string = getenv("SGE_MAX_TASKS_PER_DIRECTORY"); 
      if (env_string) {
         tasks_per_directory = (u_long32) strtol(env_string, NULL, 10); 
      }
   }
   if (!tasks_per_directory) {
      tasks_per_directory = MAX_JA_TASKS_PER_DIR;
   }
   return tasks_per_directory;
}
 
u_long32 sge_get_ja_tasks_per_file(void) {
   static u_long32 tasks_per_file = 0;
 
   if (!tasks_per_file) {
      char *env_string;
 
      env_string = getenv("SGE_MAX_TASKS_PER_FILE");
      if (env_string) {
         tasks_per_file = (u_long32) strtol(env_string, NULL, 10);
      }
   }
   if (!tasks_per_file) {
     tasks_per_file = MAX_JA_TASKS_PER_FILE;
   }
   return tasks_per_file;  
}

char *sge_get_file_path(char *buffer, sge_file_path_id_t id,
                        sge_file_path_format_t format_flags,
                        sge_spool_flags_t spool_flags,
                        u_long32 ulong_val1, u_long32 ulong_val2)
{
   int handle_as_zombie = spool_flags & SPOOL_HANDLE_AS_ZOMBIE;
   int first_part = format_flags & FORMAT_FIRST_PART;
   int second_part = format_flags & FORMAT_SECOND_PART;
   int third_part = format_flags & FORMAT_THIRD_PART;
   int insert_dot = format_flags & FORMAT_DOT_FILENAME;
   int in_execd = spool_flags & SPOOL_WITHIN_EXECD;
   char *spool_dir = (handle_as_zombie ? ZOMBIE_DIR : JOB_DIR);

   if (id == JOBS_SPOOL_DIR) {
      sprintf(buffer, SFN, spool_dir);
   } else if (id == JOB_SPOOL_DIR || id == JOB_SPOOL_FILE ||
              id == TASK_SPOOL_DIR || id == TASK_SPOOL_FILE ||
              id == JOB_SPOOL_DIR_AS_FILE) {
      stringT job_dir = "";
      stringT file_prefix = "";
      stringT id_range = "";
      stringT job_dir_first = "";
      stringT job_dir_second = "";
      stringT job_dir_third = "";

      get_spool_dir_parts(ulong_val1, job_dir_first, job_dir_second,
                          job_dir_third);
      
      if (first_part) {
         ;
      } else if (second_part) {
         sprintf(job_dir, "%s", job_dir_first);
      } else if (third_part) {
         sprintf(job_dir, "%s/%s", job_dir_first, job_dir_second);
      } else {
         if (id == JOB_SPOOL_DIR_AS_FILE && insert_dot) {
            if (in_execd) {
               sprintf(job_dir, "%s/%s/.%s."u32, job_dir_first, 
                       job_dir_second, job_dir_third, ulong_val2);
            } else {
               sprintf(job_dir, "%s/%s/.%s", job_dir_first,
                       job_dir_second, job_dir_third);
            }  
         } else {
            if (in_execd) {
               sprintf(job_dir, "%s/%s/%s."u32, job_dir_first, 
                       job_dir_second, job_dir_third, ulong_val2);
            } else {
               sprintf(job_dir, "%s/%s/%s", job_dir_first,
                       job_dir_second, job_dir_third);
            }  
         }
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
         sprintf(buffer, "%s/%s", spool_dir, job_dir);
      } else if (id == JOB_SPOOL_FILE) {
         sprintf(buffer, "%s/%s/%s%s", spool_dir, job_dir, 
            file_prefix, "common");
      } else if (id == TASK_SPOOL_DIR) {
         sprintf(buffer, "%s/%s/%s", spool_dir, job_dir, id_range);
      } else if (id == TASK_SPOOL_FILE) {
         sprintf(buffer, "%s/%s/%s/%s"u32, spool_dir, job_dir, 
                 id_range, file_prefix, ulong_val2);
      }
   } else if (id == JOB_SCRIPT_DIR) { 
      sprintf(buffer, "%s", EXEC_DIR); 
   } else if (id == JOB_SCRIPT_FILE) {
      sprintf(buffer, "%s/"u32, EXEC_DIR, ulong_val1); 
   } else if (id == JOB_ACTIVE_DIR && in_execd) {
      sprintf(buffer, ACTIVE_DIR"/"u32"."u32, ulong_val1, ulong_val2);
   } else {
      buffer[0] = '0';
   }
   return buffer;
}

/***************************************************
 Verify the applicability of a file name.
 We dont like:
 - names longer than 256 chars including '\0'
 - blanks or other ugly chars
 
 We like digits, chars and '_'.
 
 returning 0 means OK.
 ***************************************************/
int verify_filename(const char *fname) 
{
   int i=0;
 
   /* dont allow "." ".." and "../tralla" */
   if (*fname == '.') {
      fname++;
      if (!*fname || (*fname == '.' && ((!*(fname+1)) || (!*fname+1 == '/'))))
         return 1;
   }
   while (*fname && i++<256) {
      if (!isalnum((int) *fname) && !(*fname=='_') && !(*fname=='.'))
         return 1;
      fname++;
   }
   if (i>=256)
      return 1;
 
   return 0;
}              
