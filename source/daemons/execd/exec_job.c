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

#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#ifdef __sgi
#   include <sys/schedctl.h>
#endif

#include "sge.h"
#include "def.h"
#include "symbols.h"
#include "sge_log.h"
#include "sge_conf.h"
#include "sge_time.h"
#include "sge_peL.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_pe_taskL.h"
#include "sge_ckptL.h"
#include "sge_complexL.h"
#include "sge_queueL.h"
#include "sge_stringL.h"
#include "sge_answerL.h"
#include "sge_rangeL.h"
#include "parse.h"
#include "get_path.h"
#include "sge_job.h"
#include "tmpdir.h"
#include "read_write_queue.h"
#include "exec_job.h"
#include "sge_path_alias.h"
#include "slots_used.h"
#include "sge_parse_num_par.h"
#include "show_job.h"
#include "mail.h"
#include "sgermon.h"
#include "commlib.h"
#include "basis_types.h"
#include "sgedefs.h"
#include "exec_ifm.h"
#include "pdc.h"
#include "sge_afsutil.h"
#include "sge_prog.h"
#include "setup_path.h"
#include "qm_name.h"
#include "sge_string.h" 
#include "jb_now.h"
#include "sge_feature.h"
#include "sge_job_jatask.h"
#include "sge_stdlib.h"
#include "sge_unistd.h"
#include "sge_uidgid.h"
#include "sge_hostname.h"
#include "sge_os.h"
#include "sge_varL.h"
#include "sge_var.h"
#include "sge_range.h"

#include "msg_common.h"
#include "msg_execd.h"
#include "msg_utilib.h"

#define ENVIRONMENT_FILE "environment"
#define CONFIG_FILE "config"

static int ck_login_sh(char *shell);
static int get_nhosts(lList *gdil_list);
static int arch_dep_config(FILE *fp, lList *cplx, char *err_str);

/* from execd.c import the working dir of the execd */
extern char execd_spool_dir[SGE_PATH_MAX];


/* import Master Job List */
extern lList *Master_Job_List;

#if COMPILE_DC
#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
/* local functions */
static int addgrpid_already_in_use(long);
static long get_next_addgrpid(lList *, long);
#endif
#endif

/* 
   in case of regular jobs the queue is the first entry in the gdil list of the job
   in case of tasks the queue is the appropriate entry in the gdil list of the slave job
*/

lListElem* responsible_queue(
lListElem *jep,
lListElem *jatep,
lListElem *petep
) {
   lListElem *master_q = NULL;

   DENTER(TOP_LAYER, "responsible_queue");

   if (petep == NULL) {
      master_q = lFirst(lGetList(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), JG_queue));
   } else {
      lListElem *pe_queue = lFirst(lGetList(petep, PET_granted_destin_identifier_list));
      master_q = lFirst(lGetList(lGetElemStr(lGetList(jatep, JAT_granted_destin_identifier_list),
                                             JG_qname,
                                             lGetString(pe_queue, JG_qname)),
                        JG_queue));
      
   }

   DEXIT;
   return master_q;
}

#if COMPILE_DC
#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
static long get_next_addgrpid(
lList *rlp,
long last_addgrpid 
) {
   lListElem *rep;
   int take_next = 0;
   
   /* uninitialized => return first number in list */
   if (last_addgrpid == 0) {
      rep = lFirst(rlp);
      if (rep)
         return (lGetUlong(rep, RN_min));
      else
         return (0);
   } 

   /* search range and return next id */
   for_each (rep, rlp) {
      long min, max;

      min = lGetUlong(rep, RN_min);
      max = lGetUlong(rep, RN_max);
      if (take_next)
         return (min);
      else if (last_addgrpid >= min && last_addgrpid < max) 
         return (++last_addgrpid);
      else if (last_addgrpid == max)
         take_next = 1;
   }
   
   /* not successfull until now => take first number */
   rep = lFirst(rlp);
   if (rep)
      return (lGetUlong(rep, RN_min));
   
   return (0);
}

static int addgrpid_already_in_use(long add_grp_id) 
{
   lListElem *job, *ja_task, *pe_task;
   
   for_each(job, Master_Job_List) {
      for_each (ja_task, lGetList(job, JB_ja_tasks)) {
         const char *id = lGetString(ja_task, JAT_osjobid);
         if (id != NULL && atol(id) == add_grp_id) {
            return 1;
         }

         for_each(pe_task, lGetList(ja_task, JAT_task_list)) {
            id = lGetString(pe_task, PET_osjobid);
            if(id != NULL && atol(id) == add_grp_id) {
               return 1;
            }
         }
      }
   }
   return 0;
}
#endif
#endif

/************************************************************************
 part of execd. Setup job environment then start shepherd.

 If we encounter an error we return an error
 return -1==error 
        -2==general error (Halt queue)
        -3==general error (Halt job)
        err_str set to error string
 ************************************************************************/
