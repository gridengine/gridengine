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
#include <errno.h>

#include "sge.h"
#include "sge_log.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sgermon.h"
#include "cull_file.h"
#include "cull_list.h"
#include "read_write_job.h"
#include "sge_mkdir.h"
#include "sge_file_path.h"
#include "utility.h"
#include "utility_daemon.h"
#include "sge_dir.h"
#include "sge_stringL.h"
#include "sge_job.h"
#include "sge_job_jatask.h"
#include "sge_answerL.h"
#include "msg_qmaster.h"
#include "opt_silent.h"
#include "sge_washing_machine.h"
#include "sge_stat.h"
#include "sge_hash.h"
#include "sge_dir.h"
#include "job.h"

static lList *ja_tasks_create_from_file(u_long32 job_id, u_long32 ja_task_id,
                                        sge_spool_flags_t flags);

static lListElem *ja_task_create_from_file(u_long32 job_id,
                                           u_long32 ja_task_id,
                                           sge_spool_flags_t flags);

static int ja_task_write_to_disk(lListElem *ja_task, u_long32 job_id,
                                 sge_spool_flags_t flags); 

static int job_write_common_part(lListElem *job, u_long32 ja_task_id,
                                 sge_spool_flags_t flags);

static int job_write_ja_task_part(lListElem *job, u_long32 ja_task_id,
                                  sge_spool_flags_t flags);

static int job_write_as_single_file(lListElem *job, u_long32 ja_task_id,
                                   sge_spool_flags_t flags);

extern lList *Master_Job_List;
extern lList *Master_Zombie_List;

