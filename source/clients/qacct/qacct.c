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
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <sys/types.h>
#include <time.h>

#include "sge.h"
#include "def.h"
#include "path_history.h"
#include "sge_all_listsL.h"
#include "sge_string.h"
#include "sge_getme.h"
#include "sge_exit.h"
#include "setup_path.h"
#include "sge_gdi.h"
#include "sge_gdi_intern.h"
#include "sge_resource.h"
#include "complex_history.h"
#include "sge_sched.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "execution_states.h"
#include "sge_parse_date_time.h"
#include "sge_feature.h"
#include "sge_rusage.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "utility.h"
#include "qm_name.h"
#include "basis_types.h"
#include "sge_time.h"
#include "parse_range.h"
#include "sge_stat.h" 
#include "msg_history.h"
#include "msg_clients_common.h"
#include "sge_parse_num_par.h"

typedef struct {
   int host;
   int queue;
   int group;
   int owner;
   int project;
   int department;
   int granted_pe;
   int slots;
} sge_qacct_columns;

static void show_the_way(FILE *fp);
static void print_full(int length, const char* string);
static void print_full_ulong(int length, u_long32 value); 
static void calc_column_sizes(lListElem* ep, sge_qacct_columns* column_size_data );
static void showjob(sge_rusage_type *dusage);
static void get_qacct_lists(lList **ppcomplex, lList **ppqeues, lList **ppexechosts);

/*
** statics
*/
static FILE *fp = NULL;

/*
** REMOVE THIS BY SPLITTING complex_history.c to resolve link dependencies
*/
lList *Master_Complex_List;

int main(int argc, char *argv[]);

