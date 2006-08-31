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
#include "sge_string.h"
#include "sge_c_gdi.h"
#include "sge_c_report.h"
#include "sge_qmaster_main.h"
#include "sgeobj/sge_answer.h"
#include "sge_prog.h"
#include "sge_mtutil.h"
#include "sge_conf.h"
#include "sge_bootstrap.h"
#include "sge_security.h"
#include "sge_ack.h"
#include "sge_ja_taskL.h"
#include "sge_job.h"
#include "sge_qinstance.h"
#include "sge_cqueue.h"
#include "sge_lock.h"
#include "spool/sge_spooling.h"

#include "msg_qmaster.h"
#include "msg_common.h"
/*#include "msg_gdilib.h"*/

typedef struct {
   char snd_host[CL_MAXHOSTLEN]; /* sender hostname; NULL -> all              */
   char snd_name[CL_MAXHOSTLEN]; /* sender name (aka 'commproc'); NULL -> all */
   u_short snd_id;            /* sender identifier; 0 -> all               */
   int tag;                   /* message tag; TAG_NONE -> all              */
   u_long32 request_mid;      /* message id of request                     */
   sge_pack_buffer buf;       /* message buffer                            */
} struct_msg_t;

/***************************************************
 *
 * The next section ensures, that GDI multi request
 * will be handled atomic and that other requests do
 * not interfer with the GDI multi get requsts. 
 *
 * Some assumption have been made for the current
 * implementation. They should minimize the performance
 * impact of this serialisation.
 *
 * Assumption:
 * 1) If the first GDI multi request is a get request
 *    all GDI request in the GDI multi are get requests
 *
 * 2) if the first GDI multi request is not a get request
 *    all GDI requests are not a get request
 * 
 * Based on this assumption we can greate the following
 * execution matrix (GDI is used for atomix GDI requests
 * and load/job reports:
 *
 *          |  GDI     |  M-GDI-R  | M-GDI-W
 *  --------|----------|-----------|---------
 *  GDI     | parallel |  seriel   | parallel
 *  --------|----------|-----------|---------
 *  M-GDI-R | seriel   | parallel  | seriel
 *  --------|----------|-----------|---------
 *  M-GDI-W | parallel | seriel    | parallel
 *          |          |           |
 *
 * states: 
 *  NONE     0
 *  GDI      1
 *  M-GDI-R  2
 *  M-GDI-W  1 
 *
 * Based on the matrix, we do not need seperated
 * states for GDI and M-GDI-W.
 *
 * The implementation will allow a new requst to
 * execute, when no other request is executed or
 * the exectuted request as the same state as the
 * new one. If that is not the case, the new request
 * will be blocked until the others have finished.
 *
 * Implementation:
 *
 *  eval_message_and_block - eval message and assign states
 *  eval_gdi_and_block     - eval gdi and assign states
 *
 *  eval_atomic            - check current execution and block
 *
 *  eval_atomic_end        - release current block
 */
 
typedef enum {
   ATOMIC_NONE = 0,
   ATOMIC_SINGLE = 1,
   ATOMIC_MULTIPLE_WRITE = 1,
   ATOMIC_MULTIPLE_READ = 2
} request_handling_t;

typedef struct {
   request_handling_t type;      /* execution type*/
   int                counter;   /* number of requests executed of type */
   pthread_cond_t     cond_var;  /* used to block other threads */   
   bool               signal;    /* need to signal? */
   pthread_mutex_t    mutex;     /* mutex to gard this structure */
} message_control_t;

static message_control_t Master_Control = {ATOMIC_NONE, 0, PTHREAD_COND_INITIALIZER, false, PTHREAD_MUTEX_INITIALIZER};

static request_handling_t eval_message_and_block(struct_msg_t msg);
static request_handling_t eval_gdi_and_block(sge_gdi_request *req_head);
static void eval_atomic(request_handling_t type);
static void eval_atomic_end(request_handling_t type); 