lListElem *cull_create_job_from_disk(u_long32 job_id, u_long32 ja_task_id,
                                     sge_spool_flags_t flags)
{
   lListElem *job = NULL;
   stringT spool_path = "";

   DENTER(TOP_LAYER, "cull_create_job_from_disk");

   sge_get_file_path(spool_path, JOB_SPOOL_DIR, FORMAT_DEFAULT, 
                     flags, job_id, ja_task_id);  

   if (sge_is_directory(spool_path)) {
      stringT spool_path_common;
      lList *ja_tasks = NULL;

      sge_get_file_path(spool_path_common, JOB_SPOOL_FILE, FORMAT_DEFAULT, 
                        flags, job_id, ja_task_id);  
      job = lReadElemFromDisk(NULL, spool_path_common, JB_Type, "job");
      if (!job) {
         DTRACE; 
         goto error;
      }
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
      job = lReadElemFromDisk(NULL, spool_path, JB_Type, "job");
      if (!job) { 
         goto error;
      } 
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

int cull_write_jobtask_to_disk(lListElem *job, u_long32 ja_taskid, 
                               sge_spool_flags_t flags) 
{
   int ret;
   int spool_single_task_files;
   int within_execd = flags & SPOOL_WITHIN_EXECD;
   DENTER(TOP_LAYER, "cull_write_jobtask_to_disk");

   spool_single_task_files = (!within_execd && 
      job_get_number_of_ja_tasks(job) > sge_get_ja_tasks_per_file());
   if (spool_single_task_files) {
      ret = job_write_common_part(job, ja_taskid, flags);
      if (!ret) {
         ret = job_write_ja_task_part(job, ja_taskid, flags); 
      }
   } else {
      ret = job_write_as_single_file(job, ja_taskid, flags); 
   }

   DEXIT; 
   return ret;
}

static int job_write_as_single_file(lListElem *job, u_long32 ja_task_id,
                                   sge_spool_flags_t flags) 
{
   int ret = 0;
   u_long32 job_id;
   stringT job_dir_third = "";
   stringT spool_file = "";
   stringT tmp_spool_file = "";

   DENTER(TOP_LAYER, "job_write_as_single_file");
   job_id = lGetUlong(job, JB_job_number);

   sge_get_file_path(job_dir_third, JOB_SPOOL_DIR, FORMAT_THIRD_PART,
                     flags, job_id, ja_task_id);
   sge_mkdir(job_dir_third, 0755, 0);
   sge_get_file_path(spool_file, JOB_SPOOL_DIR, FORMAT_DEFAULT,
                     flags, job_id, ja_task_id);
   sge_get_file_path(tmp_spool_file, JOB_SPOOL_DIR, FORMAT_DOT_FILENAME,
                     flags, job_id, ja_task_id);

   ret = lWriteElemToDisk(job, tmp_spool_file, NULL, "job");
   if (!ret && (rename(tmp_spool_file, spool_file) == -1)) {
      DTRACE;
      ret = 1;
   }

   DEXIT;
   return ret;  
}

static int job_write_ja_task_part(lListElem *job, u_long32 ja_task_id,
                                  sge_spool_flags_t flags)
{
   lListElem *ja_task, *next_ja_task;
   u_long32 job_id;
   int ret = 0;
   DENTER(TOP_LAYER, "job_write_ja_task_part"); 

   job_id = lGetUlong(job, JB_job_number);
   if (ja_task_id) {
      next_ja_task = lGetElemUlong(lGetList(job, JB_ja_tasks),
                                   JAT_task_number, ja_task_id);
   } else {
      next_ja_task = lFirst(lGetList(job, JB_ja_tasks));
   }
   while((ja_task = next_ja_task)) {
      if (ja_task_id) {
         next_ja_task = NULL;
      } else {
         next_ja_task = lNext(ja_task);
      }

      if ((flags & SPOOL_WITHIN_EXECD) ||
          job_is_enrolled(job, lGetUlong(ja_task, JAT_task_number))) {
         ret = ja_task_write_to_disk(ja_task, job_id, flags);
         if (ret) {
            DTRACE;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

static int job_write_common_part(lListElem *job, u_long32 ja_task_id,
                                 sge_spool_flags_t flags) 
{
   int ret = 0;
   u_long32 job_id;
   stringT spool_dir;
   stringT spoolpath_common, tmp_spoolpath_common;
   lList *ja_tasks;

   DENTER(TOP_LAYER, "job_write_common_part");

   job_id = lGetUlong(job, JB_job_number);
   sge_get_file_path(spool_dir, JOB_SPOOL_DIR, FORMAT_DEFAULT,
                     flags, job_id, ja_task_id);
   sge_mkdir(spool_dir, 0755, 0);
   sge_get_file_path(spoolpath_common, JOB_SPOOL_FILE, FORMAT_DEFAULT,
                     flags, job_id, ja_task_id);
   sge_get_file_path(tmp_spoolpath_common, JOB_SPOOL_FILE,
                     FORMAT_DOT_FILENAME, flags, job_id, ja_task_id);

   ja_tasks = NULL;
   lXchgList(job, JB_ja_tasks, &ja_tasks);
   ret = lWriteElemToDisk(job, tmp_spoolpath_common, NULL, "job");
   lXchgList(job, JB_ja_tasks, &ja_tasks);

   if (!ret && (rename(tmp_spoolpath_common, spoolpath_common) == -1)) {
      DTRACE;
      ret = 1;
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
   stringT spool_dir = "";
   stringT spool_dir_second = "";
   stringT spool_dir_third = "";
   stringT spoolpath_common;
   stringT error_string = "";
   int within_execd = flags & SPOOL_WITHIN_EXECD;
   int handle_as_zombie = flags & SPOOL_HANDLE_AS_ZOMBIE;
   int spool_single_task_files;
   lList *master_list = handle_as_zombie ? Master_Zombie_List : Master_Job_List;
   lListElem *job = lGetElemUlong(master_list,  JB_job_number, jobid);

   DENTER(TOP_LAYER, "cull_remove_jobtask_from_disk");

   spool_single_task_files = (!within_execd && 
      job_get_number_of_ja_tasks(job) > sge_get_ja_tasks_per_file());

   if (ja_taskid != 0 && spool_single_task_files) {
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
         remove_task_spool_file = job_is_enrolled(job, ja_taskid);
      }
      DPRINTF(("remove_task_spool_file = %d\n", remove_task_spool_file));;

      if (remove_task_spool_file) {
         DPRINTF(("removing "SFN"\n", task_spool_file));
         if (sge_unlink(NULL, task_spool_file)) {
            ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "task spool file", task_spool_file));
            DTRACE;
         }
      }
      /*
       * Following rmdir call may fail. We can ignore this error.
       * This is only an indicator that another task is running which has been
       * spooled in the directory.
       */  
      rmdir(task_spool_dir);
   }

   sge_get_file_path(spool_dir, JOB_SPOOL_DIR, 
                     FORMAT_DEFAULT, flags, jobid, ja_taskid);
   sge_get_file_path(spool_dir_third, JOB_SPOOL_DIR, 
                     FORMAT_THIRD_PART, flags, jobid, ja_taskid);
   sge_get_file_path(spool_dir_second, JOB_SPOOL_DIR, 
                     FORMAT_SECOND_PART, flags, jobid, ja_taskid);
   sge_get_file_path(spoolpath_common, JOB_SPOOL_FILE, 
                     FORMAT_DEFAULT, flags, jobid, ja_taskid);
   if (spool_single_task_files) {
      if (ja_taskid == 0) { 
         DPRINTF(("removing "SFN"\n", spoolpath_common));
         if (sge_unlink(NULL, spoolpath_common)) {
            ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "job spool file", spoolpath_common)); 
            DTRACE;
         }
         if (recursive_rmdir(spool_dir, error_string)) {
            ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "job spool directory", spool_dir));
            DTRACE;
         }
      }
   } else {
      if (sge_unlink(NULL, spool_dir)) {
         ERROR((SGE_EVENT, "can not remove "SFN": "SFN"\n", "job spool file",
spool_dir));
         DTRACE;
      }
   }
   /*
    * Following rmdir calls may fail. We can ignore these errors.
    * This is only an indicator that another job is running which has been
    * spooled in the same directory.
    */
   if (!rmdir(spool_dir_third)) {
      rmdir(spool_dir_second); 
   }

   DEXIT;
   return 0;
}

