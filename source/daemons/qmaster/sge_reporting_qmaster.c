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
#include <time.h>

#include "sgermon.h"
#include "sge_log.h"
#include "sge_dstring.h"
#include "setup_path.h"
#include "sge_stdlib.h"
#include "sge_unistd.h"
#include "sge_spool.h"

#include "category.h"

#include "sge_answer.h"
#include "sge_feature.h"
#include "sge_object.h"
#include "sge_userset.h"

#include "sge_lock.h"

#include "sge_rusage.h"
#include "time_event.h"
#include "sge_reporting_qmaster.h"

#include "msg_common.h"

/* flush time in seconds 
 * JG: TODO: this should be a reporting config parameter.
 */
#define REPORTING_FLUSH_TIME 60

/* do we need to write an accounting file?
 * JG: TODO: this should be a reporting config parameter
 */
#define REPORTING_WRITE_ACCOUNTING_FILE false

/* global dstring for accounting data */
static dstring accounting_data = DSTRING_INIT;

/* global dstring for reporting data */
static dstring reporting_data = DSTRING_INIT;

bool
sge_initialize_reporting(lList **answer_list)
{
   bool ret = true;

   time_t now = time(0);

   DENTER(TOP_LAYER, "sge_initialize_reporting");

   /* JG: TODO: analyze reporting configuration */

   te_add(TYPE_REPORTING_TRIGGER, now, 0, 0, NULL);

   DEXIT;
   return ret;
}

bool
sge_shutdown_reporting(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_shutdown_reporting");

   /* flush the last reporting values */
   deliver_reporting_trigger(TYPE_REPORTING_TRIGGER, 0, 0, 0, NULL);

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

void
deliver_reporting_trigger(u_long32 type, u_long32 when, 
                          u_long32 uval0, u_long32 uval1, const char *key)
{
   time_t next_flush = 0;
   time_t now;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "deliver_reporting_trigger");

   /* flush the reporting data */
   if (!sge_flush_reporting(&answer_list, when, &next_flush)) {
      answer_list_output(&answer_list);
   }

   /* validate next_trigger. If it is invalid, set it to one minute after now */
   now = time(0);
   if (next_flush <= now) {
      next_flush = now + 60;
   }

   /* set timerevent for next flush */
   te_add(type, next_flush, 0, 0, NULL);

   DEXIT;
   return;
}

bool
sge_create_acct_record(lList **answer_list, 
                       lListElem *job_report, 
                       lListElem *job, lListElem *ja_task)
{
   bool ret = true;

   char category_buffer[MAX_STRING_SIZE], job_buffer[MAX_STRING_SIZE];
   dstring category_dstring, job_dstring;
   const char *category_string, *job_string;

   DENTER(TOP_LAYER, "sge_create_acct_record");

   sge_dstring_init(&category_dstring, category_buffer, 
                    sizeof(category_dstring));
   sge_dstring_init(&job_dstring, job_buffer, 
                    sizeof(job_dstring));
   category_string = sge_build_job_category(&category_dstring, job, 
                                            *(userset_list_get_master_list()));
   job_string = sge_write_rusage(&job_dstring, job_report, job, ja_task, 
                                 category_string);
   if (job_string == NULL) {
      ret = false;
   } else {
      if (REPORTING_WRITE_ACCOUNTING_FILE) {
         /* write accounting file */
         SGE_LOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
         sge_dstring_append(&accounting_data, job_string);
         SGE_UNLOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
      }

      /* write reporting file */
      ret = sge_create_reporting_record(answer_list, SGE_TYPE_JOB, job_string);
   }

   DEXIT;
   return ret;
}

bool 
sge_create_reporting_record(lList **answer_list, 
                            sge_object_type object_type,
                            const char *data)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_create_reporting_record");

   SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   sge_dstring_sprintf_append(&reporting_data, "%ld:%s:%s",
                              time(0),
                              object_type_get_name(object_type),
                              data);
   SGE_UNLOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);

   DEXIT;
   return ret;
}

bool 
sge_flush_accounting_data(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_flush_accounting");

   if (REPORTING_WRITE_ACCOUNTING_FILE) {
      SGE_LOCK(LOCK_MASTER_ACCOUNTING_BUFFER, LOCK_WRITE);
      /* write accounting data */
      ret = sge_flush_report_file(answer_list, &accounting_data, 
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

bool 
sge_flush_reporting_data(lList **answer_list)
{
   bool ret = true;

   DENTER(TOP_LAYER, "sge_flush_accounting");

   SGE_LOCK(LOCK_MASTER_REPORTING_BUFFER, LOCK_WRITE);
   /* write accounting data */
   ret = sge_flush_report_file(answer_list, &reporting_data, 
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

bool 
sge_flush_report_file(lList **answer_list, dstring *contents, 
                      const char *filename)
{
   bool ret = true;
   
   size_t size;

   DENTER(TOP_LAYER, "sge_create_acct_record");

   size = sge_dstring_strlen(contents);

   /* do we have anything to write? */ 
   if (size > 0) {
      FILE *fp;
      bool write_comment = false;
      SGE_STRUCT_STAT statbuf;

      /* if file doesn't exist: write a comment after creating it */
      if (SGE_STAT(filename, &statbuf)) {
         write_comment = true;
      }     

      /* open file for append */
      fp = fopen(filename, "a");
      if (fp == NULL) {
         if (answer_list == NULL) {
            ERROR((SGE_EVENT, MSG_ERROROPENINGFILEFORWRITING_SS, filename, 
                   strerror(errno)));
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERROROPENINGFILEFORWRITING_SS, filename, 
                                    strerror(errno));
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
                  ERROR((SGE_EVENT, MSG_ERROR_WRITINGFILE_SS, filename, strerror(errno)));
               } else {
                  answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                          ANSWER_QUALITY_ERROR, 
                                          MSG_ERROR_WRITINGFILE_SS, filename, 
                                          strerror(errno));
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
                      strerror(errno)));
            } else {
               answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                       ANSWER_QUALITY_ERROR, 
                                       MSG_ERROR_WRITINGFILE_SS, filename, 
                                       strerror(errno));
            }

            ret = false;
         }
      }

      /* close file */
      if (fclose(fp) != 0) {
         if (answer_list == NULL) {
            ERROR((SGE_EVENT, MSG_ERRORCLOSINGFILE_SS, filename, 
                   strerror(errno)));
         } else {
            answer_list_add_sprintf(answer_list, STATUS_EDISK, 
                                    ANSWER_QUALITY_ERROR, 
                                    MSG_ERRORCLOSINGFILE_SS, filename, 
                                    strerror(errno));
         }
         ret = false;
      }
   }

   DEXIT;
   return ret;
}

bool 
sge_flush_reporting(lList **answer_list, time_t flush, time_t *next_flush)
{
   bool ret = true;
   bool reporting_ret;

   DENTER(TOP_LAYER, "sge_flush_reporting");

   /* flush accounting data */
   reporting_ret = sge_flush_accounting_data(answer_list);
   if (!reporting_ret) {
      ret = false;
   }
     
   /* flush accounting data */
   reporting_ret = sge_flush_reporting_data(answer_list);
   if (!reporting_ret) {
      ret = false;
   }
     
   /* set time for next flush */  
   *next_flush = flush + REPORTING_FLUSH_TIME;

   DEXIT;
   return ret;
}