static request_handling_t do_gdi_request(struct_msg_t*, monitoring_t *monitor);
static request_handling_t do_report_request(struct_msg_t*, monitoring_t *monitor);
static void do_event_client_exit(struct_msg_t *aMsg, monitoring_t *monitor);
static void do_c_ack(struct_msg_t *aMsg, monitoring_t *monitor);
static void sge_c_job_ack(char *, char *, u_long32, u_long32, u_long32, monitoring_t *monitor);


/****** sge_qmaster_process_message/eval_message_and_block() *******************
*  NAME
*     eval_message_and_block() -- eval a message and proceed or block
*
*  SYNOPSIS
*     static request_handling_t eval_message_and_block(struct_msg_t msg) 
*
*  FUNCTION
*     determines the current block type for a message and proceeds or
*     waits for another thread to finish.
*
*  INPUTS
*     struct_msg_t msg - current message
*
*  RESULT
*     static request_handling_t - block type
*
*  NOTES
*     MT-NOTE: eval_message_and_block() is MT safe 
*
*******************************************************************************/
static request_handling_t 
eval_message_and_block(struct_msg_t msg) 
{
   request_handling_t type;

   DENTER(TOP_LAYER, "eval_message_and_block");
  
   if (msg.tag == TAG_REPORT_REQUEST) {
      type = ATOMIC_SINGLE;
   }
   else {
      type = ATOMIC_NONE;   
   }
  
   eval_atomic(type);
   
   DEXIT;   
   return type;
}

/****** sge_qmaster_process_message/eval_gdi_and_block() ***********************
*  NAME
*     eval_gdi_and_block() -- eval gdi request and proceed or block
*
*  SYNOPSIS
*     static request_handling_t eval_gdi_and_block(sge_gdi_request *req_head) 
*
*  FUNCTION
*     determines the current block type for a gdi request and proceeds or
*     waits for another thread to finish.
*
*  INPUTS
*     sge_gdi_request *req_head - ??? 
*
*  RESULT
*     static request_handling_t - returns block type
*
*
*  NOTES
*     MT-NOTE: eval_gdi_and_block() is  MT safe 
*
*******************************************************************************/
static request_handling_t 
eval_gdi_and_block(sge_gdi_request *req_head) 
{
   request_handling_t type = ATOMIC_NONE;

   DENTER(TOP_LAYER, "eval_gdi_and_block");
  
   if (req_head->next == NULL) {
      type = ATOMIC_SINGLE;     
   }
   else if (req_head->op == SGE_GDI_GET) {
      type = ATOMIC_MULTIPLE_READ; 
   }
   else {
      type = ATOMIC_MULTIPLE_WRITE;
   }
  
   eval_atomic(type);
  
   DEXIT;
   return type;
}

/****** sge_qmaster_process_message/eval_atomic() ******************************
*  NAME
*     eval_atomic() -- check proceed type
*
*  SYNOPSIS
*     static void eval_atomic(request_handling_t type) 
*
*  FUNCTION
*     checks wether the current thread can proceed or if it needs to wait
*     till the next one is done.
*
*  INPUTS
*     request_handling_t type - current block type
*
*  RESULT
*     static void - 
*
*  NOTES
*     MT-NOTE: eval_atomic() is MT safe 
*
*******************************************************************************/
static void 
eval_atomic(request_handling_t type) 
{
   bool cond = false;

   DENTER(TOP_LAYER, "eval_atomicx");
   
   if (type == ATOMIC_NONE) {
      return;
   }

   sge_mutex_lock("message_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DPRINTF(("eval before type %d, counter %d, wait %d --- ntype %d\n", Master_Control.type, 
            Master_Control.counter, Master_Control.signal, type));
   
   do {
      if (Master_Control.type == ATOMIC_NONE) {
         Master_Control.type = type;
         Master_Control.counter = 1;
         cond = true;
      }
      else if (Master_Control.type == type) {
         Master_Control.counter++;
         cond = true;
      }
      else {
         Master_Control.signal = true;
         pthread_cond_wait(&Master_Control.cond_var, &Master_Control.mutex);
      }
   } while (!cond);
  
   DPRINTF(("eval after type %d, counter %d, wait %d --- ntype %d\n\n", Master_Control.type, 
            Master_Control.counter, Master_Control.signal, type));
   
   sge_mutex_unlock("message_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);
   DEXIT; 
}

