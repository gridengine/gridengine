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
#include "sge_ulongL.h"
#include "sge_schedd_text.h"
#include "msg_schedd.h"
#include "sge_range.h"
#include "sge_job.h"

/* 
** Message structure where job scheduling informations are stored i
*/
static lListElem *sme = NULL;

/* 
** write scheduling informaten into logfilei
*/
static int log_schedd_info = 1;

/*
** list we use to locate jobs of the same category
*/
static lList *sme_job_list = NULL;

/*--------------------------------------------------------------
 * initialise structure for scheduling information
 *------------------------------------------------------------*/
void schedd_initialize_messages() 
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
   sme_job_list = NULL;
   DEXIT;
}

/*--------------------------------------------------------------
 * initialize the job-list pointer we use to locate jobs
 * of the same category
 *------------------------------------------------------------*/
void schedd_initialize_messages_joblist(
lList *job_list 
) {
   DENTER(TOP_LAYER, "schedd_initialize_messages_joblist");
   sme_job_list = job_list;
   DEXIT;
}

/*--------------------------------------------------------------
 * delete structure for scheduling information
 *------------------------------------------------------------*/
void schedd_release_messages() 
{
   DENTER(TOP_LAYER, "schedd_release_messages");
   if (sme) {
      sme = lFreeElem(sme);
   }
   DEXIT;
}

/*--------------------------------------------------------------
 * deliver the pointer to the message structure 
 *------------------------------------------------------------*/
lListElem *schedd_get_messages()
{
#ifndef WIN32NATIVE
   DENTER(TOP_LAYER, "schedd_get_messages");

   if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_FALSE)
      schedd_add_global_message(SCHEDD_INFO_TURNEDOFF);
   else if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST)
      schedd_add_global_message(SCHEDD_INFO_JOBLIST);
   else if (lGetNumberOfElem(lGetList(sme, SME_message_list))<1 &&
            lGetNumberOfElem(lGetList(sme, SME_global_message_list))<1) {
      schedd_add_global_message(SCHEDD_INFO_NOMESSAGE);
   }

#if 0
   if (!sme || (lGetNumberOfElem(lGetList(sme, SME_message_list))<1
         && lGetNumberOfElem(lGetList(sme, SME_global_message_list))<1)) {
      if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_FALSE) 
         schedd_add_global_message(SCHEDD_INFO_TURNEDOFF);
      else if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST)
         schedd_add_global_message(SCHEDD_INFO_JOBLIST);
   } else
      if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST)
         schedd_add_global_message(SCHEDD_INFO_JOBLIST);
#endif
      
   DEXIT;
   return sme;
#else
   return 0;
#endif
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
   lListElem *job = NULL;
   lList *jobs_ulng = NULL;
   lListElem *jid_ulng;
#if defined(LINUX)
   int nchars;
#endif
   
   DENTER(CULL_LAYER, "schedd_add_message");

/*    DPRINTF(("added message for job "u32"\n", job_number)); */

   /*
   ** Locate job if possible
   */
   if (job_number && sme_job_list) {
      for_each(job, sme_job_list) {
         if (lGetUlong(job, JB_job_number) == job_number)
            break;
      }
   }

   /* 
   ** Create error message 
   */
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

   /* 
   ** Add scheduling information to structure 
   */
   if (job_number && (scheddconf.schedd_job_info != SCHEDD_JOB_INFO_FALSE)) {
      if (scheddconf.schedd_job_info == SCHEDD_JOB_INFO_JOB_LIST) {
         if (!range_list_is_id_within(scheddconf.schedd_job_info_list,
                                      job_number)) {
            DPRINTF(("Job "u32" not in scheddconf.schedd_job_info_list\n", job_number));
            return; 
         }
      }
      if (!sme) 
         schedd_initialize_messages();    

      mes = lCreateElem(MES_Type);
      jobs_ulng = lCreateList("job ids", ULNG_Type);
      lSetList(mes, MES_job_number_list, jobs_ulng);
      lSetUlong(mes, MES_message_number, message_number);
      lSetString(mes, MES_message, msg);
      lAppendElem(lGetList(sme, SME_message_list), mes);

      /*
      ** if we found a job we will add the jobids of all jobs
      ** which have the same category
      ** else we will add the current job number
      */
      if (job) {
         lListElem *cat;
   
         cat = lGetRef(job, JB_category);
         for_each(job, sme_job_list) {
            if (cat == lGetRef(job, JB_category)) {
               jid_ulng = lCreateElem(ULNG_Type);
               lSetUlong(jid_ulng, ULNG, lGetUlong(job, JB_job_number));
               lAppendElem(jobs_ulng, jid_ulng);
               /* Write entry into log file */
               if (log_schedd_info) {
                  sprintf(msg_log, "Job "u32" %s", lGetUlong(job, JB_job_number), msg);
                  SCHED_MON((log_string, msg_log));
               }
            }
         }
      } else {
         jid_ulng = lCreateElem(ULNG_Type);
         lSetUlong(jid_ulng, ULNG, job_number);
         lAppendElem(jobs_ulng, jid_ulng);
         if (log_schedd_info) {
            sprintf(msg_log, "Job "u32" %s", job_number, msg);
            SCHED_MON((log_string, msg_log));
         }
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
      schedd_initialize_messages();
   mes = lCreateElem(MES_Type);
   lSetUlong(mes, MES_message_number, message_number);
   lSetString(mes, MES_message, msg);
   lAppendElem(lGetList(sme, SME_global_message_list), mes);

   /* Write entry into log file */
   SCHED_MON((log_string, msg));

   DEXIT;
}



