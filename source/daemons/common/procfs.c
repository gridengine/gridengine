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
#if !defined(COMPILE_DC)

int verydummyprocfs;

#else

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/signal.h>

#if !defined(CRAY) && !defined(NECSX4) && !defined(NECSX5)
#include <sys/syscall.h>
#endif

#include <unistd.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(ALPHA) 
#  include <sys/user.h>
#  include <sys/table.h>   
#  include <sys/procfs.h>   
#endif

#if defined(SOLARIS) 
#  include <sys/procfs.h>   
#endif

#if defined(LINUX)
#include <sys/param.h>          /* for HZ (jiffies -> seconds ) */
#endif

#include "sge_log.h"
#include "msg_sge.h"
#include "sge_log.h"
#include "sgermon.h"
#include "basis_types.h"
#include "sgedefs.h"
#include "exec_ifm.h"
#include "pdc.h"
#include "sge_unistd.h"

#if !defined(CRAY)
#include "procfs.h"
#endif

#if defined(LINUX) || defined(ALPHA) || defined(SOLARIS)
#   if defined(SVR3)
#      define PROC_DIR "/debug"
#   else
#      define PROC_DIR "/proc"
#   endif
#endif

#if defined(LINUX)

#define BIGLINE 1024

typedef struct _tLinProcStat {     /* process Linux /proc/pid/stat structure */
   int   pr_pid;                  /*  1 process id */
   char  pr_cmd[16];              /*  2 command name */
   char  pr_stat;                 /*  3 process status */
   int   pr_ppid;                 /*  4 parent process id */
   int   pr_pgrp;                 /*  5 process group */
   int   pr_sid;                  /*  6 session id */
   int   pr_tty;                  /*  7 tty line MAJOR << ? + MINOR */
   int   pr_tty_pgrp;             /*  8 tty process group */
   long  pr_flags;                /*  9 process flags PF_* in <linux/sched.h> */
   long  pr_min_flt;              /* 10 minor page faults */
   long  pr_cmin_flt;             /* 11 children minor page faults */
   long  pr_maj_flt;              /* 12 major page faults */
   long  pr_cmaj_flt;             /* 13 children major page faults */
   long  pr_utime;                /* 14 user time */
   long  pr_stime;                /* 15 system time */
   long  pr_cutime;               /* 16 children user time */
   long  pr_cstime;               /* 17 children system time */
   long  pr_counter;		        /* 18 jiffies */
   long  pr_pri;                  /* 19 priority (nice) */
   long  pr_tmout;                /* 20 Timeout time for scheduling */
   long  pr_it_real_value;        /* 21 itimer real value */
   long  pr_start;                /* 22 start of execution in jiffies since boot*/
   long  pr_vsize;                /* 23 total size t + d + s NOT pages */
   long  pr_rss;                  /* 24 resident set size pages */
   long  pr_rlim_cur;             /* 25 current rlimit ro rss */
   long  pr_start_code;           /* 26 start of code */
   long  pr_end_code;             /* 27 end of code */
   long  pr_start_stack;          /* 28 start of stack */
   long  pr_esp;                  /* 29 head of stack (stack pointer) */
   long  pr_eip;                  /* 30 instruction pointer */
   long  pr_signal;               /* 31 pending signals mask */
   long  pr_blocked;              /* 32 blocked signals mask */
   long  pr_sigignore;            /* 33 ignored signals mask */
   long  pr_sigcatch;             /* 34 catched signals mask */
   long  pr_wchan;                /* 35 WCHAN (seems to be a return address) */
} tLinProcStat;
#endif


/*-----------------------------------------------------------------------*/
#if defined(LINUX) || defined(ALPHA) || defined(SOLARIS)

static DIR *cwd;
static struct dirent *dent;

#if defined(LINUX)

int groups_in_proc (void) {
   char procnam[256];
   char buf[1024];
   FILE* fd = (FILE*) NULL;
   
   sprintf(procnam, "%s/1/status", PROC_DIR);
   if (!(fd = fopen(procnam, "r"))) {
      return (0);
   }
   while (fgets(buf, sizeof(buf), fd)) {
      if (strcmp("Groups:", strtok(buf, "\t"))==0) {
         fclose(fd);
         return (1);
      }
   }
   fclose(fd);
   return (0);
}

#endif

