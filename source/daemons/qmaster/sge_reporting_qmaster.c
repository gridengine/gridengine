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
#include <errno.h>
#include <string.h>
#include <pthread.h>

/* rmon */
#include "sgermon.h"

/* uti */
#include "sge_log.h"
#include "sge_string.h"
#include "sge_dstring.h"
#include "setup_path.h"
#include "sge_stdlib.h"
#include "sge_unistd.h"
#include "sge_spool.h"
#include "sge_time.h"

/* lck */
#include "sge_lock.h"
#include "sge_mtutil.h"

/* daemons/common */
#include "category.h"

/* sgeobj */
#include "sge_answer.h"
#include "sge_feature.h"
#include "sge_object.h"
#include "sge_centry.h"
#include "sge_conf.h"
#include "sge_host.h"
#include "sge_ja_task.h"
#include "sge_job.h"
#include "sge_pe_task.h"
#include "sge_qinstance.h"
#include "sge_str.h"
#include "sge_sharetree.h"
#include "sge_userprj.h"
#include "sge_userset.h"

/* sched */
#include "sge_sharetree_printing.h"

/* local */
#include "sge_rusage.h"
#include "sge_reporting_qmaster.h"

/* messages */
#include "msg_common.h"

/* do not change, the ":" is hard coded into the qacct file
   parsing routines. */
static const char REPORTING_DELIMITER = ':';

typedef enum {
   ACCOUNTING_BUFFER = 0,
   REPORTING_BUFFER
} rep_buf;

typedef struct {
   dstring buffer;
   pthread_mutex_t mtx;
   const char *mtx_name;
} rep_buf_t;

static rep_buf_t reporting_buffer[2] = {
   { DSTRING_INIT, PTHREAD_MUTEX_INITIALIZER, "mtx_accounting" },
   { DSTRING_INIT, PTHREAD_MUTEX_INITIALIZER, "mtx_reporting" }
};

static bool 
reporting_flush_accounting(lList **answer_list);

static bool 
reporting_flush_reporting(lList **answer_list);

static bool 
reporting_flush_report_file(lList **answer_list,
                            const char *filename, rep_buf_t *buf);

static bool 
reporting_flush(lList **answer_list, u_long32 flush, u_long32 *next_flush);

static bool 
reporting_create_record(lList **answer_list, 
                        const char *type,
                        const char *data);

static bool
reporting_write_load_values(lList **answer_list, dstring *buffer, 
                            const lList *load_list, const lList *variables);

static const char *
reporting_get_job_log_name(const job_log_t type);

static const char *
lGetStringNotNull(const lListElem *ep, int nm);

/****** qmaster/reporting/--Introduction ***************************
*  NAME
*     qmaster reporting -- creation of a reporting file 
*
*  FUNCTION
*     This module provides a set of functions that are used to write
*     the SGE reporting file and the accounting file.
*
*     See the manpages accounting.5 and qacct.1 for information about the
*     accounting file.
*
*     The reporting file contains entries for queues, hosts, accounting
*     and sharetree usage.
*     It can for example be used to transfer SGE data useful for reporting
*     and analysis purposes to a database.
*
*  SEE ALSO
*     qmaster/reporting/reporting_initialize()
*     qmaster/reporting/reporting_shutdown()
*     qmaster/reporting/reporting_trigger_handler()
*     qmaster/reporting/reporting_create_acct_record()
*     qmaster/reporting/reporting_create_host_record()
*     qmaster/reporting/reporting_create_host_consumable_record()
*     qmaster/reporting/reporting_create_queue_record()
*     qmaster/reporting/reporting_create_queue_consumable_record()
*     qmaster/reporting/reporting_create_sharelog_record()
*******************************************************************************/

