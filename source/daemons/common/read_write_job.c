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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sge.h"
#include "sge_log.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sgermon.h"
#include "cull_file.h"
#include "cull_list.h"
#include "read_write_job.h"
#include "sge_mkdir.h"
#include "sge_directoy.h"
#include "utility.h"
#include "utility_daemon.h"
#include "sge_dir.h"
#include "sge_stringL.h"
#include "sge_job.h"
#include "sge_job_jatask.h"
#include "sge_answerL.h"

static lList *ja_tasks_create_from_file(u_long32 job_id, u_long32 ja_task_id,
                                        sge_spool_flags_t flags);

static lListElem *ja_task_create_from_file(u_long32 job_id,
                                           u_long32 ja_task_id,
                                           sge_spool_flags_t flags);

static int ja_task_write_to_disk(lListElem *ja_task, u_long32 job_id,
                                 sge_spool_flags_t flags); 

extern lList *Master_Job_List;

lListElem *cull_create_job_from_disk(u_long32 job_id, u_long32 ja_task_id,
                                     sge_spool_flags_t flags)
{
   lListElem *job = NULL;
   lList *ja_tasks = NULL;
   stringT spool_path_common;

   DENTER(TOP_LAYER, "cull_create_job_from_disk");

   sge_get_file_path(spool_path_common, JOB_SPOOL_FILE, FORMAT_DEFAULT, 
                     flags, job_id, ja_task_id);  

   job = lReadElemFromDisk(NULL, spool_path_common, JB_Type, "job");
   if (job) {
      if (!(flags & SPOOL_WITHIN_EXECD)) {
         job_initialize_ja_tasks(job);
      }
      ja_tasks = ja_tasks_create_from_file(job_id, ja_task_id, flags); 
      if (ja_tasks) {
         lList *ja_task_list;

         ja_task_list = lGetList(job, JB_ja_tasks);
         if (ja_task_list) {
            lAddList(ja_task_list, ja_tasks);
         } else {
            lSetList(job, JB_ja_tasks, ja_tasks);
         }
         ja_tasks = NULL;
      }
      if (!lGetNumberOfElem(lGetList(job, JB_ja_tasks))) {
         DTRACE;
         goto error;
      }
      lPSortList(lGetList(job, JB_ja_tasks), "%I+", JAT_task_number); 
   } else {
      DTRACE;
      goto error;
   } 
   DEXIT;
   return job;
error:
   DEXIT;
   job = lFreeElem(job);
   return NULL;
}

static lList *ja_tasks_create_from_file(u_long32 job_id, u_long32 ja_task_id,
                                        sge_spool_flags_t flags)
{
   lList *dir_entries = NULL;
   lList *ja_task_entries = NULL;
   lList *ja_tasks = NULL;
   lListElem *dir_entry;
   stringT spool_dir_job;
   DENTER(TOP_LAYER, "ja_tasks_create_from_file");

   ja_tasks = lCreateList("ja_tasks", JAT_Type); 
   if (!ja_tasks) {
      DTRACE;
      goto error;
   }
   sge_get_file_path(spool_dir_job, JOB_SPOOL_DIR, FORMAT_DEFAULT, flags, 
                     job_id, ja_task_id);
   dir_entries = sge_get_dirents(spool_dir_job);
   for_each(dir_entry, dir_entries) {
      char *entry;
 
      entry = lGetString(dir_entry, STR);
      if (strcmp(entry, ".") && strcmp(entry, "..") && 
          strcmp(entry, "common")) {
         stringT spool_dir_tasks;
         lListElem *ja_task_entry; 

         sprintf(spool_dir_tasks, SFN"/"SFN, spool_dir_job, entry);
         ja_task_entries = sge_get_dirents(spool_dir_tasks);
         for_each(ja_task_entry, ja_task_entries) {
            char *ja_task_string;

            ja_task_string = lGetString(ja_task_entry, STR);
            if (strcmp(entry, ".") && strcmp(entry, "..")) {
               u_long32 ja_task_id;
               lListElem *ja_task;

               ja_task_id = atol(ja_task_string);
               if (ja_task_id == 0) {
                  DTRACE;
                  goto error;
               }
                  
               ja_task = ja_task_create_from_file(job_id, ja_task_id, flags); 
               if (ja_task) {
                  lAppendElem(ja_tasks, ja_task);
               } else {
                  DTRACE;
                  goto error;
               }
            }
         } 
         ja_task_entries = lFreeList(ja_task_entries);
      }
   }
   dir_entries = lFreeList(dir_entries);

   if (!lGetNumberOfElem(ja_tasks)) {
      DTRACE;
      goto error; 
   } 
   DEXIT;
   return ja_tasks;
error:
   ja_tasks = lFreeList(ja_tasks);
   dir_entries = lFreeList(dir_entries);
   ja_task_entries = lFreeList(ja_task_entries);  
   DEXIT;
   return NULL; 
}