/*
** NAME
**   main
** PARAMETER
**
** RETURN
**    0     - ok
**   -1     - invalid command line
**   < -1   - errors
** EXTERNAL
**   fp
**   path
**   me
**   prognames
** DESCRIPTION
**   main routine for qacct SGE client
*/
int main(
int argc,
char **argv 
) {
   /*
   ** problem: some real dynamic allocation
   ** would help save stack here and reduce
   ** problems with long input
   ** but host[0] aso is referenced
   */
   stringT group;
   stringT queue;
   stringT owner;
   stringT host;
   stringT project;
   stringT department;
   char granted_pe[256];
   u_long32 slots;
   char cell[SGE_PATH_MAX + 1];
   stringT complexes;
   char history_path[SGE_PATH_MAX + 1];

   u_long32 job_number = 0;
   stringT job_name;
   int jobfound=0;

   time_t begin_time = -1, end_time = -1;
   
   u_long32 days;
   sge_qacct_columns column_sizes;
   int groupflag=0;
   int queueflag=0;
   int ownerflag=0;
   int projectflag=0;
   int departmentflag=0;
   int hostflag=0;
   int jobflag=0;
   int cellflag=0;
   int complexflag=0;
   int historyflag=0;
   int nohistflag=0;
   int beginflag=0;
   int endflag=0;
   int daysflag=0;
   int accountflag=0;
   int granted_peflag = 0;
   int slotsflag = 0;
   u_long32 taskstart = 0;
   u_long32 taskend = 0;
   u_long32 taskstep = 0;

   char acctfile[SGE_PATH_MAX + 1];
   char account[1024];
   sge_rusage_type dusage;
   sge_rusage_type totals;
   int ii, i_ret;
   lList *complex_options = NULL;
   lList *complex_list, *queue_list, *exechost_list;
   lList *complex_dirs, *queue_dirs, *exechost_dirs;
   lList *sorted_list = NULL;
   lListElem *queue_host, *global_host;
   int is_path_setup = 0;   
   u_long32 line = 0;
   int cl_err = 0;

   DENTER_MAIN(TOP_LAYER, "qacct");

   sge_gdi_param(SET_MEWHO, QACCT, NULL);
/*    sge_gdi_param(SET_ISALIVE, 1, NULL); */
   if ((cl_err = sge_gdi_setup(prognames[QACCT]))) {
      ERROR((SGE_EVENT, MSG_GDI_SGE_SETUP_FAILED_S, cl_errstr(cl_err)));
      SGE_EXIT(1);
   }

   *acctfile = '\0';

   memset(owner, 0, sizeof(owner));
   memset(project, 0, sizeof(project));
   memset(department, 0, sizeof(department));
   memset(granted_pe, 0, sizeof(granted_pe));
   slots = 0;
   memset(group, 0, sizeof(group));
   memset(queue, 0, sizeof(queue));
   memset(host, 0, sizeof(host));
   memset(cell, 0, sizeof(cell));
   memset(complexes, 0, sizeof(complexes));
   memset(job_name, 0, sizeof(job_name));
   memset(&totals, 0, sizeof(totals));

   column_sizes.host       = strlen(MSG_HISTORY_HOST)+1;
   column_sizes.queue      = strlen(MSG_HISTORY_QUEUE)+1;
   column_sizes.group      = strlen(MSG_HISTORY_GROUP)+1;
   column_sizes.owner      = strlen(MSG_HISTORY_OWNER)+1;
   column_sizes.project    = strlen(MSG_HISTORY_PROJECT)+1;
   column_sizes.department = strlen(MSG_HISTORY_DEPARTMENT)+1;  
   column_sizes.granted_pe = strlen(MSG_HISTORY_PE)+1;
   column_sizes.slots      = 7;


   /*
   ** Read in the command line arguments.
   */
   for (ii = 1; ii < argc; ii++) {
      /*
      ** owner
      */
      if (!strcmp("-o", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               ownerflag = 1;
            }
            else {
               strcpy(owner, argv[++ii]);
            }
         }
         else
            ownerflag = 1;
      }
      /*
      ** group
      */
      else if (!strcmp("-g", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               groupflag = 1;
            }
            else {
               u_long32 gid;

               if (sscanf(argv[++ii], u32, &gid) == 1) {
                  struct group *pg;

                  pg = getgrgid(gid);
                  strcpy(group, (pg ? pg->gr_name : argv[ii]));
               }
               else {
                  strcpy(group, argv[ii]);
               }
            }
         }
         else
            groupflag = 1;
      }
      /*
      ** queue
      */
      else if (!strcmp("-q", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               queueflag = 1;
            }
            else {
               strcpy(queue, argv[++ii]);
            }
         }
         else
            queueflag = 1;
      }
      /*
      ** host
      */
      else if (!strcmp("-h", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               hostflag = 1;
            }
            else {
               u_short id = 0;
               int ret;
               prepare_enroll("qacct", id, NULL);
               ret = getuniquehostname(argv[++ii], host, 0);
               leave_commd();
               if (ret) {
                  fprintf(stderr, MSG_HISTORY_FAILEDRESOLVINGHOSTNAME_SS ,
                       argv[ii], cl_errstr(ret));
                  show_the_way(stderr);
                  return 1;
               }
            }
         }
         else
            hostflag = 1;
      }
      /*
      ** job
      */
      else if (!strcmp("-j", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               jobflag = 1;
            }
            else {
               if (sscanf(argv[++ii], u32, &job_number) != 1) {
                  job_number = 0;
                  strcpy(job_name, argv[ii]);
               }
            }
         }
         else {
            jobflag = 1;
         }
      }
      /*
      ** task id
      */
      else if (!strcmp("-t", argv[ii])) {
         if (!argv[ii+1] || *(argv[ii+1])=='-') {
            fprintf(stderr, MSG_HISTORY_TOPTIONMASTHAVELISTOFTASKIDRANGES ); 
            show_the_way(stderr);
            return 1;
         } else {
            lList* task_id_range_list=NULL;
            lList* answer=NULL;

            ii++;
            task_id_range_list = parse_ranges(argv[ii], 0, 1, &answer,
                                             NULL, INF_NOT_ALLOWED);
            if (!task_id_range_list) {
               lFreeList(answer);
               fprintf(stderr, MSG_HISTORY_INVALIDLISTOFTASKIDRANGES_S , argv[ii]);
               show_the_way(stderr);
               return 1; 
            }
            taskstart = lGetUlong(lFirst(task_id_range_list), RN_min);
            taskend = lGetUlong(lFirst(task_id_range_list), RN_max);
            taskstep = lGetUlong(lFirst(task_id_range_list), RN_step);
            if (!taskstep)
               taskstep = 1;
         }
      }
      /*
      ** time options
      ** begin time
      */
      else if (!strcmp("-b", argv[ii])) {
         if (argv[ii+1]) {
            begin_time = sge_parse_date_time(argv[++ii], NULL, NULL);
            if  (begin_time == -1) {
               /*
               ** problem: insufficient error reporting
               */
               show_the_way(stderr);
            }
            DPRINTF(("begin is: %ld\n", begin_time));
            beginflag = 1; 
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** end time
      */
      else if (!strcmp("-e", argv[ii])) {
         if (argv[ii+1]) {
            end_time = sge_parse_date_time(argv[++ii], NULL, NULL);
            if  (end_time == -1) {
               /*
               ** problem: insufficient error reporting
               */
               show_the_way(stderr);
            }
            DPRINTF(("end is: %ld\n", end_time));
            endflag = 1; 
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** days
      */
      else if (!strcmp("-d", argv[ii])) {
         if (argv[ii+1]) {
            i_ret = sscanf(argv[++ii], u32, &days);
            if (i_ret != 1) {
               /*
               ** problem: insufficient error reporting
               */
               show_the_way(stderr);
            }
            DPRINTF(("days is: %d\n", days));
            daysflag = 1; 
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** project
      */
      else if (!strcmp("-P", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               projectflag = 1;
            }
            else {
               strcpy(project, argv[++ii]);
            }
         }
         else
            projectflag = 1;
      }
      /*
      ** department
      */
      else if (!strcmp("-D", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               departmentflag = 1;
            }
            else {
               strcpy(department, argv[++ii]);
            }
         }
         else
            departmentflag = 1;
      }
      else if (!strcmp("-pe", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               granted_peflag = 1;
            }
            else {
               strcpy(granted_pe, argv[++ii]);
            }
         }
         else
            granted_peflag = 1;
      }
      else if (!strcmp("-slots", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               slotsflag = 1;
            }
            else {
               slots = atol(argv[++ii]);
            }
         }
         else
            slotsflag = 1;
      }
      
      /*
      ** -nohist does not use history data
      ** for evaluation of -l criteria
      ** problem: -history and -nohist results
      ** in strange initialisation
      */
      else if (!strcmp("-nohist",argv[ii])) {
         nohistflag = 1;
      }
      /*
      ** complex attributes
      ** option syntax is described as
      ** -l[<range-expr>] attr[=value],...
      ** with range not being applicable here,
      ** so syntax is
      ** -l attr[=value],...
      */
      else if (!strcmp("-l",argv[ii])) {
         if (argv[ii+1]) {
            /*
            ** add blank cause no range can be specified
            ** as described in sge_resource.c
            */
            strcpy(complexes, " ");
            strcat(complexes, argv[++ii]);
            complexflag = 1;
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** alternative path to history directory
      */
      else if (!strcmp("-history",argv[ii])) {
         if (argv[ii+1]) {
            strcpy(history_path, argv[++ii]);
            historyflag = 1;
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** alternative accounting file
      */
      else if (!strcmp("-f",argv[ii])) {
         if (argv[ii+1]) {
            strcpy(acctfile, argv[++ii]);
         }
         else {
            show_the_way(stderr);
         }
      }
      /*
      ** -A account
      */
      else if (!strcmp("-A",argv[ii])) {
         if (argv[ii+1]) {
            strcpy(account, argv[++ii]);
            accountflag = 1;
         }
         else {
            show_the_way(stderr);
         }
      }
      else if (!strcmp("-help",argv[ii])) {
         show_the_way(stdout);
      }
      else {
         show_the_way(stderr);
      }
   } /* end for */

   /*
   ** Note that this has to be a file on a local disk or an nfs
   ** mounted directory.
   */
   if (!acctfile[0]) {
      sge_getme(QACCT);
      if (cellflag) {
         strcpy(me.default_cell, cell);
      }
     sge_setup_paths(me.default_cell, &path, NULL);
     DPRINTF(("path.acct_file: %s\n", \
        (path.acct_file ? path.acct_file : "(NULL)")));
     strcpy(acctfile, path.acct_file);
     if (historyflag) {
        path.history_dir = history_path;
     }
     else if (nohistflag) {
        prepare_enroll(prognames[QACCT], 0, NULL);
        if (!sge_get_master(1)) {
           SGE_EXIT(1);
        }
        DPRINTF(("checking if qmaster is alive ...\n"));
        if (check_isalive(sge_get_master(0))) {
           ERROR((SGE_EVENT, MSG_HISTORY_QMASTERISNOTALIVE ));
           SGE_EXIT(1);
        }
       sge_setup_sig_handlers(QACCT);
     }
     is_path_setup = 1;
   }

   {
      SGE_STRUCT_STAT buf;
 
      if (SGE_STAT(acctfile,&buf)) {
         perror(acctfile); 
         printf(MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP);
         SGE_EXIT(1);
      }
   }
   /*
   ** evaluation time period
   ** begin and end are evaluated later, so these
   ** are the ones that have to be set to default
   ** values
   */
   /*
   ** problem: if all 3 options are set, what do?
   ** at the moment, ignore days, so nothing
   ** has to be done here in this case
   */
   if (!endflag) {
      if (daysflag && beginflag) {
         end_time = begin_time + days*24*3600;
      }
      else {
         end_time = -1; /* infty */
      }
   }
   if (!beginflag) {
      if (endflag && daysflag) {
         begin_time = end_time - days*24*3600;
      }
      else  if (daysflag) {
         begin_time = time(NULL) - days*24*3600;
      }
      else {
         begin_time = -1; /* minus infty */
      }
   }
   DPRINTF((" begin_time: %s", ctime(&begin_time)));
   DPRINTF((" end_time:   %s", ctime(&end_time)));

   /*
   ** parsing complex flags and initialising complex list
   */
   if (complexflag) {
      char *c_dir, *q_dir, *e_dir;

      complex_options = sge_parse_resources(NULL, complexes, complexes  + 1, "hard");
      if (!complex_options) {
         /*
         ** problem: still to tell some more to the user
         */
         show_the_way(stderr);
      }
      /* lDumpList(stdout, complex_options, 0); */
      if (!is_path_setup) {
         if (historyflag) {
            path.history_dir = history_path;
         }
         else {
            sge_getme(QACCT);
            if (cellflag) {
               strcpy(me.default_cell, cell);
            }
            sge_setup_paths(me.default_cell, &path, NULL);
            if (nohistflag) {
               prepare_enroll(prognames[QACCT], 0, NULL);
               if (!sge_get_master(1)) {
                  SGE_EXIT(1);
               }
               DPRINTF(("checking if qmaster is alive ...\n"));
               if (check_isalive(sge_get_master(0))) {
                  ERROR((SGE_EVENT, "qmaster is not alive"));
                  SGE_EXIT(1);
               }
               sge_setup_sig_handlers(QACCT);
            }
         }
         is_path_setup = 1;
      }
      
      if (!nohistflag) {
         c_dir = malloc(strlen(path.history_dir) + strlen(STR_DIR_COMPLEXES) + 3);
         if (!c_dir) {
            ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY));
            SGE_EXIT(1);
         }
         q_dir = malloc(strlen(path.history_dir) + strlen(STR_DIR_QUEUES) + 3);
         if (!q_dir) {
            ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY));
            FREE(c_dir);
            SGE_EXIT(1);
         }
         e_dir = malloc(strlen(path.history_dir) + strlen(STR_DIR_EXECHOSTS) + 3);
         if (!e_dir) {
            ERROR((SGE_EVENT, MSG_MEMORY_CANTALLOCATEMEMORY));
            FREE(c_dir);
            FREE(q_dir);
            SGE_EXIT(1);
         }
         strcpy(c_dir, path.history_dir);
         if (*c_dir && c_dir[strlen(c_dir) - 1] != '/') {
            strcat(c_dir, "/");
         }
         strcpy(q_dir, c_dir);
         strcpy(e_dir, c_dir);
         strcat(c_dir, STR_DIR_COMPLEXES);
         strcat(q_dir, STR_DIR_QUEUES);
         strcat(e_dir, STR_DIR_EXECHOSTS);
    
         i_ret = init_history_subdirs(c_dir, &complex_dirs);
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORCOMPLEXES_I, i_ret));
            FREE(c_dir);
            FREE(q_dir);
            FREE(e_dir);
            SGE_EXIT(1);
         }
         i_ret = init_history_subdirs(q_dir, &queue_dirs);
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFORQUEUES_I, i_ret));
            FREE(c_dir);
            FREE(q_dir);
            FREE(e_dir);
            SGE_EXIT(1);
         }
         i_ret = init_history_subdirs(e_dir, &exechost_dirs);
         if (i_ret) {
            ERROR((SGE_EVENT, MSG_HISTORY_QACCTERRORXREADHISTORYSUBDIRFOREXECHOSTS_I, i_ret));
            FREE(c_dir);
            FREE(q_dir);
            FREE(e_dir);
            SGE_EXIT(1);
         }
         FREE(c_dir);
         FREE(q_dir);
         FREE(e_dir);
      }
      else {
         get_qacct_lists(&complex_list, &queue_list, &exechost_list);
         DPRINTF(("got 3 current lists: \n"));
         lWriteList(complex_list);
         lWriteList(queue_list);
         lWriteList(exechost_list);
      } /* endif !nohistflag */
   } /* endif complexflag */

   fp = fopen(acctfile,"r");
   if (fp == NULL) {
      ERROR((SGE_EVENT, MSG_HISTORY_ERRORUNABLETOOPENX_S ,acctfile));
      printf(MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP);
      SGE_EXIT(1);
   }

   totals.ru_wallclock = 0;
   totals.ru_utime =  0;
   totals.ru_stime = 0;
   totals.cpu = 0;
   totals.mem = 0;
   totals.io = 0;
   totals.iow = 0;

   if (hostflag || queueflag || groupflag || ownerflag || projectflag ||
       departmentflag || granted_peflag || slotsflag) {
      sorted_list = lCreateList("sorted_list", QAJ_Type);
      if (!sorted_list) {
         ERROR((SGE_EVENT, MSG_HISTORY_NOTENOUGTHMEMORYTOCREATELIST ));
         SGE_EXIT(1);
      }
   }

   memset(&dusage, 0, sizeof(dusage));

   while (1) {
      line++;

      i_ret = sge_read_rusage(fp, &dusage);
      if (i_ret > 0) {
	      break;
      }
      else if (i_ret < 0) {
	      ERROR((SGE_EVENT, MSG_HISTORY_IGNORINGINVALIDENTRYINLINEX_U , u32c(line)));
	      continue;
      }

      /*
      ** skipping jobs that never ran
      */
      if ((dusage.start_time == 0) && complexflag)  {
         DPRINTF(("skipping job that never ran\n"));
         continue;
      }
      if ((begin_time != -1) && ((time_t) dusage.start_time < begin_time)) { 
         DPRINTF(("job no %ld started %s", dusage.job_number, \
            sge_ctime32(&dusage.start_time)));
         DPRINTF(("job count starts %s\n", ctime(&begin_time)));
         continue;
      }
      if ((end_time != -1) && ((time_t) dusage.start_time > end_time)) {
         DPRINTF(("job no %ld started %s", dusage.job_number, \
            sge_ctime32(&dusage.start_time)));
         DPRINTF(("job count ends %s", ctime(&end_time)));
         continue;
      }

      if (owner[0]) {
         if (sge_strnullcmp(owner, dusage.owner))
            continue;
      }
      if (group[0]) {
         if (sge_strnullcmp(group, dusage.group))
            continue;
      }
      if (queue[0]) {
         if (sge_strnullcmp(queue, dusage.qname))
            continue;
      }
      if (project[0]) {
         if (sge_strnullcmp(project, dusage.project))
            continue;
      }
      if (department[0]) {
         if (sge_strnullcmp(department, dusage.department))
            continue;
      }
      if (granted_pe[0]) {
         if (sge_strnullcmp(granted_pe, dusage.granted_pe))
            continue; 
      }
      if (slots > 0) {
         if (slots != dusage.slots)
            continue;
      }
      if (host[0]) {
         if (hostcmp(host, dusage.hostname))
            continue;
      }
      if (accountflag) {
         if (sge_strnullcmp(account, dusage.account))
            continue;
      }
      
      if (complexflag) {
         lListElem *queue;
         lList *complex_filled;
         int selected;
      
         if (!nohistflag) {
            DPRINTF(("job_no: %ld, start_time: %ld\n", dusage.job_number, dusage.start_time));
            i_ret = make_complex_list(complex_dirs, dusage.start_time, &complex_list);
            if (i_ret) {
               ERROR((SGE_EVENT, MSG_HISTORY_COMPLEXSTRUCTUREREFTOXCANTBEASSEMBLEDY_SI , 
                  sge_ctime32(&dusage.start_time), i_ret));
               ERROR((SGE_EVENT, MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG ));
               SGE_EXIT(1);
            }
            i_ret = find_queue_version_by_name(queue_dirs, dusage.qname, 
                  dusage.start_time, &queue, 0);
            if (i_ret || !queue) {
               ERROR((SGE_EVENT, MSG_HISTORY_QUEUECONFIGFORXREFTOYCANTBEASSEMBLEDY_SSI , \
                  dusage.qname, sge_ctime32(&dusage.start_time), i_ret));
               ERROR((SGE_EVENT, MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG ));
               lFreeList(complex_list);
               SGE_EXIT(1);         
            }

            i_ret = find_host_version_by_name(exechost_dirs, dusage.hostname,
                  dusage.start_time, &queue_host, FLG_BY_NAME_NOCASE);

            if (i_ret || !queue_host) {
               ERROR((SGE_EVENT, MSG_HISTORY_HOSTCONFIGFORHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSIU, 
                     dusage.hostname, sge_ctime32(&dusage.start_time), 
                     i_ret, u32c(dusage.job_number)));
               ERROR((SGE_EVENT, MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG ));
               SGE_EXIT(1);
            }

            i_ret = find_host_version_by_name(exechost_dirs, "global",
                  dusage.start_time, &global_host, FLG_BY_NAME_NOCASE);

            if (i_ret || !global_host) {
               ERROR((SGE_EVENT, MSG_HISTORY_GLOBALHOSTCONFIGFORGLOBALHOSTXREFTOYCANTBEASSEMBLEDYFORJOBZ_SSID , \
                     dusage.hostname, sge_ctime32(&dusage.start_time), i_ret, u32c(dusage.job_number)));
               ERROR((SGE_EVENT, MSG_HISTORY_USENOHISTTOREFTOCURCOMPLEXCONFIG ));
               SGE_EXIT(1);
            }

            exechost_list = lCreateList("Execution Host List", EH_Type);
            lAppendElem(exechost_list, queue_host);
            lAppendElem(exechost_list, global_host);
         }
         else {
            queue = lGetElemStr(queue_list, QU_qname, dusage.qname);
            if (!queue) {
               WARNING((SGE_EVENT, MSG_HISTORY_IGNORINGJOBXFORACCOUNTINGMASTERQUEUEYNOTEXISTS_IS,
                         (int)dusage.job_number, dusage.qname));
               continue;
            } 
         }
         complex_filled = NULL;
   
         set_qs_state(QS_STATE_EMPTY);

         queue_complexes2scheduler(&complex_filled, queue, exechost_list, complex_list, 0);

         if (!complex_filled) {
            ERROR((SGE_EVENT, \
               MSG_HISTORY_COMPLEXTEMPLATESCANTBEFILLEDCORRECTLYFORJOBX_D, \
               u32c(dusage.job_number)));
            lFreeList(complex_list);
            lFreeElem(queue);
            SGE_EXIT(1);
         }

         DPRINTF(("complex_filled: \n"));

         {
            lList *ccl[3];

#if 0
            ccl[0] = lGetList(global_host, EH_consumable_config_list);
            ccl[1] = lGetList(queue_host, EH_consumable_config_list);
            ccl[2] = lGetList(queue, QU_consumable_config_list);
#else
            ccl[0] = NULL;
            ccl[1] = NULL;
            ccl[2] = NULL;
#endif

            selected = sge_select_queue(complex_filled, complex_options, 1, NULL, 0, 1, ccl);
         }
         lFreeList(complex_filled);
  
         if (!selected) {
            continue;
         }
      } /* endif complexflag */

      if (jobflag) {
         showjob(&dusage);
      }
      else if (job_number || job_name[0]) {
         int show_ja_task = 0;

         if (taskstart && taskend && taskstep) {
            if (dusage.task_number >= taskstart && dusage.task_number <= taskend && 
                ((dusage.task_number-taskstart)%taskstep) == 0) { 
               show_ja_task = 1; 
            }
         } else {
            show_ja_task = 1;
         }
         
         if (((dusage.job_number == job_number) || !sge_strnullcmp(dusage.job_name, job_name)) &&
               show_ja_task) {
            showjob(&dusage);
            jobfound = 1;
         }
      }

      totals.ru_wallclock += dusage.ru_wallclock;
      totals.ru_utime  += dusage.ru_utime;
      totals.ru_stime  += dusage.ru_stime;
      totals.cpu += dusage.cpu;
      totals.mem += dusage.mem;
      totals.io  += dusage.io;
      totals.iow  += dusage.iow;

      /*
      ** collect information for sorted output and sums
      */
      if (hostflag || queueflag || groupflag || ownerflag || projectflag ||
          departmentflag || granted_peflag || slotsflag) {
         lListElem *ep = NULL;

         ep = lFirst(sorted_list);

         /*
         ** find either the correct place to insert the next element
         ** or the existing element to increase
         */
         while (hostflag && ep &&
                (hostcmp(lGetHost(ep, QAJ_host), dusage.hostname) < 0)) {
             ep = lNext(ep);
         }
         while (queueflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname) < 0) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname)))) {
             ep = lNext(ep);
         }
         while (groupflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_group), dusage.group) < 0) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname)))) {
             ep = lNext(ep);
         }
         while (ownerflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner) < 0) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
                (!groupflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group)))) {
             ep = lNext(ep);
         }
         while (projectflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project) < 0) &&
                (!ownerflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag ||  
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
                (!groupflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group)))) {
             ep = lNext(ep);
         }
         while (departmentflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_department), dusage.department) < 0) &&
                (!projectflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project))) &&
                (!ownerflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
                (!groupflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group)))) {
             ep = lNext(ep);
         }

         while (granted_peflag && ep &&
                (sge_strnullcmp(lGetString(ep, QAJ_granted_pe), dusage.granted_pe) < 0) &&
                (!departmentflag ||
                (sge_strnullcmp(lGetString(ep, QAJ_department), dusage.department))) &&
                (!projectflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project))) &&
                (!ownerflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
                (!groupflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group)))) {
             ep = lNext(ep);
         }
         while (slotsflag && ep &&
                (lGetUlong(ep, QAJ_slots) < dusage.slots) &&
                (!granted_peflag ||
                (sge_strnullcmp(lGetString(ep, QAJ_granted_pe), dusage.granted_pe))) &&
                (!departmentflag ||
                (sge_strnullcmp(lGetString(ep, QAJ_department), dusage.department))) &&
                (!projectflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project))) &&
                (!ownerflag ||
                (!sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) &&
                (!hostflag || 
                (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) && 
                (!queueflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
                (!groupflag || 
                (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group)))) {
             ep = lNext(ep);
         }
         /*
         ** is this now the element that we want
         ** or do we have to insert one?
         */
         if (ep &&
             (!hostflag ||       (!hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) &&
             (!queueflag ||      (!sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) &&
             (!groupflag ||      (!sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group))) &&
             (!ownerflag ||      (!sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) &&
             (!projectflag ||    (!sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project))) &&
             (!departmentflag || (!sge_strnullcmp(lGetString(ep, QAJ_department), dusage.department))) && 
             (!granted_peflag || (!sge_strnullcmp(lGetString(ep, QAJ_granted_pe), dusage.granted_pe))) &&
             (!slotsflag ||      (lGetUlong(ep, QAJ_slots) == dusage.slots))) {

            DPRINTF(("found element h:%s - q:%s - g:%s - o:%s - p:%s - d:%s - pe:%s - slots:%d\n", \
                     (dusage.hostname ? dusage.hostname : "(NULL)"), \
                     (dusage.qname ? dusage.qname : "(NULL)"), \
                     (dusage.group ? dusage.group : "(NULL)"), \
                     (dusage.owner ? dusage.owner : "(NULL)"), \
                     (dusage.project ? dusage.project : "(NULL)"), \
                     (dusage.department ? dusage.department : "(NULL)"), \
                     (dusage.granted_pe ? dusage.granted_pe : "(NULL)"), \
                     (int) dusage.slots));

            lSetDouble(ep, QAJ_ru_wallclock, lGetDouble(ep, QAJ_ru_wallclock) + dusage.ru_wallclock);

            lSetDouble(ep, QAJ_ru_utime, lGetDouble(ep, QAJ_ru_utime) + dusage.ru_utime);
            lSetDouble(ep, QAJ_ru_stime, lGetDouble(ep, QAJ_ru_stime) + dusage.ru_stime);
            lSetDouble(ep, QAJ_cpu, lGetDouble(ep, QAJ_cpu) + dusage.cpu);
            lSetDouble(ep, QAJ_mem, lGetDouble(ep, QAJ_mem) + dusage.mem);
            lSetDouble(ep, QAJ_io,  lGetDouble(ep, QAJ_io)  + dusage.io);
            lSetDouble(ep, QAJ_iow, lGetDouble(ep, QAJ_iow) + dusage.iow);
         }
         else {
            lListElem *new_ep;

            new_ep = lCreateElem(QAJ_Type);
            if (hostflag && dusage.hostname)
               lSetHost(new_ep, QAJ_host, dusage.hostname);
            if (queueflag && dusage.qname) 
               lSetString(new_ep, QAJ_queue, dusage.qname);
            if (groupflag && dusage.group) 
               lSetString(new_ep, QAJ_group, dusage.group);
            if (ownerflag && dusage.owner) 
               lSetString(new_ep, QAJ_owner, dusage.owner);
            if (projectflag && dusage.project)
               lSetString(new_ep, QAJ_project, dusage.project);
            if (departmentflag && dusage.department) 
               lSetString(new_ep, QAJ_department, dusage.department);
            if (granted_peflag && dusage.granted_pe)
               lSetString(new_ep, QAJ_granted_pe, dusage.granted_pe);
            if (slotsflag)
               lSetUlong(new_ep, QAJ_slots, dusage.slots);      

            lSetDouble(new_ep, QAJ_ru_wallclock, dusage.ru_wallclock);
            lSetDouble(new_ep, QAJ_ru_utime, dusage.ru_utime);
            lSetDouble(new_ep, QAJ_ru_stime, dusage.ru_stime);
            lSetDouble(new_ep, QAJ_cpu, dusage.cpu);
            lSetDouble(new_ep, QAJ_mem, dusage.mem);
            lSetDouble(new_ep, QAJ_io,  dusage.io);
            lSetDouble(new_ep, QAJ_iow, dusage.iow);                         
            
            lInsertElem(sorted_list, (ep ? lPrev(ep) : lLast(sorted_list)), new_ep);
         }        
      } /* endif sortflags */
   } /* end while sge_read_rusage */


   /*
   ** exit routine attempts to close file if not NULL
   */
   fclose(fp);
   fp = NULL;

   if (job_number || job_name[0]) {
      if (!jobfound) {
         if (job_number) {
            if (taskstart && taskend && taskstep) {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_DDDD , 
                  u32c(job_number), u32c(taskstart), u32c(taskend), u32c(taskstep)));
            } else {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBIDXNOTFOUND_D, u32c(job_number)));
            }
         }
         else {
            if (taskstart && taskend && taskstep) {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_SDDD , 
                  job_name, u32c(taskstart),u32c( taskend),u32c( taskstep)));
            } else {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBNAMEXNOTFOUND_S  , job_name));
            }
         }
         SGE_EXIT(1);
      }
      else 
         SGE_EXIT(0);
   } else {
      if (taskstart && taskend && taskstep) {
         ERROR((SGE_EVENT, MSG_HISTORY_TOPTIONREQUIRESJOPTION ));
         show_the_way(stderr);
         SGE_EXIT(0); 
      }
   }

   /*
   ** assorted output of statistics
   */
   if ( host[0] ) {
      column_sizes.host = strlen(host) + 1;
   } 
   if ( queue[0] ) {
      column_sizes.queue = strlen(queue) + 1;
   } 
   if ( group[0] ) {
      column_sizes.group = strlen(group) + 1;
   } 
   if ( owner[0] ) {
      column_sizes.owner = strlen(owner) + 1;
   } 
   if ( project[0] ) {
      column_sizes.project = strlen(project) + 1;
   } 
   if ( department[0] ) {
      column_sizes.department = strlen(department) + 1;
   } 
   if ( granted_pe[0] ) {
      column_sizes.granted_pe = strlen(granted_pe) + 1;
   } 
   
   calc_column_sizes(lFirst(sorted_list), &column_sizes);
   {
      lListElem *ep = NULL;
      int dashcnt = 0;
      char title_array[500];

#ifndef NO_SGE_COMPILE_DEBUG
      if (rmon_mlgetl(&DEBUG_ON, TOP_LAYER) & INFOPRINT) { 
         lWriteListTo(sorted_list, stdout);
      }
#endif

      if (host[0] || hostflag) {
         print_full(column_sizes.host , MSG_HISTORY_HOST);
         dashcnt += column_sizes.host ;
      }
      if (queue[0] || queueflag) {
         print_full(column_sizes.queue ,MSG_HISTORY_QUEUE );
         dashcnt += column_sizes.queue ;
      }
      if (group[0] || groupflag) {
         print_full(column_sizes.group , MSG_HISTORY_GROUP);
         dashcnt += column_sizes.group ;
      }
      if (owner[0] || ownerflag) {
         print_full(column_sizes.owner ,MSG_HISTORY_OWNER );
         dashcnt += column_sizes.owner ;
      }
      if (project[0] || projectflag) {
         print_full(column_sizes.project, MSG_HISTORY_PROJECT);
         dashcnt += column_sizes.project ;
      }
      if (department[0] || departmentflag) {
         print_full(column_sizes.department, MSG_HISTORY_DEPARTMENT);
         dashcnt += column_sizes.department;
      }
      if (granted_pe[0] || granted_peflag) {
         print_full(column_sizes.granted_pe, MSG_HISTORY_PE);
         dashcnt += column_sizes.granted_pe;
      }   
      if (slots > 0 || slotsflag) {
         print_full(column_sizes.slots,MSG_HISTORY_SLOTS );
         dashcnt += column_sizes.slots;
      }
         
      if (!dashcnt)
         printf(MSG_HISTORY_TOTSYSTEMUSAGE );

      sprintf(title_array,"%13.13s %13.13s %13.13s %13.13s %18.18s %18.18s %18.18s",
                     "WALLCLOCK", 
                     "UTIME", 
                     "STIME", 
                     "CPU", 
                     "MEMORY", 
                     "IO",
                     "IOW");
                        
      printf("%s\n", title_array);

      dashcnt += strlen(title_array);
      for (ii=0; ii < dashcnt; ii++)
         printf("=");
      printf("\n");
   
      if (hostflag || queueflag || groupflag || ownerflag || projectflag || 
          departmentflag || granted_peflag || slotsflag)
         ep = lFirst(sorted_list);

      while (totals.ru_wallclock) {
         const char *cp;

         if (host[0]) {
            print_full(column_sizes.host,  host);
         }
         else if (hostflag) {
            if (!ep)
               break;
            /*
            ** if file has empty fields and parsing results in NULL
            ** then we have a NULL list entry here
            ** we can't ignore it because it was a line in the
            ** accounting file
            */
            print_full(column_sizes.host, ((cp = lGetHost(ep, QAJ_host)) ? cp : ""));
         }
         if (queue[0]) {
            print_full(column_sizes.queue,  queue);
         }
         else if (queueflag) {
            if (!ep)
               break;
            print_full(column_sizes.queue, ((cp = lGetString(ep, QAJ_queue)) ? cp : ""));
         }
         if (group[0]) {
            print_full(column_sizes.group, group);
         }
         else if (groupflag) {
            if (!ep)
               break;
            print_full(column_sizes.group, ((cp = lGetString(ep, QAJ_group)) ? cp : ""));
         }
         if (owner[0]) {
            print_full(column_sizes.owner, owner);
         }
         else if (ownerflag) {
            if (!ep)
               break;
            print_full(column_sizes.owner, ((cp = lGetString(ep, QAJ_owner)) ? cp : "") );
         }
         if (project[0]) {
              print_full(column_sizes.project, project);
         }
         else if (projectflag) {
            if (!ep)
               break;
            print_full(column_sizes.project ,((cp = lGetString(ep, QAJ_project)) ? cp : ""));
         }
         if (department[0]) {
            print_full(column_sizes.department, department);
         }
         else if (departmentflag) {
            if (!ep)
               break;
            print_full(column_sizes.department, ((cp = lGetString(ep, QAJ_department)) ? cp : ""));
         }
         if (granted_pe[0]) {
            print_full(column_sizes.granted_pe, granted_pe);
         }   
         else if (granted_peflag) {
            if (!ep)
               break;
            print_full(column_sizes.granted_pe, ((cp = lGetString(ep, QAJ_granted_pe)) ? cp : ""));
         }         
         if (slots > 0) {
            print_full_ulong(column_sizes.slots, slots);
         }   
         else if (slotsflag) {
            if (!ep)
               break;
            print_full_ulong(column_sizes.slots, lGetUlong(ep, QAJ_slots));
         }         
            
         if (hostflag || queueflag || groupflag || ownerflag || projectflag || 
             departmentflag || granted_peflag || slotsflag) {
             printf("%13.0f %13.0f %13.0f %13.0f %18.3f %18.3f %18.3f\n",
                   lGetDouble(ep, QAJ_ru_wallclock),
                   lGetDouble(ep, QAJ_ru_utime),
                   lGetDouble(ep, QAJ_ru_stime),
                   lGetDouble(ep, QAJ_cpu),
                   lGetDouble(ep, QAJ_mem),
                   lGetDouble(ep, QAJ_io),
                   lGetDouble(ep, QAJ_iow));
                   
            ep = lNext(ep);
            if (!ep)
               break;
         }
         else {
            printf("%13.0f %13.0f %13.0f %13.0f %18.3f %18.3f %18.3f\n",
                totals.ru_wallclock,
                totals.ru_utime,
                totals.ru_stime,
                totals.cpu,
                totals.mem,
                totals.io,                                                
                totals.iow);
            break;
         }
      } /* end while */
   } /* end block */
 
   /*
   ** problem: other clients evaluate some status here
   */
   SGE_EXIT(0);
   return 0;
}

