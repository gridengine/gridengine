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
 *   Copyright: 2008 by Sun Microsystems, Inc.
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "sge.h"

#include <time.h>

#include "rmon/sgermon.h"

#include "uti/sge_dstring.h"
#include "uti/sge_log.h"
#include "uti/sge_string.h"
#include "uti/sge_time.h"

#include "gdi/sge_gdi_ctx.h"

#include "sgeobj/sge_advance_reservation.h"
#include "sgeobj/sge_answer.h"
#include "sgeobj/sge_centry.h"
#include "sgeobj/sge_job.h"
#include "sgeobj/sge_jsv.h"
#include "sgeobj/sge_jsv_script.h"
#include "sgeobj/sge_mailrec.h"
#include "sgeobj/sge_qref.h"
#include "sgeobj/sge_range.h"
#include "sgeobj/sge_str.h"
#include "sgeobj/sge_ulong.h"
#include "sgeobj/sge_var.h"

#define JSV_TIMEOUT 4 

typedef bool (*jsv_command_f)(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list, 
                              dstring *c, dstring *s, dstring *a);

typedef struct jsv_command_t_ jsv_command_t;

struct jsv_command_t_ {
   const char *command;
   jsv_command_f func;
};

static bool
jsv_split_commandline(const char *input, dstring *command, dstring *subcommand, dstring *args)
{
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_split_commandline");
   if (input != NULL) {
      struct saved_vars_s *cntx = NULL;
      const char *token1 = sge_strtok_r(input, " ", &cntx);

      if (token1 != NULL) {
         const char *token2;

         sge_dstring_append(command, token1);
         token2 = sge_strtok_r(NULL, " ", &cntx);
         if (token2 != NULL) {
            bool first = true;
            const char *arg = NULL;

            sge_dstring_append(subcommand, token2);
            arg = sge_strtok_r(NULL, " ", &cntx);
            while (arg != NULL) {
               if (first) {
                  first = false;
               } else {
                  sge_dstring_append(args, " ");
               }
               sge_dstring_append(args, arg);
               arg = sge_strtok_r(NULL, " ", &cntx);
            }
         }
      }
      sge_free_saved_vars(cntx);
   }
   DRETURN(ret);
}

static bool 
jsv_split_token(dstring *input, dstring *token, dstring *args) 
{
   const char *i = sge_dstring_get_string(input);
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_split_token");
   if (i != NULL) {
      struct saved_vars_s *cntx = NULL;
      const char *token1 = sge_strtok_r(i, " ", &cntx);

      if (token1 != NULL) {
         bool first = true;
         const char *arg = NULL;

         sge_dstring_append(token, token1);
         arg = sge_strtok_r(NULL, " ", &cntx);
         while (arg != NULL) {
            if (first) {
               first = false;
            } else {
               sge_dstring_append(args, " ");
            }
            sge_dstring_append(args, arg);
            arg = sge_strtok_r(NULL, " ", &cntx);
         }
      }
      sge_free_saved_vars(cntx);
   }
   DRETURN(ret);
}

static bool
jsv_handle_param_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                         dstring *c, dstring *s, dstring *a)
{
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_handle_send_command");
   DRETURN(ret);
}

static bool
jsv_handle_send_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                        dstring *c, dstring *s, dstring *a)
{
   bool ret = true;
   const char *subcommand = sge_dstring_get_string(s);

   DENTER(TOP_LAYER, "jsv_handle_send_command");
   if (strcmp(subcommand, "ENV") == 0) {
      lSetBool(jsv, JSV_send_env, true);
   } else {
      /*
       * Invalid subcommand. JSV seems to wait for information which
       * is not available in this version. Job will be rejected.
       */
      answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                              "got \"SEND "SFN"\" from JSV script which is an unknown command",
                              sge_dstring_get_string(s));

      lSetBool(jsv, JSV_send_env, true);
      ret = false;
   }
   DRETURN(ret);
}