/* ------------- public functions ------------- */
/****** qmaster/reporting/reporting_initialize() ***************************
*  NAME
*     reporting_initialize() -- initialize the reporting module
*
*  SYNOPSIS
*     bool reporting_initialize(lList **answer_list) 
*
*  FUNCTION
*     Register reporting and sharelog trigger as well as the respective event
*     handler.
*
*  INPUTS
*     lList **answer_list - used to return error messages
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_initialize() is MT safe.
*
*  SEE ALSO
*     qmaster/reporting/reporting_shutdown()
*     Timeeventmanager/te_add()
*******************************************************************************/
bool
reporting_initialize(lList **answer_list)
{
   bool ret = true;
   te_event_t ev = NULL;

   DENTER(TOP_LAYER, "reporting_initialize");

   te_register_event_handler(reporting_trigger_handler, TYPE_REPORTING_TRIGGER);
   ev = te_new_event(time(NULL), TYPE_REPORTING_TRIGGER, ONE_TIME_EVENT, 1, 0, NULL);
   te_add_event(ev);
   te_free_event(ev);

/* JG: TODO: has also be registered, when global config changed.
 * or do it in reporting_trigger_handler: 
 * - check, if sharelog is registered,
 * - if sharelog_time > 0 && not registered: register
 */
   if (sharelog_time > 0) {
      te_register_event_handler(reporting_trigger_handler, TYPE_SHARELOG_TRIGGER);
      ev = te_new_event(time(NULL), TYPE_SHARELOG_TRIGGER , ONE_TIME_EVENT, 1, 0, NULL);
      te_add_event(ev);
      te_free_event(ev);
   }

   DEXIT;
   return ret;
} /* reporting_initialize() */

/****** qmaster/reporting/reporting_shutdown() *****************************
*  NAME
*     reporting_shutdown() -- shutdown the reporting module
*
*  SYNOPSIS
*     bool reporting_shutdown(lList **answer_list) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list - used to return error messages
*
*  RESULT
*     bool - true on success, false on error.
*
*  NOTES
*     MT-NOTE: reporting_shutdown() is MT safe
*
*  SEE ALSO
*     qmaster/reporting/reporting_initialize()
*******************************************************************************/
bool
reporting_shutdown(lList **answer_list)
{
   bool ret = true;
   lList* alp = NULL;
   u_long32 dummy = 0;
   rep_buf_t *buf;

   DENTER(TOP_LAYER, "reporting_shutdown");

   /* flush the last reporting values, suppress adding new timer */
   if (!reporting_flush(&alp, 0, &dummy)) {
      answer_list_output(&alp);
   }

   buf = &reporting_buffer[ACCOUNTING_BUFFER];
   /* free memory of buffers */
   sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
   sge_dstring_free(&(buf->buffer));
   sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));

   buf = &reporting_buffer[REPORTING_BUFFER];
   sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
   sge_dstring_free(&(buf->buffer));
   sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
   
   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_trigger_handler() **********************
*  NAME
*     reporting_trigger_handler() -- process timed event
*
*  SYNOPSIS
*     void 
*     reporting_trigger_handler(te_event_t anEvent)
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     te_event_t - timed event
*
*  NOTES
*     MT-NOTE: reporting_trigger_handler() is MT safe.
*
*  SEE ALSO
*     Timeeventmanager/te_deliver()
*     Timeeventmanager/te_add()
*******************************************************************************/
void
reporting_trigger_handler(te_event_t anEvent)
{
   u_long32 flush_interval = 0;
   u_long32 next_flush = 0;
   lList *answer_list = NULL;
   

   DENTER(TOP_LAYER, "reporting_trigger_handler");

   switch (te_get_type(anEvent)) {
      case TYPE_SHARELOG_TRIGGER:
         /* dump sharetree usage and flush reporting file */
         if (!reporting_create_sharelog_record(&answer_list)) {
            answer_list_output(&answer_list);
         }
         flush_interval = sharelog_time;
         break;
      case TYPE_REPORTING_TRIGGER:
         /* only flush reporting file */
         flush_interval = reporting_flush_time;
         break;
      default:
         return;
   }

   /* flush the reporting data */
   if (!reporting_flush(&answer_list, time(NULL), &next_flush)) {
      answer_list_output(&answer_list);
   }

   /* set next flushing interval and add timer.
    * flush_interval can be 0, if sharelog is switched off, then don't
    * add trigger 
    */
   if (flush_interval > 0) {
      u_long32 next = time(NULL) + flush_interval;
      te_event_t ev = NULL;

      ev = te_new_event(next, te_get_type(anEvent), ONE_TIME_EVENT, 1, 0, NULL);
      te_add_event(ev);
      te_free_event(ev);
   }

   DEXIT;
   return;
} /* reporting_trigger_handler() */

