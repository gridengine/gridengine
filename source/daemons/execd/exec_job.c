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
#include <unistd.h>  
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
#include "sge_ckptL.h"
#include "sge_complexL.h"
#include "sge_exit.h"
#include "sge_queueL.h"
#include "sge_stringL.h"
#include "sge_answerL.h"
#include "sge_rangeL.h"
#include "parse.h"
#include "sge_dir.h"
#include "get_path.h"
#include "sge_arch.h"
#include "sge_job.h"
#include "tmpdir.h"
#include "read_write_queue.h"
#include "exec_job.h"
#include "path_aliases.h"
#include "slots_used.h"
#include "sge_parse_num_par.h"
#include "show_job.h"
#include "mail.h"
#include "sge_me.h"
#include "sgermon.h"
#include "parse_range.h"
#include "sge_getpwnam.h"
#include "commlib.h"
#include "sge_copy_append.h"
#include "sge_peopen.h"
#include "basis_types.h"
#include "sgedefs.h"
#include "exec_ifm.h"
#include "pdc.h"
#include "sge_afsutil.h"
#include "sge_switch_user.h"
#include "job.h"
#include "sge_prognames.h"
#include "setup_path.h"
#include "qm_name.h"
#include "sge_stat.h" 
#include "sge_string.h" 
#include "utility.h"
#include "jb_now.h"
#include "sge_feature.h"
#include "sge_job_jatask.h"

#include "msg_common.h"
#include "msg_execd.h"
#include "msg_daemons_common.h"

#define ENVIRONMENT_FILE "environment"
#define CONFIG_FILE "config"

#define COMPLEX2ENV_PREFIX "SGE_COMPLEX_"

