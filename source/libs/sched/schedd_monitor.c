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

#include "sge_all_listsL.h"
/* #include "sge_schedd.h" */
#include "schedd_monitor.h"
#include "sgermon.h"
#include "cull_parse_util.h"
#include "sge_time.h"
#include "setup_path.h"
#include "sge_gdi_intern.h"

static int monitor_next_run = 0;
char log_string[2048 + 1] = "invalid log_string";


/* if set we do not log into schedd log file but we fill up this answer list */
static lList **monitor_alpp = NULL;

void monitor_set_next_run(int i) 
{
   DENTER(TOP_LAYER, "monitor_set_next_run");
   DPRINTF(("monitor_next_run = %d\n", i));
   monitor_next_run = i;
   DEXIT;
}

int monitor_get_next_run(void)
{
   return monitor_next_run;
}

void clean_monitor_alp()
{
   DENTER(TOP_LAYER, "schedd_log");
   if (monitor_alpp) {
      *monitor_alpp = lFreeList(*monitor_alpp);
   }
   DEXIT; 
}

void set_monitor_alpp(lList **alpp) 
{
   DENTER(TOP_LAYER, "schedd_log");
   monitor_alpp = alpp;
   monitor_next_run = (alpp!=NULL);
   DEXIT;
}

int schedd_log(const char *logstr) {
   time_t now;
   FILE *fp;
   static char schedd_log_file[SGE_PATH_MAX + 1] = "";
   char time_str[256 + 1];

   DENTER(TOP_LAYER, "schedd_log");

   if (!monitor_next_run) {
      DPRINTF(("monitor_next_run is 0\n"));
      DEXIT;
      return 0;
   }

   if (monitor_alpp) {
      char logloglog[2048];
/*       DPRINTF(("schedd_log: %s\n", logstr)); */
      sprintf(logloglog, "%s\n", logstr);
      sge_add_answer(monitor_alpp, logloglog, STATUS_ESEMANTIC, 0);
   } else {
      if (!*schedd_log_file) {
         sprintf(schedd_log_file, "%s/%s/%s", path.cell_root, "common", SCHED_LOG_NAME);
         DPRINTF(("schedd log file >>%s<<\n", schedd_log_file));
      }

      now = sge_get_gmt();
      /* strftime(time_str, sizeof(time_str), "%m-%d-%Y:%H:%M:%S ", localtime(&now)); */
      strcpy(time_str, ctime((time_t *)&now));
      if (time_str[strlen(time_str) - 1] == '\n') {
         time_str[strlen(time_str) - 1] = '|';
      }

      fp = fopen(schedd_log_file, "a");
      if (!fp) {
         DPRINTF(("could not open schedd_log_file %s\n", schedd_log_file));
         DEXIT;
         return -1;
      }

      DPRINTF(("Logging: %s%s\n", time_str, logstr));

      fprintf(fp, "%s", time_str);
      fprintf(fp, "%s\n", logstr);
      fclose(fp);
   }

   DEXIT;
   return 0;
}


#define NUM_ITEMS_ON_LINE 10

int schedd_log_list(const char *logstr, lList *lp, int nm) {
   intprt_type fields[] = { 0, 0 };
   const char *delis[] = {NULL, " ", NULL};
   lList *lp_part = NULL;
   lListElem *ep = NULL;

   DENTER(TOP_LAYER, "schedd_log_list");

#ifndef WIN32NATIVE

   if (!monitor_next_run) {
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
         lFreeList(lp_part);
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
      sprintf(descr, "Job "u32, jobid);
      return descr;
   } else
      return "Job";
   
}