int job_remove_script_file(u_long32 job_id)
{
   stringT script_file = "";
   int ret = 0;
   DENTER(TOP_LAYER, "job_remove_script_file");

   sge_get_file_path(script_file, JOB_SCRIPT_FILE, FORMAT_DEFAULT,
                     SPOOL_DEFAULT, job_id, 0); 
   if (unlink(script_file)) {
      if (errno!=ENOENT) {
         ERROR((SGE_EVENT, MSG_CONFIG_FAILEDREMOVINGSCRIPT_SS,
              strerror(errno), script_file)); 
         DTRACE;
         ret = 1;
      }
   } else {
      INFO((SGE_EVENT, MSG_CONFIG_REMOVEDSCRIPTOFBADJOBFILEX_S, script_file));
   }
   DEXIT;
   return ret;
}

int job_read_job_list_from_disk(lList **job_list, char *list_name, int check,
                                int hash, HashTable* Job_Hash_Table,
                                sge_spool_flags_t flags, 
                                int (*init_function)(lListElem*)) 
{
   stringT first_dir = ""; 
   lList *first_direnties; 
   lListElem *first_direntry;
   stringT path;

   DENTER(TOP_LAYER, "job_read_job_list_from_disk"); 
   sge_get_file_path(first_dir, JOBS_SPOOL_DIR, FORMAT_FIRST_PART, 
                     flags, 0, 0);  
   first_direnties = sge_get_dirents(first_dir);

   if (first_direnties && !silent()) {
      printf(MSG_CONFIG_READINGINX_S, list_name);
   }

   for (;
        (first_direntry = lFirst(first_direnties)); 
        lRemoveElem(first_direnties, first_direntry)) {
      stringT second_dir = "";
      lList *second_direnties;
      lListElem *second_direntry;
      const char *first_entry_string;

      washing_machine_next_turn();

      first_entry_string = lGetString(first_direntry, STR);
      sprintf(path, "%s/%s", first_dir, first_entry_string);
      if (!sge_is_directory(path)) {
         ERROR((SGE_EVENT, MSG_CONFIG_NODIRECTORY_S, path)); 
         break;
      }
   
      sprintf(second_dir, SFN"/"SFN, first_dir, first_entry_string); 
      second_direnties = sge_get_dirents(second_dir);
      for (;
           (second_direntry = lFirst(second_direnties));
           lRemoveElem(second_direnties, second_direntry)) { 
         stringT third_dir = "";
         lList *third_direnties;
         lListElem *third_direntry;
         const char *second_entry_string;

         second_entry_string = lGetString(second_direntry, STR);
         sprintf(path, "%s/%s/%s", first_dir, first_entry_string,
                 second_entry_string);
         if (!sge_is_directory(path)) {
            ERROR((SGE_EVENT, MSG_CONFIG_NODIRECTORY_S, path));
            break;
         } 

         sprintf(third_dir, SFN"/"SFN, second_dir, second_entry_string);
         third_direnties = sge_get_dirents(third_dir);
         for (;
              (third_direntry = lFirst(third_direnties));
              lRemoveElem(third_direnties, third_direntry)) {
            lListElem *job, *ja_task;
            stringT job_dir = "";
            stringT fourth_dir = "";
            stringT job_id_string = "";
            char *ja_task_id_string;
            u_long32 job_id, ja_task_id;
            int all_finished;

            sprintf(fourth_dir, SFN"/"SFN, third_dir,
                    lGetString(third_direntry, STR));
            sprintf(job_id_string, SFN SFN SFN, 
                    lGetString(first_direntry, STR),
                    lGetString(second_direntry, STR),
                    lGetString(third_direntry, STR)); 
            job_id = (u_long32) strtol(job_id_string, NULL, 10);
            strtok(job_id_string, ".");
            ja_task_id_string = strtok(NULL, ".");
            if (ja_task_id_string) {
               ja_task_id = (u_long32) strtol(ja_task_id_string, NULL, 10);
            } else {
               ja_task_id = 0;
            }
            sge_get_file_path(job_dir, JOB_SPOOL_DIR, FORMAT_DEFAULT,
                              flags, job_id, ja_task_id);

            /* check directory name */
            if (strcmp(fourth_dir, job_dir)) {
               fprintf(stderr, "%s %s\n", fourth_dir, job_dir);
               DPRINTF(("Invalid directory "SFN"\n", fourth_dir));
               continue;
            }

            /* read job */
            job = cull_create_job_from_disk(job_id, ja_task_id, flags);
            if (!job) {
               job_remove_script_file(job_id);
               continue;
            }

            /* check for scriptfile before adding job */
            all_finished = 1;
            for_each (ja_task, lGetList(job, JB_ja_tasks)) {
               if (lGetUlong(ja_task, JAT_status) != JFINISHED) {
                  all_finished = 0;
                  break;
               }
            }
            if (check && !all_finished && lGetString(job, JB_script_file)) {
               stringT script_file;
               SGE_STRUCT_STAT stat_buffer;

               sge_get_file_path(script_file, JOB_SCRIPT_FILE, FORMAT_DEFAULT,
                                 flags, job_id, 0);
               if (SGE_STAT(script_file, &stat_buffer)) {
                  ERROR((SGE_EVENT, MSG_CONFIG_CANTFINDSCRIPTFILE_U,
                         u32c(lGetUlong(job, JB_job_number))));
                  continue;
               }
            }  
 
            /* check if filename has same name which is stored job id */
            if (lGetUlong(job, JB_job_number) != job_id) {
               ERROR((SGE_EVENT, MSG_CONFIG_JOBFILEXHASWRONGFILENAMEDELETING_U,
                     u32c(job_id)));
               cull_remove_jobtask_from_disk(job_id, 0, flags);
               /* 
                * script is not deleted here, 
                * since it may belong to a valid job 
                */
            } 

            if (init_function) {
               init_function(job);
            }

            lSetList(job, JB_jid_sucessor_list, NULL); 
            job_add_job(job_list, list_name, job, 0, hash, Job_Hash_Table);
         }
         third_direnties = lFreeList(third_direnties);
      }
      second_direnties = lFreeList(second_direnties);
   } 
   first_direnties = lFreeList(first_direnties);

   /*
    * for Master_Zombie_List we must sort the list by end time
    */
   if (flags & SPOOL_HANDLE_AS_ZOMBIE) {
      lSortOrder *so; 

      so = lParseSortOrderVarArg(JB_Type, "%I+", JB_end_time);
      lSortList(*job_list, so);
      lFreeSortOrder(so);
   }

   if (*job_list) {
      washing_machine_end_turn();
   }      

   DEXIT;
   return 0;
}
