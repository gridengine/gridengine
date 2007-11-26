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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <errno.h>

#include "uti/sge_dstring.h"
#include "uti/sge_stdio.h"
#include "uti/sge_string.h"
#include "uti/sge_stdlib.h"
#include "uti/sge_spool.h"
#include "uti/sge_unistd.h"
#include "uti/sge_uidgid.h"

#include "sge.h"
#include "sge_all_listsL.h"
#include "sge_sched.h"
#include "commlib.h"
#include "sig_handlers.h"
#include "execution_states.h"
#include "sge_feature.h"
#include "sge_rusage.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "qm_name.h"
#include "basis_types.h"
#include "sge_time.h"
#include "sge_parse_num_par.h"
#include "sge_hostname.h"
#include "sge_answer.h"
#include "sge_range.h"
#include "sge_ulong.h"
#include "sge_centry.h"
#include "sge_qinstance.h"
#include "sge_cqueue.h"
#include "sge_profiling.h"

#include "gdi/sge_gdi.h"
#include "gdi/sge_gdi_ctx.h"

#include "msg_common.h"
#include "msg_history.h"
#include "msg_clients_common.h"


typedef struct {
   int host;
   int queue;
   int group;
   int owner;
   int project;
   int department;
   int granted_pe;
   int slots;
   int arid;
} sge_qacct_columns;

static void qacct_usage(sge_gdi_ctx_class_t **ctx, FILE *fp);
static void print_full(int length, const char* string);
static void print_full_ulong(int length, u_long32 value); 
static void calc_column_sizes(lListElem* ep, sge_qacct_columns* column_size_data );
static void showjob(sge_rusage_type *dusage);
static bool get_qacct_lists(sge_gdi_ctx_class_t *ctx, lList **alpp,
                            lList **ppcomplex, lList **ppqeues, lList **ppexechosts,
                            lList **hgrp_l);
static void free_qacct_lists(lList **ppcomplex, lList **ppqeues, lList **ppexechosts, lList **hgrp_l);
static int sge_read_rusage(FILE *fp, sge_rusage_type *d);

