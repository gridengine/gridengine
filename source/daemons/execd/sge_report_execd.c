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
#include <string.h>
#include <strings.h>

#include "sge.h"
#include "sge_any_request.h"
#include "sge_usageL.h"
#include "commlib.h"
#include "job_report_execd.h"
#include "sge_host.h"
#include "load_avg.h"
#include "sge_prog.h"
#include "sgermon.h"
#include "sge_log.h"
#include "qm_name.h"
#include "sge_report_execd.h"
#include "sge_report.h"
#include "sge_load.h"

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
      state = STATE_OK;
      if (check_isalive(sge_get_master(state == STATE_ERROR)) != CL_RETVAL_OK) {
         state = STATE_ERROR;
      }
   }

   if (state == STATE_OK) {
      lList *report_list = NULL;

      DPRINTF(("SENDING LOAD AND REPORTS\n"));

      report_list = lCreateList("report list", REP_Type);

      for (i=0; report_sources[i].type; i++) {
         if (!which || which == report_sources[i].type) {
            DPRINTF(("%s\n", report_types[report_sources[i].type - 1]));
            report_sources[i].func(report_list);
         }
      }

      /*
       *  do not test if report->lp != NULL:
       *
       *  an empty load could get produced by an execd running not as root 
       *  so let him send this empty list for having an alive protocol
       */
      /* send load report asynchron to qmaster */
      ret = report_list_send(report_list, sge_get_master(0), 
                             prognames[QMASTER], 1, 0, NULL);
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
const char *host,
char *units 
) {
   char load_string[255];
 
   DENTER(BASIS_LAYER, "sge_add_double2load_report");
 
   sprintf(load_string, "%f%s", value, units?units:"");
   sge_add_str2load_report(lpp, name, load_string, host);

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
const char *host 
) {
   char load_string[255];
   int ret;

   DENTER(BASIS_LAYER, "sge_add_int2load_report");

   sprintf(load_string, "%d", value);
   ret = sge_add_str2load_report(lpp, name, load_string, host);

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
const char *host
) {
   lListElem *ep = NULL, *search_ep = NULL;
   const void *iterator = NULL;

   DENTER(BASIS_LAYER, "sge_add_str2load_report");

   if ( !lpp || !name || !value ) {
      DEXIT;
      return -1;
   }

   if(*lpp != NULL) {
      search_ep = lGetElemHostFirst(*lpp, LR_host, host, &iterator);
      while(search_ep != NULL) {
         DPRINTF(("---> %s\n", lGetString(search_ep, LR_name)));
         if(strcmp(lGetString(search_ep, LR_name), name) == 0) {
            ep = search_ep;
            break;
         }
         search_ep = lGetElemHostNext(*lpp, LR_host, host, &iterator);
      }
   }
   
   if (ep == NULL) {
      DPRINTF(("adding new load variable %s for host %s\n", name, host));
      ep = lAddElemStr(lpp, LR_name, name, LR_Type);
      lSetHost(ep, LR_host, host);
      lSetUlong(ep, LR_global, (u_long32)(strcmp(host, SGE_GLOBAL_NAME) == 0 ? 1 : 0));
      lSetUlong(ep, LR_static, sge_is_static_load_value(name));
   }

   lSetString(ep, LR_value, value);

   DPRINTF(("load value %s for host %s: %s\n", name, host, value)); 

   DEXIT;
   return 0;
}

