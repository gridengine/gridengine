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
#include <fnmatch.h>

#include "sgermon.h"
#include "symbols.h"
#include "sge.h"
#include "sge_gdi.h"
#include "sge_time.h"
#include "sge_log.h"
#include "sge_stdlib.h"
#include "sge_all_listsL.h"
#include "commlib.h"
#include "sge_host.h"
#include "slots_used.h"
#include "sge_resource.h"
#include "sig_handlers.h"
#include "sge_sched.h"
#include "cull_sort.h"
#include "usage.h"
#include "sge_feature.h"
#include "parse.h"
#include "sge_prog.h"
#include "sge_parse_num_par.h"
#include "sge_string.h"
#include "show_job.h"
#include "qstat_printing.h"
#include "sge_range.h"
#include "sge_schedd_text.h"
#include "qm_name.h"
#include "load_correction.h"
#include "msg_common.h"
#include "msg_clients_common.h"
#include "msg_qstat.h"
#include "sge_conf.h" 
#include "sgeee.h" 
#include "sge_support.h"
#include "sge_unistd.h"
#include "sge_answer.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_complex.h"
#include "sge_queue.h"

#define FORMAT_I_20 "%I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I %I "
#define FORMAT_I_10 "%I %I %I %I %I %I %I %I %I %I "
#define FORMAT_I_5 "%I %I %I %I %I "
#define FORMAT_I_2 "%I %I "
#define FORMAT_I_1 "%I "

static void get_all_lists(lList **queue_l, lList **job_l, lList **complex_l, lList **exechost_l, lList **sc_l, lList **pe_l, lList **ckpt_l, lList **acl_l, lList **zombie_l, lList *qrl, lList *perl, lList *ul, u_long32 full_listing);
static int select_by_hard_queue_list(lList *queue_list, lList *hql); 
static int select_by_pe_list(lList *queue_list, lList *peref_list, lList *pe_list);
static int select_by_queue_user_list(lList *exechost_list, lList *queue_list, lList *queue_user_list, lList *acl_list);
static lList *sge_parse_cmdline_qstat(char **argv, char **envp, lList **ppcmdline);
static lList *sge_parse_qstat(lList **ppcmdline, lList **pplresource, lList **pplqresource, lList **pplqueueref, lList **ppluser, lList **pplqueue_user, lList **pplpe, u_long32 *pfull, u_long32 *pempty, char **hostname, u_long32 *job_info, u_long32 *group_opt, lList **ppljid);
static int qstat_usage(FILE *fp, char *what);
static int qstat_show_job(lList *jid);
static int qstat_show_job_info(void);

lList *complex_list;
lList *exechost_list = NULL;
lList *sc_config = NULL;

/************************************************************************/

static int qselect_mode = 0;
/*
** Additional options for debugging output
** only available, if env MORE_INFO is set.
*/
static u_long32 global_showjobs = 0;
static u_long32 global_showqueues = 0;

extern char **environ;

int main(int argc, char *argv[]);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int selected;
   lListElem *tmp, *jep, *jatep, *qep, *up, *aep;
   lList *queue_list = NULL, *job_list = NULL, 
         *resource_list = NULL, *qresource_list = NULL, 
         *queueref_list = NULL, *user_list = NULL, 
         *acl_list = NULL, *zombie_list = NULL, *jid_list = NULL, 
         *queue_user_list = NULL, *peref_list = NULL, 
         *pe_list = NULL, *ckpt_list = NULL;
   lList *ce = NULL, *ref_list = NULL, *alp = NULL, *pcmdline = NULL;
   int a_queue_was_selected = 0;
   u_long32 full_listing = QSTAT_DISPLAY_ALL, empty_qs = 0, job_info = 0;
   u_long32 group_opt = 0;
   lList *running_per_user = NULL;
   lSortOrder *so = NULL;
   int nqueues;
   char *hostname = NULL;

   DENTER_MAIN(TOP_LAYER, "qstat");

#if RAND_ERROR
   rand_error = 1;
