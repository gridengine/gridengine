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
#include <stdarg.h>
#include <stdio.h>

#include "cull.h"
#include "sge_messageL.h"
#include "schedd_message.h"
#include "sgermon.h"
#include "schedd_monitor.h"
#include "sge_log.h"
#include "sge_ulongL.h"
#include "sge_schedd_text.h"
#include "msg_schedd.h"
#include "sge_range.h"
#include "sge_job.h"
#include "sge_schedd_conf.h"
#include "sge_ctL.h"

static void schedd_mes_find_others(lList *job_list, int category);

static lList *schedd_mes_get_same_category_jids(lRef category,
                                                lList *job_list,
                                                int ignore_category);

static lRef schedd_mes_get_category(u_long32 job_id, lList *job_list);

/* 
 * Message structure where job scheduling informations are stored 
 */
static lListElem *sme = NULL;
static lListElem *tmp_sme = NULL;

/* 
 * write scheduling informaten into logfile
 */
static int log_schedd_info = 1;

static void schedd_mes_find_others(lList *job_list,
                                   int ignore_category)
{
   if (tmp_sme && job_list) {
      lListElem *message_elem = NULL;  /* MES_Type */
      lRef category = NULL;            /* Category pointer (void*) */
      lList *jid_cat_list = NULL;      /* ULNG */
      int create_new_jid_cat_list = 0;
      lList *message_list = lGetList(tmp_sme, SME_message_list);

      /*
       * Here we have a list of message elements where each 
       * MES_job_number_list containes only one id.
       * We have to find the other jobs (jids) which have the same category.
       */
      for_each(message_elem, message_list) {
         lList *jid_list = lGetList(message_elem, MES_job_number_list);
         u_long32 jid;
         lRef jid_category; 
         jid = lGetUlong(lFirst(jid_list), ULNG);
         jid_category = schedd_mes_get_category(jid, job_list);

         /*
          * Initilize jid_cat_list if not initialized
          * or if category differs from the last run
          */
         if (category != jid_category || ignore_category) {
            jid_cat_list = schedd_mes_get_same_category_jids(jid_category,
                                                             job_list,
                                                             ignore_category);
            category = jid_category;
            create_new_jid_cat_list = 0;
         }
         else {
            create_new_jid_cat_list = 1;
         }

         /*
          * Replace the MES_job_number_list which containes only one jid
          * with a list of jids (all have same category
          */
         if (create_new_jid_cat_list) {
            lSetList(message_elem, MES_job_number_list,
                     lCopyList("", jid_cat_list));
         } else {
            lSetList(message_elem, MES_job_number_list, jid_cat_list);
         }
      }
   }
}

static lRef schedd_mes_get_category(u_long32 job_id, lList *job_list)
{
   lListElem *job = NULL;  /* JB_Type */
   lRef ret = NULL;        /* Category pointer (void*) */

   DENTER(TOP_LAYER, "schedd_mes_get_category");
   job = lGetElemUlong(job_list, JB_job_number, job_id);
   if (job) {
      ret = lGetRef(job, JB_category);
   }
   DEXIT;
   return ret;
}

static lList *schedd_mes_get_same_category_jids(lRef category,
                                                lList *job_list,
                                                int ignore_category)
{
   lList *ret = NULL;      /* ULNG */
   lListElem *job = NULL;  /* JB_Type */

   DENTER(TOP_LAYER, "schedd_mes_get_same_category_jids");
   if (job_list != NULL && category != NULL) {
      ret = lCreateList("", ULNG_Type);
      for_each(job, job_list) {
         if (ignore_category || lGetRef(job, JB_category) == category) {
            lListElem *new_jid_elem = NULL;

            new_jid_elem = lCreateElem(ULNG_Type);
            lSetUlong(new_jid_elem, ULNG, lGetUlong(job, JB_job_number));
            lAppendElem(ret, new_jid_elem);
         }
      }
   }
   DEXIT;
   return ret;
}

/****** schedd/schedd_mes/schedd_mes_initialize() *****************************
*  NAME
*     schedd_mes_initialize() -- Initialize module variables 
*
*  SYNOPSIS
*     void schedd_mes_initialize(void) 
*
*  FUNCTION
*     Initialize module variables 
*******************************************************************************/
void schedd_mes_initialize(void)
{
   DENTER(TOP_LAYER, "schedd_initialize_messages");
   if (!sme) {
      lList *tmp_list;

      sme = lCreateElem(SME_Type);
      tmp_list = lCreateList("", MES_Type);
      lSetList(sme, SME_message_list, tmp_list);
      tmp_list = lCreateList("", MES_Type);
      lSetList(sme, SME_global_message_list, tmp_list);
   }

   /* prepare tmp_sme for collecting messages */
   if (!tmp_sme) {
      lList *tmp_list;

      tmp_sme = lCreateElem(SME_Type);
      tmp_list = lCreateList("", MES_Type);
      lSetList(tmp_sme, SME_message_list, tmp_list);
   }
   DEXIT;
}