int sge_exec_job(
lListElem *jep,
lListElem *jatep,
lListElem *petep,
char *err_str 
) {
   int i;
   char sge_mail_subj[1024];
   char sge_mail_body[2048];
   char sge_mail_start[128];
   char ps_name[128];
   lList *mail_users;
   int mail_options;
   FILE *fp;
   u_long32 interval;
   struct passwd *pw;
   SGE_STRUCT_STAT buf;
   int used_slots, pe_slots = 0, host_slots = 0, nhosts = 0;
   static lList *processor_set = NULL;
   const char *cp;
   char *shell;
   const char *cwd = NULL;
   const lList *path_aliases = NULL;
   lList *cplx;
   char dce_wrapper_cmd[128];

#if COMPILE_DC
#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
   static gid_t last_addgrpid;
#endif
#endif   

   char dir[SGE_PATH_MAX], 
        shepherd_path[SGE_PATH_MAX], 
        coshepherd_path[SGE_PATH_MAX],
        hostfilename[SGE_PATH_MAX], 
        script_file[SGE_PATH_MAX], 
        tmpdir[SGE_PATH_MAX], 
        fname[SGE_PATH_MAX],
        shell_path[SGE_PATH_MAX], 
        stdout_path[SGE_PATH_MAX],
        stderr_path[SGE_PATH_MAX],
        pe_stdout_path[SGE_PATH_MAX],
        pe_stderr_path[SGE_PATH_MAX];
   char mail_str[1024], *shepherd_name;
   lListElem *gdil_ep, *master_q;
   lListElem *ep;
   lListElem *env;
   lList *environmentList;
   const char *arch = sge_get_arch();

   int write_osjob_id = 1;

   DENTER(TOP_LAYER, "sge_exec_job");

   SGE_ASSERT((jep));
   SGE_ASSERT((jatep));

   environmentList = lCreateList("environment list", VA_Type);

   DPRINTF(("job: %ld jatask: %ld petask: %s\n", lGetUlong(jep, JB_job_number), 
      lGetUlong(jatep, JAT_task_number),
      petep != NULL ? lGetString(petep, PET_id) : "none"));

   master_q = responsible_queue(jep, jatep, petep);
   SGE_ASSERT((master_q));

   /* prepare complex of master_q */
   if (!(cplx = lGetList( lGetElemStr( lGetList(jatep, JAT_granted_destin_identifier_list), 
         JG_qname, lGetString(master_q, QU_qname)), JG_complex))) {
      DEXIT;
      return -2;
   } 

   pw = sge_getpwnam(lGetString(jep, JB_owner));
   if (!pw) {
      sprintf(err_str, MSG_SYSTEM_GETPWNAMFAILED_S, lGetString(jep, JB_owner));
      DEXIT;
      return -3;        /* error only relevant for this user */
   }

   /* JG: TODO: create function make_active_job_dir() */
   if (petep != NULL) {
      SGE_STRUCT_STAT statbuf;

      /*
      ** Does the job directory for sub tasks exist?
      ** We need this test because it is possible that the
      ** execd with master task and the execd with subtask do not have
      ** shared filesystems
      */
      /* build base directory for job */
      sprintf(dir, "%s/"u32"."u32 , ACTIVE_DIR, lGetUlong(jep, JB_job_number),
         lGetUlong(jatep, JAT_task_number));
      /* Does this directory exist? */
      if (SGE_STAT(dir, &statbuf)) {
         if (mkdir(dir, 0755) == -1) {
            sprintf(err_str, MSG_FILE_CREATEDIR_SS, dir, strerror(errno));
            DEXIT;
            return -2;
         }
      }
      /*
      ** Now we can create the directory for the pe-task
      */  
      sprintf(dir, "%s/"u32"."u32"/%s", ACTIVE_DIR, lGetUlong(jep, JB_job_number), 
         lGetUlong(jatep, JAT_task_number), lGetString(petep, PET_id));
   } else {
      sprintf(dir, "%s/"u32"."u32"", ACTIVE_DIR, lGetUlong(jep, JB_job_number), 
         lGetUlong(jatep, JAT_task_number));
   }

   /* Create job or (pe) task directory */ 
   if (mkdir(dir, 0755) == -1) {
      if (errno == EEXIST) {
         DPRINTF(("cleaning active job dir\n"));
         if (sge_rmdir(dir, SGE_EVENT)) {
            sprintf(err_str, MSG_FILE_RMDIR_SS, dir, SGE_EVENT);
            DEXIT;
            return -2;
         }
         if (mkdir(dir, 0755) == -1) {
            ERROR((SGE_EVENT, MSG_FILE_CREATEDIRDEL_SS, dir, strerror(errno)));
            DEXIT;
            return -2;
         }
      } else {
         sprintf(err_str, MSG_FILE_CREATEDIR_SS, dir, strerror(errno));
         DEXIT;
         return -2;        /* general error */
      }
   }

   umask(022);

   /* make tmpdir only when this is the first task that gets started 
      in this queue. QU_job_slots_used holds actual number of used 
      slots for this job in the queue */
   if (!(used_slots=qslots_used(master_q))) {
      if (!(sge_make_tmpdir(master_q, lGetUlong(jep, JB_job_number), 
            lGetUlong(jatep, JAT_task_number), 
          pw->pw_uid, pw->pw_gid, tmpdir))) {
         sprintf(err_str, MSG_SYSTEM_CANTMAKETMPDIR);
         DEXIT;
         return -2;
      }
   } else {
      if(!(sge_get_tmpdir(master_q, 
                          lGetUlong(jep, JB_job_number), 
                          lGetUlong(jatep, JAT_task_number), 
                          tmpdir))) {
         sprintf(err_str, MSG_SYSTEM_CANTGETTMPDIR);
         DEXIT;
         return -2;
      }                    
   }

   /* increment used slots */
   DPRINTF(("%s: used slots increased from %d to %d\n", lGetString(master_q, QU_qname), 
         used_slots, used_slots+1));
   set_qslots_used(master_q, used_slots+1);

   
   /***************** write out sge host file ******************************/
   /* JG: TODO: create function write_pe_hostfile() */
   if (petep == NULL) {
      if (processor_set)
         processor_set = lFreeList(processor_set);

      sprintf(hostfilename, "%s/%s/%s", execd_spool_dir, dir, PE_HOSTFILE);
      fp = fopen(hostfilename, "w");
      if (!fp) {
         sprintf(err_str, MSG_FILE_NOOPEN_SS,  hostfilename, strerror(errno));
         DEXIT;
         return -2;
      }

      /* 
         Get number of hosts 'nhosts' where the user got queues on 

         The granted_destination_identifier_list holds
         on entry for each queue, not host. But each
         entry also contais the hosts name where the
         queue resides on.

         We need to combine the processor sets of all queues on this host. 
         They need to get passed to shepherd
      */
      nhosts = get_nhosts(lGetList(jatep, JAT_granted_destin_identifier_list));
      pe_slots = 0;
      host_slots = 0;
      for_each (gdil_ep, lGetList(jatep, JAT_granted_destin_identifier_list)) {
         int slots;
         lListElem *qep;
         lList *alp = NULL;
         const char *q_set;

         slots = (int)lGetUlong(gdil_ep, JG_slots);
         qep = lFirst(lGetList(gdil_ep, JG_queue)); 
         q_set = lGetString(qep, QU_processors);
         pe_slots += slots;
         fprintf(fp, "%s %d %s %s\n", 
            lGetHost(gdil_ep, JG_qhostname),
            slots, 
            lGetString(gdil_ep, JG_qname), 
            q_set);
         if (!sge_hostcmp(lGetHost(master_q, QU_qhostname), lGetHost(gdil_ep, JG_qhostname))) {
            host_slots += slots;
            if (q_set && strcasecmp(q_set, "UNDEFINED")) {
               range_list_parse_from_string(&processor_set, &alp,
                                            lGetString(qep, QU_processors),
                                            0, 0, INF_ALLOWED);
               if (lGetNumberOfElem(alp))
                  alp = lFreeList(alp);
            }
         }
      }

      fclose(fp);
   }
   /*************************** finished writing sge hostfile  ********/

   /********************** setup environment file ***************************/
   sprintf(fname, "%s/%s/environment", execd_spool_dir, dir);
   fp = fopen(fname, "w");
   if (!fp) {
      sprintf(err_str, MSG_FILE_NOOPEN_SS, fname, strerror(errno));
      DEXIT;
      return -2;        /* general */
   }
   
   /* write inherited environment first, to be sure that some task specific variables
   ** will be overridden
   */
   {
      char buffer[2048];
      sprintf(buffer, "%s:/usr/local/bin:/usr/ucb:/bin:/usr/bin:", tmpdir);
      var_list_set_string(&environmentList, "PATH", buffer);
   }

   /* write environment of job */
   {   
      const char *s, *name;
      int n = strlen(VAR_COMPLEX_PREFIX);

      for_each(env, lGetList(jep, JB_env_list)) {
         name = lGetString(env, VA_variable);
         if (!strncmp(name, VAR_COMPLEX_PREFIX, n)) 
            continue; /* we handle them later on in var_list_copy_complex_vars_and_value() */

         s = lGetString(env, VA_value);
         var_list_set_string(&environmentList, name, s ? s : "");
      }
   }

   /* write environment of petask */
   if(petep != NULL) {
      const char *s, *name;
      int n = strlen(VAR_COMPLEX_PREFIX);

      for_each(env, lGetList(petep, PET_environment)) {
         name = lGetString(env, VA_variable);
         if (!strncmp(name, VAR_COMPLEX_PREFIX, n)) 
            continue; /* we handle them later on in var_list_copy_complex_vars_and_value() */

         s = lGetString(env, VA_value);
         var_list_set_string(&environmentList, name, s ? s : "");
      }
   }
  
   /* 1.) try to read cwd from pe task */
   if(petep != NULL) {
      cwd = lGetString(petep, PET_cwd);
      path_aliases = lGetList(petep, PET_path_aliases);
   } 

   /* 2.) try to read cwd from job */
   if(cwd == NULL) {
      cwd = lGetString(jep, JB_cwd);
      path_aliases = lGetList(jep, JB_path_aliases);
   }

   /* 3.) if pe task or job set cwd: do path mapping */
   if(cwd != NULL) {
      static char cwd_out[SGE_PATH_MAX];
     
      /* path aliasing only for cwd flag set */
      path_alias_list_get_path(path_aliases, NULL,
                               cwd, me.qualified_hostname, cwd_out, 
                               SGE_PATH_MAX);
      cwd = cwd_out;
      var_list_set_string(&environmentList, "PWD", cwd);
   } else {
   /* 4.) if cwd not set in job: take users home dir */
      cwd = pw->pw_dir;
   }   

   {
      const char *reqname = petep == NULL ? lGetString(jep, JB_job_name) : lGetString(petep, PET_name);
      if (reqname != NULL) {
         var_list_set_string(&environmentList, "REQNAME", reqname);
      }
   }

   var_list_set_string(&environmentList, VAR_PREFIX "CELL", me.default_cell);

   var_list_set_string(&environmentList, "HOME", pw->pw_dir);
   var_list_set_string(&environmentList, "SHELL", pw->pw_shell);
   var_list_set_string(&environmentList, "USER", pw->pw_name);
   var_list_set_string(&environmentList, "LOGNAME", pw->pw_name);

   /*
   ** set JOB_NAME variable
   ** for interactive jobs a fixed name is set,
   ** for batch jobs the script name is used.
   */
   {
      u_long32 jb_now;
      if(petep != NULL) {
         jb_now = JB_NOW_QRSH;
      } else {
         jb_now = lGetUlong(jep, JB_now);
      }

      JB_NOW_CLEAR_IMMEDIATE(jb_now);
      switch(jb_now) {
         case JB_NOW_QSH:
            var_list_set_string(&environmentList, "JOB_NAME", JB_NOW_STR_QSH); 
            break;
         case JB_NOW_QLOGIN:
            var_list_set_string(&environmentList, "JOB_NAME", JB_NOW_STR_QLOGIN);
            break;
         case JB_NOW_QRSH:
            var_list_set_string(&environmentList, "JOB_NAME", JB_NOW_STR_QRSH);
            break;
         case JB_NOW_QRLOGIN:
            var_list_set_string(&environmentList, "JOB_NAME", JB_NOW_STR_QRLOGIN);
            break;
         default:
            if (lGetString(jep, JB_script_file)) {
               const char *s;

               /* build basename */ 
               s=strrchr(lGetString(jep, JB_script_file), '/');
               if (s) 
                  s++;
               else 
                  s = lGetString(jep, JB_script_file);

               var_list_set_string(&environmentList, "JOB_NAME", s);
            }
            break;
      }
   }

   /* JG: TODO (ENV): do we need REQNAME and REQUEST? */
   var_list_set_string(&environmentList, "REQUEST", petep == NULL ? lGetString(jep, JB_job_name) : lGetString(petep, PET_name));
   var_list_set_string(&environmentList, "HOSTNAME", lGetHost(master_q, QU_qhostname));
   var_list_set_string(&environmentList, "QUEUE", lGetString(master_q, QU_qname));
   /* JB: TODO (ENV): shouldn't we better have a SGE_JOB_ID? */
   var_list_set_u32(&environmentList, "JOB_ID", lGetUlong(jep, JB_job_number));
  
   /* JG: TODO (ENV): shouldn't we better use SGE_JATASK_ID and have an additional SGE_PETASK_ID? */
   if (job_is_array(jep)) {
      var_list_set_u32(&environmentList, VAR_PREFIX "TASK_ID", lGetUlong(jatep, JAT_task_number));
   } else {
      var_list_set_string(&environmentList, VAR_PREFIX "TASK_ID", "undefined");
   }

   var_list_set_string(&environmentList, "ENVIRONMENT", "BATCH");
   var_list_set_string(&environmentList, "ARC", arch);

   var_list_set_string(&environmentList, VAR_PREFIX "ARCH", arch);
  
   if ((cp=getenv("TZ")) && strlen(cp))
      var_list_set_string(&environmentList, "TZ", cp);

   if ((cp=getenv("COMMD_PORT")) && strlen(cp))
      var_list_set_string(&environmentList, "COMMD_PORT", cp);
     
   var_list_set_string(&environmentList, VAR_PREFIX "ROOT", path.sge_root);

   var_list_set_int(&environmentList, "NQUEUES", 
      lGetNumberOfElem(lGetList(jatep, JAT_granted_destin_identifier_list)));
   var_list_set_int(&environmentList, "NSLOTS", pe_slots);
   var_list_set_int(&environmentList, "NHOSTS", nhosts);

   var_list_set_int(&environmentList, "RESTARTED", (int) lGetUlong(jatep, JAT_job_restarted));

   /*
   ** interactive and login jobs have no script file
   */
   if(petep != NULL) {
      /* JG: TODO (255): passing information if it is an interactive job and which type, should not be
       *           be done in scriptfile - find a better solution, e.g. a config variable 
       */
      strcpy(script_file, "QRSH");
   } else {
      if (lGetString(jep, JB_script_file) != NULL) {
         sprintf(script_file, "%s/%s/" u32, execd_spool_dir, EXEC_DIR, 
                 lGetUlong(jep, JB_job_number));
      } else { 
         strcpy(script_file, lGetString(lGetElemStr(environmentList, VA_variable, "JOB_NAME"), VA_value));
      }
   }

   var_list_set_string(&environmentList, "JOB_SCRIPT", script_file);

   var_list_set_string(&environmentList, "TMPDIR", tmpdir);
   var_list_set_string(&environmentList, "TMP", tmpdir);

   var_list_set_string(&environmentList, VAR_PREFIX "ACCOUNT", (lGetString(jep, JB_account) ? 
            lGetString(jep, JB_account) : DEFAULT_ACCOUNT));

   sge_get_path(lGetList(jep, JB_shell_list), cwd, 
                lGetString(jep, JB_owner),
                petep == NULL ? lGetString(jep, JB_job_name) : lGetString(petep, PET_name), 
                lGetUlong(jep, JB_job_number), 
                job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_SHELL, shell_path);

   if (shell_path[0] == 0)
      strcpy(shell_path, lGetString(master_q, QU_shell));
   var_list_set_string(&environmentList, "SHELL", shell_path);

   /* forward name of pe to job */
   if (lGetString(jatep, JAT_granted_pe) != NULL) {
      char buffer[SGE_PATH_MAX];
      var_list_set_string(&environmentList, "PE", lGetString(jatep, JAT_granted_pe));
      sprintf(buffer, "%s/%s/%s", execd_spool_dir, dir, PE_HOSTFILE);
      var_list_set_string(&environmentList, "PE_HOSTFILE", buffer);
   }   

   /* forward name of ckpt env to job */
   if ((ep = lFirst(lGetList(jep, JB_checkpoint_object_list)))) {
      var_list_set_string(&environmentList, VAR_PREFIX "CKPT_ENV", lGetString(ep, CK_name));
      if (lGetString(ep, CK_ckpt_dir))
         var_list_set_string(&environmentList, VAR_PREFIX "CKPT_DIR", lGetString(ep, CK_ckpt_dir));
   }

   {
      char buffer[SGE_PATH_MAX]; 

      sprintf(buffer, "%s/%s", execd_spool_dir, dir);  
      var_list_set_string(&environmentList, VAR_PREFIX "JOB_SPOOL_DIR", buffer);
   }

   var_list_copy_complex_vars_and_value(environmentList, 
                                        lGetList(jep, JB_env_list),
                                        cplx);

   var_list_set_sharedlib_path(&environmentList);

   if (set_sge_environment) {
      var_list_copy_prefix_vars(environmentList, environmentList,
                                VAR_PREFIX, "SGE_");
   }
   if (set_cod_environment) {
      var_list_copy_prefix_vars(environmentList, environmentList,
                                VAR_PREFIX, "COD_");
   }
   if (set_grd_environment) {
      var_list_copy_prefix_vars(environmentList, environmentList,
                                VAR_PREFIX, "GRD_");
   }
   var_list_remove_prefix_vars(environmentList, VAR_PREFIX);

   var_list_dump_to_file(environmentList, fp);
   fclose(fp);  
   /*************************** finished writing environment *****************/

   /**************** write out config file ******************************/
   sprintf(fname, "%s/config", dir);
   fp = fopen(fname, "w");
   if (!fp) {
      lFreeList(environmentList);
      sprintf(err_str, MSG_FILE_NOOPEN_SS, fname, strerror(errno));
      DEXIT;
      return -2;
   }

#ifdef COMPILE_DC

#  if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)

   {
      lList *rlp = NULL;
      lList *alp = NULL;
      gid_t temp_id;
      char str_id[256];
      
#     if defined(LINUX)

      if (!sup_groups_in_proc()) {
         lFreeList(environmentList);
         sprintf(err_str, MSG_EXECD_NOSGID); 
         fclose(fp);
         DEXIT;
         return(-2);
      }

#     endif
      
      /* parse range add create list */
      DPRINTF(("gid_range = %s\n", conf.gid_range));
      range_list_parse_from_string(&rlp, &alp, conf.gid_range,
                                   0, 0, INF_NOT_ALLOWED);
      if (rlp == NULL) {
          lFreeList(alp);
          sprintf(err_str, MSG_EXECD_NOPARSEGIDRANGE);
          lFreeList(environmentList);
          fclose(fp);
          DEXIT;
          return (-2);
      } 

      /* search next add_grp_id */
      temp_id = last_addgrpid;
      last_addgrpid = get_next_addgrpid (rlp, last_addgrpid);
      while (addgrpid_already_in_use(last_addgrpid)) {
         last_addgrpid = get_next_addgrpid (rlp, last_addgrpid);
         if (temp_id == last_addgrpid) {
            sprintf(err_str, MSG_EXECD_NOADDGID);
            lFreeList(environmentList);
            fclose(fp);
            DEXIT;
            return (-1);
         }
      }

      /* write add_grp_id to job-structure and file */
      sprintf(str_id, "%ld", (long) last_addgrpid);
      fprintf(fp, "add_grp_id="gid_t_fmt"\n", last_addgrpid);
      if(petep == NULL) {
         lSetString(jatep, JAT_osjobid, str_id);
      } else {
         lSetString(petep, PET_osjobid, str_id);
      }
      
      lFreeList(rlp);
      lFreeList(alp);

   }

#  endif

#endif

   /* handle stdout/stderr */
   sge_get_path(lGetList(jep, JB_stdout_path_list), cwd, 
                lGetString(jep, JB_owner), 
                lGetString(jep, JB_job_name),
                lGetUlong(jep, JB_job_number),
                job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_STDOUT, stdout_path);
   sge_get_path(lGetList(jep, JB_stderr_path_list), cwd,
                lGetString(jep, JB_owner), 
                lGetString(jep, JB_job_name),
                lGetUlong(jep, JB_job_number), 
                job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_STDERR, stderr_path);

   fprintf(fp, "stdout_path=%s\n", stdout_path);
   fprintf(fp, "stderr_path=%s\n", stderr_path);
   fprintf(fp, "merge_stderr=%d\n", (int)lGetUlong(jep, JB_merge_stderr));

   if (lGetUlong(jep, JB_checkpoint_attr) && 
       (ep = lFirst(lGetList(jep, JB_checkpoint_object_list)))) {
      fprintf(fp, "ckpt_job=1\n");
      fprintf(fp, "ckpt_restarted=%d\n", petep != NULL ? 0 : (int) lGetUlong(jatep, JAT_job_restarted));
      fprintf(fp, "ckpt_pid=%d\n", (int) lGetUlong(jatep, JAT_pvm_ckpt_pid));
      fprintf(fp, "ckpt_osjobid=%s\n", lGetString(jatep, JAT_osjobid) ? lGetString(jatep, JAT_osjobid): "0");
      fprintf(fp, "ckpt_interface=%s\n", lGetString(ep, CK_interface));
      fprintf(fp, "ckpt_command=%s\n", lGetString(ep, CK_ckpt_command));
      fprintf(fp, "ckpt_migr_command=%s\n", lGetString(ep, CK_migr_command));
      fprintf(fp, "ckpt_rest_command=%s\n", lGetString(ep, CK_rest_command));
      fprintf(fp, "ckpt_clean_command=%s\n", lGetString(ep, CK_clean_command));
      fprintf(fp, "ckpt_dir=%s\n", lGetString(ep, CK_ckpt_dir));
      fprintf(fp, "ckpt_signal=%s\n", lGetString(ep, CK_signal));
 
      if (!(lGetUlong(jep, JB_checkpoint_attr) & CHECKPOINT_AT_MINIMUM_INTERVAL)) {
         interval = 0;
      } else {   
         parse_ulong_val(NULL, &interval, TYPE_TIM, lGetString(master_q, QU_min_cpu_interval), NULL, 0);   
         interval = MAX(interval, lGetUlong(jep, JB_checkpoint_interval));
      }   
      fprintf(fp, "ckpt_interval=%d\n", (int) interval);
   } else {
      fprintf(fp, "ckpt_job=0\n");
   }   
      
   fprintf(fp, "s_cpu=%s\n", lGetString(master_q, QU_s_cpu));
   fprintf(fp, "h_cpu=%s\n", lGetString(master_q, QU_h_cpu));
   fprintf(fp, "s_core=%s\n", lGetString(master_q, QU_s_core));
   fprintf(fp, "h_core=%s\n", lGetString(master_q, QU_h_core));
   fprintf(fp, "s_data=%s\n", lGetString(master_q, QU_s_data));
   fprintf(fp, "h_data=%s\n", lGetString(master_q, QU_h_data));
   fprintf(fp, "s_stack=%s\n", lGetString(master_q, QU_s_stack));
   fprintf(fp, "h_stack=%s\n", lGetString(master_q, QU_h_stack));
   fprintf(fp, "s_rss=%s\n", lGetString(master_q, QU_s_rss));
   fprintf(fp, "h_rss=%s\n", lGetString(master_q, QU_h_rss));
   fprintf(fp, "s_fsize=%s\n", lGetString(master_q, QU_s_fsize));
   fprintf(fp, "h_fsize=%s\n", lGetString(master_q, QU_h_fsize));
   fprintf(fp, "s_vmem=%s\n", lGetString(master_q, QU_s_vmem));
   fprintf(fp, "h_vmem=%s\n", lGetString(master_q, QU_h_vmem));


   fprintf(fp, "priority=%s\n", lGetString(master_q, QU_priority));
   fprintf(fp, "shell_path=%s\n", shell_path);
   fprintf(fp, "script_file=%s\n", script_file);
   fprintf(fp, "job_owner=%s\n", lGetString(jep, JB_owner));
   fprintf(fp, "min_gid=" u32 "\n", conf.min_gid);
   fprintf(fp, "min_uid=" u32 "\n", conf.min_uid);
   fprintf(fp, "cwd=%s\n", cwd);
#if defined(IRIX6)
   {
      const char *env_value = job_get_env_string(jep, VAR_PREFIX "O_HOST");

      if (env_value) {
         fprintf(fp, "spi_initiator=%s\n", env_value);
      } else {
         fprintf(fp, "spi_initiator=%s\n", "unknown");
      }
   }
#endif

   /* do not start prolog/epilog in case of pe tasks */
   if(petep == NULL) {
      fprintf(fp, "prolog=%s\n", 
              ((cp=lGetString(master_q, QU_prolog)) && strcasecmp(cp, "none"))?
              cp: conf.prolog);
      fprintf(fp, "epilog=%s\n", 
              ((cp=lGetString(master_q, QU_epilog)) && strcasecmp(cp, "none"))?
              cp: conf.epilog);
   } else {
      fprintf(fp, "prolog=%s\n", "none");
      fprintf(fp, "epilog=%s\n", "none");
   }

   fprintf(fp, "starter_method=%s\n", (cp=lGetString(master_q, QU_starter_method))? cp : "none");
   fprintf(fp, "suspend_method=%s\n", (cp=lGetString(master_q, QU_suspend_method))? cp : "none");
   fprintf(fp, "resume_method=%s\n", (cp=lGetString(master_q, QU_resume_method))? cp : "none");
   fprintf(fp, "terminate_method=%s\n", (cp=lGetString(master_q, QU_terminate_method))? cp : "none");

   /* JG: TODO: should at least be a define, better configurable */
   fprintf(fp, "script_timeout=120\n");

   fprintf(fp, "pe=%s\n", lGetString(jatep, JAT_granted_pe)?lGetString(jatep, JAT_granted_pe):"none");
   fprintf(fp, "pe_slots=%d\n", pe_slots);
   fprintf(fp, "host_slots=%d\n", host_slots);

   /* write pe related data */
   if (lGetString(jatep, JAT_granted_pe)) {
      lListElem *pep = NULL;
      /* no pe start/stop for petasks */
      if(petep == NULL) {
         pep = lFirst(lGetList(jatep, JAT_pe_object));
      }
      fprintf(fp, "pe_hostfile=%s/%s/%s\n", execd_spool_dir, dir, PE_HOSTFILE);
      fprintf(fp, "pe_start=%s\n",  pep != NULL && lGetString(pep, PE_start_proc_args)?
                                       lGetString(pep, PE_start_proc_args):"none");
      fprintf(fp, "pe_stop=%s\n",   pep != NULL && lGetString(pep, PE_stop_proc_args)?
                                       lGetString(pep, PE_stop_proc_args):"none");

      /* build path for stdout of pe scripts */
      sge_get_path(lGetList(jep, JB_stdout_path_list), cwd, 
                   lGetString(jep, JB_owner), 
                   lGetString(jep, JB_job_name), 
                   lGetUlong(jep, JB_job_number), 
                   job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                   SGE_PAR_STDOUT, pe_stdout_path);
      fprintf(fp, "pe_stdout_path=%s\n", pe_stdout_path);

      /* build path for stderr of pe scripts */
      sge_get_path(lGetList(jep, JB_stderr_path_list), cwd,
                   lGetString(jep, JB_owner), 
                   lGetString(jep, JB_job_name), 
                   lGetUlong(jep, JB_job_number), 
                   job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                   SGE_PAR_STDERR, pe_stderr_path);
      fprintf(fp, "pe_stderr_path=%s\n", pe_stderr_path);
   }

   fprintf(fp, "shell_start_mode=%s\n", 
           job_get_shell_start_mode(jep, master_q, conf.shell_start_mode));

   /* we need the basename for loginshell test */
   shell = strrchr(shell_path, '/');
   if (!shell)
      shell = shell_path;
   else
      shell++;   

   fprintf(fp, "use_login_shell=%d\n", ck_login_sh(shell) ? 1 : 0);

   /* the following values are needed by the reaper */
   if (sge_unparse_ma_list(lGetList(jep, JB_mail_list), 
            mail_str, sizeof(mail_str))) {
      ERROR((SGE_EVENT, MSG_MAIL_MAILLISTTOOLONG_U, u32c(lGetUlong(jep, JB_job_number))));
   }
   fprintf(fp, "mail_list=%s\n", mail_str);
   fprintf(fp, "mail_options=" u32 "\n", lGetUlong(jep, JB_mail_options));
   fprintf(fp, "forbid_reschedule=%d\n", forbid_reschedule);
   fprintf(fp, "queue=%s\n", lGetString(master_q, QU_qname));
   fprintf(fp, "host=%s\n", lGetHost(master_q, QU_qhostname));
   {
      dstring range_string = DSTRING_INIT;

      range_list_print_to_string(processor_set, &range_string, 1);
      fprintf(fp, "processors=%s", sge_dstring_get_string(&range_string)); 
      sge_dstring_free(&range_string);
   }
   fprintf(fp, "\n");
   if(petep != NULL) {
      fprintf(fp, "job_name=%s\n", lGetString(petep, PET_name));
   } else {
      fprintf(fp, "job_name=%s\n", lGetString(jep, JB_job_name));
   }
   fprintf(fp, "job_id="u32"\n", lGetUlong(jep, JB_job_number));
   fprintf(fp, "ja_task_id="u32"\n", job_is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0);
   if(petep != NULL) {
      fprintf(fp, "pe_task_id=%s\n", lGetString(petep, PET_id));
   }
   fprintf(fp, "account=%s\n", (lGetString(jep, JB_account) ? lGetString(jep, JB_account) : DEFAULT_ACCOUNT));
   if(petep != NULL) {
      fprintf(fp, "submission_time=" u32 "\n", lGetUlong(petep, PET_submission_time));
   } else {
      fprintf(fp, "submission_time=" u32 "\n", lGetUlong(jep, JB_submission_time));
   }

   {
      u_long32 notify = 0;
      if (lGetUlong(jep, JB_notify))
         parse_ulong_val(NULL, &notify, TYPE_TIM, lGetString(master_q, QU_notify), NULL, 0);
      fprintf(fp, "notify=" u32 "\n", notify);
   }
   
   /*
   ** interactive (qsh) jobs have no exec file
   */
   if (!lGetString(jep, JB_script_file)) {
      u_long32 jb_now;
      if(petep != NULL) {
         jb_now = JB_NOW_QRSH;
      } else {
         jb_now = lGetUlong(jep, JB_now);
      }

      if (!(JB_NOW_IS_QLOGIN(jb_now) || 
           JB_NOW_IS_QRSH(jb_now) || 
           JB_NOW_IS_QRLOGIN(jb_now))) {
         /*
         ** get xterm configuration value
         */
         if (conf.xterm) {
            fprintf(fp, "exec_file=%s\n", conf.xterm);
         } else {
            sprintf(err_str, MSG_EXECD_NOXTERM); 
            fclose(fp);
            lFreeList(environmentList);
            DEXIT;
            return -2;
            /*
            ** this causes a general failure
            */
         }
      }
   }

   if ((cp = lGetString(jep, JB_project)) && *cp != '\0')
      fprintf(fp, "acct_project=%s\n", cp);
   else
      fprintf(fp, "acct_project=none\n");
         

   {
      lList *args;
      lListElem *se;

      int nargs=1;
     
      /* for pe tasks we have no args - the daemon (rshd etc.) to start
       * comes from the cluster configuration 
       */
      if(petep != NULL) {
         args = NULL;
      } else {
         args = lGetList(jep, JB_job_args);
      }

      fprintf(fp, "njob_args=%d\n", lGetNumberOfElem(args));

      for_each(se, args) {
         const char *arg = lGetString(se, STR);
         if(arg != NULL) {
            fprintf(fp, "job_arg%d=%s\n", nargs++, arg);
         } else {
            fprintf(fp, "job_arg%d=\n", nargs++);
         }
      }   
   }

   fprintf(fp, "queue_tmpdir=%s\n", lGetString(master_q, QU_tmpdir));
   /*
   ** interactive jobs need a display to send output back to (only qsh - JG)
   */
   if (!lGetString(jep, JB_script_file)) {         /* interactive job  */
      u_long32 jb_now;
      if(petep != NULL) {
         jb_now = JB_NOW_QRSH;
      } else {
         jb_now = lGetUlong(jep, JB_now);    /* detect qsh case  */
      }
      
      env = lGetElemStr(lGetList(jep, JB_env_list), VA_variable, "DISPLAY");

      if (!env) {                                  /* no DISPLAY set   */
         if(JB_NOW_IS_QSH(jb_now)) {               /* and qsh -> error */
            sprintf(err_str, MSG_EXECD_NODISPLAY);
            lFreeList(environmentList);
            fclose(fp);
            DEXIT;
            return -1;
         }
      } else {
         if (!lGetString(env, VA_value) || !strcmp(lGetString(env, VA_value), "")) {  /* no value for DISPLAY */
            if(JB_NOW_IS_QSH(jb_now)) {                                               /* and qsh -> error     */
               sprintf(err_str, MSG_EXECD_EMPTYDISPLAY);
               lFreeList(environmentList);
               fclose(fp);
               DEXIT;
               return -1;
            }
         } else { /* DISPLAY exists and is valid -> write to config */
            fprintf(fp, "display=%s\n", lGetString(env, VA_value));
         }
      }   
   }

   
   if (arch_dep_config(fp, cplx, err_str)) {
      lFreeList(environmentList);
      fclose(fp);
      DEXIT;
      return -2;
   } 

   /* if "pag_cmd" is not set, do not use AFS setup for this host */
   if (feature_is_enabled(FEATURE_AFS_SECURITY) && conf.pag_cmd &&
       strlen(conf.pag_cmd) && strcasecmp(conf.pag_cmd, "none")) {
      fprintf(fp, "use_afs=1\n");
      
      shepherd_name = SGE_COSHEPHERD;
      sprintf(coshepherd_path, "%s/%s/%s", conf.binary_path, sge_get_arch(), shepherd_name);
      fprintf(fp, "coshepherd=%s\n", coshepherd_path);
      fprintf(fp, "set_token_cmd=%s\n", conf.set_token_cmd ? conf.set_token_cmd : "none");
      fprintf(fp, "token_extend_time=%d\n", (int) conf.token_extend_time);
   }
   else
      fprintf(fp, "use_afs=0\n");

   fprintf(fp, "admin_user=%s\n", conf.admin_user);

   /* notify method */
   fprintf(fp, "notify_kill_type=%d\n", notify_kill_type);
   fprintf(fp, "notify_kill=%s\n", notify_kill?notify_kill:"default");
   fprintf(fp, "notify_susp_type=%d\n", notify_susp_type);
   fprintf(fp, "notify_susp=%s\n", notify_susp?notify_susp:"default");   
   if (use_qsub_gid)
      fprintf(fp, "qsub_gid="u32"\n", lGetUlong(jep, JB_gid));
   else
      fprintf(fp, "qsub_gid=%s\n", "no");

   fprintf(fp, "set_sge_env=%d\n", set_sge_environment);
   fprintf(fp, "set_cod_env=%d\n", set_cod_environment);
   fprintf(fp, "set_grd_env=%d\n", set_grd_environment);

   /* config for interactive jobs */
   {
      u_long32 jb_now;
      if(petep != NULL) {
         jb_now = JB_NOW_QRSH;
      } else {
         jb_now = lGetUlong(jep, JB_now); 
      }
     
      if(petep != NULL) {
         fprintf(fp, "pe_task_id=%s\n", lGetString(petep, PET_id));
      }   

      if(JB_NOW_IS_QLOGIN(jb_now) || JB_NOW_IS_QRSH(jb_now) 
         || JB_NOW_IS_QRLOGIN(jb_now)) {
         lListElem *elem;
         char daemon[SGE_PATH_MAX];

         fprintf(fp, "master_host=%s\n", sge_get_master(0));
         fprintf(fp, "commd_port=%d\n", ntohs(get_commlib_state_commdport()));
               
         if((elem=lGetElemStr(environmentList, VA_variable, "QRSH_PORT")) != NULL) {
            fprintf(fp, "qrsh_control_port=%s\n", lGetString(elem, VA_value));
         }
        
         sprintf(daemon, "%s/utilbin/%s/", path.sge_root, arch);
        
         if(JB_NOW_IS_QLOGIN(jb_now)) {
            fprintf(fp, "qlogin_daemon=%s\n", conf.qlogin_daemon);
         } else {
            if(JB_NOW_IS_QRSH(jb_now)) {
               strcat(daemon, "rshd");
               if(strcasecmp(conf.rsh_daemon, "none") == 0) {
                  strcat(daemon, " -l");
                  fprintf(fp, "rsh_daemon=%s\n", daemon);
                  write_osjob_id = 0; /* will be done by our rshd */
               } else {
                  fprintf(fp, "rsh_daemon=%s\n", conf.rsh_daemon);
                  if(strncmp(daemon, conf.rsh_daemon, strlen(daemon)) == 0) {
                     write_osjob_id = 0; /* will be done by our rshd */
                  }
               }

               fprintf(fp, "qrsh_tmpdir=%s\n", tmpdir);

               if(petep != NULL) {
                  fprintf(fp, "qrsh_pid_file=%s/pid.%s\n", tmpdir, lGetString(petep, PET_id));
               } else {
                  fprintf(fp, "qrsh_pid_file=%s/pid\n", tmpdir);
               }
            } else {
               if(JB_NOW_IS_QRLOGIN(jb_now)) {
                  strcat(daemon, "rlogind");
                  if(strcasecmp(conf.rlogin_daemon, "none") == 0) {
                     strcat(daemon, " -l");
                     fprintf(fp, "rlogin_daemon=%s\n", daemon);
                     write_osjob_id = 0; /* will be done by our rlogind */
                  } else {   
                     fprintf(fp, "rlogin_daemon=%s\n", conf.rlogin_daemon);
                     if(strncmp(daemon, conf.rlogin_daemon, strlen(daemon)) == 0) {
                        write_osjob_id = 0; /* will be done by our rlogind */
                     }
                  }   
               }
            }
         }   
      }
   }   

   /* shall shepherd write osjob_id, or is it done by (our) rshd */
   fprintf(fp, "write_osjob_id=%d", write_osjob_id);

   lFreeList(environmentList);
   fclose(fp);
   /********************** finished writing config ************************/

   /* test whether we can access scriptfile */
   /*
   ** interactive jobs dont need to access script file
   */
   /* JG: TODO (255): find better way to identify interactive jobs */
   if (!lGetString(jep, JB_job_source) && petep == NULL && lGetString(jep, JB_script_file)) {
      if (SGE_STAT(script_file, &buf)) {
         sprintf(err_str, MSG_EXECD_UNABLETOFINDSCRIPTFILE_SS,
                 script_file, strerror(errno));
         DEXIT;
         return -2;
      }
   }

   shepherd_name = SGE_SHEPHERD;
   sprintf(shepherd_path, "%s/%s/%s", conf.binary_path, arch, shepherd_name);

   if (SGE_STAT(shepherd_path, &buf)) {
      /* second chance: without architecture */
      sprintf(shepherd_path, "%s/%s", conf.binary_path, shepherd_name);
      if (SGE_STAT(shepherd_path, &buf)) {
         sprintf(err_str, MSG_EXECD_NOSHEPHERD_SSS, arch, shepherd_path, strerror(errno));
         DEXIT;
         return -2;
      }
   }

   if (conf.shepherd_cmd && strlen(conf.shepherd_cmd) &&
       strcasecmp(conf.shepherd_cmd, "none")) {
      if (SGE_STAT(conf.shepherd_cmd, &buf)) {
         sprintf(err_str, MSG_EXECD_NOSHEPHERDWRAP_SS, conf.shepherd_cmd, strerror(errno));
         DEXIT;
         return -2;
      }
   }
   else if (do_credentials && feature_is_enabled(FEATURE_DCE_SECURITY)) {
      sprintf(dce_wrapper_cmd, "/%s/utilbin/%s/starter_cred",
              path.sge_root, arch);
      if (SGE_STAT(dce_wrapper_cmd, &buf)) {
         sprintf(err_str, MSG_DCE_NOSHEPHERDWRAP_SS, dce_wrapper_cmd, strerror(errno));
         DEXIT;
         return -2;
      }
   }
   else if (feature_is_enabled(FEATURE_AFS_SECURITY) && conf.pag_cmd &&
            strlen(conf.pag_cmd) && strcasecmp(conf.pag_cmd, "none")) {
      int fd, len;
      const char *cp;

      if (SGE_STAT(coshepherd_path, &buf)) {
         shepherd_name = SGE_COSHEPHERD;
         sprintf(coshepherd_path, "%s/%s", conf.binary_path, shepherd_name);
         if (SGE_STAT(coshepherd_path, &buf)) {
            sprintf(err_str, MSG_EXECD_NOCOSHEPHERD_SSS, arch, coshepherd_path, strerror(errno));
            DEXIT;
            return -2;
         }
      }
      if (!conf.set_token_cmd ||
          !strlen(conf.set_token_cmd) || !conf.token_extend_time) {
         sprintf(err_str, MSG_EXECD_AFSCONFINCOMPLETE);
         DEXIT;
         return -2;
      }

      sprintf(fname, "%s/%s", dir, TOKEN_FILE);
      if ((fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600)) == -1) {
         sprintf(err_str, MSG_EXECD_NOCREATETOKENFILE_S, strerror(errno));
         DEXIT;
         return -2;
      }   
      
      cp = lGetString(jep, JB_tgt);
      if (!cp || !(len = strlen(cp))) {
         sprintf(err_str, MSG_EXECD_TOKENZERO);
         DEXIT;
         return -3; /* problem of this user */
      }
      if (write(fd, cp, len) != len) {
         sprintf(err_str, MSG_EXECD_NOWRITETOKEN_S, strerror(errno));
         DEXIT;
         return -2;
      }
      close(fd);
   }

   /* send mail to users if requested */
   if(petep == NULL) {
      mail_users = lGetList(jep, JB_mail_list);
      mail_options = lGetUlong(jep, JB_mail_options);
      strcpy(sge_mail_start, sge_ctime(lGetUlong(jatep, JAT_start_time)));
      if (VALID(MAIL_AT_BEGINNING, mail_options)) {
         if (job_is_array(jep)) {
            sprintf(sge_mail_subj, MSG_MAIL_STARTSUBJECT_UUS, u32c(lGetUlong(jep, JB_job_number)),
                    u32c(lGetUlong(jatep, JAT_task_number)), lGetString(jep, JB_job_name));
            sprintf(sge_mail_body, MSG_MAIL_STARTBODY_UUSSSSS,
                u32c(lGetUlong(jep, JB_job_number)),
                u32c(lGetUlong(jatep, JAT_task_number)),
                lGetString(jep, JB_job_name),
                lGetString(jep, JB_owner), 
                lGetString(master_q, QU_qname),
                lGetHost(master_q, QU_qhostname), sge_mail_start);
         } 
         else {
            sprintf(sge_mail_subj, MSG_MAIL_STARTSUBJECT_US, u32c(lGetUlong(jep, JB_job_number)),
               lGetString(jep, JB_job_name));
            sprintf(sge_mail_body, MSG_MAIL_STARTBODY_USSSSS,
                u32c(lGetUlong(jep, JB_job_number)),
                lGetString(jep, JB_job_name),
                lGetString(jep, JB_owner),
                lGetString(master_q, QU_qname),
                lGetHost(master_q, QU_qhostname), sge_mail_start);
         }
         cull_mail(mail_users, sge_mail_subj, sge_mail_body, "job start");
      }
   }

   /* Change to jobs directory. Father changes back to cwd. We do this to
      ensure chdir() works before forking. */
   if (chdir(dir)) {
      sprintf(err_str, MSG_FILE_CHDIR_SS, dir, strerror(errno));
      DEXIT;
      return -2;
   }

   /* now fork and exec the shepherd */
   if (getenv("SGE_FAILURE_BEFORE_FORK") || getenv("SGE_FAILURE_BEFORE_FORK")) {
      i = -1;
   }
   else
      i = fork();

   if (i != 0) { /* parent */
      if(petep == NULL) {
         /* nothing to be done for petasks: We do not signal single petasks, but always the whole jatask */
         lSetUlong(jep, JB_hard_wallclock_gmt, 0); /* in case we are restarting! */
         lSetUlong(jatep, JAT_pending_signal, 0);
         lSetUlong(jatep, JAT_pending_signal_delivery_time, 0);
      } 

      if (chdir(execd_spool_dir))       /* go back */
         /* if this happens (dont know how) we have a real problem */
         ERROR((SGE_EVENT, MSG_FILE_CHDIR_SS, execd_spool_dir, strerror(errno))); 
      if (i == -1) {
         if (getenv("SGE_FAILURE_BEFORE_FORK") || getenv("SGE_FAILURE_BEFORE_FORK"))
            strcpy(err_str, "FAILURE_BEFORE_FORK");
         else
            sprintf(err_str, MSG_EXECD_NOFORK_S, strerror(errno));
      }

      DEXIT;
      return i;
   }

   {  /* close all fd's except 0,1,2 */ 
      fd_set keep_open;
      FD_ZERO(&keep_open); 
      FD_SET(0, &keep_open);
      FD_SET(1, &keep_open);
      FD_SET(2, &keep_open);
      sge_close_all_fds(&keep_open);
   }
   
