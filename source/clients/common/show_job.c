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
 *  License at http://www.gridengine.sunsource.net/license.html
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
#include <time.h>

#include "def.h"
#include "sge_gdi_intern.h"
#include "sge_all_listsL.h"
#include "sge_resource.h"
#include "show_job.h"
#include "parse_qsub.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_parse_util.h"
#include "utility.h"
#include "parse_range.h"
#include "get_path.h"
#include "job.h"
#include "sge_parse_num_par.h"
#include "sge_feature.h"
#include "msg_clients_common.h"

void cull_show_job(
lListElem *job,
int flags 
) {
   char *delis[] = {NULL, ",", "\n"};
   time_t ultime;   /* used to be u_long32, but problem w/ 64 bit times */

   DENTER(TOP_LAYER, "cull_show_job");

   if(!job) {
      DEXIT;
      return;
   }

DTRACE;
   if (!(flags & FLG_QALTER)) {
      if (lGetUlong(job, JB_job_number))
         printf("job_number:                 %d\n", (int) lGetUlong(job, JB_job_number));
      else
         printf("job_number:                 %s\n", MSG_JOB_UNASSIGNED);
   }
DTRACE;

   if (lGetPosViaElem(job, JB_job_file)>=0)
      if (lGetString(job, JB_job_file))
         printf("job_file:                   %s\n", lGetString(job, JB_job_file));

   if (lGetPosViaElem(job, JB_exec_file)>=0)
      if (lGetString(job, JB_exec_file))
         printf("exec_file:                  %s\n", lGetString(job, JB_exec_file));

   if (lGetPosViaElem(job, JB_submission_time)>=0)
      if ((ultime = lGetUlong(job, JB_submission_time))) {
         printf("submission_time:            %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_end_time)>=0)
      if ((ultime = lGetUlong(job, JB_end_time))) {
         printf("end_time:                   %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_owner)>=0) {
      if (lGetString(job, JB_owner))
         printf("owner:                      %s\n", lGetString(job, JB_owner));
      else
         printf("owner:                      %s\n", "");
   }
   
   if (lGetPosViaElem(job, JB_uid)>=0)
      printf("uid:                        %d\n", (int) lGetUlong(job, JB_uid));

   if (lGetPosViaElem(job, JB_group)>=0) {
      if (lGetString(job, JB_group))
         printf("group:                      %s\n", lGetString(job, JB_group));
      else
         printf("group:                      %s\n", "");
   }
   
   if (lGetPosViaElem(job, JB_gid)>=0)
      printf("gid:                        %d\n", (int) lGetUlong(job, JB_gid));

   if (lGetPosViaElem(job, JB_sge_o_home)>=0)
      if (lGetString(job, JB_sge_o_home))
         printf("sge_o_home:                 %s\n", lGetString(job, JB_sge_o_home));

   if (lGetPosViaElem(job, JB_sge_o_log_name)>=0)
      if (lGetString(job, JB_sge_o_log_name))
         printf("sge_o_log_name:             %s\n", lGetString(job, JB_sge_o_log_name));

   if (lGetPosViaElem(job, JB_sge_o_path)>=0)
      if (lGetString(job, JB_sge_o_path))
         printf("sge_o_path:                 %s\n", lGetString(job, JB_sge_o_path));

   if (lGetPosViaElem(job, JB_sge_o_mail)>=0)
      if (lGetString(job, JB_sge_o_mail))
         printf("sge_o_mail:                 %s\n", lGetString(job, JB_sge_o_mail));

   if (lGetPosViaElem(job, JB_sge_o_shell)>=0)
      if (lGetString(job, JB_sge_o_shell))
         printf("sge_o_shell:                %s\n", lGetString(job, JB_sge_o_shell));

   if (lGetPosViaElem(job, JB_sge_o_tz)>=0)
      if (lGetString(job, JB_sge_o_tz))
         printf("sge_o_tz:                   %s\n", lGetString(job, JB_sge_o_tz));

   if (lGetPosViaElem(job, JB_sge_o_workdir)>=0)
      if (lGetString(job, JB_sge_o_workdir))
         printf("sge_o_workdir:              %s\n", lGetString(job, JB_sge_o_workdir));

   if (lGetPosViaElem(job, JB_sge_o_host)>=0)
      if (lGetString(job, JB_sge_o_host))
         printf("sge_o_host:                 %s\n", lGetString(job, JB_sge_o_host));

   if (lGetPosViaElem(job, JB_execution_time)>=0)
      if ((ultime = lGetUlong(job, JB_execution_time)))
         printf("execution_time:             %s", ctime((time_t *) &ultime));

   if (lGetPosViaElem(job, JB_account)>=0)
      if (lGetString(job, JB_account))
         printf("account:                    %s\n", lGetString(job, JB_account));

   if (lGetPosViaElem(job, JB_checkpoint_object)>=0)
      if (lGetString(job, JB_checkpoint_object))
         printf("checkpoint_object:          %s\n", lGetString(job, JB_checkpoint_object));

   if (lGetPosViaElem(job, JB_checkpoint_attr)>=0)
      if (lGetUlong(job, JB_checkpoint_attr)) {
         printf("checkpoint_attr:            ");
         sge_show_checkpoint(SGE_STDOUT, lGetUlong(job, JB_checkpoint_attr));
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_checkpoint_interval)>=0)
      if (lGetUlong(job, JB_checkpoint_interval)) {
         printf("checkpoint_interval:        ");
         printf("%d seconds\n", (int) lGetUlong(job, JB_checkpoint_interval));
      }

   if (lGetPosViaElem(job, JB_cell)>=0)
      if (lGetString(job, JB_cell))
         printf("cell:                       %s\n", lGetString(job, JB_cell));

   if (lGetPosViaElem(job, JB_cwd)>=0) {
      if (lGetString(job, JB_cwd))
         printf("cwd:                        %s\n", lGetString(job, JB_cwd));
      if (lGetPosViaElem(job, JB_path_aliases)>=0)
         if (lGetList(job, JB_path_aliases)) {
            intprt_type fields[] = { PA_origin, PA_submit_host, PA_exec_host,
                                       PA_translation, 0 };

            delis[0] = " ";
            printf("path_aliases:               ");
            uni_print_list(stdout, NULL, 0, lGetList(job, JB_path_aliases), 
               fields, delis, FLG_NO_DELIS_STRINGS);
         }
   }

   if (lGetPosViaElem(job, JB_directive_prefix)>=0)
      if (lGetString(job, JB_directive_prefix))
         printf("directive_prefix:           %s\n", lGetString(job, JB_directive_prefix));

   if (lGetPosViaElem(job, JB_stderr_path_list)>=0)
      if (lGetList(job, JB_stderr_path_list)) {
         intprt_type fields[] = { PN_host, PN_path, 0 };

         delis[0] = ":";
         printf("stderr_path_list:           ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_stderr_path_list), fields, 
            delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_full_listing)>=0)
      if (lGetUlong(job, JB_full_listing))
         printf("full_listing:               %s\n", "-f");

   if (lGetPosViaElem(job, JB_merge_stderr)>=0)
      if (lGetUlong(job, JB_merge_stderr)) {
         printf("merge:                      ");
         sge_show_y_n(lGetUlong(job, JB_merge_stderr), SGE_STDOUT);
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_hard_resource_list)>=0)
      if (lGetList(job, JB_hard_resource_list)) {
         printf("hard resource_list:         ");
         sge_show_re_type_list(lGetList(job, JB_hard_resource_list));
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_soft_resource_list)>=0)
      if (lGetList(job, JB_soft_resource_list)) {
         printf("soft resource_list:         ");
         sge_show_re_type_list(lGetList(job, JB_soft_resource_list));
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_mail_options)>=0)
      if (lGetUlong(job, JB_mail_options)) {
         printf("mail_options:               ");
         sge_show_mail_options(lGetUlong(job, JB_mail_options), SGE_STDOUT);
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_mail_list)>=0)
      if (lGetList(job, JB_mail_list)) {
         intprt_type fields[] = { MR_user, MR_host, 0 };

         delis[0] = "@";
         printf("mail_list:                  ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_mail_list), 
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_notify)>=0)
      printf("notify:                     %s\n", (lGetUlong(job, JB_notify) ? "TRUE" : "FALSE"));

   if (lGetPosViaElem(job, JB_job_name)>=0) {
      if (lGetString(job, JB_job_name))
         printf("job_name:                   %s\n", lGetString(job, JB_job_name));
      else
         printf("job_name:                   %s\n", "");
   }
   
   if (lGetPosViaElem(job, JB_stdout_path_list)>=0)
      if (lGetList(job, JB_stdout_path_list)) {
         intprt_type fields[] = { PN_host, PN_path, 0 };

         delis[0] = ":";
         printf("stdout_path_list:           ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_stdout_path_list), fields, 
            delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_priority)>=0)
      if (lGetUlong(job, JB_priority) != BASE_PRIORITY) {
         printf("priority:                   ");
         printf("%d\n", (int) lGetUlong(job, JB_priority) - BASE_PRIORITY);
      }

   if (lGetPosViaElem(job, JB_hard_queue_list)>=0)
      if (lGetList(job, JB_hard_queue_list)) {
         intprt_type fields[] = {QR_name, 0 };

         delis[0] = " ";
         printf("hard_queue_list:            ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_hard_queue_list), 
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_soft_queue_list)>=0)
      if (lGetList(job, JB_soft_queue_list)) {
         intprt_type fields[] = {QR_name, 0 };

         delis[0] = " ";
         printf("soft_queue_list:            ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_soft_queue_list),
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_reauth_time)>=0)
      if (lGetUlong(job, JB_reauth_time))
         printf("reauth_time:                %d\n", (int) lGetUlong(job, JB_reauth_time));

   if (lGetPosViaElem(job, JB_restart)>=0)
      if (lGetUlong(job, JB_restart)) {
         printf("restart:                    ");
         sge_show_y_n((lGetUlong(job, JB_restart)==2)?0:1, SGE_STDOUT);
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_signal)>=0)
      if (lGetUlong(job, JB_signal))
         printf("signal:                     %d\n", (int) lGetUlong(job, JB_signal));

   if (lGetPosViaElem(job, JB_shell_list)>=0)
      if (lGetList(job, JB_shell_list)) {
         intprt_type fields[] = {PN_host, PN_path, 0 };

         delis[0] = ":";
         printf("shell_list:                 ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_shell_list), fields, 
            delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_verify)>=0)
      if (lGetUlong(job, JB_verify))
         printf("verify:                     %s\n", "-verify");

   if (lGetPosViaElem(job, JB_env_list)>=0)
      if (lGetList(job, JB_env_list)) {
         intprt_type fields[] = {VA_variable, VA_value, 0 };

         delis[0] = "=";
         printf("env_list:                   ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_env_list), 
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_job_args)>=0)
      if (lGetList(job, JB_job_args) || (flags & FLG_QALTER)) {
         intprt_type fields[] = {STR, 0 };

         delis[0] = "";
         printf("job_args:                   ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_job_args), fields, 
                           delis, 0);
      }

   if (lGetPosViaElem(job, JB_qs_args)>=0)
      if (lGetList(job, JB_qs_args) || (flags & FLG_QALTER)) {
         intprt_type fields[] = {STR, 0 };
         delis[0] = "";
         printf("qs_args:                    ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_qs_args), fields, 
                           delis, 0);
      }

   if (lGetPosViaElem(job, JB_master_hard_queue_list)>=0)
      if (lGetList(job, JB_master_hard_queue_list)) {
         intprt_type fields[] = {QR_name, 0 };
         delis[0] = " ";
         printf("master hard queue list:     ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_master_hard_queue_list), 
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   if (lGetPosViaElem(job, JB_job_identifier_list)>=0)
      if (lGetList(job, JB_job_identifier_list)) {
         intprt_type fields[] = { JRE_job_number, 0 };

         delis[0] = "";
         printf("job_identifier_list:        ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_job_identifier_list), 
            fields, delis, 0);
      }

   if (lGetPosViaElem(job, JB_message)>=0)
      if (lGetString(job, JB_message))
         printf("message:                    %s\n", lGetString(job, JB_message));

   if (lGetPosViaElem(job, JB_script_size)>=0)
      if (lGetUlong(job, JB_script_size))
         printf("script_size:                "uu32"\n", lGetUlong(job, JB_script_size));

   if (lGetPosViaElem(job, JB_script_file)>=0)
      if (lGetString(job, JB_script_file))
         printf("script_file:                %s\n", lGetString(job, JB_script_file));

   if (lGetPosViaElem(job, JB_script_ptr)>=0)
      if (lGetString(job, JB_script_ptr))
         printf("script_ptr:            \n%s\n", lGetString(job, JB_script_ptr));

   if (lGetPosViaElem(job, JB_job_source)>=0)
      if (lGetString(job, JB_job_source))
         printf("job_source:                 %s\n", lGetString(job, JB_job_source));

   if (lGetPosViaElem(job, JB_ext)>=0)
      if (lGetUlong(job, JB_ext))
         printf("ext:                        %s\n", "-ext");

   if (lGetPosViaElem(job, JB_pe)>=0)
      if (lGetString(job, JB_pe)) {
         char str[256 + 1];

         show_ranges(str, 0, NULL, lGetList(job, JB_pe_range));
         printf("parallel environment:  %s range: %s\n",
            lGetString(job, JB_pe), str);
      }

   if (lGetPosViaElem(job, JB_scheduling_priority)>=0)
      if (lGetUlong(job, JB_scheduling_priority))
         printf("scheduling_priority:        %d\n", 
            (int) lGetUlong(job, JB_scheduling_priority));

   if (lGetPosViaElem(job, JB_jid_predecessor_list)>=0)
      if (lGetList(job, JB_jid_predecessor_list)) {
         intprt_type fields[] = { JRE_job_number, 0 };

         delis[0] = "";
         printf("jid_predecessor_list:       ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_jid_predecessor_list), 
            fields, delis, 0);
      }

   if (lGetPosViaElem(job, JB_jid_sucessor_list)>=0)
      if (lGetList(job, JB_jid_sucessor_list)) {
         intprt_type fields[] = { JRE_job_number, 0 };

         delis[0] = "";
         printf("jid_sucessor_list:          ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_jid_sucessor_list), 
            fields, delis, 0);
      }

   if (lGetPosViaElem(job, JB_pvm_pid)>=0)
      if (lGetUlong(job, JB_pvm_pid))
         printf("pvm_pid:                    %d\n", (int) lGetUlong(job, JB_pvm_pid));

   if (lGetPosViaElem(job, JB_verify_suitable_queues)>=0)
      if (lGetUlong(job, JB_verify_suitable_queues))
         printf("verify_suitable_queues:     %d\n", (int)lGetUlong(job, JB_verify_suitable_queues));

   if (lGetPosViaElem(job, JB_sig)>=0)
      if (lGetUlong(job, JB_sig))
         printf("sig:                        %d\n", (int) lGetUlong(job, JB_sig));

   if (lGetPosViaElem(job, JB_notified)>=0)
      if (lGetUlong(job, JB_notified))
         printf("notified:                   %d\n", (int) lGetUlong(job, JB_notified));

   if (lGetPosViaElem(job, JB_reauth_gmt)>=0)
      if ((ultime = lGetUlong(job, JB_reauth_gmt))) {
         printf("reauth_gmt:                 %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_soft_wallclock_gmt)>=0)
      if ((ultime = lGetUlong(job, JB_soft_wallclock_gmt))) {
         printf("soft_wallclock_gmt:         %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_hard_wallclock_gmt)>=0)
      if ((ultime = lGetUlong(job, JB_hard_wallclock_gmt))) {
         printf("hard_wallclock_gmt:         %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_suspend_enable)>=0)
      if (lGetUlong(job, JB_suspend_enable))
         printf("suspend_enable:             %d\n", (int) lGetUlong(job, JB_suspend_enable));

   if (lGetPosViaElem(job, JB_soc_xsoc)>=0)
      if (lGetUlong(job, JB_soc_xsoc))
         printf("soc_xsoc:                   %d\n", (int) lGetUlong(job, JB_soc_xsoc));

   if (lGetPosViaElem(job, JB_force)>=0)
      if (lGetUlong(job, JB_force))
         printf("force:                      %d\n", (int) lGetUlong(job, JB_force));

   if (lGetPosViaElem(job, JB_version)>=0)
      if (lGetUlong(job, JB_version))
         printf("version:                    %d\n", (int) lGetUlong(job, JB_version));
      /*
      ** problem: found no format anywhere
      */

   if (lGetPosViaElem(job, JB_override_tickets)>=0)
      if (lGetUlong(job, JB_override_tickets))
         printf("oticket:                    %d\n", (int) lGetUlong(job, JB_override_tickets));

   if (lGetPosViaElem(job, JB_project)>=0)
      if (lGetString(job, JB_project))
         printf("project:                    %s\n", lGetString(job, JB_project));

   if (lGetPosViaElem(job, JB_ja_structure)>=0) {
      u_long32 start, end, step;

      get_ja_task_ids(job, &start, &end, &step);
      if (is_array(job))
         printf("job-array tasks             "u32"-"u32":"u32"\n", start, end, step);
   }

#if 0

   if (lGetPosViaElem(job, JB_share)>=0)
      if (lGetDouble(job, JB_share))
         printf("share:                      %f\n", lGetDouble(job, JB_share));

   if (lGetPosViaElem(job, JB_pid)>=0)
      if (lGetUlong(job, JB_pid))
         printf("pid:                        %d\n", (int) lGetUlong(job, JB_pid));

   if (lGetPosViaElem(job, JB_status)>=0)
      if (lGetUlong(job, JB_status))
         printf("status:                     %d\n", (int) lGetUlong(job, JB_status));

   if (lGetPosViaElem(job, JB_start_time)>=0)
      if ((ultime = lGetUlong(job, JB_start_time))) {
         printf("start_time:                 %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_hold)>=0)
      if (lGetUlong(job, JB_hold)) {
         printf("hold_list:                  ");
         sge_show_hold_list(lGetUlong(job, JB_hold), SGE_STDOUT);
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_granted_destin_identifier_list)>=0)
      if (lGetList(job, JB_granted_destin_identifier_list)) {
         intprt_type fields[] = { JG_qname, JG_qhostname, JG_slots, 0 };

         delis[0] = ":";
         printf("granted_destin_identifier_list:");
         uni_print_list(stdout, NULL, 0,
            lGetList(job, JB_granted_destin_identifier_list), fields, delis, 0);
      }

   if (lGetPosViaElem(job, JB_master_queue)>=0)
      if (lGetString(job, JB_master_queue))
         printf("master_queue:               %s\n", lGetString(job, JB_master_queue));

   if (lGetPosViaElem(job, JB_state)>=0)
      if (lGetUlong(job, JB_state)) {
         printf("state:                      ");
         sge_show_states(JB_job_number, SGE_STDOUT, lGetUlong(job, JB_state));
         printf("\n");
      }

   if (lGetPosViaElem(job, JB_pending_signal)>=0)
      if (lGetUlong(job, JB_pending_signal))
         printf("pending_signal:             %d\n", (int) lGetUlong(job, JB_pending_signal));

   if (lGetPosViaElem(job, JB_pending_signal_delivery_time)>=0)
      if ((ultime = lGetUlong(job, JB_pending_signal_delivery_time))) {
         printf("pending_signal_delivery_time: %s", ctime((time_t *) &ultime));
      }

   if (lGetPosViaElem(job, JB_osjobid)>=0)
      if (lGetString(job, JB_osjobid))
         printf("osjobid:                    %s\n", lGetString(job, JB_osjobid));

   if (lGetPosViaElem(job, JB_usage_list)>=0)
      if (lGetList(job, JB_usage_list))
         printf("usage list:                 %s\n", "FORMAT NOT YET IMPLEMENTED!!!");

   if (lGetPosViaElem(job, JB_suitable)>=0)
      if (lGetUlong(job, JB_suitable))
         printf("suitable:                   %d\n", (int) lGetUlong(job, JB_suitable));

#endif

   if (lGetPosViaElem(job, JB_context)>=0)
      if (lGetList(job, JB_context)) {
         intprt_type fields[] = {VA_variable, VA_value, 0 };

         delis[0] = "=";
         printf("context:                    ");
         uni_print_list(stdout, NULL, 0, lGetList(job, JB_context), 
            fields, delis, FLG_NO_DELIS_STRINGS);
      }

   /* display online job usage separately for each array job but summarized over all pe_tasks */
#define SUM_UP_USAGE(pe_task, dst, attr) \
      if ((uep=lGetSubStr(pe_task, UA_name, attr, JAT_scaled_usage_list))) { \
         DPRINTF(("usage for %s = %f\n", attr, lGetDouble(uep, UA_value))); \
         dst += lGetDouble(uep, UA_value); \
      } else { \
         DPRINTF(("no usage for %s\n", attr)); \
      }

   if (feature_is_enabled(FEATURE_REPORT_USAGE) 
       && lGetPosViaElem(job, JB_ja_tasks)>=0) {
      lListElem *uep, *jatep, *pe_task_ep;
      for_each (jatep, lGetList(job, JB_ja_tasks)) {
         double cpu, mem, io, vmem;
         static char cpu_usage[100], vmem_usage[100];

         if (!lPrev(jatep))
            printf("usage %4d:                  ", (int)lGetUlong(jatep, JAT_task_number));
         else
            printf("      %4d:                  ", (int)lGetUlong(jatep, JAT_task_number));

         cpu = mem = io = vmem = 0.0;

         /* master task */
         SUM_UP_USAGE(jatep, cpu, USAGE_ATTR_CPU);
         SUM_UP_USAGE(jatep, vmem, USAGE_ATTR_VMEM);
         SUM_UP_USAGE(jatep, mem, USAGE_ATTR_MEM);
         SUM_UP_USAGE(jatep, io, USAGE_ATTR_IO);

         /* slave tasks */
         for_each (pe_task_ep, lGetList(jatep, JAT_task_list)) {
            SUM_UP_USAGE(pe_task_ep, cpu, USAGE_ATTR_CPU);
            SUM_UP_USAGE(pe_task_ep, vmem, USAGE_ATTR_VMEM);
            SUM_UP_USAGE(pe_task_ep, mem, USAGE_ATTR_MEM);
            SUM_UP_USAGE(pe_task_ep, io, USAGE_ATTR_IO);
         }

         printf("cpu=%s, vmem=%s, mem=%-5.5f GBs, io=%-5.5f\n",
            resource_descr(cpu, TYPE_TIM, cpu_usage),
            resource_descr(vmem, TYPE_MEM, vmem_usage),
            mem, io);
      }
   }

   DEXIT;
   return;
}