#if defined(LINUX) || defined(SOLARIS) || defined(ALPHA)
void procfs_kill_addgrpid(gid_t add_grp_id, int sig,
   tShepherd_trace shepherd_trace)
{
   char procnam[128];
   int i;
   int groups=0;
   u_long32 max_groups;
   gid_t *list;
#if defined(SOLARIS) || defined(ALPHA)
   int fd;
   prcred_t proc_cred;
#elif defined(LINUX)
   FILE *fp;
   char buffer[1024];
   uid_t uids[4];
   gid_t gids[4];
#endif

   /* quick return in case of invalid add. group id */
   if (add_grp_id == 0)
      return;

   max_groups = sge_sysconf(SGE_SYSCONF_NGROUPS_MAX);
   if (max_groups <= 0)
      if (shepherd_trace) {
         char err_str[256];

         sprintf(err_str, MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY );
         shepherd_trace(err_str);
      }
/*
 * INSURE detects a WRITE_OVERFLOW when getgroups was invoked (LINUX).
 * Is this a bug in the kernel or in INSURE?
 */
#if defined(LINUX)
   list = (gid_t*) malloc(2*max_groups*sizeof(gid_t));
#else
   list = (gid_t*) malloc(max_groups*sizeof(gid_t));
#endif
   if (list == NULL)
      if (shepherd_trace) {
         char err_str[256];

         sprintf(err_str, MSG_SGE_PROCFSKILLADDGRPIDMALLOCFAILED );
         shepherd_trace(err_str);
      }

   pt_open();

   /* find next valid entry in procfs  */
   while ((dent = readdir(cwd))) {
      if (!dent->d_name)
         continue;
      if (!dent->d_name[0])
         continue;

      if (!strcmp(dent->d_name, "..") || !strcmp(dent->d_name, "."))
         continue;

      if (atoi(dent->d_name) == 0)
         continue;

#if defined(SOLARIS) || defined(ALPHA)
      sprintf(procnam, "%s/%s", PROC_DIR, dent->d_name);
      if ((fd = open(procnam, O_RDONLY, 0)) == -1) {
         DPRINTF(("open(%s) failed: %s\n", procnam, strerror(errno)));
         continue;
      }
#elif defined(LINUX)
      if (!strcmp(dent->d_name, "self"))
         continue;

      sprintf(procnam, "%s/%s/status", PROC_DIR, dent->d_name);
      if (!(fp = fopen(procnam, "r")))
         continue;
#endif

#if defined(SOLARIS) || defined(ALPHA)
      /* get number of groups */
      if (ioctl(fd, PIOCCRED, &proc_cred) == -1) {
         close(fd);
         continue;
      }

      /* get list of supplementary groups */
      groups = proc_cred.pr_ngroups;
      if (ioctl(fd, PIOCGROUPS, list) == -1) {
         close(fd);
         continue;
      }

#elif defined(LINUX)
      /* get number of groups and current uids, gids
       * uids[0], gids[0] => UID and GID
       * uids[1], gids[1] => EUID and EGID
       * uids[2], gids[2] => SUID and SGID
       * uids[3], gids[3] => FSUID and FSGID
       */
      groups = 0;
      while (fgets(buffer, sizeof(buffer), fp)) {
         char *label = NULL;
         char *token = NULL;

         label = strtok(buffer, " \t\n");
         if (label) {
            if (!strcmp("Groups:", label)) {
               while ((token = strtok((char*) NULL, " \t\n"))) {
                  list[groups]=(gid_t) atol(token);
                  groups++;
               }
            } else if (!strcmp("Uid:", label)) {
               int i = 0;

               while ((i < 4) && (token = strtok((char*) NULL, " \t\n"))) {
                  uids[i]=(uid_t) atol(token);
                  i++;
               }
            } else if (!strcmp("Gid:", label)) {
               int i = 0;

               while ((i < 4) && (token = strtok((char*) NULL, " \t\n"))) {
                  gids[i]=(gid_t) atol(token);
                  i++;
               }
            }
         }
      }
#endif

#if defined(SOLARIS) || defined(ALPHA)
      close(fd);
#elif defined(LINUX)
      fclose(fp);
#endif

      /* send each process a signal which belongs to add_grg_id */
      for (i = 0; i < groups; i++) {
         if (list[i] == add_grp_id) {
            pid_t pid;
            pid = (pid_t) atol(dent->d_name);

#if defined(LINUX)
            /* if UID, GID, EUID and EGID == 0
             *  don't kill the process!!! - it could be the rpc.nfs-deamon
             */
            if (!(uids[0] == 0 && gids[0] == 0 &&
                  uids[1] == 0 && gids[1] == 0)) {
#elif defined(SOLARIS) || defined(ALPHA)
            if (!(proc_cred.pr_ruid == 0 && proc_cred.pr_rgid == 0 &&
                  proc_cred.pr_euid == 0 && proc_cred.pr_egid == 0)) {
#endif

               if (shepherd_trace) {
                  char err_str[256];

                  sprintf(err_str, MSG_SGE_KILLINGPIDXY_UI , u32c(pid), groups);
                  shepherd_trace(err_str);
               }

#if 1
               kill(pid, sig);
#endif

#if defined(LINUX) || defined(SOLARIS) || defined(ALPHA)
            } else {
               if (shepherd_trace) {
                  char err_str[256];

                  sprintf(err_str, MSG_SGE_DONOTKILLROOTPROCESSXY_UI ,
                     u32c(atol(dent->d_name)), groups);
                  shepherd_trace(err_str);
               }
            }
#endif

            break;
         }
      }
   }
   pt_close();
   free(list);
}
#endif

int pt_open(void)
{
   cwd = opendir(PROC_DIR);
   return !cwd;
}
void pt_close(void)
{
   closedir(cwd);
}

int pt_dispatch_proc_to_job(
lnk_link_t *job_list,
int time_stamp 
) {
   char procnam[128];
   int fd = -1;
#if defined(LINUX)
   char buffer[BIGLINE];
   tLinProcStat pr;
   SGE_STRUCT_STAT fst;
#else
   prstatus_t pr;
   prpsinfo_t pri;
#endif

#if defined(SOLARIS) || defined(ALPHA)   
   prcred_t proc_cred;
#endif

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX) 
   int ret;
   u_long32 max_groups;
   gid_t *list;
   int groups=0;
#endif                        

   proc_elem_t *proc_elem = NULL;
   job_elem_t *job_elem = NULL;
   lnk_link_t *curr;
   double old_time = 0;
   uint64 old_vmem = 0;

   DENTER(TOP_LAYER, "pt_dispatch_proc_to_job");

#if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
   max_groups = sge_sysconf(SGE_SYSCONF_NGROUPS_MAX);
   if (max_groups <= 0) {
      ERROR((SGE_EVENT, MSG_SGE_NGROUPS_MAXOSRECONFIGURATIONNECESSARY));
      DEXIT;
      return 1;  
   }   

   list = (gid_t*) malloc(max_groups*sizeof(gid_t));
   if (list == NULL) {
      ERROR((SGE_EVENT, MSG_SGE_PTDISPATCHPROCTOJOBMALLOCFAILED));
      DEXIT;
      return 1;
   }
#endif

   /* find next valid entry in procfs */ 
   while ((dent = readdir(cwd))) {

      if (!dent->d_name)
         continue;
      if (!dent->d_name[0])
         continue;

      if (!strcmp(dent->d_name, "..") || !strcmp(dent->d_name, "."))
         continue;

      if (atoi(dent->d_name) == 0)
         continue;

#if defined(LINUX)
      sprintf(procnam, "%s/%s/stat", PROC_DIR, dent->d_name);
#else
      sprintf(procnam, "%s/%s", PROC_DIR, dent->d_name);
#endif
      if ((fd = open(procnam, O_RDONLY, 0)) == -1)
         continue;

      /** 
       ** get a list of supplementary group ids to decide
       ** whether this process will be needed;
       ** read also prstatus
       **/

#  if defined(LINUX)

      /* 
       * Read the line and append a 0-Byte 
       */
      if ((ret = read(fd, buffer, BIGLINE-1))>= BIGLINE-1) {
         close(fd);
         continue;
      }
      buffer[BIGLINE-1] = '\0';
      
      if (SGE_FSTAT(fd, &fst)) {
         close(fd);
         continue;
      }

      /* 
       * get prstatus
       */
      ret = sscanf(buffer, 
                   "%d %s %c %d %d %d %d %d %lu %lu \
                    %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu \
                    %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu \
                    %lu %lu %lu %lu %lu",
                    &pr.pr_pid,
                    pr.pr_cmd, 
                    &pr.pr_stat,
                    &pr.pr_ppid,
                    &pr.pr_pgrp,
                    &pr.pr_sid,
                    &pr.pr_tty,
                    &pr.pr_tty_pgrp,
                    &pr.pr_flags,   
                    &pr.pr_min_flt, 
                    &pr.pr_cmin_flt,
                    &pr.pr_maj_flt, 
                    &pr.pr_cmaj_flt,
                    &pr.pr_utime,   
                    &pr.pr_stime,   
                    &pr.pr_cutime,  
                    &pr.pr_cstime,
                    &pr.pr_counter,  
                    &pr.pr_pri,     
                    &pr.pr_tmout,   
                    &pr.pr_it_real_value,
                    &pr.pr_start,
                    &pr.pr_vsize,
                    &pr.pr_rss,  
                    &pr.pr_rlim_cur,
                    &pr.pr_start_code,
                    &pr.pr_end_code,  
                    &pr.pr_start_stack,
                    &pr.pr_esp,
                    &pr.pr_eip,
                    &pr.pr_signal,
                    &pr.pr_blocked,
                    &pr.pr_sigignore,
                    &pr.pr_sigcatch, 
                    &pr.pr_wchan);   

      if (ret != 35) {
         close(fd);
         continue;
      }

      /* 
       * get number of groups; 
       * get list of supplementary groups 
       */
      {
         char procnam[256];
         char buf[1024];
         FILE* fd = (FILE*) NULL;
   
         sprintf(procnam,  "%s/%s/status", PROC_DIR, dent->d_name);
         if (!(fd = fopen(procnam, "r"))) {
            continue;
         }
         groups = 0;
         while (fgets(buf, sizeof(buf), fd)) {
            if (strcmp("Groups:", strtok(buf, "\t"))==0) {
               char *token;
                  
               while ((token=strtok((char*) NULL, " "))) {
                  list[groups]=atol(token);
                  groups++;
               }
               break;
            }
         }
         fclose(fd);
      } 
#  elif defined(SOLARIS) || defined(ALPHA)
      
      /* 
       * get prstatus 
       */
      if (ioctl(fd, PIOCSTATUS, &pr)==-1) {
         close(fd);
         continue;
      }
                                    
      /* 
       * get number of groups 
       */
      ret=ioctl(fd, PIOCCRED, &proc_cred);
      if (ret < 0) {
         close(fd);
         continue;
      }
      
      /* 
       * get list of supplementary groups 
       */
      groups = proc_cred.pr_ngroups;
      ret=ioctl(fd, PIOCGROUPS, list);
      if (ret<0) {
         close(fd);
         continue;
      }

#  else

      /* 
       * get prstatus to decide whether this process will be needed 
       */
      if (ioctl(fd, PIOCSTATUS, &pr)==-1) {
         close(fd);
         continue;
      }

#  endif

#  if defined(SOLARIS) || defined(ALPHA) || defined(LINUX)
      /* 
       * try to find a matching job 
       */
      for (curr=job_list->next; curr != job_list; curr=curr->next) {
         int found_it = 0;
         int group;
         
         job_elem = LNK_DATA(curr, job_elem_t, link);

         for (group=0; !found_it && group<groups; group++) {
            if (job_elem->job.jd_jid == list[group]) {
               found_it = 1;
            }
         }
         if (found_it)
            break;
      }
   
      if (curr == job_list) { /* this is not a traced process */ 
         close(fd);
         continue;
      }
      break;
   } /* while */

   free(list);

   if (!dent) {/* visited all files in procfs */
      DEXIT;
      return 1;
   }
   /* 
    * try to find process in this jobs' proc list 
    */
   for (curr=job_elem->procs.next; curr != &job_elem->procs; 
            curr=curr->next) {
      int found_it = 0;
      int group = 0;
      
      proc_elem = LNK_DATA(curr, proc_elem_t, link);
      
      for (group=0; !found_it && group<groups; group++) {
         if (proc_elem->proc.pd_pid == pr.pr_pid) {
            found_it = 1;
         }
      }
      if (found_it) {
/*          DPRINTF(("Found process with pid %ld\n", (long) pr.pr_pid)); */
         break;
      }
   }

#  else

      /* 
       * try to find matching job 
       */
      for (curr=job_list->next; curr != job_list; curr=curr->next) {  
         job_elem = LNK_DATA(curr, job_elem_t, link);
         if (job_elem->job.jd_jid == pr.pr_sid) {

            printf("sid "pid_t_fmt" pid "pid_t_fmt"\n", pr.pr_sid, pr.pr_pid);
            break; /* found it */
         }
      }
   
      if (curr == job_list) { /* this is not a traced process */ 
         close(fd);
         continue;
      }
      break;
   } /* while */

   if (!dent) { /* visited all files in procfs */
      DEXIT; 
      return 1;
   }
   /* try to find process in this jobs' proc list */
   for (curr=job_elem->procs.next; curr != &job_elem->procs; 
            curr=curr->next) {  
      proc_elem = LNK_DATA(curr, proc_elem_t, link);
      if (  proc_elem->proc.pd_pid == pr.pr_pid )
         break; /* found it */
      else
         printf("proclist "pid_t_fmt" pid "pid_t_fmt"\n", proc_elem->proc.pd_pid, 
            pr.pr_pid);
   }

