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
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "sge_c_report.h"

#include <string.h>
#include "sgermon.h"
#include "sge_prog.h"
#include "sge_log.h"
#include "msg_qmaster.h"
#include "sge_host.h"
#include "sge_reportL.h"
#include "sge_c_gdi.h"
#include "sge_host_qmaster.h"
#include "configuration_qmaster.h"
#include "sge_conf.h"
#include "qmaster_to_execd.h"
#include "job_report_qmaster.h"
#include "sge_any_request.h"
#include "sge_persistence_qmaster.h"
#include "sge_answer.h"


static int update_license_data(lListElem *hep, lList *lp_lic); 


/****** sge_c_report() *******************************************************
*  NAME
*     sge_c_report() -- process execd load report
*
*  SYNOPSIS
*     void sge_c_report(char *rhost, char *commproc, int id, lList *report_list)
*
*  FUNCTION
*
*  INPUTS
*     char *rhost
*     char *commproc
*     int id
*     lList *report_list
*
*  RESULT
*     void - nothing
*
******************************************************************************/
void sge_c_report(char *rhost, char *commproc, int id, lList *report_list)
{
   lListElem *hep = NULL;
   u_long32 rep_type;
   lListElem *report;
   int ret = 0;
   u_long32 this_seqno, last_seqno;
   u_long32 rversion;

   DENTER(TOP_LAYER, "sge_c_report");

   if (!lGetNumberOfElem(report_list)) {
      DPRINTF(("received empty report\n"));
      DEXIT;
      return;
   }

   /* accept reports only from execd's */
   if (strcmp(prognames[EXECD], commproc)) {
      ERROR((SGE_EVENT, MSG_GOTSTATUSREPORTOFUNKNOWNCOMMPROC_S, commproc));
      DEXIT;
      return;
   }
  
   /* need exec host for all types of reports */
   if (!(hep = host_list_locate(Master_Exechost_List, rhost))) {
      ERROR((SGE_EVENT, MSG_GOTSTATUSREPORTOFUNKNOWNEXECHOST_S, rhost));
      DEXIT;
      return;
   }

   /* do not process load reports from old execution daemons */
   rversion = lGetUlong(lFirst(report_list), REP_version);
   if (verify_request_version(NULL, rversion, rhost, commproc, id)) {
      DEXIT;
      return;
   }

   /* prevent old reports being proceeded 
      frequent loggings of outdated reports can be an indication 
      of too high message traffic arriving at qmaster */ 
   this_seqno = lGetUlong(lFirst(report_list), REP_seqno);
   last_seqno = lGetUlong(hep, EH_report_seqno);

   if ((this_seqno < last_seqno && (last_seqno - this_seqno) <= 9000) &&
      !(last_seqno > 9990 && this_seqno < 10)) {
      /* this must be an old report, log and then ignore it */
      DPRINTF(("received old load report ("U32CFormat"< "U32CFormat") from exec host %s\n", 
         u32c(this_seqno), u32c(last_seqno+1), rhost));
      DEXIT;
      return;
   }
   lSetUlong(hep, EH_report_seqno, this_seqno);

   /*
   ** process the reports one after the other
   ** usually there will be a load report
   ** and a configuration version report
   */

   for_each(report, report_list) {

      rep_type = lGetUlong(report, REP_type);

      switch (rep_type) {
      case NUM_REP_REPORT_LOAD:
      
         /* Now handle execds load reports */
         sge_update_load_values(rhost, lGetList(report, REP_list));

         break;
      case NUM_REP_REPORT_CONF:

         if (hep && !is_configuration_up_to_date(hep, Master_Config_List, 
                                          lGetList(report, REP_list))) {
            DPRINTF(("configuration on host %s is not up to date\n", rhost));

            ret = host_notify_about_new_conf(hep);
            if (ret) {
               ERROR((SGE_EVENT, MSG_CONF_CANTNOTIFYEXECHOSTXOFNEWCONF_S, rhost));
               break;
            }
         }
         break;
         
      case NUM_REP_REPORT_PROCESSORS:
         /*
         ** save number of processors
         */
         ret = update_license_data(hep, lGetList(report, REP_list));
         if (ret) {
            ERROR((SGE_EVENT, MSG_LICENCE_ERRORXUPDATINGLICENSEDATA_I, ret));
            break;
         }

         break;

      case NUM_REP_REPORT_JOB:

         {
            sge_pack_buffer pb;

            if(init_packbuffer(&pb, 1024, 0) == PACK_SUCCESS) {
               process_job_report(report, hep, rhost, commproc, &pb);

               if (pb_filled(&pb)) {
                  /* send all stuff packed during processing to execd */
                  sge_send_any_request(0, NULL, rhost, commproc, id, &pb, 
                                       TAG_ACK_REQUEST); 
               }
               clear_packbuffer(&pb);
            }
         }
         break;

      default:   
         DPRINTF(("received invalid report type %ld\n", rep_type));
         DEXIT;
         return;
      }
   } /* end for_each */

   
   DEXIT;
   return;
} /* sge_c_report */

/*
** NAME
**   update_license_data
** PARAMETER
**   hep                 - pointer to host element, EH_Type
**   lp_lic              - list of license data, LIC_Type
**
** RETURN
**    0                  - ok
**   -1                  - NULL pointer received for hep
**   -2                  - NULL pointer received for lp_lic
** EXTERNAL
**   none
** DESCRIPTION
**   updates the number of processors in the host element
**   spools if it has changed
*/
static int update_license_data(lListElem *hep, lList *lp_lic)
{
   u_long32 processors, old_processors;

   DENTER(TOP_LAYER, "update_license_data");

   if (!hep) {
      DEXIT;
      return -1;
   }

   /*
   ** if it was clear what to do in this case we could return 0
   */
   if (!lp_lic) {
      DEXIT;
      return -2;
   }

   /*
   ** at the moment only the first element is evaluated
   */
   processors = lGetUlong(lFirst(lp_lic), LIC_processors);
   /* arch = lGetString(lFirst(lp_lic), LIC_arch); */ 

   old_processors = lGetUlong(hep, EH_processors);
   lSetUlong(hep, EH_processors, processors);
   /*
   ** we spool, cf. cod_update_load_values()
   */
   if (processors != old_processors) {
      lList *answer_list = NULL;

      DPRINTF(("%s has " u32 " processors\n",
         lGetHost(hep, EH_name), processors));
      sge_event_spool(&answer_list, 0, sgeE_EXECHOST_MOD, 
                      0, 0, lGetHost(hep, EH_name), NULL, NULL,
                      hep, NULL, NULL, true, true);
      answer_list_output(&answer_list);
   }

   DEXIT;
   return 0;
}