#ifdef __sgi

   /* turn off non-degrading priority */
   schedctl(NDPRI, 0, 0);

#endif

   /*
    * set KRB5CCNAME so shepherd assumes user's identify for
    * access to DFS or AFS file systems
    */
   if ((feature_is_enabled(FEATURE_DCE_SECURITY) ||
        feature_is_enabled(FEATURE_KERBEROS_SECURITY)) &&
       lGetString(jep, JB_cred)) {

      char ccname[1024];
      sprintf(ccname, "KRB5CCNAME=FILE:/tmp/krb5cc_%s_" u32, "sge",
	      lGetUlong(jep, JB_job_number));
      putenv(ccname);
   }

   DPRINTF(("**********************CHILD*********************\n"));
   shepherd_name = SGE_SHEPHERD;
   sprintf(ps_name, "%s-"u32, shepherd_name, lGetUlong(jep, JB_job_number));

   if (conf.shepherd_cmd && strlen(conf.shepherd_cmd) && 
      strcasecmp(conf.shepherd_cmd, "none")) {
      DPRINTF(("CHILD - About to exec shepherd wrapper job ->%s< under queue -<%s<\n", 
              lGetString(jep, JB_job_name), 
              lGetString(master_q, QU_qname)));
      execlp(conf.shepherd_cmd, ps_name, NULL);
   }
   else if (do_credentials && feature_is_enabled(FEATURE_DCE_SECURITY)) {
      DPRINTF(("CHILD - About to exec DCE shepherd wrapper job ->%s< under queue -<%s<\n", 
              lGetString(jep, JB_job_name), 
              lGetString(master_q, QU_qname)));
      execlp(dce_wrapper_cmd, ps_name, NULL);
   }
   else if (!feature_is_enabled(FEATURE_AFS_SECURITY) || !conf.pag_cmd ||
            !strlen(conf.pag_cmd) || !strcasecmp(conf.pag_cmd, "none")) {
      DPRINTF(("CHILD - About to exec ->%s< under queue -<%s<\n",
              lGetString(jep, JB_job_name), 
              lGetString(master_q, QU_qname)));

      if (ISTRACE)
         execlp(shepherd_path, ps_name, NULL);
      else
        execlp(shepherd_path, ps_name, "-bg", NULL);
   }
   else {
      char commandline[2048];

      DPRINTF(("CHILD - About to exec PAG command job ->%s< under queue -<%s<\n",
              lGetString(jep, JB_job_name), lGetString(master_q, QU_qname)));
      if (ISTRACE)
         sprintf(commandline, "exec %s", shepherd_path);
      else
        sprintf(commandline, "exec %s -bg", shepherd_path);

      execlp(conf.pag_cmd, conf.pag_cmd, "-c", commandline, NULL);
   }


   /*---------------------------------------------------*/
   /* exec() failed - do what shepherd does if it fails */

   fp = fopen("error", "w");
   if (fp) {
      fprintf(fp, "failed to exec shepherd for job" u32"\n", lGetUlong(jep, JB_job_number));
      fclose(fp);
   }

   fp = fopen("exit_status", "w");
   if (fp) {
      fprintf(fp, "1\n");
      fclose(fp);
   }

   CRITICAL((SGE_EVENT, MSG_EXECD_NOSTARTSHEPHERD));

   exit(1);

   /* just to please insure */
   return -1;
}

