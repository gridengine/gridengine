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

#include "sgermon.h"
#include "sge_log.h"
#include "sge_gdi_intern.h"
#include "sge_usageL.h"
#include "sge_job.h"
#include "sge_ja_task.h"
#include "sge_pe_task.h"
#include "sge_report.h"

#include "msg_gdilib.h"

#define REPORT_LAYER TOP_LAYER

/****** gdi/report/report_list_send() ******************************************
*  NAME
*     report_list_send() -- Send a list of reports. 
*
*  SYNOPSIS
*     int report_list_send(const lList *rlp, const char *rhost, 
*                          const char *commproc, int id, 
*                          int synchron, u_long32 *mid) 
*
*  FUNCTION
*     Send a list of reports. 
*
*  INPUTS
*     const lList *rlp     - REP_Type list 
*     const char *rhost    - Hostname 
*     const char *commproc - Component name 
*     int id               - Component id 
*     int synchron         - true or false 
*     u_long32 *mid        - Message id 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Unexpected error
*        -2 - No memory
*        -3 - Format error
*        other - see sge_send_any_request()
*******************************************************************************/
int report_list_send(const lList *rlp, const char *rhost, 
                     const char *commproc, int id, 
                     int synchron, u_long32 *mid) 
{
   sge_pack_buffer pb;
   int ret, size;

   DENTER(REPORT_LAYER, "report_list_send");

   /* retrieve packbuffer size to avoid large realloc's while packing */
   init_packbuffer(&pb, 0, 1);
   ret = cull_pack_list(&pb, rlp);
   size = pb_used(&pb);
   clear_packbuffer(&pb);

   /* prepare packing buffer */
   if((ret = init_packbuffer(&pb, size, 0)) == PACK_SUCCESS) {
      ret = cull_pack_list(&pb, rlp);
   }

   switch (ret) {
   case PACK_SUCCESS:
      break;

   case PACK_ENOMEM:
      ERROR((SGE_EVENT, MSG_GDI_REPORTNOMEMORY_I , size));
      clear_packbuffer(&pb);
      DEXIT;
      return -2;

   case PACK_FORMAT:
      ERROR((SGE_EVENT, MSG_GDI_REPORTFORMATERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -3;

   default:
      ERROR((SGE_EVENT, MSG_GDI_REPORTUNKNOWERROR));
      clear_packbuffer(&pb);
      DEXIT;
      return -1;
   }

   ret = sge_send_any_request(synchron, mid, rhost, commproc, id, &pb, 
                              TAG_REPORT_REQUEST);
   clear_packbuffer(&pb);

   DEXIT;
   return ret;
}

/****** gdi/report/job_report_print_usage() ***********************************
*  NAME
*     job_report_print_usage() -- Print usage contained in job report 
*
*  SYNOPSIS
*     void job_report_print_usage(const lListElem *job_report, 
*                                 FILE *fp) 
*
*  FUNCTION
*     Print usage information conatines in "job_report". Print the 
*     information to the given file stream "fp" or as debug messages. 
*
*  INPUTS
*     const lListElem *job_report - JR_Type element 
*     FILE *fp                    - file stream or NULL 
*******************************************************************************/
void job_report_print_usage(const lListElem *job_report, FILE *fp) 
{
   lListElem *uep;

   DENTER(CULL_LAYER, "job_report_print_usage");

   if (!job_report) {
      DEXIT;
      return;
   }

   for_each(uep, lGetList(job_report, JR_usage)) {
      if (fp) {
         fprintf(fp, "   \"%s\" =   %.99g\n", lGetString(uep, UA_name),
                 lGetDouble(uep, UA_value));
      } else {
         DPRINTF(("   \"%s\" =   %.99g\n", lGetString(uep, UA_name),
                  lGetDouble(uep, UA_value)));
      }
   }

   DEXIT;
   return;
}

/****** gdi/report/job_report_init_from_job() **********************************
*  NAME
*     job_report_init_from_job() -- initialize job report 
*
*  SYNOPSIS
*     void job_report_init_from_job(lListElem *job_report, 
*                                  const lListElem *job, 
*                                  const lListElem *ja_task, 
*                                  const lListElem *pe_task) 
*
*  FUNCTION
*     Initialize "job_report" from the attributes obtained from
*     "job", "ja_task" and "pe_task". 
*
*  INPUTS
*     lListElem *job_report    - JR_Type object
*     const lListElem *job     - JB_Type object
*     const lListElem *ja_task - JAT_Type object 
*     const lListElem *pe_task - PET_Type object 
*******************************************************************************/
void job_report_init_from_job(lListElem *job_report, 
                              const lListElem *job, 
                              const lListElem *ja_task, 
                              const lListElem *pe_task) 
{
   u_long32 job_id = lGetUlong(job, JB_job_number);
   u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
   lListElem *queue = NULL;   /* QU_Type */

   DENTER(TOP_LAYER, "job_report_init_from_job");

   lSetUlong(job_report, JR_job_number, job_id);
   lSetUlong(job_report, JR_ja_task_number, ja_task_id);

   if(pe_task != NULL) {
      lSetString(job_report, JR_pe_task_id_str, lGetString(pe_task, PET_id));
   }

   lSetString(job_report, JR_owner, lGetString(job, JB_owner));

   if (lGetUlong(ja_task, JAT_status) == JSLAVE){
      if(pe_task == NULL) {
         lSetUlong(job_report, JR_state, JSLAVE);
      } else {
         lSetUlong(job_report, JR_state, JWRITTEN);
      }
   } else {
      lSetUlong(job_report, JR_state, JWRITTEN);
   }

   if(pe_task != NULL) {
      queue = lFirst(lGetList(pe_task, PET_granted_destin_identifier_list));
   } else {
      queue = lFirst(lGetList(ja_task, JAT_granted_destin_identifier_list));
   }

   if (queue != NULL) {
      lSetString(job_report, JR_queue_name, lGetString(queue, JG_qname));
      lSetHost(job_report, JR_host_name,  lGetHost(queue, JG_qhostname));
   }

   DEXIT;
}