/*
** statics
*/
static FILE *fp = NULL;

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
int main(int argc, char **argv)
{
   const char *owner = NULL;
   char *group = NULL;
   char *host = NULL;
   const char *project = NULL;
   const char *department = NULL;
   const char *account = NULL;
   const char *granted_pe = NULL;
   const char *complexes = NULL;
   const char *job_name = NULL;
   u_long32 slots;
   u_long32 job_number = 0;
   u_long32 ar_number = 0;
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
   int complexflag=0;
   int beginflag=0;
   int endflag=0;
   int daysflag=0;
   int accountflag=0;
   int granted_peflag = 0;
   int slotsflag = 0;
   int arflag=0;
   u_long32 taskstart = 0;
   u_long32 taskend = 0;
   u_long32 taskstep = 0;
   bool summary_view = false;
   int ignored_jobs = 0;             /* counter of ignored jobs for accounting */

   sge_rusage_type dusage;
   sge_rusage_type totals;
   int ii;
   lList *complex_options = NULL;
   lList *centry_list = NULL;
   lList *queue_list = NULL;
   lList *exechost_list = NULL;
   lList *hgrp_list = NULL;
   lList *queueref_list = NULL;
   lList *queue_name_list = NULL;
   lList *sorted_list = NULL;
   lSortOrder *sort_order = NULL;
   int is_path_setup = 0;   
   u_long32 line = 0;
   const char *acct_file = NULL; 
   lList *alp = NULL;
   sge_gdi_ctx_class_t *ctx = NULL;

   DENTER_MAIN(TOP_LAYER, "qacct");

   prof_mt_init();

   log_state_set_log_gui(1);

   if (sge_gdi2_setup(&ctx, QACCT, MAIN_THREAD, &alp) != AE_OK) {
      answer_list_output(&alp);
      goto QACCT_EXIT;
   }

   slots = 0;
   memset(&totals, 0, sizeof(totals));

   column_sizes.host       = strlen(MSG_HISTORY_HOST)+1;
   column_sizes.queue      = strlen(MSG_HISTORY_QUEUE)+1;
   column_sizes.group      = strlen(MSG_HISTORY_GROUP)+1;
   column_sizes.owner      = strlen(MSG_HISTORY_OWNER)+1;
   column_sizes.project    = strlen(MSG_HISTORY_PROJECT)+1;
   column_sizes.department = strlen(MSG_HISTORY_DEPARTMENT)+1;  
   column_sizes.granted_pe = strlen(MSG_HISTORY_PE)+1;
   column_sizes.slots      = 5;
   column_sizes.arid       = 5;

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
            } else {
               owner = argv[++ii];
            }
         } else {
            ownerflag = 1;
         }
      }
      /*
      ** group
      */
      else if (!strcmp("-g", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               groupflag = 1;
            } else {
               u_long32 gid;
               stringT buffer;

               if (sscanf(argv[++ii], sge_u32, &gid) == 1) {
                  if (sge_gid2group((gid_t)gid, buffer, 
                                   MAX_STRING_SIZE, MAX_NIS_RETRIES) != 0) {
                     group = sge_strdup(group, argv[ii]);
                  } else {
                     group = sge_strdup(group, buffer);
                  }
               } else {
                  group = sge_strdup(group, argv[ii]);
               }
            }
         } else {
            groupflag = 1;
         }
      }
      /*
      ** queue
      */
      else if (!strcmp("-q", argv[ii])) {
         queueflag = 1;
         if (argv[ii+1]) {
            if (*(argv[ii+1]) != '-') {
               hostflag = 1;
               lAddElemStr(&queueref_list, QR_name, argv[++ii], QR_Type);
            }
         }
      }
      /*
      ** host
      */
      else if (!strcmp("-h", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               hostflag = 1;
            } else {
               char unique[CL_MAXHOSTLEN];
               ctx->prepare_enroll(ctx);
               if (getuniquehostname(argv[++ii], unique, 0) != CL_RETVAL_OK) {
                   /*
                    * we can't resolve the hostname, but that's no drama for qacct.
                    * maybe the hostname is no longer active but the usage information
                    * is already available
                    */
                  host = sge_strdup(host, argv[ii]);
               } else {
                  host = sge_strdup(host, unique);
               }
            }
         } else {
            hostflag = 1;
         }
      }
      /*
      ** job
      */
      else if (!strcmp("-j", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               jobflag = 1;
            } else if (sscanf(argv[++ii], sge_u32, &job_number) != 1) {
               job_number = 0;
               job_name = argv[ii];
            }
         } else {
            jobflag = 1;
         }
      }
      /*
      ** task id
      */
      else if (!strcmp("-t", argv[ii])) {
         if (!argv[ii+1] || *(argv[ii+1])=='-') {
            fprintf(stderr, "%s\n", MSG_HISTORY_TOPTIONMASTHAVELISTOFTASKIDRANGES ); 
            qacct_usage(&ctx, stderr);
            DRETURN(1);
         } else {
            lList* task_id_range_list=NULL;
            lList* answer=NULL;

            ii++;
            range_list_parse_from_string(&task_id_range_list, &answer,
                                         argv[ii], false, true, INF_NOT_ALLOWED);
            if (!task_id_range_list) {
               lFreeList(&answer);
               fprintf(stderr, MSG_HISTORY_INVALIDLISTOFTASKIDRANGES_S , argv[ii]);
               fprintf(stderr, "\n");
               qacct_usage(&ctx, stderr);
               DRETURN(1); 
            }
            taskstart = lGetUlong(lFirst(task_id_range_list), RN_min);
            taskend = lGetUlong(lFirst(task_id_range_list), RN_max);
            taskstep = lGetUlong(lFirst(task_id_range_list), RN_step);
            if (!taskstep) {
               taskstep = 1;
            }
         }
      }
      /*
      ** time options
      ** begin time
      */
      else if (!strcmp("-b", argv[ii])) {
         if (argv[ii+1]) {
            u_long32 tmp_begin_time;

            if  (!ulong_parse_date_time_from_string(&tmp_begin_time, NULL, argv[++ii])) {
               /*
               ** problem: insufficient error reporting
               */
               qacct_usage(&ctx, stderr);
            }
            begin_time = (time_t)tmp_begin_time;
            DPRINTF(("begin is: %ld\n", begin_time));
            beginflag = 1; 
         } else {
            qacct_usage(&ctx, stderr);
         }
      }
      /*
      ** end time
      */
      else if (!strcmp("-e", argv[ii])) {
         if (argv[ii+1]) {
            u_long32 tmp_end_time;

            if  (!ulong_parse_date_time_from_string(&tmp_end_time, NULL, argv[++ii])) {
               /*
               ** problem: insufficient error reporting
               */
               qacct_usage(&ctx, stderr);
            }
            end_time = (time_t)tmp_end_time;
            DPRINTF(("end is: %ld\n", end_time));
            endflag = 1; 
         } else {
            qacct_usage(&ctx, stderr);
         }
      }
      /*
      ** days
      */
      else if (!strcmp("-d", argv[ii])) {
         if (argv[ii+1]) {
            if (sscanf(argv[++ii], sge_u32, &days) != 1) {
               /*
               ** problem: insufficient error reporting
               */
               qacct_usage(&ctx, stderr);
            }
            DPRINTF(("days is: %d\n", days));
            daysflag = 1; 
         } else {
            qacct_usage(&ctx, stderr);
         }
      }
      /*
      ** project
      */
      else if (!strcmp("-P", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               projectflag = 1;
            } else {
               project = argv[++ii];
            }
         } else {
            projectflag = 1;
         }
      }
      /*
      ** department
      */
      else if (!strcmp("-D", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               departmentflag = 1;
            } else {
               department = argv[++ii];
            }
         } else {
            departmentflag = 1;
         }
      } else if (!strcmp("-pe", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               granted_peflag = 1;
            } else {
               granted_pe = argv[++ii];
            }
         } else {
            granted_peflag = 1;
         }
      } else if (!strcmp("-slots", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1]) == '-') {
               slotsflag = 1;
            } else {
               slots = atol(argv[++ii]);
            }
         } else {
            slotsflag = 1;
         }
      }
      /*
      ** advance reservation
      */
      else if (!strcmp("-ar", argv[ii])) {
         if (argv[ii+1]) {
            if (*(argv[ii+1])=='-') {
               arflag = 1;
            } else {
               if (sscanf(argv[++ii], sge_u32, &ar_number) != 1) {
                  fprintf(stderr, MSG_PARSE_INVALID_AR_MUSTBEUINT);
                  fprintf(stderr, "\n");
                  qacct_usage(&ctx, stderr);
                  DRETURN(1); 
               }
            }
         } else {
            arflag = 1;
         }
      }
      /*
      ** complex attributes
      ** option syntax is described as
      ** -l attr[=value],...
      */
      else if (!strcmp("-l",argv[ii])) {
         if (argv[ii+1]) {
            /*
            ** add blank cause no range can be specified
            ** as described in sge_resource.c
            */
            complexes = argv[++ii];
            complexflag = 1;
         } else {
            qacct_usage(&ctx, stderr);
         }
      } 
      /*
      ** alternative accounting file
      */
      else if (!strcmp("-f",argv[ii])) {
         if (argv[ii+1]) {
            acct_file = argv[++ii];
         } else {
            qacct_usage(&ctx, stderr);
         }
      }
      /*
      ** -A account
      */
      else if (!strcmp("-A",argv[ii])) {
         if (argv[ii+1]) {
            account = argv[++ii];
            accountflag = 1;
         } else {
            qacct_usage(&ctx, stderr);
         }
      } else if (!strcmp("-help",argv[ii])) {
         qacct_usage(&ctx, stdout);
      } else {
         qacct_usage(&ctx, stderr);
      }
   } /* end for */

   /*
   ** Note that this has to be a file on a local disk or an nfs
   ** mounted directory.
   */
   if (acct_file == NULL) {
      acct_file = ctx->get_acct_file(ctx);
      DPRINTF(("acct_file: %s\n", (acct_file ? acct_file : "(NULL)")));
     
      sge_setup_sig_handlers(QACCT);
      is_path_setup = 1;
   }

   {
      SGE_STRUCT_STAT buf;
 
      if (SGE_STAT(acct_file, &buf)) {
         perror(acct_file); 
         printf("%s\n", MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP);
         goto QACCT_EXIT;
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
         end_time = begin_time + (time_t)(days*24*3600);
      } else {
         end_time = -1; /* infty */
      }
   }
   if (!beginflag) {
      if (endflag && daysflag) {
         begin_time = end_time - (time_t)(days*24*3600);
      } else if (daysflag) {
         begin_time = time(NULL) - (time_t)(days*24*3600);
      } else {
         begin_time = -1; /* minus infty */
      }
   }
   DPRINTF((" begin_time: %s", ctime(&begin_time)));
   DPRINTF((" end_time:   %s", ctime(&end_time)));

   {
      dstring cqueue_name = DSTRING_INIT;
      dstring host_or_hgroup = DSTRING_INIT;      
      lListElem *qref_pattern = NULL;
      const char *name = NULL;
      bool has_hostname = false;
      bool has_domain = true;

      for_each(qref_pattern, queueref_list) {
         name = lGetString(qref_pattern, QR_name); 
         cqueue_name_split(name, &cqueue_name, &host_or_hgroup,
                           &has_hostname, &has_domain);
         if (has_domain) {
            break;
         }
      }
      sge_dstring_free(&cqueue_name);
      sge_dstring_free(&host_or_hgroup);
      
      /* the user did not specify a queue domain, therefor we need no information
         from the qmaster, but we have to work on the user input and generate the
         same data structure, that we would have gotten with the qmaster functions*/
      if (!has_domain) {
         for_each(qref_pattern, queueref_list) {
            dstring qi_name = DSTRING_INIT;
            char *tmp_str = NULL;
            name = lGetString(qref_pattern, QR_name); 
           
            sge_dstring_copy_string(&qi_name, name); 
           
            if ((tmp_str = strchr(name, '@')) == NULL){
               sge_dstring_append(&qi_name, "@*");
            } else if (*(tmp_str+1) == '\0'){
               sge_dstring_append(&qi_name, "*");
            }
            
            lAddElemStr(&queue_name_list, QR_name, sge_dstring_get_string(&qi_name), QR_Type);

            sge_dstring_free(&qi_name);
         }   
      }
      if (complexflag || (queueref_list && has_domain)) {
         /*
         ** parsing complex flags and initialising complex list
         */
         bool found_something;

         complex_options = centry_list_parse_from_string(NULL, complexes, true);
         if (!complex_options) {
            /*
            ** problem: still to tell some more to the user
            */
            qacct_usage(&ctx, stderr);
         }
         /* lDumpList(stdout, complex_options, 0); */
         if (!is_path_setup) {
            ctx->prepare_enroll(ctx);
            ctx->get_master(ctx, true);
            if (ctx->is_alive(ctx) != CL_RETVAL_OK) {
               ERROR((SGE_EVENT, "qmaster is not alive"));
               goto QACCT_EXIT;
            }
            sge_setup_sig_handlers(QACCT);
            is_path_setup = 1;
         }
         if (queueref_list && has_domain){ 
            if (!get_qacct_lists(ctx, &alp, NULL, &queue_list, NULL, &hgrp_list)) {
               answer_list_output(&alp);
               goto QACCT_EXIT;
            }   

            qref_list_resolve(queueref_list, NULL, &queue_name_list, 
                           &found_something, queue_list, hgrp_list, true, true);
            if (!found_something) {
               fprintf(stderr, "%s\n", MSG_QINSTANCE_NOQUEUES);
               goto QACCT_EXIT;
            }
         }  
         if (complexflag) {
            if (!get_qacct_lists(ctx, &alp, &centry_list, &queue_list, &exechost_list, NULL)) {
               answer_list_output(&alp);
               goto QACCT_EXIT;
            }   
         }   
      } /* endif complexflag */
   }

   /* debug output) */
   if (getenv("SGE_QACCT_DEBUG")) {
      printf("complex entries:\n");
      lWriteListTo(centry_list, stdout);
      printf("queue instances:\n");
      lWriteListTo(queue_list, stdout);
      printf("exec hosts:\n");
      lWriteListTo(exechost_list, stdout);
      printf("host groups\n");
      lWriteListTo(hgrp_list, stdout);
   }

   fp = fopen(acct_file, "r");
   if (fp == NULL) {
      ERROR((SGE_EVENT, MSG_HISTORY_ERRORUNABLETOOPENX_S ,acct_file));
      printf("%s\n", MSG_HISTORY_NOJOBSRUNNINGSINCESTARTUP);
      goto QACCT_EXIT;
   }

   totals.ru_wallclock = 0;
   totals.ru_utime =  0;
   totals.ru_stime = 0;
   totals.cpu = 0;
   totals.mem = 0;
   totals.io = 0;
   totals.iow = 0;

   if (hostflag || queueflag || groupflag || ownerflag || projectflag ||
       departmentflag || granted_peflag || slotsflag || arflag) {
      sorted_list = lCreateList("sorted_list", QAJ_Type);
      sort_order = lParseSortOrderVarArg(QAJ_Type, "%I+ %I+ %I+ %I+ %I+ %I+ %I+ %I+ %I+",
                                         QAJ_host,
                                         QAJ_queue,
                                         QAJ_group,
                                         QAJ_owner,
                                         QAJ_project,
                                         QAJ_department,
                                         QAJ_granted_pe,
                                         QAJ_slots,
                                         QAJ_arid);
      summary_view = true;
      if (sorted_list == NULL || sort_order == NULL) {
         ERROR((SGE_EVENT, MSG_HISTORY_NOTENOUGTHMEMORYTOCREATELIST));
         goto QACCT_EXIT;
      }
   }

   memset(&dusage, 0, sizeof(dusage));

   while (!shut_me_down) {
      int i_ret;
      line++;

      i_ret = sge_read_rusage(fp, &dusage);
      if (i_ret > 0) {
	      break;
      } else if (i_ret < 0) {
	      ERROR((SGE_EVENT, MSG_HISTORY_IGNORINGINVALIDENTRYINLINEX_U , sge_u32c(line)));
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
         continue;
      }
      if ((end_time != -1) && ((time_t) dusage.start_time > end_time)) {
         dstring ds = DSTRING_INIT;
         DPRINTF(("job no %ld started %s", dusage.job_number, \
            sge_ctime32(&dusage.start_time, &ds)));
         DPRINTF(("job count ends %s", ctime(&end_time)));
         sge_dstring_free(&ds);
         continue;
      }

      if (owner != NULL && sge_strnullcmp(owner, dusage.owner)) {
         continue;
      }
      if (group != NULL && sge_strnullcmp(group, dusage.group)) {
         continue;
      }
      if (queue_name_list){
         dstring qi = DSTRING_INIT;
         lListElem *elem = NULL;
         const char *queue;
         bool found = false;

         queue = sge_dstring_sprintf(&qi,"%s@%s", dusage.qname, dusage.hostname );
 
         for_each(elem, queue_name_list) {
            if (fnmatch(lGetString(elem, QR_name), queue, 0) == 0) {
               found = true;
               break;
            }
         }

         sge_dstring_free(&qi);
         
         if (!found){
            continue;
         }  
      }

      if (project != NULL && sge_strnullcmp(project, dusage.project)) {
         continue;
      }
      if (department != NULL && sge_strnullcmp(department, dusage.department)) {
         continue;
      }
      if (granted_pe != NULL && sge_strnullcmp(granted_pe, dusage.granted_pe)) {
         continue; 
      }
      if ((slots > 0) && (slots != dusage.slots)) {
         continue;
      }
      if ((ar_number > 0) && (ar_number != dusage.ar)) {
         continue;
      }
      if (host != NULL && sge_hostcmp(host, dusage.hostname)) {
         continue;
      }
      if (accountflag && sge_strnullcmp(account, dusage.account)) {
         continue;
      }
      
      if (complexflag) {
         dstring qi = DSTRING_INIT;
         lListElem *queue;
         int selected;
     
         sge_dstring_sprintf(&qi,"%s@%s", dusage.qname, dusage.hostname );
         queue = cqueue_list_locate_qinstance_msg(queue_list, sge_dstring_get_string(&qi), false);
         sge_dstring_free(&qi); 

         if (!queue) {
            /* 
            * queue no longer exists, we can't get the complex attributes for this job, 
            * we will ignore the job for accounting  and count the number of ignored jobs 
            */
            ignored_jobs++;
            continue;
         }
   
         sconf_set_qs_state(QS_STATE_EMPTY);

         selected = sge_select_queue(complex_options, queue, NULL, exechost_list,
                    centry_list, true, 1, NULL, NULL, NULL);
  
         if (!selected) {
            continue;
         }
      } /* endif complexflag */

      if (jobflag) {
         showjob(&dusage);
      } else if (job_number || job_name != NULL) {
         int show_ja_task = 0;

         if (taskstart && taskend && taskstep) {
            if (dusage.task_number >= taskstart && dusage.task_number <= taskend && 
                ((dusage.task_number-taskstart)%taskstep) == 0) { 
               show_ja_task = 1; 
            }
         } else {
            show_ja_task = 1;
         }
        
         if (((dusage.job_number == job_number) || !sge_patternnullcmp(dusage.job_name, job_name)) &&
         /*!sge_strnullcmp(dusage.job_name, job_name)) && */
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
      if (summary_view) {
         lListElem *ep = NULL;

         ep = lFirst(sorted_list);

         /*
         ** find either the correct place to insert the next element
         ** or the existing element to increase
         */

         while (ep && ((slotsflag && (lGetUlong(ep, QAJ_slots) != dusage.slots)) || 
                (granted_peflag && (sge_strnullcmp(lGetString(ep, QAJ_granted_pe), dusage.granted_pe))) || 
                (departmentflag && (sge_strnullcmp(lGetString(ep, QAJ_department), dusage.department))) ||
                (projectflag && (sge_strnullcmp(lGetString(ep, QAJ_project), dusage.project))) ||
                (ownerflag && (sge_strnullcmp(lGetString(ep, QAJ_owner), dusage.owner))) ||
                (hostflag && (sge_hostcmp(lGetHost(ep, QAJ_host), dusage.hostname))) || 
                (queueflag && (sge_strnullcmp(lGetString(ep, QAJ_queue), dusage.qname))) ||
                (groupflag && (sge_strnullcmp(lGetString(ep, QAJ_group) , dusage.group))) ||
                (arflag && (lGetUlong(ep, QAJ_arid) != dusage.ar))
                )){
             ep = lNext(ep);
         }
         /*
         ** is this now the element that we want
         ** or do we have to insert one?
         */
         if (ep != NULL) {

            DPRINTF(("found element h:%s - q:%s - g:%s - o:%s - p:%s - d:%s - pe:%s - slots:%d - ar:%d\n", \
                     (dusage.hostname ? dusage.hostname : "(NULL)"), \
                     (dusage.qname ? dusage.qname : "(NULL)"), \
                     (dusage.group ? dusage.group : "(NULL)"), \
                     (dusage.owner ? dusage.owner : "(NULL)"), \
                     (dusage.project ? dusage.project : "(NULL)"), \
                     (dusage.department ? dusage.department : "(NULL)"), \
                     (dusage.granted_pe ? dusage.granted_pe : "(NULL)"), \
                     (int) dusage.slots, \
                     (int) dusage.ar));

            lAddDouble(ep, QAJ_ru_wallclock, dusage.ru_wallclock);

            lAddDouble(ep, QAJ_ru_utime, dusage.ru_utime);
            lAddDouble(ep, QAJ_ru_stime, dusage.ru_stime);
            lAddDouble(ep, QAJ_cpu, dusage.cpu);
            lAddDouble(ep, QAJ_mem, dusage.mem);
            lAddDouble(ep, QAJ_io, dusage.io);
            lAddDouble(ep, QAJ_iow, dusage.iow);
         } else {
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
            if (arflag)
               lSetUlong(new_ep, QAJ_arid, dusage.ar);

            lSetDouble(new_ep, QAJ_ru_wallclock, dusage.ru_wallclock);
            lSetDouble(new_ep, QAJ_ru_utime, dusage.ru_utime);
            lSetDouble(new_ep, QAJ_ru_stime, dusage.ru_stime);
            lSetDouble(new_ep, QAJ_cpu, dusage.cpu);
            lSetDouble(new_ep, QAJ_mem, dusage.mem);
            lSetDouble(new_ep, QAJ_io,  dusage.io);
            lSetDouble(new_ep, QAJ_iow, dusage.iow);                         

            lInsertSorted(sort_order, new_ep, sorted_list);
         }        
      } /* endif sortflags */
   } /* end while sge_read_rusage */

   /*
   * print the warning about the count of ignored jobs for accounting 
   */
   if (ignored_jobs > 0) {
      WARNING((SGE_EVENT, MSG_HISTORY_IGNORINGJOBXFORACCOUNTINGMASTERQUEUEYNOTEXISTS_IS,
                      ignored_jobs));
      printf("\n");
   }

   /*
   ** exit routine attempts to close file if not NULL
   */
   FCLOSE(fp)
   fp = NULL;

   if (shut_me_down) {
      printf("%s\n", MSG_USER_ABORT);
      goto QACCT_EXIT;
   }
   if (job_number || job_name != NULL) {
      if (!jobfound) {
         if (job_number) {
            if (taskstart && taskend && taskstep) {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_DDDD , 
                  sge_u32c(job_number), sge_u32c(taskstart), sge_u32c(taskend), sge_u32c(taskstep)));
            } else {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBIDXNOTFOUND_D, sge_u32c(job_number)));
            }
         } else {
            if (taskstart && taskend && taskstep) {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBARRAYTASKSWXYZNOTFOUND_SDDD , 
                  job_name, sge_u32c(taskstart),sge_u32c( taskend),sge_u32c( taskstep)));
            } else {
               ERROR((SGE_EVENT, MSG_HISTORY_JOBNAMEXNOTFOUND_S  , job_name));
            }
         }
         goto QACCT_EXIT;
      } else {
         free_qacct_lists(&centry_list, &queue_list, &exechost_list, &hgrp_list);
         SGE_EXIT((void**)&ctx, 0);
      }
   } else if (taskstart && taskend && taskstep) {
      ERROR((SGE_EVENT, MSG_HISTORY_TOPTIONREQUIRESJOPTION ));
      qacct_usage(&ctx, stderr);
      free_qacct_lists(&centry_list, &queue_list, &exechost_list, &hgrp_list);
      SGE_EXIT((void**)&ctx, 0); 
   }

   /*
   ** assorted output of statistics
   */
   if (host != NULL) {
      column_sizes.host = strlen(host) + 1;
   } 
   if (group != NULL) {
      column_sizes.group = strlen(group) + 1;
   } 
   if (owner != NULL) {
      column_sizes.owner = strlen(owner) + 1;
   } 
   if (project != NULL) {
      column_sizes.project = strlen(project) + 1;
   } 
   if (department != NULL) {
      column_sizes.department = strlen(department) + 1;
   } 
   if (granted_pe != NULL) {
      column_sizes.granted_pe = strlen(granted_pe) + 1;
   }
 
   calc_column_sizes(lFirst(sorted_list), &column_sizes);
   {
      lListElem *ep = NULL;
      int dashcnt = 0;
      char title_array[500];

      if (host != NULL || hostflag) {
         print_full(column_sizes.host , MSG_HISTORY_HOST);
         dashcnt += column_sizes.host ;
      }
      if (queueflag) {
         print_full(column_sizes.queue ,MSG_HISTORY_QUEUE );
         dashcnt += column_sizes.queue ;
      }
      if (group != NULL || groupflag) {
         print_full(column_sizes.group , MSG_HISTORY_GROUP);
         dashcnt += column_sizes.group ;
      }
      if (owner != NULL || ownerflag) {
         print_full(column_sizes.owner ,MSG_HISTORY_OWNER );
         dashcnt += column_sizes.owner ;
      }
      if (project != NULL || projectflag) {
         print_full(column_sizes.project, MSG_HISTORY_PROJECT);
         dashcnt += column_sizes.project ;
      }
      if (department != NULL || departmentflag) {
         print_full(column_sizes.department, MSG_HISTORY_DEPARTMENT);
         dashcnt += column_sizes.department;
      }
      if (granted_pe != NULL || granted_peflag) {
         print_full(column_sizes.granted_pe, MSG_HISTORY_PE);
         dashcnt += column_sizes.granted_pe;
      }   
      if (slots > 0 || slotsflag) {
         print_full(column_sizes.slots, MSG_HISTORY_SLOTS);
         dashcnt += column_sizes.slots;
      }
      if (ar_number > 0 || arflag) {
         print_full(column_sizes.slots, MSG_HISTORY_AR);
         dashcnt += column_sizes.slots;
      }
         
      if (!dashcnt) {
         printf("%s\n", MSG_HISTORY_TOTSYSTEMUSAGE);
      }

      sprintf(title_array, "%13.13s %13.13s %13.13s %13.13s %18.18s %18.18s %18.18s",
                     "WALLCLOCK", 
                     "UTIME", 
                     "STIME", 
                     "CPU", 
                     "MEMORY", 
                     "IO",
                     "IOW");
                        
      printf("%s\n", title_array);

      dashcnt += strlen(title_array);
      for (ii=0; ii < dashcnt; ii++) {
         printf("=");
      }
      printf("\n");
   
      if (summary_view) {
         ep = lFirst(sorted_list);
      }
      
      while (totals.ru_wallclock) {
         const char *cp;

         if (host != NULL) {
            print_full(column_sizes.host,  host);
         } else if (hostflag) {
            if (ep == NULL) {
               break;
            }
            /*
            ** if file has empty fields and parsing results in NULL
            ** then we have a NULL list entry here
            ** we can't ignore it because it was a line in the
            ** accounting file
            */
            print_full(column_sizes.host, ((cp = lGetHost(ep, QAJ_host)) ? cp : ""));
         }
         if (queueflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.queue, ((cp = lGetString(ep, QAJ_queue)) ? cp : ""));
         }
         if (group != NULL) {
            print_full(column_sizes.group, group);
         } else if (groupflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.group, ((cp = lGetString(ep, QAJ_group)) ? cp : ""));
         }
         if (owner != NULL) {
            print_full(column_sizes.owner, owner);
         }
         else if (ownerflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.owner, ((cp = lGetString(ep, QAJ_owner)) ? cp : "") );
         }
         if (project != NULL) {
              print_full(column_sizes.project, project);
         }
         else if (projectflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.project ,((cp = lGetString(ep, QAJ_project)) ? cp : ""));
         }
         if (department != NULL) {
            print_full(column_sizes.department, department);
         } else if (departmentflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.department, ((cp = lGetString(ep, QAJ_department)) ? cp : ""));
         }
         if (granted_pe != NULL) {
            print_full(column_sizes.granted_pe, granted_pe);
         } else if (granted_peflag) {
            if (ep == NULL) {
               break;
            }
            print_full(column_sizes.granted_pe, ((cp = lGetString(ep, QAJ_granted_pe)) ? cp : ""));
         }         
         if (slots > 0) {
            print_full_ulong(column_sizes.slots, slots);
         } else if (slotsflag) {
            if (ep == NULL) {
               break;
            }
            print_full_ulong(column_sizes.slots, lGetUlong(ep, QAJ_slots));
         }

         if (ar_number > 0) {
            print_full_ulong(column_sizes.arid, ar_number);
         } else if (arflag) {
            if (ep == NULL) {
               break;
            }
            print_full_ulong(column_sizes.arid, lGetUlong(ep, QAJ_arid));
         }         
         
         if (summary_view) {
             printf("%13.0f %13.0f %13.0f %13.0f %18.3f %18.3f %18.3f\n",
                   lGetDouble(ep, QAJ_ru_wallclock),
                   lGetDouble(ep, QAJ_ru_utime),
                   lGetDouble(ep, QAJ_ru_stime),
                   lGetDouble(ep, QAJ_cpu),
                   lGetDouble(ep, QAJ_mem),
                   lGetDouble(ep, QAJ_io),
                   lGetDouble(ep, QAJ_iow));

            ep = lNext(ep);
            if (ep == NULL) {
               break;
            }
         } else {
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

   lFreeList(&sorted_list);
   lFreeSortOrder(&sort_order);
 
   /*
   ** problem: other clients evaluate some status here
   */
   sge_prof_cleanup();
   FREE(group);
   FREE(host);
   free_qacct_lists(&centry_list, &queue_list, &exechost_list, &hgrp_list);
   SGE_EXIT((void**)&ctx, 0);
   DRETURN(0);

FCLOSE_ERROR:
   ERROR((SGE_EVENT, MSG_FILE_ERRORCLOSEINGXY_SS, acct_file, strerror(errno)));
QACCT_EXIT:
   sge_prof_cleanup();
   lFreeList(&sorted_list);
   lFreeSortOrder(&sort_order);
   FREE(group);
   FREE(host);
   free_qacct_lists(&centry_list, &queue_list, &exechost_list, &hgrp_list);
   SGE_EXIT((void**)&ctx, 1);
   DRETURN(1);
}