/****** schedd/schedd_mes/schedd_mes_release() ********************************
*  NAME
*     schedd_mes_release() -- Free module variables 
*
*  SYNOPSIS
*     void schedd_mes_release(void) 
*
*  FUNCTION
*     Free module variables 
*******************************************************************************/
void schedd_mes_release(void)
{
   DENTER(TOP_LAYER, "schedd_release_messages");
   sme = lFreeElem(sme);
   tmp_sme = lFreeElem(tmp_sme);
   DEXIT;
}

/****** schedd/schedd_mes/schedd_mes_commit() *********************************
*  NAME
*     schedd_mes_commit() -- Complete message elements and move them
*
*  SYNOPSIS
*     void schedd_mes_commit(lList *job_list, int ignore_category) 
*
*  FUNCTION
*     Each message contained in "tmp_sme" containes only
*     one job id. We have to find other jobs in "job_list" and
*     add the job ids to the list of ids contained in "tmp_sme"
*     message elements. After that we have to move all messages 
*     contained in "tmp_sme" into "sme".
*
*     If "ignore_category" is 1 than the job category will be ignored.
*     This means thal all ids of "job_list" will be added to all 
*     messages contained in "tmp_sme". 
*     
*     If no category is passed in and ignore_category is false, the messages
*     are only generated for the current job, meaning, they are just copied.
*
*  INPUTS
*     lList *job_list     - JB_Type list 
*     int ignore_category - if set to true, the messages with be generated for all jobs
*                           in the list
*     lRef jid_category   - if not NULL, the function uses the category to ensure, that
*                           every message is only added per category once.
*******************************************************************************/
void schedd_mes_commit(lList *job_list, int ignore_category, lRef jid_category) {

   if (sme && tmp_sme) {
      lList *sme_mes_list = NULL;
      lList *tmp_sme_list = NULL;
      
      if (jid_category != NULL) {
         if (lGetBool(jid_category, CT_messages_added) ) {
            return;
         }
         lSetBool(jid_category, CT_messages_added, true);
      }

      /*
       * Try to find other jobs which apply also for created message
       */
      if (jid_category != NULL || ignore_category == 1) {
         schedd_mes_find_others(job_list, ignore_category);
      }

      /*
       * Tranfer all messages from tmp_sme to sme
       */
      sme_mes_list = lGetList(sme, SME_message_list);
      lXchgList(tmp_sme, SME_message_list, &tmp_sme_list);
      lAddList(sme_mes_list, tmp_sme_list);
      tmp_sme_list = lCreateList("job info messages", MES_Type);
      lSetList(tmp_sme, SME_message_list, tmp_sme_list);
   }
}

/****** schedd/schedd_mes/schedd_mes_rollback() *******************************
*  NAME
*     schedd_mes_rollback() -- Free temporaryly generated messages 
*
*  SYNOPSIS
*     void schedd_mes_rollback(void) 
*
*  FUNCTION
*     Free temporaryly generated messages contained in "tmp_sme". 
******************************************************************************/
void schedd_mes_rollback(void)
{
   if (tmp_sme) {
      lList *tmp_sme_list = NULL;

      tmp_sme_list = lCreateList("job info messages", MES_Type);
      lSetList(tmp_sme, SME_message_list, tmp_sme_list);
   }
}