static void print_full_ulong(int full_length, u_long32 value) {
   char tmp_buf[100];
   DENTER(TOP_LAYER, "print_full_ulong");
      sprintf(tmp_buf, "%6"fu32, value);
      print_full(full_length, tmp_buf); 
   DEXIT;
}

static void print_full(int full_length, const char* string) {

   int string_length=0;
   int i = 0;
   int spaces = 0;

   DENTER(TOP_LAYER, "print_full");
  
   if ( string != NULL) {
      printf("%s",string); 
      string_length = strlen(string);
   }
   if (full_length > string_length) {
      spaces = full_length - string_length;
   }
   for (i = 0 ; i < spaces ; i++) {
      printf(" ");
   }  
   DEXIT;
}

static void calc_column_sizes(lListElem* ep, sge_qacct_columns* column_size_data) {
   lListElem* lep = NULL;
   DENTER(TOP_LAYER, "calc_column_sizes");
   

   if ( column_size_data == NULL ) {
      DEXIT;
      return;
   }
/*   column_size_data->host = 30;
   column_size_data->queue = 15;
   column_size_data->group = 10;
   column_size_data->owner = 10;
   column_size_data->project = 17;
   column_size_data->department = 20;  
   column_size_data->granted_pe = 15;
   column_size_data->slots = 6; */

   if ( column_size_data->host < strlen(MSG_HISTORY_HOST)+1  ) {
      column_size_data->host = strlen(MSG_HISTORY_HOST)+1;
   } 
   if ( column_size_data->queue < strlen(MSG_HISTORY_QUEUE)+1  ) {
      column_size_data->queue = strlen(MSG_HISTORY_QUEUE)+1;
   } 
   if ( column_size_data->group < strlen(MSG_HISTORY_GROUP)+1  ) {
      column_size_data->group = strlen(MSG_HISTORY_GROUP)+1;
   } 
   if ( column_size_data->owner < strlen(MSG_HISTORY_OWNER)+1  ) {
      column_size_data->owner = strlen(MSG_HISTORY_OWNER)+1;
   } 
   if ( column_size_data->project < strlen(MSG_HISTORY_PROJECT)+1  ) {
      column_size_data->project = strlen(MSG_HISTORY_PROJECT)+1;
   } 
   if ( column_size_data->department < strlen(MSG_HISTORY_DEPARTMENT)+1  ) {
      column_size_data->department = strlen(MSG_HISTORY_DEPARTMENT)+1;
   } 
   if ( column_size_data->granted_pe < strlen(MSG_HISTORY_PE)+1  ) {
      column_size_data->granted_pe = strlen(MSG_HISTORY_PE)+1;
   } 
   if ( column_size_data->slots <7  ) {
      column_size_data->slots = 7;
   } 

   if ( ep != NULL) {
      char tmp_buf[100];
      int tmp_length = 0;
      const char* tmp_string = NULL;
      lep = ep;
      while (lep) {
         /* host  */
         tmp_string = lGetHost(ep, QAJ_host);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->host < tmp_length) {
               column_size_data->host  = tmp_length + 1;
            }
         } 
         /* queue */
         tmp_string = lGetString(ep, QAJ_queue);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->queue < tmp_length) {
               column_size_data->queue  = tmp_length + 1;
            }
         } 
         /* group */
         tmp_string = lGetString(ep, QAJ_group) ;
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->group < tmp_length) {
               column_size_data->group  = tmp_length + 1;
            }
         } 
         /* owner */
         tmp_string = lGetString(ep, QAJ_owner);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->owner < tmp_length) {
               column_size_data->owner  = tmp_length + 1;
            }
         } 
         /* project */
         tmp_string = lGetString(lep, QAJ_project);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->project < tmp_length) {
               column_size_data->project  = tmp_length + 1;
            }
         } 

         /* department  */
         tmp_string = lGetString(ep, QAJ_department);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->department < tmp_length) {
               column_size_data->department  = tmp_length + 1;
            }
         } 
         /* granted_pe */
         tmp_string = lGetString(ep, QAJ_granted_pe) ;
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->granted_pe < tmp_length) {
               column_size_data->granted_pe  = tmp_length + 1;
            }
         } 

         /* slots */
         sprintf(tmp_buf,"%6"fu32, lGetUlong(ep, QAJ_slots));
         tmp_length = strlen(tmp_buf);
         if (column_size_data->slots < tmp_length) {
            column_size_data->slots  = tmp_length + 1;
         }
         lep = lNext(lep);
      }
   }
   DEXIT;
}