/****** sge_qmaster_process_message/eval_atomic_end() **************************
*  NAME
*     eval_atomic_end() -- free block
*
*  SYNOPSIS
*     static void eval_atomic_end(request_handling_t type) 
*
*  FUNCTION
*     frees a current block and triggers a possible pending thread
*
*  INPUTS
*     request_handling_t type - the last processing type
*
*  RESULT
*     static void - 
*
*  NOTES
*     MT-NOTE: eval_atomic_end() is MT safe 
*
*******************************************************************************/
static void 
eval_atomic_end(request_handling_t type) 
{
 
   DENTER(TOP_LAYER, "eval_atomic_end");

   if (type == ATOMIC_NONE) {
      return; 
   }

   sge_mutex_lock("message_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DPRINTF(("end before type %d, counter %d, wait %d --- ntype %d\n", Master_Control.type, 
            Master_Control.counter, Master_Control.signal, type));
   
   if (Master_Control.type != type) {
      ERROR((SGE_EVENT, "we have a atomic type missmatch (expected = %d, got = %d\n", Master_Control.type, type));
   }
   
   Master_Control.counter--;
   
   if (Master_Control.counter <= 0) {
      Master_Control.type = ATOMIC_NONE;
   }
   
   if (Master_Control.signal) {
      Master_Control.signal = false;
      pthread_cond_broadcast(&Master_Control.cond_var);
   }
   
   DPRINTF(("end after stype %d, counter %d, wait %d --- ntype %d\n\n", Master_Control.type, 
            Master_Control.counter, Master_Control.signal, type));
   
   sge_mutex_unlock("message_master_mutex", SGE_FUNC, __LINE__, &Master_Control.mutex);

   DEXIT;
}

/****** qmaster/sge_qmaster_process_message/sge_qmaster_process_message() ******
*  NAME
*     sge_qmaster_process_message() -- Entry point for qmaster message handling
*
*  SYNOPSIS
*     void* sge_qmaster_process_message(void *anArg) 
*
*  FUNCTION
*     Get a pending message. Handle message based on message tag.
*
*  INPUTS
*     void *anArg - none 
*
*  RESULT
*     void* - none
*
*  NOTES
*     MT-NOTE: thread safety needs to be verified!
*     MT-NOTE:
*     MT-NOTE: This function should only be used as a 'thread function'
*
*******************************************************************************/
void *sge_qmaster_process_message(void *anArg, monitoring_t *monitor)
{
   int res;
   struct_msg_t msg;
   request_handling_t type = ATOMIC_NONE;

   DENTER(TOP_LAYER, "sge_qmaster_process_message");
   
   memset((void*)&msg, 0, sizeof(struct_msg_t));

   /*
    * INFO (CR)  
    *
    * The not syncron sge_get_any_request() call will not raise cpu usage to 100%
    * because sge_get_any_request() is doing a cl_commlib_trigger() which will
    * return after the timeout specified at cl_com_create_handle() call in prepare_enroll()
    * which is set to 1 second. A syncron receive would result in a unnecessary qmaster shutdown
    * timeout (syncron receive timeout) when no messages are there to read.
    *
    */
   MONITOR_IDLE_TIME((res = sge_get_any_request(msg.snd_host, msg.snd_name, &msg.snd_id, &msg.buf, 
                                &msg.tag, 1, 0, &msg.request_mid)), monitor, mconf_get_monitor_time(),
                                mconf_is_monitor_message());
   
   MONITOR_MESSAGES(monitor);      
   
   if (res != CL_RETVAL_OK) {
      DPRINTF(("%s returned: %s\n", SGE_FUNC, cl_get_error_text(res)));
      return anArg;              
   }

   switch (msg.tag)
   {
      case TAG_GDI_REQUEST: 
         type = do_gdi_request(&msg, monitor);
         break;
      case TAG_ACK_REQUEST:
         do_c_ack(&msg, monitor);
         break;
      case TAG_EVENT_CLIENT_EXIT:
         do_event_client_exit(&msg, monitor);
         MONITOR_ACK(monitor);   
         break;
      case TAG_REPORT_REQUEST: 
         type = do_report_request(&msg, monitor);
         break;
      default: 
         DPRINTF(("***** UNKNOWN TAG TYPE %d\n", msg.tag));
   }
   
   eval_atomic_end(type);

   clear_packbuffer(&(msg.buf));
  
   DEXIT;
   return anArg; 
} /* sge_qmaster_process_message */

