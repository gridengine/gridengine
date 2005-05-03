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
#include "sgermon.h"
#include "basis_types.h"
#include "sge_any_request.h"
#include "sge_ja_task.h"
#include "sge_answer.h"
#include "sge_log.h"
#include "sge_suserL.h"
#include "sge_job.h"
#include "sge_suser.h"
#include "sge_pe.h"
#include "sge_ckpt.h"
#include "sge_todo.h"
#include "sge_string.h"
#include "sge_userset.h"
#include "config_file.h"
#include "sge_event_master.h"
#include "sge_utility.h"
#include "sge_signal.h"
#include "sge_userprj.h"
#include "sge_pe_task.h"
#include "sge_manop.h"
#include "sge_host.h"
#include "sge_sharetree.h"
#include "version.h"
#include "sge_schedd_conf.h"
#include "sge_conf.h"
#include "sge_calendar.h"
#include "sge_report.h"
#include "sge_queue_event_master.h"

#include "msg_common.h"
#include "msg_qmaster.h"
#include "msg_gdilib.h"
#include "msg_sgeobjlib.h"

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
*
*  NOTES
*     MT-NOTE: report_list_send() is not MT safe (assumptions)
*******************************************************************************/
int report_list_send(const lList *rlp, const char *rhost,
                     const char *commproc, int id,
                     int synchron, u_long32 *mid)
{
   sge_pack_buffer pb;
   int ret, size;
   lList *alp = NULL;

   DENTER(TOP_LAYER, "report_list_send");

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

   ret = sge_send_any_request(synchron, mid, rhost, commproc, id, &pb, TAG_REPORT_REQUEST, 0, &alp);
   clear_packbuffer(&pb);
   answer_list_output (&alp);

   DEXIT;
   return ret;
}