static lListElem *ja_task_create_from_file(u_long32 job_id, 
                                           u_long32 ja_task_id, 
                                           sge_spool_flags_t flags) 
{
   lListElem *ja_task;
   stringT spool_path_ja_task;

   sge_get_file_path(spool_path_ja_task, TASK_SPOOL_FILE,
                     FORMAT_DEFAULT, flags, job_id, ja_task_id);
   ja_task = lReadElemFromDisk(NULL, spool_path_ja_task, JAT_Type, "ja_task"); 
   return ja_task;
}

int cull_write_jobtask_to_disk(lListElem *jep, u_long32 ja_taskid, 
                               sge_spool_flags_t flags) 
{
   int ret;
   u_long32 jobid;
    
   DENTER(TOP_LAYER, "cull_write_jobtask_to_disk");

   jobid = lGetUlong(jep, JB_job_number);

   /* 
    * Write the "common" part
    */
   {
      stringT spool_dir;
      stringT spoolpath_common, tmp_spoolpath_common;
      lList *ja_tasks;

      sge_mkdir(sge_get_file_path(spool_dir, JOB_SPOOL_DIR, FORMAT_DEFAULT, 
                flags, jobid, ja_taskid), 0755, 0);
      sge_get_file_path(spoolpath_common, JOB_SPOOL_FILE, FORMAT_DEFAULT, 
                        flags, jobid, ja_taskid);
      sge_get_file_path(tmp_spoolpath_common, JOB_SPOOL_FILE, 
                        FORMAT_DOT_FILENAME, flags, jobid, ja_taskid);

      ja_tasks = NULL;
      lXchgList(jep, JB_ja_tasks, &ja_tasks);
      DPRINTF(("writing file: "SFN"\n", tmp_spoolpath_common));
      ret = lWriteElemToDisk(jep, tmp_spoolpath_common, NULL, "job");
      lXchgList(jep, JB_ja_tasks, &ja_tasks);
      if (!ret && (rename(tmp_spoolpath_common, spoolpath_common) == -1)) {
         DEXIT;
         return 1; 
      }
   }

   /* 
    * Write ja tasks
    */
   {
      lListElem *ja_task, *next_ja_task;

      if (ja_taskid) {
         next_ja_task = lGetElemUlong(lGetList(jep, JB_ja_tasks), 
                                      JAT_task_number, ja_taskid);
      } else {
         next_ja_task = lFirst(lGetList(jep, JB_ja_tasks));
      }
      while((ja_task = next_ja_task)) {

         if (ja_taskid) {
            next_ja_task = NULL;
         } else {
            next_ja_task = lNext(ja_task);
         }

         if ((flags & SPOOL_WITHIN_EXECD) ||       
             job_is_enrolled(jep, lGetUlong(ja_task, JAT_task_number))) {
DTRACE;
            ret = ja_task_write_to_disk(ja_task, jobid, flags); 
            if (ret) {
               DEXIT;
               return 1;
            }
         }
      }
   }

   DEXIT; 
   return ret;
}

