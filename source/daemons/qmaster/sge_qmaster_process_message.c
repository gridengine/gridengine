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

#include "sge_qmaster_process_message.h"

#include <unistd.h>
#include "sgermon.h"
#include "commlib.h"
#include "sge_time.h"
#include "sge_event_master.h"
#include "sge_any_request.h"
#include "sig_handlers.h"
#include "sge_log.h"
#include "sge_gdi_request.h"
#include "msg_qmaster.h"
#include "sge_string.h"
#include "sge_c_gdi.h"
#include "sge_c_ack.h"
#include "sge_c_report.h"


/****** sge_qmaster_process_message() *********************************************
*  NAME
*     sge_qmaster_process_message() -- process message
*
*  SYNOPSIS
*     void *sge_qmaster_process_message(void *anArg)
*
*  FUNCTION
*     This function is intended to be used as a 'thread function' later. Hence
*     the function signature.
*
*  INPUTS
*     void *anArg - not used
*
*  RESULT
*     void * - returns 'anArg'
*
******************************************************************************/
void *sge_qmaster_process_message(void *anArg)
{
   enum { TIMELEVEL = 0 };

   char host[MAXHOSTLEN];
   char commproc[MAXCOMPONENTLEN];
   u_short id;
   int i, tag;
   time_t now, next_flush;
   int old_timeout, rcv_timeout;
   sge_pack_buffer pb;
   sge_gdi_request *gdi = NULL;
   sge_gdi_request *ar = NULL;
   sge_gdi_request *an = NULL;
   sge_gdi_request *answer = NULL;
   lList *report_list = NULL;


   DENTER(TOP_LAYER, "sge_qmaster_process_message");
   
   host[0] = '\0';
   commproc[0] = '\0';
   id = 0;
   tag = 0; /* we take everyting */

   now = sge_get_gmt();
   next_flush = sge_next_flush(now);
   DPRINTF(("next_flush: "u32" now: "u32"\n", next_flush, now));
   
   old_timeout = commlib_state_get_timeout_srcv();

   if (next_flush && ((next_flush - now) >= 0))
      rcv_timeout = MIN(MAX(next_flush - now, 2), 20);
   else
      rcv_timeout = 20;
      
   set_commlib_param(CL_P_TIMEOUT_SRCV, rcv_timeout, NULL, NULL);

   DPRINTF(("setting sync receive timeout to %d seconds\n", rcv_timeout));

   i = sge_get_any_request(host, commproc, &id, &pb, &tag, 1);

   set_commlib_param(CL_P_TIMEOUT_SRCV, old_timeout, NULL, NULL);

   if (sigpipe_received) {
      sigpipe_received = 0;
      INFO((SGE_EVENT, "SIGPIPE received"));
   }
   
   if (i != CL_OK) {
      sge_stopwatch_log(TIMELEVEL, "sge_get_any_request != 0");
      
      if ( i != COMMD_NACK_TIMEOUT ) {
         DPRINTF(("Problems reading request: %s\n", cl_errstr(i)));
         sleep(2);
      }
      return anArg;              
   }
   else {
      sge_stopwatch_log(TIMELEVEL, "sge_get_any_request == 0");
      sge_stopwatch_start(TIMELEVEL);
   }

   switch (tag) {

   /* ======================================== */
#ifdef SECURE
   case TAG_SEC_ANNOUNCE:    /* completly handled in libsec  */
      clear_packbuffer(&pb);
      sge_stopwatch_log(TIMELEVEL, "request handling SEC_ANNOUNCE");
      break;
#endif

   /* ======================================== */
   case TAG_GDI_REQUEST:

      if (sge_unpack_gdi_request(&pb, &gdi)) {
         ERROR((SGE_EVENT,MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI,
            host, commproc, (int)id));
         clear_packbuffer(&pb);   
         break;
      }
      clear_packbuffer(&pb);

      for (ar = gdi; ar; ar = ar->next) { 
          
         /* use id, commproc and host for authentication */  
         ar->id = id;
         ar->commproc = sge_strdup(NULL, commproc); 
         ar->host = sge_strdup(NULL, host);

#ifndef __SGE_NO_USERMAPPING__
         /* perform administrator user mapping with sge_gdi_request
            structure, this can change the user name (ar->user) */
         sge_map_gdi_request(ar);
#endif

         if (ar == gdi) {
            answer = an = new_gdi_request();
         }
         else {
            an->next = new_gdi_request();
            an = an->next;
         }

         sge_c_gdi(host, ar, an);
      }

      sge_send_gdi_request(0, host, commproc, (int)id, answer);
      answer = free_gdi_request(answer);
      gdi = free_gdi_request(gdi);
      sge_stopwatch_log(TIMELEVEL, "request handling GDI_REQUEST");

      break;
   /* ======================================== */
   case TAG_ACK_REQUEST:

      DPRINTF(("SGE_ACK_REQUEST(%s/%s/%d)\n", host, commproc, id));

      sge_c_ack(host, commproc, &pb);
      clear_packbuffer(&pb);
      sge_stopwatch_log(TIMELEVEL, "request handling ACK_REQUEST");
      break;

   /* ======================================== */
   case TAG_EVENT_CLIENT_EXIT:

      DPRINTF(("SGE_EVENT_CLIENT_EXIT(%s/%s/%d)\n", host, commproc, id));

      sge_event_client_exit(host, commproc, &pb);
      clear_packbuffer(&pb);
      sge_stopwatch_log(TIMELEVEL, "request handling EVENT_CLIENT_EXIT");
      break;

   /* ======================================== */
   case TAG_REPORT_REQUEST: 

      DPRINTF(("SGE_REPORT(%s/%s/%d)\n", host, commproc, id));

      if (cull_unpack_list(&pb, &report_list)) {
         ERROR((SGE_EVENT,MSG_CULL_FAILEDINCULLUNPACKLISTREPORT));
         break;
      }
      clear_packbuffer(&pb);

      sge_c_report(host, commproc, id, report_list);
      lFreeList(report_list);
      report_list = NULL;
      sge_stopwatch_log(TIMELEVEL, "request handling REPORT");
      break;

   /* ======================================== */
   default:
      DPRINTF(("***** UNKNOWN TAG TYPE %d\n", tag));
      clear_packbuffer(&pb);
   }
   DEXIT;
   return anArg; 
} /* sge_qmaster_process_message */