static int ck_login_sh(char *shell);
static int get_nhosts(lList *gdil_list);
static int arch_dep_config(FILE *fp, lList *cplx, char *err_str);
static int complex2environment(lList *env, lList *cplx, lListElem *job, char *err_str);
static void add_no_replace_env(lList *envl, const char *name, const char *value);
static void add_no_replace_env_u32(lList *envl, const char *name, u_long32 value);

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
lListElem *slave_jep,
lListElem *slave_jatep 
) {
   lListElem *master_q = NULL;

   DENTER(TOP_LAYER, "responsible_queue");

   if (!slave_jep)
      master_q = lFirst(lGetList(lFirst(lGetList(jatep, 
         JAT_granted_destin_identifier_list)), JG_queue));
   else {
      /* seek responsible queue for this task in gdil of slave job */
      const char *qnm;
      lListElem *gdil_ep;

      qnm = lGetString(lFirst(lGetList(jatep, JAT_granted_destin_identifier_list)), 
         JG_qname);
      /* DPRINTF(("queue to seek for: %s\n", qnm)); */
      for_each (gdil_ep, lGetList(slave_jatep, JAT_granted_destin_identifier_list)) {
         if (!strcmp(qnm, lGetString(gdil_ep, JG_qname))) {
            master_q = lFirst(lGetList(gdil_ep, JG_queue));
            break;
         }
      }
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
            id = lGetString(lFirst(lGetList(pe_task, JB_ja_tasks)), JAT_osjobid);
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

/****** execd/add_or_replace_env() ***************************************
*
*  NAME
*     add_or_replace_env -- add or change an environment variable
*
*  SYNOPSIS
*     void add_or_replace_env(lList *envl, const char *name, const char *value);
*
*  FUNCTION
*     If the environment variable <name> does not already exist in <envl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     envl  - the environment list where to set a variable
*     name  - the name of the variable
*     value - the (new) value of the variable
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*  BUGS
*
*  SEE ALSO
*     execd/add_or_replace_env()
*
****************************************************************************
*/
static void add_or_replace_env(lList *envl, const char *name, const char *value) 
{
   lListElem *elem;
   
   if(envl == NULL || name == NULL || value == NULL) {
      return;
   }

   if((elem = lGetElemStr(envl, VA_variable, name)) == NULL) {
      elem = lAddElemStr(&envl, VA_variable, name, VA_Type);
   }

   lSetString(elem, VA_value, value);
}

static void add_no_replace_env(lList *envl, const char *name, const char *value) 
{
   lListElem *elem;
   
   if(envl == NULL || name == NULL || value == NULL) {
      return;
   }

   if((elem = lGetElemStr(envl, VA_variable, name)) == NULL) {
      elem = lAddElemStr(&envl, VA_variable, name, VA_Type);
      lSetString(elem, VA_value, value);
   }
}

/****** execd/add_or_replace_env_int() ***************************************
*
*  NAME
*     add_or_replace_env_int -- add or change an environment variable
*
*  SYNOPSIS
*     void add_or_replace_env_int(lList *envl, const char *name, int value);
*
*  FUNCTION
*     If the environment variable <name> does not already exist in <envl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     envl  - the environment list where to set a variable
*     name  - the name of the variable
*     value - the (new) value of the variable
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*  BUGS
*
*  SEE ALSO
*     execd/add_or_replace_env()
*
****************************************************************************
*/
static void add_or_replace_env_int(lList *envl, const char *name, int value)
{
   char buffer[2048];

   sprintf(buffer, "%d", value);
   add_or_replace_env(envl, name, buffer);
}

/****** execd/add_or_replace_env_u32() ***************************************
*
*  NAME
*     add_or_replace_env_u32 -- add or change an environment variable
*
*  SYNOPSIS
*     void add_or_replace_env_u32(lList *envl, const char *name, 
*                                 u_long32 value);
*
*  FUNCTION
*     If the environment variable <name> does not already exist in <envl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     envl  - the environment list where to set a variable
*     name  - the name of the variable
*     value - the (new) value of the variable
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*  BUGS
*
*  SEE ALSO
*     execd/add_or_replace_env()
*
****************************************************************************
*/
static void add_or_replace_env_u32(lList *envl, const char *name, u_long32 value)
{
   char buffer[2048];

   sprintf(buffer, u32, value);
   add_or_replace_env(envl, name, buffer);
}

static void add_no_replace_env_u32(lList *envl, const char *name, u_long32 value)
{
   char buffer[2048];

   sprintf(buffer, u32, value);
   add_no_replace_env(envl, name, buffer);
}

/****** execd/dump_env() ***************************************
*
*  NAME
*     dump_env -- dump environment from list to file
*
*  SYNOPSIS
*     void dump_env(lList envl, FILE *file);
*
*  FUNCTION
*     Parses the list of type VA_type <envl> containing the
*     description of environment variables.
*     Dumps all list elements to <file> in the format:
*     <name>=<value>
*     <name> is read from VA_variable, <value> from VA_value.
*
*  INPUTS
*     envl - list of environment variables
*     file - filehandle of a previously opened (for writing) file
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/
static void dump_env(lList *envl, FILE *file) 
{
   lListElem *elem;
   
   if(envl == NULL || file == NULL) {
      return;
   }

   for_each(elem, envl) {
      fprintf(file, "%s=%s\n", lGetString(elem, VA_variable), lGetString(elem, VA_value));
   }
}
/****** execd/get_sharedlib_path_name() ***************************************
*
*  NAME
*     get_sharedlib_path_name -- return name of sharedlib path
*
*  SYNOPSIS
*     static const char *get_sharedlib_path_name(void);
*
*  FUNCTION
*     Returns the operating dependent name of the shared library path
*     (e.g. LIBPATH, SHLIB_PATH, LD_LIBRARY_PATH).
*     If the name of the sharedlib path is not known for an operating
*     system (the port has not yet been done), a compile time error
*     is raised.
*
*  INPUTS
*
*  RESULT
*     Name of the shared lib path
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*     Raising a compile time error (instead of e.g. just returning NULL
*     or LD_LIBRARY_PATH as default) has the following reason:
*     Setting the shared lib path is a very sensible operation concerning
*     security.
*     Example: If a shared linked rshd (called for qrsh execution) is
*     executed with a faked shared lib path, operations defined in
*     a non sge library libgdi.so might be executed as user root.
*
*  BUGS
*
*  SEE ALSO
*
****************************************************************************
*/
static const char *get_sharedlib_path_name(void) {
#if defined(AIX)
   return "LIBPATH";
#elif defined(HP10) || defined(HP11)
   return "SHLIB_PATH";
#elif defined(ALPHA) || defined(IRIX6) || defined(IRIX65) || defined(LINUX) || defined(SOLARIS) || defined(FREEBSD)
   return "LD_LIBRARY_PATH";
#elif defined(DARWIN)
   return "DYLD_LIBRARY_PATH";
#else
#error "don't know how to set shared lib path on this architecture"
   return NULL; /* never reached */
#endif
}

/****** execd/set_sharedlib_path() ***************************************
*
*  NAME
*     set_sharedlib_path -- set the shared library search path
*
*  SYNOPSIS
*     void set_sharedlib_path(lList environmentList);
*
*  FUNCTION
*     Sets or replaces the shared lib path in the list of environment
*     variables.
*     The SGE shared lib path is always set to the beginning of the
*     resulting shared lib path (security, see get_sharedlib_path_name())
*
*  INPUTS
*     environmentList - list of environment variables
*
*  RESULT
*
*  EXAMPLE
*
*  NOTES
*     Function should be moved to some library - same or similar code
*     might be used in other modules.
*
*  BUGS
*
*  SEE ALSO
*     execd/get_sharedlib_path_name()
*
****************************************************************************
*/

static void set_sharedlib_path(lList *environmentList) {
   char *sharedlib_path;
   char *sge_sharedlib_path;
   const char *sharedlib_path_name = get_sharedlib_path_name();
   lListElem *sharedlib_elem = NULL;

   DENTER(TOP_LAYER, "set_sharedlib_path");
   
   /* this is the SGE sharedlib path */
   sge_sharedlib_path = sge_malloc(strlen(path.sge_root) + strlen("/lib/") + strlen(sge_get_arch()) + 1);
   sprintf(sge_sharedlib_path, "%s/lib/%s", path.sge_root, sge_get_arch());

   /* if allready in environment: extend by SGE sharedlib path, else set */
   if((sharedlib_elem = lGetElemStr(environmentList, VA_variable, sharedlib_path_name)) != NULL) {
      const char *old_value = lGetString(sharedlib_elem, VA_value);

      if(old_value && strlen(old_value) > 0) {
         DPRINTF(("sharedlib path %s allready set:\n", sharedlib_path_name));
         sharedlib_path = sge_malloc(strlen(old_value) + 1 + strlen(sge_sharedlib_path) + 1);
         strcpy(sharedlib_path, sge_sharedlib_path);
         strcat(sharedlib_path, ":");
         strcat(sharedlib_path, old_value);
         lSetString(sharedlib_elem, VA_value, sharedlib_path);
         free(sharedlib_path);
      } else {
         DPRINTF(("overwriting empty sharedlib path %s\n", sharedlib_path_name));
         lSetString(sharedlib_elem, VA_value, sge_sharedlib_path);
      }
   } else {
      DPRINTF(("creating new sharedlib path %s\n", sharedlib_path_name));
      sharedlib_elem = lAddElemStr(&environmentList, VA_variable, sharedlib_path_name, VA_Type);
      lSetString(sharedlib_elem, VA_value, sge_sharedlib_path);
   }

   free(sge_sharedlib_path);
   DEXIT;
}

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
lListElem *slave_jep,
lListElem *slave_jatep,
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
   lListElem *ep, *job_jep, *job_jatep;
   lListElem *env;
   lList *environmentList;
   const char *arch = sge_get_arch();

   int write_osjob_id = 1;

   DENTER(TOP_LAYER, "sge_exec_job");

   SGE_ASSERT((jep));
   SGE_ASSERT((jatep));

   environmentList = lCreateList("environment list", VA_Type);

   DPRINTF(("Job: %ld Task: %ld\n", lGetUlong(jep, JB_job_number), 
      lGetUlong(jatep, JAT_task_number)));

   master_q = responsible_queue(jep, jatep, slave_jep, slave_jatep);
   SGE_ASSERT((master_q));

   /* prepare complex of master_q */
   if (!(cplx = lGetList( lGetElemStr( lGetList(slave_jatep?slave_jatep:jatep, 
         JAT_granted_destin_identifier_list), 
         JG_qname, lGetString(master_q, QU_qname)), JG_complex))) {
      environmentList = lFreeList(environmentList);
      DEXIT;
      return -2;
   } 

   pw = sge_getpwnam(lGetString(jep, JB_owner));
   if (!pw) {
      sprintf(err_str, MSG_SYSTEM_GETPWNAMFAILED_S, lGetString(jep, JB_owner));
      environmentList = lFreeList(environmentList);
      DEXIT;
      return -3;        /* error only relevant for this user */
   }

   if (slave_jep) {
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
            environmentList = lFreeList(environmentList);
            DEXIT;
            return -2;
         }
      }
      /*
      ** Now we can create the directory for the pe-task
      */  
      sprintf(dir, "%s/"u32"."u32"/%s", ACTIVE_DIR, lGetUlong(jep, JB_job_number), 
         lGetUlong(jatep, JAT_task_number), lGetString(jep, JB_pe_task_id_str));
   } else {
      sprintf(dir, "%s/"u32"."u32"", ACTIVE_DIR, lGetUlong(jep, JB_job_number), 
         lGetUlong(jatep, JAT_task_number));
   }

   /* Create job or (pe) task directory */ 
   if (mkdir(dir, 0755) == -1) {
      if (errno == EEXIST) {
         DPRINTF(("cleaning active job dir\n"));
         if (recursive_rmdir(dir, SGE_EVENT)) {
            sprintf(err_str, MSG_FILE_RMDIR_SS, dir, SGE_EVENT);
            environmentList = lFreeList(environmentList);
            DEXIT;
            return -2;
         }
         if (mkdir(dir, 0755) == -1) {
            ERROR((SGE_EVENT, MSG_FILE_CREATEDIRDEL_SS, dir, strerror(errno)));
            environmentList = lFreeList(environmentList);
            DEXIT;
            return -2;
         }
      } else {
         sprintf(err_str, MSG_FILE_CREATEDIR_SS, dir, strerror(errno));
         environmentList = lFreeList(environmentList);
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
         environmentList = lFreeList(environmentList);
         DEXIT;
         return -2;
      }
   } else {
      if(!(sge_get_tmpdir(master_q, 
                          lGetUlong(jep, JB_job_number), 
                          lGetUlong(jatep, JAT_task_number), 
                          tmpdir))) {
         sprintf(err_str, MSG_SYSTEM_CANTGETTMPDIR);
         environmentList = lFreeList(environmentList);
         DEXIT;
         return -2;
      }                    
   }

   /* increment used slots */
   DPRINTF(("%s: used slots increased from %d to %d\n", lGetString(master_q, QU_qname), 
         used_slots, used_slots+1));
   set_qslots_used(master_q, used_slots+1);

   /***************** write out sge host file ******************************/
   if (!slave_jep) {
      if (processor_set)
         processor_set = lFreeList(processor_set);

      sprintf(hostfilename, "%s/%s/%s", execd_spool_dir, dir, PE_HOSTFILE);
      fp = fopen(hostfilename, "w");
      if (!fp) {
         sprintf(err_str, MSG_FILE_NOOPEN_SS,  hostfilename, strerror(errno));
         environmentList = lFreeList(environmentList);
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
         lList *alp = NULL;
         const char *q_set;
#ifndef ENABLE_213_FIX
         lListElem *qep;
#endif

         slots = (int)lGetUlong(gdil_ep, JG_slots);
#ifdef ENABLE_213_FIX
         q_set = lGetString(gdil_ep, JG_processors);
#else
         qep = lFirst(lGetList(gdil_ep, JG_queue));
         q_set = qep ? lGetString(qep, QU_processors) : NULL;
#endif
         pe_slots += slots;
         fprintf(fp, "%s %d %s %s\n", 
            lGetHost(gdil_ep, JG_qhostname),
            slots, 
            lGetString(gdil_ep, JG_qname), 
            q_set ? q_set : "<NULL>");
         if (!hostcmp(lGetHost(master_q, QU_qhostname), lGetHost(gdil_ep, JG_qhostname))) {
            host_slots += slots;
            if (q_set && strcasecmp(q_set, "UNDEFINED")) {
               parse_ranges(q_set, 0, 0, &alp, &processor_set, INF_ALLOWED);
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
      environmentList = lFreeList(environmentList);
      DEXIT;
      return -2;        /* general */
   }
   
   /* take environment from job in case of tasks */
   if (slave_jep) {
      job_jep = slave_jep;
      job_jatep = slave_jatep;
   }
   else {
      job_jep = jep;
      job_jatep = jatep;
   }

   /* write inherited environment first, to be sure that some task specific variables
   ** will be overridden
   */
   {
      char buffer[2048];
      sprintf(buffer, "%s:/usr/local/bin:/usr/ucb:/bin:/usr/bin:", tmpdir);
      add_or_replace_env(environmentList, "PATH", buffer);
   }

   if (slave_jep) {
      const char *s, *name;
      int n = strlen(COMPLEX2ENV_PREFIX);
      for_each(env, lGetList(slave_jep, JB_env_list)) {
         name = lGetString(env, VA_variable);
         if (!strncmp(name, COMPLEX2ENV_PREFIX, n)) 
            continue; /* we handle them later on in complex2environment() */

         s = lGetString(env, VA_value);
         add_or_replace_env(environmentList, name, s ? s : "");
      }
   }

   {
      const char *s, *name;
      int n = strlen(COMPLEX2ENV_PREFIX);
      for_each(env, lGetList(jep, JB_env_list)) {
         name = lGetString(env, VA_variable);
         if (!strncmp(name, COMPLEX2ENV_PREFIX, n)) 
            continue; /* we handle them later on in complex2environment() */

         s = lGetString(env, VA_value);
         add_or_replace_env(environmentList, name, s ? s : "");
      }
   }
  
   /* in case of pe task: 
   ** jep = pe task
   ** slave_jep = job_jep = job 
   */
   /* write PWD and set, might get overridden by task environment */
   
   /* 1.) try to read cwd from pe task */
   if(slave_jep) {
      cwd = lGetString(jep, JB_cwd);
   } 

   /* 2.) try to read cwd from job */
   if(cwd == NULL) {
      cwd = lGetString(job_jep, JB_cwd);
   }

   /* 3.) if task or job set cwd: do path mapping */
   if(cwd != NULL) {
      static char cwd_out[SGE_PATH_MAX];
     
      /* path aliasing only for cwd flag set */
      get_path_alias(cwd, cwd_out, SGE_PATH_MAX, 
               lGetList(job_jep, JB_path_aliases), me.qualified_hostname, NULL);
      cwd = cwd_out;
      add_or_replace_env(environmentList, "PWD", cwd);
   }
   else 
      cwd = pw->pw_dir;

   if (lGetString(jep, JB_sge_o_home)) {
      if (set_sge_environment) 
         add_or_replace_env(environmentList, "SGE_O_HOME", lGetString(jep, JB_sge_o_home));
      if (set_cod_environment) 
         add_or_replace_env(environmentList, "COD_O_HOME", lGetString(jep, JB_sge_o_home));
      if (set_grd_environment) 
         add_or_replace_env(environmentList, "GRD_O_HOME", lGetString(jep, JB_sge_o_home));
   }   
   if (lGetString(jep, JB_sge_o_log_name)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_LOGNAME", lGetString(jep, JB_sge_o_log_name));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_LOGNAME", lGetString(jep, JB_sge_o_log_name));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_LOGNAME", lGetString(jep, JB_sge_o_log_name));
   }
      
   if (lGetString(jep, JB_sge_o_path)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_PATH", lGetString(jep, JB_sge_o_path));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_PATH", lGetString(jep, JB_sge_o_path));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_PATH", lGetString(jep, JB_sge_o_path));
   }   
   if (lGetString(jep, JB_sge_o_mail)) {
      if (set_sge_environment) 
         add_or_replace_env(environmentList, "SGE_O_MAIL", lGetString(jep, JB_sge_o_mail));
      if (set_cod_environment) 
         add_or_replace_env(environmentList, "COD_O_MAIL", lGetString(jep, JB_sge_o_mail));
      if (set_grd_environment) 
         add_or_replace_env(environmentList, "GRD_O_MAIL", lGetString(jep, JB_sge_o_mail));
   }   
   if (lGetString(jep, JB_sge_o_shell)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_SHELL", lGetString(jep, JB_sge_o_shell));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_SHELL", lGetString(jep, JB_sge_o_shell));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_SHELL", lGetString(jep, JB_sge_o_shell));
   }   
   if (lGetString(jep, JB_sge_o_tz)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_TZ", lGetString(jep, JB_sge_o_tz));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_TZ", lGetString(jep, JB_sge_o_tz));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_TZ", lGetString(jep, JB_sge_o_tz));
   }   
   if (lGetString(jep, JB_sge_o_workdir)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_WORKDIR", lGetString(jep, JB_sge_o_workdir));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_WORKDIR", lGetString(jep, JB_sge_o_workdir));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_WORKDIR", lGetString(jep, JB_sge_o_workdir));
   }   
   if (lGetHost(jep, JB_sge_o_host)) {
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_O_HOST", lGetHost(jep, JB_sge_o_host));
      if (set_cod_environment)
         add_or_replace_env(environmentList, "COD_O_HOST", lGetHost(jep, JB_sge_o_host));
      if (set_grd_environment)
         add_or_replace_env(environmentList, "GRD_O_HOST", lGetHost(jep, JB_sge_o_host));
   }   
   if (lGetString(job_jep, JB_job_name)) {
      add_or_replace_env(environmentList, "REQNAME", lGetString(job_jep, JB_job_name));
   }

   if (set_sge_environment) 
      add_or_replace_env(environmentList, "SGE_CELL", me.default_cell);
   if (set_cod_environment) 
      add_or_replace_env(environmentList, "COD_CELL", me.default_cell);
   if (set_grd_environment) 
      add_or_replace_env(environmentList, "GRD_CELL", me.default_cell);

   add_or_replace_env(environmentList, "HOME", pw->pw_dir);
   add_or_replace_env(environmentList, "SHELL", pw->pw_shell);
   add_or_replace_env(environmentList, "USER", pw->pw_name);
   add_or_replace_env(environmentList, "LOGNAME", pw->pw_name);

   /*
   ** interactive jobs get job name INTERACTIVE
   ** login jobs have LOGIN
   */
   if (lGetString(jep, JB_script_file)) {
      const char *s;

      /* build basename */ 
      s=strrchr(lGetString(jep, JB_script_file), '/');
      if (s) 
         s++;
      else 
         s = lGetString(jep, JB_script_file);

      add_or_replace_env(environmentList, "JOB_NAME", s);
   }
   else {
      u_long32 jb_now = lGetUlong(jep, JB_now);

      if(JB_NOW_IS_QLOGIN(jb_now)) { 
         add_or_replace_env(environmentList, "JOB_NAME", JB_NOW_STR_QLOGIN);
      } else {
         if(JB_NOW_IS_QRSH(jb_now)) {
            add_or_replace_env(environmentList, "JOB_NAME", JB_NOW_STR_QRSH);
         } else {
            if(JB_NOW_IS_QRLOGIN(jb_now)) {
               add_or_replace_env(environmentList, "JOB_NAME", JB_NOW_STR_QRLOGIN);
            } else {   
               add_or_replace_env(environmentList, "JOB_NAME", JB_NOW_STR_QSH); 
            }
         }  
      }      
   }

   add_or_replace_env(environmentList, "REQUEST", lGetString(job_jep, JB_job_name));
   add_or_replace_env(environmentList, "HOSTNAME", lGetHost(master_q, QU_qhostname));
   add_or_replace_env(environmentList, "QUEUE", lGetString(master_q, QU_qname));
   add_or_replace_env_u32(environmentList, "JOB_ID", lGetUlong(job_jep, JB_job_number));
  
   if (set_sge_environment) { 
      if (is_array(job_jep)) {
         u_long32 start, end, step;

         job_get_ja_task_ids(jep, &start, &end, &step);
         add_no_replace_env_u32(environmentList, "SGE_TASK_ID", lGetUlong(jatep, JAT_task_number));
         add_no_replace_env_u32(environmentList, "SGE_TASK_FIRST", start);
         add_no_replace_env_u32(environmentList, "SGE_TASK_LAST", end);
         add_no_replace_env_u32(environmentList, "SGE_TASK_STEPSIZE", step);
      } else {
         add_no_replace_env(environmentList, "SGE_TASK_ID", "undefined");
         add_no_replace_env(environmentList, "SGE_TASK_FIRST", "undefined");
         add_no_replace_env(environmentList, "SGE_TASK_LAST", "undefined");
         add_no_replace_env(environmentList, "SGE_TASK_STEPSIZE", "undefined");
      }
   }
   if (set_cod_environment) { 
      if (is_array(job_jep)) {
         u_long32 start, end, step;

         job_get_ja_task_ids(jep, &start, &end, &step);
         add_or_replace_env_u32(environmentList, "COD_TASK_ID", lGetUlong(jatep, JAT_task_number));
         add_no_replace_env_u32(environmentList, "COD_TASK_FIRST", start);
         add_no_replace_env_u32(environmentList, "COD_TASK_LAST", end);
         add_no_replace_env_u32(environmentList, "COD_TASK_STEPSIZE", step);
      } else {
         add_or_replace_env(environmentList, "COD_TASK_ID", "undefined");
         add_no_replace_env(environmentList, "COD_TASK_FIRST", "undefined");
         add_no_replace_env(environmentList, "COD_TASK_LAST", "undefined");
         add_no_replace_env(environmentList, "COD_TASK_STEPSIZE", "undefined");
      }
   }
   if (set_grd_environment) { 
      if (is_array(job_jep)) {
         u_long32 start, end, step;

         job_get_ja_task_ids(jep, &start, &end, &step);
         add_or_replace_env_u32(environmentList, "GRD_TASK_ID", lGetUlong(jatep, JAT_task_number));
         add_no_replace_env_u32(environmentList, "GRD_TASK_FIRST", start);
         add_no_replace_env_u32(environmentList, "GRD_TASK_LAST", end);
         add_no_replace_env_u32(environmentList, "GRD_TASK_STEPSIZE", step);
      } else {
         add_or_replace_env(environmentList, "GRD_TASK_ID", "undefined");
         add_no_replace_env(environmentList, "GRD_TASK_FIRST", "undefined");
         add_no_replace_env(environmentList, "GRD_TASK_LAST", "undefined");
         add_no_replace_env(environmentList, "GRD_TASK_STEPSIZE", "undefined");
      }
   }

   add_or_replace_env(environmentList, "ENVIRONMENT", "BATCH");
   add_or_replace_env(environmentList, "ARC", arch);

   if (set_sge_environment) 
      add_or_replace_env(environmentList, "SGE_ARCH", arch);
   if (set_cod_environment) 
      add_or_replace_env(environmentList, "COD_ARCH", arch);
   if (set_grd_environment) 
      add_or_replace_env(environmentList, "GRD_ARCH", arch);
  
   if ((cp=getenv("TZ")) && strlen(cp))
      add_or_replace_env(environmentList, "TZ", cp);

   if ((cp=getenv("COMMD_PORT")) && strlen(cp))
      add_or_replace_env(environmentList, "COMMD_PORT", cp);
     
   if (set_sge_environment) 
      add_or_replace_env(environmentList, "SGE_ROOT", path.sge_root);
   if (set_cod_environment) 
      add_or_replace_env(environmentList, "CODINE_ROOT", path.sge_root);
   if (set_grd_environment) 
      add_or_replace_env(environmentList, "GRD_ROOT", path.sge_root);

   add_or_replace_env_int(environmentList, "NQUEUES", 
      lGetNumberOfElem(lGetList(job_jatep, JAT_granted_destin_identifier_list)));
   add_or_replace_env_int(environmentList, "NSLOTS", pe_slots);
   add_or_replace_env_int(environmentList, "NHOSTS", nhosts);

   add_or_replace_env_int(environmentList, "RESTARTED", (int) lGetUlong(job_jatep, JAT_job_restarted));
   add_or_replace_env(environmentList, "FIRST_HOST", 
         lGetHost(job_jep, JB_first_host) ? lGetHost(job_jep, JB_first_host) : "UNKNOWN");
   add_or_replace_env(environmentList, "LAST_HOST", 
         lGetHost(job_jep, JB_last_host) ? lGetHost(job_jep, JB_last_host) : "UNKNOWN");   

   /*
   ** interactive and login jobs have no script file
   */
   if (lGetString(jep, JB_job_source))
      strcpy(script_file, lGetString(jep, JB_script_file));
   else if (lGetString(jep, JB_script_file))
      sprintf(script_file, "%s/%s/" u32, execd_spool_dir, EXEC_DIR, 
              lGetUlong(jep, JB_job_number));
   else { 
      strcpy(script_file, lGetString(lGetElemStr(environmentList, VA_variable, "JOB_NAME"), VA_value));
   }

   add_or_replace_env(environmentList, "JOB_SCRIPT", script_file);

   add_or_replace_env(environmentList, "TMPDIR", tmpdir);
   add_or_replace_env(environmentList, "TMP", tmpdir);

   if (set_sge_environment) {
      add_or_replace_env(environmentList, "SGE_ACCOUNT", (lGetString(job_jep, JB_account) ? 
               lGetString(job_jep, JB_account) : DEFAULT_ACCOUNT));
   }
   if (set_cod_environment) {
      add_or_replace_env(environmentList, "COD_ACCOUNT", (lGetString(job_jep, JB_account) ? 
               lGetString(job_jep, JB_account) : DEFAULT_ACCOUNT));
   }
   if (set_grd_environment) {
      add_or_replace_env(environmentList, "GRD_ACCOUNT", (lGetString(job_jep, JB_account) ? 
               lGetString(job_jep, JB_account) : DEFAULT_ACCOUNT));
   }

   sge_get_path(lGetList(job_jep, JB_shell_list), cwd, 
                lGetString(job_jep, JB_owner),
                lGetString(job_jep, JB_job_name), 
                lGetUlong(job_jep, JB_job_number), 
                is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_SHELL, shell_path);

   if (!shell_path[0])
      strcpy(shell_path, lGetString(master_q, QU_shell));
   add_or_replace_env(environmentList, "SHELL", shell_path);

   /* forward name of pe to job */
   if (lGetString(job_jatep, JAT_granted_pe)) {
      char buffer[SGE_PATH_MAX];
      add_or_replace_env(environmentList, "PE", lGetString(job_jatep, JAT_granted_pe));
      sprintf(buffer, "%s/%s/%s", execd_spool_dir, dir, PE_HOSTFILE);
      add_or_replace_env(environmentList, "PE_HOSTFILE", buffer);
   }   

   /* forward name of ckpt env to job */
   if ((ep = lFirst(lGetList(job_jep, JB_checkpoint_object_list)))) {
      if (set_sge_environment) {
         add_or_replace_env(environmentList, "SGE_CKPT_ENV", lGetString(ep, CK_name));
         if (lGetString(ep, CK_ckpt_dir))
            add_or_replace_env(environmentList, "SGE_CKPT_DIR", lGetString(ep, CK_ckpt_dir));
      }
      if (set_cod_environment) {
         add_or_replace_env(environmentList, "COD_CKPT_ENV", lGetString(ep, CK_name));
         if (lGetString(ep, CK_ckpt_dir))
            add_or_replace_env(environmentList, "COD_CKPT_DIR", lGetString(ep, CK_ckpt_dir));
      }
      if (set_grd_environment) {
         add_or_replace_env(environmentList, "GRD_CKPT_ENV", lGetString(ep, CK_name));
         if (lGetString(ep, CK_ckpt_dir))
            add_or_replace_env(environmentList, "GRD_CKPT_DIR", lGetString(ep, CK_ckpt_dir));
      }
   }

   {
      char buffer[SGE_PATH_MAX]; 

      sprintf(buffer, "%s/%s", execd_spool_dir, dir);  
      if (set_sge_environment)
         add_or_replace_env(environmentList, "SGE_JOB_SPOOL_DIR", buffer);
      if (set_cod_environment) 
         add_or_replace_env(environmentList, "COD_JOB_SPOOL_DIR", buffer);
      if (set_grd_environment) 
         add_or_replace_env(environmentList, "GRD_JOB_SPOOL_DIR", buffer);
   }

   if (complex2environment(environmentList, cplx, job_jep, err_str)) {
      environmentList = lFreeList(environmentList);
      fclose(fp);
      DEXIT;
      return -2;
   } 

   set_sharedlib_path(environmentList);

   dump_env(environmentList, fp);
   fclose(fp);  
   /*************************** finished writing environment *****************/

   /**************** write out config file ******************************/
   sprintf(fname, "%s/config", dir);
   fp = fopen(fname, "w");
   if (!fp) {
      environmentList = lFreeList(environmentList);
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
         environmentList = lFreeList(environmentList);
         sprintf(err_str, MSG_EXECD_NOSGID); 
         fclose(fp);
         DEXIT;
         return(-2);
      }