/*
** NAME
**   show_the_way
** PARAMETER
**   none
** RETURN
**   none
** EXTERNAL
**   none
** DESCRIPTION
**   displays usage for qacct client
**   note that the other clients use a common function
**   for this. output was adapted to a similar look.
*/
static void show_the_way(
FILE *fp 
) {
   DENTER(TOP_LAYER, "show_the_way");

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION));
         
   fprintf(fp, "%s qacct [options]\n",MSG_HISTORY_USAGE );
   fprintf(fp, " [-A account_string]          %s", MSG_HISTORY_A_OPT_USAGE); 
   fprintf(fp, " [-help]                      %s", MSG_HISTORY_help_OPT_USAGE);
   fprintf(fp, " [-h [host]]                  %s", MSG_HISTORY_h_OPT_USAGE );
   fprintf(fp, " [-q [queue]]                 %s", MSG_HISTORY_q_OPT_USAGE );
   fprintf(fp, " [-g [groupid|groupname]]     %s", MSG_HISTORY_g_OPT_USAGE );
   fprintf(fp, " [-o [owner]]                 %s", MSG_HISTORY_o_OPT_USAGE);
   fprintf(fp, " [-P [project]]               %s", MSG_HISTORY_P_OPT_USAGE );
   fprintf(fp, " [-D [department]]            %s", MSG_HISTORY_D_OPT_USAGE);
   fprintf(fp, " [-pe [pe_name]]              %s", MSG_HISTORY_pe_OPT_USAGE );
   fprintf(fp, " [-slots [slots]]             %s", MSG_HISTORY_slots_OPT_USAGE);
   fprintf(fp, " [-l attr=val,...]            %s", MSG_HISTORY_l_OPT_USAGE );
   fprintf(fp, " [-b begin_time]              %s", MSG_HISTORY_b_OPT_USAGE);
   fprintf(fp, " [-e end_time]                %s", MSG_HISTORY_e_OPT_USAGE);
   fprintf(fp, " [-d days]                    %s", MSG_HISTORY_d_OPT_USAGE );
   fprintf(fp, " [-j [[jobid|jobname]]]       %s", MSG_HISTORY_j_OPT_USAGE);
   fprintf(fp, " [-t taskid[-taskid[:step]]]  %s", MSG_HISTORY_t_OPT_USAGE );
   fprintf(fp, " [-history path]              %s", MSG_HISTORY_history_OPT_USAGE);
   fprintf(fp, " [-nohist]                    %s", MSG_HISTORY_nohist_OPT_USAGE);
   fprintf(fp, " [[-f] acctfile]              %s", MSG_HISTORY_f_OPT_USAGE );
   fprintf(fp, " begin_time, end_time         %s", MSG_HISTORY_beginend_OPT_USAGE );
  
   if (fp==stderr)
      SGE_EXIT(1);
   else 
      SGE_EXIT(0);   
}