#endif
   
   sge_gdi_param(SET_MEWHO, QSTAT, NULL);
   if (sge_gdi_setup(prognames[QSTAT], &alp)!=AE_OK) {
      answer_exit_if_not_recoverable(lFirst(alp));
      SGE_EXIT(1);
   }

   sge_setup_sig_handlers(QSTAT);

   alp = sge_parse_cmdline_qstat(argv, environ, &pcmdline);
  
   if(alp) {
      /*
      ** high level parsing error! sow answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      SGE_EXIT(1);
   }

   alp = sge_parse_qstat(&pcmdline, 
      &resource_list,   /* -l resource_request           */
      &qresource_list,  /* -F qresource_request          */
      &queueref_list,   /* -q queue_list                 */
      &user_list,       /* -u user_list - selects jobs   */
      &queue_user_list, /* -U user_list - selects queues */
      &peref_list,      /* -pe pe_list                   */
      &full_listing,    /* -ext                          */
      &empty_qs,        /* -empty                        */
      &hostname,
      &job_info,        /* -j ..                         */
      &group_opt,       /* -g ..                         */
      &jid_list);       /* .. jid_list                   */

   if(alp) {
      /*
      ** low level parsing error! show answer list
      */
      for_each(aep, alp) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
      }
      lFreeList(alp);
      lFreeList(pcmdline);
      lFreeList(ref_list);
      SGE_EXIT(1);
   }

   set_commlib_param(CL_P_TIMEOUT_SRCV, 10*60, NULL, NULL);
   set_commlib_param(CL_P_TIMEOUT_SSND, 10*60, NULL, NULL);

   /* if -j, then only print job info and leave */
   if (job_info) {
      int ret = 0;

      if(lGetNumberOfElem(jid_list)) {
         ret = qstat_show_job(jid_list);
      } else {
         ret = qstat_show_job_info();
      }
      DEXIT;
      SGE_EXIT(ret);
   }

   sge_stopwatch_start(0);


   get_all_lists(
      &queue_list, 
      qselect_mode?NULL:&job_list, 
      &complex_list, 
      &exechost_list, 
      &sc_config, 
      &pe_list,
      &ckpt_list,
      &acl_list,
      &zombie_list,
      queueref_list,
      peref_list,
      user_list,
      full_listing);

   complex_list_init_double_attr(complex_list);

   sge_stopwatch_log(0, "Time for getting all lists");
   
   if (getenv("MORE_INFO")) {
      if(global_showjobs) {
         lWriteListTo(job_list, stdout);
         lFreeList(job_list);
         lFreeList(queue_list);
         SGE_EXIT(0);
         return 0;
      }

      if(global_showqueues) {
         lWriteListTo(queue_list, stdout);
         lFreeList(job_list);
         lFreeList(queue_list);
         SGE_EXIT(0);
         return 0;
      }
   }

   /* 
    *
    * step 1: we tag the queues selected by the user
    *
    *         -q, -U, -pe and -l must be fulfilled by 
    *         the queue if they are given
    *
    */

   DPRINTF(("------- selecting queues -----------\n"));
   /* all queues are selected */
   queue_list_set_tag(queue_list, QUEUE_TAG_IGNORE_TEMPLATE, TAG_SHOW_IT);

   /* unseclect all queues not selected by a -q (if exist) */
   if (lGetNumberOfElem(queueref_list)>0) {
      if ((nqueues=select_by_hard_queue_list(queue_list, queueref_list))<0) {
         SGE_EXIT(1);
      }
      if (nqueues==0) {
         fprintf(stderr, MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S,"-q");
         SGE_EXIT(1);
      }
   }

   /* unseclect all queues not selected by a -U (if exist) */
   if (lGetNumberOfElem(queue_user_list)>0) {
      if ((nqueues=select_by_queue_user_list(exechost_list , queue_list, queue_user_list, acl_list))<0) {
         SGE_EXIT(1);
      }

      if (nqueues==0) {
         fprintf(stderr, MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S,"-U");
         SGE_EXIT(1);
      }
   }

   /* unseclect all queues not selected by a -pe (if exist) */
   if (lGetNumberOfElem(peref_list)>0) {
      if ((nqueues=select_by_pe_list(queue_list, peref_list, pe_list))<0) {
         SGE_EXIT(1);
      }

      if (nqueues==0) {
         fprintf(stderr, MSG_QSTAT_NOQUEUESREMAININGAFTERXQUEUESELECTION_S,"-pe");
         SGE_EXIT(1);
      }
   }

   /* unseclect all queues not selected by a -l (if exist) */
   if (lGetNumberOfElem(resource_list)) {
      lListElem *ep; 
      for_each (ep, resource_list) {
         if (sge_fill_requests(lGetList(ep,RE_entries), complex_list, 1, 1, 0)) {
            /* error message gets written by sge_fill_requests into SGE_EVENT */
            SGE_EXIT(1);
            lFreeList(job_list);
            lFreeList(queue_list);
            return -1;

         }
      }
      /* prepare request */
      for_each(qep, queue_list) {
         lList *ccl[3];
         lListElem *ep;

         /* prepare complex attributes */
         if (!strcmp(lGetString(qep, QU_qname), "template"))
            continue;

         DPRINTF(("matching queue %s with qstat -l\n", lGetString(qep, QU_qname)));
         ce = NULL;

         if (empty_qs)
            set_qs_state(QS_STATE_EMPTY);
         queue_complexes2scheduler(&ce, qep, exechost_list, complex_list, 0);
         ccl[0] = lGetList(host_list_locate(exechost_list, "global"), EH_consumable_config_list);
         ccl[1] = (ep=host_list_locate(exechost_list, lGetHost(qep, QU_qhostname)))?
                  lGetList(ep, EH_consumable_config_list):NULL;
         ccl[2] = lGetList(qep, QU_consumable_config_list);

         selected = sge_select_queue(ce, resource_list, 1, NULL, 0, -1, ccl);

         lFreeList(ce);
         if (empty_qs)
            set_qs_state(QS_STATE_FULL);

         if (!selected)
            lSetUlong(qep, QU_tagged, 0);
      }
   }

   for_each(qep, queue_list) {
      if (lGetUlong(qep, QU_tagged) & TAG_SHOW_IT) {
         a_queue_was_selected = 1;
         break;
      }
   }

   if (!a_queue_was_selected && qselect_mode) {
      fprintf(stderr, MSG_QSTAT_NOQUEUESREMAININGAFTERSELECTION);
      SGE_EXIT(1);
   }

   if (qselect_mode) {
      for_each(qep, queue_list) {
         if ((lGetUlong(qep, QU_tagged) & TAG_SHOW_IT)!=0) {
            printf("%s\n", lGetString(qep, QU_qname));
         }
      }
      SGE_EXIT(0);
   }

   if (!a_queue_was_selected && 
      (full_listing&(QSTAT_DISPLAY_QRESOURCES|QSTAT_DISPLAY_FULL))) {
      SGE_EXIT(0);
   }
     
   /* 
    *
    * step 2: we tag the jobs 
    *
    */

   /* 
   ** all jobs are selected 
   */
   for_each (jep, job_list) {
      for_each (jatep, lGetList(jep, JB_ja_tasks)) {
         lSetUlong(jatep, JAT_suitable, TAG_SHOW_IT);
      }
   }

   /*
   ** tag only jobs which satisfy the user list
   */
   if (lGetNumberOfElem(user_list)) {
      DPRINTF(("------- selecting jobs -----------\n"));

      /* ok, now we untag the jobs if the user_list was specified */ 
      for_each(up, user_list) 
         for_each (jep, job_list) {
            if (up && lGetString(up, STR) && 
                  !fnmatch(lGetString(up, STR), 
                              lGetString(jep, JB_owner), 0)) {
               for_each (jatep, lGetList(jep, JB_ja_tasks)) {
                  lSetUlong(jatep, JAT_suitable, 
                     lGetUlong(jatep, JAT_suitable)|TAG_SHOW_IT|TAG_SELECT_IT);
               }
            }
         }
   }


   if (lGetNumberOfElem(peref_list) || lGetNumberOfElem(queueref_list) || 
         lGetNumberOfElem(resource_list) || lGetNumberOfElem(queue_user_list)) {
      /*
      ** unselect all pending jobs that fit in none of the selected queues
      ** that way the pending jobs are really pending jobs for the queues 
      ** printed
      */

      set_qs_state(QS_STATE_EMPTY);
      for_each(jep, job_list) {
         lListElem *pe, *ckpt;
         int ret, show_job;

         pe = lGetString(jep, JB_pe)?
                  pe_list_locate(pe_list, lGetString(jep, JB_pe)):
                  NULL; /* aargh ! wildcard pe */
         ckpt = lGetString(jep, JB_checkpoint_name)?
                           ckpt_list_locate(ckpt_list, 
                           lGetString(jep, JB_checkpoint_name)): NULL;
         show_job = 0;

         for_each(qep, queue_list) {
            if (!(lGetUlong(qep, QU_tagged) & TAG_SHOW_IT))
               continue;

            ret = available_slots_at_queue(NULL, jep, qep, pe, ckpt, 
                                           exechost_list, complex_list, 
                                           acl_list, NULL, 1, NULL, 0, NULL, 0, NULL);
            if (ret>0) {
               show_job = 1;
               break;
            }
         }   
         
         for_each (jatep, lGetList(jep, JB_ja_tasks)) {
            if (!show_job && !(lGetUlong(jatep, JAT_status) == JRUNNING || (lGetUlong(jatep, JAT_status) == JTRANSFERING))) {
               DPRINTF(("show task "u32"."u32"\n",
                       lGetUlong(jep, JB_job_number),
                       lGetUlong(jatep, JAT_task_number)));
               lSetUlong(jatep, JAT_suitable, lGetUlong(jatep, JAT_suitable) & ~TAG_SHOW_IT);
            }
         }
         if (!show_job) {
            lSetList(jep, JB_ja_n_h_ids, NULL);
            lSetList(jep, JB_ja_u_h_ids, NULL);
            lSetList(jep, JB_ja_o_h_ids, NULL);
            lSetList(jep, JB_ja_s_h_ids, NULL);
         }
      }
      set_qs_state(QS_STATE_FULL);
   }

   /* 
    *
    * step 3: iterate over the queues (and print them)
    *
    *         tag the jobs in these queues with TAG_FOUND_IT
    *         print these jobs if necessary
    *
    */

   correct_capacities(exechost_list, complex_list);
   lPSortList(queue_list, "%I+ %I+", QU_seq_no, QU_qname);
   for_each (qep, queue_list) {
      int print_jobs_of_queue = 0;
      /* here we have the queue */
      if (lGetUlong(qep, QU_tagged) & TAG_SHOW_IT) {
         if ((full_listing & QSTAT_DISPLAY_NOEMPTYQ) && !qslots_used(qep)) {
            continue;
         }
         else {
            sge_print_queue(qep, exechost_list, complex_list, 
                              full_listing, qresource_list);
            print_jobs_of_queue = 1;
         }
      }

      if (shut_me_down) {
         SGE_EXIT(1);
      }

      sge_print_jobs_queue(qep, job_list, pe_list, user_list,
                           exechost_list, complex_list,
                           print_jobs_of_queue, full_listing, "");
   }

   /* 
    *
    * step 3.5: remove all jobs that we found till now 
    *           sort other jobs for printing them in order
    *
    */
   jep = lFirst(job_list);
   while (jep) {
      lList *task_list; 
      lListElem *jatep, *tmp_jatep;

      tmp = lNext(jep);
      task_list = lGetList(jep, JB_ja_tasks);
      jatep = lFirst(task_list);
      while (jatep) {
         tmp_jatep = lNext(jatep);
         if ((lGetUlong(jatep, JAT_suitable) & TAG_FOUND_IT)) {
            lRemoveElem(task_list, jatep);
         }
         jatep = tmp_jatep;
      }
      jep = tmp;
   }

   if (lGetNumberOfElem(job_list)>0 ) {
      if (feature_is_enabled(FEATURE_SGEEE))
         sgeee_sort_jobs(&job_list);
      else {
         so = sge_job_sort_order(lGetListDescr(job_list));
         lSortList(job_list, so);
      }
   }

   /* 
    *
    * step 4: iterate over jobs that are pending;
    *         tag them with TAG_FOUND_IT
    *
    *         print the jobs that run in these queues 
    *
    */
   sge_print_jobs_pending(job_list, user_list, exechost_list, complex_list,
                           &running_per_user, so, full_listing, group_opt);

   /* 
    *
    * step 5:  in case of SGE look for finished jobs and view them as finished 
    *          a non SGE-qstat will show them as error jobs
    *
    */
   sge_print_jobs_finished(job_list, user_list, exechost_list,
                              complex_list, full_listing);

   /* 
    *
    * step 6:  look for jobs not found. This should not happen, cause each 
    *          job is running in a queue, or pending. But if there is 
    *          s.th. wrong we have 
    *          to ensure to print this job just to give hints whats wrong
    *
    */
   sge_print_jobs_error(job_list, user_list, exechost_list, complex_list,
                           full_listing);

   /* 
    *
    * step 7:  print recently finished jobs ('zombies') 
    *
    */
   sge_print_jobs_zombie(zombie_list, user_list, exechost_list,
                           complex_list,  full_listing);

   lFreeList(zombie_list);
   lFreeList(job_list);
   lFreeList(queue_list);
   SGE_EXIT(0);
   return 0;
}