#     endif
      
      /* parse range add create list */
      DPRINTF(("gid_range = %s\n", conf.gid_range));
      if (!(rlp=parse_ranges(conf.gid_range, 0, 0, &alp, NULL, 
            INF_NOT_ALLOWED))) {
          lFreeList(alp);
          sprintf(err_str, MSG_EXECD_NOPARSEGIDRANGE);
          environmentList = lFreeList(environmentList);
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
            environmentList = lFreeList(environmentList);
            fclose(fp);
            DEXIT;
            return (-1);
         }
      }

      /* write add_grp_id to job-structure and file */
      sprintf(str_id, "%ld", (long) last_addgrpid);
      fprintf(fp, "add_grp_id="gid_t_fmt"\n", last_addgrpid);
      lSetString(jatep, JAT_osjobid, str_id);
      
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
                is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_STDOUT, stdout_path);
   sge_get_path(lGetList(jep, JB_stderr_path_list), cwd,
                lGetString(jep, JB_owner), 
                lGetString(jep, JB_job_name),
                lGetUlong(jep, JB_job_number), 
                is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                SGE_STDERR, stderr_path);

   fprintf(fp, "stdout_path=%s\n", stdout_path);
   fprintf(fp, "stderr_path=%s\n", stderr_path);
   fprintf(fp, "merge_stderr=%d\n", (int)lGetUlong(jep, JB_merge_stderr));

   if (lGetUlong(jep, JB_checkpoint_attr) && 
       (ep = lFirst(lGetList(jep, JB_checkpoint_object_list)))) {
      fprintf(fp, "ckpt_job=1\n");
      fprintf(fp, "ckpt_restarted=%d\n", slave_jep ? 0 : (int) lGetUlong(job_jatep, JAT_job_restarted));
      fprintf(fp, "ckpt_pid=%d\n", (int) lGetUlong(job_jatep, JAT_pvm_ckpt_pid));
      fprintf(fp, "ckpt_osjobid=%s\n", lGetString(job_jatep, JAT_osjobid) ? lGetString(job_jatep, JAT_osjobid): "0");
      fprintf(fp, "ckpt_interface=%s\n", lGetString(ep, CK_interface));
      fprintf(fp, "ckpt_command=%s\n", lGetString(ep, CK_ckpt_command));
      fprintf(fp, "ckpt_migr_command=%s\n", lGetString(ep, CK_migr_command));
      fprintf(fp, "ckpt_rest_command=%s\n", lGetString(ep, CK_rest_command));
      fprintf(fp, "ckpt_clean_command=%s\n", lGetString(ep, CK_clean_command));
      fprintf(fp, "ckpt_dir=%s\n", lGetString(ep, CK_ckpt_dir));
      fprintf(fp, "ckpt_signal=%s\n", lGetString(ep, CK_signal));
 
      if (!(lGetUlong(job_jep, JB_checkpoint_attr) & CHECKPOINT_AT_MINIMUM_INTERVAL))
         interval = 0;
      else {   
         parse_ulong_val(NULL, &interval, TYPE_TIM, lGetString(master_q, QU_min_cpu_interval), NULL, 0);   
         interval = MAX(interval, lGetUlong(job_jep, JB_checkpoint_interval));
      }   
      fprintf(fp, "ckpt_interval=%d\n", (int) interval);
   }
   else 
      fprintf(fp, "ckpt_job=0\n");
      
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
   if (lGetHost(job_jep, JB_sge_o_host))
      fprintf(fp, "spi_initiator=%s\n", lGetHost(job_jep, JB_sge_o_host));
   else
      fprintf(fp, "spi_initiator=%s\n", "unknown");