/****** sge_qmaster_process_message/do_gdi_request() ***************************
*  NAME
*     do_gdi_request() -- Process GDI request messages
*
*  SYNOPSIS
*     static void do_gdi_request(struct_msg_t *aMsg) 
*
*  FUNCTION
*     Process GDI request messages (TAG_GDI_REQUEST). Unpack a GDI request 
*     from the pack buffer, which is part of 'aMsg'. 
*     Process GDI request and send a response to 'commd'.
*
*  INPUTS
*     struct_msg_t *aMsg - GDI request message
*
*  RESULT
*     void - none
*
*  NOTES
*     A pack buffer may contain more than a single GDI request. 
*     This is a so called 'multi' GDI request. In case of a multi GDI 
*     request, the 'sge_gdi_request' structure filled in by 
*     'sge_unpack_gdi_request' is the head of a linked list of 
*     'sge_gdi_request' structures.
*******************************************************************************/
static request_handling_t 
do_gdi_request(struct_msg_t *aMsg, monitoring_t *monitor)
{
   request_handling_t type = ATOMIC_NONE;

   sge_pack_buffer *buf = &(aMsg->buf);
   sge_gdi_request *req_head = NULL;  /* head of request linked list */
   sge_gdi_request *resp_head = NULL; /* head of response linked list */
   sge_pack_buffer pb;

   DENTER(TOP_LAYER, "do_gdi_request");

   if (sge_unpack_gdi_request(buf, &req_head)) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDINSGEUNPACKGDIREQUEST_SSI, 
            (char *)aMsg->snd_host, (char *)aMsg->snd_name, 
            (int)aMsg->snd_id));
   } else {
      enum { ASYNC = 0, SYNC = 1 };
      lList *alp = NULL;
      sge_gdi_request *req = NULL;
      sge_gdi_request *resp = NULL;

      resp_head = new_gdi_request();
      init_packbuffer(&pb, 0, 0);

      MONITOR_WAIT_TIME((type = eval_gdi_and_block(req_head)), monitor);

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

         /* this is needed to identify a multi-gdi pack buffer */
         resp->next = ((req->next != NULL) ? resp : NULL);

         sge_c_gdi(aMsg->snd_host, req, resp, &pb, monitor);
      }

#if 0
      sge_send_gdi_request(ASYNC, aMsg->snd_host, aMsg->snd_name,
                        (int)aMsg->snd_id, resp_head, NULL, aMsg->request_mid,
                        &alp);
#else
      sge_send_any_request(ASYNC, NULL, aMsg->snd_host,
                           aMsg->snd_name, aMsg->snd_id, &pb,
                           TAG_GDI_REQUEST, aMsg->request_mid, &alp);
#endif

      clear_packbuffer(&pb);
      MONITOR_MESSAGES_OUT(monitor);
      answer_list_output (&alp);
   }
   
   free_gdi_request(resp_head);
   free_gdi_request(req_head);

   DEXIT;
   return type;
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
static request_handling_t do_report_request(struct_msg_t *aMsg, monitoring_t *monitor)
{
   lList *rep = NULL;
   request_handling_t type = ATOMIC_NONE;
   const char *admin_user = bootstrap_get_admin_user();

   DENTER(TOP_LAYER, "do_report_request");

   /* Load reports are only accepted from admin/root user */
   if (false == sge_security_verify_unique_identifier(true, admin_user, uti_state_get_sge_formal_prog_name(), 0,
                                            aMsg->snd_host, aMsg->snd_name, aMsg->snd_id)) {
      DEXIT;
      return type;
    }

   if (cull_unpack_list(&(aMsg->buf), &rep)) {
      ERROR((SGE_EVENT,MSG_CULL_FAILEDINCULLUNPACKLISTREPORT));
      return type;
   }

   MONITOR_WAIT_TIME((type = eval_message_and_block(*aMsg)), monitor); 

   sge_c_report(aMsg->snd_host, aMsg->snd_name, aMsg->snd_id, rep, monitor);
   lFreeList(&rep);

   DEXIT;
   return type;
} /* do_report_request */