static int ja_task_write_to_disk(lListElem *ja_task, u_long32 job_id,
                                 sge_spool_flags_t flags)
{
   static stringT old_task_spool_dir = "";
   stringT task_spool_dir;
   stringT task_spool_file;
   stringT tmp_task_spool_file;
   int ret = 0;
   DENTER(TOP_LAYER, "ja_task_write_to_disk");

   sge_get_file_path(task_spool_dir, TASK_SPOOL_DIR, FORMAT_DEFAULT, flags,
      job_id, lGetUlong(ja_task, JAT_task_number));
   sge_get_file_path(task_spool_file, TASK_SPOOL_FILE, FORMAT_DEFAULT, flags,
      job_id, lGetUlong(ja_task, JAT_task_number));
   sge_get_file_path(tmp_task_spool_file, TASK_SPOOL_FILE, FORMAT_DOT_FILENAME,
      flags, job_id, lGetUlong(ja_task, JAT_task_number));

   if ((flags & SPOOL_WITHIN_EXECD) ||
       strcmp(old_task_spool_dir, task_spool_dir)) {
      strcpy(old_task_spool_dir, task_spool_dir);
      sge_mkdir(task_spool_dir, 0755, 0);
   }

   ret = lWriteElemToDisk(ja_task, tmp_task_spool_file, NULL, "ja_task");
   if (!ret && (rename(tmp_task_spool_file, task_spool_file) == -1)) {
      DTRACE;
   }    
   DEXIT;
   return ret;
}

int cull_remove_jobtask_from_disk(u_long32 jobid, u_long32 ja_taskid, 
                                  sge_spool_flags_t flags)
{
   stringT spool_dir;
   stringT spoolpath_common;
   stringT error_string = "";
   int within_execd = flags & SPOOL_WITHIN_EXECD;
 
   DENTER(TOP_LAYER, "cull_remove_jobtask_from_disk");

   if (ja_taskid != 0) {
      stringT task_spool_dir;
      stringT task_spool_file;
      int remove_task_spool_file = 0;

      sge_get_file_path(task_spool_dir,
         TASK_SPOOL_DIR, FORMAT_DEFAULT, flags,
         jobid, ja_taskid);
      sge_get_file_path(task_spool_file,
         TASK_SPOOL_FILE, FORMAT_DEFAULT, flags,
         jobid, ja_taskid);

      if (within_execd) {
         remove_task_spool_file = 1;
      } else {
         lListElem *job;

         job = lGetElemUlong(Master_Job_List, JB_job_number, jobid);
         remove_task_spool_file = job_is_enrolled(job, ja_taskid);
      }
      DPRINTF(("remove_task_spool_file = %d\n", remove_task_spool_file));;

      if (remove_task_spool_file) {
         DPRINTF(("removing "SFN"\n", task_spool_file));
         if (sge_unlink(NULL, task_spool_file)) {
            ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "task spool file", task_spool_file));
            DEXIT;
            return 1;
         }
      }

      if (within_execd && !has_more_dirents(task_spool_dir, 0)) {
         if (recursive_rmdir(task_spool_dir, error_string)) {
            ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "task spool directory", task_spool_dir));
            DEXIT;
            return 1;
         }     
      }
   }

   sge_get_file_path(spool_dir, 
      JOB_SPOOL_DIR, FORMAT_DEFAULT, flags, jobid, ja_taskid);
   sge_get_file_path(spoolpath_common, 
      JOB_SPOOL_FILE, FORMAT_DEFAULT, flags, jobid, ja_taskid);
   if (ja_taskid == 0 || (within_execd && !has_more_dirents(spool_dir, 1))) { 
      DPRINTF(("removing "SFN"\n", spoolpath_common));
      if (sge_unlink(NULL, spoolpath_common)) {
         ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "job spool file", spoolpath_common)); 
         DEXIT;
         return 1;
      }
      if (recursive_rmdir(spool_dir, error_string)) {
         ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "job spool directory", spool_dir));
         DEXIT;
         return 1; 
      }
   }

   DEXIT;
   return 0;
}