#endif

   /* do not start prolog/epilog in case of pe tasks */
   fprintf(fp, "prolog=%s\n", lGetString(jep, JB_job_source) ? "none":
       ((cp=lGetString(master_q, QU_prolog)) && strcasecmp(cp, "none"))?
         cp: conf.prolog);
   fprintf(fp, "epilog=%s\n", lGetString(jep, JB_job_source) ? "none":
       ((cp=lGetString(master_q, QU_epilog)) && strcasecmp(cp, "none"))?
         cp: conf.epilog);

   fprintf(fp, "starter_method=%s\n", (cp=lGetString(master_q, QU_starter_method))? cp : "none");
   fprintf(fp, "suspend_method=%s\n", (cp=lGetString(master_q, QU_suspend_method))? cp : "none");
   fprintf(fp, "resume_method=%s\n", (cp=lGetString(master_q, QU_resume_method))? cp : "none");
   fprintf(fp, "terminate_method=%s\n", (cp=lGetString(master_q, QU_terminate_method))? cp : "none");

   fprintf(fp, "script_timeout=120\n");

   fprintf(fp, "pe=%s\n", lGetString(jatep, JAT_granted_pe)?lGetString(jatep, JAT_granted_pe):"none");
   fprintf(fp, "pe_slots=%d\n", pe_slots);
   fprintf(fp, "host_slots=%d\n", host_slots);

   if (lGetString(jatep, JAT_granted_pe)) {
      lListElem *pep;
      pep = lFirst(lGetList(jep, JB_pe_object));
      
      fprintf(fp, "pe_hostfile=%s/%s/%s\n", execd_spool_dir, dir, PE_HOSTFILE);
      fprintf(fp, "pe_start=%s\n",  lGetString(pep, PE_start_proc_args)?
                                       lGetString(pep, PE_start_proc_args):"none");
      fprintf(fp, "pe_stop=%s\n",   lGetString(pep, PE_stop_proc_args)?
                                       lGetString(pep, PE_stop_proc_args):"none");

      /* build path for stdout of pe scripts */
      sge_get_path(lGetList(jep, JB_stdout_path_list), cwd, 
                   lGetString(jep, JB_owner), 
                   lGetString(jep, JB_job_name), 
                   lGetUlong(jep, JB_job_number), 
                   is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
                   SGE_PAR_STDOUT, pe_stdout_path);
      fprintf(fp, "pe_stdout_path=%s\n", pe_stdout_path);

      /* build path for stderr of pe scripts */
      sge_get_path(lGetList(jep, JB_stderr_path_list), cwd,
                   lGetString(jep, JB_owner), 
                   lGetString(jep, JB_job_name), 
                   lGetUlong(jep, JB_job_number), 
                   is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0,
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
   fprintf(fp, "processors=");
   unparse_ranges(fp, NULL, 0, processor_set);
   fprintf(fp, "\n");
   fprintf(fp, "job_name=%s\n", lGetString(jep, JB_job_name));
   fprintf(fp, "job_id="u32"\n", lGetUlong(jep, JB_job_number));
   fprintf(fp, "ja_task_id="u32"\n", is_array(jep) ? lGetUlong(jatep, JAT_task_number) : 0);
   fprintf(fp, "account=%s\n", (lGetString(job_jep, JB_account) ? lGetString(job_jep, JB_account) : DEFAULT_ACCOUNT));
   fprintf(fp, "submission_time=" u32 "\n", lGetUlong(job_jep, JB_submission_time));

   {
      u_long32 notify = 0;
      if (lGetUlong(jep, JB_notify))
         parse_ulong_val(NULL, &notify, TYPE_TIM, lGetString(master_q, QU_notify), NULL, 0);
      fprintf(fp, "notify=" u32 "\n", notify);
   }
   
   /*
   ** interactive jobs have no exec file
   */
   if (!lGetString(jep, JB_script_file)) {
      u_long32 jb_now = lGetUlong(jep, JB_now);
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
            environmentList = lFreeList(environmentList);
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
      lListElem *se;
      int nargs=1;
      fprintf(fp, "njob_args=%d\n", lGetNumberOfElem(lGetList(jep, JB_job_args)));
      for_each(se, lGetList(jep, JB_job_args)) {
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
      u_long32 jb_now = lGetUlong(jep, JB_now);    /* detect qsh case  */
      
      env = lGetElemStr(lGetList(jep, JB_env_list), VA_variable, "DISPLAY");

      if (!env) {                                  /* no DISPLAY set   */
         if(JB_NOW_IS_QSH(jb_now)) {               /* and qsh -> error */
            sprintf(err_str, MSG_EXECD_NODISPLAY);
            environmentList = lFreeList(environmentList);
            fclose(fp);
            DEXIT;
            return -1;
         }
      } else {
         if (!lGetString(env, VA_value) || !strcmp(lGetString(env, VA_value), "")) {  /* no value for DISPLAY */
            if(JB_NOW_IS_QSH(jb_now)) {                                               /* and qsh -> error     */
               sprintf(err_str, MSG_EXECD_EMPTYDISPLAY);
               environmentList = lFreeList(environmentList);
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
      environmentList = lFreeList(environmentList);
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
      u_long32 jb_now = lGetUlong(jep, JB_now); 
      const char *qrsh_task_id = lGetString(jep, JB_pe_task_id_str);

      if (qrsh_task_id != NULL) {
         fprintf(fp, "qrsh_task_id=%s\n", qrsh_task_id);
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

               if(qrsh_task_id != NULL) {
                  fprintf(fp, "qrsh_pid_file=%s/pid.%s\n", tmpdir, qrsh_task_id);
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

   environmentList = lFreeList(environmentList);
   fclose(fp);
   /********************** finished writing config ************************/

   /* test whether we can access scriptfile */
   /*
   ** interactive jobs dont need to access script file
   */
   if (!lGetString(jep, JB_job_source) && lGetString(jep, JB_script_file)) {
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
   mail_users = lGetList(jep, JB_mail_list);
   mail_options = lGetUlong(jep, JB_mail_options);
   strcpy(sge_mail_start, sge_ctime(lGetUlong(jatep, JAT_start_time)));
   if (VALID(MAIL_AT_BEGINNING, mail_options)) {
      if (is_array(jep)) {
         /* CR: don't localize mail subject, until we send it in Mime format!
          *     The message definition is not l10n'ed (no _() macro used)!!!        
          */
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
         /* CR: don't localize mail subject, until we send it in Mime format!
          *     The message definition is not l10n'ed (no _() macro used)!!!        
          */
         sprintf(sge_mail_subj, MSG_MAIL_STARTSUBJECT_US, u32c(lGetUlong(jep, JB_job_number)),
            lGetString(jep, JB_job_name));
         sprintf(sge_mail_body, MSG_MAIL_STARTBODY_USSSSS,
             u32c(lGetUlong(jep, JB_job_number)),
             lGetString(jep, JB_job_name),
             lGetString(jep, JB_owner),
             lGetString(master_q, QU_qname),
             lGetHost(master_q, QU_qhostname), sge_mail_start);
      }
      cull_mail(mail_users, sge_mail_subj, sge_mail_body, MSG_MAIL_TYPE_START);
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

      lSetUlong(jep, JB_hard_wallclock_gmt, 0); /* in case we are restarting! */
      lSetUlong(jatep, JAT_pending_signal, 0);
      lSetUlong(jatep, JAT_pending_signal_delivery_time, 0);
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



static int complex2environment(
lList *envl,
lList *cplx,
lListElem *job,
char *err_str 
) {
   const char *s, *name;
   lListElem *env, *attr;
   int n;


   DENTER(TOP_LAYER, "complex2environment");

   n = strlen(COMPLEX2ENV_PREFIX);

   for_each(env, lGetList(job, JB_env_list)) {
      name = lGetString(env, VA_variable);
      if (strncmp(name, COMPLEX2ENV_PREFIX, n)) 
         continue;         

      if ((attr=lGetElemStr(cplx, CE_name, &name[n]))
            && (s=lGetString(attr, CE_stringval)))
         add_or_replace_env(envl, name, s);
      else
         add_or_replace_env(envl, name, "");
   }

   DEXIT;
   return 0;
}