/*****************************************************
 check whether a shell should be called as login shell
 *****************************************************/
static int ck_login_sh(
char *shell 
) {
   char *cp;
   int ret; 

   DENTER(TOP_LAYER, "ck_login_sh");

   cp = conf.login_shells;
  
   if (!cp) {
      DEXIT; 
      return 0;
   }   

   while (*cp) {

      /* skip delimiters */
      while (*cp && ( *cp == ',' || *cp == ' ' || *cp == '\t'))
         cp++;
   
      ret = strncmp(cp, shell, strlen(shell));
      DPRINTF(("strncmp(\"%s\", \"%s\", %d) = %d\n",
              cp, shell, strlen(shell), ret));
      if (!ret) {
         DEXIT;  
         return 1;
      }

      /* skip name of shell, proceed until next delimiter */
      while (*cp && *cp != ',' && *cp != ' ' && *cp != '\t')
          cp++;
   }

  DEXIT;
  return 0;
}

   
static int get_nhosts(
lList *gdil_orig  /* JG_Type */
) {
   int nhosts = 0;
   lListElem *ep;
   lCondition *where;
   lList *gdil_copy;

   DENTER(TOP_LAYER, "get_nhosts");

   gdil_copy = lCopyList("", gdil_orig);   
   while ((ep=lFirst(gdil_copy))) {
      /* remove all gdil_copy-elements with the same JG_qhostname */
      where = lWhere("%T(!(%I h= %s))", JG_Type, JG_qhostname, 
              lGetHost(ep, JG_qhostname));
      if (!where) {
         lFreeList(gdil_copy);
         CRITICAL((SGE_EVENT, MSG_EXECD_NOWHERE4GDIL_S,"JG_qhostname"));
         DEXIT;
         return -1;
      }
      gdil_copy = lSelectDestroy(gdil_copy, where);   
      lFreeWhere(where);

      nhosts++;
   }

   DEXIT;
   return nhosts;
} 

