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

#include <string.h>
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
#include "sge_qmaster_main.h"


#ifdef ENABLE_NGC
typedef struct {
   char snd_host[MAXHOSTLEN];      /* sender hostname; NULL -> all */
   char snd_name[MAXHOSTLEN];      /* sender name (aka 'commproc'); NULL -> all */
   u_short snd_id;                 /* sender identifier; 0 -> all */
   int tag;                        /* message tag; TAG_NONE -> all */
   u_long32 request_mid;           /* message id of request */
   sge_pack_buffer buf;            /* message buffer */
} struct_msg_t;
#else
typedef struct {
   char snd_host[MAXHOSTLEN];      /* sender hostname; NULL -> all */
   char snd_name[MAXCOMPONENTLEN]; /* sender name (aka 'commproc'); NULL -> all */
   u_short snd_id;                 /* sender identifier; 0 -> all */
   int tag;                        /* message tag; TAG_NONE -> all */
   sge_pack_buffer buf;            /* message buffer */
} struct_msg_t;
#endif

static int determine_timeout(void);
static void do_gdi_request(struct_msg_t *aMsg);
static void do_report_request(struct_msg_t *aMsg);


/****** sge_qmaster_process_message/sge_qmaster_process_message() **************
*  NAME
*     sge_qmaster_process_message() -- Entry point for qmaster message handling
*
*  SYNOPSIS
*     void* sge_qmaster_process_message(void *anArg) 
*
*  FUNCTION
*     Determine timeout for communication. Get a pending message. Handle message
*     based on message tag.
* 
*
*  INPUTS
*     void *anArg - none 
*
*  RESULT
*     void* - none
*
*  NOTES
*     'sge_get_any_request()' could raise a signal 'SIGPIPE'. This happens,
*     if we try to write a socket which has been closed by the receiving
*     side. In this case we just 'ignore' the signal by resetting the
*     'sigpipe_received' variable.
*     
*     This function is intended to be used as a 'thread function' Hence the 
*     signature.
*
*     MT-NOTE: thread safety needs to be verified!
*
*******************************************************************************/
void *sge_qmaster_process_message(void *anArg)
{
   int new; /* timeout values */
#ifdef ENABLE_NGC
#else
   int old;
#endif
   int res;
   struct_msg_t msg;

   DENTER(TOP_LAYER, "sge_qmaster_process_message");
   
   memset((void*)&msg, 0, sizeof(struct_msg_t));
   
   new = determine_timeout();

#ifdef ENABLE_NGC
#else
   old = commlib_state_get_timeout_srcv();

   DPRINTF(("setting sync receive timeout to %d seconds\n", new));
   set_commlib_param(CL_P_TIMEOUT_SRCV, new, NULL, NULL);
#endif
   res = sge_get_any_request(msg.snd_host, msg.snd_name, &msg.snd_id, &msg.buf, &msg.tag, 1, 0, &msg.request_mid);
#ifdef ENABLE_NGC
#else
   set_commlib_param(CL_P_TIMEOUT_SRCV, old, NULL, NULL);
#endif

   if (sigpipe_received) {
      sigpipe_received = 0;
      INFO((SGE_EVENT, "SIGPIPE received"));
   }
   
#ifdef ENABLE_NGC
   if (res != CL_RETVAL_OK) {
      DPRINTF(("sge_get_any_request returned: %s\n", cl_get_error_text(res)));
      return anArg;              
   }
#else
   if (res != CL_OK) {
      DPRINTF(("sge_get_any_request returned: %s\n", cl_errstr(res)));
      return anArg;              
   }
#endif

   switch (msg.tag)
   {
      case TAG_SEC_ANNOUNCE:
         break; /* All processing done in libsec */
      case TAG_GDI_REQUEST:
         do_gdi_request(&msg);
         break;
      case TAG_ACK_REQUEST:
         sge_c_ack(msg.snd_host, msg.snd_name, &(msg.buf));
         break;
      case TAG_EVENT_CLIENT_EXIT:
         sge_event_client_exit(msg.snd_host, msg.snd_name, &(msg.buf));
         break;
      case TAG_REPORT_REQUEST: 
         do_report_request(&msg);
         break;
      default:
         DPRINTF(("***** UNKNOWN TAG TYPE %d\n", msg.tag));
   }
   clear_packbuffer(&(msg.buf));

   DEXIT;
   return anArg; 
} /* sge_qmaster_process_message */