#  endif

   if (curr == &job_elem->procs) { 
      /* new process, add a proc element into jobs proc list */
      if (!(proc_elem=(proc_elem_t *)malloc(sizeof(proc_elem_t)))) {
         if (fd >= 0)
            close(fd);
         DEXIT;
         return 0;
      }
      memset(proc_elem, 0, sizeof(proc_elem_t));
      proc_elem->proc.pd_length = sizeof(psProc_t);
      proc_elem->proc.pd_state  = 1; /* active */
      LNK_ADD(job_elem->procs.prev, &proc_elem->link);
      job_elem->job.jd_proccount++;
   } else {
      /* save previous usage data - needed to build delta usage */
      old_time = proc_elem->proc.pd_utime + proc_elem->proc.pd_stime;
      old_vmem  = proc_elem->vmem;
   }

   proc_elem->proc.pd_tstamp = time_stamp;

   proc_elem->proc.pd_pid    = pr.pr_pid;
#if defined(LINUX)
   proc_elem->proc.pd_utime  = ((double)pr.pr_utime)/HZ;
   proc_elem->proc.pd_stime  = ((double)pr.pr_stime)/HZ;
   /* could retrieve uid/gid using stat() on stat file */
   proc_elem->vmem           = pr.pr_vsize;
#else
   proc_elem->proc.pd_utime  = pr.pr_utime.tv_sec + pr.pr_utime.tv_nsec*1E-9;
   proc_elem->proc.pd_stime  = pr.pr_stime.tv_sec + pr.pr_stime.tv_nsec*1E-9;
    
   /* Don't care if this part fails */
   if (ioctl(fd, PIOCPSINFO, &pri) != -1) {
      proc_elem->proc.pd_uid    = pri.pr_uid;
      proc_elem->proc.pd_gid    = pri.pr_gid;
      proc_elem->vmem           = pri.pr_size * pagesize;
      proc_elem->rss            = pri.pr_rssize * pagesize;
      proc_elem->proc.pd_pstart = pri.pr_start.tv_sec + pri.pr_start.tv_nsec*1E-9;
   }
#endif         

   proc_elem->mem = 
         ((proc_elem->proc.pd_stime + proc_elem->proc.pd_utime) - old_time) * 
         (( old_vmem + proc_elem->vmem)/2);

#if defined(ALPHA)
#define BLOCKSIZE 512
   {
      struct user ua;
      uint64 old_ru_ioblock = proc_elem->ru_ioblock;

      /* need to do a table(2) call for each process to retrieve io usage data */   
      /* get user area stuff */
      if (table(TBL_UAREA, proc_elem->proc.pd_pid, (char *)&ua, 1, sizeof ua) == 1) {
         proc_elem->ru_ioblock = (uint64)(ua.u_ru.ru_inblock + ua.u_ru.ru_oublock);
         proc_elem->delta_chars = (proc_elem->ru_ioblock - old_ru_ioblock)* BLOCKSIZE;
      }
   }
#endif


   close(fd);
   DEXIT;
   return 0;
}
#endif

#endif /* (!COMPILE_DC) */