/****** qmaster/sge_qmaster_process_message/do_event_client_exit() *************
*  NAME
*     do_event_client_exit() -- handle event client exit message 
*
*  SYNOPSIS
*     static void do_event_client_exit(const char *aHost, const char *aSender, 
*     sge_pack_buffer *aBuffer) 
*
*  FUNCTION
*     Handle event client exit message. Extract event client id from pack
*     buffer. Remove event client. 
*
*  INPUTS
*     const char *aHost        - sender 
*     const char *aSender      - communication endpoint 
*     sge_pack_buffer *aBuffer - buffer 
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: do_event_client_exit() is NOT MT safe. 
*
*******************************************************************************/
static void do_event_client_exit(struct_msg_t *aMsg, monitoring_t *monitor)
{
   u_long32 client_id = 0;

   DENTER(TOP_LAYER, "do_event_client_exit");

   if (unpackint(&(aMsg->buf), &client_id) != PACK_SUCCESS)
   {
      ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 1));
      DPRINTF(("%s: client id unpack failed - host %s - sender %s\n", SGE_FUNC, aMsg->snd_host, aMsg->snd_name));
      DEXIT;
      return;
   }

   DPRINTF(("%s: remove client " sge_u32 " - host %s - sender %s\n", SGE_FUNC, client_id, aMsg->snd_host, aMsg->snd_name));

   /* 
   ** check for scheduler shutdown if the request comes from admin or root 
   ** TODO: further enhancement could be matching the owner of the event client 
   **       with the request owner
   */
   if (client_id == 1) {
      const char *admin_user = bootstrap_get_admin_user();
      if (false == sge_security_verify_unique_identifier(true, admin_user, uti_state_get_sge_formal_prog_name(), 0,
                                               aMsg->snd_host, aMsg->snd_name, aMsg->snd_id)) {
         DEXIT;
         return;
       }
   }
   
   sge_remove_event_client(client_id);

   DEXIT;
   return;
} /* do_event_client_exit() */


/****************************************************
 Master code.
 Handle ack requests
 Ack requests can be packed together in one packet.

 These are:
 - an execd sends an ack for a received job
 - an execd sends an ack for a signal delivery
 - the schedd sends an ack for received events

 if the counterpart uses the dispatcher ack_tag is the
 TAG we sent to the counterpart.
 ****************************************************/
static void do_c_ack(struct_msg_t *aMsg, monitoring_t *monitor) 
{
   u_long32 ack_tag, ack_ulong, ack_ulong2;
   const char *admin_user = bootstrap_get_admin_user();

   DENTER(TOP_LAYER, "do_c_ack");

   
   /* Do some validity tests */
   while (!unpackint(&(aMsg->buf), &ack_tag)) {
      if (unpackint(&(aMsg->buf), &ack_ulong)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 1));
         DEXIT;
         return;
      }
      if (unpackint(&(aMsg->buf), &ack_ulong2)) {
         ERROR((SGE_EVENT, MSG_COM_UNPACKINT_I, 2));
         DEXIT;
         return;
      }

      DPRINTF(("ack_ulong = %ld, ack_ulong2 = %ld\n", ack_ulong, ack_ulong2));
      switch (ack_tag) {
      case TAG_SIGJOB:
      case TAG_SIGQUEUE:
         MONITOR_EACK(monitor);
         /* 
         ** accept only ack requests from admin or root
         */
         if (false == sge_security_verify_unique_identifier(true, admin_user, uti_state_get_sge_formal_prog_name(), 0,
                                                  aMsg->snd_host, aMsg->snd_name, aMsg->snd_id)) {
            DEXIT;
            return;
         }
         /* an execd sends a job specific acknowledge ack_ulong == jobid of received job */
         sge_c_job_ack(aMsg->snd_host, aMsg->snd_name, ack_tag, ack_ulong, ack_ulong2, monitor);
         break;

      case ACK_EVENT_DELIVERY:
         MONITOR_ACK(monitor); 
         /* 
         ** TODO: in this case we should check if the event belongs to the user
         **       who send the request
         */
         sge_handle_event_ack(ack_ulong2, (ev_event)ack_ulong);
         break;

      default:
         WARNING((SGE_EVENT, MSG_COM_UNKNOWN_TAG, sge_u32c(ack_tag)));
         break;
      }
   }
  
   DEXIT;
   return;
}

