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
#include "sge.h"
#include "sge_load_reportL.h"
#include "sge_hostL.h"
#include "sge_jobL.h"
#include "sge_job_reportL.h"
#include "sge_reportL.h"
#include "sge_usageL.h"
#include "commlib.h"
#include "job_report_execd.h"
#include "sge_host.h"
#include "sge_arch.h"
#include "sge_nprocs.h"
#include "load_avg.h"
#include "report.h"
#include "sge_me.h"
#include "sge_prognames.h"
#include "sgermon.h"
#include "sge_log.h"
#include "utility.h"
#include "qm_name.h"
#include "sge_gdi_intern.h"

#ifndef NO_SGE_COMPILE_DEBUG
static char* report_types[] = {
   " REPORT_LOAD",
   " REPORT_EVENTS",
   " REPORT_CONF",
   " REPORT_LICENSE",
   " REPORT_JOB"
};
#endif

#define ALIVE_INTERVAL (3*60)

/*-------------------------------------------------------------------------*/
int sge_send_all_reports(
u_long32 now,
int which,
report_source *report_sources 
) {
   enum { STATE_OK, STATE_ERROR };

   static int state = STATE_ERROR;
   static u_long32 next_alive_time = 0;
   int ret = -1;
   int i;

   DENTER(TOP_LAYER, "sge_send_all_reports");

   /* in case of error state we test every load interval */
   if (now > next_alive_time || state == STATE_ERROR) {
      DPRINTF(("ALIVE TEST OF MASTER\n"));
      next_alive_time = now + ALIVE_INTERVAL;
     
      if (check_isalive(sge_get_master(state == STATE_ERROR)))
         state = STATE_ERROR;
      else 
         state = STATE_OK;
   }

   if (state == STATE_OK) {
      lList *report_list = NULL;

      DPRINTF(("SENDING LOAD AND REPORTS\n"));

      report_list = lCreateList("report list", REP_Type);

      for (i=0; report_sources[i].type; i++)
         if (!which || which == report_sources[i].type) {
            DPRINTF(("%s\n", report_types[report_sources[i].type - 1]));
            report_sources[i].func(report_list);
         }

      /*
       *  do not test if report->lp != NULL:
       *
       *  an empty load could get produced by an execd running not as root 
       *  so let him send this empty list for having an alive protocol
       */
      /* send load report asynchron to qmaster */
      ret = sge_send_reports(sge_get_master(0), prognames[QMASTER], 0, 
         report_list, 0, NULL);

      lFreeList(report_list);
   }
   DEXIT;
   return ret;
}

/* ----------------------------------------
 
   add a double value to the load report list lpp 
 
*/
int sge_add_double2load_report(
lList **lpp,
char *name,
double value,
int global,
char *units 
) {
   char load_string[255];
 
   DENTER(BASIS_LAYER, "sge_add_double2load_report");
 
   sprintf(load_string, "%f%s", value, units?units:"");
   sge_add_str2load_report(lpp, name, load_string, global);

   DEXIT;
   return 0; 
}

/* ----------------------------------------

   add an integer value to the load report list lpp

*/
int sge_add_int2load_report(
lList **lpp,
const char *name,
int value,
int global 
) {
   char load_string[255];
   int ret;

   DENTER(BASIS_LAYER, "sge_add_int2load_report");

   sprintf(load_string, "%d", value);
   ret = sge_add_str2load_report(lpp, name, load_string, global);

   DEXIT;
   return ret;
}

/* ----------------------------------------

   add a string value to the load report list lpp

*/
int sge_add_str2load_report(
lList **lpp,
const char *name,
const char *value,
int global 
) {
   lListElem *ep;

   DENTER(BASIS_LAYER, "sge_add_str2load_report");

   if ( !lpp || !name || !value ) {
      DEXIT;
      return -1;
   }

   ep=lGetElemStr(*lpp, LR_name, name);
   if (!ep) {
      ep = lAddElemStr(lpp, LR_name, name, LR_Type);
   }
   lSetString(ep, LR_value, value);
   lSetUlong(ep, LR_global, (u_long32)(global?1:0));

   if (global)
      lSetString(ep, LR_host, SGE_GLOBAL_NAME);
   else {
      lSetString(ep, LR_host, me.qualified_hostname);
   }

   DEXIT;
   return 0;
}