/*
** NAME
**   showjob
** PARAMETER
**   dusage   - pointer to struct sge_rusage_type, representing
**              a line in the SGE accounting file
** RETURN
**   none
** EXTERNAL
**   none
** DESCRIPTION
**   detailed display of job accounting data
*/
static void showjob(
sge_rusage_type *dusage 
) {
   char maxvmem_usage[1000];

   printf("==============================================================\n");
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_QNAME, (dusage->qname ? dusage->qname : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_HOSTNAME, (dusage->hostname ? dusage->hostname : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_GROUP, (dusage->group ? dusage->group : MSG_HISTORY_SHOWJOB_NULL));    
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_OWNER, (dusage->owner ? dusage->owner : MSG_HISTORY_SHOWJOB_NULL));
   if (feature_is_enabled(FEATURE_SGEEE)) {
      printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_PROJECT, (dusage->project ? dusage->project : MSG_HISTORY_SHOWJOB_NULL));
      printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_DEPARTMENT, (dusage->department ? dusage->department : MSG_HISTORY_SHOWJOB_NULL));
   }
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_JOBNAME, (dusage->job_name ? dusage->job_name : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_JOBNUMBER, dusage->job_number);
   



   if (dusage->task_number)
      printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_TASKID, dusage->task_number);              /* job-array task number */
   else
      printf("%-13.12s%s\n",MSG_HISTORY_SHOWJOB_TASKID, "undefined");             

   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_ACCOUNT, (dusage->account ? dusage->account : MSG_HISTORY_SHOWJOB_NULL ));
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_PRIORITY, dusage->priority);
   printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_QSUBTIME,    sge_ctime32(&dusage->submission_time));

   if (dusage->start_time)
      printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_STARTTIME, sge_ctime32(&dusage->start_time));
   else
      printf("%-13.12s-/-\n",MSG_HISTORY_SHOWJOB_STARTTIME);

   if (dusage->end_time)
      printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_ENDTIME, sge_ctime32(&dusage->end_time));
   else
      printf("%-13.12s-/-\n",MSG_HISTORY_SHOWJOB_ENDTIME);


   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_GRANTEDPE, dusage->granted_pe);
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_SLOTS, dusage->slots);
   printf("%-13.12s%-3"fu32" %s %s\n",MSG_HISTORY_SHOWJOB_FAILED, dusage->failed, (dusage->failed ? ":" : ""), get_sstate_description(dusage->failed));
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_EXITSTATUS, dusage->exit_status);
   printf("%-13.12s%-13.0f\n",MSG_HISTORY_SHOWJOB_RUWALLCLOCK, dusage->ru_wallclock);  

   printf("%-13.12s%-13.0f\n",MSG_HISTORY_SHOWJOB_RUUTIME, dusage->ru_utime);    /* user time used */
   printf("%-13.12s%-13.0f\n", MSG_HISTORY_SHOWJOB_RUSTIME, dusage->ru_stime);    /* system time used */
      printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUMAXRSS,  dusage->ru_maxrss);
      printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUIXRSS,  dusage->ru_ixrss);   /* integral shared text size */
      printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUISMRSS,  dusage->ru_ismrss);     /* integral shared memory size*/
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUIDRSS,      dusage->ru_idrss);      /* integral unshared data "  */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUISRSS,      dusage->ru_isrss);      /* integral unshared stack "  */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUMINFLT,     dusage->ru_minflt);     /* page reclaims */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUMAJFLT,     dusage->ru_majflt);     /* page faults */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUNSWAP,      dusage->ru_nswap);      /* swaps */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUINBLOCK,    dusage->ru_inblock);    /* block input operations */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUOUBLOCK,    dusage->ru_oublock);    /* block output operations */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUMSGSND,     dusage->ru_msgsnd);     /* messages sent */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUMSGRCV,     dusage->ru_msgrcv);     /* messages received */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUNSIGNALS,   dusage->ru_nsignals);   /* signals received */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUNVCSW,      dusage->ru_nvcsw);      /* voluntary context switches */
   printf("%-13.12s%-20"fu32"\n",MSG_HISTORY_SHOWJOB_RUNIVCSW,     dusage->ru_nivcsw);     /* involuntary */

   printf("%-13.12s%-13.0f\n",   MSG_HISTORY_SHOWJOB_CPU,          dusage->cpu);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_MEM,          dusage->mem);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_IO,           dusage->io);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_IOW,          dusage->iow);
   printf("%-13.12s%s\n",  MSG_HISTORY_SHOWJOB_MAXVMEM,      resource_descr(dusage->maxvmem, TYPE_MEM, maxvmem_usage));
}