/****** schedd/schedd_mes/schedd_mes_obtain_package() *************************
*  NAME
*     schedd_mes_obtain_package() -- Get message structure  
*
*  SYNOPSIS
*     lListElem *schedd_mes_obtain_packagevoid) 
*
*  FUNCTION
*     Returns message structure which containes all messages.
*
*  INPUTS
*     int *global_mes_count - out: returns nr of global messages
*     int *job_mes_count    - out: returns nr of job messages
*
*  NOTES
*     The calling function is responsible to free the returned
*     message structure if it is not needed anymore.
*
*  RESULT
*     lListElem* - SME_Type element
*******************************************************************************/
lListElem *schedd_mes_obtain_package(int *global_mes_count, int *job_mes_count)
{
   lListElem *ret;
   u_long32 schedd_job_info = sconf_get_schedd_job_info();

   DENTER(TOP_LAYER, "schedd_mes_obtain_package");

   if (schedd_job_info == SCHEDD_JOB_INFO_FALSE) {
      /*
       * Temporaryly we enable schedd_job_info to add one
       * message which says that schedd_job_info is disabled. 
       */
      sconf_enable_schedd_job_info();
      schedd_mes_add_global(SCHEDD_INFO_TURNEDOFF);
      sconf_disable_schedd_job_info();
   } else if (schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST) {
      schedd_mes_add_global(SCHEDD_INFO_JOBLIST);
   } else if (lGetNumberOfElem(lGetList(sme, SME_message_list))<1 &&
            lGetNumberOfElem(lGetList(sme, SME_global_message_list))<1) {
      schedd_mes_add_global(SCHEDD_INFO_NOMESSAGE);
   }

   if (global_mes_count != NULL) {
      *global_mes_count = lGetNumberOfElem(lGetList(sme, SME_global_message_list));
   }

   if (job_mes_count != NULL) {
      *job_mes_count = lGetNumberOfElem(lGetList(sme, SME_message_list));
   }

   ret = sme; /* calling function is responsible to free messages! */
   sme = NULL; 
   tmp_sme = lFreeElem(tmp_sme);

   DEXIT;
   return ret; 
}

/****** schedd/schedd_mes/schedd_mes_set_logging() ****************************
*  NAME
*     schedd_mes_set_logging() -- Turns schedd logging on and off 
*
*  SYNOPSIS
*     void schedd_mes_set_logging(int bval) 
*
*  FUNCTION
*     Turns schedd logging on and off. 
*
*  INPUTS
*     int bval - on (0) off (1) 
*******************************************************************************/
void schedd_mes_set_logging(int bval) {
   DENTER(TOP_LAYER, "schedd_mes_set_logging");
   log_schedd_info = bval;
   DEXIT;
}

int schedd_mes_get_logging(void) {
   return log_schedd_info;
}
/****** schedd/schedd_mes/schedd_mes_add() ************************************
*  NAME
*     schedd_mes_add() -- Add one entry into the message structure. 
*
*  SYNOPSIS
*     void schedd_mes_add(u_long32 job_number, 
*                         u_long32 message_number, 
*                         ...) 
*
*  FUNCTION
*     During the time the scheduler trys to dispatch jobs it might
*     call this function to add messages into a temporary structure.
*     This function might be called several times. Each call
*     will add one element which containes one message describing
*     a reason, why a job can't be dispatched and the concerned jid.
*
*     When it is clear if the job could be dispatched or not, one of
*     following functions has to be called:
*
*        schedd_mes_commit()
*        schedd_mes_rollback()
*
*  INPUTS
*     u_long32 job_number     - job id 
*     u_long32 message_number - message number (sge_schedd_text.h) 
*     ...                     - arguments for format string
*                               sge_schedd_text(message_number) 
*
*  SEE ALSO
*     schedd/schedd_mes/schedd_mes_commit()
*     schedd/schedd_mes/schedd_mes_rollback()
*******************************************************************************/
void schedd_mes_add(u_long32 job_number, u_long32 message_number, ...)
{
#ifndef WIN32NATIVE
   va_list args;
   const char *fmt;
   lListElem *mes = NULL;
   char msg[MAXMSGLEN];
   char msg_log[MAXMSGLEN];
   lList *jobs_ulng = NULL;
   lListElem *jid_ulng;
#if defined(LINUX)
   int nchars;
#endif

   DENTER(TOP_LAYER, "schedd_mes_add");

   fmt = sge_schedd_text(message_number);
   va_start(args,message_number);

#if defined(LINUX)
   nchars = vsnprintf(msg, MAXMSGLEN, fmt, args);
   if (nchars == -1) {
      ERROR((SGE_EVENT, MSG_SCHEDDMESSAGE_CREATEJOBINFOFORMESSAGEFAILED_U,
         u32c(message_number)));
      DEXIT;
      return;
   }
#else
   vsprintf(msg, fmt, args);
#endif

   if (job_number && (sconf_get_schedd_job_info() != SCHEDD_JOB_INFO_FALSE)) {
      if (sconf_get_schedd_job_info() == SCHEDD_JOB_INFO_JOB_LIST) {
         if (!range_list_is_id_within(sconf_get_schedd_job_info_range(),
                                      job_number)) {
            DPRINTF(("Job "u32" not in scheddconf.schedd_job_info_list\n", job_number));
            return;
         }
      }
      if (!tmp_sme)
         schedd_mes_initialize();

      mes = lCreateElem(MES_Type);
      jobs_ulng = lCreateList("job ids", ULNG_Type);
      lSetList(mes, MES_job_number_list, jobs_ulng);
      lSetUlong(mes, MES_message_number, message_number);
      lSetString(mes, MES_message, msg);
      lAppendElem(lGetList(tmp_sme, SME_message_list), mes);

      jid_ulng = lCreateElem(ULNG_Type);
      lSetUlong(jid_ulng, ULNG, job_number);
      lAppendElem(jobs_ulng, jid_ulng);

      if (log_schedd_info) {
         sprintf(msg_log, "Job "u32" %s", job_number, msg);
         SCHED_MON((log_string, msg_log));
      }
   } else {
      if (log_schedd_info) {
         if (job_number)
            sprintf(msg_log, "Job "u32" %s", job_number, msg);
         else
             sprintf(msg_log, "Your job %s", msg);
         SCHED_MON((log_string, msg_log));
      }
   }

   DEXIT;
#endif
}