static bool
jsv_handle_result_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                          dstring *c, dstring *s, dstring *a)
{
   bool ret = true;
   dstring m = DSTRING_INIT;
   dstring st = DSTRING_INIT; 
   const char *sub_command = NULL;
   const char *state = NULL;
   const char *message = NULL;

   DENTER(TOP_LAYER, "jsv_handle_result_command");
   sub_command = sge_dstring_get_string(s);
   jsv_split_token(a, &st, &m);
   state = sge_dstring_get_string(&st);
   message = sge_dstring_get_string(&m);
   if (sub_command != NULL && strcmp(sub_command, "STATE") == 0 && state != NULL) {
      if (strcmp(state, "ACCEPT") == 0) {
         lSetBool(jsv, JSV_accept, true);
         lSetBool(jsv, JSV_done, true);
      } else if (strcmp(state, "CORRECT") == 0) {
         lSetBool(jsv, JSV_accept, true);
         lSetBool(jsv, JSV_done, true);
      } else if (strcmp(state, "REJECT") == 0) {
         if (message != NULL) {
            answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR, message);
         } else {
            answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                                    "job has been rejected by JSV script");
         }
         lSetBool(jsv, JSV_accept, false);
         lSetBool(jsv, JSV_done, true);
      } else if (strcmp(state, "REJECT_WAIT") == 0) {
         if (message != NULL) {
            answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR, message);
         } else {
            answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                                    "job has been temporarily rejected by JSV script");
         }
         lSetBool(jsv, JSV_accept, false);
         lSetBool(jsv, JSV_done, true);
      } else {
         answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                                 "got unknown state from JSV script (RESULT STATE %s)", a);
         ret = false;
      }
   } else {
      answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                              "got unknown result command for JSV script");
      ret = false;
   }
   
   /* disable sending of env variables for the next job verification */
   lSetBool(jsv, JSV_send_env, false);

   sge_dstring_free(&m);
   sge_dstring_free(&st);
   DRETURN(ret);
}

