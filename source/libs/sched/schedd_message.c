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
#include "schedd_conf.h"
#include "schedd_monitor.h"
#include "sge_log.h"
#include "parse_range.h"
#include "sge_jobL.h"
#include "sge_ulongL.h"
#include "sge_schedd_text.h"
#include "msg_schedd.h"

static void schedd_message_find_others(lList *job_list,
                                       int ignore_category);

static lList *schedd_mes_get_same_category_jids(lRef category,
                                                lList *job_list,
                                                int ignore_category);

static lRef schedd_mes_get_category(u_long32 job_id, lList *job_list);

/* 
** Message structure where job scheduling informations are stored i
*/
static lListElem *sme = NULL;
static lListElem *tmp_sme = NULL;

/* 
** write scheduling informaten into logfile
*/
static int log_schedd_info = 1;

static void schedd_message_find_others(lList *job_list, int ignore_category)
{
   DENTER(TOP_LAYER, "schedd_message_find_others");

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
       * If 'ignore_category' is 1 then we will add all jids
       */
      for_each(message_elem, message_list) {
         lList *jid_list = lGetList(message_elem, MES_job_number_list);
         u_long32 jid = lGetUlong(lFirst(jid_list), ULNG);
         lRef jid_category = NULL; 

         if (!ignore_category) {
            jid_category = schedd_mes_get_category(jid, job_list);
         }

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

         /*
          * Replace the MES_job_number_list which containes only one jid
          * with a list of jids (all have same category
          */
         if (create_new_jid_cat_list) {
            lSetList(message_elem, MES_job_number_list, 
                     lCopyList("", jid_cat_list));
         } else {
            lSetList(message_elem, MES_job_number_list, jid_cat_list); 
            create_new_jid_cat_list = 1;
         }
      }
   } 

   DEXIT;
}

static lRef schedd_mes_get_category(u_long32 job_id, lList *job_list)
{
   lListElem *job = NULL;  /* JB_Type */
   lRef ret = NULL;        /* Category pointer (void*) */

   job = lGetElemUlong(job_list, JB_job_number, job_id);
   if (job) {
      ret = lGetRef(job, JB_category);
   }
   return ret;
}