/*
** NAME
**   get_qacct_lists
** PARAMETER
**   ppcomplexes - list pointer-pointer to be set to the complex list, CX_Type
**   ppqueues    - list pointer-pointer to be set to the queues list, QU_Type
**   ppexechosts - list pointer-pointer to be set to the exechosts list,EH_Type
** RETURN
**   none
** EXTERNAL
**
** DESCRIPTION
**   retrieves the lists from qmaster
**   programmed after the get_all_lists() function in qstat.c,
**   but gets queue list in a different way
**   none of the old list types is explicitly used
**   function does its own error handling by calling SGE_EXIT
**   problem: exiting is against the philosophy, restricts usage to clients
*/
static void get_qacct_lists(
lList **ppcomplexes,
lList **ppqueues,
lList **ppexechosts 
) {
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lList *alp = NULL;
   lListElem *aep = NULL;
   lList *mal = NULL;
   int cx_id = 0, eh_id = 0, q_id = 0;

   DENTER(TOP_LAYER, "get_qacct_lists");

   /*
   ** GET SGE_COMPLEX_LIST 
   */
   what = lWhat("%T(ALL)", CX_Type);
   cx_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_COMPLEX_LIST, SGE_GDI_GET,
                           NULL, NULL, what, NULL);
   what = lFreeWhat(what);

   if (alp) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      SGE_EXIT(1);
   }

   /*
   ** GET SGE_EXECHOST_LIST 
   */
   where = lWhere("%T(%I!=%s)", EH_Type, EH_name, SGE_TEMPLATE_NAME);
   what = lWhat("%T(ALL)", EH_Type);
   eh_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_EXECHOST_LIST, SGE_GDI_GET,
                           NULL, where, what, NULL);
   what = lFreeWhat(what);
   where = lFreeWhere(where);

   if (alp) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      SGE_EXIT(1);
   }

   /*
   ** GET SGE_QUEUE_LIST 
   */
   what = lWhat("%T(ALL)", QU_Type);
   alp = sge_gdi(SGE_QUEUE_LIST, SGE_GDI_GET, ppqueues, NULL, what);
   q_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_QUEUE_LIST, SGE_GDI_GET,
                           NULL, NULL, what, &mal);
   what = lFreeWhat(what);

   if (alp) {
      ERROR((SGE_EVENT, lGetString(lFirst(alp), AN_text)));
      SGE_EXIT(1);
   }

   /*
   ** handle results
   */
   /* --- complex */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_COMPLEX_LIST, cx_id,
                                 mal, ppcomplexes);
   if (!alp) {
      ERROR((SGE_EVENT, MSG_HISTORY_GETALLLISTSGETCOMPLEXLISTFAILED ));
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      ERROR((SGE_EVENT, lGetString(aep, AN_text)));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- exec host */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_EXECHOST_LIST, eh_id, 
                                 mal, ppexechosts);
   if (!alp) {
      ERROR((SGE_EVENT, MSG_HISTORY_GETALLLISTSGETEXECHOSTLISTFAILED ));
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      ERROR((SGE_EVENT, lGetString(aep, AN_text)));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- queue */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_QUEUE_LIST, q_id, 
                                 mal, ppqueues);
   if (!alp) {
      ERROR((SGE_EVENT, MSG_HISTORY_GETALLLISTSGETQUEUELISTFAILED ));
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      ERROR((SGE_EVENT, lGetString(aep, AN_text)));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   mal = lFreeList(mal);

   DEXIT;
   return;
}