static bool
jsv_handle_started_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                           dstring *c, dstring *s, dstring *a)
{
   const char *prefix = "PARAM";
   dstring buffer = DSTRING_INIT;
   bool ret = true;
   lListElem *old_job = lGetRef(jsv, JSV_old_job);

   DENTER(TOP_LAYER, "jsv_handle_started_command");

lWriteElemTo(old_job, stderr);

   /* JSV VERSION <major>.<minor> */
   sge_dstring_clear(&buffer);
   sge_dstring_sprintf(&buffer, "%s VERSION 1.0", prefix);
   jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));

   /* JSV CONTEXT "client"|"server" */
   sge_dstring_clear(&buffer);
   sge_dstring_sprintf(&buffer, "%s CONTEXT %s", prefix,
                 (strcmp(lGetString(jsv, JSV_context), JSV_CONTEXT_CLIENT) == 0 ? "client" : "server"));
   jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   
   /* JSV CLIENT <program_name>*/
   {
      u_long32 progid = ctx->get_who(ctx);

      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s CLIENT %s", prefix, prognames[progid]);
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }

   /* JSV USER <user_name> */
   sge_dstring_clear(&buffer);
   sge_dstring_sprintf(&buffer, "%s USER %s", prefix, lGetString(old_job, JB_owner));
   jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));

   /* JSV GROUP <group_name> */
   sge_dstring_clear(&buffer);
   sge_dstring_sprintf(&buffer, "%s GROUP %s", prefix, lGetString(old_job, JB_group));
   jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));

   /* JSV JOB_ID <jid> (optional; not available in client environment) */
   {
      u_long32 jid = lGetUlong(old_job, JB_job_number);

      if (jid > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s JOB_ID "sge_u32, prefix, jid);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /*
    * JSV COMMAND 
    *    -b y ... <command>      => format := <command>
    *    -b n ... <job_script>   => format := <file> 
    *    -b n                    => format := "STDIN"
    */
   {
      const char *script_name = lGetString(old_job, JB_script_file);

      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s CMDNAME %s", prefix, (script_name != NULL) ? script_name : "NONE");
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }

   /* JSV SCRIPT_ARGS */
   {
      lList *list = lGetList(old_job, JB_job_args);
      lListElem *elem;
      int i = 0;

      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s CMDARGS "sge_u32, prefix, lGetNumberOfElem(list));
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));

      for_each(elem, list) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s CMDARG%d %s", prefix, i, lGetString(elem, ST_name));
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         i++;
      }
   }
   
   /* 
    * -a 
    * PARAM a <date_time> (optional; <date_time> := CCYYMMDDhhmm.SS)
    */
   {
      time_t clocks = (time_t) lGetUlong(old_job, JB_execution_time);

      if (clocks > 0) {
         struct tm time_struct;

         localtime_r(&clocks, &time_struct);
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s a %04d%02d%02d%02d%02d.%02d", prefix,
                             time_struct.tm_year + 1900, time_struct.tm_mon, 
                             time_struct.tm_mday, time_struct.tm_hour, 
                             time_struct.tm_min, time_struct.tm_sec);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -ac variable[=value],... (optional; also contains result of -dc and -sc options) */
   {
      lList *context_list = lGetList(old_job, JB_context);

      if (context_list != NULL) {
         lListElem *tmp_job = lCopyElem(old_job);
         lListElem *context = NULL;
         lList* tmp = NULL;

         lXchgList(tmp_job, JB_context, &tmp);
         set_context(tmp, tmp_job);
         context_list = lGetList(tmp_job, JB_context);
         if (context_list != NULL) {
            bool first = true;

            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "ENV ADD ac");
            for_each(context, context_list) {
               const char *name = lGetString(context, VA_variable);
               const char *value = lGetString(context, VA_value);

               sge_dstring_sprintf_append(&buffer, (first) ? " " : ",");
               first = false;
               if (value != NULL) {
                  sge_dstring_sprintf_append(&buffer, "%s=%s", name, value);
               } else {
                  sge_dstring_sprintf_append(&buffer, "%s", name);
               }
            }
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         }
         lFreeList(&tmp);
         lFreeElem(&tmp_job);
      }
   }

   /* 
    * -ar 
    * PARAM ar <ar_id> (optional; <date_time> := CCYYMMDDhhmm.SS) 
    */
   {
      u_long32 ar_id = lGetUlong(old_job, JB_ar);
   
      if (ar_id > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s ar "sge_u32, prefix, ar_id);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -A <account_string> (optional) */
   {
      const char *account_string = lGetString(old_job, JB_account);

      if (account_string != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s A %s", prefix, account_string);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -b y|n 
    * PARAM b y|n (optional; only available if -b y was specified)
    */
   if (job_is_binary(old_job)) {
      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s b %c", prefix, job_is_binary(old_job) ? 'y' : 'n');
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }

   /*
    * -c n|s|m|x   or   
    * -c <interval>
    *
    * PARAM c_occasion <occasion_string> (optional; <occasion_string> := ['n']['s']['m']['x']
    * PARAM c_interval <interval> (optional; <interval> := <2_digits>:<2_digits>:<2_digits>)
    */
   {
      u_long32 interval = lGetUlong(old_job, JB_checkpoint_interval);
      u_long32 attr = lGetUlong(old_job, JB_checkpoint_attr);

      if (interval > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s c_interval ", prefix);
         double_print_time_to_dstring((double)interval, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
      if (attr > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s c_occasion ", prefix);
         job_get_ckpt_attr(attr, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -ckpt name
    * PARAM ckpt <name> (optional);
    */
   {
      const char *ckpt = lGetString(old_job, JB_checkpoint_name);

      if (ckpt != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s ckpt %s", prefix, ckpt);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -cwd 
    *
    * Different to commandline. If -cwd was specified it will be exported to the
    * JSV by passing the complete path. To remove the path the JSV has to
    * pass an empty path.
    *
    * PARAM cwd <working_directory> (optional)
    */
   {
      const char *cwd = lGetString(old_job, JB_cwd);

      if (cwd != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s cwd %s", prefix, cwd);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -C (not handled in JSV) */
   /* -dc (handled as part of -ac parameter) */
   /* -display <display_name> (handled below where -v/-V is handled) */

   /* -dl <date_time> (optional) */
   {
      time_t clocks = (time_t) lGetUlong(old_job, JB_deadline);

      if (clocks > 0) {
         struct tm time_struct;

         localtime_r(&clocks, &time_struct);
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s dl %04d%02d%02d%02d%02d.%02d", prefix,
                             time_struct.tm_year + 1900, time_struct.tm_mon, 
                             time_struct.tm_mday, time_struct.tm_hour, 
                             time_struct.tm_min, time_struct.tm_sec);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -e <output_path> (optional) */
   {
      lList *shell_list = lGetList(old_job, JB_stderr_path_list);

      if (shell_list != NULL) {
         lListElem *shell;
         bool first = true;
 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s e", prefix);
         for_each(shell, shell_list) {
            const char *hostname = lGetHost(shell, PN_host);
            const char *path = lGetString(shell, PN_path);
           
             
            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (hostname != NULL) {
               sge_dstring_append(&buffer, hostname); 
               sge_dstring_append_char(&buffer, ':');
               sge_dstring_append(&buffer, path);
            } else {
               sge_dstring_append(&buffer, path);
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -hard (handled as l_hard and q_hard below) */

   /* -hold_jid wc_job_list (optional) TODO: EB: CLEANUP: summarize with hold_jid_ad */
   {
      lList *hold_jid_list = lGetList(old_job, JB_jid_request_list);
     
      if (hold_jid_list != NULL) {
         lListElem *hold_jid;
         bool first = true;

         sge_dstring_clear(&buffer); 
         sge_dstring_sprintf(&buffer, "%s hold_jid", prefix);
         for_each(hold_jid, hold_jid_list) {
            const char *name = lGetString(hold_jid, JRE_job_name);
            u_long32 jid = lGetUlong(hold_jid, JRE_job_number);

            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (name != NULL) {
               sge_dstring_sprintf_append(&buffer, "%s", name);
            } else {
               sge_dstring_sprintf_append(&buffer, sge_U32CFormat, sge_u32c(jid));
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -hold_jid_ad wc_job_list (optional) */
   {
      lList *hold_jid_list = lGetList(old_job, JB_ja_ad_request_list);
    
      if (hold_jid_list != NULL) {
         lListElem *hold_jid;
         bool first = true;

         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s hold_jid_ad", prefix);
         for_each(hold_jid, hold_jid_list) {
            const char *name = lGetString(hold_jid, JRE_job_name);
            u_long32 jid = lGetUlong(hold_jid, JRE_job_number);

            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (name != NULL) {
               sge_dstring_sprintf_append(&buffer, "%s", name);
            } else {
               sge_dstring_sprintf_append(&buffer, sge_U32CFormat, sge_u32c(jid));
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -h (optional; only available if job is in user hold)
    * 
    * in difference with the qsub -h switch the setting is provided as
    *
    * PARAM h u|n
    *
    * where 'u' means "user hold"
    * and 'n' whould mean "no hold"
    */
   {
      lList *hold_list = lGetList(old_job, JB_ja_u_h_ids);

      if (hold_list != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s h %c", prefix, (hold_list != NULL) ? 'u' : 'n');
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -i <input_path> (optional) */
   {
      lList *shell_list = lGetList(old_job, JB_stdin_path_list);

      if (shell_list != NULL) {
         lListElem *shell;
         bool first = true;
 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s i", prefix);
         for_each(shell, shell_list) {
            const char *hostname = lGetHost(shell, PN_host);
            const char *path = lGetString(shell, PN_path);
           
             
            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (hostname != NULL) {
               sge_dstring_append(&buffer, hostname); 
               sge_dstring_append_char(&buffer, ':');
               sge_dstring_append(&buffer, path);
            } else {
               sge_dstring_append(&buffer, path);
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -inherit (not handled in JSV) */

   /* 
    * -j y | n 
    * PARAM j y | n (optional; only available when -j y was specified)
    */
   if (lGetBool(old_job, JB_merge_stderr) == true) {
      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s j %c", prefix, lGetBool(old_job, JB_merge_stderr) ? 'y' : 'n');
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }

   /* -js job_share (optional) */
   {
      u_long32 job_share = lGetUlong(old_job, JB_jobshare);

      if (job_share > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s js "sge_U32CFormat, prefix, sge_u32c(job_share));
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -l (optional)
    *
    * -soft -l =>
    * PARAM l_soft <centry_list> 
    *
    * [-hard] -q =>
    * PARAM l_hard <centry_list>
    *
    * TODO: EB: CLEANUP: make a function for the code blocks of -soft -l and -hard -l 
    */
   {
      lList *l_hard_list = lGetList(old_job, JB_hard_resource_list);

      if (l_hard_list != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s l_hard ", prefix);
         centry_list_append_to_dstring(l_hard_list, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }
   {
      lList *l_soft_list = lGetList(old_job, JB_soft_resource_list);

      if (l_soft_list != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s l_soft ", prefix);
         centry_list_append_to_dstring(l_soft_list, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -m [b][e][a][s] or n (optional; only provided to JSV script if != 'n' */
   {
      u_long32 mail_options = lGetUlong(old_job, JB_mail_options);

      if (mail_options > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s m ", prefix);
         sge_mailopt_to_dstring(mail_options, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -M <mail_addr>, ... (optional) */
   {
      lList *mail_list = lGetList(old_job, JB_mail_list);
      lListElem *mail;
      bool first = true;
      
      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s M", prefix);
      for_each(mail, mail_list) {
         const char *user = lGetString(mail, MR_user);
         const char *host = lGetHost(mail, MR_host); 

         sge_dstring_append_char(&buffer, first ? ' ' : ',');
         sge_dstring_append(&buffer, user);
         if (host != NULL) {
            sge_dstring_sprintf_append(&buffer, "@%s", host);
         }
         first = false;
      }
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }
   

   /* -masterq wc_queue_list (optional) */
   {
      lList *master_hard_queue_list = lGetList(old_job, JB_master_hard_queue_list);

      if (master_hard_queue_list != NULL) {
         lListElem *queue;
         bool first = true;

         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s masterq", prefix);
         for_each(queue, master_hard_queue_list) {
            const char *queue_pattern = lGetString(queue, QR_name);

            sge_dstring_append_char(&buffer, first ? ' ' : ','); 
            sge_dstring_sprintf_append(&buffer, "%s", queue_pattern);  
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -notify y|n 
    * PARAM notify y|n (optional; only available when -notify y was specified)
    */
   if (lGetBool(old_job, JB_notify) == true) {
      sge_dstring_clear(&buffer);
      sge_dstring_sprintf(&buffer, "%s notify %c", prefix, lGetBool(old_job, JB_notify) ? 'y' : 'n');
      jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
   }

   /* -now y|n (not available in JSV) */

   /* 
    * -N <job_name> 
    * (optional; only abaiable if specified during job submission)
    */
   {
      const char *name = lGetString(old_job, JB_job_name);

      if (name != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s N %s", prefix, name);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -noshell y | n (not handled in JSV) */
   /* -nostdin (not handled in JSV) */

   /* -o <output_path> (optional) TODO: EB: summarize with -S -e -i */
   {
      lList *shell_list = lGetList(old_job, JB_stdout_path_list);

      if (shell_list != NULL) {
         lListElem *shell;
         bool first = true;
 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s o", prefix);
         for_each(shell, shell_list) {
            const char *hostname = lGetHost(shell, PN_host);
            const char *path = lGetString(shell, PN_path);
           
             
            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (hostname != NULL) {
               sge_dstring_append(&buffer, hostname); 
               sge_dstring_append_char(&buffer, ':');
               sge_dstring_append(&buffer, path);
            } else {
               sge_dstring_append(&buffer, path);
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -ot override_tickets (only available in qmaster. therefore not handled in JSV) */

   /* -P project_name (optional; only available if specified during submission) */
   {
      const char *project = lGetString(old_job, JB_project);

      if (project != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s P %s", prefix, project);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }
   
   /* -p priority (optional; only provided if specified during submission and != 0) */
   {
      int priority = (int) lGetUlong(old_job, JB_priority) - 1024; 

      if (priority != 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s p %d", prefix, priority);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -pe parallel_environment n[-[m]] (optional)
    *
    * PARAM pe_name <pe_name>
    * PARAM pe_min <min_number>
    * PARAM pe_max <max_number>
    *
    *                min_number  max_number
    *    -pe pe 4    4           4
    *    -pe pe 4-8  4           8
    *    -pe pe 4-   4           9999999
    *    -pe pe -8   1           8
    */
   {
      const char *pe_name = lGetString(old_job, JB_pe);
      lList *range_list = lGetList(old_job, JB_pe_range);
      lListElem *range = lFirst(range_list);

      if (pe_name != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s pe_name %s", prefix, pe_name);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
      if (range != NULL) {
         u_long32 min = lGetUlong(range, RN_min);
         u_long32 max = lGetUlong(range, RN_max);

         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s pe_min "sge_U32CFormat, prefix, sge_u32c(min));
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s pe_max "sge_U32CFormat, prefix, sge_u32c(max));
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -pty y|n (not available in JSV) */

   /* 
    * -q wc_queue_list (optional; see man page sge_types(1) for wc_queue_list specification)
    *
    * -soft -q =>
    * PARAM q_soft <wc_queue_list> (see man page sge_types(1) for wc_queue_list specification)
    *
    * [-hard] -q =>
    * PARAM q_hard <wc_queue_list> 
    *
    * TODO: EB: CLEANUP: make a function for the code blocks of -soft -q, -hard -q and -masterq
    */
   {
      lList *hard_queue_list = lGetList(old_job, JB_hard_queue_list);

      if (hard_queue_list != NULL) {
         lListElem *queue;
         bool first = true;

         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s l_hard", prefix);
         for_each(queue, hard_queue_list) {
            const char *queue_pattern = lGetString(queue, QR_name);

            sge_dstring_append_char(&buffer, first ? ' ' : ','); 
            sge_dstring_sprintf_append(&buffer, "%s", queue_pattern);  
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }
   {
      lList *soft_queue_list = lGetList(old_job, JB_soft_queue_list);

      if (soft_queue_list != NULL) {
         lListElem *queue;
         bool first = true;

         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s l_soft", prefix);
         for_each(queue, soft_queue_list) {
            const char *queue_pattern = lGetString(queue, QR_name);

            sge_dstring_append_char(&buffer, first ? ' ' : ','); 
            sge_dstring_sprintf_append(&buffer, "%s", queue_pattern);  
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -R y|n (optional; only available if specified during sibmission and value is y) */
   {
      bool reserve = lGetBool(old_job, JB_reserve) ? true : false;
  
      if (reserve) { 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s R %c", prefix, reserve ? 'y' : 'n');
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -r y|n (optional; only available if specified during submission and value is y) */
   {
      u_long32 restart = lGetUlong(old_job, JB_restart);
  
      if (restart == 1) { 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s r %c", prefix, (restart == 1) ? 'y' : 'n');
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -sc (handled as part of -ac) */

   /* -shell y|n (optional; only available if -shell n was specified */
   {
      u_long32 type = lGetUlong(old_job, JB_type);

      if (JOB_TYPE_IS_NO_SHELL(type)) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s shell %c", prefix, !JOB_TYPE_IS_NO_SHELL(type) ? 'y' : 'n');
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -soft (handled as l_soft and q_soft) */

   /* -sync y|n (not available in JSV) */

   /* 
    * -S shell_path_list (optional) 
    * PARAM S [hostname:]path,...
    */
   {
      lList *shell_list = lGetList(old_job, JB_shell_list);

      if (shell_list != NULL) {
         lListElem *shell;
         bool first = true;
 
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s S", prefix);
         for_each(shell, shell_list) {
            const char *hostname = lGetHost(shell, PN_host);
            const char *path = lGetString(shell, PN_path);
           
             
            sge_dstring_append_char(&buffer, first ? ' ' : ',');
            if (hostname != NULL) {
               sge_dstring_append(&buffer, hostname); 
               sge_dstring_append_char(&buffer, ':');
               sge_dstring_append(&buffer, path);
            } else {
               sge_dstring_append(&buffer, path);
            }
            first = false;
         }
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* 
    * -t min[-max[:step]] (optional; only available if specified during submission 
    * and if values differ from "1-1:1") 
    * PARAM t_min <number>
    * PARAM t_max <number>
    * PARAM t_step <number>
    */
   {
      lList *ja_structure_list = lGetList(old_job, JB_ja_structure); 
      lListElem *ja_structure = lFirst(ja_structure_list);

      if (ja_structure != NULL) {
         u_long32 min, max, step;

         range_get_all_ids(ja_structure, &min, &max, &step);

         /*
          * if -t is not specified then all values will be 1 therefore we have to 
          * provide the values to JSV only if one value differes from 1
          */
         if (max != 1 || min != 1 || step != 1) {
            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "%s t_min "sge_U32CFormat, prefix, sge_u32c(min));
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "%s t_max "sge_U32CFormat, prefix, sge_u32c(max));
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "%s t_step "sge_U32CFormat, prefix, sge_u32c(step));
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         }
      }
   }

   /* -terse (ignored in JSV. it is just to late to change this) */
   /* -u username,... (not handled in JSV because only available for qalter) */
   /* -v variable[=value],... (handles also -V; done below after all params are handled */
   /* -verbose (not available in JSV) */
   /* -V (handled as part of -v) */

   /* -w e|w|n|v|p (optional; only sent to JSV if != 'n') */
   {
      u_long32 verify = lGetUlong(old_job, JB_verify_suitable_queues);
   
      if (verify > 0) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s w ", prefix);
         job_get_verify_attr(verify, &buffer);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* -wd working_dir (optional) */
   {
      const char *cwd = lGetString(old_job, JB_cwd);

      if (cwd != NULL) {
         sge_dstring_clear(&buffer);
         sge_dstring_sprintf(&buffer, "%s wd %s", prefix, cwd);
         jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
      }
   }

   /* command (handled as PARAM SCRIPT NAME above) */
   /* command_args (handeled as PARAM SCRIPT above) */
   /* xterm_args (handeled as PARAM SCRIPT above) */

   /* handle -v -V and -display here */  
   {
      lList *env_list = NULL;
      lListElem *env = NULL;
      lListElem *display = NULL;

      /* make a copy of the environment */
      var_list_copy_env_vars_and_value(&env_list, lGetList(old_job, JB_env_list));

      /* remove certain variables which don't come from the user environment */
      var_list_remove_prefix_vars(&env_list, VAR_PREFIX);
      var_list_remove_prefix_vars(&env_list, VAR_PREFIX_NR);

      /* 
       * if there is a DISPLAY variable and if the client is qsh/qrsh
       * then we will send the DISPLAY value as if it originally came 
       * from -display switch.
       */
      display = lGetElemStr(env_list, VA_variable, "DISPLAY"); 
      if (display != NULL) {
         const char *value = lGetString(display, VA_value);
         u_long32 progid = ctx->get_who(ctx);

         if (value != NULL && 
             (strcmp(prognames[progid], "qsh") == 0 || strcmp(prognames[progid], "qrsh") == 0)) {
            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "PARAM display %s", value);
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         } 
      }

      /* send the varaibles to the JSV but only if it was requested */
      if (lGetBool(jsv, JSV_send_env) == true) {
         for_each(env, env_list) {
            const char *value = lGetString(env, VA_value);
            const char *name = lGetString(env, VA_variable);
      
            sge_dstring_clear(&buffer);
            sge_dstring_sprintf(&buffer, "ENV ADD %s %s", name, value);
            jsv_send_command(jsv, answer_list, sge_dstring_get_string(&buffer));
         }
      }
      lFreeList(&env_list);
   }

   /* script got all parameters. now verification can start */
   if (ret) {
      ret &= jsv_send_command(jsv, answer_list, "BEGIN");
   }

   /* cleanup */
   sge_dstring_free(&buffer);

   DRETURN(ret);
}

static bool
jsv_handle_log_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                       dstring *c, dstring *s, dstring *a)
{
   bool ret = true;
   const char *command = sge_dstring_get_string(s);
   const char *sub_command = sge_dstring_get_string(s);
   const char *args = sge_dstring_get_string(a);

   DENTER(TOP_LAYER, "jsv_handle_log_command");
   if (args != NULL) {
      if (strcmp(lGetString(jsv, JSV_context), JSV_CONTEXT_CLIENT) != 0) {
         if (strcmp(sub_command, "INFO") == 0) {
            INFO((SGE_EVENT, "%s", args));
         } else if (strcmp(sub_command, "WARNING") == 0) {
            WARNING((SGE_EVENT, "%s", args));
         } else if (strcmp(sub_command, "ERROR") == 0) {
            ERROR((SGE_EVENT, "%s", args));
         } else {
            WARNING((SGE_EVENT, "received "SFQ" from JSV with invalid type "SFQ, command, sub_command));
         }
      } else {
         printf("%s\n", args);
      }
   }
   DRETURN(ret);
}

static bool
jsv_handle_env_command(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list,
                       dstring *c, dstring *s, dstring *a)
{
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_handle_env_command");
   DRETURN(ret);
}

bool
jsv_do_communication(sge_gdi_ctx_class_t *ctx, lListElem *jsv, lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "jsv_do_communication");
   if (ret) {
      ret &= jsv_send_command(jsv, answer_list, "START");
   }
   if (ret) {
      u_long32 start_time = sge_get_gmt();

      lSetBool(jsv, JSV_done, false);
      lSetBool(jsv, JSV_soft_shutdown, true);
      while (!lGetBool(jsv, JSV_done)) {
         char input[10000];
         FILE *file = lGetRef(jsv, JSV_out);

         if (sge_get_gmt() - start_time > JSV_TIMEOUT) {
            answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                                    "got no response from JSV script "SFQ, lGetString(jsv, JSV_command));
            lSetBool(jsv, JSV_restart, true);
            lSetBool(jsv, JSV_soft_shutdown, false);
            lSetBool(jsv, JSV_done, true);
         } else {
            /* read a line from the script or wait some time before you try again */
            if (fscanf(file, "%[^\n]\n", input) == 1) {
               dstring sub_command = DSTRING_INIT;
               dstring command = DSTRING_INIT;
               dstring args = DSTRING_INIT;
               jsv_command_t commands[] = {
                  {"PARAM", jsv_handle_param_command},
                  {"ENV", jsv_handle_env_command},
                  {"LOG", jsv_handle_log_command},
                  {"RESULT", jsv_handle_result_command},
                  {"SEND", jsv_handle_send_command},
                  {"STARTED", jsv_handle_started_command},
                  {NULL, NULL}
               };
               bool handled = false;
               const char *c;
               int i = -1;

               DPRINTF(("JSV << \"%s\"\n", input));

               jsv_split_commandline(input, &command, &sub_command, &args);
               c = sge_dstring_get_string(&command);
   
               DPRINTF(("splitted input in "SFQ" and "SFQ"\n", sge_dstring_get_string(&command),
                        sge_dstring_get_string(&sub_command)));

               while(commands[++i].command != NULL) {
                  if (strcmp(c, commands[i].command) == 0) {
                     handled = true;
                     ret &= commands[i].func(ctx, jsv, answer_list, 
                                             &command, &sub_command, &args);

                     if (ret == false || 
                         lGetBool(jsv, JSV_restart) == true || 
                         lGetBool(jsv, JSV_accept) == true) {
                        lSetBool(jsv, JSV_done, true);
                     }
                     break;
                  }
               }

               if (!handled) {
                  answer_list_add_sprintf(answer_list, STATUS_DENIED, ANSWER_QUALITY_ERROR,
                                          "got "SFQ" from JSV script which is an unknown command", 
                                          c);
                  lSetBool(jsv, JSV_accept, false);
                  lSetBool(jsv, JSV_done, true);
               }
            }
         }
      }
   }
   return ret;
}