/****** schedd/schedd_mes/schedd_mes_add_global() *************************
*  NAME
*     schedd_mes_add_global() -- add a global message 
*
*  SYNOPSIS
*     void schedd_mes_add_global(u_long32 message_number, ...) 
*
*  FUNCTION
*     Add a global message into a message structure. 
*
*  INPUTS
*     u_long32 message_number - message number (sge_schedd_text.h) 
*     ...                     - arguments for format string
*                               sge_schedd_text(message_number) 
*
*******************************************************************************/
void schedd_mes_add_global(u_long32 message_number, ...)
{
   va_list args;
   const char *fmt;
   lListElem *mes;
   char msg[MAXMSGLEN];
#if defined(LINUX)
   int nchars;
#endif

   DENTER(TOP_LAYER, "schedd_mes_add_global");

   if (sconf_get_schedd_job_info() != SCHEDD_JOB_INFO_FALSE) {
      /* Create error message */
      fmt = sge_schedd_text(message_number);
      va_start(args,message_number);
#if defined(LINUX)
      nchars = vsnprintf(msg, MAXMSGLEN, fmt, args);
      if (nchars == -1) {
         ERROR((SGE_EVENT, MSG_SCHEDDMESSAGE_CREATEJOBINFOFORMESSAGEFAILED_U,
            u32c(message_number)));
         DEXIT;
         return;
      }
#else
      vsprintf(msg, fmt, args);
#endif

      /* Add scheduling info to structure */
      if (!sme)
         schedd_mes_initialize();

      mes = lCreateElem(MES_Type);
      lSetUlong(mes, MES_message_number, message_number);
      lSetString(mes, MES_message, msg);
      lAppendElem(lGetList(sme, SME_global_message_list), mes);

      /* Write entry into log file */
      SCHED_MON((log_string, msg));
   }

   DEXIT;
}


/****** schedd_message/schedd_mes_get_tmp_list() *******************************
*  NAME
*     schedd_mes_get_tmp_list() -- gets all messages for the current job 
*
*  SYNOPSIS
*     lList* schedd_mes_get_tmp_list() 
*
*  FUNCTION
*     returns a list of all messages for the current job 
*
*  RESULT
*     lList* -  message list
*
*******************************************************************************/
lList *schedd_mes_get_tmp_list(){
   lList *ret = NULL;
   
   DENTER(TOP_LAYER, "schedd_mes_get_tmp_list");

   if (tmp_sme) {
     ret =  lGetList(tmp_sme, SME_message_list);  
   }
   DEXIT;
   return ret;
}

/****** schedd_message/schedd_mes_set_tmp_list() *******************************
*  NAME
*     schedd_mes_set_tmp_list() -- sets the messages for a current job 
*
*  SYNOPSIS
*     void schedd_mes_set_tmp_list(lListElem *category, int name) 
*
*  FUNCTION
*     Takes a mesage list, changes the job number to the current job and stores
*     the list. 
*
*  INPUTS
*     lListElem *category - an object, which stores the list 
*     int name            - element id for the list
*     u_long32 job_number - job number 
*
*******************************************************************************/
void schedd_mes_set_tmp_list(lListElem *category, int name, u_long32 job_number){
   lList *tmp_list = NULL;
   lListElem *tmp_elem = NULL;
   lList *job_id_list = NULL;
   lListElem *job_id = NULL;

   DENTER(TOP_LAYER, "schedd_mes_set_tmp_list");

   lXchgList(category, name, &tmp_list);

   for_each(tmp_elem, tmp_list){
      job_id_list =  lCreateList("job ids", ULNG_Type);
      lSetList(tmp_elem, MES_job_number_list, job_id_list);

      job_id =  lCreateElem(ULNG_Type);
      lSetUlong(job_id, ULNG, job_number);
      lAppendElem(job_id_list, job_id);
   }

   if (tmp_sme && tmp_list){
      lSetList(tmp_sme, SME_message_list, tmp_list); 
   }

   DEXIT;
}