bool
reporting_create_new_job_record(lList **answer_list, const lListElem *job)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_new_job_record");

   if (do_reporting && do_joblog && job != NULL) {
      dstring job_dstring = DSTRING_INIT;

      u_long32 job_number, priority, submission_time;
      const char *job_name, *owner, *group, *project, *department, *account;

      job_number        = lGetUlong(job, JB_job_number);
      priority          = lGetUlong(job, JB_priority);
      submission_time   = lGetUlong(job, JB_submission_time);
      job_name          = lGetStringNotNull(job, JB_job_name);
      owner             = lGetStringNotNull(job, JB_owner);
      group             = lGetStringNotNull(job, JB_group);
      project           = lGetStringNotNull(job, JB_project);
      department        = lGetStringNotNull(job, JB_department);
      account           = lGetStringNotNull(job, JB_account);

      sge_dstring_sprintf(&job_dstring, U32CFormat"%c"U32CFormat"%c"U32CFormat"%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c"U32CFormat"\n", 
                          submission_time, REPORTING_DELIMITER,
                          job_number, REPORTING_DELIMITER,
                          0, REPORTING_DELIMITER,
                          "none", REPORTING_DELIMITER,
                          job_name, REPORTING_DELIMITER,
                          owner, REPORTING_DELIMITER,
                          group, REPORTING_DELIMITER,
                          project, REPORTING_DELIMITER,
                          department, REPORTING_DELIMITER,
                          account, REPORTING_DELIMITER,
                          priority);

      /* write record to reporting buffer */
      ret = reporting_create_record(answer_list, "new_job", 
                                    sge_dstring_get_string(&job_dstring));
      sge_dstring_free(&job_dstring);
   }

   DEXIT;
   return ret;
}

bool 
reporting_create_job_log(lList **answer_list,
                         u_long32 event_time,
                         const job_log_t type,
                         const char *user,
                         const char *host,
                         const lListElem *job_report,
                         const lListElem *job, const lListElem *ja_task,
                         const lListElem *pe_task,
                         const char *message)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_job_log");

   if (do_reporting && do_joblog && job != NULL) {
      dstring job_dstring = DSTRING_INIT;

      u_long32 job_id = 0, ja_task_id = 0;
      const char *pe_task_id = "none";
      u_long32 state_time = 0, jstate;
      const char *event;
      char state[20];

      job_id = lGetUlong(job, JB_job_number);
      if (ja_task != NULL) {
         ja_task_id = lGetUlong(ja_task, JAT_task_number);
      }
      if (pe_task != NULL) {
         pe_task_id = lGetStringNotNull(pe_task, PET_id);
      }

      /* JG: TODO: implement the whole job/task state mess */
      if (pe_task != NULL) {
         jstate = lGetUlong(pe_task, PET_status);
      } else if (ja_task != NULL) {
         jstate = lGetUlong(ja_task, JAT_status);
      } else {
         jstate = 0;
      }

      event = reporting_get_job_log_name(type);

      *state = '\0';
      job_get_state_string(state, jstate);
      if (message == NULL) {
         message = "";
      }

      sge_dstring_sprintf(&job_dstring, U32CFormat"%c%s%c"U32CFormat"%c"U32CFormat"%c%s%c%s%c%s%c%s%c"U32CFormat"%c%s\n", 
                          event_time, REPORTING_DELIMITER,
                          event, REPORTING_DELIMITER,
                          job_id, REPORTING_DELIMITER,
                          ja_task_id, REPORTING_DELIMITER,
                          pe_task_id, REPORTING_DELIMITER,
                          state, REPORTING_DELIMITER,
                          user, REPORTING_DELIMITER,
                          host, REPORTING_DELIMITER,
                          state_time, REPORTING_DELIMITER,
                          message);
      /* write record to reporting buffer */
      DPRINTF((sge_dstring_get_string(&job_dstring)));
      ret = reporting_create_record(answer_list, "job_log", 
                                    sge_dstring_get_string(&job_dstring));
      sge_dstring_free(&job_dstring);
   }

   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_acct_record() *******************
