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

/* daemons/common */
#include "category.h"

/* sgeobj */
#include "sge_answer.h"
#include "sge_feature.h"
#include "sge_object.h"
#include "sge_centry.h"
#include "sge_userset.h"
#include "sge_host.h"
#include "sge_str.h"
#include "sge_sharetree.h"
#include "sge_userprj.h"

/* lck */
#include "sge_lock.h"

/* sched */
#include "sge_sharetree_printing.h"

/* local */
#include "sge_rusage.h"
#include "time_event.h"
#include "sge_reporting_qmaster.h"

/* messages */
#include "msg_common.h"

/* flush time in seconds 
 * JG: TODO: this should be a reporting config parameter.
 */
#define REPORTING_FLUSH_TIME 60
#define SHARELOG_FLUSH_TIME  600

/* do we need to write an accounting file?
 * JG: TODO: this should be a reporting config parameter
 */
#define REPORTING_WRITE_ACCOUNTING_FILE true

static const char REPORTING_DELIMITER = '\t';

/* global dstring for accounting data */
static dstring accounting_data = DSTRING_INIT;

/* global dstring for reporting data */
static dstring reporting_data = DSTRING_INIT;

static bool 
reporting_flush_accounting(lList **answer_list);

static bool 
reporting_flush_reporting(lList **answer_list);

static bool 
reporting_flush_report_file(lList **answer_list, dstring *contents, 
                      const char *filename);

static bool 
reporting_flush(lList **answer_list, u_long32 flush, u_long32 *next_flush);

static bool 
reporting_create_record(lList **answer_list, 
                        const char *type,
                        const char *data);

static bool
reporting_write_load_values(lList **answer_list, dstring *buffer, 
                            const lList *load_list, const lList *variables);


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
*     qmaster/reporting/reporting_deliver_trigger()
*     qmaster/reporting/reporting_create_acct_record()
*     qmaster/reporting/reporting_create_host_record()
*     qmaster/reporting/reporting_create_host_consumable_record()
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
*     ??? 
*
*  INPUTS
*     lList **answer_list - used to return error messages
*
*  RESULT
*     bool - true on success, false on error
*
*  NOTES
*     MT-NOTE: reporting_initialize() is not MT safe, as te_add is not MT safe.
*
*  SEE ALSO
*     qmaster/reporting/reporting_shutdown()
*     Timeeventmanager/te_add()
*******************************************************************************/
bool
reporting_initialize(lList **answer_list)
{
   bool ret = true;

   u_long32 now = sge_get_gmt();

   DENTER(TOP_LAYER, "reporting_initialize");

   /* JG: TODO: analyze reporting configuration */

   te_add(TYPE_REPORTING_TRIGGER, now, 1, 0, NULL);
   te_add(TYPE_SHARELOG_TRIGGER, now, 1, 0, NULL);

   DEXIT;
   return ret;
}

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

   DENTER(TOP_LAYER, "reporting_shutdown");

   /* flush the last reporting values, suppress adding new timer */
   reporting_deliver_trigger(TYPE_REPORTING_TRIGGER, 0, 0, 0, NULL);

   /* free memory of buffers */
   SGE_LOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
   sge_dstring_free(&accounting_data);
   SGE_UNLOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);

   SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   sge_dstring_free(&reporting_data);
   SGE_UNLOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   
   DEXIT;
   return ret;
}