static void print_full_ulong(int full_length, u_long32 value) {
   char tmp_buf[100];

   DENTER(TOP_LAYER, "print_full_ulong");
   sprintf(tmp_buf, "%5"sge_fu32, value);
   print_full(full_length, tmp_buf); 
   DRETURN_VOID;
}

static void print_full(int full_length, const char* string) {

   int string_length=0;

   DENTER(TOP_LAYER, "print_full");
   if ( string != NULL) {
      printf("%s",string); 
      string_length = strlen(string);
   }
   while (full_length > string_length) {
      printf(" ");
      string_length++;
   }
   DRETURN_VOID;
}

static void calc_column_sizes(lListElem* ep, sge_qacct_columns* column_size_data) {
   lListElem* lep = NULL;
   DENTER(TOP_LAYER, "calc_column_sizes");
   
   if (column_size_data == NULL) {
      DPRINTF(("no column size data!\n"));
      DRETURN_VOID;
   }

/*   column_size_data->host = 30;
   column_size_data->queue = 15;
   column_size_data->group = 10;
   column_size_data->owner = 10;
   column_size_data->project = 17;
   column_size_data->department = 20;  
   column_size_data->granted_pe = 15;
   column_size_data->slots = 6; */

   if (column_size_data->host < strlen(MSG_HISTORY_HOST)+1) {
      column_size_data->host = strlen(MSG_HISTORY_HOST)+1;
   } 
   if (column_size_data->queue < strlen(MSG_HISTORY_QUEUE)+1) {
      column_size_data->queue = strlen(MSG_HISTORY_QUEUE)+1;
   } 
   if (column_size_data->group < strlen(MSG_HISTORY_GROUP)+1) {
      column_size_data->group = strlen(MSG_HISTORY_GROUP)+1;
   } 
   if (column_size_data->owner < strlen(MSG_HISTORY_OWNER)+1) {
      column_size_data->owner = strlen(MSG_HISTORY_OWNER)+1;
   } 
   if (column_size_data->project < strlen(MSG_HISTORY_PROJECT)+1) {
      column_size_data->project = strlen(MSG_HISTORY_PROJECT)+1;
   } 
   if (column_size_data->department < strlen(MSG_HISTORY_DEPARTMENT)+1) {
      column_size_data->department = strlen(MSG_HISTORY_DEPARTMENT)+1;
   } 
   if (column_size_data->granted_pe < strlen(MSG_HISTORY_PE)+1) {
      column_size_data->granted_pe = strlen(MSG_HISTORY_PE)+1;
   } 
   if (column_size_data->slots < 5) {
      column_size_data->slots = 5;
   } 
   if (column_size_data->arid < 5) {
      column_size_data->arid = 5;
   } 

   if (ep != NULL) {
      char tmp_buf[100];
      int tmp_length = 0;
      const char* tmp_string = NULL;
      lep = ep;
      while (lep) {
         /* host  */
         tmp_string = lGetHost(lep, QAJ_host);
         if (tmp_string != NULL) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->host <= tmp_length) {
               column_size_data->host  = tmp_length + 1;
            }
         } 
         /* queue */
         tmp_string = lGetString(lep, QAJ_queue);
         if (tmp_string != NULL) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->queue <= tmp_length) {
               column_size_data->queue  = tmp_length + 1;
            }
         } 
         /* group */
         tmp_string = lGetString(lep, QAJ_group) ;
         if (tmp_string != NULL) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->group <= tmp_length) {
               column_size_data->group  = tmp_length + 1;
            }
         } 
         /* owner */
         tmp_string = lGetString(lep, QAJ_owner);
         if (tmp_string != NULL) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->owner <= tmp_length) {
               column_size_data->owner  = tmp_length + 1;
            }
         } 
         /* project */
         tmp_string = lGetString(lep, QAJ_project);
         if (tmp_string != NULL) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->project <= tmp_length) {
               column_size_data->project  = tmp_length + 1;
            }
         } 

         /* department  */
         tmp_string = lGetString(lep, QAJ_department);
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->department <= tmp_length) {
               column_size_data->department  = tmp_length + 1;
            }
         } 
         /* granted_pe */
         tmp_string = lGetString(lep, QAJ_granted_pe) ;
         if ( tmp_string != NULL ) {
            tmp_length = strlen(tmp_string);
            if (column_size_data->granted_pe <= tmp_length) {
               column_size_data->granted_pe  = tmp_length + 1;
            }
         } 

         /* slots */
         sprintf(tmp_buf,"%5"sge_fu32, lGetUlong(lep, QAJ_slots));
         tmp_length = strlen(tmp_buf);
         if (column_size_data->slots <= tmp_length) {
            column_size_data->slots  = tmp_length + 1;
         }

         /* advance reservations */
         sprintf(tmp_buf,"%5"sge_fu32, lGetUlong(lep, QAJ_arid));
         tmp_length = strlen(tmp_buf);
         if (column_size_data->arid <= tmp_length) {
            column_size_data->arid = tmp_length + 1;
         }

         lep = lNext(lep);
      }
   } else {
     DPRINTF(("got NULL list\n")); 
   }
   
   DRETURN_VOID;
}