*  NAME
*     reporting_create_acct_record() -- create an accounting record
*
*  SYNOPSIS
*     bool reporting_create_acct_record(lList **answer_list, 
*                                       lListElem *job_report, 
*                                       lListElem *job, lListElem *ja_task) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - used to report error messages
*     lListElem *job_report - job report from execd
*     lListElem *job        - job referenced in report
*     lListElem *ja_task    - array task that finished
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_acct_record() is not MT safe as the
*              MT safety of called functions sge_build_job_category and
*              sge_write_rusage is not defined.
*******************************************************************************/
bool
reporting_create_acct_record(lList **answer_list, 
                       lListElem *job_report, 
                       lListElem *job, lListElem *ja_task)
{
   bool ret = true;

   char category_buffer[MAX_STRING_SIZE];
   dstring category_dstring;
   dstring job_dstring = DSTRING_INIT;
   const char *category_string = NULL, *job_string = NULL;

   DENTER(TOP_LAYER, "reporting_create_acct_record");

   /* anything to do at all? */
   if (do_reporting || do_accounting) {
      sge_dstring_init(&category_dstring, category_buffer, 
                       sizeof(category_buffer));
      category_string = sge_build_job_category(&category_dstring, job, 
                                          *(userset_list_get_master_list()));
   }

   if (do_accounting || do_reporting){
      job_string = sge_write_rusage(&job_dstring, job_report, job, ja_task, 
                                    category_string, REPORTING_DELIMITER);
   }

   /* create record for accounting file */
   if (do_accounting) {
      if (job_string == NULL) {
         ret = false;
      } else {
         /* write accounting file */
         rep_buf_t *buf = &reporting_buffer[ACCOUNTING_BUFFER];
         sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
         sge_dstring_append(&(buf->buffer), job_string);
         sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
      }
   }

   if (ret && do_reporting) {
      if (job_string == NULL) {
         ret = false;
      } else {
         ret = reporting_create_record(answer_list, "acct", job_string);
      }
   }

   sge_dstring_free(&job_dstring);

   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_write_consumables() ********************
*  NAME
*     reporting_write_consumables() -- dump consumables to a buffer
*
*  SYNOPSIS
*     bool reporting_write_consumables(lList **answer_list, dstring *buffer, 
*                                      const lList *actual, const lList *total) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list - used to return error messages
*     dstring *buffer     - target buffer
*     const lList *actual - actual consumable values
*     const lList *total  - configured consumable values
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_write_consumables() is MT safe
*******************************************************************************/
bool
reporting_write_consumables(lList **answer_list, dstring *buffer,
                            const lList *actual, const lList *total)
{
   bool ret = true;
   lListElem *cep; 
   
   DENTER(TOP_LAYER, "reporting_write_consumables");

   if (do_reporting) {
      for_each (cep, actual) {
         lListElem *tep = lGetElemStr(total, CE_name, lGetString(cep, CE_name));
         if (tep != NULL) {
            sge_dstring_append(buffer, lGetString(cep, CE_name));
            sge_dstring_append_char(buffer, '=');
            centry_print_resource_to_dstring(cep, buffer);
            sge_dstring_append_char(buffer, '=');
            centry_print_resource_to_dstring(tep, buffer);

            if (lNext(cep)) {
               sge_dstring_append_char(buffer, ','); 
            }
         }
      }
   }

   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_queue_record() *******************
*  NAME
*     reporting_create_queue_record() -- create queue reporting record
*
*  SYNOPSIS
*     bool 
*     reporting_create_queue_record(lList **answer_list, 
*                                  const lListElem *queue, 
*                                  u_long32 report_time) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     const lListElem *queue - the queue to output
*     u_long32 report_time  - time of the last load report
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_queue_record() is MT safe
*******************************************************************************/
bool
reporting_create_queue_record(lList **answer_list,
                             const lListElem *queue,
                             u_long32 report_time)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_queue_record");

   if (do_reporting && queue != NULL) {
      dstring queue_dstring = DSTRING_INIT;
      char state_buffer[20];
      *state_buffer = '\0';
      queue_or_job_get_states(QU_state, state_buffer, 
                              lGetUlong(queue, QU_state));

      sge_dstring_sprintf(&queue_dstring, "%s%c%s%c"U32CFormat"%c%s\n", 
                          lGetString(queue, QU_qname),
                          REPORTING_DELIMITER,
                          lGetHost(queue, QU_qhostname),
                          REPORTING_DELIMITER,
                          report_time,
                          REPORTING_DELIMITER,
                          state_buffer);

      /* write record to reporting buffer */
      ret = reporting_create_record(answer_list, "queue", 
                                    sge_dstring_get_string(&queue_dstring));

      sge_dstring_free(&queue_dstring);
   }

   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_queue_consumable_record() ********
*  NAME
*     reporting_create_queue_consumable_record() -- write queue consumables
*
*  SYNOPSIS
*     bool 
*     reporting_create_queue_consumable_record(lList **answer_list, 
*                                             const lListElem *queue, 
*                                             u_long32 report_time) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     const lListElem *queue - queue to output
*     u_long32 report_time  - time when consumables changed
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_queue_consumable_record() is MT safe
*******************************************************************************/
bool
reporting_create_queue_consumable_record(lList **answer_list,
                                        const lListElem *queue,
                                        u_long32 report_time)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_queue_consumable_record");

   if (do_reporting && queue != NULL) {
      dstring consumable_dstring = DSTRING_INIT;

      /* dump consumables */
      reporting_write_consumables(answer_list, &consumable_dstring, 
                                  lGetList(queue, QU_consumable_actual_list), 
                                  lGetList(queue, QU_consumable_config_list));

      if (sge_dstring_strlen(&consumable_dstring) > 0) {
         dstring queue_dstring = DSTRING_INIT;
         char state_buffer[20];
         *state_buffer = '\0';
         queue_or_job_get_states(QU_state, state_buffer, 
                                 lGetUlong(queue, QU_state));

         sge_dstring_sprintf(&queue_dstring, "%s%c%s%c"U32CFormat"%c%s%c%s\n", 
                             lGetString(queue, QU_qname),
                             REPORTING_DELIMITER,
                             lGetHost(queue, QU_qhostname),
                             REPORTING_DELIMITER,
                             report_time,
                             REPORTING_DELIMITER,
                             state_buffer, 
                             REPORTING_DELIMITER,
                             sge_dstring_get_string(&consumable_dstring));


         /* write record to reporting buffer */
         ret = reporting_create_record(answer_list, "queue_consumable", 
                                       sge_dstring_get_string(&queue_dstring));
         sge_dstring_free(&queue_dstring);
      }

      sge_dstring_free(&consumable_dstring);
   }


   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_host_record() *******************
*  NAME
*     reporting_create_host_record() -- create host reporting record
*
*  SYNOPSIS
*     bool 
*     reporting_create_host_record(lList **answer_list, 
*                                  const lListElem *host, 
*                                  u_long32 report_time) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     const lListElem *host - the host to output
*     u_long32 report_time  - time of the last load report
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_host_record() is MT safe
*******************************************************************************/
bool
reporting_create_host_record(lList **answer_list,
                             const lListElem *host,
                             u_long32 report_time)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_host_record");

   if (do_reporting && host != NULL) {
      dstring load_dstring = DSTRING_INIT;

      /* dump load values */
      /* JG: TODO: we need a merged variable list that contains the variable
       * lists from global and local host - or postpone this until a mechnism
       * similar to cluster_queues is found? 
       */
      reporting_write_load_values(answer_list, &load_dstring, 
                                  lGetList(host, EH_load_list), 
                                  lGetList(host, EH_report_variables));

      /* As long as we have no host status information, dump host data only if we have
       * load values to report.
       */
      if (sge_dstring_strlen(&load_dstring) > 0) {
         dstring host_dstring = DSTRING_INIT;
         sge_dstring_sprintf(&host_dstring, "%s%c"U32CFormat"%c%s%c%s\n", 
                             lGetHost(host, EH_name),
                             REPORTING_DELIMITER,
                             report_time,
                             REPORTING_DELIMITER,
                             "X",
                             REPORTING_DELIMITER,
                             sge_dstring_get_string(&load_dstring));
         /* write record to reporting buffer */
         ret = reporting_create_record(answer_list, "host", 
                                       sge_dstring_get_string(&host_dstring));

         sge_dstring_free(&host_dstring);
      }

      sge_dstring_free(&load_dstring);
   }

   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_host_consumable_record() ********
*  NAME
*     reporting_create_host_consumable_record() -- write host consumables
*
*  SYNOPSIS
*     bool 
*     reporting_create_host_consumable_record(lList **answer_list, 
*                                             const lListElem *host, 
*                                             u_long32 report_time) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list   - used to return error messages
*     const lListElem *host - host to output
*     u_long32 report_time  - time when consumables changed
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_host_consumable_record() is MT safe
*******************************************************************************/
bool
reporting_create_host_consumable_record(lList **answer_list,
                                        const lListElem *host,
                                        u_long32 report_time)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_host_consumable_record");

   if (do_reporting && host != NULL) {
      dstring consumable_dstring = DSTRING_INIT;

      /* dump consumables */
      reporting_write_consumables(answer_list, &consumable_dstring, 
                                  lGetList(host, EH_consumable_actual_list), 
                                  lGetList(host, EH_consumable_config_list));

      if (sge_dstring_strlen(&consumable_dstring) > 0) {
         dstring host_dstring = DSTRING_INIT;

         sge_dstring_sprintf(&host_dstring, "%s%c"U32CFormat"%c%s%c%s\n", 
                             lGetHost(host, EH_name),
                             REPORTING_DELIMITER,
                             report_time,
                             REPORTING_DELIMITER,
                             "X",
                             REPORTING_DELIMITER,
                             sge_dstring_get_string(&consumable_dstring));


         /* write record to reporting buffer */
         ret = reporting_create_record(answer_list, "host_consumable", 
                                       sge_dstring_get_string(&host_dstring));
         sge_dstring_free(&host_dstring);
      }

      sge_dstring_free(&consumable_dstring);
   }


   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_create_sharelog_record() ***************
*  NAME
*     reporting_create_sharelog_record() -- dump sharetree usage
*
*  SYNOPSIS
*     bool 
*     reporting_create_sharelog_record(lList **answer_list) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     lList **answer_list - used to return error messages
*
*  RESULT
*     bool -  true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_create_sharelog_record() is most probably MT safe
*              (depends on sge_sharetree_print with uncertain MT safety)
*******************************************************************************/
bool
reporting_create_sharelog_record(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "reporting_create_sharelog_record");

   if (do_reporting && sharelog_time > 0) {
      /* only create sharelog entries if we have a sharetree */
      if (lGetNumberOfElem(Master_Sharetree_List) > 0) {
         rep_buf_t *buf;
         dstring prefix_dstring = DSTRING_INIT;
         dstring data_dstring   = DSTRING_INIT;
         format_t format;
         char delim[2];
         delim[0] = REPORTING_DELIMITER;
         delim[1] = '\0';

         /* we need a prefix containing the reporting file std fields */
         sge_dstring_sprintf(&prefix_dstring, U32CFormat"%csharelog%c",
                             sge_get_gmt(),
                             REPORTING_DELIMITER, REPORTING_DELIMITER);

         /* define output format */
         format.name_format  = false;
         format.delim        = delim;
         format.line_delim   = "\n";
         format.rec_delim    = "";
         format.str_format   = "%s";
         format.field_names  = NULL;
         format.format_times = false;
         format.line_prefix  = sge_dstring_get_string(&prefix_dstring);

         /* dump the sharetree data */
         SGE_LOCK(LOCK_MASTER_SHARETREE_LST, LOCK_WRITE);
         SGE_LOCK(LOCK_MASTER_USER_LST, LOCK_READ);
         SGE_LOCK(LOCK_MASTER_PROJECT_LST, LOCK_READ);

         sge_sharetree_print(&data_dstring, Master_Sharetree_List, 
                             Master_User_List,
                             Master_Project_List,
                             true,
                             false,
                             NULL,
                             &format);
         SGE_UNLOCK(LOCK_MASTER_PROJECT_LST, LOCK_READ);
         SGE_UNLOCK(LOCK_MASTER_USER_LST, LOCK_READ);
         SGE_UNLOCK(LOCK_MASTER_SHARETREE_LST, LOCK_WRITE);

         /* write data to reporting buffer */
         buf = &reporting_buffer[REPORTING_BUFFER];
         sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
         sge_dstring_append(&(buf->buffer),
                            sge_dstring_get_string(&data_dstring));
         sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));

         /* cleanup */
         sge_dstring_free(&prefix_dstring);
         sge_dstring_free(&data_dstring);
      }
   }

   DEXIT;
   return ret;
}