static lList *schedd_mes_get_same_category_jids(lRef category,
                                                lList *job_list,
                                                int ignore_category)
{  
   lList *ret = NULL;      /* ULNG */
   lListElem *job = NULL;  /* JB_Type */

   if (job_list != NULL && (category != NULL || ignore_category)) {
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
   return ret;
}

/*--------------------------------------------------------------
 * initialise structure for scheduling information
 *------------------------------------------------------------*/
void schedd_mes_initialize() 
{
   lList *mes_list;
   lList *gmess_list;

   DENTER(TOP_LAYER, "schedd_initialize_messages");
   
   if (!sme) {
      /* Initialize the scheduler message information element */
      sme = lCreateElem(SME_Type);
      mes_list = lCreateList("job info messages", MES_Type);
      gmess_list = lCreateList("global messages", MES_Type);
      lSetList(sme, SME_message_list, mes_list);
      lSetList(sme, SME_global_message_list, gmess_list);
   }
   if (!tmp_sme) {
      /* Initialize tmp objects */
      tmp_sme = lCreateElem(SME_Type);
      mes_list = lCreateList("job info messages", MES_Type);
      lSetList(tmp_sme, SME_message_list, mes_list);
   }
   DEXIT;
}

void schedd_mes_commit(lList *job_list, int ignore_category)
{
   if (sme && tmp_sme) {
      lList *sme_mes_list = NULL;
      lList *tmp_sme_list = NULL;

      /*
       * Try to find other jobs which apply also for created message
       */
      schedd_message_find_others(job_list, ignore_category);

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

void schedd_mes_rollback(void)
{
   if (tmp_sme) {
      lList *tmp_sme_list = NULL;

      tmp_sme_list = lCreateList("job info messages", MES_Type); 
      lSetList(tmp_sme, SME_message_list, tmp_sme_list);
   }
}

/*--------------------------------------------------------------
 * delete structure for scheduling information
 *------------------------------------------------------------*/
void schedd_mes_release() 
{
   DENTER(TOP_LAYER, "schedd_release_messages");
   sme = lFreeElem(sme);
   tmp_sme = lFreeElem(tmp_sme);
   DEXIT;
}

/*--------------------------------------------------------------
 * deliver the pointer to the message structure 
 *------------------------------------------------------------*/
lListElem *schedd_mes_obtain_package(void)
{
   lListElem *ret;
   DENTER(TOP_LAYER, "schedd_mes_obtain_package");

#ifndef WIN32NATIVE
   if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_FALSE) {
      enum schedd_job_info_key old_val = scheddconf.schedd_job_info;

      /*
       * Temporaryly we enable schedd_job_info to add one
       * message which says that schedd_job_info is disabled. 
       */
      scheddconf.schedd_job_info = SCHEDD_JOB_INFO_TRUE;
      schedd_add_global_message(SCHEDD_INFO_TURNEDOFF);
      scheddconf.schedd_job_info = old_val;
   } else if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST) {
      schedd_add_global_message(SCHEDD_INFO_JOBLIST);
   } else if (lGetNumberOfElem(lGetList(sme, SME_message_list))<1 &&
            lGetNumberOfElem(lGetList(sme, SME_global_message_list))<1) {
      schedd_add_global_message(SCHEDD_INFO_NOMESSAGE);
   }

#if 0 /* EB: debug */
   {
      lListElem *elem;

      DPRINTF(("SME: %d messages (%ld bytes)\n",
         lGetNumberOfElem(lGetList(sme, SME_message_list)),
         (long)cull_get_list_packsize(lGetList(sme, SME_message_list))));
      for_each(elem, lGetList(sme, SME_message_list)) {
         lList *sub_list = lGetList(elem, MES_job_number_list);
         DPRINTF(("SME: %d ids\n", lGetNumberOfElem(sub_list)));
      }
      DPRINTF(("SME: %d global messages (%ld bytes)\n",
         lGetNumberOfElem(lGetList(sme, SME_global_message_list)),
         (long)cull_get_list_packsize(lGetList(sme, SME_global_message_list))));
   }
#endif      

   ret = sme;
   sme = NULL;
   tmp_sme = lFreeElem(tmp_sme);
#else
   ret = NULL;
#endif
   DEXIT;
   return ret; 
}

/*--------------------------------------------------------------
 * Turns log of scheduling information on and off
 *------------------------------------------------------------*/
void schedd_log_schedd_info(int bval) {
   DENTER(TOP_LAYER, "schedd_log_schedd_info");
   log_schedd_info = bval;
   DEXIT;
}

/*--------------------------------------------------------------
 * add one entry into the message structure for all jobs which have
 * the same category like job_number
 *    job_number        add an entry for this job id
 *    message_number    message number defined in sge_schedd_text.h
 *    ...               arguments for format string 
 *                      sge_schedd_text(message_number)
 *--------------------------------------------------------------*/
void schedd_add_message(u_long32 job_number, u_long32 message_number, ...)
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
   
   DENTER(TOP_LAYER, "schedd_add_message");

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

   if (job_number && (scheddconf.schedd_job_info != SCHEDD_JOB_INFO_FALSE)) {
      if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST) {
         if (!id_in_range (job_number, scheddconf.schedd_job_info_list)) {
            DPRINTF(("Job "u32" not in scheddconf.schedd_job_info_list\n", job_number));
            return; 
         }
      }
      if (tmp_sme) 
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

/*--------------------------------------------------------------
 * add a global entry into the message structure
 *    job_number        add an entry for this job id
 *    message_number    message number defined in sge_schedd_text.h
 *    ...               arguments for format string
 *                      sge_schedd_text(message_number)
 *--------------------------------------------------------------*/
void schedd_add_global_message(u_long32 message_number, ...)
{
   va_list args;
   const char *fmt;
   lListElem *mes;
   char msg[MAXMSGLEN];
#if defined(LINUX)
   int nchars;
#endif

   DENTER(TOP_LAYER, "schedd_add_message");

   if (scheddconf.schedd_job_info != SCHEDD_JOB_INFO_FALSE) {
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