/*
** NAME
**   qacct_usage
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
static void qacct_usage(sge_gdi_ctx_class_t **ctx, FILE *fp)
{
   dstring ds;
   char buffer[256];

   DENTER(TOP_LAYER, "qacct_usage");

   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
         
   fprintf(fp, "%s qacct [options]\n", MSG_HISTORY_USAGE);
   fprintf(fp, " [-ar [ar_id]]                     %s\n", MSG_HISTORY_ar_OPT_USAGE); 
   fprintf(fp, " [-A account_string]               %s\n", MSG_HISTORY_A_OPT_USAGE); 
   fprintf(fp, " [-b begin_time]                   %s\n", MSG_HISTORY_b_OPT_USAGE);
   fprintf(fp, " [-d days]                         %s\n", MSG_HISTORY_d_OPT_USAGE ); 
   fprintf(fp, " [-D [department]]                 %s\n", MSG_HISTORY_D_OPT_USAGE);
   fprintf(fp, " [-e end_time]                     %s\n", MSG_HISTORY_e_OPT_USAGE);
   fprintf(fp, " [-g [groupid|groupname]]          %s\n", MSG_HISTORY_g_OPT_USAGE );
   fprintf(fp, " [-h [host]]                       %s\n", MSG_HISTORY_h_OPT_USAGE );
   fprintf(fp, " [-help]                           %s\n", MSG_HISTORY_help_OPT_USAGE);
   fprintf(fp, " [-j [job_id|job_name|pattern]]    %s\n", MSG_HISTORY_j_OPT_USAGE);
   fprintf(fp, " [-l attr=val,...]                 %s\n", MSG_HISTORY_l_OPT_USAGE );
   fprintf(fp, " [-o [owner]]                      %s\n", MSG_HISTORY_o_OPT_USAGE);
   fprintf(fp, " [-pe [pe_name]]                   %s\n", MSG_HISTORY_pe_OPT_USAGE );
   fprintf(fp, " [-P [project]]                    %s\n", MSG_HISTORY_P_OPT_USAGE );
   fprintf(fp, " [-q [queue]]                      %s\n", MSG_HISTORY_q_OPT_USAGE );
   fprintf(fp, " [-slots [slots]]                  %s\n", MSG_HISTORY_slots_OPT_USAGE);
   fprintf(fp, " [-t taskid[-taskid[:step]]]       %s\n", MSG_HISTORY_t_OPT_USAGE );
   fprintf(fp, " [[-f] acctfile]                   %s\n", MSG_HISTORY_f_OPT_USAGE );
   
   fprintf(fp, "\n");
   fprintf(fp, " begin_time, end_time              %s\n", MSG_HISTORY_beginend_OPT_USAGE );
   fprintf(fp, " queue                             [cluster_queue|queue_instance|queue_domain|pattern]\n");
   if (fp==stderr) {
      SGE_EXIT((void**)ctx, 1);
   } else {
      SGE_EXIT((void**)ctx, 0);   
   }

   DRETURN_VOID;
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
   dstring string = DSTRING_INIT;

   printf("==============================================================\n");
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_QNAME, (dusage->qname ? dusage->qname : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_HOSTNAME, (dusage->hostname ? dusage->hostname : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_GROUP, (dusage->group ? dusage->group : MSG_HISTORY_SHOWJOB_NULL));    
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_OWNER, (dusage->owner ? dusage->owner : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_PROJECT, (dusage->project ? dusage->project : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_DEPARTMENT, (dusage->department ? dusage->department : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_JOBNAME, (dusage->job_name ? dusage->job_name : MSG_HISTORY_SHOWJOB_NULL));
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_JOBNUMBER, dusage->job_number);
   



   if (dusage->task_number) {
      printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_TASKID, dusage->task_number);              /* job-array task number */
   } else {
      printf("%-13.12s%s\n",MSG_HISTORY_SHOWJOB_TASKID, "undefined");             
   }

   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_ACCOUNT, (dusage->account ? dusage->account : MSG_HISTORY_SHOWJOB_NULL ));
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_PRIORITY, dusage->priority);
   printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_QSUBTIME,    sge_ctime32(&dusage->submission_time, &string));

   if (dusage->start_time)
      printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_STARTTIME, sge_ctime32(&dusage->start_time, &string));
   else
      printf("%-13.12s-/-\n",MSG_HISTORY_SHOWJOB_STARTTIME);

   if (dusage->end_time)
      printf("%-13.12s%-20s",MSG_HISTORY_SHOWJOB_ENDTIME, sge_ctime32(&dusage->end_time, &string));
   else
      printf("%-13.12s-/-\n",MSG_HISTORY_SHOWJOB_ENDTIME);


   printf("%-13.12s%-20s\n",MSG_HISTORY_SHOWJOB_GRANTEDPE, dusage->granted_pe);
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_SLOTS, dusage->slots);
   printf("%-13.12s%-3"sge_fu32" %s %s\n",MSG_HISTORY_SHOWJOB_FAILED, dusage->failed, (dusage->failed ? ":" : ""), get_sstate_description(dusage->failed));
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_EXITSTATUS, dusage->exit_status);
   printf("%-13.12s%-13.0f\n",MSG_HISTORY_SHOWJOB_RUWALLCLOCK, dusage->ru_wallclock);  

   printf("%-13.12s%-13.0f\n",MSG_HISTORY_SHOWJOB_RUUTIME, dusage->ru_utime);    /* user time used */
   printf("%-13.12s%-13.0f\n", MSG_HISTORY_SHOWJOB_RUSTIME, dusage->ru_stime);    /* system time used */
      printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUMAXRSS,  dusage->ru_maxrss);     /* maximum resident set size */
      printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUIXRSS,  dusage->ru_ixrss);       /* integral shared text size */
      printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUISMRSS,  dusage->ru_ismrss);     /* integral shared memory size*/
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUIDRSS,      dusage->ru_idrss);      /* integral unshared data "  */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUISRSS,      dusage->ru_isrss);      /* integral unshared stack "  */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUMINFLT,     dusage->ru_minflt);     /* page reclaims */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUMAJFLT,     dusage->ru_majflt);     /* page faults */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUNSWAP,      dusage->ru_nswap);      /* swaps */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUINBLOCK,    dusage->ru_inblock);    /* block input operations */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUOUBLOCK,    dusage->ru_oublock);    /* block output operations */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUMSGSND,     dusage->ru_msgsnd);     /* messages sent */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUMSGRCV,     dusage->ru_msgrcv);     /* messages received */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUNSIGNALS,   dusage->ru_nsignals);   /* signals received */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUNVCSW,      dusage->ru_nvcsw);      /* voluntary context switches */
   printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_RUNIVCSW,     dusage->ru_nivcsw);     /* involuntary */

   printf("%-13.12s%-13.0f\n",   MSG_HISTORY_SHOWJOB_CPU,          dusage->cpu);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_MEM,          dusage->mem);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_IO,           dusage->io);
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_IOW,          dusage->iow);