/* ----- static functions ----- */

/*
* NOTES
*     MT-NOTE: reporting_write_load_values() is MT-safe
*/
static bool
reporting_write_load_values(lList **answer_list, dstring *buffer, 
                            const lList *load_list, const lList *variables)
{
   bool ret = true;
   bool first = true;
   const lListElem *variable;

   DENTER(TOP_LAYER, "reporting_write_load_values");

   for_each (variable, variables) {
      const char *name;
      const lListElem *load;

      name = lGetString(variable, STU_name);
      load = lGetElemStr(load_list, HL_name, name);
      if (load != NULL) {
         if (first) {
            first = false;
         } else {
            sge_dstring_append_char(buffer, ',');
         }
         sge_dstring_sprintf_append(buffer, "%s=%s", 
                                    name, lGetString(load, HL_value));
      }

   }

   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_create_record() is MT-safe
*/
static bool 
reporting_create_record(lList **answer_list, 
                        const char *type,
                        const char *data)
{
   bool ret = true;
   rep_buf_t *buf = &reporting_buffer[REPORTING_BUFFER];

   DENTER(TOP_LAYER, "reporting_create_record");

   sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
   sge_dstring_sprintf_append(&(buf->buffer), U32CFormat"%c%s%c%s",
                              sge_get_gmt(),
                              REPORTING_DELIMITER,
                              type,
                              REPORTING_DELIMITER,
                              data,
                              REPORTING_DELIMITER);
   sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));

   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_flush_accounting() is MT-safe