/****** qmaster/reporting/reporting_deliver_trigger() **********************
*  NAME
*     reporting_deliver_trigger() -- process timer event
*
*  SYNOPSIS
*     void 
*     reporting_deliver_trigger(u_long32 type, u_long32 when, 
*                               u_long32 uval0, u_long32 uval1, const char *key)
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     u_long32 type   - Event type (TYPE_REPORTING_TRIGGER)
*     u_long32 when   - The time when the event is due to deliver.
*     u_long32 uval0  - if != 0, a new timer will be started
*     u_long32 uval1  - unused
*     const char *key - unused
*
*  NOTES
*     MT-NOTE: reporting_deliver_trigger() is MT safe if uval0 = 0, else it is
*              not MT safe, as te_add is not MT safe.
*
*  SEE ALSO
*     Timeeventmanager/te_deliver()
*     Timeeventmanager/te_add()
*******************************************************************************/
void
reporting_deliver_trigger(u_long32 type, u_long32 when, 
                          u_long32 uval0, u_long32 uval1, const char *key)
{
   u_long32 flush_interval = 0;
   u_long32 next_flush = 0;
   u_long32 now;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "reporting_deliver_trigger");

   switch (type) {
      case TYPE_SHARELOG_TRIGGER:
         /* dump sharetree usage and flush reporting file */
         if (!reporting_create_sharelog_record(&answer_list)) {
            answer_list_output(&answer_list);
         }
         flush_interval = SHARELOG_FLUSH_TIME;
         break;
      case TYPE_REPORTING_TRIGGER:
         /* only flush reporting file */
         flush_interval = REPORTING_FLUSH_TIME;
         break;
      default:
         return;
   }

   /* flush the reporting data */
   if (!reporting_flush(&answer_list, when, &next_flush)) {
      answer_list_output(&answer_list);
   }

   /* validate next_trigger. If it is invalid, set it to one minute after now */
   now = sge_get_gmt();
   next_flush = now + flush_interval;

   /* set timerevent for next flush */
   if (uval0 != 0) {
      te_add(type, next_flush, 1, 0, NULL);
   }

   DEXIT;
   return;
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
   const char *category_string, *job_string;

   DENTER(TOP_LAYER, "reporting_create_acct_record");

   sge_dstring_init(&category_dstring, category_buffer, 
                    sizeof(category_dstring));
   category_string = sge_build_job_category(&category_dstring, job, 
                                            *(userset_list_get_master_list()));
   /* create record for accounting file */
   if (REPORTING_WRITE_ACCOUNTING_FILE) {
      job_string = sge_write_rusage(&job_dstring, job_report, job, ja_task, 
                                    category_string, ':');
      if (job_string == NULL) {
         ret = false;
      } else {
         /* write accounting file */
         SGE_LOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
         sge_dstring_append(&accounting_data, job_string);
         SGE_UNLOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
      }
   }

   if (ret) {
      /* create record in reporting file */
      job_string = sge_write_rusage(&job_dstring, job_report, job, ja_task, 
                                    category_string, REPORTING_DELIMITER);
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

   if (host != NULL) {
      dstring host_dstring = DSTRING_INIT;

      sge_dstring_sprintf(&host_dstring, "%s%c"U32CFormat"%c%s%c", 
                          lGetHost(host, EH_name),
                          REPORTING_DELIMITER,
                          report_time,
                          REPORTING_DELIMITER,
                          "X",
                          REPORTING_DELIMITER);
      /* dump load values */
      /* JG: TODO: we need a merged variable list that contains the variable
       * lists from global and local host - or postpone this until a mechnism
       * similar to cluster_queues is found? 
       */
      reporting_write_load_values(answer_list, &host_dstring, 
                                  lGetList(host, EH_load_list), 
                                  lGetList(host, EH_report_variables));

      /* write record to reporting buffer */
      sge_dstring_append(&host_dstring, "\n");
      ret = reporting_create_record(answer_list, "host", 
                                    sge_dstring_get_string(&host_dstring));
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

   DENTER(TOP_LAYER, "reporting_create_host_record");

   if (host != NULL) {
      dstring host_dstring = DSTRING_INIT;

      sge_dstring_sprintf(&host_dstring, "%s%c"U32CFormat"%c%s%c", 
                          lGetHost(host, EH_name),
                          REPORTING_DELIMITER,
                          report_time,
                          REPORTING_DELIMITER,
                          "X",
                          REPORTING_DELIMITER);

      /* dump consumables */
      reporting_write_consumables(answer_list, &host_dstring, 
                                  lGetList(host, EH_consumable_actual_list), 
                                  lGetList(host, EH_consumable_config_list));

      /* write record to reporting buffer */
      sge_dstring_append(&host_dstring, "\n");
      ret = reporting_create_record(answer_list, "host_consumable", 
                                    sge_dstring_get_string(&host_dstring));
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

   /* only create sharelog entries if we have a sharetree */
   if (lGetNumberOfElem(Master_Sharetree_List) > 0) {
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
      SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
      sge_dstring_append(&reporting_data,
                         sge_dstring_get_string(&data_dstring));
      SGE_UNLOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);

      /* cleanup */
      sge_dstring_free(&prefix_dstring);
      sge_dstring_free(&data_dstring);
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

   DENTER(TOP_LAYER, "reporting_create_record");

   SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   sge_dstring_sprintf_append(&reporting_data, U32CFormat"%c%s%c%s",
                              sge_get_gmt(),
                              REPORTING_DELIMITER,
                              type,
                              REPORTING_DELIMITER,
                              data,
                              REPORTING_DELIMITER);
   SGE_UNLOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);

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

   if (REPORTING_WRITE_ACCOUNTING_FILE) {
      SGE_LOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
      /* write accounting data */
      ret = reporting_flush_report_file(answer_list, &accounting_data, 
                                  path_state_get_acct_file());
      /* clear accounting buffer. We do this regardless of the result of
       * the writing command. Otherwise, if writing the report file failed
       * over a longer time period, the reporting buffer could grow endlessly.
       */
      sge_dstring_clear(&accounting_data);
      SGE_UNLOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
   }
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

   DENTER(TOP_LAYER, "sge_flush_accounting");

   SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   /* write accounting data */
   ret = reporting_flush_report_file(answer_list, &reporting_data, 
                               path_state_get_reporting_file());
   /* clear accounting buffer. We do this regardless of the result of
    * the writing command. Otherwise, if writing the report file failed
    * over a longer time period, the reporting buffer could grow endlessly.
    */
   sge_dstring_clear(&reporting_data);
   SGE_UNLOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);

   DEXIT;
   return ret;
}

/*
* NOTES
*     MT-NOTE: reporting_flush_report_file() is MT-safe
*/
static bool 
reporting_flush_report_file(lList **answer_list, dstring *contents, 
                      const char *filename)
{
   bool ret = true;
   
   size_t size;

   DENTER(TOP_LAYER, "reporting_flush_report_file");

   size = sge_dstring_strlen(contents);

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
         if (fwrite(sge_dstring_get_string(contents), size, 1, fp) != 1) {
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
   *next_flush = flush + REPORTING_FLUSH_TIME;

   DEXIT;
   return ret;
}