#if 1
   /* enable this to get unit of memory value (G,M,K) */
   /* CR TODO: create units for complete qacct output: IZ: #1047 */
   sge_dstring_clear(&string);
   double_print_memory_to_dstring(dusage->maxvmem, &string);
   printf("%-13.12s%s\n",        MSG_HISTORY_SHOWJOB_MAXVMEM,      sge_dstring_get_string(&string));
#else
   printf("%-13.12s%-18.3f\n",   MSG_HISTORY_SHOWJOB_MAXVMEM,      dusage->maxvmem);
#endif
   if (dusage->ar != 0) {
      printf("%-13.12s%-20"sge_fu32"\n",MSG_HISTORY_SHOWJOB_ARID, dusage->ar);              /* job-array task number */
   } else {
      printf("%-13.12s%s\n",MSG_HISTORY_SHOWJOB_ARID, "undefined");             
   }
   sge_dstring_free(&string);
}

/*
** NAME
**   get_qacct_lists
** PARAMETER
**   ctx         - communication context
**   alpp        - list pointer-pointer answer list
**   ppcentries  - list pointer-pointer to be set to the complex list, CX_Type, can be NULL
**   ppqueues    - list pointer-pointer to be set to the queues list, QU_Type, has to be set
**   ppexechosts - list pointer-pointer to be set to the exechosts list,EH_Type, can be NULL
**   hgrp_l      - host group list, HGRP_Type, can be NULL
**
** RETURN
**   none
**
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
static bool get_qacct_lists(
sge_gdi_ctx_class_t *ctx,
lList **alpp,
lList **ppcentries,
lList **ppqueues,
lList **ppexechosts,
lList **hgrp_l
) {
   lCondition *where = NULL;
   lEnumeration *what = NULL;
   lList *mal = NULL;
   int ce_id = 0, eh_id = 0, q_id = 0, hgrp_id = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "get_qacct_lists");

   /*
   ** GET SGE_CENTRY_LIST 
   */
   if (ppcentries) {
      what = lWhat("%T(ALL)", CE_Type);
      ce_id = ctx->gdi_multi(ctx, alpp, SGE_GDI_RECORD, SGE_CENTRY_LIST, SGE_GDI_GET,
                             NULL, NULL, what, &state, true);
      lFreeWhat(&what);

      if (answer_list_has_error(alpp)) {
         DRETURN(false);
      }
   }
   /*
   ** GET SGE_EXECHOST_LIST 
   */
   if (ppexechosts) {
      where = lWhere("%T(%I!=%s)", EH_Type, EH_name, SGE_TEMPLATE_NAME);
      what = lWhat("%T(ALL)", EH_Type);
      eh_id = ctx->gdi_multi(ctx, alpp, SGE_GDI_RECORD, SGE_EXECHOST_LIST, SGE_GDI_GET,
                              NULL, where, what, &state, true);
      lFreeWhat(&what);
      lFreeWhere(&where);

      if (answer_list_has_error(alpp)) {
         DRETURN(false);
      }
   }

   /*
   ** hgroup 
   */
   if (hgrp_l) {
      what = lWhat("%T(ALL)", HGRP_Type);
      hgrp_id = ctx->gdi_multi(ctx, alpp, SGE_GDI_RECORD, SGE_HGROUP_LIST, SGE_GDI_GET, 
                           NULL, NULL, what, &state, true);
      lFreeWhat(&what);

      if (answer_list_has_error(alpp)) {
         DRETURN(false);
      }
   }
   /*
   ** GET SGE_QUEUE_LIST 
   */
   what = lWhat("%T(ALL)", QU_Type);
   q_id = ctx->gdi_multi(ctx, alpp, SGE_GDI_SEND, SGE_CQUEUE_LIST, SGE_GDI_GET,
                           NULL, NULL, what, &state, true);
   ctx->gdi_wait(ctx, alpp, &mal, &state);
   lFreeWhat(&what);

   if (answer_list_has_error(alpp)) {
      lFreeList(&mal);
      DRETURN(false);
   }

   /*
   ** handle results
   */
   /* --- complex */
   if (ppcentries) {
      sge_gdi_extract_answer(alpp, SGE_GDI_GET, SGE_CENTRY_LIST, ce_id,
                                   mal, ppcentries);
      if (answer_list_has_error(alpp)) { 
         lFreeList(&mal);
         DRETURN(false);
      }
   }

   /* --- exec host */
   if (ppexechosts) {
      sge_gdi_extract_answer(alpp, SGE_GDI_GET, SGE_EXECHOST_LIST, eh_id, 
                                    mal, ppexechosts);
      if (answer_list_has_error(alpp)) { 
         lFreeList(&mal);
         DRETURN(false);
      }
   }

   /* --- queue */
   if (ppqueues) {
      sge_gdi_extract_answer(alpp, SGE_GDI_GET, SGE_CQUEUE_LIST, q_id, 
                                 mal, ppqueues);
      if (answer_list_has_error(alpp)) { 
         lFreeList(&mal);
         DRETURN(false);
      }
   }

   /* --- hgrp */
   if (hgrp_l) {
      sge_gdi_extract_answer(alpp, SGE_GDI_GET, SGE_HGROUP_LIST, hgrp_id, mal, 
                                   hgrp_l);
      if (answer_list_has_error(alpp)) { 
         lFreeList(&mal);
         DRETURN(false);
      }
   }
   /* --- end */
   lFreeList(&mal);
   DRETURN(true);
}