static int arch_dep_config(
FILE *fp,
lList *cplx,
char *err_str 
) {
   static char *cplx2config[] = {
#if defined(NECSX4) || defined(NECSX5)
      /* Scheduling parameter */
      "nec_basepriority",     /* Base priorityi (>-20) */
      "nec_modcpu",           /* Modification factor cpu (0-39) */
      "nec_tickcnt",          /* Tick count (pos. int) */
      "nec_dcyfctr",          /* Decay factor (0-7) */
      "nec_dcyintvl",         /* Decay interval (pos. int) */
      "nec_timeslice",        /* Time slice (pos. int) */
      "nec_memorypriority",   /* Memory priority (0-39) */
      "nec_mrt_size_effct",   /* MRT size effect (0-1000) */
      "nec_mrt_pri_effct",    /* MRT priority effect (20-1000) */
      "nec_mrt_minimum",      /* MRT minimum (pos. int) */
      "nec_aging_range",      /* Aging range (pos. int) */
      "nec_slavepriority",    /* Slave priority () */
      "nec_cpu_count",        /* Cpu count (pos. int) */
      /* NEC specific limits */
      "s_tmpf",               /* Temporary file capacity limit */
      "h_tmpf",
      "s_mtdev",              /* tape drives limit */
      "h_mtdev",              
      "s_nofile",             /* open file limit */
      "h_nofile",
      "s_proc",               /* process number limit */
      "h_proc",
      "s_rlg0",               /* FSG Limits */
      "h_rlg0",
      "s_rlg1",
      "h_rlg1",
      "s_rlg2",
      "h_rlg2",
      "s_rlg3",
      "h_rlg3",
      "s_cpurestm",           /* CPU resident time */
      "h_cpurestm",
#endif
      NULL
   };

   int i, failed = 0;
   const char *s;
   lListElem *attr;

   DENTER(TOP_LAYER, "arch_dep_config");

   for (i=0; cplx2config[i]; i++) {
      if ( (attr=lGetElemStr(cplx, CE_name, cplx2config[i]))
            && (s=lGetString(attr, CE_stringval))) {
         DPRINTF(("\"%s\" = \"%s\"\n", cplx2config[i], s));
         fprintf(fp, "%s=%s\n", cplx2config[i], s);
      } else {
         sprintf(err_str, MSG_EXECD_NEEDATTRXINUSERDEFCOMPLOFYQUEUES_SS,
               cplx2config[i], sge_get_arch());
         ERROR((SGE_EVENT, err_str));
         failed = 1;
      }
   }

   if (failed) {
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}