/****
 **** get_all_lists (static)
 ****
 **** Gets copies of queue-, job-, complex-, exechost- , pe- and 
 **** sc_config-list from qmaster.
 **** The lists are stored in the .._l pointerpointer-parameters.
 **** WARNING: Lists previously stored in this pointers are not destroyed!!
 ****/
static void get_all_lists(
lList **queue_l,
lList **job_l,
lList **complex_l,
lList **exechost_l,
lList **sc_l,
lList **pe_l,
lList **ckpt_l,
lList **acl_l,
lList **zombie_l,
lList *queueref_list,
lList *peref_list,
lList *user_list,
u_long32 show 
) {
   lCondition *where= NULL, *nw = NULL, *qw = NULL, *pw = NULL, *jw = NULL, 
              *zw = NULL, *gc_where = NULL;
   lEnumeration *q_all, *pe_all, *ckpt_all, *acl_all, *j_all, *cx_all, *eh_all, 
                *sc_what, *z_all, *gc_what;
   lList *alp = NULL;
   lListElem *aep = NULL;
   lListElem *ep = NULL;
   lList *conf_l = NULL;
   lList *mal = NULL;
   int q_id, j_id = 0, pe_id = 0, ckpt_id = 0, acl_id = 0, z_id = 0, cx_id, eh_id, sc_id, gc_id;
   int show_zombies = 0;
   state_gdi_multi state = STATE_GDI_MULTI_INIT;

   DENTER(TOP_LAYER, "get_all_lists");
  
   qw = lWhere("%T(%I != %s)", QU_Type, QU_qname, SGE_TEMPLATE_NAME);

   q_all = lWhat("%T(ALL)", QU_Type);
   q_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_QUEUE_LIST, SGE_GDI_GET, 
                        NULL, qw, q_all, NULL, &state);
   q_all = lFreeWhat(q_all);
   qw = lFreeWhere(qw);
   
   if (alp) {
      printf("%s\n", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   if (show & QSTAT_DISPLAY_ZOMBIES)
      show_zombies = 1;

   /* 
   ** jobs 
   */ 
   if (job_l) {
      for_each(ep, user_list) {
         nw = lWhere("%T(%I p= %s)", JB_Type, JB_owner, lGetString(ep, STR));
         if (!jw)
            jw = nw;
         else
            jw = lOrWhere(jw, nw);
      }

      if (show & QSTAT_DISPLAY_SUSPENDED) {
         show |= ~(QSTAT_DISPLAY_PENDING|QSTAT_DISPLAY_FINISHED);
      }
      if (show & QSTAT_DISPLAY_FINISHED) {
         show |= ~QSTAT_DISPLAY_PENDING;
      }  
      if (!(show & QSTAT_DISPLAY_RUNNING)) {

         DPRINTF(("==> No running/transiting jobs\n"));

         nw = lWhere("%T(%I->%T(!(%I m= %u || %I m= %u))",
                     JB_Type, JB_ja_tasks, JAT_Type,
                     JAT_status, JRUNNING, JAT_status, JTRANSFERING); 

         if (!jw)
            jw = nw;
         else
            jw = lAndWhere(jw, nw);
      }
      if (!(show & QSTAT_DISPLAY_FINISHED)) {

         DPRINTF(("==> No finished jobs\n"));

         nw = lWhere("%T(%I->%T(!(%I m= %u)))", JB_Type, JB_ja_tasks,
            JAT_Type, JAT_status, JFINISHED);
         if (!jw)
            jw = nw;
         else
            jw = lAndWhere(jw, nw);
      }
      j_all = lWhat("%T("FORMAT_I_20 FORMAT_I_10 FORMAT_I_1")", JB_Type, 
                     JB_job_number, 
                     JB_owner,
                     JB_script_file,
                     JB_group,
                     JB_type,
                     JB_pe,
                     JB_checkpoint_name,
                     JB_jid_predecessor_list,
                     JB_env_list,
                     JB_priority,
                     JB_job_name,
                     JB_project,
                     JB_department,
                     JB_submission_time,
                     JB_deadline,
                     JB_override_tickets,
                     JB_pe_range,
                     JB_hard_resource_list,
                     JB_soft_resource_list,
                     JB_hard_queue_list,
                     JB_soft_queue_list,
                     JB_master_hard_queue_list,
                     JB_ja_structure, 
                     JB_ja_tasks,
                     JB_ja_n_h_ids,
                     JB_ja_u_h_ids,
                     JB_ja_o_h_ids,
                     JB_ja_s_h_ids,
                     JB_ja_z_ids,
                     JB_ja_template,
                     JB_execution_time );

      j_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_JOB_LIST, SGE_GDI_GET, 
                           NULL, jw, j_all, NULL, &state);
      j_all = lFreeWhat(j_all);
      jw = lFreeWhere(jw);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }

   /* 
   ** job zombies 
   */ 
   if (zombie_l && show_zombies) {
      for_each(ep, user_list) {
         nw = lWhere("%T(%I p= %s)", JB_Type, JB_owner, lGetString(ep, STR));
         if (!zw)
            zw = nw;
         else
            zw = lOrWhere(zw, nw);
      }
      z_all = lWhat("%T(" FORMAT_I_20 FORMAT_I_5 FORMAT_I_2 FORMAT_I_2")", JB_Type, 
                     JB_job_number, 
                     JB_owner,
                     JB_group,
                     JB_type,
                     JB_pe,
                     JB_checkpoint_name,
                     JB_jid_predecessor_list,
                     JB_env_list,
                     JB_priority,
                     JB_job_name,
                     JB_project,
                     JB_department,
                     JB_submission_time,
                     JB_deadline,
                     JB_override_tickets,
                     JB_pe_range,
                     JB_hard_resource_list,
                     JB_soft_resource_list,
                     JB_hard_queue_list,
                     JB_soft_queue_list,
                     JB_ja_structure,
                     JB_ja_n_h_ids,
                     JB_ja_u_h_ids,
                     JB_ja_o_h_ids,
                     JB_ja_s_h_ids, 
                     JB_ja_z_ids,
                     JB_ja_template,
                     JB_ja_tasks,
                     JB_execution_time );

      z_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_ZOMBIE_LIST, SGE_GDI_GET, 
                           NULL, zw, z_all, NULL, &state);
      z_all = lFreeWhat(z_all);
      zw = lFreeWhere(zw);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }

   /*
   ** complexes
   */
   cx_all = lWhat("%T(ALL)", CX_Type);
   cx_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_COMPLEX_LIST, SGE_GDI_GET, 
                        NULL, NULL, cx_all, NULL, &state);
   cx_all = lFreeWhat(cx_all);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /*
   ** exechosts
   */
   where = lWhere("%T(%I!=%s)", EH_Type, EH_name, SGE_TEMPLATE_NAME);
   eh_all = lWhat("%T(ALL)", EH_Type);
   eh_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_EXECHOST_LIST, SGE_GDI_GET, 
                        NULL, where, eh_all, NULL, &state);
   eh_all = lFreeWhat(eh_all);
   where = lFreeWhere(where);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /* 
   ** pe list 
   */ 
   if (pe_l) {   
      pe_all = lWhat("%T(%I%I%I)", PE_Type, PE_name, PE_job_is_first_task, PE_control_slaves);
      pe_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_PE_LIST, SGE_GDI_GET,
                           NULL, pw, pe_all, NULL, &state);
      pe_all = lFreeWhat(pe_all);
      pw = lFreeWhere(pw);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }

   /* 
   ** ckpt list 
   */ 
   if (ckpt_l) {
      ckpt_all = lWhat("%T(%I)", CK_Type, CK_name);
      ckpt_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_CKPT_LIST, SGE_GDI_GET, 
                           NULL, NULL, ckpt_all, NULL, &state);
      ckpt_all = lFreeWhat(ckpt_all);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }

   /* 
   ** acl list 
   */ 
   if (acl_l) {
      acl_all = lWhat("%T(ALL)", US_Type);
      acl_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_USERSET_LIST, SGE_GDI_GET, 
                           NULL, NULL, acl_all, NULL, &state);
      acl_all = lFreeWhat(acl_all);

      if (alp) {
         printf("%s", lGetString(lFirst(alp), AN_text));
         SGE_EXIT(1);
      }
   }
   /*
   ** scheduler configuration
   */
   sc_what = lWhat("%T(%I)", SC_Type, SC_user_sort);
   sc_id = sge_gdi_multi(&alp, SGE_GDI_RECORD, SGE_SC_LIST, SGE_GDI_GET, 
                        NULL, NULL, sc_what, NULL, &state);
   sc_what = lFreeWhat(sc_what);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /*
   ** global cluster configuration
   */
   gc_where = lWhere("%T(%I c= %s)", CONF_Type, CONF_hname, SGE_GLOBAL_NAME);
   gc_what = lWhat("%T(ALL)", CONF_Type);
   gc_id = sge_gdi_multi(&alp, SGE_GDI_SEND, SGE_CONFIG_LIST, SGE_GDI_GET,
                        NULL, gc_where, gc_what, &mal, &state);
   gc_what = lFreeWhat(gc_what);
   gc_where = lFreeWhere(gc_where);

   if (alp) {
      printf("%s", lGetString(lFirst(alp), AN_text));
      SGE_EXIT(1);
   }

   /*
   ** handle results
   */
   /* --- queue */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_QUEUE_LIST, q_id, 
                                 mal, queue_l);
   if (!alp) {
      printf(MSG_GDI_QUEUESGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- job */
   if (job_l) {
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_JOB_LIST, j_id, mal, job_l);
      if (!alp) {
         printf(MSG_GDI_JOBSGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      alp = lFreeList(alp);
   }

   /* --- job zombies */
   if (zombie_l && show_zombies) {
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_ZOMBIE_LIST, z_id, mal,
         zombie_l);
      if (!alp) {
         printf(MSG_GDI_JOBZOMBIESSGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      alp = lFreeList(alp);
   }

   /* --- complex */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_COMPLEX_LIST, cx_id,
                                 mal, complex_l);
   if (!alp) {
      printf(MSG_GDI_COMPLEXSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- exec host */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_EXECHOST_LIST, eh_id, 
                                 mal, exechost_l);
   if (!alp) {
      printf(MSG_GDI_EXECHOSTSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   alp = lFreeList(alp);

   /* --- pe */
   if (pe_l) {
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_PE_LIST, pe_id, 
                                    mal, pe_l);
      if (!alp) {
         printf(MSG_GDI_PESGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      alp = lFreeList(alp);
   }

   /* --- ckpt */
   if (ckpt_l) {
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CKPT_LIST, ckpt_id, 
                                    mal, ckpt_l);
      if (!alp) {
         printf(MSG_GDI_CKPTSGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      alp = lFreeList(alp);
   }

   /* --- acl */
   if (acl_l) {
      alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_USERSET_LIST, acl_id, 
                                    mal, acl_l);
      if (!alp) {
         printf(MSG_GDI_USERSETSGEGDIFAILED);
         SGE_EXIT(1);
      }
      if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
         printf("%s", lGetString(aep, AN_text));
         SGE_EXIT(1);
      }
      alp = lFreeList(alp);
   }

   /* --- scheduler configuration */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_SC_LIST, sc_id, mal, sc_l);
   if (!alp) {
      printf(MSG_GDI_SCHEDDCONFIGSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   if (sc_config) {
      if (lGetBool(lFirst(*sc_l), SC_user_sort)) {
         set_user_sort(1);
      }
   }
   alp = lFreeList(alp);

   /* -- apply global configuration for sge_hostcmp() scheme */
   alp = sge_gdi_extract_answer(SGE_GDI_GET, SGE_CONFIG_LIST, gc_id, mal, &conf_l);
   if (!alp) {
      printf(MSG_GDI_SCHEDDCONFIGSGEGDIFAILED);
      SGE_EXIT(1);
   }
   if (lGetUlong(aep=lFirst(alp), AN_status) != STATUS_OK) {
      printf("%s", lGetString(aep, AN_text));
      SGE_EXIT(1);
   }
   if (lFirst(conf_l)) {
      lListElem *local = NULL;
      merge_configuration(lFirst(conf_l), local, &conf, NULL);
   }
   alp = lFreeList(alp);

   mal = lFreeList(mal);

   DEXIT;
   return;
}

static int select_by_hard_queue_list(
lList *queue_list,
lList *hql 
) {
   int nqueues = 0;
   const char *qrnm;
   lListElem *qrep, *qep;
   int need_hit;

   DENTER(TOP_LAYER, "select_by_hard_queue_list");

   for_each(qrep, hql) {
      qrnm = lGetString(qrep, QR_name);
      need_hit = !sge_is_pattern(qrnm);
      for_each (qep, queue_list) {
         if (!fnmatch(qrnm, lGetString(qep, QU_qname), 0)) {
            lSetUlong(qep, QU_tagged, lGetUlong(qep, QU_tagged) | TAG_SELECT_IT);
            need_hit = 0;
         }
      }

      if (need_hit) {
         fprintf(stderr, MSG_QUEUE_UNKNOWNQUEUEX_S, qrnm);
         return -1;
      }   
   }
   
   /* untag not selected queues */
   for_each (qep, queue_list) {
      lUlong tag;
      if (((tag=lGetUlong(qep, QU_tagged))&TAG_SELECT_IT)==0)
         lSetUlong(qep, QU_tagged, tag & ~(TAG_SELECT_IT|TAG_SHOW_IT));
      else 
         nqueues++;
   }
   
   DEXIT;
   return nqueues;
}

/* 
   untag all queues not selected by a -pe

   returns 
      0 ok
      -1 error 

*/
static int select_by_pe_list(
lList *queue_list,
lList *peref_list,   /* STR_Type */
lList *pe_list 
) {
   int nqueues = 0;
   lList *pe_selected = NULL;
   lListElem *pe, *qep;

   DENTER(TOP_LAYER, "select_by_pe_list");

  /*
   * iterate through peref_list and build up a new pe_list
   * containing only those pe's referenced in peref_list
   */
   for_each(pe, peref_list) {
      lListElem *ref_pe;   /* PE_Type */
      lListElem *copy_pe;  /* PE_Type */

      ref_pe = pe_list_locate(pe_list, lGetString(pe, STR));
      copy_pe = lCopyElem(ref_pe);
      if (pe_selected == NULL) {
         const lDescr *descriptor = lGetElemDescr(ref_pe);

         pe_selected = lCreateList("", descriptor);
      }
      lAppendElem(pe_selected, copy_pe);
   }
   if (lGetNumberOfElem(pe_selected)==0) {
      fprintf(stderr, MSG_PE_NOSUCHPARALLELENVIRONMENT);
      return -1;
   }

   /* 
    * untag all non-parallel queues and queues not referenced 
    * by a pe in the selected pe list entry of a queue 
    */
   for_each(qep, queue_list) {
      lListElem* found = NULL;

      if (!queue_is_parallel_queue(qep)) {
         lSetUlong(qep, QU_tagged, 0);
         continue;
      }
      for_each (pe, pe_selected) {
         const char *pe_name = lGetString(pe, PE_name);

         found = lGetSubStr(qep, STR, pe_name, QU_pe_list);
         if (found != NULL) {
            break;
         }
      }
      if (found == NULL) {
         lSetUlong(qep, QU_tagged, 0);
      } else {
         nqueues++;
      }
   }

   if (pe_selected != NULL) {
      lFreeList(pe_selected);
   }
   DEXIT;
   return nqueues;
}

/* 
   untag all queues not selected by a -pe

   returns 
      0 ok
      -1 error 

*/
static int select_by_queue_user_list(
lList *exechost_list,
lList *queue_list,
lList *queue_user_list,
lList *acl_list 
) {
   int nqueues = 0;
   lListElem *qu = NULL;
   lListElem *qep = NULL;
   lListElem *ehep = NULL;
   lList *h_acl = NULL;
   lList *h_xacl = NULL;
   lList *global_acl = NULL;
   lList *global_xacl = NULL;
   lList *config_acl = NULL;
   lList *config_xacl = NULL;
 

   DENTER(TOP_LAYER, "select_by_queue_user_list");

   /* untag all queues where no of the users has access */

   ehep = host_list_locate(exechost_list, "global"); 
   global_acl  = lGetList(ehep, EH_acl);
   global_xacl = lGetList(ehep, EH_xacl);

   config_acl  = conf.user_lists;
   config_xacl = conf.xuser_lists; 


   for_each(qep, queue_list) {
      int access = 0;
      const char *host_name = NULL;

      /* get exec host list element for current queue 
         and its access lists */
      host_name = lGetHost(qep,QU_qhostname);
      ehep = host_list_locate(exechost_list, host_name); 
      h_acl  = lGetList(ehep, EH_acl);
      h_xacl = lGetList(ehep, EH_xacl);

      for_each (qu, queue_user_list) {
         int q_access = 0;
         int h_access = 0;
         int gh_access = 0;
         int conf_access = 0;

         const char *name = lGetString(qu, STR);
         if (name == NULL)
            continue;

         DPRINTF(("-----> checking queue user: %s\n", name )); 

         DPRINTF(("testing queue access lists\n"));
         q_access = (name[0]=='@')?
               sge_has_access(NULL, &name[1], qep, acl_list): 
               sge_has_access(name, NULL, qep, acl_list); 
         if (!q_access) {
            DPRINTF(("no access\n"));
         } else {
            DPRINTF(("ok\n"));
         }

         DPRINTF(("testing host access lists\n"));
         h_access = (name[0]=='@')?
               sge_has_access_(NULL, &name[1], h_acl, h_xacl , acl_list):
               sge_has_access_(name, NULL, h_acl, h_xacl , acl_list); 
         if (!h_access) {
            DPRINTF(("no access\n"));
         }else {
            DPRINTF(("ok\n"));
         }

         DPRINTF(("testing global host access lists\n"));
         gh_access = (name[0]=='@')?
               sge_has_access_(NULL, &name[1], global_acl , global_xacl , acl_list):
               sge_has_access_(name, NULL,global_acl , global_xacl , acl_list);
         if (!gh_access) {
            DPRINTF(("no access\n"));
         }else {
            DPRINTF(("ok\n"));
         }

         DPRINTF(("testing cluster config access lists\n"));
         conf_access = (name[0]=='@')?
               sge_has_access_(NULL, &name[1],config_acl , config_xacl , acl_list): 
               sge_has_access_(name, NULL, config_acl , config_xacl  , acl_list); 
         if (!conf_access) {
            DPRINTF(("no access\n"));
         }else {
            DPRINTF(("ok\n"));
         }

         access = q_access && h_access && gh_access && conf_access;
         if (!access) {
            break;
         }
      }
      if (!access) {
         DPRINTF(("no access for queue %s\n", lGetString(qep,QU_qname) ));
         lSetUlong(qep, QU_tagged, 0);
      }
      else {
         DPRINTF(("access for queue %s\n", lGetString(qep,QU_qname) ));
         nqueues++;
      }
   }

   DEXIT;
   return nqueues;
}

/****
 **** sge_parse_cmdline_qstat (static)
 ****
 **** 'stage 1' parsing of qstat-options. Parses options
 **** with their arguments and stores them in ppcmdline.
 ****/ 
static lList *sge_parse_cmdline_qstat(
char **argv,
char **envp,
lList **ppcmdline 
) {
char **sp;
char **rp;
stringT str;
lList *alp = NULL;
 
   DENTER(TOP_LAYER, "sge_parse_cmdline_qstat");

   if (!strcmp(sge_basename(*argv++, '/'), "qselect"))
      qselect_mode = 1;

   rp = argv;
   while(*(sp=rp)) {
      /* -help */
      if ((rp = parse_noopt(sp, "-help", NULL, ppcmdline, &alp)) != sp)
         continue;
 
      /* -alarm option */
      if (!qselect_mode && (rp = parse_noopt(sp, "-alarm", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -f option */
      if (!qselect_mode && (rp = parse_noopt(sp, "-f", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -F */
      if (!qselect_mode && (rp = parse_until_next_opt2(sp, "-F", NULL, ppcmdline, &alp)) != sp)
         continue;

      if (!qselect_mode && feature_is_enabled(FEATURE_SGEEE)) {
         /* -ext option */
         if ((rp = parse_noopt(sp, "-ext", NULL, ppcmdline, &alp)) != sp)
            continue;
      }

      /* -g */
      if (!qselect_mode && (rp = parse_until_next_opt(sp, "-g", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -j [jid {,jid}]*/
      if (!qselect_mode && (rp = parse_until_next_opt2(sp, "-j", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -l */
      if ((rp = parse_until_next_opt(sp, "-l", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -ne option */
      if (!qselect_mode && (rp = parse_noopt(sp, "-ne", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -s [p|r|s|h|...] option */
      if (!qselect_mode && (rp = parse_until_next_opt(sp, "-s", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -q */
      if ((rp = parse_until_next_opt(sp, "-q", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -r */
      if (!qselect_mode && (rp = parse_noopt(sp, "-r", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -t */
      if (!qselect_mode && (rp = parse_noopt(sp, "-t", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -u */
      if (!qselect_mode && (rp = parse_until_next_opt(sp, "-u", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -U */
      if ((rp = parse_until_next_opt(sp, "-U", NULL, ppcmdline, &alp)) != sp)
         continue;

      /* -pe */
      if ((rp = parse_until_next_opt(sp, "-pe", NULL, ppcmdline, &alp)) != sp)
         continue;

      /*
      ** Two additional flags only if MORE_INFO is set:
      ** -dj   dump jobs:  displays full global_job_list 
      ** -dq   dump queue: displays full global_queue_list
      */
      if (getenv("MORE_INFO")) {
         /* -dj */
         if ((rp = parse_noopt(sp, "-dj", NULL, ppcmdline, &alp)) != sp)
            continue;

         /* -dq */
         if ((rp = parse_noopt(sp, "-dq", NULL, ppcmdline, &alp)) != sp)
            continue;
      }

      /* oops */
      sprintf(str, MSG_ANSWER_INVALIDOPTIONARGX_S, *sp);
      qstat_usage(stderr, NULL);
      answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
      DEXIT;
      return alp;
   }
   DEXIT;
   return alp;
}

/****
 **** sge_parse_qstat (static)
 ****
 **** 'stage 2' parsing of qstat-options. Gets the options from
 **** ppcmdline, sets the full and empry_qs flags and puts the
 **** queue/res/user-arguments into the lists.
 ****/
static lList *sge_parse_qstat(
lList **ppcmdline,
lList **pplresource,
lList **pplqresource,
lList **pplqueueref,
lList **ppluser,
lList **pplqueue_user,
lList **pplpe,
u_long32 *pfull,
u_long32 *pempty_qs,
char **hostname,
u_long32 *job_info,
u_long32 *group_opt,
lList **ppljid 
) {
   stringT str;
   lList *alp = NULL;
   u_long32 helpflag;
   int usageshowed = 0;
   char *argstr;
   u_long32 full = 0;
   lList *plstringopt = NULL; 


   DENTER(TOP_LAYER, "sge_parse_qstat");

   /* Loop over all options. Only valid options can be in the
      ppcmdline list. 
   */
   while(lGetNumberOfElem(*ppcmdline))
   {
      if(parse_flag(ppcmdline, "-help",  &alp, &helpflag)) {
         usageshowed = qstat_usage(stdout, NULL);
         DEXIT;
         SGE_EXIT(0);
         break;
      }
     
      if(parse_flag(ppcmdline, "-alarm", &alp, &full)) {
         if(full) {
            (*pfull) |= QSTAT_DISPLAY_ALARMREASON | QSTAT_DISPLAY_FULL;
         }
         continue;
      }

      if (parse_multi_stringlist(ppcmdline, "-j", &alp, ppljid, ST_Type, STR)) {
         *job_info = 1;
         continue;
      }

      /*
      ** Two additional flags only if MORE_INFO is set:
      ** -dj   dump jobs:  displays full global_job_list 
      ** -dq   dump queue: displays full global_queue_list
      */
      if (getenv("MORE_INFO")) {
         if(parse_flag(ppcmdline, "-dj", &alp, &global_showjobs))
            break;
         
         if(parse_flag(ppcmdline, "-dq", &alp, &global_showqueues))
            break;
      }

      if(parse_flag(ppcmdline, "-ne", &alp, &full)) {
         if(full) {
            (*pfull) |= QSTAT_DISPLAY_NOEMPTYQ;
            full = 0;
         }
         continue;
      }

      if(parse_flag(ppcmdline, "-f", &alp, &full)) {
         if(full) {
            (*pfull) |= QSTAT_DISPLAY_FULL;
            full = 0;
         }
         continue;
      }
      if (parse_string(ppcmdline, "-s", &alp, &argstr)) {
         if (argstr) {
            static char noflag = '$';
            static char* flags[] = {
               "hu", "hs", "ho", "hj", "ha", "h", "p", "r", "s", "z", NULL
            };
            static u_long32 bits[] = {
               (QSTAT_DISPLAY_USERHOLD|QSTAT_DISPLAY_PENDING), 
               (QSTAT_DISPLAY_SYSTEMHOLD|QSTAT_DISPLAY_PENDING), 
               (QSTAT_DISPLAY_OPERATORHOLD|QSTAT_DISPLAY_PENDING), 
               (QSTAT_DISPLAY_JOBHOLD|QSTAT_DISPLAY_PENDING), 
               (QSTAT_DISPLAY_STARTTIMEHOLD|QSTAT_DISPLAY_PENDING), 
               (QSTAT_DISPLAY_HOLD|QSTAT_DISPLAY_PENDING), 
               QSTAT_DISPLAY_PENDING, 
               QSTAT_DISPLAY_RUNNING, 
               QSTAT_DISPLAY_SUSPENDED, 
               QSTAT_DISPLAY_ZOMBIES
            };
            int i, j;
            char *s_switch;
            u_long32 rm_bits = 0;
            
            /* initialize bitmask */
            for (j=0; flags[j]; j++) 
               rm_bits |= bits[j];
            (*pfull) &= ~rm_bits;
            
            /* 
            ** search each 'flag' in argstr
            ** if we find the whole string we will set the corresponding bits in '*pfull'
            */
            for (i=0, s_switch=flags[i]; s_switch != NULL; i++, s_switch=flags[i]) {
               for (j=0; argstr[j]; j++) { 
                  if ((argstr[j] == flags[i][0] && argstr[j] != noflag)) {
                     if ((strlen(flags[i]) == 2) && argstr[j+1] && (argstr[j+1] == flags[i][1])) {
                        argstr[j] = noflag;
                        argstr[j+1] = noflag;
                        (*pfull) |= bits[i];
                        break;
                     } else if ((strlen(flags[i]) == 1)){
                        argstr[j] = noflag;
                        (*pfull) |= bits[i];
                        break;
                     }
                  } 
               }
            } 

            /* search for invalid options */
            for (j=0; argstr[j]; j++) {
               if (argstr[j] != noflag) {
                  sprintf(str, MSG_OPTIONS_WRONGARGUMENTTOSOPT);
                  if (!usageshowed)
                     qstat_usage(stderr, NULL);
                  answer_list_add(&alp, str, 
                                  STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
                  DEXIT;
                  return alp;
               }  
            }
         } 
         continue;
      }

      if(parse_string(ppcmdline, "-F", &alp, &argstr)) {
         (*pfull) |= QSTAT_DISPLAY_QRESOURCES|QSTAT_DISPLAY_FULL;
         if (argstr) {
            *pplqresource = sge_parse_resources(*pplqresource, NULL, argstr, "hard");
            FREE(argstr);
         }
         continue;
      }

      if (feature_is_enabled(FEATURE_SGEEE)) {
         if(parse_flag(ppcmdline, "-ext", &alp, &full)) {
            if(full) {
               (*pfull) |= QSTAT_DISPLAY_EXTENDED;
               full = 0;
            }
            continue;
         }
      }

      if(parse_flag(ppcmdline, "-r", &alp, &full)) {
         if(full) {
            (*pfull) |= QSTAT_DISPLAY_RESOURCES;
            full = 0;
         }
         continue;
      }

      if(parse_flag(ppcmdline, "-t", &alp, &full)) {
         if(full) {
            (*pfull) |= QSTAT_DISPLAY_TASKS;
            full = 0;
         }
         continue;
      }

      if(parse_string(ppcmdline, "-l", &alp, &argstr)) {
         *pplresource = sge_parse_resources(*pplresource, NULL, argstr, "hard");
         FREE(argstr);
         continue;
      }

      if(parse_multi_stringlist(ppcmdline, "-u", &alp, ppluser, ST_Type, STR)) 
         continue;
      
      if(parse_multi_stringlist(ppcmdline, "-U", &alp, pplqueue_user, ST_Type, STR)) 
         continue;
      
      if(parse_multi_stringlist(ppcmdline, "-pe", &alp, pplpe, ST_Type, STR)) 
         continue;
      
      if(parse_multi_stringlist(ppcmdline, "-q", &alp, pplqueueref, QR_Type, QR_name))
         continue;

      if(parse_multi_stringlist(ppcmdline, "-g", &alp, &plstringopt, ST_Type, STR)) {
         *group_opt = parse_group_options(plstringopt);
         
         lFreeList(plstringopt);    
         continue;
      }
   }
   if(lGetNumberOfElem(*ppcmdline)) {
     sprintf(str, MSG_PARSE_TOOMANYOPTIONS);
     if (!usageshowed)
        qstat_usage(stderr, NULL);
     answer_list_add(&alp, str, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
     DEXIT;
     return alp;
   }
   DEXIT;
   return alp;
}

/****
 **** qstat_usage (static)
 ****
 **** displays usage of qstat on file fp.
 **** Is what NULL, full usage will be displayed.
 ****
 **** Returns always 1.
 ****
 **** If what is a pointer to an option-string,
 **** only usage for that option will be displayed.
 ****   ** not implemented yet! **
 ****/
static int qstat_usage(
FILE *fp, 
char *what 
) {
   dstring ds;
   char buffer[256];
   
   sge_dstring_init(&ds, buffer, sizeof(buffer));

   fprintf(fp, "%s\n", feature_get_product_name(FS_SHORT_VERSION, &ds));
 
   if(!what) {
      /* display full usage */
      fprintf(fp, "%s %s [options]\n", MSG_SRC_USAGE ,qselect_mode?"qselect":"qstat");
      if (!qselect_mode) 
         fprintf(fp, "        [-alarm]                        %s",MSG_QSTAT_USAGE_SHOWREASONFORQUEUEALARMSTATE);
      if (!qselect_mode && (feature_is_enabled(FEATURE_SGEEE))) {
         fprintf(fp, "        [-ext]                          %s",MSG_QSTAT_USAGE_VIEWALSOSCHEDULINGATTRIBUTES);
      }
      if (!qselect_mode) 
         fprintf(fp, "        [-f]                            %s",MSG_QSTAT_USAGE_FULLOUTPUT);
      if (!qselect_mode) 
         fprintf(fp, "        [-F [resource_attributes]]      %s",MSG_QSTAT_USAGE_FULLOUTPUTANDSHOWRESOURCESOFQUEUES);
      if (!qselect_mode) 
         fprintf(fp, "        [-g {d}]                        %s",MSG_QSTAT_USAGE_DISPLAYALLJOBARRAYTASKS);
      fprintf(fp, "        [-help]                         %s",MSG_QSTAT_USAGE_PRINTTHISHELP);
      if (!qselect_mode) 
      if (!qselect_mode) 
         fprintf(fp, "        [-j job_list ]                  %s",MSG_QSTAT_USAGE_SHOWSCHEDULERJOBINFO);
      fprintf(fp, "        [-l resource_list]              %s",MSG_QSTAT_USAGE_REQUESTTHEGIVENRESOURCES);
      if (!qselect_mode) 
         fprintf(fp, "        [-ne]                           %s",MSG_QSTAT_USAGE_HIDEEMPTYQUEUES);
      fprintf(fp, "        [-pe pe_list]                   %s",MSG_QSTAT_USAGE_SELECTONLYQUEESWITHONOFTHESEPE);
      fprintf(fp, "        [-q destin_id_list]             %s",MSG_QSTAT_USAGE_PRINTINFOONGIVENQUEUE);
      if (!qselect_mode) 
         fprintf(fp, "        [-r]                            %s",MSG_QSTAT_USAGE_SHOWREQUESTEDRESOURCESOFJOB);
      if (!qselect_mode) {
         fprintf(fp, "        [-s {p|r|s|z|hu|ho|hs|hj|ha|h}] %s",MSG_QSTAT_USAGE_SHOWPENDINGRUNNINGSUSPENDESZOMBIEJOBS);
         fprintf(fp, "                                        %s",MSG_QSTAT_USAGE_JOBSWITHAUSEROPERATORSYSTEMHOLD);
         fprintf(fp, "                                        %s",MSG_QSTAT_USAGE_JOBSWITHSTARTTIMEINFUTORE);
         fprintf(fp, "                                        %s",MSG_QSTAT_USAGE_HISABBREVIATIONFORHUHOHSHJHA);
      }
      if (!qselect_mode) 
         fprintf(fp, "        [-t]                            %s",MSG_QSTAT_USAGE_SHOWTASKINFO);
      if (!qselect_mode)  
         fprintf(fp, "        [-u user_list]                  %s",MSG_QSTAT_USAGE_VIEWONLYJOBSOFTHISUSER);
      fprintf(fp, "        [-U user_list]                  %s",MSG_QSTAT_USAGE_SELECTQUEUESWHEREUSERXHAVEACCESS);

      if (getenv("MORE_INFO")) {
         fprintf(fp, MSG_QSTAT_USAGE_ADDITIONALDEBUGGINGOPTIONS);
         fprintf(fp, "        [-dj]                           %s",MSG_QSTAT_USAGE_DUMPCOMPLETEJOBLISTTOSTDOUT);
         fprintf(fp, "        [-dq]                           %s",MSG_QSTAT_USAGE_DUMPCOMPLETEQUEUELISTTOSTDOUT);
      }

      fprintf(fp, "destin_id_list           queue[,queue,...]\n");
      fprintf(fp, "pe_list                  pe[,pe,...]\n");
      fprintf(fp, "resource_list            resource[=value][,resource[=value],.  ..]\n");
      fprintf(fp, "user_list                user|@group[,user|@group],...]\n");
      fprintf(fp, "resource_attributes      resource,resource,.\n");
   } else {
      /* display option usage */
      fprintf(fp, MSG_QDEL_not_available_OPT_USAGE_S,what);
   }
   return 1;
}

/*
** qstat_show_job
** displays information about a given job
** to be extended
**
** returns 0 on success, non-zero on failure
*/
static int qstat_show_job(
lList *jid_list 
) {
   lListElem *j_elem = 0;
   lList* jlp = NULL;
   lList* ilp = NULL;
   lListElem* aep = NULL;
   lCondition *where = NULL, *newcp = NULL;
   lEnumeration* what = NULL;
   lList* alp = NULL;
   bool schedd_info = true;
   bool jobs_exist = true;
   int line_separator=0;
   lListElem* mes;

   DENTER(TOP_LAYER, "qstat_show_job");

   /* get job scheduling information */
   what = lWhat("%T(ALL)", SME_Type);
   alp = sge_gdi(SGE_JOB_SCHEDD_INFO, SGE_GDI_GET, &ilp, NULL, what);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         schedd_info = false;
      }
   }
   lFreeList(alp);

   /* build 'where' for all jobs */
   where = NULL;
   for_each(j_elem, jid_list) {
      u_long32 jid = atol(lGetString(j_elem, STR));

      newcp = lWhere("%T(%I==%u)", JB_Type, JB_job_number, jid);
      if (!where)
         where = newcp;
      else
         where = lOrWhere(where, newcp);
   }
   what = lWhat("%T(ALL)", JB_Type);
   /* get job list */
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_GET, &jlp, where, what);
   lFreeWhere(where);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         jobs_exist = false;
      }
   }
   lFreeList(alp);
   if(!jobs_exist) {
      DEXIT;
      return 1;
   }

   /* does jlp contain all information we requested? */
   if (lGetNumberOfElem(jlp) != lGetNumberOfElem(jid_list)) {
      lListElem *elem1, *elem2;
      int first_time = 1;

      for_each(elem1, jlp) {
         char buffer[256];
            
         sprintf(buffer, U32CFormat, u32c(lGetUlong(elem1, JB_job_number)));   
         elem2 = lGetElemStr(jid_list, STR, buffer);       
         
         if (elem2) {
            lDechainElem(jid_list, elem2);
            lFreeElem(elem2);
         }    
      }
      fprintf(stderr, MSG_QSTAT_FOLLOWINGDONOTEXIST);
      for_each(elem1, jid_list) {
         if (!first_time) {
            fprintf(stderr, ", "); 
         }
         first_time = 0;
         fprintf(stderr, "%s", lGetString(elem1, STR));
      }
      fprintf(stderr, "\n");
      DEXIT;
      SGE_EXIT(1);
   }

   /* print scheduler job information and global scheduler info */
   for_each (j_elem, jlp) {
      u_long32 jid = lGetUlong(j_elem, JB_job_number);
      lListElem *sme;

      if (line_separator)
         printf("\n");
      else
         line_separator = 1;
      cull_show_job(j_elem, 0);
      if (schedd_info && (sme = lFirst(ilp))) {
         int first_run = 1;

         if (sme) {
            /* global schduling info */
            for_each (mes, lGetList(sme, SME_global_message_list)) {
               if (first_run) {
                  printf("%s:            ",MSG_SCHEDD_SCHEDULINGINFO);
                  first_run = 0;
               }
               else
                  printf("%s", "                            ");
               printf("%s\n", lGetString(mes, MES_message));
            }

            /* job scheduling info */
            for_each(mes, lGetList(sme, SME_message_list)) {
               lListElem *mes_jid;

               for_each(mes_jid, lGetList(mes, MES_job_number_list)) {
                  if (lGetUlong(mes_jid, ULNG) == jid) {
                     if (first_run) {
                        printf("%s:            ",MSG_SCHEDD_SCHEDULINGINFO);
                        first_run = 0;
                     } else
                        printf("%s", "                            ");
                     printf("%s\n", lGetString(mes, MES_message));
                  }
               }
            }
         }
      }
   }

   lFreeList(ilp);
   lFreeList(jlp);
   DEXIT;
   return 0;
}

static int qstat_show_job_info()
{
   lList *ilp = NULL, *mlp = NULL;
   lListElem* aep = NULL;
   lEnumeration* what = NULL;
   lList* alp = NULL;
   bool schedd_info = true;
   lListElem* mes;
   int initialized = 0;
   u_long32 last_jid = 0;
   u_long32 last_mid = 0;
   char text[256], ltext[256];
   int ids_per_line = 0;
   int first_run = 1;
   int first_row = 1;
   lListElem *sme;
   lListElem *jid_ulng = NULL; 

   DENTER(TOP_LAYER, "qstat_show_job");

   /* get job scheduling information */
   what = lWhat("%T(ALL)", SME_Type);
   alp = sge_gdi(SGE_JOB_SCHEDD_INFO, SGE_GDI_GET, &ilp, NULL, what);
   lFreeWhat(what);
   for_each(aep, alp) {
      if (lGetUlong(aep, AN_status) != STATUS_OK) {
         fprintf(stderr, "%s", lGetString(aep, AN_text));
         schedd_info = false;
      }
   }
   lFreeList(alp);
   if (!schedd_info) {
      DEXIT;
      return 1;
   }

   sme = lFirst(ilp);
   if (sme) {
      /* print global schduling info */
      first_run = 1;
      for_each (mes, lGetList(sme, SME_global_message_list)) {
         if (first_run) {
            printf("%s:            ",MSG_SCHEDD_SCHEDULINGINFO);
            first_run = 0;
         }
         else
            printf("%s", "                            ");
         printf("%s\n", lGetString(mes, MES_message));
      }
      if (!first_run)
         printf("\n");

      first_run = 1;

      mlp = lGetList(sme, SME_message_list);
      lPSortList (mlp, "I+", MES_message_number);

      /* 
       * Remove all jids which have more than one entry for a MES_message_number
       * After this step the MES_messages are not correct anymore
       * We do not need this messages for the summary output
       */
      {
         lListElem *flt_msg, *flt_nxt_msg;
         lList *new_list;
         lListElem *ref_msg, *ref_jid;

         new_list = lCreateList("filtered message list", MES_Type);

         flt_nxt_msg = lFirst(mlp);
         while ((flt_msg = flt_nxt_msg)) {
            lListElem *flt_jid, * flt_nxt_jid;
            int found_msg, found_jid;

            flt_nxt_msg = lNext(flt_msg);
            found_msg = 0;
            for_each(ref_msg, new_list) {
               if (lGetUlong(ref_msg, MES_message_number) == 
                   lGetUlong(flt_msg, MES_message_number)) {
                 
                  flt_nxt_jid = lFirst(lGetList(flt_msg, MES_job_number_list));
                  while ((flt_jid = flt_nxt_jid)) {
                     flt_nxt_jid = lNext(flt_jid);
                    
                     found_jid = 0; 
                     for_each(ref_jid, lGetList(ref_msg, MES_job_number_list)) {
                        if (lGetUlong(ref_jid, ULNG) == 
                            lGetUlong(flt_jid, ULNG)) {
                           lRemoveElem(lGetList(flt_msg, MES_job_number_list), flt_jid);
                           found_jid = 1;
                        }
                     }
                     if (!found_jid) { 
                        lDechainElem(lGetList(flt_msg, MES_job_number_list), flt_jid);
                        lAppendElem(lGetList(ref_msg, MES_job_number_list), flt_jid);
                     } 
                  }
                  found_msg = 1;
               }
            }
            if (!found_msg) {
               lDechainElem(mlp, flt_msg);
               lAppendElem(new_list, flt_msg);
            }
         }
         lSetList(sme, SME_message_list, new_list);
         mlp = new_list;
      }

      text[0]=0;
      for_each(mes, mlp) {
         lPSortList (lGetList(mes, MES_job_number_list), "I+", ULNG);

         for_each(jid_ulng, lGetList(mes, MES_job_number_list)) {
            u_long32 mid;
            u_long32 jid = 0;
            int skip = 0;
            int header = 0;

            mid = lGetUlong(mes, MES_message_number);
            jid = lGetUlong(jid_ulng, ULNG);

            if (initialized) {
               if (last_mid == mid && last_jid == jid)
                  skip = 1;
               else if (last_mid != mid)
                  header = 1;
            }
            else {
               initialized = 1;
               header = 1;
            }

            if (strlen(text) >= MAX_LINE_LEN || ids_per_line >= MAX_IDS_PER_LINE || header) {
               printf("%s", text);
               text[0] = 0;
               ids_per_line = 0;
               first_row = 0;
            }

            if (header) {
               if (!first_run)
                  printf("\n\n");
               else
                  first_run = 0;
               printf("%s\n", sge_schedd_text(mid+SCHEDD_INFO_OFFSET));
               first_row = 1;
            }

            if (!skip) {
               if (ids_per_line == 0)
                  if (first_row)
                     strcat(text, "\t");
                  else
                     strcat(text, ",\n\t");
               else
                  strcat(text, ",\t");
               sprintf(ltext, u32, jid);
               strcat(text, ltext);
               ids_per_line++;
            }

            last_jid = jid;
            last_mid = mid;
         }
      }
      if (text[0] != 0)
         printf("%s\n", text);
   }

   lFreeList(ilp);
   DEXIT;
   return 0;
}