static void free_qacct_lists(lList **ppcentries, lList **ppqueues, lList **ppexechosts, lList **hgrp_l)
{
   lFreeList(ppcentries);
   lFreeList(ppqueues);
   lFreeList(ppexechosts);
   lFreeList(hgrp_l);
}

static int 
sge_read_rusage(FILE *f, sge_rusage_type *d) 
{
   static char szLine[MAX_STRING_SIZE * 10] = "";
   char  *pc;
   int len;

   DENTER(TOP_LAYER, "sge_read_rusage");

   do {
      pc = fgets(szLine, sizeof(szLine), f);
      if (pc == NULL) { 
         DRETURN(2);
      }   
      len = strlen(szLine);
      if (szLine[len] == '\n') {
         szLine[len] = '\0';
      }   
   } while (len <= 1 || szLine[0] == COMMENT_CHAR); 
   
   /*
    * qname
    */
   pc = strtok(szLine, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->qname = sge_strdup(d->qname, pc);
   
   /*
    * hostname
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->hostname = sge_strdup(d->hostname, pc);

   /*
    * group
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->group = sge_strdup(d->group, pc);
          
           
   /*
    * owner
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->owner = sge_strdup(d->owner, pc);

   /*
    * job_name
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->job_name = sge_strdup(d->job_name, pc);

   /*
    * job_number
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   
   d->job_number = atol(pc);
   
   /*
    * account
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->account = sge_strdup(d->account, pc);

   /*
    * priority
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->priority = atol(pc);

   /*
    * submission_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->submission_time = atol(pc);

   /*
    * start_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->start_time = atol(pc);

   /*
    * end_time
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->end_time = atol(pc);

   /*
    * failed
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->failed = atol(pc);

   /*
    * exit_status
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->exit_status = atol(pc);

   /*
    * ru_wallclock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_wallclock = atol(pc); 

   /*
    * ru_utime
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_utime = atol(pc);

   /*
    * ru_stime
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_stime = atol(pc);

   /*
    * ru_maxrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_maxrss = atol(pc);

   /*
    * ru_ixrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_ixrss = atol(pc);

   /*
    * ru_ismrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_ismrss = atol(pc);

   /*
    * ru_idrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_idrss = atol(pc);

   /*
    * ru_isrss
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_isrss = atol(pc);
   
   /*
    * ru_minflt
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_minflt = atol(pc);

   /*
    * ru_majflt
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_majflt = atol(pc);

   /*
    * ru_nswap
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_nswap = atol(pc);

   /*
    * ru_inblock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_inblock = atol(pc);

   /*
    * ru_oublock
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_oublock = atol(pc);

   /*
    * ru_msgsnd
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_msgsnd = atol(pc);

   /*
    * ru_msgrcv
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_msgrcv = atol(pc);

   /*
    * ru_nsignals
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_nsignals = atol(pc);

   /*
    * ru_nvcsw
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_nvcsw = atol(pc);

   /*
    * ru_nivcsw
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->ru_nivcsw = atol(pc);

   /*
    * project
    */
   pc = strtok(NULL, ":");
   if (!pc) {
      DRETURN(-1);
   }
   d->project = sge_strdup(d->project, pc);

   /*
    * department
    */
   pc = strtok(NULL, ":\n");
   if (!pc) {
      DRETURN(-1);
   }
   d->department = sge_strdup(d->department, pc);

   /* PE name */
   pc = strtok(NULL, ":");
   if (pc) {
      d->granted_pe = sge_strdup(d->granted_pe, pc);
   } else {
      d->granted_pe = sge_strdup(d->granted_pe, "none");   
   }

   /* slots */
   pc = strtok(NULL, ":");
   if (pc) {
      d->slots = atol(pc);
   } else {
      d->slots = 0;
   }

   /* task number */
   pc = strtok(NULL, ":");
   if (pc) {
      d->task_number = atol(pc);
   } else {
      d->task_number = 0;
   }

   d->cpu = ((pc=strtok(NULL, ":")))?atof(pc):0;
   d->mem = ((pc=strtok(NULL, ":")))?atof(pc):0;
   d->io = ((pc=strtok(NULL, ":")))?atof(pc):0;

   /* skip job category */
   pc=strtok(NULL, ":");
#if 0   
   while ((pc=strtok(NULL, ":")) &&
          strlen(pc) &&
          pc[strlen(pc)-1] != ' ' &&
          strcmp(pc, "none")) {
      /*
       * The job category field might contain colons (':').
       * Therefore we have to skip all colons until we find a " :".
       * Only if the category is "none" then ":" is the real delimiter.
       */
      ;
   }
#endif
   d->iow = ((pc=strtok(NULL, ":")))?atof(pc):0;

   /* skip pe_taskid */
   pc=strtok(NULL, ":");

   d->maxvmem = ((pc=strtok(NULL, ":")))?atof(pc):0;
   d->ar = ((pc=strtok(NULL, ":")))?atol(pc):0;

   /* ... */ 

   DRETURN(0);
}