*/
static bool 
reporting_flush_accounting(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_flush_accounting");

   /* write accounting data */
   ret = reporting_flush_report_file(answer_list, path_state_get_acct_file(),
                                     &reporting_buffer[ACCOUNTING_BUFFER]);
   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_flush_reporting() is MT-safe
*/
static bool 
reporting_flush_reporting(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_flush_reporting");

   /* write reporting data */
   ret = reporting_flush_report_file(answer_list,
                                     path_state_get_reporting_file(),
                                     &reporting_buffer[REPORTING_BUFFER]);
   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_flush_report_file() is MT-safe
*/
static bool 
reporting_flush_report_file(lList **answer_list,
                      const char *filename, 
                      rep_buf_t *buf)
{
   bool ret = true;
   
   size_t size;

   DENTER(TOP_LAYER, "reporting_flush_report_file");

   size = sge_dstring_strlen(&(buf->buffer));

   /* do we have anything to write? */ 
   if (size > 0) {
      FILE *fp;
      bool write_comment = false;
      SGE_STRUCT_STAT statbuf;
      char error_buffer[MAX_STRING_SIZE];
      dstring error_dstring;

      sge_dstring_init(&error_dstring, error_buffer, sizeof(error_buffer));

      /* if file doesn't exist: write a comment after creating it */
      if (SGE_STAT(filename, &statbuf)) {
         write_comment = true;
      }     

      sge_mutex_lock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));

      /* open file for append */
      fp = fopen(filename, "a");
      if (fp == NULL) {
         if (answer_list == NULL) {
            ERROR((SGE_EVENT, MSG_ERROROPENINGFILEFORWRITING_SS, filename, 
                   sge_strerror(errno, &error_dstring)));
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERROROPENINGFILEFORWRITING_SS, filename, 
                                    sge_strerror(errno, &error_dstring));
         }

         ret = false;
      }

      /* write comment if necessary */
      if (ret) {
         if (write_comment) {
            int spool_ret;
            char version_buffer[MAX_STRING_SIZE];
            dstring version_dstring;
            const char *version_string;

            sge_dstring_init(&version_dstring, version_buffer, 
                             sizeof(version_buffer));
            version_string = feature_get_product_name(FS_VERSION, 
                                                      &version_dstring);

            spool_ret = sge_spoolmsg_write(fp, COMMENT_CHAR, version_string);
            if (spool_ret != 0) {
               if (answer_list == NULL) {
                  ERROR((SGE_EVENT, MSG_ERROR_WRITINGFILE_SS, filename, 
                         sge_strerror(errno, &error_dstring)));
               } else {
                  answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                          ANSWER_QUALITY_ERROR, 
                                          MSG_ERROR_WRITINGFILE_SS, filename, 
                                          sge_strerror(errno, &error_dstring));
               } 
               ret = false;
            }
         }
      }

      /* write data */
      if (ret) {
         if (fwrite(sge_dstring_get_string(&(buf->buffer)), size, 1, fp) != 1) {
            if (answer_list == NULL) {
               ERROR((SGE_EVENT, MSG_ERROR_WRITINGFILE_SS, filename, 
                      sge_strerror(errno, &error_dstring)));
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERROR_WRITINGFILE_SS, filename, 
                                       sge_strerror(errno, &error_dstring));
            }

            ret = false;
         }
      }

      /* close file */
      if (fclose(fp) != 0) {
         if (answer_list == NULL) {
            ERROR((SGE_EVENT, MSG_ERRORCLOSINGFILE_SS, filename, 
                   sge_strerror(errno, &error_dstring)));
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORCLOSINGFILE_SS, filename, 
                                    sge_strerror(errno, &error_dstring));
         }
         ret = false;
      }

      /* clear the buffer. We do this regardless of the result of
       * the writing command. Otherwise, if writing the report file failed
       * over a longer time period, the reporting buffer could grow endlessly.
       */
      sge_dstring_clear(&(buf->buffer));

      sge_mutex_unlock(buf->mtx_name, SGE_FUNC, __LINE__, &(buf->mtx));
   }

   DEXIT;
   return ret;
}
/*
* NOTES
*     MT-NOTE: reporting_flush() is MT-safe
*/
static bool 
reporting_flush(lList **answer_list, u_long32 flush, u_long32 *next_flush)
{
   bool ret = true;
   bool reporting_ret;

   DENTER(TOP_LAYER, "reporting_flush");

   /* flush accounting data */
   reporting_ret = reporting_flush_accounting(answer_list);
   if (!reporting_ret) {
      ret = false;
   }
     
   /* flush accounting data */
   reporting_ret = reporting_flush_reporting(answer_list);
   if (!reporting_ret) {
      ret = false;
   }
     
   /* set time for next flush */  
   *next_flush = flush + reporting_flush_time;

   DEXIT;
   return ret;
}

static const char *
reporting_get_job_log_name(const job_log_t type)
{
   /* JG: TODO: using a switch() would be safer! */
   static const char *names[] = {
      "unknown",
      "pending",
      "sent",
      "resent",
      "delivered",
      "running",
      "suspended",
      "unsuspended",
      "held",
      "released",
      "restart",
      "migrate",
      "deleted",
      "finished"
   };

   return names[type];
}

static const char *
lGetStringNotNull(const lListElem *ep, int nm)
{
   const char *ret = lGetString(ep, nm);
   if (ret == NULL) {
      ret = "";
   }
   return ret;
}