/****** sge_qmaster_process_message/determine_timeout() ************************
*  NAME
*     determine_timeout() -- determine communication timeout
*
*  SYNOPSIS
*     static int determine_timeout(void) 
*
*  FUNCTION
*     The timeout is affected by pending events. Pending events need to be
*     delivered on time to registered event clients. If there are pending events,
*     the (positiv) delta between event delivery time and current time is
*     calculated. If this delta is less than 'DEFAULT_TIMEOUT', it is used as
*     timeout. Otherwise 'DEFAULT_TIMEOUT' is used. The minimal timeout used is
*     'MIN_TIMEOUT'. With no pending events, 'DEFAULT_TIMEOUT' is used.
*
*  RESULT
*     int - timeout value 
*
*  NOTES
*     MT-NOTE: determine_timeout() is thread safe
*
*******************************************************************************/
static int determine_timeout(void)
{
   enum { MIN_TIMEOUT = 2, DEFAULT_TIMEOUT = 20 }; /* seconds */

   time_t now;   /* current time */
   time_t flush; /* time next event flush is due */
   int res = DEFAULT_TIMEOUT;

   DENTER(TOP_LAYER, "determine_timeout");

   now = sge_get_gmt();
   flush = sge_next_flush(now);

   if (flush && ((flush - now) >= 0)) {
      res = MIN(MAX((flush - now), MIN_TIMEOUT), DEFAULT_TIMEOUT);
   }

   DEXIT;
   return res;
} /* determine_timeout */

/****** sge_qmaster_process_message/do_gdi_request() ***************************
*  NAME
*     do_gdi_request() -- Process GDI request messages
*
*  SYNOPSIS
*     static void do_gdi_request(struct_msg_t *aMsg) 
*
*  FUNCTION
*     Process GDI request messages (TAG_GDI_REQUEST). Unpack a GDI request from
*     the pack buffer, which is part of 'aMsg'. Process GDI request and send a
*     response to 'commd'.
*
*  INPUTS
*     struct_msg_t *aMsg - GDI request message
*
*  RESULT
*     void - none
*
*  NOTES
*     A pack buffer may contain more than a single GDI request. This is a so 
*     called 'multi' GDI request. In case of a multi GDI request, the 'sge_gdi_request'
*     structure filled in by 'sge_unpack_gdi_request' is the head of a linked
*     list of 'sge_gdi_request' structures.
*
*******************************************************************************/
static void do_gdi_request(struct_msg_t *aMsg)
{
   enum { ASYNC = 0, SYNC = 1 };

   sge_pack_buffer *buf = &(aMsg->buf);
   sge_gdi_request *req_head = NULL;  /* head of request linked list */
   sge_gdi_request *resp_head = NULL; /* head of response linked list */
   sge_gdi_request *req = NULL;
   sge_gdi_request *resp = NULL;

   DENTER(TOP_LAYER, "do_gid_request");

   if (sge_unpack_gdi_request(buf, &req_head)) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI, (char *)aMsg->snd_host, (char *)aMsg->snd_name, (int)aMsg->snd_id));
      return;
   }
   resp_head = new_gdi_request();

   for (req = req_head; req; req = req->next) {
      req->id = aMsg->snd_id;
      req->commproc = sge_strdup(NULL, aMsg->snd_name);
      req->host = sge_strdup(NULL, aMsg->snd_host);

#ifndef __SGE_NO_USERMAPPING__
      sge_map_gdi_request(req);
#endif
   
      if (req == req_head) {
         resp = resp_head;
      } else {
         resp->next = new_gdi_request();
         resp = resp->next;
      }

      sge_c_gdi(aMsg->snd_host, req, resp);
   }

   sge_send_gdi_request(ASYNC, aMsg->snd_host, aMsg->snd_name, (int)aMsg->snd_id, resp_head,NULL,aMsg->request_mid);

   free_gdi_request(resp_head);
   free_gdi_request(req_head);

   DEXIT;
   return;
} /* do_gdi_request */

/****** sge_qmaster_process_message/do_report_request() ************************
*  NAME
*     do_report_request() -- Process execd load report 
*
*  SYNOPSIS
*     static void do_report_request(struct_msg_t *aMsg) 
*
*  FUNCTION
*     Process execd load reports (TAG_REPORT_REQUEST). Unpack a CULL list with
*     the load report from the pack buffer, which is part of 'aMsg'. Process
*     execd load report.
*
*  INPUTS
*     struct_msg_t *aMsg - execd load report message
*
*  RESULT
*     void - none 
*
*******************************************************************************/
static void do_report_request(struct_msg_t *aMsg)
{
   lList *rep = NULL;

   DENTER(TOP_LAYER, "do_report_request");

   if (cull_unpack_list(&(aMsg->buf), &rep)) {
      ERROR((SGE_EVENT,MSG_CULL_FAILEDINCULLUNPACKLISTREPORT));
      return;
   }

   sge_c_report(aMsg->snd_host, aMsg->snd_name, aMsg->snd_id, rep);
   lFreeList(rep);

   DEXIT;
   return;
} /* do_report_request */