/***************************************************************/
static void sge_c_job_ack(char *host, char *commproc, u_long32 ack_tag, 
                          u_long32 ack_ulong, u_long32 ack_ulong2, 
                          monitoring_t *monitor) 
{
   lListElem *qinstance = NULL;
   lListElem *jep = NULL;
   lListElem *jatep = NULL;
   lList *answer_list = NULL;

   DENTER(TOP_LAYER, "sge_c_job_ack");

   MONITOR_WAIT_TIME(SGE_LOCK(LOCK_GLOBAL, LOCK_WRITE), monitor); 

   if (strcmp(prognames[EXECD], commproc) &&
      strcmp(prognames[QSTD], commproc)) {
      ERROR((SGE_EVENT, MSG_COM_ACK_S, commproc));
      SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
      DEXIT;
      return;
   }

   switch (ack_tag) {
   case TAG_SIGJOB:
      DPRINTF(("TAG_SIGJOB\n"));
      /* ack_ulong is the jobid */
      if (!(jep = job_list_locate(*(object_type_get_master_list(SGE_TYPE_JOB)), ack_ulong))) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKOWNJOB_U, sge_u32c(ack_ulong) ));
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return;
      }
      jatep = job_search_task(jep, NULL, ack_ulong2);
      if (jatep == NULL) {
         ERROR((SGE_EVENT, MSG_COM_ACKEVENTFORUNKNOWNTASKOFJOB_UU, sge_u32c(ack_ulong2), sge_u32c(ack_ulong)));
         SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
         DEXIT;
         return;
      }

      DPRINTF(("JOB "sge_u32": SIGNAL ACK\n", lGetUlong(jep, JB_job_number)));
      lSetUlong(jatep, JAT_pending_signal, 0);
      te_delete_one_time_event(TYPE_SIGNAL_RESEND_EVENT, ack_ulong, ack_ulong2, NULL);
      {
         dstring buffer = DSTRING_INIT;
         spool_write_object(&answer_list, spool_get_default_context(), jep, 
                            job_get_key(lGetUlong(jep, JB_job_number), 
                                        ack_ulong2, NULL, &buffer), 
                            SGE_TYPE_JOB);
         sge_dstring_free(&buffer);
      }
      answer_list_output(&answer_list);
      
      break;

   case TAG_SIGQUEUE:
      {
         lListElem *cqueue = NULL;

         for_each(cqueue, *(object_type_get_master_list(SGE_TYPE_CQUEUE))) {
            lList *qinstance_list = lGetList(cqueue, CQ_qinstances);

            qinstance = lGetElemUlong(qinstance_list, 
                                      QU_queue_number, ack_ulong);
            if (qinstance != NULL) {
               break;
            }
         }
         if (qinstance == NULL) {
            ERROR((SGE_EVENT, MSG_COM_ACK_QUEUE_U, sge_u32c(ack_ulong)));
            SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
            DEXIT;
            return;
         }
      }
      
      DPRINTF(("QUEUE %s: SIGNAL ACK\n", lGetString(qinstance, QU_full_name)));

      lSetUlong(qinstance, QU_pending_signal, 0);
      te_delete_one_time_event(TYPE_SIGNAL_RESEND_EVENT, 0, 0, lGetString(qinstance, QU_full_name));
      spool_write_object(&answer_list, spool_get_default_context(), qinstance, 
                         lGetString(qinstance, QU_full_name), SGE_TYPE_QINSTANCE);
      answer_list_output(&answer_list);
      break;

   default:
      ERROR((SGE_EVENT, MSG_COM_ACK_UNKNOWN));
   }

   SGE_UNLOCK(LOCK_GLOBAL, LOCK_WRITE);
   
   DEXIT;
   return;
}
