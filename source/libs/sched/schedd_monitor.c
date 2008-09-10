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
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include "uti/sge_stdio.h"
#include "sge_all_listsL.h"
#include "schedd_monitor.h"
#include "sgermon.h"
#include "cull_parse_util.h"
#include "sge_time.h"
#include "sge_answer.h"
#include "uti/sge_string.h"

#include "msg_common.h"

static bool monitor_next_run = false; 

static char log_string[2048 + 1] = "invalid log_string";
static char schedd_log_file[SGE_PATH_MAX + 1] = "";

/* if set we do not log into schedd log file but we fill up this answer list */
static lList **monitor_alpp = NULL;

void schedd_set_schedd_log_file(sge_gdi_ctx_class_t *ctx)
{
   const char *cell_root = ctx->get_cell_root(ctx);
   
   DENTER(TOP_LAYER, "schedd_set_schedd_log_file");

   if (!*schedd_log_file) {
      snprintf(schedd_log_file, sizeof(schedd_log_file), "%s/%s/%s", cell_root, "common", SCHED_LOG_NAME);
      DPRINTF(("schedd log file >>%s<<\n", schedd_log_file));
   }

   DEXIT;
}

/****** schedd_monitor/schedd_is_monitor_next_run() ****************************
*  NAME
*     schedd_is_monitor_next_run() -- ??? 
*
*  SYNOPSIS
*     bool schedd_is_monitor_next_run(void) 
*
*  FUNCTION
*     Returns the state of scheduling log. 
*
*  RESULT
*     bool - true if logging is activated; false if logging is disabled 
*
*  NOTES
*     MT-NOTE: schedd_is_monitor_next_run() is not MT safe 
*******************************************************************************/
bool schedd_is_monitor_next_run(void)
{
   return monitor_next_run;
}

/****** schedd_monitor/schedd_set_monitor_next_run() ***************************
*  NAME
*     schedd_set_monitor_next_run() -- ??? 
*
*  SYNOPSIS
*     void schedd_set_monitor_next_run(bool set) 
*
*  FUNCTION
*     Activated and deactivates logging of scheduling information into 
*     the scheduling runlog file.
*
*  INPUTS
*     bool set - State of scheduling log for the next run. 
*
*  NOTES
*     MT-NOTE: schedd_set_monitor_next_run() is not MT safe 
*******************************************************************************/
void schedd_set_monitor_next_run(bool set)
{
   monitor_next_run = set; 
}

char* schedd_get_log_string(void)
{
   return log_string;
}

void set_monitor_alpp(lList **alpp) {
   monitor_alpp = alpp;
}

int schedd_log(const char *logstr) 
{
   DENTER(TOP_LAYER, "schedd_log");

   if (monitor_alpp) {
      /* add to answer list for verification (-w v) */
      answer_list_add(monitor_alpp, logstr, STATUS_ESEMANTIC, ANSWER_QUALITY_ERROR);
   } else if (schedd_is_monitor_next_run()){
      /* do logging (-tsm) */
      time_t now;
      FILE *fp = NULL;
      char *time_str = NULL;
      char str[128];
   
      now = (time_t)sge_get_gmt();
      time_str =  ctime_r(&now, str);
      if (time_str[strlen(time_str) - 1] == '\n') {
         time_str[strlen(time_str) - 1] = '|';
      }

      fp = fopen(schedd_log_file, "a");
      if (!fp) {
         DPRINTF(("could not open schedd_log_file "SFQ"\n", schedd_log_file));
         DEXIT;
         return -1;
      }

      fprintf(fp, "%s", time_str);
      fprintf(fp, "%s\n", logstr);
      FCLOSE(fp);
   }

   DEXIT;
   return 0;
FCLOSE_ERROR:
   DPRINTF((MSG_FILE_ERRORCLOSEINGXY_SS, schedd_log_file, strerror(errno)));
   DEXIT;
   return -1;
}


#define NUM_ITEMS_ON_LINE 10

int schedd_log_list(const char *logstr, lList *lp, int nm) {
   int fields[] = { 0, 0 };
   const char *delis[] = {NULL, " ", NULL};
   lList *lp_part = NULL;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "schedd_log_list");

#ifndef WIN32NATIVE

   if (!schedd_is_monitor_next_run()) {
      DEXIT;
      return 0;
   }
   
   fields[0] = nm;

   for_each(ep, lp) {
      if (!lp_part) {
         lp_part = lCreateList("partial list", lGetListDescr(lp));
      }
      lAppendElem(lp_part, lCopyElem(ep));
      if ((lGetNumberOfElem(lp_part) == NUM_ITEMS_ON_LINE) || !lNext(ep)) {
         strcpy(log_string, logstr);
#ifndef WIN32NATIVE
         uni_print_list(NULL, 
                        log_string + strlen(log_string), 
                        sizeof(log_string) - strlen(log_string) - 1, 
                        lp_part, 
                        fields, delis, 0);
#endif
         schedd_log(log_string);
         lFreeList(&lp_part);
         lp_part = NULL;
      }
   }
#else
   DPRINTF(("schedd_log_list does nothing for QMonNT !!!\n"));
#endif

   DEXIT;
   return 0;
}

const char *job_descr(
u_long32 jobid 
) {
   static char descr[20];

   if (jobid) {
      sprintf(descr, "Job "sge_u32, jobid);
      return descr;
   } else
      return "Job";
   
}

